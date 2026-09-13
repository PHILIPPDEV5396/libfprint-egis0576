# Builds the two offline scorers straight from the repo's driver sources.
# No libfprint build tree, no meson, no glib: a C compiler and libm only.
#
#   make            -> score-vendor score-cleanroom (next to this Makefile)
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
CLEAN_CFLAGS  := $(COMMON) -Wall -I$(DRV)/tsteppy
SCORE_CFLAGS  := $(COMMON) -Wall

VENDOR_SRC := egis_funcs.c egis_rt.c egis_engine.c egis_preprocess.c egis_coherence_map.c
CLEAN_SRC  := egis_engine_cleanroom.c tsteppy/egis_match.c

VENDOR_OBJ := $(addprefix $(OBJ)/vendor/,$(VENDOR_SRC:.c=.o))
CLEAN_OBJ  := $(addprefix $(OBJ)/cleanroom/,$(notdir $(CLEAN_SRC:.c=.o)))

all: $(HERE)score-vendor $(HERE)score-cleanroom

$(HERE)score-vendor: $(OBJ)/score.o $(VENDOR_OBJ)
	$(CC) -o $@ $^ -lm

$(HERE)score-cleanroom: $(OBJ)/score.o $(CLEAN_OBJ)
	$(CC) -o $@ $^ -lm

$(OBJ)/score.o: $(HERE)score.c $(DRV)/egis_engine.h
	@mkdir -p $(dir $@)
	$(CC) $(SCORE_CFLAGS) -c -o $@ $<

$(OBJ)/vendor/%.o: $(DRV)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(VENDOR_CFLAGS) -c -o $@ $<

$(OBJ)/cleanroom/egis_engine_cleanroom.o: $(DRV)/egis_engine_cleanroom.c
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<

$(OBJ)/cleanroom/egis_match.o: $(DRV)/tsteppy/egis_match.c
	@mkdir -p $(dir $@)
	$(CC) $(CLEAN_CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ) $(HERE)score-vendor $(HERE)score-cleanroom

.PHONY: all clean
