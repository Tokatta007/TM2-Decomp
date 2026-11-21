/* More small game helpers: slot searches, table setters, rotation setup,
 * menu/sound presets. Names stay address-based. */
/* humanCars is declared below with this file's own record view. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"
#include "semantic/settings.h"
#include "semantic/car_groups.h"
#include "semantic/port_cars.h"
#include "semantic/car_specials.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVector;

typedef struct {
    s32 f00;
    s16 f04;
    u8 pad_06[0x2E];
    s32 *f34;
    u8 pad_38[980 - 0x38];
} TM2Rec980;

typedef struct {
    s32 f00, f04;
    s32 x, y, z;
} TM2Rec20;

typedef struct {
    s32 f00;
    u8 pad_04[56];
} TM2Rec60;

/* Random event 0 at moscowDishState (started by moscowDishStartDestroy, updated by moscowDishUpdate). */
typedef struct {
    u8 active;
    s8 countdown;
    u8 pad_02[10];
    s32 armed;
    s32 pos[3];
} TM2RandomEvent;

typedef struct {
    s16 f00;
    u8 pad_02[30];
} TM2Rec32;

typedef struct {
    s8 f00;
    u8 pad_01[39];
} TM2Rec40;

typedef struct {
    s32 f00;
    u8 pad_04[8];
} TM2Rec12;

/* Human car record (2 x 2068 bytes at humanCars): only the Eternal Life flag,
 * stats.damageable in include/semantic/cars.h (TM2HumanRecord +0x1F4). */
typedef struct {
    u8 pad_000[0x1F4];
    s32 damageable;         /* +0x1F4 0 = takes no damage (Eternal Life cheat) */
    u8 pad_1F8[0x814 - 0x1F8];
} TM2HumanCarLife;

/* Route point (24 bytes at aiRouteNodes): only the successor count. */
typedef struct {
    u8 pad_00[22];
    s8 count;               /* +22 successor count */
    s8 route;               /* +23 */
} TM2RouteNodeCount;

extern u8 *slamArmObject;
extern u8 *slamClawObject;
extern s32 slamArmAngle;
extern s32 slamClawAngle;
extern s32 aiDifficulty;
extern u8 arenaShortChaseRange;
extern s32 *fxPlatformSoundObject;
extern s32 *fxTargetHitObject;
extern u8 *trackVehicleRecords;
extern s32 pickupRegisteredCount;
extern TM2RandomEvent moscowDishState;
extern TM2RouteNodeCount aiRouteNodes[];
extern s32 eiffelTowerObject, eiffelTowerPivotX, eiffelTowerPivotY, eiffelTowerPivotZ;
extern s16 eiffelTowerRotX, eiffelTowerRotY, eiffelTowerRotZ;
extern s32 audioCdVolume, audioSfxVolume;
#pragma tm2_absolute eiffelTowerObject
#pragma tm2_absolute eiffelTowerPivotX
#pragma tm2_absolute eiffelTowerPivotY
#pragma tm2_absolute eiffelTowerPivotZ
#pragma tm2_absolute eiffelTowerRotX
#pragma tm2_absolute eiffelTowerRotY
#pragma tm2_absolute eiffelTowerRotZ
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioSfxVolume
extern TM2Rec980 fxRecords[];
extern u8 fxRecordTrailSegments[];
extern TM2Rec20 worldFlameMarkers[];
extern TM2Rec60 bulletsLifeField[];
extern TM2Rec32 weaponFlameEmitters[];
extern TM2Rec40 groundFires[];
extern TM2Rec12 worldSectionFloorHeights[];
extern TM2HumanCarLife humanCars[];
extern s32 *weaponMineObjects[];
extern s32 *weaponRemoteBombObjects[];
extern s32 *fxWakeObjects[];
extern u8 *fxSplashObjects[];
extern u8 *fxLavaSplashObjects[];
extern u8 *fxBlastObjects[];
extern u8 *carPartsHammerhead[4];   /* car part k (1-based) of each boss/special car */
extern u8 *carPartsMinion[6];
extern u8 *carPartsMrSlam[4];
extern u8 *carPartsDarkTooth[4];
extern u8 *carAxelObjects[2];
extern u8 fxSprites[];
extern u8 pickupSlotsFlagField[];
extern s32 pickupAmmoTable[];
extern u8 carResetWeapons[];
extern char textFormatBuffer[];
extern char fileExtDmd[], fileExtTms[], strDmdDir[], strFileNameFmtDirStemExt[], strTmsDir[];

