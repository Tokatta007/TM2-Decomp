/* Library code 80140000..80148000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * All functions here are Sony PsyQ SDK library code (TM2's Dec 1995 libs):
 *  - LIBGTE RotTransPers, RotTransPers3: Sony assembly, written as C
 *    functions with the original instruction sequence as inline asm (no
 *    public source used; see sdk_libgte.c for the maspsx rules).
 *  - LIBGPU GetGraphType, GetGraphDebug, get_cs, get_ce: adapted from PSY-Z
 *    decomp/src/libgpu/sys.c (MIT, decomp/LICENSE).
 *  - LIBGPU FntOpen: adapted from PSY-Z decomp/src/libgpu/font.c (MIT);
 *    FntPrint: adapted from sotn-decomp
 *    src/main/psxsdk/libgpu/font.c (SPDX-License-Identifier: MIT).
 *  - LIBGPU SetPolyG3, SetPolyF4, SetSprt8, SetTile, SetBlockFill: the
 *    libgpu prim.c setters (sotn-decomp src/main/psxsdk/libgpu/prim.c, MIT).
 *  - LIBETC memclr (static copies in intr.c, intr_vb.c, intr_dma.c): adapted
 *    from psy-q-decomp src/etc/intr.c (MIT, repository licence).
 *  - __builtin_delete (libgcc): written by hand against the retail code.
 *  (LIBSND SsVabOpenHeadSticky 0x8014624C / SsVabFakeHead 0x8014627C need the
 *  libsnd recipe, GCC 2.5.7, and belong in a separate unit.)
 *
 * Library data is not defined here; it is declared extern by retail address.
 * Recipe: psyq3.3 cc1 + maspsx (aspsx 2.21), gp_size 0, no -fvolatile.
 * All functions byte-identical. FntFlush is in sdk_libgpu_font.c: it needs
 * the gcc2.6.3 recipe (psyq3.3 reloads its spilled r/g/b with lw, retail
 * uses lbu). */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef struct {
    s16 vx, vy, vz, pad;
} SVECTOR;

typedef struct {
    s16 x, y, w, h;
} RECT;

typedef struct {
    u_long addr : 24;
    u_long len : 8;
    u_char r0, g0, b0, code;
} P_TAG;

typedef struct {
    u_long tag;
    u_long code[2];
} DR_MODE;

typedef struct {
    u_long tag;
    u_char r0, g0, b0, code;
    s16 x0, y0;
    s16 w, h;
} TILE;

typedef struct {
    u_long tag;
    u_char r0, g0, b0, code;
    s16 x0, y0;
    u_char u0, v0;
    u16 clut;
} SPRT_8;

#define setlen(p, _len) (((P_TAG *)(p))->len = (u_char)(_len))
#define setcode(p, _code) (((P_TAG *)(p))->code = (u_char)(_code))

struct Debug {
    u_char version;     /* 0x00 */
    u_char queue;       /* 0x01 */
    u_char level;       /* 0x02 */
    u_char reverse;     /* 0x03 */
    volatile short w;   /* 0x04 */
    volatile short h;   /* 0x06 */
};

typedef struct {
    TILE tile;          /* 0x00 */
    DR_MODE draw_mode;  /* 0x10 */
    int capacity;       /* 0x1C */
    SPRT_8 *sprites;    /* 0x20 */
    char *buffer;       /* 0x24 */
    int written;        /* 0x28 */
    int unwrap;         /* 0x2C */
} FntStream;

/* FntPrint switch jump table (retail address). */
#pragma tm2_rdata .tm2_800D0B9C

/* ---- library data (retail addresses) ---- */
extern struct Debug gpuSysState;         /* libgpu info */
extern FntStream D_8017C43C[8];       /* Font[] */
extern int D_8017C5BC;                  /* n_fonts_open */
extern int D_8017C5C0;                  /* dump_id */
extern int D_8017CFC4;                  /* i_str */
extern char *D_8017CFC8;                /* "0123456789ABCDEF" */
extern char D_80193A50[0x400];          /* str */
extern SPRT_8 D_80193E50[0x400];        /* sprt */
extern u16 D_80197E50;                  /* font tpage */
extern u16 D_80197E54;                  /* font clut */

