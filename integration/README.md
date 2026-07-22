# Suspend / resume integration

Two small, fully reversible helpers that keep the EH576 working across
suspend/resume. Both are installed automatically by [`../install.sh`](../install.sh);
this directory documents them and lets you install them by hand.

## Why

The driver establishes a **TLS-PSK session** to the sensor when the device is
opened. Across a system suspend — especially **s2idle**, where the USB device
stays powered and the kernel does *not* re-enumerate it — that session goes
stale. On resume the next fingerprint attempt (typically the lock screen right
after wake) blocks on the dead session. On some systems (observed on AMD
Rembrandt laptops that only offer `mem_sleep=s2idle`) that block **hangs the
unlock screen hard**, forcing a reboot.

## What these do

| File | Installs to | Effect |
|---|---|---|
| `50-egis0576-fp-resume.sh` | `/usr/lib/systemd/system-sleep/` | Stops `fprintd` before suspend; forces the sensor to re-enumerate on resume, so the driver does a fresh handshake on next use. |
| `60-egis0576-fp-nosuspend.rules` | `/etc/udev/rules.d/` | Disables USB autosuspend for the sensor so its session isn't dropped while idle. |

Fingerprint stays enabled everywhere — login, `sudo`, **and** unlock. Nothing is
disabled; the sensor is just reset cleanly around sleep.

## Manual install

```bash
sudo install -m 0755 50-egis0576-fp-resume.sh    /usr/lib/systemd/system-sleep/
sudo install -m 0644 60-egis0576-fp-nosuspend.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger --attr-match=idVendor=1c7a
```

Revert by deleting the two files and reloading udev rules.

## Test it

Lock the screen, suspend, wait a bit, wake, and unlock with your fingerprint. If
the unlock is smooth, you're set. If it *still* freezes, the cause is not the
sensor but a generic GPU/compositor s2idle resume bug on your platform — the
hook won't hurt, but that needs a separate fix.

## The proper fix (in the driver)

The driver now re-establishes its TLS-PSK session across suspend by itself: a
libfprint `suspend`/`resume` handler flags the session stale and cancels any
in-flight capture, and the capture worker thread re-runs the handshake
(`egis_tls_free`/`new`/`open`) either eagerly on the next capture or as a bounded
retry when the first post-resume frame fails. `read_record` has a wall-clock
deadline so a dead session always fails fast to a password fallback rather than
hanging the unlock screen.

## Keep the hook installed — it is *not* redundant

Measured on real hardware (Lenovo Yoga 7 14ARB7, s2idle, GNOME 50, hook
deliberately disabled, suspend triggered while a verify was in flight):

- The in-driver recovery does exactly what it promises. The `suspend` handler
  fires (`suspend: invalidating TLS session, cancelling in-flight capture`), the
  in-flight verify is cancelled cleanly, and after resume the unlock screen
  **falls back to the password prompt without ever hanging** — the original
  freeze is gone.
- **But** GNOME does not re-arm the fingerprint after that suspend-time
  cancellation. So the first unlock following a *suspend-while-armed* offers the
  password only; the reader returns on the next fresh lock/unlock. (This is the
  upper-stack "re-issue after cancel" dependency — not something the driver can
  force.)

The hook closes exactly that gap: re-enumerating the sensor makes fprintd open
the device again, so the fingerprint is available immediately on the post-resume
unlock screen. The two mechanisms are **complementary, not duplicated**:

| | guarantees |
|---|---|
| in-driver `suspend`/`resume` + bounded `read_record` | the unlock screen never hangs — on any platform, with or without the hook |
| `50-…-resume.sh` hook | fingerprint is usable again immediately after resume |

The `60-…-nosuspend.rules` udev rule (disable USB autosuspend for 1c7a:0576)
stays installed regardless.
