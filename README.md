# vhelio-arduino

Électronique et électricité embarquées d'un **VHélio** (vélomobile solaire), en
alternative au montage décrit dans le
[guide de montage officiel](https://documentation.vhelio.org/vheliotech/guide-de-montage/main/080_installation_electricite.html).

Le câblage « en dur » est remplacé par un calculateur central :

- **Arduino Nano** (ATmega328P, 5 V, 16 MHz)
- **Carte d'E/S rail DIN DN22D08** (Eletechsup) — 8 relais 10 A, 8 entrées
  optocouplées, 4 boutons, afficheur 4 digits, le tout piloté par une chaîne de
  registres à décalage

Ce calculateur gère l'éclairage, la signalisation, la détection de freinage et
la télémétrie moteur. Le klaxon en est exclu : il est autonome, avec sa propre
batterie. Il **ne gère pas** la traction : le contrôleur
Bafang reste maître du moteur, l'Arduino n'est qu'un observateur et un
actionneur de sécurité redondant.

---

## Organisation du dépôt

| Chemin | Contenu |
|---|---|
| `specs/` | Spécification complète : fonctions, E/S, électricité, protocole, sécurité, tests |
| `firmware/vhelio/` | Firmware — `vhelio.ino` (vide) + modules dans `src/` |
| `tools/` | Compilation, téléversement, croquis de vérification de brochage |
| `hardware/` | Nomenclature et plan de câblage |

## Par où commencer

1. [`specs/00-vue-densemble.md`](specs/00-vue-densemble.md) — contexte, rôles, principes directeurs
2. [`specs/03-affectation-es.md`](specs/03-affectation-es.md) — **la table d'E/S, à vérifier avant tout câblage**
3. [`specs/04-electricite.md`](specs/04-electricite.md) — bus 48 V / 12 V, fusibles, sections de câbles
4. [`specs/09-questions-ouvertes.md`](specs/09-questions-ouvertes.md) — ce qu'il reste à trancher

| Fichier | Sujet |
|---|---|
| [`00-vue-densemble.md`](specs/00-vue-densemble.md) | Périmètre, rôles des équipements, architecture générale |
| [`01-exigences-fonctionnelles.md`](specs/01-exigences-fonctionnelles.md) | 39 exigences `F-xx` et 5 contraintes `NF-x` avec critère de vérification |
| [`02-machines-a-etats.md`](specs/02-machines-a-etats.md) | Automates clignotants, freinage, feu arrière, voyant de défaut |
| [`03-affectation-es.md`](specs/03-affectation-es.md) | Table d'E/S, variantes de câblage des freins, procédure de vérification |
| [`04-electricite.md`](specs/04-electricite.md) | Bilan de puissance, fusibles, sections, masses |
| [`05-protocole-bafang.md`](specs/05-protocole-bafang.md) | Écoute passive UART, décodage, mode apprentissage |
| [`06-architecture-logicielle.md`](specs/06-architecture-logicielle.md) | Modules, ordonnancement, budget mémoire |
| [`07-securite.md`](specs/07-securite.md) | Analyse de défaillances, état sûr, conformité |
| [`08-plan-de-tests.md`](specs/08-plan-de-tests.md) | 17 tests, de l'établi à la route |
| [`09-questions-ouvertes.md`](specs/09-questions-ouvertes.md) | Quatorze questions, treize tranchées ; ce qu'il reste à vérifier |
| [`10-alternatives-materiel.md`](specs/10-alternatives-materiel.md) | Faut-il changer de carte ? Analyse comparée et recommandation |

## Compilation

> **L'inverseur `485_ON` / `PRO` de la carte doit être sur `PRO`.** Sinon
> l'émetteur RS485 occupe D0/D1 et le téléversement échoue sans message clair.

```bash
./tools/setup.sh          # installe arduino-cli + coeur AVR dans ./.arduino
./tools/build-nix.sh      # compile (NixOS)
./tools/build.sh          # compile (distribution classique)
./tools/upload.sh         # televerse (port detecte seul)
./tools/monitor.sh        # console serie a 115200 bauds
```

**Le Nano de ce projet porte un ancien bootloader : le mot-clé `old`
(57 600 bauds) est obligatoire au téléversement**, sinon avrdude enchaîne les
`not in sync: resp=0x00`. Le port se force en premier argument :

```bash
./tools/upload.sh /dev/ttyACM0 old
```

Seul `upload.sh` en a besoin : `atmega328` et `atmega328old` produisent le même
binaire et ne diffèrent que par le débit. **Ce 57 600 est celui du bootloader,
pas celui des croquis** — la console, elle, reste à 115 200
(`Serial.begin`). `monitor.sh` remplace `arduino-cli monitor`, inutilisable sur
NixOS (outil `serial-monitor` prébuilt, même défaut de lien dynamique). Le nom du port ne renseigne pas sur la
carte — celle-ci sort en `/dev/ttyACM0` (pont USB-CDC générique) là où un Nano
officiel à FT232RL sort en `/dev/ttyUSB0`.

`upload.sh` **ne compile pas** : il téléverse le résultat de la compilation
précédente. Sous NixOS, l'utilisateur doit appartenir au groupe `dialout` — le
script le vérifie et donne la marche à suivre.

Croquis de vérification du brochage (`VHELIO_SKETCH` vaut pour les deux
commandes, chaque croquis ayant son propre répertoire sous `.build/`) :

```bash
VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pinscan ./tools/upload.sh /dev/ttyACM0 old
```

Firmware de **banc**, avec les huit entrées pilotables au clavier depuis la
console — de quoi dérouler la campagne 1 du plan de tests avant que le faisceau
soit serti (`specs/08-plan-de-tests.md`) :

```bash
VHELIO_SIM=1 ./tools/build-nix.sh
VHELIO_SIM=1 ./tools/upload.sh /dev/ttyACM0 old
```

`config.h` reste à `SIM_INPUTS 0` dans le dépôt : le drapeau vient de la ligne
de commande, et le binaire va dans `.build/vhelio-sim`, jamais mêlé à celui de
route. **Ce firmware ne doit pas rouler** — un caractère parasite sur la ligne
série y ferme un contact de frein.

Balayage de toutes les combinaisons d'options de `config.h` :

```bash
./tools/check-variants.sh
```

### Sur NixOS

`tools/build.sh` échoue : les binaires livrés par arduino-cli (`avr-gcc`,
`ctags`) sont liés dynamiquement contre un `/lib` inexistant.
`tools/build-nix.sh` les remplace par ceux de nixpkgs — il n'y a rien à
installer au niveau système.

### Empreinte mesurée

Configuration par défaut, avr-gcc 15.3, `-Os -flto` :
**8 886 octets de flash (28 %)** et **715 octets de RAM (34 %)** sur les
30 720 / 2 048 disponibles. Compile sans avertissement dans les quinze
combinaisons d'options couvertes par `tools/check-variants.sh`. Le firmware de
banc coûte 910 octets de flash et 2 de RAM de plus ; à `SIM_INPUTS 0`, il ne
coûte **rien du tout**, l'empreinte étant identique à l'octet près.

Temps de cycle mesuré à vide : **265 µs**, pointe à 6,7 ms sur la seconde où le
journal série est émis. Le seuil de défaut est à 10 ms.

## Autotest et contrôle au banc

`tools/seq-banc.sh` déroule la campagne 1 en 2 min 40, envoie les commandes et
annonce, horodaté, ce qui doit se produire. **Il ne vérifie rien de lui-même,
et ne le peut pas** : la chaîne de registres à décalage ne se relit pas, si
bien que le seul témoin de l'état d'un relais est son claquement. Le journal
série dit ce que le *firmware* a décidé ; le claquement dit ce que la *carte* a
fait. Le test, c'est les deux ensemble.

```bash
VHELIO_SIM=1 ./tools/build-nix.sh
VHELIO_SIM=1 ./tools/upload.sh /dev/ttyACM0 old
./tools/seq-banc.sh /dev/ttyACM0            # tout
./tools/seq-banc.sh /dev/ttyACM0 freins     # une phase seule
```

**Carte alimentée en 12 V.** Sans lui, aucun relais ne colle — les bobines sont
en 12 V — et aucune entrée ne réagit, la LED de chaque optocoupleur étant
alimentée depuis le +12 V de la carte. L'afficheur, lui, s'allume sur l'USB
seul : c'est le discriminant.

### Les huit points à contrôler à l'oreille

| # | Instant | Ce qui doit se produire | Ce que ça prouve |
|---|---|---|---|
| 1 | `t+0` | **7 claquements** de 200 ms d'affilée — R1 à R6 puis R7 — et **R8 muet** | L'autotest passe, et la coupure moteur en est bien exclue. **R8 qui claque ici est grave** |
| 2 | `t+4` puis `t+8` | Veilleuse : **2 claquements** (R1 avant, R5 arrière). Phares : **1 seul** (R2) | Le feu rouge arrière suit l'éclairage avant — exigence F-1.7 |
| 3 | `t+12` | **Rien ne bouge, pas un claquement** | `MAIN_KEEPS_PARK` : relâcher la veilleuse phares allumés n'éteint pas la veilleuse |
| 4 | `t+16` | **3 claquements** simultanés, tout retombe | Aucune sortie ne reste collée |
| 5 | `t+22` → `t+47` | **30 cycles en 22,5 s ± 1 s** au chronomètre | La cadence réglementaire, 80 cycles/min dans la plage 60–120. **Seul contrôle qui ne peut se faire qu'à l'oreille** |
| 6 | `t+47` | **Silence total** pendant 5 s, plus 1 claquement de R7 | Gauche et droite demandés ensemble éteignent les deux et allument le voyant |
| 7 | `t+73` | Le relâchement du frein en **deux temps** : R6 aussitôt, R8 environ 300 ms plus tard | `BRAKE_HOLD_MS` : l'assistance ne se réengage pas par à-coups sur un levier modulé |
| 8 | `t+154` | Le rythme devient **syncopé** — bref allumé, long éteint — **sans que la cadence change** | Le rappel d'oubli des clignotants, seul canal vers le conducteur une fois la coque fermée |

Entre le point 6 et le point 7, la détresse fait claquer R3 et R4 **en phase** :
le claquement est double, et c'est ce qui la distingue à l'oreille d'un
clignotant simple.

### Ce que le journal établit tout seul

Sans rien écouter, `.build/seq-banc.log` doit contenir :

| Attendu | Ligne |
|---|---|
| Veilleuse | `VL=1 AR=1` |
| Phares, veilleuse relâchée | `VL=1 PH=1 AR=1` — `VL` **reste** à 1 |
| Conflit de clignotants | `TRN=-` et `flt=0x05 LAMP` |
| Acquittement | `flt=0x05 ack` — le défaut est **toujours** signalé, seul le voyant s'éteint |
| Conflit refait après acquittement | `LAMP` **revient** : l'acquittement portait sur l'événement, pas sur la catégorie |
| Bus Bafang absent | `flt=0x04` en permanence et **jamais** `LAMP` — principe P2 |
| Sur toute la séquence | aucun `FLT_LOOP_SLOW`, aucun reset chien de garde |

## Régler le firmware

Deux fichiers, et deux seulement :

- `firmware/vhelio/src/config.h` — toutes les temporisations, tous les seuils,
  toutes les options de compilation (variante de câblage des freins, niveaux
  d'éclairage, rappel d'oubli des clignotants, bus Bafang, journal, chien de
  garde…).
- `firmware/vhelio/src/pins.h` + les tableaux en tête de `src/board_io.cpp` —
  le brochage et la polarité des E/S.

## Avertissement

Ce projet pilote de l'éclairage et une fonction de coupure moteur sur un
véhicule circulant sur la voie publique.

La spécification impose que **aucune fonction de sécurité ne dépende
uniquement du firmware**. C'est tenu aux deux freins :

- à l'**arrière**, le contacteur Bafang d'origine coupe l'assistance sans
  passer par l'Arduino, son connecteur restant intact ;
- à l'**avant**, le contacteur est unipolaire et ne pourrait donc a priori pas
  servir à la fois le feu stop et la ligne frein. Une **diode 1N4148** montée
  dans le boîtier lui permet de faire les deux, en bloquant le +12 V de la
  carte vers la ligne 5 V du contrôleur
  ([`hardware/cablage.md`](hardware/cablage.md) §4).

Le test T2.4 vérifie explicitement que **le moteur se coupe, Arduino
débranché, aux deux freins**. Tant qu'il ne passe pas, ne pas rouler sur route
ouverte.

Le brochage documenté ici est **mesuré** sur une DN22D08, borne par borne et
bit par bit — plus rien n'y est supposé. Il ne vaut pas pour autant pour une
autre carte de la famille : celui de l'IO22D08, dont il était déduit au départ,
s'est révélé faux sur les boutons, sur deux entrées, sur les quatre lignes de la
chaîne et sur tout l'afficheur. Si votre exemplaire diffère, `tools/pinfind` et
`tools/pinchain` le **découvrent** là où `tools/pinscan` ne sait que vérifier
une hypothèse.

> **Une hypothèse a déjà été prise en défaut sur ce projet.** La première
> version supposait des sorties à transistor pilotées par des broches dédiées ;
> ce sont en réalité des relais commandés par registre à décalage. Le coût de
> la correction est resté faible parce que le brochage était concentré dans
> deux fichiers et que l'incertitude était documentée comme telle. Le
> post-mortem est en [`specs/03-affectation-es.md`](specs/03-affectation-es.md) §7.
