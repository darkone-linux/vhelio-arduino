# Archive — la découverte du brochage de la DN22D08

> Extrait de `specs/03-affectation-es.md`, sorti de la spécification active le
> 25/08/2026. **Le résultat de ces quatre campagnes est en production** dans
> `firmware/vhelio/src/pins.h` et `src/board_io.cpp`, et résumé dans
> `specs/03-affectation-es.md` §2.
>
> Ce qui est conservé ici, c'est la **méthode** : elle sert si la carte est
> remplacée par un exemplaire au brochage différent, et le post-mortem du §7
> explique pourquoi la correction n'a coûté que deux fichiers.

## 6 bis. Quand le pinscan ne montre rien — découvrir le brochage

Symptôme : Nano bien enfiché, carte alimentée, 5 V présent au support, et
pourtant **ni bouton, ni afficheur, ni relais**. Les poussoirs et l'afficheur
n'ont rien en commun sauf la liaison au Nano ; qu'ils soient morts tous les
deux, alors que la liaison est bonne, ne laisse qu'une explication : **le
brochage supposé est faux**.

C'est le §7 qui se répète un cran plus bas. Le brochage de `pins.h` vient de
`af3556/IO22_IO_Board`, bibliothèque qui déclare ne couvrir que l'**IO22D08**
et l'IO22C04. La **DN22D08** est un produit voisin mais distinct — rail DIN,
RS485, 12/24 V — et rien ne garantit qu'elle partage le brochage de l'IO22D08.

`tools/pinscan` ne sait que **vérifier** un brochage supposé ; il ne peut donc
rien dire quand c'est l'hypothèse elle-même qui est fausse.
`tools/pinfind` le **découvre**.

### Phase A — les entrées et les boutons

```bash
VHELIO_SKETCH=$PWD/tools/pinfind ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pinfind ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Toutes les broches utilisables passent en `INPUT_PULLUP` et **aucune n'est
pilotée en sortie** : aucun conflit possible avec ce que la carte impose, quel
que soit son brochage réel. Ce croquis ne peut rien abîmer.

1. Appuyer sur les poussoirs, un par un → donne `BTN_PIN[]`. Ne demande pas le
   12 V, les poussoirs sont câblés directement au Nano.
2. Carte alimentée, relier chaque borne d'entrée à la masse → donne `IN_PIN[]`,
   et confirme la polarité NPN au passage.

> `D13` lit `0` en permanence et ce **n'est pas** un signal de la carte : la
> LED intégrée du Nano charge le tirage interne, qui ne fait que ~30 kΩ. Vrai
> sur n'importe quel Arduino, carte ou pas.

Les broches qui ne bougent **jamais** sont les candidates de la chaîne de
registres. C'est ce que la phase A produit de plus utile : elle réduit
l'espace de recherche de la phase B à quatre ou cinq broches.

### Résultats mesurés sur l'exemplaire

**Boutons** — mesuré, la supposition était fausse :

| | Supposé d'après l'IO22D08 | **Mesuré sur la DN22D08** |
|---|---|---|
| Poussoirs | D7, D8, D9, D10 | **D8, D10, D12, A0** |

`A0` valant 14 sur un Nano, c'est **8, 10, 12, 14** : une progression de deux
en deux. Les broches impaires intercalées — `D9, D11, D13, A1` — sont donc les
candidates naturelles des quatre lignes de la chaîne (données, horloge,
verrou, OE). Hypothèse, pas conclusion : elle sert seulement à **ordonner** la
recherche de la phase B, pas à la remplacer.

**Entrées** — mesuré, carte alimentée. Elles réagissent **à la masse** :
la polarité NPN annoncée par le constructeur est confirmée, tous les communs
du faisceau vont donc à GND.

| | Supposé d'après l'IO22D08 | **Mesuré sur la DN22D08** |
|---|---|---|
| IN1 … IN5 | D2, D3, D4, D5, D6 | identiques |
| IN6 | A0 | **D7** |
| IN7 | D12 | **D9** |
| IN8 | D11 | identique |

**Il reste donc six broches, et six seulement**, pour huit relais et un
afficheur quatre digits : `D13, A1, A2, A3, A4, A5`. L'argument arithmétique
du §1 s'en trouve confirmé par la mesure.

L'hypothèse des « broches impaires » formée à partir des boutons est morte :
D9 et D11 sont des entrées. C'est précisément pourquoi on mesure.

### Phase B — la chaîne de registres

Aucun brochage de la DN22D08 n'est publié : la fiche du constructeur donne les
caractéristiques mais pas le câblage, et précise ne fournir « ni code
supplémentaire ni support technique ». Le balayage est donc la seule voie.

```bash
VHELIO_SKETCH=$PWD/tools/pinchain ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pinchain ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Le balayage est **manuel**, et c'est tout l'intérêt. Une première version
enchaînait les 240 essais toute seule, à la seconde : inutilisable. Plusieurs
triplets produisent un effet partiel — décaler des bits avec l'horloge et les
données interverties fait quand même bouger quelque chose — si bien que la
console défile trop vite pour qu'on note quoi que ce soit.

