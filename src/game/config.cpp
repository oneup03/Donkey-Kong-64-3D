#include "donk_config.h"
#include "recompui/recompui.h"
#include "recompui/config.h"
#include "recompui/renderer.h"
#include "recompinput/recompinput.h"
#include <algorithm>
#include <cmath>
#include "donk_sound.h"
#include "donk_draw.h"
#include "donk_support.h"
#include "ultramodern/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/config.hpp"
#include "util/file.h"
#include <filesystem>
#include <fstream>
#include <iomanip>

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#elif defined(__APPLE__)
#include "apple/rt64_apple.h"
#endif

static void add_general_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

    static EnumOptionVector camera_invert_mode_options = {
        {dk64::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {dk64::CameraInvertMode::InvertX, "InvertX", "Invert X"},
    };
    config.add_enum_option(
        dk64::configkeys::general::third_person_camera_invert_mode,
        "Invert Camera",
        "Inverts the camera controls for the third person camera if it's enabled.",
        camera_invert_mode_options,
        dk64::CameraInvertMode::InvertNone
    );
    static EnumOptionVector first_person_invert_mode_options = {
        {dk64::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {dk64::CameraInvertMode::InvertX, "InvertX", "Invert X"},
        {dk64::CameraInvertMode::InvertY, "InvertY", "Invert Y"},
        {dk64::CameraInvertMode::InvertBoth, "InvertBoth", "Invert Both"}
    };
    config.add_enum_option(
        dk64::configkeys::general::first_person_invert_mode,
        "Invert First Person View",
        "Inverts the camera controls in first person view. <recomp-color primary>Invert Y</recomp-color> is the default and matches the original game.",
        first_person_invert_mode_options,
        dk64::CameraInvertMode::InvertY
    );
    static EnumOptionVector swimming_invert_options = {
        {dk64::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {dk64::CameraInvertMode::InvertX, "InvertX", "Invert X"},
        {dk64::CameraInvertMode::InvertY, "InvertY", "Invert Y"},
        {dk64::CameraInvertMode::InvertBoth, "InvertBoth", "Invert Both"}
    };
    config.add_enum_option(
        dk64::configkeys::general::swimming_invert_mode,
        "Invert Swimming",
        "Inverts the controls for swimming. <recomp-color primary>Invert Y</recomp-color> is the default and matches the original game.",
        swimming_invert_options,
        dk64::CameraInvertMode::InvertY
    );
    // Story Skip
    static EnumOptionVector story_skip_options = {
        {dk64::StorySkipMode::Off, "Off", "None"},
        {dk64::StorySkipMode::IntroStory, "IntroStory", "Intro Story Only"},
        {dk64::StorySkipMode::VanillaOn, "VanillaOn", "On"}
    };
    config.add_enum_option(
        dk64::configkeys::general::story_skip,
        "Story Skip",
        "Skips some story cutscenes.<br /><recomp-color primary>Off</recomp-color>: No story cutscenes are skipped.<br /><recomp-color primary>Intro Story Only</recomp-color>: Only the 5-minute introductory story is skipped. The K. Rool level intros will still play.<br /><recomp-color primary>On</recomp-color>: Both the introductory story and all K. Rool level intros are skipped.",
        story_skip_options,
        dk64::StorySkipMode::Off
    );
    // Camera Type
    static EnumOptionVector camera_type_options = {
        {dk64::CameraTypeMode::Free, "Free", "Free Cam"},
        {dk64::CameraTypeMode::Follow, "Follow", "Follow Cam"},
        {dk64::CameraTypeMode::BetterFree, "BetterFree", "Better Free Cam"},
        {dk64::CameraTypeMode::Analog, "Analog", "Analog Camera"}
    };
    config.add_enum_option(
        dk64::configkeys::general::camera_type,
        "Camera Type",
        "Changes the camera behavior.<br /><recomp-color primary>Free Cam</recomp-color>: Camera can be controlled via pressing C-Left and C-Right. The camera does not try to push itself behind the player.<br /><recomp-color primary>Follow Cam</recomp-color>: Similar to <recomp-color secondary>Free Cam</recomp-color>, but the camera tries to push itself behind the player.<br /><recomp-color primary>Better Free Cam</recomp-color>: Similar to <recomp-color secondary>Free Cam</recomp-color>, but instead of a button press turning the camera 45 degrees, holding the button moves the camera at 5 degrees per frame.<br /><recomp-color primary>Analog Camera</recomp-color>: Control of the camera is controlled by your right analogue stick. It is advised that C-Up is remapped to something outside your right analogue stick in order to have a good time with this.",
        camera_type_options,
        dk64::CameraTypeMode::Free
    );
    config.add_number_option(
        dk64::configkeys::general::analog_camera_sensitivity,
        "Analog Camera Sensitivity",
        "Sets the sensitivity of the right stick analog camera, if enabled.",
        1, 10, 1, 0, false, 3
    );
    config.add_option_hidden_dependency(
        dk64::configkeys::general::analog_camera_sensitivity,
        dk64::configkeys::general::camera_type,
        dk64::CameraTypeMode::Free,
        dk64::CameraTypeMode::Follow,
        dk64::CameraTypeMode::BetterFree
    );
    // Lightning Flashes
    static EnumOptionVector lightning_flash_options = {
        {dk64::LightningFlashMode::Off, "Off", "Off"},
        {dk64::LightningFlashMode::Reduced, "Reduced", "Reduced"},
        {dk64::LightningFlashMode::Vanilla, "Vanilla", "Vanilla"}
    };
    config.add_enum_option(
        dk64::configkeys::general::lightning_flashes,
        "Lightning Flash Intensity",
        "Changes the intensity of lightning flashes within the game.<br /><recomp-color primary>Vanilla</recomp-color>: Lightning flashes at 100% intensity<br /><recomp-color primary>Reduced</recomp-color>: Lightning flashes at 60% intensity<br /><recomp-color primary>Off</recomp-color>: Lightning flashes are completely disabled",
        lightning_flash_options,
        // Off by default in the 3D build: the effect is a full-screen white
        // flash with no geometry behind it, so in stereo it reads as the screen
        // itself lurching rather than as lightning.
        dk64::LightningFlashMode::Off
    );
    
}

template <typename T = uint32_t>
T get_general_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_general_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_general_config().get_option_value(option_id)));
}

