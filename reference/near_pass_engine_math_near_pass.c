/* Pure-C version of src/semantic/engine_math_near_pass.c whose
 * SingleTrac hand-written asm functions (renderNearPolys 80154690, renderNearSplitTri
 * 80154E30, renderNearLinkPrims 80154E84, renderNearSplitQuad 80155088) are rewritten
 * as C; the other functions are the same C as src/ (byte-identical). Same behaviour
 * as the retail asm on random polygon lists.
 * Linked only by the all-C relocatable flavour (tools/build_source_only.py
 * --relocatable --all-c, docs/SOURCE_BUILD.md "All-C flavour"), compiled with
 * the BUILD_MANIFEST.json recipe of the src/ unit it replaces; the hybrid and the
 * fixed source-only builds keep the asm (docs/RULES.md). Static proof only.
 * ABI: renderNearPolys(model, list, n) is called by compiled C (renderScene,
 * renderDrawModel) with a0..a2, as the asm expects; it reads the emitter's near list
 * at 0x1F8000A4 and the draw modes at 0x1F800060/6C as the asm does. The asm saved
 * s0-s7, t8, t9, ra, fp, gp, sp at 0x1F800000..0x1F800034 and a0/a1 at 0x40/0x4C;
 * the C does not (stack), so those scratchpad words differ afterwards. The three
 * subroutines had private register arguments; here they take a NearCtx. */

/* Library code 80154000..8015C000: SingleTrac engine part (math helpers next to
 * mathMulTransVec, the near-polygon renderer pass and its subroutines, the
 * view-mode accessors, angle wrapping/blending, sound start/stop wrappers).
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md. All code here was
 * written against the retail disassembly; no third-party source was used.
 *
 * Recipe: like the game (psyq3.6 cc1, -fvolatile, extern_gp, gp_size 8).
 * The libsnd functions of this range (GCC 2.5.7 recipe) are in a separate
 * unit. renderNearPolys/80154E30/80154E84/80155088 (the near-polygon
 * subdivision pass and its subroutines) were SingleTrac hand-written
 * assembly (registers saved in the scratchpad, internal jal subroutines
 * with register arguments); they are written here as plain C and do not
 * reproduce the retail bytes. */

#ifndef TM2_NEAR_PASS_ONLY
/* (TM2_NEAR_PASS_ONLY: src/semantic/engine_math_near_pass.c includes only the near pass below, in the port variant
 * with TM2_PORT_C_EMITTER, request 82; the rest of that unit is its own) */
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/* View mode table entry (8017EC44, 96 bytes; see view_system.c). */
typedef struct {
    s32 f0;
    s32 f4;
    s32 scale;              /* +0x08 */
    s32 fC;
    u8 pad_10[0x50];
} TM2ViewMode;

extern TM2ViewMode viewModeTable[];
extern s32 viewModes[];
extern s32 viewRenderSplitMode;

/* sound settings (gp block 80180DEC..80180E48) */
extern s32 D_80180DEC, soundFineMax, soundFineStepsPerNote, soundInstPitchScale, D_80180DFC, soundNoteMax;
extern s32 D_80180E04, D_80180E08, audioCdVolume, D_80180E10, audioMusicVolumeCap, audioSfxVolume;
extern s32 D_80180E1C, audioSfxVolumeCap, audioVolumeClampMax, audioVolumeClampMin, audioMasterVolume, D_80180E30;
extern s32 D_80180E34, D_80180E3C, D_80180E44, D_80180E48;

extern s32 sub_8013FC44(s32);              /* square root */
extern s32 timerGetFrameRate(void);             /* ticks of the last frame */
extern void sub_8015700C(void);            /* SsInit */
extern void sub_801583C8(s32);             /* SsSetReservedVoice */
extern void sub_8015D640(s32);             /* SpuSetReverb */
extern void sub_80157DD4(s32);             /* SsSetTickMode */
extern void sub_80157380(void);            /* SsStart */
extern void sub_80156EA8(s16, s16);        /* SsSetMVol */
extern void sub_80156E08();                /* SsGetMVol (SndVolume *) */
extern void sub_801583B8(void);            /* SsSetStereo */
extern void sub_8015D5F0(s32);
extern void sub_80157F48(void);            /* SsEnd */
extern void sub_8015704C(void);            /* SsQuit */

