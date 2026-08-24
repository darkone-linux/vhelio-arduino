# 07 — Sécurité et modes dégradés

## 1. Analyse des défaillances

Pour chaque panne plausible : ce qui se passe, et pourquoi c'est acceptable.

| Panne | Conséquence directe | Fonction de sécurité perdue ? | Atténuation |
|---|---|---|---|
| **Arduino planté (boucle infinie)** | Sorties figées dans leur dernier état | Non | WDT 1 s → reset → état sûr en < 1,5 s. La coupure moteur reste assurée par le câblage direct |
| **Arduino non alimenté** | Toutes sorties inactives : plus de feux, plus de clignotants | Éclairage oui, coupure moteur non | Câblage frein direct (P1). Fusible F11 dédié pour isoler la cause |
| **Reset intempestif en roulant** | Autotest 1,2 s pendant lequel les feux clignotent | Éclairage 1,2 s | L'autotest peut être désactivé (`SELFTEST_ENABLE 0`) une fois le véhicule validé |
| **Fil de frein coupé (variante A, interface transistor)** | L'entrée retombe → le firmware conclut « freinage » | Non | État sûr : stop allumé, coupure moteur active. Le défaut est visible immédiatement |
| **Fil de frein coupé (variante B, contact 12 V direct)** | L'entrée reste inactive → pas de feu stop | **Oui, le feu stop** | Détectable par `FLT_BRAKE_NEVER` (aucun freinage vu depuis 30 min de roulage). Contrôle avant départ |
| **Contacteur de frein collé** | Stop allumé en permanence, assistance coupée | Non (côté sûr) | `FLT_BRAKE_STUCK` après 120 s, journal série |
| **Bouton klaxon collé** | Klaxon continu | Non | `LOCKED` après 10 s, `FLT_HORN_STUCK` |
| **Comodo : gauche et droite simultanés** | Deux directions contradictoires | Signalisation | Retombée sur `OFF` + `FLT_TURN_CONFLICT` |
| **Bus Bafang muet** | Vitesse invalide | Non (P2) | Rappel clignotant bascule sur le critère temps seul |
| **Bus Bafang bruité (trames fausses)** | Trames rejetées par la somme de contrôle | Non | Compteur `framesRejected` dans le journal |
| **Convertisseur 48/12 en panne** | Perte totale du 12 V | Éclairage, klaxon | Le moteur continue de fonctionner (alimenté en 48 V direct) → circulation possible jusqu'à l'arrêt, mais **de nuit, c'est un arrêt immédiat** |
| **Contact de relais collé (soudé)** | Charge allumée en permanence, ou assistance coupée en permanence pour R8 | Non (côté sûr pour les feux) | Non détectable par le firmware : le registre ne relit rien. Détecté au contrôle avant départ (T3.1) |
| **Bobine de relais coupée** | Charge morte, sans aucun symptôme | **Oui pour le feu stop (R6)** | Contrôle avant départ obligatoire, observateur derrière |
| **Broche OE (A1) coupée ou flottante** | Relais indéterminés au démarrage | Potentiellement toutes | Le firmware écrit HIGH avant de passer la broche en sortie ; la carte porte normalement un tirage. À vérifier au pinscan |
| **Chaîne de registres muette** (fil data/horloge/verrou) | Les relais gardent leur dernier état latché, indéfiniment | Toutes | Le chien de garde ne le voit pas : la boucle tourne normalement. **Angle mort assumé**, cf. §5 |

## 2. Les trois barrières indépendantes

La coupure de l'assistance au freinage repose sur trois chemins, dont **deux ne
passent pas par l'Arduino** :

1. **Contacteur de frein arrière d'origine → connecteur frein Bafang.** C'est le
   chemin nominal, celui du kit moteur, inchangé.
2. **Contacteur de frein avant → même connecteur frein, en parallèle.** Ajout
   purement électrique, aucun composant actif.
3. **`OUT_MOTOR_CUT` → optocoupleur → même connecteur frein.** Le chemin
   logiciel, redondant.

Le chemin 3 est celui qui peut tomber ; les chemins 1 et 2 sont de simples
contacts secs. **C'est ce qui justifie que la sortie soit relâchée au reset**
(principe P3) : un firmware qui redémarre ne doit pas immobiliser le véhicule,
puisque le freinage reste couvert par ailleurs.

## 3. État sûr

Défini comme l'état des sorties à la mise sous tension et après reset :

