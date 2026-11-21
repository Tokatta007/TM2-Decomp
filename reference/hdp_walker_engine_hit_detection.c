/* Pure-C version of src/semantic/engine_hit_detection.c whose
 * SingleTrac hand-written asm hdp walker (hdpWalk 8014EA60 with the hdpKind*
 * handlers, 8014EF3C..8014FAB0 up to hdpPassRun) is rewritten as C with GTE
 * macros; the other functions are the same C as src/ (byte-identical). Not yet
 * run.
 * Linked only by the all-C relocatable flavour (tools/build_source_only.py
 * --relocatable --all-c, docs/SOURCE_BUILD.md "All-C flavour"), compiled with
 * the BUILD_MANIFEST.json recipe of the src/ unit it replaces; the hybrid and the
 * fixed source-only builds keep the asm (docs/RULES.md). Static proof only.
 * ABI: the only entry from outside the walker is hdpPassRun(a0..a3, 5th on the
 * stack), called by the compiled hdpCollideObject with the standard convention, as
 * the asm expects. The asm saved s0-s7, sp, gp, a0, a1, ra, fp at
 * 0x1F80006C..0x1F8000A0 (and a0 at 0x08) and used gp/sp/s8 as data registers; the
 * C keeps them on the stack. The walker internals (hdpWalkPush, hdpRotVecSplit,
 * hdpLeafTest*, ...) had private register conventions; here they are normal C
 * functions taking an HdpWalker. hdpKindHandlers (BSS) holds the C handlers'
 * addresses, not the retail labels. */

/* Library code 8014E000..80150000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything in this range is SingleTrac engine code (TIM upload, shell
 * texture packs, the hit-detection pass "hdp*"); no public source exists, all
 * of it was written against the retail code. Recipe: like the game
 * (psyq3.6, -fvolatile, extern gp).
 *
 * 8014EA60..8014FD3C is hand-written assembly in retail (the hdp collision
 * walker: no stack frame, state in the scratchpad at 0x1F800000, gp/sp/s8
 * used as data registers, internal calls with private register conventions).
 * Here it is C that does the same work with the normal calling convention;
 * it does not byte-match. The table filled by hdpInitKindTable holds the kind
 * handlers hdpKindN, which live in hdpWalk's slot as in retail. */
/* hdpTraverse switch jump table (retail address). */
#pragma tm2_rdata .tm2_800D1070

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    s16 x, y, w, h;
} RECT;

typedef struct {
    u32 pmode;
    s16 px, py;
    u16 pw, ph;
    u32 *pixel;
    s16 cx, cy;
    u16 cw, ch;
    u32 *clut;
} GsIMAGE;

typedef struct {
    s32 vx, vy, vz, pad;
} VECTOR;

extern int printf(const char *fmt, ...);
extern void exit(int code);
extern void GsGetTimInfo(u32 *tim, GsIMAGE *info);
extern int LoadImage(RECT *rect, u32 *p);

/* hdp state record (the scratchpad block at 0x1F800000). */
typedef struct {
    s32 f00;            /* 0x00 node stack base (hdpNodeStackBase) */
    void **f04;         /* 0x04 node-kind dispatch table (hdpKindHandlers) */
    s32 pad08[5];
    s32 f1c;            /* 0x1C hdpDifficulty */
    void *f20;          /* 0x20 D_801A4728 */
    s32 pad24[35];
    u32 fb0;            /* 0xB0 number of hit models in fb4 */
    u8 **fb4;           /* 0xB4 hit models (hdpHitModels, 10 entries) */
    s32 padb8[3];
    s32 fc4;            /* 0xC4 hdpSingleBoxTest */
} HdpState;

/* hdp traversal stack entry (24 bytes). */
typedef struct {
    u8 *node;
    s32 v[3];
    s16 a, b;
    s32 c;
} HdpEntry;

/* the traversed object: hit hierarchy root at 0x11C (a struct access, so the
 * load does not alias the scalar global stores around the first push) */
typedef struct {
    u8 pad[0x11C];
    u8 *f11C;
} HdpObj;

/* hit plane: normal (x, y, z), distance, response values */
typedef struct {
    s16 x, y, z, pad;
    s32 d;
    u8 b0c, b0d;
    u16 h0e;
} HdpPlane;

/* per-point hit record (32 bytes) */
typedef struct {
    s32 hit;
    s32 nx, ny, nz;
    s32 depth;
    s32 f14;
    s16 f18, f1a;
    s16 f1c, f1e;
} HdpPoint;

/* the object's hit points, after one leading word */
typedef struct {
    s32 f00;
    HdpPoint pts[1];
} HdpPoints;

/* hdp walker entry (24 bytes; retail keeps the current one in t0..t5) */
typedef struct {
    u8 *node;           /* node */
    s32 x, y, z;        /* probe position in the node's space */
    u32 lod;            /* LOD level << 16 | number of parent rotations */
    u32 ids;            /* id << 16 | id2 */
} HdpWalk;

/* hdp walker state (retail: t0..t5, t6/t7 stack pointers, t8 depth) */
typedef struct {
    HdpWalk cur;
    HdpWalk *sp;        /* next free scratchpad stack slot */
    HdpWalk *xp;        /* next free slot of the overflow stack */
    s32 depth;
} HdpWalker;

