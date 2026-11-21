/* View system (80155634..801566F8): the per-player camera views.
 *
 * Each player view i (0..1) has a mode (viewModes[i], an index into the
 * 96-byte mode table at 8017EC44: screen size, draw/display geometry, the
 * projection scale and two base rotation matrices), the double-buffered
 * draw/display environment it renders to (viewEnvs[i]) and a view object
 * (viewCameraObjects[i]) whose parent is the shared root object viewSharedRoot, or
 * the object the camera follows (viewSetParent).
 *
 * viewCreate sets up the environments for a mode; viewLoadMode loads the
 * mode's matrices into the view object; viewProc builds the frame's camera
 * matrices from the view object and its parent (eye position at
 * viewGetEyeTrans(i), rotation at viewGetViewMatrix(i)). viewCycleMode cycles
 * the view mode (0..6) for both players. */
#include <stddef.h>
#include <libgte.h>
#include <libgpu.h>
#include "semantic/players.h"
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"  /* request 99 (bit 9): portVisualMirrorEye */
#endif

typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/* One draw/display buffer pair (as TM2BufferInitRecord in rt_buffers.h). */
typedef struct {
    u8 pad_00[0x10];
    DRAWENV draw;           /* +0x10 */
    DISPENV disp;           /* +0x6C */
} TM2ViewBuffer;

typedef struct {
    TM2ViewBuffer b[2];
} TM2ViewEnv;

/* Camera object (allocated by csAlloc). */
typedef struct TM2ViewObj {
    s32 f0;                         /* +0x00 */
    u8 pad_04[0x0C];
    struct TM2ViewObj *parent;      /* +0x10 */
    u8 pad_14[0x2C];
    MATRIX m40;                     /* +0x40 */
    MATRIX m60;                     /* +0x60 */
    MATRIX rotm;                    /* +0x80 from rot */
    SVECTOR rot;                    /* +0xA0 */
    s32 pos[3];                     /* +0xA8 */
    u8 pad_B4[0x68];
    s32 f11C;                       /* +0x11C */
} TM2ViewObj;

/* Per-frame camera output (csGetRoot). */
typedef struct {
    u8 pad_00[0x14];
    s32 eye[3];                     /* +0x14 */
    MATRIX view;                    /* +0x20 */
    MATRIX m40;                     /* +0x40 */
    MATRIX m60;                     /* +0x60 */
} TM2ViewOut;

/* View mode table entry (8017EC44, 96 bytes). */
typedef struct {
    s32 f0;                 /* +0x00 -> viewFov */
    s32 f4;
    s32 scale;              /* +0x08 sub_8014A274 */
    s32 fC;                 /* +0x0C */
    s32 disp_w;             /* +0x10 */
    s32 disp_h;             /* +0x14 */
    s32 w;                  /* +0x18 */
    s32 h;                  /* +0x1C */
    s32 x;                  /* +0x20 */
    s32 y;                  /* +0x24 */
    s16 axis[3];            /* +0x28 */
    s16 pad_2E;
    SVECTOR a[3];           /* +0x30 */
    SVECTOR b[3];           /* +0x48 */
} TM2ViewMode;

extern TM2ViewMode viewModeTable[];
extern MATRIX viewAxisMatrixTemplate;
extern MATRIX viewAxisScaleMatrix;
extern s32 viewEyePositions[][3];
extern MATRIX viewMatrices[];

/* Per-view arrays: TM2_MAX_VIEWS entries (2 in retail; players.h). */
extern TM2ViewObj *viewCameraObjects[TM2_MAX_VIEWS];
extern TM2ViewEnv *viewEnvs[];
extern s32 viewModes[];
extern TM2ViewObj *viewFollowObjects[TM2_MAX_VIEWS];
extern TM2ViewObj *viewSharedRoot;
extern s32 viewFov;
extern s32 viewRenderSplitMode;
/* Background colour r, g, b. */
extern s32 viewBgColor[3];
extern char strViewOverMax[];