void mathBoundAngle(s32 *a);

/* One output component: three halfword-matrix by word-vector products,
 * summed and scaled down by 4096 (as math_vec.c). */
#define MAT_ROW(m, a, b, c, input) (((input)[0] * (m)[a] + (input)[1] * (m)[b] + (input)[2] * (m)[c]) / 4096)

/* @8015424C: 3x3 halfword matrix times word vector (row-major). */
int __attribute__((section(".text_8015424C"))) mathMulMatVec2(short *m, int *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 1, 2, input);
    output[1] = MAT_ROW(m, 3, 4, 5, input);
    output[2] = MAT_ROW(m, 6, 7, 8, input);
}

/* @80154380 mathMulTransVecShort: transposed matrix times halfword vector. */
int __attribute__((section(".text_80154380"))) mathMulTransVecShort(short *m, short *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 3, 6, input);
    output[1] = MAT_ROW(m, 1, 4, 7, input);
    output[2] = MAT_ROW(m, 2, 5, 8, input);
}

/* @801544FC mathNormalizeVec: out = in scaled to length 4096 (zero vector
 * when the length is not positive). */
void __attribute__((section(".text_801544FC"))) mathNormalizeVec(s32 *in, s32 *out)
{
    s32 len;

    len = sub_8013FC44(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
    if (len > 0) {
        out[0] = (in[0] << 12) / len;
        out[1] = (in[1] << 12) / len;
        out[2] = (in[2] << 12) / len;
    } else {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
    }
}

/* @8015465C: copy n words. */
int __attribute__((section(".text_8015465C"))) mathCopyWords(s32 *dst, s32 *src, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
}

/* @801555C4: projection scale of view i's mode. */
s32 __attribute__((section(".text_801555C4"))) viewGetProjH(int i)
{
    return viewModeTable[viewModes[i]].scale;
}

/* @801555FC: the same (second entry point). */
s32 __attribute__((section(".text_801555FC"))) viewGetProjScaleX(int i)
{
    return viewModeTable[viewModes[i]].scale;
}

/* @80156860 */
s32 __attribute__((section(".text_80156860"))) viewGetRenderSplitMode(void)
{
    return viewRenderSplitMode;
}

/* @8015686C mathBoundVector: wrap three angles into [-2048, 2048). */
void __attribute__((section(".text_8015686C"))) mathBoundVector(s32 *v)
{
    while (v[0] >= 2048)
        v[0] -= 4096;
    while (v[0] < -2048)
        v[0] += 4096;
    while (v[1] >= 2048)
        v[1] -= 4096;
    while (v[1] < -2048)
        v[1] += 4096;
    while (v[2] >= 2048)
        v[2] -= 4096;
    while (v[2] < -2048)
        v[2] += 4096;
}

/* @80156ABC mathBoundAngle: wrap an angle into [-2048, 2048). */
void __attribute__((section(".text_80156ABC"))) mathBoundAngle(s32 *a)
{
    while (*a >= 2048)
        *a -= 4096;
    while (*a < -2048)
        *a += 4096;
}

/* @80156B1C mathBoundSAngle: mathBoundAngle for a halfword angle. */
void __attribute__((section(".text_80156B1C"))) mathBoundSAngle(s16 *a)
{
    while (*a >= 2048)
        *a -= 4096;
    while (*a < -2048)
        *a += 4096;
}

/* @80156B8C: move angle towards target (pct% of the old angle per 30 ticks,
 * along the shorter way round), wrapped. */
s32 __attribute__((section(".text_80156B8C"))) mathBlendAngle(s32 angle, s32 target, s32 pct)
{
    s32 r;

    if (angle < -1024 && target > 1024)
        target -= 4096;
    else if (angle > 1024 && target < -1024)
        target += 4096;
    r = (angle * pct + target * (100 - pct)) / 100;
    r = angle + (r - angle) * timerGetFrameRate() / 30;
    mathBoundAngle(&r);
    return r;
}

/* @80156C70: the same blend for a plain value. */
s32 __attribute__((section(".text_80156C70"))) mathBlendValue(s32 value, s32 target, s32 pct)
{
    s32 r;

    r = (value * pct + target * (100 - pct)) / 100;
    return value + (r - value) * timerGetFrameRate() / 30;
}

/* @80156D08 soundInit: default sound settings, then start libsnd. */
void __attribute__((section(".text_80156D08"))) soundInit(void)
{
    audioMasterVolume = 127;
    D_80180E44 = 24;
    D_80180E34 = 24;
    D_80180E30 = 45;
    D_80180E08 = 99;
    audioVolumeClampMin = 0;
    audioSfxVolumeCap = 127;
    D_80180E1C = 0;
    audioMusicVolumeCap = 127;
    D_80180E10 = 0;
    soundNoteMax = 127;
    D_80180DFC = 0;
    soundFineMax = 127;
    D_80180DEC = 0;
    D_80180E3C = 0;
    D_80180E04 = 0;
    soundInstPitchScale = 128;
    soundFineStepsPerNote = 128;
    audioVolumeClampMax = 127;
    audioSfxVolume = 127;
    audioCdVolume = 127;
    sub_8015700C();
    sub_801583C8((u8)D_80180E34);
    sub_8015D640(0);
    sub_80157DD4(1);
    sub_80157380();
    sub_80156EA8(audioMasterVolume, audioMasterVolume);
    sub_80156E08(D_80180E48);
    sub_801583B8();
    sub_8015D5F0(D_80180E30);
}

/* @80156DE0: stop libsnd. */
void __attribute__((section(".text_80156DE0"))) soundQuit(void)
{
    sub_80157F48();
    sub_8015704C();
}

#endif /* TM2_NEAR_PASS_ONLY */

/* ---- near-polygon subdivision pass (hand-written assembly in retail) ---- */

typedef signed char s8;
typedef unsigned short u16;
typedef unsigned int u32;

/* Near-polygon list entry (12 bytes), written by the model emitter: the
 * first 71 in the scratchpad at 0x1F8000A4, the rest in the list passed in. */
typedef struct {
    u8 *face;           /* face record: +0 flags, +1 shading, +3 primitive words, +12 flags */
    u8 *prim;           /* its primitive in the packet buffer */
    s16 pad8;
    s16 otz;            /* +10 OT index, negative: own draw modes, index -otz */
} NearEntry;

/* Leading words of the draw model record passed by the renderer. */
typedef struct {
    u32 *ot;            /* +0 ordering table */
    s32 f4;
    u8 *pkt;            /* +8 next free packet byte */
    u8 *end;            /* +12 packet buffer end */
} NearModel;

/* State shared by the pass and its three subroutines (registers in retail). */
typedef struct {
    NearModel *model;
    NearEntry *e;
    u8 *face;
    u32 flags;          /* face word +12 */
    s32 len;            /* primitive words, tag included */
    s32 stride;         /* bytes from one vertex to the next */
    s32 texoff;         /* 12 triangle, 16 quad */
    u8 *q[4];           /* the primitive, then the three new ones */
    u32 *v[4];          /* field of vertex 0..3 of the primitive */
    u32 org[4];         /* original field of vertex 1..3 */
    u32 mid[5];         /* new field values (see the users) */
} NearCtx;

/* Draw modes left in the scratchpad by the renderer (0x60, 0x6C) and the
 * last texture page seen, kept in the scratchpad word 0x54 as retail does
 * (it is read for faces with flag bit 7, possibly from an earlier entry). */
#define SPAD_MODE_A ((u32 *)0x1F800060)
#define SPAD_MODE_B ((u32 *)0x1F80006C)
#define SPAD_TPAGE (*(s32 *)0x1F800054)

#define W32(p, o) (*(u32 *)((u8 *)(p) + (o)))

/* Average of byte field sh of a and b, in place. */
#define MIX(a, b, sh) ((((((a) >> (sh)) & 0xFF) + (((b) >> (sh)) & 0xFF)) >> 1) << (sh))
/* Midpoints of packed screen xy (16:16), uv (8:8) and rgb (8:8:8) words. */
#define MIDXY(a, b) (((((a) >> 16) + ((b) >> 16)) >> 1) << 16 | (((a) & 0xFFFF) + ((b) & 0xFFFF)) >> 1)
#define MIDUV(a, b) (MIX(a, b, 8) | MIX(a, b, 0))
#define MIDRGB(a, b) (MIX(a, b, 0) | MIX(a, b, 8) | MIX(a, b, 16))

/* Add primitive p of n words at the head of OT entry ot (keeping the
 * entry's top byte). */
#define LINK(ot, p, n) do {                                     \
        u32 *o_ = (ot);                                         \
        u8 *p_ = (u8 *)(p);                                     \
        s8 top_ = ((s8 *)o_)[3];                                \
        *(u32 *)p_ = *o_;                                       \
        p_[3] = (n);                                            \
        *o_ = (u32)p_;                                          \
        ((u8 *)o_)[3] = top_;                                   \
    } while (0)

