// Donkey Kong 64: Recompiled, Nintendo 3DS entry point.
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include <3ds/types.h>
#include <3ds/services/apt.h>
#include <3ds/services/hid.h>
}

#include "recomp3ds.h"
#include "rt64_3ds.h"
#include "ovl_patches.hpp"
#include "donk_game.h"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultra64.h"

extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();
extern RspUcodeFunc n_aspMain;
// recomp_api_3ds.cpp
extern int g_dk64_camera_type, g_dk64_analog_cam_sensitivity, g_dk64_camera_invert, g_dk64_aim_invert,
           g_dk64_swim_invert, g_dk64_gyro_aim, g_dk64_story_skip, g_dk64_lightning, g_dk64_draw_distance, g_dk64_particles,
           g_dk64_music_volume, g_dk64_sfx_volume, g_dk64_tag_anywhere,
           g_dk64_no_company_coins, g_dk64_autocomplete_bonuses, g_dk64_easier_beetle,
           g_dk64_fixed_beaver_bother, g_dk64_slow_shoe, g_dk64_slow_dk_phase, g_dk64_infinite_items, g_dk64_chunk_cull_off, g_dk64_low_models, g_dk64_language,
           g_dk64_in_adventure;

// Tag Anywhere reads the N64 D-pad's left and right (tag backward and
// forward through the kongs): while it is on they are touch-screen
// buttons, in the adventure (recomp_set_in_adventure), where it tags. A
// player who wants them on buttons maps them on the Controls page (D-pad
// left and right to D-Left and D-Right, say).
void dk64_set_tag_buttons() {
    recomp3ds::touch_pad_set(g_dk64_tag_anywhere != 0 && g_dk64_in_adventure != 0
                             ? (uint16_t)(recomp3ds::N64_DLEFT | recomp3ds::N64_DRIGHT) : 0);
}