#define info gpuSysState
#define VVER (*(volatile u_char *)&info.version)
#define Font D_8017C43C
#define n_fonts_open D_8017C5BC
#define dump_id D_8017C5C0
#define i_str D_8017CFC4
#define hexdigits D_8017CFC8

/* ---- other units / libraries ---- */
extern void sub_8014215C(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw); /* SetDrawMode */
extern void sub_80144C3C(void *p);              /* TermPrim */
extern void sub_80144C54(void *p, int abe);     /* SetSemiTrans */
extern void AddPrim(void *ot, void *p);
extern void DrawOTag(u_long *p);
extern int strlen(const char *s);
extern void free(void *p);

void sub_80144D44(void *p);
void sub_80144DBC(void *p);

/* RotTransPers: perspective-transform one vertex (GTE RTPS); returns sz/4. */
long __attribute__((section(".text_801404C4"))) sub_801404C4(SVECTOR *v0, long *sxy, long *p, long *flag)
{
    register long z __asm__("$2");
    __asm__ volatile (
        "lwc2 $0,0(%1)\n\t"
        "lwc2 $1,4(%1)\n\t"
        "nop\n\t"
        "cop2 0x180001\n\t"
        "swc2 $14,0(%2)\n\t"
        "swc2 $8,0(%3)\n\t"
        "cfc2 $3,$31\n\t"
        "mfc2 $2,$19\n\t"
        "sw $3,0(%4)\n\t"
        ""
        : "=r"(z) : "r"(v0), "r"(sxy), "r"(p), "r"(flag) : "$3", "memory");
    return z >> 2;
}

/* RotTransPers3: perspective-transform three vertices (GTE RTPT); returns
 * sz/4. The stack arguments are read by the asm. */
long __attribute__((section(".text_801404F0"))) sub_801404F0(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
                                                            long *sxy0, long *sxy1, long *sxy2,
                                                            long *p, long *flag)
{
    register long z __asm__("$2");
    __asm__ volatile (
        "lwc2 $0,0(%1)\n\t"
        "lwc2 $1,4(%1)\n\t"
        "lwc2 $2,0(%2)\n\t"
        "lwc2 $3,4(%2)\n\t"
        "lwc2 $4,0(%3)\n\t"
        "lwc2 $5,4(%3)\n\t"
        "nop\n\t"
        "cop2 0x280030\n\t"
        "lw $8,16($29)\n\t"
        "lw $9,20($29)\n\t"
        "lw $10,24($29)\n\t"
        "lw $11,28($29)\n\t"
        "swc2 $12,0(%4)\n\t"
        "swc2 $13,0($8)\n\t"
        "swc2 $14,0($9)\n\t"
        "swc2 $8,0($10)\n\t"
        "cfc2 $3,$31\n\t"
        "mfc2 $2,$19\n\t"
        "sw $3,0($11)\n\t"
        ""
        : "=r"(z) : "r"(v0), "r"(v1), "r"(v2), "r"(sxy0)
        : "$3", "$8", "$9", "$10", "$11", "memory");
    return z >> 2;
}

/* GetGraphType: GPU type detected by ResetGraph. */
int __attribute__((section(".text_801413B4"))) sub_801413B4(void)
{
    return info.version;
}

/* GetGraphDebug: libgpu debug level. */
int __attribute__((section(".text_801413C4"))) sub_801413C4(void)
{
    return info.level;
}

/* get_cs: GP0(E3h) drawing-area top-left word, clamped to the VRAM size. */
u_long __attribute__((section(".text_801424B4"))) sub_801424B4(short x, short y)
{
    int cx, tx;
    int sy, ty;

    /* CLAMP(x, 0, info.w - 1), CLAMP(y, 0, info.h - 1); the two clamps are
     * spelled differently to reproduce the retail register allocation. */
    cx = x;
    cx = cx < 0 ? 0 : (((tx = info.w - 1) < cx ? (cx = tx) : 0), cx);
    sy = y;
    y = sy < 0 ? 0 : (((ty = info.h - 1) < sy ? (sy = ty) : 0), sy);
    if ((u_char)(VVER - 1) < 2) {
        return 0xE3000000 | ((y & 0xFFF) << 12) | (cx & 0xFFF);
    } else {
        return 0xE3000000 | ((y & 0x3FF) << 10) | (cx & 0x3FF);
    }
}