dk64::CameraInvertMode dk64::get_camera_invert_mode() {
    return get_general_config_enum_value<dk64::CameraInvertMode>(dk64::configkeys::general::camera_invert_mode);
}

dk64::CameraInvertMode dk64::get_third_person_camera_mode() {
    return get_general_config_enum_value<dk64::CameraInvertMode>(dk64::configkeys::general::third_person_camera_invert_mode);
}

dk64::CameraInvertMode dk64::get_swimming_invert_mode() {
    return get_general_config_enum_value<dk64::CameraInvertMode>(dk64::configkeys::general::swimming_invert_mode);
}

dk64::StorySkipMode dk64::get_story_skip() {
    return get_general_config_enum_value<dk64::StorySkipMode>(dk64::configkeys::general::story_skip);
}

dk64::CameraTypeMode dk64::get_camera_type() {
    return get_general_config_enum_value<dk64::CameraTypeMode>(dk64::configkeys::general::camera_type);
}

dk64::LightningFlashMode dk64::get_lightning_flash() {
    return get_general_config_enum_value<dk64::LightningFlashMode>(dk64::configkeys::general::lightning_flashes);
}

dk64::CameraInvertMode dk64::get_first_person_invert_mode() {
    return get_general_config_enum_value<dk64::CameraInvertMode>(dk64::configkeys::general::first_person_invert_mode);
}

uint32_t dk64::get_analog_cam_sensitivity() {
    return get_general_config_number_value(dk64::configkeys::general::analog_camera_sensitivity);
}

template <typename T = uint32_t>
T get_graphics_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_graphics_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_technical_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_config("technical").get_option_value(option_id)));
}

static void add_sound_options(recomp::config::Config &config) {
    config.add_percent_number_option(
        dk64::configkeys::sound::bgm_volume,
        "Background Music Volume",
        "Controls the overall volume of background music.",
        100.0f
    );
    config.add_percent_number_option(
        dk64::configkeys::sound::sfx_volume,
        "SFX Volume",
        "Controls the overall volume of sound effects.",
        100.0f
    );
}
template <typename T = uint32_t>
T get_sound_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_sound_config().get_option_value(option_id)));
}

int dk64::get_bgm_volume() {
    return get_sound_config_number_value<int>(dk64::configkeys::sound::bgm_volume);
}

int dk64::get_sfx_volume() {
    return get_sound_config_number_value<int>(dk64::configkeys::sound::sfx_volume);
}

