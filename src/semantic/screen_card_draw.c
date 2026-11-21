/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/port_far_distance.h"
#include "semantic/port_rollback.h"   /* request 48: HOLD (streaksDraw's ticks) */
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-3 (CAR_GROUPS.md 12) */
#endif
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: the burn / lava loop by sound side */
#endif
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 36 */
#endif
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"   /* request 46 item 4: portVisual bit 4, a bullet's first streak */
/* streaksDraw: the streak's tail (its previous point), the port's moved copy in a bullet's first moved frame */
#define STREAK_TAIL tail
#else
#define STREAK_TAIL e->p1
#endif

#pragma tm2_rdata .tm2_800CF350

extern s32 gameMode;
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 soundPlayVariant(s32, s32, s32, s32, s32);
extern s32 soundStopInstance(s32, s32);
extern s32 soundStartPlayerLoop(s32, s32, s32, s32, s32);
extern u8 soundGetLoop32P1(void);
extern u8 soundGetLoop32P2(void);
extern s32 soundStopPlayerLoop(s32, s32);
extern s32 carSurfaceSoundVolume(s32, s32, s32, u8 c, u8 d, u8 e);
extern u8 soundIsActive(s32);
extern s32 soundStop(s32);

/* Per-frame car sound update for one car record: picks the player/AI layout
 * (a2 != 0: player), starts/stops pad comms, issues the engine/skid/horn
 * sound commands and a surface sound chosen by the car's surface id. */
void __attribute__((section(".text_8011DB74"))) carSoundUpdate(s32 a0, s16 *a1, u8 a2)
{
    s32 surf;
    u8 *st;
    s16 idt;
    s32 id;
    s16 *motion;
    u8 *eng;
    char *tune;
    s16 *horn;
    s32 rpm;
    s32 vol, pan;
    s32 snd;
    s32 pitch;

    if (a2) {
        surf = *(s32 *)((char *)a1 + 144);
        st = (u8 *)a1 + 156;
        rpm = *(s32 *)((char *)a1 + 140);
        idt = a1[0];
        motion = (s16 *)((char *)a1 + 1756);
        eng = (u8 *)a1 + 208;
        tune = (char *)a1 + 332;
        horn = (s16 *)((char *)a1 + 564);
    } else {
        st = (u8 *)a1 + 108;
        tune = (char *)a1 + 1276;
        surf = *(s32 *)((char *)a1 + 48);
        idt = a1[2];
        motion = (s16 *)((char *)a1 + 1820);
        horn = (s16 *)((char *)a1 + 216);
        eng = (u8 *)a1 + 68;
        rpm = 0;
    }
    id = idt;
    if (viewGetSplitMode() == 0) {
        soundLocateForView0(motion + 16);
    } else {
        soundLocateForView(motion + 16, -1);
    }
    vol = 0;
    if (gameMode == 4) {
        pan = 0;
    } else {
        vol = PadChkMtap();
        pan = soundGetListenerPan();
    }
    if (st[2] && !st[34] && !st[33] && !st[11]) {
        soundPlay(1, vol, pan, 99);
    }
    if (st[19]) {
        soundPlay(7, vol, pan, 99);
        st[19] = 0;
    }
    if (a2) {
        if (st[14]) {
            soundPlayVariant(21, vol, pan, 99, id);
        } else {
            soundStopInstance(21, id);
        }
#if defined(TM2_PORT_CAR_SOUND) || defined(TM2_PORT_HUMANS)
        /* request 33-4 (CAR_GROUPS.md 16): the burn / lava loop of players 2.. per car (human slot +0x004);
         * retail's calls for players 0 and 1; H3: by sound side (local slot 0/1) */
        if ((st[26] || st[34]) && !st[14]) {
            portHumanLoopPlay(32, vol, pan, 99, a1[0], a1[2]);
        } else {
            portHumanLoopStop(32, id, a1[2]);
        }
#else
        if ((st[26] || st[34]) && !st[14]) {
            soundStartPlayerLoop(32, vol, pan, 99, a1[0]);
        } else {
            if (id == 0 && soundGetLoop32P1()) {
                soundStopPlayerLoop(32, 0);
            }
            if (id == 1 && soundGetLoop32P2()) {
                soundStopPlayerLoop(32, 1);
            }
        }
#endif
    }
    snd = -1;
    pitch = carSurfaceSoundVolume(__builtin_abs(motion[5]) * 100 / *(s32 *)(tune + 184), rpm, motion[5], eng[13], eng[14], eng[18]);
    switch (surf) {
    case 0: default: break;
    case 0x10: snd = 0x3C; break;
    case 0x20: snd = 0x24; break;
    case 0x30: snd = 0x26; break;
    case 0x40: snd = 0x28; break;
    case 0x50: snd = 0x2A; break;
    case 0x60: snd = 0x2C; break;
    case 0x70: snd = 0x2E; break;
    case 0x80: snd = 0x30; break;
    case 0x90: snd = 0x32; break;
    case 0xA0: snd = 0x34; break;
    case 0xB0: snd = 0x36; break;
    case 0xC0: snd = 0x38; break;
    case 0xD0: snd = 0x3A; break;
    case 0xE0: snd = 0x41; break;
    }
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): retail has no engine for Dark Tooth; a player's plays the port's id,
       Minion's 0x41 by default (portDtPlayer.engine_sound 0), none when it is < 0 */
    if (a2 && surf == 0xF0)
        snd = TM2_DT_PLAYER_ENGINE();
