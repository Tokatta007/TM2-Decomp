/* PsyQ SDK LIBGPU, reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * Adapted from the MIT-licensed psxsdk sources of sotn-decomp
 * (src/main/psxsdk/libgpu/{sys.c,prim.c,ext.c,font.c}) and of PSY-Z
 * (decomp/src/libgpu/{sys.c,prim.c,ext.c}).
 * SPDX-License-Identifier: MIT
 *
 * Library version: sys.c v1.116 1995/12/01 suzu (between sotn's v1.83 and
 * PSY-Z's v1.129); _addque2/_exeque/get_alarm were ported by hand against the
 * retail code.
 *
 * Matching notes: info.w/h, info.pending, info.drawSyncCb, the queue and
 * the last-command log are volatile in this version (lhu+sll+sra loads,
 * re-reads); get_mode/get_ofs read info.version through a volatile view.
 * FntLoad follows sotn font.c; SetDefDrawEnv's getTPage() macro still calls
 * GetGraphType (0x801413B4) twice and its GetVideoMode result is unused.
 *
 * Library data lives at its retail addresses and is declared extern by
 * address (D_XXXXXXXX). Functions already in other units (ResetGraph,
 * checkRECT, _clr, _ctl, _cwb, _cwc, _param, PutDispEnv, ...) are called by
 * their retail address name.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;

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
    u_long code[15];
} DR_ENV;

typedef struct {
    u_long tag;
    u_long code[2];
} DR_TWIN, DR_AREA, DR_OFFSET, DR_MODE;

typedef struct {
    RECT clip;          /* 0x00 */
    s16 ofs[2];         /* 0x08 */
    RECT tw;            /* 0x0C */
    u16 tpage;          /* 0x14 */
    u8 dtd;             /* 0x16 */
    u8 dfe;             /* 0x17 */
    u8 isbg;            /* 0x18 */
    u8 r0, g0, b0;      /* 0x19 */
    DR_ENV dr_env;      /* 0x1C */
} DRAWENV;              /* 0x5C */

typedef struct {
    RECT disp;          /* 0x00 */
    RECT screen;        /* 0x08 */
    u8 isinter;         /* 0x10 */
    u8 isrgb24;         /* 0x11 */
    u8 pad0;            /* 0x12 */
    u8 pad1;            /* 0x13 */
} DISPENV;              /* 0x14 */

#define setlen(p, _len) (((P_TAG *)(p))->len = (u_char)(_len))
#define setaddr(p, _addr) (((P_TAG *)(p))->addr = (u_long)(_addr))
#define setcode(p, _code) (((P_TAG *)(p))->code = (u_char)(_code))
#define getlen(p) (u_char)(((P_TAG *)(p))->len)
#define getcode(p) (u_char)(((P_TAG *)(p))->code)
#define termPrim(p) setaddr(p, 0xffffffff)
#define setSemiTrans(p, abe) \
    ((abe) ? setcode(p, getcode(p) | 0x02) : setcode(p, getcode(p) & ~0x02))
#define setShadeTex(p, tge) \
    ((tge) ? setcode(p, getcode(p) | 0x01) : setcode(p, getcode(p) & ~0x01))
#define getTPage(tp, abr, x, y)                                         \
    ((sub_801413B4() == 1 || sub_801413B4() == 2)                       \
         ? ((((tp) & 0x3) << 9) | (((abr) & 0x3) << 7) |                \
            (((y) & 0x300) >> 3) | (((x) & 0x3ff) >> 6))                \
         : ((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) |                \
            (((y) & 0x100) >> 4) | (((x) & 0x3ff) >> 6) |               \
            (((y) & 0x200) << 2)))
#define getClut(x, y) ((y << 6) | ((x >> 4) & 0x3f))

#ifndef CLAMP
#define CLAMP(value, low, high) \
    value < low ? low : (value > high ? high : value)
#endif

typedef struct {
    const char *ver;
    int (*addque)(int (*exec)(), u_long p1, u_long p2);
    int (*addque2)(int (*exec)(), u_long p1, int len, u_long p2);
    int (*clr)();
    void (*ctl)(unsigned int);
    int (*cwb)(u32 *data, s32 n);
    int (*cwc)();
    int (*drs)();
    int (*dws)();
    int (*exeque)(void);
    int (*getctl)(int);
    int (*otc)(u_long *ot, s32 n);
    int (*param)(int);
    int (*reset)(int);
    u_long (*status)(void);
    int (*sync)(int mode);
} GpuVtable;

struct Debug {
    u_char version;     /* 0x00 */
    u_char queue;       /* 0x01 */
    u_char level;       /* 0x02 */
    u_char reverse;     /* 0x03 */
    volatile short w;   /* 0x04 */
    volatile short h;   /* 0x06 */
    volatile int pending; /* 0x08 DrawSync callback armed */
    void (*volatile drawSyncCb)(); /* 0x0C */
    DRAWENV draw;       /* 0x10 */
    DISPENV disp;       /* 0x6C */
};

