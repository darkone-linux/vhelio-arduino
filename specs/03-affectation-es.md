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
| D0, D1 | Console série — libres, voir §2 bis |

## 2 bis. Le RS485 et l'inverseur « 485_ON / PRO »

La carte porte un émetteur-récepteur RS485 (boîtier 8 broches, près des bornes
`A+` / `B-`) **relié à D0/D1**, donc en conflit direct avec la console série et
avec le téléversement. Un **inverseur à glissière** sérigraphié `485_ON` d'un
côté, `PRO` de l'autre, l'en déconnecte.

| Position | Effet |
|---|---|
| **`PRO`** | Le RS485 est isolé de D0/D1. Console série et téléversement USB normaux. **C'est la position d'exploitation de ce projet.** |
| `485_ON` | Le RS485 prend D0/D1. La console série et le téléversement cessent de fonctionner. |

Conséquences pratiques :

- **Laisser l'inverseur sur `PRO` en permanence.** Le RS485 n'est utilisé par
  aucune fonction du projet, et `DEBUG_SERIAL 1` reste valide.
- **Aucun convertisseur USB-RS485 n'est nécessaire.** L'ordinateur dialogue
  avec le Nano par son propre port USB, comme n'importe quel Arduino. Le RS485
  n'a d'intérêt que pour un futur bus Modbus (§ « Évolutions » de `09`).
- Si un jour le RS485 sert, la console de mise au point devra basculer sur
  l'afficheur 4 digits (`DEBUG_SERIAL 0`) — mais l'afficheur est sous la coque
  (Q12), ce qui rend l'arbitrage nettement défavorable au RS485.

> Si un téléversement échoue sans raison apparente, cet inverseur est **le
> piège classique de cette carte** — mais il n'en est pas le premier suspect.
> Tester d'abord le débit du bootloader (§6, étape 1) : cela coûte une
> commande et n'oblige pas à ouvrir le boîtier. C'est d'ailleurs ce qui s'est
> produit ici, le `not in sync: resp=0x00` venait de l'ancien bootloader et
> pas du RS485.

> **La LED D13 n'est pas utilisable comme témoin.** Elle est sur la ligne de
> données du registre et papillote au rythme du rafraîchissement. Le battement
> de cœur est reporté sur le **deux-points de l'afficheur** : 1 Hz en
> fonctionnement nominal, ~4 Hz si un défaut est actif.

## 3. Entrées — IN1 … IN8

Optocoupleurs **NPN, déclenchement à l'état bas** — confirmé par la fiche du
constructeur : « 8x opto-isolated inputs (low level trigger, NPN type) ». La
LED de l'optocoupleur
est alimentée depuis le +12 V de la carte à travers une résistance, et l'on
active une entrée en **fermant sa borne sur la masse**. Côté Nano,
l'optocoupleur tire la broche à l'état bas ; le firmware active `INPUT_PULLUP`,
ce qui donne un état inactif franc même carte non alimentée.

> **Tous les organes de commande sont donc de simples contacts secs vers la
> masse** : comodo, interrupteur veilleuse, interrupteur détresse, contacteurs
> de frein. C'est ce qui rend utilisable le contacteur de frein avant, qui est
> unipolaire (§5). Le commun du comodo va à **GND**, pas au +12 V.
>
> Le pinscan le confirme en trente secondes (§6, étape 4). Et si l'exemplaire
> démentait sa propre fiche, **le firmware serait identique** : seul le fil de
> commun changerait de borne.

| # | Broche | Nom logique | Source | Type | Anti-rebond |
|---|---|---|---|---|---|
| IN1 | D2 | `IN_TURN_LEFT` | Comodo, position gauche | maintenu | 30 ms |
| IN2 | D3 | `IN_TURN_RIGHT` | Comodo, position droite | maintenu | 30 ms |
| IN3 | D4 | `IN_ACK` | Bouton d'acquittement des défauts | momentané | 20 ms |
| IN4 | D5 | `IN_BRAKE_FRONT` | Contacteur frein avant (unipolaire, contact sec) | momentané | 15 ms |
| IN5 | D6 | `IN_BRAKE_REAR` | Micro-rupteur S2 sur le levier arrière | momentané | 15 ms |
| IN6 | A0 | `IN_PARK` | Interrupteur dédié « veilleuse » | maintenu | 30 ms |
| IN7 | D12 | `IN_MAIN` | Comodo, éclairage fort | maintenu | 30 ms |
| IN8 | D11 | `IN_HAZARD` | Interrupteur dédié détresse (S1) | maintenu | 30 ms |

