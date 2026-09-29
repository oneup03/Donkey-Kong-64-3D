#ifndef __PATCH_GRAPHICS_H__
#define __PATCH_GRAPHICS_H__

#include "patch_helpers.h"

DECLARE_FUNC(void, recomp_get_window_resolution, u32*, u32*);
DECLARE_FUNC(float, recomp_get_target_aspect_ratio, float);
DECLARE_FUNC(s32, recomp_get_target_framerate, s32);
DECLARE_FUNC(s32, recomp_high_precision_fb_enabled);
DECLARE_FUNC(float, recomp_get_resolution_scale);
// Stereo scene classification: 1 = this scene should use reduced convergence
// (menu / cutscene / minigame), 0 = the user's full configured convergence.
// The host applies the scale in apply_pending_stereo_config().
DECLARE_FUNC(void, recomp_stereo_set_low_convergence_scene, s32 active);

// Stereo first-person state: 1 = the first-person camera is live, so the aiming
// reticle is on screen. The renderer only looks for the reticle while this is
// set, which is what keeps the pause menu and bananaport transition - both of
// which draw reticle-shaped quads near the middle of the screen - out of it.
DECLARE_FUNC(void, recomp_stereo_set_first_person, s32 active);

// Fill a rectangle of an RGBA16 framebuffer in RDRAM with what is on screen,
// for game code that reads pixels (the fairy camera). xy = x << 16 | y and
// wh = w << 16 | h in framebuffer pixels. A no-op where the renderer already
// keeps RDRAM up to date (the desktop); the 3DS renders on its GPU only.
DECLARE_FUNC(void, recomp_fb_readback, u32 fb, s32 xy, s32 wh, s32 fb_width);

// DECLARE_FUNC(float, recomp_get_aspect_ratio, float);
// DECLARE_FUNC(s32, recomp_get_target_framerate, s32);
// DECLARE_FUNC(s32, recomp_high_precision_fb_enabled);
// DECLARE_FUNC(float, recomp_get_resolution_scale);

#endif