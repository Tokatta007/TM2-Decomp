/* game_misc_accessors: functions IDA did not list (config/US/extra_funcs.tsv).
   800D19C8 rtResetFieldCounter, 800D19F0 renderIsFirstFrame, 800D1A08 renderIsSecondFrame, 800D1A2C renderSelectFrame, 800D1A4C renderIsSecondFrameAlt, 800D1C38 renderSyncView1Frame, 800D3528 soundStopAll, 800D35D0 audioSetSfxVolume, 800D3B04 sub_800D3B04, 800D3B10 sub_800D3B10, 800D6EE8 carSetPlayerOneObject, 800D874C bulletFree, 800F253C carWheelsOnSection, 800FCF8C hudShowRadar, 800FCFCC hudShowWeaponList, 8011960C sub_8011960C, 8011A274 humanCarSetId, 8011A324 humanCarGetType, 8011A3C4 humanCarSetInfiniteWeapons, 8011A3F0 viewSetExtraViewsFlag, 8011CAF8 humanCarGetRot, 8011D4EC viewGetCameraObject, 8011D8BC humanCarObjectSlot, 8011D8D8 aiCarObjectSlot, 8011D98C carSaveNodeState, 8011DA1C carRestoreNodeState, 8011E078 sub_8011E078, 8011E094 sub_8011E094, 8011E0B0 sub_8011E0B0, 8011E0DC sub_8011E0DC, 8011E0F8 sub_8011E0F8, 8011E1F8 carTypeFromIndex, 8011EB18 carPairVector, 8011EB4C carPairHeadingDelta, 801220F4 sceneObjGetChoiceCount, 80123960 sceneCmdMoveTurnByInit, 80123E48 sub_80123E48 */

/* Small game functions IDA never listed (no callers in retail: mostly dead
 * getters/setters next to their live siblings). Each sits in the retail code
 * between the functions named in its comment's "see" note. */

/* Retail addresses the six render-buffer records and the three slot pointers
 * as separate symbols (the slots through $gp), as in rt_main.c. */
#define renderView0Frames tm2_rt_record_array_view
#define D_80180EE4 tm2_rt_slot_array_view
#include "semantic/rt_buffers.h"
#undef renderView0Frames
#undef D_80180EE4
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

typedef struct TM2BufferInitRecord Buffer;

extern Buffer renderView0Frames, renderView0FrameB, renderView1Frames, renderView1FrameB;
extern Buffer *D_80180EE4, *renderBufferView1;
extern int rtFieldCounter;                  /* field counter (rt_main.c) */

/* ---- render loop (rt_main.c) ---- */

/* @800D19C8: reset the field counter (see rtUpdateFieldCounter, rtGetFieldCounter). */
void __attribute__((section(".text_800D19C8"))) rtResetFieldCounter(void)
{
    rtFieldCounter = 0;
}

/* @800D19F0: 1 when view 0 is drawing into the first buffer of its pair. */
int __attribute__((section(".text_800D19F0"))) renderIsFirstFrame(void)
{
    return D_80180EE4 == &renderView0Frames;
}

/* @800D1A08: 1 when view 0 is drawing into the second buffer of its pair. */
int __attribute__((section(".text_800D1A08"))) renderIsSecondFrame(void)
{
    return D_80180EE4 != &renderView0Frames;
}

/* @800D1A2C: select view 0's draw buffer: the first of the pair, or the
 * second (the record after it) when `second` is set. */
void __attribute__((section(".text_800D1A2C"))) renderSelectFrame(int second)
{
    Buffer *b = &renderView0Frames;

    D_80180EE4 = b;
    if (second)
        D_80180EE4 = b + 1;
}

/* @800D1A4C: same test as renderIsSecondFrame (a duplicate in retail). */
int __attribute__((section(".text_800D1A4C"))) renderIsSecondFrameAlt(void)
{
    return D_80180EE4 != &renderView0Frames;
}

/* @800D1C38: point view 1's buffer slot at the buffer of its pair that
 * matches view 0's current one. */
void __attribute__((section(".text_800D1C38"))) renderSyncView1Frame(void)
{
    if (D_80180EE4 == &renderView0Frames)
        renderBufferView1 = &renderView1FrameB;
    else
        renderBufferView1 = &renderView1Frames;
}

/* ---- global task manager (command_wrappers.cpp) ---- */

extern s32 soundManagerObject;                  /* global task manager */
#pragma tm2_absolute soundManagerObject
extern void taskMgrStopAll(s32);

