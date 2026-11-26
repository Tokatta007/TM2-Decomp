/* Game code (8010..8011 range). Names stay address-based. */
#pragma tm2_rdata .tm2_800CF230
/* 8011995C (round/arena start setup) is not here: its jump table (800CF2B0) is not adjacent to
   80115310's (800CF230), and only one rdata table pragma is honoured per file. */
/* humanCars is declared below with this file's own record view. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"

extern s32 projectileHitTest(s32 *, s32, s32, s32);
extern void fxExplosion24(s32 *, u8 flag);
#include "semantic/weapon_calls.h"
extern s32 areaDamageUpDir[];
extern void fxExplosion48(s32 *, u8 flag);
extern s32 gameGetLevel(void);
extern void moscowEventsTrigger(s32 *);
extern void laEventsTrigger(s32 *);
extern void eiffelTowerBombCheck(s32 *);
#include "semantic/world.h"
#include "semantic/port_far_distance.h"

/* 28-byte weapon/effect slots @80184B04 */
typedef struct {
    s8 kind;      /* +0 */
    u8 active;    /* +1 */
    u8 pad2[2];
    s16 owner;    /* +4 */
    s16 arg6;     /* +6 */
    s16 timer;    /* +8 */
    s16 pad10;
    s32 pos[3];   /* +12 */
    s32 *obj;     /* +24 */
} Slot28;
extern u8 weaponDrops[];

/* @80109A9C: slot i (kind 20/19): probe for a hit along its position (raised 30 units); on hit, fire the
   kind-specific impact (damage / mode-specific effects), release the object and clear the timer. */
void __attribute__((section(".text_80109A9C"))) weaponDropCheckHit(s32 i)
{
    Slot28 *s = (Slot28 *)(weaponDrops + i * 28);
    s32 dmg = s->owner;
    s32 hit;
    s32 car;

    if (s->kind == 20 && s->timer < 260)
        dmg = 0;
    if (s->kind == 19 && s->timer < 32727)
        dmg = 0;
    s->pos[2] += 30;
    hit = projectileHitTest(s->pos, dmg, 12, s->kind);
    s->pos[2] -= 30;
    if (hit == 0 || hit == 8)
        return;
    if (s->kind == 20) {
        s->pos[2] += 30;
        fxExplosion24(s->pos, 0);
        weaponApplyHit(hit, 0, 0, 15, areaDamageUpDir, s->owner);
        if (s->obj != 0)
            s->obj[2] = 0;
    }
    if (s->kind == 19) {
        s->pos[2] += 30;
        fxExplosion48(s->pos, 0);
        if (gameGetLevel() == 2)
            moscowEventsTrigger(s->pos);
        if (gameGetLevel() == 1)
            laEventsTrigger(s->pos);
        if (gameGetLevel() == 3)
            eiffelTowerBombCheck(s->pos);
        areaDamage(s->pos, s->arg6, 1);
        car = TM2_HUMAN_INDEX_OF_ID(s->owner);
        if (humanCarGet(car)->weapons.drop_armed != 0) {
            humanCarGet(car)->weapons.drop_armed = 0;
            humanCarGet(car)->weapons.held--;
            humanCarGet(car)->weapons.ammo[3]--;
        }
        if (s->obj != 0)
            s->obj[2] = 0;
        s->active = 0;
    }
    s->timer = 0;
}

extern void viewGetCentre(s32, s32 *, s32 *);
extern s32 viewGetProjScaleX(s32);
extern s32 viewGetProjScaleY(s32);
extern s32 *viewGetEyeTrans(s32);
extern s16 *viewGetViewMatrix(s32);
extern void AddPrim(u32 *, void *);

/* 8-byte sprite frame: u, v, w, h, tpage, clut */
typedef struct {
    u8 u, v, w, h;
    u16 tpage;
    u16 clut;
} Frame8;

/* 36-byte impact particles @80184E74 */
typedef struct {
    s32 x, y, z;      /* +0 */
    s32 vx, vy, vz;   /* +12 */
    u8 frame;         /* +24 */
    u8 pad[3];
    s32 life;         /* +28 */
    Frame8 *anim;     /* +32 */
} Particle36;
extern u8 fxImpactParticles[];

/* Prim allocator context: [0] = OT, [2] = next free prim, [3] = prim area end. */
/* @80112F6C: draw the 20 live impact particles as textured sprites (POLY_FT4) sized by
   depth, culled to the screen and depth range, using view `view`. */
