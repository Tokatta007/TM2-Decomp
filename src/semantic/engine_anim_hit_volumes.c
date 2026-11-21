/* Library code 80150000..80152000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything in this unit is SingleTrac engine code (no Sony SDK functions):
 *   80150524..80150A74  hdp* hit detection (probe vs. hit volumes)
 *   80151378..8015166C  model draw / scene setup helpers
 *   801516D0..80151F90  texture-animation sequencer (type 3 timed frames, type 9 steps)
 * Recipe: psyq3.6, -fvolatile, extern_gp, gp_size 8 (same as the game). */

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* ---- hit detection ---------------------------------------------------- */

/* A probe: a world point plus a hit tag and a precision shift. */
typedef struct HdpProbe {
    void *target;       /* 0x00 */
    s32 x, y, z;        /* 0x04 */
    s16 tag, tag2;      /* 0x10 */
    s32 shift;          /* 0x14 */
} HdpProbe;

/* A distance band: hit when min_dist <= d^2 < max_dist (stored max first). */
typedef struct HdpBand {
    u32 max_dist;       /* 0x00 */
    u32 min_dist;       /* 0x04 */
    void *hit;          /* 0x08 */
} HdpBand;

/* A sphere-ish node: centre, number of bands, band list. */
typedef struct HdpNode {
    s32 pad00;
    s32 x, y, z;        /* 0x04 */
    s32 count;          /* 0x10 */
    HdpBand *band[1];   /* 0x14 */
} HdpNode;

/* One plane set: transformed by mathMulMatVec into three distances. */
typedef struct HdpPlanes {
    u8 in[20];          /* input for mathMulMatVec */
    s32 lim[3];         /* added to the three outputs; > 0 means inside */
} HdpPlanes;

/* A convex volume: 3..6 planes (two plane sets). */
typedef struct HdpVolume {
    u8 planes;          /* 0x00 */
    u8 pad01[3];
    HdpPlanes p[2];     /* 0x04, 0x24 */
} HdpVolume;

/* Volume group: all volumes must contain the point; then pick in/out hit. */
typedef struct HdpGroup {
    u8 pad00[6];
    u8 count;           /* 0x06 */
    u8 pad07;
    void *hit_in;       /* 0x08 */
    void *hit_out;      /* 0x0C */
    HdpVolume *vol[1];  /* 0x10 */
} HdpGroup;

/* Box test (layout used by hdpTestBox). */
typedef struct HdpBox {
    u8 pad00[4];
    u8 planes;          /* 0x04 */
    u8 pad05[11];
    HdpPlanes p[2];     /* 0x10, 0x30 */
} HdpBox;

typedef struct HdpResult {
    s32 hit;            /* 0x00 */
    s32 pad04[3];
    s32 code;           /* 0x10 */
} HdpResult;

typedef struct HdpHitTag {
    u8 pad00[10];
    s16 tag;            /* 0x0A */
    s16 tag2;           /* 0x0C */
} HdpHitTag;

typedef struct HdpState {
    s32 hit;            /* 0x00 */
    s32 pad04;
    s16 tag;            /* 0x08 */
    s16 tag2;           /* 0x0A */
    s32 pad0C;
    s32 code;           /* 0x10 */
} HdpState;

typedef struct HdpObject {
    u8 pad000[0x118];
    s32 value;          /* 0x118 */
    void *hdp;          /* 0x11C */
} HdpObject;

extern HdpState hdpQueryResult;

extern void hdpPush(void *hit, s32 *pos, s16 tag, s16 tag2, s32 shift);
extern void mathMulMatVec(void *in, void *pos, s32 *out);
extern HdpObject *csGetRoot(void);
extern void hdpTraverse(HdpObject *obj, void *probe, HdpState *state);

s32 hdpIsPtInVolume(HdpVolume *v, s32 *pos);

/* @80150524: find the distance band of a node containing the probe and report its hit. */
void __attribute__((section(".text_80150524"))) hdpTestSphereBands(HdpNode *node, HdpProbe *pr)
{
    s32 d[3];
    u32 dist;
    s32 i;
    s32 first;
    HdpBand *b;

    d[0] = node->x - pr->x;
    d[1] = node->y - pr->y;
    d[2] = node->z - pr->z;
    d[0] = d[0] >> pr->shift;
    d[1] = d[1] >> pr->shift;
    d[2] = d[2] >> pr->shift;
    dist = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    first = 1;
    for (i = 0; i < node->count && first; i++) {
        if (dist >= node->band[i]->min_dist && dist < node->band[i]->max_dist) {
            hdpPush(node->band[i]->hit, &pr->x, pr->tag, pr->tag2, pr->shift);
            first = 0;
        }
    }
}

