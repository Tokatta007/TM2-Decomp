/* Game code (objects and triggers). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/port_far_distance.h"
#include "semantic/port_humans.h"   /* request 50: vacant human records (TM2_HUMAN_SKIP_VACANT) */

extern void viewGetCentre(s32, s32 *, s32 *);
extern s32 viewGetProjScaleX(s32);
extern s32 viewGetProjScaleY(s32);
extern void fxImpactParticlesDraw(s32 *, s32);
extern s32 *viewGetEyeTrans(s32);
extern s16 *viewGetViewMatrix(s32);
extern void mathMulMatVec(s16 *, s32 *, s32 *);
extern void AddPrim(u32 *, void *);
extern s32 ratan2(s32, s32);
extern s32 rsin(s32);
extern s32 rcos(s32);

/* 8-byte sprite frame: u, v, w, h, tpage, clut */
typedef struct {
    u8 u, v, w, h;
    u16 tpage;
    u16 clut;
} Frame8;

/* 48-byte effect records @80185144 (spawned by fxSpriteSpawn, updated by fxSpritesUpdate) */
typedef struct {
    u8 type;      /* +0: 1 = rotated toward its z-offset point */
    u8 follow;    /* +1: copy position from *src each frame */
    u8 tinted;    /* +2 */
    u8 r, g, b;   /* +3 */
    s8 depth;     /* +6: 1 = pull 8 OT slots forward, 8 = fixed OT slot 8 */
    u8 pad7;
    u8 scale;     /* +8 */
    u8 rate;      /* +9: frames per animation frame */
    s8 frame;     /* +10: -1 = free */
    s8 frames;    /* +11 */
    s16 loops;    /* +12 */
    s16 zofs;     /* +14 */
    Frame8 *table;/* +16 */
    s32 pos[3];   /* +20 */
    s32 vel[3];   /* +32 */
    s32 *src;     /* +44 */
} Fx48;
extern Fx48 fxSprites[];
extern u8 fxSpritesDrawParity; /* gp+396 */

/* @80110BE4: draw all live 48-byte effect records as textured sprites (POLY_FT4, optionally
   tinted) for view `view`, sized by depth; type-1 effects are rotated along their z-offset
   direction. Toggles fxSpritesDrawParity at the end. */
