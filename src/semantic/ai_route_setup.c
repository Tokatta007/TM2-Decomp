/* Game code (800F..8010 range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_specials.h"   /* request 35: portCarSpecialAsBlockInit */
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/car_groups.h"
#include "semantic/dt_player.h"   /* request 36: a player's Dark Tooth's specials */
#include "semantic/car_numbers.h"   /* request 70: portCarProfileWeapons */
#endif
extern void aiSetSegmentSpeed(struct TM2AIRecord *ai, unsigned char mode);

#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

/* Route points: position, up to 6 successor indices, successor count, route group. */
typedef struct {
    s32 x, y;
    s16 z;
    s16 next[6];
    s8 count;
    s8 route;
} TM2RouteNodeG3;
extern TM2RouteNodeG3 aiRouteNodes[];

/* [0] next node, [1] next route, [2] link target, by [route][target route] (35 routes). */
extern u8 aiRouteTransitionTables[3][35][35];

/* @800F6080: track-specific patch of the route-node table (successor counts/links,
 * fixed nodes 84-95) and a few per-node byte tables. Source statement order
 * drives the scheduler (it reorders these non-volatile global stores). */
void __attribute__((section(".text_800F6080"))) aiRoutePatchBossLevel(void)
{
    s32 i;

    aiRouteNodes[0].count = 3;
    aiRouteNodes[4].count = 2;
    aiRouteNodes[25].count = 1;
    aiRouteNodes[49].count = 1;
    aiRouteNodes[51].count = 1;
    aiRouteNodes[53].count = 1;
    aiRouteNodes[55].count = 1;
    aiRouteNodes[9].count = 2;
    aiRouteNodes[41].count = 2;
    aiRouteNodes[43].count = 2;
    aiRouteNodes[45].count = 2;
    aiRouteNodes[5].count = 2;
    aiRouteNodes[8].count = 2;
    aiRouteNodes[37].count = 2;
    aiRouteNodes[39].count = 2;
    aiRouteNodes[2].next[3] = 40;
    aiRouteNodes[11].next[3] = 44;
    aiRouteNodes[40].next[1] = 2;
    aiRouteNodes[44].next[0] = 11;
    aiRouteNodes[112].count = 2;
    aiRouteNodes[112].next[0] = 11;
    aiRouteNodes[112].next[1] = 44;
    aiRouteNodes[117].count = 2;
    aiRouteNodes[117].next[0] = 2;
    aiRouteNodes[117].next[1] = 40;
    for (i = 84; i < 87; i++) {
        aiRouteNodes[i].x = -4800;
        aiRouteNodes[i].count = 1;
        aiRouteNodes[i].next[0] = 4;
    }
    for (i = 87; i < 90; i++) {
        aiRouteNodes[i].x = 4800;
        aiRouteNodes[i].count = 1;
        aiRouteNodes[i].next[0] = 9;
    }
    for (i = 90; i < 93; i++) {
        aiRouteNodes[i].x = 4800;
        aiRouteNodes[i].count = 1;
        aiRouteNodes[i].next[0] = 11;
    }
    for (i = 93; i < 96; i++) {
        aiRouteNodes[i].x = -4800;
        aiRouteNodes[i].count = 1;
        aiRouteNodes[i].next[0] = 2;
    }
    aiRouteTransitionTables[2][24][20] = 49;
    aiRouteTransitionTables[2][24][21] = 51;
    aiRouteTransitionTables[2][24][22] = 53;
    aiRouteTransitionTables[2][24][4] = 44;
    aiRouteTransitionTables[2][24][5] = 40;
    aiRouteTransitionTables[2][24][6] = 4;
    aiRouteTransitionTables[2][24][7] = 9;
    aiRouteTransitionTables[0][24][8] = 37;
    aiRouteTransitionTables[1][24][8] = 4;
    aiRouteTransitionTables[0][24][9] = 39;
    aiRouteTransitionTables[1][24][9] = 5;
    aiRouteTransitionTables[0][24][10] = 39;
    aiRouteTransitionTables[1][24][10] = 5;
    aiRouteTransitionTables[0][24][11] = 37;
    aiRouteTransitionTables[1][24][11] = 5;
    aiRouteTransitionTables[0][24][14] = 37;
    aiRouteTransitionTables[1][24][14] = 4;
    aiRouteTransitionTables[0][24][15] = 39;
    aiRouteTransitionTables[1][24][15] = 5;
    aiRouteTransitionTables[0][24][17] = 39;
    aiRouteTransitionTables[1][24][17] = 5;
    aiRouteTransitionTables[0][24][18] = 37;
    aiRouteTransitionTables[1][24][18] = 4;
}

