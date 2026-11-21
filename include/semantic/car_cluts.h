/* Per-car CLUTs for the PC port (request 33 step 1, docs/CAR_GROUPS.md 13, docs/PER_CAR_PLAN.md C.1).
 *
 * Retail shares an image by key (vramPlaceTexture -> bitmapFindRecord): the second car of a type takes the first
 * car's pixels and CLUTs, and every car takes the shared part images (undercarriage, shadow, flame, effect
 * frames) from the first car or the arena that holds them. So a team colour painted into a car's body CLUT, or
 * the invisibility fade (a CLUT animation), acts on every car that shares the row.
 *
 * With -DTM2_PORT_CAR_CLUTS (needs TM2_PORT_CAR_GROUPS) and portCarOptions bit 7 set before a load, every CLUT a
 * car takes from another car or from the arena gets a copy of its own; the pixels stay shared. The reuse branch
 * of vramPlaceTexture notes each such image (portCarClutNote); after the last car TMS of the load,
 * sessionLoadArenaAndCars runs the copy pass (portCarClutsCopyAll): each copy is read back from VRAM
 * (gpuStoreImageLine: the row as the arena or the first car uploaded it) and written to a spot of its own
 * (gpuLoadImageLine), and the slot's CLUT x / y moves to it before assetInstallImage remaps the car's DMD. The
 * pass runs after every original is placed, so no retail placement moves or fails because of a copy. Spots:
 * VRAM rows 240..255 first (x 0..639, under the 320x240 frame buffers: SIXTEEN_CARS.md 4.6 option 2, the
 * port's answer PER_CAR_PLAN.md G.1), then the rest of VRAM by retail's CLUT scan order with whole rectangles
 * tested in the usage bitmap. A copy that finds no room leaves the car on the shared row (retail), flagged in
 * portCarCluts.shared. Each car's invisibility fade is then its own CLUT animation (id key + 8, as with banks).
 *
 * Rows 240..255 have one owner, the allocator below (portClutRowsAlloc), also with TM2_PORT_DARK_TOOTH: with
 * portCarOptions bit 6 (Dark Tooth anywhere, SURVIVAL.md 4.1) a CLUT upload that finds no room in the upper
 * half takes a spot there before the scan goes on to the lower half (vramUploadClut). bitmapInit keeps
 * reserving rows 0..255 of x 0..639 in the usage bitmap, so retail's scans never see these rows.
 *
 * With portCarOptions bit 9 (this define, independent of bit 7) vramUploadClut tests every texel of a CLUT spot
 * (retail tests the top-left one, so with 12-14 car types a 256-colour original can land on the font area or car
 * pixels in the lower half: RETAIL_BUGS.md 27), and an upload with no whole free spot in either half takes a spot
 * in rows 240..255 as its last resort (portClutRowsLast). Request 41 item B: with bit 9 vramPlaceTexture's image
 * scan tests the whole w x h rectangle too (portImageSpotFree; retail tests its top-left texel, the same bug), in
 * two passes of retail's order; when neither finds a whole free rectangle, retail's two passes run as before
 * (the image loads where retail would put it rather than failing).
 *
 * With texture banks live (TM2_PORT_TEXTURE_BANKS and bit 5) every car has its own bank, so bit 7 is ignored
 * (no copy; the body CLUT is portTexBanks.body_clut / body_bank in both modes).
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss); this header declares it for the
 * port variant and, with the defines, the port functions. Plain C types only. */
#ifndef TM2_SEMANTIC_CAR_CLUTS_H
#define TM2_SEMANTIC_CAR_CLUTS_H

#define TM2_PORT_CAR_OPT_CAR_CLUTS 0x80      /* portCarOptions bit 7: per-car CLUT copies for the next load
                                                (latched by bitmapInit; ignored while banks are live) */
