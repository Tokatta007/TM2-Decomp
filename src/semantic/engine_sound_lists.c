/* SingleTrac engine sound code 8015DB28..8015FF60 (sound request lists,
 * listener transform, CD audio player, sound object pool), reconstructed C.
 * Written by hand against the retail code (no outside source).
 *
 * Retail objects are C++ classes (GCC 2.x vtables: {s16 delta; s16 index;
 * fn} entries, vptr at +0x38 for sound objects, +8 for the CD player); here
 * they are plain structs with the vtable pointer as a field and VCALL().
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

#include "semantic/spu_voices.h"   /* request 87: the request's tag (portSndReqTag) */
#include "semantic/port_cars.h"   /* the request pool's size (players.h TM2_MAX_SOUND_REQUESTS; request 33-4) */
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: the listeners are the local humans' views */
#endif

/* jump table of 8015E57C (retail address) */
#pragma tm2_rdata .tm2_800D115C

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

void sndReqReset(SndObj *o, s32 id, s32 f30);
void sndListInit(SndList *l);
void sndListSetMax(SndList *l, s32 max);
void sndListInsertBefore(SndList *l, SndObj *o, SndObj *at);
void sndListInsertByPriority(SndList *l, SndObj *o);
void sndListRemove(SndList *l, SndObj *o);
void cdPlayerBaseInit(CdPlayer *p);
CdPlayer *cdPlayerBaseCtor(CdPlayer *p);
s32 cdAudioSync(s32 mode, u8 *result);
void cdAudioPlayRange(CdPlayer *p, s32 start, s32 end);
void cdAudioReadyCallback(s32 intr, u8 *result);
void cdAudioRestart(s32 mode);
SndObj *sndReqCtor(SndObj *o);
SndObj *sndListMergeById(SndList *l, SndObj *o);
SndObj *sndListReplaceTail(SndList *l, SndObj *o);
void sndPoolFree(SndObj *o);
void sndObjSetup(SndObj *o, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);

/* 8015DB28: sound request base constructor. */
SndObj * __attribute__((section(".text_8015DB28"))) sndReqCtor(SndObj *o)
{
    o->vtbl = sndReqVtbl;
    sndReqReset(o, 0, 0);
    return o;
}

/* 8015DB68: listener defaults (distance clamp/scale). */
void __attribute__((section(".text_8015DB68"))) sndListenerInitDefaults(void)
{
    sndMaxDistance = 0x16B7;
    sndListenerClampMax = 0xD1D;
    sndListenerClampMin = -0xD1D;
    sndPanMax = 0x280;
    sndPanMin = -0x280;
    sndListenerDistScale = 0x400;
    D_80180E5C = 0;
    sndDopplerRange = 0x16B7;
    sndDopplerMax = 0x1F4;
}

/* 8015DBB0: sound request reset. */
void __attribute__((section(".text_8015DBB0"))) sndReqReset(SndObj *o, s32 id, s32 f30)
{
    o->f10 = 0;
    o->f14 = 0;
    o->next = 0;
    o->prev = 0;
    o->f18 = -1;
    o->u20.p.pri = 0;
    o->u20.p.f24 = 0;
    o->f28 = 0;
    o->id = id;
    o->f30 = f30;
    o->f34 = 0;
}

#define LISTEN_EYE(view)                                                   \
    sndListenerEyeOffset = viewGetEyeTrans(view);                                       \
    sndListenerViewMatrix = viewGetViewMatrix(view);                                       \
    sndListenerWorldPos.vx = pos->vx + sndListenerEyeOffset->vx;                              \
    sndListenerWorldPos.vy = pos->vy + sndListenerEyeOffset->vy;                              \
    sndListenerWorldPos.vz = pos->vz + sndListenerEyeOffset->vz;

