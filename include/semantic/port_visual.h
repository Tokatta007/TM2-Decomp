/* Vanilla Plus visual switches (PC port request 42 items 6 and 7, docs/CUSTOM_TOURNAMENT_ANSWERS.md 6 and 7).
 *
 * With -DTM2_PORT_VISUAL (the port's `--port-define TM2_PORT_VISUAL`; needs the port variant) the port's word
 * portVisual switches presentation-only changes, one per bit, read live (no latch: they draw only, call no rand()
 * of their own, change no game state and need not be part of an online ruleset):
 *   bit 0 TM2_VIS_ARC_DEPTH  weaponArcsDraw (801084D8) links each LINE_F2 segment of a taser / lightning arc at its
 *                            own depth (the nearer end, mapped as the level emitter maps SZ: SZ >> 3, then
 *                            renderCompressDepth 8014D368, TM2_VIS_ARC_BIAS entries in front) instead of OT entry 0
 *                            (drawn last, over everything: retail's arc shows through walls). No depth buffer:
 *                            ordering only, so a big wall polygon sorted farther may still let a little through.
 *   bit 1 TM2_VIS_TASER_LOW  the taser's arc (kind 0; the lightning keeps retail's) starts at Outlaw 2's light bar
 *                            (request 76, CUSTOM_TOURNAMENT_ANSWERS.md 7.4): the bar's top on her near model,
 *                            TM2_VIS_TASER_SIREN_Y behind the car's centre and TM2_VIS_TASER_SIREN_Z up, turned
 *                            with the car's body (motion.body_m; portVisualTaserFrom) and followed to the target,
 *                            instead of + 30 straight up over the centre (request 42 had + 25). Its jitter is
 *                            retail's (+-32 at every point; requests 42-87 scaled points 1 and 2 to +-8 / +-16,
 *                            request 88 took that out: the user wants retail's shake).
 *                            The rand() calls are retail's (count and order): only the values they feed are scaled.
 *   bit 2 TM2_VIS_OWN_CAR    renderScene (80152284) walks the view's own car (the local human whose view it is)
 *                            first, before the level, so a full mesh queue (280 / portMeshListCap) or packet buffer
 *                            drops other things before it (retail walks it as the last car: PLAYTEST_ANSWERS.md 3).
 *                            Same matrix, culling and depth as in the object loop; not in the mirror view (mode 4).
 *                            Then one more object number, so the level's top-level meshes never share the car's
 *                            last translation key (else drawn at the car's translation: HIDDEN_AREAS.md 10).
 *   bit 3 TM2_VIS_SHADOW     request 46 item 5 (PLAYTEST_ANSWERS.md 12): renderScene sorts every car's shadow (its
 *                            body object, a world model: carBodyObjects / portCarGroups.body) behind its own car:
 *                            before the emitter, each shadow mesh queued this view gets the OT entry one farther
 *                            than its car's farthest primitive (the emitter's own mapping of the deepest SZ, bias,
 *                            fixed and override entries included), through the draw entry's override word (the
 *                            emitter's `light[0]`), instead of its own depth with the data's -2..-7 bias that put it
 *                            over the car's low rear. Only when the car was walked this view; else retail's.
 *   bit 4 TM2_VIS_GUN_START  request 46 item 4 (PLAYTEST_ANSWERS.md 11): streaksDraw (800D8230) draws a machine-gun
 *                            bullet's streak in its first moved frame from where the shooter's muzzle is now (the
 *                            tail moved by the shooter's own move since the shot) instead of from the spawn point,
 *                            which a fast car (Twister) has left a step behind. The bullet itself (its hit test,
 *                            impacts, damage) is retail's.
 *   bit 5 TM2_VIS_CAMERA_WALL request 63 (FAR_LOD.md 10): viewChaseCamera (80102CC4) keeps the chase camera's eye
 *                            inside the level: the segment from the look-at point to the eye is tested against the
 *                            level's hit volumes (the walls the cars hit: hdp kind 8 under the level model, walked
 *                            read-only by portVisualCameraWall with the car's own LOD / zone / select choices), and
 *                            the eye is pulled in to TM2_VIS_CAMERA_MARGIN in front of the first wall it crosses
 *                            (at least TM2_VIS_CAMERA_NEAR from the look-at point). Each machine's own camera
 *                            (rollback_regions.tsv: portVisual hash no); stores only the camera's eye.
 *   bit 6 TM2_VIS_HIDDEN_AREAS request 72 (HIDDEN_AREAS.md 11; needs --port-define TM2_PORT_HIDDEN_AREAS too):
 *                            renderScene draws the hidden areas of the level from anywhere: for the length of the
 *                            level root's walk of each view, portHiddenAreasBegin points the listed zones and LOD
 *                            records of the loaded level at what the eye could see without the switch (Paris's
 *                            church: the outer walls and the real inside at once, the near facade with its doorway
 *                            and the door at every distance), portHiddenAreasEnd puts every word back before
 *                            anything else reads them (the hit walker reads the same nodes: collisions stay
 *                            retail). For the port's depth test only: without it the inside paints over the walls.
 *                            Read live per view; portHiddenAreas.drawn[view] tells the port what the view drew.
 *                            Request 75 (HIDDEN_AREAS.md 12): the church also gets a roof behind its towers
 *                            (portHiddenRoof.built 2), at the notch's floor height, facing up.
 *   bit 7 TM2_VIS_STEADY_SHADOW request 92 (RENDER_ANSWERS.md R7): every car's shadow (its body object) is drawn tilted by
 *                            the car's own orientation only (motion.rot_m), without the wheels' suspension: retail's
 *                            carBodyPoseUpdate (80114F24) adds each wheel's height over the ground (sway, bounce,
 *                            crush) to the tilt, so on a flat road the blob rocks about its centre 3 units over the
 *                            road and its ends sink into it and come out. Its place (the car's x, y) and height (the
 *                            wheels' ground, + 3 for five types) are retail's. Only while renderScene walks a view
 *                            (portVisualSteadyBegin / End: the step's matrices back after the walks; the body objects
 *                            are model nodes, part of the rollback hash): drawing only, no rand().
 *   bit 8 TM2_VIS_WHOLE_NEAR request 93 (RENDER_ANSWERS.md R8): the model emitter (sub_800CDC54) defers no polygon to
 *                            the near pass: renderScene (80152284) and renderDrawModel (80151378) set the emitter's
 *                            near threshold (scratchpad +0x40: retail 70, 20 split screen; renderDrawModel 70) to 0
 *                            through portVisualNearOtz, so every polygon goes to the order table whole, in the entry
 *                            the near pass would have linked its four pieces in, and renderNearPolys gets an empty
 *                            list. Retail's near pass cuts each polygon nearer than the threshold once into four at
 *                            its screen-space edge midpoints, with the corners' UVs and colours averaged: the affine
 *                            error fixed into the pieces (the pieces' textures slide against each other as the camera
 *                            moves). For a perspective-correct renderer with a depth buffer. Drawing only, no rand().
 *   bit 9 TM2_VIS_MIRROR_CAR request 99 (RENDER_ANSWERS.md R9): the rear-view mirror (view mode 4) looks back from the
 *                            car instead of from the chase camera's eye (retail: viewProc 80155E80 copies view 0's
 *                            camera, 280 behind and 112 above the car in car view 2, 480 / 168 in view 3, and turns it
 *                            half a turn): viewProc, after retail's copy and turn, puts the eye on the car's roof
 *                            (stats roof_height + TM2_VIS_MIRROR_UP over motion.pos, TM2_VIS_MIRROR_BACK behind its
 *                            centre, turned by motion.rot_m) looking back level along the car's heading
 *                            (portVisualMirrorEye); renderScene leaves that car out of the mirror's walk
 *                            (portVisualMirrorSkip: the eye sits on it). Only in car views 2 and 3 (the mirror's);
 *                            the camera objects only (rollback_regions.tsv: hash no): drawing only, no rand().
 *   bit 10 TM2_VIS_SHADOW_GROUND request 102 item 2 (RENDER_ANSWERS.md R10): every car's shadow drawn after all the
 *                            ground under it, for the port's depth test: renderScene, before the emitter, gives each
 *                            shadow mesh queued this view the OT entry TM2_VIS_SHADOW_LIFT nearer than its own
 *                            nearest corner (the emitter's mapping of the shallowest SZ, no bias; at least 1), through
 *                            the same override pair as bit 3. A ground polygon under any point of the shadow has a
 *                            corner at least as far as that point, so it is linked farther and drawn first (retail,
 *                            and bit 3: the shadow at its own farthest corner or its car's, so a big floor polygon
 *                            whose farthest corner is nearer than that, the tile the car has driven off toward the
 *                            camera, was drawn over it). Wins over bit 3. Without a depth test the shadow is drawn
 *                            over its car's low parts too (the depth test keeps the car, nearer, over it). Drawing
 *                            only, no rand().
 *   bit 11 TM2_VIS_PRECISE_CULL request 116 (needs --port-define TM2_PORT_C_EMITTER, not
 *                            TM2_PORT_VISUAL): the model emitter (sub_800CDC54) makes its backface test again on the
 *                            exact projection when NCLIP culls a triangle or quad, and keeps the polygon when it faces
 *                            the eye there (port_c_emitter.h). NCLIP works on the GTE's whole-pixel corners, which
 *                            turn a polygon less than a pixel thin over: Hong Kong's crosswalk (mesh 8002A2D0, eight
 *                            strips 96 units deep) seen from far along the road at the port's near scale 2 lost five
 *                            of its eight strips in one copy, the hole in the road. Never culls a polygon retail
 *                            keeps. Drawing only, no rand().
 * 0 = retail. The word is defined in every variant (port_variant_arrays.c, .port_bss, last); this header declares it
 * and, with the define, the port function. */
