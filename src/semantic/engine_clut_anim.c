/* Library code 80148000..8014C000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Almost all of this range is SingleTrac engine code: the CLUT (palette)
 * animation system, the object/coordinate-system pool (csUpdMat), pad
 * start/stop, the dbscan hierarchy walker and its record stack, and the
 * debris/burst effect slots. The one SDK function (_spu_getInTransfer,
 * adapted from sotn-decomp, MIT) compiles identically with the engine recipe.
 * Recipe: psyq3.6, -fvolatile, extern_gp, gp_size 8, delay slots compiler. */

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

extern int printf(const char *, ...);

/* Jump table of the dbscan opcode switch (dbscanHierarchy). */
#pragma tm2_rdata .tm2_800D0F8C

/* ---- CLUT (palette) animation system ------------------------------------
 * clutAnimDefs: up to 20 palette-animation definitions, clutAnimLastIndex = index of
 * the last used one (-1 = none). clutAnimRunning: 10 running animations, each
 * with two 256-colour work buffers. */
typedef struct {
    u16 id;         /* +0x00 object id */
    u16 sub;        /* +0x02 sub id */
    s32 type;       /* +0x04 0..3 */
    s32 x;          /* +0x08 CLUT x in VRAM */
    s32 y;          /* +0x0C CLUT y */
    s32 count;      /* +0x10 colours (16 or 256) */
} TM2ClutDef;

typedef struct {
    s32 active;     /* +0x000 */
    s32 pos;        /* +0x004 */
    s32 pos2;       /* +0x008 */
    s32 tick;       /* +0x00C */
    s32 f10;        /* +0x010 */
    s32 rate;       /* +0x014 */
    s32 toggle;     /* +0x018 */
    s32 last;       /* +0x01C */
    TM2ClutDef *def; /* +0x020 */
    u16 cur[256];   /* +0x024 */
    u16 orig[256];  /* +0x224 */
} TM2ClutAnim;

#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern TM2ClutDef clutAnimDefs[TM2_MAX_CLUT_ANIM_DEFS];
extern TM2ClutAnim clutAnimRunning[TM2_MAX_CLUT_ANIMS];
extern s32 clutAnimLastIndex;

extern void gpuStoreImageLine(u16 *, s32, s32, s32);
extern void gpuLoadImageLine(u16 *, s32, s32, s32);
/* request 48 stage 3 CLUT_SHADOW (ONLINE_ROLLBACK.md 12): the animations' palette rows through RB_CLUT_LOAD /
 * RB_CLUT_STORE (portClutLoadLine / portClutStoreLine: the shadow in RAM, no upload while re-running); without the
 * define retail's gpuLoadImageLine / gpuStoreImageLine */
#include "semantic/port_rollback.h"

/* @80148660: clear all animations and definitions. */
void __attribute__((section(".text_80148660"))) clutAnimInit(void)
{
    s32 i;

    for (i = (TM2_MAX_CLUT_ANIMS - 1) * sizeof(TM2ClutAnim); i >= 0; i -= sizeof(TM2ClutAnim)) {
        *(s32 *)((u8 *)clutAnimRunning + i) = 0;
    }
    clutAnimLastIndex = -1;
}

/* @80149018: allocate a new definition for (id, sub); NULL if it exists or the table is full. */
TM2ClutDef * __attribute__((section(".text_80149018"))) clutAnimNewDef(s32 id, s32 sub)
{
    s32 i;

    for (i = 0; i <= clutAnimLastIndex; i++) {
        if (clutAnimDefs[i].id == id && clutAnimDefs[i].sub == sub) {
            return 0;
        }
    }
    clutAnimLastIndex++;
    if (clutAnimLastIndex < TM2_MAX_CLUT_ANIM_DEFS) {
        return &clutAnimDefs[clutAnimLastIndex];
    }
    return 0;
}

/* @801490B0: find the definition for (id, sub). */
TM2ClutDef * __attribute__((section(".text_801490B0"))) clutAnimFindDef(s32 id, s32 sub)
{
    s32 i;

    for (i = 0; i <= clutAnimLastIndex; i++) {
        if (clutAnimDefs[i].id == id && clutAnimDefs[i].sub == sub) {
            return &clutAnimDefs[i];
        }
    }
    return 0;
}

/* @80149120: find the running animation of (id, sub). */
TM2ClutAnim * __attribute__((section(".text_80149120"))) clutAnimFindRunning(s32 id, s32 sub)
{
    s32 i;

    for (i = 0; i < TM2_MAX_CLUT_ANIMS; i++) {
        if (clutAnimRunning[i].active != 0 && clutAnimRunning[i].def->id == id && clutAnimRunning[i].def->sub == sub) {
            return &clutAnimRunning[i];
        }
    }
    return 0;
}

/* @801491A0: first free animation slot. */
TM2ClutAnim * __attribute__((section(".text_801491A0"))) clutAnimFreeSlot(void)
{
    s32 i;

    for (i = 0; i < TM2_MAX_CLUT_ANIMS; i++) {
        if (clutAnimRunning[i].active == 0) {
            return &clutAnimRunning[i];
        }
    }
    return 0;
}

/* @8014868C: start a looping palette animation of (id, sub), rate = ticks per step. */
void __attribute__((section(".text_8014868C"))) clutAnimStartSweep(s32 id, s32 sub, s32 rate)
{
    TM2ClutDef *def;
    TM2ClutAnim *a;

    def = clutAnimFindDef(id, sub);
    if (def != 0) {
        a = clutAnimFreeSlot();
        if (a != 0) {
            a->active = 1;
            a->tick = 0;
            a->pos = -1;
            a->def = def;
            a->pos2 = -1;
            a->rate = rate;
            if (def->count == 256) {
                a->f10 = 32;
            } else {
                a->f10 = 8;
            }
            RB_CLUT_STORE(a->cur, def->x, def->y, def->count);
            RB_CLUT_STORE(a->orig, def->x, def->y, def->count);
        }
    }
}

