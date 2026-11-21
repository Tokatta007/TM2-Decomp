/* The local-player map and the humans' inputs (PC port request 37, docs/HUMANS16_PLAN.md 2.2, 2.4, steps H1 and
 * H2).
 *
 * A human record's index h (humanCars[h], record `player`, object id TM2_HUMAN_ID(h), every per-human array)
 * is not its screen any more. Each human has a local slot L (0..TM2_MAX_VIEWS-1: its view, camera, HUD record,
 * radar, target panel and hotkey targets on this machine) or none (-1: a remote human, driven by another machine;
 * it has no camera, view, HUD or hotkeys here). The port decides it, per machine, in portHumanLocal:
 *
 *   portHumanLocal[h]   0      default: slot h for h < TM2_MAX_VIEWS (retail's identity), remote past that
 *                       1..4   local slot 0..3 (value - 1; values above TM2_MAX_VIEWS count as remote)
 *                       < 0    remote (no slot)
 *
 * All zero (the default) is retail: humans 0..3 on slots 0..3. The port keeps the slots unique (two humans on one
 * slot would both drive its camera; the HUD shows the first). The storage is defined in every variant
 * (port_variant_arrays.c, .port_bss after portArenaLive, signed char[TM2_MAX_HUMANS]); the functions and the
 * game's use of the map exist only with -DTM2_PORT_HUMANS:
 *
 *   portHumanSlot(h)       h's local slot, or -1 (remote; also for h outside 0..TM2_MAX_HUMANS-1). Reads
 *                          portHumanLocal only (no presence test: the binding uses it before humanCarTotal counts h)
 *   portLocalHuman(L)      the human on slot L: the first h < humanCarTotal with portHumanSlot(h) == L, or -1
 *   portLocalHumanCount()  the slots 0..TM2_MAX_VIEWS-1 that have a human: the views / HUDs this machine shows
 *   portHumanRadarPlot(type, delta, angle, h)   hudRadarPlotCar on human h's radar (its slot), nothing when h is
 *                          remote; the caller's rand() test stays where it was (HUMANS16_PLAN.md 2.5.1)
 *
 * All four are pure (no global written except the radar's own record, no rand()). Nothing local-only feeds the
 * simulation: a remote human's camera bytes (record +0x006, +0x00C..+0x08B), view (+0x10C), view_last (+0x110)
 * and carViewHotkeyReleased[h] differ between machines and are presentation only (HUMANS16_PLAN.md H1 "As built").
 *
 * Inputs (step H2). A human whose local slot is 0 or 1 reads pad 0 / 1 as retail (padCheckAction, padComboHeld,
 * padAnyLatched with the slot as the port), unless portHumanOptions bit 0 (INPUT_ALL) is set; every other human
 * (remote, local slots 2-3, and everyone with INPUT_ALL) reads its own input word:
 *
 *   portHumanPad[h]        this frame's buttons of human h, one 16-bit word in padButtonsDecoded's half layout
 *                          (active high: 0 L2, 1 R2, 2 L1, 3 R1, 4 triangle, 5 circle, 6 cross, 7 square, 8 select,
 *                          9 L3, 10 R3, 11 start, 12 up, 13 right, 14 down, 15 left); written by the port before
 *                          each frame (before rtMainLoop's padUpdate / worldUpdate), one word per frame as the latch
 *   portHumanSteer[h]      the stick tilt of human h, 0..4096 (4096 = full, or a digital press); the C never reads
 *                          it: the port's analog hook around carThrottleSteer does, for every human
 *   portHumanLayout[h]     h's control layout 0..3 (padLayoutTable's); the port fills it from the match setup
 *   portHumanBindings[h]   h's action -> button table (128 entries, buttons 1..14 as padButtonHeld's), built
 *                          from the layout by portHumanSetLayout exactly as padSetLayout builds a pad's table
 *                          (padDefaultBindings, then the layout's 43 rows); sessionRun builds every human's at the
 *                          match start, portCarStartHuman at a drop-in; the port may overwrite single entries after
 *                          (its own buttons), the same on every machine
 *   portHumanLives[h]      h's lives (gameGetLives: for h >= 4 always, for every h with bit 5 LIVES)
 *   portHumanOptions       the bits TM2_HUMAN_OPT_* below; 0 = retail
 *
 *   portHumanAction(action, h)   padCheckAction for h: slot 0/1 (bit 0 clear) the retail call; else action 6 = the
 *                                word is not 0, other actions portPadButtonInWord(portHumanBindings[h][action], word)
 *   portHumanCombo(combo, h)     padComboHeld for h: slot 0/1 the retail call (slot 0 compares the whole 32-bit
 *                                latch, slot 1 pad 1's half); else the same nine compares on the 16-bit word
 *   portHumanAnyLatched(h)       padAnyLatched for h: slot 0/1 the retail call; else the word is not 0
 *   portPadButtonInWord(b, w)    padButtonHeld's button test (1..14) on one word; other values 0
 *   portHumanSetLayout(h, layout)   writes portHumanLayout[h] and builds portHumanBindings[h] (layout >= 4 ignored,
 *                                as padSetLayout)
 *   portHumanCheatsOff()         portHumanOptions bit 3: humanCarTick's cheat combos and carReadMoveCodes' three
 *                                cheat codes do nothing
 *
 * All of them are pure (portHumanSetLayout writes only h's layout and bindings; padCheckAction's own action cache
 * on the retail path is retail's), none calls rand(). Lockstep: the words, the layouts / bindings and the option
 * bits are the same on every machine; online the port sets INPUT_ALL (no human reads a local pad) and NO_CHEATS.
 *
 * Sound sides and local feedback (step H3, request 37 answers 7 and 12). Retail plays a human's turbo (0), gun (9)
 * and burn / lava (32) loops in one of two fixed lists, side 0 or 1, by the human's index; with the map the side
 * is the local slot when that is 0 or 1. A human's own feedback (HUD messages, jingles, the denied sound, the
 * full-volume sounds retail plays for "the player") is heard only on a machine where the human is local; a
 * remote human's car sounds as a computer car's (positional), or not at all where retail plays nothing for a
 * computer car:
 *
 *   portHumanSide(h)       h's sound side: its local slot when that is 0 or 1, else -1 (no side)
 *   portHumanFeedback(h)   1 when h is local on this machine (portHumanSlot(h) >= 0), else 0
 *   portHumanMessage(h, m, p)        hudMessagePost(m, p) when h is local
 *   portHumanSound(h, s, v, pa, pi)  soundPlay(s, v, pa, pi) when h is local
 *   portHumanDenied(h)               soundSetDeniedPending(1) when h is local
 *   portHumanLoopPlay / Stop         (car_sound.h, also built with TM2_PORT_HUMANS alone) the loops by side;
 *                                    a human without a side: its own instance (TM2_PORT_CAR_SOUND; a remote
 *                                    human's positional), a remote human's gun in the computer cars' shared loop
 *
 * The game sites use the macros below (retail's own calls without the define). The listener (sndListenerTransform
 * with view -1) also takes views 2..3 when a local human is on them, and portCarSoundFrame's engine cap ranks
 * the cars by their distance to the local humans. None of this writes simulation state or reaches rand().
 *
 * Rules (step H5, request 37 answers 8 and 9). Retail decides the round on fixed humans 0/1: any human's death
 * ends the round (humanCarDamageUpdate: event 1, or in deathmatch 4 for human 0 and 3 for the other), the round is
 * won when the AI cars are gone and human 0 is alive (worldUpdate), a lost round takes the team's life from
 * lives[0] (sessionRun), the result screens tally players 1/2, co-op's lives show on HUD 0, the target panel
 * ignores bots whenever two humans play. With portHumanOptions bit 1 (RULES) the port's battle modes own all of
 * that: the C ends no round and decides no result on its own; a human who dies stays down (its wreck, nothing
 * restarts it) until the port starts it again:
 *
 *   portHumanRules()          1 with bit 1 outside the attract demo (the gate at every rule site)
 *   portHumanAlive(h)         h is present (h < humanCarTotal, bound to a car), not destroyed, HP > 0
 *   portHumanDown(h)          h is present and destroyed (dead flag, or HP <= 0): with bit 1 it waits for the port
 *   portHumanAliveCount()     the humans alive; portAiAliveCount(): the computer cars alive (not vacant)
 *   portRoundEnd(event, delay, winner, flags)   the round over trigger: roundOverFlag, the event in `delay` ticks
 *                             (rtScheduleEvent: rtMainLoop returns `event` to sessionRun), portRound filled; flag
 *                             TM2_ROUND_FREEZE also does retail's won-round stop (roundWonFlag: no car takes damage,
 *                             the computer cars' gun loops stop). 0 when a round end is pending already
 *   portRoundResults(s, result)  sessionRun's result for the port's rules (called after every round with bit 1;
 *                             the hook the port wraps for its results screen): -1 = retail's handling, 0 = next
 *                             round, 1 = the session ends (TM2_ROUND_* below)
 *   portHumanRespawn(h, node, flags)   h starts again on its own car as a drop-in start: fresh stats, full HP,
 *                             fresh weapons (TM2_HUMAN_RESPAWN_KEEP_WEAPONS: retail's lost-a-life rule), at route
 *                             node `node` or (node < 0) the start farthest from every live car; nothing else in
 *                             the match is reset (projectiles, drops, specials, HUD messages stay)
 *   gameGetLives(h)           with bit 1 portHumanLives[h] for every human (as bit 5); the split HUDs show each
 *                             slot's own human's lives in co-op
 *
 * The port calls portRoundEnd and portHumanRespawn between frames, on every machine at the same frame (they write
 * the simulation; the respawn draws rand() for the heading, as retail's start). portRound is match state (the
 * rollback snapshot keeps it). Nothing here assumes human 0 is local, alive or present.
 *
 * Hand-over and vacant records (request 50, docs/HUMANS16_PLAN.md 8). Built with TM2_PORT_HUMANS and
 * TM2_PORT_CAR_SWAP (TM2_HUMAN_HANDOVER below; TM2_PORT_CAR_DROPIN is not needed). A player who leaves mid-match
 * leaves its human record h VACANT: no car (its object is a dummy that is never drawn, collides with nothing and
 * lies far away), dead with HP 0 and wreck state 4, takes no damage, remote, never updated, never restarted, never
 * a target, not counted by portHumanAlive / Down / AliveCount and not bound by sessionBindCars. The other human
 * records keep their indices (no compaction):
 *
 *   portHumanHandOver(h, j)   human h's car, as it is (place, motion, HP, weapons, ammo, energy, its slot, so
 *                             its team colours and every reference by slot), goes to AI record j (-1 = a free
 *                             one: with TM2_PORT_CAR_DROPIN the lowest vacant AI record, else a new record
 *                             aiCarTotal) as a computer car from the next frame: fresh AI state (route, target =
 *                             the nearest human, ranges, controls, stuck counters), the AI difficulty (speed scale,
 *                             weights, fire chance), AI stats of the type (max HP without the human's +20: HP kept,
 *                             clamped to it), object id j + 50 (its projectiles, mines, arcs, ricochets and bullets
 *                             in flight change owner with it); a special or look in progress ends as in a swap.
 *                             Human h is left vacant. Returns j, or an error below
 *   portHumanLeave(h)         human h leaves without a successor: its car parked (a free car of its group, as
 *                             portCarRemove leaves it; with TM2_PORT_CAR_DROPIN portCarStartAi can start a new
 *                             computer car on it: request 50 (b)), its weapons in the air removed, h vacant
 *   portHumanReturn(h, j)     the reverse: vacant human h takes AI j's car back as it is (a rejoining player);
 *                             AI record j leaves the match (the last AI record: aiCarTotal - 1; any record with
 *                             TM2_PORT_CAR_DROPIN: vacant). portHumanRespawn(h, node, 0) after it starts fresh
 *   portHumanRejoin(h, group, node)   a joining player takes vacant record h back in place (no renumbering) on
 *                             the free car of load group `group` (a parked car: portHumanLeave, portCarRemove), as
 *                             a fresh start at route node `node` (< 0: the farthest free start); with
 *                             TM2_PORT_CAR_DROPIN portCarStartHumanAt(h, group, flags, node) does the same for a
 *                             vacant h. portHumanFreeGroupOfType(type): a free car's group, 0 = none
 *   portHumanVacantCount()    the vacant human records
 *
 * The vacancy lasts until the next load (carTablesClear). Between frames, on every machine at the same step; no
 * rand() of their own (ending a special may step an animation, as a swap does). State: portHumanVacant (.port_bss every variant, in the rollback snapshot). */
