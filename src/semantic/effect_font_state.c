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
/* @80187194..801872C4: one backing store for overlapping retail views.
 * This does not claim the original C declared a thirty-record effect array. */
union TM2EffectFontStorage fxFontUvStorage __attribute__((section(".tm2_80187194"))) = {0};