typedef struct { s32 base; s32 a, b; } TM2KindInfoG3;
extern TM2KindInfoG3 worldSectionFloorHeights[];
extern s32 rand(void);
extern void aiChooseNextNode(struct TM2AIRecord *ai, s32 node);
extern void aiUpdateStuck(struct TM2AIRecord *ai);
extern void aiAdvanceRouteNode(struct TM2AIRecord *ai);
extern void wheelBlockReset(struct TM2CarWheels *);
extern void wheelBlockSetSection(struct TM2CarWheels *, s32);
extern void wheelBlockSetHeight(struct TM2CarWheels *, s32);
extern void carBodyAttitudeUpdate(void *car, s32);

#define AI_NODE(ai) (AI(ai)->stats.spawn_node)
#define AI_TARGET(ai) (AI(ai)->stats.target_node)

/* @800F6518: (re)place an AI car on its current route node: pick a random successor,
 * walk the path state machine onto the node, then reset target, ground and steering state. */
void __attribute__((section(".text_800F6518"))) aiPlaceOnRouteNode(struct TM2AIRecord *ai)
{
    s32 k;

    if (aiRouteNodes[AI_NODE(ai)].count > 0)
        k = rand() % aiRouteNodes[AI_NODE(ai)].count;
    else
        k = 0;
    aiChooseNextNode(ai, aiRouteNodes[AI_NODE(ai)].next[k]);
    aiUpdateStuck(ai);
    aiSetSegmentSpeed(ai, 0);
    aiAdvanceRouteNode(ai);
    aiChooseNextNode(ai, AI_NODE(ai));
    aiUpdateStuck(ai);
    aiSetSegmentSpeed(ai, 0);
    aiAdvanceRouteNode(ai);
    k = 0;
    {
        s32 j;
        for (j = 0; j < aiRouteNodes[AI_NODE(ai)].count; j++) {
            if (aiRouteNodes[AI_NODE(ai)].next[j] == AI_TARGET(ai)) {
                k = j;
                break;
            }
        }
    }
    AI_TARGET(ai) = aiRouteNodes[AI_NODE(ai)].next[k];
    aiChooseNextNode(ai, AI_TARGET(ai));
    aiUpdateStuck(ai);
    aiSetSegmentSpeed(ai, 0);
    aiAdvanceRouteNode(ai);
    k++;
    if (k >= aiRouteNodes[AI_NODE(ai)].count)
        k = 0;
    AI_TARGET(ai) = aiRouteNodes[AI_NODE(ai)].next[k];
    ai->motion.pos[0] = aiRouteNodes[AI_NODE(ai)].x;
    ai->motion.pos[1] = aiRouteNodes[AI_NODE(ai)].y;
    ai->motion.pos[2] = worldSectionFloorHeights[aiRouteNodes[AI_NODE(ai)].route].base;
    ai->motion.speed = 0;
    ai->state.f01 = 0;
    DRV(ai)->f45E = 0;
    ai->motion.rot[2] = DRV(ai)->heading;
    wheelBlockReset(&ai->wheels);
    wheelBlockSetSection(&ai->wheels, aiRouteNodes[AI_NODE(ai)].route);
    wheelBlockSetHeight(&ai->wheels, aiRouteNodes[AI_NODE(ai)].z);
    carBodyAttitudeUpdate(ai, 0);
}


/* Per-buffer (x2) overlay primitives at 0x801837C8, 880 bytes each. */
typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0, x1, y1, x2, y2, x3, y3; } TM2PolyF4G3;
typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0, x1, y1; } TM2LineF2G3;
typedef struct {
    TM2PolyF4G3 back;        /* 0x000 */
    TM2PolyF4G3 frame;       /* 0x018 */
    TM2LineF2G3 lines[TM2_HUD_RADAR_LINES];   /* 0x030 (48 for 12 blips) */
    TM2LineF2G3 l0, l1;      /* 0x330 */
    u32 dm[3];               /* 0x350 DR_MODE */
    s32 pad35C;
    s32 f360, f364, f368, f36C;
} TM2OverlayG3;
extern TM2OverlayG3 hudRadars[];
extern s32 viewGetSplitMode(void);
extern void SetSemiTrans(void *, s32);
extern void SetDrawMode(void *, s32, s32, s32, void *);