#define LISTEN_CLAMP(raw, dist)                                            \
    mathMulMatVec(sndListenerViewMatrix, &sndListenerWorldPos, &sndListenerRelPos);                    \
    raw = sndListenerRelPos.vx;                                                   \
    sndListenerRelPos.vx = sndListenerRelPos.vx / sndListenerDistScale;                            \
    sndListenerRelPos.vx = sndListenerRelPos.vx <= sndListenerClampMax ? sndListenerRelPos.vx : sndListenerClampMax; \
    sndListenerRelPos.vx = sndListenerRelPos.vx < sndListenerClampMin ? sndListenerClampMin : sndListenerRelPos.vx; \
    sndListenerRelPos.vy = sndListenerRelPos.vy <= sndListenerClampMax ? sndListenerRelPos.vy : sndListenerClampMax; \
    sndListenerRelPos.vy = sndListenerRelPos.vy < sndListenerClampMin ? sndListenerClampMin : sndListenerRelPos.vy; \
    sndListenerRelPos.vz = sndListenerRelPos.vz <= sndListenerClampMax ? sndListenerRelPos.vz : sndListenerClampMax; \
    sndListenerRelPos.vz = sndListenerRelPos.vz < sndListenerClampMin ? sndListenerClampMin : sndListenerRelPos.vz; \
    dist = sndListenerRelPos.vx * sndListenerRelPos.vx + sndListenerRelPos.vy * sndListenerRelPos.vy + sndListenerRelPos.vz * sndListenerRelPos.vz; \
    dist = sub_8013FC44(dist);

/* 8015DBE8: transform a world position into listener space for a view
 * (both views when view == -1, keeping the nearer); sndListenerRelPos = clamped
 * relative position, sndListenerDist = its distance.
 * TM2_PORT_HUMANS (HUMANS16_PLAN.md H3): view -1 also takes views 2..3 when a
 * local human is on them (the port's split screen with 3-4 players; views 0/1
 * as retail). Sound only: no simulation code reads the listener values. */
void __attribute__((section(".text_8015DBE8"))) sndListenerTransform(Vec3 *pos, s32 view)
{
#ifdef TM2_PORT_HUMANS
    s32 v;

#endif
    if (view != -1) {
        LISTEN_EYE(view)
        LISTEN_CLAMP(sndListenerPan, sndListenerDist)
    } else {
        LISTEN_EYE(0)
        LISTEN_CLAMP(sndListenerPan, sndListenerDist)
        LISTEN_EYE(1)
        sndListenerRelPos.vx = 0;
        sndListenerRelPos.vy = 0;
        sndListenerRelPos.vz = 0;
        LISTEN_CLAMP(sndListenerPanView1, sndListenerDistView1)
        if (sndListenerDistView1 < sndListenerDist) {
            sndListenerDist = sndListenerDistView1;
            sndListenerPan = sndListenerPanView1;
        }
#ifdef TM2_PORT_HUMANS
        for (v = 2; v < TM2_MAX_VIEWS; v++) {
            if (portLocalHuman(v) < 0)
                continue;
            LISTEN_EYE(v)
            LISTEN_CLAMP(sndListenerPanView1, sndListenerDistView1)
            if (sndListenerDistView1 < sndListenerDist) {
                sndListenerDist = sndListenerDistView1;
                sndListenerPan = sndListenerPanView1;
            }
        }
#endif
    }
}

/* 8015E0D4: sound list constructor. */
SndList * __attribute__((section(".text_8015E0D4"))) sndListCtor(SndList *l)
{
    sndListInit(l);
    return l;
}

/* 8015E100: sound list constructor with a capacity. */
SndList * __attribute__((section(".text_8015E100"))) sndListCtorWithMax(SndList *l, s32 max)
{
    sndListInit(l);
    sndListSetMax(l, max);
    return l;
}

/* 8015E144: sound list init (empty, capacity 0). */
void __attribute__((section(".text_8015E144"))) sndListInit(SndList *l)
{
    sndListSetMax(l, 0);
    l->head = 0;
    l->tail = 0;
    l->count = 0;
}

