// The translations built into the 3DS build (lib/mods: RecompFrench,
// RecompGerman, RecompSpanish by Ballaam, RecompBrazilianPortuguese by
// brigandier; all CC0), one at a time: the Mods page's Language row, read
// at boot (recomp_get_language: 0 English, 1 French, 2 German, 3 Spanish,
// 4 Brazilian Portuguese).
//
// Each language is linked in as an object of its own whose symbols all
// take its tag (patches/Makefile, LANG_MODS_DIR: fr_, de_, es_, ptbr_), its
// patches plain functions there. The game functions they patch are patched
// here once, by dispatchers that call the chosen language's version, or for
// English a copy of the game's own from the decomp (lib/dk64_decomp,
// attributed at each), since a patch can't call the function it replaces.
// The languages' callbacks run from the game's events in builtin_mods.c
// (language_on_init and the rest below).
#if defined(DK64_LANGUAGES)

#include "common_structs.h"
#include "patch_helpers.h"
#include "languages.h"

DECLARE_FUNC(s32, recomp_get_language);

enum { LANG_EN, LANG_FR, LANG_DE, LANG_ES, LANG_PTBR, LANG_COUNT };
static s32 language = -1;

// At boot (the font tables are rewritten then, the text files cached as
// they load): a change takes a restart.
static s32 lang(void) {
    if (language < 0) {
        language = recomp_get_language();
        if (language < 0 || language >= LANG_COUNT) language = LANG_EN;
    }
    return language;
}

// ---- the languages' functions (lang_prefix.sh's names)
#define LANG_DECLS(t) \
    s8 t##_func_global_asm_806FB914(s16, u8 *); \
    void t##_func_global_asm_806FBB58(void); \
    Gfx *t##_func_global_asm_806FBEF0(Gfx *, u8, s16); \
    void t##_languageHackLoader(s32, s32, u8 **, s32 *); \
    void t##_languageHackInit(void);
LANG_DECLS(fr)
LANG_DECLS(de)
LANG_DECLS(es)
LANG_DECLS(ptbr)
void ptbr_arcade_draw_text(Gfx **, char *);
void ptbr_func_jetpac_8002AEFC(Gfx **, char *, s32, s32, s32);
s16 ptbr_func_global_asm_806FDB8C(s16, u8 *, u8, f32, f32, f32);
Gfx *ptbr_func_global_asm_8068C20C(Gfx *, u8);
void ptbr_ptbr_subtitle_dl_budget(s32 *);
void ptbr_ptbr_intro_update(void);
void ptbr_ptbr_fixed_text_frame(void);

// ---- the game's events (builtin_mods.c)

void language_on_init(void) {
    switch (lang()) {
        case LANG_FR: fr_languageHackInit(); break;
        case LANG_DE: de_languageHackInit(); break;
        case LANG_ES: es_languageHackInit(); break;
        case LANG_PTBR: ptbr_languageHackInit(); break;
    }
}

void language_asset_override(s32 tableIndex, s32 fileIndex, u8 **outputFile, s32 *file_size) {
    switch (lang()) {
        case LANG_FR: fr_languageHackLoader(tableIndex, fileIndex, outputFile, file_size); break;
        case LANG_DE: de_languageHackLoader(tableIndex, fileIndex, outputFile, file_size); break;
        case LANG_ES: es_languageHackLoader(tableIndex, fileIndex, outputFile, file_size); break;
        case LANG_PTBR: ptbr_languageHackLoader(tableIndex, fileIndex, outputFile, file_size); break;
    }
}

void language_every_frame(void) {
    if (lang() == LANG_PTBR) {
        ptbr_ptbr_intro_update();
        ptbr_ptbr_fixed_text_frame();
    }
}

void language_adjust_dl_allocation(s32 *allocation) {
    if (lang() == LANG_PTBR) ptbr_ptbr_subtitle_dl_budget(allocation);
}

// ---- the game's text functions, as the decomp has them (English)