extern int printf(const char *, ...);
extern void *memcpy(void *, const void *, int);
extern TM2ViewObj *csAlloc(void);
extern void csSetModel(TM2ViewObj *, s32);
extern TM2ViewOut *csGetRoot(void);
extern void sub_8014A274(s32);
extern void mathMulTransVec(MATRIX *, VECTOR *, VECTOR *);
extern void mathMulMatVec2(MATRIX *, s32 *, long *);

void viewLoadMode(int mode, int i);
void viewCopyWords(s32 *dst, s32 *src, int n);

/* @80155634 */
s32 __attribute__((section(".text_80155634"))) viewGetProjScaleY(int i)
{
    return viewModeTable[viewModes[i]].fC;
}

/* @8015576C: next view mode (0..6) for each active view. */
void __attribute__((section(".text_8015576C"))) viewCycleMode(void)
{
    int i;

    for (i = 0; i < TM2_MAX_VIEWS; i++) {
        if (viewCameraObjects[i]) {
            ++viewModes[i];
            if (viewModes[i] >= 7)
                viewModes[i] = 0;
            viewLoadMode(viewModes[i], i);
        }
    }
}

/* @8015580C */
void __attribute__((section(".text_8015580C"))) viewInit(void)
{
    int i;

    for (i = 0; i < TM2_MAX_VIEWS; i++) {
        viewCameraObjects[i] = 0;
        viewEnvs[i] = 0;
        viewModes[i] = 1;
        viewFollowObjects[i] = 0;
    }
    viewSharedRoot = 0;
    viewBgColor[2] = 0;
    viewBgColor[1] = 0;
    viewBgColor[0] = 0;
    viewRenderSplitMode = 0;
}

/* @80155888 */
void __attribute__((section(".text_80155888"))) viewCreate(TM2ViewEnv *env, int mode, int i)
{
    s32 w, h, x, y, dw, dh;

    if (i >= TM2_MAX_VIEWS) {
        printf(strViewOverMax);
        return;
    }
    viewModes[i] = mode;
    viewEnvs[i] = env;
    w = viewModeTable[mode].w;
    h = viewModeTable[mode].h;
    x = viewModeTable[mode].x;
    y = viewModeTable[mode].y;
    dw = viewModeTable[mode].disp_w;
    dh = viewModeTable[mode].disp_h;
    SetDefDrawEnv(&env->b[0].draw, x, y, w, h);
    SetDefDispEnv(&env->b[1].disp, 0, 0, dw, dh);
    if (dw == 640) {
        SetDefDrawEnv(&env->b[1].draw, x, y, w, h);
        SetDefDispEnv(&env->b[0].disp, 0, 0, 640, dh);
    } else {
        SetDefDrawEnv(&env->b[1].draw, x + dw, y, w, h);
        SetDefDispEnv(&env->b[0].disp, dw, 0, dw, dh);
    }
    sub_8014A274(viewModeTable[mode].scale);
    SetGeomOffset(w / 2, h / 2);
    env->b[0].draw.isbg = 1;
    env->b[1].draw.isbg = 1;
    env->b[0].draw.dtd = 0;
    env->b[1].draw.dtd = 0;
    env->b[0].draw.r0 = viewBgColor[0];
    env->b[0].draw.g0 = viewBgColor[1];
    env->b[0].draw.b0 = viewBgColor[2];
    env->b[1].draw.r0 = viewBgColor[0];
    env->b[1].draw.g0 = viewBgColor[1];
    env->b[1].draw.b0 = viewBgColor[2];
    if (!viewSharedRoot)
        viewSharedRoot = csAlloc();
    if (!viewCameraObjects[i])
        viewCameraObjects[i] = csAlloc();
    viewCameraObjects[i]->parent = viewSharedRoot;
    viewLoadMode(mode, i);
}

