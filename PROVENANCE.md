# Provenance & Licensing

This document is an honest account of where the code in this repository comes from,
because part of it is a genuine copyright grey area and you deserve to know that
before you use or fork it.

## What this is

A [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint) driver for the
**EgisTec (LighTuning) EH576** fingerprint sensor (USB ID `1c7a:0576`), which ships
in several laptops but has **no vendor-provided Linux driver**. It was produced by
reverse-engineering the Windows driver (`EgisTouchFP0576.dll`) for the sole purpose
of **interoperability** — making hardware that people already own usable on Linux.

## Two categories of code

### 1. Original work — © the author, LGPL-2.1-or-later

Written from scratch for libfprint:

| File | What it is |
|------|-----------|
| `egis0576.c`, `egis0576.h` | The libfprint `FpDevice` driver: enroll/verify/identify state machines, capture worker thread, per-boot flat-field, software finger detection. |
| `egis0576/egis0576_tls.c`, `.h` | The TLS-PSK transport (OpenSSL/libcrypto over gusb), per-device gain calibration, init orchestration. |
| `egis0576/egis_rt.c`, `.h`, `egis_engine.c`, `.h` | The flat-memory runtime and the enroll/verify/gallery wrapper around the matcher. |

These files are licensed **LGPL-2.1-or-later**, matching libfprint. See `LICENSE`.

### 2. Reverse-engineered for interoperability — no copyright claimed by the author

Derived from Egis Technology's proprietary Windows driver and included because no
clean-room equivalent proved viable (see below):

| File | What it is |
|------|-----------|
| `egis0576/egis_funcs.c` | Egis' fingerprint feature extractor + matcher, translated from the decompiled DLL to native C. |
| `egis0576/egis_coherence_map.c` | The ridge-coherence estimator and its 6 lookup tables, extracted byte-exact from the DLL `.rdata`. |
| `egis0576/egis_preprocess.c` | The image preprocessing pipeline (min-subtract, invert, Otsu contrast stretch, vertical flip). |
| `egis0576/egis_tls_init.h` | The sensor initialization command sequence. |
| `egis0576/egis_blobs.h`, `egis_dat.h`, `egis_decls.h`, `egis_intrin.h` | Extracted data tables and support declarations for the translated code. |
| The TLS-PSK pre-shared key in `egis0576_tls.c` | A per-**model** constant recovered from the DLL — required to speak to the sensor at all. |

The author **does not claim copyright** over this second category. It is the
intellectual property of Egis Technology Inc. and is reproduced here only to the
extent necessary for the device to function.

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
orientation fields, Gabor FingerCode, and census/LBP texture. **Every one of them
failed the decisive security case:** on this tiny **70×57-pixel** sensor they could
not reliably tell an *adjacent same-hand finger* (e.g. your own middle finger) apart
from an enrolled index finger — a real false-accept hole. Egis' purpose-built
matcher does separate them.

A driver that any adjacent finger can unlock would be worse than no driver, so the
reverse-engineered matcher is used. **A viable clean-room matcher is the single most
valuable contribution this project could receive** — it would make the driver
cleanly upstreamable to libfprint.
