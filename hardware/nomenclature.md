# Nomenclature

Ce qui s'ajoute aux composants déjà listés dans le projet (moteur, batterie,
BMS, MPPT, comodo, feux, convertisseur, boîtier fusibles). Le klaxon est
autonome et sort du périmètre.

## Calculateur

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| U1 | Arduino Nano (ATmega328P) | 1 | Vérifier le bootloader : « old » = ancien |
| U2 | Carte d'E/S rail DIN DN22D08 | 1 | Alimentation **12 V**. Inverseur `485_ON`/`PRO` à laisser sur **`PRO`** |
| — | Boîtier rail DIN étanche IP54 minimum | 1 | Le calculateur ne doit pas prendre l'eau |
| — | Presse-étoupes M12 / M16 | 6–8 | Selon le nombre de faisceaux |

> **Aucun convertisseur USB-RS485 n'est nécessaire.** Le RS485 de la carte
> partage D0/D1 avec la console série ; l'inverseur `PRO` l'en déconnecte, et
> l'on programme le Nano par son propre port USB. Le RS485 ne servirait qu'à
> un futur bus Modbus entre plusieurs cartes.

> **Le boîtier est monté sous la coque**, donc l'afficheur n'est pas lisible en
> roulant : c'est un outil de maintenance. Corollaire important — **le boîtier
> ne doit pas étouffer le claquement des relais**, seul canal de retour vers le
> conducteur en marche. Le fixer sur un panneau rigide, qui fera caisse de
> résonance ; ne pas le caler dans de la mousse.

## Alimentation 12 V

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| — | Convertisseur 48 → 12 V non isolé, 10 A | 1 | **Déjà en possession.** Entrée 36–48 V, 60 V max — voir la réserve ci-dessous |
*(Le condensateur tampon de 10 000 µF est **sans objet** : il n'existait que
pour absorber l'appel du klaxon, lequel est autonome. L'éclairage complet
consomme 5,0 A pour 10 A disponibles, sans aucune pointe transitoire.)*

> **Marge de tension d'entrée insuffisante.** Le pack 16S monte à 58,4 V à
> 3,65 V/cellule, le convertisseur est donné pour 60 V : **2,7 % de marge**.
> Régler la charge du BMS JK à **3,50 V/cellule (56,0 V pack)** — gratuit,
> sans perte de capacité utile, et bénéfique pour la durée de vie du pack. Au
> prochain achat, prendre un convertisseur donné pour **72 V ou 80 V**.
> Un buck non isolé qui claque en court-circuit met le 48 V sur les feux.

## Étages de puissance

**Aucun en principe.** Les huit sorties de la DN22D08 sont des relais à
contacts secs 10 A : les charges se branchent directement dessus. Cela
supprime de la nomenclature initiale trois modules MOSFET, un relais de klaxon
et un optocoupleur.

Plus aucune réserve depuis que les phares ont été mesurés (12 W pièce) et que
le klaxon s'est révélé autonome :

| Rep. | Désignation | Qté | Quand |
|---|---|---|---|
| K1 | Relais automobile 12 V / 20 A + support | 0 | **Sans objet** : le klaxon est autonome et n'est pas relié à la carte. |
*(Le relais automobile envisagé pour les phares est sans objet : ils ont été
mesurés à **12 W chacun**, soit 2,5 A pour la paire sur un contact de 10 A.)*

> Prévoir une diode de roue libre sur toute charge inductive ajoutée plus tard :
> les contacts de relais n'aiment pas les surtensions de coupure.

## Interface de lecture de la ligne frein Bafang — *optionnelle*

Alternative au micro-rupteur S2, à ne monter que si l'ajout d'un rupteur sur
le levier arrière est impossible (`hardware/cablage.md` §5). Quatre composants
et une intervention sur le faisceau moteur, contre un rupteur à 2 €.

| Rep. | Désignation | Qté |
|---|---|---|
| Q1 | Transistor NPN BC547 | 1 |
| Q2 | MOSFET canal P (IRF9540N ou AO3401) | 1 |
| R1, R2 | Résistance 10 kΩ 1/4 W | 3 |
| R3 | Résistance 100 kΩ 1/4 W | 1 |