/* @80155B30: load a mode's projection and base matrices into view i. */
void __attribute__((section(".text_80155B30"))) viewLoadMode(int mode, int i)
{
    TM2ViewMode *m = &viewModeTable[mode];
    s32 w = m->disp_w;
    s32 h = m->disp_h;

    sub_8014A274(m->scale);
    SetGeomOffset(w / 2, h / 2);
    viewAxisMatrixTemplate.m[0][0] = m->axis[0];
    viewAxisMatrixTemplate.m[1][2] = -m->axis[2];
    viewAxisMatrixTemplate.m[2][1] = m->axis[1];
    viewAxisScaleMatrix = viewAxisMatrixTemplate;
    viewCameraObjects[i]->m40.m[0][0] = m->a[0].vx;
    viewCameraObjects[i]->m40.m[1][0] = m->a[0].vy;
    viewCameraObjects[i]->m40.m[2][0] = m->a[0].vz;
    viewCameraObjects[i]->m40.m[0][1] = m->a[1].vx;
    viewCameraObjects[i]->m40.m[1][1] = m->a[1].vy;
    viewCameraObjects[i]->m40.m[2][1] = m->a[1].vz;
    viewCameraObjects[i]->m40.m[0][2] = m->a[2].vx;
    viewCameraObjects[i]->m40.m[1][2] = m->a[2].vy;
    viewCameraObjects[i]->m40.m[2][2] = m->a[2].vz;
    viewCameraObjects[i]->m40.t[0] = 0;
    viewCameraObjects[i]->m40.t[1] = 0;
    viewCameraObjects[i]->m40.t[2] = 0;
    viewCameraObjects[i]->m60.m[0][0] = m->b[0].vx;
    viewCameraObjects[i]->m60.m[1][0] = m->b[0].vy;
    viewCameraObjects[i]->m60.m[2][0] = m->b[0].vz;
    viewCameraObjects[i]->m60.m[0][1] = m->b[1].vx;
    viewCameraObjects[i]->m60.m[1][1] = m->b[1].vy;
    viewCameraObjects[i]->m60.m[2][1] = m->b[1].vz;
    viewCameraObjects[i]->m60.m[0][2] = m->b[2].vx;
    viewCameraObjects[i]->m60.m[1][2] = m->b[2].vy;
    viewCameraObjects[i]->m60.m[2][2] = m->b[2].vz;
    viewCameraObjects[i]->m60.t[0] = 0;
    viewCameraObjects[i]->m60.t[1] = 0;
    viewCameraObjects[i]->m60.t[2] = 0;
    viewFov = m->f0;
}

/* @80155DAC */
s32 __attribute__((section(".text_80155DAC"))) viewGetFov(void)
{
    return viewFov;
}

/* @80155E78 */
void __attribute__((section(".text_80155E78"))) nullsub_22(void)
{
}

