/* Library code 80160000..80164000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * SingleTrac engine code (game recipe: psyq3.6, -fvolatile, extern_gp):
 *  - task manager methods on the shell object soundManagerObject (task array, run list);
 *  - the sound-instance driver (soundOn / soundOffOn / voice allocation /
 *    per-frame update with priority sort) on top of libsnd/libspu. */

typedef signed char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;

/* ---- task manager ---------------------------------------------------- */

typedef struct TaskVEntry { s16 delta; s16 index; s32 (*fn)(void *); } TaskVEntry;

typedef struct Task {
    u8 pad0[12];
    s32 state;               /* +0x0C: queued request: 0 = start, 1 = stop */
    struct Task *next;       /* +0x10 */
    struct Task *prev;       /* +0x14 */
    u8 pad18[8];
    u32 level;               /* +0x20 */
    u8 pad24[0x14];
    TaskVEntry *vtable;      /* +0x38 */
} Task;

typedef struct TaskManager {
    u8 pad0[4];
    Task *pending[45];       /* +0x04 */
    s32 npending;            /* +0xB8: index of last, -1 = none */
    Task *head;              /* +0xBC */
    Task *tail;              /* +0xC0 */
    s32 count;               /* +0xC4 */
    TaskVEntry *vtable;      /* +0xC8 */
} TaskManager;

#define VCALL(obj, vt, slot) ((vt)[slot].fn((u8 *)(obj) + (vt)[slot].delta))

extern s32 sndVabIds[16];
extern u32 sndMaxDistance;
#pragma tm2_absolute sndMaxDistance
extern Task *taskMgrCheckCursor;
extern Task *taskMgrRunNext;
extern Task *taskMgrRunCurrent;
extern Task *taskMgrStopNext;
extern Task *taskMgrStopCurrent;

void taskMgrListError(TaskManager *m);
void taskMgrErrorStub(TaskManager *m);
Task *taskMgrFindTask(TaskManager *m, Task *t);
void taskMgrCheckList(TaskManager *m);
void taskMgrUnlink(TaskManager *m, Task *t);
void taskMgrLink(TaskManager *m, Task *t);
void taskMgrRunFrame(TaskManager *m);
void soundDriverUpdate(void);

/* Task manager constructor body: clear 16 slots, empty pending array and list. */
void __attribute__((section(".text_8016006C"))) taskMgrInit(TaskManager *m)
{
    s32 i;

    for (i = 0; i < 16; i++)
        sndVabIds[i] = -1;
    for (i = 0; i < 45; i++)
        m->pending[i] = 0;
    m->npending = -1;
    m->head = 0;
    m->tail = 0;
    m->count = 0;
}

/* Consistency check: walk the run list; if its length differs from count, report. */
void __attribute__((section(".text_801600C8"))) taskMgrCheckList(TaskManager *m)
{
    s32 n = 0;

    taskMgrCheckCursor = m->head;
loop:
    if (taskMgrCheckCursor) {
        taskMgrCheckCursor = taskMgrCheckCursor->next;
        n++;
        goto loop;
    }
    if (n != m->count)
        taskMgrListError(m);
}

/* Find t in the run list (returns t or 0). */
Task * __attribute__((section(".text_80160120"))) taskMgrFindTask(TaskManager *m, Task *t)
{
    Task *p = m->head;

loop:
    if (p && t != p) {
        p = p->next;
        goto loop;
    }
    return p;
}

/* List-corruption hook (calls an empty function). */
void __attribute__((section(".text_8016014C"))) taskMgrListError(TaskManager *m)
{
    taskMgrErrorStub(m);
}

void __attribute__((section(".text_8016016C"))) taskMgrErrorStub(TaskManager *m)
{
}

/* Queue t for start (state 0) if its level (+0x20) is enabled (<= sndMaxDistance). */
void __attribute__((section(".text_80160174"))) taskMgrQueueStart(TaskManager *m, Task *t)
{
    s32 ok;

    ok = t->level <= sndMaxDistance;
    if (ok) {
        if (m->npending == -1)
            m->npending = 0;
        t->state = 0;
        m->pending[m->npending] = t;
        m->npending++;
        if (m->npending > 45)
            m->npending--;
    }
}