/* @80148740: start a fade-in animation of (id, sub) over 'steps' frames. */
void __attribute__((section(".text_80148740"))) clutAnimStartFadeOut(s32 id, s32 sub, s32 steps)
{
    TM2ClutDef *def;
    TM2ClutAnim *a;
    s32 i;
    s32 done;

    def = clutAnimFindDef(id, sub);
    if (def != 0) {
        a = clutAnimFreeSlot();
        if (a != 0) {
            a->active = 1;
            a->tick = 0;
            a->pos = 0;
            a->def = def;
            RB_CLUT_STORE(a->cur, def->x, def->y, def->count);
            RB_CLUT_STORE(a->orig, def->x, def->y, def->count);
            a->last = def->count - 1;
            i = 0;
            done = 0;
            if (i < def->count) {
                do {
                    if (a->cur[i] == 0) {
                        a->last = i - 1;
                        done = 1;
                    }
                    i++;
                } while (i < def->count && !done);
            }
            a->rate = a->last / steps + 1;
        }
    }
}

/* @80148868: make the running animation of (id, sub) fade back in. */
void __attribute__((section(".text_80148868"))) clutAnimFadeIn(s32 id, s32 sub)
{
    TM2ClutAnim *a;

    a = clutAnimFindRunning(id, sub);
    if (a != 0) {
        a->tick = 2;
        a->pos = 0;
    }
}

/* @8014889C: start a cycling palette animation of (id, sub). */
void __attribute__((section(".text_8014889C"))) clutAnimStartCycle(s32 id, s32 sub, s32 rate)
{
    TM2ClutDef *def;
    TM2ClutAnim *a;
    s32 i;
    s32 done;

    def = clutAnimFindDef(id, sub);
    if (def != 0) {
        a = clutAnimFreeSlot();
        if (a != 0) {
            a->active = 1;
            a->tick = 0;
            a->pos = -1;
            a->rate = rate;
            a->def = def;
            a->toggle = 0;
            RB_CLUT_STORE(a->cur, def->x, def->y, def->count);
            RB_CLUT_STORE(a->orig, def->x, def->y, def->count);
            i = 0;
            done = 0;
            if (def->type == 3) {
                a->last = 7;
                return;
            }
            a->last = def->count - 1;
            if (i < def->count) {
                do {
                    if (a->cur[i] == 0) {
                        a->last = i - 1;
                        done = 1;
                    }
                    i++;
                } while (i < def->count && !done);
            }
        }
    }
}

typedef struct {
    u8 kind;        /* +0x00 0 = object */
    u8 pad1;
    u16 id;         /* +0x02 */
    s32 f4;         /* +0x04 */
    s32 f8;
    u8 *data;       /* +0x0C */
    s32 f10;
    u16 sub;        /* +0x14 */
} TM2ClutObj;

extern char strClutAnimNotObjectFmt[];
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_CAR_CLUTS)
#include "semantic/car_groups.h"   /* portCarLoadKey */
#include "semantic/tex_banks.h"    /* request 32 step 4; request 33-1 (car_cluts.h) */
#endif

/* @801489B4: register a palette-animation definition for a loaded object. */
void __attribute__((section(".text_801489B4"))) clutAnimRegister(TM2ClutObj *obj, s32 type)
{
    TM2ClutObj *o;
    TM2ClutDef *def;
    s32 f4;
    u8 *d;
    u16 *p;
    u16 clut;
    u16 mode;
    u8 unused_buf[16]; /* retail frame has 16 unused bytes */
#ifdef TM2_PORT_TEX_SHARED
    s32 id;
#endif

    if (obj->kind == 0) {
        o = obj;
        goto ok;
    }
    printf(strClutAnimNotObjectFmt, obj->kind);
    return;
ok:
#ifdef TM2_PORT_TEX_SHARED
    /* request 32 step 4 (TEXTURE_BANKS.md 4.1, "As built"): while banks are live a car's +8 fade (type 2) is
       one definition per car key, id key + 8 (= type + 8 for the first car of a type, as retail), so a
       second car of a type gets its own instead of being refused; the same while request 33-1's CLUT copies
       are live (CAR_GROUPS.md 13: the primitive holds the car's own copy) */
    id = o->id;
#ifdef TM2_PORT_CAR_CLUTS
    if (type == 2 && (portTexBanks.live || portCarCluts.live) && portCarLoadKey() != 0)
#else
    if (type == 2 && portTexBanks.live && portCarLoadKey() != 0)
#endif
        id = portCarLoadKey() + 8;
    def = clutAnimNewDef(id, o->sub);
#else
    def = clutAnimNewDef(o->id, o->sub);
#endif
    if (def != 0) {
        f4 = o->f4; /* read but unused in retail */
        d = o->data;
        p = (u16 *)(d + ((d[15] >> 2) & 0x1C));
        p += 9;
        clut = *p++;
        p++;
        mode = *p;
        def->x = (clut & 0x3F) << 4;
        def->y = clut >> 6;
#ifdef TM2_PORT_TEXTURE_BANKS
        /* the CLUT's bank: the textured primitive's tpage bits 9..15 (modelRemapTextures), carried in
           def->y bits 9..15 to gpuStoreImageLine / gpuLoadImageLine; 0 for an untextured primitive
           (Outlaw 2, Mr. Grimm, Thumper: retail's junk row stays in bank 0) and for the arena */
        if (portTexBanks.live && (d[0] & 0x7F) != 0)
            def->y |= mode & 0xFE00;
#endif
        def->count = (mode >> 7) & 3;
        if (def->count == 0) {
            def->count = 16;
        }
        if (def->count == 1) {
            def->count = 256;
        }
#ifdef TM2_PORT_TEX_SHARED
        def->id = id;
#else
        def->id = o->id;
#endif
        def->sub = o->sub;
        def->type = type;
        if (def->type == 0) {
            clutAnimStartCycle(def->id, def->sub, 4);
        }
        if (def->type == 3) {
            clutAnimStartCycle(def->id, def->sub, 2);
        }
        if (def->type == 2) {
            def->sub = 0;
        }
    }
}

void clutAnimStepSweep(TM2ClutAnim *a);
void clutAnimStepFade(TM2ClutAnim *a);
void clutAnimStepCycle(TM2ClutAnim *a);

/* @80148B1C: per-frame update of all running palette animations. */
void __attribute__((section(".text_80148B1C"))) clutAnimUpdate(void)
{
    s32 i;
    TM2ClutAnim *a;
    TM2ClutDef *def;

    for (i = 0; i < TM2_MAX_CLUT_ANIMS; i++) {
        a = &clutAnimRunning[i];
        if (a->active != 0) {
            def = a->def;
            if (def->type == 0 || def->type == 3) {
                clutAnimStepCycle(a);
            } else if (def->type == 1) {
                clutAnimStepSweep(a);
            } else if (def->type == 2) {
                clutAnimStepFade(a);
            }
        }
    }
}