struct QueueItem {
    int (*exec)(u_long, u_long); /* 0x00 */
    u_long p1;                   /* 0x04 */
    u_long p2;                   /* 0x08 */
    u_long data[21];             /* 0x0C */
};                               /* 0x60 */

/* ---- library data (retail addresses) ---- */
extern GpuVtable *gpuDriver;           /* gpu */
extern int (*gpuPrintf)();             /* GPU_printf */
extern struct Debug gpuSysState;         /* info */
extern u_long D_8017C3C4[5];            /* move_image */
extern u_long D_8017C3D8[];             /* terminator packet */
extern volatile u_long *D_8017C3EC;     /* GPU_DATA  0x1F801810 */
extern volatile u_long *D_8017C3F0;     /* GPU_STATUS 0x1F801814 */
extern volatile u_long *D_8017C3F4;     /* DMA2_MADR */
extern volatile u_long *D_8017C3F8;     /* DMA2_BCR */
extern volatile u_long *D_8017C3FC;     /* DMA2_CHCR */
extern volatile u_long *D_8017C400;     /* DMA6_MADR */
extern volatile u_long *D_8017C404;     /* DMA6_BCR */
extern volatile u_long *D_8017C408;     /* DMA6_CHCR */
extern volatile u_long *D_8017C40C;     /* DPCR */
extern volatile u_long D_8017C410[3];            /* last executed queue entry */
extern volatile int D_8017C420;         /* _qin */
extern volatile int D_8017C424;         /* _qout */
extern int D_8017C428;                  /* _addque2 interrupt mask */
extern int D_8017C42C;                  /* _exeque interrupt mask */
extern int D_8017C430;                  /* _reset interrupt mask */
extern int D_8017C434;                  /* alarm vsync */
extern int D_8017C438;                  /* alarm counter */
extern u_char D_80193950[];             /* ctlbuf */
extern volatile struct QueueItem D_801F72A0[64]; /* _que */

extern u_char D_8017C43C[];             /* Font[] */
extern int D_8017C5BC;                  /* n_fonts_open */
extern u_long D_8017C5C4[];             /* font clut + 4bpp image */
extern u16 D_80197E50;                  /* font tpage */
extern u16 D_80197E54;                  /* font clut */

extern char D_800D09D4[];   /* "SetGraphReverse(%d)...\n" */
extern char D_800D09EC[];   /* "SetGraphDebug:level:%d,type:%d reverse:%d\n" */
extern char D_800D0A18[];   /* "SetGrapQue(%d)...\n" */
extern char D_800D0A48[];   /* "SetDispMask(%d)...\n" */
extern char D_800D0A5C[];   /* "DrawSync(%d)...\n" */
extern char D_800D0A94[];   /* "ClearImage" */
extern char D_800D0AA0[];   /* "LoadImage" */
extern char D_800D0AAC[];   /* "StoreImage" */
extern char D_800D0AB8[];   /* "MoveImage" */
extern char D_800D0AC4[];   /* "ClearOTag(%08x,%d)...\n" */
extern char D_800D0ADC[];   /* "ClearOTagR(%08x,%d)...\n" */
extern char D_800D0B08[];   /* "PutDrawEnv(%08x)...\n" */
extern char D_800D0B38[];   /* "GPU timeout:que=%d,stat=%08x,chcr=%08x,madr=%08x," */
extern char D_800D0B6C[];   /* "func=(%08x)(%08x,%08x)\n" */
extern char D_800D0C20[];   /* "tpage: (%d,%d,%d,%d)\n" */
extern char D_800D0C38[];   /* "clut: (%d,%d)\n" */
extern char D_800D0C48[];   /* "clip (%3d,%3d)-(%d,%d)\n" */
extern char D_800D0C60[];   /* "ofs  (%3d,%3d)\n" */
extern char D_800D0C70[];   /* "tw   (%d,%d)-(%d,%d)\n" */
extern char D_800D0C88[];   /* "dtd   %d\n" */
extern char D_800D0C94[];   /* "dfe   %d\n" */
extern char D_800D0CA0[];   /* "disp   (%3d,%3d)-(%d,%d)\n" */
extern char D_800D0CBC[];   /* "screen (%3d,%3d)-(%d,%d)\n" */
extern char D_800D0CD8[];   /* "isinter %d\n" */
extern char D_800D0CE4[];   /* "isrgb24 %d\n" */

/* ---- other units / libraries ---- */
extern int sub_801413B4(void);                  /* GetGraphType */
extern void sub_80141538(char *, RECT *);       /* checkRECT */
extern u_long sub_801424B4(short, short);       /* get_cs */
extern u_long sub_80142580(short, short);       /* get_ce */
extern void sub_80143C24(void *, int, int);     /* sys.c memset */
extern int sub_8014510C(int);                   /* VSync */
extern int sub_8014546C(int);                   /* SetIntrMask */
extern void *sub_80145350(int, int (*)());      /* DMACallback */
extern int sub_801388A8(const char *, ...);     /* printf */
extern void *memcpy(void *, void *, int);
extern void *sub_80144968(void *, int, int);    /* memset */
extern int GsGetWorkBase(void);                 /* GetVideoMode (0x80145E00) */

