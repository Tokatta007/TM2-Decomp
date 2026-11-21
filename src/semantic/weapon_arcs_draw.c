/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"   /* u8..u32, car record blocks, TM2CarSlam */
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */
#include "semantic/spu_voices.h"   /* request 87: the grab's sound carries the car (TM2_VOICE_TAG) */
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */

#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

extern int rand(void);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);

extern s32 slamClawGetAngle(void);
extern s32 slamArmGetAngle(void);
/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern void mathBoundAngle(s32 *);
#include "semantic/weapon_calls.h"

/* Rumble command for the human side only (CPU side sends zeros). */
static __inline__ void gc15_feedback(struct TM2CarMotion *motion, s32 command, s32 side)
{
    s32 a;
    if (side) {
        soundPlay(command, 0, 0, 0x63);
    } else {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(motion->pos);
        else
            soundLocateForView(motion->pos, -1);
        a = PadChkMtap();
        soundPlay(command, a, soundGetListenerPan(), 0x63);
    }
}

#define GC15_LAMP_CHECK()                                   \
    if (angle >= 569 && state->spin_pending != 0)           \
        carSpinOutByObject(state->spin_target, 0, 1)

/* @800F4CC0: advance one side's (side&0xff: 1 = slam record at human weapons.slam (+0x810), 0 = AI (+0x850)) staged boot/lid sequence: open the
   two lid angles, swing the bound object out/in with lamps and feedback, count down the hold timer, release the
   bound object, then close both angles and clear the state. */
void __attribute__((section(".text_800F4CC0"))) weaponSlamGrabUpdate(void *arg, s32 arg1)
{
    void *car = arg;        /* human (side) or AI record; the copies keep retail's registers */
    s32 side = arg1;
    s32 car_id;
    struct TM2CarStatsBlock *geometry;
    struct TM2CarMotion *motion;
    s32 angle;
    u8 done1;
    u8 done2;
    struct TM2CarSlam *state;

    if (side & 0xff) {
        state = HUMAN->weapons.slam;
        car_id = HUMAN->slot;
        geometry = &HUMAN->stats;
        motion = &HUMAN->motion;
    } else {
        state = AI->weapons.slam;
        car_id = AI->slot;
        geometry = &AI->stats;
        motion = &AI->motion;
    }
    if (state->grab == 0)
        return;
    TM2_VOICE_TAG(car_id, TM2_VOICE_SPECIAL);    /* request 87: Mr. Slam's special */
    gc15_feedback(motion, 0x39, TM2_CAR_FEEDBACK(side & 0xff, HUMAN->player));   /* H3 */
    TM2_VOICE_UNTAG();
    if (state->f06 != 0) {
        if (state->spin_pending != 0)
            weaponSlamCarryTarget(state->spin_target, geometry, motion, 0, 0);
        else
            weaponSlamFindTarget(car, side & 0xff);
        angle = slamClawGetAngle();
        if (angle < -681) {
            done1 = 1;
        } else {
            angle -= timerGetFrameTicks() * 68;
            mathBoundAngle(&angle);
            if (angle < -682)
                angle = -682;
            slamClawSetAngle(angle);
            done1 = 0;
        }
        angle = slamArmGetAngle();
        done2 = 1;
        if (angle < 910) {
            angle += timerGetFrameTicks() * 56;
            mathBoundAngle(&angle);
            if (angle >= 911)
                angle = 910;
            slamArmSetAngle(angle);
            done2 = 0;
        }
        if (done1 && done2) {
            state->f06 = 0;
            if (state->spin_pending != 0) {
                state->f08 = 1;
                state->f03 = 1;
                state->f09 = 0;
                state->f0A = 0;
            } else {
                state->f08 = 0;
                state->f09 = 1;
            }
        }
        return;
    }
    if (state->f08 != 0) {
        if (state->f03 != 0) {
            angle = slamArmGetAngle();
            GC15_LAMP_CHECK();
            if (angle >= 114) {
                angle -= timerGetFrameTicks() * 34;
                mathBoundAngle(&angle);
                if (angle < 113)
                    angle = 113;
                slamArmSetAngle(angle);
                weaponSlamCarryTarget(state->spin_target, geometry, motion, 1, 1);
            } else {
                state->f03 = 0;
            }
            return;
        }
        if (state->f0A != 0) {
            angle = slamArmGetAngle();
            GC15_LAMP_CHECK();
            if (angle < 114)
                return;
            weaponSlamCarryTarget(state->spin_target, geometry, motion, 1, 1);
            angle -= timerGetFrameTicks() * 34;
            mathBoundAngle(&angle);
            if (angle < 113) {
                angle = 113;
                state->f08 = 0;
                state->f09 = 1;
                carSpinOutById(state->spin_target, motion->rot[2], 1);
                state->spin_target = -1;
                state->spin_pending = 0;
            }
            slamArmSetAngle(angle);
            return;
        }
        angle = slamArmGetAngle();
        GC15_LAMP_CHECK();
        if (angle < 910) {
            weaponSlamCarryTarget(state->spin_target, geometry, motion, 1, 0);
            angle += timerGetFrameTicks() * 68;
            mathBoundAngle(&angle);
            if (angle >= 911) {
                angle = 910;
                gc15_feedback(motion, 2, TM2_CAR_FEEDBACK(side & 0xff, HUMAN->player));
                if (state->spin_pending != 0)
                    carSpinOutByObject(state->spin_target, 0, 1);
            }
            slamArmSetAngle(angle);
            return;
        }
        if (state->half_damage != 0)
            carDamageByObject(state->spin_target, car_id, 5);
        else
            carDamageByObject(state->spin_target, car_id, 10);
        state->hold--;
        if (state->hold <= 0) {
            state->f08 = 0;
            state->f09 = 1;
            carSpinOutById(state->spin_target, motion->rot[2], 0);
            state->spin_target = -1;
            state->spin_pending = 0;
            if (rand() & 1)
                state->f0A = 1;
        } else {
            state->f03 = 1;
        }
        return;
    }
    angle = slamClawGetAngle();
    if (angle < 739) {
        angle += timerGetFrameTicks() * 45;
        mathBoundAngle(&angle);
        slamClawSetAngle(angle);
    }
    angle = slamArmGetAngle();
    GC15_LAMP_CHECK();
    if (angle >= -112) {
        angle -= timerGetFrameTicks() * 34;
        mathBoundAngle(&angle);
        slamArmSetAngle(angle);
    }
    if (slamClawGetAngle() >= 739 && slamArmGetAngle() < -112) {
        angle = 739;
        slamClawSetAngle(angle);
        angle = -113;
        slamArmSetAngle(angle);
        state->grab = 0;
        state->active = 0;
    }
}

