typedef signed char s8; typedef unsigned char u8; typedef unsigned short u16; typedef int s32;

#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern int gameGetLevel(void);
extern int aiGetDifficulty(void);
#include "semantic/weapon_calls.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"

/* Shared tables owned elsewhere (declarations repeated per TU; the header set is frozen). */
struct TM2Area_C160 { signed char field_0; u8 field_1[3]; u16 field_4; u8 field_6[2]; s32 field_8[3]; u8 *field_14[5]; };
struct TM2Emitter_A7A0 { short field_0; u16 field_2; u8 field_4[24]; u8 *field_1C; };
struct TM2EffectShot { short f0, model, range, f6; int f8, life; int f10[7]; void *obj; };
struct TM2EffectTable { void *slots[TM2_MAX_RICOCHETS]; struct TM2EffectShot shots[TM2_MAX_RICOCHETS]; };
struct TM2SpecialRecord { short life, owner; void *lookup; void *obj; };
struct TM2SpecialTable { void *slots[TM2_MAX_SHIELDS]; struct TM2SpecialRecord records[TM2_MAX_SHIELDS]; };
extern struct TM2Area_C160 groundFires[30];         /* @801842C4 fx_ground_fires.c */
extern struct TM2Emitter_A7A0 weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS];       /* @80184774 weapon_flame_update.c */
extern struct TM2EffectTable weaponRicochetPool;           /* @80184860 asset_pools.c */
extern struct TM2SpecialTable weaponShieldPool;          /* @80184A68 asset_pools.c */
extern unsigned char *weaponMineObjects[TM2_MAX_MINE_OBJECTS]; /* @80184AA8 asset_pools.c (23; request 40) */
extern unsigned char *weaponRemoteBombObjects[];                /* @80180F30 asset_pools.c (unsized: addressed absolutely) */
extern unsigned char *weaponTornadoObject;                  /* @80180F38 asset_pools.c ($gp) */
extern unsigned char *weaponTornadoFunnelObject;                  /* @80180F3C asset_pools.c ($gp) */
extern unsigned char *darkToothBossObject;                  /* @80180F40 asset_pools.c / ai_dark_tooth_boss_object.c ($gp) */
extern int weaponFireInterval;                             /* @80180F44 weapon_tick_init.c ($gp) */
extern unsigned char *weaponStrikeObject[];                /* @80184E70 asset_pools.c (unsized: addressed absolutely) */
extern int weaponSlamStateCount;                             /* @80180AF0 pickup record count ($gp), weaponSlamStateAlloc */

/* Storage owned here: zero-initialized retail BSS blocks that this reset writes first. */
struct TM2SlotRecord { short id, f2, count, f6; u8 pad[12]; };                           /* 20 bytes; weaponArcSlotFind reads id (+0, as weaponArcs) and count (+4, as weaponArcsCountField) */
struct TM2PickupRecord { u8 b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, bA, bB, bC, bD, bE, bF; short h10; }; /* 18 bytes, weaponSlamStateAlloc */
struct TM2Table_B04 { int f0, f4; short status; short fA; int fC, f10, f14; int active; };   /* 28 bytes; weaponDropSlotFind reads status (+8, as weaponDropsStatusField) */
struct TM2ShieldState { short timer; short f2; short kind; short f6; u8 *anchor; u8 *flags; u8 *object;
                        u8 f14, f15; short f16; int f18, f1C; short f20, f22; int f24, f28, f2C, f30, f34, f38, f3C, f40, f44, f48, f4C; };
struct TM2StrikeState { signed char mode; char f1; unsigned short owner_id; short active; short timer; int velocity[3], delta[3], previous[3]; };
typedef char tm2_round_sizes[(sizeof(struct TM2SlotRecord) == 20 && sizeof(struct TM2PickupRecord) == 18 && sizeof(struct TM2Table_B04) == 28 &&
                              sizeof(struct TM2ShieldState) == 80 && sizeof(struct TM2StrikeState) == 44 && sizeof(struct TM2Area_C160) == 40 &&
                              sizeof(struct TM2Emitter_A7A0) == 32 && sizeof(struct TM2EffectTable) == 52 * TM2_MAX_RICOCHETS &&
                              sizeof(struct TM2SpecialTable) == 16 * TM2_MAX_SHIELDS) ? 1 : -1];