/* @800D3528: stop every task of the global task manager soundManagerObject
 * (taskMgrStopAll). One function of 10 words (800D3528..800D354F): the load of
 * soundManagerObject is scheduled ahead of the prologue, as in soundFlush. IDA split
 * it at 800D3530 (ida_funcs.tsv lists 800D3530; not_funcs.tsv marks it as
 * this function's body). No code or data refers to either address. */
void __attribute__((section(".text_800D3528"))) soundStopAll(void)
{
    taskMgrStopAll(soundManagerObject);
}

/* ---- sound settings ---- */

/* Retail addresses the volume settings absolutely, not through $gp. */
#pragma tm2_absolute audioSfxVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern s32 audioSfxVolume;                  /* sound-effect volume */
extern s32 audioVolumeClampMax, audioVolumeClampMin;      /* volume maximum, minimum */

/* @800D35D0: set the sound-effect volume, clamped like the other volume
 * setters: the second (minimum) clamp overwrites the first, so only the
 * minimum takes effect (same as menu_volume_action.cpp set_sound_volume). */
void __attribute__((section(".text_800D35D0"))) audioSetSfxVolume(s32 v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

/* ---- one-line setters (global_accessors.c has the getters) ---- */

extern s32 D_80180A28, D_80180A2C;
extern s32 *carPlayerOneObject;                 /* actor from humanCarObject(0) */

/* @800D3B04: setter of D_80180A28 (getter sub_800D3B1C). */
void __attribute__((section(".text_800D3B04"))) sub_800D3B04(s32 value)
{
    D_80180A28 = value;
}

/* @800D3B10: setter of D_80180A2C (getter sub_800D3B28). */
void __attribute__((section(".text_800D3B10"))) sub_800D3B10(s32 value)
{
    D_80180A2C = value;
}

/* @800D6EE8: setter of carPlayerOneObject (set from humanCarObject(0) by bulletsUpdate). */
void __attribute__((section(".text_800D6EE8"))) carSetPlayerOneObject(s32 *value)
{
    carPlayerOneObject = value;
}

/* ---- 60-byte records @801814C0 (see bulletFindFree) ---- */

typedef struct { s32 f0; u8 pad4[56]; } Rec60;
extern Rec60 bulletsLifeField[];

/* @800D874C: free record i (first word 0; bulletFindFree finds free ones). */
void __attribute__((section(".text_800D874C"))) bulletFree(s32 i)
{
    bulletsLifeField[i].f0 = 0;
}

/* ---- car wheels (see wheelBlockReset: four 72-byte wheel records at +24) ---- */

typedef struct {
    u8 active;              /* +0x00 */
    u8 pad01[0xF];
    s16 id;                 /* +0x10 */
    u8 pad12[0x36];
} Wheel;

typedef struct {
    u8 pad00[0x18];
    Wheel wheel[4];         /* +0x18 */
} WheelBlock;

/* @800F253C: 1 when any active wheel of the block carries `id`. */
u8 __attribute__((section(".text_800F253C"))) carWheelsOnSection(WheelBlock *p, s32 id)
{
    u8 found = 0;

    if (p->wheel[0].active)
        found = p->wheel[0].id == id;
    if (p->wheel[1].active && p->wheel[1].id == id)
        found = 1;
    if (p->wheel[2].active && p->wheel[2].id == id)
        found = 1;
    if (p->wheel[3].active && p->wheel[3].id == id)
        found = 1;
    return found;
}

/* ---- HUD flags (see the toggles hudToggleRadar, hudToggleWeaponList) ---- */

extern u8 hudRadarVisible[];
extern u8 hudWeaponListVisible[];

/* @800FCF8C: set flag `index` (toggle: hudToggleRadar). */
void __attribute__((section(".text_800FCF8C"))) hudShowRadar(s32 index)
{
    hudRadarVisible[index] = 1;
}

/* @800FCFCC: set flag `index` (toggle: hudToggleWeaponList). */
void __attribute__((section(".text_800FCFCC"))) hudShowWeaponList(s32 index)
{
    hudWeaponListVisible[index] = 1;
}

/* ---- cars and players (car_records.c, ua_init.c) ---- */

extern u8 D_80180B8C;                   /* set only here; never read */
extern s32 humanCarTypes[];                /* per human slot: car id (carBindHumanSlot) */
extern u8 viewExtraViewsUnlocked;                   /* 1: extra views unlocked */
extern struct TM2Object *viewCameraObjectTable[];  /* camera objects */
extern s32 humanCarObjectSlots[];                /* object slot of each human */
extern s32 aiCarObjectSlots[];                /* object slot of each AI car */
extern s32 D_80180FD8[], D_80180FE0[];  /* per-player counters (zeroed by uaInit) */
extern s32 carTypeIdTable[];                /* 14-entry table (carTypeToIndex) */
#include "semantic/players.h"
extern s32 carPairVectors[][TM2_MAX_CAR_SLOTS][3];         /* vector from car i to car j */
extern s32 carPairHeadingDeltas[][TM2_MAX_CAR_SLOTS];            /* heading change since last frame */

/* World object of a car: +0x11C is its model node; a node of type 1 keeps a
 * state byte at +20 that a car can save in its record and put back. */
typedef struct {
    u8 type;                /* +0 */
    u8 pad1[19];
    u8 state;               /* +20 */
} CarNode;
typedef struct {
    u8 pad000[0x11C];
    CarNode *node;          /* +0x11C */
} CarObject;
extern CarObject *humanCarObjects[];         /* human car world objects */
extern CarObject *aiCarObjects[];         /* AI car world objects */

/* Views of the car records (cars.h) with the fields used here. */
typedef struct {
    s16 player;             /* +0x000 */
    s16 id;                 /* +0x002 */
    u8 pad_004[0x15A];
    s16 saved_state;        /* +0x15E node state saved by carSaveNodeState */
    u8 pad_160[0x5A8];
    s32 rot[3];             /* +0x708 */
    u8 pad_714[0xB4];
    s32 word_7C8;           /* +0x7C8 */
    u8 pad_7CC[0x48];
} HumanCar;
typedef struct {
    u8 pad_000[0x50E];
    s16 saved_state;        /* +0x50E */
    u8 pad_510[0x344];
} AICarView;
typedef char tm2_human_view_size[(sizeof(HumanCar) == 0x814) ? 1 : -1];
typedef char tm2_ai_view_size[(sizeof(AICarView) == 0x854) ? 1 : -1];
#include "semantic/players.h"
extern HumanCar humanCars[TM2_MAX_HUMANS];
extern AICarView aiCars[TM2_MAX_AI_CARS];
struct TM2Object;

/* @8011960C: set D_80180B8C. */
void __attribute__((section(".text_8011960C"))) sub_8011960C(void)
{
    D_80180B8C = 1;
}

/* @8011A274: set human car i's id. */
void __attribute__((section(".text_8011A274"))) humanCarSetId(s32 i, s32 id)
{
    humanCars[i].id = id;
}

/* @8011A324: car id bound to human slot i. */
s32 __attribute__((section(".text_8011A324"))) humanCarGetType(s32 i)
{
    return humanCarTypes[i];
}

/* @8011A3C4: set human car i's word at +0x7C8. */
void __attribute__((section(".text_8011A3C4"))) humanCarSetInfiniteWeapons(s32 i, u8 value)
{
    humanCars[i].word_7C8 = value;
}

/* @8011A3F0: set the extra-views-unlocked flag (first argument unused). */
void __attribute__((section(".text_8011A3F0"))) viewSetExtraViewsFlag(s32 unused, u8 value)
{
    viewExtraViewsUnlocked = value;
}

/* @8011CAF8: human car i's rotation as a short vector (position: humanCarGetPos). */
void __attribute__((section(".text_8011CAF8"))) humanCarGetRot(s32 i, s16 *rot)
{
    rot[0] = humanCars[i].rot[0];
    rot[1] = humanCars[i].rot[1];
    rot[2] = humanCars[i].rot[2];
}

/* @8011D4EC: camera object i. */
struct TM2Object *__attribute__((section(".text_8011D4EC"))) viewGetCameraObject(s32 i)
{
    return viewCameraObjectTable[i];
}

/* @8011D8BC: object slot of human i. */
s32 __attribute__((section(".text_8011D8BC"))) humanCarObjectSlot(s32 i)
{
    return humanCarObjectSlots[i];
}

/* @8011D8D8: object slot of AI car i. */
s32 __attribute__((section(".text_8011D8D8"))) aiCarObjectSlot(s32 i)
{
    return aiCarObjectSlots[i];
}

/* @8011D98C: save the state byte of car i's model node into human record i
 * and clear it (the AI table only picks the object; the value always goes
 * to the human record). */
void __attribute__((section(".text_8011D98C"))) carSaveNodeState(s32 i, u8 human)
{
    CarObject *object;
    CarNode *node;

    if (human)
        object = humanCarObjects[i];
    else
        object = aiCarObjects[i];
    if (object) {
        node = object->node;
        if (node->type == 1 && node->state) {
            humanCars[i].saved_state = node->state;
            object->node->state = 0;
        }
    }
}

/* @8011DA1C: put a saved state byte back into car i's model node. */
void __attribute__((section(".text_8011DA1C"))) carRestoreNodeState(s32 i, u8 human)
{
    CarObject *object;
    CarNode *node;
    s32 state;

    if (human) {
        object = humanCarObjects[i];
        state = humanCars[i].saved_state;
    } else {
        object = aiCarObjects[i];
        state = aiCars[i].saved_state;
    }
    if (state && object) {
        node = object->node;
        if (node->type == 1)
            node->state = state;
    }
}

/* @8011E078: player counter D_80180FE0[i]. */
s32 __attribute__((section(".text_8011E078"))) sub_8011E078(s32 i)
{
    return D_80180FE0[i];
}

/* @8011E094: player counter D_80180FD8[i]. */
s32 __attribute__((section(".text_8011E094"))) sub_8011E094(s32 i)
{
    return D_80180FD8[i];
}

/* @8011E0B0: increment player counter D_80180FD8[i] and return it. */
s32 __attribute__((section(".text_8011E0B0"))) sub_8011E0B0(s32 i)
{
    return ++D_80180FD8[i];
}

/* @8011E0DC: set player counter D_80180FD8[i]. */
void __attribute__((section(".text_8011E0DC"))) sub_8011E0DC(s32 i, s32 value)
{
    D_80180FD8[i] = value;
}

/* @8011E0F8: set player counter D_80180FE0[i]. */
void __attribute__((section(".text_8011E0F8"))) sub_8011E0F8(s32 i, s32 value)
{
    D_80180FE0[i] = value;
}

/* @8011E1F8: entry i of the 14-entry table searched by carTypeToIndex. */
s32 __attribute__((section(".text_8011E1F8"))) carTypeFromIndex(s32 i)
{
    return carTypeIdTable[i];
}

/* @8011EB18: the vector from car i to car j (see carPairsUpdate). */
s32 *__attribute__((section(".text_8011EB18"))) carPairVector(s32 i, s32 j)
{
    return carPairVectors[i][j];
}

/* @8011EB4C: heading change from car i to car j since the last update. */
s32 __attribute__((section(".text_8011EB4C"))) carPairHeadingDelta(s32 i, s32 j)
{
    return carPairHeadingDeltas[i][j];
}

/* ---- choice objects (see car_switch_registry.c, sceneObjGetChoiceState) ---- */

/* @801220F4: number of states of node p's choice object (type 9), else 0. */
s32 __attribute__((section(".text_801220F4"))) sceneObjGetChoiceCount(u8 **p)
{
    if (p[1][0] == 9)
        return p[1][6];
    return 0;
}

/* ---- script commands (object_control_helpers.cpp, motion_commands.c) ---- */

/* g++ 2.x command object: duration, remaining, vtable, then the target and
 * the parameters. */
typedef struct {
    s32 duration;
    s32 remaining;
    const void *vtable;
    void *target;
    s32 dx, dy, dz;         /* per-tick position delta */
    s32 da, db, dc;         /* per-tick rotation delta */
} MoveTurnBy;

extern const u8 D_800CF4B4[];   /* _vt.10TM2Command */
extern const u8 D_800CF494[];   /* _vt.16TM2VectorCommand */
extern const u8 sceneCmdMoveTurnByVtbl[];   /* move-and-turn-by command; run() = sceneCmdMoveTurnByRun */

/* @80123960: constructor of the move-and-turn-by command (run by
 * sceneCmdMoveTurnByRun): stores the vtable of each class in the chain in turn
 * (TM2Command, TM2VectorCommand, final), then the target, the frame count
 * and the two per-tick deltas; returns the object like a g++ constructor. */
MoveTurnBy *__attribute__((section(".text_80123960"))) sceneCmdMoveTurnByInit(MoveTurnBy *c, void *target,
    s32 dx, s32 dy, s32 dz, s32 da, s32 db, s32 dc, s32 frames)
{
    c->vtable = D_800CF4B4;
    c->vtable = D_800CF494;
    c->vtable = sceneCmdMoveTurnByVtbl;
    c->target = target;
    c->remaining = frames;
    c->duration = c->remaining;
    c->dx = dx;
    c->dy = dy;
    c->dz = dz;
    c->da = da;
    c->db = db;
    c->dc = dc;
    return c;
}

/* @80123E48: the signed short at +12 of a base object (setter sub_80123E5C). */
s32 __attribute__((section(".text_80123E48"))) sub_80123E48(u8 *p)
{
    return *(s16 *)(p + 12);
}