/* forward declarations of this unit */
int sub_801416F4(RECT *rect, u_long *p);
void sub_801421B4(DR_ENV *dr_env, DRAWENV *env);
u_long sub_80142458(int dfe, int dtd, u_short tpage);
u_long sub_8014264C(short x, short y);
u_long sub_80142694(RECT *rect);
int sub_80143118(int (*exec)(), u_long p1, int len, u_long p2);
int sub_8014340C(void);
int sub_80143720(int mode);
void sub_801439A4(void);
int sub_801439D8(void);
int sub_80143B44(int mode);
u_short sub_80143C50(u_long *pix, int tp, int abr, int x, int y, int w, int h);
u_short sub_80143D3C(u_long *clut, int x, int y);
u_short sub_80144978(int tp, int abr, int x, int y);
u_short sub_80144A40(int x, int y);

#define gpu gpuDriver
#define GPU_printf gpuPrintf
#define info gpuSysState
#define GPU_DATA D_8017C3EC
#define GPU_STATUS D_8017C3F0
#define DMA2_MADR D_8017C3F4
#define DMA2_BCR D_8017C3F8
#define DMA2_CHCR D_8017C3FC
#define DMA6_MADR D_8017C400
#define DMA6_BCR D_8017C404
#define DMA6_CHCR D_8017C408
#define DPCR D_8017C40C
#define _qin D_8017C420
#define _qout D_8017C424
#define _que D_801F72A0


#define VVER (*(volatile u_char *)&info.version)
#define STATUS_READY_TO_RECEIVE_CMD (1 << 26)
#define STATUS_READY_TO_SEND_VRAM_TO_CPU (1 << 27)

/* SetGraphReverse: set the display reverse flag; returns the old one. */
int __attribute__((section(".text_80141188"))) sub_80141188(int mode)
{
    u_char prev = info.reverse;
    if (info.level >= 2) {
        GPU_printf(D_800D09D4, mode);
    }
    info.reverse = mode;
    gpu->ctl(gpu->getctl(8) | (info.reverse ? 0x08000080 : 0x08000000));
    if (info.version == 2) {
        gpu->ctl(info.reverse ? 0x20000501 : 0x20000504);
    }
    return prev;
}

/* SetGraphDebug: set the libgpu debug level; returns the old one. */
int __attribute__((section(".text_8014129C"))) sub_8014129C(int level)
{
    u_char prev = info.level;
    if ((info.level = level)) {
        GPU_printf(D_800D09EC, info.level, info.version, info.reverse);
    }
    return prev;
}

/* SetGraphQueue: switch queued/immediate drawing; returns the old mode. */
int __attribute__((section(".text_80141308"))) sub_80141308(int mode)
{
    u_char prev = info.queue;
    if (info.level >= 2) {
        GPU_printf(D_800D0A18, mode);
    }
    if (mode != info.queue) {
        gpu->reset(1);
        info.queue = mode;
        sub_80145350(2, 0);
    }
    return prev;
}

/* SetDispMask: blank (0) or enable the display. */
void __attribute__((section(".text_80141430"))) sub_80141430(int mask)
{
    if (info.level >= 2) {
        GPU_printf(D_800D0A48, mask);
    }
    if (mask == 0) {
        sub_80143C24(&info.disp, -1, sizeof(DISPENV));
    }
    gpu->ctl(mask ? 0x03000000 : 0x03000001);
}

/* DrawSync: wait for (mode 0) or poll the drawing queue. */
int __attribute__((section(".text_801414CC"))) sub_801414CC(int mode)
{
    if (info.level >= 2) {
        GPU_printf(D_800D0A5C, mode);
    }
    return gpu->sync(mode);
}

/* ClearImage: queue a VRAM rectangle fill. */
int __attribute__((section(".text_80141660"))) sub_80141660(RECT *rect, u8 r, u8 g, u8 b)
{
    sub_80141538(D_800D0A94, rect);
    return gpu->addque2(gpu->clr, (u_long)rect, sizeof(RECT),
                        (b << 0x10) | (g << 8) | r);
}

/* LoadImage: queue a main-memory -> VRAM transfer. */
int __attribute__((section(".text_801416F4"))) sub_801416F4(RECT *rect, u_long *p)
{
    sub_80141538(D_800D0AA0, rect);
    return gpu->addque2(gpu->dws, (u_long)rect, sizeof(RECT), (u_long)p);
}

/* StoreImage: queue a VRAM -> main-memory transfer. */
int __attribute__((section(".text_80141758"))) sub_80141758(RECT *rect, u_long *p)
{
    sub_80141538(D_800D0AAC, rect);
    return gpu->addque2(gpu->drs, (u_long)rect, sizeof(RECT), (u_long)p);
}

