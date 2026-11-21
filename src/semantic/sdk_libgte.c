/* PsyQ SDK LIBGTE, reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * Sources:
 *  - rsin, sin_1, rcos, ratan2: adapted from sotn-decomp src/main/psxsdk/libgte
 *    (geo_00.c, geo_01.c, ratan.c), SPDX-License-Identifier: MIT.
 *  - MatrixNormal_0: adapted from psyz decomp/src/libgte/geo_03.c (MIT,
 *    decomp/LICENSE).
 *  - RotMatrix*_C, IsIdMatrix, EigenMatrix: Sony C with no public source,
 *    written by hand against the retail code.
 *  - The rest are Sony assembly (GTE transfers/commands, hand-scheduled
 *    integer code). Each is a C function whose body is the original
 *    instruction sequence as inline asm; no public source was used.
 *
 * Library data is not defined here; it is declared extern by retail address.
 *
 * Recipe: psyq3.3 cc1 + maspsx (aspsx 2.21), gp_size 0, no -fvolatile.
 * All 24 functions are byte-identical with retail.
 *
 * Inline-asm notes (maspsx behaviour this file relies on):
 *  - The assembler runs in noreorder mode; maspsx itself inserts the nop
 *    after a jal and load-delay nops, so asm bodies omit those.
 *  - ".set\tnoreorder" (with a TAB) also stops maspsx adding branch nops;
 *    such a block ends with ".set\treorder" (resets maspsx) followed by
 *    ".set noreorder" (with a space: seen only by the assembler).
 *  - Where retail fills the return delay slot, the last operation is written
 *    in C so the compiler schedules it into the slot. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    s16 m[3][3];
    s32 t[3];
} MATRIX;

typedef struct {
    s32 vx, vy, vz, pad;
} VECTOR;

typedef struct {
    s16 vx, vy, vz, pad;
} SVECTOR;

/* EigenMatrix switch jump table (retail address). */
#pragma tm2_rdata .tm2_800D0948

/* ---- library data (retail addresses) ---- */
extern s16 D_80176F9C[];   /* rsin_tbl */
extern s16 D_8017BAD0[];   /* ratan_tbl */

int sub_8013F260(int a);
int sub_8013F29C(int a);
int sub_8013F33C(int a);

/* rsin, rcos, sin_1, ratan2: adapted from sotn-decomp
 * src/main/psxsdk/libgte/geo_00.c (rsin, sin_1), geo_01.c (rcos), ratan.c
 * (ratan2); SPDX-License-Identifier: MIT. */

/* rsin: fixed-point sine (4096 = 1.0, angle 4096 = 360 degrees). */
int __attribute__((section(".text_8013F260"))) sub_8013F260(int a)
{
    if (a < 0) {
        return -sub_8013F29C(-a & 0xFFF);
    } else {
        return sub_8013F29C(a & 0xFFF);
    }
}

/* sin_1: sine of a 0..4095 angle from the quarter-wave table. */
int __attribute__((section(".text_8013F29C"))) sub_8013F29C(int a)
{
    if (a <= 0x800) {
        if (a <= 0x400) {
            return D_80176F9C[a];
        } else {
            return D_80176F9C[0x800 - a];
        }
    } else {
        if (a <= 0xC00) {
            return -D_80176F9C[a - 0x800];
        } else {
            return -D_80176F9C[0x1000 - a];
        }
    }
}

/* rcos: fixed-point cosine. */
int __attribute__((section(".text_8013F33C"))) sub_8013F33C(int a)
{
    if (a < 0) {
        a = -a;
    }
    a &= 0xFFF;

    if (a <= 0x800) {
        if (a <= 0x400) {
            return D_80176F9C[0x400 - a];
        } else {
            return -D_80176F9C[a - 0x400];
        }
    } else {
        if (a <= 0xC00) {
            return -D_80176F9C[0xC00 - a];
        } else {
            return D_80176F9C[a - 0xC00];
        }
    }
}

/* ratan2: fixed-point atan2(dy, dx) via the ratan table. */
long __attribute__((section(".text_80140E04"))) sub_80140E04(long dx, long dy)
{
    long ret;
    int flag0;
    int flag1;
    int lookup;

    flag0 = 0;
    flag1 = 0;
    if (dy < 0) {
        flag0 = 1;
        dy = -dy;
    }
    if (dx < 0) {
        flag1 = 1;
        dx = -dx;
    }
    if ((dy == 0) && (dx == 0)) {
        return 0;
    }
    if (dx < dy) {
        if (dx & 0x7FE00000) {
            lookup = (dx / (dy >> 10));
        } else {
            lookup = ((dx << 10) / dy);
        }
        ret = D_8017BAD0[lookup];
    } else {
        if (dy & 0x7FE00000) {
            lookup = (dy / (dx >> 10));
        } else {
            lookup = ((dy << 10) / dx);
        }
        ret = 0x400 - D_8017BAD0[lookup];
    }
    if (flag0)
        ret = 0x800 - ret;
    if (flag1)
        ret = -ret;
    return ret;
}


/* RotMatrixX_C / Y_C / Z_C: rotate matrix m by angle r about one axis
 * (C versions; written by hand against the retail code). */
#define ROT_AXIS(m, c, s, a, b) \
    { \
        short tmp[3][3]; \
        tmp[a][0] = (c * m->m[a][0] - s * m->m[b][0]) >> 12; \
        tmp[a][1] = (c * m->m[a][1] - s * m->m[b][1]) >> 12; \
        tmp[a][2] = (c * m->m[a][2] - s * m->m[b][2]) >> 12; \
        m->m[b][0] = (s * m->m[a][0] + c * m->m[b][0]) >> 12; \
        m->m[b][1] = (s * m->m[a][1] + c * m->m[b][1]) >> 12; \
        m->m[b][2] = (s * m->m[a][2] + c * m->m[b][2]) >> 12; \
        m->m[a][0] = tmp[a][0]; \
        m->m[a][1] = tmp[a][1]; \
        m->m[a][2] = tmp[a][2]; \
    }

/* RotMatrixX_C: multiply m by a rotation of r about X (rows 1,2). */
MATRIX * __attribute__((section(".text_8013EE04"))) sub_8013EE04(short r, MATRIX *m)
{
    int c, s;
    c = sub_8013F33C(r);
    s = sub_8013F260(r);
    ROT_AXIS(m, c, s, 1, 2);
    return m;
}