static RT64::UserConfiguration::StereoMode dk64_to_rt64_stereo(dk64::StereoMode mode) {
    switch (mode) {
        case dk64::StereoMode::SideBySide:       return RT64::UserConfiguration::StereoMode::SideBySide;
        case dk64::StereoMode::TopAndBottom:     return RT64::UserConfiguration::StereoMode::TopAndBottom;
        case dk64::StereoMode::RowInterlaced:    return RT64::UserConfiguration::StereoMode::RowInterlaced;
        case dk64::StereoMode::ColumnInterlaced: return RT64::UserConfiguration::StereoMode::ColumnInterlaced;
        case dk64::StereoMode::Checkerboard:     return RT64::UserConfiguration::StereoMode::Checkerboard;
        case dk64::StereoMode::Anaglyph:         return RT64::UserConfiguration::StereoMode::Anaglyph;
        case dk64::StereoMode::LeiaSR:           return RT64::UserConfiguration::StereoMode::LeiaSR;
        default:                                 return RT64::UserConfiguration::StereoMode::Off;
    }
}

static void push_stereo_config_to_renderer() {
    // Read the temp values so slider drags push to the renderer immediately,
    // before the user hits Apply. For tabs that don't require confirmation,
    // get_temp_option_value falls back to the committed value, so this is
    // safe for one-shot reads at startup too.
    auto &graphics_config = recompui::config::get_graphics_config();
    auto get_num = [&graphics_config](const std::string &id) {
        return std::get<double>(graphics_config.get_temp_option_value(id));
    };

    auto mode = static_cast<dk64::StereoMode>(std::get<uint32_t>(
        graphics_config.get_temp_option_value(dk64::configkeys::graphics::stereo_mode)));
    uint32_t separation = static_cast<uint32_t>(get_num(dk64::configkeys::graphics::stereo_separation));
    // Convergence is the one slider with sub-unit steps, so it crosses the
    // renderer bridge in tenths rather than whole units. Everything downstream
    // of set_stereo_config works in those tenths.
    uint32_t convergence = static_cast<uint32_t>(std::lround(
        get_num(dk64::configkeys::graphics::stereo_convergence) * 10.0));
    uint32_t hud_depth = static_cast<uint32_t>(get_num(dk64::configkeys::graphics::stereo_hud_depth));
    bool auto_conv = std::get<bool>(
        graphics_config.get_temp_option_value(dk64::configkeys::graphics::stereo_auto_convergence));
    // Signed on the slider, but the renderer bridge packs it into 8 unsigned
    // bits, so it travels biased by +50 and is un-biased on the far side.
    const int32_t comfort_signed = static_cast<int32_t>(std::lround(get_num(dk64::configkeys::graphics::stereo_comfort_target)));
    uint32_t comfort_target = static_cast<uint32_t>(std::clamp(comfort_signed, -50, 60) + 50);
    uint32_t ghost_contrast = static_cast<uint32_t>(get_num(dk64::configkeys::graphics::stereo_ghost_contrast));
    uint32_t ghost_black_floor = static_cast<uint32_t>(get_num(dk64::configkeys::graphics::stereo_ghost_black_floor));

    recompui::renderer::set_stereo_config(dk64_to_rt64_stereo(mode), separation, convergence, hud_depth,
        auto_conv, comfort_target, ghost_contrast, ghost_black_floor);
}

