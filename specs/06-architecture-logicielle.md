# 06 — Architecture logicielle

## 1. Modèle d'exécution

Ordonnanceur coopératif à balayage complet : à chaque tour de boucle, **tous**
les modules sont réévalués à partir de l'instant courant `now = millis()`.
Aucune tâche n'est sautée, aucune n'est planifiée. Ce modèle est choisi parce
que le budget de calcul est ridiculement large (quelques centaines de
microsecondes par tour) et qu'il élimine toute une classe de bugs de
synchronisation.

Règles :

- **Aucun `delay()`**, aucune boucle d'attente. Toute temporisation est un
  `now - t0 >= duree`.
- `now` est **capturé une seule fois** en début de boucle et passé à tous les
  modules. Deux modules ne peuvent pas diverger sur l'instant courant.
- Les comparaisons de temps utilisent la soustraction non signée
  (`now - t0 >= d`), correcte au repliement de `millis()` à 49,7 jours.
- Aucune allocation dynamique, aucun `String`, aucun `float` dans les chemins
  fréquents (arithmétique entière en dixièmes d'unité).

## 2. Ordre d'appel et dépendances

```
loop()
  now = millis()
  ├─ bafang::poll(now)        vide le tampon série, décode      (aucune dépendance)
  ├─ inputs::update(now)      anti-rebond, fronts                (aucune dépendance)
  ├─ brakes::update(now, in)  → braking(), pilote OUT_MOTOR_CUT
  ├─ turnsignals::update(...) → OUT_TURN_*, rappel d'oubli
  ├─ lights::update(now, in, brakes::braking())  → OUT_PARK_FRONT/MAIN/TAIL_*
  ├─ horn::update(now, in)    (non compilé : HORN_ENABLE 0)
  ├─ telemetry::update(now)   consolide vitesse, intègre l'odomètre
  └─ diag::update(now)        voyant de défaut, acquittement, journal
  wdt_reset()
```

`lights` est appelé **après** `brakes` parce qu'il consomme `brakes::braking()`.
C'est la seule dépendance d'ordre du système ; elle est signalée en commentaire
dans le `.ino`.

## 3. Découpage en modules

| Module | Possède | Lit |
|---|---|---|
| `board_io` | l'accès matériel aux 16 E/S | `pins.h`, `config.h` |
| `debounce` | la classe `Debouncer` | — |
| `inputs` | les 8 états débouncés + fronts | `board_io` |
| `brakes` | `OUT_MOTOR_CUT` | `inputs` |
| `turnsignals` | `OUT_TURN_LEFT`, `OUT_TURN_RIGHT` | `inputs`, `telemetry` |
| `lights` | `OUT_PARK_FRONT`, `OUT_MAIN`, `OUT_TAIL_PARK`, `OUT_TAIL_STOP` | `inputs`, `brakes` |
| `horn` | — **désactivé** (`HORN_ENABLE 0`), la voie IN3/R7 appartient à `diag` | `inputs` |
| `display` | les 4 digits de l'afficheur | `telemetry`, `bafang`, `diag` |
| `bafang` | le port logiciel, le décodeur | — |
| `telemetry` | vitesse consolidée, odomètre | `bafang`, `wheelspeed` |
| `diag` | deux-points de l'afficheur, **voyant `OUT_FAULT`**, journal série, drapeaux de défaut | tous |

**Une sortie, un propriétaire.** Aucune sortie n'est écrite par deux modules.
Les deux relais des feux arrière appartiennent à `lights` : `brakes` lui
*demande* le stop, il ne le pilote pas. L'afficheur est coupé en deux
propriétaires disjoints : `display` possède les digits, `diag` possède le
deux-points (battement de cœur et indicateur de défaut).

## 4. Arborescence

```
firmware/vhelio/
├── vhelio.ino          VIDE de code — point d'entrée de croquis Arduino
└── src/
    ├── scheduler.cpp   setup() / loop(), ordre d'appel des modules
    ├── config.h        tous les réglages, toutes les options de compilation
    ├── pins.h          brochage DN22D08 ↔ Nano, index logiques
    ├── board_io.h/.cpp couche matérielle : registre à décalage, afficheur
    ├── debounce.h/.cpp classe Debouncer générique
    ├── inputs.h/.cpp   agrégation des 8 entrées
    ├── brakes.h/.cpp   automate freinage + coupure moteur
    ├── turnsignals.h/.cpp automate clignotants + détresse
    ├── lights.h/.cpp   phares + arbitrage PWM feu arrière
    ├── horn.h/.cpp     automate klaxon — désactivé, la voie IN3/R7 est au voyant
    ├── display.h/.cpp  pages de l'afficheur 4 digits
    ├── bafang.h/.cpp   écoute passive + décodage
    ├── wheelspeed.h/.cpp capteur de roue par scrutation (option)
    ├── telemetry.h/.cpp vitesse consolidée, odomètre
    └── diag.h/.cpp     LED, journal, défauts, autotest
```

Le sous-dossier `src/` est compilé récursivement par l'IDE Arduino (≥ 1.6.6) et
par `arduino-cli`. Il permet d'avoir de vrais modules sans passer par une
bibliothèque installée.

### Pourquoi `vhelio.ino` ne contient aucun code

L'IDE Arduino ne compile pas un `.ino` tel quel : il le réécrit, en y insérant
les prototypes des fonctions qu'un `ctags` **patché par Arduino** y détecte.
Cette étape est fragile dès qu'on sort de la chaîne d'outils officielle :

| ctags utilisé | Résultat |
|---|---|
| Celui d'Arduino | Correct — mais binaire lié dynamiquement, inutilisable sur NixOS |
| Exuberant Ctags (nixpkgs) | Prototypes sans type de retour → **erreur de compilation** |
| Universal Ctags | Numéros de ligne décalés de 1 → prototypes insérés **à l'intérieur** de `setup()`, qui s'appelle alors lui-même. **Ça compile, et le firmware part en récursion au démarrage.** |

Le dernier cas est le dangereux : une panne silencieuse, invisible à la
compilation. En déplaçant `setup()` et `loop()` dans `src/scheduler.cpp`, il
n'y a plus aucune fonction dans le `.ino`, donc plus rien à générer, et le code
compilé est exactement celui qui est écrit. La même précaution est appliquée à
`tools/pinscan`.

## 5. Abstraction matérielle (`board_io`)

Ce module a démontré son intérêt : la carte s'est révélée être d'une
architecture entièrement différente de celle supposée au départ, et **aucun
module métier n'a eu à changer**. Un module écrit toujours
`setOutput(OUT_HORN, true)` ; ce qu'il y a derrière est passé d'une broche à un
bit de registre à décalage sans qu'il le sache.

Il absorbe trois particularités :

**Les relais ne sont pas des broches.** `setOutput()` positionne un bit dans un
octet. Rien n'est appliqué tant que `refresh()` n'a pas émis la trame sur la
chaîne de registres — ce qui a lieu une fois par tour de boucle. Le tableau
`RELAY_BIT[]` encode l'ordre non séquentiel des bits (le relais 8 est sur le
bit 0), une bizarrerie du câblage qui ne remonte jamais plus haut.

