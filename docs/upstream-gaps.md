# Upstream readiness — the verified gap list (v0.5.0 roadmap)

Produced 2026-09-21 by a five-lens audit of this driver against the pristine libfprint 1.94.100 tree
(I/O architecture, device lifecycle, matcher integration, tests and build, style and provenance), every gap
attacked by two independent reviewers; 53 gaps survived, listed here by severity as the reviewers left it
(`blocker` = upstream would not merge with it; `major` = reviewers would require a change; `minor` = will be
asked to fix; `note` = information). Duplicates across lenses are kept where the wording adds something.
This file is the definition of v0.5.0: the release is cut when every blocker and major below is closed.

*2026-09-26: closing every gap below is necessary for the upstream submission, not sufficient. The submission is on
hold on matcher accuracy, which no gap here covers -- see the "matcher accuracy" row of the status table.*

## Status

| step | state | where |
|---|---|---|
| 1 tree, provenance, instance state, contract | done | `0e5f7bb`, `000afc5`, `8cbfc6a`, `a60fd36` (2026-09-21) |
| 2 asynchronous I/O | done, hardware-validated 2026-09-22 (enrol 12/12 with two steering refusals, genuine press 0.88/0.95 confirmed by the second frame, other finger 0.55 rejected, cancel answered in 3 ms, open 0.37 s) | `c81516a` (2026-09-21): FpiSsm over FpiUsbTransfer, matcher in a GTask |
| 3 suspend/resume/autosuspend in the driver | done, hardware-validated 2026-09-22 (s2idle with the sleep hook disabled and a verify running: parked 1.5 s before `PM: suspend entry`, resumed, re-initialised, the same verify matched 0.86/0.86; the following GNOME unlock matched 0.93/0.91; autosuspend from runtime-suspended state ok) | `6b9835c`, `ae537ab`, `ee16468` |
| 4 umockdev test | custom.py written for what a non-finger recording can honestly assert (enrol, non-match verify/identify, empty gallery, alien print, finger status, close); tooling ready; the recording itself needs ~14 presses of the measured object at the machine | |
| 5 style, headers, statement, MR text | in progress: uncrustify passes (`113ba93`), LGPL blocks (`32e065c`), Pengu601 statement (`6fbb7b8`), adapter/contract headers (`ef93f46`), nbis_count (`30ad123`), submission-tree generator (`d1503fc`), MR draft `docs/upstream-mr.md`; open: the umockdev recording (step 4). Closed since: the engine contract takes a caller buffer (`b94a371`), the thermal choice is argued in the MR text, the clean-room matcher is the default (`03f65b4`) | |
| matcher audit (2026-09-22) | done: the period check closes only constant-period families (jitter/loop/noise pass at 0.80-0.90, oracle attacker unaffected) and the claims are corrected; `EM_FQ_MIN_NBLK` 20 -> 12 (attack numbers bit-identical from 8 to 20, one genuine press of 60 recovered under the driver's rule); the alignment search runs once instead of twice (11.18 -> 6.20 ms a pair, identical on 32400 pairs); the kit reports the driver's accept rule | `5aecaeb`, `f23ac5c`, `49448b9` |
| **matcher accuracy (2026-09-26)** | **not met — the submission is on hold.** No gap below covers it: the audit cited the Gabor front-end's 0 %/0 % on the reference unit as a precondition met (the `egis_engine.h` contract gap) and nothing in it measured a second session. That figure is one session (2026-09-13), in-sample; the same unit's 2026-09-18 session gives 13/60 false rejects and 18/480 false accepts under the driver's own rule (vendor on the same frames: 2/60, 0/480), cross-session 9/240 and 15/240, and no constant measured reaches 0/0. Bar for resuming: an own matcher at least as good as vendor -- 0 false accepts under the driver's rule on every dataset (both reference-unit sessions, both cross-session directions, sam-dant, tsteppy), no more false rejects than vendor, tuned on one dataset and certified on the others | `docs/matcher-comparison.md` "2026-09-26: what prevents a universal 0/0"; `docs/upstream-mr.md` |

## The work, in the order it should be done

1. **Submission tree and provenance** — one clean-room flavour under `libfprint/drivers/egis0576/`, two meson hunks,
   no build option, no static library with relaxed flags, no `-include`; vendor remnants out of the clean-room path
   (`EGIS_STEP_TABLE`, `auto_expose`, decompilation names and offsets, the dead Pengu601-derived block in
   `egis0576.h`); the engine contract without vendor-shaped semantics; per-instance state instead of process
   globals (engine handle owned by the device instance — the vendor flavour in this repo can return a singleton
   behind the same handle, so it stays buildable here without going upstream).
2. **Asynchronous I/O** — the worker thread and the private `GMainContext` go; the transport becomes `FpiSsm`
   sub-machines over `FpiUsbTransfer` on the main loop (command, readiness poll, init replay under
   `fpi_device_critical_enter/leave`, frame, calibration), `open()` no longer blocks, cancellation is a stop flag
   honoured between transfers with a NULL cancellable (the sensor wedges if an in-flight URB is aborted — that
   rule stays), matching runs in a `GTask` thread as `secugen` does, finger presence is reported.
3. **Suspend/resume/autosuspend in the driver** — `integration/` stays out of libfprint; the suspend vfunc must
   honour the completion contract; the driver is validated with `power/control=auto`; `1c7a:0576` leaves upstream's
   *known unsupported* list, `autosuspend.hwdb` is regenerated, the wiki page is updated.
4. **umockdev test** — `tests/egis0576/custom.py` + recording made with a non-finger object; no `fp_warn` on
   ordinary paths (the harness runs with fatal warnings); no timeouts as control flow; a deterministic replay path
   where floating-point matcher decisions would otherwise steer the USB sequence.
5. **Style, headers, documentation, MR text** — uncrustify over every submitted file; license headers and copyright
   lines; the tsteppy file without its evaluator `main()`; a written statement on the Pengu601 origin; the
   non-NBIS justification with `nbis_count` in `tools/accuracy/`; `docs/worker-thread.md` retired.

Effort as the reviewers estimated it: architecture 5–7 working days, lifecycle and hwdb 1–2, tree/state/provenance
2–3, tests 1–2, style/docs 1 — and then the hardware re-validation the old bugs demand (cancel-proof harness,
s2idle, the long-lock run), because a rewritten capture path inherits none of the proofs the current one has.

## The gaps

### [blocker] Driver-owned USB worker thread has no precedent in the tree; USB must be driven as FpiUsbTransfer/FpiSsm callbacks on the main loop

*Where:* driver/egis0576.c:69-72 (GThread *thread, atomic cancel), :440-773 (capture_thread), :805 (g_thread_new), :282-372 (idle_handle_msg/post_msg marshalling); driver/egis0576/egis0576_proto.c:99-192 (egis_bulk/egis_control:

*Fix:* Rewrite the I/O as FpiSsm state machines driving FpiUsbTransfer on the main loop: no GThread, no private GMainContext, no g_usleep, no g_atomic_*, no g_idle_add marshalling (Msg/idle_handle_msg/post_msg/finish_teardown at egis0576.c:243-372 go away; the vfunc completions are called directly from SSM completion callbacks).  Hard rules that the original fix left implicit: R1. Every fpi_usb_transfer_submit passes cancellable=NULL (egis0570.c:221/240/253 precedent). fpi-usb-transfer.c:416-450 forwards the cancellable to gusb, which aborts the URB; proto.c:164-177 measured that this wedges the sensor until board power. Keep the measurement as a code comment next to the first submit so a reviewer

### [blocker->major/blocker] integration/ (systemd-sleep hook, udev no-autosuspend rule, fprintd ExecStartPre gate) has no upstream home and must be dropped from the submission

*Where:* integration/50-egis0576-fp-resume.sh, integration/60-egis0576-fp-nosuspend.rules, integration/egis0576-fprintd-wait.conf, integration/egis0576-fp-wait, install.sh:69-82, integration/README.md, docs/suspend-resume.md — vs

*Fix:* 1) MR content: exclude integration/ entirely and the install.sh:69-88 block; the driver's only udev/power artefact is its FpIdEntry { 0x1c7a, 0x0576 } in id_table. 2) hwdb/unsupported-list sync (missing from the original fix): remove `{ .vid = 0x1c7a, .pid = 0x0576 }` from libfprint/fprint-list-udev-hwdb.c:151, ask a maintainer (or do it with a GitLab wiki account) to drop 1c7a:0576 from the wiki page Unsupported-Devices.md so CI test_unsupported_list stays green, and regenerate data/autosuspend.hwdb with `meson compile -C build sync-udev-hwdb` so 0576 moves from "# Known unsupported devices" to "# Supported by libfprint driver egis0576" (still ID_AUTOSUSPEND=1, ID_PERSIST=0). 3) Hardware ve

### [blocker] No umockdev driver test exists (tests/egis0576/ custom.py + custom.pcapng + device, drivers_tests entry)

*Where:* our repo: no tests/ directory at all; upstream tests/meson.build:30-63 (drivers_tests dict), tests/README.md, tests/elanmoc/custom.py, tests/create-driver-test.py.in

*Fix:* Sequence this as the LAST step of v0.5: the recording is only stable once every driver/matcher change that alters how many frames an action consumes has landed (consuming more frames than recorded runs the pcap out -> REAPURB EAGAIN -> 800 ms gusb timeout -> re-init -> readiness poll fails -> action error; consuming fewer is tolerated).  (0) Pre-flight, no hardware: (i) demote the three fp_warn in the capture loop (driver/egis0576.c:692, 710, 738) to fp_dbg -- they fire in normal operation and tests run under G_DEBUG=fatal-warnings while create-driver-test.py records without it, so a press with one sub-threshold frame after a candidate records fine and aborts every replay; (ii) `make -C tool

### [blocker] udev-hwdb test aborts: 1c7a:0576 is on upstream's "known unsupported" allowlist and must be removed, autosuspend.hwdb regenerated, wiki page synced

*Where:* upstream libfprint/fprint-list-udev-hwdb.c:151 `{ .vid = 0x1c7a, .pid = 0x0576 },` and data/autosuspend.hwdb:502 `usb:v1C7Ap0576*` (under "# Known unsupported devices"); tests/test-generated-hwdb.sh; tests/meson.build:45

*Fix:* In the MR: (1) delete `{ .vid = 0x1c7a, .pid = 0x0576 },` (libfprint/fprint-list-udev-hwdb.c:151). (2) Regenerate data/autosuspend.hwdb WITHOUT the sync-udev-hwdb target: `ninja -C _build libfprint/fprint-list-udev-hwdb && _build/libfprint/fprint-list-udev-hwdb > data/autosuspend.hwdb` (or `ninja -C _build libfprint/autosuspend.hwdb && cp _build/libfprint/autosuspend.hwdb data/`). Do not use `meson compile sync-udev-hwdb` until the wiki is updated: it runs scripts/sync-unsupported-devices.py first and re-adds 0576 from https://gitlab.freedesktop.org/libfprint/wiki/-/wikis/Unsupported-Devices.md (verified in scratch). Expected diff: 0576 leaves the `# Known unsupported devices` block and gain

### [blocker] meson patch is unshippable: three-flavour static libraries with relaxed flags, a meson option, a forced -include, and an if-block that hides libfprint_drivers from every other build

*Where:* patches/libfprint-1.94.100-egis0576.patch (hunks at libfprint/meson.build:259-320, meson.build:132, meson_options.txt:38-43); driver/egis0576/egis_engine_cleanroom.c:186-191 (`__has_include`); driver/egis0576/gabor/egis_

*Fix:* Upstream branch (the submission, not this repo's patch): put exactly 11 files under libfprint/drivers/egis0576/ -- egis0576.c, egis0576.h, egis0576_proto.c, egis0576_proto.h, egis_init.h, egis_engine.h, egis_engine_cleanroom.c, egis_match.h (tsteppy's interface header, Thaddeus Stepanovich copyright intact), egis_match_check.h, egis_cr_tuning_gabor.h, egis_match_gabor.c. Meson changes are two hunks only: libfprint/meson.build driver_sources `'egis0576' : files('drivers/egis0576/egis0576.c', 'drivers/egis0576/egis0576_proto.c', 'drivers/egis0576/egis_engine_cleanroom.c', 'drivers/egis0576/egis_match_gabor.c')` and root meson.build drivers_info `'egis0576': {}`. No meson option, no static_libr

### [blocker] meson patch defines libfprint_drivers only when egis0576 is enabled; every other configuration fails at setup

*Where:* patches/libfprint-1.94.100-egis0576.patch, hunk @@ -250,13 +256,76 @@ of libfprint/meson.build (the '+endif' placed AFTER 'libfprint_drivers = static_library(...)'); conflicts with upstream .gitlab-ci.yml '.build_one_dri

*Fix:* Immediate fix (verified, ~15 min): in libfprint/meson.build move the trailing 'endif' so the flavour block closes before 'libfprint_drivers = static_library(' -- i.e. '    endif\nendif\n\nlibfprint_drivers = static_library(...\n    link_with: [libfprint_private] + egis0576_engine_libs,\n    install: false)\n' with 'egis0576_engine_libs = []' staying unconditional above the if. Regenerate patches/libfprint-1.94.100-egis0576.patch with 'git diff' against the pristine tree instead of editing the hunk by hand, then verify 'meson setup --werror -Ddoc=false' with -Ddrivers=virtual_image, -Ddrivers=elan,egis0570, no -Ddrivers, -Ddrivers=all, and -Ddrivers=egis0576 with -Degis0576_matcher=cleanroom

### [blocker] Three-flavour matcher option and the vendor-matcher default cannot go upstream; the submission must be a single clean-room build

*Where:* patches/libfprint-1.94.100-egis0576.patch: meson_options.txt hunk (option 'egis0576_matcher', combo vendor/cleanroom/gabor, default 'vendor'); libfprint/meson.build hunk lines 'if egis0576_matcher == "vendor"' (builds eg

*Fix:* Upstream branch = one clean-room build, no options. Concretely (all verified to compile with the exact upstream drivers_cflags + -Werror and to link with no undefined symbols):  1. meson.build: add 'egis0576': {} to drivers_info (after 'egis_etu905'). libfprint/meson.build: add to driver_sources    'egis0576' : files('drivers/egis0576.c', 'drivers/egis0576/egis0576_proto.c', 'drivers/egis0576/egis_engine_cleanroom.c', 'drivers/egis0576/gabor/egis_match_gabor.c'),    nothing else: no meson_options.txt hunk, no static_library, no include_directories, no c_args (mathlib_dep is already in 'deps'). Headers shipped: egis0576.h, egis0576/{egis_engine.h, egis0576_proto.h, egis_init.h}, egis0576/gabo

### [blocker] Three-flavour build option and vendor matcher sources cannot go upstream; submission tree must be a single clean-room front-end

*Where:* patches/libfprint-1.94.100-egis0576.patch (meson_options.txt 'egis0576_matcher' combo; libfprint/meson.build lines +259..+327 building egis_funcs.c/egis_rt.c/egis_engine.c/egis_preprocess.c/egis_coherence_map.c with '-w

*Fix:* Submission tree, all under libfprint/drivers/egis0576/ (precedent: goodixmoc/goodix.c + goodix_proto.c, synaptics/synaptics.c + bmkt_message.c):   egis0576.c        driver; absorbs the adapter's enrol/gallery/raw-corroboration logic as per-instance state (costed in the per-instance gap) and compares NCC doubles directly to the accept constant -- drop EGIS_THRESHOLD, ncc_to_score and the 5000/0.78 scale, which exist only to fit the vendor engine's int contract.   egis0576_proto.c/.h   transport, MINUS egis_dev_autoexpose (proto.c:512, never called; proto.h:88) and EGIS_STEP_TABLE (proto.c:36-41) with its '.rdata @0x180077170' comment: PROVENANCE.md §2 lines 53-55 classifies that table as cate

### [blocker] Matcher state is process-global (enrol session, gallery, probe scratch, flat-field baseline, exposure calibration); reviewers will require per-instance state in the FpDevice struct

*Where:* driver/egis0576/egis_engine_cleanroom.c:290-303 (static enrol*, gallery[16], gallery_n, probe*); driver/egis0576.c:154-155 (static g_baseline[], g_baseline_valid); driver/egis0576/egis0576_proto.c:288-289 (static g_expos

*Fix:* Keep the matcher behind the contract but make it an object, do not dissolve it into the GObject. (a) egis_engine.h: add `typedef struct EgisEngine EgisEngine; EgisEngine *egis_engine_new(void); void egis_engine_free(EgisEngine *);` and give every contract function (`egis_enroll_begin/add/finish`, `egis_gallery_load`, `egis_verify`, `egis_verify_raw_ok`, `egis_identify`, `egis_gallery_capacity`) an `EgisEngine *` first argument; drop `egis_engine_init()`. In egis_engine_cleanroom.c move the four statics (egis_engine_cleanroom.c:286-297: enrol session incl. steer_refusals, gallery entries, gallery_n, probe scratch) into `struct EgisEngine`, allocated in egis_engine_new (calloc, ~479 KB + probe

### [major] open() blocks the main loop for up to ~10 s in a nested private GMainLoop

*Where:* driver/egis0576.c:814-860 (egis0576_open calling egis_dev_open + egis_dev_calibrate inline); driver/egis0576/egis0576_proto.c:256-273 (egis_wait_ready: 10 x (800 ms read timeout + 50 ms g_usleep)), :291-382 (33-record re

*Fix:* Make open() an FpiSsm (pattern: egismoc.c:1537-1560 / secugen.c:2157) driven by fpi_usb_transfer_submit with callbacks, no nested main loops and no g_usleep: states CLAIM -> FLUSH (bulk IN, 30 ms, loop while data) -> WAIT_READY (ReadRegister 0 via async bulk OUT+IN; on miss fpi_ssm_next_state_delayed(ssm, 50) with a counter of 10, on exhaustion optional ForceReset control transfer then fpi_ssm_mark_failed) -> REPLAY (one state that advances a record index; skip the IN for the upload command and its 8 payload chunks exactly as egis0576_proto.c:346-351 does) -> READ_DC_C / REAPPLY / READ_GAIN -> CALIBRATE (a sub-SSM via fpi_ssm_start_subsm, 6 iterations of writereg + the same GetFrame SSM the

### [major] Transport bypasses FpiUsbTransfer: no FP_DEBUG_TRANSFER tracing, no short_is_error, no umockdev-friendly transfer log for reviewers

*Where:* driver/egis0576/egis0576_proto.c:126-192 (egis_bulk, egis_control, usb_out, usb_in). Conflicts with libfprint/fpi-usb-transfer.c:43-70 (log_transfer) and :330-341 (short_is_error).

*Fix:* Fold into gap 1 as an acceptance criterion, not a separate item: once capture and open() are driven by FpiSsm on the main loop, replace egis_bulk/egis_control (proto.c:126-162) with fpi_usb_transfer_new + fpi_usb_transfer_fill_bulk/fill_control + fpi_usb_transfer_submit, one SSM state per transfer (the 26-record init replay and the 8-chunk 0x73 upload become a chained state with a record index; the SIGE reply read is a follow-up bulk IN state; getframe is preamble states + a bulk IN of EGIS_IMG). Pass the transfer's GError through to fpi_ssm_mark_failed instead of NULL (drops the "init record %d failed" without cause at proto.c:342-343). Keep the driver's "never cancel an in-flight URB" rule

### [major->minor/major] Suspend vfunc violates the fpi_device_suspend_complete() contract (NULL error while cancelling), arms USB remote wakeup on a cancelled action, and releases the sleep inhibitor with a bulk URB possibly still in flight

*Where:* driver/egis0576.c:1029-1054 (egis0576_suspend) and :1057-1070 (egis0576_resume) — vs. libfprint/fpi-device.c:1925-1980 (fpi_device_suspend_complete doc + implementation), :1905-1912 (fpi_device_suspend_completed → fpi_de

*Fix:* Do option A for v0.5, keep option B as a documented post-merge follow-up.  A (driver/egis0576.c:1029-1070, ~30 min code, ~1-2 h test): 1. egis0576_suspend: keep only `g_atomic_int_set (&self->needs_reinit, TRUE);` and `fpi_device_suspend_complete (dev, fpi_device_error_new (FP_DEVICE_ERROR_NOT_SUPPORTED));`. Delete the direct `egis0576_cancel (dev)` and `g_cancellable_cancel (fpi_device_get_cancellable (dev))`: libfprint cancels current_cancellable itself (fpi-device.c:1980), which dispatches egis0576_cancel in idle (fp-device.c:84-105), and only returns the suspend task after the action's task completes, i.e. after finish_teardown has joined the worker. Rewrite the comment: NULL is reserved

### [major] Autosuspend: no per-driver opt-out exists upstream; the driver must be validated with power/control=auto and the 60-egis0576 rule dropped

*Where:* integration/60-egis0576-fp-nosuspend.rules:12, install.sh:78 — vs. libfprint/fprint-list-udev-hwdb.c:272-275, data/autosuspend.hwdb:499-505 (1c7a:0576 already listed under 'Known unsupported devices' with ID_AUTOSUSPEND=

*Fix:* 1. Validation (no fprintd, no sudo beyond one sysfs write): with 60-egis0576-fp-nosuspend.rules removed (or `echo auto > /sys/bus/usb/devices/3-3/power/control`), run 20 cycles of the upstream build's `examples/verify` (or `img-capture`) with `sleep 5` between runs; before each run assert `power/runtime_status == suspended`; inside each run assert (fp_dbg timing) that egis_wait_ready answers on the first 850 ms iteration and the first capture matches. Add 3 s2idle cycles entered while the device is runtime-suspended (kernel takes the reset-resume path there; the sleep hook's post-phase re-enumeration should cover it but has never been exercised from that state). The 99-egis-eh576.rules group

### [major] open() runs the whole bring-up synchronously on the fprintd main loop (up to ~9.5 s on a silent sensor), so cancel/suspend cannot be delivered during it

*Where:* driver/egis0576.c:815-860 (egis0576_open: egis_dev_open + egis_dev_calibrate inline), driver/egis0576/egis0576_proto.c:104-140 (egis_bulk/egis_control block on a private GMainLoop), :256-268 (egis_wait_ready: 10 × (800 m

*Fix:* Corrected numbers first (driver/egis0576/egis0576_proto.c): open() on a healthy sensor ≈ 1 s (flush_in 30 ms + wait_ready ~1 ms + replay ~0.3 s + calibrate 6 x (writereg 300 ms budget, answers in ms + getframe ~0.1 s)). On a silent sensor open() never reaches the replay: it is the readiness poll that fails — 10 x (800 ms IN timeout + 50 ms) + 500 ms ForceReset ≈ 9 s if the sensor accepts the OUT, but 10 x (3000 ms OUT timeout + 50 ms) + 500 ms ≈ 31 s in the measured wedge state where bulk OUT NAKs (egis_cmd returns -1 on the OUT without reading). A sensor that passes the poll and goes silent mid-replay costs 24 x 800 ms + 2 x 800 ms cache reads + ~2.9 s first calibration getframe ≈ 24 s (IN

### [major->major/minor] temp_hot_seconds = -1 (FP_DEVICE_FEATURE_ALWAYS_ON) on a host-polled sensor with no hardware finger detect

*Where:* driver/egis0576.c:1098-1101 (`dev_class->temp_hot_seconds = -1;`), capture loop :471-747 (getframe every ~40 ms while armed) — vs. fpi-device.c:168-169 (ALWAYS_ON granted from temp_hot_seconds < 0), fp-device.c:181-196,

*Fix:* Default path (recommended for the first MR): delete `dev_class->temp_hot_seconds = -1;` and its comment (egis0576.c:1098-1101). Behaviour then matches egis0570/elan: 180 s of armed streaming, then FP_DEVICE_ERROR_TOO_HOT -> fprintd "verify-disconnected" -> password fallback, ~205 s cool-down (hot->warm at ratio 0.5 with temp_cold_seconds 540). Zero reviewer friction; enrolment (12 stages at a few seconds each) is far below 180 s. Note in the MR that continuous streaming is required because the current protocol has no finger-present event.  If keeping -1: (1) replace the comment with a thermal justification the class field actually documents (fpi-device.h:92-93) — measure sensor surface tempe

### [major] Capture does blocking USB on a GThread with a private GMainContext; upstream drivers are FpiSsm + FpiUsbTransfer on the main loop, with only compute in a GTask thread (secugen precedent)

*Where:* driver/egis0576.c:440-806 (capture_thread, start_capture, g_thread_new/g_thread_join), driver/egis0576/egis0576_proto.c:99-162 (egis_bulk/egis_control spinning a private GMainLoop), docs/worker-thread.md; upstream libfpr

*Fix:* Rewrite driver/egis0576.c + driver/egis0576/egis0576_proto.c as FpiSsm on the device main loop; no GThread, no private GMainContext, no g_idle_add marshalling, no atomic cancel flag, no borrowed GCancellable. Sub-SSMs: (a) OPEN: g_usb_device_claim_interface, DRAIN (up to 8 bulk IN 4096 @30 ms, timeout tolerated), WAIT_READY (readreg 0x00 OUT+IN 800 ms, up to 10 iterations via fpi_ssm_jump_to_state_delayed 50 ms, ForceReset control transfer only on the first open's failure), REPLAY (33 egis_init_records as bulk OUT 3000 ms; after record 15 (0x73) the next 8 records get no IN read; every other EGIS record is followed by a bulk IN 64 @800 ms whose timeout is tolerated), READ_DC_C/REAPPLY/READ_G

### [major->major/minor] fp_warn in the normal capture path will abort the umockdev test (G_DEBUG=fatal-warnings)

*Where:* driver/egis0576.c:692-693, 710-711, 738-739 (fp_warn), 794 and 890 (g_warn_if_reached); upstream tests/meson.build:3 and libfprint/fpi-log.h

*Fix:* In driver/egis0576.c change the three capture-path fp_warn calls (lines 692, 710, 738) to fp_dbg; they describe a no-match outcome, not a fault. Keep g_warn_if_reached at 794 and 890 (genuine invariant violations, and the umockdev recording must never hit them anyway). When adding the tests/egis0576/ umockdev recording, run it under G_DEBUG=fatal-warnings before submitting so any remaining g_warning path in the recorded session is caught locally. Optionally add `#define FP_COMPONENT "egis0576"` + `#include "fpi-log.h"` at the top of driver/egis0576/egis0576_proto.c for domain consistency, but upstream precedent (drivers/goodixmoc/goodix_proto.c logs under the default "libfprint" domain) show

### [major->major/minor] EGIS_STEP_TABLE is a byte-exact copy from the vendor DLL's .rdata and sits in a file destined for upstream; it is only reachable through dead code

*Where:* driver/egis0576/egis0576_proto.c:36-40 (EGIS_STEP_TABLE), :465-515 (auto_expose_mm, egis_dev_autoexpose), driver/egis0576/egis0576_proto.h:88; PROVENANCE.md 'egis0576_proto.c is listed under category 1 ... with one excep

*Fix:* In the upstream copy of driver/egis0576/egis0576_proto.c and .h: delete EGIS_STEP_TABLE, auto_expose_mm, egis_dev_autoexpose (definition :508-515, declaration proto.h:81-88) and the `gain` member plus its load at :380. Keep the register-naming block comment (it is referenced from driver/egis0576.c:383 — re-anchor that cross-reference to the new comment location) but rewrite it as protocol facts: drop "FUN_18000b2d0", "FUN_180008cd0" and the ".rdata @0x180077170" offset; phrasing like "the Windows driver stores these per-unit values in the device's registry Device Parameters and its auto-exposure only ever writes regs 0x0f and 0x12" matches the accepted upstream style (egismoc.c:9, aes3500.c:

### [major] Every submission file fails upstream's uncrustify check; the cleanroom adapter is in a different brace/indent style altogether

*Where:* driver/egis0576.c (51 changed lines), driver/egis0576.h (4), driver/egis0576/egis0576_proto.c (56), egis0576_proto.h (18), egis_init.h (70), egis_engine.h (31), egis_engine_cleanroom.c (404 of 593 lines), gabor/egis_matc

*Fix:* Do the pre-edits BEFORE running the formatter, because uncrustify either mangles or does not converge on these spots and CI re-runs the formatter over the committed tree (a hand repair after the run is undone in CI): (a) Rewrite the three one-line structs as multi-line GNU structs: BaselineAcc (driver/egis0576.c:157), FRAME_PREAMBLE (driver/egis0576/egis0576_proto.c:387), egis_init_records (driver/egis0576/egis_init.h:17). (b) driver/egis0576.c:637: move the '/* PH_AWAIT_OFF */' comment from between 'else' and '{' into the block (uncrustify otherwise flips 'else { if ..}' to 'else if' on pass 1 and re-braces it on pass 2, so a pass-1 commit fails test_indent). The multi-line 'else /* VERIFY

### [major->minor/major] License headers missing or incomplete on six of the upstream files; no copyright line on the driver itself

*Where:* driver/egis0576.c:1-27 (no 'Copyright (C)' line, no warranty/address paragraphs); driver/egis0576.h:1-11 (no copyright); driver/egis0576/egis0576_proto.c:1-3, egis0576_proto.h:1-15, egis_init.h:1-13, egis_engine.h:1-3, g

*Fix:* Give every category-1 submission file the upstream egis0570.c:1-19 form: title line, 'Copyright (C) 2026 Philipp Oster' (append '<philipp2861@gmail.com>' only if the author wants the address published, as upstream authors do), then the full three-paragraph LGPL-2.1-or-later text. Files: driver/egis0576.c (replace lines 23-27), driver/egis0576.h (replace 7-10), egis0576/egis0576_proto.c, egis0576_proto.h, egis_engine.h, gabor/egis_match_check.h (prepend), and complete the header on egis_engine_cleanroom.c:6-9 (keep line 4 'Matcher: Copyright (C) 2026 Thaddeus Stepanovich'), gabor/egis_match_gabor.c:6-9 and gabor/egis_cr_tuning_gabor.h (keep the SPDX line, add the paragraphs). egis0576/egis_in

### [major] Pengu601/EgisTec-EH576, credited as the origin of the capture path, has no license; the MR needs a written statement that no code was taken from it

*Where:* driver/egis0576.h:4-5 ('Protocol reverse-engineered by the Pengu601/EgisTec-EH576 project'); PROVENANCE.md 'Prior art by others': 'the first working capture path were established by the third-party Pengu601/EgisTec-EH576

*Fix:* 1. driver/egis0576.h: delete the dead block lines 36-105 (EGIS0576_FINGER_ON/OFF_THRESHOLD 13.0/8.0, EgisPkt, egis0576_init_pkts[30], EGIS0576_INIT_TOTAL, egis0576_repeat_pkts[5], EGIS0576_REPEAT_TOTAL, egis0576_poll_pkt, egis0576_image_pkt) and the other "Unused, historical" defines (EGIS0576_TIMEOUT, EGIS0576_IMG_SIZE, EGIS0576_CMD_RECV_LEN, EGIS0576_EP_OUT/IN, EGIS0576_CONF). Nothing outside the header references them (only EGIS0576_INTF is used, egis0576.c:828/840/901); either keep a ~15-line header with EGIS0576_INTF or move the define into egis0576.c and drop the header. This removes the only literal transcription of Pengu601 material (INIT_SEQUENCE/REPEAT_SEQUENCE from capture_fingerp

### [major] Driver header comment and engine header describe the vendor matcher as the matcher; comments cite repo-only paths and the vendor pipeline

*Where:* driver/egis0576.c:9-12 ('matched host-side with Egis' own feature extractor + matcher, reverse-engineered from the Windows driver and reimplemented as native C (driver/egis0576/egis_engine.*)'), :134-140 ('BYTE-EXACT tra

*Fix:* Do this on the branch that becomes the MR, keeping out-of-tree main buildable in all three flavours.  A. egis0576.c header (lines 1-27): rewrite in the egismoc.c/secugen.c shape: sensor facts (EH576 1c7a:0576, 70x57 8-bit press sensor, plaintext EGIS/SIGE bulk protocol), provenance ("protocol reverse-engineered from USB captures of the Windows driver; init sequence replayed verbatim, egis_init.h"), matcher ("frames are matched on the host by the LGPL correlation matcher in egis_engine_cleanroom.c + <front-end file>; templates are opaque fpi-data blobs"), the per-boot flat-field and two-frame confirmation in one sentence each, the worker-thread rationale in three sentences ending "see the tra

### [major->minor/major] Process-global mutable state in three files instead of per-instance state

*Where:* driver/egis0576.c:154-155 (static g_baseline[], g_baseline_valid, 'Single-sensor driver, so a process global is fine'); driver/egis0576/egis0576_proto.c:288-289 (static g_exposure_calibrated, g_dc_c_calibrated); driver/e

*Fix:* 1. Baseline: delete the dead instance fields baseline[]/have_baseline (egis0576.c:104-105) and reintroduce them as the real store: move g_baseline/g_baseline_valid into FpDeviceEgis0576 (plus baseline_dc_c, the reg-0x0f value the baseline was collected at); baseline_feed()/the flat-field function take self. Written and read only by the capture worker, so no locking. Do NOT clear them in close(): the FpDevice instance persists across fprintd Claim/Release (FpContext owns it from enumeration to removal), so per-instance == per-boot for one device and behaviour is unchanged; nothing to measure. Invalidate the baseline when the calibrated dc_c differs from baseline_dc_c (fixes the latent mismatc

### [major->major/minor] fp_warn used for ordinary runtime events; under the test harness every g_warning is fatal

*Where:* driver/egis0576.c:692 ('match not corroborated on the raw frame'), :710 ('candidate frame ... not confirmed by the next frame'), :738 ('finger lifted on an unconfirmed candidate'); egis0576.c:794 and :890 g_warn_if_reach

*Fix:* In driver/egis0576.c change :710 and :738 from fp_warn to fp_dbg (ordinary outcomes of the two-frame gate and of an early finger lift; synaptics.c:622 uses fp_info for the equivalent "not in database" case). Change :692 to fp_info (or fp_dbg) as well, since the raw-corroboration miss is an expected rejection path for smudges per egis_match_gabor.c:147; keep it as fp_warn only if the team wants the suspect-baseline signal in the journal, accepting that a recorded test hitting it would abort. Leave the two g_warn_if_reached at :794 and :890 unchanged, they guard real invariants in the same way fpi-device.c does. Before recording any custom.py umockdev test, run the driver once with G_DEBUG=fat

### [major] Compute placement: the ~2 ms + 8 ms x 12 frames per probe frame must run in a GTask thread (secugen pattern), and the gallery precompute currently stalls the main loop at action start

*Where:* driver/egis0576.c:927 (load_one_print → egis_gallery_load in egis0576_verify, main thread), :989 (egis_gallery_load in egis0576_identify, main thread), :578-663 (em_frame_compute/em_match per frame inside capture_thread,

*Fix:* Keep the secugen pattern, with these corrections and additions:  1. Per probe frame (main loop, in the capture SSM after the frame transfer completes): run frame_variance, baseline_feed and flat_field on the main loop (they are 4k-element loops, microseconds) so the process-global g_baseline is only ever touched from one thread; then snapshot BOTH the flat-fielded frame and the raw frame (2 x 3990 bytes) plus action kind, into a task-data struct; g_task_new(dev, fpi_device_get_cancellable(dev), frame_done, ssm); g_task_set_check_cancellable(task, TRUE); g_task_set_task_data; g_task_run_in_thread(task, match_thread). The SSM stays in a 'matching' state and issues the next getframe only from f

### [major] Blocking USB on a worker GThread with a private GMainContext has no upstream precedent; the driver must be an FpiSsm on the main loop using FpiUsbTransfer

*Where:* driver/egis0576.c:775-806 (start_capture: g_thread_new per action), :441-773 (capture_thread: blocking egis_dev_getframe loop with g_usleep), :862-903 (close joins the thread); driver/egis0576/egis0576_proto.c:126-160 (e

*Fix:* Port driver/egis0576.c and driver/egis0576/egis0576_proto.c to the upstream asynchronous model; keep the measured behaviours, drop the thread.  1. Transport: turn egis0576_proto.c into transfer builders on FpiUsbTransfer (fpi_usb_transfer_new + fpi_usb_transfer_fill_bulk / fill_control + fpi_usb_transfer_submit with cancellable NULL, callback fpi_ssm_usb_transfer_cb or a driver callback that sets transfer->ssm). Delete egis_bulk/egis_control, the private GMainContext, g_main_loop_run, g_usleep and docs/worker-thread.md (replace with a short note in the file header: 'URBs are never cancelled in flight, measured to wedge the sensor; cancellation is checked between transfers'). Keep the existin

### [major->minor/major] Print blob: hand-packed 16-byte header inside 'ay', storing 12 raw flat-fielded sensor frames — needs a structured GVariant, a stated format, and an explicit privacy argument in the MR

*Where:* driver/egis0576/egis_engine_cleanroom.c:107-125 (format comment), :277-279 (EGIS_CR_MAGIC/VERSION/HDR_SIZE), :335-350 (blob_nframes), :472-497 (egis_enroll_finish), driver/egis0576.c:208-221 (print_get_blob: G_VARIANT_TY

*Fix:* 1) Move the format into the driver, keep the engine libfprint-free. Reduce the engine blob to bare frames: egis_enroll_finish() returns nframes*EGIS_IMG_SIZE bytes (no header); egis_gallery_load(blobs, sizes, n) keeps its signature but blob_nframes() becomes: size > 0, size % EGIS_IMG_SIZE == 0, size/EGIS_IMG_SIZE <= max frames. Delete EGIS_CR_MAGIC/VERSION/HDR_SIZE, put_u16/get_u16. tools/accuracy/score.c needs no change (it round-trips enroll_finish -> gallery_load in-process). 2) Single source for the stage count: define EGIS_ENROLL_STAGES 12 in egis_engine.h; driver uses it for dev_class->nr_enroll_stages (egis0576.c:1096) and the adapter for EGIS_CR_MAX_FRAMES (it dimensions static arra

### [major] The MR's justification for a non-NBIS matcher exists as raw counts in /var/tmp, not in the repo, and lacks the run configuration

*Where:* /var/tmp/egis-nbis/work/{nbis_count.c,counts.txt,counts_perim.txt} (not in git); docs/matcher-comparison.md (no mindtct numbers; only 'measured 0 % genuine accept with NBIS minutiae' in PROVENANCE.md:123-126 and docs/sen

*Fix:* Documentation-only change, no driver code touched. Steps: (a) Move nbis_count.c into tools/accuracy/ (it is clean: libfprint's own nbis/mindtct plus the documented flat-field, prints counts only) and make it print one header line with the effective configuration and the libfprint source version: "# nbis_count libfprint=1.94.100 scale=<sc> ppmm=<ppmm> norm=<0|1> keep_perim=<0|1> lfsparms=g_lfsparms_V2". Add a Makefile target "nbis-count" that takes LIBFPRINT_SRC=<path to a libfprint checkout> and compiles against $(LIBFPRINT_SRC)/libfprint/nbis/{include,libfprint-include,mindtct} with -I$(LIBFPRINT_SRC)/libfprint and glib-2.0 cflags (verified: cc -O2 -w, 3 s). Document the flags in tools/accu

### [major] The egis_engine.h contract and adapter carry vendor-shaped semantics that make no sense once only one matcher exists

*Where:* driver/egis0576/egis_engine.h:12-24 (EGIS_THRESHOLD 5000 explained via 'The Windows default (660)'), :25 (egis_engine_init 'map + configure (mode 5)'), :56-61 (egis_preprocess 'byte-exact Windows per-frame preprocessing'

*Fix:* Precondition: the submission carries exactly one matcher, the Gabor front-end (tools/accuracy numbers: 0 %/0 % vs 35 % FRR on the reference unit, 6.9 % vs 51.7 % on the second). [2026-09-26: the numbers this precondition rests on do not hold. The 0 %/0 % is one in-sample session of the reference unit; its second session gives 18/480 false accepts under the driver's rule. The Gabor front-end does not meet the bar the submission now waits for -- see the "matcher accuracy" row of the status table.] tsteppy/egis_match.c stays in the out-of-tree repo for the accuracy kit only and is not submitted.  1. New header driver/egis0576/egis0576_match.h (own work, LGPL-2.1-or-later, with the attribution line for Thaddeus Stepanovich's frame layout and masked NCC that PROVENANCE.md already makes):    - typedef struct { double img[EGIS0576_FRAME_N]; guint8 mask[EGIS0576_FRAME_N]; double coverage; } Egis0576Frame;  (moved from tsteppy/egis_match.h so no orphan header remains)    - typedef struct { double ncc; int dx, dy; dou

### [major] Vendor-derived remnants and decompilation references remain in the clean-room path and must be removed before the tree is shown to reviewers

*Where:* driver/egis0576/egis0576_proto.c:36-42 (EGIS_STEP_TABLE 'byte-exact from the vendor driver's .rdata @0x180077170'), :461-516 (auto_expose_mm / egis_dev_autoexpose, the table's only user, 'deliberately NOT wired into the

*Fix:* Do this on the submission branch, in the same commit as (or immediately after) collapsing to the single clean-room flavour, so the flavour-comparison prose is written once.  1. driver/egis0576/egis0576_proto.c: delete EGIS_STEP_TABLE (:36-40), auto_expose_mm and its 20-line naming-note comment (:444-503), egis_dev_autoexpose (:505-515), the `gain` field (:33), REG_GAIN (:22) and the `d->gain = egis_readreg(REG_GAIN)` readback at :380; adjust the ":355 two cache reads" comment to one read. Rewrite :12-16 as "class request 9, wValue 0x00ff = ForceResetDevice (returns the sensor to command mode; it drops off the bus and re-enumerates on the next transfer)"; drop the wValue=0 line. Rewrite :18-2

### [minor] fpi_device_* and self-> fields accessed from the worker thread

*Where:* driver/egis0576.c:401 (fpi_device_get_usb_device in worker_reinit), :445 (fpi_device_get_current_action in capture_thread), :410-411 (self->sensor replaced on the worker while egis0576_close reads it on the main thread),

*Fix:* Fold into gap 1 as an explicit acceptance criterion rather than assuming it vanishes: the SSM rewrite must leave no thread function that dereferences FpDevice/self-> or calls fpi_device_*. If matcher compute stays off the main loop (secugen precedent, secugen.c:1756-1781), the GTask thread func receives a heap snapshot (copy of the frame, the FpiDeviceAction, enroll stage, matcher handle) and returns results via g_task_return_*; the main-thread callback is the only place that touches self-> and fpi_device_*. Drop the "self->sensor vs egis0576_close" sub-point from the evidence: close is refused BUSY while an action is current (fp-device.c:843), so it is unreachable, not a race -- cite it onl

### [minor] Results are posted with g_idle_add on the global default context instead of the task's context

*Where:* driver/egis0576.c:367-372 (post_msg -> g_idle_add). Conflicts with libfprint/fp-device.c:115-121 and libfprint/fpi-device.c:464-468.

*Fix:* Long term (v0.5 / upstream): moot once the worker thread is replaced by an FpiSsm + fpi_usb_transfer_submit() pipeline with the Gabor matcher in g_task_run_in_thread() (secugen.c:1773-1781 precedent) -- every completion then already lands on g_task_get_context(current_task).  Interim (any release that still ships the worker thread) -- do NOT use fpi_device_add_timeout() from the worker: it reads priv->current_task and does an unlocked g_slist_prepend(priv->sources) (fpi-device.c:463-472), racing the main thread's fpi_device_remove_timeout()/dispatch (fpi-device.c:413). Instead: 1. In start_capture() (main thread, runs inside the async vfunc, so the thread-default context IS the task's contex

### [minor] Finger PRESENT status is never reported (cannot be, from the worker)

*Where:* driver/egis0576.c:802-804 (only NEEDED at start), :279 (NONE at teardown); no FP_FINGER_STATUS_PRESENT anywhere in the driver. Compare libfprint/drivers/egismoc/egismoc.c:86 and goodixmoc/goodix.c:423,499.

*Fix:* Do it now, in the existing worker/message design, not "inside the rewrite" (there is no CAP_PROCESS state in this driver; the SSM rewrite is a separate gap and this change is independent of it and survives it unchanged).  1. driver/egis0576.c MsgKind (:~245-251): add M_FINGER_ON, M_FINGER_OFF. 2. idle_handle_msg (:~285):     case M_FINGER_ON:  fpi_device_report_finger_status_changes (dev, FP_FINGER_STATUS_PRESENT, FP_FINGER_STATUS_NONE); break;    case M_FINGER_OFF: fpi_device_report_finger_status_changes (dev, FP_FINGER_STATUS_NONE, FP_FINGER_STATUS_PRESENT); break;    (mirrors fpi-image-device.c:404-411: NEEDED, set at :802, persists for the whole action; PRESENT is layered on top. For ver

### [minor->minor/note] close() backstop can join a busy worker on the main loop for seconds

*Where:* driver/egis0576.c:862-903 (egis0576_close: g_warn_if_reached + egis0576_cancel + g_thread_join). No upstream counterpart; contradicts libfprint/fpi-device.h:137-139 expectation that close is short.

*Fix:* Not a separate work item. If the worker thread survives in any form: replace the cancel+join backstop in egis0576_close with g_return_if_fail (self->thread == NULL) (or g_assert), citing that libfprint refuses close while an action is current (fp-device.c:925-930) and asserts no current task at finalize (fp-device.c:221); keep the sensor/cancellable teardown. With the SSM rewrite (the real gap, tracked separately) the whole block disappears, as the original fix says.

### [minor] ForceResetDevice in open() (reset_if_stuck) exists only for an out-of-tree migration case and takes the device off the bus mid-open

*Where:* driver/egis0576/egis0576_proto.c:296-320 (egis_dev_open, `if (reset_if_stuck) egis_control (... MODE_REQUEST, FORCE_RESET ...)`), driver/egis0576.c:836 (egis_dev_open (self->sensor, TRUE, ...)), :390-392 (worker_reinit p

*Fix:* Drop the migration reset entirely, no replacement: - egis0576_proto.h: `gboolean egis_dev_open (EgisDev *d, GError **error);` and rewrite the doc comment (lines 57-64) to: "Fails with G_IO_ERROR_NOT_INITIALIZED if the sensor does not answer the plaintext readiness poll (a sensor whose protocol mode was switched by other software; only class request 0x21/9 wValue=0x00ff, ForceResetDevice, or a board power cut brings it back -- this driver issues neither)." - egis0576_proto.c: remove the `if (reset_if_stuck) egis_control (...)` block and the message suffix (lines 305-314); remove egis_control() and control_done() (unused afterwards, -Wunused under upstream's meson flags); keep the MODE_REQUEST

### [minor] hwdb / metainfo / wiki bookkeeping for adding a driver is not in the patch

*Where:* patches/libfprint-1.94.100-egis0576.patch (touches only libfprint/meson.build, meson.build, meson_options.txt) — vs. data/autosuspend.hwdb:499-505 (1c7a:0576 under '# Known unsupported devices'), libfprint/fprint-list-ud

*Fix:* Order matters and it is CI-blocking, not optional bookkeeping (tests/meson.build:3 G_DEBUG=fatal-warnings turns the duplicate g_warning into an abort, and test-generated-hwdb.sh then diffs against data/autosuspend.hwdb). Steps: (1) Edit the wiki first: remove 1c7a:0576 from https://gitlab.freedesktop.org/libfprint/wiki/-/wikis/Unsupported-Devices and add it to Supported-Devices (needs a gitlab.freedesktop.org account with wiki write access; if unavailable, hand-delete line 151 of libfprint/fprint-list-udev-hwdb.c and ask the maintainer in the MR description to sync the wiki -- the --check only runs at `meson dist`, not in MR CI). (2) Configure with all drivers so the regenerated hwdb keeps e

### [minor] Process-global per-boot state (flat-field baseline, exposure calibration, matcher gallery/enrol session) instead of per-instance state

*Where:* driver/egis0576.c:154-155 (`static guint8 g_baseline[EGIS_IMG]; static gboolean g_baseline_valid`), driver/egis0576/egis0576_proto.c:288-289 (`static gboolean g_exposure_calibrated; static unsigned char g_dc_c_calibrated

*Fix:* Three parts, all in the out-of-tree repo, no transport-protocol change:  1. Flat-field baseline (driver/egis0576.c): delete `g_baseline`/`g_baseline_valid` (154-155) and use the already-present but unused instance fields `self->baseline[EGIS_IMG]` / `self->have_baseline` (104-105). Change `baseline_feed (BaselineAcc *b, const guint8 *frame)` and `flat_field (const guint8 *raw, guint8 *out)` to take `FpDeviceEgis0576 *self`. Keep `BaselineAcc bl` as the per-capture stack accumulator in capture_thread (line 469) -- do NOT move it into the instance, that would accumulate across captures and change behaviour. Both callers (570, 577) are worker-thread only, so no new synchronisation. Update the c

### [minor] Code style: egis_engine_cleanroom.c is 4-space K&R, CI's uncrustify check would reject the MR

*Where:* driver/egis0576/egis_engine_cleanroom.c:281-593 (e.g. line 304 `for (int i = 0; i < EGIS_CR_MAX_GALLERY; i++) {`, 4-space indent); upstream scripts/uncrustify.cfg (indent_columns 2, nl_if_brace Force, nl_fdef_brace Force

*Fix:* 1. Decide first which matcher flavour goes upstream (only one will be accepted; see HACKING.md). That fixes the file set: driver/egis0576.c, egis0576.h, egis0576/egis0576_proto.c, egis0576_proto.h, egis_engine.h, egis_init.h, egis_engine_cleanroom.c, plus either gabor/{egis_match_gabor.c,egis_match_check.h,egis_cr_tuning_gabor.h} or tsteppy/{egis_match.c,egis_match.h}. If tsteppy goes, drop the "byte-for-byte / plain cp" policy (egis_engine_cleanroom.c:50, PROVENANCE.md:80) or land the GNU formatting in tsteppy upstream first. 2. Rewrite driver/egis0576/egis_init.h in the egis0570.h house style before touching uncrustify: 25 command records as `static const guint8` arrays (or a `[][N]` table

### [minor] Process-global state, env-var knob and dead header data will draw review comments and make the test harness's single open() the only tested path

*Where:* driver/egis0576.c:154-155 (g_baseline, g_baseline_valid), :853 (`g_getenv ("EGIS0576_NO_CALIBRATE")`); driver/egis0576/egis0576_proto.c:288-289 (g_exposure_calibrated, g_dc_c_calibrated); driver/egis0576/egis_engine_clea

*Fix:* 1. Baseline (egis0576.c): delete g_baseline/g_baseline_valid (:154-155); use the already-declared but unused instance fields `baseline[EGIS_IMG]` and `have_baseline` (:104-105) in baseline_feed/flat_field (pass `self`); delete the unused `corrected[EGIS_IMG]` field (:103). ~30 min.  2. Exposure calibration (egis0576_proto.[ch], egis0576.c): delete g_exposure_calibrated/g_dc_c_calibrated (:288-289). EgisDev is freed at every close, so the cache must live in FpDeviceEgis0576 (`gboolean exposure_calibrated; guint8 dc_c_calibrated;`) and cross the transport API: `gboolean egis_dev_calibrate (EgisDev *d, guint8 *dc_c_out, GError **error)` and `gboolean egis_dev_open (EgisDev *d, gboolean reset_if

### [minor] egis0576.h is 95 percent dead 'historical' content; only EGIS0576_INTF is used

*Where:* driver/egis0576.h:19-107 (EGIS0576_CONF, EGIS0576_EP_OUT/IN, EGIS0576_TIMEOUT 'Unused, historical', EGIS0576_IMG_SIZE, EGIS0576_CMD_RECV_LEN, FINGER_ON/OFF_THRESHOLD 'Unused, historical', EgisPkt, egis0576_init_pkts[30]

*Fix:* Delete driver/egis0576.h outright (it has no G_DECLARE_FINAL_TYPE -- that is already in egis0576.c:111 -- and nothing else live). In driver/egis0576.c: replace line 31 '#include "egis0576.h"' with nothing and add '#define EGIS0576_INTF 0' next to EGIS0576_ENROLL_STAGES (line 36); drop 'guint8 baseline[EGIS_IMG];' and 'gboolean have_baseline;' (lines 104-105). Keep include order as drivers_api.h first, then egis0576/egis_engine.h and egis0576/egis0576_proto.h (matches egismoc.c; both orders are accepted upstream, but with the FPI_USB_ENDPOINT_OUT use gone there is no dependency either way). Update the PROVENANCE.md table row (line 32) that lists egis0576.h; the meson patch lists only drivers/

### [minor] tsteppy/egis_match.c carries a stdio evaluator main() and is copied 'byte-for-byte' -- neither survives upstream

*Where:* driver/egis0576/tsteppy/egis_match.c:226-370 ('#ifdef EGIS_MATCH_MAIN' block with fopen/printf/main); egis_engine_cleanroom.c:47-51 'Copied byte-for-byte (license header intact); re-sync with plain cp'; PROVENANCE.md cat

*Fix:* Submission branch (gabor-only), in this order:  1. Flatten the driver directory to match upstream (egismoc/, goodixmoc/, synaptics/ are flat; meson registers a plain files() list, no static_library/include_directories/c_args): drivers/egis0576/{egis0576.c, egis0576.h, egis0576_proto.c, egis0576_proto.h, egis_init.h, egis_engine.h, egis_engine_cleanroom.c, egis_match.h, egis_match_gabor.c, egis_match_check.h, egis_cr_tuning_gabor.h}. No tsteppy/ or gabor/ subdirectory; do NOT merge EmFrame into a gabor file -- keep egis_match.h as his file with 'Copyright (C) 2026 Thaddeus Stepanovich' intact, let uncrustify reformat it.  2. Drop egis_match.c and with it the flavour scaffolding it justified:

### [minor] Env-var knob, libc malloc handed to g_free, and g_idle_add on the global default context are style points reviewers will raise

*Where:* driver/egis0576.c:853 (g_getenv ("EGIS0576_NO_CALIBRATE")); egis_engine_cleanroom.c:470 ('uint8_t *blob = malloc ((size_t) size); /* libc malloc: driver frees with g_free */') paired with egis0576.c:341 ('g_free, m->blob

*Fix:* 1. Delete the EGIS0576_NO_CALIBRATE branch at driver/egis0576.c:853 (calibration is already non-fatal with a fallback; do not gate it behind fpi_device_emulation_mode_enabled, which is the umockdev helper hook). ~10 min. 2. Keep the adapter GLib-free (it is built with only mathlib_dep, egis_engine_cleanroom.c includes no glib.h): in egis0576.c idle_handle_msg M_ENROLL_DONE, pass `free` (not g_free) as the GVariant destroy notify, or g_memdup2 the blob and free() the original; alternatively add glib_dep to the egis0576-cleanroom/gabor static_library and use g_malloc at cleanroom.c:470 and update the egis_engine.h:36 contract. Mirror the upstream precedent nbis/mindtct/binar.c:217 (patched to

### [minor] udev hwdb: 1c7a:0576 is on the 'unsupported devices' allowlist and the committed hwdb needs regeneration; the out-of-tree no-autosuspend rule contradicts upstream's ID_AUTOSUSPEND=1 policy

*Where:* /var/tmp/egis-nbis/libfprint-v1.94.100/libfprint/fprint-list-udev-hwdb.c:151 ('{ .vid = 0x1c7a, .pid = 0x0576 }' in allowlist_id_table), data/autosuspend.hwdb:502 ('usb:v1C7Ap0576*' under the unsupported section); integr

*Fix:* 1. Do NOT hand-edit allowlist_id_table: lines 32-204 of libfprint/fprint-list-udev-hwdb.c are a generated block that scripts/sync-unsupported-devices.py rewrites from https://gitlab.freedesktop.org/libfprint/wiki/-/wikis/Unsupported-Devices, and CI (test_unsupported_list, allow_failure) reruns it and diffs. In the MR description ask the maintainers to drop '1c7a:0576' from the wiki page (external contributors normally lack wiki write access); until that happens leave the line in place — the generator prints the driver block first and skips the allowlist duplicate with a g_warning, so the hwdb output is already correct. After the wiki edit: `meson setup _build -Ddrivers=all && meson compile -

### [minor->major/major] No umockdev test; the matcher's quality gates and the enrolment steering make the standard 'custom.py' recording harder than for other drivers

*Where:* tests/meson.build:31-63 drivers_tests (no 'egis0576'); tests/README.md ('For non-image drivers, create a custom.py script in advance and select the custom test'; 'To avoid submitting a real fingerprint ... the side of fi

*Fix:* Do this LAST, after the SSM/fpi-usb-transfer rewrite: the pcap pins the exact URB sequence (flush_in 8x30 ms reads, wait_ready, 25-record replay, 6-step calibration, 6 preamble cmds + GetFrame per frame) AND the matcher's accept/refuse decisions on the recorded frames (egis_cr_tuning_gabor.h thresholds, steering egis_engine_cleanroom.c:425-448, two-frame confirmation egis0576.c:671-713); any later change to either alters the number of GetFrames and desynchronises the replay -> re-record on the reference unit (~1 h each time, author only).  Before recording: (a) Change the three fp_warn in the verify loop (driver/egis0576.c:692, :710, :738) to fp_dbg: they fire on ordinary presses and abort t

### [minor] Code style: the adapter is 4-space/K&R and would fail upstream's uncrustify CI; the rest is unverified

*Where:* driver/egis0576/egis_engine_cleanroom.c (110 lines indented 4 spaces, 11 'if/for (...) {' same-line braces, e.g. :306-315, :335-350, :392-399) — vs. scripts/uncrustify.cfg:10 indent_columns 2, :93 nl_if_brace Force, :94

*Fix:* Do the reformat as its own format-only step, independent of whether egis_engine_cleanroom.c is later folded into egis0576.c (moving code does not reformat it). Steps: (1) install uncrustify from Fedora (CI image is Fedora rawhide per .gitlab-ci.yml:23-25; same-distro binary keeps output identical); (2) in the upstream checkout, add driver files under libfprint/drivers/egis0576/ and COMMIT them first, because scripts/uncrustify.sh only formats files listed by `git ls-tree -r HEAD` (untracked files are silently skipped); (3) run `scripts/uncrustify.sh` (no -c) then `git diff` and review: expected churn is the adapter's 110 four-space-indented lines and 11 same-line braces, two braced single-st

### [note] Deferred cancellation with NULL cancellable is conformant; the init replay should use fpi_device_critical_enter/leave instead of a hand-rolled 'one unit' rule

*Where:* driver/egis0576/egis0576_proto.c:164-177, :318-330 (replay as one cancellation unit), :31-45 (header rule); driver/egis0576.c:1000-1026 (egis0576_cancel). Compare libfprint/drivers/egis0570.c:283-289,414-421; libfprint/f

*Fix:* In the SSM rewrite (part of gap 1):  1. Pass NULL as cancellable to every fpi_usb_transfer_submit. Put the docs/sensor-tuning.md section 4 measurement (unlinking an in-flight URB wedged the sensor until board power-off; timeouts never do) as a comment above the single transfer-submit helper, and note that fpi_device_get_cancellable() is unaffected by critical sections (fpi-device.c:895-897), which is why the flag is the only cancellation path.  2. Make one init sub-SSM (READY_POLL x10 -> REPLAY_RECORD[0..N] -> READ_DC_C -> [WRITE_DC_C] -> READ_GAIN), used by both open() and the in-action recovery re-init. fpi_device_critical_enter at the entry of the first REPLAY_RECORD state only; fpi_devic

### [note] suspend vfunc cancels the action and completes with NULL error -- matches egismoc, but note the API contract

*Where:* driver/egis0576.c:1028-1054 (egis0576_suspend), :1056-1070 (egis0576_resume). Compare libfprint/drivers/egismoc/egismoc.c:1575-1582 and libfprint/fpi-device.c:1926-1936 (fpi_device_suspend_complete doc), :1905-1922.

*Fix:* Keep the egismoc-shaped suspend (cancel + suspend_complete(NULL)); it is accepted in tree (egismoc.c:1574-1582). Before submission: (1) rewrite the comments at driver/egis0576.c:1040-1050 and :1061-1067 to reference functions, not line numbers (all three cited ranges are already wrong for 1.94.100: egismoc_suspend is 1574-1582, the NULL early-return in fpi_device_suspend_complete is 1958-1964, fpi_device_resume's ACTION_NONE branch is 1795-1803), and add one sentence that fpi_device_suspend_completed enables USB wakeup because the action is still current at that instant, undone by fpi_device_resume_complete on resume. (2) Only if the worker thread is replaced by an SSM (separate gap) drop th

### [note] Timeouts used as control flow (drain reads, readiness poll) will not replay faithfully under umockdev

*Where:* driver/egis0576/egis0576_proto.c:203-210 (flush_in: up to 8 x 30 ms reads terminated by timeout), :256-273 (egis_wait_ready), :351 (`usb_in (d, r, sizeof r, 800)` reply consumed with errors ignored). Compare libfprint/dr

*Fix:* Keep the transport as is: flush_in, egis_wait_ready and the ignored reply reads already tolerate the zero-byte read that umockdev substitutes for a recorded timeout (verified: umockdev replays the recorded -ENOENT/0 completion immediately; libusb maps -ENOENT with reap_action NORMAL to COMPLETED/0 bytes; gusb returns 0). Do NOT replace flush_in with a single read -- the loop already stops at the first non-positive read, so a clean sensor records one 30 ms timeout, not 8; the bound of 8 is the drain limit for leftover replies. Add a one-line comment on flush_in and egis_wait_ready noting that the n<=0 exit is what makes them umockdev-safe.  Test work, after the rewrite settles (any change to

### [note->minor/note] Process-global transport/baseline state relies on thread-join ordering for safety

*Where:* driver/egis0576/egis0576_proto.c:277-289 (g_exposure_calibrated, g_dc_c_calibrated; comment explains ordering via g_thread_new/g_thread_join); driver/egis0576.c:142-155 (g_baseline, g_baseline_valid); driver/egis0576/egi

*Fix:* Not a pure relocation -- the proto layer needs a small API change because the EgisDev is recreated on every open (egis0576.c:835) and every worker_reinit (egis0576.c:411), which is why the cache was hoisted to a static in the first place. Concretely:  1. egis0576_proto.h/.c: drop g_exposure_calibrated/g_dc_c_calibrated (proto.c:288-289). Change `gboolean egis_dev_calibrate (EgisDev *d, GError **error)` to return the found value (e.g. `gboolean egis_dev_calibrate (EgisDev *d, guint8 *dc_c_out, GError **error)`, removing the once-per-process short-circuit at :535) and give egis_dev_open the value to re-apply (e.g. `egis_dev_open (EgisDev *d, gboolean reset_if_stuck, int calibrated_dc_c /* -1 =

### [note] integration/ (systemd-sleep hook, fprintd ExecStartPre gate, no-autosuspend udev rule) and packaging/ are out of libfprint's scope and stay out-of-tree

*Where:* integration/50-egis0576-fp-resume.sh, integration/egis0576-fp-wait, integration/egis0576-fprintd-wait.conf, integration/60-egis0576-fp-nosuspend.rules, integration/README.md; upstream data/meson.build, libfprint/fpi-devi

*Fix:* Out of tree (unchanged): integration/50-egis0576-fp-resume.sh, egis0576-fp-wait and egis0576-fprintd-wait.conf stay in this repo's packaging (all four packaging flavours already install them); add one paragraph to README.md and integration/README.md stating that upstream libfprint carries only driver/ and that the sleep hook + fprintd gate remain a distro-package concern until gnome-shell #7791 / fprintd release the claim across suspend (they are not EH576-specific and could be generalised later).  In the MR (new, missed by the original fix): (a) delete `{ .vid = 0x1c7a, .pid = 0x0576 },` from allowlist_id_table in libfprint/fprint-list-udev-hwdb.c (line 151 in 1.94.100); (b) `meson compile

### [note->minor/note] Floating-point matcher decisions drive the USB sequence, so the recorded test is only guaranteed to replay on the architecture it was recorded on

*Where:* driver/egis0576/gabor/egis_match_gabor.c (exp/cos/sin/atan2/hypot/lround throughout, e.g. lines 291, 371-372, 454-455, 471), egis_engine_cleanroom.c:409-458 (enrol gates decide whether a frame is stored and thus how many

*Fix:* Keep the gap as a recording-session checklist item, with the mechanism corrected: enrol's GetFrame count is gate-invariant (the worker polls until finger-off regardless of the add result, egis0576.c:585-640), so the only replay-hang path is verify/identify, where the two-frame confirmation stops polling early (egis0576.c:671-691). Concretely: (1) When recording custom.pcapng, run with G_MESSAGES_DEBUG=all and log coverage and NCC per frame (a one-line fp_dbg in the adapter's enrol/verify paths is enough); reject the take if any enrol settle frame has coverage within ~0.01 of EGIS_CR_MIN_ENROL_COVERAGE (coverage is kept/EM_N, so single-pixel mask flips move it by 1/EM_N) or any verify frame h

### [note->major/major] Worker GThread plus blocking USB in open() has no upstream precedent; expect the architecture to be the main review topic

*Where:* driver/egis0576.c:805 (g_thread_new per capture), :826-857 (open() runs readiness poll, ForceReset, 33-record replay and a 6-frame exposure search synchronously on the caller's thread); driver/egis0576/egis0576_proto.c:1

*Fix:* Port to the in-tree pattern; keep the gap's plan and add these specifics: (1) Transport: replace egis_bulk/egis_control (egis0576_proto.c:127-160) with fpi_usb_transfer_fill_bulk/_control + fpi_usb_transfer_submit, ALWAYS with a NULL cancellable (precedent egis0570.c:221/:240/:253), and check fpi_device_action_is_cancelled() only at the same sequence boundaries the code checks today (egis_etu905.c:183, upekts.c:1183). This preserves the measured "never abort a URB" policy. (2) open(): one FpiSsm: READY_POLL (readreg 0x00, on failure fpi_ssm_next_state_delayed 50 ms, max 10 iterations) -> FORCE_RESET (control transfer) -> INIT_REPLAY (single state with a record index; send records 15..23 back

### [note->minor/minor] Identify cost scales linearly with enrolled prints x 12 frames with no early exit, and the gallery cap fails loudly instead of scaling

*Where:* driver/egis0576/egis_engine_cleanroom.c:363-380 (score_entry, stop_at_accept), :573 (egis_identify passes 0), :204-210 (EGIS_CR_MAX_GALLERY 16), driver/egis0576.c:945-962 (identify refuses > capacity with FP_DEVICE_ERROR

*Fix:* 1. Drop the cap: in egis_engine_cleanroom.c make gallery and the nframes scratch heap-allocated to n (g_new0/calloc), remove EGIS_CR_MAX_GALLERY and egis_gallery_capacity(), and delete the refusal branch in egis0576.c:945-958 so identify takes whatever GPtrArray fp-device.c:1498 hands over (precedent: fpi-image-device.c:331, synaptics.c:841 caps only for a uint8 wire index). Any per-flavour limit that remains must be justified by a real resource limit and documented in the MR. 2. Align identify with fpi-image-device.c:331-341: per print, break at the first frame >= EGIS_CR_ACCEPT_NCC and return that print, in gallery order, instead of max-ranking; keep the full-maximum path only behind -DEGI
