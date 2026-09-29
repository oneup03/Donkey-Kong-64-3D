// The DK64-specific host functions (patches/syms.ld manual symbols) for the
// 3DS. The desktop versions in src/game/recomp_api.cpp read the launcher's
// configuration; here the values are fixed or come from the 3DS settings.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

#include "recomp.h"
#include "librecomp/helpers.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/addresses.hpp"
#include "librecomp/game.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/error_handling.hpp"
#include "xxHash/xxh3.h"
#include "recomp3ds.h"
#include "rt64_3ds.h"
#include "../../patches/builtin_mods.h"

namespace recomp3ds {
    void input_get_right_stick(float* x, float* y);   // input_hid.cpp
}

namespace {
bool g_right_analog_suppressed = false;

// Values the desktop reads from its configuration and the 3DS keeps fixed.
constexpr int   kCutsceneBorders = 0;   // Off: a 240-line screen has no room for bars

constexpr uint32_t k1_to_phys(uint32_t addr) { return addr & 0x1FFFFFFF; }
}

// The desktop's gameplay options, set from the 3DS settings menu
// (main_3ds.cpp), in the desktop's units unless noted.
// Camera: 0 Free (the original), 1 Follow, 2 Better Free (C-Left/C-Right
// keep turning while held), 3 Analog (the C-Stick).
int g_dk64_camera_type = 2;             // Better Free
int g_dk64_analog_cam_sensitivity = 3;  // 1..10
// Axis inversion: 0 none, 1 X, 2 Y, 3 both.
int g_dk64_camera_invert = 2;           // the camera (C-Left/C-Right and analog)
int g_dk64_aim_invert = 0;              // first person; the original game inverts Y
int g_dk64_swim_invert = 2;             // swimming; Y is the original game's
int g_dk64_gyro_aim = 200;              // percent of the console's own turn, 0 = off
int g_dk64_story_skip = 0;              // 0 off, 1 intro story only, 2 on
int g_dk64_lightning = 0;               // 0 off (the owner's 3D build), 1 reduced, 2 vanilla
int g_dk64_draw_distance = 0;           // percent: 0 = the game's own culling, 100 = ten times it
int g_dk64_music_volume = 100;          // percent
int g_dk64_sfx_volume = 100;            // percent
int g_dk64_tag_anywhere = 0;            // the mods built into the patches (patches/builtin_mods.c)
int g_dk64_no_company_coins = 0, g_dk64_autocomplete_bonuses = 0, g_dk64_easier_beetle = 0,
    g_dk64_fixed_beaver_bother = 0, g_dk64_slow_shoe = 0, g_dk64_slow_dk_phase = 0;   // the Mods page

extern "C" void recomp_update_inputs(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram; (void)ctx;
}

extern "C" void recomp_puts(uint8_t* rdram, recomp_context* ctx) {
    PTR(char) cur_str = _arg<0, PTR(char)>(rdram, ctx);
    u32 length = _arg<1, u32>(rdram, ctx);
    for (u32 i = 0; i < length; i++) {
        fputc(MEM_B(i, (gpr)cur_str), stderr);
    }
}

extern "C" void recomp_exit(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram; (void)ctx;
    ultramodern::quit();
}

extern "C" void recomp_error(uint8_t* rdram, recomp_context* ctx) {
    std::string str{};
    PTR(u8) str_ptr = _arg<0, PTR(u8)>(rdram, ctx);
    for (size_t i = 0; MEM_B(str_ptr, i) != '\x00'; i++) {
        str += (char)MEM_B(str_ptr, i);
    }
    ultramodern::error_handling::message_box(str.c_str());
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
}

// First-person aiming reads these as stick values in place of a centred
// stick (patches/sound_options_patches.c): the second output turns, the
// first (negated) pitches, one stick unit being 0.08 degrees for that game
// frame. At 100% the aim turns as far as the console did. The patch then
// applies the first-person inversion, meant for the stick; it is undone
// here so the gyro always follows the console.
extern "C" void recomp_get_gyro_deltas(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);
    *x_out = 0.0f; *y_out = 0.0f;
    if (g_dk64_gyro_aim <= 0) return;
    float right_deg, up_deg;
    recomp3ds::input_take_gyro(&right_deg, &up_deg);
    const float k = (float)g_dk64_gyro_aim / 100.0f / 0.08f;
    float turn = right_deg * k;         // positive: turn right, as the stick
    float up = up_deg * k;              // positive: look up
    // The patch turns `turn` into stick x, inverted with X; and `up` into
    // stick y = -up, negated again unless Y is inverted - where the
    // original game's (inverted) stick y > 0 looks down.
    if (g_dk64_aim_invert & 1) turn = -turn;
    if (!(g_dk64_aim_invert & 2)) up = -up;
    *x_out = up;
    *y_out = turn;
}

