/**
 * Stereoscopic 3D runtime scene classification.
 *
 * Once per frame this pushes a single bool to the host: does the current scene
 * want reduced stereo convergence? The gameplay camera is tuned for a
 * third-person view of Kong-sized geometry, and the same convergence reads as
 * far too strong on a menu, a full-screen cutscene, or a 2D minigame, where
 * everything sits close to the camera and pops uncomfortably out of the screen.
 *
 * The host multiplies the user's convergence by their "Auto Convergence Scale"
 * whenever this is set and the feature is enabled -- see
 * apply_pending_stereo_config() in RecompFrontend's rt64_render_context.cpp.
 *
 * Two rules govern how this file is written, both learned the hard way:
 *
 *  1. Hook a per-frame site that runs in EVERY game mode. The world draw path
 *     only runs during normal gameplay, so a signal driven from there would
 *     never update for exactly the scenes this feature exists to handle.
 *
 *  2. Read game state out of BSS directly rather than calling the game's
 *     accessors. DK64 pages code overlays in and out at runtime (menu,
 *     multiplayer, minecart, bonus, race, critter, boss, arcade, jetpac), and
 *     a call into an overlay that is not currently mapped crashes. Reading the
 *     underlying variables is always safe: before the game initializes them
 *     they are zero, which classifies as "no reduction", a harmless default.
 */

#include "common_structs.h"
#include "graphics.h"

// game_mode @ 0x80755318, values from the game_mode_e enum in the decomp.
extern u8 game_mode;

// is_cutscene_active @ 0x807444EC. Non-zero while a cutscene is playing;
// patches_interpolation.c treats 3 and 4 as the Arcade/Jetpac sub-modes.
extern u8 is_cutscene_active;

enum stereo_game_mode {
    STEREO_GAME_MODE_NINTENDO_LOGO = 0,
    STEREO_GAME_MODE_OPENING_CUTSCENE = 1,
    STEREO_GAME_MODE_DK_RAP = 2,
    STEREO_GAME_MODE_DK_TV = 3,
    STEREO_GAME_MODE_MAIN_MENU = 5,
    STEREO_GAME_MODE_ADVENTURE = 6,
    STEREO_GAME_MODE_GAME_OVER = 9,
    STEREO_GAME_MODE_END_SEQUENCE = 10,
    STEREO_GAME_MODE_DK_THEATRE = 11,
    STEREO_GAME_MODE_MYSTERY_MENU_MINIGAME = 12,
    STEREO_GAME_MODE_SNIDES_BONUS_GAME = 13,
    STEREO_GAME_MODE_END_SEQUENCE_DK_THEATRE = 14,
};

/**
 * Game modes that are wholly menu, cutscene or minigame, regardless of map.
 *
 * These are deliberately mode checks rather than a map-ID list. A mode is
 * state-level and mod-safe; a map ID is not, because a mod can repurpose an
 * unused map slot for real gameplay and would then silently get its
 * convergence pulled in.
 */
static s32 stereo_mode_wants_low_convergence(u8 mode) {
    switch (mode) {
        case STEREO_GAME_MODE_NINTENDO_LOGO:
        case STEREO_GAME_MODE_OPENING_CUTSCENE:
        case STEREO_GAME_MODE_DK_RAP:
        case STEREO_GAME_MODE_DK_TV:
        case STEREO_GAME_MODE_MAIN_MENU:
        case STEREO_GAME_MODE_GAME_OVER:
        case STEREO_GAME_MODE_END_SEQUENCE:
        case STEREO_GAME_MODE_DK_THEATRE:
        case STEREO_GAME_MODE_MYSTERY_MENU_MINIGAME:
        case STEREO_GAME_MODE_SNIDES_BONUS_GAME:
        case STEREO_GAME_MODE_END_SEQUENCE_DK_THEATRE:
            return 1;
        default:
            return 0;
    }
}

static s32 stereo_scene_wants_low_convergence(void) {
    if (stereo_mode_wants_low_convergence(game_mode)) {
        return 1;
    }

    // In-engine cutscenes during Adventure mode. These frame characters much
    // closer than gameplay does, so they get the same treatment. This reuses
    // the flag the interpolation patches already trust for the same kind of
    // decision, so the two stay consistent.
    if (is_cutscene_active != 0) {
        return 1;
    }

    return 0;
}

/**
 * First-person state.
 *
 * The renderer identifies the aiming reticle geometrically - an untagged,
 * roughly square, centred quad - because the game gives its tiles no transform
 * id to match on. That description also fits a pause-menu icon and a bananaport
 * transition tile, and shifting one of those to aim depth is what corrupts them.
 * Both of those happen inside Adventure mode, so the low-convergence
 * classification above does not separate them either.
 *
 * What does separate them is whether the first-person camera is actually live.
 * func_global_asm_806EA628 is the game's first-person control update and runs
 * only then, so it pulses the counter below (see sound_options_patches.c). The
 * counter is a hold rather than a latch on purpose: a pause or a bananaport
 * suspends those updates, the pulse stops arriving, and the flag lapses within
 * a couple of frames without needing to enumerate every way first person can
 * end. Two frames of tolerance keeps it from flickering if an update is ever
 * skipped, at the cost of the reticle staying depth-aware two frames into a
 * pause, which is not visible.
 */
#define STEREO_FIRST_PERSON_HOLD_FRAMES 2

static u8 stereo_first_person_frames = 0;

void stereo_note_first_person_frame(void) {
    stereo_first_person_frames = STEREO_FIRST_PERSON_HOLD_FRAMES;
}

static s32 stereo_first_person_active(void) {
    if (stereo_first_person_frames == 0) {
        return 0;
    }
    stereo_first_person_frames--;
    return 1;
}

/**
 * Called once per frame from the main game thread. See
 * patches/boot_logos_patches.c, next to the dk64recomp_every_frame() event.
 */
void stereo_runtime_tick(void) {
    recomp_stereo_set_low_convergence_scene(stereo_scene_wants_low_convergence());
    recomp_stereo_set_first_person(stereo_first_person_active());
}
