/* Library code 8014C000..8014E000 (Sony PsyQ SDK and SingleTrac engine), reconstructed C.
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md.
 *
 * Everything here is SingleTrac engine code (debris renderer, file I/O over
 * libcd, VRAM upload helpers) plus libcd's three event callbacks, all written
 * from the retail disassembly (no outside source). Compile recipe: PsyQ 3.6
 * cc1, -fvolatile, extern gp, -G8, compiler delay slots (the libcd callbacks
 * are identical under it too). */

typedef signed char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;

typedef struct {
    s16 x, y, w, h;
} TM2Rect;

/* ISO9660 directory entry used by the file layer (CdlFILE). */
typedef struct {
    u8 pos[4];
    u32 size;
    char name[16];
} TM2CdFile;

/* Model header: +0 flag, +4 vertices (8 bytes each), +8 normals,
 * +12 first primitive, +16 primitive count. */
typedef struct {
    u8 flag;
    u8 pad[3];
    s16 *verts;
    s16 *normals;
    u8 *prims;
    u32 count;
} TM2Model;

/* Model primitive: +1 vertex count, +2 size in words, +4 vertex indices,
 * +13..15 flags, +16 colours / texture data. */
typedef struct {
    u8 flags0;
    u8 nverts;
    u8 size;
    u8 pad3;
    s16 v[4];
    u8 flags12, flags13, flags14, flags15;
    u32 words[4];
} TM2Prim;

/* Light source record of the lighting block at *lightCurrentSet (+0xE8). */
typedef struct {
    s32 color;
    s32 pos[3];
    s32 range;
} TM2Light;

typedef struct {
    s32 unk0;
    s32 count;
    u8 pad[0xE0];
    TM2Light lights[16];
} TM2LightSet;

typedef struct {
    s32 color;
    s32 pos[3];
    s32 range;
    s32 unk14;
    s32 mat;
    s32 unk1c;
} TM2LightSrc;

extern TM2LightSet *lightCurrentSet;
extern TM2CdFile fileioCdFile;
extern char strCdFilePathFmt[];
extern char strFileNameFmt[];
extern s32 fileioWorkBuffer;
extern s32 fileioWorkBuffer2;

extern s32 sprintf(char *, const char *, ...);
extern s32 sub_8013A72C(u8 com, void *param, u8 *result);   /* CdControl */
extern s32 sub_8013A9B0(u8 com, void *param, u8 *result);   /* CdControlB */
extern s32 sub_8013A6A4(s32 mode, u8 *result);              /* CdSync */
extern s32 sub_8013A61C(s32 level);                         /* CdSetDebug */
extern void *sub_8013CF8C(TM2CdFile *fp, char *name);       /* CdSearchFile */
extern s32 sub_8013ABB0(s32 sectors, void *buf, s32 mode);  /* CdRead */
extern s32 sub_8013AB90(s32 mode, u8 *result);              /* CdReadSync */
extern void *sub_8013AC30(s32 i, void *p);                  /* CdIntToPos */
extern s32 sub_8013AD34(void *p);                           /* CdPosToInt */
extern s32 sub_8014DB74(void);                              /* CdInit */
extern s32 sub_8014510C(s32 mode);                          /* VSync */
extern s32 sub_801416F4(TM2Rect *rect, void *p);            /* LoadImage */
extern s32 sub_80141758(TM2Rect *rect, void *p);            /* StoreImage */
extern s32 sub_801414CC(s32 mode);                          /* DrawSync */
extern u16 sub_80144A40(s32 x, s32 y);                      /* GetClut */
extern void DeliverEvent(u32 event, u32 spec);
#include "semantic/world.h"
#include "semantic/port_far_distance.h"
extern void grutilsUvBounds(u8 *points, u16 *x, u16 *y, u16 *w, u16 *h);

s32 fileioWaitCdSync(void);
s32 fileioClose(s32 fd);
s32 sub_8014DB34(void);

/* @8014D368: compress a depth value: linear below 1024, then 1/4, 1/8
 * and 1/32 slopes. */
void __attribute__((section(".text_8014D368"))) renderCompressDepth(s32 *z)
{
    if (*z >= 1024) {
        if (*z < 4096)
            *z = ((*z - 1024) >> 2) + 1024;
        else if (*z < 16384)
            *z = ((*z - 4096) >> 3) + 1792;
        else
            *z = ((*z - 16384) >> 5) + 3328;
    }
}