#define TM2_PORT_CAR_OPT_CLUT_SCAN 0x200     /* portCarOptions bit 9: vramUploadClut tests every texel of a CLUT
                                                spot (retail: the top-left one, RETAIL_BUGS.md 27), and takes
                                                rows 240..255 when neither half has a whole free spot; request 41
                                                item B: vramPlaceTexture tests the whole image rectangle the same
                                                way, retail's scan as the fallback (latched by bitmapInit;
                                                independent of bit 7) */
#define TM2_PORT_CLUT_GROUPS 64              /* load groups the tables hold (= car_groups.h TM2_CAR_SLOT_CAP) */
#define TM2_PORT_CLUT_IMAGES 32              /* images per group (an image index is 0..31) */
#define TM2_PORT_CLUT_ROW_FIRST 240          /* the rows the allocator owns: 240..255, x 0..639 */
#define TM2_PORT_CLUT_ROWS_N 16
#define TM2_PORT_CLUT_ROW_UNITS 40           /* 16-texel units per row (x 0..639) */

/* portCarCluts.state[group][image] */
#define TM2_PORT_CLUT_NONE 0                 /* no CLUT (or the slot belongs to the next group: stride 20) */
#define TM2_PORT_CLUT_OWN 1                  /* uploaded for this group (the original of its type / of the key) */
#define TM2_PORT_CLUT_COPY 2                 /* this group's copy */
#define TM2_PORT_CLUT_SHARED 3               /* still on another group's or the arena's row (no copy, or no room) */
/* portCarCluts.shared[group] */
#define TM2_PORT_CLUT_SHARED_BODY 1          /* the body CLUT (image 0) is also another car's: team paint would
                                                recolour both */
#define TM2_PORT_CLUT_SHARED_PART 2          /* another of its CLUTs is also another car's */
#define TM2_PORT_CLUT_SHARED_ARENA 4         /* one of its CLUTs is still the arena's */
/* portCarCluts.rows_live */
#define TM2_PORT_CLUT_ROWS_COPIES 1          /* bit 7 latched: the copies use rows 240..255 */
#define TM2_PORT_CLUT_ROWS_BOSS 2            /* bit 6 latched (TM2_PORT_DARK_TOOTH): uploads may fall back to them */
#define TM2_PORT_CLUT_ROWS_SCAN 4            /* bit 9 latched (TM2_PORT_CAR_CLUTS): whole-spot CLUT scan; an upload
                                                with no whole free spot in VRAM takes a spot there last */

#ifdef TM2_PORT_VARIANT
/* portCarCluts (.port_bss, config/US/port_variant_arrays.tsv group one). Written by the C; the port reads it
 * after the load. Offsets in bytes. */
struct TM2PortCarCluts {
    int live;                                         /* +0x000 1 = copies this load (bit 7, banks not live) */
    int rows_live;                                    /* +0x004 TM2_PORT_CLUT_ROWS_* as latched by bitmapInit */
    int copies;                                       /* +0x008 copies made this load */
    int failed;                                       /* +0x00C copies that found no room (left shared) */
    unsigned int rows[TM2_PORT_CLUT_ROWS_N][2];       /* +0x010 rows 240..255: used 16-texel units, unit u
                                                         (x 16u..16u+15) = bit u & 31 of word u >> 5 */
    unsigned int pend16[TM2_PORT_CLUT_GROUPS];        /* +0x090 per group: images whose 16-colour CLUT is to be
                                                         copied (bit = image) */
    unsigned int pend256[TM2_PORT_CLUT_GROUPS];       /* +0x190 ... 256-colour CLUT */
    unsigned int arena[TM2_PORT_CLUT_GROUPS];         /* +0x290 ... taken from the arena's TMS */
    unsigned short clut[TM2_PORT_CLUT_GROUPS][TM2_PORT_CLUT_IMAGES];
                                                      /* +0x390 CLUT id (x / 16 | y << 6) of each image after the
                                                         load, 0 = none */
    unsigned char state[TM2_PORT_CLUT_GROUPS][TM2_PORT_CLUT_IMAGES];
                                                      /* +0x1390 TM2_PORT_CLUT_NONE / OWN / COPY / SHARED */
    unsigned char shared[TM2_PORT_CLUT_GROUPS];       /* +0x1B90 TM2_PORT_CLUT_SHARED_* per group */
};
typedef char tm2_port_car_cluts_size[(sizeof(struct TM2PortCarCluts) == 0x1BD0) ? 1 : -1];
#endif

