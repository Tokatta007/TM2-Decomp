/* Vanilla Plus balancing (PC port requests 40, 41 and round 3, docs/BALANCE.md; it absorbs request 39's ram rules).
 *
 * With -DTM2_PORT_BALANCE (the port's `--port-define TM2_PORT_BALANCE`) each rule of the port's Vanilla Plus list is
 * one bit of one word, portBalance (0 = retail). The port sets all of them (TM2_BAL_ALL) in Vanilla Plus and
 * online (one ruleset; put the word in the ruleset hash). The gates are calls of the port functions below at the
 * points BALANCE.md 4 lists; every function returns the retail value (or does nothing) when its bit is clear.
 *
 *   portBalance       the port's switches (TM2_BAL_* below), 0 = retail
 *   portBalanceLive   the switches of this round, written by the C: copied at roundStart (portBalanceLatch),
 *                     read by every gate. The port reads it, never writes it
 *   portBalanceDropSeq[TM2_MAX_DROPS], portBalanceDropSeqNext   the placement order of each drop record (bit 2:
 *                     "a 6th mine removes that driver's oldest"); written by the C
 *   portBalanceRideTop[TM2_MAX_CAR_SLOTS]   per crusher slot: the one-time +5 "on top" is still owed (bit 3);
 *                     written by the C
 *   portBalanceCars[TM2_MAX_CAR_SLOTS]   per car slot (request 41, struct TM2PortBalanceCar): the freeze chain
 *                     (bit 17), the refused gun button (bit 15), the machine guns' half point (bit 20); written
 *                     by the C
 *   portBalanceGunObjects   the machine guns' half point of every hit that is not a car (bit 20, round 3);
 *                     written by the C
 *   portBalance2, portBalanceLive2   request 124: the second word of switches (TM2_BAL2_*), set by the port, and
 *                     its copy for the round, latched with portBalanceLive (portBalance's 32 bits are all used)
 *
 * Word zero: every gate passes and the retail logic runs, with the same rand() calls in the same order (no port
 * function calls rand(); tools/check_balance.py). With bits set the rand() stream differs only where a rule makes
 * one of retail's own calls run or not run (BALANCE.md 5), the same on every machine for the same word.
 *
 * Request 39's define TM2_PORT_RAM_BALANCE and its words portRamOptions / portRamCrushDamage / portRamLive /
 * portRamCrushLive are retired (PR #53 was never merged): its bit 0 is now bits 3 + 4, its bit 1 is bit 11, the
 * crush word is gone (the user's numbers are fixed: 15 at the mount, +5 on top).
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss, after portHumanRadarBlink); this header
 * declares it for the port variant and, with the define, the port functions the gates call. */
#ifndef TM2_SEMANTIC_PORT_BALANCE_H
#define TM2_SEMANTIC_PORT_BALANCE_H

#ifdef TM2_PORT_RAM_BALANCE
#error "TM2_PORT_RAM_BALANCE (request 39) is retired: use TM2_PORT_BALANCE (request 40, docs/BALANCE.md; bits 3, 4, 11)"
#endif

/* portBalance bits (BALANCE.md 2) */
#define TM2_BAL_MISSILES      0x0001  /* bit 0, item 1: fire missile 10, homing missile 7 */
#define TM2_BAL_SHIELD        0x0002  /* bit 1, item 2: a shield cast while shielded (or the pool is full) is refused,
                                         costs nothing */
#define TM2_BAL_MINES         0x0004  /* bit 2, item 3: mines permanent, 5 per driver (the 6th removes the oldest),
                                         own range of drop records, refund when one cannot be placed */
#define TM2_BAL_HAMMERHEAD    0x0008  /* bit 3, item 4: ram damage dealt x1.2; crush 15 + 5 once on top; recharge 700
                                         (request 41 item 1; request 40 built 900) */
#define TM2_BAL_AXEL          0x0010  /* bit 4, item 5: never mounts; ram damage dealt x1.1 */
#define TM2_BAL_MINION        0x0020  /* bit 5, item 6: never mounts; ram damage dealt x1.1; recharge 600 */
#define TM2_BAL_TWISTER       0x0040  /* bit 6, items 7a-7c: no mine / remote-bomb placement while her tornado runs;
                                         recharge 600; no ram damage taken while it runs */