extern void *RotMatrixYXZ(TM2SVector *, u8 *);
extern void *RotMatrixX(s32, u8 *);
#include "semantic/fx.h"
extern void viewSetBgColor(s32, s32, s32);
extern void hdpSetSingleBoxTest(s32);
extern void routeLoadFwy(void), terrainLoadGrpFwy(void), terrainLoadRmpFwy(void);
#if defined(TM2_PORT_FWY_PARAPETS) && !defined(TM2_PORT_FWY_GROUND)
#define TM2_PORT_FWY_GROUND             /* request 112's first name */
#endif
#ifdef TM2_PORT_FWY_GROUND
extern u8 worldSurfaces[];                /* RMP surfaces, 28 bytes each (car_wheels.c TM2Surface), 225 records */
extern u8 worldSectionFirstSurface[];     /* section -> first surface */
extern u8 worldSectionLastSurface[];      /* section -> last surface */
extern s32 terrainSectionCount;           /* GRP section count */

/* PC port request 112 (RETAIL_BUGS 42 and 43): ground FWY.RMP lacks under drawn tops of FREEWAY.DMD, as round
   surfaces (type 2, corner heights by angle and radius: inner sn, inner cs, outer sn, outer cs). Groups in the order
   of their section's last disc surface: section, count, centre x, y; then per surface radius from, to, angle from,
   to, the four heights. Made from the drawn polygons (docs/PLAYTEST_ANSWERS.md 30 and 31):
   - section 0, 12, 13 (the ground-level curves NW, SW, SE): the outer lip at both ends of each curve (rising from
     96 to about 170 where the ground stays near 96) and the inner kerb (16..48 above the ground);
   The elevated curve's hedge-wall tops (440, no ground on the disc) are left as retail: the wall's hit box stops
   every car below them (tested by the user, 2026-11-10), so nothing can reach them. */
