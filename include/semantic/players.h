/* Player-count limits (PC port request 25; docs/SOURCE_BUILD.md "Port variant",
 * docs/FOUR_HUMANS.md).
 *
 * Retail is built for two human records, two views and two HUDs. These
 * constants name those limits. The default build keeps them at 2 and is
 * byte-identical to retail's layout (every use expands to the retail C).
 *
 * The port variant (`build.py --source-only --relocatable --port-variant`,
 * which compiles every unit with -DTM2_PORT_VARIANT) raises them to 4:
 *   - every array sized by them is defined in src/semantic/port_variant_arrays.c,
 *     in the sections .port_bss (zero) and .port_data (initialised), which the
 *     relocatable link places at the port base (default 0x80200000, above the
 *     PS1's 2 MB; NOLOAD: crt0 does not clear it, the port does);
 *   - the units that defined the retail storage drop those definitions;
 *   - the loops and "other player" tests of FOUR_HUMANS.md section 2 run for
 *     N players under `#if TM2_MAX_HUMANS > 2` (the #else is the retail C);
 *   - the moved arrays are addressed absolutely (`#pragma tm2_absolute`
 *     below): $gp reaches only 801789D4..801889D3.
 * TM2_MAX_HUMANS / TM2_MAX_VIEWS / TM2_MAX_HUDS may be predefined (the static
 * check of the variant builds it with all three forced to 2: only the
 * placement changes). */
#ifndef TM2_SEMANTIC_PLAYERS_H
#define TM2_SEMANTIC_PLAYERS_H

/* What retail's arrays hold (the retail storage of a moved array is this many
 * entries; settings human_car[] has this many slots). */
#define TM2_RETAIL_HUMANS 2
#define TM2_RETAIL_VIEWS 2
#define TM2_RETAIL_HUDS 2
/* The human records the port variant had before step H4 of docs/HUMANS16_PLAN.md: retail's 4 bytes of
 * carResetWeapons (u8[2] + 2 bytes of padding, 80180FA0) and the settings record's lives[4] hold that many.
 * With more (TM2_PORT_HUMANS 5..16) carResetWeapons moves to port storage (tsv group humans>4) and
 * settingsCopy copies only the record's own entries. */
#define TM2_VARIANT_HUMANS 4
/* Rows of the view-mode table viewModeTable (8017EC44, 96 bytes each; PC port request 26). */
#define TM2_RETAIL_VIEW_MODES 9

#ifdef TM2_PORT_VARIANT
#ifndef TM2_MAX_HUMANS
#ifdef TM2_PORT_HUMANS
#define TM2_MAX_HUMANS TM2_PORT_HUMANS   /* HUMANS16_PLAN.md (step H0: 4 until H4) */
#else
#define TM2_MAX_HUMANS 4
#endif
#endif
#ifndef TM2_MAX_VIEWS
#define TM2_MAX_VIEWS 4
#endif
#ifndef TM2_MAX_HUDS
#define TM2_MAX_HUDS 4
#endif
/* The port adds view modes 9-12 (the four quarters) and 13 (the split-screen
 * mirror); rows 9-15 start zeroed and the port fills them. */
#ifndef TM2_MAX_VIEW_MODES
#define TM2_MAX_VIEW_MODES 16
#endif
#else
#define TM2_MAX_HUMANS TM2_RETAIL_HUMANS
#define TM2_MAX_VIEWS TM2_RETAIL_VIEWS
#define TM2_MAX_HUDS TM2_RETAIL_HUDS
#define TM2_MAX_VIEW_MODES TM2_RETAIL_VIEW_MODES
#endif

/* ---- Cars and the pools they fill (PC port request 32 step 2, docs/CAR_GROUPS.md "Step 2",
 * docs/SIXTEEN_CARS.md 1.6-1.7). ----
 * What retail's storage and code hold: */