#endif
#ifdef TM2_PORT_CAR_SOUND
    /* request 33-3 (CAR_GROUPS.md 12): the car slot's own engine instance (human slot +0x004, AI +0x006);
     * portCarOptions bit 4 clear: the retail calls below, made by portCarEngineUpdate */
    portCarEngineUpdate(a2 ? a1[2] : a1[3], snd, !st[0], vol, pan, pitch);
#else
    if (snd != -1) {
        if (!st[0]) {
            soundPlay(snd, vol, pan, pitch);
        } else if (soundIsActive(snd)) {
            soundStop(snd);
        }
    }
#endif
    if (horn[7] && !st[0]) {
        soundPlay(3, vol, pan, horn[7]);
    }
}


extern void mathMulTransVec(void *, s32 *, s32 *);
extern s32 humanCarCount(void);
extern s32 aiCarCount(void);

#define G8_DIST(v, pos) (__builtin_abs((v)[0] - (pos)[0]) + \
    __builtin_abs((v)[1] - (pos)[1]) + __builtin_abs((v)[2] - (pos)[2]))

/* Looks for another car (player list first, then AI list) whose position is
 * within 160 units of two points projected ahead of this car (120 and 60
 * units along its heading) and flags it as hit (bump timers etc.); the
 * slam record (weapons.slam) gets the impact strength. Returns 1 when
 * the other car was not slower than this one. car is a TM2HumanRecord when
 * human is set, else a TM2AIRecord. */
