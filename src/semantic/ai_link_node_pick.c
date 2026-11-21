/* Game code (800E..8010 range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/port_balance.h"  /* request 91 item 3 (BALANCE.md 20) */

#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Mat;

extern u8 worldSectionHasFloor[];
extern u8 worldSectionHasDrop[];
extern u8 worldSectionFirstSurface[];
extern u8 worldSectionLastSurface[];
extern struct { s32 v; s32 b; s32 c; } worldSectionFloorHeights[];
extern s32 timerGetFrameTicks(void);

/* Wheel ground update: switch the wheel's section (kind) to `slot`, looking up the
   per-slot value table; starts a countdown / latches values depending on the delta. */
void __attribute__((section(".text_800EFCBC"))) wheelSectionUpdate(struct TM2CarWheel *w, s32 slot, s32 limit, u8 force, u8 moving /* unused; the retail caller passes it */)
{
    s32 oldv;
    s32 newv;
    s32 diff;
    s32 margin;

    oldv = 0;
    if (worldSectionHasFloor[w->kind] != 0)
        oldv = worldSectionFloorHeights[w->kind].v;
    if (worldSectionHasFloor[slot] != 0)
        newv = worldSectionFloorHeights[slot].v;
    else
        newv = 0;

    if (w->kind != slot) {
        diff = newv - oldv;
        if (worldSectionHasDrop[w->kind] == 0 && diff < 0)
            diff = 0;
        margin = w->pos[2] - newv;
        if (diff > 0) {
            if (-limit < margin || force) {
                w->kind = slot;
                w->f26 = newv;
                if (w->touch != 0) {
latch:
                    w->level = w->pos[2];
                    goto copy;
                }
            }
        } else if (diff < 0) {
            w->kind = slot;
            w->f26 = newv;
            if (w->on != 0)
                goto check;
            if (w->touch == 0) {
                w->touch = 1;
                w->f12 = 0;
                w->land_z = w->pos[2];
            }
            w->on = 1;
            w->fall = -32 / (s16)timerGetFrameTicks();
            w->depth = 0;
            goto latch;
        } else {
            w->kind = slot;
            w->f26 = newv;
            if (w->on != 0) {
check:
                if (w->surface < worldSectionFirstSurface[slot] || w->surface > worldSectionLastSurface[slot]) {
copy:
                    w->h = w->f26;
                }
            }
        }
    } else {
        if (w->f26 != newv)
            w->f26 = newv;
    }
}

extern void *RotMatrixYXZ(s16 *, TM2Mat *);
extern void mathMulTransVec(TM2Mat *, s32 *, s32 *);

/* Compute the AI car's offset from one of two edge lines (side select `which`),
   keep the dominant axis, then rotate it into the heading frame into out[3]. */
void __attribute__((section(".text_800F37B4"))) aiRouteLateralOffset(struct TM2AIRecord *ai, s32 *out, u8 which)
{
    s32 active;
    s32 axis;
    s32 unused;
    s32 dx, dz, ox, oz, ex, ez;
    s32 x, z;
    s16 rot[4];
    s32 vec[4];
    TM2Mat mat;

    if (which) {
        active = DRV(ai)->diagonal;
        axis = DRV(ai)->axis_x;
        unused = DRV(ai)->axis_y;
        dx = DRV(ai)->delta[0];
        dz = DRV(ai)->delta[1];
        ox = DRV(ai)->goal_prev[0];
        oz = DRV(ai)->goal_prev[1];
        ex = DRV(ai)->corner[0];
        ez = DRV(ai)->corner[1];
    } else {
        active = DRV(ai)->next_diagonal;
        axis = DRV(ai)->next_axis_x;
        unused = DRV(ai)->next_axis_y;
        dx = DRV(ai)->next_delta[0];
        dz = DRV(ai)->next_delta[1];
        ox = DRV(ai)->goal[0];
        oz = DRV(ai)->goal[1];
        ex = DRV(ai)->next_corner[0];
        ez = DRV(ai)->next_corner[1];
    }

    if ((u8)active) {
        if (dz)
            out[0] = ai->motion.pos[0] - ox - dx * (ai->motion.pos[1] - oz) / dz;
        else
            out[0] = 0;
        if (dx) {
            out[1] = ai->motion.pos[1] - oz - dz * (ai->motion.pos[0] - ox) / dx;
            x = out[0];
            z = out[1];
            if (x < 0)
                x = -x;
            if (z < 0)
                z = -z;
            if (z < x) {
                out[0] = 0;
                goto rotate;
            }
        }
        out[1] = 0;
    } else if ((u8)axis) {
        out[0] = ai->motion.pos[0] - ex;
        out[1] = 0;
    } else {
        out[0] = 0;
        out[1] = ai->motion.pos[1] - ez;
    }
rotate:
    rot[0] = 0;
    rot[1] = 0;
    rot[2] = -DRV(ai)->heading;
    RotMatrixYXZ(rot, &mat);
    vec[0] = out[0];
    vec[1] = out[1];
    vec[2] = 0;
    mathMulTransVec(&mat, vec, out);
}

typedef struct {
    s32 x;
    s32 z;
    u8 pad[15];
    s8 group;
} TM2RouteNode;

typedef struct {
    u8 pad[10];
    s16 nodes[4];
    s8 groups[4];
    s8 count;
} TM2RouteLink;

extern TM2RouteNode aiRouteNodes[];
extern s32 aiFollowRouteTarget(struct TM2AIRecord *ai, s32 *out);

/* AI route choice: among the link's candidate nodes pick the nearest (or, in mode 2, the
   farthest) node to the car, avoiding repeating the current node more than 3 times.
   Writes the chosen node's group to *outGroup, returns the node index. */
