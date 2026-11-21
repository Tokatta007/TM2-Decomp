/* Library code 80152000..80154000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything here is SingleTrac engine code (scene renderer and lighting
 * environments); no third-party source was used, all written by hand
 * against the retail code.
 *
 * Recipe: the engine one, psyq3.6 cc1 + maspsx, -fvolatile, extern_gp,
 * gp_size 8 (not the SDK psyq3.3 / gp_size 0 recipe: the renderer and
 * the light functions address $gp globals). All 20 functions byte-identical.
 *
 * renderPushMatrix, renderWalkModel and renderMulTransMatVec are hand-written assembly
 * (non-ABI register conventions, state in the scratchpad at 0x1F800000,
 * node handlers dispatched through a runtime table that renderInitNodeHandlers
 * fills). They are kept as the original instruction sequence in
 * file-scope asm at the end of the file, each in its own .text_<VA>
 * section. ".set	noreorder" (TAB) stops maspsx from inserting nops,
 * ".set noreorder" (space, seen only by the assembler) keeps GNU as from
 * filling delay slots; the cop2 command words are emitted as .word. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    s16 m[3][3];
    s32 t[3];
} MATRIX;

/* Renderer state record (in the scratchpad, 0x1F800000), shared with the
 * model emitter sub_800CDC54 and the model walker renderWalkModel. */
typedef struct {
    void *near_overflow; /* 0x00 */
    void *m04;           /* 0x04 */
    u32 count;           /* 0x08 matrix slot counter (<= 200) */
    void *m0C;           /* 0x0C */
    s32 depth;           /* 0x10 */
    s32 m14;
    void *m18;           /* 0x18 */
    void *matrices;      /* 0x1C */
    void *m20;           /* 0x20 */
    s32 m24;
    s32 m28;             /* 0x28 */
    void *m2C;           /* 0x2C node handler table */
    s32 m30;
    s32 objects;         /* 0x34 object counter */
    s32 near_count;      /* 0x38 */
    s32 end;             /* 0x3C */
    s32 near_otz;        /* 0x40 */
    void *m44;           /* 0x44 */
    void *m48;           /* 0x48 */
    void *near_list;     /* 0x4C */
    s32 pad50[4];
    u32 mode_a[3];       /* 0x60 */
    u32 mode_b[3];       /* 0x6C */
    s32 pad78[4];
    s32 cull_flip;       /* 0x88 */
} RenderState;

extern RenderState *renderCtx;
extern s32 renderFarDistance;
extern s32 renderFarDistanceDefault;
extern s32 renderWorldShift;
extern s32 renderFrameParity;
extern s32 renderPrevFrameParity;
extern s32 renderSkipNodeCull;
extern s32 renderUnculledObject;
extern s32 renderNodeCullEnable;
extern s32 renderCurrentOt;
extern s32 renderFrameTicks;
extern s32 lightCurrentEnv;

extern u8 renderSharedWorkBuffer[];
extern u8 renderSlotMatrixPtrs[];
extern u8 renderWalkerSaveRegs[];
extern u8 renderMatrixSlots[];
extern u8 renderObjectMatrices[];
extern u8 renderMeshList[];
extern u8 renderNodeHandlers[];
extern u8 renderFrustumMatricesA[];
extern u8 renderFrustumMatricesB[];
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern void *renderWorldModels[];

#include "semantic/world.h"
#include "semantic/port_far_lod.h"
#include "semantic/port_far_distance.h"
#include "semantic/port_visual.h"
#include "semantic/port_render_stats.h"
#include "semantic/render_slots.h"    /* the matrix slot limit (request 47; retail 200 / 198) */
extern s32 viewGetMode(s32 view);
extern u8 *viewGetFollowObj(s32 view);
extern u8 *csListHead(void);
extern s32 viewGetRenderSplitMode(void);
extern void memCopyWords(void *dst, void *src, s32 n);
extern void csUpdMat(void *obj, void *slot, s32 mode);   /* csUpdMat */
extern void sub_8014215C(void *p, s32 dfe, s32 dtd, s32 tpage, void *tw); /* SetDrawMode */
extern void sub_800CDC54(void *db, void *a, s32 b, void *c, void *d);
extern void renderNearPolys(void *db, void *a, s32 b);
extern void animUpdateAll(void);
extern void renderWalkModel(u8 *obj, void *model);
extern s32 sub_8013F260(s32 a);   /* rsin */
extern s32 sub_8013F33C(s32 a);   /* rcos */

/* Frame bookkeeping: flip the double-buffer flag and store ticks per frame. */
void __attribute__((section(".text_80152194"))) renderFlipFrame(void)
{
    renderPrevFrameParity = renderFrameParity;
    renderFrameParity = renderFrameParity == 0;
    renderFrameTicks = (s16)timerGetFrameTicks();
}

/* Fill the renderer state record for a new frame. */
void __attribute__((section(".text_801521D0"))) renderInitState(void)
{
    RenderState *s;
    s32 depth, m24, m28, near;

    renderCtx->near_overflow = renderSharedWorkBuffer;
    s = *(RenderState **)&renderCtx;
    depth = *(s32 *)&renderFarDistance;
    m24 = *(s32 *)&renderUnculledObject;
    m28 = *(s32 *)&renderSkipNodeCull;
    near = *(s32 *)&renderWorldShift;
    s->m04 = renderSlotMatrixPtrs;
    s->count = 0;
    s->m0C = renderWalkerSaveRegs;
    s->depth = depth;
    s->m14 = 0;
    s->m18 = renderMatrixSlots;
    s->matrices = renderObjectMatrices;
    s->m20 = renderMeshList;
    s->m24 = m24;
    s->m28 = m28;
    s->m2C = renderNodeHandlers;
    s->m30 = 0;
    s->objects = 0;
    s->near_count = near;
    s->end = 0;
    s->near_otz = 0;
    s->m44 = renderFrustumMatricesA;
    s->m48 = renderFrustumMatricesB;
}

/* The far distance of the normal views: retail's default, or the PC port's
 * (request 31, FAR_DISTANCE.md: portFarDistance, 0 = retail). */
#ifdef TM2_PORT_FAR_DISTANCE
#define RENDER_FAR_DEFAULT portFarDistanceFar(renderFarDistanceDefault)
#else
#define RENDER_FAR_DEFAULT renderFarDistanceDefault
#endif
/* The rear-view mirror's (view mode 4) far distances, 500 for the level and the world models, 1000 for the objects:
 * retail's, or the port's portMirrorFar for both (request 46 item 9, PLAYTEST_ANSWERS.md 15). */
#ifdef TM2_PORT_FAR_DISTANCE
#define RENDER_FAR_MIRROR(retail) portFarDistanceMirror(retail)
#else
#define RENDER_FAR_MIRROR(retail) (retail)
#endif

/* PC port request 45 item 3 (portVisual bit 2, PLAYTEST_ANSWERS.md 3): the object loop skips the car walked
 * first (`own`, 0 with the bit clear). */
#ifdef TM2_PORT_VISUAL
#define RENDER_OBJ_WALKED(obj) (*(s32 *)((obj) + 8) && (obj) != own && (obj) != mirror_own)
#else
#define RENDER_OBJ_WALKED(obj) (*(s32 *)((obj) + 8))
#endif

/* PC port request 46 item 5 (portVisual bit 3, PLAYTEST_ANSWERS.md 12): each walk of a world model or an object
 * tells the port which draw entries it queued (the walker leaves its queue pointer at ctx+0x20), so the shadows can
 * be sorted behind their cars before the emitter (portVisualShadows). Retail: the plain walk. */
#ifdef TM2_PORT_VISUAL
#define RENDER_WALK(obj, model) do { \
        u8 *first_ = (u8 *)renderCtx->m20; \
        renderWalkModel((obj), (model)); \
        portVisualWalked((obj), (model), first_); \
    } while (0)
#else
#define RENDER_WALK(obj, model) renderWalkModel((obj), (model))
#endif

/* Scene renderer: level model, registered models, the view's own model and
 * every active object, then the polygon emitter passes. */
void __attribute__((section(".text_80152284"))) renderScene(u8 *db, s32 view)
{
    u32 *count;
    s32 *objects;
    u8 *obj;
    s32 i;
#ifdef TM2_PORT_FAR_DISTANCE
    s32 meshBias;
#endif
#ifdef TM2_PORT_VISUAL
    u8 *own;
    u8 *mirror_own;     /* request 99 */
    s32 keep_depth, keep_m28;
#endif
#ifdef TM2_PORT_HIDDEN_AREAS
    s32 hidden;
#endif
#ifdef TM2_PORT_VISUAL
    s32 steady;
#endif
#ifdef TM2_PORT_FAR_LOD
    s32 farLod;

    /* PC port request 30 (FAR_LOD.md): with the port's switch on, the LOD records found at
     * load keep their farthest band past its end while this view's models are walked; request
     * 46 item 6 (FAR_LOD.md 8): with portFarLodNearScale every band is that much farther. */
#ifdef TM2_PORT_FAR_DISTANCE
    portFarLodBudgetView(view);     /* request 98: this view's band scale by its previous frame (FAR_LOD.md 14) */
#endif
    farLod = portFarLod | portFarLodNearScale;
    if (farLod)
        farLod = portFarLodBegin();
#endif
#ifdef TM2_PORT_HIDDEN_AREAS
    /* PC port request 72 (HIDDEN_AREAS.md 11): with portVisual bit 6 the level's hidden areas (Paris's church) are
     * drawn from anywhere for this view's walks, for the port's depth test. */
    hidden = portHiddenAreasBegin(view);
    /* request 113 (HIDDEN_AREAS.md 16): the breakable walls drawn from both sides, back in portHiddenAreasDrawn */
    hidden += portTwoSidedBegin();
#endif
#ifdef TM2_PORT_VISUAL
    /* PC port request 92 (portVisual bit 7, RENDER_ANSWERS.md R7): the cars' shadows tilted with their cars only (no
     * suspension) for this view's walks; the step's matrices back after them. */
    steady = portVisualSteadyBegin();
#endif

    count = &renderCtx->count;
    objects = &renderCtx->objects;
    if (viewGetMode(view) == 4) {
        renderFarDistance = RENDER_FAR_MIRROR(500);
    } else {
        renderFarDistance = RENDER_FAR_DEFAULT;
    }
    renderInitState();
#ifdef TM2_PORT_FAR_DISTANCE
    /* PC port request 31 (FAR_DISTANCE.md): the port's mesh queue and cap. */
    meshBias = portFarDistanceMeshListBegin();
#endif
#ifdef TM2_PORT_VISUAL
    /* bit 2: the view's own car first, as the object loop below walks it (its own slot, the loop's depth and node
       cull setting), then one more slot so the level's matrix (copied to slot *count) does not overwrite it */
    own = portVisualOwnCar(view);
    mirror_own = portVisualMirrorSkip(view);    /* request 99 (bit 9): the mirror's eye sits on that car */
    if (own) {
        keep_depth = renderCtx->depth;
        keep_m28 = renderCtx->m28;
        renderCtx->depth = RENDER_FAR_DEFAULT;
        renderCtx->m28 = renderNodeCullEnable == 0 ? 1 : renderSkipNodeCull;
        if (*count < TM2_RENDER_SLOT_LIMIT) {
            *count = *count + 1;
        }
        csUpdMat(own, renderObjectMatrices + (*count << 5), renderNodeCullEnable);
        if (*(s32 *)(own + 292)) {
            *objects = *objects + 1;
            RENDER_WALK(own, *(void **)(own + 284));
        }
        renderCtx->depth = keep_depth;
        renderCtx->m28 = keep_m28;
        if (*count < TM2_RENDER_SLOT_LIMIT) {
            *count = *count + 1;
        }
        /* and one more object number (HIDDEN_AREAS.md 10): the level walk starts its translation key at the counter
           (w.obj = ctx+0x34) without a step of its own, so its top-level meshes would share the key of the car's
           last-numbered part; the emitter recomputes the translation only when the key changes from the previous
           entry, and the level's first meshes would be drawn at the car's translation (Amazonia's tunnel seen from
           above the level's origin) */
        *objects = *objects + 1;
    }
#endif
    obj = csGetRoot();
    if (*(void **)(obj + 284)) {
        memCopyWords(renderObjectMatrices + (*count << 5), *(u8 **)(obj + 4) + 44, 8);
        renderWalkModel(obj, *(void **)(obj + 284));
    }
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++) {
        if (renderWorldModels[i]) {
            if (*count < TM2_RENDER_SLOT_LIMIT) {
                *count = *count + 1;
            }
            obj = csGetRoot();
            memCopyWords(renderObjectMatrices + (*count << 5), *(u8 **)(obj + 4) + 44, 8);
            RENDER_WALK(obj, renderWorldModels[i]);
        }
    }
    renderFarDistance = RENDER_FAR_DEFAULT;
    renderCtx->depth = renderFarDistance;
    obj = viewGetFollowObj(view);
    if (obj) {
        if (*count < TM2_RENDER_SLOT_LIMIT) {
            *count = *count + 1;
        }
        csUpdMat(obj, renderObjectMatrices + (*count << 5), 1);
        *objects = *objects + 1;
        renderWalkModel(obj, *(void **)(obj + 284));
    }
    if (viewGetMode(view) == 4) {
        renderFarDistance = RENDER_FAR_MIRROR(1000);
    } else {
        renderFarDistance = RENDER_FAR_DEFAULT;
    }
    renderCtx->depth = renderFarDistance;
    if (renderNodeCullEnable == 0) {
        renderSkipNodeCull = 1;
    }
    renderCtx->m28 = renderSkipNodeCull;
    for (obj = csListHead(); obj; obj = *(u8 **)(obj + 288)) {
        if (RENDER_OBJ_WALKED(obj)) {
            if (*count < TM2_RENDER_SLOT_LIMIT) {
                *count = *count + 1;
            }
            csUpdMat(obj, renderObjectMatrices + (*count << 5), renderNodeCullEnable);
            if (*(s32 *)(obj + 292)) {
                *objects = *objects + 1;
                RENDER_WALK(obj, *(void **)(obj + 284));
            }
        }
    }
    renderCtx->m28 = 0;
    renderSkipNodeCull = 0;