namespace {

// A=A  B=B  L=Z  R=R  START=START; the C buttons on X (up), Y (down),
// ZL (left), ZR (right), the D-pad and the C-Stick. DK64 has no use for the
// N64 D-pad or L.
using namespace recomp3ds;
const ButtonMap kButtons[] = {
    { KEY_A, N64_A }, { KEY_B, N64_B }, { KEY_L, N64_Z }, { KEY_R, N64_R }, { KEY_START, N64_START },
    { KEY_X | KEY_DUP, N64_CUP }, { KEY_Y | KEY_DDOWN, N64_CDOWN },
    { KEY_ZL | KEY_DLEFT, N64_CLEFT }, { KEY_ZR | KEY_DRIGHT, N64_CRIGHT },
};
void tag_anywhere_changed(int) { dk64_set_tag_buttons(); }

const char* const kCameraNames[] = { "Free", "Follow", "Better Free", "Analog" };
void camera_changed(int type) {
    // The analog camera reads the C-Stick itself; its C buttons would turn
    // pushing it up into first person.
    input_set_cstick_buttons(type != 3);
}
void gyro_changed(int percent) { input_set_gyro(percent > 0); }
const char* const kInvertNames[] = { "None", "X", "Y", "Both" };
const char* const kStorySkipNames[] = { "Off", "Intro only", "On" };
const char* const kLightningNames[] = { "Off", "Reduced", "Vanilla" };
const char* const kOffOnNames[] = { "Off", "On" };
const char* const kParticleNames[] = { "Full", "Half", "Quarter" };
const char* const kModelNames[] = { "Full", "Low" };
const char* const kLanguageNames[] = { "English", "Francais", "Deutsch", "Espanol", "Portugues BR" };
// The desktop's options, on the 3DS menu's pages (defaults as on the
// desktop, except the camera, Better Free here, and gyro aim, on here).
// Not const: an Old 3DS's Camera row stops at Better Free (main).
MenuOption kOptions[] = {
    { "aim_invert", "Aim invert", 0, 3, &g_dk64_aim_invert, kInvertNames, nullptr, PageControls },
    { "swim_invert", "Swim invert", 0, 3, &g_dk64_swim_invert, kInvertNames, nullptr, PageControls },
    { "gyro_aim", "Gyro aim", 0, 500, &g_dk64_gyro_aim, nullptr, gyro_changed, PageControls, 25, "%", "Off" },
    { "camera_type", "Camera", 0, 3, &g_dk64_camera_type, kCameraNames, camera_changed, PageControls },
    // The Analog camera's, which reads the C-Stick: a New 3DS's row only.
    { "analog_cam_speed", "Analog speed", 1, 10, &g_dk64_analog_cam_sensitivity, nullptr, nullptr, PageControls, 1, nullptr, nullptr, true },
    { "camera_invert", "Camera invert", 0, 3, &g_dk64_camera_invert, kInvertNames, nullptr, PageControls },
    { "story_skip", "Story skip", 0, 2, &g_dk64_story_skip, kStorySkipNames, nullptr, PageGame },
    { "lightning", "Lightning", 0, 2, &g_dk64_lightning, kLightningNames, nullptr, PageGame },
    { "draw_distance", "Draw distance", 0, 100, &g_dk64_draw_distance, nullptr, nullptr, PageGame, 10, "%", "Vanilla" },
    // Bursts of particle sprites thinned to one in 1, 2 or 4
    // (patches/particles_3ds.c): each is a draw of its own, which an Old 3DS
    // can't afford by the hundred.
    { "particles", "Particles", 0, 2, &g_dk64_particles, kParticleNames, nullptr, PageGame },
    // Low: the Kongs drawn with the models the game uses for two
    // or more players, the same skeleton with a third to a half less
    // geometry, and the game's own far-model swap for actors (the blue
    // beaver's) back on (patches_interpolation.c).
    { "models", "Models", 0, 1, &g_dk64_low_models, kModelNames, nullptr, PageGame },
    { "music_volume", "Music volume", 0, 100, &g_dk64_music_volume, nullptr, nullptr, PageGame, 5, "%" },
    { "sfx_volume", "SFX volume", 0, 100, &g_dk64_sfx_volume, nullptr, nullptr, PageGame, 5, "%" },
    // settings.ini only: the game's chunk cull against the frustum widened to the screen (patches_main.c) off, an A/B.
    { "dbg_chunk_cull_off", "Chunk cull off (dbg)", 0, 1, &g_dk64_chunk_cull_off, nullptr, nullptr, PageCount },
    // The Mods page: the mods in lib/mods, built into the patches.
    // DK64TagAnywhereRecomp: tag from anywhere with D-pad left/right.
    // The translations built in (lib/mods; patches/languages.c), one at a
    // time: the game's text and fonts change at its init, so at restart.
    { "language", "Language (at restart)", 0, 4, &g_dk64_language, kLanguageNames, nullptr, PageMods },
    { "tag_anywhere", "Tag Anywhere", 0, 1, &g_dk64_tag_anywhere, kOffOnNames, tag_anywhere_changed, PageMods },
    // theballaam96's:
    // RecompNoCompanyCoins: Helm's coin door opens without the Nintendo and
    // Rareware coins (it sets the door's flag in the save at file start).
    { "mod_no_company_coins", "No coin door", 0, 1, &g_dk64_no_company_coins, kOffOnNames, nullptr, PageMods },
    // RecompAutocompleteBonuses: a bonus barrel gives its reward at once.
    { "mod_autocomplete_bonuses", "Auto bonuses", 0, 1, &g_dk64_autocomplete_bonuses, kOffOnNames, nullptr, PageMods },
    // RecompEasierBeetle: slower beetles in the races.
    { "mod_easier_beetle", "Easy beetle", 0, 1, &g_dk64_easier_beetle, kOffOnNames, nullptr, PageMods },
    // RecompFixedBeaverBother: gold beavers, a longer scare, noclip into the hole.
    { "mod_fixed_beaver_bother", "Fixed beavers", 0, 1, &g_dk64_fixed_beaver_bother, kOffOnNames, nullptr, PageMods },
    // RecompSlowShoe: K. Rool's toe sequence slower (from the next map load).
    { "mod_slow_shoe", "Slow shoe", 0, 1, &g_dk64_slow_shoe, kOffOnNames, nullptr, PageMods },
    // RecompSlowDKPhase: a wider window in K. Rool's DK phase (next map load).
    { "mod_slow_dk_phase", "Slow DK phase", 0, 1, &g_dk64_slow_dk_phase, kOffOnNames, nullptr, PageMods },
    // patches/infinite_consumables.c: ammo, oranges, crystals, film and
    // instrument energy kept full once the player has them.
    { "mod_infinite_items", "Infinite items", 0, 1, &g_dk64_infinite_items, kOffOnNames, nullptr, PageMods },
};

// DK64's defaults for the library's rows; settings.ini, once written, keeps
// what the player chose.
const MenuDefault kDefaults[] = {
    { "separation", 50 },               // 3D depth
    { "convergence_tenths", 100 },      // 10.0
    { "stereo_ghost_contrast", 100 },   // percent: 100 with the 0% floor is off (a full-screen pass an eye less)
    { "frame_cap", 2 },                 // 60 interp. (old3ds_frame_cap on an Old 3DS)
    { "stick_deadzone", 15 },           // percent
    { "cstick_deadzone", 20 },          // percent
    { "cstick_up", 0 },                 // first person from X (C-Up) only
    { "hud_on_bottom", 1 },             // the HUD on the touch screen (the Game page's HUD row; Top puts it back as on the N64)
    // A quarter of each particle burst on an Old 3DS (the Particles row).
    { "particles", 2, MenuDefault::Old3dsOnly },
    // The Kongs' multiplayer models and far actors' low models (the Models row).
    { "models", 1, MenuDefault::Old3dsOnly },
    // The Audio row's High priority on an Old 3DS too (the library's Old 3DS
    // default is Low, which never interrupts the game's code for the audio
    // thread): at the Old clock DK64's game thread holds its core at 85-100%
    // and without the preemption its audio thread handed over as few as
    // 12,700 of 22,050 samples a second, the music slowed and its notes came
    // up to 2.4 s late (rt64-3ds docs/porting-a-game.md, Preemption). High
    // preempts only while the DSP holds under 25 ms of audio.
    { "audio_mode", 2, MenuDefault::Old3dsOnly },
};

// The crosshairs the renderer finds by their texture (RenderDesc::
// aim_texture_hashes) and places by the 3D page's Crosshair row, as it does
// the first-person quad: TABLE_14_TEXTURES_HUD 0x38, the 64x64 IA8 ring of
// the guns' and the porthole's sights, and 0x83..0x8E, the 12 frames of the
// four arrows that close in on the bonus games' crosshair (Kremling Kosh,
// Teetering Turtle Trouble and Searchlight Seek among them;
// func_bonus_80026690, func_bonus_8002CC08), which moves across the screen. Hashed over the
// texels as RDRAM holds them (each word byte-swapped), from the ROM's table
// and checked against captures (dl_test DL_AIM_HASH). The race arrows are
// frames of the same texture: patches/aim_3ds.c pauses the match while they
// are up.
const uint64_t dk64_aim_textures[] = {
    0x36938954c1501037ull,      // 0x38
    0xcf27966394b60d3eull, 0x908ce37a60fdcfe7ull, 0x87abcc565cec9e70ull, 0x838b6b9417e310fbull,   // 0x83..0x86
    0x967fd965b09d22ebull, 0xbdab58b0805bc592ull, 0x9e02ca775ebc6a76ull, 0xb3903fb9294bd6b3ull,   // 0x87..0x8A
    0xca66348c80486e23ull, 0xe69bf72771369c33ull, 0x84a67a986b9b98ecull, 0xe9a1f8f6172d649dull,   // 0x8B..0x8E
};
// Their shapes: only textures of these are hashed (G_IM_FMT_IA, G_IM_SIZ_8b;
// G_IM_FMT_RGBA, G_IM_SIZ_16b).
const rt64_3ds::TexShape dk64_aim_shapes[] = { { 64, 64, 3, 1 }, { 32, 16, 0, 2 } };

RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    switch (task->t.type) {
        case M_AUDTASK:
            return n_aspMain;
        default:
            fprintf(stderr, "Unknown RSP task: %" PRIu32 "\n", task->t.type);
            return nullptr;
    }
}