/* Queue t for stop (state 1). */
void __attribute__((section(".text_80160204"))) taskMgrQueueStop(TaskManager *m, Task *t)
{
    if (m->npending == -1)
        m->npending = 0;
    t->state = 1;
    m->pending[m->npending] = t;
    m->npending++;
    if (m->npending > 45)
        m->npending--;
}

/* Append t to the run list (unless already in it). */
void __attribute__((section(".text_80160278"))) taskMgrLink(TaskManager *m, Task *t)
{
    if (t) {
        if (m->head == 0) {
            m->head = t;
        } else {
            if (taskMgrFindTask(m, t))
                goto check;
            m->tail->next = t;
            t->prev = m->tail;
        }
        m->tail = t;
        m->count++;
check:
        taskMgrCheckList(m);
    }
}

/* Unlink t from the run list. */
void __attribute__((section(".text_80160310"))) taskMgrUnlink(TaskManager *m, Task *t)
{
    s32 alone;

    if (taskMgrFindTask(m, t)) {
        alone = t->prev == 0 && t->next == 0;
        if (alone) {
            m->head = 0;
            m->tail = 0;
        } else if (t->prev == 0) {
            m->head = t->next;
            t->next->prev = 0;
            t->next = 0;
        } else if (t->next == 0) {
            m->tail = t->prev;
            t->prev->next = 0;
            t->prev = 0;
        } else {
            t->prev->next = t->next;
            t->next->prev = t->prev;
            t->prev = 0;
            t->next = 0;
        }
        m->count--;
    }
    taskMgrCheckList(m);
}

/* Per-frame task step: process pending starts/stops (virtual stop/start
 * hooks), update the sound driver, then run every task's frame method
 * (slot 5); a task returning non-zero is unlinked. */
void __attribute__((section(".text_80160428"))) taskMgrRunFrame(TaskManager *m)
{
    s32 i;
    Task *t;
    s32 stop;
    TaskVEntry *vt;
    Task *cur;

    if (m->npending != -1) {
        for (i = 0; ({ i < m->npending; }); i++) {
            t = m->pending[i];
            stop = 0;
            if (t->state == 1)
                stop = VCALL(t, t->vtable, 6) != 0;
            if (stop) {
                taskMgrUnlink(m, t);
                VCALL(t, t->vtable, 2);
            } else if (t->state == 0) {
                VCALL(t, t->vtable, 3);
                taskMgrLink(m, t);
            }
        }
    }
    soundDriverUpdate();
    taskMgrRunNext = m->head;
loop2:
    if ((cur = taskMgrRunNext) != 0) {
        vt = cur->vtable;
        *(Task **)&taskMgrRunCurrent = cur;
        taskMgrRunNext = cur->next;
        if (VCALL(cur, vt, 5))
            taskMgrUnlink(m, taskMgrRunCurrent);
        goto loop2;
    }
    m->npending = -1;
}

/* Stop everything: unlink and destroy every running task, clear the pending
 * queue, step once, then call the manager's own virtual slot 2. */
void __attribute__((section(".text_801605B4"))) taskMgrStopAll(TaskManager *m)
{
    Task *cur;
    s32 i;

    taskMgrStopNext = m->head;
loop:
    if ((cur = taskMgrStopNext) != 0) {
        *(Task **)&taskMgrStopCurrent = cur;
        taskMgrStopNext = cur->next;
        taskMgrUnlink(m, cur);
        VCALL(taskMgrStopCurrent, taskMgrStopCurrent->vtable, 2);
        goto loop;
    }
    if (m->npending != -1)
        for (i = 0; i < 45; i++)
            m->pending[i] = 0;
    m->npending = -1;
    m->tail = 0;
    taskMgrRunFrame(m);
    VCALL(m, m->vtable, 2);
}

/* ---- sound instances ------------------------------------------------- */

