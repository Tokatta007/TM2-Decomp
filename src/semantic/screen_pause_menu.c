/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/scene.h"
#include "semantic/settings.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#include "semantic/port_rollback.h"   /* request 48: PAUSE_PORT */
#include "semantic/port_balance.h"    /* request 101 (BALANCE.md 22) */
#ifdef TM2_PORT_DARK_TOOTH
/* request 33-2e, portCarOptions bit 6: Hong Kong's warps only on level 8 (SURVIVAL.md 1.5 #4); request 42 item 1:
   never for an ordinary Dark Tooth (bit 10; `ai` is aiFollowRouteTarget's record) */
#define TM2_HK_WARPS ((!TM2_PORT_BOSS_ANYWHERE || gameGetLevel() == 8) && !TM2_DT_ORDINARY(ai->slot))
#else
#define TM2_HK_WARPS 1
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)
/* pauseMenu's password line: retail shows it for every car but 12 and 13 (the 4-bit car field holds 0..11).
   Request 42 (DARK_TOOTH_PLAYER.md 10.2, PER_CAR_PLAN.md E.5): also none for Dark Tooth (14, TM2_PORT_DT_PLAYER: retail
   encoded 16 * (14 + 4) past the field) and a mod's number (TM2_PORT_CAR_NUMBERS) */
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
#define TM2_PAUSE_PASSWORD_CAR(car) ((unsigned int)(car) < 12)
#else
#define TM2_PAUSE_PASSWORD_CAR(car) ((car) != 13)
#endif

/* 36-byte moving-object records @80187448 */
typedef struct {
    u8 *obj;      /* +0x00 */
    s32 delta;    /* +0x04 */
    s32 f8;       /* +0x08 */
    s32 fC;       /* +0x0C */
    s32 links;    /* +0x10 */
    s32 id;       /* +0x14 */
    u8 b18, b19, b1A, pad1B;
    s8 nb[6];     /* +0x1C */
    u8 pad22[2];
} G10Rec36;
extern G10Rec36 worldCollapseSections[];
extern s32 worldCollapseLinkTotal, worldCollapseTriggersDone, worldCollapseTimer, worldCollapseDebrisCooldown, worldCollapseDebrisCount;
extern u8 worldCollapseTriggerActive;

#define NB6(r, a, b, c, d, e, f) \
    worldCollapseSections[r].nb[0] = a; worldCollapseSections[r].nb[1] = b; worldCollapseSections[r].nb[2] = c; \
    worldCollapseSections[r].nb[3] = d; worldCollapseSections[r].nb[4] = e; worldCollapseSections[r].nb[5] = f;

