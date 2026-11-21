/* Online with bots, players dropping in (PC port request 33 "Online with bots, players dropping in" and "load a
 * joining player's car on demand"; docs/CAR_GROUPS.md 14, docs/PER_CAR_PLAN.md B.6 / B.7 with the port's answers
 * G.11-G.15).
 *
 * A match starts with a server and its bots; a player who joins does not take over a bot's car: one bot is
 * removed for good and the player starts fresh (as a respawn) on a car of the type they picked: a free loaded car
 * of that type, or a car loaded mid-match from buffers the port hands over. A player who leaves is removed the
 * same way; a bot may take the free place. The pieces (port code, .port_text):
 *   portCarRemove        a record and its car leave the match for good: specials, look and sound ended, its
 *                        projectiles, mines, drops, arcs, shields and flames removed at once (G.11), every
 *                        reference to its slot, object, object id or record cleared; the car parked (a free car
 *                        of its type); an AI record left vacant (safe for every loop over aiCarTotal), a human's
 *                        place filled by the last human (portHumanCompact: humans stay contiguous, G.13);
 *   portCarStartHuman /  a record started mid-match on a free car (load group) as a respawn: the per-record part
 *   portCarStartAi       of uaInit, the binding, fresh stats, weapons and HP at the free start farthest from the
 *                        live cars; humanCarTotal / aiCarTotal kept; at most TM2_PORT_CARS active cars;
 *   portCarLoadGroupLive a new load group (TMS + DMD from port buffers, no CD read, G.15) installed between frames,
 *                        into the resources earlier unloads gave back (DMD bytes, slots, key, group number,
 *                        coordinate systems);
 *   portCarUnloadGroup   a group no car uses (a removed car, a spare, a car of the match start) given back for good:
 *                        its DMD bytes (first-fit free list), car slots, overflow key, load group number, scene
 *                        objects (the coordinate-system free list csAlloc takes from first), per-key registries
 *                        and special state, CLUT copies and rows 240..255 spots, texture bank and image records,
 *                        sound instances; every reference to them cleared or moved (CAR_GROUPS.md 14.7);
 *   portCarRoom /        what is free (DMD bytes and the largest block, slots, keys, groups, coordinate systems,
 *   portCarCanLoad       ...) and whether a given car file would load now, so the port can decide on a join.
 *
 * With -DTM2_PORT_CAR_DROPIN (needs TM2_PORT_CAR_GROUPS, TM2_PORT_CARS and TM2_PORT_CAR_SWAP). Nothing changes
 * until the port calls one of the functions (portCarDropIn.live stays 0): the game runs its retail logic; csAlloc's
 * free list is empty until an unload, so its bump allocation is retail's. */
#ifndef TM2_SEMANTIC_CAR_DROPIN_H
#define TM2_SEMANTIC_CAR_DROPIN_H

#define TM2_PORT_DROPIN_AI 32          /* vacant flags (>= TM2_MAX_AI_CARS of any variant) */
#define TM2_PORT_DROPIN_GROUPS 64      /* per load group (= car_groups.h TM2_CAR_SLOT_CAP) */
#define TM2_PORT_DROPIN_OBJECT 0x128   /* a scene object (csObjectPool entry, TM2Object) */
/* Live-load storage (bitmapSetupStorage under the define): the VRAM usage bitmap (1024 x 512 bits + the word
 * retail keeps before the slots), the texture-slot table (5 words per slot, TM2_CAR_SLOT_CAP groups of
 * stride 32) and the image records (6 words each). */
#define TM2_PORT_LIVE_BITMAP_BYTES 0x10004
#define TM2_PORT_LIVE_SLOT_BYTES (64 * 32 * 20)
#define TM2_PORT_LIVE_RECORDS 4096
#define TM2_PORT_LIVE_RECORD_BYTES (TM2_PORT_LIVE_RECORDS * 24)

