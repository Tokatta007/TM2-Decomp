/* PsyQ SDK LIBSND (second half), reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * The voice manager (vmanager: pitch bend, auto volume/pan, init, noise,
 * flush, key on/off, sequence/program volume) and the SsUt* utility calls.
 *
 * Adapted from:
 *  - the LSD: Dream Emulator decompilation (CC0 1.0 public domain dedication):
 *    src/psyq/libsnd_vmanager.c, libsnd_vm_vol_ut_key_ut_keyv.c,
 *    libsnd_ut_cp_ut_cadsr_ut_vvol_ut_autov_ut_autop.c, libsnd_ut_ako.c and
 *    include/{svm_data.h,ss_score.h,libsnd_internal.h}
 *    SPDX-License-Identifier: CC0-1.0
 *  - the psxsdk sources of sotn-decomp (MIT): src/main/psxsdk/libsnd/
 *    {vmanager.c,vm_vsu.c,ut_gpa.c,ut_gva.c}
 *    SPDX-License-Identifier: MIT
 * TM2's libsnd is a 3.4-era build that differs from both in places (it has
 * SpuVmKeyOffNow / SpuVmSelectToneAndVag / vmNoiseOff as separate functions,
 * SpuVmSetVol scales by the channel volume, ...); every function here was
 * checked against the retail disassembly and follows retail where they differ.
 *
 * Matching: retail libsnd was built with GCC 2.5.7 (the unit's recipe:
 * gcc2.5.7 cc1, aspsx 2.21, gp_size 0); the C follows the shapes that
 * compiler needs (e.g. the shared failure exit placed inside the last
 * `if`, an unused local where retail reserves stack, initialisers that
 * steer CSE), always with retail's behaviour.
 *
 * The library's data lives at its retail addresses and is declared extern by
 * address (D_XXXXXXXX), with the SDK names #defined onto them. Functions of
 * the first half (sdk_libsnd.c) and of libspu are called by address name.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): with --port-define TM2_PORT_SPU48 the voice manager runs 48 voices
 * (TM2_SPU_VOICES); voices 24..47 have their registers in the second block (TM2_SPU_EXT_REGS) and their key /
 * reverb / envelope bits in portSpuExt. Without it TM2_SPU_VOICES is 24 and nothing here changes. */
#include "semantic/spu_voices.h"

/* ---- <libsnd.h> VAB structures ---- */
typedef struct VabHdr {
    s32 form;
    s32 ver;
    s32 id;
    u32 fsize;
    u16 reserved0;
    u16 ps;       /* +0x12 number of programs */
    u16 ts;
    u16 vs;
    u8 mvol;      /* +0x18 */
    u8 pan;
    u8 attr1;
    u8 attr2;
    u32 reserved1;
} VabHdr;

typedef struct ProgAtr {
    u8 tones;     /* +0x0 */
    u8 mvol;      /* +0x1 */
    u8 prior;     /* +0x2 */
    u8 mode;      /* +0x3 */
    u8 mpan;      /* +0x4 */
    u8 reserved0;
    s16 attr;     /* +0x6 */
    u32 reserved1; /* +0x8 low byte: the program's index into _svm_tn (x16) */
    u32 reserved2;
} ProgAtr;

typedef struct VagAtr {
    u8 prior;  /* +0x00 */
    u8 mode;   /* +0x01 */
    u8 vol;    /* +0x02 */
    u8 pan;    /* +0x03 */
    u8 center; /* +0x04 */
    u8 shift;  /* +0x05 */
    u8 min;    /* +0x06 */
    u8 max;    /* +0x07 */
    u8 vibW;
    u8 vibT;
    u8 porW;
    u8 porT;
    u8 pbmin;  /* +0x0C */
    u8 pbmax;  /* +0x0D */
    u8 reserved1;
    u8 reserved2;
    u16 adsr1; /* +0x10 */
    u16 adsr2; /* +0x12 */
    s16 prog;  /* +0x14 */
    s16 vag;   /* +0x16 */
    s16 reserved[4];
} VagAtr;

/* ---- voice manager tables (LSD svm_data.h) ---- */
typedef struct SvmVoice {
    s16 vag;      /* +0x00 VAG number, 0xFF = noise */
    u16 age;      /* +0x02 */
    s16 pitch;    /* +0x04 */
    u16 envx;     /* +0x06 */
    s16 vol;      /* +0x08 */
    u8 pan;       /* +0x0A */
    u8 pad0B;
    s16 note;     /* +0x0C */
    s16 seq;      /* +0x0E packed seq/sep number, 0x21 = SsUt / sound effect */
    s16 progIndex; /* +0x10 */
    s16 prog;     /* +0x12 */
    s16 tone;     /* +0x14 */
    s16 vabId;    /* +0x16 */
    s16 prior;    /* +0x18 */
    u8 pad1A;
    u8 keyState;  /* +0x1B 1 on, 2 noise, 0 off */
    s16 autoVolActive;    /* +0x1C */
    s16 autoVolStep;      /* +0x1E */
    s16 autoVolInterval;  /* +0x20 */
    s16 autoVolCountdown; /* +0x22 */
    s16 autoVolValue;     /* +0x24 */
    s16 autoVolTarget;    /* +0x26 */
    s16 autoPanActive;    /* +0x28 */
    s16 autoPanStep;      /* +0x2A */
    s16 autoPanInterval;  /* +0x2C */
    s16 autoPanCountdown; /* +0x2E */
    s16 autoPanValue;     /* +0x30 */
    s16 autoPanTarget;    /* +0x32 */
} SvmVoice; /* 0x34 */

/* A voice's shadow of its SPU registers, copied out by SpuVmFlush. */
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

/* The SPU register block at 0x1F801C00. */
typedef struct {
    SpuVoiceRegs voice[24];
    u8 pad180[8];
    u16 keyOn[2];    /* +0x188 */
    u16 keyOff[2];   /* +0x18C */
    u8 pad190[4];
    u16 noiseOn[2];  /* +0x194 */
    u16 reverbOn[2]; /* +0x198 */
} SpuRegs;

/* _svm_cur: the key-on request the voice manager is working on. */
typedef struct SvmCur {
    u8 tones;     /* +0x00 the program's tone count */
    u8 vabId;     /* +0x01 */
    u8 note;      /* +0x02 */
    u8 fine;      /* +0x03 */
    u8 voll;      /* +0x04 */
    u8 volr;      /* +0x05 pan from the voll/volr pair */
    u8 progNo;    /* +0x06 the program number (SpuVmVSetUp's) */
    u8 program;   /* +0x07 the program's tone block index (ProgAtr reserved1) */
    u8 pad08[2];
    u8 mvol;      /* +0x0A program volume */
    u8 mpan;      /* +0x0B program pan */
    u8 tone;      /* +0x0C */
    u8 vol;       /* +0x0D tone volume */
    u8 pan;       /* +0x0E tone pan */
    u8 prior;     /* +0x0F */
    u8 center;    /* +0x10 */
    u8 shift;     /* +0x11 */
    u8 min;       /* +0x12 */
    u8 max;       /* +0x13 */
    u8 mode;      /* +0x14 */
    u8 pad15;
    s16 seqSepNo; /* +0x16 */
    s16 vag;      /* +0x18 */
    s16 voice;    /* +0x1A */
    u8 pad1C[4];
} SvmCur;

/* ss_score.h, the fields used here */
typedef struct SsScore {
    u8 pad00[0x12];
    u8 channel;          /* +0x12 */
    u8 pad13[0x4E - 0x13];
    s16 channelVol[16];  /* +0x4E */
    u8 pad6E[0x74 - 0x6E];
    u16 volL;            /* +0x74 */
    u16 volR;            /* +0x76 */
    u8 pad78[0xAC - 0x78];
} SsScore; /* 0xAC */

typedef struct {
    s16 left;
    s16 right;
} SpuVolume;