static void add_stereo_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    namespace keys = dk64::configkeys::graphics;

    static EnumOptionVector stereo_mode_options = {
        {dk64::StereoMode::Off, "Off", "Off"},
        {dk64::StereoMode::SideBySide, "SideBySide", "SbS"},
        {dk64::StereoMode::TopAndBottom, "TopAndBottom", "TaB"},
        {dk64::StereoMode::RowInterlaced, "RowInterlaced", "RowInt"},
        {dk64::StereoMode::ColumnInterlaced, "ColumnInterlaced", "ColInt"},
        {dk64::StereoMode::Checkerboard, "Checkerboard", "Checker"},
        {dk64::StereoMode::Anaglyph, "Anaglyph", "Anaglyph"},
        {dk64::StereoMode::LeiaSR, "LeiaSR", "LeiaSR"},
    };
    config.add_enum_option(
        keys::stereo_mode,
        "Stereoscopic 3D",
        "Renders the world from two viewpoints and packs them into a single frame for a 3D display. "
        "<recomp-color primary>SbS</recomp-color> and <recomp-color primary>TaB</recomp-color> suit 3D TVs and headsets, "
        "<recomp-color primary>RowInt</recomp-color> / <recomp-color primary>ColInt</recomp-color> / <recomp-color primary>Checker</recomp-color> suit passive and shutter displays, "
        "and <recomp-color primary>Anaglyph</recomp-color> works with red/cyan glasses on any screen.<br />"
        "<recomp-color primary>LeiaSR</recomp-color> drives a Leia autostereoscopic display via lenticular weaving; "
        "requires the SR Platform service and the <recomp-color primary>D3D12</recomp-color> graphics API."
        "<br /><br />Stereo rendering roughly doubles GPU cost, and is not compatible with MSAA.",
        stereo_mode_options,
        dk64::StereoMode::Off
    );
    config.add_number_option(
        keys::stereo_separation,
        "Stereo Separation",
        "How far apart the two eyes are, i.e. how strong the 3D effect is. Each point is 0.2% of the screen width, "
        "so the default 10 is a comfortable 2% and the maximum 50 is 10% — about the point where the eyes would have "
        "to diverge to fuse the image.",
        0, 50, 1, 0, false, 25
    );
    config.add_number_option(
        keys::stereo_convergence,
        "Stereo Convergence",
        "The distance at which the world sits exactly on the screen plane. Objects nearer than this pop out of the "
        "screen, objects further away sit behind it. Lower it if close-up geometry is uncomfortable.",
        0.1, 20, 0.1, 1, false, 6
    );
    config.add_number_option(
        keys::stereo_hud_depth,
        "HUD Depth",
        "Where the HUD sits in depth. <recomp-color primary>50</recomp-color> is the screen plane, higher pops it "
        "towards you, lower pushes it behind the screen.",
        0, 100, 1, 0, false, 50
    );
    config.add_bool_option(
        keys::stereo_auto_convergence,
        "Auto Convergence",
        "Automatically reduces convergence during menus, cutscenes and minigames, where the gameplay-tuned value "
        "reads as too strong.",
        true,
        false
    );
    config.add_number_option(
        keys::stereo_comfort_target,
        "Comfort Target",
        "How far the nearest thing on screen may pop out before "
        "<recomp-color primary>Auto Convergence</recomp-color> pulls the screen plane in, in tenths of a percent of "
        "screen width. <recomp-color primary>0</recomp-color> puts the screen plane right on the nearest object; "
        "<recomp-color primary>negative</recomp-color> values push it in front of that, so the whole scene sits "
        "behind the screen with no pop-out at all. Cutscenes and menus tighten this automatically.",
        -20, 30, 1, 0, false, 0
    );
    config.add_number_option(
        keys::stereo_ghost_contrast,
        "Ghost Reduction: Contrast",
        "Reduces cross-talk (a faint copy of one eye's image bleeding into the other) by squeezing contrast towards "
        "mid-grey. <recomp-color primary>100</recomp-color> is off. Lower values reduce ghosting at the cost of "
        "overall contrast.",
        50, 100, 1, 0, false, 100
    );
    config.add_number_option(
        keys::stereo_ghost_black_floor,
        "Ghost Reduction: Black Floor",
        "Reduces cross-talk by raising the black level. <recomp-color primary>0</recomp-color> is off. Only helps on "
        "displays that cancel cross-talk themselves (autostereoscopic panels in particular), where it gives the "
        "correction room to work without clipping. Blacks go grey quickly above 5.",
        0, 20, 1, 0, false, 0
    );
    // Everything below the mode dropdown is meaningless when stereo is off.
    for (const auto &dependent : {keys::stereo_separation, keys::stereo_convergence, keys::stereo_hud_depth,
                                  keys::stereo_auto_convergence, keys::stereo_ghost_contrast,
                                  keys::stereo_ghost_black_floor}) {
        config.add_option_hidden_dependency(dependent, keys::stereo_mode, dk64::StereoMode::Off);
    }

    // ConfigOptionDependency stores ONE value-list per dependent and overwrites
    // it on every add_option_hidden_dependency call, so the naive pattern of
    // two separate calls only ends up honoring the last one. Build a single
    // combined value-list ({stereo_mode == Off, auto_conv == false}) and
    // register it under both source options — each source's value comparison
    // will only match the entry of its own variant type, so the right
    // condition triggers on the right change.
    {
        std::vector<recomp::config::ConfigValueVariant> hide_when_scale = {
            static_cast<uint32_t>(dk64::StereoMode::Off),
            false
        };
        config.add_option_hidden_dependency(keys::stereo_comfort_target, keys::stereo_mode, hide_when_scale);
        config.add_option_hidden_dependency(keys::stereo_comfort_target, keys::stereo_auto_convergence, hide_when_scale);
    }

    auto stereo_change_callback = [](recomp::config::ConfigValueVariant, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext) {
        push_stereo_config_to_renderer();
    };
    for (const auto &id : {keys::stereo_mode, keys::stereo_separation, keys::stereo_convergence,
                           keys::stereo_hud_depth, keys::stereo_auto_convergence,
                           keys::stereo_comfort_target, keys::stereo_ghost_contrast,
                           keys::stereo_ghost_black_floor}) {
        config.add_option_change_callback(id, stereo_change_callback);
    }

    // LeiaSR weaving only works on the D3D12 backend (the weaver SDK is DX12-
    // only). We skip the OptionChangeContext::Load path because calling
    // update_enum_option_disabled during config load, before the UI is set up,
    // crashes the app at startup.
    config.add_option_change_callback(recompui::config::graphics::options::api_option,
        [&config](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext change_context) {
            if (change_context == recomp::config::OptionChangeContext::Load) {
                return;
            }
            const auto api = static_cast<ultramodern::renderer::GraphicsApi>(std::get<uint32_t>(cur_value));
            const bool dx12_capable = (api == ultramodern::renderer::GraphicsApi::Auto) ||
                                      (api == ultramodern::renderer::GraphicsApi::D3D12);
            config.update_enum_option_disabled(
                dk64::configkeys::graphics::stereo_mode,
                static_cast<uint32_t>(dk64::StereoMode::LeiaSR),
                !dx12_capable
            );
        }
    );
}

