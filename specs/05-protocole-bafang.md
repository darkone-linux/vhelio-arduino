# 05 — Écoute passive de la liaison UART Bafang

## 1. Ce qui est certain, ce qui ne l'est pas

Le protocole Bafang entre afficheur et contrôleur n'est pas documenté par le
constructeur. Ce qui suit combine ce qui est solidement établi par la
rétro-ingénierie communautaire et ce qui ne l'est pas. **La distinction est
explicite** — le firmware est conçu pour fonctionner même si la seconde partie
est fausse sur votre matériel.

### Établi

- Liaison **asynchrone 1200 bauds, 8 bits, sans parité, 1 bit de stop**, niveaux
  logiques **5 V TTL**, repos à l'état haut.
- L'afficheur est **maître** : il interroge, le contrôleur répond. Le contrôleur
  n'émet jamais spontanément.
- Une requête de lecture commence par l'octet **`0x11`**, suivi du numéro de
  registre.
- Une réponse commence par le **numéro de registre** et se termine par une
  **somme de contrôle** = somme de tous les octets précédents, modulo 256.
- Les écritures commencent par **`0x16`**. Le firmware n'en émet aucune.

### Non établi — à calibrer sur votre matériel

- La correspondance exacte registre → grandeur varie selon la série de moteur
  (BBS02, BBSHD, série G à afficheur UART) et la version de firmware du
  contrôleur.
- L'unité et le codage de la vitesse : selon les sources, le champ transporte
  soit la vitesse en dixièmes de km/h, soit la **période de rotation de roue en
  millisecondes**. Les deux interprétations sont implémentées et sélectionnables.
- La longueur exacte de certaines réponses.

## 2. Point de piquage

