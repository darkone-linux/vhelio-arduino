# 07 — Sécurité et modes dégradés

## 1. Analyse des défaillances

Pour chaque panne plausible : ce qui se passe, et pourquoi c'est acceptable.

| Panne | Conséquence directe | Fonction de sécurité perdue ? | Atténuation |
|---|---|---|---|
| **Arduino planté (boucle infinie)** | Sorties figées dans leur dernier état | Non | WDT 1 s → reset → état sûr en < 1,5 s. La coupure moteur reste assurée par le câblage direct |
| **Arduino non alimenté** | Toutes sorties inactives : plus de feux, plus de clignotants | Éclairage oui ; coupure moteur **oui au frein avant**, non à l'arrière | Le frein arrière coupe nativement. Le frein avant ne coupe plus — voir §2. Panne très visible (plus aucun feu). Fusible F11 dédié |
| **Reset intempestif en roulant** | Autotest 1,2 s pendant lequel les feux clignotent | Éclairage 1,2 s | L'autotest peut être désactivé (`SELFTEST_ENABLE 0`) une fois le véhicule validé |
| **Fil de contacteur de frein coupé (contact sec direct)** | L'entrée reste inactive → pas de feu stop, et au frein avant, plus de coupure moteur du tout | **Oui, le feu stop** | Détectable par `FLT_BRAKE_NEVER` (aucun freinage vu depuis 2 km). Contrôle avant départ T3.1 |
| **Fil de frein coupé (interface transistor optionnelle)** | L'entrée retombe → le firmware conclut « freinage » | Non | État sûr : stop allumé, coupure moteur active. Le défaut est visible immédiatement |
| **Ligne frein Bafang raccordée par erreur à une borne d'entrée** | +12 V injecté dans une entrée 5 V du contrôleur | — | **Destruction probable du contrôleur.** Interdit par construction : le connecteur jaune reste intact, S2 est un contact sec séparé (`03` §5) |
| **Contacteur de frein collé** | Stop allumé en permanence, assistance coupée | Non (côté sûr) | `FLT_BRAKE_STUCK` après 120 s, journal série |
| **Bouton klaxon collé** | Klaxon continu | Non | `LOCKED` après 10 s, `FLT_HORN_STUCK` |
| **Comodo : gauche et droite simultanés** | Deux directions contradictoires | Signalisation | Retombée sur `OFF` + `FLT_TURN_CONFLICT` |
| **Bus Bafang muet** | Vitesse invalide | Non (P2) | Rappel clignotant bascule sur le critère temps seul |
| **Bus Bafang bruité (trames fausses)** | Trames rejetées par la somme de contrôle | Non | Compteur `framesRejected` dans le journal |
| **Convertisseur 48/12 en panne** | Perte totale du 12 V | Éclairage, klaxon, coupure moteur au frein avant | Le moteur continue de fonctionner (alimenté en 48 V direct) → circulation possible jusqu'à l'arrêt, mais **de nuit, c'est un arrêt immédiat** |
| **Convertisseur 48/12 claqué en court-circuit** | Le 48 V arrive sur le réseau 12 V : feux, comodo, carte | Toutes | Convertisseur non isolé donné pour 60 V, pack à 58,4 V : **marge de 2,7 %**. Abaisser la charge à 3,50 V/cellule (`04` §2.1). Fusible F5 |
| **Coup de klaxon en éclairage de nuit** | Appel de 5–7,5 A sur un convertisseur de 10 A → limitation → **les phares clignent** | Éclairage, brièvement | Condensateur tampon 10 000 µF sur le bus 12 V (`04` §2.3) |
| **Contact de relais collé (soudé)** | Charge allumée en permanence, ou assistance coupée en permanence pour R8 | Non (côté sûr pour les feux) | Non détectable par le firmware : le registre ne relit rien. Détecté au contrôle avant départ (T3.1) |
| **Bobine de relais coupée** | Charge morte, sans aucun symptôme | **Oui pour le feu stop (R6)** | Contrôle avant départ obligatoire, observateur derrière |
| **Broche OE (A1) coupée ou flottante** | Relais indéterminés au démarrage | Potentiellement toutes | Le firmware écrit HIGH avant de passer la broche en sortie ; la carte porte normalement un tirage. À vérifier au pinscan |
| **Chaîne de registres muette** (fil data/horloge/verrou) | Les relais gardent leur dernier état latché, indéfiniment | Toutes | Le chien de garde ne le voit pas : la boucle tourne normalement. **Angle mort assumé**, cf. §5 |

## 2. Les barrières de coupure d'assistance — **révisé**

> **Cette section a changé de conclusion.** Le contacteur de frein avant s'est
> avéré être un contact **unipolaire** : un seul jeu de contacts, qui ne peut
> pas à la fois informer l'Arduino et fermer la ligne frein du contrôleur. La
> couverture n'est plus la même à l'avant et à l'arrière.

