/* Arena hazards and destructibles: the port's off switches (PC port request 38, docs/ARENA_SWITCHES.md).
 *
 * Retail always runs its arenas' hazards (the automatic ones: Antarctica's collapsing ice, lava burn, the fire
 * volumes, Hong Kong's trains) and lets players set off the destructibles (the Los Angeles quake and timed
 * target, Moscow's bridges and dish, the Eiffel Tower, New York's 519 -> 509 event, Holland's barns, the Hong Kong
 * trains as targets). With -DTM2_PORT_HAZARDS (the port's `--port-define TM2_PORT_HAZARDS`) each of them is an
 * item that can be switched off: the arena then plays as if it never happens (ARENA_SWITCHES.md 3).
 *
 *   portArenaOptions  the port's two menu switches: bit 0 hazards off, bit 1 destructibles off; 0 = retail
 *   portArenaItems    more items off, one bit per item (TM2_ARENA_BIT below), ORed in; 0 = none. Per-arena
 *                     switches later only set these bits: no new gate is needed (ARENA_SWITCHES.md 6)
 *   portArenaLive     the items off this round, written by the C: copied at roundStart (portArenaLatch) from
 *                     the two words above, read by every gate. The port reads it, never writes it
 *
 * Both words zero: every gate passes and the retail logic runs, with the same rand() calls in the same order (the
 * gates and the latch draw none). The port should change the words only at match setup: a destroyed arena stays
 * destroyed until it is loaded again (retail resets nothing at roundStart).
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss, after portDtPlayer); this header
 * declares it for the port variant and, with the define, the port functions the gates call. */
#ifndef TM2_SEMANTIC_PORT_HAZARDS_H
#define TM2_SEMANTIC_PORT_HAZARDS_H

/* portArenaOptions bits */
#define TM2_ARENA_OPT_HAZARDS_OFF       0x1   /* bit 0: the hazards (items 0..7) off */
#define TM2_ARENA_OPT_DESTRUCTIBLES_OFF 0x2   /* bit 1: the destructibles (items 8..15) off */

/* Items: bit numbers of portArenaItems / portArenaLive. Hazards 0..7, destructibles 8..15, others 16.. */
#define TM2_ARENA_ICE            0    /* Antarctica (6): the ice sections never collapse (worldUpdate skips
                                         worldCollapseUpdate) */
#define TM2_ARENA_LAVA           1    /* ground class 2 (Amazonia, Los Angeles section 14): no HP lost
                                         (carGroundEffects; splash and wheel effects stay) */
#define TM2_ARENA_FIRE           2    /* fire volumes, material 666 (Los Angeles 2, Moscow 3 after its bridge
                                         events): no burning (carContactApplyObject; the volume is consumed) */
#define TM2_ARENA_TRAINS         3    /* Hong Kong (8): the two trains removed (hidden, parked outside the map
                                         with their roof surfaces and targets; portArenaTrainsPark) */
#define TM2_ARENA_LA_QUAKE       8    /* Los Angeles (1): the quake, event 1 (laEventStart ignored) */
#define TM2_ARENA_LA_TIMED       9    /* Los Angeles (1): timed target 508 takes no damage */
#define TM2_ARENA_MOSCOW_BRIDGES 10   /* Moscow (2): the four bridge events (moscowEventStart ignored) */
#define TM2_ARENA_MOSCOW_DISH    11   /* Moscow (2): the dish (moscowDishDamage ignored) */
#define TM2_ARENA_EIFFEL         12   /* Paris (3): the tower never falls (eiffelTowerStartFall ignored) and the
                                         "fewer than 3 AI cars" route patch is skipped (aiCarDamageUpdate) */
#define TM2_ARENA_NY_EVENT       13   /* New York (5): targets 519 and 509 take no damage */
#define TM2_ARENA_HOLLAND_BARNS  14   /* Holland (7): the two barns, target 510, take no damage */
#define TM2_ARENA_TRAIN_TARGETS  15   /* Hong Kong (8): the trains, targets 506 / 507, take no damage */
#define TM2_ARENA_PROPS          16   /* every arena: breakable props 500..505 and 520 take no damage (not set by
                                         portArenaOptions; only through portArenaItems) */
#define TM2_ARENA_BIT(item) (1u << (item))
#define TM2_ARENA_HAZARD_ITEMS (TM2_ARENA_BIT(TM2_ARENA_ICE) | TM2_ARENA_BIT(TM2_ARENA_LAVA) \
                                | TM2_ARENA_BIT(TM2_ARENA_FIRE) | TM2_ARENA_BIT(TM2_ARENA_TRAINS))   /* 0x0000000F */
#define TM2_ARENA_DESTRUCTIBLE_ITEMS 0x0000FF00u   /* items 8..15 */

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
extern int portArenaOptions;            /* the port's switches (bits above), 0 = retail */
extern unsigned int portArenaItems;     /* more items off (TM2_ARENA_BIT), 0 = none */
extern unsigned int portArenaLive;      /* the items off this round (roundStart), written by the C */
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_HAZARDS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_HAZARDS needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portArenaOptions
#pragma tm2_absolute portArenaItems
#pragma tm2_absolute portArenaLive
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c); none of them calls rand() */
extern void portArenaLatch(void);               /* roundStart: portArenaLive = the items off this round */
extern int  portArenaOff(int item);             /* 1 when `item` is off this round */
extern int  portArenaTargetImmune(int type);    /* targetDamage: 1 when target `type` takes no damage */
extern int  portArenaTrainsPark(void);          /* worldUpdate (level 8): with TRAINS off park both trains and
                                                   return 1 (no worldTrackVehiclesUpdate); else 0 */
#ifdef __cplusplus
}
#endif
#endif

#endif
