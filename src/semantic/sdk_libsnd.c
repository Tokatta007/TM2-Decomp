/* PsyQ SDK LIBSND (first half), reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * Adapted from:
 *  - the LSD: Dream Emulator decompilation (CC0 1.0, public domain dedication):
 *    src/psyq/libsnd_ssinit.c (_SsInit), libsnd_cres.c (_SsSndCrescendo),
 *    libsnd_decre.c (_SsSndDecrescendo, reworked), libsnd_vmanager.c
 *    (SpuVmKeyOnNow, vmNoiseOn2) and include/{svm_data.h,ss_score.h} (table
 *    layouts).
 *    SPDX-License-Identifier: CC0-1.0
 *  - sotn-decomp src/main/psxsdk/libsnd (MIT): scssattr.c, scssvol.c,
 *    scsmvol.c, vs_vab.c, vs_vh.c, vs_vtb.c, vs_vtc.c, vs_srv.c, sscall.c,
 *    stop.c, pause.c, replay.c, tempo.c, vmanager.c (SpuVmAlloc,
 *    SpuVmDoAllocate, vmNoiseOn, note2pitch, note2pitch2,
 *    _SsVmSelectToneAndVag).  SPDX-License-Identifier: MIT
 *  - psy-q-decomp (MIT, repository licence): src/snd/ssstart.c (_SsStart,
 *    SsStart, SsStart2, _SsTrapIntrVSync, _SsSeqCalledTbyT_1per2), ssend.c
 *    (SsEnd), sstick.c (SsSetTickMode), vs_vtb.c (SsVabTransBody),
 *    libsnd_i.h / snd_defs.h (score and tick-environment layouts).
 *    SPDX-License-Identifier: MIT
 *  - psyz decomp/src/libsnd/vm_nowon.c (MIT): SpuVmKeyOnNow's shape (the
 *    unused `pad` local, shared temporaries).  SPDX-License-Identifier: MIT
 *  SpuVmKeyOffNow and vmNoiseOff were written from the retail code (no source
 *  carries this build's version).
 *
 * TM2's libsnd is a December 1995 (3.4-era) build compiled by GCC 2.5.7
 * (manifest compiler gcc2.5.7, aspsx 2.21). Some locals exist only to
 * reproduce the original stack frame (`volatile int pad`, unused) or the
 * original register sharing; they do not change behaviour. Where sources
 * disagreed with retail the retail behaviour was followed (e.g.
 * SsVabOpenHeadWithMode decrements the VAB count on every failure after the
 * slot is taken).
 *
 * The library's data lives at its retail addresses and is declared extern by
 * address (D_XXXXXXXX). Second-half functions (sdk_libsnd2.c) and other
 * libraries are called by address name (sub_<VA>).
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): with --port-define TM2_PORT_SPU48 the voice manager runs 48 voices; voices
 * 24..47 have their registers in the second block (TM2_SPU_EXT_REGS) and their key / reverb / envelope bits in
 * portSpuExt. Without it nothing here changes. */
#include "semantic/spu_voices.h"

#pragma tm2_rdata .tm2_800D110C

/* ---- sequence score (one per SEQ/SEP track, 0xAC bytes) ---- */
struct SeqStruct {
    u8 unk0;
    u8 pad1[3];
    u8 *read_pos;
    u8 *next_sep_pos; /* 0x8 */
    u8 *loop_pos;     /* 0xC */
    u8 unk10;
    u8 unk11;
    u8 channel;
    u8 unk13;
    u8 unk14;
    u8 unk15;
    u8 unk16;
    u8 panpot[16];
    u8 unk27;
    u8 unk28;
    u8 unk29;
    u8 unk2a;
    u8 unk2b;
    u8 programs[16];
    u8 unk3C;
    u8 pad3D;
    s16 unk3E;
    s16 unk40;
    s16 unk42;
    s16 unk44;
    s16 unk46;
    s16 unk48;
    s16 unk4a;
    s16 unk4c;
    s16 vol[16];
    s16 unk6E;
    s16 unk70;
    s16 unk72;
    u16 unk74;   /* volL */
    u16 unk76;   /* volR */
    s16 unk78;
    s16 unk7A;
    s32 unk7c;
    u32 unk80;
    s32 unk84;
    s32 delta_value;
    u32 unk8c;   /* tempo */
    s32 unk90;
    s32 unk94;
    s32 unk98;
    s32 unk9C;
    u32 unkA0;   /* tempo ramp ticks left */
    u32 unkA4;
    s16 padA6;
    s16 padaa;
};

typedef void (*SndSsMarkCallbackProc)(short seq_no, short sep_no, short data);

/* ---- <libspu.h> ---- */
typedef struct {
    short left;
    short right;
} SpuVolume;

typedef struct {
    SpuVolume volume;
    long reverb;
    long mix;
} SpuExtAttr;

typedef struct {
    unsigned long mask;
    SpuVolume mvol;
    SpuVolume mvolmode;
    SpuVolume mvolx;
    SpuExtAttr cd;
    SpuExtAttr ext;
} SpuCommonAttr;

#define SPU_COMMON_MVOLL (1 << 0)
#define SPU_COMMON_MVOLR (1 << 1)
#define SPU_COMMON_CDVOLL (1 << 6)
#define SPU_COMMON_CDVOLR (1 << 7)
#define SPU_COMMON_CDREV (1 << 8)
#define SPU_COMMON_CDMIX (1 << 9)
#define SPU_COMMON_EXTVOLL (1 << 10)
#define SPU_COMMON_EXTVOLR (1 << 11)
#define SPU_COMMON_EXTREV (1 << 12)
#define SPU_COMMON_EXTMIX (1 << 13)

/* ---- <libsnd.h> VAB structures ---- */
typedef struct VabHdr {
    s32 form;
    s32 ver;
    s32 id;
    u32 fsize;
    u16 reserved0;
    u16 ps;
    u16 ts;
    u16 vs;
    u8 mvol;
    u8 pan;
    u8 attr1;
    u8 attr2;
    u32 reserved1;
} VabHdr;

typedef struct ProgAtr {
    u8 tones;
    u8 mvol;
    u8 prior;
    u8 mode;
    u8 mpan;
    u8 reserved0;
    s16 attr;
    u32 reserved1;
    u16 reserved2;
    u16 reserved3;
} ProgAtr;

typedef struct VagAtr {
    u8 prior;
    u8 mode;
    u8 vol;
    u8 pan;
    u8 center;
    u8 shift;
    u8 min;
    u8 max;
    u8 vibW;
    u8 vibT;
    u8 porW;
    u8 porT;
    u8 pbmin;
    u8 pbmax;
    u8 reserved1;
    u8 reserved2;
    u16 adsr1;
    u16 adsr2;
    s16 prog;
    s16 vag;
    s16 reserved[4];
} VagAtr;

/* ---- library data (retail addresses) ---- */
/* _snd_seq_tick_env: one struct in Sony's source; declared member by member
 * here so each access is addressed on its own, as in retail. */
extern s32 D_8017F034;          /* tick mode */
extern s32 D_8017F038;          /* no-tick flag */
extern void (*D_8017F03C)();    /* tick function (SsSeqCalledTbyT) */
extern void (*D_8017F040)();    /* saved VSync interrupt callback */
extern u8 D_8017F044;           /* ticking from VSyncCallback */
extern u8 D_8017F045;           /* 1-per-2 divider on */
extern u8 D_8017F046;           /* interrupt id in use */
extern u32 D_8017F048;          /* 1-per-2 toggle */
struct SndSeqTickEnv {
    s32 unk0;
    s32 unk4;
    void (*unk8)();
    void (*unk12)();
    u8 unk16;
    u8 unk17;
    u8 unk18;
    u8 unk19;
    u32 unk20;
};
/* _SsStart addresses the members through the real struct object (shared base
 * registers); the asm label keeps its symbol D_8017F034. */
