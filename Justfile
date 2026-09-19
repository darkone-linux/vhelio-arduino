# Justfile du calculateur Vhélio.
#
# Recettes :
#   just build             compile le firmware de route
#   just check             balaye les quinze variantes de config.h (avant commit)
#   just package           assemble dist/vhelio-X.Y.Z.zip (même paquet que la CI)
#   just release [niveau]  publie une version : changelog, commit, etiquette, poussee
#
# `just release` attend patch, minor, major ou une version explicite X.Y.Z :
#   just release            -> 0.3.0 devient 0.3.1
#   just release minor      -> 0.3.0 devient 0.4.0
#   just release 0.3.0      -> publie la version deja inscrite a config.h
#                              (premiere version etiquetee du depot)
#   just release patch --dry-run -> affiche ce qui serait fait, sans rien toucher
#
# A la fin, le commit ET l'etiquette sont pousses : c'est la poussee de
# l'etiquette vX.Y.Z qui declenche .github/workflows/release.yml, laquelle
# verifie, package et cree la Release GitHub avec le zip.

default:
    @just --list

# Compile le firmware de route (ajouter "old" pour un Nano a ancien bootloader).
build *args="":
    ./tools/build.sh {{ args }}

# Les quinze variantes doivent compiler — obligatoire avant tout commit.
check:
    ./tools/check-variants.sh

# Assemble le paquet de la version inscrite a config.h.
package *args="":
    ./tools/package.sh {{ args }}

# Publie une version : bump, changelog, commit, etiquette vX.Y.Z, poussee.
release niveau="patch" *args="":
    #!/usr/bin/env bash
    set -euo pipefail
    NIVEAU="{{ niveau }}"
    ARGS="{{ args }}"
    CFG=firmware/vhelio/src/config.h

    secoue() { echo "release : $*" >&2; exit 1; }

    # A blanc, on ne touche a rien : les gardes ne s'appliquent pas.
    if [[ "$ARGS" != *--dry-run* ]]; then
      git diff --quiet && git diff --cached --quiet || secoue "arbre de travail sale, commiter ou remiser avant de publier"
      [ "$(git rev-parse --abbrev-ref HEAD)" = "main" ] || secoue "publier depuis main, pas depuis $(git rev-parse --abbrev-ref HEAD)"
    fi

    ACTUELLE="$(sed -nE 's/^#define[[:space:]]+VHELIO_FW_VERSION[[:space:]]+"([^"]+)".*/\1/p' "$CFG")"
    [ -n "$ACTUELLE" ] || secoue "VHELIO_FW_VERSION introuvable dans $CFG"

    if [[ "$NIVEAU" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
      NOUVELLE="$NIVEAU"
    else
      Majeur="${ACTUELLE%%.*}"; reste="${ACTUELLE#*.}"
      mineur="${reste%%.*}"; correctif="${reste##*.}"
      case "$NIVEAU" in
        patch) NOUVELLE="$Majeur.$mineur.$((correctif + 1))" ;;
        minor) NOUVELLE="$Majeur.$((mineur + 1)).0" ;;
        major) NOUVELLE="$((Majeur + 1)).0.0" ;;
        *) secoue "niveau attendu : patch, minor, major ou X.Y.Z (recu : $NIVEAU)" ;;
      esac
    fi
    ETIQUETTE="v$NOUVELLE"
    git rev-parse "$ETIQUETTE" >/dev/null 2>&1 && secoue "l'etiquette $ETIQUETTE existe deja"

    # Notes = sujets des commits depuis la derniere etiquette (ou tout l'historique).
    DERNIERE="$(git describe --tags --abbrev=0 2>/dev/null || true)"
    if [ -n "$DERNIERE" ]; then NOTES="$(git log "$DERNIERE..HEAD" --format='- %s')"; else NOTES="$(git log --format='- %s' --reverse)"; fi
    [ -n "$NOTES" ] || secoue "rien a publier : aucun commit depuis ${DERNIERE:-le debut}"

    if [[ "$ARGS" == *--dry-run* ]]; then
      echo "--- DRY-RUN : version $ACTUELLE -> $NOUVELLE ($ETIQUETTE, base : ${DERNIERE:-aucune etiquette})"
      echo "$NOTES"
      echo "--- DRY-RUN : $CFG et CHANGELOG.md inchanges, rien de commite, rien de pousse"
      exit 0
    fi

    echo "== Publication $NOUVELLE (base : ${DERNIERE:-aucune etiquette}) =="

    # 1. config.h suit la nouvelle version (sans effet si version explicite identique).
    sed -i -E "s/^(#define[[:space:]]+VHELIO_FW_VERSION[[:space:]]+\")[^\"]+(\".*)$/\1$NOUVELLE\2/" "$CFG"
    grep -q "VHELIO_FW_VERSION \"$NOUVELLE\"" "$CFG" || secoue "mise a jour de $CFG impossible"

    # 2. CHANGELOG.md : nouvelle section datee sous [Non publié], liens compares tenus.
    export NOUVELLE DATE NOTES DERNIERE
    DATE="$(date +%F)"
    python3 - "$NOUVELLE" "$DATE" <<'EOF'
    import os, sys
    nouvelle, date = sys.argv[1], sys.argv[2]
    notes = os.environ["NOTES"]
    chemin = "CHANGELOG.md"
    texte = open(chemin).read()
    if f"## [{nouvelle}]" not in texte:
        section = f"## [{nouvelle}] - {date}\n\n{notes}\n"
        marqueur = "## [Non publié]\n"
        assert marqueur in texte, "section [Non publié] introuvable"
        texte = texte.replace(marqueur, marqueur + "\n" + section, 1)
    ancienne = os.environ.get("DERNIERE", "")
    ancienne = ancienne[1:] if ancienne.startswith("v") else None
    if ancienne and ancienne != nouvelle:
        texte = texte.replace(
            f"[Non publié]: https://github.com/darkone-linux/vhelio-arduino/compare/v{ancienne}...HEAD",
            f"[Non publié]: https://github.com/darkone-linux/vhelio-arduino/compare/v{nouvelle}...HEAD", 1)
        texte += f"[{nouvelle}]: https://github.com/darkone-linux/vhelio-arduino/compare/v{ancienne}...v{nouvelle}\n"
    open(chemin, "w").write(texte)
    EOF

    # 3. Les quinze variantes compilent — obligatoire avant commit.
    ./tools/check-variants.sh

    echo "Rappel : si l'empreinte a change, mettre a jour README.md et specs/01 + specs/06 avant de pousser."
    read -r -p "Relire le diff, puis ENTREE pour commiter et etiqueter $ETIQUETTE (Ctrl-C pour arreter) : " _
    git diff --stat
    read -r -p "Pousser commit + etiquette vers origin (declenche la CI Release) ? [o/N] " REP
    [[ "$REP" =~ ^[oOyY]$ ]] || secoue "abandon, rien de commite"

    # 4. Commit (une ligne, en francais), etiquette annotee, poussee.
    # Cas limite : version explicite deja en place et changelog deja a jour
    # (premiere release) — rien a commiter, on etiquette HEAD tel quel.
    git add "$CFG" CHANGELOG.md
    if git diff --cached --quiet; then
      echo "== Aucun changement a commiter, $ETIQUETTE pointera sur HEAD =="
    else
      git commit -m "Version $NOUVELLE"
    fi
    git tag -a "$ETIQUETTE" -m "Version $NOUVELLE"
    git push origin main
    git push origin "$ETIQUETTE"
    echo "== $ETIQUETTE pousse : la CI publie le paquet sur https://github.com/darkone-linux/vhelio-arduino/releases =="