#ifndef TM2_SEMANTIC_PORT_HUMANS_H
#define TM2_SEMANTIC_PORT_HUMANS_H

#include "semantic/players.h"

/* portHumanOptions bits (0 = retail) */
#define TM2_HUMAN_OPT_INPUT_ALL 0x01   /* bit 0: every human reads portHumanPad, local slots 0/1 too (netplay) */
#define TM2_HUMAN_OPT_RULES     0x02   /* bit 1: the port decides round end and results (step H5): no death ends
                                          the round, no won test on human 0, no lives[0], no 2-player result
                                          screens; a dead human waits for portHumanRespawn */
#define TM2_HUMAN_OPT_AI_AIM    0x04   /* bit 2: a bot's aimed shot (weaponFindTargetAhead / Behind) goes at its own
                                          target (aiGetTargetCar's car), not human 0 */
#define TM2_HUMAN_OPT_NO_CHEATS 0x08   /* bit 3: the cheat combos (humanCarTick) and cheat codes (carReadMoveCodes:
                                          0x12233344, 0x21431122, 0x12344321) off; the port sets it whenever any
                                          human is remote */
#define TM2_HUMAN_OPT_RADAR_AI  0x10   /* bit 4: the AI blips on every local human's radar in every game mode (retail:
                                          human 0's radar, plus the others' in co-op, mode 2) */