#define TM2_RETAIL_AI_CARS 9            /* aiCars, aiCarObjects, aiCarObjectSlots, aiCarTargets */
#define TM2_RETAIL_LOADED_CARS 11       /* car DMDs (load groups 1..11) a match loads: 2 humans + 9 opponents */
#define TM2_RETAIL_CAR_SLOTS 15         /* carRegisterModel slots (cars + Minions' second roots), pair tables [15][15] */
#define TM2_RETAIL_OPPONENT_SLOTS 9     /* settings opponent_car[] slots the game uses (slot 9 of the record is unused) */
#define TM2_RETAIL_OPPONENT_CARDS 9     /* the opponent-card screen: 9 cards + the template (screenCardQuads) */
#define TM2_RETAIL_RADAR_BLIPS 12       /* blips per radar (hudRadarPlotCar: count + 2 >= 2 * 12 + 1) */
#define TM2_RETAIL_PROJECTILES 16       /* fxRecords (projectile records, 980 bytes each) */
#define TM2_RETAIL_COORD_SYSTEMS 128    /* csObjectPool (csAlloc) */
#define TM2_RETAIL_SOUND_INSTANCES 35   /* soundInstPool (soundInstAlloc) and soundDriverUpdate's sort list */
#define TM2_RETAIL_SOUND_REQUESTS 35    /* sndObjPool: sound requests (sndPoolAlloc; the objects on main's stack) */
#define TM2_RETAIL_ARCS 5           /* weaponArcs: taser and lightning arcs (weaponArcSlotFind) */
#define TM2_RETAIL_SHIELDS 4        /* weaponShieldPool: energy shields (weaponShieldSlotFind; objects + records) */
#define TM2_RETAIL_REMOTE_BOMBS 2   /* weaponRemoteBombObjects (weaponBombObjectFind) */
#define TM2_RETAIL_RICOCHETS 10     /* weaponRicochetPool: ricochet shots (weaponRicochetSlotFind; objects + shots) */
#define TM2_RETAIL_FLAME_EMITTERS 2 /* weaponFlameEmitters: Thumper flames (weaponFlameSlotFind) */
#define TM2_RETAIL_SLAM_RECORDS 4   /* weaponSlamRecords: Mr. Slam claw records (weaponSlamStateAlloc; retail hands out 5 into 4, the 5th over weaponRicochetPool) */
#define TM2_RETAIL_WORLD_MODELS 20  /* renderWorldModels: models drawn every frame (car bodies, level models) */
#define TM2_RETAIL_CLUT_ANIM_DEFS 20 /* clutAnimDefs: CLUT animation definitions (clutAnimNewDef) */
#define TM2_RETAIL_CLUT_ANIMS 10    /* clutAnimRunning: running CLUT animations */
#define TM2_RETAIL_DROPS 25         /* weaponDrops: mine and remote-bomb records, 28 bytes (weaponDropSlotFind) */
#define TM2_RETAIL_MINE_OBJECTS 23  /* weaponMineObjects: mine scene objects (weaponMineObjectFind; asset 0x8A4) */
#define TM2_RETAIL_FX_SPRITES 150   /* fxSprites: effect sprites, 48 bytes (fxSpriteFindFree; explosions, smoke) */

