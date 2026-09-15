#!/usr/bin/env bash
#
# Build Debian binary packages for libfprint with the egis0576 driver.
#
# Mirrors install.sh and packaging/fedora/libfprint.spec: clones pristine
# upstream libfprint at LIBFPRINT_VERSION, stages this debian/ directory on
# top of it (dereferencing debian/extra-driver and debian/extra-integration,
# which are symlinks back into this repo, so driver.c and the integration
# files exist as real files only inside the throwaway build tree, never
# duplicated in this repo), copies in the meson integration patch, and runs
# dpkg-buildpackage.
#
# source/format is "3.0 (quilt)", which needs an .orig.tar.* beside the
# build tree before any dpkg-buildpackage invocation that touches the source
# package (-S; -b does not). That tarball is made here, from the pristine
# clone, before debian/ is staged on top of it, so a source build works too.
#
# Needs: git, dpkg-dev, and the packages debian/control lists under
# Build-Depends (or just run this inside debian:trixie with those installed).
set -euo pipefail

LIBFPRINT_VERSION="1.94.100"
LIBFPRINT_URL="https://gitlab.freedesktop.org/libfprint/libfprint.git"
SOURCE_NAME="libfprint-egis0576"
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTDIR="${OUTDIR:-$REPO/packaging/debian/build-output}"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo ">>> cloning pristine upstream libfprint $LIBFPRINT_VERSION ..."
git clone --quiet --depth 1 --branch "v$LIBFPRINT_VERSION" "$LIBFPRINT_URL" "$WORK/src"

echo ">>> creating the orig tarball for source builds ..."
tar --exclude-vcs \
    --transform "s,^\./,${SOURCE_NAME}-${LIBFPRINT_VERSION}/," \
    -cJf "$WORK/${SOURCE_NAME}_${LIBFPRINT_VERSION}.orig.tar.xz" \
    -C "$WORK/src" .

echo ">>> staging debian/ packaging (dereferencing driver/integration symlinks) ..."
cp -rL "$REPO/packaging/debian" "$WORK/src/debian"
# A prior run's OUTDIR default lives under packaging/debian/; strip any
# leftover .deb output from the staged copy so it never rides along inside
# debian.tar.xz or a source build.
rm -rf "$WORK/src/debian/build-output"

echo ">>> building (dpkg-buildpackage -us -uc -b) ..."
( cd "$WORK/src" && dpkg-buildpackage -us -uc -b )

mkdir -p "$OUTDIR"
cp "$WORK"/*.deb "$WORK"/*.changes "$WORK"/*.buildinfo "$OUTDIR"/
echo ">>> done. Packages in $OUTDIR:"
ls -1 "$OUTDIR"
