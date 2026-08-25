# Archive — les treize questions tranchées

> Extrait de `specs/09-questions-ouvertes.md`, sorti de la spécification active
> le 25/08/2026. **Chaque conclusion est déjà intégrée** dans le document
> concerné et dans le code ; ce fichier ne garde que le raisonnement, pour le
> jour où l'une de ces décisions serait rouverte.
>
> Q13 (« faut-il changer de matériel ? ») a sa propre archive :
> `alternatives-materiel.md`.

## Tranché

### ~~Q1 — Confirmer le brochage au pinscan~~ — **entièrement mesuré**

La question demandait de *confirmer* trois points. Les trois sont tranchés,
mais aucun ne l'a été par confirmation : le brochage supposé était faux, et il
a fallu le **découvrir** au lieu de le vérifier. `tools/pinscan` ne sait que
vérifier une hypothèse ; il ne peut rien dire quand c'est l'hypothèse qui est
fausse. D'où trois outils de plus — `pinfind`, `pinchain`, `pindisp` — et un
quatrième, `dispcheck`, pour valider l'assemblage des mesures.

Les trois points posés :

1. **L'ordre des relais.** **Correspondance directe**, bit 0 → CH1 … bit 7 →
   CH8. Le décalage de l'IO22D08, où le relais 8 occupait le bit 0, **n'existe
   pas sur la DN22D08**. `RELAY_BIT[]` est devenu l'identité — on le garde
   quand même, il coûte huit octets et garde le firmware indifférent à la
   question.
2. **L'ordre des boutons.** **Il n'y a rien à inverser** : K1 est bien le
   poussoir de gauche. Ce sont les **numéros de broche** qui décroissent de
   gauche à droite — 12, 10, 8, puis A0 (= 14) — et c'est cette décroissance
   qui a fait parler d'une sérigraphie « inversée » sur les cartes voisines.
3. **La polarité des entrées.** **NPN confirmée à la masse**, conformément à la
   fiche du constructeur. Tous les communs du faisceau vont à GND. Le
   sertissage n'est plus bloqué.

Ce que la question ne demandait pas, et que la mesure a livré :

| | Supposé (bibliothèque IO22D08) | **Mesuré (DN22D08)** |
|---|---|---|
| Boutons K1..K4 | D7, D8, D9, D10 | **D12, D10, D8, A0** |
| IN6 / IN7 | A0 / D12 | **D7 / D9** |
| Chaîne données / horloge / verrou | D13 / A3 / A2 | **A5 / A4 / A3** |
| Validation OE | A1 | **A2**, active à l'état bas |
| Afficheur | 1 octet segments + 1 octet sélection | **segments répartis sur les deux octets**, sélection **active à l'état bas** |
| Deux-points | supposé présent | **inexistant** — rien que des points décimaux |

Deux conséquences se paient, aucune n'est bloquante :

- **L'écoute Bafang déménage sur A1.** A4 lui était réservée ; la chaîne a pris
  A4 et A5, l'OE a pris A2. A1 est la dernière broche libre à interruption sur
  changement d'état — A6 et A7 sont analogiques seules. D13 hérite du TX
  logiciel, non câblé, ce qui laisse la **LED du Nano allumée en fixe**.
- **Le battement de cœur passe sur le point décimal du digit de gauche**, à
  défaut de deux-points. Aucun format numérique ne réclame ce point-là.

Détail complet en `03-affectation-es.md` §2, §6 bis, §6 ter et §6 quater.

> **Ce que cet épisode valide.** Le brochage était concentré dans `pins.h` et
> deux tableaux de `board_io.cpp`, et la couche `board_io` existait déjà comme
> abstraction. Quatre campagnes de mesure ont réfuté le brochage supposé sur
> presque tous les points **sans qu'un seul module métier soit touché**.

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

1. `DISPLAY_DEFAULT_PAGE` passe à **0 (événements)** : c'est ce qu'on veut voir
   en ouvrant la coque. La page dit en clair ce qui vient de se produire —
   `Fr`, `Ph`, `Err` — plutôt qu'un code hexadécimal qui suppose d'avoir la
   documentation sous les yeux. Le code reste sur la page défauts, au bouton
   K1 : « Err » dit qu'il y a un défaut, elle seule dit lequel.
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