static const s16 portFwyGroundTable[] __attribute__((section(".port_rodata"))) = {
    0, 31, -5376, 2688,   /* section 0: after surface 51 */
        2234, 2391, -597, -461, 96, 96, 41, 44,
        2234, 2398, -814, -651, 96, 96, 19, 33,
        2234, 2394, -651, -597, 96, 96, 33, 41,
        2234, 2397, -461, -298, 96, 96, 44, 26,
        2234, 2399, -298, -190, 96, 96, 26, 16,
        2387, 2482, -624, -461, 36, 44, 71, 80,
        2389, 2482, -461, -407, 44, 36, 80, 71,
        2476, 2567, -570, -461, 80, 80, 116, 116,
        2478, 2569, -597, -570, 76, 80, 112, 116,
        2479, 2570, -461, -434, 80, 76, 116, 112,
        2965, 3078, -868, -841, 131, 136, 145, 153,
        2973, 3038, -950, -922, 44, 61, 113, 119,
        2974, 3062, -82, -54, 44, 26, 114, 108,
        2974, 3070, -976, -949, 26, 44, 107, 113,
        2971, 3092, -163, -136, 131, 126, 146, 138,
        2975, 3109, -1004, -976, 9, 26, 101, 107,
        2975, 3097, -895, -868, 125, 131, 137, 145,
        2975, 3099, -54, -27, 26, 9, 108, 102,
        2975, 3142, -1024, -1003, 0, 9, 96, 101,
        2976, 3142, -27, 0, 9, 0, 102, 96,
        2990, 3122, -922, -895, 119, 125, 129, 137,
        2985, 3115, -136, -108, 126, 120, 138, 130,
        3012, 3153, -950, -922, 113, 119, 120, 129,
        3006, 3145, -109, -81, 120, 114, 130, 122,
        3031, 3179, -82, -54, 114, 108, 122, 113,
        3038, 3190, -976, -950, 107, 113, 112, 120,
        3062, 3221, -54, -27, 108, 102, 113, 105,
        3070, 3233, -1004, -976, 101, 107, 103, 112,
        3099, 3270, -27, 0, 102, 96, 105, 96,
        3190, 3234, -1004, -976, 135, 112, 135, 112,
        3179, 3224, -54, -26, 113, 137, 113, 137,
    13, 24, 5376, -2688,   /* section 13: after surface 103 */
        2234, 2391, 1451, 1587, 96, 96, 41, 44,
        2234, 2397, 1289, 1451, 96, 96, 24, 41,
        2234, 2397, 1587, 1750, 96, 96, 44, 26,
        2234, 2398, 1750, 1804, 96, 96, 26, 21,
        2973, 3038, 1098, 1126, 44, 61, 113, 119,
        2974, 3062, 1966, 1994, 44, 26, 114, 108,
        2974, 3070, 1072, 1099, 26, 44, 107, 113,
        2971, 3092, 1885, 1912, 131, 126, 146, 138,
        2975, 3109, 1044, 1072, 9, 26, 101, 107,
        2975, 3097, 1153, 1180, 125, 131, 137, 145,
        2975, 3099, 1994, 2021, 26, 9, 108, 102,
        2975, 3142, 1024, 1045, 0, 9, 96, 101,
        2976, 3142, 2021, 2048, 9, 0, 102, 96,
        2990, 3122, 1126, 1153, 119, 125, 129, 137,
        2985, 3115, 1912, 1940, 126, 120, 138, 130,
        3012, 3153, 1098, 1126, 113, 119, 120, 129,
        3006, 3145, 1939, 1967, 120, 114, 130, 122,
        3031, 3179, 1966, 1994, 114, 108, 122, 113,
        3038, 3190, 1072, 1098, 107, 113, 112, 120,
        3062, 3221, 1994, 2021, 108, 102, 113, 105,
        3070, 3233, 1044, 1072, 101, 107, 103, 112,
        3099, 3270, 2021, 2048, 102, 96, 105, 96,
        3190, 3234, 1044, 1072, 135, 112, 135, 112,
        3179, 3224, 1994, 2022, 113, 137, 113, 137,
    12, 24, -5376, -2688,   /* section 12: after surface 127 */
        2234, 2391, -1621, -1485, 96, 96, 41, 44,
        2234, 2397, -1783, -1621, 96, 96, 24, 41,
        2234, 2397, -1485, -1322, 96, 96, 44, 26,
        2234, 2398, -1322, -1268, 96, 96, 26, 21,
        2973, 3038, -1974, -1946, 44, 61, 113, 119,
        2974, 3062, -1106, -1078, 44, 26, 114, 108,
        2974, 3070, -2000, -1973, 26, 44, 107, 113,
        2971, 3092, -1187, -1160, 131, 126, 146, 138,
        2975, 3109, -2028, -2000, 9, 26, 101, 107,
        2975, 3097, -1919, -1892, 125, 131, 137, 145,
        2975, 3099, -1078, -1051, 26, 9, 108, 102,
        2975, 3142, -2048, -2027, 0, 9, 96, 101,
        2976, 3142, -1051, -1024, 9, 0, 102, 96,
        2990, 3122, -1946, -1919, 119, 125, 129, 137,
        2985, 3115, -1160, -1132, 126, 120, 138, 130,
        3012, 3153, -1974, -1946, 113, 119, 120, 129,
        3006, 3145, -1133, -1105, 120, 114, 130, 122,
        3031, 3179, -1106, -1078, 114, 108, 122, 113,
        3038, 3190, -2000, -1974, 107, 113, 112, 120,
        3062, 3221, -1078, -1051, 108, 102, 113, 105,
        3070, 3233, -2028, -2000, 101, 107, 103, 112,
        3099, 3270, -1051, -1024, 102, 96, 105, 96,
        3190, 3234, -2028, -2000, 135, 112, 135, 112,
        3179, 3224, -1078, -1050, 113, 137, 113, 137,
    -1
};
#endif
extern void routeLoadDish(void), terrainLoadGrpDish(void), terrainLoadRmpDish(void);
extern void routeLoadIce(void), terrainLoadGrpIce(void), terrainLoadRmpIce(void), worldCollapseClearDone(void);
extern void routeLoadLava(void), terrainLoadGrpLava(void), terrainLoadRmpLava(void);
extern void routeLoadParis(void), terrainLoadGrpParis(void), terrainLoadRmpParis(void);
extern void routeLoadHkong(void), terrainLoadGrpHkong(void), terrainLoadRmpHkong(void);
extern void routeLoadRoof(void), terrainLoadGrpRoof(void), terrainLoadRmpRoof(void);
extern void routeLoadSwamp(void), terrainLoadGrpSwamp(void), terrainLoadRmpSwamp(void);
extern void routeLoadCyb(void), terrainLoadGrpCyb(void), terrainLoadRmpCyb(void);
extern void routeLoadSroof(void), terrainLoadGrpSroof(void), terrainLoadRmpSroof(void);
extern void moscowDishStartDestroy(void);
#include "semantic/world.h"
extern void wheelBlockSetHeight(struct TM2CarWheels *wheels, s32);
extern void aiCarsSetActive(u8);
extern void worldTrackVehiclePlace(s32);
extern struct TM2HumanRecord *humanCarFindById(s32 id);
extern s32 gameGetLevel(void);
extern u8 worldSectionHasMovingSurfaces(s32);
extern int sprintf(char *, const char *, ...);
extern s32 renderGetView0Frames(void);
extern s32 screenDisplayImage(s32, s32, s32, s32);
extern s32 aiPickWeaponFromTable9(struct TM2AIRecord *ai);
extern void aiPickWeaponFromTable10(struct TM2AIRecord *ai);
extern void mathBoundSAngle(s16 *);

/* @8011526C */
void __attribute__((section(".text_8011526C"))) slamClawSetAngle(s32 angle)
{
    TM2SVector v;

    if (slamClawObject) {
        slamClawAngle = angle;
        v.vx = angle;
        v.vy = 0;
        v.vz = 0;
        RotMatrixYXZ(&v, slamClawObject + 4);
    }
}

