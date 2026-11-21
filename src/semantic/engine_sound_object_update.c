/* SingleTrac engine 8015F42C: sound object update (voice start by type,
 * distance attenuation, pan, doppler), reconstructed C.
 * Written by hand against the retail code (no outside source).
 * Kept in its own unit because its two jump tables (800D1260, 800D12A0)
 * are not adjacent to the one of 8015E57C.
 *
 * Recipe: PsyQ 3.6 cc1 (C), -fvolatile, extern_gp, gp_size 8:
 *   --compiler psyq3.6 --fvolatile on --extern-gp on --delay-slots compiler
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* jump tables of 8015F42C (800D1260, then 800D12A0) */
#pragma tm2_rdata .tm2_800D1260

typedef struct VEnt {
    s16 delta;
    s16 index;
    s32 (*fn)(void *);
} VEnt;

typedef struct { s32 vx, vy, vz; } Vec3;

/* sound request object (vptr at +0x38) */
typedef struct SndObj {
    s32 f00;
    struct SndObj *next;   /* 04 */
    struct SndObj *prev;   /* 08 */
    s32 f0C;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 type;              /* 1C */
    union {
        struct { u32 pri; s32 f24; } p;
        struct SndPri { u32 pri; s32 f24; } s;
    } u20;                 /* 20 */
    s32 f28;
    s32 id;                /* 2C */
    s32 f30;
    s32 f34;
    VEnt *vtbl;            /* 38 */
    s32 f3C;
    s32 f40;
    s32 f44;
    s32 f48;
    s32 f4C;
    s32 f50;
    s32 f54;
    u8 f58;
    u8 f59;
    u8 pad5A[2];
    s32 f5C;
    s32 f60;
    s32 f64;
    s32 f68;
    struct Voice *voices;  /* 6C */
    s32 index;             /* 70 */
    s32 inuse;             /* 74 */
} SndObj;

/* list of sound requests */
typedef struct SndList {
    s32 f00;
    s32 max;               /* 04 */
    s32 count;             /* 08 */
    SndObj *head;          /* 0C */
    SndObj *tail;          /* 10 */
} SndList;

/* CD audio player (vptr at +8) */
typedef struct CdPlayer {
    s32 f00;
    s32 track;             /* 04 */
    VEnt *vtbl;            /* 08 */
} CdPlayer;

typedef struct Voice {
    u8 pad00[4];
    u8 vol;                /* 04 */
    u8 pad05[0x2B];
    struct Voice *next;    /* 30 */
    struct Voice *prev;    /* 34 */
    u8 pad38[0xC];
    s32 f44;               /* 44 */
    u8 pad48[8];
    s16 f50;               /* 50 */
    s16 f52;               /* 52 */
    u8 pad54[4];
    s16 f58;               /* 58 */
    u8 pad5A[8];
    s16 volL;              /* 62 */
    s16 volR;              /* 64 */
    u8 pad66[2];
    s32 f68;               /* 68 */
    u8 pad6C[4];
    s32 f70;               /* 70 */
    s32 f74;               /* 74 */
    s32 f78;               /* 78 */
} Voice;

#define VCALL(o, n) \
    { VEnt *vt_ = (o)->vtbl; vt_[n].fn((char *)(o) + vt_[n].delta); }

extern VEnt sndReqVtbl[];
extern VEnt cdAudioVtbl[];
extern VEnt cdPlayerBaseVtbl[];
extern VEnt sndObjVtbl[];

extern s32 sndDopplerMax, sndDopplerRange, sndListenerClampMin, sndListenerClampMax, D_80180E5C;
extern s32 sndMaxDistance, sndPanMin, sndPanMax, sndListenerDistScale;
extern s32 sndListenerPanView1, sndListenerPan;
extern void *sndListenerViewMatrix;
extern Vec3 *sndListenerEyeOffset;
extern u32 sndListenerDistView1, sndListenerDist;
extern s32 cdAudioEndedFlag, cdAudioPlayCount, cdAudioSector, cdAudioEndSector, cdAudioStartSector;
extern u8 cdAudioLocParam[];
extern u32 cdAudioPlayState;
extern s32 sndPoolFreeCount, sndPoolSearchEnd, sndPoolSearchStart;
extern s16 audioCdVolume[];
extern Vec3 sndListenerRelPos;
extern Vec3 sndListenerWorldPos;
extern SndObj *sndObjPool[];
extern s32 cdAudioTrackToc[];

