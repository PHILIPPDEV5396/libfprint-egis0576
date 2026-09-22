# Fingerprint after suspend/resume — the full story

**TL;DR.** After a system suspend, GNOME's lock screen sometimes won't offer the
fingerprint and drops straight to the password. This is **not** a bug in this
driver — it is a well-known, still-unfixed **gnome-shell/fprintd** bug where the
fingerprint device's *claim* is not released across suspend (`"Device was already
claimed"`; the same message could also come from a driver hang fixed in v0.4.3 —
see below). The driver is never even invoked at the failing moment. The reliable,
community-standard fix is to **restart `fprintd` around suspend**, which is exactly
what the [`integration/`](../integration/) systemd-sleep hook does. Keep it
installed. This document records the investigation so nobody (the author included)
re-treads the dead ends.

---

## Symptom

On an AMD Rembrandt laptop (Lenovo Yoga 7 14ARB7, Fedora, GNOME 50, `mem_sleep=
s2idle`):

- Historically (before reads from the sensor were bounded in wall-clock time, see
  below) a suspend that
  happened *while the sensor was armed* could **hang the unlock screen hard**
  (gnome-shell unresponsive, forced reboot).
- After that hang was fixed, the remaining symptom is: **~40 % of resumes, the
  lock screen offers only the password** — no fingerprint. Locking again
  (`Super+L`) or switching VT makes the reader work again.

## The real root cause

With the hook disabled and `FP_DEBUG=all`, the fprintd journal right after resume
shows, verbatim:

```
Preparing devices for resume
Requesting authorization from :1.482 to call method 'Claim' for device '… EH576'
Authorization denied to :1.482 to call method 'Claim' for device '… EH576':
        Device was already claimed
```

No `open`, no `identify`, **nothing from the driver** — libfprint is never
reached. `fprintd` still holds a **stale claim** taken by gnome-shell's
pre-suspend lock-screen verify, which is not released across the suspend. The new
post-resume claim is therefore refused, and gnome-shell falls back to the password.

Note (since v0.4.3): the line `Device was already claimed` on its own does not
identify this bug. Builds v0.1.0–v0.4.2 could produce the same refusal from a
driver-side hang — a verify that never completes because of a lost wakeup in
gusb's synchronous transfer helpers, after which every Claim is refused until
fprintd is restarted (see [`worker-thread.md`](worker-thread.md)). The two are
told apart in the journal: the upstream stale-claim bug shows `Preparing devices
for resume` followed directly by the Claim denial with no driver activity at all;
the driver bug shows an earlier verify/identify that never finished, and it is
not tied to suspend. Everything below concerns the upstream case; it was observed
across several driver versions (the in-driver experiments date from the TLS era,
the ForceReset measurements from 2026-09-09 and 2026-09-13) and does not depend
on the driver version.

This is an upstream **gnome-shell + fprintd** interaction bug, entirely above
libfprint. It is widely reported and, as of mid-2026, **open / unfixed**:

- GNOME/gnome-shell #7791 — *Fingerprint unlock sometimes not available after
  suspend* (~40 % of resumes; workaround = VT switch / re-login)
  <https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/7791>
- Ubuntu #2067135 — *Fingerprint authentication doesn't work after suspend*
  (ThinkPad T15/X1 Yoga/P1, Ubuntu 23.10/24.04)
  <https://bugs.launchpad.net/bugs/2067135>
- GNOME/gnome-control-center #3166, Ubuntu #2077837, Red Hat Bugzilla 503898,
  Framework and Arch forums — same class of bug on unrelated readers.

## The fix (what actually works): the `integration/` hook

The community-standard workaround is to **restart `fprintd` around suspend** so no
stale claim survives. The hook here does exactly that in its `pre` phase:

```sh
pre)  systemctl stop fprintd.service   # drop the stale claim before we sleep
```