/* The pieces several defines share (internal names, not port defines):
 *   TM2_PORT_TEX_SHARED  the image bookkeeping of request 32 step 4 (portTexPlaceBegin, portTexFindReuse with
 *                        portCarGroupNoReuse, portTexNoteSlot's body CLUT, portTexFadeId): banks or CLUT copies
 *   TM2_PORT_CLUT_ROWS   the rows 240..255 allocator: CLUT copies or Dark Tooth's bit 6 */
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_CAR_CLUTS)
#define TM2_PORT_TEX_SHARED 1
#endif
#if defined(TM2_PORT_DARK_TOOTH) || defined(TM2_PORT_CAR_CLUTS)
#define TM2_PORT_CLUT_ROWS 1
#endif

#ifdef TM2_PORT_CAR_CLUTS
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_CAR_CLUTS needs TM2_PORT_CAR_GROUPS (load groups, car keys and the per-key fade)"
#endif
#endif

#ifdef TM2_PORT_CLUT_ROWS
#pragma tm2_absolute portCarCluts
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortCarCluts portCarCluts;
/* .port_text (port_variant_arrays.c) */
extern void portCarClutsReset(void);             /* bitmapInit: latch bits 6 / 7 for this load, clear the tables */
extern int  portClutRowsAlloc(int w, int *x, int *y);  /* a 16- or 256-texel spot in rows 240..255, 0 = full */
#ifdef TM2_PORT_DARK_TOOTH
extern int  portClutRowsUpload(int w, int h, void *data, int slot, int bank);
                                                 /* vramUploadClut with bit 6, after the upper half: a spot in
                                                    rows 240..255, the slot's CLUT x / y, LoadImage; 0 = none */
#endif
#ifdef TM2_PORT_CAR_CLUTS
extern void portCarClutNote(int group, int image, int mode, int *known);
                                                 /* vramPlaceTexture, after writing slot (group, image): note its
                                                    CLUT, and a CLUT taken from another car or the arena */
extern void portCarClutsCopyAll(void);           /* sessionLoadArenaAndCars, after the car TMS files: the copy
                                                    pass (bodies first, then the other car CLUTs, then the
                                                    arena's), then portCarCluts.shared */
extern int  portCarClutsCopyGroup(int group);    /* the same for one group (a mid-match load); returns the
                                                    copies made */
extern int  portClutSpotFree(int x, int y, int w);  /* vramUploadClut's spot test: bit 9 latched, every texel of
                                                    the w x 1 spot free in the usage bitmap; else retail's
                                                    bitmapIsFreeXY(x, y) */
extern int  portClutRowsLast(int w, int h, void *data, int slot, int bank);
                                                 /* vramUploadClut with bit 9, after both halves: a spot in rows
                                                    240..255, the slot's CLUT x / y, LoadImage; 0 = none */
extern int  portImagePasses(void);               /* vramPlaceTexture's scan passes: 2 (retail), 4 with bit 9 (two
                                                    whole-rectangle passes, then retail's two) */
extern int  portImageSpotFree(int x, int y, int w, int h, int pass);
                                                 /* vramPlaceTexture's spot test (pass 1..4): bit 9 latched and
                                                    pass <= 2, every texel of the w x h rectangle free and inside
                                                    VRAM; else retail's bitmapIsFreeXY(x, y) */
#endif
#ifdef __cplusplus
}
#endif
#endif

#endif
