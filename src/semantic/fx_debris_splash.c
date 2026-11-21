/* Game helpers (8011 range). Names stay address-based. */
#include "semantic/cars.h"
#ifdef TM2_PORT_MOD_ARENA
#include "semantic/port_mod_arena.h"   /* request 114 */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

#pragma tm2_rdata .tm2_800CF250

extern s32 rand(void);
extern void *memcpy(void *, const void *, u32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 fxImpactParticleFindFree(void);
extern u8 *renderGetCurrentFrame(void);
extern void SetDrawMode(void *, s32, s32, s32, void *);
extern s32 gameGetLevel(void);
extern void aiPlaceOnRouteNode(struct TM2AIRecord *ai);
extern void worldSectionSetHeight(s32, s16 value);
extern void worldCollapseReleaseStub(s32 *);
extern void fxPlatformSoundStop(void);
extern s32 trackVehicleSurfaceIndex(s32);
extern s32 humanCarCount(void);
#include "semantic/world.h"

/* libgpu primitive tag helpers */
typedef struct { unsigned addr:24; unsigned len:8; unsigned char r0, g0, b0, code; } P_TAG;
#define setaddr(p, _addr) (((P_TAG *)(p))->addr = (unsigned long)(_addr))
#define getaddr(p) (unsigned long)(((P_TAG *)(p))->addr)
#define addPrim(ot, p) setaddr(p, getaddr(ot)), setaddr(ot, p)

/* 36-byte impact particles @80184E74 (life at +28 = fxImpactParticlesLifeField slots) */
typedef struct {
    s32 x, y, z;      /* +0 */
    s32 vx, vy, vz;   /* +12 */
    u8 frame;         /* +24 */
    u8 pad[3];
    s32 life;         /* +28 */
    u8 *anim;         /* +32 */
} Particle36;
extern Particle36 fxImpactParticles[];
extern u8 fxDebrisFramesA[];
extern u8 fxDebrisFramesB[];

/* @80112C90: impact effect - rumble for big hits (size >= 3, stronger >= 7), then spawn
 * `size` debris particles at pos with random velocity and one of two animations. */
void __attribute__((section(".text_80112C90"))) fxImpactDebris(s32 size, s32 *pos)
{
    s32 i;
    Particle36 *p;
    s32 slot;
    s32 r;

    if (size >= 3) {
        if (!viewGetSplitMode())
            soundLocateForView0(pos);
        else
            soundLocateForView(pos, -1);
        if (size >= 7)
            soundPlay(6, PadChkMtap(), soundGetListenerPan(), 99);
        else
            soundPlay(6, PadChkMtap(), soundGetListenerPan(), 75);
    }
    for (i = 0; i < size; i++) {
        slot = fxImpactParticleFindFree();
        if (slot < 0)
            return;
        p = &fxImpactParticles[slot];
        p->frame = 0;
        p->life = 40;
        p->x = pos[0];
        p->y = pos[1];
        p->z = pos[2];
        p->vx = (rand() & 63) - 32;
        p->vy = (rand() & 63) - 32;
        p->vz = (rand() & 31) + 16;
        r = rand() & 3;
        if (r == 0)
            p->anim = fxDebrisFramesA;
        else if (r == 1)
            p->anim = fxDebrisFramesB;
        else if (r == 2)
            p->anim = fxDebrisFramesA;
        else
            p->anim = fxDebrisFramesB;
    }
}

/* Flat-shaded quad primitive (POLY_F4 layout). */
typedef struct {
    u8 tag[3];
    u8 len;
    u8 r, g, b, code;
    s16 x0, y0, x1, y1, x2, y2, x3, y3;
} TM2PolyF4;
#define POLY(p) ((TM2PolyF4 *)(p))

typedef struct {
    u32 *ot;      /* +0 */
    s32 pad4;
    u8 *next;     /* +8 */
    u8 *end;      /* +12 */
} DrawCtx;

/* @8011335C explodeMakeScreenRed: add a full-screen semi-transparent red POLY_F4
 * plus a draw-mode primitive to the current ordering table. */
void __attribute__((section(".text_8011335C"))) explodeMakeScreenRed(void)
{
    DrawCtx *ctx;
    u8 *p;
    u8 *dm;

    ctx = (DrawCtx *)renderGetCurrentFrame();
    p = ctx->next;
    if (p + 144 < ctx->end) {
        ctx->next += 96;
        POLY(p)->len = 5;
        POLY(p)->code = 0x28;
        POLY(p)->r = 255;
        POLY(p)->g = 0;
        POLY(p)->b = 0;
        POLY(p)->code |= 2;
        POLY(p)->x0 = 0;
        POLY(p)->y0 = 0;
        POLY(p)->x1 = 320;
        POLY(p)->y1 = 0;
        POLY(p)->x2 = 0;
        POLY(p)->y2 = 240;
        POLY(p)->x3 = 320;
        POLY(p)->y3 = 240;
        addPrim(ctx->ot, p);
        dm = ctx->next;
        ctx->next += 48;
        SetDrawMode(dm, 0, 0, 0, 0);
        addPrim(ctx->ot, dm);
    }
}

extern s32 carPartsHammerhead[];
extern s32 carPartsDarkTooth[];
extern s32 carBodyObjects[];
extern s32 carWreckModels[];
extern s32 carAxelObjects[2];
/* Splash effect object: position at +4. */
typedef struct {
    s32 f00;
    s32 pos[3];             /* +0x04 */
} TM2SplashObject;
extern TM2SplashObject *fxSplashObjects[];
extern u8 *fxLavaSplashObjects[];
extern u8 *fxBlastObjects[];
extern u8 *fxWakeObjects[];
extern s32 slamArmObject;
extern s32 slamClawObject;
extern s32 slamArmAngle;
extern s32 slamClawAngle;
extern s32 D_80180F88;
extern s32 fxTargetHitObject;
extern s32 fxSplashPoolIndex;
extern s32 fxLavaSplashPoolIndex;
extern s32 fxBlastPoolIndex;
typedef struct { s32 a, b; } Pair8;
extern Pair8 lightFlickerTimers[];

/* @80114380: reset effect/scene state tables (counters, slot tables, ring indices). */
void __attribute__((section(".text_80114380"))) carPartsReset(void)
{
    s32 i;
    carPartsHammerhead[0] = 0;
    carPartsHammerhead[1] = 0;
    carPartsHammerhead[2] = 0;
    carPartsHammerhead[3] = 0;
    carPartsHammerhead[4] = 0;
    carPartsHammerhead[5] = 0;
    carPartsHammerhead[6] = 0;
    carPartsHammerhead[7] = 0;
    carPartsHammerhead[8] = 0;
    carPartsHammerhead[9] = 0;
    carPartsHammerhead[10] = 0;
    carPartsHammerhead[11] = 0;
    carPartsHammerhead[12] = 0;
    carPartsHammerhead[13] = 0;
    carAxelObjects[0] = 0;
    carAxelObjects[1] = 0;
    carPartsDarkTooth[0] = 0;
    carPartsDarkTooth[1] = 0;
    carPartsDarkTooth[2] = 0;
    carPartsDarkTooth[3] = 0;
    for (i = 0; i < 15; i++) {
        carBodyObjects[i] = 0;
        carWreckModels[i] = 0;
    }
    slamArmObject = 0;
    slamClawObject = 0;
    slamArmAngle = 11;
    slamClawAngle = 11;
    D_80180F88 = 0;
    fxTargetHitObject = 0;
    for (i = 0; i < 5; i++) {
        fxSplashObjects[i] = 0;
        fxLavaSplashObjects[i] = 0;
        fxBlastObjects[i] = 0;
        fxWakeObjects[i] = 0;
    }
    fxSplashPoolIndex = 1;
    fxLavaSplashPoolIndex = 1;
    fxBlastPoolIndex = 1;
    for (i = 0; i < 10; i++) {
        lightFlickerTimers[i].a = 0;
        lightFlickerTimers[i].b = 0;
    }
}

/* 40-byte effect records @80187934 */
typedef struct {
    s8 active;    /* +0 */
    u8 flag;      /* +1 */
    s8 link;      /* +2 */
    u8 pad3;
    s16 timer;    /* +4 */
    u8 pad6[2];
    s32 v[3];     /* +8 */
    u8 pad20[12];
    void *src;    /* +32 car: human (flag) or AI record */
    struct TM2Object *obj;      /* +36 */
} Rec40;
extern Rec40 worldWarps[];

/* @8011566C: start effect record idx for car record src (human when flag, else AI; unless it or its linked record is
 * busy), copy the record position into its object and rumble the pad. */
void __attribute__((section(".text_8011566C"))) worldWarpStart(s32 idx, void *src, u8 flag)
{
    Rec40 *r;
    s32 v;

    if (flag)
        v = HREC(src)->type;
    else
        v = AI(src)->type;
    if (v == 0xF0 && idx == 0)
        return;
    r = &worldWarps[idx];
    if (r->active)
        return;
    if (worldWarps[r->link].active)
        return;
    r->active = 1;
    r->timer = 0;
    r->flag = flag;
    r->src = src;
    r->obj->visible = 1;
    r->obj->pos[0] = r->v[0];
    r->obj->pos[1] = r->v[1];
    r->obj->pos[2] = r->v[2];
    if (flag || AI(src)->active) {
        if (!viewGetSplitMode())
            soundLocateForView0(r->obj->pos);
        else
            soundLocateForView(r->obj->pos, -1);
        soundPlay(0x17, PadChkMtap(), soundGetListenerPan(), 99);
    }
}

/* @80115A48: pick the route node to place the AI car on (stats spawn_node, +0x5D0) and the
 * node to head for (stats target_node, +0x53C) for the current level and warp side; if one
 * was chosen, apply it via aiPlaceOnRouteNode. */
void __attribute__((section(".text_80115A48"))) aiWarpSetExitNode(struct TM2AIRecord *ai, s16 side)
{
    u8 set = 0;

#ifdef TM2_PORT_MOD_ARENA
    /* request 114 (MOD_ARENAS.md): a mod arena's exits from portArenaRules; a spawn node below 0 or a side past the
       table is "no exit" (the car not placed, as retail's levels without one) */
    if (portArenaRules.warps) {
        if (side >= 0 && side < TM2_MOD_WARP_SIDES && portArenaRules.exit[side][0] >= 0) {
            ai->stats.spawn_node = portArenaRules.exit[side][0];
            ai->stats.target_node = portArenaRules.exit[side][1];
            aiPlaceOnRouteNode(ai);
        }
        return;
    }
#endif
    switch (gameGetLevel()) {
    case 1:
        set = 1;
        ai->stats.spawn_node = 0x4A;
        ai->stats.target_node = 0x35;
        break;
    case 6:
        if (side == 0) {
            ai->stats.spawn_node = 0x43;
            ai->stats.target_node = 0x45;
        } else {
            ai->stats.spawn_node = 0x34;
            ai->stats.target_node = 0x33;
        }
        set = 1;
        break;
    case 4:
        if (side == 0) {
            ai->stats.spawn_node = 0x52;
            ai->stats.target_node = 0x37;
        } else {
            ai->stats.spawn_node = 0x3D;
            ai->stats.target_node = 0x3B;
        }
        set = 1;
        break;
    case 3:
        if (side == 0) {
            ai->stats.spawn_node = 0x56;
            ai->stats.target_node = 0x31;
        } else {
            ai->stats.spawn_node = 0x57;
            ai->stats.target_node = 0x58;
        }
        set = 1;
        break;
    case 5:
        if (side == 0) {
            ai->stats.spawn_node = 0;
            ai->stats.target_node = 2;
        } else {
            ai->stats.spawn_node = 0x3B;
            ai->stats.target_node = 0x3C;
        }
        set = 1;
        break;
    case 8:
        if (side == 0) {
            ai->stats.spawn_node = 0x4F;
            ai->stats.target_node = 0x51;
        } else if (side == 1) {
            ai->stats.spawn_node = 0;
            ai->stats.target_node = 0xB;
        } else {
            set = 1;
            if (side != 2)
                break;
            ai->stats.spawn_node = 0x42;
            ai->stats.target_node = 0x86;
        }
        set = 1;
        break;
    case 7:
        if (side == 0) {
            ai->stats.spawn_node = 0x2E;
            ai->stats.target_node = 9;
        } else {
            ai->stats.spawn_node = 0x2F;
            ai->stats.target_node = 0x26;
        }
        set = 1;
        break;
    }
    if (set)
        aiPlaceOnRouteNode(ai);
}

/* Collapsing section object (same layout as in world_collapse_warps.c); kind 4 and
 * kind 5 keep their fields at different offsets. */
typedef struct {
    u8 kind;                /* +0x00 */
    u8 pad_01[3];
    s32 f04[3];             /* +0x04 kind 4: position */
    u8 pad_10[2];
    u8 f12;                 /* +0x12 kind 4: active */
    u8 pad_13[5];
    s32 f18[3];             /* +0x18 kind 5: position */
    u8 pad_24[2];
    u8 f26;                 /* +0x26 kind 5: active */
} TM2CollapseObject;
typedef struct { TM2CollapseObject *obj; u8 pad[32]; } Rec36;
extern Rec36 worldCollapseSections[];
typedef struct { s32 id; s32 pad[2]; } Rec12;
extern Rec12 worldSectionFloorHeights[];

/* @80116D94: release effect record n (clear its active byte by type), reset its slot id
 * and the slot ids of its paired sub-effects, then notify listeners. */
void __attribute__((section(".text_80116D94"))) worldCollapseRelease(s32 n)
{
    s32 v[3];
    TM2CollapseObject *o;

    o = worldCollapseSections[n].obj;
    if (o->kind == 4) {
        o->f12 = 0;
        memcpy(v, worldCollapseSections[n].obj->f04, 12);
        v[2] = 0;
    } else if (o->kind == 5) {
        o->f26 = 0;
        v[0] = worldCollapseSections[n].obj->f18[0];
        v[1] = worldCollapseSections[n].obj->f18[1];
        v[2] = 0;
    }
    worldSectionFloorHeights[n].id = -1;
    worldSectionSetHeight(n, -80);
    if (n == 6) {
        worldSectionFloorHeights[7].id = -1;
        worldSectionFloorHeights[8].id = -1;
        worldSectionSetHeight(7, -80);
        worldSectionSetHeight(8, -80);
    } else if (n == 13) {
        worldSectionFloorHeights[14].id = -1;
        worldSectionSetHeight(14, -80);
    } else if (n == 19) {
        worldSectionFloorHeights[20].id = -1;
        worldSectionSetHeight(20, -80);
    } else if (n == 22) {
        worldSectionFloorHeights[23].id = -1;
        worldSectionSetHeight(23, -80);
    } else if (n == 26) {
        worldSectionFloorHeights[27].id = -1;
        worldSectionSetHeight(27, -80);
    }
    worldCollapseReleaseStub(v);
    fxPlatformSoundStop();
}

extern struct TM2TrackVehicle *trackVehicleRecords;

/* @80118F38: arm record n, then run carSpinOut on every active car whose +0x5CA id
 * matches the current id, and finally re-place record n's object. */
void __attribute__((section(".text_80118F38"))) trackVehicleDestroy(s32 n)
{
    s32 v[3];
    struct TM2Object *o;
    s32 id;
    s32 cnt;
    s32 i;
    struct TM2HumanRecord *car;

    o = trackVehicleRecords[n].obj;
    trackVehicleRecords[n].busy = 1;
    o->visible = 0;
    id = trackVehicleSurfaceIndex(n);
    cnt = humanCarCount();
    for (i = 0; i < cnt; i++) {
        car = humanCarGet(i);
        if (car->wheels.wheel[0].moving || car->wheels.wheel[1].moving
            || car->wheels.wheel[2].moving || car->wheels.wheel[3].moving) {
            if (car->wheels.wheel[0].object == id)
                carSpinOut(car, 1, 4, 1);
        }
    }
    v[0] = 0x5DC0;
    v[1] = 0x5DC0;
    v[2] = 0;
    trackVehicleSurfaceSet(n, v, 0, trackVehicleRecords[n].obj->rot[2]);
}

/* @80119174: place the next ring slot's effect object at pos, trigger it, and rumble
 * the pad if close enough (strength by distance d). */
void __attribute__((section(".text_80119174"))) fxSplashAt(s32 *pos, s32 d)
{
    s32 i;
    s32 m;

    if (fxSplashPoolIndex >= 5)
        fxSplashPoolIndex = 1;
    i = fxSplashPoolIndex;
    if (fxSplashObjects[i]) {
        fxSplashObjects[i]->pos[0] = pos[0];
        fxSplashObjects[i]->pos[1] = pos[1];
        fxSplashObjects[i]->pos[2] = pos[2];
        objRegistrySetStep(0x2AC, i, 1);
        fxSplashPoolIndex++;
        if (!viewGetSplitMode())
            soundLocateForView0(pos);
        else
            soundLocateForView(pos, -1);
        m = PadChkMtap();
        if ((m < 0 ? -m : m) < 1500) {
            if ((d < 0 ? -d : d) < 1200)
                soundPlay(0x21, PadChkMtap(), soundGetListenerPan(), 99);
            else
                soundPlay(0x22, PadChkMtap(), soundGetListenerPan(), 99);
        }
    }
}
