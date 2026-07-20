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

## The proper fix (TODO)

The real fix belongs inside the driver: re-establish the TLS session from a
libfprint suspend/resume handler instead of resetting the device externally.
Until that lands, these helpers are the reliable workaround. Contributions
welcome.
