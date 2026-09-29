// Donkey Kong 64: Recompiled, Nintendo 3DS entry point.
#include <cinttypes>
#include <cstdio>
#include <string>

extern "C" {
#include <3ds/types.h>
#include <3ds/services/hid.h>
}

#include "recomp3ds.h"
#include "ovl_patches.hpp"
#include "donk_game.h"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultra64.h"

extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();
extern RspUcodeFunc n_aspMain;
// recomp_api_3ds.cpp
extern int g_dk64_camera_type, g_dk64_analog_cam_sensitivity, g_dk64_camera_invert, g_dk64_aim_invert,
           g_dk64_swim_invert, g_dk64_gyro_aim, g_dk64_story_skip, g_dk64_lightning, g_dk64_draw_distance,
           g_dk64_music_volume, g_dk64_sfx_volume, g_dk64_tag_anywhere,
           g_dk64_no_company_coins, g_dk64_autocomplete_bonuses, g_dk64_easier_beetle,
           g_dk64_fixed_beaver_bother, g_dk64_slow_shoe, g_dk64_slow_dk_phase;

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
// With Tag Anywhere on, D-pad left and right are the N64 D-pad, which tags
// backward and forward through the kongs; C-Left and C-Right stay on ZL/ZR
// and the C-Stick.
const ButtonMap kButtonsTag[] = {
    { KEY_A, N64_A }, { KEY_B, N64_B }, { KEY_L, N64_Z }, { KEY_R, N64_R }, { KEY_START, N64_START },
    { KEY_X | KEY_DUP, N64_CUP }, { KEY_Y | KEY_DDOWN, N64_CDOWN },
    { KEY_ZL, N64_CLEFT }, { KEY_ZR, N64_CRIGHT },
    { KEY_DLEFT, N64_DLEFT }, { KEY_DRIGHT, N64_DRIGHT },
};
void tag_anywhere_changed(int on) {
    if (on) input_set_map(kButtonsTag, sizeof(kButtonsTag) / sizeof(kButtonsTag[0]));
    else input_set_map(kButtons, sizeof(kButtons) / sizeof(kButtons[0]));
}

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
// The desktop's options, on the 3DS menu's pages (defaults as on the
// desktop, except the camera, Better Free here, and gyro aim, on here).
const MenuOption kOptions[] = {
    { "aim_invert", "Aim invert", 0, 3, &g_dk64_aim_invert, kInvertNames, nullptr, PageControls },
    { "swim_invert", "Swim invert", 0, 3, &g_dk64_swim_invert, kInvertNames, nullptr, PageControls },
    { "gyro_aim", "Gyro aim", 0, 500, &g_dk64_gyro_aim, nullptr, gyro_changed, PageControls, 25, "%", "Off" },
    { "camera_type", "Camera", 0, 3, &g_dk64_camera_type, kCameraNames, camera_changed, PageControls },
    { "analog_cam_speed", "Analog speed", 1, 10, &g_dk64_analog_cam_sensitivity, nullptr, nullptr, PageControls },
    { "camera_invert", "Camera invert", 0, 3, &g_dk64_camera_invert, kInvertNames, nullptr, PageControls },
    { "story_skip", "Story skip", 0, 2, &g_dk64_story_skip, kStorySkipNames, nullptr, PageGame },
    { "lightning", "Lightning", 0, 2, &g_dk64_lightning, kLightningNames, nullptr, PageGame },
    { "draw_distance", "Draw distance", 0, 100, &g_dk64_draw_distance, nullptr, nullptr, PageGame, 10, "%", "Vanilla" },
    { "music_volume", "Music volume", 0, 100, &g_dk64_music_volume, nullptr, nullptr, PageGame, 5, "%" },
    { "sfx_volume", "SFX volume", 0, 100, &g_dk64_sfx_volume, nullptr, nullptr, PageGame, 5, "%" },
    // The Mods page: the mods in lib/mods, built into the patches.
    // DK64TagAnywhereRecomp: tag from anywhere with D-pad left/right.
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
};

// DK64's defaults for the library's rows; settings.ini, once written, keeps
// what the player chose.
const MenuDefault kDefaults[] = {
    { "separation", 50 },               // 3D depth
    { "convergence_tenths", 100 },      // 10.0
    { "stereo_ghost_contrast", 80 },    // percent
    { "stick_deadzone", 15 },           // percent
    { "cstick_deadzone", 20 },          // percent
    { "cstick_up", 0 },                 // first person from X (C-Up) only
};

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
    desc.render.hud_on_bottom_supported = true;
    desc.render.depth_to_rdram = true;      // the camera's wall avoidance samples the depth buffer
    desc.render.depthless_world_is_hud = true;   // the fairy-camera film card: HUD depth
    desc.audio_hle = true;      // n_aspMain interpreted on the CPU (see rt64-3ds naudio_hle.cpp)
    desc.button_map = kButtons;
    desc.button_map_count = sizeof(kButtons) / sizeof(kButtons[0]);
    desc.menu_options = kOptions;
    desc.menu_option_count = sizeof(kOptions) / sizeof(kOptions[0]);
    desc.menu_defaults = kDefaults;
    desc.menu_default_count = sizeof(kDefaults) / sizeof(kDefaults[0]);
#ifdef DK64_NULL_RENDERER
    desc.null_renderer = true;
#endif

    return recomp3ds::run(desc);
}
