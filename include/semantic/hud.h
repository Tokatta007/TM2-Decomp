/* hud.h: shared prototypes (see docs/SIGNATURES.md). Plain C types only, so units
 * with their own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_HUD_H
#define TM2_SEMANTIC_HUD_H

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2Glyph; struct TM2Sprite;
#ifdef __cplusplus
extern "C" {
#endif

/* effect_packet.c */
int grutilsParseSpriteRects(void *arg0, unsigned char *output, unsigned int requested);

/* font_buffer.c */
int fontSpritePrintCenteredXY(int font, int x, int y, void *mode, void *packets, unsigned char *text, unsigned int *ot);
int fontSpritePrintXY(int font, int x, int y, void *mode, void *packets, const unsigned char *text, unsigned int *ot);

/* font_color.c */
void fontResetColor(int index);
void fontSetColor(int index, int r, int g, int b);

/* font_print.c */
int fontPrintCenteredXY(int font, int x, int y, const unsigned char *text);

/* hud_init.c */
void hudInitPanels(void);

/* packet_digits.c */
int grutilsParse2DSprite(void *source, unsigned char *records, unsigned int requested);
void hudDrawNumber(int value, struct TM2Sprite *sprites, struct TM2Glyph *glyphs, unsigned int *ot);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
