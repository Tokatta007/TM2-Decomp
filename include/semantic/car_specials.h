/* Per-car specials for the PC port (request 33 step 2a, docs/CAR_GROUPS.md 10, docs/PER_CAR_PLAN.md C.2).
 *
 * Retail keeps the state of five specials once (one global each): Mr. Slam's arm and claw (objects and
 * angles), Twister's tornado (state, tornado and funnel objects), Shadow's strike (the 0x64-byte block and
 * the 2325 animation), Axel's shockwave animation (2324) and Grasshopper's leap (0x3C bytes). Two cars of
 * one of these types (request 32's duplicates) share it (PER_CAR_PLAN.md A.2 rows 1-5).
 *
 * With -DTM2_PORT_CAR_SPECIALS (the port's `--port-define TM2_PORT_CAR_SPECIALS`; needs
 * TM2_PORT_CAR_GROUPS) the state is per car key (PER_CAR_PLAN.md D5):
 *   - the first car of a type (key = type) keeps the retail global, at its retail address;
 *   - a duplicate (overflow key (16 + d) << 4) has an entry d of portCarSpecials (.port_bss).
 * The game code that reads and writes the state is retail's, run in the context of one car: a "selection"
 * (portCarSpecialSelect(key)) exchanges the retail global of that key's special with the key's entry for
 * the time of one call, and portCarSpecialRestore puts both back. A key below 0x100 selects the retail
 * storage itself, so with no duplicate every selection is a no-op and the game runs retail's code on
 * retail's storage in retail's order (no rand() call added or moved). The per-frame updates without a car
 * argument (weaponTornadoUpdate, weaponHomingLeapUpdate, weaponShadowStrikeUpdate) run retail's body for
 * the first car, then once per duplicate key of the type in key order (PER_CAR_PLAN.md D6).
 *
 * Request 33 step 2e (-DTM2_PORT_DARK_TOOTH, needs TM2_PORT_CAR_SPECIALS; docs/CAR_GROUPS.md 11): Dark Tooth's
 * head and boss state (the 7 controller scalars, the swoop block, the head object, the swoop vectors and the
 * stage flag aiBossDefeatedFlag) become a sixth part of the same mechanism, so every Dark Tooth has its own
 * head, timers and stage. portCarOptions bit 6 (TM2_PORT_CAR_OPT_BOSS_ANYWHERE) adds docs/SURVIVAL.md 1.5/1.6:
 * the boss weapons and stage 2 on any arena, a per-car stage loop, the Hong Kong jump/warp code only on level 8.
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss, after portCarGroupNoReuse);
 * this header declares its layout for the port variant and, with the define, the port functions. */
#ifndef TM2_SEMANTIC_CAR_SPECIALS_H
#define TM2_SEMANTIC_CAR_SPECIALS_H

#define TM2_PORT_SPECIAL_KEYS 32     /* overflow keys (car_groups.h TM2_CAR_KEYS - 16): key index 16 + d */
#define TM2_PORT_SPECIAL_PARTS 6     /* the specials with a single retail state (portCarSpecialPart; 5 = Dark
                                        Tooth, used only with TM2_PORT_DARK_TOOTH) */
#define TM2_PORT_CAR_OPT_BOSS_ANYWHERE 0x40  /* portCarOptions bit 6 (TM2_PORT_DARK_TOOTH): SURVIVAL.md 1.5/1.6 */
#define TM2_PORT_CAR_OPT_DT_ORDINARY 0x400   /* portCarOptions bit 10 (TM2_PORT_DARK_TOOTH, request 42 item 1): a
                                                computer Dark Tooth fights as his first phase only and dies as a car,
                                                unless the port marks his car slot as the boss (portCarDarkToothRole) */
#define TM2_PORT_CLAW_EXTRA 32       /* Mr. Slam claw records after weaponSlamRecords (weaponSlamStateAlloc) */

#ifdef TM2_PORT_VARIANT
/* portCarSpecials (.port_bss, config/US/port_variant_arrays.tsv group one). Entry d = key (16 + d) << 4.
 * Each entry holds the retail global's bytes for that car while it is not selected. Offsets in bytes. */
