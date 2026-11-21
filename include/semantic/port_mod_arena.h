/* PC port request 114 (docs/MOD_ARENAS.md): two of the level's rules as data for a mod arena.
 *
 * The port plays a mod arena as level 13 with gameGetLevel answering 7 (Holland), so every per-level rule takes
 * Holland's. Two of them are places on Holland's map; with TM2_PORT_MOD_ARENA they come from portArenaRules when the
 * port sets them:
 *   heal   carTick's hidden full-heal spot (car_tick.c): 0 retail (the level's square), 1 the 64 x 64 square whose
 *          low corner is (healX, healZ) (car motion pos[0], pos[1]), 2 none.
 *   warps  aiWarpSetExitNode's route nodes for a computer car leaving a teleporter (fx_debris_splash.c): 0 retail, 1
 *          exit[side] = {spawn node, target node}; a spawn node below 0, or a side past 2, leaves the car's nodes as
 *          they are and does not place it (retail's "no exit" for that level and side).
 * The port writes them when it loads an arena (all 0 for a retail one); nothing in the game writes them. They are
 * the arena's data, constant through a match, as the arena's files are. */
#ifndef TM2_PORT_MOD_ARENA_H
#define TM2_PORT_MOD_ARENA_H

#define TM2_MOD_HEAL_RETAIL 0
#define TM2_MOD_HEAL_SPOT   1
#define TM2_MOD_HEAL_NONE   2
#define TM2_MOD_WARP_SIDES  3

typedef struct {
    int heal;                                   /* +0x00 TM2_MOD_HEAL_* */
    int healX;                                  /* +0x04 the square's low corner, pos[0] */
    int healZ;                                  /* +0x08 the square's low corner, pos[1] */
    int warps;                                  /* +0x0C 0 retail, 1 exit[] */
    short exit[TM2_MOD_WARP_SIDES][2];          /* +0x10 per side: spawn node, target node */
} TM2PortArenaRules;                            /* 0x1C bytes */

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
extern TM2PortArenaRules portArenaRules;        /* .port_bss, written by the port; all 0 = retail */
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_MOD_ARENA
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_MOD_ARENA needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portArenaRules
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c); no rand() */
extern int portArenaHealSpot(int *x, int *z);   /* carTick: TM2_MOD_HEAL_* from portArenaRules.heal; with
                                                   TM2_MOD_HEAL_SPOT *x, *z = the square's low corner */
#ifdef __cplusplus
}
#endif
#endif

#endif