/* -DTM2_PORT_CARS=N (11..16, the port's --port-define; needs the port variant): up to N ACTIVE cars
 * in a match, any mix of humans and computer cars (16 humans later, request 33, or 1 human + 15
 * computer cars), plus loaded cars that do not drive (the Juggernaut's parked Minion, spares for
 * players who join, request 32 Q8/Q14). Records and loaded cars are separate limits: "humans +
 * opponents <= N active"
 * is a run-time rule the port keeps where it builds the settings, not an array size. Each limit
 * below covers the worst case of its own kind, none is derived from the human count, and each stays
 * a macro (every one may be predefined; --port-limits 2 forces the retail values):
 *   TM2_PORT_CARS_LOADED    32       car DMDs / load groups a match may load (16 active + the
 *                                    Juggernaut's Minion + spares + joins)
 *   TM2_MAX_LOADED_CARS     = TM2_PORT_CARS_LOADED (without TM2_PORT_CARS: humans + AI records)
 *   TM2_MAX_AI_CARS         N        computer-car records: 1 human + N - 1 computer cars + the parked
 *                                    Minion (each record limit reaches N on its own)
 *   TM2_MAX_CAR_SLOTS       2 x 32   every loaded car + a second root per Minion (CAR_GROUPS_PLAN.md D2)
 *   TM2_TEX_SLOT_STRIDE     32       texture slots per load group (retail 20; an image index is 0..31)
 *   TM2_MAX_RADAR_BLIPS     N - 1    every other active car on every radar
 *   TM2_MAX_PROJECTILES     3 N      48 for 16 cars (each takes two coordinate systems)
 *   TM2_MAX_COORD_SYSTEMS   16 N     256 for 16 cars (20 N = 320 with TM2_PORT_BALANCE: the 5 N mine objects)
 *   TM2_MAX_SOUND_INSTANCES 4 N      64 for 16 cars (the SPU still has 24 voices: the driver keeps the
 *                                    24 of highest priority, as retail)
 *   TM2_MAX_SOUND_REQUESTS  4 N      64 for 16 cars: the sound request pool (sndObjPool; a request per playing
 *                                    sound: 16 engines alone took 16 of retail's 35; request 33 step 4)
 *   the weapon and render pools (request 32 Q9, "about one per car"): TM2_MAX_ARCS N, TM2_MAX_SHIELDS N,
 *   TM2_MAX_REMOTE_BOMBS N, TM2_MAX_RICOCHETS 2 N, TM2_MAX_FLAME_EMITTERS N, TM2_MAX_SLAM_RECORDS N + 1
 *   (the claw allocator hands out one more than its wrap point), TM2_MAX_WORLD_MODELS loaded + 8,
 *   TM2_MAX_CLUT_ANIM_DEFS 40, TM2_MAX_CLUT_ANIMS 20, TM2_MAX_FX_SPRITES 25 N (400 for 16 cars; request 102 item 3:
 *   15 opponents filled retail's 150, so an explosion was skipped); with TM2_PORT_BALANCE (request 40, docs/BALANCE.md 3.3)
 *   also TM2_MAX_DROPS 6 N and TM2_MAX_MINE_OBJECTS 5 N (retail 25 / 23). Unlike the arrays above these move to port
 *   storage only when raised (config/US/port_variant_arrays.tsv groups "name+"), so a variant without
 *   TM2_PORT_CARS keeps them at their retail addresses.
 * The human records stay TM2_MAX_HUMANS (4 in the variant, up to 16 with TM2_PORT_HUMANS below, HUMANS16_PLAN.md). */
#if defined(TM2_PORT_CARS) && !defined(TM2_PORT_VARIANT)
#error "TM2_PORT_CARS needs the port variant (TM2_PORT_VARIANT)"
#endif
#ifdef TM2_PORT_CARS
#if TM2_PORT_CARS < 11 || TM2_PORT_CARS > 16
#error "TM2_PORT_CARS must be 11..16"
#endif
#ifndef TM2_PORT_CARS_LOADED
#define TM2_PORT_CARS_LOADED 32   /* request 32 Q14: 16 active + the Juggernaut's Minion + spares + joins */
#endif
#ifndef TM2_MAX_LOADED_CARS
#define TM2_MAX_LOADED_CARS TM2_PORT_CARS_LOADED
#endif
#ifndef TM2_MAX_AI_CARS
#define TM2_MAX_AI_CARS TM2_PORT_CARS
#endif
#ifndef TM2_MAX_CAR_SLOTS
#define TM2_MAX_CAR_SLOTS (2 * TM2_PORT_CARS_LOADED)
#endif
#ifndef TM2_MAX_RADAR_BLIPS
#define TM2_MAX_RADAR_BLIPS (TM2_PORT_CARS - 1)
#endif
#ifndef TM2_MAX_PROJECTILES
#define TM2_MAX_PROJECTILES (3 * TM2_PORT_CARS)
#endif
#ifndef TM2_MAX_COORD_SYSTEMS
#ifdef TM2_PORT_BALANCE
#define TM2_MAX_COORD_SYSTEMS (20 * TM2_PORT_CARS)   /* request 40 (BALANCE.md 3.3): + 4 N for the 5 N mine objects */
#else
#define TM2_MAX_COORD_SYSTEMS (16 * TM2_PORT_CARS)
#endif
#endif
#ifndef TM2_MAX_SOUND_INSTANCES
#define TM2_MAX_SOUND_INSTANCES (4 * TM2_PORT_CARS)
#endif
#ifndef TM2_MAX_SOUND_REQUESTS
#define TM2_MAX_SOUND_REQUESTS (4 * TM2_PORT_CARS)
#endif
#ifndef TM2_MAX_ARCS
#define TM2_MAX_ARCS TM2_PORT_CARS
#endif
#ifndef TM2_MAX_SHIELDS
#define TM2_MAX_SHIELDS TM2_PORT_CARS
#endif
#ifndef TM2_MAX_REMOTE_BOMBS
#define TM2_MAX_REMOTE_BOMBS TM2_PORT_CARS
#endif
#ifndef TM2_MAX_RICOCHETS
#define TM2_MAX_RICOCHETS (2 * TM2_PORT_CARS)
#endif
#ifndef TM2_MAX_FLAME_EMITTERS
#define TM2_MAX_FLAME_EMITTERS TM2_PORT_CARS
#endif
#ifndef TM2_MAX_SLAM_RECORDS
#define TM2_MAX_SLAM_RECORDS (TM2_PORT_CARS + 1)
#endif
#ifndef TM2_MAX_WORLD_MODELS
#define TM2_MAX_WORLD_MODELS (TM2_PORT_CARS_LOADED + 8)
#endif
#ifndef TM2_MAX_CLUT_ANIM_DEFS
#define TM2_MAX_CLUT_ANIM_DEFS 40
#endif
#ifndef TM2_MAX_CLUT_ANIMS
#define TM2_MAX_CLUT_ANIMS 20
#endif
#ifndef TM2_MAX_FX_SPRITES
#define TM2_MAX_FX_SPRITES (25 * TM2_PORT_CARS)
#endif
#ifndef TM2_TEX_SLOT_STRIDE
#define TM2_TEX_SLOT_STRIDE 32
#endif
#ifdef TM2_PORT_BALANCE
/* request 40 (docs/BALANCE.md 3.3, portBalance bit 2): 5 permanent mines per driver: 6 N drop records (mines in
 * [0, 5 N), remote bombs in the top N) and 5 N mine objects (80 / 96 at 16 cars) */