/* @8014D3F4: add up to 16 light sources (32-byte records) to the light
 * set, transforming each position by matrix m[record +24]. */
void __attribute__((section(".text_8014D3F4"))) lightAddSources(TM2LightSrc *src, s32 n, u8 *mats, u8 *objects /* unused; the retail caller passes it */)
{
    s32 i;

    for (i = 0; i < n; i++) {
        s32 color = src[i].color;
        if (i < 16) {
            TM2LightSet *set = lightCurrentSet;
            set->count++;
            set->lights[i].color = color;
            set->lights[i].range = src[i].range;
            mathMulMatVec((s16 *)(mats + src[i].mat * 32), src[i].pos, set->lights[i].pos);
        }
    }
}

/* @8014D4CC: set each primitive's quadrant bits (byte 13, bits 5..7) from
 * the sign of its first vertex x/y; primitives 2, 4 and 6 use the reverse
 * order. */
void __attribute__((section(".text_8014D4CC"))) meshSetPrimQuadrants(TM2Model *m)
{
    s16 v[2];
    s32 q0 = 0, q1 = 1, q2 = 2, q3 = 3;
    s16 *verts = m->verts;
    u8 *p = m->prims;
    u32 i;

    for (i = 0; i < m->count; i++) {
        v[0] = verts[*(s16 *)(p + 4) * 4];
        v[1] = verts[*(s16 *)(p + 4) * 4 + 1];
        if (i == 2 || i == 4 || i == 6) {
            q0 = 3;
            q1 = 2;
            q2 = 1;
            q3 = 0;
        }
        if (v[0] >= 0) {
            if (v[1] >= 0)
                p[13] = (p[13] & 0x1f) | (q0 << 5);
            else
                p[13] = (p[13] & 0x1f) | (q1 << 5);
        } else {
            if (v[1] >= 0)
                p[13] = (p[13] & 0x1f) | (q2 << 5);
            else
                p[13] = (p[13] & 0x1f) | (q3 << 5);
        }
        q0 = 0;
        q1 = 1;
        q2 = 2;
        q3 = 3;
        p += p[2] * 4;
    }
}

/* @8014D700 fileioOpenFile: find "\NAME;1" on the disc (up to 300 tries,
 * one per frame), store its size and seek to it. Returns 1, or 0 with
 * *size = 0 when the file is missing. */
s32 __attribute__((section(".text_8014D700"))) fileioOpenFile(char *name, s32 *size)
{
    char path[48];
    u8 param[8];
    u8 failed = 1;
    u8 done = 0;
    s32 tries = 0;

    sprintf(path, strCdFilePathFmt, name);
    fileioWaitCdSync();
    param[0] = 0x80;
    while (!sub_8013A72C(0xE, param, 0))
        ;
    while (!done) {
        if (sub_8013CF8C(&fileioCdFile, path)) {
            done = 1;
            failed = 0;
        } else if (tries >= 300) {
            done = 1;
        } else {
            tries++;
            sub_8014510C(0);
        }
    }
    if (failed) {
        *size = 0;
        return 0;
    }
    *size = fileioCdFile.size;
    do
        fileioWaitCdSync();
    while (!sub_8013A72C(2, &fileioCdFile, 0));
    return 1;
}

/* @8014D820: wrapper for fileioOpenFile. */
s32 __attribute__((section(".text_8014D820"))) fileioOpen(char *name, s32 *size)
{
    return fileioOpenFile(name, size);
}

/* @8014D840: seek (blocking) to byte offset off inside the open file;
 * whence must be 0. */
s32 __attribute__((section(".text_8014D840"))) fileioSeek(s32 fd, s32 off, s32 whence)
{
    u8 loc[4];

    if (whence)
        return -1;
    sub_8013AC30(sub_8013AD34(&fileioCdFile) + off / 2048, loc);
    do
        fileioWaitCdSync();
    while (!sub_8013A9B0(0x15, loc, 0));
    return 1;
}

