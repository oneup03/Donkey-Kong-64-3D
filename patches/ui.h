#ifndef __UI_FUNCS_H__
#define __UI_FUNCS_H__

#include "patch_helpers.h"

DECLARE_FUNC(void, recomp_get_ui_bounds, s32*, s32*);
DECLARE_FUNC(s32, recomp_get_ui_pillar);
DECLARE_FUNC(float, recomp_get_draw_distance);
// The 3DS build's World distance row: the scale on the chunk cull's far
// plane (70% = 0.7), 1 at 100%.
DECLARE_FUNC(float, recomp_get_draw_scale);
// The 3DS build's Models row: 1 for Low (the Kongs' multiplayer models, the
// actors' far models).
DECLARE_FUNC(s32, recomp_get_low_models);

#endif