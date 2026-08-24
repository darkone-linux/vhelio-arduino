# vhelio-arduino

Électronique et électricité embarquées d'un **VHélio** (vélomobile solaire), en
alternative au montage décrit dans le
[guide de montage officiel](https://documentation.vhelio.org/vheliotech/guide-de-montage/main/080_installation_electricite.html).

Le câblage « en dur » est remplacé par un calculateur central :

- **Arduino Nano** (ATmega328P, 5 V, 16 MHz)
- **Carte d'E/S rail DIN DN22D08** (Eletechsup) — 8 relais 10 A, 8 entrées
  optocouplées, 4 boutons, afficheur 4 digits, le tout piloté par une chaîne de
  registres à décalage

Ce calculateur gère l'éclairage, la signalisation, le klaxon, la détection de
freinage et la télémétrie moteur. Il **ne gère pas** la traction : le contrôleur
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
| [`02-machines-a-etats.md`](specs/02-machines-a-etats.md) | Automates clignotants, freinage, feu arrière, klaxon |
| [`03-affectation-es.md`](specs/03-affectation-es.md) | Table d'E/S, variantes de câblage des freins, procédure de vérification |
| [`04-electricite.md`](specs/04-electricite.md) | Bilan de puissance, fusibles, sections, masses |
| [`05-protocole-bafang.md`](specs/05-protocole-bafang.md) | Écoute passive UART, décodage, mode apprentissage |
| [`06-architecture-logicielle.md`](specs/06-architecture-logicielle.md) | Modules, ordonnancement, budget mémoire |
| [`07-securite.md`](specs/07-securite.md) | Analyse de défaillances, état sûr, conformité |
| [`08-plan-de-tests.md`](specs/08-plan-de-tests.md) | 17 tests, de l'établi à la route |
| [`09-questions-ouvertes.md`](specs/09-questions-ouvertes.md) | Douze questions, dix tranchées ; ce qu'il reste à vérifier |
| [`10-alternatives-materiel.md`](specs/10-alternatives-materiel.md) | Faut-il changer de carte ? Analyse comparée et recommandation |

## Compilation

> **L'inverseur `485_ON` / `PRO` de la carte doit être sur `PRO`.** Sinon
> l'émetteur RS485 occupe D0/D1 et le téléversement échoue sans message clair.

```bash
./tools/setup.sh          # installe arduino-cli + coeur AVR dans ./.arduino
./tools/build-nix.sh      # compile (NixOS)
./tools/build.sh          # compile (distribution classique)
./tools/upload.sh /dev/ttyUSB0
```

Ajouter `old` en argument pour un Nano à ancien bootloader.

Croquis de vérification du brochage :

```bash
VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
```

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
**8 918 octets de flash (29 %)** et **705 octets de RAM (34 %)** sur les
30 720 / 2 048 disponibles. Compile sans avertissement dans les douze
combinaisons d'options couvertes par `tools/check-variants.sh`.

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

Vérifiez le brochage de votre carte avec `tools/pinscan` avant la première mise
sous tension. Le brochage documenté ici vient de la bibliothèque de référence
de la famille de cartes IO22/DN22, pas du datasheet de votre exemplaire précis.

> **Une hypothèse a déjà été prise en défaut sur ce projet.** La première
> version supposait des sorties à transistor pilotées par des broches dédiées ;
> ce sont en réalité des relais commandés par registre à décalage. Le coût de
> la correction est resté faible parce que le brochage était concentré dans
> deux fichiers et que l'incertitude était documentée comme telle. Le
> post-mortem est en [`specs/03-affectation-es.md`](specs/03-affectation-es.md) §7.
