typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;

typedef struct TargetRecord {
    s16 type;
    s16 instance;
    s16 counter;
    u8 flag;
    u8 pad;
    s32 x;
    s32 y;
    s32 z;
} TargetRecord;

typedef struct TargetObject {
    u8 bytes[6];
    u8 count;
    u8 state;
} TargetObject;

extern TargetRecord targetPool[50];
extern u16 targetTimedEvent[];
extern u16 targetLevelEvent[];
/* Per-player target slots (44 bytes): +0 hit, +1 blocked, +2 instance. */
typedef struct { signed char hit; signed char blocked; s16 instance; u8 pad[40]; } TM2TargetSlot;
extern TM2TargetSlot targetPlayerEvents[2];
extern void *targetDamageFxPos;
#pragma tm2_absolute targetPool
#pragma tm2_absolute targetPlayerEvents
#pragma tm2_absolute targetDebrisAngles
#pragma tm2_rdata .tm2_800CF570
typedef struct { s16 v[4]; } TM2Pair8;
extern TM2Pair8 targetDebrisAngles;

extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
#include "semantic/world.h"
extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion12(void *, u8 flag);
extern void fxExplosion24(void *, u8 flag);
extern void fxExplosion48(void *, u8 flag);
extern void fxExplosion96(void *, u8 flag);
extern void fxSmokeA12(void *, u8 arg1);
extern void fxSmokeA24(void *, s32);
extern void fxSmokeA48(void *, s32);
extern void fxSmokeB48(void *, s32);
extern void fxImpactDebris(s32, void *);
extern void trackVehicleDestroy(s32);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
extern void debrisBurstAt(void *, void *, void *, s32, s32, s32, s32, s32, s32);
extern s32 rand(void);

/* @80125118: damage target (type, instance) by `amount`. When its counter
 * runs out: report it (objRegistrySetStep when flagged, else pickupSetVisible), then
 * per type: 500 trigger (pad/marker, object state), 501 explosion, 502 /
 * 503 / 520 debris bursts (2 or 3), 504 / 508 / 509 / 510 / 519 small
 * bursts plus one-shot level events (510: per-player slots), 505..507
 * large bursts; 506 / 507 are the two track vehicles (trackVehicleDestroy, see
 * track_vehicles.c), which reset to 25 and respawn. Returns 1 once the
 * target is destroyed, 0 while it survives or is unknown. */
