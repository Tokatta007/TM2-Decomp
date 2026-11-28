/* Car numbers past retail's, and car select's carousel for them (PC port request 42 items 3 and 5;
 * docs/PER_CAR_PLAN.md E.5, docs/CAR_GROUPS.md 18, docs/DARK_TOOTH_PLAYER.md 10).
 *
 * A car number is what the settings call a car-select index: 0..11 the regular cars, 12 Sweet Tooth, 13 Minion,
 * 14 Dark Tooth (carIndexToType). Retail keeps them in signed bytes of the settings record (human_car[2] at +0x08,
 * opponent_car[10] at +0x0A; with the port variant players 3.. and opponents 9.. in sessionPortHumanCars /
 * sessionPortOpponentCars), which leaves about 96 numbers above the mod base 0x20 and breaks at 0x80 (sign).
 *
 * With -DTM2_PORT_CAR_NUMBERS (the port's `--port-define TM2_PORT_CAR_NUMBERS`; needs the port variant) a car number
 * is a 16-bit value: 0..14 retail's, TM2_PORT_MOD_FIRST + m (0x20..0x9F) the port's mod car m (m 0..127, described in
 * portModCars[m]), -1 none. sessionSettings' numbers are kept in portSettingsHumanCar / portSettingsOpponentCar; the
 * record's bytes keep the mod's base (the retail car it is built on, portModCars[m].base), so every reader of the
 * bytes sees a retail car (the load's disc fallback, the bind's type, the boss and Minion rules, settingsCopy, the
 * port's own byte reads):
 *   - every setter writes the number and the byte (the mirror), every clear both (-1);
 *   - the getters (settingsGetHumanCar, sessionGetHumanCar, settingsGetOpponentCar, sessionGetOpponentCar) return an
 *     int: a mod's number when the table holds one whose base is the byte, else the byte (another record; bytes
 *     written around the setters), so without a mod number every getter returns retail's value;
 *   - carIndexToType, carFileCode and carDisplayName take numbers (a mod: its base's type, the port's file code and
 *     name or else the base's); carIsSelectable compares numbers and refuses numbers that are no car; the
 *     tournament password is never shown with a number >= 12 (sessionRun, pauseMenu); screenEnding plays a mod's
 *     movie (the port's) and reads endingMovieTable only below 14; sessionLoadArenaAndCars (TM2_PORT_CARS) hands
 *     portCarLoadGroup the number, whose disc fallback loads the base's files, and records it per load group
 *     (portCarNumberOfGroup); shellAttractDemo keeps the numbers around its demo match; portHumanCompact moves them.
 *
 * Car select (item 5): the carousel index IS the car number. CARSEL.DMD's select node 9005 and its two name-label
 * nodes 9006 / 9007 hold 14 children each. portCarSelExtend gives all three copies (portCarSel.node) with more:
 *   - TM2_PORT_DT_PLAYER and portCarOptions bit 8: 15 (request 36), child 14 Dark Tooth (portDtPlayer.carsel_model
 *     or the empty node); the labels' child 14 the empty node (the request 42 fix: retail's labels have 14, so
 *     menuCopyChoiceRun's index 14 made the label walk the word after their child table);
 *   - TM2_PORT_CAR_NUMBERS and portCarSel.mods > 0: TM2_PORT_MOD_FIRST + mods, children 0..13 retail's, 14 Dark Tooth
 *     as above (else empty), 15..0x1F empty and never accepted (carIsSelectable), 0x20 + m the mod's: the empty node
 *     until the port puts its relocated in-game model there (portCarSelSetChild; the draw walks only
 *     child[choice], with the carousel's position, spin, camera, lights, OT and packets). portCarSelDraw is called
 *     before each draw of the extended carousel: the port wraps it to load the highlighted mod car on demand.
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss); this header declares its layout for the
 * port variant and, with the defines, the port functions. */
#ifndef TM2_SEMANTIC_CAR_NUMBERS_H
#define TM2_SEMANTIC_CAR_NUMBERS_H

