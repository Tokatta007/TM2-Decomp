/* Pure-C version of src/semantic/model_emitter.c: retail 800CDC54 is
 * hand-written assembly (no frame, state and saved registers in the scratchpad
 * at 0x1F800000), which C cannot reproduce byte for byte. Runtime-verified for the port variant (below).
 * Linked by the all-C relocatable flavour (tools/build_source_only.py
 * --relocatable --all-c, docs/SOURCE_BUILD.md "All-C flavour"), compiled with
 * the BUILD_MANIFEST.json recipe of src/semantic/model_emitter.c; the hybrid
 * and the fixed source-only builds keep the asm (docs/RULES.md). It is 0xD38
 * bytes against the 0xCBC-byte retail slot, so everything after it moves.
 *
 * ABI: both callers are compiled C (renderScene 80152284, renderDrawModel
 * 80151378) and pass the standard o32 arguments: a0 = draw context, a1 = draw
 * entries (D_801AC810), a2 = count, a3 = matrix slots, the 5th (light
 * matrices) at 16($sp) -- exactly what the asm reads (lw $20,16($29)). They set
 * the scratchpad state the emitter reads (0x40 near threshold, 0x4C overflow
 * list, 0x60/0x6C draw modes, 0x88 cull flip) before the call, and the C reads
 * and writes the same Scratch fields as the asm (0x30..0x54, 0x78..0x84, the
 * near list from 0xA0/0xA4, read by renderNearPolys). Differences: the asm
 * saved s0-s7, t8, t9, ra at 0x1F800000..0x1F80002B and spilled a0-a3/s0 at
 * 0x8C..0x9F around the ratan2/rsin/rcos calls; the C uses its stack frame, so
 * those scratchpad words are not written (and t8/t9, caller-saved in the ABI,
 * are not preserved). Its divides use HWDIV below (the raw-div results, no
 * trap). The scratchpad base is the plain constant SCR.
 *
 * The port variant links it too, with TM2_PORT_C_EMITTER (src/semantic/model_emitter.c, PC port request 61
 * addendum): runtime-verified there against the asm on a MIPS interpreter with a GTE model (tools/check_custom.py
 * check 8: the same packets, OT links, near list and scratchpad state on disc meshes and made-up ones), and with
 * portPreciseEmit its beams and sprites also report subpixel corners (include/semantic/port_c_emitter.h). */
/* 800CDC54. Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* sub_800CDC54 is hand-written assembly in retail (no stack frame: it saves
 * s0-s7/t8/t9/ra into the scratchpad and keeps its state there). This is a
 * semantic C reconstruction; the GTE is reached through the genuine
 * lwc2/mtc2/ctc2/cop2/mfc2/swc2 instructions below, everything else is C. */
#ifndef __mips__
#error This unit requires the PSX GTE.
#endif

extern s32 ratan2(s32 y, s32 x);
extern s32 rsin(s32 a);
extern s32 rcos(s32 a);

/* Scratchpad record at 0x1F800000 shared with the caller (80151xxx) and the
 * near-polygon pass renderNearPolys. */
typedef struct {
    s32 save[12];        /* 0x00 retail register save area */
    s32 trans_id;        /* 0x30 cached entry[7] (translation key) */
    s32 rot_id;          /* 0x34 cached entry[6] (matrix slot) */
    s32 near_count;      /* 0x38 deferred near polygons */
    u32 end;             /* 0x3C packet buffer end */
    s32 near_otz;        /* 0x40 near threshold (caller: 70 / 20) */
    s32 pad44;
    u8 *normals;         /* 0x48 current mesh normals */
    u8 *near_overflow;   /* 0x4C caller: dword_801F5A90 */
    s32 counter;         /* 0x50 primitive counter */
    s32 sprite_sxy;      /* 0x54 SXY2 of the last sprite */
    s32 pad58[2];
    u32 mode_a[3];       /* 0x60 draw-mode packet (caller sub_8014215C) */
    u32 mode_b[3];       /* 0x6C mirrored draw-mode packet */
    s32 mirror;          /* 0x78 mesh flags & 2 */
    s32 fixed_otz;       /* 0x7C (mesh flags >> 3) & 0xFFF */
    s32 otz_override;    /* 0x80 entry light record word 0 */
    s32 *light_cache;    /* 0x84 cached light record */
    s32 cull_flip;       /* 0x88 NCLIP sign flip (caller) */
    s32 spill[5];        /* 0x8C */
    u8 *near_ptr;        /* 0xA0 next near-list entry */
    u8 near_list[4];     /* 0xA4 near list (12-byte entries) */
} Scratch;

typedef struct {
    s32 m0;
    u8 *verts;           /* +4: 8-byte vertices (xy word, z word) */
    u8 *normals;         /* +8 */
    u8 *prims;           /* +C */
    s32 nprims;          /* +10 */
    s32 m14;
    s16 m18;
    s16 flags;           /* +1A: bit1 mirror, bit2 near-deferrable, bits3..14 fixed otz */
} Mesh;

typedef struct {
    Mesh *mesh;          /* [0] */
    s32 x, y, z;         /* [1..3] world translation */
    s32 shift;           /* [4] SZ >> shift = world z */
    s32 *light;          /* [5] {otz override, light record*} */
    s32 rot;             /* [6] matrix slot (32-byte records) */
    s32 trans_id;        /* [7] */
} DrawEntry;

typedef struct {
    u32 *ot;             /* +0 ordering table (4096 entries) */
    s32 c4;
    u8 *cur;             /* +8 packet cursor */
    u8 *end;             /* +C packet end */
} DrawCtx;

