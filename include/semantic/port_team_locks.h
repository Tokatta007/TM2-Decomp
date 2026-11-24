/* A team game's locks that do not go through the aimed-shot search (PC port request 123 item 3,
 * docs/PLAYTEST_ANSWERS.md 37.3).
 *
 * A computer car's own target at -1 (aiGetTargetCar 8011CF0C: "the nearest player", humanCarNearest) is a player of
 * any team: where it drives, and weaponFireSelected's melee check that forces the machine guns at a car within 400
 * ahead. With TM2_PORT_TEAM_LOCKS it is the nearest player not of its team (portTeamNearestFoe), and so is a
 * computer Dark Tooth's swoop at -1 (with TM2_PORT_DT_SWOOP).
 *
 * Every steering shot (the fire, homing and napalm missiles, the freeze and napalm energy shots, the specials of
 * Sweet Tooth, Warthog, Mr. Grimm, Spectre, Roadkill and Minion, Dark Tooth's missiles) takes its target once, at
 * the fire, from weaponFindTargetAhead / Behind (the port's portAiAim for a computer car), and keeps it: no
 * projectile picks a new target in flight (weaponProjectileUpdate steers at p->target or drops it; only Roadkill's
 * boomerang turns back, to its own thrower). Two locks pick their own car, the nearest in reach, with no team test:
 *   - Grasshopper's homing leap (weaponHomingLeapInit 8010EF44): the nearest car within 400 ahead (a 682 cone);
 *   - Outlaw's taser (weaponTaserTarget 801075C8): the nearest car within 80 (a mod car's range), stunned (spin 6).
 *
 * With TM2_PORT_TEAM_LOCKS both pass a candidate of the firer's team: portCarTeam[slot] (per car slot, .port_bss,
 * every variant) holds each car's team as the port sets it, 0 = none (no team: every car a foe, retail). Two cars are
 * teammates when both are nonzero and equal. All zero (the default, a game without teams) is retail's pick. No
 * rand(); the same on every machine (the table is match state the port writes from its settings).
 *
 *   portTeamMates(a, b)   1 when car slots a and b are on one team (both nonzero, equal), else 0 (slots out of range
 *                         too)
 *   portTeamNearestFoe(k)  computer car k's player at -1: the nearest player (retail's |dx| + |dy|, retail's first
 *                         present one beyond 0x7FFF) not of its team; none: retail's humanCarNearest */
#ifndef TM2_PORT_TEAM_LOCKS_H
#define TM2_PORT_TEAM_LOCKS_H

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
/* .port_bss, every variant: per car slot (TM2_MAX_CAR_SLOTS), the car's team, 0 = none */
extern unsigned char portCarTeam[];
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_TEAM_LOCKS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_TEAM_LOCKS needs the port variant (build_source_only.py --port-variant)"
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern int portTeamMates(int a, int b);
extern int portTeamNearestFoe(int k);
#ifdef __cplusplus
}
#endif
/* a lock's candidate in slot b for the firer in slot a: not a teammate */
#define TM2_TEAM_FOE(a, b) (!portTeamMates((a), (b)))
#else
#define TM2_TEAM_FOE(a, b) 1
#endif

#endif