void renderNearSplitTri(NearCtx *c, s32 off);
void renderNearLinkPrims(NearCtx *c);
void renderNearSplitQuad(NearCtx *c, s32 off);

#ifdef TM2_PORT_C_EMITTER
/* PC port request 82 (port_c_emitter.h, PLAYTEST_ANSWERS.md 27): with portPreciseEmit set, every vertex word the split
 * writes goes to the port's precise ring with its place (1/16 pixel) and depth, from the corners' depths the emitter
 * kept (portNearDepth). Drawing only; the packets are as before. */
#include "semantic/port_c_emitter.h"
#define PN_TEXT __attribute__((section(".port_text")))

/* The depth of the screen-space midpoint of two points of depth a and b (1 / z linear on the screen): 2ab / (a + b). */
static s32 PN_TEXT portNearMidZ(s32 a, s32 b)
{
    s32 s = 0;

    if (a <= 0 || b <= 0)
        return a > b ? a : b;
    while (a > 0xFFFF || b > 0xFFFF) {
        a >>= 1;
        b >>= 1;
        s++;
    }
    return (s32)(((u32)a * (u32)b / (u32)(a + b)) * 2) << s;
}

/* One ring entry (port_c_emitter.h): the word at p, x and y in 1/16 pixel, depth z. */
static void PN_TEXT portNearEntry(u8 *p, s32 x16, s32 y16, s32 z)
{
    u32 *ring = (u32 *)PORT_PRECISE_RING;
    u32 n = ring[0];

    if (n < PORT_PRECISE_MAX) {
        ring[1 + n * 4] = (u32)p;
        ring[2 + n * 4] = (u32)x16;
        ring[3 + n * 4] = (u32)y16;
        ring[4 + n * 4] = (u32)z;
        ring[0] = n + 1;
    }
}

