typedef signed int s32;
typedef unsigned int u32;
typedef signed short s16;
typedef unsigned short u16;
typedef signed char s8;
typedef unsigned char u8;

extern u32 menuTextVtbl[4];
extern u8 *fileioWorkBuffer;
#pragma tm2_absolute fileioWorkBuffer
extern u8 movieRgb24;
extern void *sub_80123E98(void *, u16 first, u16 second);
#include "semantic/hud.h"

/* Menu text-item constructor: base object init (sub_80123E98), vtable
   menuTextVtbl, font/colour id (+52), value (+56), mode (+60), position
   (+62/+64); then sets the font colour for that id. Returns the object. */
void * __attribute__((section(".text_801373EC")))
menuTextInit(u8 *object, s32 identifier, s32 value, s8 mode,
             u16 first, u16 second, u16 third)
{
    sub_80123E98(object, 0, third);
    *(u32 *)(object + 48) = (u32)menuTextVtbl;
    object[52] = identifier;
    *(s32 *)(object + 56) = value;
    *(s8 *)(object + 60) = mode;
    *(u16 *)(object + 62) = first;
    *(u16 *)(object + 64) = second;
    fontSetColor(object[52], 255, 161, 0);
    return object;
}

/* Movie/display buffer setup: three frame buffers at fileioWorkBuffer+0x10000
   spaced 0x12C00 apart, mode flag (+32 = mode^1), the display/draw rects
   (+16..+30) and a clip rect at +36 whose width is 24 when movieRgb24
   (wide mode) is set, else 16. */
void __attribute__((section(".text_80137FD0")))
movieBuffersInit(u8 *object, s32 mode, s32 x, s32 y,
             s32 x2, s32 y2, s32 x3, s32 y3)
{
    u32 address = (u32)fileioWorkBuffer + 0x10000;
    u8 wide = movieRgb24;

    *(u32 *)(object + 0) = address;
    address += 0x12c00;
    *(u32 *)(object + 4) = address;
    address += 0x12c00;
    *(u32 *)(object + 8) = 0;
    *(u32 *)(object + 12) = address;
    *(s32 *)(object + 32) = mode ^ 1;
    *(u32 *)(object + 44) = 0;
    *(s16 *)(object + 16) = x;
    *(s16 *)(object + 18) = y;
    *(s16 *)(object + 20) = x3;
    *(s16 *)(object + 22) = y3;
    *(s16 *)(object + 24) = x2;
    *(s16 *)(object + 26) = y2;
    *(s16 *)(object + 28) = x3;
    *(s16 *)(object + 30) = y3;
    if (wide != 0) {
        *(s16 *)(object + 36) = x;
        *(s16 *)(object + 38) = y;
        *(s16 *)(object + 40) = 24;
        *(s16 *)(object + 42) = y3;
    } else {
        *(s16 *)(object + 36) = x;
        *(s16 *)(object + 38) = y;
        *(s16 *)(object + 40) = 16;
        *(s16 *)(object + 42) = y3;
    }
}