#define TM2_PORT_MOD_CARS 128            /* mod cars the numbers describe (portModCars) */
#define TM2_PORT_MOD_FIRST 0x20          /* the first mod's number (PER_CAR_PLAN.md E.4.1) */
#define TM2_PORT_NUMBER_HUMANS 16        /* portSettingsHumanCar entries (TM2_MAX_HUMANS at most) */
#define TM2_PORT_NUMBER_OPPONENTS 16     /* portSettingsOpponentCar entries (TM2_MAX_AI_CARS at most) */
#define TM2_PORT_NUMBER_GROUPS 64        /* portCarNumbers.group entries (load groups, car_groups.h TM2_CAR_SLOT_CAP) */
#define TM2_PORT_CARSEL_CHILDREN (TM2_PORT_MOD_FIRST + TM2_PORT_MOD_CARS)   /* 160: the carousel's most children */
#define TM2_PORT_CARSEL_RETAIL 14        /* CARSEL's carousel and labels: 14 children */
#define TM2_PORT_AISEL_RETAIL 12         /* AICARSEL's carousel and labels (Challenge's opponent picker): 12 children */
#define TM2_PORT_CAR_OPT_AI_SWEET_TOOTH 0x800   /* portCarOptions bit 11 (TM2_PORT_CAR_NUMBERS, request 42 item 2): the
                                            Challenge opponent picker (screenAiSelect) offers Sweet Tooth (car 12)
                                            while he is unlocked (unlockSweetTooth); locked: retail's 12 cars */
#define TM2_PORT_CARSEL_LOADS 4          /* request 71: carousel models installed at once (portCarSelLoad slots) */
#define TM2_PORT_CARSEL_DMD_BYTES 0x20000   /* each one's DMD copy (Dark Tooth's BB.DMD is 61,660 bytes) */
#define TM2_PORT_CARSEL_IMAGES 32        /* TMS images per model (Dark Tooth's BB.TMS has 31) */
#define TM2_PORT_CARSEL_ROOTS 4          /* car roots drawn per model (Minion's file has one; a mod may have more) */
#define TM2_PORT_CARSEL_BANK 124         /* TM2_PORT_TEXTURE_BANKS: slot k's VRAM bank is 124 + k (a match's load
                                            groups take the lowest free banks from 1; banks are freed per load) */
#define TM2_PORT_CARSEL_HEAD_ID 249     /* request 74: Dark Tooth's head root in BB.DMD (the boss update places it) */
#define TM2_PORT_CARSEL_HEAD_Z 240      /*   put this far above his truck on the carousel (the boss's hover height;
                                            request 109: in a transform node, a half turn about z) */
#define TM2_PORT_CARSEL_PHASE2_Z (-90)  /* request 118: the head alone (his second phase) this far from the truck's origin:
                                           26 over the ground as in a match (ride 50, hover -24), the truck's origin
                                           at its ride height 116 */
/* request 73 (docs/SPECIAL_TEMPLATES.md 6): portSpecialProfile's number fields and pickup templates */
#define TM2_PORT_SPECIAL_NUMS 16         /* number overrides per mod */
#define TM2_SPN_DAMAGE 1                 /* num[] fields: the main hit (each missile's, Axel's shockwave; stage 2:
                                            the taser's two hits, Thumper's flame per frame, Shadow's strike on
                                            contact), 0..8191 */
#define TM2_SPN_DAMAGE2 2                /*   Minion's freeze missile, 0..8191 */
#define TM2_SPN_LIFE 3                   /*   a projectile's life in its record (the spiral's and the boomerang's
                                            include retail's x4), Thumper's flame, Shadow's strike: 1..8191 */
#define TM2_SPN_SPEED 4                  /*   a projectile's start speed (the game caps every one at 150), Shadow's
                                            strike speed: 1..150 */
#define TM2_SPN_TURN 5                   /*   a projectile's turn rate: 1..4095 (the spiral's never 99) */
#define TM2_SPN_RANGE 6                  /*   the taser's reach (D-units, 80), Axel's radius (800): 1..4000 */
#define TM2_SPN_HEIGHT 7                 /*   Axel's height window (200): 1..4000 */
#define TM2_SPN_DURATION 8               /*   the taser's arc (40): 40..400 */
#define TM2_SPN_FREEZE 9                 /*   stage 2: a freeze missile's freeze, ticks (300; every freeze missile
                                            the car fires): 1..6000 */
