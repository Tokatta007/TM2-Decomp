union TM2UvRecord { unsigned int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
union TM2EffectFontStorage {
    struct {
        union TM2UvRecord effects[10];
        union TM2FontDrawMode drawmode[7];
        union TM2FontSprite sprites[7];
    } layout;
    union TM2UvRecord receive[30];
    unsigned int words[76];
    unsigned short halves[152];
    unsigned char bytes[304];
};
typedef char shared_graphics_size[(sizeof(union TM2EffectFontStorage) == 304) ? 1 : -1];
typedef char drawmode_offset[((unsigned int)&((union TM2EffectFontStorage *)0)->layout.drawmode == 80) ? 1 : -1];
typedef char sprite_offset[((unsigned int)&((union TM2EffectFontStorage *)0)->layout.sprites == 164) ? 1 : -1];
extern union TM2EffectFontStorage fxFontUvStorage;
extern union TM2FontSprite fontSpriteTemplates[];   /* = fxFontUvStorage.layout.sprites */
#pragma tm2_rdata .tm2_800CF214
#include "semantic/hud.h"
#include "semantic/fx.h"

/* @80112AB0: load the font/effect UV records from an asset (retail requests thirty
 * 8-byte records, overlapping the following draw-mode and sprite templates). */
void __attribute__((section(".text_80112AB0"))) explodeStoreLightningAnimation(void *source)
{
    grutilsParseSpriteRects(source, fxFontUvStorage.bytes, 30);
}

/* @80112600: spawn a 10-frame effect animation from the shared UV bank at `position`;
 * on success set its frame delay 5 and colour/flag bytes. */
signed char *__attribute__((section(".text_80112600"))) fxLightningSpark(void *position, unsigned int arg1)
{
    signed char *result = (signed char *)fxSpriteSpawn(position, 8, 1, 10, fxFontUvStorage.bytes, 1, arg1 & 255);
    if (result) {
        *(short *)(result + 12) = 5;
        result[2] = 1;
        result[3] = 255;
        result[4] = 200;
        result[5] = 50;
    }
    return result;
}

/* @80114284: set the colour of font `index` (0..6) sprite template. */
void __attribute__((section(".text_80114284"))) fontSetColor(int index, int r, int g, int b)
{
    union TM2FontSprite *base = fontSpriteTemplates;
    union TM2FontSprite *sprite = &base[index];
    sprite->bytes[4] = r;
    sprite->bytes[5] = g;
    sprite->bytes[6] = b;
}

/* @801142B0: reset font `index` to its default colour; indices >= 7 are ignored. */
void __attribute__((section(".text_801142B0"))) fontResetColor(int index)
{
    union TM2FontSprite *sprite = &fontSpriteTemplates[index];
    switch (index) {
    case 0: sprite->bytes[4] = 93;  sprite->bytes[5] = 30;  sprite->bytes[6] = 23;  break;
    case 1: sprite->bytes[4] = 115; sprite->bytes[5] = 61;  sprite->bytes[6] = 27;  break;
    case 3: sprite->bytes[4] = 115; sprite->bytes[5] = 25;  sprite->bytes[6] = 8;   break;
    case 2: sprite->bytes[4] = 128; sprite->bytes[5] = 128; sprite->bytes[6] = 128; break;
    case 4: sprite->bytes[4] = 100; sprite->bytes[5] = 0;   sprite->bytes[6] = 30;  break;
    case 6: sprite->bytes[4] = 115; sprite->bytes[5] = 84;  sprite->bytes[6] = 35;  break;
    case 5: sprite->bytes[4] = 50;  sprite->bytes[5] = 50;  sprite->bytes[6] = 50;  break;
    }
}
