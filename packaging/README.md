# Packaging — one-command install

A libfprint driver is compiled **into** `libfprint-2.so` (it is not a loadable
module), so every package here ships a **rebuilt libfprint that replaces the
distro's** and includes the `egis0576` driver. The unchanged system `fprintd`
loads it through the same soname — no `fprintd` rebuild.

Two layers below: **installing** (for users, once published) and **publishing**
(for the maintainer — needs your COPR/AUR accounts, like the GitHub push did).

---

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
#    Source10 = this repo's v0.1.0 archive itself)
copr-cli build-package ... # or the SCM method pointed at this repo + packaging/fedora/libfprint.spec
#   simplest: build a local SRPM and upload it:
sudo dnf install rpmdevtools rpmbuild
rpmdev-setuptree
spectool -g -R packaging/fedora/libfprint.spec        # download sources
rpmbuild -bs packaging/fedora/libfprint.spec          # build the SRPM
copr-cli build libfprint-egis0576 ~/rpmbuild/SRPMS/libfprint-1.94.10-99.egis1*.src.rpm
```

**What the spec builds:** pristine upstream libfprint **v1.94.10** + the egis0576
driver, keeping the package name `libfprint` (drop-in). BuildRequires, `%files`
and `%meson -Ddrivers=all` are taken verbatim from Fedora's own libfprint.spec, so
the file layout matches stock exactly.

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

```bash
yay -S libfprint-egis0576          # or paru, or makepkg -si from a clone
sudo systemctl restart fprintd
```

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
driver (tag `v0.1.0`), default driver set (so other readers keep working),
`provides/conflicts libfprint`. Arch's stock libfprint is also `1.94.10`, so this
is a lateral rebuild, not a downgrade.

---

## Common notes

- **OpenSSL:** the egis0576 engine links `libcrypto`; both packages already pull
  OpenSSL ≥ 3.0 (Fedora `openssl-devel` BR / Arch `openssl` dep). RPM's automatic
  soname dep adds `libcrypto.so.3` at runtime.
- **SELinux (Fedora):** the rebuilt `.so` installs to the standard `%{_libdir}`
  with the correct context, and has **no** text relocations (verified `readelf -d`
  has no `TEXTREL`), so it loads under enforcing with nothing to do.
- **Not `curl | bash`:** installing a replacement *system library* as root from a
  blind pipe is a real risk (no integrity check, files outside the package DB, no
  clean uninstall). Prefer the package managers above; for unsupported distros,
  the source `install.sh` (download, read, run a pinned tag) is the honest fallback.
