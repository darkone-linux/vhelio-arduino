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

**Mesuré** avec `tools/pinfind` (§6 bis), et non plus déduit de l'IO22D08.

| Broche Nano | Rôle |
|---|---|
| D2, D3, D4, D5, D6, D7 | Entrées optocouplées IN1 à IN6 |
| D9, D11 | Entrées optocouplées IN7 et IN8 |
| D8, D10, D12 | Boutons K3, K2, K1 |
| A0 | Bouton K4 |
| A5 | **Données** de la chaîne (PC5) |
| A4 | **Horloge** de la chaîne (PC4) |
| A3 | **Verrou** de la chaîne (PC3) |
| A2 | **OE** du registre relais — validation, active à l'état bas |
| A1 | Écoute UART Bafang (RX logiciel, PCINT9) |
| D13 | TX logiciel, **non câblé** — *et LED intégrée du Nano* |
| A6, A7 | Libres, analogiques seules |
| D0, D1 | Console série — libres, voir §2 bis |

Les boutons occupent D8, D10, D12 puis A0 (= 14) : les numéros **décroissent**
de gauche à droite, ce qui explique la réputation de sérigraphie « inversée »
sur les cartes de cette famille. Il n'y a rien à inverser, K1 est bien le
poussoir de gauche.

Les trois lignes de la chaîne sont sur **PORTC**, contrairement au brochage
supposé qui les répartissait sur deux ports. Une seule écriture de port
suffirait à les piloter ensemble.

> **L'écoute Bafang survit, mais tout juste.** A4 lui était réservée ; la
> chaîne l'a prise avec A5, et l'OE a pris A2. Il ne restait que D13 et A1.
> Elle va sur **A1** : les deux ont bien l'interruption sur changement d'état
> qu'exige un RX logiciel (PCINT9, PCINT5), mais la LED intégrée du Nano
> charge D13 — sans importance pour une sortie, gênant pour une entrée qu'on
> écoute. A6 et A7 n'auraient pas pu du tout : analogiques seules, sans PCINT.
>
> D13 hérite du TX, que `SoftwareSerial` exige mais que rien ne câble. Il le
> met au repos à l'état haut : **la LED du Nano reste allumée en permanence**.
> Comme on n'émet jamais, la rendre à l'état d'entrée juste après `begin()`
> l'éteint.

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

> **La LED D13 n'est toujours pas utilisable comme témoin, mais la raison a
> changé.** On la croyait sur la ligne de données du registre, papillotant au
> rythme du rafraîchissement ; la mesure a mis les données sur A5. D13 porte
> désormais le TX logiciel du Bafang, que `SoftwareSerial` maintient à l'état
> haut au repos : la LED reste **allumée en fixe**. Un témoin qui ne varie
> jamais ne dit rien.
>
> Le battement de cœur reste donc sur le **deux-points de l'afficheur** :
> 1 Hz en fonctionnement nominal, ~4 Hz si un défaut est actif.

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
| IN6 | D7 | `IN_PARK` | Interrupteur dédié « veilleuse » | maintenu | 30 ms |
| IN7 | D9 | `IN_MAIN` | Comodo, éclairage fort | maintenu | 30 ms |
| IN8 | D11 | `IN_HAZARD` | Interrupteur dédié détresse (S1) | maintenu | 30 ms |

**Gain par rapport à l'ancienne hypothèse** : les huit entrées sont sur de
vraies broches numériques. Le contournement `analogRead()` sur A6/A7, et le
risque de broche flottante qu'il traînait, ont entièrement disparu.

## 4. Sorties — 8 relais

Contacts secs 10 A. **Aucun étage de puissance externe n'est nécessaire** :
ni module MOSFET, ni relais externe, ni optocoupleur de coupure moteur.

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

> **Mesuré : la correspondance est directe**, bit 0 → CH1 … bit 7 → CH8. Le
> décalage de l'IO22D08, où le relais 8 occupait le bit 0, **n'existe pas sur
> la DN22D08**. Une bizarrerie documentée depuis le début de ce projet vient
> donc de disparaître : elle avait été héritée avec le reste du brochage.
>
> Le tableau `RELAY_BIT[]` de `board_io.cpp` est conservé quand même, bien
> qu'il soit devenu l'identité. Il coûte huit octets et garde le reste du
> firmware indifférent à la question — c'est exactement ce qui a permis
> d'encaisser tous les changements de brochage de cette carte sans toucher un
> seul module métier.

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

