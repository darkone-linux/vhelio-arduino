# 00 — Vue d'ensemble

## 1. Contexte

Le VHélio est un vélomobile solaire à assistance électrique. Le guide de montage
officiel décrit un câblage « en dur » : chaque interrupteur alimente directement
sa charge. Ce projet remplace ce câblage par un **calculateur central** qui
centralise la logique (clignotants, feux stop, temporisations, cohérence des
commandes) et ouvre la porte à la télémétrie et au diagnostic.

## 2. Périmètre

### Dans le périmètre

- Éclairage : feu de croisement, feu de route, feux rouges arrière
- Signalisation : clignotants gauche / droite, feux de détresse
- Klaxon 12 V
- Détection de freinage (avant + arrière) → feu stop + pause assistance
- Télémétrie moteur en **écoute passive** sur la liaison UART afficheur ↔ contrôleur Bafang
- Diagnostic : autotest au démarrage, journal série, chien de garde

### Hors périmètre

- Toute commande du moteur autre que la ligne « frein » du contrôleur.
  L'assistance, les niveaux PAS et le bridage restent gérés par l'afficheur Bafang.
- La gestion de la batterie : elle est assurée par le BMS JK. L'Arduino ne coupe
  ni ne surveille le pack.
- La charge solaire : le MPPT Victron est autonome.
- L'affichage conducteur : l'afficheur Bafang d'origine est conservé tel quel.

## 3. Principes directeurs

Ces quatre principes arbitrent toutes les décisions de conception qui suivent.

**P1 — Le firmware n'est jamais un point de défaillance unique pour la sécurité.**
La coupure moteur au freinage est assurée **matériellement** par le câblage des
contacteurs de frein sur la ligne frein du contrôleur Bafang. La sortie
`OUT_MOTOR_CUT` de l'Arduino est *redondante*, pas indispensable.

**P2 — La télémétrie est un confort, pas une fonction de sécurité.**
La perte totale du bus Bafang (fil coupé, afficheur débranché, décodage erroné)
n'a aucun effet sur les feux, les clignotants, le stop ou le klaxon. Le module
peut être désactivé à la compilation (`BAFANG_ENABLE 0`) sans rien casser d'autre.

**P3 — État sûr par défaut.**
Au reset, toutes les sorties sont inactives (feux éteints, klaxon coupé,
coupure moteur relâchée). La coupure moteur relâchée au boot est le bon choix
*parce que* P1 garantit la coupure matérielle : un firmware planté ne doit pas
immobiliser le véhicule.

**P4 — Aucun `delay()`, aucune boucle bloquante.**
Le firmware est un ordonnanceur coopératif. Le temps de cycle mesuré doit rester
sous 10 ms pour que la réaction au freinage soit imperceptible.

## 4. Rôles des équipements

| Équipement | Rôle | Interface avec l'Arduino |
|---|---|---|
| **Pack LiFePO4 16S** | Stockage 48 V (51,2 V nom.) | Aucune |
| **BMS JK PB1A16S10P 100 A** | Protection cellules, équilibrage, coupure | Aucune (Bluetooth propre) |
| **MPPT Victron SmartSolar** | Charge solaire du pack 48 V | Aucune en v1 (VE.Direct en évolution) |
| **Convertisseur 48 → 12 V** | Alimente tout le réseau accessoire | Alimente la carte DN22D08 |
| **Contrôleur Bafang 750 W** | Maître de la traction | Ligne frein (entrée), ligne UART TX (sortie, sniffée) |
| **Afficheur Bafang UART** | IHM conducteur, réglage assistance | Aucune (l'Arduino n'écrit jamais sur le bus) |
| **Comodo** | Commandes conducteur | 5 entrées 12 V |
| **Contacteurs de frein** | Sécurité freinage | 1–2 entrées + ligne frein contrôleur |
| **Arduino Nano + DN22D08** | Logique éclairage / signalisation / diagnostic | — |
| **Boîtier fusibles** | Protection de chaque départ 12 V | Aucune |

## 5. Architecture générale

```mermaid
flowchart LR
  PV[Panneau solaire] --> MPPT[MPPT Victron<br/>SmartSolar 48V]
  MPPT --> PACK[Pack LiFePO4 16S<br/>+ BMS JK 100A]
  PACK -->|48 V| CTRL[Contrôleur Bafang 750 W]
  PACK -->|48 V| CONV[Convertisseur 48→12 V]
  CTRL --> MOT[Moteur Bafang]
  CTRL <-->|UART 1200 bd| DISP[Afficheur Bafang]
  CTRL -.->|TX sniffé| NANO
  CONV -->|12 V| FUSE[Boîtier fusibles]
  FUSE --> NANO[Arduino Nano<br/>+ DN22D08]
  FUSE --> LOADS[Phares · Feux AR · Clignotants<br/>Klaxon · Allume-cigare]
  COMODO[Comodo] --> NANO
  BRK[Contacteurs frein AV/AR] --> NANO
  BRK ==>|câblage direct<br/>failsafe| CTRL
  NANO --> LOADS
  NANO -.->|coupure redondante| CTRL
```

Le trait épais `BRK ==> CTRL` matérialise le principe P1 : le chemin de sécurité
ne passe pas par l'Arduino.

## 6. Glossaire

| Terme | Définition |
|---|---|
| **Comodo** | Bloc de commandes au guidon (clignotants, klaxon, éclairage) |
| **Ligne frein** | Entrée du contrôleur Bafang qui coupe l'assistance quand elle est fermée à la masse |
| **PAS** | Pedal Assist System — niveau d'assistance sélectionné à l'afficheur |
| **Veilleuse** | Feu de position arrière, allumé en permanence avec l'éclairage |
| **Stop** | Feu de freinage, plus lumineux que la veilleuse |
| **Sniff** | Écoute passive d'une liaison série, sans jamais émettre |
| **SOC** | State Of Charge — niveau de charge batterie en % |
| **PCINT** | Pin Change Interrupt de l'ATmega328P |
| **DN22D08** | Carte rail DIN 8 entrées optocouplées / 8 sorties, support Arduino Nano |
