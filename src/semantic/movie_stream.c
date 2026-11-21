/* SingleTrac movie player (80138068..80138783): MDEC/CD-stream .STR playback built
 * on Sony's "tuto" streaming sample (movieStrInit / movieStrCallback / movieStrNextVlc / movieStrNext /
 * strSync / movieStrKickCd) plus moviePlay. Written by hand from the retail
 * disassembly (no public source used).
 *
 * Recipe: game recipe (psyq3.6 cc1, -fvolatile, extern_gp, gp_size 8). All 7
 * functions are byte-identical with retail. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct { s16 x, y, w, h; } RECT;

typedef struct {
    u32 *vlcbuf[2];     /* +0  */
    s32 vlcid;          /* +8  */
    u16 *imgbuf;        /* +12 */
    RECT rect[2];       /* +16 */
    s32 rectid;         /* +32 */
    RECT slice;         /* +36 */
    s32 isdone;         /* +44 */
} DECENV;

typedef struct {
    u16 id, type, secCount, nSectors;
    u32 frameCount, frameSize;
    u16 width, height;
    u32 dummy1, dummy2;
    u8 loc[4];
} StHEADER;

typedef struct {
    RECT disp, screen;
    u8 isinter, isrgb24, pad0, pad1;
} DISPENV;

typedef struct {
    u8 loc[4];
    u32 size;
    char name[16];
} CdlFILE;

extern u8 movieRgb24;           /* 24-bit (wide) movie mode */
extern u8 movieStartLoc[4];        /* movie start position (CdlLOC) */
extern s32 movieEndReached;          /* end of movie reached */
extern u32 *movieRingBuffer;         /* stream ring buffer */
extern s32 movieFrameNumber;          /* current frame number */
extern s32 movieFrameWidth;          /* frame width */
extern s32 movieFrameHeight;          /* frame height */
extern u32 *fileioWorkBuffer;
#pragma tm2_absolute fileioWorkBuffer
extern s32 D_801AB888;          /* libcd StCdIntrFlag */
#pragma tm2_absolute D_801AB888
extern DECENV movieDecEnv;       /* movie decode environment */

extern void sub_80139498(s32);                  /* DecDCTReset */
extern void sub_801396C8(void (*)());           /* DecDCToutCallback */
extern void sub_8013A540(u32 *, u32);           /* StSetRing */
extern void sub_8013DB10(u32, u32, u32, void (*)(), void (*)());  /* StSetStream */
extern void sub_8013DD78(void);                 /* StCdInterrupt */
extern int sub_801416F4(RECT *, u32 *);        /* LoadImage */
extern void sub_80139644(void *, u32);          /* DecDCTout */
extern s32 sub_80139B9C(u32 *, u32 *);          /* DecDCTvlc */
extern u32 sub_8013DB98(u32 *);                 /* StFreeRing */
extern u32 sub_8013DC94(u32 **, StHEADER **);   /* StGetNext */
extern s32 sub_8013A72C(u8, u8 *, u8 *);        /* CdControl */
extern s32 sub_8013D9B4(s32);                   /* CdRead2 */
extern CdlFILE *sub_8013CF8C(CdlFILE *, char *);/* CdSearchFile */
extern void sub_80145EF8(char a, char b, char c);        /* SsSetSerialAttr */
extern void sub_80145F8C(char a, short b, short c);        /* SsSetSerialVol */
extern void movieBuffersInit(DECENV *, s32, s32, s32, s32, s32, s32, s32);
extern void sub_801395C8(u32 *, s32);           /* DecDCTin */
extern s32 sub_8014510C(s32);                   /* VSync */
extern void sub_80141BA0(DISPENV *);            /* PutDispEnv */
extern void sub_80141430(s32);                  /* SetDispMask */
extern u32 *sub_80144258(s32);                  /* FntFlush */
extern void sub_8013AB4C(void (*)());           /* CdDataCallback */
extern void *sub_8013A6FC(void (*)());          /* CdSyncCallback */

void movieStrKickCd(u8 *loc);
void movieStrCallback(void);
s32 movieStrNextVlc(DECENV *dec, u32 end);
u32 *movieStrNext(DECENV *dec, u32 end);
void movieStrSync(DECENV *dec, s32 mode);

/* movieStrInit: reset the MDEC, install the slice callback, start streaming from loc. */
void __attribute__((section(".text_80138068"))) movieStrInit(u8 *loc, void (*callback)())
{
    sub_80139498(0);
    movieEndReached = 0;
    sub_801396C8(callback);
    sub_8013A540(movieRingBuffer, 32);
    sub_8013DB10(movieRgb24, 1, -1, 0, 0);
    movieStrKickCd(loc);
}

/* movieStrCallback: MDEC-out callback; load the decoded slice to VRAM and advance. */
void __attribute__((section(".text_801380D8"))) movieStrCallback(void)
{
    if (movieRgb24) {
        if (D_801AB888) {
            sub_8013DD78();
            D_801AB888 = 0;
        }
    }
    sub_801416F4(&movieDecEnv.slice, (u32 *)movieDecEnv.imgbuf);
    movieDecEnv.slice.x += movieDecEnv.slice.w;
    if (movieDecEnv.slice.x < movieDecEnv.rect[movieDecEnv.rectid].x + movieDecEnv.rect[movieDecEnv.rectid].w) {
        sub_80139644(movieDecEnv.imgbuf, movieDecEnv.slice.w * movieDecEnv.slice.h / 2);
    } else {
        movieDecEnv.isdone = 1;
        movieDecEnv.rectid = movieDecEnv.rectid ? 0 : 1;
        movieDecEnv.slice.x = movieDecEnv.rect[movieDecEnv.rectid].x;
        movieDecEnv.slice.y = movieDecEnv.rect[movieDecEnv.rectid].y;
    }
}