typedef struct {
    u32 mask;
    s32 mode;
    SpuVolume depth;
    s32 delay;
    s32 feedback;
} SpuReverbAttr;

/* ---- library data (retail addresses) ---- */
extern SvmVoice D_801A5398[];      /* _svm_voice (TM2_SPU_VOICES; 48 are moved to .port_bss) */
extern SvmSreg D_801A5200[];       /* _svm_sreg_buf (TM2_SPU_VOICES) */
extern u8 D_801A5380[];            /* _svm_sreg_dirty (TM2_SPU_VOICES) */
extern SpuRegs *D_8017F04C;        /* _svm_sreg = 0x1F801C00 */
extern VabHdr *D_801B2B10;         /* _svm_vh */
extern ProgAtr *D_801B1DE0;        /* _svm_pg */
extern VagAtr *D_801B2B1C;         /* _svm_tn */
extern u8 D_801B2D10;              /* spuVmMaxVoice */
extern SvmCur D_801B2D50;          /* _svm_cur */
extern s16 D_801AB93C;             /* _svm_stereo_mono */
extern u16 D_801A5AE4;             /* _svm_okon1 */
extern u16 D_801A5AEC;             /* _svm_okon2 */
extern u16 D_801FAD38;             /* _svm_okof1 */
extern u16 D_801FAD40;             /* _svm_okof2 */
extern u16 D_801A5AF4;             /* _svm_orev1 */
extern u16 D_801A5AFC;             /* _svm_orev2 */
extern s32 D_801A5878;             /* _svm_envx_ptr */
extern s32 D_801A5880[];           /* _svm_envx_hist (16) */
extern u8 D_801B2E04;              /* _svm_auto_kof_mode */
extern s16 D_801B2D40;             /* _svm_vcf */
extern s16 D_801AB8CC;             /* _svm_damper */
extern u32 D_801A58C8[];           /* _ss_spu_vm_rec */
extern s16 D_801FACA8;             /* _svm_vab_count */
extern u8 D_801B2D70[];            /* _svm_vab_used (16) */
extern SpuReverbAttr D_801AB040;   /* _svm_rattr */
extern s16 D_801B1CB4;             /* kMaxPrograms */
extern SsScore *D_801FA3C4[];      /* _ss_score */
extern s32 D_801AEB10;             /* _snd_ev_flag */
extern VabHdr *D_801AB88C[];       /* _svm_vab_vh (16) */
extern ProgAtr *D_801AB848[];      /* _svm_vab_pg (16) */
extern VagAtr *D_801AB8D4[];       /* _svm_vab_tn (16) */

#define _svm_voice D_801A5398
#define _svm_sreg_buf D_801A5200
#define _svm_sreg_dirty D_801A5380
#define _svm_sreg D_8017F04C
#define _svm_vh D_801B2B10
#define _svm_pg D_801B1DE0
#define _svm_tn D_801B2B1C
#define spuVmMaxVoice D_801B2D10
#define _svm_cur D_801B2D50
#define _svm_stereo_mono D_801AB93C
#define _svm_okon1 D_801A5AE4
#define _svm_okon2 D_801A5AEC
#define _svm_okof1 D_801FAD38
#define _svm_okof2 D_801FAD40
#define _svm_orev1 D_801A5AF4
#define _svm_orev2 D_801A5AFC
#define _svm_envx_ptr D_801A5878
#define _svm_envx_hist D_801A5880
#define _svm_auto_kof_mode D_801B2E04
#define _svm_vcf D_801B2D40
#define _svm_damper D_801AB8CC
#define _ss_spu_vm_rec D_801A58C8
#define _svm_vab_count D_801FACA8
#define _svm_vab_used D_801B2D70
#define _svm_rattr D_801AB040
#define kMaxPrograms D_801B1CB4
#define _ss_score D_801FA3C4
#define _snd_ev_flag D_801AEB10
#define _svm_vab_vh D_801AB88C
#define _svm_vab_pg D_801AB848
#define _svm_vab_tn D_801AB8D4

#define SVM_SREG_DIRTY_VOL 0x03
#define SVM_SREG_DIRTY_PITCH 0x04
#define SVM_SREG_DIRTY_ADSR 0x30
#define SPUVM_SE_SEQ 0x21

#if TM2_SPU_VOICES > 24
#define SVM_EXT_SREG ((u16 *)TM2_SPU_EXT_REGS)   /* voices 24..47: their SPU registers */
#endif

/* ---- libspu ---- */
extern void sub_80148290(s32 mode);            /* _spu_setInTransfer */
extern s32 sub_8015D4C4(s32 num, void *top);   /* SpuInitMalloc */
extern s32 sub_8015D518(s32 on_off, s32 mask); /* SpuSetNoiseVoice */

/* ---- first-half functions (sdk_libsnd.c) ---- */
extern s32 sub_80158400();                     /* SpuVmAlloc (callers pass 0, 0xFF or nothing) */
extern void sub_801586A4(s32 count, s32 pitch); /* SpuVmKeyOnNow */
extern void sub_80158B90(s32 mode);            /* SpuVmKeyOffNow (voice in _svm_cur.voice) */
extern s32 sub_80158C6C(u8 *toneIdx, u8 *vagIdx); /* SpuVmSelectToneAndVag */
extern void sub_80158D1C(void);                /* SpuVmDoAllocate */
extern void sub_80158F54(s32 voice);           /* vmNoiseOn */
extern void sub_80159424(s32 voice, s32 volL, s32 volR, s32 adsr1, s32 adsr2); /* vmNoiseOn2 */
extern void sub_801595E4(s32 voice);           /* vmNoiseOff */
extern s32 sub_80159630(void);                 /* note2pitch */
extern s32 sub_801596EC(s32 note, s32 fine);   /* note2pitch2 */
extern void sub_80159A24(s16 voice, s16 from, s16 to, s16 duration); /* SeAutoVol */

/* ---- this file ---- */
void sub_80159BF4(s16 voice);
void sub_8015A198(s16 voice);
void sub_8015A8D0(s32 volL, s32 volR, s32 adsr1, s32 adsr2);
s16 sub_8015AA28(s16 voice, s16 seq, s16 vabId, s16 prog, u16 bend);
void sub_8015AD64(void);
s32 sub_8015B118(s16 seqSepNo, s16 vabId, s16 prog, u16 note, u16 voll, u16 volr);
s32 sub_8015B5E4(s16 seqSepNo, s16 vabId, s16 prog, u16 note);
s32 sub_8015D334(s16 vabId, s16 prog);

/* SePitchBend: bends a keyed voice by `bend` (signed; 127 = the tone's pbmax
 * semitones up, negative = pbmin down) and retunes it. */
void __attribute__((section(".text_801597F0"))) sub_801597F0(u8 voice, s16 bend)
{
    s32 tone;
    s16 note;
    s16 fine;
    s32 pos;
    u32 pr;

    pos = voice * 8;
    if (voice < TM2_SPU_VOICES) {
        _svm_cur.program = _svm_voice[voice].progIndex;
        _svm_cur.tone = _svm_voice[voice].tone;
        _svm_cur.voice = voice;
        pr = _svm_cur.program;
        tone = _svm_cur.tone + pr * 0x10;
        if (bend >= 0) {
            note = _svm_voice[voice].note + (bend * _svm_tn[tone].pbmax) / 0x7F;
            fine = (bend * _svm_tn[tone].pbmax) % 0x7F;
        } else {
            note = (_svm_voice[voice].note + (bend * _svm_tn[tone].pbmin) / 0x7F) - 1;
            fine = ((bend * _svm_tn[tone].pbmin) / 0x7F) + 0x7F;
        }
        ((u16 *)_svm_sreg_buf)[pos + 2] = sub_801596EC(note, fine);
        _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_PITCH;
    }
}