**Gain par rapport à l'ancienne hypothèse** : les huit entrées sont sur de
vraies broches numériques. Le contournement `analogRead()` sur A6/A7, et le
risque de broche flottante qu'il traînait, ont entièrement disparu.

## 4. Sorties — 8 relais

Contacts secs 10 A. **Aucun étage de puissance externe n'est nécessaire** :
ni module MOSFET, ni relais externe, ni optocoupleur de coupure moteur.

| Relais | Bit registre | Nom logique | Charge | Régime |
|---|---|---|---|---|
| R1 | 1 | `OUT_PARK_FRONT` | Veilleuse avant | continu |
| R2 | 2 | `OUT_MAIN` | Phares (éclairage fort) | continu |
| R3 | 3 | `OUT_TURN_LEFT` | Clignotants gauche (AV + AR) | **cyclique 1,33 Hz** |
| R4 | 4 | `OUT_TURN_RIGHT` | Clignotants droite (AV + AR) | **cyclique 1,33 Hz** |
| R5 | 5 | `OUT_TAIL_PARK` | Feux de position arrière | continu |
| R6 | 6 | `OUT_TAIL_STOP` | Feux stop arrière | intermittent |
| R7 | 7 | `OUT_FAULT` | Voyant rouge de défaut | rare |
| R8 | **0** | `OUT_MOTOR_CUT` | Ligne frein du contrôleur | intermittent |

> L'ordre des bits n'est pas séquentiel : le relais 8 occupe le **bit 0**, les
> relais 1 à 7 les bits 1 à 7. C'est le câblage de la carte. Le tableau
> `RELAY_BIT[]` de `board_io.cpp` encode cette bizarrerie une fois pour
> toutes ; aucun module métier ne la voit.

### Ce que les relais changent, en bien

- **La coupure moteur devient un contact sec.** Plus besoin d'optocoupleur :
  un relais est galvaniquement isolé par construction. C'est plus simple *et*
  plus sûr que la solution initiale.
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

## 5. Câblage des freins

Le matériel réel impose le schéma. Deux contacteurs, de natures très
différentes :

| | Frein avant | Frein arrière |
|---|---|---|
| Organe | Contact sec **unipolaire**, ouvert au repos | Connecteur Higo **rond jaune 3 broches** du kit Bafang |
| Commande | Les deux freins des roues avant | Le frein arrière |
| Nature électrique | Un seul jeu de contacts, rien d'autre | Ligne logique ~5 V référencée à la masse du contrôleur |

### Frein avant — un seul contact pour deux circuits, grâce à une diode

Les entrées étant NPN (§3), le contacteur se câble entre la borne `IN4` et la
masse. Rien d'autre n'est nécessaire : pas de +12 V commuté, pas d'interface.

**Un contact unipolaire semble ne pouvoir servir qu'une fois** : câblé sur IN4
il informe l'Arduino, mais ne ferme pas la ligne frein du contrôleur — ce qui
ferait dépendre du firmware la coupure d'assistance au frein principal d'un
tricycle.

Il se trouve que les deux circuits demandent exactement la même chose : une
mise à la **masse**. Une **diode 1N4148**, cathode côté `IN4`, laisse donc un
seul contact les servir tous les deux, en bloquant le seul flux indésirable —
le +12 V de la carte remontant vers la ligne 5 V du contrôleur quand le contact
est ouvert.

| Contact | Entrée IN4 | Ligne frein Bafang |
|---|---|---|
| Ouvert | inactive | intacte, isolée du 12 V par la diode |
| Fermé | active | tirée à ~0,6 V → assistance coupée |

Coût : cinq centimes, et la diode se monte **dans le boîtier**, sur le bornier.
Rien à modifier au levier. Schéma complet et modes de défaillance en
`hardware/cablage.md` §4 et `07-securite.md` §2.