/* 8015E17C: sound list clear. */
void __attribute__((section(".text_8015E17C"))) sndListClear(SndList *l)
{
    l->head = 0;
    l->tail = 0;
    l->count = 0;
}

/* 8015E190: sound list set capacity. */
void __attribute__((section(".text_8015E190"))) sndListSetMax(SndList *l, s32 max)
{
    l->max = max;
}

/* 8015E19C: merge a request into the list (same id: update it; else
 * replace the lowest-priority entry). Returns the entry used, or 0. */
static __inline__ s32 SndHigher(SndObj *a, SndObj *b)
{
    return a->u20.p.pri >= b->u20.p.pri;
}

SndObj * __attribute__((section(".text_8015E19C"))) sndListMergeById(SndList *l, SndObj *o)
{
    SndObj *n = l->head;
    SndObj *low = o;
    s32 restart;

loop:
    if (n) {
        if (SndHigher(n, low)) low = n;
        if (n->id == o->id) goto out;
        n = n->next;
        goto loop;
    }
out:
    if (!n && o == low) {
        VCALL(low, 2)
        return 0;
    }
    if (n) {
        __builtin_memcpy(&n->u20, &o->u20, 8);
        n->f28 = o->f28;
        n->f30 = o->f30;
        VCALL(o, 2)
        return n;
    }
    __builtin_memcpy(&low->u20, &o->u20, 8);
    low->f28 = o->f28;
    low->id = o->id;
    low->f30 = o->f30;
    restart = 0;
    if (o->type == 5 || o->type == 6 || o->type == 7 || o->type == 8 || o->type == 9) {
        if (o->f30 == 1) restart = 1;
    }
    if (restart) {
        VCALL(low, 4)
        VCALL(low, 3)
    }
    VCALL(o, 2)
    return low;
}

/* 8015E3A4: insert a request before `at` (append when at == 0). */
void __attribute__((section(".text_8015E3A4"))) sndListInsertBefore(SndList *l, SndObj *o, SndObj *at)
{
    if (at) {
        if (at->prev) { SndObj *q = at->prev; q->next = o; }
        o->prev = at->prev;
        o->next = at;
        at->prev = o;
        ++l->count;
        if (!o->prev) l->head = o;
        if (!o->next) l->tail = o;
    } else {
        if (!l->count) {
            l->head = o;
            l->tail = o;
            o->next = 0;
            o->prev = 0;
        } else {
            l->tail->next = o;
            o->next = 0;
            o->prev = l->tail;
            l->tail = o;
        }
        ++l->count;
    }
}

static __inline__ SndObj *SndListFind(SndList *l, SndObj *o)
{
    SndObj *n;
    n = l->head;
loop:
    if (n) {
        if (n->u20.p.pri > o->u20.p.pri) return n;
        n = n->next;
        goto loop;
    }
    return 0;
}

/* 8015E484: insert a request by priority. */
void __attribute__((section(".text_8015E484"))) sndListInsertByPriority(SndList *l, SndObj *o)
{
    SndObj *at = 0;
    if (l->count) at = SndListFind(l, o);
    sndListInsertBefore(l, o, at);
}

/* 8015E4EC: replace the tail (lowest) request if the new one ranks higher. */
SndObj * __attribute__((section(".text_8015E4EC"))) sndListReplaceTail(SndList *l, SndObj *o)
{
    if (l->tail->u20.p.pri > o->u20.p.pri) {
        sndListRemove(l, l->tail);
        sndListInsertByPriority(l, o);
        return o;
    }
    VCALL(o, 2)
    return 0;
}

/* 8015E57C: submit a request to the list. */
void __attribute__((section(".text_8015E57C"))) sndListSubmit(SndList *l, SndObj *o, void *ctx)
{
    SndObj *r;
    s32 t;
    if (!l->max) goto release;
    if (l->count < l->max) {
        sndListInsertByPriority(l, o);
        taskMgrQueueStart(ctx, o);
        return;
    }
    t = o->type;
    switch (t) {
    case 0: case 1: case 2: case 3: case 4:
    case 5: case 6: case 7: case 8: case 9:
        r = sndListMergeById(l, o);
        break;
    case 10: case 11:
        r = sndListReplaceTail(l, o);
        break;
    case 12: case 13:
        goto release;
    case 14:
    default:
        return;
    }
    if (r) taskMgrQueueStart(ctx, r);
    return;
release:
    VCALL(o, 2)
}