#define TM2_BAL_GRASSHOPPER   0x0080  /* bit 7, items 8a-8b: leap damage 20 (18 until request 121); no ram damage
                                         taken while leaping
                                         (8c, touching the target counts as the hit, is bit 25 since request 88) */
#define TM2_BAL_RECHARGE      0x0100  /* bit 8, item 9: Outlaw 2's recharge 900 (request 45 split the bit: Mr. Grimm's
                                         is bit 21, Thumper's bit 22) */
#define TM2_BAL_OUTLAW2_RECHARGE TM2_BAL_RECHARGE /* bit 8 by its request-45 name */
#define TM2_BAL_GUN_RANGE     0x0200  /* bit 9, request 121 item 4 (BALANCE.md 25): every machine-gun bullet flies
                                         TM2_BAL_GUN_RANGE_PCT % as far (its life: a player's 12 frames -> 15, a
                                         computer car's 15 -> 19), every car. Until request 121 bit 9 was item 10's,
                                         reserved with no effect (Sweet Tooth's spiral is retail: BALANCE.md 3.11) */
#define TM2_BAL_SHADOW        0x0400  /* bit 10, item 11: the raised-surface splash fix: Shadow's detonation
                                         min(retail, 25) + 3, a remote bomb's min(retail, 25) */
#define TM2_BAL_HH_BORROW     0x0800  /* bit 11, request 39 bit 1: a Hammerhead borrowing another special does not
                                         mount (needs TM2_PORT_SPECIAL_AS; ignored without it) */
#define TM2_BAL_TWISTER_THROW 0x1000  /* bit 12, item 7d (option c): the catching as retail; when her tornado ends,
                                         every car still in her catch box is thrown out (BALANCE.md 3.7) */
#define TM2_BAL_LEAP_LANDING  0x2000  /* bit 13, item 8d: Grasshopper's clean landing (first cut; awaits the port's
                                         runtime look) */
/* request 41 (round 2, BALANCE.md 12), one rule per bit */
#define TM2_BAL_THUMPER_LOCK  0x4000  /* bit 14, request 121 item 5 (BALANCE.md 25): a Thumper (or a borrower of his
                                         special) fires no special while a flame of his own still burns: denied
                                         sound, no cost (two flames on one car hit it twice a frame). Until request
                                         121 bit 14 was reserved with no effect (round 3, BALANCE.md 13: request 41's
                                         "no hit before the hop", removed) */
#define TM2_BAL_TWISTER_LOCK  0x8000  /* bit 15, item 4: while her tornado runs a Twister fires nothing (pickups and
                                         her special, the machine guns, every energy attack but the shield, a remote
                                         bomb's detonation): denied sound, no cost. Request 46 item 3
                                         (PLAYTEST_ANSWERS.md 10): the shield (energy attack 103) is let through
                                         (request 41's bit 15 locked it too) */
#define TM2_BAL_LOCK_FREE_KIND 103    /* bit 15: the energy attack the lock lets through (the shield) */
#define TM2_BAL_TASER         0x10000 /* bit 16, item 6: Outlaw 2's taser shot that connected to no one may still
                                         connect on a later tick of its 40: the connect's lift (carSpinOut kind
                                         6) at once, one hit of 10 (retail's last; round 3, BALANCE.md 13) */
#define TM2_BAL_FREEZE_DR     0x20000 /* bit 17, item 7: an ice freeze (type 14: freeze missile, energy freeze,
                                         Minion's volley) of a car frozen, or unfrozen for at most 20 frames, lasts
                                         half its previous one (floor 15 ticks) */
#define TM2_BAL_RICOCHET      0x40000 /* bit 18, item 9: a ricochet bomb cannot hit its owner before it has once been
                                         outside his box (retail: 2 ticks a human's, 4 an AI's); request 45: until
                                         then it also passes through his car's hit box (no bounce, no flash, no
                                         push-out) */
#define TM2_BAL_FREE_JUMP     0x80000 /* bit 19, item 10: Grasshopper's energy jump (attack 104) costs no energy (its
                                         cooldown kept) */
/* round 3 (BALANCE.md 13) */
#define TM2_BAL_GUNS          0x100000 /* bit 20: the machine guns: no 50 % coin flip, every bullet that connects
                                         counts at half its power (a half point carried per target car, exact) */
