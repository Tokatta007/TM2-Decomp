/* Game code (mixed). Names stay address-based. */
#include "semantic/weapons.h"
#include "semantic/car_numbers.h"   /* request 70: TM2_CAR_DEFAULT_TOP_SPEED (a mod car's own top speed) */
extern void aiSetSegmentSpeed(struct TM2AIRecord *ai, unsigned char mode);

#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

extern s32 gameGetLevel(void);
extern s32 aiGetDifficulty(void);
extern s32 gameGetMode(void);
extern s32 aiCarCount(void);
extern void mathMulTransVec(void *, s32 *, s32 *);
extern void missileTrailSetUVs(u8 *);

/* pairs of 12-byte tables picked by object type (0x10..0xF0) */
extern s32 aiSweetToothWeights9[], aiSweetToothWeights10[], aiHammerheadWeights9[], aiHammerheadWeights10[];
extern s32 aiOutlaw2Weights9[], aiOutlaw2Weights10[], aiWarthogWeights9[], aiWarthogWeights10[];
extern s32 aiMrGrimmWeights9[], aiMrGrimmWeights10[], aiGrasshopperWeights9[], aiGrasshopperWeights10[];
extern s32 aiThumperWeights9[], aiThumperWeights10[], aiSpectreWeights9[], aiSpectreWeights10[];
extern s32 aiRoadKillWeights9[], aiRoadKillWeights10[], aiTwisterWeights9[], aiTwisterWeights10[];
extern s32 aiAxelWeights9[], aiAxelWeights10[], aiMrSlamWeights9[], aiMrSlamWeights10[];
extern s32 aiShadowWeights9[], aiShadowWeights10[], aiMinionWeights9[], aiMinionWeights10[];
extern s32 aiDarkToothWeights9[], aiDarkToothWeights10[];

#define SET_TABLES(o, A, B) \
    (AI(o)->weapons.weights9 = (u8 *)(A), AI(o)->weapons.weights10 = (u8 *)(B))
/* request 73 (SPECIAL_TEMPLATES.md 5): a mod car takes its special template's tables and fire chance */
#ifdef TM2_PORT_SPECIAL_PROFILE
#define SETUP_TYPE(ai) portSpecialAiType(ai)
#else
#define SETUP_TYPE(ai) ((ai)->type)
#endif

/* @800D3C7C: pick the two data tables for the object type at +0x30 and set
   the strength byte at +0x812 from the two settings returned by
   gameGetLevel / aiGetDifficulty (3*a + 15*b + base, or 100); then add mode
   bonuses (+45 for modes 5/6, +15 for gameGetMode()==2, 100 for mode 3
   when aiCarCount()==1) and clamp to 100. */