/* 8015E664: remove every request. */
void __attribute__((section(".text_8015E664"))) sndListRemoveAll(SndList *l)
{
    SndObj *n = l->head;
    SndObj *cur;
loop:
    if (n) {
        cur = n;
        n = n->next;
        sndListRemove(l, cur);
        goto loop;
    }
}

/* 8015E6B4: unlink a request and release it. */
void __attribute__((section(".text_8015E6B4"))) sndListRemove(SndList *l, SndObj *o)
{
    VEnt *vt;
    s32 d;
    --l->count;
    if (o->prev) o->prev->next = o->next;
    if (o->next) o->next->prev = o->prev;
    if (o == l->head) l->head = o->next;
    if (o == l->tail) l->tail = o->prev;
    vt = o->vtbl;
    d = vt[2].delta;
    o->next = 0;
    o->prev = 0;
    vt[2].fn((char *)o + d);
}

/* 8015E780: stop requests by id (-1: every finished one). */
void __attribute__((section(".text_8015E780"))) sndListStopById(SndList *l, void *ctx, s32 id)
{
    SndObj *n = l->head;
    VEnt *vt;
    if (id == -1) {
    loop1:
        if (n) {
            vt = n->vtbl;
            if (!vt[5].fn((char *)n + vt[5].delta)) taskMgrQueueStop(ctx, n);
            n = n->next;
            goto loop1;
        }
    } else {
    loop2:
        if (n) {
            if (n->id == id) goto found;
            n = n->next;
            goto loop2;
        }
    }
    return;
found:
    taskMgrQueueStop(ctx, n);
}

/* 8015E82C: remove finished requests. */
void __attribute__((section(".text_8015E82C"))) sndListPurgeFinished(SndList *l)
{
    SndObj *n = l->head;
    SndObj *cur;
    VEnt *vt;
loop:
    if (n) {
        vt = n->vtbl;
        if (vt[5].fn((char *)n + vt[5].delta)) {
            cur = n;
            n = n->next;
            sndListRemove(l, cur);
        } else {
            n = n->next;
        }
        goto loop;
    }
}

/* 8015E8B0: CD audio player constructor. */
CdPlayer * __attribute__((section(".text_8015E8B0"))) cdAudioCtor(CdPlayer *p)
{
    cdPlayerBaseCtor(p);
    p->vtbl = cdAudioVtbl;
    cdAudioPlayState = 0;
    return p;
}

/* 8015E8EC: CD audio player destructor body. */
void __attribute__((section(".text_8015E8EC"))) cdAudioInit(CdPlayer *p)
{
    cdPlayerBaseInit(p);
    cdAudioEndedFlag = 0;
}

/* 8015E910: CD audio: play a track (seek to it, set the repeat range). */
s32 __attribute__((section(".text_8015E910"))) cdAudioPlayTrack(CdPlayer *p, s32 track)
{
    u8 changed;
    s32 start;
    changed = p->track != track;
    p->track = track;
    if (cdAudioPlayState < 2 || cdAudioPlayState == 4 || changed) {
        cdAudioSync(0, 0);
        sub_8015EE44(cdAudioTrackToc);
        start = sub_8013AD34(&cdAudioTrackToc[track]);
        cdAudioPlayRange(p, start, sub_8013AD34(&cdAudioTrackToc[track + 1]) - 150);
        cdAudioPlayState = 1;
        sub_8013AC30(cdAudioSector, cdAudioLocParam);
    }
    cdAudioEndedFlag = 0;
    return track;
}