u8 __attribute__((section(".text_800F5498"))) weaponSlamFindTarget(void *car, u8 human)
{
    s32 best;
    u8 found;
    u8 ret;
    struct TM2CarSlam *rec;
    s32 id;
    struct TM2CarStatsBlock *tune;
    struct TM2CarMotion *motion;
    s32 i;
    s32 dist;
    s32 n;
    s32 nai;
    struct TM2HumanRecord *h;
    struct TM2AIRecord *ai;
    s32 d;
    s32 in[3];
    s32 p1[3];
    s32 p2[3];

    found = 0;
    best = 0;
    ret = 0;
    if (human) {
        rec = ((struct TM2HumanRecord *)car)->weapons.slam;
        id = ((struct TM2HumanRecord *)car)->player;
        tune = &((struct TM2HumanRecord *)car)->stats;
        motion = &((struct TM2HumanRecord *)car)->motion;
    } else {
        rec = ((struct TM2AIRecord *)car)->weapons.slam;
        id = ((struct TM2AIRecord *)car)->index;
        tune = &((struct TM2AIRecord *)car)->stats;
        motion = &((struct TM2AIRecord *)car)->motion;
    }
    in[0] = 0;
    in[1] = tune->axle_front + 120;
    in[2] = 0;
    mathMulTransVec(&motion->rot_m, in, p1);
    in[1] = tune->axle_front + 60;
    mathMulTransVec(&motion->rot_m, in, p2);
    p1[0] += motion->pos[0];
    p1[1] += motion->pos[1];
    p1[2] += motion->pos[2];
    p2[0] += motion->pos[0];
    p2[1] += motion->pos[1];
    p2[2] += motion->pos[2];
    i = 0;
    dist = 0x7FFF;
    n = humanCarCount();
    nai = aiCarCount();
    for (; i < n; i++) {
        if (i != id || !human) {
            h = humanCarGet(i);
            d = G8_DIST(p1, h->motion.pos);
            if (d < dist) {
                best = i;
                dist = d;
            }
            d = G8_DIST(p2, h->motion.pos);
            if (d < dist) {
                best = i;
                dist = d;
            }
        }
    }
    if (dist < 160) {
        found = 1;
        h = humanCarGet(best);
        h->state.frozen = 1;
        h->stats.freeze_timer = 60;
        h->motion.speed = 0;
        h->state.f1D = 1;
        h->state.crush_peer = 1;
        if (h->stats.mass < tune->mass) {
            rec->spin_pending = 1;
            if (dist < 64) {
                rec->hold = 3;
            } else if (dist < 136) {
                rec->hold = 2;
            } else {
                rec->hold = 1;
            }
        } else {
            ret = 1;
            rec->hold = 0;
        }
#ifdef TM2_PORT_CAR_GROUPS
        rec->spin_target = TM2_CAR_KEY(h->slot, h->type);   /* request 32 */
#else
        rec->spin_target = h->type;
#endif
    }
    if (!found) {
        for (i = 0; i < nai; i++) {
            if (i != id || human) {
                ai = aiCarGet(i);
                d = G8_DIST(p1, ai->motion.pos);
                if (d < dist) {
                    best = i;
                    dist = d;
                }
                d = G8_DIST(p2, ai->motion.pos);
                if (d < dist) {
                    best = i;
                    dist = d;
                }
            }
        }
        if (dist < 160) {
            found = 1;
            ai = aiCarGet(best);
            ai->state.frozen = 1;
            ai->stats.freeze_timer = 60;
            ai->motion.speed = 0;
            ai->state.f1D = 1;
            ai->state.crush_peer = 1;
            if (ai->stats.mass < tune->mass) {
                rec->spin_pending = 1;
                if (dist < 64) {
                    rec->hold = 3;
                } else if (dist < 136) {
                    rec->hold = 2;
                } else {
                    rec->hold = 1;
                }
            } else {
                ret = 1;
                rec->hold = 0;
            }
#ifdef TM2_PORT_CAR_GROUPS
            rec->spin_target = TM2_CAR_KEY(ai->slot, ai->type);   /* request 32 */
#else
            rec->spin_target = ai->type;
#endif
        }
    }
    return ret;
}

extern u8 **modelRelocateStack;
extern void modelRemapTextures(u8 *);

/* Node layouts walked by modelRelocate (byte 0 = type, byte 1 = 0xB1 once relocated). */
typedef struct { u8 type; u8 flag; u8 pad[2]; s32 a; s32 b; s32 c; } G8Node0;
typedef struct { u8 type; u8 flag; u8 pad[18]; u8 cnt; u8 pad2[3]; s32 kid[1]; } G8Node1;
typedef struct { s32 pad[2]; s32 next; } G8Kid2;
typedef struct { u8 type; u8 flag; u8 pad[14]; s32 cnt; s32 kid[1]; } G8Node2;
typedef struct { u8 type; u8 flag; u8 pad[9]; u8 cnt; u8 pad2[16]; s32 extra; s32 kid[1]; } G8Node3;
typedef struct { u8 type; u8 flag; u8 pad[16]; u8 cnt; u8 pad2; s32 kid[1]; } G8Node4;
typedef struct { u8 type; u8 flag; u8 pad[36]; u8 cnt; u8 pad2; s32 kid[1]; } G8Node5;
typedef struct { u8 type; u8 flag; u8 pad[10]; s32 ref; } G8Node7;
typedef struct { u8 type; u8 flag; u8 pad[4]; u8 cnt; u8 pad2[9]; s32 kid[1]; } G8Node9;
typedef struct { u8 type; u8 flag; u8 pad[4]; u8 cnt; u8 pad2; s32 a; s32 b; s32 kid[1]; } G8Node11;
typedef struct { u8 type; u8 flag; u8 pad[4]; u16 cnt; s32 kid[1]; } G8Node12;

#define G8_RELOC_LIST(T)                                            \
    {                                                               \
        s32 i;                                                      \
        T *n = (T *)p;                                              \
        if (n->flag != 0xB1) {                                      \
            for (i = 0; i < n->cnt; i++) {                          \
                n->kid[i] += delta;                                 \
                modelRelocateStack[sp++] = (u8 *)n->kid[i];                 \
            }                                                       \
        }                                                           \
        n->flag = 0xB1;                                             \
    }                                                               \
    break;

