# 03 — Affectation des entrées / sorties

**Tout ce document est mesuré**, borne par borne et bit par bit, sur
l'exemplaire de DN22D08 du projet. Plus rien n'y est supposé. La méthode de
mesure et le post-mortem de l'hypothèse initiale — fausse — sont archivés dans
[`archives/decouverte-brochage.md`](archives/decouverte-brochage.md).

Le brochage vit dans **deux fichiers, et deux seulement** :
`firmware/vhelio/src/pins.h` et les tableaux en tête de `src/board_io.cpp`.
C'est ce qui a permis d'absorber un changement complet d'architecture de carte
sans toucher un seul module métier (NF-3).

---

## 1. Architecture de la carte

La DN22D08 (famille Eletechsup IO22/DN22) embarque 8 relais 10 A NO/NC,
8 entrées optocouplées NPN, 4 boutons, un afficheur 4 digits 7 segments, une
interface RS485 et un support Arduino Nano.

Les entrées et les boutons sont câblés **directement** au Nano. Les relais et
l'afficheur, non : ils passent par **trois 74HC595 chaînés** (U3 et U4 pour
l'afficheur, U5 pour les relais).

Ce n'est pas un choix du fabricant, c'est une nécessité arithmétique — 8
entrées + 4 boutons + 8 relais + 8 segments (point compris) + 4 sélections de
digit = **32 broches**, quand un Nano en expose 20.

## 2. Brochage

| Broche Nano | Rôle |
|---|---|
| D2 … D7 | Entrées optocouplées IN1 à IN6 |
| D9, D11 | Entrées optocouplées IN7 et IN8 |
| D12, D10, D8 | Boutons K1, K2, K3 *(les numéros de broche décroissent)* |
| A0 | Bouton K4 |
| A5 | **Données** de la chaîne à décalage (PC5) |
| A4 | **Horloge** de la chaîne (PC4) |
| A3 | **Verrou** de la chaîne (PC3) |
| A2 | **OE** du registre relais — validation, **active à l'état bas** |
| A1 | Écoute UART Bafang — RX logiciel (PCINT9) |
| D13 | TX logiciel, **non câblé** — et LED intégrée du Nano |
| A6, A7 | Libres, mais **analogiques seules** : ni tirage interne, ni PCINT |
| D0, D1 | Console série — voir §2 bis |

Les trois lignes de la chaîne sont sur **PORTC**, d'où l'accès direct aux ports
de `board_io.cpp` (~8 µs par trame, contre ~120 µs avec `shiftOut()`).

> **Pourquoi l'écoute Bafang est sur A1.** A4 lui était promise ; la chaîne l'a
> prise avec A5, et l'OE a pris A2. Il ne restait que D13 et A1, les deux
> seules broches libres à interruption sur changement d'état — A6 et A7
> n'auraient pas pu. A1 l'emporte parce que la LED intégrée du Nano charge
> D13 : sans importance pour une sortie, gênant pour une entrée qu'on écoute.
>
> **D13 hérite donc du TX**, que `SoftwareSerial` exige mais que rien ne câble
> — c'est ce qui rend l'émission physiquement impossible (F-5.1). Effet
> visible : le TX est maintenu à l'état haut au repos, donc **la LED du Nano
> reste allumée en fixe**. Elle ne peut pas servir de témoin ; le battement de
> cœur est sur le point décimal du digit de gauche (§4 bis).

## 2 bis. Le RS485 et l'inverseur « 485_ON / PRO »

La carte porte un émetteur-récepteur RS485 **relié à D0/D1**, donc en conflit
avec la console série et avec le téléversement. Un inverseur à glissière l'en
déconnecte.

| Position | Effet |
|---|---|
| **`PRO`** | RS485 isolé de D0/D1. Console et téléversement USB normaux. **Position d'exploitation de ce projet, en permanence.** |
| `485_ON` | Le RS485 prend D0/D1 : plus de console, plus de téléversement |

Aucun convertisseur USB-RS485 n'est nécessaire : on programme le Nano par son
propre port USB. Le RS485 ne servirait qu'à un futur bus entre cartes.

> Si un téléversement échoue sans raison apparente, cet inverseur est le piège
> classique de cette carte — mais **pas le premier suspect** : tester d'abord
> le débit du bootloader (§6, étape 1), qui coûte une commande et n'oblige pas
> à ouvrir le boîtier.