/* the hdp scratchpad block at 0x1F800000 (the walker's view of HdpState) */
typedef struct {
    HdpWalk *xstack;    /* 0x00 overflow stack (hdpNodeStackBase) */
    void **table;       /* 0x04 node-kind handlers (hdpKindHandlers) */
    u8 *obj;            /* 0x08 object being tested */
    s32 r2;             /* 0x0C squared radius of its hit sphere */
    s32 npts;           /* 0x10 number of hit points */
    s16 *lpts;          /* 0x14 hit points in object space (8 bytes each) */
    HdpPoint *pts;      /* 0x18 hit records */
    s32 shift;          /* 0x1C base LOD level (hdpDifficulty) */
    u8 **xf;            /* 0x20 parent rotations of the current path (D_801A4728) */
    s32 pad24[3];       /* 0x24 */
    s32 w[6];           /* 0x30 point position (world, node space) / previous-position plane values */
    s32 d[6];           /* 0x48 plane values of the current position */
    s16 *nrm;           /* 0x60 normal of the hit plane */
    s32 *dep;           /* 0x64 value of the hit plane */
    s32 pad68[15];      /* 0x68 */
    void *prevm;        /* 0xA4 previous rotation of the object */
    s32 *prevp;         /* 0xA8 previous position of the object */
    u8 *hitobj;         /* 0xAC object whose hierarchy is walked */
    s32 nhit;           /* 0xB0 hit model count */
    u8 **hits;          /* 0xB4 hit models (hdpHitModels, 10 entries) */
    s32 single;         /* 0xB8 single-box hit (fc4 mode) */
    s32 left;           /* 0xBC walk while > 0 */
    s32 margin;         /* 0xC0 box margin / hit sphere radius */
    s32 fc4;            /* 0xC4 single-box mode (hdpSingleBoxTest) */
    s32 padC8[2];       /* 0xC8 */
    HdpWalk stack[33];  /* 0xD0 walker stack (deeper entries go to xstack) */
} HdpPad;

extern s32 hdpDifficulty;
extern s32 hdpSingleBoxTest;
extern HdpState *hdpState;
extern HdpEntry *hdpStackScratch;
extern s32 hdpShift;
extern s32 hdpNodeStackBase;
extern HdpEntry *hdpEntryStack;
extern s32 hdpStackDepth;

extern u8 hdpPlaneEnabled;
#pragma tm2_absolute hdpPlaneEnabled
extern HdpPlane hdpPlane;
extern u8 *hdpHitModels[];
extern void *hdpKindHandlers[];
extern s32 D_801A4728[];

extern char strTexNotTmsFile[];
extern char strTexWrongVersion[];
extern char strTexVersionFmt[];

/* node-kind handlers in hdpWalk's slot (retail: kinds 0, 6, 7, 10 and 13
 * share one label; here hdpKind6/7/10/13 are separate copies of hdpKind0 so
 * that hdpInitKindTable is the byte-identical C of src/) */
s32 hdpKind0(HdpWalker *w, u8 ***list);
s32 hdpKind6(HdpWalker *w, u8 ***list);
s32 hdpKind7(HdpWalker *w, u8 ***list);
s32 hdpKind10(HdpWalker *w, u8 ***list);
s32 hdpKind13(HdpWalker *w, u8 ***list);
s32 hdpKind1(HdpWalker *w, u8 ***list);
s32 hdpKind2(HdpWalker *w, u8 ***list);
s32 hdpKind3(HdpWalker *w, u8 ***list);
s32 hdpKind4(HdpWalker *w, u8 ***list);
s32 hdpKind5(HdpWalker *w, u8 ***list);
s32 hdpKind8(HdpWalker *w, u8 ***list);
s32 hdpKind9(HdpWalker *w, u8 ***list);
s32 hdpKind11(HdpWalker *w, u8 ***list);
s32 hdpKind12(HdpWalker *w, u8 ***list);

void timIntoVRAM(u32 *tim);
void hdpTestPlane(u8 *st, s32 *pos, HdpPoints *hp);
void hdpInitKindTable(void **table);
void hdpPassRun(void *a0, void *a1, s32 a2, s32 a3, s32 a4);
void hdpWalk(HdpWalker *w);
void hdpWalkPush(HdpWalker *w, u8 *node);
void hdpRotVecSplit(s32 *in, s32 *out);
void hdpRotTransSVector(s16 *m, s16 *v, s32 *out);
void hdpLeafTestPoints(HdpWalker *w);
void hdpApplyParentTransforms(HdpWalker *w, s32 *v);
void hdpLeafTestPointsMark(HdpWalker *w);
void hdpLeafTestSingleBox(HdpWalker *w);
s32 hdpBoxTestModel(HdpWalker *w, u8 *m);
s32 hdpBox(u8 *b, u32 planes, s32 *v, s32 mg, s32 *d);
u8 *csGetRoot(void);
u8 *csListHead(void);
void animControl(s32 a, u8 b);
void mathMulTransVecShort(void *m, void *v, VECTOR *out);
void hdpTestSphereBands(u8 *node, HdpEntry *e);
s32 hdpTestBox(u8 *node, s32 *v, s32 *res, s32 flag);
void hdpSetHitTag(s32 *res, s16 tag, u8 b, HdpEntry *e);
void hdpTestVolumeGroup(HdpEntry *e, s16 a, s16 b);
void mathMulMatVec(void *m, s32 *in, s32 *out);
void hdpPush(u8 *node, s32 *v, s16 a, s16 b, s32 c);
s32 hdpPop(HdpEntry **out);
void hdpSetNodeIds(s16 *a, s16 *b, HdpEntry *e, s16 id, s16 id2);
s32 hdpIsPtInSphere(s32 *p, s32 r, s32 *c);

/* timIntoVRAM: upload a TIM image (and its CLUT, if any) to VRAM. */
void __attribute__((section(".text_8014E4C8"))) timIntoVRAM(u32 *tim)
{
    RECT rect;
    GsIMAGE img;

    GsGetTimInfo(tim + 1, &img);
    rect.x = img.px;
    rect.y = img.py;
    rect.w = img.pw;
    rect.h = img.ph;
    LoadImage(&rect, img.pixel);
    if ((img.pmode >> 3) & 1) {
        rect.x = img.cx;
        rect.y = img.cy;
        rect.w = img.cw;
        rect.h = img.ch;
        LoadImage(&rect, img.clut);
    }
}

/* shellTextureInit: check a "PSXT" texture pack (version 0x43) and upload all
 * TIMs in it; returns the pack's first header word. */
s32 __attribute__((section(".text_8014E558"))) shellTextureInit(u32 *p)
{
    s32 result;
    s32 count;
    s32 i;
    u32 size;

    if (p == 0) {
        return 0;
    }
    if (*p++ != 0x50535854) {
        p--;
        printf(strTexNotTmsFile);
        exit(-1);
    }
    if (*p++ != 0x43) {
        p--;
        printf(strTexWrongVersion);
        printf(strTexVersionFmt, *p, 0x43);
        exit(-1);
    }
    result = *p++;
    count = *p++;
    p += count;
    size = *p;
    for (i = 0; i < count; i++) {
        p++;
        timIntoVRAM(p);
        p += size >> 2;
        size = *p;
    }
    return result;
}

