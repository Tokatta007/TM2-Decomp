/* Small game helpers, third batch: slot resets and searches, effect object
 * presets, view setup, car-type predicates. Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/port_cars.h"

#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct {
    s16 f00;
    s16 f02;
    s16 f04;
    u8 pad_06[0x2E];
    s32 *f34;
    u8 pad_38[980 - 0x38];
} TM2Rec980;

typedef struct {
    s8 f00;
    s8 f01;
    u8 pad_02[2];
    s16 f04;
    u8 pad_06[22];
} TM2Rec28;

typedef struct {
    s16 kind;
    s16 used;
    s32 x, y, z;
} TM2Mark;

/* Route point (24 bytes at aiRouteNodes): only the route byte. */
typedef struct {
    u8 pad_00[23];
    s8 route;           /* +23 route group (track section) */
} TM2RouteNodeRoute;

/* 40-byte warp record at worldWarps (see car_damage_human.c Warp40). */
typedef struct {
    u8 pad_00[36];
    u8 *object;         /* +36 */
} TM2WarpObject;

/* hudRadars record: its last 0x14 bytes (880 bytes for retail's 12 blips, players.h TM2_HUD_RADAR_BYTES) */
typedef struct {
    u8 pad_000[TM2_HUD_RADAR_BYTES - 0x14];
    u8 f35C, f35D, f35E, f35F;
    s32 f360;
    u8 pad_364[8];
    s32 f36C;
} TM2Rec880;

extern TM2Rec980 fxRecords[];
extern TM2Rec28 weaponDrops[];
extern TM2Mark pickupSlots[];
extern TM2WarpObject worldWarps[];
extern TM2Rec880 hudRadars[];
extern s32 fxSheetTimer;
extern s32 pickupRegisteredCount;
extern s32 carWorldObjectCount;
extern s32 aiCarTotal;
extern s32 aiRouteNodeCount;
extern s32 eiffelTowerPivotX, eiffelTowerPivotY;
#pragma tm2_absolute aiRouteNodeCount
#pragma tm2_absolute eiffelTowerPivotX
#pragma tm2_absolute eiffelTowerPivotY
extern u8 *carBodyObjects[];
extern u8 moscowEvents[];
extern s32 viewCameraObjectTable[];
extern u8 fxSparkFrames[], fxSmokeFramesC[];
extern u8 *carSlotObjects[];
extern s32 carSlotOwners[];
extern s32 carSlotKinds[];
extern TM2RouteNodeRoute aiRouteNodes[];

extern void bulletsClear(void);
extern void renderRemoveWorldModel(u8 *);
extern void renderAddWorldModel(u8 *);
extern void viewCreate(s32, s32, s32);
extern s32 csAlloc(void);
extern void viewSetParent(s32, s32);
extern s32 worldSectionAt(s32, s32, s32);
#include "semantic/input.h"
extern void aiStartGiveUp(struct TM2AIRecord *ai);
extern s32 gameGetLevel(void);
extern s32 aiSectionEdgeDistance(struct TM2AIRecord *ai);
extern u8 *fxSpriteSpawn(s32, s32, s32, s32, u8 *, u16 count, u8 flag);
#include "semantic/weapon_calls.h"
extern void missileSetTrailColor(s32, s32, s32, s32);
extern void eiffelTowerStartFall(void);
extern void carStatsDerive(struct TM2CarStatsBlock *stats);
extern void csListAdd(s32);
extern void csSetDrawEnable(u8 *, u8 v);
extern void aiRouteNodeRemove(s32);
extern s32 carFindSlotByOwner(s32);
extern void carBindAiSlot(s32 slot, s32 id, s32 owner);

/* @800D79C4 */
void __attribute__((section(".text_800D79C4"))) projectilesReset(void)
{
    s32 i;
    s32 *p;

    bulletsClear();
    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        p = fxRecords[i].f34;
        fxRecords[i].f04 = -1;
        if (p)
            p[2] = 0;
    }
    fxSheetTimer = 0;
}

/* @80114E88: if body-object slot id>>4 holds a type-5 object, renderRemoveWorldModel it.
 * A local base pointer reproduces retail's lui-before-sll address order. */
void __attribute__((section(".text_80114E88"))) carBodyObjectHide(s32 id)
{
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): id is a car key */
    s32 i = id >> 4;
    u8 **e;

    if ((u32)i >= TM2_CAR_KEYS)
        return;
    e = (u8 **)&portCarGroups.body[i];
#else
    s32 i = id >> 4;
    u8 **base = carBodyObjects;
    u8 **e = &base[i];
#endif

    if (*e && **e == 5)
        renderRemoveWorldModel(*e);
}

/* @80114E10: as 80114E88, then also renderAddWorldModel on the object. */
void __attribute__((section(".text_80114E10"))) carBodyObjectShow(s32 id)
{
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): id is a car key */
    s32 i = id >> 4;
    u8 **e;

    if ((u32)i >= TM2_CAR_KEYS)
        return;
    e = (u8 **)&portCarGroups.body[i];
