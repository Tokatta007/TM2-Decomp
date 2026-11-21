/* More cars for the PC port (request 32 step 2, docs/CAR_GROUPS.md "Step 2", docs/CAR_GROUPS_PLAN.md 2).
 *
 * The limits themselves are in players.h (TM2_MAX_AI_CARS, TM2_MAX_CAR_SLOTS and the pools); this header
 * declares the port's run-time switches and storage that the changed game functions read under
 * -DTM2_PORT_CARS=N (the port's `--port-define TM2_PORT_CARS=16` on a port variant build). The storage is
 * defined in every variant (src/semantic/port_variant_arrays.c, .port_bss). All 0 = retail. */
#ifndef TM2_SEMANTIC_PORT_CARS_H
#define TM2_SEMANTIC_PORT_CARS_H

#include "semantic/players.h"

/* portCarOptions bits (request 32; bit 0 is step 1's, TM2_PORT_CAR_GROUPS) */
#define TM2_PORT_CAR_OPT_REPEATS 1      /* the car pickers allow repeated cars (carIsSelectable) */
#define TM2_PORT_CAR_OPT_HIGH_DMD 2     /* car DMDs load into portCarDmdArea instead of after the level */
#define TM2_PORT_CAR_OPT_FAR_STARTS 4   /* startNodePickRandom picks the start farthest from the cars */
/* the end of retail's load area (the EXE's first byte, 0x800CDC54: the level DMD and the car DMDs after
 * it must end there); the car DMD area's size is TM2_PORT_CAR_DMD_AREA_BYTES (players.h) */
#define TM2_PORT_LOW_AREA_END 0x800CDC54
/* A car file the port hands over for one load group (mod cars, request 33), instead of the disc's
 * TMS\XX.TMS / DMD\XX.DMD: pointer and size per pass, 0 = the disc file. portCarSource[group],
 * group 1..TM2_MAX_LOADED_CARS (human i = i + 1, opponent i = humans + i + 1; entry 0 unused). */