struct TM2PortCarSpecials {
    int sel[TM2_PORT_SPECIAL_PARTS];         /* +0x000 per part: the entry now in the retail storage, d + 1
                                                (0 = the first car's own state is there: no selection) */
    int pad[2];                              /* +0x018 */
    struct {                                 /* +0x020 Mr. Slam (0xC0): slamArmObject 80180F74, slamClawObject */
        void *arm, *claw;                    /*        80180F78, slamArmAngle 80180F7C, slamClawAngle 80180F80 */
        int arm_angle, claw_angle;
    } slam[TM2_PORT_SPECIAL_KEYS];
    struct {                                 /* +0x220 Twister (0xA0): weaponTornadoState 80184DC0 (0x14 bytes), */
        int state[5];                        /*        weaponTornadoObject 80180F38 (asset 2323), */
        void *tornado, *funnel;              /*        weaponTornadoFunnelObject 80180F3C (the Twister root) */
    } tornado[TM2_PORT_SPECIAL_KEYS];
    struct {                                 /* +0x5A0 Shadow (0xD0): weaponStrikeState 80184E10..80184E74 (state, */
        int block[25];                       /*        corners, owner weapons block, strike object), */
        void *anim;                          /*        objRegistryId2325 80181068 (its animation) */
    } strike[TM2_PORT_SPECIAL_KEYS];
    void *axel_anim[TM2_PORT_SPECIAL_KEYS];  /* +0x12A0 Axel (0xB0): objRegistryId2324 80181064 (shockwave) */
    int leap[TM2_PORT_SPECIAL_KEYS][15];     /* +0x1320 Grasshopper (0x60): homingLeapState 80184DD4 (0x3C) */
    short claw[TM2_PORT_CLAW_EXTRA][9];      /* +0x1AA0 Mr. Slam claw records after weaponSlamRecords (18 bytes) */
    struct {                                 /* +0x1CE0 Dark Tooth (0xF0, request 33-2e), 0x40 bytes each: */
        short scalars[7];                    /* +0x00 aiDarkToothShotDelay..aiDarkToothSwoopTarget 80180B4C (7 x s16) */
        short pad;                           /* +0x0E */
        int swoop[4];                        /* +0x10 darkToothSwoopDelay, darkToothSwooping, darkToothHoverHeight,
                                                      darkToothSwoopTarget 80180F18 (0x10 bytes) */
        void *head;                          /* +0x20 darkToothBossObject 80180F40 (the head, asset 249) */
        int vectors[6];                      /* +0x24 darkToothSwoopVectors 80183F64 */
        int stage;                           /* +0x3C aiBossDefeatedFlag 8018102C (1 = stage 2) */
    } dark_tooth[TM2_PORT_SPECIAL_KEYS];
};
typedef char tm2_port_car_specials_size[(sizeof(struct TM2PortCarSpecials) == 0x24E0) ? 1 : -1];
#endif

#ifdef TM2_PORT_CAR_SPECIALS
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_CAR_SPECIALS needs TM2_PORT_CAR_GROUPS (the special state is per car key)"
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c) */
extern int  portCarSpecialSelect(int key);       /* the key's state into the retail storage; returns the token
                                                    for portCarSpecialRestore (0 = nothing to restore) */
extern void portCarSpecialRestore(int token);    /* undo the selection (selections nest) */
extern int  portCarSpecialNext(int type, int *index); /* select the next duplicate key of `type` from entry
                                                    *index on (token), 0 when there is none */
extern void portCarSpecialRegister(int id, void *object); /* objRegistryAdd ids 2324 / 2325 by load key */
extern int  portCarKeyOfStats(void *stats);      /* the car key of the record holding this stats block, 0 */
extern void *portCarClawRecord(int index);       /* claw record `index` of weaponSlamStateAlloc's pool */
extern void portCarSpecialsReset(void);          /* portCarGroupsReset: every duplicate's state cleared */
extern void portCarSpecialsRoundReset(void);     /* weaponsRoundReset: the duplicates' part of it */
#ifdef __cplusplus
}
#endif
#endif

/* Request 33 step 2e: Dark Tooth per car (docs/CAR_GROUPS.md 11). */
#ifdef TM2_PORT_DARK_TOOTH
#ifndef TM2_PORT_CAR_SPECIALS
#error "TM2_PORT_DARK_TOOTH needs TM2_PORT_CAR_SPECIALS (Dark Tooth's state is a part of the per-car specials)"
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern int gameGetLevel(void);
/* .port_text (port_variant_arrays.c) */
extern int  portCarBossDefeated(int key);        /* aiBossDefeated() of the car with this key (its stage flag) */
extern int  portCarDarkToothWreck(void *ai);     /* carApplyDamage, inside the dying Dark Tooth's selection: its
                                                    wreck state (3 retail; 4 after a final death with bit 6) */
extern int  portCarDarkToothStages(void);        /* bit 6: the per-car stage loop (SURVIVAL.md 1.6); returns the
                                                    Dark Tooths that wait for their stage 2 */
extern int  portCarDarkToothRoundEnd(void);      /* worldUpdate's round end: the stage flag of aiCars[0]'s car
                                                    (its head hidden when set), as retail */
extern void portCarDarkToothClearStages(void);   /* arenaSetup: every duplicate's stage flag cleared */
extern int  portCarDarkToothShooter(int owner);  /* a type-61 missile's owner id is a Dark Tooth's car object */
extern int  portCarDarkToothStage(int slot, int stage); /* the port's API: the stage flag of the Dark Tooth in
                                                    car slot `slot` (stage < 0: read only); -1 if not one */