/* @80148BEC: type 1 step: sweep a white (0x8000) band through the palette. */
void __attribute__((section(".text_80148BEC"))) clutAnimStepSweep(TM2ClutAnim *a)
{
    TM2ClutDef *def;
    s32 i;

    a->tick++;
    if (a->tick > a->rate) {
        a->tick = 0;
        /* pos += 1; retail loads pos, then def, then stores and re-reads pos */
        i = a->pos;
        def = a->def;
        i = a->pos = i + 1;
        if (a->pos >= def->count && a->pos2 >= def->count) {
            a->active = 0;
        } else if (a->pos < a->f10) {
            a->cur[a->pos] = 0x8000;
        } else if (a->pos < a->def->count - 2) {
            a->pos2++;
            a->cur[a->pos] = 0x8000;
            a->cur[a->pos2] = 0;
        } else if (a->pos2 < a->def->count) {
            a->pos2++;
            a->cur[a->pos2] = 0;
        }
        RB_CLUT_LOAD(a->cur, a->def->x, a->def->y, a->def->count);
        a->toggle = (a->toggle == 0);
    }
}

/* @80148D7C: type 2 step: fade out (tick 0) / restore (tick 2) 'rate' colours per frame. */
void __attribute__((section(".text_80148D7C"))) clutAnimStepFade(TM2ClutAnim *a)
{
    s32 i;

    if (a->tick == 0) {
        for (i = 0; i < a->rate; i++) {
            if (a->pos < a->last) {
                a->cur[a->pos] = 0;
                a->pos++;
            } else {
                a->tick = 1;
            }
        }
        if (a->tick == 1) {
            a->pos = 0;
        }
    } else if (a->tick == 2) {
        for (i = 0; i < a->rate; i++) {
            if (a->pos < a->last) {
                a->cur[a->pos] = a->orig[a->pos];
                a->pos++;
            } else {
                a->active = 0;
            }
        }
    } else {
        return;
    }
    RB_CLUT_LOAD(a->cur, a->def->x, a->def->y, a->def->count);
}

/* @80148ED8: types 0/3 step: rotate the palette by one entry, ping-ponging the two buffers. */
void __attribute__((section(".text_80148ED8"))) clutAnimStepCycle(TM2ClutAnim *a)
{
    s32 i;

    a->tick++;
    if (a->tick > a->rate) {
        if (a->toggle != 0) {
            for (i = 0; i < a->last; i++) {
                a->orig[i + 1] = a->cur[i];
            }
            a->orig[0] = a->cur[a->last];
            RB_CLUT_LOAD(a->orig, a->def->x, a->def->y, a->def->count);
        } else {
            for (i = 0; i < a->last; i++) {
                a->cur[i + 1] = a->orig[i];
            }
            a->cur[0] = a->orig[a->last];
            RB_CLUT_LOAD(a->cur, a->def->x, a->def->y, a->def->count);
        }
        a->toggle = (a->toggle == 0);
        a->tick = 0;
    }
}

/* ---- Object (coordinate system) pool ------------------------------------
 * 128 objects of 0x128 bytes at csObjectPool (csPoolCount used). csRootObject is
 * the root object, csSharedObject a second shared object, csDrawListHead the head of
 * the drawn-object list (linked through next). */
typedef struct { s16 m[3][3]; s32 t[3]; } TM2Mat;

typedef struct TM2Obj {
    s32 f0;                 /* +0x000 */
    u8 *light;              /* +0x004 lighting environment (lightGetEnv) */
    s32 f8;                 /* +0x008 */
    s32 fC;                 /* +0x00C */
    struct TM2Obj *parent;  /* +0x010 */
    s32 wpos[3];            /* +0x014 world position */
    TM2Mat m20;             /* +0x020 */
    TM2Mat m40;             /* +0x040 */
    TM2Mat m60;             /* +0x060 */
    TM2Mat rotm;            /* +0x080 */
    s16 rot[3];             /* +0x0A0 */
    s16 padA6;
    s32 pos[3];             /* +0x0A8 local position */
    u8 padB4[0x118 - 0xB4];
    s32 f118;               /* +0x118 */
    u8 *model;              /* +0x11C */
    struct TM2Obj *next;    /* +0x120 */
    s32 visible;            /* +0x124 */
} TM2Obj;

#include "semantic/players.h"
extern TM2Obj csObjectPool[TM2_MAX_COORD_SYSTEMS];   /* 128 (256 with TM2_PORT_CARS=16) */
extern s32 csPoolCount;
extern TM2Obj *csRootObject;
extern TM2Obj *csSharedObject;
extern TM2Obj *csDrawListHead;
extern TM2Mat csIdentityMatrix;
extern char strTooManyCoordSystemsFmt[];
#ifdef TM2_PORT_CAR_DROPIN
#include "semantic/car_dropin.h"   /* PC port request 33: csAlloc's free list (portCarUnload, portCsTake) */
#endif

extern void *memcpy(void *, const void *, u32);
extern u8 *lightGetEnv(s32);
extern void mathMulTransVec(void *, s32 *, s32 *);
extern void mathMulMatVec(void *, s32 *, s32 *);
extern void *TransposeMatrix(void *, void *);
extern void SetRotMatrix(void *);
extern void SetLightMatrix(void *);
extern void SetColorMatrix(void *);
extern void SetBackColor(s32, s32, s32);
extern void *MulRotMatrix0(void *, void *);
extern s32 *viewGetEyeTrans(s32);
extern void *viewGetViewMatrix(s32);
extern void mathMulMatVec2(void *, s32 *, s32 *);

s32 csSphereVisible(TM2Obj *obj, s32 *pos);

