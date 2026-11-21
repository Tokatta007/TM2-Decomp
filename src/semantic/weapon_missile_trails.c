/* Game code (800D..800E range). Names stay address-based. */
#include "semantic/weapons.h"   /* cars.h, TM2SceneObject */
#include "semantic/port_cars.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map */
#endif

/* Vertex j of an effect record (TM2Fx980.v). Pointer arithmetic, not
 * v[j]: this address form keeps retail operand order. */
#define VTX(r, j) (*((r)->v + (j)))

/* Effect record (980 bytes each, table at fxRecords); 18 36-byte segments start at +0x14C. */
typedef struct {
    u8 pad_00[4];
    s16 i0, i1, i2, i3;
    u8 pad_0C[36 - 12];
} TM2Seg36;

typedef struct {
    u8 a : 7, b : 1;        /* bitfield view of a 36-byte segment (used by the reset) */
    u8 c1, c2, c3;
    s16 i0, i1, i2, i3;
    u8 d : 7, d7 : 1;
    u8 pad_0D;
    u8 e : 3, f : 3, g : 2;
    u8 h : 4, k : 3, l : 1;
    u8 r, gg, bb, code;
    u8 pad_14[16];
} TM2SegBits;

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVec;

/* Trail strip header (effect record +0xF4): the render object's model
 * (TM2SceneObject.model) points at it. */
struct TM2TrailStrip {
    u8 f00;                 /* +0x00 */
    u8 pad_01;
    s16 f02;                /* +0x02 */
    TM2SVec *verts;         /* +0x04 vertex array (record v) */
    s32 f08;                /* +0x08 */
    TM2Seg36 *segs;         /* +0x0C segment array (record seg) */
    s32 count;              /* +0x10 segments drawn */
    s16 f14;                /* +0x14 */
    s16 f16;                /* +0x16 */
    union {
        s32 word;
        u16 half[2];
        u8 byte[4];
    } flags;                /* +0x18 */
    s32 f1C;                /* +0x1C */
    s32 f20;                /* +0x20 */
    s32 f24;                /* +0x24 */
    s32 f28;                /* +0x28 */
};
typedef char tm2_trail_strip_size[(sizeof(struct TM2TrailStrip) == 0x2C) ? 1 : -1];

typedef struct {
    u8 pad_00[0x34];
    struct TM2SceneObject *owner;   /* 0x34 */
    u8 pad_38[0x44 - 0x38];
    s8 count;               /* 0x44 */
    u8 pad_45[3];
    TM2SVec v[21];          /* 0x48 */
    struct TM2SceneObject *obj;     /* 0xF0 render object */
    struct TM2TrailStrip strip;     /* 0xF4 */
    u8 pad_120[0x14C - 0x120];
    TM2Seg36 seg[18];       /* 0x14C */
} TM2Fx980;
typedef char tm2_fx980_size[(sizeof(TM2Fx980) == 980) ? 1 : -1];

extern TM2Fx980 fxRecords[];
extern u8 fxRecordTrailSegments[];

typedef struct {
    s16 a, b;
    u8 pad[16];
} TM2Rec20;

extern int rand(void);
/* Random event record, 196 bytes (laEventGet; laEventsUpdate in
 * car_destroyed.c). Lanes 1 and 2 are used. */
struct TM2LaEvent {
    u8 enabled;             /* +0 */
    u8 active;              /* +1 */
    u8 end_time;            /* +2 */
    s8 spark_rate;          /* +3 */
    u8 done;                /* +4 */
    u8 lane_done[3];        /* +5 */
    u8 pad_08[2];
    s16 timer;              /* +10 */
    s16 section;            /* +12 world section disabled while it runs */
    s16 start[3];           /* +14 lane start time */
    u8 pad_14[4];
    s16 duration[3];        /* +24 */
    u8 pad_1E[6];
    s32 obj[3];             /* +36 */
    u8 pad_30[8];
    s32 pos[3][3];          /* +56 */
    u8 pad_5C[24];
    s16 dir[3][4];          /* +116 per-kind direction */
    u8 pad_8C[16];
    s16 axis[3][4];         /* +156 random axis */
    u8 pad_B4[16];
};
typedef char tm2_la_event_size[(sizeof(struct TM2LaEvent) == 196) ? 1 : -1];
extern struct TM2LaEvent *laEventGet(s32);
extern void worldSectionDisable(s32 id);
extern struct TM2SceneObject *csAlloc(void);
extern void csListAdd(struct TM2SceneObject *);
extern void mathMulTransVec(struct TM2CarMatrix *, s32 *, s32 *);
extern TM2Rec20 *targetPoolGet(void);
extern void targetActorHit(s32, s32 *);
extern int VectorNormal(s32 *, s32 *);
extern void carApplyDamage(s32, s32, s32 *, s32, s32);
extern s32 targetDamage(s32, s32, s32);
extern void moscowDishDamage(s32);
extern void weaponHitStub3503(s16, s16);
extern void viewSetCameraPose(s32 i, s32 *, s16 *);
extern struct TM2AIRecord *aiGetWatchedCar(void);
extern void rtSetRearView(s32);
extern s32 rtGetRearViewActive(void);
extern void rtToggleRearView(void);
extern void viewSetPosition(s32 *pos, int view);
extern void viewSetRotation(s16 *rot, int view);