/* 8015E9F0: CD audio: pause. */
s32 __attribute__((section(".text_8015E9F0"))) cdAudioPause(CdPlayer *p, s32 arg)
{
    if (cdAudioPlayState == 1) {
        sub_8013A6FC(0);
        cdAudioSync(0, 0);
        sub_8013AC30(cdAudioSector, cdAudioLocParam);
        cdAudioPlayState = 4;
    }
    return arg;
}

/* 8015EA54: CD audio: resume. */
s32 __attribute__((section(".text_8015EA54"))) cdAudioResume(CdPlayer *p, s32 arg)
{
    u8 mode;
    if (cdAudioPlayState == 4) {
        mode = 5;
        cdAudioSync(0, 0);
        sub_8013A72C(0xE, &mode, 0);
        sub_8013A6FC((void *)cdAudioReadyCallback);
        cdAudioSync(0, 0);
        sub_8013A72C(3, cdAudioLocParam, 0);
        cdAudioPlayState = 1;
    }
    return arg;
}

/* 8015EAE4: CdSync wrapper. */
s32 __attribute__((section(".text_8015EAE4"))) cdAudioSync(s32 mode, u8 *result)
{
    return sub_8013A6A4(mode, result);
}

/* 8015EB04: CD audio: start playing sectors start..end (report mode). */
void __attribute__((section(".text_8015EB04"))) cdAudioPlayRange(CdPlayer *p, s32 start, s32 end)
{
    u8 mode;
    cdAudioStartSector = start;
    cdAudioSector = start;
    cdAudioEndSector = end;
    cdAudioPlayCount = 0;
    mode = 5;
    cdAudioSync(0, 0);
    sub_8013A9B0(0xE, &mode, 0);
    sub_8013A6FC((void *)cdAudioReadyCallback);
    cdAudioRestart(3);
    sub_80145EF8(0, 0, 1);
    sub_80145EF8(0, 1, 1);
    sub_80145F8C(0, audioCdVolume[0], audioCdVolume[0]);
}

/* 8015EC2C: CD ready callback: track the play position, replay at the end. */
void __attribute__((section(".text_8015EC2C"))) cdAudioReadyCallback(s32 intr, u8 *result)
{
    u8 pos[4];
    if (intr == 1) {
        if (!(result[4] & 0x80)) {
            pos[0] = result[3];
            pos[1] = result[4];
            pos[2] = 0;
            cdAudioSector = sub_8013AD34(pos);
        }
        if (cdAudioSector > cdAudioEndSector || cdAudioSector < cdAudioStartSector) {
            cdAudioRestart(3);
            cdAudioEndedFlag = intr;
        }
    } else {
        cdAudioRestart(3);
    }
}

/* 8015ECD8 cdAudioVSyncPoll: every 15 frames poll the CD position, replay at the end. */
void __attribute__((section(".text_8015ECD8"))) cdAudioVSyncPoll(void)
{
    u8 result[8];
    s32 r, v;
    if (sub_8014510C(-1) % 15 == 0) {
        r = sub_8013A6A4(1, result);
        if (r == 5) {
            cdAudioRestart(6);
        } else if (r == 2) {
            if (cdAudioSector > cdAudioEndSector || cdAudioSector < cdAudioStartSector) {
                cdAudioRestart(6);
            } else {
                v = sub_8013AD34(&result[5]);
                if (v > 0) cdAudioSector = v;
                cdAudioSync(0, 0);
                sub_8013A874(0x11, 0);
            }
        }
    }
}

/* 8015EDAC: CD audio: issue a play/read command from the start sector. */
void __attribute__((section(".text_8015EDAC"))) cdAudioRestart(s32 mode)
{
    u8 pos[4];
    sub_8013AC30(cdAudioStartSector, pos);
    cdAudioSync(0, 0);
    sub_8013A72C(mode, pos, 0);
    cdAudioSector = cdAudioStartSector;
    cdAudioPlayCount++;
}

/* 8015EE0C: CD audio player (empty virtual). */
void __attribute__((section(".text_8015EE0C"))) cdAudioSlot0Stub(void)
{
}