/* @801491E8 csUpdMat: update an object's matrices from its parent; out = final light/rotation matrix. */
void __attribute__((section(".text_801491E8"))) csUpdMat(TM2Obj *obj, void *out, s32 force)
{
    TM2Mat tmp;
    s32 v[3];

    v[0] = obj->parent->wpos[0] + obj->pos[0];
    v[1] = obj->parent->wpos[1] + obj->pos[1];
    v[2] = obj->parent->wpos[2] + obj->pos[2];
    obj->visible = csSphereVisible(obj, v);
    if (force != 0) {
        obj->visible = 1;
    }
    if (obj->visible != 0) {
        mathMulMatVec(&obj->rotm, v, obj->wpos);
        TransposeMatrix(&obj->rotm, &tmp);
        SetRotMatrix(&obj->parent->m20);
        MulRotMatrix0(&tmp, &obj->m20);
        if (force != 0) {
            SetRotMatrix(&obj->rotm);
            MulRotMatrix0(&obj->parent->m40, &obj->m40);
            SetRotMatrix(&obj->rotm);
            MulRotMatrix0(&obj->parent->m60, &obj->m60);
        }
        TransposeMatrix(&obj->rotm, &tmp);
        SetRotMatrix(obj->parent->light + 0x2C);
        MulRotMatrix0(&tmp, out);
    }
}

/* @80149344: bounding-sphere visibility test of an object's model at world position pos. */
s32 __attribute__((section(".text_80149344"))) csSphereVisible(TM2Obj *obj, s32 *pos)
{
    s32 *p2 = pos;
    u8 *model;
    s32 r;
    s32 v[3];

    model = obj->model;
    if (model[0] == 1) {
        if (model[21] & 4) {
            return 1;
        }
        r = *(s32 *)(model + 16);
        mathMulTransVec(&obj->parent->m40, pos, v);
        r = -r;
        if (v[0] < r) {
            return 0;
        }
        if (v[1] < r) {
            return 0;
        }
        if (v[2] < r) {
            return 0;
        }
        mathMulTransVec(&obj->parent->m60, p2, v);
        if (v[0] < r) {
            return 0;
        }
        if (v[1] < r) {
            return 0;
        }
    }
    return 1;
}

/* @80149438: head of the drawn-object list. */
TM2Obj * __attribute__((section(".text_80149438"))) csListHead(void)
{
    return csDrawListHead;
}

/* @80149444: push an object onto the drawn-object list. */
void __attribute__((section(".text_80149444"))) csListAdd(TM2Obj *obj)
{
    obj->next = csDrawListHead;
    csDrawListHead = obj;
}

/* @80149458: reset an object (identity matrices, zero position/rotation, parent = root). */
void __attribute__((section(".text_80149458"))) csReset(TM2Obj *obj)
{
    obj->rotm = csIdentityMatrix;
    obj->m20 = csIdentityMatrix;
    obj->rot[0] = 0;
    obj->rot[1] = 0;
    obj->rot[2] = 0;
    obj->pos[0] = 0;
    obj->pos[1] = 0;
    obj->pos[2] = 0;
    obj->wpos[0] = 0;
    obj->wpos[1] = 0;
    obj->wpos[2] = 0;
    obj->rotm.t[0] = 0;
    obj->rotm.t[1] = 0;
    obj->rotm.t[2] = 0;
    obj->m20.t[0] = 0;
    obj->m20.t[1] = 0;
    obj->m20.t[2] = 0;
    obj->m40.t[0] = 0;
    obj->m40.t[1] = 0;
    obj->m40.t[2] = 0;
    obj->m60.t[0] = 0;
    obj->m60.t[1] = 0;
    obj->m60.t[2] = 0;
    obj->parent = csRootObject;
    obj->model = 0;
    obj->f0 = 0;
    obj->fC = 1;
    obj->f118 = 0;
    obj->f8 = 1;
    obj->next = 0;
    obj->light = lightGetEnv(0);
}

/* @8014952C: allocate and reset an object from the pool. */
TM2Obj * __attribute__((section(".text_8014952C"))) csAlloc(void)
{
    TM2Obj *obj;

#ifdef TM2_PORT_CAR_DROPIN
    /* PC port request 33 (CAR_GROUPS.md 14.7): an object a car unload gave back first (portCarUnloadGroup); the
       list is empty until a port call unloads a car, so this is retail's bump allocation */
    if (portCarUnload.cs_count != 0 && (obj = (TM2Obj *)portCsTake()) != 0) {
        csReset(obj);
        return obj;
    }
#endif
    if (csPoolCount < TM2_MAX_COORD_SYSTEMS) {
        obj = &csObjectPool[csPoolCount];
        csReset(obj);
        csPoolCount++;
        return obj;
    }
    printf(strTooManyCoordSystemsFmt, csPoolCount);
    return 0;
}

/* @801495A8: set an object's model. */
void __attribute__((section(".text_801495A8"))) csSetModel(TM2Obj *obj, u8 *model)
{
    obj->model = model;
}

/* @801495C0: reset the object pool and create the root object. */
void __attribute__((section(".text_801495C0"))) csPoolInit(void)
{
    csPoolCount = 0;
    csRootObject = csAlloc();
    csRootObject->f118 = 8;
    csDrawListHead = 0;
    csSharedObject = 0;
}

/* @801495F8: the root object. */
TM2Obj * __attribute__((section(".text_801495F8"))) csGetRoot(void)
{
    return csRootObject;
}

/* @80149604: set an object's flag byte. */
void __attribute__((section(".text_80149604"))) csSetDrawEnable(TM2Obj *obj, u8 v)
{
    obj->f8 = v;
}

/* @80149614: create the shared object csSharedObject with the given model (once). */
void __attribute__((section(".text_80149614"))) csCreateShared(u8 *model)
{
    if (csSharedObject == 0) {
        csSharedObject = csAlloc();
        csSharedObject->model = model;
    }
}

/* @80149650: the shared object. */
TM2Obj * __attribute__((section(".text_80149650"))) csGetShared(void)
{
    return csSharedObject;
}

/* @8014965C: load the GTE camera/light state for player view 'view'. */
void __attribute__((section(".text_8014965C"))) viewLoadGte(s32 view)
{
    TM2Obj *root;
    s32 *eye;
    void *m;
    s32 *unset;

    root = csGetRoot();
    eye = viewGetEyeTrans(view);
    m = viewGetViewMatrix(view);
    mathMulMatVec2(m, eye, unset);
    SetRotMatrix(m);
    SetLightMatrix(root->light + 0x2C);
    SetColorMatrix(root->light + 0xC);
    SetBackColor(((s32 *)root->light)[0], ((s32 *)root->light)[1], ((s32 *)root->light)[2]);
}

