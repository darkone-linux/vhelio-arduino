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

Nécessaires si les sorties de la DN22D08 sont des transistors ~0,5 A
(question Q2 de `specs/09-questions-ouvertes.md`).

| Rep. | Désignation | Qté | Charge |
|---|---|---|---|
| K1 | Relais automobile 12 V / 20 A + support | 1 | Klaxon — **obligatoire** |
| M1 | Module MOSFET 12 V / 10 A à entrée logique, **compatible PWM** | 1 | Feux rouges arrière |
| M2, M3 | Module MOSFET 12 V / 10 A | 2 | Feu de croisement, feu de route |
| OK1 | Optocoupleur PC817 + résistance 1 kΩ | 1 | Coupure moteur vers ligne frein |

> Le module M1 doit accepter du PWM à ~490 Hz. Beaucoup de modules « relais
> statique » à optocoupleur lent ne le supportent pas : vérifier la fiche.

## Interface de lecture de la ligne frein Bafang

Seulement en variante A de câblage (`specs/03-affectation-es.md` §4).

| Rep. | Désignation | Qté |
|---|---|---|
| Q1 | Transistor NPN BC547 | 1 |
| Q2 | MOSFET canal P (IRF9540N ou AO3401) | 1 |
| R1, R2 | Résistance 10 kΩ 1/4 W | 3 |
| R3 | Résistance 100 kΩ 1/4 W | 1 |

## Écoute UART Bafang

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| R4 | Résistance 1 kΩ 1/4 W | 1 | En série sur la ligne sniffée |
| — | Connecteur Higo/JST au format du faisceau Bafang | 1 | Dérivation en Y, sans couper l'existant |
| OK2 | PC817 + résistance 10 kΩ | 1 | **Seulement** si le convertisseur 48/12 V est isolé |

## Signalisation et commande

| Rep. | Désignation | Qté | Remarque |
|---|---|---|---|
| S1 | Interrupteur à bascule 12 V, feux de détresse | 1 | Le comodo n'en fournit pas |
| BZ1 | Buzzer 12 V actif, < 100 mA | 1 | Retour sonore clignotants (rôle par défaut de OUT8) |
| S2 | Micro-rupteur pour levier de frein arrière | 1 | Variante B seulement |

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