#define TM2_HUMAN_OPT_LIVES     0x20   /* bit 5: gameGetLives reads portHumanLives for every human (clear: settings
                                          lives[0..3] for humans 0..3, retail; portHumanLives past them) */

#define TM2_HUMAN_BINDINGS 128         /* actions per binding table (padActiveBindings' rows) */

/* portHumanControl[h] bits (request 66, ANALOG_STEERING.md 2.6; 0 = retail): human h's own control options, set by
 * the port from each player's Controls option, match state (the same on every machine: snapshot and hash) */
#define TM2_HUMAN_CTL_STEER_KEEP 0x01  /* bit 0: the turn keeps its sign in reverse (Left turns the nose left, rolling
                                          as in place); carThrottleSteer's rolling yaw rate and carRicochetOffObject's
                                          deflection target as going forward (retail flips both with the speed) */

/* H5: round events (rtScheduleEvent values, what rtMainLoop returns to sessionRun) */
#define TM2_ROUND_LOST      1          /* retail: a human died (tournament: a life lost) */
#define TM2_ROUND_QUIT      2          /* retail: quit from the pause menu */
#define TM2_ROUND_WON       3          /* retail: the computer cars are gone (deathmatch: player 2 died) */
#define TM2_ROUND_LOST_P1   4          /* retail deathmatch: player 1 died */
#define TM2_ROUND_GAME_OVER 5          /* bit 1: GAME OVER and the session ends (retail's last life) */
#define TM2_ROUND_PORT      6          /* bit 1: this value and above are the port's own (next round) */
#define TM2_ROUND_FREEZE    1          /* portRoundEnd flag: retail's won-round stop (roundWonFlag) */
#define TM2_HUMAN_RESPAWN_KEEP_WEAPONS 1   /* portHumanRespawn flag: carRestart's reset_weapons = 0 (2 specials,
                                              full energy, the rest kept) instead of fresh weapons */
