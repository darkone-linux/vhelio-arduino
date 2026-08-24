# 03 — Affectation des entrées / sorties

> **Révision majeure.** La première version de ce document supposait une carte
> à sorties transistor pilotées par des broches dédiées (`OUT1..8 → D2..D9`,
> `IN1..8 → A0..A7`). **C'était faux.** La DN22D08 est une carte à **relais
> pilotés par registre à décalage**. Toute la couche matérielle a été refaite.
> L'ancienne hypothèse et ce qu'elle impliquait sont conservées en §7, parce
> que la façon dont elle a été invalidée est réutilisable.

## 1. Architecture réelle de la carte

La DN22D08 (famille Eletechsup IO22/DN22) embarque :

- 8 sorties **relais**, contacts secs 10 A NO/NC, + une LED par voie
- 8 entrées **optocouplées** NPN, déclenchement à l'état bas
- 4 boutons-poussoirs sur la carte
- 1 afficheur **4 digits 7 segments** avec deux-points
- 1 interface RS485
- 1 support Arduino Nano V3.0

### Pourquoi il ne peut pas y avoir une broche par voie

| Fonction | Broches nécessaires |
|---|---|
| 8 entrées optocouplées | 8 |
| 4 boutons | 4 |
| 8 relais | 8 |
| Afficheur : 8 segments + point | 9 |
| Afficheur : sélection des 4 digits | 4 |
| **Total** | **33** |

Un Nano en expose 20. Le registre à décalage n'est donc pas un choix de
conception du fabricant, c'est une **nécessité arithmétique** — ce qui est
aussi la raison pour laquelle on peut être sûr de cette architecture sans
avoir la carte sous les yeux.

Trois 74HC595 sont chaînés : deux pour l'afficheur (U3, U4), un pour les
relais (U5). Les entrées et les boutons, eux, sont bien reliés directement au
Nano.

## 2. Brochage

| Broche Nano | Rôle |
|---|---|
| D2, D3, D4, D5, D6 | Entrées optocouplées IN1 à IN5 |
| D7, D8, D9, D10 | Boutons K1 à K4 (sur la carte) |
| D11, D12 | Entrées optocouplées IN8 et **IN7** (ordre inversé) |
| **D13** | **Données** de la chaîne de registres — *et LED intégrée du Nano* |
| A0 | Entrée optocouplée IN6 |
| A1 | **OE** du registre relais — validation, active à l'état bas |
| A2 | **Verrou** (latch) de la chaîne |
| A3 | **Horloge** de la chaîne |
| A4 | Libre → écoute UART Bafang (RX logiciel) |
| A5 | Libre → TX logiciel, **non câblé** |
| A6, A7 | Libres, analogiques seules |
| D0, D1 | Console série — **à vérifier**, le RS485 y est probablement raccordé |

> **La LED D13 n'est pas utilisable comme témoin.** Elle est sur la ligne de
> données du registre et papillote au rythme du rafraîchissement. Le battement
> de cœur est reporté sur le **deux-points de l'afficheur** : 1 Hz en
> fonctionnement nominal, ~4 Hz si un défaut est actif.

## 3. Entrées — IN1 … IN8

Optocoupleurs NPN : un signal appliqué sur la borne fait conduire
l'optocoupleur, qui tire la broche du Nano **à l'état bas**. Le firmware
active `INPUT_PULLUP`, ce qui donne un état inactif franc même carte non
alimentée.

| # | Broche | Nom logique | Source | Type | Anti-rebond |
|---|---|---|---|---|---|
| IN1 | D2 | `IN_TURN_LEFT` | Comodo, position gauche | maintenu | 30 ms |
| IN2 | D3 | `IN_TURN_RIGHT` | Comodo, position droite | maintenu | 30 ms |
| IN3 | D4 | `IN_HORN` | Comodo, bouton klaxon | momentané | 20 ms |
| IN4 | D5 | `IN_BRAKE_FRONT` | Contacteur frein avant | momentané | 15 ms |
| IN5 | D6 | `IN_BRAKE_REAR` | Contacteur frein arrière | momentané | 15 ms |
| IN6 | A0 | `IN_LOWBEAM` | Comodo, croisement | maintenu | 30 ms |
| IN7 | D12 | `IN_HIGHBEAM` | Comodo, feu de route | maintenu | 30 ms |
| IN8 | D11 | `IN_HAZARD` | Interrupteur détresse dédié | maintenu | 30 ms |