/* 8015EE14: clear the CD "track ended" flag. */
void __attribute__((section(".text_8015EE14"))) cdAudioClearEnded(void)
{
    cdAudioEndedFlag = 0;
}

/* 8015EE20: CD "track ended" flag. */
s32 __attribute__((section(".text_8015EE20"))) cdAudioHasEnded(void)
{
    return cdAudioEndedFlag;
}

/* 8015EE2C: CD play count. */
s32 __attribute__((section(".text_8015EE2C"))) cdAudioGetPlayCount(void)
{
    return cdAudioPlayCount;
}

/* 8015EE38: CD current sector. */
s32 __attribute__((section(".text_8015EE38"))) cdAudioGetSector(void)
{
    return cdAudioSector;
}

/* 8015F08C: CD player base destructor body (empty). */
void __attribute__((section(".text_8015F08C"))) cdPlayerBaseInit(CdPlayer *p)
{
}

/* 8015F094: CD player base constructor. */
CdPlayer * __attribute__((section(".text_8015F094"))) cdPlayerBaseCtor(CdPlayer *p)
{
    p->vtbl = cdPlayerBaseVtbl;
    p->track = -1;
    return p;
}

/* 8015F0B4: sound object constructor. */
SndObj * __attribute__((section(".text_8015F0B4"))) sndObjCtor(SndObj *o)
{
    sndReqCtor(o);
    o->vtbl = sndObjVtbl;
    return o;
}

/* 8015F0EC: reset the sound object pool (35 entries; TM2_MAX_SOUND_REQUESTS, raised by TM2_PORT_CARS: request
 * 33 step 4, players.h). */
void __attribute__((section(".text_8015F0EC"))) sndPoolReset(void)
{
    s32 i;
    sndPoolSearchEnd = TM2_MAX_SOUND_REQUESTS - 1;
    sndPoolSearchStart = 0;
    sndPoolFreeCount = TM2_MAX_SOUND_REQUESTS;
    for (i = 0; i < sndPoolFreeCount; i++) sndObjPool[i]->inuse = 0;
}

