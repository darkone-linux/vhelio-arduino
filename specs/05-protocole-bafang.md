# 05 — Écoute passive de la liaison UART Bafang

Fonction de **confort** (principe P2) : sa perte totale n'a aucun effet sur les
feux, les clignotants ni le stop, et `BAFANG_ENABLE 0` la supprime sans rien
casser d'autre.

## 1. Ce qui est certain, ce qui ne l'est pas

Le protocole n'est pas documenté par le constructeur. La distinction ci-dessous
est explicite : **le firmware est conçu pour fonctionner même si la seconde
partie est fausse sur votre matériel.**

### Établi

- Liaison **1200 bauds, 8N1**, niveaux **5 V TTL**, repos à l'état haut.
- L'afficheur est **maître** : il interroge, le contrôleur répond. Le
  contrôleur n'émet jamais spontanément.
- Une requête de lecture commence par `0x11`, une écriture par `0x16`. **Le
  firmware n'émet ni l'une ni l'autre.**
- Une réponse commence par le numéro de registre et se termine par une **somme
  de contrôle** = somme des octets précédents, modulo 256.

### Non établi — à calibrer sur votre matériel

- La correspondance registre → grandeur varie selon la série de moteur (BBS02,
  BBSHD, série G) et la version de firmware du contrôleur.
- Le codage de la vitesse : selon les sources, dixièmes de km/h **ou** période
  de rotation de roue en millisecondes. Les deux sont implémentés et
  sélectionnables.
- La longueur exacte de certaines réponses.

## 2. Point de piquage

On sniffe **uniquement la ligne TX du contrôleur**, celle qui porte les
réponses vers l'afficheur.

```
Contrôleur ──── TX ────┬──────────────> Afficheur (inchangé)
                       │
                      1 kΩ
                       │
                       └──────────────> A1 du Nano (RX logiciel, PCINT9)

Contrôleur ──── GND ───────────────────> GND de la carte DN22D08
```

Règles impératives :

- **Ne jamais toucher au fil d'alimentation** du connecteur afficheur : il
  porte la tension batterie (48 V) sur certains modèles. Seuls les fils
  **données** et **masse** sont concernés.
- La résistance série de 1 kΩ limite le courant en cas d'erreur de câblage et
  ne charge pas la ligne.
- **D13, déclarée comme TX du port logiciel, reste non câblée.** C'est ce qui
  rend l'émission physiquement impossible, et pas seulement évitée par le code
  (F-5.1). Conséquence visible : `SoftwareSerial` maintient ce TX à l'état haut
  au repos, donc **la LED intégrée du Nano reste allumée en fixe** et ne peut
  pas servir de témoin.
- Utiliser une **dérivation en Y** au format d'origine plutôt que de couper le
  faisceau : le montage reste réversible.
- Le convertisseur 48/12 V étant **non isolé**, la masse 12 V est déjà celle du
  contrôleur : il n'y a rien de particulier à faire (`04` §5, cas 1).

## 3. Segmentation des trames

On ne voit que des réponses, sans les requêtes qui les délimiteraient : la
segmentation repose donc sur le **silence inter-trames**.

À 1200 bauds un octet occupe 10 bits ≈ **8,3 ms** ; les octets d'une même trame
se suivent sans interruption, et l'afficheur interroge toutes les 100 à 300 ms.
Un silence de **30 ms** (`BAFANG_FRAME_GAP_MS`) est donc un séparateur fiable.

Chaque trame isolée est validée par sa somme de contrôle. Une trame fausse est
**silencieusement rejetée** et comptée (`rej=` au journal). Un taux de rejet
élevé signale un mauvais piquage, une masse absente ou un mauvais débit — pas
un bug de décodage.

## 4. Table de registres par défaut

Pré-remplie dans `bafang.cpp`, **à valider en mode apprentissage**.

| 1er octet | Longueur | Grandeur supposée | Décodage |
|---|---|---|---|
| `0x20` | 3 | Niveau de charge | `soc_pct = f[1]`, rejeté si > 100 |
| `0x0A` | 3 | Courant moteur | `courant_A×10 = f[1] × 5` (unité 0,5 A) |
| `0x11` | 4 | Vitesse | `raw = (f[1] << 8) \| f[2]`, puis §5 |