/* MoveImage: queue a VRAM -> VRAM copy. */
int __attribute__((section(".text_801417BC"))) sub_801417BC(RECT *rect, int x, int y)
{
    sub_80141538(D_800D0AB8, rect);
    if (!rect->w || !rect->h) {
        return -1;
    }
    D_8017C3C4[2] = *(int *)&rect->x;
    D_8017C3C4[3] = y << 0x10 | (u16)x;
    D_8017C3C4[4] = *(int *)&rect->w;
    return gpu->addque2(gpu->cwc, (u_long)D_8017C3C4, sizeof(D_8017C3C4), 0);
}

/* ClearOTag: link an ordering table forwards, ending at the terminator. */
u_long *__attribute__((section(".text_80141880"))) sub_80141880(u_long *ot, int n)
{
    if (info.level >= 2) {
        GPU_printf(D_800D0AC4, ot, n);
    }
    while (--n) {
        setlen(ot, 0);
        setaddr(ot, ot + 1);
        ot++;
    }
    *ot = (u_long)D_8017C3D8 & 0xFFFFFF;
    return ot;
}

/* ClearOTagR: link an ordering table backwards with DMA channel 6. */
u_long *__attribute__((section(".text_80141938"))) sub_80141938(u_long *ot, int n)
{
    if (info.level >= 2) {
        GPU_printf(D_800D0ADC, ot, n);
    }
    gpu->otc(ot, n);
    *ot = (u_long)D_8017C3D8 & 0xFFFFFF;
    return ot;
}

/* DrawPrim: draw one primitive immediately (waits for the queue). */
void __attribute__((section(".text_801419D0"))) sub_801419D0(void *p)
{
    int len = getlen(p);
    gpu->sync(0);
    gpu->cwb((u32 *)&((P_TAG *)p)->r0, len);
}

/* PutDrawEnv: build and queue the drawing environment packet, cache it. */
DRAWENV *__attribute__((section(".text_80141AA4"))) sub_80141AA4(DRAWENV *env)
{
    if (info.level >= 2) {
        GPU_printf(D_800D0B08, env);
    }
    sub_801421B4(&env->dr_env, env);
    termPrim(&env->dr_env);
    gpu->addque2(gpu->cwc, (u_long)&env->dr_env, sizeof(DR_ENV), 0);
    memcpy(&info.draw, env, sizeof(DRAWENV));
    return env;
}

/* GetODE: odd/even interlace field bit of GPUSTAT. */
int __attribute__((section(".text_80141FFC"))) sub_80141FFC(void)
{
    return gpu->status() >> 0x1F;
}

/* SetTexWindow: build a texture-window primitive. */
void __attribute__((section(".text_80142030"))) sub_80142030(DR_TWIN *p, RECT *tw)
{
    setlen(p, 2);
    p->code[0] = sub_80142694(tw);
    p->code[1] = 0;
}

/* SetDrawArea: build a drawing-area primitive. */
void __attribute__((section(".text_8014206C"))) sub_8014206C(DR_AREA *p, RECT *r)
{
    setlen(p, 2);
    p->code[0] = sub_801424B4(r->x, r->y);
    p->code[1] = sub_80142580(r->x + r->w - 1, r->y + r->h - 1);
}

/* SetDrawOffset: build a drawing-offset primitive. */
void __attribute__((section(".text_801420F0"))) sub_801420F0(DR_OFFSET *p, u_short *ofs)
{
    setlen(p, 2);
    p->code[0] = sub_8014264C(ofs[0], ofs[1]);
    p->code[1] = 0;
}

/* SetDrawMode: build a draw-mode (tpage + texture window) primitive. */
void __attribute__((section(".text_8014215C"))) sub_8014215C(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw)
{
    setlen(p, 2);
    p->code[0] = sub_80142458(dfe, dtd, tpage);
    p->code[1] = sub_80142694(tw);
}

/* min(a, b) as an inline function: the inlined return value gets its own
 * temporary, which SetDrawEnv's clamp code needs (lim < v ? lim : v, then a
 * copy into the result register). */
static __inline__ int sde_min(int a, int b)
{
    return b > a ? a : b;
}

/* SetDrawEnv: build the drawing-environment packet (optionally with a
 * background clear; the clear size is clamped to 0..info.w-1 / info.h-1). */