| Sortie | État au reset | Justification |
|---|---|---|
| `OUT_LOWBEAM` / `OUT_HIGHBEAM` | inactif | Rétabli en < 10 ms dès le premier balayage des entrées |
| `OUT_TAIL` | inactif | idem |
| `OUT_TURN_*` | inactif | idem |
| `OUT_HORN` | inactif | Un klaxon qui sonne au reset serait dangereux |
| `OUT_MOTOR_CUT` | **relâché** | Voir §2 |

L'état sûr est obtenu **matériellement** avant même que le firmware ne
s'exécute : la broche OE est portée à l'état haut dès le début de `setup()`,
ce qui maintient les huit relais relâchés quel que soit le contenu résiduel du
registre à décalage. C'est plus fort qu'une simple initialisation logicielle.

`outputsEnabled(false)` réactive ce mécanisme à tout moment : **les huit relais
retombent en un cycle d'horloge**, sans altérer le registre, et l'état
antérieur est restitué intact à la réactivation. C'est un arrêt d'urgence
matériel disponible pour un futur mode sécurité.

Le premier balayage complet des entrées a lieu au premier tour de `loop()`,
soit **moins de 10 ms** après la fin de `setup()`. L'éclairage est donc rétabli
imperceptiblement — sauf si l'autotest est actif, auquel cas il faut compter
1,2 s de plus.

## 4. Chien de garde

- Désarmé explicitement au tout début de `setup()` :
  `MCUSR = 0; wdt_disable();`. Sans cela, après un reset par WDT, l'ancien
  bootloader Optiboot peut repartir en boucle de reset — le WDT reste armé avec
  un délai trop court pour laisser le bootloader finir.
- Le drapeau `WDRF` de `MCUSR` est lu **avant** d'être effacé et publié dans
  `FLT_WDT_RESET`. Un reset par chien de garde en roulage est une anomalie qui
  doit être visible.
- Armé à **1 s**, après l'autotest (qui dure 1,2 s et déclencherait le WDT).
- `wdt_reset()` est appelé **une seule fois**, en fin de `loop()`. Jamais dans
  une boucle interne : cela masquerait précisément le blocage qu'on cherche à
  détecter.

## 5. Ce que le système ne protège pas

À dire explicitement, pour ne pas donner une fausse confiance :

- **Aucune surveillance de la batterie.** Sous-tension, sur-tension,
  température, déséquilibre : c'est le rôle exclusif du BMS JK. L'Arduino ne
  voit rien du pack et ne peut rien couper.
- **Aucune surveillance thermique** du moteur, du contrôleur ou du convertisseur.
- **Aucune mesure de courant.** La DN22D08 n'a pas d'entrée analogique libre ni
  de shunt. Une surcharge n'est vue que par les fusibles.
- **Aucune détection de rupture de lampe.** Un feu grillé n'est pas signalé.
  Ce serait techniquement possible (mesure de courant par sortie) mais hors du
  matériel retenu.
- **Aucune relecture de l'état réel des relais.** Le registre à décalage est
  un composant en écriture seule : le firmware sait ce qu'il a *demandé*, pas
  ce qui s'est réellement produit. Un contact soudé, une bobine coupée ou une
  chaîne de registres muette ne sont **pas** détectables. C'est l'angle mort
  principal de ce montage, et la raison pour laquelle le contrôle avant départ
  du test T3.1 n'est pas une formalité.
- **Aucune fonction de freinage.** Le système allume un feu et coupe une
  assistance ; il ne freine pas.

## 6. Conformité réglementaire

Points à vérifier au regard du code de la route français pour un cycle :

| Point | Choix retenu |
|---|---|
| Feu de position arrière rouge non clignotant | Respecté : veilleuse PWM constante, pas de modulation visible |
| Feu stop non clignotant | Respecté : `BRAKE_FLASH_ENABLE` à **0** par défaut |
| Cadence des clignotants 60–120 c/min | Respecté : 80 c/min (1,33 Hz) |
| Feux de détresse en phase | Respecté |
| Klaxon | Un avertisseur sonore de type automobile sur un cycle relève d'une vérification locale — le VHélio peut être homologué en tant que cycle ou cyclomoteur selon la version |

> Les feux de position et le feu stop sont sur des relais, donc en tout-ou-rien
> franc : aucune modulation, aucun scintillement, aucune question de
> conformité de ce côté. La différenciation entre position et stop est
> entièrement **matérielle** — le feu stop doit être nettement plus lumineux.
> C'est le point à valider au test T3.3, et il ne dépend plus du firmware.
