typedef signed short s16;
typedef unsigned char u8;
typedef signed int s32;

#pragma tm2_rdata .tm2_800CF518

/* 20-byte target record, pool of 50 at targetPool. */
typedef struct TargetRecord {
    s16 type, instance, counter;  /* counter < 0: slot free */
    u8 flag, pad;
    s32 x, y, z;
} TargetRecord;

extern TargetRecord targetPool[50];
/* Only the target instance fields of the timed event (target_actor_events.c)
 * and the second level event (target_secondary_events.c). */
typedef struct { signed char active, done; s16 instance; u8 pad_04[40]; } TM2TimedEvent;
typedef struct { signed char active, stage; s16 instance, instance2; u8 pad_06[18]; } TM2LevelEvent;
extern TM2TimedEvent targetTimedEvent;
extern TM2LevelEvent targetLevelEvent;
#pragma tm2_absolute targetTimedEvent
#pragma tm2_absolute targetLevelEvent
extern s32 targetTrackVehicleSlotA, targetTrackVehicleSlotB;
extern void targetActorsReset(void);
extern s32 printf(const char *, ...);
extern char aErrorTargetTyp[];
s32 targetPoolFindFree(void);

/* Clear the target pool (all counters -1) and reset the target actors. */
void __attribute__((section(".text_80124EF8"))) targetPoolReset(void)
{
    s32 i;
    s16 freed = -1;
    for (i = 49 * 20; i >= 0; i -= 20)
        *(s16 *)((u8 *)targetPool + i + 4) = freed;
    targetActorsReset();
}

/* Register a target of `type` at `position` in a free slot; per-type initial counter and bookkeeping. */
void __attribute__((section(".text_80124F38"))) targetRegister(s32 type, s32 instance, s32 *position)
{
    s32 slot = targetPoolFindFree();
    TargetRecord *r;
    s16 state;
    if (slot < 0) {
        printf(aErrorTargetTyp, type, instance);
        return;
    }
    r = &targetPool[slot];
    r->type = type;
    r->instance = instance;
    r->x = position[0];
    r->y = position[1];
    r->z = position[2];
    r->flag = 0;
    r->counter = 1;
    switch (type) {
    case 500:
    case 520:
        state = 1;
        goto set;
    case 501:
        state = 2;
        goto set;
    case 502:
        state = 4;
        goto set;
    case 503:
        state = 8;
        goto set;
    case 504:
    case 508:
    case 509:
    case 519:
        r->counter = 12;
        r->flag = 1;
        if (type == 508) targetTimedEvent.instance = instance;
        if (type == 519) targetLevelEvent.instance = instance;
        if (type == 509) targetLevelEvent.instance2 = instance;
        break;
    case 510:
        state = 15;
        goto set;
    case 505:
        state = 18;
    set:
        r->counter = state;
        r->flag = 1;
        break;
    case 506:
        r->counter = 25;
        r->flag = 1;
        targetTrackVehicleSlotA = slot;
        break;
    case 507:
        r->counter = 25;
        r->flag = 1;
        targetTrackVehicleSlotB = slot;
        break;
    }
}

/* Base of the target pool. */
TargetRecord * __attribute__((section(".text_801250CC"))) targetPoolGet(void)
{
    return targetPool;
}

/* First free target slot (counter < 0), or -1. */
s32 __attribute__((section(".text_801250DC"))) targetPoolFindFree(void)
{
    s32 i;
    for (i = 0; i < 50; i++)
        if (targetPool[i].counter < 0)
            return i;
    return -1;
}