typedef struct SoundInst {
    u8 pad0[2];
    u8 prior;                /* +0x02 VagAtr */
    u8 mode_;
    u8 vol;
    u8 pan;
    u8 center;               /* +0x06 */
    u8 shift;                /* +0x07 */
    u8 min;                  /* +0x08 */
    u8 max;                  /* +0x09 */
    u8 vagatr_rest[24];
    s8 vab;                  /* +0x22 */
    s8 prog;                 /* +0x23 */
    s8 tone;                 /* +0x24 */
    s8 index;                /* +0x25 */
    u8 pad26[2];
    s32 voice;               /* +0x28 */
    void *owner;             /* +0x2C */
    s32 f30, f34;            /* +0x30 */
    struct SoundInst *next;  /* +0x38 */
    struct SoundInst *prev;  /* +0x3C */
    s32 inuse;               /* +0x40 */
    s32 mode;                /* +0x44 */
    u8 pad48[8];
    s16 timer;               /* +0x50 */
    s16 timed;               /* +0x52 */
    s32 state;               /* +0x54: 0 off, 1 on, 2 retrigger, 3 */
    u16 dirty;               /* +0x58 */
    s16 f5A;                 /* +0x5A */
    s16 f5C;                 /* +0x5C */
    s16 f5E;                 /* +0x5E */
    s16 f60;                 /* +0x60 */
    s16 voll;                /* +0x62 */
    s16 volr;                /* +0x64 */
    u8 pad66[2];
    s32 sched;               /* +0x68 */
    s32 noise;               /* +0x6C */
    u32 priority;            /* +0x70 */
    u8 pad74[4];
    s32 bend;                /* +0x78 */
    s32 f7C;                 /* +0x7C */
    s32 f80;                 /* +0x80 */
} SoundInst;

typedef struct SndVoiceAttr {
    u32 voice;               /* +0x00 */
    u32 mask;                /* +0x04 */
    s16 pad8[4];
    s16 volx_l;              /* +0x10 */
    s16 volx_r;              /* +0x12 */
    u16 pitch;               /* +0x14 */
    u16 note;                /* +0x16 */
    u16 sample_note;         /* +0x18 */
    u8 pad1a[0x26];
} SndVoiceAttr;

extern SndVoiceAttr sndVoiceAttr;
extern SoundInst *soundOverflowInsts[29];
/* Request 34 (docs/SPU_VOICES.md): TM2_SPU_VOICES voices (24; 48 with --port-define TM2_PORT_SPU48, the table
 * then moved to .port_bss), of which the driver hands out TM2_SOUND_VOICES (soundVoiceLimit; 44 with the
 * define: 44..47 are the port's); a voice's libspu mask is TM2_SPU_VOICE_BIT (1 << voice in retail). */
#include "semantic/spu_voices.h"
extern s32 soundVoiceInUse[TM2_SPU_VOICES];
#include "semantic/port_cars.h"
#include "semantic/car_sound.h"
extern SoundInst soundInstPool[TM2_MAX_SOUND_INSTANCES];   /* 35 (64 with TM2_PORT_CARS=16) */
extern s32 soundFineMax;
#pragma tm2_absolute soundFineMax
extern s32 soundFineStepsPerNote;
#pragma tm2_absolute soundFineStepsPerNote
extern s32 soundInstPitchScale;
#pragma tm2_absolute soundInstPitchScale
extern s32 soundNoteMax;
#pragma tm2_absolute soundNoteMax
extern s32 audioVolumeClampMin;
#pragma tm2_absolute audioVolumeClampMin
extern s32 audioMasterVolume;
#pragma tm2_absolute audioMasterVolume
extern s32 soundLoopKillFrom;
extern s32 soundLoopKeepCount;
extern s32 soundLoopCandidateCount;
extern SoundInst *soundActiveTail;
extern SoundInst *soundActiveHead;
extern s32 soundActiveCount;
extern s32 soundKeyedOnCount;
extern s32 soundVoiceLimit;
extern s32 soundSortedCount;
extern s32 soundInstFreeCount;
extern s32 soundInstSearchEnd;
extern s32 soundInstSearchStart;

