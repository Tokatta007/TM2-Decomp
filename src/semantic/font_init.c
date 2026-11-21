union TM2UvRecord { unsigned int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
union TM2EffectFontStorage {
    struct { union TM2UvRecord effects[10]; union TM2FontDrawMode drawmode[7]; union TM2FontSprite sprites[7]; } layout;
    union TM2UvRecord receive[30];
    unsigned int words[76]; unsigned short halves[152]; unsigned char bytes[304];
};
struct TM2FontAsset { const char *name; int loaded, page, clut; };
struct TM2TimInfo {
    unsigned int mode;
    short px, py;
    unsigned short width, height;
    void *pixels;
    short cx, cy;
    unsigned short cw, ch;
    void *clut;
};
typedef char tim_size[(sizeof(struct TM2TimInfo) == 28) ? 1 : -1];
typedef char font_shared_size[(sizeof(union TM2EffectFontStorage) == 304) ? 1 : -1];
extern union TM2EffectFontStorage fxFontUvStorage;
extern int fontExtraSpacing[7];
extern unsigned char *fontGlyphTables[7];
extern int fontVramOffsets[7][2];
extern unsigned char fontGlyphsMain[], fontGlyphsFont6[], fontGlyphsFont4[], fontGlyphsCp[]; /* glyph tables (6 bytes per glyph) */
extern char aScreenUafont1T[20], aScreenCpfontTi[20];
extern int fontCursorX, fontCursorY;
extern unsigned char *fileioWorkBuffer;
#pragma tm2_absolute fileioWorkBuffer
extern union TM2FontDrawMode fontDrawModes[]; /* = fxFontUvStorage.layout.drawmode */
extern union TM2FontSprite fontSpriteTemplates[7];   /* = fxFontUvStorage.layout.sprites */
/* The local font-asset initialiser lives in retail .rodata at 800CF1F4. */
#pragma tm2_rdata .tm2_800CF1F4
extern int fileioLoadFileIntoRam(void *, const char *);
extern void GsGetTimInfo(void *, struct TM2TimInfo *);
extern int LoadImage(short *, void *);
extern unsigned short GetTPage(unsigned int, int, int, int);
extern unsigned short Load4BitClut(void *, int, int);
#include "semantic/hud.h"
extern void SetShadeTex(void *, int);
extern void SetDrawMode(void *, int, int, int, void *);
extern int DrawSync(int);

/* @80113F30: initialise the seven font slots. Fonts 2 and 3 use CPFONT.TIM, the others
 * UAFONT1.TIM; each TIM is loaded once (into VRAM at 960+offset, 256+offset, with its
 * 4-bit CLUT at row 511-slot), then every slot's sprite and draw-mode templates are set
 * up. Glyph tables (fontGlyphTables) and extra spacing (fontExtraSpacing) are assigned per slot.
 * Byte-identical. Store order and registers come from the two schedulers and local-alloc:
 * - sched1 keeps the stores in source order and sinks each constant load (g = fontGlyphsMain,
 *   m = -1, the Font4/3/7/Font6 temporaries, Cp) to just before its first use.
 * - local-alloc ranks g and m (4 refs each) by shorter life. The slot 4-6 stores (array
 *   names, so non-volatile) are written between gt[1] and sp[6]: g's life holds only the
 *   Font4 temporary while m's runs on to sp[6], so g gets v1 and m a0 (the v0 temporaries
 *   keep both off v0; Cp is born after Font6 dies and gets v0).
 * - sched2 keeps insn order among ready stores, but each v0 store stays blocked until the next
 *   v0 load is scheduled, and loads are only picked once no store is ready. So the
 *   Font4..Font6 chain moves up above the cursor/[0]/[1]/gt[5] stores and the Cp load sits
 *   right after gt[6], giving retail's order (with sp[6] before gt[3]).
 * The sprite is addressed as &fontSpriteTemplates[i] (a giv, as in retail)
 * and the CLUT store as fontSpriteTemplates[i] too: loop.c then reduces the pointer giv and the
 * i*20 index giv separately, as retail does. */
void __attribute__((section(".text_80113F30"))) fontInit(void)
{
    short rect[4];
    struct TM2TimInfo image;
    struct TM2FontAsset assets[2] = {
        { aScreenUafont1T, 0, -1, -1 },
        { aScreenCpfontTi, 0, -1, -1 }
    };
    struct TM2FontAsset *slots[7] = {
        &assets[0], &assets[0], &assets[1], &assets[1], &assets[0], &assets[0], &assets[0]
    };
    int i;
    {
        int *sp = fontExtraSpacing;
        unsigned char **gt = fontGlyphTables;
        unsigned char *g = fontGlyphsMain;
        int m = -1;
        fontCursorX = 0;
        fontCursorY = 0;
        fontGlyphTables[0] = g;
        fontExtraSpacing[0] = m;
        gt[1] = g;
        fontGlyphTables[4] = fontGlyphsFont4;
        sp[1] = m;
        gt[5] = g;
        fontExtraSpacing[4] = 3;
        fontExtraSpacing[5] = 7;
        fontGlyphTables[6] = fontGlyphsFont6;
        sp[6] = m;
        gt[3] = fontGlyphsCp;
        sp[3] = 0;
        gt[2] = fontGlyphsCp;
        sp[2] = 0;
    }
    for (i = 0; i < 7; i++) {
        if (!slots[i]->loaded) {
            fileioLoadFileIntoRam(fileioWorkBuffer, slots[i]->name);
            GsGetTimInfo(fileioWorkBuffer + 4, &image);
            rect[0] = fontVramOffsets[i][0] + 960;
            rect[1] = fontVramOffsets[i][1] + 256;
            rect[2] = image.width;
            rect[3] = image.height;
            LoadImage(rect, image.pixels);
            slots[i]->page = (unsigned short)GetTPage(image.mode, 0, rect[0], rect[1]);
            if (image.clut)
                slots[i]->clut = (unsigned short)Load4BitClut(image.clut, 960, 511 - i);
            slots[i]->loaded = 1;
        }
        /* (&fontSpriteTemplates[i])-> (not fontSpriteTemplates[i].): the pointer form is the giv */
        (&fontSpriteTemplates[i])->bytes[3] = 4;
        (&fontSpriteTemplates[i])->bytes[7] = 100;
        if (slots[i]->clut != -1)
            fontSpriteTemplates[i].halves[7] = slots[i]->clut;
        else
            fontSpriteTemplates[i].halves[7] = 0;
        fontResetColor(i);
        SetShadeTex(&fontSpriteTemplates[i], 0);
        (&fontSpriteTemplates[i])->bytes[12] = 0;
        (&fontSpriteTemplates[i])->bytes[13] = 0;
        SetDrawMode(&fontDrawModes[i], 0, 0, slots[i]->page, 0);
        DrawSync(0);
    }
}
