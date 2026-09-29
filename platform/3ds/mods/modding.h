// Stand-in for the mods' include/modding.h when they are built into the 3DS
// build's patches (lib/mods, see patches/Makefile, BUILTIN_MODS_DIR): found
// first on the include path. The 3DS runs without the mod loader, so
// RECOMP_CALLBACK functions become plain functions that patches/builtin_mods.c
// calls from the game's events, RECOMP_IMPORT only declares (builtin_mods.c
// supplies what the mods use), and a mod's own events are empty functions.
// The mod-only imports of recomputils.h are left out; no built-in mod uses
// them.
#ifndef __MODDING_H__
#define __MODDING_H__

#define RECOMP_EXPORT __attribute__((section(".recomp_export")))

#define RECOMP_PATCH __attribute__((section(".recomp_patch")))

#define RECOMP_FORCE_PATCH __attribute__((section(".recomp_force_patch")))

#define RECOMP_IMPORT(mod, func) func

#define RECOMP_DECLARE_EVENT(func)                                                                              \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")                     \
        __attribute__((noinline)) void func {}                                                                  \
    _Pragma("GCC diagnostic pop")

#define RECOMP_CALLBACK(mod, event)

#endif
