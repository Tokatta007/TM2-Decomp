/* Pure-C version of src/semantic/engine_render_scene.c whose
 * SingleTrac hand-written asm functions (renderPushMatrix 80152664, renderWalkModel
 * 801526CC, renderMulTransMatVec 8015338C) are rewritten as C; the other functions
 * are the same C as src/ (byte-identical). Same behaviour as the retail asm on random
 * model trees, together with the near pass.
 * Linked by the all-C relocatable flavour (build_source_only.py --relocatable --all-c,
 * SOURCE_BUILD.md "All-C flavour"); the port variant with TM2_PORT_RENDER_SLOTS links
 * only its walker (src/ includes it with TM2_RENDER_WALKER_ONLY; request 47). The hybrid
 * and the fixed source-only builds keep the asm (docs/RULES.md). Static proof only.
 * ABI: renderWalkModel(obj, model) is called by compiled C (renderScene,
 * renderDrawModel...) with a0/a1, as the asm expects. The asm also saved s0-s7, sp,
 * gp, a2, a3 and ra into the scratchpad record (0x1F80004C..0x1F800078, ra at 0x40)
 * and kept the walk in private registers; the C saves on the stack, so those
 * scratchpad words keep other values afterwards (callers rewrite 0x40/0x4C/0x60/
 * 0x6C before the emitter). renderPushMatrix and renderMulTransMatVec had private
 * register conventions and are only called by the walker; here they are normal C
 * functions. The retail asm labels renderNodeHandler0..2 / renderNodeSkipHandler /
 * D_80152xxx that renderInitNodeHandlers stores in the handler table are only
 * tokens for the C walker (it dispatches with a switch); the link resolves them to
 * renderWalkModel's entry. */

/* Library code 80152000..80154000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything here is SingleTrac engine code (scene renderer and lighting
 * environments); no third-party source was used, all written by hand
 * against the retail code.
 *
 * Recipe: the engine one, psyq3.6 cc1 + maspsx, -fvolatile, extern_gp,
 * gp_size 8 (not the SDK psyq3.3 / gp_size 0 recipe: the renderer and
 * the light functions address $gp globals).
 *
 * renderPushMatrix, renderWalkModel and renderMulTransMatVec (the model tree walker and
 * its two subroutines) were hand-written assembly in retail: non-ABI
 * register conventions, registers saved in the scratchpad at 0x1F800000,
 * node handlers dispatched through a table that renderInitNodeHandlers fills. They
 * are written here as plain C (only the GTE transfers are asm), so they do
 * not reproduce the retail bytes; the other 17 functions are byte-identical. */
#ifndef TM2_RENDER_WALKER_ONLY    /* src/ (port variant, request 47) has everything up to the walker */
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
extern u8 D_801ABA50[];
extern u8 D_801A4DEC[];
extern u8 renderMatrixSlots[];
extern u8 renderObjectMatrices[];
extern u8 D_801AC810[];
extern u8 renderNodeHandlers[];
extern u8 D_801B2E0C[];
extern u8 D_801B470C[];
extern void *renderWorldModels[];

extern s32 timerGetFrameTicks(void);
extern s32 viewGetMode(s32 view);
extern u8 *csGetRoot(void);
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
#define RENDER_WALKER_NODE_SLOTS 198      /* the transform nodes' slot limit (src/ sets the port's) */
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
    s->m04 = D_801ABA50;
    s->count = 0;
    s->m0C = D_801A4DEC;
    s->depth = depth;
    s->m14 = 0;
    s->m18 = renderMatrixSlots;
    s->matrices = renderObjectMatrices;
    s->m20 = D_801AC810;
    s->m24 = m24;
    s->m28 = m28;
    s->m2C = renderNodeHandlers;
    s->m30 = 0;
    s->objects = 0;
    s->near_count = near;
    s->end = 0;
    s->near_otz = 0;
    s->m44 = D_801B2E0C;
    s->m48 = D_801B470C;
}

/* Scene renderer: level model, registered models, the view's own model and
 * every active object, then the polygon emitter passes. */