#define TM2_SPN_BURN 10                  /*   stage 2: the burn Thumper's flame or a napalm hit sets, frames (100):
                                            1..2000 */
#define TM2_SPN_BLAST 11                 /*   stage 2: Shadow's detonation, the damage at its centre (25; on a
                                            surface polygon 25 more), 0..8191 */
#define TM2_SPN_BLAST_RADIUS 12          /*   stage 2: its radius, D-units (75): 1..400 */
#define TM2_SPN_TURNBACK 13              /*   stage 2: the boomerang's turn-back distance, D-units (300): 1..4000 */
#define TM2_SPN_LAST 13
/* pickup weapons as a special (portCarSpecialAs values, never a car type) */
#define TM2_SPECIAL_PICKUP_FIRE 0x101    /* fire missile (pickup kind 1) */
#define TM2_SPECIAL_PICKUP_HOMING 0x102  /* homing missile (2) */
#define TM2_SPECIAL_PICKUP_BOMB 0x103    /* stage 2: remote bomb (3): a press places it, the next one detonates */
#define TM2_SPECIAL_PICKUP_POWER 0x104   /* power missile (4) */
#define TM2_SPECIAL_PICKUP_NAPALM 0x105  /* napalm (5) */
#define TM2_SPECIAL_PICKUP_RICOCHET 0x106 /* stage 2: ricochet bomb (6) */
#define TM2_SPECIAL_PICKUP_MEGA 0x10A    /* Mega Napalm (napalm with weap->mega_napalm) */
#define TM2_SPECIAL_PICKUP_FREEZE 0x164  /* freeze missile (energy kind 100) */
#define TM2_SPECIAL_PICKUP_AI 50         /* a computer car's slot-0 weight for a pickup template without ai_* */
#define TM2_SPECIAL_AI_TABLE 12          /* bytes of an AI weight table copy (9 or 10 entries used) */
#define TM2_PORT_PROFILE_STATS 24        /* request 70: stats overrides per profile */
#define TM2_PORT_PROFILE_STEP 0x01       /* portCarProfile flags: step replaces the base's */
#define TM2_PORT_PROFILE_SPECIAL 0x02    /*   recharge, capacity, max_specials replace the base's */
#define TM2_PORT_PROFILE_RADAR 0x04      /*   radar[] is the blip's colour */
#define TM2_PORT_PROFILE_RADAR_KEY 0x1000000    /* hudRadarPlotCar's type argument with a profile's colour: this bit
                                            and 0xRRGGBB (no car type has it) */

#ifdef TM2_PORT_VARIANT
/* portModCars[m] (.port_bss, config/US/port_variant_arrays.tsv group one): mod car m = number 0x20 + m. Set by the
 * port (0 = no such car); read by the C. Offsets in bytes. */
struct TM2PortModCar {
    unsigned char used;          /* +0x00 1 = number 0x20 + m is a car */
    unsigned char base;          /* +0x01 the retail car (0..14) it is built on: the settings byte, its type and the
                                          disc fallback of its files */
    unsigned char pad[2];        /* +0x02 */
    const char *name;            /* +0x04 carDisplayName (0 = the base's) */
    const char *code;            /* +0x08 carFileCode: the opponent plate TIM "<code><suffix>" (0 = the base's) */
    const char *bio_picture;     /* +0x0C car select "info": a TIM file (screenLoadImageFile's name); 0 = none */
    const char *ending_movie;    /* +0x10 screenEnding: its movie (moviePlayEnding's name); 0 = none (YOUWON.STR
                                          still plays) */
    int ending_frames;           /* +0x14 its frame count */
    int ending_wide;             /* +0x18 its wide flag (480 x 240) */
    int reserved;                /* +0x1C */
};
typedef char tm2_port_mod_car_size[(sizeof(struct TM2PortModCar) == 0x20) ? 1 : -1];