/* @80150694: test a point against a box (3..6 planes); sets res->hit, optional consume. */
s32 __attribute__((section(".text_80150694"))) hdpTestBox(HdpBox *box, void *pos, HdpResult *res, s32 consume)
{
    s32 t[3];

    mathMulMatVec(box->p[0].in, pos, t);
    if (box->p[0].lim[0] + t[0] > 0 && box->p[0].lim[1] + t[1] > 0 && box->p[0].lim[2] + t[2] > 0) {
        if (box->planes < 4) {
            res->hit = 1;
        } else {
            mathMulMatVec(box->p[1].in, pos, t);
            if (box->p[1].lim[0] + t[0] > 0) {
                if (box->planes < 5) {
                    res->hit = 1;
                } else if (box->p[1].lim[1] + t[1] > 0) {
                    if (box->planes < 6) {
                        res->hit = 1;
                    } else if (box->p[1].lim[2] + t[2] > 0) {
                        res->hit = 1;
                    }
                }
            }
        }
    }
    if (res->hit == 1 && consume) {
        res->code = -2;
        res->hit = 0;
        return res->code;
    }
    return res->hit == 1;
}

/* @80150804: report the in- or out-hit of a volume group for the probe.
 * tag arrives as an int (callers pass a sign-extended s16) and tag2 is copied
 * to a local after the group load; the two identical arms of the pr/tag test
 * are merged by cross-jumping and the test is then deleted, so the code is a
 * single load. These shapes give retail's register assignment. */
void __attribute__((section(".text_80150804"))) hdpTestVolumeGroup(HdpProbe *pr, s32 tag, s16 tag2)
{
    HdpGroup *g;
    s32 i;
    s32 inside;
    s16 t2;

    if ((s32)pr != tag)
        g = (HdpGroup *)pr->target;
    else
        g = (HdpGroup *)pr->target;
    t2 = tag2;
    i = 0;
    inside = 1;
    while (inside && i < g->count) {
        if (hdpIsPtInVolume(g->vol[i], &pr->x))
            inside = 0;
        i++;
    }
    if (inside) {
        if (g->hit_in)
            hdpPush(g->hit_in, &pr->x, tag, t2, pr->shift);
    } else {
        if (g->hit_out)
            hdpPush(g->hit_out, &pr->x, tag, t2, pr->shift);
    }
}

/* @80150910: 1 if the point is inside the convex volume (3..6 planes). */
s32 __attribute__((section(".text_80150910"))) hdpIsPtInVolume(HdpVolume *v, s32 *pos)
{
    s32 t[3];
    s32 u[3];
    s32 r;

    mathMulMatVec(v->p[0].in, pos, t);
    r = 0;
    if (v->p[0].lim[0] + t[0] > 0 && v->p[0].lim[1] + t[1] > 0 && v->p[0].lim[2] + t[2] > 0) {
        if (v->planes < 4) {
            r = 1;
        } else {
            mathMulMatVec(v->p[1].in, pos, u);
            if (v->p[1].lim[0] + u[0] > 0) {
                if (v->planes < 5) {
                    r = 1;
                } else if (v->p[1].lim[1] + u[1] > 0) {
                    if (v->planes >= 6) {
                        s32 t = v->p[1].lim[2] + u[2];
                        r = t > 0;
                    } else
                        r = 1;
                }
            }
        }
    }
    return r;
}

/* @80150A34 hdpSetHitTag: tag a hit with explicit values, or the probe's tags when tag == 0. */
void __attribute__((section(".text_80150A34"))) hdpSetHitTag(HdpHitTag *h, s16 tag, s16 tag2, HdpProbe *pr)
{
    if (tag == 0) {
        h->tag = pr->tag;
        h->tag2 = pr->tag2;
    } else {
        h->tag = tag;
        h->tag2 = tag2;
    }
}