## 3. Entrées — IN1 … IN8

Optocoupleurs **NPN, déclenchement à l'état bas** : la LED de l'optocoupleur
est alimentée depuis le +12 V de la carte, et l'on active une entrée en
**fermant sa borne sur la masse**. Le firmware arme `INPUT_PULLUP`, ce qui
donne un état inactif franc même carte non alimentée.

**Tous les organes de commande sont donc de simples contacts secs vers la
masse** — comodo, interrupteurs, contacteurs de frein — et le commun du comodo
va à **GND**, pas au +12 V. C'est ce qui rend utilisable le contacteur de frein
avant, qui est unipolaire (§5).

| # | Broche | Nom logique | Source | Type | Anti-rebond |
|---|---|---|---|---|---|
| IN1 | D2 | `IN_TURN_LEFT` | Comodo, position gauche | maintenu | 30 ms |
| IN2 | D3 | `IN_TURN_RIGHT` | Comodo, position droite | maintenu | 30 ms |
| IN3 | D4 | `IN_ACK` | Bouton d'acquittement des défauts | momentané | 20 ms |
| IN4 | D5 | `IN_BRAKE_FRONT` | Contacteur frein avant (unipolaire) | momentané | 15 ms |
| IN5 | D6 | `IN_BRAKE_REAR` | Micro-rupteur S2, levier arrière | momentané | 15 ms |
| IN6 | D7 | `IN_PARK` | Interrupteur dédié « veilleuse » (S3) | maintenu | 30 ms |
| IN7 | D9 | `IN_MAIN` | Comodo, éclairage fort | maintenu | 30 ms |
| IN8 | D11 | `IN_HAZARD` | Interrupteur dédié détresse (S1) | maintenu | 30 ms |

## 4. Sorties — 8 relais

Contacts secs 10 A. **Aucun étage de puissance externe n'est nécessaire** : ni
module MOSFET, ni relais externe, ni optocoupleur de coupure moteur.

| Relais | Bit registre | Nom logique | Charge | Régime |
|---|---|---|---|---|
| R1 | 0 | `OUT_PARK_FRONT` | Veilleuse avant | continu |
| R2 | 1 | `OUT_MAIN` | Phares (éclairage fort) | continu |
| R3 | 2 | `OUT_TURN_LEFT` | Clignotants gauche (AV + AR) | **cyclique 1,33 Hz** |
| R4 | 3 | `OUT_TURN_RIGHT` | Clignotants droite (AV + AR) | **cyclique 1,33 Hz** |
| R5 | 4 | `OUT_TAIL_PARK` | Feux de position arrière | continu |
| R6 | 5 | `OUT_TAIL_STOP` | Feux stop arrière | intermittent |
| R7 | 6 | `OUT_FAULT` | Voyant rouge de défaut | rare |
| R8 | 7 | `OUT_MOTOR_CUT` | Ligne frein du contrôleur | intermittent |

La correspondance bit ↔ relais est **directe** : bit 0 → CH1 … bit 7 → CH8.
Le tableau `RELAY_BIT[]` de `board_io.cpp` est donc devenu l'identité ; il est
conservé quand même, parce qu'il coûte huit octets et garde le reste du
firmware indifférent à la question.

**Ce que les relais imposent, en bien comme en mal :**

- La coupure moteur est un **contact sec**, galvaniquement isolé par
  construction — plus simple *et* plus sûr qu'un optocoupleur.
- **Aucune modulation.** Un feu arrière à circuit unique et intensité variable
  est matériellement impossible : il faut deux circuits, R5 et R6.
- **R3 et R4 sont les pièces d'usure du montage** : 4 800 manœuvres par heure
  de clignotement, soit ≈ 20 h de clignotement continu pour une endurance
  courante de 10⁵ manœuvres. Plusieurs dizaines de milliers de kilomètres à
  5 % du temps de roulage — acceptable, mais à savoir.
- **Le claquement remplace le buzzer** : à 1,33 Hz, c'est exactement le bruit
  d'un relais de clignotant d'origine. Une sortie et un composant économisés.
- Temps de commutation ~5 à 10 ms, à ajouter au budget du feu stop. On reste
  très en dessous des 50 ms de F-2.2.