/* portCarNumbers (.port_bss, group one): written by the C. */
struct TM2PortCarNumbers {
    short group[TM2_PORT_NUMBER_GROUPS];            /* +0x00 per load group: 1 + the car number portCarLoadGroup
                                                       loaded last into it (0 = none since the variant started) */
    short saved_human[TM2_PORT_NUMBER_HUMANS];      /* +0x80 shellAttractDemo: the numbers around its demo match */
    short saved_opponent[TM2_PORT_NUMBER_OPPONENTS];/* +0xA0 */
    int pad[4];                                     /* +0xC0 */
};
typedef char tm2_port_car_numbers_size[(sizeof(struct TM2PortCarNumbers) == 0xD0) ? 1 : -1];

/* portCarSel (.port_bss, group one): car select's extended nodes (request 36's carousel and request 42's). */
struct TM2PortCarSel {
    int mods;                    /* +0x000 port: the mod cars car select offers (numbers 0x20 .. 0x20 + mods - 1, at
                                          most 128), with TM2_PORT_CAR_NUMBERS; 0 = none */
    int count;                   /* +0x004 C: the extended carousel's children this car select (0 = CARSEL's 14) */
    int shown;                   /* +0x008 C: the index the extended carousel drew last (portCarSelDraw) */
    int retail;                  /* +0x00C C: the copied nodes' own children (14 car select, 12 AI select) */
    unsigned char blank[28];     /* +0x010 the empty node: a hidden group (type 1, +21 flag 2) */
    int pad2;                    /* +0x02C */
    unsigned char node[3][16 + 4 * TM2_PORT_CARSEL_CHILDREN];   /* +0x030 the copies of the carousel (9005), label a
                                          (9006) and label b (9007): header (type 9, +6 count, +7 choice), children */
};
typedef char tm2_port_car_sel_size[(sizeof(struct TM2PortCarSel) == 0x7E0) ? 1 : -1];

/* portCarSelLoads (.port_bss, group one; request 71): the in-game models portCarSelLoad installed for car select's
 * carousel (or the AI select's), one per load slot k (a player each in split-screen car select). Written by the C;
 * the port reads `index` and may write a file straight into `dmd` (then passes that address: nothing is copied).
 * Shell presentation only: rollback_regions.tsv snapshot and hash no. */
struct TM2PortCarSelLoad {
    int index;                   /* +0x000 the carousel child this model is set on, -1 none */
    int bank;                    /* +0x004 its VRAM bank (TM2_PORT_TEXTURE_BANKS: TM2_PORT_CARSEL_BANK + k), 0 = the
                                          PS1's VRAM (slot 0 only, car select's free block) */
    int images;                  /* +0x008 the TMS's images placed */
    int roots;                   /* +0x00C the car roots under `group` */
    int slots[TM2_PORT_CARSEL_IMAGES][5];   /* +0x010 the private texture slot table modelRemapTextures reads
                                          (tpage with the bank in bits 9..15, x, y, CLUT x, CLUT y), image i */
    short rects[TM2_PORT_CARSEL_IMAGES][4]; /* +0x290 the pixel rectangles taken (x, y, w, h) */
    unsigned char group[24 + 4 * TM2_PORT_CARSEL_ROOTS]; /* +0x390 a group node (type 1) over the roots when there
                                          are several (Minion), else unused */
    void *model;                 /* +0x3B8 the node set as the child (a root in dmd, or group), 0 none */
    int pad;                     /* +0x3BC */
    unsigned char dmd[TM2_PORT_CARSEL_DMD_BYTES];  /* +0x3C0 the DMD copy (relocated here) */
};
typedef char tm2_port_car_sel_load_size[(sizeof(struct TM2PortCarSelLoad) == 0x3C0 + TM2_PORT_CARSEL_DMD_BYTES) ? 1
                                        : -1];

/* portCarProfile[m] (.port_bss, group one; request 70): mod car m's own numbers over its base's, set by the port
 * before each load (the same on every machine: match state, hashed), read by the C only with TM2_PORT_CAR_PROFILE.
 * valid 0 = the base car's everything (retail). Offsets in bytes. */
