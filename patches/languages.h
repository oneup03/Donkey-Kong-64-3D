// The translations' hooks into the game's events (languages.c, built with
// DK64_LANGUAGES: the 3DS build's translations).
#ifndef __LANGUAGES_H__
#define __LANGUAGES_H__

#include "ultra64.h"

void language_on_init(void);
void language_asset_override(s32 tableIndex, s32 fileIndex, u8 **outputFile, s32 *file_size);
void language_every_frame(void);
void language_adjust_dl_allocation(s32 *allocation);

#endif
