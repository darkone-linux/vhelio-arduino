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

| Charge | Puissance | Courant 12 V | Régime |
|---|---|---|---|
| Feu de croisement LED | 20 W | 1,7 A | continu de nuit |
| Feu de route LED | 20 W | 1,7 A | intermittent |
| Feux rouges arrière (2 × 3 W) | 6 W | 0,5 A | continu de nuit |
| Clignotants (4 × 3 W) | 12 W | 1,0 A crête / 0,5 A moyen | intermittent |
| Klaxon 12 V | 60–90 W | 5–7,5 A | crête, < 5 s |
| Prise allume-cigare n°1 | 120 W | 10 A | selon usage |
| Prise allume-cigare n°2 | 120 W | 10 A | selon usage |
| Arduino + DN22D08 + relais | ~2,5 W | 0,2 A | continu |
| **Total crête théorique** | | **≈ 32 A** | |
| **Total réaliste de nuit + charge téléphone** | | **≈ 6 A** | |

### Dimensionnement du convertisseur 48 → 12 V

Deux options cohérentes, à choisir :

| Option | Convertisseur | Contrainte |
|---|---|---|
| **A** — confortable | 12 V / **30 A** (360 W) | Aucune : les deux prises peuvent tirer 10 A chacune |
| **B** — économique *(recommandée)* | 12 V / **20 A** (240 W) | Une prise 10 A + une prise USB 3 A ; fusible général 20 A |

Côté 48 V, un convertisseur de 360 W consomme 360 / 51,2 / 0,9 ≈ **7,8 A**.
Prévoir un convertisseur **non isolé (buck)** : la masse 12 V est alors commune
à la masse 48 V, ce qui est nécessaire pour l'écoute UART (§5).

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
| F2 | Pack → convertisseur 48/12 V | 48 V | 10 A | MIDI 58 V DC ou 10×38 gPV |
| F3 | MPPT → pack | 48 V | selon panneau (typ. 15 A) | 10×38 gPV |
| F4 | Panneau → MPPT | 48 V+ | selon panneau | 10×38 gPV |
| F5 | Sortie générale 12 V | 12 V | 20 ou 30 A | MIDI / lame maxi |
| F6 | Éclairage avant (croisement + route) | 12 V | 5 A | lame |
| F7 | Éclairage arrière + clignotants | 12 V | 5 A | lame |
| F8 | Klaxon (bobine relais + contact) | 12 V | 10 A | lame |
| F9 | Prise allume-cigare n°1 | 12 V | 10 A | lame |
| F10 | Prise allume-cigare n°2 | 12 V | 10 A | lame |
| F11 | Arduino + carte DN22D08 + comodo | 12 V | 2 A | lame |

Ajouter un **sectionneur / coupe-circuit 48 V DC** (≥ 63 V DC, ≥ 40 A) entre le
pack et tout le reste — pour la maintenance et en cas d'incident. Un
interrupteur automobile classique n'est **pas** qualifié pour couper 48 V DC.

## 4. Sections de câbles

Dimensionnement thermique et chute de tension < 3 %, cuivre souple.

| Liaison | Courant | Longueur aller-retour | Section |
|---|---|---|---|
| Pack ↔ contrôleur moteur | 25 A crête | 3 m | **6 mm²** |
| Pack ↔ convertisseur 48/12 | 8 A | 3 m | **2,5 mm²** |
| MPPT ↔ pack | 15 A | 2 m | **4 mm²** |
| Convertisseur → boîtier fusibles 12 V | 30 A | 1,5 m | **6 mm²** |
| Départ éclairage 12 V | 5 A | 6 m | **1,5 mm²** |
| Départ klaxon (contact relais) | 8 A | 4 m | **2,5 mm²** |
| Départ allume-cigare | 10 A | 3 m | **2,5 mm²** |
| Signaux comodo / freins vers DN22D08 | < 0,05 A | 4 m | **0,75 mm²** |

## 5. Masses et référence de potentiel

L'écoute de la liaison UART Bafang impose que l'Arduino **partage la masse du
contrôleur moteur**. Trois cas :

1. **Convertisseur non isolé (buck) — cas nominal.** La masse 12 V est déjà la
   masse 48 V, donc la masse du contrôleur. Rien à faire : relier simplement la
   masse du connecteur UART à la masse de la carte DN22D08.
2. **Convertisseur isolé.** Réaliser un **unique point de liaison** (« masse en
   étoile ») entre le négatif 12 V et le négatif 48 V, au plus près du
   convertisseur. Un seul point, jamais deux, pour éviter les boucles de masse.
3. **Isolation à conserver absolument.** Insérer un optocoupleur (PC817 +
   résistance de tirage 10 kΩ côté Arduino) sur la ligne sniffée. À 1200 bauds,
   un PC817 est largement assez rapide.

La sortie `OUT_MOTOR_CUT` traverse **toujours** un optocoupleur, quelle que soit
la situation de masse ci-dessus : le circuit frein du contrôleur ne doit voir
aucune tension étrangère.

## 6. Le second BMS Daly 60 A

Le Daly 60 A est décrit comme « de secours ». Deux lectures possibles, aux
conséquences très différentes :

- **BMS de rechange, non câblé.** C'est le cas simple et sans risque : le Daly
  reste au garage et remplace le JK en cas de panne. Recommandé.
- **Second BMS câblé en série sur le même pack.** Techniquement possible (les
  MOSFET des deux BMS sont en série sur le chemin de courant, chaque BMS ayant
  son propre faisceau d'équilibrage), mais cela demande de la rigueur : les
  faisceaux de mesure des deux BMS lisent les mêmes cellules, les seuils
  doivent être décalés (le Daly plus permissif que le JK, pour qu'il n'agisse
  qu'en secours), et le courant est plafonné par le plus petit des deux, soit
  **60 A** — pas 100 A.

Ce point est laissé ouvert (`09-questions-ouvertes.md`). Quoi qu'il en soit,
l'Arduino n'interagit avec **aucun** des deux BMS.

## 7. Évolution : lecture du MPPT Victron

Le SmartSolar expose un port **VE.Direct** : sortie texte ASCII, 19 200 bauds,
une trame toutes les secondes (tension pack, courant de charge, puissance PV,
état). Il serait tentant de la lire pour afficher la production solaire.

Ce n'est **pas** possible dans le brochage actuel : il n'y a plus d'UART libre,
et un second port logiciel à 19 200 bauds bloquerait les interruptions trop
longtemps. Deux voies si le besoin se confirme :

- désactiver l'écoute Bafang (`BAFANG_ENABLE 0`) et réaffecter D10 au VE.Direct ;
- passer sur un Arduino Mega 2560 (4 UART matériels), qui accepte la même carte
  d'E/S via un adaptateur de brochage.