### Validation globale des sorties (OE)

**A2** pilote l'entrée OE du registre des relais, active à l'état bas. À l'état
haut, **les huit relais retombent en un cycle d'horloge** sans que le contenu
du registre soit touché : l'état antérieur est restitué intact à la
réactivation. C'est un arrêt d'urgence matériel, exposé par
`board::outputsEnabled(false)` et disponible pour un futur mode sécurité.

> Au démarrage, `board::begin()` écrit `HIGH` sur A2 **avant** de la passer en
> sortie. Sur une broche encore en entrée, `digitalWrite(HIGH)` active le
> tirage interne : la broche est donc déjà haute quand elle devient une sortie.
> L'ordre inverse produirait une impulsion basse — **tous les relais collés** —
> pendant quelques microsecondes à chaque mise sous tension.

## 4 bis. L'afficheur

Le mot d'afficheur fait seize bits, émis avant l'octet des relais. Les segments
sont **répartis sur les deux octets** — l'hypothèse « un octet segments, un
octet sélection » est fausse.

| | Polarité | Bits du mot |
|---|---|---|
| Segments `A B C D E F G DP` | **actifs à l'état haut** | 4, 12, 7, 3, 1, 6, 11, 5 |
| Sélection des digits 1 à 4 | **active à l'état bas** | 2, 9, 10, 13 |
| Inutilisés | — | 0, 8, 14, 15 |

**Il n'y a pas de deux-points**, rien que des points décimaux, un par digit. Le
battement de cœur est donc porté par le **point du digit de gauche** : c'est le
seul qu'aucun format numérique ne réclame — un point après le chiffre des
milliers ne veut rien dire, alors que `12.5` ou `1.234` le placent après le
deuxième ou le troisième.

L'afficheur est **multiplexé** : chaque `board::refresh()` émet un digit, donc
quatre tours de boucle forment une trame complète. C'est le même appel qui
applique l'état des relais — tant qu'il n'a pas eu lieu, rien n'a bougé côté
matériel.

## 5. Câblage des freins

Deux contacteurs, de natures très différentes :

| | Frein avant | Frein arrière |
|---|---|---|
| Organe | Contact sec **unipolaire**, ouvert au repos | Connecteur Higo **rond jaune 3 broches** du kit Bafang |
| Commande | Les deux freins des roues avant | Le frein arrière |
| Nature électrique | Un seul jeu de contacts | Ligne logique ~5 V référencée à la masse du contrôleur |

### Frein avant — un contact, deux circuits, une diode

Un contact unipolaire semble ne pouvoir servir qu'une fois : câblé sur IN4 il
informe l'Arduino, mais ne ferme pas la ligne frein du contrôleur — ce qui
ferait dépendre du firmware la coupure d'assistance au frein principal.

Or les deux circuits demandent exactement la même chose : une mise à la
**masse**. Une **diode 1N4148, cathode côté `IN4`**, laisse donc un seul
contact les servir tous les deux, en bloquant le seul flux indésirable — le
+12 V de la carte remontant vers la ligne 5 V du contrôleur quand le contact
est ouvert.

| Contact | Entrée IN4 | Ligne frein Bafang |
|---|---|---|
| Ouvert | inactive | intacte, isolée du 12 V par la diode |
| Fermé | active | tirée à ~0,6 V → assistance coupée |

Coût : cinq centimes, et la diode se monte **dans le boîtier**, sur le bornier.
Rien à modifier au levier. Schéma et modes de défaillance en
`hardware/cablage.md` §4 et `07-securite.md` §2.

> Ceci suppose la ligne frein **active à l'état bas**, cas courant mais
> **à mesurer au multimètre avant de souder**.

### Frein arrière — ne pas toucher au connecteur jaune

Le connecteur rond jaune porte une ligne logique en ~5 V. **Ne jamais la
raccorder à une borne d'entrée de la carte** : celle-ci est tirée au +12 V à
travers la LED de son optocoupleur, et injecterait donc du 12 V dans une entrée
5 V du contrôleur. Destruction probable du contrôleur.

- **Le connecteur d'origine reste intact** et continue à couper l'assistance
  nativement : c'est la barrière indépendante de l'Arduino.
- **Un micro-rupteur S2 est ajouté sur le levier**, contact sec vers `IN5` et
  la masse, exactement comme le contacteur avant. C'est lui qui informe le
  firmware.