// The decomp's declarations these use (its include/structs.h and
// functions.h); the game's allocator is _malloc here, and strlen the
// host's (patches.h).
typedef struct {
    u8 file_start;
    u8 file_count;
    u8 height;
    u8 kerning_space;
    u8 kerning_character;
    u8 kerning_animation;
} Struct80754A34;
typedef struct {
    f32 unk0, unk4, unk8, unkC, unk10, unk14, unk18, unk1C, unk20, unk24, unk28, unk2C, unk30, unk34;
} Struct806FD9FC;
typedef struct Struct80754AD0 Struct80754AD0;
struct Struct80754AD0 {
    Struct80754AD0 *next;
    u8 *unk4;
    Struct806FD9FC *unk8;
    u8 unkC;
    u8 unkD;
    u8 padE[0x10 - 0x0E];
    f32 unk10;
    f32 unk14;
    f32 unk18;
    s16 unk1C;
};
extern Struct80754AD0 *D_global_asm_80754AD0;
extern s16 D_global_asm_80754AD4;
extern void *_malloc(s32);
#define malloc _malloc
u8 *_strchr(const u8 *str, s32 c);
u8 *_strcpy(u8 *dst, const u8 *src);
// The game's rand under the name its recompilation gives it (a libc name):
// imported at its own address (syms.ld), as strlen is.
s32 rand_recomp(void);
#define rand rand_recomp
u32 strlen(const u8 *s);       // strlen_recomp, the host's (patches.h)
void func_global_asm_80611690(void *arg0);
void *func_global_asm_806FBB9C(s16 textureIndex);
s32 getCenterOfString(s16 renderStyle, u8 *string);
s16 func_global_asm_806FBEAC(u8 *arg0);
Gfx *func_global_asm_806FD490(Gfx *dl, s16 style, s16 x, s16 y, char *string);
extern Struct80754A34 D_global_asm_80754A34[];

typedef struct {
    s16 x_start;
    u8 width;
    u8 unk3;
} LangCharStruct;

extern void **D_global_asm_807FD7F0;
extern u8 *D_global_asm_807FD7F4;

typedef struct { u8 unk0[57]; } Struct806FB914_0;
typedef struct { u8 unk0[48]; } Struct806FB914_2;
extern Struct806FB914_0 D_global_asm_80754A64;
extern Struct806FB914_2 D_global_asm_80754AA0;

// func_global_asm_806FB914: lib/dk64_decomp src/global_asm/code_100180.c
// (its C, matching but for rodata).
static s8 vanilla_806FB914(s16 arg0, u8 *arg1) {
    s8 ret;
    Struct806FB914_0 spA4;
    Struct806FB914_2 sp74;

    ret = 0;
    switch (arg0) {
        case 0:
            switch (*arg1) {
                case 0x2E:
                case 0x67:
                case 0x70:
                case 0x71:
                case 0x80:
                    ret = 2;
                    break;
                case 0x61:
                case 0x79:
                    ret = 1;
                    break;
            }
            *arg1 -= 0x21;
            break;
        case 1:
            spA4 = D_global_asm_80754A64;
            *arg1 = _strchr(spA4.unk0, *arg1) - spA4.unk0;
            break;
        case 6:
            sp74 = D_global_asm_80754AA0;
            *arg1 = _strchr(sp74.unk0, *arg1) - sp74.unk0;
            break;
        case 2:
        {
            const char sp38[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-?{}:=0123456789<>m)!@#$%^&cab";
            *arg1 = _strchr((const u8 *)sp38, *arg1) - (const u8 *)sp38;
            break;
        }
        case 3:
        case 7:
        {
            const char sp28[] = "0123456789%/";
            *arg1 = _strchr((const u8 *)sp28, *arg1) - (const u8 *)sp28;
            break;
        }
        case 4:
            if (*arg1 == 0x40) {
                *arg1 = 0x60;
            } else {
                *arg1 = *arg1 - 0x21;
            }
            break;
        case 5:
            *arg1 -= 0x21;
            break;
    }
    return ret;
}

// func_global_asm_806FBB58: the same file.
static void vanilla_806FBB58(void) {
    D_global_asm_807FD7F0 = malloc(sizeof(s32) * 43);
    func_global_asm_80611690(D_global_asm_807FD7F4 = malloc(sizeof(u8) * 43));
}