struct GC15Taser {
    s16 id;
    s16 source;
    s16 count;
    s16 attachment;
    s16 target;
    s16 pad;
    s32 *from;
    s32 *to;
};
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern struct GC15Taser weaponArcs[TM2_MAX_ARCS];
/* libgpu LINE_F2 packet (16 bytes; the packet buffer steps 64 per line). */
struct GC15LineF2 {
    u8 tag[3];
    u8 len;                 /* +3 words after the tag */
    u8 r0, g0, b0, code;    /* +4 */
    s16 x0, y0, x1, y1;     /* +8 */
};
extern s32 weaponArcsActive;
extern void viewGetCentre(s32, s32 *, s32 *);
extern s32 viewGetProjScaleX(s32);
extern s32 viewGetProjScaleY(s32);
extern s32 *viewGetEyeTrans(s32);
extern s16 *viewGetViewMatrix(s32);
extern s32 rcos(s32);
extern s32 rsin(s32);
extern void mathMulMatVec(s16 *, s32 *, s32 *);
extern void fxSmokeC12(s32 *, s32);
extern void fxExplosion12(s32 *, u8 flag);
extern void carApplyDamage(s32, s32, s32 *, s32, s32);
extern void AddPrim(void *, void *);
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 41 bit 16 (BALANCE.md 12.4) */
#endif
/* Request 42 items 6 and 7 (port_visual.h, portVisual, read live; 0 = retail):
   ARC_OT(j)        bit 0: segment j's OT entry at the depth of its nearer end (portVisualArcOtz), retail: entry 0;
   ARC_FROM(i)      bit 1 (request 76): the taser's (kind 0) start, Outlaw 2's light bar (portVisualTaserFrom, into
                    the local `at`); retail and the lightning: the shooter's position;
   ARC_LIFT(i)      bit 1: the start's lift over ARC_FROM, 0 (the bar's height is in it; retail and the lightning: 30);
   ARC_JIT(i, j, r) the jitter r of point j: retail's with any bit (request 88: bit 1 no longer scales it down at
                    points 1 and 2; the user wants retail's shake); the rand() call that feeds r is retail's. */
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"
#define ARC_TASER_LOW(i) ((portVisual & TM2_VIS_TASER_LOW) && weaponArcs[i].attachment == 0)
#define ARC_FROM(i) from
#define ARC_LIFT(i) (ARC_TASER_LOW(i) ? 0 : 30)
/* one rand() call site per axis, as retail's (request 76 shifted points 1 and 2 right with bit 1; request 88: retail's) */
#define ARC_JIT(i, j, r) (r)
#define ARC_OT(j) ((portVisual & TM2_VIS_ARC_DEPTH) ? ot + 4 * portVisualArcOtz(scr[j][2], scr[(j) + 1][2]) : ot)
#else
#define ARC_FROM(i) weaponArcs[i].from
#define ARC_LIFT(i) 30
#define ARC_JIT(i, j, r) (r)
#define ARC_OT(j) ot
#endif
/* Request 48 stage 2 (ONLINE_ROLLBACK.md 11, port_rollback.h; portRollbackOptions, 0 = retail): ONCE_PER_STEP
   splits the function by pass (portRollbackPass): the step's call (context 0, from portRollbackStepEffects) runs
   the tick only (weaponArcsActive, the taser retry, the smoke / explosion rand(), the countdown and the damage), a
   view's call only draws (it writes no state: the free end goes to a local pointer, not weaponArcs[i].to); the
   jitter of a drawing pass, and with VISUAL_RAND of every pass, comes from the visual stream (ARC_RAND; retail's
   rand() call stays at each site for the other case).
   ARC_TO(i)  the arc's end: retail weaponArcs[i].to; with the define a local copy (the free end when untargeted) */
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"
#define ARC_TO(i) to
#define ARC_RAND() (vis ? portRollbackVisualRand() : rand())
#else
#define ARC_TO(i) weaponArcs[i].to
#define ARC_RAND() rand()
#endif

