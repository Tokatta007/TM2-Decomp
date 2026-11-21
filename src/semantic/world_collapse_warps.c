/* Game code (8011..8012 range). Names stay address-based. */
/* humanCars / aiCars are declared below with this file's own record views. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* 40-byte effect records @80187934 (see fx_debris_splash.c) */
typedef struct {
    u8 active;    /* +0 */
    u8 flag;      /* +1 */
    s8 link;      /* +2 */
    u8 pad3;
    s16 timer;    /* +4 */
    u8 pad6[14];
    s32 pos[3];   /* +20 */
    void *car;    /* +32 human (flag) or AI record */
    struct TM2Object *obj;      /* +36 */
} Rec40;
extern Rec40 worldWarps[];

extern void wheelBlockReset(struct TM2CarWheels *);
extern s32 worldSectionAt(s32, s32, s32);
extern void wheelBlockSetSection(struct TM2CarWheels *, s32);
extern void wheelBlockSetHeight(struct TM2CarWheels *, s32);
extern void carBodyAttitudeUpdate(void *car, s32);
extern void aiWarpSetExitNode(struct TM2AIRecord *ai, s16 side);

/* @801157D4: tick the three effect records: freeze the car for 10 frames, then at frame 10
 * warp it to the record position (re-init wheels/region), at 11 re-enable, at 40 finish. */
void __attribute__((section(".text_801157D4"))) worldWarpsUpdate(void)
{
    s32 *pos = 0;
    s16 *ang = 0;
    s32 i;
    Rec40 *r;
    void *car;              /* human (r->flag) or AI record */
    struct TM2CarMotion *b;
    struct TM2CarWheels *w;
    struct TM2CarStatsBlock *c;
    s32 dummy;

    for (i = 0; i < 3; i++) {
        r = &worldWarps[i];
        if (r->active) {
            car = r->car;
            b = &HREC(car)->motion;
            if (r->flag) {
                w = &HREC(car)->wheels;
                c = &HREC(car)->stats;
                dummy = HREC(car)->type;
                pos = HREC(car)->cam_pos;
                ang = HREC(car)->cam_rot;
            } else {
                b = &AI(car)->motion;
                w = &AI(car)->wheels;
                c = &AI(car)->stats;
                dummy = AI(car)->type;
            }
            r->timer++;
            if (r->timer < 10) {
                b->vel0 = 0;
                b->speed = 0;
                b->vel2 = 0;
            }
            if (r->timer == 10) {
                c->probe_on = 0;
                b->pos[0] = r->pos[0];
                b->pos[1] = r->pos[1];
                b->pos[2] = r->pos[2];
                wheelBlockReset(w);
                wheelBlockSetSection(w, worldSectionAt(r->pos[0], r->pos[1], b->pos[2]));
                wheelBlockSetHeight(w, b->pos[2]);
                carBodyAttitudeUpdate(car, r->flag);
                if (r->flag) {
                    pos[0] = b->pos[0];
                    pos[1] = b->pos[1];
                    pos[2] = b->pos[2];
                    ang[0] = b->rot[0];
                    ang[1] = b->rot[1];
                    ang[2] = b->rot[2];
                } else {
                    aiWarpSetExitNode(car, i);
                }
                r->obj->pos[0] = r->pos[0];
                r->obj->pos[1] = r->pos[1];
                r->obj->pos[2] = r->pos[2];
            }
            if (r->timer == 11)
                c->probe_on = 1;
            if (r->timer == 40) {
                r->obj->visible = 0;
                r->active = 0;
            }
        }
    }
}

/* Collapsing section object (same layout as in fx_debris_splash.c); kind 4 and
 * kind 5 keep their position at different offsets. */
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

/* 36-byte moving-object records @80187448 */
typedef struct {
    TM2CollapseObject *obj;     /* +0 */
    s32 delta;    /* +4 */
    u8 pad8[28];
} Rec36;
extern Rec36 worldCollapseSections[];
typedef struct { s32 x, y, z; } Vec3;
extern Vec3 worldSections[];
extern s32 worldSectionLengths[]; /* [35] x sizes, then [35] y sizes */
extern void fxPlatformSoundStart(s32 *);

/* @80116B4C: move object i by its height delta (object type 4 -> +0xC, type 5 -> +0x20,
 * plus its position z, and linked follower records for 6/13/19/22/26), then refresh it. */