extern struct SndSeqTickEnv snd_tick_env __asm__("D_8017F034");
#define ENV snd_tick_env
extern u16 D_8017F004[8];                /* _SsInit voice register template */
extern u16 D_8017F014[16];               /* _SsInit control register template */
extern SndSsMarkCallbackProc D_801FA444[32][16]; /* _SsMarkCallback */
extern s32 D_801F8AA0;                   /* VBLANK_MINUS */
extern s32 D_801B2D48;                   /* _snd_openflag */
extern s32 D_801AEB10;                   /* _snd_ev_flag */
extern struct SeqStruct *D_801FA3C4[32]; /* _ss_score */
extern s16 D_801FAC44;                   /* _snd_seq_s_max */
extern s16 D_801FAC4C;                   /* _snd_seq_t_max */

/* ---- other libraries ---- */
extern void *sub_801452F0(void);                        /* ResetCallback */
extern void *sub_80145320(int irq, void (*f)());        /* InterruptCallback */
extern void sub_80145380(void (*f)());                  /* VSyncCallback */
extern int GsGetWorkBase(void);                         /* GetVideoMode */
extern int EnterCriticalSection(void);
extern int ExitCriticalSection(void);
extern s32 sub_80155444(s32 spec, s32 target, s32 mode); /* SetRCnt */
extern s32 sub_8015558C(s32 spec);                      /* ResetRCnt */
extern void sub_8015D408(void);                         /* SpuInit */
extern void sub_8015D4A4(void);                         /* SpuInitHot */

/* ---- libsnd functions ---- */
extern void sub_8015A560(s32 voices);                   /* SpuVmInit */
extern void sub_8015AD64(void);                         /* SpuVmFlush */
extern void sub_80157D40(s16, s16);                     /* _SsSndPlay */
void sub_801570FC(s32 arg0);
void sub_801575F8(s16, s16);
void sub_80157960(s16, s16);
void sub_80157C40(s16, s16);
void sub_80157D6C(s16, s16);
void sub_80157FF8(s16, s16);
void sub_801581BC(s16, s16);
void sub_8015706C(void);
void sub_801570AC(void);

/* _SsInit: resets callbacks, brings up the SPU (cold or hot), writes the
 * voice/control register templates, starts the voice manager for 24 voices,
 * clears the mark callbacks and the sequencer flags. */
void __attribute__((section(".text_80156EF8"))) sub_80156EF8(s32 arg0)
{
    s32 i, j;
    u16 *base;
    u16 *src;
    u16 *reg;

    sub_801452F0();

    if (arg0 == 0) {
        sub_8015D408();
    } else {
        sub_8015D4A4();
    }

    reg = (u16 *)0x1F801C00;
    i = 0;
    base = D_8017F004;
    for (; i < 0x18; i++) {
        j = 0;
        src = base;
        for (; j < 8; j++) {
            *reg = *src;
            src++;
            reg++;
        }
    }

    reg = (u16 *)0x1F801D80;
    i = 0;
    src = D_8017F014;
    for (; i < 0x10; i++) {
        *reg = *src;
        src++;
        reg++;
    }

#if TM2_SPU_VOICES > 24
    /* voices 24..47: the voice template, and the control template's key on / key off / reverb words */
    reg = (u16 *)TM2_SPU_EXT_REGS;
    for (i = 0; i < TM2_SPU_VOICES - 24; i++) {
        for (j = 0; j < 8; j++) {
            reg[i * 8 + j] = D_8017F004[j];
        }
    }
    for (j = 4; j < 8; j++) {
        reg[0xC0 + j] = D_8017F014[j];
    }
    reg[0xCC] = D_8017F014[12];
    reg[0xCD] = D_8017F014[13];
    sub_8015A560(TM2_SPU_VOICES);
#else
    sub_8015A560(0x18);
#endif

    for (j = 0; j < 0x20; j++) {
        for (i = 15; i >= 0; i--) {
            D_801FA444[j][i] = 0;
        }
    }

    D_801F8AA0 = 0x3C;
    D_801B2D48 = 0;
    D_801AEB10 = 0;
}

/* SsInit: cold sound-system init. */
void __attribute__((section(".text_8015700C"))) sub_8015700C(void)
{
    sub_80156EF8(0);
}

/* _SsTrapIntrVSync: VSync interrupt handler; chains the saved callback, then ticks the sequencer. */
void __attribute__((section(".text_8015706C"))) sub_8015706C(void)
{
    if (D_8017F040 != 0) {
        D_8017F040();
    }
    D_8017F03C();
}

/* _SsSeqCalledTbyT_1per2: root-counter handler that ticks the sequencer every second call. */
void __attribute__((section(".text_801570AC"))) sub_801570AC(void)
{
    if (D_8017F048 == 0) {
        D_8017F048 = 1;
        return;
    }
    D_8017F048 = 0;
    D_8017F03C();
}

/* _SsStart: arms the sequencer tick (VSync callback or root counter 2 interrupt) for the tick mode. */
void __attribute__((section(".text_801570FC"))) sub_801570FC(s32 arg0)
{
    u16 rcnt_target;
    s32 wait;
    u32 rcnt_spec;
    u8 temp_a0;

    for (wait = 0; wait < 3000; wait++) {
        wait += 2;
    }
    ENV.unk16 = 0;
    ENV.unk17 = 0;
    ENV.unk12 = 0;
    ENV.unk18 = 6;
    rcnt_spec = 0xF2000002;
    switch (ENV.unk0) {
    case 0:
        ENV.unk18 = 0xFF;
        return;

    case 5:
        ENV.unk18 = 0;
        if (arg0 == 0) {
            ENV.unk16 = 1;
        } else {
            rcnt_spec = 0xF2000003;
            rcnt_target = 1;
        }
        break;

    case 3:
        rcnt_target = 0x89D0;
        break;

    case 2:
        rcnt_target = 0x44E8;
        break;

    default:
        if (ENV.unk4 == 0) {
            if (ENV.unk0 < 0x46) {
                rcnt_target = 0x204CC0 / ENV.unk0;
                ENV.unk17++;
            } else {
                rcnt_target = 0x409980 / ENV.unk0;
            }
        } else {
            return;
        }
        break;
    }

    if (ENV.unk16 != 0) {
        EnterCriticalSection();
        sub_80145380(ENV.unk8);
    } else {
        EnterCriticalSection();
        sub_8015558C(rcnt_spec);
        sub_80155444(rcnt_spec, rcnt_target, 0x1000);
        temp_a0 = ENV.unk18;
        if (temp_a0 == 0) {
            ENV.unk12 = sub_80145320(0, 0);
            sub_80145320(ENV.unk18, sub_8015706C);
        } else if (ENV.unk17 == 0) {
            sub_80145320(temp_a0, ENV.unk8);
        } else {
            sub_80145320(temp_a0, sub_801570AC);
        }
    }
    ExitCriticalSection();
}

/* SsStart: start the sequencer tick (root counter in VSync mode). */
void __attribute__((section(".text_80157380"))) sub_80157380(void)
{
    sub_801570FC(1);
}

/* SsStart2: start the sequencer tick (VSyncCallback in VSync mode). */
void __attribute__((section(".text_801573A0"))) sub_801573A0(void)
{
    sub_801570FC(0);
}

/* sscall.c calls the _SsSnd* handlers without a prototype (int arguments) */
#define KR(f) ((void (*)())(f))

/* SsSeqCalledTbyT: one sequencer tick: flush the voice manager, then play,
 * fade, pause/replay/stop every open sequence according to its flags. */