/* Request 42 item 1 (portCarOptions bit 10, DARK_TOOTH_PLAYER.md 10.1): the ordinary Dark Tooth. */
extern int  portCarDarkToothOrdinary(int slot);  /* 1 = car slot `slot` holds a computer Dark Tooth that fights as an
                                                    ordinary car: bit 10 set and the slot not marked as the boss;
                                                    0 always with bit 10 clear (retail / bit 6 logic) */
extern int  portCarDarkToothRole(int slot, int boss); /* the port's API, between frames: boss 1 = the Dark Tooth of
                                                    car slot `slot` keeps the boss parts (bit 6, retail's level 8),
                                                    0 = ordinary (with bit 10); boss < 0: read only. Returns the
                                                    role before (1 boss, 0 ordinary), -1 for a slot past 63 */
extern void portCarDarkToothHeadShow(void *ai);  /* aiWeaponFireDecide, inside the Dark Tooth's selection: an
                                                    ordinary one's head shown while his car object is */
/* Request 46 (PLAYTEST_ANSWERS.md 7): a Dark Tooth nobody drives has his head hidden (not drawn, out of the hit pass);
   shown again over his truck when a computer record drives the car. portCarDtParkMask (port_variant_arrays.c) holds
   the heads hidden this way, per car slot. */
extern void portCarDarkToothParkHead(int slot);  /* the park paths: the head of car slot `slot` hidden, remembered */
extern void portCarDarkToothParked(void);        /* aiCarsCountRemaining, each frame: heads of undriven cars hidden,
                                                    a remembered one shown again when its car is driven */
extern unsigned int portCarDtParkMask[2];
#ifdef __cplusplus
}
#endif
#define TM2_PORT_BOSS_ANYWHERE ((portCarOptions & TM2_PORT_CAR_OPT_BOSS_ANYWHERE) != 0)
/* request 42 item 1: the ordinary Dark Tooth test of the game functions (0 with bit 10 clear: the function tests the
   bit), and bit 10 itself (the missile wobble) */
#define TM2_DT_ORDINARY(slot) portCarDarkToothOrdinary(slot)
#define TM2_PORT_DT_ORDINARY_ON ((portCarOptions & TM2_PORT_CAR_OPT_DT_ORDINARY) != 0)
#endif
/* Request 42 item 1 for retail's own code (roundStart's Hong Kong boss route patch, carDestroyed's boss wreck timer):
   1 without TM2_PORT_DARK_TOOTH, so the expression is retail's. */
#ifdef TM2_PORT_DARK_TOOTH
#define TM2_DT_BOSS_SLOT(slot) (!portCarDarkToothOrdinary(slot))
#else
#define TM2_DT_BOSS_SLOT(slot) 1
#endif

/* Request 35: any car fires another car's special (Gun Game; docs/GUN_GAME_SPECIALS.md "As built").
 *
 * With -DTM2_PORT_SPECIAL_AS (needs TM2_PORT_CAR_SPECIALS) the car in slot s fires the special of car type
 * portCarSpecialAs[s] (0 or -1 = its own; the load sets every slot to -1, so all -1 = retail). The port sets it
 * between frames with portCarSpecialAsSet(slot, type, ammo) only. The borrowable types (TM2_PORT_SPECIAL_AS_MASK,
 * bit type >> 4) are Sweet Tooth, Outlaw 2, Warthog, Mr. Grimm, Thumper, Spectre, Roadkill, Axel, Shadow and
 * Minion; Hammerhead, Grasshopper, Twister, Mr. Slam and Dark Tooth are refused. Shadow's strike and Axel's
 * shockwave have a single retail state: a borrowing car gets its own copy (an "as-entry" per car slot), a part
 * of the 33-2a selection mechanism (selection values 33..96 = the as-entry of slot 0..63), with its own strike
 * object and shockwave ring object (made by portCarSpecialAsSet, drawing the loaded Shadow's / Axel's nodes).
 * portCarSpecialLoad (bit type >> 4) asks the next load (with TM2_PORT_CARS and portCarOptions bit 1) to load
 * the car file of a type that no car of the match has, as a parked spare group: Sweet Tooth, Mr. Grimm,
 * Spectre, Roadkill, Axel and Shadow (TM2_PORT_SPECIAL_LOAD_FILES) need it; the others use the level's assets. */