**Gain par rapport à l'ancienne hypothèse** : les huit entrées sont sur de
vraies broches numériques. Le contournement `analogRead()` sur A6/A7, et le
risque de broche flottante qu'il traînait, ont entièrement disparu.

## 4. Sorties — 8 relais

Contacts secs 10 A. **Aucun étage de puissance externe n'est nécessaire** :
ni module MOSFET, ni relais de klaxon, ni optocoupleur de coupure moteur.

| Relais | Bit registre | Nom logique | Charge | Régime |
|---|---|---|---|---|
| R1 | 1 | `OUT_LOWBEAM` | Feu de croisement | continu |
| R2 | 2 | `OUT_HIGHBEAM` | Feu de route | continu |
| R3 | 3 | `OUT_TURN_LEFT` | Clignotants gauche (AV + AR) | **cyclique 1,33 Hz** |
| R4 | 4 | `OUT_TURN_RIGHT` | Clignotants droite (AV + AR) | **cyclique 1,33 Hz** |
| R5 | 5 | `OUT_TAIL_PARK` | Feux de position arrière | continu |
| R6 | 6 | `OUT_TAIL_STOP` | Feux stop arrière | intermittent |
| R7 | 7 | `OUT_HORN` | Klaxon | intermittent |
| R8 | **0** | `OUT_MOTOR_CUT` | Ligne frein du contrôleur | intermittent |

> L'ordre des bits n'est pas séquentiel : le relais 8 occupe le **bit 0**, les
> relais 1 à 7 les bits 1 à 7. C'est le câblage de la carte. Le tableau
> `RELAY_BIT[]` de `board_io.cpp` encode cette bizarrerie une fois pour
> toutes ; aucun module métier ne la voit.

### Ce que les relais changent, en bien

- **La coupure moteur devient un contact sec.** Plus besoin d'optocoupleur :
  un relais est galvaniquement isolé par construction. C'est plus simple *et*
  plus sûr que la solution initiale.
- **Le klaxon n'a plus besoin de son relais externe.** Une voie de 10 A suffit
  pour 5 à 8 A. Prévoir tout de même le fusible F8 : un klaxon à compresseur
  peut avoir une pointe d'appel supérieure au calibre du contact.
- **Les phares se branchent en direct.** Plus de modules MOSFET.

### Ce qu'ils changent, en moins bien

- **Aucune modulation possible.** Un relais ne fait pas de PWM. Le feu arrière
  à circuit unique et intensité variable est **matériellement impossible** :
  il faut deux circuits, R5 et R6 (§4 du tableau).
- **Les clignotants usent de la mécanique.** À 1,33 Hz, chaque heure de
  clignotement représente 4 800 manœuvres. Pour une endurance électrique
  courante de l'ordre de 10⁵ manœuvres sous charge, cela donne ≈ 20 heures de
  clignotement **continu**. À raison de 5 % du temps de roulage, on est de
  l'ordre de plusieurs dizaines de milliers de kilomètres — acceptable, mais
  R3 et R4 sont les pièces d'usure du montage, et il faut le savoir.
- **Le claquement est audible en permanence.** Ce n'est pas un défaut : c'est
  exactement le bruit d'un relais de clignotant d'origine, et il **remplace le
  buzzer** qui était prévu. Une sortie et un composant économisés.
- **Temps de commutation ~5 à 10 ms**, à ajouter au budget de réaction du feu
  stop. On reste très en dessous des 50 ms exigés par F-2.2.

### Validation globale des sorties (OE)

La broche A1 pilote l'entrée OE du registre des relais. À l'état haut, **tous
les relais retombent en un cycle d'horloge**, sans toucher au contenu du
registre : l'état antérieur est restitué intact à la réactivation. C'est un
arrêt d'urgence matériel, exploitable pour un futur mode sécurité.

Au démarrage, le firmware écrit `HIGH` sur A1 **avant** de la passer en sortie.
Sur une broche encore en entrée, `digitalWrite(HIGH)` active le tirage interne,
donc la broche est déjà haute au moment où elle devient une sortie. L'ordre
inverse produirait une impulsion basse — **tous les relais collés** — pendant
quelques microsecondes à chaque mise sous tension.

## 5. Câblage des freins — deux variantes