/* @80115C84: reset the 35 moving-object records and fill in the static id/link/neighbour tables. */
void __attribute__((section(".text_80115C84"))) worldCollapseReset(void)
{
    s32 i;
    G10Rec36 *p;

    worldCollapseLinkTotal = 0;
    worldCollapseTriggersDone = 0;
    worldCollapseTimer = -720;
    worldCollapseDebrisCooldown = 0;
    worldCollapseDebrisCount = 0;
    worldCollapseTriggerActive = 0;
    for (i = 0; i < 35; i++) {
        p = &worldCollapseSections[i];
        p->b18 = 0;
        p->b1A = 0;
        p->b19 = 0;
        p->delta = 0;
        p->f8 = 0;
        p->links = 1;
        p->fC = 0;
        p->obj = 0;
        p->nb[0] = -1;
        p->nb[1] = -1;
        p->nb[2] = -1;
        p->nb[3] = -1;
        p->nb[4] = -1;
        p->nb[5] = -1;
    }
    worldCollapseSections[6].links = 3;
    worldCollapseSections[13].links = 2;
    worldCollapseSections[19].links = 2;
    worldCollapseSections[22].links = 2;
    worldCollapseSections[26].links = 2;
    worldCollapseSections[0].nb[0] = 1;
    worldCollapseSections[0].nb[1] = 11;
    worldCollapseSections[0].nb[2] = 10;
    worldCollapseSections[0].id = 40;
    worldCollapseSections[1].nb[1] = 2;
    worldCollapseSections[1].nb[2] = 11;
    worldCollapseSections[1].nb[3] = 0;
    worldCollapseSections[1].id = 43;
    worldCollapseSections[2].nb[2] = 12;
    worldCollapseSections[2].nb[3] = 11;
    worldCollapseSections[2].nb[4] = 1;
    worldCollapseSections[2].id = 42;
    worldCollapseSections[3].nb[2] = 4;
    worldCollapseSections[3].nb[3] = 16;
    worldCollapseSections[3].nb[4] = 24;
    worldCollapseSections[3].id = 4;
    worldCollapseSections[4].nb[5] = 3;
    worldCollapseSections[4].nb[4] = 16;
    worldCollapseSections[4].id = 4;
    worldCollapseSections[5].nb[0] = 17;
    worldCollapseSections[5].nb[5] = 25;
    worldCollapseSections[5].id = 7;
    worldCollapseSections[6].nb[0] = 19;
    worldCollapseSections[6].nb[1] = 18;
    worldCollapseSections[6].id = 18;
    worldCollapseSections[7].id = 18;
    worldCollapseSections[8].id = 18;
    worldCollapseSections[9].nb[0] = 21;
    worldCollapseSections[9].nb[1] = 19;
    worldCollapseSections[9].nb[2] = 19;
    worldCollapseSections[9].nb[5] = 10;
    worldCollapseSections[9].id = 36;
    worldCollapseSections[10].nb[0] = 11;
    worldCollapseSections[10].nb[1] = 21;
    worldCollapseSections[10].nb[2] = 9;
    worldCollapseSections[10].nb[5] = 0;
    worldCollapseSections[10].id = 36;
    worldCollapseSections[11].nb[0] = 2;
    worldCollapseSections[11].nb[1] = 12;
    worldCollapseSections[11].nb[2] = 21;
    worldCollapseSections[11].nb[3] = 10;
    worldCollapseSections[11].nb[4] = 0;
    worldCollapseSections[11].nb[5] = 1;
    worldCollapseSections[11].id = 35;
    worldCollapseSections[12].nb[1] = 22;
    worldCollapseSections[12].nb[2] = 22;
    worldCollapseSections[12].nb[3] = 21;
    worldCollapseSections[12].nb[4] = 11;
    worldCollapseSections[12].nb[5] = 2;
    worldCollapseSections[12].id = 35;
    worldCollapseSections[13].nb[2] = 15;
    worldCollapseSections[13].nb[3] = 22;
    worldCollapseSections[13].id = 30;
    worldCollapseSections[14].id = 30;
    worldCollapseSections[15].nb[2] = 24;
    worldCollapseSections[15].nb[3] = 28;
    worldCollapseSections[15].nb[4] = 22;
    worldCollapseSections[15].nb[5] = 13;
    worldCollapseSections[15].id = 16;
    worldCollapseSections[16].nb[0] = 3;
    worldCollapseSections[16].nb[1] = 4;
    worldCollapseSections[16].nb[3] = 17;
    worldCollapseSections[16].nb[4] = 30;
    worldCollapseSections[16].nb[5] = 24;
    worldCollapseSections[16].id = 12;
    worldCollapseSections[17].nb[0] = 16;
    worldCollapseSections[17].nb[3] = 5;
    worldCollapseSections[17].nb[4] = 25;
    worldCollapseSections[17].nb[5] = 30;
    worldCollapseSections[17].id = 56;
    worldCollapseSections[18].nb[0] = 29;
    worldCollapseSections[18].nb[1] = 25;
    worldCollapseSections[18].nb[4] = 6;
    worldCollapseSections[18].nb[5] = 19;
    worldCollapseSections[18].id = 11;
    worldCollapseSections[19].nb[0] = 26;
    worldCollapseSections[19].nb[1] = 29;
    worldCollapseSections[19].nb[2] = 18;
    worldCollapseSections[19].nb[3] = 6;
    worldCollapseSections[19].nb[4] = 9;
    worldCollapseSections[19].nb[5] = 9;
    worldCollapseSections[19].id = 25;
    worldCollapseSections[20].id = 25;
    worldCollapseSections[21].nb[0] = 12;
    worldCollapseSections[21].nb[1] = 26;
    worldCollapseSections[21].nb[2] = 26;
    worldCollapseSections[21].nb[3] = 9;
    worldCollapseSections[21].nb[4] = 10;
    worldCollapseSections[21].nb[5] = 11;
    worldCollapseSections[21].id = 25;
    worldCollapseSections[22].nb[0] = 13;
    worldCollapseSections[22].nb[1] = 15;
    worldCollapseSections[22].nb[2] = 28;
    worldCollapseSections[22].nb[3] = 26;
    worldCollapseSections[22].nb[4] = 21;
    worldCollapseSections[22].nb[5] = 21;
    worldCollapseSections[22].id = 27;
    worldCollapseSections[23].id = 27;
    worldCollapseSections[24].nb[1] = 3;
    worldCollapseSections[24].nb[2] = 16;
    worldCollapseSections[24].nb[3] = 30;
    worldCollapseSections[24].nb[4] = 28;
    worldCollapseSections[24].nb[5] = 15;
    worldCollapseSections[24].id = 60;
    worldCollapseSections[25].nb[0] = 30;
    worldCollapseSections[25].nb[1] = 17;
    worldCollapseSections[25].nb[2] = 5;
    worldCollapseSections[25].nb[4] = 18;
    worldCollapseSections[25].nb[5] = 29;
    worldCollapseSections[25].id = 55;
    worldCollapseSections[26].nb[0] = 22;
    worldCollapseSections[26].nb[1] = 28;
    worldCollapseSections[26].nb[2] = 29;
    worldCollapseSections[26].nb[3] = 19;
    worldCollapseSections[26].nb[4] = 21;
    worldCollapseSections[26].nb[5] = 21;
    worldCollapseSections[26].id = 62;
    worldCollapseSections[27].id = 62;
    worldCollapseSections[28].nb[0] = 15;
    worldCollapseSections[28].nb[1] = 24;
    worldCollapseSections[28].nb[2] = 30;
    worldCollapseSections[28].nb[3] = 29;
    worldCollapseSections[28].nb[4] = 26;
    worldCollapseSections[28].nb[5] = 22;
    worldCollapseSections[28].id = 62;
    worldCollapseSections[29].nb[0] = 28;
    worldCollapseSections[29].nb[1] = 30;
    worldCollapseSections[29].nb[2] = 25;
    worldCollapseSections[29].nb[3] = 18;
    worldCollapseSections[29].nb[4] = 19;
    worldCollapseSections[29].nb[5] = 26;
    worldCollapseSections[29].id = 63;
    worldCollapseSections[30].nb[0] = 24;
    worldCollapseSections[30].nb[1] = 16;
    worldCollapseSections[30].nb[2] = 17;
    worldCollapseSections[30].nb[3] = 25;
    worldCollapseSections[30].nb[4] = 29;
    worldCollapseSections[30].nb[5] = 28;
    worldCollapseSections[30].id = 63;
}