/* Near entry k just split (its xy fields written: renderNearSplitTri / Quad at offset 8), its corners' packet words
 * w[0..3] as they were. Points: 0..3 the corners; a triangle's 4 m01, 5 m12, 6 m20; a quad's 4 m02, 5 m01, 6 m13,
 * 7 m23, 8 the centre. Each primitive's vertices are the points of the split's table (the subroutines' comments). */
static void PN_TEXT portNearReport(NearCtx *c, s32 k, u32 *w, s32 quad)
{
    /* the split's tables as nibbles (no .rodata): a primitive's vertex i is point (row >> 4i) & 15 (15: its word not
     * written), a midpoint i's ends (A >> 4i) & 15 and (B >> 4i) & 15 */
    s32 x[9], y[9], z[9], i, j, n, a, b, p, row;
    if (k >= PORT_NEAR_MAX)
        return;
    n = quad ? 4 : 3;
    for (i = 0; i < n; i++) {
        x[i] = (s16)(w[i] & 0xFFFF) * 16;
        y[i] = (s16)(w[i] >> 16) * 16;
        z[i] = portNearDepth[k * 4 + i];
        if (z[i] <= 0)
            return;                 /* no depth kept for it: leave it to the port */
    }
    for (i = 0; i < (quad ? 5 : 3); i++) {
        /* the midpoints' ends: a triangle's m01, m12, m20; a quad's m02, m01, m13, m23 and the centre (m02, m13) */
        a = quad ? (0x43110 >> (i * 4)) & 15 : (0x210 >> (i * 4)) & 15;
        b = quad ? (0x62302 >> (i * 4)) & 15 : (0x021 >> (i * 4)) & 15;
        x[4 + i] = (x[a] + x[b]) / 2;
        y[4 + i] = (y[a] + y[b]) / 2;
        z[4 + i] = portNearMidZ(z[a], z[b]);
    }
    for (j = 0; j < 4; j++) {
        /* q[0] (v0, m20, m01), q[1] (m12, v1, m01), q[2] (m12, m20, v2), q[3] (m12, m20, m01); quads: q[0] (v0, m01,
           m02, c), q[1] (m02, c, v2, m23), q[2] (m01, v1, c, m13), q[3] (c, m13, m23, v3) */
        if (quad)
            row = j == 0 ? 0x845F : j == 1 ? 0x7284 : j == 2 ? 0x6815 : 0x3768;
        else
            row = j == 0 ? 0xF46F : j == 1 ? 0xF415 : j == 2 ? 0xF265 : 0xF465;
        for (i = 0; i < n; i++) {
            p = (row >> (i * 4)) & 15;
            if (p != 15)
                portNearEntry(c->q[j] + 8 + i * c->stride, x[p], y[p], z[p]);
        }
    }
}
#endif

