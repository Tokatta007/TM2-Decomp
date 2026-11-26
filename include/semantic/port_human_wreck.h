/* PC port request 126 (docs/PLAYTEST_ANSWERS.md 40): a destroyed player's wreck lives a computer car's wreck's life.
 *
 * Retail takes a destroyed car out of play only by breaking it up (carApplyDamage's heavy-hit branch: debris, wreck
 * state 4, its object's word +8 cleared). A computer car's wreck lives on in carWreckUpdate (AI_LIVES.md 1.2): it burns
 * (the computer-car branch: fxFlame10 with 500 of life, burn_time held at 0), spins about 2 s (wreck timer 120 > 0),
 * one small blast at timer < -960, and is blown apart at timer < -1200, 22 s after the death (carApplyDamage(id, 15,
 * 0, 2, 0), the blast and smoke, the flame out), or sooner when its target is 4320 away. A player's wreck gets the
 * spin only: the flame, the small blast and the break-up are the computer cars'. So after any death but a heavy
 * killing hit (type 1 or 2, 6 or more) a player's wreck stays in play for good: solid to every car, a stop for every
 * shot, and a live car touching it takes ram damage credited to it. In retail a player's death ended the round.
 *
 * With TM2_PORT_HUMAN_WRECK, for a player's car still in play at its death (not broken up, sunk or fallen):
 *   - humanCarDamageUpdate, at the death (after carDestroyed): portHumanWreckStart shows the car's body object again
 *     (the shadow blob, node 3, that carBodyObjectHide removed), clears the small-blast flag and marks the wreck;
 *   - carWreckUpdate (every step the record is dead, whatever the player's view): while marked, the player's wreck
 *     takes the computer cars' flame, small blast and break-up, by the wreck timer only (a player's car has no
 *     target): blown apart 22 s after the death (portHumanWreckBreak: the computer cars' break-up, the mark cleared);
 *   - humanCarDamageUpdate, each step (portHumanWreckTick): HP above 0 (a respawn) or the car hidden by anything else
 *     (+8 = 0) clears the mark and puts the wreck's flame out.
 * A player out of lives gets the whole life too. The respawn puts the car back as now: csSetDrawEnable(object, 1) and,
 * once its HP is full, humanCarDamageUpdate's own reset (damage stage 0, body shown, wreck state 0). The mark is match
 * state (.port_bss: snapshot and hash); the timer is the record's; the flame, debris and blasts draw on the game's
 * rand() as a computer car's do: the same on every machine. */
#ifndef TM2_PORT_HUMAN_WRECK_H
#define TM2_PORT_HUMAN_WRECK_H

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
/* .port_bss, every variant */
extern int portHumanWreckLive[];    /* [TM2_MAX_HUMANS] 1 = human h's wreck lives a computer car's wreck's life */
#ifdef TM2_PORT_HUMAN_WRECK
extern void portHumanWreckStart(int i, int key);    /* humanCarDamageUpdate, at human i's death */
extern void portHumanWreckTick(int i);              /* humanCarDamageUpdate, each step */
extern int  portHumanWreckBurns(void *car);         /* carWreckUpdate: 1 = this player's wreck is marked */
extern void portHumanWreckBreak(void *car);         /* carWreckUpdate, timer < -1200: the computer cars' break-up */
#endif
#ifdef __cplusplus
}
#endif
#endif

#endif
