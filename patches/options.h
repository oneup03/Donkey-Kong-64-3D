#ifndef __PATCH_OPTIONS_H__
#define __PATCH_OPTIONS_H__

#include "patch_helpers.h"

DECLARE_FUNC(int, recomp_get_story_skip);
DECLARE_FUNC(int, recomp_get_camera_type);
DECLARE_FUNC(float, recomp_get_lightning_intensity);
DECLARE_FUNC(int, recomp_get_cutscene_bordering);
DECLARE_FUNC(int, recomp_get_mp_enabled);
// The built-in mods switched on, BUILTIN_MOD_* bits (builtin_mods.h; builds
// with DK64_BUILTIN_MODS).
DECLARE_FUNC(unsigned int, recomp_get_builtin_mods);

#endif