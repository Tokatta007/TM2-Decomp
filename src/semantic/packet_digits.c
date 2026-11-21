#include "semantic/hud.h"
/* AddPrim is void, as in PsyQ libgpu.h. hudDrawNumber returns no value: retail only
 * leaves AddPrim's v0 behind. With no v0 in the RTL, reload may reuse v0 for the
 * hundreds division (an int-returning AddPrim made v0 a bad spill reg: 2 words off). */
extern void AddPrim(void *,void *);

/* @8014E130. Extraction returns differences between bounds; sprite extents
 * include the last pixel. Both byte increments wrap, including 255 -> 0. */
int __attribute__((section(".text_8014E130"))) grutilsParse2DSprite(void *source,
    unsigned char *records,unsigned int requested)
{
    int count=grutilsParseSpriteRects(source,records,requested);
    int i;
    for (i=0;i<count;++i) {
        records[2] += 1;
        records[3] += 1;
        records+=8;
    }
    return count;
}

/* Font glyph descriptor (8 bytes) and 20-byte textured sprite packet. */
struct TM2Glyph { unsigned char u, v, w, h; unsigned char pad[4]; };
struct TM2Sprite {
    unsigned int tag;
    unsigned char r0, g0, b0, code;
    unsigned short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    unsigned short w, h;
};

#define DIGIT(p,q) ((p)->u0 = (q)->u, (p)->v0 = (q)->v, (p)->w = (q)->w, (p)->h = (q)->h)

/* @8014E194. Draw a 1-3 digit number: one sprite per digit, each next X is the
 * previous sprite's X + width + 1 (read after its OT insertion); values of 1000
 * and more are reduced modulo 1000 after the digit count is chosen. Returns
 * nothing (callers ignore the result). */
void __attribute__((section(".text_8014E194"))) hudDrawNumber(int value,
    struct TM2Sprite *sprites, struct TM2Glyph *glyphs, unsigned int *ot)
{
    int digit;
    if (value < 10) {
        DIGIT(&sprites[0], &glyphs[value]);
        AddPrim(ot, &sprites[0]);
        return;
    }
    if (value < 100) {
        digit = value % 10;
        DIGIT(&sprites[0], &glyphs[value / 10]);
        AddPrim(ot, &sprites[0]);
        sprites[1].x0 = sprites[0].x0 + sprites[0].w + 1;
        DIGIT(&sprites[1], &glyphs[digit]);
        AddPrim(ot, &sprites[1]);
        return;
    }
    if (value >= 1000) value %= 1000;
    {
        int ones = value % 10, tens = (value % 100) / 10;
        DIGIT(&sprites[0], &glyphs[value / 100]);
        AddPrim(ot, &sprites[0]);
        sprites[1].x0 = sprites[0].x0 + sprites[0].w + 1;
        DIGIT(&sprites[1], &glyphs[tens]);
        AddPrim(ot, &sprites[1]);
        sprites[2].x0 = sprites[1].x0 + sprites[1].w + 1;
        DIGIT(&sprites[2], &glyphs[ones]);
        AddPrim(ot, &sprites[2]);
        return;
    }
}
