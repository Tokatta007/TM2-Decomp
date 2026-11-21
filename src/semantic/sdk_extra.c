/* sdk_extra: PsyQ SDK functions IDA did not list (config/US/extra_funcs.tsv).
   801394D0 DecDCTGetEnv, 8013955C DecDCTPutEnv, 8013A634 CdComstr, 8013A66C CdIntstr, 8013CD20 CD_set_test_parmnum, 80142134 SetPriority, 80143EA0 SetDumpFnt, 80144B68 NextPrim, 80144B84 IsEndPrim, 80144BDC AddPrims, 80144C18 CatPrim, 80144CA4 SetPolyF3, 80144CB8 SetPolyFT3, 80144CE0 SetPolyGT3, 80144D08 SetPolyFT4, 80144D1C SetPolyG4, 80144D30 SetPolyGT4, 80144D58 SetSprt16, 80144D6C SetSprt, 80144D80 SetTile1, 80144D94 SetTile8, 80144DA8 SetTile16, 80144DD0 SetLineF2, 80144DE4 SetLineG2, 80144DF8 SetLineF3, 80144E18 SetLineG3, 80144E38 SetLineF4, 80144E58 SetLineG4, 80144E8C sub_80144E8C, 80144EA8 MargePrim, 80147BD4 _spu_FgetRXXa, 8015D710 _SpuIsInAllocateArea, 80162460 _spu_pitch2note
 *
 * All are unused by TM2 (linked in with their library objects).
 *
 * Adapted from:
 *  - PSY-Z (MIT): decomp/src/libgpu/prim.c (NextPrim .. SetLineG4,
 *    MargePrim), decomp/src/libgpu/sys.c (SetPriority),
 *    decomp/src/libgpu/font.c (SetDumpFnt), decomp/src/libcd/sys.c
 *    (CdComstr, CdIntstr).  SPDX-License-Identifier: MIT
 *  - the LSD: Dream Emulator decompilation (CC0 1.0):
 *    src/psyq/libcd_bios.c (CD_set_test_parmnum).
 *    SPDX-License-Identifier: CC0-1.0
 *  - psy-q-decomp (MIT): src/spu/s_m_util.c (_SpuIsInAllocateArea, the
 *    pre-4.0 version).  SPDX-License-Identifier: MIT
 *  DecDCTGetEnv/DecDCTPutEnv, _spu_FgetRXXa, _spu_pitch2note (an older,
 *  table-free version than the public 4.0 C) and sub_80144E8C were written
 *  from the retail code.
 *
 * Recipe: psyq3.3 (GCC 2.6.0 + aspsx 2.21), gp_size 0, no -fvolatile, no
 * extern gp. Library data is declared extern by retail address.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned long u_long;
typedef unsigned char u_char;

/* ---- libpress ---- */
typedef struct {
    u_char iq_y[64];    /* 0x00 */
    u_char iq_c[64];    /* 0x40 */
    s16 dct[64];        /* 0x80 */
} DECDCTENV;

extern u_long D_801649B0[16];   /* iq_y */
extern u_long D_801649F0[16];   /* iq_c */
extern u_long D_80164A34[32];   /* dct */

/* DecDCTGetEnv: copy the current decoder environment out. */
DECDCTENV *__attribute__((section(".text_801394D0"))) sub_801394D0(DECDCTENV *env)
{
    u_long *src, *dst;
    s32 i;

    dst = (u_long *)env->iq_y;
    src = D_801649B0;
    for (i = 15; i != -1; i--)
        *dst++ = *src++;
    dst = (u_long *)env->iq_c;
    src = D_801649F0;
    for (i = 15; i != -1; i--)
        *dst++ = *src++;
    dst = (u_long *)env->dct;
    src = D_80164A34;
    for (i = 31; i != -1; i--)
        *dst++ = *src++;
    return env;
}

/* DecDCTPutEnv: install a decoder environment (quantisation tables). */
DECDCTENV *__attribute__((section(".text_8013955C"))) sub_8013955C(DECDCTENV *env)
{
    u_long *src, *dst;
    s32 i;

    dst = D_801649B0;
    src = (u_long *)env->iq_y;
    for (i = 15; i != -1; i--)
        *dst++ = *src++;
    dst = D_801649F0;
    src = (u_long *)env->iq_c;
    for (i = 15; i != -1; i--)
        *dst++ = *src++;
    return env;
}

/* ---- libcd ---- */
extern char *D_80176BBC[];      /* CD_comstr */
extern char *D_80176C3C[];      /* CD_intstr */
extern char D_800D0498[];       /* "none" */
extern s32 D_80176E40;          /* CD_test_parmnum */