/* @80155E80: build view i's camera matrices for this frame. */
void __attribute__((section(".text_80155E80"))) viewProc(int i)
{
    MATRIX m, pm, m40, m60;
    VECTOR eye, neg;
    TM2ViewObj *v;
    TM2ViewOut *out;

    if (viewModes[i] == 4) {
        viewCopyWords((s32 *)&viewCameraObjects[1]->rot, (s32 *)&viewCameraObjects[0]->rot, 3);
        viewCopyWords(viewCameraObjects[1]->pos, viewCameraObjects[0]->pos, 3);
        viewCopyWords((s32 *)&viewCameraObjects[1]->parent->rot, (s32 *)&viewCameraObjects[0]->parent->rot, 3);
        viewCopyWords(viewCameraObjects[1]->parent->pos, viewCameraObjects[0]->parent->pos, 3);
        viewCameraObjects[1]->rot.vz += 0x800;
        viewCameraObjects[1]->rot.vx -= 56;
#ifdef TM2_PORT_VISUAL
        /* PC port request 99 (portVisual bit 9, RENDER_ANSWERS.md R9): the eye on the car's roof, looking back */
        portVisualMirrorEye(1, viewCameraObjects[1]->pos, (short *)&viewCameraObjects[1]->rot);
#endif
    }
    v = viewCameraObjects[i];
    out = csGetRoot();
    RotMatrixYXZ(&v->rot, &v->rotm);
    RotMatrixYXZ(&v->parent->rot, &v->parent->rotm);
    viewAxisScaleMatrix.m[0][0] = viewModeTable[viewModes[i]].axis[0];
    viewAxisScaleMatrix.m[1][2] = -viewModeTable[viewModes[i]].axis[2];
    viewAxisScaleMatrix.m[2][1] = viewModeTable[viewModes[i]].axis[1];
    SetRotMatrix(&viewAxisScaleMatrix);
    SetTransMatrix(&viewAxisScaleMatrix);
    MulRotMatrix0(&v->rotm, &m);
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    MulRotMatrix0(&v->parent->rotm, &out->view);
    neg.vx = -v->pos[0];
    neg.vy = -v->pos[1];
    neg.vz = -v->pos[2];
    mathMulTransVec(&v->parent->rotm, &neg, &eye);
    out->eye[0] = eye.vx - v->parent->pos[0];
    out->eye[1] = eye.vy - v->parent->pos[1];
    out->eye[2] = eye.vz - v->parent->pos[2];
    mathMulMatVec2(&out->view, out->eye, out->view.t);
    viewCopyWords(viewEyePositions[i], out->eye, 3);
    viewCopyWords((s32 *)&viewMatrices[i], (s32 *)&out->view, 8);
    m.t[0] = 0;
    m.t[1] = 0;
    m.t[2] = 0;
    pm.t[0] = 0;
    pm.t[1] = 0;
    pm.t[2] = 0;
    m40.t[0] = 0;
    m40.t[1] = 0;
    m40.t[2] = 0;
    m60.t[0] = 0;
    m60.t[1] = 0;
    m60.t[2] = 0;
    TransposeMatrix(&v->rotm, &m);
    TransposeMatrix(&v->parent->rotm, &pm);
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    MulRotMatrix0(&v->m40, &m40);
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    MulRotMatrix0(&v->m60, &m60);
    SetRotMatrix(&pm);
    SetTransMatrix(&pm);
    MulRotMatrix0(&m40, &out->m40);
    SetRotMatrix(&pm);
    SetTransMatrix(&pm);
    MulRotMatrix0(&m60, &out->m60);
    sub_8014A274(viewModeTable[viewModes[i]].scale);
    SetGeomOffset(viewModeTable[viewModes[i]].w / 2, viewModeTable[viewModes[i]].h / 2);
}

/* @801562B8 */
s32 __attribute__((section(".text_801562B8"))) viewGetMode(int i)
{
    return viewModes[i];
}

/* @801562D4: attach view i to an object (0: back to the root object). */
void __attribute__((section(".text_801562D4"))) viewSetParent(TM2ViewObj *p, int i)
{
    if (p && viewCameraObjects[i])
        viewCameraObjects[i]->parent = p;
    else if (viewCameraObjects[i])
        viewCameraObjects[i]->parent = viewSharedRoot;
}

/* @80156344 */
void __attribute__((section(".text_80156344"))) viewSetFollowObj(TM2ViewObj *obj, s32 x)
{
    viewFollowObjects[0] = obj;
    obj->f0 = 0xFFF;
    viewRenderSplitMode = x;
    viewFollowObjects[1] = csAlloc();
    viewFollowObjects[1]->f0 = 0xFFF;
    csSetModel(viewFollowObjects[1], obj->f11C);
}

/* @801563A0 */
TM2ViewObj *__attribute__((section(".text_801563A0"))) viewGetFollowObj(int i)
{
    return viewFollowObjects[i];
}

/* @801563BC */
void __attribute__((section(".text_801563BC"))) viewSetFollowObjPos(s32 *pos, int i)
{
    if (viewFollowObjects[i]) {
        viewFollowObjects[i]->pos[0] = pos[0];
        viewFollowObjects[i]->pos[1] = pos[1];
        viewFollowObjects[i]->pos[2] = pos[2];
    }
}

