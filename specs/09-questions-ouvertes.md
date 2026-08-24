# 09 — Questions ouvertes

Ce qui reste à trancher pour figer la conception. Classé par urgence.

**Douze questions ont été posées, dix sont tranchées.** Le détail de chaque
réponse et ses conséquences sont dans les documents concernés ; ce fichier
n'en garde que la conclusion et ce qui reste à faire.

---

## Encore ouvert

### Q1 — Confirmer le brochage au pinscan *(bloquant avant tout câblage)*

Le brochage retenu vient de la bibliothèque de référence de la famille
IO22/DN22, pas du datasheet de cet exemplaire précis. Trois points restent
à confirmer, et le troisième conditionne le sertissage du faisceau :

1. **L'ordre des relais.** `RELAY_BIT[]` place R8 sur le bit 0 — bizarrerie
   documentée mais non vérifiée ici.
2. **L'ordre des boutons.** La sérigraphie `K1..K4` serait imprimée à
   l'envers : le poussoir marqué `K4` serait celui relié à D7.
3. **La polarité des entrées.** NPN attendu — on active une borne en la
   fermant sur la **masse**. Si cet exemplaire était PNP, le firmware ne
   changerait pas d'une ligne, mais **tous les communs du faisceau iraient au
   +12 V au lieu de la masse**.

**Action** : dérouler `03-affectation-es.md` §6 avec `tools/pinscan`. Une
demi-heure, avant de sertir quoi que ce soit.

### Q9 — Les phares font-ils 60 W au total, ou 60 W chacun ?

« 2 phares 6 LED, 60 W » se lit des deux façons, et l'écart décide du
dimensionnement :

| Lecture | Courant | Conséquence |
|---|---|---|
| 60 W pour la paire | 5,0 A | Tout va bien : branchement direct sur R2 |
| 60 W par phare | 10,0 A | **Sature le convertisseur de 10 A à lui seul**, et R2 est à son calibre exact. Il faut un convertisseur 20 A et un relais automobile K2 |

Le projet retient l'hypothèse basse. Deux façons de trancher :

- lire l'étiquette ou la fiche produit ;
- **mesurer à la pince**, ce qui est plus sûr : les projecteurs LED chinois
  « 60 W » consomment très souvent 15 à 20 W réels. C'est le test T2.3.

### Q10 — Circonférence de roue

`WHEEL_CIRCUMFERENCE_MM` vaut 2200 mm en attendant la roue réelle. Ne sert
qu'à la formule de vitesse « période de roue » et à l'odomètre — **aucune
fonction de sécurité**. À corriger quand la roue sera montée.

---

## Tranché

### ~~Q2 — Nature des sorties~~ — **relais**

Contacts secs 10 A NO/NC pilotés par un registre à décalage 74HC595.
Conséquences intégrées partout : plus aucun étage de puissance externe, la
coupure moteur devient un contact sec isolé par construction, le buzzer
disparaît au profit du claquement des relais — et **la sortie modulée du feu
arrière devient matériellement impossible**, d'où deux relais et deux circuits
distincts (position et stop).

Reste la contrepartie : **R3 et R4 sont les pièces d'usure du montage**
(4 800 manœuvres par heure de clignotement). Une alternative matérielle plus
évolutive est esquissée en fin de ce document.

### ~~Q3 — Tension d'alimentation de la carte~~ — **12 V**

Conforme à toute la spécification. Rien à changer.

### ~~Q4 — Type des contacteurs de frein~~ — **avant unipolaire, arrière Higo 3 broches**

C'est la réponse qui a eu le plus de conséquences.

- **Avant** : contact sec unipolaire, ouvert au repos. Se câble directement sur
  `IN4` et la masse, sans interface — les entrées étant NPN. Mais **un contact
  unipolaire ne peut servir qu'une fois** : câblé sur IN4, il ne ferme pas la
  ligne frein du contrôleur. La coupure d'assistance au frein avant passe donc
  uniquement par R8, **donc par le firmware**. Régression réelle de P1,
  analysée en `07-securite.md` §2, corrigible par un micro-rupteur S4 à 2 €.
- **Arrière** : connecteur Higo rond jaune 3 broches, ligne logique ~5 V.
  **Il ne doit surtout pas être raccordé à une borne d'entrée** — celle-ci est
  tirée au +12 V à travers son optocoupleur et détruirait probablement le
  contrôleur. Le connecteur reste donc **intact**, et un micro-rupteur S2
  ajouté sur le levier informe le firmware.

### ~~Q5 — Second BMS Daly 60 A~~ — **rechange au garage**

Non câblé. Le pack conserve ses 100 A, aucun seuil à coordonner.

### ~~Q6 — Convertisseur 48 → 12 V~~ — **non isolé, 10 A, 60 V max**

Deux conséquences, dont une qui demande une action :

1. **Non isolé** : la masse 12 V est déjà la masse 48 V. Rien à faire pour
   l'écoute UART (`04-electricite.md` §5, cas 1). Bonne nouvelle.
2. **60 V maximum pour un pack qui monte à 58,4 V** : 2,7 % de marge, moins
   que l'ondulation d'un MPPT en fin d'absorption. **Régler la charge du BMS
   JK à 3,50 V/cellule (56,0 V pack)** — gratuit, sans perte de capacité
   utile, bénéfique pour le pack. Et prendre un modèle 72 ou 80 V au prochain
   achat : un buck non isolé qui claque met le 48 V sur les feux.
