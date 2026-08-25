# 06 — Architecture logicielle

## 1. Modèle d'exécution

Ordonnanceur coopératif à **balayage complet** : à chaque tour de boucle, tous
les modules sont réévalués à partir de l'instant courant `now = millis()`.
Aucune tâche n'est sautée, aucune n'est planifiée. Le budget de calcul est
ridiculement large — quelques centaines de microsecondes par tour — et ce
modèle élimine toute une classe de bugs de synchronisation.

Règles, toutes vérifiables par revue :

- **Aucun `delay()`**, aucune boucle d'attente hors de l'autotest de `setup()`.
  Toute temporisation est un `now - t0 >= duree`.
- `now` est **capturé une seule fois** en début de boucle et passé à tous les
  modules. Deux modules ne peuvent pas diverger sur l'instant courant.
- Les comparaisons de temps utilisent la **soustraction non signée**, correcte
  au repliement de `millis()` à 49,7 jours.
- Aucune allocation dynamique, aucun `String`, aucun `float` dans les chemins
  fréquents — arithmétique entière en dixièmes d'unité (NF-2).

## 2. Ordre d'appel et dépendances

```
loop()
  t0  = micros()
  now = millis()

  Acquisition
  ├─ simconsole::poll()       banc d'essai seul ; avant inputs, les touches
  │                           valent des bornes
  ├─ bafang::poll(now)        vide le tampon série, segmente, décode
  ├─ wheelspeed::update(now)  source de vitesse alternative (option)
  └─ inputs::update(now)      anti-rebond, fronts  ->  InputState

  Décision et commande
  ├─ brakes::update(now, in)         -> OUT_MOTOR_CUT
  ├─ turnsignals::update(now, in)    -> OUT_TURN_*, rappel d'oubli
  ├─ lights::update(now, in, brakes::braking())
  │                                  -> OUT_PARK_FRONT, OUT_MAIN, OUT_TAIL_*
  └─ horn::update(now, in)           (non compilé : HORN_ENABLE 0)

  Observation
  ├─ telemetry::update(now)   consolide la vitesse, intègre l'odomètre
  ├─ display::update(now)     page courante, événements
  └─ diag::update(now)        défauts, voyant, battement, journal

  board::refresh()            APPLIQUE les relais + un digit d'afficheur
  diag::noteLoop(micros() - t0)
  wdt_reset()
```

**`lights` est appelé après `brakes`** parce qu'il consomme
`brakes::braking()` : c'est la seule dépendance d'ordre du système, et elle est
signalée en commentaire dans `scheduler.cpp`.

**`board::refresh()` est le seul point qui touche le matériel de sortie.** Tant
qu'il n'a pas eu lieu, aucun `setOutput()` du tour n'a bougé un relais.

## 3. Découpage en modules

| Module | Possède | Lit |
|---|---|---|
| `board_io` | l'accès matériel : relais, entrées, boutons, afficheur | `pins.h`, `config.h` |
| `debounce` | la classe `Debouncer` | — |
| `inputs` | les 8 états débouncés + fronts | `board_io`, `simconsole` |
| `brakes` | `OUT_MOTOR_CUT` | `inputs` |
| `turnsignals` | `OUT_TURN_LEFT`, `OUT_TURN_RIGHT` | `inputs`, `telemetry` |
| `lights` | `OUT_PARK_FRONT`, `OUT_MAIN`, `OUT_TAIL_PARK`, `OUT_TAIL_STOP` | `inputs`, `brakes` |
| `horn` | — **désactivé** (`HORN_ENABLE 0`) ; la voie IN3/R7 appartient à `diag` | `inputs` |
| `display` | les 4 **digits** de l'afficheur | `telemetry`, `bafang`, `diag`, `lights`, `brakes`, `turnsignals`, `inputs` |
| `bafang` | le port logiciel, le décodeur | — |
| `telemetry` | vitesse consolidée, odomètre | `bafang`, `wheelspeed` |
| `diag` | `OUT_FAULT`, le **point décimal de gauche**, le journal, les drapeaux | tous |
| `simconsole` | l'injection des entrées au clavier — **banc seul**, ne compile rien si `SIM_INPUTS 0` | la console série |

**Une sortie, un propriétaire.** Aucune sortie n'est écrite par deux modules.
Les deux relais des feux arrière appartiennent à `lights` : `brakes` lui
*demande* le stop, il ne le pilote pas. L'afficheur est coupé en deux
propriétaires disjoints — `display` possède les digits, `diag` possède le point
décimal du digit de gauche (l'afficheur n'a pas de deux-points).

**`diag` va chercher les défauts** auprès des modules plutôt que d'être
notifié : aucun module métier ne connaît `diag`, ce qui garde le graphe de
dépendances acyclique.

## 4. Arborescence