/* request 45 (BALANCE.md 14): bit 8 split per car (bit 8 stays Outlaw 2's) */
#define TM2_BAL_GRIMM_RECHARGE   0x200000 /* bit 21: Mr. Grimm's recharge 1200 (retail 1400; bit 8 before request 45) */
#define TM2_BAL_THUMPER_RECHARGE 0x400000 /* bit 22: Thumper's recharge 1500 (retail 1600; bit 8 before request 45) */
#define TM2_BAL_PICKUP_DRAW   0x800000 /* bit 23: request 67 (BALANCE.md 16, PICKUP_LOOKS.md 4): pickupRespawnTick's
                                         one rand() picks among the pickups that are taken (model hidden, slot active
                                         0) and can show (not under a select a target / level event keeps whole):
                                         rand() % that count; health keeps retail's half chance; none: nothing */
/* request 77 (BALANCE.md 17) */
#define TM2_BAL_HH_MOVE       0x1000000 /* bit 24: Hammerhead's movement: brakes (+0x5C) 2 and slide speed (+0x56) 60
                                         (retail 1 and 25), set by carStatsLoad before the derived fields (braking
                                         +0xCC, grip +0xC0), for every car of his type (human, computer, a swap); read
                                         from portBalance itself at the load, not portBalanceLive: computer cars load
                                         at bind time, before roundStart's latch */
/* request 88 (BALANCE.md 18): an option the port leaves off, so not in TM2_BAL_ALL */
#define TM2_BAL_LEAP_TOUCH    0x2000000 /* bit 25, item 8c (bit 7's until request 88): Grasshopper touching her
                                         leap's target counts as the hit (portBalanceLeapTouch) */
/* request 91 item 2 (BALANCE.md 19) */
#define TM2_BAL_SLAM_LOCK     0x4000000 /* bit 26: while his arm and claw special runs (his slam record active: the
                                         grab or the swing) a Mr. Slam fires no pickup weapon, no special and no
                                         energy attack but the shield (a remote bomb's detonation too): denied
                                         sound, no cost, as bit 15's Twister; the machine guns as retail */
/* request 91 item 3 (BALANCE.md 20) */
#define TM2_BAL_AI_UNJAM      0x8000000 /* bit 27: a computer car stopped by other cars (not a wall, not its own
                                         target) TM2_BAL_JAM_STOPS times within TM2_BAL_JAM_WINDOW backs out and,
                                         for TM2_BAL_JAM_AVOID, picks no route node it was jammed toward and does
                                         not drive straight at its target: it goes round the pile */
/* request 95 (BALANCE.md 21) */
#define TM2_BAL_BOOMERANG     0x10000000 /* bit 28: a boomerang (type 51) on its way back is caught, the thrower's
                                         charge back (+1, at most its max_specials), wherever it comes within
                                         TM2_BAL_BOOMERANG_CATCH of its thrower: in either sub-step's test and where
                                         retail's steering takes it away without a charge (weaponProjectileSteer's
                                         < 20); only the return gives it (a hit or the life's end as retail: no
                                         charge; through walls on its way back as retail); any thrower (human or
                                         computer) */
/* request 101 (BALANCE.md 22) */
#define TM2_BAL_AI_ROUTE      0x20000000 /* bit 29: computer cars find their way by the route graph where the level's
                                         tables give none: a car whose route section has no node takes the nearest
                                         node of any (carNearestRoutePoint, not node 0); a route table entry past the
                                         level's nodes, or the target's section on a node the car is not in, is
                                         replaced by the next node of the graph's shortest way to the target's
                                         nearest node (aiFollowRouteTarget); no direct chase at a target the graph
                                         cannot reach (aiShouldChaseDirect). Roof Tops (SROOF), whose TM1 route
                                         data has every node in section 6 and a next-node table past its 9 nodes */
/* request 102 item 3 (BALANCE.md 23) */
#define TM2_BAL_BOOM_SOUND    0x40000000 /* bit 30: an explosion's sound (fxExplosion6..192, the smoke bursts' sound 8)
                                         plays inside retail's 6-step cooldown too when it is TM2_BAL_BOOM_APART or
                                         more from the last one that played: retail's one cooldown for the whole
                                         arena let a far, unheard blast silence a near one (16 cars: most hits) */