La broche **A2** pilote l'entrée OE du registre des relais — mesuré (§6). À l'état haut, **tous
les relais retombent en un cycle d'horloge**, sans toucher au contenu du
registre : l'état antérieur est restitué intact à la réactivation. C'est un
arrêt d'urgence matériel, exploitable pour un futur mode sécurité.

Au démarrage, le firmware écrit `HIGH` sur A2 **avant** de la passer en sortie.
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

## 6 bis. Quand le pinscan ne montre rien — découvrir le brochage

Symptôme : Nano bien enfiché, carte alimentée, 5 V présent au support, et
pourtant **ni bouton, ni afficheur, ni relais**. Les poussoirs et l'afficheur
n'ont rien en commun sauf la liaison au Nano ; qu'ils soient morts tous les
deux, alors que la liaison est bonne, ne laisse qu'une explication : **le
brochage supposé est faux**.

C'est le §7 qui se répète un cran plus bas. Le brochage de `pins.h` vient de
`af3556/IO22_IO_Board`, bibliothèque qui déclare ne couvrir que l'**IO22D08**
et l'IO22C04. La **DN22D08** est un produit voisin mais distinct — rail DIN,
RS485, 12/24 V — et rien ne garantit qu'elle partage le brochage de l'IO22D08.

`tools/pinscan` ne sait que **vérifier** un brochage supposé ; il ne peut donc
rien dire quand c'est l'hypothèse elle-même qui est fausse.
`tools/pinfind` le **découvre**.

### Phase A — les entrées et les boutons

```bash
VHELIO_SKETCH=$PWD/tools/pinfind ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pinfind ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Toutes les broches utilisables passent en `INPUT_PULLUP` et **aucune n'est
pilotée en sortie** : aucun conflit possible avec ce que la carte impose, quel
que soit son brochage réel. Ce croquis ne peut rien abîmer.

1. Appuyer sur les poussoirs, un par un → donne `BTN_PIN[]`. Ne demande pas le
   12 V, les poussoirs sont câblés directement au Nano.
2. Carte alimentée, relier chaque borne d'entrée à la masse → donne `IN_PIN[]`,
   et confirme la polarité NPN au passage.

> `D13` lit `0` en permanence et ce **n'est pas** un signal de la carte : la
> LED intégrée du Nano charge le tirage interne, qui ne fait que ~30 kΩ. Vrai
> sur n'importe quel Arduino, carte ou pas.

Les broches qui ne bougent **jamais** sont les candidates de la chaîne de
registres. C'est ce que la phase A produit de plus utile : elle réduit
l'espace de recherche de la phase B à quatre ou cinq broches.

### Résultats mesurés sur l'exemplaire

**Boutons** — mesuré, la supposition était fausse :

| | Supposé d'après l'IO22D08 | **Mesuré sur la DN22D08** |
|---|---|---|
| Poussoirs | D7, D8, D9, D10 | **D8, D10, D12, A0** |

`A0` valant 14 sur un Nano, c'est **8, 10, 12, 14** : une progression de deux
en deux. Les broches impaires intercalées — `D9, D11, D13, A1` — sont donc les
candidates naturelles des quatre lignes de la chaîne (données, horloge,
verrou, OE). Hypothèse, pas conclusion : elle sert seulement à **ordonner** la
recherche de la phase B, pas à la remplacer.

**Entrées** — mesuré, carte alimentée. Elles réagissent **à la masse** :
la polarité NPN annoncée par le constructeur est confirmée, tous les communs
du faisceau vont donc à GND.

| | Supposé d'après l'IO22D08 | **Mesuré sur la DN22D08** |
|---|---|---|
| IN1 … IN5 | D2, D3, D4, D5, D6 | identiques |
| IN6 | A0 | **D7** |
| IN7 | D12 | **D9** |
| IN8 | D11 | identique |

**Il reste donc six broches, et six seulement**, pour huit relais et un
afficheur quatre digits : `D13, A1, A2, A3, A4, A5`. L'argument arithmétique
du §1 s'en trouve confirmé par la mesure.

L'hypothèse des « broches impaires » formée à partir des boutons est morte :
D9 et D11 sont des entrées. C'est précisément pourquoi on mesure.

### Phase B — la chaîne de registres

Aucun brochage de la DN22D08 n'est publié : la fiche du constructeur donne les
caractéristiques mais pas le câblage, et précise ne fournir « ni code
supplémentaire ni support technique ». Le balayage est donc la seule voie.

```bash
VHELIO_SKETCH=$PWD/tools/pinchain ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pinchain ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Le balayage est **manuel**, et c'est tout l'intérêt. Une première version
enchaînait les 240 essais toute seule, à la seconde : inutilisable. Plusieurs
triplets produisent un effet partiel — décaler des bits avec l'horloge et les
données interverties fait quand même bouger quelque chose — si bien que la
console défile trop vite pour qu'on note quoi que ce soit.