/* @80150A74: reset the hit state, traverse an object's hdp tree, return the state. */
HdpState *__attribute__((section(".text_80150A74"))) hdpQueryPoint(s32 a, s32 b, void *probe, s32 *value)
{
    HdpObject *obj;

    hdpQueryResult.hit = 0;
    hdpQueryResult.tag = -1;
    hdpQueryResult.tag2 = -1;
    hdpQueryResult.code = 0;
    *value = 0;
    obj = csGetRoot();
    if (obj->hdp) {
        hdpTraverse(obj, probe, &hdpQueryResult);
        if (hdpQueryResult.hit == 1)
            *value = obj->value;
    }
    return &hdpQueryResult;
}

/* ---- model draw / scene setup ----------------------------------------- */

typedef struct DrawEnv {
    u8 pad00[0x28];
    s32 flag28;         /* 0x28 */
    u8 pad2C[0x0C];
    s32 f38;            /* 0x38 */
    s32 ot;             /* 0x3C */
    s32 f40;            /* 0x40 */
    u8 pad44[0x08];
    void *f4C;          /* 0x4C */
    u8 pad50[0x10];
    u8 dr60[0x0C];      /* 0x60 DR_MODE */
    u8 dr6C[0x0C];      /* 0x6C DR_MODE */
    u8 pad78[0x10];
    s32 f88;            /* 0x88 */
} DrawEnv;

typedef struct DrawModel {
    u8 pad00[0x1C];
    u8 rect[8];         /* 0x1C RECT */
    u16 tpage;          /* 0x24 */
    u8 dtd;             /* 0x26 */
    u8 dfe;             /* 0x27 */
} DrawModel;

extern s32 renderFarDistanceDefault, renderFarDistance;
extern s32 renderFrameTicks, renderCurrentOt;
extern s32 renderSkipNodeCull, renderUnculledObject, renderNodeCullEnable;
extern DrawEnv *renderCtx;
extern s32 renderWorldShift;
extern u8 renderObjectMatrices[];
extern u8 renderMeshList[];
extern u8 renderMatrixSlots[];
extern u8 renderSharedWorkBuffer[];
extern u8 renderNodeHandlers[];
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"   /* request 93: portVisualNearOtz (bit 8) */
#endif
#include "semantic/port_rollback.h"   /* request 48: HOLD (no animation steps while held); stage 2: ANIM_IN_STEP,
                                         VISUAL_RAND (ONLINE_ROLLBACK.md 11) */
extern void *renderWorldModels[TM2_MAX_WORLD_MODELS];

extern s32 timerGetFrameTicks(void);
extern void renderInitState(void);
extern void csUpdMat(void *obj, void *mat, s32 mode);
extern void renderWalkModel(void *obj, void *model);
extern void SetDrawMode(void *p, s32 dfe, s32 dtd, s32 tpage, void *tw);
extern void sub_800CDC54(DrawModel *m, void *a, s32 ot, void *b, void *mat);
extern void renderNearPolys(DrawModel *m, void *a, s32 c);
extern void lightAddSources(void *src, s32 n, u8 *mats, u8 *objects);
extern void hdpInit(void *a);
extern void hdpSetStackBuffer(void *a);
extern void dbscanInitStacks(void *a, void *b);
extern void renderInitNodeHandlers(void *a);
extern void hdpSetDifficulty(s32 v);
extern void hdpSetShift(s32 a);

void renderClearModelLists(void);
void animClearList(void);

/* @80151378: draw one model with its own draw modes through the model emitter. */
void __attribute__((section(".text_80151378"))) renderDrawModel(DrawModel *m, HdpObject *obj)
{
    DrawEnv *e;

    renderFarDistance = renderFarDistanceDefault;
    renderFrameTicks = (s16)timerGetFrameTicks();
    renderInitState();
    if (renderNodeCullEnable == 0)
        renderSkipNodeCull = 1;
    renderCtx->flag28 = renderSkipNodeCull;
    csUpdMat(obj, renderObjectMatrices, renderNodeCullEnable);
    renderWalkModel(obj, obj->hdp);
    e = renderCtx;
    e->flag28 = 0;
    renderSkipNodeCull = 0;
    renderCurrentOt = e->ot;
    SetDrawMode(e->dr60, m->dfe, m->dtd, m->tpage, m->rect);
    SetDrawMode(renderCtx->dr6C, m->dfe, 1, m->tpage, m->rect);
    renderCtx->f4C = renderSharedWorkBuffer;
    renderCtx->f88 = 0;
    renderCtx->f40 = 0x46;
#ifdef TM2_PORT_VISUAL
    /* PC port request 93 (portVisual bit 8, RENDER_ANSWERS.md R8): threshold 0, no polygon cut by the near pass */
    renderCtx->f40 = portVisualNearOtz(0x46);
#endif
    sub_800CDC54(m, renderMeshList, renderCurrentOt, renderMatrixSlots, renderObjectMatrices);
    renderNearPolys(m, renderSharedWorkBuffer, renderCtx->f38);
}