#define TM2_HUMAN_ERR_RECORD (-1)      /* portHumanRespawn: no such present human bound to a car */
#define TM2_HUMAN_ERR_BUSY   (-9)      /* portHumanRespawn: attract demo */
#define TM2_HUMAN_ERR_NODE   (-11)     /* portHumanRespawn: node past the arena's route nodes */
#define TM2_HUMAN_ERR_FULL   (-3)      /* request 50: the vacant slot is a car's (carWorldObjectCount) */
#define TM2_HUMAN_ERR_INDEX  (-4)      /* request 50: no such free AI record (hand-over) / AI record that may leave */
#define TM2_HUMAN_ERR_LAST   (-5)      /* request 50: the last present human cannot leave */
#define TM2_HUMAN_ERR_DOWN   (-13)     /* request 50: the car is destroyed (respawn it first, or portHumanLeave) */

#if defined(TM2_PORT_HUMANS) && defined(TM2_PORT_CAR_SWAP)
#define TM2_HUMAN_HANDOVER 1           /* request 50: the hand-over and vacant human records */
#endif
#define TM2_PORT_VACANT_HUMANS 16      /* portHumanVacant.ai entries (>= TM2_MAX_HUMANS of any variant) */
#define TM2_PORT_VACANT_OBJECT 0x128   /* a scene object (TM2Object) */