/* CdComstr: name of a CD command. */
char *__attribute__((section(".text_8013A634"))) sub_8013A634(u_char com)
{
    if (com > 27) {
        return D_800D0498;
    }
    return D_80176BBC[com];
}

/* CdIntstr: name of a CD interrupt status. */
char *__attribute__((section(".text_8013A66C"))) sub_8013A66C(u_char intr)
{
    if (intr > 6) {
        return D_800D0498;
    }
    return D_80176C3C[intr];
}

/* CD_set_test_parmnum */
void __attribute__((section(".text_8013CD20"))) sub_8013CD20(s32 arg0)
{
    D_80176E40 = arg0;
}

/* ---- libgpu ---- */
typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} P_TAG;

typedef struct {
    u_long tag;
    u_long code[2];
} DR_PRIO;

#define setlen(p, _len) (((u_char *)(p))[3] = (u_char)(_len))
#define setcode(p, _code) (((u_char *)(p))[7] = (u_char)(_code))
#define getlen(p) (((u_char *)(p))[3])

extern s32 D_8017C5BC;          /* number of open font streams */
extern s32 D_8017C5C0;          /* dump stream id */
extern int (*gpuPrintf)();     /* GPU_printf */
extern int sub_80144580();      /* FntPrint */

/* SetPriority: build a mask-bit (priority) primitive. */
void __attribute__((section(".text_80142134"))) sub_80142134(DR_PRIO *p, int pbc, int pbw)
{
    int data;

    setlen(p, 2);
    data = 0xE6000000;
    if (pbc) {
        data = 0xE6000002;
    }
    p->code[0] = data | (pbw != 0);
    p->code[1] = 0;
}

/* SetDumpFnt: route the Dump* functions to a font stream. */
void __attribute__((section(".text_80143EA0"))) sub_80143EA0(int id)
{
    if (id >= 0 && D_8017C5BC >= id) {
        D_8017C5C0 = id;
        gpuPrintf = sub_80144580;
    }
}

/* NextPrim: next primitive in a list. */
void *__attribute__((section(".text_80144B68"))) sub_80144B68(void *p)
{
    return (void *)((*(u_long *)p & 0xFFFFFF) | 0x80000000);
}

/* IsEndPrim: 1 if p terminates its list. */
int __attribute__((section(".text_80144B84"))) sub_80144B84(void *p)
{
    return (*(u_long *)p & 0xFFFFFF) == 0xFFFFFF;
}

/* AddPrims: insert the list p0..p1 at ot (bit-field form gives retail's
 * mask register order). */
void __attribute__((section(".text_80144BDC"))) sub_80144BDC(void *ot, void *p0, void *p1)
{
    ((P_TAG *)p1)->addr = ((P_TAG *)ot)->addr;
    ((P_TAG *)ot)->addr = (u_long)p0;
}

/* CatPrim: link p1 after p0. */
void __attribute__((section(".text_80144C18"))) sub_80144C18(void *p0, void *p1)
{
    *(u_long *)p0 = (*(u_long *)p0 & 0xFF000000) | ((u_long)p1 & 0xFFFFFF);
}

/* Primitive initialisers: length and command code. */
void __attribute__((section(".text_80144CA4"))) sub_80144CA4(void *p) { setlen(p, 4); setcode(p, 0x20); }  /* SetPolyF3 */
void __attribute__((section(".text_80144CB8"))) sub_80144CB8(void *p) { setlen(p, 7); setcode(p, 0x24); }  /* SetPolyFT3 */
void __attribute__((section(".text_80144CE0"))) sub_80144CE0(void *p) { setlen(p, 9); setcode(p, 0x34); }  /* SetPolyGT3 */
void __attribute__((section(".text_80144D08"))) sub_80144D08(void *p) { setlen(p, 9); setcode(p, 0x2C); }  /* SetPolyFT4 */
void __attribute__((section(".text_80144D1C"))) sub_80144D1C(void *p) { setlen(p, 8); setcode(p, 0x38); }  /* SetPolyG4 */
void __attribute__((section(".text_80144D30"))) sub_80144D30(void *p) { setlen(p, 12); setcode(p, 0x3C); } /* SetPolyGT4 */
void __attribute__((section(".text_80144D58"))) sub_80144D58(void *p) { setlen(p, 3); setcode(p, 0x7C); }  /* SetSprt16 */
void __attribute__((section(".text_80144D6C"))) sub_80144D6C(void *p) { setlen(p, 4); setcode(p, 0x64); }  /* SetSprt */
void __attribute__((section(".text_80144D80"))) sub_80144D80(void *p) { setlen(p, 2); setcode(p, 0x68); }  /* SetTile1 */
void __attribute__((section(".text_80144D94"))) sub_80144D94(void *p) { setlen(p, 2); setcode(p, 0x70); }  /* SetTile8 */
void __attribute__((section(".text_80144DA8"))) sub_80144DA8(void *p) { setlen(p, 2); setcode(p, 0x78); }  /* SetTile16 */
void __attribute__((section(".text_80144DD0"))) sub_80144DD0(void *p) { setlen(p, 3); setcode(p, 0x40); }  /* SetLineF2 */
void __attribute__((section(".text_80144DE4"))) sub_80144DE4(void *p) { setlen(p, 4); setcode(p, 0x50); }  /* SetLineG2 */

