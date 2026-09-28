#!/usr/bin/env bash
# Tag and publish a fork release.
#
#   scripts/release.sh tc_v0.7.0 1      # -> tag tc_v0.7.0-fork.1
#
# Steps: check CHANGELOG entry, tag HEAD of "fork", push branch fork/<tag> + tag
# to GitHub (remote "fork"), push tooling branch. Docker push: scripts/build.sh <tag> --push
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG="${1:?usage: $0 <version-folder> <fork-revision>}"
REV="${2:?usage: $0 <version-folder> <fork-revision>}"
DIR="$ROOT/$TAG"
RELEASE="$TAG-fork.$REV"

grep -q "^## \[${RELEASE#tc_v}\]" "$ROOT/CHANGELOG.md" || {
    echo "Error: CHANGELOG.md has no section '## [${RELEASE#tc_v}]'." >&2
    exit 1
}
git -C "$DIR" diff --quiet HEAD || { echo "Error: uncommitted changes in $DIR." >&2; exit 1; }

"$ROOT/scripts/export-patches.sh" "$TAG" >/dev/null
if [ -n "$(git -C "$ROOT" status --porcelain)" ]; then
    echo "Error: tooling repo has uncommitted changes (patches/CHANGELOG) - commit them first." >&2
    git -C "$ROOT" status --short >&2
    exit 1
fi

git -C "$DIR" tag -a "$RELEASE" -m "$RELEASE" fork
git -C "$DIR" push fork "fork:refs/heads/fork/$TAG" "refs/tags/$RELEASE"
git -C "$ROOT" push origin tooling

echo "Released $RELEASE. Next: scripts/build.sh $TAG --push"