/* Arm the 196-byte random event `kind`: randomise its two lanes and set the per-kind direction pattern.
 * (Identical with the lane loop written as indexed arrays over i = 1..2; the pointer-walking
 * do/while left the switch's case-3 constant in v0 instead of retail's v1.) */
void __attribute__((section(".text_800D58BC"))) laEventStart(s32 kind)
{
    struct TM2LaEvent *p = laEventGet(kind);
    s32 i;
    s16 v;

#ifdef TM2_PORT_HAZARDS
    if (portArenaOff(TM2_ARENA_LA_QUAKE))   /* request 38 G7: no quake (before its rand() and section 6) */
        return;
#endif
    if (p->active != 0)
        return;
    if (p->done != 0)
        return;
    if (p->enabled == 0)
        return;
    p->active = 1;
    p->timer = 0;
    p->spark_rate = 30;
    p->end_time = 0;
    p->done = 0;
    for (i = 1; i < 3; i++) {
        p->duration[i] = (rand() >> 9 < 21) ? 20 : rand() >> 9;
        p->axis[i][0] = rand() / 8;
        p->axis[i][1] = rand() / 8;
        p->axis[i][2] = 0xFFF;
        p->start[i] = (rand() >> 9) + 20;
        p->lane_done[i] = 0;
    }
    switch (kind) {
    case 1:
        if (rand() & 1) {
            p->dir[1][0] = 7;
            v = -7;
        } else {
            p->dir[1][0] = -7;
            v = 7;
        }
        p->dir[2][0] = v;
        p->dir[1][1] = 0;
        p->dir[1][2] = 0;
        p->dir[2][1] = 0;
        p->dir[2][2] = 0;
        break;
    case 2:
        p->dir[1][0] = 0;
        p->dir[1][1] = 7;
        p->dir[1][2] = 0;
        p->dir[2][0] = 0;
        p->dir[2][1] = -7;
        p->dir[2][2] = 0;
        break;
    case 3:
        p->dir[1][0] = 7;
        p->dir[1][1] = 0;
        p->dir[1][2] = 0;
        p->dir[2][0] = -7;
        p->dir[2][1] = 0;
        p->dir[2][2] = 0;
        break;
    case 4:
        p->dir[1][0] = 0;
        p->dir[1][1] = -7;
        p->dir[1][2] = 0;
        p->dir[2][0] = 0;
        p->dir[2][1] = 7;
        p->dir[2][2] = 0;
        break;
    }
    worldSectionDisable(p->section);
}

/* Reset all 16 effect records: default the 18 segments of each and create/attach their render objects.
 * (Identical with the segment address taken by array indexing inside the inner loop; walking
 * base/segment pointers lets loop.c hoist the constant 9, unlike retail.) */