#define SCR ((Scratch *)0x1F800000)
#define W32(p, o) (*(u32 *)((u8 *)(p) + (o)))
/* Divide with the results of retail's raw R3000 `div` and no trap: a zero
 * divisor gives LO = -1 (n >= 0) or +1 (n < 0), and 0x80000000 / -1 gives
 * 0x80000000. (A plain compiled `/` would trap on both, break 7 / break 6.) */
static __inline__ s32 HWDIV(s32 n, s32 d)
{
    if (d == 0)
        return n < 0 ? 1 : -1;
    if (d == -1)
        return (s32)(0u - (u32)n);
    return n / d;
}
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))

#define CTC5(p, r0, r1, r2, r3, r4) \
    __asm__ volatile ("ctc2 %0,$" #r0 "\n\tctc2 %1,$" #r1 "\n\tctc2 %2,$" #r2 \
                      "\n\tctc2 %3,$" #r3 "\n\tctc2 %4,$" #r4 \
                      : : "r"((p)[0]), "r"((p)[1]), "r"((p)[2]), "r"((p)[3]), "r"((p)[4]))

/* OT link: keep the OT entry's top byte, packet length in the packet's top byte. */
#define ADD_PRIM(slot, pkt, len) do { \
        u32 w_ = *(slot); s8 h_ = ((s8 *)(slot))[3]; \
        *(u32 *)(pkt) = w_; ((u8 *)(pkt))[3] = (len); \
        *(slot) = (u32)(pkt); ((s8 *)(slot))[3] = h_; } while (0)

#ifdef TM2_PORT_C_EMITTER
/* PC port request 61 addendum (port_c_emitter.h; PLAYTEST_ANSWERS.md 4.3 item 1, FAR_DISTANCE.md 2 "Precise
 * corners"): with portPreciseEmit set, the beam and point-sprite branches also report their four corners in 1/16
 * pixel to the port's precise ring. Drawing only; the packet's integer corners are stored as before. The helpers are
 * port code (.port_text). */
#include "semantic/port_c_emitter.h"
#define PE_TEXT __attribute__((section(".port_text")))

/* The entry's translation as the GTE holds it, in 1/256 GTE unit: TR = (R.hi) << 3 + (R.lo) >> 12 with the >> 12's
 * fraction kept ((R.lo) / 16, rounded); r = the rotation slot's nine elements; tr[3] = 0 when it does not fit 32 bits. */
static void PE_TEXT portPreciseTr(s16 *r, DrawEntry *e, s32 *tr)
{
    s32 hi[3], lo[3], v, i, j, m;

    for (j = 0; j < 3; j++) {
        v = j == 0 ? e->x : j == 1 ? e->y : e->z;
        if (v < 0) { v = -v; hi[j] = -(v >> 15); lo[j] = -(v & 0x7FFF); }
        else { hi[j] = v >> 15; lo[j] = v & 0x7FFF; }
    }
    tr[3] = 1;
    for (i = 0; i < 3; i++) {
        m = r[3 * i] * hi[0] + r[3 * i + 1] * hi[1] + r[3 * i + 2] * hi[2];
        if (m > 0x7FFFF || m < -0x7FFFF)
            tr[3] = 0;
        tr[i] = m * 2048 + ((r[3 * i] * lo[0] + r[3 * i + 1] * lo[1] + r[3 * i + 2] * lo[2] + 8) >> 4);
    }
}

/* a / b rounded to the nearest (b > 0) */
static s32 PE_TEXT portPreciseRound(s32 a, s32 b)
{
    return a >= 0 ? (a + b / 2) / b : -((b / 2 - a) / b);
}

/* q / c in 1/16, rounded (c > 0; c x 16 fits 32 bits) */
static s32 PE_TEXT portPreciseDiv16(s32 q, s32 c)
{
    return (q / c) * 16 + portPreciseRound((q % c) * 16, c);
}

/* Vertex (x, y, z) through rotation r and translation tr (1/256 GTE unit): out = the screen place in 1/16 pixel and
 * the depth in 1/16 GTE unit; 0 when it is behind the eye or within a unit of it, off the GTE's +-1024 pixels or
 * too far for 32 bits. (The camera coordinates are halved together until x H fits 32 bits: full precision near the
 * eye, where a unit is pixels, less far away, where it is a fraction of one.) */
static int PE_TEXT portPreciseProject(s16 *r, s32 *tr, s32 x, s32 y, s32 z, s32 h, s32 *out)
{
    s32 c[3], i, ofx, ofy;

    if (!tr[3] || h <= 0 || h >= 1024)
        return 0;
    for (i = 0; i < 3; i++)
        c[i] = tr[i] + ((r[3 * i] * x + r[3 * i + 1] * y + r[3 * i + 2] * z + 8) >> 4);
    if (c[2] < 256)
        return 0;
    out[2] = c[2] >> 4;
    while (c[0] > 0x1FFFFF || c[0] < -0x1FFFFF || c[1] > 0x1FFFFF || c[1] < -0x1FFFFF || c[2] > 0x7FFFFF) {
        c[0] >>= 1;
        c[1] >>= 1;
        c[2] >>= 1;
    }
    if (c[2] <= 0)
        return 0;
    __asm__ volatile ("cfc2 %0,$24\n\tcfc2 %1,$25\n\tnop" : "=r"(ofx), "=r"(ofy));
    out[0] = (ofx >> 12) + portPreciseDiv16(c[0] * h, c[2]);
    out[1] = (ofy >> 12) + portPreciseDiv16(c[1] * h, c[2]);
    if (out[0] <= -1024 * 16 || out[0] >= 1024 * 16 || out[1] <= -1024 * 16 || out[1] >= 1024 * 16)
        return 0;
    return 1;
}