#define TM2_PORT_SPECIAL_AS_MASK 0x6BBA      /* 0x10 0x30 0x40 0x50 0x70 0x80 0x90 0xB0 0xD0 0xE0 */
#define TM2_PORT_SPECIAL_LOAD_FILES 0x2B22   /* 0x10 0x50 0x80 0x90 0xB0 0xD0: their projectile / node is in the car file */
#define TM2_PORT_SPECIAL_AS_LOADS 16         /* spare groups a load may add */
/* portCarSpecialAsSet results */
#define TM2_SPECIAL_AS_OK 1
#define TM2_SPECIAL_AS_ERR_SLOT (-1)         /* no such slot, or no record drives it */
#define TM2_SPECIAL_AS_ERR_TYPE (-2)         /* a refused or unknown type */
#define TM2_SPECIAL_AS_ERR_NOT_LOADED (-3)   /* the type's car file is not loaded in this match */
#define TM2_SPECIAL_AS_ERR_ROOM (-4)         /* no coordinate system left for the strike / ring object */

#ifdef TM2_PORT_VARIANT
/* portSpecialAs (.port_bss, config/US/port_variant_arrays.tsv group one): the as-entries, entry s = car slot s,
 * same layouts as portCarSpecials' strike and Axel parts. Offsets in bytes. */
struct TM2PortSpecialAs {
    struct {                                 /* +0x0000 Shadow (0xD0): weaponStrikeState 80184E10 (0x64 bytes: */
        int block[25];                       /*         state, corners, owner weapons block, the strike object), */
        void *anim;                          /*         objRegistryId2325 80181068 (the loaded Shadow's node) */
    } strike[64];
    struct {                                 /* +0x1A00 Axel (0xB0): objRegistryId2324 80181064 (the loaded */
        void *anim;                          /*         Axel's shockwave node) and the slot's ring object (its */
        void *ring;                          /*         model = that node), not part of the exchange */
    } axel[64];
    int load_n;                              /* +0x1C00 spare groups of this load (portCarSpecialLoadPlan) */
    int load_first;                          /* +0x1C04 the first spare group */
    signed char load_car[TM2_PORT_SPECIAL_AS_LOADS];  /* +0x1C08 their car-select indices */
};
typedef char tm2_port_special_as_size[(sizeof(struct TM2PortSpecialAs) == 0x1C18) ? 1 : -1];
#endif

#ifdef TM2_PORT_SPECIAL_AS
#ifndef TM2_PORT_CAR_SPECIALS
#error "TM2_PORT_SPECIAL_AS needs TM2_PORT_CAR_SPECIALS (the borrowed strike and shockwave are as-entries of the per-car specials)"
#endif
#pragma tm2_absolute portCarSpecialAs
#pragma tm2_absolute portCarSpecialLoad
#pragma tm2_absolute portSpecialAs
#ifdef __cplusplus
extern "C" {
#endif
extern short portCarSpecialAs[];     /* per car slot (64): the type whose special the car fires; 0 / -1 = its own */
extern int portCarSpecialLoad;       /* bit type >> 4: load that type's car file at the next load (spare group) */
/* .port_text (port_variant_arrays.c) */
extern int  portCarSpecialAsSet(int slot, int type, int ammo);  /* the port's API, between frames: car slot `slot`
                                                    fires `type`'s special (-1 = its own) with `ammo` specials
                                                    (-1: keep, capped); ends the old special. TM2_SPECIAL_AS_* */
extern int  portCarSpecialType(int slot, int type);  /* the type whose special the car fires (`type` = its own) */
extern int  portCarSpecialSelectFire(int slot, int *vehicle);  /* weaponFireSpecial: the firer's selection (its
                                                    as-entry, or its key's state); *vehicle = the type it fires */
extern int  portCarSpecialSelectAs(int slot, int type); /* select slot's as-entry of type's part (token, 0 none) */
extern int  portCarSpecialAsNext(int type, int *slot);  /* the per-frame loops: the next slot borrowing `type`,
                                                    selected (token), 0 when there is none */
extern void portCarSpecialAsRing(int slot, void *firer); /* weaponFireSpecial 0xB0: the slot's ring at the firer */
extern void portCarSpecialAsEnd(int slot);        /* end the slot's as-entry (strike, ring) */
extern void portCarSpecialAsBlockInit(void *car, void *weapons, int human); /* weaponBlockInit: a borrowed
                                                    special's max_specials and recharge_period */
extern void portCarSpecialAsReset(void);          /* each load: every slot -1, the as-entries and the car
                                                    projectile models cleared */
extern int  portCarSpecialLoadPlan(void *settings); /* sessionLoadArenaAndCars: the spare groups of this load */
extern int  portCarSpecialLoadCar(int i);         /* spare i's car-select index, -1 after the last */
extern void portCarSpecialLoadDone(void);         /* after the spares' DMDs: their slots parked */
#ifdef __cplusplus
}
#endif
#define TM2_SPECIAL_TYPE(slot, type) portCarSpecialType((slot), (type))
#else
#define TM2_SPECIAL_TYPE(slot, type) (type)
#endif

#endif
