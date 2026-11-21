typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/*
 * Spawning and removing scene objects in the shell's object lists.
 *
 * owner is the scene/shell object: per-type object lists (20 bytes each)
 * start at +0xCC (indexed by object type), plus fixed lists:
 *   +0x608 / +0x61C  type 0  (side 0 / side 1)
 *   +0x630 / +0x644  type 9  (side 0 / side 1)
 *   +0x658           generic list (soundMgrPlayGeneric / soundMgrStopGeneric)
 *   +0x66C / +0x680  type 32 (side 0 / side 1)
 * sndEffectTable is the 20-byte object type table (first word: list state).
 */
typedef struct ObjectType {
    s32 state;  /* 1: objects of this type use the per-type list */
    u8 pad[16];
} ObjectType;

/* g++ 2.x vtable entry: this-adjust delta, index, function.
   Entry 2 of a SceneObject vtable is the (deleting) destructor. */
typedef struct VtblEntry {
    s16 delta;
    s16 index;
    void (*func)(void *);
} VtblEntry;

typedef struct SceneObject {
    u8 pad0[32];
    s32 x, y, z;          /* 32 */
    u8 pad44[12];
    VtblEntry *vptr;      /* 56 */
} SceneObject;

extern ObjectType sndEffectTable[];
extern SceneObject *sndPoolAlloc(void);                              /* allocate object */
extern void sndObjSetupFromRecord(SceneObject *, ObjectType *, s32, s32);    /* construct from type */
extern void sndListSubmit(void *list, SceneObject *object, void *owner);  /* list add */
extern void sndListStopById(void *list, void *owner, s32 id);          /* list remove */
extern void soundSetSharedLoop(u8 value);
extern void soundSetLoop32P1(u8 value);
extern void soundSetLoop32P2(u8 value);
extern void soundSetTurboLoopP1(u8 value);
extern void soundSetTurboLoopP2(u8 value);
extern void soundSetGunLoopP1(u8 value);
extern void soundSetGunLoopP2(u8 value);
#include "semantic/sound.h"

/* Create an object of the given type/variant at (x, y, z) in the generic list. */
void __attribute__((section(".text_80128494"))) soundMgrPlayGeneric(u8 *owner, s32 type,
    s32 x, s32 y, s32 z, s32 variant)
{
    SceneObject *object = sndPoolAlloc();

    if (object != 0) {
        sndObjSetupFromRecord(object, &sndEffectTable[type], variant, 0);
        object->x = x;
        object->y = y;
        object->z = z;
        sndListSubmit(owner + 0x658, object, owner);
        soundSetSharedLoop(1);
    }
}

/* Create an object of type 0, 9 or 32 at (x, y, z) in side 0's or side 1's
   list; any other type on side 0/1 is deleted again (virtual destructor,
   vtable entry 2). Other sides leave the object alone. */
void __attribute__((section(".text_8012854C"))) soundMgrPlaySided(u8 *owner, s32 type,
    s32 x, s32 y, s32 z, s32 side)
{
    SceneObject *object = sndPoolAlloc();
    VtblEntry *entry;

    if (object == 0)
        return;
    sndObjSetupFromRecord(object, &sndEffectTable[type], 0, 0);
    object->x = x;
    object->y = y;
    object->z = z;
    if (side == 0) {
        switch (type) {
        case 9:
            sndListSubmit(owner + 0x630, object, owner);
            soundSetGunLoopP1(1);
            break;
        case 0:
            sndListSubmit(owner + 0x608, object, owner);
            soundSetTurboLoopP1(1);
            break;
        case 32:
            sndListSubmit(owner + 0x66c, object, owner);
            soundSetLoop32P1(1);
            break;
        default:
            entry = &object->vptr[2];
            entry->func((u8 *)object + entry->delta);
            break;
        }
    } else if (side == 1) {
        switch (type) {
        case 9:
            sndListSubmit(owner + 0x644, object, owner);
            soundSetGunLoopP2(1);
            break;
        case 0:
            sndListSubmit(owner + 0x61c, object, owner);
            soundSetTurboLoopP2(1);
            break;
        case 32:
            sndListSubmit(owner + 0x680, object, owner);
            soundSetLoop32P2(1);
            break;
        default:
            entry = &object->vptr[2];
            entry->func((u8 *)object + entry->delta);
            break;
        }
    }
}

/* Clear side 0's or side 1's list for object type 0, 9 or 32. */
void __attribute__((section(".text_80128740"))) soundMgrStopSided(u8 *owner, s32 type, s32 side)
{
    void *list;

    if (side == 0) {
        switch (type) {
        case 9:
            soundSetGunLoopP1(0);
            list = owner + 0x630;
            break;
        case 0:
            soundSetTurboLoopP1(0);
            list = owner + 0x608;
            break;
        case 32:
            soundSetLoop32P1(0);
            list = owner + 0x66c;
            break;
        default:
            return;
        }
    } else if (side == 1) {
        switch (type) {
        case 9:
            soundSetGunLoopP2(0);
            list = owner + 0x644;
            break;
        case 0:
            soundSetTurboLoopP2(0);
            list = owner + 0x61c;
            break;
        case 32:
            soundSetLoop32P2(0);
            list = owner + 0x680;
            break;
        default:
            return;
        }
    } else
        return;
    sndListStopById(list, owner, -1);
}

/* Remove object id from the generic list. */
void __attribute__((section(".text_80128848"))) soundMgrStopGeneric(u8 *owner, s32 unused, s32 id)
{
    soundSetSharedLoop(0);
    sndListStopById(owner + 0x658, owner, id);
}

/* Create an object of a list-managed type (state 1) at (x, y, z) in its
   per-type list; variant -1 means default construction.
   (The position stores are duplicated in both branches on purpose: that is
   what reproduces retail scheduling after cross-jumping merges them.) */
void __attribute__((section(".text_80128890"))) soundMgrPlay(u8 *owner, s32 type,
    s32 x, s32 y, s32 z, s32 variant, s32 extra)
{
    SceneObject *object;

    switch (sndEffectTable[type].state) {
    case 0:
        break;
    case 1:
        object = sndPoolAlloc();
        if (object == 0)
            break;
        if (variant != -1) {
            sndObjSetupFromRecord(object, &sndEffectTable[type], variant, extra);
            object->x = x;
            object->y = y;
            object->z = z;
        } else {
            sndObjSetupFromRecord(object, &sndEffectTable[type], 0, 0);
            object->x = x;
            object->y = y;
            object->z = z;
        }
        sndListSubmit(owner + (type * 20 + 0xcc), object, owner);
        soundSetActive(type, 1);
        break;
    }
}
