# 08 — Plan de tests

Trois campagnes, dans cet ordre. **Ne pas passer à la suivante avant d'avoir
tout validé dans la précédente.**

---

## Campagne 1 — Établi, sans le véhicule

Matériel : Nano + DN22D08, alimentation de labo 12 V limitée à 1 A, huit LED +
résistances en guise de charges, **un fil volant vers la masse** (les entrées
sont NPN : un contact vers GND, jamais vers le +12 V), un multimètre.

> **Le 12 V est obligatoire.** Sans lui, aucun relais ne colle — les bobines
> sont en 12 V — et aucune entrée ne bouge, la LED de chaque optocoupleur étant
> alimentée depuis le +12 V de la carte. L'afficheur, lui, s'allume sur l'USB
> seul : c'est le discriminant (`03-affectation-es.md` §6).

### Comment solliciter les entrées avant que le faisceau existe

Deux méthodes, et elles cohabitent dans **la même compilation** :

| | Fil volant vers GND | Console série (`SIM_INPUTS`) |
|---|---|---|
| Ce que ça exerce | Le chemin complet : borne, optocoupleur, broche, firmware | Le firmware seul, à partir de l'anti-rebond |
| Combinaisons | Difficiles — trois mains pour détresse + frein + veilleuse | Immédiates, l'état est mémorisé |
| Chronométrages | Peu commodes | Précis et reproductibles |
| Ce que ça ne prouve pas | — | **Rien du câblage** |

Le firmware de banc se compile à part, et le fichier versionné reste à `0` :

```bash
VHELIO_SIM=1 ./tools/build-nix.sh
VHELIO_SIM=1 ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Une touche prend la borne et **ferme** le contact ; la même touche le rouvre.
Une entrée que la console n'a pas réquisitionnée continue d'être lue sur son
optocoupleur — c'est ce qui permet de mener les deux méthodes de front.

| Touche | Entrée | | Touche | Entrée |
|---|---|---|---|---|
| `1` ou `g` | IN1 clignotant gauche | | `5` ou `r` | IN5 frein arrière |
| `2` ou `d` | IN2 clignotant droit | | `6` ou `v` | IN6 veilleuse |
| `3` ou `q` | IN3 acquittement | | `7` ou `p` | IN7 phares |
| `4` ou `a` | IN4 frein avant | | `8` ou `w` | IN8 détresse |

`0` relâche les huit bornes en les gardant sous simulation, `x` les rend toutes
au matériel, `?` rappelle l'aide. `#0` à `#8` affiche un **numéro de point de
contrôle** sur le digit de droite, en éteignant l'écran deux secondes pour
marquer le changement.

`tools/seq-banc.sh` enchaîne T1.2 à T1.6 tout seul — trois minutes — en
**annonçant horodaté ce qui doit s'entendre** et en numérotant les points sur
l'afficheur. Il journalise la console dans `.build/seq-banc.log` :

```bash
./tools/seq-banc.sh /dev/ttyACM0            # tout
./tools/seq-banc.sh /dev/ttyACM0 freins     # une phase seule
```

Il ne vérifie rien de lui-même, et ne le peut pas : **la chaîne de registres ne
se relit pas**, le seul témoin de l'état d'un relais est son claquement. C'est
un métronome pour l'opérateur. Le journal dit ce que le *firmware* a décidé, le
claquement dit ce que la *carte* a fait — il faut les deux.

> **Ce firmware ne doit jamais rouler.** Un caractère parasite sur la ligne
> série fermerait un contact de frein ou allumerait une détresse. Trois choses
> le rappellent : un `#warning` à la compilation, une bannière au démarrage, et
> la colonne `sim=` que porte **chaque** ligne du journal. Le binaire de route
> est dans `.build/vhelio`, celui de banc dans `.build/vhelio-sim` : ils ne se
> mélangent pas.
>
> **Chaque test doit finir par une passe au fil volant.** La simulation ne
> prouve rien du câblage — c'est précisément pour cela qu'elle est utile : un
> écart entre les deux méthodes désigne le matériel, jamais la logique.

### Ce que la simulation a déjà établi — 25/08/2026, carte sur USB

Premier passage du firmware de banc, **sans le 12 V** : les relais ne collent
donc pas et rien n'a été jugé à l'oreille. Seules les décisions du firmware,
lues au journal, sont validées ici.

| Sollicitation | Observé | Couvre |
|---|---|---|
| `v` veilleuse | `VL=1 AR=1` | T1.3 ligne 1 |
| `p` phares, veilleuse déjà fermée | `PH=1 VL=1 AR=1` | T1.3 ligne 2 — `MAIN_KEEPS_PARK`, et **F-1.7** |
| `a` frein avant | `ST=1 CUT=1` | T1.5 ligne 1 |
| `g` puis `d` | `TRN=-`, `flt=0x05 LAMP` | T1.4 ligne 3 **et** T1.6 ligne 2 : conflit, les deux clignotants éteints, voyant allumé |
| `d` relâché | `TRN=L`, `flt=0x04`, plus de `LAMP` | Le conflit disparaît de lui-même |
| `w` détresse | `TRN=H` | T1.4 ligne 4 |
| `g` maintenu | `TRN=L!` **au 45ᵉ tour de journal** | T1.4 ligne 5 : `BLINK_REMINDER_MS` respecté à la seconde près |
| Aucune source Bafang | `flt=0x04` en permanence, **jamais** `LAMP` | T1.6 ligne 5 — le principe P2, le critère le plus important de ce test |
| Boucle à vide | `loop=265 µs`, pointe **6,7 ms** sur la ligne de journal | T1.8, marge confortable sous les 10 ms |