/* get_ce: GP0(E4h) drawing-area bottom-right word, clamped to the VRAM size. */
u_long __attribute__((section(".text_80142580"))) sub_80142580(short x, short y)
{
    int cx, tx;
    int sy, ty;

    /* CLAMP(x, 0, info.w - 1), CLAMP(y, 0, info.h - 1); the two clamps are
     * spelled differently to reproduce the retail register allocation. */
    cx = x;
    cx = cx < 0 ? 0 : (((tx = info.w - 1) < cx ? (cx = tx) : 0), cx);
    sy = y;
    y = sy < 0 ? 0 : (((ty = info.h - 1) < sy ? (sy = ty) : 0), sy);
    if ((u_char)(VVER - 1) < 2) {
        return 0xE4000000 | ((y & 0xFFF) << 12) | (cx & 0xFFF);
    } else {
        return 0xE4000000 | ((y & 0x3FF) << 10) | (cx & 0x3FF);
    }
}

#define FONT_TILE (*&Font[n_fonts_open].tile)

/* FntOpen: open a debug-font print stream (window x,y,w,h, optional
 * background, n characters); returns its id or -1. */
int __attribute__((section(".text_80143F84"))) sub_80143F84(int x, int y, int w, int h, int isbg, int n)
{
    int i;
    RECT rect;
    SPRT_8 *sprites;

    if (n_fonts_open >= 8) {
        return -1;
    }
    if (n_fonts_open == 0) {
        i_str = 0;
    }
    Font[n_fonts_open].unwrap = w == 0;
    if (i_str + n > 0x400) {
        n = 0x400 - i_str;
    }
    rect.x = 0;
    rect.y = 0;
    rect.w = 256;
    rect.h = 256;
    sub_8014215C(&Font[n_fonts_open].draw_mode, 0, 0, D_80197E50, &rect);
    if (isbg) {
        sub_80144DBC(&Font[n_fonts_open].tile);
        FONT_TILE.r0 = 0;
        FONT_TILE.g0 = 0;
        FONT_TILE.b0 = 0;
        sub_80144C54(&Font[n_fonts_open].tile, isbg == 2);
    }
    FONT_TILE.x0 = x;
    FONT_TILE.y0 = y;
    FONT_TILE.w = w;
    FONT_TILE.h = h;
    Font[n_fonts_open].capacity = n;
    Font[n_fonts_open].written = 0;
    Font[n_fonts_open].buffer = &D_80193A50[i_str];
    Font[n_fonts_open].sprites = &D_80193E50[i_str];
    Font[n_fonts_open].buffer[0] = '\0';
    sprites = Font[n_fonts_open].sprites;
    for (i = 0; i < n; i++, sprites++) {
        sub_80144D44(sprites);
        sprites->clut = D_80197E54;
    }
    i_str += n;
    return n_fonts_open++;
}

/* FntFlush (0x80144258) lives in sdk_libgpu_font.c (gcc2.6.3 recipe). */

#define LOW(x) (*(s32 *)&(x))

#define WriteChar(c)                                                           \
    font->buffer[font->written++] = (c);                                       \
    if (font->written > font->capacity) {                                      \
        return -1;                                                             \
    }