/* error codes of the API (negative) */
#define TM2_DROPIN_ERR_RECORD   (-1)   /* no such record, or not bound to its car */
#define TM2_DROPIN_ERR_GROUP    (-2)   /* no car in that load group, or it is not free */
#define TM2_DROPIN_ERR_FULL     (-3)   /* TM2_PORT_CARS active cars already (humans + bots) */
#define TM2_DROPIN_ERR_INDEX    (-4)   /* bad record index (human: not humanCarTotal; past the arrays) */
#define TM2_DROPIN_ERR_LAST     (-5)   /* the last human cannot leave */
#define TM2_DROPIN_ERR_NOLOAD   (-6)   /* no load group, slot, key or DMD space left */
#define TM2_DROPIN_ERR_VRAM     (-7)   /* the textures do not fit (no bank, no VRAM): nothing was installed */
#define TM2_DROPIN_ERR_BUFFER   (-8)   /* missing or bad buffer */
#define TM2_DROPIN_ERR_BUSY     (-9)   /* the group still has a driver; before the match load; attract demo */
#define TM2_DROPIN_ERR_TYPE     (-10)  /* a second car of a type whose special state is single in this build
                                          (unload: another loaded car of the type shares it) */
#define TM2_DROPIN_ERR_NODE     (-11)  /* portCarStartHumanAt: node past the arena's route nodes */
#define TM2_DROPIN_ERR_SHARED   (-12)  /* portCarUnloadGroup: another loaded group has the same car key (more
                                          duplicates of a type than overflow keys: retail sharing) */

/* Unload bookkeeping sizes (portCarUnload) */
#define TM2_PORT_UNLOAD_RANGES 66      /* free DMD ranges (one more than groups can leave holes between) */
#define TM2_PORT_UNLOAD_CS 512         /* coordinate systems on the free list (>= TM2_MAX_COORD_SYSTEMS of any
                                          variant: 320 with TM2_PORT_CARS=16 and TM2_PORT_BALANCE) */
#define TM2_PORT_SLOT_FREE (-1)        /* portCarGroups.slot_group of a slot an unload gave back */

/* portCarStartHuman / portCarStartAi flags */
#define TM2_DROPIN_START_RETAIL 1      /* human: carRestart's match-wide resets too (projectiles, drops, specials'
                                          round reset, effect sprites, HUD message), as retail's lost-a-life
                                          respawn; clear = only the record's own reset (the default) */
#define TM2_DROPIN_START_RANDOM 2      /* the retail start-node picker (random) instead of the farthest free */

#ifdef TM2_PORT_VARIANT
/* portCarDropIn (.port_bss, config/US/port_variant_arrays.tsv group one), written by the C; the port may read it
 * between frames. Offsets in bytes. */
struct TM2PortCarDropIn {
    int live;                                          /* +0x000 a drop-in function ran in this load */
    int removed;                                       /* +0x004 cars removed in this load */
    int started;                                       /* +0x008 records started in this load */
    int loaded;                                        /* +0x00C groups loaded live in this load */
    unsigned char vacant[TM2_PORT_DROPIN_AI];          /* +0x010 AI record j vacant (removed, not restarted) */
    unsigned char group_live[TM2_PORT_DROPIN_GROUPS];  /* +0x030 1 = loaded live, 2 = unloaded (its number free
                                                          for the next live load) */
    unsigned char *group_dmd[TM2_PORT_DROPIN_GROUPS];  /* +0x070 its DMD copy (the match load's and live loads) */
    int group_size[TM2_PORT_DROPIN_GROUPS];            /* +0x170 its size */
    unsigned char *dmd_next;                           /* +0x270 first free byte of the DMD area's tail (the free
                                                          ranges below it: portCarUnload) */
    unsigned int dmd_end;                              /* +0x274 the area's end */
    int next_group;                                    /* +0x278 the next load group number, 0 before the load */
    int start_live;                                    /* +0x27C 1 while a start picks its node (portStartNodePick) */
    unsigned char dummy[TM2_PORT_DROPIN_OBJECT];       /* +0x280 the scene object a vacant AI record points at */
};
typedef char tm2_port_car_dropin_size[(sizeof(struct TM2PortCarDropIn) == 0x3A8) ? 1 : -1];