void __attribute__((section(".text_801573C0"))) sub_801573C0(void)
{
    int i;
    int j;
    if (D_801AEB10 != 1) {
        D_801AEB10 = 1;

        sub_8015AD64();

        for (i = 0; i < D_801FAC44; i++) {
            if ((1 << i) & D_801B2D48) {
                for (j = 0; j < D_801FAC4C; j++) {
                    if (D_801FA3C4[i][j].unk90 & 1) {
                        KR(sub_80157D40)(i, j);
                        if (D_801FA3C4[i][j].unk90 & 0x10) {
                            KR(sub_801575F8)(i, j);
                        }
                        if (D_801FA3C4[i][j].unk90 & 0x20) {
                            KR(sub_80157960)(i, j);
                        }
                        if (D_801FA3C4[i][j].unk90 & 0x40) {
                            KR(sub_801581BC)(i, j);
                        }
                        if (D_801FA3C4[i][j].unk90 & 0x80) {
                            KR(sub_801581BC)(i, j);
                        }
                    }
                    if (D_801FA3C4[i][j].unk90 & 2) {
                        KR(sub_80157C40)(i, j);
                    }
                    if (D_801FA3C4[i][j].unk90 & 8) {
                        KR(sub_80157D6C)(i, j);
                    }
                    if (D_801FA3C4[i][j].unk90 & 4) {
                        KR(sub_80157FF8)(i, j);
                        D_801FA3C4[i][j].unk90 = 0;
                    }
                }
            }
        }
        D_801AEB10 = 0;
    }
}

/* SsSetTickMode: set the sequencer tick mode and VBLANK_MINUS (ticks per second). */
void __attribute__((section(".text_80157DD4"))) sub_80157DD4(long tick_mode)
{
    s32 videoMode;
    s32 var_v0;

    videoMode = GsGetWorkBase();
    if (tick_mode & 0x1000) {
        D_8017F038 = 1;
        D_8017F034 = tick_mode & 0xFFF;
    } else {
        D_8017F038 = 0;
        D_8017F034 = tick_mode;
    }
    if (D_8017F034 < 6) {
        switch (D_8017F034) {
        case 4:
            D_801F8AA0 = 0x32;
            if (videoMode != 1) {
                D_8017F034 = 0x32;
                return;
            }
            D_8017F034 = 5;
            return;
        case 1:
            var_v0 = 0x3C;
            D_801F8AA0 = 0x3C;
            if (videoMode == 0) {
                var_v0 = 5;
            }
            D_8017F034 = var_v0;
            return;
        case 3:
            D_801F8AA0 = 0x78;
            return;
        case 2:
            D_801F8AA0 = 0xF0;
            return;
        case 5:
            switch (videoMode) {
            case 1:
                D_801F8AA0 = 0x32;
                return;
            default:
            case 0:
                D_801F8AA0 = 0x3C;
                return;
            }
            break;
        case 0:
            if (videoMode != 0) {
                if (videoMode == 1) {
                    D_801F8AA0 = 0x32;
                    return;
                }
            }
            D_801F8AA0 = 0x3C;
            return;
        default:
            D_801F8AA0 = 0x3C;
            return;
        }
    } else {
        D_801F8AA0 = D_8017F034;
    }
}

/* SsEnd: stop the sequencer tick, restoring the interrupt callbacks. */
void __attribute__((section(".text_80157F48"))) sub_80157F48(void)
{
    if (D_8017F038 == 0) {
        D_8017F045 = 0;
        EnterCriticalSection();
        if (D_8017F044 != 0) {
            sub_80145380(0);
            D_8017F044 = 0;
        } else if (D_8017F046 == 0) {
            sub_80145320(0, D_8017F040);
            D_8017F040 = 0;
        } else {
            sub_80145320(6, 0);
        }
        ExitCriticalSection();
        D_8017F046 = 0xFF;
    }
}
/* ---- voice manager VAB table (vm_g.c) ---- */
extern u8 D_801B2D70[16];       /* _svm_vab_used */
extern s32 D_801FACB0[16];      /* _svm_vab_start */
extern u16 D_801FACA8;          /* _svm_vab_count */
extern u8 *D_801FACF4[16];      /* _svm_brr_start_addr */
extern s32 D_801FAC68[16];      /* _svm_vab_total */
extern u8 D_801B2D10;           /* spuVmMaxVoice */

/* ---- libspu ---- */
extern void sub_801482CC(SpuCommonAttr *attr);   /* SpuSetCommonAttr */
extern void sub_8014808C(s32 addr);              /* SpuFree */
extern s32 sub_801481A4(s32 mode);               /* SpuSetTransferMode */
extern u32 sub_80148168(u32 addr);               /* SpuSetTransferStartAddr */
extern u32 sub_80148108(u8 *addr, u32 size);     /* SpuWrite */
extern void sub_80148290(s32 flag);              /* _spu_setInTransfer */
extern s32 sub_801481D8(s32 flag);               /* SpuIsTransferCompleted */

s16 sub_8014621C(u8 *addr, s16 vabid);
s16 sub_801466E0(u8 *addr, s16 vabid);
s16 sub_801462AC(u8 *addr, s16 vabid, s16 mode, u32 sbaddr);

/* SsSetSerialAttr: CD / external input reverb or mix switch. */
void __attribute__((section(".text_80145EF8"))) sub_80145EF8(char s_num, char attr, char mode)
{
    SpuCommonAttr c_attr;

    if (s_num == 0 && attr == 0) {
        c_attr.mask = SPU_COMMON_CDMIX;
        c_attr.cd.mix = mode;
    }
    if (s_num == 0 && attr == 1) {
        c_attr.mask = SPU_COMMON_CDREV;
        c_attr.cd.reverb = mode;
    }
    if (s_num == 1 && attr == 0) {
        c_attr.mask = SPU_COMMON_EXTMIX;
        c_attr.ext.mix = mode;
    }
    if (s_num == 1 && attr == 1) {
        c_attr.mask = SPU_COMMON_EXTREV;
        c_attr.ext.reverb = mode;
    }
    sub_801482CC(&c_attr);
}

/* SsSetSerialVol: CD / external input volume (0-127, scaled to 0-0x7FFF). */
void __attribute__((section(".text_80145F8C"))) sub_80145F8C(char s_num, short voll, short volr)
{
    SpuCommonAttr attr;

    if (s_num == 0) {
        attr.mask = SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
        if (voll > 0x7F) {
            voll = 0x7F;
        }
        if (volr > 0x7F) {
            volr = 0x7F;
        }
        attr.cd.volume.left = (voll * 0x7FFF) / 0x7F;
        attr.cd.volume.right = (volr * 0x7FFF) / 0x7F;
    }
    if (s_num == 1) {
        attr.mask = SPU_COMMON_EXTVOLL | SPU_COMMON_EXTVOLR;
        if (voll > 0x7F) {
            voll = 0x7F;
        }
        if (volr > 0x7F) {
            volr = 0x7F;
        }
        attr.ext.volume.left = (voll * 0x7FFF) / 0x7F;
        attr.ext.volume.right = (volr * 0x7FFF) / 0x7F;
    }
    sub_801482CC(&attr);
}

/* SsVabClose: free a VAB's sound buffer area and its slot. */
void __attribute__((section(".text_80146134"))) sub_80146134(s16 vabid)
{
    if ((vabid >= 0 && vabid < 0x10) && (D_801B2D70[vabid] == 1)) {
        sub_8014808C(D_801FACB0[vabid]);
        D_801B2D70[vabid] = 0;
        D_801FACA8 -= 1;
    }
}

/* SsVabOpen: open a VAB header and transfer its body (packed .VAB in memory). */
s16 __attribute__((section(".text_801461C0"))) sub_801461C0(u8 *addr, VabHdr *vab_header)
{
    s16 vab_id;

    vab_id = sub_8014621C(addr, -1);
    if (vab_id != -1) {
        vab_id = sub_801466E0(D_801FACF4[vab_id], vab_id);
    }
    return vab_id;
}

/* SsVabOpenHead: open a VAB header, allocating sound buffer memory. */
s16 __attribute__((section(".text_8014621C"))) sub_8014621C(u8 *addr, s16 vabid)
{
    return sub_801462AC(addr, vabid, 0, 0);
}