3. **10 A ne couvrent pas tout** : prises allume-cigare fusiblées à 5 A, et
   condensateur tampon de 10 000 µF sans lequel un coup de klaxon fait cligner
   les phares (`04-electricite.md` §2.3).

### ~~Q7 — Commandes d'éclairage et de détresse~~ — **trois interrupteurs dédiés**

Le montage réel n'est pas croisement/route mais **veilleuse / éclairage fort** :

| Entrée | Organe | Sortie |
|---|---|---|
| `IN_PARK` (IN6) | Interrupteur dédié S3 « veilleuse » | R1, veilleuse avant |
| `IN_MAIN` (IN7) | Interrupteur du comodo | R2, phares |
| `IN_HAZARD` (IN8) | Interrupteur dédié S1 | R3 + R4 en phase |

Deux règles en découlent :

- **Le feu de position arrière suit l'un OU l'autre niveau**, sans condition et
  sans option de compilation. Rouler éclairé à l'avant sans feu rouge arrière
  doit être impossible par construction.
- **Les phares ne sont pas conditionnés à la veilleuse** (`MAIN_REQUIRES_PARK`
  vaut 0) : un interrupteur de veilleuse resté ouvert ne doit jamais priver
  d'éclairage.

Pour les témoins, prendre des interrupteurs à bascule **lumineux** pour S1 et
S3 : la position du contacteur est le témoin, sans aucun câblage de voyant.

### ~~Q8 — Rôle de `OUT8`~~ — **sans objet**

Les huit relais sont exactement consommés par les huit fonctions.

### ~~Q11 — Le RS485 est-il câblé sur D0/D1 ?~~ — **oui, et l'inverseur le règle**

La carte porte bien un émetteur-récepteur RS485 sur D0/D1, mais l'**inverseur à
glissière `485_ON` / `PRO`** l'en déconnecte.

- **Le laisser sur `PRO` en permanence.** Console série et téléversement USB
  fonctionnent alors normalement, et `DEBUG_SERIAL 1` reste valide.
- **Aucun convertisseur USB-RS485 à acheter.** On programme le Nano par son
  propre port USB. Le RS485 ne servirait qu'à un futur bus entre plusieurs
  cartes.
- Si un téléversement échoue sans raison apparente, **vérifier cet inverseur
  avant toute autre hypothèse**.

### ~~Q12 — Où est monté le calculateur ?~~ — **sous la coque, afficheur illisible**

L'afficheur 4 digits devient un **outil de maintenance**, pas un tableau de
bord. Trois conséquences, toutes traitées :

1. `DISPLAY_DEFAULT_PAGE` passe à **2 (défauts)** : c'est ce qu'on veut voir en
   ouvrant la coque.
2. **Le seul canal vers le conducteur en marche est le claquement des relais.**
   Le rappel d'oubli des clignotants l'exploite : il raccourcit la phase
   allumée de 375 à 200 ms **sans toucher à la période**. La cadence reste à
   80 cycles/min, donc réglementaire, mais le rythme devient syncopé et
   reconnaissable. Aucun composant, aucune sortie.
   *(C'est pourquoi le rappel n'accélère pas la cadence : ce serait sortir de
   la plage 60–120 c/min.)*
3. **Le boîtier ne doit pas étouffer ce bruit** : le fixer sur un panneau
   rigide, ne pas le caler dans de la mousse.

Pour des témoins visuels, voir `hardware/cablage.md` §8 — ils sont purement
électriques et ne consomment ni relais ni broche.

---

## La question qui reste, à traiter dans un second temps

### Q13 — Faut-il changer de couple carte/microcontrôleur ?

Le matériel actuel fonctionne et le firmware est écrit. Mais la Q2 a laissé
trois limites structurelles :

- **aucune modulation** — pas de feu arrière à intensité variable, pas de
  gradation de phare, pas de feux de jour à mi-puissance ;
- **R3 et R4 s'usent mécaniquement** à chaque clignotement ;
- **tout est saturé** — 8 entrées sur 8, 8 relais sur 8, un seul UART, et
  l'afficheur est sous la coque.

L'analyse comparée et une recommandation sont en `10-alternatives-materiel.md`.
**Par défaut, on ne change rien** : la carte est achetée, le firmware compile
et vole. La question mérite d'être reprise seulement si l'une des trois limites
ci-dessus devient gênante à l'usage.

---

## Évolutions possibles, hors périmètre v1

| Idée | Coût | Obstacle |
|---|---|---|
| **Micro-rupteur S4 sur le levier avant** | **~2 €** | **Aucun — c'est l'évolution la plus utile du projet** (`07` §2) |
| Convertisseur 48/12 V donné pour 72 V | ~15 € | Aucun, au prochain achat |
| Lecture du MPPT en VE.Direct | Faible | Plus d'UART libre (`04-electricite.md` §7) |
| ~~Écran de bord dédié~~ | — | Sans objet : la carte a son afficheur… mais il est sous la coque |
| Journalisation sur carte SD | Élevé | Le SPI est inutilisable : D11/D12 sont des entrées, D13 la ligne de données du registre |
| Détection de rupture de lampe | Élevé | Nécessite une mesure de courant par voie |
| Arrêt d'urgence par la broche OE | Faible | Déjà câblé et implémenté ; il ne reste qu'à définir sa condition de déclenchement |
| Forçage du niveau d'assistance | Élevé | Impose l'interposition UART, écartée (`05` §8) |
| Feu stop proportionnel à la décélération | — | Impossible : un relais ne module pas |