Tout autre premier octet est ignoré sans que ce soit une erreur.

## 5. Les deux interprétations de la vitesse

Sélection par `BAFANG_SPEED_FORMULA`.

**Formule 0 — valeur directe** : `vitesse_kmh_x10 = raw`.

**Formule 1 — période de roue** *(défaut)* :
`vitesse_kmh_x10 = 36 × circonférence_mm / raw_ms`.

Contrôle de cohérence, circonférence 2200 mm : à 25 km/h un tour dure
2,2 / 6,94 = 317 ms, et 36 × 2200 / 317 = 250, soit 25,0 km/h.

Le firmware rejette les valeurs aberrantes : `raw_ms` hors de [50, 5000] ms —
au-delà de 5000, la roue est déclarée à l'arrêt, ce qui est **valide** et non
inconnu — ou vitesse résultante > 99,9 km/h.

## 6. Mode apprentissage

`BAFANG_LEARN_MODE 1` publie chaque trame **valide** sur le port série :

```
[BAFANG] 20 5A 7A            (3o)
[BAFANG] 0A 0C 16            (3o)
[BAFANG] 11 01 3B 4D         (4o)
```

Méthode de calibration, roue soulevée, véhicule sur béquille :

1. Moteur à l'arrêt, noter les trames stables. Celle qui porte le niveau de
   charge lu sur l'afficheur d'origine identifie le registre SOC.
2. Faire tourner la roue à vitesse croissante. La trame dont un champ varie de
   façon **monotone** est la vitesse. Noter plusieurs couples (valeur brute,
   vitesse affichée) et vérifier laquelle des deux formules du §5 les relie.
3. Mettre de l'assistance en charge : le champ qui suit l'effort est le
   courant.
4. Reporter dans la table de `bafang.cpp`, repasser `BAFANG_LEARN_MODE` à 0.

> Le journal d'état est **suspendu** en mode apprentissage : les deux se
> disputeraient la ligne série.

## 7. Coût réel de l'écoute, et pourquoi c'est acceptable

`SoftwareSerial` désactive les interruptions pendant toute la réception d'un
octet, soit ≈ **8,3 ms** à 1200 bauds. Pendant ce temps les débordements du
Timer0 sont perdus et `millis()` prend du retard : de l'ordre de **2 à 5 % de
dérive**.

| Fonction | Impact | Acceptable ? |
|---|---|---|
| Cadence clignotants | 1,33 → 1,27–1,33 Hz | Oui, la plage réglementaire est 1–2 Hz |
| Anti-rebond freins | 15 → 15,5 ms | Oui |
| Réaction au freinage | retard max +8,3 ms | Oui, imperceptible |
| Maintien coupure moteur | 300 → 315 ms | Oui |
| Odomètre / rappel clignotant | ±5 % | Oui, fonction de confort |

Aucune de ces dérives ne touche significativement une fonction de sécurité. Si
elle devient gênante, `BAFANG_ENABLE 0` la supprime intégralement — et c'est
précisément l'intérêt du principe P2.

## 8. Ce qui a été explicitement écarté

**L'interposition (MITM)** — placer l'Arduino entre afficheur et contrôleur —
permettrait de forcer l'assistance à 0 au freinage, de brider la vitesse ou
d'ajouter des modes. Écartée parce que :

- deux ports logiciels à 1200 bauds avec relayage temps réel saturent les
  interruptions d'un ATmega328P ;
- un firmware planté rendrait le moteur **totalement inopérant**, alors qu'en
  écoute passive il ne se passe rien du tout ;
- la coupure d'assistance recherchée est déjà obtenue, plus simplement et plus
  sûrement, par la ligne frein du contrôleur.

Si le besoin de forçage apparaît, il faudra deux UART matériels : Mega 2560 ou
carte 32 bits (`archives/alternatives-materiel.md`).