extern void wheelSwayUpdate(struct TM2CarWheel *e, s16 amp);
extern void wheelEffectEmit(struct TM2CarWheel *e, s32 t);
extern s32 sub_8013FC44(s32);
extern int ratan2(int, int);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(s32 *);
extern void soundLocateForView(s32 *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void mathBoundAngle(s32 *);
extern u8 worldSectionGroundClass[];

/* @800F15D0: per-frame wheel suspension update for one car side (human or AI record):
 * runs the four wheel sway animators, derives body pitch/roll from the wheel offsets,
 * emits the wheel effects, and on landing plays the rumble by drop height and starts
 * the landing spin-correction. */
void __attribute__((section(".text_800F15D0"))) carSuspensionUpdate(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarWheels *c;
    struct TM2CarStatsBlock *st;
    struct TM2CarMotion *m;
    struct TM2CarAim *out;
    struct TM2CarState *state;
    s32 unused;
    s32 w0, w1, w2, w3;
    s32 h, a;
    s32 tmp;

    if (human) {
        c = &HREC(car)->wheels;
        out = &HREC(car)->aim;
        st = &HREC(car)->stats;
        m = &HREC(car)->motion;
        state = &HREC(car)->state;
        unused = HREC(car)->player;
        unused = HREC(car)->type;
    } else {
        c = &AI(car)->wheels;
        out = &AI(car)->aim;
        st = &AI(car)->stats;
        m = &AI(car)->motion;
        state = &AI(car)->state;
        unused = AI(car)->index;
        unused = AI(car)->type;
    }
    if (human) {
        wheelSwayUpdate(&c->wheel[0], st->pitch_travel);
        wheelSwayUpdate(&c->wheel[1], st->pitch_travel);
        wheelSwayUpdate(&c->wheel[2], st->pitch_travel);
        wheelSwayUpdate(&c->wheel[3], st->pitch_travel);
    }
    w0 = c->wheel[0].sway;
    w1 = c->wheel[1].sway;
    w2 = c->wheel[2].sway;
    w3 = c->wheel[3].sway;
    h = ((w2 - w0) + (w3 - w1)) / 2;
    out->wheel_pitch = ratan2(h, sub_8013FC44(st->wheelbase * st->wheelbase - h * h));
    h = ((w1 - w0) + (w3 - w2)) / 2;
    out->wheel_roll = ratan2(h, sub_8013FC44(st->track_width * st->track_width - h * h));
    if (!state->falling && !c->wheel[0].f09 && !c->wheel[1].f09 && !c->wheel[2].f09 && !c->wheel[3].f09) {
        wheelEffectEmit(&c->wheel[0], st->pitch_travel);
        wheelEffectEmit(&c->wheel[1], st->pitch_travel);
        wheelEffectEmit(&c->wheel[2], st->pitch_travel);
        wheelEffectEmit(&c->wheel[3], st->pitch_travel);
    }
    if (state->f1B && !state->all_in_air) {
        state->f1B = 0;
        a = c->wheel[0].land_z - c->wheel[0].pos[2];
        a = (a < c->wheel[1].land_z - c->wheel[1].pos[2]) ? c->wheel[1].land_z - c->wheel[1].pos[2] : a;
        a = (a < c->wheel[2].land_z - c->wheel[2].pos[2]) ? c->wheel[2].land_z - c->wheel[2].pos[2] : a;
        a = (a < c->wheel[3].land_z - c->wheel[3].pos[2]) ? c->wheel[3].land_z - c->wheel[3].pos[2] : a;
        c->wheel[0].land_z = c->wheel[0].pos[2];
        c->wheel[1].land_z = c->wheel[1].pos[2];
        c->wheel[2].land_z = c->wheel[2].pos[2];
        c->wheel[3].land_z = c->wheel[3].pos[2];
        if (a > 0) {
            unused = m->vel[0];
            unused = m->vel[1];
            unused = m->vel[2];
            /* human motion.pos, also for AI records (retail) */
            if (!viewGetSplitMode())
                soundLocateForView0(HREC(car)->motion.pos);
            else
                soundLocateForView(HREC(car)->motion.pos, -1);
            if (worldSectionGroundClass[c->wheel[0].kind] != 2 && worldSectionGroundClass[c->wheel[0].kind] != 3 && !state->dead) {
                if ((unsigned)(a - 450) < 550) {
                    soundPlay(5, PadChkMtap(), soundGetListenerPan(), a * 67 / 549 + 5);
                } else if (a >= 1000) {
                    soundPlay(5, PadChkMtap(), soundGetListenerPan(), 99);
                }
                if ((unsigned)(a - 90) < 360) {
                    soundPlay(3, PadChkMtap(), soundGetListenerPan(), a * 67 / 359 + 43);
                } else if (a >= 450) {
                    soundPlay(3, PadChkMtap(), soundGetListenerPan(), 99);
                }
            }
        }
        c->tilt = 0;
        if (state->hit_from_side) {
            state->f12 = 1;
            m->f02 = m->body_rot[2];
            tmp = m->body_rot[2] - m->rot[2];
            mathBoundAngle(&tmp);
            if (tmp < 0) {
                m->spin1 = 1;
                m->f04 = m->body_rot[2] + 1024;
            } else {
                m->spin1 = -1;
                m->f04 = m->body_rot[2] - 1024;
            }
            st->f20 = 0;
            state->hit_from_side = 0;
        }
    }
}

/* 24-byte AI route nodes @801B1DE8 */
typedef struct {
    s32 x, y;       /* +0x00 */
    s16 z;          /* +0x08 */
    s16 links[6];   /* +0x0A */
    s8 count;       /* +0x16 */
    s8 route;       /* +0x17 */
} G10RouteNode;
extern G10RouteNode aiRouteNodes[];
extern u8 aiRouteTransitionTables[][35][35];  /* [0] next node, [1] next route, [2] link target, by [route][target route] */
extern u8 worldSectionHasDrop[];
extern void worldWarpStart(s32 idx, void *car, u8 human);
extern s32 GsGetWorkBase_0(void);
extern void mathMulTransVec(void *, s32 *, s32 *);

/* @800F7D54: AI route target: from the AI's current route and the route of the car it
 * follows, choose the next route node (following link nodes / shortcut transitions) and
 * write the aim point to out[3]; returns the node's route group when that route allows
 * leaving it (worldSectionHasDrop), else -1. */
s32 __attribute__((section(".text_800F7D54"))) aiFollowRouteTarget(struct TM2AIRecord *ai, s32 *out)
{
    u8 th;
    void *tgt;              /* human (th) or AI record */
    struct TM2CarMotion *m;
    struct TM2CarStatsBlock *st;
    s32 troute, route, next, best, i, n, half, a;
    s16 b;
    s32 follow;
    u8 isLink;
    s32 v[3];
    s32 r[3];

    tgt = aiGetTargetCar(ai->index, &th);
    if (th) {
        troute = HREC(tgt)->state.x2C.route;
        m = &HREC(tgt)->motion;
        st = &HREC(tgt)->stats;
    } else {
        troute = DRV(tgt)->route;
        m = &AI(tgt)->motion;
        st = &AI(tgt)->stats;
    }
    route = aiRouteNodes[DRV(ai)->node].route;
    if (DRV(ai)->link_pending && DRV(ai)->link_node > 0) {
        next = DRV(ai)->link_node;
        if (next != (s8)aiRouteTransitionTables[2][route][troute]) {
            DRV(ai)->link_pending = 0;
            next = (s8)aiRouteTransitionTables[0][route][troute];
            isLink = 0;
            follow = (s8)aiRouteTransitionTables[1][route][troute];
        } else {
            isLink = 1;
            follow = (s8)aiRouteTransitionTables[1][route][aiRouteNodes[next].route];
        }
    } else {
        next = (s8)aiRouteTransitionTables[0][route][troute];
        isLink = 0;
        follow = (s8)aiRouteTransitionTables[1][route][troute];
    }
    if ((isLink && route == aiRouteNodes[next].route) || troute == route) {
        ai->f040 = 0;
    } else if (next < 0) {
        ai->f040 = 0;
        if (ai->type == 240 && troute >= 5 && route < 4 && TM2_HK_WARPS) {
            if (DRV(ai)->node == 66 || DRV(ai)->node == 68
                || DRV(ai)->node == 70 || DRV(ai)->node == 72) {
                worldWarpStart(1, ai, 0);
                ai->f040 = 0;
                route = 24;
            }
        }
    } else {
        ai->f040 = 1;
        if (ai->type == 240 && troute < 4 && route >= 4 && DRV(ai)->node == 0 && TM2_HK_WARPS) {
            worldWarpStart(2, ai, 0);
            ai->f040 = 0;
            route = 3;
        }
    }
#ifdef TM2_PORT_BALANCE
    /* request 101 (bit 29, BALANCE.md 22): a table entry past the level's nodes, or the target's section on a node
       the car is not in: the route graph's way (-1 chase the target, else the node to head for) */
    i = portBalanceRouteNext(DRV(ai)->node, DRV(ai)->route, next, route, troute, m->pos);
    if (i != -2) {
        isLink = 0;
        ai->f040 = i >= 0;
        if (i >= 0) {
            next = i;
            follow = route;
        }
    }
#endif
    if (ai->f040) {
        if (ai->f001 < 2) {
            if (next == DRV(ai)->node) {
                best = aiRouteNodes[next].links[0];
                for (i = 0; i < aiRouteNodes[next].count; i++) {
                    n = aiRouteNodes[next].links[i];
                    if (aiRouteNodes[n].route == follow) {
                        best = n;
                        route = follow;
                        if (GsGetWorkBase_0() & 1)
                            break;
                    }
                }
                next = best;
            }
            out[0] = aiRouteNodes[next].x;
            out[1] = aiRouteNodes[next].y;
            out[2] = m->pos[2];
            if (!worldSectionHasDrop[route])
                route = -1;
        } else {
            out[0] = m->pos[0];
            out[1] = m->pos[1];
            out[2] = m->pos[2];
            route = -1;
        }
    } else {
        v[0] = 0;
        half = ai->target_dist / 2;
        v[2] = 0;
        v[1] = half;
        if (isLink) {
            out[0] = aiRouteNodes[next].x;
            out[1] = aiRouteNodes[next].y;
            out[2] = aiRouteNodes[next].z;
        } else {
            if (ai->f001 < 2 && ai->target_dist < ai->range_far) {
                a = m->speed;
                b = m->speed;
                v[1] = half + ((a < 0) ? -a : a) * 1600 / ((b < st->speed_cap) ? st->speed_cap : m->speed);
            }
            mathMulTransVec(&m->rot_m, v, r);
            out[0] = m->pos[0] + r[0];
            out[1] = m->pos[1] + r[1];
            out[2] = m->pos[2] + r[2];
        }
        route = -1;
    }
    return route;
}

typedef void (*G10Method)(u8 *, ...);
typedef struct { s16 delta; s16 index; G10Method fn; } G10VEntry;  /* vtable slot */
extern u8 *soundManagerObject;   /* sound object */
extern u8 *musicManagerObject;   /* music object */
/* vtable pointers of the two (C++) objects */
typedef struct { u8 pad_00[200]; G10VEntry *vptr; } G10SoundManager;   /* vptr at +200 */
typedef struct { u8 pad_00[8]; G10VEntry *vptr; } G10MusicManager;     /* vptr at +8 */
#define SOUND_VPTR (((G10SoundManager *)soundManagerObject)->vptr)
#define MUSIC_VPTR (((G10MusicManager *)musicManagerObject)->vptr)
extern s32 audioCdVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin

extern char strBattlePaused[];    /* "BATTLE PAUSED" */
extern char strBattleAborted[];    /* "BATTLE ABORTED" */
extern char *pauseMenuLabels[];   /* { "RESUME", "QUIT", ... } */
extern char textFormatBuffer[];    /* password text */
extern s32 sessionGetMode(struct TM2Settings *);
extern s32 sessionGetLevel(struct TM2Settings *);
#include "semantic/input.h"
extern void padFlush(void);
extern void SsSetSerialVol(char s_num, short voll, short volr);
extern void screenLostALife(void);
#include "semantic/hud.h"
extern void passwordBuildString(void);
extern void taskMgrRunFrame(u8 *);
extern s32 gameGetLevel(void);
extern s32 audioGetMusicVolume(void);
extern int DrawSync(int);
extern int VSync(int);
extern void rtScheduleEvent(s32, s32);

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] and apply it. */
static __inline__ void G10SetSerialVolume(s32 v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
    SsSetSerialVol(0, (s16)audioCdVolume, (s16)audioCdVolume);
}