#ifdef TM2_PORT_VARIANT
/* portRound (.port_bss every variant, config/US/port_variant_arrays.tsv group one): the port's last round end
 * (portRoundEnd), match state; read it in the results (portRoundResults). Offsets in bytes. */
struct TM2PortRound {
    int event;          /* +0x0 the event of the last portRoundEnd (0 = none yet) */
    int delay;          /* +0x4 its delay (ticks) */
    int winner;         /* +0x8 its winner as the port gave it (the C never reads it: a human, a team, -1) */
    int start_node;     /* +0xC internal: the start a respawn / drop-in forces (node + 1, -1 = the farthest
                           from the live cars), 0 outside those calls */
};
typedef char tm2_port_round_size[(sizeof(struct TM2PortRound) == 0x10) ? 1 : -1];
/* portHumanVacant (.port_bss every variant, config/US/port_variant_arrays.tsv group one, last): the vacant human
 * records (request 50), match state written by the C (in the rollback snapshot and hash); cleared at each load.
 * The port may read it. Offsets in bytes. */
struct TM2PortHumanVacant {
    unsigned int mask;                                 /* +0x000 bit h: human record h vacant */
    int handed;                                        /* +0x004 hand-overs in this load */
    int left;                                          /* +0x008 portHumanLeave calls in this load */
    int returned;                                      /* +0x00C portHumanReturn calls in this load */
    int rejoined;                                      /* +0x010 portHumanRejoin calls in this load (request 50b) */
    signed char ai[TM2_PORT_VACANT_HUMANS];            /* +0x014 the AI record + 1 that took human h's car, 0 =
                                                          none (it left, or it came back) */
    unsigned char dummy[TM2_PORT_VACANT_OBJECT];       /* +0x024 the object a vacant record points at */
};
typedef char tm2_port_human_vacant_size[(sizeof(struct TM2PortHumanVacant) == 0x14C) ? 1 : -1];
#pragma tm2_absolute portRound
#pragma tm2_absolute portHumanVacant
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortRound portRound;
extern struct TM2PortHumanVacant portHumanVacant;
#ifdef __cplusplus
}
#endif
#endif

/* Request 50: the game's gates for vacant human records (HUMANS16_PLAN.md 8.3). Without TM2_HUMAN_HANDOVER they are
 * nothing (retail text). */
#ifdef TM2_HUMAN_HANDOVER
#ifdef __cplusplus
extern "C" {
#endif
extern int portHumanHandOver(int h, int j);
extern int portHumanLeave(int h);
extern int portHumanReturn(int h, int j);
extern int portHumanRejoin(int h, int group, int node);
extern int portHumanFreeGroupOfType(int type);
extern int portHumanVacantCount(void);
extern int portHumanFirstPresent(void);
extern int portHumanBindAiCount(int count);
extern void portHumanVacantReset(void);
#ifdef __cplusplus
}
#endif
#define TM2_HUMAN_VACANT(h) ((portHumanVacant.mask >> (h)) & 1)
#define TM2_HUMAN_SKIP_VACANT(h) if (TM2_HUMAN_VACANT(h)) continue
#define TM2_HUMAN_NEAREST_FIRST portHumanFirstPresent()        /* humanCarNearest's default: not a vacant record */
#define TM2_HUMAN_BIND_AI_COUNT(n) portHumanBindAiCount(n)     /* sessionBindCars: the AI records a hand-over added */
#else
#define TM2_HUMAN_VACANT(h) 0
#define TM2_HUMAN_SKIP_VACANT(h)
#define TM2_HUMAN_NEAREST_FIRST 0
#define TM2_HUMAN_BIND_AI_COUNT(n) (n)
#endif

