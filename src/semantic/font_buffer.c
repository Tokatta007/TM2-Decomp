union TM2UvRecord { unsigned int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
union TM2EffectFontStorage {
    struct { union TM2UvRecord effects[10]; union TM2FontDrawMode drawmode[7]; union TM2FontSprite sprites[7]; } layout;
    union TM2UvRecord receive[30];
    unsigned int words[76]; unsigned short halves[152]; unsigned char bytes[304];
};
typedef char shared_font_size[(sizeof(union TM2EffectFontStorage)==304)?1:-1];
extern union TM2EffectFontStorage fxFontUvStorage;
extern union TM2FontDrawMode fontDrawModes[]; /* = fxFontUvStorage.layout.drawmode */
extern union TM2FontSprite fontSpriteTemplates[];   /* = fxFontUvStorage.layout.sprites: template sprite per font */
extern int fontExtraSpacing[7], fontVramOffsets[7][2];
extern unsigned char *fontGlyphTables[7];
extern int fontCursorX, fontCursorY;
extern int fontMapUsefulCharacter(unsigned int);
extern void AddPrim(unsigned int *, void *);

#define FONT_MAPC(c) ((unsigned char)fontMapUsefulCharacter(c))
/* Glyph index for a character: mapped code 48..124 -> 0..76, anything else -> 47 (blank).
 * Retail calls fontMapUsefulCharacter up to three times (a macro in the original). */
#define FONT_GLYPH(c) ((FONT_MAPC(c) < 48 || FONT_MAPC(c) >= 125) ? 47 : FONT_MAPC(c) - 48)

/* @80113C00: like fontPrint, but builds sprite packets into `packets` and links them into
 * `ot` instead of drawing directly. Every consumed byte reserves a sprite slot, including
 * controls and spaces; only actual glyphs are linked. The draw mode (copied into `mode`)
 * is linked last, so it is drawn first. Returns the number of bytes consumed. */
int __attribute__((section(".text_80113C00"))) fontSpritePrint(int font, union TM2FontDrawMode *mode,
        union TM2FontSprite *packets, unsigned char *text, unsigned int *ot)
{
    int count = 0;
    union TM2FontSprite *pkt;
    unsigned char c;
    __builtin_memcpy(mode, &fontDrawModes[font], 12);
    c = *text;
    while (c) {
        switch (c) {
        case 10:
            fontCursorX = 0;
            fontCursorY += fontGlyphTables[font][261];
            break;
        case 13:
            fontCursorY += fontGlyphTables[font][261];
            break;
        case 32:
            fontCursorX += fontGlyphTables[font][284] + fontExtraSpacing[font];
            break;
        case 8:
            fontCursorX--;
            break;
        default: {
            int glyph;
            glyph = FONT_GLYPH(c);
            pkt = &packets[count];
            __builtin_memcpy(pkt, &fontSpriteTemplates[font], 20);
            pkt->bytes[12] = fontGlyphTables[font][glyph * 6] + fontVramOffsets[font][0];
            pkt->bytes[13] = fontGlyphTables[font][glyph * 6 + 1] + fontVramOffsets[font][1];
            pkt->halves[8] = fontGlyphTables[font][glyph * 6 + 2];
            pkt->halves[9] = fontGlyphTables[font][glyph * 6 + 3];
            pkt->halves[4] = fontCursorX;
            pkt->halves[5] = fontCursorY - fontGlyphTables[font][glyph * 6 + 3] + *(unsigned short *)&fontGlyphTables[font][glyph * 6 + 4];
            AddPrim(ot, pkt);
            fontCursorX += fontGlyphTables[font][glyph * 6 + 2] + fontExtraSpacing[font];
            break;
        }
        }
        text++;
        c = *text;
        count++;
    }
    AddPrim(ot, mode);
    return count;
}

/* @801139D8: set the cursor and build sprite packets (seven o32 arguments, three on the stack). */
int __attribute__((section(".text_801139D8"))) fontSpritePrintXY(int font, int x, int y,
        union TM2FontDrawMode *mode, union TM2FontSprite *packets, const unsigned char *text, unsigned int *ot)
{
    fontCursorX = x;
    fontCursorY = y;
    return fontSpritePrint(font, mode, packets, text, ot);
}

/* @80113A14: measure the text by running the cursor over it (also moves Y on CR/LF), then
 * build sprite packets centred on x at y. */
int __attribute__((section(".text_80113A14"))) fontSpritePrintCenteredXY(int font, int x, int y,
        union TM2FontDrawMode *mode, union TM2FontSprite *packets, unsigned char *text, unsigned int *ot)
{
    unsigned char *p = text;   /* non-const: a const text load would float above the cursor stores */
    unsigned char c;
    fontCursorX = 0;
    fontCursorY = 0;
    c = *p;
    while (c) {
        switch (c) {
        case 10:
            fontCursorX = 0;
            fontCursorY += fontGlyphTables[font][261];
            break;
        case 13:
            fontCursorY += fontGlyphTables[font][261];
            break;
        case 32:
            fontCursorX += fontGlyphTables[font][284] + fontExtraSpacing[font];
            break;
        case 8:
            fontCursorX--;
            break;
        default: {
            int glyph = FONT_GLYPH(c);
            fontCursorX += fontGlyphTables[font][glyph * 6 + 2] + fontExtraSpacing[font];
            break;
        }
        }
        p++;
        c = *p;
    }
    fontCursorY = y;
    fontCursorX = x - (fontCursorX >> 1);
    return fontSpritePrint(font, mode, packets, text, ot);
}
