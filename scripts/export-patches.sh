#!/usr/bin/env bash
# Regenerate patches/ from the "fork" branch of a version folder.
# Run after adding/changing fork commits (or after resolving conflicts).
#
#   scripts/export-patches.sh tc_v0.7.0
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG="${1:?usage: $0 <version-folder/tag>}"
DIR="$ROOT/$TAG"

export_series() { # <repo-dir> <base> <patch-dir>
    rm -f "$3"/*.patch
    git -C "$1" format-patch -q --no-signature -o "$3" "$2..fork"
}

export_series "$DIR" "$TAG" "$ROOT/patches/server"

# teddycloud_web: base is the submodule commit pinned by the upstream tag.
if git -C "$DIR/teddycloud_web" rev-parse -q --verify fork >/dev/null; then
    WEB_BASE="$(git -C "$DIR" rev-parse "$TAG:teddycloud_web")"
    export_series "$DIR/teddycloud_web" "$WEB_BASE" "$ROOT/patches/web"
fi

ls -1 "$ROOT"/patches/*/*.patch 2>/dev/null || echo "(no patches)"