void __attribute__((section(".text_800D87F0"))) missileTrailsInit(void)
{
    s32 i, j;
    TM2SegBits *s;
    TM2Fx980 *rec;

    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        for (j = 0; j < 18; j++) {
            s = &((TM2SegBits *)fxRecords[i].seg)[j];
            s->a = 4;
            s->b = 0;
            s->c1 = 1;
            s->c2 = 9;
            s->c3 = 9;
            s->i0 = 0;
            s->i1 = 1;
            s->i2 = 2;
            s->i3 = 3;
            s->d = 0;
            s->e = 4;
            s->f = 0;
            s->g = 2;
            s->h = 0;
            s->k = 1;
            s->l = 0;
            s->r = 0x80;
            s->gg = 0x80;
            s->bb = 0x80;
            s->code = 46;
        }
    }
    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        rec = &fxRecords[i];
        rec->strip.f00 = 0;
        rec->strip.f02 = 0;
        rec->strip.verts = fxRecords[i].v;
        rec->strip.f08 = 0;
        rec->strip.segs = fxRecords[i].seg;
        rec->strip.count = 18;
        rec->strip.f16 = 1;
        rec->strip.flags.word = 0;
        rec->strip.flags.byte[2] &= 0xFE;
        rec->strip.flags.half[0] &= 0xE000;
        rec->strip.flags.byte[2] &= 0xFD;
        rec->strip.flags.byte[2] &= 0xFB;
        rec->strip.flags.half[1] &= 0x8007;
        rec->strip.flags.byte[3] &= 0x7F;
        rec->strip.flags.byte[1] |= 0x20;
        rec->strip.f14 = 0;
        rec->strip.f1C = 0;
        rec->strip.f20 = 0;
        rec->strip.f24 = 0;
        rec->strip.f28 = 0x7FFF;
        rec->obj = csAlloc();
        rec->obj->model = (s32)&rec->strip;
        rec->obj->visible = 0;
        rec->obj->pos[0] = 0;
        rec->obj->pos[1] = 0;
        rec->obj->pos[2] = 0;
        csListAdd(rec->obj);
    }
}

/* Build effect record `index`'s strip: triangle index triples per segment, then all vertices at the
 * point 200 units above its owner (rec+0x34), copying the owner position into the render object. */
void __attribute__((section(".text_800D8AD4"))) missileTrailStart(s32 index)
{
    TM2Fx980 *rec = &fxRecords[index];
    s32 i, j;
    s32 in[3];
    s32 out[3];

    rec->strip.count = rec->count * 3;
    for (i = 0; i < rec->count; i++) {
        rec->seg[i * 3].i2 = 0;
        rec->seg[i * 3].i3 = 0;
        rec->seg[i * 3].i0 = 1;
        rec->seg[i * 3].i1 = 1;
        rec->seg[i * 3 + 1].i2 = 1;
        rec->seg[i * 3 + 1].i3 = 1;
        rec->seg[i * 3 + 1].i0 = 2;
        rec->seg[i * 3 + 1].i1 = 2;
        rec->seg[i * 3 + 2].i2 = 2;
        rec->seg[i * 3 + 2].i3 = 2;
        rec->seg[i * 3 + 2].i0 = 0;
        rec->seg[i * 3 + 2].i1 = 0;
    }
    in[0] = 0;
    in[1] = -200;
    in[2] = 0;
    mathMulTransVec(&rec->owner->matrix, in, out);
    rec->obj->pos[0] = rec->owner->pos[0];
    rec->obj->pos[1] = rec->owner->pos[1];
    rec->obj->pos[2] = rec->owner->pos[2];
    for (i = 0; i < rec->count + 1; i++) {
        j = i * 3;
        VTX(rec, j).vx = out[0];
        VTX(rec, j).vy = out[1];
        VTX(rec, j).vz = out[2];
        j++;
        VTX(rec, j).vx = out[0];
        VTX(rec, j).vy = out[1];
        VTX(rec, j).vz = out[2];
        j++;
        VTX(rec, j).vx = out[0];
        VTX(rec, j).vy = out[1];
        VTX(rec, j).vz = out[2];
    }
    rec->obj->visible = 1;
}

/* Apply a hit of `type` (breakable ids 500..5999 resolve through the breakable table, >= 6000 go to
 * targetActorHit, 1..99 spawn a car-hit effect). Returns 1 if handled. */
s32 __attribute__((section(".text_800DC65C"))) weaponApplyHit(s32 type, s32 a, s32 b, s32 strength, s32 *dir, s32 id)
{
    s32 owner;
    u8 human;
    s32 idx;
    s32 n[4];
    TM2Rec20 *t;

    if (carFindById(id, &human, &idx)) {
        if (human)
            owner = humanCarGet(idx)->slot;
        else
            owner = aiCarGet(idx)->slot;
    } else
        owner = -1;
    if ((u32)(type - 500) < 5500 && (u32)(type - 3503) >= 2) {
        t = targetPoolGet();
        a = t[type - 500].a;
        b = t[type - 500].b;
        type = 8;
    }
    if ((u32)(type - 3503) < 2)
        a = type;
    if (type >= 6000)
        targetActorHit(type - 6000, dir);
    if ((u32)(type - 1) < 99 && type != 8) {
        VectorNormal(dir, n);
        carApplyDamage(type, strength, n, 1, owner);
        return 1;
    }
    switch (a) {
    case 500: case 501: case 502: case 503: case 504: case 505:
    case 506: case 507: case 508: case 509: case 510:
    case 519: case 520:
        if (id < 50) {
            targetDamage(a, b, strength);
            return 1;
        }
        break;
    case 3005: case 3006:
        moscowDishDamage(strength);
        return 1;
    case 3503: case 3504:
        weaponHitStub3503(type - 3503, strength);
        return 1;
    default:
        return 0;
    }
    return 1;
}