/* SsVabTransBody: transfer a VAB body (VB) to the sound buffer. */
s16 __attribute__((section(".text_801466E0"))) sub_801466E0(u8 *addr, s16 vabid)
{
    s32 start;

    if (vabid >= 0 && vabid < 0x11) {
        if (D_801B2D70[vabid] == 2) {
            start = D_801FACB0[vabid];
            sub_801481A4(0);
            sub_80148168(start);
            sub_80148108(addr, D_801FAC68[vabid]);
            D_801B2D70[vabid] = 1;
            return vabid;
        }
    }
    sub_80148290(0);
    return -1;
}

/* SsVabTransCompleted: poll / wait for the VAB body transfer. */
s16 __attribute__((section(".text_801467A8"))) sub_801467A8(s16 immediateFlag)
{
    return sub_801481D8(immediateFlag);
}

/* SsSetMVol: master volume (0-127 each side). */
void __attribute__((section(".text_80156EA8"))) sub_80156EA8(short voll, short volr)
{
    SpuCommonAttr attr;

    attr.mask = SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;
    attr.mvol.left = 0x81 * voll;
    attr.mvol.right = 0x81 * volr;
    sub_801482CC(&attr);
}

/* SsSeqStop: stop a SEQ. */
void __attribute__((section(".text_80158168"))) sub_80158168(short seq_access_num)
{
    sub_80157FF8(seq_access_num, 0);
}

/* SsSepStop: stop one SEQ of a SEP. */
void __attribute__((section(".text_80158190"))) sub_80158190(short sep_access_num, short seq_num)
{
    sub_80157FF8(sep_access_num, seq_num);
}

