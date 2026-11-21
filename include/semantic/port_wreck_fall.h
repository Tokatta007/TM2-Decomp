/* PC port request 117 (docs/PLAYTEST_ANSWERS.md 33, RETAIL_BUGS.md 44): a wreck that falls is hidden with its splash, or
 * explodes on the floor, as a car that dies of the fall.
 *
 * Retail decides a car's end once, in carDestroyed at the moment of its death, from where the car is then:
 *   - Antarctica (level 6) below height 240: the car is hidden and splashes (fxSplashAt, a human's sound 34);
 *     level 8 below -40: hidden;
 *   - anywhere else: it explodes where it is.
 * A car destroyed in the air (a weapon's kill as it goes over the edge) explodes in the air, and its wreck then falls
 * to the floor and lies there, never hidden. A human's car destroyed after the round is won (roundWonFlag; with the
 * port's rules, the round end with TM2_ROUND_FREEZE) gets nothing at all: carDestroyed returns at once.
 *
 * With TM2_PORT_WRECK_FALL humanCarTick and aiCarDrive call portWreckFall after their fall test, every frame:
 *   - a wreck (state.dead) still drawn on Antarctica below 240 (level 8: below -40) is hidden as carDestroyed hides a
 *     car there: drawing off, the splash and a human's sound 34 (Antarctica), the wreck timer at most 0;
 *   - a falling wreck whose pit test passes (state.falling and FALL_Z < worldFallDeathZ, the fall death's own test)
 *     and that has not exploded there explodes as carDestroyed explodes a car: the smoke, the spin out, the two
 *     explosions and the wreck sound (once; carDestroyed records whether it blew the car up there:
 *     portWreckFallBlown);
 *   - a live car: nothing (its mark cleared).
 * A car not drawn (hidden already, or parked by the port) is left alone. Same frame on every machine: the mark is
 * match state (.port_bss, snapshot and hash). No rand() of its own: the effects and the spin out draw on the game's,
 * as carDestroyed's do. */
#ifndef TM2_PORT_WRECK_FALL_H
#define TM2_PORT_WRECK_FALL_H

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
/* .port_bss, every variant: per car slot, 1 = the wreck has exploded at the floor (cleared while the car lives) */
extern unsigned char portWreckFallMark[];
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_WRECK_FALL
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_WRECK_FALL needs the port variant (build_source_only.py --port-variant)"
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c); car = a human record (human 1) or an AI record (human 0) */
extern void portWreckFall(void *car, int human);
/* carDestroyed's explosion: the car's mark = 1 when it blew up at the floor (falling, past the pit test) */
extern void portWreckFallBlown(void *car, int human);
#ifdef __cplusplus
}
#endif
#endif

#endif