/* hdp: run the hit pass for one object and dispatch its hit models. */
void __attribute__((section(".text_8014E66C"))) hdpCollideObject(void *a0, void *a1, s32 a2, s32 a3, s32 a4)
{
    HdpState *st;
    u32 i;
    u8 **list;
    u8 **pp;
    u8 *m;
    s32 x, y;
    s32 z;

    z = *(s32 *)&a4;    /* (read first, like retail) */
    list = hdpHitModels;
    hdpState->f00 = hdpNodeStackBase;
    st = *(HdpState **)&hdpState;
    x = *(s32 *)&hdpDifficulty;
    y = *(s32 *)&hdpSingleBoxTest;
    st->f04 = hdpKindHandlers;
    st->f1c = x;
    st->f20 = D_801A4728;
    st->fb4 = list;
    st->fc4 = y;
    hdpPassRun(a0, a1, a2, a3, z);
    if (hdpPlaneEnabled) {
        hdpTestPlane(a0, (s32 *)((u8 *)a0 + 168), a1);
    }
    for (i = 0; i < hdpState->fb0; i++) {
        if (*list[i] == 8) {
            m = list[i];
            animControl(*(s32 *)(m + 12), m[11]);
        }
    }
}

/* hdp init: set the node stack base, clear the plane flag, fill the table. */
void __attribute__((section(".text_8014E788"))) hdpInit(s32 base)
{
    hdpNodeStackBase = base;
    hdpPlaneEnabled = 0;
    hdpInitKindTable(hdpKindHandlers);
}

/* hdp: set the value copied into st->f1c on each pass. */
void __attribute__((section(".text_8014E7BC"))) hdpSetDifficulty(s32 v)
{
    hdpDifficulty = v;
}

/* hdp: set the value copied into st->fc4 on each pass (selects the
 * single-box leaf test). */
void __attribute__((section(".text_8014E7C8"))) hdpSetSingleBoxTest(s32 v)
{
    hdpSingleBoxTest = v;
}

/* hdp: test the object's hit points against the global plane hdpPlane. */
void __attribute__((section(".text_8014E824"))) hdpTestPlane(u8 *st, s32 *pos, HdpPoints *hp)
{
    s32 i;
    s32 d;
    VECTOR v;

    for (i = 0; i < *(s32 *)(st + 180); i++) {
        if (hp->pts[i].hit == 0) {
            mathMulTransVecShort(st + 128, st + 184 + i * 8, &v);
            v.vx += pos[0];
            v.vy += pos[1];
            v.vz += pos[2];
            d = (v.vx * hdpPlane.x + v.vy * hdpPlane.y + v.vz * hdpPlane.z) / 4096;
            if (d + hdpPlane.d > 0) {
                hp->pts[i].hit = 1;
                hp->pts[i].f14 = 0;
                hp->pts[i].f1e = hdpPlane.h0e;
                hp->pts[i].f1c = 0;
                hp->pts[i].f18 = hdpPlane.b0d;
                hp->pts[i].nx = hdpPlane.x;
                hp->pts[i].ny = hdpPlane.y;
                hp->pts[i].nz = hdpPlane.z;
                hp->pts[i].depth = d + hdpPlane.d;
            }
        }
    }
}

/* hdp: fill the node-kind dispatch table with the walker's handlers. */
void __attribute__((section(".text_8014E9B0"))) hdpInitKindTable(void **table)
{
    table[0] = hdpKind0;
    table[1] = hdpKind1;
    table[2] = hdpKind2;
    table[3] = hdpKind3;
    table[4] = hdpKind4;
    table[5] = hdpKind5;
    table[6] = hdpKind6;
    table[7] = hdpKind7;
    table[8] = hdpKind8;
    table[9] = hdpKind9;
    table[10] = hdpKind10;
    table[11] = hdpKind11;
    table[12] = hdpKind12;
    table[13] = hdpKind13;
}

/* ---- hdp collision walker (8014EA60..8014FD3C) ------------------------------
 * Retail is SingleTrac hand-written assembly with private register
 * conventions; this is C doing the same work with the normal calling
 * convention. The walker state retail keeps in t0..t8 is an HdpWalker; the
 * scratchpad block is HdpPad. */

/* GTE (cop2) access: hardware instructions C cannot express. */
#define gte_ldR(p)                                                            \
    do {                                                                      \
        u32 *m_ = (u32 *)(p);                                                 \
        u32 a_ = m_[0], b_ = m_[1], c_ = m_[2], d_ = m_[3], e_ = m_[4];       \
        __asm__ volatile ("ctc2 %0,$0\n\tctc2 %1,$1\n\tctc2 %2,$2\n\t"        \
                          "ctc2 %3,$3\n\tctc2 %4,$4"                          \
                          : : "r"(a_), "r"(b_), "r"(c_), "r"(d_), "r"(e_));   \
    } while (0)
#define gte_ldR5(a, b, c, d, e)                                               \
    __asm__ volatile ("ctc2 %0,$0\n\tctc2 %1,$1\n\tctc2 %2,$2\n\t"            \
                      "ctc2 %3,$3\n\tctc2 %4,$4"                              \
                      : : "r"(a), "r"(b), "r"(c), "r"(d), "r"(e))
#define gte_ldIR(x, y, z) \
    __asm__ volatile ("mtc2 %0,$9\n\tmtc2 %1,$10\n\tmtc2 %2,$11" : : "r"(x), "r"(y), "r"(z))
#define gte_ldv0(p) \
    __asm__ volatile ("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)" : : "r"(p))
