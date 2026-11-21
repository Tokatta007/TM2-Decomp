/* Small game helpers, fourth batch: record setup and resets, effect
 * spawns, wheel/spin helpers, range tests. Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/port_cars.h"

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVector;

typedef struct {
    s16 f00;
    s16 f02;
    s16 f04;
    u8 pad_06[2];
    s16 f08;
    u8 pad_0A[980 - 0x0A];
} TM2Rec980;

/* Ground surface (28 bytes at worldSurfaces, see car_wheels.c TM2Surface). */
typedef struct {
    u8 type;                /* +0x00 */
    u8 f01;                 /* +0x01 */
    s16 section;            /* +0x02 track section the surface belongs to */
    s16 w, l;               /* +0x04 */
    s16 sn, cs;             /* +0x08 */
    s16 h[4];               /* +0x0C corner heights */
    s32 x, y;               /* +0x14 origin */
} TM2Surface;
typedef char tm2_surface_size[(sizeof(TM2Surface) == 28) ? 1 : -1];

/* 196-byte random-event record (moscowEvents / laEvents tables): one
 * object pointer and one position per lane (lane = object's u16 at +16). */
typedef struct {
    u8 on;
    u8 pad_01[35];
    u8 *obj[5];
    s32 pos[11][3];
    u8 pad_BC[8];
} TM2Rec196;

typedef struct {
    u8 *object;
    u8 pad_04[12];
    s32 f10;
    u8 pad_14[4];
    u8 f18;
    u8 pad_19[11];
} TM2Slot36;

extern s32 soundThrottleLastTime, soundThrottleLastGap;
extern s32 worldCollapseLinkTotal, worldCollapseTriggersDone, carWorldObjectCount;
extern u8 *eiffelTowerObject;
extern s32 eiffelTowerPivotX, eiffelTowerPivotY, eiffelTowerPivotZ;
#pragma tm2_absolute eiffelTowerObject
#pragma tm2_absolute eiffelTowerPivotX
#pragma tm2_absolute eiffelTowerPivotY
#pragma tm2_absolute eiffelTowerPivotZ
extern TM2Rec980 fxRecords[];
extern u8 aiRouteSpeedLimitTable[][35];
extern u8 fxDebrisFramesA[], fxDebrisFramesB[], fxDebrisFramesC[], fxDebrisFramesD[];
extern u8 moscowEvents[], laEvents[];
extern u8 worldSectionFirstSurface[], worldSectionLastSurface[];
extern TM2Surface worldSurfaces[];
extern TM2Slot36 worldCollapseSections[];
extern s32 humanCarTypes[];
extern s32 carSlotOwners[], carSlotKinds[];

extern int rand(void);
extern s32 csAlloc(void);
extern void csListAdd(s32);
extern s32 timerGetFrameTicks(void);
extern u8 *mathJitterPos(s32, s32, s32, s32);
extern void mathMulTransVec(u8 *, u8 *, s32 *);
extern void fxRandomDebrisBurst(s32 *);
#include "semantic/hud.h"
extern s32 humanCarCount(void);
extern s16 weaponFireHomingMissile(s32, s32, s32, s32, s32);
extern void missileSetTrailColor(s32, s32, s32, s32);
extern void fxImpactDebris(s32, s32 *);
extern void fxSmokeA12(s32 *, u8 arg1);
extern void fxSmokeC24(s32 *, s32);
extern void fxSmokeC12(s32 *, s32);
extern s32 gameGetMode(void);
extern void worldCollapseUnlink(s32);
extern void worldSectionDisable(s32);
extern void carBindHumanSlot(s32, s32, s32);

/* @800D3B34 */
u8 __attribute__((section(".text_800D3B34"))) soundRepeatThrottle(s32 time)
{
    s32 r = 0;
    s32 v;

    /* split into two statements: keeps retail's move v1,v0 after rand() */
    v = rand() / 8192;
    v = v * 2 + 12;
    if (time >= soundThrottleLastTime + v && v != soundThrottleLastGap) {
        r = 1;
        soundThrottleLastTime = time;
        soundThrottleLastGap = v;
    }
    return r;
}

/* @800D90C0 */
void __attribute__((section(".text_800D90C0"))) missilesCreateObjects(void)
{
    s32 i;
    u8 *rec;
    s32 *p;

    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        rec = (u8 *)&fxRecords[i];
        *(s32 *)(rec + 52) = csAlloc();
        csListAdd(*(s32 *)(rec + 52));
        *(s32 *)(*(u8 **)(rec + 52) + 280) = 10;
        *(s32 *)(*(u8 **)(rec + 52) + 8) = 0;
        p = *(s32 **)(rec + 52);
        rec[0] = i;
        *(s16 *)(rec + 4) = -15;
        p[3] = 0;
    }
}

