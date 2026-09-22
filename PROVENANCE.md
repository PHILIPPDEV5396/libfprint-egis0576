# Provenance & Licensing

This document is an honest account of where the code in this repository comes from,
because part of it is a genuine copyright grey area and you deserve to know that
before you use or fork it.

## What this is

A [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint) driver for the
**EgisTec (LighTuning) EH576** fingerprint sensor (USB ID `1c7a:0576`), which ships
in several laptops but has **no vendor-provided Linux driver**. It was produced by
reverse-engineering Egis' proprietary Windows driver (`EgisTouchFP0576.dll`) for the
sole purpose of **interoperability** — making hardware that people already own usable
on Linux.

Two builds of that DLL were examined, and the distinction matters below: the build
shipped for this device (v3.10.3.3, 2021), which has no TLS record layer, and an
older one (v3.10.0.7, 2020) that bundles mbedTLS and carries the sensor's
pre-shared key.

## Where the code comes from

File paths in the tables below are relative to `driver/`; `install.sh` copies them
into `libfprint/drivers/` of the libfprint tree it builds.

### 1. Original work — © the author, LGPL-2.1-or-later

Written from scratch for libfprint:

| File | What it is |
|------|-----------|
| `egis0576.c`, `egis0576.h` | The libfprint `FpDevice` driver: the capture `FpiSsm` (enroll/verify/identify), the matcher `GTask`, per-boot flat-field, software finger detection. |
| `egis0576/egis0576_proto.c`, `.h` | The plaintext `EGIS`/`SIGE` transport as `FpiSsm` machines over `FpiUsbTransfer` (bring-up, frame, exposure calibration). |
| `egis0576/egis_rt.c`, `.h`, `egis_engine.c`, `.h` | The flat-memory runtime and the enroll/verify/gallery wrapper around the matcher. |
| `egis0576/egis_engine_cleanroom.c` | Adapter that implements the `egis_engine.h` contract on top of Thaddeus Stepanovich's clean-room correlation matcher (see category 3); compiled with `-Degis0576_matcher=cleanroom` or `=gabor`. |
| `egis0576/gabor/egis_match_gabor.c`, `egis_cr_tuning_gabor.h` | An alternative front-end for that adapter behind the same two-function interface (`em_frame_compute` / `em_match`): orientation-selective Gabor enhancement, per-pixel coherence mask, rotation-aware coarse-to-fine NCC search, and its operating point. Own work from public-domain building blocks (Hong, Wan & Jain 1998; structure-tensor orientation; masked NCC as in category 3); no vendor code or vendor-derived constant was consulted. Compiled only with `-Degis0576_matcher=gabor`. |

These files are licensed **LGPL-2.1-or-later**, matching libfprint. See `LICENSE`.

### 2. Reverse-engineered for interoperability — no copyright claimed by the author

Derived from Egis Technology's proprietary Windows driver and included because no
clean-room equivalent has yet matched it (see below):

| File | What it is |
|------|-----------|
| `egis0576/egis_funcs.c` | Egis' fingerprint feature extractor + matcher, translated from the decompiled DLL to native C. |
| `egis0576/egis_coherence_map.c` | The ridge-coherence estimator and its 6 lookup tables, extracted byte-exact from the DLL `.rdata`. |
| `egis0576/egis_preprocess.c` | The image preprocessing pipeline (min-subtract, invert, Otsu contrast stretch, vertical flip). |
| `egis0576/egis_init.h` | The sensor's init + calibration sequence: 25 `EGIS` command records plus the 3990-byte payload upload. Recovered by observing a decrypted vendor session rather than from the decompilation; the bytes are transport-independent and are now sent in the clear. |
| `egis0576/egis_blobs.h`, `egis_dat.h`, `egis_decls.h`, `egis_intrin.h` | Extracted data tables and support declarations for the translated code. |

`egis0576/egis0576_proto.c` is category 1 without exception since 0e5f7bb: the
16-entry exposure step table it used to carry (taken byte-exact from the vendor
DLL's `.rdata`, only reachable through an auto-exposure routine that was never
wired in) is gone, together with that routine and every comment that named a
decompiled function or a section offset. What remains of the vendor's material
in the transport is protocol fact: the register roles of init record 25, which
are the same values Windows keeps in the device's registry entry.

An earlier revision of this driver also carried the sensor's TLS-PSK pre-shared
key, recovered from the older (2020) vendor DLL. That transport has been removed —
the device is driven in the clear, the way its own shipped Windows driver drives
it — so the key is **not present in the current source**. It does remain reachable
in this repository's **git history** (in the deleted `egis0576/egis0576_tls.c`);
the history has deliberately not been rewritten, so anyone re-publishing this
repository re-publishes that key with it.

The driver also no longer links any crypto library. `libcrypto` remains a
dependency of the built `libfprint-2.so`, but only because upstream's own
`uru4000` driver requires it.

The author **does not claim copyright** over this second category. It is the
intellectual property of Egis Technology Inc. and is reproduced here only to the
extent necessary for the device to function.

### 3. Prior art by others