/* Play UI sound `id` on the sound object (vtable slot 1, then flush via slot 2). */
static __inline__ void G10PlaySound(s32 id)
{
    G10VEntry *e;

    e = SOUND_VPTR + 1;
    e->fn(soundManagerObject + e->delta, id, 0, 0, 99, -1, 0);
    taskMgrRunFrame(soundManagerObject);
    e = SOUND_VPTR + 2;
    e->fn(soundManagerObject + e->delta);
}

/* Draw the pause title and the RESUME / QUIT items (selected item in colour 1). */
static __inline__ void G10DrawPause(s32 sel, s32 *col, char **labels, char *title)
{
    s32 *lo;
    s32 *p;

    fontPrintCenteredXY(6, 159, 60, title);
    lo = &col[0];
    fontPrintCenteredXY(*(sel == 0 ? &col[1] : lo), 159, 140, labels[0]);
    /* p = lo before the test (not a ?:) keeps the li 1 next to its branch, so
     * loop.c judges the three `== 1` constants in pauseMenu's loop not
     * worth hoisting into a register, as in retail */
    p = lo;
    if (sel == 1)
        p = &col[1];
    fontPrintCenteredXY(*p, 159, 160, labels[1]);
}

/* Call vtable slot `e` of the sound object with sound `id`. */
static __inline__ void G10SoundSlot(G10VEntry *e, s32 id)
{
    e->fn(soundManagerObject + e->delta, id, 0, 0, 99, -1, 0);
}