/* ---- Pad start/stop ------------------------------------------------------ */
extern u8 padRecvBufferPort1[34];
extern u8 padRecvBufferPort2[34];
extern s32 padBiosBuf1Size;
extern s32 padBiosBuf2Size;
extern int InitPAD2(u8 *, int, u8 *, int);
extern void StartPAD2(void);
extern void StopPAD2(void);
extern int ChangeClearPAD(int);

/* @8014A2A4: register the two BIOS pad buffers and start pad polling. */
void __attribute__((section(".text_8014A2A4"))) padStart(void)
{
    InitPAD2(padRecvBufferPort1, padBiosBuf1Size, padRecvBufferPort2, padBiosBuf2Size);
    StartPAD2();
    ChangeClearPAD(0);
}

/* @8014A2EC: stop pad polling. */
void __attribute__((section(".text_8014A2EC"))) padStop(void)
{
    StopPAD2();
}

/* ---- Deferred draw-call stack ----------------------------------------------
 * 28-byte records; the first 35 live in drawRecStackScratch's buffer, the rest in
 * drawRecStackOverflow's (pre-offset so the same index works). dbscanStackDepth = count. */
typedef struct {
    s32 a;
    s32 b;
    s32 v[3];
    s32 c;
    s32 d;
} TM2DrawRec;

extern s32 dbscanStackDepth;
extern TM2DrawRec *drawRecStackScratch;
extern TM2DrawRec *drawRecStackOverflow;
extern TM2Mat *dbscanMatrixStack;
extern char strDbScanStackOverflow[];

/* @8014AAA0: set the overflow record buffer (and a second pointer). */
void __attribute__((section(".text_8014AAA0"))) dbscanInitStacks(TM2Mat *p, TM2DrawRec *buf)
{
    dbscanMatrixStack = p;
    drawRecStackOverflow = buf;
}

/* Fill one deferred record (inlined into each branch of dbscanPush). */
static inline void put_rec(TM2DrawRec *r, s32 a, s32 b, s32 *v, s32 c, s32 d)
{
    r->a = a;
    r->b = b;
    r->v[0] = v[0];
    r->v[1] = v[1];
    r->v[2] = v[2];
    r->c = c;
    r->d = d;
}

/* @8014AAB0: push a record. Each branch fills its own record (the compiler
 * merges the identical tails). */
void __attribute__((section(".text_8014AAB0"))) dbscanPush(s32 a, s32 b, s32 *v, s32 c, s32 d)
{
    u32 n;
    s32 m;

    n = dbscanStackDepth;
    if (n < 35) {
        put_rec((TM2DrawRec *)(n * sizeof(TM2DrawRec) + (u32)drawRecStackScratch), a, b, v, c, d);
    } else {
        put_rec((TM2DrawRec *)(n * sizeof(TM2DrawRec) + (u32)drawRecStackOverflow), a, b, v, c, d);
    }
    m = *(s32 *)&dbscanStackDepth;
    if (m < 320) {
        dbscanStackDepth = m + 1;
    } else {
        printf(strDbScanStackOverflow);
    }
}

/* Copy one deferred record out (inlined into each branch of dbscanPop). */
static inline void get_rec(TM2DrawRec *out, TM2DrawRec *r)
{
    out->a = r->a;
    out->b = r->b;
    out->v[0] = r->v[0];
    out->v[1] = r->v[1];
    out->v[2] = r->v[2];
    out->c = r->c;
    out->d = r->d;
}

/* @8014AB5C: pop a record into *out; returns the remaining count. */
s32 __attribute__((section(".text_8014AB5C"))) dbscanPop(TM2DrawRec *out)
{
    s32 n;

    n = dbscanStackDepth - 1;
    dbscanStackDepth = n;
    if (n >= 0) {
        if ((u32)n < 35) {
            get_rec(out, (TM2DrawRec *)(n * sizeof(TM2DrawRec) + (u32)drawRecStackScratch));
        } else {
            get_rec(out, (TM2DrawRec *)(n * sizeof(TM2DrawRec) + (u32)drawRecStackOverflow));
        }
    }
    return dbscanStackDepth;
}

/* ---- Shadow / effect object slots ------------------------------------------ */
typedef struct {
    s32 vel[3];             /* +0x00 */
    s16 spin[3];            /* +0x0C */
    s16 pad12;
    TM2Obj *obj;            /* +0x14 */
    s32 active;             /* +0x18 */
    s32 age;                /* +0x1C */
    s32 life;               /* +0x20 */
} TM2Shadow;

typedef struct {
    s32 active;             /* +0x000 */
    s32 f4;                 /* +0x004 */
    s32 age;                /* +0x008 */
    s32 end;                /* +0x00C */
    s32 len;                /* +0x010 */
    s32 vel[4][3];          /* +0x014 */
    s32 pos[4][3];          /* +0x044 */
    s32 wpos[4][3];         /* +0x074 */
    s32 dvel[3];            /* +0x0A4 */
    s16 spin2[3];           /* +0x0B0 */
    s16 padB6;
    s16 rot[3];             /* +0x0B8 */
    s16 padBE;
    s16 spin1[3];           /* +0x0C0 */
    s16 padC6;
    TM2Mat m;               /* +0x0C8 */
    s32 padE8;
    s32 origin[3];          /* +0x0EC */
    u8 padF8[0x228 - 0xF8];
} TM2FxSlot;

extern TM2Shadow debrisChunks[8];
extern TM2FxSlot debrisBursts[4];
extern TM2Obj *debrisRootObject;
extern TM2Obj *debrisBurstObject;

extern void fxDebrisDraw(s32, TM2FxSlot *);
void debrisUpdateChunks(void);
void debrisUpdateBursts(void);

/* @8014B328: create the 8 shadow objects and the two effect objects; clear all slots. */
void __attribute__((section(".text_8014B328"))) debrisInit(void)
{
    s32 i;
    s32 j;

    for (i = 0; i < 8; i++) {
        debrisChunks[i].obj = csAlloc();
        csListAdd(debrisChunks[i].obj);
        csSetDrawEnable(debrisChunks[i].obj, 0);
        debrisChunks[i].active = 0;
    }
    for (j = 3 * sizeof(TM2FxSlot); j >= 0; j -= sizeof(TM2FxSlot)) {
        *(s32 *)((u8 *)debrisBursts + j) = 0;
    }
    debrisRootObject = csAlloc();
    debrisBurstObject = csAlloc();
    debrisBurstObject->parent = debrisRootObject;
}