void __attribute__((section(".text_801421B4"))) sub_801421B4(DR_ENV *dr_env, DRAWENV *env)
{
#define PUSH_CODE ((u_long *)(&dr->code[-1]))[len++]
    DR_ENV *dr;
    int len;
    RECT rect;

    len = 1;
    dr = dr_env;
    PUSH_CODE = sub_801424B4(env->clip.x, env->clip.y);
    PUSH_CODE = sub_80142580(env->clip.w + env->clip.x - 1,
                             env->clip.y + env->clip.h - 1);
    PUSH_CODE = sub_8014264C(env->ofs[0], env->ofs[1]);
    PUSH_CODE = sub_80142458(env->dfe, env->dtd, env->tpage);
    PUSH_CODE = sub_80142694(&env->tw);
    PUSH_CODE = 0xE6000000;
    if (env->isbg) {
        rect.x = env->clip.x;
        rect.y = env->clip.y;
        rect.w = env->clip.w;
        rect.h = env->clip.h;
        rect.w = rect.w < 0 ? 0 : sde_min(info.w - 1, rect.w);
        rect.h = rect.h < 0 ? 0 : sde_min(info.h - 1, rect.h);
        if (rect.x & 0x3F || rect.w & 0x3F) {
            rect.x -= env->ofs[0];
            rect.y -= env->ofs[1];
            PUSH_CODE = 0x60000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            PUSH_CODE = *(u32 *)&rect.x;
            PUSH_CODE = *(u32 *)&rect.w;
            rect.x += env->ofs[0];
            rect.y += env->ofs[1];
        } else {
            PUSH_CODE = 0x02000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            PUSH_CODE = *(u32 *)&rect.x;
            PUSH_CODE = *(u32 *)&rect.w;
        }
    }
    setlen(dr, len - 1);
#undef PUSH_CODE
}

/* get_mode: GP0(E1h) draw-mode word for the detected GPU type. */
u_long __attribute__((section(".text_80142458"))) sub_80142458(int dfe, int dtd, u_short tpage)
{
    if ((u_char)(VVER - 1) < 2) {
        return (dtd ? 0xE1000800 : 0xE1000000) | (dfe ? 0x1000 : 0) |
               (tpage & 0x27FF);
    } else {
        return (dtd ? 0xE1000200 : 0xE1000000) | (dfe ? 0x400 : 0) |
               (tpage & 0x9FF);
    }
}

/* get_ofs: GP0(E5h) drawing-offset word. */
u_long __attribute__((section(".text_8014264C"))) sub_8014264C(short x, short y)
{
    if ((u_char)(VVER - 1) < 2) {
        return 0xE5000000 | ((y & 0xFFF) << 12) | (x & 0xFFF);
    } else {
        return 0xE5000000 | ((y & 0x7FF) << 11) | (x & 0x7FF);
    }
}

/* get_tw: GP0(E2h) texture-window word (0 for no window). */
u_long __attribute__((section(".text_80142694"))) sub_80142694(RECT *rect)
{
    u_long code[4];
    if (rect) {
        code[0] = (rect->x & 0xFF) >> 3;
        code[2] = (-rect->w & 0xFF) >> 3;
        code[1] = (rect->y & 0xFF) >> 3;
        code[3] = (-rect->h & 0xFF) >> 3;
        return 0xE2000000 | (code[1] << 15) | (code[0] << 10) | (code[3] << 5) |
               code[2];
    }
    return 0;
}

/* _otc: clear an ordering table backwards with DMA channel 6. */
int __attribute__((section(".text_801427EC"))) sub_801427EC(u_long *ot, int n)
{
    *DPCR |= 0x08000000;
    *DMA6_CHCR = 0;
    *DMA6_MADR = (u_long)(ot - 1 + n);
    *DMA6_BCR = n;
    *DMA6_CHCR = 0x11000002;
    sub_801439A4();
    while (*DMA6_CHCR & 0x01000000) {
        if (sub_801439D8()) {
            return -1;
        }
    }
    return n;
}

/* _dws: write an image to VRAM (FIFO for the remainder, DMA for 16-word blocks). */
int __attribute__((section(".text_80142AF4"))) sub_80142AF4(RECT *rect, u_long *data)
{
    int to_write;
    int size;
    int var_s0;
    int var_s4;

    sub_801439A4();
    var_s4 = 0;
    rect->w = rect->w < 0 ? 0 : (info.w < rect->w ? info.w : rect->w);
    rect->h = rect->h < 0 ? 0 : (info.h < rect->h ? info.h : rect->h);
    to_write = (rect->w * rect->h + 1) / 2;
    if (to_write <= 0) {
        return -1;
    }
    var_s0 = to_write % 16;
    size = to_write / 16;
    while (!(*GPU_STATUS & STATUS_READY_TO_RECEIVE_CMD)) {
        if (sub_801439D8()) {
            return -1;
        }
    }

    *GPU_STATUS = 0x04000000;
    *GPU_DATA = 0x01000000;
    {
        volatile u_long *d = GPU_DATA;
        u_long cmd = 0xA0000000;
        if (var_s4) cmd = 0xB0000000;
        *d = cmd;
    }
    *GPU_DATA = *(s32 *)&rect->x;
    *GPU_DATA = *(s32 *)&rect->w;

    while (var_s0--) {
        *GPU_DATA = *data++;
    }
    if (size) {
        *GPU_STATUS = 0x04000002;
        *DMA2_MADR = (u_long)data;
        *DMA2_BCR = size << 16 | 0x10;
        *DMA2_CHCR = 0x01000201;
    }
    return 0;
}