## Écoute UART Bafang

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| R4 | Résistance 1 kΩ 1/4 W | 1 | En série sur la ligne sniffée, vers **A4** |
| — | Connecteur Higo/JST au format du faisceau Bafang | 1 | Dérivation en Y, sans couper l'existant |
| OK2 | PC817 + résistance 10 kΩ | 1 | **Seulement** si le convertisseur 48/12 V est isolé |

## Signalisation et commande

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| S1 | Interrupteur à bascule **lumineux** 12 V, feux de détresse | 1 | Le comodo n'en fournit pas. Le modèle lumineux sert de témoin, ce qui évite tout câblage de voyant |
| S2 | Micro-rupteur pour levier de frein **arrière** | 1 | **Indispensable** : il informe le firmware sans qu'on ait à toucher au connecteur Bafang jaune. Contact sec vers IN5 et la masse |
| S3 | Interrupteur à bascule **lumineux** 12 V, veilleuse | 1 | Premier niveau d'éclairage, déjà en possession |
| **D1** | **Diode 1N4148** | **1** | **Indispensable.** Montée dans le boîtier entre la borne `IN4` et le fil vers la dérivation en Y, **cathode côté IN4**. Elle permet au contacteur de frein avant, pourtant unipolaire, de servir à la fois l'entrée de la carte et la ligne frein du contrôleur : la coupure d'assistance au frein avant cesse ainsi de dépendre du firmware (`hardware/cablage.md` §4). **Ne pas remplacer par une Schottky** : son courant de fuite inverse ferait remonter le potentiel de la ligne frein |
| — | LED verte 12 V + résistance 1 kΩ / 1 W | 0 à 2 | Témoins de clignotant, en parallèle sur R3 et R4. Purement électriques, aucun relais ni broche consommés |
| H1 | LED rouge 12 V + résistance 1 kΩ / 1 W | 1 | **Voyant de défaut** sur R7. Seul moyen de savoir en roulant qu'un défaut est actif, l'afficheur étant sous la coque. À monter dans le champ de vision |
| S5 | Bouton poussoir NO, contact sec | 1 | **Acquittement des défauts**, vers IN3 et la masse. Efface les défauts mémorisés et éteint le voyant sans couper l'alimentation. N'a aucun effet sur les feux, les freins ou le moteur |

> **Pas de buzzer.** Les clignotants sont portés par les relais R3 et R4, dont
> le claquement à 1,33 Hz est exactement le bruit d'un relais de clignotant
> d'origine. Le composant et la sortie qu'il aurait consommée sont économisés.
> Le rappel d'oubli emprunte le même canal : il raccourcit la phase allumée
> sans toucher à la période, ce qui rend le claquement syncopé donc
> reconnaissable, tout en gardant une cadence réglementaire.
>
> Corollaire, à ne pas négliger : **le boîtier ne doit pas étouffer ce bruit**,
> c'est le seul retour vers le conducteur en marche.

## Protection

Détail des calibres dans `specs/04-electricite.md` §3.

| Désignation | Qté | Remarque |
|---|---|---|
| Boîtier porte-fusibles à lames, 8 à 12 voies, **12 V** | 1 | Réseau accessoire uniquement. Prises allume-cigare fusiblées à **5 A**, pas 10 A : deux prises à 10 A dépasseraient à elles seules le convertisseur |
| Porte-fusibles MIDI/MEGA **58 V DC** | 2 | Départs 48 V |
| Porte-fusibles 10×38 mm gPV + cartouches | 2–3 | MPPT et panneau |
| Sectionneur DC ≥ 63 V / 40 A | 1 | **Un interrupteur automobile n'est pas qualifié pour couper 48 V DC** |

## Câblage

| Section | Usage | Longueur estimée |
|---|---|---|
| 6 mm² | Pack ↔ contrôleur moteur | 3 m |
| 4 mm² | Convertisseur → boîtier fusibles 12 V | 2 m |
| 4 mm² | MPPT ↔ pack | 2 m |
| 1,5 mm² | Convertisseur 48 V, départs éclairage, allume-cigare | 20 m |
| 0,75 mm² | Signaux comodo, freins, capteurs | 20 m |

Prévoir également : gaine annelée, cosses à sertir isolées, gaine
thermorétractable, colliers, et une **tresse de masse** vers le châssis.
