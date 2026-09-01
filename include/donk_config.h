#ifndef __DONK_CONFIG_H__
#define __DONK_CONFIG_H__

#include <filesystem>
#include <string>
#include <string_view>

#include "json/json.hpp"

namespace dk64 {
    inline const std::u8string program_id = u8"DK64Recompiled";
    inline const std::string program_name = "DK64: Rekongpiled";

    namespace configkeys {
        namespace general {
            inline const std::string camera_invert_mode = "camera_invert_mode";
            inline const std::string analog_cam_mode = "analog_cam_mode";
            inline const std::string third_person_camera_invert_mode = "third_person_camera_invert_mode";
            inline const std::string swimming_invert_mode = "swimming_invert_mode";
            inline const std::string first_person_invert_mode = "first_person_invert_mode";
            inline const std::string analog_camera_sensitivity = "analog_camera_sensitivity";
            inline const std::string story_skip = "story_skip";
            inline const std::string camera_type = "camera_type";
            inline const std::string lightning_flashes = "lightning_flashes";
        }

        namespace sound {
            inline const std::string bgm_volume = "bgm_volume";
            inline const std::string sfx_volume = "sfx_volume";
        }

        namespace graphics {
            inline const std::string cutscene_borders = "cutscene_borders";
            inline const std::string draw_distance = "draw_distance";
            inline const std::string stereo_mode = "stereo_mode";
            inline const std::string stereo_separation = "stereo_separation";
            inline const std::string stereo_convergence = "stereo_convergence";
            inline const std::string stereo_hud_depth = "stereo_hud_depth";
            inline const std::string stereo_auto_convergence = "stereo_auto_convergence";
            inline const std::string stereo_comfort_target = "stereo_comfort_target";
            inline const std::string stereo_ghost_contrast = "stereo_ghost_contrast";
            inline const std::string stereo_ghost_black_floor = "stereo_ghost_black_floor";
        }

        namespace technical {
            inline const std::string multiplayer_enabled = "multiplayer_enabled";
        }
    }

    // TODO: Move loading configs to the runtime once we have a way to allow per-project customization.
    void init_config();

    enum class CameraInvertMode {
        InvertNone,
        InvertX,
        InvertY,
        InvertBoth
    };

    CameraInvertMode get_camera_invert_mode();

    CameraInvertMode get_third_person_camera_mode();

    CameraInvertMode get_swimming_invert_mode();

    CameraInvertMode get_first_person_invert_mode();

    uint32_t get_analog_cam_sensitivity();

    enum class StorySkipMode {
        Off,
        IntroStory,
        VanillaOn
    };

    StorySkipMode get_story_skip();

    enum class CameraTypeMode {
        Free,
        Follow,
        BetterFree,
        Analog
    };

    CameraTypeMode get_camera_type();

    enum class LightningFlashMode {
        Vanilla,
        Reduced,
        Off
    };
    LightningFlashMode get_lightning_flash();

    enum class CutsceneBordersMode {
        On,
        Off
    };
    CutsceneBordersMode get_cutscene_borders();

    enum class MultiplayerEnabled {
        Off,
        On
    };
    MultiplayerEnabled get_multiplayer_enabled();

    // Mirrors RT64::UserConfiguration::StereoMode. Kept as a separate game-side
    // enum, converted explicitly in config.cpp, so a reorder on either side
    // can't silently misalign the two.
    enum class StereoMode {
        Off,
        SideBySide,
        TopAndBottom,
        RowInterlaced,
        ColumnInterlaced,
        Checkerboard,
        Anaglyph,
        LeiaSR,
        OptionCount
    };
    StereoMode get_stereo_mode();

    // Separation is the total background disparity as a fraction of screen
    // width: each slider point is 0.2%, so the default 10 is 2% and the maximum
    // 50 is 10% (roughly IPD over screen width, the divergence ceiling).
    uint32_t get_stereo_separation();
    // The distance at which geometry sits exactly on the screen plane. This is
    // the one slider with sub-unit steps (0.1..50); it crosses the renderer
    // bridge in tenths, converted once in push_stereo_config_to_renderer().
    double get_stereo_convergence();
    // 50 = screen plane, above = pop-out, below = pushed back.
    uint32_t get_stereo_hud_depth();
    // Depth-driven auto convergence: measures the nearest on-screen geometry and
    // pulls the screen plane in when it would pop out uncomfortably.
    bool get_stereo_auto_convergence();
    // Comfort budget for that loop. Lower tolerates less pop-out and so pulls in
    // more eagerly.
    uint32_t get_stereo_comfort_target();
    // Ghost reduction. Both are exact no-ops at their defaults: contrast 100
    // means no squeeze toward mid-grey, black floor 0 means no lift.
    uint32_t get_stereo_ghost_contrast();
    uint32_t get_stereo_ghost_black_floor();

    void open_quit_game_prompt();
};

#endif
