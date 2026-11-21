/* PsyQ SDK LIBAPI, reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * Root counters (libapi counter.c): SetRCnt, GetRCnt, StartRCnt, StopRCnt,
 * ResetRCnt.
 * Adapted from sotn-decomp src/main/psxsdk/libapi/counter.c
 * SPDX-License-Identifier: MIT
 *
 * The library's data lives at its retail addresses and is declared extern by
 * address (D_XXXXXXXX). */

typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;

typedef struct {
    u16 rootCounter;    /* +0 count */
    s16 unk2;
    s16 mode;           /* +4 */
    s16 : 16;
    s16 target;         /* +8 */
    s32 : 32;
} Counter;              /* 16 bytes per root counter (0x1F801100 + 0x10*n) */

extern volatile s32 *D_8017EC0C;    /* 0x1F801070: I_STAT (I_MASK at +4) */
extern volatile Counter *D_8017EC10;/* 0x1F801100: root counters */
extern s32 D_8017EC14[];            /* interrupt masks {0x10,0x20,0x40,0x01} */

/* @80155444 SetRCnt: program root counter spec's target and mode. */
s32 __attribute__((section(".text_80155444"))) sub_80155444(s32 spec, s16 target, s32 flags)
{
    s32 i = spec & 0xFFFF;
    s32 final_mode = 0x48;
    if (i >= 3) {
        return 0;
    }
    D_8017EC10[i].mode = 0;
    D_8017EC10[i].target = target;

    if (i < 2u) {
        if (flags & 0x10) {
            final_mode = 0x49;
        }
        if (!(flags & 1)) {
            final_mode |= 0x100;
        }
    } else if (i == 2u) {
        if (!(flags & 1)) {
            final_mode = 0x248;
        }
    }
    if ((flags & 0x1000) != 0) {
        final_mode |= 0x10;
    }

    D_8017EC10[i].mode = final_mode;
    return 1;
}

/* @801554E4 GetRCnt: current count of root counter spec (0 if invalid). */
s32 __attribute__((section(".text_801554E4"))) sub_801554E4(s32 spec)
{
    s32 i = spec & 0xFFFF;
    if (i >= 3) {
        return 0;
    }
    return D_8017EC10[i].rootCounter;
}

/* @8015551C StartRCnt: enable root counter spec's interrupt. */
s32 __attribute__((section(".text_8015551C"))) sub_8015551C(s32 spec)
{
    s32 i = spec & 0xFFFF;
    D_8017EC0C[1] |= D_8017EC14[i];
    return i < 3;
}

/* @80155554 StopRCnt: disable root counter spec's interrupt. */
s32 __attribute__((section(".text_80155554"))) sub_80155554(s32 spec)
{
    s32 i = spec & 0xFFFF;
    D_8017EC0C[1] &= ~D_8017EC14[i];
    return 1;
}

/* @8015558C ResetRCnt: zero root counter spec's count. */
s32 __attribute__((section(".text_8015558C"))) sub_8015558C(s32 spec)
{
    s32 i = spec & 0xFFFF;
    if (i >= 3) {
        return 0;
    }
    D_8017EC10[i].rootCounter = 0;
    return 1;
}