/* Empty vibrato hook (vm_vib). */
void __attribute__((section(".text_801599E0"))) sub_801599E0(void)
{
}

/* Empty vibrato hook (vm_vib). */
void __attribute__((section(".text_801599E8"))) sub_801599E8(void)
{
}

/* soundUtVibrateOnStub: empty in this library build. */
void __attribute__((section(".text_801599F0"))) sub_801599F0(s16 vc, s16 vibW, s16 vibT)
{
}

/* soundUtVibrateOffStub: empty in this library build. */
void __attribute__((section(".text_801599F8"))) sub_801599F8(s16 vc)
{
}

/* SetAutoVol: steps a voice's volume ramp once (SpuVmFlush calls it while
 * the ramp runs) and recomputes the voice's shadow volumes from the new
 * volume, the VAB/program/tone volumes and the three pans. */
void __attribute__((section(".text_80159BF4"))) sub_80159BF4(s16 voice)
{
    s16 off;
    s16 acc;
    s32 p;
    u32 q;
    u16 volL;
    u16 volR;

    off = voice * 8;
    if (_svm_voice[voice].autoVolInterval != 0) {
        if (_svm_voice[voice].autoVolCountdown-- > 0) {
            return;
        }
        _svm_voice[voice].autoVolCountdown = _svm_voice[voice].autoVolInterval;
    }
    _svm_voice[voice].autoVolValue += _svm_voice[voice].autoVolStep;
    if (_svm_voice[voice].autoVolStep > 0) {
        if (_svm_voice[voice].autoVolValue >= _svm_voice[voice].autoVolTarget) {
            _svm_voice[voice].autoVolValue = _svm_voice[voice].autoVolTarget;
            _svm_voice[voice].autoVolActive = 0;
        }
    } else if (_svm_voice[voice].autoVolStep < 0) {
        if (_svm_voice[voice].autoVolValue <= _svm_voice[voice].autoVolTarget) {
            _svm_voice[voice].autoVolValue = _svm_voice[voice].autoVolTarget;
            _svm_voice[voice].autoVolActive = 0;
        }
    }

    acc = _svm_voice[voice].autoVolValue;
    _svm_cur.voll = acc;

    q = _svm_vh->mvol * 0x3FFF;
    q = (acc * (s32)q) / 16129;
    q = (q * _svm_cur.mvol * _svm_cur.vol) / 16129u;

    p = _svm_cur.pan;
    if ((u32)p < 0x40) {
        volL = q;
        volR = (q * p) >> 6;
    } else {
        volR = q;
        volL = (q * (0x7F - p)) >> 6;
    }

    p = _svm_cur.mpan;
    if ((u32)p < 0x40) {
        volR = (volR * p) / 64;
    } else {
        volL = (volL * (0x7F - p)) / 64;
    }

    p = _svm_cur.volr;
    if ((u32)p < 0x40) {
        volR = (volR * p) / 64;
    } else {
        volL = (volL * (0x7F - p)) / 64;
    }

    if (_svm_stereo_mono == 1) {
        if (volR > volL) {
            volL = volR;
        } else {
            volR = volL;
        }
    }

    ((s16 *)_svm_sreg_buf)[off + 1] = volR;
    ((s16 *)_svm_sreg_buf)[off] = volL;
    _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_VOL;
}

/* SetAutoPan: steps a voice's pan ramp once, as SetAutoVol does for the
 * volume; the value becomes _svm_cur's pan and the shadow volumes are
 * recomputed. */
void __attribute__((section(".text_8015A198"))) sub_8015A198(s16 voice)
{
    s16 off;
    s32 pan;
    s32 p;
    u32 q;
    u16 volL;
    u16 volR;

    off = voice * 8;
    if (_svm_voice[voice].autoPanInterval != 0) {
        if (_svm_voice[voice].autoPanCountdown-- > 0) {
            return;
        }
        _svm_voice[voice].autoPanCountdown = _svm_voice[voice].autoPanInterval;
    }
    _svm_voice[voice].autoPanValue += _svm_voice[voice].autoPanStep;
    if (_svm_voice[voice].autoPanStep > 0) {
        if (_svm_voice[voice].autoPanValue >= _svm_voice[voice].autoPanTarget) {
            _svm_voice[voice].autoPanValue = _svm_voice[voice].autoPanTarget;
            _svm_voice[voice].autoPanActive = 0;
        }
    } else if (_svm_voice[voice].autoPanStep < 0) {
        if (_svm_voice[voice].autoPanValue <= _svm_voice[voice].autoPanTarget) {
            _svm_voice[voice].autoPanValue = _svm_voice[voice].autoPanTarget;
            _svm_voice[voice].autoPanActive = 0;
        }
    }

    pan = (u8)_svm_voice[voice].autoPanValue;
    _svm_cur.volr = pan;

    q = _svm_vh->mvol * 0x3FFF;
    q = (_svm_cur.voll * (s32)q) / 16129;
    q = (q * _svm_cur.mvol * _svm_cur.vol) / 16129u;

    p = _svm_cur.pan;
    if ((u32)p < 0x40) {
        volL = q;
        volR = (q * p) >> 6;
    } else {
        volR = q;
        volL = (q * (0x7F - p)) >> 6;
    }

    p = _svm_cur.mpan;
    if ((u32)p < 0x40) {
        volR = (volR * p) / 64;
    } else {
        volL = (volL * (0x7F - p)) / 64;
    }

    p = pan;
    if ((u32)p < 0x40) {
        volR = (volR * p) / 64;
    } else {
        volL = (volL * (0x7F - p)) / 64;
    }

    if (_svm_stereo_mono == 1) {
        if (volR > volL) {
            volL = volR;
        } else {
            volR = volL;
        }
    }

    ((s16 *)_svm_sreg_buf)[off + 1] = volR;
    ((s16 *)_svm_sreg_buf)[off] = volL;
    _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_VOL;
}

/* SpuVmInit: resets the voice manager for `voices` voices (at most 24; 48 with TM2_PORT_SPU48):
 * every voice, its shadow and SPU registers, the VAB table, the key masks,
 * the reverb depth (0x3FFF), then one SpuVmFlush. */