/* SsSetReservedVoice: number of voices libsnd may allocate (1-24; 1-48 with TM2_PORT_SPU48). */
char __attribute__((section(".text_801583C8"))) sub_801583C8(char voices)
{
#if TM2_SPU_VOICES > 24
    if ((voices > TM2_SPU_VOICES) || (voices == 0)) {
#else
    if ((voices >= 0x19) || (voices == 0)) {
#endif
        return -1;
    }
    D_801B2D10 = voices;
    return D_801B2D10;
}

extern s32 sub_8015B9EC(); /* SpuVmGetSeqVol(seqSepNo, &volL, &volR): called unprototyped */
extern s32 sub_8015B86C(); /* SpuVmSetSeqVol(seqSepNo, volL, volR, 0): called unprototyped */
extern s32 sub_8015BB04(s32 seqSepNo);                             /* SpuVmSeqKeyOff */

/* _SsSndCrescendo: one tick of a sequence volume fade-in (flag 0x10). */
void __attribute__((section(".text_801575F8"))) sub_801575F8(s16 a0, s16 a1)
{
    struct SeqStruct **arr;
    struct SeqStruct *entry;
    s16 thresh;
    u16 sp10;
    u16 sp12;

    entry = &D_801FA3C4[a0][a1];
    arr = &D_801FA3C4[a0];
    entry->unk98 = entry->unk98 - 1;
    if (entry->unk42 > 0) {
        if ((u32)entry->unk98 % (u32)entry->unk42 != 0) {
            goto end;
        }
        if (entry->unk3E > 0) {
            entry->unk40 = entry->unk40 - 1;
            if (entry->unk40 >= 0) {
                sub_8015B9EC((a1 << 8) | a0, (s16 *)&sp10, (s16 *)&sp12);
                if ((sp10 + 1) < 0x80 && (sp12 + 1) < 0x80) {
                    sub_8015B86C((a1 << 8) | a0, sp10 + 1, sp12 + 1, 0);
                } else {
                    sub_8015B86C((a1 << 8) | a0, 0x7F, 0x7F, 0);
                    D_801FA3C4[a0][a1].unk90 &= ~0x10;
                }
            } else {
                sub_8015B86C((a1 << 8) | a0, 0x7F, 0x7F, 0);
                (*arr)[a1].unk90 &= ~0x10;
            }
        }
    } else {
        if (entry->unk3E > 0) {
            entry->unk40 = entry->unk40 + entry->unk42;
            sub_8015B9EC((a1 << 8) | a0, (s16 *)&sp10, (s16 *)&sp12);
            if (entry->unk40 >= 0) {
                thresh = entry->unk42;
                if ((sp10 - thresh) < 0x80 && (sp12 - thresh) < 0x80) {
                    sub_8015B86C((a1 << 8) | a0, sp10 - thresh, sp12 - thresh, 0);
                } else {
                    sub_8015B86C((a1 << 8) | a0, 0x7F, 0x7F, 0);
                    D_801FA3C4[a0][a1].unk90 &= ~0x10;
                }
            } else {
                sub_8015B86C((a1 << 8) | a0, 0x7F, 0x7F, 0);
                (*arr)[a1].unk90 &= ~0x10;
            }
        }
    }
    if (entry->unk98 == 0 || entry->unk40 == 0) {
        D_801FA3C4[a0][a1].unk90 &= ~0x10;
    }
end:
    sub_8015B9EC((a1 << 8) | a0, &entry->unk78, &entry->unk7A);
}

/* _SsSndDecrescendo: one tick of a sequence volume fade-out (flag 0x20). */
void __attribute__((section(".text_80157960"))) sub_80157960(s16 a0, s16 a1)
{
    struct SeqStruct **arr;
    struct SeqStruct *entry;
    s16 thresh;
    u16 sp10;
    u16 sp12;

    entry = &D_801FA3C4[a0][a1];
    arr = &D_801FA3C4[a0];
    entry->unk98 = entry->unk98 - 1;
    if (entry->unk42 > 0) {
        if ((u32)entry->unk98 % (u32)entry->unk42 != 0) {
            goto end;
        }
        if (entry->unk3E > 0) {
            entry->unk40 = entry->unk40 - 1;
            if (entry->unk40 >= 0) {
                sub_8015B9EC((a1 << 8) | a0, (s16 *)&sp10, (s16 *)&sp12);
                if (sp10 != 0 && sp12 != 0) {
                    sub_8015B86C((a1 << 8) | a0, sp10 - 1, sp12 - 1, 0);
                } else {
                    sub_8015B86C((a1 << 8) | a0, 0, 0, 0);
                    D_801FA3C4[a0][a1].unk90 &= ~0x20;
                }
            } else {
                sub_8015B86C((a1 << 8) | a0, 0, 0, 0);
                (*arr)[a1].unk90 &= ~0x20;
            }
        }
    } else {
        if (entry->unk3E > 0) {
            entry->unk40 = entry->unk40 + entry->unk42;
            if (entry->unk40 >= 0) {
                sub_8015B9EC((a1 << 8) | a0, (s16 *)&sp10, (s16 *)&sp12);
                thresh = entry->unk42;
                if (sp10 >= -thresh && sp12 >= -thresh) {
                    sub_8015B86C((a1 << 8) | a0, sp10 + thresh, sp12 + thresh, 0);
                } else {
                    sub_8015B86C((a1 << 8) | a0, 0, 0, 0);
                    D_801FA3C4[a0][a1].unk90 &= ~0x20;
                }
            } else {
                sub_8015B86C((a1 << 8) | a0, 0, 0, 0);
                (*arr)[a1].unk90 &= ~0x20;
            }
        }
    }
    if (entry->unk98 == 0 || entry->unk40 == 0) {
        D_801FA3C4[a0][a1].unk90 &= ~0x20;
    }
end:
    sub_8015B9EC((a1 << 8) | a0, &entry->unk78, &entry->unk7A);
}

/* _SsSndPause: key off a sequence's voices and clear its pause request (flag 2). */
void __attribute__((section(".text_80157C40"))) sub_80157C40(s16 arg0, s16 arg1)
{
    struct SeqStruct *score = &D_801FA3C4[arg0][arg1];

    sub_8015BB04(arg1 << 8 | arg0);
    score->unk2b = 0;
    D_801FA3C4[arg0][arg1].unk90 &= ~2;
}

/* _SsSndReplay: resume a paused sequence (clears flag 8). */
void __attribute__((section(".text_80157D6C"))) sub_80157D6C(s16 arg0, s16 arg1)
{
    D_801FA3C4[arg0][arg1].unk2b = 1;
    D_801FA3C4[arg0][arg1].unk90 &= ~8;
}

/* _SsSndStop: stop a sequence: key its voices off and rewind it to the start. */
void __attribute__((section(".text_80157FF8"))) sub_80157FF8(s16 arg0, s16 arg1)
{
    struct SeqStruct *score = &D_801FA3C4[arg0][arg1];
    int i;

    D_801FA3C4[arg0][arg1].unk90 &= ~1;
    D_801FA3C4[arg0][arg1].unk90 &= ~2;
    D_801FA3C4[arg0][arg1].unk90 &= ~8;
    D_801FA3C4[arg0][arg1].unk90 |= 4;
    sub_8015BB04(arg1 << 8 | arg0);
    score->unk2b = 0;
    score->unk80 = 0;
    score->unk27 = 0;
    score->unk13 = 0;
    score->unk14 = 0;
    score->unk29 = 0;
    score->unk15 = 0;
    score->unk16 = 0;
    score->unk2a = 0;
    score->channel = 0;
    score->unk48 = 0;
    score->unk27 = 0;
    score->unk28 = 0;
    score->unk10 = 0;
    score->unk11 = 0;
    score->delta_value = score->unk7c;
    score->unk8c = score->unk84;
    score->unk70 = score->unk72;
    score->read_pos = score->next_sep_pos;
    score->loop_pos = score->next_sep_pos;
    for (i = 0; i < 16; i++) {
        score->programs[i] = i;
        score->panpot[i] = 0x40;
        score->vol[i] = 127;
    }

    score->unk78 = 0x7F;
    score->unk7A = 0x7F;
}

/* _SsSndTempo: one tick of a tempo ramp (flags 0x40/0x80). */
void __attribute__((section(".text_801581BC"))) sub_801581BC(s16 arg0, s16 arg1)
{
    struct SeqStruct *pSeq = &D_801FA3C4[arg0][arg1];

    pSeq->unkA0--;

    if (pSeq->unk44 > 0) {
        if ((pSeq->unkA0 % pSeq->unk44) == 0) {
            if (pSeq->unk8c > pSeq->unkA4) {
                pSeq->unk8c--;
            } else if (pSeq->unk8c < pSeq->unkA4) {
                pSeq->unk8c++;
            }

            pSeq->unk70 = (pSeq->unk4a * pSeq->unk8c * 10) / (D_801F8AA0 * 60);
            if (pSeq->unk70 <= 0) {
                pSeq->unk70 = 1;
            }
            if ((pSeq->unkA0 == 0) || (pSeq->unk8c == pSeq->unkA4)) {
                D_801FA3C4[arg0][arg1].unk90 &= ~0x40;
                D_801FA3C4[arg0][arg1].unk90 &= ~0x80;
            }
        }
    } else {
        if (pSeq->unk8c > pSeq->unkA4) {
            pSeq->unk8c += pSeq->unk44;
            if (pSeq->unk8c < pSeq->unkA4) {
                pSeq->unk8c = pSeq->unkA4;
            }
        } else if (pSeq->unk8c < pSeq->unkA4) {
            pSeq->unk8c -= pSeq->unk44;
            if (pSeq->unk8c > pSeq->unkA4) {
                pSeq->unk8c = pSeq->unkA4;
            }
        }

        pSeq->unk70 = (pSeq->unk4a * pSeq->unk8c * 10) / (D_801F8AA0 * 60);
        if (pSeq->unk70 <= 0) {
            pSeq->unk70 = 1;
        }
        if ((pSeq->unkA0 == 0) || (pSeq->unk8c == pSeq->unkA4)) {
            D_801FA3C4[arg0][arg1].unk90 &= ~0x40;
            D_801FA3C4[arg0][arg1].unk90 &= ~0x80;
        }
    }
}

/* ---- voice manager tables (LSD svm_data.h) ---- */
typedef struct SvmVoice {
    s16 vag;      /* +0x00 */
    u16 age;      /* +0x02 */
    s16 pitch;    /* +0x04 */
    u16 envx;     /* +0x06 */
    s16 vol;      /* +0x08 */
    u8 pan;       /* +0x0A */
    u8 pad0B;
    s16 note;     /* +0x0C */
    s16 seq;      /* +0x0E */
    s16 progIndex; /* +0x10 */
    s16 prog;     /* +0x12 */
    s16 tone;     /* +0x14 */
    s16 vabId;    /* +0x16 */
    s16 prior;    /* +0x18 */
    u8 pad1A;
    u8 keyState;  /* +0x1B */
    s16 autoVol[6]; /* +0x1C */
    s16 autoPan[6]; /* +0x28 */
} SvmVoice; /* 0x34 */

typedef struct SvmSreg {
    s16 volL;  /* +0x0 */
    s16 volR;  /* +0x2 */
    u16 pitch; /* +0x4 */
    u16 addr;  /* +0x6 */
    s16 adsr1; /* +0x8 */
    s16 adsr2; /* +0xA */
    u8 padC[4];
} SvmSreg; /* 0x10 */

typedef struct {
    s16 volL;
    s16 volR;
    s16 pitch;
    s16 addr;
    s16 adsr1;
    s16 adsr2;
    u16 envx;
    u8 padE[2];
} SpuVoiceRegs;

typedef struct {
    SpuVoiceRegs voice[24];
    u8 pad180[8];
    u16 keyOn[2];    /* +0x188 */
    u16 keyOff[2];   /* +0x18C */
    u8 pad190[4];
    u16 noiseOn[2];  /* +0x194 */
    u16 reverbOn[2]; /* +0x198 */
} SpuRegs;

/* _svm_cur (0x801B2D50): the key-on request being worked on */
struct SvmCur {
    u8 tones;         /* +0x00 tones in the program */
    u8 vabId;         /* +0x01 */
    u8 note;          /* +0x02 */
    u8 fine;          /* +0x03 */
    u8 vol;           /* +0x04 key-on velocity */
    u8 pan;           /* +0x05 key-on pan */
    u8 progNo;        /* +0x06 */
    u8 prog;          /* +0x07 program index */
    u8 unk08;
    u8 unk09;
    u8 mvol;          /* +0x0A program volume */
    u8 mpan;          /* +0x0B program pan */
    u8 tone;          /* +0x0C */
    u8 tvol;          /* +0x0D tone volume */
    u8 tpan;          /* +0x0E tone pan */
    u8 prior;         /* +0x0F */
    u8 center;        /* +0x10 */
    u8 shift;         /* +0x11 */
    u8 unk12;
    u8 unk13;
    u8 mode;          /* +0x14 tone mode (bit 2: reverb) */
    u8 unk15;
    u16 seqSepNo;     /* +0x16 */
    u16 vagIdx;       /* +0x18 the tone's VAG number */
    s16 voice;        /* +0x1A */
    s16 sregIdx;      /* +0x1C voice * 8, index of its register shadow */
    s16 toneIdx;      /* +0x1E prog * 16 + tone */
};
extern struct SvmCur D_801B2D50; /* _svm_cur */

extern SvmVoice D_801A5398[];   /* _svm_voice (24; 48 with TM2_PORT_SPU48, moved to .port_bss) */
extern SvmSreg D_801A5200[];    /* _svm_sreg_buf (24; 48 with TM2_PORT_SPU48) */
extern u8 D_801A5380[];         /* _svm_sreg_dirty (24; 48 with TM2_PORT_SPU48) */
extern SpuRegs *D_8017F04C;     /* _svm_sreg (0x1F801C00) */
extern u16 D_8017F050[];        /* pitch table: 12 semitones x 16 fine steps at octave 5 */
extern VagAtr *D_801B2B1C;      /* _svm_tn */

/* vmNoiseOff: take a voice off the noise generator. */
void __attribute__((section(".text_801595E4"))) sub_801595E4(u8 voice)
{
    D_801A5398[voice].keyState = 0;
    D_801A5398[voice].pitch = 0;
    D_8017F04C->noiseOn[0] = 0;
    D_8017F04C->noiseOn[1] = 0;
}

/* note2pitch: SPU pitch of _svm_cur's note on its tone (centre note, fine shift). */
s32 __attribute__((section(".text_80159630"))) sub_80159630(void)
{
    s32 semitones;
    s32 octave;
    s32 semitone;
    u16 step;
    u16 pitch;
    s32 sh;

    semitones = D_801B2D50.note - (D_801B2D50.center - 60);
    step = D_801B2D50.shift / 8;
    octave = semitones / 12;
    semitone = semitones % 12;
    if (step >= 16) {
        step = 15;
    }
    semitone = semitone * 16;
    pitch = D_8017F050[semitone + step];
    sh = octave - 5;
    if (sh > 0) {
        pitch <<= sh;
    } else if (sh < 0) {
        pitch = pitch >> -sh;
    }
    return pitch;
}

/* note2pitch2: SPU pitch of `note` with fine tune `fine` (0-127) on _svm_cur's tone. */
s32 __attribute__((section(".text_801596EC"))) sub_801596EC(u16 note, u16 fine)
{
    s16 octave;
    s16 step;
    s16 carry;
    s16 semitones;
    u16 pitch;
    s32 pos;
    s32 tone;

    tone = D_801B2D50.tone + (D_801B2D50.prog * 16);
    step = (fine + D_801B2B1C[tone].shift) / 8;
    carry = 0;
    if (step >= 16) {
        carry = 1;
        step -= 16;
    }
    semitones = note + 60 - D_801B2B1C[tone].center + carry;
    octave = semitones / 12;
    pos = (semitones % 12) * 16;
    pitch = D_8017F050[pos + step];
    octave -= 5;
    if (octave > 0) {
        pitch <<= octave;
    } else if (octave < 0) {
        pitch >>= -octave;
    }
    return pitch;
}

extern u16 D_801FAD38;          /* _svm_okof1 */
extern u16 D_801FAD40;          /* _svm_okof2 */
extern u16 D_801A5AE4;          /* _svm_okon1 */
extern u16 D_801A5AEC;          /* _svm_okon2 */

/* SpuVmKeyOffNow: release _svm_cur's voice at the next flush and free it. */
void __attribute__((section(".text_80158B90"))) sub_80158B90(void)
{
    u16 voice;
    s32 lowBit;
    s32 highBit;

    voice = D_801B2D50.voice;
#if TM2_SPU_VOICES > 24
    if (voice >= 24) {  /* voices 24..47: their words in portSpuExt */
        s32 w = (voice - 24) >> 4;

        D_801A5398[voice].keyState = 0;
        D_801A5398[voice].pitch = 0;
        D_801A5398[voice].vag = 0;
        portSpuExt.okof[w] |= 1 << ((voice - 24) & 15);
        portSpuExt.okon[0] &= ~portSpuExt.okof[0];
        portSpuExt.okon[1] &= ~portSpuExt.okof[1];
        return;
    }
#endif
    if (voice < 16) {
        lowBit = 1 << voice;
        highBit = 0;
    } else {
        lowBit = 0;
        highBit = 1 << (voice - 16);
    }
    D_801A5398[voice].keyState = 0;
    D_801A5398[voice].pitch = 0;
    D_801A5398[voice].vag = 0;
    D_801FAD38 = lowBit | D_801FAD38;
    D_801FAD40 = highBit | D_801FAD40;
    D_801A5AE4 = D_801A5AE4 & ~D_801FAD38;
    D_801A5AEC = D_801A5AEC & ~D_801FAD40;
}

/* SpuVmSelectToneAndVag: list the tones of _svm_cur's program whose key range
 * holds _svm_cur's note (tone index into toneList, VAG number into vagList). */
u8 __attribute__((section(".text_80158C6C"))) sub_80158C6C(u8 *toneList, u8 *vagList)
{
    u8 i;
    u8 count;
    VagAtr *tn;

    count = 0;
    for (i = 0; i < D_801B2D50.tones; i++) {
        tn = &D_801B2B1C[(D_801B2D50.prog * 16) + i];
        if (tn->min > D_801B2D50.note || D_801B2D50.note > tn->max) {
            continue;
        }
        vagList[count] = tn->vag;
        toneList[count++] = i;
    }
    return count;
}

/* vmNoiseOn2: play `voice` from the noise generator at volL/volR; it becomes
 * the only noise voice. adsr1/adsr2 are not read. */
void __attribute__((section(".text_80159424"))) sub_80159424(u8 voice, s16 volL, s16 volR, s32 adsr1, s32 adsr2)
{
    s32 lowBit;
    s32 highBit;
    u16 i;
    volatile int pad;

#if TM2_SPU_VOICES > 24
    if (voice >= 24) {
        return;  /* voices 24..47 have no noise (docs/SPU_VOICES.md 2): the voice stays silent */
    }
#endif
    D_801A5200[voice].volR = volR;
    D_801A5200[voice].volL = volL;
    D_801A5380[voice] |= 3;
    if (voice < 16) {
        lowBit = 1 << voice;
        highBit = 0;
    } else {
        lowBit = 0;
        highBit = 1 << (voice - 16);
    }
    D_801A5398[voice].pitch = 10;
    for (i = 0; i < D_801B2D10; i++) {
        D_801A5398[i].keyState &= 1;
    }
    D_801A5398[voice].keyState = 2;
    D_801A5398[voice].age = 0;
    D_801A5AE4 = lowBit | D_801A5AE4;
    D_801A5AEC = highBit | D_801A5AEC;
    D_801FAD38 = D_801FAD38 & ~D_801A5AE4;
    D_801FAD40 = D_801FAD40 & ~D_801A5AEC;
    D_8017F04C->noiseOn[0] = lowBit;
    D_8017F04C->noiseOn[1] = highBit;
}

extern VabHdr *D_801B2B10;      /* _svm_vh */
extern s16 D_801AB93C;          /* _svm_stereo_mono */
extern u16 D_801A5AF4;          /* _svm_orev1 */
extern u16 D_801A5AFC;          /* _svm_orev2 */
extern void sub_8015D518(s32 on_off, s32 voice_bit);   /* SpuSetNoiseVoice */

/* SpuVmAlloc: pick a voice for _svm_cur's request: a free silent voice, else
 * the playing voice of lowest priority (ties: quietest envelope, then oldest).
 * Returns spuVmMaxVoice when every voice outranks the request. */
u8 __attribute__((section(".text_80158400"))) sub_80158400(void)
{
    u8 found;
    u16 bestTer;
    u16 bestSec;
    u8 idx;
    u8 chosen;
    u8 bestIdx;
    u16 threshold;

    chosen = 99;
    bestSec = 0xFFFF;
    found = 0;
    bestTer = 0;
    threshold = D_801B2D50.prior;
    bestIdx = 99;
    for (idx = 0; idx < D_801B2D10; idx++) {
        if (D_801A5398[idx].keyState == 0 && D_801A5398[idx].envx == 0) {
            chosen = idx;
            break;
        }
        if (D_801A5398[idx].prior < threshold) {
            threshold = D_801A5398[idx].prior;
            bestSec = D_801A5398[idx].envx;
            bestTer = D_801A5398[idx].age;
            bestIdx = idx;
            found = 1;
        } else if (D_801A5398[idx].prior == threshold) {
            found += 1;
            if (D_801A5398[idx].envx < bestSec) {
                bestSec = D_801A5398[idx].envx;
                bestTer = D_801A5398[idx].age;
                bestIdx = idx;
            } else if (D_801A5398[idx].envx == bestSec) {
                if (bestTer < (s16)D_801A5398[idx].age) {
                    bestTer = D_801A5398[idx].age;
                    bestIdx = idx;
                }
            }
        }
    }
    if (chosen == 99) {
        if (found == 0) {
            chosen = D_801B2D10;
        } else {
            chosen = bestIdx;
        }
    }
    if (chosen < D_801B2D10) {
        for (idx = 0; idx < D_801B2D10; idx++) {
            D_801A5398[idx].age++;
        }
        D_801A5398[chosen].age = 0;
        D_801A5398[chosen].prior = D_801B2D50.prior;
        if (D_801A5398[chosen].keyState == 2) {
            sub_8015D518(0, 0xFFFFFF);
        }
#if TM2_SPU_VOICES > 24
        portVoiceCar[chosen] = 0;       /* request 87: libsnd's own voice (a sequence's, SsUtKeyOn's): no car */
        portVoiceKind[chosen] = 0;
#endif
    }
    return chosen;
}

/* SpuVmKeyOnNow: set _svm_cur's voice volumes (velocity x VAB, program, tone
 * and sequence volumes, panned by tone, program and key-on pan) and pitch,
 * and mark it to key on (and to or from reverb) at the next flush. */
void __attribute__((section(".text_801586A4"))) sub_801586A4(s32 unused, s32 pitch)
{
    volatile int pad;
    u32 seed;
    u32 chL;
    u32 chR;
    s32 lowBit;
    s32 highBit;
    struct SeqStruct *ss;
    u16 voice;
    s32 volMul;
    s16 ssn;
    u32 voll, volr;

    volMul = D_801B2B10->mvol * 16383;
    chL = (D_801B2D50.vol * volMul) / 16129;
    seed = ((chL * D_801B2D50.mvol) * D_801B2D50.tvol) / 16129;
    chL = seed;
    chR = seed;
    ssn = D_801B2D50.seqSepNo;
    voice = D_801B2D50.voice * 8;
    ss = &D_801FA3C4[ssn & 0xFF][(ssn >> 8) & 0xFF];
    if (ssn != 0x21) {
        chL = (seed * ss->unk74) / 127;
        chR = (seed * ss->unk76) / 127;
    }
    if (D_801B2D50.tpan < 64) {
        voll = chL;
        volr = (chR * D_801B2D50.tpan) / 63;
    } else {
        voll = (chL * (127 - D_801B2D50.tpan)) / 63;
        volr = chR;
    }
    if (D_801B2D50.mpan < 64) {
        volr = (volr * D_801B2D50.mpan) / 63;
    } else {
        voll = (voll * (127 - D_801B2D50.mpan)) / 63;
    }
    if (D_801B2D50.pan < 64) {
        volr = (volr * D_801B2D50.pan) / 63;
    } else {
        voll = (voll * (127 - D_801B2D50.pan)) / 63;
    }
    if (D_801AB93C == 1) {
        if (voll < volr) {
            voll = volr;
        } else {
            volr = voll;
        }
    }
    voll = (voll * voll) / 16383;
    volr = (volr * volr) / 16383;
    ((s16 *)D_801A5200)[voice + 2] = pitch;
    ((s16 *)D_801A5200)[voice + 0] = voll;
    ((s16 *)D_801A5200)[voice + 1] = volr;
    D_801A5380[D_801B2D50.voice] |= 7;
    D_801A5398[D_801B2D50.voice].pitch = pitch;
    D_801A5398[D_801B2D50.voice].keyState = 1;
#if TM2_SPU_VOICES > 24
    if (D_801B2D50.voice >= 24) {  /* voices 24..47: their words in portSpuExt */
        s32 w = (D_801B2D50.voice - 24) >> 4;
        u16 bit = 1 << ((D_801B2D50.voice - 24) & 15);

        if (D_801B2D50.mode & 4) {
            portSpuExt.orev[w] |= bit;
        } else {
            portSpuExt.orev[w] &= ~bit;
        }
        portSpuExt.okon[w] |= bit;
        portSpuExt.okof[0] &= ~portSpuExt.okon[0];
        portSpuExt.okof[1] &= ~portSpuExt.okon[1];
        return;
    }
#endif
    if (D_801B2D50.voice < 16) {
        lowBit = 1 << D_801B2D50.voice;
        highBit = 0;
    } else {
        lowBit = 0;
        highBit = 1 << (D_801B2D50.voice - 16);
    }
    if (D_801B2D50.mode & 4) {
        D_801A5AF4 = lowBit | D_801A5AF4;
        D_801A5AFC = highBit | D_801A5AFC;
    } else {
        D_801A5AF4 = D_801A5AF4 & ~lowBit;
        D_801A5AFC = D_801A5AFC & ~highBit;
    }
    D_801A5AE4 = lowBit | D_801A5AE4;
    D_801A5AEC = highBit | D_801A5AEC;
    D_801FAD38 = D_801FAD38 & ~D_801A5AE4;
    D_801FAD40 = D_801FAD40 & ~D_801A5AEC;
}

extern ProgAtr *D_801B1DE0;     /* _svm_pg */
extern u16 D_801AB8CC;          /* _svm_damper */
extern u32 D_801A5880[16];      /* _svm_envx_hist */

/* SpuVmDoAllocate: program _svm_cur's voice with its tone before the key-on:
 * clear its dead-envelope history, set its start address from the VAB's VAG
 * table and its ADSR from the tone (adsr2 plus _svm_damper). */
void __attribute__((section(".text_80158D1C"))) sub_80158D1C(void)
{
    s32 i;

    D_801B2D50.sregIdx = D_801B2D50.voice * 8;
    D_801B2D50.toneIdx = (D_801B2D50.prog * 16) + D_801B2D50.tone;
    D_801A5398[D_801B2D50.voice].envx = 0x7FFF;
#if TM2_SPU_VOICES > 24
    if (D_801B2D50.voice >= 24) {
        for (i = 0; i < 16; i++) {
            portSpuExt.envx_hist[i] &= ~(1 << (D_801B2D50.voice - 24));
        }
    } else
#endif
    for (i = 0; i < 16; i++) {
        D_801A5880[i] &= ~(1 << D_801B2D50.voice);
    }
    if (((s16)D_801B2D50.vagIdx & 1) > 0) {
        ((u16 *)D_801A5200)[D_801B2D50.sregIdx + 3] =
            D_801B1DE0[((s16)D_801B2D50.vagIdx - 1) / 2].reserved2;
        D_801A5380[D_801B2D50.voice] |= 8;
    } else {
        ((u16 *)D_801A5200)[D_801B2D50.sregIdx + 3] =
            D_801B1DE0[((s16)D_801B2D50.vagIdx - 1) / 2].reserved3;
        D_801A5380[D_801B2D50.voice] |= 8;
    }

    ((u16 *)D_801A5200)[D_801B2D50.sregIdx + 4] =
        D_801B2B1C[(D_801B2D50.prog * 16) + D_801B2D50.tone].adsr1;
    ((u16 *)D_801A5200)[D_801B2D50.sregIdx + 5] =
        D_801B2B1C[(D_801B2D50.prog * 16) + D_801B2D50.tone].adsr2 + D_801AB8CC;
    D_801A5380[D_801B2D50.voice] |= 0x30;
}

/* vmNoiseOn: key `voice` on the noise generator for _svm_cur's request (a tone
 * whose VAG is the noise marker): sequence x program x tone volumes, panned,
 * noise clock from note - centre. */
void __attribute__((section(".text_80158F54"))) sub_80158F54(u8 voice)
{
    struct SeqStruct *score;
    s16 i;
    u16 bitsUpper;
    u16 bitsLower;
    u32 voll_t, volr_t;
    u32 voll, volr;
    u16 cnd;
    u32 temp;
    volatile int pad;

#if TM2_SPU_VOICES > 24
    if (voice >= 24) {
        return;  /* voices 24..47 have no noise (docs/SPU_VOICES.md 2): the voice stays silent */
    }
#endif
    score = &D_801FA3C4[D_801B2D50.seqSepNo & 0xFF][(D_801B2D50.seqSepNo & 0xFF00) >> 8];

    voll_t = score->unk74 * 0x81;
    volr_t = score->unk76 * 0x81;

    voll_t = (voll_t * D_801B2D50.mvol) / 0x7F;
    volr_t = (volr_t * D_801B2D50.mvol) / 0x7F;

    voll_t = (voll_t * D_801B2D50.tvol) / 0x7F;
    volr_t = (volr_t * D_801B2D50.tvol) / 0x7F;

    cnd = ((u16 *)D_8017F04C)[0x1AA / 2];

    temp = D_801B2D50.tpan;
    if (temp < 0x40) {
        voll = voll_t;
        volr = (volr_t * temp) / 0x3F;
    } else {
        voll = (voll_t * (0x7F - temp)) / 0x3F;
        volr = volr_t;
    }
    temp = D_801B2D50.mpan;
    if (temp < 0x40) {
        volr = (volr * temp) / 0x3F;
    } else {
        voll = (voll * (0x7F - temp)) / 0x3F;
    }
    temp = D_801B2D50.pan;
    if (temp < 0x40) {
        volr = (temp * volr) / 0x3F;
    } else {
        voll = (voll * (0x7F - temp)) / 0x3F;
    }

    if (D_801AB93C == 1) {
        if (voll < volr) {
            voll = volr;
        } else {
            volr = voll;
        }
    }

    cnd &= ~0x3F00;
    cnd |= ((D_801B2D50.note - D_801B2D50.center) & 0x3F) << 8;
    ((u16 *)D_8017F04C)[0x1AA / 2] = cnd;

    D_801A5200[voice].volR = volr;
    D_801A5200[voice].volL = voll;
    D_801A5380[voice] |= 3;
    if (voice < 0x10) {
        bitsLower = 1 << voice;
        bitsUpper = 0;
    } else {
        bitsLower = 0;
        bitsUpper = 1 << (voice - 0x10);
    }
    D_801A5398[voice].pitch = 10;
    for (i = 0; i < D_801B2D10; i++) {
        D_801A5398[i].keyState &= 1;
    }
    D_801A5398[voice].keyState = 2;

    D_801A5AE4 |= bitsLower;
    D_801A5AEC |= bitsUpper;
    D_801FAD38 &= ~D_801A5AE4;
    D_801FAD40 &= ~D_801A5AEC;

    if (D_801B2D50.mode & 4) {
        D_801A5AF4 |= bitsLower;
        D_801A5AFC |= bitsUpper;
    } else {
        D_801A5AF4 &= ~bitsLower;
        D_801A5AFC &= ~bitsUpper;
    }

    D_8017F04C->noiseOn[0] = bitsLower;
    D_8017F04C->noiseOn[1] = bitsUpper;
}

extern VabHdr *D_801AB88C[16];  /* _svm_vab_vh */
extern ProgAtr *D_801AB848[16]; /* _svm_vab_pg */
extern VagAtr *D_801AB8D4[16];  /* _svm_vab_tn */
extern s16 D_801B1CB4;          /* kMaxPrograms */
extern s32 sub_801482BC(void);  /* _spu_getInTransfer */
extern s32 sub_801467D0(s32 size); /* SpuMalloc */

/* SsVabOpenHeadWithMode: open a VAB header (VH): register it in a free (or the given)
 * slot, index its programs and VAG sizes, and reserve sound buffer memory
 * (allocated when mode is 0, else at sbaddr). Returns the VAB id or -1. */
s16 __attribute__((section(".text_801462AC"))) sub_801462AC(u8 *addr, s16 vabid, s16 mode, u32 sbaddr)
{
    int vagLens[256];
    s32 i;
    s32 total;
    s16 vabId_2;
    u16 temp_v1;
    u16 *ptr_vag_off_table;
    u32 magic;
    u32 spuAllocMem;
    u8 num_vags;
    ProgAtr *pProgTable;
    u8 *ptr;
    VabHdr *vab_hdr_2;

    vabId_2 = 0x10;
    if (sub_801482BC() == 1) {
        return -1;
    }
    sub_80148290(1);
    if (vabid >= 0x10) {
        sub_80148290(0);
        return -1;
    }
    if (vabid == -1) {
        for (i = 0; i < 16; i++) {
            if (D_801B2D70[i] == 0) {
                D_801B2D70[i] = 1;
                vabId_2 = i;
                D_801FACA8++;
                break;
            }
        }
    } else {
        if (D_801B2D70[vabid] == 0) {
            D_801B2D70[vabid] = 1;
            vabId_2 = vabid;
            D_801FACA8++;
        }
    }
    if (vabId_2 >= 0x10) {
        goto fail;
    }
    ptr = addr;
    D_801AB88C[vabId_2] = (VabHdr *)ptr;
    ptr = ptr + 0x20;
    vab_hdr_2 = (VabHdr *)addr;
    magic = vab_hdr_2->form;
    if ((magic >> 8) != ('V' << 0x10 | 'A' << 0x8 | 'B')) {
        D_801B2D70[vabId_2] = 0;
        goto fail;
    }
    if ((magic & 0xFF) == 'p' && vab_hdr_2->ver >= 5) {
        D_801B1CB4 = 0x80;
    } else {
        D_801B1CB4 = 0x40;
    }
    if (vab_hdr_2->ps > D_801B1CB4) {
        goto end;
    }
    D_801AB848[vabId_2] = (ProgAtr *)ptr;
    pProgTable = (ProgAtr *)ptr;
    ptr = ptr + (D_801B1CB4 * 0x10);
    total = 0;
    for (i = 0; i < D_801B1CB4; i++) {
        pProgTable[i].reserved1 = total;
        if (pProgTable[i].tones != 0) {
            total++;
        }
    }
    D_801AB8D4[vabId_2] = (VagAtr *)ptr;
    total = 0;
    ptr_vag_off_table = (u16 *)(ptr + (vab_hdr_2->ps << 9));
    num_vags = vab_hdr_2->vs;
    for (i = 0; i < 256; i++) {
        if (num_vags >= i) {
            temp_v1 = *ptr_vag_off_table;
            if (vab_hdr_2->ver >= 5) {
                vagLens[i] = temp_v1 * 8;
            } else {
                vagLens[i] = temp_v1 * 4;
            }
            total += vagLens[i];
        }
        ptr_vag_off_table++;
    }

    D_801FACF4[vabId_2] = (u8 *)ptr_vag_off_table;
    spuAllocMem = sbaddr;
    if (mode == 0) {
        spuAllocMem = sub_801467D0(total);
        if (spuAllocMem == -1) {
            D_801B2D70[vabId_2] = 0;
            goto fail;
        }
    }
    if ((spuAllocMem + total) > 0x80000U) {
    end:
        D_801B2D70[vabId_2] = 0;
    fail:
        sub_80148290(0);
        D_801FACA8 -= 1;
        return -1;
    }
    D_801FACB0[vabId_2] = spuAllocMem;
    total = 0;
    for (i = 0; i <= num_vags; i++) {
        total += vagLens[i];
        if (!(i & 1)) {
            pProgTable[i / 2].reserved2 = (spuAllocMem + total) >> 3;
        } else {
            pProgTable[i / 2].reserved3 = (spuAllocMem + total) >> 3;
        }
    }

    D_801FAC68[vabId_2] = total;
    D_801B2D70[vabId_2] = 2;
    return vabId_2;
}