Les boutons de la carte, que la phase A vient d'identifier, règlent le
problème. Le montage reste dans l'état choisi indéfiniment.

| Bouton | Effet |
|---|---|
| K1 (D12) | Triplet suivant |
| K2 (D10) | Triplet précédent |
| K3 (D8) | Niveau des trois autres broches, `BAS` ↔ `HAUT` — couvre une validation OE parmi elles |
| K4 (A0) | Motif, `TOUS` ↔ `BIT A BIT` |

**Deux motifs, et le second est le juge.**

`TOUS` alterne `0xFF` et `0x00` : fort, repérable de loin, bon pour dégrossir.
Mais il ne prouve rien — un triplet faux fait souvent claquer quelque chose.

`BIT A BIT` promène **un seul 1** à travers les 24 bits. Sur le bon triplet,
et sur lui seul, on entend **exactement un relais à la fois**, proprement,
huit fois sur vingt-quatre positions. C'est ce qui distingue le triplet
correct de ceux qui font seulement du bruit.

Et il donne gratuitement `RELAY_BIT[]` : la position annoncée quand tel relais
claque **est** son bit dans la chaîne. C'est l'étape suivante faite d'avance.

Noter le **numéro du triplet**, le niveau des autres broches, et la position
de chacun des huit relais. Puis reporter dans `pins.h` et `board_io.cpp`.

#### Résultat mesuré

**Triplet 120 sur 120** : `données = A5`, `horloge = A4`, `verrou = A3`, les
trois autres broches à l'état **bas**.

La signature ne laisse pas de place au doute. En motif `BIT A BIT` : un relais
à la fois sur les huit premières positions, puis les segments de l'afficheur.
Les triplets faux, eux, faisaient bien claquer les relais — mais tous ensemble,
au rythme de l'horloge. Du bruit, pas de l'ordre. C'est exactement la
distinction que le motif `BIT A BIT` avait été ajouté pour trancher.

L'octet des relais est donc le **dernier** émis des trois.

**Les deux inconnues restantes ont été levées** par `tools/pinscan` :

| Question | Résultat mesuré |
|---|---|
| Correspondance bit ↔ relais | **Directe** : bit 0 → CH1 … bit 7 → CH8 |
| Emplacement de l'OE | **A2**, active à l'état bas. D13 et A1 sont sans effet sur les relais |

Le brochage de la DN22D08 est donc **entièrement mesuré**, à l'exception de
l'afficheur.

Le brochage de l'**afficheur** — sélection des digits, ordre des segments —
fait l'objet du §6 ter.

> **Le faisceau ne doit pas être câblé.** En motif `TOUS`, huit circuits sont
> fermés en même temps, ce qui n'est un régime prévu nulle part. À vide, les
> huit bobines représentent ≈ 300 mA sur le 12 V, que la carte encaisse sans
> difficulté.

Piloter ces six broches en sortie n'est sans risque que parce que la phase A a
prouvé qu'aucune ne porte d'entrée optocouplée ni de poussoir — donc qu'aucun
organe de la carte ne cherche à leur imposer un niveau.

## 6 ter. L'afficheur — phase D

Dernier organe non mesuré. Il n'est nécessaire à aucune fonction de conduite :
c'est un organe de maintenance, sous la coque (Q12). Mais il porte le battement
de cœur et l'affichage des défauts, et la LED D13 ne peut pas le remplacer
(§2 bis).

Acquis : les relais occupent le **dernier** octet émis, bits 0 à 7. Restent les
seize bits des deux premiers octets.

### Ce qu'une documentation officieuse propose

Une synthèse trouvée en ligne donne la structure suivante :

| Octet | Ordre d'émission | Rôle | Bits de trame |
|---|---|---|---|
| 1er | premier | sélection des digits | 16 à 23 |
| 2e | | segments, codage `DP G F E D C B A` | 8 à 15 |
| 3e | dernier | relais | 0 à 7 |

