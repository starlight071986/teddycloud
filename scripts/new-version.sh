#!/usr/bin/env bash
# Clone an original upstream release into ./<tag>/ and apply our patch series.
#
#   scripts/new-version.sh tc_v0.7.1
#
# Result: <tag>/ is an untouched upstream clone (tag <tag>) with a branch
# "fork" = <tag> + patches/server/*.patch (+ patches/web/*.patch in teddycloud_web).
set -euo pipefail

UPSTREAM="https://github.com/toniebox-reverse-engineering/teddycloud.git"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG="${1:?usage: $0 <upstream-tag>  (e.g. tc_v0.7.1, see: git ls-remote --tags $UPSTREAM)}"
DIR="$ROOT/$TAG"

if [ -e "$DIR" ]; then
    echo "Error: $DIR already exists." >&2
    exit 1
fi

git clone --recurse-submodules --branch "$TAG" "$UPSTREAM" "$DIR"
git -C "$DIR" checkout -b fork "$TAG"

# Commit identity + fork remote are taken from the tooling repo, never from the global git config.
NAME="$(git -C "$ROOT" config --local user.name || true)"
EMAIL="$(git -C "$ROOT" config --local user.email || true)"
[ -n "$NAME" ] && [ -n "$EMAIL" ] || { echo "Error: set user.name/user.email locally in $ROOT first." >&2; exit 1; }
for repo in "$DIR" "$DIR/teddycloud_web"; do
    git -C "$repo" config user.name "$NAME"
    git -C "$repo" config user.email "$EMAIL"
done
git -C "$DIR" remote add fork "$(git -C "$ROOT" remote get-url origin)"

apply_series() { # <repo-dir> <patch-dir>
    local repo="$1" patches=("$2"/*.patch)
    [ -e "${patches[0]}" ] || return 0
    if ! git -C "$repo" am -3 "${patches[@]}"; then
        cat >&2 <<EOF

Patch conflict in $repo.
Resolve the conflicting files, then:
    git -C "$repo" add <files> && git -C "$repo" am --continue
When done, refresh the patch files with:
    scripts/export-patches.sh $TAG
EOF
        exit 1
    fi
}

apply_series "$DIR" "$ROOT/patches/server"

# Frontend patches go into the teddycloud_web submodule.
if compgen -G "$ROOT/patches/web/*.patch" >/dev/null; then
    git -C "$DIR/teddycloud_web" checkout -b fork
    apply_series "$DIR/teddycloud_web" "$ROOT/patches/web"
    echo "Note: frontend patched - run 'make web' in $DIR (needs npm) before building the image."
fi

echo
echo "Done. Fork commits on top of $TAG:"
git -C "$DIR" log --oneline "$TAG..fork"
