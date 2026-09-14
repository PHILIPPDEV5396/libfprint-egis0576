#!/bin/bash
# Build packaging/ubuntu-tod for one Ubuntu series, stamping a version and a
# distribution that identify that series in the resulting .deb.
#
# Why a build script instead of two checked-in debian/changelog files: dch is
# the ordinary Debian tool for stamping a version and a distribution, and one
# script keeps noble and resolute from drifting out of sync by hand. The
# change lands in debian/changelog only for the duration of the build; this
# script restores the file with `git checkout` afterward, so the committed
# changelog names no series, and a plain `dpkg-buildpackage` from a clean
# checkout still works and simply omits the ~series suffix.
#
# Why the version needs a per-series suffix at all: this driver builds
# directly from the repository tree (see packaging/README.md), so its
# version does not track Ubuntu's own libfprint version and does not need
# to out-sort an SRU the way a replacement libfprint package would. The
# problem here is narrower: a module built on noble and a module built on
# resolute are different binaries (different libgusb runtime dependency
# name, see below), so they must carry different, comparable versions.
#
# ~24.04 / ~26.04 rather than ~noble / ~resolute: dpkg compares a ~-tagged
# suffix numerically where both sides are digits, so ~24.04 sorts below
# ~26.04 for the same reason Ubuntu's own ~24.04.8-style suffixes do. Two
# codenames would sort alphabetically instead, which happens to agree here
# (noble < resolute) but is not a rule a future series name is bound by.
# Verified directly, not reasoned about:
#   $ dpkg --compare-versions 0.4.4-1~24.04 '<<' 0.4.4-1~26.04 && echo yes
#   yes
# so a user who upgrades noble to resolute and installs the resolute .deb
# ends up on it even without removing the noble one first.
set -euo pipefail

usage() {
    echo "usage: $(basename "$0") <noble|resolute>" >&2
    exit 1
}

SERIES="${1:-}"
[ -n "$SERIES" ] || usage

case "$SERIES" in
    noble)    RELEASE=24.04 ;;
    resolute) RELEASE=26.04 ;;
    *)
        echo "error: unknown series '$SERIES' (expected noble or resolute)" >&2
        exit 1
        ;;
esac

cd "$(dirname "$0")"

if ! git diff --quiet -- debian/changelog || ! git diff --quiet --cached -- debian/changelog; then
    echo "error: debian/changelog already has uncommitted changes; commit or revert first" >&2
    exit 1
fi

BASE_VERSION=$(dpkg-parsechangelog -S Version)
case "$BASE_VERSION" in
    *~*)
        echo "error: debian/changelog version '$BASE_VERSION' already carries a ~ suffix; run from a clean checkout" >&2
        exit 1
        ;;
esac
SERIES_VERSION="${BASE_VERSION}~${RELEASE}"

restore_changelog() { git checkout -- debian/changelog; }
trap restore_changelog EXIT

DEBFULLNAME="${DEBFULLNAME:-$(dpkg-parsechangelog -S Maintainer | sed 's/ <.*//')}" \
DEBEMAIL="${DEBEMAIL:-$(dpkg-parsechangelog -S Maintainer | sed 's/.*<\(.*\)>/\1/')}" \
    dch -b -v "$SERIES_VERSION" --distribution "$SERIES" \
        "Build for Ubuntu $SERIES ($RELEASE): stamps the ~$RELEASE version suffix so the noble and resolute .debs sort and are named apart."

dpkg-buildpackage -us -uc -b

echo "Built $SERIES ($RELEASE): version $SERIES_VERSION"
