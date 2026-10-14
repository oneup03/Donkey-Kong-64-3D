// Particle sprites thinned (the 3DS Game page's Particles row: Full, Half,
// Quarter; an Old 3DS starts at Quarter). DK64 draws its particles as
// sprites: sparkles, dust, splashes and explosions are bursts of
// drawSpriteAtPosition calls, and each sprite is a draw of its own, with
// its own texture frame and colour, for the renderer (a draw costs an Old
// 3DS about 45 us between the read and the replay). A burst is spawns of
// one sprite definition within a few milliseconds of each other: the first
// is kept, then every divisor-th, and the rest are hidden from the draw.
// The spawn itself still happens and still returns its sprite, since the
// callers keep the pointer (menus, the HUD, multiplayer icons) and the
// sprite's own logic runs as before; a sprite spawned once, or once a
// frame, is never hidden. HUD-aligned spawns and sprites attached to actors
// or bones (func_global_asm_80714C08, func_global_asm_80714B84) are never
// thinned. Hidden sprites are remembered by their address, set or cleared at
// every spawn, so a reused address is never stale.
#ifdef DK64_3DS
#include "common_structs.h"
#include "patch_helpers.h"

DECLARE_FUNC(s32, recomp_get_particle_divisor);

extern u8 D_global_asm_807FDB1D;    // the spawn's alignment mode: non-zero is HUD-aligned

#define DK64_3DS_BURST_CYCLES ((u32)OS_USEC_TO_CYCLES(8000))   // spawns closer than this are one burst
#define DK64_3DS_BURST_DEFS 16
#define DK64_3DS_HIDDEN_SLOTS 512

typedef struct {
    void *def;      // the sprite definition spawned
    u32 cycles;     // osGetCount at its last spawn
    u32 count;      // spawns in the burst so far
} BurstDef;

static BurstDef s_bursts[DK64_3DS_BURST_DEFS];
static u32 s_burst_next = 0;
static Struct80717D84 *s_hidden[DK64_3DS_HIDDEN_SLOTS];

static u32 hidden_slot(Struct80717D84 *sprite) {
    return ((u32)sprite >> 4) & (DK64_3DS_HIDDEN_SLOTS - 1);
}

// Whether the spawn about to happen of def is the thinned part of a burst:
// the k-th of one with k - 1 not a multiple of the divisor.
s32 dk64_3ds_particle_thinned(void *def) {
    s32 divisor = recomp_get_particle_divisor();
    u32 now;
    s32 i;
    BurstDef *b;

    if (divisor <= 1 || D_global_asm_807FDB1D != 0) {
        return FALSE;
    }
    now = osGetCount();
    b = NULL;
    for (i = 0; i < DK64_3DS_BURST_DEFS; i++) {
        if (s_bursts[i].def == def) {
            b = &s_bursts[i];
            break;
        }
    }
    if (b == NULL) {
        b = &s_bursts[s_burst_next];
        s_burst_next = (s_burst_next + 1) % DK64_3DS_BURST_DEFS;
        b->def = def;
        b->count = 0;
    } else if ((u32)(now - b->cycles) > DK64_3DS_BURST_CYCLES) {
        b->count = 0;   // a new burst
    }
    b->cycles = now;
    b->count++;
    return ((b->count - 1) % (u32)divisor) != 0;
}

// After the spawn: the sprite remembered as hidden, or forgotten.
void dk64_3ds_particle_mark(Struct80717D84 *sprite, s32 thinned) {
    u32 slot;
    if (sprite == NULL) {
        return;
    }
    slot = hidden_slot(sprite);
    if (thinned) {
        s_hidden[slot] = sprite;
    } else if (s_hidden[slot] == sprite) {
        s_hidden[slot] = NULL;
    }
}

// The draw's test (patches_ui.c, func_global_asm_80715E94).
s32 dk64_3ds_sprite_hidden(Struct80717D84 *sprite) {
    return sprite != NULL && s_hidden[hidden_slot(sprite)] == sprite;
}
#endif
