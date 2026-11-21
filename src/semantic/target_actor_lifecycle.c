typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/* 60-byte target-actor record at targetActors (six slots). */
typedef struct TargetActor {
    u8 pad0[2];
    u8 active;          /* +2 */
    u8 pad3;
    s16 type;           /* +4, -1 = free */
    s16 instance;       /* +6 */
    s16 pad8;
    s16 counter;        /* +10 */
    u8 pad12[44];
    void *object;       /* +56 */
} TargetActor;

extern TargetActor targetActors[6];
extern s32 targetActorCount;
extern s32 targetActorRunOverCount;
#include "semantic/world.h"
extern void csSetModel(void *, u8 *);
extern void targetActorPlace(TargetActor *);

/* Reset all six target-actor slots (type -1, no object) and the counters. */
void __attribute__((section(".text_80125844"))) targetActorsReset(void)
{
    s32 i;
    for (i = 0; i < 6; i++) {
        targetActors[i].type = -1;
        targetActors[i].object = 0;
    }
    targetActorCount = 0;
    targetActorRunOverCount = 0;
}

/* Spawn a target actor from a type-9 description: allocate its object, init it and register the target. */
void __attribute__((section(".text_80125888"))) targetActorSpawn(s32 type, s16 instance, u8 *description)
{
    s32 index = targetActorCount;
    if (index < 6 && description[0] == 9) {
        TargetActor *record = &targetActors[index];
        record->object = csAlloc();
        if (record->object != 0) {
            record->type = type;
            record->instance = instance;
            record->counter = 0;
            csSetModel(record->object, description);
            csListAdd(record->object);
            *(s32 *)((u8 *)record->object + 8) = 1;
            *(s32 *)((u8 *)record->object + 12) = 0;
            *(s32 *)((u8 *)record->object + 280) = targetActorCount + 6000;
            record->active = 0;
            targetActorPlace(record);
            objRegistrySetStep(type, instance, 1);
            targetActorCount++;
        }
    }
}

/* Re-activate target actor slot `index` and re-register its target. */
void __attribute__((section(".text_801259A4"))) targetActorRespawn(s32 index)
{
    TargetActor *record = &targetActors[index];
    *(s32 *)((u8 *)record->object + 8) = 1;
    record->active = 0;
    targetActorPlace(record);
    objRegistrySetStep(record->type, record->instance, 1);
}