void __attribute__((section(".text_80110BE4"))) fxSpritesDraw(s32 *ctx, s32 view)
{
    s32 i;
    s32 sy;
    s32 *eye;
    s16 *mat;
    u32 *ot;
    s32 h1, h2;
    s32 ofsx, ofsy;
    s32 v[3];
    s32 out[3];
    Fx48 *r;
    s32 z, z0, sx, half;
    s32 tx, ty, a, dx, dy;
    u8 *prim;
    Frame8 *tb;
    s32 fi, s;

    ot = (u32 *)*ctx;
    viewGetCentre(view, &ofsx, &ofsy);
    h1 = viewGetProjScaleX(view);
    h2 = viewGetProjScaleY(view);
    fxImpactParticlesDraw(ctx, view);
    eye = viewGetEyeTrans(view);
    mat = viewGetViewMatrix(view);
    for (i = 0; i < TM2_MAX_FX_SPRITES; i++) {
        if (fxSprites[i].frame < 0)
            continue;
        r = &fxSprites[i];
        if (view > 0 && r->frame == 1 && r->loops == 0)
            continue;
        if (r->follow) {
            r->pos[0] = r->src[0];
            r->pos[1] = r->src[1];
            r->pos[2] = r->src[2];
        }
        v[0] = r->pos[0] + eye[0];
        v[1] = r->pos[1] + eye[1];
        v[2] = r->pos[2] + eye[2];
        mathMulMatVec(mat, v, out);
        z = out[2];
        if (z < -32)
            continue;
#ifdef TM2_PORT_FAR_DISTANCE
        if (z > portFarDistanceFxLimit())   /* PC port request 31: follows the far distance */
            continue;
#else
        if (z > 32000)
            continue;
#endif
        if (z < 8)
            continue;
        half = (r->table->w * r->scale * h1 / z) >> 3;
        if (half > 512)
            half = 512;
        if (half <= 0)
            half = 1;
        sx = ofsx + out[0] * h1 / z;
        sy = ofsy + out[1] * h2 / z;
        z0 = z;
#ifdef TM2_PORT_FAR_DISTANCE
        z = portFarDistanceFxOtz(z);        /* the emitter's OT mapping in far mode */
#else
        z >>= 3;
#endif
        if (r->depth == 1)
            z -= 8;
        else if (r->depth == 8)
            z = 8;
        if (z < 0)
            z = 0;
        if ((u32)(sx + 99) >= 1099)
            continue;
        if ((u32)(sy + 99) >= 1099)
            continue;
        prim = (u8 *)ctx[2];
        if ((u32)(ctx[2] + 160) >= (u32)ctx[3])
            continue;
        ctx[2] += 160;
        prim[3] = 9;
        prim[7] = 0x2C;
        prim[7] |= 2;
        prim[7] |= 1;
        if (r->tinted) {
            prim[7] &= ~1;
            prim[4] = r->r;
            prim[5] = r->g;
            prim[6] = r->b;
        }
        tb = r->table;
        fi = r->frame / r->rate;
        prim[12] = tb[fi].u;
        prim[13] = tb[fi].v;
        prim[20] = tb[fi].u + tb[fi].w;
        prim[21] = tb[fi].v;
        prim[28] = tb[fi].u;
        prim[29] = tb[fi].v + tb[fi].h;
        prim[36] = tb[fi].u + tb[fi].w;
        prim[37] = tb[fi].v + tb[fi].h;
        *(u16 *)(prim + 22) = tb[fi].tpage;
        *(u16 *)(prim + 14) = tb[fi].clut;
        if (r->type == 1) {
            v[0] = r->pos[0] + eye[0];
            v[1] = r->pos[1] + eye[1];
            v[2] = r->pos[2] + eye[2] + r->zofs;
            mathMulMatVec(mat, v, out);
            ty = ofsy + out[1] * h2 / z0;
            tx = ofsx + out[0] * h1 / z0;
            a = ratan2(ty - sy, tx - sx) + 512;
            dx = ((rcos(a) - rsin(a)) * half) >> 12;
            s = rsin(a);
            dy = ((rcos(a) + s) * half) >> 12;
            *(s16 *)(prim + 8) = tx - dx;
            *(s16 *)(prim + 10) = ty - dy;
            *(s16 *)(prim + 16) = tx + dx;
            *(s16 *)(prim + 18) = ty + dy;
            *(s16 *)(prim + 24) = sx - dx;
            *(s16 *)(prim + 26) = sy - dy;
            *(s16 *)(prim + 32) = sx + dx;
            *(s16 *)(prim + 34) = sy + dy;
        } else {
            *(s16 *)(prim + 8) = sx - half;
            *(s16 *)(prim + 10) = sy - half;
            *(s16 *)(prim + 16) = sx - half + half * 2;
            *(s16 *)(prim + 18) = sy - half;
            *(s16 *)(prim + 24) = sx - half;
            *(s16 *)(prim + 26) = sy - half + half * 2;
            *(s16 *)(prim + 32) = sx - half + half * 2;
            *(s16 *)(prim + 34) = sy - half + half * 2;
        }
        AddPrim(ot + z, prim);
    }
    fxSpritesDrawParity = 1 - fxSpritesDrawParity;
}

/* Track section tables, one object at 801A9854 (see car_wheel_sway.c). */
typedef struct {
    s32 x, z, y;
} Pos12;
typedef struct {
    Pos12 pos[35];         /* 801A9854 */
    s32 w[35];             /* 801A99F8 section length */
    s32 l[35];             /* 801A9A84 section width */
} TrackSec;
extern TrackSec worldSections;
extern s32 worldCollapseTimer; /* gp+1428: collapse timer */
/* 36-byte collapsing-section records @80187448 (one per track section) */
typedef struct {
    void *object;   /* +0: section object */
    s32 y;          /* +4: platform height offset */
    s32 t;          /* +8: countdown (-480 when triggered) / fall speed */
    s32 pad0C;
    s32 score;      /* +16: added to worldCollapseTriggersDone when done */
    s32 arg;        /* +20: passed to aiCarsRerouteFromSection */
    u8 active;      /* +24 */
    u8 falling;     /* +25 */
    u8 triggered;   /* +26 */
    u8 pad1B;
    s8 edge[6];     /* +28 */
    u8 pad22[2];
} Trig36;
typedef char tm2_trig36_size[(sizeof(Trig36) == 36) ? 1 : -1];
extern Trig36 worldCollapseSections[];
extern s32 rand(void);
extern void worldCollapseDebris(s32 *);