/* @801514B8: draw an object through the generic path (no per-model draw modes). */
void __attribute__((section(".text_801514B8"))) renderDrawObjectUnculled(HdpObject *obj)
{
    renderSkipNodeCull = 1;
    renderUnculledObject = 1;
    renderFarDistance = renderFarDistanceDefault;
    renderInitState();
    csUpdMat(obj, renderObjectMatrices, renderNodeCullEnable);
    renderWalkModel(obj, obj->hdp);
    renderUnculledObject = 0;
    renderSkipNodeCull = 0;
    renderCurrentOt = renderCtx->ot;
    lightAddSources(renderMeshList, renderCurrentOt, renderMatrixSlots, renderObjectMatrices);
}

/* @8015155C: initialise the draw/scene system state. */
void __attribute__((section(".text_8015155C"))) renderInit(void)
{
    hdpInit(renderSharedWorkBuffer);
    hdpSetStackBuffer(renderSharedWorkBuffer);
    dbscanInitStacks(renderMatrixSlots, renderSharedWorkBuffer);
    renderInitNodeHandlers(renderNodeHandlers);
    renderSkipNodeCull = 0;
    renderUnculledObject = 0;
    renderNodeCullEnable = 0;
    renderClearModelLists();
}

/* @801515C8: clear the animation list and the per-frame callback table. */
void __attribute__((section(".text_801515C8"))) renderClearModelLists(void)
{
    s32 i;

    animClearList();
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++)
        renderWorldModels[i] = 0;
}

/* @80151608: address of the scene light/colour block. */
void *__attribute__((section(".text_80151608"))) renderGetSharedBuffer(void)
{
    return renderSharedWorkBuffer;
}

/* @80151618: address of the scene view block. */
void *__attribute__((section(".text_80151618"))) renderGetMatrixSlots(void)
{
    return renderMatrixSlots;
}

/* @80151628: copy n words. */
void __attribute__((section(".text_80151628"))) memCopyWords(s32 *dst, s32 *src, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
}

/* @8015166C renderSetWorldShift: store the difficulty and propagate it. */
void __attribute__((section(".text_8015166C"))) renderSetWorldShift(s32 level)
{
    renderWorldShift = level;
    hdpSetDifficulty(level);
    hdpSetShift(level);
}

/* @801516A4: current difficulty. */
s32 __attribute__((section(".text_801516A4"))) renderGetWorldShift(void)
{
    return renderWorldShift;
}

/* @801516B0 nullsub_20 */
void __attribute__((section(".text_801516B0"))) sub_801516B0(void)
{
}

/* @801516B8 nullsub_21 */
void __attribute__((section(".text_801516B8"))) sub_801516B8(void)
{
}

/* ---- texture-animation sequencer -------------------------------------- */

typedef struct Anim {
    u8 type;            /* 0x00: 3 timed frames, 9 step list */
    u8 pad01[4];
    u8 trigger_frame;   /* 0x05 */
    u16 period;         /* 0x06 */
    u8 first_frame;     /* 0x08 */
    u8 last_frame;      /* 0x09 */
    u8 mode;            /* 0x0A: 0 loop, 1 ping-pong, 2 random, 3 weighted random */
    u8 frames;          /* 0x0B */
    u8 frame;           /* 0x0C */
    u8 pad0D;
    u16 elapsed;        /* 0x0E */
    u8 done[2];         /* 0x10: per-buffer "updated" flags */
    u8 manual;          /* 0x12 */
    u8 child_frame;     /* 0x13 */
    u16 min_on;         /* 0x14 */
    u16 max_on;         /* 0x16 */
    u16 min_off;        /* 0x18 */
    u16 max_off;        /* 0x1A */
    struct Anim *child; /* 0x1C */
} Anim;