> Ceci suppose la ligne frein **active à l'état bas**, ce qui est le cas
> courant mais reste à mesurer au multimètre avant de souder.

### Frein arrière — ne pas toucher au connecteur jaune

Le connecteur rond jaune à 3 broches porte une ligne logique en ~5 V (masse,
alimentation capteur, signal). **Il ne faut surtout pas la raccorder à une
borne d'entrée de la carte** : celle-ci est tirée au +12 V à travers la LED de
son optocoupleur, et injecterait donc du 12 V dans une entrée 5 V du
contrôleur. Destruction probable du contrôleur.

Solution retenue, la plus simple et la plus sûre :

- **Le connecteur d'origine reste intact**, il continue à couper l'assistance
  nativement. C'est la barrière 1, entièrement indépendante de l'Arduino.
- **Un micro-rupteur S2 est ajouté sur le levier arrière**, câblé en contact
  sec vers `IN5` et la masse, exactement comme le contacteur avant. C'est lui
  qui informe le firmware.

Si l'ajout de S2 est refusé, `hardware/cablage.md` §5 décrit une interface
transistor qui lit la ligne frein Bafang sans lui imposer de potentiel
(`IN_INVERT_BRAKE_REAR 1`). Elle fonctionne, mais coûte quatre composants et
une soudure sur le faisceau moteur, pour remplacer un micro-rupteur à 2 €.

> La diode D1 du frein avant ne transpose **pas** au frein arrière : le
> contacteur Bafang d'origine est déjà câblé au contrôleur et n'est pas
> accessible comme contact sec libre. C'est bien un organe séparé qu'il faut
> pour informer le firmware.

### R8 — la coupure moteur

Contact sec, à insérer sur la ligne frein du contrôleur via une **dérivation
en Y** au format Higo, sans couper le faisceau. Deux câblages possibles selon
la polarité réelle du capteur, à déterminer au multimètre :

| Mesure sur le fil signal | Câblage de R8 |
|---|---|
| ~5 V au repos, 0 V au freinage *(cas courant)* | Contact **NO** entre signal et masse : R8 collé = freinage simulé |
| 0 V au repos, ~5 V au freinage | Contact **NC** en série sur le signal : R8 collé = ligne ouverte |

Dans les deux cas, c'est un contact sec : **aucun potentiel n'est injecté**, ce
qui est précisément l'avantage que les relais ont apporté sur la conception
initiale à optocoupleur.

### Variantes de compilation

| `BRAKE_WIRING_VARIANT` | Signification |
|---|---|
| **2** *(défaut)* | Avant sur IN4, arrière sur IN5 via S2. Les deux freins sont discriminés. |
| 1 | Une seule entrée frein câblée (IN4). IN5 est forcée inactive par le firmware. À n'utiliser que si S2 n'est pas monté — le feu stop ne s'allumera alors **pas** au frein arrière seul. |

## 6. Procédure de vérification

À faire **avant de câbler le faisceau** — mais **carte alimentée en 12 V**.
L'USB seul ne suffit pas, et c'est contre-intuitif parce que la console série,
elle, fonctionne parfaitement sans.

| Fonctionne sur USB seul | Exige le 12 V au bornier |
|---|---|
| Console série, boutons K1..K4, afficheur | **Relais** (bobines 12 V) et **entrées IN1..IN8** (la LED de chaque optocoupleur est alimentée par le +12 V de la carte, §3) |

> **Symptôme.** Aucun relais ne claque à l'étape 3, et les entrées restent à
> `11111111` quoi qu'on relie à la masse à l'étape 4. La carte n'est pas
> alimentée.
>
> **Discriminant : l'afficheur.** Les 74HC595 sont alimentés en 5 V, que le
> Nano fournit depuis l'USB. L'afficheur doit donc s'allumer même sans 12 V.
> S'il s'allume et que rien ne claque → alimentation. S'il reste éteint
> *aussi* → le problème est bien dans la chaîne de registres.
>
> Alimenter la borne 12 V de la carte (F11, 2 A — `hardware/cablage.md`).
> **USB et 12 V peuvent rester branchés ensemble**, c'est le régime normal de
> mise au point.