#define gte_sqr_lm() __asm__ volatile ("cop2 0xA00428")          /* SQR lm */
#define gte_rtir() __asm__ volatile ("nop\n\tcop2 0x41E012")     /* MVMVA R*IR */
#define gte_rtir_sf() __asm__ volatile ("nop\n\tcop2 0x49E012")  /* MVMVA R*IR >> 12 */
#define gte_rtv0_sf() __asm__ volatile ("nop\n\tcop2 0x486012")  /* MVMVA R*V0 >> 12 */
#define gte_stMAC(x, y, z) \
    __asm__ volatile ("mfc2 %0,$25\n\tmfc2 %1,$26\n\tmfc2 %2,$27" : "=r"(x), "=r"(y), "=r"(z))

#define PAD ((HdpPad *)0x1F800000)

/* 8014EA60: walk the hierarchy from w->cur: run each node's kind handler
 * from st->table, push the children it returns, then pop the next entry,
 * until the stack is empty or st->left drops to 0. (Retail handlers push
 * their children themselves, after their last change to the entry.) */
void __attribute__((section(".text_8014EA60"))) hdpWalk(HdpWalker *w)
{
    HdpPad *st = PAD;
    HdpWalk *e;
    u8 **list;
    u32 kind;
    s32 cnt;

    w->xp = st->xstack;
    w->sp = st->stack;
    w->depth = 0;
    while (st->left > 0) {
        kind = *w->cur.node;
        if (kind < 13) {
            cnt = ((s32 (*)(HdpWalker *, u8 ***))st->table[kind])(w, &list);
            for (; cnt > 0; cnt--) {
                hdpWalkPush(w, *list++);
            }
        }
        cnt = w->depth - 1;
        w->depth = cnt;
        if (cnt < 0) {
            break;
        }
        if ((u32)cnt < 33) {
            e = --w->sp;
        } else {
            e = --w->xp;
        }
        w->cur.node = e->node;
        w->cur.x = e->x;
        w->cur.y = e->y;
        w->cur.z = e->z;
        w->cur.lod = e->lod;
        w->cur.ids = e->ids;
    }
}

/* Kind handlers: update the current entry and return the number of children
 * to push (from *list). */

/* kinds 0, 6, 7, 10 and 13: nothing. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind0(HdpWalker *w, u8 ***list)
{
    return 0;
}

/* kind 1: bounding sphere (centre +4, radius +16; flag 4 = always enter):
 * the children (+24, count +20) when the probe is inside. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind1(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 fl = n[21];
    s32 r, in, sum, mx, my, mz;
    u32 id;

    if (fl & 1) {
        return 0;
    }
    r = *(s32 *)(n + 16);
    in = 1;
    if (r < 0xB504) {
        gte_ldIR(*(s32 *)(n + 4) - w->cur.x, *(s32 *)(n + 8) - w->cur.y, *(s32 *)(n + 12) - w->cur.z);
        gte_sqr_lm();
        in = 0;
        gte_stMAC(mx, my, mz);
        sum = mz + (mx + my);
        if (sum >= 0 && sum <= PAD->r2 + r * r) {
            in = 1;
        }
    }
    if ((in | (fl & 4)) == 0) {
        return 0;
    }
    id = *(u16 *)(n + 2);
    if (id) {
        w->cur.ids = (id << 16) | n[22];
    }
    *list = (u8 **)(n + 24);
    return n[20];
}

/* kind 2: distance switch: the child of the first range record (+20, count
 * +16; {max, min, child}) holding the squared distance to the centre (+4,
 * scaled down by the LOD level). */
s32 __attribute__((section(".text_8014EA60"))) hdpKind2(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 sh = w->cur.lod >> 16;
    s32 cnt;
    u32 sum, mx, my, mz;
    u32 **r;

    gte_ldIR((*(s32 *)(n + 4) - w->cur.x) >> sh, (*(s32 *)(n + 8) - w->cur.y) >> sh,
             (*(s32 *)(n + 12) - w->cur.z) >> sh);
    gte_sqr_lm();
    cnt = *(s32 *)(n + 16);
    r = (u32 **)(n + 20);
    gte_stMAC(mx, my, mz);
    sum = mx + my + mz;
    for (; cnt > 0; cnt--, r++) {
        if (sum >= (*r)[1] && sum < (*r)[0]) {
            *list = (u8 **)&(*r)[2];
            return 1;
        }
    }
    return 0;
}

/* kind 4: translation (+4) of the children (+20, count +18). */
s32 __attribute__((section(".text_8014EA60"))) hdpKind4(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 id;

    if (n[19] & 1) {
        return 0;
    }
    w->cur.x -= *(s32 *)(n + 4);
    w->cur.y -= *(s32 *)(n + 8);
    w->cur.z -= *(s32 *)(n + 12);
    id = *(u16 *)(n + 2);
    if (id) {
        w->cur.ids = (id << 16) | *(u16 *)(n + 16);
    }
    *list = (u8 **)(n + 20);
    return n[18];
}

/* kind 5: rotation (+4) and translation (+24) of the children (+40, count
 * +38); the rotation is recorded in st->xf for the leaf tests. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind5(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    s32 v[3];
    u32 id;

    if (n[39] & 1) {
        return 0;
    }
    v[0] = w->cur.x - *(s32 *)(n + 24);
    v[1] = w->cur.y - *(s32 *)(n + 28);
    v[2] = w->cur.z - *(s32 *)(n + 32);
    gte_ldR(n + 4);
    hdpRotVecSplit(v, &w->cur.x);
    PAD->xf[w->cur.lod & 0xFFFF] = n + 4;
    w->cur.lod++;
    id = *(u16 *)(n + 2);
    if (id) {
        w->cur.ids = (id << 16) | *(u16 *)(n + 36);
    }
    *list = (u8 **)(n + 40);
    return n[38];
}

/* kind 8: hit volume leaf. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind8(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;

    if (n[7] != 1) {
        if ((s8)n[5] != -2) {
            hdpLeafTestPoints(w);
        } else if (PAD->fc4 == 0) {
            hdpLeafTestPointsMark(w);
        } else {
            hdpLeafTestSingleBox(w);
        }
    }
    return 0;
}

/* kind 9: child n[7] of the list at +16. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind9(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 id = *(u16 *)(n + 2);

    if (id) {
        w->cur.ids = (id << 16) | *(u16 *)(n + 4);
    }
    *list = (u8 **)(n + 16) + n[7];
    return 1;
}

/* kind 3: child n[12] of the list at +32. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind3(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 id = *(u16 *)(n + 2);

    if (id) {
        w->cur.ids = (id << 16) | *(u16 *)(n + 4);
    }
    *list = (u8 **)(n + 32) + n[12];
    return 1;
}

/* kind 11: the child at +12 when the probe is inside one of the boxes
 * listed at +16 (count +6, at least one is tested), else the one at +8
 * (either only if set). */