/* @8011ED4C: 1 for car types 4..11, 14, 15, 17, 18. */
u8 __attribute__((section(".text_8011ED4C"))) worldSectionHasMovingSurfaces(s32 type)
{
    u8 r = 0;

    if ((u32)(type - 4) < 8 || (u32)(type - 14) < 2 || (u32)(type - 17) < 2)
        r = 1;
    return r;
}

/* @800D5F54 */
void __attribute__((section(".text_800D5F54"))) moscowDishRegister(s32 a, s32 *v)
{
    TM2RandomEvent *e = &moscowDishState;

    e->countdown = 1;
    e->armed = a;
    e->pos[0] = v[0];
    e->pos[1] = v[1];
    e->pos[2] = v[2];
}

/* @800DC308 */
TM2Rec980 *__attribute__((section(".text_800DC308"))) fxRecordGet(u32 index)
{
    return index < TM2_MAX_PROJECTILES ? &fxRecords[index] : 0;
}

/* @80109E84: first of 23 entries whose +8 is clear, else -1. */
s32 __attribute__((section(".text_80109E84"))) weaponMineObjectFind(void)
{
    s32 i;

    for (i = 0; i < TM2_MINE_OBJECTS_LIVE; i++)   /* 23 (request 40: the objects made this match) */
        if (!weaponMineObjects[i][2])
            return i;
    return -1;
}

/* @8011D94C: by difficulty: 75 easy, 120 medium, 140 hard. */
s32 __attribute__((section(".text_8011D94C"))) aiDifficultySpeedScale(void)
{
    s32 v;

    switch (aiDifficulty) {
    case 0:
    default:
        v = 75;
        break;
    case 1:
        v = 120;
        break;
    case 2:
        v = 140;
        break;
    }
    return v;
}

/* @800D6638 */
void __attribute__((section(".text_800D6638"))) worldFlameMarkerSetPos(s32 *v, s32 index)
{
    worldFlameMarkers[index].x = v[0];
    worldFlameMarkers[index].y = v[1];
    worldFlameMarkers[index].z = v[2];
}

/* @800D9198 */
void __attribute__((section(".text_800D9198"))) fxRecordsClear(void)
{
    s32 i;

    for (i = 0; i < TM2_PROJECTILES_LIVE; i++)
        fxRecordFree(i, 0);
}

/* @800F603C */
void __attribute__((section(".text_800F603C"))) aiRoutePatchEiffelTower(void)
{
    aiRouteNodes[90].count = 2;
    aiRouteNodes[92].count = 2;
    aiRouteNodes[93].count = 2;
    aiRouteNodes[95].count = 2;
    aiRouteNodes[100].count = 2;
    aiRouteNodes[104].count = 2;
    aiRouteNodes[106].count = 2;
}

/* @8010A5AC */
s16 __attribute__((section(".text_8010A5AC"))) weaponFlameSlotFind(void)
{
    s32 i;

    for (i = 0; i < TM2_POOL_LIVE(FLAME_EMITTERS); i++)
        if (weaponFlameEmitters[i].f00 <= 0)
            return i;
    return -1;
}

#ifdef TM2_PORT_FWY_GROUND
/* Request 112: add portFwyGroundTable's surfaces, each group right after its section's last disc surface, so every
   section's surfaces stay one range (carWheelProbe walks first..last). The 128 disc records move up in place,
   backwards; then every section's first and last move with them. The disc's own records are unchanged, only
   renumbered, and the count (terrainSurfaceCount, 128) is only the moving surfaces' base on level 8, so it stays.
   Every machine loads the same; the surfaces and the section table are in the rollback state with the rest of the
   ground. */
void __attribute__((section(".port_text"))) portFwyGroundAdd(void)
{
    const s16 *grp[8];
    s32 at[8], n[8];
    const s16 *g, *e;
    s32 groups, add, k, i, j, dst, x, f, l;
    u8 *d;
    s32 *w;

    groups = 0;
    add = 0;
    for (g = portFwyGroundTable; g[0] >= 0 && groups < 8; g += 4 + 8 * g[1]) {
        grp[groups] = g;
        at[groups] = worldSectionLastSurface[g[0]];
        n[groups] = g[1];
        add += g[1];
        groups++;
    }
    if (128 + add > 225)
        return;
    dst = 128 + add;
    for (k = 127; k >= 0; k--) {
        for (i = groups - 1; i >= 0; i--) {
            if (at[i] != k)
                continue;
            g = grp[i];
            for (j = n[i] - 1; j >= 0; j--) {
                e = g + 4 + 8 * j;
                dst--;
                d = worldSurfaces + 28 * dst;
                d[0] = 2;                           /* round */
                d[1] = 1;                           /* heights by angle and radius */
                *(s16 *)(d + 0x02) = -1;            /* no section event */
                *(s16 *)(d + 0x04) = e[0];
                *(s16 *)(d + 0x06) = e[1];
                *(s16 *)(d + 0x08) = e[2];
                *(s16 *)(d + 0x0A) = e[3];
                *(s16 *)(d + 0x0C) = e[4];
                *(s16 *)(d + 0x0E) = e[5];
                *(s16 *)(d + 0x10) = e[6];
                *(s16 *)(d + 0x12) = e[7];
                *(s32 *)(d + 0x14) = g[2];
                *(s32 *)(d + 0x18) = g[3];
            }
        }
        dst--;
        if (dst != k) {
            w = (s32 *)(worldSurfaces + 28 * dst);
            for (j = 0; j < 7; j++)
                w[j] = ((s32 *)(worldSurfaces + 28 * k))[j];
        }
    }
    for (i = 0; i < terrainSectionCount; i++) {
        f = worldSectionFirstSurface[i];
        l = worldSectionLastSurface[i];
        if (f > l)
            continue;                               /* no surfaces (1..0) */
        x = 0;
        for (j = 0; j < groups; j++)
            if (at[j] < f)
                x += n[j];
        worldSectionFirstSurface[i] = f + x;
        x = 0;
        for (j = 0; j < groups; j++)
            if (at[j] <= l)
                x += n[j];
        worldSectionLastSurface[i] = l + x;
    }
}
#endif