/* request 103 (BALANCE.md 24) */
#define TM2_BAL_LEAP_PUSH_BIT 0x80000000 /* bit 31: Grasshopper's leap hit pushes its target TM2_BAL_LEAP_PUSH (before
                                         the target's mass, as a missile's: (100 - mass)%, half again at mass 51 or
                                         more) along her flight (from her to the target, flat), through retail's
                                         carKnockBack after the hit's spin (carApplyDamage type 2, unchanged); her
                                         own movement unchanged; every Grasshopper (human, computer car, twins) */
#define TM2_BAL_ALL           0xFDFFFFFF /* Vanilla Plus and online (bits 9 and 14 are request 121's since it); bit 25 is not
                                         in it (the user's: off). Bit 31 is set: read the word as unsigned */

/* portBalance2 bits (request 124 on: portBalance's 32 bits are all used). Latched with portBalance
 * (portBalanceLive2), the same rules: 0 = retail, put the word in the ruleset hash next to portBalance. */
#define TM2_BAL2_PICKUP_PASS  0x0001  /* bit 0, request 124 (BALANCE.md 26): every shot passes a pickup's hit box,
                                         ids 401..480 (retail: 401..440, and the ricochet bomb's push-out check
                                         after a bounce passed none: next to a wall a pickup trapped it). The
                                         ricochet's two checks, the missiles' and the bullets' one */
#define TM2_BAL2_ALL          0x0001  /* Vanilla Plus and online */
#define TM2_BAL2_PICKUP_LAST  480     /* bit 0: the last pickup id (441..480: the +40 "keep" pickups) */

/* The numbers (BALANCE.md 3) */
#define TM2_BAL_BOOMERANG_CATCH 20      /* bit 28: the catch distance (the source's DISTANCE, 8 position units: 160);
                                         retail's steering removes the boomerang inside it */
#define TM2_BAL_HAMMERHEAD_TYPE 0x20
#define TM2_BAL_OUTLAW2_TYPE    0x30
#define TM2_BAL_GRIMM_TYPE      0x50
#define TM2_BAL_GRASSHOPPER_TYPE 0x60
#define TM2_BAL_THUMPER_TYPE    0x70
#define TM2_BAL_TWISTER_TYPE    0xA0
#define TM2_BAL_SLAM_TYPE       0xC0
#define TM2_BAL_AXEL_TYPE       0xB0
#define TM2_BAL_MINION_TYPE     0xE0
#define TM2_BAL_HH_TENTHS       12    /* x1.2 */
#define TM2_BAL_AXEL_TENTHS     11    /* x1.1 */
#define TM2_BAL_MINION_TENTHS   11    /* x1.1 */
#define TM2_BAL_RIDE_TOP        5     /* Hammerhead: once, the first frame on top (15 + 5 = 20) */
#define TM2_BAL_HH_BRAKE        2     /* bit 24: Hammerhead's brakes (retail 1) */
#define TM2_BAL_HH_SLIDE        60    /* bit 24: and his slide speed (retail 25) */
#define TM2_BAL_MINES_PER_DRIVER 5
#define TM2_BAL_LEAP_DAMAGE     20    /* retail 15; the PC version's 20 (request 121 item 3; 18 until it) */
#define TM2_BAL_GUN_RANGE_PCT   125   /* bit 9: a bullet's life x 1.25, rounded (12 -> 15, 15 -> 19) */
#define TM2_BAL_SPLASH_CAP      25    /* bit 10: a car's splash damage capped here (Shadow and remote bombs), then */
#define TM2_BAL_SPLASH_BONUS    3     /* + 3 for Shadow's detonation only (28 max) */
#define TM2_BAL_TORNADO_BOX     35    /* bit 12: the catch box, retail's DISTANCE(her, car) < 35 (280 units) */
#define TM2_BAL_HH_RECHARGE     700   /* bit 3: Hammerhead's recharge (retail 1200; request 40 built 900) */
#define TM2_BAL_TASER_RANGE     80    /* bit 16: weaponTaserTarget's DISTANCE < 80 (640 units), nearest */
#define TM2_BAL_FREEZE_WINDOW   20    /* bit 17: frames (timerFrameCount) unfrozen that keep the chain (1 s) */
#define TM2_BAL_FREEZE_FLOOR    15    /* bit 17: the shortest freeze (ticks, 5 frames at 3 ticks) */
#define TM2_BAL_JUMP_COST       120   /* bit 19: the energy jump's cost (weaponFireEnergyAttack 104) */
#define TM2_BAL_JAM_STOPS       3     /* bit 27: the car stops (aiUpdateStop) that make a jam ... */
#define TM2_BAL_JAM_WINDOW      120   /* ... within these frames (timerFrameCount, 20 a second: 6 s) */
#define TM2_BAL_JAM_AVOID       100   /* bit 27: frames the car then goes round (5 s) */
#define TM2_BAL_ROUTE_NODES     140   /* bit 29: route nodes the graph walks at most (the PTS block, 0xD20 / 24) */
#define TM2_BAL_ROUTE_NEAR      1200  /* bit 29: a target this near (|dx| + |dy| + |dz|) a node the graph reaches
                                         counts as reached */
#ifndef TM2_BAL_LEAP_PUSH
#define TM2_BAL_LEAP_PUSH       300   /* bit 31: the leap's push before the target's mass (Spectre's missile 155, the
                                         boomerang on its way back 424); -DTM2_BAL_LEAP_PUSH=n to tune */