/* @80154690: split every near polygon (the emitter's list, n entries) into
 * four: the primitive keeps one corner, three new primitives built after it
 * in the packet buffer take the rest; screen xy, uv (textured faces) and
 * colours (gouraud faces) are interpolated at the edge midpoints. Stops
 * when the packet buffer would overflow. */
void __attribute__((section(".text_80154690"))) renderNearPolys(NearModel *model, NearEntry *list, s32 n)
{
    NearCtx c;
    NearEntry *e;
    u8 *end, *prim;
    u32 *v0;
    u32 a, b, x, d;
    s32 k;

    c.model = model;
    end = model->end;
    e = (NearEntry *)0x1F8000A4;
    for (k = 0; n > 0; n--) {
        c.e = e;
        c.face = e->face;
        prim = e->prim;
        c.q[0] = prim;
        c.flags = W32(c.face, 12);
        c.stride = ((c.flags >> 22) & 3) * 4;
        c.len = c.face[3] + 1;
        c.q[1] = model->pkt;
        c.q[2] = c.q[1] + c.len * 4;
        c.q[3] = c.q[2] + c.len * 4;
        if ((u32)end < (u32)(c.q[3] + 160))
            break;
        v0 = (u32 *)(prim + 8);
        c.v[1] = (u32 *)((u8 *)v0 + c.stride);
        c.v[2] = (u32 *)((u8 *)c.v[1] + c.stride);
        if (((c.flags >> 16) & 7) == 3) {
            /* triangle: corners 0 (the primitive), 1, 2 and the centre */
            c.texoff = 12;
            a = *v0;
            b = *c.v[1];
            x = *c.v[2];
            c.mid[0] = MIDXY(a, b);
            c.mid[1] = MIDXY(b, x);
            c.mid[2] = MIDXY(x, a);
            c.org[1] = b;
            c.org[2] = x;
            renderNearSplitTri(&c, 8);
#ifdef TM2_PORT_C_EMITTER
            if (PORT_PRECISE_EMIT) {
                u32 w_[4];
                w_[0] = a;
                w_[1] = b;
                w_[2] = x;
                w_[3] = 0;
                portNearReport(&c, k, w_, 0);   /* request 82 */
            }
#endif
            if (W32(c.face, 0) & 0x7F) {
                /* uv, with the clut on vertex 0 and the tpage on vertex 1 */
                v0++;
                c.v[1]++;
                c.v[2]++;
                a = *v0;
                b = *c.v[1];
                x = *c.v[2];
                c.mid[0] = MIDUV(a, b);
                c.mid[1] = MIDUV(b, x);
                c.mid[2] = MIDUV(x, a);
                SPAD_TPAGE = ((s16 *)c.v[1])[1];
                c.mid[1] |= ((s16 *)v0)[1] << 16;
                c.mid[2] |= SPAD_TPAGE << 16;
                c.org[1] = b;
                c.org[2] = x;
                renderNearSplitTri(&c, 12);
            }
            v0 = (u32 *)(prim + 4);
            c.v[1] = (u32 *)((u8 *)v0 + c.stride);
            if (c.face[1] < 2 && ((c.flags >> 19) & 7) < 2)
                goto flat;
            /* gouraud: the code byte of vertex 2 goes with the centre */
            c.v[2] = (u32 *)((u8 *)c.v[1] + c.stride);
            a = *v0;
            b = *c.v[1];
            x = *c.v[2];
            c.mid[0] = MIDRGB(a, b);
            c.mid[1] = MIDRGB(b, x) | (x & 0xFF000000);
            c.mid[2] = MIDRGB(x, a);
            c.org[1] = b;
            c.org[2] = x;
            renderNearSplitTri(&c, 4);
        } else {
            /* quad: the four quarters (edges 0-2, 0-1, 1-3, 2-3, centre) */
            c.texoff = 16;
            c.v[3] = (u32 *)((u8 *)c.v[2] + c.stride);
            a = *v0;
            b = *c.v[1];
            x = *c.v[2];
            d = *c.v[3];
            c.mid[0] = MIDXY(a, x);
            c.mid[1] = MIDXY(b, a);
            c.mid[2] = MIDXY(b, d);
            c.mid[3] = MIDXY(d, x);
            c.mid[4] = MIDXY(c.mid[0], c.mid[2]);
            c.org[1] = b;
            c.org[2] = x;
            c.org[3] = d;
            renderNearSplitQuad(&c, 8);
#ifdef TM2_PORT_C_EMITTER
            if (PORT_PRECISE_EMIT) {
                u32 w_[4];
                w_[0] = a;
                w_[1] = b;
                w_[2] = x;
                w_[3] = d;
                portNearReport(&c, k, w_, 1);   /* request 82 */
            }
#endif
            if (W32(c.face, 0) & 0x7F) {
                v0++;
                c.v[1]++;
                c.v[2]++;
                c.v[3]++;
                a = *v0;
                b = *c.v[1];
                x = *c.v[2];
                d = *c.v[3];
                c.mid[0] = MIDUV(a, x);
                c.mid[1] = MIDUV(b, a);
                c.mid[2] = MIDUV(b, d);
                c.mid[3] = MIDUV(d, x);
                c.mid[4] = MIDUV(c.mid[0], c.mid[2]);
                SPAD_TPAGE = ((s16 *)c.v[1])[1];
                c.mid[0] = (c.mid[0] & 0xFFFF) | ((s16 *)v0)[1] << 16;
                c.mid[4] = (c.mid[4] & 0xFFFF) | SPAD_TPAGE << 16;
                c.mid[1] = (c.mid[1] & 0xFFFF) | SPAD_TPAGE << 16;
                c.mid[2] = (c.mid[2] & 0xFFFF) | SPAD_TPAGE << 16;
                c.org[1] = b;
                c.org[2] = x;
                c.org[3] = d;
                renderNearSplitQuad(&c, 12);
                /* first vertex of the new quads: the clut */
                ((s16 *)(c.q[2] + 12))[1] = ((s16 *)v0)[1];
                ((s16 *)(c.q[3] + 12))[1] = ((s16 *)v0)[1];
            }
            v0 = (u32 *)(prim + 4);
            c.v[1] = (u32 *)((u8 *)v0 + c.stride);
            if (c.face[1] < 2 && ((c.flags >> 19) & 7) < 2)
                goto flat;
            /* gouraud; retail takes the blue of the 1-3 and 2-3 midpoints
             * from vertices 1+2 and 3+1 */
            c.v[2] = (u32 *)((u8 *)c.v[1] + c.stride);
            c.v[3] = (u32 *)((u8 *)c.v[2] + c.stride);
            a = *v0;
            b = *c.v[1];
            x = *c.v[2];
            d = *c.v[3];
            c.mid[0] = MIDRGB(a, x) | (a & 0xFF000000);
            c.mid[1] = MIDRGB(b, a) | (a & 0xFF000000);
            c.mid[2] = MIX(b, d, 0) | MIX(b, d, 8) | MIX(b, x, 16);
            c.mid[3] = MIX(d, x, 0) | MIX(d, x, 8) | MIX(d, b, 16);
            c.mid[4] = MIDRGB(c.mid[0], c.mid[2]) | (a & 0xFF000000);
            c.org[1] = b;
            c.org[2] = x;
            c.org[3] = d;
            renderNearSplitQuad(&c, 4);
        }
        goto link;
    flat:
        a = *v0;
        W32(c.q[1], 4) = a;
        W32(c.q[2], 4) = a;
        W32(c.q[3], 4) = a;
    link:
        renderNearLinkPrims(&c);
        if (k == 70)
            e = list;
        else
            e++;
        k++;
    }
}