static void add_graphics_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    // Cutscene borders
    static EnumOptionVector cutscene_border_options = {
        {dk64::CutsceneBordersMode::Off, "Off", "Off"},
        {dk64::CutsceneBordersMode::On, "On", "On"}
    };
    config.add_enum_option(
        dk64::configkeys::graphics::cutscene_borders,
        "Cutscene Borders",
        "If turned on, cutscenes will show borders on the top and the bottom of the screen as in the vanilla game.",
        cutscene_border_options,
        dk64::CutsceneBordersMode::Off
    );
    config.add_percent_number_option(
        dk64::configkeys::graphics::draw_distance,
        "Draw Distance",
        "Controls the draw distance within the game from 0% (vanilla) to 100% (10x vanilla). Some objects do not get a draw distance increase for technical reasons.",
        100.0f
    );
    add_stereo_options(config);
}

dk64::CutsceneBordersMode dk64::get_cutscene_borders() {
    return get_graphics_config_enum_value<dk64::CutsceneBordersMode>(dk64::configkeys::graphics::cutscene_borders);
}

int dk64::get_draw_distance() {
    return get_graphics_config_number_value<int>(dk64::configkeys::graphics::draw_distance);
}

dk64::StereoMode dk64::get_stereo_mode() {
    return get_graphics_config_enum_value<dk64::StereoMode>(dk64::configkeys::graphics::stereo_mode);
}

uint32_t dk64::get_stereo_separation() {
    return get_graphics_config_number_value<uint32_t>(dk64::configkeys::graphics::stereo_separation);
}

double dk64::get_stereo_convergence() {
    return get_graphics_config_number_value<double>(dk64::configkeys::graphics::stereo_convergence);
}

uint32_t dk64::get_stereo_hud_depth() {
    return get_graphics_config_number_value<uint32_t>(dk64::configkeys::graphics::stereo_hud_depth);
}

bool dk64::get_stereo_auto_convergence() {
    return std::get<bool>(recompui::config::get_graphics_config().get_option_value(dk64::configkeys::graphics::stereo_auto_convergence));
}

uint32_t dk64::get_stereo_comfort_target() {
    return get_graphics_config_number_value<uint32_t>(dk64::configkeys::graphics::stereo_comfort_target);
}

uint32_t dk64::get_stereo_ghost_contrast() {
    return get_graphics_config_number_value<uint32_t>(dk64::configkeys::graphics::stereo_ghost_contrast);
}

uint32_t dk64::get_stereo_ghost_black_floor() {
    return get_graphics_config_number_value<uint32_t>(dk64::configkeys::graphics::stereo_ghost_black_floor);
}

static void add_technical_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    // Multiplayer
    static EnumOptionVector multiplayer_options = {
        {dk64::MultiplayerEnabled::Off, "Off", "Off"},
        {dk64::MultiplayerEnabled::On, "On", "On"}
    };
    config.add_enum_option(
        dk64::configkeys::technical::multiplayer_enabled,
        "Enable Multiplayer",
        "Enables the ability to enter Multiplayer. Multiplayer is considered a <recomp-color secondary>Work-in-Progress feature</recomp-color> that is classed as non-functional.<br />You should not enable this feature unless you wish to use glitches such as \"Funky Weapons Glitch\" or \"Main Menu Moves\"",
        multiplayer_options,
        dk64::MultiplayerEnabled::Off
    );
}