/* RotMatrixY_C: multiply m by a rotation of r about Y (rows 0,2). */
MATRIX * __attribute__((section(".text_8013EF78"))) sub_8013EF78(short r, MATRIX *m)
{
    int c, s;
    c = sub_8013F33C(r);
    s = -sub_8013F260(r);
    ROT_AXIS(m, c, s, 0, 2);
    return m;
}

/* RotMatrixZ_C: multiply m by a rotation of r about Z (rows 0,1). */
MATRIX * __attribute__((section(".text_8013F0EC"))) sub_8013F0EC(short r, MATRIX *m)
{
    int c, s;
    c = sub_8013F33C(r);
    s = sub_8013F260(r);
    ROT_AXIS(m, c, s, 0, 1);
    return m;
}

/* RotMatrixZYX_C: build m from Euler angles r (C version). */
MATRIX * __attribute__((section(".text_8013E994"))) sub_8013E994(SVECTOR *r, MATRIX *m)
{
    int c0, c1, c2, s0, s1, s2, ss, cs;
    c0 = sub_8013F33C(r->vx);
    c1 = sub_8013F33C(r->vy);
    c2 = sub_8013F33C(r->vz);
    s0 = sub_8013F260(r->vx);
    s1 = sub_8013F260(r->vy);
    s2 = sub_8013F260(r->vz);
    m->m[0][0] = (c1 * c2) >> 12;
    ss = (s0 * s1) >> 12;
    m->m[0][1] = ((ss * c2) >> 12) - ((s2 * c0) >> 12);
    cs = (s1 * c0) >> 12;
    m->m[0][2] = ((cs * c2) >> 12) + ((s0 * s2) >> 12);
    m->m[1][0] = (s2 * c1) >> 12;
    m->m[1][1] = ((ss * s2) >> 12) + ((c0 * c2) >> 12);
    m->m[1][2] = ((cs * s2) >> 12) - ((s0 * c2) >> 12);
    m->m[2][0] = -s1;
    m->m[2][1] = (s0 * c1) >> 12;
    m->m[2][2] = (c0 * c1) >> 12;
    return m;
}

/* RotMatrix_C: build m from Euler angles r, X then Y then Z (C version). */
MATRIX * __attribute__((section(".text_8013EB0C"))) sub_8013EB0C(SVECTOR *r, MATRIX *m)
{
    int c0, c1, c2, s0, s1, s2, ms1, x, y;
    c0 = sub_8013F33C(r->vx);
    c1 = sub_8013F33C(r->vy);
    c2 = sub_8013F33C(r->vz);
    s0 = sub_8013F260(r->vx);
    s1 = sub_8013F260(r->vy);
    s2 = sub_8013F260(r->vz);
    m->m[0][0] = (c2 * c1) >> 12;
    ms1 = -s1;
    m->m[1][0] = ((s2 * c0) >> 12) - (((x = (c2 * ms1) >> 12) * s0) >> 12);
    m->m[2][0] = ((s2 * s0) >> 12) + ((x * c0) >> 12);
    m->m[0][1] = -(s2 * c1) >> 12;
    m->m[1][1] = ((c2 * c0) >> 12) + (((y = (s2 * ms1) >> 12) * s0) >> 12);
    m->m[2][1] = ((c2 * s0) >> 12) - ((y * c0) >> 12);
    m->m[0][2] = s1;
    m->m[1][2] = -(c1 * s0) >> 12;
    m->m[2][2] = (c1 * c0) >> 12;
    return m;
}

/* RotMatrixC: build m from Euler angles r (C version, alternate order). */
MATRIX * __attribute__((section(".text_8013EC8C"))) sub_8013EC8C(SVECTOR *r, MATRIX *m)
{
    int c0, c1, c2, s0, s1, s2, p, q;
    c0 = sub_8013F33C(r->vx);
    c1 = sub_8013F33C(r->vy);
    c2 = sub_8013F33C(r->vz);
    s0 = sub_8013F260(r->vx);
    s1 = sub_8013F260(r->vy);
    s2 = sub_8013F260(r->vz);
    m->m[0][0] = ((c1 * c2) >> 12) + ((((p = (s1 * s0) >> 12)) * s2) >> 12);
    m->m[1][0] = (s2 * c0) >> 12;
    m->m[2][0] = -((s1 * c2) >> 12) + (((q = (c1 * s0) >> 12) * s2) >> 12);
    m->m[0][1] = -((c1 * s2) >> 12) + ((p * c2) >> 12);
    m->m[1][1] = (c2 * c0) >> 12;
    m->m[2][1] = ((s1 * s2) >> 12) + ((q * c2) >> 12);
    m->m[0][2] = (s1 * c0) >> 12;
    m->m[1][2] = -s0;
    m->m[2][2] = (c1 * c0) >> 12;
    return m;
}

/* ---- Hand-written GTE routines. Sony wrote these in assembly; each is a C
 * function whose body is the original instruction sequence (GTE transfers and
 * commands as inline asm). Where retail fills the return delay slot, the last
 * operation is expressed in C so the compiler schedules it there. ---- */

/* VectorNormal: normalise long vector v0 into v1 (sub_8013FE60 does the
 * work in t0..t2; ra is kept in a3 across the internal call). */
void __attribute__((section(".text_8013FE04"))) sub_8013FE04(VECTOR *v0, VECTOR *v1)
{
    register s32 x __asm__("$8");
    register s32 y __asm__("$9");
    register s32 z __asm__("$10");
    __asm__ volatile (
        "lw $8,0($4)\n\t"
        "lw $9,4($4)\n\t"
        "lw $10,8($4)\n\t"
        "move $7,$31\n\t"
        "jal sub_8013FE60\n\t"
        "move $31,$7\n\t"
        ""
        : "=r"(x), "=r"(y), "=r"(z) : "r"(v0)
        : "$1", "$2", "$3", "$7", "$11", "$12", "$13", "$14", "memory");
    v1->vx = x;
    v1->vy = y;
    v1->vz = z;
}

/* RotTrans: v1 = R*v0 + T (GTE RT), returns the GTE FLAG in *flag. */
void __attribute__((section(".text_80140544"))) sub_80140544(SVECTOR *v0, VECTOR *v1, long *flag)
{
    register long f __asm__("$2");
    __asm__ volatile (
        "lwc2 $0,0(%1)\n\t"
        "lwc2 $1,4(%1)\n\t"
        "nop\n\t"
        "cop2 0x480012\n\t"
        "swc2 $25,0(%2)\n\t"
        "swc2 $26,4(%2)\n\t"
        "swc2 $27,8(%2)\n\t"
        "cfc2 $2,$31\n\t"
        ""
        : "=r"(f) : "r"(v0), "r"(v1) : "memory");
    *flag = f;
}

