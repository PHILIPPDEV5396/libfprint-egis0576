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
| Security | genuine finger matches; other fingers (incl. adjacent same-hand) rejected at a strict threshold |
| Validated on | **one** physical unit (Lenovo Yoga 7 14ARB7, Fedora). See "Universality" below. |

## The sensor, briefly

The EH576 is a small **70×57 px** capacitive/optical image sensor behind a
**TLS-PSK** secure channel (the host is the TLS *server*, the device the client).
After the handshake it speaks a simple `EGIS`/`SIGE` command protocol; the host
pulls raw frames and does feature extraction + matching in software.

This driver implements the whole chain natively: TLS-PSK (OpenSSL) → sensor init →
frame capture → per-boot flat-field + preprocessing → Egis' extractor/matcher →
libfprint enroll/verify. Design detail lives in the source comments and
[`PROVENANCE.md`](PROVENANCE.md).

## Universality (does it work on *other* EH576 units?)

The driver is engineered to be **device-independent**, and every per-device
dependency is handled at runtime rather than baked in:

- **PSK / init / protocol** — per-**model** constants, identical for every EH576.
- **Gain** — a per-device closed-loop calibration runs once at open (binary search
  over the fine-gain register to a fixed no-finger exposure target), so each unit
  self-adjusts.
- **Fixed-pattern noise** — removed by a per-boot host-side flat-field captured
  fresh on every unit.
- **On-chip background/vdm** — neutralized to a model-level constant so no single
  unit's calibration data is embedded.

**Honest caveat:** all of that was validated on the author's single unit. True
cross-device operation cannot be *proven* without a second physical EH576. If you
have one, **please open an issue with your results** — success or failure. That
report is the most useful thing you can contribute after a clean-room matcher.

## Install

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

**Arch (AUR):**
```bash
yay -S libfprint-egis0576
sudo systemctl restart fprintd
```

Packaging sources and how they're published live in [`packaging/`](packaging/).
Both replace the stock `libfprint` (same soname); your other fingerprint hardware
keeps working because the build includes the full default driver set.

> Not published to a repo yet? Build from source below — it's the same result.

## Building from source (any distro / development)

A libfprint driver is compiled *into* `libfprint-2.so` (it is not a loadable
module), so this builds a full libfprint that includes the driver. It does **not**
rebuild `fprintd` — the stock daemon loads the new library through its unchanged ABI.

### 1. Install build dependencies

You need the usual libfprint build stack plus **OpenSSL/libcrypto ≥ 3.0**:

- **Fedora:** `sudo dnf install git meson ninja-build gcc pkgconf-pkg-config glib2-devel libgusb-devel openssl-devel gobject-introspection-devel nss-devel systemd-devel libgudev-devel pixman-devel cairo-gobject-devel`
- **Arch:** `sudo pacman -S --needed git meson ninja gcc pkgconf glib2 libgusb openssl gobject-introspection nss systemd-libs libgudev pixman cairo`
- **Debian/Ubuntu:** `sudo apt install git meson ninja-build build-essential pkg-config libglib2.0-dev libgusb-dev libssl-dev libgirepository1.0-dev libnss3-dev libsystemd-dev libgudev-1.0-dev libpixman-1-dev libcairo2-dev`
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

The driver holds a **TLS-PSK session** to the sensor, established at device-open.
Across a system suspend — especially **s2idle**, where the USB device stays
powered and is not re-enumerated — that session goes stale. On resume the next
fingerprint attempt (typically the lock screen after wake) can then block on the
dead session and, on some systems (observed on AMD Rembrandt laptops that only
offer `mem_sleep=s2idle`), **hang the unlock screen hard**.

The driver **self-heals** this: an in-driver `suspend`/`resume` handler flags the
TLS session stale and cancels any in-flight capture, and the capture worker
re-runs the handshake on the next capture (with a wall-clock deadline in the
record reader so a dead session always fails fast to a password fallback instead
of hanging the unlock screen).

As belt-and-suspenders, two small reversible helpers in
[`integration/`](integration/) — a systemd-sleep hook that re-enumerates the
sensor around sleep, and a udev rule disabling its USB autosuspend — are still
installed by `install.sh` and do no harm. They remain the proven fallback until
you've verified fingerprint unlock across several s2idle cycles on your hardware;
see [`integration/README.md`](integration/README.md) to remove the hook once
you're confident.

## Usability & security notes

- **Press firmly, flat, centred, and hold ~2 s.** Verification scores every frame
  while the finger is down and takes the best; light or brief taps on a 70×57 sensor
  can score zero.
- The match **threshold is strict** and not user-tunable by design — the priority is
  rejecting impostors (including adjacent same-hand fingers), not maximum convenience.
- Templates are stored by fprintd under `/var/lib/fprint/`, protected by filesystem
  permissions (as with every libfprint driver).
- Re-enroll after anything that changes capture conditions materially (the gain
  calibration is deterministic per boot, so normal reboots are fine).

## Troubleshooting

- **`fprintd` still uses the old driver / device not found:** confirm
  `ldconfig -p | grep libfprint-2` points at your `PREFIX`, then
  `sudo systemctl restart fprintd`.
- **Enrollment "fails to capture":** press more firmly and hold; ensure no finger is
  on the sensor during the first ~2 s (baseline + gain calibration run then).
- **Verify always fails after it worked:** re-enroll with coverage (see "Enrolling").
- **Debug logging:** `sudo G_MESSAGES_DEBUG=all fprintd` (stop the service first) or
  `journalctl -u fprintd -f`.

## Contributing

Most wanted, in order:

1. **A clean-room fingerprint matcher** good enough to distinguish adjacent same-hand
   fingers at 70×57 — this is what blocks clean libfprint upstreaming. See the
   "Why the RE'd matcher" section in [`PROVENANCE.md`](PROVENANCE.md).
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