typedef struct AnimSteps {
    u8 type;            /* 0x00 (9) */
    u8 pad01[5];
    u8 count;           /* 0x06 */
    u8 cur;             /* 0x07 */
    u8 child_frame;     /* 0x08 */
    u8 trigger;         /* 0x09 */
    u8 pad0A[2];
    void *child;        /* 0x0C */
} AnimSteps;

extern s32 animListCount;          /* number of registered animations */
extern s32 animListEnabled;          /* animation updates enabled */
extern s32 renderFrameParity;          /* current buffer index */
extern s32 renderPrevFrameParity;          /* other buffer index */
extern Anim *animNodeList[100];
extern char strAnimLimitExceeded[];

extern int printf(const char *fmt, ...);
extern int rand(void);
extern void animStepLoop(Anim *a);
extern void animStepPingPong(Anim *a);
void animStepRandom(Anim *a);
void animStepWeightedRandom(Anim *a);
void animPickPeriod(Anim *a, s32 on);
void animControl(void *a, s32 mode);

/* @801516D0: clear the animation list. */
void __attribute__((section(".text_801516D0"))) animClearList(void)
{
    s32 i;

    animListCount = 0;
    animListEnabled = 0;
    for (i = 0; i < 100; i++)
        animNodeList[i] = 0;
}

/* @80151700: advance every registered animation once per buffer. */
void __attribute__((section(".text_80151700"))) animUpdateAll(void)
{
    s32 i;
    Anim *a;

    if (animListEnabled == 0)
        return;
#ifdef TM2_PORT_ROLLBACK
    /* request 48 HOLD: a held iteration draws only (no step, no animator rand()); stage 2 ANIM_IN_STEP: the step
       steps the animations (portRollbackAnimStep), the renderer's call steps nothing */
    if (portRollbackAnimDrawOff())
        return;
#endif
    for (i = 0; i < animListCount; i++) {
        a = animNodeList[i];
        a->done[renderPrevFrameParity] = 0;
        if (a->done[renderFrameParity] == 0) {
            if (a->mode == 0)
                animStepLoop(a);
            else if (a->mode == 1)
                animStepPingPong(a);
            else if (a->mode == 2)
                animStepRandom(a);
            else if (a->mode == 3)
                animStepWeightedRandom(a);
            a->done[renderFrameParity] = 1;
        }
    }
}

/* @80151830: register an animation (once). */
void __attribute__((section(".text_80151830"))) animRegister(Anim *a)
{
    s32 i;

    if (a == 0)
        return;
    if (animListCount >= 100) {
        printf(strAnimLimitExceeded);
        return;
    }
    for (i = 0; i < animListCount; i++) {
        if (animNodeList[i] == a)
            return;
    }
    animNodeList[animListCount] = a;
    animListCount++;
}

/* @801518CC: disable animation updates. */
void __attribute__((section(".text_801518CC"))) animDisable(void)
{
    animListEnabled = 0;
}

/* @801518D8: enable animation updates. */
void __attribute__((section(".text_801518D8"))) animEnable(void)
{
    animListEnabled = 1;
}

/* @801518E8: advance one (non-manual) animation outside the list. */
void __attribute__((section(".text_801518E8"))) animUpdateOne(Anim *a)
{
#ifdef TM2_PORT_ROLLBACK
    if (portRollbackAnimDrawOff())  /* request 48 HOLD and stage 2 ANIM_IN_STEP, as animUpdateAll */
        return;
#endif
    if (a->manual == 0) {
        a->done[renderPrevFrameParity] = 0;
        if (a->done[renderFrameParity] == 0) {
            if (a->mode == 0)
                animStepLoop(a);
            else if (a->mode == 1)
                animStepPingPong(a);
            else if (a->mode == 2)
                animStepRandom(a);
            else if (a->mode == 3)
                animStepWeightedRandom(a);
            a->done[renderFrameParity] = 1;
        }
    }
}

