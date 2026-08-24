# 01 — Exigences fonctionnelles

Chaque exigence porte un identifiant stable `F-xx`, une priorité et un critère de
vérification. Les priorités : **M** = obligatoire (Must), **S** = souhaitable
(Should), **C** = confort (Could).

---

## F-1 — Éclairage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-1.1 | M | La veilleuse avant (R1) s'allume quand l'interrupteur dédié `IN_PARK` est actif, s'éteint sinon. | Actionner l'interrupteur, mesurer R1. |
| F-1.2 | M | Les feux de position arrière (R5) s'allument dès qu'**un niveau quelconque** d'éclairage avant est demandé — veilleuse **ou** phares. Non configurable. | Comodo seul, interrupteur veilleuse ouvert : R5 doit coller. |
| F-1.3 | M | Les phares (R2) s'allument quand l'interrupteur du comodo `IN_MAIN` est actif. Ils ne sont **pas** conditionnés à la veilleuse (`MAIN_REQUIRES_PARK` = 0) : un interrupteur de veilleuse resté ouvert ne doit jamais priver d'éclairage. | Comodo seul : R2 doit coller. |
| F-1.4 | S | La veilleuse reste allumée quand les phares sont actifs (`MAIN_KEEPS_PARK`). | R1 et R2 actives simultanément. |
| F-1.5 | — | *Supprimée.* La rampe d'allumage progressive supposait une sortie modulable. Les sorties de la carte sont des relais : sans objet. |
| F-1.6 | C | Option feux de jour : les feux de position arrière peuvent être forcés en permanence (`TAIL_ALWAYS_ON`). | Compilation avec l'option, éclairage éteint : R5 collé. |
| F-1.7 | M | Aucune configuration ne permet d'allumer un éclairage avant sans le feu de position arrière. | Revue de code de `lights::update` + balayage `check-variants.sh`. |

## F-2 — Freinage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-2.1 | M | Le système détecte l'action sur le frein avant **et** sur le frein arrière. | Actionner chaque levier séparément. |
| F-2.2 | M | Dès qu'un frein au moins est actionné, le feu stop (relais R6) s'allume. | Chrono : < 50 ms entre contact et allumage, temps de collage du relais inclus. |
| F-2.3 | M | Le feu stop reste allumé tant qu'un frein est actionné, quel que soit l'état de l'éclairage. Il est sur un circuit distinct des feux de position. | Éclairage éteint + freinage : R6 collé, R5 relâché. |
| F-2.4 | M | La coupure moteur au frein **arrière** est indépendante de l'Arduino : le contacteur Bafang d'origine reste câblé au contrôleur. | Arduino débranché, frein arrière : le moteur doit se couper. |
| F-2.4bis | M | La coupure moteur au frein **avant** est également indépendante de l'Arduino, bien que le contacteur soit unipolaire : une diode **D1 (1N4148)** lui permet de servir à la fois l'entrée `IN4` et la ligne frein, en bloquant le +12 V de la carte vers la ligne 5 V du contrôleur. | Arduino débranché, frein avant : le moteur doit se couper. Et la tension de la ligne frein au repos doit être inchangée par rapport à l'avant-montage. |
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
| F-3.5 | M | Les feux de détresse fonctionnent éclairage éteint, et sont commandés par un interrupteur dédié sur `IN8`. | Test avec `IN_PARK` et `IN_MAIN` inactives. |
| F-3.6 | S | Un retour sonore accompagne chaque transition du clignotant. **Assuré sans aucun composant** : les clignotants sont portés par des relais, dont le claquement à 1,33 Hz est exactement le bruit d'un relais de clignotant d'origine. | Écoute. |
| F-3.7 | S | Rappel d'oubli : au-delà de 45 s **ou** 300 m de clignotement continu, la phase allumée est raccourcie de 375 à 200 ms (`BLINK_REMINDER_ON_MS`). La **période est inchangée**, donc la cadence reste réglementaire, mais le rythme du claquement des relais devient syncopé et audible. C'est le seul canal vers le conducteur, le boîtier étant sous la coque (Q12). Le clignotant n'est **pas** annulé automatiquement : le comodo est un inverseur maintenu, une annulation logicielle créerait une incohérence entre la position du levier et l'état réel. | Laisser le clignotant 50 s, écouter le changement de rythme. |

## F-4 — Klaxon

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-4.1 | M | Le klaxon sonne tant que le bouton du comodo est enfoncé. | Appui / relâche. |
| F-4.2 | M | Le klaxon est branché **directement** sur R7 : le contact sec supporte 10 A. Un relais externe n'est requis que pour un klaxon à compresseur, dont la pointe d'appel peut coller le contact. | Contrôle de la nomenclature + échauffement après 10 coups. |
| F-4.5 | M | Un coup de klaxon ne doit pas faire varier l'éclairage. L'éclairage complet consommant 5,0 A sur un convertisseur de 10 A, un klaxon de plus de ~4 A impose un condensateur tampon de 10 000 µF. | Mesurer le klaxon à la pince. De nuit, phares allumés, klaxonner : aucune variation visible. |
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
| F-6.3 | S | Un autotest au démarrage active chaque sortie 200 ms dans l'ordre, hors klaxon et coupure moteur. | Observation visuelle et **auditive** — c'est le seul contrôle de bon fonctionnement des relais audible depuis le poste de conduite. |
| F-6.4 | S | Le **deux-points de l'afficheur** bat à 1 Hz en fonctionnement nominal, à ~4 Hz si un défaut est actif. La LED D13 du Nano n'est pas utilisable : elle porte la ligne de données du registre à décalage. | Observation, coque ouverte. |
| F-6.5 | S | Un journal série (115 200 bauds) publie l'état consolidé toutes les secondes, désactivable à la compilation. L'inverseur de la carte doit être sur **`PRO`**, faute de quoi le RS485 occupe D0/D1. | Terminal série. |
| F-6.6 | M | Le temps de cycle maximal observé est journalisé ; il doit rester < 10 ms. | Champ `loopMax` du journal. |

## F-7 — Contraintes non fonctionnelles

| Id | Prio | Exigence |
|---|---|---|
| NF-1 | M | Empreinte flash < 24 ko et RAM statique < 1,4 ko (marge sur les 30,7 ko / 2 ko utilisables du ATmega328P). **Mesuré : 8 918 o de flash (29 %) et 705 o de RAM (34 %).** |
| NF-2 | M | Aucun `String`, aucune allocation dynamique. |
| NF-3 | M | Tout le brochage est concentré dans `src/pins.h` et les tableaux en tête de `src/board_io.cpp` ; tout le réglage dans `src/config.h`. C'est ce qui a permis d'absorber le changement complet d'architecture de la carte sans toucher un seul module métier. |
| NF-4 | S | Chaque fonction est un module indépendant, testable en isolant ses entrées. |
| NF-5 | M | Le firmware compile sans avertissement avec `--warnings all`, dans **toutes** les combinaisons d'options de `config.h` (`tools/check-variants.sh` — 12 variantes). |
