# libfprint driver for the EgisTec EH576 (`1c7a:0576`)

A native Linux [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint)
driver for the **EgisTec (LighTuning) EH576** fingerprint sensor — a match-in-host
image sensor that ships in a number of laptops but has **no vendor Linux driver**.

With this driver the sensor works for real: `fprintd` enrollment, and fingerprint
login / `sudo` / screen-unlock through PAM, on stock GNOME/KDE.

> **Heads-up before you rely on this:** parts of this driver are reverse-engineered
> from Egis' proprietary Windows driver, including the fingerprint matcher. Please
> read [`PROVENANCE.md`](PROVENANCE.md) — it explains exactly what is original work,
> what is reverse-engineered, the licensing, and the (honest) legal grey area.

## Status

| | |
|---|---|
| Enroll / verify / identify | ✅ works via `fprintd` |
| PAM login, `sudo`, unlock | ✅ works (`sufficient`, password fallback intact) |
| Cross-reboot matching | ✅ (per-boot flat-field) |
| Security | measured on one person, one session, 5 fingers × 12 presses, impostors = own adjacent fingers: **0 / 60 false rejects, 0 / 480 false accepts** at the shipped threshold, every impostor scoring exactly 0 ([how, and what that does and does not show](docs/matcher-comparison.md)) |
| Validated on | **four** laptop models (AMD + Intel; Fedora, Arch and Ubuntu); one of the four is a partial pass. See "Tested platforms" below. |

## The sensor, briefly

The EH576 is a small **70×57 px** capacitive/optical image sensor speaking a
simple `EGIS`/`SIGE` command protocol in the clear: the host pulls raw frames and
does feature extraction + matching in software.

This driver implements the whole chain natively: sensor bring-up → frame capture →
per-boot flat-field + preprocessing → Egis' extractor/matcher → libfprint
enroll/verify. Design detail lives in the source comments and
[`PROVENANCE.md`](PROVENANCE.md).

The sensor also has a TLS-PSK session mode, reachable with class request `0x21/9
wValue=0`. **This driver deliberately does not use it**, and neither does the
vendor's own Windows driver for this device — verified against a USB capture of a
working Windows session, which contains no vendor *or class* control transfers at
all — only plaintext `EGIS`/`SIGE` bulk traffic. (The mode switch is a class
request, so ruling out vendor requests alone would not have settled it.)
Entering that mode is a one-way door: the sensor stays in it across USB
autosuspend, a USB port reset and a reboot, and only a `ForceResetDevice`
(`wValue=0x00ff`, which re-enumerates the device) brings it back. On a dual-boot
machine that left Windows unable to start the sensor at all — Device Manager
Code 10 — until the board was fully powered down. Driving the same command
sequence in the clear also measures *better*: finger/no-finger frame variance
1069/140 = 7.6×, against 890/160 = 5.6× through the TLS path on the same unit.

## Universality (does it work on *other* EH576 units?)

The driver is engineered to be **device-independent**, and every per-device
dependency is handled at runtime rather than baked in:

- **Init / protocol** — per-**model** constants, identical for every EH576.
- **Exposure** — a per-device closed-loop calibration runs once at open (binary
  search over register `0x0f` to a fixed no-finger exposure target), so each unit
  self-adjusts.
- **Fixed-pattern noise** — removed by a per-boot host-side flat-field captured
  fresh on every unit.
- **On-chip background/vdm** — neutralized to a model-level constant so no single
  unit's calibration data is embedded.

