// Prototypes for the host functions the generated C calls without declaring
// them. Force-included into RecompiledFuncs and RecompiledPatches: on a 32-bit
// target an implicitly declared function taking ctx->r4 (a 64-bit value) gets
// its arguments wrong.
#ifndef DK64_HOST_PROTOS_H
#define DK64_HOST_PROTOS_H

#include "recomp.h"

// The originals in RecompiledFuncs must yield to the patched versions in
// RecompiledPatches. Clang's RECOMP_FUNC is weak, which gives that for free;
// GCC's is not, so this header (force-included only into RecompiledFuncs)
// redefines it weak for the originals. The patches keep the strong symbol.
#undef RECOMP_FUNC
#define RECOMP_FUNC __attribute__((weak, noipa, optimize("rounding-math")))

#ifdef __cplusplus
extern "C" {
#endif

// Called from the us.toml hook text as load_dk64_overlay(ctx->r4, 0, 0).
void load_dk64_overlay(uint32_t compressed_rom, int32_t ram_addr, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