s32 __attribute__((section(".text_80125118"))) targetDamage(s32 type, s32 instance, s32 amount)
{
    TM2Pair8 pair;
    s32 i, n;
    s32 found;
    TargetRecord *r, *record;
    TargetObject *object;

#ifdef TM2_PORT_HAZARDS
    /* request 38 G5: an event target (or a prop, item PROPS) switched off takes no damage: before the pool
       search, the counter and every rand() of its effects. No caller reads the result. */
    if (portArenaTargetImmune(type))
        return 0;
#endif
    pair = targetDebrisAngles;
    found = -1;
    n = 0;
    r = targetPool;
    for (; n < 50; n++, r++) {
        if (r->type == type && r->instance == instance && r->counter > 0)
            found = n;
    }
    if (found < 0)
        return 0;
    record = &targetPool[found];
    record->counter -= amount;
    if (record->counter > 0)
        return 0;

    if (record->flag)
        objRegistrySetStep(record->type, record->instance, 1);
    else
        pickupSetVisible(record->type, record->instance, 0);

    switch (record->type) {
    case 500:
        if (viewGetSplitMode() == 0)
            soundLocateForView0(&record->x);
        else
            soundLocateForView(&record->x, -1);
        soundPlay(30, PadChkMtap(), soundGetListenerPan(), 99);
        fxSmokeB48(&record->x, 0);
        object = targetGetLevelObject(record->instance);
        if (object->count >= 3) {
            object->state = 2;
            debrisBurstAt(object, &record->x, &pair, 0, 0, 0, 10, 60, 0);
        }
        object->state = 1;
        return 1;
    case 501:
        fxExplosion12(&record->x, 0);
        fxSmokeA12(&record->x, 0);
        fxImpactDebris(2, &record->x);
        return 1;
    case 502:
        record->z += 80;
        for (i = 0; i < 2; i++) {
            targetDamageFxPos = mathJitterPos(&record->x, 0x332, 0x332, 0x332);
            fxExplosion12(targetDamageFxPos, 0);
            fxSmokeA12(targetDamageFxPos, 0);
            fxImpactDebris(3, targetDamageFxPos);
        }
        return 1;
    case 503:
    case 520:
        record->z += 80;
        for (i = 0; i < 3; i++) {
            targetDamageFxPos = mathJitterPos(&record->x, 0x332, 0x332, 0x332);
            fxExplosion12(targetDamageFxPos, 0);
            fxSmokeA12(targetDamageFxPos, 0);
            fxImpactDebris(3, targetDamageFxPos);
        }
        return 1;
    case 504:
    case 508:
    case 509:
    case 510:
    case 519:
        record->z += 80;
        for (i = 0; i < 2; i++) {
            targetDamageFxPos = mathJitterPos(&record->x, 0x199, 0x199, 0x199);
            fxExplosion24(targetDamageFxPos, 0);
            fxSmokeA24(targetDamageFxPos, 0);
            fxImpactDebris(4, targetDamageFxPos);
        }
        if (record->type == 508 && targetTimedEvent[0] == 0) {
            *(u8 *)targetTimedEvent = 1;
            targetTimedEvent[2] = 0; /* clock (+4) */
            objRegistrySetStep(record->type, record->instance, 0);
        }
        if (record->type == 519 && targetLevelEvent[0] == 0) {
            *(u8 *)targetLevelEvent = 1;
            objRegistrySetStep(record->type, record->instance, 0);
        }
        if (record->type == 509 && targetLevelEvent[0] == 0x100) {
            *(u8 *)targetLevelEvent = 1;
            objRegistrySetStep(record->type, record->instance, 0);
        }
        if (record->type == 510) {
            for (i = 0; i < 2; i++) {
                if (targetPlayerEvents[i].instance == record->instance && !targetPlayerEvents[i].hit && !targetPlayerEvents[i].blocked) {
                    targetPlayerEvents[i].hit = 1;
                    objRegistrySetStep(record->type, record->instance, 0);
                    if (i == 0) {
                        worldSectionDisable(2);
                        worldSectionDisable(4);
                    } else {
                        worldSectionDisable(0);
                        worldSectionDisable(5);
                    }
                }
            }
        }
        return 1;
    case 505:
    case 506:
    case 507:
        record->z += 272;
        for (i = 0; i < 3; i++) {
            targetDamageFxPos = mathJitterPos(&record->x, 0xcc, 0xcc, 0xcc);
            if (rand() & 1)
                fxExplosion48(targetDamageFxPos, 0);
            else
                fxExplosion96(targetDamageFxPos, 0);
            fxSmokeA48(targetDamageFxPos, 0);
            fxImpactDebris(4, targetDamageFxPos);
        }
        if (record->type == 506) {
            trackVehicleDestroy(0);
            objRegistrySetStep(record->type, 4, 0);
            record->counter = 25;
            for (i = 0; i < 3; i++) {
                targetDamageFxPos = mathJitterPos(&record->x, 0x199, 0x199, 0x199);
                fxImpactDebris(4, targetDamageFxPos);
            }
        } else if (record->type == 507) {
            trackVehicleDestroy(1);
            objRegistrySetStep(record->type, 5, 0);
            record->counter = 25;
            for (i = 0; i < 3; i++) {
                targetDamageFxPos = mathJitterPos(&record->x, 0x199, 0x199, 0x199);
                fxImpactDebris(4, targetDamageFxPos);
            }
        }
        return 1;
    }
    return 1;
}