/* Poly-line initialisers: length, code and the 0x55555555 terminator. */
void __attribute__((section(".text_80144DF8"))) sub_80144DF8(u_long *p) /* SetLineF3 */
{
    setlen(p, 5); setcode(p, 0x48); p[5] = 0x55555555;
}
void __attribute__((section(".text_80144E18"))) sub_80144E18(u_long *p) /* SetLineG3 */
{
    setlen(p, 7); setcode(p, 0x58); p[7] = 0x55555555;
}
void __attribute__((section(".text_80144E38"))) sub_80144E38(u_long *p) /* SetLineF4 */
{
    setlen(p, 6); setcode(p, 0x4C); p[6] = 0x55555555;
}
void __attribute__((section(".text_80144E58"))) sub_80144E58(u_long *p) /* SetLineG4 */
{
    setlen(p, 9); setcode(p, 0x5C); p[9] = 0x55555555;
}

/* sub_80144E8C: length-5 packet with code word 0x01000000 and 0x80000000
 * (VRAM-to-VRAM move header, cf. SetDrawMove). */
void __attribute__((section(".text_80144E8C"))) sub_80144E8C(u_long *p)
{
    setlen(p, 5); setcode(p, 1); p[2] = 0x80000000;
}

/* MargePrim: merge p1 into p0 (lengths add up to at most 16 words). */
int __attribute__((section(".text_80144EA8"))) sub_80144EA8(void *p0, void *p1)
{
    int newLen = getlen(p0) + getlen(p1) + 1;

    if (newLen > 32)
        return -1;
    setlen(p0, newLen);
    *(u_long *)p1 = 0;
    return 0;
}

/* ---- libspu ---- */
extern volatile u16 *D_8017E0DC;    /* _spu_RXX */
extern s32 D_8017E100;              /* _spu_mem_mode_plus */

typedef struct {
    u32 addr;
    u32 size;
} SPU_MALLOC;

extern SPU_MALLOC *D_8017E5A0;      /* _spu_memList */

/* _spu_FgetRXXa: read an SPU register, scaled to a byte address unless mode is -1. */
int __attribute__((section(".text_80147BD4"))) sub_80147BD4(int offset, int mode)
{
    u16 v = D_8017E0DC[offset];

    if (mode == -1)
        return v;
    return v << D_8017E100;
}

/* _SpuIsInAllocateArea: 1 if addr lies at or below the end of an allocated block. */
s32 __attribute__((section(".text_8015D710"))) sub_8015D710(u32 arg0)
{
    SPU_MALLOC *var_a1;
    u32 temp_v1;
    u32 bits1;
    u32 bits2;
    u32 mask;

    bits1 = 0x80000000;
    bits2 = 0x40000000;
    mask = 0x0FFFFFFF;

    var_a1 = D_8017E5A0;
    while (1) {
        temp_v1 = var_a1->addr;
        if (temp_v1 & bits1) {
            var_a1 += 1;
            continue;
        }
        if (!(temp_v1 & bits2)) {
            if ((temp_v1 & mask) < arg0) {
                if (arg0 >= (u32)((temp_v1 & mask) + var_a1->size)) {
                    var_a1 += 1;
                    continue;
                }
                return 1;
            } else {
                return 1;
            }
        }
        break;
    }
    return 0;
}

/* _spu_pitch2note: SPU pitch -> (note << 8 | fine) relative to a centre
 * note, or -1 if out of range. MATCHING: the sum gets its own variable `s`
 * (reusing cents/semi changes the register allocation). */
int __attribute__((section(".text_80162460"))) sub_80162460(u16 note, u16 fine, u16 pitch)
{
    int i, lsb, range, rem, semi, cents, n, f, s;

    lsb = 0;
    for (i = 0; i < 16; i++) {
        if ((pitch >> i) & 1)
            lsb = i;
    }
    range = (1 << (lsb + 1)) - (1 << lsb);
    rem = pitch - (1 << lsb);
    semi = rem * 12 / range;
    cents = (rem - range * semi / 12) * 1536 / range;
    n = note + (lsb - 12) * 12 + semi;
    s = fine + cents;
    f = s % 128;
    n += (s >= 128);
    if (n < 0)
        return -1;
    if (n >= 128)
        return -1;
    return (n << 8) | f;
}