**L'afficheur est multiplexé.** Chaque `refresh()` émet un digit ; quatre tours
de boucle forment une trame complète. Le rafraîchissement partage la même
chaîne de registres que les relais, donc les deux sont émis ensemble.

**Polarité configurable.** `IN_ACTIVE_LOW[]` traduit entre niveau électrique et
état logique. Découvrir que les optocoupleurs sont câblés à l'envers se règle
en éditant un tableau.

### Coût du rafraîchissement

`shiftOut()` de la bibliothèque Arduino coûte ~5 µs par bit, soit ~120 µs pour
les 24 bits d'une trame. Appelé à chaque tour de boucle, ce serait le poste de
calcul dominant du firmware. `board_io` utilise un accès direct aux ports, qui
ramène le coût à ~8 µs.

**Effet de bord à connaître** : `SoftwareSerial` bloque les interruptions ~8,3 ms
pendant la réception d'un octet Bafang. Pendant ce temps, la boucle ne tourne
pas, donc un digit reste allumé plus longtemps que les autres. Cela se traduit
par un léger scintillement de l'afficheur toutes les ~200 ms. C'est cosmétique
et sans effet sur les relais ; `BAFANG_ENABLE 0` le supprime.

## 6. Anti-rebond

Une seule classe, `Debouncer`, instanciée huit fois avec des durées différentes
(15 ms pour les freins, 20 ms pour l'acquittement, 30 ms pour le comodo).

Algorithme : la sortie stable ne change que si l'entrée brute est restée
identique pendant `stableMs`. Les drapeaux `rose()` / `fell()` ne sont valides
que pendant le cycle où la transition a eu lieu.

Ce choix — filtrage par stabilité plutôt que par temporisation de sortie —
introduit un retard égal à `stableMs`, ce qui est explicitement pris en compte
dans le budget de F-2.2 (15 ms d'anti-rebond + 10 ms de cycle < 50 ms exigés).

## 7. Budget mémoire

Mesures réelles (avr-gcc 15.3, `-Os -flto`, Nano / ATmega328P — 30 720 o de
flash utilisables après bootloader, 2 048 o de RAM) :

| Configuration | Flash | RAM |
|---|---|---|
| **Défaut** (Bafang + afficheur + voyant + journal + autotest + WDT) | **8 890 o — 28 %** | **715 o — 34 %** |
| Sans voyant de défaut | 8 734 o — 28 % | — |
| Sans bus Bafang, vitesse par capteur de roue | 7 046 o — 22 % | — |
| Sans afficheur | 8 384 o — 27 % | — |
| Production silencieuse (ni journal ni autotest ni WDT) | 6 394 o — 20 % | — |
| Mode apprentissage Bafang | 8 088 o — 26 % | — |
| Klaxon raccordé à la voie IN3 / R7 | 8 918 o — 29 % | — |
| Minimal (ni afficheur, ni bus, ni journal) | 3 638 o — 11 % | — |

Cible NF-1 (< 24 ko flash / < 1,4 ko RAM) tenue avec une marge de plus du
double. Les chaînes du journal sont placées en flash via `F()` : c'est ce qui
maintient la RAM à 715 o malgré une trentaine de messages.

Le balayage de toutes ces variantes est automatisé par
`tools/check-variants.sh` : une branche `#if` non prise n'est pas vérifiée par
le compilateur, et resterait cassée sans qu'on le sache.

## 8. Options de compilation

Toutes dans `config.h`, toutes vérifiées par des `#error` quand elles sont
incompatibles.

| Option | Défaut | Effet |
|---|---|---|
| `BRAKE_WIRING_VARIANT` | 2 | 1 = entrée frein unique ; 2 = avant/arrière séparés |
| `IN_INVERT_BRAKE_FRONT` / `_REAR` | 0 | Inverse la lecture (interface transistor, `cablage.md` §5) |
| `DISPLAY_ENABLE` | 1 | Compile ou non le pilotage des digits |
| `DISPLAY_DEFAULT_PAGE` | 2 | Page affichée au démarrage (2 = défauts : l'afficheur est un outil de maintenance, pas un tableau de bord) |
| `BAFANG_ENABLE` | 1 | Compile ou non l'écoute UART |
| `BAFANG_LEARN_MODE` | 0 | Dump hexadécimal des trames |
| `BAFANG_SPEED_FORMULA` | 1 | 0 = km/h ×10 direct ; 1 = période de roue |
| `SPEED_SOURCE_WHEEL` | 0 | Vitesse depuis le capteur de roue sur A6 |
| `BRAKE_FLASH_ENABLE` | 0 | Flash d'attaque du feu stop |
| `TAIL_ALWAYS_ON` | 0 | Veilleuse arrière permanente (feux de jour) |
| `MAIN_REQUIRES_PARK` | 0 | Le phare exige la veilleuse (0 = jamais bloqué) |
| `MAIN_KEEPS_PARK` | 1 | La veilleuse reste allumée avec le phare |
| `BLINK_REMINDER_ON_MS` | 200 | Phase allumée pendant le rappel d'oubli ; 0 = rappel muet |
| `FAULT_LAMP_ENABLE` | 1 | Voyant de défaut sur R7 + acquittement sur IN3 |
| `FAULT_LAMP_MASK` | `0x6B` | Quels défauts allument le voyant. Exclut le lien Bafang (P2), contrôlé par `static_assert` |
| `HORN_ENABLE` | 0 | Rend la voie IN3 / R7 à un klaxon ; exclusif du voyant |
| `DEBUG_SERIAL` | 1 | Journal série 115 200 bd |
| `WATCHDOG_ENABLE` | 1 | Chien de garde 1 s |