typedef struct { unsigned addr : 24; unsigned len : 8; u8 r0, g0, b0, code; } TM2PTagG3;
#define G3_SETLEN(p, l, c) (((TM2PTagG3 *)(p))->len = (l), ((TM2PTagG3 *)(p))->code = (c))
#define G3_RGB0(p, r, g, b) ((p)->r0 = (r), (p)->g0 = (g), (p)->b0 = (b))
#define G3_XY4(p, a, b, c, d, e, f, g, h) ((p)->x0 = (a), (p)->y0 = (b), (p)->x1 = (c), (p)->y1 = (d), \
    (p)->x2 = (e), (p)->y2 = (f), (p)->x3 = (g), (p)->y3 = (h))
#define G3_XY2(p, a, b, c, d) ((p)->x0 = (a), (p)->y0 = (b), (p)->x1 = (c), (p)->y1 = (d))

/* @800FC5C4: initialise both buffers' overlay primitives (two flat quads, 50 lines,
 * draw mode) with layout depending on the display mode from viewGetSplitMode
 * (0 and >1: narrow layout, 1: wide layout).
 * MATCH: the narrow layout is written twice (mode 0 and mode >1) so CSE follows
 * into each copy with the body's address registers, as in retail; jump2
 * cross-jumps the copies into one block. The line base lb and j = 0 set before
 * the frame SetSemiTrans call give retail's line-loop registers.
 * loop.c: equal constants in the two narrow copies would be matched (savings and
 * lifetimes add up), so 6, 63, 79, 82, 43 and 42 would be hoisted out of the
 * outer loop (threshold 29, -3 per move, 248 insns). The mode >1 copy therefore
 * takes them from s16 variables first referenced before the loop (dead initial
 * values, deleted by flow): a user variable in a conditional block whose first
 * reference is not its set is not movable, so only 3 (and the addresses, 5, 36)
 * move, as in retail. s16 keeps the sets identical to the mode 0 copy's for
 * cross-jumping; the back pointer b puts the 6 after the address computation. */
void __attribute__((section(".text_800FC5C4"))) hudRadarInit(void)
{
    s32 i;
    /* narrow-layout values for the mode >1 copy; the dead initial values only make
     * the loop below see these as variables first set before it (see above) */
    s16 bx0 = 0, ly1 = 0, lx = 0, by2 = 0, bx1 = 0, fx1 = 0;

    for (i = 0; i < TM2_MAX_HUDS; i++) {
        TM2LineF2G3 *ln;
        u32 lb;
        s32 j;
#define o (&hudRadars[i])
        G3_SETLEN(&o->back, 5, 0x28);
        G3_SETLEN(&o->frame, 5, 0x28);
        if (viewGetSplitMode())
            SetSemiTrans(&o->back, 1);
        else
            SetSemiTrans(&o->back, 0);
        lb = (u32)hudRadars[0].lines;
        j = 0;
        SetSemiTrans(&o->frame, 1);
        G3_RGB0(&o->back, 17, 38, 5);
        ln = (TM2LineF2G3 *)(lb + i * TM2_HUD_RADAR_BYTES);
        G3_RGB0(&o->frame, 36, 150, 36);
        for (; j < TM2_HUD_RADAR_LINES; j++) {
            G3_SETLEN((j << 4) + i * TM2_HUD_RADAR_BYTES + lb, 3, 0x40);
            SetSemiTrans(ln, 0);
            ln++;
        }
        G3_SETLEN(&o->l0, 3, 0x40);
        G3_SETLEN(&o->l1, 3, 0x40);
        G3_RGB0(&o->l0, 36, 150, 36);
        G3_RGB0(&o->l1, 36, 150, 36);
        if (!viewGetSplitMode()) {
            G3_XY4(&o->back, 6, 18, 79, 18, 6, 63, 79, 63);
            G3_XY4(&o->frame, 3, 15, 82, 15, 3, 66, 82, 66);
            G3_XY2(&o->l0, 41, 41, 45, 41);
            G3_XY2(&o->l1, 43, 40, 43, 42);
        } else if (viewGetSplitMode() == 1) {
            G3_XY4(&o->back, 7, 20, 80, 20, 7, 65, 80, 65);
            G3_XY4(&o->frame, 4, 17, 83, 17, 4, 68, 83, 68);
            G3_XY2(&o->l0, 42, 43, 46, 43);
            G3_XY2(&o->l1, 44, 42, 44, 44);
        } else {
            TM2PolyF4G3 *b = &o->back;
            bx0 = 6; ly1 = 42; lx = 43; by2 = 63; bx1 = 79; fx1 = 82;
            G3_XY4(b, bx0, 18, bx1, 18, bx0, by2, bx1, by2);
            G3_XY4(&o->frame, 3, 15, fx1, 15, 3, 66, fx1, 66);
            G3_XY2(&o->l0, 41, 41, 45, 41);
            G3_XY2(&o->l1, lx, 40, lx, ly1);
        }
        SetDrawMode(o->dm, 0, 0, 28, 0);
        hudRadars[i].f364 = 8000;
        hudRadars[i].f368 = 1365333;
        hudRadars[i].f360 = 0;
        hudRadars[i].f36C = 0;
#undef o
    }
}