extern "C" void recomp_get_mouse_deltas(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);
    *x_out = 0.0f; *y_out = 0.0f;
}

extern "C" void recomp_powf(uint8_t* rdram, recomp_context* ctx) {
    float a = _arg<0, float>(rdram, ctx);
    float b = ctx->f14.fl;
    _return(ctx, std::pow(a, b));
}

extern "C" void recomp_get_target_framerate(uint8_t* rdram, recomp_context* ctx) {
    int frame_divisor = _arg<0, u32>(rdram, ctx);
    _return(ctx, ultramodern::get_target_framerate(60 / frame_divisor));
}

extern "C" void recomp_get_window_resolution(uint8_t* rdram, recomp_context* ctx) {
    gpr width_out = _arg<0, PTR(u32)>(rdram, ctx);
    gpr height_out = _arg<1, PTR(u32)>(rdram, ctx);
    MEM_W(0, width_out) = 400;
    MEM_W(0, height_out) = 240;
}

extern "C" void recomp_get_target_aspect_ratio(uint8_t* rdram, recomp_context* ctx) {
    float original = _arg<0, float>(rdram, ctx);
    _return(ctx, std::max(400.0f / 240.0f, original));
}

// The game's volume scale is 0..40; the desktop's percent / 2.5.
extern "C" void recomp_get_bgm_volume(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, (float)g_dk64_music_volume / 2.5f); }
extern "C" void recomp_get_sfx_volume(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, (float)g_dk64_sfx_volume / 2.5f); }
// A floor under the game's draw distances, in world units (percent x 50).
extern "C" void recomp_get_draw_distance(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, (float)g_dk64_draw_distance * 50.0f); }

extern "C" void recomp_stereo_set_low_convergence_scene(uint8_t* rdram, recomp_context* ctx) {
    rt64_3ds::set_low_convergence_scene(_arg<0, s32>(rdram, ctx) != 0);
}

extern "C" void recomp_stereo_set_first_person(uint8_t* rdram, recomp_context* ctx) {
    rt64_3ds::set_first_person(_arg<0, s32>(rdram, ctx) != 0);
}

// The fairy camera reads the picture out of the framebuffer (patches_heap.c).
extern "C" void recomp_fb_readback(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t fb = _arg<0, uint32_t>(rdram, ctx);
    const int32_t xy = _arg<1, int32_t>(rdram, ctx), wh = _arg<2, int32_t>(rdram, ctx);
    const int32_t width = _arg<3, int32_t>(rdram, ctx);
    rt64_3ds::read_back_frame(rdram, fb, (uint32_t)width, xy >> 16, xy & 0xFFFF, wh >> 16, wh & 0xFFFF);
}

extern "C" void recomp_get_story_skip(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, g_dk64_story_skip); }
extern "C" void recomp_get_builtin_mods(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    uint32_t on = 0;
    if (g_dk64_tag_anywhere) on |= BUILTIN_MOD_TAG_ANYWHERE;
    if (g_dk64_no_company_coins) on |= BUILTIN_MOD_NO_COMPANY_COINS;
    if (g_dk64_autocomplete_bonuses) on |= BUILTIN_MOD_AUTOCOMPLETE_BONUS;
    if (g_dk64_easier_beetle) on |= BUILTIN_MOD_EASIER_BEETLE;
    if (g_dk64_fixed_beaver_bother) on |= BUILTIN_MOD_FIXED_BEAVER_BOTHER;
    if (g_dk64_slow_shoe) on |= BUILTIN_MOD_SLOW_SHOE;
    if (g_dk64_slow_dk_phase) on |= BUILTIN_MOD_SLOW_DK_PHASE;
    _return(ctx, on);
}
extern "C" void recomp_get_camera_type(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, g_dk64_camera_type); }
// A float: an integer literal would land in the wrong return register.
extern "C" void recomp_get_lightning_intensity(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, g_dk64_lightning == 2 ? 1.0f : g_dk64_lightning == 1 ? 0.6f : 0.0f);
}
extern "C" void recomp_get_cutscene_bordering(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, kCutsceneBorders); }
extern "C" void recomp_get_mp_enabled(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, 0); }

