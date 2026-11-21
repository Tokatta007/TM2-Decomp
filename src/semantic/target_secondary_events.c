/* Secondary timed events (8012774C..80127CE0): the second level event at
 * targetLevelEvent (armed for target types 519 / 509, see target_executor.c) and
 * the two per-player timed events targetPlayerEvents[2] (same layout as the event
 * at targetTimedEvent, target_actor_events.c). */
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;

/* Second level event: counts up to `boom`, explodes; the first time it
 * reports target 519 and re-arms itself, the second time target 509. */
typedef struct {
    signed char active;     /* +0 */
    signed char stage;      /* +1 0, 1 after the first explosion, 2 done */
    s16 instance;           /* +2 target 519 instance */
    s16 instance2;          /* +4 target 509 instance */
    s16 clock;              /* +6 */
    s16 rate;               /* +8 effect chance 1 in rate */
    s16 boom;               /* +10 */
    s32 pos[3];             /* +12 */
} TM2LevelEvent;

typedef struct {
    signed char active;     /* +0 */
    signed char done;       /* +1 */
    s16 instance;           /* +2 */
    s16 clock;              /* +4 */
    s16 rate;               /* +6 */
    s16 boom;               /* +8 */
    s16 pad10;
    void *object;           /* +12 */
    s32 pos[3];             /* +16 */
    s16 dir[4];             /* +28 */
    s16 spin[4];            /* +36 */
} TM2TimedEvent;
typedef char tm2_timed_event_size[(sizeof(TM2TimedEvent) == 44) ? 1 : -1];

typedef struct { s16 type, instance, counter; u8 flag, pad; s32 x, y, z; } TM2TargetRecord;
typedef struct { s16 v[4]; } TM2Pair8;

extern TM2LevelEvent targetLevelEvent;
extern TM2TimedEvent targetPlayerEvents[2];
extern TM2TargetRecord targetPool[50];
extern TM2Pair8 targetDebrisAngles;
#pragma tm2_absolute targetLevelEvent
#pragma tm2_absolute targetPlayerEvents
#pragma tm2_absolute targetPool
#pragma tm2_absolute targetDebrisAngles

extern s32 rand(void);
extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion96(void *, u8 flag);
extern void fxExplosion192(void *, u8 flag);
extern void fxSmokeA48(void *, s32);
extern void fxSmokeA128(void *, s32);
extern void fxSheetStart(s32, s32, s32, s32);
#include "semantic/world.h"
extern void debrisBurstAt(void *, void *, void *, void *, s32, s32, s32, s32, s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);

/* @8012774C: arm the second level event. */
void __attribute__((section(".text_8012774C"))) targetLevelEventInit(void)
{
    targetLevelEvent.rate = 20;
    targetLevelEvent.boom = 80;
    targetLevelEvent.pos[0] = 0x3FF8;
    targetLevelEvent.pos[1] = 0x4D50;
    targetLevelEvent.active = 0;
    targetLevelEvent.stage = 0;
    targetLevelEvent.clock = 0;
    targetLevelEvent.pos[2] = 0x15E0;
}

/* @801277A8: run the second level event. */
void __attribute__((section(".text_801277A8"))) targetLevelEventUpdate(void)
{
    TM2Pair8 pair;
    void *effect;
    u8 *object;

    pair = targetDebrisAngles;
    if (!targetLevelEvent.active || targetLevelEvent.stage >= 2)
        return;
    if (++targetLevelEvent.clock == 1) {
        fxExplosion192(targetLevelEvent.pos, 0);
        fxSmokeA128(targetLevelEvent.pos, 0);
    }
    if (targetLevelEvent.rate-- < 2)
        targetLevelEvent.rate = 2;
    if (rand() % targetLevelEvent.rate == 0) {
        effect = mathJitterPos(targetLevelEvent.pos, 20, 20, 10);
        if (!(rand() & 1)) {
            fxExplosion96(effect, 0);
            fxSmokeA48(effect, 0);
        } else {
            fxExplosion192(effect, 0);
            fxSmokeA128(effect, 0);
        }
    }
    if (targetLevelEvent.clock == targetLevelEvent.boom) {
        fxSheetStart(5, 255, 200, 200);
        fxExplosion192(targetLevelEvent.pos, 0);
        fxSmokeA128(targetLevelEvent.pos, 0);
        if (targetLevelEvent.stage == 0) {
            object = targetGetLevelObject(targetLevelEvent.instance);
            if (object[6] >= 3) {
                object[7] = 2;
                debrisBurstAt(object, &targetPool[targetLevelEvent.instance].x, &pair, 0, 0, 0, 60, 180, 0);
            }
            object[7] = 1;
            objRegistrySetStep(519, targetLevelEvent.instance, 1);
            targetLevelEventInit();
            targetLevelEvent.stage = 1;
        } else {
            objRegistrySetStep(509, targetLevelEvent.instance2, 1);
            targetLevelEvent.stage = 2;
        }
        if (viewGetSplitMode() == 0)
            soundLocateForView0(targetLevelEvent.pos);
        else
            soundLocateForView(targetLevelEvent.pos, -1);
        soundPlay(4, PadChkMtap(), soundGetListenerPan(), 99);
        soundPlay(19, PadChkMtap(), soundGetListenerPan(), 99);
    }
}

/* @80127AE4: arm per-player timed event `number` (1 or 2) for `owner`:
 * player-2 or player-1 start position, random +/- launch direction
 * (rand()/8 each axis), z 0xFFF, no spin.  The `/ 8` is spelt as
 * `a < 0 ? (a += 7) >> 3 : a >> 3` (same value): retail adjusts `a` in
 * place after computing the store address and recomputes the address for
 * the trailing stores, which only this if/else shape reproduces. */
void __attribute__((section(".text_80127AE4"))) targetPlayerEventInit(u8 *owner, s32 number)
{
    s32 i = number - 1;
    s32 a;

    targetPlayerEvents[i].rate = 20;
    targetPlayerEvents[i].active = 0;
    targetPlayerEvents[i].done = 0;
    targetPlayerEvents[i].clock = 0;
    targetPlayerEvents[i].boom = 50;
    targetPlayerEvents[i].object = owner;
    targetPlayerEvents[i].instance = *(s16 *)(owner + 4);
    if (i == 1) {
        targetPlayerEvents[1].pos[0] = 0xF40;
        targetPlayerEvents[1].pos[1] = 0xEE8;
        targetPlayerEvents[1].pos[2] = 0x2A8;
    } else {
        targetPlayerEvents[i].pos[0] = 0xF70;
        targetPlayerEvents[i].pos[1] = -4088;
        targetPlayerEvents[i].pos[2] = 0x2A8;
    }
    a = (rand() & 1) ? rand() : -rand();
    targetPlayerEvents[i].dir[0] = (a < 0) ? (a += 7) >> 3 : a >> 3;
    a = (rand() & 1) ? rand() : -rand();
    targetPlayerEvents[i].dir[1] = (a < 0) ? (a += 7) >> 3 : a >> 3;
    targetPlayerEvents[i].dir[2] = 0xFFF;
    targetPlayerEvents[i].spin[0] = 0;
    targetPlayerEvents[i].spin[1] = 0;
    targetPlayerEvents[i].spin[2] = 0;
}
