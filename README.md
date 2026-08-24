# vhelio-arduino

Électronique et électricité embarquées d'un **VHélio** (vélomobile solaire), en
alternative au montage décrit dans le
[guide de montage officiel](https://documentation.vhelio.org/vheliotech/guide-de-montage/main/080_installation_electricite.html).

Le câblage « en dur » est remplacé par un calculateur central :

- **Arduino Nano** (ATmega328P, 5 V, 16 MHz)
- **Carte d'E/S rail DIN DN22D08** — 8 entrées optocouplées 12–24 V, 8 sorties

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
| [`01-exigences-fonctionnelles.md`](specs/01-exigences-fonctionnelles.md) | 30 exigences `F-xx` avec critère de vérification |
| [`02-machines-a-etats.md`](specs/02-machines-a-etats.md) | Automates clignotants, freinage, feu arrière, klaxon |
| [`03-affectation-es.md`](specs/03-affectation-es.md) | Table d'E/S, variantes de câblage des freins, procédure de vérification |
| [`04-electricite.md`](specs/04-electricite.md) | Bilan de puissance, fusibles, sections, masses |
| [`05-protocole-bafang.md`](specs/05-protocole-bafang.md) | Écoute passive UART, décodage, mode apprentissage |
| [`06-architecture-logicielle.md`](specs/06-architecture-logicielle.md) | Modules, ordonnancement, budget mémoire |
| [`07-securite.md`](specs/07-securite.md) | Analyse de défaillances, état sûr, conformité |
| [`08-plan-de-tests.md`](specs/08-plan-de-tests.md) | 16 tests, de l'établi à la route |
| [`09-questions-ouvertes.md`](specs/09-questions-ouvertes.md) | Décisions en attente |

## Compilation

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
**8 494 octets de flash (27 %)** et **641 octets de RAM (31 %)** sur les
30 720 / 2 048 disponibles.

## Régler le firmware

Deux fichiers, et deux seulement :

- `firmware/vhelio/src/config.h` — toutes les temporisations, tous les seuils,
  toutes les options de compilation (variante de câblage des freins, rôle de
  `OUT8`, activation du bus Bafang, journal, chien de garde…).
- `firmware/vhelio/src/pins.h` + les tableaux en tête de `src/board_io.cpp` —
  le brochage et la polarité des E/S.

## Avertissement

Ce projet pilote de l'éclairage et une fonction de coupure moteur sur un
véhicule circulant sur la voie publique.

La spécification impose que **aucune fonction de sécurité ne dépende
uniquement du firmware** : la coupure d'assistance au freinage est assurée par
le câblage direct des contacteurs sur la ligne frein du contrôleur Bafang, la
sortie de l'Arduino n'étant qu'un troisième chemin redondant. Le test T2.4 du
plan de tests vérifie explicitement que **le moteur se coupe toujours, Arduino
débranché**.

Vérifiez le brochage de votre carte au multimètre avant la première mise sous
tension : celui documenté ici est celui de la variante la plus courante, mais
il existe des révisions différentes.