/* movieStrNextVlc: wait for the next frame and VLC-decode it into the other buffer. */
s32 __attribute__((section(".text_80138204"))) movieStrNextVlc(DECENV *dec, u32 end)
{
    s32 cnt = 0x800000;
    u32 *next;

    while ((next = movieStrNext(dec, end)) == 0) {
        if (--cnt == 0) {
            return -1;
        }
    }
    dec->vlcid = dec->vlcid ? 0 : 1;
    sub_80139B9C(next, dec->vlcbuf[dec->vlcid]);
    sub_8013DB98(next);
    return 0;
}

/* movieStrNext: get the next streamed frame; flags the end and tracks frame size. */
u32 * __attribute__((section(".text_801382A8"))) movieStrNext(DECENV *dec, u32 end)
{
    u32 *addr;
    StHEADER *sector;
    s32 cnt = 0x800000;

    while (sub_8013DC94(&addr, &sector)) {
        if (--cnt == 0) {
            return 0;
        }
    }
    movieFrameNumber = sector->frameCount;
    if (sector->frameCount >= end) {
        movieEndReached = 1;
    }
    if (movieFrameWidth != sector->width || movieFrameHeight != sector->height) {
        movieFrameWidth = sector->width;
        movieFrameHeight = sector->height;
    }
    /* 24-bit frames are 1.5x wider in 16-bit VRAM pixels. */
    if (movieRgb24) {
        dec->rect[0].w = dec->rect[1].w = movieFrameWidth * 3 / 2;
    } else {
        dec->rect[0].w = dec->rect[1].w = movieFrameWidth;
    }
    dec->rect[0].h = dec->rect[1].h = movieFrameHeight;
    dec->slice.h = movieFrameHeight;
    return addr;
}

/* strSync: wait for the frame decode to finish (forces a buffer switch on timeout). */
void __attribute__((section(".text_801383CC"))) movieStrSync(DECENV *dec, s32 mode)
{
    u32 cnt[1];   /* in memory, read through a pointer: -fvolatile reloads it */

    *cnt = 0x800000;
    while (dec->isdone == 0) {
        if (--*cnt == 0) {
            dec->isdone = 1;
            dec->rectid = dec->rectid ? 0 : 1;
            dec->slice.x = dec->rect[dec->rectid].x;
            dec->slice.y = dec->rect[dec->rectid].y;
        }
    }
    dec->isdone = 0;
}

/* movieStrKickCd: seek to loc and start the streaming read. */
void __attribute__((section(".text_80138478"))) movieStrKickCd(u8 *loc)
{
    while (sub_8013A72C(0x15, loc, 0) == 0)
        ;
    while (sub_8013D9B4(0x1C0) == 0)
        ;
}

/* moviePlay: play a .STR movie from CD; returns the callback's result. */
s32 __attribute__((section(".text_801384C4")))
moviePlay(char *name, s32 (*func)(s32, u32, s32), u8 loop, u8 wide, u32 end,
             s32 x, s32 y, s32 x2, s32 y2, s32 x3, s32 y3, DISPENV *disp, u8 *id)
{
    CdlFILE file;
    s32 ret = 0;
    u8 done = 0;
    s32 mode = 2;

    u8 rgb24;
    s32 buf;

    movieRgb24 = wide;
    if (wide) {
        mode = 3;
    }
    if (sub_8013CF8C(&file, name) == 0) {
        return 0;
    }
    movieRingBuffer = fileioWorkBuffer;
    rgb24 = wide;
    sub_80145EF8(0, 0, 1);
    sub_80145F8C(0, 0x7F, 0x7F);
    do {
        movieStartLoc[0] = file.loc[0];
        movieStartLoc[1] = file.loc[1];
        movieStartLoc[2] = file.loc[2];
        movieBuffersInit(&movieDecEnv, *id, x, y, x2, y2, x3, y3);
        movieStrInit(movieStartLoc, movieStrCallback);
        movieStrNextVlc(&movieDecEnv, end);
        for (;;) {
            /* MATCHING: goto (not a while condition) keeps the ret test at the loop top. */
            if (ret != 0) goto stop;

            sub_801395C8(movieDecEnv.vlcbuf[movieDecEnv.vlcid], mode);
            sub_80139644(movieDecEnv.imgbuf, movieDecEnv.slice.w * movieDecEnv.slice.h / 2);
            movieStrNextVlc(&movieDecEnv, end);
            movieStrSync(&movieDecEnv, 0);
            sub_8014510C(0);
            buf = movieDecEnv.rectid ? 0 : 1;
            *id = buf;
            if (rgb24) {
                disp[buf].isrgb24 = movieRgb24;
            }
            sub_80141BA0(&disp[buf]);
            sub_80141430(1);
            if (!rgb24) {
                sub_80144258(-1);
            }
            if (movieEndReached == 1) {
                break;
            }
            if (func) {
                ret = func(movieFrameNumber, end, buf);
            }
        }
    stop:
        if (ret || !movieEndReached || !loop) {
            done = 1;
        }
    } while (!done);
    sub_80145F8C(0, 0, 0);
    sub_801396C8(0);
    sub_8013AB4C(0);
    sub_8013A6FC(0);
    return ret;
}