/* Relocates a loaded model/scene node tree: walks the node graph with an
 * explicit stack (modelRelocateStack), adding delta to every internal pointer of
 * each node (by node type) and pushing child nodes; byte +1 = 0xB1 marks a
 * node as already relocated. Mesh nodes (type 0) also get their textures
 * re-pointed (modelRemapTextures). */
void __attribute__((section(".text_80120FEC"))) modelRelocate(u8 *root, s32 delta)
{
    s32 sp;
    u8 *p;

    sp = 1;
    modelRelocateStack[0] = root;
    do {
        sp--;
        p = modelRelocateStack[sp];
        if (p) {
            switch (p[0]) {
            case 0:
                if (p[1] != 0xB1) {
                    ((G8Node0 *)p)->a += delta;
                    ((G8Node0 *)p)->b += delta;
                    ((G8Node0 *)p)->c += delta;
                    modelRemapTextures(p);
                }
                p[1] = 0xB1;
                break;
            case 1:
                G8_RELOC_LIST(G8Node1)
            case 2:
                {
                    s32 i;
                    G8Node2 *n = (G8Node2 *)p;
                    if (n->flag != 0xB1) {
                        for (i = 0; i < n->cnt; i++) {
                            n->kid[i] += delta;
                            ((G8Kid2 *)n->kid[i])->next = ((G8Kid2 *)n->kid[i])->next + delta;
                            modelRelocateStack[sp++] = (u8 *)((G8Kid2 *)n->kid[i])->next;
                        }
                    }
                    n->flag = 0xB1;
                }
                break;
            case 3:
                {
                    s32 i;
                    G8Node3 *n = (G8Node3 *)p;
                    if (n->flag != 0xB1) {
                        for (i = 0; i < n->cnt; i++) {
                            n->kid[i] += delta;
                            modelRelocateStack[sp++] = (u8 *)n->kid[i];
                        }
                        if (n->extra) {
                            n->extra += delta;
                        }
                    }
                    n->flag = 0xB1;
                }
                break;
            case 4:
                G8_RELOC_LIST(G8Node4)
            case 5:
                G8_RELOC_LIST(G8Node5)
            case 7:
            case 8:
                if (p[1] != 0xB1) {
                    if (((G8Node7 *)p)->ref) {
                        ((G8Node7 *)p)->ref += delta;
                        p[1] = 0xB1;
                    }
                }
                break;
            case 9:
                G8_RELOC_LIST(G8Node9)
            case 11:
                {
                    s32 i;
                    G8Node11 *n = (G8Node11 *)p;
                    if (n->flag != 0xB1) {
                        n->a += delta;
                        n->b += delta;
                        for (i = 0; i < n->cnt; i++) {
                            n->kid[i] += delta;
                        }
                        modelRelocateStack[sp++] = (u8 *)n->a;
                        modelRelocateStack[sp++] = (u8 *)n->b;
                    }
                    n->flag = 0xB1;
                }
                break;
            case 12:
                G8_RELOC_LIST(G8Node12)
            }
        }
    } while (sp > 0);
}

typedef struct {
    s32 p0[3];
    s32 p1[3];
    s32 pad18[4];
    s32 life;
    s32 pad2C[2];
    u8 type;
    u8 pad35[7];
} G8Streak;

extern s32 fxSheetTimer;
extern G8Streak bulletRecords[];
extern void viewGetCentre(s32, s32 *, s32 *);
extern s32 viewGetProjScaleX(s32);
extern s32 viewGetProjScaleY(s32);
extern s32 *viewGetEyeTrans(s32);
extern s16 *viewGetViewMatrix(s32);
extern void fxSheetDisplay(u32 *);
extern void weaponDropsUpdate(void *, s32);
extern void weaponFlameUpdate(void);
extern void weaponArcsDraw(void *, s32);
extern void mathMulMatVec(s16 *, s32 *, s32 *);
extern void sub_80144CCC(u8 *);
extern void AddPrim(u32 *, void *);