#ifdef TM2_PORT_ROLLBACK
/* ---- request 48 stage 2, ANIM_IN_STEP (ONLINE_ROLLBACK.md 4.4 option (b), 11) ----
 * The animations step in the simulation step, not in the renderer: rtMainLoop calls portRollbackAnimStep right
 * after renderFlipFrame (where view 0's renderScene stepped them in retail), and animUpdateAll / animUpdateOne
 * return (portRollbackAnimDrawOff). The step covers what a renderer without culling would reach: the registered
 * list (animUpdateAll's loop) and every type-3 node of the level model (csGetRoot, whose animation frames hold
 * the hit volumes of DISH and FREEWAY), the world models, the view's follow object (shared by every view:
 * viewSetFollowObj at the load) and every visible object (+0x08; not the on-screen flag +0x124, which is
 * culling). The walk takes the renderer's choices where they are state (an animation's current frame, a select's
 * step, the hidden flags) and every branch where they are camera-dependent (all LOD bands, both zone children).
 * The parity guard steps a node shared by several objects once. The animators' rand() is the game's stream here
 * (RB_ANIM_RAND: VISUAL_RAND only moves draw-time animators): the walk is the same on every machine. */
#define RB_PORT_TEXT __attribute__((section(".port_text")))
extern void *csListHead(void);
extern void *viewGetFollowObj(int i);

/* animUpdateOne's step without its gate */
static void RB_PORT_TEXT portRollbackAnimStepOne(Anim *a)
{
    if (a->manual == 0) {
        a->done[renderPrevFrameParity] = 0;
        if (a->done[renderFrameParity] == 0) {
            if (a->mode == 0)
                animStepLoop(a);
            else if (a->mode == 1)
                animStepPingPong(a);
            else if (a->mode == 2)
                animStepRandom(a);
            else if (a->mode == 3)
                animStepWeightedRandom(a);
            a->done[renderFrameParity] = 1;
        }
    }
}

/* The renderer's tree (renderWalkModel's node types; reference/renderer_walker_engine_render_scene.c) without the
 * camera: 1 group, 4 offset, 5 transform (unless hidden), 12 scale: every child; 2 LOD: every band; 3 animation:
 * step it unless listed (+0x12), then its current frame; 9 select: its current step; 11 zones: both children. */
static void RB_PORT_TEXT portRollbackAnimWalk(u8 *node)
{
    s32 i, n;
    u8 **child;

    while (node) {
        switch (node[0]) {
        case 1:
            if (node[21] & 2)
                return;
            n = node[20];
            child = (u8 **)(node + 24);
            break;
        case 2:
            n = *(s32 *)(node + 16);
            child = (u8 **)(node + 20);
            for (i = 0; i < n; i++)
                portRollbackAnimWalk(*(u8 **)(child[i] + 8));
            return;
        case 3:
            if (node[18] == 0)
                portRollbackAnimStepOne((Anim *)node);
            node = ((u8 **)(node + 32))[node[12]];
            continue;
        case 4:
            if (node[19] & 2)
                return;
            n = node[18];
            child = (u8 **)(node + 20);
            break;
        case 5:
            if (node[39] & 2)
                return;
            n = node[38];
            child = (u8 **)(node + 40);
            break;
        case 9:
            node = ((u8 **)(node + 16))[node[7]];
            continue;
        case 11:
            if (node[4] & 2)
                return;
            portRollbackAnimWalk(*(u8 **)(node + 8));
            node = *(u8 **)(node + 12);
            continue;
        case 12:
            n = *(u16 *)(node + 6);
            child = (u8 **)(node + 8);
            break;
        default:
            return;
        }
        for (i = 0; i < n; i++)
            portRollbackAnimWalk(child[i]);
        return;
    }
}

void RB_PORT_TEXT portRollbackAnimStep(void)
{
    s32 i;
    Anim *a;
    u8 *obj;

    if (!(portRollbackOptions & TM2_RB_ANIM_IN_STEP))
        return;
    if (animListEnabled != 0) {     /* animUpdateAll's loop */
        for (i = 0; i < animListCount; i++) {
            a = animNodeList[i];
            a->done[renderPrevFrameParity] = 0;
            if (a->done[renderFrameParity] == 0) {
                if (a->mode == 0)
                    animStepLoop(a);
                else if (a->mode == 1)
                    animStepPingPong(a);
                else if (a->mode == 2)
                    animStepRandom(a);
                else if (a->mode == 3)
                    animStepWeightedRandom(a);
                a->done[renderFrameParity] = 1;
            }
        }
    }
    obj = (u8 *)csGetRoot();
    if (obj)
        portRollbackAnimWalk(*(u8 **)(obj + 284));
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++)
        portRollbackAnimWalk((u8 *)renderWorldModels[i]);
    obj = (u8 *)viewGetFollowObj(0);
    if (obj)
        portRollbackAnimWalk(*(u8 **)(obj + 284));
    for (obj = (u8 *)csListHead(); obj; obj = *(u8 **)(obj + 288)) {
        if (*(s32 *)(obj + 8))
            portRollbackAnimWalk(*(u8 **)(obj + 284));
    }
}
#endif /* TM2_PORT_ROLLBACK */

