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

- **The first fingerprint attempt after the upgrade can fail once.** The driver
  finds the sensor in that mode, issues `ForceResetDevice` and fails that open;
  the sensor stays on the bus until the next transfer to it, then drops off and
  re-enumerates (~0.4 s), after which attempts work normally. Details in
  [`../docs/sensor-tuning.md` §6](../docs/sensor-tuning.md#6-forceresetdevice-takes-effect-on-the-next-transfer).
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
copr-cli create libfprint-egis0576 --chroot fedora-43-x86_64 --chroot fedora-44-x86_64 \
    --chroot fedora-45-x86_64 --chroot fedora-rawhide-x86_64
#    (the live project has these four chroots; enable "Follow Fedora branching"
#    in the project settings so new releases are added automatically)

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
copr-cli build libfprint-egis0576 ~/rpmbuild/SRPMS/libfprint-1.94.100-99.*.src.rpm
```

**What the spec builds:** pristine upstream libfprint **v1.94.100** + the egis0576
driver, keeping the package name `libfprint` (drop-in). BuildRequires, `%files`
and `%meson -Ddrivers=all` are taken verbatim from Fedora's own libfprint.spec, so
the file layout matches stock exactly — plus two files the spec installs on top:
the suspend/resume sleep hook and the no-autosuspend udev rule from
[`../integration/`](../integration/).

**No tradeoff since 1.94.100:** Fedora's own package now carries *no* downstream
patches (its spec has no `Patch:` lines) and upstream ships the `egis_etu905`
driver (USB `1c7a:05ae` / `1c7a:9201`) itself, so this build is a true superset of
Fedora's — nothing of theirs is lost. Up to 1.94.10 that was not true, and this
directory carried a separately rebased patch against Fedora's tree; the rebase
retired it.

**Maintenance:** you are now forwarding libfprint updates for enabled users. When
Fedora ships a **newer** libfprint version, bump `Version:` in the spec, rebase
`../patches/libfprint-<version>-egis0576.patch` (the meson hunks move), re-test,
and rebuild. (COPR can auto-rebuild.) *Status (September 2026):* this spec is at
1.94.100, the same version Fedora 44 ships, so `Release: 99…` wins on its own —
but keep the `priority=90` from step 2 anyway: it is what protects enabled users
during the window between a Fedora bump and the next rebase here.

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

**What the PKGBUILD builds:** upstream libfprint tag `v1.94.100` + this repo's
driver (at the release tag pinned in the PKGBUILD's `source=`), default driver set
(so other readers keep working),
`provides/conflicts libfprint`. `pkgver` is pinned to the upstream tag the meson
patch targets; if Arch's repo libfprint has moved past it, installing this package
downgrades libfprint itself (the driver still works — but rebasing the patch onto
the newer tag is the better move). As of September 2026 Arch's stock libfprint is
`1.94.100` (extra, since 2026-07-26) and this PKGBUILD targets the same tag, so it
is a lateral rebuild, not a downgrade.

---

## Debian / Ubuntu  →  `packaging/debian/`

### Users install with

Not published to a repository — build the `.deb` files from this repo:

```bash
git clone https://github.com/PHILIPPDEV5396/libfprint-egis0576.git
cd libfprint-egis0576
sudo apt-get build-dep ./packaging   # or install Build-Depends by hand, see below
bash packaging/debian/build.sh
sudo apt install -y ./packaging/debian/build-output/libfprint-2-2_*.deb \
                    ./packaging/debian/build-output/libfprint-2-dev_*.deb \
                    ./packaging/debian/build-output/gir1.2-fprint-2.0_*.deb
sudo systemctl restart fprintd
```

Debian's `libfprint-2-tests` package pins an exact version of `libfprint-2-2`
this build cannot satisfy, so if it is installed, `apt` removes it as part of
this install. `libfprint-2-tests` is a test-suite package almost nobody
installs, and nothing else on a normal system depends on it.

`packaging/debian/build.sh` needs `git` and `dpkg-dev` on the host, plus every
package `packaging/debian/control`'s `Build-Depends` lists (`debhelper-compat`,
`meson`, `libglib2.0-dev`, `libgirepository1.0-dev`, `libgusb-dev`,
`libgudev-1.0-dev`, `libpixman-1-dev`, `libssl-dev`, `libcairo2-dev`,
`systemd-dev`, `gobject-introspection` and the two `gir1.2-*-dev` packages).
`apt-get build-dep` reads that list from `packaging/debian/control`, but it
expects `<dir>/debian/control`, so it is pointed at `packaging/`, the parent
of `packaging/debian`, which plays the role of that `debian/` subdirectory.
The script clones pristine upstream libfprint at the version pinned inside
it, stages `packaging/debian/` on top, and runs `dpkg-buildpackage -us -uc
-b`. The `.deb` files (`libfprint-2-2`, `libfprint-2-dev`,
`gir1.2-fprint-2.0`, plus a `-dbgsym`) land in
`packaging/debian/build-output/`.

Since this keeps Debian's own package names, `apt install ./*.deb` replaces
the stock `libfprint-2-2` in place, with no `Provides`/`Conflicts` dance and
no install-time flag needed (see "Maintainer" below for why). `apt upgrade`
later can still offer to replace it back with Debian's own build once
Debian's own version moves past this build's `1.94.100`. When that happens,
reject the upgrade, add the apt pin described below, or rerun `build.sh`
after a driver update.

### Maintainer: what `packaging/debian/` builds and why

**Strategy:** pristine upstream libfprint **v1.94.100** (the same tag the
Fedora spec and the AUR `PKGBUILD` build) plus this repo's driver, patch and
integration files, **not** a rebuild of Debian's own `libfprint` source
package. Debian trixie ships 1.94.9 and sid/forky ship 1.94.10 — both predate
the meson build refactor `patches/libfprint-1.94.100-egis0576.patch` targets,
so that patch does not apply against Debian's source (confirmed: 2 of its 4
hunks fail against trixie's tree). Rebuilding on top of a pristine, matching
upstream tag is the same approach already proven for Fedora/COPR and Arch/AUR,
and it means one patch to rebase per driver release instead of two.

**No duplicated driver/patch/integration content.** `packaging/debian/`
contains no copies of `driver/`, `patches/` or `integration/`.
`packaging/debian/extra-driver` and `packaging/debian/extra-integration` are
symlinks back to `../../driver` and `../../integration`, and
`packaging/debian/patches/0001-egis0576-meson-integration.patch` is a symlink
to `../../../patches/libfprint-1.94.100-egis0576.patch`. `build.sh` is the
only place any of that content is copied, and only into a throwaway build
tree (`cp -rL`, which follows the symlinks) — this repo's own files stay the
single source of truth. `debian/rules`' `override_dh_auto_configure` and
`override_dh_auto_install` then copy from `debian/extra-driver` and
`debian/extra-integration` exactly the way the Fedora spec's `%prep`/`%install`
copy from the unpacked driver tarball.

**Package split, names and versioning:** the source package is
`libfprint-egis0576` (this repo's own identity), but the **binary** packages
keep Debian's own names (`libfprint-2-2`, `libfprint-2-dev`,
`gir1.2-fprint-2.0`), so `apt` treats this as a newer build of the same
package with no `Provides`/`Conflicts`/`Replaces` needed.

An epoch outranks everything else in a Debian version comparison: `dpkg`
compares epoch first, and only falls through to the upstream `Version` and
then the revision when both sides carry the same epoch. Debian trixie's
stock `libfprint-2-2` carries epoch `1:` (`1:1.94.9-1`), and sid/forky's
carries the same epoch at a newer upstream version (`1:1.94.10-1`). A build
with no epoch (epoch `0` implicitly) sorts below both of those regardless of
its own upstream `Version` or revision, which is why the version scheme this
package started with (`1.94.100-99egis1`, no epoch) lost to Debian's own
package and needed `--allow-downgrades` to install at all, and would have
been silently reverted on the next `apt upgrade`.

This package's changelog therefore carries the same epoch Debian carries,
`1:`, giving it version `1:1.94.100-99egis1`. With the epoch matched,
comparison falls through to the upstream `Version`, and `1.94.100` beats
both `1.94.9` (trixie) and `1.94.10` (sid/forky):

```
$ dpkg --compare-versions '1:1.94.100-99egis1' gt '1:1.94.9-1'  && echo true
true
$ dpkg --compare-versions '1:1.94.100-99egis1' gt '1:1.94.10-1' && echo true
true
```

The `-99egis1` revision then mirrors the Fedora spec's `Release:
99...egis9` comment: the numeric `99` sorts above any plausible normal
Debian revision (`-1`, `-2`, ...) at the same epoch and the same upstream
`Version`, including a hypothetical future Debian rebuild of `1.94.100`
itself:

```
$ dpkg --compare-versions '1:1.94.100-99egis1' gt '1:1.94.100-1' && echo true
true
```

No `Provides`/`Conflicts`/`Replaces` is needed, and neither is an
install-time flag: "Users install with" above runs a plain `apt install`.

**Backstop for when Debian moves past this upstream version:** the epoch
and the `99` revision only win while this build's upstream `Version`
(`1.94.100`) is at or above Debian's own. If Debian's own `libfprint`
package later ships an upstream version above `1.94.100` (still at epoch
`1:`), that Debian build wins the comparison again and `apt upgrade` reverts
silently to it, the same failure mode `--allow-downgrades` created and this
epoch fix closes for now. The robust backstop for that case is the same one
the COPR section below documents for Fedora: an `apt` pin, added to
`/etc/apt/preferences.d/libfprint-egis0576`:

```
Package: libfprint-2-2 libfprint-2-dev gir1.2-fprint-2.0
Pin: version 1:1.94.100-99egis*
Pin-Priority: 1001
```

A `Pin-Priority` above 1000 tells `apt` to accept this pin even to
downgrade the installed version, which is exactly the property needed here:
Debian's own newer build must lose to this pin on purpose until this
package is rebuilt against a newer upstream tag.

This pin matches only the version glob it names. Once this package is
rebuilt against a newer upstream tag, its version string changes, the pin
above no longer matches it, and the pin stops protecting the installed
package, silently, the same way the missing epoch did before this fix.
Update the pin's version glob at the same time as any rebuild against a
newer upstream tag.

**Integration files installed automatically, unlike the AUR build:** the
suspend/resume sleep hook and no-autosuspend udev rule are in
`libfprint-2-2`'s own `.install` file, the same way the Fedora spec's
`%install` adds them — closing the gap the AUR section above documents for
its own `package()`.

**No `debian/*.symbols` file:** Debian's own `libfprint-2-2.symbols` is a
hand-maintained, version-tagged export list tied to Debian's own upstream
import lineage (`1:1.90.1`, `1:1.94.1`, ...), which this build does not
follow — it tracks the tag pinned in `build.sh` instead. `dh_makeshlibs -- -c0`
gives plain `${shlibs:Depends}` (soname + version floor) instead.

**Verified with a real build** (`debian:trixie`, this session, `linux/arm64`
only — the packaging logic is architecture-neutral, and CI builds and tests
`linux/amd64` on every tag, but only `linux/arm64` was built and tested in
this session):

```
$ dpkg-deb -f libfprint-2-2_1.94.100-99egis1_arm64.deb Version
1:1.94.100-99egis1

$ dpkg -c libfprint-2-2_1.94.100-99egis1_arm64.deb | grep -E "system-sleep|nosuspend|libfprint-2.so"
-rw-r--r-- root/root   1208848 ... ./usr/lib/aarch64-linux-gnu/libfprint-2.so.2.0.0
-rwxr-xr-x root/root      2344 ... ./usr/lib/systemd/system-sleep/50-egis0576-fp-resume.sh
-rw-r--r-- root/root       810 ... ./usr/lib/udev/rules.d/60-egis0576-fp-nosuspend.rules
lrwxrwxrwx root/root         0 ... ./usr/lib/aarch64-linux-gnu/libfprint-2.so.2 -> libfprint-2.so.2.0.0

$ objdump -p usr/lib/aarch64-linux-gnu/libfprint-2.so.2.0.0 | grep SONAME
  SONAME               libfprint-2.so.2
```

Plain `apt-get install`, no `--allow-downgrades`, against a container with
Debian's own stock package already installed:

```
$ dpkg-query -W -f='${Version}' libfprint-2-2
1:1.94.9-1

$ apt-get install -y ./libfprint-2-2_1.94.100-99egis1_arm64.deb \
                     ./libfprint-2-dev_1.94.100-99egis1_arm64.deb \
                     ./gir1.2-fprint-2.0_1.94.100-99egis1_arm64.deb
...
Setting up libfprint-2-2 (1:1.94.100-99egis1) ...
...

$ dpkg-query -W -f='${Version}' libfprint-2-2
1:1.94.100-99egis1

$ strings /usr/lib/aarch64-linux-gnu/libfprint-2.so.2.0.0 | grep -i egis0576
FpDeviceEgis0576
egis0576
libfprint-egis0576
EGIS0576_NO_CALIBRATE
egis0576-capture
../libfprint/drivers/egis0576.c
No valid egis0576 templates to match
Stored print has no valid egis0576 template
../libfprint/drivers/egis0576/egis0576_proto.c
egis0576_close
```

`lintian` on the built packages reports one warning, left as-is:
`appstream-metadata-missing-modalias-provide` for the udev rule's
`usb:v1C7Ap0576d*` match — the package ships no AppStream metainfo advertising
that modalias. Fedora's `.spec` has the same gap (no AppStream entry for the
sensor either), so this is not a regression from packaging for Debian; adding
one is a separate, independent piece of work if this ever needs an AppStream
listing. Adding `gir1.2-gobject-2.0-dev` and `gir1.2-gio-2.0-dev` to
`Build-Depends`, and dropping `${shlibs:Depends}` from `gir1.2-fprint-2.0`'s
`Depends` (that package ships no ELF binary needing a shlibs substitution),
fixed the three cosmetic build warnings an earlier trial run of this same
strategy hit; the build above shows none of them.

## Releasing a new driver version

All three recipes pin the driver by **tag** — none follows `main` — so a
driver release means bumping every one of them, or users keep getting the old
one:

| File | What to bump |
|---|---|
| `packaging/fedora/libfprint.spec` | `%global egis_tag` **and** the `Release:` suffix (`egisN`), plus a `%changelog` entry |
| `packaging/aur/PKGBUILD` | the `#tag=` in `source=`, and reset `pkgrel=1` |
| `packaging/debian/changelog` | a new entry at the top (`dch -i` or by hand), bumping the `-99egisN` revision |

## Release CI (GitHub Actions)

`.github/workflows/release.yml` builds all three packages in containers and
attaches them to the GitHub Release for a tag, as a convenience alongside
COPR, the AUR and a manual Debian build, not a replacement for any of them.

**What it does:** on a push of a tag matching `v*`, a first job checks that
`packaging/fedora/libfprint.spec` (`%global egis_tag`) and
`packaging/aur/PKGBUILD` (the `#tag=` pin) both point at the pushed tag, and
fails the run if either is stale. A matrix job then builds the Fedora RPM (in
a `fedora:43` container, from the unmodified `packaging/fedora/libfprint.spec`),
the Arch package (in an `archlinux:latest` container, from the unmodified
`packaging/aur/PKGBUILD`), and the Debian package set (in a `debian:trixie`
container, using `packaging/debian/build.sh` the same way "Users install
with" above does), each on GitHub's native x86_64 runners with no aarch64
leg. **The Fedora RPM is a Fedora 43 build only** (the container is
`fedora:43`, so `%{?dist}` expands to `.fc43`); COPR remains the supported
install path for other Fedora versions, since its project builds separate
chroots for each one. Each leg then installs its own freshly built package on
top of the distro's stock package (where one ships) inside the same
container, and checks the installed files: the package version, the shared
library's presence and size, the egis0576 driver symbols compiled into it,
and (Fedora and Debian) the suspend/resume hook and udev rule. The produced
`.deb` files are prefixed `debian-trixie-` so they read apart from a later
leg's per-series Ubuntu packages on the Releases page. A failed check fails
that leg's build, so a package this workflow does not verify never reaches
the release. A final job then attaches every built package, plus a generated
`SHA256SUMS` file, to the GitHub Release for that tag, creating the release
if one does not already exist for it. The workflow uses only the built-in
`GITHUB_TOKEN`, requests just the `contents` permission it needs, and never
pushes to COPR or the AUR — publishing to those still follows the steps
above.

If the tagged commit is not an ancestor of the repository's default branch,
the release is created as a draft instead of published, and the job output
says so. A normal release, tagged from the default branch, still publishes
as before.

**Known behaviour:** if a tag is deleted and re-pushed at a different commit,
re-running the workflow replaces the release's assets but leaves its body,
generated notes, and recorded target commit at their original values. Delete
the release itself first if you need those to match the new commit.

**Cutting a release with it:** bump the driver version per the table above,
commit, then push a tag:

```bash
git tag v0.4.5
git push origin v0.4.5
```

The workflow picks up the tag push, builds all three packages, and attaches
them to the `v0.4.5` GitHub Release once the build jobs finish.

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