/* portCarLiveTex (.port_bss, group one): the load's texture bookkeeping kept through the match under the define
 * (bitmapSetupStorage -> portCarLiveStorage), and a snapshot for a mid-match load that does not fit. Internal. */
struct TM2PortCarLiveTex {
    unsigned char bitmap[TM2_PORT_LIVE_BITMAP_BYTES];  /* bitmapWords (64 KB) and the 4 bytes retail keeps before
                                                          the slots */
    unsigned char slots[TM2_PORT_LIVE_SLOT_BYTES];     /* vramTexSlots: 5 words per slot, group * 32 + image */
    unsigned char records[TM2_PORT_LIVE_RECORD_BYTES]; /* bitmapRecords: 6 words per image */
    unsigned char backup[TM2_PORT_LIVE_BITMAP_BYTES];  /* the bitmap before a mid-match parse */
    unsigned int rows[32];                             /* portCarCluts.rows before it */
};
typedef char tm2_port_car_live_tex_size[(sizeof(struct TM2PortCarLiveTex) == 0x10004 * 2 + 0xA000 + 0x18000 + 0x80) ? 1 : -1];

/* portCarUnload (.port_bss, group one, after the port storage of every variant): what unloads gave back
 * (CAR_GROUPS.md 14.7). Written by the C; the port reads it through portCarRoom. Offsets in bytes. */
struct TM2PortCarUnload {
    int unloaded;                                      /* +0x000 groups unloaded in this load */
    int cs_count;                                      /* +0x004 coordinate systems on the free list */
    int cs_high;                                       /* +0x008 csPoolCount when the list last grew (a smaller
                                                          count = the pool was reset since: the list is stale) */
    int range_count;                                   /* +0x00C free DMD ranges below the tail */
    unsigned int range_start[TM2_PORT_UNLOAD_RANGES];  /* +0x010 by address, coalesced, 4-aligned */
    unsigned int range_size[TM2_PORT_UNLOAD_RANGES];   /* +0x118 */
    unsigned short cs_free[TM2_PORT_UNLOAD_CS];        /* +0x220 csObjectPool indices (a stack) */
    unsigned int clut256[TM2_PORT_DROPIN_GROUPS];      /* +0x620 per group: images whose CLUT copy is 256 wide
                                                          (TM2_PORT_CAR_CLUTS, portCarClutCopyOne) */
    int rec_first[TM2_PORT_DROPIN_GROUPS];             /* +0x720 per group: its image records (bitmapAddRecord: */
    int rec_end[TM2_PORT_DROPIN_GROUPS];               /* +0x820 one per image) [first, end), -1 = none */
    int rec_group;                                     /* +0x920 the group whose TMS the match load parses */
    int pad;                                           /* +0x924 */
};
typedef char tm2_port_car_unload_size[(sizeof(struct TM2PortCarUnload) == 0x928) ? 1 : -1];

/* portCarRoom(&room): what the match has free now (filled by the C between frames). A car needs: its DMD size in
 * one block (dmd_largest), one slot (Minion two consecutive: slots_run), a group number, an overflow key when its
 * type is in the match already, up to 24 coordinate systems, up to 16 animation entries and 4 CLUT animation
 * definitions, one image record per image, and VRAM (a bank with banks live; else room in VRAM, known only by
 * trying: portCarLoadGroupLive returns TM2_DROPIN_ERR_VRAM and changes nothing). */
