# 08 — Plan de tests

Trois campagnes, dans cet ordre. **Ne pas passer à la suivante avant d'avoir
tout validé dans la précédente.**

---

## Campagne 1 — Établi, sans le véhicule

Matériel : Nano + DN22D08, alimentation de labo 12 V limitée à 1 A, huit LED +
résistances en guise de charges, huit interrupteurs vers +12 V, un multimètre.

### T1.1 — Brochage (bloquant)

Suivre `03-affectation-es.md` §6 avec `tools/pinscan`.

**Critères**, tous obligatoires :
- un digit s'allume sur l'afficheur → la chaîne de registres répond ;
- les relais collent **un par un**, dans l'ordre annoncé sur la console →
  `RELAY_BIT[]` est correct ;
- au repos, la console affiche `IN1..IN8 = 11111111` ;
- chaque borne d'entrée sollicitée fait passer **un seul** chiffre à `0` ;
- les quatre boutons répondent.

Tout écart se corrige dans `pins.h` et `board_io.cpp` avant de continuer.

### T1.2 — Autotest

Mise sous tension.
**Critère** : les six relais d'éclairage et de signalisation collent l'un après
l'autre, 200 ms chacun, **puis le voyant de défaut (R7)** — c'est audible
autant que visible. Seule la coupure moteur (R8) **n'est pas** activée.
L'afficheur reste multiplexé pendant toute la séquence : s'il s'éteint,
`board::refresh()` n'est pas appelé dans la boucle d'attente.

### T1.3 — Éclairage

| Action | Attendu |
|---|---|
| `IN_PARK` seule (veilleuse) | R1 collé, **R5 collé** |
| `IN_MAIN` seule (comodo, veilleuse ouverte) | R2 collé, R1 collé (`MAIN_KEEPS_PARK`), **R5 collé** — c'est le test de F-1.7 |
| `IN_PARK` + `IN_MAIN` | R1, R2, R5 collés |
| Relâcher tout | R1, R2, R5 relâchés |

> La deuxième ligne est la plus importante du tableau : elle vérifie qu'aucune
> combinaison de commandes ne permet de rouler éclairé à l'avant sans feu
> rouge arrière.

### T1.4 — Clignotants

| Action | Attendu |
|---|---|
| `IN_TURN_LEFT` | R3 claque à 1,33 Hz, R4 muet, démarrage sur une phase **allumée** |
| Chronométrer 30 cycles | 22,5 s ± 1 s (750 ms/cycle) |
| `IN_TURN_LEFT` + `IN_TURN_RIGHT` | R3 **et** R4 relâchés, `FLT_TURN_CONFLICT` au journal |
| `IN_HAZARD` + `IN_TURN_LEFT` | R3 et R4 claquent **en phase** |
| Laisser `IN_TURN_LEFT` 50 s | Le rappel apparaît au journal (`TRN=L!`) **et le rythme du claquement change** : de régulier à syncopé |
| Chronométrer 10 cycles pendant le rappel | 7,5 s ± 0,5 s — la **période est inchangée**, seule la phase allumée est raccourcie |

Le claquement des relais est le retour sonore du clignotant : il doit être
audible depuis le poste de conduite. Si le boîtier l'étouffe complètement,
c'est un élément de réponse à Q12.

### T1.5 — Freinage

| Action | Attendu |
|---|---|
| `IN_BRAKE_FRONT`, éclairage éteint | R6 collé, R5 relâché, R8 collé |
| `IN_BRAKE_REAR` (S2), éclairage allumé | R6 collé **en plus** de R5 |
| Relâcher | R6 retombe immédiatement ; R8 reste collé 300 ms |
| Impulsions à 20 Hz sur l'entrée | R8 ne bat pas (le maintien de 300 ms absorbe) |
| Maintenir 130 s | `FLT_BRAKE_STUCK` au journal |

Mesurer le délai contact → R6 collé : **< 50 ms** (F-2.2), temps de collage du
relais inclus.

> R5 et R6 sont deux circuits distincts. Vérifier au contrôleur de continuité
> qu'ils ne se rebouclent pas : le feu stop doit pouvoir s'allumer feux
> éteints, et les feux de position rester allumés hors freinage.

### T1.6 — Voyant de défaut et acquittement

| Action | Attendu |
|---|---|
| Mise sous tension | R7 colle 200 ms en fin d'autotest, puis retombe. **C'est le contrôle de la LED elle-même** |
| Forcer `IN_TURN_LEFT` **et** `IN_TURN_RIGHT` | R7 colle et **reste collé** — pas de clignotement |
| Appuyer sur le bouton d'acquittement (IN3) | R7 retombe. Au journal : `flt=0x01 ack` — le défaut est **toujours** signalé |
| Relâcher les entrées, puis refaire le conflit | R7 se **rallume** : l'acquittement portait sur l'événement passé |
| **Débrancher la source Bafang** | `FLT_BAFANG_LINK` au journal (`flt=0x04`), R7 **reste éteint** — c'est le principe P2, et c'est le critère le plus important de ce test |
| Injecter un reset chien de garde, puis acquitter | `FLT_WDT_RESET` disparaît du journal, sans coupure d'alimentation |
| Appuyer sur IN3 pendant que tout fonctionne | **Aucune sortie ne bouge** : ni feux, ni clignotants, ni R8 |

> La ligne « bus Bafang » est celle qu'il ne faut pas rater. Si le voyant
> s'allume quand l'afficheur d'origine est débranché, `FAULT_LAMP_MASK` a été
> modifié : le voyant serait allumé en permanence sur un véhicule dont la
> télémétrie n'est pas branchée, et ne voudrait plus rien dire. Le
> `static_assert` de `diag.cpp` est censé rendre cette erreur impossible.