**À traiter comme une orientation, pas comme une source.** La même
documentation donne pour la chaîne `D13/A3/A2/A1`, c'est-à-dire le brochage de
l'IO22D08, que le balayage des 120 triplets a réfuté : chaque rôle y est
décalé d'un cran par rapport au nôtre. Elle se contredit d'ailleurs
elle-même — sa prose annonce les relais « décalés en premier » quand son code
les envoie en dernier.

Ce qu'elle apporte de solide, c'est que **son code place les relais en
dernier**, ce que nous avions mesuré indépendamment. Et sa structure explique
l'observation faite en phase B — un segment s'allumant sur les quatre digits à
la fois — qui est le comportement attendu si la sélection valait alors zéro et
les activait tous.

### La mesure

```bash
VHELIO_SKETCH=$PWD/tools/pindisp ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pindisp ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Un seul bit d'afficheur allumé à la fois, **l'octet des relais tenu à zéro** :
cette phase est muette, l'afficheur se règle à l'œil et des relais qui
claquent n'ajouteraient que du bruit. Avance à la main, aux boutons.

| Bouton | Effet |
|---|---|
| K1 (D12) | Bit suivant |
| K2 (D10) | Bit précédent |
| K3 (D8) | L'**autre** octet d'afficheur, `0x00` ↔ `0xFF` |
| K4 (A0) | Avance automatique, 1,5 s par bit |

**K3 est le test décisif.** L'autre octet à `0xFF` :

- un **segment** s'allume partout → c'est l'octet des segments ;
- un **digit entier** s'allume → c'est l'octet de sélection, et le rang du bit
  donne la position du digit.

### Résultat — les segments, mesurés

L'hypothèse « un octet segments, un octet sélection » est **fausse**. Les huit
segments sont répartis sur les **deux** octets, six d'un côté et deux de
l'autre.

Le mot d'afficheur fait seize bits : bits 0 à 7 = deuxième octet émis (trame
8 à 15), bits 8 à 15 = premier octet émis (trame 16 à 23).

| Segment | Bit du mot | Octet émis |
|---|---|---|
| A — haut | 4 | 2ᵉ, b4 |
| B — haut droite | **12** | 1ᵉʳ, b4 |
| C — bas droite | 7 | 2ᵉ, b7 |
| D — bas | 3 | 2ᵉ, b3 |
| E — bas gauche | 1 | 2ᵉ, b1 |
| F — haut gauche | 6 | 2ᵉ, b6 |
| G — milieu | **11** | 1ᵉʳ, b3 |
| DP — point | 5 | 2ᵉ, b5 |

**Ce relevé se valide lui-même.** Deux essais ont produit des caractères
entiers plutôt que des segments isolés : `6.` (A C D E F G + point) et `0.`
(A B C D E F + point). Une seule affectation est compatible avec les deux, et
c'est celle-ci. Deux lectures isolées annonçaient « haut gauche » là où le
modèle prédit « haut droite » — les caractères composés tranchent, et la
confusion gauche/droite à l'œil est sans conséquence.

### La sélection des digits est active à l'état bas

C'est le point le plus déroutant du relevé, et son explication. Un segment
seul s'allume sur **tous** les digits : sélection à zéro, les quatre digits
sont validés. Corollaire, **un bit de sélection ne montre rien** en parcours
positif — non qu'il soit inerte, mais parce qu'aucun segment n'est allumé pour
le révéler.

Un bit est déjà identifié par déduction : **2ᵉ octet b2 = digit 1**. C'est le
seul essai qui ait donné un écran entièrement noir (`bit 10` avec l'autre
octet à `0xFF`), donc le seul où le digit 1 était éteint en plus des autres.

### Le parcours négatif

D'où l'ajout d'un second sens à `pindisp`, sur K3 : **un seul 0, tout le reste
à 1**. Tous les segments allumés, toutes les sélections inhibées, écran noir.
Effacer un bit de sélection valide son digit, qui s'allume seul, tous segments
dehors — un `8.` franc. Effacer un bit de segment ne change rien.

Le négatif est au bit de sélection ce que le motif `BIT A BIT` de la phase B
était au triplet correct : le seul essai qui produise de l'ordre plutôt que du
bruit.

Restent trois digits et le deux-points. C'est la fin.

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
| Broches Nano hors carte | A1 (écoute Bafang), D13 (TX non câblé) | A6, A7 — analogiques seules |
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
