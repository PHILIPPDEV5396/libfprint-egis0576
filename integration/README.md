# Suspend / resume integration

Two small, fully reversible helpers that keep the EH576 usable across
suspend/resume. Both are installed automatically by [`../install.sh`](../install.sh);
this directory documents them and lets you install them by hand.

**Keep them installed.** They are the standard fix for a real upstream
gnome-shell/fprintd bug, not a crutch for a driver shortcoming. The full
investigation is in [`../docs/suspend-resume.md`](../docs/suspend-resume.md).

## Why

Two independent problems, only one of which is even in this driver's reach:

1. **Hard freeze** (historic): a suspend fired while the sensor was armed left the
   driver talking to an unresponsive sensor, hanging the unlock screen. **Fixed in the
   driver** by a bounded timeout on every bulk read (`egis0576_proto.c`) — not by these helpers.
2. **"No fingerprint after resume"**: on ~40 % of resumes the lock screen offers
   only the password. The journal shows the cause has nothing to do with the
   sensor:

   ```
   Requesting authorization … to call method 'Claim' for device '… EH576'
   Authorization denied … : Device was already claimed
   ```

   `fprintd` still holds a **stale claim** taken by gnome-shell before suspend and
   never released; the post-resume claim is refused and libfprint is never
   reached. This is a known, unfixed gnome-shell/fprintd bug (gnome-shell
   [#7791](https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/7791), Ubuntu
   [#2067135](https://bugs.launchpad.net/bugs/2067135)), not specific to the EH576.

## What these do

| File | Installs to | Effect |
|---|---|---|
| `50-egis0576-fp-resume.sh` | `/usr/lib/systemd/system-sleep/` | **`pre`: stops `fprintd` before sleep** so no stale claim survives — this is the fix for problem 2. **`post`: re-enumerates the sensor** (`authorized` 0→1) so its exposure is reset and the first post-resume capture is well-exposed. |
| `egis0576-fp-wait` | `/usr/libexec/` | **Gate on `fprintd`'s start.** The hook's `post` phase takes about a second to re-enumerate the sensor, and `fprintd` is D-Bus activated, so the desktop can activate it inside exactly that window and claim a device that is about to leave the bus. This waits for the hook's marker to disappear, then lets `fprintd` start. Fail-open: after 15 s it starts it anyway, so a marker left by a hook that died cannot disable fingerprint authentication. |
| `egis0576-fprintd-wait.conf` | `/usr/lib/systemd/system/fprintd.service.d/` (packages) or `/etc/systemd/system/fprintd.service.d/` (`install.sh`) | The `ExecStartPre=` drop-in that runs the gate above. |
| `60-egis0576-fp-nosuspend.rules` | `/etc/udev/rules.d/` | Keeps the sensor powered instead of letting it autosuspend while idle. There is no session state to protect — the protocol is stateless — but an interactively used reader should not have to come back from runtime PM on every press. |

Restarting `fprintd` around suspend is exactly the community-standard workaround
for the upstream claim bug. Fingerprint stays enabled everywhere — login, `sudo`,
**and** unlock; nothing is disabled.

## Manual install

```bash
sudo install -m 0755 50-egis0576-fp-resume.sh    /usr/lib/systemd/system-sleep/
sudo install -m 0644 60-egis0576-fp-nosuspend.rules /etc/udev/rules.d/
sudo install -Dm 0755 egis0576-fp-wait /usr/libexec/egis0576-fp-wait
sudo install -Dm 0644 egis0576-fprintd-wait.conf \
    /etc/systemd/system/fprintd.service.d/10-egis0576-resume-wait.conf
sudo systemctl daemon-reload
sudo udevadm control --reload-rules
sudo udevadm trigger --attr-match=idVendor=1c7a
```

Revert by deleting the four files, then `sudo systemctl daemon-reload` and
reloading udev rules.

The gate is the one piece here that was not written for this repository first:
the race it closes was diagnosed on an IdeaPad Flex 5 14ITL05 by
[sam-dant](https://github.com/sam-dant/egis0576-resume-workaround), who measured
`fprintd` starting one second before the hook finished and the claim still being
refused 46 s later, and who built and tested the marker-plus-gate design this
implements ([#3](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/3)).

## Test it

Lock the screen (`Super+L`), suspend, wait ~30 s, wake, and unlock with your
fingerprint. It should be offered immediately. If it *hangs* (rather than falling
back to password), that is a different, GPU/compositor s2idle bug on your platform,
not this reader.

## Can I drop the hook and fix it in the driver instead?

No — and [`../docs/suspend-resume.md`](../docs/suspend-resume.md) explains why in
detail. Short version: at the moment recovery is needed, the failing operation is
an fprintd/polkit device **claim** that happens *before* libfprint is opened, so
the driver has no code path there. Three in-driver approaches (cancel-on-suspend;
keep-alive + worker re-init; even replicating the Windows `ForceReset`
control request `0x21/9 wValue=0x00ff`, which re-enumerates the device) were built
and tested — none can help,
because the driver is never invoked. Clearing the stale claim requires restarting
`fprintd` at resume time, which is inherently a systemd-sleep hook.

If gnome-shell/fprintd fix the claim-across-suspend bug upstream, the hook becomes
a harmless no-op and can be removed.