/* @80151BC4: random mode: pick a random frame when the period elapses. */
void __attribute__((section(".text_80151BC4"))) animStepRandom(Anim *a)
{
    a->elapsed = a->elapsed + renderFrameTicks;
    if (a->elapsed > a->period) {
        a->frame = RB_ANIM_RAND() % (a->frames - 1);    /* stage 2 VISUAL_RAND (retail's rand() without it) */
        if (a->frame == 0)
            animPickPeriod(a, 0);
        else
            animPickPeriod(a, 1);
        if (a->child && a->frame == a->trigger_frame)
            animControl(a->child, a->child_frame);
        a->elapsed = 0;
    }
}

/* @80151CB8: weighted random mode: low frames are more likely (weights n..1). */
void __attribute__((section(".text_80151CB8"))) animStepWeightedRandom(Anim *a)
{
    s32 n, k, i, sum, r, acc;

    a->elapsed = a->elapsed + renderFrameTicks;
    if (a->elapsed > a->period) {
        k = a->frames - 1;
        sum = 0;
        n = k;
        for (i = 1; i <= n; i++)
            sum += i;
        r = RB_ANIM_RAND() % sum;
        acc = 0;
        for (i = 0; i < n; i++) {
            acc += n - i;
            if (r < acc) {
                k = i + 1;
                break;
            }
        }
        a->frame = k;
        if (a->child && a->frame == a->trigger_frame)
            animControl(a->child, a->child_frame);
    }
}

/* @80151DF4: choose the next period, random in [min, max] for the on/off phase. */
void __attribute__((section(".text_80151DF4"))) animPickPeriod(Anim *a, s32 on)
{
    u16 v;

    if (on == 0) {
        if (a->max_off - a->min_off >= 2)
            v = RB_ANIM_RAND() % (a->max_off + 1 - a->min_off) + a->min_off;
        else
            v = a->min_off;
    } else {
        if (a->max_on - a->min_on >= 2)
            v = RB_ANIM_RAND() % (a->max_on + 1 - a->min_on) + a->min_on;
        else
            v = a->min_on;
    }
    a->period = v;
}

/* @80151F14: set the current step of a type-9 step list; fire its child on the trigger step. */
void __attribute__((section(".text_80151F14"))) animSetStep(AnimSteps *s, u32 step)
{
    if (s && step < s->count && s->type == 9) {
        s->cur = step;
        if (s->child && s->cur == s->trigger)
            animControl(s->child, s->child_frame);
    }
}

/* @80151F90: control an animation: 0 reset, 1 step now, 2 jump to last frame. */
void __attribute__((section(".text_80151F90"))) animControl(void *p, s32 mode)
{
    AnimSteps *s = (AnimSteps *)p;

    if (((Anim *)p)->type == 3) {
        Anim *a = (Anim *)p;
        if (mode == 0) {
            a->elapsed = 0;
            a->frame = 0;
        } else if (mode == 1) {
            a->elapsed = a->period + 1;
            if (a->mode == 0)
                animStepLoop(a);
            else if (a->mode == 1)
                animStepPingPong(a);
            else if (a->mode == 2)
                animStepRandom(a);
            else if (a->mode == 3)
                animStepWeightedRandom(a);
        } else if (mode == 2) {
            a->elapsed = 0;
            a->frame = a->frames - 1;
        }
        if (mode != 1 && a->child && a->frame == a->trigger_frame)
            animControl(a->child, a->child_frame);
    } else if (s->type == 9) {
        if (mode == 0)
            s->cur = 0;
        if (mode == 2)
            s->cur = s->count - 1;
        if (mode == 1) {
            s->cur++;
            if (s->cur >= s->count)
                s->cur = 0;
        }
        if (s->child && s->cur == s->trigger)
            animControl(s->child, s->child_frame);
    }
}
