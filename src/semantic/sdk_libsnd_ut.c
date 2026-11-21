/* PsyQ SDK LIBSND leftovers 8015D09C (SsUtSetVVol), 8015D188 (SsUtAutoPan).
 * Adapted from the LSD: Dream Emulator decompilation (CC0 1.0):
 * src/psyq/libsnd_ut_cp_ut_cadsr_ut_vvol_ut_autov_ut_autop.c
 * SPDX-License-Identifier: CC0-1.0
 *
 * Recipe: GCC 2.5.7 cc1 (like sdk_libsnd*.c), aspsx 2.21, gp_size 0,
 * no -fvolatile, no extern_gp:  --compiler gcc2.5.7 --gp-size 0
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): TM2_SPU_VOICES is 24, or 48 with --port-define TM2_PORT_SPU48. */
#include "semantic/spu_voices.h"

/* SsUtSetVVol (libsnd, gcc 2.5.7) */
typedef struct SvmSreg {
    s16 volL;
    s16 volR;
    u16 pitch;
    u16 addr;
    s16 adsr1;
    s16 adsr2;
    u8 padC[4];
} SvmSreg;
extern SvmSreg D_801A5200[];
extern u8 D_801A5380[];
extern void sub_80159FC8(s16 voice, s16 from, s16 to, s16 duration);

s16 __attribute__((section(".text_8015D09C"))) sub_8015D09C(s16 idx, s16 p1, s16 p2)
{
    s32 unused[2];
    s16 t1, t2;

    if ((u16)idx < TM2_SPU_VOICES) {
        t1 = p1 * 129;
        t2 = p2 * 129;
        D_801A5200[idx].volR = t2;
        D_801A5380[idx] |= 3;
        D_801A5200[idx].volL = t1;
        return 0;
    }
    return -1;
}

/* SsUtAutoPan (libsnd, gcc 2.5.7) */
s16 __attribute__((section(".text_8015D188"))) sub_8015D188(s16 voice, s16 from, s16 to, s16 duration)
{
    if ((u16)voice < TM2_SPU_VOICES) {
        sub_80159FC8(voice, from, to, duration);
        return 0;
    }
    return -1;
}