/* A half size in 1/16 pixel: w x H / (z / 16 x 2^k), z in 1/16 GTE unit; at most `most` (1/16 pixel). */
static s32 PE_TEXT portPreciseSize(s32 w, s32 h, s32 z, s32 k, s32 most)
{
    s32 q;

    if (w <= 0)
        return 0;
    if (w > 0x7FFF)
        w = 0x7FFF;
    q = portPreciseDiv16(w * h * 16, z << k);       /* (w x H x 16 < 2^30) */
    q = q > most ? most : q;
    return q;
}

static void PE_TEXT portPreciseCorner(u8 *p, s32 x16, s32 y16, s32 z)
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

static u32 PE_TEXT portPreciseSqrt(u32 v)
{
    u32 r = 0, b = 1u << 30;

    while (b > v)
        b >>= 2;
    while (b != 0) {
        if (v >= r + b) {
            v -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
        b >>= 2;
    }
    return r;
}

/* k of an entry: the GTE range pass's scaled entries (TM2_PORT_FAR_DISTANCE) carry it in the key, else 0. */
static s32 PE_TEXT portPreciseK(DrawEntry *e)
{
#ifdef TM2_PORT_FAR_DISTANCE
    if (e->trans_id & 0x40000000)
        return ((u32)e->trans_id >> 28) & 3;
#endif
    return 0;
}

/* The beam (type 1) at vertex p of an entry with rotation r and translation tr: its corners at packet `xy`. */
static void PE_TEXT portPreciseBeam(Mesh *mesh, u8 *prim, DrawEntry *e, s16 *r, s32 *tr, u8 *p, u8 *xy,
    s32 stride, s32 h)
{
    s32 a[3], b[3], k, w, f, q, dx, dy, dyf, s, c, z;
    u32 l;
    s32 px = S16(p, 0), py = S16(p, 2), pz = S16(p, 4), len = S16(prim, 8);
    u8 *src;

    if (!portPreciseProject(r, tr, px, py, (s16)(pz + len), h, a) ||
        !portPreciseProject(r, tr, px, py, (s16)(pz - len), h, b))
        return;
    k = portPreciseK(e);
    src = prim;
#ifdef TM2_PORT_FAR_DISTANCE
    if (k)
        src = portFarSpriteSource((unsigned char *)mesh, prim);
#endif
    w = S16(src, 6);
    q = portPreciseSize(w, h, a[2], k, ((w * 32) >> k) < 16000 ? (w * 32) >> k : 16000);
    f = PORT_PRECISE_EMIT;
    dx = a[0] - b[0];
    dy = a[1] - b[1];
    while (dx > 16384 || dx < -16384 || dy > 16384 || dy < -16384) {
        dx >>= 1;
        dy >>= 1;
    }
    while ((dx | dy) != 0 && dx < 4096 && dx > -4096 && dy < 4096 && dy > -4096) {
        dx <<= 1;                                   /* (a short beam: the root's precision) */
        dy <<= 1;
    }
    dyf = (dy * f) >> 12;
    l = portPreciseSqrt((u32)(dx * dx + dyf * dyf));
    if (l == 0) {
        s = 0;
        c = q;
    } else {
        s = portPreciseRound(portPreciseRound(q * dyf, (s32)l) * f, 4096);
        c = portPreciseRound(q * dx, (s32)l);
    }
    z = (a[2] >> 4) << k;
    portPreciseCorner(xy, a[0] + s, a[1] - c, z);
    portPreciseCorner(xy + stride, a[0] - s, a[1] + c, z);
    portPreciseCorner(xy + stride * 2, b[0] + s, b[1] - c, z);
    portPreciseCorner(xy + stride * 3, b[0] - s, b[1] + c, z);
}

/* The point sprite at vertex p: its corners at packet `xy` (0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right). */
static void PE_TEXT portPreciseSprite(Mesh *mesh, u8 *prim, DrawEntry *e, s16 *r, s32 *tr, u8 *p, u8 *xy,
    s32 stride, s32 h)
{
    s32 o[3], k, hw, hh, z;
    u8 *src;

    if (!portPreciseProject(r, tr, S16(p, 0), S16(p, 2), S16(p, 4), h, o))
        return;
    k = portPreciseK(e);
    src = prim;
#ifdef TM2_PORT_FAR_DISTANCE
    if (k)
        src = portFarSpriteSource((unsigned char *)mesh, prim);
#endif
    hw = portPreciseSize(S16(src, 6), h, o[2], k, 500 * 16);
    hh = portPreciseSize(S16(src, 8), h, o[2], k, 500 * 16);
    hw = portPreciseRound(hw * PORT_PRECISE_EMIT, 4096);
    z = (o[2] >> 4) << k;
    portPreciseCorner(xy, o[0] - hw, o[1] - hh, z);
    portPreciseCorner(xy + stride, o[0] + hw, o[1] - hh, z);
    portPreciseCorner(xy + stride * 2, o[0] - hw, o[1] + hh, z);
    portPreciseCorner(xy + stride * 3, o[0] + hw, o[1] + hh, z);
}

/* Request 116 (portVisual bit 11, port_visual.h TM2_VIS_PRECISE_CULL): portVisual read as an array, as portPreciseEmit
 * is (this unit has extern_gp, and .port_bss is not $gp-reachable); port_variant_arrays.c defines the word. */
extern int portVisual[];
#define PORT_PRECISE_CULL (portVisual[0] & PORT_PRECISE_CULL_BIT)

/* Halve v[0..2] together until each is within +-most (a positive scale: the signs below are unchanged); most is
 * 2^k - 1. Request 128: the halvings are counted first on u = the three magnitudes ORed (|v| for v >= 0, |v| - 1 for
 * v < 0), in five steps: (u >> n) <= most exactly when every element's magnitude >> n is, so n is never more than the
 * halvings needed, and one shift by n is the same as n shifts by one; the loop then does the one halving more that a
 * negative element needs when its v >> n is exactly -(most + 1). The same results as halving one at a time, in a fraction of
 * the steps (camera coordinates in 1/256 unit took up to 16 rounds of six tests). */
static void PE_TEXT portPreciseFit(s32 *v, s32 most)
{
    u32 u = (u32)(v[0] ^ (v[0] >> 31)) | (u32)(v[1] ^ (v[1] >> 31)) | (u32)(v[2] ^ (v[2] >> 31));
    s32 n = 0;

    if (u > (u32)most) {
        if ((u >> 16) > (u32)most) n = 16;
        if ((u >> (n + 8)) > (u32)most) n += 8;
        if ((u >> (n + 4)) > (u32)most) n += 4;
        if ((u >> (n + 2)) > (u32)most) n += 2;
        if ((u >> (n + 1)) > (u32)most) n += 1;
        n += 1;
        v[0] >>= n;
        v[1] >>= n;
        v[2] >>= n;
    }
    while (v[0] > most || v[0] < -most || v[1] > most || v[1] < -most || v[2] > most || v[2] < -most) {
        v[0] >>= 1;
        v[1] >>= 1;
        v[2] >>= 1;
    }
}

/* The edge from vertex p to vertex q through rotation r: out = R (q - p), each fitted within +-0x3FFF. */
static void PE_TEXT portPreciseEdge(s16 *r, u8 *p, u8 *q, s32 *out)
{
    s32 d[3], i;

    d[0] = S16(q, 0) - S16(p, 0);
    d[1] = S16(q, 2) - S16(p, 2);
    d[2] = S16(q, 4) - S16(p, 4);
    portPreciseFit(d, 0x3FFF);
    for (i = 0; i < 3; i++)
        out[i] = r[3 * i] * d[0] + r[3 * i + 1] * d[1] + r[3 * i + 2] * d[2];
    portPreciseFit(out, 0x3FFF);
}

/* Request 116: does the triangle of vertices p0, p1, p2 face the eye, on the exact projection? NCLIP's sign on the
 * GTE's corners, (x1 - x0)(y2 - y0) - (x2 - x0)(y1 - y0) with x = H X / Z and y = H Y / Z, is H^2 / (Z0 Z1 Z2) times
 * det(c0, c1, c2) = c0 . ((c1 - c0) x (c2 - c0)) of the camera-space corners c: so with the three in front of the eye
 * its sign is that determinant's, with no division and nothing rounded to pixels (the GTE's whole-pixel corners can
 * turn a polygon less than a pixel thin, a road strip seen from far along it, over: its NCLIP the wrong sign). c0 =
 * the entry's translation tr (1/256 unit, portPreciseTr) + R p0, the edges R (p1 - p0) and R (p2 - p0) exact; each
 * vector is halved to fit 32 bits (the sign kept). Returns 1 when it faces the eye in the sense the emitter keeps
 * (cull flip 0: a determinant >= 0, as NCLIP >= 0), 0 when not or when the test cannot be made (tr not valid, a corner
 * within a unit of the eye or behind it): then the GTE's choice stands.
 *
 * Request 128 (the rear-view mirror): the GTE's translation is R t, the slot's rotation times the key's translation
 * t, so c = R (p + t) and det(c0, c1, c2) = det(R) det(p0 + t, p1 + t, p2 + t) = det(R) (p0 + t) . ((p1 - p0) x (p2 -
 * p0)): the same sign from the mesh's own integer coordinates, with no rotation per polygon (about a sixth of the
 * steps of the camera-space test, with the scalar code below).
 * key = the key's t, its slot and the sign of that slot's det(R) (portPreciseKey): the mirror's view matrix has its x
 * axis negated, so det(R) < 0 there and the sign follows NCLIP's as the cull flip expects. When the entry's slot is
 * not the key's (a key shared across slots: the GTE keeps the first slot's R t) or det(R) is about 0 (a flattened
 * matrix), the camera-space test above. */
static s32 PE_TEXT portPreciseDetSign(s16 *r)
{
    s32 x = (r[4] * r[8] - r[5] * r[7]) >> 12;
    s32 y = (r[5] * r[6] - r[3] * r[8]) >> 12;
    s32 w = (r[3] * r[7] - r[4] * r[6]) >> 12;
    s32 d = r[0] * x + r[1] * y + r[2] * w;

    return d > 256 ? 1 : d < -256 ? -1 : 0;
}

/* At a key change: its translation, its slot and the slot's det(R) sign, for portPreciseFacing. */
static void PE_TEXT portPreciseKey(s16 *r, DrawEntry *e, s32 *key)
{
    key[0] = e->x;
    key[1] = e->y;
    key[2] = e->z;
    key[3] = e->rot;
    key[4] = portPreciseDetSign(r);
}

/* The halvings that bring u (magnitudes ORed, v ^ (v >> 31): |v|, or |v| - 1 for v < 0) within most = 2^k - 1, for
 * u > most: then every v >> n is within -(most + 1) .. most. (Scalars: the unit is built with -fvolatile, where every
 * array element read or written is a memory access.) */
#define PMAG(v) ((u32)((v) ^ ((v) >> 31)))
static s32 PE_TEXT portPreciseShift(u32 u, u32 most)
{
    s32 n = 0;

    if ((u >> 16) > most) n = 16;
    if ((u >> (n + 8)) > most) n += 8;
    if ((u >> (n + 4)) > most) n += 4;
    if ((u >> (n + 2)) > most) n += 2;
    if ((u >> (n + 1)) > most) n += 1;
    return n + 1;
}

static int PE_TEXT portPreciseFacing(s16 *r, s32 *tr, s32 *key, s32 slot, u8 *p0, u8 *p1, u8 *p2, s32 flip)
{
    s32 c[3], a[3], b[3], n[3], i, z;
    s32 x0, y0, z0, x1, y1, z1, x2, y2, z2, r6, r7, r8, t2, sg, k;
    s32 ax, ay, az, bx, by, bz, nx, ny, nw, cx, cy, cz;
    u32 u;

    if (!tr[3])
        return 0;
    x0 = S16(p0, 0); y0 = S16(p0, 2); z0 = S16(p0, 4);
    x1 = S16(p1, 0); y1 = S16(p1, 2); z1 = S16(p1, 4);
    x2 = S16(p2, 0); y2 = S16(p2, 2); z2 = S16(p2, 4);
    r6 = r[6]; r7 = r[7]; r8 = r[8];
    t2 = tr[2];
    if (t2 + ((r6 * x0 + r7 * y0 + r8 * z0 + 8) >> 4) < 256 ||
        t2 + ((r6 * x1 + r7 * y1 + r8 * z1 + 8) >> 4) < 256 ||
        t2 + ((r6 * x2 + r7 * y2 + r8 * z2 + 8) >> 4) < 256)
        return 0;
    sg = key[4];
    if (slot == key[3] && sg != 0) {
        ax = x1 - x0; ay = y1 - y0; az = z1 - z0;
        bx = x2 - x0; by = y2 - y0; bz = z2 - z0;
        u = PMAG(ax) | PMAG(ay) | PMAG(az) | PMAG(bx) | PMAG(by) | PMAG(bz);
        if (u > 0x3FFF) {
            k = portPreciseShift(u, 0x3FFF);
            ax >>= k; ay >>= k; az >>= k;
            bx >>= k; by >>= k; bz >>= k;
        }
        nx = ay * bz - az * by;     /* (each product within 2^28) */
        ny = az * bx - ax * bz;
        nw = ax * by - ay * bx;
        u = PMAG(nx) | PMAG(ny) | PMAG(nw);
        if (u > 0x3FFF) {
            k = portPreciseShift(u, 0x3FFF);
            nx >>= k; ny >>= k; nw >>= k;
        }
        cx = x0 + key[0];
        cy = y0 + key[1];
        cz = z0 + key[2];
        u = PMAG(cx) | PMAG(cy) | PMAG(cz);
        if (u > 0x7FFF) {
            k = portPreciseShift(u, 0x7FFF);
            cx >>= k; cy >>= k; cz >>= k;
        }
        z = cx * nx + cy * ny + cz * nw;  /* (within 3 x 2^14 x 2^15) */
        if (sg < 0)
            z = -z;
        return (z >= 0) == (flip == 0);
    }
    for (i = 0; i < 3; i++)
        c[i] = tr[i] + ((r[3 * i] * S16(p0, 0) + r[3 * i + 1] * S16(p0, 2) + r[3 * i + 2] * S16(p0, 4) + 8) >> 4);
    portPreciseEdge(r, p0, p1, a);
    portPreciseEdge(r, p0, p2, b);
    n[0] = a[1] * b[2] - a[2] * b[1];
    n[1] = a[2] * b[0] - a[0] * b[2];
    n[2] = a[0] * b[1] - a[1] * b[0];
    portPreciseFit(n, 0x1FFF);
    portPreciseFit(c, 0x7FFF);
    z = c[0] * n[0] + c[1] * n[1] + c[2] * n[2];
    return (z >= 0) == (flip == 0);
}
#endif /* TM2_PORT_C_EMITTER */

/* @800CDC54: model primitive emitter. For each 32-byte draw entry: load the
 * rotation/light matrices of its slot, rotate its translation (split >>15 /
 * &0x7FFF for precision), load back colour + colour matrix, then per mesh
 * primitive: project (3 tri, 4 quad, 1 camera-facing beam, other = point
 * sprite), backface-cull with NCLIP, map max SZ to a compressed OTZ, light
 * the vertex colours (NCCS) or copy them, copy UVs, and link the packet into
 * the OT (with draw-mode packets around it when mirrored / flagged), or defer
 * it to the near list (renderNearPolys). Stops when the packet buffer is full. */
void __attribute__((section(".text_800CDC54"))) sub_800CDC54(DrawCtx *ctx, DrawEntry *e,
    s32 count, u8 *rotmats, u8 *lightmats)
{
    Scratch *S;
    u8 *cur;
    u8 *prim;
    u8 *verts;
    s32 nprims;
    s32 tp = 0;     /* retail t8: last value is stored in near-list entries */
#ifdef TM2_PORT_C_EMITTER
    s32 ptr[4];     /* the port's precise corners: the GTE's translation in 1/256 unit, valid (portPreciseTr) */
    u8 *pxy = 0;    /* the beam's or sprite's packet corners, its vertex */
    u8 *pv = 0;
    s32 nz[4];      /* request 82: the primitive's corner depths (SZ), for the near pass's split vertices */
    s32 pkey[5];    /* request 128: the key's translation, slot and det(R) sign (portPreciseKey) */
#endif

    S = SCR;
#ifdef TM2_PORT_C_EMITTER
    ptr[0] = ptr[1] = ptr[2] = ptr[3] = 0;
    pkey[0] = pkey[1] = pkey[2] = pkey[4] = 0;
    pkey[3] = -1;
#endif
    S->trans_id = -1;
    S->near_count = 0;
    S->rot_id = -1;
    S->end = (u32)ctx->end;
    cur = ctx->cur;
    S->light_cache = 0;
    S->near_ptr = S->near_list;

    while (count-- > 0) {
        Mesh *mesh;
        s32 id;
        s32 *lr;
        s32 fl;

        id = e->rot;
        mesh = e->mesh;
        if (id != S->rot_id) {
            u32 *m = (u32 *)(rotmats + id * 32);
            CTC5(m, 0, 1, 2, 3, 4);
            S->rot_id = id;
            m = (u32 *)(lightmats + id * 32);
            CTC5(m, 8, 9, 10, 11, 12);
            tp = (s32)m[3];         /* retail t8: the light matrix's word 3 (800CDD0C; check 8) */
        }
        id = e->trans_id;
        fl = S->trans_id;
        S->trans_id = id;
        if (id != fl) {
            s32 v, hx, hy, hz, lx, ly, lz, mx, my, mz, ax, ay, az;
            v = e->x;
            if (v < 0) { v = -v; hx = -(v >> 15); lx = -(v & 0x7FFF); }
            else { hx = v >> 15; lx = v & 0x7FFF; }
            v = e->y;
            if (v < 0) { v = -v; hy = -(v >> 15); ly = -(v & 0x7FFF); }
            else { hy = v >> 15; ly = v & 0x7FFF; }
            v = e->z;
            if (v < 0) { v = -v; hz = -(v >> 15); lz = -(v & 0x7FFF); }
            else { hz = v >> 15; lz = v & 0x7FFF; }
            /* MVMVA rt*ir sf=0 on the high parts, then sf=1 on the low parts */
            __asm__ volatile ("mtc2 %3,$9\n\tmtc2 %4,$10\n\tmtc2 %5,$11\n\tnop\n\t"
                              "cop2 0x41E012\n\tmfc2 %0,$25\n\tmfc2 %1,$26\n\tmfc2 %2,$27\n\tnop"
                              : "=&r"(mx), "=&r"(my), "=&r"(mz)
                              : "r"(hx), "r"(hy), "r"(hz));
            __asm__ volatile ("mtc2 %0,$9\n\tmtc2 %1,$10\n\tmtc2 %2,$11\n\tnop\n\t"
                              "cop2 0x49E012" : : "r"(lx), "r"(ly), "r"(lz));
            mx <<= 3;
            my <<= 3;
            mz <<= 3;
            tp = mx;
            __asm__ volatile ("mfc2 %0,$25\n\tmfc2 %1,$26\n\tmfc2 %2,$27\n\tnop"
                              : "=r"(ax), "=r"(ay), "=r"(az));
            ax += mx;
            ay += my;
            az += mz;
            __asm__ volatile ("ctc2 %0,$5\n\tctc2 %1,$6\n\tctc2 %2,$7"
                              : : "r"(ax), "r"(ay), "r"(az));
#ifdef TM2_PORT_C_EMITTER
            if (PORT_PRECISE_EMIT || PORT_PRECISE_CULL)
                portPreciseTr((s16 *)(rotmats + e->rot * 32), e, ptr);
            if (PORT_PRECISE_CULL)
                portPreciseKey((s16 *)(rotmats + e->rot * 32), e, pkey);
#endif
        }
        lr = (s32 *)e->light[1];
        if (lr != S->light_cache) {
            __asm__ volatile ("ctc2 %0,$13\n\tctc2 %1,$14\n\tctc2 %2,$15\n\tnop"
                              : : "r"(lr[0] << 4), "r"(lr[1] << 4), "r"(lr[2] << 4));
            CTC5(lr + 3, 16, 17, 18, 19, 20);
            S->light_cache = lr;
        }
        fl = mesh->flags;
        S->mirror = fl & 2;
        S->otz_override = e->light[0];
        S->fixed_otz = ((u32)fl >> 3) & 0xFFF;
        verts = mesh->verts;
        S->normals = mesh->normals;
        prim = mesh->prims;
        nprims = mesh->nprims;
        S->counter = 0;

        for (;;) {
            u32 ctl;
            s32 type, stride, z, otz, n, mode, lo, h, d, xy0, xy1;
            u8 *xy, *src, *dst;

            n = S->counter;
            if (nprims == n)
                break;
            S->counter = n + 1;
            if (S->end < (u32)cur)
                goto done;
            xy = cur + 8;
            ctl = W32(prim, 12);
            type = (ctl >> 16) & 7;
            stride = ((ctl >> 22) & 3) * 4;

            if (type == 3 || type == 4) {
                u32 a = W32(prim, 4), b = W32(prim, 8);
                s32 s0, s1, s2, s3;
                u8 *p0 = verts + (s16)a * 8;
                u8 *p1 = verts + ((s32)a >> 16) * 8;
                u8 *p2 = verts + (s16)b * 8;
                __asm__ volatile ("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tlwc2 $2,0(%1)\n\t"
                                  "lwc2 $3,4(%1)\n\tlwc2 $4,0(%2)\n\tlwc2 $5,4(%2)\n\t"
                                  "cop2 0x280030" : : "r"(p0), "r"(p1), "r"(p2));
                if ((s32)ctl >= 0) {
                    s32 mac0;
                    __asm__ volatile ("cop2 0x1400006\n\tmfc2 %0,$24\n\tnop" : "=r"(mac0));
                    if ((mac0 >= 0) != (S->cull_flip == 0)) {
#ifdef TM2_PORT_C_EMITTER
                        /* request 116 (portVisual bit 11): kept when it faces the eye on the exact projection */
                        if (!PORT_PRECISE_CULL ||
                            !portPreciseFacing((s16 *)(rotmats + e->rot * 32), ptr, pkey, e->rot, p0, p1, p2,
                                               S->cull_flip))
#endif
                        goto next;
                    }
                }
                __asm__ volatile ("swc2 $12,0(%0)\n\tswc2 $13,0(%1)\n\tswc2 $14,0(%2)"
                                  : : "r"(xy), "r"(xy + stride), "r"(xy + stride * 2) : "memory");
                if (type == 3) {
                    __asm__ volatile ("mfc2 %0,$17\n\tmfc2 %1,$18\n\tmfc2 %2,$19\n\tnop"
                                      : "=r"(s1), "=r"(s2), "=r"(s3));
                    z = s1 < s2 ? s2 : s1;
#ifdef TM2_PORT_C_EMITTER
                    nz[0] = s1;     /* request 82: corners 0..2 */
                    nz[1] = s2;
                    nz[2] = s3;
                    nz[3] = s3;
#endif
                } else {
                    u8 *p3 = verts + ((s32)b >> 16) * 8;
                    __asm__ volatile ("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tnop\n\t"
                                      "cop2 0x180001\n\tswc2 $14,0(%1)"
                                      : : "r"(p3), "r"(xy + stride * 3) : "memory");
                    __asm__ volatile ("mfc2 %0,$16\n\tmfc2 %1,$17\n\tmfc2 %2,$18\n\tmfc2 %3,$19\n\tnop"
                                      : "=r"(s0), "=r"(s1), "=r"(s2), "=r"(s3));
                    z = s0 < s1 ? s1 : s0;
                    tp = z < s2 ? s2 : z;
                    z = tp;
#ifdef TM2_PORT_C_EMITTER
                    nz[0] = s0;     /* request 82: corners 0..3 (the FIFO after the fourth RTPS) */
                    nz[1] = s1;
                    nz[2] = s2;
                    nz[3] = s3;
#endif
                }
                z = z < s3 ? s3 : z;
                lo = 0;
            } else {
                u8 *p = verts + S16(prim, 4) * 8;
                __asm__ volatile ("cfc2 %0,$26\n\tnop" : "=r"(h));
                lo = 1;
                if (type == 1) {
                    /* camera-facing beam: both ends at the vertex, z +- len */
                    s32 len = S16(prim, 8);
                    s32 pxy = W32(p, 0), pz = W32(p, 4);
                    s32 s2, s3;
                    __asm__ volatile ("mtc2 %0,$0\n\tmtc2 %1,$1\n\tmtc2 %0,$2\n\tmtc2 %2,$3\n\t"
                                      "mtc2 %0,$4\n\tmtc2 %2,$5\n\tnop\n\tcop2 0x280030\n\tnop"
                                      : : "r"(pxy), "r"(pz + len), "r"(pz - len));
                    __asm__ volatile ("mfc2 %0,$12\n\tmfc2 %1,$13\n\tmfc2 %2,$17\n\t"
                                      "mfc2 %3,$18\n\tmfc2 %4,$19\n\tnop"
                                      : "=r"(xy0), "=r"(xy1), "=r"(d), "=r"(s2), "=r"(s3));
                    tp = xy1;               /* retail t8: SXY1 (800CE264), kept when the beam is dropped */
                    z = d < s2 ? s2 : d;
                    z = z < s2 ? s2 : z;    /* retail compares SZ2 twice (SZ3 unused) */
                } else {
                    /* point sprite at the vertex */
                    __asm__ volatile ("lwc2 $0,0(%1)\n\tlwc2 $1,4(%1)\n\tnop\n\t"
                                      "cop2 0x180001\n\tmfc2 %0,$19\n\tswc2 $14,0(%2)\n\tnop"
                                      : "=r"(d) : "r"(p), "r"(&S->sprite_sxy) : "memory");
                    if (d == 0)
                        goto next;
                    z = d;
                }
            }
            /* OTZ: world z (SZ >> shift), log-compressed beyond 1024 */
            otz = z >> e->shift;
            if (otz >= 1024) {
                if (otz < 4096) otz = ((otz - 1024) >> 2) + 1024;
                else if (otz < 16384) otz = ((otz - 4096) >> 3) + 1792;
                else otz = ((otz - 16384) >> 5) + 3328;
            } else if (otz > 0) {
                otz += (s32)(ctl << 19) >> 26;
                if (otz <= 0) otz = 1;
            }
            /* drop: otz-1 outside [0,4095) (tri/quad) or (0,4095) (beam/sprite) */
            if (otz - 1 < lo || otz - 1 >= 4095)
                goto next;
#ifdef TM2_PORT_C_EMITTER
            pxy = xy;
            pv = verts + S16(prim, 4) * 8;
#endif
            if (type == 1) {
                s32 x0, y0, x1, y1, q, w, c, s;
                y0 = xy0 >> 16;
                y1 = xy1 >> 16;
                x0 = (s16)xy0;
                x1 = (s16)xy1;
                tp = ratan2(y0 - y1, x0 - x1);
                w = S16(prim, 6);
                q = w * h;
                s = rsin(tp);
                q = HWDIV(q, d);    /* half width = w * H / SZ1, clamped 1..2w */
                if (q <= 0) q = 1;
                if (q >= w * 2) q = w * 2;
                c = rcos(tp);
                s = (q * s) / 4096;
                c = (q * c) / 4096;
                S16(xy, 0) = x0 + s;
                S16(xy, 2) = y0 - c;
                xy += stride;
                S16(xy, 0) = x0 - s;
                S16(xy, 2) = y0 + c;
                xy += stride;
                S16(xy, 0) = x1 + s;
                S16(xy, 2) = y1 - c;
                xy += stride;
                S16(xy, 0) = x1 - s;
                S16(xy, 2) = y1 + c;
#ifdef TM2_PORT_C_EMITTER
                if (PORT_PRECISE_EMIT)
                    portPreciseBeam(mesh, prim, e, (s16 *)(rotmats + e->rot * 32), ptr, pv, pxy, stride, h);
#endif
            } else if (lo) {
                /* sprite half size = (+6, +8) * H / SZ, clamped 1..500 */
                s32 hw, hh, x, y;
                hw = HWDIV(S16(prim, 6) * h, d);
                hh = HWDIV(S16(prim, 8) * h, d);
                if (hw <= 0) hw = 1;
                if (hw > 500) hw = 500;
                if (hh <= 0) hh = 1;
                if (hh > 500) hh = 500;
                x = S16(&S->sprite_sxy, 0) - hw;
                y = S16(&S->sprite_sxy, 2) - hh;
                hw = x + hw * 2;
                hh = y + hh * 2;
                S16(xy, 0) = x;
                S16(xy, 2) = y;
                xy += stride;
                S16(xy, 0) = hw;
                S16(xy, 2) = y;
                xy += stride;
                S16(xy, 0) = x;
                S16(xy, 2) = hh;
                xy += stride;
                S16(xy, 0) = hw;
                S16(xy, 2) = hh;
#ifdef TM2_PORT_C_EMITTER
                if (PORT_PRECISE_EMIT)
                    portPreciseSprite(mesh, prim, e, (s16 *)(rotmats + e->rot * 32), ptr, pv, pxy, stride, h);
#endif
            }
            /* vertex colours: copy, or NCCS with the mesh normals */
            n = prim[1];
            mode = (ctl >> 19) & 7;
            src = prim + 16;
            dst = cur + 4;
            if (mode == 0) {
                for (;;) {
                    W32(dst, 0) = W32(src, 0);
                    dst += stride;
                    if (--n == 0) break;
                    src += 4;
                }
            } else {
                u8 *nrm = src + ((ctl >> 24) & 0xF) * 4;
                s32 nstep = mode == 1 ? 0 : 2;
                s32 cstep = n == 1 ? 0 : 4;
                if (n < mode) n = mode;
                for (;;) {
                    u8 *v = S->normals + S16(nrm, 0) * 8;
                    tp = (s32)v;
                    __asm__ volatile ("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tlwc2 $6,0(%1)\n\t"
                                      "cop2 0x108041B\n\tswc2 $22,0(%2)"
                                      : : "r"(v), "r"(src), "r"(dst) : "memory");
                    if (--n == 0) break;
                    nrm += nstep;
                    src += cstep;
                    dst += stride;
                }
            }
            src += 4;
            /* texture words */
            n = S16(prim, 0) & 0x7F;
            dst = cur + 12;
            if (n != 0) {
                tp = S16(src, 6);
                do {
                    W32(dst, 0) = W32(src, 0);
                    dst += stride;
                    src += 4;
                } while (--n != 0);
            }
            n = S->near_count;
            if (otz < S->near_otz && (mesh->flags & 4) && n < 200) {
                /* defer to the near pass: {prim, packet, t8, sort z} */
                u8 *ent;
                s32 sz;
                s32 sp = n < 71;
                ent = sp ? S->near_ptr : S->near_overflow;
#ifdef TM2_PORT_C_EMITTER
                if (PORT_PRECISE_EMIT && n < PORT_NEAR_MAX) {
                    /* request 82: the corners' depths for the near pass (a beam's or sprite's: its sort depth) */
                    s32 i_, k_ = portPreciseK(e);
                    for (i_ = 0; i_ < 4; i_++)
                        portNearDepth[n * 4 + i_] = (type == 3 || type == 4 ? nz[i_] : z) << k_;
                }
#endif
                S16(ent, 8) = tp;
                W32(ent, 0) = (u32)prim;
                W32(ent, 4) = (u32)cur;
                sz = otz;
                if (S->fixed_otz) sz = S->fixed_otz;
                if (S->otz_override) sz = S->otz_override;
                if (S->mirror) sz = -sz;
                S16(ent, 10) = sz;
                ent += 12;
                S->near_ptr = ent;
                if (!sp) S->near_overflow = ent;
                S->near_count = n + 1;
                cur += prim[3] * 4 + 4;
            } else {
                u32 *slot;
                s32 flag, mir;
                if (S->otz_override) otz = S->otz_override;
                else if (S->fixed_otz) otz = S->fixed_otz;
                flag = W32(prim, 0) & 0x80;
                mir = S->mirror;
                slot = ctx->ot + otz;
                if (mir || flag) {
                    u8 *pkt = cur;
                    u32 *m;
                    cur += prim[3] * 4 + 4;
                    W32(cur, 0) = S->mode_a[0];
                    W32(cur, 4) = S->mode_a[1];
                    W32(cur, 8) = S->mode_a[2];
                    ADD_PRIM(slot, cur, 2);
                    cur += 12;
                    ADD_PRIM(slot, pkt, prim[3]);
                    m = mir ? S->mode_b : S->mode_a;
                    W32(cur, 0) = m[0];
                    W32(cur, 4) = m[1];
                    W32(cur, 8) = m[2];
                    if (flag) {
                        cur[4] = tp;
                        W32(cur, 8) = W32(src, 0);
                    }
                    ADD_PRIM(slot, cur, 2);
                    cur += 12;
                } else {
                    ADD_PRIM(slot, cur, prim[3]);
                    cur += prim[3] * 4 + 4;
                }
            }
next:
            prim += prim[2] * 4;
        }
        e++;
    }
done:
    ctx->cur = cur;
}