Les boutons de la carte, que la phase A vient d'identifier, règlent le
problème. Le montage reste dans l'état choisi indéfiniment.

| Bouton | Effet |
|---|---|
| K1 (D12) | Triplet suivant |
| K2 (D10) | Triplet précédent |
| K3 (D8) | Niveau des trois autres broches, `BAS` ↔ `HAUT` — couvre une validation OE parmi elles |
| K4 (A0) | Motif, `TOUS` ↔ `BIT A BIT` |

**Deux motifs, et le second est le juge.**

`TOUS` alterne `0xFF` et `0x00` : fort, repérable de loin, bon pour dégrossir.
Mais il ne prouve rien — un triplet faux fait souvent claquer quelque chose.

`BIT A BIT` promène **un seul 1** à travers les 24 bits. Sur le bon triplet,
et sur lui seul, on entend **exactement un relais à la fois**, proprement,
huit fois sur vingt-quatre positions. C'est ce qui distingue le triplet
correct de ceux qui font seulement du bruit.

Et il donne gratuitement `RELAY_BIT[]` : la position annoncée quand tel relais
claque **est** son bit dans la chaîne. C'est l'étape suivante faite d'avance.

Noter le **numéro du triplet**, le niveau des autres broches, et la position
de chacun des huit relais. Puis reporter dans `pins.h` et `board_io.cpp`.

#### Résultat mesuré

**Triplet 120 sur 120** : `données = A5`, `horloge = A4`, `verrou = A3`, les
trois autres broches à l'état **bas**.

La signature ne laisse pas de place au doute. En motif `BIT A BIT` : un relais
à la fois sur les huit premières positions, puis les segments de l'afficheur.
Les triplets faux, eux, faisaient bien claquer les relais — mais tous ensemble,
au rythme de l'horloge. Du bruit, pas de l'ordre. C'est exactement la
distinction que le motif `BIT A BIT` avait été ajouté pour trancher.

L'octet des relais est donc le **dernier** émis des trois.

**Les deux inconnues restantes ont été levées** par `tools/pinscan` :

| Question | Résultat mesuré |
|---|---|
| Correspondance bit ↔ relais | **Directe** : bit 0 → CH1 … bit 7 → CH8 |
| Emplacement de l'OE | **A2**, active à l'état bas. D13 et A1 sont sans effet sur les relais |

Le brochage de la DN22D08 est donc **entièrement mesuré**, à l'exception de
l'afficheur.

Le brochage de l'**afficheur** — sélection des digits, ordre des segments —
fait l'objet du §6 ter.

> **Le faisceau ne doit pas être câblé.** En motif `TOUS`, huit circuits sont
> fermés en même temps, ce qui n'est un régime prévu nulle part. À vide, les
> huit bobines représentent ≈ 300 mA sur le 12 V, que la carte encaisse sans
> difficulté.

Piloter ces six broches en sortie n'est sans risque que parce que la phase A a
prouvé qu'aucune ne porte d'entrée optocouplée ni de poussoir — donc qu'aucun
organe de la carte ne cherche à leur imposer un niveau.

## 6 ter. L'afficheur — phase D

Dernier organe non mesuré. Il n'est nécessaire à aucune fonction de conduite :
c'est un organe de maintenance, sous la coque (Q12). Mais il porte le battement
de cœur et l'affichage des défauts, et la LED D13 ne peut pas le remplacer
(§2 bis).

Acquis : les relais occupent le **dernier** octet émis, bits 0 à 7. Restent les
seize bits des deux premiers octets.

### Ce qu'une documentation officieuse propose

Une synthèse trouvée en ligne donne la structure suivante :

| Octet | Ordre d'émission | Rôle | Bits de trame |
|---|---|---|---|
| 1er | premier | sélection des digits | 16 à 23 |
| 2e | | segments, codage `DP G F E D C B A` | 8 à 15 |
| 3e | dernier | relais | 0 à 7 |

