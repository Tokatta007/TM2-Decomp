/* PsyQ SDK LIBPRESS, reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * MDEC (motion decoder) front end: DecDCTReset/in/out/outCallback, the
 * MDEC_reset and timeout helpers, and the VLC entry points.
 * Written by hand from the retail disassembly. MDEC_in/MDEC_out and the two
 * sync waits live in mdec_transfer.c (MDEC_in, MDEC_out, sub_80139918,
 * sub_801399B0).
 *
 * MDEC_vlc @80139C1C (the version 1/2 decoder) is hand-written assembly in
 * Sony's library and is left retail. (The nop after the dead CHCR read in
 * MDEC_reset and timeout is aspsx 2.21's load/overwrite hazard nop, which
 * tools/maspsx emits for --aspsx-version=2.21.) MDEC_vlc2 and
 * ivlc_dct_dc_size (Sony's VLC2.C) are byte-identical.
 *
 * The library's data lives at its retail addresses and is declared extern by
 * address (D_XXXXXXXX). The DMA/MDEC register pointers are initialised data
 * (e.g. D_80164AE8 = 0x1F801824). */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef void (*Callback)();

/* ---- library data (retail addresses) ---- */
extern u32 D_801649AC[];                /* mdec_iq: quantisation table packet */
extern u32 D_80164A30[];                /* mdec_coef: IDCT coefficient packet */
extern volatile u32 *mdecHwRegs;        /* DMA0 (MDECin) MADR  0x1F801080 */
extern volatile u32 *D_80164ABC;        /* DMA0 CHCR           0x1F801088 */
extern volatile u32 *D_80164AC0;        /* DMA1 (MDECout) MADR 0x1F801090 */
extern volatile u32 *D_80164AC8;        /* DMA1 CHCR           0x1F801098 */
extern volatile u32 *D_80164AE8;        /* MDEC1 control/status 0x1F801824 */
extern s32 D_80164AF0;                  /* VLC output size limit (halfwords) */

extern char D_800D03D0[];               /* "MDEC_rest:bad option(%d)\n" */
extern char D_800D040C[];               /* "\t DMA=(%d,%d), ADDR=(0x%08x->0x%08x)\n" */
extern char D_800D0434[];               /* "\t FIFO=(%d,%d),BUSY=%d,DREQ=(%d,%d),RGB24=%d,STP=%d\n" */
extern char D_800D046C[];               /* "%s timeout:\n" */

extern int printf(const char *, ...);
extern void exit();
extern void *sub_801452F0(void);                    /* ResetCallback */
extern Callback sub_80145350(int dma, Callback f);  /* DMACallback */
extern void MDEC_in(u32 *input, u32 count);
extern void MDEC_out(void *output, u32 count);

void sub_801396EC(s32 option);
s32 sub_80139BEC(s32 size);
s32 sub_80139C1C(u32 *bs, u16 *buf);       /* MDEC_vlc: hand-written assembly in Sony's lib, left retail */
s32 sub_80139F50(u16 *bs, u16 *buf);
s32 sub_8013A32C(void);

/* @80139498 DecDCTReset: reset the MDEC (option 0 also resets callbacks). */
void __attribute__((section(".text_80139498"))) sub_80139498(s32 option)
{
    if (option == 0) {
        sub_801452F0();
    }
    sub_801396EC(option);
}

/* @801395C8 DecDCTin: set RGB24/STP mode bits in the run-level header and
 * start the input DMA (size = low halfword of the header). */
void __attribute__((section(".text_801395C8"))) sub_801395C8(u32 *buf, s32 mode)
{
    if (mode & 1) {
        *buf &= 0xF7FFFFFF;
    } else {
        *buf |= 0x08000000;
    }
    if (mode & 2) {
        *buf |= 0x02000000;
    } else {
        *buf &= 0xFDFFFFFF;
    }
    MDEC_in(buf, *(u16 *)buf);
}

/* @80139644 DecDCTout: start the output DMA of size words into buf. */
void __attribute__((section(".text_80139644"))) sub_80139644(void *buf, u32 size)
{
    MDEC_out(buf, size);
}

/* @801396C8 DecDCToutCallback: install the MDECout DMA-complete handler. */
void __attribute__((section(".text_801396C8"))) sub_801396C8(Callback func)
{
    sub_80145350(1, func);
}

/* @801396EC MDEC_reset: option 0 = full reset + reload tables, 1 = soft
 * reset (abort DMA), anything else is fatal. */