/* Draws the 40 streak/spark effect slots (bulletRecords, 60 bytes each) for
 * one view: projects both end points of each live streak and emits either
 * a flat line-ish prim (type 0, 80 bytes) or a widened gouraud quad
 * (type != 0, 112 bytes) into the ordering table. Also draws the FX sheet,
 * the effects of weaponDropsUpdate/weaponFlameUpdate and the tasers first. */
void __attribute__((section(".text_800D8230"))) streaksDraw(u32 **ctx, s32 view)
{
    u32 *ot;
    s32 cx, cy;
    s32 sx, sy;
    s32 *eye;
    s16 *mat;
    u8 *pl;
    u8 *pq;
    s32 i;
    G8Streak *e;
    s32 in[3];
    s32 a[3];
    s32 b[3];
    s32 za, zb;
    s32 c, lo;
    u8 *next;
    s32 x0, y0, x1, y1;
    s32 dy, dx;  /* (y0 - y1) / 12 + 1 and (x0 - x1) / 12: quad half-width */
#ifdef TM2_PORT_VISUAL
    s32 *tail;
#endif

    ot = ctx[0];
    viewGetCentre(view, &cx, &cy);
    sx = viewGetProjScaleX(view);
    sy = viewGetProjScaleY(view);
    eye = viewGetEyeTrans(view);
    mat = viewGetViewMatrix(view);
    pl = 0;
    pq = 0;
    /* request 48 (ONLINE_ROLLBACK.md 6, HOLD): a held iteration draws only, so the four ticks (the flash countdown,
       the drops' fuses, the flames, the arcs' damage) and their drawing are skipped; the streaks still draw.
       Stage 2 (ONLINE_ROLLBACK.md 11, ONCE_PER_STEP): the drops' and flames' ticks are skipped in every view (the
       step runs them once: portRollbackStepEffects), and the flash and the arcs only draw (portRollbackPass), held
       or not */
    if (fxSheetTimer > 0) {
        RB_VIEW_DRAW(fxSheetDisplay(ot));
    }
    RB_VIEW_TICK(weaponDropsUpdate(ctx, view));
    RB_VIEW_TICK(weaponFlameUpdate());
    RB_VIEW_DRAW(weaponArcsDraw(ctx, view));
    for (i = 0; i < 40; i++) {
        if (bulletRecords[i].life <= 0) {
            continue;
        }
        e = &bulletRecords[i];
        in[0] = e->p0[0] + eye[0];
        in[1] = e->p0[1] + eye[1];
        in[2] = e->p0[2] + eye[2];
        mathMulMatVec(mat, in, a);
#ifdef TM2_PORT_VISUAL
        tail = portVisualBulletTail(i, e->p1);  /* request 46 item 4 (PLAYTEST_ANSWERS.md 11): bit 4 */
#endif
        in[0] = STREAK_TAIL[0] + eye[0];
        in[1] = STREAK_TAIL[1] + eye[1];
        in[2] = STREAK_TAIL[2] + eye[2];
        mathMulMatVec(mat, in, b);
        za = a[2];
        zb = b[2];
        if (za <= 0 || zb <= 0) {
            continue;
        }
        if (e->type == 0) {
            pl = (u8 *)ctx[2];
            next = (u8 *)ctx[2] + 80;
            if (next >= (u8 *)ctx[3]) {
                continue;
            }
            ctx[2] = (u32 *)next;
            pl[3] = 4;
            pl[7] = 0x50;
        } else {
            pq = (u8 *)ctx[2];
            next = (u8 *)ctx[2] + 112;
            if (next >= (u8 *)ctx[3]) {
                continue;
            }
            ctx[2] = (u32 *)next;
            sub_80144CCC(pq);
            pq[7] &= ~2;
        }
#ifdef TM2_PORT_FAR_DISTANCE
        if (za > portFarDistanceFxLimit()) {   /* PC port request 31: follows the far distance */
            continue;
        }
#else
        if (za >= 32001) {
            continue;
        }
#endif
        lo = e->life & 0x1F;
        c = lo + 223;
        if (e->life <= 0) {
            return;
        }
        if (e->type == 0) {
            pl[4] = c;
            pl[5] = c;
            pl[6] = c >> 2;
            c = lo + 163;
            pl[12] = c;
            pl[13] = c >> 1;
            pl[14] = 0;
        } else {
            pq[4] = c;
            pq[5] = c;
            pq[6] = c;
            pq[12] = c;
            pq[13] = c;
            pq[14] = c;
            pq[20] = c >> 1;
            pq[21] = c >> 1;
            pq[22] = c;
        }
        x0 = a[0] * sx / za + cx;
        y0 = a[1] * sy / za + cy;
        x1 = b[0] * sx / zb + cx;
        y1 = b[1] * sy / zb + cy;
        if (-(cx * 2) < x0 && x0 < cx * 4 && -(cy * 2) < y0 && y0 < cy * 4 &&
            -(cx * 2) < x1 && x1 < cx * 4 && -(cy * 2) < y1 && y1 < cy * 4) {
            if (e->type == 0) {
                *(s16 *)(pl + 8) = x0;
                *(s16 *)(pl + 10) = y0;
                *(s16 *)(pl + 16) = x1;
                *(s16 *)(pl + 18) = y1;
#ifdef TM2_PORT_FAR_DISTANCE
                AddPrim(ot + portFarDistanceFxOtz(za), pl);
#else
                AddPrim(ot + (za >> 3), pl);
#endif
            } else {
                dy = (y0 - y1) / 12 + 1;
                dx = (x0 - x1) / 12;
                *(s16 *)(pq + 24) = x1;
                *(s16 *)(pq + 26) = y1;
                *(s16 *)(pq + 8) = x0 - dy;
                *(s16 *)(pq + 16) = x0 + dy;
                *(s16 *)(pq + 10) = y0 + dx;
                *(s16 *)(pq + 18) = y0 - dx;
#ifdef TM2_PORT_FAR_DISTANCE
                AddPrim(ot + portFarDistanceFxOtz(za), pq);
#else
                AddPrim(ot + (za >> 3), pq);
#endif
            }
        }
    }
}