#ifdef TM2_PORT_HUMANS
#if TM2_MAX_HUDS != TM2_MAX_VIEWS
#error "the local map gives one slot for a view and its HUD (TM2_MAX_HUDS == TM2_MAX_VIEWS)"
#endif
#pragma tm2_absolute portHumanLocal
#pragma tm2_absolute portHumanOptions
#pragma tm2_absolute portHumanPad
#pragma tm2_absolute portHumanSteer
#pragma tm2_absolute portHumanLayout
#pragma tm2_absolute portHumanLives
#pragma tm2_absolute portHumanBindings
#pragma tm2_absolute portHumanRadarBlink
#pragma tm2_absolute portHumanControl
#ifdef __cplusplus
extern "C" {
#endif
extern signed char portHumanLocal[TM2_MAX_HUMANS];
extern int portHumanOptions;
extern unsigned short portHumanPad[TM2_MAX_HUMANS];
extern short portHumanSteer[TM2_MAX_HUMANS];
extern unsigned char portHumanLayout[TM2_MAX_HUMANS];
extern unsigned char portHumanLives[TM2_MAX_HUMANS];
extern unsigned char portHumanBindings[TM2_MAX_HUMANS][TM2_HUMAN_BINDINGS];
extern unsigned int portHumanRadarBlink;   /* bit 4's blink clock (presentation only; worldUpdate) */
extern unsigned char portHumanControl[TM2_MAX_HUMANS];   /* request 66: TM2_HUMAN_CTL_* per human (match state) */
extern int portHumanSteerKeep(void *car, int human);    /* request 66: 1 = the record's turn keeps its sign in reverse
                                                            (a human record whose portHumanControl has bit 0) */
extern int portHumanSlot(int h);
extern int portLocalHuman(int L);
extern int portLocalHumanCount(void);
extern void portHumanRadarPlot(void *type, int *delta, int angle, int h);
extern int portPadButtonInWord(int button, unsigned int word);
extern unsigned char portHumanAction(int action, int h);
extern int portHumanCombo(int combo, int h);
extern int portHumanAnyLatched(int h);
extern void portHumanSetLayout(int h, int layout);
extern void portHumanSetLayouts(void);
extern int portHumanCheatsOff(void);
extern int portHumanCheatCode(int code);
extern int *portAiAimTarget(int owner);
extern int portHumanSide(int h);
extern int portHumanFeedback(int h);
extern void portHumanMessage(int h, int msg, int prio);
extern void portHumanSound(int h, int snd, int vol, int pan, int pitch);
extern void portHumanDenied(int h);
extern void portHumanLoopPlay(int snd, int vol, int pan, int pitch, int player, int slot);
extern void portHumanLoopStop(int snd, int player, int slot);
struct TM2Settings;
extern int portHumanRules(void);
extern int portHumanAlive(int h);
extern int portHumanDown(int h);
extern int portHumanAliveCount(void);
extern int portAiAliveCount(void);
extern int portRoundEnd(int event, int delay, int winner, int flags);
extern int portRoundResults(struct TM2Settings *s, int result);
extern int portHumanRespawn(int h, int node, int flags);
extern void *portHumanNearestCar(int h, unsigned char *human);
#ifdef __cplusplus
}
#endif
#define TM2_HUMAN_LOCAL(h) portHumanSlot(h)     /* human index -> local slot, -1 remote */
#define TM2_LOCAL_HUMAN(L) portLocalHuman(L)    /* local slot -> human index, -1 none */
/* H3: a human's own feedback, heard / shown only where it is local */
#define TM2_HUMAN_FEEDBACK(h) portHumanFeedback(h)
#define TM2_CAR_FEEDBACK(human, h) ((human) && portHumanFeedback(h))   /* a human record whose human is local */
#define TM2_CAR_POSITIONAL(human, ai) ((human) || (ai))   /* the AI's positional branch, a remote human's too */
#define TM2_HUMAN_MESSAGE(h, msg, prio) portHumanMessage((h), (msg), (prio))
#define TM2_HUMAN_SOUND(h, snd, vol, pan, pitch) portHumanSound((h), (snd), (vol), (pan), (pitch))
#define TM2_HUMAN_DENIED(h) portHumanDenied(h)
#else
#define TM2_HUMAN_LOCAL(h) (h)                  /* retail: human i is view / HUD i */
#define TM2_LOCAL_HUMAN(L) (L)
#define TM2_HUMAN_FEEDBACK(h) 1                 /* retail: every human is this machine's */
#define TM2_CAR_FEEDBACK(human, h) (human)
#define TM2_CAR_POSITIONAL(human, ai) (ai)
#define TM2_HUMAN_MESSAGE(h, msg, prio) hudMessagePost((msg), (prio))
#define TM2_HUMAN_SOUND(h, snd, vol, pan, pitch) soundPlay((snd), (vol), (pan), (pitch))
#define TM2_HUMAN_DENIED(h) soundSetDeniedPending(1)
#endif
#define TM2_PORT_SHARED_GUN(slot) (0x100 + (slot))   /* a remote human's instance in the computer cars' shared gun
                                                        loop (their instances are AI indices 0..15) */

#endif