/* @80119D08 */
void __attribute__((section(".text_80119D08"))) arenaLoadLosAngeles(void)
{
    arenaShortChaseRange = 0;
    viewSetBgColor(104, 88, 72);
    routeLoadFwy();
    terrainLoadGrpFwy();
    terrainLoadRmpFwy();
#ifdef TM2_PORT_FWY_GROUND
    portFwyGroundAdd();
#endif
}

/* @80119D4C */
void __attribute__((section(".text_80119D4C"))) arenaLoadMoscow(void)
{
    arenaShortChaseRange = 0;
    viewSetBgColor(100, 100, 93);
    routeLoadDish();
    terrainLoadGrpDish();
    terrainLoadRmpDish();
}

/* @80119D90 */
void __attribute__((section(".text_80119D90"))) arenaLoadAntarctica(void)
{
    arenaShortChaseRange = 0;
    viewSetBgColor(0, 61, 72);
    routeLoadIce();
    terrainLoadGrpIce();
    terrainLoadRmpIce();
    worldCollapseClearDone();
}

/* @80119DDC */
void __attribute__((section(".text_80119DDC"))) arenaLoadAmazonia(void)
{
    arenaShortChaseRange = 1;
    viewSetBgColor(60, 45, 20);
    routeLoadLava();
    terrainLoadGrpLava();
    terrainLoadRmpLava();
}

/* @80119E24 */
void __attribute__((section(".text_80119E24"))) arenaLoadParis(void)
{
    arenaShortChaseRange = 1;
    viewSetBgColor(80, 105, 110);
    routeLoadParis();
    terrainLoadGrpParis();
    terrainLoadRmpParis();
}

/* @80119E6C */
void __attribute__((section(".text_80119E6C"))) arenaLoadHongKong(void)
{
    arenaShortChaseRange = 1;
    viewSetBgColor(50, 60, 65);
    routeLoadHkong();
    terrainLoadGrpHkong();
    terrainLoadRmpHkong();
}

/* @80119EB4 */
void __attribute__((section(".text_80119EB4"))) arenaLoadNewYork(void)
{
    arenaShortChaseRange = 1;
    viewSetBgColor(0, 0, 0);
    routeLoadRoof();
    terrainLoadGrpRoof();
    terrainLoadRmpRoof();
}

/* @80119F4C */
void __attribute__((section(".text_80119F4C"))) arenaLoadSuicideSwamp(void)
{
    hdpSetSingleBoxTest(0);
    viewSetBgColor(106, 180, 218);
    routeLoadSwamp();
    terrainLoadGrpSwamp();
    terrainLoadRmpSwamp();
}

/* @80119F94 */
void __attribute__((section(".text_80119F94"))) arenaLoadCyburbia(void)
{
    hdpSetSingleBoxTest(0);
    viewSetBgColor(110, 110, 100);
    routeLoadCyb();
    terrainLoadGrpCyb();
    terrainLoadRmpCyb();
}

/* @80119FDC */
void __attribute__((section(".text_80119FDC"))) arenaLoadRoofTops(void)
{
    hdpSetSingleBoxTest(0);
    viewSetBgColor(0, 20, 0);
    routeLoadSroof();
    terrainLoadGrpSroof();
    terrainLoadRmpSroof();
}

/* @801220B0 */
s32 __attribute__((section(".text_801220B0"))) sceneObjGetChoiceState(u8 **p)
{
    u8 *q = p[1];

    if (q && *q == 9)
        return p[1][7];
    return -1;
}

