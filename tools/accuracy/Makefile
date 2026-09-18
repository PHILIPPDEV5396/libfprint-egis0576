# Builds the two offline scorers straight from the repo's driver sources.
# No libfprint build tree, no meson, no glib: a C compiler and libm only.
#
#   make            -> score-vendor score-cleanroom score-gabor (next to this Makefile)
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
# The kit wants the true best-of-template score, not the driver's
# first-frame-over-threshold shortcut (egis_engine_cleanroom.c).
CLEAN_CFLAGS  := $(COMMON) -Wall -I$(DRV)/tsteppy -DEGIS_CR_VERIFY_EARLY_EXIT=0
# The Gabor flavour is the same adapter on a different front-end, with the
# adapter's three operating-point constants force-included from the header
# that documents where they come from (driver/egis0576/gabor/).
GABOR_CFLAGS  := $(COMMON) -Wall -std=c99 -I$(DRV)/tsteppy \
                 -DEGIS_CR_VERIFY_EARLY_EXIT=0 \
                 -include $(DRV)/gabor/egis_cr_tuning_gabor.h
SCORE_CFLAGS  := $(COMMON) -Wall

VENDOR_SRC := egis_funcs.c egis_rt.c egis_engine.c egis_preprocess.c egis_coherence_map.c
CLEAN_SRC  := egis_engine_cleanroom.c tsteppy/egis_match.c

VENDOR_OBJ := $(addprefix $(OBJ)/vendor/,$(VENDOR_SRC:.c=.o))
CLEAN_OBJ  := $(addprefix $(OBJ)/cleanroom/,$(notdir $(CLEAN_SRC:.c=.o)))
GABOR_OBJ  := $(OBJ)/gabor/egis_engine_cleanroom.o $(OBJ)/gabor/egis_match_gabor.o

all: $(HERE)score-vendor $(HERE)score-cleanroom $(HERE)score-gabor

$(HERE)score-vendor: $(OBJ)/score.o $(VENDOR_OBJ)
	$(CC) -o $@ $^ -lm

$(HERE)score-cleanroom: $(OBJ)/score.o $(CLEAN_OBJ)
	$(CC) -o $@ $^ -lm

$(HERE)score-gabor: $(OBJ)/score.o $(GABOR_OBJ)
	$(CC) -o $@ $^ -lm

$(OBJ)/gabor/egis_engine_cleanroom.o: $(DRV)/egis_engine_cleanroom.c $(DRV)/gabor/egis_cr_tuning_gabor.h
	@mkdir -p $(dir $@)
	$(CC) $(GABOR_CFLAGS) -c -o $@ $<

$(OBJ)/gabor/egis_match_gabor.o: $(DRV)/gabor/egis_match_gabor.c $(DRV)/tsteppy/egis_match.h
	@mkdir -p $(dir $@)
	$(CC) $(GABOR_CFLAGS) -c -o $@ $<

$(OBJ)/score.o: $(HERE)score.c $(DRV)/egis_engine.h
	@mkdir -p $(dir $@)
	$(CC) $(SCORE_CFLAGS) -c -o $@ $<

$(OBJ)/vendor/%.o: $(DRV)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(VENDOR_CFLAGS) -c -o $@ $<

$(OBJ)/cleanroom/egis_engine_cleanroom.o: $(DRV)/egis_engine_cleanroom.c
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<

# The clean-room matcher's translation search half-width. Empty (the default)
# compiles Thaddeus' file exactly as it ships, EM_SRCH 6. Setting it, e.g.
#
#     make EM_SRCH=19
#
# compiles a generated copy whose only difference is that the constant can be
# overridden, and nothing else changes -- built with EM_SRCH=6 it reproduces the
# shipped scorer's numbers exactly. Measured on the reference dataset, 19 or 20
# takes the clean-room FRR from 35 % to 3.3 % at an unchanged threshold with no
# false accepts; see docs/matcher-comparison.md. His file is never edited: it is
# copied byte-for-byte from his repository (PROVENANCE.md category 3).
ifeq ($(strip $(EM_SRCH)),)
$(OBJ)/cleanroom/egis_match.o: $(DRV)/tsteppy/egis_match.c
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<
else
$(OBJ)/cleanroom/egis_match_srch.c: $(DRV)/tsteppy/egis_match.c
	@mkdir -p $(dir $@)
	sed 's|^#define EM_SRCH 6$$|#ifndef EM_SRCH_VAL\n#define EM_SRCH_VAL 6\n#endif\n#define EM_SRCH EM_SRCH_VAL|' $< > $@

$(OBJ)/cleanroom/egis_match.o: $(OBJ)/cleanroom/egis_match_srch.c
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -DEM_SRCH_VAL=$(EM_SRCH) -c -o $@ $<
endif

clean:
	rm -rf $(OBJ) $(HERE)score-vendor $(HERE)score-cleanroom $(HERE)score-gabor

.PHONY: all clean