/* @80116FF8: building-collapse explosions for track section `a`: for each of the section's six
   edge slots in worldCollapseSections[a] that is not -1, spawn an explosion (worldCollapseDebris) at a random
   point along that edge, at height 4800..4919. Repeats 1 + timer/60 times while the timer
   worldCollapseTimer is below 300, else once. */
void __attribute__((section(".text_80116FF8"))) worldCollapseExplosions(s32 a)
{
    s32 q, h, d, r, k, n;
    s32 p[3];

    q = worldSections.w[a] / 4;
    h = worldSections.w[a] / 2;
    d = worldSections.l[a] / 2;
    if (worldCollapseTimer < 300)
        n = worldCollapseTimer / 60 + 1;
    else
        n = 1;
    for (k = 0; k < n; k++) {
        if (worldCollapseSections[a].edge[0] != -1) {
            p[0] = worldSections.pos[a].x + q + rand() % h;
            p[1] = worldSections.pos[a].z + worldSections.l[a];
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
        if (worldCollapseSections[a].edge[1] != -1) {
            r = rand() % d;
            p[0] = worldSections.pos[a].x + h + q + (d - r) * q / d;
            p[1] = worldSections.pos[a].z + d + r;
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
        if (worldCollapseSections[a].edge[2] != -1) {
            r = rand() % d;
            p[0] = worldSections.pos[a].x + h + q + r * q / d;
            p[1] = worldSections.pos[a].z + r;
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
        if (worldCollapseSections[a].edge[3] != -1) {
            p[0] = worldSections.pos[a].x + q + rand() % h;
            p[1] = worldSections.pos[a].z;
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
        if (worldCollapseSections[a].edge[4] != -1) {
            r = rand() % d;
            p[0] = worldSections.pos[a].x + (d - r) * q / d;
            p[1] = worldSections.pos[a].z + r;
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
        if (worldCollapseSections[a].edge[5] != -1) {
            r = rand() % d;
            p[0] = worldSections.pos[a].x + r * q / d;
            p[1] = worldSections.pos[a].z + d + r;
            p[2] = rand() % 120 + 4800;
            worldCollapseDebris(p);
        }
    }
}

extern u8 worldCollapseTriggerActive;  /* gp+1412: a trigger is in progress */
extern s32 worldCollapseTriggerSection; /* gp+1416: its section */
extern s32 worldCollapseTriggersDone; /* gp+1424: triggers done (score) */
extern s32 worldCollapseDebrisCount; /* gp+1436 */
/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern s32 humanCarCount(void);
extern void mathBoundAngle(s32 *);
extern void aiRouteRemoveSectionNodes(s32);
extern void aiCarsRerouteFromSection(s32, s32);
extern void worldCollapseUnlink(s32);
extern void worldCollapseMoveSection(s32);
extern void worldSectionAddHeight(s32, s32);
extern void worldCollapseRelease(s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern u32 PadChkMtap(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 mathBlendValue(s32, s32, s32);

/* @801163B8 (IDA worldCollapseUpdate): level trigger state machine. Advances the timer
   worldCollapseTimer; when idle and past 361, picks the first active section of the current stage
   (by worldCollapseTriggersDone) that a car is in (or, after 781, roughly faces; after 1081, randomly) and
   triggers it (countdown -480, rumble, aiCarsRerouteFromSection/80116F30). A triggered section shakes
   its platform until the countdown reaches 0, then falls (with explosions) until its floor
   goes below 0, which completes it and adds its score. */
void __attribute__((section(".text_801163B8"))) worldCollapseUpdate(void)
{
    s32 trig, i, end, j, n, ang, k, t, start;
    u8 found;
    struct TM2HumanRecord *car;

    trig = -1;
    worldCollapseTimer += timerGetFrameTicks();
    if (worldCollapseTriggerActive) {
        trig = worldCollapseTriggerSection;
    } else if (worldCollapseTimer >= 361) {
        if (worldCollapseTriggersDone < 6) {
            start = 0;
            end = 6;
        } else if (worldCollapseTriggersDone < 18) {
            start = 6;
            end = 18;
        } else if (worldCollapseTriggersDone < 25) {
            start = 18;
            end = 25;
        } else if (worldCollapseTriggersDone < 29) {
            start = 25;
            end = 29;
        } else {
            start = 1;
            end = 0;
        }
        found = 0;
        for (i = start; i < end && !found; i++) {
            if (worldCollapseSections[i].active == 0)
                continue;
            n = humanCarCount();
            for (j = 0; j < n && !found; j++) {
                TM2_HUMAN_SKIP_VACANT(j);   /* request 50: a vacant record looks at no section (nothing with no define) */
                car = humanCarGet(j);
                if (worldCollapseTimer >= 781) {
                    ang = ratan2(worldSections.pos[i].x + worldSections.w[i] / 2 - car->motion.pos[0],
                                 worldSections.pos[i].z + worldSections.l[i] / 2 - car->motion.pos[1])
                          - car->motion.rot[2];
                    mathBoundAngle(&ang);
                    if (__builtin_abs(ang) < 227)
                        found = 1;
                    else if (worldCollapseTimer >= 1081 && (rand() & 3) == 1)
                        found = 1;
                } else {
                    if (car->wheels.wheel[0].kind == i && car->wheels.wheel[1].kind == i &&
                        car->wheels.wheel[2].kind == i && car->wheels.wheel[3].kind == i)
                        found = 1;
                }
                if (found) {
                    worldCollapseSections[i].triggered = 1;
                    worldCollapseSections[i].t = -480;
                    worldCollapseTriggerActive = 1;
                    worldCollapseTriggerSection = i;
                    worldCollapseTimer = 0;
                    worldCollapseDebrisCount = 0;
                    trig = i;
                    aiRouteRemoveSectionNodes(trig);
                    aiCarsRerouteFromSection(trig, worldCollapseSections[i].arg);
                    worldCollapseUnlink(trig);
                }
            }
        }
    }
    if (trig < 0)
        return;
    if (worldCollapseSections[trig].falling) {
        if (worldCollapseTimer < 600)
            worldCollapseExplosions(trig);
        if (worldCollapseSections[trig].t > 0) {
            worldCollapseSections[trig].y = timerGetFrameTicks() * -(worldCollapseSections[trig].t * 2) * worldCollapseSections[trig].t / 16;
            k = -1;
        } else {
            worldCollapseSections[trig].y = 0;
            k = 0;
        }
        k += worldCollapseSections[trig].y / 32;
        if (-(timerGetFrameTicks() * 32) < k)
            worldCollapseSections[trig].t += timerGetFrameTicks();
        worldCollapseSections[trig].y = k;
        worldCollapseMoveSection(trig);
        worldSectionAddHeight(trig, worldCollapseSections[trig].y);
        if (worldSections.pos[trig].y < 0) {
            worldCollapseSections[trig].active = 0;
            worldCollapseTriggerActive = 0;
            worldCollapseTimer = 0;
            worldCollapseDebrisCount = 0;
            worldCollapseTriggersDone += worldCollapseSections[trig].score;
            worldCollapseRelease(trig);
        }
    } else if (worldCollapseSections[trig].triggered) {
        t = worldCollapseSections[trig].t;
        if (t == -480 || (u32)(t + 359) < 4 || (u32)(t + 239) < 4 || (u32)(t + 119) < 4 || t >= -9) {
            if (!viewGetSplitMode())
                soundLocateForView0(&worldSections.pos[trig]);
            else
                soundLocateForView(&worldSections.pos[trig], -1);
            soundPlay(31, PadChkMtap() / 3, 0, 99);
        }
        t = worldCollapseSections[trig].t;
        if (t < -360)
            k = 17 - (rand() & 31);
        else if (t < -240)
            k = 30 - (rand() & 63);
        else if (t < -120)
            k = 34 - (rand() & 63);
        else
            k = 31 - (rand() & 63);
        if (worldSections.pos[trig].y > 4920)
            k = -40;
        else if (worldSections.pos[trig].y < 4752)
            k = 40;
        worldCollapseSections[trig].y = mathBlendValue(worldCollapseSections[trig].y, k, 75);
        worldCollapseMoveSection(trig);
        worldSectionAddHeight(trig, worldCollapseSections[trig].y);
        worldCollapseExplosions(trig);
        worldCollapseSections[trig].t += timerGetFrameTicks();
        if (worldCollapseSections[trig].t >= 0) {
            worldCollapseSections[trig].triggered = 0;
            worldCollapseSections[trig].falling = 1;
            worldCollapseSections[trig].y = 0;
        }
    }
}