typedef struct {
    u8 addr[3], len;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} G8PolyFT4;

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } G8Matrix;
typedef struct { s16 vx, vy, vz, pad; } G8SVector;
typedef struct { s32 vx, vy, vz, pad; } G8Vector;

typedef void (*G8Method)(u8 *, ...);
typedef struct { s16 delta; s16 index; G8Method fn; } G8VEntry;

extern s16 screenCardBufferIndex;
extern u32 *screenCardOrderTable;
extern s16 screenCardPosX, screenCardPosY, screenCardAngle;
extern u8 *soundManagerObject;
extern u8 *fileioWorkBuffer;
#pragma tm2_absolute fileioWorkBuffer
extern u32 screenCardOts[];
extern G8PolyFT4 screenCardQuads[];
extern u8 screenCardDrawEnvs[];
extern u8 screenCardDispEnvs[];
extern int DrawSync(s32);
extern u32 *ClearOTagR(u32 *, s32);
extern void screenCardSlotPos(s32, s16 *, s16 *);
extern s32 mathBlendValue(s32, s32, s32);
extern void taskMgrRunFrame(u8 *);
extern s32 rand(void);
extern void *RotMatrixYXZ(G8SVector *, G8Matrix *);
extern void SetTransMatrix(G8Matrix *);
extern void SetRotMatrix(G8Matrix *);
extern void RotTrans(G8SVector *, G8Vector *, s32 *);
extern int VSync(s32);
extern void *PutDrawEnv(void *);
extern void *PutDispEnv(void *);
extern s32 screenDisplayImage(s32, void *, s32, s32);
extern void DrawOTag(u32 *);

/* The sound object (C++): its vtable pointer sits at +200. */
typedef struct { u8 pad_00[200]; G8VEntry *vptr; } G8SoundManager;

/* Call vtable slot `e` of the sound object with sound `id`. */
static __inline__ void G8SoundSlot(G8VEntry *e, s32 id)
{
    e->fn(soundManagerObject + e->delta, id, 0, 0, 99, -1, 0);
}

/* IDA: screenDrawCard (screenDrawCard). Flips the double buffer, then (unless
 * keep) eases the card camera toward card `card`'s spot (snap != 0: jump
 * there with sound 3 and a random roll of 360 +- 0..6), rotates that card's
 * four corners into the template quad screenCardQuads[9] and queues it, queues
 * cards 0..card-1, and presents the frame over the background image. */