void __attribute__((section(".text_800D3C7C"))) aiCarSetupProfile(struct TM2AIRecord *ai)
{
    s32 a, b;
    struct TM2CarWeapons *p;

    a = gameGetLevel();
    b = aiGetDifficulty();
    p = &ai->weapons;
    switch (SETUP_TYPE(ai)) {
    case 0x10:
        SET_TABLES(ai, aiSweetToothWeights9, aiSweetToothWeights10);
        ai->weapons.fire_chance = 100;
        break;
    case 0x20:
        SET_TABLES(ai, aiHammerheadWeights9, aiHammerheadWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x30:
        SET_TABLES(ai, aiOutlaw2Weights9, aiOutlaw2Weights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x40:
        SET_TABLES(ai, aiWarthogWeights9, aiWarthogWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 22);
        break;
    case 0x50:
        SET_TABLES(ai, aiMrGrimmWeights9, aiMrGrimmWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x60:
        SET_TABLES(ai, aiGrasshopperWeights9, aiGrasshopperWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x70:
        SET_TABLES(ai, aiThumperWeights9, aiThumperWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x80:
        SET_TABLES(ai, aiSpectreWeights9, aiSpectreWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0x90:
        SET_TABLES(ai, aiRoadKillWeights9, aiRoadKillWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 30);
        break;
    case 0xA0:
        SET_TABLES(ai, aiTwisterWeights9, aiTwisterWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0xB0:
        SET_TABLES(ai, aiAxelWeights9, aiAxelWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 17);
        break;
    case 0xC0:
        SET_TABLES(ai, aiMrSlamWeights9, aiMrSlamWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 20);
        break;
    case 0xD0:
        SET_TABLES(ai, aiShadowWeights9, aiShadowWeights10);
        ai->weapons.fire_chance = a * 3 + (b * 15 + 30);
        break;
    case 0xE0:
        SET_TABLES(ai, aiMinionWeights9, aiMinionWeights10);
        ai->weapons.fire_chance = 100;
        break;
    case 0xF0:
        SET_TABLES(ai, aiDarkToothWeights9, aiDarkToothWeights10);
        ai->weapons.fire_chance = 100;
        break;
    }
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialAiTables(ai);    /* request 73: a pickup template's or the profile's slot-0 weights */
#endif
    if (gameGetLevel() == 6 || gameGetLevel() == 5)
        p->fire_chance += 45;
    if (gameGetMode() == 2)
        p->fire_chance += 15;
    if (gameGetMode() == 3 && aiCarCount() == 1)
        p->fire_chance = 100;
    p->fire_chance = (p->fire_chance < 100) ? p->fire_chance : 100;
}

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVec;

typedef struct {
    s16 i0, i1, i2, i3;
    u8 pad[28];
} TM2TrailQuad;

typedef struct {
    u8 pad00[0x28];
    s32 x, y, z;            /* 0x28 origin */
    struct TM2SceneObject *car;     /* 0x34 owner */
    u8 pad38[8];
    s8 seg;                 /* 0x40 current segment */
    s8 slot;                /* 0x41 current vertex slot (0..6) */
    u8 pad42[2];
    s8 nseg;                /* 0x44 number of segments */
    u8 timer;               /* 0x45 */
    u8 pad46[2];
    TM2SVec v[21];          /* 0x48 */
    u8 padF0[0x60];
    TM2TrailQuad q[1];      /* 0x150 */
} TM2Trail;

/* t->v[n], written as raw address arithmetic so the addu operands match retail */
#define TVP(t, n) (*(TM2SVec *)((n) * 8 + (s32)(t) + 0x48))
#define TRAIL_VERT(t, n, out) \
    TVP(t, n).vx = (t)->car->pos[0] + (out)[0] - (t)->x; \
    TVP(t, n).vy = (t)->car->pos[1] + (out)[1] - (t)->y; \
    TVP(t, n).vz = (t)->car->pos[2] + (out)[2] - (t)->z

#define TRAIL_STEP3(t, n, in, out, d) \
    in[0] = 0; in[1] = 0; in[2] = d; \
    mathMulTransVec(&(t)->car->matrix, in, out); \
    TRAIL_VERT(t, n, out); \
    n++; \
    in[0] = d; { s32 e = -8; in[1] = 0; in[2] = e; \
    mathMulTransVec(&(t)->car->matrix, in, out); \
    TRAIL_VERT(t, n, out); \
    n++; \
    in[0] = e; in[1] = 0; in[2] = e; \
    mathMulTransVec(&(t)->car->matrix, in, out); \
    TRAIL_VERT(t, n, out); }

/* @800D9238: trail effect tick. Every second frame advance to the next
   segment / vertex slot, relink the segment's index pairs and call
   missileTrailSetUVs; every frame refresh the three vertices of the current slot
   from the owner's position plus the offsets (0,0,8), (8,0,-8), (-8,0,-8)
   rotated by its matrix. The -8 offset lives in its own
   block-local e (single-set, so local-alloc gives it s0 after d dies), and the
   vertex address is written n*8 + t so the addu operands match retail. */
void __attribute__((section(".text_800D9238"))) fxTrailUpdate(TM2Trail *t, s32 index /* unused; every retail caller passes the record index */)
{
    s32 d;
    s32 in[3], out[3];
    s32 n, k;

    d = 8;
    if (--t->timer == 0) {
        n = t->slot * 3;
        if (++t->seg >= t->nseg)
            t->seg = 0;
        k = t->seg * 3;
        t->q[k].i3 = n + 1; t->q[k].i1 = n;
        t->q[k + 1].i3 = n + 2; t->q[k + 1].i1 = n + 1;
        t->q[k + 2].i3 = n;     t->q[k + 2].i1 = n + 2;
        if (++t->slot >= 7)
            t->slot = 0;
        n = t->slot * 3;
        t->q[k].i2 = n + 1; t->q[k].i0 = n;
        t->q[k + 1].i2 = n + 2; t->q[k + 1].i0 = n + 1;
        t->q[k + 2].i2 = n;     t->q[k + 2].i0 = n + 2;
        TRAIL_STEP3(t, n, in, out, d);
        t->timer = 2;
        missileTrailSetUVs((u8 *)t);
    } else {
        n = t->slot * 3;
        TRAIL_STEP3(t, n, in, out, d);
    }
}

extern s32 mathBlendValue(s32 value, s32 target, s32 pct);
extern int ratan2(int, int);
extern int rsin(int);

/* @800E5134: per-frame car body sway; car is a human record when side is
   set, else an AI record (aim block side ? +276 : +156).
   Eases the roll angle (+24) towards a value derived from the speed and the
   steering (clamped by the steering input), the pitch angle (+22) towards
   the (clamped) slope heading; while a shake is active (+4) runs its sine
   phase (+14) to produce the offsets at +48/+50, halving the amplitudes
   (+40/+42) each period until the count (+12) runs out, then re-arms the
   shake through carShakeStart. */
void __attribute__((section(".text_800E5134"))) carBodySwayUpdate(void *car, u8 side)
{
    struct TM2CarAim *p;
    struct TM2CarMotion *q;
    struct TM2CarStatsBlock *r;
    struct TM2CarState *w;
    s32 lim;
    s16 f;
    s32 d, a, k, t;
    s32 ay;

    if (side) {
        p = &((struct TM2HumanRecord *)car)->aim;
        q = &((struct TM2HumanRecord *)car)->motion;
        r = &((struct TM2HumanRecord *)car)->stats;
        w = &((struct TM2HumanRecord *)car)->state;
        lim = ((struct TM2HumanRecord *)car)->speed_change;
    } else {
        p = &AI(car)->aim;
        q = &AI(car)->motion;
        r = &AI(car)->stats;
        w = &AI(car)->state;
        lim = 0;
    }
    if (w->all_in_air != 0)
        return;
    if (side)
        t = r->steer_limit;
    else
        t = r->steer_limit / 10;
    f = t;
    d = q->steer * r->lean_max / f;
    d = d * __builtin_abs(q->speed) / r->reverse_cap;
    if (__builtin_abs(r->lean_max) < __builtin_abs(d)) {
        if (r->lean_max > 0 ? d <= 0 : d > 0)
            d = -r->lean_max;
        else
            d = r->lean_max;
    }
    p->lean_roll = mathBlendValue(p->lean_roll, d, 90);
    a = ratan2(r->pitch_travel, r->wheelbase);
    if (lim > 0) {
        if (a < lim)
            lim = a;
    } else {
        lim = (lim < -a) ? -a : lim;
    }
    p->lean_pitch = mathBlendValue(p->lean_pitch, -lim, 90);
    if (p->shake != 0) {
        p->shake_phase += p->shake_step;
        if (p->shake_phase < 4096) {
            k = r->speed_cap / 8;
            if (k < __builtin_abs(q->speed)) {
                a = p->shake_amp_x;
                ay = p->shake_amp_y;
            } else {
                a = p->shake_amp_x * q->speed / k;
                ay = p->shake_amp_y * q->speed / k;
            }
            p->pitch = rsin(p->shake_phase) * a / 4096;
            p->f32 = rsin(p->shake_phase) * ay / 4096;
            return;
        }
        p->shake_phase -= 4096;
        if (p->shake_count > 0) {
            p->shake_count--;
            p->shake_amp_x /= 2;
            p->shake_amp_y /= 2;
            return;
        }
    } else if (p->shake_level == 0)
        return;
    carShakeStart(car, p->shake_level, p->shake_axes, 0, side);
}

/* Route nodes (24 bytes): position, up to four successors with lane offsets. */
typedef struct {
    s32 x, y;
    s16 z;
    s16 next[4];            /* +0x0A */
    s8 lane[4];             /* +0x12 */
    s8 count;               /* +0x16 */
    s8 route;               /* +0x17 */
} TM2RouteNode;

extern TM2RouteNode aiRouteNodes[];
extern int rand(void);
extern s32 aiPickLinkNode(struct TM2AIRecord *ai, TM2RouteNode *from, s32 *lane);
extern s32 sub_8013FC44(s32);
extern void mathBoundSAngle(s16 *);

/* @800F763C: choose the AI car's next route node (+1126): `node` when not
   -1, else a random successor different from the previous node (+1122) in
   mode 3, or aiPickLinkNode's pick; store its lane offset (+1148), the
   distance (+1236) and heading (+1162) from the current node (+1124), the
   target point shifted sideways by +1144 (+1260/+1264), the heading change
   (+1164) and whether it is a sharp turn (+1106). */
void __attribute__((section(".text_800F763C"))) aiChooseNextNode(struct TM2AIRecord *ai, s32 node)
{
    s32 k;
    s32 d[4];
    s32 lane;

    if (node != -1) {
        DRV(ai)->node_next = node;
        DRV(ai)->next_lane = 0;
    } else if (ai->f001 == 3) {
        k = 0;
        if (aiRouteNodes[DRV(ai)->node].count >= 2) {
            do {
                k = rand() % aiRouteNodes[DRV(ai)->node].count;
            } while (aiRouteNodes[DRV(ai)->node].next[k] == DRV(ai)->node_prev);
        }
        DRV(ai)->node_next = aiRouteNodes[DRV(ai)->node].next[k];
        DRV(ai)->next_lane = aiRouteNodes[DRV(ai)->node].lane[k];
    } else {
        s32 nn, ll;
        nn = aiPickLinkNode(ai, &aiRouteNodes[DRV(ai)->node], &lane);
        ll = lane;
        DRV(ai)->node_next = nn;
        DRV(ai)->next_lane = ll;
    }
    d[0] = aiRouteNodes[DRV(ai)->node_next].x - aiRouteNodes[DRV(ai)->node].x;
    d[1] = aiRouteNodes[DRV(ai)->node_next].y - aiRouteNodes[DRV(ai)->node].y;
    DRV(ai)->next_dist = sub_8013FC44(d[0] * d[0] + d[1] * d[1]);
    if (d[1] == 0)
        d[1] = 1;
    DRV(ai)->next_heading = ratan2(d[0], d[1]);
    mathBoundSAngle(&DRV(ai)->next_heading);
    if (DRV(ai)->side_pos) {
        DRV(ai)->goal2[0] = aiRouteNodes[DRV(ai)->node_next].x + DRV(ai)->side_offset;
        DRV(ai)->goal2[1] = aiRouteNodes[DRV(ai)->node_next].y + DRV(ai)->side_offset;
    } else if (DRV(ai)->side_neg) {
        DRV(ai)->goal2[0] = aiRouteNodes[DRV(ai)->node_next].x - DRV(ai)->side_offset;
        DRV(ai)->goal2[1] = aiRouteNodes[DRV(ai)->node_next].y - DRV(ai)->side_offset;
    } else {
        DRV(ai)->goal2[0] = aiRouteNodes[DRV(ai)->node_next].x;
        DRV(ai)->goal2[1] = aiRouteNodes[DRV(ai)->node_next].y;
    }
    DRV(ai)->turn = DRV(ai)->next_heading - DRV(ai)->heading;
    mathBoundSAngle(&DRV(ai)->turn);
    if (__builtin_abs(DRV(ai)->turn) >= 1593)
        DRV(ai)->sharp_turn = 1;
    else
        DRV(ai)->sharp_turn = 0;
}

extern s32 carDefaultTopSpeed(s32);
extern void aiSetPassLane(struct TM2AIRecord *ai, s32 mode);
extern void aiDarkToothJumpSelect(struct TM2AIRecord *ai);

#define ROUTE_LEVEL(ai, v) carStatsSetTopSpeed(&(ai)->stats, (v))

/* @800F68D8: AI car reached its target node: shift the node history
   (+1122 <- +1124 <- +1126), heading/distance/lane, set the speed level
   from the lane (-2..1 -> 1/4..2/3 of carDefaultTopSpeed) unless +113 is set,
   choose the side offset and side (+1104/+1105), shift the target points,
   flag a route change (+1108), store the node delta (+1180/+1184) with
   its axis-alignment flags (+1098..+1100) and the corner point
   (+1204/+1208), then reset the follow state (+1096/+1126/+1138/+1150). */
void __attribute__((section(".text_800F68D8"))) aiAdvanceRouteNode(struct TM2AIRecord *ai)
{
    DRV(ai)->node_prev = DRV(ai)->node;
    DRV(ai)->node = DRV(ai)->node_next;
    DRV(ai)->heading = DRV(ai)->next_heading;
    DRV(ai)->seg_dist = DRV(ai)->next_dist;
    DRV(ai)->lane = DRV(ai)->next_lane;
    if (ai->state.f05 == 0) {
        if (DRV(ai)->lane == -2)
            ROUTE_LEVEL(ai, TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) / 4);
        else if (DRV(ai)->lane == -1)
            ROUTE_LEVEL(ai, TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) / 3);
        else if (DRV(ai)->lane == 0)
            ROUTE_LEVEL(ai, TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) / 2);
        else if (DRV(ai)->lane == 1)
            ROUTE_LEVEL(ai, TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) * 2 / 3);
        else
            ROUTE_LEVEL(ai, TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type));
    }
    if (DRV(ai)->lane <= 0) {
        DRV(ai)->side_pos = 0;
        DRV(ai)->side_neg = 0;
    } else {
        if (ai->type != 0xF0 && DRV(ai)->lane == 3)
            DRV(ai)->side_offset = 72;
        else
            DRV(ai)->side_offset = 0;
        if (ai->index & 1) {
            DRV(ai)->side_pos = 1;
            DRV(ai)->side_neg = 0;
        } else {
            DRV(ai)->side_pos = 0;
            DRV(ai)->side_neg = 1;
        }
    }
    DRV(ai)->goal_prev[0] = DRV(ai)->goal[0];
    DRV(ai)->goal_prev[1] = DRV(ai)->goal[1];
    DRV(ai)->goal[0] = DRV(ai)->goal2[0];
    DRV(ai)->goal[1] = DRV(ai)->goal2[1];
    if (aiRouteNodes[DRV(ai)->node_prev].route == aiRouteNodes[DRV(ai)->node].route)
        DRV(ai)->route_change = 0;
    else
        DRV(ai)->route_change = 1;
    aiSetSegmentSpeed(ai, 0);
    DRV(ai)->f4D8 = DRV(ai)->f484;
    DRV(ai)->delta[0] = aiRouteNodes[DRV(ai)->node].x - aiRouteNodes[DRV(ai)->node_prev].x;
    DRV(ai)->delta[1] = aiRouteNodes[DRV(ai)->node].y - aiRouteNodes[DRV(ai)->node_prev].y;
    DRV(ai)->axis_x = __builtin_abs(DRV(ai)->delta[0]) < 40;
    DRV(ai)->axis_y = __builtin_abs(DRV(ai)->delta[1]) < 40;
    DRV(ai)->diagonal = !DRV(ai)->axis_x && !DRV(ai)->axis_y;
    if (DRV(ai)->diagonal) {
        DRV(ai)->corner[0] = aiRouteNodes[DRV(ai)->node_prev].x + DRV(ai)->delta[0] / 2;
        DRV(ai)->corner[1] = aiRouteNodes[DRV(ai)->node_prev].y + DRV(ai)->delta[1] / 2;
    } else if (DRV(ai)->axis_x)
        DRV(ai)->corner[0] = aiRouteNodes[DRV(ai)->node_prev].x;
    else
        DRV(ai)->corner[1] = aiRouteNodes[DRV(ai)->node_prev].y;
    aiSetPassLane(ai, 0);
    DRV(ai)->f448 = 0;
    DRV(ai)->node_next = DRV(ai)->node_prev;
    DRV(ai)->f472 = 0;
    DRV(ai)->turned = 0;
    if (ai->type == 0xF0)
        aiDarkToothJumpSelect(ai);
}