void __attribute__((section(".text_8015A560"))) sub_8015A560(s32 voices)
{
    u16 i;
    u16 *regs;
    u16 off;
    u32 n;

    sub_80148290(0);
    _svm_vcf = 0;
    _svm_damper = 0;
    sub_8015D4C4(0x20, _ss_spu_vm_rec);

    for (i = 0; i < TM2_SPU_VOICES * 8; i++) {
        ((u16 *)_svm_sreg_buf)[i] = 0;
    }
    for (i = 0; i < TM2_SPU_VOICES; i++) {
        _svm_sreg_dirty[i] = 0;
    }
    _svm_vab_count = 0;
    for (i = 0; i < 16; i++) {
        _svm_vab_used[i] = 0;
    }

    n = (u8)voices;
    if (n >= TM2_SPU_VOICES) {
        spuVmMaxVoice = TM2_SPU_VOICES;
    } else {
        spuVmMaxVoice = n;
    }

    for (i = 0; i < spuVmMaxVoice; i++) {
        off = i * 8;
        _svm_voice[i].age = 0x18;
        _svm_voice[i].vag = 0xFF;
        _svm_voice[i].keyState = 0;
        _svm_voice[i].pitch = 0;
        _svm_voice[i].envx = 0;
        _svm_voice[i].seq = -1;
        _svm_voice[i].progIndex = 0;
        _svm_voice[i].prog = 0;
        _svm_voice[i].tone = 0xFF;
        _svm_voice[i].vol = 0;
        _svm_voice[i].pan = 0x40;
        _svm_voice[i].autoVolActive = 0;
        _svm_voice[i].autoVolStep = 0;
        _svm_voice[i].autoVolInterval = 0;
        _svm_voice[i].autoVolCountdown = 0;
        _svm_voice[i].autoPanActive = 0;
        _svm_voice[i].autoPanStep = 0;
        _svm_voice[i].autoPanInterval = 0;
        _svm_voice[i].autoPanCountdown = 0;
        _svm_voice[i].autoPanValue = 0;
        _svm_voice[i].autoVolValue = 0;

#if TM2_SPU_VOICES > 24
        if (i >= 24) {
            regs = SVM_EXT_SREG + (i - 24) * 8;
        } else
#endif
        regs = (u16 *)_svm_sreg + off;
        regs[0] = 0;      /* volL */
        regs[1] = 0;      /* volR */
        regs[3] = 0x200;  /* addr */
        regs[2] = 0x1000; /* pitch */
        regs[4] = 0x80FF; /* adsr1 */
        regs[5] = 0x4000; /* adsr2 */

        _svm_cur.voice = i;
        sub_80158B90(1);
    }

    _svm_okon1 = 0;
    _svm_okon2 = 0;
    _svm_okof1 = 0;
    _svm_orev1 = 0;
    _svm_orev2 = 0;
#if TM2_SPU_VOICES > 24
    portSpuExt.okon[0] = 0;
    portSpuExt.okon[1] = 0;
    portSpuExt.okof[0] = 0;
    portSpuExt.okof[1] = 0;
    portSpuExt.orev[0] = 0;
    portSpuExt.orev[1] = 0;
#endif
    _svm_rattr.mask = 0;
    _svm_rattr.depth.left = 0x3FFF;
    _svm_rattr.depth.right = 0x3FFF;
    _svm_rattr.mode = 0;
    _svm_auto_kof_mode = 0;
    _svm_stereo_mono = 0;
    kMaxPrograms = 0x80;
    sub_8015AD64();
}

/* SpuVmNoiseOnWithAdsr: allocates a voice at top priority and keys it on the
 * noise generator at volumes volL/volR. */
void __attribute__((section(".text_8015A8D0"))) sub_8015A8D0(s32 volL, s32 volR, s32 adsr1, s32 adsr2)
{
    s16 v;

    _svm_cur.prior = 0x7F;
    _svm_cur.voice = v = sub_80158400(0xFF) & 0xFF;
    if (v < spuVmMaxVoice) {
        sub_80159424(v, volL & 0xFFFF, volR & 0xFFFF, adsr1 & 0xFFFF, adsr2 & 0xFFFF);
    }
}

/* SpuVmNoiseOff: releases every voice playing the noise generator. */
void __attribute__((section(".text_8015A960"))) sub_8015A960(void)
{
    s16 i;

    for (i = 0; i < spuVmMaxVoice; i++) {
        if (_svm_voice[i].keyState == 2) {
            sub_801595E4(i);
        }
    }
}

/* SpuVmNoiseOn: SpuVmNoiseOnWithAdsr with the default envelope. */
void __attribute__((section(".text_8015A9FC"))) sub_8015A9FC(s32 volL, s32 volR)
{
    sub_8015A8D0(volL & 0xFFFF, volR & 0xFFFF, 0x80FF, 0x5FC8);
}

/* SpuVmPBVoice: bends `voice`'s pitch if it plays sequence `seq`, VAB
 * `vabId` and program `prog`; `bend` is 0..127 centred on 0x40, scaled to the
 * tone's pbmax/pbmin. Returns 1 when the voice matched, else 0. */
s16 __attribute__((section(".text_8015AA28"))) sub_8015AA28(s16 voice, s16 seq, s16 vabId, s16 prog, u16 bend)
{
    s16 amount;
    u16 tone;
    u16 base;
    u16 note;
    u16 fine;

    amount = bend - 0x40;
    if (_svm_voice[voice].seq == seq && _svm_voice[voice].vabId == vabId &&
        _svm_voice[voice].prog == prog) {
        tone = _svm_voice[voice].tone + (_svm_cur.program << 4);
        base = _svm_voice[voice].note;
        if (amount > 0) {
            note = base + amount * _svm_tn[tone].pbmax / 63;
            fine = (amount * _svm_tn[tone].pbmax % 63) * 2;
        } else {
            note = base;
            if (amount < 0) {
                note = base + amount * _svm_tn[tone].pbmin / 64 - 1;
                fine = (amount * _svm_tn[tone].pbmin % 64) * 2 + 127;
            } else {
                fine = 0;
            }
        }
        _svm_cur.tone = (u8)_svm_voice[voice].tone;
        _svm_cur.voice = voice;
        _svm_sreg_buf[voice].pitch = sub_801596EC(note, fine);
        _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_PITCH;
        return 1;
    } else {
        return 0;
    }
}

/* SpuVmPitchBend: SpuVmPBVoice on every voice; returns how many matched. */
s32 __attribute__((section(".text_8015AC78"))) sub_8015AC78(s16 seq, s16 vabId, s16 prog, u16 bend)
{
    s16 i;
    s32 sum = 0;

    sub_8015D334(vabId, prog);
    _svm_cur.seqSepNo = seq;
    for (i = 0; i < spuVmMaxVoice; i++) {
        sum += sub_8015AA28(i, seq, vabId, prog, bend);
    }
    return sum;
}

/* SpuVmFlush: the once-a-tick flush. Records which voices' envelopes have
 * died, frees voices silent across the whole history (unless
 * _svm_auto_kof_mode), steps the volume/pan ramps, copies the dirty shadow
 * registers to the SPU and writes the key-off, key-on and reverb masks. */
