/* Small game helpers (sound presets 8010..8010B). Names stay address-based. */
#include "semantic/players.h"   /* TM2_MAX_REMOTE_BOMBS */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* Track route-point block: 0xD20 bytes copied from the load buffer (byte-aligned copy). */
typedef struct { u8 b[0xD20]; } TM2RouteBlock;

extern s32 aiRouteNodeCount;               /* route point count ($gp) */
extern u8 *fileioWorkBuffer;               /* file load buffer */
#pragma tm2_absolute fileioWorkBuffer
extern TM2RouteBlock aiRouteNodes;     /* route points (see ai_steps.c) */
extern s32 fileioLoadFileIntoRam(void *, const char *);

extern char strTerrainFwyPts[], strTerrainDishPts[], strTerrainIcePts[], strTerrainLavaPts[], strTerrainParisPts[], strTerrainHkongPts[];
extern char strTerrainRoofPts[], strTerrainDenPts[], strTerrainSwampPts[], strTerrainCybPts[], strTerrainSroofPts[];
extern void startNodesFwy(void), startNodesDish(void), startNodesIce(void), startNodesLava(void);
extern void startNodesParis(void), startNodesHkong(void), startNodesRoof(void), startNodesDen(void);
extern void startNodesSwamp(void), startNodesCyb(void), startNodesSroof(void);

/* Per-track route setup: set route point count, run the track's companion setup, load
 * TERRAIN\<track>.PTS into the load buffer and copy its 0xD20 bytes to aiRouteNodes.
 * (__builtin_memcpy, not struct assignment: under -fvolatile a struct copy through a
 * pointer becomes a memcpy call, the builtin gives retail's inline aligned/unaligned loop.) */
#define ROUTE_LOAD(count, setup, name) \
    aiRouteNodeCount = count; \
    setup(); \
    fileioLoadFileIntoRam(fileioWorkBuffer, name); \
    __builtin_memcpy(&aiRouteNodes, fileioWorkBuffer, sizeof(TM2RouteBlock))

/* FWY: 79 route points, TERRAIN\FWY.PTS */
void __attribute__((section(".text_80101B58"))) routeLoadFwy(void)
{
    ROUTE_LOAD(79, startNodesFwy, strTerrainFwyPts);
}
/* DISH: 33 route points */
void __attribute__((section(".text_80101CDC"))) routeLoadDish(void)
{
    ROUTE_LOAD(33, startNodesDish, strTerrainDishPts);
}
/* ICE: 79 route points */
void __attribute__((section(".text_80101E9C"))) routeLoadIce(void)
{
    ROUTE_LOAD(79, startNodesIce, strTerrainIcePts);
}
/* LAVA: 94 route points */
void __attribute__((section(".text_8010202C"))) routeLoadLava(void)
{
    ROUTE_LOAD(94, startNodesLava, strTerrainLavaPts);
}
/* PARIS: 118 route points */
void __attribute__((section(".text_801021B8"))) routeLoadParis(void)
{
    ROUTE_LOAD(118, startNodesParis, strTerrainParisPts);
}
/* HKONG: 140 route points */
void __attribute__((section(".text_8010236C"))) routeLoadHkong(void)
{
    ROUTE_LOAD(140, startNodesHkong, strTerrainHkongPts);
}
/* ROOF: 101 route points */
void __attribute__((section(".text_801024F0"))) routeLoadRoof(void)
{
    ROUTE_LOAD(101, startNodesRoof, strTerrainRoofPts);
}
/* DEN: 58 route points */
void __attribute__((section(".text_801026AC"))) routeLoadDen(void)
{
    ROUTE_LOAD(58, startNodesDen, strTerrainDenPts);
}
/* SWAMP: 8 route points */
void __attribute__((section(".text_80102814"))) routeLoadSwamp(void)
{
    ROUTE_LOAD(8, startNodesSwamp, strTerrainSwampPts);
}
/* CYB: 10 route points */
void __attribute__((section(".text_80102994"))) routeLoadCyb(void)
{
    ROUTE_LOAD(10, startNodesCyb, strTerrainCybPts);
}
/* SROOF: 9 route points */
void __attribute__((section(".text_80102B08"))) routeLoadSroof(void)
{
    ROUTE_LOAD(9, startNodesSroof, strTerrainSroofPts);
}

/* Weapon-shot records (28 bytes) at weaponDrops and the asset object pools. */
struct TM2ShotRec { u8 f0, f1; u8 pad2[6]; s16 f8; u8 padA[14]; s32 *obj; };
extern struct TM2ShotRec weaponDrops[TM2_MAX_DROPS];          /* 25 (request 40: TM2_MAX_DROPS) */
extern s32 *weaponMineObjects[TM2_MAX_MINE_OBJECTS];          /* 23 */
extern s32 *weaponRemoteBombObjects[];

/* Round reset of weapon objects: clear the 25 shot records (state bytes, halfword +8) and
 * detach their objects (object word +8 = 0, pointer dropped); then clear word +8 of every
 * object held in the 23-entry and 2-entry asset pools. */
void __attribute__((section(".text_80109440"))) weaponDropsReset(void)
{
    s32 i;
    s32 **p;

    for (i = 0; i < TM2_MAX_DROPS; i++) {
        weaponDrops[i].f8 = 0;
        weaponDrops[i].f0 = 0;
        weaponDrops[i].f1 = 0;
        if (weaponDrops[i].obj) {
            weaponDrops[i].obj[2] = 0;
            weaponDrops[i].obj = 0;
        }
    }
    for (i = 0, p = weaponMineObjects; i < TM2_MAX_MINE_OBJECTS; i++, p++) {
        if (*p)
            (*p)[2] = 0;
    }
    for (i = 0, p = weaponRemoteBombObjects; i < TM2_MAX_REMOTE_BOMBS; i++, p++) {
        if (*p)
            (*p)[2] = 0;
    }
}