/* @800D6140 */
void __attribute__((section(".text_800D6140"))) moscowDishDamage(s32 n)
{
    TM2RandomEvent *e = &moscowDishState;

#ifdef TM2_PORT_HAZARDS
    if (portArenaOff(TM2_ARENA_MOSCOW_DISH))   /* request 38 G9: the dish takes no damage (its countdown stays) */
        return;
#endif
    e->countdown -= n;
    if (e->countdown <= 0)
        moscowDishStartDestroy();
}

/* @800D6DE8 */
void __attribute__((section(".text_800D6DE8"))) eiffelTowerRegister(s32 a)
{
    eiffelTowerPivotY = -6144;
    eiffelTowerPivotX = 0;
    eiffelTowerPivotZ = 240;
    eiffelTowerRotX = 0;
    eiffelTowerRotY = 0;
    eiffelTowerRotZ = 0;
    eiffelTowerObject = a;
}

/* @800F08A4 */
void __attribute__((section(".text_800F08A4"))) wheelSinkStep(struct TM2CarWheel *wheel)
{
    wheel->crush = mathBlendValue(wheel->crush, 320, 92);
    wheel->f09 = 1;
}

/* @800F08EC */
void __attribute__((section(".text_800F08EC"))) wheelSinkRecover(struct TM2CarWheel *wheel)
{
    wheel->crush -= (s16)timerGetFrameTicks() * 8;
    if (wheel->crush < 0) {
        wheel->crush = 0;
        wheel->f09 = 0;
    }
}

/* @800F3628 */
void __attribute__((section(".text_800F3628"))) wheelBlockSetSection(struct TM2CarWheels *w, s32 index)
{
    w->wheel[0].kind = index;
    w->wheel[1].kind = index;
    w->wheel[2].kind = index;
    w->wheel[3].kind = index;
    wheelBlockSetHeight(w, worldSectionFloorHeights[index].f00);
}

/* @8010C118 */
s32 __attribute__((section(".text_8010C118"))) groundFireFindFree(void)
{
    s32 i;

    for (i = 0; groundFires[i].f00 && i < 30; i++)
        ;
    return i;
}

/* @80115188 */
void __attribute__((section(".text_80115188"))) slamArmRegister(u8 *object)
{
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): the arm of the Mr. Slam being installed (a duplicate: its own) */
    int t = portCarSpecialSelect(portCarLoadKey());
#endif
    if (object && *object == 5) {
        slamArmObject = object;
        slamArmAngle = -113;
        RotMatrixX(-113, object + 4);
    }
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(t);
#endif
}

/* @801151D0 */
void __attribute__((section(".text_801151D0"))) slamClawRegister(u8 *object)
{
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a: the claw of the Mr. Slam being installed */
    int t = portCarSpecialSelect(portCarLoadKey());
#endif
    if (object && *object == 5) {
        slamClawObject = object;
        slamClawAngle = 739;
        RotMatrixX(739, object + 4);
    }
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(t);
#endif
}

/* @8011912C */
void __attribute__((section(".text_8011912C"))) fxRegisterSplashObject(u8 *object)
{
    if (*(u16 *)(object + 16) < 5 && *object == 4)
        fxSplashObjects[*(u16 *)(object + 16)] = object;
}

/* @801192C4 */
void __attribute__((section(".text_801192C4"))) fxRegisterLavaSplashObject(u8 *object)
{
    if (*(u16 *)(object + 16) < 5 && *object == 4)
        fxLavaSplashObjects[*(u16 *)(object + 16)] = object;
}

/* @80119420 */
void __attribute__((section(".text_80119420"))) fxRegisterBlastObject(u8 *object)
{
    if (*(u16 *)(object + 16) < 5 && *object == 4)
        fxBlastObjects[*(u16 *)(object + 16)] = object;
}

/* @800D914C */
void __attribute__((section(".text_800D914C"))) fxRecordFree(s16 index, s32 unused)
{
    s32 *p = fxRecords[index].f34;

    fxRecords[index].f04 = -1;
    p[2] = 0;
}

/* @801195C0 */
s32 *__attribute__((section(".text_801195C0"))) fxGetFreeWakeObject(void)
{
    s32 i;

    for (i = 0; i < 5; i++)
        if (!fxWakeObjects[i][2])
            return fxWakeObjects[i];
    return 0;
}

/* @800D3C2C */
void __attribute__((section(".text_800D3C2C"))) aiPickWeapon(struct TM2AIRecord *ai)
{
    switch (ai->f0D4) {
    case 0:
        aiPickWeaponFromTable9(ai);
        break;
    case 1:
        aiPickWeaponFromTable10(ai);
        break;
    }
}

/* @80109EC4 */
s32 __attribute__((section(".text_80109EC4"))) weaponBombObjectFind(void)
{
    s32 i;

    for (i = 0; i < TM2_MAX_REMOTE_BOMBS; i++)   /* objects not created this load are 0 */
        if (weaponRemoteBombObjects[i] && !weaponRemoteBombObjects[i][2])
            return i;
    return -1;
}

