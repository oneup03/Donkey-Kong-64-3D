TARGET = patches.elf

CC      ?= clang
LD      ?= ld.lld

CFLAGS   := -D_LANGUAGE_C -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -DF3DEX_GBI_2 -mno-odd-spreg -mno-check-zero-division \
			-fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin-memset -fno-builtin \
			-Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable -Wno-missing-braces -Wno-unsupported-floating-point-opt
CPPFLAGS := -DF3DEX_GBI_2x -nostdinc -D_LANGUAGE_C -DMIPS -I dummy_headers -I ../lib/dk64_decomp/include -I ../lib/dk64_decomp/include/2.0L/PR -I ../lib/dk64_decomp/include/2.0L -I ../lib/dk64_decomp/include/libc -I ../lib/dk64_decomp/src -I../lib/rt64/include
LDFLAGS  := -nostdlib -T patches.ld -T syms.ld -Map patches.map --unresolved-symbols=ignore-all --emit-relocs

# A platform's own defines (the 3DS build passes -DDK64_3DS). The define
# changes objects too: `make clean` when switching.
CPPFLAGS += $(PLATFORM_CPPFLAGS)

C_SRCS := $(wildcard *.c)
C_OBJS := $(C_SRCS:.c=.o)
C_DEPS := $(C_SRCS:.c=.d)

# Mods built in, for builds without the mod loader (the 3DS):
# BUILTIN_MODS_DIR holds one directory per mod, a copy of lib/mods/<mod>
# (src/ and include/) with platform/3ds/mods/<mod>/*.patch applied, made by
# platform/3ds/Makefile. Each mod's sources see its own headers behind the
# stand-in modding.h (platform/3ds/mods), and this directory last, for
# builtin_mods.h. The define changes other objects too: `make clean` when
# switching.
ifneq ($(BUILTIN_MODS_DIR),)
MODS := $(notdir $(wildcard $(BUILTIN_MODS_DIR)/*))
CPPFLAGS += -DDK64_BUILTIN_MODS
# The mods declare the game's sqrtf by that name; here it is the host's
# sqrtf_recomp, as patches.h has it.
MOD_CPPFLAGS := -I ../platform/3ds/mods -Dsqrtf=sqrtf_recomp
define mod_rules
MOD_OBJS += $$(patsubst $(BUILTIN_MODS_DIR)/$(1)/src/%.c,mod_$(1)_%.o,$$(wildcard $(BUILTIN_MODS_DIR)/$(1)/src/*.c))
mod_$(1)_%.o: $(BUILTIN_MODS_DIR)/$(1)/src/%.c
	$$(CC) $$(CFLAGS) $$(MOD_CPPFLAGS) -I $(BUILTIN_MODS_DIR)/$(1)/include $$(CPPFLAGS) -I . $$< -c -o $$@
endef
$(foreach m,$(MODS),$(eval $(call mod_rules,$(m))))
endif

$(TARGET): $(C_OBJS) $(MOD_OBJS) patches.ld syms.ld
	$(LD) $(C_OBJS) $(MOD_OBJS) $(LDFLAGS) -o $@

$(C_OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
	rm -rf $(C_OBJS) $(TARGET) $(C_DEPS) mod_*.o

-include $(C_DEPS)

.PHONY: clean