// The UI is laid out for the top screen: 400x240, i.e. the 320-wide N64 UI
// expanded to the screen's 5:3.
extern "C" void recomp_get_ui_bounds(uint8_t* rdram, recomp_context* ctx) {
    s32* x_out = _arg<0, s32*>(rdram, ctx);
    s32* y_out = _arg<1, s32*>(rdram, ctx);
    *x_out = 400;
    *y_out = 240;
}

extern "C" void recomp_get_ui_pillar(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, (400 - 320) / 2);      // HUD keeps its 4:3 placement, centred
}

extern "C" void recomp_get_analog_cam_sensitivity(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return<uint32_t>(ctx, (uint32_t)g_dk64_analog_cam_sensitivity);
}

extern "C" void recomp_time_us(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, static_cast<u32>(std::chrono::duration_cast<std::chrono::microseconds>(ultramodern::time_since_start()).count()));
}

extern "C" void recomp_load_overlays(uint8_t* rdram, recomp_context* ctx) {
    u32 rom = _arg<0, u32>(rdram, ctx);
    PTR(void) ram = _arg<1, PTR(void)>(rdram, ctx);
    u32 size = _arg<2, u32>(rdram, ctx);
    load_overlays(rom, ram, size);
}

extern "C" void recomp_high_precision_fb_enabled(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, static_cast<s32>(0)); }
extern "C" void recomp_get_resolution_scale(uint8_t* rdram, recomp_context* ctx) { (void)rdram; _return(ctx, 1.0f); }

// Camera inversion: the desktop defaults are None (camera), None (third person),
// Invert Y (swimming) and Invert Y (first person), which match the original game.
static void return_axes(uint8_t* rdram, recomp_context* ctx, bool x, bool y) {
    s32* x_out = _arg<0, s32*>(rdram, ctx);
    s32* y_out = _arg<1, s32*>(rdram, ctx);
    *x_out = x; *y_out = y;
}
extern "C" void recomp_get_inverted_axes(uint8_t* rdram, recomp_context* ctx) { return_axes(rdram, ctx, false, false); }
extern "C" void recomp_get_analog_inverted_axes(uint8_t* rdram, recomp_context* ctx) {
    return_axes(rdram, ctx, (g_dk64_camera_invert & 1) != 0, (g_dk64_camera_invert & 2) != 0);
}
extern "C" void recomp_get_swimming_inverted_axes(uint8_t* rdram, recomp_context* ctx) {
    return_axes(rdram, ctx, (g_dk64_swim_invert & 1) != 0, (g_dk64_swim_invert & 2) != 0);
}
extern "C" void recomp_get_first_person_inverted_axes(uint8_t* rdram, recomp_context* ctx) {
    return_axes(rdram, ctx, (g_dk64_aim_invert & 1) != 0, (g_dk64_aim_invert & 2) != 0);
}

extern "C" void recomp_get_right_analog_inputs(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);
    if (g_right_analog_suppressed) {
        *x_out = 0.0f; *y_out = 0.0f;
        return;
    }
    recomp3ds::input_get_right_stick(x_out, y_out);
}

extern "C" void recomp_set_right_analog_suppressed(uint8_t* rdram, recomp_context* ctx) {
    g_right_analog_suppressed = _arg<0, s32>(rdram, ctx) != 0;
}

extern "C" void osPiReadIo_recomp(uint8_t* rdram, recomp_context* ctx) {
    uint32_t devAddr = recomp::rom_base | ctx->r4;
    gpr dramAddr = ctx->r5;
    uint32_t physical_addr = k1_to_phys(devAddr);
    if (physical_addr > recomp::rom_base) {
        recomp::do_rom_pio(rdram, dramAddr, physical_addr);
    }
    ctx->r2 = 0;
}

extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 11;   // PFS_ERR_DEVICE
}

extern "C" void recomp_load_overlays_by_rom(uint8_t* rdram, recomp_context* ctx) {
    u32 rom_addr = _arg<0, u32>(rdram, ctx);
    PTR(void) ram_addr = _arg<1, PTR(void)>(rdram, ctx);
    u32 size = _arg<2, u32>(rdram, ctx);
    load_overlays(rom_addr, ram_addr, size);
}