#ifndef TM2_SEMANTIC_PORT_VISUAL_H
#define TM2_SEMANTIC_PORT_VISUAL_H

#define TM2_VIS_ARC_DEPTH  0x1      /* bit 0: arcs sorted at their depth (item 6) */
#define TM2_VIS_TASER_LOW  0x2      /* bit 1: the taser starts at Outlaw 2's light bar (item 7; jitter retail's) */
#define TM2_VIS_OWN_CAR    0x4      /* bit 2: the view's own car walked first (request 45 item 3) */
#define TM2_VIS_SHADOW     0x8      /* bit 3: every car's shadow sorted behind its car (request 46 item 5) */
#define TM2_VIS_GUN_START  0x10     /* bit 4: a bullet's first streak from the muzzle's place now (request 46 item 4) */
#define TM2_VIS_CAMERA_WALL 0x20     /* bit 5: the chase camera's eye kept inside the level (request 63) */
#define TM2_VIS_HIDDEN_AREAS 0x40    /* bit 6: the hidden areas drawn from anywhere (request 72, TM2_PORT_HIDDEN_AREAS) */
#define TM2_VIS_STEADY_SHADOW 0x80   /* bit 7: shadows tilted with the car only, no suspension (request 92) */
#define TM2_VIS_WHOLE_NEAR 0x100    /* bit 8: no near-pass cutting, every polygon whole (request 93) */
#define TM2_VIS_MIRROR_CAR 0x200    /* bit 9: the mirror looks back from the car's roof (request 99) */
#define TM2_VIS_SHADOW_GROUND 0x400  /* bit 10: every car's shadow after the ground under it (request 102 item 2) */
#define TM2_VIS_PRECISE_CULL 0x800   /* bit 11: the emitter's backface test on the exact projection (request 116;
                                        TM2_PORT_C_EMITTER, port_c_emitter.h PORT_PRECISE_CULL_BIT) */