/* _drs: read an image from VRAM. */
int __attribute__((section(".text_80142D48"))) sub_80142D48(RECT *rect, u_long *data)
{
    int to_read;
    int size;
    int var_s0;

    sub_801439A4();
    rect->w = rect->w < 0 ? 0 : (info.w < rect->w ? info.w : rect->w);
    rect->h = rect->h < 0 ? 0 : (info.h < rect->h ? info.h : rect->h);
    to_read = (rect->w * rect->h + 1) / 2;
    if (to_read <= 0) {
        return -1;
    }
    var_s0 = to_read % 16;
    size = to_read / 16;
    while (!(*GPU_STATUS & STATUS_READY_TO_RECEIVE_CMD)) {
        if (sub_801439D8()) {
            return -1;
        }
    }

    *GPU_STATUS = 0x04000000;
    *GPU_DATA = 0x01000000;
    *GPU_DATA = 0xC0000000;
    *GPU_DATA = *(s32 *)&rect->x;
    *GPU_DATA = *(s32 *)&rect->w;

    while (!(*GPU_STATUS & STATUS_READY_TO_SEND_VRAM_TO_CPU)) {
        if (sub_801439D8()) {
            return -1;
        }
    }

    while (var_s0--) {
        *data++ = *GPU_DATA;
    }

    if (size != 0) {
        *GPU_STATUS = 0x04000003;
        *DMA2_MADR = (u_long)data;
        *DMA2_BCR = (size << 0x10) | 0x10;
        *DMA2_CHCR = 0x01000200;
    }
    return 0;
}

/* _addque: queue a command with no parameter copy. */
int __attribute__((section(".text_801430F4"))) sub_801430F4(int (*exec)(), u_long p1, u_long p2)
{
    return sub_80143118(exec, p1, 0, p2);
}

/* _addque2: run a command now if the GPU is idle, otherwise copy its
 * parameter block into the 64-entry queue and kick _exeque. */
int __attribute__((section(".text_80143118"))) sub_80143118(int (*exec)(), u_long p1, int len, u_long p2)
{
    int i;

    sub_801439A4();
    while (((_qin + 1) & 0x3F) == _qout) {
        if (sub_801439D8()) {
            return -1;
        }
        sub_8014340C();
    }
    D_8017C428 = sub_8014546C(0);
    info.pending = 1;
    if (info.queue == 0 ||
        (_qin == _qout && !(*DMA2_CHCR & 0x01000000) && info.drawSyncCb == 0)) {
        do {
        } while (!(*GPU_STATUS & 0x04000000));
        exec(p1, p2);
        D_8017C410[0] = (u_long)exec;
        D_8017C410[1] = p1;
        D_8017C410[2] = p2;
        sub_8014546C(D_8017C428);
        return 0;
    }
    sub_80145350(2, sub_8014340C);
    if (len) {
        for (i = 0; i < len / 4; i++) {
            _que[_qin].data[i] = ((u_long *)p1)[i];
        }
        _que[_qin].p1 = (u_long)_que[_qin].data;
    } else {
        _que[_qin].p1 = p1;
    }
    _que[_qin].p2 = p2;
    _que[_qin].exec = exec;
    _qin = (_qin + 1) & 0x3F;
    sub_8014546C(D_8017C428);
    sub_8014340C();
    return (_qin - _qout) & 0x3F;
}

/* _exeque: run queued commands while DMA channel 2 is idle; fires the
 * DrawSync callback once the queue drains. Returns the queue depth. */
int __attribute__((section(".text_8014340C"))) sub_8014340C(void)
{
    if (*DMA2_CHCR & 0x01000000) {
        return 1;
    }
    D_8017C42C = sub_8014546C(0);
    while (_qin != _qout && !(*DMA2_CHCR & 0x01000000)) {
        if (((_qout + 1) & 0x3F) == _qin && info.drawSyncCb == 0) {
            sub_80145350(2, 0);
        }
        while (!(*GPU_STATUS & 0x04000000)) {
        }
        _que[_qout].exec(_que[_qout].p1, _que[_qout].p2);
        D_8017C410[0] = (u_long)_que[_qout].exec;
        D_8017C410[1] = _que[_qout].p1;
        D_8017C410[2] = _que[_qout].p2;
        _qout = (_qout + 1) & 0x3F;
    }
    sub_8014546C(D_8017C42C);
    if (_qin == _qout && !(*DMA2_CHCR & 0x01000000) && info.pending &&
        info.drawSyncCb) {
        info.pending = 0;
        info.drawSyncCb();
    }
    return (_qin - _qout) & 0x3F;
}

/* _reset: flush the queue and reset the GPU (mode 0: full reset + detect
 * GPU type, 1: cancel drawing only). */