/* NormalColorCol: light normal v0 with primary colour v1 (GTE NCCS) -> v2. */
void __attribute__((section(".text_80140670"))) sub_80140670(SVECTOR *v0, void *v1, void *v2)
{
    __asm__ volatile (
        "lwc2 $0,0(%0)\n\t"
        "lwc2 $1,4(%0)\n\t"
        "lwc2 $6,0(%1)\n\t"
        "nop\n\t"
        "cop2 0x108041b\n\t"
        "swc2 $22,0(%2)\n\t"
        ""
        : : "r"(v0), "r"(v1), "r"(v2) : "memory");
}

/* OuterProduct12: v2 = v0 x v1 via the GTE OP command (rotation matrix
 * diagonal saved and restored). */
void __attribute__((section(".text_801408B8"))) sub_801408B8(VECTOR *v0, VECTOR *v1, VECTOR *v2)
{
    __asm__ volatile (
        "cfc2 $13,$0\n\t"
        "cfc2 $14,$2\n\t"
        "cfc2 $15,$4\n\t"
        "lw $8,0(%0)\n\t"
        "lw $9,4(%0)\n\t"
        "lw $10,8(%0)\n\t"
        "ctc2 $8,$0\n\t"
        "ctc2 $9,$2\n\t"
        "ctc2 $10,$4\n\t"
        "lwc2 $11,8(%1)\n\t"
        "lwc2 $9,0(%1)\n\t"
        "lwc2 $10,4(%1)\n\t"
        "nop\n\t"
        "cop2 0x178000c\n\t"
        "swc2 $25,0(%2)\n\t"
        "swc2 $26,4(%2)\n\t"
        "swc2 $27,8(%2)\n\t"
        "ctc2 $13,$0\n\t"
        "ctc2 $14,$2\n\t"
        "ctc2 $15,$4\n\t"
        ""
        : : "r"(v0), "r"(v1), "r"(v2) : "$8", "$9", "$10", "$13", "$14", "$15", "memory");
}

/* TransposeMatrix: m1 = transpose(m0); returns m1. */
MATRIX * __attribute__((section(".text_80140984"))) sub_80140984(MATRIX *m0, MATRIX *m1)
{
    register MATRIX *ret __asm__("$2");
    register s32 t1 __asm__("$9");
    __asm__ volatile (
        "lh $9,0(%2)\n\t"
        "move $2,%3\n\t"
        "sh $9,0(%3)\n\t"
        "lh $10,6(%2)\n\t"
        "lh $9,12(%2)\n\t"
        "sh $10,2(%3)\n\t"
        "lh $11,2(%2)\n\t"
        "sh $9,4(%3)\n\t"
        "lh $10,8(%2)\n\t"
        "sh $11,6(%3)\n\t"
        "lh $9,14(%2)\n\t"
        "sh $10,8(%3)\n\t"
        "lh $11,4(%2)\n\t"
        "sh $9,10(%3)\n\t"
        "lh $10,10(%2)\n\t"
        "sh $11,12(%3)\n\t"
        "lh $9,16(%2)\n\t"
        "sh $10,14(%3)"
        : "=r"(ret), "=r"(t1) : "r"(m0), "r"(m1) : "$10", "$11", "memory");
    m1->m[2][2] = t1;
    return ret;
}

/* MulRotMatrix0: m1 = R * m0 (column by column through GTE MVMVA);
 * returns m1. */
MATRIX * __attribute__((section(".text_80149970"))) sub_80149970(MATRIX *m0, MATRIX *m1)
{
    register MATRIX *ret __asm__("$2");
    __asm__ volatile (
        "lhu $8,0(%1)\n\t"
        "lw $9,4(%1)\n\t"
        "lw $10,12(%1)\n\t"
        "lui $1,0xffff\n\t"
        "and $9,$9,$1\n\t"
        "or $8,$8,$9\n\t"
        "mtc2 $8,$0\n\t"
        "mtc2 $10,$1\n\t"
        "nop\n\t"
        "cop2 0x486012\n\t"
        "lhu $8,2(%1)\n\t"
        "lw $9,8(%1)\n\t"
        "lh $10,14(%1)\n\t"
        "sll $9,$9,0x10\n\t"
        "or $8,$8,$9\n\t"
        "mfc2 $11,$9\n\t"
        "mfc2 $12,$10\n\t"
        "mfc2 $13,$11\n\t"
        "mtc2 $8,$0\n\t"
        "mtc2 $10,$1\n\t"
        "nop\n\t"
        "cop2 0x486012\n\t"
        "lhu $8,4(%1)\n\t"
        "lw $9,8(%1)\n\t"
        "lw $10,16(%1)\n\t"
        "lui $1,0xffff\n\t"
        "and $9,$9,$1\n\t"
        "or $8,$8,$9\n\t"
        "mfc2 $14,$9\n\t"
        "mfc2 $15,$10\n\t"
        "mfc2 $24,$11\n\t"
        "mtc2 $8,$0\n\t"
        "mtc2 $10,$1\n\t"
        "nop\n\t"
        "cop2 0x486012\n\t"
        "andi $11,$11,0xffff\n\t"
        "sll $14,$14,0x10\n\t"
        "or $14,$14,$11\n\t"
        "sw $14,0(%2)\n\t"
        "andi $13,$13,0xffff\n\t"
        "sll $24,$24,0x10\n\t"
        "or $24,$24,$13\n\t"
        "sw $24,12(%2)\n\t"
        "mfc2 $8,$9\n\t"
        "mfc2 $9,$10\n\t"
        "andi $8,$8,0xffff\n\t"
        "sll $12,$12,0x10\n\t"
        "or $8,$8,$12\n\t"
        "sw $8,4(%2)\n\t"
        "andi $15,$15,0xffff\n\t"
        "sll $9,$9,0x10\n\t"
        "or $9,$9,$15\n\t"
        "sw $9,8(%2)\n\t"
        "swc2 $11,16(%2)\n\t"
        "move $2,%2\n\t"
        ""
        : "=r"(ret) : "r"(m0), "r"(m1)
        : "$1", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$24", "memory");
    return ret;
}

/* RotTransPers4: perspective-transform four vertices (RTPT + RTPS);
 * returns OTZ (SZ3 / 4). */
long __attribute__((section(".text_8014D680"))) sub_8014D680(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
    SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *flag)
{
    register long z __asm__("$2");
    __asm__ volatile (
        "lwc2 $0,0($4)\n\t"
        "lwc2 $1,4($4)\n\t"
        "lwc2 $2,0($5)\n\t"
        "lwc2 $3,4($5)\n\t"
        "lwc2 $4,0($6)\n\t"
        "lwc2 $5,4($6)\n\t"
        "nop\n\t"
        "cop2 0x280030\n\t"
        "lw $8,16($29)\n\t"
        "lw $9,20($29)\n\t"
        "lw $10,24($29)\n\t"
        "swc2 $12,0($8)\n\t"
        "swc2 $13,0($9)\n\t"
        "swc2 $14,0($10)\n\t"
        "cfc2 $3,$31\n\t"
        "lwc2 $0,0($7)\n\t"
        "lwc2 $1,4($7)\n\t"
        "nop\n\t"
        "cop2 0x180001\n\t"
        "lw $8,28($29)\n\t"
        "lw $9,32($29)\n\t"
        "lw $10,36($29)\n\t"
        "swc2 $14,0($8)\n\t"
        "swc2 $8,0($9)\n\t"
        "cfc2 $8,$31\n\t"
        "mfc2 $2,$19\n\t"
        "or $8,$8,$3\n\t"
        "sw $8,0($10)\n\t"
        ""
        : "=r"(z) : "r"(v0), "r"(v1), "r"(v2), "r"(v3)
        : "$3", "$8", "$9", "$10", "memory");
    return z >> 2;
}

void sub_8013FE04(VECTOR *v0, VECTOR *v1);
void sub_801408B8(VECTOR *v0, VECTOR *v1, VECTOR *v2);

/* IsIdMatrix: 1 if m is (close to) the identity matrix (sum |m - I| < 20). */
int __attribute__((section(".text_8013F794"))) sub_8013F794(MATRIX *m)
{
    int d[3][3];
    int i, j, sum;

    d[0][0] = m->m[0][0] - 4096;
    d[0][1] = m->m[0][1];
    d[0][2] = m->m[0][2];
    d[1][0] = m->m[1][0];
    d[1][1] = m->m[1][1] - 4096;
    d[1][2] = m->m[1][2];
    d[2][0] = m->m[2][0];
    d[2][1] = m->m[2][1];
    d[2][2] = m->m[2][2] - 4096;
    sum = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (d[i][j] < 0) {
                d[i][j] = -d[i][j];
            }
            sum += d[i][j];
        }
    }
    return sum < 20;
}

