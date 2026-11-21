/* Texture banks for the PC port (request 32 step 4, docs/TEXTURE_BANKS.md "As built", docs/CAR_GROUPS.md 9).
 *
 * With -DTM2_PORT_TEXTURE_BANKS (the port's `--port-define TM2_PORT_TEXTURE_BANKS`; needs
 * TM2_PORT_CAR_GROUPS) and portCarOptions bit 5 set before a load, every car load group gets a VRAM
 * "bank" of its own (1..127; bank 0 is the PS1's VRAM: the arena, fonts, frame buffers, everything not a
 * car). A banked group places all its images and CLUTs in its bank (no reuse by key from other cars, so
 * two cars of one type each have their own copy); an image whose key the arena's TMS holds keeps the
 * arena's coordinates in bank 0, as retail. The slot of each image records its bank in tpage bits 9..15,
 * modelRemapTextures copies those bits into each primitive's tpage halfword, and the port's GPU samples
 * bank (attr >> 9). CPU-side uploads say their bank through portVramUploadBank (set before LoadImage /
 * StoreImage, reset to 0 after the DrawSync). With bit 5 clear (or without the define) all is retail.
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss); this header declares it
 * for the port variant and, with the define, the port functions. */
#ifndef TM2_SEMANTIC_TEX_BANKS_H
#define TM2_SEMANTIC_TEX_BANKS_H

#define TM2_PORT_CAR_OPT_TEXTURE_BANKS 0x20  /* portCarOptions bit 5: banks for the next load (latched by
                                                bitmapInit); bit 4 is request 33-3's (engine sound, car_sound.h) */
#define TM2_PORT_TEX_BANKS 128               /* bank numbers 0..127 (7 bits: tpage bits 9..15) */
#define TM2_PORT_TEX_GROUPS 64               /* load groups the bank tables hold (= TM2_CAR_SLOT_CAP) */
#define TM2_PORT_TEX_BANK_RECTS 72           /* rectangles of the bank being filled: 4 reserved + 32 images
                                                with a CLUT each + slack */

#ifdef TM2_PORT_VARIANT
/* portTexBanks (.port_bss, config/US/port_variant_arrays.tsv group one). Offsets in bytes. */
struct TM2PortTexBanks {
    int live;                                      /* +0x000 portCarOptions bit 5 latched at bitmapInit */
    int level_records;                             /* +0x004 image records of the level TMS (-1 until the
                                                      first car image of the load) */
    int cur_bank;                                  /* +0x008 the bank whose rectangles rects[] holds, 0 none */
    int rect_count;                                /* +0x00C */
    short rects[TM2_PORT_TEX_BANK_RECTS][4];       /* +0x010 x, y, w, h taken in cur_bank */
    unsigned char group_bank[TM2_PORT_TEX_GROUPS]; /* +0x250 bank of each load group, 0 = bank 0 (none) */
    unsigned char bank_group[TM2_PORT_TEX_BANKS];  /* +0x290 load group owning each bank, 0 = free */
    unsigned short body_clut[TM2_PORT_TEX_GROUPS]; /* +0x310 CLUT id (x / 16 | y << 6) of image 0 */
    unsigned char body_bank[TM2_PORT_TEX_GROUPS];  /* +0x390 bank of that CLUT */
    short rec_first[TM2_PORT_TEX_GROUPS];          /* +0x3D0 first image record of the group, -1 none */
    short rec_end[TM2_PORT_TEX_GROUPS];            /* +0x450 its end (records are appended per group) */
};
typedef char tm2_port_tex_banks_size[(sizeof(struct TM2PortTexBanks) == 0x4D0) ? 1 : -1];
#endif

#include "semantic/car_cluts.h"   /* TM2_PORT_TEX_SHARED: banks or request 33-1's CLUT copies */

#ifdef TM2_PORT_TEXTURE_BANKS
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_TEXTURE_BANKS needs TM2_PORT_CAR_GROUPS (fades and banks per load group / car key)"
#endif
#endif

/* The image bookkeeping (record ranges, no-reuse flags, body CLUT, per-key fade id) serves the banks and the
 * per-car CLUT copies of request 33-1 (TM2_PORT_CAR_CLUTS, car_cluts.h): compiled with either define. */
#ifdef TM2_PORT_TEX_SHARED
#pragma tm2_absolute portTexBanks
#pragma tm2_absolute portCarGroupNoReuse
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortTexBanks portTexBanks;
extern unsigned char portCarGroupNoReuse[];      /* per load group, set by the port: never share an image
                                                    by key (mod cars) */
/* .port_text (port_variant_arrays.c) */
extern void portTexBanksReset(void);             /* bitmapInit: latch bit 5, free every bank */
extern int  portTexPlaceBegin(int group);        /* vramPlaceTexture: the group's bank (allocated on its
                                                    first image while live), records its record range */
extern int *portTexFindReuse(int key, int group, int bank); /* the record to share by key, 0 = place */
extern void portTexNoteSlot(int group, int image);   /* publish image 0's CLUT (body) per group */
extern int  portTexFadeId(int slot, int type);   /* the +8 CLUT fade id of a car: key + 8 while banks or
                                                    CLUT copies are live */
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_TEXTURE_BANKS
#pragma tm2_absolute portVramUploadBank
#ifdef __cplusplus
extern "C" {
#endif
extern int portVramUploadBank;                   /* the bank of the CPU upload in progress, 0 = VRAM */
/* .port_text (port_variant_arrays.c) */
extern int  portTexBankOfGroup(int group);       /* the group's bank, 0 = none */
extern int  portTexBankIsFree(int bank, int x, int y, int w, int h);
extern void portTexBankMark(int bank, int x, int y, int w, int h);
extern int  portTexBankAlloc(int group);         /* mid-match API: a fresh bank for a group, 0 = none */
extern void portTexBankFree(int group);          /* mid-match API: give the group's bank back */
extern void portTexLoadImage(int bank, void *rect, void *data);  /* the port's own uploads (team paint) */
#ifdef __cplusplus
}
#endif
/* The bank bits of a slot's tpage word (vramTexSlots[].tpage, bits 9..15) */
#define TM2_TEX_BANK_BITS(bank) ((bank) << 9)
#endif

#endif