/* @8014B3E0: deactivate all shadow and effect slots. */
void __attribute__((section(".text_8014B3E0"))) debrisClear(void)
{
    s32 i;
    s32 j;

    for (i = 0; i < 8; i++) {
        debrisChunks[i].active = 0;
        csSetDrawEnable(debrisChunks[i].obj, 0);
    }
    for (j = 3 * sizeof(TM2FxSlot); j >= 0; j -= sizeof(TM2FxSlot)) {
        *(s32 *)((u8 *)debrisBursts + j) = 0;
    }
}

/* @8014B45C: per-frame update of shadows and effects. */
void __attribute__((section(".text_8014B45C"))) debrisUpdate(void)
{
    debrisUpdateChunks();
    debrisUpdateBursts();
}

/* @8014B484: draw the active effect slots. */
void __attribute__((section(".text_8014B484"))) debrisDraw(s32 arg)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (debrisBursts[i].active != 0) {
            fxDebrisDraw(arg, &debrisBursts[i]);
        }
    }
}

/* @8014B8F8: first free shadow slot. */
TM2Shadow * __attribute__((section(".text_8014B8F8"))) debrisFreeChunk(void)
{
    s32 i;

    for (i = 0; i < 8; i++) {
        if (debrisChunks[i].active == 0) {
            return &debrisChunks[i];
        }
    }
    return 0;
}

/* @8014BEF4: first free effect slot. */
TM2FxSlot * __attribute__((section(".text_8014BEF4"))) debrisFreeBurst(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (debrisBursts[i].active == 0) {
            return &debrisBursts[i];
        }
    }
    return 0;
}

extern void debrisStartBurst(); /* defined below (unprototyped: args promote unchanged) */

/* @8014B820: place the effect object at pos (relative to a type-4 model's origin) and run it. */
void __attribute__((section(".text_8014B820"))) debrisBurstAt(u8 *model, s32 *pos, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8)
{
    if (model != 0) {
        if (model[0] == 4) {
            debrisBurstObject->pos[0] = pos[0] - *(s32 *)(model + 4);
            debrisBurstObject->pos[1] = pos[1] - *(s32 *)(model + 8);
            debrisBurstObject->pos[2] = pos[2] - *(s32 *)(model + 12);
        } else {
            debrisBurstObject->pos[0] = pos[0];
            debrisBurstObject->pos[1] = pos[1];
            debrisBurstObject->pos[2] = pos[2];
        }
        debrisBurstObject->model = model;
        debrisStartBurst(debrisBurstObject, a2, a3, a5, a6, a7, a8, a4);
    }
}

extern void *RotMatrixYXZ(void *, void *);

/* @8014B940: move the active shadow/debris objects (spin, velocity, gravity after 10 frames, expiry). */
void __attribute__((section(".text_8014B940"))) debrisUpdateChunks(void)
{
    s32 i;
    TM2Shadow *s;

    for (i = 0; i < 8; i++) {
        s = &debrisChunks[i];
        if (s->active != 0) {
            s->obj->rot[0] += debrisChunks[i].spin[0];
            s->obj->rot[1] += debrisChunks[i].spin[1];
            s->obj->rot[2] += debrisChunks[i].spin[2];
            RotMatrixYXZ(s->obj->rot, &debrisChunks[i].obj->rotm);
            s->obj->pos[0] += s->vel[0];
            s->obj->pos[1] += s->vel[1];
            s->obj->pos[2] += s->vel[2];
            if (s->age >= 11) {
                s->vel[2] = s->vel[2] - 1;
            }
            if (s->age > s->life) {
                csSetDrawEnable(s->obj, 0);
                s->active = 0;
            }
            s->age++;
        }
    }
}

/* @8014BF3C: animate the active effect slots (four pieces each). */
void __attribute__((section(".text_8014BF3C"))) debrisUpdateBursts(void)
{
    s32 i;
    s32 j;
    TM2FxSlot *f;

    for (i = 0; i < 4; i++) {
        f = &debrisBursts[i];
        if (f->active != 0) {
            if (f->age < f->len) {
                for (j = 0; j < 4; j++) {
                    f->pos[j][0] += f->dvel[0];
                    f->pos[j][1] += f->dvel[1];
                    f->pos[j][2] += f->dvel[2];
                }
                f->rot[0] += f->spin1[0];
                f->rot[1] += f->spin1[1];
                f->rot[2] += f->spin1[2];
                RotMatrixYXZ(f->rot, &f->m);
            } else if (f->age != f->len) {
                for (j = 0; j < 4; j++) {
                    f->pos[j][0] += f->vel[j][0];
                    f->pos[j][1] += f->vel[j][1];
                    f->pos[j][2] += f->vel[j][2];
                    f->vel[j][2] = f->vel[j][2] - 1;
                    f->wpos[j][0] = f->pos[j][0] + f->origin[0];
                    f->wpos[j][1] = f->pos[j][1] + f->origin[1];
                    f->wpos[j][2] = f->pos[j][2] + f->origin[2];
                    f->rot[0] += f->spin2[0];
                    f->rot[1] += f->spin2[1];
                    f->rot[2] += f->spin2[2];
                    RotMatrixYXZ(f->rot, &f->m);
                }
                if (f->age > debrisBursts[i].end) {
                    f->active = 0;
                    f->f4 = 0;
                }
            }
            f->age++;
        }
    }
}

extern int rand(void);
extern int VectorNormal(s32 *, s32 *);
extern void renderDrawObjectUnculled(TM2Obj *);
extern TM2FxSlot *lightCurrentSet;