__inline__ int __attribute__((section(".text_80143720"))) sub_80143720(int mode)
{
    D_8017C430 = sub_8014546C(0);
    _qout = 0;
    _qin = _qout;
    switch (mode & 7) {
    case 0:
        *DMA2_CHCR = 0x401;
        *DPCR |= 0x800;
        *GPU_STATUS = 0;
        sub_80143C24(D_80193950, 0, 0x100);
        sub_80143C24((void *)_que, 0, sizeof(_que));
        break;
    case 1:
        *DMA2_CHCR = 0x401;
        *DPCR |= 0x800;
        *GPU_STATUS = 0x02000000;
        *GPU_STATUS = 0x01000000;
        break;
    }
    sub_8014546C(D_8017C430);
    return !(mode & 7) ? sub_80143B44(mode) : 0;
}

/* _sync: mode 0 waits for queue/DMA/GPU idle (-1 on timeout); otherwise
 * returns the number of pending entries (1 if only the GPU is busy). */
int __attribute__((section(".text_8014385C"))) sub_8014385C(int mode)
{
    int queued;
    if (mode == 0) {
        sub_801439A4();
        while (_qin != _qout) {
            sub_8014340C();
            if (sub_801439D8()) {
                return -1;
            }
        }
        while (*DMA2_CHCR & 0x01000000 || !(*GPU_STATUS & 0x04000000)) {
            if (sub_801439D8()) {
                return -1;
            }
        }
        return 0;
    }
    queued = (_qin - _qout) & 0x3F;
    if (queued) {
        sub_8014340C();
    }
    if (*DMA2_CHCR & 0x01000000 || !(*GPU_STATUS & 0x04000000)) {
        if (!queued) {
            return 1;
        }
        return queued;
    }
    return queued;
}

/* set_alarm: arm the GPU timeout (240 vblanks from now). */
void __attribute__((section(".text_801439A4"))) sub_801439A4(void)
{
    D_8017C434 = sub_8014510C(-1) + 0xF0;
    D_8017C438 = 0;
}

/* get_alarm: on timeout print the GPU state, reset the queue and return -1. */
int __attribute__((section(".text_801439D8"))) sub_801439D8(void)
{
    if (D_8017C434 < sub_8014510C(-1) || D_8017C438++ > 0xF0000) {
        *GPU_STATUS;
        sub_801388A8(D_800D0B38, (_qin - _qout) & 0x3F, *GPU_STATUS,
                     *DMA2_CHCR, *DMA2_MADR);
        sub_801388A8(D_800D0B6C, D_8017C410[0], D_8017C410[1], D_8017C410[2]);
        sub_80143720(1);
        return -1;
    }
    return 0;
}

/* _version: detect the GPU type (0..4) and set the VRAM mode. */
int __attribute__((section(".text_80143B44"))) sub_80143B44(int mode)
{
    *GPU_STATUS = 0x10000007;
    if ((*GPU_DATA & 0xFFFFFF) != 2) {
        *GPU_DATA = 0xE1001000 | (*GPU_STATUS & 0x3FFF);
        *GPU_DATA;
        if (!(*GPU_STATUS & 0x1000)) {
            return 0;
        }
        if (!(mode & 8)) {
            return 1;
        }
        *GPU_STATUS = 0x20000504;
        return 2;
    } else if (!(mode & 8)) {
        return 3;
    } else {
        *GPU_STATUS = 0x09000001;
        return 4;
    }
}

/* LoadTPage: upload a texture page image and return its tpage id. */
u_short __attribute__((section(".text_80143C50"))) sub_80143C50(u_long *pix, int tp, int abr, int x, int y, int w, int h)
{
    RECT rect;

    rect.x = x;
    rect.y = y;
    rect.h = h;
    switch (tp) {
    case 0:
        rect.w = w / 4;
        break;
    case 1:
        rect.w = w / 2;
        break;
    case 2:
        rect.w = w;
        break;
    }
    sub_801416F4(&rect, pix);
    return sub_80144978(tp, abr, x, y);
}

/* LoadClut: upload a 256-entry CLUT and return its clut id. */
u_short __attribute__((section(".text_80143D3C"))) sub_80143D3C(u_long *clut, int x, int y)
{
    RECT rect;

    rect.x = x;
    rect.y = y;
    rect.w = 0x100;
    rect.h = 1;
    sub_801416F4(&rect, clut);
    return sub_80144A40(x, y);
}

/* SetDefDrawEnv: default drawing environment for a w*h area at (x,y). */
DRAWENV *__attribute__((section(".text_80143DA4"))) sub_80143DA4(DRAWENV *env, int x, int y, int w, int h)
{
    GsGetWorkBase();
    env->clip.x = x;
    env->clip.y = y;
    env->clip.w = w;
    env->clip.h = h;
    env->tw.x = 0;
    env->tw.y = 0;
    env->tw.w = 0;
    env->tw.h = 0;
    env->r0 = 0;
    env->g0 = 0;
    env->b0 = 0;
    env->dtd = 1;
    env->dfe = 0;
    env->ofs[0] = x;
    env->ofs[1] = y;
    env->tpage = getTPage(0, 0, 640, 0);
    env->isbg = 0;
    return env;
}

