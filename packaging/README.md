# Packaging — one-command install

A libfprint driver is compiled **into** `libfprint-2.so` (it is not a loadable
module), so every package here ships a **rebuilt libfprint that replaces the
distro's** and includes the `egis0576` driver. The unchanged system `fprintd`
loads it through the same soname — no `fprintd` rebuild.

Two layers below: **installing** (for users, once published) and **publishing**
(for the maintainer — needs your COPR/AUR accounts, like the GitHub push did).

---


## Upgrading from a release before v0.4.0

Those builds spoke a TLS-PSK transport to the sensor and left it in a mode it keeps
until something resets it. Two consequences for anyone packaging or installing the
upgrade:

- **The first fingerprint attempt after the upgrade can fail once** while the driver
  resets the sensor and it re-enumerates (~0.5 s). The next one works.
- **No re-enrollment is needed.** Existing prints keep matching — the exposure
  target and register are unchanged.

The same change also fixes dual-boot: earlier releases left the sensor in a state
where the Windows vendor driver failed to start it (Device Manager **Code 10**)
until the machine was fully powered down. v0.4.0 never enters that mode, and resets
a sensor it finds stuck in it.

## Fedora / COPR  →  `packaging/fedora/`

### Users install with

```bash
sudo dnf copr enable PHILIPPDEV5396/libfprint-egis0576
sudo dnf upgrade 'libfprint*'      # switches libfprint to the egis0576 build
sudo systemctl restart fprintd
```

### Maintainer: publish to COPR

Needs a [Fedora account](https://accounts.fedoraproject.org/) (COPR login) and
`copr-cli` (`sudo dnf install copr-cli`, then `copr-cli` config token from
`https://copr.fedorainfracloud.org/api/`).

```bash
# 1. create the project (once)
copr-cli create libfprint-egis0576 --chroot fedora-44-x86_64 --chroot fedora-rawhide-x86_64

# 2. IMPORTANT: set a repo priority so the COPR wins over Fedora's libfprint
#    regardless of version (Settings → "Repo priority" in the web UI, e.g. 90),
#    or after enabling, add `priority=90` to the generated /etc/yum.repos.d/_copr:*.repo.

# 3. build from this spec (it fetches Source0 = upstream tarball and
#    Source10 = this repo's own archive, at the tag in `%global egis_tag`)
copr-cli build-package ... # or the SCM method pointed at this repo + packaging/fedora/libfprint.spec
#   simplest: build a local SRPM and upload it:
sudo dnf install rpmdevtools rpm-build
rpmdev-setuptree
spectool -g -R packaging/fedora/libfprint.spec        # download sources
rpmbuild -bs packaging/fedora/libfprint.spec          # build the SRPM
copr-cli build libfprint-egis0576 ~/rpmbuild/SRPMS/libfprint-1.94.10-99.*.src.rpm
```

**What the spec builds:** pristine upstream libfprint **v1.94.10** + the egis0576
driver, keeping the package name `libfprint` (drop-in). BuildRequires, `%files`
and `%meson -Ddrivers=all` are taken verbatim from Fedora's own libfprint.spec, so
the file layout matches stock exactly — plus two files the spec installs on top:
the suspend/resume sleep hook and the no-autosuspend udev rule from
[`../integration/`](../integration/).

**Tradeoff:** the pristine build omits Fedora's *downstream* patches — notably
Fedora's own `egis_etu905` driver (USB `1c7a:05ae` / `1c7a:9201`). This only
affects you if you own one of those (rare) readers. If you want a true **superset**
on top of Fedora's package (keeping `egis_etu905` + Fedora's fixes), rebase onto
Fedora's dist-git and apply the tested
`libfprint-1.94.10-egis0576-fedora.patch` in this directory — it 3-way-merges
cleanly with Fedora's `egis_reader.patch` (both `egis0576` and `egis_etu905` then
coexist in one `libfprint-2.so`, verified).

**Maintenance:** you are now forwarding libfprint updates for enabled users. When
Fedora ships a **newer** libfprint version, bump `Version:` in the spec, re-test,
and rebuild. (COPR can auto-rebuild.)

---

## Arch / AUR  →  `packaging/aur/`

### Users install with

Not on the AUR — build from the PKGBUILD kept in this repo:

```bash
git clone https://github.com/PHILIPPDEV5396/libfprint-egis0576.git
cd libfprint-egis0576/packaging/aur
makepkg -si
sudo systemctl restart fprintd
```

`package()` is a bare `meson install`, so — unlike the Fedora RPM — it installs
**neither** the suspend/resume sleep hook nor the no-autosuspend udev rule. Install
both by hand afterwards; see [`../integration/`](../integration/).

pacman will prompt to replace the stock `libfprint` (this package `conflicts` with
it). Every time Arch bumps `libfprint`, `pacman -Syu` offers the stock package —
decline it, or rebuild this package at the new `pkgver`.

### Maintainer: publish to the AUR

Needs an [AUR account](https://aur.archlinux.org/) with an SSH key registered.

```bash
# generate .SRCINFO and push to the AUR git
cd packaging/aur
makepkg --printsrcinfo > .SRCINFO
git clone ssh://aur@aur.archlinux.org/libfprint-egis0576.git aur-repo
cp PKGBUILD .SRCINFO aur-repo/
cd aur-repo && git add -A && git commit -m "Initial import" && git push
```

**What the PKGBUILD builds:** upstream libfprint tag `v1.94.10` + this repo's
driver (at the release tag pinned in the PKGBUILD's `source=`), default driver set
(so other readers keep working),
`provides/conflicts libfprint`. `pkgver` is pinned to the upstream tag the meson
patch targets; if Arch's repo libfprint has moved past it, installing this package
downgrades libfprint itself (the driver still works — but rebasing the patch onto
the newer tag is the better move). As of September 2026 Arch's stock libfprint is also `1.94.10`, so this
is a lateral rebuild, not a downgrade.

---

## Releasing a new driver version

Both recipes pin the driver by **tag** — neither follows `main` — so a driver
release means bumping both, or users keep getting the old one:

| File | What to bump |
|---|---|
| `packaging/fedora/libfprint.spec` | `%global egis_tag` **and** the `Release:` suffix (`egisN`), plus a `%changelog` entry |
| `packaging/aur/PKGBUILD` | the `#tag=` in `source=`, and reset `pkgrel=1` |

## Common notes

- **OpenSSL:** the egis0576 driver and its engine static library link **no** crypto
  at all — the sensor's `EGIS`/`SIGE` transport is plaintext. `libcrypto` is still a
  dependency of the finished `libfprint-2.so`, but only because upstream's own
  `uru4000` driver uses it. Both packages already pull
  OpenSSL ≥ 3.0 (Fedora `openssl-devel` BR / Arch `openssl` dep). RPM's automatic
  soname dep adds `libcrypto.so.3` at runtime.
- **SELinux (Fedora):** the rebuilt `.so` installs to the standard `%{_libdir}`
  with the correct context, and has **no** text relocations (verified `readelf -d`
  has no `TEXTREL`), so it loads under enforcing with nothing to do.
- **Not `curl | bash`:** installing a replacement *system library* as root from a
  blind pipe is a real risk (no integrity check, files outside the package DB, no
  clean uninstall). Prefer the package managers above; for unsupported distros,
  the source `install.sh` (download, read, run a pinned tag) is the honest fallback.