struct TM2PortCarStat {
    unsigned char offset;        /* +0 the stats block field (CAR_STATS.md 2: +0x52 top speed, +0x3A max HP, ...) */
    unsigned char width;         /* +1 its size: 1, 2 or 4 bytes (0 = unused entry) */
    short value;                 /* +2 the value (sign-extended into a 4-byte field) */
};
struct TM2PortCarProfile {
    unsigned char valid;         /* +0x00 1 = consult this profile */
    unsigned char stat_count;    /* +0x01 entries of stat[] used */
    unsigned char flags;         /* +0x02 TM2_PORT_PROFILE_*: which of the fields below replace the base's */
    unsigned char cam_mask;      /* +0x03 bit v: cam[v] replaces that camera offset (0 look-at point, 1 chase,
                                          2 view 3, 3 the other full-screen views, 4 split mode 1, 5 split mode 2) */
    short step;                  /* +0x04 the highest climbable step (carWheelsSetStepHeight; retail 0x28..0xA0) */
    short recharge;              /* +0x06 the special's recharge period in frames (weaponBlockInit) */
    short capacity;              /* +0x08 the pickup capacity (24 for every car) */
    signed char max_specials;    /* +0x0A the specials held at most */
    unsigned char radar[3];      /* +0x0B the radar blip's colour r, g, b (hudRadarPlotCar) */
    unsigned char pad_0e[2];     /* +0x0E */
    const char *name;            /* +0x10 the target panel's name (hudDrawTargetPanel; 0 = the base's, carTypeName) */
    short cam[6][3];             /* +0x14 the camera offsets of cam_mask (viewCameraOffsetsInit, the human record's
                                          +0x080, +0x044, +0x050, +0x05C, +0x068, +0x074; car space x, y, z) */
    int reserved[2];             /* +0x38 */
    struct TM2PortCarStat stat[TM2_PORT_PROFILE_STATS];  /* +0x40 the stats overrides, in order */
};
typedef char tm2_port_car_profile_size[(sizeof(struct TM2PortCarProfile) == 0xA0) ? 1 : -1];

/* Request 73 (docs/SPECIAL_TEMPLATES.md 6): a mod car's special. portSpecialProfile[m] (.port_bss, group one), by mod
 * like portCarProfile, set by the port before each load (match state, hashed), read by the C only with
 * TM2_PORT_SPECIAL_PROFILE and when the mod's portCarProfile is valid: `special` is the template the car fires
 * (0 = its base's own; a borrowable car type, TM2_PORT_SPECIAL_AS_MASK; or a pickup weapon TM2_SPECIAL_PICKUP_*),
 * num[] the numbers of whatever special it fires (TM2_SPN_*, each clamped to its limit), ai_ahead / ai_behind a
 * computer car's slot-0 weight in percent (0 = the template's table). Offsets in bytes. */
struct TM2PortSpecialNum {
    unsigned char field;         /* +0 TM2_SPN_* (0 = unused) */
    unsigned char pad;           /* +1 */
    short value;                 /* +2 */
};
struct TM2PortSpecialProfile {
    short special;               /* +0x00 the template (0 = the base's special) */
    unsigned char count;         /* +0x02 entries of num[] used */
    unsigned char ai_ahead;      /* +0x03 a computer car's slot-0 weight with its target ahead, percent (0 = table) */
    unsigned char ai_behind;     /* +0x04 and behind */
    unsigned char fire_chance;   /* +0x05 request 87: a computer driver's fire chance, 1..100 (0 = the template's):
                                    in place of the type's base, before the level and mode bonuses and the clamp */
    unsigned char pad[2];        /* +0x06 */
    struct TM2PortSpecialNum num[TM2_PORT_SPECIAL_NUMS];   /* +0x08 */
};
typedef char tm2_port_special_profile_size[(sizeof(struct TM2PortSpecialProfile) == 0x48) ? 1 : -1];
/* portSpecialState (.port_bss, group one; written by the C): the projectile records free when the fire began (bit i
 * of free[i >> 5]), and per mod the AI weight tables with its slot-0 weights (aiCarSetupProfile points a computer
 * car at them). */
struct TM2PortSpecialState {
    unsigned int free[2];                                          /* +0x000 */
    unsigned char w9[TM2_PORT_MOD_CARS][TM2_SPECIAL_AI_TABLE];     /* +0x008 */
    unsigned char w10[TM2_PORT_MOD_CARS][TM2_SPECIAL_AI_TABLE];    /* +0x608 */
};
typedef char tm2_port_special_state_size[(sizeof(struct TM2PortSpecialState) == 0xC08) ? 1 : -1];
#endif

