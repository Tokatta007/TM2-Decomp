/* Car groups for the PC port (request 32 step 1, docs/CAR_GROUPS.md, docs/CAR_GROUPS_PLAN.md 1).
 *
 * Two or more cars of one type in a match. Retail keeps the per-car looks (body, wreck,
 * damage stage, tires, turbo flame, spinning parts, freeze record) in tables indexed by the
 * car type (`id >> 4`, docs/SURVIVAL.md 2.2), so a second car of a type overwrites the first
 * one's entries, and the binders bind both cars to the first car's slot (2.4).
 *
 * With -DTM2_PORT_CAR_GROUPS (the port's `--port-define TM2_PORT_CAR_GROUPS` on a port variant
 * build) every reader and writer of those tables uses portCarGroups instead, indexed by a
 * "car key" with the layout of the retail value (`key >> 4` is the table index):
 *   - the first car of each type in the load order gets key = type (0x10..0xF0);
 *   - every further car of a type already loaded gets an overflow key (16 + d) << 4, d = 0, 1, ...;
 *   - the roots of one DMD (Minion's two 224 roots) share their load group's key.
 * With no duplicate type in the match every key equals the type, so every index, object and
 * stored value (crush_owner, spin_target) is the retail one.
 *
 * Without the define this header only declares the storage type (port variant) and
 * TM2_CAR_KEY(slot, type) expands to the retail expression `type`. */
#ifndef TM2_SEMANTIC_CAR_GROUPS_H
#define TM2_SEMANTIC_CAR_GROUPS_H

#define TM2_CAR_KEYS 48          /* key index 0 unused, 1..15 first car of each type, 16..47 duplicates (up to
                                    32 loaded cars, request 32 Q14: a type loaded 32 times has 31 duplicates) */
#define TM2_CAR_SLOT_CAP 64      /* car slots (carWorldObjectCount) the registries can hold: 32 loaded cars
                                    (players.h TM2_PORT_CARS_LOADED) with two roots each (Minions); also load groups */
#if defined(TM2_MAX_CAR_SLOTS) && TM2_MAX_CAR_SLOTS > TM2_CAR_SLOT_CAP
#error "TM2_MAX_CAR_SLOTS (players.h) is larger than the car-group registries (TM2_CAR_SLOT_CAP)"
#endif

#ifdef TM2_PORT_VARIANT
/* port_variant_arrays.c, .port_bss, config/US/port_variant_arrays.tsv (group one). The comments
 * name the retail table each field replaces under the define. */
struct TM2PortCarGroups {
    int  slot_group[TM2_CAR_SLOT_CAP];    /* load group (vramTexSlotGroup) of each car slot */
    int  slot_key[TM2_CAR_SLOT_CAP];      /* key of each car slot (0 = not registered) */
    int  group_key[TM2_CAR_SLOT_CAP];     /* key of each load group (0 = none yet) */
    int  key_type[TM2_CAR_KEYS];          /* car type of each key index */
    int  dup_count;                       /* overflow keys handed out in this load */
    int  pad;
    void *body[TM2_CAR_KEYS];             /* carBodyObjects (801873D0) */
    void *wreck[TM2_CAR_KEYS];            /* carWreckModels (8018740C) */
    void *damage_stage[TM2_CAR_KEYS];     /* carDamageStageObjects (8018F4E4) */
    void *tire[TM2_CAR_KEYS][3];          /* carTireSwitchObjects (8018F524) */
    int  tire_accum[TM2_CAR_KEYS];        /* carPartsTireTurnAccum (80187344); never cleared, as retail */
    void *turbo[TM2_CAR_KEYS];            /* carTurboObjects (8018F5E4) */
    void *parts[TM2_CAR_KEYS][6];         /* carPartsHammerhead/Minion/MrSlam/DarkTooth, carAxelObjects */
    int  part_angle[TM2_CAR_KEYS];        /* carPartsSpinAngle* (80180B78..80180B88); never cleared, as retail */
    unsigned char freeze[TM2_CAR_KEYS][264]; /* carFreezeObjects (8018F950), 264-byte choice records */
};
typedef char tm2_port_car_groups_size[(sizeof(struct TM2PortCarGroups) == 0x4088) ? 1 : -1];
#endif

