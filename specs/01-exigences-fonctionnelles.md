# 01 — Exigences fonctionnelles

Chaque exigence porte un identifiant stable `F-x.y`, une priorité et un critère
de vérification. Priorités : **M** = obligatoire, **S** = souhaitable,
**C** = confort.

---

## F-1 — Éclairage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-1.1 | M | La veilleuse avant (R1) s'allume quand `IN_PARK` est actif, s'éteint sinon | Actionner l'interrupteur, mesurer R1 |
| F-1.2 | M | Les feux de position arrière (R5) s'allument dès qu'**un niveau quelconque** d'éclairage avant est demandé — veilleuse **ou** phares. Non configurable | Comodo seul, interrupteur veilleuse ouvert : R5 doit coller |
| F-1.3 | M | Les phares (R2) s'allument quand `IN_MAIN` est actif. Ils ne sont **pas** conditionnés à la veilleuse (`MAIN_REQUIRES_PARK 0`) : un interrupteur de veilleuse resté ouvert ne doit jamais priver d'éclairage | Comodo seul : R2 doit coller |
| F-1.4 | S | La veilleuse reste allumée quand les phares sont actifs (`MAIN_KEEPS_PARK`) | R1 et R2 actives simultanément |
| F-1.5 | — | *Supprimée.* La rampe d'allumage progressive supposait une sortie modulable ; les sorties sont des relais | — |
| F-1.6 | C | Option feux de jour : les feux de position arrière peuvent être forcés en permanence (`TAIL_ALWAYS_ON`) | Compilation avec l'option, éclairage éteint : R5 collé |
| F-1.7 | M | **Aucune configuration ne permet d'allumer un éclairage avant sans le feu de position arrière** | Revue de `lights::update` + balayage `check-variants.sh` |

## F-2 — Freinage

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-2.1 | M | Le système détecte l'action sur le frein avant **et** sur le frein arrière | Actionner chaque levier séparément |
| F-2.2 | M | Dès qu'un frein au moins est actionné, le feu stop (R6) s'allume | Chrono : < 50 ms entre contact et allumage, temps de collage du relais inclus |
| F-2.3 | M | Le feu stop reste allumé tant qu'un frein est actionné, quel que soit l'état de l'éclairage, sur un circuit distinct des feux de position | Éclairage éteint + freinage : R6 collé, R5 relâché |
| F-2.4 | M | La coupure moteur au frein **arrière** est indépendante de l'Arduino : le contacteur Bafang d'origine reste câblé au contrôleur | **Arduino débranché**, frein arrière : le moteur doit se couper |
| F-2.4bis | M | La coupure moteur au frein **avant** est également indépendante de l'Arduino, bien que le contacteur soit unipolaire : la diode **D1 (1N4148)** lui permet de servir à la fois `IN4` et la ligne frein | **Arduino débranché**, frein avant : le moteur doit se couper. Et la tension de la ligne frein au repos doit être **inchangée** par rapport à l'avant-montage |
| F-2.5 | S | `OUT_MOTOR_CUT` reproduit l'état de freinage, avec un maintien minimum de 300 ms après relâche (anti-battement) | Relâche brève : la sortie reste active 300 ms |
| F-2.6 | S | L'anti-rebond des entrées frein est ≤ 15 ms, pour ne pas retarder l'allumage du stop | Injection d'un rebond de 5 ms : pas de scintillement, pas de retard > 20 ms |
| F-2.7 | C | Option « flash d'attaque » : 3 clignotements rapides du stop avant l'allumage fixe. **Désactivée par défaut** — non conforme au code de la route français, et six manœuvres mécaniques de plus par freinage | Compilation avec `BRAKE_FLASH_ENABLE 1` (le compilateur émet un avertissement) |