/* Request 42 item 5: the extended car select, with a player's Dark Tooth (bit 8) or with mod cars. */
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
#pragma tm2_absolute portCarSel
#pragma tm2_absolute portCarSelLoads
#define TM2_CARSEL_DT_PHASE2 0x10000    /* request 118: portCarSelLoad's `index` | this: Dark Tooth (BB.DMD) shown in
                                            his second phase, the head alone where it sits on the car in a match */
#define TM2_CARSEL_LOAD_SLOT -1          /* portCarSelLoad: k is no slot (without texture banks only slot 0) */
#define TM2_CARSEL_LOAD_INDEX -2         /*   `index` is not one of the extended carousel's extra children */
#define TM2_CARSEL_LOAD_DMD -3           /*   the DMD is no PSXD image, is not 4-aligned, or has no root */
#define TM2_CARSEL_LOAD_TMS -4           /*   the TMS is no PSXT container, is cut short, or has too many images */
#define TM2_CARSEL_LOAD_VRAM -5          /*   its images do not fit (without banks: the AI select, or more than
                                            car select's free block) */
#define TM2_CARSEL_LOAD_LARGE -6         /*   request 130: portCarSelLoad: the DMD is larger than the slot's copy
                                            (TM2_PORT_CARSEL_DMD_BYTES; before request 130 this was -3); the port
                                            can load it with portCarSelLoadInPlace instead */
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortCarSel portCarSel;
/* .port_text (port_variant_arrays.c) */
extern void portCarSelExtend(void *carousel, void *label_a, void *label_b);   /* screenCarSelect, after CARSEL is
                                                    loaded: the extended copies (bit 8 / mods), else CARSEL's own */
extern int  portCarSelCount(void);              /* the carousel's children this car select (14 when not extended) */
extern const char *portCarBioFile(int index);   /* the info picture of carousel index `index`, 0 = none */
extern void *portCarSelSetChild(int index, void *model); /* the port's API: carousel child `index` (14 or a mod's)
                                                    becomes `model` (0 = the empty node); returns the old one, 0 when
                                                    the index has no extended child */
extern int  portCarSelNumber(int index);        /* screenCarSelect: the car number of carousel index `index` (the
                                                    same value: index = number), -1 if it is no car */
extern void portCarEnding(int number);           /* screenEnding, a car number past 13: Dark Tooth's movie (the
                                                    port's, portDtPlayer) or a mod's (portModCars), else none */
extern struct TM2PortCarSelLoad portCarSelLoads[];   /* [TM2_PORT_CARSEL_LOADS] request 71 */
extern int  portCarSelLoad(int k, int index, const void *dmd, int dmd_len, const void *tms, int tms_len);
                                                /* the port's API (request 71): install a car's DMD and TMS in load
                                                    slot k and set it as carousel child `index` (slot k's previous
                                                    model freed); 1, or TM2_CARSEL_LOAD_* < 0 (nothing set) */
extern int  portCarSelLoadInPlace(int k, int index, void *dmd, int dmd_len, const void *tms, int tms_len);
                                                /* request 130: as portCarSelLoad, the DMD relocated and drawn in the
                                                    port's buffer `dmd` (any size, 4-aligned; kept unchanged until slot
                                                    k is loaded again or unloaded) */
extern void portCarSelUnload(int k);            /* the port's API: slot k's child back to the empty node */
#ifdef TM2_PORT_CAR_NUMBERS
extern void portAiSelExtend(void *carousel, void *label_a, void *label_b);   /* screenAiSelect, after AICARSEL is
                                                    loaded: bit 11 and Sweet Tooth unlocked: 13-child copies of
                                                    the carousel and both name labels (child 12 Sweet Tooth: the
                                                    empty node until the port sets a model, portCarSelSetChild) */