1. Téléverser le croquis de contrôle. **`VHELIO_SKETCH` est nécessaire aux
   deux commandes** : chaque croquis a son propre répertoire de compilation,
   et `upload.sh` ne compile pas — il téléverse ce que la compilation a
   produit.
   ```bash
   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/upload.sh /dev/ttyACM0 old
   ```

   > **Le Nano de ce projet porte un ancien bootloader : le mot-clé `old`
   > (57 600 bauds) est obligatoire au téléversement.** Sans lui, avrdude
   > parle à 115 200 et enchaîne dix `not in sync: resp=0x00`, message qui
   > ressemble à s'y méprendre à une panne de câblage. Mesuré sur
   > l'exemplaire : `HW Version 3 / FW Version 4.4`, signature `1E 95 0F`.
   >
   > Il est en revanche **inutile de recompiler avec `old`** : `atmega328` et
   > `atmega328old` partagent `build.mcu` et `maximum_size` (30 720 octets) et
   > produisent le même binaire. Seul le débit de `upload.sh` change.

   Le port se passe en premier argument pour forcer la détection. **Son nom ne
   dit rien de la provenance de la carte** : celle-ci s'énumère en
   `0843:5740 FIREPHX USB SER` sur le pilote `cdc_acm`, donc en
   `/dev/ttyACM0`, alors qu'un Nano officiel à FT232RL sort en `/dev/ttyUSB0`.
   Ne rien déduire du port, ni sur le bootloader ni sur l'authenticité.

   > **Diagnostic d'un téléversement qui ne passe pas.** avrdude sait
   > interroger la carte sans rien écrire — pas de `-U`, donc aucun risque :
   > ```bash
   > avrdude -v -p atmega328p -c arduino -P /dev/ttyACM0 -b 57600
   > ```
   > `Device signature = 1E 95 0F` → le lien série et le débit sont bons.
   > Échec **aux deux débits** (115 200 puis 57 600) → suspecter alors
   > l'inverseur `485_ON` (§2 bis), puis le câble, puis l'auto-reset.

   > **Prérequis NixOS.** Le port série appartient à `root:dialout` et
   > l'utilisateur n'est pas dans ce groupe par défaut. `upload.sh` le vérifie
   > avant de lancer avrdude et donne la ligne de configuration à ajouter. La
   > session doit être **rouverte** après le `nixos-rebuild switch`.

2. Ouvrir la console à 115 200 bauds :
   ```bash
   ./tools/monitor.sh
   ```

   > **115 200 est le débit des croquis** (`Serial.begin`). Le 57 600 de
   > l'étape 1 est celui de l'ancien bootloader et ne vaut **que pendant le
   > téléversement**. Les confondre donne une console illisible, pas une
   > console muette — c'est ce qui rend la confusion tenace.
   >
   > **Ne pas utiliser `arduino-cli monitor` sur NixOS.** Il réclame l'outil
   > prébuilt `builtin:serial-monitor`, lié dynamiquement contre un `/lib`
   > inexistant, et échoue sur `No monitor available for the port protocol
   > serial` après l'avoir téléchargé. Lancé sans les variables
   > `ARDUINO_DIRECTORIES_*`, il peuple en prime `~/.arduino15` au lieu du
   > `./.arduino` du dépôt. `monitor.sh` n'utilise que `stty` et `cat`.
3. **Boutons — c'est le test de la liaison Nano ↔ carte, à faire en premier.**
   Appuyer sur les quatre poussoirs, de gauche à droite.

   > Les poussoirs sont câblés **directement** de la carte aux broches D7..D10.
   > Ils ne dépendent ni du 12 V, ni de la chaîne de registres, ni des
   > optocoupleurs : c'est le seul organe de la carte que le Nano voit sans
   > aucun intermédiaire. **Si `K1..K4` ne bouge pas, le Nano n'est pas
   > réellement connecté à la carte** — mal enfiché, décalé d'une rangée,
   > retourné, ou une broche tordue sous le support.
   >
   > Tout ce qui précède — console, téléversement — passe par l'USB du Nano et
   > ne prouve **rien** sur le support : un Nano posé à côté de la carte
   > donnerait exactement les mêmes résultats. D'où l'ordre de cette étape.

   Attention : la sérigraphie `K1..K4` est réputée **inversée** par rapport au
   câblage sur cette famille de cartes — le poussoir marqué `K4` serait celui
   relié à D7. Noter quel poussoir physique fait passer `K1` à `0` : c'est
   celui qui changera de page d'afficheur.
