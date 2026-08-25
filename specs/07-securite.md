# 07 — Sécurité et modes dégradés

## 1. Analyse des défaillances

Pour chaque panne plausible : ce qui se passe, et pourquoi c'est acceptable.

| Panne | Conséquence directe | Fonction de sécurité perdue ? | Atténuation |
|---|---|---|---|
| **Arduino planté (boucle infinie)** | Sorties figées dans leur dernier état | Non | WDT 1 s → reset → état sûr en < 1,5 s. La coupure moteur reste assurée par le câblage direct |
| **Arduino non alimenté** | Toutes sorties inactives : plus de feux, plus de clignotants | Éclairage oui ; **coupure moteur non** | Les deux freins coupent nativement (contacteur d'origine à l'arrière, diode D1 à l'avant). Panne très visible : plus aucun feu. Fusible F11 dédié |
| **Reset intempestif en roulant** | Autotest 2 s pendant lequel les feux clignotent | Éclairage 2 s | L'autotest peut être désactivé (`SELFTEST_ENABLE 0`) une fois le véhicule validé |
| **Fil de contacteur de frein coupé (contact sec direct)** | L'entrée reste inactive → pas de feu stop, et au frein avant, plus de coupure moteur du tout | **Oui, le feu stop** | Détectable par `FLT_BRAKE_NEVER` (aucun freinage vu depuis 2 km). Contrôle avant départ T3.1 |
| **Fil de frein coupé (interface transistor optionnelle)** | L'entrée retombe → le firmware conclut « freinage » | Non | État sûr : stop allumé, coupure moteur active. Le défaut est visible immédiatement |
| **Ligne frein Bafang raccordée à une borne d'entrée SANS diode** | +12 V injecté dans une entrée 5 V du contrôleur | — | **Destruction probable du contrôleur.** C'est exactement ce que D1 empêche à l'avant ; à l'arrière, le connecteur jaune reste intact et S2 est un contact sec séparé (`03` §5) |
| **D1 en court-circuit** | ~1,4 mA de +12 V remontent vers la ligne frein au repos | — | Assistance possiblement inhibée en permanence. Gênant, pas dangereux ; le vélo reste utilisable sans assistance |
| **Contacteur de frein collé** | Stop allumé en permanence, assistance coupée | Non (côté sûr) | `FLT_BRAKE_STUCK` après 120 s, journal série |
| **Bouton d'acquittement collé** | Les défauts mémorisés sont effacés en boucle, les défauts actifs restent acquittés : **le voyant ne s'allume plus** | Le voyant, oui | Non détectable par le firmware. Reste visible sur la page « défauts » et au journal. Contrôle avant départ T3.1 : le voyant doit s'allumer pendant l'autotest |
| **LED du voyant grillée** | Plus aucune alerte en roulant | Le voyant, oui | Détecté à chaque mise sous tension : le voyant s'allume 200 ms pendant l'autotest. C'est la raison d'être de ce balayage |
| **Comodo : gauche et droite simultanés** | Deux directions contradictoires | Signalisation | Retombée sur `OFF` + `FLT_TURN_CONFLICT` |
| **Bus Bafang muet** | Vitesse invalide | Non (P2) | Rappel clignotant bascule sur le critère temps seul |
| **Bus Bafang bruité (trames fausses)** | Trames rejetées par la somme de contrôle | Non | Compteur `framesRejected` dans le journal |
| **Convertisseur 48/12 en panne** | Perte totale du 12 V | Éclairage seul — le klaxon est autonome | Les deux freins coupent toujours l'assistance : les chemins passifs ne dépendent pas du 12 V. Le moteur reste alimenté en 48 V → circulation possible jusqu'à l'arrêt, mais **de nuit, c'est un arrêt immédiat** |
| **Convertisseur 48/12 claqué en court-circuit** | Le 48 V arrive sur le réseau 12 V : feux, comodo, carte | Toutes | Convertisseur non isolé donné pour 60 V, pack à 58,4 V : **marge de 2,7 %**. Abaisser la charge à 3,50 V/cellule (`04` §2.1). Fusible F5 |
| **Contact de relais collé (soudé)** | Charge allumée en permanence, ou assistance coupée en permanence pour R8 | Non (côté sûr pour les feux) | Non détectable par le firmware : le registre ne relit rien. Détecté au contrôle avant départ (T3.1) |
| **Bobine de relais coupée** | Charge morte, sans aucun symptôme | **Oui pour le feu stop (R6)** | Contrôle avant départ obligatoire, observateur derrière |
| **Broche OE (A2) coupée ou flottante** | Relais indéterminés au démarrage | Potentiellement toutes | Le firmware écrit HIGH avant de passer la broche en sortie ; la carte porte normalement un tirage. A2 mesurée comme la validation, active à l'état bas (`03` §4) |
| **Chaîne de registres muette** (fil data/horloge/verrou) | Les relais gardent leur dernier état latché, indéfiniment | Toutes | Le chien de garde ne le voit pas : la boucle tourne normalement. **Angle mort assumé**, cf. §5 |

## 2. Les barrières de coupure d'assistance

> **Cette section a changé de conclusion deux fois.** Le contacteur de frein
> avant s'est révélé **unipolaire** — un seul jeu de contacts, incapable a
> priori d'informer la carte *et* de fermer la ligne frein — ce qui faisait
> dépendre la coupure au frein avant du firmware. **Une diode de découplage
> lève cette limite** en laissant un seul contact servir les deux circuits.
> P1 est de nouveau respecté aux deux freins.

### Frein arrière — deux chemins, dont un indépendant

1. **Contacteur Bafang d'origine → connecteur frein du contrôleur.** Le chemin
   nominal du kit, laissé strictement intact. Ne dépend d'aucun composant du
   projet.
2. **`OUT_MOTOR_CUT` (R8) → même connecteur, en contact sec.** Le chemin
   logiciel, redondant, informé par le micro-rupteur S2.

### Frein avant — deux chemins, dont un indépendant

1. **Contacteur avant → diode D1 → ligne frein du contrôleur.** Chemin
   purement passif : un contact et une diode. Aucun composant actif, aucune
   alimentation, aucun firmware.
2. **`OUT_MOTOR_CUT` (R8)**, informé par la même entrée `IN4`.

Le mécanisme tient à ce que les deux circuits demandent la même chose — une
mise à la **masse** — et à ce que la diode empêche le +12 V de la carte
d'atteindre la ligne 5 V du contrôleur quand le contact est ouvert. Le montage
complet est en `hardware/cablage.md` §4.

### Pourquoi c'est robuste

| Panne | Effet |
|---|---|
| Arduino planté, non alimenté ou mal programmé | Les deux freins coupent quand même. **P1 tenu** |
| **D1 en circuit ouvert** (le mode de défaillance courant d'une diode) | Dégradation **vers l'état antérieur** : le frein avant ne coupe plus que par R8, donc par le firmware. Pas de danger nouveau, et détecté au test T2.4 |
| **D1 en court-circuit** (rare) | Le +12 V de la carte, limité à ~1,4 mA par la résistance série de l'optocoupleur, remonte vers la ligne frein au repos. Le contrôleur peut le lire comme un freinage permanent : **assistance morte, vélo utilisable sans assistance**. Gênant, pas dangereux |
| D1 montée à l'envers | Le frein avant ne coupe pas. Sans gravité, détecté au test T2.4 |
| Fil de contacteur avant coupé | Ni feu stop ni coupure au frein avant. Détecté par `FLT_BRAKE_NEVER` et au contrôle avant départ |

Le point important est que **le mode de défaillance probable de D1 est
l'ouverture**, et qu'il ramène simplement à la situation d'avant, celle qui
avait déjà été jugée acceptable. La diode ne peut donc qu'améliorer les choses.

### Ce qui reste conditionné à une mesure

Tout ceci suppose la ligne frein Bafang **active à l'état bas** — le cas
courant. Si la mesure au multimètre montre l'inverse (`hardware/cablage.md` §4),
D1 est inopérante et doit être retirée : le frein avant redevient alors
dépendant du firmware, et c'est l'analyse de la version précédente qui
s'applique. **Mesurer avant de souder.**

### Ce qui reste vrai dans tous les cas

`OUT_MOTOR_CUT` est **relâché au reset** (principe P3). Un firmware qui
redémarre ne doit pas immobiliser le véhicule : trois secondes de moteur
pendant un reboot sont moins dangereuses qu'une assistance bloquée à l'arrêt
au milieu d'un carrefour, et les chemins passifs répondent de toute façon.

## 3. État sûr

Défini comme l'état des sorties à la mise sous tension et après reset :

| Sortie | État au reset | Justification |
|---|---|---|
| `OUT_PARK_FRONT` / `OUT_MAIN` | inactif | Rétabli en < 10 ms dès le premier balayage des entrées |
| `OUT_TAIL_PARK` / `OUT_TAIL_STOP` | inactif | idem |
| `OUT_TURN_*` | inactif | idem |
| `OUT_FAULT` (R7) | inactif | Voyant éteint au reset, puis allumé 200 ms par l'autotest — ce qui prouve que la LED fonctionne |
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
2 s de plus.

## 4. Chien de garde

- Désarmé explicitement au tout début de `setup()` :
  `MCUSR = 0; wdt_disable();`. Sans cela, après un reset par WDT, l'ancien
  bootloader Optiboot peut repartir en boucle de reset — le WDT reste armé avec
  un délai trop court pour laisser le bootloader finir.
- Le drapeau `WDRF` de `MCUSR` est lu **avant** d'être effacé et publié dans
  `FLT_WDT_RESET`. Un reset par chien de garde en roulage est une anomalie qui
  doit être visible.
- Armé à **1 s**, après l'autotest (qui dure 2 s et déclencherait le WDT).
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
- **Un seul bit d'information en roulant.** Le voyant de R7 dit qu'un défaut
  est actif, pas lequel. Le détail se lit à l'arrêt, coque ouverte, sur
  l'afficheur ou la console série. Et il ne couvre délibérément **pas** la
  perte du bus Bafang (P2, `02-machines-a-etats.md` §2.4).
- **Le voyant ne surveille pas les relais eux-mêmes.** Il rapporte ce que le
  firmware *croit*, pas ce que le matériel *fait* — le registre à décalage
  n'est pas relisible. Voyant éteint ne veut pas dire feux fonctionnels.
  Le contrôle avant départ T3.1 reste la seule vérification réelle.

## 6. Conformité réglementaire

Points à vérifier au regard du code de la route français pour un cycle :

| Point | Choix retenu |
|---|---|
| Feu de position arrière rouge non clignotant | Respecté : relais en tout-ou-rien, aucune modulation |
| Feu stop non clignotant | Respecté : `BRAKE_FLASH_ENABLE` à **0** par défaut |
| Cadence des clignotants 60–120 c/min | Respecté : 80 c/min (1,33 Hz), **y compris pendant le rappel d'oubli** — celui-ci modifie le rapport cyclique (375 → 200 ms allumé), jamais la période. C'est précisément pourquoi il a été conçu ainsi plutôt qu'en accélérant la cadence |
| Feux de détresse en phase | Respecté |
| Klaxon | Autonome, hors du périmètre de ce calculateur. Rappel tout de même : un avertisseur de type automobile sur un cycle relève d'une vérification locale — le Vhélio peut être homologué en cycle ou en cyclomoteur selon la version |

> Les feux de position et le feu stop sont sur des relais, donc en tout-ou-rien
> franc : aucune modulation, aucun scintillement, aucune question de
> conformité de ce côté. La différenciation entre position et stop est
> entièrement **matérielle** — le feu stop doit être nettement plus lumineux.
> C'est le point à valider au test T3.3, et il ne dépend plus du firmware.