/* @8014D8C0: look a file up and seek to it (CdlSeekL). */
s32 __attribute__((section(".text_8014D8C0"))) fileioSeekToFile(char *name)
{
    sub_8013CF8C(&fileioCdFile, name);
    do
        fileioWaitCdSync();
    while (!sub_8013A72C(0x15, &fileioCdFile, 0));
    return 0;
}

/* @8014D910: read size bytes of the open file into buf, re-seeking and
 * retrying until the drive reports success; returns size. */
s32 __attribute__((section(".text_8014D910"))) fileioReadFile(s32 fd, void *buf, s32 size)
{
    u8 result[8];
    s32 sectors = (size + 2047) / 2048;
    s32 done = 0;
    s32 ret;

    for (;;) {
        if (done)
            goto out;
        sub_8013ABB0(sectors, buf, 0x81);
        sub_8013AB90(0, result);
        if (result[0] == 0x22) {
            ret = size;
            done = 1;
        } else {
            do
                fileioWaitCdSync();
            while (!sub_8013A72C(2, &fileioCdFile, 0));
            ret = 0;
        }
    }
out:
    return ret;
}

/* @8014D9D4 fileioReadFileAsync: start reading size bytes into buf. */
s32 __attribute__((section(".text_8014D9D4"))) fileioReadFileAsync(s32 fd, void *buf, s32 size)
{
    return sub_8013ABB0((size + 2047) / 2048, buf, 0x81);
}

/* @8014DA04 fileioAsyncReadCompleted: 1 when the async read is done. */
s32 __attribute__((section(".text_8014DA04"))) fileioAsyncReadCompleted(void)
{
    u8 result[8];

    return sub_8013AB90(1, result) == 0;
}

/* @8014DA28: close (nothing to do). */
s32 __attribute__((section(".text_8014DA28"))) fileioClose(s32 fd)
{
    return 0;
}

/* @8014DA30 fileioLoadFileIntoRam: open, read whole file into buf, close.
 * Returns the size read, or 0. */
s32 __attribute__((section(".text_8014DA30"))) fileioLoadFileIntoRam(void *buf, char *name)
{
    s32 size;
    s32 fd = fileioOpenFile(name, &size);
    s32 ret;

    if (fd == -1)
        return 0;
    ret = fileioReadFile(fd, buf, size);
    fileioClose(fd);
    return ret;
}

/* @8014DA9C */
void __attribute__((section(".text_8014DA9C"))) sub_8014DA9C(void)
{
}

/* @8014DAA4: file layer init: remember the two arguments, CdInit,
 * CdSetDebug(0). */
void __attribute__((section(".text_8014DAA4"))) fileioInit(s32 a, s32 b)
{
    fileioWorkBuffer = a;
    fileioWorkBuffer2 = b;
    sub_8014DB74();
    sub_8013A61C(0);
}

/* @8014DAD4 */
s32 __attribute__((section(".text_8014DAD4"))) sub_8014DAD4(void)
{
    return 1;
}

/* @8014DADC: format a file name into buf. */
char *__attribute__((section(".text_8014DADC"))) fileioFormatName(char *buf, char *name)
{
    sprintf(buf, strFileNameFmt, name);
    return buf;
}

/* @8014DB14 */
s32 __attribute__((section(".text_8014DB14"))) sub_8014DB14(void)
{
    return sub_8014DB34();
}

/* @8014DB34 */
s32 __attribute__((section(".text_8014DB34"))) sub_8014DB34(void)
{
    return 0x44;
}

/* @8014DB3C: wait for the CD command to complete (CdSync(1) == CdlComplete). */
s32 __attribute__((section(".text_8014DB3C"))) fileioWaitCdSync(void)
{
    u8 result[8];

    while (sub_8013A6A4(1, result) != 2)
        ;
}

/* @8014DC04 libcd def_cbsync */
void __attribute__((section(".text_8014DC04"))) sub_8014DC04(void)
{
    DeliverEvent(0xF0000003, 0x20);
}

/* @8014DC2C libcd def_cbready */
void __attribute__((section(".text_8014DC2C"))) sub_8014DC2C(void)
{
    DeliverEvent(0xF0000003, 0x40);
}

/* @8014DC54 libcd def_cbread */
void __attribute__((section(".text_8014DC54"))) sub_8014DC54(void)
{
    DeliverEvent(0xF0000003, 0x40);
}

