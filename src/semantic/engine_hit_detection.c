/* Library code 8014E000..80150000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything in this range is SingleTrac engine code (TIM upload, shell
 * texture packs, the hit-detection pass "hdp*"); no public source exists, all
 * of it was written against the retail code. Recipe: like the game
 * (psyq3.6, -fvolatile, extern gp).
 *
 * 8014EA60..8014FD3C is hand-written assembly (the hdp collision walker: no
 * stack frame, state in the scratchpad at 0x1F800000, gp/sp/s8 used as data
 * registers, internal calls with private register conventions). Those
 * functions are kept as their original instruction sequence in top-level asm
 * blocks; the table filled by hdpInitKindTable points at the handler labels
 * hdpKind0..13 inside hdpWalk.
 *
 * Top-level asm notes: ".set\tnoreorder" (TAB) keeps maspsx from adding nops;
 * the block ends with ".set\treorder" (resets maspsx) and ".set noreorder"
 * (space: seen only by the assembler, which the compiler output expects). */
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

/* hdp state record (the scratchpad block at 0x1F800000 used by the asm). */
typedef struct {
    s32 f00;            /* 0x00 node stack base (hdpNodeStackBase) */
    void **f04;         /* 0x04 node-kind dispatch table (hdpKindHandlers) */
    s32 pad08[5];
    s32 f1c;            /* 0x1C hdpDifficulty */
    void *f20;          /* 0x20 hdpPathRotations */
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
extern s32 hdpPathRotations[];

extern char strTexNotTmsFile[];
extern char strTexWrongVersion[];
extern char strTexVersionFmt[];

/* node-kind handlers inside the hand-written walker hdpWalk (kinds
 * 0, 6, 7, 10 and 13 share the "skip" entry at 8014EA74) */
extern void hdpKind0(void);
extern void hdpKind1(void);
extern void hdpKind2(void);
extern void hdpKind3(void);
extern void hdpKind4(void);
extern void hdpKind5(void);
extern void hdpKind6(void);
extern void hdpKind7(void);
extern void hdpKind8(void);
extern void hdpKind9(void);
extern void hdpKind10(void);
extern void hdpKind11(void);
extern void hdpKind12(void);
extern void hdpKind13(void);

void timIntoVRAM(u32 *tim);
void hdpTestPlane(u8 *st, s32 *pos, HdpPoints *hp);
void hdpInitKindTable(void **table);
void hdpPassRun(void *a0, void *a1, s32 a2, s32 a3, s32 a4);
void animControl(s32 a, s32 b);
void mathMulTransVecShort(void *m, void *v, VECTOR *out);
void hdpTestSphereBands(u8 *node, HdpEntry *e);
s32 hdpTestBox(u8 *node, s32 *v, s32 *res, s32 flag);
void hdpSetHitTag(s32 *res, s16 tag, s16 b, HdpEntry *e);
void hdpTestVolumeGroup(HdpEntry *e, s32 a, s16 b);  /* defined with s32 a: callers pass a sign-extended s16 */
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
    st->f20 = hdpPathRotations;
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

/* 8014EA60 (hand-written asm): walker main loop: pops entries (node in t0, position t1..t3, shift/ids
 * t4/t5) off its private stack and dispatches on the node kind through the
 * table at st->f04; the hdpKindN labels are the handlers. */
__asm__(
    ".section .text_8014EA60,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpWalk\n"
    ".type hdpWalk,@function\n"
    "hdpWalk:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $31,204($16)\n"   /* 8014EA60: ae1f00cc */
    "\tlw $15,0($16)\n"   /* 8014EA64: 8e0f0000 */
    "\taddiu $14,$16,208\n"   /* 8014EA68: 260e00d0 */
    "\tj .L8014EAC4\n"   /* 8014EA6C: 08053ab1 */
    "\taddu $24,$0,$0\n"   /* 8014EA70: 0000c021 */
    ".L8014EA74:\n"
    ".globl hdpKind0\n"
    "hdpKind0:\n"
    ".globl hdpKind6\n"
    "hdpKind6:\n"
    ".globl hdpKind7\n"
    "hdpKind7:\n"
    ".globl hdpKind10\n"
    "hdpKind10:\n"
    ".globl hdpKind13\n"
    "hdpKind13:\n"
    "\taddiu $24,$24,-1\n"   /* 8014EA74: 2718ffff */
    "\tbltz $24,.L8014EAC4\n"   /* 8014EA78: 07000012 */
    "\tsltiu $17,$24,33\n"   /* 8014EA7C: 2f110021 */
    "\tbeq $17,$0,.L8014EAA8\n"   /* 8014EA80: 12200009 */
    "\tsll $0,$0,0x0\n"   /* 8014EA84: 00000000 */
    "\taddiu $14,$14,-24\n"   /* 8014EA88: 25ceffe8 */
    "\tlw $8,0($14)\n"   /* 8014EA8C: 8dc80000 */
    "\tlw $9,4($14)\n"   /* 8014EA90: 8dc90004 */
    "\tlw $10,8($14)\n"   /* 8014EA94: 8dca0008 */
    "\tlw $11,12($14)\n"   /* 8014EA98: 8dcb000c */
    "\tlw $12,16($14)\n"   /* 8014EA9C: 8dcc0010 */
    "\tj .L8014EAC4\n"   /* 8014EAA0: 08053ab1 */
    "\tlw $13,20($14)\n"   /* 8014EAA4: 8dcd0014 */
    ".L8014EAA8:\n"
    "\taddiu $15,$15,-24\n"   /* 8014EAA8: 25efffe8 */
    "\tlw $8,0($15)\n"   /* 8014EAAC: 8de80000 */
    "\tlw $9,4($15)\n"   /* 8014EAB0: 8de90004 */
    "\tlw $10,8($15)\n"   /* 8014EAB4: 8dea0008 */
    "\tlw $11,12($15)\n"   /* 8014EAB8: 8deb000c */
    "\tlw $12,16($15)\n"   /* 8014EABC: 8dec0010 */
    "\tlw $13,20($15)\n"   /* 8014EAC0: 8ded0014 */
    ".L8014EAC4:\n"
    "\tlw $18,188($16)\n"   /* 8014EAC4: 8e1200bc */
    "\tbltz $24,.L8014EF2C\n"   /* 8014EAC8: 07000118 */
    "\tsll $0,$0,0x0\n"   /* 8014EACC: 00000000 */
    "\tblez $18,.L8014EF2C\n"   /* 8014EAD0: 1a400116 */
    "\tsll $0,$0,0x0\n"   /* 8014EAD4: 00000000 */
    "\tlbu $18,0($8)\n"   /* 8014EAD8: 91120000 */
    "\tlw $19,4($16)\n"   /* 8014EADC: 8e130004 */
    "\tsltiu $20,$18,13\n"   /* 8014EAE0: 2e54000d */
    "\tbeq $20,$0,.L8014EA74\n"   /* 8014EAE4: 1280ffe3 */
    "\tsll $18,$18,0x2\n"   /* 8014EAE8: 00129080 */
    "\taddu $18,$19,$18\n"   /* 8014EAEC: 02729021 */
    "\tlw $18,0($18)\n"   /* 8014EAF0: 8e520000 */
    "\tsll $0,$0,0x0\n"   /* 8014EAF4: 00000000 */
    "\tjr $18\n"   /* 8014EAF8: 02400008 */
    "\tsll $0,$0,0x0\n"   /* 8014EAFC: 00000000 */
    ".globl hdpKind1\n"
    "hdpKind1:\n"
    "\tlbu $17,21($8)\n"   /* 8014EB00: 91110015 */
    "\tlbu $18,20($8)\n"   /* 8014EB04: 91120014 */
    "\tandi $19,$17,0x1\n"   /* 8014EB08: 32330001 */
    "\tbne $19,$0,.L8014EA74\n"   /* 8014EB0C: 1660ffd9 */
    "\tandi $20,$17,0x4\n"   /* 8014EB10: 32340004 */
    "\tlw $21,16($8)\n"   /* 8014EB14: 8d150010 */
    "\taddiu $22,$8,4\n"   /* 8014EB18: 25160004 */
    "\tori $3,$0,0xb504\n"   /* 8014EB1C: 3403b504 */
    "\tslt $23,$21,$3\n"   /* 8014EB20: 02a3b82a */
    "\tbeq $23,$0,.L8014EB90\n"   /* 8014EB24: 12e0001a */
    "\tori $2,$0,0x1\n"   /* 8014EB28: 34020001 */
    "\tlw $23,0($22)\n"   /* 8014EB2C: 8ed70000 */
    "\tlw $3,4($22)\n"   /* 8014EB30: 8ec30004 */
    "\tlw $4,8($22)\n"   /* 8014EB34: 8ec40008 */
    "\tsubu $23,$23,$9\n"   /* 8014EB38: 02e9b823 */
    "\tsubu $3,$3,$10\n"   /* 8014EB3C: 006a1823 */
    "\tsubu $4,$4,$11\n"   /* 8014EB40: 008b2023 */
    "\tmtc2 $23,$9\n"   /* 8014EB44: 48974800 */
    "\tmtc2 $3,$10\n"   /* 8014EB48: 48835000 */
    "\tmtc2 $4,$11\n"   /* 8014EB4C: 48845800 */
    "\tcop2 0xa00428\n"   /* 8014EB50: 4aa00428 */
    "\tmult $21,$21\n"   /* 8014EB54: 02b50018 */
    "\tlw $23,12($16)\n"   /* 8014EB58: 8e17000c */
    "\tmflo $3\n"   /* 8014EB5C: 00001812 */
    "\taddu $2,$0,$0\n"   /* 8014EB60: 00001021 */
    "\tmfc2 $4,$25\n"   /* 8014EB64: 4804c800 */
    "\tmfc2 $5,$26\n"   /* 8014EB68: 4805d000 */
    "\tmfc2 $6,$27\n"   /* 8014EB6C: 4806d800 */
    "\taddu $4,$4,$5\n"   /* 8014EB70: 00852021 */
    "\taddu $6,$6,$4\n"   /* 8014EB74: 00c43021 */
    "\tbltz $6,.L8014EB90\n"   /* 8014EB78: 04c00005 */
    "\tadd $3,$23,$3\n"   /* 8014EB7C: 02e31820 */
    "\tslt $3,$3,$6\n"   /* 8014EB80: 0066182a */
    "\tbne $3,$0,.L8014EB90\n"   /* 8014EB84: 14600002 */
    "\tsll $0,$0,0x0\n"   /* 8014EB88: 00000000 */
    "\tori $2,$0,0x1\n"   /* 8014EB8C: 34020001 */
    ".L8014EB90:\n"
    "\tor $23,$2,$20\n"   /* 8014EB90: 0054b825 */
    "\tbeq $23,$0,.L8014EA74\n"   /* 8014EB94: 12e0ffb7 */
    "\taddiu $22,$8,24\n"   /* 8014EB98: 25160018 */
    "\tlhu $19,2($8)\n"   /* 8014EB9C: 95130002 */
    "\tlbu $20,22($8)\n"   /* 8014EBA0: 91140016 */
    "\tbeq $19,$0,.L8014EBB4\n"   /* 8014EBA4: 12600003 */
    "\tsll $0,$0,0x0\n"   /* 8014EBA8: 00000000 */
    "\tsll $19,$19,0x10\n"   /* 8014EBAC: 00139c00 */
    "\tor $13,$19,$20\n"   /* 8014EBB0: 02746825 */
    ".L8014EBB4:\n"
    "\tblez $18,.L8014EA74\n"   /* 8014EBB4: 1a40ffaf */
    "\tlw $4,0($22)\n"   /* 8014EBB8: 8ec40000 */
    "\taddiu $18,$18,-1\n"   /* 8014EBBC: 2652ffff */
    "\tjal hdpWalkPush\n"   /* 8014EBC0: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014EBC4: 00000000 */
    "\tj .L8014EBB4\n"   /* 8014EBC8: 08053aed */
    "\taddiu $22,$22,4\n"   /* 8014EBCC: 26d60004 */
    ".globl hdpKind2\n"
    "hdpKind2:\n"
    "\tlw $17,4($8)\n"   /* 8014EBD0: 8d110004 */
    "\tlw $18,8($8)\n"   /* 8014EBD4: 8d120008 */
    "\tlw $19,12($8)\n"   /* 8014EBD8: 8d13000c */
    "\tsubu $17,$17,$9\n"   /* 8014EBDC: 02298823 */
    "\tsubu $18,$18,$10\n"   /* 8014EBE0: 024a9023 */
    "\tsubu $19,$19,$11\n"   /* 8014EBE4: 026b9823 */
    "\tsrl $20,$12,0x10\n"   /* 8014EBE8: 000ca402 */
    "\tsrav $17,$17,$20\n"   /* 8014EBEC: 02918807 */
    "\tsrav $18,$18,$20\n"   /* 8014EBF0: 02929007 */
    "\tsrav $19,$19,$20\n"   /* 8014EBF4: 02939807 */
    "\tmtc2 $17,$9\n"   /* 8014EBF8: 48914800 */
    "\tmtc2 $18,$10\n"   /* 8014EBFC: 48925000 */
    "\tmtc2 $19,$11\n"   /* 8014EC00: 48935800 */
    "\tcop2 0xa00428\n"   /* 8014EC04: 4aa00428 */
    "\tlw $18,16($8)\n"   /* 8014EC08: 8d120010 */
    "\taddiu $19,$8,20\n"   /* 8014EC0C: 25130014 */
    "\taddiu $18,$18,1\n"   /* 8014EC10: 26520001 */
    "\tmfc2 $17,$25\n"   /* 8014EC14: 4811c800 */
    "\tmfc2 $20,$26\n"   /* 8014EC18: 4814d000 */
    "\tmfc2 $21,$27\n"   /* 8014EC1C: 4815d800 */
    "\taddu $17,$17,$20\n"   /* 8014EC20: 02348821 */
    "\taddu $17,$17,$21\n"   /* 8014EC24: 02358821 */
    ".L8014EC28:\n"
    "\taddiu $18,$18,-1\n"   /* 8014EC28: 2652ffff */
    "\tblez $18,.L8014EC68\n"   /* 8014EC2C: 1a40000e */
    "\tsll $0,$0,0x0\n"   /* 8014EC30: 00000000 */
    "\tlw $22,0($19)\n"   /* 8014EC34: 8e760000 */
    "\tsll $0,$0,0x0\n"   /* 8014EC38: 00000000 */
    "\tlw $20,4($22)\n"   /* 8014EC3C: 8ed40004 */
    "\tlw $21,0($22)\n"   /* 8014EC40: 8ed50000 */
    "\tsltu $20,$17,$20\n"   /* 8014EC44: 0234a02b */
    "\tbne $20,$0,.L8014EC28\n"   /* 8014EC48: 1680fff7 */
    "\taddiu $19,$19,4\n"   /* 8014EC4C: 26730004 */
    "\tsltu $21,$17,$21\n"   /* 8014EC50: 0235a82b */
    "\tbeq $21,$0,.L8014EC28\n"   /* 8014EC54: 12a0fff4 */
    "\tsll $0,$0,0x0\n"   /* 8014EC58: 00000000 */
    "\tlw $4,8($22)\n"   /* 8014EC5C: 8ec40008 */
    "\tjal hdpWalkPush\n"   /* 8014EC60: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014EC64: 00000000 */
    ".L8014EC68:\n"
    "\tj .L8014EA74\n"   /* 8014EC68: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EC6C: 00000000 */
    ".globl hdpKind4\n"
    "hdpKind4:\n"
    "\tlbu $17,19($8)\n"   /* 8014EC70: 91110013 */
    "\tlbu $18,18($8)\n"   /* 8014EC74: 91120012 */
    "\tandi $19,$17,0x1\n"   /* 8014EC78: 32330001 */
    "\tbne $19,$0,.L8014EA74\n"   /* 8014EC7C: 1660ff7d */
    "\tsll $0,$0,0x0\n"   /* 8014EC80: 00000000 */
    "\tlw $20,4($8)\n"   /* 8014EC84: 8d140004 */
    "\tlw $21,8($8)\n"   /* 8014EC88: 8d150008 */
    "\tlw $22,12($8)\n"   /* 8014EC8C: 8d16000c */
    "\tsubu $9,$9,$20\n"   /* 8014EC90: 01344823 */
    "\tsubu $10,$10,$21\n"   /* 8014EC94: 01555023 */
    "\tsubu $11,$11,$22\n"   /* 8014EC98: 01765823 */
    "\tlhu $19,2($8)\n"   /* 8014EC9C: 95130002 */
    "\tlhu $20,16($8)\n"   /* 8014ECA0: 95140010 */
    "\tbeq $19,$0,.L8014ECB4\n"   /* 8014ECA4: 12600003 */
    "\taddiu $22,$8,20\n"   /* 8014ECA8: 25160014 */
    "\tsll $19,$19,0x10\n"   /* 8014ECAC: 00139c00 */
    "\tor $13,$19,$20\n"   /* 8014ECB0: 02746825 */
    ".L8014ECB4:\n"
    "\tblez $18,.L8014EA74\n"   /* 8014ECB4: 1a40ff6f */
    "\tlw $4,0($22)\n"   /* 8014ECB8: 8ec40000 */
    "\taddiu $18,$18,-1\n"   /* 8014ECBC: 2652ffff */
    "\tjal hdpWalkPush\n"   /* 8014ECC0: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014ECC4: 00000000 */
    "\tj .L8014ECB4\n"   /* 8014ECC8: 08053b2d */
    "\taddiu $22,$22,4\n"   /* 8014ECCC: 26d60004 */
    ".globl hdpKind5\n"
    "hdpKind5:\n"
    "\tlbu $17,39($8)\n"   /* 8014ECD0: 91110027 */
    "\tlbu $18,38($8)\n"   /* 8014ECD4: 91120026 */
    "\tandi $19,$17,0x1\n"   /* 8014ECD8: 32330001 */
    "\tbne $19,$0,.L8014EA74\n"   /* 8014ECDC: 1660ff65 */
    "\tsll $0,$0,0x0\n"   /* 8014ECE0: 00000000 */
    "\tlw $20,24($8)\n"   /* 8014ECE4: 8d140018 */
    "\tlw $21,28($8)\n"   /* 8014ECE8: 8d15001c */
    "\tlw $22,32($8)\n"   /* 8014ECEC: 8d160020 */
    "\tsubu $4,$9,$20\n"   /* 8014ECF0: 01342023 */
    "\tsubu $5,$10,$21\n"   /* 8014ECF4: 01552823 */
    "\tsubu $6,$11,$22\n"   /* 8014ECF8: 01763023 */
    "\taddiu $20,$8,4\n"   /* 8014ECFC: 25140004 */
    "\tlw $19,0($20)\n"   /* 8014ED00: 8e930000 */
    "\tlw $21,4($20)\n"   /* 8014ED04: 8e950004 */
    "\tlw $22,8($20)\n"   /* 8014ED08: 8e960008 */
    "\tlw $23,12($20)\n"   /* 8014ED0C: 8e97000c */
    "\tlw $2,16($20)\n"   /* 8014ED10: 8e820010 */
    "\tctc2 $19,$0\n"   /* 8014ED14: 48d30000 */
    "\tctc2 $21,$1\n"   /* 8014ED18: 48d50800 */
    "\tctc2 $22,$2\n"   /* 8014ED1C: 48d61000 */
    "\tctc2 $23,$3\n"   /* 8014ED20: 48d71800 */
    "\tctc2 $2,$4\n"   /* 8014ED24: 48c22000 */
    "\tjal hdpRotVecSplit\n"   /* 8014ED28: 0c053be6 */
    "\taddiu $23,$8,4\n"   /* 8014ED2C: 25170004 */
    "\taddu $9,$2,$0\n"   /* 8014ED30: 00404821 */
    "\taddu $10,$3,$0\n"   /* 8014ED34: 00605021 */
    "\taddu $11,$7,$0\n"   /* 8014ED38: 00e05821 */
    "\tlw $19,32($16)\n"   /* 8014ED3C: 8e130020 */
    "\tandi $22,$12,0xffff\n"   /* 8014ED40: 3196ffff */
    "\tsll $21,$22,0x2\n"   /* 8014ED44: 0016a880 */
    "\taddu $21,$21,$19\n"   /* 8014ED48: 02b3a821 */
    "\tsw $20,0($21)\n"   /* 8014ED4C: aeb40000 */
    "\taddiu $12,$12,1\n"   /* 8014ED50: 258c0001 */
    "\tlhu $19,2($8)\n"   /* 8014ED54: 95130002 */
    "\tlhu $20,36($8)\n"   /* 8014ED58: 95140024 */
    "\tbeq $19,$0,.L8014ED6C\n"   /* 8014ED5C: 12600003 */
    "\taddiu $22,$8,40\n"   /* 8014ED60: 25160028 */
    "\tsll $19,$19,0x10\n"   /* 8014ED64: 00139c00 */
    "\tor $13,$19,$20\n"   /* 8014ED68: 02746825 */
    ".L8014ED6C:\n"
    "\tblez $18,.L8014EA74\n"   /* 8014ED6C: 1a40ff41 */
    "\tlw $4,0($22)\n"   /* 8014ED70: 8ec40000 */
    "\taddiu $18,$18,-1\n"   /* 8014ED74: 2652ffff */
    "\tjal hdpWalkPush\n"   /* 8014ED78: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014ED7C: 00000000 */
    "\tj .L8014ED6C\n"   /* 8014ED80: 08053b5b */
    "\taddiu $22,$22,4\n"   /* 8014ED84: 26d60004 */
    ".globl hdpKind8\n"
    "hdpKind8:\n"
    "\tlbu $17,7($8)\n"   /* 8014ED88: 91110007 */
    "\tlb $18,5($8)\n"   /* 8014ED8C: 81120005 */
    "\tori $20,$0,0x1\n"   /* 8014ED90: 34140001 */
    "\tbeq $17,$20,.L8014EA74\n"   /* 8014ED94: 1234ff37 */
    "\taddi $19,$0,-2\n"   /* 8014ED98: 2013fffe */
    "\tlw $20,196($16)\n"   /* 8014ED9C: 8e1400c4 */
    "\tbne $19,$18,.L8014EDD0\n"   /* 8014EDA0: 1672000b */
    "\tsll $0,$0,0x0\n"   /* 8014EDA4: 00000000 */
    "\tbne $20,$0,.L8014EDC0\n"   /* 8014EDA8: 16800005 */
    "\tsll $0,$0,0x0\n"   /* 8014EDAC: 00000000 */
    "\tjal hdpLeafTestPointsMark\n"   /* 8014EDB0: 0c053dd4 */
    "\tsll $0,$0,0x0\n"   /* 8014EDB4: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EDB8: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EDBC: 00000000 */
    ".L8014EDC0:\n"
    "\tjal hdpLeafTestSingleBox\n"   /* 8014EDC0: 0c053e32 */
    "\tsll $0,$0,0x0\n"   /* 8014EDC4: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EDC8: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EDCC: 00000000 */
    ".L8014EDD0:\n"
    "\tjal hdpLeafTestPoints\n"   /* 8014EDD0: 0c053c5a */
    "\tsll $0,$0,0x0\n"   /* 8014EDD4: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EDD8: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EDDC: 00000000 */
    ".globl hdpKind9\n"
    "hdpKind9:\n"
    "\tlhu $19,2($8)\n"   /* 8014EDE0: 95130002 */
    "\tlhu $20,4($8)\n"   /* 8014EDE4: 95140004 */
    "\tlbu $23,7($8)\n"   /* 8014EDE8: 91170007 */
    "\tbeq $19,$0,.L8014EDFC\n"   /* 8014EDEC: 12600003 */
    "\taddiu $22,$8,16\n"   /* 8014EDF0: 25160010 */
    "\tsll $19,$19,0x10\n"   /* 8014EDF4: 00139c00 */
    "\tor $13,$19,$20\n"   /* 8014EDF8: 02746825 */
    ".L8014EDFC:\n"
    "\tsll $23,$23,0x2\n"   /* 8014EDFC: 0017b880 */
    "\taddu $22,$22,$23\n"   /* 8014EE00: 02d7b021 */
    "\tlw $4,0($22)\n"   /* 8014EE04: 8ec40000 */
    "\tjal hdpWalkPush\n"   /* 8014EE08: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014EE0C: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EE10: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EE14: 00000000 */
    ".globl hdpKind3\n"
    "hdpKind3:\n"
    "\tlhu $19,2($8)\n"   /* 8014EE18: 95130002 */
    "\tlhu $20,4($8)\n"   /* 8014EE1C: 95140004 */
    "\tlbu $23,12($8)\n"   /* 8014EE20: 9117000c */
    "\tbeq $19,$0,.L8014EE34\n"   /* 8014EE24: 12600003 */
    "\taddiu $22,$8,32\n"   /* 8014EE28: 25160020 */
    "\tsll $19,$19,0x10\n"   /* 8014EE2C: 00139c00 */
    "\tor $13,$19,$20\n"   /* 8014EE30: 02746825 */
    ".L8014EE34:\n"
    "\tsll $23,$23,0x2\n"   /* 8014EE34: 0017b880 */
    "\taddu $22,$22,$23\n"   /* 8014EE38: 02d7b021 */
    "\tlw $4,0($22)\n"   /* 8014EE3C: 8ec40000 */
    "\tjal hdpWalkPush\n"   /* 8014EE40: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014EE44: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EE48: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EE4C: 00000000 */
    ".globl hdpKind11\n"
    "hdpKind11:\n"
    "\tlhu $21,2($8)\n"   /* 8014EE50: 95150002 */
    "\tlbu $17,4($8)\n"   /* 8014EE54: 91110004 */
    "\tlbu $20,5($8)\n"   /* 8014EE58: 91140005 */
    "\tandi $19,$17,0x1\n"   /* 8014EE5C: 32330001 */
    "\tbne $19,$0,.L8014EA74\n"   /* 8014EE60: 1660ff04 */
    "\tsll $0,$0,0x0\n"   /* 8014EE64: 00000000 */
    "\tbeq $21,$0,.L8014EE74\n"   /* 8014EE68: 12a00002 */
    "\tsll $21,$21,0x10\n"   /* 8014EE6C: 0015ac00 */
    "\tor $13,$21,$20\n"   /* 8014EE70: 02b46825 */
    ".L8014EE74:\n"
    "\taddiu $18,$8,16\n"   /* 8014EE74: 25120010 */
    "\tlbu $25,6($8)\n"   /* 8014EE78: 91190006 */
    ".L8014EE7C:\n"
    "\tlw $19,0($18)\n"   /* 8014EE7C: 8e530000 */
    "\tjal hdpBoxTestModel\n"   /* 8014EE80: 0c053e72 */
    "\tsll $0,$0,0x0\n"   /* 8014EE84: 00000000 */
    "\tbne $17,$0,.L8014EEA4\n"   /* 8014EE88: 16200006 */
    "\tlw $4,12($8)\n"   /* 8014EE8C: 8d04000c */
    "\taddiu $25,$25,-1\n"   /* 8014EE90: 2739ffff */
    "\taddiu $18,$18,4\n"   /* 8014EE94: 26520004 */
    "\tbgtz $25,.L8014EE7C\n"   /* 8014EE98: 1f20fff8 */
    "\tsll $0,$0,0x0\n"   /* 8014EE9C: 00000000 */
    "\tlw $4,8($8)\n"   /* 8014EEA0: 8d040008 */
    ".L8014EEA4:\n"
    "\tsll $0,$0,0x0\n"   /* 8014EEA4: 00000000 */
    "\tbeq $4,$0,.L8014EA74\n"   /* 8014EEA8: 1080fef2 */
    "\tsll $0,$0,0x0\n"   /* 8014EEAC: 00000000 */
    "\tjal hdpWalkPush\n"   /* 8014EEB0: 0c053bcf */
    "\tsll $0,$0,0x0\n"   /* 8014EEB4: 00000000 */
    "\tj .L8014EA74\n"   /* 8014EEB8: 08053a9d */
    "\tsll $0,$0,0x0\n"   /* 8014EEBC: 00000000 */
    ".globl hdpKind12\n"
    "hdpKind12:\n"
    "\tlhu $18,4($8)\n"   /* 8014EEC0: 95120004 */
    "\tsrl $19,$12,0x10\n"   /* 8014EEC4: 000c9c02 */
    "\tsubu $20,$19,$18\n"   /* 8014EEC8: 0272a023 */
    "\tbgez $20,.L8014EEEC\n"   /* 8014EECC: 06810007 */
    "\tsll $0,$0,0x0\n"   /* 8014EED0: 00000000 */
    "\tnegu $20,$20\n"   /* 8014EED4: 0014a023 */
    "\tsllv $9,$9,$20\n"   /* 8014EED8: 02894804 */
    "\tsllv $10,$10,$20\n"   /* 8014EEDC: 028a5004 */
    "\tsllv $11,$11,$20\n"   /* 8014EEE0: 028b5804 */
    "\tj .L8014EF00\n"   /* 8014EEE4: 08053bc0 */
    "\tsll $0,$0,0x0\n"   /* 8014EEE8: 00000000 */
    ".L8014EEEC:\n"
    "\tblez $20,.L8014EF00\n"   /* 8014EEEC: 1a800004 */
    "\tsll $0,$0,0x0\n"   /* 8014EEF0: 00000000 */
    "\tsrav $9,$9,$20\n"   /* 8014EEF4: 02894807 */
    "\tsrav $10,$10,$20\n"   /* 8014EEF8: 028a5007 */
    "\tsrav $11,$11,$20\n"   /* 8014EEFC: 028b5807 */
    ".L8014EF00:\n"
    "\tsll $18,$18,0x10\n"   /* 8014EF00: 00129400 */
    "\tandi $12,$12,0xffff\n"   /* 8014EF04: 318cffff */
    "\tor $12,$12,$18\n"   /* 8014EF08: 01926025 */
    "\tlhu $18,6($8)\n"   /* 8014EF0C: 95120006 */
    "\taddiu $19,$8,8\n"   /* 8014EF10: 25130008 */
    ".L8014EF14:\n"
    "\tblez $18,.L8014EA74\n"   /* 8014EF14: 1a40fed7 */
    "\tlw $4,0($19)\n"   /* 8014EF18: 8e640000 */
    "\tjal hdpWalkPush\n"   /* 8014EF1C: 0c053bcf */
    "\taddiu $18,$18,-1\n"   /* 8014EF20: 2652ffff */
    "\tj .L8014EF14\n"   /* 8014EF24: 08053bc5 */
    "\taddiu $19,$19,4\n"   /* 8014EF28: 26730004 */
    ".L8014EF2C:\n"
    "\tlw $31,204($16)\n"   /* 8014EF2C: 8e1f00cc */
    "\tsll $0,$0,0x0\n"   /* 8014EF30: 00000000 */
    "\tjr $31\n"   /* 8014EF34: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014EF38: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpWalk,.-hdpWalk\n");

/* 8014EF3C (hand-written asm): walker push: entry t0..t5 onto the stack (first 33 in the scratchpad
 * at t6, the rest in the st->f00 buffer at t7; depth in t8, capped at 200). */
__asm__(
    ".section .text_8014EF3C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpWalkPush\n"
    ".type hdpWalkPush,@function\n"
    "hdpWalkPush:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsltiu $3,$24,33\n"   /* 8014EF3C: 2f030021 */
    "\tbne $3,$0,.L8014EF78\n"   /* 8014EF40: 1460000d */
    "\tsw $4,0($14)\n"   /* 8014EF44: adc40000 */
    "\tsw $4,0($15)\n"   /* 8014EF48: ade40000 */
    "\tsw $9,4($15)\n"   /* 8014EF4C: ade90004 */
    "\tsw $10,8($15)\n"   /* 8014EF50: adea0008 */
    "\tsw $11,12($15)\n"   /* 8014EF54: adeb000c */
    "\tsw $12,16($15)\n"   /* 8014EF58: adec0010 */
    "\tsw $13,20($15)\n"   /* 8014EF5C: aded0014 */
    "\tsltiu $3,$24,200\n"   /* 8014EF60: 2f0300c8 */
    "\tbne $3,$0,.L8014EF90\n"   /* 8014EF64: 1460000a */
    "\taddiu $15,$15,24\n"   /* 8014EF68: 25ef0018 */
    "\taddiu $15,$15,-24\n"   /* 8014EF6C: 25efffe8 */
    "\tj .L8014EF90\n"   /* 8014EF70: 08053be4 */
    "\taddiu $24,$24,-1\n"   /* 8014EF74: 2718ffff */
    ".L8014EF78:\n"
    "\tsw $9,4($14)\n"   /* 8014EF78: adc90004 */
    "\tsw $10,8($14)\n"   /* 8014EF7C: adca0008 */
    "\tsw $11,12($14)\n"   /* 8014EF80: adcb000c */
    "\tsw $12,16($14)\n"   /* 8014EF84: adcc0010 */
    "\tsw $13,20($14)\n"   /* 8014EF88: adcd0014 */
    "\taddiu $14,$14,24\n"   /* 8014EF8C: 25ce0018 */
    ".L8014EF90:\n"
    "\tjr $31\n"   /* 8014EF90: 03e00008 */
    "\taddiu $24,$24,1\n"   /* 8014EF94: 27180001 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpWalkPush,.-hdpWalkPush\n");

/* 8014EF98 (hand-written asm): rotate the vector a0..a2 by the GTE rotation matrix in split 15-bit
 * precision; result in v0/v1/a3 (gp/sp/s8 reloaded from s7). */
__asm__(
    ".section .text_8014EF98,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpRotVecSplit\n"
    ".type hdpRotVecSplit,@function\n"
    "hdpRotVecSplit:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $4,36($16)\n"   /* 8014EF98: ae040024 */
    "\tsw $5,40($16)\n"   /* 8014EF9C: ae050028 */
    "\tsw $6,44($16)\n"   /* 8014EFA0: ae06002c */
    "\tbgez $4,.L8014EFC4\n"   /* 8014EFA4: 04810007 */
    "\tsra $2,$4,0xf\n"   /* 8014EFA8: 000413c3 */
    "\tnegu $4,$4\n"   /* 8014EFAC: 00042023 */
    "\tsra $2,$4,0xf\n"   /* 8014EFB0: 000413c3 */
    "\tandi $4,$4,0x7fff\n"   /* 8014EFB4: 30847fff */
    "\tnegu $2,$2\n"   /* 8014EFB8: 00021023 */
    "\tj .L8014EFC8\n"   /* 8014EFBC: 08053bf2 */
    "\tnegu $4,$4\n"   /* 8014EFC0: 00042023 */
    ".L8014EFC4:\n"
    "\tandi $4,$4,0x7fff\n"   /* 8014EFC4: 30847fff */
    ".L8014EFC8:\n"
    "\tbgez $5,.L8014EFE8\n"   /* 8014EFC8: 04a10007 */
    "\tsra $3,$5,0xf\n"   /* 8014EFCC: 00051bc3 */
    "\tnegu $5,$5\n"   /* 8014EFD0: 00052823 */
    "\tsra $3,$5,0xf\n"   /* 8014EFD4: 00051bc3 */
    "\tandi $5,$5,0x7fff\n"   /* 8014EFD8: 30a57fff */
    "\tnegu $3,$3\n"   /* 8014EFDC: 00031823 */
    "\tj .L8014EFEC\n"   /* 8014EFE0: 08053bfb */
    "\tnegu $5,$5\n"   /* 8014EFE4: 00052823 */
    ".L8014EFE8:\n"
    "\tandi $5,$5,0x7fff\n"   /* 8014EFE8: 30a57fff */
    ".L8014EFEC:\n"
    "\tbgez $6,.L8014F00C\n"   /* 8014EFEC: 04c10007 */
    "\tsra $7,$6,0xf\n"   /* 8014EFF0: 00063bc3 */
    "\tnegu $6,$6\n"   /* 8014EFF4: 00063023 */
    "\tsra $7,$6,0xf\n"   /* 8014EFF8: 00063bc3 */
    "\tandi $6,$6,0x7fff\n"   /* 8014EFFC: 30c67fff */
    "\tnegu $7,$7\n"   /* 8014F000: 00073823 */
    "\tj .L8014F010\n"   /* 8014F004: 08053c04 */
    "\tnegu $6,$6\n"   /* 8014F008: 00063023 */
    ".L8014F00C:\n"
    "\tandi $6,$6,0x7fff\n"   /* 8014F00C: 30c67fff */
    ".L8014F010:\n"
    "\tmtc2 $2,$9\n"   /* 8014F010: 48824800 */
    "\tmtc2 $3,$10\n"   /* 8014F014: 48835000 */
    "\tmtc2 $7,$11\n"   /* 8014F018: 48875800 */
    "\tsll $0,$0,0x0\n"   /* 8014F01C: 00000000 */
    "\tcop2 0x41e012\n"   /* 8014F020: 4a41e012 */
    "\tlw $28,0($23)\n"   /* 8014F024: 8efc0000 */
    "\tlw $29,4($23)\n"   /* 8014F028: 8efd0004 */
    "\tlw $30,8($23)\n"   /* 8014F02C: 8efe0008 */
    "\tmfc2 $2,$25\n"   /* 8014F030: 4802c800 */
    "\tmfc2 $3,$26\n"   /* 8014F034: 4803d000 */
    "\tmfc2 $7,$27\n"   /* 8014F038: 4807d800 */
    "\tmtc2 $4,$9\n"   /* 8014F03C: 48844800 */
    "\tmtc2 $5,$10\n"   /* 8014F040: 48855000 */
    "\tmtc2 $6,$11\n"   /* 8014F044: 48865800 */
    "\tsll $0,$0,0x0\n"   /* 8014F048: 00000000 */
    "\tcop2 0x49e012\n"   /* 8014F04C: 4a49e012 */
    "\tbgez $2,.L8014F068\n"   /* 8014F050: 04410005 */
    "\tsll $0,$0,0x0\n"   /* 8014F054: 00000000 */
    "\tnegu $2,$2\n"   /* 8014F058: 00021023 */
    "\tsll $2,$2,0x3\n"   /* 8014F05C: 000210c0 */
    "\tj .L8014F06C\n"   /* 8014F060: 08053c1b */
    "\tnegu $2,$2\n"   /* 8014F064: 00021023 */
    ".L8014F068:\n"
    "\tsll $2,$2,0x3\n"   /* 8014F068: 000210c0 */
    ".L8014F06C:\n"
    "\tbgez $3,.L8014F084\n"   /* 8014F06C: 04610005 */
    "\tsll $0,$0,0x0\n"   /* 8014F070: 00000000 */
    "\tnegu $3,$3\n"   /* 8014F074: 00031823 */
    "\tsll $3,$3,0x3\n"   /* 8014F078: 000318c0 */
    "\tj .L8014F088\n"   /* 8014F07C: 08053c22 */
    "\tnegu $3,$3\n"   /* 8014F080: 00031823 */
    ".L8014F084:\n"
    "\tsll $3,$3,0x3\n"   /* 8014F084: 000318c0 */
    ".L8014F088:\n"
    "\tbgez $7,.L8014F0A0\n"   /* 8014F088: 04e10005 */
    "\tsll $0,$0,0x0\n"   /* 8014F08C: 00000000 */
    "\tnegu $7,$7\n"   /* 8014F090: 00073823 */
    "\tsll $7,$7,0x3\n"   /* 8014F094: 000738c0 */
    "\tj .L8014F0A4\n"   /* 8014F098: 08053c29 */
    "\tnegu $7,$7\n"   /* 8014F09C: 00073823 */
    ".L8014F0A0:\n"
    "\tsll $7,$7,0x3\n"   /* 8014F0A0: 000738c0 */
    ".L8014F0A4:\n"
    "\tmfc2 $4,$25\n"   /* 8014F0A4: 4804c800 */
    "\tmfc2 $5,$26\n"   /* 8014F0A8: 4805d000 */
    "\tmfc2 $6,$27\n"   /* 8014F0AC: 4806d800 */
    "\taddu $2,$4,$2\n"   /* 8014F0B0: 00821021 */
    "\taddu $3,$5,$3\n"   /* 8014F0B4: 00a31821 */
    "\taddu $7,$6,$7\n"   /* 8014F0B8: 00c73821 */
    "\tlw $4,36($16)\n"   /* 8014F0BC: 8e040024 */
    "\tlw $5,40($16)\n"   /* 8014F0C0: 8e050028 */
    "\tjr $31\n"   /* 8014F0C4: 03e00008 */
    "\tlw $6,44($16)\n"   /* 8014F0C8: 8e06002c */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpRotVecSplit,.-hdpRotVecSplit\n");

/* 8014F0CC (hand-written asm): load the rotation matrix at a0 into the GTE and transform the short
 * vector at a1; result in v0/v1/a3. */
__asm__(
    ".section .text_8014F0CC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpRotTransSVector\n"
    ".type hdpRotTransSVector,@function\n"
    "hdpRotTransSVector:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlh $2,6($4)\n"   /* 8014F0CC: 84820006 */
    "\tlh $3,0($4)\n"   /* 8014F0D0: 84830000 */
    "\tsll $2,$2,0x10\n"   /* 8014F0D4: 00021400 */
    "\tandi $3,$3,0xffff\n"   /* 8014F0D8: 3063ffff */
    "\tor $3,$3,$2\n"   /* 8014F0DC: 00621825 */
    "\tctc2 $3,$0\n"   /* 8014F0E0: 48c30000 */
    "\tlh $2,2($4)\n"   /* 8014F0E4: 84820002 */
    "\tlh $3,12($4)\n"   /* 8014F0E8: 8483000c */
    "\tsll $2,$2,0x10\n"   /* 8014F0EC: 00021400 */
    "\tandi $3,$3,0xffff\n"   /* 8014F0F0: 3063ffff */
    "\tor $3,$3,$2\n"   /* 8014F0F4: 00621825 */
    "\tctc2 $3,$1\n"   /* 8014F0F8: 48c30800 */
    "\tlh $2,14($4)\n"   /* 8014F0FC: 8482000e */
    "\tlh $3,8($4)\n"   /* 8014F100: 84830008 */
    "\tsll $2,$2,0x10\n"   /* 8014F104: 00021400 */
    "\tandi $3,$3,0xffff\n"   /* 8014F108: 3063ffff */
    "\tor $3,$3,$2\n"   /* 8014F10C: 00621825 */
    "\tctc2 $3,$2\n"   /* 8014F110: 48c31000 */
    "\tlh $2,10($4)\n"   /* 8014F114: 8482000a */
    "\tlh $3,4($4)\n"   /* 8014F118: 84830004 */
    "\tsll $2,$2,0x10\n"   /* 8014F11C: 00021400 */
    "\tandi $3,$3,0xffff\n"   /* 8014F120: 3063ffff */
    "\tor $3,$3,$2\n"   /* 8014F124: 00621825 */
    "\tctc2 $3,$3\n"   /* 8014F128: 48c31800 */
    "\tlw $2,16($4)\n"   /* 8014F12C: 8c820010 */
    "\tsll $0,$0,0x0\n"   /* 8014F130: 00000000 */
    "\tctc2 $2,$4\n"   /* 8014F134: 48c22000 */
    "\tlwc2 $0,0($5)\n"   /* 8014F138: c8a00000 */
    "\tlwc2 $1,4($5)\n"   /* 8014F13C: c8a10004 */
    "\tsll $0,$0,0x0\n"   /* 8014F140: 00000000 */
    "\tcop2 0x486012\n"   /* 8014F144: 4a486012 */
    "\tlw $28,0($23)\n"   /* 8014F148: 8efc0000 */
    "\tlw $29,4($23)\n"   /* 8014F14C: 8efd0004 */
    "\tlw $30,8($23)\n"   /* 8014F150: 8efe0008 */
    "\tmfc2 $2,$25\n"   /* 8014F154: 4802c800 */
    "\tmfc2 $3,$26\n"   /* 8014F158: 4803d000 */
    "\tmfc2 $7,$27\n"   /* 8014F15C: 4807d800 */
    "\tjr $31\n"   /* 8014F160: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014F164: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpRotTransSVector,.-hdpRotTransSVector\n");

/* 8014F168 (hand-written asm): kind 8 leaf test for the hit points of the current object against a
 * box (with LOD shift), recording hits in the per-point records. */
__asm__(
    ".section .text_8014F168,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpLeafTestPoints\n"
    ".type hdpLeafTestPoints,@function\n"
    "hdpLeafTestPoints:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $31,200($16)\n"   /* 8014F168: ae1f00c8 */
    "\tlw $17,16($16)\n"   /* 8014F16C: 8e110010 */
    "\tlw $18,24($16)\n"   /* 8014F170: 8e120018 */
    "\tlw $25,20($16)\n"   /* 8014F174: 8e190014 */
    ".L8014F178:\n"
    "\tblez $17,.L8014F6A8\n"   /* 8014F178: 1a20014b */
    "\tandi $21,$12,0xffff\n"   /* 8014F17C: 3195ffff */
    "\tlw $19,0($18)\n"   /* 8014F180: 8e530000 */
    "\tlw $20,8($16)\n"   /* 8014F184: 8e140008 */
    "\tbne $19,$0,.L8014F698\n"   /* 8014F188: 16600143 */
    "\taddiu $4,$20,128\n"   /* 8014F18C: 26840080 */
    "\taddu $5,$25,$0\n"   /* 8014F190: 03202821 */
    "\tjal hdpRotTransSVector\n"   /* 8014F194: 0c053c33 */
    "\taddiu $23,$20,168\n"   /* 8014F198: 269700a8 */
    "\taddu $28,$2,$28\n"   /* 8014F19C: 005ce021 */
    "\taddu $29,$3,$29\n"   /* 8014F1A0: 007de821 */
    "\taddu $30,$7,$30\n"   /* 8014F1A4: 00fef021 */
    "\tsw $28,48($16)\n"   /* 8014F1A8: ae1c0030 */
    "\tsw $29,52($16)\n"   /* 8014F1AC: ae1d0034 */
    "\tsw $30,56($16)\n"   /* 8014F1B0: ae1e0038 */
    "\tlw $22,32($16)\n"   /* 8014F1B4: 8e160020 */
    "\tjal hdpApplyParentTransforms\n"   /* 8014F1B8: 0c053dae */
    "\taddiu $23,$16,28\n"   /* 8014F1BC: 2617001c */
    "\taddu $4,$2,$9\n"   /* 8014F1C0: 00492021 */
    "\taddu $5,$3,$10\n"   /* 8014F1C4: 006a2821 */
    "\taddu $6,$7,$11\n"   /* 8014F1C8: 00eb3021 */
    "\tsw $4,60($16)\n"   /* 8014F1CC: ae04003c */
    "\tsw $5,64($16)\n"   /* 8014F1D0: ae050040 */
    "\tsw $6,68($16)\n"   /* 8014F1D4: ae060044 */
    "\taddiu $20,$8,16\n"   /* 8014F1D8: 25140010 */
    "\tlw $22,0($20)\n"   /* 8014F1DC: 8e960000 */
    "\tlw $23,4($20)\n"   /* 8014F1E0: 8e970004 */
    "\tlw $2,8($20)\n"   /* 8014F1E4: 8e820008 */
    "\tlw $3,12($20)\n"   /* 8014F1E8: 8e83000c */
    "\tlw $7,16($20)\n"   /* 8014F1EC: 8e870010 */
    "\tctc2 $22,$0\n"   /* 8014F1F0: 48d60000 */
    "\tctc2 $23,$1\n"   /* 8014F1F4: 48d70800 */
    "\tctc2 $2,$2\n"   /* 8014F1F8: 48c21000 */
    "\tctc2 $3,$3\n"   /* 8014F1FC: 48c31800 */
    "\tctc2 $7,$4\n"   /* 8014F200: 48c72000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F204: 0c053be6 */
    "\taddiu $23,$8,36\n"   /* 8014F208: 25170024 */
    "\taddu $20,$2,$28\n"   /* 8014F20C: 005ca021 */
    "\tsw $20,72($16)\n"   /* 8014F210: ae140048 */
    "\tblez $20,.L8014F698\n"   /* 8014F214: 1a800120 */
    "\taddu $21,$3,$29\n"   /* 8014F218: 007da821 */
    "\tblez $21,.L8014F698\n"   /* 8014F21C: 1aa0011e */
    "\taddu $22,$7,$30\n"   /* 8014F220: 00feb021 */
    "\tblez $22,.L8014F698\n"   /* 8014F224: 1ac0011c */
    "\tsll $0,$0,0x0\n"   /* 8014F228: 00000000 */
    "\tlbu $20,4($8)\n"   /* 8014F22C: 91140004 */
    "\tsw $21,76($16)\n"   /* 8014F230: ae15004c */
    "\tsw $22,80($16)\n"   /* 8014F234: ae160050 */
    "\tsltiu $21,$20,4\n"   /* 8014F238: 2e950004 */
    "\tbne $21,$0,.L8014F2AC\n"   /* 8014F23C: 16a0001b */
    "\taddiu $23,$8,48\n"   /* 8014F240: 25170030 */
    "\tlw $2,0($23)\n"   /* 8014F244: 8ee20000 */
    "\tlw $3,4($23)\n"   /* 8014F248: 8ee30004 */
    "\tlw $7,8($23)\n"   /* 8014F24C: 8ee70008 */
    "\tlw $29,12($23)\n"   /* 8014F250: 8efd000c */
    "\tlw $28,16($23)\n"   /* 8014F254: 8efc0010 */
    "\tctc2 $2,$0\n"   /* 8014F258: 48c20000 */
    "\tctc2 $3,$1\n"   /* 8014F25C: 48c30800 */
    "\tctc2 $7,$2\n"   /* 8014F260: 48c71000 */
    "\tctc2 $29,$3\n"   /* 8014F264: 48dd1800 */
    "\tctc2 $28,$4\n"   /* 8014F268: 48dc2000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F26C: 0c053be6 */
    "\taddiu $23,$8,68\n"   /* 8014F270: 25170044 */
    "\taddu $21,$2,$28\n"   /* 8014F274: 005ca821 */
    "\tsw $21,84($16)\n"   /* 8014F278: ae150054 */
    "\tblez $21,.L8014F698\n"   /* 8014F27C: 1aa00106 */
    "\tsltiu $23,$20,5\n"   /* 8014F280: 2e970005 */
    "\tbne $23,$0,.L8014F2AC\n"   /* 8014F284: 16e00009 */
    "\taddu $22,$3,$29\n"   /* 8014F288: 007db021 */
    "\tsw $22,88($16)\n"   /* 8014F28C: ae160058 */
    "\tblez $22,.L8014F698\n"   /* 8014F290: 1ac00101 */
    "\tsltiu $23,$20,6\n"   /* 8014F294: 2e970006 */
    "\tbne $23,$0,.L8014F2AC\n"   /* 8014F298: 16e00004 */
    "\taddu $22,$7,$30\n"   /* 8014F29C: 00feb021 */
    "\tsw $22,92($16)\n"   /* 8014F2A0: ae16005c */
    "\tblez $22,.L8014F698\n"   /* 8014F2A4: 1ac000fc */
    "\tsll $0,$0,0x0\n"   /* 8014F2A8: 00000000 */
    ".L8014F2AC:\n"
    "\tlb $20,5($8)\n"   /* 8014F2AC: 81140005 */
    "\tsll $0,$0,0x0\n"   /* 8014F2B0: 00000000 */
    "\tbltz $20,.L8014F2F0\n"   /* 8014F2B4: 0680000e */
    "\tsll $21,$20,0x1\n"   /* 8014F2B8: 0014a840 */
    "\taddu $21,$21,$20\n"   /* 8014F2BC: 02b4a821 */
    "\tsll $21,$21,0x1\n"   /* 8014F2C0: 0015a840 */
    "\taddu $21,$21,$8\n"   /* 8014F2C4: 02a8a821 */
    "\tslti $22,$20,3\n"   /* 8014F2C8: 2a960003 */
    "\tbeq $22,$0,.L8014F2D8\n"   /* 8014F2CC: 12c00002 */
    "\taddiu $23,$21,30\n"   /* 8014F2D0: 26b7001e */
    "\taddiu $23,$21,16\n"   /* 8014F2D4: 26b70010 */
    ".L8014F2D8:\n"
    "\tsw $23,96($16)\n"   /* 8014F2D8: ae170060 */
    "\taddiu $21,$16,72\n"   /* 8014F2DC: 26150048 */
    "\tsll $22,$20,0x2\n"   /* 8014F2E0: 0014b080 */
    "\taddu $22,$22,$21\n"   /* 8014F2E4: 02d5b021 */
    "\tj .L8014F4A8\n"   /* 8014F2E8: 08053d2a */
    "\tsw $22,100($16)\n"   /* 8014F2EC: ae160064 */
    ".L8014F2F0:\n"
    "\tlw $4,164($16)\n"   /* 8014F2F0: 8e0400a4 */
    "\tlw $23,168($16)\n"   /* 8014F2F4: 8e1700a8 */
    "\tjal hdpRotTransSVector\n"   /* 8014F2F8: 0c053c33 */
    "\taddu $5,$25,$0\n"   /* 8014F2FC: 03202821 */
    "\tlw $20,48($16)\n"   /* 8014F300: 8e140030 */
    "\taddu $28,$2,$28\n"   /* 8014F304: 005ce021 */
    "\tlw $21,52($16)\n"   /* 8014F308: 8e150034 */
    "\taddu $29,$3,$29\n"   /* 8014F30C: 007de821 */
    "\tlw $22,56($16)\n"   /* 8014F310: 8e160038 */
    "\taddu $30,$7,$30\n"   /* 8014F314: 00fef021 */
    "\tsubu $2,$28,$20\n"   /* 8014F318: 03941023 */
    "\tsubu $3,$29,$21\n"   /* 8014F31C: 03b51823 */
    "\tsubu $7,$30,$22\n"   /* 8014F320: 03d63823 */
    "\tandi $21,$12,0xffff\n"   /* 8014F324: 3195ffff */
    "\tlw $22,32($16)\n"   /* 8014F328: 8e160020 */
    "\tjal hdpApplyParentTransforms\n"   /* 8014F32C: 0c053dae */
    "\taddiu $23,$16,28\n"   /* 8014F330: 2617001c */
    "\tlw $20,60($16)\n"   /* 8014F334: 8e14003c */
    "\tlw $21,64($16)\n"   /* 8014F338: 8e150040 */
    "\tlw $22,68($16)\n"   /* 8014F33C: 8e160044 */
    "\taddu $4,$2,$20\n"   /* 8014F340: 00542021 */
    "\taddu $5,$3,$21\n"   /* 8014F344: 00752821 */
    "\taddu $6,$7,$22\n"   /* 8014F348: 00f63021 */
    "\taddiu $20,$8,16\n"   /* 8014F34C: 25140010 */
    "\tlw $22,0($20)\n"   /* 8014F350: 8e960000 */
    "\tlw $23,4($20)\n"   /* 8014F354: 8e970004 */
    "\tlw $2,8($20)\n"   /* 8014F358: 8e820008 */
    "\tlw $3,12($20)\n"   /* 8014F35C: 8e83000c */
    "\tlw $7,16($20)\n"   /* 8014F360: 8e870010 */
    "\tctc2 $22,$0\n"   /* 8014F364: 48d60000 */
    "\tctc2 $23,$1\n"   /* 8014F368: 48d70800 */
    "\tctc2 $2,$2\n"   /* 8014F36C: 48c21000 */
    "\tctc2 $3,$3\n"   /* 8014F370: 48c31800 */
    "\tctc2 $7,$4\n"   /* 8014F374: 48c72000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F378: 0c053be6 */
    "\taddiu $23,$8,36\n"   /* 8014F37C: 25170024 */
    "\taddu $20,$2,$28\n"   /* 8014F380: 005ca021 */
    "\tsw $20,48($16)\n"   /* 8014F384: ae140030 */
    "\taddu $21,$3,$29\n"   /* 8014F388: 007da821 */
    "\tsw $21,52($16)\n"   /* 8014F38C: ae150034 */
    "\tlbu $2,4($8)\n"   /* 8014F390: 91020004 */
    "\taddu $22,$7,$30\n"   /* 8014F394: 00feb021 */
    "\tsw $22,56($16)\n"   /* 8014F398: ae160038 */
    "\tsltiu $2,$2,4\n"   /* 8014F39C: 2c420004 */
    "\tbne $2,$0,.L8014F3F0\n"   /* 8014F3A0: 14400013 */
    "\taddiu $23,$8,48\n"   /* 8014F3A4: 25170030 */
    "\tlw $2,0($23)\n"   /* 8014F3A8: 8ee20000 */
    "\tlw $3,4($23)\n"   /* 8014F3AC: 8ee30004 */
    "\tlw $7,8($23)\n"   /* 8014F3B0: 8ee70008 */
    "\tlw $29,12($23)\n"   /* 8014F3B4: 8efd000c */
    "\tlw $28,16($23)\n"   /* 8014F3B8: 8efc0010 */
    "\tctc2 $2,$0\n"   /* 8014F3BC: 48c20000 */
    "\tctc2 $3,$1\n"   /* 8014F3C0: 48c30800 */
    "\tctc2 $7,$2\n"   /* 8014F3C4: 48c71000 */
    "\tctc2 $29,$3\n"   /* 8014F3C8: 48dd1800 */
    "\tctc2 $28,$4\n"   /* 8014F3CC: 48dc2000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F3D0: 0c053be6 */
    "\taddiu $23,$8,68\n"   /* 8014F3D4: 25170044 */
    "\taddu $2,$2,$28\n"   /* 8014F3D8: 005c1021 */
    "\tsw $2,60($16)\n"   /* 8014F3DC: ae02003c */
    "\taddu $3,$3,$29\n"   /* 8014F3E0: 007d1821 */
    "\tsw $3,64($16)\n"   /* 8014F3E4: ae030040 */
    "\taddu $7,$7,$30\n"   /* 8014F3E8: 00fe3821 */
    "\tsw $7,68($16)\n"   /* 8014F3EC: ae070044 */
    ".L8014F3F0:\n"
    "\tlbu $23,4($8)\n"   /* 8014F3F0: 91170004 */
    "\taddiu $3,$0,-1\n"   /* 8014F3F4: 2403ffff */
    "\taddu $2,$23,$0\n"   /* 8014F3F8: 02e01021 */
    "\taddiu $22,$16,48\n"   /* 8014F3FC: 26160030 */
    "\tlui $5,0x7fff\n"   /* 8014F400: 3c057fff */
    "\taddiu $4,$0,-1\n"   /* 8014F404: 2404ffff */
    ".L8014F408:\n"
    "\taddiu $3,$3,1\n"   /* 8014F408: 24630001 */
    "\tblez $23,.L8014F43C\n"   /* 8014F40C: 1ae0000b */
    "\tlw $20,0($22)\n"   /* 8014F410: 8ed40000 */
    "\tlw $21,24($22)\n"   /* 8014F414: 8ed50018 */
    "\taddiu $22,$22,4\n"   /* 8014F418: 26d60004 */
    "\tbgtz $20,.L8014F408\n"   /* 8014F41C: 1e80fffa */
    "\taddiu $23,$23,-1\n"   /* 8014F420: 26f7ffff */
    "\tslt $6,$21,$5\n"   /* 8014F424: 02a5302a */
    "\tbeq $6,$0,.L8014F408\n"   /* 8014F428: 10c0fff7 */
    "\tsll $0,$0,0x0\n"   /* 8014F42C: 00000000 */
    "\taddu $5,$21,$0\n"   /* 8014F430: 02a02821 */
    "\tj .L8014F408\n"   /* 8014F434: 08053d02 */
    "\taddu $4,$3,$0\n"   /* 8014F438: 00602021 */
    ".L8014F43C:\n"
    "\tbgez $4,.L8014F474\n"   /* 8014F43C: 0481000d */
    "\tlui $5,0x7fff\n"   /* 8014F440: 3c057fff */
    "\taddiu $3,$0,-1\n"   /* 8014F444: 2403ffff */
    "\taddiu $22,$16,48\n"   /* 8014F448: 26160030 */
    ".L8014F44C:\n"
    "\taddiu $3,$3,1\n"   /* 8014F44C: 24630001 */
    "\tblez $2,.L8014F474\n"   /* 8014F450: 18400008 */
    "\tlw $20,0($22)\n"   /* 8014F454: 8ed40000 */
    "\taddiu $22,$22,4\n"   /* 8014F458: 26d60004 */
    "\tslt $6,$20,$5\n"   /* 8014F45C: 0285302a */
    "\tbeq $6,$0,.L8014F44C\n"   /* 8014F460: 10c0fffa */
    "\taddiu $2,$2,-1\n"   /* 8014F464: 2442ffff */
    "\taddu $5,$20,$0\n"   /* 8014F468: 02802821 */
    "\tj .L8014F44C\n"   /* 8014F46C: 08053d13 */
    "\taddu $4,$3,$0\n"   /* 8014F470: 00602021 */
    ".L8014F474:\n"
    "\tsll $21,$4,0x1\n"   /* 8014F474: 0004a840 */
    "\taddu $21,$21,$4\n"   /* 8014F478: 02a4a821 */
    "\tsll $21,$21,0x1\n"   /* 8014F47C: 0015a840 */
    "\taddu $21,$21,$8\n"   /* 8014F480: 02a8a821 */
    "\tslti $22,$4,3\n"   /* 8014F484: 28960003 */
    "\tbeq $22,$0,.L8014F494\n"   /* 8014F488: 12c00002 */
    "\taddiu $23,$21,30\n"   /* 8014F48C: 26b7001e */
    "\taddiu $23,$21,16\n"   /* 8014F490: 26b70010 */
    ".L8014F494:\n"
    "\tsw $23,96($16)\n"   /* 8014F494: ae170060 */
    "\taddiu $21,$16,72\n"   /* 8014F498: 26150048 */
    "\tsll $22,$4,0x2\n"   /* 8014F49C: 0004b080 */
    "\taddu $22,$22,$21\n"   /* 8014F4A0: 02d5b021 */
    "\tsw $22,100($16)\n"   /* 8014F4A4: ae160064 */
    ".L8014F4A8:\n"
    "\tlw $19,100($16)\n"   /* 8014F4A8: 8e130064 */
    "\tlw $20,28($16)\n"   /* 8014F4AC: 8e14001c */
    "\tsrl $21,$12,0x10\n"   /* 8014F4B0: 000cac02 */
    "\tsubu $21,$21,$20\n"   /* 8014F4B4: 02b4a823 */
    "\tbgez $21,.L8014F4CC\n"   /* 8014F4B8: 06a10004 */
    "\tlw $22,0($19)\n"   /* 8014F4BC: 8e760000 */
    "\tnegu $21,$21\n"   /* 8014F4C0: 0015a823 */
    "\tj .L8014F4D8\n"   /* 8014F4C4: 08053d36 */
    "\tsllv $22,$22,$21\n"   /* 8014F4C8: 02b6b004 */
    ".L8014F4CC:\n"
    "\tbeq $21,$0,.L8014F4D8\n"   /* 8014F4CC: 12a00002 */
    "\tsll $0,$0,0x0\n"   /* 8014F4D0: 00000000 */
    "\tsrav $22,$22,$21\n"   /* 8014F4D4: 02b6b007 */
    ".L8014F4D8:\n"
    "\tlw $19,0($18)\n"   /* 8014F4D8: 8e530000 */
    "\tlw $20,16($18)\n"   /* 8014F4DC: 8e540010 */
    "\tori $21,$0,0x1\n"   /* 8014F4E0: 34150001 */
    "\tbne $19,$21,.L8014F4F4\n"   /* 8014F4E4: 16750003 */
    "\tslt $23,$22,$20\n"   /* 8014F4E8: 02d4b82a */
    "\tbne $23,$0,.L8014F698\n"   /* 8014F4EC: 16e0006a */
    "\tsll $0,$0,0x0\n"   /* 8014F4F0: 00000000 */
    ".L8014F4F4:\n"
    "\tlw $2,172($16)\n"   /* 8014F4F4: 8e0200ac */
    "\tsw $21,0($18)\n"   /* 8014F4F8: ae550000 */
    "\tsw $2,20($18)\n"   /* 8014F4FC: ae420014 */
    "\tsw $22,16($18)\n"   /* 8014F500: ae560010 */
    "\tandi $19,$12,0xffff\n"   /* 8014F504: 3193ffff */
    "\tlw $21,96($16)\n"   /* 8014F508: 8e150060 */
    "\taddiu $19,$19,-1\n"   /* 8014F50C: 2673ffff */
    "\tlh $2,0($21)\n"   /* 8014F510: 86a20000 */
    "\tlh $3,2($21)\n"   /* 8014F514: 86a30002 */
    "\tlh $4,4($21)\n"   /* 8014F518: 86a40004 */
    "\tlw $5,32($16)\n"   /* 8014F51C: 8e050020 */
    "\tsll $20,$19,0x2\n"   /* 8014F520: 0013a080 */
    "\tadd $5,$5,$20\n"   /* 8014F524: 00b42820 */
    ".L8014F528:\n"
    "\tbltz $19,.L8014F620\n"   /* 8014F528: 0660003d */
    "\tsll $0,$0,0x0\n"   /* 8014F52C: 00000000 */
    "\tlw $20,0($5)\n"   /* 8014F530: 8cb40000 */
    "\tsll $0,$0,0x0\n"   /* 8014F534: 00000000 */
    "\tlh $28,0($20)\n"   /* 8014F538: 869c0000 */
    "\tsll $0,$0,0x0\n"   /* 8014F53C: 00000000 */
    "\tmult $28,$2\n"   /* 8014F540: 03820018 */
    "\tmflo $28\n"   /* 8014F544: 0000e012 */
    "\tlh $23,6($20)\n"   /* 8014F548: 86970006 */
    "\tsll $0,$0,0x0\n"   /* 8014F54C: 00000000 */
    "\tmult $23,$3\n"   /* 8014F550: 02e30018 */
    "\tmflo $23\n"   /* 8014F554: 0000b812 */
    "\tadd $28,$23,$28\n"   /* 8014F558: 02fce020 */
    "\tlh $23,12($20)\n"   /* 8014F55C: 8697000c */
    "\tsll $0,$0,0x0\n"   /* 8014F560: 00000000 */
    "\tmult $23,$4\n"   /* 8014F564: 02e40018 */
    "\tmflo $23\n"   /* 8014F568: 0000b812 */
    "\tadd $28,$28,$23\n"   /* 8014F56C: 0397e020 */
    "\tbgez $28,.L8014F57C\n"   /* 8014F570: 07810002 */
    "\tsll $0,$0,0x0\n"   /* 8014F574: 00000000 */
    "\taddiu $28,$28,4095\n"   /* 8014F578: 279c0fff */
    ".L8014F57C:\n"
    "\tlh $29,2($20)\n"   /* 8014F57C: 869d0002 */
    "\tsra $28,$28,0xc\n"   /* 8014F580: 001ce303 */
    "\tmult $29,$2\n"   /* 8014F584: 03a20018 */
    "\tmflo $29\n"   /* 8014F588: 0000e812 */
    "\tlh $23,8($20)\n"   /* 8014F58C: 86970008 */
    "\tsll $0,$0,0x0\n"   /* 8014F590: 00000000 */
    "\tmult $23,$3\n"   /* 8014F594: 02e30018 */
    "\tmflo $23\n"   /* 8014F598: 0000b812 */
    "\tadd $29,$23,$29\n"   /* 8014F59C: 02fde820 */
    "\tlh $23,14($20)\n"   /* 8014F5A0: 8697000e */
    "\tsll $0,$0,0x0\n"   /* 8014F5A4: 00000000 */
    "\tmult $23,$4\n"   /* 8014F5A8: 02e40018 */
    "\tmflo $23\n"   /* 8014F5AC: 0000b812 */
    "\tadd $29,$29,$23\n"   /* 8014F5B0: 03b7e820 */
    "\tbgez $29,.L8014F5C0\n"   /* 8014F5B4: 07a10002 */
    "\tsll $0,$0,0x0\n"   /* 8014F5B8: 00000000 */
    "\taddiu $29,$29,4095\n"   /* 8014F5BC: 27bd0fff */
    ".L8014F5C0:\n"
    "\tlh $30,4($20)\n"   /* 8014F5C0: 869e0004 */
    "\tsra $29,$29,0xc\n"   /* 8014F5C4: 001deb03 */
    "\tmult $30,$2\n"   /* 8014F5C8: 03c20018 */
    "\tmflo $30\n"   /* 8014F5CC: 0000f012 */
    "\tlh $23,10($20)\n"   /* 8014F5D0: 8697000a */
    "\tsll $0,$0,0x0\n"   /* 8014F5D4: 00000000 */
    "\tmult $23,$3\n"   /* 8014F5D8: 02e30018 */
    "\tmflo $23\n"   /* 8014F5DC: 0000b812 */
    "\tadd $30,$23,$30\n"   /* 8014F5E0: 02fef020 */
    "\tlh $23,16($20)\n"   /* 8014F5E4: 86970010 */
    "\tsll $0,$0,0x0\n"   /* 8014F5E8: 00000000 */
    "\tmult $23,$4\n"   /* 8014F5EC: 02e40018 */
    "\tmflo $23\n"   /* 8014F5F0: 0000b812 */
    "\tadd $30,$30,$23\n"   /* 8014F5F4: 03d7f020 */
    "\tbgez $30,.L8014F604\n"   /* 8014F5F8: 07c10002 */
    "\tsll $0,$0,0x0\n"   /* 8014F5FC: 00000000 */
    "\taddiu $30,$30,4095\n"   /* 8014F600: 27de0fff */
    ".L8014F604:\n"
    "\tsra $30,$30,0xc\n"   /* 8014F604: 001ef303 */
    "\taddu $2,$28,$0\n"   /* 8014F608: 03801021 */
    "\taddu $3,$29,$0\n"   /* 8014F60C: 03a01821 */
    "\taddu $4,$30,$0\n"   /* 8014F610: 03c02021 */
    "\taddiu $5,$5,-4\n"   /* 8014F614: 24a5fffc */
    "\tj .L8014F528\n"   /* 8014F618: 08053d4a */
    "\taddiu $19,$19,-1\n"   /* 8014F61C: 2673ffff */
    ".L8014F620:\n"
    "\tsw $2,4($18)\n"   /* 8014F620: ae420004 */
    "\tsw $3,8($18)\n"   /* 8014F624: ae430008 */
    "\tlh $20,2($8)\n"   /* 8014F628: 85140002 */
    "\tsw $4,12($18)\n"   /* 8014F62C: ae44000c */
    "\tlbu $23,8($8)\n"   /* 8014F630: 91170008 */
    "\tbeq $20,$0,.L8014F64C\n"   /* 8014F634: 12800005 */
    "\tsw $13,28($18)\n"   /* 8014F638: ae4d001c */
    "\tlbu $21,6($8)\n"   /* 8014F63C: 91150006 */
    "\tsll $0,$0,0x0\n"   /* 8014F640: 00000000 */
    "\tsh $21,28($18)\n"   /* 8014F644: a655001c */
    "\tsh $20,30($18)\n"   /* 8014F648: a654001e */
    ".L8014F64C:\n"
    "\tlw $20,12($8)\n"   /* 8014F64C: 8d14000c */
    "\tsw $23,24($18)\n"   /* 8014F650: ae570018 */
    "\tbeq $20,$0,.L8014F698\n"   /* 8014F654: 12800010 */
    "\tlw $21,176($16)\n"   /* 8014F658: 8e1500b0 */
    "\tlw $22,180($16)\n"   /* 8014F65C: 8e1600b4 */
    "\tslti $23,$21,10\n"   /* 8014F660: 2ab7000a */
    "\tbeq $23,$0,.L8014F698\n"   /* 8014F664: 12e0000c */
    "\tsll $0,$0,0x0\n"   /* 8014F668: 00000000 */
    ".L8014F66C:\n"
    "\tblez $21,.L8014F688\n"   /* 8014F66C: 1aa00006 */
    "\tlw $4,0($22)\n"   /* 8014F670: 8ec40000 */
    "\tsll $0,$0,0x0\n"   /* 8014F674: 00000000 */
    "\tbeq $4,$8,.L8014F698\n"   /* 8014F678: 10880007 */
    "\taddiu $21,$21,-1\n"   /* 8014F67C: 26b5ffff */
    "\tj .L8014F66C\n"   /* 8014F680: 08053d9b */
    "\taddiu $22,$22,4\n"   /* 8014F684: 26d60004 */
    ".L8014F688:\n"
    "\tlw $21,176($16)\n"   /* 8014F688: 8e1500b0 */
    "\tsw $8,0($22)\n"   /* 8014F68C: aec80000 */
    "\taddiu $21,$21,1\n"   /* 8014F690: 26b50001 */
    "\tsw $21,176($16)\n"   /* 8014F694: ae1500b0 */
    ".L8014F698:\n"
    "\taddiu $25,$25,8\n"   /* 8014F698: 27390008 */
    "\taddiu $17,$17,-1\n"   /* 8014F69C: 2631ffff */
    "\tj .L8014F178\n"   /* 8014F6A0: 08053c5e */
    "\taddiu $18,$18,32\n"   /* 8014F6A4: 26520020 */
    ".L8014F6A8:\n"
    "\tlw $31,200($16)\n"   /* 8014F6A8: 8e1f00c8 */
    "\tsll $0,$0,0x0\n"   /* 8014F6AC: 00000000 */
    "\tjr $31\n"   /* 8014F6B0: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014F6B4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpLeafTestPoints,.-hdpLeafTestPoints\n");

/* 8014F6B8 (hand-written asm): apply the s5 parent transforms listed at s6 to v0/v1/a3, then rescale
 * by the node shift. */
__asm__(
    ".section .text_8014F6B8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpApplyParentTransforms\n"
    ".type hdpApplyParentTransforms,@function\n"
    "hdpApplyParentTransforms:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\taddu $20,$31,$0\n"   /* 8014F6B8: 03e0a021 */
    ".L8014F6BC:\n"
    "\tblez $21,.L8014F708\n"   /* 8014F6BC: 1aa00012 */
    "\taddu $4,$2,$0\n"   /* 8014F6C0: 00402021 */
    "\tlw $6,0($22)\n"   /* 8014F6C4: 8ec60000 */
    "\taddu $5,$3,$0\n"   /* 8014F6C8: 00602821 */
    "\tlw $19,0($6)\n"   /* 8014F6CC: 8cd30000 */
    "\tlw $30,4($6)\n"   /* 8014F6D0: 8cde0004 */
    "\tlw $28,8($6)\n"   /* 8014F6D4: 8cdc0008 */
    "\tlw $2,12($6)\n"   /* 8014F6D8: 8cc2000c */
    "\tlw $3,16($6)\n"   /* 8014F6DC: 8cc30010 */
    "\tctc2 $19,$0\n"   /* 8014F6E0: 48d30000 */
    "\tctc2 $30,$1\n"   /* 8014F6E4: 48de0800 */
    "\tctc2 $28,$2\n"   /* 8014F6E8: 48dc1000 */
    "\tctc2 $2,$3\n"   /* 8014F6EC: 48c21800 */
    "\tctc2 $3,$4\n"   /* 8014F6F0: 48c32000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F6F4: 0c053be6 */
    "\taddu $6,$7,$0\n"   /* 8014F6F8: 00e03021 */
    "\taddiu $21,$21,-1\n"   /* 8014F6FC: 26b5ffff */
    "\tj .L8014F6BC\n"   /* 8014F700: 08053daf */
    "\taddiu $22,$22,4\n"   /* 8014F704: 26d60004 */
    ".L8014F708:\n"
    "\tlw $28,28($16)\n"   /* 8014F708: 8e1c001c */
    "\tsrl $19,$12,0x10\n"   /* 8014F70C: 000c9c02 */
    "\tsubu $19,$19,$28\n"   /* 8014F710: 027c9823 */
    "\tbgez $19,.L8014F730\n"   /* 8014F714: 06610006 */
    "\tsll $0,$0,0x0\n"   /* 8014F718: 00000000 */
    "\tnegu $19,$19\n"   /* 8014F71C: 00139823 */
    "\tsrav $2,$2,$19\n"   /* 8014F720: 02621007 */
    "\tsrav $3,$3,$19\n"   /* 8014F724: 02631807 */
    "\tj .L8014F744\n"   /* 8014F728: 08053dd1 */
    "\tsrav $7,$7,$19\n"   /* 8014F72C: 02673807 */
    ".L8014F730:\n"
    "\tblez $19,.L8014F744\n"   /* 8014F730: 1a600004 */
    "\tsll $0,$0,0x0\n"   /* 8014F734: 00000000 */
    "\tsllv $2,$2,$19\n"   /* 8014F738: 02621004 */
    "\tsllv $3,$3,$19\n"   /* 8014F73C: 02631804 */
    "\tsllv $7,$7,$19\n"   /* 8014F740: 02673804 */
    ".L8014F744:\n"
    "\taddu $31,$20,$0\n"   /* 8014F744: 0280f821 */
    "\tjr $31\n"   /* 8014F748: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014F74C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpApplyParentTransforms,.-hdpApplyParentTransforms\n");

/* 8014F750 (hand-written asm): kind 8 leaf test (variant without st->fc4): box test per hit point,
 * marking hit records with 2. */
__asm__(
    ".section .text_8014F750,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpLeafTestPointsMark\n"
    ".type hdpLeafTestPointsMark,@function\n"
    "hdpLeafTestPointsMark:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $31,48($16)\n"   /* 8014F750: ae1f0030 */
    "\tlw $17,16($16)\n"   /* 8014F754: 8e110010 */
    "\tlw $18,24($16)\n"   /* 8014F758: 8e120018 */
    "\tlw $25,20($16)\n"   /* 8014F75C: 8e190014 */
    ".L8014F760:\n"
    "\tblez $17,.L8014F8B8\n"   /* 8014F760: 1a200055 */
    "\tandi $21,$12,0xffff\n"   /* 8014F764: 3195ffff */
    "\tlw $22,0($18)\n"   /* 8014F768: 8e560000 */
    "\tlw $20,8($16)\n"   /* 8014F76C: 8e140008 */
    "\tbne $22,$0,.L8014F8A8\n"   /* 8014F770: 16c0004d */
    "\taddiu $4,$20,128\n"   /* 8014F774: 26840080 */
    "\taddu $5,$25,$0\n"   /* 8014F778: 03202821 */
    "\tjal hdpRotTransSVector\n"   /* 8014F77C: 0c053c33 */
    "\taddiu $23,$20,168\n"   /* 8014F780: 269700a8 */
    "\tlw $22,32($16)\n"   /* 8014F784: 8e160020 */
    ".L8014F788:\n"
    "\tblez $21,.L8014F7D4\n"   /* 8014F788: 1aa00012 */
    "\taddu $4,$2,$0\n"   /* 8014F78C: 00402021 */
    "\tlw $6,0($22)\n"   /* 8014F790: 8ec60000 */
    "\taddu $5,$3,$0\n"   /* 8014F794: 00602821 */
    "\tlw $29,0($6)\n"   /* 8014F798: 8cdd0000 */
    "\tlw $30,4($6)\n"   /* 8014F79C: 8cde0004 */
    "\tlw $28,8($6)\n"   /* 8014F7A0: 8cdc0008 */
    "\tlw $2,12($6)\n"   /* 8014F7A4: 8cc2000c */
    "\tlw $3,16($6)\n"   /* 8014F7A8: 8cc30010 */
    "\tctc2 $29,$0\n"   /* 8014F7AC: 48dd0000 */
    "\tctc2 $30,$1\n"   /* 8014F7B0: 48de0800 */
    "\tctc2 $28,$2\n"   /* 8014F7B4: 48dc1000 */
    "\tctc2 $2,$3\n"   /* 8014F7B8: 48c21800 */
    "\tctc2 $3,$4\n"   /* 8014F7BC: 48c32000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F7C0: 0c053be6 */
    "\taddu $6,$7,$0\n"   /* 8014F7C4: 00e03021 */
    "\taddiu $21,$21,-1\n"   /* 8014F7C8: 26b5ffff */
    "\tj .L8014F788\n"   /* 8014F7CC: 08053de2 */
    "\taddiu $22,$22,4\n"   /* 8014F7D0: 26d60004 */
    ".L8014F7D4:\n"
    "\tadd $4,$2,$9\n"   /* 8014F7D4: 00492020 */
    "\tadd $5,$3,$10\n"   /* 8014F7D8: 006a2820 */
    "\tadd $6,$7,$11\n"   /* 8014F7DC: 00eb3020 */
    "\taddiu $20,$8,16\n"   /* 8014F7E0: 25140010 */
    "\tlw $22,0($20)\n"   /* 8014F7E4: 8e960000 */
    "\tlw $23,4($20)\n"   /* 8014F7E8: 8e970004 */
    "\tlw $2,8($20)\n"   /* 8014F7EC: 8e820008 */
    "\tlw $3,12($20)\n"   /* 8014F7F0: 8e83000c */
    "\tlw $7,16($20)\n"   /* 8014F7F4: 8e870010 */
    "\tctc2 $22,$0\n"   /* 8014F7F8: 48d60000 */
    "\tctc2 $23,$1\n"   /* 8014F7FC: 48d70800 */
    "\tctc2 $2,$2\n"   /* 8014F800: 48c21000 */
    "\tctc2 $3,$3\n"   /* 8014F804: 48c31800 */
    "\tctc2 $7,$4\n"   /* 8014F808: 48c72000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F80C: 0c053be6 */
    "\taddiu $23,$8,36\n"   /* 8014F810: 25170024 */
    "\taddu $20,$2,$28\n"   /* 8014F814: 005ca021 */
    "\tblez $20,.L8014F8A8\n"   /* 8014F818: 1a800023 */
    "\taddu $21,$3,$29\n"   /* 8014F81C: 007da821 */
    "\tblez $21,.L8014F8A8\n"   /* 8014F820: 1aa00021 */
    "\taddu $22,$7,$30\n"   /* 8014F824: 00feb021 */
    "\tblez $22,.L8014F8A8\n"   /* 8014F828: 1ac0001f */
    "\tsll $0,$0,0x0\n"   /* 8014F82C: 00000000 */
    "\tlbu $20,4($8)\n"   /* 8014F830: 91140004 */
    "\tsll $0,$0,0x0\n"   /* 8014F834: 00000000 */
    "\tsltiu $21,$20,4\n"   /* 8014F838: 2e950004 */
    "\tbne $21,$0,.L8014F8A0\n"   /* 8014F83C: 16a00018 */
    "\taddiu $23,$8,48\n"   /* 8014F840: 25170030 */
    "\tlw $2,0($23)\n"   /* 8014F844: 8ee20000 */
    "\tlw $3,4($23)\n"   /* 8014F848: 8ee30004 */
    "\tlw $7,8($23)\n"   /* 8014F84C: 8ee70008 */
    "\tlw $29,12($23)\n"   /* 8014F850: 8efd000c */
    "\tlw $28,16($23)\n"   /* 8014F854: 8efc0010 */
    "\tctc2 $2,$0\n"   /* 8014F858: 48c20000 */
    "\tctc2 $3,$1\n"   /* 8014F85C: 48c30800 */
    "\tctc2 $7,$2\n"   /* 8014F860: 48c71000 */
    "\tctc2 $29,$3\n"   /* 8014F864: 48dd1800 */
    "\tctc2 $28,$4\n"   /* 8014F868: 48dc2000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F86C: 0c053be6 */
    "\taddiu $23,$8,68\n"   /* 8014F870: 25170044 */
    "\taddu $21,$2,$28\n"   /* 8014F874: 005ca821 */
    "\tblez $21,.L8014F8A8\n"   /* 8014F878: 1aa0000b */
    "\tsltiu $23,$20,5\n"   /* 8014F87C: 2e970005 */
    "\tbne $23,$0,.L8014F8A0\n"   /* 8014F880: 16e00007 */
    "\taddu $22,$3,$29\n"   /* 8014F884: 007db021 */
    "\tblez $22,.L8014F8A8\n"   /* 8014F888: 1ac00007 */
    "\tsltiu $23,$20,6\n"   /* 8014F88C: 2e970006 */
    "\tbne $23,$0,.L8014F8A0\n"   /* 8014F890: 16e00003 */
    "\taddu $22,$7,$30\n"   /* 8014F894: 00feb021 */
    "\tblez $22,.L8014F8A8\n"   /* 8014F898: 1ac00003 */
    "\tsll $0,$0,0x0\n"   /* 8014F89C: 00000000 */
    ".L8014F8A0:\n"
    "\tori $20,$0,0x2\n"   /* 8014F8A0: 34140002 */
    "\tsw $20,0($18)\n"   /* 8014F8A4: ae540000 */
    ".L8014F8A8:\n"
    "\taddiu $25,$25,8\n"   /* 8014F8A8: 27390008 */
    "\taddiu $17,$17,-1\n"   /* 8014F8AC: 2631ffff */
    "\tj .L8014F760\n"   /* 8014F8B0: 08053dd8 */
    "\taddiu $18,$18,32\n"   /* 8014F8B4: 26520020 */
    ".L8014F8B8:\n"
    "\tlw $31,48($16)\n"   /* 8014F8B8: 8e1f0030 */
    "\tsll $0,$0,0x0\n"   /* 8014F8BC: 00000000 */
    "\tjr $31\n"   /* 8014F8C0: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014F8C4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpLeafTestPointsMark,.-hdpLeafTestPointsMark\n");

/* 8014F8C8 (hand-written asm): kind 8 leaf test with st->fc4 set: one box test with margin st->fc0;
 * sets st->fb8 = 1, st->fbc = 0 on a hit. */
__asm__(
    ".section .text_8014F8C8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpLeafTestSingleBox\n"
    ".type hdpLeafTestSingleBox,@function\n"
    "hdpLeafTestSingleBox:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $31,48($16)\n"   /* 8014F8C8: ae1f0030 */
    "\taddu $4,$9,$0\n"   /* 8014F8CC: 01202021 */
    "\taddu $5,$10,$0\n"   /* 8014F8D0: 01402821 */
    "\taddu $6,$11,$0\n"   /* 8014F8D4: 01603021 */
    "\taddiu $20,$8,16\n"   /* 8014F8D8: 25140010 */
    "\tlw $22,0($20)\n"   /* 8014F8DC: 8e960000 */
    "\tlw $23,4($20)\n"   /* 8014F8E0: 8e970004 */
    "\tlw $2,8($20)\n"   /* 8014F8E4: 8e820008 */
    "\tlw $3,12($20)\n"   /* 8014F8E8: 8e83000c */
    "\tlw $7,16($20)\n"   /* 8014F8EC: 8e870010 */
    "\tctc2 $22,$0\n"   /* 8014F8F0: 48d60000 */
    "\tctc2 $23,$1\n"   /* 8014F8F4: 48d70800 */
    "\tctc2 $2,$2\n"   /* 8014F8F8: 48c21000 */
    "\tctc2 $3,$3\n"   /* 8014F8FC: 48c31800 */
    "\tctc2 $7,$4\n"   /* 8014F900: 48c72000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F904: 0c053be6 */
    "\taddiu $23,$8,36\n"   /* 8014F908: 25170024 */
    "\tlw $17,192($16)\n"   /* 8014F90C: 8e1100c0 */
    "\taddu $20,$2,$28\n"   /* 8014F910: 005ca021 */
    "\tsubu $20,$20,$17\n"   /* 8014F914: 0291a023 */
    "\tblez $20,.L8014F9B8\n"   /* 8014F918: 1a800027 */
    "\taddu $21,$3,$29\n"   /* 8014F91C: 007da821 */
    "\tsubu $21,$21,$17\n"   /* 8014F920: 02b1a823 */
    "\tblez $21,.L8014F9B8\n"   /* 8014F924: 1aa00024 */
    "\taddu $22,$7,$30\n"   /* 8014F928: 00feb021 */
    "\tblez $22,.L8014F9B8\n"   /* 8014F92C: 1ac00022 */
    "\tsll $0,$0,0x0\n"   /* 8014F930: 00000000 */
    "\tlbu $20,4($8)\n"   /* 8014F934: 91140004 */
    "\tsll $0,$0,0x0\n"   /* 8014F938: 00000000 */
    "\tsltiu $21,$20,4\n"   /* 8014F93C: 2e950004 */
    "\tbne $21,$0,.L8014F9AC\n"   /* 8014F940: 16a0001a */
    "\taddiu $23,$8,48\n"   /* 8014F944: 25170030 */
    "\tlw $2,0($23)\n"   /* 8014F948: 8ee20000 */
    "\tlw $3,4($23)\n"   /* 8014F94C: 8ee30004 */
    "\tlw $7,8($23)\n"   /* 8014F950: 8ee70008 */
    "\tlw $29,12($23)\n"   /* 8014F954: 8efd000c */
    "\tlw $28,16($23)\n"   /* 8014F958: 8efc0010 */
    "\tctc2 $2,$0\n"   /* 8014F95C: 48c20000 */
    "\tctc2 $3,$1\n"   /* 8014F960: 48c30800 */
    "\tctc2 $7,$2\n"   /* 8014F964: 48c71000 */
    "\tctc2 $29,$3\n"   /* 8014F968: 48dd1800 */
    "\tctc2 $28,$4\n"   /* 8014F96C: 48dc2000 */
    "\tjal hdpRotVecSplit\n"   /* 8014F970: 0c053be6 */
    "\taddiu $23,$8,68\n"   /* 8014F974: 25170044 */
    "\taddu $21,$2,$28\n"   /* 8014F978: 005ca821 */
    "\tsubu $21,$21,$17\n"   /* 8014F97C: 02b1a823 */
    "\tblez $21,.L8014F9B8\n"   /* 8014F980: 1aa0000d */
    "\tsltiu $23,$20,5\n"   /* 8014F984: 2e970005 */
    "\tbne $23,$0,.L8014F9AC\n"   /* 8014F988: 16e00008 */
    "\taddu $22,$3,$29\n"   /* 8014F98C: 007db021 */
    "\tsubu $22,$22,$17\n"   /* 8014F990: 02d1b023 */
    "\tblez $22,.L8014F9B8\n"   /* 8014F994: 1ac00008 */
    "\tsltiu $23,$20,6\n"   /* 8014F998: 2e970006 */
    "\tbne $23,$0,.L8014F9AC\n"   /* 8014F99C: 16e00003 */
    "\taddu $22,$7,$30\n"   /* 8014F9A0: 00feb021 */
    "\tblez $22,.L8014F9B8\n"   /* 8014F9A4: 1ac00004 */
    "\tsll $0,$0,0x0\n"   /* 8014F9A8: 00000000 */
    ".L8014F9AC:\n"
    "\tsw $0,188($16)\n"   /* 8014F9AC: ae0000bc */
    "\tori $20,$0,0x1\n"   /* 8014F9B0: 34140001 */
    "\tsw $20,184($16)\n"   /* 8014F9B4: ae1400b8 */
    ".L8014F9B8:\n"
    "\tlw $31,48($16)\n"   /* 8014F9B8: 8e1f0030 */
    "\tsll $0,$0,0x0\n"   /* 8014F9BC: 00000000 */
    "\tjr $31\n"   /* 8014F9C0: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014F9C4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpLeafTestSingleBox,.-hdpLeafTestSingleBox\n");

/* 8014F9C8 (hand-written asm): kind 11 box test of the model at s3; s1 = 1 when inside. */
__asm__(
    ".section .text_8014F9C8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpBoxTestModel\n"
    ".type hdpBoxTestModel,@function\n"
    "hdpBoxTestModel:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsw $31,48($16)\n"   /* 8014F9C8: ae1f0030 */
    "\taddu $4,$9,$0\n"   /* 8014F9CC: 01202021 */
    "\taddu $5,$10,$0\n"   /* 8014F9D0: 01402821 */
    "\taddu $6,$11,$0\n"   /* 8014F9D4: 01603021 */
    "\taddu $17,$0,$0\n"   /* 8014F9D8: 00008821 */
    "\taddiu $20,$19,4\n"   /* 8014F9DC: 26740004 */
    "\tlw $22,0($20)\n"   /* 8014F9E0: 8e960000 */
    "\tlw $23,4($20)\n"   /* 8014F9E4: 8e970004 */
    "\tlw $2,8($20)\n"   /* 8014F9E8: 8e820008 */
    "\tlw $3,12($20)\n"   /* 8014F9EC: 8e83000c */
    "\tlw $7,16($20)\n"   /* 8014F9F0: 8e870010 */
    "\tctc2 $22,$0\n"   /* 8014F9F4: 48d60000 */
    "\tctc2 $23,$1\n"   /* 8014F9F8: 48d70800 */
    "\tctc2 $2,$2\n"   /* 8014F9FC: 48c21000 */
    "\tctc2 $3,$3\n"   /* 8014FA00: 48c31800 */
    "\tctc2 $7,$4\n"   /* 8014FA04: 48c72000 */
    "\tjal hdpRotVecSplit\n"   /* 8014FA08: 0c053be6 */
    "\taddiu $23,$19,24\n"   /* 8014FA0C: 26770018 */
    "\taddu $20,$2,$28\n"   /* 8014FA10: 005ca021 */
    "\tblez $20,.L8014FAA0\n"   /* 8014FA14: 1a800022 */
    "\taddu $21,$3,$29\n"   /* 8014FA18: 007da821 */
    "\tblez $21,.L8014FAA0\n"   /* 8014FA1C: 1aa00020 */
    "\taddu $22,$7,$30\n"   /* 8014FA20: 00feb021 */
    "\tblez $22,.L8014FAA0\n"   /* 8014FA24: 1ac0001e */
    "\tsll $0,$0,0x0\n"   /* 8014FA28: 00000000 */
    "\tlbu $20,0($19)\n"   /* 8014FA2C: 92740000 */
    "\tsll $0,$0,0x0\n"   /* 8014FA30: 00000000 */
    "\tsltiu $21,$20,4\n"   /* 8014FA34: 2e950004 */
    "\tbne $21,$0,.L8014FA9C\n"   /* 8014FA38: 16a00018 */
    "\taddiu $23,$19,36\n"   /* 8014FA3C: 26770024 */
    "\tlw $2,0($23)\n"   /* 8014FA40: 8ee20000 */
    "\tlw $3,4($23)\n"   /* 8014FA44: 8ee30004 */
    "\tlw $7,8($23)\n"   /* 8014FA48: 8ee70008 */
    "\tlw $29,12($23)\n"   /* 8014FA4C: 8efd000c */
    "\tlw $28,16($23)\n"   /* 8014FA50: 8efc0010 */
    "\tctc2 $2,$0\n"   /* 8014FA54: 48c20000 */
    "\tctc2 $3,$1\n"   /* 8014FA58: 48c30800 */
    "\tctc2 $7,$2\n"   /* 8014FA5C: 48c71000 */
    "\tctc2 $29,$3\n"   /* 8014FA60: 48dd1800 */
    "\tctc2 $28,$4\n"   /* 8014FA64: 48dc2000 */
    "\tjal hdpRotVecSplit\n"   /* 8014FA68: 0c053be6 */
    "\taddiu $23,$19,56\n"   /* 8014FA6C: 26770038 */
    "\taddu $21,$2,$28\n"   /* 8014FA70: 005ca821 */
    "\tblez $21,.L8014FAA0\n"   /* 8014FA74: 1aa0000a */
    "\tsltiu $23,$20,5\n"   /* 8014FA78: 2e970005 */
    "\tbne $23,$0,.L8014FA9C\n"   /* 8014FA7C: 16e00007 */
    "\taddu $22,$3,$29\n"   /* 8014FA80: 007db021 */
    "\tblez $22,.L8014FAA0\n"   /* 8014FA84: 1ac00006 */
    "\tsltiu $23,$20,6\n"   /* 8014FA88: 2e970006 */
    "\tbne $23,$0,.L8014FA9C\n"   /* 8014FA8C: 16e00003 */
    "\taddu $22,$7,$30\n"   /* 8014FA90: 00feb021 */
    "\tblez $22,.L8014FAA0\n"   /* 8014FA94: 1ac00002 */
    "\tsll $0,$0,0x0\n"   /* 8014FA98: 00000000 */
    ".L8014FA9C:\n"
    "\tori $17,$0,0x1\n"   /* 8014FA9C: 34110001 */
    ".L8014FAA0:\n"
    "\tlw $31,48($16)\n"   /* 8014FAA0: 8e1f0030 */
    "\tsll $0,$0,0x0\n"   /* 8014FAA4: 00000000 */
    "\tjr $31\n"   /* 8014FAA8: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014FAAC: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpBoxTestModel,.-hdpBoxTestModel\n");

/* 8014FAB0 (hand-written asm): hdp pass entry (called by hdpCollideObject): saves registers in the
 * scratchpad, resets the per-point records, walks the object's own hierarchy
 * and then the other objects (csGetRoot / csListHead list). */
__asm__(
    ".section .text_8014FAB0,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl hdpPassRun\n"
    ".type hdpPassRun,@function\n"
    "hdpPassRun:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlui $8,0x1f80\n"   /* 8014FAB0: 3c081f80 */
    "\tsw $16,108($8)\n"   /* 8014FAB4: ad10006c */
    "\tlui $16,0x1f80\n"   /* 8014FAB8: 3c101f80 */
    "\tsw $17,112($16)\n"   /* 8014FABC: ae110070 */
    "\tsw $18,116($16)\n"   /* 8014FAC0: ae120074 */
    "\tsw $19,120($16)\n"   /* 8014FAC4: ae130078 */
    "\tsw $20,124($16)\n"   /* 8014FAC8: ae14007c */
    "\tsw $21,128($16)\n"   /* 8014FACC: ae150080 */
    "\tsw $22,132($16)\n"   /* 8014FAD0: ae160084 */
    "\tsw $23,136($16)\n"   /* 8014FAD4: ae170088 */
    "\tsw $29,140($16)\n"   /* 8014FAD8: ae1d008c */
    "\tsw $28,144($16)\n"   /* 8014FADC: ae1c0090 */
    "\tsw $4,148($16)\n"   /* 8014FAE0: ae040094 */
    "\tsw $5,152($16)\n"   /* 8014FAE4: ae050098 */
    "\tsw $31,156($16)\n"   /* 8014FAE8: ae1f009c */
    "\tsw $30,160($16)\n"   /* 8014FAEC: ae1e00a0 */
    "\tsw $4,8($16)\n"   /* 8014FAF0: ae040008 */
    "\tlw $23,180($4)\n"   /* 8014FAF4: 8c9700b4 */
    "\taddiu $18,$4,184\n"   /* 8014FAF8: 249200b8 */
    "\tsw $23,16($16)\n"   /* 8014FAFC: ae170010 */
    "\tsw $23,188($16)\n"   /* 8014FB00: ae1700bc */
    "\tsw $18,20($16)\n"   /* 8014FB04: ae120014 */
    "\taddiu $22,$5,4\n"   /* 8014FB08: 24b60004 */
    "\tsw $22,24($16)\n"   /* 8014FB0C: ae160018 */
    "\tlw $17,16($29)\n"   /* 8014FB10: 8fb10010 */
    "\tsw $7,164($16)\n"   /* 8014FB14: ae0700a4 */
    "\tsw $17,168($16)\n"   /* 8014FB18: ae1100a8 */
    "\tsw $0,176($16)\n"   /* 8014FB1C: ae0000b0 */
    "\tsw $0,184($16)\n"   /* 8014FB20: ae0000b8 */
    "\taddiu $17,$0,-1\n"   /* 8014FB24: 2411ffff */
    ".L8014FB28:\n"
    "\tsw $0,0($22)\n"   /* 8014FB28: aec00000 */
    "\tsh $0,26($22)\n"   /* 8014FB2C: a6c0001a */
    "\tsw $0,20($22)\n"   /* 8014FB30: aec00014 */
    "\tsh $17,28($22)\n"   /* 8014FB34: a6d1001c */
    "\tsh $0,30($22)\n"   /* 8014FB38: a6c0001e */
    "\tsw $0,24($22)\n"   /* 8014FB3C: aec00018 */
    "\taddiu $23,$23,-1\n"   /* 8014FB40: 26f7ffff */
    "\tbgtz $23,.L8014FB28\n"   /* 8014FB44: 1ee0fff8 */
    "\taddiu $22,$22,32\n"   /* 8014FB48: 26d60020 */
    "\tlw $17,180($16)\n"   /* 8014FB4C: 8e1100b4 */
    "\tori $18,$0,0xa\n"   /* 8014FB50: 3412000a */
    ".L8014FB54:\n"
    "\tsw $0,0($17)\n"   /* 8014FB54: ae200000 */
    "\taddiu $18,$18,-1\n"   /* 8014FB58: 2652ffff */
    "\tbgtz $18,.L8014FB54\n"   /* 8014FB5C: 1e40fffd */
    "\taddiu $17,$17,4\n"   /* 8014FB60: 26310004 */
    "\tlw $8,284($4)\n"   /* 8014FB64: 8c88011c */
    "\tori $19,$0,0x1\n"   /* 8014FB68: 34130001 */
    "\tbeq $8,$0,.L8014FB88\n"   /* 8014FB6C: 11000006 */
    "\tori $20,$0,0x64\n"   /* 8014FB70: 34140064 */
    "\tlbu $18,0($8)\n"   /* 8014FB74: 91120000 */
    "\tsll $0,$0,0x0\n"   /* 8014FB78: 00000000 */
    "\tbne $18,$19,.L8014FB88\n"   /* 8014FB7C: 16530002 */
    "\tsll $0,$0,0x0\n"   /* 8014FB80: 00000000 */
    "\tlw $20,16($8)\n"   /* 8014FB84: 8d140010 */
    ".L8014FB88:\n"
    "\tlw $28,144($16)\n"   /* 8014FB88: 8e1c0090 */
    "\tlw $30,160($16)\n"   /* 8014FB8C: 8e1e00a0 */
    "\tlw $29,140($16)\n"   /* 8014FB90: 8e1d008c */
    "\tjal csGetRoot\n"   /* 8014FB94: 0c05257e */
    "\tsw $20,192($16)\n"   /* 8014FB98: ae1400c0 */
    "\tlw $8,284($2)\n"   /* 8014FB9C: 8c48011c */
    "\tbeq $6,$0,.L8014FC28\n"   /* 8014FBA0: 10c00021 */
    "\tsll $0,$0,0x0\n"   /* 8014FBA4: 00000000 */
    "\tbeq $8,$0,.L8014FC28\n"   /* 8014FBA8: 1100001f */
    "\tsll $0,$0,0x0\n"   /* 8014FBAC: 00000000 */
    "\tmult $20,$20\n"   /* 8014FBB0: 02940018 */
    "\tlw $18,28($16)\n"   /* 8014FBB4: 8e12001c */
    "\taddu $12,$0,$0\n"   /* 8014FBB8: 00006021 */
    "\tsll $18,$18,0x10\n"   /* 8014FBBC: 00129400 */
    "\tor $12,$12,$18\n"   /* 8014FBC0: 01926025 */
    "\tsw $2,172($16)\n"   /* 8014FBC4: ae0200ac */
    "\tmflo $20\n"   /* 8014FBC8: 0000a012 */
    "\tlw $9,168($4)\n"   /* 8014FBCC: 8c8900a8 */
    "\tlw $10,172($4)\n"   /* 8014FBD0: 8c8a00ac */
    "\tlw $11,176($4)\n"   /* 8014FBD4: 8c8b00b0 */
    "\tsw $20,12($16)\n"   /* 8014FBD8: ae14000c */
    "\tjal hdpWalk\n"   /* 8014FBDC: 0c053a98 */
    "\taddu $13,$0,$0\n"   /* 8014FBE0: 00006821 */
    "\tlw $17,16($16)\n"   /* 8014FBE4: 8e110010 */
    "\tlw $19,184($16)\n"   /* 8014FBE8: 8e1300b8 */
    "\tlw $18,24($16)\n"   /* 8014FBEC: 8e120018 */
    "\tlw $22,188($16)\n"   /* 8014FBF0: 8e1600bc */
    ".L8014FBF4:\n"
    "\tblez $17,.L8014FC28\n"   /* 8014FBF4: 1a20000c */
    "\tsll $0,$0,0x0\n"   /* 8014FBF8: 00000000 */
    "\tbne $19,$0,.L8014FC14\n"   /* 8014FBFC: 16600005 */
    "\taddu $23,$0,$0\n"   /* 8014FC00: 0000b821 */
    "\tlw $20,0($18)\n"   /* 8014FC04: 8e540000 */
    "\tori $21,$0,0x2\n"   /* 8014FC08: 34150002 */
    "\tbne $20,$21,.L8014FC1C\n"   /* 8014FC0C: 16950003 */
    "\tsll $0,$0,0x0\n"   /* 8014FC10: 00000000 */
    ".L8014FC14:\n"
    "\tsw $23,0($18)\n"   /* 8014FC14: ae570000 */
    "\taddiu $22,$22,1\n"   /* 8014FC18: 26d60001 */
    ".L8014FC1C:\n"
    "\taddiu $17,$17,-1\n"   /* 8014FC1C: 2631ffff */
    "\tj .L8014FBF4\n"   /* 8014FC20: 08053efd */
    "\taddiu $18,$18,32\n"   /* 8014FC24: 26520020 */
    ".L8014FC28:\n"
    "\tlw $28,144($16)\n"   /* 8014FC28: 8e1c0090 */
    "\tlw $30,160($16)\n"   /* 8014FC2C: 8e1e00a0 */
    "\tlw $29,140($16)\n"   /* 8014FC30: 8e1d008c */
    "\tjal csListHead\n"   /* 8014FC34: 0c05250e */
    "\tsw $22,188($16)\n"   /* 8014FC38: ae1600bc */
    ".L8014FC3C:\n"
    "\tblez $22,.L8014FCFC\n"   /* 8014FC3C: 1ac0002f */
    "\tlw $17,28($16)\n"   /* 8014FC40: 8e11001c */
    "\tsw $2,172($16)\n"   /* 8014FC44: ae0200ac */
    "\tbeq $2,$0,.L8014FCFC\n"   /* 8014FC48: 1040002c */
    "\tori $12,$0,0x1\n"   /* 8014FC4C: 340c0001 */
    "\tlw $18,8($2)\n"   /* 8014FC50: 8c520008 */
    "\tlw $19,12($2)\n"   /* 8014FC54: 8c53000c */
    "\tsll $17,$17,0x10\n"   /* 8014FC58: 00118c00 */
    "\tand $18,$18,$19\n"   /* 8014FC5C: 02539024 */
    "\tlw $20,8($16)\n"   /* 8014FC60: 8e140008 */
    "\tbeq $18,$0,.L8014FCEC\n"   /* 8014FC64: 12400021 */
    "\tor $12,$12,$17\n"   /* 8014FC68: 01916025 */
    "\tbeq $20,$2,.L8014FCEC\n"   /* 8014FC6C: 1282001f */
    "\tsll $0,$0,0x0\n"   /* 8014FC70: 00000000 */
    "\tlw $17,32($16)\n"   /* 8014FC74: 8e110020 */
    "\taddiu $18,$2,128\n"   /* 8014FC78: 24520080 */
    "\tsw $18,0($17)\n"   /* 8014FC7C: ae320000 */
    "\tlw $19,0($18)\n"   /* 8014FC80: 8e530000 */
    "\tlw $21,4($18)\n"   /* 8014FC84: 8e550004 */
    "\tlw $22,8($18)\n"   /* 8014FC88: 8e560008 */
    "\tlw $23,12($18)\n"   /* 8014FC8C: 8e57000c */
    "\tlw $3,16($18)\n"   /* 8014FC90: 8e430010 */
    "\tctc2 $19,$0\n"   /* 8014FC94: 48d30000 */
    "\tctc2 $21,$1\n"   /* 8014FC98: 48d50800 */
    "\tctc2 $22,$2\n"   /* 8014FC9C: 48d61000 */
    "\tctc2 $23,$3\n"   /* 8014FCA0: 48d71800 */
    "\tctc2 $3,$4\n"   /* 8014FCA4: 48c32000 */
    "\tlw $19,168($20)\n"   /* 8014FCA8: 8e9300a8 */
    "\tlw $21,172($20)\n"   /* 8014FCAC: 8e9500ac */
    "\tlw $23,176($20)\n"   /* 8014FCB0: 8e9700b0 */
    "\tlw $4,168($2)\n"   /* 8014FCB4: 8c4400a8 */
    "\tlw $5,172($2)\n"   /* 8014FCB8: 8c4500ac */
    "\tlw $6,176($2)\n"   /* 8014FCBC: 8c4600b0 */
    "\tlw $8,284($2)\n"   /* 8014FCC0: 8c48011c */
    "\tsubu $4,$19,$4\n"   /* 8014FCC4: 02642023 */
    "\tsubu $5,$21,$5\n"   /* 8014FCC8: 02a52823 */
    "\tsubu $6,$23,$6\n"   /* 8014FCCC: 02e63023 */
    "\tjal hdpRotVecSplit\n"   /* 8014FCD0: 0c053be6 */
    "\taddu $23,$20,$0\n"   /* 8014FCD4: 0280b821 */
    "\taddu $9,$2,$0\n"   /* 8014FCD8: 00404821 */
    "\taddu $10,$3,$0\n"   /* 8014FCDC: 00605021 */
    "\taddu $11,$7,$0\n"   /* 8014FCE0: 00e05821 */
    "\tjal hdpWalk\n"   /* 8014FCE4: 0c053a98 */
    "\taddu $13,$0,$0\n"   /* 8014FCE8: 00006821 */
    ".L8014FCEC:\n"
    "\tlw $2,172($16)\n"   /* 8014FCEC: 8e0200ac */
    "\tlw $22,188($16)\n"   /* 8014FCF0: 8e1600bc */
    "\tj .L8014FC3C\n"   /* 8014FCF4: 08053f0f */
    "\tlw $2,288($2)\n"   /* 8014FCF8: 8c420120 */
    ".L8014FCFC:\n"
    "\tlw $17,112($16)\n"   /* 8014FCFC: 8e110070 */
    "\tlw $18,116($16)\n"   /* 8014FD00: 8e120074 */
    "\tlw $19,120($16)\n"   /* 8014FD04: 8e130078 */
    "\tlw $20,124($16)\n"   /* 8014FD08: 8e14007c */
    "\tlw $21,128($16)\n"   /* 8014FD0C: 8e150080 */
    "\tlw $22,132($16)\n"   /* 8014FD10: 8e160084 */
    "\tlw $23,136($16)\n"   /* 8014FD14: 8e170088 */
    "\tlw $29,140($16)\n"   /* 8014FD18: 8e1d008c */
    "\tlw $28,144($16)\n"   /* 8014FD1C: 8e1c0090 */
    "\tlw $4,148($16)\n"   /* 8014FD20: 8e040094 */
    "\tlw $5,152($16)\n"   /* 8014FD24: 8e050098 */
    "\tlw $31,156($16)\n"   /* 8014FD28: 8e1f009c */
    "\tlw $30,160($16)\n"   /* 8014FD2C: 8e1e00a0 */
    "\tlw $16,108($16)\n"   /* 8014FD30: 8e10006c */
    "\tjr $31\n"   /* 8014FD34: 03e00008 */
    "\tsll $0,$0,0x0\n"   /* 8014FD38: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size hdpPassRun,.-hdpPassRun\n");


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
 * in hdpEntryStack; depth capped at 200). */
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
