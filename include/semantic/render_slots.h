/* Matrix slots per view for the PC port (request 47, docs/FAR_DISTANCE.md 3 "Matrix slots").
 *
 * renderScene gives the level, each world model, the view's sky and each object a matrix slot (ctx+0x08 counts
 * them, `*count < 200`), and the walker gives each drawn transform node (opcode 5) one more while the counter is
 * below 198 (`slti 198` at 80152C18); past that a node shares the last slot (its parts are drawn with the wrong
 * matrix) and past 200 every later object shares slot 200. The slots live in five arrays the walker and the emitter
 * index by slot (renderInitState puts their addresses in the scratchpad RenderState):
 *   renderSlotMatrixPtrs    801ABA50  16 bytes a slot (4 matrix pointers; RenderState +0x04), retail 220 x 16
 *   renderMatrixSlots       801AEB18  32 bytes a slot (rotation; +0x18, the emitter's 4th argument)
 *   renderObjectMatrices    801F8AB8  32 bytes a slot (light; +0x1C, the emitter's 5th argument)
 *   renderFrustumMatricesA  801B2E0C  32 bytes a slot (cull planes, slot matrix 2; +0x44)
 *   renderFrustumMatricesB  801B470C  32 bytes a slot (cull planes, slot matrix 3; +0x48)
 * Retail's 32-byte arrays hold 200 slots, so slot 200 is one past them (RETAIL_BUGS.md 13).
 *
 * -DTM2_PORT_RENDER_SLOTS=N (the port's --port-define; needs the port variant): the limit is N (201..2048; the
 * bare define means 400): renderScene counts up to N, the walker gives transform nodes slots while the counter
 * is below N - 2, and the five arrays move to .port_bss with N + 1 slots (config/US/port_variant_arrays.tsv group
 * renderslots+, so slot N is inside them). The walker is hand-written asm with the 198 as an immediate, so the
 * variant links its pure-C form instead (reference/renderer_walker_engine_render_scene.c, the copy the all-C
 * build proves, included by src/semantic/engine_render_scene.c with the limit as RENDER_WALKER_NODE_SLOTS).
 * Without the define every limit is retail's and the arrays stay at their retail addresses.
 *
 *   TM2_MAX_RENDER_SLOTS        slots in each array: retail 200, N + 1 (may be predefined: the placement check
 *                               forces 200, the retail arrays and limits with the C walker)
 *   TM2_RENDER_SLOT_LIMIT       renderScene's `*count < limit` (retail 200, so its last slot is past the arrays)
 *   TM2_RENDER_NODE_SLOT_LIMIT  the walker's `count < limit` for a transform node's own slot (retail 198) */
#ifndef TM2_SEMANTIC_RENDER_SLOTS_H
#define TM2_SEMANTIC_RENDER_SLOTS_H

#define TM2_RETAIL_RENDER_SLOTS 200

#ifdef TM2_PORT_RENDER_SLOTS
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_RENDER_SLOTS needs the port variant (build_source_only.py --port-variant)"
#endif
#if TM2_PORT_RENDER_SLOTS == 1
#define TM2_PORT_RENDER_SLOT_LIMIT 400      /* the bare define */
#elif TM2_PORT_RENDER_SLOTS < 201 || TM2_PORT_RENDER_SLOTS > 2048
#error "TM2_PORT_RENDER_SLOTS must be 201..2048"
#else
#define TM2_PORT_RENDER_SLOT_LIMIT TM2_PORT_RENDER_SLOTS
#endif
#ifndef TM2_MAX_RENDER_SLOTS
#define TM2_MAX_RENDER_SLOTS (TM2_PORT_RENDER_SLOT_LIMIT + 1)
#endif
#endif

#ifndef TM2_MAX_RENDER_SLOTS
#define TM2_MAX_RENDER_SLOTS TM2_RETAIL_RENDER_SLOTS
#endif
#if TM2_MAX_RENDER_SLOTS < TM2_RETAIL_RENDER_SLOTS
#error "the matrix slot arrays below retail's"
#endif

#if TM2_MAX_RENDER_SLOTS > TM2_RETAIL_RENDER_SLOTS
#define TM2_RENDER_SLOT_LIMIT (TM2_MAX_RENDER_SLOTS - 1)
#else
#define TM2_RENDER_SLOT_LIMIT 200           /* retail: slot 200 is one past the arrays (RETAIL_BUGS.md 13) */
#endif
#define TM2_RENDER_NODE_SLOT_LIMIT (TM2_RENDER_SLOT_LIMIT - 2)

#endif