#endif
#define TM2_BAL_BOOM_APART      1000  /* bit 30: |dx| + |dy| + |dz| from the last explosion sound that makes another
                                         (one blast's own explosions lie within it) */
/* d scaled by tenths, rounded to nearest (half up); d >= 0 */
#define TM2_BAL_SCALE(d, tenths) (((d) * (tenths) + 5) / 10)

#ifdef TM2_PORT_VARIANT
/* per car slot (request 41; round 3 drops bit 14's leap fields); 8 bytes */
struct TM2PortBalanceCar {
    unsigned int frozen_at;         /* +0x00 bit 17: the last frame (timerFrameCount) the car was frozen */
    short freeze_len;               /* +0x04 bit 17: the length of its last ice freeze in this chain (ticks; 0 = none) */
    unsigned char gun_half;         /* +0x06 bit 20: the half point of gun damage this car still owes (0 or 1) */
    unsigned char guns_refused;     /* +0x07 bit 15: the gun button was refused last frame (one denied sound) */
};
typedef char tm2_port_balance_car_size[(sizeof(struct TM2PortBalanceCar) == 8) ? 1 : -1];
/* Request 91 item 3 (bit 27, BALANCE.md 20): per car slot, a computer car's jam count and its way round. */
struct TM2PortBalanceJam {
    unsigned int since;             /* +0x00 the frame (timerFrameCount) of the first stop counted */
    unsigned int avoid_until;       /* +0x04 the frame its way round ends (0 = none) */
    short avoid;                    /* +0x08 the route node it was jammed toward */
    unsigned char stops;            /* +0x0A the stops by cars counted since `since` */
    unsigned char pad;
};
typedef char tm2_port_balance_jam_size[(sizeof(struct TM2PortBalanceJam) == 12) ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
extern int portBalance;             /* the port's switches (TM2_BAL_*), 0 = retail */
extern int portBalanceLive;         /* the switches this round (roundStart), written by the C */
extern unsigned int portBalanceDropSeq[];       /* [TM2_MAX_DROPS] placement order of each drop record (bit 2) */
extern unsigned int portBalanceDropSeqNext;     /* the last order handed out */
extern unsigned char portBalanceRideTop[];      /* [TM2_MAX_CAR_SLOTS] the +5 on top still owed (bit 3) */
extern struct TM2PortBalanceCar portBalanceCars[]; /* [TM2_MAX_CAR_SLOTS] request 41 (bits 15, 17, 20) */
extern unsigned int portBalanceGunObjects;      /* bit 20: the half point of the gun hits that are not cars */
extern struct TM2PortBalanceJam portBalanceJam[]; /* [TM2_MAX_CAR_SLOTS] request 91 item 3 (bit 27) */
extern int portBalanceBoom[];       /* [4] request 102 (bit 30): the last explosion sound's x, y, z and 1 (0 = none) */
extern int portBalance2;            /* request 124: the port's second word of switches (TM2_BAL2_*), 0 = retail */
extern int portBalanceLive2;        /* its copy for this round (portBalanceLatch), written by the C */
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_BALANCE
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_BALANCE needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portBalance
#pragma tm2_absolute portBalanceLive
#pragma tm2_absolute portBalanceDropSeq
#pragma tm2_absolute portBalanceDropSeqNext
#pragma tm2_absolute portBalanceRideTop
#pragma tm2_absolute portBalanceCars
#pragma tm2_absolute portBalanceGunObjects
#pragma tm2_absolute portBalanceJam
#pragma tm2_absolute portBalance2
#pragma tm2_absolute portBalanceLive2
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c); none of them calls rand(). Each returns the retail value (or does nothing)
 * when its bit is clear. */
extern void portBalanceLatch(void);                  /* roundStart: the switches of this round */
extern int  portBalancePickupPass(int kind);         /* request 124: 1 = a shot passes this hit (portBalance2 bit 0
                                                        and a pickup id 401..480), else 0 (the caller's retail filter) */
extern int  portBalancePickupSlot(int r);            /* pickupRespawnTick: the slot to bring back from its one rand()
                                                        r (bit 23: among the taken ones that can show, -1 = none;
                                                        clear: r % pickupRegisteredCount, retail) */
extern int  portBalanceMissile(int kind, int damage); /* weaponLaunchProjectile: a missile's damage (bit 0) */
extern int  portBalanceShieldRefused(int shielded);  /* weaponFireEnergyAttack 103: 1 = refuse, no cost (bit 1) */
extern int  portBalanceMineRefused(void *car, int human); /* weaponFireEnergyAttack 101: 1 = refuse, no cost
                                                        (bit 2: no room; bit 6: her tornado runs) */
extern int  portBalanceBombRefused(void *car, int human); /* weaponFireSelected 3: 1 = refuse the placement (bit 6) */
extern int  portBalanceMineSlot(int id);             /* weaponDropWrite 101: the record for a mine of car `id`
                                                        (bit 2: removes the driver's oldest at 5; -1 = none) */
extern int  portBalanceBombSlot(void);               /* weaponDropWrite 3: the record for a remote bomb (bit 2) */
extern int  portBalanceMineKeep(int kind, int timer); /* weaponDropsUpdate: 1 = do not count down (bit 2) */
extern int  portBalanceNoMount(int slot, int type);  /* carCarCollision: 1 = may not mount (bits 4, 5, 11) */
extern int  portBalanceRam(void *car, int human, int other, int d); /* carContactRespond: the car-to-car ram
                                                        damage taken (bits 3-7) */
extern void portBalanceCrushStart(int slot, int kind); /* carCrashStart: a crush starts (bit 3: +5 owed) */
extern void portBalanceCrushTop(void *car, int human, int peer_human, int peer_index); /* carCrushContactUpdate:
                                                        on top (bit 3: the +5, once) */
extern void portBalanceCrushEnd(void *car, int human); /* carCrushContactUpdate: the ride ended */
extern int  portBalancePeriod(void *car, int human, int period); /* weaponTick: the recharge period (bits 3, 5,
                                                        6, 8, 21, 22) */
extern int  portBalanceLeapDamage(int damage);       /* weaponHomingLeapUpdate: the hit's damage (bit 7) */
extern int  portBalanceGunLife(int life);           /* bulletSpawn: the bullet's life, x 1.25 with bit 9 (request 121) */
extern int  portBalanceFlameLocked(void *car, int human); /* weaponFireSelected, the special: 1 = refuse, no cost
                                                     (bit 14: a Thumper whose own flame still burns; request 121) */
extern int  portBalanceLeapTouch(void);              /* weaponHomingLeapUpdate: 1 = her contact is with the target
                                                        (bit 25; bit 7 until request 88) */
extern void portBalanceLeapPush(void);               /* weaponHomingLeapUpdate: at the hit, after it (bit 31) */
extern void portBalanceLeapLand(void);               /* weaponHomingLeapUpdate: at the hit (bit 13) */
extern int  portBalanceSplash(int damage, int big);  /* areaDamage: a car's splash damage (bit 10) */
extern void portBalanceTornadoThrow(void);           /* weaponTornadoUpdate, as her tornado ends (inside her
                                                        selection): throw out the cars caught at the end (bit 12) */
/* request 41 (BALANCE.md 12; bit 14's portBalanceLeapArmed / portBalanceLeapHold removed in round 3) */
extern int  portBalanceLocked(void *car, int human); /* weaponFireSelected: 1 = refuse, no cost (bit 15: a Twister
                                                        whose tornado runs; bit 26: a Mr. Slam whose special runs) */
extern int  portBalanceEnergyLocked(void *car, int human, int kind); /* weaponFireEnergyAttack: 1 = refuse, no cost
                                                        (bits 15 and 26 as portBalanceLocked, any energy attack but
                                                        the shield TM2_BAL_LOCK_FREE_KIND; request 46 item 3) */
extern int  portBalanceGunsRefused(void *car, int human, int held); /* weaponTick: the machine guns (bit 15): 0
                                                        fire, 1 refused, 2 refused with the denied sound (press) */
/* request 91 item 3 (bit 27, BALANCE.md 20): a computer car (its AI record) */
extern int  portBalanceJamStop(void *ai);            /* aiUpdateStop, stopped: 1 = jammed by cars, back out */
extern int  portBalanceJamAvoid(void *ai, int node); /* aiPickLinkNode: 1 = skip this route node (going round) */
extern int  portBalanceJamRound(void *ai);           /* aiShouldChaseDirect: 1 = going round, no direct chase */
/* request 102 item 3 (bit 30, BALANCE.md 23): fxExplosion6..192 and the smoke bursts, `ready` retail's cooldown
   test (fxExplosionCooldown >= 6): 1 = play the sound (the caller then zeroes the cooldown) */
extern int  portBalanceBoomReady(int *pos, int ready);
/* request 101 (bit 29, BALANCE.md 22): route nodes are aiRouteNodes' indices, `pos` a position (x, y, z), `sec` its
   route section */
extern int  portBalanceRouteNearest(int *pos, int retail); /* carNearestRoutePoint, no node of the car's section:
                                                        the nearest node of any (bit clear: `retail`) */
extern int  portBalanceRouteNext(int node, int carsec, int next, int route, int troute, int *tpos);
                                                     /* aiFollowRouteTarget: -2 retail's choice, -1 chase the
                                                        target, else the node to head for */
extern int  portBalanceRouteBlocked(int node, int *tpos, int tsec); /* aiShouldChaseDirect, and the port's own
                                                        targeting: 1 = the graph cannot take a car at `node` to
                                                        the target (bit clear: 0) */
/* request 95 (bit 28, BALANCE.md 21): `rec` a projectile record (weapons.h struct TM2Projectile), `pt` its point now */
extern int  portBalanceBoomerangCatch(void *rec, int *pt); /* weaponProjectileSteer, weaponProjectileUpdate: 1 =
                                                        caught (freed, the charge given) */
extern void portBalanceTaserRetry(int arc);          /* weaponArcsDraw, an unconnected taser arc (bit 16): the
                                                        target search again; on a find the connect and its lift
                                                        (the one hit is retail's last tick) */
extern int  portBalanceFreeze(void *car, int human, int len); /* projectileHitTest, an ice freeze (the car
                                                        marked frozen): its length (bit 17: half the chain's last) */
extern void portBalanceFrozenTick(void *car, int human); /* humanCarTick, aiCarDrive, each frame frozen (bit 17) */
extern int  portBalanceRicochetOwner(void *shot, int retail, int dist); /* weaponRicochetUpdate: the shot may hit
                                                        its owner (bit 18: once it has left his box) */
extern int  portBalanceRicochetPass(void *shot, void *obj); /* weaponRicochetUpdate, after each hit pass: 1 = drop
                                                        the hit (bit 18, request 45: the volume hit is its owner's
                                                        car and the shot has not left his box yet: no bounce) */
extern int  portBalanceJumpFree(void *car, int human); /* weaponFireEnergyAttack 104: 1 = no energy (bit 19:
                                                        Grasshopper) */
/* round 3 (BALANCE.md 13) */
extern int  portBalanceGunHit(int hit, int power);   /* bulletsMove, a bullet that connected: -1 = retail (the coin
                                                        flip, `power`); else the damage to apply now (bit 20: every
                                                        hit, half its power, the half point carried per target) */
/* request 77 */
extern void portBalanceCarStats(void *stats, int type); /* carStatsLoad, after the type's own values (bit 24:
                                                        Hammerhead's brakes and slide speed) */
#ifdef __cplusplus
}
#endif
#endif

#endif