extern int  portAiSelCount(void);               /* the AI select carousel's children (13 when extended, else 12) */
#endif
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_CAR_NUMBERS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_CAR_NUMBERS needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portSettingsHumanCar
#pragma tm2_absolute portSettingsOpponentCar
#pragma tm2_absolute portModCars
#pragma tm2_absolute portCarNumbers
#ifdef __cplusplus
extern "C" {
#endif
struct TM2Settings;
extern short portSettingsHumanCar[];            /* [16] sessionSettings' human car numbers (-1 none) */
extern short portSettingsOpponentCar[];         /* [16] its opponent car numbers */
extern struct TM2PortModCar portModCars[];      /* [128] the port's mod cars */
extern struct TM2PortCarNumbers portCarNumbers;
/* .port_text (port_variant_arrays.c) */
extern int  portCarNumberGet(struct TM2Settings *s, int kind, int i);   /* kind 0 human i, 1 opponent i: the
                                                    number (a mod's when the table's base is the byte), else the byte */
extern void portCarNumberSet(struct TM2Settings *s, int kind, int i, int number);  /* the setters: number + byte */
extern void portCarNumbersClear(struct TM2Settings *s, int kinds);     /* the clears: bit 0 humans, bit 1 opponents */
extern int  portCarNumberBase(int number);      /* the settings byte of a number: itself below 0x20, a used mod's
                                                    base, else -1 */
extern int  portCarNumberPickable(int number);  /* carIsSelectable: 0 for a number that is no car (15..0x1F, an unused
                                                    mod; 14 without TM2_PORT_DT_PLAYER's bit 8) */
extern int  portCarNumberType(int number);      /* carIndexToType past retail's: a used mod's base's type, else -1 */
extern const char *portCarNumberName(int number, const char *unknown);  /* carDisplayName past retail's: a used mod's
                                                    name (else its base's), else `unknown` */
extern const char *portCarNumberCode(int number, const char *unknown);  /* carFileCode past retail's: a used mod's
                                                    file code (else its base's), else `unknown` */
extern int  portCarNumberOfGroup(int group);    /* the port's API: the car number of load group `group`, -1 none */
extern void portCarNumbersKeep(int restore);    /* shellAttractDemo: 0 save the numbers, 1 restore them */
extern void portCarSelDraw(void *node);         /* sceneObjDraw, before drawing the extended carousel (the port
                                                    wraps it: load the highlighted mod car, portCarSelSetChild) */
#ifdef __cplusplus
}
#endif
#endif

/* Request 70 (PER_CAR_PLAN.md E.2, docs/CAR_PROFILES.md): a mod car's own numbers. With -DTM2_PORT_CAR_PROFILE every
 * function that picks a car's numbers by its type asks portCarProfile first for the car a record drives (its slot's
 * load group's number, portCarNumberOfGroup): a valid profile of a used mod replaces the base's stats fields, step
 * height, special numbers, camera offsets, radar colour and target name; anything it leaves out stays the base's,
 * and without a valid profile every caller runs retail's code. The behaviour (the special, the AI's code, the Dark
 * Tooth and Minion rules) stays the base type's. */
#ifdef TM2_PORT_CAR_PROFILE
#if !defined(TM2_PORT_CAR_NUMBERS) || !defined(TM2_PORT_CARS)
#error "TM2_PORT_CAR_PROFILE needs TM2_PORT_CAR_NUMBERS and TM2_PORT_CARS (the car number of each load group)"
#endif
#pragma tm2_absolute portCarProfile
#ifdef __cplusplus
extern "C" {
#endif
struct TM2CarStatsBlock;
struct TM2CarWheels;
struct TM2CarWeapons;
struct TM2HumanRecord;
extern struct TM2PortCarProfile portCarProfile[];  /* [128] by mod: number 0x20 + m */
/* .port_text (port_variant_arrays.c) */
extern struct TM2PortCarProfile *portCarProfileOfSlot(int slot);   /* the port's API too: the valid profile of the
                                                    car in car slot `slot`, 0 = none (the base's numbers) */
extern void portCarProfileStats(void *stats);   /* carStatsLoad, before its derived fields: the overrides */
extern void portCarProfileStep(struct TM2CarWheels *wheels);    /* carWheelsSetStepHeight: the step */
extern void portCarProfileCamera(struct TM2HumanRecord *human); /* viewCameraOffsetsInit: the camera offsets */
extern void portCarProfileWeapons(void *car, struct TM2CarWeapons *rec, int human); /* weaponBlockInit */
extern int  portCarProfileTopSpeed(int slot, int type);  /* carDefaultTopSpeed for the car in `slot` */
extern int  portCarProfileRadar(int slot, int type);     /* hudRadarPlotCar's type argument: `type`, or the
                                                    profile's colour | TM2_PORT_PROFILE_RADAR_KEY */
extern const char *portCarProfileName(int slot, int type);   /* hudDrawTargetPanel: the profile's name, else
                                                    carTypeName(type) */
#ifdef __cplusplus
}
#endif
#define TM2_CAR_DEFAULT_TOP_SPEED(slot, type) portCarProfileTopSpeed((slot), (type))
#else
#define TM2_CAR_DEFAULT_TOP_SPEED(slot, type) carDefaultTopSpeed(type)
#endif

