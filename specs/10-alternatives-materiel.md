# 10 — Faut-il changer de matériel ?

Réponse à la question posée en marge de Q2 : le couple Arduino Nano + DN22D08
impose des fonctionnalités dégradées (aucune modulation, R3/R4 en usure
mécanique, ressources saturées). Existe-t-il mieux, plus évolutif et aussi
répandu ?

> **Conclusion, pour ceux qui ne liront pas la suite : on ne change rien.**
> La carte est achetée, le firmware compile et remplit **toutes** les exigences
> obligatoires. Les limites recensées ne coûtent que du confort. Le seul
> scénario qui justifierait un changement est décrit au §5, et il n'est pas
> d'actualité.

---

## 1. Séparer trois questions qu'on confond

« Changer de carte » recouvre trois griefs très différents, et ils n'ont pas
la même gravité.

| Grief | Gravité réelle | Ce qu'il coûte |
|---|---|---|
| **Aucune modulation** | Faible | Un feu arrière à deux circuits au lieu d'un. C'est le câblage automobile standard depuis 1960, pas une régression |
| **R3/R4 s'usent** | Faible… avec une réserve | ≈ 20 h de clignotement continu pour 10⁵ manœuvres, soit plusieurs dizaines de milliers de km. **Mais les relais sont soudés** : les remplacer demande un fer, pas un tournevis |
| **Tout est saturé** | **Réelle, mais atténuée** | 7 entrées sur 8 et 7 relais sur 8 depuis que le klaxon s'est révélé autonome. Il reste **une** voie libre, et un seul UART. Au-delà, seconde carte obligatoire |

Le premier grief est cosmétique. Le deuxième est un problème de maintenance
dans dix ans. **Seul le troisième est structurel** — et il ne devient gênant
que le jour où l'on veut ajouter quelque chose.

S'y ajoute une limite découverte en cours de route, qui n'a rien à voir avec
les relais mais qui pèse plus lourd que les trois autres : **le boîtier est
sous la coque et l'afficheur est illisible en roulant** (Q12). Le calculateur
n'a plus de moyen de parler au conducteur, sinon en faisant claquer ses
relais.

---

## 2. Les candidats

Prix indicatifs, ordres de grandeur, hors port.

| | Actuel | ESP32 rail DIN | Mega + modules | Automate industriel |
|---|---|---|---|---|
| **Exemple** | Nano + DN22D08 | Kincony KC868-A8 ou A16 | Mega 2560 + carte 8 relais + carte 8 optos | Arduino Opta, Controllino |
| **Prix** | ~25 € *(payé)* | 50–70 € | 30–40 € | 150–300 € |
| **Rail DIN, borniers à vis** | oui | oui | non | oui |
| **Sorties** | 8 relais 10 A | 8 ou 16 relais | 8 relais, câblage à faire | 4 à 10 relais |
| **Entrées optocouplées** | 8 | 8 ou 16 | 8, câblage à faire | 8 à 16 |
| **UART libres** | **0** | 2 à 3 | **3** | 1 à 2 |
| **Modulation (PWM)** | non | oui, **mais pas sur les relais** | oui, idem | non |
| **Sans fil** | non | **WiFi + BLE** | non | selon modèle |
| **Mise à jour à distance** | non | **oui (OTA)** | non | selon modèle |
| **Disponibilité** | très bonne | bonne | excellente | moyenne |
| **Robustesse électrique** | correcte | correcte | **faible** (fils volants) | **excellente** |

### Ce que chaque option apporte vraiment

**ESP32 rail DIN (famille Kincony KC868).** Le seul candidat qui règle le
problème découvert en Q12 : avec le WiFi ou le Bluetooth, **le téléphone
devient le tableau de bord**, et l'afficheur enfermé sous la coque cesse
d'être un problème. L'OTA supprime en prime l'ouverture de la coque à chaque
correction de firmware — ce qui, sur un véhicule caréné, n'est pas un détail.
Trois UART matériels permettraient d'écouter le Bafang **et** le MPPT
VE.Direct simultanément, ce qui est aujourd'hui impossible.

À porter au passif, honnêtement :

- Les sorties restent des **relais**. Le PWM de l'ESP32 ne rétablit la
  modulation que si l'on ajoute un étage MOSFET — donc le grief n°1 n'est
  qu'à moitié réglé.
- Logique **3,3 V**, moins tolérante que le 5 V de l'AVR.
- Un ESP32 est plus fragile qu'un ATmega328P : brownout au démarrage,
  usure de la flash, pile réseau qui peut se figer. Sur un calculateur
  d'éclairage, cela s'atténue (chien de garde, état sûr matériel par la
  broche OE) mais ne s'annule pas.