#define TM2_VIS_ALL        0xFFF
#define TM2_VIS_MIRROR_UP  8        /* bit 9: the eye this far over the roof (position units) */
#define TM2_VIS_MIRROR_BACK (-16)   /* bit 9: and this far behind the car's centre (+y forward) */
#define TM2_VIS_MIRROR_PITCH 0      /* bit 9: looking back level (the camera's pitch) */
#define TM2_VIS_CAMERA_MARGIN 48    /* bit 5: the eye this far (position units, 6 world units) in front of the wall */
#define TM2_VIS_CAMERA_NEAR 64      /* bit 5: and at least this far from the look-at point */
#define TM2_VIS_CAMERA_CLEAR 128    /* bit 5, requests 96 and 100 (FAR_LOD.md 15, 16): after request 63's pull the
                                       eye is moved off any solid hit volume nearer than this (position units, 16
                                       world units) along each axis, to the middle where the space is narrower than
                                       twice this (probes from the eye: sideways from a wall, up from a floor; never
                                       more than this toward the car). At 1 player's world shift 3 the GTE's
                                       projection saturates nearer than H / 2 = 114 of its units, about 127 position
                                       units, so wall polygons nearer than that break up */
#define TM2_VIS_ARC_BIAS   4        /* bit 0: OT entries in front of the nearer end (8 camera units each up close) */
#define TM2_VIS_TASER_SIREN_Y (-15) /* bit 1: the light bar's centre on Outlaw 2's model (+y forward) */
#define TM2_VIS_TASER_SIREN_Z 20    /* bit 1: and its top (retail's start: + 30 straight up over the centre) */
#define TM2_VIS_SHADOW_KEYS 48      /* bit 3: body-object slots tracked (TM2_CAR_KEYS; 15 in retail) */
#define TM2_VIS_SHADOW_PAIRS 64     /* bit 3: shadow draw entries a view can re-sort */
#define TM2_VIS_SHADOW_LIFT 2       /* bit 10: OT entries in front of the shadow's nearest corner (the 3-unit lift, the
                                       in-between frames) */