/* Request 32 step 3 (CAR_GROUPS.md 8): the car swap. portCarBindGroup (.port_bss, every variant) holds one
 * load group per record, 0 = the settings' group: human i at [i], AI j at [TM2_PORT_BIND_HUMANS + j]. */
#define TM2_PORT_BIND_HUMANS 16
#define TM2_PORT_BIND_ENTRIES 48
#ifdef TM2_PORT_CAR_SWAP
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_CAR_SWAP needs TM2_PORT_CAR_GROUPS (the swap re-binds per-car keys)"
#endif
#pragma tm2_absolute portCarBindGroup
extern signed char portCarBindGroup[TM2_PORT_BIND_ENTRIES];
/* The functions are in .port_text (port_variant_arrays.c). kind: 0 human, 1 AI. */
extern int  portCarSetGroup(int human, int index, int group, int hp); /* the swap: record -> load group's car */
extern int  portCarParkAi(int index);                    /* SURVIVAL.md 3.1: AI record j off (dead, hidden, far) */
extern int  portCarBindGroupOf(int ai, int index, int group); /* sessionBindCars: the record's group */
extern int  portCarBindTypeOf(int ai, int index, int type);   /* sessionBindCars: the type of that group */
/* shared blocks (request 33's remove / start mid-match, PER_CAR_PLAN.md B.6, reuse them) */
extern void portCarResetSpecials(int slot);              /* end every special state that holds the slot's car */
extern void portCarResetLook(int slot);                  /* freeze, fire, turbo, invisibility, engine sound */
extern void portCarUnbind(int kind, int index);          /* the record leaves its slot (slot kind 2) */
extern void portCarBindSlot(int kind, int index, int slot);   /* raw rebind, no restart */
extern int  portCarApplyType(int kind, int index, int hp);   /* stats, weapons, camera, AI of the slot's type */
extern void portCarParkSlot(int slot);                   /* a car without a driver: hidden, far away, kind 2 */
extern void portCarRekeyReferences(int slot_a, int slot_b);  /* credits and targets: a <-> b (b < 0: clear a) */
#endif

#ifdef TM2_PORT_CAR_GROUPS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_CAR_GROUPS needs the port variant (TM2_PORT_VARIANT)"
#endif
#pragma tm2_absolute portCarGroups
#pragma tm2_absolute portCarOptions
extern struct TM2PortCarGroups portCarGroups;
extern int portCarOptions;               /* bit 0: the car pickers allow repeated cars; bits 1-3: port_cars.h; 0 = retail */

/* The functions are in .port_text (port_variant_arrays.c). */
extern int  portCarKey(int slot, int type);         /* the slot's key; `type` if the slot has none */
extern int  portCarKeyType(int key);                /* the car type of a key */
extern int  portCarLoadKey(void);                   /* key of load group vramTexSlotGroup (0 before its root) */
extern int  portCarKeyOfObject(void *object);       /* key of the car slot holding this world object, else 0 */
extern int  portCarSlotOfGroup(int group);          /* first car slot of a load group, -1 if none */
extern void portCarRegisterRoot(int slot, int type);/* carRegisterModel: group and key of a new slot */
extern void portCarGroupsReset(void);               /* carTablesClear: per-load reset */
extern void portCarTurboSetStep(int key, int value);/* animSetStep on the key's turbo object */
extern void portCarPartsSpin(int key, int amount);  /* the carPartsSpin* body for the key's parts */
#define TM2_CAR_KEY(slot, type) portCarKey((slot), (type))
#else
#define TM2_CAR_KEY(slot, type) (type)
#endif

#endif
