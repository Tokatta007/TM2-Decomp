/* Far distance for the PC port (request 31, docs/FAR_DISTANCE.md).
 *
 * Only with -DTM2_PORT_FAR_DISTANCE (the port's `--port-define TM2_PORT_FAR_DISTANCE`
 * on a port variant build); otherwise this header declares nothing and every caller
 * is the retail C.
 *
 * The port's settings (port_variant_arrays.c, .port_bss, config/US/port_variant_arrays.tsv),
 * all 0 = retail:
 *   portFarDistance        far distance of the normal views, world units (1..32767);
 *                          also turns on the GTE range pass and the effect culls/sort
 *   portMeshListCap        meshes queued per view (retail 280; at most 2048)
 *   portPacketBufferBytes  packet bytes per frame buffer (retail 0x17CF0; at most 390,320 since request 47
 *                          revised: a buffer of its own for every view, views 1-4 and the mirror, and parity)
 *   portMirrorFar          the rear-view mirror's far distance, world units (1..32767; retail 500 for the
 *                          level, 1000 for the objects; request 46 item 9)
 * The functions are in .port_text (port_variant_arrays.c). */
#ifndef SEMANTIC_PORT_FAR_DISTANCE_H
#define SEMANTIC_PORT_FAR_DISTANCE_H

#ifdef TM2_PORT_FAR_DISTANCE
/* renderScene: the far distance of the normal views (retailFar when portFarDistance is 0). */
extern int portFarDistanceFar(int retailFar);
/* renderScene: the rear-view mirror's far distance (view mode 4: retailFar 500 for the level, 1000 for the
 * objects) or the port's portMirrorFar (1..32767) for both; request 46 item 9. */
extern int portFarDistanceMirror(int retailFar);
/* renderScene, after renderInitState: the mesh queue; returns the walker counter's bias. */
extern int portFarDistanceMeshListBegin(void);
/* renderScene: the GTE range pass and the emitter (sub_800CDC54) on this view's queue. Request 61: a scaled entry's
 * translation is corrected for the scaled matrix's rounding, a mesh with sprites or beams is drawn from a copy in
 * portFarSprites with their sizes / 2^k, and the key (+0x1C) is 0x40000000 | k << 28 | the entry's index: the
 * port reads k as (key >> 28) & 3 when bit 30 is set (FAR_DISTANCE.md 2). */
extern void portFarDistanceEmit(void *db, int count);
/* fxSpritesDraw, fxImpactParticlesDraw, streaksDraw: the depth cull and the OT index. */
extern int portFarDistanceFxLimit(void);
extern int portFarDistanceFxOtz(int z);
/* fxDebrisDraw: does a debris piece fit the 16-bit vertices (tr = its 4 x 3 offsets)? */
extern int portFarDistanceDebrisFits(int *tr);
/* One frame record's packet buffer and end: view 0..3 (views 1-4) or 4 (the mirror), parity 0 / 1, at that
 * view's own buffer (request 47 revised; the port calls it for its records of views 3-4, view 2 / 3 here). */
extern void portPacketBuffersApplyView(unsigned char *record, int view, int parity);
/* rtMainLoop, each frame: the six frame records (views 1-2 and the mirror, two parities each). */
extern void portPacketBuffersApply(unsigned char *records, int count);
/* Request 47's call, kept: set 0 = portPacketBuffersApply, set 1 = the port's records of views 3-4. */
extern void portPacketBuffersApplySet(unsigned char *records, int count, int set);
/* rtMainLoop (view 2, the mirror) and the port: a record's packet cursor after the view `before`: its own
 * start when it has a buffer of its own, else `before`'s cursor (retail's shared buffer). */
extern unsigned char *portPacketBufferCursor(unsigned char *record, unsigned char *before);
#endif

#endif