**Ce que cela ne prouve pas** : aucun optocoupleur, aucune bobine, aucun
contact n'a été traversé. Tout le tableau reste à rejouer au fil volant, carte
alimentée en 12 V — c'est là que se jouent les chronométrages de cadence, le
maintien de 300 ms sur R8 et le délai des 50 ms du feu stop.

### T1.1 — Brochage (bloquant) — **fait**

Le brochage supposé était faux ; il a fallu le découvrir au lieu de le
vérifier. Quatre campagnes de mesure, détaillées en `03-affectation-es.md`
§6 bis (entrées, boutons), §6 ter (afficheur) et §6 quater (glyphes et
multiplexage). Résultat consigné dans `pins.h` et `board_io.cpp` : **plus rien
n'y est supposé**.

**Critères**, tous obligatoires — à rejouer si la carte est remplacée :
- un digit s'allume sur l'afficheur → la chaîne de registres répond ;
- les relais collent **un par un**, dans l'ordre annoncé sur la console →
  `RELAY_BIT[]` est correct ;
- au repos, la console affiche `IN1..IN8 = 11111111` ;
- chaque borne d'entrée sollicitée fait passer **un seul** chiffre à `0` ;
- les quatre boutons répondent.

Tout écart se corrige dans `pins.h` et `board_io.cpp` avant de continuer.

### T1.2 — Autotest

Mise sous tension.

**Critère 1** : les six relais d'éclairage et de signalisation collent l'un
après l'autre, 200 ms chacun, **puis le voyant de défaut (R7)** — c'est audible
autant que visible. Seule la coupure moteur (R8) **n'est pas** activée.

**Critère 2** : **tous les segments et tous les points décimaux sont allumés**
pendant les deux premières secondes, les quatre digits compris. C'est le seul
moment où un segment mort se voit : en usage normal il ne manquerait qu'un
morceau de caractère, ce qui se lit comme un *autre* caractère plutôt que comme
une panne. Le test se superpose aux 1,4 s de l'autotest des relais, la carte ne
met donc que 600 ms de plus à démarrer.

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

Lire `loopMax` au journal après 5 min. **Avec le binaire de route**, pas celui
de banc : afficher l'aide (`?`) tient la boucle ~19 ms — quatre lignes à écouler
dans un tampon série de 64 octets — ce qui gonfle `loopMax` et mémorise un
`FLT_LOOP_SLOW` sans rapport avec le firmware embarqué. L'acquittement (touche
`3`) l'efface. Mesuré à vide : **~265 µs par tour, pointe à 6,7 ms** sur la
ligne de journal elle-même, qui est de loin le plus long traitement du cycle.

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
- [ ] Aucun défaut : à l'ouverture de la coque, l'afficheur montre `---0`
      (c'est la page par défaut) et le point décimal de gauche bat à 1 Hz, pas
      ~4 Hz. Trois tirets, pas `Err` : au bouton K1, la page défauts doit
      montrer `F000`
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

Une case n'est **OK** que si le test a été mené carte alimentée en 12 V, feux
ou LED témoins branchés. « Simulé » veut dire que la logique est vérifiée et
que le matériel ne l'est pas.

| Test | Date | Résultat | Observations |
|---|---|---|---|
| T1.1 | 25/08/2026 | **OK** | Brochage entièrement mesuré, pas confirmé : `pinfind`, `pinchain`, `pindisp`, `dispcheck`. Voir `03` §6 bis à §6 quater |
| T1.2 | | | |
| T1.3 | 25/08/2026 | simulé | Lignes 1 et 2 OK au journal, dont F-1.7. Relais non entendus, 12 V absent |
| T1.4 | 25/08/2026 | simulé | Conflit, détresse et rappel à 45 s OK. **Cadence non chronométrée** |
| T1.5 | 25/08/2026 | simulé | Frein avant → `ST=1 CUT=1`. Maintien de 300 ms et délai de 50 ms non mesurés |
| T1.6 | 25/08/2026 | simulé | Voyant sur conflit, **pas** sur perte du bus Bafang (P2). Acquittement non rejoué |
| T1.7 | | | |
| T1.7 bis | | | |
| T1.8 | 25/08/2026 | simulé | 265 µs par tour, pointe 6,7 ms. À reprendre 5 min avec le binaire de route |
| T2.1 | | | |
| T2.2 | | | |
| T2.3 | | | |
| T2.4 | | | |
| T2.5 | | | |
| T3.1 | | | |
| T3.2 | | | |
| T3.3 | | | |
