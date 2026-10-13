# v0.6 decision: matcher v4.1 on other people's units — pre-registration

Written 2026-10-14, before any result of matcher v4.1 on a unit other than the
maintainer's exists. Not to be changed after the first such result arrives;
any later change is a new, dated section below, never an edit of this one.

## What is being decided

Whether the clean-room matcher **v4.1** (the `gabor` flavour with the
identity-evidence gate; branch `identity-gate`, matcher commit `2bafaf3`,
`git rev-parse 2bafaf3:driver` = `824b6718ded9e937481cd0d862daec232da59d4e`)
becomes the default matcher of release v0.6.

## Why there is a decision to make

On the maintainer's unit, v4.1 was certified against a fresh, sealed session
(2026-09-28, pre-registered 2026-09-28, run 2026-09-29, two independent
certifiers with identical numbers):

- **False accepts: 0 of 2,400 impostor presses** over 9 enrol/verify sets,
  every impostor press at level 0.000x of the accept point under the driver's
  rule.
- **False rejects: 74 of 540 genuine presses (13.7 %) against the vendor
  matcher's 66 of 540 (12.2 %)** on the same presses. Discordant presses
  40 vs 32 (exact McNemar p = 0.41 two-sided, 0.20 one-sided). The
  pre-registered bar then was "pooled false rejects <= vendor", so v4.1 was
  **not certified**.

The released default (v0.5.x, `gabor` without the gate) is known to accept
other fingers under the driver's rule: 18 of 480 impostor presses on the
maintainer's own second session (docs/matcher-comparison.md, 2026-09-26) and
2 of 480 on sam-dant's unit (#5, kit_version 3, 2026-09-26).
On 2026-10-13 the maintainer set the bar for v0.6 to: **no false accept, and
not measurably worse than the vendor matcher at rejecting genuine presses,
on units other than his own** — and asked for the measurement before any such
number existed.

**This bar is looser than the one v4.1 missed, and it was set after that
miss.** Applied to the certification presses above, rule (b) below gives
one-sided p = 0.20: a pass. That is exactly why the maintainer's unit cannot
test it — the bar was chosen knowing its result — and why only units other
than his count.

## The rule

Applied by `decide.py` in this directory (sha256 `82672fa3aa508bbfd0a4d5c51302bd296867867bef97d287c9c734039707c29f`)
to the `results.json` files of `tools/accuracy/evaluate.py`, comparing the
`gabor` flavour (v4.1) with the `vendor` flavour under the driver's rule
(two consecutive finger-on frames + raw corroboration, gallery reloaded for
every press, kit_version 5):

- **(a)** per dataset: v4.1 confirms **no** impostor press. Any -> FAIL.
- **(c)** per dataset: v4.1's strongest impostor press, scored frame by frame
  without the raw corroboration, reaches at most **0.67x** the accept point.
  Above -> FAIL. (On the certification session this measure reached 0.56x,
  while every impostor press scored 0.000x under the driver's full rule.)
- **(b)** pooled over all datasets, genuine presses paired press by press:
  b = presses vendor accepts and v4.1 rejects, c = the reverse;
  one-sided exact McNemar p = P(X >= b | X ~ Bin(b + c, 1/2)).
  **p < 0.05 -> FAIL** ("measurably worse than vendor").
- A dataset where v4.1 cannot enrol or score a finger that vendor can is a
  FAIL for that dataset.

A file is INVALID (excluded, reported, and the verdict cannot be PASS while
it is excluded) when it was not produced by this kit as published: kit
version, threshold 5000, the driver's enrolment frame, the per-press lists,
and the build provenance (driver tree = 2bafaf3's, kit tree = a reviewed one,
clean checkout, default up-to-date builds of both scorers). The full list
is in decide.py's docstring.

**Power, stated up front.** On the maintainer's data v4.1's rejections are a
superset of vendor's (c = 0). Then (b) can only fail once b >= 5. With two or
three datasets of 60 genuine presses each, a PASS on (b) therefore means *no
measurable deficit on these units*, not *equivalent*. All counts are
published either way.

## Data

- `results.json` files from **units other than the maintainer's**, produced
  with the published `identity-gate` branch (kit tree
  `e9d006c33f229b965933235faacb7cac578ef062`), posted between
  **2026-10-14 and 2026-11-04 23:59 UTC** (issue #5 or any other channel the
  maintainer receives them on).
- Any capture date: existing datasets and fresh captures both count. v4.1 has
  never been run on any of them; what was known during its development is the
  summary numbers their owners posted in #5.
- One file per dataset; several datasets per unit count. Fingers of a second
  person inside a dataset count as impostors exactly as evaluate.py defines
  them.
- The maintainer's own unit is excluded (all its sessions are development
  data now).

## When and how the decision is made

`decide.py` is run **once**, on every valid file of the window, after the
window closes. Exception: a per-dataset FAIL under (a), (c) or the enrol rule
is final as soon as it is seen, because no further data can undo it.

- **PASS** -> v0.6 ships v4.1 as the default matcher in every package (the
  Ubuntu TOD package with the agreement of its maintainer), and the upstream
  libfprint submission is resumed.
- **FAIL** -> no default change; the failure is diagnosed and published.
- **INCOMPLETE** -> reruns are asked for; the window is extended once by
  seven days.
- **NO DATA** -> reported; the maintainer decides how to continue.

## What this does not test

The impostor population is the participants' own fingers (plus any second
person they choose to include), not the population at large; identify (as
opposed to verify) is not measured separately; long-term use is not covered.