#ifndef TM2_MAX_DROPS
#define TM2_MAX_DROPS (6 * TM2_PORT_CARS)
#endif
#ifndef TM2_MAX_MINE_OBJECTS
#define TM2_MAX_MINE_OBJECTS (5 * TM2_PORT_CARS)
#endif
#endif
#endif
#ifndef TM2_MAX_AI_CARS
#define TM2_MAX_AI_CARS TM2_RETAIL_AI_CARS
#endif
#ifndef TM2_MAX_CAR_SLOTS
#define TM2_MAX_CAR_SLOTS TM2_RETAIL_CAR_SLOTS
#endif
#ifndef TM2_MAX_RADAR_BLIPS
#define TM2_MAX_RADAR_BLIPS TM2_RETAIL_RADAR_BLIPS
#endif
#ifndef TM2_MAX_PROJECTILES
#define TM2_MAX_PROJECTILES TM2_RETAIL_PROJECTILES
#endif
#ifndef TM2_MAX_COORD_SYSTEMS
#define TM2_MAX_COORD_SYSTEMS TM2_RETAIL_COORD_SYSTEMS
#endif
#ifndef TM2_MAX_SOUND_INSTANCES
#define TM2_MAX_SOUND_INSTANCES TM2_RETAIL_SOUND_INSTANCES
#endif
#ifndef TM2_MAX_SOUND_REQUESTS
#define TM2_MAX_SOUND_REQUESTS TM2_RETAIL_SOUND_REQUESTS
#endif
#ifndef TM2_MAX_ARCS
#define TM2_MAX_ARCS TM2_RETAIL_ARCS
#endif
#ifndef TM2_MAX_SHIELDS
#define TM2_MAX_SHIELDS TM2_RETAIL_SHIELDS
#endif
#ifndef TM2_MAX_REMOTE_BOMBS
#define TM2_MAX_REMOTE_BOMBS TM2_RETAIL_REMOTE_BOMBS
#endif
#ifndef TM2_MAX_RICOCHETS
#define TM2_MAX_RICOCHETS TM2_RETAIL_RICOCHETS
#endif
#ifndef TM2_MAX_FLAME_EMITTERS
#define TM2_MAX_FLAME_EMITTERS TM2_RETAIL_FLAME_EMITTERS
#endif
#ifndef TM2_MAX_SLAM_RECORDS
#define TM2_MAX_SLAM_RECORDS TM2_RETAIL_SLAM_RECORDS
#endif
#ifndef TM2_MAX_WORLD_MODELS
#define TM2_MAX_WORLD_MODELS TM2_RETAIL_WORLD_MODELS
#endif
#ifndef TM2_MAX_CLUT_ANIM_DEFS
#define TM2_MAX_CLUT_ANIM_DEFS TM2_RETAIL_CLUT_ANIM_DEFS
#endif
#ifndef TM2_MAX_CLUT_ANIMS
#define TM2_MAX_CLUT_ANIMS TM2_RETAIL_CLUT_ANIMS
#endif
#ifndef TM2_MAX_DROPS
#define TM2_MAX_DROPS TM2_RETAIL_DROPS
#endif
#ifndef TM2_MAX_MINE_OBJECTS
#define TM2_MAX_MINE_OBJECTS TM2_RETAIL_MINE_OBJECTS
#endif
#ifndef TM2_MAX_FX_SPRITES
#define TM2_MAX_FX_SPRITES TM2_RETAIL_FX_SPRITES
#endif
#if TM2_MAX_ARCS < TM2_RETAIL_ARCS || \
    TM2_MAX_SHIELDS < TM2_RETAIL_SHIELDS || \
    TM2_MAX_REMOTE_BOMBS < TM2_RETAIL_REMOTE_BOMBS || \
    TM2_MAX_RICOCHETS < TM2_RETAIL_RICOCHETS || \
    TM2_MAX_FLAME_EMITTERS < TM2_RETAIL_FLAME_EMITTERS || \
    TM2_MAX_SLAM_RECORDS < TM2_RETAIL_SLAM_RECORDS || \
    TM2_MAX_WORLD_MODELS < TM2_RETAIL_WORLD_MODELS || \
    TM2_MAX_CLUT_ANIM_DEFS < TM2_RETAIL_CLUT_ANIM_DEFS || \
    TM2_MAX_CLUT_ANIMS < TM2_RETAIL_CLUT_ANIMS || \
    TM2_MAX_DROPS < TM2_RETAIL_DROPS || \
    TM2_MAX_MINE_OBJECTS < TM2_RETAIL_MINE_OBJECTS || \
    TM2_MAX_FX_SPRITES < TM2_RETAIL_FX_SPRITES
