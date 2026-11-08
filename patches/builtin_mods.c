// Mods built into the patches for builds without the mod loader (the 3DS):
// patches/Makefile compiles lib/mods' sources when BUILTIN_MODS_DIR is set,
// which also defines DK64_BUILTIN_MODS. The game's events are then ordinary
// functions (patches.h) and this file defines each one: it calls the
// callbacks of the built-in mods that are switched on
// (recomp_get_builtin_mods; RECOMP_CALLBACK is empty in
// platform/3ds/mods/modding.h) and otherwise does nothing, as the 3DS did
// before. A mod switched off leaves the game vanilla.
#ifdef DK64_BUILTIN_MODS

#include "common_structs.h"
#include "patch_helpers.h"
#include "options.h"
#include "builtin_mods.h"
#ifdef DK64_LANGUAGES
#include "languages.h"
#endif

// The mods' callbacks, by the event they subscribe to.
void tag_anywhere(void);                                    // DK64TagAnywhereRecomp: every frame
void removeChunkyBunch(void);                               //   init; edits the table below
void open_coin_door(void);                                  // RecompNoCompanyCoins: file start
void overwrite_coin_door_checks(s16 *flag, u8 *flag_type);  //   flag check
void autocompleteLoop(void);                                // RecompAutocompleteBonuses: every frame
void beetle_slowdown_main(void);                            // RecompEasierBeetle: every frame
void changeShoeTimings(void);                               // RecompSlowShoe: map load
void changeWindowTimings(void);                             // RecompSlowDKPhase: map load
void infinite_items_every_frame(void);                      // infinite_consumables.c: every frame
void infinite_items_file_start(void);                       //   file start

// Which mods are on: read from the host at the events that start
// something (init, every frame, map load, file start), not at the
// frequent ones (flag checks).
static u32 mods_on = 0;

static void refresh(void) {
    mods_on = recomp_get_builtin_mods();
}

int builtin_mod_on(unsigned int bit) {
    return (mods_on & bit) != 0;
}

int tag_anywhere_enabled(void) {
    return builtin_mod_on(BUILTIN_MOD_TAG_ANYWHERE);
}

// RecompAutocompleteBonuses reads its two options (the Oh Banana jingle, an
// explosion); both are Disabled by default, option 0.
unsigned long recomp_get_config_u32(const char *key) {
    return 0;
}

// Tag Anywhere's init callback removes Chunky's banana bunches from a table
// (they would be unreachable without tagging): applied when the mod is
// switched on, and the saved entries put back when it is switched off.
typedef struct {
    u16 unk0;
    s8 unk2;
    u8 unk3;
    s32 unk4;
    s32 unk8;
    s16 unkC;
    s16 unkE;
    s16 unk10;
    s16 unk12;
} Struct80753EFC;

extern Struct80753EFC D_global_asm_80753EF0[];

static s32 ta_applied = 0;
static s16 ta_saved[42];

static void tag_anywhere_follow_setting(void) {
    const s32 on = builtin_mod_on(BUILTIN_MOD_TAG_ANYWHERE);
    s32 i;

    if (on == ta_applied) return;
    if (on) {
        for (i = 0; i < 42; i++) ta_saved[i] = D_global_asm_80753EF0[i].unkE;
        removeChunkyBunch();
    } else {
        for (i = 0; i < 42; i++) D_global_asm_80753EF0[i].unkE = ta_saved[i];
    }
    ta_applied = on;
}

// The events (declared where the patches trigger them).

void recomp_on_init(void) {
    refresh();
#ifdef DK64_LANGUAGES
    language_on_init();
#endif
}

extern int gameIsInAdventureMode(void);
extern Maps current_map;

// What the host was last told of the adventure (recomp_set_in_adventure).
static s32 in_adventure = -1;
// ... and of the arcade games (recomp_set_arcade_game).
static s32 in_arcade = -1;

void dk64recomp_every_frame(void) {
    const s32 adventure = gameIsInAdventureMode() != 0;

    refresh();
    if (adventure != in_adventure) {
        recomp_set_in_adventure(adventure);
        in_adventure = adventure;
    }
    {
        const s32 arcade = current_map == MAP_DK_ARCADE || current_map == MAP_JETPAC;
        if (arcade != in_arcade) {
            recomp_set_arcade_game(arcade);
            in_arcade = arcade;
        }
    }
#ifdef DK64_LANGUAGES
    language_every_frame();
#endif
    tag_anywhere_follow_setting();
    if (builtin_mod_on(BUILTIN_MOD_TAG_ANYWHERE)) tag_anywhere();
    if (builtin_mod_on(BUILTIN_MOD_AUTOCOMPLETE_BONUS)) autocompleteLoop();
    if (builtin_mod_on(BUILTIN_MOD_EASIER_BEETLE)) beetle_slowdown_main();
    infinite_items_every_frame();
}

void recomp_on_map_load(void) {
    refresh();
    if (builtin_mod_on(BUILTIN_MOD_SLOW_SHOE)) changeShoeTimings();
    if (builtin_mod_on(BUILTIN_MOD_SLOW_DK_PHASE)) changeWindowTimings();
}

void recomp_on_file_start(void) {
    refresh();
    infinite_items_file_start();
    if (builtin_mod_on(BUILTIN_MOD_NO_COMPANY_COINS)) open_coin_door();
}

void recomp_on_flag_check(s16 *flag, u8 *flag_type) {
    if (builtin_mod_on(BUILTIN_MOD_NO_COMPANY_COINS)) overwrite_coin_door_checks(flag, flag_type);
}

// Events no built-in mod uses.
void recomp_on_eeprom_load(void) {}
void recomp_on_music_bin_load(s32 song, s32 bank, u8 *bin) {}
void recomp_on_cutscene_play(s16 *cutscene, u8 *cutscene_bitfield) {}
void recomp_on_autowalk(void) {}
#ifdef DK64_LANGUAGES
void recomp_on_asset_file_load_override(s32 tableIndex, s32 fileIndex, u8 **outputFile, s32 *file_size) {
    language_asset_override(tableIndex, fileIndex, outputFile, file_size);
}
#else
void recomp_on_asset_file_load_override(s32 tableIndex, s32 fileIndex, u8 **outputFile, s32 *file_size) {}
#endif
#ifdef DK64_LANGUAGES
void recomp_adjust_dl_allocation(s32 *allocation) { language_adjust_dl_allocation(allocation); }
#else
void recomp_adjust_dl_allocation(s32 *allocation) {}
#endif
void recomp_on_new_file_start(void) {}
void recomp_on_dirty_file_start(void) {}
void recomp_on_flag_change(s16 *flag, u8 *target_state, u8 *flag_type) {}

#endif
