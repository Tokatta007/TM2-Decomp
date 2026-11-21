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
extern union TM2UvPool16 fxSmokeFramesB;
/* cc: fill */
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;
extern int fxSpriteSpawn();
struct S { u8 pad0[2]; s8 unk2; s8 unk3; s8 unk4; s8 unk5; u8 pad6[0x22]; s32 unk28; };
void fxSmokeB12(s32 arg0, s32 arg1){
    struct S *v0 = (struct S *)fxSpriteSpawn(arg0, 0xc, 1, 0x10, fxSmokeFramesB.bytes, 1, arg1 & 0xff);
    if (v0 != 0) {
        v0->unk28 = 6;
        v0->unk2 = 1;
        v0->unk3 = -0x80;
        v0->unk4 = -0x80;
        v0->unk5 = -0x80;
    }
}