extern void weaponSlamStateInit(struct TM2CarWeapons *);

/* @800FDCAC: reset a car's pickup/weapon record (rec) and set its range/timer fields
 * from the car type; car is a human record when alt is set, else an AI record. */
void __attribute__((section(".text_800FDCAC"))) weaponBlockInit(void *car, struct TM2CarWeapons *rec, u8 alt)
{
    s32 kind;
    s16 i;
    struct TM2FireEffect *p;

    if (alt == 0)
        kind = AI(car)->type;
    else
        kind = ((struct TM2HumanRecord *)car)->type;
    rec->rear_fire = 0;
    rec->drop_armed = 0;
    rec->invisible = 0;
    rec->invis_timer = 0;
    rec->gun_cooldown = 0;
    rec->fire_cooldown = 0;
    rec->f36 = 0;
    rec->gun_side = 0;
    for (i = 0; i < 10; i++)
        rec->ammo[i] = 0;       /* i = 9 is f20 */
    rec->ammo[8] = 0;
    rec->selected = 0;
    rec->ammo[0] = 2;
    rec->ammo[1] = 2;
    rec->ammo[5] = 1;
    rec->f2E = 0;
    rec->f2A = 99;
    rec->turbo = 200;
    rec->held = 5;
    rec->f0C = 107;
    rec->energy = 1000;
    switch (kind) {
    case 16:  rec->recharge_period = 360;  rec->capacity = 24; rec->max_specials = 5; break;
    case 32:  rec->recharge_period = 1200; rec->capacity = 24; rec->max_specials = 5; break;
    case 48:  rec->recharge_period = 1100; rec->capacity = 24; rec->max_specials = 3; break;
    case 64:  rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 5; break;
    case 80:  rec->recharge_period = 1400; rec->capacity = 24; rec->max_specials = 3; break;
    case 96:  rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 5; break;
    case 112: rec->recharge_period = 1600; rec->capacity = 24; rec->max_specials = 5; break;
    case 128: rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 4; break;
    case 144: rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 5; break;
    case 160: rec->recharge_period = 360;  rec->capacity = 24; rec->max_specials = 3; break;
    case 176: rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 5; break;
    case 192:
        rec->recharge_period = 1000;
        weaponSlamStateInit(rec);
        rec->capacity = 24;
        rec->max_specials = 3;
        break;
    case 208: rec->recharge_period = 400;  rec->capacity = 24; rec->max_specials = 5; break;
    case 240: rec->recharge_period = 700;  rec->capacity = 24; rec->max_specials = 5; break;
    case 224: rec->recharge_period = 360;  rec->capacity = 24; rec->max_specials = 5; break;
    default:  rec->capacity = 10; rec->max_specials = 5; break;
    }
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth fires the head swoop: Sweet Tooth's 360 / 5 unless the
       port sets its own (0 = the default) */
    if (alt != 0 && kind == 0xF0) {
        rec->recharge_period = portDtPlayer.recharge > 0 ? portDtPlayer.recharge : TM2_DT_PLAYER_RECHARGE;
        rec->max_specials = portDtPlayer.max_specials > 0 ? portDtPlayer.max_specials : TM2_DT_PLAYER_MAX_SPECIALS;
    }
#endif
#ifdef TM2_PORT_CAR_PROFILE
    portCarProfileWeapons(car, rec, alt);   /* request 70 (CAR_PROFILES.md): a mod car's own special numbers */
#endif
#ifdef TM2_PORT_SPECIAL_AS
    portCarSpecialAsBlockInit(car, rec, alt);   /* request 35: a borrowed special's max_specials, recharge_period */
#endif
    if ((p = rec->fire_effect) != 0)
        p->active = 0;
    if ((p = rec->f40) != 0)
        p->active = 0;
}

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2MatrixG3;
extern TM2MatrixG3 mathIdentityMatrix;   /* identity matrix */
extern void carStateFlagsClear(struct TM2CarState *);
extern s32 aiDifficultySpeedScale(void);
extern s32 carStatsSetSpeedMult(struct TM2CarStatsBlock *, s32);
extern void carStatsLoad(struct TM2CarStatsBlock *, s32);
extern void carWheelsSetStepHeight(struct TM2CarWheels *, s32);

