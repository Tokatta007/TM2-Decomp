/* Small game helpers (800D range). Names stay address-based. */
#include "semantic/weapons.h"
#include "semantic/port_cars.h"
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"   /* request 48 stage 2: ONCE_PER_STEP (fxSheetStart) */
#endif
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif

#pragma tm2_rdata .tm2_800CE960

typedef struct {
    s16 f00, f02, f04, f06, f08, f0A;
    u8 pad_0C[0x34 - 0x0C];
    struct TM2SceneObject *f34;
    s32 f38;
    s32 f3C;
    u8 pad_40[2];
    s16 f42;
    u8 f44;
    u8 pad_45[0xF0 - 0x45];
    struct TM2SceneObject *fF0;
    u8 pad_F4[980 - 0xF4];
} TM2Rec980;

typedef struct {
    s32 f00, f04;
    s32 x, y, z;
} TM2Rec20;

/* Moscow dish emitter (5 x 16 bytes at moscowDishEmitters): a flag word and the
 * emitter position (moscowDishEmitterPositions / moscowDishSatelliteEmitters
 * name the positions of entries 0 and 1). */
typedef struct {
    s32 flag;
    s32 pos[3];
} TM2DishEmitter;

/* Level event record, 196 bytes (moscowEvents[5], laEvents[5]). */
typedef struct {
    u8 f00;                 /* +0x00 armed */
    u8 f01;                 /* +0x01 */
    u8 pad_02[2];
    u8 f04;                 /* +0x04 */
    u8 f05[5];              /* +0x05 */
    u8 pad_0A[2];
    s16 section;            /* +0x0C trigger section, -1 = none */
    u8 pad_0E[196 - 0x0E];
} TM2LevelEvent;

/* Scripted rising/spinning event object (same layout as car_object_contact.c). */
typedef struct {
    s8 active;              /* +0x00 */
    s8 spark_rate;          /* +0x01 */
    s8 done;                /* +0x02 */
    u8 pad_03;
    s16 spin_step;          /* +0x04 */
    s16 speed;              /* +0x06 */
    s16 time;               /* +0x08 */
    s16 start;              /* +0x0A */
    u8 *obj;                /* +0x0C */
    s32 emit[3];            /* +0x10 */
    s32 pos[3];             /* +0x1C */
    s16 rot[4];             /* +0x28 */
} TM2RiseEvent;

/* Flat-shaded quad primitive (POLY_F4 layout). */
typedef struct {
    u8 tag[3];
    u8 len;
    u8 r, g, b, code;
    s16 x0, y0, x1, y1, x2, y2, x3, y3;
} TM2PolyF4;

/* Per-effect-id enabled flags (gp-relative bytes). */
extern u8 soundActiveId21, soundActiveId18, soundActiveId47, soundActiveId51, soundActiveId36, soundActiveId38;
extern u8 soundActiveId40, soundActiveId42, soundActiveId44, soundActiveId46, soundActiveId48, soundActiveId50;
extern u8 soundActiveId52, soundActiveId54, soundActiveId56, soundActiveId58, soundActiveId60, soundActiveId65;
extern s32 weaponHomingMissileModel, weaponFreezeMissileModel, weaponBoomerangModel, weaponSpiralMissileModel, fxSheetTimer;

extern u8 eiffelTowerState[];
extern u8 eiffelTowerSparkRate;
extern s16 eiffelTowerSpinStep, eiffelTowerFallSpeed, eiffelTowerTimer, eiffelTowerFallStart;
extern u8 *eiffelTowerObject;
extern s32 eiffelTowerPivotY, eiffelTowerPivotZ;
#pragma tm2_absolute eiffelTowerSparkRate
#pragma tm2_absolute eiffelTowerSpinStep
#pragma tm2_absolute eiffelTowerFallSpeed
#pragma tm2_absolute eiffelTowerTimer
#pragma tm2_absolute eiffelTowerFallStart
#pragma tm2_absolute eiffelTowerObject
#pragma tm2_absolute eiffelTowerPivotY
#pragma tm2_absolute eiffelTowerPivotZ

