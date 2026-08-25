# 09 — Questions ouvertes

Quatorze questions ont été posées, **treize sont tranchées**. Leurs conclusions
sont intégrées dans les documents concernés et dans le code ; le raisonnement
est archivé dans
[`archives/questions-tranchees.md`](archives/questions-tranchees.md) et, pour
Q13, dans [`archives/alternatives-materiel.md`](archives/alternatives-materiel.md).

---

## Q10 — Circonférence de roue *(seule question encore ouverte)*

`WHEEL_CIRCUMFERENCE_MM` vaut **2200 mm** en attendant la roue réelle. Ne sert
qu'à la formule de vitesse « période de roue » (`BAFANG_SPEED_FORMULA 1`) et à
l'odomètre : **aucune fonction de sécurité**. À corriger quand la roue sera
montée, puis rejouer T2.5.

## Ce qui reste à mesurer avant de rouler

Ce ne sont pas des questions de conception — les décisions sont prises — mais
des mesures dont dépend un câblage déjà spécifié.

| À mesurer | Où | Conséquence si le résultat surprend |
|---|---|---|
| **Polarité de la ligne frein Bafang** au multimètre, connecteur jaune | `03` §5, `hardware/cablage.md` §4 | Si elle est active à l'état **haut**, la diode D1 est inopérante et doit être retirée ; le frein avant redevient dépendant du firmware, et R8 passe en contact NC |
| **Table de registres Bafang**, en mode apprentissage | `05` §6 | Le décodage vitesse / charge / courant est faux tant qu'elle n'est pas calibrée. Sans effet sur les feux (P2) |
| **Courant total d'éclairage** à la pince | `04` §2.2 | Attendu ≈ 5 A. Au-delà de 6 A, reprendre le bilan de puissance |

## Évolutions possibles, hors périmètre v1

| Idée | Coût | Obstacle |
|---|---|---|
| Convertisseur 48/12 V donné pour 72 V | ~15 € | Aucun, au prochain achat |
| Lecture du MPPT en VE.Direct | Faible | Plus d'UART libre (`04` §7) |
| Journalisation sur carte SD | Élevé | Le SPI est inutilisable : D11 porte IN8, D12 le bouton K1, D13 le TX Bafang |
| Détection de rupture de lampe | Élevé | Nécessite une mesure de courant par voie |
| Arrêt d'urgence par la broche OE | Faible | Déjà câblé et implémenté (`board::outputsEnabled`) ; il ne reste qu'à définir sa condition de déclenchement |
| Forçage du niveau d'assistance | Élevé | Impose l'interposition UART, écartée (`05` §8) |
| Feu stop proportionnel à la décélération | — | Impossible : un relais ne module pas |