La coupure moteur passant maintenant par un contact sec, la variante A perd
son principal intérêt (l'isolation était déjà résolue). Les deux restent
possibles.

### Variante B — détection discriminante *(défaut du firmware)*

- **Frein avant** : contacteur alimenté vers IN4. Pour qu'il ferme *aussi* la
  ligne frein du contrôleur, il faut un contacteur **bipolaire** — sinon on
  injecterait la tension d'entrée dans le contrôleur.
- **Frein arrière** : ajouter un micro-rupteur dédié sur le levier, vers IN5.
  Le contacteur Bafang d'origine reste câblé au contrôleur.

### Variante A — freins en parallèle sur la ligne frein

Les deux contacteurs sont câblés en parallèle directement sur le connecteur
frein du contrôleur ; l'Arduino lit cette ligne via l'interface transistor
décrite dans `hardware/cablage.md` §5. Une seule entrée est alors utilisée,
IN5 reste libre, et la logique est inversée (`IN_INVERT_BRAKE_FRONT 1`) — une
rupture de fil est alors interprétée comme un freinage, ce qui est l'état sûr.

Choix dans `config.h` : `BRAKE_WIRING_VARIANT 1` ou `2`.

## 6. Procédure de vérification

À faire **avant** de brancher autre chose que l'USB.

1. Téléverser le croquis de contrôle :
   ```bash
   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
   ./tools/upload.sh /dev/ttyUSB0
   ```
2. Ouvrir la console à 115 200 bauds.
3. **Chaîne de registres.** Un digit doit s'allumer sur l'afficheur et les
   relais doivent coller **un par un**, 1,5 s chacun, dans l'ordre annoncé.
   - Si rien ne bouge : les broches data / horloge / verrou sont fausses.
   - Si tous les relais collent en même temps : OE est mal identifiée.
   - Si l'ordre ne correspond pas : corriger `RELAY_BIT[]` dans `board_io.cpp`.
4. **Entrées.** Au repos, la console doit afficher `IN1..IN8 = 11111111`.
   Appliquer le signal sur chaque borne : le chiffre correspondant doit passer
   à `0`. Noter tout écart d'ordre et corriger `IN_PIN[]`.
5. **Boutons.** Appuyer sur K1 à K4 : `K1..K4` doit passer à `0`.
6. **RS485.** Vérifier si un circuit type MAX485 est relié à D0/D1. Si oui, la
   console série de mise au point entre en conflit avec lui — voir Q11 dans
   `09-questions-ouvertes.md`.
7. Reporter les écarts dans `firmware/vhelio/src/pins.h` et
   `firmware/vhelio/src/board_io.cpp`.

## 7. Comment l'hypothèse initiale a été invalidée

Elle venait d'un mapping très répandu sur les cartes rail DIN pour Nano
(`OUT → D2..D9`, `IN → A0..A7`), plausible mais pas vérifié pour ce modèle. La
spécification l'assumait explicitement et prévoyait sa vérification — c'est ce
qui a permis de la corriger sans rien casser d'autre.

Trois choses ont limité les dégâts :

- **Le brochage était concentré** dans `pins.h` et deux tableaux de
  `board_io.cpp`. Aucun module métier n'a été touché par le changement de
  brochage lui-même.
- **La couche `board_io` existait déjà** comme abstraction. Elle est passée de
  « masquer une polarité » à « masquer un registre à décalage » sans que ses
  appelants changent d'une ligne.
- **L'incertitude était documentée** comme question ouverte bloquante, avec
  une procédure de vérification. Elle a été traitée comme un risque connu, pas
  découverte au câblage.

Ce qui a réellement dû changer : le module d'éclairage (deux relais au lieu
d'une sortie modulée), la suppression du buzzer, l'ajout du pilotage de
l'afficheur, et le déplacement de l'écoute UART de D10 vers A4.

## 8. Bilan des ressources

| Ressource | Utilisé | Libre |
|---|---|---|
| Entrées optocouplées | 8 / 8 (7 / 8 en variante A) | 0 (ou 1) |
| Relais | 8 / 8 | 0 |
| Boutons carte | 1 / 4 (page d'afficheur) | 3 |
| Afficheur | vitesse, charge, défauts, odomètre | — |
| Broches Nano hors carte | A4, A5 (écoute Bafang) | A6, A7 |
| Timers | Timer0 (`millis`) | Timer1, Timer2 |
| UART matériel | console de mise au point | — |

Les relais et les entrées sont saturés. Les marges restantes sont **trois
boutons** et **deux broches analogiques**. Toute fonction supplémentaire
nécessitant une sortie de puissance impose une seconde carte.
