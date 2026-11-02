# Driver for the 3DS build. Host code generation first, then the cross build.
#
#   make nmr-patch    apply the 3DS runtime patch series to lib/N64ModernRuntime (once)
#   make host-tools   build N64Recomp, RSPRecomp and file_to_c (once)
#   make codegen      run the recompiler over the ROM and the patches
#   make patches      the patches alone (with the mods in lib/mods built in)
#   make              configure + build the .3dsx
#   make cia          installable title (needs makerom + bannertool)
#   make deploy-azahar / run-azahar / hw N3DS_IP=...
#
# Needs DEVKITPRO (user-prefix installs are fine), clang with the MIPS target
# and ld.lld for the patches (PATCH_CC=... / LLD=... to name other ones), and
# the submodules, rt64-3ds (lib/rt64-3ds; RT64_3DS=... for a checkout
# elsewhere) and the built-in mods (lib/mods) among them.
ROOT       := $(abspath ../..)
RT64_3DS   ?= $(ROOT)/lib/rt64-3ds
BUILD      := $(ROOT)/build-3ds
HOST       := $(ROOT)/build-host
PATCH_CC   ?= clang
LLD        ?= ld.lld
# The translations' objects are renamed with these (patches/lang_prefix.sh).
LLVM_NM      ?= llvm-nm
LLVM_OBJCOPY ?= llvm-objcopy
LLD_LIBS   ?=
TARGET     := DK64Recompiled3DS
AZAHAR_SD  := $(HOME)/.var/app/org.azahar_emu.Azahar/data/azahar-emu/sdmc
APPDIR     := $(AZAHAR_SD)/3ds/DK64

export DEVKITPRO ?= $(HOME)/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM

all: build

# The runtime needs a small patch series for the 3DS (kept in rt64-3ds so
# every game applies the same one). Idempotent.
nmr-patch:
	$(RT64_3DS)/nmr-patches/apply.sh $(ROOT)/lib/N64ModernRuntime

host-tools:
	cmake -S $(ROOT)/lib/N64ModernRuntime/N64Recomp -B $(HOST) -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build $(HOST) --target N64RecompCLI RSPRecomp -j
	g++ -std=c++17 -O2 -o $(HOST)/file_to_c $(ROOT)/lib/rt64/src/tools/file_to_c/file_to_c.cpp

# libultra's matrix helpers run as rt64-3ds's host functions on the 3DS
# (gu_native.cpp): the game's recompilation runs on a copy of its config
# with them on the ignored list, so they are not recompiled and the calls,
# the patches' too, resolve to the host functions of the same names. The
# desktop config stays as it is.
GU_IGNORED := "guMtxL2F", "guMtxF2L", "guMtxIdentF", "guMtxIdent", "guMtxCatF", "guMtxXFMF", "guMtxXFML", "guNormalize", "guRotateF", "guTranslateF", "guScaleF",
# DK64's own bone-matrix builder likewise (bones_native.cpp): hand-written
# MIPS, the game core's biggest cost recompiled.
GU_IGNORED += "func_global_asm_806195D0",

codegen:
	cd $(ROOT) && sed '/^ignored = \[/a\    $(GU_IGNORED)' us.toml > us.3ds.toml && $(HOST)/N64Recomp us.3ds.toml
	cd $(ROOT) && $(HOST)/RSPRecomp n_aspMain.toml
	$(MAKE) patches