/* MatrixNormal_0: re-orthonormalise m into n from rows 0 and 1.
 * Adapted from psyz decomp/src/libgte/geo_03.c (SPDX-License-Identifier: MIT). */
void __attribute__((section(".text_8013F864"))) sub_8013F864(MATRIX *m, MATRIX *n)
{
    VECTOR v0, v1, v2;

    v0.vx = m->m[0][0];
    v0.vy = m->m[0][1];
    v0.vz = m->m[0][2];

    v1.vx = m->m[1][0];
    v1.vy = m->m[1][1];
    v1.vz = m->m[1][2];

    sub_801408B8(&v0, &v1, &v2);
    sub_801408B8(&v1, &v2, &v0);

    sub_8013FE04(&v0, &v0);
    sub_8013FE04(&v1, &v1);
    sub_8013FE04(&v2, &v2);

    n->m[0][0] = v0.vx;
    n->m[0][1] = v0.vy;
    n->m[0][2] = v0.vz;

    n->m[1][0] = v1.vx;
    n->m[1][1] = v1.vy;
    n->m[1][2] = v1.vz;

    n->m[2][0] = v2.vx;
    n->m[2][1] = v2.vy;
    n->m[2][2] = v2.vz;
}

/* InitGeom / _patch_gte: GTE initialisation. Sony assembly with no C
 * equivalent (ra parked in library data, raw cop0/cop2 control writes, a
 * jalr through the BIOS A0 table); kept as the original sequence. maspsx
 * supplies the nop after each jal; a jump whose delay slot retail fills is
 * emitted as a .word so maspsx leaves the slot alone. */
extern s32 D_801777A0;          /* InitGeom saved ra */
extern s32 D_801810C0;          /* _patch_gte saved ra */
extern u32 D_80140FF4[];        /* GTE exception-handler patch code (start) */
extern u32 D_8014102C[];        /* (end) */
extern int EnterCriticalSection(void);
extern int ExitCriticalSection(void);
extern void FlushCache(void);
void sub_80140F8C(void);

/* InitGeom: patch the kernel, enable cop2, set default GTE state
 * (ZSF3/ZSF4, H=1000, DQA/DQB, offset 0,0). */
void __attribute__((section(".text_8013FBC4"))) sub_8013FBC4(void)
{
    __asm__ volatile (
        "sw $31,D_801777A0\n\t"         /* as: lui at / sw ra,lo(at) */
        "jal sub_80140F8C\n\t"
        "lui $31,%%hi(D_801777A0)\n\t"
        "lw $31,%%lo(D_801777A0)($31)\n\t"
        "nop\n\t"
        "mfc0 $2,$12\n\t"
        "lui $3,0x4000\n\t"
        "or $2,$2,$3\n\t"
        "mtc0 $2,$12\n\t"
        "nop\n\t"
        "addiu $8,$0,341\n\t"
        "ctc2 $8,$29\n\t"
        "nop\n\t"
        "addiu $8,$0,256\n\t"
        "ctc2 $8,$30\n\t"
        "nop\n\t"
        "addiu $8,$0,1000\n\t"
        "ctc2 $8,$26\n\t"
        "nop\n\t"
        "addiu $8,$0,-4194\n\t"
        "ctc2 $8,$27\n\t"
        "nop\n\t"
        "lui $8,0x140\n\t"
        "ctc2 $8,$28\n\t"
        "nop\n\t"
        "ctc2 $0,$24\n\t"
        "ctc2 $0,$25\n\t"
        "nop"
        : : : "$1", "$2", "$3", "$8", "memory");
}

/* _patch_gte: copy the GTE patch into the kernel exception handler
 * (address from BIOS A0h function 56h), inside a critical section. */
