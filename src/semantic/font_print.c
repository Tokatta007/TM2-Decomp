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
extern union TM2FontSprite fontSpriteTemplates[];   /* = fxFontUvStorage.layout.sprites */
extern int fontExtraSpacing[7], fontVramOffsets[7][2];
extern unsigned char *fontGlyphTables[7];
extern int fontCursorX, fontCursorY;
extern int fontMapUsefulCharacter(unsigned int);
extern int fontStringWidth(int, const unsigned char *);
extern int DrawSync(int);
extern void DrawPrim(void *);

#define FONT_MAPC(c) ((unsigned char)fontMapUsefulCharacter(c))
/* Glyph index for a character: mapped code 48..124 -> 0..76, anything else -> 47 (blank).
 * Retail calls fontMapUsefulCharacter up to three times (a macro in the original). */
#define FONT_GLYPH(c) ((FONT_MAPC(c) >= 48 && FONT_MAPC(c) < 125) ? FONT_MAPC(c) - 48 : 47)

/* @80113724: direct GPU printing at the cursor (fontCursorX, fontCursorY). LF resets X and
 * advances Y, CR only advances Y, BS steps X back one pixel, space advances by glyph 47's
 * width. Returns the number of bytes consumed. */
int __attribute__((section(".text_80113724"))) fontPrint(int font, const unsigned char *text)
{
    int count = 0;
    unsigned char c;
    DrawSync(0);
    DrawPrim(&fontDrawModes[font]);
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
            union TM2FontSprite *sprite;
            glyph = (FONT_MAPC(c) < 48 || FONT_MAPC(c) >= 125) ? 47 : FONT_MAPC(c) - 48;
            DrawSync(0);
            sprite = &fontSpriteTemplates[font];
            sprite->bytes[12] = fontGlyphTables[font][glyph * 6] + fontVramOffsets[font][0];
            sprite->bytes[13] = fontGlyphTables[font][glyph * 6 + 1] + fontVramOffsets[font][1];
            sprite->halves[8] = fontGlyphTables[font][glyph * 6 + 2];
            sprite->halves[9] = fontGlyphTables[font][glyph * 6 + 3];
            sprite->halves[4] = fontCursorX;
            sprite->halves[5] = fontCursorY - fontGlyphTables[font][glyph * 6 + 3] + *(unsigned short *)&fontGlyphTables[font][glyph * 6 + 4];
            DrawPrim(sprite);
            fontCursorX += fontGlyphTables[font][glyph * 6 + 2] + fontExtraSpacing[font];
            break;
        }
        }
        text++;
        c = *text;
        count++;
    }
    DrawSync(0);
    return count;
}

/* @801134D8: set the cursor and print. */
int __attribute__((section(".text_801134D8"))) fontPrintXY(int font, int x, int y, const unsigned char *text)
{
    fontCursorX = x;
    fontCursorY = y;
    return fontPrint(font, text);
}

/* @801136B8: print centred on x (arithmetic right shift, not signed division by two). */
int __attribute__((section(".text_801136B8"))) fontPrintCenteredXY(int font, int x, int y, const unsigned char *text)
{
    int width = fontStringWidth(font, text);
    fontCursorX = (unsigned int)x - (unsigned int)(width >> 1);
    fontCursorY = y;
    return fontPrint(font, text);
}