## F-3 — Clignotants et détresse

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-3.1 | M | Cadence 1,33 Hz (période 750 ms), rapport cyclique 50 %, dans la plage réglementaire 60–120 c/min | Chronométrage sur 30 cycles : 22,5 s ± 1 s |
| F-3.2 | M | Gauche et droite mutuellement exclusifs. Les deux entrées actives simultanément (défaut de comodo) éteignent les deux clignotants | Forcer les deux entrées : aucune sortie active |
| F-3.3 | M | Le clignotement démarre par une phase **allumée**, immédiatement à l'activation | Pas de temps mort au démarrage |
| F-3.4 | M | La détresse allume les deux côtés **en phase** et est prioritaire sur les clignotants | Détresse + clignotant gauche : les deux côtés clignotent, en phase |
| F-3.5 | M | La détresse fonctionne éclairage éteint, commandée par un interrupteur dédié sur `IN8` | Test avec `IN_PARK` et `IN_MAIN` inactives |
| F-3.6 | S | Un retour sonore accompagne chaque transition. **Assuré sans aucun composant** : le claquement des relais à 1,33 Hz est exactement le bruit d'un relais de clignotant d'origine | Écoute |
| F-3.7 | S | Rappel d'oubli : au-delà de 45 s **ou** 300 m, la phase allumée passe de 375 à 200 ms (`BLINK_REMINDER_ON_MS`). La **période est inchangée**, donc la cadence reste réglementaire, mais le claquement devient syncopé. Le clignotant n'est **pas** annulé automatiquement | Laisser le clignotant 50 s, écouter le changement de rythme |

## F-4 — Voyant de défaut et acquittement

La voie `IN3` / `R7`, libérée par le klaxon autonome, porte le diagnostic au
poste de conduite : le boîtier est sous la coque, rien ne signalait un défaut
avant l'ouverture.

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-4.1 | M | `R7` alimente un voyant rouge dès qu'un défaut du masque `FAULT_LAMP_MASK` est actif et non acquitté | Provoquer un conflit de clignotants : le voyant s'allume |
| F-4.2 | M | L'allumage est **fixe**, jamais clignotant : un relais n'est pas fait pour battre. La discrimination se lit sur l'afficheur, coque ouverte | Observation ; comptage des manœuvres de R7 sur un trajet |
| F-4.3 | M | **Le lien Bafang perdu n'allume PAS le voyant** (P2) : afficheur débranché, il resterait allumé en permanence et ne voudrait plus rien dire. Contrôlé à la compilation par un `static_assert` | Débrancher l'afficheur Bafang : `FLT_BAFANG_LINK` au journal, voyant **éteint** |
| F-4.4 | M | Le voyant s'allume pendant l'autotest de mise sous tension, puis s'éteint — comme un témoin de tableau de bord au contact. Sans cela, une LED grillée serait indiscernable d'une absence de défaut | Mise sous tension : R7 colle 200 ms en fin de séquence |
| F-4.5 | M | Le bouton `IN3` efface les défauts **mémorisés** (reset chien de garde, cycle lent), qui sinon survivraient jusqu'à la coupure de l'alimentation | Provoquer un reset WDT, acquitter : `FLT_WDT_RESET` disparaît du journal |
| F-4.6 | M | Pour un défaut **encore actif**, l'acquittement éteint le voyant sans masquer le défaut sur l'afficheur ni au journal | Frein collé + acquittement : voyant éteint, `flt=0x08 ack` au journal |
| F-4.7 | M | Un défaut acquitté qui **disparaît puis revient** rallume le voyant : l'acquittement porte sur un événement, pas sur une catégorie | Conflit, acquitter, relâcher, refaire : le voyant se rallume |
| F-4.8 | M | Le bouton d'acquittement n'a **aucun** effet sur les feux, les freins ou la coupure moteur. C'est le seul organe actionnable en roulant, il ne doit toucher que `diag` | Revue de code : `acknowledge()` n'écrit que sur `g_faults` et `g_acked` |
| F-4.9 | C | *(si `HORN_ENABLE 1`)* La voie redevient un klaxon avec verrou anti-blocage à 10 s. Les deux usages s'excluent, `config.h` le vérifie par `#error` | Compilation avec l'option |

