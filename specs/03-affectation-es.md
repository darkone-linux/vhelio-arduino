# 03 — Affectation des entrées / sorties

> **À vérifier avant tout câblage.** Le brochage ci-dessous est celui de la
> variante la plus répandue de la DN22D08. Des révisions existent. La procédure
> de vérification au multimètre est en §5 — elle prend dix minutes et évite de
> griller une carte.

## 1. Ressources du Nano

L'ATmega328P offre 20 broches d'E/S. La DN22D08 en consomme 16 (8 in + 8 out).

| Broche | Affectation |
|---|---|
| D0 / D1 | UART matériel — console de mise au point 115 200 bd (partagé avec l'USB) |
| D2 … D9 | **OUT1 … OUT8** de la carte |
| D10 | RX logiciel — écoute UART Bafang (1200 bd) |
| D11 | TX logiciel — **non câblé**, réservé par `SoftwareSerial` |
| D12 | Capteur de vitesse roue (option `SPEED_SOURCE_WHEEL`) |
| D13 | LED intégrée — battement de cœur / défaut |
| A0 … A7 | **IN1 … IN8** de la carte |

Deux conséquences importantes :

- **A4 / A5 sont mobilisées** comme entrées logiques. L'I²C n'est donc pas
  disponible : pas d'extension d'E/S par expandeur sans libérer ces broches.
- **A6 et A7 n'existent qu'en analogique** sur l'ATmega328P en boîtier TQFP :
  `digitalRead()` ne fonctionne pas dessus. Le firmware les lit par
  `analogRead()` avec un seuil à 512 (~2,5 V). Les deux signaux les moins
  critiques en temps y sont affectés (feu de route, détresse).

## 2. Entrées — IN1 … IN8

Les entrées de la DN22D08 sont optocouplées. Un signal **+12 V** appliqué sur la
borne d'entrée (masse commune) allume la LED de l'optocoupleur, qui tire la
broche Arduino **à l'état bas**. La convention est donc *actif = niveau bas*,
gérée par `IN_ACTIVE_LOW[]` dans `board_io.cpp`.

| # | Broche | Nom logique | Source | Type | Anti-rebond |
|---|---|---|---|---|---|
| IN1 | A0 | `IN_TURN_LEFT` | Comodo, position gauche | maintenu | 30 ms |
| IN2 | A1 | `IN_TURN_RIGHT` | Comodo, position droite | maintenu | 30 ms |
| IN3 | A2 | `IN_HORN` | Comodo, bouton klaxon | momentané | 20 ms |
| IN4 | A3 | `IN_BRAKE_FRONT` | Contacteur frein avant | momentané | 15 ms |
| IN5 | A4 | `IN_BRAKE_REAR` | Contacteur frein arrière | momentané | 15 ms |
| IN6 | A5 | `IN_LOWBEAM` | Comodo, éclairage / croisement | maintenu | 30 ms |
| IN7 | A6 * | `IN_HIGHBEAM` | Comodo, feu de route | maintenu | 30 ms |
| IN8 | A7 * | `IN_HAZARD` | Interrupteur détresse dédié | maintenu | 30 ms |

\* lecture par `analogRead()`, seuil 512.

> A6 et A7 n'ont **pas de résistance de tirage interne**. Sur les six autres
> entrées, le firmware active `INPUT_PULLUP`, ce qui donne un état inactif
> franc même carte d'E/S non alimentée. Sur A6/A7, l'état au repos dépend
> entièrement du tirage présent sur la carte DN22D08. Le vérifier à l'étape 4
> de la procédure du §5 : si la valeur brute affichée flotte au lieu d'être
> stable, ajouter une résistance de 10 kΩ vers +5 V sur la broche concernée.

Le comodo ne fournissant pas de commande de détresse, IN8 attend un
**interrupteur à bascule dédié**, à monter sur le tableau de bord.

## 3. Sorties — OUT1 … OUT8

Broche Arduino à l'état haut = sortie active (`OUT_ACTIVE_HIGH[]`).

| # | Broche | Nom logique | Charge | Courant | Étage de puissance |
|---|---|---|---|---|---|
| OUT1 | D2 | `OUT_LOWBEAM` | Feu de croisement LED | ~1,7 A | Relais ou module MOSFET |
| OUT2 | D3 | `OUT_HIGHBEAM` | Feu de route LED | ~1,7 A | Relais ou module MOSFET |
| OUT3 | D4 | `OUT_TURN_LEFT` | 2 clignotants gauche | ~0,5 A | Direct si la sortie tient 1 A |
| OUT4 | D5 | `OUT_TURN_RIGHT` | 2 clignotants droite | ~0,5 A | Direct si la sortie tient 1 A |
| OUT5 | **D6** | `OUT_TAIL` | Feux rouges arrière | ~0,5 A | **Module MOSFET obligatoire (PWM)** |
| OUT6 | D7 | `OUT_HORN` | Klaxon 12 V | 5–8 A crête | **Relais 12 V / 20 A obligatoire** |
| OUT7 | D8 | `OUT_MOTOR_CUT` | Ligne frein contrôleur | < 10 mA | **Optocoupleur** (isolation 5 V/12 V) |
| OUT8 | D9 | `OUT_AUX` | Buzzer clignotants (défaut) ou relais accessoires | < 0,1 A | Direct |

Contraintes de brochage à ne pas casser en réaffectant :

- `OUT_TAIL` **doit** rester sur une broche PWM matérielle. Sur le Nano, parmi
  D2–D9, seules **D3, D5, D6, D9** le sont. D6 (Timer0) est retenue.
  Timer0 pilote aussi `millis()` : `analogWrite()` sur D5/D6 n'interfère pas
  avec `millis()`, seul un changement de prescaler le ferait — le firmware n'en
  change aucun.
- `OUT_MOTOR_CUT` passe par un optocoupleur et **jamais** en liaison directe :
  la ligne frein du contrôleur Bafang est un circuit ~5 V référencé à la masse
  du contrôleur, il ne faut y injecter ni 12 V ni le 5 V de l'Arduino.

## 4. Câblage des freins — deux variantes

### Variante A — freins en parallèle sur la ligne frein *(recommandée)*

Les deux contacteurs sont câblés **en parallèle directement sur le connecteur
frein du contrôleur Bafang**. La coupure moteur est alors 100 % matérielle
(principe P1) et fonctionne même Arduino débranché.

L'Arduino lit l'état de cette ligne via une **interface transistor haute
impédance**, pour ne pas la charger :

```
Ligne frein Bafang (~5 V au repos, 0 V au freinage)
      |
     10k
      |
    [B] Q1 = BC547            Q2 = P-MOSFET (IRF9540 / AO3401)
   Q1.E = GND                 Q2.S = +12 V
   Q1.C ---- 10k ---- Q2.G    Q2.G ---- 100k ---- +12 V
                              Q2.D ---> borne IN4 de la DN22D08
   (10k entre base et émetteur de Q1)
```

Comportement : au repos la ligne est à 5 V, Q1 conduit, Q2 conduit, +12 V arrive
sur IN4 → **entrée active = frein relâché**. Au freinage la ligne tombe à 0 V,
Q2 se bloque, IN4 retombe → entrée inactive.

La logique est donc **inversée**, ce que le firmware gère par
`IN_INVERT_BRAKE_FRONT 1`. Effet de bord bénéfique : une rupture de fil fait
retomber l'entrée, donc le firmware conclut « freinage » — état sûr.

En variante A, `IN_BRAKE_REAR` (IN5 / A4) est **libre** et disponible pour une
extension.

### Variante B — détection discriminante *(par défaut dans le firmware)*

Deux entrées distinctes, ce qui permet de savoir *quel* frein est actionné
(utile au diagnostic et pour un futur bridage différencié) :

- **Frein avant** (contacteur simple, non Bafang) : contacteur alimenté en
  +12 V, sortie vers IN4. Contact fermé = 12 V = actif. Câbler **en plus** le
  contacteur sur la ligne frein Bafang exige un contacteur **bipolaire**
  (deux circuits isolés) — sinon on injecterait du 12 V dans le contrôleur.
- **Frein arrière** (contacteur Bafang existant) : ajouter un **micro-rupteur
  dédié** sur le levier, alimenté en +12 V vers IN5. Le contacteur Bafang
  d'origine reste câblé sur le contrôleur et assure la coupure matérielle.

> Le choix se fait dans `config.h` : `BRAKE_WIRING_VARIANT 1` ou `2`.

## 5. Procédure de vérification du brochage

À faire **avant** de brancher quoi que ce soit d'autre que l'USB.

1. Carte DN22D08 **non alimentée en 12 V**, Nano alimenté par l'USB seul.
2. Téléverser le croquis fourni :
   `VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh` puis
   `./tools/upload.sh /dev/ttyUSB0`.
3. **Sorties** : le croquis active D2 … D9 l'une après l'autre, 1 s chacune, et
   affiche le nom de la broche sur le port série. Mesurer à l'ohmmètre ou au
   voltmètre sur chaque bornier de sortie et noter la correspondance réelle.
4. **Entrées** : alimenter la carte en 12 V, appliquer +12 V successivement sur
   chaque borne d'entrée. Le croquis affiche en continu l'état des broches
   A0 … A7. Noter quelle broche bascule, et **dans quel sens**.
5. Reporter les écarts dans `firmware/vhelio/src/pins.h` (tableaux `DN_OUT_PIN`
   et `DN_IN_PIN`) et dans `board_io.cpp` (`IN_ACTIVE_LOW`, `OUT_ACTIVE_HIGH`).
6. Vérifier au datasheet **le courant admissible par sortie** et par la carte
   entière. Si une sortie ne tient pas la charge annoncée en §3, insérer un
   relais ou un module MOSFET.

## 6. Bilan des ressources

| Ressource | Utilisé | Libre |
|---|---|---|
| Entrées carte | 8 / 8 (7 / 8 en variante A) | 0 (ou 1) |
| Sorties carte | 8 / 8 | 0 |
| Broches Nano hors carte | D0, D1, D10, D11, D13 | D12 (si vitesse UART) |
| Timers | Timer0 (millis + PWM D5/D6) | Timer1, Timer2 |
| Interruptions externes | aucune (D2/D3 en sortie) | — |
| UART matériel | console de debug | — |

Le système est **saturé côté E/S**. Toute fonction supplémentaire (feux de
recul, éclairage de cabine, capteur de température…) impose soit de libérer
A4/A5 pour un expandeur I²C (PCF8574), soit une seconde carte, soit un passage
sur un Arduino Mega.