/* @80156414 */
void __attribute__((section(".text_80156414"))) viewGetRotation(SVECTOR *dst, int i)
{
    memcpy(dst, &viewCameraObjects[i]->rot, 8);
}

/* @80156484 */
void __attribute__((section(".text_80156484"))) viewGetPosition(s32 *dst, int i)
{
    memcpy(dst, viewCameraObjects[i]->pos, 12);
}

/* @801564D8 */
void __attribute__((section(".text_801564D8"))) viewSetRotation(SVECTOR *src, int i)
{
    if (viewCameraObjects[i])
        memcpy(&viewCameraObjects[i]->rot, src, 8);
}

/* @80156528 */
void __attribute__((section(".text_80156528"))) viewSetPosition(s32 *src, int i)
{
    if (viewCameraObjects[i])
        memcpy(viewCameraObjects[i]->pos, src, 12);
}

/* @8015659C: background colour of both views. */
void __attribute__((section(".text_8015659C"))) viewSetBgColor(s32 r, s32 g, s32 b)
{
    int i;

    viewBgColor[0] = r;
    viewBgColor[1] = g;
    viewBgColor[2] = b;
    for (i = 0; i < TM2_MAX_VIEWS; i++) {
        if (viewCameraObjects[i]) {
            TM2ViewEnv *env = viewEnvs[i];
            env->b[0].draw.r0 = r;
            env->b[0].draw.g0 = g;
            env->b[0].draw.b0 = b;
            env->b[1].draw.r0 = r;
            env->b[1].draw.g0 = g;
            env->b[1].draw.b0 = b;
        }
    }
}

/* @80156614: view i's screen centre. */
void __attribute__((section(".text_80156614"))) viewGetCentre(int i, s32 *x, s32 *y)
{
    *x = viewModeTable[viewModes[i]].w / 2;
    *y = viewModeTable[viewModes[i]].h / 2;
}

/* @80156694 */
s32 *__attribute__((section(".text_80156694"))) viewGetEyeTrans(int i)
{
    return viewEyePositions[i];
}

/* @801566B0 */
MATRIX *__attribute__((section(".text_801566B0"))) viewGetViewMatrix(int i)
{
    return &viewMatrices[i];
}

/* @801566C4 */
void __attribute__((section(".text_801566C4"))) viewCopyWords(s32 *dst, s32 *src, int n)
{
    int i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
}

/* @801566F8: a fixed top-down/side camera for view i. */
void __attribute__((section(".text_801566F8"))) viewSetFixedCamera(int i, s32 dist, int side)
{
    TM2ViewObj *v = viewCameraObjects[i];
    TM2ViewOut *out = csGetRoot();

    out->eye[0] = 0;
    if (!side) {
        out->eye[1] = 0;
        out->eye[2] = dist;
        out->view.m[0][0] = 0x1000;
        out->view.m[0][1] = 0;
        out->view.m[0][2] = 0;
        out->view.m[1][0] = 0;
        out->view.m[1][1] = -0x1000;
        out->view.m[1][2] = 0;
        out->view.m[2][0] = 0;
        out->view.m[2][1] = 0;
        out->view.m[2][2] = 0x1000;
    } else {
        out->view.m[0][0] = 0x1000;
        out->view.m[0][1] = 0;
        out->view.m[0][2] = 0;
        out->view.m[1][0] = 0;
        out->view.m[1][1] = 0;
        out->view.m[1][2] = -0x1000;
        out->view.m[2][0] = 0;
        out->view.m[2][1] = 0x1000;
        out->view.m[2][2] = 0;
        out->eye[1] = dist;
        out->eye[2] = 0;
    }
    sub_8014A274(1);
    SetGeomOffset(viewModeTable[viewModes[i]].w / 2, viewModeTable[viewModes[i]].h / 2);
    viewCopyWords((s32 *)&out->m40, (s32 *)&v->m40, 8);
    viewCopyWords((s32 *)&out->m60, (s32 *)&v->m60, 8);
}
