/* Library code 80138000..80140000: Sony PsyQ SDK functions, reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Sources:
 *  - CdSync, CdReady, CdSyncCallback, CdReadyCallback, CdMix (libcd sys.c) and
 *    memcpy2 (libcd iso9660.c): adapted from sotn-decomp src/main/psxsdk/libcd,
 *    SPDX-License-Identifier: MIT.
 *  - MatrixNormal_1, MatrixNormal_2: adapted from psyz decomp/src/libgte/geo_03.c
 *    (MIT, decomp/LICENSE).
 *  - __main, DecDCTinSync, DecDCToutSync, DecDCTinCallback: one-line wrappers
 *    written from the retail disassembly.
 *  - start (crt0), sprintf, MDEC_vlc, SquareRoot0 and the VectorNormal core
 *    (8013FE60) are not cc1 output: each is file-scope assembly in its own
 *    .text_<VA> section reproducing the retail instruction sequence (no public
 *    source used). Data is referenced by retail address (%hi/%lo of D_XXXXXXXX);
 *    stores keep a numeric low half because maspsx expands %lo(sym)(reg) stores,
 *    and two loads in sprintf are .words so maspsx adds no load-delay nop.
 *
 * The SingleTrac movie-player functions in this range (movieStrInit .. moviePlay,
 * 80138068..80138783) need the game recipe and live in their own unit.
 *
 * Recipe: psyq3.3 cc1 (GCC 2.6.0) + maspsx (aspsx 2.21), gp_size 0, no -fvolatile.
 * Library data is not defined here; it is declared extern by retail address. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct { s16 m[3][3]; s32 t[3]; } MATRIX;
typedef struct { s32 vx, vy, vz, pad; } VECTOR;

extern void sub_80139918(void);                 /* MDEC_in_sync */
extern void sub_801399B0(void);                 /* MDEC_out_sync */
extern void *sub_80145350(s32, void (*)());     /* DMACallback */
extern s32 sub_8013B2F8(s32, u8 *);             /* CD_sync */
extern s32 sub_8013B57C(s32, u8 *);             /* CD_ready */
extern void sub_8013BCB4(void *);               /* CD_vol */
extern void (*D_80176B9C)();                    /* CD_cbsync */
extern void (*D_80176BA0)();                    /* CD_cbready */
extern void sub_801408B8(VECTOR *, VECTOR *, VECTOR *);  /* OuterProduct12 */
extern void sub_8013FE04(VECTOR *, VECTOR *);   /* VectorNormal */

/* __main: empty (GCC static-constructor hook). */
void __attribute__((section(".text_80138784"))) sub_80138784(void)
{
}

/* DecDCTinSync: wait for the MDEC input DMA. */
void __attribute__((section(".text_80139664"))) sub_80139664(s32 mode)
{
    sub_80139918();
}

/* DecDCToutSync: wait for the MDEC output DMA. */
void __attribute__((section(".text_80139684"))) sub_80139684(s32 mode)
{
    sub_801399B0();
}

/* DecDCTinCallback: install the MDEC-in (DMA 0) callback. */
void __attribute__((section(".text_801396A4"))) sub_801396A4(void (*func)())
{
    sub_80145350(0, func);
}

/* CdSync (sotn-decomp libcd/sys.c, MIT). */
s32 __attribute__((section(".text_8013A6A4"))) sub_8013A6A4(s32 mode, u8 *result)
{
    return sub_8013B2F8(mode, result);
}

/* CdReady (sotn-decomp libcd/sys.c, MIT). */
s32 __attribute__((section(".text_8013A6C4"))) sub_8013A6C4(s32 mode, u8 *result)
{
    return sub_8013B57C(mode, result);
}

/* CdSyncCallback: install the command-complete callback, return the old one
 * (sotn-decomp libcd/sys.c, MIT). */
void * __attribute__((section(".text_8013A6FC"))) sub_8013A6FC(void (*func)())
{
    void (*old)() = D_80176B9C;
    D_80176B9C = func;
    return old;
}

/* CdReadyCallback: install the data-ready callback, return the old one
 * (sotn-decomp libcd/sys.c, MIT). */
void * __attribute__((section(".text_8013A714"))) sub_8013A714(void (*func)())
{
    void (*old)() = D_80176BA0;
    D_80176BA0 = func;
    return old;
}

/* CdMix: set the CD audio mixing volumes (sotn-decomp libcd/sys.c, MIT). */
s32 __attribute__((section(".text_8013AB04"))) sub_8013AB04(void *vol)
{
    sub_8013BCB4(vol);
    return 1;
}

/* memcpy2: byte copy used by the ISO9660 directory reader
 * (sotn-decomp libcd/iso9660.c, MIT). */
void __attribute__((section(".text_8013D960"))) sub_8013D960(char *dst, char *src, u32 size)
{
    while (size--) {
        *dst++ = *src++;
    }
}

/* MatrixNormal_1: re-orthonormalise m into n from rows 1 and 2
 * (psyz decomp/src/libgte/geo_03.c, MIT). */