s32 __attribute__((section(".text_8014EA60"))) hdpKind11(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 id = *(u16 *)(n + 2);
    s32 cnt;
    u8 **m;

    if (n[4] & 1) {
        return 0;
    }
    if (id) {
        w->cur.ids = (id << 16) | n[5];
    }
    m = (u8 **)(n + 16);
    cnt = n[6];
    *list = (u8 **)(n + 12);
    do {
        if (hdpBoxTestModel(w, *m++)) {
            goto found;
        }
    } while (--cnt > 0);
    *list = (u8 **)(n + 8);
found:
    return **list != 0;
}

/* kind 12: LOD level change (+4): rescale the probe; the children (+8,
 * count +6) are at the new level. */
s32 __attribute__((section(".text_8014EA60"))) hdpKind12(HdpWalker *w, u8 ***list)
{
    u8 *n = w->cur.node;
    u32 lv = *(u16 *)(n + 4);
    s32 d = (w->cur.lod >> 16) - lv;

    if (d < 0) {
        d = -d;
        w->cur.x <<= d;
        w->cur.y <<= d;
        w->cur.z <<= d;
    } else if (d > 0) {
        w->cur.x >>= d;
        w->cur.y >>= d;
        w->cur.z >>= d;
    }
    w->cur.lod = (w->cur.lod & 0xFFFF) | (lv << 16);
    *list = (u8 **)(n + 8);
    return *(u16 *)(n + 6);
}

/* kinds 6, 7, 10 and 13: nothing (retail: the hdpKind0 label). */
s32 __attribute__((section(".text_8014EA60"))) hdpKind6(HdpWalker *w, u8 ***list)
{
    return 0;
}

s32 __attribute__((section(".text_8014EA60"))) hdpKind7(HdpWalker *w, u8 ***list)
{
    return 0;
}

s32 __attribute__((section(".text_8014EA60"))) hdpKind10(HdpWalker *w, u8 ***list)
{
    return 0;
}

s32 __attribute__((section(".text_8014EA60"))) hdpKind13(HdpWalker *w, u8 ***list)
{
    return 0;
}

/* 8014EF3C: push node with the current position/LOD/ids: the first 33
 * entries in the scratchpad, the rest in st->xstack (depth capped at 200;
 * the scratchpad slot's node word is written either way). */
void __attribute__((section(".text_8014EF3C"))) hdpWalkPush(HdpWalker *w, u8 *node)
{
    HdpWalk *e = w->sp;
    s32 n = w->depth;

    e->node = node;
    if ((u32)n < 33) {
        w->sp = e + 1;
        n++;
    } else {
        e = w->xp;
        e->node = node;
        if ((u32)n < 200) {
            w->xp = e + 1;
            n++;
        }
    }
    w->depth = n;
    e->x = w->cur.x;
    e->y = w->cur.y;
    e->z = w->cur.z;
    e->lod = w->cur.lod;
    e->ids = w->cur.ids;
}

/* 8014EF98: rotate in[] by the GTE rotation matrix in split precision (the
 * bits from 15 up and the low 15 bits in two MVMVA passes) into out[]
 * (which may be in). */
void __attribute__((section(".text_8014EF98"))) hdpRotVecSplit(s32 *in, s32 *out)
{
    s32 x, y, z, hx, hy, hz, ax, ay, az, bx, by, bz;

    x = in[0];
    y = in[1];
    z = in[2];
    if (x < 0) {
        x = -x;
        hx = -(x >> 15);
        x = -(x & 0x7FFF);
    } else {
        hx = x >> 15;
        x &= 0x7FFF;
    }
    if (y < 0) {
        y = -y;
        hy = -(y >> 15);
        y = -(y & 0x7FFF);
    } else {
        hy = y >> 15;
        y &= 0x7FFF;
    }
    if (z < 0) {
        z = -z;
        hz = -(z >> 15);
        z = -(z & 0x7FFF);
    } else {
        hz = z >> 15;
        z &= 0x7FFF;
    }
    gte_ldIR(hx, hy, hz);
    gte_rtir();
    gte_stMAC(ax, ay, az);
    gte_ldIR(x, y, z);
    gte_rtir_sf();
    gte_stMAC(bx, by, bz);
    out[0] = bx + (ax << 3);
    out[1] = by + (ay << 3);
    out[2] = bz + (az << 3);
}

/* 8014F0CC: out = transpose(m) * v >> 12 on the GTE (m in MATRIX layout,
 * v a short vector). */
void __attribute__((section(".text_8014F0CC"))) hdpRotTransSVector(s16 *m, s16 *v, s32 *out)
{
    gte_ldR5((u16)m[0] | (m[3] << 16), (u16)m[6] | (m[1] << 16), (u16)m[4] | (m[7] << 16),
             (u16)m[2] | (m[5] << 16), *(s32 *)(m + 8));
    gte_ldv0(v);
    gte_rtv0_sf();
    gte_stMAC(out[0], out[1], out[2]);
}

/* 8014F168: kind 8 leaf test: for each hit point not yet hit, transform it
 * into the node's space and test it against the box (n+16); on a hit record
 * the plane (n[5], or when negative the one the point's previous position
 * was outside of with the least penetration), the depth, the normal back in
 * world space and the ids, and add the node to the hit model list. */