extern TM2LevelEvent moscowEvents[], laEvents[];
/* Moscow dish event (weapon_fire_missiles.c TM2DishEvent): the fields reset here. */
typedef struct {
    u8 active;              /* +0x00 */
    u8 pad_01[4];
    u8 destroyed;           /* +0x05 */
    u8 pad_06[6];
    s32 object;             /* +0x0C */
    u8 pad_10[0x26];
} TM2DishEvent;
extern TM2DishEvent moscowDishState;
#pragma tm2_absolute moscowDishState
extern TM2DishEmitter moscowDishEmitters[5];
extern TM2Rec20 worldFlameMarkers[];
extern TM2Rec980 fxRecords[];
extern TM2PolyF4 fxSheetPolys[];

extern int rand(void);
extern void fxExplosion192(s32 *, u8 flag);
extern void *RotMatrixYXZ(s16 *, u8 *);
extern s16 weaponFireHomingMissile(s32, s32, s32, s32, s32);
extern void missileSetTrailColor(s32, s32, s32, s32);
extern s32 fxRecordFindFree(s32);
extern void fxRecordInitObject(TM2Rec980 *, s32, s32);
extern void missileTrailStart(s32);
extern void missileTrailSetUVs(TM2Rec980 *);
extern void fxTrailUpdate(TM2Rec980 *, s32);
extern void fxRecordFree(s16 index, s32);
extern void weaponProjectileUpdate(TM2Rec980 *);
#include "semantic/sound.h"
extern s32 viewGetSplitMode(void);
extern void SetSemiTrans(void *, s32);

/* Get the enabled flag for effect id (18..65); 0 for unknown ids. */
u8 __attribute__((section(".text_800D3638"))) soundIsActive(s32 id)
{
    switch (id) {
    case 21: return soundActiveId21;
    case 18: return soundActiveId18;
    case 47: return soundActiveId47;
    case 51: return soundActiveId51;
    case 36: return soundActiveId36;
    case 38: return soundActiveId38;
    case 40: return soundActiveId40;
    case 42: return soundActiveId42;
    case 44: return soundActiveId44;
    case 46: return soundActiveId46;
    case 48: return soundActiveId48;
    case 50: return soundActiveId50;
    case 52: return soundActiveId52;
    case 54: return soundActiveId54;
    case 56: return soundActiveId56;
    case 58: return soundActiveId58;
    case 60: return soundActiveId60;
    case 65: return soundActiveId65;
    }
    return 0;
}

/* Set the enabled flag for effect id (18..65); unknown ids are ignored. */
void __attribute__((section(".text_800D3744"))) soundSetActive(s32 id, u8 value)
{
    switch (id) {
    case 21: soundActiveId21 = value; break;
    case 18: soundActiveId18 = value; break;
    case 47: soundActiveId47 = value; break;
    case 51: soundActiveId51 = value; break;
    case 36: soundActiveId36 = value; break;
    case 38: soundActiveId38 = value; break;
    case 40: soundActiveId40 = value; break;
    case 42: soundActiveId42 = value; break;
    case 44: soundActiveId44 = value; break;
    case 46: soundActiveId46 = value; break;
    case 48: soundActiveId48 = value; break;
    case 50: soundActiveId50 = value; break;
    case 52: soundActiveId52 = value; break;
    case 54: soundActiveId54 = value; break;
    case 56: soundActiveId56 = value; break;
    case 58: soundActiveId58 = value; break;
    case 60: soundActiveId60 = value; break;
    case 65: soundActiveId65 = value; break;
    }
}

/* Pick a weighted random slot (0..8) from the record's 9-byte weight table
 * (+0x840) and set its kind (+0x815) and action code (+0x814). In mode 160
 * slot 0 becomes 1 unless the kind is already 0. */
s32 __attribute__((section(".text_800D4124"))) aiPickWeaponFromTable9(struct TM2AIRecord *rec)
{
    s32 r;
    s32 sum;
    s32 i;
    u8 *w;
    s32 code;

    sum = 0;
    r = rand() / 328;
    i = 0;
    w = rec->weapons.weights9;
    for (; i < 9; i++) {
        sum += *w;
        if (r < sum)
            break;
        w++;
    }
    if (i < 8) {
        if (rec->type == 160 && i == 0 && rec->weapons.selected == 0)
            i = 1;
        code = 0x6B;
        rec->weapons.selected = i;
    } else if (i == 8) {
        rec->weapons.selected = 8;
        code = 0x64;
    } else {
        rec->weapons.selected = 8;
        code = 0x6B;
    }
    rec->weapons.f0C = code;
    return code;
}