/* @80112820: reset the 150 (TM2_MAX_FX_SPRITES) 48-byte records at fxSprites (slot -1, id = index),
 * then fxImpactParticlesClear. Per-iteration &D[i*48] form matches the hoisting order. */
void __attribute__((section(".text_80112820"))) fxSpritesReset(void)
{
    s32 i;
    u8 *rec;

    for (i = 0; i < TM2_MAX_FX_SPRITES; i++) {
        rec = &fxSprites[i * 48];
        *(s8 *)(rec + 10) = -1;
        *(s32 *)(rec + 16) = 0;
        rec[7] = i;
        *(s16 *)(rec + 12) = 0;
    }
    fxImpactParticlesClear();
}

/* @801149B8 */
void __attribute__((section(".text_801149B8"))) carPartRegisterHammerhead(u8 *object)
{
    u16 k;

    if (object && *object == 5) {
        k = *(u16 *)(object + 36);
#ifdef TM2_PORT_CAR_GROUPS
        if ((u32)(k - 1) < 4)
            portCarGroups.parts[(portCarLoadKey() ? portCarLoadKey() : 0x20) >> 4][k - 1] = object;   /* request 32 */
#else
        if ((u32)(k - 1) < 4)
            carPartsHammerhead[k - 1] = object;
#endif
    }
}

/* @80114A08 */
void __attribute__((section(".text_80114A08"))) carPartRegisterMinion(u8 *object)
{
    u16 k;

    if (object && *object == 5) {
        k = *(u16 *)(object + 36);
#ifdef TM2_PORT_CAR_GROUPS
        if ((u32)(k - 1) < 6)
            portCarGroups.parts[(portCarLoadKey() ? portCarLoadKey() : 0xE0) >> 4][k - 1] = object;   /* request 32 */
#else
        if ((u32)(k - 1) < 6)
            carPartsMinion[k - 1] = object;
#endif
    }
}

/* @80114A58 */
void __attribute__((section(".text_80114A58"))) carPartRegisterMrSlam(u8 *object)
{
    u16 k;

    if (object && *object == 5) {
        k = *(u16 *)(object + 36);
#ifdef TM2_PORT_CAR_GROUPS
        if ((u32)(k - 1) < 4)
            portCarGroups.parts[(portCarLoadKey() ? portCarLoadKey() : 0xC0) >> 4][k - 1] = object;   /* request 32 */
#else
        if ((u32)(k - 1) < 4)
            carPartsMrSlam[k - 1] = object;
#endif
    }
}

/* @80114AA8 */
void __attribute__((section(".text_80114AA8"))) carPartRegisterDarkTooth(u8 *object)
{
    u16 k;

    if (object && *object == 5) {
        k = *(u16 *)(object + 36);
#ifdef TM2_PORT_CAR_GROUPS
        if ((u32)(k - 1) < 4)
            portCarGroups.parts[(portCarLoadKey() ? portCarLoadKey() : 0xF0) >> 4][k - 1] = object;   /* request 32 */
#else
        if ((u32)(k - 1) < 4)
            carPartsDarkTooth[k - 1] = object;
#endif
    }
}

/* @80114AF8 */
void __attribute__((section(".text_80114AF8"))) carPartRegisterAxel(u8 *object)
{
    u16 k;

    if (object && *object == 5) {
        k = *(u16 *)(object + 36);
#ifdef TM2_PORT_CAR_GROUPS
        if ((u32)(k - 1) < 2)
            portCarGroups.parts[(portCarLoadKey() ? portCarLoadKey() : 0xB0) >> 4][k - 1] = object;   /* request 32 */
#else
        if ((u32)(k - 1) < 2)
            carAxelObjects[k - 1] = object;
#endif
    }
}

/* @8011A340 */
void __attribute__((section(".text_8011A340"))) humanCarSetEternalLife(s32 index, u8 on)
{
    humanCars[index].damageable = !on;
    if (!on)
        aiCarsSetActive(1);
}

/* @8010AE04 */
void __attribute__((section(".text_8010AE04"))) pickupSlotsReset(void)
{
    s32 i, j;

    for (j = 49 * 16; j >= 0; j -= 16)
        *(s16 *)(pickupSlotsFlagField + j) = 0;
    pickupRegisteredCount = 0;
    for (i = 0; i < 10; i++)
        pickupAmmoTable[i] = 2;
    pickupAmmoTable[3] = 1;
}

/* @80117978: attach object to record index (56-byte records at trackVehicleRecords,
 * field +52), tag it with index + 100, then worldTrackVehiclePlace. Unused "old" load
 * is retail's dead read; no rec local keeps retail's address operand order. */