void __attribute__((section(".text_8014F168"))) hdpLeafTestPoints(HdpWalker *w)
{
    HdpPad *st = PAD;
    u8 *n = w->cur.node;
    HdpPoint *p;
    s16 *lp;
    s16 *m;
    s32 *t;
    u8 **xf;
    u8 **h;
    s32 i, j, k, cnt, best, dep, x, y, z, x2, y2;
    s32 v[3];
    s32 o[3];

    for (i = st->npts, p = st->pts, lp = st->lpts; i > 0; i--, p++, lp += 4) {
        if (p->hit) {
            continue;
        }
        hdpRotTransSVector((s16 *)(st->obj + 128), lp, v);
        t = (s32 *)(st->obj + 168);
        st->w[0] = t[0] + v[0];
        st->w[1] = t[1] + v[1];
        st->w[2] = t[2] + v[2];
        hdpApplyParentTransforms(w, v);
        st->w[3] = v[0] + w->cur.x;
        st->w[4] = v[1] + w->cur.y;
        st->w[5] = v[2] + w->cur.z;
        if (!hdpBox(n + 16, n[4], st->w + 3, 0, st->d)) {
            continue;
        }
        k = (s8)n[5];
        if (k < 0) {
            hdpRotTransSVector(st->prevm, lp, v);
            t = st->prevp;
            v[0] = t[0] + v[0] - st->w[0];
            v[1] = t[1] + v[1] - st->w[1];
            v[2] = t[2] + v[2] - st->w[2];
            hdpApplyParentTransforms(w, v);
            v[0] += st->w[3];
            v[1] += st->w[4];
            v[2] += st->w[5];
            gte_ldR(n + 16);
            hdpRotVecSplit(v, o);
            t = (s32 *)(n + 36);
            st->w[0] = o[0] + t[0];
            st->w[1] = o[1] + t[1];
            st->w[2] = o[2] + t[2];
            if (n[4] >= 4) {
                gte_ldR(n + 48);
                hdpRotVecSplit(v, o);
                t = (s32 *)(n + 68);
                st->w[3] = o[0] + t[0];
                st->w[4] = o[1] + t[1];
                st->w[5] = o[2] + t[2];
            }
            cnt = n[4];
            best = 0x7FFF0000;
            for (j = 0; j < cnt; j++) {
                if (st->w[j] <= 0 && st->d[j] < best) {
                    best = st->d[j];
                    k = j;
                }
            }
            if (k < 0) {
                best = 0x7FFF0000;
                for (j = 0; j < cnt; j++) {
                    if (st->w[j] < best) {
                        best = st->w[j];
                        k = j;
                    }
                }
            }
        }
        st->nrm = (s16 *)(n + k * 6 + (k < 3 ? 16 : 30));
        st->dep = &st->d[k];
        dep = *st->dep;
        x = (w->cur.lod >> 16) - st->shift;
        if (x < 0) {
            dep <<= -x;
        } else if (x != 0) {
            dep >>= x;
        }
        if (p->hit == 1 && dep < p->depth) {
            continue;
        }
        p->hit = 1;
        p->f14 = (s32)st->hitobj;
        p->depth = dep;
        k = (w->cur.lod & 0xFFFF) - 1;
        m = st->nrm;
        x = m[0];
        y = m[1];
        z = m[2];
        for (xf = st->xf + k; k >= 0; k--, xf--) {
            m = (s16 *)*xf;
            x2 = (m[0] * x + m[3] * y + m[6] * z) / 4096;
            y2 = (m[1] * x + m[4] * y + m[7] * z) / 4096;
            z = (m[2] * x + m[5] * y + m[8] * z) / 4096;
            x = x2;
            y = y2;
        }
        p->nx = x;
        p->ny = y;
        p->nz = z;
        *(u32 *)&p->f1c = w->cur.ids;
        x = *(s16 *)(n + 2);
        if (x) {
            p->f1c = n[6];
            p->f1e = x;
        }
        *(s32 *)&p->f18 = n[8];
        if (*(s32 *)(n + 12) == 0) {
            continue;
        }
        cnt = st->nhit;
        if (cnt >= 10) {
            continue;
        }
        for (h = st->hits; cnt > 0 && *h != n; cnt--) {
            h++;
        }
        if (cnt <= 0) {
            *h = n;
            st->nhit = st->nhit + 1;
        }
    }
}

/* 8014F6B8: apply the current path's parent rotations (st->xf; count in
 * the low half of lod) to v, then rescale v from the node's LOD level to
 * the base level st->shift. */
void __attribute__((section(".text_8014F6B8"))) hdpApplyParentTransforms(HdpWalker *w, s32 *v)
{
    HdpPad *st = PAD;
    s32 k = w->cur.lod & 0xFFFF;
    u8 **xf = st->xf;
    s32 d;

    for (; k > 0; k--) {
        gte_ldR(*xf++);
        hdpRotVecSplit(v, v);
    }
    d = (w->cur.lod >> 16) - st->shift;
    if (d < 0) {
        d = -d;
        v[0] >>= d;
        v[1] >>= d;
        v[2] >>= d;
    } else if (d > 0) {
        v[0] <<= d;
        v[1] <<= d;
        v[2] <<= d;
    }
}

/* 8014F750: kind 8 leaf test without st->fc4: mark each hit point (not yet
 * hit) inside the box with 2 (no LOD rescale). */
void __attribute__((section(".text_8014F750"))) hdpLeafTestPointsMark(HdpWalker *w)
{
    HdpPad *st = PAD;
    u8 *n = w->cur.node;
    HdpPoint *p;
    s16 *lp;
    u8 **xf;
    s32 i, k;
    s32 v[3];
    s32 d[6];

    for (i = st->npts, p = st->pts, lp = st->lpts; i > 0; i--, p++, lp += 4) {
        if (p->hit) {
            continue;
        }
        hdpRotTransSVector((s16 *)(st->obj + 128), lp, v);
        for (k = w->cur.lod & 0xFFFF, xf = st->xf; k > 0; k--) {
            gte_ldR(*xf++);
            hdpRotVecSplit(v, v);
        }
        v[0] += w->cur.x;
        v[1] += w->cur.y;
        v[2] += w->cur.z;
        if (hdpBox(n + 16, n[4], v, 0, d)) {
            p->hit = 2;
        }
    }
}

/* 8014F8C8: kind 8 leaf test with st->fc4 set: one box test of the probe
 * with margin st->margin; a hit ends the pass (st->left = 0, st->single). */