void __attribute__((section(".text_80140F8C"))) sub_80140F8C(void)
{
    __asm__ volatile (
        "sw $31,D_801810C0\n\t"         /* as: lui at / sw ra,lo(at) */
        "jal EnterCriticalSection\n\t"
        "addiu $10,$0,176\n\t"
        ".word 0x0140f809\n\t"          /* jalr $10 */
        "addiu $9,$0,86\n\t"
        "lui $10,%%hi(D_80140FF4)\n\t"
        "lui $9,%%hi(D_8014102C)\n\t"
        "lw $2,24($2)\n\t"
        "addiu $10,$10,%%lo(D_80140FF4)\n\t"
        "addiu $9,$9,%%lo(D_8014102C)\n\t"
        "lw $3,0($10)\n\t"
        "addiu $10,$10,4\n\t"
        "addiu $2,$2,4\n\t"
        ".word 0x1549fffc\n\t"          /* bne $10,$9,.-12 */
        "sw $3,-4($2)\n\t"
        "jal FlushCache\n\t"
        "jal ExitCriticalSection\n\t"
        "lui $31,%%hi(D_801810C0)\n\t"
        "lw $31,%%lo(D_801810C0)($31)\n\t"
        "nop"
        : : : "$1", "$2", "$3", "$9", "$10", "memory");
}

/* RotMatrixX: multiply m by a rotation of r about X, using the packed
 * sin/cos table (Sony assembly, kept as the original sequence). */
extern s32 D_80177AD0[];        /* rcossin_tbl: (cos << 16) | sin per angle */
MATRIX * __attribute__((section(".text_80140C64"))) sub_80140C64(long r, MATRIX *m)
{
    register MATRIX *ret __asm__("$2");
    __asm__ volatile (
        ".set\tnoreorder\n\t"
        "move $15,$4\n\t"   /* 80140C64: 00807821 */
        "move $2,$5\n\t"   /* 80140C68: 00a01021 */
        "bgez $15,.Lgte_80140CA8\n\t"   /* 80140C6C: 05e1000e */
        "andi $25,$15,0xfff\n\t"   /* 80140C70: 31f90fff */
        "negu $15,$15\n\t"   /* 80140C74: 000f7823 */
        "bgez $15,.Lgte_80140C80\n\t"   /* 80140C78: 05e10001 */
        "andi $15,$15,0xfff\n\t"   /* 80140C7C: 31ef0fff */
        ".Lgte_80140C80:\n\t"
        "sll $24,$15,0x2\n\t"   /* 80140C80: 000fc080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140C84: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140C88: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140C8C: 8f397ad0 */
        "nop\n\t"   /* 80140C90: 00000000 */
        "sll $14,$25,0x10\n\t"   /* 80140C94: 00197400 */
        "sra $14,$14,0x10\n\t"   /* 80140C98: 000e7403 */
        "negu $9,$14\n\t"   /* 80140C9C: 000e4823 */
        "j .Lgte_80140CC8\n\t"   /* 80140CA0: 08050332 */
        "sra $8,$25,0x10\n\t"   /* 80140CA4: 00194403 */
        ".Lgte_80140CA8:\n\t"
        "sll $24,$25,0x2\n\t"   /* 80140CA8: 0019c080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140CAC: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140CB0: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140CB4: 8f397ad0 */
        "nop\n\t"   /* 80140CB8: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140CBC: 0019c400 */
        "sra $9,$24,0x10\n\t"   /* 80140CC0: 00184c03 */
        "sra $8,$25,0x10\n\t"   /* 80140CC4: 00194403 */
        ".Lgte_80140CC8:\n\t"
        "lh $10,6($5)\n\t"   /* 80140CC8: 84aa0006 */
        "lh $13,12($5)\n\t"   /* 80140CCC: 84ad000c */
        "multu $8,$10\n\t"   /* 80140CD0: 010a0019 */
        "lh $11,8($5)\n\t"   /* 80140CD4: 84ab0008 */
        "lh $14,14($5)\n\t"   /* 80140CD8: 84ae000e */
        "mflo $24\n\t"   /* 80140CDC: 0000c012 */
        "lh $12,10($5)\n\t"   /* 80140CE0: 84ac000a */
        "lh $15,16($5)\n\t"   /* 80140CE4: 84af0010 */
        "multu $9,$13\n\t"   /* 80140CE8: 012d0019 */
        "nop\n\t"   /* 80140CEC: 00000000 */
        "nop\n\t"   /* 80140CF0: 00000000 */
        "mflo $25\n\t"   /* 80140CF4: 0000c812 */
        "subu $25,$24,$25\n\t"   /* 80140CF8: 0319c823 */
        "sra $24,$25,0xc\n\t"   /* 80140CFC: 0019c303 */
        "multu $8,$11\n\t"   /* 80140D00: 010b0019 */
        "sh $24,6($5)\n\t"   /* 80140D04: a4b80006 */
        "nop\n\t"   /* 80140D08: 00000000 */
        "mflo $24\n\t"   /* 80140D0C: 0000c012 */
        "nop\n\t"   /* 80140D10: 00000000 */
        "nop\n\t"   /* 80140D14: 00000000 */
        "multu $9,$14\n\t"   /* 80140D18: 012e0019 */
        "nop\n\t"   /* 80140D1C: 00000000 */
        "nop\n\t"   /* 80140D20: 00000000 */
        "mflo $25\n\t"   /* 80140D24: 0000c812 */
        "subu $25,$24,$25\n\t"   /* 80140D28: 0319c823 */
        "sra $24,$25,0xc\n\t"   /* 80140D2C: 0019c303 */
        "multu $8,$12\n\t"   /* 80140D30: 010c0019 */
        "sh $24,8($5)\n\t"   /* 80140D34: a4b80008 */
        "nop\n\t"   /* 80140D38: 00000000 */
        "mflo $24\n\t"   /* 80140D3C: 0000c012 */
        "nop\n\t"   /* 80140D40: 00000000 */
        "nop\n\t"   /* 80140D44: 00000000 */
        "multu $9,$15\n\t"   /* 80140D48: 012f0019 */
        "nop\n\t"   /* 80140D4C: 00000000 */
        "nop\n\t"   /* 80140D50: 00000000 */
        "mflo $25\n\t"   /* 80140D54: 0000c812 */
        "subu $25,$24,$25\n\t"   /* 80140D58: 0319c823 */
        "sra $24,$25,0xc\n\t"   /* 80140D5C: 0019c303 */
        "multu $9,$10\n\t"   /* 80140D60: 012a0019 */
        "sh $24,10($5)\n\t"   /* 80140D64: a4b8000a */
        "nop\n\t"   /* 80140D68: 00000000 */
        "mflo $24\n\t"   /* 80140D6C: 0000c012 */
        "nop\n\t"   /* 80140D70: 00000000 */
        "nop\n\t"   /* 80140D74: 00000000 */
        "multu $8,$13\n\t"   /* 80140D78: 010d0019 */
        "nop\n\t"   /* 80140D7C: 00000000 */
        "nop\n\t"   /* 80140D80: 00000000 */
        "mflo $25\n\t"   /* 80140D84: 0000c812 */
        "addu $25,$24,$25\n\t"   /* 80140D88: 0319c821 */
        "sra $24,$25,0xc\n\t"   /* 80140D8C: 0019c303 */
        "multu $9,$11\n\t"   /* 80140D90: 012b0019 */
        "sh $24,12($5)\n\t"   /* 80140D94: a4b8000c */
        "nop\n\t"   /* 80140D98: 00000000 */
        "mflo $24\n\t"   /* 80140D9C: 0000c012 */
        "nop\n\t"   /* 80140DA0: 00000000 */
        "nop\n\t"   /* 80140DA4: 00000000 */
        "multu $8,$14\n\t"   /* 80140DA8: 010e0019 */
        "nop\n\t"   /* 80140DAC: 00000000 */
        "nop\n\t"   /* 80140DB0: 00000000 */
        "mflo $25\n\t"   /* 80140DB4: 0000c812 */
        "addu $25,$24,$25\n\t"   /* 80140DB8: 0319c821 */
        "sra $24,$25,0xc\n\t"   /* 80140DBC: 0019c303 */
        "multu $9,$12\n\t"   /* 80140DC0: 012c0019 */
        "sh $24,14($5)\n\t"   /* 80140DC4: a4b8000e */
        "nop\n\t"   /* 80140DC8: 00000000 */
        "mflo $24\n\t"   /* 80140DCC: 0000c012 */
        "nop\n\t"   /* 80140DD0: 00000000 */
        "nop\n\t"   /* 80140DD4: 00000000 */
        "multu $8,$15\n\t"   /* 80140DD8: 010f0019 */
        "nop\n\t"   /* 80140DDC: 00000000 */
        "nop\n\t"   /* 80140DE0: 00000000 */
        "mflo $25\n\t"   /* 80140DE4: 0000c812 */
        "addu $25,$24,$25\n\t"   /* 80140DE8: 0319c821 */
        "sra $24,$25,0xc\n\t"   /* 80140DEC: 0019c303 */
        "sh $24,16($5)\n\t"   /* 80140DF0: a4b80010 */
        ".set\treorder\n\t"
        ".set noreorder"
        : "=r"(ret) : "r"(r), "r"(m)
        : "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$24", "$25", "memory");
    return ret;
}