/* @800F3718 */
s32 __attribute__((section(".text_800F3718"))) aiRouteSpeedLimit(s32 i, s32 j)
{
    /* redundant (u8) cast reproduces retail's andi after lbu */
    s32 v = (u8)aiRouteSpeedLimitTable[i][j];
    s32 t = (s16)timerGetFrameTicks() * 19;
    return v * t / 100 * 32;
}

/* @800F4C34 */
void __attribute__((section(".text_800F4C34"))) weaponSlamRelease(void *car, u8 human)
{
    struct TM2CarSlam *s;
    struct TM2CarMotion *m;

    if (human) {
        s = HREC(car)->weapons.slam;
        m = &HREC(car)->motion;
    } else {
        s = AI(car)->weapons.slam;
        m = &AI(car)->motion;
    }
    s->swing = 0;
    if (s->spin_pending) {
        s->grab = 1;
        s->f06 = 1;
    } else {
        s->active = 0;
        carSpinOutById(s->spin_target, m->rot[2], 0);
        s->spin_target = -1;
    }
}

/* @800FDC20 */
void __attribute__((section(".text_800FDC20"))) carStateFlagsClear(struct TM2CarState *state)
{
    state->dead = 0;
    state->f01 = 0;
    state->f02 = 0;
    state->f03 = 0;
    state->accelerating = 0;
    state->f06 = 0;
    state->reversing = 0;
    state->f08 = 0;
    state->weapon_fired = 0;
    state->on_surface = 0;
    state->wheel_in_air = 0;
    state->frozen = 0;
    state->energy_shield = 0;
    state->burning = 0;
    state->f0F = 0;
    state->f10 = 0;
    state->f11 = 0;
    state->f12 = 0;
    state->hud_notice = 0;
    state->crush_contact = 0;
    state->f17 = 0;
    state->f1C = 0;
    state->f1D = 0;
    state->crush_peer = 0;
    state->hit_from_side = 0;
    state->falling = 0;
    state->in_water = 0;
    state->sinking = 0;
    state->in_lava = 0;
    state->riding = 0;
    state->f05 = 0;
    state->f26 = 0;
    state->f27 = 0;
}

/* @800D6D54 */
void __attribute__((section(".text_800D6D54"))) eiffelTowerDebrisPoint(s32 a, s32 *out)
{
    mathMulTransVec(eiffelTowerObject + 4, mathJitterPos(a, 81, 81, 15), out);
    out[0] += eiffelTowerPivotX;
    out[1] += eiffelTowerPivotY;
    out[2] += eiffelTowerPivotZ;
}

/* @800F3F20 */
void __attribute__((section(".text_800F3F20"))) carScrapeSparks(void *car, u8 human)
{
    struct TM2CarMotion *m;
    s32 v[3];
    s32 t;

    if (human)
        m = &HREC(car)->motion;
    else
        m = &AI(car)->motion;
    v[0] = m->pos[0] + (rand() & 63) - 32;
    v[1] = m->pos[1] + (rand() & 63) - 32;
    t = (rand() & 31) + 32;
    v[2] = m->pos[2] - t;
    if (v[2] <= 0)
        v[2] = 1;
    fxRandomDebrisBurst(v);
}

/* @80112B9C */
void __attribute__((section(".text_80112B9C"))) explodeStoreDebrisAnimation(s32 id, u8 *object)
{
    if (*object)
        return;
    if (id == 0x29E)
        grutilsParseSpriteRects(object, fxDebrisFramesA, 4);
    else if (id == 0x29F)
        grutilsParseSpriteRects(object, fxDebrisFramesB, 4);
    else if (id == 0x2A0)
        grutilsParseSpriteRects(object, fxDebrisFramesC, 4);
    else if (id == 0x2A1)
        grutilsParseSpriteRects(object, fxDebrisFramesD, 4);
}

/* @800D4448 */
void __attribute__((section(".text_800D4448"))) moscowEventRegisterObject(u8 *object, s32 *v, s32 index)
{
    TM2Rec196 *rec = (TM2Rec196 *)(moscowEvents + index * 196);

    /* struct/array indexing (not byte offsets) gives retail operand order */
    rec->on = 1;
    rec->obj[*(u16 *)(object + 16)] = object;
    rec->pos[*(u16 *)(object + 16)][0] = v[0];
    rec->pos[*(u16 *)(object + 16)][1] = v[1];
    rec->pos[*(u16 *)(object + 16)][2] = v[2];
}

