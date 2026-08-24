# 04 — Électricité de puissance

## 1. Le bus 48 V

| Grandeur | Valeur |
|---|---|
| Configuration | 16S LiFePO4 |
| Tension nominale | 51,2 V (16 × 3,2 V) |
| Tension pleine charge | 58,4 V (16 × 3,65 V) |
| Tension mini utile | 44,8 V (16 × 2,8 V, coupure basse BMS typique) |
| BMS principal | JK PB1A16S10P — 100 A continu, équilibrage actif |
| Charge | MPPT Victron SmartSolar, sortie 48 V |

> **Point d'attention majeur — la tension maximale.** Un pack 16S atteint
> **58,4 V** en fin de charge. C'est au-dessus de la tenue de la quasi-totalité
> de l'appareillage automobile. Voir §3.

## 2. Bilan de puissance côté 12 V

Le convertisseur retenu est un **48 → 12 V non isolé, entrée 36–48 V (60 V
max), sortie 12 V / 10 A**, soit **120 W**. C'est cette limite qui structure
tout ce qui suit.

### 2.1. Deux points d'attention sur ce convertisseur

> **La marge en tension d'entrée est trop faible.** Un pack 16S atteint
> **58,4 V** à 3,65 V/cellule. Le convertisseur est donné pour **60 V maximum**.
> Il reste **1,6 V de marge, soit 2,7 %** — moins que l'ondulation qu'un MPPT
> peut produire en fin d'absorption.
>
> Deux corrections, à appliquer de préférence toutes les deux :
>
> 1. **Abaisser la tension de charge du BMS JK à 3,50 V/cellule**, soit
>    **56,0 V pack**. La marge passe à 4 V. Le coût en capacité est
>    négligeable : une cellule LiFePO4 est chargée à plus de 99 % dès 3,45 V,
>    la fin de courbe n'apporte presque rien — et la durée de vie du pack y
>    gagne nettement. C'est gratuit et c'est le réglage à faire de toute façon.
> 2. **Prévoir un convertisseur donné pour 72 V ou 80 V d'entrée** au prochain
>    achat. La différence de prix est de quelques euros.
>
> Le mode de défaillance en cas de dépassement n'est pas bénin : un buck non
> isolé qui claque en court-circuit met le 48 V sur le réseau 12 V, donc sur
> les feux, le comodo et la carte.

> **120 W suffisent largement.** L'éclairage complet en consomme la moitié, et
> le klaxon — seule charge à forte pointe — est autonome. Voir §2.3.

### 2.2. Bilan des charges

Les phares ont été **mesurés à la pince** : **12 W chacun** (12 V / 1 A), et
non les 60 W de l'étiquette — écart typique des projecteurs LED annoncés en
« watts crête ». Le bilan retient **15 W chacun**, soit 30 W pour la paire,
pour se garder une marge.

La consommation de la carte est celle du constructeur : 12 mA en veille
afficheur éteint, **48 mA afficheur allumé**, environ 30 mA par relais collé,
**288 mA les huit relais collés**.

| Charge | Puissance | Courant 12 V | Régime |
|---|---|---|---|
| Phares (2 × 15 W retenus, **12 W mesurés**) | 30 W | 2,5 A | continu de nuit |
| Veilleuse avant | 10 W | 0,8 A | continu de nuit |
| Feux rouges arrière position (2 × 3 W) | 6 W | 0,5 A | continu de nuit |
| Feux stop (2 × 3 W) | 6 W | 0,5 A | intermittent |
| Clignotants (2 × 3 W par côté) | 6 W | 0,5 A | intermittent |
| Arduino + DN22D08, 6 relais collés | ~2,5 W | 0,21 A | continu |
| **Total « nuit + freinage + clignotant »** | | **≈ 5,0 A** | **la moitié du convertisseur** |
| Prise allume-cigare n°1 | 60 W *(fusible 5 A)* | 5 A | selon usage |
| Prise allume-cigare n°2 | 60 W *(fusible 5 A)* | 5 A | selon usage |

### 2.3. Un seul arbitrage subsiste

Deux bonnes nouvelles se cumulent : les phares consomment 2,5 A au lieu des
10 A redoutés, et **le klaxon est autonome** — sa propre batterie, son propre
interrupteur, aucun lien avec le réseau 12 V du véhicule. Or c'était la seule
charge capable de faire déborder le convertisseur.