void __attribute__((section(".text_8013F984"))) sub_8013F984(MATRIX *m, MATRIX *n)
{
    VECTOR v0, v1, v2;

    v1.vx = m->m[1][0];
    v1.vy = m->m[1][1];
    v1.vz = m->m[1][2];

    v2.vx = m->m[2][0];
    v2.vy = m->m[2][1];
    v2.vz = m->m[2][2];

    sub_801408B8(&v1, &v2, &v0);
    sub_801408B8(&v0, &v1, &v2);

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

/* MatrixNormal_2: re-orthonormalise m into n from rows 2 and 0
 * (psyz decomp/src/libgte/geo_03.c, MIT). */
void __attribute__((section(".text_8013FAA4"))) sub_8013FAA4(MATRIX *m, MATRIX *n)
{
    VECTOR v0, v1, v2;

    v2.vx = m->m[2][0];
    v2.vy = m->m[2][1];
    v2.vz = m->m[2][2];

    v0.vx = m->m[0][0];
    v0.vy = m->m[0][1];
    v0.vz = m->m[0][2];

    sub_801408B8(&v2, &v0, &v1);
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

/* ======== asm functions ======== */

/* start: C runtime entry (clear BSS, set up stack/heap/gp, call main). Sony assembly. */
__asm__(
        ".section .text_8013878C,\"ax\",@progbits\n\t"
        ".align 2\n\t"
        ".globl sub_8013878C\n\t"
        ".type sub_8013878C,@function\n"
        "sub_8013878C:\n\t"
        ".set\tnoreorder\n\t"
        ".set noat\n\t"
        "lui $2,%hi(D_80180EE4)\n\t"   /* 8013878C: 3c028018 lui v0,0x8018 */
        "addiu $2,$2,%lo(D_80180EE4)\n\t"   /* 80138790: 24420ee4 addiu v0,v0,3812 */
        "lui $3,%hi(D_801FAD48)\n\t"   /* 80138794: 3c038020 lui v1,0x8020 */
        "addiu $3,$3,%lo(D_801FAD48)\n\t"   /* 80138798: 2463ad48 addiu v1,v1,-21176 */
        ".L8013878C_8013879C:\n\t"
        "sw $0,0($2)\n\t"   /* 8013879C: ac400000 sw zero,0(v0) */
        "addiu $2,$2,4\n\t"   /* 801387A0: 24420004 addiu v0,v0,4 */
        "sltu $1,$2,$3\n\t"   /* 801387A4: 0043082b sltu at,v0,v1 */
        "bne $1,$0,.L8013878C_8013879C\n\t"   /* 801387A8: 1420fffc bnez at,0x8013879c */
        "nop\n\t"   /* 801387AC: 00000000 nop  */
        "addiu $2,$0,4\n\t"   /* 801387B0: 24020004 li v0,4 */
        "nop\n\t"   /* 801387B4: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 801387B8: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 801387BC: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 801387C0: 00000000 .word 0x00000000 */
        "lui $4,%hi(D_80138838)\n\t"   /* 801387C4: 3c048014 lui a0,0x8014 */
        "addiu $4,$4,%lo(D_80138838)\n\t"   /* 801387C8: 24848838 addiu a0,a0,-30664 */
        "addu $4,$4,$2\n\t"   /* 801387CC: 00822021 addu a0,a0,v0 */
        "lw $2,0($4)\n\t"   /* 801387D0: 8c820000 lw v0,0(a0) */
        "lui $8,0x8000\n\t"   /* 801387D4: 3c088000 lui t0,0x8000 */
        "or $29,$2,$8\n\t"   /* 801387D8: 0048e825 or sp,v0,t0 */
        "lui $4,%hi(D_801FAD48)\n\t"   /* 801387DC: 3c048020 lui a0,0x8020 */
        "addiu $4,$4,%lo(D_801FAD48)\n\t"   /* 801387E0: 2484ad48 addiu a0,a0,-21176 */
        "sll $4,$4,0x3\n\t"   /* 801387E4: 000420c0 sll a0,a0,0x3 */
        "srl $4,$4,0x3\n\t"   /* 801387E8: 000420c2 srl a0,a0,0x3 */
        "lui $3,%hi(D_8017E0CC)\n\t"   /* 801387EC: 3c038018 lui v1,0x8018 */
        "lw $3,%lo(D_8017E0CC)($3)\n\t"   /* 801387F0: 8c63e0cc lw v1,-7988(v1) */
        "nop\n\t"   /* 801387F4: 00000000 nop  */
        "subu $5,$2,$3\n\t"   /* 801387F8: 00432823 subu a1,v0,v1 */
        "subu $5,$5,$4\n\t"   /* 801387FC: 00a42823 subu a1,a1,a0 */
        "or $4,$4,$8\n\t"   /* 80138800: 00882025 or a0,a0,t0 */
        "lui $1,%hi(D_801810BC)\n\t"   /* 80138804: 3c018018 lui at,0x8018 */
        "sw $31,%lo(D_801810BC)($1)\n\t"   /* 80138808: ac3f10bc sw ra,4284(at) */
        "lui $28,%hi(D_801809D4)\n\t"   /* 8013880C: 3c1c8018 lui gp,0x8018 */
        "addiu $28,$28,%lo(D_801809D4)\n\t"   /* 80138810: 279c09d4 addiu gp,gp,2516 */
        "addu $30,$29,$0\n\t"   /* 80138814: 03a0f021 move s8,sp */
        "jal sub_80145E10\n\t"   /* 80138818: 0c051784 jal 0x80145e10 */
        "addi $4,$4,4\n\t"   /* 8013881C: 20840004 addi a0,a0,4 */
        "lui $31,%hi(D_801810BC)\n\t"   /* 80138820: 3c1f8018 lui ra,0x8018 */
        "lw $31,%lo(D_801810BC)($31)\n\t"   /* 80138824: 8fff10bc lw ra,4284(ra) */
        "nop\n\t"   /* 80138828: 00000000 nop  */
        "jal sub_8012C8DC\n\t"   /* 8013882C: 0c04b237 jal 0x8012c8dc */
        "nop\n\t"   /* 80138830: 00000000 nop  */
        ".word 0x0000004d\n\t"   /* 80138834: 0000004d break 0x0,0x1 */
        ".set at\n\t"
        ".set\treorder\n\t"
        ".set noreorder\n\t"
        ".size sub_8013878C,.-sub_8013878C\n\t"
        ".text");

/* sprintf (LIBC). Not cc1 output; kept as the original instruction sequence. */
__asm__(
        ".section .text_801388B8,\"ax\",@progbits\n\t"
        ".align 2\n\t"
        ".globl sub_801388B8\n\t"
        ".type sub_801388B8,@function\n"
        "sub_801388B8:\n\t"
        ".set\tnoreorder\n\t"
        ".set noat\n\t"
        "addiu $29,$29,-640\n\t"   /* 801388B8: 27bdfd80 addiu sp,sp,-640 */
        "sw $31,28($29)\n\t"   /* 801388BC: afbf001c sw ra,28(sp) */
        "sw $4,640($29)\n\t"   /* 801388C0: afa40280 sw a0,640(sp) */
        "sw $6,648($29)\n\t"   /* 801388C4: afa60288 sw a2,648(sp) */
        "sw $7,652($29)\n\t"   /* 801388C8: afa7028c sw a3,652(sp) */
        "addu $8,$5,$0\n\t"   /* 801388CC: 00a04021 move t0,a1 */
        "lb $2,0($8)\n\t"   /* 801388D0: 81020000 lb v0,0(t0) */
        "addu $10,$0,$0\n\t"   /* 801388D4: 00005021 move t2,zero */
        "beq $2,$0,.L801388B8_801393D4\n\t"   /* 801388D8: 104002be beqz v0,0x801393d4 */
        "addiu $12,$29,648\n\t"   /* 801388DC: 27ac0288 addiu t4,sp,648 */
        "sw $16,68($29)\n\t"   /* 801388E0: afb00044 sw s0,68(sp) */
        "sw $17,64($29)\n\t"   /* 801388E4: afb10040 sw s1,64(sp) */
        "sw $18,60($29)\n\t"   /* 801388E8: afb2003c sw s2,60(sp) */
        "sw $19,56($29)\n\t"   /* 801388EC: afb30038 sw s3,56(sp) */
        "sw $20,52($29)\n\t"   /* 801388F0: afb40034 sw s4,52(sp) */
        "sw $21,48($29)\n\t"   /* 801388F4: afb50030 sw s5,48(sp) */
        "sw $22,44($29)\n\t"   /* 801388F8: afb6002c sw s6,44(sp) */
        "sw $23,40($29)\n\t"   /* 801388FC: afb70028 sw s7,40(sp) */
        "sw $30,36($29)\n\t"   /* 80138900: afbe0024 sw s8,36(sp) */
        "addiu $30,$0,100\n\t"   /* 80138904: 241e0064 li s8,100 */
        "addiu $23,$0,110\n\t"   /* 80138908: 2417006e li s7,110 */
        "addiu $22,$0,-5\n\t"   /* 8013890C: 2416fffb li s6,-5 */
        "addiu $21,$0,35\n\t"   /* 80138910: 24150023 li s5,35 */
        "addiu $20,$0,-3\n\t"   /* 80138914: 2414fffd li s4,-3 */
        "addiu $19,$0,43\n\t"   /* 80138918: 2413002b li s3,43 */
        "addiu $18,$0,45\n\t"   /* 8013891C: 2412002d li s2,45 */
        "addiu $17,$0,-65\n\t"   /* 80138920: 2411ffbf li s1,-65 */
        "addiu $16,$0,-9\n\t"   /* 80138924: 2410fff7 li s0,-9 */
        "addiu $11,$0,48\n\t"   /* 80138928: 240b0030 li t3,48 */
        "addiu $13,$0,32\n\t"   /* 8013892C: 240d0020 li t5,32 */
        "addiu $31,$0,-2\n\t"   /* 80138930: 241ffffe li ra,-2 */
        "addiu $1,$0,37\n\t"   /* 80138934: 24010025 li at,37 */
        ".L801388B8_80138938:\n\t"
        "beq $2,$1,.L801388B8_8013894C\n\t"   /* 80138938: 10410004 beq v0,at,0x8013894c */
        "nop\n\t"   /* 8013893C: 00000000 nop  */
        "lw $14,640($29)\n\t"   /* 80138940: 8fae0280 lw t6,640(sp) */
        "beq $0,$0,.L801388B8_801391EC\n\t"   /* 80138944: 10000229 b 0x801391ec */
        "addu $9,$14,$10\n\t"   /* 80138948: 01ca4821 addu t1,t6,t2 */
        ".L801388B8_8013894C:\n\t"
        "lui $24,%hi(D_8016496C)\n\t"   /* 8013894C: 3c188016 lui t8,0x8016 */
        "addiu $24,$24,%lo(D_8016496C)\n\t"   /* 80138950: 2718496c addiu t8,t8,18796 */
        "lw $1,0($24)\n\t"   /* 80138954: 8f010000 lw at,0(t8) */
        "addiu $15,$29,84\n\t"   /* 80138958: 27af0054 addiu t7,sp,84 */
        "sw $1,0($15)\n\t"   /* 8013895C: ade10000 sw at,0(t7) */
        "lw $25,4($24)\n\t"   /* 80138960: 8f190004 lw t9,4(t8) */
        "addiu $7,$29,616\n\t"   /* 80138964: 27a70268 addiu a3,sp,616 */
        "sw $25,4($15)\n\t"   /* 80138968: adf90004 sw t9,4(t7) */
        "lw $1,8($24)\n\t"   /* 8013896C: 8f010008 lw at,8(t8) */
        "nop\n\t"   /* 80138970: 00000000 nop  */
        "sw $1,8($15)\n\t"   /* 80138974: ade10008 sw at,8(t7) */
        ".L801388B8_80138978:\n\t"
        "lb $2,1($8)\n\t"   /* 80138978: 81020001 lb v0,1(t0) */
        "addiu $8,$8,1\n\t"   /* 8013897C: 25080001 addiu t0,t0,1 */
        "bne $2,$18,.L801388B8_801389A0\n\t"   /* 80138980: 14520007 bne v0,s2,0x801389a0 */
        "nop\n\t"   /* 80138984: 00000000 nop  */
        "lw $14,84($29)\n\t"   /* 80138988: 8fae0054 lw t6,84(sp) */
        "nop\n\t"   /* 8013898C: 00000000 nop  */
        "and $15,$14,$31\n\t"   /* 80138990: 01df7824 and t7,t6,ra */
        "ori $24,$15,0x1\n\t"   /* 80138994: 35f80001 ori t8,t7,0x1 */
        "beq $0,$0,.L801388B8_80138978\n\t"   /* 80138998: 1000fff7 b 0x80138978 */
        "sw $24,84($29)\n\t"   /* 8013899C: afb80054 sw t8,84(sp) */
        ".L801388B8_801389A0:\n\t"
        "bne $2,$19,.L801388B8_801389C0\n\t"   /* 801389A0: 14530007 bne v0,s3,0x801389c0 */
        "nop\n\t"   /* 801389A4: 00000000 nop  */
        "lw $25,84($29)\n\t"   /* 801389A8: 8fb90054 lw t9,84(sp) */
        "nop\n\t"   /* 801389AC: 00000000 nop  */
        "and $14,$25,$20\n\t"   /* 801389B0: 03347024 and t6,t9,s4 */
        "ori $15,$14,0x2\n\t"   /* 801389B4: 35cf0002 ori t7,t6,0x2 */
        "beq $0,$0,.L801388B8_80138978\n\t"   /* 801389B8: 1000ffef b 0x80138978 */
        "sw $15,84($29)\n\t"   /* 801389BC: afaf0054 sw t7,84(sp) */
        ".L801388B8_801389C0:\n\t"
        "bne $2,$13,.L801388B8_801389D0\n\t"   /* 801389C0: 144d0003 bne v0,t5,0x801389d0 */
        "nop\n\t"   /* 801389C4: 00000000 nop  */
        "beq $0,$0,.L801388B8_80138978\n\t"   /* 801389C8: 1000ffeb b 0x80138978 */
        "sb $13,85($29)\n\t"   /* 801389CC: a3ad0055 sb t5,85(sp) */
        ".L801388B8_801389D0:\n\t"
        "bne $2,$21,.L801388B8_801389F0\n\t"   /* 801389D0: 14550007 bne v0,s5,0x801389f0 */
        "nop\n\t"   /* 801389D4: 00000000 nop  */
        "lw $24,84($29)\n\t"   /* 801389D8: 8fb80054 lw t8,84(sp) */
        "nop\n\t"   /* 801389DC: 00000000 nop  */
        "and $25,$24,$22\n\t"   /* 801389E0: 0316c824 and t9,t8,s6 */
        "ori $14,$25,0x4\n\t"   /* 801389E4: 372e0004 ori t6,t9,0x4 */
        "beq $0,$0,.L801388B8_80138978\n\t"   /* 801389E8: 1000ffe3 b 0x80138978 */
        "sw $14,84($29)\n\t"   /* 801389EC: afae0054 sw t6,84(sp) */
        ".L801388B8_801389F0:\n\t"
        "bne $2,$11,.L801388B8_80138A14\n\t"   /* 801389F0: 144b0008 bne v0,t3,0x80138a14 */
        "addiu $1,$0,42\n\t"   /* 801389F4: 2401002a li at,42 */
        "lw $15,84($29)\n\t"   /* 801389F8: 8faf0054 lw t7,84(sp) */
        "nop\n\t"   /* 801389FC: 00000000 nop  */
        "and $24,$15,$16\n\t"   /* 80138A00: 01f0c024 and t8,t7,s0 */
        "ori $25,$24,0x8\n\t"   /* 80138A04: 37190008 ori t9,t8,0x8 */
        "beq $0,$0,.L801388B8_80138978\n\t"   /* 80138A08: 1000ffdb b 0x80138978 */
        "sw $25,84($29)\n\t"   /* 80138A0C: afb90054 sw t9,84(sp) */
        "addiu $1,$0,42\n\t"   /* 80138A10: 2401002a li at,42 */
        ".L801388B8_80138A14:\n\t"
        "bne $2,$1,.L801388B8_80138A64\n\t"   /* 80138A14: 14410013 bne v0,at,0x80138a64 */
        "slti $1,$2,48\n\t"   /* 80138A18: 28410030 slti at,v0,48 */
        "lw $14,0($12)\n\t"   /* 80138A1C: 8d8e0000 lw t6,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138A20: 258c0004 addiu t4,t4,4 */
        "sw $14,88($29)\n\t"   /* 80138A24: afae0058 sw t6,88(sp) */
        "lw $15,88($29)\n\t"   /* 80138A28: 8faf0058 lw t7,88(sp) */
        "nop\n\t"   /* 80138A2C: 00000000 nop  */
        "bgez $15,.L801388B8_80138A54\n\t"   /* 80138A30: 05e10008 bgez t7,0x80138a54 */
        "nop\n\t"   /* 80138A34: 00000000 nop  */
        "lw $24,84($29)\n\t"   /* 80138A38: 8fb80054 lw t8,84(sp) */
        "nop\n\t"   /* 80138A3C: 00000000 nop  */
        "and $25,$24,$31\n\t"   /* 80138A40: 031fc824 and t9,t8,ra */
        "ori $14,$25,0x1\n\t"   /* 80138A44: 372e0001 ori t6,t9,0x1 */
        "subu $24,$0,$15\n\t"   /* 80138A48: 000fc023 negu t8,t7 */
        "sw $14,84($29)\n\t"   /* 80138A4C: afae0054 sw t6,84(sp) */
        "sw $24,88($29)\n\t"   /* 80138A50: afb80058 sw t8,88(sp) */
        ".L801388B8_80138A54:\n\t"
        "lb $2,1($8)\n\t"   /* 80138A54: 81020001 lb v0,1(t0) */
        "beq $0,$0,.L801388B8_80138AAC\n\t"   /* 80138A58: 10000014 b 0x80138aac */
        "addiu $8,$8,1\n\t"   /* 80138A5C: 25080001 addiu t0,t0,1 */
        "slti $1,$2,48\n\t"   /* 80138A60: 28410030 slti at,v0,48 */
        ".L801388B8_80138A64:\n\t"
        "bne $1,$0,.L801388B8_80138AAC\n\t"   /* 80138A64: 14200011 bnez at,0x80138aac */
        "slti $1,$2,58\n\t"   /* 80138A68: 2841003a slti at,v0,58 */
        "beq $1,$0,.L801388B8_80138AB0\n\t"   /* 80138A6C: 10200010 beqz at,0x80138ab0 */
        "addiu $1,$0,46\n\t"   /* 80138A70: 2401002e li at,46 */
        ".L801388B8_80138A74:\n\t"
        "lw $25,88($29)\n\t"   /* 80138A74: 8fb90058 lw t9,88(sp) */
        "addiu $8,$8,1\n\t"   /* 80138A78: 25080001 addiu t0,t0,1 */
        "sll $14,$25,0x2\n\t"   /* 80138A7C: 00197080 sll t6,t9,0x2 */
        "addu $14,$14,$25\n\t"   /* 80138A80: 01d97021 addu t6,t6,t9 */
        "sll $14,$14,0x1\n\t"   /* 80138A84: 000e7040 sll t6,t6,0x1 */
        "addu $15,$14,$2\n\t"   /* 80138A88: 01c27821 addu t7,t6,v0 */
        "addiu $24,$15,-48\n\t"   /* 80138A8C: 25f8ffd0 addiu t8,t7,-48 */
        "sw $24,88($29)\n\t"   /* 80138A90: afb80058 sw t8,88(sp) */
        "lb $2,0($8)\n\t"   /* 80138A94: 81020000 lb v0,0(t0) */
        "nop\n\t"   /* 80138A98: 00000000 nop  */
        "slti $1,$2,48\n\t"   /* 80138A9C: 28410030 slti at,v0,48 */
        "bne $1,$0,.L801388B8_80138AAC\n\t"   /* 80138AA0: 14200002 bnez at,0x80138aac */
        "slti $1,$2,58\n\t"   /* 80138AA4: 2841003a slti at,v0,58 */
        "bne $1,$0,.L801388B8_80138A74\n\t"   /* 80138AA8: 1420fff2 bnez at,0x80138a74 */
        ".L801388B8_80138AAC:\n\t"
        "addiu $1,$0,46\n\t"   /* 80138AAC: 2401002e li at,46 */
        ".L801388B8_80138AB0:\n\t"
        "bne $2,$1,.L801388B8_80138B58\n\t"   /* 80138AB0: 14410029 bne v0,at,0x80138b58 */
        "lw $14,84($29)\n\t"   /* 80138AB4: 8fae0054 lw t6,84(sp) */
        "lb $2,1($8)\n\t"   /* 80138AB8: 81020001 lb v0,1(t0) */
        "addiu $1,$0,42\n\t"   /* 80138ABC: 2401002a li at,42 */
        "bne $2,$1,.L801388B8_80138AE0\n\t"   /* 80138AC0: 14410007 bne v0,at,0x80138ae0 */
        "addiu $8,$8,1\n\t"   /* 80138AC4: 25080001 addiu t0,t0,1 */
        "lw $25,0($12)\n\t"   /* 80138AC8: 8d990000 lw t9,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138ACC: 258c0004 addiu t4,t4,4 */
        "sw $25,92($29)\n\t"   /* 80138AD0: afb9005c sw t9,92(sp) */
        "lb $2,1($8)\n\t"   /* 80138AD4: 81020001 lb v0,1(t0) */
        "beq $0,$0,.L801388B8_80138B30\n\t"   /* 80138AD8: 10000015 b 0x80138b30 */
        "addiu $8,$8,1\n\t"   /* 80138ADC: 25080001 addiu t0,t0,1 */
        ".L801388B8_80138AE0:\n\t"
        "slti $1,$2,48\n\t"   /* 80138AE0: 28410030 slti at,v0,48 */
        "bne $1,$0,.L801388B8_80138B30\n\t"   /* 80138AE4: 14200012 bnez at,0x80138b30 */
        "slti $1,$2,58\n\t"   /* 80138AE8: 2841003a slti at,v0,58 */
        "beq $1,$0,.L801388B8_80138B34\n\t"   /* 80138AEC: 10200011 beqz at,0x80138b34 */
        "lw $14,92($29)\n\t"   /* 80138AF0: 8fae005c lw t6,92(sp) */
        ".word 0x8fae005c\n\t"   /* 80138AF4: lw t6,92(sp), as a .word: maspsx would put a nop between the two identical loads */
        ".L801388B8_80138AF8:\n\t"
        "addiu $8,$8,1\n\t"   /* 80138AF8: 25080001 addiu t0,t0,1 */
        "sll $15,$14,0x2\n\t"   /* 80138AFC: 000e7880 sll t7,t6,0x2 */
        "addu $15,$15,$14\n\t"   /* 80138B00: 01ee7821 addu t7,t7,t6 */
        "sll $15,$15,0x1\n\t"   /* 80138B04: 000f7840 sll t7,t7,0x1 */
        "addu $24,$15,$2\n\t"   /* 80138B08: 01e2c021 addu t8,t7,v0 */
        "addiu $25,$24,-48\n\t"   /* 80138B0C: 2719ffd0 addiu t9,t8,-48 */
        "sw $25,92($29)\n\t"   /* 80138B10: afb9005c sw t9,92(sp) */
        "lb $2,0($8)\n\t"   /* 80138B14: 81020000 lb v0,0(t0) */
        "nop\n\t"   /* 80138B18: 00000000 nop  */
        "slti $1,$2,48\n\t"   /* 80138B1C: 28410030 slti at,v0,48 */
        "bne $1,$0,.L801388B8_80138B30\n\t"   /* 80138B20: 14200003 bnez at,0x80138b30 */
        "slti $1,$2,58\n\t"   /* 80138B24: 2841003a slti at,v0,58 */
        "bne $1,$0,.L801388B8_80138AF8\n\t"   /* 80138B28: 1420fff3 bnez at,0x80138af8 */
        "lw $14,92($29)\n\t"   /* 80138B2C: 8fae005c lw t6,92(sp) */
        ".L801388B8_80138B30:\n\t"
        "lw $14,92($29)\n\t"   /* 80138B30: 8fae005c lw t6,92(sp) */
        ".L801388B8_80138B34:\n\t"
        "nop\n\t"   /* 80138B34: 00000000 nop  */
        "bltz $14,.L801388B8_80138B58\n\t"   /* 80138B38: 05c00007 bltz t6,0x80138b58 */
        "lw $14,84($29)\n\t"   /* 80138B3C: 8fae0054 lw t6,84(sp) */
        "lw $15,84($29)\n\t"   /* 80138B40: 8faf0054 lw t7,84(sp) */
        "addiu $1,$0,-17\n\t"   /* 80138B44: 2401ffef li at,-17 */
        "and $24,$15,$1\n\t"   /* 80138B48: 01e1c024 and t8,t7,at */
        "ori $25,$24,0x10\n\t"   /* 80138B4C: 37190010 ori t9,t8,0x10 */
        "sw $25,84($29)\n\t"   /* 80138B50: afb90054 sw t9,84(sp) */
        "lw $14,84($29)\n\t"   /* 80138B54: 8fae0054 lw t6,84(sp) */
        ".L801388B8_80138B58:\n\t"
        "nop\n\t"   /* 80138B58: 00000000 nop  */
        "andi $15,$14,0x1\n\t"   /* 80138B5C: 31cf0001 andi t7,t6,0x1 */
        "beq $15,$0,.L801388B8_801391FC\n\t"   /* 80138B60: 11e001a6 beqz t7,0x801391fc */
        "nop\n\t"   /* 80138B64: 00000000 nop  */
        "and $24,$14,$16\n\t"   /* 80138B68: 01d0c024 and t8,t6,s0 */
        "beq $0,$0,.L801388B8_801391FC\n\t"   /* 80138B6C: 100001a3 b 0x801391fc */
        "sw $24,84($29)\n\t"   /* 80138B70: afb80054 sw t8,84(sp) */
        "lw $25,84($29)\n\t"   /* 80138B74: 8fb90054 lw t9,84(sp) */
        ".L801388B8_80138B78:\n\t"
        "addiu $1,$0,-33\n\t"   /* 80138B78: 2401ffdf li at,-33 */
        "and $15,$25,$1\n\t"   /* 80138B7C: 03217824 and t7,t9,at */
        "ori $14,$15,0x20\n\t"   /* 80138B80: 35ee0020 ori t6,t7,0x20 */
        "sw $14,84($29)\n\t"   /* 80138B84: afae0054 sw t6,84(sp) */
        "lb $2,1($8)\n\t"   /* 80138B88: 81020001 lb v0,1(t0) */
        "beq $0,$0,.L801388B8_801391FC\n\t"   /* 80138B8C: 1000019b b 0x801391fc */
        "addiu $8,$8,1\n\t"   /* 80138B90: 25080001 addiu t0,t0,1 */
        "lw $24,84($29)\n\t"   /* 80138B94: 8fb80054 lw t8,84(sp) */
        ".L801388B8_80138B98:\n\t"
        "addiu $8,$8,1\n\t"   /* 80138B98: 25080001 addiu t0,t0,1 */
        "and $25,$24,$17\n\t"   /* 80138B9C: 0311c824 and t9,t8,s1 */
        "ori $15,$25,0x40\n\t"   /* 80138BA0: 372f0040 ori t7,t9,0x40 */
        "sw $15,84($29)\n\t"   /* 80138BA4: afaf0054 sw t7,84(sp) */
        "lb $2,0($8)\n\t"   /* 80138BA8: 81020000 lb v0,0(t0) */
        "beq $0,$0,.L801388B8_801391FC\n\t"   /* 80138BAC: 10000193 b 0x801391fc */
        "nop\n\t"   /* 80138BB0: 00000000 nop  */
        "lw $14,84($29)\n\t"   /* 80138BB4: 8fae0054 lw t6,84(sp) */
        ".L801388B8_80138BB8:\n\t"
        "addiu $1,$0,-129\n\t"   /* 80138BB8: 2401ff7f li at,-129 */
        "and $24,$14,$1\n\t"   /* 80138BBC: 01c1c024 and t8,t6,at */
        "ori $25,$24,0x80\n\t"   /* 80138BC0: 37190080 ori t9,t8,0x80 */
        "sw $25,84($29)\n\t"   /* 80138BC4: afb90054 sw t9,84(sp) */
        "lb $2,1($8)\n\t"   /* 80138BC8: 81020001 lb v0,1(t0) */
        "beq $0,$0,.L801388B8_801391FC\n\t"   /* 80138BCC: 1000018b b 0x801391fc */
        "addiu $8,$8,1\n\t"   /* 80138BD0: 25080001 addiu t0,t0,1 */
        ".L801388B8_80138BD4:\n\t"
        "lw $14,84($29)\n\t"   /* 80138BD4: 8fae0054 lw t6,84(sp) */
        "lw $15,640($29)\n\t"   /* 80138BD8: 8faf0280 lw t7,640(sp) */
        "sll $24,$14,0x19\n\t"   /* 80138BDC: 000ec640 sll t8,t6,0x19 */
        "srl $25,$24,0x1f\n\t"   /* 80138BE0: 0018cfc2 srl t9,t8,0x1f */
        "addu $6,$0,$0\n\t"   /* 80138BE4: 00003021 move a2,zero */
        "beq $25,$0,.L801388B8_80138BFC\n\t"   /* 80138BE8: 13200004 beqz t9,0x80138bfc */
        "addu $9,$15,$10\n\t"   /* 80138BEC: 01ea4821 addu t1,t7,t2 */
        "lw $3,0($12)\n\t"   /* 80138BF0: 8d830000 lw v1,0(t4) */
        "beq $0,$0,.L801388B8_80138C04\n\t"   /* 80138BF4: 10000003 b 0x80138c04 */
        "addiu $12,$12,4\n\t"   /* 80138BF8: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138BFC:\n\t"
        "lw $3,0($12)\n\t"   /* 80138BFC: 8d830000 lw v1,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138C00: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138C04:\n\t"
        "lw $15,84($29)\n\t"   /* 80138C04: 8faf0054 lw t7,84(sp) */
        "nop\n\t"   /* 80138C08: 00000000 nop  */
        "sll $14,$15,0x1a\n\t"   /* 80138C0C: 000f7680 sll t6,t7,0x1a */
        "srl $24,$14,0x1f\n\t"   /* 80138C10: 000ec7c2 srl t8,t6,0x1f */
        "beq $24,$0,.L801388B8_80138C24\n\t"   /* 80138C14: 13000003 beqz t8,0x80138c24 */
        "nop\n\t"   /* 80138C18: 00000000 nop  */
        "sll $3,$3,0x10\n\t"   /* 80138C1C: 00031c00 sll v1,v1,0x10 */
        "sra $3,$3,0x10\n\t"   /* 80138C20: 00031c03 sra v1,v1,0x10 */
        ".L801388B8_80138C24:\n\t"
        "bgez $3,.L801388B8_80138C3C\n\t"   /* 80138C24: 04610005 bgez v1,0x80138c3c */
        "lw $25,84($29)\n\t"   /* 80138C28: 8fb90054 lw t9,84(sp) */
        "subu $3,$0,$3\n\t"   /* 80138C2C: 00031823 negu v1,v1 */
        "beq $0,$0,.L801388B8_80138CA8\n\t"   /* 80138C30: 1000001d b 0x80138ca8 */
        "sb $18,85($29)\n\t"   /* 80138C34: a3b20055 sb s2,85(sp) */
        "lw $25,84($29)\n\t"   /* 80138C38: 8fb90054 lw t9,84(sp) */
        ".L801388B8_80138C3C:\n\t"
        "nop\n\t"   /* 80138C3C: 00000000 nop  */
        "sll $15,$25,0x1e\n\t"   /* 80138C40: 00197f80 sll t7,t9,0x1e */
        "srl $14,$15,0x1f\n\t"   /* 80138C44: 000f77c2 srl t6,t7,0x1f */
        "beq $14,$0,.L801388B8_80138CAC\n\t"   /* 80138C48: 11c00018 beqz t6,0x80138cac */
        "lw $14,84($29)\n\t"   /* 80138C4C: 8fae0054 lw t6,84(sp) */
        "beq $0,$0,.L801388B8_80138CA8\n\t"   /* 80138C50: 10000015 b 0x80138ca8 */
        "sb $19,85($29)\n\t"   /* 80138C54: a3b30055 sb s3,85(sp) */
        ".L801388B8_80138C58:\n\t"
        "lw $25,84($29)\n\t"   /* 80138C58: 8fb90054 lw t9,84(sp) */
        "lw $24,640($29)\n\t"   /* 80138C5C: 8fb80280 lw t8,640(sp) */
        "sll $15,$25,0x19\n\t"   /* 80138C60: 00197e40 sll t7,t9,0x19 */
        "srl $14,$15,0x1f\n\t"   /* 80138C64: 000f77c2 srl t6,t7,0x1f */
        "addu $6,$0,$0\n\t"   /* 80138C68: 00003021 move a2,zero */
        "beq $14,$0,.L801388B8_80138C80\n\t"   /* 80138C6C: 11c00004 beqz t6,0x80138c80 */
        "addu $9,$24,$10\n\t"   /* 80138C70: 030a4821 addu t1,t8,t2 */
        "lw $3,0($12)\n\t"   /* 80138C74: 8d830000 lw v1,0(t4) */
        "beq $0,$0,.L801388B8_80138C88\n\t"   /* 80138C78: 10000003 b 0x80138c88 */
        "addiu $12,$12,4\n\t"   /* 80138C7C: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138C80:\n\t"
        "lw $3,0($12)\n\t"   /* 80138C80: 8d830000 lw v1,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138C84: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138C88:\n\t"
        "lw $24,84($29)\n\t"   /* 80138C88: 8fb80054 lw t8,84(sp) */
        "nop\n\t"   /* 80138C8C: 00000000 nop  */
        "sll $25,$24,0x1a\n\t"   /* 80138C90: 0018ce80 sll t9,t8,0x1a */
        "srl $15,$25,0x1f\n\t"   /* 80138C94: 00197fc2 srl t7,t9,0x1f */
        "beq $15,$0,.L801388B8_80138CA4\n\t"   /* 80138C98: 11e00002 beqz t7,0x80138ca4 */
        "nop\n\t"   /* 80138C9C: 00000000 nop  */
        "andi $3,$3,0xffff\n\t"   /* 80138CA0: 3063ffff andi v1,v1,0xffff */
        ".L801388B8_80138CA4:\n\t"
        "sb $0,85($29)\n\t"   /* 80138CA4: a3a00055 sb zero,85(sp) */
        ".L801388B8_80138CA8:\n\t"
        "lw $14,84($29)\n\t"   /* 80138CA8: 8fae0054 lw t6,84(sp) */
        ".L801388B8_80138CAC:\n\t"
        "nop\n\t"   /* 80138CAC: 00000000 nop  */
        "sll $24,$14,0x1b\n\t"   /* 80138CB0: 000ec6c0 sll t8,t6,0x1b */
        "srl $25,$24,0x1f\n\t"   /* 80138CB4: 0018cfc2 srl t9,t8,0x1f */
        "bne $25,$0,.L801388B8_80138D04\n\t"   /* 80138CB8: 17200012 bnez t9,0x80138d04 */
        "nop\n\t"   /* 80138CBC: 00000000 nop  */
        "sll $15,$14,0x1c\n\t"   /* 80138CC0: 000e7f00 sll t7,t6,0x1c */
        "srl $24,$15,0x1f\n\t"   /* 80138CC4: 000fc7c2 srl t8,t7,0x1f */
        "beq $24,$0,.L801388B8_80138CF0\n\t"   /* 80138CC8: 13000009 beqz t8,0x80138cf0 */
        "lw $24,92($29)\n\t"   /* 80138CCC: 8fb8005c lw t8,92(sp) */
        "lw $25,88($29)\n\t"   /* 80138CD0: 8fb90058 lw t9,88(sp) */
        "lb $14,85($29)\n\t"   /* 80138CD4: 83ae0055 lb t6,85(sp) */
        "sw $25,92($29)\n\t"   /* 80138CD8: afb9005c sw t9,92(sp) */
        "beq $14,$0,.L801388B8_80138CF0\n\t"   /* 80138CDC: 11c00004 beqz t6,0x80138cf0 */
        "lw $24,92($29)\n\t"   /* 80138CE0: 8fb8005c lw t8,92(sp) */
        "addiu $15,$25,-1\n\t"   /* 80138CE4: 272fffff addiu t7,t9,-1 */
        "sw $15,92($29)\n\t"   /* 80138CE8: afaf005c sw t7,92(sp) */
        "lw $24,92($29)\n\t"   /* 80138CEC: 8fb8005c lw t8,92(sp) */
        ".L801388B8_80138CF0:\n\t"
        "nop\n\t"   /* 80138CF0: 00000000 nop  */
        "bgtz $24,.L801388B8_80138D04\n\t"   /* 80138CF4: 1f000003 bgtz t8,0x80138d04 */
        "nop\n\t"   /* 80138CF8: 00000000 nop  */
        "addiu $14,$0,1\n\t"   /* 80138CFC: 240e0001 li t6,1 */
        "sw $14,92($29)\n\t"   /* 80138D00: afae005c sw t6,92(sp) */
        ".L801388B8_80138D04:\n\t"
        "beq $3,$0,.L801388B8_80138D30\n\t"   /* 80138D04: 1060000a beqz v1,0x80138d30 */
        "addiu $1,$0,10\n\t"   /* 80138D08: 2401000a li at,10 */
        ".L801388B8_80138D0C:\n\t"
        "divu $0,$3,$1\n\t"   /* 80138D0C: 0061001b divu zero,v1,at */
        "addiu $7,$7,-1\n\t"   /* 80138D10: 24e7ffff addiu a3,a3,-1 */
        "addiu $6,$6,1\n\t"   /* 80138D14: 24c60001 addiu a2,a2,1 */
        "mfhi $25\n\t"   /* 80138D18: 0000c810 mfhi t9 */
        "addiu $15,$25,48\n\t"   /* 80138D1C: 272f0030 addiu t7,t9,48 */
        "mflo $3\n\t"   /* 80138D20: 00001812 mflo v1 */
        "sb $15,0($7)\n\t"   /* 80138D24: a0ef0000 sb t7,0(a3) */
        "bne $3,$0,.L801388B8_80138D0C\n\t"   /* 80138D28: 1460fff8 bnez v1,0x80138d0c */
        "addiu $1,$0,10\n\t"   /* 80138D2C: 2401000a li at,10 */
        ".L801388B8_80138D30:\n\t"
        "lw $24,92($29)\n\t"   /* 80138D30: 8fb8005c lw t8,92(sp) */
        "nop\n\t"   /* 80138D34: 00000000 nop  */
        "slt $1,$6,$24\n\t"   /* 80138D38: 00d8082a slt at,a2,t8 */
        "beq $1,$0,.L801388B8_80138D60\n\t"   /* 80138D3C: 10200008 beqz at,0x80138d60 */
        "lb $25,85($29)\n\t"   /* 80138D40: 83b90055 lb t9,85(sp) */
        ".L801388B8_80138D44:\n\t"
        "sb $11,-1($7)\n\t"   /* 80138D44: a0ebffff sb t3,-1(a3) */
        "lw $14,92($29)\n\t"   /* 80138D48: 8fae005c lw t6,92(sp) */
        "addiu $6,$6,1\n\t"   /* 80138D4C: 24c60001 addiu a2,a2,1 */
        "slt $1,$6,$14\n\t"   /* 80138D50: 00ce082a slt at,a2,t6 */
        "bne $1,$0,.L801388B8_80138D44\n\t"   /* 80138D54: 1420fffb bnez at,0x80138d44 */
        "addiu $7,$7,-1\n\t"   /* 80138D58: 24e7ffff addiu a3,a3,-1 */
        "lb $25,85($29)\n\t"   /* 80138D5C: 83b90055 lb t9,85(sp) */
        ".L801388B8_80138D60:\n\t"
        "nop\n\t"   /* 80138D60: 00000000 nop  */
        "beq $25,$0,.L801388B8_801392E8\n\t"   /* 80138D64: 13200160 beqz t9,0x801392e8 */
        "lw $14,88($29)\n\t"   /* 80138D68: 8fae0058 lw t6,88(sp) */
        "addiu $7,$7,-1\n\t"   /* 80138D6C: 24e7ffff addiu a3,a3,-1 */
        "sb $25,0($7)\n\t"   /* 80138D70: a0f90000 sb t9,0(a3) */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 80138D74: 1000015b b 0x801392e4 */
        "addiu $6,$6,1\n\t"   /* 80138D78: 24c60001 addiu a2,a2,1 */
        "lw $24,84($29)\n\t"   /* 80138D7C: 8fb80054 lw t8,84(sp) */
        ".L801388B8_80138D80:\n\t"
        "lw $15,640($29)\n\t"   /* 80138D80: 8faf0280 lw t7,640(sp) */
        "sll $14,$24,0x19\n\t"   /* 80138D84: 00187640 sll t6,t8,0x19 */
        "srl $25,$14,0x1f\n\t"   /* 80138D88: 000ecfc2 srl t9,t6,0x1f */
        "addu $6,$0,$0\n\t"   /* 80138D8C: 00003021 move a2,zero */
        "beq $25,$0,.L801388B8_80138DA4\n\t"   /* 80138D90: 13200004 beqz t9,0x80138da4 */
        "addu $9,$15,$10\n\t"   /* 80138D94: 01ea4821 addu t1,t7,t2 */
        "lw $3,0($12)\n\t"   /* 80138D98: 8d830000 lw v1,0(t4) */
        "beq $0,$0,.L801388B8_80138DAC\n\t"   /* 80138D9C: 10000003 b 0x80138dac */
        "addiu $12,$12,4\n\t"   /* 80138DA0: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138DA4:\n\t"
        "lw $3,0($12)\n\t"   /* 80138DA4: 8d830000 lw v1,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138DA8: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138DAC:\n\t"
        "lw $15,84($29)\n\t"   /* 80138DAC: 8faf0054 lw t7,84(sp) */
        "nop\n\t"   /* 80138DB0: 00000000 nop  */
        "sll $24,$15,0x1a\n\t"   /* 80138DB4: 000fc680 sll t8,t7,0x1a */
        "srl $14,$24,0x1f\n\t"   /* 80138DB8: 001877c2 srl t6,t8,0x1f */
        "beq $14,$0,.L801388B8_80138DCC\n\t"   /* 80138DBC: 11c00003 beqz t6,0x80138dcc */
        "lw $25,84($29)\n\t"   /* 80138DC0: 8fb90054 lw t9,84(sp) */
        "andi $3,$3,0xffff\n\t"   /* 80138DC4: 3063ffff andi v1,v1,0xffff */
        "lw $25,84($29)\n\t"   /* 80138DC8: 8fb90054 lw t9,84(sp) */
        ".L801388B8_80138DCC:\n\t"
        "nop\n\t"   /* 80138DCC: 00000000 nop  */
        "sll $15,$25,0x1b\n\t"   /* 80138DD0: 00197ec0 sll t7,t9,0x1b */
        "srl $24,$15,0x1f\n\t"   /* 80138DD4: 000fc7c2 srl t8,t7,0x1f */
        "bne $24,$0,.L801388B8_80138E14\n\t"   /* 80138DD8: 1700000e bnez t8,0x80138e14 */
        "nop\n\t"   /* 80138DDC: 00000000 nop  */
        "sll $14,$25,0x1c\n\t"   /* 80138DE0: 00197700 sll t6,t9,0x1c */
        "srl $15,$14,0x1f\n\t"   /* 80138DE4: 000e7fc2 srl t7,t6,0x1f */
        "beq $15,$0,.L801388B8_80138E00\n\t"   /* 80138DE8: 11e00005 beqz t7,0x80138e00 */
        "lw $25,92($29)\n\t"   /* 80138DEC: 8fb9005c lw t9,92(sp) */
        "lw $24,88($29)\n\t"   /* 80138DF0: 8fb80058 lw t8,88(sp) */
        "nop\n\t"   /* 80138DF4: 00000000 nop  */
        "sw $24,92($29)\n\t"   /* 80138DF8: afb8005c sw t8,92(sp) */
        "lw $25,92($29)\n\t"   /* 80138DFC: 8fb9005c lw t9,92(sp) */
        ".L801388B8_80138E00:\n\t"
        "nop\n\t"   /* 80138E00: 00000000 nop  */
        "bgtz $25,.L801388B8_80138E14\n\t"   /* 80138E04: 1f200003 bgtz t9,0x80138e14 */
        "nop\n\t"   /* 80138E08: 00000000 nop  */
        "addiu $14,$0,1\n\t"   /* 80138E0C: 240e0001 li t6,1 */
        "sw $14,92($29)\n\t"   /* 80138E10: afae005c sw t6,92(sp) */
        ".L801388B8_80138E14:\n\t"
        "beq $3,$0,.L801388B8_80138E3C\n\t"   /* 80138E14: 10600009 beqz v1,0x80138e3c */
        "lw $25,84($29)\n\t"   /* 80138E18: 8fb90054 lw t9,84(sp) */
        ".L801388B8_80138E1C:\n\t"
        "andi $15,$3,0x7\n\t"   /* 80138E1C: 306f0007 andi t7,v1,0x7 */
        "addiu $24,$15,48\n\t"   /* 80138E20: 25f80030 addiu t8,t7,48 */
        "srl $3,$3,0x3\n\t"   /* 80138E24: 000318c2 srl v1,v1,0x3 */
        "sb $24,-1($7)\n\t"   /* 80138E28: a0f8ffff sb t8,-1(a3) */
        "addiu $7,$7,-1\n\t"   /* 80138E2C: 24e7ffff addiu a3,a3,-1 */
        "bne $3,$0,.L801388B8_80138E1C\n\t"   /* 80138E30: 1460fffa bnez v1,0x80138e1c */
        "addiu $6,$6,1\n\t"   /* 80138E34: 24c60001 addiu a2,a2,1 */
        "lw $25,84($29)\n\t"   /* 80138E38: 8fb90054 lw t9,84(sp) */
        ".L801388B8_80138E3C:\n\t"
        "nop\n\t"   /* 80138E3C: 00000000 nop  */
        "sll $14,$25,0x1d\n\t"   /* 80138E40: 00197740 sll t6,t9,0x1d */
        "srl $15,$14,0x1f\n\t"   /* 80138E44: 000e7fc2 srl t7,t6,0x1f */
        "beq $15,$0,.L801388B8_80138E78\n\t"   /* 80138E48: 11e0000b beqz t7,0x80138e78 */
        "lw $25,92($29)\n\t"   /* 80138E4C: 8fb9005c lw t9,92(sp) */
        "beq $6,$0,.L801388B8_80138E78\n\t"   /* 80138E50: 10c00009 beqz a2,0x80138e78 */
        "lw $25,92($29)\n\t"   /* 80138E54: 8fb9005c lw t9,92(sp) */
        "lb $24,0($7)\n\t"   /* 80138E58: 80f80000 lb t8,0(a3) */
        "nop\n\t"   /* 80138E5C: 00000000 nop  */
        "beq $11,$24,.L801388B8_80138E78\n\t"   /* 80138E60: 11780005 beq t3,t8,0x80138e78 */
        "lw $25,92($29)\n\t"   /* 80138E64: 8fb9005c lw t9,92(sp) */
        "addiu $7,$7,-1\n\t"   /* 80138E68: 24e7ffff addiu a3,a3,-1 */
        "sb $11,0($7)\n\t"   /* 80138E6C: a0eb0000 sb t3,0(a3) */
        "addiu $6,$6,1\n\t"   /* 80138E70: 24c60001 addiu a2,a2,1 */
        "lw $25,92($29)\n\t"   /* 80138E74: 8fb9005c lw t9,92(sp) */
        ".L801388B8_80138E78:\n\t"
        "nop\n\t"   /* 80138E78: 00000000 nop  */
        "slt $1,$6,$25\n\t"   /* 80138E7C: 00d9082a slt at,a2,t9 */
        "beq $1,$0,.L801388B8_801392E8\n\t"   /* 80138E80: 10200119 beqz at,0x801392e8 */
        "lw $14,88($29)\n\t"   /* 80138E84: 8fae0058 lw t6,88(sp) */
        ".L801388B8_80138E88:\n\t"
        "sb $11,-1($7)\n\t"   /* 80138E88: a0ebffff sb t3,-1(a3) */
        "lw $14,92($29)\n\t"   /* 80138E8C: 8fae005c lw t6,92(sp) */
        "addiu $6,$6,1\n\t"   /* 80138E90: 24c60001 addiu a2,a2,1 */
        "slt $1,$6,$14\n\t"   /* 80138E94: 00ce082a slt at,a2,t6 */
        "bne $1,$0,.L801388B8_80138E88\n\t"   /* 80138E98: 1420fffb bnez at,0x80138e88 */
        "addiu $7,$7,-1\n\t"   /* 80138E9C: 24e7ffff addiu a3,a3,-1 */
        "beq $0,$0,.L801388B8_801392E8\n\t"   /* 80138EA0: 10000111 b 0x801392e8 */
        "lw $14,88($29)\n\t"   /* 80138EA4: 8fae0058 lw t6,88(sp) */
        ".L801388B8_80138EA8:\n\t"
        "lw $15,84($29)\n\t"   /* 80138EA8: 8faf0054 lw t7,84(sp) */
        "nop\n\t"   /* 80138EAC: 00000000 nop  */
        "and $24,$15,$17\n\t"   /* 80138EB0: 01f1c024 and t8,t7,s1 */
        "ori $25,$24,0x40\n\t"   /* 80138EB4: 37190040 ori t9,t8,0x40 */
        "sll $14,$25,0x19\n\t"   /* 80138EB8: 00197640 sll t6,t9,0x19 */
        "srl $15,$14,0x1f\n\t"   /* 80138EBC: 000e7fc2 srl t7,t6,0x1f */
        "srl $24,$25,0x4\n\t"   /* 80138EC0: 0019c102 srl t8,t9,0x4 */
        "xor $14,$15,$24\n\t"   /* 80138EC4: 01f87026 xor t6,t7,t8 */
        "sll $15,$14,0x1f\n\t"   /* 80138EC8: 000e7fc0 sll t7,t6,0x1f */
        "srl $24,$15,0x1b\n\t"   /* 80138ECC: 000fc6c2 srl t8,t7,0x1b */
        "sw $25,84($29)\n\t"   /* 80138ED0: afb90054 sw t9,84(sp) */
        "xor $14,$24,$25\n\t"   /* 80138ED4: 03197026 xor t6,t8,t9 */
        "addiu $15,$0,8\n\t"   /* 80138ED8: 240f0008 li t7,8 */
        "sw $14,84($29)\n\t"   /* 80138EDC: afae0054 sw t6,84(sp) */
        "sw $15,92($29)\n\t"   /* 80138EE0: afaf005c sw t7,92(sp) */
        ".L801388B8_80138EE4:\n\t"
        "lw $24,640($29)\n\t"   /* 80138EE4: 8fb80280 lw t8,640(sp) */
        "lui $4,%hi(D_80164978)\n\t"   /* 80138EE8: 3c048016 lui a0,0x8016 */
        "addiu $4,$4,%lo(D_80164978)\n\t"   /* 80138EEC: 24844978 addiu a0,a0,18808 */
        "addu $6,$0,$0\n\t"   /* 80138EF0: 00003021 move a2,zero */
        "beq $0,$0,.L801388B8_80138F10\n\t"   /* 80138EF4: 10000006 b 0x80138f10 */
        "addu $9,$24,$10\n\t"   /* 80138EF8: 030a4821 addu t1,t8,t2 */
        "lw $25,640($29)\n\t"   /* 80138EFC: 8fb90280 lw t9,640(sp) */
        ".L801388B8_80138F00:\n\t"
        "lui $4,%hi(D_8016498C)\n\t"   /* 80138F00: 3c048016 lui a0,0x8016 */
        "addiu $4,$4,%lo(D_8016498C)\n\t"   /* 80138F04: 2484498c addiu a0,a0,18828 */
        "addu $6,$0,$0\n\t"   /* 80138F08: 00003021 move a2,zero */
        "addu $9,$25,$10\n\t"   /* 80138F0C: 032a4821 addu t1,t9,t2 */
        ".L801388B8_80138F10:\n\t"
        "lw $14,84($29)\n\t"   /* 80138F10: 8fae0054 lw t6,84(sp) */
        "nop\n\t"   /* 80138F14: 00000000 nop  */
        "sll $15,$14,0x19\n\t"   /* 80138F18: 000e7e40 sll t7,t6,0x19 */
        "srl $24,$15,0x1f\n\t"   /* 80138F1C: 000fc7c2 srl t8,t7,0x1f */
        "beq $24,$0,.L801388B8_80138F34\n\t"   /* 80138F20: 13000004 beqz t8,0x80138f34 */
        "nop\n\t"   /* 80138F24: 00000000 nop  */
        "lw $3,0($12)\n\t"   /* 80138F28: 8d830000 lw v1,0(t4) */
        "beq $0,$0,.L801388B8_80138F3C\n\t"   /* 80138F2C: 10000003 b 0x80138f3c */
        "addiu $12,$12,4\n\t"   /* 80138F30: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138F34:\n\t"
        "lw $3,0($12)\n\t"   /* 80138F34: 8d830000 lw v1,0(t4) */
        "addiu $12,$12,4\n\t"   /* 80138F38: 258c0004 addiu t4,t4,4 */
        ".L801388B8_80138F3C:\n\t"
        "lw $25,84($29)\n\t"   /* 80138F3C: 8fb90054 lw t9,84(sp) */
        "nop\n\t"   /* 80138F40: 00000000 nop  */
        "sll $14,$25,0x1a\n\t"   /* 80138F44: 00197680 sll t6,t9,0x1a */
        "srl $15,$14,0x1f\n\t"   /* 80138F48: 000e7fc2 srl t7,t6,0x1f */
        "beq $15,$0,.L801388B8_80138F5C\n\t"   /* 80138F4C: 11e00003 beqz t7,0x80138f5c */
        "lw $24,84($29)\n\t"   /* 80138F50: 8fb80054 lw t8,84(sp) */
        "andi $3,$3,0xffff\n\t"   /* 80138F54: 3063ffff andi v1,v1,0xffff */
        "lw $24,84($29)\n\t"   /* 80138F58: 8fb80054 lw t8,84(sp) */
        ".L801388B8_80138F5C:\n\t"
        "nop\n\t"   /* 80138F5C: 00000000 nop  */
        "sll $25,$24,0x1b\n\t"   /* 80138F60: 0018cec0 sll t9,t8,0x1b */
        "srl $14,$25,0x1f\n\t"   /* 80138F64: 001977c2 srl t6,t9,0x1f */
        "bne $14,$0,.L801388B8_80138FB4\n\t"   /* 80138F68: 15c00012 bnez t6,0x80138fb4 */
        "nop\n\t"   /* 80138F6C: 00000000 nop  */
        "sll $15,$24,0x1c\n\t"   /* 80138F70: 00187f00 sll t7,t8,0x1c */
        "srl $25,$15,0x1f\n\t"   /* 80138F74: 000fcfc2 srl t9,t7,0x1f */
        "beq $25,$0,.L801388B8_80138FA0\n\t"   /* 80138F78: 13200009 beqz t9,0x80138fa0 */
        "lw $15,92($29)\n\t"   /* 80138F7C: 8faf005c lw t7,92(sp) */
        "lw $14,88($29)\n\t"   /* 80138F80: 8fae0058 lw t6,88(sp) */
        "sll $15,$24,0x1d\n\t"   /* 80138F84: 00187f40 sll t7,t8,0x1d */
        "srl $25,$15,0x1f\n\t"   /* 80138F88: 000fcfc2 srl t9,t7,0x1f */
        "beq $25,$0,.L801388B8_80138F9C\n\t"   /* 80138F8C: 13200003 beqz t9,0x80138f9c */
        "sw $14,92($29)\n\t"   /* 80138F90: afae005c sw t6,92(sp) */
        "addiu $24,$14,-2\n\t"   /* 80138F94: 25d8fffe addiu t8,t6,-2 */
        "sw $24,92($29)\n\t"   /* 80138F98: afb8005c sw t8,92(sp) */
        ".L801388B8_80138F9C:\n\t"
        "lw $15,92($29)\n\t"   /* 80138F9C: 8faf005c lw t7,92(sp) */
        ".L801388B8_80138FA0:\n\t"
        "nop\n\t"   /* 80138FA0: 00000000 nop  */
        "bgtz $15,.L801388B8_80138FB4\n\t"   /* 80138FA4: 1de00003 bgtz t7,0x80138fb4 */
        "nop\n\t"   /* 80138FA8: 00000000 nop  */
        "addiu $25,$0,1\n\t"   /* 80138FAC: 24190001 li t9,1 */
        "sw $25,92($29)\n\t"   /* 80138FB0: afb9005c sw t9,92(sp) */
        ".L801388B8_80138FB4:\n\t"
        "beq $3,$0,.L801388B8_80138FE0\n\t"   /* 80138FB4: 1060000a beqz v1,0x80138fe0 */
        "lw $25,92($29)\n\t"   /* 80138FB8: 8fb9005c lw t9,92(sp) */
        ".L801388B8_80138FBC:\n\t"
        "andi $14,$3,0xf\n\t"   /* 80138FBC: 306e000f andi t6,v1,0xf */
        "addu $24,$4,$14\n\t"   /* 80138FC0: 008ec021 addu t8,a0,t6 */
        "lb $15,0($24)\n\t"   /* 80138FC4: 830f0000 lb t7,0(t8) */
        "srl $3,$3,0x4\n\t"   /* 80138FC8: 00031902 srl v1,v1,0x4 */
        "addiu $7,$7,-1\n\t"   /* 80138FCC: 24e7ffff addiu a3,a3,-1 */
        "addiu $6,$6,1\n\t"   /* 80138FD0: 24c60001 addiu a2,a2,1 */
        "bne $3,$0,.L801388B8_80138FBC\n\t"   /* 80138FD4: 1460fff9 bnez v1,0x80138fbc */
        "sb $15,0($7)\n\t"   /* 80138FD8: a0ef0000 sb t7,0(a3) */
        "lw $25,92($29)\n\t"   /* 80138FDC: 8fb9005c lw t9,92(sp) */
        ".L801388B8_80138FE0:\n\t"
        "nop\n\t"   /* 80138FE0: 00000000 nop  */
        "slt $1,$6,$25\n\t"   /* 80138FE4: 00d9082a slt at,a2,t9 */
        "beq $1,$0,.L801388B8_8013900C\n\t"   /* 80138FE8: 10200008 beqz at,0x8013900c */
        "lw $24,84($29)\n\t"   /* 80138FEC: 8fb80054 lw t8,84(sp) */
        ".L801388B8_80138FF0:\n\t"
        "sb $11,-1($7)\n\t"   /* 80138FF0: a0ebffff sb t3,-1(a3) */
        "lw $14,92($29)\n\t"   /* 80138FF4: 8fae005c lw t6,92(sp) */
        "addiu $6,$6,1\n\t"   /* 80138FF8: 24c60001 addiu a2,a2,1 */
        "slt $1,$6,$14\n\t"   /* 80138FFC: 00ce082a slt at,a2,t6 */
        "bne $1,$0,.L801388B8_80138FF0\n\t"   /* 80139000: 1420fffb bnez at,0x80138ff0 */
        "addiu $7,$7,-1\n\t"   /* 80139004: 24e7ffff addiu a3,a3,-1 */
        "lw $24,84($29)\n\t"   /* 80139008: 8fb80054 lw t8,84(sp) */
        ".L801388B8_8013900C:\n\t"
        "nop\n\t"   /* 8013900C: 00000000 nop  */
        "sll $15,$24,0x1d\n\t"   /* 80139010: 00187f40 sll t7,t8,0x1d */
        "srl $25,$15,0x1f\n\t"   /* 80139014: 000fcfc2 srl t9,t7,0x1f */
        "beq $25,$0,.L801388B8_801392E8\n\t"   /* 80139018: 132000b3 beqz t9,0x801392e8 */
        "lw $14,88($29)\n\t"   /* 8013901C: 8fae0058 lw t6,88(sp) */
        "addiu $7,$7,-1\n\t"   /* 80139020: 24e7ffff addiu a3,a3,-1 */
        "sb $2,0($7)\n\t"   /* 80139024: a0e20000 sb v0,0(a3) */
        "addiu $7,$7,-1\n\t"   /* 80139028: 24e7ffff addiu a3,a3,-1 */
        "sb $11,0($7)\n\t"   /* 8013902C: a0eb0000 sb t3,0(a3) */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 80139030: 100000ac b 0x801392e4 */
        "addiu $6,$6,2\n\t"   /* 80139034: 24c60002 addiu a2,a2,2 */
        ".L801388B8_80139038:\n\t"
        "lw $14,0($12)\n\t"   /* 80139038: 8d8e0000 lw t6,0(t4) */
        "addiu $7,$29,615\n\t"   /* 8013903C: 27a70267 addiu a3,sp,615 */
        "sb $14,0($7)\n\t"   /* 80139040: a0ee0000 sb t6,0(a3) */
        "lw $24,640($29)\n\t"   /* 80139044: 8fb80280 lw t8,640(sp) */
        "addiu $12,$12,4\n\t"   /* 80139048: 258c0004 addiu t4,t4,4 */
        "addiu $6,$0,1\n\t"   /* 8013904C: 24060001 li a2,1 */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 80139050: 100000a4 b 0x801392e4 */
        "addu $9,$24,$10\n\t"   /* 80139054: 030a4821 addu t1,t8,t2 */
        "lw $25,84($29)\n\t"   /* 80139058: 8fb90054 lw t9,84(sp) */
        ".L801388B8_8013905C:\n\t"
        "lw $15,640($29)\n\t"   /* 8013905C: 8faf0280 lw t7,640(sp) */
        "sll $14,$25,0x1d\n\t"   /* 80139060: 00197740 sll t6,t9,0x1d */
        "lw $7,0($12)\n\t"   /* 80139064: 8d870000 lw a3,0(t4) */
        "srl $24,$14,0x1f\n\t"   /* 80139068: 000ec7c2 srl t8,t6,0x1f */
        "addiu $12,$12,4\n\t"   /* 8013906C: 258c0004 addiu t4,t4,4 */
        "beq $24,$0,.L801388B8_801390A8\n\t"   /* 80139070: 1300000d beqz t8,0x801390a8 */
        "addu $9,$15,$10\n\t"   /* 80139074: 01ea4821 addu t1,t7,t2 */
        "sll $15,$25,0x1b\n\t"   /* 80139078: 00197ec0 sll t7,t9,0x1b */
        "lbu $6,0($7)\n\t"   /* 8013907C: 90e60000 lbu a2,0(a3) */
        "srl $14,$15,0x1f\n\t"   /* 80139080: 000f77c2 srl t6,t7,0x1f */
        "beq $14,$0,.L801388B8_801392E4\n\t"   /* 80139084: 11c00097 beqz t6,0x801392e4 */
        "addiu $7,$7,1\n\t"   /* 80139088: 24e70001 addiu a3,a3,1 */
        "lw $24,92($29)\n\t"   /* 8013908C: 8fb8005c lw t8,92(sp) */
        "nop\n\t"   /* 80139090: 00000000 nop  */
        "slt $1,$24,$6\n\t"   /* 80139094: 0306082a slt at,t8,a2 */
        "beq $1,$0,.L801388B8_801392E8\n\t"   /* 80139098: 10200093 beqz at,0x801392e8 */
        "lw $14,88($29)\n\t"   /* 8013909C: 8fae0058 lw t6,88(sp) */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 801390A0: 10000090 b 0x801392e4 */
        "addu $6,$24,$0\n\t"   /* 801390A4: 03003021 move a2,t8 */
        ".L801388B8_801390A8:\n\t"
        "lw $25,84($29)\n\t"   /* 801390A8: 8fb90054 lw t9,84(sp) */
        "nop\n\t"   /* 801390AC: 00000000 nop  */
        "sll $15,$25,0x1b\n\t"   /* 801390B0: 00197ec0 sll t7,t9,0x1b */
        "srl $14,$15,0x1f\n\t"   /* 801390B4: 000f77c2 srl t6,t7,0x1f */
        "bne $14,$0,.L801388B8_80139104\n\t"   /* 801390B8: 15c00012 bnez t6,0x80139104 */
        "nop\n\t"   /* 801390BC: 00000000 nop  */
        "sw $7,616($29)\n\t"   /* 801390C0: afa70268 sw a3,616(sp) */
        "sw $8,644($29)\n\t"   /* 801390C4: afa80284 sw t0,644(sp) */
        "sw $9,72($29)\n\t"   /* 801390C8: afa90048 sw t1,72(sp) */
        "sw $10,624($29)\n\t"   /* 801390CC: afaa0270 sw t2,624(sp) */
        "addu $4,$7,$0\n\t"   /* 801390D0: 00e02021 move a0,a3 */
        "jal sub_801393F8\n\t"   /* 801390D4: 0c04e4fe jal 0x801393f8 */
        "sw $12,80($29)\n\t"   /* 801390D8: afac0050 sw t4,80(sp) */
        "lw $7,616($29)\n\t"   /* 801390DC: 8fa70268 lw a3,616(sp) */
        "lw $8,644($29)\n\t"   /* 801390E0: 8fa80284 lw t0,644(sp) */
        "lw $9,72($29)\n\t"   /* 801390E4: 8fa90048 lw t1,72(sp) */
        "lw $10,624($29)\n\t"   /* 801390E8: 8faa0270 lw t2,624(sp) */
        "lw $12,80($29)\n\t"   /* 801390EC: 8fac0050 lw t4,80(sp) */
        "addiu $11,$0,48\n\t"   /* 801390F0: 240b0030 li t3,48 */
        "addiu $13,$0,32\n\t"   /* 801390F4: 240d0020 li t5,32 */
        "addiu $31,$0,-2\n\t"   /* 801390F8: 241ffffe li ra,-2 */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 801390FC: 10000079 b 0x801392e4 */
        "addu $6,$2,$0\n\t"   /* 80139100: 00403021 move a2,v0 */
        ".L801388B8_80139104:\n\t"
        "sw $7,616($29)\n\t"   /* 80139104: afa70268 sw a3,616(sp) */
        "sw $8,644($29)\n\t"   /* 80139108: afa80284 sw t0,644(sp) */
        "lw $6,92($29)\n\t"   /* 8013910C: 8fa6005c lw a2,92(sp) */
        "sw $9,72($29)\n\t"   /* 80139110: afa90048 sw t1,72(sp) */
        "sw $10,624($29)\n\t"   /* 80139114: afaa0270 sw t2,624(sp) */
        "addu $4,$7,$0\n\t"   /* 80139118: 00e02021 move a0,a3 */
        "addu $5,$0,$0\n\t"   /* 8013911C: 00002821 move a1,zero */
        "jal sub_80139408\n\t"   /* 80139120: 0c04e502 jal 0x80139408 */
        "sw $12,80($29)\n\t"   /* 80139124: afac0050 sw t4,80(sp) */
        "lw $7,616($29)\n\t"   /* 80139128: 8fa70268 lw a3,616(sp) */
        "lw $8,644($29)\n\t"   /* 8013912C: 8fa80284 lw t0,644(sp) */
        "lw $9,72($29)\n\t"   /* 80139130: 8fa90048 lw t1,72(sp) */
        "lw $10,624($29)\n\t"   /* 80139134: 8faa0270 lw t2,624(sp) */
        "lw $12,80($29)\n\t"   /* 80139138: 8fac0050 lw t4,80(sp) */
        "addiu $11,$0,48\n\t"   /* 8013913C: 240b0030 li t3,48 */
        "addiu $13,$0,32\n\t"   /* 80139140: 240d0020 li t5,32 */
        "beq $2,$0,.L801388B8_80139154\n\t"   /* 80139144: 10400003 beqz v0,0x80139154 */
        "addiu $31,$0,-2\n\t"   /* 80139148: 241ffffe li ra,-2 */
        "beq $0,$0,.L801388B8_801392E4\n\t"   /* 8013914C: 10000065 b 0x801392e4 */
        "subu $6,$2,$7\n\t"   /* 80139150: 00473023 subu a2,v0,a3 */
        ".L801388B8_80139154:\n\t"
        "lw $6,92($29)\n\t"   /* 80139154: 8fa6005c lw a2,92(sp) */
        "beq $0,$0,.L801388B8_801392E8\n\t"   /* 80139158: 10000063 b 0x801392e8 */
        "lw $14,88($29)\n\t"   /* 8013915C: 8fae0058 lw t6,88(sp) */
        ".L801388B8_80139160:\n\t"
        "lw $24,84($29)\n\t"   /* 80139160: 8fb80054 lw t8,84(sp) */
        "lw $7,0($12)\n\t"   /* 80139164: 8d870000 lw a3,0(t4) */
        "sll $25,$24,0x1a\n\t"   /* 80139168: 0018ce80 sll t9,t8,0x1a */
        "srl $15,$25,0x1f\n\t"   /* 8013916C: 00197fc2 srl t7,t9,0x1f */
        "beq $15,$0,.L801388B8_80139180\n\t"   /* 80139170: 11e00003 beqz t7,0x80139180 */
        "addiu $12,$12,4\n\t"   /* 80139174: 258c0004 addiu t4,t4,4 */
        "beq $0,$0,.L801388B8_8013939C\n\t"   /* 80139178: 10000088 b 0x8013939c */
        "sh $10,0($7)\n\t"   /* 8013917C: a4ea0000 sh t2,0(a3) */
        ".L801388B8_80139180:\n\t"
        "lw $14,84($29)\n\t"   /* 80139180: 8fae0054 lw t6,84(sp) */
        "nop\n\t"   /* 80139184: 00000000 nop  */
        "sll $24,$14,0x19\n\t"   /* 80139188: 000ec640 sll t8,t6,0x19 */
        "srl $25,$24,0x1f\n\t"   /* 8013918C: 0018cfc2 srl t9,t8,0x1f */
        "beq $25,$0,.L801388B8_801391A0\n\t"   /* 80139190: 13200003 beqz t9,0x801391a0 */
        "nop\n\t"   /* 80139194: 00000000 nop  */
        "beq $0,$0,.L801388B8_8013939C\n\t"   /* 80139198: 10000080 b 0x8013939c */
        "sw $10,0($7)\n\t"   /* 8013919C: acea0000 sw t2,0(a3) */
        ".L801388B8_801391A0:\n\t"
        "beq $0,$0,.L801388B8_8013939C\n\t"   /* 801391A0: 1000007e b 0x8013939c */
        "sw $10,0($7)\n\t"   /* 801391A4: acea0000 sw t2,0(a3) */
        "addiu $1,$0,37\n\t"   /* 801391A8: 24010025 li at,37 */
        ".L801388B8_801391AC:\n\t"
        "beq $2,$1,.L801388B8_801391E4\n\t"   /* 801391AC: 1041000d beq v0,at,0x801391e4 */
        "lw $15,640($29)\n\t"   /* 801391B0: 8faf0280 lw t7,640(sp) */
        "lw $16,68($29)\n\t"   /* 801391B4: 8fb00044 lw s0,68(sp) */
        "lw $17,64($29)\n\t"   /* 801391B8: 8fb10040 lw s1,64(sp) */
        "lw $18,60($29)\n\t"   /* 801391BC: 8fb2003c lw s2,60(sp) */
        "lw $19,56($29)\n\t"   /* 801391C0: 8fb30038 lw s3,56(sp) */
        "lw $20,52($29)\n\t"   /* 801391C4: 8fb40034 lw s4,52(sp) */
        "lw $21,48($29)\n\t"   /* 801391C8: 8fb50030 lw s5,48(sp) */
        "lw $22,44($29)\n\t"   /* 801391CC: 8fb6002c lw s6,44(sp) */
        "lw $23,40($29)\n\t"   /* 801391D0: 8fb70028 lw s7,40(sp) */
        "lw $30,36($29)\n\t"   /* 801391D4: 8fbe0024 lw s8,36(sp) */
        "beq $0,$0,.L801388B8_801393D8\n\t"   /* 801391D8: 1000007f b 0x801393d8 */
        "lw $14,640($29)\n\t"   /* 801391DC: 8fae0280 lw t6,640(sp) */
        "lw $15,640($29)\n\t"   /* 801391E0: 8faf0280 lw t7,640(sp) */
        ".L801388B8_801391E4:\n\t"
        "nop\n\t"   /* 801391E4: 00000000 nop  */
        "addu $9,$15,$10\n\t"   /* 801391E8: 01ea4821 addu t1,t7,t2 */
        ".L801388B8_801391EC:\n\t"
        "sb $2,0($9)\n\t"   /* 801391EC: a1220000 sb v0,0(t1) */
        "addiu $10,$10,1\n\t"   /* 801391F0: 254a0001 addiu t2,t2,1 */
        "beq $0,$0,.L801388B8_8013939C\n\t"   /* 801391F4: 10000069 b 0x8013939c */
        "addiu $9,$9,1\n\t"   /* 801391F8: 25290001 addiu t1,t1,1 */
        ".L801388B8_801391FC:\n\t"
        "beq $2,$23,.L801388B8_80139160\n\t"   /* 801391FC: 1057ffd8 beq v0,s7,0x80139160 */
        "slti $1,$2,111\n\t"   /* 80139200: 2841006f slti at,v0,111 */
        "beq $1,$0,.L801388B8_8013928C\n\t"   /* 80139204: 10200021 beqz at,0x8013928c */
        "addiu $1,$0,117\n\t"   /* 80139208: 24010075 li at,117 */
        "beq $2,$30,.L801388B8_80138BD4\n\t"   /* 8013920C: 105efe71 beq v0,s8,0x80138bd4 */
        "slti $1,$2,101\n\t"   /* 80139210: 28410065 slti at,v0,101 */
        "beq $1,$0,.L801388B8_80139250\n\t"   /* 80139214: 1020000e beqz at,0x80139250 */
        "addiu $1,$0,88\n\t"   /* 80139218: 24010058 li at,88 */
        "beq $2,$1,.L801388B8_80138EE4\n\t"   /* 8013921C: 1041ff31 beq v0,at,0x80138ee4 */
        "slti $1,$2,89\n\t"   /* 80139220: 28410059 slti at,v0,89 */
        "beq $1,$0,.L801388B8_8013923C\n\t"   /* 80139224: 10200005 beqz at,0x8013923c */
        "addiu $1,$0,76\n\t"   /* 80139228: 2401004c li at,76 */
        "beq $2,$1,.L801388B8_80138BB8\n\t"   /* 8013922C: 1041fe62 beq v0,at,0x80138bb8 */
        "lw $14,84($29)\n\t"   /* 80139230: 8fae0054 lw t6,84(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 80139234: 1000ffdd b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 80139238: 24010025 li at,37 */
        ".L801388B8_8013923C:\n\t"
        "addiu $1,$0,99\n\t"   /* 8013923C: 24010063 li at,99 */
        "beq $2,$1,.L801388B8_80139038\n\t"   /* 80139240: 1041ff7d beq v0,at,0x80139038 */
        "nop\n\t"   /* 80139244: 00000000 nop  */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 80139248: 1000ffd8 b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 8013924C: 24010025 li at,37 */
        ".L801388B8_80139250:\n\t"
        "addiu $1,$0,105\n\t"   /* 80139250: 24010069 li at,105 */
        "beq $2,$1,.L801388B8_80138BD4\n\t"   /* 80139254: 1041fe5f beq v0,at,0x80138bd4 */
        "slti $1,$2,106\n\t"   /* 80139258: 2841006a slti at,v0,106 */
        "beq $1,$0,.L801388B8_80139274\n\t"   /* 8013925C: 10200005 beqz at,0x80139274 */
        "addiu $1,$0,104\n\t"   /* 80139260: 24010068 li at,104 */
        "beq $2,$1,.L801388B8_80138B78\n\t"   /* 80139264: 1041fe44 beq v0,at,0x80138b78 */
        "lw $25,84($29)\n\t"   /* 80139268: 8fb90054 lw t9,84(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 8013926C: 1000ffcf b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 80139270: 24010025 li at,37 */
        ".L801388B8_80139274:\n\t"
        "addiu $1,$0,108\n\t"   /* 80139274: 2401006c li at,108 */
        "beq $2,$1,.L801388B8_80138B98\n\t"   /* 80139278: 1041fe47 beq v0,at,0x80138b98 */
        "lw $24,84($29)\n\t"   /* 8013927C: 8fb80054 lw t8,84(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 80139280: 1000ffca b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 80139284: 24010025 li at,37 */
        "addiu $1,$0,117\n\t"   /* 80139288: 24010075 li at,117 */
        ".L801388B8_8013928C:\n\t"
        "beq $2,$1,.L801388B8_80138C58\n\t"   /* 8013928C: 1041fe72 beq v0,at,0x80138c58 */
        "slti $1,$2,118\n\t"   /* 80139290: 28410076 slti at,v0,118 */
        "beq $1,$0,.L801388B8_801392D0\n\t"   /* 80139294: 1020000e beqz at,0x801392d0 */
        "addiu $1,$0,112\n\t"   /* 80139298: 24010070 li at,112 */
        "beq $2,$1,.L801388B8_80138EA8\n\t"   /* 8013929C: 1041ff02 beq v0,at,0x80138ea8 */
        "slti $1,$2,113\n\t"   /* 801392A0: 28410071 slti at,v0,113 */
        "beq $1,$0,.L801388B8_801392BC\n\t"   /* 801392A4: 10200005 beqz at,0x801392bc */
        "addiu $1,$0,111\n\t"   /* 801392A8: 2401006f li at,111 */
        "beq $2,$1,.L801388B8_80138D80\n\t"   /* 801392AC: 1041feb4 beq v0,at,0x80138d80 */
        "lw $24,84($29)\n\t"   /* 801392B0: 8fb80054 lw t8,84(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 801392B4: 1000ffbd b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 801392B8: 24010025 li at,37 */
        ".L801388B8_801392BC:\n\t"
        "addiu $1,$0,115\n\t"   /* 801392BC: 24010073 li at,115 */
        "beq $2,$1,.L801388B8_8013905C\n\t"   /* 801392C0: 1041ff66 beq v0,at,0x8013905c */
        "lw $25,84($29)\n\t"   /* 801392C4: 8fb90054 lw t9,84(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 801392C8: 1000ffb8 b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 801392CC: 24010025 li at,37 */
        ".L801388B8_801392D0:\n\t"
        "addiu $1,$0,120\n\t"   /* 801392D0: 24010078 li at,120 */
        "beq $2,$1,.L801388B8_80138F00\n\t"   /* 801392D4: 1041ff0a beq v0,at,0x80138f00 */
        "lw $25,640($29)\n\t"   /* 801392D8: 8fb90280 lw t9,640(sp) */
        "beq $0,$0,.L801388B8_801391AC\n\t"   /* 801392DC: 1000ffb3 b 0x801391ac */
        "addiu $1,$0,37\n\t"   /* 801392E0: 24010025 li at,37 */
        ".L801388B8_801392E4:\n\t"
        "lw $14,88($29)\n\t"   /* 801392E4: 8fae0058 lw t6,88(sp) */
        ".L801388B8_801392E8:\n\t"
        "nop\n\t"   /* 801392E8: 00000000 nop  */
        "slt $1,$6,$14\n\t"   /* 801392EC: 00ce082a slt at,a2,t6 */
        "beq $1,$0,.L801388B8_8013932C\n\t"   /* 801392F0: 1020000e beqz at,0x8013932c */
        "nop\n\t"   /* 801392F4: 00000000 nop  */
        "lw $24,84($29)\n\t"   /* 801392F8: 8fb80054 lw t8,84(sp) */
        "nop\n\t"   /* 801392FC: 00000000 nop  */
        "andi $25,$24,0x1\n\t"   /* 80139300: 33190001 andi t9,t8,0x1 */
        "bne $25,$0,.L801388B8_8013932C\n\t"   /* 80139304: 17200009 bnez t9,0x8013932c */
        "nop\n\t"   /* 80139308: 00000000 nop  */
        ".L801388B8_8013930C:\n\t"
        "sb $13,0($9)\n\t"   /* 8013930C: a12d0000 sb t5,0(t1) */
        "lw $15,88($29)\n\t"   /* 80139310: 8faf0058 lw t7,88(sp) */
        "addiu $10,$10,1\n\t"   /* 80139314: 254a0001 addiu t2,t2,1 */
        "addiu $14,$15,-1\n\t"   /* 80139318: 25eeffff addiu t6,t7,-1 */
        "slt $1,$6,$14\n\t"   /* 8013931C: 00ce082a slt at,a2,t6 */
        "addiu $9,$9,1\n\t"   /* 80139320: 25290001 addiu t1,t1,1 */
        "bne $1,$0,.L801388B8_8013930C\n\t"   /* 80139324: 1420fff9 bnez at,0x8013930c */
        "sw $14,88($29)\n\t"   /* 80139328: afae0058 sw t6,88(sp) */
        ".L801388B8_8013932C:\n\t"
        "sw $6,632($29)\n\t"   /* 8013932C: afa60278 sw a2,632(sp) */
        "sw $8,644($29)\n\t"   /* 80139330: afa80284 sw t0,644(sp) */
        "sw $10,624($29)\n\t"   /* 80139334: afaa0270 sw t2,624(sp) */
        "addu $4,$9,$0\n\t"   /* 80139338: 01202021 move a0,t1 */
        "addu $5,$7,$0\n\t"   /* 8013933C: 00e02821 move a1,a3 */
        "jal sub_80139418\n\t"   /* 80139340: 0c04e506 jal 0x80139418 */
        "sw $12,80($29)\n\t"   /* 80139344: afac0050 sw t4,80(sp) */
        "lw $6,632($29)\n\t"   /* 80139348: 8fa60278 lw a2,632(sp) */
        "lw $24,88($29)\n\t"   /* 8013934C: 8fb80058 lw t8,88(sp) */
        "lw $10,624($29)\n\t"   /* 80139350: 8faa0270 lw t2,624(sp) */
        "lw $8,644($29)\n\t"   /* 80139354: 8fa80284 lw t0,644(sp) */
        "lw $12,80($29)\n\t"   /* 80139358: 8fac0050 lw t4,80(sp) */
        "slt $1,$6,$24\n\t"   /* 8013935C: 00d8082a slt at,a2,t8 */
        "addiu $11,$0,48\n\t"   /* 80139360: 240b0030 li t3,48 */
        "addiu $13,$0,32\n\t"   /* 80139364: 240d0020 li t5,32 */
        "addiu $31,$0,-2\n\t"   /* 80139368: 241ffffe li ra,-2 */
        "beq $1,$0,.L801388B8_8013939C\n\t"   /* 8013936C: 1020000b beqz at,0x8013939c */
        "addu $10,$10,$6\n\t"   /* 80139370: 01465021 addu t2,t2,a2 */
        "lw $25,640($29)\n\t"   /* 80139374: 8fb90280 lw t9,640(sp) */
        "nop\n\t"   /* 80139378: 00000000 nop  */
        "addu $9,$25,$10\n\t"   /* 8013937C: 032a4821 addu t1,t9,t2 */
        ".L801388B8_80139380:\n\t"
        "sb $13,0($9)\n\t"   /* 80139380: a12d0000 sb t5,0(t1) */
        "lw $15,88($29)\n\t"   /* 80139384: 8faf0058 lw t7,88(sp) */
        "addiu $6,$6,1\n\t"   /* 80139388: 24c60001 addiu a2,a2,1 */
        "slt $1,$6,$15\n\t"   /* 8013938C: 00cf082a slt at,a2,t7 */
        "addiu $10,$10,1\n\t"   /* 80139390: 254a0001 addiu t2,t2,1 */
        "bne $1,$0,.L801388B8_80139380\n\t"   /* 80139394: 1420fffa bnez at,0x80139380 */
        "addiu $9,$9,1\n\t"   /* 80139398: 25290001 addiu t1,t1,1 */
        ".L801388B8_8013939C:\n\t"
        "lb $2,1($8)\n\t"   /* 8013939C: 81020001 lb v0,1(t0) */
        "addiu $8,$8,1\n\t"   /* 801393A0: 25080001 addiu t0,t0,1 */
        "bne $2,$0,.L801388B8_80138938\n\t"   /* 801393A4: 1440fd64 bnez v0,0x80138938 */
        "addiu $1,$0,37\n\t"   /* 801393A8: 24010025 li at,37 */
        "lw $16,68($29)\n\t"   /* 801393AC: 8fb00044 lw s0,68(sp) */
        "lw $17,64($29)\n\t"   /* 801393B0: 8fb10040 lw s1,64(sp) */
        "lw $18,60($29)\n\t"   /* 801393B4: 8fb2003c lw s2,60(sp) */
        "lw $19,56($29)\n\t"   /* 801393B8: 8fb30038 lw s3,56(sp) */
        "lw $20,52($29)\n\t"   /* 801393BC: 8fb40034 lw s4,52(sp) */
        "lw $21,48($29)\n\t"   /* 801393C0: 8fb50030 lw s5,48(sp) */
        "lw $22,44($29)\n\t"   /* 801393C4: 8fb6002c lw s6,44(sp) */
        "lw $23,40($29)\n\t"   /* 801393C8: 8fb70028 lw s7,40(sp) */
        "lw $30,36($29)\n\t"   /* 801393CC: 8fbe0024 lw s8,36(sp) */
        "nop\n\t"   /* 801393D0: 00000000 nop  */
        ".L801388B8_801393D4:\n\t"
        "lw $14,640($29)\n\t"   /* 801393D4: 8fae0280 lw t6,640(sp) */
        ".L801388B8_801393D8:\n\t"
        "addu $2,$10,$0\n\t"   /* 801393D8: 01401021 move v0,t2 */
        "addu $24,$14,$10\n\t"   /* 801393DC: 01cac021 addu t8,t6,t2 */
        "sb $0,0($24)\n\t"   /* 801393E0: a3000000 sb zero,0(t8) */
        "lw $31,28($29)\n\t"   /* 801393E4: 8fbf001c lw ra,28(sp) */
        "addiu $29,$29,640\n\t"   /* 801393E8: 27bd0280 addiu sp,sp,640 */
        "jr $31\n\t"   /* 801393EC: 03e00008 jr ra */
        "nop\n\t"   /* 801393F0: 00000000 nop  */
        ".set at\n\t"
        ".set\treorder\n\t"
        ".set noreorder\n\t"
        ".size sub_801388B8,.-sub_801388B8\n\t"
        ".text");

/* MDEC_vlc (LIBPRESS): VLC decoder, Sony hand-written assembly. */
__asm__(
        ".section .text_80139C1C,\"ax\",@progbits\n\t"
        ".align 2\n\t"
        ".globl sub_80139C1C\n\t"
        ".type sub_80139C1C,@function\n"
        "sub_80139C1C:\n\t"
        ".set\tnoreorder\n\t"
        ".set noat\n\t"
        "bne $4,$0,.L80139C1C_80139C60\n\t"   /* 80139C1C: 14800010 bnez a0,0x80139c60 */
        "nop\n\t"   /* 80139C20: 00000000 nop  */
        "lui $5,%hi(D_80164AF4)\n\t"   /* 80139C24: 3c058016 lui a1,0x8016 */
        "lw $5,%lo(D_80164AF4)($5)\n\t"   /* 80139C28: 8ca54af4 lw a1,19188(a1) */
        "lui $10,%hi(D_80164AF8)\n\t"   /* 80139C2C: 3c0a8016 lui t2,0x8016 */
        "lw $10,%lo(D_80164AF8)($10)\n\t"   /* 80139C30: 8d4a4af8 lw t2,19192(t2) */
        "lui $8,%hi(D_80164AFC)\n\t"   /* 80139C34: 3c088016 lui t0,0x8016 */
        "lw $8,%lo(D_80164AFC)($8)\n\t"   /* 80139C38: 8d084afc lw t0,19196(t0) */
        "lui $4,%hi(D_80164B00)\n\t"   /* 80139C3C: 3c048016 lui a0,0x8016 */
        "lw $4,%lo(D_80164B00)($4)\n\t"   /* 80139C40: 8c844b00 lw a0,19200(a0) */
        "lui $2,%hi(D_80164AF0)\n\t"   /* 80139C44: 3c028016 lui v0,0x8016 */
        "lw $2,%lo(D_80164AF0)($2)\n\t"   /* 80139C48: 8c424af0 lw v0,19184(v0) */
        "lui $6,%hi(D_80164B04)\n\t"   /* 80139C4C: 3c068016 lui a2,0x8016 */
        "lw $6,%lo(D_80164B04)($6)\n\t"   /* 80139C50: 8cc64b04 lw a2,19204(a2) */
        "sll $2,$2,0x1\n\t"   /* 80139C54: 00021040 sll v0,v0,0x1 */
        "j .L80139C1C_80139CF8\n\t"   /* 80139C58: 0804e73e j 0x80139cf8 */
        "addu $11,$5,$2\n\t"   /* 80139C5C: 00a25821 addu t3,a1,v0 */
        ".L80139C1C_80139C60:\n\t"
        "lui $2,%hi(D_80164AF0)\n\t"   /* 80139C60: 3c028016 lui v0,0x8016 */
        "lw $2,%lo(D_80164AF0)($2)\n\t"   /* 80139C64: 8c424af0 lw v0,19184(v0) */
        "lhu $3,0($4)\n\t"   /* 80139C68: 94830000 lhu v1,0(a0) */
        "addiu $4,$4,2\n\t"   /* 80139C6C: 24840002 addiu a0,a0,2 */
        "sll $2,$2,0x1\n\t"   /* 80139C70: 00021040 sll v0,v0,0x1 */
        "addu $11,$5,$2\n\t"   /* 80139C74: 00a25821 addu t3,a1,v0 */
        "sh $3,0($5)\n\t"   /* 80139C78: a4a30000 sh v1,0(a1) */
        "addiu $5,$5,2\n\t"   /* 80139C7C: 24a50002 addiu a1,a1,2 */
        "lhu $2,0($4)\n\t"   /* 80139C80: 94820000 lhu v0,0(a0) */
        "addiu $4,$4,2\n\t"   /* 80139C84: 24840002 addiu a0,a0,2 */
        "sh $2,0($5)\n\t"   /* 80139C88: a4a20000 sh v0,0(a1) */
        "addiu $5,$5,2\n\t"   /* 80139C8C: 24a50002 addiu a1,a1,2 */
        "lhu $10,0($4)\n\t"   /* 80139C90: 948a0000 lhu t2,0(a0) */
        "addiu $4,$4,2\n\t"   /* 80139C94: 24840002 addiu a0,a0,2 */
        "lhu $3,0($4)\n\t"   /* 80139C98: 94830000 lhu v1,0(a0) */
        "ori $2,$0,0x1\n\t"   /* 80139C9C: 34020001 li v0,0x1 */
        "beq $3,$2,.L80139C1C_80139CB4\n\t"   /* 80139CA0: 10620004 beq v1,v0,0x80139cb4 */
        "addiu $4,$4,2\n\t"   /* 80139CA4: 24840002 addiu a0,a0,2 */
        "ori $2,$0,0x2\n\t"   /* 80139CA8: 34020002 li v0,0x2 */
        "bne $3,$2,.L80139C1C_80139F48\n\t"   /* 80139CAC: 146200a6 bne v1,v0,0x80139f48 */
        "addiu $2,$0,-1\n\t"   /* 80139CB0: 2402ffff li v0,-1 */
        ".L80139C1C_80139CB4:\n\t"
        "addiu $8,$4,4\n\t"   /* 80139CB4: 24880004 addiu t0,a0,4 */
        "ori $6,$0,0xa\n\t"   /* 80139CB8: 3406000a li a2,0xa */
        "addu $9,$5,$0\n\t"   /* 80139CBC: 00a04821 move t1,a1 */
        "addiu $5,$5,2\n\t"   /* 80139CC0: 24a50002 addiu a1,a1,2 */
        "sll $7,$10,0xa\n\t"   /* 80139CC4: 000a3a80 sll a3,t2,0xa */
        "lhu $2,0($4)\n\t"   /* 80139CC8: 94820000 lhu v0,0(a0) */
        "lhu $3,2($4)\n\t"   /* 80139CCC: 94830002 lhu v1,2(a0) */
        "sll $2,$2,0x10\n\t"   /* 80139CD0: 00021400 sll v0,v0,0x10 */
        "or $4,$2,$3\n\t"   /* 80139CD4: 00432025 or a0,v0,v1 */
        "srl $3,$4,0x16\n\t"   /* 80139CD8: 00041d82 srl v1,a0,0x16 */
        "bltz $3,.L80139C1C_80139CEC\n\t"   /* 80139CDC: 04600003 bltz v1,0x80139cec */
        "sll $4,$4,0xa\n\t"   /* 80139CE0: 00042280 sll a0,a0,0xa */
        "j .L80139C1C_80139CF4\n\t"   /* 80139CE4: 0804e73d j 0x80139cf4 */
        "or $2,$7,$3\n\t"   /* 80139CE8: 00e31025 or v0,a3,v1 */
        ".L80139C1C_80139CEC:\n\t"
        "addiu $2,$3,1024\n\t"   /* 80139CEC: 24620400 addiu v0,v1,1024 */
        "or $2,$7,$2\n\t"   /* 80139CF0: 00e21025 or v0,a3,v0 */
        ".L80139C1C_80139CF4:\n\t"
        "sh $2,0($9)\n\t"   /* 80139CF4: a5220000 sh v0,0(t1) */
        ".L80139C1C_80139CF8:\n\t"
        "srl $2,$4,0x13\n\t"   /* 80139CF8: 000414c2 srl v0,a0,0x13 */
        ".L80139C1C_80139CFC:\n\t"
        "sll $2,$2,0x3\n\t"   /* 80139CFC: 000210c0 sll v0,v0,0x3 */
        "lui $3,%hi(D_80165B08)\n\t"   /* 80139D00: 3c038016 lui v1,0x8016 */
        "addiu $3,$3,%lo(D_80165B08)\n\t"   /* 80139D04: 24635b08 addiu v1,v1,23304 */
        "addu $7,$2,$3\n\t"   /* 80139D08: 00433821 addu a3,v0,v1 */
        "lhu $2,2($7)\n\t"   /* 80139D0C: 94e20002 lhu v0,2(a3) */
        "nop\n\t"   /* 80139D10: 00000000 nop  */
        "bne $2,$0,.L80139C1C_80139D50\n\t"   /* 80139D14: 1440000e bnez v0,0x80139d50 */
        "srl $2,$4,0xf\n\t"   /* 80139D18: 000413c2 srl v0,a0,0xf */
        "sll $2,$2,0x3\n\t"   /* 80139D1C: 000210c0 sll v0,v0,0x3 */
        "lui $3,%hi(D_80175B08)\n\t"   /* 80139D20: 3c038017 lui v1,0x8017 */
        "addiu $3,$3,%lo(D_80175B08)\n\t"   /* 80139D24: 24635b08 addiu v1,v1,23304 */
        "addu $7,$2,$3\n\t"   /* 80139D28: 00433821 addu a3,v0,v1 */
        "addiu $6,$6,8\n\t"   /* 80139D2C: 24c60008 addiu a2,a2,8 */
        "andi $2,$6,0x10\n\t"   /* 80139D30: 30c20010 andi v0,a2,0x10 */
        "beq $2,$0,.L80139C1C_80139D50\n\t"   /* 80139D34: 10400006 beqz v0,0x80139d50 */
        "sll $4,$4,0x8\n\t"   /* 80139D38: 00042200 sll a0,a0,0x8 */
        "andi $6,$6,0xf\n\t"   /* 80139D3C: 30c6000f andi a2,a2,0xf */
        "lhu $2,0($8)\n\t"   /* 80139D40: 95020000 lhu v0,0(t0) */
        "addiu $8,$8,2\n\t"   /* 80139D44: 25080002 addiu t0,t0,2 */
        "sllv $2,$2,$6\n\t"   /* 80139D48: 00c21004 sllv v0,v0,a2 */
        "or $4,$4,$2\n\t"   /* 80139D4C: 00822025 or a0,a0,v0 */
        ".L80139C1C_80139D50:\n\t"
        "lhu $2,0($7)\n\t"   /* 80139D50: 94e20000 lhu v0,0(a3) */
        "nop\n\t"   /* 80139D54: 00000000 nop  */
        "sllv $4,$4,$2\n\t"   /* 80139D58: 00442004 sllv a0,a0,v0 */
        "addu $6,$6,$2\n\t"   /* 80139D5C: 00c23021 addu a2,a2,v0 */
        "andi $2,$6,0x10\n\t"   /* 80139D60: 30c20010 andi v0,a2,0x10 */
        "beq $2,$0,.L80139C1C_80139D80\n\t"   /* 80139D64: 10400006 beqz v0,0x80139d80 */
        "nop\n\t"   /* 80139D68: 00000000 nop  */
        "andi $6,$6,0xf\n\t"   /* 80139D6C: 30c6000f andi a2,a2,0xf */
        "lhu $2,0($8)\n\t"   /* 80139D70: 95020000 lhu v0,0(t0) */
        "addiu $8,$8,2\n\t"   /* 80139D74: 25080002 addiu t0,t0,2 */
        "sllv $2,$2,$6\n\t"   /* 80139D78: 00c21004 sllv v0,v0,a2 */
        "or $4,$4,$2\n\t"   /* 80139D7C: 00822025 or a0,a0,v0 */
        ".L80139C1C_80139D80:\n\t"
        "lhu $2,2($7)\n\t"   /* 80139D80: 94e20002 lhu v0,2(a3) */
        "nop\n\t"   /* 80139D84: 00000000 nop  */
        "andi $3,$2,0xffff\n\t"   /* 80139D88: 3043ffff andi v1,v0,0xffff */
        "sh $2,0($5)\n\t"   /* 80139D8C: a4a20000 sh v0,0(a1) */
        "ori $2,$0,0x7c1f\n\t"   /* 80139D90: 34027c1f li v0,0x7c1f */
        "beq $3,$2,.L80139C1C_80139E74\n\t"   /* 80139D94: 10620037 beq v1,v0,0x80139e74 */
        "ori $2,$0,0xfe00\n\t"   /* 80139D98: 3402fe00 li v0,0xfe00 */
        "bne $3,$2,.L80139C1C_80139DEC\n\t"   /* 80139D9C: 14620013 bne v1,v0,0x80139dec */
        "srl $3,$4,0x16\n\t"   /* 80139DA0: 00041d82 srl v1,a0,0x16 */
        ".L80139C1C_80139DA4:\n\t"
        "ori $2,$0,0x1ff\n\t"   /* 80139DA4: 340201ff li v0,0x1ff */
        "beq $3,$2,.L80139C1C_80139F00\n\t"   /* 80139DA8: 10620055 beq v1,v0,0x80139f00 */
        "addiu $5,$5,2\n\t"   /* 80139DAC: 24a50002 addiu a1,a1,2 */
        "addiu $6,$6,10\n\t"   /* 80139DB0: 24c6000a addiu a2,a2,10 */
        "andi $2,$6,0x10\n\t"   /* 80139DB4: 30c20010 andi v0,a2,0x10 */
        "beq $2,$0,.L80139C1C_80139DD4\n\t"   /* 80139DB8: 10400006 beqz v0,0x80139dd4 */
        "sll $4,$4,0xa\n\t"   /* 80139DBC: 00042280 sll a0,a0,0xa */
        "andi $6,$6,0xf\n\t"   /* 80139DC0: 30c6000f andi a2,a2,0xf */
        "lhu $2,0($8)\n\t"   /* 80139DC4: 95020000 lhu v0,0(t0) */
        "addiu $8,$8,2\n\t"   /* 80139DC8: 25080002 addiu t0,t0,2 */
        "sllv $2,$2,$6\n\t"   /* 80139DCC: 00c21004 sllv v0,v0,a2 */
        "or $4,$4,$2\n\t"   /* 80139DD0: 00822025 or a0,a0,v0 */
        ".L80139C1C_80139DD4:\n\t"
        "addu $9,$5,$0\n\t"   /* 80139DD4: 00a04821 move t1,a1 */
        "addiu $5,$5,2\n\t"   /* 80139DD8: 24a50002 addiu a1,a1,2 */
        "bgez $3,.L80139C1C_80139ED8\n\t"   /* 80139DDC: 0461003e bgez v1,0x80139ed8 */
        "sll $7,$10,0xa\n\t"   /* 80139DE0: 000a3a80 sll a3,t2,0xa */
        "j .L80139C1C_80139EE4\n\t"   /* 80139DE4: 0804e7b9 j 0x80139ee4 */
        "addiu $2,$3,1024\n\t"   /* 80139DE8: 24620400 addiu v0,v1,1024 */
        ".L80139C1C_80139DEC:\n\t"
        "lhu $2,4($7)\n\t"   /* 80139DEC: 94e20004 lhu v0,4(a3) */
        "addiu $5,$5,2\n\t"   /* 80139DF0: 24a50002 addiu a1,a1,2 */
        "andi $3,$2,0xffff\n\t"   /* 80139DF4: 3043ffff andi v1,v0,0xffff */
        "sh $2,0($5)\n\t"   /* 80139DF8: a4a20000 sh v0,0(a1) */
        "ori $2,$0,0x7c1f\n\t"   /* 80139DFC: 34027c1f li v0,0x7c1f */
        "beq $3,$2,.L80139C1C_80139E74\n\t"   /* 80139E00: 1062001c beq v1,v0,0x80139e74 */
        "slti $2,$3,31776\n\t"   /* 80139E04: 28627c20 slti v0,v1,31776 */
        "beq $2,$0,.L80139C1C_80139E20\n\t"   /* 80139E08: 10400005 beqz v0,0x80139e20 */
        "nop\n\t"   /* 80139E0C: 00000000 nop  */
        "beq $3,$0,.L80139C1C_80139CFC\n\t"   /* 80139E10: 1060ffba beqz v1,0x80139cfc */
        "srl $2,$4,0x13\n\t"   /* 80139E14: 000414c2 srl v0,a0,0x13 */
        "j .L80139C1C_80139E2C\n\t"   /* 80139E18: 0804e78b j 0x80139e2c */
        "nop\n\t"   /* 80139E1C: 00000000 nop  */
        ".L80139C1C_80139E20:\n\t"
        "ori $2,$0,0xfe00\n\t"   /* 80139E20: 3402fe00 li v0,0xfe00 */
        "beq $3,$2,.L80139C1C_80139DA4\n\t"   /* 80139E24: 1062ffdf beq v1,v0,0x80139da4 */
        "srl $3,$4,0x16\n\t"   /* 80139E28: 00041d82 srl v1,a0,0x16 */
        ".L80139C1C_80139E2C:\n\t"
        "lhu $2,6($7)\n\t"   /* 80139E2C: 94e20006 lhu v0,6(a3) */
        "addiu $5,$5,2\n\t"   /* 80139E30: 24a50002 addiu a1,a1,2 */
        "andi $3,$2,0xffff\n\t"   /* 80139E34: 3043ffff andi v1,v0,0xffff */
        "sh $2,0($5)\n\t"   /* 80139E38: a4a20000 sh v0,0(a1) */
        "ori $2,$0,0x7c1f\n\t"   /* 80139E3C: 34027c1f li v0,0x7c1f */
        "beq $3,$2,.L80139C1C_80139E74\n\t"   /* 80139E40: 1062000c beq v1,v0,0x80139e74 */
        "slti $2,$3,31776\n\t"   /* 80139E44: 28627c20 slti v0,v1,31776 */
        "beq $2,$0,.L80139C1C_80139E60\n\t"   /* 80139E48: 10400005 beqz v0,0x80139e60 */
        "nop\n\t"   /* 80139E4C: 00000000 nop  */
        "beq $3,$0,.L80139C1C_80139CFC\n\t"   /* 80139E50: 1060ffaa beqz v1,0x80139cfc */
        "srl $2,$4,0x13\n\t"   /* 80139E54: 000414c2 srl v0,a0,0x13 */
        "j .L80139C1C_80139CFC\n\t"   /* 80139E58: 0804e73f j 0x80139cfc */
        "addiu $5,$5,2\n\t"   /* 80139E5C: 24a50002 addiu a1,a1,2 */
        ".L80139C1C_80139E60:\n\t"
        "ori $2,$0,0xfe00\n\t"   /* 80139E60: 3402fe00 li v0,0xfe00 */
        "beq $3,$2,.L80139C1C_80139E98\n\t"   /* 80139E64: 1062000c beq v1,v0,0x80139e98 */
        "srl $3,$4,0x16\n\t"   /* 80139E68: 00041d82 srl v1,a0,0x16 */
        "j .L80139C1C_80139CF8\n\t"   /* 80139E6C: 0804e73e j 0x80139cf8 */
        "addiu $5,$5,2\n\t"   /* 80139E70: 24a50002 addiu a1,a1,2 */
        ".L80139C1C_80139E74:\n\t"
        "srl $2,$4,0x10\n\t"   /* 80139E74: 00041402 srl v0,a0,0x10 */
        "sll $4,$4,0x10\n\t"   /* 80139E78: 00042400 sll a0,a0,0x10 */
        "sh $2,0($5)\n\t"   /* 80139E7C: a4a20000 sh v0,0(a1) */
        "lhu $2,0($8)\n\t"   /* 80139E80: 95020000 lhu v0,0(t0) */
        "addiu $8,$8,2\n\t"   /* 80139E84: 25080002 addiu t0,t0,2 */
        "addiu $5,$5,2\n\t"   /* 80139E88: 24a50002 addiu a1,a1,2 */
        "sllv $2,$2,$6\n\t"   /* 80139E8C: 00c21004 sllv v0,v0,a2 */
        "j .L80139C1C_80139CF8\n\t"   /* 80139E90: 0804e73e j 0x80139cf8 */
        "or $4,$4,$2\n\t"   /* 80139E94: 00822025 or a0,a0,v0 */
        ".L80139C1C_80139E98:\n\t"
        "ori $2,$0,0x1ff\n\t"   /* 80139E98: 340201ff li v0,0x1ff */
        "beq $3,$2,.L80139C1C_80139F00\n\t"   /* 80139E9C: 10620018 beq v1,v0,0x80139f00 */
        "addiu $5,$5,2\n\t"   /* 80139EA0: 24a50002 addiu a1,a1,2 */
        "addiu $6,$6,10\n\t"   /* 80139EA4: 24c6000a addiu a2,a2,10 */
        "andi $2,$6,0x10\n\t"   /* 80139EA8: 30c20010 andi v0,a2,0x10 */
        "beq $2,$0,.L80139C1C_80139EC8\n\t"   /* 80139EAC: 10400006 beqz v0,0x80139ec8 */
        "sll $4,$4,0xa\n\t"   /* 80139EB0: 00042280 sll a0,a0,0xa */
        "andi $6,$6,0xf\n\t"   /* 80139EB4: 30c6000f andi a2,a2,0xf */
        "lhu $2,0($8)\n\t"   /* 80139EB8: 95020000 lhu v0,0(t0) */
        "addiu $8,$8,2\n\t"   /* 80139EBC: 25080002 addiu t0,t0,2 */
        "sllv $2,$2,$6\n\t"   /* 80139EC0: 00c21004 sllv v0,v0,a2 */
        "or $4,$4,$2\n\t"   /* 80139EC4: 00822025 or a0,a0,v0 */
        ".L80139C1C_80139EC8:\n\t"
        "addu $9,$5,$0\n\t"   /* 80139EC8: 00a04821 move t1,a1 */
        "addiu $5,$5,2\n\t"   /* 80139ECC: 24a50002 addiu a1,a1,2 */
        "bltz $3,.L80139C1C_80139EE0\n\t"   /* 80139ED0: 04600003 bltz v1,0x80139ee0 */
        "sll $7,$10,0xa\n\t"   /* 80139ED4: 000a3a80 sll a3,t2,0xa */
        ".L80139C1C_80139ED8:\n\t"
        "j .L80139C1C_80139EE8\n\t"   /* 80139ED8: 0804e7ba j 0x80139ee8 */
        "or $2,$7,$3\n\t"   /* 80139EDC: 00e31025 or v0,a3,v1 */
        ".L80139C1C_80139EE0:\n\t"
        "addiu $2,$3,1024\n\t"   /* 80139EE0: 24620400 addiu v0,v1,1024 */
        ".L80139C1C_80139EE4:\n\t"
        "or $2,$7,$2\n\t"   /* 80139EE4: 00e21025 or v0,a3,v0 */
        ".L80139C1C_80139EE8:\n\t"
        "sh $2,0($9)\n\t"   /* 80139EE8: a5220000 sh v0,0(t1) */
        "sltu $2,$11,$5\n\t"   /* 80139EEC: 0165102b sltu v0,t3,a1 */
        "bne $2,$0,.L80139C1C_80139F20\n\t"   /* 80139EF0: 1440000b bnez v0,0x80139f20 */
        "ori $2,$0,0x1\n\t"   /* 80139EF4: 34020001 li v0,0x1 */
        "j .L80139C1C_80139CFC\n\t"   /* 80139EF8: 0804e73f j 0x80139cfc */
        "srl $2,$4,0x13\n\t"   /* 80139EFC: 000414c2 srl v0,a0,0x13 */
        ".L80139C1C_80139F00:\n\t"
        "ori $2,$0,0xfe00\n\t"   /* 80139F00: 3402fe00 li v0,0xfe00 */
        "ori $3,$0,0x3f\n\t"   /* 80139F04: 3403003f li v1,0x3f */
        ".L80139C1C_80139F08:\n\t"
        "sh $2,0($5)\n\t"   /* 80139F08: a4a20000 sh v0,0(a1) */
        "addiu $3,$3,-1\n\t"   /* 80139F0C: 2463ffff addiu v1,v1,-1 */
        "bgez $3,.L80139C1C_80139F08\n\t"   /* 80139F10: 0461fffd bgez v1,0x80139f08 */
        "addiu $5,$5,2\n\t"   /* 80139F14: 24a50002 addiu a1,a1,2 */
        "j .L80139C1C_80139F48\n\t"   /* 80139F18: 0804e7d2 j 0x80139f48 */
        "addu $2,$0,$0\n\t"   /* 80139F1C: 00001021 move v0,zero */
        ".L80139C1C_80139F20:\n\t"
        "lui $1,%hi(D_80164AF4)\n\t"   /* 80139F20: 3c018016 lui at,0x8016 */
        "sw $5,%lo(D_80164AF4)($1)\n\t"   /* 80139F24: ac254af4 sw a1,19188(at) */
        "lui $1,%hi(D_80164AF8)\n\t"   /* 80139F28: 3c018016 lui at,0x8016 */
        "sw $10,%lo(D_80164AF8)($1)\n\t"   /* 80139F2C: ac2a4af8 sw t2,19192(at) */
        "lui $1,%hi(D_80164AFC)\n\t"   /* 80139F30: 3c018016 lui at,0x8016 */
        "sw $8,%lo(D_80164AFC)($1)\n\t"   /* 80139F34: ac284afc sw t0,19196(at) */
        "lui $1,%hi(D_80164B00)\n\t"   /* 80139F38: 3c018016 lui at,0x8016 */
        "sw $4,%lo(D_80164B00)($1)\n\t"   /* 80139F3C: ac244b00 sw a0,19200(at) */
        "lui $1,%hi(D_80164B04)\n\t"   /* 80139F40: 3c018016 lui at,0x8016 */
        "sw $6,%lo(D_80164B04)($1)\n\t"   /* 80139F44: ac264b04 sw a2,19204(at) */
        ".L80139C1C_80139F48:\n\t"
        "jr $31\n\t"   /* 80139F48: 03e00008 jr ra */
        "nop\n\t"   /* 80139F4C: 00000000 nop  */
        ".set at\n\t"
        ".set\treorder\n\t"
        ".set noreorder\n\t"
        ".size sub_80139C1C,.-sub_80139C1C\n\t"
        ".text");

/* SquareRoot0 (LIBGTE MSC01): integer square root via the GTE leading-zero counter and a table. Sony assembly. */
__asm__(
        ".section .text_8013FC44,\"ax\",@progbits\n\t"
        ".align 2\n\t"
        ".globl sub_8013FC44\n\t"
        ".type sub_8013FC44,@function\n"
        "sub_8013FC44:\n\t"
        ".set\tnoreorder\n\t"
        ".set noat\n\t"
        "mtc2 $4,$30\n\t"   /* 8013FC44: 4884f000 mtc2 a0,$30 */
        "nop\n\t"   /* 8013FC48: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 8013FC4C: 00000000 .word 0x00000000 */
        "mfc2 $2,$31\n\t"   /* 8013FC50: 4802f800 mfc2 v0,$31 */
        "addiu $1,$0,32\n\t"   /* 8013FC54: 24010020 li at,32 */
        "beq $2,$1,.L8013FC44_8013FCC0\n\t"   /* 8013FC58: 10410019 beq v0,at,0x8013fcc0 */
        "nop\n\t"   /* 8013FC5C: 00000000 nop  */
        "andi $8,$2,0x1\n\t"   /* 8013FC60: 30480001 andi t0,v0,0x1 */
        "addiu $10,$0,-2\n\t"   /* 8013FC64: 240afffe li t2,-2 */
        "and $10,$2,$10\n\t"   /* 8013FC68: 004a5024 and t2,v0,t2 */
        "addiu $9,$0,31\n\t"   /* 8013FC6C: 2409001f li t1,31 */
        "sub $9,$9,$10\n\t"   /* 8013FC70: 012a4822 sub t1,t1,t2 */
        "sra $9,$9,0x1\n\t"   /* 8013FC74: 00094843 sra t1,t1,0x1 */
        "addi $11,$10,-24\n\t"   /* 8013FC78: 214bffe8 addi t3,t2,-24 */
        "bltz $11,.L8013FC44_8013FC8C\n\t"   /* 8013FC7C: 05600003 bltz t3,0x8013fc8c */
        "nop\n\t"   /* 8013FC80: 00000000 nop  */
        "sllv $12,$4,$11\n\t"   /* 8013FC84: 01646004 sllv t4,a0,t3 */
        "beq $0,$0,.L8013FC44_8013FC98\n\t"   /* 8013FC88: 10000003 b 0x8013fc98 */
        ".L8013FC44_8013FC8C:\n\t"
        "addiu $11,$0,24\n\t"   /* 8013FC8C: 240b0018 li t3,24 */
        "sub $11,$11,$10\n\t"   /* 8013FC90: 016a5822 sub t3,t3,t2 */
        "srav $12,$4,$11\n\t"   /* 8013FC94: 01646007 srav t4,a0,t3 */
        ".L8013FC44_8013FC98:\n\t"
        "addi $12,$12,-64\n\t"   /* 8013FC98: 218cffc0 addi t4,t4,-64 */
        "sll $12,$12,0x1\n\t"   /* 8013FC9C: 000c6040 sll t4,t4,0x1 */
        "lui $13,%hi(D_801777B0)\n\t"   /* 8013FCA0: 3c0d8017 lui t5,0x8017 */
        "addu $13,$13,$12\n\t"   /* 8013FCA4: 01ac6821 addu t5,t5,t4 */
        "lh $13,%lo(D_801777B0)($13)\n\t"   /* 8013FCA8: 85ad77b0 lh t5,30640(t5) */
        "nop\n\t"   /* 8013FCAC: 00000000 nop  */
        "sllv $13,$13,$9\n\t"   /* 8013FCB0: 012d6804 sllv t5,t5,t1 */
        "srl $2,$13,0xc\n\t"   /* 8013FCB4: 000d1302 srl v0,t5,0xc */
        "jr $31\n\t"   /* 8013FCB8: 03e00008 jr ra */
        "nop\n\t"   /* 8013FCBC: 00000000 nop  */
        ".L8013FC44_8013FCC0:\n\t"
        "jr $31\n\t"   /* 8013FCC0: 03e00008 jr ra */
        "addiu $2,$0,0\n\t"   /* 8013FCC4: 24020000 li v0,0 */
        ".set at\n\t"
        ".set\treorder\n\t"
        ".set noreorder\n\t"
        ".size sub_8013FC44,.-sub_8013FC44\n\t"
        ".text");

/* VectorNormal core (LIBGTE MSC02): normalise t0..t2 in place using the GTE. Sony assembly. */
__asm__(
        ".section .text_8013FE60,\"ax\",@progbits\n\t"
        ".align 2\n\t"
        ".globl sub_8013FE60\n\t"
        ".type sub_8013FE60,@function\n"
        "sub_8013FE60:\n\t"
        ".set\tnoreorder\n\t"
        ".set noat\n\t"
        "mtc2 $8,$9\n\t"   /* 8013FE60: 48884800 mtc2 t0,$9 */
        "mtc2 $9,$10\n\t"   /* 8013FE64: 48895000 mtc2 t1,$10 */
        "mtc2 $10,$11\n\t"   /* 8013FE68: 488a5800 mtc2 t2,$11 */
        "nop\n\t"   /* 8013FE6C: 00000000 nop  */
        "cop2 0xa00428\n\t"   /* 8013FE70: 4aa00428 c2 0xa00428 */
        "mfc2 $11,$25\n\t"   /* 8013FE74: 480bc800 mfc2 t3,$25 */
        "mfc2 $12,$26\n\t"   /* 8013FE78: 480cd000 mfc2 t4,$26 */
        "mfc2 $13,$27\n\t"   /* 8013FE7C: 480dd800 mfc2 t5,$27 */
        "add $11,$11,$12\n\t"   /* 8013FE80: 016c5820 add t3,t3,t4 */
        "add $2,$11,$13\n\t"   /* 8013FE84: 016d1020 add v0,t3,t5 */
        "mtc2 $2,$30\n\t"   /* 8013FE88: 4882f000 mtc2 v0,$30 */
        "nop\n\t"   /* 8013FE8C: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 8013FE90: 00000000 .word 0x00000000 */
        "mfc2 $3,$31\n\t"   /* 8013FE94: 4803f800 mfc2 v1,$31 */
        "addiu $1,$0,-2\n\t"   /* 8013FE98: 2401fffe li at,-2 */
        "and $3,$3,$1\n\t"   /* 8013FE9C: 00611824 and v1,v1,at */
        "addiu $14,$0,31\n\t"   /* 8013FEA0: 240e001f li t6,31 */
        "sub $14,$14,$3\n\t"   /* 8013FEA4: 01c37022 sub t6,t6,v1 */
        "addi $11,$3,-24\n\t"   /* 8013FEA8: 206bffe8 addi t3,v1,-24 */
        "bltz $11,.L8013FE60_8013FEBC\n\t"   /* 8013FEAC: 05600003 bltz t3,0x8013febc */
        "sra $14,$14,0x1\n\t"   /* 8013FEB0: 000e7043 sra t6,t6,0x1 */
        "beq $0,$0,.L8013FE60_8013FEC8\n\t"   /* 8013FEB4: 10000004 b 0x8013fec8 */
        "sllv $12,$2,$11\n\t"   /* 8013FEB8: 01626004 sllv t4,v0,t3 */
        ".L8013FE60_8013FEBC:\n\t"
        "addiu $11,$0,24\n\t"   /* 8013FEBC: 240b0018 li t3,24 */
        "sub $11,$11,$3\n\t"   /* 8013FEC0: 01635822 sub t3,t3,v1 */
        "srav $12,$2,$11\n\t"   /* 8013FEC4: 01626007 srav t4,v0,t3 */
        ".L8013FE60_8013FEC8:\n\t"
        "addi $12,$12,-64\n\t"   /* 8013FEC8: 218cffc0 addi t4,t4,-64 */
        "sll $12,$12,0x1\n\t"   /* 8013FECC: 000c6040 sll t4,t4,0x1 */
        "lui $13,%hi(D_80177944)\n\t"   /* 8013FED0: 3c0d8017 lui t5,0x8017 */
        "addu $13,$13,$12\n\t"   /* 8013FED4: 01ac6821 addu t5,t5,t4 */
        "lh $13,%lo(D_80177944)($13)\n\t"   /* 8013FED8: 85ad7944 lh t5,31044(t5) */
        "nop\n\t"   /* 8013FEDC: 00000000 nop  */
        "mtc2 $13,$8\n\t"   /* 8013FEE0: 488d4000 mtc2 t5,$8 */
        "mtc2 $8,$9\n\t"   /* 8013FEE4: 48884800 mtc2 t0,$9 */
        "mtc2 $9,$10\n\t"   /* 8013FEE8: 48895000 mtc2 t1,$10 */
        "mtc2 $10,$11\n\t"   /* 8013FEEC: 488a5800 mtc2 t2,$11 */
        "nop\n\t"   /* 8013FEF0: 00000000 .word 0x00000000 */
        "nop\n\t"   /* 8013FEF4: 00000000 .word 0x00000000 */
        "cop2 0x190003d\n\t"   /* 8013FEF8: 4b90003d c2 0x190003d */
        "mfc2 $8,$25\n\t"   /* 8013FEFC: 4808c800 mfc2 t0,$25 */
        "mfc2 $9,$26\n\t"   /* 8013FF00: 4809d000 mfc2 t1,$26 */
        "mfc2 $10,$27\n\t"   /* 8013FF04: 480ad800 mfc2 t2,$27 */
        "srav $8,$8,$14\n\t"   /* 8013FF08: 01c84007 srav t0,t0,t6 */
        "srav $9,$9,$14\n\t"   /* 8013FF0C: 01c94807 srav t1,t1,t6 */
        "srav $10,$10,$14\n\t"   /* 8013FF10: 01ca5007 srav t2,t2,t6 */
        "jr $31\n\t"   /* 8013FF14: 03e00008 jr ra */
        "nop\n\t"   /* 8013FF18: 00000000 nop  */
        ".set at\n\t"
        ".set\treorder\n\t"
        ".set noreorder\n\t"
        ".size sub_8013FE60,.-sub_8013FE60\n\t"
        ".text");
