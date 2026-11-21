/* PsyQ SDK LIBSND functions in 80156E08..8015B868 that were not yet in C
 * (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * Adapted from:
 *  - the LSD: Dream Emulator decompilation (CC0 1.0 public domain dedication):
 *    src/psyq/libsnd_vmanager.c (SeAutoVol, SeAutoPan),
 *    src/psyq/libsnd_ssinit.c (SsInitHot)
 *    SPDX-License-Identifier: CC0-1.0
 *  - the psxsdk sources of sotn-decomp (MIT): src/main/psxsdk/libsnd/
 *    ssquit.c (SsQuit), vs_mono.c (SsSetStereo)
 *    SPDX-License-Identifier: MIT
 *  - psy-q-decomp (MIT): src/snd/play.c (_SsSndPlay)
 *    SPDX-License-Identifier: MIT
 * SsGetMVol and SsSetNck were written by hand against the retail code.
 *
 * Recipe: gcc2.5.7 cc1, aspsx 2.21, gp_size 0 (as sdk_libsnd*.c).
 * Library data is declared extern by retail address. */

typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    s16 left;
    s16 right;
} SpuVolume;

typedef struct {
    s32 mask;
    SpuVolume mvol;
    SpuVolume mvolmode;
    SpuVolume mvolx;
    struct {
        SpuVolume volume;
        s32 reverb;
        s32 mix;
    } cd, ext;
} SpuCommonAttr;

typedef struct {
    s16 left;
    s16 right;
} SndVolume;

typedef struct SvmVoice {
    u8 pad_00[0x1C];
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

extern SvmVoice D_801A5398[];      /* _svm_voice (24) */
extern s16 D_801AB93C;             /* _svm_stereo_mono */
#define _svm_voice D_801A5398
#define _svm_stereo_mono D_801AB93C

extern void sub_8015D7F8(SpuCommonAttr *attr);  /* SpuGetCommonAttr */
extern void sub_80156EF8(s32 mode);             /* _SsInit */
extern void sub_8015D428(void);                 /* SpuQuit */
extern void sub_8015DAF8(s16 a, s16 b);         /* _SsSeqPlay */
extern void sub_801599E8(s16 n);

/* @80156E08 SsGetMVol: master volume (0..127) from the SPU's. */
void __attribute__((section(".text_80156E08"))) sub_80156E08(SndVolume *m_vol)
{
    SpuCommonAttr attr;

    sub_8015D7F8(&attr);
    m_vol->left = attr.mvol.left / 129;
    m_vol->right = attr.mvol.right / 129;
}

/* @8015702C SsInitHot */
void __attribute__((section(".text_8015702C"))) sub_8015702C(void)
{
    sub_80156EF8(1);
}

/* @8015704C SsQuit */
void __attribute__((section(".text_8015704C"))) sub_8015704C(void)
{
    sub_8015D428();
}

/* @80157D40 _SsSndPlay */
void __attribute__((section(".text_80157D40"))) sub_80157D40(s16 access, s16 seq)
{
    sub_8015DAF8(access, seq);
}

/* @801583B8 SsSetStereo */
void __attribute__((section(".text_801583B8"))) sub_801583B8(void)
{
    _svm_stereo_mono = 0;
}

/* @80159A00 SsSetNck */
void __attribute__((section(".text_80159A00"))) sub_80159A00(s16 n_clock)
{
    sub_801599E8(n_clock);
}

/* @80159A24 SeAutoVol: start a volume ramp on a voice from `from` to `to`
 * over `duration` ticks (stepped by SetAutoVol). */
void __attribute__((section(".text_80159A24"))) sub_80159A24(s16 voice, s16 from, s16 to, s16 duration)
{
    s16 q;

    if (from == to) {
        return;
    }
    _svm_voice[voice].autoVolActive = 1;
    _svm_voice[voice].autoVolValue = from;
    _svm_voice[voice].autoVolTarget = to;
    if ((from - to < 0 ? to - from : from - to) < duration) {
        q = duration / (from - to);
        _svm_voice[voice].autoVolStep = 1;
        _svm_voice[voice].autoVolInterval = q;
        _svm_voice[voice].autoVolCountdown = q;
    } else {
        q = (from - to) / duration;
        _svm_voice[voice].autoVolInterval = 0;
        _svm_voice[voice].autoVolStep = q;
    }
}

/* @80159FC8 SeAutoPan: the same ramp over the pan fields (SetAutoPan). */
void __attribute__((section(".text_80159FC8"))) sub_80159FC8(s16 voice, s16 from, s16 to, s16 duration)
{
    s16 q;

    if (from == to) {
        return;
    }
    _svm_voice[voice].autoPanActive = 1;
    _svm_voice[voice].autoPanValue = from;
    _svm_voice[voice].autoPanTarget = to;
    if ((from - to < 0 ? to - from : from - to) < duration) {
        q = duration / (from - to);
        _svm_voice[voice].autoPanStep = 1;
        _svm_voice[voice].autoPanInterval = q;
        _svm_voice[voice].autoPanCountdown = q;
    } else {
        q = (from - to) / duration;
        _svm_voice[voice].autoPanInterval = 0;
        _svm_voice[voice].autoPanStep = q;
    }
}

/* @8015B864: empty libsnd entry. */
void __attribute__((section(".text_8015B864"))) sub_8015B864(void)
{
}