void __attribute__((section(".text_8014F8C8"))) hdpLeafTestSingleBox(HdpWalker *w)
{
    HdpPad *st = PAD;
    u8 *n = w->cur.node;
    s32 d[6];

    if (hdpBox(n + 16, n[4], &w->cur.x, st->margin, d)) {
        st->left = 0;
        st->single = 1;
    }
}

/* 8014F9C8: kind 11 box test of the probe against the box record m. */
s32 __attribute__((section(".text_8014F9C8"))) hdpBoxTestModel(HdpWalker *w, u8 *m)
{
    s32 d[6];

    return hdpBox(m + 4, m[0], &w->cur.x, 0, d);
}

/* box test: v rotated by the first matrix (b) plus its offset (b+20) must be
 * positive on x, y and z, and with 4..6 planes also on x, y, z of the second
 * matrix (b+32, offset b+52); margin mg is taken off the x and y values.
 * The plane values go to d[] as they are tested. */
s32 __attribute__((section(".text_8014F9C8"))) hdpBox(u8 *b, u32 planes, s32 *v, s32 mg, s32 *d)
{
    s32 *t;
    s32 y, z;
    s32 o[3];

    gte_ldR(b);
    hdpRotVecSplit(v, o);
    t = (s32 *)(b + 20);
    if ((d[0] = o[0] + t[0] - mg) <= 0) {
        return 0;
    }
    if ((y = o[1] + t[1] - mg) <= 0) {
        return 0;
    }
    if ((z = o[2] + t[2]) <= 0) {
        return 0;
    }
    d[1] = y;
    d[2] = z;
    if (planes < 4) {
        return 1;
    }
    gte_ldR(b + 32);
    hdpRotVecSplit(v, o);
    t = (s32 *)(b + 52);
    if ((d[3] = o[0] + t[0] - mg) <= 0) {
        return 0;
    }
    if (planes < 5) {
        return 1;
    }
    if ((d[4] = o[1] + t[1] - mg) <= 0) {
        return 0;
    }
    if (planes < 6) {
        return 1;
    }
    if ((d[5] = o[2] + t[2]) <= 0) {
        return 0;
    }
    return 1;
}

/* 8014FAB0: hdp pass for obj (hit points a1, previous rotation/position
 * a3/a4): reset the hit records and the hit model list, walk the root
 * object's hierarchy (when a2 is set), then those of the other listed
 * objects while points remain. */
void __attribute__((section(".text_8014FAB0"))) hdpPassRun(void *a0, void *a1, s32 a2, s32 a3, s32 a4)
{
    HdpPad *st = PAD;
    u8 *obj = a0;
    HdpPoint *p;
    u8 **h;
    u8 *node;
    u8 *o;
    s32 n, r, left;
    s32 v[3];
    HdpWalker w;

    st->obj = obj;
    n = *(s32 *)(obj + 180);
    st->npts = n;
    st->left = n;
    st->lpts = (s16 *)(obj + 184);
    p = ((HdpPoints *)a1)->pts;
    st->pts = p;
    st->prevm = (void *)a3;
    st->prevp = (s32 *)a4;
    st->nhit = 0;
    st->single = 0;
    do {
        p->hit = 0;
        p->f14 = 0;
        *(s32 *)&p->f18 = 0;
        p->f1c = -1;
        p->f1e = 0;
        p++;
    } while (--n > 0);
    h = st->hits;
    n = 10;
    do {
        *h++ = 0;
    } while (--n > 0);
    r = 100;
    node = *(u8 **)(obj + 284);
    if (node != 0 && *node == 1) {
        r = *(s32 *)(node + 16);
    }
    st->margin = r;
    o = csGetRoot();
    left = (s32)p; /* (retail's counter register still holds the record pointer) */
    if (a2 != 0 && (node = *(u8 **)(o + 284)) != 0) {
        st->hitobj = o;
        st->r2 = r * r;
        w.cur.node = node;
        w.cur.x = *(s32 *)(obj + 168);
        w.cur.y = *(s32 *)(obj + 172);
        w.cur.z = *(s32 *)(obj + 176);
        w.cur.lod = st->shift << 16;
        w.cur.ids = 0;
        hdpWalk(&w);
        left = st->left;
        for (n = st->npts, p = st->pts; n > 0; n--, p++) {
            if (st->single || p->hit == 2) {
                p->hit = 0;
                left++;
            }
        }
    }
    st->left = left;
    for (o = csListHead(); st->left > 0; o = *(u8 **)(st->hitobj + 288)) {
        st->hitobj = o;
        if (o == 0) {
            break;
        }
        if ((*(u32 *)(o + 8) & *(u32 *)(o + 12)) && st->obj != o) {
            st->xf[0] = o + 128;
            gte_ldR(o + 128);
            obj = st->obj;
            v[0] = *(s32 *)(obj + 168) - *(s32 *)(o + 168);
            v[1] = *(s32 *)(obj + 172) - *(s32 *)(o + 172);
            v[2] = *(s32 *)(obj + 176) - *(s32 *)(o + 176);
            w.cur.node = *(u8 **)(o + 284);
            hdpRotVecSplit(v, &w.cur.x);
            w.cur.lod = (st->shift << 16) | 1;
            w.cur.ids = 0;
            hdpWalk(&w);
        }
    }
}


/* traversal stack entry n: the first 41 live in hdpStackScratch, the rest in
 * hdpEntryStack */
static __inline__ HdpEntry *hdpEntryAt(s32 n)
{
    if ((u32)n < 41) {
        return &hdpStackScratch[n];
    }
    return &hdpEntryStack[n];
}

/* Fill one hdp stack entry (inlined into each branch of hdpPush). */
static inline void hdp_put(HdpEntry *e, u8 *node, s32 *v, s16 a, s16 b, s32 c)
{
    e->node = node;
    e->v[0] = v[0];
    e->v[1] = v[1];
    e->v[2] = v[2];
    e->a = a;
    e->b = b;
    e->c = c;
}

/* hdp: push an entry on the traversal stack (first 41 in hdpStackScratch, the rest
 * in hdpEntryStack; depth capped at 200). Same C as src/ (byte-identical). */
