// The HUD on the 3DS's touch screen (rt64-3ds's touch-screen HUD; the Game
// page's HUD row, on the touch screen by default). Marked for it: the health
// melons and the counters' numbers here, the counters' icons (sprites made by
// func_global_asm_806F9744 and the Golden Banana count's, flagged
// ALIGN_BOTTOM_SCREEN and marked as they are drawn, patches_ui.c) and the
// locked-camera icon. The minigame timer stays on the top screen. rt64-3ds
// places a marked draw at the game's own coordinates there and skips the
// marks while the HUD row says top screen.
//
// On the N64 a counter slides in when it changes and out again after a
// second or so, and the melons when Kong is hurt or healed. On the touch
// screen nothing covers the scene, so while the HUD is there:
// - the coloured bananas, the coins and the kong's Golden Bananas are up
//   from the start (races and minigames put their own counters, items 11, 13
//   and 14, in those places, and those win);
// - any other counter the game shows stays (its timer never runs out, and
//   one already on its way out comes back);
// - the melons are always up, at their full place.
// Not while the game hides its HUD (cutscenes, menus), nor in multiplayer.
#ifdef DK64_3DS
#include "common_structs.h"
#include "patches_ui.h"
#include "patches_main.h"
#include "patch_helpers.h"
#include "rt64_3ds_gbi.h"

DECLARE_FUNC(s32, recomp_get_hud_on_bottom);