**L'éclairage complet, freinage et clignotant compris, consomme 5,0 A pour
10 A disponibles.** Il n'y a plus de pointe transitoire à absorber :

- Le **condensateur tampon de 10 000 µF devient inutile.** Il n'existait que
  pour fournir l'appel du klaxon ; il disparaît de la nomenclature.
- Le **fusible F8** disparaît lui aussi.
- Reste un seul arbitrage : **fusibler les prises allume-cigare à 5 A**.
  Éclairage de nuit (5,0 A) + une prise à 10 A ferait 15 A, soit une fois et
  demie le convertisseur. À 5 A, une prise passe confortablement (10,1 A au
  pire, et les phares ne sont pas allumés en plein jour quand on recharge un
  téléphone).

Le relais R2 voit 2,5 A pour un calibre de 10 A : **aucun étage de puissance
externe n'est nécessaire**, et l'idée d'un relais automobile intercalé est
abandonnée.

Côté 48 V, la consommation réelle de nuit (60 W au secondaire) représente
60 / 51,2 / 0,9 ≈ **1,3 A**, et la pointe théorique de 120 W ≈ **2,6 A**.
Le convertisseur est **non isolé**, donc la masse 12 V est déjà la masse 48 V :
rien de particulier à faire pour l'écoute UART (§5, cas 1).

## 3. Protection — boîtier fusibles

> **Ne jamais utiliser de fusibles à lame automobile standard sur le bus 48 V.**
> Ils sont qualifiés **32 V DC**. À 58 V continu, l'arc peut ne pas s'éteindre :
> le fusible fond mais le courant continue de passer. Sur le bus 48 V, utiliser
> des fusibles **qualifiés ≥ 58 V DC** (MIDI/MEGA 58 V DC, Class T) ou des
> cartouches cylindriques 10 × 38 mm gPV (1000 V DC) telles qu'utilisées en
> photovoltaïque. Le boîtier fusibles à lames classique est réservé au **12 V**.

| Rep. | Circuit protégé | Tension | Calibre | Type |
|---|---|---|---|---|
| F1 | Pack → contrôleur moteur | 48 V | 30 A | MIDI 58 V DC |
| F2 | Pack → convertisseur 48/12 V | 48 V | 5 A | MIDI 58 V DC ou 10×38 gPV |
| F3 | MPPT → pack | 48 V | selon panneau (typ. 15 A) | 10×38 gPV |
| F4 | Panneau → MPPT | 48 V+ | selon panneau | 10×38 gPV |
| F5 | Sortie générale 12 V | 12 V | 15 A | MIDI / lame maxi (convertisseur 10 A) |
| F6 | Phares (éclairage fort) — 2,5 A mesurés | 12 V | 5 A | lame |
| F6b | Veilleuse avant | 12 V | 2 A | lame |
| F7 | Éclairage arrière + clignotants | 12 V | 5 A | lame |
| F9 | Prise allume-cigare n°1 | 12 V | **5 A** | lame — voir §2.3 |
| F10 | Prise allume-cigare n°2 | 12 V | **5 A** | lame — voir §2.3 |
| F11 | Arduino + carte DN22D08 + comodo — 288 mA max | 12 V | 2 A | lame |

Ajouter un **sectionneur / coupe-circuit 48 V DC** (≥ 63 V DC, ≥ 40 A) entre le
pack et tout le reste — pour la maintenance et en cas d'incident. Un
interrupteur automobile classique n'est **pas** qualifié pour couper 48 V DC.

## 4. Sections de câbles

Dimensionnement thermique et chute de tension < 3 %, cuivre souple.

| Liaison | Courant | Longueur aller-retour | Section |
|---|---|---|---|
| Pack ↔ contrôleur moteur | 25 A crête | 3 m | **6 mm²** |
| Pack ↔ convertisseur 48/12 | 3 A | 3 m | **1,5 mm²** |
| MPPT ↔ pack | 15 A | 2 m | **4 mm²** |
| Convertisseur → boîtier fusibles 12 V | 10 A | 1,5 m | **4 mm²** |
| Départ éclairage 12 V | 3 A | 6 m | **1,5 mm²** |
| Départ allume-cigare | 5 A | 3 m | **1,5 mm²** |
| Signaux comodo / freins vers DN22D08 | < 0,05 A | 4 m | **0,75 mm²** |

