/* A computer Dark Tooth's swoop at the car he is after (PC port request 121 item 1, docs/PLAYTEST_ANSWERS.md 35).
 *
 * Retail's swoop (aiDarkToothBossUpdate, ai_dark_tooth_boss.c) goes at the nearest player only
 * (humanCarNearest), and its hit (7 damage, the car set burning) only on a human record. In the boss fight that is
 * all there is; in a match with computer cars (TM2_PORT_DARK_TOOTH's ordinary Dark Tooth, a Challenge Match) he
 * never swoops at the car he chases.
 *
 * With TM2_PORT_DT_SWOOP the swoop takes his own target (aiCarTargets[his index]): a computer car when that is
 * one (alive, not him), else, at -1, the nearest player as retail. The target is kept as a code in
 * aiDarkToothSwoopTarget: a human index (retail's values), or 0x100 + a computer car's index. The hit does
 * retail's damage and burn on whichever record the code names, in retail's order; a computer car that leaves the
 * match (its record vacated or gone) ends the swoop. No rand() of its own: the effects draw as retail's do. The
 * boss fight (no computer car but him: his target -1) runs exactly as retail.
 *
 *   portDtSwoopPick(boss)     the target code at the swoop's start
 *   portDtSwoopPos(code)      its car's motion.pos (darkToothSwoopTarget)
 *   portDtSwoopLive(code)     0 = a computer car target is gone (end the swoop); 1 for players (retail)
 *   portDtSwoopHit(code, slot)  the hit's damage and burn, from his car slot */
#ifndef TM2_PORT_DT_SWOOP_H
#define TM2_PORT_DT_SWOOP_H

#ifdef TM2_PORT_DT_SWOOP
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_DT_SWOOP needs the port variant (build_source_only.py --port-variant)"
#endif
#define TM2_DT_SWOOP_AI 0x100       /* aiDarkToothSwoopTarget: 0x100 + a computer car's index */
#ifdef __cplusplus
extern "C" {
#endif
extern int portDtSwoopPick(void *boss);
extern int *portDtSwoopPos(int code);
extern int portDtSwoopLive(int code);
extern void portDtSwoopHit(int code, int slot);
#ifdef __cplusplus
}
#endif
#endif

#endif