```
firmware/vhelio/
├── vhelio.ino          VIDE de code — point d'entrée de croquis Arduino
└── src/
    ├── scheduler.cpp   setup() / loop(), ordre d'appel des modules
    ├── config.h        tous les réglages, toutes les options de compilation
    ├── pins.h          brochage DN22D08 <-> Nano, index logiques
    ├── board_io.h/.cpp couche matérielle : chaîne à décalage, afficheur
    ├── debounce.h/.cpp classe Debouncer générique
    ├── inputs.h/.cpp   agrégation des 8 entrées
    ├── brakes.h/.cpp   automate freinage + coupure moteur
    ├── turnsignals.h/.cpp automate clignotants + détresse + rappel d'oubli
    ├── lights.h/.cpp   éclairage avant + deux circuits arrière
    ├── horn.h/.cpp     automate klaxon — désactivé, la voie IN3/R7 est au voyant
    ├── display.h/.cpp  pages de l'afficheur 4 digits
    ├── bafang.h/.cpp   écoute passive + décodage
    ├── wheelspeed.h/.cpp capteur de roue par scrutation (option)
    ├── telemetry.h/.cpp vitesse consolidée, odomètre
    ├── diag.h/.cpp     autotest, voyant, journal, défauts
    └── simconsole.h/.cpp entrées simulées au clavier — banc d'essai seul
```

`src/` est compilé récursivement par `arduino-cli` : de vrais modules sans
passer par une bibliothèque installée.

### Pourquoi `vhelio.ino` ne contient aucun code

L'IDE Arduino ne compile pas un `.ino` tel quel : il le réécrit, en y insérant
les prototypes des fonctions qu'un `ctags` **patché par Arduino** y détecte.
Cette étape est fragile dès qu'on sort de la chaîne d'outils officielle :

| ctags utilisé | Résultat |
|---|---|
| Celui d'Arduino | Correct — mais lié dynamiquement, inutilisable sur NixOS |
| Exuberant Ctags (nixpkgs) | Prototypes sans type de retour → **erreur de compilation** |
| Universal Ctags | Lignes décalées de 1 → prototypes insérés **à l'intérieur** de `setup()`, qui s'appelle alors lui-même. **Ça compile, et le firmware part en récursion au démarrage** |

Le dernier cas est le dangereux : une panne silencieuse, invisible à la
compilation. En déplaçant `setup()` et `loop()` dans `src/scheduler.cpp`, il
n'y a plus aucune fonction dans le `.ino`, donc plus rien à générer, et le code
compilé est exactement celui qui est écrit. **Même précaution dans tous les
croquis de `tools/`.**

## 5. Abstraction matérielle (`board_io`)

Un module métier écrit `setOutput(OUT_TAIL_STOP, true)` et ne sait rien du
reste. C'est ce qui a permis à la carte de se révéler d'une architecture
entièrement différente de celle supposée **sans qu'aucun module métier ne
change** (voir `archives/decouverte-brochage.md`).

Il absorbe trois particularités :

**Les relais ne sont pas des broches.** `setOutput()` positionne un bit dans un
octet ; rien n'est appliqué tant que `refresh()` n'a pas émis la trame. Le
tableau `RELAY_BIT[]` traduit index logique → bit de registre ; il vaut
l'identité sur cet exemplaire, et n'est conservé que pour garder le reste du
firmware indifférent à la question.

**L'afficheur est multiplexé.** Chaque `refresh()` émet un digit ; quatre tours
forment une trame. Il partage la chaîne de registres avec les relais, donc les
deux sont émis ensemble.

**Polarité configurable.** `IN_ACTIVE_LOW[]` traduit entre niveau électrique et
état logique : découvrir des optocoupleurs câblés à l'envers se règle en
éditant un tableau.

### Coût du rafraîchissement

`shiftOut()` coûte ~5 µs par bit, soit ~120 µs pour les 24 bits d'une trame —
appelé à chaque tour, ce serait le poste de calcul dominant. `board_io` utilise
un **accès direct aux ports** (les trois lignes sont sur PORTC), qui ramène le
coût à ~8 µs.

> **Effet de bord à connaître** : `SoftwareSerial` bloque les interruptions
> ~8,3 ms pendant la réception d'un octet Bafang. La boucle ne tourne pas, donc
> un digit reste allumé plus longtemps que les autres — léger scintillement de
> l'afficheur toutes les ~200 ms. Cosmétique, sans effet sur les relais, et
> supprimé par `BAFANG_ENABLE 0`.

## 6. Anti-rebond

Une seule classe, `Debouncer` : filtrage **par stabilité**, la sortie ne change
que si l'entrée brute est restée identique pendant `stableMs`. Les drapeaux
`rose()` / `fell()` ne sont valides que pendant le cycle de la transition.