void __attribute__((section(".text_8012A0E0"))) screenDrawCard(s32 card, s32 snap, s32 keep)
{
    G8VEntry *e;
    s16 tx, ty;
    s32 ang;
    s32 r;
    s32 i, j;
    s16 k;
    G8Matrix m;
    G8SVector rot;
    G8SVector v;
    G8Vector out;
    s32 flag;

    if (card < 0) {
        return;
    }
    screenCardBufferIndex ^= 1;
    screenCardOrderTable = &screenCardOts[screenCardBufferIndex * 2];
    DrawSync(0);
    ClearOTagR(screenCardOrderTable, 2);
    if (keep == 0) {
        screenCardSlotPos(card, &tx, &ty);
        if (snap == 0) {
            screenCardPosX = mathBlendValue(screenCardPosX, tx, 85);
            screenCardPosY = mathBlendValue(screenCardPosY, ty, 85);
            /* this store is shared (cross-jumped) with the snap path's
             * screenCardAngle store at 8012A24C */
            screenCardAngle = mathBlendValue(screenCardAngle, 1080, 85);
        } else {
            /* slot-1 call through G8SoundSlot with the vptr read as a struct
             * member: puts a1 = 3 ahead of the sound-object loads, so reorg
             * moves it into the `bnez snap` delay slot, as in retail (either
             * change alone leaves it after the loads) */
            G8SoundSlot(((G8SoundManager *)soundManagerObject)->vptr + 1, 3);
            taskMgrRunFrame(soundManagerObject);
            e = *(G8VEntry **)(soundManagerObject + 200) + 2;
            e->fn(soundManagerObject + e->delta);
            screenCardPosX = tx;
            screenCardPosY = ty;
            r = rand();
            if (r & 8) {
                ang = -(r & 6);
            } else {
                ang = r & 6;
            }
            screenCardAngle = ang + 360;
        }
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                m.m[i][j] = (i == j) << 12;
            }
            m.t[i] = 0;
        }
        m.t[0] = screenCardPosX;
        m.t[1] = screenCardPosY;
        m.t[2] = 0;
        rot.vx = 0;
        rot.vy = 0;
        rot.vz = (screenCardAngle << 12) / 360;
        RotMatrixYXZ(&rot, &m);
        SetTransMatrix(&m);
        SetRotMatrix(&m);
        v.vx = screenCardQuads[card].x0;
        v.vy = screenCardQuads[card].y0;
        RotTrans(&v, &out, &flag);
        screenCardQuads[9].x0 = out.vx;
        screenCardQuads[9].y0 = out.vy;
        v.vx = screenCardQuads[card].x1;
        v.vy = screenCardQuads[card].y1;
        RotTrans(&v, &out, &flag);
        screenCardQuads[9].x1 = out.vx;
        screenCardQuads[9].y1 = out.vy;
        v.vx = screenCardQuads[card].x2;
        v.vy = screenCardQuads[card].y2;
        RotTrans(&v, &out, &flag);
        screenCardQuads[9].x2 = out.vx;
        screenCardQuads[9].y2 = out.vy;
        v.vx = screenCardQuads[card].x3;
        v.vy = screenCardQuads[card].y3;
        RotTrans(&v, &out, &flag);
        screenCardQuads[9].x3 = out.vx;
        screenCardQuads[9].y3 = out.vy;
        screenCardQuads[9].u0 = screenCardQuads[card].u0;
        screenCardQuads[9].u1 = screenCardQuads[card].u1;
        screenCardQuads[9].u2 = screenCardQuads[card].u2;
        screenCardQuads[9].u3 = screenCardQuads[card].u3;
        screenCardQuads[9].v0 = screenCardQuads[card].v0;
        screenCardQuads[9].v1 = screenCardQuads[card].v1;
        screenCardQuads[9].v2 = screenCardQuads[card].v2;
        screenCardQuads[9].v3 = screenCardQuads[card].v3;
        screenCardQuads[9].clut = screenCardQuads[card].clut;
        screenCardQuads[9].tpage = screenCardQuads[card].tpage;
        AddPrim(screenCardOrderTable, &screenCardQuads[9]);
    }
    for (k = 0; k < card; k++) {
        AddPrim(screenCardOrderTable, &screenCardQuads[k]);
    }
    DrawSync(0);
    VSync(0);
    PutDrawEnv(screenCardDrawEnvs + screenCardBufferIndex * 92);
    PutDispEnv(screenCardDispEnvs + screenCardBufferIndex * 20);
    screenDisplayImage(screenCardBufferIndex, fileioWorkBuffer, 0, 0);
    DrawOTag(screenCardOrderTable + 1);
}