void __attribute__((section(".text_8015AD64"))) sub_8015AD64(void)
{
    u32 mask;
    s32 i;
    SpuRegs *spu;
    u16 okof1;
    u16 okof2;
    u16 okon1;
    u16 okon2;
    u16 orev1;
    u16 orev2;
#if TM2_SPU_VOICES > 24
    u32 mask2;
#endif

    _svm_envx_ptr = (_svm_envx_ptr + 1) & 0xF;
    _svm_envx_hist[_svm_envx_ptr] = 0;
#if TM2_SPU_VOICES > 24
    portSpuExt.envx_hist[_svm_envx_ptr] = 0;
#endif
    for (i = 0; i < spuVmMaxVoice; i++) {
#if TM2_SPU_VOICES > 24
        if (i >= 24) {  /* voices 24..47: their block and their history word */
            _svm_voice[i].envx = SVM_EXT_SREG[(i - 24) * 8 + 6];
            if (_svm_voice[i].envx == 0) {
                portSpuExt.envx_hist[_svm_envx_ptr] |= 1 << (i - 24);
            }
            continue;
        }
#endif
        _svm_voice[i].envx = ((u16 *)_svm_sreg)[i * 8 + 6];
        if (_svm_voice[i].envx == 0) {
            _svm_envx_hist[_svm_envx_ptr] |= 1 << i;
        }
    }

    if (_svm_auto_kof_mode == 0) {
        mask = 0xFFFFFFFF;
        for (i = 0; i < 15; i++) {
            mask &= _svm_envx_hist[i];
        }
#if TM2_SPU_VOICES > 24
        mask2 = 0xFFFFFFFF;
        for (i = 0; i < 15; i++) {
            mask2 &= portSpuExt.envx_hist[i];
        }
#endif
        for (i = 0; i < spuVmMaxVoice; i++) {
#if TM2_SPU_VOICES > 24
            if (i >= 24 ? (mask2 & (1 << (i - 24))) != 0 : (mask & (1 << i)) != 0) {
#else
            if (mask & (1 << i)) {
#endif
                if (_svm_voice[i].keyState == 2) {
                    sub_8015D518(0, 0xFFFFFF);
                }
                _svm_voice[i].keyState = 0;
            }
        }
    }

    _svm_okon1 &= ~_svm_okof1;
    _svm_okon2 &= ~_svm_okof2;
#if TM2_SPU_VOICES > 24
    portSpuExt.okon[0] &= ~portSpuExt.okof[0];
    portSpuExt.okon[1] &= ~portSpuExt.okof[1];
#endif
    for (i = 0; i < TM2_SPU_VOICES; i++) {
        if (_svm_voice[i].autoVolActive != 0) {
            sub_80159BF4(i);
        }
        if (_svm_voice[i].autoPanActive != 0) {
            sub_8015A198(i);
        }
    }

    for (i = 0; i < 24; i++) {  /* (voices 0..23; 24..47 below) */
        if (_svm_sreg_dirty[i] & 1) {
            ((u16 *)_svm_sreg)[i * 8 + 0] = ((u16 *)_svm_sreg_buf)[i * 8 + 0];
            ((u16 *)_svm_sreg)[i * 8 + 1] = ((u16 *)_svm_sreg_buf)[i * 8 + 1];
        }
        if (_svm_sreg_dirty[i] & 4) {
            ((u16 *)_svm_sreg)[i * 8 + 2] = ((u16 *)_svm_sreg_buf)[i * 8 + 2];
        }
        if (_svm_sreg_dirty[i] & 8) {
            ((u16 *)_svm_sreg)[i * 8 + 3] = ((u16 *)_svm_sreg_buf)[i * 8 + 3];
        }
        if (_svm_sreg_dirty[i] & 0x10) {
            ((u16 *)_svm_sreg)[i * 8 + 4] = ((u16 *)_svm_sreg_buf)[i * 8 + 4];
            ((u16 *)_svm_sreg)[i * 8 + 5] = ((u16 *)_svm_sreg_buf)[i * 8 + 5];
        }
        _svm_sreg_dirty[i] = 0;
    }
#if TM2_SPU_VOICES > 24
    for (i = 24; i < TM2_SPU_VOICES; i++) {  /* voices 24..47: the same, to their block */
        u16 *regs = SVM_EXT_SREG + (i - 24) * 8;

        if (_svm_sreg_dirty[i] & 1) {
            regs[0] = ((u16 *)_svm_sreg_buf)[i * 8 + 0];
            regs[1] = ((u16 *)_svm_sreg_buf)[i * 8 + 1];
        }
        if (_svm_sreg_dirty[i] & 4) {
            regs[2] = ((u16 *)_svm_sreg_buf)[i * 8 + 2];
        }
        if (_svm_sreg_dirty[i] & 8) {
            regs[3] = ((u16 *)_svm_sreg_buf)[i * 8 + 3];
        }
        if (_svm_sreg_dirty[i] & 0x10) {
            regs[4] = ((u16 *)_svm_sreg_buf)[i * 8 + 4];
            regs[5] = ((u16 *)_svm_sreg_buf)[i * 8 + 5];
        }
        _svm_sreg_dirty[i] = 0;
    }
#endif

    okof1 = _svm_okof1;
    spu = _svm_sreg;
    okof2 = _svm_okof2;
    okon1 = _svm_okon1;
    okon2 = _svm_okon2;
    orev1 = _svm_orev1;
    orev2 = _svm_orev2;
    _svm_okof1 = 0;
    _svm_okof2 = 0;
    _svm_okon1 = 0;
    _svm_okon2 = 0;
    spu->keyOff[0] = okof1;
    spu->keyOff[1] = okof2;
    spu->keyOn[0] = okon1;
    spu->keyOn[1] = okon2;
    spu->reverbOn[0] = orev1;
    spu->reverbOn[1] = orev2;
#if TM2_SPU_VOICES > 24
    spu = (SpuRegs *)TM2_SPU_EXT_REGS;  /* voices 24..47: their key off, key on and reverb words */
    okof1 = portSpuExt.okof[0];
    okof2 = portSpuExt.okof[1];
    okon1 = portSpuExt.okon[0];
    okon2 = portSpuExt.okon[1];
    portSpuExt.okof[0] = 0;
    portSpuExt.okof[1] = 0;
    portSpuExt.okon[0] = 0;
    portSpuExt.okon[1] = 0;
    spu->keyOff[0] = okof1;
    spu->keyOff[1] = okof2;
    spu->keyOn[0] = okon1;
    spu->keyOn[1] = okon2;
    spu->reverbOn[0] = portSpuExt.orev[0];
    spu->reverbOn[1] = portSpuExt.orev[1];
#endif
}

/* SpuVmKeyOn: keys on `note` of VAB `vabId`'s program `prog` for a sequence:
 * one allocated voice per tone whose note range holds the note. A `voll` of
 * 0 is a key-off. Returns a bit per voice keyed, -1 when a voice could not
 * be allocated or the VAB/program is invalid. */
s32 __attribute__((section(".text_8015B118"))) sub_8015B118(s16 seqSepNo, s16 vabId, s16 prog, u16 note, u16 voll, u16 volr)
{
    /* unused (the compiler drops it), but it shapes the register allocation */
    SsScore *score = &_ss_score[seqSepNo & 0xFF][(seqSepNo & 0xFF00) >> 8];
    u8 vagIdx[128];
    u8 toneIdx[128];
    u8 i;
    s32 result;
    s32 tone;
    u8 count;

    result = 0;
    if (sub_8015D334(vabId, prog)) {
        return -1;
    }
    _svm_cur.seqSepNo = seqSepNo;
    _svm_cur.note = note;
    _svm_cur.fine = 0;
    _svm_cur.voll = voll;
    _svm_cur.volr = volr;
    _svm_cur.mvol = _svm_pg[prog].mvol;
    _svm_cur.mpan = _svm_pg[prog].mpan;
    _svm_cur.tones = _svm_pg[prog].tones;
    if (_svm_cur.program >= _svm_vh->ps) {
        return -1;
    }
    if (voll == 0) {
        result = sub_8015B5E4(seqSepNo, vabId, prog, note);
    } else {
        count = sub_80158C6C(toneIdx, vagIdx);
        for (i = 0; i < count; i++) {
            _svm_cur.vag = vagIdx[i];
            _svm_cur.tone = toneIdx[i];
            tone = _svm_cur.tone + (_svm_cur.program * 0x10);
            _svm_cur.prior = _svm_tn[tone].prior;
            _svm_cur.vol = _svm_tn[tone].vol;
            _svm_cur.pan = _svm_tn[tone].pan;
            _svm_cur.center = _svm_tn[tone].center;
            _svm_cur.shift = _svm_tn[tone].shift;
            _svm_cur.mode = _svm_tn[tone].mode;
            _svm_cur.min = _svm_tn[tone].min;
            _svm_cur.max = _svm_tn[tone].max;
            _svm_cur.voice = sub_80158400(0) & 0xFF;
            if (_svm_cur.voice < spuVmMaxVoice) {
                _svm_voice[_svm_cur.voice].keyState = 1;
                _svm_voice[_svm_cur.voice].age = 0;
                _svm_voice[_svm_cur.voice].seq = seqSepNo;
                _svm_voice[_svm_cur.voice].vabId = _svm_cur.vabId;
                _svm_voice[_svm_cur.voice].progIndex = _svm_cur.program;
                _svm_voice[_svm_cur.voice].prog = prog;
                if (seqSepNo != SPUVM_SE_SEQ) {
                    _svm_voice[_svm_cur.voice].vol = voll;
                }
                _svm_voice[_svm_cur.voice].pan = volr;
                _svm_voice[_svm_cur.voice].tone = _svm_cur.tone;
                _svm_voice[_svm_cur.voice].note = note;
                _svm_voice[_svm_cur.voice].prior = _svm_cur.prior;
                _svm_voice[_svm_cur.voice].vag = _svm_cur.vag;
                sub_80158D1C();
                if (_svm_cur.vag == 0xFF) {
                    sub_80158F54(_svm_cur.voice);
                } else {
                    sub_801586A4(count, sub_80159630() & 0xFFFF);
                }
#if TM2_SPU_VOICES > 32
                if (_svm_cur.voice < 32) {  /* (a bit per voice keyed: voices 32..47 have none) */
                    result |= 1 << _svm_cur.voice;
                }
#else
                result |= 1 << _svm_cur.voice;
#endif
            } else {
                result = -1;
            }
        }
    }
    return result;
}

/* SpuVmKeyOff: releases every voice playing `note` of VAB `vabId`'s program
 * `prog` for sequence `seqSepNo`; returns how many were released. */
s32 __attribute__((section(".text_8015B5E4"))) sub_8015B5E4(s16 seqSepNo, s16 vabId, s16 prog, u16 note)
{
    u8 v;
    s32 n;

    n = 0;
    for (v = 0; v < spuVmMaxVoice; v++) {
        if (_svm_voice[v].note == note && _svm_voice[v].prog == prog &&
            _svm_voice[v].seq == seqSepNo && _svm_voice[v].vabId == vabId) {
            if (_svm_voice[v].vag == 0xFF) {
                sub_801595E4(v);
            } else {
                _svm_cur.voice = v;
                sub_80158B90(0);
            }
            n++;
        }
    }
    return n;
}

/* SpuVmSeKeyOn: keys on a sound effect (sequence 0x21) with the louder of
 * the two volumes as the velocity and their ratio as the pan. */
s32 __attribute__((section(".text_8015B740"))) sub_8015B740(s16 vabId, s16 prog, u16 note, s32 unused, u16 volL, u16 volR)
{
    u16 vol;
    u16 pan;

    if (volL == volR) {
        pan = 0x40;
        vol = volL;
    } else if (volR < volL) {
        vol = volL;
        pan = (volR << 6) / volL;
    } else {
        vol = volR;
        pan = 0x7F - ((volL << 6) / volR);
    }
    return sub_8015B118(SPUVM_SE_SEQ, vabId, prog, note, vol, pan);
}

/* SpuVmSeKeyOff: keys off a sound effect SpuVmSeKeyOn started. */
s32 __attribute__((section(".text_8015B830"))) sub_8015B830(s16 vabId, s16 prog, u16 note)
{
    return sub_8015B5E4(SPUVM_SE_SEQ, vabId, prog, note);
}

/* SpuVmSetSeqVol: sets sequence `seqSepNo`'s volume (clamped to 0x7F) and,
 * with mode 1, writes it straight to the voices the sequence plays. */
s32 __attribute__((section(".text_8015B86C"))) sub_8015B86C(s16 seqSepNo, u16 volL, u16 volR, s16 mode)
{
    SsScore *score;
    s32 l;
    s32 r;
    s16 v;
    s16 off;
    s32 seq = seqSepNo;
    s32 unused[2]; /* retail's frame reserves 8 bytes nothing uses */

    score = &_ss_score[seqSepNo & 0xFF][(seqSepNo >> 8) & 0xFF];
    _svm_cur.seqSepNo = seqSepNo;
    score->volL = volL;
    score->volR = volR;
    if (score->volL >= 0x80) {
        score->volL = 0x7F;
    }
    if (score->volR >= 0x80) {
        score->volR = 0x7F;
    }
    l = volL * 0x81;
    r = volR * 0x81;
    if (mode == 1) {
        for (v = 0; v < spuVmMaxVoice; v++) {
            if ((u16)_svm_voice[v].seq == (u16)seq) {
                off = v * 8;
                ((s16 *)_svm_sreg_buf)[off] = l;
                ((s16 *)_svm_sreg_buf)[off + 1] = r;
                _svm_sreg_dirty[v] |= SVM_SREG_DIRTY_VOL;
            }
        }
    }
    return _svm_cur.seqSepNo;
}

/* SpuVmGetSeqVol: reads sequence `seqSepNo`'s volume back. */
s32 __attribute__((section(".text_8015B9EC"))) sub_8015B9EC(s32 seqSepNo, s16 *volL, s16 *volR)
{
    SsScore *score;

    score = &_ss_score[seqSepNo & 0xFF][(seqSepNo & 0xFF00) >> 8];
    _svm_cur.seqSepNo = seqSepNo;
    *volL = score->volL;
    *volR = score->volR;
    return _svm_cur.seqSepNo;
}

/* SpuVmSeqKeyOff: keys off every voice sequence `seqSepNo` is playing. */
void __attribute__((section(".text_8015BB04"))) sub_8015BB04(s16 seqSepNo)
{
    u8 v;

    for (v = 0; v < spuVmMaxVoice; v++) {
        if (_svm_voice[v].seq == seqSepNo) {
            _svm_cur.voice = v;
            sub_80158B90(0);
        }
    }
}

/* SpuVmSetProgVol: sets a program's volume; returns it, or -1. */
s32 __attribute__((section(".text_8015BBA0"))) sub_8015BBA0(s16 vabId, s16 prog, s32 vol)
{
    if (sub_8015D334(vabId, prog) != 0) {
        return -1;
    }
    _svm_pg[prog].mvol = vol;
    return _svm_pg[prog].mvol;
}

/* SpuVmGetProgVol: a program's volume, or -1. */
s32 __attribute__((section(".text_8015BC10"))) sub_8015BC10(s16 vabId, s16 prog)
{
    if (sub_8015D334(vabId, prog) != 0) {
        return -1;
    }
    return _svm_pg[prog].mvol;
}

/* SpuVmSetProgPan: sets a program's pan; returns it, or -1. */
s32 __attribute__((section(".text_8015BC60"))) sub_8015BC60(s16 vabId, s16 prog, s32 pan)
{
    if (sub_8015D334(vabId, prog) != 0) {
        return -1;
    }
    _svm_pg[prog].mpan = pan;
    return _svm_pg[prog].mpan;
}

/* SpuVmGetProgPan: a program's pan, or -1. */
s32 __attribute__((section(".text_8015BCD0"))) sub_8015BCD0(s16 vabId, s16 prog)
{
    if (sub_8015D334(vabId, prog) != 0) {
        return -1;
    }
    return _svm_pg[prog].mpan;
}

/* SpuVmSetVol: rescales every voice sequence `seqSepNo` plays on VAB
 * `vabId`'s program `prog` to channel volume `vol` and pan `pan`: the
 * voice's level (relative to the channel volume) times the VAB's, program's
 * and tone's volumes and the sequence's L/R volume, panned by the tone, the
 * program and `pan`, squared into the shadow volume registers. Returns the
 * number of voices rescaled. */
s32 __attribute__((section(".text_8015BD20"))) sub_8015BD20(s16 seqSepNo, s16 vabId, s16 prog, u16 vol, u16 pan)
{
    SsScore *score = &_ss_score[seqSepNo & 0xFF][(seqSepNo & 0xFF00) >> 8];
    s32 n;
    u8 v;
    s32 lvl;
    u32 voll, volr;
    u32 voll_t, volr_t;
    u8 p;

    n = 0;
    sub_8015D334(vabId, prog);
    _svm_cur.seqSepNo = seqSepNo;
    for (v = 0; v < spuVmMaxVoice; v++) {
        if (_svm_voice[v].seq == seqSepNo && _svm_voice[v].prog == prog &&
            _svm_voice[v].vabId == vabId) {
            if (score->channelVol[score->channel] == vol) {
                lvl = (_svm_vh->mvol * 0x3FFF * _svm_voice[v].vol) / 0x3F01;
            } else {
                lvl = (_svm_vh->mvol * 0x3FFF * (_svm_voice[v].vol * (vol / score->channelVol[score->channel]))) / 0x3F01;
            }
            voll_t = (u32)(lvl * _svm_pg[prog].mvol * _svm_tn[(prog << 4) + _svm_voice[v].tone].vol) / 0x3F01;
            volr_t = (voll_t * score->volR) / 127;
            voll_t = (voll_t * score->volL) / 127;

            /* (retail indexes the tone's pan without the program offset) */
            p = _svm_tn[_svm_voice[v].tone].pan;
            if (p < 0x40) {
                voll = voll_t;
                volr = (volr_t * p) / 63;
            } else {
                voll = (voll_t * (0x7F - p)) / 63;
                volr = volr_t;
            }
            p = _svm_pg[_svm_voice[v].progIndex].mpan;
            if (p < 0x40) {
                volr = (volr * p) / 63;
            } else {
                voll = (voll * (0x7F - p)) / 63;
            }
            p = pan;
            if (p < 0x40) {
                volr = (volr * p) / 63;
            } else {
                voll = (voll * (0x7F - p)) / 63;
            }
            if (_svm_stereo_mono == 1) {
                if (voll < volr) {
                    voll = volr;
                } else {
                    volr = voll;
                }
            }
            voll = (voll * voll) / 0x3FFF;
            volr = (volr * volr) / 0x3FFF;
            ((u16 *)_svm_sreg_buf)[v * 8 + 0] = voll;
            ((u16 *)_svm_sreg_buf)[v * 8 + 1] = volr;
            n++;
            _svm_sreg_dirty[v] |= SVM_SREG_DIRTY_VOL;
        }
    }
    return n;
}

/* SsUtKeyOn: keys on one tone of a VAB program on a free voice (vmNoiseOn
 * for a noise tone). Returns the voice, or -1. */
s16 __attribute__((section(".text_8015C2AC"))) sub_8015C2AC(s16 vabId, s16 prog, s16 tone, s16 note, s16 fine, s16 voll, s16 volr)
{
    ProgAtr *pg;
    VagAtr *t;
    s32 v;
    u8 w;

    if (_snd_ev_flag == 1) {
        return -1;
    }
    _snd_ev_flag = 1;
    if (sub_8015D334(vabId, prog) != 0) {
        goto fail;
    }
    _svm_cur.seqSepNo = SPUVM_SE_SEQ;
    _svm_cur.note = note;
    _svm_cur.fine = fine;
    _svm_cur.tone = tone;
    if (voll == volr) {
        _svm_cur.volr = 0x40;
        _svm_cur.voll = voll;
    } else if (volr < voll) {
        _svm_cur.voll = voll;
        _svm_cur.volr = (volr << 6) / voll;
    } else {
        _svm_cur.voll = volr;
        _svm_cur.volr = 0x7F - ((voll << 6) / volr);
    }

    pg = &_svm_pg[prog];
    _svm_cur.mvol = pg->mvol;
    _svm_cur.mpan = pg->mpan;
    _svm_cur.tones = pg->tones;

    t = &_svm_tn[_svm_cur.tone + (_svm_cur.program << 4)];
    _svm_cur.prior = t->prior;
    _svm_cur.vag = t->vag;
    _svm_cur.vol = t->vol;
    _svm_cur.pan = t->pan;
    _svm_cur.center = t->center;
    _svm_cur.shift = t->shift;
    _svm_cur.mode = t->mode;
    _svm_cur.min = t->min;
    _svm_cur.max = t->max;
    if (_svm_cur.vag == 0) {
        goto fail;
    }

    v = sub_80158400(_svm_cur.vag) & 0xFF; /* SpuVmAlloc ignores its argument */
    w = v;
    if (w == spuVmMaxVoice) {
fail:
        _snd_ev_flag = 0;
        return -1;
    }
    _svm_cur.voice = v;
    _svm_voice[w].seq = SPUVM_SE_SEQ;
    _svm_voice[w].vabId = vabId;
    _svm_voice[w].progIndex = _svm_cur.program;
    _svm_voice[w].prog = prog;
    _svm_voice[w].vag = (u16)_svm_cur.vag;
    _svm_voice[w].tone = _svm_cur.tone;
    _svm_voice[w].note = note;
    _svm_voice[w].keyState = 1;
    _svm_voice[w].age = 0;
    sub_80158D1C();
    if (_svm_cur.vag == 0xFF) {
        sub_80158F54(w);
    } else {
        sub_801586A4(1, sub_801596EC(note, fine) & 0xFFFF);
    }
    _snd_ev_flag = 0;
    return v;
}

/* SsUtKeyOff: keys off the voice SsUtKeyOn returned, if it still plays the
 * same VAB, program, tone and note. Returns 0, or -1. */
s16 __attribute__((section(".text_8015C6A4"))) sub_8015C6A4(s16 voice, s16 vabId, s16 prog, s16 tone, s16 note)
{
    if (_snd_ev_flag == 1) {
        return -1;
    }
    _snd_ev_flag = 1;
    if ((u16)voice < TM2_SPU_VOICES && _svm_voice[voice].vabId == vabId &&
        _svm_voice[voice].prog == prog && _svm_voice[voice].tone == tone &&
        _svm_voice[voice].note == note) {
        if (_svm_voice[voice].vag == 0xFF) {
            sub_801595E4(voice);
        } else {
            _svm_cur.voice = voice;
            sub_80158B90(0);
        }
        _snd_ev_flag = 0;
        return 0;
    }
    _snd_ev_flag = 0;
    return -1;
}

/* SsUtKeyOnV: SsUtKeyOn on a caller-chosen voice. Returns the voice, or -1. */
s16 __attribute__((section(".text_8015C7D0"))) sub_8015C7D0(s16 voice, s16 vabId, s16 prog, s16 tone, s16 note, s16 fine, s16 voll, s16 volr)
{
    ProgAtr *pg;
    VagAtr *t;

    if (_snd_ev_flag == 1) {
        return -1;
    }
    _snd_ev_flag = 1;
    if ((u16)voice >= TM2_SPU_VOICES || sub_8015D334(vabId, prog) != 0) {
        goto fail;
    }
    _svm_cur.seqSepNo = SPUVM_SE_SEQ;
    _svm_cur.note = note;
    _svm_cur.fine = fine;
    _svm_cur.tone = tone;
    if (voll == volr) {
        _svm_cur.volr = 0x40;
        _svm_cur.voll = voll;
    } else if (volr < voll) {
        _svm_cur.voll = voll;
        _svm_cur.volr = (volr << 6) / voll;
    } else {
        _svm_cur.voll = volr;
        _svm_cur.volr = 0x7F - ((voll << 6) / volr);
    }

    pg = &_svm_pg[prog];
    _svm_cur.mvol = pg->mvol;
    _svm_cur.mpan = pg->mpan;
    _svm_cur.tones = pg->tones;

    t = &_svm_tn[_svm_cur.tone + (_svm_cur.program << 4)];
    _svm_cur.prior = t->prior;
    _svm_cur.vag = t->vag;
    _svm_cur.vol = t->vol;
    _svm_cur.pan = t->pan;
    _svm_cur.center = t->center;
    _svm_cur.shift = t->shift;
    _svm_cur.mode = t->mode;
    _svm_cur.min = t->min;
    _svm_cur.max = t->max;
    if (_svm_cur.vag == 0) {
fail:
        _snd_ev_flag = 0;
        return -1;
    }

    _svm_cur.voice = voice;
    _svm_voice[voice].seq = SPUVM_SE_SEQ;
    _svm_voice[voice].vabId = vabId;
    _svm_voice[voice].progIndex = _svm_cur.program;
    _svm_voice[voice].prog = prog;
    _svm_voice[voice].vag = (u16)_svm_cur.vag;
    _svm_voice[voice].tone = _svm_cur.tone;
    _svm_voice[voice].note = note;
    _svm_voice[voice].keyState = 1;
    _svm_voice[voice].age = 0;
    sub_80158D1C();
    if (_svm_cur.vag == 0xFF) {
        sub_80158F54(voice);
    } else {
        sub_801586A4(1, sub_801596EC(note, fine) & 0xFFFF);
    }
    _snd_ev_flag = 0;
    return voice;
}

/* SsUtKeyOffV: keys off a voice by number. Returns 0, or -1. */
s16 __attribute__((section(".text_8015CBC8"))) sub_8015CBC8(s16 voice)
{
    if (_snd_ev_flag == 1) {
        return -1;
    }
    _snd_ev_flag = 1;
    if ((u16)voice < TM2_SPU_VOICES) {
        _svm_cur.voice = voice;
        sub_80158B90(0);
        _snd_ev_flag = 0;
        return 0;
    }
    _snd_ev_flag = 0;
    return -1;
}

/* SsUtPitchBend: bends a voice SsUtKeyOn started; 0 on success, -1 when the
 * voice does not play that VAB/program. */
s16 __attribute__((section(".text_8015CC3C"))) sub_8015CC3C(s16 voice, s16 vabId, s16 prog, s16 note, s16 pbend)
{
    sub_8015D334(vabId, prog);
    _svm_cur.seqSepNo = SPUVM_SE_SEQ;
    return -(sub_8015AA28(voice, SPUVM_SE_SEQ, vabId, prog, pbend) == 0);
}

/* SsUtChangePitch: retunes a voice from (note) to (newNote, newFine) if it
 * still plays that VAB/program/note. Returns 0, or -1. */
s16 __attribute__((section(".text_8015CCD0"))) sub_8015CCD0(s16 voice, s16 vabId, s16 prog, s16 note, s16 fine, s32 newNote, s32 newFine)
{
    if ((u16)voice < TM2_SPU_VOICES && _svm_voice[voice].vabId == vabId &&
        _svm_voice[voice].prog == prog && _svm_voice[voice].note == note) {
        sub_8015D334(_svm_voice[voice].vabId, _svm_voice[voice].prog);
        _svm_cur.seqSepNo = SPUVM_SE_SEQ;
        _svm_cur.voice = voice;
        _svm_cur.tone = (u8)_svm_voice[voice].tone;
        _svm_sreg_buf[voice].pitch = sub_801596EC((s16)newNote, (s16)newFine);
        _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_PITCH;
        return 0;
    }
    return -1;
}

/* SsUtChangeADSR: sets a voice's envelope if it still plays that
 * VAB/program/note. Returns 0, or -1. */
s16 __attribute__((section(".text_8015CE30"))) sub_8015CE30(s16 voice, s16 vabId, s16 prog, s16 note, u16 adsr1, u16 adsr2)
{
    s32 unused[2]; /* retail's frame reserves 8 bytes nothing uses */

    if ((u16)voice < TM2_SPU_VOICES && _svm_voice[voice].vabId == vabId &&
        _svm_voice[voice].prog == prog && _svm_voice[voice].note == note) {
        _svm_sreg_buf[voice].adsr1 = adsr1;
        _svm_sreg_buf[voice].adsr2 = adsr2;
        _svm_sreg_dirty[voice] |= SVM_SREG_DIRTY_ADSR;
        return 0;
    }
    return -1;
}

/* SsUtAutoVol: starts a volume ramp on a voice (SeAutoVol). Returns 0, or
 * -1 for a bad voice. */
s16 __attribute__((section(".text_8015D134"))) sub_8015D134(s16 voice, s16 from, s16 to, s16 duration)
{
    if ((u16)voice < TM2_SPU_VOICES) {
        sub_80159A24(voice, from, to, duration);
        return 0;
    }
    return -1;
}

/* SsUtAllKeyOff: resets and keys off every voice. */
void __attribute__((section(".text_8015D1DC"))) sub_8015D1DC(void)
{
    s16 i;
    u16 *regs;

    for (i = 0; i < spuVmMaxVoice; i++) {
        _svm_voice[i].age = 0x18;
        _svm_voice[i].vag = 0xFF;
        _svm_voice[i].keyState = 0;
        _svm_voice[i].pitch = 0;
        _svm_voice[i].envx = 0;
        _svm_voice[i].seq = 0xFF;
        _svm_voice[i].progIndex = 0;
        _svm_voice[i].prog = 0;
        _svm_voice[i].tone = 0xFF;

#if TM2_SPU_VOICES > 24
        if (i >= 24) {
            regs = SVM_EXT_SREG + (i - 24) * 8;
        } else
#endif
        regs = (u16 *)_svm_sreg + (s16)(i * 8);
        regs[0] = 0;      /* volL */
        regs[1] = 0;      /* volR */
        regs[3] = 0x200;  /* addr */
        regs[2] = 0x1000; /* pitch */
        regs[4] = 0x80FF; /* adsr1 */
        regs[5] = 0x4000; /* adsr2 */

        _svm_cur.voice = i;
        sub_80158B90(1);
    }
}

/* SpuVmVSetUp: selects VAB `vabId` (program `prog`) as the current VAB
 * (_svm_vh/_svm_pg/_svm_tn). Returns 0, or -1 if the VAB is not open or the
 * program is out of range. */
s32 __attribute__((section(".text_8015D334"))) sub_8015D334(s16 vabId, s16 prog)
{
    if ((u16)vabId >= 16 || _svm_vab_used[vabId] != 1 || prog >= kMaxPrograms) {
        return -1;
    }
    _svm_vh = _svm_vab_vh[vabId];
    _svm_pg = _svm_vab_pg[vabId];
    _svm_tn = _svm_vab_tn[vabId];
    _svm_cur.vabId = vabId;
    _svm_cur.progNo = prog;
    _svm_cur.program = (u8)_svm_pg[prog].reserved1;
    return 0;
}

/* SsUtGetProgAtr: copies a program's attributes. Returns 0, or -1. */
s16 __attribute__((section(".text_8015FF64"))) sub_8015FF64(s16 vabId, s16 prog, ProgAtr *out)
{
    if (_svm_vab_used[vabId] == 1) {
        sub_8015D334(vabId, prog);
        out->tones = _svm_pg[prog].tones;
        out->mvol = _svm_pg[prog].mvol;
        out->prior = _svm_pg[prog].prior;
        out->mode = _svm_pg[prog].mode;
        out->mpan = _svm_pg[prog].mpan;
        out->attr = _svm_pg[prog].attr;
        return 0;
    }
    return -1;
}

/* SsUtFlush: SpuVmFlush under the sound-system lock. */
void __attribute__((section(".text_801619CC"))) sub_801619CC(void)
{
    if (_snd_ev_flag != 1) {
        _snd_ev_flag = 1;
        sub_8015AD64();
        _snd_ev_flag = 0;
    }
}

/* SsUtGetVagAtr: copies a tone's attributes. Returns 0, or -1. */
s16 __attribute__((section(".text_80161A0C"))) sub_80161A0C(s16 vabId, s16 prog, s16 tone, VagAtr *out)
{
    s16 i;

    if (_svm_vab_used[vabId] == 1) {
        sub_8015D334(vabId, prog);
        i = tone + (_svm_cur.program << 4);
        out->prior = _svm_tn[i].prior;
        out->mode = _svm_tn[i].mode;
        out->vol = _svm_tn[i].vol;
        out->pan = _svm_tn[i].pan;
        out->center = _svm_tn[i].center;
        out->shift = _svm_tn[i].shift;
        out->max = _svm_tn[i].max;
        out->min = _svm_tn[i].min;
        out->vibW = _svm_tn[i].vibW;
        out->vibT = _svm_tn[i].vibT;
        out->porW = _svm_tn[i].porW;
        out->porT = _svm_tn[i].porT;
        out->pbmin = _svm_tn[i].pbmin;
        out->pbmax = _svm_tn[i].pbmax;
        out->adsr1 = _svm_tn[i].adsr1;
        out->adsr2 = _svm_tn[i].adsr2;
        out->prog = _svm_tn[i].prog;
        out->vag = _svm_tn[i].vag;
        return 0;
    }
    return -1;
}