void __attribute__((section(".text_80117978"))) worldTrackVehicleAttach(u8 *object, s32 index)
{
    u8 *rec;
    u8 *old;

    if (object) {
        old = *(u8 **)(trackVehicleRecords + index * 56 + 52);
        *(u8 **)(trackVehicleRecords + index * 56 + 52) = object;
        *(s32 *)(*(u8 **)(trackVehicleRecords + index * 56 + 52) + 280) = index + 100;
        worldTrackVehiclePlace(index);
    }
}

/* @80119530 */
void __attribute__((section(".text_80119530"))) fxTargetHitAt(s32 *v)
{
    if (fxTargetHitObject) {
        fxTargetHitObject[42] = v[0];
        fxTargetHitObject[43] = v[1];
        fxTargetHitObject[44] = v[2];
        objRegistrySetStep(0x29D, 0, 1);
    }
}

/* @8011909C */
void __attribute__((section(".text_8011909C"))) fxPlatformSoundStart(s32 *v)
{
    if (fxPlatformSoundObject) {
        fxPlatformSoundObject[42] = v[0];
        fxPlatformSoundObject[43] = v[1];
        fxPlatformSoundObject[44] = v[2];
        fxPlatformSoundObject[2] = 1;
        objRegistrySetStep(0xC1F, 0, 1);
    }
}

/* @800D8A7C: set the three colour bytes (+16..18) of all 18 36-byte entries of
 * 980-byte record index (< 16) at fxRecordTrailSegments. Indexed rec[i*36+N] form keeps
 * the stores ahead of i++; retail does not mask a/b/c at entry. */
void __attribute__((section(".text_800D8A7C"))) missileSetTrailColor(u32 index, s32 a, s32 b, s32 c)
{
    s32 i;
    u8 *rec;

    if (index < TM2_MAX_PROJECTILES) {
        rec = fxRecordTrailSegments + index * 980;
        for (i = 0; i < 18; i++) {
            rec[i * 36 + 16] = a;
            rec[i * 36 + 17] = b;
            rec[i * 36 + 18] = c;
        }
    }
}

/* @8011E114 */
void __attribute__((section(".text_8011E114"))) sub_8011E114(s32 id)
{
    struct TM2HumanRecord *human = humanCarFindById(id);

    carRestart(human, human->type, carResetWeapons[human->player]);
    human->stats.wreck_state = 4;
}

/* @8011ECF4 */
s32 __attribute__((section(".text_8011ECF4"))) worldMovingSurfaceCount(s32 type)
{
    s32 r = 0;

    if (gameGetLevel() == 8)
        r = worldSectionHasMovingSurfaces(type) ? 2 : 0;
    return r;
}

/* @8012A940: default settings. */
void __attribute__((section(".text_8012A940"))) settingsInitDefaults(struct TM2Settings *settings)
{
    settings->humans = 0;
    settings->opponents = 0;
    settings->car_select_player = 0;
    settings->opponents_picked = 0;
    settings->level = 1;
    settings->boss_intro = 0;
    settings->dark_tooth_stage = 0;
    settings->dark_tooth_reveal = 0;
    settings->difficulty = 1;
    settings->world_map = 1;
    settings->cd_volume = audioCdVolume;
    settings->sfx_volume = audioSfxVolume;
}

/* @8012AB20 */
char *__attribute__((section(".text_8012AB20"))) carDmdFileName(struct TM2Settings *settings, s32 car)
{
    sprintf(textFormatBuffer, strFileNameFmtDirStemExt, strDmdDir, carFileCode(settings, car), fileExtDmd);
    return textFormatBuffer;
}

/* @8012AB78 */
char *__attribute__((section(".text_8012AB78"))) carTmsFileName(struct TM2Settings *settings, s32 car)
{
    sprintf(textFormatBuffer, strFileNameFmtDirStemExt, strTmsDir, carFileCode(settings, car), fileExtTms);
    return textFormatBuffer;
}

/* @800D91DC: first free record from `start` on, else -1. */
s32 __attribute__((section(".text_800D91DC"))) fxRecordFindFree(s32 start)
{
    s32 i;

    for (i = start; i < TM2_PROJECTILES_LIVE; i++)
        if (fxRecords[i].f04 <= 0)
            return i;
    return -1;
}

/* @8012201C */
void __attribute__((section(".text_8012201C"))) sceneDrawBackground(u8 *p, s32 value)
{
    if (p[44])
        screenDisplayImage(value != renderGetView0Frames(), *(s32 *)(p + 40), 0, 0);
}

/* @800E46F4 */
void __attribute__((section(".text_800E46F4"))) carContactSetTargetHeading(void *car, u8 human)
{
    struct TM2CarMotion *m;
    struct TM2CarContact *w;

    if (human) {
        m = &HREC(car)->motion;
        w = &HREC(car)->contact;
    } else {
        m = &AI(car)->motion;
        w = &AI(car)->contact;
    }
    w->f08 = *(u16 *)&w->hit.f10 + m->rot[2];
    mathBoundSAngle(&w->f08);
    w->f03 = 1;
}
