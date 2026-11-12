// The HUD on the 3DS's touch screen (rt64-3ds's touch-screen HUD; the Game
// page's HUD row, on the top screen by default). Marked for it: the health
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
// - every counter of the level that means something for the kong is up
//   from the start: coloured bananas, coins, the Golden Bananas, the ammo
//   (homing once it has some) with the gun, oranges, crystals and film
//   while there are any, the instrument's with the instrument. Races and
//   minigames put their own counters (items 11, 13, 14) where the bananas
//   and coins go, and those win;
// - any other counter the game shows stays (its timer never runs out, and
//   one already on its way out comes back);
// - the melons are always up, at their full place.
// Only in the adventure (not the opening, DK TV or the minigames of the
// Mystery menu), not while the game hides its HUD (cutscenes, menus), nor in
// multiplayer. A counter's icon is the kong's it was made for
// (func_global_asm_806FACE8), so a kong switch makes them anew.
//
// The layout there: the right-hand column (x 290, all but the bottom row)
// moves left by HUD_BOTTOM_RIGHT_SHIFT, beside the left one, and the melons
// to the top right, which leaves the right of the screen under them to the
// touch buttons (GameDesc::touch_area_hud, main_3ds.cpp).
#ifdef DK64_3DS
#include "common_structs.h"
#include "patches_ui.h"
#include "patches_main.h"
#include "patch_helpers.h"
#include "rt64_3ds_gbi.h"

DECLARE_FUNC(s32, recomp_get_hud_on_bottom);
DECLARE_FUNC(void, recomp_set_hud_leaves, void *tiles);