Compter les manœuvres de R7 sur un trajet complet : elles doivent se compter
sur les doigts d'une main. Un relais qui bat signale un défaut intermittent,
pas un voyant qui fonctionne.

### T1.7 — Chien de garde

Injecter temporairement `while(1);` dans `loop()`.
**Critère** : redémarrage en ~1 s, et `FLT_WDT_RESET` présent au journal après
reprise. **Retirer l'injection ensuite.**

### T1.7 bis — Arrêt d'urgence par la broche OE

Injecter temporairement un appel à `board::outputsEnabled(false)` déclenché par
un bouton de la carte.
**Critère** : les huit relais retombent **immédiatement**, et l'état antérieur
est intégralement restitué au relâchement — sans que le firmware ait eu à
recalculer quoi que ce soit. **Retirer l'injection ensuite.**

### T1.8 — Temps de cycle

Lire `loopMax` au journal après 5 min.
**Critère** : < 10 ms (F-6.6). Attention, avec `BAFANG_ENABLE 1` et une source
UART branchée, des pics à ~9 ms sont normaux (`SoftwareSerial`).

Observer aussi l'afficheur : un léger scintillement toutes les ~200 ms est
attendu et sans gravité, c'est la trace des octets Bafang. Un scintillement
permanent signalerait en revanche une boucle trop lente.

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
3. Brancher la carte DN22D08 seule. Contrôler la consommation contre les
   valeurs du constructeur : **12 mA** en veille afficheur éteint, **48 mA**
   afficheur allumé, environ **+30 mA par relais collé**, **288 mA** les huit
   collés. Un écart franc signale un relais collé mécaniquement ou une charge
   parasite.
4. Brancher les charges d'éclairage une par une, en contrôlant le courant.

### T2.3 — Rejouer T1.3 à T1.6 avec les vraies charges

Mêmes critères. Ajouter :

- **le contrôle du courant total** : phares + veilleuse + arrière allumés,
  mesurer à la pince sur l'entrée du boîtier fusibles. Attendu **≈ 5 A**
  (phares mesurés à 12 W pièce). Au-delà de 6 A, reprendre
  `04-electricite.md` §2.2.

Le klaxon étant autonome, il ne figure plus dans ces essais.

### T2.4 — Coupure moteur : ce qui coupe, et ce qui ne coupe pas

Roue en l'air, assistance engagée à faible niveau :

| Test | Attendu |
|---|---|
| Actionner le frein arrière | La roue s'arrête d'être entraînée |
| Actionner le frein avant | Idem |
| **Débrancher l'Arduino**, actionner le frein **arrière** | La roue s'arrête **quand même**. Si ce test échoue, le connecteur Bafang d'origine a été altéré — **reprendre le câblage avant toute mise sur route** |
| **Arduino débranché**, actionner le frein **avant** | La roue s'arrête **quand même**, par la diode D1. Si elle continue : D1 est absente, montée à l'envers, ou la ligne frein n'est pas active à l'état bas (`hardware/cablage.md` §4) |
| Rebrancher, débrancher le seul fil de `OUT_MOTOR_CUT` | Les deux freins coupent toujours |

> Les deux tests « Arduino débranché » sont **le** test du principe P1. Tant
> qu'ils ne passent pas tous les deux, ne pas rouler sur route ouverte.
> Mesurer aussi, au multimètre, la tension de la ligne frein au repos avec
> l'Arduino débranché : elle doit être **inchangée** par rapport à la mesure
> faite avant de monter D1. Une tension qui aurait monté signale une diode en
> court-circuit ou montée à l'envers.

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

- [ ] Veilleuse avant, phares, **et feu rouge arrière dans les deux cas**
- [ ] Clignotant gauche, clignotant droit, détresse
- [ ] Feu stop au frein avant **et** au frein arrière (observateur derrière)
- [ ] Klaxon *(autonome — vérifier sa propre batterie)*
- [ ] **Le voyant rouge s'est allumé puis éteint à la mise sous tension.**
      S'il ne s'est pas allumé, la LED ou son câblage sont morts et il n'y
      aura aucune alerte de tout le trajet
- [ ] Le voyant rouge est éteint au départ
- [ ] Aucun défaut : à l'ouverture de la coque, l'afficheur montre `F000`
      (c'est la page par défaut) et le point décimal de gauche bat à 1 Hz, pas ~4 Hz
- [ ] Autotest au démarrage : les six relais claquent l'un après l'autre.
      C'est le seul contrôle des relais audible coque fermée

### T3.2 — Comportement dynamique

| Situation | Attendu |
|---|---|
| Freinage franc à 25 km/h | Assistance coupée immédiatement, stop visible |
| Freinage modulé (pompage du levier) | Le stop suit, l'assistance ne se réengage pas par à-coups |
| Clignotant maintenu 300 m | Le rythme du claquement devient syncopé, audible du poste de conduite |
| Passage sur pavés / vibrations | Aucun scintillement de feu, aucun déclenchement parasite |
| 30 min de roulage continu | `loopMax` < 10 ms, aucun reset WDT, **voyant resté éteint** |

### T3.3 — Visibilité (à valider par l'observateur, à 30 m)

- [ ] Le feu stop est nettement distinguable de la veilleuse
- [ ] Les clignotants sont visibles de jour
- [ ] Les phares n'éblouissent pas

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
| T1.7 bis | | | |
| T1.8 | | | |
| T2.1 | | | |
| T2.2 | | | |
| T2.3 | | | |
| T2.4 | | | |
| T2.5 | | | |
| T3.1 | | | |
| T3.2 | | | |
| T3.3 | | | |