Instances : **huit dans `inputs`** (15 ms pour les freins, 20 ms pour
l'acquittement, 30 ms pour le comodo), plus une dans `display` et une dans
`diag` pour des boutons qui ne passent pas par `inputs`.

Ce choix — filtrage par stabilité plutôt que temporisation de sortie —
introduit un retard égal à `stableMs`, explicitement pris en compte dans le
budget de F-2.2 : 15 ms d'anti-rebond + 10 ms de cycle + ~10 ms de collage du
relais, pour 50 ms exigés.

## 7. Budget mémoire

Mesuré avec `tools/check-variants.sh` (avr-gcc 15.3, `-Os -flto`, ATmega328P —
30 720 o de flash utilisables après bootloader, 2 048 o de RAM) :

| Configuration | Flash | |
|---|---|---|
| **Défaut** (Bafang + afficheur + voyant + journal + autotest + WDT) | **9 692 o** | **31 %** |
| Freins en parallèle + flash d'attaque du stop | 9 916 o | 32 % |
| Bus Bafang **et** capteur de roue | 9 834 o | 32 % |
| Freins inversés (interface transistor) | 9 734 o | 31 % |
| Klaxon raccordé à la voie IN3 / R7 | 9 710 o | 31 % |
| Phare conditionné à la veilleuse | 9 690 o | 31 % |
| Rappel clignotant muet + page vitesse | 9 664 o | 31 % |
| Vitesse Bafang en valeur directe | 9 642 o | 31 % |
| Sans voyant de défaut | 9 528 o | 31 % |
| Mode apprentissage Bafang | 8 906 o | 28 % |
| Sans afficheur | 8 584 o | 27 % |
| Sans bus Bafang, vitesse par capteur de roue | 7 818 o | 25 % |
| Production silencieuse (ni journal, ni autotest, ni WDT) | 7 124 o | 23 % |
| Minimal (ni afficheur, ni bus, ni journal) | 3 844 o | 12 % |
| *Banc d'essai — entrées simulées à la console* | *10 764 o* | *35 %* |

**RAM en configuration par défaut : 783 o, soit 38 %.** À `SIM_INPUTS 0`, le
firmware de banc coûte **zéro** — l'empreinte est identique à l'octet près.

Cible NF-1 (< 24 ko flash / < 1,4 ko RAM) tenue avec une marge de plus du
double. Les chaînes du journal sont en flash via `F()` : c'est ce qui maintient
la RAM à 783 o malgré une trentaine de messages.

Le balayage de ces quinze variantes est automatisé par
`tools/check-variants.sh`. Une branche `#if` non prise n'est pas vérifiée par
le compilateur et resterait cassée sans qu'on le sache.

**Temps de cycle mesuré à vide : 265 µs**, pointe à 6,7 ms sur la seconde où le
journal série est émis — c'est de loin le plus long traitement du cycle. Seuil
de défaut à 10 ms (`LOOP_SLOW_US`).

## 8. Options de compilation

Toutes dans `config.h`, toutes gardées par des `#error` ou des `#warning` quand
elles sont incompatibles ou dangereuses.

| Option | Défaut | Effet |
|---|---|---|
| `BRAKE_WIRING_VARIANT` | 2 | 1 = entrée frein unique ; 2 = avant/arrière séparés |
| `IN_INVERT_BRAKE_FRONT` / `_REAR` | 0 | Inverse la lecture (interface transistor, `cablage.md` §5) |
| `MAIN_REQUIRES_PARK` | 0 | Le phare exige la veilleuse (0 = jamais bloqué) |
| `MAIN_KEEPS_PARK` | 1 | La veilleuse reste allumée avec le phare |
| `TAIL_ALWAYS_ON` | 0 | Feu de position arrière permanent (feux de jour) |
| `BRAKE_FLASH_ENABLE` | 0 | Flash d'attaque du feu stop — non conforme, et usant pour le relais |
| `BLINK_REMINDER_ON_MS` | 200 | Phase allumée pendant le rappel d'oubli ; 0 = rappel muet |
| `FAULT_LAMP_ENABLE` | 1 | Voyant de défaut sur R7 + acquittement sur IN3 |
| `FAULT_LAMP_MASK` | `0x6B` | Quels défauts allument le voyant. Exclut le lien Bafang (P2), contrôlé par `static_assert` |
| `HORN_ENABLE` | 0 | Rend la voie IN3 / R7 à un klaxon ; exclusif du voyant |
| `DISPLAY_ENABLE` | 1 | Compile ou non le pilotage des digits |
| `DISPLAY_DEFAULT_PAGE` | 0 | 0 = événements, en clair (l'afficheur est un outil de maintenance) |
| `BAFANG_ENABLE` | 1 | Compile ou non l'écoute UART |
| `BAFANG_LEARN_MODE` | 0 | Dump hexadécimal des trames |
| `BAFANG_SPEED_FORMULA` | 1 | 0 = km/h ×10 direct ; 1 = période de roue |
| `SPEED_SOURCE_WHEEL` | 0 | Vitesse depuis le capteur de roue sur A6 |
| `WHEEL_CIRCUMFERENCE_MM` | 2200 | **À corriger quand la roue sera montée** (Q10) |
| `DEBUG_SERIAL` | 1 | Journal série 115 200 bd |
| `SELFTEST_ENABLE` | 1 | Balayage des relais + tous segments allumés au démarrage |
| `WATCHDOG_ENABLE` | 1 | Chien de garde 1 s |
| `SIM_INPUTS` | 0 | Entrées pilotables au clavier. **Jamais dans le dépôt** : le drapeau vient de `VHELIO_SIM=1` sur la ligne de commande |