#if TM2_MAX_ARCS > TM2_RETAIL_ARCS
extern struct TM2SlotRecord weaponArcs[TM2_MAX_ARCS];   /* TM2_PORT_CARS: port_variant_arrays.c (.port_bss) */
#else
struct TM2SlotRecord weaponArcs[5] __attribute__((section(".tm2_801847B4"))) = {{0}};     /* @801847B4..80184818 */
#endif
#if TM2_MAX_SLAM_RECORDS > TM2_RETAIL_SLAM_RECORDS
extern struct TM2PickupRecord weaponSlamRecords[TM2_MAX_SLAM_RECORDS];   /* TM2_PORT_CARS: port_variant_arrays.c */
#else
struct TM2PickupRecord weaponSlamRecords[4] __attribute__((section(".tm2_80184818"))) = {{0}};   /* @80184818..80184860 */
#endif
#if TM2_MAX_DROPS > TM2_RETAIL_DROPS
extern struct TM2Table_B04 weaponDrops[TM2_MAX_DROPS];   /* request 40 (TM2_PORT_BALANCE + TM2_PORT_CARS): port_variant_arrays.c */
#else
struct TM2Table_B04 weaponDrops[25] __attribute__((section(".tm2_80184B04"))) = {{0}};     /* @80184B04..80184DC0 (follows weaponMineObjects) */
#endif
struct TM2ShieldState weaponTornadoState __attribute__((section(".tm2_80184DC0"))) = {0};         /* @80184DC0..80184E10 */
struct TM2StrikeState weaponStrikeState __attribute__((section(".tm2_80184E10"))) = {0};         /* @80184E10..80184E3C */

/* @80108CF0. Round reset of the weapon state: weaponFireInterval = (80 - 4 * gameGetLevel()) >> aiGetDifficulty() (logical);
 * the 30 effect areas are cleared (kind byte, count, three words, five pointers); the five slot records get count -1;
 * both emitters get a zero timer; every effect shot loses its range and, when it holds an object, that object's word
 * +8 is cleared and the pointer dropped; the four special records likewise (life, owner, lookup, object). The shield
 * byte +0x14 and halfword +0x22 are cleared and, while its timer is positive, the timer is cleared and the objects
 * held in weaponTornadoObject / weaponTornadoFunnelObject get their word +8 cleared. The strike active/timer halfwords are cleared, the
 * weaponSlamStateCount pickup records are reset (bytes 0,2,7,11 = 0, bytes 1,3 = 1, halfword +0x10 = 0), and the two camera
 * setters slamClawSetAngle(739) / slamArmSetAngle(-113) run last. Callers (carRestart, roundStart) ignore $v0. */