Si S2 est refusé, `hardware/cablage.md` §5 décrit une interface transistor qui
lit la ligne frein sans lui imposer de potentiel (`IN_INVERT_BRAKE_REAR 1`).
Elle coûte quatre composants et une soudure sur le faisceau moteur, pour
remplacer un micro-rupteur à 2 €.

### R8 — la coupure moteur

Contact sec, inséré sur la ligne frein par une **dérivation en Y** au format
Higo, sans couper le faisceau. Deux câblages selon la polarité réelle, à
déterminer au multimètre :

| Mesure sur le fil signal | Câblage de R8 |
|---|---|
| ~5 V au repos, 0 V au freinage *(cas courant)* | Contact **NO** entre signal et masse |
| 0 V au repos, ~5 V au freinage | Contact **NC** en série sur le signal |

Dans les deux cas **aucun potentiel n'est injecté** — c'est l'avantage que les
relais ont apporté sur la conception initiale à optocoupleur.

### Variantes de compilation

| `BRAKE_WIRING_VARIANT` | Signification |
|---|---|
| **2** *(défaut)* | Avant sur IN4, arrière sur IN5 via S2. Les deux freins sont discriminés |
| 1 | Une seule entrée frein câblée (IN4) ; IN5 est forcée inactive par le firmware. À n'utiliser que si S2 n'est pas monté — le feu stop ne s'allumera alors **pas** au frein arrière seul |

## 6. Procédure de vérification

À faire **avant de câbler le faisceau**, et à rejouer si la carte est
remplacée. **Carte alimentée en 12 V** : l'USB seul ne suffit pas, et c'est
contre-intuitif parce que la console série, elle, fonctionne parfaitement sans.

| Fonctionne sur USB seul | Exige le 12 V au bornier |
|---|---|
| Console série, boutons K1..K4, afficheur | **Relais** (bobines 12 V) et **entrées IN1..IN8** (LED des optocoupleurs alimentée depuis le +12 V) |

> **Discriminant : l'afficheur.** Les 74HC595 sont en 5 V, que le Nano fournit
> depuis l'USB. S'il s'allume et que rien ne claque → alimentation absente.
> S'il reste éteint *aussi* → le problème est dans la chaîne de registres.
> Alimenter la borne 12 V (F11, 2 A). USB et 12 V peuvent rester branchés
> ensemble : c'est le régime normal de mise au point.

1. **Téléverser `tools/pinscan`.** `VHELIO_SKETCH` est nécessaire aux **deux**
   commandes — chaque croquis a son propre répertoire de compilation, et
   `upload.sh` ne compile pas.
   ```bash
   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/upload.sh /dev/ttyACM0 old
   ```
   > **Le mot-clé `old` (57 600 bauds) est obligatoire** : le Nano de ce projet
   > porte un ancien bootloader (`HW 3 / FW 4.4`, signature `1E 95 0F`). Sans
   > lui, avrdude parle à 115 200 et enchaîne les `not in sync: resp=0x00`, qui
   > ressemblent à s'y méprendre à une panne de câblage. Il est en revanche
   > inutile de recompiler avec `old` : `atmega328` et `atmega328old`
   > produisent le même binaire.
   >
   > **Le nom du port ne dit rien de la carte** : celle-ci s'énumère en
   > `0843:5740 FIREPHX USB SER` sur `cdc_acm`, donc en `/dev/ttyACM0`, là où un
   > Nano officiel à FT232RL sort en `/dev/ttyUSB0`.
   >
   > **Diagnostic d'un téléversement qui ne passe pas** — avrdude sait
   > interroger la carte sans rien écrire (pas de `-U`, donc aucun risque) :
   > ```bash
   > avrdude -v -p atmega328p -c arduino -P /dev/ttyACM0 -b 57600
   > ```
   > `Device signature = 1E 95 0F` → le lien et le débit sont bons. Échec aux
   > **deux** débits → suspecter l'inverseur `485_ON` (§2 bis), puis le câble.
   >
   > **Sur NixOS**, le port appartient à `root:dialout` ; `upload.sh` le vérifie
   > et donne la ligne à ajouter. **Rouvrir la session** après le
   > `nixos-rebuild switch`.