struct TM2PortCarRoom {
    int dmd_free;           /* +0x00 bytes free in the DMD area (the holes and the tail) */
    int dmd_largest;        /* +0x04 the largest DMD that fits now (the largest hole or the tail) */
    int dmd_holes;          /* +0x08 free ranges below the tail (fragments) */
    int slots_free;         /* +0x0C car slots free below the vacant slot (given back + never used) */
    int slots_run;          /* +0x10 the longest run of consecutive free slots (Minion's file needs 2) */
    int keys_free;          /* +0x14 overflow keys free (a second car of a type needs one) */
    int groups_free;        /* +0x18 load group numbers free */
    int cs_free;            /* +0x1C coordinate systems free (an install takes up to 24) */
    int anim_free;          /* +0x20 animation list entries free (of 100; an install takes up to 16) */
    int clut_defs_free;     /* +0x24 CLUT animation definitions free (an install takes up to 4) */
    int records_free;       /* +0x28 image records free (of TM2_PORT_LIVE_RECORDS; one per image) */
    int banks_free;         /* +0x2C texture banks free (0 while banks are not live) */
    int rows16_free;        /* +0x30 16-colour CLUT spots free in rows 240..255 */
    int rows256_free;       /* +0x34 256-colour CLUT spots free in rows 240..255 */
    int groups_loaded;      /* +0x38 car groups loaded now (not unloaded) */
    int unloaded;           /* +0x3C groups unloaded in this load */
};
typedef char tm2_port_car_room_size[(sizeof(struct TM2PortCarRoom) == 0x40) ? 1 : -1];
#endif

#ifdef TM2_PORT_CAR_DROPIN
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_CAR_DROPIN needs TM2_PORT_CAR_GROUPS (cars by load group and key)"
#endif
#ifndef TM2_PORT_CARS
#error "TM2_PORT_CAR_DROPIN needs TM2_PORT_CARS (the DMD area, portCarSource, the farthest-free start picker)"
#endif
#ifndef TM2_PORT_CAR_SWAP
#error "TM2_PORT_CAR_DROPIN needs TM2_PORT_CAR_SWAP (the shared reset, bind and park blocks)"
#endif
#pragma tm2_absolute portCarDropIn
#pragma tm2_absolute portCarLiveTex
#pragma tm2_absolute portCarUnload
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortCarDropIn portCarDropIn;
extern struct TM2PortCarLiveTex portCarLiveTex;
extern struct TM2PortCarUnload portCarUnload;
/* .port_text (port_variant_arrays.c). kind: 0 human, 1 AI. Call between frames (the port's frame hook before
 * the car updates), never from inside a game callback. */
extern int  portCarRemove(int kind, int index);                     /* 1, or an error code */
extern void portHumanCompact(int from, int to);                     /* human record from -> to (portCarRemove) */
extern int  portCarStartHuman(int index, int group, int flags);     /* the new human index, or an error code */
extern int  portCarStartHumanAt(int index, int group, int flags, int node);
                                                                    /* the same at route node `node` (H5;
                                                                       < 0 = the farthest free start) */
extern int  portCarStartAi(int index, int group, int flags);        /* the AI index (-1 = pick), or an error */
extern int  portCarActiveCount(void);                               /* humans + AI records driving a car */
extern int  portCarGroupFree(int group);                            /* 1 = the group's car has no driver */
extern int  portCarFreeGroupOfType(int type);                       /* a free group of a car type, 0 = none */
extern int  portCarLoadGroupLive(const void *tms, int tms_size, const void *dmd, int dmd_size, int flags);
                                                                    /* a new group, or an error code */
extern int  portCarUnloadGroup(int group);                          /* 1, or an error code */
extern int  portCarRoom(struct TM2PortCarRoom *room);               /* fills *room; returns dmd_largest */
extern int  portCarCanLoad(const void *tms, int tms_size, const void *dmd, int dmd_size);
                                                                    /* 0 = portCarLoadGroupLive would load it
                                                                       (VRAM aside), else its error code */
/* game hooks */
extern void *portCsTake(void);                                      /* csAlloc: an object unloads gave back, 0 =
                                                                       none (allocate from the pool) */
extern int  portCarBindSkip(int ai, int index);                     /* sessionBindCars: a vacant AI record */
extern int  portCarBindCount(int ai, int count);                    /* sessionBindCars: records to bind */
extern void portCarLoadDone(unsigned char *next, unsigned int end, int groups);
                                                                    /* sessionLoadArenaAndCars: the DMD area left */
extern void portCarLiveStorage(void);                               /* bitmapSetupStorage: port blocks */
#ifdef __cplusplus
}
#endif
#endif

#endif
