/* sdk_extra_snd: PsyQ SDK LIBSND functions IDA did not list
 * (config/US/extra_funcs.tsv). All are unused by TM2.
 *   80157CDC Snd_nextpause, 801583A4 SsSetMono, 8015BA5C SpuVmGetSeqLVol,
 *   8015BAB0 SpuVmGetSeqRVol, 8015CF20 SsUtGetDetVVol, 8015CF78 SsUtSetDetVVol,
 *   8015CFEC SsUtGetVVol
 *
 * Adapted from:
 *  - the LSD: Dream Emulator decompilation (CC0 1.0):
 *    src/psyq/libsnd_ut_cp_ut_cadsr_ut_vvol_ut_autov_ut_autop.c
 *    (SsUtGetDetVVol, SsUtSetDetVVol, SsUtGetVVol) and
 *    src/psyq/libsnd_vmanager.c (SpuVmGetSeqLVol, SpuVmGetSeqRVol).
 *    SPDX-License-Identifier: CC0-1.0
 *  - psy-q-decomp (MIT): src/snd/pause.c (Snd_nextpause, VERSION 33),
 *    src/snd/vs_mono.c (SsSetMono).  SPDX-License-Identifier: MIT
 *
 * Recipe: GCC 2.5.7 cc1 (like sdk_libsnd*.c), aspsx 2.21, gp_size 0,
 * no -fvolatile, no extern_gp.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): TM2_SPU_VOICES is 24, or 48 with --port-define TM2_PORT_SPU48. */
#include "semantic/spu_voices.h"

/* sequence score (one per SEQ/SEP track, 0xAC bytes; see sdk_libsnd.c) */
struct SeqStruct {
    u8 pad00[0x2B];
    u8 unk2b;           /* 0x2B */
    u8 pad2C[0x74 - 0x2C];
    u16 volL;           /* 0x74 */
    u16 volR;           /* 0x76 */
    u8 pad78[0x90 - 0x78];
    s32 unk90;          /* 0x90 flags */
    u8 pad94[0xAC - 0x94];
};

/* SPU voice registers (SPU block / its shadow copy, 16 bytes per voice) */
typedef struct SvmSreg {
    s16 volL;
    s16 volR;
    u16 pitch;
    u16 addr;
    s16 adsr1;
    s16 adsr2;
    u8 padC[4];
} SvmSreg;

extern struct SeqStruct *D_801FA3C4[]; /* _ss_score */
extern s16 D_801AB93C;                 /* _svm_stereo_mono */
extern s16 D_801B2D66;                 /* _svm_cur score index */
extern SvmSreg *D_8017F04C;            /* _svm_sreg (SPU voice registers) */
extern SvmSreg D_801A5200[];           /* _svm_sreg_buf */
extern u8 D_801A5380[];                /* _svm_sreg_dirty */

/* Snd_nextpause: stop the score at the end of the current SEP. */
void __attribute__((section(".text_80157CDC"))) sub_80157CDC(s16 arg0, s16 arg1)
{
    D_801FA3C4[arg0][arg1].unk2b = 0;
    D_801FA3C4[arg0][arg1].unk90 &= ~0x100;
}

/* SsSetMono: mono output. */
void __attribute__((section(".text_801583A4"))) sub_801583A4(void)
{
    D_801AB93C = 1;
}

/* SpuVmGetSeqLVol: left volume of a sequence (access | seq << 8). */
s32 __attribute__((section(".text_8015BA5C"))) sub_8015BA5C(s32 seqSepNo)
{
    struct SeqStruct *e = &D_801FA3C4[seqSepNo & 0xFF][(seqSepNo & 0xFF00) >> 8];

    D_801B2D66 = seqSepNo;
    return (s16)e->volL;
}

/* SpuVmGetSeqRVol: right volume of a sequence (access | seq << 8). */
s32 __attribute__((section(".text_8015BAB0"))) sub_8015BAB0(s32 seqSepNo)
{
    struct SeqStruct *e = &D_801FA3C4[seqSepNo & 0xFF][(seqSepNo & 0xFF00) >> 8];

    D_801B2D66 = seqSepNo;
    return (s16)e->volR;
}

/* SsUtGetDetVVol: a voice's raw left/right volume from the SPU registers. */
s16 __attribute__((section(".text_8015CF20"))) sub_8015CF20(s16 idx, s16 *out1, s16 *out2)
{
    if ((u16)idx < 0x18) {
        *out1 = (D_8017F04C + idx)->volL;
        *out2 = (D_8017F04C + idx)->volR;
        return 0;
    }
#if TM2_SPU_VOICES > 24
    if ((u16)idx < TM2_SPU_VOICES) {  /* voices 24..47: their block */
        *out1 = ((SvmSreg *)TM2_SPU_EXT_REGS + (idx - 24))->volL;
        *out2 = ((SvmSreg *)TM2_SPU_EXT_REGS + (idx - 24))->volR;
        return 0;
    }
#endif
    return -1;
}

/* SsUtSetDetVVol: write a voice's raw left/right volume to the shadow
 * registers and mark them dirty. */
s16 __attribute__((section(".text_8015CF78"))) sub_8015CF78(s16 idx, s16 p1, s16 p2)
{
    s32 unused[2];

    if ((u16)idx < TM2_SPU_VOICES) {
        D_801A5200[idx].volR = p2;
        D_801A5380[idx] |= 3;
        D_801A5200[idx].volL = p1;
        return 0;
    }
    return -1;
}

/* SsUtGetVVol: SsUtGetDetVVol in 0..127 units (register value / 129). */
s16 __attribute__((section(".text_8015CFEC"))) sub_8015CFEC(s16 idx, s16 *out1, s16 *out2)
{
    SvmSreg *e;
    s16 f0, f2;

    if ((u16)idx < TM2_SPU_VOICES) {
#if TM2_SPU_VOICES > 24
        e = idx < 24 ? &D_8017F04C[idx] : (SvmSreg *)TM2_SPU_EXT_REGS + (idx - 24);  /* 24..47: their block */
#else
        e = &D_8017F04C[idx];
#endif
        f0 = e->volL;
        f2 = e->volR;
        *out1 = f0 / 129;
        *out2 = f2 / 129;
        return 0;
    }
    return -1;
}