On resume, `fprintd` is D-Bus-activated fresh, with no claim held, so the lock
screen's `Claim` succeeds and the fingerprint works on the first press. The `post`
phase additionally re-enumerates the sensor (`authorized` 0→1), which resets its
exposure state so the first post-resume capture is well-exposed. (Until v0.5.0 a
udev rule pinned the sensor's `power/control` to `on`; it was a conservative
default, never a fix for a measured failure, and is gone: USB autosuspend was
validated with the asynchronous driver — the sensor goes to runtime suspend 2 s
after `close()`, comes back on the next `open()` ~0.1 s slower, and a device
held open never autosuspends.)

None of this can live in libfprint: clearing an fprintd claim and restarting the
service are operations *above* the driver, and they must run *at suspend/resume
time*, which is inherently a systemd-sleep hook.

## What the driver *does* contribute

Two things, both real but both narrower than earlier versions of this doc claimed:

1. **A bounded timeout on every bulk read** (`egis0576_proto.c`). If the driver
   ever *does* talk to an unresponsive sensor (e.g. a suspend fires while a capture is
   genuinely in flight), every USB read is now bounded, so `getframe`/the
   handshake fail fast to a clean error instead of spinning forever. **This is
   what removed the original hard freeze.** Keep it.
2. **`suspend`/`resume` vfuncs** that keep a running action alive across the
   sleep, the way libfprint's `fpi_device_suspend_complete()` contract asks for
   (since v0.5.0; v0.4.x cancelled the action instead, egismoc-style). On
   suspend the capture machine parks at its next transfer boundary — nothing in
   flight, nothing scheduled — and only then is the suspend completed, so
   fprintd releases its sleep inhibitor with a quiet bus; a re-init that is in
   flight stops at its own boundaries too, so the park never waits out a
   readiness poll. On resume the machine goes back to its loop head, re-runs the
   sensor bring-up (the sensor does not come back from s2idle initialised) and
   continues the same verify. Measured on the reference unit (2026-09-22, sleep
   hook disabled, `fprintd-verify` running): parked 1.5 s before the kernel's
   `PM: suspend entry`, resumed, re-initialised, the same verify matched on the
   first press after waking. These vfuncs only run when an action is *active at
   the instant of suspend* — which, in the real GNOME lock-screen flow, is
   usually **not** the case (fprintd has already gone idle and closed the
   device). So they make the driver correct, not the lock screen work; that is
   the hook's job.

## Why there is no pure in-driver fix (the dead ends)