/* @80154E30: store the triangle fields at byte off of each vertex: the new
 * primitives get (m12, v1, m01), (m12, m20, v2), (m12, m20, m01) and the
 * primitive itself becomes (v0, m20, m01). */
void __attribute__((section(".text_80154E30"))) renderNearSplitTri(NearCtx *c, s32 off)
{
    s32 st;
    u8 *p;

    st = c->stride;
    p = c->q[1] + off;
    W32(p, 0) = c->mid[1];
    W32(p, st) = c->org[1];
    W32(p, st * 2) = c->mid[0];
    p = c->q[2] + off;
    W32(p, 0) = c->mid[1];
    W32(p, st) = c->mid[2];
    W32(p, st * 2) = c->org[2];
    p = c->q[3] + off;
    W32(p, 0) = c->mid[1];
    W32(p, st) = c->mid[2];
    W32(p, st * 2) = c->mid[0];
    *c->v[1] = c->mid[2];
    *c->v[2] = c->mid[0];
}

/* @80154E84: link the primitive and the three new ones into the OT. A face
 * with flag bit 7 or a negative OT index gets a draw mode before them
 * (mode A) and one after (mode A, or B for a negative index; flag bit 7
 * patches its texture window and tpage from the face). */
void __attribute__((section(".text_80154E84"))) renderNearLinkPrims(NearCtx *c)
{
    NearModel *m;
    u32 *ot, *d;
    s32 otz, len, i;
    s8 flag;

    flag = c->face[0];
    otz = c->e->otz;
    len = c->len - 1;
    m = c->model;
    if (flag >= 0 && otz >= 0) {
        m->pkt = c->q[3] + c->len * 4;
        ot = m->ot + otz;
        for (i = 0; i < 4; i++)
            LINK(ot, c->q[i], len);
        return;
    }
    d = (u32 *)(c->q[3] + c->len * 4);
    d[0] = SPAD_MODE_A[0];
    d[1] = SPAD_MODE_A[1];
    d[2] = SPAD_MODE_A[2];
    ot = m->ot + (otz < 0 ? -otz : otz);
    LINK(ot, d, 2);
    d += 3;
    for (i = 0; i < 4; i++)
        LINK(ot, c->q[i], len);
    if (otz >= 0) {
        d[0] = SPAD_MODE_A[0];
        d[1] = SPAD_MODE_A[1];
        d[2] = SPAD_MODE_A[2];
    } else {
        d[0] = SPAD_MODE_B[0];
        d[1] = SPAD_MODE_B[1];
        d[2] = SPAD_MODE_B[2];
    }
    if (flag < 0) {
        d[2] = W32(c->face, 16 + ((c->flags >> 28) & 7) * 4 + c->texoff);
        ((u8 *)d)[4] = SPAD_TPAGE;
    }
    LINK(ot, d, 2);
    m->pkt = (u8 *)(d + 3);
}