#error "a weapon or render pool below retail's"
#endif
#ifndef TM2_MAX_LOADED_CARS
#define TM2_MAX_LOADED_CARS (TM2_MAX_HUMANS + TM2_MAX_AI_CARS)   /* 11 retail, 13 in the variant */
#endif
/* Texture slots per load group (vramTexSlots, group * stride + image; SURVIVAL.md 4.2): retail's 20 lets a
 * group with more than 20 images (Dark Tooth's 31, a mod car) overwrite the next group's; 32 holds every
 * image index (0..31). The table (packet buffer 0 + 0x10004) holds 1,599 slots: 49 groups at 32. */
#if defined(TM2_PORT_DT_PLAYER) && !defined(TM2_TEX_SLOT_STRIDE)
#define TM2_TEX_SLOT_STRIDE 32   /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth loads before the
                                    opponents, so his images 20..30 must not take the next group's slots */
#endif
#ifndef TM2_TEX_SLOT_STRIDE
#define TM2_TEX_SLOT_STRIDE 20
#endif
#if defined(TM2_PORT_DT_PLAYER) && TM2_TEX_SLOT_STRIDE < 32
#error "TM2_PORT_DT_PLAYER needs TM2_TEX_SLOT_STRIDE 32 (Dark Tooth's 31 images, SURVIVAL.md 4.2)"
#endif
#if (TM2_MAX_LOADED_CARS + 1) * TM2_TEX_SLOT_STRIDE > 1599
#error "the texture slot table (1,599 slots) cannot hold every load group"
#endif
/* ---- Object ids of the human records (docs/HUMANS16_PLAN.md 1.4, step H0) ----
 * Retail: human i's car object gets id i + 1 (carBindHumanSlot), AI j's id j + 50 (carBindAiSlot); id 8
 * is the world root ("no car": weaponApplyHit, carContactResolve, the hit results), id 10 every missile
 * record's scene object (missilesCreateObjects), which hdpQueryPoint can report as a hit. With more than
 * 7 human records the ids skip both: humans 0..6 keep 1..7, human 7 gets 9, humans 8.. get 11.. (16
 * humans: 1..7, 9, 11..18). Nothing else uses 11..49 (H0 scan, HUMANS16_PLAN.md 1.4).
 *   TM2_HUMAN_ID(i)            record index -> object id (every writer of a human id: binds, owners)
 *   TM2_HUMAN_INDEX_OF_ID(id)  object id of a human -> record index (no range check: the caller knows
 *                              the id is a human's; portHumanOfId is the checked form for the port)
 * Up to 7 human records both are retail's i + 1 / id - 1, so retail and every variant with at most 7
 * humans compile to the same code. tools/check_humans.py proves no game function computes a human id
 * or index any other way. */