/* @80129528: in-race battle pause menu: on START (pad 0, or pad 1 in two-player) pause the
 * music, show "BATTLE PAUSED" with RESUME / QUIT (and the password when applicable); up/down
 * moves the cursor, START confirms: resume restores the music/volume, quit shows
 * "BATTLE ABORTED" and ends the battle. */
void __attribute__((section(".text_80129528"))) pauseMenu(void)
{
    s32 sel;
    s32 prev;
    s16 pad;
    s32 ok;
    s32 quitable;
    G10VEntry *e;
    char *labels[3];
    s32 col[2];
    char *paused;
    char *aborted;

#ifdef TM2_PORT_ROLLBACK
    /* request 48 (ONLINE_ROLLBACK.md 6, 8.1 bit 9, PAUSE_PORT): online the menu never opens (no pad read, no
       blocking loop); the port's own menu quits through portRollbackQuit (rtMainLoop's top, every machine) */
    if (portRollbackPauseOff())
        return;
#endif
    sel = 0;
    __builtin_memcpy(labels, pauseMenuLabels, 12);
    paused = strBattlePaused;
    aborted = strBattleAborted;
    col[0] = 0;
    col[1] = 1;
    if (!sessionGetMode(&sessionSettings))
        return;
    if (!padCheckAction(8, 0)) {
        ok = 0;
        if (viewGetSplitMode())
            ok = padCheckAction(8, 1) != 0;
        pad = 1;
        if (!ok)
            return;
    } else
        pad = 0;
    e = MUSIC_VPTR + 3;
    e->fn(musicManagerObject + e->delta, 2);
    G10SetSerialVolume(0);
    screenLostALife();
    G10DrawPause(sel, col, labels, paused);
    quitable = 0;
    if (sessionGetMode(&sessionSettings) == 1 && sessionGetLevel(&sessionSettings) >= 2
        && sessionGetHumanCar(&sessionSettings, 0) != 12)
        quitable = TM2_PAUSE_PASSWORD_CAR(sessionGetHumanCar(&sessionSettings, 0));
    if (quitable) {
        passwordBuildString();
        fontSetColor(5, 255, 161, 0);
        fontPrintCenteredXY(5, 159, 222, textFormatBuffer);
        fontResetColor(5);
    }
    while (padAnyPressed(pad))
        padUpdate();
    padSetRepeatMode(0);
    while (1) {
        VSync(0);
        prev = sel;
        padUpdate();
        if (padCheckAction(0, pad)) {
            if (sel == 0)
                sel = 1;
            else
                sel--;
            G10PlaySound(28);
        } else if (padCheckAction(1, pad)) {
            if (sel == 1)
                sel = 0;
            else
                sel++;
            G10PlaySound(28);
        } else if (padCheckAction(8, pad)) {
            /* G10PlaySound(29) with the slot-1 call through G10SoundSlot: the
             * inlined parameter puts a1 = 29 ahead of the sound-object loads,
             * so reorg moves it into the branch delay slot, as in retail. */
            G10SoundSlot(SOUND_VPTR + 1, 29);
            taskMgrRunFrame(soundManagerObject);
            e = SOUND_VPTR + 2;
            e->fn(soundManagerObject + e->delta);
            switch (sel) {
            case 0:
                prev = gameGetLevel();
                e = MUSIC_VPTR + 4;
                e->fn(musicManagerObject + e->delta, prev + 1);
                G10SetSerialVolume(audioGetMusicVolume());
                screenDisplayToDisplay(0, 1);
                DrawSync(0);
                padSetRepeatMode(1);
                while (padAnyPressed(pad))
                    padUpdate();
                return;
            case 1:
                VSync(0);
                screenDisplayToDisplay(0, 1);
                fontPrintCenteredXY(6, 159, 115, aborted);
                padWaitConfirmOrTimeout(60, 60);
                rtScheduleEvent(2, 0);
                DrawSync(0);
                while (padAnyPressed(pad))
                    padFlush();
                return;
            }
        }
        if (prev != sel) {
            VSync(0);
            G10DrawPause(sel, col, labels, paused);
        }
    }
}

