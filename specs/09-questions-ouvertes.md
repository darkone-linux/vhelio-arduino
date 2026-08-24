# 09 — Questions ouvertes

Ce qui reste à trancher pour figer la conception. Classé par urgence.

---

## Bloquant avant le câblage

### Q1 — Brochage réel de la DN22D08

Le firmware part du brochage `OUT1..8 = D2..D9` / `IN1..8 = A0..A7`. Il existe
des révisions différentes de cette famille de cartes.

**Action** : dérouler la procédure `03-affectation-es.md` §5 et confirmer ou
corriger `pins.h`. Dix minutes, et cela conditionne tout le reste.

### Q2 — Nature des sorties de la carte

Relais secs, ou MOSFET / transistors ? Et quel courant par voie, quel courant
total ?

Le dimensionnement de `04-electricite.md` suppose des sorties **à transistor,
côté bas, ~0,5 A par voie**, d'où les modules MOSFET et le relais klaxon en
externe. Si ce sont des relais 10 A, le relais klaxon devient inutile — mais
alors la sortie PWM du feu arrière n'est plus possible (un relais ne module
pas), et il faudra basculer sur le câblage à deux circuits pour les feux
arrière.

**Action** : donner la référence exacte ou une photo du bornier / du datasheet.
**Impact si non tranché** : la sortie `OUT_TAIL` en PWM est le seul point de la
conception qui dépend vraiment de la réponse.

### Q3 — Tenue en tension des sorties et alimentation de la carte

La DN22D08 est-elle alimentée en 12 V (depuis le convertisseur) ou envisagez-vous
de l'alimenter en 24 V ? Toute la spécification suppose **12 V**.

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

### Q8 — Rôle de `OUT8`

Par défaut : **buzzer de clignotants**. Sur un vélomobile caréné, le conducteur
ne voit pas toujours ses répétiteurs — le retour sonore a une vraie valeur.

L'alternative est un **relais de coupure des prises allume-cigare**, pour éviter
de vider le pack sur un appareil oublié. Les deux ne tiennent pas sur une seule
sortie.

**Recommandation** : buzzer. Le risque de décharge par les prises se traite avec
un interrupteur manuel, qui ne coûte rien et ne dépend d'aucun firmware.

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
| Écran de bord dédié (OLED I²C) | Faible | A4/A5 mobilisées par la carte |
| Journalisation sur carte SD | Moyen | SPI libre (D10–D13) mais D10 sert au sniff |
| Détection de rupture de lampe | Élevé | Nécessite une mesure de courant par voie |
| Forçage du niveau d'assistance | Élevé | Impose l'interposition UART, écartée (cf. `05` §8) |
| Feu stop proportionnel à la décélération | Moyen | Nécessite un accéléromètre I²C, donc A4/A5 |