s32 __attribute__((section(".text_800F7B00"))) aiPickLinkNode(struct TM2AIRecord *ai, TM2RouteLink *link, s32 *outGroup)
{
    s32 pos[4];
    s32 grp;
    s32 i;
    s32 best;
    s32 bestDist;
    s32 node;
    s32 dist;
    s32 g;

    grp = aiFollowRouteTarget(ai, pos);
    best = link->nodes[0];
    *outGroup = link->groups[0];
    bestDist = 0x7FFFFFFF;
    if (ai->f001 == 2)
        bestDist = 0;
    for (i = 0; i < link->count; i++) {
        node = link->nodes[i];
        if (ai->f001 < 2 && grp >= 0 && aiRouteNodes[node].group != grp)
            continue;
#ifdef TM2_PORT_BALANCE
        if (portBalanceJamAvoid(ai, node))  /* request 91 item 3 (bit 27): the node it was jammed toward */
            continue;
#endif
        dist = (pos[0] - aiRouteNodes[node].x) * (pos[0] - aiRouteNodes[node].x)
             + (pos[1] - aiRouteNodes[node].z) * (pos[1] - aiRouteNodes[node].z);
        g = link->groups[i];
        if (ai->f001 == 2) {
            if (bestDist < dist) {
                if (node != DRV(ai)->node_prev || DRV(ai)->node_repeat < 3) {
                    best = node;
                    bestDist = dist;
                    *outGroup = g;
                }
            }
        } else if (dist < bestDist) {
            if (node != DRV(ai)->node_prev) {
                best = node;
                bestDist = dist;
                *outGroup = g;
            } else if (DRV(ai)->node_repeat < 3) {
                best = node;
                bestDist = dist + 80;
                *outGroup = g;
            }
        }
    }
    if (best == DRV(ai)->node_prev)
        DRV(ai)->node_repeat++;
    else
        DRV(ai)->node_repeat = 0;
    return best;
}

extern s32 rand(void);
extern s32 mathJitterResult[];

/* Random jitter: returns a static vector = pos +/- rand()/range on each axis (axes with
   range 0 keep their previous value). */
s32 *__attribute__((section(".text_8010BDE8"))) mathJitterPos(s32 *pos, s32 rx, s32 ry, s32 rz)
{
    if (rx) {
        if (rand() & 1)
            mathJitterResult[0] = pos[0] + rand() / rx;
        else
            mathJitterResult[0] = pos[0] - rand() / rx;
    }
    if (ry) {
        if (rand() & 1)
            mathJitterResult[1] = pos[1] + rand() / ry;
        else
            mathJitterResult[1] = pos[1] - rand() / ry;
    }
    if (rz) {
        if (rand() & 1)
            mathJitterResult[2] = pos[2] + rand() / rz;
        else
            mathJitterResult[2] = pos[2] - rand() / rz;
    }
    return mathJitterResult;
}

extern s32 humanCarCount(void);
extern struct TM2Object *humanCarObject(s32);
extern void clutAnimStartFadeOut(s32, s32, s32);
extern void clutAnimFadeIn(s32, s32);
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_CAR_CLUTS)
#include "semantic/tex_banks.h"   /* request 32 step 4; request 33-1 (car_cluts.h) */
#endif
extern void carBodyObjectShow(s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void fxSparkBlue(s32 *, u8 flag);

/* Per-player invisibility tick: counts down weapons.invis_timer of each human car,
   firing start/stop hooks at 120/95/25 and jitter + pad feedback outside 26..94. */
void __attribute__((section(".text_8010DE54"))) weaponInvisibilityUpdate(void)
{
    s32 n;
    s32 i;
    struct TM2Object *car;
    struct TM2HumanRecord *st;
    s32 *v;
    s32 a;
    s32 *pad;

    n = humanCarCount();
    for (i = 0; i < n; i++) {
        car = humanCarObject(i);
        st = humanCarGet(i);
        if (st->weapons.invisible == 0)
            continue;
        if (st->weapons.invis_timer == 120) {
#ifdef TM2_PORT_TEX_SHARED
            clutAnimStartFadeOut(portTexFadeId(st->slot, st->type), 0, 20);   /* step 4 / 33-1: this car's fade */
#else
            clutAnimStartFadeOut(st->type + 8, 0, 20);
#endif
#ifdef TM2_PORT_CAR_GROUPS
            carBodyObjectHide(TM2_CAR_KEY(st->slot, st->type));   /* request 32 */
#else
            carBodyObjectHide(st->type);
#endif
        }
        if (st->weapons.invis_timer == 95)
            car->visible = 0;
        if (st->weapons.invis_timer == 25) {
#ifdef TM2_PORT_TEX_SHARED
            clutAnimFadeIn(portTexFadeId(st->slot, st->type), 0);   /* request 32 step 4 / 33-1 */
#else
            clutAnimFadeIn(st->type + 8, 0);
#endif
#ifdef TM2_PORT_CAR_GROUPS
            carBodyObjectShow(TM2_CAR_KEY(st->slot, st->type));   /* request 32 */
#else
            carBodyObjectShow(st->type);
#endif
            car->visible = 1;
        }
        st->weapons.invis_timer--;
        if (st->weapons.invis_timer >= 95 || st->weapons.invis_timer < 26) {
            pad = car->pos;
            v = mathJitterPos(pad, 0x333, 0x333, 0x333);
            if (viewGetSplitMode() == 0)
                soundLocateForView0(pad);
            else
                soundLocateForView(pad, -1);
            a = PadChkMtap();
            soundPlay(22, a, soundGetListenerPan(), 99);
            fxSparkBlue(v, 1);
        }
        if (st->weapons.invis_timer <= 0)
            st->weapons.invisible = 0;
    }
}
