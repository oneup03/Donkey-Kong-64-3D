// The race arrows and the crosshair the renderer finds by its texture.
// rt64-3ds places the bonus games' crosshair by the 3D page's Crosshair row
// (at the depth under it, or the HUD's), finding it by its texture
// (main_3ds.cpp, dk64_aim_textures): the 12 frames of TABLE_14_TEXTURES_HUD
// 0x83..0x8E. The arrows func_global_asm_8068E474 draws in the races are
// frames of the same texture, and
// would be taken for a crosshair there and sit at the depth under the
// middle of the screen. While they are up the match is paused, from before
// the list they are drawn in is read until two frames after the last of
// them, as stereo_runtime.c holds first person.
#ifdef DK64_3DS
#include "common_structs.h"
#include "patches_ui.h"
#include "patches_main.h"
#include "patch_helpers.h"
#include "graphics.h"

#define AIM_PAUSE_HOLD_FRAMES 2

static u8 aim_pause_frames = 0;

// Once a frame (stereo_runtime_tick): the match back once the arrows are gone.
void aim_3ds_tick(void) {
    if (aim_pause_frames == 0) {
        return;
    }
    aim_pause_frames--;
    if (aim_pause_frames == 0) {
        recomp_stereo_set_aim_textures(1);
    }
}

// The gyro's pitch, turned by the angle the console turned. The game does
// not aim up and down by an angle: first person and the cannon move a look
// point a fixed distance ahead up and down, so the stick's step in height
// is about half a degree near level and less towards the limits, where the
// same gyro tilt as the turn's barely moves the aim. This raises a point
// `h` above the eye and `dist` ahead by `deg` degrees: tan(a + d) from
// tan(a) = h / dist and tan(d), the series being exact enough for a frame's
// turn. Past straight up or down it stops; the game's own limit on the
// height still applies after it.
f32 aim_3ds_raise(f32 h, f32 dist, f32 deg) {
    f32 d = deg * (MATH_PI_F / 180.0f);
    f32 u, t, den;

    if (d > 0.5f) {
        d = 0.5f;
    } else if (d < -0.5f) {
        d = -0.5f;
    }
    u = d + (d * d * d) / 3.0f;
    t = h / dist;
    den = 1.0f - (t * u);
    if (den < 0.05f) {
        den = 0.05f;
    }
    return dist * (t + u) / den;
}

typedef struct {
    u8 unk0;
} AAD_8068E474_unk34;

typedef struct {
    u8 unk0[0x34 - 0x0];
    AAD_8068E474_unk34 *unk34;
} AAD_8068E474;

// @recomp 3DS: the race arrows, the crosshair's texture match paused while they are up.
RECOMP_PATCH Gfx *func_global_asm_8068E474(Gfx *dl, Actor *arg1) {
    s16 rot;
    AAD_8068E474 *aaD;
    s16 temp;

    if (aim_pause_frames == 0) {
        recomp_stereo_set_aim_textures(0);
    }
    aim_pause_frames = AIM_PAUSE_HOLD_FRAMES;
    temp = arg1->unkEC + 0x5A;
    gSPDisplayList(dl++, &D_1000118);
    gDPSetCombineMode(dl++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gSPMatrix(dl++, &D_20000C0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    if (D_global_asm_807FBB64 & 0x80000) {
        aaD = arg1->additional_actor_data;
        rot = arg1->y_rotation;
        if (rot < 0x200 || rot >= 0xE01 || (rot >= 0x601 && rot < 0xA00)) {
            if ((aaD->unk34->unk0 != 0x10) && (aaD->unk34->unk0 != 0x13)) {
                dl = displayImage(dl, (((u32)object_timer >> 1) % 12U) + 0x83, 0, 2, 0x20, 0x10, 0x78, 0x1E0, 4.0f, 4.0f, 0, 0.0f);
                dl = displayImage(dl, (((u32)object_timer >> 1) % 12U) + 0x83, 0, 2, 0x20, 0x10, 0x488, 0x1E0, 4.0f, 4.0f, 0xB4, 0.0f);
            }
        } else {
            dl = displayImage(dl, (((u32)object_timer >> 1) % 12U) + 0x83, 0, 2, 0x20, 0x10, 0x280, 0x78, 4.0f, 4.0f, 0x5A, 0.0f);
            dl = displayImage(dl, (((u32)object_timer >> 1) % 12U) + 0x83, 0, 2, 0x20, 0x10, 0x280, 0x348, 4.0f, 4.0f, 0x10E, 0.0f);
        }
    } else {
        dl = displayImage(dl, (((u32)object_timer >> 1) % 12U) + 0x83, 0, 2, 0x20, 0x10, 0xB4, 0xC8, 8.0f, 8.0f, temp, 0.0f);
    }
    return dl;
}
#endif