/* @8014B504: break a model into flying parts (one shadow slot per type-4 part) plus a burst effect. */
void __attribute__((section(".text_8014B504"))) debrisBreakModel(TM2Obj *obj, u8 *model, s32 *target, s16 *dir, s32 speed, s32 life)
{
    s32 k;
    s32 i;
    s32 r;
    s32 t;
    s32 u;
    TM2Shadow *s;
    u8 *part;
    TM2Obj *saved;
    s32 d[4];
    s32 n[4];
    s32 v[4];
    s16 zero[4];

    k = 0;
    if (model != 0) {
        for (i = 0; i < model[20]; i++) {
            s = debrisFreeChunk();
            if (s != 0) {
                part = ((u8 **)(model + 24))[i];
                if (part[0] == 4) {
                    s->active = 1;
                    s->life = life;
                    csSetModel(s->obj, *(u8 **)(part + 20));
                    csSetDrawEnable(s->obj, 1);
                    mathMulTransVec(&obj->rotm, (s32 *)(part + 4), v);
                    s->obj->pos[0] = obj->pos[0] + v[0];
                    s->obj->pos[1] = obj->pos[1] + v[1];
                    s->obj->pos[2] = obj->pos[2] + v[2];
                    s->obj->rot[0] = 0;
                    s->obj->rot[1] = 0;
                    s->obj->rot[2] = 0;
                    r = rand() & 0x3F;
                    if (r <= 0) {
                        s->vel[0] = (speed * (dir[0] + r)) >> 12;
                        s->vel[1] = (speed * (dir[1] + r)) >> 12;
                        s->vel[2] = speed + 5;
                    } else {
                        d[0] = target[0] - s->obj->pos[0];
                        d[1] = target[1] - s->obj->pos[1];
                        d[2] = target[2] - s->obj->pos[2];
                        VectorNormal(d, n);
                        t = n[0] * 3;
                        s->vel[0] = (speed * t) >> 12;
                        t = n[1] * 3;
                        s->vel[1] = (speed * t) >> 12;
                        u = k + 5;
                        s->vel[2] = speed + u;
                        s->vel[0] = s->vel[0] + k;
                        s->vel[1] = s->vel[1] + k;
                        k += 3;
                    }
                    s->spin[0] = r + 15;
                    s->spin[1] = (rand() & 0x3F) + 10;
                    s->spin[2] = (rand() & 0x3F) + 10;
                    s->age = 0;
                }
            }
        }
    }
    saved = obj->parent;
    obj->parent = debrisRootObject;
    zero[0] = 0;
    zero[1] = 0;
    zero[2] = 0;
    debrisStartBurst(obj, dir, zero, 0, speed, life, 1, 0);
    obj->parent = saved;
    obj->f8 = 0;
}

/* @8014BAD4: start a four-piece effect in a free slot. */
void __attribute__((section(".text_8014BAD4"))) debrisStartBurst(TM2Obj *obj, s16 *dir, s16 *spin, s32 len, s32 speed, s32 end, s32 randspin, s32 *dvel)
{
    TM2FxSlot *f;
    s32 j;
    s32 r;

    f = debrisFreeBurst();
    if (f != 0) {
        f->vel[0][0] = (speed * dir[0]) >> 12;
        f->vel[0][1] = (speed * dir[1]) >> 12;
        f->vel[0][2] = (rand() & 0x1F) + speed;
        f->vel[1][0] = (speed * dir[0]) >> 12;
        f->vel[1][1] = (speed * dir[1]) >> 12;
        f->vel[1][2] = (rand() & 0x1F) + 5;
        f->vel[2][0] = (speed * dir[0]) >> 12;
        f->vel[2][1] = (speed * dir[1]) >> 12;
        f->vel[2][2] = (rand() & 0xF) + speed;
        f->vel[3][0] = (speed * dir[0]) >> 12;
        f->vel[3][1] = (speed * dir[1]) >> 12;
        f->vel[3][2] = (rand() & 0x1F) + speed;
        f->vel[0][0] += rand() & 0x1F;
        f->vel[0][1] += rand() & 0x1F;
        f->vel[1][0] += rand() & 0x1F;
        f->vel[1][1] -= rand() & 0x1F;
        f->vel[2][0] -= rand() & 0x1F;
        f->vel[2][1] += rand() & 0x1F;
        f->vel[3][0] -= rand() & 0x1F;
        f->vel[3][1] -= rand() & 0x1F;
        for (j = 0; j < 4; j++) {
            f->pos[j][0] = 0;
            f->pos[j][1] = 0;
            f->pos[j][2] = 0;
        }
        if (obj != 0) {
            f->age = 0;
            f->end = end;
            lightCurrentSet = f;
            f->f4 = 0;
            renderDrawObjectUnculled(obj);
            f->active = 1;
        }
        f->rot[0] = 0;
        f->rot[1] = 0;
        f->rot[2] = 0;
        f->spin2[0] = 0;
        f->spin2[1] = 0;
        f->spin2[2] = 0;
        if (dvel != 0) {
            f->dvel[0] = dvel[0];
            f->dvel[1] = dvel[1];
            f->dvel[2] = dvel[2];
        } else {
            f->dvel[0] = 0;
            f->dvel[1] = 0;
            f->dvel[2] = 0;
        }
        if (randspin != 0) {
            r = rand() & 3;
            if (r == 0) {
                f->spin2[1] = (rand() & 0x1F) + 30;
            } else if (r == 1) {
                f->spin2[0] = (rand() & 0x1F) + 30;
            } else if (r == 2) {
                f->spin2[2] = (rand() & 0x1F) + 30;
            } else if (r == 3) {
                f->spin2[1] = (rand() & 0x3F) + 2;
            }
        }
        if (spin != 0) {
            f->spin1[0] = spin[0];
            f->spin1[1] = spin[1];
            f->spin1[2] = spin[2];
        } else {
            f->spin1[0] = 0;
            f->spin1[1] = 0;
            f->spin1[2] = 0;
        }
        f->len = len;
    }
}

typedef struct { s32 v[3]; } TM2Vec3;

extern TM2Vec3 dbscanZeroVec;     /* {0, 0, 0} */
extern TM2Mat dbscanIdentityMat;      /* identity */
extern char strDbScanNoMemStack[];      /* "Memory stack not set via dbscanInit, can't scan \n" */
extern char strBadHierarchyOpcodeFmt[];      /* "Bad hierarchy opcode %d \n" */
extern void meshSetPrimQuadrants(u8 *);
extern void animRegister(u8 *);

/* @8014ABF8 (dbscan): walk an object hierarchy depth-first using the record
 * stack, accumulating translations/rotations (matrix stack dbscanMatrixStack) and
 * calling cb(id, node, world position, user) for every node with an id.
 * 'level' is the coordinate scale (shift) wanted by the caller. */
