/* Far LOD for the PC port (request 30, docs/FAR_LOD.md).
 *
 * Only with -DTM2_PORT_FAR_LOD (the port's `--port-define TM2_PORT_FAR_LOD` on a
 * port variant build); otherwise this header declares nothing and every
 * caller is the retail C.
 *
 * portFarLod (port_variant_arrays.c, .port_bss, config/US/port_variant_arrays.tsv)
 * is the port's switch: 0 = retail; non-zero = a level-of-detail node (model
 * opcode 2) whose distance lies past its farthest record draws that record,
 * unless a farther band of the same set of LOD nodes takes over. The renderer
 * applies it per view (renderScene); hit detection keeps the retail records.
 * The functions are in .port_text (port_variant_arrays.c). */
#ifndef SEMANTIC_PORT_FAR_LOD_H
#define SEMANTIC_PORT_FAR_LOD_H

/* Request 98 (FAR_LOD.md 14): the per-view budget's state (.port_bss, every variant: the layout does not depend on the
 * define; port_variant_arrays.c includes this header for it). */
#define TM2_FAR_LOD_BUDGET_UP 8         /* views of a row after a step up before the next */
#define TM2_FAR_LOD_BUDGET_DOWN 240     /* and after a step down */
#define TM2_FAR_LOD_BUDGET_ROWS 5       /* portRenderStats' rows: views 0..3, the mirror */
struct TM2PortFarLodBudget {            /* 0x10 bytes */
    int current;                        /* +0x00 the scale of the view being walked (0: the budget not in force) */
    unsigned char scale[TM2_FAR_LOD_BUDGET_ROWS];   /* +0x04 per row: its scale now (0: not yet set) */
    unsigned char hold[TM2_FAR_LOD_BUDGET_ROWS];    /* +0x09 per row: views before it may step up */
    unsigned char pad[2];
};
typedef char tm2_port_far_lod_budget_size[(sizeof(struct TM2PortFarLodBudget) == 0x10) ? 1 : -1];

#ifdef TM2_PORT_FAR_LOD
#pragma tm2_absolute portFarLod
extern int portFarLod;
/* Request 46 item 6 (FAR_LOD.md 8): 0 / 1 = retail; 2..16 = while renderScene walks a view every LOD band's
 * bounds are at that many times their distance (the sharp near bands kept that much farther), with or without
 * portFarLod. Read live. */
#pragma tm2_absolute portFarLodNearScale
extern int portFarLodNearScale;
/* Request 98 (FAR_LOD.md 14): the per-view budget. 0 / 1 = off (portFarLodNearScale as set); 2..16 = each view's band
 * scale steps between portFarLodNearScale (the floor) and this value by the view's previous frame
 * (portRenderStats, with TM2_PORT_FAR_DISTANCE): down one step when it used more than 7/8 of the mesh queue or the
 * packet buffer (or reached one), up one when both were under half and TM2_FAR_LOD_BUDGET_UP (after a step up) or
 * TM2_FAR_LOD_BUDGET_DOWN (after a step down) views of that row have passed; the steps 2, 3, 4, 6, 8, 12, 16.
 * 16 draws the finest records out to 16 times their band (Holland: the fences' rails at every distance). Read live;
 * drawing only. */
#pragma tm2_absolute portFarLodBudget
#pragma tm2_absolute portFarLodBudgetState
extern int portFarLodBudget;
extern struct TM2PortFarLodBudget portFarLodBudgetState;
#ifdef TM2_PORT_FAR_DISTANCE
/* renderScene, before portFarLodBegin: this view's scale by the budget (0 = the budget off) */
extern int portFarLodBudgetView(int view);
#endif

/* Called by worldLoadStartObjects once the level's roots are loaded: forgets
 * the previous match's records and scans every level root. */
extern void portFarLodScanLevel(void);
/* Called by assetInstallImage for each relocated car-file root. */
extern void portFarLodScan(void *root);
/* renderScene, around its model walks: extend the records (returns non-zero
 * when it did) and put the retail values back. */
extern int portFarLodBegin(void);
extern void portFarLodEnd(void);
#endif

#endif