/* Reset both 5-entry 196-byte record tables and the related globals. */
void __attribute__((section(".text_800D42E0"))) levelEventsReset(void)
{
    s32 i, j, m;
    TM2LevelEvent *rec;

    i = 0;
    m = -1;
    for (rec = moscowEvents; i < 5; i++, rec++) {
        rec->f00 = 0;
        rec->f01 = 0;
        rec->section = m;
        rec->f04 = 0;
        for (j = 0; j < 5; j++)
            rec->f05[j] = 0;
    }
    i = 0;
    m = -1;
    for (rec = laEvents; i < 5; i++, rec++) {
        rec->f00 = 0;
        rec->f01 = 0;
        rec->section = m;
        rec->f04 = 0;
        for (j = 0; j < 5; j++)
            rec->f05[j] = 0;
    }
    moscowDishState.active = 0;
    moscowDishState.object = 0;
    moscowDishState.destroyed = 0;
    for (i = 0; i < 5; i++) {
        moscowDishEmitters[i].flag = 0;
        worldFlameMarkers[i].f00 = 0;
        worldFlameMarkers[i].f04 = 0;
    }
}

/* Start the effect object at eiffelTowerState if idle: set state, random timer,
 * move its position, and build its rotation matrix. */
void __attribute__((section(".text_800D6710"))) eiffelTowerStartFall(void)
{
    u8 *p = eiffelTowerState;
    s32 r;

#ifdef TM2_PORT_HAZARDS
    if (portArenaOff(TM2_ARENA_EIFFEL))   /* request 38 G10: the tower stands (both bomb sites; before rand()) */
        return;
#endif
    if ((*(s32 *)p & 0xFF00FF) == 0) {     /* active and done both 0 (one word test) */
        ((TM2RiseEvent *)p)->active = 1;
        eiffelTowerTimer = 0;
        eiffelTowerSparkRate = 20;
        r = rand();
        eiffelTowerSpinStep = 0;
        eiffelTowerFallSpeed = 75;
        eiffelTowerPivotY += 600;
        eiffelTowerPivotZ += 800;
        eiffelTowerFallStart = r / 656 + 50;
        fxExplosion192(((TM2RiseEvent *)p)->pos, 0);
        eiffelTowerPivotZ = 1344;
        RotMatrixYXZ(((TM2RiseEvent *)p)->rot, eiffelTowerObject + 4);
    }
}

/* Spawn an effect record via weaponFireHomingMissile and set it up (state 51). */
s32 __attribute__((section(".text_800D98E0"))) weaponFireRoadkillBoomerang(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5)
{
    s32 idx;

    idx = weaponFireHomingMissile(a0, a1, a2, a3, a4);
    if (idx < 0)
        return idx;
    if (weaponBoomerangModel != 0)
        fxRecords[idx].f34->model = weaponBoomerangModel;
    fxRecords[idx].f08 = 5;
    fxRecords[idx].f06 = 75;
    fxRecords[idx].f44 = 3;
    missileSetTrailColor(idx, 0xFF, 0xFF, 0xFF);
    fxRecords[idx].f02 = 51;
    fxRecords[idx].f3C = a5;
    fxRecords[idx].f04 = fxRecords[idx].f04 * 4;
    return idx;
}

/* Spawn an effect record via weaponFireHomingMissile and set it up (state 14). */
s32 __attribute__((section(".text_800D9AE0"))) weaponFireFreezeMissile(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4)
{
    s32 idx;

    idx = weaponFireHomingMissile(a0, a1, a2, a3, a4);
    if (idx < 0)
        return idx;
    if (a0 < 50)
        fxRecords[idx].f08 = 15;
    else
        fxRecords[idx].f08 = 4;
    fxRecords[idx].f06 = 50;
    fxRecords[idx].f34->model = weaponFreezeMissileModel;
    missileSetTrailColor(idx, 32, 32, 159);
    fxRecords[idx].f02 = 14;
    return idx;
}