**À traiter comme une orientation, pas comme une source.** La même
documentation donne pour la chaîne `D13/A3/A2/A1`, c'est-à-dire le brochage de
l'IO22D08, que le balayage des 120 triplets a réfuté : chaque rôle y est
décalé d'un cran par rapport au nôtre. Elle se contredit d'ailleurs
elle-même — sa prose annonce les relais « décalés en premier » quand son code
les envoie en dernier.

Ce qu'elle apporte de solide, c'est que **son code place les relais en
dernier**, ce que nous avions mesuré indépendamment. Et sa structure explique
l'observation faite en phase B — un segment s'allumant sur les quatre digits à
la fois — qui est le comportement attendu si la sélection valait alors zéro et
les activait tous.

### La mesure

```bash
VHELIO_SKETCH=$PWD/tools/pindisp ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/pindisp ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

Un seul bit d'afficheur allumé à la fois, **l'octet des relais tenu à zéro** :
cette phase est muette, l'afficheur se règle à l'œil et des relais qui
claquent n'ajouteraient que du bruit. Avance à la main, aux boutons.

| Bouton | Effet |
|---|---|
| K1 (D12) | Bit suivant |
| K2 (D10) | Bit précédent |
| K3 (D8) | L'**autre** octet d'afficheur, `0x00` ↔ `0xFF` |
| K4 (A0) | Avance automatique, 1,5 s par bit |

**K3 est le test décisif.** L'autre octet à `0xFF` :

- un **segment** s'allume partout → c'est l'octet des segments ;
- un **digit entier** s'allume → c'est l'octet de sélection, et le rang du bit
  donne la position du digit.

### Résultat — les segments, mesurés

L'hypothèse « un octet segments, un octet sélection » est **fausse**. Les huit
segments sont répartis sur les **deux** octets, six d'un côté et deux de
l'autre.

Le mot d'afficheur fait seize bits : bits 0 à 7 = deuxième octet émis (trame
8 à 15), bits 8 à 15 = premier octet émis (trame 16 à 23).

| Segment | Bit du mot | Octet émis |
|---|---|---|
| A — haut | 4 | 2ᵉ, b4 |
| B — haut droite | **12** | 1ᵉʳ, b4 |
| C — bas droite | 7 | 2ᵉ, b7 |
| D — bas | 3 | 2ᵉ, b3 |
| E — bas gauche | 1 | 2ᵉ, b1 |
| F — haut gauche | 6 | 2ᵉ, b6 |
| G — milieu | **11** | 1ᵉʳ, b3 |
| DP — point | 5 | 2ᵉ, b5 |

**Ce relevé se valide lui-même.** Deux essais ont produit des caractères
entiers plutôt que des segments isolés : `6.` (A C D E F G + point) et `0.`
(A B C D E F + point). Une seule affectation est compatible avec les deux, et
c'est celle-ci. Deux lectures isolées annonçaient « haut gauche » là où le
modèle prédit « haut droite » — les caractères composés tranchent, et la
confusion gauche/droite à l'œil est sans conséquence.

### La sélection des digits est active à l'état bas

C'est le point le plus déroutant du relevé, et son explication. Un segment
seul s'allume sur **tous** les digits : sélection à zéro, les quatre digits
sont validés. Corollaire, **un bit de sélection ne montre rien** en parcours
positif — non qu'il soit inerte, mais parce qu'aucun segment n'est allumé pour
le révéler.

Un bit est déjà identifié par déduction : **2ᵉ octet b2 = digit 1**. C'est le
seul essai qui ait donné un écran entièrement noir (`bit 10` avec l'autre
octet à `0xFF`), donc le seul où le digit 1 était éteint en plus des autres.

### Le parcours négatif

D'où l'ajout d'un second sens à `pindisp`, sur K3 : **un seul 0, tout le reste
à 1**. Tous les segments allumés, toutes les sélections inhibées, écran noir.
Effacer un bit de sélection valide son digit, qui s'allume seul, tous segments
dehors — un `8.` franc. Effacer un bit de segment ne change rien.

Le négatif est au bit de sélection ce que le motif `BIT A BIT` de la phase B
était au triplet correct : le seul essai qui produise de l'ordre plutôt que du
bruit.

### Résultat — la sélection des digits

| Digit | Bit du mot | Octet émis |
|---|---|---|
| 1 — milliers, à gauche | 2 | 2ᵉ, b2 |
| 2 | 9 | 1ᵉʳ, b1 |
| 3 | 10 | 1ᵉʳ, b2 |
| 4 — unités | 13 | 1ᵉʳ, b5 |

Quatre bits du mot ne servent à rien : 0, 8, 14 et 15.

**Il n'y a pas de deux-points**, contrairement à ce que la conception initiale
supposait — rien que des points décimaux, un par digit. Le battement de cœur
déménage donc sur le point du digit de gauche (§2 bis).

### Validation

Le modèle complet reproduit exactement les deux observations composées de la
phase D, ce qui vaut mieux qu'un relevé bit à bit non recoupé :

| Trame envoyée | Prédit | Observé |
|---|---|---|
| 2ᵉ octet `0xFF`, 1ᵉʳ octet bit 3 | A C D E F G + point, digit 1 inhibé | `6.6.6.` sur les digits 2, 3, 4 |
| 2ᵉ octet `0xFF`, 1ᵉʳ octet bit 4 | A B C D E F + point, digit 1 inhibé | `0.0.0.` sur les digits 2, 3, 4 |

Dans les deux cas, `0xFF` sur le deuxième octet arme le bit 2 — la sélection du
digit 1 — et l'inhibe donc, ce qui explique que seuls trois digits s'allument.
Aucun ajustement n'a été nécessaire pour faire coïncider modèle et mesure.

**Le brochage de la DN22D08 est entièrement mesuré.** Plus rien n'y est
supposé.

## 6 quater. L'afficheur en conditions réelles

Les phases A à D ont mesuré le brochage **bit par bit**. Vérifier que
l'assemblage de ces mesures produit quelque chose de **lisible** n'est pas la
même chose : un relevé bit à bit peut être juste et la table de glyphes fausse
quand même. Il suffit d'un segment mal nommé à l'œil, et le `6` devient un `5`
sans que rien ne le signale.

```bash
VHELIO_SKETCH=$PWD/tools/dispcheck ./tools/build-nix.sh
VHELIO_SKETCH=$PWD/tools/dispcheck ./tools/upload.sh /dev/ttyACM0 old
./tools/monitor.sh
```

`tools/dispcheck` reprend les tables de `board_io.cpp` **à l'identique**, mais
sans dépendre du firmware — il doit rester utilisable pendant une refonte.
Toute divergence entre les deux fichiers est un bug de l'un ou de l'autre.

| Bouton | Effet |
|---|---|
| K1 (D12) | Mode — compteur, défilé, tout allumé |
| K2 (D10) | Vitesse du compteur — 10 ms, 100 ms, 1 ms |
| K3 (D8) | Zéros de tête, affichés ou masqués |
| K4 (A0) | Battement de cœur, marche/arrêt |

**Trois modes, qui ne prouvent pas la même chose.**

- **Compteur** 0 → 9999 : les dix chiffres passent dans les quatre positions.
  C'est le test des glyphes *et* du multiplexage — un digit mal sélectionné se
  voit immédiatement, le chiffre apparaît sur la mauvaise position.
- **Défilé** : les dix chiffres, puis tous les glyphes de service. Ceux-là
  n'apparaîtraient jamais dans un compteur, or ce sont eux qui écrivent les
  codes de défaut (`F0xx` en hexadécimal, donc `A b C d E F`) et les messages
  de la page d'événements — et un code de défaut illisible est pire
  qu'inutile.
- **Tout allumé** : huit segments sur quatre digits, points compris. Un segment
  mort ou un digit mort ne se voit que là.

Le point du digit de **gauche** bat à 1 Hz : c'est le battement de cœur (§2 bis).
Les relais restent à zéro, ce croquis est muet.

## 7. Comment l'hypothèse initiale a été invalidée

Elle venait d'un mapping très répandu sur les cartes rail DIN pour Nano
(`OUT → D2..D9`, `IN → A0..A7`), plausible mais pas vérifié pour ce modèle. La
spécification l'assumait explicitement et prévoyait sa vérification — c'est ce
qui a permis de la corriger sans rien casser d'autre.

Trois choses ont limité les dégâts :

- **Le brochage était concentré** dans `pins.h` et deux tableaux de
  `board_io.cpp`. Aucun module métier n'a été touché par le changement de
  brochage lui-même.
- **La couche `board_io` existait déjà** comme abstraction. Elle est passée de
  « masquer une polarité » à « masquer un registre à décalage » sans que ses
  appelants changent d'une ligne.
- **L'incertitude était documentée** comme question ouverte bloquante, avec
  une procédure de vérification. Elle a été traitée comme un risque connu, pas
  découverte au câblage.

Ce qui a réellement dû changer : le module d'éclairage (deux relais au lieu
d'une sortie modulée), la suppression du buzzer, l'ajout du pilotage de
l'afficheur, et le déplacement de l'écoute UART de D10 vers A4.

