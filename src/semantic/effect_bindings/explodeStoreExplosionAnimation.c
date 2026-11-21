/* Storage-view migration; the original function body is otherwise retained. */
union TM2UvRecord { int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
union TM2UvPool4 { union TM2UvRecord records[4]; int words[8]; unsigned short halves[16]; unsigned char bytes[32]; };
typedef char uv_pool_4_size[(sizeof(union TM2UvPool4) == 32) ? 1 : -1];
union TM2UvPool8 { union TM2UvRecord records[8]; int words[16]; unsigned short halves[32]; unsigned char bytes[64]; };
typedef char uv_pool_8_size[(sizeof(union TM2UvPool8) == 64) ? 1 : -1];
union TM2UvPool10 { union TM2UvRecord records[10]; int words[20]; unsigned short halves[40]; unsigned char bytes[80]; };
typedef char uv_pool_10_size[(sizeof(union TM2UvPool10) == 80) ? 1 : -1];
union TM2UvPool16 { union TM2UvRecord records[16]; int words[32]; unsigned short halves[64]; unsigned char bytes[128]; };
typedef char uv_pool_16_size[(sizeof(union TM2UvPool16) == 128) ? 1 : -1];
extern union TM2UvPool16 fxExplosionFrames;
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;typedef long long s64; typedef unsigned long long u64;
/* cc: fill */
extern int grutilsParseSpriteRects();
void explodeStoreExplosionAnimation(int arg0) {
    s32 var_v1;
    u16 *var_s0;

    grutilsParseSpriteRects(arg0, fxExplosionFrames.halves, 0x10);
    var_v1 = 0;
    var_s0 = fxExplosionFrames.halves + 2;
    do {
        var_v1 += 1;
        *var_s0 = (*var_s0 & 0x1F) + 0x20;
        var_s0 += 4;
    } while (var_v1 < 0x10);
}