/* Spawn an effect record via weaponFireHomingMissile and set it up (state 61). */
s32 __attribute__((section(".text_800D9D38"))) weaponFireDarkToothMissile(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4)
{
    s32 idx;

    idx = weaponFireHomingMissile(a0, a1, a2, a3, a4);
    if (idx < 0)
        return idx;
    if (rand() < 0x4000)
        fxRecords[idx].f08 = 99;
    else
        fxRecords[idx].f08 = 20;
    fxRecords[idx].f06 = 37;
    fxRecords[idx].f04 = fxRecords[idx].f04 * 5;
    missileSetTrailColor(idx, 0xFF, 0, 150);
    fxRecords[idx].f02 = 61;
#ifdef TM2_PORT_SPECIAL_AS
    if (weaponSpiralMissileModel != 0)      /* request 35: as weaponFireSpiralMissile */
#endif
    fxRecords[idx].f34->model = weaponSpiralMissileModel;
    return idx;
}

/* Allocate a free effect record and set it up (state 10) at pos/dir. */
s16 __attribute__((section(".text_800D9F40"))) weaponSpawnProjectile(s32 kind, s32 pos, s32 dir, s32 a3)
{
    s32 idx;
    TM2Rec980 *rec;

    idx = fxRecordFindFree(0);
    if (idx < 0)
        return -1;
    rec = &fxRecords[idx];
    rec->f34->model = weaponHomingMissileModel;
    rec->f02 = 10;
    fxRecordInitObject(rec, pos, dir);
    rec->f04 = 50;
    rec->f06 = 150;
    rec->f0A = a3;
    rec->f38 = 0;
    rec->f42 = kind;
    missileTrailStart(idx);
    missileTrailSetUVs(rec);
    fxTrailUpdate(rec, idx);
    return idx;
}

/* Update all 16 effect records; when none in state 51 is still alive and
 * effect 51 is enabled, trigger soundStop(51). */
void __attribute__((section(".text_800DC348"))) fxRecordsUpdate(void)
{
    s32 n;
    s32 i;

    n = 0;
    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        if (fxRecords[i].f04 == 0)
            fxRecordFree(i, 0);
        if (fxRecords[i].f04 >= -11)
            weaponProjectileUpdate(&fxRecords[i]);
        else
            fxRecords[i].fF0->visible = 0;
        if (fxRecords[i].f02 == 51 && fxRecords[i].f04 > 0)
            n++;
    }
    if (n == 0 && soundIsActive(51))
        soundStop(51);
}

/* libgpu-style primitive setup macros (pointer accesses). */
#define SetPolyF4(p) ((p)->len = 5, (p)->code = 40)
#define SetRGB0(p, R, G, B) ((p)->r = (R), (p)->g = (G), (p)->b = (B))
#define SetXY4(p, X0, Y0, X1, Y1, X2, Y2, X3, Y3)     ((p)->x0 = (X0), (p)->y0 = (Y0), (p)->x1 = (X1), (p)->y1 = (Y1),      (p)->x2 = (X2), (p)->y2 = (Y2), (p)->x3 = (X3), (p)->y3 = (Y3))

/* Set up the two full-screen semi-transparent FX quads with colour r,g,b
 * and set the FX timer (doubled when viewGetSplitMode() is set). */
void __attribute__((section(".text_800DC45C"))) fxSheetStart(s32 time, s32 r, s32 g, s32 b)
{
    SetPolyF4(&fxSheetPolys[0]);
    SetPolyF4(&fxSheetPolys[1]);
    SetRGB0(&fxSheetPolys[0], r, g, b);
    SetRGB0(&fxSheetPolys[1], r, g, b);
    SetXY4(&fxSheetPolys[0], 0, 0, 320, 0, 0, 240, 320, 240);
    SetXY4(&fxSheetPolys[1], 0, 0, 320, 0, 0, 240, 320, 240);
    SetSemiTrans(fxSheetPolys, 1);
    /* request 48 stage 2 (ONLINE_ROLLBACK.md 11, ONCE_PER_STEP): the countdown runs once per step, not once per
       drawn view, so the split screen's doubling goes */
#ifdef TM2_PORT_ROLLBACK
    if (viewGetSplitMode() && !portRollbackOncePerStep())
#else
    if (viewGetSplitMode())
#endif
        fxSheetTimer = time * 2;
    else
        fxSheetTimer = time;
}