### Frein arrière — deux barrières, dont une indépendante

1. **Contacteur Bafang d'origine → connecteur frein du contrôleur.** Le chemin
   nominal du kit, laissé strictement intact. Ne dépend d'aucun composant du
   projet.
2. **`OUT_MOTOR_CUT` (R8) → même connecteur, en contact sec.** Le chemin
   logiciel, redondant, informé par le micro-rupteur S2.

Le principe P1 est pleinement respecté : un Arduino planté, non alimenté ou
mal programmé n'empêche pas le frein arrière de couper l'assistance.

### Frein avant — une seule barrière, et elle passe par le firmware

Le contacteur avant est câblé sur `IN4`. La coupure d'assistance qui en découle
passe **uniquement par R8**, donc par l'Arduino.

**Conséquence à énoncer clairement : si l'Arduino est en panne ou non
alimenté, freiner du seul levier avant ne coupe pas l'assistance.** Sur un
tricycle où l'avant est le frein principal, ce n'est pas anodin.

Ce qui rend la situation acceptable, sans la rendre satisfaisante :

- Les freins eux-mêmes sont **purement mécaniques** et ne dépendent de rien.
  On s'arrête, simplement en luttant contre le moteur.
- Le contrôleur Bafang coupe aussi l'assistance à l'**arrêt du pédalage** (PAS)
  et au **relâchement de l'accélérateur**. Un freinage réel s'accompagne
  presque toujours de l'un des deux.
- Le **levier arrière reste disponible** et coupe nativement.
- Le chien de garde ramène un Arduino planté en moins de 1,5 s.
- Une panne d'alimentation de l'Arduino éteint aussi tous les feux : elle ne
  passe pas inaperçue, elle est immédiatement visible de nuit et détectable au
  contrôle avant départ de jour.

**Remède, et il est trivial** : ajouter un second micro-rupteur sur le levier
avant, câblé en parallèle sur la ligne frein du contrôleur. Quelques euros, et
le frein avant retrouve exactement la même couverture que l'arrière. C'est la
seule évolution matérielle de ce projet qui touche à la sécurité, et elle
devrait être faite dès qu'un contacteur bipolaire ou un second micro-rupteur
est disponible.

### Ce qui reste vrai dans tous les cas

`OUT_MOTOR_CUT` est **relâché au reset** (principe P3). Un firmware qui
redémarre ne doit pas immobiliser le véhicule : trois secondes de moteur
pendant un reboot sont moins dangereuses qu'une assistance bloquée à l'arrêt
au milieu d'un carrefour, et le frein mécanique répond de toute façon.

## 3. État sûr

Défini comme l'état des sorties à la mise sous tension et après reset :

| Sortie | État au reset | Justification |
|---|---|---|
| `OUT_PARK_FRONT` / `OUT_MAIN` | inactif | Rétabli en < 10 ms dès le premier balayage des entrées |
| `OUT_TAIL_PARK` / `OUT_TAIL_STOP` | inactif | idem |
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
- **La coupure d'assistance au frein avant dépend du firmware.** Voir §2 : ce
  n'est pas une limite de conception mais une conséquence du contacteur
  unipolaire disponible, et elle se corrige avec un micro-rupteur à 2 €.
- **Aucun annonciateur en roulant.** Le boîtier est sous la coque et
  l'afficheur n'est pas lisible. Le seul canal vers le conducteur en marche est
  le **claquement des relais** — d'où le rappel d'oubli des clignotants par
  changement de rythme. Tout le reste du diagnostic se lit à l'arrêt, coque
  ouverte, sur l'afficheur ou la console série.

## 6. Conformité réglementaire

Points à vérifier au regard du code de la route français pour un cycle :

| Point | Choix retenu |
|---|---|
| Feu de position arrière rouge non clignotant | Respecté : relais en tout-ou-rien, aucune modulation |
| Feu stop non clignotant | Respecté : `BRAKE_FLASH_ENABLE` à **0** par défaut |
| Cadence des clignotants 60–120 c/min | Respecté : 80 c/min (1,33 Hz), **y compris pendant le rappel d'oubli** — celui-ci modifie le rapport cyclique (375 → 200 ms allumé), jamais la période. C'est précisément pourquoi il a été conçu ainsi plutôt qu'en accélérant la cadence |
| Feux de détresse en phase | Respecté |
| Klaxon | Un avertisseur sonore de type automobile sur un cycle relève d'une vérification locale — le VHélio peut être homologué en tant que cycle ou cyclomoteur selon la version |

> Les feux de position et le feu stop sont sur des relais, donc en tout-ou-rien
> franc : aucune modulation, aucun scintillement, aucune question de
> conformité de ce côté. La différenciation entre position et stop est
> entièrement **matérielle** — le feu stop doit être nettement plus lumineux.
> C'est le point à valider au test T3.3, et il ne dépend plus du firmware.
