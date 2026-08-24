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
  ├─ turnsignals::update(...) → OUT_TURN_*, demande buzzer
  ├─ lights::update(now, in, brakes::braking())  → OUT_LOWBEAM/HIGHBEAM/TAIL
  ├─ horn::update(now, in)    → OUT_HORN
  ├─ telemetry::update(now)   consolide vitesse, intègre l'odomètre
  └─ diag::update(now)        LED, journal, temps de cycle
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
| `turnsignals` | `OUT_TURN_LEFT`, `OUT_TURN_RIGHT`, demande buzzer | `inputs`, `telemetry` |
| `lights` | `OUT_LOWBEAM`, `OUT_HIGHBEAM`, `OUT_TAIL` | `inputs`, `brakes` |
| `horn` | `OUT_HORN` | `inputs` |
| `aux_out` | `OUT_AUX` (arbitre buzzer / accessoires) | `turnsignals` |
| `bafang` | le port logiciel, le décodeur | — |
| `telemetry` | vitesse consolidée, odomètre | `bafang`, `wheelspeed` |
| `diag` | LED D13, journal série, drapeaux de défaut | tous |

**Une sortie, un propriétaire.** Aucune sortie n'est écrite par deux modules.
`OUT_TAIL` est partagée entre éclairage et freinage, mais un seul module
(`lights`) l'écrit : `brakes` lui *demande* le stop, il ne le pilote pas.
Même principe pour `OUT_AUX`, arbitrée par `aux_out`.

## 4. Arborescence

```
firmware/vhelio/
├── vhelio.ino          VIDE de code — point d'entrée de croquis Arduino
└── src/
    ├── scheduler.cpp   setup() / loop(), ordre d'appel des modules
    ├── config.h        tous les réglages, toutes les options de compilation
    ├── pins.h          brochage DN22D08 ↔ Nano, index logiques
    ├── board_io.h/.cpp couche matérielle : polarité, A6/A7, PWM
    ├── debounce.h/.cpp classe Debouncer générique
    ├── inputs.h/.cpp   agrégation des 8 entrées
    ├── brakes.h/.cpp   automate freinage + coupure moteur
    ├── turnsignals.h/.cpp automate clignotants + détresse
    ├── lights.h/.cpp   phares + arbitrage PWM feu arrière
    ├── horn.h/.cpp     automate klaxon
    ├── aux_out.h/.cpp  buzzer ou relais accessoires
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

Elle absorbe trois particularités du montage, pour qu'aucun module métier n'ait
à s'en soucier :

**Polarité configurable.** `IN_ACTIVE_LOW[]` et `OUT_ACTIVE_HIGH[]` traduisent
entre « niveau électrique » et « état logique ». Changer de carte, ou découvrir
que les optocoupleurs sont câblés à l'envers, se règle en éditant deux tableaux.

**A6 / A7 sans lecture numérique.** `rawInput()` teste si la broche est A6 ou A7
et bascule sur `analogRead()` avec un seuil à 512. Les modules appelants ne
voient aucune différence.

**PWM avec repli.** `setOutputPwm()` vérifie que la broche est capable de PWM
matériel. Si elle ne l'est pas (réaffectation malheureuse), elle retombe sur un
tout-ou-rien à seuil 128 plutôt que de produire un comportement silencieusement
faux. Si la sortie est active à l'état bas, le rapport cyclique est inversé.

## 6. Anti-rebond

Une seule classe, `Debouncer`, instanciée huit fois avec des durées différentes
(15 ms pour les freins, 20 ms pour le klaxon, 30 ms pour le comodo).

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
| **Défaut** (Bafang + journal + autotest + WDT) | **8 494 o — 27 %** | **641 o — 31 %** |
| Sans bus Bafang, vitesse par capteur de roue | 6 628 o — 21 % | — |
| Freins en parallèle + flash stop + relais accessoires | 8 628 o — 28 % | — |
| Production silencieuse (ni journal ni autotest ni WDT) | 5 890 o — 19 % | — |
| Mode apprentissage Bafang | 7 710 o — 25 % | — |
| Bus Bafang **et** capteur de roue | 8 618 o — 28 % | — |

Cible NF-1 (< 24 ko flash / < 1,2 ko RAM) tenue avec une marge de plus du
double. Les chaînes du journal sont placées en flash via `F()` : c'est ce qui
maintient la RAM à 641 o malgré une trentaine de messages.

Le balayage de toutes ces variantes est automatisé par
`tools/check-variants.sh` : une branche `#if` non prise n'est pas vérifiée par
le compilateur, et resterait cassée sans qu'on le sache.

## 8. Options de compilation

Toutes dans `config.h`, toutes vérifiées par des `#error` quand elles sont
incompatibles.

| Option | Défaut | Effet |
|---|---|---|
| `BRAKE_WIRING_VARIANT` | 2 | 1 = entrée frein unique ; 2 = avant/arrière séparés |
| `IN_INVERT_BRAKE_FRONT` / `_REAR` | 0 | Inverse la lecture (interface transistor, §03-4) |
| `OUT8_ROLE_BUZZER` | 1 | 1 = buzzer clignotants ; 0 = relais accessoires |
| `BAFANG_ENABLE` | 1 | Compile ou non l'écoute UART |
| `BAFANG_LEARN_MODE` | 0 | Dump hexadécimal des trames |
| `BAFANG_SPEED_FORMULA` | 1 | 0 = km/h ×10 direct ; 1 = période de roue |
| `SPEED_SOURCE_WHEEL` | 0 | Vitesse depuis le capteur de roue sur D12 |
| `BRAKE_FLASH_ENABLE` | 0 | Flash d'attaque du feu stop |
| `TAIL_ALWAYS_ON` | 0 | Veilleuse arrière permanente (feux de jour) |
| `HIGHBEAM_REQUIRES_LOWBEAM` | 1 | Le feu de route exige le croisement |
| `HIGHBEAM_KEEPS_LOWBEAM` | 1 | Le croisement reste allumé avec la route |
| `DEBUG_SERIAL` | 1 | Journal série 115 200 bd |
| `WATCHDOG_ENABLE` | 1 | Chien de garde 1 s |
