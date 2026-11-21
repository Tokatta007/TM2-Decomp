/* Render capacity counter for the PC port (docs/RENDER_ANSWERS.md "Measuring the render caps").
 *
 * Read-only observation of how close each view gets to the renderer's per-view limits, so the port can decide
 * whether to raise them: the matrix slots (200 per view, 198 for the walker's transform nodes, `slti 198` at
 * 80152C18; TM2_PORT_RENDER_SLOTS's limit and limit - 2, render_slots.h, request 47), the mesh queue (280 or
 * portMeshListCap) and the frame's packet buffer (97,520 bytes or portPacketBufferBytes). With -DTM2_PORT_FAR_DISTANCE (the define that owns the mesh and packet caps)
 * renderScene records each view twice: portRenderStatsWalked after the walks (the slot counter ctx+0x08 and the
 * mesh count, before the emitter, which saves a register over ctx+0x08 and reuses ctx+0x3C), portRenderStatsDrawn
 * after the emitter and the near pass (the frame record's packet cursor). Nothing else reads the block: no game
 * state, no rand(), no change to what is drawn. Leave it out of the rollback snapshot and the state hash.
 *
 * The block is defined in every variant (port_variant_arrays.c, .port_bss, after portVisualState), so the
 * variant's layout does not depend on the define; without it nothing writes it (all zero). */
#ifndef TM2_SEMANTIC_PORT_RENDER_STATS_H
#define TM2_SEMANTIC_PORT_RENDER_STATS_H

#define TM2_RENDER_STATS_VIEWS  4       /* rows 0..3: renderScene's view argument 0..3 (mode 4 excepted) */
#define TM2_RENDER_STATS_MIRROR 4       /* row 4: any view drawn in view mode 4, the rear-view mirror */
#define TM2_RENDER_STATS_ROWS   5

/* Cap bits (caps, caps_seen) */
#define TM2_RENDER_CAP_NODES    0x1     /* slots >= 198 (the limit - 2): a transform node (walker opcode 5) would get no
                                           slot of its own */
#define TM2_RENDER_CAP_SLOTS    0x2     /* slots == 200 (the limit): later objects share the last slot (retail's slot 200
                                           is one past the arrays; TM2_PORT_RENDER_SLOTS's is inside them) */
#define TM2_RENDER_CAP_MESHES   0x4     /* meshes == mesh_cap: the queue is full, any later mesh of the view dropped */
#define TM2_RENDER_CAP_PACKETS  0x8     /* packet > packet_cap - 80: the emitter stopped or the next primitive would */
#include "semantic/render_slots.h"
#define TM2_RENDER_SLOTS_NODES  TM2_RENDER_NODE_SLOT_LIMIT  /* retail 198 */
#define TM2_RENDER_SLOTS_MAX    TM2_RENDER_SLOT_LIMIT       /* retail 200 */
#define TM2_RENDER_PACKET_SLACK 80      /* the emitter tests end < cursor before each primitive (at most 76 bytes) */

struct TM2PortRenderView {              /* 64 bytes */
    int frames;         /* +0x00 views recorded in this row since the reset */
    int slots;          /* +0x04 matrix slots this frame: ctx+0x08 after the walks (0..200, or the port's limit) */
    int slots_max;      /* +0x08 high-water mark */
    int meshes;         /* +0x0C meshes queued this frame (the walker's counter minus the port's bias) */
    int meshes_max;     /* +0x10 high-water mark */
    int mesh_cap;       /* +0x14 the queue's cap this frame: 280, or portMeshListCap (1..2048) */
    int packet;         /* +0x18 bytes of the view's packet buffer used after this view (cursor - start: with
                                 retail's buffer every view drawn so far this frame and their effects, the views
                                 share it; with portPacketBufferBytes this view's alone, request 47 revised) */
    int packet_max;     /* +0x1C high-water mark */
    int packet_cap;     /* +0x20 the buffer's size (end - start): 97,520 or portPacketBufferBytes */
    int packet_scene;   /* +0x24 bytes this view's emitter and near pass added (the level and objects) */
    int packet_scene_max; /* +0x28 high-water mark */
    int caps;           /* +0x2C TM2_RENDER_CAP_* reached this frame */
    int caps_seen;      /* +0x30 every TM2_RENDER_CAP_* reached since the reset */
    int capped;         /* +0x34 frames (views of this row) with any cap reached */
    int first_capped;   /* +0x38 `frames` of the first such view (1-based), 0 = none yet */
    int walk_cursor;    /* +0x3C internal: the packet cursor after the walks */
};

struct TM2PortRenderStats {             /* 0x150 bytes */
    int reset;          /* +0x00 the port writes non-zero: the next recorded view zeroes every row, then this word */
    int row;            /* +0x04 internal: the row of the view being drawn, -1 = none */
    int pad[2];         /* +0x08 */
    struct TM2PortRenderView view[TM2_RENDER_STATS_ROWS];   /* +0x10 */
};
typedef char tm2_port_render_stats_size[(sizeof(struct TM2PortRenderStats) == 0x150) ? 1 : -1];

#ifdef TM2_PORT_FAR_DISTANCE
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_FAR_DISTANCE needs the port variant (build_source_only.py --port-variant)"
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortRenderStats portRenderStats;
/* .port_text (port_variant_arrays.c), called by renderScene only */
extern void portRenderStatsWalked(void *db, int view);  /* after the walks, before the emitter */
extern void portRenderStatsDrawn(void *db);             /* after the emitter and the near pass */
#ifdef __cplusplus
}
#endif
#endif

#endif