void __attribute__((section(".text_8014ABF8"))) dbscanHierarchy(u8 *root, s32 level, void (*cb)(s32, u8 *, s32 *, s32))
{
    s32 lev;
    s32 w[3];
    s32 out[4];
    TM2Vec3 lpos;
    TM2DrawRec rec;
    TM2Vec3 start;
    TM2Mat ident;
    TM2Mat tm;
    s32 id;
    s32 sh;
    s32 n;

    start = dbscanZeroVec;
    ident = dbscanIdentityMat;
    if (drawRecStackOverflow == 0) {
        printf(strDbScanNoMemStack);
        return;
    }
    lev = level;
    dbscanStackDepth = 0;
    dbscanPush((s32)root, 0, start.v, level, 0);
    memcpy(dbscanMatrixStack, &ident, 0x20);
    while (dbscanPop(&rec) >= 0) {
        switch (((u8 *)rec.a)[0]) {
        case 0:
            {
            u8 *node;
            node = (u8 *)rec.a;
            meshSetPrimQuadrants(node);
            id = *(u16 *)(node + 2);
            }
            break;
        case 1:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            for (i = 0; i < node[20]; i++) {
                dbscanPush(((s32 *)(node + 24))[i], rec.b, rec.v, rec.c, (s32)node);
            }
            }
            break;
        case 2:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            for (i = 0; i < *(s32 *)(node + 16); i++) {
                dbscanPush(*(s32 *)(((u8 **)(node + 20))[i] + 8), rec.b, rec.v, rec.c, rec.d);
            }
            }
            break;
        case 3:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            if (node[10] == 1) {
                node[13] = 1;
            }
            if (node[18] == 1) {
                animRegister(node);
            }
            for (i = 0; i < node[11]; i++) {
                dbscanPush(((s32 *)(node + 32))[i], rec.b, rec.v, rec.c, rec.d);
            }
            }
            break;
        case 4:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            rec.v[0] += *(s32 *)(node + 4);
            rec.v[1] += *(s32 *)(node + 8);
            rec.v[2] += *(s32 *)(node + 12);
            for (i = 0; i < node[18]; i++) {
                dbscanPush(((s32 *)(node + 20))[i], rec.b, rec.v, rec.c, rec.d);
            }
            }
            break;
        case 5:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            rec.v[0] += *(s32 *)(node + 24);
            rec.v[1] += *(s32 *)(node + 28);
            rec.v[2] += *(s32 *)(node + 32);
            lpos.v[0] = rec.v[0];
            lpos.v[1] = rec.v[1];
            lpos.v[2] = rec.v[2];
            mathMulMatVec(node + 4, lpos.v, rec.v);
            TransposeMatrix(node + 4, &tm);
            SetRotMatrix(&dbscanMatrixStack[rec.b]);
            MulRotMatrix0(&tm, &dbscanMatrixStack[dbscanStackDepth]);
            for (i = 0; i < node[38]; i++) {
                dbscanPush(((s32 *)(node + 40))[i], dbscanStackDepth, rec.v, rec.c, rec.d);
            }
            }
            break;
        case 8:
        case 13:
            id = *(u16 *)((u8 *)rec.a + 2);
            break;
        case 9:
            {
            u8 *node;
            s32 i;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            for (i = 0; i < node[6]; i++) {
                dbscanPush(((s32 *)(node + 16))[i], rec.b, rec.v, rec.c, rec.d);
            }
            }
            break;
        case 11:
            {
            u8 *node;
            node = (u8 *)rec.a;
            id = *(u16 *)(node + 2);
            dbscanPush(*(s32 *)(node + 8), rec.b, rec.v, rec.c, rec.d);
            dbscanPush(*(s32 *)(node + 12), rec.b, rec.v, rec.c, rec.d);
            }
            break;
        case 12:
            {
            u8 *node;
            s32 i;
            s32 sh;
            s32 n;
            node = (u8 *)rec.a;
            sh = rec.c - *(u16 *)(node + 4);
            id = 0;
            if (sh < 0) {
                n = __builtin_abs(sh);
                rec.v[0] <<= n;
                rec.v[1] <<= n;
                rec.v[2] <<= n;
            } else if (sh > 0) {
                rec.v[0] >>= sh;
                rec.v[1] >>= sh;
                rec.v[2] >>= sh;
            }
            for (i = 0; i < *(u16 *)(node + 6); i++) {
                dbscanPush(((s32 *)(node + 8))[i], rec.b, rec.v, *(u16 *)(node + 4), rec.d);
            }
            }
            break;
        case 6:
            id = 0;
            break;
        default:
            id = 0;
            printf(strBadHierarchyOpcodeFmt, ((u8 *)rec.a)[0]);
            break;
        }
        if (id != 0) {
            sh = rec.c - lev;
            if (sh < 0) {
                w[0] = rec.v[0] << __builtin_abs(sh);
                w[1] = rec.v[1] << __builtin_abs(sh);
                w[2] = rec.v[2] << __builtin_abs(sh);
            } else if (sh > 0) {
                w[0] = rec.v[0] >> sh;
                w[1] = rec.v[1] >> sh;
                w[2] = rec.v[2] >> sh;
            } else {
                w[0] = rec.v[0];
                w[1] = rec.v[1];
                w[2] = rec.v[2];
            }
            if (((u8 *)rec.a)[0] == 5) {
                if (sh < 0) {
                    lpos.v[0] <<= __builtin_abs(sh);
                    lpos.v[1] <<= __builtin_abs(sh);
                    lpos.v[2] <<= __builtin_abs(sh);
                } else if (sh > 0) {
                    lpos.v[0] >>= sh;
                    lpos.v[1] >>= sh;
                    lpos.v[2] >>= sh;
                }
                mathMulMatVec(&dbscanMatrixStack[rec.b], lpos.v, out);
            } else {
                mathMulMatVec(&dbscanMatrixStack[rec.b], w, out);
            }
            cb(id, (u8 *)rec.a, out, rec.d);
        }
    }
}

/* ---- libspu S_IT.OBJ (Sony PsyQ SDK) ----------------------------------------
 * SPDX-License-Identifier: MIT
 * Adapted from sotn-decomp src/main/psxsdk/libspu/s_it.c. Compiles identically
 * with the engine recipe (no pointers; the global is lui/lw-addressed). */
extern s32 spuInTransfer; /* _spu_inTransfer */
#pragma tm2_absolute spuInTransfer

/* @801482BC _spu_getInTransfer: 1 while an SPU transfer is in progress. */
s32 __attribute__((section(".text_801482BC"))) sub_801482BC(void)
{
    return spuInTransfer == 0;
}