/* RotMatrixYXZ: build m from Euler angles r using the packed sin/cos table
 * (Sony assembly, kept as the original sequence). */
MATRIX * __attribute__((section(".text_801409D4"))) sub_801409D4(SVECTOR *r, MATRIX *m)
{
    register MATRIX *ret __asm__("$2");
    __asm__ volatile (
        ".set\tnoreorder\n\t"
        "lh $15,0($4)\n\t"   /* 801409D4: 848f0000 */
        "move $2,$5\n\t"   /* 801409D8: 00a01021 */
        "bgez $15,.Lgte_80140A18\n\t"   /* 801409DC: 05e1000e */
        "andi $25,$15,0xfff\n\t"   /* 801409E0: 31f90fff */
        "negu $15,$15\n\t"   /* 801409E4: 000f7823 */
        "bgez $15,.Lgte_801409F0\n\t"   /* 801409E8: 05e10001 */
        "andi $15,$15,0xfff\n\t"   /* 801409EC: 31ef0fff */
        ".Lgte_801409F0:\n\t"
        "sll $24,$15,0x2\n\t"   /* 801409F0: 000fc080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 801409F4: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 801409F8: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 801409FC: 8f397ad0 */
        "nop\n\t"   /* 80140A00: 00000000 */
        "sll $14,$25,0x10\n\t"   /* 80140A04: 00197400 */
        "sra $14,$14,0x10\n\t"   /* 80140A08: 000e7403 */
        "negu $11,$14\n\t"   /* 80140A0C: 000e5823 */
        "j .Lgte_80140A3C\n\t"   /* 80140A10: 0805028f */
        "sra $8,$25,0x10\n\t"   /* 80140A14: 00194403 */
        ".Lgte_80140A18:\n\t"
        "sll $24,$25,0x2\n\t"   /* 80140A18: 0019c080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140A1C: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140A20: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140A24: 8f397ad0 */
        "nop\n\t"   /* 80140A28: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140A2C: 0019c400 */
        "sra $11,$24,0x10\n\t"   /* 80140A30: 00185c03 */
        "negu $14,$11\n\t"   /* 80140A34: 000b7023 */
        "sra $8,$25,0x10\n\t"   /* 80140A38: 00194403 */
        ".Lgte_80140A3C:\n\t"
        "lh $15,2($4)\n\t"   /* 80140A3C: 848f0002 */
        "nop\n\t"   /* 80140A40: 00000000 */
        "bgez $15,.Lgte_80140A80\n\t"   /* 80140A44: 05e1000e */
        "andi $25,$15,0xfff\n\t"   /* 80140A48: 31f90fff */
        "negu $15,$15\n\t"   /* 80140A4C: 000f7823 */
        "bgez $15,.Lgte_80140A58\n\t"   /* 80140A50: 05e10001 */
        "andi $15,$15,0xfff\n\t"   /* 80140A54: 31ef0fff */
        ".Lgte_80140A58:\n\t"
        "sll $24,$15,0x2\n\t"   /* 80140A58: 000fc080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140A5C: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140A60: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140A64: 8f397ad0 */
        "nop\n\t"   /* 80140A68: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140A6C: 0019c400 */
        "sra $24,$24,0x10\n\t"   /* 80140A70: 0018c403 */
        "negu $12,$24\n\t"   /* 80140A74: 00186023 */
        "j .Lgte_80140AA0\n\t"   /* 80140A78: 080502a8 */
        "sra $9,$25,0x10\n\t"   /* 80140A7C: 00194c03 */
        ".Lgte_80140A80:\n\t"
        "sll $24,$25,0x2\n\t"   /* 80140A80: 0019c080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140A84: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140A88: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140A8C: 8f397ad0 */
        "nop\n\t"   /* 80140A90: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140A94: 0019c400 */
        "sra $12,$24,0x10\n\t"   /* 80140A98: 00186403 */
        "sra $9,$25,0x10\n\t"   /* 80140A9C: 00194c03 */
        ".Lgte_80140AA0:\n\t"
        "multu $12,$8\n\t"   /* 80140AA0: 01880019 */
        "lh $15,4($4)\n\t"   /* 80140AA4: 848f0004 */
        "sh $14,10($5)\n\t"   /* 80140AA8: a4ae000a */
        "mflo $24\n\t"   /* 80140AAC: 0000c012 */
        "sra $14,$24,0xc\n\t"   /* 80140AB0: 00187303 */
        "nop\n\t"   /* 80140AB4: 00000000 */
        "multu $9,$8\n\t"   /* 80140AB8: 01280019 */
        "sh $14,4($5)\n\t"   /* 80140ABC: a4ae0004 */
        "bgez $15,.Lgte_80140B08\n\t"   /* 80140AC0: 05e10011 */
        "andi $25,$15,0xfff\n\t"   /* 80140AC4: 31f90fff */
        "mflo $24\n\t"   /* 80140AC8: 0000c012 */
        "sra $14,$24,0xc\n\t"   /* 80140ACC: 00187303 */
        "sh $14,16($5)\n\t"   /* 80140AD0: a4ae0010 */
        "negu $15,$15\n\t"   /* 80140AD4: 000f7823 */
        "bgez $15,.Lgte_80140AE0\n\t"   /* 80140AD8: 05e10001 */
        "andi $15,$15,0xfff\n\t"   /* 80140ADC: 31ef0fff */
        ".Lgte_80140AE0:\n\t"
        "sll $24,$15,0x2\n\t"   /* 80140AE0: 000fc080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140AE4: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140AE8: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140AEC: 8f397ad0 */
        "nop\n\t"   /* 80140AF0: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140AF4: 0019c400 */
        "sra $24,$24,0x10\n\t"   /* 80140AF8: 0018c403 */
        "negu $13,$24\n\t"   /* 80140AFC: 00186823 */
        "j .Lgte_80140B34\n\t"   /* 80140B00: 080502cd */
        "sra $10,$25,0x10\n\t"   /* 80140B04: 00195403 */
        ".Lgte_80140B08:\n\t"
        "mflo $15\n\t"   /* 80140B08: 00007812 */
        "sra $14,$15,0xc\n\t"   /* 80140B0C: 000f7303 */
        "sh $14,16($5)\n\t"   /* 80140B10: a4ae0010 */
        "sll $24,$25,0x2\n\t"   /* 80140B14: 0019c080 */
        "lui $25,%%hi(D_80177AD0)\n\t"   /* 80140B18: 3c198017 */
        "addu $25,$25,$24\n\t"   /* 80140B1C: 0338c821 */
        "lw $25,%%lo(D_80177AD0)($25)\n\t"   /* 80140B20: 8f397ad0 */
        "nop\n\t"   /* 80140B24: 00000000 */
        "sll $24,$25,0x10\n\t"   /* 80140B28: 0019c400 */
        "sra $13,$24,0x10\n\t"   /* 80140B2C: 00186c03 */
        "sra $10,$25,0x10\n\t"   /* 80140B30: 00195403 */
        ".Lgte_80140B34:\n\t"
        "multu $13,$8\n\t"   /* 80140B34: 01a80019 */
        "nop\n\t"   /* 80140B38: 00000000 */
        "nop\n\t"   /* 80140B3C: 00000000 */
        "mflo $15\n\t"   /* 80140B40: 00007812 */
        "sra $14,$15,0xc\n\t"   /* 80140B44: 000f7303 */
        "sh $14,6($5)\n\t"   /* 80140B48: a4ae0006 */
        "multu $10,$8\n\t"   /* 80140B4C: 01480019 */
        "nop\n\t"   /* 80140B50: 00000000 */
        "nop\n\t"   /* 80140B54: 00000000 */
        "mflo $15\n\t"   /* 80140B58: 00007812 */
        "sra $14,$15,0xc\n\t"   /* 80140B5C: 000f7303 */
        "nop\n\t"   /* 80140B60: 00000000 */
        "multu $12,$11\n\t"   /* 80140B64: 018b0019 */
        "sh $14,8($5)\n\t"   /* 80140B68: a4ae0008 */
        "nop\n\t"   /* 80140B6C: 00000000 */
        "mflo $15\n\t"   /* 80140B70: 00007812 */
        "sra $24,$15,0xc\n\t"   /* 80140B74: 000fc303 */
        "nop\n\t"   /* 80140B78: 00000000 */
        "multu $24,$13\n\t"   /* 80140B7C: 030d0019 */
        "nop\n\t"   /* 80140B80: 00000000 */
        "nop\n\t"   /* 80140B84: 00000000 */
        "mflo $15\n\t"   /* 80140B88: 00007812 */
        "sra $14,$15,0xc\n\t"   /* 80140B8C: 000f7303 */
        "nop\n\t"   /* 80140B90: 00000000 */
        "multu $9,$10\n\t"   /* 80140B94: 012a0019 */
        "nop\n\t"   /* 80140B98: 00000000 */
        "nop\n\t"   /* 80140B9C: 00000000 */
        "mflo $15\n\t"   /* 80140BA0: 00007812 */
        "sra $25,$15,0xc\n\t"   /* 80140BA4: 000fcb03 */
        "addu $15,$25,$14\n\t"   /* 80140BA8: 032e7821 */
        "multu $9,$13\n\t"   /* 80140BAC: 012d0019 */
        "sh $15,0($5)\n\t"   /* 80140BB0: a4af0000 */
        "nop\n\t"   /* 80140BB4: 00000000 */
        "mflo $14\n\t"   /* 80140BB8: 00007012 */
        "sra $25,$14,0xc\n\t"   /* 80140BBC: 000ecb03 */
        "negu $15,$25\n\t"   /* 80140BC0: 00197823 */
        "multu $24,$10\n\t"   /* 80140BC4: 030a0019 */
        "nop\n\t"   /* 80140BC8: 00000000 */
        "nop\n\t"   /* 80140BCC: 00000000 */
        "mflo $14\n\t"   /* 80140BD0: 00007012 */
        "sra $25,$14,0xc\n\t"   /* 80140BD4: 000ecb03 */
        "addu $14,$15,$25\n\t"   /* 80140BD8: 01f97021 */
        "multu $9,$11\n\t"   /* 80140BDC: 012b0019 */
        "sh $14,2($5)\n\t"   /* 80140BE0: a4ae0002 */
        "nop\n\t"   /* 80140BE4: 00000000 */
        "mflo $15\n\t"   /* 80140BE8: 00007812 */
        "sra $24,$15,0xc\n\t"   /* 80140BEC: 000fc303 */
        "nop\n\t"   /* 80140BF0: 00000000 */
        "multu $24,$10\n\t"   /* 80140BF4: 030a0019 */
        "nop\n\t"   /* 80140BF8: 00000000 */
        "nop\n\t"   /* 80140BFC: 00000000 */
        "mflo $15\n\t"   /* 80140C00: 00007812 */
        "sra $14,$15,0xc\n\t"   /* 80140C04: 000f7303 */
        "nop\n\t"   /* 80140C08: 00000000 */
        "multu $12,$13\n\t"   /* 80140C0C: 018d0019 */
        "nop\n\t"   /* 80140C10: 00000000 */
        "nop\n\t"   /* 80140C14: 00000000 */
        "mflo $15\n\t"   /* 80140C18: 00007812 */
        "sra $25,$15,0xc\n\t"   /* 80140C1C: 000fcb03 */
        "addu $15,$25,$14\n\t"   /* 80140C20: 032e7821 */
        "multu $12,$10\n\t"   /* 80140C24: 018a0019 */
        "sh $15,14($5)\n\t"   /* 80140C28: a4af000e */
        "nop\n\t"   /* 80140C2C: 00000000 */
        "mflo $14\n\t"   /* 80140C30: 00007012 */
        "sra $25,$14,0xc\n\t"   /* 80140C34: 000ecb03 */
        "negu $15,$25\n\t"   /* 80140C38: 00197823 */
        "multu $24,$13\n\t"   /* 80140C3C: 030d0019 */
        "nop\n\t"   /* 80140C40: 00000000 */
        "nop\n\t"   /* 80140C44: 00000000 */
        "mflo $14\n\t"   /* 80140C48: 00007012 */
        "sra $25,$14,0xc\n\t"   /* 80140C4C: 000ecb03 */
        "addu $14,$15,$25\n\t"   /* 80140C50: 01f97021 */
        "sh $14,12($5)\n\t"   /* 80140C54: a4ae000c */
        ".set\treorder\n\t"
        ".set noreorder"
        : "=r"(ret) : "r"(r), "r"(m)
        : "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$24", "$25", "memory");
    return ret;
}

