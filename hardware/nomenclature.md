# Nomenclature

Ce qui s'ajoute aux composants déjà listés dans le projet (moteur, batterie,
BMS, MPPT, comodo, feux, klaxon, convertisseur, boîtier fusibles).

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
| C1 | Condensateur électrolytique **10 000 µF / 25 V** | 1 | Au plus près du boîtier fusibles. **Sans lui, un coup de klaxon fait cligner les phares** : le convertisseur de 10 A ne couvre pas les deux. Le composant le plus rentable du montage |

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

Deux réserves, toutes deux optionnelles et conditionnées à une mesure :

| Rep. | Désignation | Qté | Quand |
|---|---|---|---|
| K1 | Relais automobile 12 V / 20 A + support | 0 ou 1 | **Uniquement** si le klaxon est à compresseur : sa pointe d'appel peut dépasser le calibre 10 A du contact et le souder. R7 ne pilote alors que la bobine de K1. |
| K2 | Relais automobile 12 V / 30 A + support | 0 ou 1 | **Uniquement** si les phares font 60 W **chacun** (120 W = 10 A, soit le calibre exact de R2, inrush compris). À mesurer au test T2.3. |

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
| S4 | Micro-rupteur pour levier de frein **avant** | 0 ou 1 | **Fortement recommandé** dès que possible. Câblé en parallèle sur la ligne frein du contrôleur, il rétablit la coupure d'assistance indépendante de l'Arduino au frein avant (`specs/07` §2). C'est la seule évolution matérielle de ce projet qui touche à la sécurité |
| — | LED verte 12 V + résistance 1 kΩ / 1 W | 0 à 2 | Témoins de clignotant, en parallèle sur R3 et R4. Purement électriques, aucun relais ni broche consommés |

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
| 2,5 mm² | Klaxon | 4 m |
| 1,5 mm² | Convertisseur 48 V, départs éclairage, allume-cigare | 20 m |
| 0,75 mm² | Signaux comodo, freins, capteurs | 20 m |

Prévoir également : gaine annelée, cosses à sertir isolées, gaine
thermorétractable, colliers, et une **tresse de masse** vers le châssis.