extern "C" void recomp_abort(uint8_t* rdram, recomp_context* ctx) {
    std::string msg = _arg_string<0>(rdram, ctx);
    ultramodern::error_handling::message_box(msg.c_str());
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
}

extern "C" void recomp_xxh3(uint8_t* rdram, recomp_context* ctx) {
    PTR(void) data = _arg<0, PTR(void)>(rdram, ctx);
    u32 size = _arg<1, u32>(rdram, ctx);
    XXH3_state_t xxh3;
    XXH3_64bits_reset(&xxh3);
    for (size_t i = 0; i < size; i++) {
        XXH3_64bits_update(&xxh3, TO_PTR(u8, data + i), 1);
    }
    uint64_t ret = XXH3_64bits_digest(&xxh3);
    ctx->r2 = (int32_t)(ret >> 32);
    ctx->r3 = (int32_t)(ret >> 0);
}

// Compressed ROM address -> decompressed ROM address of each code overlay
// (src/game/recomp_api.cpp keeps the same table).
extern "C" void load_dk64_overlay(uint32_t compressed_rom, int32_t ram_addr, uint32_t size) {
    uint32_t decompressed_rom = 0;
    switch (compressed_rom) {
        case 0x113F0: decompressed_rom = 0x2000000; ram_addr = 0x805FB300; size = 0x165D50; break;   // global_asm
        case 0xCBE70: decompressed_rom = 0x2165D50; ram_addr = 0x80024000; size = 0xFF10;   break;   // menu
        case 0xD4B00: decompressed_rom = 0x2175C60; ram_addr = 0x80024000; size = 0x3100;   break;   // multiplayer
        case 0xD6B00: decompressed_rom = 0x2178D60; ram_addr = 0x80024000; size = 0x4E10;   break;   // minecart
        case 0xD9A40: decompressed_rom = 0x217DB70; ram_addr = 0x80024000; size = 0x9EF0;   break;   // bonus
        case 0xDF600: decompressed_rom = 0x2187A60; ram_addr = 0x80024000; size = 0xC160;   break;   // race
        case 0xE6780: decompressed_rom = 0x2193BC0; ram_addr = 0x80024000; size = 0x61B0;   break;   // critter
        case 0xEA0B0: decompressed_rom = 0x2199D70; ram_addr = 0x80024000; size = 0x12DC0;  break;   // boss
        case 0xF41A0: decompressed_rom = 0x21ACB30; ram_addr = 0x80024000; size = 0x26C00;  break;   // arcade
        case 0xFD2F0: decompressed_rom = 0x21D3730; ram_addr = 0x80024000; size = 0xAC30;   break;   // jetpac
    }
    if (decompressed_rom != 0) {
        load_overlays(decompressed_rom, ram_addr, size);
    }
}

extern "C" void boot_osPiRawStartDma(uint8_t* rdram, recomp_context* ctx) {
    uint32_t device_address = ctx->r5;
    gpr rdram_address = ctx->r6;
    uint32_t size = ctx->r7;
    recomp::do_rom_read(rdram, rdram_address, device_address + recomp::rom_base, size);
}

extern "C" void __f_to_ull_recomp(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    uint64_t ret = (uint64_t)ctx->f12.fl;
    ctx->r2 = (int32_t)(ret >> 32);
    ctx->r3 = (int32_t)(ret >> 0);
}

extern "C" void __osSpSetStatus_recomp(uint8_t* rdram, recomp_context* ctx) { (void)rdram; (void)ctx; }

extern "C" void osViGetCurrentMode_recomp(uint8_t* rdram, recomp_context* ctx) {
    constexpr gpr os_vi_curr_ptr_addr = 0xFFFFFFFF80010190ull;
    const gpr vi_curr_ctx = (gpr)(int32_t)MEM_W(0, os_vi_curr_ptr_addr);
    if (vi_curr_ctx == 0) {
        ctx->r2 = 0;
        return;
    }
    const gpr modep = (gpr)(int32_t)MEM_W(0x8, vi_curr_ctx);
    if (modep == 0) {
        ctx->r2 = 0;
        return;
    }
    ctx->r2 = MEM_BU(0x3, modep);
}
