# Builds the two offline scorers straight from the repo's driver sources.
# No libfprint build tree, no meson, no glib: a C compiler and libm only.
#
#   make            -> score-vendor score-cleanroom score-gabor pairdiag (next to this Makefile)
#                      and the cross-session scorers score_xs-vendor score_xs-cleanroom score_xs-gabor
#                      (+ build-info-<flavour>.txt: how score-<flavour> was built, see "Build provenance")
#   make clean
#
# The vendor objects are Egis' matcher machine-translated from the Windows
# driver; they only compile with the relaxed flag set below (the same set the
# libfprint meson build applies to them). The .rdata/.data blobs they need are
# embedded in egis_blobs.h and mmap'd at fixed addresses by egis_rt.c -- no
# external files at run time.

HERE := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
DRV  := $(abspath $(HERE)../../driver/egis0576)
OBJ  := $(HERE)build

CC     ?= cc
COMMON := -std=gnu99 -O3 -fno-strict-aliasing -D_GNU_SOURCE -I$(DRV)
# the relaxed set from libfprint/meson.build's egis0576-engine block
VENDOR_CFLAGS := $(COMMON) -w -fno-stack-protector -fpermissive -fwrapv \
                 -Wno-int-conversion -Wno-incompatible-pointer-types \
                 -Wno-implicit-function-declaration
# The clean-room adapter is built against one of two front-ends: the shipped
# Gabor one (no flag) or, for the comparison, tsteppy's original
# (-DEGIS_CR_FRONTEND_TSTEPPY); see egis_engine_cleanroom.c.
# The tsteppy comparison build keeps the true best-of-template score
# (EGIS_CR_VERIFY_EARLY_EXIT=0): its raw corroboration is disabled (returns 1)
# and its early exit only cuts scores that are already over the accept point,
# so every frame's accept / reject is the driver's either way.
CLEAN_CFLAGS  := $(COMMON) -Wall -I$(DRV)/tsteppy -DEGIS_CR_VERIFY_EARLY_EXIT=0 \
                 -DEGIS_CR_FRONTEND_TSTEPPY
# GABOR_EXTRA passes constants to the shipped front-end without editing it, so
# a reporter can measure a proposed change on their own captures in one line:
#   make clean && make GABOR_EXTRA=-DEG_MIN_OVERLAP=1400
# (the alignment search's overlap floor, egis_match_gabor.c; every #define in
# gabor/*.h is guarded by #ifndef for exactly this.)
# The Gabor build is compiled exactly as the driver compiles it: no
# EGIS_CR_VERIFY_EARLY_EXIT define, so verify stops at the first template
# frame (and pose) over the accept point, as it does in the driver. That is
# not a detail here: the raw corroboration (egis_verify_raw_ok) is asked at
# the pose verify recorded, so a build without the early exit makes other
# decisions than the driver (until 2026-09-27 this Makefile built it with
# =0, and the kit measured a matcher the driver does not run). Scores over
# the accept point are therefore the first ones to clear it, a lower bound
# of the best-of-template score; below it they are the best-of-template
# score. See README.md, "kit_version 5".
GABOR_EXTRA   ?=
GABOR_CFLAGS  := $(COMMON) -Wall -std=c99 -I$(DRV)/tsteppy -I$(DRV)/gabor \
                 $(GABOR_EXTRA)
SCORE_CFLAGS  := $(COMMON) -Wall

VENDOR_SRC := egis_funcs.c egis_rt.c egis_engine.c egis_preprocess.c egis_coherence_map.c
CLEAN_SRC  := egis_engine_cleanroom.c tsteppy/egis_match.c

VENDOR_OBJ := $(addprefix $(OBJ)/vendor/,$(VENDOR_SRC:.c=.o))
CLEAN_OBJ  := $(addprefix $(OBJ)/cleanroom/,$(notdir $(CLEAN_SRC:.c=.o)))
GABOR_OBJ  := $(OBJ)/gabor/egis_engine_cleanroom.o $(OBJ)/gabor/egis_match_gabor.o \
              $(OBJ)/gabor/egis_match_evidence.o

XS_BIN := $(HERE)score_xs-vendor $(HERE)score_xs-cleanroom $(HERE)score_xs-gabor

all: $(HERE)score-vendor $(HERE)score-cleanroom $(HERE)score-gabor $(HERE)pairdiag $(XS_BIN)

# Linking score-<flavour> also writes build-info-<flavour>.txt (write_info,
# "Build provenance" below).
$(HERE)score-vendor: $(OBJ)/score.o $(VENDOR_OBJ)
	$(CC) -o $@ $^ -lm
	$(call write_info,vendor)

$(HERE)score-cleanroom: $(OBJ)/score.o $(CLEAN_OBJ)
	$(CC) -o $@ $^ -lm
	$(call write_info,cleanroom)

$(HERE)score-gabor: $(OBJ)/score.o $(GABOR_OBJ)
	$(CC) -o $@ $^ -lm
	$(call write_info,gabor)

# the bare names (`make score-gabor`, and `make -q score-gabor`, which is how
# evaluate.py asks whether a binary is up to date)
score-vendor: $(HERE)score-vendor
score-cleanroom: $(HERE)score-cleanroom
score-gabor: $(HERE)score-gabor

