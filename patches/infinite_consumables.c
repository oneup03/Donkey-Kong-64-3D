// The Mods page's Infinite items (BUILTIN_MOD_INFINITE_ITEMS): the
// consumables never run out. Ammo, homing ammo, orange grenades, crystal
// coconuts, film and instrument energy are filled to what the Kong can carry
// (the game's own capacity, func_global_asm_806F8EDC) every frame, so what
// is spent comes straight back.
//
// Only items the player has: one is filled once its count has been above
// zero this file (the instrument's per Kong, its energy being each Kong's
// own), so nothing is handed out before the game would give it - a
// capacity is not ownership (the orange count has one from the start).
//
// The game's own states are left alone: an item it marks infinite or not
// available (HUDDisplay::unk_2c; the bonus barrel games, K. Rool, Helm's
// crystals, DK TV), multiplayer, and anything outside the adventure.
#ifdef DK64_BUILTIN_MODS

#include "common_structs.h"
#include "patches_ui.h"
#include "patches_main.h"
#include "patch_helpers.h"
#include "builtin_mods.h"

extern int gameIsInDKTVMode(void);
extern int gameIsInAdventureMode(void);
extern s32 func_global_asm_806C8DE0(s32 playerIndex);
extern u16 func_global_asm_806F8EDC(s32 HUDItemIndex, s32 playerIndex);

#define INF_FIRST_ITEM 2    // standard ammo; then homing ammo, oranges, crystals, film
#define INF_LAST_ITEM 7     // instrument energy
#define INF_KONGS 6         // five Kongs and Krusha

static u8 inf_owned[INF_LAST_ITEM + 1];
static u8 inf_instrument_owned[INF_KONGS];

void infinite_items_file_start(void) {
    s32 i;
    for (i = 0; i <= INF_LAST_ITEM; i++) inf_owned[i] = 0;
    for (i = 0; i < INF_KONGS; i++) inf_instrument_owned[i] = 0;
}

void infinite_items_every_frame(void) {
    PlayerHUD *hud;
    s32 i;
    s32 kong;

    if (!builtin_mod_on(BUILTIN_MOD_INFINITE_ITEMS)) return;
    if (cc_number_of_players >= 2 || gameIsInDKTVMode() || !gameIsInAdventureMode()) return;
    hud = D_global_asm_80754280;    // player 0's (func_global_asm_806F7FD0)
    if (hud == NULL) return;
    kong = func_global_asm_806C8DE0(0);
    for (i = INF_FIRST_ITEM; i <= INF_LAST_ITEM; i++) {
        HUDDisplay *item = &hud->hud_item[i];
        u8 *owned = &inf_owned[i];
        u16 max;
        if (item->actual_count_pointer == NULL || item->unk_2c != 0) continue;
        if (i == INF_LAST_ITEM) {
            if (kong < 0 || kong >= INF_KONGS) continue;
            owned = &inf_instrument_owned[kong];
        }
        if (*item->actual_count_pointer != 0) *owned = 1;
        if (!*owned) continue;
        max = func_global_asm_806F8EDC(i, 0);
        if (*item->actual_count_pointer < max) *item->actual_count_pointer = max;
    }
}

#endif