void __attribute__((section(".text_80108CF0")))
weaponsRoundReset(void)
{
    /* Each loop counter is its own variable: GCC eliminates a counter only
     * when its last use is inside that loop, as retail does for a, s and e. */
    int timer, count, shift, i, k, a, s, e;

    count = gameGetLevel();
    shift = aiGetDifficulty();
    weaponFireInterval = (unsigned int)(80 - 4 * count) >> shift;
    for (a = 0; a < 30; a++) {
        groundFires[a].field_0 = 0;
        groundFires[a].field_4 = 0;
        groundFires[a].field_8[0] = 0;
        groundFires[a].field_8[1] = 0;
        groundFires[a].field_8[2] = 0;
        for (k = 0; k < 5; k++)
            groundFires[a].field_14[k] = 0;
    }
    for (s = 0; s < TM2_MAX_ARCS; s++)
        weaponArcs[s].count = -1;
    for (e = 0; e < TM2_MAX_FLAME_EMITTERS; e++)
        weaponFlameEmitters[e].field_0 = 0;
    for (i = 0; i < TM2_MAX_RICOCHETS; i++) {
        weaponRicochetPool.shots[i].range = 0;
        if (weaponRicochetPool.shots[i].obj != 0) {
            *(int *)((u8 *)weaponRicochetPool.shots[i].obj + 8) = 0;
            weaponRicochetPool.shots[i].obj = 0;
        }
    }
    for (i = 0; i < TM2_MAX_SHIELDS; i++) {
        weaponShieldPool.records[i].life = 0;
        weaponShieldPool.records[i].owner = 0;
        weaponShieldPool.records[i].lookup = 0;
        if (weaponShieldPool.records[i].obj != 0) {
            *(int *)((u8 *)weaponShieldPool.records[i].obj + 8) = 0;
            weaponShieldPool.records[i].obj = 0;
        }
    }
    timer = weaponTornadoState.timer;   /* read first: keeps &weaponTornadoState in a register */
    weaponTornadoState.f14 = 0;
    weaponTornadoState.f22 = 0;
    if (timer > 0) {
        weaponTornadoState.timer = 0;
        if (weaponTornadoObject != 0)
            *(int *)(weaponTornadoObject + 8) = 0;
        if (weaponTornadoFunnelObject != 0)
            *(int *)(weaponTornadoFunnelObject + 8) = 0;
    }
    weaponStrikeState.active = 0;
    weaponStrikeState.timer = 0;
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): the claw records past weaponSlamRecords are portCarSpecials' */
    for (i = 0; i < weaponSlamStateCount; i++) {
        struct TM2PickupRecord *rec = (struct TM2PickupRecord *)portCarClawRecord(i);

        rec->b0 = 0;
        rec->b1 = 1;
        rec->b2 = 0;
        rec->b7 = 0;
        rec->b3 = 1;
        rec->bB = 0;
        rec->h10 = 0;
    }
#else
    for (i = 0; i < weaponSlamStateCount; i++) {
        weaponSlamRecords[i].b0 = 0;
        weaponSlamRecords[i].b1 = 1;
        weaponSlamRecords[i].b2 = 0;
        weaponSlamRecords[i].b7 = 0;
        weaponSlamRecords[i].b3 = 1;
        weaponSlamRecords[i].bB = 0;
        weaponSlamRecords[i].h10 = 0;
    }
#endif
    slamClawSetAngle(739);
    slamArmSetAngle(-113);
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialsRoundReset();    /* the same for every duplicate's special state */
#endif
}

/* @80108F70. Boot-time clear of the weapon object tables: the ten effect-shot objects, the four special lookups and
 * objects, the weaponTornadoObject / weaponTornadoFunnelObject / darkToothBossObject objects, the shield object, weaponStrikeObject, the 25 weaponDrops words +0x18,
 * the 23 weaponMineObjects objects and the two weaponRemoteBombObjects objects. Leaf; the caller (arenaResetSystems) ignores $v0. */
void __attribute__((section(".text_80108F70")))
weaponObjectsDetach(void)
{
    struct TM2SpecialRecord *rec;
    u8 **ptr;
    int i, off;

    /* Retail walks the shot objects and the weaponDrops words by descending byte offset; kept in that form so the
     * body fits its 50-word slot (the typed index form materializes each element address separately). */
    for (off = (TM2_MAX_RICOCHETS - 1) * sizeof(struct TM2EffectShot); off >= 0; off -= sizeof(struct TM2EffectShot))
        *(void **)((u8 *)weaponRicochetPool.shots + off + 0x2c) = 0;
    for (i = 0; i < TM2_MAX_SHIELDS; i++) {
        weaponShieldPool.records[i].lookup = 0;
        weaponShieldPool.records[i].obj = 0;
    }
    weaponTornadoObject = 0;
    weaponTornadoFunnelObject = 0;
    weaponTornadoState.object = 0;
    weaponStrikeObject[0] = 0;
    darkToothBossObject = 0;
    for (off = (TM2_MAX_DROPS - 1) * sizeof(struct TM2Table_B04); off >= 0; off -= sizeof(struct TM2Table_B04))
        *(int *)((u8 *)weaponDrops + off + 0x18) = 0;
    for (i = 0, ptr = weaponMineObjects; i < TM2_MAX_MINE_OBJECTS; i++)
        *ptr++ = 0;
    for (i = 0, ptr = weaponRemoteBombObjects; i < TM2_MAX_REMOTE_BOMBS; i++)
        *ptr++ = 0;
}