The plaintext `EGIS`/`SIGE` framing and the first working capture path were
established by the third-party [`Pengu601/EgisTec-EH576`](https://github.com/Pengu601/EgisTec-EH576)
project, which this work started from. That repository carries no license, so
here is exactly what of it is and is not in this one:

- **Taken at the start, gone since:** the first version of this driver
  (`3b6f850`, 2026-07-19) transcribed that project's packet lists into
  `driver/egis0576.h` (`egis0576_init_pkts`, `egis0576_repeat_pkts`,
  `egis0576_poll_pkt`, `egis0576_image_pkt`) as reference material. The
  driver never sent them: from that first commit it brought the sensor up
  with the vendor driver's own sequence, recovered by this project from a
  decrypted session (`egis_tls_init.h` then, `egis_init.h` since v0.4.0,
  `94ca704`, sends the same records in the clear), and the transcribed
  tables sat unused until `0e5f7bb` (2026-09-21) deleted them. No line of
  that project's code — no function, structure, comment or table — is in
  the tree now.
- **Still the same, because it is the device:** the per-frame command
  sequence the transport sends before every `GetFrame` (five register
  accesses, `egis0576_proto.c` `FRAME_PREAMBLE`) is byte for byte the sequence
  that project published. It is what the vendor driver issues per frame — the
  sensor's interface, observed by both projects independently of each other
  and not an expression either could own — and is kept as protocol fact, the
  same way the `egis_init.h` records are.
- **Credit:** that project is named here and in the driver's documentation as
  the origin of the plaintext protocol description this work began from.

`egis0576/tsteppy/egis_match.c`, `.h` — the clean-room correlation matcher,
© 2026 Thaddeus Stepanovich, LGPL-2.1-or-later, copied byte-for-byte (license
header intact) from
[`tsteppy/egistec-eh576-libfprint`](https://github.com/tsteppy/egistec-eh576-libfprint)
at commit `56ac424f` (`driver/egis_match.{c,h}` there). It is built only when the
driver is configured with `-Degis0576_matcher=cleanroom` (default: `vendor`),
together with the author's adapter `egis0576/egis_engine_cleanroom.c` (category
1). Exactly one matcher flavour is compiled: in a `cleanroom` build none of the
category-2 matcher sources (`egis_funcs.c`, `egis_coherence_map.c`,
`egis_preprocess.c` and the headers they pull in) are compiled at all — of the
reverse-engineered material only `egis_init.h` (the sensor bring-up sequence)
remains. Its measured accuracy is in
[`docs/matcher-comparison.md`](docs/matcher-comparison.md).

### Everything outside `driver/`

`patches/`, `packaging/`, `integration/`, `tools/` (the FAR/FRR accuracy kit in
`tools/accuracy/`), `install.sh` and `docs/` are the author's own work and carry
the repository's licence.

## Legal basis and honest risk disclosure

*This is not legal advice.*

- **Reverse-engineering for interoperability is legal.** In the EU it is explicitly
  permitted by [Directive 2009/24/EC, Article 6](https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32009L0024);
  in the US it is broadly protected under the interoperability line of fair-use
  cases. Producing *this driver* is squarely within that.
- **Redistributing the translated code is the grey area.** Article 6 permits
  decompilation for interop but does not clearly authorize *publishing* the
  decompiled/translated result. `egis_funcs.c` and the other category-2 files are
  therefore a real, acknowledged grey area.
- **Realistic worst case: a DMCA takedown**, not prosecution of users. If Egis
  requests removal, this repository comes down — and that is an acceptable trade
  against leaving the hardware unusable for its owners. Others have shipped
  interop drivers under the same conditions.
- **The original DLL and its full decompilation are deliberately excluded** (see
  `.gitignore`). Only the minimum needed for the device to work is present here.

## Why the reverse-engineered matcher (and not a clean-room one)?

A clean-room matcher was seriously attempted — block-local normalized cross-
correlation, band-limited phase-only correlation, NBIS minutiae/bozorth, ridge-
orientation fields, Gabor FingerCode, and census/LBP texture. **Not one was usable on this tiny
70×57-pixel sensor**, and they failed in two different ways. The minutiae route
(NBIS/bozorth, in an early `FpImageDevice` version of this driver) could not accept
a *genuine* finger at all — measured 0 % genuine accept, because the patch is too
small to carry enough minutiae. The rest reached usable genuine-accept rates but
could not reliably tell an *adjacent same-hand finger* (e.g. your own middle
finger) from an enrolled index finger — a real false-accept hole. Egis' purpose-built
matcher does separate them.

A driver that any adjacent finger can unlock would be worse than no driver, so the
reverse-engineered matcher is used. **A viable clean-room matcher is the single most
valuable contribution this project could receive** — it would make the driver
cleanly upstreamable to libfprint.

Since v0.4.2 a clean-room correlation matcher by Thaddeus Stepanovich ships
in-tree (`egis0576/tsteppy/egis_match.{c,h}`, adapter
`egis0576/egis_engine_cleanroom.c`) and can be built instead of the vendor matcher
with `-Degis0576_matcher=cleanroom` (see the README, "Experimental — clean-room
matcher"). The `gabor` flavour (`egis0576/gabor/`, own work, LGPL) keeps his
adapter and NCC but replaces the front-end; on the reference dataset it is the
first clean-room configuration whose genuine and impostor populations do not
overlap (0 % / 0 % at the shipped threshold, cross-fold checked), measured so far
on one unit only. Measured on identical captures from three units, one person each
([`docs/matcher-comparison.md`](docs/matcher-comparison.md)): the vendor matcher
scored 0/60, 2/60 and 37/60 false rejects and 0/480 false accepts on each; the
clean-room matcher rejected 35 %, 73.3 % and 93.3 % of genuine presses at its
published threshold, its genuine/impostor scores overlap on every run (EER 15 %,
35 %, 45 %), and on one of the three it admitted 5 of 480 impostor comparisons
where the vendor matcher admitted none. So it is not a drop-in replacement —
which is why the vendor matcher remains the default. It is, however, the
starting point for one. (Genuine acceptance of the *vendor* matcher varies
sharply between those runs too; that is a separate, unexplained finding, not an
argument about the clean-room matcher.)