void __attribute__((section(".text_80116B4C"))) worldCollapseMoveSection(s32 i)
{
    TM2CollapseObject *o;
    s32 v[3];

    o = worldCollapseSections[i].obj;
    if (o->kind == 4)
        o->f04[2] += worldCollapseSections[i].delta;
    else if (o->kind == 5)
        o->f18[2] += worldCollapseSections[i].delta;
    worldSections[i].z += worldCollapseSections[i].delta;
    if (i == 6) {
        worldSections[7].z += worldCollapseSections[7].delta;
        worldSections[8].z += worldCollapseSections[8].delta;
    } else if (i == 13) {
        worldSections[14].z += worldCollapseSections[14].delta;
    } else if (i == 19) {
        worldSections[20].z += worldCollapseSections[20].delta;
    } else if (i == 22) {
        worldSections[23].z += worldCollapseSections[23].delta;
    } else if (i == 26) {
        worldSections[27].z += worldCollapseSections[27].delta;
    }
    v[0] = worldSections[i].x + worldSectionLengths[i] / 2;
    v[1] = worldSections[i].y + worldSectionLengths[i + 35] / 2;
    v[2] = worldSections[i].z;
    fxPlatformSoundStart(v);
}

extern s32 worldCollapseTimer; /* race timer (seconds) */
extern s32 worldCollapseDebrisCooldown; /* spawn cooldown */
extern s32 worldCollapseDebrisCount; /* spawned count */
extern s32 rand(void);
extern u8 fxSpawnPooledBlast(s32 *);
extern s32 gameGetMode(void);
/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern int fxExplosionB96Fast(s32 *, char arg1);
extern void fxExplosionB48(s32 *, s32);
extern void fxSmokeC48(s32 *, s32);
extern void fxSmokeC24(s32 *, s32);

/* @801176C0: random pickup/hazard spawner: occasionally spawn an extra item (limited by
 * elapsed time), otherwise count down the cooldown and spawn one of four random kinds. */
void __attribute__((section(".text_801176C0"))) worldCollapseDebris(s32 *p)
{
    u8 done = 0;

    if (worldCollapseDebrisCount < worldCollapseTimer / 30 + 1 && (rand() & 7) == 1) {
        p[2] -= rand() % 120;
        if (fxSpawnPooledBlast(p)) {
            done = 1;
            worldCollapseDebrisCount++;
        }
    }
    if (gameGetMode() == 4 || gameGetMode() == 2)
        done = 1;
    if (done)
        return;
    if (worldCollapseDebrisCooldown > 0) {
        worldCollapseDebrisCooldown -= timerGetFrameTicks();
        return;
    }
    if (worldCollapseTimer < 120)
        worldCollapseDebrisCooldown = 120;
    else if (worldCollapseTimer < 240)
        worldCollapseDebrisCooldown = 60;
    else
        worldCollapseDebrisCooldown = 20;
    if ((rand() & 3) == 0) {
        p[2] += rand() % 240;
        fxExplosionB96Fast(p, 0);
    } else if ((rand() & 3) == 1) {
        fxExplosionB48(p, 0);
    } else {
        p[2] += rand() % 120;
        if (rand() & 1)
            fxSmokeC48(p, 0);
        else
            fxSmokeC24(p, 0);
    }
}

/* AI car record (0x854 bytes; see include/semantic/cars.h) */
typedef struct {
    u8 pad000[0x6];
    s16 slot;         /* +0x006 */
    u8 pad008[0x28];
    s32 obj;          /* +0x030 */
    u8 pad034[0x38];
    u8 dead;          /* +0x06C */
    u8 pad06D[0x493];
    s8 smoke_arg;     /* +0x500 */
    s8 smoke;         /* +0x501 damage smoke level 0..2 */
    u8 pad502[0x34];
    s16 max_hp;       /* +0x536 */
    u8 pad538[0x68];
    s32 hp;           /* +0x5A0 */
    u8 pad5A4[0x198];
    s32 pos[3];       /* +0x73C */
    s32 rot[3];       /* +0x748 */
    u8 pad754[0x100];
} AICar;
extern AICar aiCars[];
extern s32 aiCarsRemaining;
extern void carSetDamageStage(s32, s32, s32);
extern void carBodyObjectShow(s32);
extern s32 gameGetLevel(void);
extern void aiRoutePatchEiffelTower(void);
#include "semantic/car_groups.h"
/* request 32 (CAR_GROUPS.md): AI car i's key */
#ifdef TM2_PORT_CAR_GROUPS
#define AI_KEY(i) TM2_CAR_KEY(aiCars[i].slot, aiCars[i].obj)
#else
#define AI_KEY(i) aiCars[i].obj
#endif

/* @8011BBD4: update AI car i's damage smoke level from its hp percentage (>40, >12, >0);
 * when the hp runs out, destroy it once and (mode 3, few left) trigger aiRoutePatchEiffelTower. */