extern s16 sub_8015C7D0(s16 voice, s16 vab, s16 prog, s16 tone, s16 note, s16 fine, s16 voll, s16 volr);
extern s16 sub_8015CBC8(s16 voice);
extern void sub_801619CC(void);
extern s16 sub_80161A0C(s16 vab, s16 prog, s16 tone, void *out);
extern void sub_80161C48(SndVoiceAttr *attr);
extern s32 sub_8015D518(s32 on_off, u32 voice_bit);
extern s32 sub_801622D8(u32 voice_bit);
extern short sub_8015D09C(short voice, short voll, short volr);
extern void sndObjRemoveVoice(void *owner, void *inst);

void soundVoiceFree(s32 voice);
void soundOn(SoundInst *r);
void soundOffOn(SoundInst *r);
void soundInstKeyOff(SoundInst *r);
s32 soundVoiceAlloc(void);
void soundSilenceFreeVoices(void);
void soundInstDetachOwner(SoundInst *r);
void soundInstUnlinkActive(SoundInst *r);
void soundInstFree(SoundInst *r);
void soundInstGetBentNote(SoundInst *r, u8 *note, u8 *fine);
void soundInstUpdate(SoundInst *r);
void soundSwapInstances(SoundInst **a, s32 i, s32 j);
void soundSortByPriority(SoundInst **a, s32 lo, s32 hi);
void soundSortByVagPriority(SoundInst **a, s32 lo, s32 hi);
void soundSortInstances(SoundInst **a, s32 lo, s32 hi);
void soundUpdateSorted(SoundInst **a);

/* Sound driver init: instance pool bounds, voice table cleared. */
void __attribute__((section(".text_8016069C"))) soundDriverInit(void)
{
    s32 i;

    soundInstSearchEnd = TM2_MAX_SOUND_INSTANCES - 1;
    soundInstFreeCount = TM2_MAX_SOUND_INSTANCES;
    soundInstSearchStart = 0;
    soundSortedCount = 0;
    soundActiveHead = 0;
    soundActiveTail = 0;
    soundVoiceLimit = TM2_SOUND_VOICES;
    soundKeyedOnCount = 0;
    for (i = 0; i < soundVoiceLimit; i++)
        soundVoiceInUse[i] = 0;
    soundLoopCandidateCount = 0;
}

/* Sound driver reset: key off every voice, free all instances. */
void __attribute__((section(".text_801606F8"))) soundDriverReset(void)
{
    s32 i;

    soundInstSearchEnd = TM2_MAX_SOUND_INSTANCES - 1;
    soundInstFreeCount = TM2_MAX_SOUND_INSTANCES;
    soundInstSearchStart = 0;
    soundSortedCount = 0;
    soundActiveHead = 0;
    soundActiveTail = 0;
    soundVoiceLimit = TM2_SOUND_VOICES;
    soundKeyedOnCount = 0;
    soundActiveCount = 0;
    for (i = 0; i < soundVoiceLimit; i++) {
        sub_8015CBC8(i);
        soundVoiceFree(i);
    }
    for (i = 0; i < TM2_MAX_SOUND_INSTANCES; i++) {
        soundInstPool[i].f30 = 0;
        soundInstPool[i].f34 = 0;
        soundInstPool[i].next = 0;
        soundInstPool[i].prev = 0;
        soundInstPool[i].state = 0;
    }
    soundLoopCandidateCount = 0;
    soundLoopKeepCount = 0;
    soundLoopKillFrom = 0;
}