- **Le WiFi est une surface d'attaque sur un système d'éclairage.** Si cette
  voie est prise, l'interface sans fil doit être en lecture seule, ou au
  minimum incapable de commander les feux et la coupure moteur.

**Mega 2560 + modules séparés.** Beaucoup d'E/S et trois UART pour presque
rien, mais on échange une carte propre à borniers à vis contre une poignée de
modules reliés par des fils Dupont. Sur un véhicule qui vibre, c'est une
régression de fiabilité qui annule tout le reste. **Écarté.**

**Automate industriel (Opta, Controllino).** Qualité de construction sans
comparaison, alimentation 12–24 V native, borniers sérieux, souvent certifiés.
C'est ce qu'on choisirait pour une petite série. Deux obstacles ici : le prix,
et surtout le fait qu'aucun modèle d'entrée de gamme n'offre **huit** sorties
relais — il faut un module d'extension, et la facture double. **Écarté pour un
exemplaire unique**, à reconsidérer en cas de série.

---

## 3. La fausse bonne idée : remplacer le Nano par un ESP32 au format Nano

C'est la première chose à laquelle on pense, puisque la DN22D08 a un support
au format Nano. **Cela ne marche pas :**

- Les 74HC595 de la carte sont alimentés en 5 V et exigent au moins **3,5 V**
  pour reconnaître un niveau haut. Un ESP32 sort **3,3 V**. On est sous la
  spécification : cela « marche » sur l'établi et lâche à la première variation
  de température.
- La carte injecte du 5 V dans la broche `5V` du support.
- `A6` et `A7` n'existent pas sur la plupart des cartes ESP32 au format Nano.

Si l'on va vers l'ESP32, c'est avec une carte conçue pour lui.

---

## 4. Ce que coûterait réellement une migration

C'est le point rassurant, et c'est un résultat direct de l'architecture
retenue (NF-3). Sur les **27 fichiers** du firmware, **5 seulement** touchent
au matériel AVR :

| Fichier | Dépendance | À faire |
|---|---|---|
| `pins.h` | brochage, ports | **réécrire** |
| `board_io.cpp` | registre à décalage, accès direct aux ports | **réécrire** |
| `scheduler.cpp` | `avr/wdt.h` | trois lignes |
| `diag.cpp` / `diag.h` | `MCUSR`, `WDRF` | deux lignes |

**Tout le reste est du C++ ordinaire et se porte tel quel** : `brakes`,
`lights`, `turnsignals`, `horn`, `inputs`, `debounce`, `telemetry`, `display`,
`bafang`, `wheelspeed`. La logique métier, les machines à états, les temps de
maintien, les anti-rebonds, le décodage Bafang — rien de tout cela ne serait à
refaire.

C'est exactement ce qui s'est déjà produit lors de la refonte de la v0.1 vers
la v0.2, quand la carte s'est révélée être à relais : la couche `board_io` a
absorbé le changement complet d'architecture sans qu'aucun module métier ne
bouge. **La migration n'est donc pas un argument contre le changement** — elle
représente deux soirées, pas une réécriture.

---

## 5. Recommandation

**Rester sur Nano + DN22D08.** Le matériel est acheté, le firmware satisfait
toutes les exigences obligatoires, et les limites recensées ne coûtent que du
confort. Changer maintenant, ce serait payer 50 à 70 € et deux soirées pour
résoudre des problèmes qu'on n'a pas encore rencontrés.

**Reprendre la question le jour où l'un de ces trois besoins apparaît :**

1. **« Je veux voir la vitesse et la charge en roulant. »** C'est le plus
   probable, et c'est directement la conséquence de Q12. Un ESP32 et un
   téléphone règlent la question mieux que n'importe quel afficheur ajouté.
2. **« Je veux lire la production solaire du MPPT. »** Impossible aujourd'hui :
   il n'y a plus d'UART libre. Trois UART matériels sur ESP32.
3. **« Je veux ajouter deux fonctions et il n'y a plus qu'une voie libre. »**
   Le klaxon autonome a rendu IN3 et R7 ; la suivante se heurte au mur, qui
   est net et sans contournement.

En attendant, deux composants à moins de 5 € valent mieux qu'un changement de
carte :

- **la diode D1 (1N4148)** sur le contacteur de frein avant
  (`hardware/cablage.md` §4) — elle rétablit une coupure d'assistance
  indépendante du firmware, et c'est la seule modification du projet qui touche
  à la sécurité ;
- **une LED rouge sur le relais R7**, libéré par le klaxon autonome : c'est
  aujourd'hui le seul moyen d'apprendre en roulant qu'un défaut est actif
  (`09-questions-ouvertes.md` Q14).