void __attribute__((section(".text_80152284"))) renderScene(u8 *db, s32 view)
{
    u32 *count;
    s32 *objects;
    u8 *obj;
    s32 i;

    count = &renderCtx->count;
    objects = &renderCtx->objects;
    if (viewGetMode(view) == 4) {
        renderFarDistance = 500;
    } else {
        renderFarDistance = renderFarDistanceDefault;
    }
    renderInitState();
    obj = csGetRoot();
    if (*(void **)(obj + 284)) {
        memCopyWords(renderObjectMatrices + (*count << 5), *(u8 **)(obj + 4) + 44, 8);
        renderWalkModel(obj, *(void **)(obj + 284));
    }
    for (i = 0; i < 20; i++) {
        if (renderWorldModels[i]) {
            if (*count < 200) {
                *count = *count + 1;
            }
            obj = csGetRoot();
            memCopyWords(renderObjectMatrices + (*count << 5), *(u8 **)(obj + 4) + 44, 8);
            renderWalkModel(obj, renderWorldModels[i]);
        }
    }
    renderFarDistance = renderFarDistanceDefault;
    renderCtx->depth = renderFarDistance;
    obj = viewGetFollowObj(view);
    if (obj) {
        if (*count < 200) {
            *count = *count + 1;
        }
        csUpdMat(obj, renderObjectMatrices + (*count << 5), 1);
        *objects = *objects + 1;
        renderWalkModel(obj, *(void **)(obj + 284));
    }
    if (viewGetMode(view) == 4) {
        renderFarDistance = 1000;
    } else {
        renderFarDistance = renderFarDistanceDefault;
    }
    renderCtx->depth = renderFarDistance;
    if (renderNodeCullEnable == 0) {
        renderSkipNodeCull = 1;
    }
    renderCtx->m28 = renderSkipNodeCull;
    for (obj = csListHead(); obj; obj = *(u8 **)(obj + 288)) {
        if (*(s32 *)(obj + 8)) {
            if (*count < 200) {
                *count = *count + 1;
            }
            csUpdMat(obj, renderObjectMatrices + (*count << 5), renderNodeCullEnable);
            if (*(s32 *)(obj + 292)) {
                *objects = *objects + 1;
                renderWalkModel(obj, *(void **)(obj + 284));
            }
        }
    }
    renderCtx->m28 = 0;
    renderSkipNodeCull = 0;
    renderCurrentOt = renderCtx->end;
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
    sub_800CDC54(db, D_801AC810, renderCurrentOt, renderMatrixSlots, renderObjectMatrices);
    renderNearPolys(db, renderSharedWorkBuffer, renderCtx->near_count);
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

/* Install the model node handler table: the retail addresses of the node
 * handlers inside renderWalkModel (the C walker dispatches with a switch on
 * the node type instead, see there). */
extern u8 renderNodeHandler0[], renderNodeHandler1[], renderNodeHandler2[], D_80152FAC[], D_80152ABC[],
    D_80152B18[], renderNodeSkipHandler[], D_80152F84[], D_80153048[], D_801532D8[];
/* The "skip" handler (801527A4) is named from different neighbouring labels
 * so the compiler rematerialises its address for every entry, as retail does. */
extern u8 D_801527A0[], D_8015279C[], D_80152798[], D_80152794[];
void __attribute__((section(".text_80153460"))) renderInitNodeHandlers(void **t)
{
    t[0] = renderNodeHandler0;
    t[1] = renderNodeHandler1;
    t[2] = renderNodeHandler2;
    t[3] = D_80152FAC;
    t[4] = D_80152ABC;
    t[5] = D_80152B18;
    t[6] = renderNodeSkipHandler;
    t[7] = D_801527A0 + 4;
    t[8] = D_8015279C + 8;
    t[9] = D_80152F84;
    t[10] = D_80152798 + 12;
    t[11] = D_80153048;
    t[12] = D_801532D8;
    t[13] = D_80152794 + 16;
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
#endif /* TM2_RENDER_WALKER_ONLY */
/* ---- model tree walker (hand-written assembly in retail, here as C) ---- */

/* GTE (cop2) access: hardware instructions C cannot express. Two nops let a
 * transfer settle before a command; one after mfc2 covers its load delay. */
#define gte_ldR(r0, r1, r2, r3, r4) \
    __asm__ volatile ("ctc2 %0,$0\n\tctc2 %1,$1\n\tctc2 %2,$2\n\tctc2 %3,$3\n\tctc2 %4,$4" \
                      : : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4))
#define gte_ldV0(xy, z) __asm__ volatile ("mtc2 %0,$0\n\tmtc2 %1,$1" : : "r"(xy), "r"(z))
#define gte_ldIR(x, y, z) __asm__ volatile ("mtc2 %0,$9\n\tmtc2 %1,$10\n\tmtc2 %2,$11" : : "r"(x), "r"(y), "r"(z))
#define gte_rtv0() __asm__ volatile ("nop\n\tnop\n\tcop2 0x486012")  /* MVMVA sf=1: IR = R * V0 */
#define gte_sqr0() __asm__ volatile ("nop\n\tnop\n\tcop2 0xA00428")  /* SQR sf=0: MAC = IR * IR */
#define gte_stIR(x, y, z) \
    __asm__ volatile ("mfc2 %0,$9\n\tmfc2 %1,$10\n\tmfc2 %2,$11\n\tnop" : "=r"(x), "=r"(y), "=r"(z))
#define gte_stMAC(x, y, z) \
    __asm__ volatile ("mfc2 %0,$25\n\tmfc2 %1,$26\n\tmfc2 %2,$27\n\tnop" : "=r"(x), "=r"(y), "=r"(z))

#define W32(p, o) (*(s32 *)((u8 *)(p) + (o)))

/* Halfword matrix row (m[0], m[1], m[2]) times v, / 4096 rounded toward zero. */
#define DOT3(m, v) (((m)[0] * (v)[0] + (m)[1] * (v)[1] + (m)[2] * (v)[2]) / 4096)

/* One saved node (28 bytes): the node and the transform it is walked with. */
typedef struct {
    u8 *node;
    s32 mtx;
    s32 t[3];
    s32 obj, shift;
} WalkEntry;

/* Walker state: registers t0..t9 of the retail code, in that order (the
 * order of the record saved at state->m0C around the animation call). */
typedef struct {
    u8 *node;           /* t0 current node */
    s32 mtx;            /* t1 matrix slot */
    s32 obj;            /* t2 object number */
    s32 t[3];           /* t3..t5 translation */
    s32 shift;          /* t6 coordinate shift */
    s32 depth;          /* t7 saved nodes */
    WalkEntry *stk;     /* t8 node stack in the scratchpad (32 entries) */
    WalkEntry *ovf;     /* t9 overflow stack (renderSharedWorkBuffer, up to 200) */
} WalkCtx;

/* Meshes queued for the model emitter (32 bytes each). */
typedef struct {
    u8 *node;
    s32 t[3];
    s32 shift;
    u8 *obj;
    s32 mtx, objno;
} WalkMesh;

extern void animUpdateOne(void *anim);

/* renderPushMatrix: save child node `node` with the current transform: on the
 * scratchpad stack while fewer than 32 are saved, then on the overflow
 * stack; from 200 on, the top overflow entry is overwritten. */
void __attribute__((section(".text_80152664"))) renderPushMatrix(WalkCtx *w, u8 *node)
{
    WalkEntry *e, **sp;
    s32 depth, mtx, x, y, z, obj, shift;

    depth = w->depth;
    sp = depth < 32 ? &w->stk : &w->ovf;
    e = *sp;
    if (depth < 200)
        *sp = e + 1;
    else
        depth--;
    mtx = w->mtx;
    x = w->t[0];
    y = w->t[1];
    z = w->t[2];
    obj = w->obj;
    shift = w->shift;
    e->node = node;
    e->mtx = mtx;
    e->t[0] = x;
    e->t[1] = y;
    e->t[2] = z;
    e->obj = obj;
    e->shift = shift;
    w->depth = depth + 1;
}

/* renderMulTransMatVec: transposed matrix times vector, / 4096 rounded toward zero. */
void __attribute__((section(".text_8015338C"))) renderMulTransMatVec(s16 *m, s32 *v, s32 *out)
{
    s32 x, y, z;

    x = v[0];
    y = v[1];
    z = v[2];
    out[0] = (m[0] * x + m[3] * y + m[6] * z) / 4096;
    out[1] = (m[1] * x + m[4] * y + m[7] * z) / 4096;
    out[2] = (m[2] * x + m[5] * y + m[8] * z) / 4096;
}

/* renderWalkModel(obj, model): walk the model tree of obj, pushing children on
 * a stack (LIFO, so the last child is walked first). Each node starts with
 * a type byte; retail dispatches through the handler table at state->m2C
 * (renderNodeHandlers, filled once by renderInitNodeHandlers with fixed handler addresses
 * inside this function), which the type tests below reproduce:
 *   0 mesh: sphere test against the view, then queue it for sub_800CDC54
 *   1 group: sphere test, then walk the children
 *   2 LOD: choose the child whose distance band holds the squared distance
 *   3 animation: advance it (animUpdateOne), walk the current frame's child
 *   4 offset: translate, walk the children
 *   5 transform: rotate/translate, build the child matrix slot, walk children
 *   9 select: walk child node[7]
 *  11 zones: walk one of two children by a set of box tests
 *  12 scale: change the coordinate shift, walk the children
 *  other: nothing. */
void __attribute__((section(".text_801526CC"))) renderWalkModel(u8 *obj, void *model)
{
    RenderState *S;
    WalkCtx w;
    WalkEntry *e;
    WalkMesh *q;
    u8 *node, *b, **child;
    s16 *m, *M, **parent, **slot;
    s32 v[3], o[3];
    s32 type, n, i, j, k, a, s, r, count, nocull;
    u32 dist;
    s32 ir1, ir2, ir3;
    s16 *out;

    S = (RenderState *)0x1F800000;
    w.stk = (WalkEntry *)((u8 *)S + 124);
    w.ovf = (WalkEntry *)S->near_overflow;
    q = (WalkMesh *)S->m20;
    S->m30 = (s32)obj;
    count = S->count;
    for (i = 0; i < 8; i++)
        ((s32 *)((u8 *)S->m18 + count * 32))[i] = ((s32 *)(obj + 32))[i];
    slot = (s16 **)((u8 *)S->m04 + count * 16);
    slot[0] = (s16 *)(obj + 32);
    slot[1] = (s16 *)((u8 *)S->matrices + count * 32);
    slot[2] = (s16 *)(obj + 64);
    slot[3] = (s16 *)(obj + 96);
    w.node = (u8 *)model;
    w.mtx = count;
    w.obj = S->objects;
    w.t[0] = W32(obj, 20);
    w.t[1] = W32(obj, 24);
    w.t[2] = W32(obj, 28);
    w.shift = S->near_count;
    w.depth = 0;
    goto dispatch;

next:
    if (--w.depth < 0)
        goto done;
    if (w.depth < 32)
        e = --w.stk;
    else
        e = --w.ovf;
    w.node = e->node;
    w.mtx = e->mtx;
    w.t[0] = e->t[0];
    w.t[1] = e->t[1];
    w.t[2] = e->t[2];
    w.obj = e->obj;
    w.shift = e->shift;

dispatch:
    node = w.node;
    type = node[0];
    if (type <= 1) {        /* 0 mesh, 1 group */
        if (type == 0) {
            nocull = *(s16 *)(node + 24) & 0x2000;
            b = node + 28;          /* sphere centre and radius */
            r = W32(node, 40);
        } else {
            if (node[21] & 2)
                goto next;
            nocull = node[21] & 4;
            b = node + 4;
            r = W32(node, 16);
        }
        if (nocull == 0 && S->m28 == 0) {
            /* sphere against the planes of view matrices 2 and 3 of the
             * slot, and the far distance (depth << shift) */
            for (k = 0; k < 3; k++)
                v[k] = w.t[k] + W32(b, k * 4);
            slot = (s16 **)((u8 *)S->m04 + w.mtx * 16);
            r = -r;
            renderMulTransMatVec(slot[2], v, o);
            if (o[0] < r || o[1] < r || o[2] < r)
                goto next;
            renderMulTransMatVec(slot[3], v, o);
            if (o[0] < r || o[1] < r || o[2] < r - (S->depth << w.shift))
                goto next;
        }
        if (type == 0) {
            if (S->end >= 280)
                goto next;
            q->node = node;
            q->t[0] = w.t[0];
            q->t[1] = w.t[1];
            q->t[2] = w.t[2];
            q->shift = w.shift;
            q->obj = (u8 *)S->m30;
            q->mtx = w.mtx;
            q->objno = w.obj;
            S->end++;
            q++;
            goto next;
        }
        n = node[20];
        child = (u8 **)(node + 24);
    } else if (type == 2) {     /* level of detail */
        gte_ldIR((W32(node, 4) + w.t[0]) >> w.shift, (W32(node, 8) + w.t[1]) >> w.shift,
                 (W32(node, 12) + w.t[2]) >> w.shift);
        gte_sqr0();
        gte_stMAC(ir1, ir2, ir3);
        dist = ir1 + ir2 + ir3;
        if (S->m24 != 0)
            dist = 100;
        n = W32(node, 16);
        child = (u8 **)(node + 20);
        while (n-- > 0) {
            b = *child++;       /* band: max, min, child */
            if (dist < (u32)W32(b, 4) || dist >= (u32)W32(b, 0))
                continue;
            renderPushMatrix(&w, *(u8 **)(b + 8));
            break;
        }
        goto next;

    } else if (type == 3) {     /* animation */
        if (node[18] == 0) {
            /* the walker registers (t0..t9) are kept in the state->m0C
             * record across the call */
            for (k = 0; k < 10; k++)
                ((s32 *)S->m0C)[k] = ((s32 *)&w)[k];
            animUpdateOne(node);
            for (k = 0; k < 10; k++)
                ((s32 *)&w)[k] = ((s32 *)S->m0C)[k];
        }
        renderPushMatrix(&w, ((u8 **)(w.node + 32))[w.node[12]]);
        goto next;

    } else if (type == 4) {     /* offset */
        if (node[19] & 2)
            goto next;
        for (k = 0; k < 3; k++)
            w.t[k] += W32(node, 4 + k * 4);
        w.obj = ++S->objects;
        n = node[18];
        child = (u8 **)(node + 20);
    } else if (type == 5) {     /* transform: rotation at +4, offset at +24 */
        if (node[39] & 2)
            goto next;
        m = (s16 *)(node + 4);
        for (k = 0; k < 3; k++)
            v[k] = w.t[k] + W32(node, 24 + k * 4);
        count = S->count;
        for (k = 0; k < 3; k++)
            w.t[k] = DOT3(m + k * 3, v);
        if (count < RENDER_WALKER_NODE_SLOTS) {
            /* new matrix slot: parent matrices 0/1 times the node rotation
             * (its rows as vectors), the node rotation times parent
             * matrices 2/3 (their columns as vectors) */
            S->count = ++count;
            parent = (s16 **)((u8 *)S->m04 + w.mtx * 16);
            slot = (s16 **)((u8 *)S->m04 + count * 16);
            for (i = 0; i < 4; i++) {
                if (i < 2) {
                    M = parent[i];
                    gte_ldR(W32(M, 0), W32(M, 4), W32(M, 8), W32(M, 12), W32(M, 16));
                    M = m;
                    a = 3;
                    s = 1;
                    out = (s16 *)((u8 *)(i == 0 ? S->m18 : S->matrices) + count * 32);
                } else {
                    if (i == 2)
                        gte_ldR(W32(m, 0), W32(m, 4), W32(m, 8), W32(m, 12), W32(m, 16));
                    M = parent[i];
                    a = 1;
                    s = 3;
                    out = (s16 *)((u8 *)(i == 2 ? S->m44 : S->m48) + count * 32);
                }
                for (j = 0; j < 3; j++) {
                    gte_ldV0((u16)M[a * j] | (M[a * j + s] << 16), M[a * j + 2 * s]);
                    gte_rtv0();
                    gte_stIR(ir1, ir2, ir3);
                    out[j] = ir1;
                    out[3 + j] = ir2;
                    if (j < 2)
                        out[6 + j] = ir3;
                    else
                        *(s32 *)(out + 8) = ir3;
                }
                slot[i] = out;
            }
        }
        w.obj = ++S->objects;
        w.mtx = S->count;
        n = node[38];
        child = (u8 **)(node + 40);
    } else if (type == 9) {     /* select */
        renderPushMatrix(&w, ((u8 **)(node + 16))[node[7]]);
        goto next;

    } else if (type == 11) {    /* zones: walk node+12 when inside one of them, else node+8 */
        if (node[4] & 2)
            goto next;
        for (k = 0; k < 3; k++)
            v[k] = -w.t[k];
        n = node[6];
        child = (u8 **)(node + 16);
        do {
            /* zone: type byte, a box at +4 (rotation, then offsets at +24)
             * and for type >= 4 a second one at +36 of which the first
             * type - 3 rows count; inside when M * -t + offset > 0 */
            b = *child;
            M = (s16 *)(b + 4);
            s = 3;
            for (;;) {
                for (k = 0; k < s; k++)
                    if (DOT3(M + k * 3, v) + W32(M, 20 + k * 4) <= 0)
                        goto outside;
                if (M != (s16 *)(b + 4))
                    goto inside;
                s = b[0] - 3;
                if (s <= 0)
                    goto inside;
                if (s > 3)
                    s = 3;
                M = (s16 *)(b + 36);
            }
        outside:
            child++;
        } while (--n > 0);
        node = *(u8 **)(node + 8);
        goto zone;
    inside:
        node = *(u8 **)(node + 12);
    zone:
        if (node != 0)
            renderPushMatrix(&w, node);
        goto next;

    } else if (type == 12) {    /* scale: coordinates now in units of 1 << node.h4 */
        n = *(u16 *)(node + 4);
        i = w.shift - n;
        for (k = 0; k < 3; k++) {
            if (i < 0)
                w.t[k] <<= -i;
            else if (i > 0)
                w.t[k] >>= i;
        }
        w.shift = n;
        w.obj = ++S->objects;
        n = *(u16 *)(node + 6);
        child = (u8 **)(node + 8);
    } else {
        goto next;
    }

    /* walk the n children at child */
    for (; n > 0; n--)
        renderPushMatrix(&w, *child++);
    goto next;

done:
    S->m20 = q;
}