/* @801084D8 (weaponArcsDraw): for each of the 5 active taser slots (weaponArcs), build a jittered 16-point arc from the
   source point to the target (or a swinging free end when untargeted), project it through view arg1, spawn sparks,
   count the slot down (hit effects at 39 and 0 ticks), and emit up to 15 LINE_F2 segments (orange when attached,
   blue otherwise) into the ordering table/packet buffer at arg0. */
void __attribute__((section(".text_801084D8"))) weaponArcsDraw(u8 **ctx, s32 view)
{
    s32 free_end[3];
    s32 tmp[3];
    s32 pts[16][3];
    s32 scr[16][3];
    s32 d[3];
    s32 ofs_x, ofs_y;
    s32 i;
    s16 *mat;
    u8 *ot;
    s32 proj_x, proj_y;
    s32 *eye;
    s32 j;
    s32 *p;
    s32 x, y, z;
    struct GC15LineF2 *prim;
#ifdef TM2_PORT_VISUAL
    s32 at[3];  /* bit 1: the taser's start (ARC_FROM) */
    s32 *from;
#endif
#ifdef TM2_PORT_ROLLBACK
    s32 pass;   /* TM2_RB_PASS_*: BOTH (retail), TICK (the step's call, ctx 0), DRAW (a view's call) */
    s32 vis;    /* 1: the jitter from the visual stream */
    s32 *to;

    pass = portRollbackPass(ctx == 0);
    vis = portRollbackArcVisual(pass);
    if (pass != TM2_RB_PASS_TICK) {
#endif
    ot = ctx[0];
    viewGetCentre(view, &ofs_x, &ofs_y);
    proj_x = viewGetProjScaleX(view);
    proj_y = viewGetProjScaleY(view);
    eye = viewGetEyeTrans(view);
    mat = viewGetViewMatrix(view);
#ifdef TM2_PORT_ROLLBACK
    }
    if (pass != TM2_RB_PASS_DRAW)
#endif
    weaponArcsActive = 0;
    for (i = 0; i < TM2_MAX_ARCS; i++) {
        if (weaponArcs[i].count <= 0)
            continue;
#ifdef TM2_PORT_ROLLBACK
        to = weaponArcs[i].to;
        if (pass != TM2_RB_PASS_DRAW) {
#endif
        weaponArcsActive = 1;
#ifdef TM2_PORT_BALANCE
        /* request 41 bit 16 (BALANCE.md 12.4): a taser shot that connected to no one may still connect */
        if (weaponArcs[i].target < 0)
            portBalanceTaserRetry(i);
#endif
#ifdef TM2_PORT_ROLLBACK
            to = weaponArcs[i].to;      /* the retry may have connected it */
        }
#endif
        if (weaponArcs[i].target < 0) {
#ifdef TM2_PORT_ROLLBACK
            to = free_end;
            if (pass != TM2_RB_PASS_DRAW)   /* retail's store (the drawing pass writes no state) */
#endif
            weaponArcs[i].to = free_end;
            free_end[0] = weaponArcs[i].from[0] + rcos(weaponArcs[i].count * 400) / 5;
            free_end[1] = weaponArcs[i].from[1] + rsin(weaponArcs[i].count * 400) / 5;
            free_end[2] = weaponArcs[i].from[2];
        }
#ifdef TM2_PORT_ROLLBACK
        if (pass == TM2_RB_PASS_TICK)
            goto tick;      /* ONCE_PER_STEP, the step's call: no points, no jitter, no drawing */
#endif
#ifdef TM2_PORT_VISUAL
        from = ARC_TASER_LOW(i) ? portVisualTaserFrom(weaponArcs[i].from, at) : weaponArcs[i].from;
#endif
        d[0] = ARC_TO(i)[0] - ARC_FROM(i)[0];
        d[1] = ARC_TO(i)[1] - ARC_FROM(i)[1];
        d[2] = ARC_TO(i)[2] - ARC_FROM(i)[2];
        pts[0][0] = ARC_FROM(i)[0];
        pts[0][1] = ARC_FROM(i)[1];
        p = pts[0];
        p[2] = ARC_FROM(i)[2] + ARC_LIFT(i);
        p = pts[15];
        p[0] = ARC_TO(i)[0] + ARC_RAND() / 819;
        p[1] = ARC_TO(i)[1] + ARC_RAND() / 819;
        p[2] = ARC_TO(i)[2] + ARC_RAND() / 819;
        for (j = 1; j < 15; j++) {
            *(s32 *)&pts[j][0] = ARC_FROM(i)[0] + d[0] * j / 16;
            *(s32 *)&pts[j][1] = ARC_FROM(i)[1] + d[1] * j / 16;
            *(s32 *)&pts[j][2] = ARC_FROM(i)[2] + ARC_LIFT(i) + d[2] * j / 16;
            pts[j][2] += ARC_JIT(i, j, (ARC_RAND() & 0x3f) - 32);
            pts[j][1] += ARC_JIT(i, j, (ARC_RAND() & 0x3f) - 32);
            pts[j][0] += ARC_JIT(i, j, (ARC_RAND() & 0x3f) - 32);
        }
        for (j = 0; j < 16; j++) {
            tmp[0] = pts[j][0] + eye[0];
            tmp[1] = pts[j][1] + eye[1];
            tmp[2] = pts[j][2] + eye[2];
            p = scr[j];
            mathMulMatVec(mat, tmp, p);
            if ((z = p[2]) > 0) {
                x = p[0] * proj_x / z;
                y = p[1] * proj_y / p[2];
                scr[j][0] = ofs_x + x;
                scr[j][1] = ofs_y + y;
                scr[j][2] = p[2] / 4;
                if ((u32)(p[0] + 4000) > 8000)
                    p[2] = 0;
                if ((u32)(p[1] + 4000) > 8000)
                    p[2] = 0;
            }
        }
#ifdef TM2_PORT_ROLLBACK
    tick:
        if (pass != TM2_RB_PASS_DRAW) {     /* the tick: the step's call, or retail's combined pass */
#endif
        if (rand() < 512)
            fxSmokeC12(ARC_TO(i), 0);
        if (rand() < 512)
            fxExplosion12(ARC_TO(i), 0);
        weaponArcs[i].count--;
        if (weaponArcs[i].target >= 0 && weaponArcs[i].count == 39)
            carApplyDamage(weaponArcs[i].target, TM2_SPECIAL_NUM(weaponArcs[i].source, TM2_SPN_DAMAGE, 10), 0, 2,
                           weaponArcs[i].source);
        if (weaponArcs[i].target >= 0 && weaponArcs[i].count == 0)
            carApplyDamage(weaponArcs[i].target, TM2_SPECIAL_NUM(weaponArcs[i].source, TM2_SPN_DAMAGE, 10), 0, 2,
                           weaponArcs[i].source);
#ifdef TM2_PORT_ROLLBACK
        }
        if (pass == TM2_RB_PASS_TICK)
            continue;
#endif
        for (j = 0; j < 15; j++) {
            if (scr[j][2] > 0 && scr[j + 1][2] > 0 && scr[j][2] < 4091 && scr[j + 1][2] < 4091) {
                prim = (struct GC15LineF2 *)ctx[2];
                if (ctx[2] + 64 < ctx[3]) {
                    prim->len = 3;
                    prim->code = 0x40;
                    ctx[2] += 64;
                    if (weaponArcs[i].attachment != 0) {
                        prim->r0 = 255;
                        prim->g0 = 200;
                        prim->b0 = 50;
                    } else {
                        prim->r0 = 170;
                        prim->g0 = 180;
                        prim->b0 = 255;
                    }
                    prim->x0 = scr[j][0];
                    prim->y0 = scr[j][1];
                    prim->x1 = scr[j + 1][0];
                    prim->y1 = scr[j + 1][1];
                    AddPrim(ARC_OT(j), prim);
                }
            }
        }
    }
}
