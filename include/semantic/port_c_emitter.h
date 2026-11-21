/* The model emitter's C form in the port variant (PC port request 61 addendum; docs/PLAYTEST_ANSWERS.md 4.3 item 1,
 * docs/FAR_DISTANCE.md 2 "Precise corners").
 *
 * Only with -DTM2_PORT_C_EMITTER (the port's `--port-define TM2_PORT_C_EMITTER` on a port variant build): the variant
 * links reference/model_emitter_800CDC54.c in place of sub_800CDC54's asm (src/semantic/model_emitter.c). Its integer
 * outputs (packets, OT links, the near list) are the asm's (tools/check_custom.py check 8). Otherwise this header
 * declares nothing and every build keeps the asm.
 *
 * The port's setting (port_variant_arrays.c, .port_bss, every variant; config/US/port_variant_arrays.tsv):
 *   portPreciseEmit   0 = off (retail: the emitter writes nothing more); else the port wants subpixel corners for the
 *                     emitter's camera-facing beams (primitive type 1) and point sprites (types 0, 2, 5..7), and the
 *                     value is the anamorphic x factor in 1/4096 (4096 = none; the port's widescreen squeeze 1/k:
 *                     4096 / k). The integer corners in the packet are stored as before; the subpixel ones go to
 *                     the port's precise ring only. Drawing only (rollback_regions.tsv: snapshot and hash no).
 *
 * The precise ring (the port's memory above the PS1's 8 MB, the port's PORT_PRECISE_RING, decomp/ overlay
 * port_hooks.h): word 0 = the count n, then n entries of 4 words {the vertex word's address in the packet, x and y in
 * 1/16 pixel (s32), depth (s32, > 0)}; at most PORT_PRECISE_MAX entries (the port's reader, port_precise_ring in
 * vplus/port_config.c, takes them into its precise table and sets the count back to 0). Written only while
 * portPreciseEmit != 0. Per drawn beam or sprite four entries, corners 0..3 in the packet's order (+8 + i x stride).
 *
 * The corners (the emitter's own arithmetic in 1/16 GTE unit and 1/16 pixel, no float):
 *   - the end points (a beam's two, a sprite's centre): the vertex through the entry's rotation slot and the exact
 *     translation (the emitter's split R.t, its >> 12 fraction kept, as the port's GTE keeps it), divided by the depth
 *     as the GTE does but not rounded (H, OFX, OFY from the GTE);
 *   - a beam's half width q = w x H / (Z_A x 2^k) (Z_A: end A's depth, as retail's SZ1), at most 2w / 2^k, never
 *     clamped to 1; its direction from the two ends' subpixel difference, taken in the display's shape (the x
 *     difference over the anamorphic factor f), the side step back in the frame's: s = q f (f dy) / L, c = q dx / L,
 *     L = |(dx, f dy)| (f = 1: retail's s = q sin t, c = q cos t with t = ratan2(dy, dx), untruncated); the ends
 *     coinciding: s = 0, c = q (retail's ratan2(0, 0));
 *   - a sprite's half sizes (w, h) x H / (Z x 2^k), at most 500, never clamped to 1, the x one times f;
 *   - k = (key >> 28) & 3 when the entry's key (+0x1C) has bit 30 (the GTE range pass's scaled entries,
 *     TM2_PORT_FAR_DISTANCE), else 0; w and h are the source mesh's (portFarSpriteSource: a far copy's sizes are
 *     rounded w / 2^k);
 *   - depth: Z_A (a sprite's Z) in unscaled GTE units (x 2^k), the same for the four corners.
 * A corner is not reported when an end lies behind the eye or off the GTE's +-1024 pixel range, or the arithmetic
 * would not fit 32 bits (the port keeps the integer corners). */
#ifndef SEMANTIC_PORT_C_EMITTER_H
#define SEMANTIC_PORT_C_EMITTER_H

/* Request 116: portVisual bit 11 (port_visual.h TM2_VIS_PRECISE_CULL, the same value; the
 * emitter's unit does not include that header): the emitter's backface test of a triangle or quad that NCLIP culls is
 * made again on the exact projection (the sign of the camera-space corners' determinant, no division, nothing rounded
 * to pixels), and the polygon is kept when it faces the eye there. The GTE's whole-pixel corners turn polygons less
 * than a pixel thin over (a road's crosswalk strips seen from far along the road: Hong Kong at the port's near scale
 * 2), so retail culls some that face the eye; never culls one that retail keeps. Needs the C emitter (the asm reads no
 * bit). Drawing only (packets, the OT, the near list), no rand(). */
#define PORT_PRECISE_CULL_BIT 0x800

#ifdef TM2_PORT_C_EMITTER
/* Request 94: the port's values (its port_hooks.h; keep the two headers the same): free high RAM 0x80CB0000..0x80CEFFFF,
 * room for every view's beams and sprites (Holland's field at Farthest: 3,072 beams, 12,288 corners; at 4,000 the rest
 * fell back to whole pixels without depth, the flashing rectangles). Before request 94: 0x80C80000, 4,000. */
#define PORT_PRECISE_RING 0x80CB0000
#define PORT_PRECISE_MAX 16380
/* (an array of unknown size here: the emitter's unit has extern_gp, and .port_bss is not $gp-reachable) */
extern int portPreciseEmit[];
#define PORT_PRECISE_EMIT (portPreciseEmit[0])
/* Request 82 (PLAYTEST_ANSWERS.md 27): the near pass's split vertices reported to the precise ring. While
 * portPreciseEmit != 0 the emitter keeps, for each primitive it defers to the near list (entry n, in the list's order:
 * the scratchpad's 71, then the overflow), the GTE depths (SZ, x 2^k as the ring's depths) of its corners in
 * portNearDepth[n * 4 + i] (a triangle's three, a quad's four; a beam or sprite its sort depth four times), and the
 * C near pass (linked with the C emitter, engine_math_near_pass.c) reports to the ring every vertex word it writes:
 * a corner copied into a new primitive at its own place and depth; an edge midpoint at the exact middle of its two
 * corners (in 1/16 pixel: the packet's MIDXY truncates) with the depth of the screen-space midpoint (1 / z is linear on
 * the screen: the harmonic mean 2 za zb / (za + zb)); the centre (quads) the midpoint of the 0-2 and 1-3 midpoints, so.
 * The first corner of the primitive itself keeps its word and its record. Drawing only (snapshot and hash no). */
#define PORT_NEAR_MAX 200
extern int portNearDepth[];
#ifdef TM2_PORT_FAR_DISTANCE
/* The primitive of the mesh the GTE range pass copied for a scaled entry (port_variant_arrays.c, request 61): `prim`
 * itself unless `mesh` is one of portFarSprites' copies, then the source mesh's primitive at the same place. */
extern unsigned char *portFarSpriteSource(unsigned char *mesh, unsigned char *prim);
#endif
#endif

#endif