2. **Ouvrir la console à 115 200 bauds** — le débit des croquis, à ne pas
   confondre avec le 57 600 du bootloader, qui ne vaut que pendant le
   téléversement :
   ```bash
   ./tools/monitor.sh
   ```
   `arduino-cli monitor` est inutilisable sur NixOS (outil prébuilt lié
   dynamiquement) ; `monitor.sh` n'utilise que `stty` et `cat`.

3. **Boutons — d'abord, parce que c'est le test de la liaison Nano ↔ carte.**
   Les poussoirs sont câblés **directement** aux broches D12, D10, D8 et A0 :
   ils ne dépendent ni du 12 V, ni de la chaîne, ni des optocoupleurs. **Si
   `K1..K4` ne bouge pas, le Nano n'est pas réellement connecté** — mal
   enfiché, décalé d'une rangée, ou une broche tordue sous le support. Tout ce
   qui précède passe par l'USB du Nano et ne prouve **rien** sur le support.

4. **Chaîne de registres.** Un digit doit s'allumer et les relais coller un par
   un, 1,5 s chacun, dans l'ordre annoncé.
   - Rien ne bouge, afficheur allumé → le défaut est côté relais : 12 V absent,
     mauvaises bornes, ou bobines 24 V.
   - Rien ne bouge, afficheur éteint → reprendre l'étape 3.
   - Tous les relais collent ensemble → OE mal identifiée.
   - L'ordre ne correspond pas → corriger `RELAY_BIT[]` dans `board_io.cpp`.

5. **Entrées.** Au repos, la console doit afficher `IN1..IN8 = 11111111`.
   Relier chaque borne **à la masse**, une par une :
   - le chiffre passe à `0` → entrées NPN confirmées, tous les communs à GND ;
   - rien ne bouge → **vérifier le 12 V avant toute autre conclusion**, son
     absence imite exactement une carte PNP. Carte alimentée et toujours rien →
     réessayer en appliquant **+12 V**. Si le chiffre passe alors à `0`, les
     entrées sont PNP : le firmware ne change pas, mais tous les communs vont
     au +12 V, et `hardware/cablage.md` §2 est à corriger.

   Noter tout écart d'**ordre** et corriger `IN_PIN[]`.

6. **Inverseur `485_ON` / `PRO`.** Sur `PRO`, la console répond ; sur `485_ON`,
   elle doit **cesser** de répondre. Le remettre sur `PRO` et l'y laisser.

7. Reporter les écarts dans `pins.h` et `board_io.cpp`.

> **Si rien ne répond du tout** — ni bouton, ni afficheur, ni relais — alors
> c'est l'hypothèse de brochage elle-même qui est fausse, et `pinscan` ne sait
> que *vérifier*. `tools/pinfind`, `tools/pinchain` et `tools/pindisp` le
> **découvrent** ; `tools/dispcheck` valide l'assemblage des mesures. La méthode
> complète est en [`archives/decouverte-brochage.md`](archives/decouverte-brochage.md).

## 7. Bilan des ressources

| Ressource | Utilisé | Libre |
|---|---|---|
| Entrées optocouplées | 8 / 8 (7 / 8 si S2 n'est pas monté) | 0 (ou 1) |
| Relais | 8 / 8 | 0 |
| Boutons carte | 1 / 4 (page d'afficheur) | 3, mais sous la coque |
| Afficheur | événements, vitesse, charge, défauts, odomètre — **maintenance seule** | — |
| Broches Nano hors carte | A1 (écoute Bafang), D13 (TX non câblé) | A6, A7 — analogiques seules |
| Timers | Timer0 (`millis`) | Timer1, Timer2 |
| UART matériel | console de mise au point (inverseur sur `PRO`) | — |
| RS485 | inutilisé, déconnecté par l'inverseur | disponible |

Le klaxon autonome avait libéré la voie **IN3 / R7** ; elle est réaffectée au
**diagnostic au poste de conduite** — voyant rouge sur R7, bouton
d'acquittement sur IN3. C'était le trou laissé par le montage sous la coque :
rien ne signalait un défaut avant l'ouverture.

Entrées et relais sont donc de nouveau **saturés**. Les marges restantes
tiennent en **trois boutons** inaccessibles et **deux broches analogiques**.
Toute fonction supplémentaire réclamant une sortie de puissance impose une
seconde carte.