dk64::MultiplayerEnabled dk64::get_multiplayer_enabled() {
    return get_technical_config_enum_value<dk64::MultiplayerEnabled>(dk64::configkeys::technical::multiplayer_enabled);
}

static void set_control_defaults() {
    using namespace recompinput;

    // Left shoulder -> C Down | Backwards eggs / zoom out
    set_default_mapping_for_controller(
        GameInput::C_DOWN,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY, true),
            InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
        }
    );

    // Right shoulder -> C Up | Forwards eggs / first person
    set_default_mapping_for_controller(
        GameInput::C_UP,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY, false),
            InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
        }
    );

    // North button -> C Left | Talon trot / camera left
    set_default_mapping_for_controller(
        GameInput::C_LEFT,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX, false),
            InputField::controller_digital(SDL_CONTROLLER_BUTTON_NORTH)
        }
    );

    // East button -> C Right | Wonderwing / camera right
    set_default_mapping_for_controller(
        GameInput::C_RIGHT,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX, true),
            InputField::controller_digital(SDL_CONTROLLER_BUTTON_EAST)
        }
    );

    // R3 -> L | Unused in BK but can be used in mods
    set_default_mapping_for_controller(GameInput::L, { InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK) });
}

static void set_control_descriptions() {
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_POS, "Used to move and for steering while swimming. Axis inversion for swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_NEG, "Used to move and for steering while swimming. Axis inversion for swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_NEG, "Used to move and for steering while swimming. Axis inversion for swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_POS, "Used to move and for steering while swimming. Axis inversion for swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::A, "Used to jump and select options in menus.");
    recompinput::set_game_input_description(recompinput::GameInput::B, "Used for attacks, which change depending on whether you are stationary, moving, in the air, or crouching.");
    recompinput::set_game_input_description(recompinput::GameInput::Z, "Used to crouch, which enables A, B and the C-Buttons to perform different actions.");
    recompinput::set_game_input_description(recompinput::GameInput::L, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::R, "Used to center the camera behind the player, and to perform tighter turns while in the coconut boat.");
    recompinput::set_game_input_description(recompinput::GameInput::START, "Used for pausing and for skipping certain cutscenes.");
    recompinput::set_game_input_description(recompinput::GameInput::C_UP, "Used to enter first-person mode and to play your instrument while holding Z");
    recompinput::set_game_input_description(recompinput::GameInput::C_DOWN, "Used to toggle between the different camera zoom levels, and to pull out the fairy camera while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::C_LEFT, "Used to rotate the camera sideways. Axis inversion can be configured in the General tab. Also used to pull out your fruit weapon while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::C_RIGHT, "Used to rotate the camera sideways. Axis inversion can be configured in the General tab). Also used to throw an orange while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_UP, "Used to move in Jetpac and Donkey Kong arcade games. Mods may also use it separately for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_DOWN, "Used to move in Jetpac and Donkey Kong arcade games. Mods may also use it separately for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_LEFT, "Used to move in Jetpac and Donkey Kong arcade games. Mods may also use it separately for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_RIGHT, "Used to move in Jetpac and Donkey Kong arcade games. Mods may also use it separately for additional features.");
}

void dk64::init_config() {
    std::filesystem::path recomp_dir = recompui::file::get_app_folder_path();

    if (!recomp_dir.empty()) {
        std::filesystem::create_directories(recomp_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = true;
    general_options.has_mouse_sensitivity = true;

    auto &general_config = recompui::config::create_general_tab(general_options);
    add_general_options(general_config);

    auto &graphics_config = recompui::config::create_graphics_tab();
    add_graphics_options(graphics_config);

    set_control_defaults();
    set_control_descriptions();
    recompui::config::create_controls_tab();

    auto &sound_config = recompui::config::create_sound_tab();
    add_sound_options(sound_config);

    auto &technical_config = recompui::config::create_config_tab("Technical", "technical", true);
    add_technical_options(technical_config);

    recompui::config::create_mods_tab();

    recompui::config::finalize();

    // Seed the renderer with the stereo config that was just loaded so the first
    // frame already has the user's saved Stereo Mode / Separation / Convergence
    // applied, rather than waiting for the first option change.
    push_stereo_config_to_renderer();
}