/* Car shake: randomise the aim-block jitter of car (a human record when side is
 * set, else an AI record) around its current offsets. */
void __attribute__((section(".text_800E4F30"))) carShakeRandomize(void *car, u8 side)
{
    struct TM2CarAim *p;

    if (side)
        p = &((struct TM2HumanRecord *)car)->aim;
    else
        p = &((struct TM2AIRecord *)car)->aim;
    p->shake_step = 0x155;
    if (p->shake_x > 0)
        p->shake_amp_x = p->shake_x - rand() % (p->shake_x / 2);
    else if (p->shake_x < 0)
        p->shake_amp_x = p->shake_x + rand() % (p->shake_x / 2);
    if (p->shake_y > 0)
        p->shake_amp_y = p->shake_y - rand() % (p->shake_y / 2);
    else if (p->shake_y < 0)
        p->shake_amp_y = p->shake_y + rand() % (p->shake_y / 2);
}

#define CAM_COPY_POS(c) \
    (c)->cam_pos[0] = (c)->motion.pos[0]; \
    (c)->cam_pos[1] = (c)->motion.pos[1]; \
    (c)->cam_pos[2] = (c)->motion.pos[2]
#define CAM_COPY_ALL(c) \
    CAM_COPY_POS(c); \
    (c)->cam_rot[0] = 0; \
    (c)->cam_rot[1] = 0; \
    (c)->cam_rot[2] = (c)->motion.rot[2]

/* Switch camera `cam` to view `mode` (2/3 chase, 4 rear, 7 target-follow, else free) and apply it.
 * TM2_PORT_HUMANS (H1): the record's part (view, view_last, the camera fields) for every human; the view system
 * and the rear-view globals only for a local human, on its slot (one gate for every caller: bind, restart,
 * crash, hotkeys, drop-in). */
void __attribute__((section(".text_800E7380"))) carSetViewMode(struct TM2HumanRecord *human, u32 mode)
{
    u8 ok;
    s16 rot[3];
    s32 pos[3];
#ifdef TM2_PORT_HUMANS
    s32 slot = TM2_HUMAN_LOCAL(human->player);
#endif

    if (mode < 5)
        human->view_last = mode;
    ok = 0;
    if (mode < 8) {
        rot[0] = 0;
        rot[1] = 0;
        rot[2] = 0;
        pos[0] = 0;
        pos[1] = 0;
        pos[2] = 0;
#ifdef TM2_PORT_HUMANS
        if (slot >= 0)
            viewSetCameraPose(slot, pos, rot);
#else
        viewSetCameraPose(human->player, pos, rot);
#endif
        ok = 1;
    }
    switch (mode) {
    case 2:
        if (ok && human->view == mode) {
            CAM_COPY_ALL(human);
        }
#ifdef TM2_PORT_HUMANS
        if (slot >= 0)
#endif
        rtSetRearView(1);
        break;
    case 3:
        if (ok && human->view == mode) {
            CAM_COPY_POS(human);
        }
#ifdef TM2_PORT_HUMANS
        if (slot >= 0)
#endif
        rtSetRearView(1);
        break;
    case 7:
        human->cam_pos[0] = aiGetWatchedCar()->motion.pos[0];
        human->cam_pos[1] = aiGetWatchedCar()->motion.pos[1];
        human->cam_pos[2] = aiGetWatchedCar()->motion.pos[2];
        break;
    case 4:
        if (ok && human->view == mode) {
            CAM_COPY_POS(human);
        }
#ifdef TM2_PORT_HUMANS
        if (slot < 0)
            break;
#endif
        if (rtGetRearViewActive())
            rtToggleRearView();
        rtSetRearView(0);
        break;
    default:
        CAM_COPY_ALL(human);
        break;
    }
#ifdef TM2_PORT_HUMANS
    if (slot >= 0) {
        viewSetPosition(human->cam_pos, slot);
        viewSetRotation(human->cam_rot, slot);
    }
#else
    viewSetPosition(human->cam_pos, human->player);
    viewSetRotation(human->cam_rot, human->player);
#endif
    human->view = mode;
}