/* @80155088: store the quad fields at byte off of each vertex (mid = edge
 * 0-2, 0-1, 1-3, 2-3 midpoints and the centre): the new primitives get
 * (m02, c, v2, m23), (m01, v1, c, m13), (c, m13, m23, v3) and the primitive
 * itself becomes (v0, m01, m02, c). */
void __attribute__((section(".text_80155088"))) renderNearSplitQuad(NearCtx *c, s32 off)
{
    s32 st;
    u8 *p;

    st = c->stride;
    p = c->q[1] + off;
    W32(p, 0) = c->mid[0];
    W32(p, st) = c->mid[4];
    W32(p, st * 2) = c->org[2];
    W32(p, st * 3) = c->mid[3];
    p = c->q[2] + off;
    W32(p, 0) = c->mid[1];
    W32(p, st) = c->org[1];
    W32(p, st * 2) = c->mid[4];
    W32(p, st * 3) = c->mid[2];
    p = c->q[3] + off;
    W32(p, 0) = c->mid[4];
    W32(p, st) = c->mid[2];
    W32(p, st * 2) = c->mid[3];
    W32(p, st * 3) = c->org[3];
    *c->v[1] = c->mid[1];
    *c->v[2] = c->mid[0];
    *c->v[3] = c->mid[4];
}