/* @8014DC7C Load4BitClut: upload a 16-colour CLUT to (x, y). */
u16 __attribute__((section(".text_8014DC7C"))) Load4BitClut(void *clut, s32 x, s32 y)
{
    TM2Rect r;

    r.x = x;
    r.y = y;
    r.w = 16;
    r.h = 1;
    sub_801416F4(&r, clut);
    sub_801414CC(0);
    return sub_80144A40(x, y);
}

#ifdef TM2_PORT_TEXTURE_BANKS
#include "semantic/tex_banks.h"   /* request 32 step 4 */
#endif

/* @8014DCEC: upload one VRAM line of w pixels. */
void __attribute__((section(".text_8014DCEC"))) gpuLoadImageLine(void *p, s32 x, s32 y, s32 w)
{
    TM2Rect r;

#ifdef TM2_PORT_TEXTURE_BANKS
    /* request 32 step 4 (TEXTURE_BANKS.md "As built"): a CLUT animation of a banked car passes its bank in
       y bits 9..15 (clutAnimRegister); the port's GPU takes it from portVramUploadBank */
    portVramUploadBank = (y >> 9) & 0x7F;
    y &= 0x1FF;
#endif
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = 1;
    sub_801416F4(&r, p);
    sub_801414CC(0);
#ifdef TM2_PORT_TEXTURE_BANKS
    portVramUploadBank = 0;
#endif
}

/* @8014DD30: read back one VRAM line of w pixels. */
void __attribute__((section(".text_8014DD30"))) gpuStoreImageLine(void *p, s32 x, s32 y, s32 w)
{
    TM2Rect r;

#ifdef TM2_PORT_TEXTURE_BANKS
    /* request 32 step 4: as gpuLoadImageLine (the read comes from the bank) */
    portVramUploadBank = (y >> 9) & 0x7F;
    y &= 0x1FF;
#endif
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = 1;
    sub_80141758(&r, p);
    sub_801414CC(0);
#ifdef TM2_PORT_TEXTURE_BANKS
    portVramUploadBank = 0;
#endif
}

/* @8014DE98: build up to n 16-byte sprite records from a model's
 * primitives: texture rect via grutilsUvBounds, position = first vertex / 8
 * (y negated). Returns the number written. */
s32 __attribute__((section(".text_8014DE98"))) grutilsParseSpritesWithPos(TM2Model *m, u16 *out, s32 n)
{
    s16 *verts;
    TM2Prim *p;
    u8 *tex;
    s32 off;
    s32 i;

    if (m == 0 || m->flag != 0)
        return 0;
    verts = m->verts;
    p = (TM2Prim *)m->prims;
    n = ((u32)n < m->count) ? n : m->count;
    for (i = 0; i < n; i++) {
        u8 *data = (u8 *)p + 16;
        tex = data + ((p->flags15 >> 2) & 0x1c);
        out[6] = verts[p->v[0] * 4] / 8;
        out[7] = -verts[p->v[0] * 4 + 1] / 8;
        grutilsUvBounds(tex, out, out + 1, out + 2, out + 3);
        out[5] = *(u16 *)(tex + 2);
        out[4] = *(u16 *)(tex + 6);
        p = (TM2Prim *)((u8 *)p + p->size * 4);
        out += 8;
    }
    return n;
}

/* Draw context passed by the main loop's debrisDraw: ordering table,
 * packet cursor and limit, and the texture window/page for the
 * DR_MODE packets built in the scratchpad. */
typedef struct {
    u32 *ot;
    s32 unk4;
    u32 *pkt;
    u32 *pktEnd;
    u8 pad10[12];
    TM2Rect tw;
    u16 tpage;
    u8 dtd;
    u8 dfe;
} TM2DrawCtx;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} TM2Matrix;

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVector;

typedef struct {
    s32 vx, vy, vz, pad;
} TM2Vector;

/* One debris chunk: model, position and depth shift. */
typedef struct {
    TM2Model *model;
    s32 x, y, z;
    s32 shift;
} TM2DebrisPiece;

typedef struct {
    s32 unk0;
    s32 count;
    u8 pad8[60];
    s32 off[4][3];
    u8 pad74[84];
    TM2Matrix mat;
    TM2DebrisPiece pieces[1];
} TM2Debris;

typedef struct {
    s32 back[3];
    s16 color[16];
    s16 light[16];
} TM2Light2;

