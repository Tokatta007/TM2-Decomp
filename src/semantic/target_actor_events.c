typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;

/* Actor-triggered target events (60 bytes each). */
typedef struct {
    u8 pad0[2];
    u8 armed;               /* +2 */
    u8 pad3;
    s16 type;               /* +4 */
    s16 instance;           /* +6 */
    s16 f8;                 /* +8 */
    s16 f10;                /* +10 */
    u8 pad12[12];
    u8 matrix[32];          /* +24 */
    u8 *object;             /* +56 */
} TM2ActorEvent;
typedef char tm2_actor_event_size[(sizeof(TM2ActorEvent) == 60) ? 1 : -1];
extern TM2ActorEvent targetActors[6];
/* Timed level event armed by target type 508 (target_executor.c). */
typedef struct {
    signed char active;     /* +0 */
    signed char done;       /* +1 */
    s16 instance;           /* +2 */
    s16 clock;              /* +4 */
    s16 rate;               /* +6 effect chance 1 in rate */
    s16 boom;               /* +8 explosion time */
    s16 pad10;
    s32 object;             /* +12 */
    s32 pos[3];             /* +16 */
    s16 dir[4];             /* +28 */
    s16 spin[4];            /* +36 */
} TM2TimedEvent;
extern TM2TimedEvent targetTimedEvent;
#pragma tm2_absolute targetTimedEvent
#pragma tm2_absolute targetActors
extern s32 rand(void);
extern int ratan2(s32, s32);
extern void *RotMatrixYXZ(s16 *, void *);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void fxSheetStart(s32, s32, s32, s32);
extern void fxTargetHitAt(void *);
#include "semantic/world.h"
extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion96(void *, u8 flag);
extern void fxExplosion192(void *, u8 flag);
extern void fxSmokeA48(void *, s32);
extern void fxSmokeA128(void *, s32);
extern void debrisBurstAt(s32, void *, void *, void *, s32, s32, s32, s32, s32);


void __attribute__((section(".text_80127228"))) targetActorHit(s32 index, s32 *direction)
{
    TM2ActorEvent *r = &targetActors[index];
    s16 angles[3];

    if (!r->armed) {
        angles[0] = 0;
        angles[1] = 0;
        if (direction[1] == 0)
            direction[1] = 1;
        angles[2] = ratan2(direction[0], direction[1]);
        if (r->type != 0x205) {
            if (viewGetSplitMode() == 0)
                soundLocateForView0(r->object + 168);
            else
                soundLocateForView(r->object + 168, -1);
            soundPlay(25, PadChkMtap() * 2, soundGetListenerPan(), 50);
        }
        r->armed = 1;
        r->f8 = 0;
        r->f10 = 25;
        RotMatrixYXZ(angles, r->matrix);
        objRegistrySetStep(r->type, r->instance, 2);
        *(s32 *)(r->object + 176) += 40;
        fxTargetHitAt(r->object + 168);
        *(s32 *)(r->object + 176) -= 40;
    }
}

/* @80127398: set up the timed event (target 508) for `owner`: fixed
 * position, random +/- launch direction, idle until armed.  Separate
 * locals for the two random values are needed for retail's registers. */
void __attribute__((section(".text_80127398"))) targetTimedEventInit(s32 owner)
{
    s32 a;
    s32 b;

    targetTimedEvent.rate = 20;
    targetTimedEvent.boom = 50;
    targetTimedEvent.pos[0] = -18360;
    targetTimedEvent.pos[1] = 0x32F0;
    targetTimedEvent.active = 0;
    targetTimedEvent.done = 0;
    targetTimedEvent.clock = 0;
    targetTimedEvent.object = owner;
    targetTimedEvent.pos[2] = 0x730;
    if (rand() & 1)
        a = rand();
    else
        a = -rand();
    targetTimedEvent.dir[0] = a / 8;
    if (rand() & 1)
        b = rand();
    else
        b = -rand();
    targetTimedEvent.dir[1] = b / 8;
    targetTimedEvent.dir[2] = 0xFFF;
    targetTimedEvent.spin[0] = 0;
    targetTimedEvent.spin[1] = 0;
    targetTimedEvent.spin[2] = 0;
}

/* @801274C0: run the timed event: sparks and smoke at an increasing rate,
 * the explosion at `boom` (flash, effects, debrisBurstAt, report target
 * 508), and 60 ticks later the follow-up calls; then it is done. */
void __attribute__((section(".text_801274C0"))) targetTimedEventUpdate(void)
{
    void *effect;

    if (!targetTimedEvent.active || targetTimedEvent.done)
        return;
    if (++targetTimedEvent.clock == 1) {
        fxExplosion192(targetTimedEvent.pos, 0);
        fxSmokeA128(targetTimedEvent.pos, 0);
    }
    if (targetTimedEvent.rate-- < 2)
        targetTimedEvent.rate = 2;
    if (rand() % targetTimedEvent.rate == 0) {
        effect = mathJitterPos(targetTimedEvent.pos, 12, 12, 41);
        if (rand() & 1) {
            fxExplosion96(effect, 0);
            fxSmokeA48(effect, 0);
        } else {
            fxExplosion192(effect, 0);
            fxSmokeA128(effect, 0);
        }
    }
    if (targetTimedEvent.clock == targetTimedEvent.boom) {
        fxSheetStart(5, 255, 200, 200);
        fxExplosion192(targetTimedEvent.pos, 0);
        fxSmokeA128(targetTimedEvent.pos, 0);
        debrisBurstAt(targetTimedEvent.object, targetTimedEvent.pos, targetTimedEvent.dir, targetTimedEvent.spin, 0, 0, 40, 120, 0);
        objRegistrySetStep(508, targetTimedEvent.instance, 1);
    }
    if (targetTimedEvent.clock == targetTimedEvent.boom + 60) {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(targetTimedEvent.pos);
        else
            soundLocateForView(targetTimedEvent.pos, -1);
        soundPlay(4, PadChkMtap(), soundGetListenerPan(), 99);
        soundPlay(19, PadChkMtap(), soundGetListenerPan(), 99);
        targetTimedEvent.done = 1;
    }
}
