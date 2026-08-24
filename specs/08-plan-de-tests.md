# 08 — Plan de tests

Trois campagnes, dans cet ordre. **Ne pas passer à la suivante avant d'avoir
tout validé dans la précédente.**

---

## Campagne 1 — Établi, sans le véhicule

Matériel : Nano + DN22D08, alimentation de labo 12 V limitée à 1 A, huit LED +
résistances en guise de charges, huit interrupteurs vers +12 V, un multimètre.

### T1.1 — Brochage (bloquant)

Suivre `03-affectation-es.md` §5 avec `tools/pinscan`.
**Critère** : les 16 correspondances sont notées et concordent avec `pins.h`,
ou `pins.h` a été corrigé.

### T1.2 — Autotest

Mise sous tension.
**Critère** : les six sorties d'éclairage/signalisation s'activent l'une après
l'autre, 150 ms chacune. Le klaxon et la coupure moteur **ne** sont **pas**
activés pendant l'autotest.

### T1.3 — Éclairage

| Action | Attendu |
|---|---|
| `IN_LOWBEAM` seule | OUT1 actif, OUT5 à ~20 % |
| `IN_HIGHBEAM` seule (sans croisement) | OUT2 **inactif** |
| `IN_LOWBEAM` + `IN_HIGHBEAM` | OUT1 et OUT2 actifs |
| Relâcher tout | OUT1, OUT2, OUT5 inactifs |

Mesurer le rapport cyclique de OUT5 à l'oscilloscope ou avec un multimètre en
mode tension moyenne : ≈ 2,4 V sur 12 V.

### T1.4 — Clignotants

| Action | Attendu |
|---|---|
| `IN_TURN_LEFT` | OUT3 clignote, OUT4 éteint, démarrage **allumé** |
| Chronométrer 30 cycles | 22,5 s ± 1 s (750 ms/cycle) |
| `IN_TURN_LEFT` + `IN_TURN_RIGHT` | OUT3 **et** OUT4 éteints, `FLT_TURN_CONFLICT` au journal |
| `IN_HAZARD` + `IN_TURN_LEFT` | OUT3 et OUT4 clignotent **en phase** |
| Laisser `IN_TURN_LEFT` 50 s | Le retour sonore passe en bip long |

### T1.5 — Freinage

| Action | Attendu |
|---|---|
| `IN_BRAKE_FRONT`, éclairage éteint | OUT5 à 100 %, OUT7 actif |
| `IN_BRAKE_REAR`, éclairage allumé | OUT5 passe de 20 % à 100 % |
| Relâcher | OUT5 revient à son état d'éclairage **immédiatement** ; OUT7 reste actif 300 ms |
| Impulsions à 20 Hz sur l'entrée | Pas de scintillement de OUT7 (le maintien absorbe) |
| Maintenir 130 s | `FLT_BRAKE_STUCK` au journal |

Mesurer le délai contact → OUT5 à 100 % : **< 50 ms** (F-2.2).

### T1.6 — Klaxon

| Action | Attendu |
|---|---|
| Appui bref | OUT6 actif pendant l'appui |
| Appui maintenu 15 s | OUT6 coupe à 10 s, `FLT_HORN_STUCK` |
| Relâcher puis rappuyer | OUT6 réactif |

### T1.7 — Chien de garde

Injecter temporairement `while(1);` dans `loop()`.
**Critère** : redémarrage en ~1 s, et `FLT_WDT_RESET` présent au journal après
reprise. **Retirer l'injection ensuite.**

### T1.8 — Temps de cycle

Lire `loopMax` au journal après 5 min.
**Critère** : < 10 ms (F-6.6). Attention, avec `BAFANG_ENABLE 1` et une source
UART branchée, des pics à ~9 ms sont normaux (`SoftwareSerial`).

---

## Campagne 2 — Sur le véhicule, roue en l'air

Véhicule sur béquille, roue motrice libre, **personne devant la roue**.

### T2.1 — Câblage de puissance (bloquant, avant mise sous tension)