std::string get_game_thread_name(const OSThread* t) {
    std::string name = "[Game] ";
    switch (t->id) {
        case 0:  name += (t->priority == 150) ? "PIMGR" : (t->priority == 254) ? "VIMGR" : std::to_string(t->id); break;
        case 1:  name += "INIT"; break;
        case 2:  name += "NO_EXP_PAK"; break;
        case 3:  name += "MAIN"; break;
        case 4:  name += "AUDIO"; break;
        case 5:  name += "GFX_DEBUG"; break;
        case 8:  name += "CPU_DEBUG"; break;
        case 9:  name += "EEPROM"; break;
        case 11: name += "IDLE"; break;
        default: name += std::to_string(t->id); break;
    }
    return name;
}

// Projection-group ids the DK64 patches use (patches/common_structs.h) and
// the owner's stereo classification of them (rt64-3D DK64-3D branch,
// rt64_projection_processor.cpp).
constexpr uint32_t MTXTAG_FRAMEBUFFERTRANSITION = 2;
constexpr uint32_t MTXTAG_SKYBOXBLEND = 4;
constexpr uint32_t MTXTAG_CAMERAPROJECTION = 5;
constexpr uint32_t MTXTAG_PROJ_AT_INFINITY = 0x700;
constexpr uint32_t MTXTAG_PROJ_WEATHER = 0x710;