void __attribute__((section(".text_801396EC"))) sub_801396EC(s32 option)
{
    switch (option) {
    case 0:
        *D_80164AE8 = 0x80000000;
        *D_80164ABC = 0;
        *D_80164AC8 = 0;
        *D_80164AE8 = 0x60000000;
        MDEC_in(D_801649AC, 0x20);
        MDEC_in(D_80164A30, 0x20);
        break;
    case 1:
        *D_80164AE8 = 0x80000000;
        *D_80164ABC = 0;
        *D_80164AC8 = 0;
        *D_80164AC8;
        *D_80164AE8 = 0x60000000;
        break;
    default:
        printf(D_800D03D0, option);
        exit();
        break;
    }
}

/* @80139A48 timeout: dump DMA/MDEC state after a sync timeout and reset. */
s32 __attribute__((section(".text_80139A48"))) sub_80139A48(char *func)
{
    u32 status;

    printf(D_800D046C, func);
    status = *D_80164AE8;
    printf(D_800D040C, (*D_80164ABC >> 24) & 1, (*D_80164AC8 >> 24) & 1,
           *mdecHwRegs, *D_80164AC0);
    printf(D_800D0434, ~status >> 31, (status >> 30) & 1, (status >> 29) & 1,
           (status >> 28) & 1, (status >> 27) & 1, (status >> 25) & 1,
           (status >> 23) & 1);
    *D_80164AE8 = 0x80000000;
    *D_80164ABC = 0;
    *D_80164AC8 = 0;
    *D_80164AC8;
    *D_80164AE8 = 0x60000000;
    return 0;
}

/* @80139B7C DecDCTvlcSize: set the VLC output limit, return the old one. */
s32 __attribute__((section(".text_80139B7C"))) sub_80139B7C(s32 size)
{
    return sub_80139BEC(size);
}

/* @80139B9C DecDCTvlc: decode a bitstream to run-level codes; version-1/2
 * frames go through MDEC_vlc, anything it rejects through MDEC_vlc2. */
s32 __attribute__((section(".text_80139B9C"))) sub_80139B9C(u32 *bs, u16 *buf)
{
    s32 ret = sub_80139C1C(bs, buf);
    if (ret == -1) {
        return sub_80139F50((u16 *)bs, buf);
    }
    return ret;
}

/* @80139BEC MDEC_vlc_brk: set the output limit (0 = unlimited), return the
 * previous one. */
s32 __attribute__((section(".text_80139BEC"))) sub_80139BEC(s32 size)
{
    s32 old = D_80164AF0;
    if (size) {
        D_80164AF0 = size * 2;
    } else {
        D_80164AF0 = 0x00FFFFFF;
    }
    return old;
}

/* ---- MDEC_vlc2: VLC decoder for version-3 bitstreams ----
 * Bit reader state: bitbuf holds the next 32 bits MSB-first, bitcnt the bits
 * consumed from the halfword after it; every 16 consumed bits pull the next
 * halfword from the stream. */
typedef struct {
    u16 nbits;          /* code length */
    u16 size;           /* DC: differential size in bits */
    u16 pad[2];
} DcVlc;

typedef struct {
    u16 nbits;          /* code length */
    u16 code[3];        /* run-level words; 0 = none, 0x7C1F escape, 0xFE00 EOB */
} AcVlc;

extern u32 D_801B2B18;          /* bitbuf */
extern s32 D_801B2D18;          /* bitcnt */
extern u16 *D_801AB80C;         /* stream pointer */
extern s32 D_801B1CBC;          /* block index in macroblock (0..5) */
extern s32 D_801A5AE0;          /* DC predictor Cr */
extern s32 D_801FACF0;          /* DC predictor Cb */
extern s32 D_801F5A8C;          /* DC predictor Y */
extern DcVlc D_80164B08[];      /* DC size table, luminance (by top 8 bits) */
extern DcVlc D_80165308[];      /* DC size table, chrominance */
extern AcVlc D_80165B08[];      /* AC table by top 13 bits */
extern AcVlc D_80175B08[];      /* AC table for long codes (by top 17 bits) */
extern char D_800D047C[];       /* "MDEC_vlec: invalid VLC ID\n" */