/* EigenMatrix: eigenvectors of rotation matrix m (axis for eigenvalue 1)
 * into the columns of t. Sony C, reconstructed by hand from retail
 * (the cofactor terms are kept exactly as retail computes them). */
void __attribute__((section(".text_8013F3EC"))) sub_8013F3EC(MATRIX *m, MATRIX *t)
{
    MATRIX am;
    int c[9];
    VECTOR v0, v1, v2, n0, n2, n1;
    int i, j, k, max;

    am.m[0][0] = m->m[0][0] - 4096;
    am.m[0][1] = m->m[0][1];
    am.m[0][2] = m->m[0][2];
    am.m[1][0] = m->m[1][0];
    am.m[1][1] = m->m[1][1] - 4096;
    am.m[1][2] = m->m[1][2];
    am.m[2][0] = m->m[2][0];
    am.m[2][1] = m->m[2][1];
    am.m[2][2] = m->m[2][2] - 4096;

    c[0] = am.m[1][1] * am.m[2][2] - am.m[1][2] * am.m[2][1];
    c[1] = am.m[1][0] * am.m[2][2] - am.m[0][2] * am.m[2][0];
    c[2] = am.m[1][0] * am.m[2][1] - am.m[1][1] * am.m[2][0];
    c[3] = am.m[0][1] * am.m[2][2] - am.m[0][2] * am.m[2][1];
    c[4] = am.m[0][0] * am.m[2][2] - am.m[0][2] * am.m[2][0];
    c[5] = am.m[0][0] * am.m[2][1] - am.m[0][1] * am.m[2][0];
    c[6] = am.m[0][1] * am.m[2][2] - am.m[0][2] * am.m[2][1];
    c[7] = am.m[0][0] * am.m[1][2] - am.m[0][2] * am.m[1][0];
    c[8] = am.m[0][0] * am.m[1][1] - am.m[0][1] * am.m[1][0];

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (c[i * 3 + j] < 0) {
                c[i * 3 + j] = -c[i * 3 + j];
            }
        }
    }

    k = 0;
    max = c[0];
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (max < c[i * 3 + j]) {
                k = i * 3 + j;
                max = c[i * 3 + j];
            }
        }
    }

    switch (k) {
    case 6: case 7: case 8:
        v0.vx = am.m[0][0]; v0.vy = am.m[0][1]; v0.vz = am.m[0][2];
        v1.vx = am.m[1][0]; v1.vy = am.m[1][1]; v1.vz = am.m[1][2];
        break;
    case 3: case 4: case 5:
        v0.vx = am.m[0][0]; v0.vy = am.m[0][1]; v0.vz = am.m[0][2];
        v1.vx = am.m[2][0]; v1.vy = am.m[2][1]; v1.vz = am.m[2][2];
        break;
    case 0: case 1: case 2:
        v0.vx = am.m[1][0]; v0.vy = am.m[1][1]; v0.vz = am.m[1][2];
        v1.vx = am.m[2][0]; v1.vy = am.m[2][1]; v1.vz = am.m[2][2];
        break;
    }

    sub_801408B8(&v0, &v1, &v2);
    sub_8013FE04(&v2, &n0);
    t->m[0][0] = n0.vx;
    t->m[1][0] = n0.vy;
    t->m[2][0] = n0.vz;
    sub_8013FE04(&v0, &n1);
    t->m[0][1] = n1.vx;
    t->m[1][1] = n1.vy;
    t->m[2][1] = n1.vz;
    sub_801408B8(&n0, &n1, &v2);
    sub_8013FE04(&v2, &n2);
    t->m[0][2] = n2.vx;
    t->m[1][2] = n2.vy;
    t->m[2][2] = n2.vz;
}