/* @800D54C0 */
void __attribute__((section(".text_800D54C0"))) laEventRegisterObject(u8 *object, s32 unused, s32 index)
{
    TM2Rec196 *rec = (TM2Rec196 *)(laEvents + index * 196);

    rec->on = 1;
    rec->obj[*(u16 *)(object + 16)] = object;
    rec->pos[*(u16 *)(object + 16)][0] = 0;
    rec->pos[*(u16 *)(object + 16)][1] = -42;
    rec->pos[*(u16 *)(object + 16)][2] = 240;
}

/* @800E4CBC: shake a car (human record when human is set, else AI) by the
 * strength of a collision at the given speed. */
void __attribute__((section(".text_800E4CBC"))) carSetCollisionBounce(void *car, u8 human, s32 speed)
{
    speed = __builtin_abs(speed);
    if (speed < 227)
        carShakeStart(car, 1, 2, 2, human);
    else if (speed < 512)
        carShakeStart(car, 2, 2, 2, human);
    else if (speed < 1024)
        carShakeStart(car, 3, 2, 2, human);
    else if (speed < 1536)
        carShakeStart(car, 3, 2, 2, human);
    else
        carShakeStart(car, 5, 2, 2, human);
}

/* @800F26DC */
void __attribute__((section(".text_800F26DC"))) worldSectionSetHeight(s32 id, s16 value)
{
    s32 k;

    for (k = worldSectionFirstSurface[id]; k <= worldSectionLastSurface[id]; k++)
        if (worldSurfaces[k].section == id) {
            worldSurfaces[k].h[0] = value;
            worldSurfaces[k].h[1] = value;
            worldSurfaces[k].h[2] = value;
            worldSurfaces[k].h[3] = value;
        }
}

/* @80118B20: 1 if any human car is of type 4..11, 14, 15, 17 or 18. */
u8 __attribute__((section(".text_80118B20"))) trackVehiclesShouldStop(void)
{
    u8 r = 0; /* narrow flag type gives retail's register allocation */
    s32 n = humanCarCount();
    s32 i;
    s32 t;

    for (i = 0; i < n; i++) {
        t = humanCarGet(i)->state.x2C.route;
        if ((u32)(t - 4) < 8 || (u32)(t - 14) < 2 || (u32)(t - 17) < 2)
            r = 1;
    }
    return r;
}

/* @800D9840 */
s16 __attribute__((section(".text_800D9840"))) weaponFireMissile(s32 a, s32 b, s32 c, s32 d, s32 e)
{
    s16 i = weaponFireHomingMissile(a, b, c, d, e);

    /* early return (not a nested if) gives retail's two return paths */
    if (i < 0)
        return i;
    fxRecords[i].f08 = 10;
    missileSetTrailColor(i, 128, 128, 128);
    fxRecords[i].f02 = 10;
    return i;
}

#define IN95(v, a) ((u32)((v) - (a)) < 95)

/* @800F0804: 1 inside one of five 95-unit squares. */
u8 __attribute__((section(".text_800F0804"))) parisSinkZoneContains(s32 x, s32 y)
{
    s32 r = 0;

    if (IN95(x, 241) && IN95(y, 241))
        r = 1;
    else if (IN95(x, 49) && IN95(y, 49))
        r = 1;
    else if (IN95(x, 433) && IN95(y, 49))
        r = 1;
    else if (IN95(x, 49) && IN95(y, 433))
        r = 1;
    else if (IN95(x, 433) && IN95(y, 433))
        r = 1;
    return r;
}

/* @800F3FB4 */
void __attribute__((section(".text_800F3FB4"))) fxRandomDebrisBurst(s32 *pos)
{
    fxImpactDebris(rand() & 3, pos);
    switch (rand() & 15) {
    case 0:
        fxSmokeA12(pos, 0);
        break;
    case 1:
        fxSmokeC24(pos, 0);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        fxSmokeC12(pos, 0);
        break;
    }
}