4. **Chaîne de registres.** Un digit doit s'allumer sur l'afficheur et les
   relais doivent coller **un par un**, 1,5 s chacun, dans l'ordre annoncé.
   - Rien ne bouge, mais l'afficheur s'allume : la chaîne fonctionne, le
     défaut est du côté des relais — 12 V absent, appliqué aux mauvaises
     bornes, ou bobines prévues pour 24 V.
   - Rien ne bouge et l'afficheur reste éteint : la chaîne ne tourne pas.
     Reprendre l'étape 3 — c'est presque toujours la liaison Nano ↔ carte.
     Les broches data / horloge / verrou ne viennent qu'après : elles sont
     recoupées avec `af3556/IO22_IO_Board`, qui donne les mêmes.
   - Si tous les relais collent en même temps : OE est mal identifiée.
   - Si l'ordre ne correspond pas : corriger `RELAY_BIT[]` dans `board_io.cpp`.
5. **Entrées.** La polarité est donnée par le constructeur (NPN) ; il ne
   s'agit que de la confirmer. Au repos, la console doit afficher
   `IN1..IN8 = 11111111`. Relier alors chaque borne d'entrée **à la masse**,
   une par une, avec un simple fil volant :
   - le chiffre correspondant passe à `0` → **entrées NPN**, hypothèse
     confirmée, tous les communs du faisceau vont à la masse ;
   - rien ne bouge → **vérifier le 12 V de la carte avant toute autre
     conclusion** : sans lui la LED de l'optocoupleur ne peut pas conduire et
     aucune entrée ne bougera jamais, ce qui imite exactement une carte PNP.
     Carte alimentée et toujours rien → réessayer en appliquant **+12 V** sur
     la borne. Si le
     chiffre passe à `0`, les entrées sont PNP : **le firmware ne change pas**,
     mais tous les communs du faisceau vont au +12 V. Corriger
     `hardware/cablage.md` §2 en conséquence.

   Noter aussi tout écart d'**ordre** (borne IN3 qui fait bouger le 5ᵉ
   chiffre, par exemple) et corriger `IN_PIN[]`.
6. **Inverseur `485_ON` / `PRO`.** Le mettre sur `PRO` et vérifier que la
   console répond. Le basculer sur `485_ON` : la console doit **cesser** de
   répondre — ce qui confirme que le RS485 est bien sur D0/D1. Le remettre sur
   `PRO` et l'y laisser (§2 bis).
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
| Entrées optocouplées | 8 / 8 (7 / 8 si S2 n'est pas monté) | 0 (ou 1) |
| Relais | 8 / 8 | 0 |
| Boutons carte | 1 / 4 (page d'afficheur) | 3, mais sous la coque |
| Afficheur | vitesse, charge, défauts, odomètre — **maintenance seule** | — |
| Broches Nano hors carte | A4, A5 (écoute Bafang) | A6, A7 |
| Timers | Timer0 (`millis`) | Timer1, Timer2 |
| UART matériel | console de mise au point (inverseur sur `PRO`) | — |
| RS485 | inutilisé, déconnecté par l'inverseur | disponible |

Le klaxon autonome avait libéré la voie **IN3 / R7** ; elle est réaffectée au
**diagnostic au poste de conduite** (Q14, option A) : voyant rouge sur R7,
bouton d'acquittement sur IN3. C'était le trou laissé par Q12 — le boîtier
étant sous la coque, rien ne signalait un défaut avant l'ouverture.

Les entrées et les relais sont donc de nouveau **saturés**. Les marges
restantes tiennent en **trois boutons** (inaccessibles, sous la coque) et
**deux broches analogiques**. Toute fonction supplémentaire nécessitant une
sortie de puissance impose désormais une seconde carte.