void __attribute__((section(".text_80112F6C"))) fxImpactParticlesDraw(s32 *ctx, s32 view)
{
    s32 ofsx, ofsy;
    s32 v[3];
    s32 out[3];
    u32 *ot;
    s32 h1, h2;
    s32 *eye;
    s16 *mat;
    s32 i, off;
    Particle36 *p;
    s32 f;
    s32 z, sx, sy, half;
    u8 *prim;

    ot = (u32 *)*ctx;
    viewGetCentre(view, &ofsx, &ofsy);
    h1 = viewGetProjScaleX(view);
    h2 = viewGetProjScaleY(view);
    eye = viewGetEyeTrans(view);
    mat = viewGetViewMatrix(view);
    for (i = 0, off = 0; i < 20; i++, off += 36) {
        p = (Particle36 *)(fxImpactParticles + off);
        if (p->life <= 0)
            continue;
        f = p->frame;
        v[0] = p->x + eye[0];
        v[1] = p->y + eye[1];
        v[2] = p->z + eye[2];
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
            z = 8;
        sx = ofsx + out[0] * h1 / z;
        sy = ofsy + out[1] * h2 / z;
        half = h1 * 10 / z;
#ifdef TM2_PORT_FAR_DISTANCE
        z = portFarDistanceFxOtz(z);        /* the emitter's OT mapping in far mode */
#else
        z >>= 3;
#endif
        if ((u32)(sx + 99) >= 1099)
            continue;
        if ((u32)(sy + 99) >= 1099)
            continue;
        prim = (u8 *)ctx[2];
        if ((u32)ctx[3] < (u32)(prim + 160))
            continue;
        ctx[2] += 160;
        prim[3] = 9;
        prim[7] = 0x2C;
        prim[7] |= 1;
        prim[12] = p->anim[f].u;
        prim[13] = p->anim[f].v;
        prim[20] = p->anim[f].u + p->anim[f].w;
        prim[21] = p->anim[f].v;
        prim[28] = p->anim[f].u;
        prim[29] = p->anim[f].v + p->anim[f].h;
        prim[36] = p->anim[f].u + p->anim[f].w;
        prim[37] = p->anim[f].v + p->anim[f].h;
        *(u16 *)(prim + 22) = p->anim[f].tpage;
        *(u16 *)(prim + 14) = p->anim[f].clut;
        *(s16 *)(prim + 8) = sx - half;
        *(s16 *)(prim + 10) = sy - half;
        *(s16 *)(prim + 16) = sx - half + half * 2;
        *(s16 *)(prim + 18) = sy - half;
        *(s16 *)(prim + 24) = sx - half;
        *(s16 *)(prim + 26) = sy - half + half * 2;
        *(s16 *)(prim + 32) = sx - half + half * 2;
        *(s16 *)(prim + 34) = sy - half + half * 2;
        AddPrim(ot + z, prim);
    }
}
/* 40-byte linked warp records @80187934 (see fx_debris_splash.c) */
typedef struct {
    s8 active;    /* +0 */
    u8 flag;      /* +1 */
    s8 link;      /* +2: index of the paired record */
    u8 pad3;
    s16 timer;    /* +4 */
    u8 pad6[2];
    s32 v[3];     /* +8: this record's position */
    s32 dst[3];   /* +20: destination position */
    u8 *src;      /* +32 */
    u8 *obj;      /* +36 */
} Warp40;
extern Warp40 worldWarps[];

#define SETW(r, a0, a1, a2, b0, b1, b2, l) \
    (r)->v[0] = (a0); (r)->v[1] = (a1); (r)->v[2] = (a2); \
    (r)->dst[0] = (b0); (r)->dst[1] = (b1); (r)->dst[2] = (b2); (r)->link = (l)

/* @80115310: reset the 3 linked warp records, then set up the warp pair(s) (positions,
   destinations and link indices) for arena `level` (1..8; 2 has none). */
void __attribute__((section(".text_80115310"))) worldWarpsSetup(s32 level)
{
    s32 i;
    Warp40 *r0, *r1, *r2;

    for (i = 0; i < 3; i++) {
        worldWarps[i].active = 0;
        worldWarps[i].timer = 0;
        worldWarps[i].src = 0;
    }
    r0 = worldWarps;
    r1 = r0 + 1;
    r2 = r0 + 2;
    switch (level) {
    case 1:
        SETW(r0, 0, 1016, 264, 3144, 4368, 360, 1);
        SETW(r1, 3144, 4368, 360, 0, 1016, 264, 0);
        break;
    case 4:
        SETW(r0, 304, 14200, 208, -6968, 9336, 136, 1);
        SETW(r1, -6968, 8800, 168, -120, 14200, 216, 0);
        break;
    case 6:
        SETW(r0, 800, 2784, 4848, 15072, -3440, 4016, 1);
        SETW(r1, 15256, -2736, 4016, 1792, 192, 4816, 0);
        break;
    case 3:
        SETW(r0, -384, 7920, 192, 0, 7584, 832, 2);
        SETW(r1, 0, -6144, 232, 0, -6144, 1344, 2);
        break;
    case 8:
        SETW(r0, 0, 0, 1216, 0, 0, 1776, 2);
        SETW(r1, 0, -2664, 1056, 0, -4800, 400, 2);
        SETW(r2, 0, -4800, 400, 0, -2664, 1056, 1);
        break;
    case 5:
        SETW(r0, -4896, -1896, 6424, -5496, -2696, 8264, 2);
        SETW(r1, 5400, 2200, 6400, 1400, 3400, 7616, 2);
        break;
    case 7:
        SETW(r0, 4800, 4600, 24, 4072, 3960, 520, 2);
        SETW(r1, 4752, -4960, 24, 4112, -4160, 520, 2);
        break;
    }
}
extern void carSetDamageStage(s32, s32, s32);
extern void carBodyObjectShow(s32);
extern void fxSmokeA24(void *, s32);
extern void rtScheduleEvent(s32, s32);