#else
    s32 i = id >> 4;
    u8 **base = carBodyObjects;
    u8 **e = &base[i];
#endif

    if (*e && **e == 5) {
        renderRemoveWorldModel(*e);
        renderAddWorldModel(*e);
    }
}

/* @80119CA0: create view, then lazily create its camera object viewCameraObjectTable[view]
 * and parent it to the view. */
void __attribute__((section(".text_80119CA0"))) viewCreateWithCamera(s32 a, s32 b, s32 view)
{
    s32 *base;
    s32 *e;

    viewCreate(a, b, view);
    base = viewCameraObjectTable;
    e = &base[view];
    if (!*e) {
        *e = csAlloc();
        viewSetParent(*e, view);
    }
}

/* @8011D854: 1 for car types 0x00, 0x30, 0x70, 0xA0, 0xB0, 0xC0. */
u8 __attribute__((section(".text_8011D854"))) carHasMeleeSpecial(s32 type)
{
    switch (type) {
    case 0x00:
    case 0x30:
    case 0x70:
    case 0xA0:
    case 0xB0:
    case 0xC0:
        return 1;
    }
    return 0;
}

/* @800D43DC */
void __attribute__((section(".text_800D43DC"))) moscowEventsInitSections(void)
{
    s32 i;
    u8 *rec;

    for (i = 0, rec = moscowEvents; i < 5; i++, rec += 196)
        if (rec[0])
            *(s16 *)(rec + 12) = worldSectionAt(*(s32 *)(rec + 68), *(s32 *)(rec + 72), *(s32 *)(rec + 76));
}

/* @800FBDFC */
void __attribute__((section(".text_800FBDFC"))) hudRadarSwapBuffers(s32 index)
{
    s32 v = hudRadars[index].f36C;
    TM2Rec880 *p;

    hudRadars[index].f360 = 0;
    hudRadars[index].f36C = 1 - v;
    p = &hudRadars[index];
    p->f35E = 0;
    p->f35F = 0;
    p->f35D = 0;
    hudRadars[index].f35C = 0;
}

/* @8012B82C */
u8 __attribute__((section(".text_8012B82C"))) movieSkipCheckDelayed(s32 value)
{
    u8 r;

    padUpdate();
    if (value < 35)
        return 0;
    r = 0;
    if (padCheckAction(10, 2) || padCheckAction(8, 2))
        r = 1;
    return r;
}

/* @800E17DC */
void __attribute__((section(".text_800E17DC"))) aiWallBumpCount(struct TM2AIRecord *ai)
{
    s16 *c = &DRV(ai)->f474;
    s16 t;

    *c += 1;
    t = *c;
    if (*c >= 4)
        aiStartGiveUp(ai);
    if (ai->f041)
        DRV(ai)->backup_time = 30;
}

/* @800F02D0 */
u8 __attribute__((section(".text_800F02D0"))) aiNearSectionEdge(struct TM2AIRecord *ai)
{
    u8 r = 0;

    if ((gameGetLevel() == 5 || gameGetLevel() == 6) && aiSectionEdgeDistance(ai) < 480)
        r = 1;
    return r;
}

/* @80109684 */
s32 __attribute__((section(".text_80109684"))) weaponRemoteBombFind(s32 id)
{
    s32 i;

    for (i = 0; i < TM2_MAX_DROPS; i++)   /* 25 (request 40: TM2_MAX_DROPS) */
        if (weaponDrops[i].f00 == 19 && weaponDrops[i].f01 && weaponDrops[i].f04 == id)
            return i;
    return -1;
}

/* @80111384 */
void __attribute__((section(".text_80111384"))) fxSparkBlue(s32 a, u8 flag)
{
    u8 *o = fxSpriteSpawn(a, 5, 1, 4, fxSparkFrames, 1, flag);

    if (o) {
        *(s32 *)(o + 40) = 3;
        o[2] = 1;
        o[3] = 0;
        o[4] = 20;
        o[5] = 100;
    }
}

/* @8011244C */
void __attribute__((section(".text_8011244C"))) fxSmokeC6(s32 a, u8 flag)
{
    u8 *o = fxSpriteSpawn(a, 6, 2, 16, fxSmokeFramesC, 1, flag);

    if (o) {
        *(s32 *)(o + 40) = 1;
        o[2] = 1;
        o[3] = 16;
        o[4] = 16;
        o[5] = 16;
    }
}

/* @800D9BE8 */
s16 __attribute__((section(".text_800D9BE8"))) weaponFirePowerMissile(s32 kind, s32 pos, s32 dir, s32 a3)
{
    s16 i = weaponSpawnProjectile(kind, pos, dir, a3);

    if (i >= 0) {
        missileSetTrailColor(i, 250, 0, 0);
        fxRecords[i].f02 = 12;
    }
    return i;
}