On sniffe **uniquement la ligne TX du contrôleur** (celle qui va vers
l'afficheur), qui porte les réponses.

```
Contrôleur ──── TX ────┬──────────────> Afficheur (inchangé)
                       │
                      1 kΩ
                       │
                       └──────────────> D10 du Nano (RX logiciel)

Contrôleur ──── GND ───────────────────> GND de la carte DN22D08
```

Règles impératives :

- **Ne jamais toucher au fil d'alimentation** du connecteur afficheur : il porte
  la tension batterie (48 V) sur certains modèles. Seuls les fils **données** et
  **masse** sont concernés.
- La résistance série de 1 kΩ limite le courant en cas d'erreur de câblage et
  ne charge pas la ligne.
- La broche D11, déclarée comme TX du port logiciel, **reste non câblée**.
  `SoftwareSerial` la force en sortie à l'état haut ; c'est sans effet tant
  qu'elle n'est reliée à rien. Cette contrainte est structurelle : elle rend
  l'émission physiquement impossible.

## 3. Segmentation des trames

L'écoute étant unidirectionnelle, on ne voit que des réponses, sans les requêtes
qui les délimiteraient. La segmentation repose donc sur le **silence
inter-trames** :

- à 1200 bauds, un octet occupe 10 bits ≈ **8,3 ms** ;
- les octets d'une même trame se suivent sans interruption ;
- l'afficheur interroge à une cadence de l'ordre de 100 à 300 ms.

Un silence de **30 ms** est donc un séparateur fiable : il est plus long qu'un
intervalle intra-trame (≈ 0 ms) et plus court que la période d'interrogation.
C'est la valeur `BAFANG_FRAME_GAP_MS`.

Chaque trame ainsi isolée est validée par sa somme de contrôle. Une trame dont
la somme est fausse est **silencieusement rejetée** et comptabilisée
(`framesRejected` dans le journal). Un taux de rejet élevé signale un mauvais
piquage, une masse absente ou une mauvaise vitesse de transmission.

## 4. Table de registres par défaut

Table pré-remplie dans `bafang.cpp`, **à valider en mode apprentissage**.

| 1er octet | Longueur | Grandeur supposée | Décodage |
|---|---|---|---|
| `0x20` | 3 | Niveau de charge | `soc_pct = f[1]` |
| `0x0A` | 3 | Courant moteur | `courant_A×10 = f[1] × 5` (unité 0,5 A) |
| `0x11` | 4 | Vitesse | `raw = (f[1] << 8) | f[2]`, puis §5 |
| `0x08` | 3 | Inconnu (statut ?) | ignoré |
| `0x31` | 3 | Cadence pédalier (?) | ignoré |

## 5. Les deux interprétations de la vitesse

Sélection par `BAFANG_SPEED_FORMULA` dans `config.h`.

**Formule 0 — valeur directe** : le champ est la vitesse en dixièmes de km/h.

```
vitesse_kmh_x10 = raw
```

**Formule 1 — période de roue** : le champ est la durée d'un tour de roue en ms.

```
vitesse_kmh_x10 = (36 × circonférence_mm) / raw_ms
```

Vérification de cohérence, avec une circonférence de 2200 mm (roue 700 C) :
à 25 km/h, un tour dure 2,2 m ÷ 6,94 m/s = 317 ms, et
36 × 2200 / 317 = 250, soit 25,0 km/h. La formule est bonne.

Le firmware rejette les valeurs aberrantes : `raw_ms` hors de [50, 5000] ms, ou
vitesse résultante > 99,9 km/h.

## 6. Mode apprentissage

Compiler avec `BAFANG_LEARN_MODE 1`. Chaque trame **valide** est publiée sur le
port série :

```
[BAFANG] 20 5A 7A            (3o)
[BAFANG] 0A 0C 16            (3o)
[BAFANG] 11 01 3B 4D         (4o)
```

Méthode de calibration, roue soulevée, véhicule sur béquille :

1. Moteur à l'arrêt, noter les trames stables. Celle qui porte le niveau de
   charge affiché par l'afficheur identifie le registre SOC.
2. Faire tourner la roue à vitesse croissante. La trame dont un champ varie de
   façon **monotone** est la vitesse. Noter plusieurs couples
   (valeur brute, vitesse lue sur l'afficheur d'origine) et vérifier laquelle
   des deux formules du §5 les relie.
3. Mettre de l'assistance en charge : le champ qui suit l'effort est le courant.
4. Reporter les constatations dans la table de `bafang.cpp`, repasser
   `BAFANG_LEARN_MODE` à 0.

## 7. Coût réel de l'écoute et pourquoi c'est acceptable

`SoftwareSerial` désactive les interruptions pendant toute la réception d'un
octet, soit **≈ 8,3 ms** à 1200 bauds. Pendant ce temps, les débordements du
Timer0 sont perdus et `millis()` prend du retard.

Estimation : quelques octets toutes les ~200 ms → de l'ordre de **2 à 5 % de
dérive** de l'horloge logicielle. Conséquences :

| Fonction | Impact | Acceptable ? |
|---|---|---|
| Cadence clignotants | 1,33 Hz → 1,27–1,33 Hz | Oui, la plage réglementaire est 1–2 Hz |
| Anti-rebond freins | 15 ms → 15,5 ms | Oui |
| Réaction au freinage | retard max +8,3 ms | Oui, imperceptible |
| Maintien coupure moteur | 300 ms → 315 ms | Oui |
| Odomètre / rappel clignotant | ±5 % | Oui, fonction de confort |

Aucune de ces dérives ne touche une fonction de sécurité de façon
significative. Si elle devient gênante, `BAFANG_ENABLE 0` la supprime
intégralement — et c'est précisément l'intérêt du principe P2.

## 8. Ce qui a été explicitement écarté

**Interposition (MITM)** — placer l'Arduino entre afficheur et contrôleur
permettrait de forcer le niveau d'assistance à 0 au freinage, de brider la
vitesse ou d'ajouter des modes. Écarté parce que :

- deux ports logiciels à 1200 bauds sur un ATmega328P, avec relayage temps réel,
  saturent les interruptions ;
- un firmware planté rendrait le moteur totalement inopérant, alors qu'en
  écoute passive il ne se passe rien du tout ;
- la coupure d'assistance recherchée est déjà obtenue, plus simplement et plus
  sûrement, par la ligne frein du contrôleur.

Si le besoin de forçage apparaît plus tard, il faudra un microcontrôleur avec
deux UART matériels (Mega 2560, ou une carte 32 bits).