/* 2068-byte human car records @80187B10 (fields used here) */
typedef struct {
    u8 pad0[4];
    s16 slot;       /* +4 */
    u8 pad6[138];
    s32 model;      /* +144 */
    u8 pad148[8];
    u8 dead;        /* +156 */
    u8 pad157[35];
    u8 alt_sfx;     /* +192 */
    u8 pad193[143];
    s8 smoke_arg;   /* +336 */
    s8 dmg_state;   /* +337: 0 ok, 1 damaged, 2 critical, 4 = no smoke emitter */
    u8 pad338[52];
    s16 max_hp;     /* +390 */
    u8 pad392[104];
    s32 hp;         /* +496 */
    u8 pad500[1288];
    u8 smoke[280];  /* +1788 */
} HumanCar2068;
extern HumanCar2068 humanCars[];
#include "semantic/car_groups.h"
/* request 32 (CAR_GROUPS.md): human i's key */
#ifdef TM2_PORT_CAR_GROUPS
#define HUMAN_KEY(i) TM2_CAR_KEY(humanCars[i].slot, humanCars[i].model)
#else
#define HUMAN_KEY(i) humanCars[i].model
#endif
extern u8 carResetWeapons[];
extern s32 gameMode;
extern s32 roundOverFlag;
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H5: portHumanRules */
#endif
#include "semantic/port_human_wreck.h"  /* request 126: portHumanWreckOut */

/* @8011B89C: update human car i's damage look from its hp percentage (>40 ok, >12 damaged,
   >0 critical: switch model damage stage and smoke); at hp <= 0 mark it destroyed once,
   flag the round end and play the destroyed sound for its screen. */
void __attribute__((section(".text_8011B89C"))) humanCarDamageUpdate(s32 i)
{
    s32 hp = humanCars[i].hp;
    s32 pct = hp * 100 / humanCars[i].max_hp;

#ifdef TM2_PORT_HUMAN_WRECK
    portHumanWreckTick(i);      /* request 126: the wreck clock (a respawn ends it; at its seconds the break-up) */
#endif
    if (pct > 40) {
        if (humanCars[i].dmg_state != 0) {
            carSetDamageStage(HUMAN_KEY(i), 0, humanCars[i].smoke_arg);
            carBodyObjectShow(HUMAN_KEY(i));
            if (humanCars[i].dmg_state != 4)
                fxSmokeA24(humanCars[i].smoke, 0);
            humanCars[i].dmg_state = 0;
        }
    } else if (pct > 12) {
        if (humanCars[i].dmg_state != 1) {
            carSetDamageStage(HUMAN_KEY(i), 1, humanCars[i].smoke_arg);
            if (humanCars[i].dmg_state != 4)
                fxSmokeA24(humanCars[i].smoke, 0);
            humanCars[i].dmg_state = 1;
        }
    } else if (pct > 0) {
        if (humanCars[i].dmg_state != 2) {
            carSetDamageStage(HUMAN_KEY(i), 2, humanCars[i].smoke_arg);
            if (humanCars[i].dmg_state != 4)
                fxSmokeA24(humanCars[i].smoke, 0);
            humanCars[i].dmg_state = 2;
        }
    } else if (hp <= 0 && humanCars[i].dead == 0) {
        carBodyObjectHide(HUMAN_KEY(i));
        humanCars[i].dead = 1;
        carResetWeapons[i] = 1;
        carDestroyed(&humanCars[i], 1);
#ifdef TM2_PORT_HUMAN_WRECK
        portHumanWreckStart(i, HUMAN_KEY(i));   /* request 126: a burning wreck (body shown), broken up later */
#endif
#ifdef TM2_PORT_HUMANS
        /* H5: with portHumanOptions bit 1 the round goes on; the human stays down (portHumanDown) until the port's
           portHumanRespawn: no round end, no result by human 0 / the others */
        if (portHumanRules())
            return;
#endif
        roundOverFlag = 1;
        if (gameMode == 4) {
            if (i == 0) {
                if (humanCars[0].alt_sfx)
                    rtScheduleEvent(4, 210);
                else
                    rtScheduleEvent(4, 420);
            } else {
                if (humanCars[i].alt_sfx)
                    rtScheduleEvent(3, 210);
                else
                    rtScheduleEvent(3, 420);
            }
        } else {
            if (humanCars[i].alt_sfx)
                rtScheduleEvent(1, 210);
            else
                rtScheduleEvent(1, 420);
        }
    }
}