void __attribute__((section(".text_8014FD3C"))) hdpPush(u8 *node, s32 *v, s16 a, s16 b, s32 c)
{
    s32 n = hdpStackDepth;
    s32 n2;

    /* Each branch fills its own entry (the compiler merges the tails). */
    if ((u32)n < 41) {
        hdp_put((HdpEntry *)(n * sizeof(HdpEntry) + (u32)hdpStackScratch), node, v, a, b, c);
    } else {
        hdp_put((HdpEntry *)(n * sizeof(HdpEntry) + (u32)hdpEntryStack), node, v, a, b, c);
    }
    n2 = *(s32 *)&hdpStackDepth;
    if (n2 < 200) {
        hdpStackDepth = n2 + 1;
    }
}

/* hdp: pop the top traversal entry into *out; returns the new depth. */
s32 __attribute__((section(".text_8014FDC0"))) hdpPop(HdpEntry **out)
{
    s32 n = hdpStackDepth - 1;

    hdpStackDepth = n;
    if (n >= 0) {
        *out = hdpEntryAt(n);
    }
    return hdpStackDepth;
}

/* hdpSetNodeIds: node ids from the arguments, or from the entry when id is 0. */
void __attribute__((section(".text_8014FE0C"))) hdpSetNodeIds(s16 *a, s16 *b, HdpEntry *e, s16 id, s16 id2)
{
    if (id == 0) {
        *a = e->a;
        *b = e->b;
    } else {
        *a = id;
        *b = id2;
    }
}

/* hdpIsPtInSphere: point p within radius r of centre c (large radius: yes). */
s32 __attribute__((section(".text_8014FE50"))) hdpIsPtInSphere(s32 *p, s32 r, s32 *c)
{
    s32 d[3];
    s32 sum;

    if (r <= 0xB504) {
        d[0] = p[0] - c[0];
        d[1] = p[1] - c[1];
        d[2] = p[2] - c[2];
        sum = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        if (sum < 0 || sum >= r * r) {
            return 0;
        }
    }
    return 1;
}

/* hdpTraverse: depth-first walk of an object's hit hierarchy from obj->f11C,
 * dispatching on each node's kind, until res[4] is set or the stack empties. */
void __attribute__((section(".text_8014FEEC"))) hdpTraverse(u8 *obj, s32 *vec, s32 *res)
{
    HdpEntry *e;
    s16 ida, idb;

    e = 0;
    hdpStackDepth = 0;
    hdpPush(((HdpObj *)obj)->f11C, vec, 0, 0, hdpShift);
    while (res[4] == 0) {
        if (hdpPop(&e) < 0) {
            break;
        }
        switch (*e->node) {
        case 0:
        default:
            break;
        case 8: {
            u8 *n = e->node;

            if (n[7] != 1) {
                res[4] = hdpTestBox(n, e->v, res, (s8)n[5] == -2);
                if (res[0]) {
                    hdpSetHitTag(res, *(s16 *)(n + 2), n[6], e);
                    if (*(s32 *)(n + 12)) {
                        animControl(*(s32 *)(n + 12), n[11]);
                    }
                }
            }
            break;
        }
        case 1: {
            u8 *n = e->node;
            s32 i;
            s32 cnt;

            if (!(n[21] & 1) && hdpIsPtInSphere((s32 *)(n + 4), *(s32 *)(n + 16), e->v)) {
                hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), *(s16 *)(n + 22));
                cnt = n[20];
                for (i = 0; i < cnt; i++) {
                    hdpPush(*(u8 **)(n + 24), e->v, ida, idb, e->c);
                    n += 4;
                }
            }
            break;
        }
        case 2:
            hdpTestSphereBands(e->node, e);
            break;
        case 4: {
            u8 *n = e->node;
            s32 i;

            if (!(n[19] & 1)) {
                e->v[0] -= *(s32 *)(n + 4);
                e->v[1] -= *(s32 *)(n + 8);
                e->v[2] -= *(s32 *)(n + 12);
                hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), *(s16 *)(n + 16));
                for (i = 0; i < n[18]; i++) {
                    hdpPush(((u8 **)(n + 20))[i], e->v, ida, idb, e->c);
                }
            }
            break;
        }
        case 5: {
            u8 *n = e->node;
            s32 i;

            if (!(n[39] & 1)) {
                s32 tmp[3];

                e->v[0] -= *(s32 *)(n + 24);
                e->v[1] -= *(s32 *)(n + 28);
                e->v[2] -= *(s32 *)(n + 32);
                tmp[0] = e->v[0];
                tmp[1] = e->v[1];
                tmp[2] = e->v[2];
                mathMulMatVec(n + 4, tmp, e->v);
                hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), *(s16 *)(n + 36));
                for (i = 0; i < n[38]; i++) {
                    hdpPush(((u8 **)(n + 40))[i], e->v, ida, idb, e->c);
                }
            }
            break;
        }
        case 9: {
            u8 *n = e->node;

            hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), *(s16 *)(n + 4));
            hdpPush(*(u8 **)(n + 16 + n[7] * 4), e->v, ida, idb, e->c);
            break;
        }
        case 3: {
            u8 *n = e->node;

            hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), n[4]);
            hdpPush(*(u8 **)(n + 32 + n[12] * 4), e->v, ida, idb, e->c);
            break;
        }
        case 11: {
            u8 *n = e->node;

            if (!(n[4] & 1)) {
                hdpSetNodeIds(&ida, &idb, e, *(s16 *)(n + 2), n[5]);
                hdpTestVolumeGroup(e, ida, idb);
            }
            break;
        }
        case 12: {
            u8 *n = e->node;
            s32 i;
            s32 sh;

            sh = e->c - *(u16 *)(n + 4);
            if (sh < 0) {
                sh = __builtin_abs(sh);
                e->v[0] <<= sh;
                e->v[1] <<= sh;
                e->v[2] <<= sh;
            } else if (sh > 0) {
                e->v[0] >>= sh;
                e->v[1] >>= sh;
                e->v[2] >>= sh;
            }
            for (i = 0; i < *(u16 *)(n + 6); i++) {
                hdpPush(((u8 **)(n + 8))[i], e->v, e->a, e->b, *(u16 *)(n + 4));
            }
            break;
        }
        case 13:
            break;
        }
    }
}
