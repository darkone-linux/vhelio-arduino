# 01 — Exigences fonctionnelles

Chaque exigence porte un identifiant stable `F-xx`, une priorité et un critère de
vérification. Les priorités : **M** = obligatoire (Must), **S** = souhaitable
(Should), **C** = confort (Could).

---

## F-1 — Éclairage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-1.1 | M | Le feu de croisement s'allume quand l'entrée `IN_LOWBEAM` du comodo est active, s'éteint sinon. | Actionner le comodo, mesurer la sortie OUT1. |
| F-1.2 | M | Les feux de position arrière (relais R5) s'allument dès que le feu de croisement est allumé. | Contrôle visuel + continuité sur R5. |
| F-1.3 | M | Le feu de route s'allume quand `IN_HIGHBEAM` est active **et** que le feu de croisement est allumé. | Actionner route sans croisement : OUT2 doit rester inactive. |
| F-1.4 | S | Le feu de croisement reste allumé quand le feu de route est actif (éclairage cumulé). Configurable par `HIGHBEAM_KEEPS_LOWBEAM`. | Les deux sorties actives simultanément. |
| F-1.5 | — | *Supprimée.* La rampe d'allumage progressive supposait une sortie modulable. Les sorties de la carte sont des relais : sans objet. |
| F-1.6 | C | Option feux de jour : les feux de position arrière peuvent être forcés en permanence (`TAIL_ALWAYS_ON`). | Compilation avec l'option, éclairage éteint : R5 collé. |

## F-2 — Freinage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-2.1 | M | Le système détecte l'action sur le frein avant **et** sur le frein arrière. | Actionner chaque levier séparément. |
| F-2.2 | M | Dès qu'un frein au moins est actionné, le feu stop (relais R6) s'allume. | Chrono : < 50 ms entre contact et allumage, temps de collage du relais inclus. |
| F-2.3 | M | Le feu stop reste allumé tant qu'un frein est actionné, quel que soit l'état de l'éclairage. Il est sur un circuit distinct des feux de position. | Éclairage éteint + freinage : R6 collé, R5 relâché. |
| F-2.4 | M | La coupure moteur matérielle (contacteurs câblés sur la ligne frein du contrôleur) est indépendante de l'Arduino. | Arduino débranché : le moteur doit toujours se couper au freinage. |
| F-2.5 | S | La sortie `OUT_MOTOR_CUT` reproduit l'état de freinage, avec un maintien minimum de 300 ms après relâche (anti-battement). | Relâche brève : la sortie reste active 300 ms. |
| F-2.6 | S | L'anti-rebond des entrées frein est ≤ 15 ms, pour ne pas retarder l'allumage du stop. | Injection d'un rebond de 5 ms : pas de scintillement, pas de retard > 20 ms. |
| F-2.7 | C | Option « flash d'attaque » : 3 clignotements rapides du stop avant l'allumage fixe. **Désactivée par défaut** — non conforme au code de la route français, et surtout six manœuvres mécaniques supplémentaires par freinage sur un relais. | Compilation avec `BRAKE_FLASH_ENABLE 1` (le compilateur émet un avertissement). |