/* @800F3670 */
void __attribute__((section(".text_800F3670"))) wheelBlockSetHeight(struct TM2CarWheels *w, s32 v)
{
    w->wheel[0].pos[2] = v;
    w->wheel[1].pos[2] = v;
    w->wheel[2].pos[2] = v;
    w->wheel[3].pos[2] = v;
    w->wheel[0].f1C = w->wheel[0].pos[2];
    w->wheel[1].f1C = w->wheel[1].pos[2];
    w->wheel[2].f1C = w->wheel[2].pos[2];
    w->wheel[3].f1C = w->wheel[3].pos[2];
    w->wheel[0].h = w->wheel[0].pos[2];
    w->wheel[1].h = w->wheel[1].pos[2];
    w->wheel[2].h = w->wheel[2].pos[2];
    w->wheel[3].h = w->wheel[3].pos[2];
    w->wheel[0].f26 = w->wheel[0].pos[2];
    w->wheel[1].f26 = w->wheel[1].pos[2];
    w->wheel[2].f26 = w->wheel[2].pos[2];
    w->wheel[3].f26 = w->wheel[3].pos[2];
}

/* @80116310 */
void __attribute__((section(".text_80116310"))) worldCollapseClearDone(void)
{
    s32 i;
#ifdef TM2_PORT_WHOLE_MAPS
    /* PC port request 111 (RETAIL_BUGS 41): modes 2 and 4 load GLACIER2.DMD, which has no floes 0..5, and this takes
       their collision; a port that loads the whole GLACIER.DMD in those modes (the Challenge's) drew six floes with no
       ground under them from the first frame. The six are taken only when the file has none of them
       (worldCollapseRegister marks each floe the load found; worldCollapseReset clears the marks first), so the whole
       file's floes stand and break away as in the Challenge, and GLACIER2's as retail. */
    s32 have = 0;

    for (i = 0; i < 6; i++)
        have |= worldCollapseSections[i].f18;
    if ((gameGetMode() == 4 || gameGetMode() == 2) && worldCollapseTriggersDone < 6 && !have)
        worldCollapseTriggersDone = 6;
#else
    if ((gameGetMode() == 4 || gameGetMode() == 2) && worldCollapseTriggersDone < 6)
        worldCollapseTriggersDone = 6;
#endif
    for (i = 0; i < worldCollapseTriggersDone; i++) {
        worldCollapseUnlink(i);
        worldCollapseSections[i].f18 = 0;
        worldSectionDisable(i);
    }
}

/* @80116280 */
void __attribute__((section(".text_80116280"))) worldCollapseRegister(u8 *object)
{
    s32 k;

    if (*object == 4)
        k = *(u16 *)(object + 16);
    else if (*object == 5)
        k = *(u16 *)(object + 36);
    else
        k = -1;
    if (k >= 0) {
        worldCollapseSections[k].f18 = 1;
        worldCollapseLinkTotal += worldCollapseSections[k].f10;
        worldCollapseSections[k].object = object;
    }
}

/* @8011A29C */
#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): bind human `view` to the slot of its own load group (sessionBindCars
 * passes view + 1); retail's first slot of the type if that group has no slot of this type. */
void __attribute__((section(".text_8011A29C"))) carBindHumanByType(s32 view, s32 id, s32 group)
{
    s32 i;
    s32 dead;

    i = portCarSlotOfGroup(group);
    if (i >= 0 && carSlotOwners[i] == id) {
        humanCarTypes[view] = id;
        dead = *(s32 *)((u8 *)carSlotKinds + i * 4);
        carBindHumanSlot(view, i, id);
        return;
    }
    for (i = 0; i < carWorldObjectCount; i++)
        if (carSlotOwners[i] == id) {
            humanCarTypes[view] = id;
            dead = *(s32 *)((u8 *)carSlotKinds + i * 4);
            carBindHumanSlot(view, i, id);
            return;
        }
}
#else
void __attribute__((section(".text_8011A29C"))) carBindHumanByType(s32 view, s32 id)
{
    s32 i;
    s32 dead;

    for (i = 0; i < carWorldObjectCount; i++)
        if (carSlotOwners[i] == id) {
            humanCarTypes[view] = id;
            dead = *(s32 *)((u8 *)carSlotKinds + i * 4);
            carBindHumanSlot(view, i, id);
            return;
        }
}
#endif

/* @800D31A4 */
s32 __attribute__((section(".text_800D31A4"))) carSurfaceSoundVolume(s32 v, s32 a, s32 b, u8 c, u8 d, u8 e)
{
    s32 r = 0;

    if (a > 0 && b > 0 && (c || d || e)) {
        if (v >= 100)
            return 99;
        if (v < 55)
            r = v + 50;
        else if ((u32)(v - 55) < 33)
            r = v * 3 / 2 - 30;
        else if ((u32)(v - 88) < 12)
            r = v * 2 - 105;
    } else
        r = v;
    return r;
}
