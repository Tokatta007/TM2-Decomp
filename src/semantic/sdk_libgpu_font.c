/* LIBGPU font.c: FntFlush (0x80144258).
 * Adapted from PSY-Z decomp/src/libgpu/font.c (MIT, decomp/LICENSE); the
 * loop keeps PSY-Z's structure (separate ' ' arm that duplicates the
 * x-advance, re-reads of *buf), which is what gives the retail register
 * and spill allocation.
 * Recipe: --compiler gcc2.6.3, gp_size 0, no -fvolatile. Byte-identical.
 * (psyq3.3, the lib_r140 recipe, compiles the same source to 5 differing
 * words: it reloads the spilled r/g/b ints with lw where retail uses lbu.)
 * Library data is declared extern by retail address (see lib_r140.c). */

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef unsigned long u_long;
typedef unsigned char u_char;

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

typedef struct {
    TILE tile;          /* 0x00 */
    DR_MODE draw_mode;  /* 0x10 */
    int capacity;       /* 0x1C */
    SPRT_8 *sprites;    /* 0x20 */
    char *buffer;       /* 0x24 */
    int written;        /* 0x28 */
    int unwrap;         /* 0x2C */
} FntStream;

extern FntStream D_8017C43C[8];       /* Font[] */
extern int D_8017C5BC;                  /* n_fonts_open */
extern int D_8017C5C0;                  /* dump_id */

#define Font D_8017C43C
#define n_fonts_open D_8017C5BC
#define dump_id D_8017C5C0

extern void sub_80144C3C(void *p);      /* TermPrim */
extern void AddPrim(void *ot, void *p);
extern void DrawOTag(u_long *p);

/* FntFlush: draw a font stream's text (sprites, ~cRGB colour escapes) and
 * clear it; returns the primitive list. */
u_long *__attribute__((section(".text_80144258"))) sub_80144258(int id)
{
    s32 line_break;
    u8 u;
    DR_MODE *dr;
    FntStream *font;
    s32 xscreen;
    SPRT_8 *sprt;
    int x;
    s32 len;
    s32 max_y;
    int r;
    s32 unwrap;
    int g;
    int b;
    int y;
    s32 ch2;
    u8 v;
    char *buf;
    s32 max_x;

    xscreen = 0;
    r = 0x80;
    g = 0x80;
    b = 0x80;
    if (id < 0 || id >= n_fonts_open) {
        id = dump_id;
        if (!Font[id].buffer) {
            return 0;
        }
    }
    font = &Font[id];
    dr = &font->draw_mode;
    sprt = font->sprites;
    unwrap = font->unwrap;
    buf = font->buffer;
    len = font->capacity;
    x = font->tile.x0;
    y = font->tile.y0;
    max_x = font->tile.w;
    max_x = x + (short)max_x;
    max_y = y + font->tile.h;
    sub_80144C3C(dr);
    while (*buf) {
        if (!len) {
            break;
        }
        line_break = 0;
        ch2 = *buf;
        if (ch2 != ' ') {
            if (ch2 <= ' ') {
                switch (ch2) {
                case '\t':
                    goto do_tab;
                case '\n':
                    goto set_linebreak;
                }
                goto do_char;
            } else if (ch2 == '~') {
                if (*++buf == 'c') {
                    r = (*++buf - '0') * 16;
                    g = (*++buf - '0') * 16;
                    b = (*++buf - '0') * 16;
                }
            } else {
                goto do_char;
            do_tab:
                x += 0x20;
                goto check_x_boundary;
            do_char:
                if (*buf >= 'a' && *buf <= 'z') {
                    ch2 = *buf - 0x40;
                } else {
                    ch2 = *buf - 0x20;
                }
                u = (ch2 % 16) * 8;
                v = (ch2 / 16) * 8;
                sprt->u0 = u;
                sprt->v0 = v;
                sprt->x0 = x;
                sprt->y0 = y;
                sprt->r0 = r;
                sprt->g0 = g;
                sprt->b0 = b;
                AddPrim(dr, sprt++);
                x += 8;
            check_x_boundary:
                if (x >= max_x && !unwrap) {
                set_linebreak:
                    line_break = 1;
                }
            }
        } else {
            x += 8;
            if (x >= max_x && !unwrap) {
                line_break = 1;
            }
        }
        if (line_break) {
            if (xscreen < x) {
                xscreen = x;
            }
            y += 8;
            x = font->tile.x0;
            if (y >= max_y) {
                break;
            }
        }
        if (!*++buf) {
            break;
        }
        len--;
    }
    if (font->tile.code) {
        AddPrim(dr, &font->tile);
        if (unwrap) {
            unwrap = xscreen;
            font->tile.w = unwrap - font->tile.x0;
            font->tile.h = y - (s16)(font->tile.y0 - 8);
        }
    }
    DrawOTag((u_long *)dr);
    font->written = 0;
    *font->buffer = '\0';
    return (u_long *)dr;
}