#ifdef TM2_PORT_HIDDEN_AREAS
    if (hidden)
        portHiddenAreasEnd();   /* the level's words again before anything else reads them */
#endif
#ifdef TM2_PORT_FAR_LOD
    if (farLod)
        portFarLodEnd();    /* the retail records again before anything else reads them */
#endif
#ifdef TM2_PORT_VISUAL
    if (steady)
        portVisualSteadyEnd();  /* request 92: the step's shadow matrices again */
#endif
    renderCurrentOt = renderCtx->end;
#ifdef TM2_PORT_FAR_DISTANCE
    renderCurrentOt -= meshBias;
    /* the render capacity counter (RENDER_ANSWERS.md "Measuring the render caps"): the matrix slots and meshes now,
       before the emitter overwrites ctx+0x08 / ctx+0x3C */
    portRenderStatsWalked(db, view);
#endif
    sub_8014215C(renderCtx->mode_a, db[39], db[38], *(u16 *)(db + 36), db + 28);
    sub_8014215C(renderCtx->mode_b, db[39], 1, *(u16 *)(db + 36), db + 28);
    renderCtx->near_list = renderSharedWorkBuffer;
    if (viewGetMode(view) == 4) {
        renderCtx->cull_flip = 1;
    } else {
        renderCtx->cull_flip = 0;
    }
    if (viewGetRenderSplitMode()) {
        renderCtx->near_otz = 20;
    } else {
        renderCtx->near_otz = 70;
    }
#ifdef TM2_PORT_VISUAL
    /* PC port request 93 (portVisual bit 8, RENDER_ANSWERS.md R8): threshold 0, no polygon cut by the near pass */
    renderCtx->near_otz = portVisualNearOtz(renderCtx->near_otz);
    portVisualShadows();    /* request 46 item 5: bit 3, the shadows behind their cars (clears the view's ranges) */
#endif
#ifdef TM2_PORT_FAR_DISTANCE
    /* the port's queue, the GTE range pass for far meshes, then the retail emitter */
    portFarDistanceEmit(db, renderCurrentOt);
#else
    sub_800CDC54(db, renderMeshList, renderCurrentOt, renderMatrixSlots, renderObjectMatrices);
#endif
    renderNearPolys(db, renderSharedWorkBuffer, renderCtx->near_count);
#ifdef TM2_PORT_HIDDEN_AREAS
    if (hidden)
        portHiddenAreasDrawn(); /* request 84: the vertices moved for this view back, once the emitter and the near
                                   pass have read them */
#endif
#ifdef TM2_PORT_FAR_DISTANCE
    portRenderStatsDrawn(db);   /* the frame's packet bytes after this view, the caps reached */
#endif
    animUpdateAll();
}

/* ---- lighting environments ---- */

typedef struct {
    s32 x, y, z;
} LightVec;

typedef struct {
    s32 ambient[3];  /* +0 */
    MATRIX color;    /* +12 local colour matrix (columns = lights) */
    MATRIX light;    /* +44 local light matrix (rows = directions) */
} LightEnv;          /* 76 bytes */

extern s32 lightAmbientColor[];        /* current ambient colour */
extern LightVec lightAngles[];   /* light angles (x = elevation, z = heading) */
extern LightVec lightColors[];   /* light colours */
extern LightEnv lightEnvironments[];   /* 5 environments */

/* Install the model node handler table (labels inside renderWalkModel). */
extern u8 renderNodeHandler0[], renderNodeHandler1[], renderNodeHandler2[], renderNodeAnimHandler[], renderNodeOffsetHandler[],
    renderNodeTransformHandler[], renderNodeSkipHandler[], renderNodeSelectHandler[], renderNodeZoneHandler[], renderNodeScaleHandler[];
/* The "skip" handler (801527A4) is named from different neighbouring labels
 * so the compiler rematerialises its address for every entry, as retail does. */
extern u8 D_801527A0[], D_8015279C[], D_80152798[], D_80152794[];
/* PC port request 47 (TM2_PORT_RENDER_SLOTS, render_slots.h): the variant links the C walker (end of the file), which
 * dispatches with a switch and never reads the table, and has none of the asm's labels: every entry is the walker's
 * entry, as the all-C link resolves the labels. Retail: the labels. */
#ifdef TM2_PORT_RENDER_SLOTS
#define RENDER_NODE_HANDLER(label) ((u8 *)renderWalkModel)
#define RENDER_NODE_HANDLER_AT(label, off) ((u8 *)renderWalkModel)
#else
#define RENDER_NODE_HANDLER(label) (label)
#define RENDER_NODE_HANDLER_AT(label, off) ((label) + (off))
#endif
void __attribute__((section(".text_80153460"))) renderInitNodeHandlers(void **t)
{
    t[0] = RENDER_NODE_HANDLER(renderNodeHandler0);
    t[1] = RENDER_NODE_HANDLER(renderNodeHandler1);
    t[2] = RENDER_NODE_HANDLER(renderNodeHandler2);
    t[3] = RENDER_NODE_HANDLER(renderNodeAnimHandler);
    t[4] = RENDER_NODE_HANDLER(renderNodeOffsetHandler);
    t[5] = RENDER_NODE_HANDLER(renderNodeTransformHandler);
    t[6] = RENDER_NODE_HANDLER(renderNodeSkipHandler);
    t[7] = RENDER_NODE_HANDLER_AT(D_801527A0, 4);
    t[8] = RENDER_NODE_HANDLER_AT(D_8015279C, 8);
    t[9] = RENDER_NODE_HANDLER(renderNodeSelectHandler);
    t[10] = RENDER_NODE_HANDLER_AT(D_80152798, 12);
    t[11] = RENDER_NODE_HANDLER(renderNodeZoneHandler);
    t[12] = RENDER_NODE_HANDLER(renderNodeScaleHandler);
    t[13] = RENDER_NODE_HANDLER_AT(D_80152794, 16);
}

/* Set light l of environment env: direction from two angles in degrees,
 * colour (r, g, b). */
void __attribute__((section(".text_80153510"))) lightSetEnvLight(s32 env, s32 l, s32 az, s32 el, s16 r, s16 g, s16 b)
{
    s32 x, y, z;

    if (env < 5 && l < 3) {
        x = -(sub_8013F260((az << 12) / 360) * sub_8013F33C((el << 12) / 360)) / 4096;
        y = -(sub_8013F33C((az << 12) / 360) * sub_8013F33C((el << 12) / 360)) / 4096;
        z = sub_8013F260((-el << 12) / 360);
        lightEnvironments[env].light.m[l][0] = -x;
        lightEnvironments[env].light.m[l][1] = -y;
        lightEnvironments[env].light.m[l][2] = -z;
        lightEnvironments[env].color.m[0][l] = r << 4;
        lightEnvironments[env].color.m[1][l] = g << 4;
        lightEnvironments[env].color.m[2][l] = b << 4;
    }
}

/* Set the direction of light l (angles in degrees) for environment env. */
void __attribute__((section(".text_801536F4"))) lightSetEnvDirection(s32 env, s32 l, s16 az, s16 el)
{
    s32 x, y, z;

    if (env < 5 && l < 3) {
        lightAngles[l].x = el;
        lightAngles[l].y = 0;
        lightAngles[l].z = az;
        x = -(sub_8013F260((az << 12) / 360) * sub_8013F33C((el << 12) / 360)) / 4096;
        y = -(sub_8013F33C((az << 12) / 360) * sub_8013F33C((el << 12) / 360)) / 4096;
        z = sub_8013F260((-el << 12) / 360);
        lightEnvironments[env].light.m[l][0] = -x;
        lightEnvironments[env].light.m[l][1] = -y;
        lightEnvironments[env].light.m[l][2] = -z;
    }
}

/* Set the colour of light l for environment env. */
void __attribute__((section(".text_801538A8"))) lightSetEnvColor(s32 env, s32 l, s16 r, s16 g, s16 b)
{
    if (env < 5 && l < 3) {
        lightColors[l].x = r;
        lightColors[l].y = g;
        lightColors[l].z = b;
        lightEnvironments[env].color.m[0][l] = r << 4;
        lightEnvironments[env].color.m[1][l] = g << 4;
        lightEnvironments[env].color.m[2][l] = b << 4;
    }
}

/* Set the ambient colour of environment env (and the current ambient). */
void __attribute__((section(".text_80153964"))) lightSetEnvAmbient(s32 env, s16 r, s16 g, s16 b)
{
    if (env < 5) {
        lightEnvironments[env].ambient[0] = r;
        lightEnvironments[env].ambient[1] = g;
        lightEnvironments[env].ambient[2] = b;
        lightAmbientColor[0] = r;
        lightAmbientColor[1] = g;
        lightAmbientColor[2] = b;
    }
}

void lightRebuildCurrentEnv(s32 unused);
void lightSetRedFlashEnv(void);
void lightSetCyanEnv(void);
void lightSetLightningEnv(void);

/* lightInit: default ambient, light angles and colours; build environments
 * 0 (normal), 1 (red flash), 2 and 3 (lightning). */
void __attribute__((section(".text_801539DC"))) lightInit(void)
{
    lightAmbientColor[0] = 32;
    lightAmbientColor[1] = 32;
    lightAmbientColor[2] = 32;
    lightAngles[0].z = 170;
    lightAngles[0].x = 30;
    lightAngles[0].y = 0;
    lightAngles[1].z = -60;
    lightAngles[1].x = 15;
    lightAngles[1].y = 0;
    lightAngles[2].z = 60;
    lightAngles[2].x = 20;
    lightAngles[2].y = 0;
    lightColors[0].x = 200;
    lightColors[0].y = 200;
    lightColors[0].z = 200;
    lightColors[1].x = 128;
    lightColors[1].y = 75;
    lightColors[1].z = 75;
    lightColors[2].x = 60;
    lightColors[2].y = 128;
    lightColors[2].z = 60;
    lightRebuildCurrentEnv(0);
    lightSetRedFlashEnv();
    lightSetCyanEnv();
    lightSetLightningEnv();
}

/* lightSetRedFlashEnv: environment 1, all lights red. */
void __attribute__((section(".text_80153AE4"))) lightSetRedFlashEnv(void)
{
    lightSetEnvAmbient(1, lightAmbientColor[0], lightAmbientColor[1], lightAmbientColor[2]);
    lightSetEnvLight(1, 0, lightAngles[0].z, lightAngles[0].x, 255, 0, 0);
    lightSetEnvLight(1, 1, lightAngles[1].z, lightAngles[1].x, 255, 0, 0);
    lightSetEnvLight(1, 2, lightAngles[2].z, lightAngles[2].x, 255, 0, 0);
}

/* Environment 2: all lights (100, 255, 255). */
void __attribute__((section(".text_80153BA0"))) lightSetCyanEnv(void)
{
    lightSetEnvAmbient(2, lightAmbientColor[0], lightAmbientColor[1], lightAmbientColor[2]);
    lightSetEnvLight(2, 0, lightAngles[0].z, lightAngles[0].x, 100, 255, 255);
    lightSetEnvLight(2, 1, lightAngles[1].z, lightAngles[1].x, 100, 255, 255);
    lightSetEnvLight(2, 2, lightAngles[2].z, lightAngles[2].x, 100, 255, 255);
}