## F-3 — Clignotants et détresse

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-3.1 | M | La cadence de clignotement est de 1,33 Hz (période 750 ms), rapport cyclique 50 %, dans la plage réglementaire 60–120 cycles/min. | Chronométrage sur 30 cycles. |
| F-3.2 | M | Gauche et droite sont mutuellement exclusifs. Si les deux entrées sont actives simultanément (défaut de comodo), les deux clignotants sont éteints. | Forcer les deux entrées : aucune sortie active. |
| F-3.3 | M | Le clignotement démarre par une phase **allumée**, immédiatement à l'activation. | Oscilloscope : pas de temps mort au démarrage. |
| F-3.4 | M | Les feux de détresse allument les deux côtés **en phase** et sont prioritaires sur les clignotants. | Détresse + clignotant gauche : les deux côtés clignotent. |
| F-3.5 | M | Les feux de détresse fonctionnent éclairage éteint. | Test avec `IN_LOWBEAM` inactive. |
| F-3.6 | S | Un retour sonore accompagne chaque transition du clignotant. **Assuré sans aucun composant** : les clignotants sont portés par des relais, dont le claquement à 1,33 Hz est exactement le bruit d'un relais de clignotant d'origine. | Écoute. |
| F-3.7 | S | Rappel d'oubli : au-delà de 45 s **ou** 300 m de clignotement continu, l'état est signalé au journal série et à l'afficheur. Le clignotant n'est **pas** annulé automatiquement (le comodo est un inverseur maintenu : une annulation logicielle créerait une incohérence entre la position du levier et l'état réel). **Limite connue** : sans buzzer, le rappel n'est pas audible et n'est visible que si l'afficheur de la carte est dans le champ de vision — voir Q12. | Laisser le clignotant 50 s. |

## F-4 — Klaxon

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-4.1 | M | Le klaxon sonne tant que le bouton du comodo est enfoncé. | Appui / relâche. |
| F-4.2 | M | Le klaxon est piloté par un relais externe : la sortie de la carte ne supporte pas les 5–8 A d'un klaxon 12 V. | Contrôle de la nomenclature. |
| F-4.3 | S | Anti-blocage : au-delà de 10 s continues, le klaxon est coupé jusqu'au relâchement du bouton. | Maintenir l'appui 15 s. |
| F-4.4 | S | Anti-rebond 20 ms sur le bouton. | Injection de rebonds. |

## F-5 — Télémétrie Bafang (écoute passive)

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-5.1 | M | L'Arduino **n'émet jamais** sur la liaison UART Bafang. La broche TX du port logiciel n'est pas câblée. | Contrôle visuel du câblage + revue de code. |
| F-5.2 | S | Les trames contrôleur → afficheur sont segmentées par silence inter-trame (> 30 ms) et validées par somme de contrôle. | Mode apprentissage, comptage des trames rejetées. |
| F-5.3 | S | Vitesse, courant et niveau de charge sont extraits quand les trames correspondantes sont reconnues. | Comparaison avec l'afficheur d'origine. |
| F-5.4 | S | Un mode apprentissage (`BAFANG_LEARN_MODE`) dumpe en hexadécimal toutes les trames valides sur le port série, pour calibrer le décodage sur le matériel réel. | Sortie série lisible. |
| F-5.5 | M | Si aucune trame valide n'est reçue pendant 2 s, la télémétrie est marquée invalide. Aucune autre fonction n'est dégradée. | Débrancher l'afficheur en roulant : feux et freins inchangés. |
| F-5.6 | C | Source de vitesse alternative : lecture directe du capteur de roue par scrutation d'une entrée dédiée (`SPEED_SOURCE_WHEEL`). | Compilation avec l'option. |

## F-6 — Diagnostic et sécurité

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-6.1 | M | Un chien de garde matériel (WDT 1 s) redémarre l'Arduino en cas de blocage. | Boucle infinie injectée en test. |
| F-6.2 | M | Le WDT est explicitement désarmé au tout début de `setup()` (`MCUSR = 0; wdt_disable();`) pour éviter le redémarrage en boucle avec les anciens bootloaders. | Revue de code. |
| F-6.3 | S | Un autotest au démarrage active chaque sortie 150 ms dans l'ordre, hors klaxon et coupure moteur. | Observation visuelle. |
| F-6.4 | S | La LED intégrée (D13) bat à 1 Hz en fonctionnement nominal, à 5 Hz si un défaut est actif. | Observation. |
| F-6.5 | S | Un journal série (115 200 bauds) publie l'état consolidé toutes les secondes, désactivable à la compilation. | Terminal série. |
| F-6.6 | M | Le temps de cycle maximal observé est journalisé ; il doit rester < 10 ms. | Champ `loopMax` du journal. |

## F-7 — Contraintes non fonctionnelles

| Id | Prio | Exigence |
|---|---|---|
| NF-1 | M | Empreinte flash < 24 ko et RAM statique < 1,4 ko (marge sur les 30,7 ko / 2 ko utilisables du ATmega328P). **Mesuré : 8 750 o de flash (28 %) et 703 o de RAM (34 %).** |
| NF-2 | M | Aucun `String`, aucune allocation dynamique. |
| NF-3 | M | Tout le brochage est concentré dans `src/pins.h` et les tableaux en tête de `src/board_io.cpp` ; tout le réglage dans `src/config.h`. C'est ce qui a permis d'absorber le changement complet d'architecture de la carte sans toucher un seul module métier. |
| NF-4 | S | Chaque fonction est un module indépendant, testable en isolant ses entrées. |
| NF-5 | M | Le firmware compile sans avertissement avec `--warnings all`, dans **toutes** les combinaisons d'options de `config.h` (`tools/check-variants.sh`). |
