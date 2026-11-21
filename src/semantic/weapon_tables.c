typedef unsigned char u8;

#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
/* Blocks owned by round_reset.c (declarations repeated per TU; the header set is frozen). */
struct TM2SlotRecord { short id, f2, count, f6; u8 pad[12]; };                                     /* 20 bytes @801847B4 */
struct TM2PickupRecord { u8 b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, bA, bB, bC, bD, bE, bF; short h10; }; /* 18 bytes @80184818 */
struct TM2Table_B04 { int f0, f4; short status; short fA; int fC, f10, f14; int active; };           /* 28 bytes @80184B04 */
typedef char tm2_table_sizes[(sizeof(struct TM2SlotRecord) == 20 && sizeof(struct TM2PickupRecord) == 18 && sizeof(struct TM2Table_B04) == 28) ? 1 : -1];
extern struct TM2SlotRecord weaponArcs[TM2_MAX_ARCS];
extern struct TM2PickupRecord weaponSlamRecords[TM2_MAX_SLAM_RECORDS];
extern struct TM2Table_B04 weaponDrops[TM2_MAX_DROPS];   /* 25 (request 40) */
#pragma tm2_absolute weaponArcs
#pragma tm2_absolute weaponSlamRecords
#pragma tm2_absolute weaponDrops

#if TM2_MAX_SLAM_RECORDS > TM2_RETAIL_SLAM_RECORDS
extern unsigned char weaponRicochetPool[];   /* the retail overflow target of the fifth claw record */
#pragma tm2_absolute weaponRicochetPool
#endif

/* @80180AF0: the pickup record cursor, a $gp word (retail reaches it gp-relative). */
extern int weaponSlamStateCount;

/* @80107574. Slot lookup: walks the five slot records; the first record with a non-positive count returns its index,
 * a record whose id equals the argument returns -1; -1 when all five are taken by other ids. Callers: weaponTaserTarget,
 * weaponLightningScan. */
int __attribute__((section(".text_80107574")))
weaponArcSlotFind(int id)
{
    int i;

    for (i = 0; i < TM2_POOL_LIVE(ARCS); i++) {
        if (weaponArcs[i].count <= 0)
            return i;
        if (weaponArcs[i].id == id)
            return -1;
    }
    return -1;
}

/* @80109648. First free entry of the 25-record table (status halfword +8 non-positive), -1 when full. Caller: weaponDropWrite
 * (request 40: portBalanceMineSlot / portBalanceBombSlot with TM2_PORT_BALANCE). The records in use this match:
 * TM2_DROPS_LIVE (port_cars.h; retail's 25 unless TM2_PORT_BALANCE and TM2_PORT_CARS raise the table). */
int __attribute__((section(".text_80109648")))
weaponDropSlotFind(void)
{
    int i;

    for (i = 0; i < TM2_DROPS_LIVE; i++) {
        if (weaponDrops[i].status <= 0)
            return i;
    }
    return -1;
}

/* @8010EE58. Pickup record cursor reset. */
void __attribute__((section(".text_8010EE58")))
weaponSlamStatesReset(void)
{
    weaponSlamStateCount = 0;
}

/* @8010EE64. Next pickup record: returns the record at the cursor (computed before the cursor moves, as in retail, so a
 * cursor of 4 still addresses the fifth 18-byte slot) and advances the cursor, wrapping to 0 after index 4. Caller:
 * weaponSlamStateInit. */
struct TM2PickupRecord * __attribute__((section(".text_8010EE64")))
weaponSlamStateAlloc(void)
{
    int i = weaponSlamStateCount;
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): weaponSlamRecords (retail's 4, or TM2_PORT_CARS' raised table,
       whatever portCarOptions bit 3) then portCarSpecials' TM2_PORT_CLAW_EXTRA records: one per Mr. Slam
       record; retail's fifth record over weaponRicochetPool is never handed out. The first records and the
       cursor are retail's (a match without duplicates allocates at most one per Mr. Slam record) */
    struct TM2PickupRecord *rec = (struct TM2PickupRecord *)portCarClawRecord(i);

    if (i < TM2_MAX_SLAM_RECORDS + TM2_PORT_CLAW_EXTRA - 1)
        weaponSlamStateCount = i + 1;
    else
        weaponSlamStateCount = 0;
    return rec;
#elif TM2_MAX_SLAM_RECORDS > TM2_RETAIL_SLAM_RECORDS
    /* TM2_PORT_CARS: with the raised pools (portCarPoolsLive) records 0..TM2_MAX_SLAM_RECORDS-1 in the port
       table; else retail's five, the fifth at the end of its four records, over the ricochet table's first
       object pointers (retail behaviour, kept: RETAIL_BUGS) */
    struct TM2PickupRecord *rec = portCarPoolsLive || i < TM2_RETAIL_SLAM_RECORDS
        ? (struct TM2PickupRecord *)((u8 *)weaponSlamRecords + i * 18) : (struct TM2PickupRecord *)weaponRicochetPool;

    if (i < (portCarPoolsLive ? TM2_MAX_SLAM_RECORDS - 1 : TM2_RETAIL_SLAM_RECORDS))
        weaponSlamStateCount = i + 1;
    else
        weaponSlamStateCount = 0;
    return rec;
#else
    struct TM2PickupRecord *rec = (struct TM2PickupRecord *)((u8 *)weaponSlamRecords + i * 18);

    if (i < 4)
        weaponSlamStateCount = i + 1;
    else
        weaponSlamStateCount = 0;
    return rec;
#endif
}