## 4 bis. Les relais de la carte remplacent tous les étages de puissance

Les huit sorties sont des **contacts secs 10 A**. Aucun module MOSFET, aucun
relais externe, aucun optocoupleur n'est nécessaire :

| Charge | Courant | Verdict |
|---|---|---|
| Veilleuse avant | 0,8 A | direct sur R1 |
| Phares, **12 W mesurés par phare** (15 W retenus) | 2,5 A | direct sur R2 |
| Clignotants (2 × 3 W par côté) | 0,5 A | direct sur R3 / R4 |
| Feux de position arrière | 0,5 A | direct sur R5 |
| Feux stop arrière | 0,5 A | direct sur R6 |
| Ligne frein du contrôleur | < 0,05 A | direct sur R8, contact sec isolé |

**Le klaxon ne figure plus dans ce tableau** : il est autonome, avec sa propre
batterie et son propre interrupteur. R7 est libre.

**Réserve sur les charges inductives.** Si une charge inductive est un jour
ajoutée, prévoir une diode de roue libre : les contacts de relais n'aiment pas
les surtensions de coupure.

## 5. Masses et référence de potentiel

L'écoute de la liaison UART Bafang impose que l'Arduino **partage la masse du
contrôleur moteur**. Trois cas :

1. **Convertisseur non isolé (buck) — c'est le cas du matériel retenu.** La
   masse 12 V est déjà la masse 48 V, donc la masse du contrôleur. Rien à
   faire : relier simplement la masse du connecteur UART à la masse de la
   carte DN22D08. Les deux cas suivants sont conservés pour mémoire.
2. **Convertisseur isolé.** Réaliser un **unique point de liaison** (« masse en
   étoile ») entre le négatif 12 V et le négatif 48 V, au plus près du
   convertisseur. Un seul point, jamais deux, pour éviter les boucles de masse.
3. **Isolation à conserver absolument.** Insérer un optocoupleur (PC817 +
   résistance de tirage 10 kΩ côté Arduino) sur la ligne sniffée. À 1200 bauds,
   un PC817 est largement assez rapide.

La sortie `OUT_MOTOR_CUT` est un **contact sec de relais**, galvaniquement
isolé par construction. La difficulté qui imposait un optocoupleur dans la
version initiale de cette spécification a disparu d'elle-même avec le passage
aux relais : le contact se referme sur la ligne frein sans y injecter aucun
potentiel.

## 6. Le second BMS Daly 60 A

**Tranché : rechange au garage, non câblé.** C'est le cas simple et sans
risque ; le Daly remplace le JK en cas de panne. Le pack conserve ses 100 A et
il n'y a aucun réglage de seuils à coordonner.

La section ci-dessous est conservée pour mémoire, au cas où l'option serait
un jour reconsidérée.

- **Second BMS câblé en série sur le même pack.** Techniquement possible (les
  MOSFET des deux BMS sont en série sur le chemin de courant, chaque BMS ayant
  son propre faisceau d'équilibrage), mais cela demande de la rigueur : les
  faisceaux de mesure des deux BMS lisent les mêmes cellules, les seuils
  doivent être décalés (le Daly plus permissif que le JK, pour qu'il n'agisse
  qu'en secours), et le courant est plafonné par le plus petit des deux, soit
  **60 A** — pas 100 A.

Dans tous les cas, l'Arduino n'interagit avec **aucun** des deux BMS et ne
voit rien du pack : c'est une limite explicite, rappelée en `07-securite.md` §5.

## 7. Évolution : lecture du MPPT Victron

Le SmartSolar expose un port **VE.Direct** : sortie texte ASCII, 19 200 bauds,
une trame toutes les secondes (tension pack, courant de charge, puissance PV,
état). Il serait tentant de la lire pour afficher la production solaire.

Ce n'est **pas** possible dans le brochage actuel : il n'y a plus d'UART libre,
et un second port logiciel à 19 200 bauds bloquerait les interruptions trop
longtemps. Deux voies si le besoin se confirme :

- désactiver l'écoute Bafang (`BAFANG_ENABLE 0`) et réaffecter A4 au VE.Direct ;
- passer sur un Arduino Mega 2560 (4 UART matériels), qui accepte la même carte
  d'E/S via un adaptateur de brochage.