extern struct TM2ShellScene shellScene;             /* the shell scene */
/* soundManagerObject (the sound object) is addressed absolutely here but through $gp in
 * pauseMenu; this alias (the resolver also maps aXXXXXXXX names) carries the
 * absolute addressing. */
extern s32 a80180BC8;
#pragma tm2_absolute a80180BC8
extern char strShellHistoryTim[];           /* "SHELLDB2\HISTORY.TIM" */
extern char strShellHistoryScene[];           /* scene name */
/* title screen */
extern void *screenOptions(void);
extern void *screenHistoryMovie(void);
extern void sceneInit(struct TM2ShellScene *scene, void *back, void *cancel, void *idle, s32);
extern u16 sceneSetCamera(struct TM2ShellScene *scene, s32 x, s32 y, s32 z, u16, u16, u16);
extern void *screenImageRecordInit(void *record, const char *name, s32 buffer);
extern void *sub_80137F60(void *item, u16 id, u16 count, void *next);
extern void sceneObjSetPosition(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetUseCamera(void *object, u16 v);
extern void sub_80123E5C(void *object, u16 v);
extern void sceneObjSetScale(void *object, s32 v);
extern void *menuSelectNextInit(void *object, void *owner);
extern void *menuSelectPrevInit(void *object, void *owner);
extern void *sub_80123828(void *object, void *target, s32, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);   /* TM2MoveBy */
extern void *sub_80123B64(void *object, void *target, s32, s32, void *);     /* TM2FourWay */
extern void *menuPlaySoundInit(void *object, s32 actor, s32, s32, s32);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);

