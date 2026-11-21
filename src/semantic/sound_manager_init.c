typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/* Target pool record (20 bytes, pool of 50 at targetPool; see target_pool.c).
   targetTrackVehicleSlotA / targetTrackVehicleSlotB are the slots of the two special targets, object
   types 506 and 507. */
typedef struct TargetRecord {
    s16 type, instance, counter;
    u8 flag, pad;
    s32 x, y, z;
} TargetRecord;

extern TargetRecord targetPool[];
extern s32 targetTrackVehicleSlotA;
extern s32 targetTrackVehicleSlotB;
extern void taskMgrInit(void *);
extern void sndListSetMax(void *, s32);

/* Store the world position of special target type 506 or 507.
   (position must not be const: a const pointee lets the loads move ahead of
   the stores and the code no longer matches.) */
void __attribute__((section(".text_80128148"))) targetSetTrackVehiclePos(s32 type, s32 *position)
{
    if (type == 506) {
        targetPool[targetTrackVehicleSlotA].x = position[0];
        targetPool[targetTrackVehicleSlotA].y = position[1];
        targetPool[targetTrackVehicleSlotA].z = position[2];
    }
    if (type == 507) {
        targetPool[targetTrackVehicleSlotB].x = position[0];
        targetPool[targetTrackVehicleSlotB].y = position[1];
        targetPool[targetTrackVehicleSlotB].z = position[2];
    }
}

/* Initialise an owner's per-type object lists (types 1..66, 20 bytes each from
   +0xE0) with the list mode each type needs, then the seven extra lists
   (+0x608 .. +0x680). Types 31, 7 and 28 use fixed list slots. */
void __attribute__((section(".text_801281EC"))) soundMgrInitLists(u8 *owner)
{
    s32 type;

    taskMgrInit(owner);
    for (type = 1; type < 67; type++) {
        s32 index = type - 1;
        if ((unsigned)index < 2 || type == 6 || type == 10 || type == 17 || type == 20 ||
            type == 39 || type == 22 || type == 33 || type == 30 || type == 34 ||
            type == 16 || type == 59)
            sndListSetMax(owner + 0xe0 + index * 20, 2);
        else if (type == 12 || type == 5 || type == 14 || type == 4 ||
                 type == 19 || type == 35 || type == 43 || type == 55 ||
                 type == 64 || type == 61 || type == 49 || type == 11)
            sndListSetMax(owner + 0xe0 + index * 20, 3);
        else if (type == 24 || type == 8 || type == 15 || type == 3 ||
                 type == 29 || type == 13)
            sndListSetMax(owner + 0xe0 + index * 20, 4);
        else if (type == 31)
            sndListSetMax(owner + 0x338, 5);
        else if (type == 7)
            sndListSetMax(owner + 0x158, 6);
        else if (type == 28)
            sndListSetMax(owner + 0x2fc, 7);
        else
            sndListSetMax(owner + 0xe0 + index * 20, 1);
    }
    sndListSetMax(owner + 0x608, 1);
    sndListSetMax(owner + 0x61c, 1);
    sndListSetMax(owner + 0x630, 1);
    sndListSetMax(owner + 0x644, 1);
    sndListSetMax(owner + 0x658, 1);
    sndListSetMax(owner + 0x66c, 1);
    sndListSetMax(owner + 0x680, 1);
}