#if TM2_MAX_HUMANS > 7
#define TM2_HUMAN_ID(i) ((i) < 7 ? (i) + 1 : (i) < 8 ? (i) + 2 : (i) + 3)
#define TM2_HUMAN_INDEX_OF_ID(id) ((id) <= 7 ? (id) - 1 : (id) <= 9 ? (id) - 2 : (id) - 3)
#else
#define TM2_HUMAN_ID(i) ((i) + 1)
#define TM2_HUMAN_INDEX_OF_ID(id) ((id) - 1)
#endif
/* -DTM2_PORT_HUMANS=N (the port's --port-define; needs TM2_PORT_CARS): N human records (TM2_MAX_HUMANS
 * = N, 4..16; online, request 33 / HUMANS16_PLAN.md: up to 16, at most 4 of them local, 16 cars in all in any
 * mix). Step H0 brings the ids, step H1 the local map (port_humans.h: views, cameras, HUDs and radars by local
 * slot), step H2 the inputs (port_humans.h: pads by local slot, the port's input words), step H3 the sound sides
 * and local feedback, step H4 the storage: every per-human array is [TM2_MAX_HUMANS] in port storage
 * (config/US/port_variant_arrays.tsv), carResetWeapons moves there past TM2_VARIANT_HUMANS (below), settingsCopy
 * keeps to the settings record's own entries. A human record is an active car, so N <= TM2_PORT_CARS. More than 7
 * records without TM2_PORT_HUMANS is an error (the ids above are its design). */
#ifdef TM2_PORT_HUMANS
#ifndef TM2_PORT_CARS
#error "TM2_PORT_HUMANS needs TM2_PORT_CARS (16 humans need the 64 car slots, 32 load groups and 16 AI records)"
#endif
#if TM2_PORT_HUMANS < 4 || TM2_PORT_HUMANS > 16
#error "TM2_PORT_HUMANS must be 4..16"
#endif
#if TM2_PORT_HUMANS > TM2_PORT_CARS
#error "TM2_PORT_HUMANS must not exceed TM2_PORT_CARS (every human record is an active car)"
#endif
#elif TM2_MAX_HUMANS > 7
#error "more than 7 human records need TM2_PORT_HUMANS (object ids, HUMANS16_PLAN.md 1.4)"
#endif
#if TM2_HUMAN_ID(TM2_MAX_HUMANS - 1) >= 50
#error "the last human's object id would reach the AI ids (50..)"
#endif
#if TM2_MAX_AI_CARS > 49
#error "TM2_MAX_AI_CARS > 49: AI object ids 50.. would reach 99"
#endif
/* The car DMD area (portCarDmdArea, every variant; used with portCarOptions bit 1 under TM2_PORT_CARS):
 * 2 MiB + one CD sector of overrun by default (request 32 Q14: 32 loaded cars packed by their actual
 * size; the disc's car DMDs are 24,940..39,168 bytes, Dark Tooth's 61,660; preloading every pickable car
 * takes about 1.15 MB). Predefine it to change it (a multiple of 4); the port sections must still end
 * below the port RAM end (--port-ram-end, default 0x80C00000: gen_link.DEFAULT_PORT_RAM_END). */
#ifndef TM2_PORT_CAR_DMD_AREA_BYTES
#define TM2_PORT_CAR_DMD_AREA_BYTES (0x200000 + 0x800)
#endif
/* A radar record (hudRadars, hud_radar.c TM2Radar): 0x30 bytes of polygons, 4 lines per blip (two
 * per blip and frame buffer, 16 bytes each), then 0x40 bytes (cross, draw modes, flags, count,
 * scale, buffer). Retail: 0x370 for 12 blips. */