# The patches alone (after editing patches/*.c), with the mods in lib/mods
# built in (a setting each on the 3DS). Those submodules are marked
# update = none, so recursive updates skip them (and their own nested
# submodules, which the build does not need); a missing one is fetched here.
# Each mod's src/ and include/ are copied, the patches in mods/<mod>/
# applied to the copy (the submodules stay as they are), and the patch
# build told where the copies are. The patches are recompiled from a copy of
# patches.toml with trace_mode on, as the game's functions are (trace.h): the
# preemption check in patched functions too, and in a DK64_GAME_PROFILE build
# the sampling profile naming them rather than booking their time to the
# nearest traced caller.
MODS_WORK := $(HOST)/mods
# The translations (a Language row: one at a time, at restart): lib/mods/
# <repo> and the tag its symbols take in the patches (patches/Makefile,
# LANG_MODS_DIR); copied to $(LANG_WORK)/<tag> as the built-in mods are.
LANG_MODS := RecompFrench:fr RecompGerman:de RecompSpanish:es RecompBrazilianPortuguese:ptbr
LANG_WORK := $(HOST)/lang_mods
BUILTIN_MODS := $(filter-out $(foreach l,$(LANG_MODS),$(firstword $(subst :, ,$(l)))),$(notdir $(wildcard $(ROOT)/lib/mods/*)))
patches:
	cd $(ROOT) && for m in $(BUILTIN_MODS) $(foreach l,$(LANG_MODS),$(firstword $(subst :, ,$(l)))); do \
	    [ -e lib/mods/$$m/src ] || git submodule update --init --checkout lib/mods/$$m || exit 1; done
	rm -rf $(MODS_WORK) $(LANG_WORK)
	for m in $(BUILTIN_MODS); do \
	    mkdir -p $(MODS_WORK)/$$m && cp -r $(ROOT)/lib/mods/$$m/src $(ROOT)/lib/mods/$$m/include $(MODS_WORK)/$$m/ || exit 1; \
	    for p in $(CURDIR)/mods/$$m/*.patch; do [ -e "$$p" ] || continue; \
	        (cd $(MODS_WORK)/$$m && patch -p1 --quiet --no-backup-if-mismatch < "$$p") || exit 1; done; done
	for l in $(LANG_MODS); do m=$${l%%:*}; t=$${l##*:}; \
	    mkdir -p $(LANG_WORK)/$$t && cp -r $(ROOT)/lib/mods/$$m/src $(ROOT)/lib/mods/$$m/include $(LANG_WORK)/$$t/ || exit 1; \
	    for p in $(CURDIR)/mods/$$m/*.patch; do [ -e "$$p" ] || continue; \
	        (cd $(LANG_WORK)/$$t && patch -p1 --quiet --no-backup-if-mismatch < "$$p") || exit 1; done; done
	cd $(ROOT)/patches && $(MAKE) clean && $(if $(LLD_LIBS),LD_LIBRARY_PATH=$(LLD_LIBS)) $(MAKE) CC=$(PATCH_CC) LD=$(LLD) NM=$(LLVM_NM) OBJCOPY=$(LLVM_OBJCOPY) BUILTIN_MODS_DIR=$(MODS_WORK) LANG_MODS_DIR=$(LANG_WORK) PLATFORM_CPPFLAGS="-DDK64_3DS -I$(RT64_3DS)/include"
	cd $(ROOT) && sed '/^\[input\]/a\trace_mode = true' patches.toml > patches.3ds.toml && $(HOST)/N64Recomp patches.3ds.toml
	$(HOST)/file_to_c $(ROOT)/patches/patches.bin bk_patches_bin $(ROOT)/RecompiledPatches/patches_bin.c $(ROOT)/RecompiledPatches/patches_bin.h

configure:
	cmake -S . -B $(BUILD) -G Ninja -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_TOOLCHAIN_FILE=$(RT64_3DS)/cmake/devkitARM-3ds.cmake -DRT64_3DS_DIR=$(RT64_3DS)

build:
	@[ -f $(BUILD)/build.ninja ] || $(MAKE) configure
	cmake --build $(BUILD) -j
	@$(DEVKITARM)/bin/arm-none-eabi-size $(BUILD)/$(TARGET) | tail -1

deploy-azahar: build
	mkdir -p $(APPDIR)
	cp -f $(BUILD)/$(TARGET).3dsx $(APPDIR)/
	@[ -e $(AZAHAR_SD)/3ds/dspfirm.cdc ] || : > $(AZAHAR_SD)/3ds/dspfirm.cdc
	@echo "deployed to $(APPDIR) (the ROM goes there as DK64.z64)"

run-azahar: deploy-azahar
	flatpak run --env=QT_QPA_PLATFORM=xcb org.azahar_emu.Azahar -w $(APPDIR)/$(TARGET).3dsx

cia: build
	cmake --build $(BUILD) --target $(TARGET)_cia
	@ls -l $(BUILD)/$(TARGET).cia

hw: build
	$(DEVKITPRO)/tools/bin/3dslink -a $(N3DS_IP) -s $(BUILD)/$(TARGET).3dsx

clean:
	rm -rf $(BUILD)

.PHONY: all nmr-patch host-tools codegen patches configure build cia deploy-azahar run-azahar hw clean
