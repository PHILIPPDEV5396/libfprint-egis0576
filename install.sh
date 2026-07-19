#!/usr/bin/env bash
#
# Build and install libfprint with the EgisTec EH576 (egis0576) driver.
#
# This clones a pinned upstream libfprint, drops the egis0576 driver into it,
# applies a small meson patch, builds the full default driver set (so your other
# fingerprint hardware keeps working), and installs it.
#
# A libfprint driver is compiled *into* libfprint-2.so — it is not a loadable
# module — so "installing the driver" means installing a libfprint that contains
# it. This does NOT rebuild fprintd; the stock fprintd loads the new library via
# its unchanged ABI.
#
set -euo pipefail

LIBFPRINT_VERSION="1.94.10"
LIBFPRINT_URL="https://gitlab.freedesktop.org/libfprint/libfprint.git"
PREFIX="${PREFIX:-/usr/local}"
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH="$REPO/patches/libfprint-${LIBFPRINT_VERSION}-egis0576.patch"

echo ">>> EgisTec EH576 libfprint driver installer"
echo "    libfprint version : $LIBFPRINT_VERSION"
echo "    install prefix    : $PREFIX   (override with: PREFIX=/usr ./install.sh)"
echo

# --- required build tools ---------------------------------------------------
missing=0
for tool in git meson ninja gcc pkg-config; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "  MISSING build tool: $tool"; missing=1
    fi
done
if [ "$missing" = 1 ]; then
    echo
    echo "Install the build tools + dev libraries first (see README.md, 'Building')."
    echo "You also need the -dev packages for: glib2, gusb, openssl/libcrypto (>=3.0),"
    echo "gobject-introspection, gudev, nss, pixman, cairo."
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo ">>> cloning libfprint $LIBFPRINT_VERSION ..."
git clone --quiet --depth 1 --branch "v$LIBFPRINT_VERSION" "$LIBFPRINT_URL" "$WORK/libfprint"
cd "$WORK/libfprint"

echo ">>> installing the egis0576 driver sources ..."
cp "$REPO/driver/egis0576.c" "$REPO/driver/egis0576.h" libfprint/drivers/
cp -r "$REPO/driver/egis0576" libfprint/drivers/

echo ">>> applying the meson integration patch ..."
git apply "$PATCH"

echo ">>> configuring + building (full default driver set + egis0576) ..."
meson setup builddir --prefix="$PREFIX" --buildtype=release
ninja -C builddir

echo ">>> installing to $PREFIX (sudo) ..."
sudo ninja -C builddir install
sudo ldconfig

echo
echo ">>> Done. libfprint (with egis0576) installed to $PREFIX."
echo
echo "    IMPORTANT: fprintd must load THIS libfprint, not the distro's."
echo "    - If you installed to /usr/local, make sure $PREFIX/lib (or lib64) is on"
echo "      the linker path (it usually is on Fedora/Arch). Verify with:"
echo "          ldconfig -p | grep libfprint-2"
echo "      It should point at $PREFIX. If not, add $PREFIX/lib* to"
echo "      /etc/ld.so.conf.d/ and re-run 'sudo ldconfig'."
echo "    - Then restart the daemon:  sudo systemctl restart fprintd"
echo
echo "    Next: enroll a finger and enable PAM — see README.md."
