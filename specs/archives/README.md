# Archives

Ce qui a servi à décider, mais ne sert plus à écrire ni à lire le code.

Rien ici ne fait autorité. En cas de contradiction avec `specs/`, avec
`hardware/` ou avec le code, **c'est la spécification active qui a raison** :
ces documents ne sont pas tenus à jour.

| Fichier | Ce qu'il garde | Quand le rouvrir |
|---|---|---|
| [`decouverte-brochage.md`](decouverte-brochage.md) | Les quatre campagnes de mesure du brochage de la DN22D08 (`pinfind`, `pinchain`, `pindisp`, `dispcheck`) et le post-mortem de l'hypothèse initiale | La carte est remplacée par un exemplaire qui ne répond pas comme celui-ci |
| [`questions-tranchees.md`](questions-tranchees.md) | Les treize questions de conception résolues, avec leur raisonnement | Une décision est rouverte et l'on veut savoir ce qui avait été écarté, et pourquoi |
| [`alternatives-materiel.md`](alternatives-materiel.md) | L'analyse comparée Nano+DN22D08 / ESP32 rail DIN / Mega / automate industriel. Conclusion : on ne change rien | L'un des trois besoins du §5 apparaît : télémétrie lisible en roulant, lecture du MPPT, ou fonction nouvelle sans entrée ni relais libre |