- Continuité et polarité de chaque départ 12 V.
- Chaque fusible est au calibre du tableau `04-electricite.md` §3.
- **Aucun fusible à lame automobile sur le bus 48 V.**
- Sectionneur 48 V en position ouverte.
- Isolation : mesurer > 1 MΩ entre le + 48 V et le châssis.

### T2.2 — Mise sous tension progressive

1. Fermer le sectionneur 48 V, contrôleur moteur **débranché**.
2. Vérifier 12,0–13,8 V en sortie de convertisseur, à vide.
3. Brancher la carte DN22D08 seule. Contrôler la consommation : < 300 mA.
4. Brancher les charges d'éclairage une par une, en contrôlant le courant.

### T2.3 — Rejouer T1.3 à T1.6 avec les vraies charges

Mêmes critères. Ajouter le contrôle de l'échauffement des modules MOSFET et du
relais klaxon après 2 min de fonctionnement continu.

### T2.4 — Coupure moteur, les trois barrières

Roue en l'air, assistance engagée à faible niveau :

| Test | Attendu |
|---|---|
| Actionner le frein arrière | La roue s'arrête d'être entraînée |
| Actionner le frein avant | Idem |
| **Débrancher l'Arduino**, actionner chaque frein | La roue s'arrête **quand même** — c'est le test critique du principe P1 |
| Rebrancher, débrancher le fil de `OUT_MOTOR_CUT` | La roue s'arrête quand même |

Si le troisième test échoue, **le câblage des freins est à reprendre** avant
toute mise sur route.

### T2.5 — Écoute Bafang

Avec `BAFANG_LEARN_MODE 1`, suivre la procédure de calibration de
`05-protocole-bafang.md` §6.

| Critère | Seuil |
|---|---|
| Trames valides reçues | > 3 par seconde |
| Taux de rejet somme de contrôle | < 5 % |
| Vitesse décodée vs afficheur d'origine | écart < 1 km/h entre 5 et 30 km/h |
| Débrancher l'afficheur | `FLT_BAFANG_LINK` après 2 s, **aucun** effet sur les feux |

---

## Campagne 3 — Route, en conditions réelles

À faire de jour, sur voie privée ou peu fréquentée, avec un observateur.

### T3.1 — Contrôle avant départ (à répéter avant chaque sortie ensuite)

- [ ] Croisement, route, veilleuse arrière
- [ ] Clignotant gauche, clignotant droit, détresse
- [ ] Feu stop au frein avant **et** au frein arrière (observateur derrière)
- [ ] Klaxon
- [ ] Aucun défaut au journal série (ou LED D13 à 1 Hz, pas 5 Hz)

### T3.2 — Comportement dynamique

| Situation | Attendu |
|---|---|
| Freinage franc à 25 km/h | Assistance coupée immédiatement, stop visible |
| Freinage modulé (pompage du levier) | Le stop suit, l'assistance ne se réengage pas par à-coups |
| Clignotant maintenu 300 m | Rappel sonore |
| Passage sur pavés / vibrations | Aucun scintillement de feu, aucun déclenchement parasite |
| 30 min de roulage continu | `loopMax` < 10 ms, aucun reset WDT |

### T3.3 — Visibilité (à valider par l'observateur, à 30 m)

- [ ] Le feu stop est nettement distinguable de la veilleuse
- [ ] Les clignotants sont visibles de jour
- [ ] Le feu de croisement n'éblouit pas

---

## Journal de recette

| Test | Date | Résultat | Observations |
|---|---|---|---|
| T1.1 | | | |
| T1.2 | | | |
| T1.3 | | | |
| T1.4 | | | |
| T1.5 | | | |
| T1.6 | | | |
| T1.7 | | | |
| T1.8 | | | |
| T2.1 | | | |
| T2.2 | | | |
| T2.3 | | | |
| T2.4 | | | |
| T2.5 | | | |
| T3.1 | | | |
| T3.2 | | | |
| T3.3 | | | |