typedef struct {
    s32 unk0;
    TM2Light2 *light;
    s32 pad8[3];
    s32 t[3];
    TM2Matrix rot;
} TM2View;

extern void sub_8014215C(void *p, s32 dfe, s32 dtd, s32 tpage, TM2Rect *tw);  /* SetDrawMode */
extern void *sub_80140984(TM2Matrix *m0, TM2Matrix *m1);                      /* TransposeMatrix */
extern void sub_80140410(TM2Matrix *m);                                      /* SetRotMatrix */
extern void *sub_80149970(TM2Matrix *m0, TM2Matrix *m1);                      /* MulRotMatrix0 */
extern void sub_80140440(void *m);                                           /* SetLightMatrix */
extern void sub_80140470(void *m);                                           /* SetColorMatrix */
extern void sub_8014A1A8(s32 r, s32 g, s32 b);                               /* SetBackColor */
extern void sub_801404A0(TM2Matrix *m);                                      /* SetTransMatrix */
extern s32 sub_801404F0(TM2SVector *v0, TM2SVector *v1, TM2SVector *v2, u32 *sxy0,
                        u32 *sxy1, u32 *sxy2, s32 *p, s32 *flag);            /* RotTransPers3 */
extern s32 sub_8014D680(TM2SVector *v0, TM2SVector *v1, TM2SVector *v2, TM2SVector *v3,
                        u32 *sxy0, u32 *sxy1, u32 *sxy2, u32 *sxy3, s32 *p, s32 *flag); /* RotTransPers4 */
extern s32 sub_801404C4(TM2SVector *v0, s32 *sxy, s32 *p, s32 *flag);       /* RotTransPers */
extern void sub_8014D600(s32 *a, s32 *b, s32 *c);
extern void sub_8014D614(s32 *a, s32 *b, s32 *c, s32 *d);
extern s32 viewGetProjH(s32 i);
extern void sub_80140670(TM2SVector *v0, void *v1, void *v2);               /* NormalColorCol */
extern void sub_80144BA0(void *ot, void *p);                                 /* AddPrim */

/* Scratchpad: two DR_MODE packets built each frame, and a flag word. */
extern u32 renderScrDrMode[];
extern u32 renderScrDrModeDither[];
extern s32 D_1F800088;
#pragma tm2_absolute D_1F800088
#define SCR_DRMODE0 renderScrDrMode
#define SCR_DRMODE1 renderScrDrModeDither

#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define VTX(n, c) (verts[*(s16 *)(prim + 4 + (n) * 2) * 4 + (c)] + tr[prim[13] >> 5][c])

/* @8014C19C: draw the debris chunks (called from debrisDraw in the main
 * loop). Each chunk's model is offset by four rotated corner offsets
 * (chosen per primitive by byte 13 >> 5), transformed with the GTE,
 * depth-sorted (renderCompressDepth), lit with NormalColorCol and linked into the
 * ordering table, optionally wrapped in the two scratchpad DR_MODE
 * packets. Stops when the packet buffer limit is reached. */