/* @800D6698: call eiffelTowerStartFall when v is within 1200 of (eiffelTowerPivotX, eiffelTowerPivotY)
 * on both X and Y (__builtin_abs gives retail's register allocation). */
void __attribute__((section(".text_800D6698"))) eiffelTowerBombCheck(s32 *v)
{
    if (__builtin_abs(eiffelTowerPivotX - v[0]) < 1200 &&
        __builtin_abs(eiffelTowerPivotY - v[1]) < 1200)
        eiffelTowerStartFall();
}

/* @800FE478 */
void __attribute__((section(".text_800FE478"))) carStatsSetTopSpeed(struct TM2CarStatsBlock *stats, s32 value)
{
    stats->top_speed = value;
    stats->reverse_speed = stats->top_speed * 3 / 4;
    stats->rollover_speed = stats->top_speed * 2 / 3;
    carStatsDerive(stats);
}

/* @8010AE58 */
void __attribute__((section(".text_8010AE58"))) pickupRegister(s32 *v, s32 kind, s32 index)
{
    s32 *m;

    if (index < 51 && !pickupSlots[index].used) {
        pickupSlots[index].used = 1;
        m = &pickupSlots[index].x;
        m[0] = v[0];
        m[1] = v[1];
        pickupRegisteredCount++;
        m[2] = v[2];
        pickupSlots[index].kind = kind;
    }
}

/* @80115C08 */
void __attribute__((section(".text_80115C08"))) worldWarpObjectsCreate(s32 value)
{
    s32 i;
    s32 o;

    for (i = 0; i < 3; i++) {
        o = csAlloc();
        worldWarps[i].object = (u8 *)o;
        csListAdd(o);
        *(s32 *)(worldWarps[i].object + 284) = value;
        *(s32 *)(worldWarps[i].object + 8) = 0;
    }
}

/* @8011961C */
void __attribute__((section(".text_8011961C"))) carRegisterModel(u8 *object, s32 id)
{
    if (carWorldObjectCount < TM2_MAX_CAR_SLOTS) {
        carSlotObjects[carWorldObjectCount] = object;
        carSlotOwners[carWorldObjectCount] = id;
        csSetDrawEnable(carSlotObjects[carWorldObjectCount], 0);
        carSlotKinds[carWorldObjectCount] = 2;
#ifdef TM2_PORT_CAR_GROUPS
        portCarRegisterRoot(carWorldObjectCount, id);   /* request 32: the slot's load group and key */
#endif
        carWorldObjectCount++;
    }
}

/* @800F2288 */
void __attribute__((section(".text_800F2288"))) aiRouteRemoveSectionNodes(s32 id)
{
    s32 i;

    for (i = 0; i < aiRouteNodeCount; i++)
        if (aiRouteNodes[i].route == id)
            aiRouteNodeRemove(i);
}

/* @80100318 */
void __attribute__((section(".text_80100318"))) aiRouteStateReset(struct TM2AIDrive *drive)
{
    drive->node_prev = 0;
    drive->node = 0;
    drive->node_next = 0;
    drive->route = 0;
    drive->away = 0;
    drive->closer = 0;
    drive->node_repeat = 0;
    drive->side_offset = 0;
    drive->f472 = 0;
    drive->f448 = 0;
    drive->f449 = 0;
    drive->diagonal = 0;
    drive->axis_x = 0;
    drive->axis_y = 0;
    drive->side_neg = 0;
    drive->side_pos = 0;
    drive->sharp_turn = 0;
    drive->f453 = 0;
    drive->route_change = 0;
    drive->link_pending = 0;
    drive->f460 = 0;
    drive->backup_time = 0;
    drive->f458 = 0;
    drive->f498 = 0;
    drive->f45A = 0;
    drive->f45B = 0;
    drive->f45D = 0;
    drive->f45C = 0;
    drive->f45E = 0;
    drive->f45F = 0;
}

/* @8011A54C */
#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): bind opponent a to the slot of its own load group (sessionBindCars
 * passes humans + a + 1); retail's first slot of the type if that group has no slot of this type. */
void __attribute__((section(".text_8011A54C"))) carBindAiByType(s32 a, s32 id, s32 group)
{
    s32 i = portCarSlotOfGroup(group);

    if (i < 0 || carSlotOwners[i] != id)
        i = carFindSlotByOwner(id);
#else
void __attribute__((section(".text_8011A54C"))) carBindAiByType(s32 a, s32 id)
{
    s32 i = carFindSlotByOwner(id);
#endif

    if (i >= 0) {
        if (carSlotKinds[i] != 2)
            carBindAiSlot(a, i, id);
        else {
            aiCarTotal++;
            carBindAiSlot(a, i, id);
        }
    }
}