# Cross-session scorers (score_xs.c, driven by cross.py): score.c with a second
# flat-field baseline for the probe session, linked against exactly the engine
# objects of the matching score-<flavour> above -- same flags, same objects, so
# a cross-session number is the same engine the in-session one measures.
# score_xs.c #includes score.c (one copy of the frame reader and flat field).
$(HERE)score_xs-vendor: $(OBJ)/score_xs.o $(VENDOR_OBJ)
	$(CC) -o $@ $^ -lm

$(HERE)score_xs-cleanroom: $(OBJ)/score_xs.o $(CLEAN_OBJ)
	$(CC) -o $@ $^ -lm

$(HERE)score_xs-gabor: $(OBJ)/score_xs.o $(GABOR_OBJ)
	$(CC) -o $@ $^ -lm

# the bare names (`make score_xs-gabor`), as for pairdiag below
score_xs-vendor: $(HERE)score_xs-vendor
score_xs-cleanroom: $(HERE)score_xs-cleanroom
score_xs-gabor: $(HERE)score_xs-gabor

# Diagnostic, not a scorer: coverage, winning-overlap, identity and flow
# aggregates for one dataset (pairdiag.c). Built against the Gabor front-end
# only, which pairdiag.c #includes (it needs the front-end's static helpers),
# so it is compiled from source here rather than linked as an object.
$(HERE)pairdiag: $(HERE)pairdiag.c $(DRV)/gabor/egis_match_gabor.c $(DRV)/gabor/egis_match_check.h \
                 $(DRV)/gabor/egis_cr_tuning_gabor.h $(DRV)/tsteppy/egis_match.h $(OBJ)/stamp-gabor
	$(CC) $(GABOR_CFLAGS) -o $@ $< -lm
# `make pairdiag` (what pairdiag.c tells you to run) names the target by its
# bare name; without this alias make falls back on its built-in %: %.c rule,
# which has none of the flags above and fails.
pairdiag: $(HERE)pairdiag

$(OBJ)/gabor/egis_engine_cleanroom.o: $(DRV)/egis_engine_cleanroom.c $(DRV)/gabor/egis_cr_tuning_gabor.h $(DRV)/gabor/egis_match_check.h \
                                      $(DRV)/gabor/egis_match_evidence.h $(OBJ)/stamp-gabor
	@mkdir -p $(dir $@)
	$(CC) $(GABOR_CFLAGS) -c -o $@ $<

$(OBJ)/gabor/egis_match_gabor.o: $(DRV)/gabor/egis_match_gabor.c $(DRV)/tsteppy/egis_match.h \
                                 $(DRV)/gabor/egis_match_evidence.h $(OBJ)/stamp-gabor
	@mkdir -p $(dir $@)
	$(CC) $(GABOR_CFLAGS) -c -o $@ $<

$(OBJ)/gabor/egis_match_evidence.o: $(DRV)/gabor/egis_match_evidence.c $(DRV)/gabor/egis_match_evidence.h \
                                    $(DRV)/tsteppy/egis_match.h $(OBJ)/stamp-gabor
	@mkdir -p $(dir $@)
	$(CC) $(GABOR_CFLAGS) -c -o $@ $<

$(OBJ)/score.o: $(HERE)score.c $(DRV)/egis_engine.h $(OBJ)/stamp-score
	@mkdir -p $(dir $@)
	$(CC) $(SCORE_CFLAGS) -c -o $@ $<

$(OBJ)/score_xs.o: $(HERE)score_xs.c $(HERE)score.c $(DRV)/egis_engine.h $(OBJ)/stamp-score
	@mkdir -p $(dir $@)
	$(CC) $(SCORE_CFLAGS) -c -o $@ $<

$(OBJ)/vendor/%.o: $(DRV)/%.c $(OBJ)/stamp-vendor
	@mkdir -p $(dir $@)
	$(CC) $(VENDOR_CFLAGS) -c -o $@ $<

$(OBJ)/cleanroom/egis_engine_cleanroom.o: $(DRV)/egis_engine_cleanroom.c $(OBJ)/stamp-cleanroom
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<

# The original front-end's translation search half-width (EM_SRCH, default 6
# as tsteppy ships it). His file guards its constants with #ifndef since his
# 97dbf8a, so `make EM_SRCH=19` just passes the define; nothing is generated.
# Measured on the reference dataset, 19 or 20 takes that flavour's FRR from
# 35 % to 3.3 % at an unchanged threshold; see docs/matcher-comparison.md.
ifneq ($(strip $(EM_SRCH)),)
CLEAN_CFLAGS += -DEM_SRCH=$(EM_SRCH)
endif
$(OBJ)/cleanroom/egis_match.o: $(DRV)/tsteppy/egis_match.c $(OBJ)/stamp-cleanroom
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<