void __attribute__((section(".text_8014C19C"))) fxDebrisDraw(TM2DrawCtx *ctx, TM2Debris *obj)
{
    TM2SVector v[4];
    TM2Vector pos;
    TM2Matrix m;
    TM2Matrix m2;
    s32 tr[4][3];
    s32 p;
    s32 flag;
    s32 z0, z1, z2;
    s32 otz;
    s32 z3;
    s32 sxy;
    s16 *normals;
    u32 *pkt;
    s32 stride;
    s32 uv;
    u32 j;
    s32 i;
    TM2Model *model;
    u32 *tex;
    TM2View *view;
    s32 shift;
    s16 *verts;
    u8 *prim;
    s32 k;
    s32 zmax;
    s32 t;
    s32 c;
    u32 zlim;

    pkt = ctx->pkt;
    otz = 1;
    sub_8014215C((void *)0x1F800060, ctx->dfe, ctx->dtd, ctx->tpage, &ctx->tw);
    sub_8014215C((void *)0x1F80006C, ctx->dfe, 1, ctx->tpage, &ctx->tw);
    D_1F800088 = 0;
    view = csGetRoot();
    sub_80140984(&obj->mat, &m);
    sub_80140410(&view->rot);
    sub_80149970(&m, &m2);
    sub_80140410(&m2);
    sub_80140440(view->light->light);
    sub_80140470(view->light->color);
    uv = 0;
    sub_8014A1A8(view->light->back[0], view->light->back[1], view->light->back[2]);
    do
        ;
    while (0);  /* keeps the uv store ahead of SetBackColor */
    zlim = 4095;
    m.t[0] = 0;
    m.t[1] = 0;
    m.t[2] = 0;
    sub_801404A0(&m);
    tex = 0;
    for (i = 0; i < obj->count; i++) {
        pos.vx = obj->pieces[i].x + view->t[0];
        pos.vy = obj->pieces[i].y + view->t[1];
        pos.vz = obj->pieces[i].z + view->t[2];
        for (c = 0; c < 4; c++) {
            pos.vx += obj->off[c][0];
            pos.vy += obj->off[c][1];
            pos.vz += obj->off[c][2];
            mathMulMatVec((s16 *)&obj->mat, &pos.vx, tr[c]);
        }
#ifdef TM2_PORT_FAR_DISTANCE
        /* PC port request 31 (FAR_DISTANCE.md): in far mode a piece whose 16-bit vertices
         * would wrap (past about 4,000 world units) is skipped, not drawn across the screen. */
        if (!portFarDistanceDebrisFits(&tr[0][0]))
            continue;
#endif
        model = obj->pieces[i].model;
        shift = obj->pieces[i].shift;
        verts = model->verts;
        normals = model->normals;
        prim = model->prims;
        for (j = 0; j < model->count; j++) {
            u32 *sx;

            if (ctx->pktEnd < pkt)
                goto done;
            if (obj->off[prim[13] >> 5][2] + obj->pieces[i].z < 0)
                goto next;
            stride = prim[14] >> 6;
            t = prim[14] & 7;
            sx = pkt + 2;
            if (t == 3) {
                v[0].vx = VTX(0, 0);
                v[0].vy = VTX(0, 1);
                v[0].vz = VTX(0, 2);
                v[1].vx = VTX(1, 0);
                v[1].vy = VTX(1, 1);
                v[1].vz = VTX(1, 2);
                v[2].vx = VTX(2, 0);
                v[2].vy = VTX(2, 1);
                v[2].vz = VTX(2, 2);
                sub_801404F0(&v[0], &v[1], &v[2], sx, sx + stride, sx + stride * 2, &p, &flag);
                sub_8014D600(&z0, &z1, &z2);
                otz = MAX(MAX(z0, z1), z2);
                otz = otz >> shift;
                renderCompressDepth(&otz);
            } else if (t == 4) {
                v[0].vx = VTX(0, 0);
                v[0].vy = VTX(0, 1);
                v[0].vz = VTX(0, 2);
                v[1].vx = VTX(1, 0);
                v[1].vy = VTX(1, 1);
                v[1].vz = VTX(1, 2);
                v[2].vx = VTX(2, 0);
                v[2].vy = VTX(2, 1);
                v[2].vz = VTX(2, 2);
                v[3].vx = VTX(3, 0);
                v[3].vy = VTX(3, 1);
                v[3].vz = VTX(3, 2);
                sub_8014D680(&v[0], &v[1], &v[2], &v[3], sx, sx + stride, sx + stride * 2,
                             sx + stride * 3, &p, &flag);
                sub_8014D614(&z0, &z1, &z2, &z3);
                zmax = MAX(MAX(MAX(z0, z1), z2), z3);
                otz = zmax >> shift;
                renderCompressDepth(&otz);
            } else if (t < 2) {
                v[0].vx = VTX(0, 0);
                v[0].vy = VTX(0, 1);
                v[0].vz = VTX(0, 2);
                sub_801404C4(&v[0], &sxy, &p, &flag);
                sub_8014D614(&z0, &z1, &z2, &z3);
                otz = z3 >> shift;
                renderCompressDepth(&otz);
                if ((u32)(otz - 1) < zlim) {
                    s32 w = *(s16 *)(prim + 6) * viewGetProjH(0) / z3;
                    s32 h = *(s16 *)(prim + 8) * viewGetProjH(0) / z3;

                    if (w <= 0)
                        w = 1;
                    if (h <= 0)
                        h = 1;
                    if (w > 500)
                        w = 500;
                    if (h > 500)
                        h = 500;
                    pkt[2] = sxy;
                    ((s16 *)pkt)[4] -= w;
                    ((s16 *)pkt)[5] -= h;
                    w *= 2;
                    h *= 2;
                    ((s16 *)(sx + stride))[0] = ((s16 *)pkt)[4] + w;
                    ((s16 *)(sx + stride))[1] = ((s16 *)pkt)[5];
                    ((s16 *)(sx + stride * 2))[0] = ((s16 *)pkt)[4];
                    ((s16 *)(sx + stride * 2))[1] = ((s16 *)pkt)[5] + h;
                    ((s16 *)(sx + stride * 3))[0] = ((s16 *)pkt)[4] + w;
                    ((s16 *)(sx + stride * 3))[1] = ((s16 *)pkt)[5] + h;
                }
            }
            if ((u32)(otz - 1) < zlim) {
                u8 *data;
                u8 *src;
                u32 *col;
                s32 ncol;

                t = prim[1];
                ncol = (prim[14] >> 3) & 7;
                src = data = prim + 16;
                col = pkt + 1;
                if (ncol != 0) {
                    s16 *nidx = (s16 *)(data + (prim[15] & 0xf) * 4);
                    s32 n = ncol;
                    s32 nstep = n != 1;
                    s32 vstep = t != 1;

                    if (n < t)
                        n = t;
                    for (k = 0; k < n; k++) {
                        sub_80140670((TM2SVector *)(normals + *nidx * 4), src, col);
                        nidx += nstep;
                        src += vstep * 4;
                        col += stride;
                    }
                } else {
                    pkt[1] = *(u32 *)(prim + 16);
                    if (t >= 2) {
                        col += stride;
                        *col = *(u32 *)(prim + 20);
                        col += stride;
                        *col = *(u32 *)(prim + 24);
                        if (t == 4) {
                            col += stride;
                            *col = *(u32 *)(prim + 28);
                        }
                    }
                }
                t = prim[0] & 0x7f;
                if (t != 0) {
                    u32 *d;

                    tex = (u32 *)(data + ((prim[15] >> 2) & 0x1c));
                    d = pkt + 3;
                    *d = *tex++;
                    uv = ((s16 *)tex)[1];
                    d += stride;
                    *d = *tex++;
                    d += stride;
                    *d = *tex;
                    if (t == 4) {
                        tex++;
                        d += stride;
                        *d = *tex;
                    }
                    tex++;
                }
                if (*(u32 *)((u8 *)model + 24) & 0x7ff80000)
                    otz = (*(u16 *)((u8 *)model + 26) >> 3) & 0xfff;
                if ((*(u32 *)prim & 0x80) || (*(u32 *)((u8 *)model + 24) & 0x20000)) {
                    u32 *q = pkt + prim[3];

                    q[1] = SCR_DRMODE0[0];
                    q[2] = SCR_DRMODE0[1];
                    q[3] = SCR_DRMODE0[2];
                    sub_80144BA0(ctx->ot + otz, q + 1);
                    {
                        u32 *lp = pkt;

                        pkt = q + 4;
                        ((u8 *)lp)[3] = prim[3];
                        sub_80144BA0(ctx->ot + otz, lp);
                    }
                    if (*(u32 *)((u8 *)model + 24) & 0x20000) {
                        q[4] = SCR_DRMODE1[0];
                        q[5] = SCR_DRMODE1[1];
                        q[6] = SCR_DRMODE1[2];
                    } else {
                        q[4] = SCR_DRMODE0[0];
                        q[5] = SCR_DRMODE0[1];
                        q[6] = SCR_DRMODE0[2];
                    }
                    if (*(u32 *)prim & 0x80) {
                        pkt[2] = *tex;
                        ((u8 *)pkt)[4] = uv;
                    }
                    sub_80144BA0(ctx->ot + otz, pkt);
                    pkt += 3;
                } else {
                    ((u8 *)pkt)[3] = prim[3];
                    sub_80144BA0(ctx->ot + otz, pkt);
                    pkt = pkt + prim[3] + 1;
                }
            }
        next:
            prim += prim[2] * 4;
        }
    }
done:
    ctx->pkt = pkt;
}
