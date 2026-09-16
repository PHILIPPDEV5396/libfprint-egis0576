# libfprint driver for the EgisTec EH576 (`1c7a:0576`)

A native Linux [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint)
driver for the **EgisTec (LighTuning) EH576** fingerprint sensor — a match-on-host
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
| Accuracy | measured on **three units, one person each** (5 fingers × 12 presses per run, impostors = own adjacent fingers). False rejects at the shipped threshold: **0 / 60, 2 / 60 and 37 / 60 presses** — genuine acceptance is strongly run-dependent and no single figure stands for the driver. False accepts: **0 / 480 on each of the three** — no impostor comparison produced a non-zero vendor score in any run (480 comparisons per run, but only 60 distinct presses, each scored against the 8 templates not built from its own finger, so they are not 480 independent trials). ([all three runs, and what they do and do not show](docs/matcher-comparison.md)) |
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
- **Exposure** — a per-device closed-loop calibration (binary search over register
  `0x0f` to a fixed no-finger exposure target) is measured once per fprintd process
  and re-applied after every sensor bring-up (the vendor init sequence resets the
  register, and the sensor does not retain it), so each unit self-adjusts.
- **Fixed-pattern noise** — removed by a per-boot host-side flat-field captured
  fresh on every unit.
- **On-chip background/vdm** — neutralized to a model-level constant so no single
  unit's calibration data is embedded.