void __attribute__((section(".text_8011BBD4"))) aiCarDamageUpdate(s32 i)
{
    s32 hp = aiCars[i].hp;
    s32 pct = hp * 100 / aiCars[i].max_hp;

    if (pct > 40) {
        if (aiCars[i].smoke) {
            carSetDamageStage(AI_KEY(i), 0, aiCars[i].smoke_arg);
            carBodyObjectShow(AI_KEY(i));
            aiCars[i].smoke = 0;
        }
    } else if (pct > 12) {
        if (aiCars[i].smoke != 1) {
            carSetDamageStage(AI_KEY(i), 1, aiCars[i].smoke_arg);
            aiCars[i].smoke = 1;
        }
    } else if (pct > 0) {
        if (aiCars[i].smoke != 2) {
            carSetDamageStage(AI_KEY(i), 2, aiCars[i].smoke_arg);
            aiCars[i].smoke = 2;
        }
    } else if (hp <= 0) {
        if (!aiCars[i].dead) {
            carBodyObjectHide(AI_KEY(i));
            aiCars[i].dead = 1;
            carDestroyed(&aiCars[i], 0);
        }
#ifdef TM2_PORT_HAZARDS
        /* request 38 G11: Eiffel off = the routes stay the standing tower's */
        if (gameGetLevel() == 3 && aiCarsRemaining < 3 && !portArenaOff(TM2_ARENA_EIFFEL))
#else
        if (gameGetLevel() == 3 && aiCarsRemaining < 3)
#endif
            aiRoutePatchEiffelTower();
    }
}

/* Human car record (0x814 bytes; see include/semantic/cars.h) */
typedef struct {
    u8 pad000[0xC];
    s16 cam_rot[3];   /* +0x00C */
    u8 pad012[2];
    s32 cam_pos[3];   /* +0x014 */
    u8 pad020[0xEC];
    s32 view;         /* +0x10C */
    u8 pad110[0x5EC];
    s32 pos[3];       /* +0x6FC */
    u8 pad708[0x10C];
} HumanCar;
extern HumanCar humanCars[];
extern s32 viewWatchedAiCar; /* AI car being watched */
extern s32 demoWatchTimer;
extern s32 demoRespawnTimer;
extern s32 demoFollowPlayerCamera;
extern s32 aiCarTotal; /* AI car count */
extern s32 sub_8013FC44(s32);
extern s32 ratan2(s32, s32);
extern void carSetViewMode(HumanCar *, s32);

/* @8011EDD0: switch the spectator camera to the next AI car: either aim player 0's camera
 * (view 5) at it from the player position, or copy the AI car's pose into the camera. */
void __attribute__((section(".text_8011EDD0"))) demoWatchNextAiCar(void)
{
    s32 dx, dy, dz;

    demoWatchTimer = 0;
    if (demoFollowPlayerCamera) {
        humanCars[0].view = 5;
        if (!demoRespawnTimer) {
            if (++viewWatchedAiCar >= aiCarTotal)
                viewWatchedAiCar = 0;
            dx = aiCars[viewWatchedAiCar].pos[0] - humanCars[0].pos[0];
            dy = aiCars[viewWatchedAiCar].pos[1] - humanCars[0].pos[1];
            dz = aiCars[viewWatchedAiCar].pos[2] - humanCars[0].pos[2];
            humanCars[0].cam_rot[0] = -ratan2(dz, sub_8013FC44(dx * dx + dy * dy));
            humanCars[0].cam_rot[1] = 0;
            humanCars[0].cam_rot[2] = ratan2(dx, dy);
        }
    } else {
        if (++viewWatchedAiCar >= aiCarTotal)
            viewWatchedAiCar = 0;
        humanCars[0].cam_rot[0] = aiCars[viewWatchedAiCar].rot[0];
        humanCars[0].cam_rot[1] = aiCars[viewWatchedAiCar].rot[1];
        humanCars[0].cam_rot[2] = aiCars[viewWatchedAiCar].rot[2];
        humanCars[0].cam_pos[0] = aiCars[viewWatchedAiCar].pos[0];
        humanCars[0].cam_pos[1] = aiCars[viewWatchedAiCar].pos[1];
        humanCars[0].cam_pos[2] = aiCars[viewWatchedAiCar].pos[2];
        carSetViewMode(&humanCars[0], 7);
    }
}

