# Journal des modifications

Tous les changements notables du firmware sont consignés ici, en français,
une entrée par version publiée.

La source de vérité du numéro de version est
`firmware/vhelio/src/config.h` (`VHELIO_FW_VERSION`). Les étiquettes git
correspondantes sont de la forme `vX.Y.Z`. Le format suit, en esprit,
[Keep a Changelog](https://keepachangelog.com/fr/1.1.0/) : sections
`Ajouté`, `Changé`, `Corrigé` sous chaque version.

## [Non publié]

## [0.3.0] - 2026-08-24

Version de banc : le brochage est mesuré borne par borne, le faisceau peut
être testé avant d'être serti.

### Ajouté

- Brochage DN22D08 entièrement mesuré : entrées sur D2-D7/D9/D11, boutons
  sur D8/D10/D12/A0, chaîne données A5 / horloge A4 / verrou A3, OE sur A2,
  segments d'afficheur répartis sur les deux octets (actif haut,
  sélection actif bas).
- Croquis de découverte et de vérification : `tools/pinfind`,
  `tools/pinchain`, `tools/pinscan`, `tools/pindisp`, `tools/dispcheck`
  (compteur 0-9999 pour valider glyphes et multiplexage).
- Firmware de banc (`SIM_INPUTS=1`, jamais dans `config.h`, toujours par
  `VHELIO_SIM=1` sur la ligne de commande) : huit entrées pilotables au
  clavier depuis la console.
- `tools/seq-banc.sh` : campagne 1 du plan de tests déroulée au métronome
  (181 s, huit points à contrôler à l'oreille).
- Afficheur événementiel : ce qui vient de se passer sur trois digits,
  numéro du point de contrôle sur le quatrième (`Fr`, `AC`, `Err`, `Ph`,
  `UE`, `CLG`/`CLd`, `CLO`, `CL2`, `---`), battement de cœur 1 Hz
  (~4 Hz en défaut), page défauts en hexadécimal.
- `tools/monitor.sh` : console série sans arduino-cli (inutilisable sur
  NixOS).

### Changé

- Écoute Bafang déplacée sur A1 : A4/A5 portent la chaîne de registres.
- Spécifications condensées : ce qui a servi à décider part en
  `specs/archives/` (ne fait plus autorité).
- Un événement d'afficheur tient 1,5 s au lieu d'1 s.

### Corrigé

- Téléversement : mot-clé `old` obligatoire (ancien bootloader,
  57 600 bauds), `upload.sh` ne compile plus (il téléversait un binaire
  inattendu), chaîne de compilation sortie de `.build/`, détection du port.
- Le pinscan exige le 12 V au bornier, pas seulement l'USB (bobines des
  relais et LED des optocoupleurs alimentées en 12 V).
- Versions AVR et ctags cherchées au lieu d'être codées en dur.

## [0.2.0] - 2026-08-24

Refonte pour l'architecture réelle de la DN22D08 : relais pilotés par
registre à décalage, et non sorties à transistor sur broches dédiées.

### Ajouté

- Couche `board_io` : `setOutput()` ne pose qu'un bit, `board::refresh()`
  est le seul point qui touche le matériel de sortie.
- Voyant de défaut sur R7, acquittement sur IN3.
- Diode 1N4148 de découplage sur le frein avant (un contacteur unipolaire
  sert à la fois le feu stop 12 V et la ligne frein 5 V du contrôleur).
- Klaxon autonome (hors périmètre du firmware) : IN3 et R7 deviennent la
  seule voie libre du montage.
- Phares mesurés, entrées NPN confirmées.
- Réponses intégrées aux questions ouvertes Q3 à Q12.

### Changé

- `VHELIO_FW_VERSION` passe à 0.2.0.

## [0.1.0] - 2026-08-24

Première version : calculateur d'éclairage et de signalisation pour
Arduino Nano + carte rail DIN DN22D08, en remplacement du câblage en dur
du guide de montage officiel.

### Ajouté

- 14 modules, ordonnanceur coopératif sans `delay()`, temps de cycle
  < 10 ms, état sûr par défaut, chien de garde.
- Éclairage, clignotants, détresse, feu stop, coupure moteur redondante,
  télémétrie Bafang en écoute passive, diagnostic, autotest.
- Spécification complète (`specs/`), nomenclature et plan de câblage
  (`hardware/`), compilation et téléversement (`tools/`).
- Empreinte mesurée : 8 494 octets de flash (27 %) et 641 octets de RAM
  (31 %) sur ATmega328P ; 9 variantes de `config.h` couvertes par
  `tools/check-variants.sh`.

[Non publié]: https://github.com/darkone-linux/vhelio-arduino/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/darkone-linux/vhelio-arduino/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/darkone-linux/vhelio-arduino/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/darkone-linux/vhelio-arduino/releases/tag/v0.1.0