#define TM2_HUD_RADAR_LINES (4 * TM2_MAX_RADAR_BLIPS)
#define TM2_HUD_RADAR_BYTES (0x70 + 16 * TM2_HUD_RADAR_LINES)
#if TM2_MAX_AI_CARS < TM2_RETAIL_AI_CARS || TM2_MAX_CAR_SLOTS < TM2_RETAIL_CAR_SLOTS || \
    TM2_MAX_LOADED_CARS < TM2_RETAIL_LOADED_CARS || \
    TM2_MAX_RADAR_BLIPS < TM2_RETAIL_RADAR_BLIPS || TM2_MAX_PROJECTILES < TM2_RETAIL_PROJECTILES || \
    TM2_MAX_COORD_SYSTEMS < TM2_RETAIL_COORD_SYSTEMS || TM2_MAX_SOUND_INSTANCES < TM2_RETAIL_SOUND_INSTANCES ||     TM2_MAX_SOUND_REQUESTS < TM2_RETAIL_SOUND_REQUESTS
#error "a car or pool limit below retail's"
#endif

#ifdef TM2_PORT_VARIANT
/* The moved arrays live above 2 MB: never through $gp (config/US/port_variant_arrays.tsv). */
#pragma tm2_absolute aiCars
#pragma tm2_absolute aiCarObjects
#pragma tm2_absolute aiCarObjectSlots
#pragma tm2_absolute aiCarTargets
#pragma tm2_absolute carSlotKinds
#pragma tm2_absolute carSlotIndices
#pragma tm2_absolute carSlotOwners
#pragma tm2_absolute carSlotObjects
#pragma tm2_absolute carPairVectors
#pragma tm2_absolute carPairHeadingDeltas
#pragma tm2_absolute carPairDirections
#pragma tm2_absolute carPairHeadings
#pragma tm2_absolute carPairDistances
#pragma tm2_absolute sessionPortOpponentCars
#pragma tm2_absolute portCarsPerArena
#pragma tm2_absolute portCarDmdArea
#pragma tm2_absolute fxRecords
#pragma tm2_absolute fxRecordTrailSegments
#pragma tm2_absolute csObjectPool
#pragma tm2_absolute soundInstPool
#pragma tm2_absolute portSoundSortList
#pragma tm2_absolute humanCars
#pragma tm2_absolute humanCarObjects
#pragma tm2_absolute humanCarObjectSlots
#pragma tm2_absolute padConfigs
#pragma tm2_absolute humanCarTypes
#pragma tm2_absolute hudSmoothedSpeeds
#pragma tm2_absolute carViewHotkeyReleased
#pragma tm2_absolute viewCameraObjectTable
#pragma tm2_absolute viewFollowObjects
#pragma tm2_absolute viewCameraObjects
#pragma tm2_absolute viewEnvs
#pragma tm2_absolute viewModes
#pragma tm2_absolute viewModeTable
#pragma tm2_absolute a80181120
#pragma tm2_absolute viewEyePositions
#pragma tm2_absolute viewMatrices
#pragma tm2_absolute hudMarkerBlinkCounters
#pragma tm2_absolute hudMarkerBlinkPhases
#pragma tm2_absolute hudOrderTables
#pragma tm2_absolute hudPlayerRecords
#pragma tm2_absolute hudRadars
#pragma tm2_absolute hudRadarArrowUpPrims
#pragma tm2_absolute hudRadarArrowDownPrims
#pragma tm2_absolute hudRadarArrowLeftPrims
#pragma tm2_absolute hudRadarArrowRightPrims
#pragma tm2_absolute hudTargetNameModes
#pragma tm2_absolute hudTargetNameSprites
#pragma tm2_absolute hudRadarArrowUpSprites
#pragma tm2_absolute hudRadarArrowDownSprites
#pragma tm2_absolute hudRadarArrowLeftSprites
#pragma tm2_absolute hudRadarArrowRightSprites
#pragma tm2_absolute sessionPortHumanCars
#if TM2_MAX_HUMANS > TM2_VARIANT_HUMANS
#pragma tm2_absolute carResetWeapons   /* moved past 4 humans (step H4) */
#endif

/* Port API (.port_text, port_variant_arrays.c; every port variant): the human record index of an object id
 * (a car object's +0x118, a projectile / drop / bullet owner), or -1 when the id is not a present human's
 * (0, 8, 10, AI ids 50.., an index >= humanCarTotal). For the port's kill feed: never `id - 1`. */
extern int portHumanOfId(int id);
#endif

#endif