#ifdef TM2_PORT_VARIANT
struct TM2PortCarSource {
    const unsigned char *tms;
    int tms_size;
    const unsigned char *dmd;
    int dmd_size;
};
#endif
#define TM2_PORT_CAR_OPT_POOLS 8        /* the weapon pools, the sound instances and requests use their TM2_MAX_* size
                                         * (retail's while clear: a full pool refuses as retail) */
/* Request 49 (CAR_GROUPS.md 7.4): the cars placed this round, for OPT_FAR_STARTS' "all taken" case. Defined in
 * every variant (port_variant_arrays.c, .port_bss, after the rollback words); written only with TM2_PORT_CARS. */
#define TM2_PORT_START_AI 16            /* car[] index of AI car 0 (humans 0..15) */
#define TM2_PORT_START_CARS 64
#ifdef TM2_PORT_VARIANT
struct TM2PortStartPlaced {
    unsigned char car[TM2_PORT_START_CARS];   /* +0x00 1 = its start ran since startNodesClearUsed: human h at h, AI car
                                                 a at TM2_PORT_START_AI + a */
    unsigned char picks[16];                  /* +0x40 the picks of each start-list entry since then (all cases) */
};
/* Request 85 (SIXTEEN_CARS.md 5.5): with OPT_FAR_STARTS, when the start node the pick chose (the "all taken" case) lies
 * within TM2_START_LINK_CLEAR of a live car placed this round, the pick looks along the route's links for the point
 * farthest from those cars (up to 15 points a link, every 256 or more); when that one is at least
 * TM2_START_LINK_CLEAR away, the pick returns the link's first node and leaves the point here, and the car's start
 * (humanCarPlaceAtStart, aiCarStart) puts the car there instead of on the node: on the ground (worldGroundHeightAt,
 * probed from TM2_START_LINK_PROBE above the higher end), facing along the link; an AI car's next node is the link's
 * second. A point over a gap (its ground more than TM2_START_LINK_DROP below both ends: Roof Tops) is refused (the node
 * as before). Written and taken within one car's start; no rand(). */
#define TM2_START_LINK_CLEAR 700        /* position units, straight-line distance */
#define TM2_START_LINK_PROBE 256        /* the ground probe starts this far above the link's higher end */
#define TM2_START_LINK_DROP 160         /* a point whose ground is more than this below both ends (a gap): no link start */
struct TM2PortStartLink {
    int set;                                  /* +0x00 1 = a link start is waiting for the car being started */
    int x, y;                                 /* +0x04 the point */
    int z;                                    /* +0x0C the link's higher end (route node height) */
    int from, to;                             /* +0x10 the link (route nodes) */
};                                            /* 0x18 bytes */
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortStartPlaced portStartPlaced;
extern struct TM2PortStartLink portStartLink;
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_CARS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_CARS needs the port variant (TM2_PORT_VARIANT)"
#endif
#pragma tm2_absolute portCarOptions
#pragma tm2_absolute portCarsPerArena
#pragma tm2_absolute portCarDmdArea
#ifdef __cplusplus
extern "C" {
#endif
extern int portCarOptions;          /* the port's switches for request 32 (bits above), 0 = retail */
extern int portCarsPerArena;        /* cars per arena, humans included; 0 = retail table */
extern int portCarDmdArea[];        /* the car DMD area (1 MiB + one sector), used with OPT_HIGH_DMD */
extern int portStartNodePick(int count);   /* .port_text: the farthest-free start picker */
extern void portStartPlacedClear(void);    /* .port_text, request 49: startNodesClearUsed (no car placed) */
extern void portStartPlacedMark(void *car, int human, int placed);   /* request 49: around a car's start pick */
extern int portStartLinkPlace(int *pos, int *heading, int *ground, int *to); /* request 85: a car's start, after its
                                         node placement: 1 when a link start waits (pos x, y, the ground height, the
                                         heading along the link, its second node), else 0 */
extern struct TM2PortCarSource portCarSource[];  /* per load group, 0 = disc files */
extern void *portSoundSortList[];   /* soundDriverUpdate's priority list with the raised pool */
#if TM2_MAX_SOUND_REQUESTS > TM2_RETAIL_SOUND_REQUESTS
extern int portSndObjSlots[];       /* the raised sound request pool's objects (main; request 33-4) */
#pragma tm2_absolute portSndObjSlots
#endif
#pragma tm2_absolute portSoundSortList
#pragma tm2_absolute portCarSource
/* .port_text: load car file `car` (car-select index) of load group `group` to dst (dmd: 0 the TMS, 1 the
 * DMD): the port's buffer when portCarSource has one that fits below `end`, else the disc file
 * (fileioLoadFileIntoRam, as retail). Returns the size. */
extern int portCarLoadGroup(unsigned char *dst, int group, int car, int dmd, unsigned int end);
extern int portCarPoolsLive;        /* portCarOptions bit 3 latched by carTablesClear at each load */
#pragma tm2_absolute portCarPoolsLive
#define TM2_PORT_POOLS_ON ((portCarOptions & TM2_PORT_CAR_OPT_POOLS) != 0)
/* A weapon pool's size this match: TM2_MAX_* when bit 3 was set at the load (carTablesClear, before the
 * level and car DMDs create the pools' objects), else retail's, so a pool never changes size during a
 * match and a full retail-sized pool refuses as retail does. */
#define TM2_POOL_LIVE(name) (portCarPoolsLive ? TM2_MAX_##name : TM2_RETAIL_##name)
#define TM2_PROJECTILES_LIVE TM2_POOL_LIVE(PROJECTILES)
/* the sound instances the allocator may hand out (read at each allocation) */
#define TM2_SOUND_INSTANCES_LIVE (TM2_PORT_POOLS_ON ? TM2_MAX_SOUND_INSTANCES : TM2_RETAIL_SOUND_INSTANCES)
/* the sound requests sndPoolAlloc may hand out (read at each allocation; request 33 step 4) */
#define TM2_SOUND_REQUESTS_LIVE (TM2_PORT_POOLS_ON ? TM2_MAX_SOUND_REQUESTS : TM2_RETAIL_SOUND_REQUESTS)
#ifdef __cplusplus
}
#endif
#else
#define TM2_POOL_LIVE(name) TM2_MAX_##name
#define TM2_PROJECTILES_LIVE TM2_MAX_PROJECTILES
#define TM2_SOUND_INSTANCES_LIVE TM2_MAX_SOUND_INSTANCES
#define TM2_SOUND_REQUESTS_LIVE TM2_MAX_SOUND_REQUESTS
#endif
/* The drop records and mine objects this match (request 40, docs/BALANCE.md 3.3): raised only with
 * TM2_PORT_BALANCE and TM2_PORT_CARS (players.h); while not raised, retail's constant (25 / 23). */
#if TM2_MAX_DROPS > TM2_RETAIL_DROPS
#define TM2_DROPS_LIVE TM2_POOL_LIVE(DROPS)
#else
#define TM2_DROPS_LIVE TM2_RETAIL_DROPS
#endif
#if TM2_MAX_MINE_OBJECTS > TM2_RETAIL_MINE_OBJECTS
#define TM2_MINE_OBJECTS_LIVE TM2_POOL_LIVE(MINE_OBJECTS)
#else
#define TM2_MINE_OBJECTS_LIVE TM2_RETAIL_MINE_OBJECTS
#endif

#endif