#define TM2_VIS_BULLETS    40       /* bit 4: bulletRecords */

/* The port's visual state (request 46; .port_bss, every variant, after portCarDtParkMask; used only with the define):
 * per view the draw-entry ranges each body object (shadow) and each car object queued (renderScene's walks), the
 * override pairs handed to the emitter for the shadows, and per bullet record the shooter's position at the shot. */
struct TM2PortVisualRange {
    unsigned char *first, *last;    /* draw entries [first, last) of renderMeshList / portMeshList, 0 = none */
};
struct TM2PortVisualBullet {
    int *pos;                       /* the shooter's motion.pos, 0 = none (no car with the bullet's owner id) */
    int at[3];                      /* that position at the shot */
    int life;                       /* the bullet's life right after the shot (bulletSpawn's 12 or 15) */
};
struct TM2PortVisualState {
    struct TM2PortVisualRange shadow[TM2_VIS_SHADOW_KEYS];   /* +0x000 bit 3: per body-object slot (key >> 4) */
    struct TM2PortVisualRange car[TM2_VIS_SHADOW_KEYS];      /* +0x180 bit 3: that key's car object */
    int pairs[TM2_VIS_SHADOW_PAIRS][2];                       /* +0x300 bit 3: {OT entry, light record} per entry */
    struct TM2PortVisualBullet bullets[TM2_VIS_BULLETS];      /* +0x500 bit 4 */
    int tail[3];                                              /* +0x820 bit 4: the tail streaksDraw draws */
};                                                            /* 0x82C bytes */
typedef char tm2_port_visual_state_size[(sizeof(struct TM2PortVisualState) == 0x82C) ? 1 : -1];

/* Request 92 (bit 7; .port_bss, every variant, before portStackGuard): the step's matrix of each body object
 * portVisualSteadyBegin re-posed for this view's walks, put back by portVisualSteadyEnd (32 bytes each: the 3 x 3
 * rotation, pad and translation at the object's +0x04). Drawing only (rollback_regions.tsv snapshot and hash no). */
struct TM2PortSteadyShadow {
    unsigned char saved[TM2_VIS_SHADOW_KEYS];   /* +0x000 1 = m[k] holds body object k's step matrix now */
    int m[TM2_VIS_SHADOW_KEYS][8];              /* +0x030 */
};                                              /* 0x630 bytes */
typedef char tm2_port_steady_shadow_size[(sizeof(struct TM2PortSteadyShadow) == 0x630) ? 1 : -1];

/* Request 72 (HIDDEN_AREAS.md 11; .port_bss, every variant, after portCarProfile; used only with
 * TM2_PORT_HIDDEN_AREAS): bit 6's patches of the level model. Drawing only: every patched word is put back by the end
 * of the level root's walk (rollback_regions.tsv snapshot and hash no). */
#define TM2_VIS_HIDDEN_PATCHES 32   /* words a level's list may patch (the church: 6; Hong Kong: the temple's 8 and the
                                       subway's 24, request 104) */
#define TM2_VIS_HIDDEN_SUBWAY 20    /* request 104: the subway's own parts walked with the city (HIDDEN_AREAS.md 15) */
#define TM2_VIS_HIDDEN_VIEWS 8      /* renderScene's view numbers kept in drawn[] (view & 7) */
#define TM2_VIS_HIDDEN_NONE 0       /* place: no listed hidden area in the loaded level */
#define TM2_VIS_HIDDEN_PARIS 1      /* place: Paris's church, PARIS.DMD */
#define TM2_VIS_HIDDEN_PARIS2 2     /* place: Paris's church, PARIS2.DMD (the same nodes at other addresses) */
#define TM2_VIS_HIDDEN_HKONG 3      /* place: Hong Kong's temple, HKONG.DMD (request 97, HIDDEN_AREAS.md 14) */
#define TM2_VIS_HIDDEN_BURB 4       /* place: Cyburbia's suburb and ring highway, BURB.DMD (request 119,
                                       HIDDEN_AREAS.md 17) */