/* 8015F194: allocate a sound object from the pool. */
SndObj * __attribute__((section(".text_8015F194"))) sndPoolAlloc(void)
{
    SndObj *o = 0;
    s32 i, j;
    if (sndPoolFreeCount) {
#if TM2_MAX_SOUND_REQUESTS > TM2_RETAIL_SOUND_REQUESTS
        /* request 33-4: the pool holds TM2_MAX_SOUND_REQUESTS, the allocator hands out the first
           TM2_SOUND_REQUESTS_LIVE (35 while portCarOptions bit 3 is clear: a search past them finds nothing and
           changes nothing, as retail's empty free count) */
        for (i = sndPoolSearchStart; i <= sndPoolSearchEnd && i < TM2_SOUND_REQUESTS_LIVE; i++) {
            if (!sndObjPool[i]->inuse) {
                o = sndObjPool[i];
                sndPoolFreeCount--;
                for (j = sndPoolSearchStart + 1; j <= sndPoolSearchEnd && j < TM2_SOUND_REQUESTS_LIVE; j++) {
#else
        for (i = sndPoolSearchStart; i <= sndPoolSearchEnd; i++) {
            if (!sndObjPool[i]->inuse) {
                o = sndObjPool[i];
                sndPoolFreeCount--;
                for (j = sndPoolSearchStart + 1; j <= sndPoolSearchEnd; j++) {
#endif
                    if (!sndObjPool[j]->inuse) {
                        sndPoolSearchStart = j;
                        break;
                    }
                }
                o->index = i;
                o->inuse = 1;
                o->f50 = 0;
                o->f4C = 0;
#if TM2_SPU_VOICES > 24
                if (i < TM2_VOICE_REQS)
                    portSndReqTag[i] = portSoundTag;    /* request 87: whose play made it (0 none) */
#endif
                break;
            }
        }
    }
    return o;
}

/* 8015F27C: return a sound object to the pool. */
void __attribute__((section(".text_8015F27C"))) sndPoolFree(SndObj *o)
{
    if (o) {
        o->inuse = 0;
        sndPoolSearchStart = o->index < sndPoolSearchStart ? o->index : sndPoolSearchStart;
        sndPoolSearchEnd = sndPoolSearchEnd < o->index ? o->index : sndPoolSearchEnd;
        o->f50 = 0;
        o->f4C = 0;
        sndPoolFreeCount++;
    }
}

/* 8015F2E0: sound object setup from explicit values. */
void __attribute__((section(".text_8015F2E0"))) sndObjSetup(SndObj *o, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f)
{
    sndReqReset(o, 0, 0);
    o->f18 = a;
    o->f3C = b;
    o->f40 = c;
    o->f44 = d;
    o->f48 = e;
    o->type = f;
    o->voices = 0;
    o->f68 = 0;
}

/* 8015F378: sound object setup from a sound table record. */
void __attribute__((section(".text_8015F378"))) sndObjSetupFromRecord(SndObj *o, s8 *rec, s32 id, s32 f30)
{
    sndReqReset(o, id, f30);
    o->f18 = rec[4];
    o->f3C = rec[5];
    o->f40 = rec[6];
    o->f44 = rec[7];
    o->f48 = rec[8];
    o->type = *(s32 *)(rec + 12);
    o->voices = 0;
    o->f68 = 0;
}

/* 8015FCE4: stop every voice of a sound object. */
void __attribute__((section(".text_8015FCE4"))) sndObjStopVoices(SndObj *o)
{
    struct Voice *v;
    extern void soundInstStop(struct Voice *);
    extern void soundInstFree(struct Voice *);
    extern void sndObjRemoveVoice(SndObj *, struct Voice *);
    v = o->voices;
    while (v) {
        soundInstStop(v);
        sndObjRemoveVoice(o, v);
        soundInstFree(v);
        v = o->voices;
    }
    o->voices = 0;
}



/* 8015FD50: append a voice to a sound object. */
void __attribute__((section(".text_8015FD50"))) sndObjAddVoice(SndObj *o, Voice *v)
{
    Voice *n;
    if (o->voices) {
        n = o->voices;
    loop:
        if (n->next) {
            n = n->next;
            goto loop;
        }
        n->next = v;
        v->prev = n;
    } else {
        o->voices = v;
    }
}

/* 8015FDA0: unlink a voice from a sound object. */
void __attribute__((section(".text_8015FDA0"))) sndObjRemoveVoice(SndObj *o, Voice *v)
{
    s32 alone = !v->prev && !v->next;
    if (alone) {
        o->voices = 0;
        return;
    }
    if (!v->prev) {
        o->voices = v->next;
        v->next->prev = 0;
    } else if (!v->next) {
        v->prev->next = 0;
        v->prev = 0;
        return;
    } else {
        v->prev->next = v->next;
        v->next->prev = v->prev;
        v->prev = 0;
    }
    v->next = 0;
}

/* 8015FE58: release a sound object (stop it, return it to the pool). */
void __attribute__((section(".text_8015FE58"))) sndObjRelease(SndObj *o)
{
    VCALL(o, 4)
    sndPoolFree(o);
}

/* 8015FEA4: sound object finished (no voices left). */
s32 __attribute__((section(".text_8015FEA4"))) sndObjIsFinished(SndObj *o)
{
    return o->voices == 0;
}

/* 8015FEB0: sound object in use. */
s32 __attribute__((section(".text_8015FEB0"))) sndObjIsInUse(SndObj *o)
{
    return o->inuse == 1;
}

/* 8015FEC4: sound object constructor from explicit values. */
SndObj * __attribute__((section(".text_8015FEC4"))) sndObjCtorWithValues(SndObj *o, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f)
{
    sndReqCtor(o);
    o->vtbl = sndObjVtbl;
    sndObjSetup(o, a, b, c, d, e, f);
    return o;
}