Three in-driver approaches were built and tested on hardware; all failed for the
same underlying reason — **at the moment recovery is needed, the driver is not in
the call path** (fprintd hasn't opened/claimed the device yet, or refuses to).

1. **`suspend` cancels the action (egismoc-style).** No freeze, but GNOME does not
   re-arm after the cancel → password only. (And in the common idle-at-suspend
   case the vfunc never even fires.)
2. **`suspend` keeps the action alive + worker re-handshakes on resume.** The
   worker re-init either ran at the wrong time (the `suspend` vfunc fires ~2 s
   *before* the actual kernel suspend) or produced non-matching frames; recovery
   still came from a later fresh `open`, sometimes after a multi-second stall.
3. **Replicate the vendor recovery.** A class control request **`0x21/9`
   `wValue=0x00ff`** (*ForceResetDevice*; the same request with `wValue=0` is the
   TLS-session door this driver never opens) returns the sensor to its plaintext command mode. In the vendor's
   Windows driver it is orchestrated by `check_and_recovery` in
   `egis_fp_common_5XX.c` (poll register 0 for token `0xAA` → ForceReset →
   re-write regs `0x0A/0x0C/0x50` + vdm upload → `tz_calibrate_dvr`). The reset
   itself works reliably, and it survives in the shipped driver as
   `egis_dev_open (reset_if_stuck=TRUE)` (`egis0576_proto.c`) for the migration case
   described below — but as a *suspend/resume* fix it is **moot here**: in the real flow the post-resume `Claim` is refused with
   *"Device was already claimed"* before any `open` runs, so a driver-level reset
   is never reached.

   Two corrections to what this document previously claimed, both measured on
   hardware (2026-09-09):

   - **The reset DOES re-enumerate the device.** Earlier text here said it worked
     "without USB re-enumeration". That is wrong: the sensor drops off the bus and
     comes back with a new device number (`dmesg`: `USB disconnect` → `new
     high-speed USB device`, ~0.4 s apart). Anything calling it has to cope with
     the device object it holds becoming invalid. Refined 2026-09-13 with the
     accuracy kit ([`sensor-tuning.md` §6](sensor-tuning.md)): the drop happens
     on the *next transfer to the device*, not on the request itself — after
     `ForceResetDevice` the sensor sat on the bus for 5 s with its device number
     unchanged; the first bulk access then failed with `ENODEV` and the kernel
     re-enumerated it ~0.4 s later. The "~530 ms" measured earlier was that
     access following the request immediately. `egis_dev_open` issues the
     request and fails the open; the re-enumeration then happens on whatever
     touches the sensor next.
   - **The name `CRealTekDeviceCtrlForET576WithTLS` does not come from this
     device's driver.** It appears in none of the three DLLs Lenovo ships for the
     EH576 (`EgisTouchFP0576.dll`, `EgisTouchFPEngine0576.dll`,
     `EgisTouchFPSensor0576.dll`) and should not be cited as the source of the
     TLS protocol. The `check_and_recovery` / `0xAA` poll described above *is*
     from this device's driver and is confirmed by Ghidra decompilation of
     `EgisTouchFP0576.dll` (`FUN_180009594` → `FUN_18000bd14(0, 0xAA, 0xFE)`,
     polling up to 3000 times); note that its recovery branch runs only *after*
     the poll succeeds, so the vendor driver has no recovery for a sensor that is
     not answering plaintext at all.

The decisive measurement was a clean test with the *real* GNOME lock screen (not
`fprintd-verify`, whose single-shot lifecycle confounded earlier runs): the
driver's `suspend` vfunc did not fire at all, and the failure was the polkit-level
`Claim` denial above.

The `wValue=0x00ff` ForceReset sequence is documented above in case a future
maintainer needs an in-driver device reset for a *different* reason — but it is not
a fix for the suspend/resume claim problem.

## The dual-boot one-way door (and why v0.4.0 removed it)

What follows is a separate story that happens to use the same ForceReset
mechanism. **It changes none of the suspend/resume conclusions above.**

It is also how the driver recovers a sensor that a *pre-plaintext* build of itself
left behind, which is worth recording because it drove a redesign.

The sensor keeps whichever protocol mode it was last put into, and that state
survives USB autosuspend, a USB port reset and a reboot — all measured. Only
ForceResetDevice, or cutting board power, clears it. While this driver used the
TLS-PSK session mode it therefore broke the vendor's Windows driver *for this
device* (v3.10.3.3, which has no TLS at all — an older 2020 build does), which only
ever speaks plaintext and fails the device with `STATUS_UNSUCCESSFUL` (Device
Manager Code 10) when its `0xAA` readiness poll gets TLS records instead. Every
placement of a compensating reset inside the driver's lifecycle was tested and
rejected: from `close()` it costs roughly one failed authentication per reset
(fprintd swaps the re-enumerated device too slowly), and from GObject `finalize`
it fires after fprintd has released its D-Bus name, landing in the middle of the
next session. There is no race-free point in the fprintd lifecycle for it.

**The fix was to stop entering the mode.** The driver now speaks the sensor's
plaintext protocol throughout, so it never opens the one-way door and needs no
reset at all: measured across three consecutive fprintd sessions, zero
re-enumerations, and the sensor still answering the plaintext readiness poll
afterwards — exactly what Windows needs to find. `egis_dev_open` still issues one
ForceResetDevice if it meets a sensor stuck in session mode by an older build; that
open fails and the next one, on the re-enumerated device, succeeds. It is a
one-time migration cost, not part of normal operation.

## Bottom line

- **Keep the hook and its fprintd gate** ([`integration/`](../integration/)).
  They are the correct, standard fix for a real upstream bug, not a workaround
  for a driver shortcoming. (The udev rule is gone since v0.5.0: USB autosuspend
  stays at libfprint's default, validated.)
- The per-read timeout in the driver (every bulk read in `egis0576_proto.c` is
  bounded) prevents the hard freeze.
- If gnome-shell/fprintd ever fix the claim-across-suspend bug upstream, the hook
  simply becomes a harmless no-op and can be removed.
