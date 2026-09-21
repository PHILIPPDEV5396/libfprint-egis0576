# Upstream readiness — the verified gap list (v0.5.0 roadmap)

Produced 2026-09-21 by a five-lens audit of this driver against the pristine libfprint 1.94.100 tree
(I/O architecture, device lifecycle, matcher integration, tests and build, style and provenance), every gap
attacked by two independent reviewers; 0 gaps survived, listed here by severity as the reviewers left it
(`blocker` = upstream would not merge with it; `major` = reviewers would require a change; `minor` = will be
asked to fix; `note` = information). Duplicates across lenses are kept where the wording adds something.
This file is the definition of v0.5.0: the release is cut when every blocker and major below is closed.

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