# ---- Build provenance --------------------------------------------------------
# evaluate.py records in results.json how each scorer it ran was built, so a
# stale binary, or one built with extra defines, shows in the shared file.
# Per flavour this Makefile keeps a record of how its objects are compiled:
# the flavour's extra defines (GABOR_EXTRA, EM_SRCH), the compiler's version
# line and the flags, with the driver directory written repo-relative --
# nothing absolute, no user or host name (the compiler is named by its
# version line, never by its path). The record is a stamp, build/stamp-<name>,
# that the flavour's objects depend on, and it is rewritten only when its
# content would change (compared below, while make reads this file). So
# `make GABOR_EXTRA=...` after a plain `make`, or the other way round,
# recompiles that flavour instead of linking the old objects, an unchanged
# record leaves the stamp's date alone, and `make -q score-gabor` answers
# "would make rebuild it?" truthfully. Linking score-<flavour> copies the
# record next to the binary as build-info-<flavour>.txt, plus the binary's
# sha256, which evaluate.py checks against the binary it runs (and does not
# copy into results.json). This block must stay below every flag assignment
# (the EM_SRCH one above): the records are expanded where they are defined.
CC_VERSION := $(shell $(CC) --version 2>/dev/null | head -n 1)
DRV_REL    := $(if $(filter $(abspath $(HERE)../../driver/egis0576),$(DRV)),driver/egis0576,<overridden>)
sq    = '$(subst ','\'',$(1))'
rel   = $(strip $(subst $(DRV),$(DRV_REL),$(1)))
record = $(call sq,flavour: $(1)) $(call sq,extra_defines: $(strip $(2))) \
         $(call sq,driver_dir: $(DRV_REL)) $(call sq,cc: $(CC_VERSION)) \
         $(call sq,cflags: $(call rel,$(3))) $(call sq,score_cflags: $(call rel,$(SCORE_CFLAGS)))
INFO_vendor    := $(call record,vendor,,$(VENDOR_CFLAGS))
INFO_cleanroom := $(call record,cleanroom,$(if $(strip $(EM_SRCH)),-DEM_SRCH=$(strip $(EM_SRCH))),$(CLEAN_CFLAGS))
INFO_gabor     := $(call record,gabor,$(GABOR_EXTRA),$(GABOR_CFLAGS))
INFO_score     := $(call sq,cc: $(CC_VERSION)) $(call sq,score_cflags: $(call rel,$(SCORE_CFLAGS)))
# non-empty when build/stamp-$(1) does not hold exactly INFO_$(1)
stale = $(shell [ "$$(printf '%s\n' $(INFO_$(1)))" = "$$(cat $(OBJ)/stamp-$(1) 2>/dev/null)" ] || echo stale)
ifneq ($(call stale,vendor),)
$(OBJ)/stamp-vendor: FORCE
endif
ifneq ($(call stale,cleanroom),)
$(OBJ)/stamp-cleanroom: FORCE
endif
ifneq ($(call stale,gabor),)
$(OBJ)/stamp-gabor: FORCE
endif
ifneq ($(call stale,score),)
$(OBJ)/stamp-score: FORCE
endif
$(OBJ)/stamp-%:
	@mkdir -p $(dir $@)
	@printf '%s\n' $(INFO_$*) > $@
FORCE:

write_info = @{ cat $(OBJ)/stamp-$(1); printf 'sha256: %s\n' "$$(sha256sum $@ | cut -d ' ' -f 1)"; } \
             > $(OBJ)/build-info-$(1).tmp && mv -f $(OBJ)/build-info-$(1).tmp $(HERE)build-info-$(1).txt

clean:
	rm -rf $(OBJ) $(HERE)score-vendor $(HERE)score-cleanroom $(HERE)score-gabor $(HERE)pairdiag $(HERE)nbis-count \
	       $(XS_BIN) $(HERE)build-info-vendor.txt $(HERE)build-info-cleanroom.txt $(HERE)build-info-gabor.txt

# How many minutiae NBIS mindtct finds per frame (the "why not an image
# driver" number, docs/matcher-comparison.md). Needs a libfprint source tree
# for its bundled nbis and its meson version string:
#   make nbis-count LIBFPRINT_SRC=/path/to/libfprint
LIBFPRINT_SRC ?=
NBIS := $(LIBFPRINT_SRC)/libfprint/nbis
NBIS_VERSION := $(shell sed -n "s/^ *version: *'\([^']*\)'.*/\1/p" $(LIBFPRINT_SRC)/meson.build 2>/dev/null | head -1)
nbis-count: $(HERE)nbis_count.c
	@test -n "$(LIBFPRINT_SRC)" || { echo "make nbis-count LIBFPRINT_SRC=<libfprint checkout>"; exit 1; }
	$(CC) -O2 -w -o $(HERE)nbis-count $(HERE)nbis_count.c \
	    -DNBIS_LIBFPRINT_VERSION='"$(NBIS_VERSION)"' \
	    -I$(NBIS)/include -I$(NBIS)/libfprint-include -I$(LIBFPRINT_SRC)/libfprint \
	    $(wildcard $(NBIS)/mindtct/*.c) $$(pkg-config --cflags --libs glib-2.0) -lm

.PHONY: all clean nbis-count pairdiag score_xs-vendor score_xs-cleanroom score_xs-gabor \
        score-vendor score-cleanroom score-gabor FORCE