struct TM2PortHiddenAreas {
    int place;                                  /* +0x00 TM2_VIS_HIDDEN_*: what the latest walk found */
    int count;                                  /* +0x04 words patched now (0 outside a walk) */
    int drawn[TM2_VIS_HIDDEN_VIEWS];            /* +0x08 per view: the words its latest level walk patched, 0 =
                                                   drawn as retail (bit clear, nothing listed in the level) */
    unsigned int *word[TM2_VIS_HIDDEN_PATCHES]; /* +0x028 the patched words */
    unsigned int saved[TM2_VIS_HIDDEN_PATCHES]; /* +0x0A8 their values before the walk */
    unsigned char group[32];                    /* +0x128 a type-1 node (never culled) holding two children: the
                                                   church's outer walls and its inside, for the zone (Hong Kong:
                                                   the city and the temple's inside) */
    unsigned char subway[32];                   /* +0x148 request 104: a type-1 node (never culled): the subway
                                                   zone's outside child (the temple's zone, the city) and
                                                   subway_parts */
    unsigned char subway_parts[24 + 4 * TM2_VIS_HIDDEN_SUBWAY];  /* +0x168 a type-1 node (never culled): the
                                                   subway's parts the city lacks */
};                                              /* 0x1D0 bytes */
typedef char tm2_port_hidden_areas_size[(sizeof(struct TM2PortHiddenAreas) == 0x1D0) ? 1 : -1];

/* Request 75 (HIDDEN_AREAS.md 12; .port_bss, every variant, after portSpecialState; used only with
 * TM2_PORT_HIDDEN_AREAS): the church's roof under bit 6. The level model has no roof: the car drives on the
 * collision's top (z 824, position units) and only the notch between the towers has a floor (4 tiles at z 832).
 * The roof is the nave's ceiling tiles behind the towers (two runs of its primitive list), walked a second time
 * through a turn node (opcode 5, half a turn about y: x -> -x, z -> 1,752 - z): from z 920, facing down, to z 832,
 * facing up (the tiles are symmetric in x). No polygon is
 * copied: the two meshes are the nave's header with their own primitive start and count. Drawing only (snapshot and
 * hash no). */
#define TM2_VIS_HIDDEN_ROOF_Z 832           /* the roof's height (the notch's floor; the walls' upper parts start there) */
#define TM2_VIS_HIDDEN_CEILING_Z 920        /* the nave's ceiling, the tiles' height in the file */
#define TM2_VIS_HIDDEN_ROOF_BACK 13         /* the nave's primitives 13..24: its ceiling's back two rows */
#define TM2_VIS_HIDDEN_ROOF_BACK_N 12
#define TM2_VIS_HIDDEN_ROOF_MID 31          /* primitives 31..36: the row in front of them, up to the towers */
#define TM2_VIS_HIDDEN_ROOF_MID_N 6
/* Request 78 (HIDDEN_AREAS.md 12.6): the nave's walls run up to its ceiling (z 920), 88 above the roof, and face in:
 * from on the roof their tops stood up as a wall around it (the "little wall", request 76 item 1) that no car hits.
 * For the walk of a view whose eye is above the roof (world z > TM2_VIS_HIDDEN_ROOF_Z), the nave's vertices at z 920
 * (its ceiling and its walls' top edges: 42 of them, shared with nothing walked) are at z 832 instead and the turn's
 * offset is 2 x 832, so the roof stays at 832; portHiddenAreasDrawn (request 84: after the emitter and the near pass,
 * which read the vertices; End came too early) puts them back. From inside nothing changes. */
