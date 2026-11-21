/* A player's Dark Tooth (PC port request 36, docs/DARK_TOOTH_PLAYER.md).
 *
 * Retail lets the computer drive Dark Tooth only (the Survival / tournament boss: aiDarkToothBossUpdate). With
 * -DTM2_PORT_DT_PLAYER (the port's `--port-define TM2_PORT_DT_PLAYER`; needs TM2_PORT_DARK_TOOTH and
 * TM2_PORT_CAR_GROUPS) a human record can drive him (car-select index 14 = type 0xF0) and plays him as close to
 * the boss as a player can:
 *   - the machine-gun button fires the boss's missiles (weaponFireDarkToothMissile: type 61, 5 damage, the spiral
 *     model, alternating left / right from the boss's front launch points) every portDtPlayer.fire_rate updates
 *     (0 = 10, the boss's quickest) instead of his machine guns;
 *   - the special is the boss's head swoop at the nearest car (human or AI) within the boss's range (4000 world
 *     units, Chebyshev): 7 damage, the target set burning; refunded when no car is in range;
 *   - his own head (asset 249, his car key's darkToothBossObject) hovers 240 above the truck, placed exactly from
 *     the car each frame, with the boss's sway; no boss controller, no stage 2, a normal final wreck (state 4);
 *   - request 118: with portDtPlayer.phase2 bit h, human h's Dark Tooth is his second phase from every start (the
 *     stage flag of his key, carStatsDarkToothStage2, the truck replaced by a hit-only model with the head's box so
 *     he still bumps and is bumped, the head at -24 on the car, a car's camera; docs/DARK_TOOTH_PLAYER.md 12);
 *   - camera offsets for his size, missiles' +25 hit radius against him, texture slot stride 32, the Hong Kong
 *     boss markers left alone, no NULL bio picture / ending movie / wrong password for index 14.
 * Car select offers index 14 only with portCarOptions bit 8 (TM2_PORT_CAR_OPT_DT_PLAYER, read live). Without a
 * human Dark Tooth (and with bit 8 clear) every changed line runs retail's logic.
 *
 * The storage (portDtPlayer, .port_bss) is defined in every variant (port_variant_arrays.c, after
 * portCarSoundLoops); this header declares its layout for the port variant and, with the define, the port
 * functions. */
#ifndef TM2_SEMANTIC_DT_PLAYER_H
#define TM2_SEMANTIC_DT_PLAYER_H

#define TM2_PORT_CAR_OPT_DT_PLAYER 0x100    /* portCarOptions bit 8: car select offers Dark Tooth (index 14) */
#define TM2_DT_PLAYER_INDEX 14              /* his car-select index (carIndexToType: 0xF0) */
#define TM2_DT_PLAYER_FIRE_RATE 10          /* portDtPlayer.fire_rate 0: the boss's quickest shot interval */
#define TM2_DT_PLAYER_RECHARGE 360          /* portDtPlayer.recharge 0: Sweet Tooth's special recharge */
#define TM2_DT_PLAYER_MAX_SPECIALS 5        /* portDtPlayer.max_specials 0: retail's 0xF0 / Sweet Tooth's 5 */
#define TM2_DT_PLAYER_ENGINE_SOUND 0x41     /* portDtPlayer.engine_sound 0: Minion's engine (carSoundUpdate 0xE0) */

#ifdef TM2_PORT_VARIANT
/* portDtPlayer (.port_bss, config/US/port_variant_arrays.tsv group one). Offsets in bytes. The port writes the
 * first nine words (0 = the default); the C writes the rest. */
struct TM2PortDtPlayer {
    int fire_rate;              /* +0x00 updates between two missiles of the machine-gun button; 0 = 10 */
    int recharge;               /* +0x04 his recharge_period (weaponBlockInit, human only); 0 = 360 */
    int max_specials;           /* +0x08 his max_specials; 0 = 5 */
    int engine_sound;           /* +0x0C his engine sound id (carSoundUpdate, human only); 0 = Minion's 0x41
                                            (the user's choice, 2026-10-14), < 0 = none (retail's 0xF0) */
    const char *bio_picture;    /* +0x10 car select "info" for index 14: a TIM file (screenLoadImageFile's
                                            name); 0 = no picture (the info button returns at once) */
    const char *ending_movie;   /* +0x14 screenEnding for index 14: his car movie (moviePlayEnding's name);
                                            0 = none (YOUWON.STR still plays) */
    int ending_frames;          /* +0x18 its frame count */
    int ending_wide;            /* +0x1C its wide flag (480 x 240) */
    void *carsel_model;         /* +0x20 the carousel's child 14 (a model node, already relocated and textured);
                                            0 = an empty node (the port draws him itself) */
    int carsel_count;           /* +0x24 children of the carousel this car select: 15 (or more with mod cars,
                                            request 42) when extended, 0 not (= portCarSel.count) */
    unsigned int phase2;        /* +0x28 request 118: bit h set = human h's Dark Tooth plays his second phase (the
                                            head on the car, the truck hidden, stage 2's stats) from every start;
                                            game state, the same on every machine; 0 = request 36's phase 1 */
    unsigned char blank[24];    /* +0x2C unused since request 42 (the nodes moved to portCarSel, car_numbers.h) */
    unsigned char node[80];     /* +0x44 unused since request 42 (kept for the layout) */
};
typedef char tm2_port_dt_player_size[(sizeof(struct TM2PortDtPlayer) == 0x94) ? 1 : -1];
#endif

#ifdef TM2_PORT_DT_PLAYER
#ifndef TM2_PORT_DARK_TOOTH
#error "TM2_PORT_DT_PLAYER needs TM2_PORT_DARK_TOOTH (a player's Dark Tooth uses his car key's own head and boss state)"
#endif
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_DT_PLAYER needs TM2_PORT_CAR_GROUPS"
#endif
#pragma tm2_absolute portDtPlayer
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortDtPlayer portDtPlayer;
/* A human Dark Tooth's engine sound id: portDtPlayer.engine_sound when > 0, -1 (none) when < 0, else Minion's. */
#define TM2_DT_PLAYER_ENGINE() (portDtPlayer.engine_sound > 0 ? portDtPlayer.engine_sound : \
                                portDtPlayer.engine_sound < 0 ? -1 : TM2_DT_PLAYER_ENGINE_SOUND)
/* Request 118: human h plays Dark Tooth's second phase (portDtPlayer.phase2 bit h; a car of another type ignores it). */
#define TM2_DT_PLAYER_PHASE2(h) ((unsigned int)(h) < 32 && ((portDtPlayer.phase2 >> (h)) & 1))
/* .port_text (port_variant_arrays.c) */
extern void portCarDtPlayerStart(void *human);  /* request 118: after a human's stats load (humanCarPlaceAtStart, the
                                                   swap): phase 2's stage flag, stats and look */
extern void portCarDtPlayerHead(void *human);   /* humanCarTick: his head (hover, sway, the swoop) */
extern void portCarDtPlayerGuns(void *human);   /* weaponTick: the machine-gun button fires the boss's missiles */
extern int  portCarDtPlayerSwoop(void *human);  /* weaponFireSpecial: start the head swoop; 0 = no car in range */
extern int  portCarDtPlayerShooter(int owner);  /* the missile wobble: is `owner` a human Dark Tooth's car object */
#ifdef __cplusplus
}
#endif
#endif
/* the extended car select (portCarSelExtend, portCarSelCount, portCarBioFile; request 42: also the name labels) */
#include "semantic/car_numbers.h"

#endif