/* Request 73 (docs/SPECIAL_TEMPLATES.md 6): with -DTM2_PORT_SPECIAL_PROFILE a mod car fires the special its
 * portSpecialProfile names (applied as request 35's borrowed special at each start: weaponBlockInit), a pickup weapon
 * through the launcher, and every special it fires takes the profile's numbers: the projectiles a fire makes
 * (damage, life, speed, turn), Axel's shockwave, the taser, Thumper's flame, Shadow's strike. A computer car uses the
 * template's weight tables. Without a valid profile every hook is retail's. */
#ifdef TM2_PORT_SPECIAL_PROFILE
#if !defined(TM2_PORT_CAR_PROFILE) || !defined(TM2_PORT_SPECIAL_AS)
#error "TM2_PORT_SPECIAL_PROFILE needs TM2_PORT_CAR_PROFILE and TM2_PORT_SPECIAL_AS (the template is a borrowed special)"
#endif
#pragma tm2_absolute portSpecialProfile
#pragma tm2_absolute portSpecialState
#ifdef __cplusplus
extern "C" {
#endif
struct TM2AIRecord;
extern struct TM2PortSpecialProfile portSpecialProfile[];   /* [128] by mod: number 0x20 + m */
extern struct TM2PortSpecialState portSpecialState;
/* .port_text (port_variant_arrays.c) */
extern int  portSpecialTemplate(int slot);       /* the template of the car in `slot` (0 = none) */
extern int  portSpecialNum(int slot, int field, int retail);   /* the car's number `field`, else `retail` */
extern void portSpecialFireBegin(void);          /* weaponFireSpecial, before its switch: the free records */
extern void portSpecialFireEnd(int slot, int vehicle); /* after it: the new projectiles take the numbers */
extern void portSpecialFirePickup(void *car, unsigned int human, int vehicle); /* a pickup template's fire */
extern int  portSpecialAiType(struct TM2AIRecord *ai);   /* aiCarSetupProfile: the type whose tables it takes */
extern void portSpecialAiTables(struct TM2AIRecord *ai); /* after them: a pickup template's slot-0 weights */
extern int  portSpecialNumById(int id, int field, int retail);  /* stage 2: the number of the car whose object id
                                                    is `id` (a projectile's or emitter's owner), else `retail` */
extern int  portSpecialBlast(int slot, int big, int retail); /* areaDamage: Shadow's blast base (big 0) */
extern void portSpecialBoomerangCaught(int id);  /* weaponProjectileUpdate: a caught boomerang's charge back to
                                                    its thrower (retail: the first human Roadkill) */
#ifdef __cplusplus
}
#endif
#define TM2_SPECIAL_NUM(slot, field, retail) portSpecialNum((slot), (field), (retail))
#define TM2_SPECIAL_NUM_ID(id, field, retail) portSpecialNumById((id), (field), (retail))
/* a burn's start (the burn ends when burn_time reaches 100): 100 - the car's burn length, retail 0 */
#define TM2_SPECIAL_BURN_START(id) (100 - portSpecialNumById((id), TM2_SPN_BURN, 100))
#else
#define TM2_SPECIAL_NUM(slot, field, retail) (retail)
#define TM2_SPECIAL_NUM_ID(id, field, retail) (retail)
#define TM2_SPECIAL_BURN_START(id) 0
#endif

#endif