extern void func_global_asm_806F8BC4(s32 HUDItemIndex, u8 arg1, s32 arg2);
extern void func_global_asm_806F9518(s32 HUDItemIndex);
extern Gfx *func_global_asm_806FA5A4(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_806FA7BC(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_806FA9C0(s32 HUDItemIndex, Gfx *dl);
extern Gfx *func_global_asm_80708DA4(Gfx *dl, s16 playerIndex, CharacterChange *arg2);
extern Gfx *func_global_asm_807085D0(Gfx *dl, void *player, void *character, CharacterChange *arg3);
extern s32 func_global_asm_80714464(void);
extern s32 func_global_asm_806C8DE0(s32 playerIndex);
extern int gameIsInAdventureMode(void);
extern u8 D_global_asm_807FC950[];      // PlayerProgress[], 0x306 bytes each
extern u8 D_global_asm_80744504;

#define PLAYER_PROGRESS_SIZE 0x306
#define CHARACTER_PROGRESS_SIZE 0x5E

enum {
    HUD_COLOURED_BANANAS = 0,
    HUD_COINS = 1,
    HUD_AMMO = 2,
    HUD_HOMING_AMMO = 3,
    HUD_ORANGES = 4,
    HUD_CRYSTALS = 5,
    HUD_FILM = 6,
    HUD_INSTRUMENT = 7,
    HUD_GOLDEN_BANANAS = 8,
    HUD_RACE_COINS = 11,
    HUD_ALT_COINS = 13,
    HUD_ALT_BANANAS = 14,
};

// CharacterProgress's bitfields (the decomp's structs.h).
#define KONG_WEAPON 2           // xxxxxshw: the gun, homing, sniper
#define KONG_INSTRUMENT 4       // its first bit: the instrument

// A fade or zipper (a game's load, a level's, a bananaport's) covers the
// top screen: D_global_asm_807FD888 runs from 0 to 31 (covered) and back,
// and is 0 again only once it has opened (func_global_asm_80704484). The
// top screen's HUD is under it; the touch screen's waits for it.
static s32 hud_in_transition(void) {
    return D_global_asm_807FD888 != 0.0f;
}

// In the adventure, and what the melons' draw tests too: the game shows its HUD.
static s32 hud_game_shows(void) {
    return gameIsInAdventureMode() && !(global_properties_bitfield & 3) && !func_global_asm_80714464()
        && current_map != MAP_MAIN_MENU && !hud_in_transition();
}

// The kong the counters up were made for.
static s32 hud_kong = -1;

static s32 hud_item_up(s32 HUDItemIndex) {
    return D_global_asm_80754280->hud_item[HUDItemIndex].hud_state != 0;
}

static s32 hud_count(s32 HUDItemIndex) {
    u16 *count = D_global_asm_80754280->hud_item[HUDItemIndex].actual_count_pointer;
    return count != NULL ? *count : 0;
}

// Whether the item is one of the level's counters for this kong.
static s32 hud_wanted(s32 HUDItemIndex) {
    u8 *kong = D_global_asm_807FC950 + func_global_asm_806C8DE0(0) * CHARACTER_PROGRESS_SIZE;
    s32 gun = kong[KONG_WEAPON] & 1;
    s32 homing = gun && (kong[KONG_WEAPON] & 2) && hud_count(HUD_HOMING_AMMO) > 0;

    switch (HUDItemIndex) {
        case HUD_COLOURED_BANANAS:
        case HUD_COINS:
        case HUD_GOLDEN_BANANAS:
            return TRUE;
        case HUD_AMMO:
            return gun && !homing;
        case HUD_HOMING_AMMO:
            return homing;
        case HUD_ORANGES:
        case HUD_CRYSTALS:
        case HUD_FILM:
            return hud_count(HUDItemIndex) > 0;
        case HUD_INSTRUMENT:
            return (kong[KONG_INSTRUMENT] & 1) != 0;
        default:
            return FALSE;
    }
}

// Whether another counter holds the item's place: a race's or minigame's,
// or the other kind of ammo.
static s32 hud_place_taken(s32 HUDItemIndex) {
    switch (HUDItemIndex) {
        case HUD_COLOURED_BANANAS:
            return hud_item_up(HUD_ALT_BANANAS);
        case HUD_COINS:
            return hud_item_up(HUD_RACE_COINS) || hud_item_up(HUD_ALT_COINS);
        case HUD_AMMO:
            return hud_wanted(HUD_HOMING_AMMO);
        case HUD_HOMING_AMMO:
            return hud_wanted(HUD_AMMO);
        default:
            return FALSE;
    }
}

static void hud_keep_up(s32 HUDItemIndex) {
    HUDDisplay *item = &D_global_asm_80754280->hud_item[HUDItemIndex];

    if (hud_place_taken(HUDItemIndex)) {
        // Make room: out, as the game would take it.
        if (item->hud_state == 1 || item->hud_state == 2) {
            item->hud_state = 3;
        }
        return;
    }
    switch (item->hud_state) {
        case 0:
            if (hud_wanted(HUDItemIndex)) {
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
    if (recomp_get_hud_on_bottom() && hud_in_transition()) {
        // Down while the screen is covered, as on the kong's change: back
        // up, coming in, once it has opened.
        for (HUDItemIndex = 0; HUDItemIndex < 0xF; HUDItemIndex++) {
            if (D_global_asm_80754280->hud_item[HUDItemIndex].hud_state != 0) {
                func_global_asm_806F9518(HUDItemIndex);
            }
        }
    }
    if (keep && func_global_asm_806C8DE0(0) != hud_kong) {
        // Another kong: what is up goes (the Golden Bananas' kong heads
        // with it, as nothing places them now) and comes back next frame
        // with this kong's icons.
        hud_kong = func_global_asm_806C8DE0(0);
        for (HUDItemIndex = 0; HUDItemIndex < 0xF; HUDItemIndex++) {
            func_global_asm_806F9518(HUDItemIndex);
        }
        keep = FALSE;
    }
    gDPPipeSync(dl++);
    gSPMatrix(dl++, &D_2000180, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    for (HUDItemIndex = 0; HUDItemIndex < 0xF; HUDItemIndex++) {
        HUDDisplay *item = &D_global_asm_80754280->hud_item[HUDItemIndex];
        if (keep) {
            hud_keep_up(HUDItemIndex);
        }
        // Each item a span of its own, placed by its N64 place.
        gEX3DSBottomScreen(dl++, HUD_BOTTOM_IS_RIGHT_COLUMN(item->screen_x, item->screen_y) ? HUD_BOTTOM_RIGHT_SHIFT : 0,
                           HUD_BOTTOM_ITEM_DY(HUDItemIndex));
        switch (item->hud_state) {
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
        gEX3DSTopScreen(dl++);
    }
    return dl;
}

// The melons' own update draws them as they slide in and out; on the touch
// screen that update still runs (it applies the damage and the healing) but
// into this list, which nothing draws, and the melons are drawn at their
// full place instead. At most 3 melons of two images each: under 200
// commands.
static Gfx hud_melons_discarded[384];

// @recomp 3DS: the health melons, marked for the touch screen (at its top
// right) and always up there.
RECOMP_PATCH Gfx *func_global_asm_80709344(Gfx *dl) {
    s16 i;
    s32 single = cc_number_of_players == 1;
    s32 always = single && recomp_get_hud_on_bottom() && gameIsInAdventureMode();

    if (single) {
        gEX3DSBottomScreen(dl++, HUD_BOTTOM_MELONS_SHIFT, HUD_BOTTOM_TOP_ROW_DY);
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

// The main menu's leaves, still, behind the touch screen's HUD. The menu
// background (func_global_asm_80706F90, patches_framebuffer.c) loads its 8
// tiles (32x64 RGBA16, TABLE_25) and hands them here, copied; the adventure
// always starts from that menu. The host makes them the touch screen's
// picture (recomp_set_hud_leaves: the menu's 128x128 block of tiles, 4
// columns of 32 and two rows of 64, flipped as the menu draws them, repeated
// over the 320x240, not scrolling), so play draws nothing for them.
#define HUD_LEAF_TILES 8
#define HUD_LEAF_TILE_WORDS (32 * 64 * 2 / 4)
static u32 hud_leaves[HUD_LEAF_TILES][HUD_LEAF_TILE_WORDS] __attribute__((aligned(8)));
static s32 hud_leaves_kept = FALSE;
static s32 hud_leaves_sent = FALSE;     // the host's picture is of these

void hud_3ds_keep_leaves(void **tiles) {
    s32 i, w;
    // Each frame the menu draws them, not only the first: the last copy is
    // of tiles the menu has visibly drawn.
    for (i = 0; i < HUD_LEAF_TILES; i++) {
        if (tiles[i] == NULL) {
            return;
        }
    }
    for (i = 0; i < HUD_LEAF_TILES; i++) {
        const u32 *src = (const u32 *)tiles[i];
        for (w = 0; w < HUD_LEAF_TILE_WORDS; w++) {
            if (hud_leaves[i][w] != src[w]) {
                hud_leaves[i][w] = src[w];
                hud_leaves_sent = FALSE;
            }
        }
    }
    hud_leaves_kept = TRUE;
}

// At the frame's start: the leaves behind everything the HUD marks there,
// in the frames where the HUD shows on the touch screen (rt64-3ds draws
// the picture in the frames whose list asks for it, black in the rest).
Gfx *hud_3ds_background(Gfx *dl) {
    if (!hud_leaves_kept || cc_number_of_players >= 2 || !recomp_get_hud_on_bottom() || !hud_game_shows()) {
        return dl;
    }
    if (!hud_leaves_sent) {
        recomp_set_hud_leaves(hud_leaves);
        hud_leaves_sent = TRUE;
    }
    gEX3DSBottomBackground(dl++);
    return dl;
}

extern Gfx *func_global_asm_80706F90(Gfx *dl);
extern Gfx *func_global_asm_80707980(Gfx *dl, f32 arg1, f32 arg2, Mtx *arg3, s16 arg4);
extern s32 D_global_asm_807552EC;

// @recomp 3DS: each frame's backdrop (the world's first draw, from the
// frame's world builder func_global_asm_8062C29C), with the touch screen's
// leaves ahead of it so they lie under everything the HUD marks there. The
// rest is the decomp's.
RECOMP_PATCH Gfx *func_global_asm_8070835C(Gfx *dl, s16 arg1) {
    s32 var_a1;
    s32 var_v0;
    s32 new_var;
    CharacterChange *cc = &character_change_array[arg1];

    dl = hud_3ds_background(dl);
    if ((cc_number_of_players > 1) || (global_properties_bitfield & 0x10) == 0) {
        if (current_map == MAP_MAIN_MENU) {
            dl = func_global_asm_80706F90(dl);
        } else {
            new_var = (global_properties_bitfield & 2) == 0;
            var_a1 = new_var;
            if (cc_number_of_players > 1) {
                var_v0 = current_map == MAP_KONG_BATTLE_ARENA_2 ? FALSE : TRUE;
                var_v0 |= (D_global_asm_807552EC > 1);
                var_a1 |= var_v0;
            }
            if (!var_a1) {
                return dl;
            }
            gDPPipeSync(dl++);
            gDPSetRenderMode(dl++, G_RM_NOOP, G_RM_NOOP2);
            gDPSetFillColor(dl++, 0x00010001);
            gDPSetCycleType(dl++, G_CYC_FILL);
            gDPFillRectangle(dl++, cc->unk270[0], cc->unk270[1], cc->unk270[2] - 1, cc->unk270[3] - 1);
            gDPPipeSync(dl++);
            gDPSetCycleType(dl++, G_CYC_1CYCLE);
        }
    } else {
        dl = func_global_asm_80707980(dl, cc->unk2D4, cc->unk2D8, (Mtx *)cc->pad108 + D_global_asm_807444FC, arg1);
    }
    return dl;
}
#endif