/* lightSetLightningEnv: environment 3, bright white key light only. */
void __attribute__((section(".text_80153C68"))) lightSetLightningEnv(void)
{
    lightSetEnvAmbient(3, 256, 256, 256);
    lightSetEnvLight(3, 0, lightAngles[0].z, lightAngles[0].x, 256, 256, 256);
    lightSetEnvLight(3, 1, lightAngles[1].z, lightAngles[1].x, 0, 0, 0);
    lightSetEnvLight(3, 2, lightAngles[2].z, lightAngles[2].x, 0, 0, 0);
}

/* Rebuild the current environment (lightCurrentEnv) from the light settings. */
void __attribute__((section(".text_80153D10"))) lightRebuildCurrentEnv(s32 unused)
{
    lightSetEnvAmbient(lightCurrentEnv, lightAmbientColor[0], lightAmbientColor[1], lightAmbientColor[2]);
    lightSetEnvLight(lightCurrentEnv, 0, lightAngles[0].z, lightAngles[0].x,
                 lightColors[0].x, lightColors[0].y, lightColors[0].z);
    lightSetEnvLight(lightCurrentEnv, 1, lightAngles[1].z, lightAngles[1].x,
                 lightColors[1].x, lightColors[1].y, lightColors[1].z);
    lightSetEnvLight(lightCurrentEnv, 2, lightAngles[2].z, lightAngles[2].x,
                 lightColors[2].x, lightColors[2].y, lightColors[2].z);
}

/* Current ambient colour. */
s32 * __attribute__((section(".text_80153E0C"))) lightGetAmbient(void)
{
    return lightAmbientColor;
}

/* Light angles. */
LightVec * __attribute__((section(".text_80153E1C"))) lightGetAngles(void)
{
    return lightAngles;
}

/* Light colours. */
LightVec * __attribute__((section(".text_80153E2C"))) lightGetColors(void)
{
    return lightColors;
}

/* lightGetEnv: environment i (0 when out of range). */
LightEnv * __attribute__((section(".text_80153E3C"))) lightGetEnv(s32 i)
{
    if (i < 4) {
        return &lightEnvironments[i];
    }
    return lightEnvironments;
}

/* ---- hand-written assembly (kept as the original instruction sequence) ---- */

#ifdef TM2_PORT_RENDER_SLOTS
/* PC port request 47 (render_slots.h, FAR_DISTANCE.md 3 "Matrix slots"): the port variant with more matrix slots
 * links the walker's pure-C form, the copy in reference/ that the all-C build proves (SOURCE_BUILD.md "All-C
 * flavour"), with the transform nodes' slot limit TM2_RENDER_NODE_SLOT_LIMIT in place of the asm's `slti 198`
 * (80152C18). It reads every slot array through the RenderState pointers renderInitState sets, so the moved arrays
 * need nothing else. The hybrid, the fixed source-only builds and every other variant keep the asm below. */
#define RENDER_WALKER_NODE_SLOTS TM2_RENDER_NODE_SLOT_LIMIT
#define TM2_RENDER_WALKER_ONLY
#include "../../reference/renderer_walker_engine_render_scene.c"
#else

/* renderPushMatrix (non-ABI, called from renderWalkModel): push the current node
 * transform (a0, t1..t6) on the scratchpad stack (t8, 32 entries of 28
 * bytes) or, past that, on the overflow stack (t9, up to 200); t7 = depth. */
