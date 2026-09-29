// The mods built into the patches where there is no mod loader (the 3DS):
// one bit each in recomp_get_builtin_mods(), set while the mod is switched
// on. Shared by patches/builtin_mods.c and the 3DS host
// (platform/3ds/recomp_api_3ds.cpp).
#ifndef __BUILTIN_MODS_H__
#define __BUILTIN_MODS_H__

#define BUILTIN_MOD_TAG_ANYWHERE        (1 << 0)    // lib/mods/DK64TagAnywhereRecomp
#define BUILTIN_MOD_NO_COMPANY_COINS    (1 << 1)    // lib/mods/RecompNoCompanyCoins
#define BUILTIN_MOD_AUTOCOMPLETE_BONUS  (1 << 2)    // lib/mods/RecompAutocompleteBonuses
#define BUILTIN_MOD_EASIER_BEETLE       (1 << 3)    // lib/mods/RecompEasierBeetle
#define BUILTIN_MOD_FIXED_BEAVER_BOTHER (1 << 4)    // lib/mods/RecompFixedBeaverBother
#define BUILTIN_MOD_SLOW_SHOE           (1 << 5)    // lib/mods/RecompSlowShoe
#define BUILTIN_MOD_SLOW_DK_PHASE       (1 << 6)    // lib/mods/RecompSlowDKPhase

#ifdef MIPS
// Whether a mod is switched on, for the mods' own patches
// (platform/3ds/mods/<mod>/*.patch).
int builtin_mod_on(unsigned int bit);
#endif

#endif