**Update:** originally all of this was validated on the author's single unit —
that caveat is now retired. An independent tester ran the full stack on a second
EH576 (different laptop, CPU vendor and USB host controller; same sensor revision,
`bcdDevice` 15.72) with
a 17-point PASS ([#2](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/2)).
More reports are still very welcome — success *or* failure.

## Tested platforms

| Laptop | Platform | Sensor `bcdDevice` | Distro | Result | Report |
|---|---|---|---|---|---|
| Lenovo Yoga 7 14ARB7 | AMD Ryzen 7 6800U | 15.72 | Fedora 44 | ✅ full stack (enroll, verify, GDM, sudo, unlock, suspend/resume) | author |
| Lenovo Yoga 7 15ITL5 | Intel Core i5-1135G7 | 15.72 | Fedora 44 | ✅ full stack, 17-point PASS incl. 3× suspend/resume | [#2](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/2) |
| Lenovo IdeaPad Flex 5 16IRU8 | Intel Core i7-1355U | 15.72 | Arch Linux (Omarchy 4.0.2) | ✅ full stack (enroll, verify, sudo/PAM, polkit, lock screen, reboot, 3× suspend/resume); one long-lock stale-claim limitation, see report | [#3](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/3) |
| Lenovo Yoga 6 13ALC6 | AMD Ryzen 7 5700U | 15.72 | Ubuntu 26.04 | ✅ enroll, verify, sudo/PAM, polkit, unlock, suspend/resume (GDM login not tested) | [#4](https://github.com/PHILIPPDEV5396/libfprint-egis0576/pull/4) |

Got a different machine with an EH576? **Please open an issue with your results.**

## Install

> **Upgrading from a release before v0.4.0?** Those builds spoke a TLS-PSK
> transport and left the sensor in a mode it keeps until something resets it — so
> the **first** fingerprint attempt after the upgrade can fail once while the
> driver resets the sensor and it re-enumerates (~0.5 s). The next attempt works.
> **No re-enrollment is needed**: existing prints keep matching, because the
> exposure target and register are unchanged.
>
> The same change fixes dual-boot: earlier releases left the sensor in a state
> where Windows' own driver failed to start it (Device Manager **Code 10**) until
> the machine was fully powered down. v0.4.0 never enters that mode.

> **Upgrading from any release before v0.4.3 (v0.1.0 – v0.4.2)?** Those
> releases could hang: a fingerprint verify that never answers, and from then
> on **every** attempt refused with `"Device was already claimed"` until
> `fprintd` is restarted (the lock screen falls back to the password). The
> cause was a lost wakeup between the driver's capture thread and the fprintd
> main loop inside gusb's synchronous transfer helpers, narrow enough to
> survive months of use before it was caught on hardware. **v0.4.3 fixes it**
> — the capture thread no longer depends on the main loop at all (write-up in
> [`docs/worker-thread.md`](docs/worker-thread.md)). No re-enrollment is
> needed; capture and matching are unchanged.


The driver is compiled *into* `libfprint-2.so` (it is not a loadable module), so
installing it means installing a libfprint that includes it, replacing the distro's.
The stock `fprintd` then loads it through the unchanged ABI (no `fprintd` rebuild).

### One command (packaged)

**Fedora (COPR):**
```bash
sudo dnf copr enable PHILIPPDEV5396/libfprint-egis0576
sudo dnf upgrade 'libfprint*'
sudo systemctl restart fprintd
```

**Arch (PKGBUILD, not on AUR yet):**
```bash
git clone https://github.com/PHILIPPDEV5396/libfprint-egis0576.git
cd libfprint-egis0576/packaging/aur
makepkg -si
sudo systemctl restart fprintd
```

Unlike the Fedora package, the PKGBUILD installs **neither** the suspend/resume
sleep hook nor the no-autosuspend udev rule — install both by hand, see
[`integration/`](integration/).

Packaging sources and how they're published live in [`packaging/`](packaging/).
Both replace the stock `libfprint` (same soname); your other fingerprint hardware
keeps working because the build includes the full **upstream** default driver set.
One caveat on Fedora: the COPR build is pristine upstream, so it does not carry
Fedora's *downstream* `egis_etu905` driver (`1c7a:05ae` / `1c7a:9201`). That only
matters if you also own one of those readers — see [`packaging/`](packaging/) for a
rebased patch that keeps both.

> Not published to a repo yet? Build from source below — it's the same result.

## Building from source (any distro / development)

A libfprint driver is compiled *into* `libfprint-2.so` (it is not a loadable
module), so this builds a full libfprint that includes the driver. It does **not**
rebuild `fprintd` — the stock daemon loads the new library through its unchanged ABI.

### 1. Install build dependencies

You need the usual libfprint build stack, which includes **OpenSSL ≥ 3.0** —
upstream's own `uru4000` driver requires it (`meson.build`: `'uru4000' : [ 'openssl' ]`).
The egis0576 driver itself links no crypto at all; it speaks the sensor's plaintext
protocol over gusb.

- **Fedora:** `sudo dnf install git meson ninja-build gcc pkgconf-pkg-config glib2-devel libgusb-devel openssl-devel gobject-introspection-devel nss-devel systemd-devel libgudev-devel pixman-devel cairo-gobject-devel`
- **Arch:** `sudo pacman -S --needed git meson ninja gcc pkgconf glib2 libgusb openssl gobject-introspection nss systemd-libs libgudev pixman cairo`
- **Debian/Ubuntu:** `sudo apt install git meson ninja-build build-essential pkg-config libglib2.0-dev libgusb-dev libssl-dev libgirepository1.0-dev libnss3-dev libsystemd-dev systemd-dev libgudev-1.0-dev libpixman-1-dev libcairo2-dev gtk-doc-tools`
- **openSUSE:** `sudo zypper install git meson ninja gcc pkgconf glib2-devel libgusb-devel libopenssl-devel gobject-introspection-devel mozilla-nss-devel systemd-devel libgudev-1_0-devel pixman-devel cairo-devel`

### 2. Build + install

```bash
./install.sh
```

This clones libfprint **1.94.10**, applies the driver, builds the full default
driver set (so any other fingerprint hardware you have keeps working), and installs
to `/usr/local` (override with `PREFIX=/usr ./install.sh`).

**Make sure fprintd loads *this* libfprint**, not the distro's:

```bash
ldconfig -p | grep libfprint-2      # should point at your PREFIX
sudo systemctl restart fprintd
```

If it still points at the distro copy, add `PREFIX/lib` (or `lib64`) to
`/etc/ld.so.conf.d/`, run `sudo ldconfig`, and restart fprintd. (Installing with
`PREFIX=/usr` replaces the distro library outright — simplest, but a distro update
can overwrite it, so you'd re-run `install.sh` after libfprint updates.)

**Experimental — clean-room matcher.** The driver's default matcher is the
vendor's own, machine-translated from the Windows driver, which is what keeps
this driver out of upstream libfprint. Thaddeus Stepanovich's LGPL clean-room
correlation matcher can be selected at build time instead, so the two can be
compared on the same captures:

```bash
EGIS0576_MESON_ARGS="-Degis0576_matcher=cleanroom" ./install.sh
```

Templates enrolled under one matcher are rejected by the other, so switching
means re-enrolling. **Measured on the same 714-frame dataset as the vendor
matcher** ([`docs/matcher-comparison.md`](docs/matcher-comparison.md)): at its
published threshold it rejects 35 % of genuine presses (vendor: 0 %), and its
genuine and impostor scores overlap (EER 15 %), so it is not yet a drop-in
replacement — it is the starting point for one. Details in
[`driver/egis0576/egis_engine_cleanroom.c`](driver/egis0576/egis_engine_cleanroom.c).

## Enrolling

```bash
fprintd-enroll                 # or: Settings → Users → Fingerprint Login
fprintd-verify                 # test it
```

**Enroll with coverage:** press the same finger at deliberately varied positions
(centre, up/down, left/right, slight rotations) across the enrollment stages. On a
sensor this small, a template built from one position matches poorly.

## Enabling fingerprint auth

> These steps assume `libfprint` + `fprintd` are installed and your fingerprint is
> already enrolled.

### How PAM fingerprint auth works (read first)

Enabling fingerprint login means adding one line to the PAM **auth** stack:

```
auth    sufficient    pam_fprintd.so
```

- It must sit **above `pam_unix.so`** (the password module). PAM evaluates auth
  modules top-to-bottom.
- **`sufficient`** = if the fingerprint matches, auth succeeds immediately; if it
  fails, times out, or nothing is enrolled, PAM **falls through** to the password.
  This keeps your password as a working fallback.
- **Do not use `required`** for `pam_fprintd` unless you *want* fingerprint-AND-
  password (2-factor) and accept that a missing/broken reader can lock you out. For
  normal "fingerprint OR password", use `sufficient` and keep the `pam_unix` line.
- `fprintd` is **D-Bus/systemd-activated** on all distros — it starts on demand, so
  `systemctl enable --now fprintd` is normally unnecessary.
- **Keyrings still need your password.** GNOME Keyring / KWallet are unlocked by the
  login password, so a fingerprint-only login still prompts for the keyring once.

### Fedora

```bash
sudo authselect enable-feature with-fingerprint
```

(`fprintd-pam` provides the module.) Wires `pam_fprintd.so` into the system-auth /
password-auth stacks for login, sudo, and the display manager.

### Arch Linux (and Manjaro, EndeavourOS, …)

- **Packages:** `fprintd` (provides the daemon **and** `pam_fprintd.so`) + `libfprint`.
- Arch has no PAM helper — edit by hand. Add to the **top of the auth section** of
  `/etc/pam.d/system-local-login`:
  ```
  auth      sufficient pam_fprintd.so
  auth      include    system-login
  ```
  For `sudo`, add the same `auth sufficient pam_fprintd.so` at the top of the auth
  section of `/etc/pam.d/sudo`.
- GDM and KDE/Plasma have built-in fingerprint handling (don't edit `kde-fingerprint`).
- **Keep an open root shell while editing PAM** so a mistake can't lock you out.
- Ref: [ArchWiki – fprint](https://wiki.archlinux.org/title/Fprint)

### Debian / Ubuntu (and Mint, Pop!\_OS, …)

```bash
sudo apt install fprintd libpam-fprintd
sudo pam-auth-update --enable fprintd      # or run `sudo pam-auth-update` and tick the box
```

Installing `libpam-fprintd` *registers* the profile but does **not** enable it —
the command above inserts `auth sufficient pam_fprintd.so` into
`/etc/pam.d/common-auth` (included by login, sudo, the DM). Add `--force` if it
refuses due to local PAM edits; disable later with `--remove fprintd`.
Ref: [Debian Wiki – fingerprint authentication](https://wiki.debian.org/SecurityManagement/fingerprint%20authentication)

### openSUSE (Tumbleweed / Leap)

```bash
sudo zypper install fprintd fprintd-pam
sudo pam-config -a --fprintd               # disable later: pam-config -d --fprintd
```

Adds `auth sufficient pam_fprintd.so` to the `pam-config`-managed common auth stack.
Prefer `pam-config` over hand-editing (it regenerates those files).
Ref: [openSUSE SDB – Using fingerprint authentication](https://en.opensuse.org/SDB:Using_fingerprint_authentication)

## Suspend / resume

The sensor is brought up at device-open. Across a system suspend — especially
**s2idle**, where the USB device stays powered and is not re-enumerated — it comes
back with its capture pipeline unusable. On resume the next fingerprint attempt
(typically the lock screen after wake) can then block on it and, on some systems
(observed on AMD Rembrandt laptops that only offer `mem_sleep=s2idle`), **hang the
unlock screen hard**.

Two separate things are at play, and it took a long investigation to tell them
apart (written up in full in [`docs/suspend-resume.md`](docs/suspend-resume.md)):

- **The hard freeze** is prevented **in the driver**: every bulk read is bounded
  by a timeout, so talking to an unresponsive sensor fails fast to a clean error
  instead of spinning forever.
- **"No fingerprint offered after resume"** (the lock screen drops to password) is
  **not this driver's bug at all.** It is a well-known, still-unfixed
  **gnome-shell/fprintd** issue: `fprintd` keeps a stale device *claim* across
  suspend (`"Device was already claimed"`), so the post-resume claim is refused —
  libfprint is never even reached. See gnome-shell
  [#7791](https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/7791) and Ubuntu
  [#2067135](https://bugs.launchpad.net/bugs/2067135) (it hits ThinkPads, Framework
  laptops, etc., on unrelated readers).

The community-standard fix — **restart `fprintd` around suspend** — is what the two
helpers in [`integration/`](integration/) do (a systemd-sleep hook that stops
`fprintd` before sleep and re-enumerates the sensor on resume, plus a udev rule
disabling USB autosuspend). `install.sh` installs them and you should **keep them
installed**; they are the correct fix for the upstream bug, not a workaround for a
driver shortcoming. Details and the full investigation:
[`integration/README.md`](integration/README.md) and
[`docs/suspend-resume.md`](docs/suspend-resume.md).

## Usability & security notes

- **The wire is plaintext, by design.** Traffic to the sensor is the vendor's own
  `EGIS`/`SIGE` protocol, exactly as the shipped Windows driver sends it, and the
  driver links no crypto. The sensor does have a TLS-PSK session mode; using it
  would not have bought a security property here — the key is a fixed per-*model*
  constant, the channel would terminate in the same userspace process that then
  handles the decrypted image anyway, and matching is host-side, so the template
  is on disk regardless. On Linux the meaningful boundary is match-on-chip, which
  this sensor does not offer. The variance figures quoted above, and what else was
  measured and found to be at its limit, are in
  [`docs/sensor-tuning.md`](docs/sensor-tuning.md).

- **Every capture runs on a worker thread**, never on the fprintd main loop
  (only the one-time bring-up in `open()` runs there), and the transport drives
  gusb's async API on a private `GMainContext` rather than gusb's synchronous
  wrappers. Why that distinction matters — the lost-wakeup hang it fixed in
  v0.4.3 — is in [`docs/worker-thread.md`](docs/worker-thread.md).

- **Press firmly, flat, centred, and hold ~2 s.** Verification scores every frame
  while the finger is down and takes the best; light or brief taps on a 70×57 sensor
  can score zero.
- The match **threshold is strict** and not user-tunable by design — the priority is
  rejecting impostors (including adjacent same-hand fingers), not maximum convenience.
- Templates are stored by fprintd under `/var/lib/fprint/`, protected by filesystem
  permissions (as with every libfprint driver).
- Re-enroll after anything that changes capture conditions materially (the exposure
  calibration converges to the same fixed target every boot, so normal reboots are fine).

## Troubleshooting

- **The first verify after upgrading fails, the next one works.** Expected, once:
  the driver found the sensor in the session mode an older release left behind,
  reset it, and the device re-enumerated. See the note under [Install](#install).

- **`fprintd` still uses the old driver / device not found:** confirm
  `ldconfig -p | grep libfprint-2` points at your `PREFIX`, then
  `sudo systemctl restart fprintd`.
- **Enrollment "fails to capture":** press more firmly and hold; ensure no finger is
  on the sensor during the first ~2 s (baseline + exposure calibration run then).
- **Verify always fails after it worked:** re-enroll with coverage (see "Enrolling").
- **Debug logging:** `sudo G_MESSAGES_DEBUG=all fprintd` (stop the service first) or
  `journalctl -u fprintd -f`.

## Contributing

Most wanted, in order:

1. **A clean-room fingerprint matcher** good enough to distinguish adjacent same-hand
   fingers at 70×57 — this is what blocks clean libfprint upstreaming. See the
   ["Why the reverse-engineered matcher (and not a clean-room one)?"](PROVENANCE.md#why-the-reverse-engineered-matcher-and-not-a-clean-room-one)
   in `PROVENANCE.md`.
2. **Test reports from other physical EH576 units** (see "Universality").
3. Packaging (AUR, .deb, DKMS-style), other distro build recipes.

## License & legal

Original driver code: **LGPL-2.1-or-later** (see [`LICENSE`](LICENSE)), matching
libfprint. Reverse-engineered portions and the full legal picture:
[`PROVENANCE.md`](PROVENANCE.md). The original Egis Windows driver and its
decompilation are **not** included and must not be committed here.

## Credits

Reverse-engineered and ported to libfprint by the repository author, with the goal
of making already-owned hardware usable on Linux. Built on the excellent
[libfprint](https://gitlab.freedesktop.org/libfprint/libfprint) and `fprintd`.