/* 20-byte texture slot presets (see bitmap_record_append.c) */
typedef struct {
    s32 page;     /* +0 tpage bits */
    s32 x;        /* +4 */
    s32 y;        /* +8 */
    s32 clut_x;   /* +12 */
    s32 clut_y;   /* +16 */
} TexPreset;
extern TexPreset *vramTexSlots;
extern s32 vramTexSlotGroup;
typedef struct {
    u32 cnt : 7;
    u32 flag : 1;
    u32 pad : 24;
} PrimHdr;
/* uv + tpage word of a textured primitive (tpage in the high half) */
typedef struct {
    u32 u : 8;
    u32 v : 8;
    u32 tpage : 5;  /* tpage x/y bits */
    u32 abr : 2;
    u32 tp : 2;     /* colour mode: 0 = 4-bit, 1 = 8-bit, 2 = 15-bit */
    u32 rest : 7;
} UVTpWord;

/* @80120744: re-point every textured primitive of a model (prim list at +0xC, count at
 * +0x10) at the current texture-slot preset: rewrite CLUT, tpage and shift its UVs (or, for
 * prims with the flag bit, the texture-window offsets in the word after the last UV).
 * q = uv0/clut word, w = uv1/tpage word, r = cursor. Matching notes: r is reassigned after
 * w = r + 4 so CSE cannot fold w-relative addresses back onto q; the tpage index goes
 * through t; preset fields are read as vramTexSlots[n] (offset + base add order).
 * In the uv branch the third UV pointer steps the copy r (r += 4; w = r), which forms
 * w + 4 from r as retail does (addiu a1,a3,4). */
/* Model record: primitive list at +0xC, primitive count at +0x10. */
typedef struct {
    u8 pad_00[0xC];
    u8 *prims;              /* +0x0C */
    u32 count;              /* +0x10 */
} TM2ModelPrims;
#ifdef TM2_PORT_TEXTURE_BANKS
#include "semantic/tex_banks.h"   /* request 32 step 4 */
#endif

void __attribute__((section(".text_80120744"))) modelRemapTextures(TM2ModelPrims *model)
{
    u8 *p = model->prims;
    u32 i;
    u8 *d;
    u8 *q;
    u8 *r;
    u8 *w;
    s32 n;
    s32 t;
    s32 u;
    u32 v;

    for (i = 0; i < model->count; i++, p += p[2] * 4) {
        if (((PrimHdr *)p)->cnt > 0) {
            d = p + 16;
            r = d + ((p[15] >> 2) & 0x1C);
            w = r + 4;
            q = r;
            r = w;
            t = w[2] & 0x1F;
            n = vramTexSlotGroup * TM2_TEX_SLOT_STRIDE + t;   /* 20; 32 with TM2_PORT_CARS (players.h) */
            /* q + 2: CLUT half of the uv0/clut word (primitive bytes, kept raw) */
            *(s16 *)(q + 2) = ((vramTexSlots[n].clut_x >> 4) & 0x3F) | ((vramTexSlots[n].clut_y & 0x3FF) << 6);
            ((UVTpWord *)w)->tpage = vramTexSlots[n].page;
#ifdef TM2_PORT_TEXTURE_BANKS
            /* request 32 step 4 (TEXTURE_BANKS.md 2.4, "As built"): while banks are live, the slot's bank
               (its tpage bits 9..15, vramPlaceTexture) into the primitive's tpage bits 9..15: every GPU
               packet built from the primitive carries it; 0 for the arena's images */
            if (portTexBanks.live)
                ((UVTpWord *)w)->rest = vramTexSlots[n].page >> 9;
#endif
            if ((*(u32 *)w & 0x1800000) == 0) {
                u = vramTexSlots[n].x % 64 * 4;
                v = vramTexSlots[n].y & 0xFF;
            } else if ((*(u32 *)w & 0x1800000) == 0x800000) {
                u = vramTexSlots[n].x % 64 * 2;
                v = vramTexSlots[n].y & 0xFF;
            } else {
                u = vramTexSlots[n].x % 64;
                v = vramTexSlots[n].y & 0xFF;
            }
            if (((PrimHdr *)p)->flag) {
                r += 4;
                if (((PrimHdr *)p)->cnt >= 4)
                    r += 4;
                r += 4;
                *(u32 *)r = (*(u32 *)r & 0xFFFF83FF) | ((u << 7) & 0x7C00);
                *(u32 *)r = (*(u32 *)r & 0xFFF07FFF) | ((v >> 3) << 15);
            } else {
                q[0] += u;
                q[1] += v;
                w[0] += u;
                w[1] += v;
                r += 4;
                w = r;
                w[0] += u;
                w[1] += v;
                if (((PrimHdr *)p)->cnt >= 4) {
                    w += 4;
                    w[0] += u;
                    w[1] += v;
                }
            }
        }
    }
}