#define G3_COPYMAT(d) \
    ((d).m[0][0] = mathIdentityMatrix.m[0][0], (d).m[0][1] = mathIdentityMatrix.m[0][1], \
     (d).m[0][2] = mathIdentityMatrix.m[0][2], (d).m[1][0] = mathIdentityMatrix.m[1][0], \
     (d).m[1][1] = mathIdentityMatrix.m[1][1], (d).m[1][2] = mathIdentityMatrix.m[1][2], \
     (d).m[2][0] = mathIdentityMatrix.m[2][0], (d).m[2][1] = mathIdentityMatrix.m[2][1], \
     (d).m[2][2] = mathIdentityMatrix.m[2][2], (d).t[0] = mathIdentityMatrix.t[0], \
     (d).t[1] = mathIdentityMatrix.t[1], (d).t[2] = mathIdentityMatrix.t[2])

/* @80100034: reset a car for a (re)start: clear AI motion/steering state, reset both
 * orientation matrices to identity, then re-init sub-records, wheels, model box and limits. */
void __attribute__((section(".text_80100034"))) aiCarReset(struct TM2AIRecord *ai)
{
    ai->motion.pos[0] = 0;
    ai->motion.pos[1] = 0;
    ai->motion.pos[2] = ai->stats.ride_height;
    ai->motion.vel0 = 0;
    ai->motion.speed = 0;
    ai->motion.vel2 = 0;
    ai->motion.rot[0] = 0;
    ai->motion.rot[1] = 0;
    ai->motion.rot[2] = 0;
    ai->motion.spin0 = 0;
    ai->motion.spin1 = 0;
    ai->motion.yaw_rate = 0;
    ai->motion.body_rot[0] = 0;
    ai->motion.body_rot[1] = 0;
    ai->motion.body_rot[2] = 0;
    ai->motion.body_spin[0] = 0;
    ai->motion.body_spin[1] = 0;
    ai->motion.body_spin[2] = 0;
    G3_COPYMAT(ai->motion.rot_m);
    G3_COPYMAT(ai->motion.body_rot_m);
    DRV(ai)->f448 = 0;
    carStateFlagsClear(&ai->state);
    carSpinStop(ai, 0);
    ai->stats.probe_on = 1;
    carPushClear(&ai->contact);
    carStatsSetSpeedMult(&ai->stats, aiDifficultySpeedScale());
    carStatsLoad(&ai->stats, ai->type);
    carStatsSetTopSpeed(&ai->stats, ai->stats.top_speed);
    carBuildCollisionBox(ai, 0);
    carWheelsSetStepHeight(&ai->wheels, ai->type);
    ai->range_near = 320;
    ai->range_1 = 160;
    ai->range_2 = 600;
    ai->range_3 = 1000;
    ai->range_far = 2000;
    ai->stats.hp = ai->stats.max_hp;
}
