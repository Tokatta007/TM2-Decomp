/* Session settings record (sessionSettings, 8016475C, 48 bytes).
 *
 * Layout from the accessors in game_setting_accessors.c,
 * game_settings_extra.c, menu_state_accessors.c and settingsCopy
 * (menu_state_copy.cpp); see docs/TOURNAMENT_PASSWORDS.md and
 * docs/COOP_LIVES.md. Uses plain C types so that units with their own
 * u8/s32 typedefs can include it. */
#ifndef TM2_SEMANTIC_SETTINGS_H
#define TM2_SEMANTIC_SETTINGS_H

#include "semantic/players.h"

struct TM2Settings {
    unsigned int mode;                  /* +0x00 0 attract/demo, 1 tournament, 2 co-op
                                         * tournament, 3 challenge, 4 deathmatch */
    unsigned char humans;               /* +0x04 human players */
    unsigned char opponents;            /* +0x05 computer opponents in the arena */
    unsigned char car_select_player;    /* +0x06 humans who have picked a car */
    unsigned char opponents_picked;     /* +0x07 opponents picked (AI select) */
    signed char human_car[2];           /* +0x08 car-select index of each human */
    signed char opponent_car[10];       /* +0x0A car-select index per opponent slot,
                                         * -1 = empty (settingsClearOpponentCars
                                         * clears 13 bytes, into lives[0..2]) */
    unsigned char lives[4];             /* +0x14 per player; the runner decrements [0] */
    int level;                          /* +0x18 level / arena (tournament 1..8) */
    unsigned char boss_intro;           /* +0x1C */
    unsigned char dark_tooth_stage;     /* +0x1D Dark Tooth stage of level 8 */
    unsigned char dark_tooth_reveal;    /* +0x1E */
    unsigned char pad_1F;
    int f20;                            /* +0x20 last bitmapParseContainer result
                                         * (sessionLoadArenaAndCars) */
    unsigned char difficulty;           /* +0x24 0 easy, 1 medium, 2 hard */
    unsigned char world_map;            /* +0x25 */
    unsigned char control_layout[2];    /* +0x26 controller layout per player, 0..3 */
    unsigned char cd_volume;            /* +0x28 (settingsInitDefaults) */
    unsigned char sfx_volume;           /* +0x29 */
    unsigned char pad_2A[6];
};
typedef char tm2_settings_size[(sizeof(struct TM2Settings) == 0x30) ? 1 : -1];
typedef char tm2_settings_level_at[((unsigned int)&((struct TM2Settings *)0)->level == 0x18) ? 1 : -1];
typedef char tm2_settings_layout_at[((unsigned int)&((struct TM2Settings *)0)->control_layout == 0x26) ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2Settings sessionSettings;
#if TM2_MAX_HUMANS > TM2_RETAIL_HUMANS
/* Port variant (players.h): the car-select indices of players 3..TM2_MAX_HUMANS
 * live in this port table (port_variant_arrays.c, .port_bss), not in the record,
 * where human_car[2..3] would be opponent_car[0..1] (FOUR_HUMANS.md 5.2). It
 * belongs to sessionSettings (the only record the session code uses). */
extern signed char sessionPortHumanCars[TM2_MAX_HUMANS - TM2_RETAIL_HUMANS];
/* human i's car-select index as an lvalue: the record for 0..1, the port table after */
#define TM2_SETTINGS_HUMAN_CAR(s, i) (*((i) < TM2_RETAIL_HUMANS ? &(s)->human_car[i]                                         : &sessionPortHumanCars[(i) - TM2_RETAIL_HUMANS]))
#else
#define TM2_SETTINGS_HUMAN_CAR(s, i) ((s)->human_car[i])
#endif
#if TM2_MAX_AI_CARS > TM2_RETAIL_OPPONENT_SLOTS
/* Port variant with TM2_PORT_CARS (players.h; PC port request 32 step 2, CAR_GROUPS.md): the
 * car-select indices of opponent slots 9..TM2_MAX_AI_CARS-1 live in this port table
 * (port_variant_arrays.c, .port_bss); the record keeps slots 0..8 (its slot 9 stays unused, and
 * lives[] after it). The port fills it before sessionBindCars. */
extern signed char sessionPortOpponentCars[TM2_MAX_AI_CARS - TM2_RETAIL_OPPONENT_SLOTS];
/* opponent i's car-select index as an lvalue: the record for 0..8, the port table after */
#define TM2_SETTINGS_OPPONENT_CAR(s, i) (*((i) < TM2_RETAIL_OPPONENT_SLOTS ? &(s)->opponent_car[i] \
                                         : &sessionPortOpponentCars[(i) - TM2_RETAIL_OPPONENT_SLOTS]))
#else
#define TM2_SETTINGS_OPPONENT_CAR(s, i) ((s)->opponent_car[i])
#endif
#ifdef __cplusplus
}
#endif

/* A car number as the getters return it (car_numbers.h): retail's signed byte; an int with TM2_PORT_CAR_NUMBERS
 * (request 42 item 3: 16-bit numbers, mods 0x20..0x9F, no sign extension). */
#ifdef TM2_PORT_CAR_NUMBERS
typedef int TM2CarNumber;
#else
typedef signed char TM2CarNumber;
#endif

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2Settings;
#ifdef __cplusplus
extern "C" {
#endif

/* game_setting_accessors.c */
TM2CarNumber sessionGetHumanCar(struct TM2Settings *settings, int player);

/* game_settings_extra.c */
TM2CarNumber settingsGetHumanCar(struct TM2Settings *settings, int player);

/* menu_state_accessors.c */
unsigned char settingsGetControlLayout(struct TM2Settings *settings, unsigned char player);
unsigned char settingsGetDifficulty(struct TM2Settings *settings);
unsigned char settingsGetWorldMap(struct TM2Settings *settings);
void settingsSetCarSelectPlayer(struct TM2Settings *settings, int v);
void settingsSetHumanCar(struct TM2Settings *settings, int player, int car);
void settingsSetHumans(struct TM2Settings *settings, unsigned char v);
void settingsSetOpponentCar(struct TM2Settings *settings, int slot, int car);
void settingsSetOpponents(struct TM2Settings *settings, int v);
void settingsSetOpponentsPicked(struct TM2Settings *settings, int v);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#ifdef TM2_PORT_CAR_NUMBERS
#include "semantic/car_numbers.h"   /* request 42 item 3: the 16-bit numbers behind the getters and setters */
#endif

#endif
