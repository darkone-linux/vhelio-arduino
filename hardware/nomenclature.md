# Nomenclature

Ce qui s'ajoute aux composants déjà listés dans le projet (moteur, batterie,
BMS, MPPT, comodo, feux, klaxon, convertisseur, boîtier fusibles).

## Calculateur

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| U1 | Arduino Nano (ATmega328P) | 1 | Vérifier le bootloader : « old » = ancien |
| U2 | Carte d'E/S rail DIN DN22D08 | 1 | Alimentation 12 V |
| — | Boîtier rail DIN étanche IP54 minimum | 1 | Le calculateur ne doit pas prendre l'eau |
| — | Presse-étoupes M12 / M16 | 6–8 | Selon le nombre de faisceaux |

## Étages de puissance

**Aucun.** Les huit sorties de la DN22D08 sont des relais à contacts secs
10 A : les charges se branchent directement dessus. Cela supprime de la
nomenclature initiale trois modules MOSFET, un relais de klaxon et un
optocoupleur.

Une seule réserve, optionnelle :

| Rep. | Désignation | Qté | Quand |
|---|---|---|---|
| K1 | Relais automobile 12 V / 20 A + support | 0 ou 1 | **Uniquement** si le klaxon est à compresseur : sa pointe d'appel peut dépasser le calibre 10 A du contact et le souder. R7 ne pilote alors que la bobine de K1. |

> Prévoir une diode de roue libre sur toute charge inductive ajoutée plus tard :
> les contacts de relais n'aiment pas les surtensions de coupure.

## Interface de lecture de la ligne frein Bafang

Seulement en variante A de câblage (`specs/03-affectation-es.md` §5).

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
| S1 | Interrupteur à bascule 12 V, feux de détresse | 1 | Le comodo n'en fournit pas |
| S2 | Micro-rupteur pour levier de frein arrière | 1 | Variante B seulement |

> **Pas de buzzer.** Les clignotants sont portés par les relais R3 et R4, dont
> le claquement à 1,33 Hz est exactement le bruit d'un relais de clignotant
> d'origine. Le composant et la sortie qu'il aurait consommée sont économisés.
> Corollaire : le boîtier ne doit pas étouffer complètement ce bruit.

## Protection

Détail des calibres dans `specs/04-electricite.md` §3.

| Désignation | Qté | Remarque |
|---|---|---|
| Boîtier porte-fusibles à lames, 8 à 12 voies, **12 V** | 1 | Réseau accessoire uniquement |
| Porte-fusibles MIDI/MEGA **58 V DC** | 2 | Départs 48 V |
| Porte-fusibles 10×38 mm gPV + cartouches | 2–3 | MPPT et panneau |
| Sectionneur DC ≥ 63 V / 40 A | 1 | **Un interrupteur automobile n'est pas qualifié pour couper 48 V DC** |

## Câblage

| Section | Usage | Longueur estimée |
|---|---|---|
| 6 mm² | Pack ↔ contrôleur, convertisseur → fusibles | 5 m |
| 4 mm² | MPPT ↔ pack | 2 m |
| 2,5 mm² | Convertisseur, klaxon, allume-cigare | 10 m |
| 1,5 mm² | Départs éclairage | 15 m |
| 0,75 mm² | Signaux comodo, freins, capteurs | 20 m |

Prévoir également : gaine annelée, cosses à sertir isolées, gaine
thermorétractable, colliers, et une **tresse de masse** vers le châssis.
