// Stand-in for the translation mods' include/modding.h (lib/mods/Recomp*
// languages, see patches/Makefile, LANG_MODS_DIR): found first on their
// include path. Each language is built into the 3DS build's patches as an
// object of its own whose symbols all take the language's prefix (fr_,
// de_, es_, ptbr_), so its patches are plain functions here and
// patches/languages.c replaces the game's functions with dispatchers that
// call the chosen language's, or the game's own (English). Its callbacks
// are plain functions too, which languages.c calls from the game's events.
#ifndef __MODDING_H__
#define __MODDING_H__

#define RECOMP_EXPORT
#define RECOMP_PATCH
#define RECOMP_FORCE_PATCH
#define RECOMP_IMPORT(mod, func) func

#define RECOMP_DECLARE_EVENT(func)                                                                              \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")                     \
        __attribute__((noinline)) void func {}                                                                  \
    _Pragma("GCC diagnostic pop")

#define RECOMP_CALLBACK(mod, event)

#endif
