#!/usr/bin/env bash
# Build the fork image from a version folder.
#
#   scripts/build.sh tc_v0.7.0            # local image for this machine's arch (teddycloud-fork:dev)
#   scripts/build.sh tc_v0.7.0 --push     # multi-arch (amd64+arm64) build, push to Docker Hub
#
# --push requires the HEAD of branch "fork" to carry a release tag tc_vX.Y.Z-fork.N
# (see scripts/release.sh). Pushed tags: <repo>:X.Y.Z-fork.N and <repo>:latest
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG="${1:?usage: $0 <version-folder> [--push]}"
DIR="$ROOT/$TAG"
MODE="${2:-}"
REPO="${DOCKER_REPO:-starlight1986/teddycloud}"

if [ "$(git -C "$DIR" rev-parse --abbrev-ref HEAD)" != "fork" ]; then
    echo "Error: $DIR is not on branch 'fork'." >&2
    exit 1
fi
if ! git -C "$DIR" diff --quiet HEAD; then
    echo "Error: uncommitted changes in $DIR - commit them first (and run export-patches.sh)." >&2
    exit 1
fi

SHA="$(git -C "$DIR" rev-parse --short HEAD)"
RELEASE="$(git -C "$DIR" tag --points-at HEAD --list "$TAG-fork.*" | sort -V | tail -1)"

if [ "$MODE" = "--push" ]; then
    if [ -z "$RELEASE" ]; then
        echo "Error: HEAD of $DIR has no release tag $TAG-fork.N - run scripts/release.sh first." >&2
        exit 1
    fi
    VERSION="${RELEASE#tc_v}"
    docker buildx build --platform linux/amd64,linux/arm64 --push \
        -t "$REPO:$VERSION" -t "$REPO:latest" \
        --build-arg "TEDDYCLOUD_VERSION=$RELEASE" -f "$DIR/Dockerfile" "$DIR"
    echo "Pushed: $REPO:$VERSION and $REPO:latest"
else
    docker buildx build --load \
        -t "teddycloud-fork:dev" -t "teddycloud-fork:${RELEASE#tc_v}${RELEASE:+-}$SHA" \
        --build-arg "TEDDYCLOUD_VERSION=${RELEASE:-$TAG-fork}-$SHA" -f "$DIR/Dockerfile" "$DIR"
    echo "Built: teddycloud-fork:dev"
fi