/* SetDefDispEnv: default display environment for a w*h area at (x,y). */
DISPENV *__attribute__((section(".text_80143E64"))) sub_80143E64(DISPENV *env, int x, int y, int w, int h)
{
    env->disp.x = x;
    env->disp.y = y;
    env->disp.w = w;
    env->disp.h = h;
    env->screen.x = 0;
    env->screen.y = 0;
    env->screen.w = 0;
    env->screen.h = 0;
    env->isrgb24 = 0;
    env->isinter = 0;
    env->pad1 = 0;
    env->pad0 = 0;
    return env;
}

/* FntLoad: upload the debug font (clut + 4bpp page) and close all streams. */
void __attribute__((section(".text_80143EE0"))) sub_80143EE0(int tx, int ty)
{
    D_80197E54 = sub_80143D3C(D_8017C5C4, tx, ty + 0x80);
    D_80197E50 = sub_80143C50(D_8017C5C4 + 0x80, 0, 0, tx, ty, 0x80, 0x20);
    D_8017C5BC = 0;
    sub_80144968(D_8017C43C, 0, 0x180);
}

/* GetTPage: tpage id for texture mode, blend rate and VRAM position. */
u_short __attribute__((section(".text_80144978"))) sub_80144978(int tp, int abr, int x, int y)
{
    return getTPage(tp, abr, x, y);
}

/* GetClut: clut id for a VRAM position. */
u_short __attribute__((section(".text_80144A40"))) sub_80144A40(int x, int y)
{
    return getClut(x, y);
}

/* DumpTPage: print the fields of a tpage id. */
void __attribute__((section(".text_80144A58"))) sub_80144A58(u_short tpage)
{
    if (sub_801413B4() == 1 || sub_801413B4() == 2) {
        GPU_printf(D_800D0C20, (tpage >> 9) & 3, (tpage >> 7) & 3,
                   (tpage << 6) & 0x7C0, (tpage << 3) & 0x300);
    } else {
        GPU_printf(D_800D0C20, (tpage >> 7) & 3, (tpage >> 5) & 3,
                   (tpage << 6) & 0x7C0,
                   ((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200));
    }
}

/* DumpClut: print the VRAM position of a clut id. */
void __attribute__((section(".text_80144B28"))) sub_80144B28(u_short clut)
{
    GPU_printf(D_800D0C38, (clut & 0x3F) << 4, clut >> 6);
}

/* TermPrim: terminate a primitive list at p. */
void __attribute__((section(".text_80144C3C"))) sub_80144C3C(void *p)
{
    termPrim(p);
}

/* SetSemiTrans: set/clear the semi-transparency bit of a primitive. */
void __attribute__((section(".text_80144C54"))) sub_80144C54(void *p, int abe)
{
    setSemiTrans(p, abe);
}

/* SetShadeTex: set/clear the texture-shading-off bit of a primitive. */
void __attribute__((section(".text_80144C7C"))) sub_80144C7C(void *p, int tge)
{
    setShadeTex(p, tge);
}

/* DumpDrawEnv: print a drawing environment. */
void __attribute__((section(".text_80144EE0"))) sub_80144EE0(DRAWENV *env)
{
    GPU_printf(D_800D0C48, env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    GPU_printf(D_800D0C60, env->ofs[0], env->ofs[1]);
    GPU_printf(D_800D0C70, env->tw.x, env->tw.y, env->tw.w, env->tw.h);
    GPU_printf(D_800D0C88, env->dtd);
    GPU_printf(D_800D0C94, env->dfe);
    if (sub_801413B4() == 1 || sub_801413B4() == 2) {
        GPU_printf(D_800D0C20, (env->tpage >> 9) & 3, (env->tpage >> 7) & 3,
                   (env->tpage << 6) & 0x7C0, (env->tpage << 3) & 0x300);
    } else {
        GPU_printf(D_800D0C20, (env->tpage >> 7) & 3, (env->tpage >> 5) & 3,
                   (env->tpage << 6) & 0x7C0,
                   ((env->tpage << 4) & 0x100) + ((env->tpage >> 2) & 0x200));
    }
}

/* DumpDispEnv: print a display environment. */
void __attribute__((section(".text_80145060"))) sub_80145060(DISPENV *env)
{
    GPU_printf(D_800D0CA0, env->disp.x, env->disp.y, env->disp.w, env->disp.h);
    GPU_printf(D_800D0CBC, env->screen.x, env->screen.y, env->screen.w,
               env->screen.h);
    GPU_printf(D_800D0CD8, env->isinter);
    GPU_printf(D_800D0CE4, env->isrgb24);
}