extern Vec3 *viewGetEyeTrans(s32 view);                 /* viewGetEyeTrans */
extern void *viewGetViewMatrix(s32 view);
extern void mathMulMatVec(void *m, Vec3 *in, Vec3 *out);
extern u32 sub_8013FC44(u32 v);                      /* SquareRoot0 */
extern void taskMgrQueueStart(void *ctx, SndObj *o);
extern void taskMgrQueueStop(void *ctx, SndObj *o);
extern void sub_8015EE44(void *toc);                 /* CdGetToc */
extern s32 sub_8013AD34(void *pos);                  /* CdPosToInt */
extern void *sub_8013AC30(s32 i, void *pos);         /* CdIntToPos */
extern s32 sub_8013A72C(u8 com, u8 *param, u8 *result); /* CdControl */
extern s32 sub_8013A874(u8 com, u8 *param);          /* CdControlF */
extern s32 sub_8013A9B0(u8 com, u8 *param, u8 *result); /* CdControlB */
extern s32 sub_8013A6A4(s32 mode, u8 *result);       /* CdSync */
extern void *sub_8013A6FC(void *cb);                 /* CdReadyCallback */
extern s32 sub_8014510C(s32 mode);                   /* VSync */
extern void sub_80145EF8(char s, char attr, char mode);    /* SsSetSerialAttr */
extern void sub_80145F8C(char s, short l, short r);        /* SsSetSerialVol */

extern s32 sndObjNeedVoices, sndObjUseVolumeScale;
extern Voice *sndObjCurVoice;
extern s32 sndObjVolumeScale;
extern u32 sndObjDistance;
extern s32 sndObjPan, sndObjDistanceVolume, sndObjVolumeLeft, sndObjVolumeRight, sndObjDistanceDelta;
extern s32 a80180E4C;
extern u32 a80180E50;
extern u32 a80180E60;
extern s32 a80180E64, a80180E68, a80180E18;
#pragma tm2_absolute a80180E4C
#pragma tm2_absolute a80180E50
#pragma tm2_absolute a80180E60
#pragma tm2_absolute a80180E64
#pragma tm2_absolute a80180E68
#pragma tm2_absolute a80180E18
extern s16 sub_8015FF64(s16 vab, s16 prog, void *attr);   /* SsUtGetProgAtr */
extern Voice *soundInstAlloc(void);
extern void soundInstSetPitchPos(Voice *v, s32 a);
extern void soundInstLinkActive(Voice *v);
extern void soundInstBindTone(Voice *v, s32 a, s32 b, s32 c, s32 d, s32 e, SndObj *o);
/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern s32 abs(s32);
void sndObjAddVoice(SndObj *o, Voice *v);

#define CUR sndObjCurVoice

static __inline__ s32 CurValid(void)
{
    return CUR != 0;
}

static __inline__ s32 LessThan(s32 a, s32 b)
{
    return a < b;
}

/* 8015F42C: sound object update: start/refresh its voices by type, then
 * compute distance attenuation, pan and doppler for every voice.
 * The final +-limit clamp of voice->f78 keeps the limit and its negation
 * in locals; the upper clamp's then-arm is written -nl (== l2) so the
 * compiler does not swap the arms. */