struct TM2PortHiddenRoof {
    int built;                                  /* +0x00 the roof's meshes in the latest walk (2), 0 = none */
    unsigned char group[32];                    /* +0x04 a type-1 node (never culled): portHiddenAreas.group and
                                                   the turn node, for the zone */
    unsigned char turn[48];                     /* +0x24 a type-5 node: rotation diag(-1, 1, -1), offset z 1,752,
                                                   the two meshes */
    unsigned char mesh[2][44];                  /* +0x54 the nave's mesh header, primitives 13 (12) and 31 (6) */
    unsigned char *nave;                        /* +0xAC request 78: the nave the roof was built from (0 = none) */
    int lowered;                                /* +0xB0 request 78: 1 while the nave's top vertices are at the roof's
                                                   height (the eye above the roof), put back by
                                                   portHiddenAreasDrawn */
};                                              /* 0xB4 bytes */
typedef char tm2_port_hidden_roof_size[(sizeof(struct TM2PortHiddenRoof) == 0xB4) ? 1 : -1];

/* Request 79 (PICKUP_LOOKS.md 10; .port_bss, every variant, after portHiddenRoof; used only with TM2_PORT_VISUAL): a
 * pickup look for "a special", built by worldLoadStartObjects at each load of a level, for the port to point pickups
 * at in the shared-model way (PICKUP_LOOKS.md 6.1: the pickup node's child = portPickupSpecial.model). It is a weapon
 * pickup's model with the level's HUD icon for the special instead of the weapon's: the card is the HUD's SPECIAL
 * globe (object 1260's selected icon 0, 32 x 32, which reads SPECIAL itself), an upright billboard as the weapon's;
 * the hit box is the donor weapon pickup's (shared). No blinking name (request 83: the globe already says it).
 * Nothing of the level is changed: the port does the pointing. No rand(). */
#define TM2_PICKUP_SPECIAL_CARD 32          /* the card's half width and half height (the weapon cards: 20 x 50) */
#define TM2_PICKUP_SPECIAL_CARD_RGB 0x808080    /* the HUD's selected icon: 128, 128, 128 */
struct TM2PortPickupSpecial {
    unsigned char *model;                       /* +0x00 the look (a type-1 group) for a pickup node's child, 0 = none
                                                   (the level has no HUD icons or no weapon pickup of the usual shape) */
    void *hud;                                  /* +0x04 the HUD icon object (1260) of the load in progress */
    unsigned char *donor;                       /* +0x08 the weapon pickup model it was built from */
    unsigned char group[32];                    /* +0x0C type 1: the card, the donor's hit box */
    unsigned char mesh[44];                     /* +0x2C type 0: the card (one primitive) */
    unsigned int prim[9];                       /* +0x58 an FT4 camera-facing beam (kind 1, unlit) */
    short vert[4];                              /* +0x7C its centre (the donor's card's) */
};                                              /* 0x84 bytes */
typedef char tm2_port_pickup_special_size[(sizeof(struct TM2PortPickupSpecial) == 0x84) ? 1 : -1];

/* Request 81 (PICKUP_LOOKS.md 11; used only with TM2_PORT_VISUAL): where a mine's and the freeze shot's pictures are
 * in VRAM in this match, for the port's kill feed. They are sprites of the loaded level's models (asset 2212, the mine
 * pool's model; 2202, weaponFreezeMissileModel), on each arena's own texture pages, so they are read from the models
 * at run time. portWeaponIcon fills a TM2PortIcon with the model's largest sprite: the mine's one sprite, the freeze
 * shot's head in animation frame `frame` (0..15, taken modulo the frame count). Returns 1, or 0 when the model is not
 * loaded or not of that shape (out untouched). Reads only; no rand(). */
#define TM2_ICON_MINE 0                     /* the dropped mine (energy attack 101's and the mine pickups') */
#define TM2_ICON_FREEZE 1                   /* the freeze shot (energy attack 106, the freeze missile) */
struct TM2PortIcon {
    unsigned short tpage;                   /* +0x00 the GPU texture page word (x / 64, y / 256 << 4, abr << 5, depth << 7) */
    unsigned short clut;                    /* +0x02 the GPU CLUT word (x / 16 | y << 6) */
    unsigned char u, v, w, h;               /* +0x04 the texel rectangle in the page (w x h, u and v its top-left) */
    unsigned int colour;                    /* +0x08 the model's colour word (code << 24 | b << 16 | g << 8 | r):
                                               code 0x2E is semi-transparent, in the page's abr mode (the mine: 2,
                                               subtractive; the freeze: 1, additive, blue) */
};                                          /* 12 bytes */
typedef char tm2_port_icon_size[(sizeof(struct TM2PortIcon) == 12) ? 1 : -1];