typedef char *va_list;
#define __va_rounded_size(TYPE) \
    (((sizeof(TYPE) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(AP, LASTARG) \
    (AP = ((char *)&(LASTARG) + __va_rounded_size(LASTARG)))
#define va_arg(AP, TYPE) \
    (AP += __va_rounded_size(TYPE), *((TYPE *)(AP - __va_rounded_size(TYPE))))

/* FntPrint: printf (%d %x %X %c %s, width, 0-pad) into a font stream; the
 * stream id argument is optional. Returns the stream length or -1. */
int __attribute__((section(".text_80144580"))) sub_80144580(const char *id, ...)
{
    char buf[0x200];
    va_list args;
    FntStream *font;
    u8 padZeros;
    s32 num;
    s32 len;
    s32 width;
    char *f;
    char *bufPtr;
    char sign;
    u32 ch;

    va_start(args, id);
    if (LOW(id) < 0 || LOW(id) >= n_fonts_open) {
        f = (char *)id;
        LOW(id) = dump_id;
        if (Font[LOW(id)].buffer == 0) {
            return -1;
        }
    } else {
        f = va_arg(args, char *);
    }

    font = &Font[LOW(id)];
    if (font->written > font->capacity) {
        return -1;
    }
    for (; ch = *(u8 *)f, ch; ++f) {
        if (ch != '%') {
            WriteChar(ch);
            continue;
        }

        ch = *(u8 *)++f;
        if (ch == '%') {
            WriteChar('%');
            continue;
        }

        width = 0;
        padZeros = ch == '0';
        while (ch >= '0' && ch <= '9') {
            width = (width * 10) + (ch - '0');
            ch = *(u8 *)++f;
        }
        bufPtr = (char *)&args;
        if (width <= 0) {
            width = 1;
        }

        switch (ch) {
        case 'd':
            num = va_arg(args, s32);
            sign = 0;
            if (num < 0) {
                num = -num;
                sign = '-';
            }
            len = 0;
            do {
                do {
                    *--bufPtr = (num % 10U) + '0';
                    num /= 10U;
                    len++;
                } while (len == 0);
            } while (num != 0);
            if (sign != 0) {
                *--bufPtr = sign;
                len++;
            }
            break;

        case 'X':
        case 'x':
            len = 0;
            num = va_arg(args, s32);
            do {
                do {
                    *--bufPtr = hexdigits[num % 16U];
                    num /= 16U;
                    len++;
                } while (len == 0);
            } while (num != 0);
            if (padZeros) {
                while (len < width) {
                    *--bufPtr = '0';
                    len++;
                }
            }
            break;

        case 'c':
            *--bufPtr = va_arg(args, int);
            len = 1;
            break;

        case 's':
            bufPtr = va_arg(args, char *);
            len = strlen(bufPtr);
            break;
        }

        while (len < width) {
            WriteChar(' ');
            width--;
        }
        len--;
        while (len != -1) {
            WriteChar(*bufPtr++);
            len--;
        }
    }
    font->buffer[font->written] = 0;
    return font->written;
}

/* SetPolyG3: initialise a gouraud triangle primitive header. */
void __attribute__((section(".text_80144CCC"))) sub_80144CCC(void *p)
{
    setlen(p, 6);
    setcode(p, 0x30);
}

/* SetPolyF4: initialise a flat quad primitive header. */
void __attribute__((section(".text_80144CF4"))) sub_80144CF4(void *p)
{
    setlen(p, 5);
    setcode(p, 0x28);
}

/* SetSprt8: initialise an 8x8 sprite primitive header. */
void __attribute__((section(".text_80144D44"))) sub_80144D44(void *p)
{
    setlen(p, 3);
    setcode(p, 0x74);
}

/* SetTile: initialise a variable-size tile primitive header. */
void __attribute__((section(".text_80144DBC"))) sub_80144DBC(void *p)
{
    setlen(p, 3);
    setcode(p, 0x60);
}

/* SetBlockFill: initialise a VRAM fill primitive header. */
void __attribute__((section(".text_80144E78"))) sub_80144E78(void *p)
{
    setlen(p, 3);
    setcode(p, 0x02);
}

/* memclr (libetc intr.c, static): zero n words. */
void __attribute__((section(".text_801459D8"))) sub_801459D8(void *ptr, s32 size)
{
    s32 i;
    s32 *e = (s32 *)ptr;

    for (i = size - 1; i != -1; i--) {
        *e = 0;
        e++;
    }
}

/* memclr (libetc intr_vb.c, static): zero n words. */
void __attribute__((section(".text_80145B14"))) sub_80145B14(void *ptr, s32 size)
{
    s32 i;
    s32 *e = (s32 *)ptr;

    for (i = size - 1; i != -1; i--) {
        *e = 0;
        e++;
    }
}

/* memclr (libetc intr_dma.c, static): zero n words. */
void __attribute__((section(".text_80145DBC"))) sub_80145DBC(void *ptr, s32 size)
{
    s32 i;
    s32 *e = (s32 *)ptr;

    for (i = size - 1; i != -1; i--) {
        *e = 0;
        e++;
    }
}

/* __builtin_delete (libgcc operator delete): free a non-null pointer. */
void __attribute__((section(".text_80145EC4"))) sub_80145EC4(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

