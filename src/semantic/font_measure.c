extern int fontExtraSpacing[7];
extern unsigned char *fontGlyphTables[7];

/* @80113500. Retail masks the input before mapping punctuation. */
int __attribute__((section(".text_80113500"))) fontMapUsefulCharacter(unsigned int c)
{
    c &= 255;
    if (c == '.') return 59;
    if (c == '!') return 92;
    if (c == ',') return 94;
    if (c == '\'') return 96;
    if (c == '*') return 123;
    return c;
}

/* @8011355C. Width is the final line's accumulator, not the widest line.
 * The glyph lookup maps the character up to three times (a macro), as in
 * retail. */
#define TM2_FONT_MAP(c) ((unsigned char)fontMapUsefulCharacter(c))
int __attribute__((section(".text_8011355C"))) fontStringWidth(int font, unsigned char *text)
{
    int width = 0;
    unsigned char c;
    for (; (c = *text) != 0; text++) {
        switch (c) {
        case 10:
            width = 0;
            break;
        case 32:
            width += fontGlyphTables[font][47 * 6 + 2] + fontExtraSpacing[font];
            break;
        case 8:
            width--;
            break;
        case 13:
            break;
        default: {
            int glyph = (TM2_FONT_MAP(c) < 48 || TM2_FONT_MAP(c) >= 125) ? 47 : TM2_FONT_MAP(c) - 48;
            width += fontGlyphTables[font][glyph * 6 + 2] + fontExtraSpacing[font];
            break;
        }
        }
    }
    return width;
}