const rt64_3ds::StereoRule dk64_stereo_rules[] = {
    { MTXTAG_PROJ_AT_INFINITY,      MTXTAG_PROJ_AT_INFINITY,      rt64_3ds::ProjKind::Any,         rt64_3ds::StereoClass::Infinity },
    { MTXTAG_SKYBOXBLEND,           MTXTAG_SKYBOXBLEND,           rt64_3ds::ProjKind::Perspective, rt64_3ds::StereoClass::World },
    { MTXTAG_SKYBOXBLEND,           MTXTAG_SKYBOXBLEND,           rt64_3ds::ProjKind::Ortho,       rt64_3ds::StereoClass::Infinity },
    { MTXTAG_CAMERAPROJECTION,      MTXTAG_CAMERAPROJECTION,      rt64_3ds::ProjKind::Perspective, rt64_3ds::StereoClass::World },
    { MTXTAG_FRAMEBUFFERTRANSITION, MTXTAG_FRAMEBUFFERTRANSITION, rt64_3ds::ProjKind::Ortho,       rt64_3ds::StereoClass::ScreenOverlay },
    { MTXTAG_PROJ_WEATHER,          MTXTAG_PROJ_WEATHER,          rt64_3ds::ProjKind::Ortho,       rt64_3ds::StereoClass::ScreenOverlay },
};

}   // namespace

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    recomp3ds::GameDesc desc{};
    desc.game_id = u8"DK64";
    desc.sd_dir = "DK64";
    desc.entry = recomp::GameEntry{
        .rom_hash = 0x4d876060f09b3fc5ULL,
        .internal_name = "DONKEY KONG 64",
        .display_name = "Donkey Kong 64",
        .game_id = u8"DK64",
        .mod_game_id = "",                 // no mods on the 3DS
        .discovery_url = "",
        .save_type = recomp::SaveType::Eep16k,
        .thumbnail_bytes = {},
        .is_enabled = false,
        .decompression_routine = nullptr,
        .has_compressed_code = true,
        .entrypoint_address = get_entrypoint_address(),
        .entrypoint = recomp_entrypoint,
        .on_init_callback = dk64::dk_on_init,
    };
    desc.rsp.get_rsp_microcode = get_rsp_microcode;
    desc.register_overlays = dk64::register_bk_overlays;
    desc.register_patches = dk64::register_bk_patches;
    desc.get_game_thread_name = get_game_thread_name;

    desc.render.game_name = "Donkey Kong 64";
    desc.render.rules = dk64_stereo_rules;
    desc.render.rule_count = sizeof(dk64_stereo_rules) / sizeof(dk64_stereo_rules[0]);
    desc.render.unclassified_persp = rt64_3ds::StereoClass::None;
    desc.render.unclassified_ortho = rt64_3ds::StereoClass::Hud;
    desc.render.frame_head_proj_id = MTXTAG_CAMERAPROJECTION;
    desc.render.hud_id_lo = 0x600;
    desc.render.hud_id_hi = 0x6FF;
    desc.render.bubble_id_lo = 0x680;
    desc.render.vi_width = 320;
    desc.render.vi_height = 240;
    // The HUD on the touch screen (patches/hud_3ds.c marks it), the Game
    // page's HUD row choosing; the counters keep their N64 places there,
    // the melons along the top.
    desc.render.hud_on_bottom_supported = true;
    // Behind it, the main menu's leaves: their picture made once the menu
    // has drawn them (recomp_set_hud_leaves), shown in the frames whose
    // HUD is there (gEX3DSBottomBackground, patches/hud_3ds.c).
    desc.render.hud_bottom_background_at_run_time = true;
    desc.render.depth_to_rdram = true;      // the camera's wall avoidance samples the depth buffer
    desc.render.depthless_world_is_hud = true;   // the fairy-camera film card: HUD depth
    // The patches tag the camera, actors and bones for RT64's high frame
    // rate (patches_matrix.c, patches_interpolation.c): offer "60 interp." on
    // the Frame rate row, an interpolated frame between each two of the
    // game's while it runs at 30. The in-betweens draw into eye targets of
    // their own (double_eyes, 1125 KB of VRAM), which also keeps the depth
    // the camera reads back the game frame's; without them no list could
    // have an in-between. rt64-3ds turns them on by default on a New 3DS
    // only, as an Old 3DS starts at the game's rate (old3ds_frame_cap):
    // "60 interp." there also takes gpu_double_eyes=1 in settings.ini.
    desc.render.frame_interpolation = true;
    desc.render.double_eyes = true;
    // The game waits for each list to be drawn before its next frame: it
    // hears that once the last frame's depth is in RDRAM (for the camera),
    // not after the renderer's in-betweens and waits for the screen, which
    // in heavy scenes made it miss retraces (50 ms frames).
    desc.render.early_drawn = true;
    // The GPU's share of a frame, as Banjo and Goemon have it: 16-bit
    // eye depth, as fine as the N64's own out to ~600 units, which is what
    // the wall avoidance reads, and 750 KB more VRAM for textures; I/IA
    // textures as LA8, the same texels at half the size; and the leading
    // full-screen fill as the clear, which the interiors' frames have.
    desc.render.eye_depth16 = true;
    desc.render.intensity_textures16 = true;
    desc.render.fill_clear = true;
    // The game's own level of detail (gSPBranchLessZ's far models at a
    // distance), as the N64 draws it and as Banjo keeps it: the desktop
    // frontends force the near model everywhere, which the log showed as
    // every one of ~900 branches a window taken.
    desc.render.force_lod_branch = false;
    // The camera's depth buffer written every other game frame on an Old
    // 3DS: the fill of the 320x240 buffer costs the renderer's core several
    // ms a frame there, and the camera's and the sun's tests read a depth a
    // frame old meanwhile.
    desc.render.depth_to_rdram_every_old3ds = 2;
    desc.render.aim_texture_hashes = dk64_aim_textures;
    desc.render.aim_texture_count = sizeof(dk64_aim_textures) / sizeof(dk64_aim_textures[0]);
    desc.render.aim_texture_shapes = dk64_aim_shapes;
    desc.render.aim_texture_shape_count = sizeof(dk64_aim_shapes) / sizeof(dk64_aim_shapes[0]);
    desc.audio_hle = true;      // n_aspMain interpreted on the CPU (see rt64-3ds naudio_hle.cpp)
    // The Audio engine row's DSP mode (rt64-3ds dsp_synth.h): the voices
    // play on DSP channels, as us.toml's n_alSyn* hooks report them, and the
    // audio task and its command list are skipped. n_syn (the synthesizer's
    // pointer) at 0x807563E4, the stock n_audio layout; StartVoiceParams has
    // three more arguments before its ramp time (sp+36); the effect has
    // DK64's own layout, and this microcode leaves it unfiltered; the sound
    // modes (surround, mono, headphones) at 0x807FF0F0 move pan.
    desc.synth = recomp3ds::SynthFlavor::NAudio;
    desc.synth.synth_global = 0x807563E4;
    desc.synth.start_t_stack = 36;
    desc.synth.fx = recomp3ds::SynthFx::Dk64;
    desc.synth.fx_lowpass = false;
    desc.synth.sound_modes = 0x807FF0F0;
    // With two cores (an Old 3DS) the game stays on core 1 and the renderer,
    // audio and synth take core 0 (two_cores_game_on_core0 left false, the
    // Cores row's "2, game on 1"): the cores' loads at the Old clock favoured
    // it, unlike Banjo's.
    desc.button_map = kButtons;
    desc.button_map_count = sizeof(kButtons) / sizeof(kButtons[0]);
    // An Old 3DS has no C-Stick: no Analog camera there (a saved one
    // becomes Better Free).
    bool new_3ds = false;
    APT_CheckNew3DS(&new_3ds);
    if (!new_3ds) {
        for (MenuOption& o : kOptions) {
            if (strcmp(o.key, "camera_type") == 0) o.hi = 2;
        }
    }
    desc.menu_options = kOptions;
    desc.menu_option_count = sizeof(kOptions) / sizeof(kOptions[0]);
    desc.menu_defaults = kDefaults;
    desc.menu_default_count = sizeof(kDefaults) / sizeof(kDefaults[0]);
    desc.old3ds_frame_cap = 0;          // the game's own rate on an Old 3DS
    // The touch buttons (Tag Anywhere's pair and the player's two) while the
    // HUD is on the touch screen: x 200..303, y 64..183. patches/hud_3ds.c
    // moves the right-hand counters left (icons at x 162, numbers from x
    // ~96) and the melons to the top right (x ~200..301, to y 61), and the bottom row
    // starts at y 188.
    desc.touch_area_hud = { 25, 8, 13, 15 };
    // The ROM: in memory on a New 3DS. On an Old 3DS the CIA asks for the
    // 80 MB mode (CMakeLists.txt; 96 MB crashed the HOME Menu), whose heap
    // (about 56 MB) holds the 32 MB ROM beside 12 MB of RDRAM and the rest:
    // the heap's peak in play was 53.4 MB with the ROM and 16 MB of RDRAM,
    // so about 5.3 MB of rest, and 8 MB is set for room. A .3dsx in the
    // Homebrew Launcher's 64 MB (a heap of about 39 MB) streams it.
    desc.rom_new3ds = recomp3ds::GameDesc::RomPlace::Memory;
    desc.rom_old3ds = recomp3ds::GameDesc::RomPlace::Auto;
    desc.rom_room_mb = 8;
    // Streamed, it is read in 128 KB blocks
    // with 16 MB of them: the card costs an Old 3DS about 100 ms a 128 KB
    // read (70 ms for 32 KB). Walking around the Isles, a 4 MB cache read
    // 139 MB in five minutes, the game waiting on the card a third of the
    // time; 12 MB cut that to 13%. The heap peaked at 31 MB of 40 MB with
    // 12 MB, the cache the only part that grew.
    desc.rom_cache_mb = 16;
    desc.rom_block_kb = 128;
#ifdef DK64_NULL_RENDERER
    desc.null_renderer = true;
#endif

    return recomp3ds::run(desc);
}