**Update — it works on other units.** Originally all of this was validated on
the author's single unit; for *functional* operation that caveat is retired.
Independent testers have run the full stack on other EH576 units (different
laptops, CPU vendors and USB host controllers; same sensor revision,
`bcdDevice` 15.72): the first with a 17-point PASS
([#2](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/2)), the rest
in the table above. More reports are still very welcome — success *or* failure.

**Accuracy is a separate axis, and there the caveat stands.** Three accuracy
runs now exist — the author's plus sam-dant's and irvingpop's in
[#5](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/5) — and they
disagree sharply: with the vendor matcher, 0, 2 and 37 of 60 genuine presses
were rejected at the shipped threshold (on the worst run 33 of those 37 scored
exactly 0). Each run is one person on one unit in one session, so person and
unit change together and nothing in the data says which of the two the spread
comes from. Exposure calibration and template size both order the runs the
*wrong* way; the one measurement that does track the failure is how often the
matcher found an enrolment frame already covered by the template — 19, 10 and
none across the three runs — which is suggestive and nothing more. What this means practically: the driver working on your machine does not
imply the accuracy figures from another machine, so **if you are going to rely
on fingerprint login, try it before you do** — and please
[run the kit](tools/accuracy/README.md) and post the result. Detail and all
three tables: [`docs/matcher-comparison.md`](docs/matcher-comparison.md).
Note that the worst of the three runs is on a unit whose *functional* report in
the table below is a pass, though a qualified one (GDM login not tested there);
the kit measures an offline protocol with thinner enrolment than the driver's,
not the driver's own login success rate.

## Tested platforms

| Laptop | Platform | Sensor `bcdDevice` | Distro | Result | Report |
|---|---|---|---|---|---|
| Lenovo Yoga 7 14ARB7 | AMD Ryzen 7 6800U | 15.72 | Fedora 44 | ✅ full stack (enroll, verify, GDM, sudo, unlock, suspend/resume) | author |
| Lenovo Yoga 7 15ITL5 | Intel Core i5-1135G7 | 15.72 | Fedora 44 | ✅ full stack, 17-point PASS incl. 3× suspend/resume | [#2](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/2) |
| Lenovo IdeaPad Flex 5 16IRU8 | Intel Core i7-1355U | 15.72 | Arch Linux (Omarchy 4.0.2) | ✅ full stack (enroll, verify, sudo/PAM, polkit, lock screen, reboot, 3× suspend/resume); the long-lock stale-claim failure from the original report is gone on v0.4.5, re-tested over a 10 h 46 min locked screen and 1265 verification timeouts with zero refused claims | [#3](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/3) |
| Lenovo Yoga 6 13ALC6 | AMD Ryzen 7 5700U | 15.72 | Ubuntu 26.04 | ✅ enroll, verify, sudo/PAM, polkit, unlock, suspend/resume (GDM login not tested) | [#4](https://github.com/PHILIPPDEV5396/libfprint-egis0576/pull/4) |
| Lenovo IdeaPad Flex 5 14ITL05 | Intel (11th gen) | 15.72 | Zorin OS 18.1 | ⚠️ enroll, verify and screen-lock unlock work; unlock after suspend needs an extra activation gate on top of the shipped hook, see report | [#3](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/3) |

Got a different machine with an EH576? **Please open an issue with your results.**

## Install

> **Upgrading from a release before v0.4.0?** Those builds spoke a TLS-PSK
> transport and left the sensor in a mode it keeps until something resets it — so
> the **first** fingerprint attempt after the upgrade can fail once: the driver
> finds the sensor in that mode, issues `ForceResetDevice` and fails that open;
> the sensor stays on the bus until the next transfer to it, then drops off and
> re-enumerates (~0.4 s), after which attempts work normally. Details in
> [`docs/sensor-tuning.md` §6](docs/sensor-tuning.md#6-forceresetdevice-takes-effect-on-the-next-transfer).
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


How the driver reaches `fprintd` depends on the distribution. On Fedora, Arch and
Debian it is compiled *into* `libfprint-2.so`, so installing it means installing a
libfprint that includes it, replacing the distro's; the stock `fprintd` then loads it
through the unchanged ABI (no `fprintd` rebuild). On **Ubuntu** nothing is replaced:
every Ubuntu series ships `libfprint-2-tod1`, and the package in
[`packaging/ubuntu-tod/`](packaging/ubuntu-tod/) builds the driver as a loadable TOD
module that the distro's own libfprint picks up
([#9](https://github.com/PHILIPPDEV5396/libfprint-egis0576/pull/9), contributed by
[irvingpop](https://github.com/irvingpop)).

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

As noted under [Install](#install), the driver is compiled *into* `libfprint-2.so`,
so this builds a full libfprint that includes it; `fprintd` is not rebuilt.

### 1. Install build dependencies

You need the usual libfprint build stack, which includes **OpenSSL ≥ 3.0** —
upstream's own `uru4000` driver requires it (`meson.build`: `'uru4000' : [ 'openssl' ]`).
The egis0576 driver itself links no crypto at all; it speaks the sensor's plaintext
protocol over gusb.

- **Fedora:** `sudo dnf install git meson ninja-build gcc pkgconf-pkg-config glib2-devel libgusb-devel openssl-devel gobject-introspection-devel nss-devel systemd-devel libgudev-devel pixman-devel cairo-gobject-devel gtk-doc`
- **Arch:** `sudo pacman -S --needed git meson ninja gcc pkgconf glib2 libgusb openssl gobject-introspection nss systemd-libs libgudev pixman cairo gtk-doc`
- **Debian/Ubuntu:** `sudo apt install git meson ninja-build build-essential pkg-config libglib2.0-dev libgusb-dev libssl-dev libgirepository1.0-dev libnss3-dev libsystemd-dev systemd-dev libgudev-1.0-dev libpixman-1-dev libcairo2-dev gtk-doc-tools`
- **openSUSE:** `sudo zypper install git meson ninja gcc pkgconf glib2-devel libgusb-devel libopenssl-devel gobject-introspection-devel mozilla-nss-devel systemd-devel libgudev-1_0-devel pixman-devel cairo-devel gtk-doc`

### 2. Build + install

```bash
./install.sh
```

This clones libfprint **1.94.100**, applies the driver, builds the full default
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
means re-enrolling. **Measured on the same captures as the vendor matcher, on
all three accuracy-tested units**
([`docs/matcher-comparison.md`](docs/matcher-comparison.md)): at its published
threshold it rejects **35 %, 73.3 % and 93.3 %** of genuine presses (the vendor
matcher on the same captures: 0 %, 3.33 %, 61.7 %), its genuine and impostor
scores overlap on every run (EER 15 %, 35 %, 45 %), and on one of the three it
produced the project's first measured false accepts — 5 of 480 impostor
comparisons at or above the threshold, where the vendor matcher scored 0 on all
480. It is not a drop-in replacement — it is the starting point for one. Details in
[`driver/egis0576/egis_engine_cleanroom.c`](driver/egis0576/egis_engine_cleanroom.c);
reproduce the measurement on your own unit with
[`tools/accuracy/`](tools/accuracy/README.md).

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
  can score zero. How much this matters clearly differs between people and units:
  in one of the three accuracy runs 33 of 60 genuine presses scored exactly 0
  ([`docs/matcher-comparison.md`](docs/matcher-comparison.md)).
- The match **threshold is strict** and not user-tunable by design — the priority is
  rejecting impostors (including adjacent same-hand fingers), not maximum convenience.
  Lowering it would not have rescued that run either: 33 of its 37 failing presses
  scored 0, which no threshold recovers.
- Templates are stored by fprintd under `/var/lib/fprint/`, protected by filesystem
  permissions (as with every libfprint driver).
- Re-enroll after anything that changes capture conditions materially (the exposure
  calibration converges to the same fixed target every boot, so normal reboots are fine).

## Troubleshooting

- **The first verify (or two) after upgrading fails, then it works.** Expected, once:
  the driver found the sensor in the session mode an older release left behind
  and issued a reset; the sensor re-enumerates on the next access
  ([`docs/sensor-tuning.md` §6](docs/sensor-tuning.md#6-forceresetdevice-takes-effect-on-the-next-transfer)).
  See the note under [Install](#install).

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
3. **Accuracy reports from your own fingers** — especially if the result is
   poor; the three runs collected so far disagree sharply and more points are
   the only way to understand that. Run the
   [`tools/accuracy/`](tools/accuracy/README.md) kit (about 15 minutes; only the
   `results.json` summary is shared, never frames) and open an issue titled
   `accuracy: <laptop model>` as its README asks — collected under
   [#5](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/5).
4. **Packaging** — AUR, Debian/Ubuntu `.deb` and release CI (coordinated in
   [#6](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/6)), a way
   to rebuild automatically when the distro updates libfprint, other distro
   build recipes.

## License & legal

Original driver code: **LGPL-2.1-or-later** (see [`LICENSE`](LICENSE)), matching
libfprint. Reverse-engineered portions and the full legal picture:
[`PROVENANCE.md`](PROVENANCE.md). The original Egis Windows driver and its
decompilation are **not** included and must not be committed here.

## Credits

Reverse-engineered and ported to libfprint by the repository author, with the goal
of making already-owned hardware usable on Linux. The optional clean-room
correlation matcher (`driver/egis0576/tsteppy/`, LGPL-2.1-or-later) is the work of
Thaddeus Stepanovich. The suspend/resume activation gate in
[`integration/`](integration/) implements a design diagnosed, built and tested by
[sam-dant](https://github.com/sam-dant/egis0576-resume-workaround). The Debian and
Ubuntu packaging, the Ubuntu TOD module and the release CI's version checks were
contributed by [irvingpop](https://github.com/irvingpop), who maintains that part of
the packaging stack. Prior art
this project started from is listed in
[PROVENANCE.md §3](PROVENANCE.md#3-prior-art-by-others); the people who tested the
driver on their hardware are named in the "Tested platforms" table above. Built on
the excellent [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint) and
`fprintd`.