/* @8013629C: shell "history" screen (SHELLDB2\HISTORY.TIM): two menu items (9001 -> 801368D8,
 * 9002 -> 80136A3C) with up/down cursor moves, turn/flip animations and the select/confirm
 * sounds, then runs the scene. */
void *__attribute__((section(".text_8013629C"))) screenHistory(void)
{
    u8 record[48];
    u8 item_a[56], item_b[56];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set0[24], set1[24], set2[24];
    u8 turn_a[32], turn_b[32];
    u8 up_a[32], down_a[32], up_b[32], down_b[32];
    u8 snd_move[32], snd_ok[32];

    sceneInit(&shellScene, screenTitle, screenOptions, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellHistoryTim, 0x80048270);
    sub_80137F60(item_a, 9001, 1, screenHistoryMovie);
    sceneObjSetPosition(item_a, -1784, 0, 360);
    sceneObjSetUseCamera(item_a, 0);
    sub_80123E5C(item_a, 0);
    sceneObjSetScale(item_a, 1);
    sub_80137F60(item_b, 9002, 1, screenCredits);
    sceneObjSetPosition(item_b, -584, 0, 176);
    sceneObjSetUseCamera(item_b, 0);
    sub_80123E5C(item_b, 0);
    sceneObjSetScale(item_b, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, 0, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123A20(turn_a, item_a, 40, 0, 0, 15);
    sub_80123A20(turn_b, item_b, -40, 0, 0, 15);
    sub_80123B64(up_a, item_a, 0, 1, 0);
    sub_80123B64(down_a, item_a, 0, -1, 0);
    sub_80123B64(up_b, item_b, 0, 1, 0);
    sub_80123B64(down_b, item_b, 0, -1, 0);
    menuPlaySoundInit(snd_move, a80180BC8, 28, 0, 0);
    menuPlaySoundInit(snd_ok, a80180BC8, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, item_a);
    sceneAddItem(&shellScene, item_b);
    sceneAddAction(&shellScene, 7, 0, 2, snd_move);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, snd_move);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 3, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 3, 0, 1, wait);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 14, 0, 1, wait);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 15, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, turn_a);
    sceneAddAction(&shellScene, 0, 0, 0, turn_b);
    sceneAddAction(&shellScene, 2, turn_a, 2, snd_move);
    sceneAddAction(&shellScene, 2, turn_a, 2, up_a);
    sceneAddAction(&shellScene, 2, turn_a, 0, wait);
    sceneAddAction(&shellScene, 7, item_a, 2, down_a);
    sceneAddAction(&shellScene, 7, item_a, 2, up_b);
    sceneAddAction(&shellScene, 8, item_a, 2, down_a);
    sceneAddAction(&shellScene, 8, item_a, 2, up_b);
    sceneAddAction(&shellScene, 7, item_b, 2, down_b);
    sceneAddAction(&shellScene, 7, item_b, 2, up_a);
    sceneAddAction(&shellScene, 8, item_b, 2, down_b);
    sceneAddAction(&shellScene, 8, item_b, 2, up_a);
    sceneAddAction(&shellScene, 2, wait, 2, set2);
    sceneLoadTmsDmd(&shellScene, strShellHistoryScene);
    screenImageRecordLoad(record);
    return (void *)sceneRun(&shellScene);   /* the next screen handler */
}

