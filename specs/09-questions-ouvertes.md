# 09 — Questions ouvertes

Ce qui reste à trancher pour figer la conception. Classé par urgence.

**Quatorze questions ont été posées, douze sont tranchées.** Les deux
restantes attendent du matériel ou une manipulation, pas une décision. Le détail de chaque
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
3. **La polarité des entrées.** ~~À déterminer~~ — **confirmée NPN par la
   fiche du constructeur** : « 8x opto-isolated inputs (low level trigger, NPN
   type) ». On active une borne en la fermant sur la **masse**, et tous les
   communs du faisceau vont donc à la masse. Il ne reste qu'à le vérifier au
   passage.

**Action** : dérouler `03-affectation-es.md` §6 avec `tools/pinscan`. Une
demi-heure, avant de sertir quoi que ce soit.

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
  `IN4` et la masse, sans interface — les entrées étant NPN. Un contact
  unipolaire semblait ne pouvoir servir qu'une fois, laissant la coupure
  d'assistance au frein avant dépendre du firmware. **Une diode 1N4148 lève la
  limite** : les deux circuits demandent une mise à la masse, la diode bloque
  simplement le +12 V de la carte quand le contact est ouvert. Un seul contact
  sert donc les deux, la diode se monte dans le boîtier, et rien n'est modifié
  au levier. Voir `hardware/cablage.md` §4.
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
3. **10 A suffisent largement.** L'éclairage complet ne prend que 5,0 A depuis
   la mesure des phares (Q9), et le klaxon — seule charge à forte pointe — est
   autonome (Q14). Il ne reste qu'un arbitrage : prises allume-cigare
   fusiblées à **5 A**, une prise à 10 A ferait déborder le convertisseur
   (`04-electricite.md` §2.3).

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

### ~~Q9 — Puissance des phares~~ — **12 W chacun, mesurés**

Mesure à la pince : **12 V / 1 A par phare**, soit 24 W pour la paire, contre
60 W annoncés. Écart typique des projecteurs LED vendus en « watts crête ».
Le bilan retient **15 W chacun** pour se garder une marge.

Conséquences, toutes favorables :

- **L'éclairage complet ne consomme que 5,0 A**, soit la moitié du
  convertisseur. Le budget passe de tendu à confortable.
- **Aucun relais externe** : R2 voit 2,5 A pour un calibre de 10 A.
- **Le condensateur tampon devient inutile** : il n'existait que pour absorber
  l'appel du klaxon, lequel s'est révélé autonome (Q14).
- Fusible F6 ramené de 7,5 A à **5 A**, section d'éclairage confirmée en
  1,5 mm².

### ~~Q14 — Que faire de la voie libérée par le klaxon ?~~ — **option A retenue**

Le klaxon a sa propre batterie et son propre interrupteur : aucun lien avec la
carte. `HORN_ENABLE` passe à 0 — le module est conservé mais n'est plus
compilé — et le fusible F8 comme le condensateur tampon disparaissent.

La voie `IN3` / `R7` ainsi libérée est affectée au **diagnostic au poste de
conduite**, seul moyen d'apprendre en roulant qu'un défaut est actif
maintenant que l'afficheur est sous la coque (Q12) :

- **`R7`** alimente un **voyant rouge**, allumage fixe.
- **`IN3`** reçoit un **bouton d'acquittement**.

Trois points de conception valent d'être retenus :

1. **Tous les défauts n'allument pas le voyant.** Le masque `FAULT_LAMP_MASK`
   exclut délibérément la perte du bus Bafang : c'est le principe P2 appliqué,
   la télémétrie est un confort. Afficheur d'origine débranché, le voyant
   resterait allumé en permanence et cesserait de vouloir dire quoi que ce
   soit. La règle est vérifiée **à la compilation** par un `static_assert`.
2. **Le voyant s'allume pendant l'autotest**, comme un témoin de tableau de
   bord au contact. Sans cela, une LED grillée serait indiscernable d'une
   absence de défaut — et c'est le seul mode de panne de ce dispositif.
3. **L'acquittement porte sur un événement, pas sur une catégorie.** Un défaut
   acquitté qui disparaît puis revient rallume le voyant.

Détail en `02-machines-a-etats.md` §2.4, exigences F-4.1 à F-4.9, test T1.6.

Coût : une LED rouge, une résistance, un bouton poussoir. **156 octets de
flash et 15 octets de RAM.**

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
| ~~Micro-rupteur sur le levier avant~~ | — | **Sans objet** : la diode D1 obtient le même résultat sans toucher au levier |
| Convertisseur 48/12 V donné pour 72 V | ~15 € | Aucun, au prochain achat |
| Lecture du MPPT en VE.Direct | Faible | Plus d'UART libre (`04-electricite.md` §7) |
| ~~Écran de bord dédié~~ | — | Sans objet : la carte a son afficheur… mais il est sous la coque |
| Journalisation sur carte SD | Élevé | Le SPI est inutilisable : D11/D12 sont des entrées, D13 la ligne de données du registre |
| Détection de rupture de lampe | Élevé | Nécessite une mesure de courant par voie |
| Arrêt d'urgence par la broche OE | Faible | Déjà câblé et implémenté ; il ne reste qu'à définir sa condition de déclenchement |
| Forçage du niveau d'assistance | Élevé | Impose l'interposition UART, écartée (`05` §8) |
| Feu stop proportionnel à la décélération | — | Impossible : un relais ne module pas |
