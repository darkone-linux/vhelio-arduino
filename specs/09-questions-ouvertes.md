# 09 — Questions ouvertes

Ce qui reste à trancher pour figer la conception. Classé par urgence.

---

## Bloquant avant le câblage

### ~~Q2 — Nature des sorties de la carte~~ — **TRANCHÉE**

**Ce sont des relais**, contacts secs 10 A NO/NC, pilotés par un registre à
décalage 74HC595. Confirmé par les spécifications Eletechsup et par la
bibliothèque de référence `af3556/IO22_IO_Board` de la même famille de cartes.

Conséquences, toutes intégrées au projet :

- La sortie modulée du feu arrière est **matériellement impossible**. Deux
  relais et deux circuits distincts (position + stop) — c'est la variante qui
  avait été écartée au départ, et le matériel la réimpose.
- Modules MOSFET, relais de klaxon et optocoupleur de coupure moteur :
  **tous inutiles**. La nomenclature s'allège d'autant.
- Le buzzer disparaît : le claquement des relais de clignotant le remplace.
- R3 et R4 deviennent les pièces d'usure du montage.

### Q1 — Confirmer le brochage au pinscan

Le brochage retenu vient de la bibliothèque de référence de la famille
IO22/DN22, pas du datasheet de votre exemplaire précis. Il reste à confirmer.

**Action** : dérouler `03-affectation-es.md` §6. Le croquis vérifie d'un coup
la chaîne de registres, l'ordre des bits des relais, les huit entrées et les
quatre boutons.

### Q11 — Le RS485 est-il câblé sur D0/D1 ?

La carte embarque une interface RS485. Si son circuit (type MAX485) est relié à
D0/D1, il entre en conflit avec la console série de mise au point, et
potentiellement avec le téléversement.

**Action** : repérer le circuit RS485 sur la carte et vérifier ses liaisons, ou
simplement observer si la console série fonctionne normalement carte alimentée.
**Repli si conflit** : `DEBUG_SERIAL 0` en exploitation, l'afficheur 4 digits
prenant le relais pour le diagnostic.

### Q12 — Où est monté le calculateur, et l'afficheur est-il visible ?

L'afficheur 4 digits et son deux-points portent désormais tout le diagnostic
embarqué : vitesse, charge, code de défaut, odomètre, battement de cœur.

- **Visible du poste de conduite** : c'est un vrai tableau de bord, et le
  rappel d'oubli des clignotants a un annonciateur.
- **Enfermé sous la coque** : il ne sert qu'à la maintenance, et le rappel
  d'oubli n'a plus aucun moyen de se manifester en roulant.

Dans le second cas, deux options pour retrouver un buzzer : renoncer à l'écoute
UART (ce qui libère A4/A5), ou utiliser la broche d'un bouton (D9 ou D10,
toutes deux capables de PWM) en sortie, avec une résistance série de 330 Ω pour
survivre à un appui simultané.

**Action** : décider de l'emplacement du boîtier.

### Q3 — Tension d'alimentation de la carte

La DN22D08 est-elle alimentée en 12 V (depuis le convertisseur) ou envisagez-vous
de l'alimenter en 24 V ? Toute la spécification suppose **12 V**. La plage
annoncée par le fabricant est DC 7–25 V.

---

## Bloquant avant la mise en route

### Q4 — Type exact des contacteurs de frein

- Le contacteur **avant** (non Bafang) est-il un simple contact sec, unipolaire
  ou bipolaire ? La variante B de câblage exige un **bipolaire** pour alimenter
  à la fois l'entrée 12 V de la carte et la ligne frein du contrôleur.
- Le contacteur **arrière** Bafang est-il à 2 fils (contact sec) ou à 3 fils
  (capteur à effet Hall, cas des freins hydrauliques) ? Un capteur Hall ne se
  met pas en parallèle aussi simplement.

**Action** : compter les fils de chaque connecteur et mesurer à l'ohmmètre le
comportement au repos / au freinage.
**Défaut retenu si sans réponse** : variante B avec micro-rupteur dédié ajouté
au levier arrière.

### Q5 — Le second BMS Daly 60 A

Rechange au garage, ou câblé en série sur le pack ? Voir `04-electricite.md` §6.
Si câblé, le courant maximal du pack tombe à 60 A et les seuils des deux BMS
doivent être décalés.

**Recommandation** : le garder en rechange, non câblé.

### Q6 — Isolation du convertisseur 48 → 12 V

Référence du convertisseur ? S'il est **isolé**, il faut soit réaliser la masse
en étoile, soit optocoupler la ligne UART sniffée (`04-electricite.md` §5).
S'il est **non isolé** (buck classique), il n'y a rien à faire.

---

## À trancher au montage

### Q7 — Interrupteur de détresse

Le comodo n'a pas de commande de feux de détresse. `IN8` attend un interrupteur
à bascule dédié. Où le monter, et faut-il un témoin lumineux ?

**Alternative** : renoncer à la détresse et réaffecter `IN8` (par exemple à un
contacteur à clé, ou à un second niveau d'éclairage).

### ~~Q8 — Rôle de `OUT8`~~ — **SANS OBJET**

Il n'y a plus de sortie auxiliaire : les huit relais sont exactement consommés
par les huit fonctions. Le buzzer est remplacé par le claquement des relais de
clignotant, et les prises allume-cigare restent sur le bus 12 V fusionné, avec
un interrupteur manuel qui ne dépend d'aucun firmware.

### Q9 — Puissance réelle des feux

Le bilan de `04-electricite.md` §2 suppose des phares LED de 20 W. Si vous
utilisez des phares de 55 W (halogène automobile), le bilan double et le
convertisseur 20 A ne suffit plus.

**Action** : donner les références des feux avant, arrière et des clignotants.

### Q10 — Circonférence de roue

`WHEEL_CIRCUMFERENCE_MM` vaut 2200 mm (roue 700 C). À corriger selon la roue
motrice réelle. Ne sert qu'à la formule de vitesse « période de roue » et à
l'odomètre — aucune fonction de sécurité.

---

## Évolutions possibles, hors périmètre v1

| Idée | Coût | Obstacle |
|---|---|---|
| Lecture du MPPT en VE.Direct | Faible | Plus d'UART libre (cf. `04-electricite.md` §7) |
| ~~Écran de bord dédié~~ | — | Sans objet : la carte a son afficheur 4 digits |
| Journalisation sur carte SD | Élevé | Le SPI est inutilisable : D11/D12 sont des entrées, D13 la ligne de données du registre |
| Détection de rupture de lampe | Élevé | Nécessite une mesure de courant par voie |
| Arrêt d'urgence par la broche OE | Faible | Déjà câblé et implémenté ; il ne reste qu'à définir sa condition de déclenchement |
| Forçage du niveau d'assistance | Élevé | Impose l'interposition UART, écartée (cf. `05` §8) |
| Feu stop proportionnel à la décélération | — | Impossible : un relais ne module pas |