// func_global_asm_806FBEF0: the same file.
static Gfx *vanilla_806FBEF0(Gfx *dl, u8 arg1, s16 arg2) {
    s16 textureIndex;

    textureIndex = (D_global_asm_80754A34[arg1].file_start + arg2) - 1;
    func_global_asm_806FBB9C(textureIndex);
    switch (arg1) {
        case 6:
            gDPLoadTextureBlock(dl++, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000,
                     G_IM_FMT_IA, G_IM_SIZ_8b, 176, 13, 0,
                     G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                     G_TX_NOMASK, G_TX_NOMASK,
                     G_TX_NOLOD, G_TX_NOLOD);
            break;
        case 0:
            gDPSetTextureImage(dl++, G_IM_FMT_IA, G_IM_SIZ_16b, 1, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000);\
            gDPSetTile(dl++, G_IM_FMT_IA, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPLoadSync(dl++);\
            gDPLoadBlock(dl++, G_TX_LOADTILE, 0, 0, 2047, 64);\
            gDPPipeSync(dl++);\
            gDPSetTile(dl++, G_IM_FMT_IA, G_IM_SIZ_4b, 32, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPSetTileSize(dl++, G_TX_RENDERTILE, 0, 0, 0x07FC, 0x003C);\
            break;
        case 1:
            gDPLoadTextureBlock(dl++, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000,
                     G_IM_FMT_RGBA, G_IM_SIZ_16b, 76, 24, 0,
                     G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                     G_TX_NOMASK, G_TX_NOMASK,
                     G_TX_NOLOD, G_TX_NOLOD);
            break;
        case 2:
            gDPSetTextureImage(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 1, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPLoadSync(dl++);\
            gDPLoadBlock(dl++, G_TX_LOADTILE, 0, 0, 1023, 64);\
            gDPPipeSync(dl++);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_4b, 32, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPSetTileSize(dl++, G_TX_RENDERTILE, 0, 0, 0x07FC, 0x001C);\
            break;
        case 3:
        case 7:
            gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000);\
            gDPSetTile(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPLoadSync(dl++);\
            gDPLoadBlock(dl++, G_TX_LOADTILE, 0, 0, 1023, 256);\
            gDPPipeSync(dl++);\
            gDPSetTile(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPSetTileSize(dl++, G_TX_RENDERTILE, 0, 0, 0x007C, 0x007C);\
            break;
        case 4:
            gDPSetTextureImage(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 1, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPLoadSync(dl++);\
            gDPLoadBlock(dl++, G_TX_LOADTILE, 0, 0, 2047, 32);\
            gDPPipeSync(dl++);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_4b, 64, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPSetTileSize(dl++, G_TX_RENDERTILE, 0, 0, 0x0FFC, 0x001C);\
            break;
        case 5:
            gDPSetTextureImage(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 1, (s32)D_global_asm_807FD7F0[textureIndex] + 0x80000000);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPLoadSync(dl++);\
            gDPLoadBlock(dl++, G_TX_LOADTILE, 0, 0, 2047, 32);\
            gDPPipeSync(dl++);\
            gDPSetTile(dl++, G_IM_FMT_I, G_IM_SIZ_4b, 64, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOLOD);\
            gDPSetTileSize(dl++, G_TX_RENDERTILE, 0, 0, 0x0FFC, 0x001C);\
            break;
    }
    return dl;
}


extern u8 D_global_asm_8074450C;

// func_global_asm_806FDB8C: lib/dk64_decomp src/global_asm/code_1025E0.c.
static s16 vanilla_806FDB8C(s16 arg0, u8 *arg1, u8 arg2, f32 arg3, f32 arg4, f32 arg5) {
    Struct80754AD0 *temp_v0;
    Struct80754AD0 *var_v1;
    s16 i;
    u32 len;

    getCenterOfString(arg0, arg1);
    temp_v0 = malloc(sizeof(Struct80754AD0));
    func_global_asm_80611690(temp_v0);
    if (D_global_asm_80754AD0 == NULL) {
        D_global_asm_80754AD0 = temp_v0;
        D_global_asm_80754AD0->next = NULL;
        D_global_asm_80754AD4 = 0;
    } else {
        var_v1 = D_global_asm_80754AD0;
        while (var_v1->next) {
            var_v1 = var_v1->next;
        }
        var_v1->next = temp_v0;
        temp_v0->next = NULL;
    }
    temp_v0->unk1C = D_global_asm_80754AD4++;
    len = strlen(arg1);
    temp_v0->unk4 = malloc(len + 1);
    _strcpy(temp_v0->unk4, arg1);
    temp_v0->unkD = func_global_asm_806FBEAC(arg1);
    temp_v0->unk8 = malloc(temp_v0->unkD * sizeof(Struct806FD9FC));
    func_global_asm_80611690(temp_v0->unk8);
    temp_v0->unkC = arg0;
    for (i = 0; i < temp_v0->unkD; i++) {
        if (arg2 == 3) {
            temp_v0->unk8[i].unk0 = (((rand() >> 0xF) % 1440) - 0x50);
            temp_v0->unk8[i].unk4 = (((rand() >> 0xF) % 1120) - 0x50);
            temp_v0->unk8[i].unk20 = (((rand() >> 0xF) % 6283) / 1000.0);
            temp_v0->unk14 = 0.1f;
        } else {
            temp_v0->unk8[i].unk0 = arg3 * 4.0f;
            temp_v0->unk8[i].unk4 = arg4 * 4.0f;
            temp_v0->unk8[i].unk8 = arg5 * 4.0f;
            temp_v0->unk8[i].unk30 = D_global_asm_8074450C == 2 ? 1.0 : 0.1;
            temp_v0->unk14 = 0.2f;
        }
    }
    return temp_v0->unk1C;
}

typedef struct {
    Gfx *(*unk0)(Gfx*, Actor*);
    Actor *unk4;
    u8 unk8;
    s8 unk9;
    s8 unkA;
    s8 unkB;
} LangTempCBig;
extern LangTempCBig D_global_asm_807FC630[];
extern u8 D_global_asm_807501E0;

// func_global_asm_8068C20C: lib/dk64_decomp src/global_asm/done/code_8F4B0.c.
static Gfx *vanilla_8068C20C(Gfx *dl, u8 arg1) {
    s32 j;
    u8 i;
    u8 k;

    i = D_global_asm_807501E0;

    while (i) {
        j = i - 1;
        k = j;
        i = j;
        if (arg1 == D_global_asm_807FC630[k].unk8) {
            gDPPipeSync(dl++);
            dl = D_global_asm_807FC630[i].unk0(dl, D_global_asm_807FC630[i].unk4);
        }
    }

    if (arg1 == 7) {
        D_global_asm_807501E0 = 0;
    }
    return dl;
}

extern u8 arcade_text_red, arcade_text_green, arcade_text_blue, arcade_text_alpha;
extern s32 arcade_text_x, arcade_text_y;

// arcade_draw_text: lib/dk64_decomp src/arcade/code_0.c.
static void vanilla_arcade_draw_text(Gfx **gpp, char *string) {
    Gfx *gp = *gpp;
    gDPPipeSync(gp++);
    gDPSetCycleType(gp++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gp++, -1);
    gSPSetGeometryMode(gp++, G_SHADE | G_SHADING_SMOOTH);
    gDPSetPrimColor(gp++, 0, 0, arcade_text_red, arcade_text_green, arcade_text_blue, arcade_text_alpha);
    gDPSetCombineMode(gp++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetRenderMode(gp++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    *gpp = func_global_asm_806FD490(gp, 2, (s16)arcade_text_x, (s16)arcade_text_y, string);
}

extern u8 D_jetpac_8002E9C0, D_jetpac_8002E9C4, D_jetpac_8002E9C8, D_jetpac_8002E9CC;
extern u8 D_jetpac_80045BE0, D_jetpac_80045BE1, D_jetpac_80045BE2, D_jetpac_80045BE3;

// func_jetpac_8002AEFC: lib/dk64_decomp src/jetpac/code_4FC0.c.
static void vanilla_jetpac_8002AEFC(Gfx **arg0, char *string, s32 x, s32 y, s32 arg4) {
    Gfx *dl;

    dl = *arg0;
    if (arg4 != 0) {
        gDPSetPrimColor(dl++, 0, 0, D_jetpac_8002E9C0, D_jetpac_8002E9C4, D_jetpac_8002E9C8, D_jetpac_8002E9CC);
        gDPSetEnvColor(dl++, D_jetpac_80045BE0, D_jetpac_80045BE1, D_jetpac_80045BE2, D_jetpac_80045BE3);
    } else {
        gDPSetPrimColor(dl++, 0, 0, D_jetpac_80045BE0, D_jetpac_80045BE1, D_jetpac_80045BE2, D_jetpac_80045BE3);
        gDPSetEnvColor(dl++, D_jetpac_8002E9C0, D_jetpac_8002E9C4, D_jetpac_8002E9C8, D_jetpac_8002E9CC);
    }
    *arg0 = func_global_asm_806FD490(dl, 4, x, y, string);
}

// ---- the dispatchers

RECOMP_PATCH s8 func_global_asm_806FB914(s16 arg0, u8 *arg1) {
    switch (lang()) {
        case LANG_FR: return fr_func_global_asm_806FB914(arg0, arg1);
        case LANG_DE: return de_func_global_asm_806FB914(arg0, arg1);
        case LANG_ES: return es_func_global_asm_806FB914(arg0, arg1);
        case LANG_PTBR: return ptbr_func_global_asm_806FB914(arg0, arg1);
        default: return vanilla_806FB914(arg0, arg1);
    }
}

RECOMP_PATCH void func_global_asm_806FBB58(void) {
    switch (lang()) {
        case LANG_FR: fr_func_global_asm_806FBB58(); break;
        case LANG_DE: de_func_global_asm_806FBB58(); break;
        case LANG_ES: es_func_global_asm_806FBB58(); break;
        case LANG_PTBR: ptbr_func_global_asm_806FBB58(); break;
        default: vanilla_806FBB58(); break;
    }
}

RECOMP_PATCH Gfx *func_global_asm_806FBEF0(Gfx *dl, u8 arg1, s16 arg2) {
    switch (lang()) {
        case LANG_FR: return fr_func_global_asm_806FBEF0(dl, arg1, arg2);
        case LANG_DE: return de_func_global_asm_806FBEF0(dl, arg1, arg2);
        case LANG_ES: return es_func_global_asm_806FBEF0(dl, arg1, arg2);
        case LANG_PTBR: return ptbr_func_global_asm_806FBEF0(dl, arg1, arg2);
        default: return vanilla_806FBEF0(dl, arg1, arg2);
    }
}

RECOMP_PATCH s16 func_global_asm_806FDB8C(s16 arg0, u8 *arg1, u8 arg2, f32 arg3, f32 arg4, f32 arg5) {
    if (lang() == LANG_PTBR) return ptbr_func_global_asm_806FDB8C(arg0, arg1, arg2, arg3, arg4, arg5);
    return vanilla_806FDB8C(arg0, arg1, arg2, arg3, arg4, arg5);
}

RECOMP_PATCH Gfx *func_global_asm_8068C20C(Gfx *dl, u8 pass) {
    if (lang() == LANG_PTBR) return ptbr_func_global_asm_8068C20C(dl, pass);
    return vanilla_8068C20C(dl, pass);
}

RECOMP_PATCH void arcade_draw_text(Gfx **gpp, char *string) {
    if (lang() == LANG_PTBR) ptbr_arcade_draw_text(gpp, string);
    else vanilla_arcade_draw_text(gpp, string);
}

RECOMP_PATCH void func_jetpac_8002AEFC(Gfx **arg0, char *string, s32 x, s32 y, s32 arg4) {
    if (lang() == LANG_PTBR) ptbr_func_jetpac_8002AEFC(arg0, string, x, y, arg4);
    else vanilla_jetpac_8002AEFC(arg0, string, x, y, arg4);
}

#endif