#ifdef TM2_PORT_VISUAL
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_VISUAL needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portVisual
#pragma tm2_absolute portVisualState
#pragma tm2_absolute portHiddenAreas
#pragma tm2_absolute portHiddenRoof
#pragma tm2_absolute portPickupSpecial
#pragma tm2_absolute portSteadyShadow
#ifdef __cplusplus
extern "C" {
#endif
extern int portVisual;                          /* the port's switches (TM2_VIS_*), 0 = retail; read live */
extern struct TM2PortVisualState portVisualState; /* request 46: written by the C */
extern struct TM2PortHiddenAreas portHiddenAreas; /* request 72: written by the C */
extern struct TM2PortHiddenRoof portHiddenRoof; /* request 75: written by the C */
extern struct TM2PortPickupSpecial portPickupSpecial; /* request 79: written by the C at each level load */
extern struct TM2PortSteadyShadow portSteadyShadow; /* request 92: written by the C */
/* .port_text (port_variant_arrays.c) */
extern int portVisualArcOtz(int za, int zb);    /* bit 0: the OT entry of an arc segment whose ends have depth
                                                   z / 4 = za, zb (weaponArcsDraw's scr[][2]); 1..4094 */
extern unsigned char *portVisualOwnCar(int view); /* bit 2: renderScene: the car object to walk first for `view`,
                                                   or 0 (bit clear, mirror view, no local human, hidden car) */
extern void portVisualWalked(unsigned char *obj, void *model, unsigned char *first); /* bit 3: renderScene, after
                                                   a walk of `model` for `obj` that queued draw entries from `first` */
extern void portVisualShadows(void);            /* bit 3: renderScene, before the emitter: the shadows' entries;
                                                   clears the view's ranges (bit clear too) */
extern int portVisualSteadyBegin(void);         /* bit 7: renderScene, before the walks: the shadows re-posed;
                                                   returns how many (0 = none, bit clear) */
extern void portVisualSteadyEnd(void);          /* bit 7: renderScene, after the walks: the step's matrices back */
extern int portVisualNearOtz(int retail);       /* bit 8: renderScene / renderDrawModel: the emitter's near
                                                   threshold (0 with the bit, else `retail`) */
extern int portVisualMirrorEye(int view, int *pos, short *rot); /* bit 9: viewProc, the mirror view `view`: its
                                                   camera's eye and rotation on the car; 1 when set */
extern unsigned char *portVisualMirrorSkip(int view); /* bit 9: renderScene: the car object the mirror view leaves
                                                   out (0 none) */
extern void portVisualBulletSpawn(int slot);    /* bit 4: bulletSpawn, a bullet just set up in `slot` */
extern int *portVisualTaserFrom(int *from, int *out); /* bit 1: weaponArcsDraw: the taser's start (the light bar)
                                                 * for a shooter at `from` (its motion.pos), into `out` */
extern void portVisualCameraWall(int *look, int *eye); /* bit 5: viewChaseCamera: pull `eye` in front of the
                                                   level's first wall between `look` and it (reads only, but `eye`) */
extern int *portVisualBulletTail(int slot, int *tail); /* bit 4: streaksDraw: the streak's tail to draw (the
                                                   record's previous point `tail`, or the port's moved copy) */
extern void portPickupSpecialBuild(void);       /* request 79: worldLoadStartObjects, after the level's objects:
                                                   portPickupSpecial.model from the HUD icons and a weapon pickup */
extern int portWeaponIcon(int which, int frame, struct TM2PortIcon *out); /* request 81: the port's kill feed: a
                                                   mine's / the freeze shot's sprite in VRAM (TM2_ICON_*) */
#ifdef TM2_PORT_HIDDEN_AREAS
extern int portHiddenAreasBegin(int view);      /* bit 6: renderScene, before the level root's walk: patch the
                                                   loaded level's list; returns the words patched (0 = none) */
extern void portHiddenAreasEnd(void);           /* bit 6: renderScene, after that walk: every word back */
extern void portHiddenAreasDrawn(void);         /* bit 6: renderScene, after the emitter and the near pass: the
                                                   vertices moved for the view back (request 84), and the
                                                   breakable walls one-sided again (request 113) */
extern int portTwoSidedBegin(void);             /* bit 6: renderScene, after portHiddenAreasBegin: the loaded level's
                                                   breakable walls drawn from both sides (request 113); returns how
                                                   many faces */
#endif
#ifdef __cplusplus
}
#endif
#endif

#endif