/* Allocate a free sound instance from the pool (0 if none). */
SoundInst * __attribute__((section(".text_801607D0"))) soundInstAlloc(void)
{
    SoundInst *r = 0;
    s32 i, j;

    if (soundInstFreeCount) {
#ifdef TM2_PORT_CARS
        /* request 32 step 2: the pool holds TM2_MAX_SOUND_INSTANCES, the allocator hands out the first
           TM2_SOUND_INSTANCES_LIVE (35 while portCarOptions bit 3 is clear: a search past them finds
           nothing and changes nothing, as retail's empty free count) */
        for (i = soundInstSearchStart; i <= soundInstSearchEnd && i < TM2_SOUND_INSTANCES_LIVE; i++) {
#else
        for (i = soundInstSearchStart; i <= soundInstSearchEnd; i++) {
#endif
            if (soundInstPool[i].inuse == 0) {
                soundInstFreeCount--;
                r = &soundInstPool[i];
#ifdef TM2_PORT_CARS
                for (j = soundInstSearchStart + 1; j <= soundInstSearchEnd && j < TM2_SOUND_INSTANCES_LIVE; j++) {
#else
                for (j = soundInstSearchStart + 1; j <= soundInstSearchEnd; j++) {
#endif
                    if (soundInstPool[j].inuse == 0) {
                        soundInstSearchStart = j;
                        break;
                    }
                }
                r->index = i;
                r->inuse = 1;
                r->f30 = 0;
                r->f34 = 0;
                r->next = 0;
                r->prev = 0;
                r->state = 0;
                r->noise = 0;
                r->timed = 0;
                r->bend = 0;
                break;
            }
        }
    }
    return r;
}

/* Return an instance to the pool. */
void __attribute__((section(".text_801608E4"))) soundInstFree(SoundInst *r)
{
    s32 idx, v;

    if (r) {
        r->inuse = 0;
        idx = r->index;
        v = soundInstSearchStart;
        if (idx < v)
            v = idx;
        soundInstSearchStart = v;
        v = soundInstSearchEnd;
        if (v < r->index)
            v = r->index;
        soundInstSearchEnd = v;
        soundInstFreeCount++;
    }
}

/* Set the pitch from a 0..99 position inside the instance's note range. */
void __attribute__((section(".text_80160958"))) soundInstSetPitchPos(SoundInst *r, s32 pos)
{
    s32 x, unit;

    if (pos < 0)
        pos = 0;
    if (pos > 99)
        pos = 99;
    x = r->f5C + pos * r->f60 / 99;
    r->center = x / soundFineStepsPerNote;
    unit = soundFineStepsPerNote;
    r->shift = x - r->center * unit;
    r->dirty = 1;
}

/* Append to the active instance list. */
void __attribute__((section(".text_80160A2C"))) soundInstLinkActive(SoundInst *r)
{
    if (soundActiveHead == 0) {
        soundActiveHead = r;
        soundActiveTail = r;
    } else {
        SoundInst *last = soundActiveTail;
        soundActiveTail = r;
        last->next = r;
        r->prev = last;
    }
    soundActiveCount++;
}

/* Unlink from the active instance list. */
void __attribute__((section(".text_80160A74"))) soundInstUnlinkActive(SoundInst *r)
{
    if (r->prev == 0 && r->next == 0) {
        soundActiveHead = 0;
        soundActiveTail = 0;
    } else if (r->prev == 0) {
        soundActiveHead = r->next;
        r->next->prev = 0;
        r->next = 0;
    } else if (r->next == 0) {
        soundActiveTail = r->prev;
        r->prev->next = 0;
        r->prev = 0;
    } else {
        r->prev->next = r->next;
        r->next->prev = r->prev;
        r->prev = 0;
        r->next = 0;
    }
    soundActiveCount--;
}

/* Bind the instance to a VAB tone and read its VagAtr; derive the note range. */
void __attribute__((section(".text_80160B38"))) soundInstBindTone(SoundInst *r, s32 vab, s32 prog, s32 tone, s32 a4, s32 base, void *owner)
{
    r->vab = vab;
    r->prog = prog;
    r->tone = tone;
    r->f7C = a4;
    r->f80 = base;
    sub_80161A0C(r->vab, r->prog, r->tone, &r->prior);
    r->f5A = soundInstPitchScale * r->f80;
    r->center = r->f80 - (r->center - r->f80);
    r->f5E = soundInstPitchScale * (r->center + (r->max - r->f80));
    r->f5C = soundInstPitchScale * (r->center - (r->f80 - r->min));
    r->f60 = r->f5E - r->f5C;
    r->owner = owner;
}

/* Split (center * unit + bend) into note and fine. */
void __attribute__((section(".text_80160C4C"))) soundInstGetBentNote(SoundInst *r, u8 *note, u8 *fine)
{
    s32 x, unit, unit2;

    unit = soundFineStepsPerNote;
    x = r->center * unit + r->bend;
    *note = x / unit;
    unit2 = soundFineStepsPerNote;
    *fine = x - *note * unit2;
}

/* Per-frame update of one instance: (re)start, pitch, volume.
 * CLOSE: 18 words, same instructions: register choice in the volume clamps
 * and the order of the mask store vs the note shift. */
void __attribute__((section(".text_80160CCC"))) soundInstUpdate(SoundInst *r)
{
    u8 n[2];
    s32 lo, hi;
    u8 fine;

    switch (r->mode) {
    case 0:
        if (r->state == 1) {
            soundOffOn(r);
            break;
        }
        soundOn(r);
        break;
    case 1:
    case 2:
        soundOn(r);
        break;
    }
    if (r->dirty && r->state == 1) {
        if (r->f7C && r->bend)
            soundInstGetBentNote(r, &n[0], &n[1]);
        else {
            n[0] = r->center;
            n[1] = r->shift;
        }
        n[0] = n[0] >= soundNoteMax ? soundNoteMax : n[0];
        fine = n[1] >= soundFineMax ? soundFineMax : n[1];
        sndVoiceAttr.voice = TM2_SPU_VOICE_BIT(r->voice);
        sndVoiceAttr.mask = 0x60;
        {
            s32 note = (n[0] << 8) + fine;

            sndVoiceAttr.note = note;
        }
        sndVoiceAttr.sample_note = r->f80 << 8;
        n[1] = fine;
        sub_80161C48(&sndVoiceAttr);
        r->dirty = 0;
    }
    lo = audioVolumeClampMin;
    r->voll = r->voll > lo ? r->voll : lo;
    hi = audioMasterVolume;
    r->voll = r->voll < hi ? r->voll : hi;
    r->volr = r->volr > lo ? r->volr : lo;
    r->volr = r->volr < hi ? r->volr : hi;
    sub_8015D09C(r->voice, r->voll, r->volr);
}

/* soundOn: grab a free voice and key the instance on. */
void __attribute__((section(".text_80160F20"))) soundOn(SoundInst *r)
{
    s32 v;

    if (r->state == 0 || r->state == 2) {
        v = soundVoiceAlloc();
        if (v != -1) {
            sndVoiceAttr.voice = TM2_SPU_VOICE_BIT(v);
            sndVoiceAttr.mask = 3;
            sndVoiceAttr.volx_l = 0;
            sndVoiceAttr.volx_r = 0;
            sub_80161C48(&sndVoiceAttr);
            r->center = r->center < soundNoteMax ? r->center : soundNoteMax;
            r->shift = r->shift < soundFineMax ? r->shift : soundFineMax;
#if TM2_SPU_VOICES > 24
            portVoiceOwner(v, r->owner);    /* request 87: whose sound the voice plays, before its key-on */
#endif
            r->voice = sub_8015C7D0(v, r->vab, r->prog, r->tone, r->center, r->shift, 0, 0);
            sub_801619CC();
            sndVoiceAttr.voice = TM2_SPU_VOICE_BIT(r->voice);
            sndVoiceAttr.mask = 0x60;
            sndVoiceAttr.note = r->center << 8;
            sndVoiceAttr.note = r->shift | sndVoiceAttr.note;
            sndVoiceAttr.sample_note = r->f80 << 8;
            sub_80161C48(&sndVoiceAttr);
            if (r->noise == 1)  /* (never set in TM2: docs/SPU_VOICES.md 2) */
                sub_8015D518(1, TM2_SPU_VOICE_BIT(r->voice));
            r->state = 1;
            soundKeyedOnCount++;
        }
    }
}

/* soundOffOn: retrigger (key off if playing, then on). */
void __attribute__((section(".text_80161104"))) soundOffOn(SoundInst *r)
{
    if (r->state == 1)
        soundInstKeyOff(r);
    soundOn(r);
}

/* soundOff: key the instance's voice off and release it. */
void __attribute__((section(".text_80161148"))) soundInstKeyOff(SoundInst *r)
{
    if (r->state == 1 || r->state == 3) {
        if (r->noise == 1)
            sub_8015D518(0, TM2_SPU_VOICE_BIT(r->voice));
        sub_8015CBC8(r->voice);
        soundVoiceFree(r->voice);
        r->state = 0;
        soundKeyedOnCount--;
    }
}

/* Unlink and key off (named soundInstStop by IDA). */
void __attribute__((section(".text_801611DC"))) soundInstStop(SoundInst *r)
{
    soundInstUnlinkActive(r);
    soundInstKeyOff(r);
}

/* Allocate a free SPU voice (not reserved and not keyed on); -1 if none. */
s32 __attribute__((section(".text_8016120C"))) soundVoiceAlloc(void)
{
    s32 v = -1;
    s32 i;

    for (i = 0; i < soundVoiceLimit; i++) {
        if (soundVoiceInUse[i] == 0 && sub_801622D8(TM2_SPU_VOICE_BIT(i)) == 0) {
            v = i;
            soundVoiceInUse[i] = 1;
            break;
        }
    }
    return v;
}

/* Silence every unreserved voice. */
void __attribute__((section(".text_801612A8"))) soundSilenceFreeVoices(void)
{
    s32 i;

    for (i = 0; i < soundVoiceLimit; i++)
        if (soundVoiceInUse[i] == 0)
            sub_8015D09C(i, 0, 0);
}

#define KILL(p) do { soundInstKeyOff(p); soundInstDetachOwner(p); soundInstUnlinkActive(p); soundInstFree(p); } while (0)

/* Per-frame sound driver: retire finished instances, sort the playing ones
 * by priority, steal voices beyond the hardware count, update the rest.
 * CLOSE: p/next swap registers (s0/s1) and the third retire path schedules
 * the arg move after the next load (retail: before), so 4 bytes short. */
void __attribute__((section(".text_80161318"))) soundDriverUpdate(void)
{
#ifdef TM2_PORT_CARS
    /* request 32 step 2: retail sorts in the scratchpad, which has room for 35 pointers before the
       hit-detection state (0x1F8000B0..); with the raised pool (or more instances active) the list is
       portSoundSortList */
    SoundInst **arr = (TM2_PORT_POOLS_ON || soundActiveCount > TM2_RETAIL_SOUND_INSTANCES)
                          ? (SoundInst **)portSoundSortList : (SoundInst **)0x1F800000;
#else
    SoundInst **arr = (SoundInst **)0x1F800000;
#endif
    SoundInst *next, *p;
    s32 again, n, i;

    soundLoopCandidateCount = 0;
    if (soundActiveHead == 0)
        return;
    soundSortedCount = 0;
    p = soundActiveHead;
    while (({ p != 0; })) {
        again = 0;
        if (p->sched == 0 && p->state == 1) {
            if (sub_801622D8(TM2_SPU_VOICE_BIT(p->voice)) == 0) {
                soundInstKeyOff(p);
                soundInstDetachOwner(p);
                next = p->next;
                soundInstUnlinkActive(p);
                soundInstFree(p);
                again = 1;
                p = next;
            } else if (p->timed) {
                if (--p->timer <= 0) {
                    soundInstKeyOff(p);
                    soundInstDetachOwner(p);
                    next = p->next;
                    soundInstUnlinkActive(p);
                    soundInstFree(p);
                    again = 1;
                    p = next;
                } else
                    p->sched = 1;
            } else if ((u32)p->mode < 2 || p->mode == 2) {
                n = soundLoopCandidateCount;
                if (n + 1 < 29) {
                    soundOverflowInsts[n] = p;
                    p = p->next;
                    soundLoopCandidateCount = n + 1;
                    again = 1;
                } else {
                    /* block-local successor: allocated in s0 by local-alloc,
                     * which keeps the cursor p in s1 as in retail */
                    SoundInst *nx = p->next;

                    soundInstKeyOff(p);
                    soundInstDetachOwner(p);
                    soundInstUnlinkActive(p);
                    soundInstFree(p);
                    again = 1;
                    p = nx;
                }
            }
        }
        if (again)
            continue;
        if (p->sched == 2)
            p->sched = 1;
        if (p->sched == 1)
            arr[soundSortedCount++] = p;
        p = p->next;
    }
    soundSortInstances(arr, 0, soundSortedCount - 1);
    if (soundSortedCount >= soundVoiceLimit) {
        for (i = soundVoiceLimit; i < soundSortedCount; i++)
            KILL(arr[i]);
        soundSortedCount = soundVoiceLimit;
    } else {
        s32 m = soundVoiceLimit - soundSortedCount;

        if (soundLoopCandidateCount < m)
            m = soundLoopCandidateCount;
        soundLoopKeepCount = m;
        soundLoopKillFrom = m;
    }
    if (soundLoopCandidateCount)
        for (i = soundLoopKillFrom; i < soundLoopCandidateCount; i++)
            KILL(soundOverflowInsts[i]);
    soundLoopCandidateCount = soundLoopKeepCount;
    soundUpdateSorted(arr);
}

/* Notify the instance's owner that it finished. */
void __attribute__((section(".text_8016164C"))) soundInstDetachOwner(SoundInst *r)
{
    if (r->owner)
        sndObjRemoveVoice(r->owner, r);
}

/* soundSwapInstances */
void __attribute__((section(".text_80161684"))) soundSwapInstances(SoundInst **a, s32 i, s32 j)
{
    SoundInst *t = a[i];

    a[i] = a[j];
    a[j] = t;
}

/* Quicksort by priority (+0x70). */
void __attribute__((section(".text_801616B0"))) soundSortByPriority(SoundInst **a, s32 lo, s32 hi)
{
    s32 i, last;

    if (lo >= hi)
        return;
    soundSwapInstances(a, lo, (lo + hi) / 2);
    last = lo;
    for (i = lo + 1; i <= hi; i++)
        if (a[i]->priority <= a[lo]->priority)
            soundSwapInstances(a, ++last, i);
    soundSwapInstances(a, lo, last);
    soundSortByPriority(a, lo, last - 1);
    soundSortByPriority(a, last + 1, hi);
}

/* Quicksort by VagAtr priority (+0x02). */
void __attribute__((section(".text_801617BC"))) soundSortByVagPriority(SoundInst **a, s32 lo, s32 hi)
{
    s32 i, last;

    if (lo >= hi)
        return;
    soundSwapInstances(a, lo, (lo + hi) / 2);
    last = lo;
    for (i = lo + 1; i <= hi; i++)
        if (a[i]->prior <= a[lo]->prior)
            soundSwapInstances(a, ++last, i);
    soundSwapInstances(a, lo, last);
    soundSortByVagPriority(a, lo, last - 1);
    soundSortByVagPriority(a, last + 1, hi);
}

/* soundSortInstances */
void __attribute__((section(".text_801618C8"))) soundSortInstances(SoundInst **a, s32 lo, s32 hi)
{
#ifdef TM2_PORT_CAR_SOUND
    /* request 33-3 (CAR_GROUPS.md 12): with bit 4 the engine instances sort after every other sound, the
     * farthest last, so the voices past soundVoiceLimit are taken from engines first */
    if (portCarSoundSortInstances((void **)a, lo, hi))
        return;
#endif
    soundSortByPriority(a, lo, hi);
    soundSortByVagPriority(a, lo, hi);
}

/* Update every sorted instance, silence unused voices, clear the flags. */
void __attribute__((section(".text_80161918"))) soundUpdateSorted(SoundInst **a)
{
    s32 i;

    for (i = 0; i < soundSortedCount; i++)
        soundInstUpdate(a[i]);
    soundSilenceFreeVoices();
    for (i = 0; i < soundSortedCount; i++)
        a[i]->sched = 0;
}

/* Release a reserved voice. */
void __attribute__((section(".text_801619B0"))) soundVoiceFree(s32 voice)
{
    soundVoiceInUse[voice] = 0;
}