void __attribute__((section(".text_8015F42C"))) sndObjUpdate(SndObj *o)
{
    s32 i;
    s32 near;
    s32 lim;

    sndObjUseVolumeScale = 0;
    sndObjVolumeScale = 99;
    if (!o->f68) {
        sub_8015FF64(o->f3C, o->f40, &o->f58);
        o->f68 = 1;
    }
    sndObjNeedVoices = 1;
    if (o->voices) {
        CUR = o->voices;
        switch (o->type) {
        case 1: case 6:
            while (CurValid()) {
                soundInstSetPitchPos(CUR, o->f28);
                CUR->f68 = 1;
                CUR = CUR->next;
            }
            sndObjNeedVoices = 0;
            break;
        case 2: case 7:
            while (CurValid()) {
                if (!o->f34) soundInstSetPitchPos(CUR, 99);
                else soundInstSetPitchPos(CUR, ((s32 (*)(s32))o->f34)(o->f28));
                CUR->f68 = 1;
                CUR = CUR->next;
            }
            sndObjNeedVoices = 0;
            break;
        case 4: case 9: case 10:
            VCALL(o, 4)
            sndObjNeedVoices = 1;
            break;
        case 11:
            VCALL(o, 4)
            sndObjNeedVoices = 1;
            sndObjUseVolumeScale = 1;
            sndObjVolumeScale = o->f28;
            break;
        case 0: case 5: case 12:
            while (CurValid()) {
                CUR->f68 = 1;
                CUR = CUR->next;
            }
            sndObjNeedVoices = 0;
            break;
        case 3: case 8: case 13:
            sndObjUseVolumeScale = 1;
            sndObjVolumeScale = o->f28;
            while (CurValid()) {
                CUR->f68 = 1;
                CUR = CUR->next;
            }
            sndObjNeedVoices = 0;
            break;
        case 14:
            break;
        }
    }
    if (sndObjNeedVoices) {
        for (i = 0; LessThan(i, o->f58); i++) {
            CUR = soundInstAlloc();
            if (!CUR) break;
            soundInstBindTone(CUR, o->f3C, o->f40, i, o->f44, o->f48, o);
            sndObjAddVoice(o, CUR);
            soundInstLinkActive(CUR);
            CUR->f68 = 1;
            switch (o->type) {
            case 0: case 5:
                CUR->f44 = 2;
                break;
            case 1: case 6:
                soundInstSetPitchPos(CUR, o->f28);
                CUR->f44 = 2;
                break;
            case 2: case 7:
                if (!o->f34) soundInstSetPitchPos(CUR, 99);
                else soundInstSetPitchPos(CUR, ((s32 (*)(s32))o->f34)(o->f28));
                CUR->f44 = 2;
                break;
            case 4: case 9:
                CUR->f44 = 2;
                CUR->f52 = 1;
                CUR->f50 = o->f28;
                break;
            case 12:
                CUR->f44 = 1;
                break;
            case 13:
                sndObjUseVolumeScale = 1;
                sndObjVolumeScale = o->f28;
                CUR->f44 = 1;
                break;
            case 10:
                CUR->f44 = 0;
                break;
            case 3: case 8:
                CUR->f44 = 2;
                sndObjUseVolumeScale = 1;
                sndObjVolumeScale = o->f28;
                break;
            case 11:
            default:
                CUR->f44 = 0;
                sndObjUseVolumeScale = 1;
                sndObjVolumeScale = o->f28;
                break;
            }
        }
    }
    sndObjDistance = o->u20.p.pri < a80180E60 ? o->u20.p.pri : a80180E60;
    sndObjPan = a80180E64 > o->u20.p.f24 ? a80180E64 : o->u20.p.f24;
    lim = a80180E68;
    sndObjPan = o->u20.p.f24 < lim ? o->u20.p.f24 : lim;
    sndObjDistanceVolume = (a80180E60 - sndObjDistance) * a80180E18 / a80180E60;
    if (sndObjPan < 0) {
        sndObjVolumeLeft = sndObjDistanceVolume;
        sndObjVolumeRight = sndObjDistanceVolume - sndObjDistanceVolume * -sndObjPan / a80180E68;
    } else {
        sndObjVolumeRight = sndObjDistanceVolume;
        sndObjVolumeLeft = sndObjDistanceVolume - sndObjDistanceVolume * sndObjPan / a80180E68;
    }
    sndObjVolumeLeft = o->f59 * sndObjVolumeLeft / 127;
    sndObjVolumeRight = o->f59 * sndObjVolumeRight / 127;
    CUR = o->voices;
    while (CurValid()) {
        CUR->volL = CUR->vol * sndObjVolumeLeft / 127;
        CUR->volR = CUR->vol * sndObjVolumeRight / 127;
        if (sndObjUseVolumeScale) {
            CUR->volL = CUR->volL * sndObjVolumeScale / 99;
            CUR->volR = CUR->volR * sndObjVolumeScale / 99;
        }
        CUR->f74 = CUR->f70;
        CUR->f70 = o->u20.p.pri;
        if (o->f44) {
            if (a80180E50 >= o->u20.p.pri) {
                sndObjDistanceDelta = CUR->f74 - CUR->f70;
                CUR->f78 = sndObjDistanceDelta * o->f44 / timerGetFrameTicks();
                if (CUR->f78 > a80180E4C) CUR->f78 = 0;
                near = 0;
                if (CUR->f78) near = abs(CUR->f78) < a80180E4C;
                if (near) CUR->f58 = 1;
                {
                    s32 l2 = a80180E4C;
                    s32 nl = -l2;
                    s32 t;
                    s32 u;

                    t = nl >= CUR->f78 ? nl : CUR->f78;
                    CUR->f78 = t;
                    u = l2 < CUR->f78 ? -nl : CUR->f78;
                    CUR->f78 = u;
                }
            } else {
                CUR->f78 = 0;
            }
        }
        CUR = CUR->next;
    }
}