__asm__(
    ".section .text_80152664,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderPushMatrix\n"
    ".type renderPushMatrix,@function\n"
    "renderPushMatrix:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tsltiu $3,$15,32\n"   /* 80152664: 2de30020 */
    "\tbnez $3,.L801526A4\n"   /* 80152668: 1460000e */
    "\tnop\n"   /* 8015266C: 00000000 */
    "\tsw $4,0($25)\n"   /* 80152670: af240000 */
    "\tsw $9,4($25)\n"   /* 80152674: af290004 */
    "\tsw $11,8($25)\n"   /* 80152678: af2b0008 */
    "\tsw $12,12($25)\n"   /* 8015267C: af2c000c */
    "\tsw $13,16($25)\n"   /* 80152680: af2d0010 */
    "\tsw $10,20($25)\n"   /* 80152684: af2a0014 */
    "\tsw $14,24($25)\n"   /* 80152688: af2e0018 */
    "\tsltiu $3,$15,200\n"   /* 8015268C: 2de300c8 */
    "\tbnez $3,.L801526C4\n"   /* 80152690: 1460000c */
    "\taddiu $25,$25,28\n"   /* 80152694: 2739001c */
    "\taddiu $25,$25,-28\n"   /* 80152698: 2739ffe4 */
    "\tj .L801526C4\n"   /* 8015269C: 080549b1 */
    "\taddiu $15,$15,-1\n"   /* 801526A0: 25efffff */
    ".L801526A4:\n"
    "\tsw $4,0($24)\n"   /* 801526A4: af040000 */
    "\tsw $9,4($24)\n"   /* 801526A8: af090004 */
    "\tsw $11,8($24)\n"   /* 801526AC: af0b0008 */
    "\tsw $12,12($24)\n"   /* 801526B0: af0c000c */
    "\tsw $13,16($24)\n"   /* 801526B4: af0d0010 */
    "\tsw $10,20($24)\n"   /* 801526B8: af0a0014 */
    "\tsw $14,24($24)\n"   /* 801526BC: af0e0018 */
    "\taddiu $24,$24,28\n"   /* 801526C0: 2718001c */
    ".L801526C4:\n"
    "\tjr $31\n"   /* 801526C4: 03e00008 */
    "\taddiu $15,$15,1\n"   /* 801526C8: 25ef0001 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderPushMatrix,.-renderPushMatrix\n");

/* renderWalkModel(obj, model): model tree walker. Saves s0-s7/sp/gp/a2/a3/ra
 * in the scratchpad record, then walks the node list: node type byte
 * selects a handler from the table at state+0x2C (renderInitNodeHandlers); handlers
 * cull against the view, compose matrices (renderMulTransMatVec), load the GTE
 * rotation/translation and queue meshes for sub_800CDC54. */
__asm__(
    ".section .text_801526CC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderWalkModel\n"
    ".type renderWalkModel,@function\n"
    "renderWalkModel:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tlui $8,0x1f80\n"   /* 801526CC: 3c081f80 */
    "\tsw $16,76($8)\n"   /* 801526D0: ad10004c */
    "\tsw $17,80($8)\n"   /* 801526D4: ad110050 */
    "\tsw $18,84($8)\n"   /* 801526D8: ad120054 */
    "\tsw $19,88($8)\n"   /* 801526DC: ad130058 */
    "\tsw $20,92($8)\n"   /* 801526E0: ad14005c */
    "\tsw $21,96($8)\n"   /* 801526E4: ad150060 */
    "\tsw $22,100($8)\n"   /* 801526E8: ad160064 */
    "\tsw $23,104($8)\n"   /* 801526EC: ad170068 */
    "\tsw $29,108($8)\n"   /* 801526F0: ad1d006c */
    "\tsw $28,112($8)\n"   /* 801526F4: ad1c0070 */
    "\tsw $6,116($8)\n"   /* 801526F8: ad060074 */
    "\tsw $7,120($8)\n"   /* 801526FC: ad070078 */
    "\tmove $17,$8\n"   /* 80152700: 01008821 */
    "\taddiu $24,$17,124\n"   /* 80152704: 2638007c */
    "\tlw $25,0($17)\n"   /* 80152708: 8e390000 */
    "\tlw $16,32($17)\n"   /* 8015270C: 8e300020 */
    "\tsw $31,64($17)\n"   /* 80152710: ae3f0040 */
    "\tsw $4,48($17)\n"   /* 80152714: ae240030 */
    "\tlw $18,24($17)\n"   /* 80152718: 8e320018 */
    "\tlw $23,8($17)\n"   /* 8015271C: 8e370008 */
    "\taddiu $20,$4,32\n"   /* 80152720: 24940020 */
    "\tsll $19,$23,0x5\n"   /* 80152724: 00179940 */
    "\taddu $18,$18,$19\n"   /* 80152728: 02539021 */
    "\tli $21,0x8\n"   /* 8015272C: 34150008 */
    "\tmove $6,$18\n"   /* 80152730: 02403021 */
    "\tmove $7,$20\n"   /* 80152734: 02803821 */
    ".L80152738:\n"
    "\tblez $21,.L80152754\n"   /* 80152738: 1aa00006 */
    "\tlw $22,0($7)\n"   /* 8015273C: 8cf60000 */
    "\taddiu $7,$7,4\n"   /* 80152740: 24e70004 */
    "\tsw $22,0($6)\n"   /* 80152744: acd60000 */
    "\taddiu $21,$21,-1\n"   /* 80152748: 26b5ffff */
    "\tj .L80152738\n"   /* 8015274C: 080549ce */
    "\taddiu $6,$6,4\n"   /* 80152750: 24c60004 */
    ".L80152754:\n"
    "\tlw $18,4($17)\n"   /* 80152754: 8e320004 */
    "\tlw $21,28($17)\n"   /* 80152758: 8e35001c */
    "\tsll $2,$23,0x4\n"   /* 8015275C: 00171100 */
    "\taddu $22,$18,$2\n"   /* 80152760: 0242b021 */
    "\taddu $21,$21,$19\n"   /* 80152764: 02b3a821 */
    "\tsw $20,0($22)\n"   /* 80152768: aed40000 */
    "\tsw $21,4($22)\n"   /* 8015276C: aed50004 */
    "\taddiu $20,$4,64\n"   /* 80152770: 24940040 */
    "\taddiu $21,$4,96\n"   /* 80152774: 24950060 */
    "\tsw $20,8($22)\n"   /* 80152778: aed40008 */
    "\tsw $21,12($22)\n"   /* 8015277C: aed5000c */
    "\tmove $8,$5\n"   /* 80152780: 00a04021 */
    "\tmove $9,$23\n"   /* 80152784: 02e04821 */
    "\tlw $10,52($17)\n"   /* 80152788: 8e2a0034 */
    "\tlw $11,20($4)\n"   /* 8015278C: 8c8b0014 */
    "\tlw $12,24($4)\n"   /* 80152790: 8c8c0018 */
    "\tlw $13,28($4)\n"   /* 80152794: 8c8d001c */
    "\tlw $14,56($17)\n"   /* 80152798: 8e2e0038 */
    "\tj .L80152804\n"   /* 8015279C: 08054a01 */
    "\tmove $15,$0\n"   /* 801527A0: 00007821 */
    ".L801527A4:\n"
    "\taddiu $15,$15,-1\n"   /* 801527A4: 25efffff */
    "\tbltz $15,.L801527FC\n"   /* 801527A8: 05e00014 */
    "\tsltiu $18,$15,32\n"   /* 801527AC: 2df20020 */
    "\tbeqz $18,.L801527DC\n"   /* 801527B0: 1240000a */
    "\tnop\n"   /* 801527B4: 00000000 */
    "\taddiu $24,$24,-28\n"   /* 801527B8: 2718ffe4 */
    "\tlw $8,0($24)\n"   /* 801527BC: 8f080000 */
    "\tlw $9,4($24)\n"   /* 801527C0: 8f090004 */
    "\tlw $11,8($24)\n"   /* 801527C4: 8f0b0008 */
    "\tlw $12,12($24)\n"   /* 801527C8: 8f0c000c */
    "\tlw $13,16($24)\n"   /* 801527CC: 8f0d0010 */
    "\tlw $10,20($24)\n"   /* 801527D0: 8f0a0014 */
    "\tj .L801527FC\n"   /* 801527D4: 080549ff */
    "\tlw $14,24($24)\n"   /* 801527D8: 8f0e0018 */
    ".L801527DC:\n"
    "\taddiu $25,$25,-28\n"   /* 801527DC: 2739ffe4 */
    "\tlw $8,0($25)\n"   /* 801527E0: 8f280000 */
    "\tlw $9,4($25)\n"   /* 801527E4: 8f290004 */
    "\tlw $11,8($25)\n"   /* 801527E8: 8f2b0008 */
    "\tlw $12,12($25)\n"   /* 801527EC: 8f2c000c */
    "\tlw $13,16($25)\n"   /* 801527F0: 8f2d0010 */
    "\tlw $10,20($25)\n"   /* 801527F4: 8f2a0014 */
    "\tlw $14,24($25)\n"   /* 801527F8: 8f2e0018 */
    ".L801527FC:\n"
    "\tbltz $15,.L80153348\n"   /* 801527FC: 05e002d2 */
    "\tnop\n"   /* 80152800: 00000000 */
    ".L80152804:\n"
    "\tlbu $18,0($8)\n"   /* 80152804: 91120000 */
    "\tlw $19,44($17)\n"   /* 80152808: 8e33002c */
    "\tsltiu $20,$18,13\n"   /* 8015280C: 2e54000d */
    "\tbeqz $20,.L801527A4\n"   /* 80152810: 1280ffe4 */
    "\tsll $18,$18,0x2\n"   /* 80152814: 00129080 */
    "\taddu $18,$19,$18\n"   /* 80152818: 02729021 */
    "\tlw $18,0($18)\n"   /* 8015281C: 8e520000 */
    "\tnop\n"   /* 80152820: 00000000 */
    "\tjr $18\n"   /* 80152824: 02400008 */
    "\tnop\n"   /* 80152828: 00000000 */
    "\tlh $18,24($8)\n"   /* 8015282C: 85120018 */
    "\tnop\n"   /* 80152830: 00000000 */
    "\tandi $18,$18,0x2000\n"   /* 80152834: 32522000 */
    "\tbnez $18,.L801528E4\n"   /* 80152838: 1640002a */
    "\tli $18,0x1\n"   /* 8015283C: 34120001 */
    "\tlw $19,40($8)\n"   /* 80152840: 8d130028 */
    "\taddiu $20,$8,28\n"   /* 80152844: 2514001c */
    "\tlw $4,40($17)\n"   /* 80152848: 8e240028 */
    "\tlw $21,0($20)\n"   /* 8015284C: 8e950000 */
    "\tli $18,0x1\n"   /* 80152850: 34120001 */
    "\tbnez $4,.L801528E4\n"   /* 80152854: 14800023 */
    "\tlw $22,4($20)\n"   /* 80152858: 8e960004 */
    "\tlw $23,8($20)\n"   /* 8015285C: 8e970008 */
    "\taddu $21,$11,$21\n"   /* 80152860: 0175a821 */
    "\taddu $22,$12,$22\n"   /* 80152864: 0196b021 */
    "\tlw $4,4($17)\n"   /* 80152868: 8e240004 */
    "\taddu $23,$13,$23\n"   /* 8015286C: 01b7b821 */
    "\tsll $5,$9,0x4\n"   /* 80152870: 00092900 */
    "\taddu $4,$4,$5\n"   /* 80152874: 00852021 */
    "\tlw $5,8($4)\n"   /* 80152878: 8c850008 */
    "\tnegu $19,$19\n"   /* 8015287C: 00139823 */
    "\tjal renderMulTransMatVec\n"   /* 80152880: 0c054ce3 */
    "\tnop\n"   /* 80152884: 00000000 */
    "\tslt $6,$6,$19\n"   /* 80152888: 00d3302a */
    "\tbnez $6,.L801528E0\n"   /* 8015288C: 14c00014 */
    "\tslt $7,$7,$19\n"   /* 80152890: 00f3382a */
    "\tbnez $7,.L801528E0\n"   /* 80152894: 14e00012 */
    "\tslt $20,$20,$19\n"   /* 80152898: 0293a02a */
    "\tbnez $20,.L801528E0\n"   /* 8015289C: 16800010 */
    "\tlw $5,12($4)\n"   /* 801528A0: 8c85000c */
    "\tnop\n"   /* 801528A4: 00000000 */
    "\tjal renderMulTransMatVec\n"   /* 801528A8: 0c054ce3 */
    "\tnop\n"   /* 801528AC: 00000000 */
    "\tslt $6,$6,$19\n"   /* 801528B0: 00d3302a */
    "\tbnez $6,.L801528E0\n"   /* 801528B4: 14c0000a */
    "\tslt $7,$7,$19\n"   /* 801528B8: 00f3382a */
    "\tbnez $7,.L801528E0\n"   /* 801528BC: 14e00008 */
    "\tlw $5,16($17)\n"   /* 801528C0: 8e250010 */
    "\tnop\n"   /* 801528C4: 00000000 */
    "\tsllv $5,$5,$14\n"   /* 801528C8: 01c52804 */
    "\tnegu $5,$5\n"   /* 801528CC: 00052823 */
    "\taddu $5,$5,$19\n"   /* 801528D0: 00b32821 */
    "\tslt $5,$20,$5\n"   /* 801528D4: 0285282a */
    "\tbeqz $5,.L801528E4\n"   /* 801528D8: 10a00002 */
    "\tnop\n"   /* 801528DC: 00000000 */
    ".L801528E0:\n"
    "\tmove $18,$0\n"   /* 801528E0: 00009021 */
    ".L801528E4:\n"
    "\tlw $19,60($17)\n"   /* 801528E4: 8e33003c */
    "\tbeqz $18,.L801527A4\n"   /* 801528E8: 1240ffae */
    "\tslti $20,$19,280\n"   /* 801528EC: 2a740118 */
    "\tbeqz $20,.L801527A4\n"   /* 801528F0: 1280ffac */
    "\tlw $18,48($17)\n"   /* 801528F4: 8e320030 */
    "\tsw $8,0($16)\n"   /* 801528F8: ae080000 */
    "\tsw $11,4($16)\n"   /* 801528FC: ae0b0004 */
    "\tsw $12,8($16)\n"   /* 80152900: ae0c0008 */
    "\tsw $13,12($16)\n"   /* 80152904: ae0d000c */
    "\tsw $14,16($16)\n"   /* 80152908: ae0e0010 */
    "\tsw $18,20($16)\n"   /* 8015290C: ae120014 */
    "\tsw $9,24($16)\n"   /* 80152910: ae090018 */
    "\tsw $10,28($16)\n"   /* 80152914: ae0a001c */
    "\taddiu $19,$19,1\n"   /* 80152918: 26730001 */
    "\tsw $19,60($17)\n"   /* 8015291C: ae33003c */
    "\tj .L801527A4\n"   /* 80152920: 080549e9 */
    "\taddiu $16,$16,32\n"   /* 80152924: 26100020 */
    "\tlbu $18,21($8)\n"   /* 80152928: 91120015 */
    "\tnop\n"   /* 8015292C: 00000000 */
    "\tandi $19,$18,0x2\n"   /* 80152930: 32530002 */
    "\tbnez $19,.L801527A4\n"   /* 80152934: 1660ff9b */
    "\tnop\n"   /* 80152938: 00000000 */
    "\tandi $19,$18,0x4\n"   /* 8015293C: 32530004 */
    "\tbnez $19,.L801529EC\n"   /* 80152940: 1660002a */
    "\tli $18,0x1\n"   /* 80152944: 34120001 */
    "\tlw $19,16($8)\n"   /* 80152948: 8d130010 */
    "\taddiu $20,$8,4\n"   /* 8015294C: 25140004 */
    "\tlw $4,40($17)\n"   /* 80152950: 8e240028 */
    "\tlw $21,0($20)\n"   /* 80152954: 8e950000 */
    "\tli $18,0x1\n"   /* 80152958: 34120001 */
    "\tbnez $4,.L801529EC\n"   /* 8015295C: 14800023 */
    "\tlw $22,4($20)\n"   /* 80152960: 8e960004 */
    "\tlw $23,8($20)\n"   /* 80152964: 8e970008 */
    "\taddu $21,$11,$21\n"   /* 80152968: 0175a821 */
    "\taddu $22,$12,$22\n"   /* 8015296C: 0196b021 */
    "\tlw $4,4($17)\n"   /* 80152970: 8e240004 */
    "\taddu $23,$13,$23\n"   /* 80152974: 01b7b821 */
    "\tsll $5,$9,0x4\n"   /* 80152978: 00092900 */
    "\taddu $4,$4,$5\n"   /* 8015297C: 00852021 */
    "\tlw $5,8($4)\n"   /* 80152980: 8c850008 */
    "\tnegu $19,$19\n"   /* 80152984: 00139823 */
    "\tjal renderMulTransMatVec\n"   /* 80152988: 0c054ce3 */
    "\tnop\n"   /* 8015298C: 00000000 */
    "\tslt $6,$6,$19\n"   /* 80152990: 00d3302a */
    "\tbnez $6,.L801529E8\n"   /* 80152994: 14c00014 */
    "\tslt $7,$7,$19\n"   /* 80152998: 00f3382a */
    "\tbnez $7,.L801529E8\n"   /* 8015299C: 14e00012 */
    "\tslt $20,$20,$19\n"   /* 801529A0: 0293a02a */
    "\tbnez $20,.L801529E8\n"   /* 801529A4: 16800010 */
    "\tlw $5,12($4)\n"   /* 801529A8: 8c85000c */
    "\tnop\n"   /* 801529AC: 00000000 */
    "\tjal renderMulTransMatVec\n"   /* 801529B0: 0c054ce3 */
    "\tnop\n"   /* 801529B4: 00000000 */
    "\tslt $6,$6,$19\n"   /* 801529B8: 00d3302a */
    "\tbnez $6,.L801529E8\n"   /* 801529BC: 14c0000a */
    "\tslt $7,$7,$19\n"   /* 801529C0: 00f3382a */
    "\tbnez $7,.L801529E8\n"   /* 801529C4: 14e00008 */
    "\tlw $5,16($17)\n"   /* 801529C8: 8e250010 */
    "\tnop\n"   /* 801529CC: 00000000 */
    "\tsllv $5,$5,$14\n"   /* 801529D0: 01c52804 */
    "\tnegu $5,$5\n"   /* 801529D4: 00052823 */
    "\taddu $5,$5,$19\n"   /* 801529D8: 00b32821 */
    "\tslt $5,$20,$5\n"   /* 801529DC: 0285282a */
    "\tbeqz $5,.L801529EC\n"   /* 801529E0: 10a00002 */
    "\tnop\n"   /* 801529E4: 00000000 */
    ".L801529E8:\n"
    "\tmove $18,$0\n"   /* 801529E8: 00009021 */
    ".L801529EC:\n"
    "\tbeqz $18,.L801527A4\n"   /* 801529EC: 1240ff6d */
    "\tlbu $18,20($8)\n"   /* 801529F0: 91120014 */
    "\taddiu $19,$8,24\n"   /* 801529F4: 25130018 */
    ".L801529F8:\n"
    "\tblez $18,.L801527A4\n"   /* 801529F8: 1a40ff6a */
    "\tlw $4,0($19)\n"   /* 801529FC: 8e640000 */
    "\taddiu $18,$18,-1\n"   /* 80152A00: 2652ffff */
    "\tjal renderPushMatrix\n"   /* 80152A04: 0c054999 */
    "\tnop\n"   /* 80152A08: 00000000 */
    "\tj .L801529F8\n"   /* 80152A0C: 08054a7e */
    "\taddiu $19,$19,4\n"   /* 80152A10: 26730004 */
    "\tlw $18,4($8)\n"   /* 80152A14: 8d120004 */
    "\tlw $19,8($8)\n"   /* 80152A18: 8d130008 */
    "\tlw $20,12($8)\n"   /* 80152A1C: 8d14000c */
    "\taddu $18,$18,$11\n"   /* 80152A20: 024b9021 */
    "\taddu $19,$19,$12\n"   /* 80152A24: 026c9821 */
    "\taddu $20,$20,$13\n"   /* 80152A28: 028da021 */
    "\tsrav $18,$18,$14\n"   /* 80152A2C: 01d29007 */
    "\tsrav $19,$19,$14\n"   /* 80152A30: 01d39807 */
    "\tsrav $20,$20,$14\n"   /* 80152A34: 01d4a007 */
    "\tmtc2 $18,$9\n"   /* 80152A38: 48924800 */
    "\tmtc2 $19,$10\n"   /* 80152A3C: 48935000 */
    "\tmtc2 $20,$11\n"   /* 80152A40: 48945800 */
    "\t.word 0x4aa00428\n"   /* 80152A44: 4aa00428 */
    "\tlw $19,16($8)\n"   /* 80152A48: 8d130010 */
    "\tlw $22,36($17)\n"   /* 80152A4C: 8e360024 */
    "\taddiu $19,$19,1\n"   /* 80152A50: 26730001 */
    "\tmfc2 $18,$25\n"   /* 80152A54: 4812c800 */
    "\tmfc2 $21,$26\n"   /* 80152A58: 4815d000 */
    "\tmfc2 $20,$27\n"   /* 80152A5C: 4814d800 */
    "\taddu $18,$18,$21\n"   /* 80152A60: 02559021 */
    "\taddu $18,$18,$20\n"   /* 80152A64: 02549021 */
    "\tbeq $0,$22,.L80152A74\n"   /* 80152A68: 10160002 */
    "\taddiu $20,$8,20\n"   /* 80152A6C: 25140014 */
    "\tli $18,0x64\n"   /* 80152A70: 34120064 */
    ".L80152A74:\n"
    "\taddiu $19,$19,-1\n"   /* 80152A74: 2673ffff */
    "\tblez $19,.L80152AB4\n"   /* 80152A78: 1a60000e */
    "\tnop\n"   /* 80152A7C: 00000000 */
    "\tlw $23,0($20)\n"   /* 80152A80: 8e970000 */
    "\tnop\n"   /* 80152A84: 00000000 */
    "\tlw $21,4($23)\n"   /* 80152A88: 8ef50004 */
    "\tlw $22,0($23)\n"   /* 80152A8C: 8ef60000 */
    "\tsltu $21,$18,$21\n"   /* 80152A90: 0255a82b */
    "\tbnez $21,.L80152A74\n"   /* 80152A94: 16a0fff7 */
    "\taddiu $20,$20,4\n"   /* 80152A98: 26940004 */
    "\tsltu $22,$18,$22\n"   /* 80152A9C: 0256b02b */
    "\tbeqz $22,.L80152A74\n"   /* 80152AA0: 12c0fff4 */
    "\tnop\n"   /* 80152AA4: 00000000 */
    "\tlw $4,8($23)\n"   /* 80152AA8: 8ee40008 */
    "\tjal renderPushMatrix\n"   /* 80152AAC: 0c054999 */
    "\tnop\n"   /* 80152AB0: 00000000 */
    ".L80152AB4:\n"
    "\tj .L801527A4\n"   /* 80152AB4: 080549e9 */
    "\tnop\n"   /* 80152AB8: 00000000 */
    "\tlbu $18,19($8)\n"   /* 80152ABC: 91120013 */
    "\tnop\n"   /* 80152AC0: 00000000 */
    "\tandi $18,$18,0x2\n"   /* 80152AC4: 32520002 */
    "\tbnez $18,.L801527A4\n"   /* 80152AC8: 1640ff36 */
    "\tlw $18,4($8)\n"   /* 80152ACC: 8d120004 */
    "\tlw $19,8($8)\n"   /* 80152AD0: 8d130008 */
    "\tlw $20,12($8)\n"   /* 80152AD4: 8d14000c */
    "\tlw $21,52($17)\n"   /* 80152AD8: 8e350034 */
    "\taddu $11,$11,$18\n"   /* 80152ADC: 01725821 */
    "\taddu $12,$12,$19\n"   /* 80152AE0: 01936021 */
    "\taddu $13,$13,$20\n"   /* 80152AE4: 01b46821 */
    "\taddiu $19,$8,20\n"   /* 80152AE8: 25130014 */
    "\tlbu $20,18($8)\n"   /* 80152AEC: 91140012 */
    "\taddiu $21,$21,1\n"   /* 80152AF0: 26b50001 */
    "\tsw $21,52($17)\n"   /* 80152AF4: ae350034 */
    "\tmove $10,$21\n"   /* 80152AF8: 02a05021 */
    ".L80152AFC:\n"
    "\tblez $20,.L801527A4\n"   /* 80152AFC: 1a80ff29 */
    "\tlw $4,0($19)\n"   /* 80152B00: 8e640000 */
    "\taddiu $20,$20,-1\n"   /* 80152B04: 2694ffff */
    "\tjal renderPushMatrix\n"   /* 80152B08: 0c054999 */
    "\tnop\n"   /* 80152B0C: 00000000 */
    "\tj .L80152AFC\n"   /* 80152B10: 08054abf */
    "\taddiu $19,$19,4\n"   /* 80152B14: 26730004 */
    "\tlbu $18,39($8)\n"   /* 80152B18: 91120027 */
    "\tnop\n"   /* 80152B1C: 00000000 */
    "\tandi $18,$18,0x2\n"   /* 80152B20: 32520002 */
    "\tbnez $18,.L801527A4\n"   /* 80152B24: 1640ff1f */
    "\tlw $18,24($8)\n"   /* 80152B28: 8d120018 */
    "\tlw $19,28($8)\n"   /* 80152B2C: 8d13001c */
    "\tlw $20,32($8)\n"   /* 80152B30: 8d140020 */
    "\taddu $18,$11,$18\n"   /* 80152B34: 01729021 */
    "\taddu $19,$12,$19\n"   /* 80152B38: 01939821 */
    "\taddu $20,$13,$20\n"   /* 80152B3C: 01b4a021 */
    "\taddiu $21,$8,4\n"   /* 80152B40: 25150004 */
    "\tlw $23,8($17)\n"   /* 80152B44: 8e370008 */
    "\tlh $11,0($21)\n"   /* 80152B48: 86ab0000 */
    "\tnop\n"   /* 80152B4C: 00000000 */
    "\tmult $11,$18\n"   /* 80152B50: 01720018 */
    "\tmflo $11\n"   /* 80152B54: 00005812 */
    "\tlh $22,2($21)\n"   /* 80152B58: 86b60002 */
    "\tnop\n"   /* 80152B5C: 00000000 */
    "\tmult $22,$19\n"   /* 80152B60: 02d30018 */
    "\tmflo $22\n"   /* 80152B64: 0000b012 */
    "\tadd $11,$22,$11\n"   /* 80152B68: 02cb5820 */
    "\tlh $22,4($21)\n"   /* 80152B6C: 86b60004 */
    "\tnop\n"   /* 80152B70: 00000000 */
    "\tmult $22,$20\n"   /* 80152B74: 02d40018 */
    "\tmflo $22\n"   /* 80152B78: 0000b012 */
    "\tadd $11,$11,$22\n"   /* 80152B7C: 01765820 */
    "\tbgez $11,.L80152B8C\n"   /* 80152B80: 05610002 */
    "\tnop\n"   /* 80152B84: 00000000 */
    "\taddiu $11,$11,4095\n"   /* 80152B88: 256b0fff */
    ".L80152B8C:\n"
    "\tlh $12,6($21)\n"   /* 80152B8C: 86ac0006 */
    "\tsra $11,$11,0xc\n"   /* 80152B90: 000b5b03 */
    "\tmult $12,$18\n"   /* 80152B94: 01920018 */
    "\tmflo $12\n"   /* 80152B98: 00006012 */
    "\tlh $22,8($21)\n"   /* 80152B9C: 86b60008 */
    "\tnop\n"   /* 80152BA0: 00000000 */
    "\tmult $22,$19\n"   /* 80152BA4: 02d30018 */
    "\tmflo $22\n"   /* 80152BA8: 0000b012 */
    "\tadd $12,$22,$12\n"   /* 80152BAC: 02cc6020 */
    "\tlh $22,10($21)\n"   /* 80152BB0: 86b6000a */
    "\tnop\n"   /* 80152BB4: 00000000 */
    "\tmult $22,$20\n"   /* 80152BB8: 02d40018 */
    "\tmflo $22\n"   /* 80152BBC: 0000b012 */
    "\tadd $12,$12,$22\n"   /* 80152BC0: 01966020 */
    "\tbgez $12,.L80152BD0\n"   /* 80152BC4: 05810002 */
    "\tnop\n"   /* 80152BC8: 00000000 */
    "\taddiu $12,$12,4095\n"   /* 80152BCC: 258c0fff */
    ".L80152BD0:\n"
    "\tlh $13,12($21)\n"   /* 80152BD0: 86ad000c */
    "\tsra $12,$12,0xc\n"   /* 80152BD4: 000c6303 */
    "\tmult $13,$18\n"   /* 80152BD8: 01b20018 */
    "\tmflo $13\n"   /* 80152BDC: 00006812 */
    "\tlh $22,14($21)\n"   /* 80152BE0: 86b6000e */
    "\tnop\n"   /* 80152BE4: 00000000 */
    "\tmult $22,$19\n"   /* 80152BE8: 02d30018 */
    "\tmflo $22\n"   /* 80152BEC: 0000b012 */
    "\tadd $13,$22,$13\n"   /* 80152BF0: 02cd6820 */
    "\tlh $22,16($21)\n"   /* 80152BF4: 86b60010 */
    "\tnop\n"   /* 80152BF8: 00000000 */
    "\tmult $22,$20\n"   /* 80152BFC: 02d40018 */
    "\tmflo $22\n"   /* 80152C00: 0000b012 */
    "\tadd $13,$13,$22\n"   /* 80152C04: 01b66820 */
    "\tbgez $13,.L80152C14\n"   /* 80152C08: 05a10002 */
    "\tnop\n"   /* 80152C0C: 00000000 */
    "\taddiu $13,$13,4095\n"   /* 80152C10: 25ad0fff */
    ".L80152C14:\n"
    "\tsra $13,$13,0xc\n"   /* 80152C14: 000d6b03 */
    "\tslti $18,$23,198\n"   /* 80152C18: 2af200c6 */
    "\tbeqz $18,.L80152F48\n"   /* 80152C1C: 124000ca */
    "\tlw $6,24($17)\n"   /* 80152C20: 8e260018 */
    "\taddiu $23,$23,1\n"   /* 80152C24: 26f70001 */
    "\tsw $23,8($17)\n"   /* 80152C28: ae370008 */
    "\tsll $7,$23,0x4\n"   /* 80152C2C: 00173900 */
    "\tsll $23,$23,0x5\n"   /* 80152C30: 0017b940 */
    "\tlw $3,4($17)\n"   /* 80152C34: 8e230004 */
    "\tsll $19,$9,0x4\n"   /* 80152C38: 00099900 */
    "\taddu $18,$3,$19\n"   /* 80152C3C: 00739021 */
    "\tlw $19,0($18)\n"   /* 80152C40: 8e530000 */
    "\taddu $3,$3,$7\n"   /* 80152C44: 00671821 */
    "\tlw $20,0($19)\n"   /* 80152C48: 8e740000 */
    "\tlw $22,4($19)\n"   /* 80152C4C: 8e760004 */
    "\tlw $4,8($19)\n"   /* 80152C50: 8e640008 */
    "\tlw $5,12($19)\n"   /* 80152C54: 8e65000c */
    "\tlw $2,16($19)\n"   /* 80152C58: 8e620010 */
    "\tctc2 $20,$0\n"   /* 80152C5C: 48d40000 */
    "\tctc2 $22,$1\n"   /* 80152C60: 48d60800 */
    "\tctc2 $4,$2\n"   /* 80152C64: 48c41000 */
    "\tctc2 $5,$3\n"   /* 80152C68: 48c51800 */
    "\tctc2 $2,$4\n"   /* 80152C6C: 48c22000 */
    "\taddu $6,$6,$23\n"   /* 80152C70: 00d73021 */
    "\tlwc2 $0,0($21)\n"   /* 80152C74: caa00000 */
    "\tlwc2 $1,4($21)\n"   /* 80152C78: caa10004 */
    "\tlhu $4,6($21)\n"   /* 80152C7C: 96a40006 */
    "\t.word 0x4a486012\n"   /* 80152C80: 4a486012 */
    "\tlhu $5,8($21)\n"   /* 80152C84: 96a50008 */
    "\tlhu $2,10($21)\n"   /* 80152C88: 96a2000a */
    "\tsll $5,$5,0x10\n"   /* 80152C8C: 00052c00 */
    "\tor $4,$4,$5\n"   /* 80152C90: 00852025 */
    "\tmfc2 $19,$9\n"   /* 80152C94: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152C98: 48145000 */
    "\tmfc2 $22,$11\n"   /* 80152C9C: 48165800 */
    "\tsh $19,0($6)\n"   /* 80152CA0: a4d30000 */
    "\tsh $20,6($6)\n"   /* 80152CA4: a4d40006 */
    "\tsh $22,12($6)\n"   /* 80152CA8: a4d6000c */
    "\tmtc2 $4,$0\n"   /* 80152CAC: 48840000 */
    "\tmtc2 $2,$1\n"   /* 80152CB0: 48820800 */
    "\tnop\n"   /* 80152CB4: 00000000 */
    "\t.word 0x4a486012\n"   /* 80152CB8: 4a486012 */
    "\tmfc2 $19,$9\n"   /* 80152CBC: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152CC0: 48145000 */
    "\tmfc2 $22,$11\n"   /* 80152CC4: 48165800 */
    "\tsh $19,2($6)\n"   /* 80152CC8: a4d30002 */
    "\tsh $20,8($6)\n"   /* 80152CCC: a4d40008 */
    "\tsh $22,14($6)\n"   /* 80152CD0: a4d6000e */
    "\tlwc2 $0,12($21)\n"   /* 80152CD4: caa0000c */
    "\tlwc2 $1,16($21)\n"   /* 80152CD8: caa10010 */
    "\tnop\n"   /* 80152CDC: 00000000 */
    "\t.word 0x4a486012\n"   /* 80152CE0: 4a486012 */
    "\tmfc2 $19,$9\n"   /* 80152CE4: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152CE8: 48145000 */
    "\tsh $19,4($6)\n"   /* 80152CEC: a4d30004 */
    "\tsh $20,10($6)\n"   /* 80152CF0: a4d4000a */
    "\tswc2 $11,16($6)\n"   /* 80152CF4: e8cb0010 */
    "\tsw $6,0($3)\n"   /* 80152CF8: ac660000 */
    "\tlw $19,4($18)\n"   /* 80152CFC: 8e530004 */
    "\tlw $6,28($17)\n"   /* 80152D00: 8e26001c */
    "\tlw $20,0($19)\n"   /* 80152D04: 8e740000 */
    "\tlw $22,4($19)\n"   /* 80152D08: 8e760004 */
    "\tlw $4,8($19)\n"   /* 80152D0C: 8e640008 */
    "\tlw $5,12($19)\n"   /* 80152D10: 8e65000c */
    "\tlw $2,16($19)\n"   /* 80152D14: 8e620010 */
    "\tctc2 $20,$0\n"   /* 80152D18: 48d40000 */
    "\tctc2 $22,$1\n"   /* 80152D1C: 48d60800 */
    "\tctc2 $4,$2\n"   /* 80152D20: 48c41000 */
    "\tctc2 $5,$3\n"   /* 80152D24: 48c51800 */
    "\tctc2 $2,$4\n"   /* 80152D28: 48c22000 */
    "\taddu $6,$6,$23\n"   /* 80152D2C: 00d73021 */
    "\tlwc2 $0,0($21)\n"   /* 80152D30: caa00000 */
    "\tlwc2 $1,4($21)\n"   /* 80152D34: caa10004 */
    "\tlhu $4,6($21)\n"   /* 80152D38: 96a40006 */
    "\t.word 0x4a486012\n"   /* 80152D3C: 4a486012 */
    "\tlhu $5,8($21)\n"   /* 80152D40: 96a50008 */
    "\tlhu $2,10($21)\n"   /* 80152D44: 96a2000a */
    "\tsll $5,$5,0x10\n"   /* 80152D48: 00052c00 */
    "\tor $4,$4,$5\n"   /* 80152D4C: 00852025 */
    "\tmfc2 $19,$9\n"   /* 80152D50: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152D54: 48145000 */
    "\tmfc2 $22,$11\n"   /* 80152D58: 48165800 */
    "\tsh $19,0($6)\n"   /* 80152D5C: a4d30000 */
    "\tsh $20,6($6)\n"   /* 80152D60: a4d40006 */
    "\tsh $22,12($6)\n"   /* 80152D64: a4d6000c */
    "\tmtc2 $4,$0\n"   /* 80152D68: 48840000 */
    "\tmtc2 $2,$1\n"   /* 80152D6C: 48820800 */
    "\tnop\n"   /* 80152D70: 00000000 */
    "\t.word 0x4a486012\n"   /* 80152D74: 4a486012 */
    "\tmfc2 $19,$9\n"   /* 80152D78: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152D7C: 48145000 */
    "\tmfc2 $22,$11\n"   /* 80152D80: 48165800 */
    "\tsh $19,2($6)\n"   /* 80152D84: a4d30002 */
    "\tsh $20,8($6)\n"   /* 80152D88: a4d40008 */
    "\tsh $22,14($6)\n"   /* 80152D8C: a4d6000e */
    "\tlwc2 $0,12($21)\n"   /* 80152D90: caa0000c */
    "\tlwc2 $1,16($21)\n"   /* 80152D94: caa10010 */
    "\tnop\n"   /* 80152D98: 00000000 */
    "\t.word 0x4a486012\n"   /* 80152D9C: 4a486012 */
    "\tmfc2 $19,$9\n"   /* 80152DA0: 48134800 */
    "\tmfc2 $20,$10\n"   /* 80152DA4: 48145000 */
    "\tsh $19,4($6)\n"   /* 80152DA8: a4d30004 */
    "\tsh $20,10($6)\n"   /* 80152DAC: a4d4000a */
    "\tswc2 $11,16($6)\n"   /* 80152DB0: e8cb0010 */
    "\tsw $6,4($3)\n"   /* 80152DB4: ac660004 */
    "\tlw $20,0($21)\n"   /* 80152DB8: 8eb40000 */
    "\tlw $22,4($21)\n"   /* 80152DBC: 8eb60004 */
    "\tlw $4,8($21)\n"   /* 80152DC0: 8ea40008 */
    "\tlw $5,12($21)\n"   /* 80152DC4: 8ea5000c */
    "\tlw $2,16($21)\n"   /* 80152DC8: 8ea20010 */
    "\tctc2 $20,$0\n"   /* 80152DCC: 48d40000 */
    "\tctc2 $22,$1\n"   /* 80152DD0: 48d60800 */
    "\tctc2 $4,$2\n"   /* 80152DD4: 48c41000 */
    "\tctc2 $5,$3\n"   /* 80152DD8: 48c51800 */
    "\tctc2 $2,$4\n"   /* 80152DDC: 48c22000 */
    "\tlw $6,68($17)\n"   /* 80152DE0: 8e260044 */
    "\tlw $19,8($18)\n"   /* 80152DE4: 8e530008 */
    "\taddu $6,$6,$23\n"   /* 80152DE8: 00d73021 */
    "\tlhu $20,0($19)\n"   /* 80152DEC: 96740000 */
    "\tlhu $22,6($19)\n"   /* 80152DF0: 96760006 */
    "\tlw $4,12($19)\n"   /* 80152DF4: 8e64000c */
    "\tsll $22,$22,0x10\n"   /* 80152DF8: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152DFC: 0296a025 */
    "\tmtc2 $20,$0\n"   /* 80152E00: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152E04: 48840800 */
    "\tlhu $20,2($19)\n"   /* 80152E08: 96740002 */
    "\t.word 0x4a486012\n"   /* 80152E0C: 4a486012 */
    "\tlhu $22,8($19)\n"   /* 80152E10: 96760008 */
    "\tlhu $4,14($19)\n"   /* 80152E14: 9664000e */
    "\tsll $22,$22,0x10\n"   /* 80152E18: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152E1C: 0296a025 */
    "\tmfc2 $5,$9\n"   /* 80152E20: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152E24: 48025000 */
    "\tmfc2 $21,$11\n"   /* 80152E28: 48155800 */
    "\tsh $5,0($6)\n"   /* 80152E2C: a4c50000 */
    "\tsh $2,6($6)\n"   /* 80152E30: a4c20006 */
    "\tmtc2 $20,$0\n"   /* 80152E34: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152E38: 48840800 */
    "\tsh $21,12($6)\n"   /* 80152E3C: a4d5000c */
    "\t.word 0x4a486012\n"   /* 80152E40: 4a486012 */
    "\tlhu $20,4($19)\n"   /* 80152E44: 96740004 */
    "\tlhu $22,10($19)\n"   /* 80152E48: 9676000a */
    "\tlw $4,16($19)\n"   /* 80152E4C: 8e640010 */
    "\tsll $22,$22,0x10\n"   /* 80152E50: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152E54: 0296a025 */
    "\tmfc2 $5,$9\n"   /* 80152E58: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152E5C: 48025000 */
    "\tmfc2 $21,$11\n"   /* 80152E60: 48155800 */
    "\tsh $5,2($6)\n"   /* 80152E64: a4c50002 */
    "\tsh $2,8($6)\n"   /* 80152E68: a4c20008 */
    "\tmtc2 $20,$0\n"   /* 80152E6C: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152E70: 48840800 */
    "\tsh $21,14($6)\n"   /* 80152E74: a4d5000e */
    "\t.word 0x4a486012\n"   /* 80152E78: 4a486012 */
    "\tmfc2 $5,$9\n"   /* 80152E7C: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152E80: 48025000 */
    "\tsh $5,4($6)\n"   /* 80152E84: a4c50004 */
    "\tsh $2,10($6)\n"   /* 80152E88: a4c2000a */
    "\tswc2 $11,16($6)\n"   /* 80152E8C: e8cb0010 */
    "\tsw $6,8($3)\n"   /* 80152E90: ac660008 */
    "\tlw $6,72($17)\n"   /* 80152E94: 8e260048 */
    "\tlw $19,12($18)\n"   /* 80152E98: 8e53000c */
    "\taddu $6,$6,$23\n"   /* 80152E9C: 00d73021 */
    "\tlhu $20,0($19)\n"   /* 80152EA0: 96740000 */
    "\tlhu $22,6($19)\n"   /* 80152EA4: 96760006 */
    "\tlw $4,12($19)\n"   /* 80152EA8: 8e64000c */
    "\tsll $22,$22,0x10\n"   /* 80152EAC: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152EB0: 0296a025 */
    "\tmtc2 $20,$0\n"   /* 80152EB4: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152EB8: 48840800 */
    "\tlhu $20,2($19)\n"   /* 80152EBC: 96740002 */
    "\t.word 0x4a486012\n"   /* 80152EC0: 4a486012 */
    "\tlhu $22,8($19)\n"   /* 80152EC4: 96760008 */
    "\tlhu $4,14($19)\n"   /* 80152EC8: 9664000e */
    "\tsll $22,$22,0x10\n"   /* 80152ECC: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152ED0: 0296a025 */
    "\tmfc2 $5,$9\n"   /* 80152ED4: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152ED8: 48025000 */
    "\tmfc2 $21,$11\n"   /* 80152EDC: 48155800 */
    "\tsh $5,0($6)\n"   /* 80152EE0: a4c50000 */
    "\tsh $2,6($6)\n"   /* 80152EE4: a4c20006 */
    "\tmtc2 $20,$0\n"   /* 80152EE8: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152EEC: 48840800 */
    "\tsh $21,12($6)\n"   /* 80152EF0: a4d5000c */
    "\t.word 0x4a486012\n"   /* 80152EF4: 4a486012 */
    "\tlhu $20,4($19)\n"   /* 80152EF8: 96740004 */
    "\tlhu $22,10($19)\n"   /* 80152EFC: 9676000a */
    "\tlw $4,16($19)\n"   /* 80152F00: 8e640010 */
    "\tsll $22,$22,0x10\n"   /* 80152F04: 0016b400 */
    "\tor $20,$20,$22\n"   /* 80152F08: 0296a025 */
    "\tmfc2 $5,$9\n"   /* 80152F0C: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152F10: 48025000 */
    "\tmfc2 $21,$11\n"   /* 80152F14: 48155800 */
    "\tsh $5,2($6)\n"   /* 80152F18: a4c50002 */
    "\tsh $2,8($6)\n"   /* 80152F1C: a4c20008 */
    "\tmtc2 $20,$0\n"   /* 80152F20: 48940000 */
    "\tmtc2 $4,$1\n"   /* 80152F24: 48840800 */
    "\tsh $21,14($6)\n"   /* 80152F28: a4d5000e */
    "\t.word 0x4a486012\n"   /* 80152F2C: 4a486012 */
    "\tmfc2 $5,$9\n"   /* 80152F30: 48054800 */
    "\tmfc2 $2,$10\n"   /* 80152F34: 48025000 */
    "\tsh $5,4($6)\n"   /* 80152F38: a4c50004 */
    "\tsh $2,10($6)\n"   /* 80152F3C: a4c2000a */
    "\tswc2 $11,16($6)\n"   /* 80152F40: e8cb0010 */
    "\tsw $6,12($3)\n"   /* 80152F44: ac66000c */
    ".L80152F48:\n"
    "\tlw $21,52($17)\n"   /* 80152F48: 8e350034 */
    "\tlbu $20,38($8)\n"   /* 80152F4C: 91140026 */
    "\taddi $19,$8,40\n"   /* 80152F50: 21130028 */
    "\taddiu $21,$21,1\n"   /* 80152F54: 26b50001 */
    "\tsw $21,52($17)\n"   /* 80152F58: ae350034 */
    "\tlw $23,8($17)\n"   /* 80152F5C: 8e370008 */
    "\tmove $10,$21\n"   /* 80152F60: 02a05021 */
    "\tmove $9,$23\n"   /* 80152F64: 02e04821 */
    ".L80152F68:\n"
    "\tblez $20,.L801527A4\n"   /* 80152F68: 1a80fe0e */
    "\tlw $4,0($19)\n"   /* 80152F6C: 8e640000 */
    "\taddiu $20,$20,-1\n"   /* 80152F70: 2694ffff */
    "\tjal renderPushMatrix\n"   /* 80152F74: 0c054999 */
    "\tnop\n"   /* 80152F78: 00000000 */
    "\tj .L80152F68\n"   /* 80152F7C: 08054bda */
    "\taddiu $19,$19,4\n"   /* 80152F80: 26730004 */
    "\tlbu $18,7($8)\n"   /* 80152F84: 91120007 */
    "\taddiu $19,$8,16\n"   /* 80152F88: 25130010 */
    "\tsll $18,$18,0x2\n"   /* 80152F8C: 00129080 */
    "\taddu $19,$19,$18\n"   /* 80152F90: 02729821 */
    "\tlw $4,0($19)\n"   /* 80152F94: 8e640000 */
    "\tnop\n"   /* 80152F98: 00000000 */
    "\tjal renderPushMatrix\n"   /* 80152F9C: 0c054999 */
    "\tnop\n"   /* 80152FA0: 00000000 */
    "\tj .L801527A4\n"   /* 80152FA4: 080549e9 */
    "\tnop\n"   /* 80152FA8: 00000000 */
    "\tlbu $18,18($8)\n"   /* 80152FAC: 91120012 */
    "\tnop\n"   /* 80152FB0: 00000000 */
    "\tbnez $18,.L80153024\n"   /* 80152FB4: 1640001b */
    "\tnop\n"   /* 80152FB8: 00000000 */
    "\tlw $2,12($17)\n"   /* 80152FBC: 8e22000c */
    "\tmove $4,$8\n"   /* 80152FC0: 01002021 */
    "\tsw $8,0($2)\n"   /* 80152FC4: ac480000 */
    "\tsw $9,4($2)\n"   /* 80152FC8: ac490004 */
    "\tsw $10,8($2)\n"   /* 80152FCC: ac4a0008 */
    "\tsw $11,12($2)\n"   /* 80152FD0: ac4b000c */
    "\tsw $12,16($2)\n"   /* 80152FD4: ac4c0010 */
    "\tsw $13,20($2)\n"   /* 80152FD8: ac4d0014 */
    "\tsw $14,24($2)\n"   /* 80152FDC: ac4e0018 */
    "\tsw $15,28($2)\n"   /* 80152FE0: ac4f001c */
    "\tsw $24,32($2)\n"   /* 80152FE4: ac580020 */
    "\tsw $25,36($2)\n"   /* 80152FE8: ac590024 */
    "\tjal animUpdateOne\n"   /* 80152FEC: 0c05463a */
    "\tnop\n"   /* 80152FF0: 00000000 */
    "\tlw $2,12($17)\n"   /* 80152FF4: 8e22000c */
    "\tnop\n"   /* 80152FF8: 00000000 */
    "\tlw $8,0($2)\n"   /* 80152FFC: 8c480000 */
    "\tlw $9,4($2)\n"   /* 80153000: 8c490004 */
    "\tlw $10,8($2)\n"   /* 80153004: 8c4a0008 */
    "\tlw $11,12($2)\n"   /* 80153008: 8c4b000c */
    "\tlw $12,16($2)\n"   /* 8015300C: 8c4c0010 */
    "\tlw $13,20($2)\n"   /* 80153010: 8c4d0014 */
    "\tlw $14,24($2)\n"   /* 80153014: 8c4e0018 */
    "\tlw $15,28($2)\n"   /* 80153018: 8c4f001c */
    "\tlw $24,32($2)\n"   /* 8015301C: 8c580020 */
    "\tlw $25,36($2)\n"   /* 80153020: 8c590024 */
    ".L80153024:\n"
    "\tlbu $18,12($8)\n"   /* 80153024: 9112000c */
    "\taddiu $19,$8,32\n"   /* 80153028: 25130020 */
    "\tsll $18,$18,0x2\n"   /* 8015302C: 00129080 */
    "\taddu $19,$19,$18\n"   /* 80153030: 02729821 */
    "\tlw $4,0($19)\n"   /* 80153034: 8e640000 */
    "\tjal renderPushMatrix\n"   /* 80153038: 0c054999 */
    "\tnop\n"   /* 8015303C: 00000000 */
    "\tj .L801527A4\n"   /* 80153040: 080549e9 */
    "\tnop\n"   /* 80153044: 00000000 */
    "\tlbu $18,4($8)\n"   /* 80153048: 91120004 */
    "\tlbu $19,6($8)\n"   /* 8015304C: 91130006 */
    "\tlw $4,12($8)\n"   /* 80153050: 8d04000c */
    "\tandi $18,$18,0x2\n"   /* 80153054: 32520002 */
    "\tbnez $18,.L801527A4\n"   /* 80153058: 1640fdd2 */
    "\taddiu $20,$8,16\n"   /* 8015305C: 25140010 */
    "\tlw $18,0($20)\n"   /* 80153060: 8e920000 */
    ".L80153064:\n"
    "\tnegu $21,$11\n"   /* 80153064: 000ba823 */
    "\tnegu $22,$12\n"   /* 80153068: 000cb023 */
    "\taddiu $18,$18,4\n"   /* 8015306C: 26520004 */
    "\tnegu $23,$13\n"   /* 80153070: 000db823 */
    "\tlh $3,0($18)\n"   /* 80153074: 86430000 */
    "\tnop\n"   /* 80153078: 00000000 */
    "\tmult $3,$21\n"   /* 8015307C: 00750018 */
    "\tmflo $3\n"   /* 80153080: 00001812 */
    "\tlh $7,2($18)\n"   /* 80153084: 86470002 */
    "\tnop\n"   /* 80153088: 00000000 */
    "\tmult $7,$22\n"   /* 8015308C: 00f60018 */
    "\tmflo $7\n"   /* 80153090: 00003812 */
    "\tadd $3,$7,$3\n"   /* 80153094: 00e31820 */
    "\tlh $7,4($18)\n"   /* 80153098: 86470004 */
    "\tnop\n"   /* 8015309C: 00000000 */
    "\tmult $7,$23\n"   /* 801530A0: 00f70018 */
    "\tmflo $7\n"   /* 801530A4: 00003812 */
    "\tadd $3,$3,$7\n"   /* 801530A8: 00671820 */
    "\tbgez $3,.L801530B8\n"   /* 801530AC: 04610002 */
    "\tnop\n"   /* 801530B0: 00000000 */
    "\taddiu $3,$3,4095\n"   /* 801530B4: 24630fff */
    ".L801530B8:\n"
    "\tlh $5,6($18)\n"   /* 801530B8: 86450006 */
    "\tsra $3,$3,0xc\n"   /* 801530BC: 00031b03 */
    "\tmult $5,$21\n"   /* 801530C0: 00b50018 */
    "\tmflo $5\n"   /* 801530C4: 00002812 */
    "\tlh $7,8($18)\n"   /* 801530C8: 86470008 */
    "\tnop\n"   /* 801530CC: 00000000 */
    "\tmult $7,$22\n"   /* 801530D0: 00f60018 */
    "\tmflo $7\n"   /* 801530D4: 00003812 */
    "\tadd $5,$7,$5\n"   /* 801530D8: 00e52820 */
    "\tlh $7,10($18)\n"   /* 801530DC: 8647000a */
    "\tnop\n"   /* 801530E0: 00000000 */
    "\tmult $7,$23\n"   /* 801530E4: 00f70018 */
    "\tmflo $7\n"   /* 801530E8: 00003812 */
    "\tadd $5,$5,$7\n"   /* 801530EC: 00a72820 */
    "\tbgez $5,.L801530FC\n"   /* 801530F0: 04a10002 */
    "\tnop\n"   /* 801530F4: 00000000 */
    "\taddiu $5,$5,4095\n"   /* 801530F8: 24a50fff */
    ".L801530FC:\n"
    "\tlh $6,12($18)\n"   /* 801530FC: 8646000c */
    "\tsra $5,$5,0xc\n"   /* 80153100: 00052b03 */
    "\tmult $6,$21\n"   /* 80153104: 00d50018 */
    "\tmflo $6\n"   /* 80153108: 00003012 */
    "\tlh $7,14($18)\n"   /* 8015310C: 8647000e */
    "\tnop\n"   /* 80153110: 00000000 */
    "\tmult $7,$22\n"   /* 80153114: 00f60018 */
    "\tmflo $7\n"   /* 80153118: 00003812 */
    "\tadd $6,$7,$6\n"   /* 8015311C: 00e63020 */
    "\tlh $7,16($18)\n"   /* 80153120: 86470010 */
    "\tnop\n"   /* 80153124: 00000000 */
    "\tmult $7,$23\n"   /* 80153128: 00f70018 */
    "\tmflo $7\n"   /* 8015312C: 00003812 */
    "\tadd $6,$6,$7\n"   /* 80153130: 00c73020 */
    "\tbgez $6,.L80153140\n"   /* 80153134: 04c10002 */
    "\tnop\n"   /* 80153138: 00000000 */
    "\taddiu $6,$6,4095\n"   /* 8015313C: 24c60fff */
    ".L80153140:\n"
    "\tsra $6,$6,0xc\n"   /* 80153140: 00063303 */
    "\tlw $21,20($18)\n"   /* 80153144: 8e550014 */
    "\tlw $22,24($18)\n"   /* 80153148: 8e560018 */
    "\tlw $23,28($18)\n"   /* 8015314C: 8e57001c */
    "\taddu $21,$3,$21\n"   /* 80153150: 0075a821 */
    "\tblez $21,.L801532A8\n"   /* 80153154: 1aa00054 */
    "\taddu $22,$5,$22\n"   /* 80153158: 00b6b021 */
    "\tblez $22,.L801532A8\n"   /* 8015315C: 1ac00052 */
    "\taddu $23,$6,$23\n"   /* 80153160: 00d7b821 */
    "\tblez $23,.L801532A8\n"   /* 80153164: 1ae00050 */
    "\taddiu $18,$18,-4\n"   /* 80153168: 2652fffc */
    "\tlbu $2,0($18)\n"   /* 8015316C: 92420000 */
    "\taddiu $18,$18,36\n"   /* 80153170: 26520024 */
    "\tsltiu $21,$2,4\n"   /* 80153174: 2c550004 */
    "\tbnez $21,.L801532C0\n"   /* 80153178: 16a00051 */
    "\tnop\n"   /* 8015317C: 00000000 */
    "\tnegu $21,$11\n"   /* 80153180: 000ba823 */
    "\tnegu $22,$12\n"   /* 80153184: 000cb023 */
    "\tnegu $23,$13\n"   /* 80153188: 000db823 */
    "\tlh $3,0($18)\n"   /* 8015318C: 86430000 */
    "\tnop\n"   /* 80153190: 00000000 */
    "\tmult $3,$21\n"   /* 80153194: 00750018 */
    "\tmflo $3\n"   /* 80153198: 00001812 */
    "\tlh $7,2($18)\n"   /* 8015319C: 86470002 */
    "\tnop\n"   /* 801531A0: 00000000 */
    "\tmult $7,$22\n"   /* 801531A4: 00f60018 */
    "\tmflo $7\n"   /* 801531A8: 00003812 */
    "\tadd $3,$7,$3\n"   /* 801531AC: 00e31820 */
    "\tlh $7,4($18)\n"   /* 801531B0: 86470004 */
    "\tnop\n"   /* 801531B4: 00000000 */
    "\tmult $7,$23\n"   /* 801531B8: 00f70018 */
    "\tmflo $7\n"   /* 801531BC: 00003812 */
    "\tadd $3,$3,$7\n"   /* 801531C0: 00671820 */
    "\tbgez $3,.L801531D0\n"   /* 801531C4: 04610002 */
    "\tnop\n"   /* 801531C8: 00000000 */
    "\taddiu $3,$3,4095\n"   /* 801531CC: 24630fff */
    ".L801531D0:\n"
    "\tlh $5,6($18)\n"   /* 801531D0: 86450006 */
    "\tsra $3,$3,0xc\n"   /* 801531D4: 00031b03 */
    "\tmult $5,$21\n"   /* 801531D8: 00b50018 */
    "\tmflo $5\n"   /* 801531DC: 00002812 */
    "\tlh $7,8($18)\n"   /* 801531E0: 86470008 */
    "\tnop\n"   /* 801531E4: 00000000 */
    "\tmult $7,$22\n"   /* 801531E8: 00f60018 */
    "\tmflo $7\n"   /* 801531EC: 00003812 */
    "\tadd $5,$7,$5\n"   /* 801531F0: 00e52820 */
    "\tlh $7,10($18)\n"   /* 801531F4: 8647000a */
    "\tnop\n"   /* 801531F8: 00000000 */
    "\tmult $7,$23\n"   /* 801531FC: 00f70018 */
    "\tmflo $7\n"   /* 80153200: 00003812 */
    "\tadd $5,$5,$7\n"   /* 80153204: 00a72820 */
    "\tbgez $5,.L80153214\n"   /* 80153208: 04a10002 */
    "\tnop\n"   /* 8015320C: 00000000 */
    "\taddiu $5,$5,4095\n"   /* 80153210: 24a50fff */
    ".L80153214:\n"
    "\tlh $6,12($18)\n"   /* 80153214: 8646000c */
    "\tsra $5,$5,0xc\n"   /* 80153218: 00052b03 */
    "\tmult $6,$21\n"   /* 8015321C: 00d50018 */
    "\tmflo $6\n"   /* 80153220: 00003012 */
    "\tlh $7,14($18)\n"   /* 80153224: 8647000e */
    "\tnop\n"   /* 80153228: 00000000 */
    "\tmult $7,$22\n"   /* 8015322C: 00f60018 */
    "\tmflo $7\n"   /* 80153230: 00003812 */
    "\tadd $6,$7,$6\n"   /* 80153234: 00e63020 */
    "\tlh $7,16($18)\n"   /* 80153238: 86470010 */
    "\tnop\n"   /* 8015323C: 00000000 */
    "\tmult $7,$23\n"   /* 80153240: 00f70018 */
    "\tmflo $7\n"   /* 80153244: 00003812 */
    "\tadd $6,$6,$7\n"   /* 80153248: 00c73020 */
    "\tbgez $6,.L80153258\n"   /* 8015324C: 04c10002 */
    "\tnop\n"   /* 80153250: 00000000 */
    "\taddiu $6,$6,4095\n"   /* 80153254: 24c60fff */
    ".L80153258:\n"
    "\tsra $6,$6,0xc\n"   /* 80153258: 00063303 */
    "\tlw $21,20($18)\n"   /* 8015325C: 8e550014 */
    "\tnop\n"   /* 80153260: 00000000 */
    "\taddu $21,$3,$21\n"   /* 80153264: 0075a821 */
    "\tblez $21,.L801532A8\n"   /* 80153268: 1aa0000f */
    "\tsltiu $21,$2,5\n"   /* 8015326C: 2c550005 */
    "\tbnez $21,.L801532C0\n"   /* 80153270: 16a00013 */
    "\tlw $21,24($18)\n"   /* 80153274: 8e550018 */
    "\tnop\n"   /* 80153278: 00000000 */
    "\taddu $21,$5,$21\n"   /* 8015327C: 00b5a821 */
    "\tblez $21,.L801532A8\n"   /* 80153280: 1aa00009 */
    "\tsltiu $21,$2,6\n"   /* 80153284: 2c550006 */
    "\tbnez $21,.L801532C0\n"   /* 80153288: 16a0000d */
    "\tlw $21,28($18)\n"   /* 8015328C: 8e55001c */
    "\tnop\n"   /* 80153290: 00000000 */
    "\taddu $21,$6,$21\n"   /* 80153294: 00d5a821 */
    "\tblez $21,.L801532A8\n"   /* 80153298: 1aa00003 */
    "\tnop\n"   /* 8015329C: 00000000 */
    "\tj .L801532C0\n"   /* 801532A0: 08054cb0 */
    "\tnop\n"   /* 801532A4: 00000000 */
    ".L801532A8:\n"
    "\taddiu $19,$19,-1\n"   /* 801532A8: 2673ffff */
    "\taddiu $20,$20,4\n"   /* 801532AC: 26940004 */
    "\tbgtz $19,.L80153064\n"   /* 801532B0: 1e60ff6c */
    "\tlw $18,0($20)\n"   /* 801532B4: 8e920000 */
    "\tlw $4,8($8)\n"   /* 801532B8: 8d040008 */
    "\tnop\n"   /* 801532BC: 00000000 */
    ".L801532C0:\n"
    "\tbeqz $4,.L801527A4\n"   /* 801532C0: 1080fd38 */
    "\tnop\n"   /* 801532C4: 00000000 */
    "\tjal renderPushMatrix\n"   /* 801532C8: 0c054999 */
    "\tnop\n"   /* 801532CC: 00000000 */
    "\tj .L801527A4\n"   /* 801532D0: 080549e9 */
    "\tnop\n"   /* 801532D4: 00000000 */
    "\tlhu $18,4($8)\n"   /* 801532D8: 95120004 */
    "\tlw $23,52($17)\n"   /* 801532DC: 8e370034 */
    "\tsubu $20,$14,$18\n"   /* 801532E0: 01d2a023 */
    "\tbgez $20,.L80153304\n"   /* 801532E4: 06810007 */
    "\taddiu $23,$23,1\n"   /* 801532E8: 26f70001 */
    "\tnegu $20,$20\n"   /* 801532EC: 0014a023 */
    "\tsllv $11,$11,$20\n"   /* 801532F0: 028b5804 */
    "\tsllv $12,$12,$20\n"   /* 801532F4: 028c6004 */
    "\tsllv $13,$13,$20\n"   /* 801532F8: 028d6804 */
    "\tj .L80153318\n"   /* 801532FC: 08054cc6 */
    "\tnop\n"   /* 80153300: 00000000 */
    ".L80153304:\n"
    "\tblez $20,.L80153318\n"   /* 80153304: 1a800004 */
    "\tnop\n"   /* 80153308: 00000000 */
    "\tsrav $11,$11,$20\n"   /* 8015330C: 028b5807 */
    "\tsrav $12,$12,$20\n"   /* 80153310: 028c6007 */
    "\tsrav $13,$13,$20\n"   /* 80153314: 028d6807 */
    ".L80153318:\n"
    "\tmove $14,$18\n"   /* 80153318: 02407021 */
    "\tlhu $18,6($8)\n"   /* 8015331C: 95120006 */
    "\taddiu $19,$8,8\n"   /* 80153320: 25130008 */
    "\tsw $23,52($17)\n"   /* 80153324: ae370034 */
    "\tmove $10,$23\n"   /* 80153328: 02e05021 */
    ".L8015332C:\n"
    "\tblez $18,.L801527A4\n"   /* 8015332C: 1a40fd1d */
    "\tlw $4,0($19)\n"   /* 80153330: 8e640000 */
    "\taddiu $18,$18,-1\n"   /* 80153334: 2652ffff */
    "\tjal renderPushMatrix\n"   /* 80153338: 0c054999 */
    "\tnop\n"   /* 8015333C: 00000000 */
    "\tj .L8015332C\n"   /* 80153340: 08054ccb */
    "\taddiu $19,$19,4\n"   /* 80153344: 26730004 */
    ".L80153348:\n"
    "\tlw $31,64($17)\n"   /* 80153348: 8e3f0040 */
    "\tlw $4,48($17)\n"   /* 8015334C: 8e240030 */
    "\tsw $16,32($17)\n"   /* 80153350: ae300020 */
    "\tlw $16,76($17)\n"   /* 80153354: 8e30004c */
    "\tlw $18,84($17)\n"   /* 80153358: 8e320054 */
    "\tlw $19,88($17)\n"   /* 8015335C: 8e330058 */
    "\tlw $20,92($17)\n"   /* 80153360: 8e34005c */
    "\tlw $21,96($17)\n"   /* 80153364: 8e350060 */
    "\tlw $22,100($17)\n"   /* 80153368: 8e360064 */
    "\tlw $23,104($17)\n"   /* 8015336C: 8e370068 */
    "\tlw $29,108($17)\n"   /* 80153370: 8e3d006c */
    "\tlw $28,112($17)\n"   /* 80153374: 8e3c0070 */
    "\tlw $6,116($17)\n"   /* 80153378: 8e260074 */
    "\tlw $7,120($17)\n"   /* 8015337C: 8e270078 */
    "\tlw $17,80($17)\n"   /* 80153380: 8e310050 */
    "\tjr $31\n"   /* 80153384: 03e00008 */
    "\tnop\n"   /* 80153388: 00000000 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderWalkModel,.-renderWalkModel\n");

/* renderMulTransMatVec (non-ABI): transposed matrix times vector: a2/a3/s4 =
 * (column j of M(a1)) . (s5, s6, s7) / 4096 (rounded toward zero). */
__asm__(
    ".section .text_8015338C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderMulTransMatVec\n"
    ".type renderMulTransMatVec,@function\n"
    "renderMulTransMatVec:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tlh $6,0($5)\n"   /* 8015338C: 84a60000 */
    "\tnop\n"   /* 80153390: 00000000 */
    "\tmult $6,$21\n"   /* 80153394: 00d50018 */
    "\tmflo $6\n"   /* 80153398: 00003012 */
    "\tlh $2,6($5)\n"   /* 8015339C: 84a20006 */
    "\tnop\n"   /* 801533A0: 00000000 */
    "\tmult $2,$22\n"   /* 801533A4: 00560018 */
    "\tmflo $2\n"   /* 801533A8: 00001012 */
    "\tadd $6,$2,$6\n"   /* 801533AC: 00463020 */
    "\tlh $2,12($5)\n"   /* 801533B0: 84a2000c */
    "\tnop\n"   /* 801533B4: 00000000 */
    "\tmult $2,$23\n"   /* 801533B8: 00570018 */
    "\tmflo $2\n"   /* 801533BC: 00001012 */
    "\tadd $6,$6,$2\n"   /* 801533C0: 00c23020 */
    "\tbgez $6,.L801533D0\n"   /* 801533C4: 04c10002 */
    "\tnop\n"   /* 801533C8: 00000000 */
    "\taddiu $6,$6,4095\n"   /* 801533CC: 24c60fff */
    ".L801533D0:\n"
    "\tlh $7,2($5)\n"   /* 801533D0: 84a70002 */
    "\tsra $6,$6,0xc\n"   /* 801533D4: 00063303 */
    "\tmult $7,$21\n"   /* 801533D8: 00f50018 */
    "\tmflo $7\n"   /* 801533DC: 00003812 */
    "\tlh $2,8($5)\n"   /* 801533E0: 84a20008 */
    "\tnop\n"   /* 801533E4: 00000000 */
    "\tmult $2,$22\n"   /* 801533E8: 00560018 */
    "\tmflo $2\n"   /* 801533EC: 00001012 */
    "\tadd $7,$2,$7\n"   /* 801533F0: 00473820 */
    "\tlh $2,14($5)\n"   /* 801533F4: 84a2000e */
    "\tnop\n"   /* 801533F8: 00000000 */
    "\tmult $2,$23\n"   /* 801533FC: 00570018 */
    "\tmflo $2\n"   /* 80153400: 00001012 */
    "\tadd $7,$7,$2\n"   /* 80153404: 00e23820 */
    "\tbgez $7,.L80153414\n"   /* 80153408: 04e10002 */
    "\tnop\n"   /* 8015340C: 00000000 */
    "\taddiu $7,$7,4095\n"   /* 80153410: 24e70fff */
    ".L80153414:\n"
    "\tlh $20,4($5)\n"   /* 80153414: 84b40004 */
    "\tsra $7,$7,0xc\n"   /* 80153418: 00073b03 */
    "\tmult $20,$21\n"   /* 8015341C: 02950018 */
    "\tmflo $20\n"   /* 80153420: 0000a012 */
    "\tlh $2,10($5)\n"   /* 80153424: 84a2000a */
    "\tnop\n"   /* 80153428: 00000000 */
    "\tmult $2,$22\n"   /* 8015342C: 00560018 */
    "\tmflo $2\n"   /* 80153430: 00001012 */
    "\tadd $20,$2,$20\n"   /* 80153434: 0054a020 */
    "\tlh $2,16($5)\n"   /* 80153438: 84a20010 */
    "\tnop\n"   /* 8015343C: 00000000 */
    "\tmult $2,$23\n"   /* 80153440: 00570018 */
    "\tmflo $2\n"   /* 80153444: 00001012 */
    "\tadd $20,$20,$2\n"   /* 80153448: 0282a020 */
    "\tbgez $20,.L80153458\n"   /* 8015344C: 06810002 */
    "\tnop\n"   /* 80153450: 00000000 */
    "\taddiu $20,$20,4095\n"   /* 80153454: 26940fff */
    ".L80153458:\n"
    "\tjr $31\n"   /* 80153458: 03e00008 */
    "\tsra $20,$20,0xc\n"   /* 8015345C: 0014a303 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderMulTransMatVec,.-renderMulTransMatVec\n");
#endif /* TM2_PORT_RENDER_SLOTS */