## F-5 — Télémétrie Bafang (écoute passive)

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-5.1 | M | L'Arduino **n'émet jamais** sur la liaison UART Bafang. La broche TX du port logiciel n'est pas câblée | Contrôle visuel du câblage + revue de code |
| F-5.2 | S | Les trames contrôleur → afficheur sont segmentées par silence inter-trame (> 30 ms) et validées par somme de contrôle | Mode apprentissage, comptage des trames rejetées : < 5 % |
| F-5.3 | S | Vitesse, courant et niveau de charge sont extraits quand les trames correspondantes sont reconnues | Comparaison avec l'afficheur d'origine : écart < 1 km/h entre 5 et 30 km/h |
| F-5.4 | S | `BAFANG_LEARN_MODE` dumpe en hexadécimal toutes les trames valides, pour calibrer le décodage sur le matériel réel | Sortie série lisible |
| F-5.5 | M | Sans trame valide pendant 2 s, la télémétrie est marquée invalide. **Aucune autre fonction n'est dégradée** | Débrancher l'afficheur en roulant : feux et freins inchangés |
| F-5.6 | C | Source de vitesse alternative : lecture directe du capteur de roue (`SPEED_SOURCE_WHEEL`) | Compilation avec l'option |

## F-6 — Diagnostic et sécurité

| Id | Prio | Exigence | Vérification |
|---|---|---|---|
| F-6.1 | M | Un chien de garde matériel (WDT 1 s) redémarre l'Arduino en cas de blocage | Boucle infinie injectée en test (T1.7) |
| F-6.2 | M | Le WDT est explicitement désarmé au tout début de `setup()` (`MCUSR = 0; wdt_disable();`) pour éviter le redémarrage en boucle avec les anciens bootloaders | Revue de code |
| F-6.3 | S | Un autotest au démarrage active chaque sortie 200 ms dans l'ordre : les six relais d'éclairage et de signalisation, **puis le voyant de défaut**. Seule la coupure moteur (R8) est exclue | Observation visuelle et **auditive** — le claquement est le seul contrôle des relais perceptible coque fermée |
| F-6.4 | S | Le **point décimal du digit de gauche** bat à 1 Hz en fonctionnement nominal, à ~4 Hz si un défaut est actif. L'afficheur n'a pas de deux-points, et la LED D13 n'est pas utilisable : elle porte le TX logiciel du Bafang, maintenu haut au repos | Observation, coque ouverte |
| F-6.5 | S | Un journal série (115 200 bauds) publie l'état consolidé toutes les secondes, désactivable à la compilation. L'inverseur de la carte doit être sur **`PRO`** | Terminal série |
| F-6.6 | M | Le temps de cycle maximal observé est journalisé ; il doit rester < 10 ms | Champ `loop=…/…us` du journal |

## F-7 — Contraintes non fonctionnelles

| Id | Prio | Exigence |
|---|---|---|
| NF-1 | M | Empreinte flash < 24 ko et RAM statique < 1,4 ko. **Mesuré : 9 692 o de flash (31 %) et 783 o de RAM (38 %)** sur les 30 720 / 2 048 utilisables |
| NF-2 | M | Aucun `String`, aucune allocation dynamique |
| NF-3 | M | Tout le brochage est concentré dans `src/pins.h` et les tableaux en tête de `src/board_io.cpp` ; tout le réglage dans `src/config.h`. C'est ce qui a permis d'absorber un changement complet d'architecture de carte sans toucher un module métier |
| NF-4 | S | Chaque fonction est un module indépendant, testable en isolant ses entrées |
| NF-5 | M | Le firmware compile **sans avertissement** avec `--warnings all`, dans **toutes** les combinaisons d'options de `config.h` — `tools/check-variants.sh`, 15 variantes |