#define VLC_SKIP(n)                                                     \
    D_801B2B18 <<= (n);                                                 \
    D_801B2D18 += (n);                                                  \
    if (D_801B2D18 & 0x10) {                                            \
        D_801B2B18 = (*D_801AB80C++ << (D_801B2D18 &= 0xF)) | D_801B2B18; \
    }

/* @8013A32C ivlc_dct_dc_size: decode one DC coefficient, add it to the
 * block's predictor and return the predicted value << 2. */
s32 __attribute__((section(".text_8013A32C"))) sub_8013A32C(void)
{
    DcVlc *e;
    s32 v = 0;

    if (D_801B1CBC < 2) {
        e = &D_80165308[D_801B2B18 >> 24];
    } else {
        e = &D_80164B08[D_801B2B18 >> 24];
    }
    VLC_SKIP(e->nbits);
    if (e->size) {
        s32 n = e->size;
        u32 bb = D_801B2B18;
        s32 half;
        v = bb >> (32 - n);
        half = 1 << (n - 1);
        if (v < half) {
            v += 1 - (half << 1);
        }
        D_801B2B18 = bb << n;
        D_801B2D18 += n;
        if (D_801B2D18 & 0x10) {
            D_801B2B18 = (*D_801AB80C++ << (D_801B2D18 &= 0xF)) | D_801B2B18;
        }
    }
    switch (D_801B1CBC) {
    case 0:
        v = D_801A5AE0 += v;
        break;
    case 1:
        v = D_801FACF0 += v;
        break;
    default:
        v = D_801F5A8C += v;
        break;
    }
    if (++D_801B1CBC == 6) {
        D_801B1CBC = 0;
    }
    return v << 2;
}

/* DC word: quantiser scale in the top 6 bits, 10-bit DC value below. */
#define VLC_DC()                                                        \
    i = sub_8013A32C();                                                 \
    *buf++ = (q << 10) | ((i >= 0) ? i : i + 1024);

/* @80139F50 MDEC_vlc2: decode a version-3 bitstream into run-level codes.
 * Returns 0, or -1 for an unknown bitstream version. */
s32 __attribute__((section(".text_80139F50"))) sub_80139F50(u16 *bs, u16 *buf)
{
    AcVlc *e;
    s32 q;
    s32 i;      /* one scratch int, as in Sony's code: DC value, code word,
                 * end-of-picture test and fill counter all share it (that
                 * sharing decides the register allocation) */

    D_801F5A8C = 0;
    D_801A5AE0 = 0;
    D_801FACF0 = 0;
    D_801B1CBC = 0;
    *buf++ = *bs++;
    *buf++ = *bs++;
    q = *bs++;
    if (*bs++ != 3) {
        printf(D_800D047C);
        return -1;
    }
    D_801B2D18 = 0;
    D_801B2B18 = (bs[0] << 16) | bs[1];
    D_801AB80C = bs + 2;
    VLC_DC();
loop:
    for (;;) {  /* a real loop: its end note keeps CSE from carrying the
                 * escape/EOB constants into the code-word switches */
        e = &D_80165B08[D_801B2B18 >> 19];
        if (e->code[0] != 0) {
            VLC_SKIP(e->nbits);
        } else {
            e = &D_80175B08[D_801B2B18 >> 15];
            VLC_SKIP(8);
            VLC_SKIP(e->nbits);
        }
        i = *buf = e->code[0];
        if (i == 0x7C1F) {
            goto escape;
        }
        if (i != 0xFE00) {
            break;
        }
eob:
        buf++;
        if ((i = D_801B2B18 >> 22) == 0x3FF) {
            goto end;
        }
        VLC_DC();
    }
    /* not EOB: emit the second and third run-level words of the code */
    buf++;
    switch (*buf = e->code[1]) {
    case 0x7C1F:
        goto escape;
    case 0xFE00:
        goto eob;
    case 0:
        goto loop;
    }
    buf++;
    switch (*buf = e->code[2]) {
    case 0x7C1F:
        goto escape;
    case 0xFE00:
        goto eob;
    case 0:
        goto loop;
    }
    buf++;
    goto loop;
escape:
    /* escape: the next 16 bits are a raw run-level word */
    *buf = D_801B2B18 >> 16;
    D_801B2B18 <<= 16;
    D_801B2B18 = (*D_801AB80C++ << D_801B2D18) | D_801B2B18;
    buf++;
    goto loop;
end:
    for (i = 0; i < 64; i++) {
        *buf++ = 0xFE00;
    }
    return 0;
}