extern void func_global_asm_806F8BC4(s32 HUDItemIndex, u8 arg1, s32 arg2);
extern void func_global_asm_806F9518(s32 HUDItemIndex);
extern Gfx *func_global_asm_806FA5A4(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_806FA7BC(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_806FA9C0(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_80708DA4(Gfx *dl, s16 playerIndex, CharacterChange *arg2);
extern Gfx *func_global_asm_807085D0(Gfx *dl, void *player, void *character, CharacterChange *arg3);
extern s32 func_global_asm_80714464(void);
extern s32 func_global_asm_806C8DE0(s32 playerIndex);
extern u8 D_global_asm_807FC950[];      // PlayerProgress[], 0x306 bytes each
extern u8 D_global_asm_80744504;

#define PLAYER_PROGRESS_SIZE 0x306
#define CHARACTER_PROGRESS_SIZE 0x5E

enum {
    HUD_COLOURED_BANANAS = 0,
    HUD_COINS = 1,
    HUD_GOLDEN_BANANAS = 8,
    HUD_RACE_COINS = 11,
    HUD_ALT_COINS = 13,
    HUD_ALT_BANANAS = 14,
};

// What the melons' draw tests too: the game shows its HUD.
static s32 hud_game_shows(void) {
    return !(global_properties_bitfield & 3) && !func_global_asm_80714464() && current_map != MAP_MAIN_MENU;
}

static s32 hud_item_up(s32 HUDItemIndex) {
    return D_global_asm_80754280->hud_item[HUDItemIndex].hud_state != 0;
}

// A core item whose place a race's or minigame's own counter holds.
static s32 hud_core_blocked(s32 HUDItemIndex) {
    switch (HUDItemIndex) {
        case HUD_COLOURED_BANANAS:
            return hud_item_up(HUD_ALT_BANANAS);
        case HUD_COINS:
            return hud_item_up(HUD_RACE_COINS) || hud_item_up(HUD_ALT_COINS);
        default:
            return FALSE;
    }
}

static void hud_keep_up(s32 HUDItemIndex) {
    HUDDisplay *item = &D_global_asm_80754280->hud_item[HUDItemIndex];
    s32 core = HUDItemIndex == HUD_COLOURED_BANANAS || HUDItemIndex == HUD_COINS || HUDItemIndex == HUD_GOLDEN_BANANAS;

    if (core && hud_core_blocked(HUDItemIndex)) {
        // Make room: out, as the game would take it.
        if (item->hud_state == 1 || item->hud_state == 2) {
            item->hud_state = 3;
        }
        return;
    }
    switch (item->hud_state) {
        case 0:
            if (core) {
                func_global_asm_806F8BC4(HUDItemIndex, 0, 0);
            }
            break;
        case 2:
            // Up until its timer runs out (func_global_asm_806FA7BC): never here.
            if (item->counter_timer < 2) {
                item->counter_timer = 2;
            }
            break;
        case 3:
            item->hud_state = 1;
            break;
    }
}

// @recomp 3DS: the counters' numbers (and the items' state), marked for the touch screen.
RECOMP_PATCH Gfx *func_global_asm_806FAB20(Gfx *dl) {
    s32 HUDItemIndex;
    s32 keep;

    if (cc_number_of_players >= 2) {
        return dl;
    }
    keep = recomp_get_hud_on_bottom() && hud_game_shows();
    gEX3DSBottomScreen(dl++, 0, 0);
    gDPPipeSync(dl++);
    gSPMatrix(dl++, &D_2000180, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    for (HUDItemIndex = 0; HUDItemIndex < 0xF; HUDItemIndex++) {
        if (keep) {
            hud_keep_up(HUDItemIndex);
        }
        switch (D_global_asm_80754280->hud_item[HUDItemIndex].hud_state) {
            case 0:
                func_global_asm_806F9518(HUDItemIndex);
                break;
            case 1:
                dl = func_global_asm_806FA5A4(HUDItemIndex, dl);
                break;
            case 2:
                dl = func_global_asm_806FA7BC(HUDItemIndex, dl);
                break;
            case 3:
                dl = func_global_asm_806FA9C0(HUDItemIndex, dl);
                break;
        }
    }
    gEX3DSTopScreen(dl++);
    return dl;
}

// The melons' own update draws them as they slide in and out; on the touch
// screen that update still runs (it applies the damage and the healing) but
// into this list, which nothing draws, and the melons are drawn at their
// full place instead. At most 3 melons of two images each: under 200
// commands.
static Gfx hud_melons_discarded[384];

// @recomp 3DS: the health melons, marked for the touch screen and always up there.
RECOMP_PATCH Gfx *func_global_asm_80709344(Gfx *dl) {
    s16 i;
    s32 single = cc_number_of_players == 1;
    s32 always = single && recomp_get_hud_on_bottom();

    if (single) {
        gEX3DSBottomScreen(dl++, 0, 0);
    }
    for (i = 0; i < cc_number_of_players; i++) {
        CharacterChange *cc = &character_change_array[i];
        if (!cc->does_player_exist) {
            continue;
        }
        if (!(cc->unk2DC.unk6 & 0x20)
        && (((cc->unk2DC.unk8
            || cc->unk2DC.unk6)
            && (D_global_asm_80744504 == 0))
            || (cc->unk2DC.unk6 & 0x10))) {
            if (always) {
                func_global_asm_80708DA4(hud_melons_discarded, i, cc);
            } else {
                dl = func_global_asm_80708DA4(dl, i, cc);
            }
        }
        if (always && !(cc->unk2DC.unk6 & 0x20) && (D_global_asm_80744504 == 0)
        && (func_global_asm_805FCA64() || (cc->unk2DC.unk6 & 0x80))) {
            // unk2DC.unk4: how far in the melons are (0..65535).
            u16 shown = cc->unk2DC.unk4;
            u8 *player = D_global_asm_807FC950 + i * PLAYER_PROGRESS_SIZE;
            cc->unk2DC.unk4 = 0xFFFF;
            dl = func_global_asm_807085D0(dl, player, player + func_global_asm_806C8DE0(i) * CHARACTER_PROGRESS_SIZE, cc);
            cc->unk2DC.unk4 = shown;
        }
    }
    if (single) {
        gEX3DSTopScreen(dl++);
    }
    return dl;
}
#endif
