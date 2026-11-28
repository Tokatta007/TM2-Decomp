/* Port variant storage (PC port request 25; docs/SOURCE_BUILD.md "Port variant").
 *
 * Compiled only by the port variant (`build.py --source-only --relocatable
 * --port-variant`, -DTM2_PORT_VARIANT); never part of the retail builds, which
 * keep this storage at its retail addresses. Every array whose size is a
 * player limit of include/semantic/players.h is defined here, sized by the
 * limit, in one of two sections that tools/gen_link.py places at the port base
 * (default 0x80200000, above the PS1's 2 MB; the port has 8 MB):
 *   .port_data  initialised storage (loaded by the port: port_data.bin);
 *   .port_bss   zero storage (NOLOAD: crt0 clears only the retail BSS, so the
 *               port must zero this area before the game runs).
 * car_object_lookup.c, the only built unit that defines retail storage of one
 * of these arrays (humanCarObjects), leaves it out in the variant; the retail
 * storage stays where it was, unused (plain retail bytes in the link).
 * config/US/port_variant_arrays.tsv lists each array with its retail address
 * and size.
 *
 * Only sizes and alignment matter here (the units read the arrays through
 * their own record views), so the element types are plain words. Order is
 * kept as in retail where code reaches one array from another (the four
 * radar-arrow sprite arrays, which retail's hudRadarArrowsLoad addresses as
 * one [4][2] block). */
#include "semantic/players.h"
#include "semantic/car_groups.h"
#include "semantic/port_cars.h"
#include "semantic/tex_banks.h"
#include "semantic/car_specials.h"
#include "semantic/car_sound.h"
#include "semantic/car_cluts.h"
#include "semantic/car_dropin.h"
#include "semantic/spu_voices.h"
#include "semantic/dt_player.h"
#include "semantic/port_hazards.h"
#include "semantic/port_humans.h"
#include "semantic/port_balance.h"
#include "semantic/port_visual.h"
#include "semantic/car_numbers.h"
#include "semantic/port_render_stats.h"
#include "semantic/render_slots.h"
#include "semantic/port_rollback.h"
#include "semantic/port_far_lod.h"    /* request 98: struct TM2PortFarLodBudget */
#include "semantic/port_mod_arena.h"  /* request 114: TM2PortArenaRules */
#include "semantic/port_wreck_fall.h" /* request 117: portWreckFallMark */

/* Without -DTM2_PORT_VARIANT (a retail build, tools/data_audit.py) the unit is empty. */
#ifdef TM2_PORT_VARIANT

#define PORT_BSS __attribute__((section(".port_bss")))
#define PORT_DATA __attribute__((section(".port_data")))

/* ---- initialised (.port_data) ---- */
/* carViewHotkeys: hotkey released (re-armed) per player; retail sdata {1, 1}. */
int carViewHotkeyReleased[TM2_MAX_HUMANS] PORT_DATA = {
#if TM2_MAX_HUMANS > 2
    [0 ... TM2_MAX_HUMANS - 1] = 1      /* every human record (up to 16, HUMANS16_PLAN.md H4) */
#else
    1, 1,
#endif
};

/* viewModeTable: the view-mode table (8017EC44, 96 bytes a row; docs/WIDESCREEN.md s2), PC port
 * request 26. Rows 0-8 are the retail rows word for word (retail .data, no pointers); rows 9 to
 * TM2_MAX_VIEW_MODES-1 start zeroed for the port's own modes (9-12 the four quarters, 13 the
 * split-screen mirror). */
int viewModeTable[TM2_MAX_VIEW_MODES][96 / 4] PORT_DATA = {
    { /* 0 */
        0x0000003C, 0x0000002F, 0x00000115, 0x00000115, 0x00000140, 0x000000F0,
        0x00000140, 0x000000F0, 0x00000000, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x0800F225, 0x00000000, 0x08000DDB, 0x00000000, 0x0FFF0000, 0x00000000,
        0x065B0000, 0x0000F152, 0x065B0000, 0x00000EAE, 0xF0010000, 0x00000000,
    },
    { /* 1 */
        0x00000046, 0x00000037, 0x000000E4, 0x000000E4, 0x00000140, 0x000000F0,
        0x00000140, 0x000000F0, 0x00000000, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x092DF2E5, 0x00000000, 0x092D0D1B, 0x00000000, 0x0FFF0000, 0x00000000,
        0x07700000, 0x0000F1D6, 0x07700000, 0x00000E2A, 0xF0010000, 0x00000000,
    },
    { /* 2 */
        0x00000046, 0x00000037, 0x000000E4, 0x000000E4, 0x00000140, 0x000000F0,
        0x00000140, 0x00000078, 0x00000000, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x0898F282, 0x00000000, 0x08980D7E, 0x00000000, 0x0FFF0000, 0x00000000,
        0x06E50000, 0x0000F191, 0x06E50000, 0x00000E6F, 0xF0010000, 0x00000000,
    },
    { /* 3 */
        0x00000046, 0x00000037, 0x000000E4, 0x000000E4, 0x00000140, 0x000000F0,
        0x00000140, 0x00000078, 0x00000000, 0x00000078, 0x0E6A1000, 0x00000E6A,
        0x0898F282, 0x00000000, 0x08980D7E, 0x00000000, 0x0FFF0000, 0x00000000,
        0x06E50000, 0x0000F191, 0x06E50000, 0x00000E6F, 0xF0010000, 0x00000000,
    },
    { /* 4 */
        0x00000046, 0x00000037, 0x0000005B, 0x0000005B, 0x00000140, 0x000000F0,
        0x00000065, 0x0000001E, 0x000000D3, 0x00000014, 0x0E6AF000, 0x00000E6A,
        0x0898F282, 0x00000000, 0x08980D7E, 0x00000000, 0x0FFF0000, 0x00000000,
        0x06E50000, 0x0000F191, 0x06E50000, 0x00000E6F, 0xF0010000, 0x00000000,
    },
    { /* 5 */
        0x0000003C, 0x0000002F, 0x0000008A, 0x0000008A, 0x00000140, 0x000000F0,
        0x000000A0, 0x000000F0, 0x00000000, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x0800F225, 0x00000000, 0x08000DDB, 0x00000000, 0x0FFF0000, 0x00000000,
        0x0A790000, 0x0000F3E8, 0x0A790000, 0x00000C18, 0xF0010000, 0x00000000,
    },
    { /* 6 */
        0x0000003C, 0x0000002F, 0x0000008A, 0x0000008A, 0x00000140, 0x000000F0,
        0x000000A0, 0x000000F0, 0x000000A0, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x0800F225, 0x00000000, 0x08000DDB, 0x00000000, 0x0FFF0000, 0x00000000,
        0x0A790000, 0x0000F3E8, 0x0A790000, 0x00000C18, 0xF0010000, 0x00000000,
    },
    { /* 7 */
        0x00000046, 0x00000037, 0x000001C9, 0x000001C9, 0x00000280, 0x000001E0,
        0x00000280, 0x000001E0, 0x00000000, 0x00000000, 0x0E6A1000, 0x00000E6A,
        0x092DF2E5, 0x00000000, 0x092D0D1B, 0x00000000, 0x0FFF0000, 0x00000000,
        0x07700000, 0x0000F1D6, 0x07700000, 0x00000E2A, 0xF0010000, 0x00000000,
    },
    { /* 8 */
        0x00000046, 0x00000037, 0x000000B6, 0x000000B6, 0x00000280, 0x000001E0,
        0x000000C9, 0x0000006B, 0x000000EA, 0x00000121, 0x0E6A1000, 0x00000E6A,
        0x092DF2E5, 0x00000000, 0x092D0D1B, 0x00000000, 0x0FFF0000, 0x00000000,
        0x05960000, 0x0000F102, 0x05960000, 0x00000EFE, 0xF0010000, 0x00000000,
    },
};

/* ---- zero (.port_bss) ---- */
/* Human records and their per-index arrays (FOUR_HUMANS.md 1.1). */
int humanCars[TM2_MAX_HUMANS][0x814 / 4] PORT_BSS = {{0}};
int humanCarObjects[TM2_MAX_HUMANS] PORT_BSS = {0};
int humanCarObjectSlots[TM2_MAX_HUMANS] PORT_BSS = {0};
int padConfigs[TM2_MAX_HUMANS] PORT_BSS = {0};
int humanCarTypes[TM2_MAX_HUMANS] PORT_BSS = {0};
int hudSmoothedSpeeds[TM2_MAX_HUMANS] PORT_BSS = {0};
#if TM2_MAX_HUMANS > TM2_VARIANT_HUMANS
/* Weapon-reset flag per human (carSetResetWeapons; 1 = reset the pickup weapons at the round start). Retail's
 * storage (timer_state.c, 80180FA0: u8[2] + 2 bytes of padding) holds 4; with more human records
 * (TM2_PORT_HUMANS 5..16, HUMANS16_PLAN.md H4) it is here and timer_state.c drops its definition. */
unsigned char carResetWeapons[TM2_MAX_HUMANS] PORT_BSS = {0};
#endif
#if TM2_MAX_HUMANS > 2
/* Car-select indices of players 3..TM2_MAX_HUMANS (settings.h). */
signed char sessionPortHumanCars[TM2_MAX_HUMANS - 2] PORT_BSS = {0};
#endif

/* Views (FOUR_PLAYERS.md 4.4): camera parents, sky copies, view objects,
 * environments, modes, eye positions, view matrices. */
int viewCameraObjectTable[TM2_MAX_VIEWS] PORT_BSS = {0};
int viewFollowObjects[TM2_MAX_VIEWS] PORT_BSS = {0};    /* retail sdata, zero */
int viewCameraObjects[TM2_MAX_VIEWS] PORT_BSS = {0};
int viewEnvs[TM2_MAX_VIEWS] PORT_BSS = {0};
int viewModes[TM2_MAX_VIEWS] PORT_BSS = {0};
int viewEyePositions[TM2_MAX_VIEWS][3] PORT_BSS = {{0}};
int viewMatrices[TM2_MAX_VIEWS][8] PORT_BSS = {{0}};   /* MATRIX, 32 bytes */

/* HUDs (FOUR_PLAYERS.md 5.1). */
int hudMarkerBlinkCounters[TM2_MAX_HUDS] PORT_BSS = {0};   /* retail sdata, zero */
int hudMarkerBlinkPhases[TM2_MAX_HUDS] PORT_BSS = {0};     /* retail sdata, zero */
int hudOrderTables[2][TM2_MAX_HUDS][2] PORT_BSS = {{{0}}}; /* [buffer][player][2] */
int hudPlayerRecords[TM2_MAX_HUDS][1900 / 4] PORT_BSS = {{0}};
/* hudRadars: after the car storage (below), as its size depends on TM2_PORT_CARS (radar blips) */
int hudRadarArrowUpPrims[TM2_MAX_HUDS][32 / 4] PORT_BSS = {{0}};
int hudRadarArrowDownPrims[TM2_MAX_HUDS][32 / 4] PORT_BSS = {{0}};
int hudRadarArrowLeftPrims[TM2_MAX_HUDS][32 / 4] PORT_BSS = {{0}};
int hudRadarArrowRightPrims[TM2_MAX_HUDS][32 / 4] PORT_BSS = {{0}};
int hudTargetNameModes[TM2_MAX_HUDS][12 / 4] PORT_BSS = {{0}};
int hudTargetNameSprites[TM2_MAX_HUDS][600 / 4] PORT_BSS = {{0}};
int hudRadarArrowUpSprites[TM2_MAX_HUDS][8 / 4] PORT_BSS = {{0}};
int hudRadarArrowDownSprites[TM2_MAX_HUDS][8 / 4] PORT_BSS = {{0}};
int hudRadarArrowLeftSprites[TM2_MAX_HUDS][8 / 4] PORT_BSS = {{0}};
int hudRadarArrowRightSprites[TM2_MAX_HUDS][8 / 4] PORT_BSS = {{0}};

/* ---- far LOD (PC port request 30, docs/FAR_LOD.md) ---- */
/* The port's switch, set from its settings: 0 = retail. Non-zero has an effect only in a
 * build with --port-define TM2_PORT_FAR_LOD (the hooks in worldLoadStartObjects,
 * assetInstallImage and renderScene). */
int portFarLod PORT_BSS = 0;

/* The LOD records to extend in this match (filled at load by portFarLodScanLevel and
 * portFarLodScan): each record and its retail max; bit 0 of a record pointer is set while
 * renderScene has that record extended (portFarLodBegin .. portFarLodEnd). The disc needs at
 * most 809 for a level (DISH) and 13 per car (tools/dmd_lod_dump.py --summary); beyond 2048
 * a record keeps its retail max. Used only with TM2_PORT_FAR_LOD. */
#define FAR_LOD_MAX_RECORDS 2048
struct PortFarLodTable {
    int count;
    unsigned int *records[FAR_LOD_MAX_RECORDS];
    unsigned int maxes[FAR_LOD_MAX_RECORDS];
};
struct PortFarLodTable portFarLodTable PORT_BSS = {0};
/* Request 46 item 6 (FAR_LOD.md 8): the band scale and every record's retail band, defined after
 * portCarDtParkMask (below; nothing before it moves). */
#define FAR_LOD_NEAR_RECORDS 4096   /* the disc: at most 1,582 records in a level (DISH), 33 in a car file */
struct PortFarLodNear {
    int count;
    unsigned int *records[FAR_LOD_NEAR_RECORDS];    /* bit 0 set while renderScene has the record scaled */
    unsigned int maxes[FAR_LOD_NEAR_RECORDS];       /* its retail max and min (d2, world units squared) */
    unsigned int mins[FAR_LOD_NEAR_RECORDS];
};
extern int portFarLodNearScale;
extern struct PortFarLodNear portFarLodNear;
extern int portFarLodBudget;                            /* request 98 */
extern struct TM2PortFarLodBudget portFarLodBudgetState;
extern int viewGetMode(int view);

#ifdef TM2_PORT_FAR_LOD
#define PORT_TEXT __attribute__((section(".port_text")))

/* Model nodes as the tree walker renderWalkModel reads them (docs/DRAW_DISTANCE.md 1,
 * reference/renderer_walker_engine_render_scene.c). An LOD node (opcode 2) holds the record
 * count at +16 and the record pointers from +20; a record is {max, min, child}: the walker
 * draws the child of the first record with min <= d2 < max (d2 = squared distance in world
 * units, unsigned). */
#define FAR_LOD_W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))
#define FAR_LOD_P(p, o) (*(unsigned char **)((unsigned char *)(p) + (o)))
#define FAR_LOD_MAX_DEPTH 32        /* the disc's model trees are at most 22 deep */
/* A listed record pointer's low bits: bit 0 = changed by portFarLodBegin now; bit 1 (request 91, FAR_LOD.md 13) = a
 * record of a beam field's finest level: in portFarLodNear, drawn at every distance while portFarLod is set; in
 * portFarLodTable, a coarser version of one, drawn at none. */
#define FAR_LOD_FINE 2u
#define FAR_LOD_REC(p) ((unsigned int *)((unsigned int)(p) & ~3u))

/* Request 91: the LOD records the scan's walk came through, from the root down, that have a coarser version (an LOD
 * node and the index of its record), and the path of the last beam LOD marked (the same instance again: nothing to
 * do). On the scan's stack. */
struct PortFarLodPath {
    int n, last_n;
    unsigned char *last_lod;
    unsigned char *lods[FAR_LOD_MAX_DEPTH + 2], *last_lods[FAR_LOD_MAX_DEPTH + 2];
    unsigned char recs[FAR_LOD_MAX_DEPTH + 2], last_recs[FAR_LOD_MAX_DEPTH + 2];
};
#define portFarLodCount portFarLodTable.count
#define portFarLodRecords portFarLodTable.records
#define portFarLodMaxes portFarLodTable.maxes

/* How many children the walker can push from `node`: every frame of an animation, both
 * children of a zone, every child of a select, every record of an LOD (at most 32). The scan
 * recurses through these with small frames: the game's stack is short. */
static int PORT_TEXT portFarLodKids(unsigned char *node)
{
    int n;

    switch (node[0]) {
    case 1:
        return node[20];
    case 2:
        n = FAR_LOD_W(node, 16);
        return n < 0 ? 0 : n > 32 ? 32 : n;
    case 3:
        return node[11];
    case 4:
        return node[18];
    case 5:
        return node[38];
    case 9:
        return node[6];
    case 11:
        return 2;
    case 12:
        return *(unsigned short *)(node + 6);
    }
    return 0;
}

/* Child i of `node` (an LOD record's child for opcode 2; outside, inside for opcode 11). */
static unsigned char PORT_TEXT *portFarLodKid(unsigned char *node, int i)
{
    switch (node[0]) {
    case 1:
        return FAR_LOD_P(node, 24 + i * 4);
    case 2:
        return FAR_LOD_P(FAR_LOD_P(node, 20 + i * 4), 8);
    case 3:
        return FAR_LOD_P(node, 32 + i * 4);
    case 4:
        return FAR_LOD_P(node, 20 + i * 4);
    case 5:
        return FAR_LOD_P(node, 40 + i * 4);
    case 9:
        return FAR_LOD_P(node, 16 + i * 4);
    case 11:
    case 12:
        return FAR_LOD_P(node, 8 + i * 4);
    }
    return 0;
}

/* Does the walker reach a mesh (opcode 0) below `node`, whatever the run-time state
 * (culling, hidden flags, animation frame, zone, select)? Records that only hold hit
 * volumes (opcode 8) draw nothing. */
static int PORT_TEXT portFarLodDraws(unsigned char *node, int depth)
{
    int n, i;

    if (node == 0 || depth > FAR_LOD_MAX_DEPTH)
        return 0;
    if (node[0] == 0)
        return 1;
    n = portFarLodKids(node);
    for (i = 0; i < n; i++)
        if (portFarLodDraws(portFarLodKid(node, i), depth + 1))
            return 1;
    return 0;
}

/* Request 90 (RENDER_ANSWERS.md R6.1): 1 when every mesh the walker can reach below `node` holds only camera-facing
 * beams (primitive kind 1) and there is one, -1 when another mesh is reached, 0 when none is. */
static int PORT_TEXT portFarLodBeams(unsigned char *node, int depth)
{
    unsigned char *p;
    int n, i, k, r;

    if (node == 0 || depth > FAR_LOD_MAX_DEPTH)
        return 0;
    if (node[0] == 0) {
        p = FAR_LOD_P(node, 12);
        n = FAR_LOD_W(node, 16);
        if (n <= 0)
            return -1;
        for (i = 0; i < n; i++) {
            if (((FAR_LOD_W(p, 12) >> 16) & 7) != 1)
                return -1;
            p += p[2] * 4;
        }
        return 1;
    }
    r = 0;
    n = portFarLodKids(node);
    for (i = 0; i < n; i++) {
        k = portFarLodBeams(portFarLodKid(node, i), depth + 1);
        if (k < 0)
            return -1;
        if (k > 0)
            r = 1;
    }
    return r;
}

/* Request 90: does record i of LOD node `lod` have a coarser version, a drawing record of the node starting at or
 * beyond its max? (Holland's grass: a block's 16 cells [0, 150) against the block's one quad [150, ..).) */
static int PORT_TEXT portFarLodCoarser(unsigned char *lod, int i)
{
    unsigned int *r, *q;
    int n, j;

    r = (unsigned int *)FAR_LOD_P(lod, 20 + i * 4);
    n = portFarLodKids(lod);
    for (j = 0; j < n; j++) {
        q = (unsigned int *)FAR_LOD_P(lod, 20 + j * 4);
        if (j != i && q[1] >= r[0] && portFarLodDraws((unsigned char *)q[2], 0))
            return 1;
    }
    return 0;
}

/* The record of LOD node `lod` with the largest max (the first of equal ones): the one the
 * walker draws past every record once that max is unbounded. */
static unsigned int PORT_TEXT *portFarLodTop(unsigned char *lod)
{
    unsigned int *top, *r;
    int n, i;

    top = 0;
    n = portFarLodKids(lod);
    for (i = 0; i < n; i++) {
        r = (unsigned int *)FAR_LOD_P(lod, 20 + i * 4);
        if (top == 0 || r[0] > top[0])
            top = r;
    }
    return top;
}

/* Request 46 item 6 (FAR_LOD.md 8): list every record of LOD node `lod` with its retail band, once, for
 * portFarLodNearScale (beyond FAR_LOD_NEAR_RECORDS a record keeps its retail band). */
static void PORT_TEXT portFarLodNearNote(unsigned char *lod)
{
    unsigned int *r;
    int n, i, k;

    n = portFarLodKids(lod);
    for (i = 0; i < n; i++) {
        r = (unsigned int *)FAR_LOD_P(lod, 20 + i * 4);
        for (k = 0; k < portFarLodNear.count; k++)
            if (FAR_LOD_REC(portFarLodNear.records[k]) == r)
                break;
        if (k < portFarLodNear.count || portFarLodNear.count >= FAR_LOD_NEAR_RECORDS)
            continue;
        portFarLodNear.records[k] = r;
        portFarLodNear.maxes[k] = r[0];
        portFarLodNear.mins[k] = r[1];
        portFarLodNear.count = k + 1;
    }
}

/* A band bound (d2) at `s` times its distance: d2 * s * s, an unbounded max (0xFFFFFFFF) stays, a product past the
 * 32 bits saturates just below it. */
static unsigned int PORT_TEXT portFarLodNearD2(unsigned int d2, unsigned int s)
{
    if (d2 == 0xFFFFFFFF)
        return d2;
    if (d2 > 0xFFFFFFFEu / (s * s))
        return 0xFFFFFFFE;
    return d2 * s * s;
}

/* The scale in force: 1..16 (portFarLodNearScale 0 and 1 = retail's 1; request 98: the view's budget scale while it
 * is in force). */
static unsigned int PORT_TEXT portFarLodNearFactor(void)
{
    int s;

    s = portFarLodBudgetState.current != 0 ? portFarLodBudgetState.current : portFarLodNearScale;
    return s <= 1 ? 1 : s > 16 ? 16 : s;
}

/* Does a drawing record of the set start at or beyond `max`? The set is the LOD children of
 * `parent`, a node whose children the walker all draws (opcodes 1, 4, 5, 12), or `lod` alone
 * (parent 0: a root, or the child of an LOD, animation, select or zone node, of which the
 * walker draws one). Split LODs keep each band in a node of their own ([0,200) | [200,500) |
 * [500,1000) under one group, as in most car files): only the farthest band may stay. */
static int PORT_TEXT portFarLodLater(unsigned char *parent, unsigned char *lod, unsigned int max)
{
    unsigned char *l;
    unsigned int *r;
    int n, m, i, k;

    n = parent != 0 ? portFarLodKids(parent) : 1;
    for (i = 0; i < n; i++) {
        l = parent != 0 ? portFarLodKid(parent, i) : lod;
        if (l == 0 || l[0] != 2)
            continue;
        m = portFarLodKids(l);
        for (k = 0; k < m; k++) {
            r = (unsigned int *)FAR_LOD_P(l, 20 + k * 4);
            if (r[1] >= max && portFarLodDraws((unsigned char *)r[2], 0))
                return 1;
        }
    }
    return 0;
}

/* Request 91 (FAR_LOD.md 13): set bit 1 on record `r` in portFarLodNear (`fine`: it must be listed: every record of
 * an LOD the scan reached is) or in portFarLodTable (a coarser version: listed now when it is not, room allowing). */
static void PORT_TEXT portFarLodFlag(unsigned int *r, int fine)
{
    unsigned int **recs;
    int n, k;

    recs = fine ? portFarLodNear.records : portFarLodRecords;
    n = fine ? portFarLodNear.count : portFarLodCount;
    for (k = 0; k < n; k++)
        if (FAR_LOD_REC(recs[k]) == r) {
            recs[k] = (unsigned int *)((unsigned int)recs[k] | FAR_LOD_FINE);
            return;
        }
    if (!fine && portFarLodCount < FAR_LOD_MAX_RECORDS) {
        portFarLodRecords[portFarLodCount] = (unsigned int *)((unsigned int)r | FAR_LOD_FINE);
        portFarLodMaxes[portFarLodCount] = r[0];
        portFarLodCount++;
    }
}

/* Request 91: record i of LOD node `lod` leads to a beam field's finer level: fine, and every coarser version of it
 * (a drawing record starting at or beyond its max, as portFarLodCoarser) drawn at no distance. */
static void PORT_TEXT portFarLodFineRecord(unsigned char *lod, int i)
{
    unsigned int *r, *q;
    int n, j;

    r = (unsigned int *)FAR_LOD_P(lod, 20 + i * 4);
    portFarLodFlag(r, 1);
    n = portFarLodKids(lod);
    for (j = 0; j < n; j++) {
        q = (unsigned int *)FAR_LOD_P(lod, 20 + j * 4);
        if (j != i && q[1] >= r[0] && portFarLodDraws((unsigned char *)q[2], 0))
            portFarLodFlag(q, 0);
    }
}

/* Request 91: the beam LOD `lod` (Holland's tall grass) under the walk's `path`: its records and every record of the
 * path are fine, their coarser versions off. With portFarLod set the field then draws its finest level (the cells and
 * their beams) at every distance, as near the car. */
static void PORT_TEXT portFarLodFineField(unsigned char *lod, struct PortFarLodPath *path)
{
    int n, i;

    if (path->last_lod == lod && path->last_n == path->n) {
        for (i = 0; i < path->n; i++)
            if (path->last_lods[i] != path->lods[i] || path->last_recs[i] != path->recs[i])
                break;
        if (i == path->n)
            return;
    }
    portFarLodNearNote(lod);
    n = portFarLodKids(lod);
    for (i = 0; i < n; i++)
        portFarLodFlag((unsigned int *)FAR_LOD_P(lod, 20 + i * 4), 1);
    for (i = 0; i < path->n; i++) {
        portFarLodFineRecord(path->lods[i], path->recs[i]);
        path->last_lods[i] = path->lods[i];
        path->last_recs[i] = path->recs[i];
    }
    path->last_lod = lod;
    path->last_n = path->n;
}

/* LOD node `lod` of the set of `parent`: list its top record when it is finite, draws, and no
 * band of the set lies beyond it. Request 90: `coarse` is set when the nearest LOD above `lod` reached it through a
 * record with a coarser version; a beam-only LOD there (Holland's tall grass) is listed for neither the scale nor the
 * extension and keeps its retail band: else its beams fill every near tile out to the outer switch, in tile-shaped
 * patches. */
static void PORT_TEXT portFarLodNode(unsigned char *lod, unsigned char *parent, int coarse,
                                     struct PortFarLodPath *path)
{
    unsigned int *top;
    int k;

    if (coarse && portFarLodBeams(lod, 0) > 0) {
        portFarLodFineField(lod, path);     /* request 91: listed for the scale only, and its field's finest level */
        return;
    }
    portFarLodNearNote(lod);    /* request 46 item 6: every record, for portFarLodNearScale */
    top = portFarLodTop(lod);
    if (top == 0 || top[0] == 0xFFFFFFFF || !portFarLodDraws((unsigned char *)top[2], 0))
        return;
    if (portFarLodLater(parent, lod, top[0]))
        return;
    for (k = 0; k < portFarLodCount; k++)
        if (FAR_LOD_REC(portFarLodRecords[k]) == top)
            return;
    if (portFarLodCount < FAR_LOD_MAX_RECORDS) {
        portFarLodRecords[portFarLodCount] = top;
        portFarLodMaxes[portFarLodCount] = top[0];
        portFarLodCount++;
    }
}

/* `coarse`: request 90, as for portFarLodNode (an LOD node sets it again for each of its records). `path`: request
 * 91, the walk's LOD records with a coarser version down to `node`. */
static void PORT_TEXT portFarLodWalk(unsigned char *node, int depth, int coarse, struct PortFarLodPath *path)
{
    unsigned char *c;
    int n, i, all, k, np;

    if (node == 0 || depth > FAR_LOD_MAX_DEPTH)
        return;
    n = portFarLodKids(node);
    all = node[0] == 1 || node[0] == 4 || node[0] == 5 || node[0] == 12;
    np = path->n;
    for (i = 0; i < n; i++) {
        c = portFarLodKid(node, i);
        k = node[0] == 2 ? portFarLodCoarser(node, i) : coarse;
        path->n = np;
        if (node[0] == 2 && k) {
            path->lods[np] = node;
            path->recs[np] = i;
            path->n = np + 1;
        }
        if (c != 0 && c[0] == 2)
            portFarLodNode(c, all ? node : 0, k, path);
        portFarLodWalk(c, depth + 1, k, path);
    }
    path->n = np;
}

/* One root of a model file, already relocated (assetInstallImage) or built for its address
 * (the level file at 0x80019C40). */
void PORT_TEXT portFarLodScan(void *root)
{
    struct PortFarLodPath path;
    unsigned char *r;

    path.n = 0;
    path.last_n = 0;
    path.last_lod = 0;
    r = (unsigned char *)root;
    if (r != 0 && r[0] == 2)
        portFarLodNode(r, 0, 0, &path);
    portFarLodWalk(r, 0, 0, &path);
}

/* The level file (SANTA.md 1.2): root count at 0x80019C58, roots from 0x80019C5C; a zero
 * root ends the list, as in worldLoadStartObjects. Forgets the previous match first. */
void PORT_TEXT portFarLodScanLevel(void)
{
    unsigned int count, i;
    void **roots;

    portFarLodCount = 0;
    portFarLodNear.count = 0;   /* request 46 item 6 */
    count = *(unsigned int *)0x80019C58;
    roots = (void **)0x80019C5C;
    for (i = 0; i < count && roots[i] != 0; i++)
        portFarLodScan(roots[i]);
}

/* Request 46 item 6: with portFarLodNearScale s (2..16) every listed record still holding its band of the scan
 * gets its bounds at s times their distance (d2 * s * s), first; then, with portFarLod set, each listed far record
 * whose max still holds the value seen at the scan (retail's, or that value scaled) is extended (the model memory
 * is reused between matches: a record that changed is left alone). Request 91 (FAR_LOD.md 13): with portFarLod set,
 * a beam field's fine records (bit 1 in portFarLodNear) are [0, ..) instead of scaled, and their coarser versions
 * (bit 1 in portFarLodTable) get max 0, drawn at no distance, instead of extended. Returns non-zero when it changed
 * a record. */
int PORT_TEXT portFarLodBegin(void)
{
    unsigned int *r;
    unsigned int s, f;
    int i, done;

    done = 0;
    s = portFarLodNearFactor();
    if (s > 1 || portFarLod != 0)
        for (i = 0; i < portFarLodNear.count; i++) {
            r = FAR_LOD_REC(portFarLodNear.records[i]);
            f = (unsigned int)portFarLodNear.records[i] & FAR_LOD_FINE;
            if (r[0] != portFarLodNear.maxes[i] || r[1] != portFarLodNear.mins[i])
                continue;
            if (f && portFarLod != 0) {
                r[0] = 0xFFFFFFFF;
                r[1] = 0;
            } else if (s > 1) {
                r[0] = portFarLodNearD2(r[0], s);
                r[1] = portFarLodNearD2(r[1], s);
            } else
                continue;
            portFarLodNear.records[i] = (unsigned int *)((unsigned int)r | f | 1);
            done = 1;
        }
    if (portFarLod == 0)
        return done;
    for (i = 0; i < portFarLodCount; i++) {
        r = FAR_LOD_REC(portFarLodRecords[i]);
        f = (unsigned int)portFarLodRecords[i] & FAR_LOD_FINE;
        if (r[0] == portFarLodMaxes[i] || (s > 1 && r[0] == portFarLodNearD2(portFarLodMaxes[i], s))) {
            r[0] = f ? 0 : 0xFFFFFFFF;
            portFarLodRecords[i] = (unsigned int *)((unsigned int)r | f | 1);
            done = 1;
        }
    }
    return done;
}

#ifdef TM2_PORT_FAR_DISTANCE
/* Request 98: the budget's next step from scale s (2, 3, 4, 6, 8, 12, 16), up or down. */
static int PORT_TEXT portFarLodBudgetStep(int s, int up)
{
    if (up)
        return s < 2 ? 2 : s < 3 ? 3 : s < 4 ? 4 : s < 6 ? 6 : s < 8 ? 8 : s < 12 ? 12 : 16;
    return s > 12 ? 12 : s > 8 ? 8 : s > 6 ? 6 : s > 4 ? 4 : s > 3 ? 3 : s > 2 ? 2 : 1;
}

/* Request 98 (FAR_LOD.md 14): renderScene, before portFarLodBegin: the band scale of view `view` by its previous
 * frame in portRenderStats (port_far_lod.h portFarLodBudget), kept in portFarLodBudgetState for portFarLodNearFactor
 * until the next view; returns it, 0 when the budget is off (portFarLodBudget 0 / 1) or the view has no stats row.
 * Drawing only; no rand(). */
int PORT_TEXT portFarLodBudgetView(int view)
{
    struct TM2PortFarLodBudget *b = &portFarLodBudgetState;
    struct TM2PortRenderView *r;
    int row, lo, hi, s, full, room;

    b->current = 0;
    if (portFarLodBudget <= 1)
        return 0;
    if (viewGetMode(view) == 4)
        row = TM2_RENDER_STATS_MIRROR;
    else if (view >= 0 && view < TM2_RENDER_STATS_VIEWS)
        row = view;
    else
        return 0;
    lo = portFarLodNearScale < 1 ? 1 : portFarLodNearScale > 16 ? 16 : portFarLodNearScale;
    hi = portFarLodBudget > 16 ? 16 : portFarLodBudget < lo ? lo : portFarLodBudget;
    s = b->scale[row];
    if (s < lo || s > hi) {
        s = lo;
        b->hold[row] = 0;
    }
    r = &portRenderStats.view[row];
    if (r->frames > 0 && r->mesh_cap > 0 && r->packet_cap > 0) {
        full = (r->caps & (TM2_RENDER_CAP_MESHES | TM2_RENDER_CAP_PACKETS)) != 0 ||
               r->meshes > r->mesh_cap - (r->mesh_cap >> 3) || r->packet > r->packet_cap - (r->packet_cap >> 3);
        room = r->meshes < (r->mesh_cap >> 1) && r->packet < (r->packet_cap >> 1);
        if (full && s > lo) {
            s = portFarLodBudgetStep(s, 0);
            if (s < lo)
                s = lo;
            b->hold[row] = TM2_FAR_LOD_BUDGET_DOWN;
        } else if (b->hold[row] != 0)
            b->hold[row]--;
        else if (room && s < hi) {
            s = portFarLodBudgetStep(s, 1);
            if (s > hi)
                s = hi;
            b->hold[row] = TM2_FAR_LOD_BUDGET_UP;
        }
    }
    b->scale[row] = (unsigned char)s;
    b->current = s;
    return s;
}
#endif

/* Put back the retail band of every record portFarLodBegin scaled and the retail max of every record it
 * extended. */
void PORT_TEXT portFarLodEnd(void)
{
    unsigned int *r;
    int i;

    for (i = 0; i < portFarLodNear.count; i++) {
        r = portFarLodNear.records[i];
        if ((unsigned int)r & 1) {
            portFarLodNear.records[i] = (unsigned int *)((unsigned int)r & ~1u);    /* bit 1 kept: request 91 */
            r = FAR_LOD_REC(r);
            r[0] = portFarLodNear.maxes[i];
            r[1] = portFarLodNear.mins[i];
        }
    }
    for (i = 0; i < portFarLodCount; i++) {
        r = portFarLodRecords[i];
        if ((unsigned int)r & 1) {
            portFarLodRecords[i] = (unsigned int *)((unsigned int)r & ~1u);
            r = FAR_LOD_REC(r);
            r[0] = portFarLodMaxes[i];
        }
    }
}
#endif /* TM2_PORT_FAR_LOD */

/* ---- far distance (PC port request 31, docs/FAR_DISTANCE.md) ---- */
/* The port's settings, all 0 = retail. They act only in a build with --port-define
 * TM2_PORT_FAR_DISTANCE (renderScene, rtMainLoop and the effect drawers call the functions
 * below); without it nothing reads them.
 *   portFarDistance        far distance of the normal views in world units (retail 4800,
 *                          renderFarDistanceDefault); 1..32767 (larger values act as 32767).
 *                          Non-zero also turns on the GTE range pass and makes the effect
 *                          sprites' cull and sort follow the far distance.
 *   portMeshListCap        meshes queued per view (retail 280, renderMeshList); up to
 *                          PORT_MESH_LIST_MAX, in portMeshList.
 *   portPacketBufferBytes  packet bytes per frame buffer (retail 0x17CF0 = 97,520, shared by
 *                          the views of a frame); up to PORT_PACKET_BUFFER_BYTES - 80 =
 *                          390,320 (request 47 revised), in portPacketBuffers: a buffer per
 *                          view and parity (views 1-4 and the mirror), each view's frame
 *                          records pointed at its own (portPacketBuffersApplyView). */
int portFarDistance PORT_BSS = 0;
int portMeshListCap PORT_BSS = 0;
int portPacketBufferBytes PORT_BSS = 0;
extern int portMirrorFar;   /* request 46 item 9: the mirror's far distance (defined after portCarDtParkMask) */

/* The mesh queue the walker fills when portMeshListCap is set: 32-byte draw entries (node,
 * translation x/y/z, coordinate shift, object, matrix slot, translation key). */
#define PORT_MESH_LIST_MAX 2048
int portMeshList[PORT_MESH_LIST_MAX][8] PORT_BSS = {{0}};

/* The far-distance code's state: meshList = portMeshList is this view's queue, meshBias =
 * 280 - cap (the walker's counter starts there), packetSet = bit 2 v + p: a frame record of view v
 * and parity p points at portPacketBuffers[v][p]. Then the GTE range pass's copies of the matrix slots the
 * emitter reads (rotation and light, 32 bytes each, at the same index) and a scaled copy per
 * (slot, k) that a far mesh uses; map[slot][k] = 1 + the copy's index (0 = none yet). Retail has
 * at most 201 slots (renderScene's and the walker's counter, ctx+0x08, reaches 200); with
 * TM2_PORT_RENDER_SLOTS (request 47, render_slots.h) TM2_MAX_RENDER_SLOTS (the limit + 1). The
 * sizes follow the slot arrays: TM2_MAX_RENDER_SLOTS + 1 sources and 311 more copies (retail's
 * 201 / 512; config/US/port_variant_arrays.tsv sizes the block by the same expression). */
#define PORT_FAR_SRC_SLOTS (TM2_MAX_RENDER_SLOTS + 1)
#define PORT_FAR_SLOTS (TM2_MAX_RENDER_SLOTS + 312)
struct PortFarState {
    int meshList;
    int meshBias;
    int packetSet;
    int pad;
    short map[PORT_FAR_SRC_SLOTS][4];
    int rot[PORT_FAR_SLOTS][8];
    int light[PORT_FAR_SLOTS][8];
};
struct PortFarState portFarState PORT_BSS = {0};

/* Request 61 (FAR_DISTANCE.md 2 "Sprites and beams"): the GTE range pass's copies of the far meshes that hold point
 * sprites or beams. The emitter sizes them from the scaled depth (hw, hh = prim +6 / +8 * H / SZ; a beam's half width
 * q = prim +6 * H / SZ), so in a mesh scaled by 2^-k they came out 2^k too large; a copy of the mesh (its node's first
 * PORT_FAR_SPRITE_HEAD bytes, the prims pointer at +0x0C moved to the copy of its primitives) carries those sizes
 * divided by 2^k (rounded; a beam's at least 1), the beam's half length (+8, a vertex offset) as it is. Made afresh for
 * every view (the hash's generation `gen`; a mesh drawn at two k in a view gets two copies), so a primitive the game
 * changes is copied as it is now; `data` holds this view's copies (`used` bytes), `full` counts the scaled meshes left
 * with retail's sizes because the room ran out. Request 61 addendum: a copy's head is the node's first 44 bytes and
 * then the source mesh's primitives pointer (+44), so that the C emitter (TM2_PORT_C_EMITTER) reports the precise
 * corners from the unrounded sizes (portFarSpriteSource). Drawing only (rollback_regions.tsv: snapshot and hash no). Defined after
 * portDmdImages (nothing before it moves); used only with TM2_PORT_FAR_DISTANCE. */
#define PORT_FAR_SPRITE_BYTES 131072
#define PORT_FAR_SPRITE_HASH 512
#define PORT_FAR_SPRITE_HEAD 48     /* the node's first 44 bytes, the source primitives (request 61 addendum) */
struct PortFarSpriteSlot {
    unsigned char *node;    /* the mesh node the walker queued */
    int gen;                /* the view it was looked up in (portFarSprites.gen) */
    int k;                  /* its scale shift */
    unsigned char *mesh;    /* what the emitter draws: the copy, or node (no sprite or beam, or no room) */
};
struct PortFarSprites {
    int gen;                /* views the pass ran in (1.., 0 = never) */
    int used;               /* bytes of data this view's copies use */
    int copies;             /* meshes copied this view */
    int full;               /* scaled meshes with sprites or beams left with retail's sizes this view (no room) */
    struct PortFarSpriteSlot hash[PORT_FAR_SPRITE_HASH];
    int data[PORT_FAR_SPRITE_BYTES / 4];
};
extern struct PortFarSprites portFarSprites;

/* The packet buffers: one per view (PORT_PACKET_VIEWS: views 1-4 = 0..3, the rear-view mirror =
 * PORT_PACKET_MIRROR 4, the rows of portRenderStats) and frame parity, PORT_PACKET_BUFFER_FACTOR
 * times retail's 97,600 each; the last 80 bytes are slack past the end, as in retail (the emitter
 * checks the end before each primitive and a primitive with its draw-mode packets is at most 76
 * bytes). Request 47 revised: a buffer of 4 x for every view, the same for every player count (the
 * port's worst pair at the farthest distance with near scale 4 was 387 KB for views 3-4, about
 * half of 4 x a view; FAR_DISTANCE.md 6). It was 8 x shared at first, 4 x shared since request 33-1, 8 x per
 * view pair in request 47. */
#define PORT_PACKET_BUFFER_FACTOR 4
#define PORT_PACKET_BUFFER_BYTES (PORT_PACKET_BUFFER_FACTOR * 97600)
#define PORT_PACKET_VIEWS 5
#define PORT_PACKET_MIRROR 4
int portPacketBuffers[PORT_PACKET_VIEWS][2][PORT_PACKET_BUFFER_BYTES / 4] PORT_BSS = {{{0}}};

#ifdef TM2_PORT_FAR_DISTANCE
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif

/* The renderer state in the scratchpad (engine_render_scene.c RenderState): the matrix slot
 * counter at +0x08, the mesh queue pointer at +0x20 and the mesh counter at +0x3C, which the
 * walker (renderWalkModel, asm) tests with `slti ...,280` before queuing a mesh. */
extern unsigned char *renderCtx;
#define FAR_CTX_W(o) (*(int *)(renderCtx + (o)))
extern unsigned char renderMeshList[], renderMatrixSlots[], renderObjectMatrices[];
extern int renderWorldShift;
extern unsigned long renderPacketBuffer0[], renderPacketBuffer1[];
extern void sub_800CDC54(void *db, void *list, int count, void *rot, void *light);

#define FAR_RETAIL_MESHES 280
#define FAR_GTE_RANGE 32767     /* SZ and IR1..IR3 exact: every coordinate of the mesh fits */
#define FAR_GTE_NEAR 1024       /* nearest point of a scaled mesh, in GTE units: SZ stays well
                                   above H/2 (H <= 457, viewModeTable), where the projection
                                   divide saturates */

/* renderScene, for the normal views (mode 4, the mirror, keeps its 500): the far distance. */
int PORT_TEXT portFarDistanceFar(int retailFar)
{
    int far;

    far = portFarDistance;
    if (far == 0)
        return retailFar;
    if (far < 1)
        far = 1;
    if (far > 32767)
        far = 32767;
    return far;
}

/* renderScene, for the rear-view mirror (view mode 4; request 46 item 9, PLAYTEST_ANSWERS.md 15): the far
 * distance of its level pass (retail 500) and of its objects (retail 1000), or portMirrorFar for both when set
 * (1..32767). The mirror's packets come out of the same frame buffer as view 0's, after them. */
int PORT_TEXT portFarDistanceMirror(int retailFar)
{
    int far;

    far = portMirrorFar;
    if (far <= 0)
        return retailFar;
    return far > 32767 ? 32767 : far;
}

/* renderScene, right after renderInitState: with portMeshListCap set, the walker queues into
 * portMeshList and its counter starts at 280 - cap, so its `slti ...,280` test allows `cap`
 * meshes (the walker keeps the queue pointer at ctx+0x20 and only compares the counter). The
 * caller subtracts the returned bias from the final counter. Returns 0 (retail) otherwise. */
int PORT_TEXT portFarDistanceMeshListBegin(void)
{
    int cap;

    cap = portMeshListCap;
    portFarState.meshList = 0;
    portFarState.meshBias = 0;
    if (cap == 0)
        return 0;
    if (cap < 1)
        cap = 1;
    if (cap > PORT_MESH_LIST_MAX)
        cap = PORT_MESH_LIST_MAX;
    portFarState.meshList = 1;
    portFarState.meshBias = FAR_RETAIL_MESHES - cap;
    *(int **)(renderCtx + 0x20) = &portMeshList[0][0];
    FAR_CTX_W(0x3C) = portFarState.meshBias;
    return portFarState.meshBias;
}

/* Integer square root (bit by bit). */
static unsigned int PORT_TEXT portFarIsqrt(unsigned int v)
{
    unsigned int r, b;

    r = 0;
    b = 1u << 30;
    while (b > v)
        b >>= 2;
    while (b != 0) {
        if (v >= r + b) {
            v -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
        b >>= 2;
    }
    return r;
}

/* |(x, y, z)| rounded up, without overflow for any 32-bit coordinates. */
static int PORT_TEXT portFarLength(int x, int y, int z)
{
    int m, s;
    unsigned int q;

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (z < 0)
        z = -z;
    m = x > y ? x : y;
    if (z > m)
        m = z;
    s = 0;
    while ((m >> s) > 16383)
        s++;
    x >>= s;
    y >>= s;
    z >>= s;
    q = (unsigned int)(x * x) + (unsigned int)(y * y) + (unsigned int)(z * z);
    return (int)((portFarIsqrt(q) + 2) << s);
}

/* round(v / 2^k) for k >= 1 (request 61: the scaled matrix elements and the sprite sizes; was v >> k). */
#define FAR_ROUND_SHIFT(v, k) (((v) + (1 << ((k) - 1))) >> (k))
/* A scaled entry's translation key: bit 30 set, k in bits 28..29, the entry's index below (request 61; was
 * key ^ k << 28). Every scaled entry has its own: its translation is corrected for its own mesh centre. The port reads
 * k of a queued mesh as (key >> 28) & 3 when bit 30 is set (0 = left as retail queued it). */
#define FAR_KEY_SCALED 0x40000000

/* Request 61 (FAR_DISTANCE.md 2 "Precision"): the translation of a scaled entry. The emitter projects
 * R'.(t + v) / 4096 with R' = round(R / 2^k), while the true scaled point is R.(t + v) / 2^k / 4096; the difference,
 * D.(t + v) / 2^k with D = R - 2^k R' (elements |D| <= 2^(k-1)), is the error of the matrix's 12 - k fraction bits
 * times the whole distance (t + v): up to about 0.1 x 2^k pixels (k = 3: 1.2 px), changing as the camera turns and
 * jumping when k changes (the far geometry's wiggle). The translation is moved so that the mesh centre c projects
 * exactly: R'.t_s = R'.t + D.(t + c) / 2^k, i.e. t_s = t + R'^-1 D (t + c) / 2^k = t + R^T D (t + c) / |R|^2 (R' ~
 * R / 2^k; |R|^2 = 4096^2 for a rotation, here the rows' mean squared length). Left is D.(v - c) / 2^k: the mesh's
 * own extent instead of its distance. 32-bit arithmetic: u = D.(t + c) is split into u >> 12 and u & 0xFFF. A matrix
 * with an element past +-8192 (an object scaled more than 2 x) keeps its translation. */
static void PORT_TEXT portFarTranslate(int *e, unsigned char *node, short *r, short *rs, int k)
{
    int d[9], p[3], u[3], i, j, a, b, n2, q;

    n2 = 0;
    for (i = 0; i < 9; i++) {
        if (r[i] > 8192 || r[i] < -8192)
            return;
        d[i] = r[i] - rs[i] * (1 << k);
        n2 += r[i] * r[i];
    }
    q = (n2 / 3) >> 12;
    if (q <= 0)
        return;
    for (i = 0; i < 3; i++)
        p[i] = e[1 + i] + *(int *)(node + 28 + 4 * i);
    for (i = 0; i < 3; i++)
        u[i] = d[3 * i] * p[0] + d[3 * i + 1] * p[1] + d[3 * i + 2] * p[2];
    for (j = 0; j < 3; j++) {
        a = r[j] * (u[0] >> 12) + r[3 + j] * (u[1] >> 12) + r[6 + j] * (u[2] >> 12);
        b = r[j] * (u[0] & 0xFFF) + r[3 + j] * (u[1] & 0xFFF) + r[6 + j] * (u[2] & 0xFFF);
        a += b >> 12;
        if (a >= 0)
            e[1 + j] += (a + (q >> 1)) / q;
        else
            e[1 + j] -= (-a + (q >> 1)) / q;
    }
}

/* Request 61 (FAR_DISTANCE.md 2 "Sprites and beams"): the mesh the emitter draws for a node scaled by 2^-k: the node
 * itself when it has no sprite or beam (primitive types other than 3 and 4), else this view's copy with the sizes
 * divided by 2^k (struct PortFarSprites), so that w * H / SZ on the scaled depth gives retail's w * H / (SZ 2^k). A
 * sprite's +6 / +8 (half sizes) and a beam's +6 (half width; at least 1, so that its clamp 1..2w keeps a width) are
 * rounded; a beam's +8 is its half length in vertex units, which the scaled matrix already scales. The emitter's
 * clamps are retail's: a sprite 1..500, a beam 1..2w (2w only below SZ = H / 2, which a scaled mesh, at least
 * FAR_GTE_NEAR deep, never reaches). Out of room: the node (retail's sizes, as before request 61). */
static unsigned char *PORT_TEXT portFarSpriteMesh(unsigned char *node, int k)
{
    struct PortFarSpriteSlot *s;
    unsigned char *prims, *p, *dst;
    int *src, *out;
    int n, i, bytes, size, type, h, probe, w, sprites;

    h = (int)((((unsigned int)node >> 3) ^ ((unsigned int)node >> 12) ^ (unsigned int)k) &
        (PORT_FAR_SPRITE_HASH - 1));
    s = 0;
    for (probe = 0; probe < 8; probe++) {
        s = &portFarSprites.hash[(h + probe) & (PORT_FAR_SPRITE_HASH - 1)];
        if (s->gen != portFarSprites.gen)
            break;                  /* free in this view */
        if (s->node == node && s->k == k)
            return s->mesh;
    }
    if (probe == 8)
        s = 0;                      /* no free slot nearby: not cached */
    prims = *(unsigned char **)(node + 12);
    n = *(int *)(node + 16);
    bytes = 0;
    sprites = 0;
    p = prims;
    for (i = 0; i < n; i++) {
        size = p[2] * 4;
        if (size == 0) {
            sprites = 0;            /* not a primitive list the emitter could walk either: leave it */
            break;
        }
        type = (*(unsigned int *)(p + 12) >> 16) & 7;
        if (type != 3 && type != 4)
            sprites++;
        bytes += size;
        p += size;
    }
    dst = node;
    if (sprites != 0) {
        if (portFarSprites.used + PORT_FAR_SPRITE_HEAD + bytes <= PORT_FAR_SPRITE_BYTES) {
            dst = (unsigned char *)portFarSprites.data + portFarSprites.used;
            portFarSprites.used += PORT_FAR_SPRITE_HEAD + bytes;
            portFarSprites.copies++;
            src = (int *)node;
            out = (int *)dst;
            for (i = 0; i < 44 / 4; i++)
                out[i] = src[i];
            *(unsigned char **)(dst + 12) = dst + PORT_FAR_SPRITE_HEAD;
            *(unsigned char **)(dst + 44) = prims;      /* the source's primitives (portFarSpriteSource) */
            src = (int *)prims;
            out = (int *)(dst + PORT_FAR_SPRITE_HEAD);
            for (i = 0; i < bytes / 4; i++)
                out[i] = src[i];
            p = dst + PORT_FAR_SPRITE_HEAD;
            for (i = 0; i < n; i++) {
                type = (*(unsigned int *)(p + 12) >> 16) & 7;
                if (type != 3 && type != 4) {
                    w = *(short *)(p + 6);
                    if (type == 1 && w > 0 && FAR_ROUND_SHIFT(w, k) < 1)
                        *(short *)(p + 6) = 1;
                    else
                        *(short *)(p + 6) = FAR_ROUND_SHIFT(w, k);
                    if (type != 1)
                        *(short *)(p + 8) = FAR_ROUND_SHIFT(*(short *)(p + 8), k);
                }
                p += p[2] * 4;
            }
        } else {
            portFarSprites.full++;
        }
    }
    if (s != 0) {
        s->node = node;
        s->gen = portFarSprites.gen;
        s->k = k;
        s->mesh = dst;
    }
    return dst;
}

/* Request 61 addendum (TM2_PORT_C_EMITTER, port_c_emitter.h): the primitive of the source mesh at the place of `prim`
 * when `mesh` is one of this view's copies (the copy's +44), else `prim`: the C emitter's precise corners take the
 * sizes before their rounding / 2^k. */
unsigned char *PORT_TEXT portFarSpriteSource(unsigned char *mesh, unsigned char *prim)
{
    unsigned char *data = (unsigned char *)portFarSprites.data;

    if (mesh < data || mesh >= data + PORT_FAR_SPRITE_BYTES)
        return prim;
    return *(unsigned char **)(mesh + 44) + (prim - *(unsigned char **)(mesh + 12));
}

/* The GTE range pass. The emitter (sub_800CDC54, asm) projects each mesh with RTPT/RTPS: SZ is
 * 16 bits (0..65535) and IR1/IR2, which the projection uses, are clamped to +-32767, in the
 * mesh's coordinate units (world << shift, shift 3 for every arena). Beyond 4,095 world units
 * a vertex can leave that range, beyond 8,191 every vertex does: it projects to the wrong
 * place and its depth saturates (z = 65535 >> 3 = 8191, one OT slot for everything).
 * A projection is unchanged when the vertex and the translation are both scaled by 2^-k, so
 * a far mesh gets a copy of its rotation slot with the 9 elements / 2^k (the emitter computes
 * the translation from that matrix), its shift lowered by k (the emitter's z = SZ >> shift
 * stays in world units, so its OT mapping is unchanged) and a new translation key (the
 * emitter caches the rotated translation by that key). k is the smallest that keeps the
 * mesh's bounding sphere inside +-32767 GTE units, at most the mesh's shift, and 0 when the
 * sphere's nearest point would come within FAR_GTE_NEAR GTE units of the eye (a mesh
 * around the camera keeps retail's precision and projection). Meshes that need no scaling
 * (the whole retail 4,800 range, every mesh within 4,095 units) are left exactly as retail
 * queued them. Request 61: the elements are rounded, the translation is corrected for that rounding
 * (portFarTranslate), the mesh is drawn from a copy with its sprites' and beams' sizes / 2^k when it has any
 * (portFarSpriteMesh), and the key is the entry's own (FAR_KEY_SCALED). Returns the slot arrays the emitter
 * must read. */
static void PORT_TEXT portFarDistanceScale(int (*list)[8], int count, unsigned char **rot,
    unsigned char **light)
{
    int i, k, n, len, big, near, slot, copy, nslots, used, j;
    int *e, *src, *dst;
    unsigned char *node;
    short *m;

    nslots = FAR_CTX_W(0x08) + 1;
    if (nslots > PORT_FAR_SRC_SLOTS)
        nslots = PORT_FAR_SRC_SLOTS;
    used = 0;
    if (++portFarSprites.gen == 0) {
        /* wrapped: forget every slot */
        for (j = 0; j < PORT_FAR_SPRITE_HASH; j++)
            portFarSprites.hash[j].gen = 0;
        portFarSprites.gen = 1;
    }
    portFarSprites.used = 0;
    portFarSprites.copies = 0;
    portFarSprites.full = 0;
    for (i = 0; i < count; i++) {
        e = list[i];
        node = (unsigned char *)e[0];
        n = e[4];
        slot = e[6];
        if (n <= 0 || slot < 0 || slot >= nslots)
            continue;
        len = portFarLength(*(int *)(node + 28) + e[1], *(int *)(node + 32) + e[2],
            *(int *)(node + 36) + e[3]);
        big = len + *(int *)(node + 40);
        near = len - *(int *)(node + 40);
        if (big <= FAR_GTE_RANGE)
            continue;
        k = 1;
        while (k < n && (big >> k) > FAR_GTE_RANGE)
            k++;
        while (k > 0 && (near >> k) < FAR_GTE_NEAR)
            k--;
        if (k == 0)
            continue;
        if (used == 0) {
            /* first far mesh of the view: the retail slots, at their own indices */
            for (j = 0; j < nslots; j++) {
                src = (int *)(renderMatrixSlots + j * 32);
                dst = portFarState.rot[j];
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
                dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
                src = (int *)(renderObjectMatrices + j * 32);
                dst = portFarState.light[j];
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
                dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
                portFarState.map[j][0] = portFarState.map[j][1] = 0;
                portFarState.map[j][2] = portFarState.map[j][3] = 0;
            }
            used = nslots;
        }
        if (k > 3)
            k = 3;
        copy = portFarState.map[slot][k] - 1;
        if (copy < 0) {
            if (used >= PORT_FAR_SLOTS)
                continue;           /* out of copies: this mesh stays as retail queued it */
            copy = used++;
            portFarState.map[slot][k] = copy + 1;
            src = portFarState.rot[slot];
            dst = portFarState.rot[copy];
            for (j = 0; j < 8; j++)
                dst[j] = src[j];
            m = (short *)dst;
            for (j = 0; j < 9; j++)
                m[j] = FAR_ROUND_SHIFT(m[j], k);    /* request 61: rounded (was m >> k) */
            src = portFarState.light[slot];
            dst = portFarState.light[copy];
            for (j = 0; j < 8; j++)
                dst[j] = src[j];
        }
        /* request 61: the translation corrected for the scaled matrix's rounding, the sprites' and beams' sizes
           divided by 2^k, a key of the entry's own */
        portFarTranslate(e, node, (short *)portFarState.rot[slot], (short *)portFarState.rot[copy], k);
        e[0] = (int)portFarSpriteMesh(node, k);
        e[4] = n - k;
        e[6] = copy;
        e[7] = FAR_KEY_SCALED | (k << 28) | i;
    }
    if (used != 0) {
        *rot = (unsigned char *)portFarState.rot;
        *light = (unsigned char *)portFarState.light;
    } else {
        *rot = renderMatrixSlots;
        *light = renderObjectMatrices;
    }
}

/* renderScene, in place of its emitter call when the port changed something: the queue
 * (renderMeshList or portMeshList), the GTE range pass when portFarDistance is set, then the
 * retail emitter. */
void PORT_TEXT portFarDistanceEmit(void *db, int count)
{
    unsigned char *list, *rot, *light;

    list = portFarState.meshList ? (unsigned char *)portMeshList : renderMeshList;
    rot = renderMatrixSlots;
    light = renderObjectMatrices;
    if (portFarDistance != 0)
        portFarDistanceScale((int (*)[8])list, count, &rot, &light);
    sub_800CDC54(db, list, count, rot, light);
}

/* The OT index of a depth in world units, as the emitter maps it (renderCompressDepth,
 * 8014D368): linear to 1024, then slopes 1/4, 1/8, 1/32. */
static int PORT_TEXT portFarCompress(int z)
{
    if (z >= 1024) {
        if (z < 4096)
            z = ((z - 1024) >> 2) + 1024;
        else if (z < 16384)
            z = ((z - 4096) >> 3) + 1792;
        else
            z = ((z - 16384) >> 5) + 3328;
    }
    if (z > 4094)
        z = 4094;
    return z;
}

/* The effect drawers (fxSpritesDraw, fxImpactParticlesDraw, streaksDraw) cull camera depth z
 * (position units, world << 3) at 32000, about 4,000 world units, and sort at OT index z >> 3,
 * linear. With portFarDistance set the cull follows the far distance and the index is the
 * emitter's mapping, so a sprite sorts with the geometry around it at any distance (retail's
 * linear index sorts a sprite past 1,024 units behind geometry that is farther than it). */
int PORT_TEXT portFarDistanceFxLimit(void)
{
    if (portFarDistance == 0)
        return 32000;
    return portFarDistanceFar(4800) << 3;
}

int PORT_TEXT portFarDistanceFxOtz(int z)
{
    if (portFarDistance == 0)
        return z >> 3;
    return portFarCompress(z >> 3);
}

/* fxDebrisDraw builds each debris vertex as model vertex + rotated camera-relative offset in a
 * 16-bit SVECTOR: past 32767 position units (4,095 world units) it wraps and the piece is drawn
 * across the screen. With portFarDistance set such a piece is skipped instead (returns 0). */
int PORT_TEXT portFarDistanceDebrisFits(int *tr)
{
    int i;

    if (portFarDistance == 0)
        return 1;
    for (i = 0; i < 12; i++)
        if (tr[i] > 30000 || tr[i] < -30000)
            return 0;
    return 1;
}

/* One frame record (0x80 bytes) of view `view` (0..3 = views 1-4, PORT_PACKET_MIRROR 4 = the
 * rear-view mirror) and frame parity `parity` (0 / 1: the record's place in its view's pair) at
 * that view's own buffer portPacketBuffers[view][parity] with portPacketBufferBytes (at most
 * 390,320), or back at retail's buffers (renderPacketBuffer0/1 by parity, 0x17CF0 bytes) once it
 * is 0 again. At 0 from the start it writes nothing. It writes the record's start (+4) and end
 * (+0x0C), not its cursor (+8): the frame being drawn by the GPU keeps its packets where they
 * are, only the next frame's cursor moves. Another view number does nothing. Request 47 revised:
 * the port calls it for its own records (views 3-4, view 2 / 3 here) at the top of each frame. */
void PORT_TEXT portPacketBuffersApplyView(unsigned char *record, int view, int parity)
{
    unsigned char *start, *end;
    int bytes, bit;

    if (view < 0 || view >= PORT_PACKET_VIEWS)
        return;
    parity &= 1;
    bit = 1 << (view * 2 + parity);
    bytes = portPacketBufferBytes;
    if (bytes <= 0 && (portFarState.packetSet & bit) == 0)
        return;
    if (bytes > PORT_PACKET_BUFFER_BYTES - 80)
        bytes = PORT_PACKET_BUFFER_BYTES - 80;
    if (bytes > 0) {
        start = (unsigned char *)portPacketBuffers[view][parity];
        end = start + (bytes & ~3);
        portFarState.packetSet |= bit;
    } else {
        start = (unsigned char *)(parity ? renderPacketBuffer1 : renderPacketBuffer0);
        end = start + 0x17CF0;
        portFarState.packetSet &= ~bit;
    }
    *(unsigned char **)(record + 4) = start;
    *(unsigned char **)(record + 12) = end;
}

/* rtMainLoop, at the top of each frame, before it picks the frame's buffer: the six frame
 * records (renderView0Frames .. renderMirrorFrameB, even = parity 0, odd = parity 1): records 0-1
 * view 1, 2-3 view 2, 4-5 the mirror, each at its own buffers. */
void PORT_TEXT portPacketBuffersApply(unsigned char *records, int count)
{
    int i, view;

    for (i = 0; i < count; i++) {
        view = i >> 1;
        if (view == 2)
            view = PORT_PACKET_MIRROR;
        portPacketBuffersApplyView(records + i * 0x80, view, i & 1);
    }
}

/* Request 47's call, kept for the port: set 0 = portPacketBuffersApply (views 1-2 and the
 * mirror), set 1 = the port's records of views 3-4 (records 0-1 view 3, 2-3 view 4), each record
 * at its own view's buffers now. Another set number does nothing. */
void PORT_TEXT portPacketBuffersApplySet(unsigned char *records, int count, int set)
{
    int i;

    if (set == 0)
        portPacketBuffersApply(records, count);
    else if (set == 1)
        for (i = 0; i < count; i++)
            portPacketBuffersApplyView(records + i * 0x80, 2 + (i >> 1), i & 1);
}

/* rtMainLoop, before view 2's or the mirror's drawing (retail: `record->field_8 = view 1's
 * field_8`, the frame's packets go on in one buffer): the record's packet cursor. When the record
 * starts where `before` (the view drawn before it) starts, the views share a buffer (retail's,
 * portPacketBufferBytes 0) and the cursor goes on from `before`'s; else the record has a buffer
 * of its own and the cursor is its start. The port uses the same rule for its views 3-4 and its
 * redraws. */
unsigned char *PORT_TEXT portPacketBufferCursor(unsigned char *record, unsigned char *before)
{
    unsigned char *start;

    start = *(unsigned char **)(record + 4);
    if (start == *(unsigned char **)(before + 4))
        return *(unsigned char **)(before + 8);
    return start;
}

/* ---- the render capacity counter (docs/RENDER_ANSWERS.md "Measuring the render caps") ---- */
/* renderScene, after the walks and before the emitter (which saves a register over ctx+0x08 and reuses ctx+0x3C):
 * the view's row (mode 4 = the mirror's row 4, views 0..3 their own, others none), its matrix slots (ctx+0x08)
 * and its meshes (renderCurrentOt: the walker's counter minus the port's bias) against the caps in force, and the
 * packet cursor before the emitter. Reads only; writes only portRenderStats. */
extern int viewGetMode(int view);
extern int renderCurrentOt;
void PORT_TEXT portRenderStatsWalked(void *db, int view)
{
    struct TM2PortRenderView *r;
    int *w;
    int i, n, cap, caps;

    if (portRenderStats.reset) {
        w = (int *)portRenderStats.view;
        for (i = 0; i < (int)(sizeof(portRenderStats.view) / sizeof(int)); i++)
            w[i] = 0;
        portRenderStats.reset = 0;
    }
    portRenderStats.row = -1;
    if (viewGetMode(view) == 4)
        i = TM2_RENDER_STATS_MIRROR;
    else if (view >= 0 && view < TM2_RENDER_STATS_VIEWS)
        i = view;
    else
        return;
    portRenderStats.row = i;
    r = &portRenderStats.view[i];
    r->frames++;
    caps = 0;
    n = FAR_CTX_W(0x08);
    r->slots = n;
    if (n > r->slots_max)
        r->slots_max = n;
    if (n >= TM2_RENDER_SLOTS_NODES)
        caps |= TM2_RENDER_CAP_NODES;
    if (n >= TM2_RENDER_SLOTS_MAX)
        caps |= TM2_RENDER_CAP_SLOTS;
    n = renderCurrentOt;
    cap = FAR_RETAIL_MESHES - portFarState.meshBias;
    r->meshes = n;
    r->mesh_cap = cap;
    if (n > r->meshes_max)
        r->meshes_max = n;
    if (n >= cap)
        caps |= TM2_RENDER_CAP_MESHES;
    r->caps = caps;
    r->walk_cursor = *(int *)((unsigned char *)db + 8);
}

/* renderScene, after the emitter and the near pass: the buffer's packet bytes so far (the frame record's cursor
 * +8 minus its start +4; the views of a frame share retail's buffer, each has its own with
 * portPacketBufferBytes since request 47 revised), this view's scene share, the buffer's size (end
 * +0x0C minus start), then the caps reached this frame. */
void PORT_TEXT portRenderStatsDrawn(void *db)
{
    struct TM2PortRenderView *r;
    int start, cursor, end, n;

    if (portRenderStats.row < 0)
        return;
    r = &portRenderStats.view[portRenderStats.row];
    portRenderStats.row = -1;
    start = *(int *)((unsigned char *)db + 4);
    cursor = *(int *)((unsigned char *)db + 8);
    end = *(int *)((unsigned char *)db + 12);
    n = cursor - start;
    r->packet = n;
    if (n > r->packet_max)
        r->packet_max = n;
    r->packet_cap = end - start;
    n = cursor - r->walk_cursor;
    r->packet_scene = n;
    if (n > r->packet_scene_max)
        r->packet_scene_max = n;
    if (cursor > end - TM2_RENDER_PACKET_SLACK)
        r->caps |= TM2_RENDER_CAP_PACKETS;
    if (r->caps) {
        r->caps_seen |= r->caps;
        r->capped++;
        if (r->first_capped == 0)
            r->first_capped = r->frames;
    }
}
#endif /* TM2_PORT_FAR_DISTANCE */

/* ---- car groups (PC port request 32 step 1, docs/CAR_GROUPS.md) ---- */
/* Defined in every variant (the layout does not depend on the option); used only with
 * --port-define TM2_PORT_CAR_GROUPS. Zero is the correct initial state: retail's tables are
 * BSS and its spin angles sdata 0.
 *   portCarGroups   per-car registries of duplicate cars (include/semantic/car_groups.h);
 *   portCarOptions  the port's switches, 0 = retail: bit 0 the car pickers allow repeated
 *                   cars (carIsSelectable); bits 1-3 are step 2's (port_cars.h, TM2_PORT_CARS). */
struct TM2PortCarGroups portCarGroups PORT_BSS = {0};
int portCarOptions PORT_BSS = 0;

/* ---- more cars (PC port request 32 step 2, docs/CAR_GROUPS.md "Step 2") ---- */
/* The car arrays of SIXTEEN_CARS.md 1.1 and the pools of 1.6, sized by the limits of players.h:
 * retail's sizes in every variant, TM2_PORT_CARS's with --port-define TM2_PORT_CARS=N. aiCars comes
 * first, so its address is the same with and without the define (the storage before it does not
 * depend on the limits); the DMD area last. */
int aiCars[TM2_MAX_AI_CARS][0x854 / 4] PORT_BSS = {{0}};        /* AI car records (TM2AICar) */
int aiCarObjects[TM2_MAX_AI_CARS] PORT_BSS = {0};               /* world object of each AI car */
int aiCarObjectSlots[TM2_MAX_AI_CARS] PORT_BSS = {0};           /* car slot of each AI car */
int aiCarTargets[TM2_MAX_AI_CARS] PORT_BSS = {0};               /* AI target (AI index, -1 nearest human) */
int carSlotKinds[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};             /* 0 human, 1 AI, 2 loaded but free */
int carSlotIndices[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};           /* index in its record array, -1 free */
int carSlotOwners[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};            /* car type of each slot */
void *carSlotObjects[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};         /* world object of each slot */
#if TM2_MAX_AI_CARS > TM2_RETAIL_OPPONENT_SLOTS
/* Car-select indices of opponent slots 9..TM2_MAX_AI_CARS-1 (settings.h TM2_SETTINGS_OPPONENT_CAR);
 * the port fills them (with the record's slots 0..8) before sessionBindCars. */
signed char sessionPortOpponentCars[TM2_MAX_AI_CARS - TM2_RETAIL_OPPONENT_SLOTS] PORT_BSS = {0};
#endif
/* Cars per arena, humans included (tournamentOpponentCount, which the menus turn into the
 * opponent count): 0 = retail's table. The port sets it from its settings. */
int portCarsPerArena PORT_BSS = 0;
/* portCarOptions bit 3 latched at each load by carTablesClear (TM2_POOL_LIVE, port_cars.h): the weapon
 * pools' size this match, 1 = TM2_MAX_*, 0 = retail's. */
int portCarPoolsLive PORT_BSS = 0;
/* Car files the port hands over per load group (port_cars.h, mod cars of request 33): 0 = the disc's
 * files. Read only by portCarLoadGroup (TM2_PORT_CARS). */
struct TM2PortCarSource portCarSource[TM2_MAX_LOADED_CARS + 1] PORT_BSS = {{0}};

/* Pools that more cars fill (SIXTEEN_CARS.md 1.6): projectile records (980 bytes), coordinate
 * systems (0x128 bytes), sound instances (0x84 bytes) and the sound driver's priority list (retail
 * sorts it in the scratchpad at 0x1F800000, which holds no more than 35 entries before the
 * hit-detection state at +0xB0: with more instances the driver sorts here, CAR_GROUPS.md). */
int fxRecords[TM2_MAX_PROJECTILES][980 / 4] PORT_BSS = {{0}};
int csObjectPool[TM2_MAX_COORD_SYSTEMS][0x128 / 4] PORT_BSS = {{0}};
int soundInstPool[TM2_MAX_SOUND_INSTANCES][0x84 / 4] PORT_BSS = {{0}};   /* retail .data, all zero */
void *portSoundSortList[TM2_MAX_SOUND_INSTANCES] PORT_BSS = {0};

/* The weapon and render pools of request 32 Q9 (players.h): defined, and moved from their retail storage,
 * only when TM2_PORT_CARS raises them (tsv groups "name+"). Layouts as the units read them. */
#if TM2_MAX_ARCS > TM2_RETAIL_ARCS
int weaponArcs[TM2_MAX_ARCS][20 / 4] PORT_BSS = {{0}};                         /* 20-byte arc records */
#endif
#if TM2_MAX_SHIELDS > TM2_RETAIL_SHIELDS
int weaponShieldPool[TM2_MAX_SHIELDS * (4 + 12) / 4] PORT_BSS = {0};          /* objects[N], records[N] */
#endif
#if TM2_MAX_REMOTE_BOMBS > TM2_RETAIL_REMOTE_BOMBS
void *weaponRemoteBombObjects[TM2_MAX_REMOTE_BOMBS] PORT_BSS = {0};
#endif
#if TM2_MAX_RICOCHETS > TM2_RETAIL_RICOCHETS
int weaponRicochetPool[TM2_MAX_RICOCHETS * (4 + 48) / 4] PORT_BSS = {0};      /* objects[N], shots[N] */
#endif
#if TM2_MAX_FLAME_EMITTERS > TM2_RETAIL_FLAME_EMITTERS
int weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS][32 / 4] PORT_BSS = {{0}};
#endif
#if TM2_MAX_SLAM_RECORDS > TM2_RETAIL_SLAM_RECORDS
short weaponSlamRecords[TM2_MAX_SLAM_RECORDS][18 / 2] PORT_BSS = {{0}};        /* 18-byte claw records */
#endif
#if TM2_MAX_WORLD_MODELS > TM2_RETAIL_WORLD_MODELS
void *renderWorldModels[TM2_MAX_WORLD_MODELS] PORT_BSS = {0};
#endif
#if TM2_MAX_CLUT_ANIM_DEFS > TM2_RETAIL_CLUT_ANIM_DEFS
int clutAnimDefs[TM2_MAX_CLUT_ANIM_DEFS][20 / 4] PORT_BSS = {{0}};
#endif
#if TM2_MAX_CLUT_ANIMS > TM2_RETAIL_CLUT_ANIMS
int clutAnimRunning[TM2_MAX_CLUT_ANIMS][0x424 / 4] PORT_BSS = {{0}};
#endif
/* Request 40 (TM2_PORT_BALANCE with TM2_PORT_CARS, docs/BALANCE.md 3.3): the drop records (28 bytes: mines and
 * remote bombs) and the mine objects, 6 N and 5 N (tsv groups drops+ / mines+). Byte array as this unit reads it. */
#if TM2_MAX_DROPS > TM2_RETAIL_DROPS
unsigned char weaponDrops[TM2_MAX_DROPS * 28] __attribute__((aligned(4))) PORT_BSS = {0};
#endif
#if TM2_MAX_MINE_OBJECTS > TM2_RETAIL_MINE_OBJECTS
void *weaponMineObjects[TM2_MAX_MINE_OBJECTS] PORT_BSS = {0};
#endif
/* Request 102 item 3: the effect sprites (48 bytes; fxSpriteFindFree), 25 per car (tsv group fxsprites+) */
#if TM2_MAX_FX_SPRITES > TM2_RETAIL_FX_SPRITES
unsigned char fxSprites[TM2_MAX_FX_SPRITES * 48] __attribute__((aligned(4))) PORT_BSS = {0};
#endif

/* The five pair tables ([slot][slot], carPairsUpdate): the row stride is the slot limit. */
int carPairVectors[TM2_MAX_CAR_SLOTS * TM2_MAX_CAR_SLOTS][3] PORT_BSS = {{0}};
int carPairHeadingDeltas[TM2_MAX_CAR_SLOTS * TM2_MAX_CAR_SLOTS] PORT_BSS = {0};
int carPairDirections[TM2_MAX_CAR_SLOTS * TM2_MAX_CAR_SLOTS][3] PORT_BSS = {{0}};
short carPairHeadings[(TM2_MAX_CAR_SLOTS * TM2_MAX_CAR_SLOTS + 1) & ~1] PORT_BSS = {0};   /* s16, word-padded */
int carPairDistances[TM2_MAX_CAR_SLOTS * TM2_MAX_CAR_SLOTS] PORT_BSS = {0};

/* The car DMD load area (SIXTEEN_CARS.md 3.5) when portCarOptions bit 1 is set: the cars' DMD
 * files packed by their actual size from its start (sessionLoadArenaAndCars, portCarLoadGroup):
 * TM2_PORT_CAR_DMD_AREA_BYTES (players.h; 2 MiB and one sector of slack for the CD read's overrun,
 * fileioReadFile reads whole 2,048-byte sectors). The
 * level DMD stays at 0x80019C40. It must end below the port RAM end, where the port's own RAM starts
 * (--port-ram-end, default 0x80C00000: gen_link.DEFAULT_PORT_RAM_END). */
int portCarDmdArea[TM2_PORT_CAR_DMD_AREA_BYTES / 4] PORT_BSS = {0};

/* The radar record per HUD (FOUR_PLAYERS.md 5.1): 880 bytes for retail's 12 blips, larger with
 * TM2_PORT_CARS (players.h TM2_HUD_RADAR_BYTES). Defined last, so that the storage before it (the car
 * arrays from aiCars on, request 32 step 2) does not move with the blip count. */
int hudRadars[TM2_MAX_HUDS][TM2_HUD_RADAR_BYTES / 4] PORT_BSS = {{0}};

/* ---- car swap (PC port request 32 step 3, docs/CAR_GROUPS.md 8) ---- */
/* The load group each record was swapped to (portCarSetGroup), 0 = the settings' group: human i at [i],
 * AI j at [TM2_PORT_BIND_HUMANS + j] (car_groups.h). sessionBindCars binds a record with an entry to that
 * group's car, so a swap survives the per-round rebind of modes 3/4. Cleared at each load
 * (portCarGroupsReset). Defined in every variant after hudRadars (nothing before it moves); read only with
 * --port-define TM2_PORT_CAR_SWAP. */
signed char portCarBindGroup[TM2_PORT_BIND_ENTRIES] PORT_BSS = {0};

/* ---- texture banks (PC port request 32 step 4, docs/TEXTURE_BANKS.md "As built", CAR_GROUPS.md 9) ---- */
/* Defined in every variant after portCarBindGroup (nothing before them moves); used only with
 * --port-define TM2_PORT_TEXTURE_BANKS (include/semantic/tex_banks.h):
 *   portTexBanks         bank per load group, owner per bank, the rectangles of the bank being filled, the
 *                        body CLUT per group, the level's record count (written by the C; the port reads)
 *   portVramUploadBank   the bank of the CPU upload (LoadImage / StoreImage) in progress, 0 = VRAM: the game
 *                        sets it before a bank-aware upload and resets it to 0 after its DrawSync
 *   portCarGroupNoReuse  per load group, set by the port (mod cars): the group neither takes an image by key
 *                        nor gives one (never cleared by the C) */
struct TM2PortTexBanks portTexBanks PORT_BSS = {0};
int portVramUploadBank PORT_BSS = 0;
unsigned char portCarGroupNoReuse[TM2_PORT_TEX_GROUPS] PORT_BSS = {0};

/* ---- per-car specials (PC port request 33 steps 2a and 2e, docs/CAR_GROUPS.md 10 and 11) ---- */
/* Defined in every variant after portCarGroupNoReuse (nothing before it moves); used only with
 * --port-define TM2_PORT_CAR_SPECIALS (include/semantic/car_specials.h): the special state (Mr. Slam's arm
 * and claw, Twister's tornado, Shadow's strike, Axel's shockwave, Grasshopper's leap; with
 * TM2_PORT_DARK_TOOTH also Dark Tooth's head and boss state) of each duplicate car key, the selection per
 * part, and the claw records past weaponSlamRecords. Written by the C only. */
struct TM2PortCarSpecials portCarSpecials PORT_BSS = {{0}};

/* ---- per-car engine sound (PC port request 33 step 3, docs/CAR_GROUPS.md 12) ---- */
/* Defined in every variant after portCarSpecials (nothing before it moves); used only with
 * --port-define TM2_PORT_CAR_SOUND (include/semantic/car_sound.h):
 *   portCarSound      per car slot: the engine sound id its instance plays, allowed by the cap, flame sound
 *                     started; bit 4 as applied (written by the C, the port may read it)
 *   portEngineVoices  the port's engine cap: 0 = every car's engine plays, N > 0 = the N nearest to a human */
struct TM2PortCarSound portCarSound PORT_BSS = {0};
int portEngineVoices PORT_BSS = 0;

/* ---- per-car CLUTs (PC port request 33 step 1, docs/CAR_GROUPS.md 13) ---- */
/* Defined in every variant after portEngineVoices (nothing before it moves); used with --port-define
 * TM2_PORT_CAR_CLUTS (the copies, the per-group CLUT table) and TM2_PORT_DARK_TOOTH (the rows 240..255 allocator
 * that bit 6 shares with the copies), include/semantic/car_cluts.h. Written by the C; the port reads it. */
struct TM2PortCarCluts portCarCluts PORT_BSS = {0};

/* ---- online with bots, players dropping in (PC port request 33, docs/CAR_GROUPS.md 14) ---- */
/* Defined in every variant after portCarCluts (nothing before them moves); used only with --port-define
 * TM2_PORT_CAR_DROPIN (include/semantic/car_dropin.h):
 *   portCarDropIn    the drop-in state of this load: vacant AI records, groups loaded mid-match, the DMD area's
 *                    free end, the dummy object of vacant records (written by the C; the port may read it)
 *   portCarLiveTex   the VRAM usage bitmap, texture-slot table and image records kept through the match
 *                    (bitmapSetupStorage), and a snapshot for a mid-match load that does not fit (internal) */
struct TM2PortCarDropIn portCarDropIn PORT_BSS = {0};
struct TM2PortCarLiveTex portCarLiveTex PORT_BSS = {{0}};

/* ---- any car fires another car's special (PC port request 35, docs/GUN_GAME_SPECIALS.md "As built") ---- */
/* Defined in every variant after portCarLiveTex (nothing before them moves); used only with --port-define
 * TM2_PORT_SPECIAL_AS (include/semantic/car_specials.h):
 *   portCarSpecialAs     per car slot, the type whose special the car fires; 0 / -1 = its own (the load sets -1);
 *                        written by portCarSpecialAsSet (the port reads it)
 *   portCarSpecialLoad   set by the port: bit type >> 4 = load that type's car file at the next load (a parked
 *                        spare group; with TM2_PORT_CARS and portCarOptions bit 1); never cleared by the C
 *   portSpecialAs        the per-slot as-entries (Shadow's strike, Axel's shockwave and ring) and the load's
 *                        spare groups (written by the C) */
short portCarSpecialAs[TM2_CAR_SLOT_CAP] PORT_BSS = {0};
int portCarSpecialLoad PORT_BSS = 0;
struct TM2PortSpecialAs portSpecialAs PORT_BSS = {{{{0}}}};

/* ---- the rest of per-car sound (PC port request 33 step 4, docs/CAR_GROUPS.md 16) ---- */
/* Defined in every variant after portSpecialAs (nothing before it moves); used only with --port-define
 * TM2_PORT_CAR_SOUND (include/semantic/car_sound.h): per car slot, the loops (TM2_PORT_LOOP_*: a human's turbo,
 * gun and burn loops for humans 2.., Twister's tornado sound with bit 4) started as the slot's instance. */
unsigned char portCarSoundLoops[TM2_PORT_SOUND_SLOTS] PORT_BSS = {0};

/* ---- a player's Dark Tooth (PC port request 36, docs/DARK_TOOTH_PLAYER.md) ---- */
/* Defined in every variant after portCarSoundLoops (nothing before it moves); used only with --port-define
 * TM2_PORT_DT_PLAYER (include/semantic/dt_player.h): the port's settings for him (missile interval, special
 * recharge and count, engine sound, the info picture and ending movie of index 14, a carousel model) and the
 * extended carousel node of the car select in progress (written by the C). */
struct TM2PortDtPlayer portDtPlayer PORT_BSS = {0};

/* ---- arena hazards and destructibles off (PC port request 38, docs/ARENA_SWITCHES.md) ---- */
/* Defined in every variant after portDtPlayer (nothing before it moves); used only with --port-define
 * TM2_PORT_HAZARDS (include/semantic/port_hazards.h): the port's two switches (bit 0 hazards off, bit 1
 * destructibles off), its per-item mask (one bit per item, TM2_ARENA_*), and the items off this round, copied at
 * roundStart (portArenaLatch, written by the C). All zero = retail. */
int portArenaOptions PORT_BSS = 0;
unsigned int portArenaItems PORT_BSS = 0;
unsigned int portArenaLive PORT_BSS = 0;

/* ---- the local-player map (docs/HUMANS16_PLAN.md step H1) ---- */
/* Defined in every variant after portArenaLive (nothing before it moves); used only with --port-define
 * TM2_PORT_HUMANS (include/semantic/port_humans.h): per human record, its local slot on this machine (0 = the
 * default: slot h for h < TM2_MAX_VIEWS, remote past that; 1..4 = slot 0..3; < 0 = remote). Set by the port. */
signed char portHumanLocal[TM2_MAX_HUMANS] PORT_BSS = {0};

/* ---- the humans' inputs, layouts and lives (docs/HUMANS16_PLAN.md step H2) ---- */
/* Defined in every variant after portHumanLocal (nothing before them moves); used only with --port-define
 * TM2_PORT_HUMANS (include/semantic/port_humans.h): the option bits (TM2_HUMAN_OPT_*, 0 = retail), per human its
 * input word of this frame (padButtonsDecoded's half layout) and stick tilt (0..4096, read by the port), its
 * control layout, its lives and its action -> button table (built by portHumanSetLayout), and the blink clock of
 * the bit-4 AI blips (written by the C, presentation only). Set by the port, except the bindings and the clock. */
int portHumanOptions PORT_BSS = 0;
unsigned short portHumanPad[TM2_MAX_HUMANS] PORT_BSS = {0};
short portHumanSteer[TM2_MAX_HUMANS] PORT_BSS = {0};
extern short portHumanThrottle[TM2_MAX_HUMANS];    /* request 121: defined after portDtPhase2Truck (nothing moves) */
extern int portHumanWreckLive[TM2_MAX_HUMANS];     /* request 126: defined with the other port storage below */
unsigned char portHumanLayout[TM2_MAX_HUMANS] PORT_BSS = {0};
unsigned char portHumanLives[TM2_MAX_HUMANS] PORT_BSS = {0};
unsigned char portHumanBindings[TM2_MAX_HUMANS][128] PORT_BSS = {{0}};
unsigned int portHumanRadarBlink PORT_BSS = 0;

/* ---- Vanilla Plus balancing (PC port request 40, docs/BALANCE.md; replaces request 39's four words) ---- */
/* Defined in every variant after portHumanRadarBlink (nothing before them moves); used only with --port-define
 * TM2_PORT_BALANCE (include/semantic/port_balance.h): the port's switches (TM2_BAL_*, 0 = retail) and their copy
 * for this round, made at roundStart (portBalanceLatch, written by the C); the placement order of each drop record
 * (bit 2, "the driver's oldest mine") and, per crusher slot, Hammerhead's +5 on top still owed (bit 3), both
 * written by the C. */
int portBalance PORT_BSS = 0;
int portBalanceLive PORT_BSS = 0;
unsigned int portBalanceDropSeq[TM2_MAX_DROPS] PORT_BSS = {0};
unsigned int portBalanceDropSeqNext PORT_BSS = 0;
unsigned char portBalanceRideTop[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};

/* ---- the rules (docs/HUMANS16_PLAN.md step H5) ---- */
/* Defined in every variant after portBalanceRideTop (nothing before it moves); used only with --port-define
 * TM2_PORT_HUMANS (include/semantic/port_humans.h): the port's last round end (portRoundEnd: event, delay,
 * winner) and the start a respawn / drop-in forces while it runs (portStartNodePick). Written by the C. */
struct TM2PortRound portRound PORT_BSS = {0};

#if TM2_MAX_SOUND_REQUESTS > TM2_RETAIL_SOUND_REQUESTS
/* The sound request pool (sndObjPool, sndPoolAlloc) raised by TM2_PORT_CARS (request 33 step 4): the pointer
 * table moves here (tsv group sndreqs+, linear) and the request objects, which retail keeps on main's stack (35
 * of 120 bytes, shell_main.c), are portSndObjSlots. Defined last (only when raised). */
void *sndObjPool[TM2_MAX_SOUND_REQUESTS] PORT_BSS = {0};
int portSndObjSlots[TM2_MAX_SOUND_REQUESTS * 120 / 4] PORT_BSS = {0};   /* [N][120 bytes] */
#endif

/* ---- unloading cars mid-match (PC port request 33, CAR_GROUPS.md 14.7) ---- */
/* Defined in every variant after the rest of the port storage (nothing before it moves); used only with
 * --port-define TM2_PORT_CAR_DROPIN (include/semantic/car_dropin.h): the DMD area's free ranges, csAlloc's free list
 * of coordinate systems, the width of each CLUT copy, each group's image records. Written by the C (portCarRoom
 * reports it to the port). */
struct TM2PortCarUnload portCarUnload PORT_BSS = {0};

/* ---- Vanilla Plus balancing, rounds 2 and 3 (PC port request 41, docs/BALANCE.md 12 and 13) ---- */
/* Defined in every variant after portCarUnload (nothing before it moves); used only with --port-define
 * TM2_PORT_BALANCE (port_balance.h struct TM2PortBalanceCar): per car slot, the freeze chain (bit 17), the refused
 * gun button (bit 15) and the machine guns' half point (bit 20); then the guns' half point of the hits that are not
 * cars (bit 20). Written by the C, cleared by portBalanceLatch. */
struct TM2PortBalanceCar portBalanceCars[TM2_MAX_CAR_SLOTS] PORT_BSS = {{0}};
unsigned int portBalanceGunObjects PORT_BSS = 0;

/* ---- custom tournament and mods (PC port request 42, docs/CUSTOM_TOURNAMENT_ANSWERS.md) ---- */
/* Defined in every variant after portBalanceGunObjects (nothing before them moves):
 *   portVisual               item 6 / 7: the port's visual switches (port_visual.h TM2_VIS_*, 0 = retail; read
 *                            live), used only with --port-define TM2_PORT_VISUAL
 *   portCarDtBossMask        item 1: per car slot (bit slot & 31 of word slot >> 5), the Dark Tooths that keep the
 *                            boss parts while portCarOptions bit 10 makes the others ordinary (car_specials.h
 *                            portCarDarkToothRole; set by the port, never cleared by the C; 0 = all ordinary), used
 *                            only with TM2_PORT_DARK_TOOTH
 *   portSettingsHumanCar, portSettingsOpponentCar, portModCars, portCarNumbers
 *                            item 3: sessionSettings' 16-bit car numbers, the port's 128 mod cars, the numbers per
 *                            load group (car_numbers.h), used only with TM2_PORT_CAR_NUMBERS
 *   portCarSel               item 5: car select's extended carousel and name labels (car_numbers.h), used with
 *                            TM2_PORT_DT_PLAYER (bit 8) and TM2_PORT_CAR_NUMBERS (mods) */
int portVisual PORT_BSS = 0;
unsigned int portCarDtBossMask[2] PORT_BSS = {0};
short portSettingsHumanCar[TM2_PORT_NUMBER_HUMANS] PORT_BSS = {0};
short portSettingsOpponentCar[TM2_PORT_NUMBER_OPPONENTS] PORT_BSS = {0};
struct TM2PortModCar portModCars[TM2_PORT_MOD_CARS] PORT_BSS = {{0}};
struct TM2PortCarNumbers portCarNumbers PORT_BSS = {{0}};
struct TM2PortCarSel portCarSel PORT_BSS = {0};
typedef char tm2_r42_number_humans[(TM2_MAX_HUMANS <= TM2_PORT_NUMBER_HUMANS) ? 1 : -1];
typedef char tm2_r42_number_opponents[(TM2_MAX_AI_CARS <= TM2_PORT_NUMBER_OPPONENTS) ? 1 : -1];
typedef char tm2_r42_number_groups[(TM2_MAX_LOADED_CARS + 1 <= TM2_PORT_NUMBER_GROUPS) ? 1 : -1];

/* ---- a parked Dark Tooth's head (PC port request 46, docs/PLAYTEST_ANSWERS.md 7) ---- */
/* Defined in every variant after portCarSel (nothing before it moves); used only with TM2_PORT_DARK_TOOTH:
 *   portCarDtParkMask        per car slot (bit slot & 31 of word slot >> 5), the Dark Tooths whose head the C hid
 *                            because nobody drives the car (portCarDarkToothParkHead); the frame hook
 *                            portCarDarkToothParked shows it again when a record drives the car. Written by the C,
 *                            cleared each load (portCarSpecialsReset). */
unsigned int portCarDtParkMask[2] PORT_BSS = {0};

/* ---- the user's third playtest (PC port request 46 items 4-6, docs/PLAYTEST_ANSWERS.md 11-13) ---- */
/* Defined in every variant after portCarDtParkMask (nothing before them moves):
 *   portFarLodNearScale      item 6: the distance scale of every LOD band while renderScene walks a view (0 / 1 =
 *                            retail, 2..16: each band's bounds at that many times their distance, so the sharp near
 *                            bands are kept that much farther; FAR_LOD.md 8), set by the port, read live; used only
 *                            with TM2_PORT_FAR_LOD
 *   portFarLodNear           item 6: every LOD record of the match (the level's and each car file's, found by the
 *                            far-LOD scan) and its retail band, for the scale; used only with TM2_PORT_FAR_LOD
 *   portMirrorFar            item 9: the rear-view mirror's far distance in world units, 0 = retail (500 for
 *                            the level, 1000 for the objects), 1..32767 for both (FAR_DISTANCE.md 5); set by the
 *                            port, read live; used only with TM2_PORT_FAR_DISTANCE
 *   portVisualState          items 4 and 5: portVisual bits 3 and 4's state (port_visual.h struct
 *                            TM2PortVisualState: the shadows' and cars' draw entries of the view, the shadows' OT
 *                            pairs, each bullet's shooter at the shot); written by the C; used only with
 *                            TM2_PORT_VISUAL */
int portFarLodNearScale PORT_BSS = 0;
int portMirrorFar PORT_BSS = 0;
struct PortFarLodNear portFarLodNear PORT_BSS = {0};
struct TM2PortVisualState portVisualState PORT_BSS = {{{0}}};

/* ---- the render capacity counter (docs/RENDER_ANSWERS.md "Measuring the render caps") ---- */
/* Defined in every variant after portVisualState (nothing before it moves); written only with
 * TM2_PORT_FAR_DISTANCE (portRenderStatsWalked / portRenderStatsDrawn, called by renderScene), read by the port:
 *   portRenderStats          per view (rows 0..3) and the rear-view mirror (row 4): the matrix slots, the meshes
 *                            queued and the frame's packet bytes, this frame and their high-water marks, the caps
 *                            reached (port_render_stats.h struct TM2PortRenderStats); the port writes `reset` to
 *                            start again. Observation only: not game state (no snapshot, no hash). */
struct TM2PortRenderStats portRenderStats PORT_BSS = {0};

/* ---- more matrix slots (PC port request 47, render_slots.h, docs/FAR_DISTANCE.md 3 "Matrix slots") ---- */
/* The five slot arrays renderInitState points the scratchpad RenderState at (the walker fills them by slot, the
 * emitter reads the rotation and light matrices by a mesh's slot), TM2_MAX_RENDER_SLOTS slots each (the limit + 1:
 * the last slot renderScene hands out is inside them). Defined only with TM2_PORT_RENDER_SLOTS (tsv group
 * renderslots+: otherwise retail's storage stays in use), after portRenderStats (nothing before them moves); their
 * retail storage is left unused. Byte arrays, as the units declare them. */
#if TM2_MAX_RENDER_SLOTS > TM2_RETAIL_RENDER_SLOTS
unsigned char renderSlotMatrixPtrs[TM2_MAX_RENDER_SLOTS * 16] __attribute__((aligned(4))) PORT_BSS = {0};
unsigned char renderMatrixSlots[TM2_MAX_RENDER_SLOTS * 32] __attribute__((aligned(4))) PORT_BSS = {0};
unsigned char renderObjectMatrices[TM2_MAX_RENDER_SLOTS * 32] __attribute__((aligned(4))) PORT_BSS = {0};
unsigned char renderFrustumMatricesA[TM2_MAX_RENDER_SLOTS * 32] __attribute__((aligned(4))) PORT_BSS = {0};
unsigned char renderFrustumMatricesB[TM2_MAX_RENDER_SLOTS * 32] __attribute__((aligned(4))) PORT_BSS = {0};
#endif

/* ---- online, stage 1 (PC port request 48, docs/ONLINE_ROLLBACK.md 8 "Stage 1 as built") ---- */
/* Defined in every variant after the matrix slots (nothing before them moves); used only with --port-define
 * TM2_PORT_ROLLBACK (include/semantic/port_rollback.h):
 *   portRollbackOptions      the bits (TM2_RB_*: 0 FIXED_TICKS, 1 SEED, 7 HOLD, 9 PAUSE_PORT; stage 2: 2 ONCE_PER_STEP,
 *                            3 ANIM_IN_STEP, 4 VISUAL_RAND; stage 3: 5 SOUND_GATE, 6 CLUT_SHADOW, 8 STEP_SPLIT), 0 = retail; set by the port, read live
 *   portResimulating         the port's flag around a batch of re-run steps (read by stage 3: SOUND_GATE, CLUT_SHADOW, portStepBody)
 *   portStepHold             HOLD: non-zero = rtMainLoop's iterations draw only (latched at the top of each)
 *   portStepTicks            FIXED_TICKS: the ticks of a step, 0 = 3 in every mode (20 steps a second)
 *   portRollbackSeed         SEED: the match seed
 *   portRollbackFrameCount   SEED: timerFrameCount at the start of each load
 *   portRollbackQuit         PAUSE_PORT: non-zero = quit (event 2) at the top of the next step; cleared by the C
 *   portRollbackHolding      written by the C: this iteration's HOLD latch
 *   portRollbackReseeds      written by the C: the srand calls since the load's seed (SEED's mix) */
int portRollbackOptions PORT_BSS = 0;
int portResimulating PORT_BSS = 0;
int portStepHold PORT_BSS = 0;
int portStepTicks PORT_BSS = 0;
unsigned int portRollbackSeed PORT_BSS = 0;
unsigned int portRollbackFrameCount PORT_BSS = 0;
int portRollbackQuit PORT_BSS = 0;
int portRollbackHolding PORT_BSS = 0;
unsigned int portRollbackReseeds PORT_BSS = 0;

/* ---- the start picker's cars (PC port request 49, docs/CAR_GROUPS.md 7.4) ---- */
/* Defined in every variant after the rollback words; used only with TM2_PORT_CARS (port_cars.h): the cars whose
 * start ran since startNodesClearUsed (humans 0.., AI cars 16..) and the picks of each start-list entry since then.
 * Written by the C (portStartPlacedClear / portStartPlacedMark / portStartNodePick). */
struct TM2PortStartPlaced portStartPlaced PORT_BSS = {{0}};
/* Request 85 (port_cars.h, SIXTEEN_CARS.md 5.5): a link start waiting for the car whose start pick set it; set and
 * taken within that car's start (match state under .port_bss's default, snapshot and hash yes: always 0 between steps). Every variant, after
 * portStartPlaced (nothing before it moves); written only with TM2_PORT_CARS. */
struct TM2PortStartLink portStartLink PORT_BSS = {0};

/* ---- online, stage 2 (PC port request 48, docs/ONLINE_ROLLBACK.md 11) ---- */
/* Defined in every variant after portStartPlaced (nothing before it moves); used only with TM2_PORT_ROLLBACK:
 *   portRollbackVisualSeed   the visual rand() stream's state (portRollbackVisualRand), for the draws that must not
 *                            touch the game's stream (ONCE_PER_STEP's arc drawing, VISUAL_RAND); any value, the port
 *                            may seed it; presentation (out of the hash, never restored) */
unsigned int portRollbackVisualSeed PORT_BSS = 0;

/* ---- online, stage 3 (PC port request 48, docs/ONLINE_ROLLBACK.md 12) ---- */
/* Defined in every variant after portRollbackVisualSeed (nothing before it moves); used only with TM2_PORT_ROLLBACK:
 *   portRollbackSplitting    written by the C: this iteration's STEP_SPLIT latch (1 = the step ran at the top, the
 *                            iteration's own update calls are skipped)
 *   portRollbackEnded        written by the C: 1 = a re-run (portRunStep) fired an event; rtMainLoop leaves at its next
 *                            top with rtFiredEventValue
 *   portClutShadow           CLUT_SHADOW: the RAM copy of the palette rows the animations touch (game state: in the
 *                            snapshot and the hash) */
int portRollbackSplitting PORT_BSS = 0;
int portRollbackEnded PORT_BSS = 0;
struct TM2PortClutShadow portClutShadow PORT_BSS = {0};
/* Request 50 (port_humans.h "Hand-over", HUMANS16_PLAN.md 8): the vacant human records, every variant, last (nothing
 * before it moves). Written by the C (portHumanHandOver / Leave / Return, cleared at each load); used only with
 * TM2_PORT_HUMANS and TM2_PORT_CAR_SWAP. Match state: in the rollback snapshot and hash (port.bss). */
struct TM2PortHumanVacant portHumanVacant PORT_BSS = {0};
/* Request 54 (ONLINE_ROLLBACK.md 15.1): the model files installed for the match, by load group (portDmdNoteImage from
 * assetInstallImage; cleared at each load by portRollbackPreload, a group's entry by portCarUnloadGroup), every
 * variant, after portHumanVacant (nothing before it moves); used only with TM2_PORT_ROLLBACK. Load-time bookkeeping:
 * portDmdMutableList walks it, no step reads it (rollback_regions.tsv: snapshot and hash no). */
struct TM2PortDmdImages portDmdImages PORT_BSS = {{0}};
/* Request 61 (FAR_DISTANCE.md 2): the GTE range pass's sprite and beam meshes (struct PortFarSprites above), every
 * variant, after portDmdImages (nothing before it moves); written only by portFarDistanceScale (renderScene's emitter
 * call, per drawn view): drawing only. */
struct PortFarSprites portFarSprites PORT_BSS = {0};
/* Request 61 addendum (TM2_PORT_C_EMITTER, include/semantic/port_c_emitter.h; PLAYTEST_ANSWERS.md 4.3 item 1): the
 * port's switch for the C emitter's precise corners: 0 off, else the anamorphic x factor in 1/4096 (4096 = none); the
 * emitter then also writes the beams' and point sprites' subpixel corners to the port's precise ring. Set by the port
 * each frame (its precise mode and widescreen; may differ between machines), read only by the emitter's drawing:
 * rollback_regions.tsv snapshot and hash no. Every variant, after portFarSprites (nothing before it moves). */
int portPreciseEmit PORT_BSS = 0;
/* Request 66 (ANALOG_STEERING.md 2.6, port_humans.h TM2_HUMAN_CTL_*): per human record its control options (bit 0: the
 * turn keeps its sign in reverse), set by the port from each player's Controls option at the match's start, the same
 * on every machine: match state (rollback_regions.tsv snapshot and hash yes; carThrottleSteer reads it in the step).
 * Every variant, after portPreciseEmit (nothing before it moves); used only with TM2_PORT_HUMANS. */
unsigned char portHumanControl[TM2_MAX_HUMANS] PORT_BSS = {0};
/* Request 71 (car_numbers.h struct TM2PortCarSelLoad): car select's in-game models, one per load slot (portCarSelLoad;
 * used with TM2_PORT_DT_PLAYER or TM2_PORT_CAR_NUMBERS): shell presentation, rollback_regions.tsv snapshot and hash
 * no. Request 70 (struct TM2PortCarProfile): each mod car's own numbers, set by the port before a load, the same on
 * every machine: match state (snapshot and hash yes; read only with TM2_PORT_CAR_PROFILE). Every variant, after
 * portHumanControl (nothing before them moves). */
struct TM2PortCarSelLoad portCarSelLoads[TM2_PORT_CARSEL_LOADS] PORT_BSS = {{0}};
struct TM2PortCarProfile portCarProfile[TM2_PORT_MOD_CARS] PORT_BSS = {{0}};
/* Request 72 (port_visual.h struct TM2PortHiddenAreas, HIDDEN_AREAS.md 11): portVisual bit 6's patches of the level
 * model, put back by the end of each view's walk: drawing only (rollback_regions.tsv snapshot and hash no; used only
 * with TM2_PORT_HIDDEN_AREAS). Every variant, after portCarProfile (nothing before it moves). */
struct TM2PortHiddenAreas portHiddenAreas PORT_BSS = {0};
/* Request 73 (car_numbers.h, SPECIAL_TEMPLATES.md 6): each mod car's special (set by the port before a load: match
 * state, snapshot and hash yes) and the C's state for it (the free projectile records of a fire in progress, the AI
 * weight tables per mod: snapshot and hash yes). Every variant, after portHiddenAreas (nothing before them moves);
 * used only with TM2_PORT_SPECIAL_PROFILE. */
struct TM2PortSpecialProfile portSpecialProfile[TM2_PORT_MOD_CARS] PORT_BSS = {{0}};
struct TM2PortSpecialState portSpecialState PORT_BSS = {{0}};
/* Request 75 (port_visual.h struct TM2PortHiddenRoof, HIDDEN_AREAS.md 12): the church's roof under bit 6, rebuilt by
 * each view's portHiddenAreasBegin: drawing only (snapshot and hash no; used only with TM2_PORT_HIDDEN_AREAS). Every
 * variant, after portSpecialState (nothing before it moves). */
struct TM2PortHiddenRoof portHiddenRoof PORT_BSS = {0};
/* Request 79 (port_visual.h struct TM2PortPickupSpecial, PICKUP_LOOKS.md 10): the special's pickup look, rebuilt at
 * each level load (the same on every machine: level data only); read by the walks and the hit walk through the
 * pickups the port points at it (snapshot and hash no: load state, not changed by a step). Every variant, after
 * portHiddenRoof (nothing before it moves); used only with TM2_PORT_VISUAL. */
struct TM2PortPickupSpecial portPickupSpecial PORT_BSS = {0};
/* Request 82 (port_c_emitter.h, PLAYTEST_ANSWERS.md 27): the corners' depths of the primitives the emitter defers to
 * the near pass, written only while portPreciseEmit != 0 (TM2_PORT_C_EMITTER), read by the C near pass for the precise
 * ring: drawing only (snapshot and hash no). Every variant, after portPickupSpecial (nothing before it moves). */
int portNearDepth[200 * 4] PORT_BSS = {0};
/* Request 86 (port_rollback.h, ONLINE_ROLLBACK.md 23): the step's stack fill, the port's settings (snapshot and hash
 * no). Every variant, after portNearDepth (nothing before them moves); read only with TM2_PORT_ROLLBACK. */
int portStepScrub PORT_BSS = 0;
unsigned int portStepScrubFill PORT_BSS = 0;
/* Request 87 (spu_voices.h, SPU_VOICES.md 11): whose sound each voice plays (the port reads it at the key-on), the
 * request pool's tags and the play's tag. Every variant, after portStepScrubFill (nothing before them moves); written
 * only with TM2_PORT_SPU48; the sound side (snapshot and hash no). */
unsigned char portVoiceCar[48] PORT_BSS = {0};
unsigned char portVoiceKind[48] PORT_BSS = {0};
unsigned short portSndReqTag[TM2_VOICE_REQS] PORT_BSS = {0};
int portSoundTag PORT_BSS = 0;

#if TM2_SPU_VOICES > 24
struct PortVoiceReq {                   /* SndObj (a sound request) as portVoiceOwner reads it */
    int pad00[6];
    int sound;                          /* +0x18 the sound id */
    int pad1C[4];
    int instance;                       /* +0x2C soundPlay 0, soundPlayVariant e (a car's own: slot + 1) */
    int pad30[16];
    int index;                          /* +0x70 its index in sndObjPool */
};

/* The kind a sound id may carry: TM2_VOICE_ENGINE for the cars' engines (0x24..0x3C even, 0x41), TM2_VOICE_SPECIAL for
 * the specials' sounds (the fire sounds 0x27..0x3D odd: the boomerang's flight 0x33, the flame 47, the tornado 0x35 and
 * Dark Tooth's first swoop 61 among them; his second swoop 66; a pickup template's launch and flight sounds 10..15,
 * the ricochet 0x12 and the bomb 0x13), else 0 (the hits, the explosions and the rest). */
static int PORT_TEXT portVoiceKindOf(int snd)
{
    if (snd == 0x41 || (snd >= 0x24 && snd <= 0x3C && (snd & 1) == 0))
        return TM2_VOICE_ENGINE;
    if ((snd >= 0x27 && snd <= 0x3D && (snd & 1) != 0) || snd == 66 || (snd >= 10 && snd <= 15) || snd == 0x12 ||
        snd == 0x13)
        return TM2_VOICE_SPECIAL;
    return 0;
}

/* soundOn, before SsUtKeyOnV keys `voice` on for `request` (the key-on reaches the SPU at the flush after it): the
 * request's tag when its sound is of the tag's kind; else its instance when that is a car's own (slot + 1) and its
 * sound an engine or a special sound played per car (the flame 47, the tornado 0x35, the swoops 61 and 66); else an
 * engine's kind with no car (bit 4 clear: one request for every car of the type); else none. */
void PORT_TEXT portVoiceOwner(int voice, void *request)
{
    struct PortVoiceReq *q = request;
    int car = 0, kind = 0, k, tag;

    if ((unsigned int)voice >= 48)
        return;
    if (q != 0) {
        k = portVoiceKindOf(q->sound);
        tag = (unsigned int)q->index < TM2_VOICE_REQS ? portSndReqTag[q->index] : 0;
        if (k != 0 && tag >> 8 == k) {
            car = tag & 0xFF;
            kind = k;
        } else if ((k == TM2_VOICE_ENGINE || q->sound == 47 || q->sound == 0x35 || q->sound == 61 || q->sound == 66) &&
                   q->instance > 0 && q->instance <= 64) {
            car = q->instance;
            kind = k;
        } else if (k == TM2_VOICE_ENGINE) {
            kind = k;
        }
    }
    portVoiceCar[voice] = car;
    portVoiceKind[voice] = kind;
}
#endif
typedef char tm2_r49_humans[(TM2_MAX_HUMANS <= TM2_PORT_START_AI) ? 1 : -1];
typedef char tm2_r49_ai[(TM2_PORT_START_AI + TM2_MAX_AI_CARS <= TM2_PORT_START_CARS) ? 1 : -1];

#ifdef TM2_PORT_CAR_GROUPS
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif

extern int vramTexSlotGroup;            /* load group of the DMD being installed (assetInstallImage) */
extern int carWorldObjectCount;         /* car slots registered in this load */
extern void animSetStep(void *s, unsigned int step);
extern void *RotMatrixX(int, void *);
extern void mathCopyWords(void *, void *, int);

#define CG portCarGroups
#ifdef TM2_PORT_CAR_DROPIN
static void portCarDropInReset(void);

/* The lowest overflow key index d (key (16 + d) << 4) no group holds: below dup_count (the keys handed out in
 * this load) one an unload gave back (key_type 0), else dup_count; -1 when all are taken. */
static int PORT_TEXT portCarFreeDupKey(void)
{
    int d;

    for (d = 0; d < CG.dup_count && CG.key_type[16 + d] != 0; d++)
        ;
    return d < TM2_CAR_KEYS - 16 ? d : -1;
}
#endif

/* carTablesClear (every load, before the level and car DMDs register): forget the slots,
 * groups and keys and the object pointers (retail clears its pointer tables in carPartsReset
 * and objRegistriesReset). The tire accumulators, the spin angles and the freeze records of
 * the first cars (key indices 1..15) are kept, as retail keeps its own; the duplicates'
 * freeze records (16..31) are zeroed, because carFreezeResetAll steps every record and a
 * stale one would point into a DMD area reused since. */
void PORT_TEXT portCarGroupsReset(void)
{
    int i, j;

    for (i = 0; i < TM2_CAR_SLOT_CAP; i++) {
        CG.slot_group[i] = 0;
        CG.slot_key[i] = 0;
        CG.group_key[i] = 0;
    }
    for (i = 0; i < TM2_CAR_KEYS; i++) {
        CG.key_type[i] = 0;
        CG.body[i] = 0;
        CG.wreck[i] = 0;
        CG.damage_stage[i] = 0;
        CG.turbo[i] = 0;
        for (j = 0; j < 3; j++)
            CG.tire[i][j] = 0;
        for (j = 0; j < 6; j++)
            CG.parts[i][j] = 0;
    }
    CG.dup_count = 0;
    for (i = 16; i < TM2_CAR_KEYS; i++)
        for (j = 0; j < 264; j++)
            CG.freeze[i][j] = 0;
#ifdef TM2_PORT_CAR_SWAP
    /* request 32 step 3: the swaps of the last match name its load groups */
    for (i = 0; i < TM2_PORT_BIND_ENTRIES; i++)
        portCarBindGroup[i] = 0;
#endif
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialsReset();         /* request 33-2a: the duplicates' special state */
#endif
#ifdef TM2_PORT_CAR_DROPIN
    portCarDropInReset();           /* request 33 drop-in: vacant records, live groups of the last match */
#endif
#ifdef TM2_HUMAN_HANDOVER
    portHumanVacantReset();         /* request 50: no vacant human record in a new load */
#endif
}

/* carRegisterModel, for each car root (slot = the new slot, type = the root id 0x10..0xF0):
 * the slot's load group and key. The first root of a group fixes the group's key: the type
 * if no other group has that type's key yet, else the next overflow key (after 16 of them,
 * the type again: retail sharing). Further roots of the group (Minion's second 224) share it. */
void PORT_TEXT portCarRegisterRoot(int slot, int type)
{
    int g, h, key;

    if ((unsigned int)slot >= TM2_CAR_SLOT_CAP)
        return;
    g = vramTexSlotGroup;
    CG.slot_group[slot] = g;
    if ((unsigned int)g >= TM2_CAR_SLOT_CAP) {
        CG.slot_key[slot] = type;
        return;
    }
    key = CG.group_key[g];
    if (key == 0) {
        key = type;
        for (h = 0; h < TM2_CAR_SLOT_CAP; h++)
            if (h != g && CG.group_key[h] == type)
                break;
#ifdef TM2_PORT_CAR_DROPIN
        /* request 33 (CAR_GROUPS.md 14.7): the lowest overflow key no group holds (an unload gives its key back,
           key_type 0); with nothing unloaded that is dup_count, the next one, as below */
        if (h < TM2_CAR_SLOT_CAP && (h = portCarFreeDupKey()) >= 0) {
            key = (16 + h) << 4;
            if (h == CG.dup_count)
                CG.dup_count++;
        }
#else
        if (h < TM2_CAR_SLOT_CAP && CG.dup_count < TM2_CAR_KEYS - 16)
            key = (16 + CG.dup_count++) << 4;
#endif
        CG.group_key[g] = key;
        CG.key_type[key >> 4] = type;
    }
    CG.slot_key[slot] = key;
}

/* The registration functions (carBodyObjectRegister, carFreezeBuild, ...): the key of the
 * group being installed, 0 when its root has not registered (the callers then fall back to
 * the retail index). */
int PORT_TEXT portCarLoadKey(void)
{
    if ((unsigned int)vramTexSlotGroup >= TM2_CAR_SLOT_CAP)
        return 0;
    return CG.group_key[vramTexSlotGroup];
}

/* The key of car slot `slot` (record +0x004 human / +0x006 AI); `type` when it has none. */
int PORT_TEXT portCarKey(int slot, int type)
{
    if ((unsigned int)slot < TM2_CAR_SLOT_CAP && CG.slot_key[slot] != 0)
        return CG.slot_key[slot];
    return type;
}

/* The car type of a key (a key below 0x100 is a type). */
int PORT_TEXT portCarKeyType(int key)
{
    if (key < 0x100)
        return key;
    if ((unsigned int)(key >> 4) >= TM2_CAR_KEYS)
        return 0;
    return CG.key_type[key >> 4];
}

/* The key of the car slot whose world object is `object`, 0 if none. */
int PORT_TEXT portCarKeyOfObject(void *object)
{
    int i;

    for (i = 0; i < carWorldObjectCount && i < TM2_CAR_SLOT_CAP; i++)
        if (carSlotObjects[i] == object)
            return CG.slot_key[i];
    return 0;
}

/* The first car slot of load group `group` (human i = group i + 1, opponent i = group
 * humans + i + 1: sessionLoadArenaAndCars), -1 if none. */
int PORT_TEXT portCarSlotOfGroup(int group)
{
    int i;

    for (i = 0; i < carWorldObjectCount && i < TM2_CAR_SLOT_CAP; i++)
        if (CG.slot_group[i] == group)
            return i;
    return -1;
}

/* Retail's objRegistrySetStep(type + 6, 0, value) for the turbo flame, by key (the ids
 * type + 6 match no case of its switch, so only animSetStep on the turbo object runs). */
void PORT_TEXT portCarTurboSetStep(int key, int value)
{
    if ((unsigned int)(key >> 4) < TM2_CAR_KEYS)
        animSetStep(CG.turbo[key >> 4], value);
}

/* carPartsSpinHammerhead/Minion/MrSlam/DarkTooth/Axel (car_parts_spin.c, world_fall_limit.c:
 * one pattern) for the key's parts and angle: turn the angle by amount / 7 and rotate each
 * registered part about X by it. */
void PORT_TEXT portCarPartsSpin(int key, int amount)
{
    struct { short m[3][3]; short pad; int t[3]; } m;
    int i, j, k;

    k = key >> 4;
    if ((unsigned int)k >= TM2_CAR_KEYS)
        return;
    CG.part_angle[k] -= amount / 7;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++)
            m.m[i][j] = (i == j) << 12;
        m.t[i] = 0;
    }
    RotMatrixX(-CG.part_angle[k], &m);
    for (i = 0; i < 6; i++)
        if (CG.parts[k][i])
            mathCopyWords((unsigned char *)CG.parts[k][i] + 4, &m, 5);
}
#undef CG
#endif /* TM2_PORT_CAR_GROUPS */

#ifdef TM2_PORT_CARS
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif

/* The start list (startNodeList 801B2B28, ai_start_nodes.c): route node ids s16[15], then taken
 * flags u8[15] at +0x1E. Route nodes (aiRouteNodes 801B1DE8): 24 bytes, x and y first. */
extern unsigned char startNodeList[];
struct PortRouteNode {
    int x, y;
    short z, next[6];
    signed char count, route;
};
extern struct PortRouteNode aiRouteNodes[];
extern int humanCarTotal, aiCarTotal;
extern int rand(void);
#define START_ID(i) (((short *)startNodeList)[i])
#define START_TAKEN(i) (startNodeList[0x1E + (i)])
#define START_MAX 15
/* positions and HP in the car records (cars.h: human pos +0x6FC, stats +0x14C with hp at +0xA4;
 * AI pos +0x73C, hp +0x5A0) */
#define HUMAN_X(i) (humanCars[i][0x6FC / 4])
#define HUMAN_Y(i) (humanCars[i][0x700 / 4])
#define HUMAN_HP(i) (humanCars[i][0x1F0 / 4])
#define AI_X(i) (aiCars[i][0x73C / 4])
#define AI_Y(i) (aiCars[i][0x740 / 4])
#define AI_HP(i) (aiCars[i][0x5A0 / 4])

/* |dx| + |dy|, saturating (map coordinates are well inside +-2^29, this is only a guard). */
static int PORT_TEXT portStartGap(int x, int y, int x2, int y2)
{
    unsigned int dx, dy;

    dx = x > x2 ? (unsigned int)x - (unsigned int)x2 : (unsigned int)x2 - (unsigned int)x;
    dy = y > y2 ? (unsigned int)y - (unsigned int)y2 : (unsigned int)y2 - (unsigned int)y;
    if (dx > 0x3FFFFFFF || dy > 0x3FFFFFFF)
        return 0x7FFFFFFF;
    return (int)(dx + dy);
}

/* startNodePickRandom with portCarOptions bit 2 (SIXTEEN_CARS.md 5.3 A). Same contract: an entry of
 * the start list (0..count-1), its node id returned, marked taken while untaken entries remain.
 *   - no entry taken yet (the round's first car): retail's pick (rand() % count, the first untaken
 *     from there), one rand() call;
 *   - else, among the untaken entries the one whose node is farthest (largest smallest |dx| + |dy|)
 *     from the taken entries' nodes (the cars placed since startNodesClearUsed); no rand();
 *   - all taken: the entry farthest from the live cars placed this round (request 49: the humans and
 *     AI cars whose start ran since startNodesClearUsed, portStartPlaced, HP > 0; a car not placed yet
 *     keeps its last match's position and HP and is not counted), not marked; ties go to the entry
 *     picked the fewest times this round (so cars past the list spread over it at the round's start,
 *     where every entry has a car on it), then the lower entry; with no such car, a random entry as retail.
 * Ties of the other cases go to the lower entry. While a respawn or drop-in of HUMANS16_PLAN.md H5 runs, portRound.start_node
 * forces the node it picked (> 0: node + 1) or the last case (< 0); it is 0 at every other time. */
/* ---- request 85 (SIXTEEN_CARS.md 5.5): a start along a route link when every start node is near a car ---- */
extern int aiRouteNodeCount;
extern int worldGroundHeightAt(int *pos);
extern int ratan2(int y, int x);

/* The squared straight-line distance in 1/16 position units (each axis clamped, so it fits 31 bits). */
static int PORT_TEXT portStartDist2(int dx, int dy)
{
    dx /= 16;
    dy /= 16;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx > 30000)
        dx = 30000;
    if (dy > 30000)
        dy = 30000;
    return dx * dx + dy * dy;
}

/* The nearest live car placed this round (request 49's, as the "all taken" case counts them) from (x, y), as
 * portStartDist2; 0x7FFFFFFF when there is none. */
static int PORT_TEXT portStartNearest(int x, int y)
{
    int j, d, gap = 0x7FFFFFFF;

    for (j = 0; j < humanCarTotal && j < TM2_MAX_HUMANS; j++) {
        if (!portStartPlaced.car[j] || HUMAN_HP(j) <= 0)
            continue;
        d = portStartDist2(x - HUMAN_X(j), y - HUMAN_Y(j));
        if (d < gap)
            gap = d;
    }
    for (j = 0; j < aiCarTotal && j < TM2_MAX_AI_CARS; j++) {
        if (!portStartPlaced.car[TM2_PORT_START_AI + j] || AI_HP(j) <= 0)
            continue;
        d = portStartDist2(x - AI_X(j), y - AI_Y(j));
        if (d < gap)
            gap = d;
    }
    return gap;
}

/* The point along the route's links farthest from the live placed cars: each link (node i to each of its next[])
 * at len / 256 points (2..16 parts, the ends excluded; len = |dx| + |dy|), the first of equals. Sets portStartLink and
 * returns 1 when it is at least TM2_START_LINK_CLEAR away and its ground (worldGroundHeightAt, as portStartLinkPlace
 * probes it) is not more than TM2_START_LINK_DROP below the link's lower end; else 0. */
static int PORT_TEXT portStartLinkFind(void)
{
    struct TM2PortStartLink *l = &portStartLink;
    int i, j, s, n, m, dx, dy, x, y, g, len, low, q[3], best = -1;

    for (i = 0; i < aiRouteNodeCount && i < 1024; i++)
        for (j = 0; j < aiRouteNodes[i].count && j < 6; j++) {
            m = aiRouteNodes[i].next[j];
            if (m < 0 || m >= aiRouteNodeCount || m == i)
                continue;
            dx = aiRouteNodes[m].x - aiRouteNodes[i].x;
            dy = aiRouteNodes[m].y - aiRouteNodes[i].y;
            len = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            n = len / 256;
            if (n < 2)
                n = 2;
            if (n > 16)
                n = 16;
            for (s = 1; s < n; s++) {
                x = aiRouteNodes[i].x + dx / n * s + dx % n * s / n;
                y = aiRouteNodes[i].y + dy / n * s + dy % n * s / n;
                g = portStartNearest(x, y);
                if (g > best) {
                    best = g;
                    l->x = x;
                    l->y = y;
                    l->from = i;
                    l->to = m;
                }
            }
        }
    if (best < (TM2_START_LINK_CLEAR / 16) * (TM2_START_LINK_CLEAR / 16))
        return 0;
    l->z = aiRouteNodes[l->from].z > aiRouteNodes[l->to].z ? aiRouteNodes[l->from].z : aiRouteNodes[l->to].z;
    /* over a gap (Roof Tops' links cross between roofs) the ground lies far below both ends: no link start */
    q[0] = l->x;
    q[1] = l->y;
    q[2] = l->z + TM2_START_LINK_PROBE;
    low = aiRouteNodes[l->from].z < aiRouteNodes[l->to].z ? aiRouteNodes[l->from].z : aiRouteNodes[l->to].z;
    if (worldGroundHeightAt(q) < low - TM2_START_LINK_DROP)
        return 0;
    l->set = 1;
    return 1;
}

/* humanCarPlaceAtStart / aiCarStart, after the node placement (request 85): when the pick left a link start, the
 * car's x and y (pos[0..1]), the ground height there (*ground: worldGroundHeightAt probed from TM2_START_LINK_PROBE
 * above the link's higher end), the heading along the link (*heading, as humanCarPlaceAtStart turns to a next node)
 * and the link's second node (*to); returns 1 and forgets it. Else 0 and nothing written. No rand(). */
int PORT_TEXT portStartLinkPlace(int *pos, int *heading, int *ground, int *to)
{
    struct TM2PortStartLink *l = &portStartLink;
    int q[3], dx, dy;

    if (!l->set)
        return 0;
    l->set = 0;
    q[0] = l->x;
    q[1] = l->y;
    q[2] = l->z + TM2_START_LINK_PROBE;
    pos[0] = l->x;
    pos[1] = l->y;
    *ground = worldGroundHeightAt(q);
    dx = aiRouteNodes[l->to].x - aiRouteNodes[l->from].x;
    dy = aiRouteNodes[l->to].y - aiRouteNodes[l->from].y;
    if (dy == 0)
        dy = 1;
    *heading = ratan2(dx, dy);
    *to = l->to;
    return 1;
}

int PORT_TEXT portStartNodePick(int count)
{
    int i, j, taken, best, bestGap, gap, d, x, y;

    portStartLink.set = 0;      /* request 85: a link start is only for the car of this pick */
    if (portRound.start_node > 0)
        return portRound.start_node - 1;    /* H5: the start node the port picked (portHumanRespawn / StartHumanAt) */
    if (count > START_MAX)
        count = START_MAX;
    if (count <= 0)
        return START_ID(0);
    taken = 0;
    for (i = 0; i < count; i++)
        if (START_TAKEN(i))
            taken++;
    if (portRound.start_node < 0)
        taken = count;      /* H5: a respawn takes the start farthest from the live cars */
#ifdef TM2_PORT_CAR_DROPIN
    if (portCarDropIn.start_live)
        taken = count;      /* request 33: a car starting mid-match takes the start farthest from the live cars */
#endif
    if (taken == 0) {
        i = rand() % count;
        START_TAKEN(i) = 1;
        portStartPlaced.picks[i]++;
        return START_ID(i);
    }
    best = -1;
    bestGap = -1;
    if (taken < count) {
        for (i = 0; i < count; i++) {
            if (START_TAKEN(i))
                continue;
            x = aiRouteNodes[START_ID(i)].x;
            y = aiRouteNodes[START_ID(i)].y;
            gap = 0x7FFFFFFF;
            for (j = 0; j < count; j++) {
                if (!START_TAKEN(j))
                    continue;
                d = portStartGap(x, y, aiRouteNodes[START_ID(j)].x, aiRouteNodes[START_ID(j)].y);
                if (d < gap)
                    gap = d;
            }
            if (gap > bestGap) {
                bestGap = gap;
                best = i;
            }
        }
        START_TAKEN(best) = 1;
        portStartPlaced.picks[best]++;
        return START_ID(best);
    }
    for (i = 0; i < count; i++) {
        x = aiRouteNodes[START_ID(i)].x;
        y = aiRouteNodes[START_ID(i)].y;
        gap = 0x7FFFFFFF;
        for (j = 0; j < humanCarTotal && j < TM2_MAX_HUMANS; j++) {
            if (!portStartPlaced.car[j] || HUMAN_HP(j) <= 0)
                continue;       /* request 49: only the cars placed this round */
            d = portStartGap(x, y, HUMAN_X(j), HUMAN_Y(j));
            if (d < gap)
                gap = d;
        }
        for (j = 0; j < aiCarTotal && j < TM2_MAX_AI_CARS; j++) {
            if (!portStartPlaced.car[TM2_PORT_START_AI + j] || AI_HP(j) <= 0)
                continue;
            d = portStartGap(x, y, AI_X(j), AI_Y(j));
            if (d < gap)
                gap = d;
        }
        if (gap > bestGap || (gap == bestGap && portStartPlaced.picks[i] < portStartPlaced.picks[best])) {
            bestGap = gap;
            best = i;
        }
    }
    if (bestGap == 0x7FFFFFFF)
        best = rand() % count;   /* no live car */
    portStartPlaced.picks[best]++;
    /* request 85: the chosen node within TM2_START_LINK_CLEAR of a live car: a clear point along a link if any */
    if (bestGap != 0x7FFFFFFF &&
        portStartNearest(aiRouteNodes[START_ID(best)].x, aiRouteNodes[START_ID(best)].y) <
            (TM2_START_LINK_CLEAR / 16) * (TM2_START_LINK_CLEAR / 16) &&
        portStartLinkFind())
        return portStartLink.from;
    return START_ID(best);
}

/* startNodesClearUsed (request 49): a new round's starts; no car is placed yet. */
void PORT_TEXT portStartPlacedClear(void)
{
    int i;

    for (i = 0; i < TM2_PORT_START_CARS; i++)
        portStartPlaced.car[i] = 0;
    for (i = 0; i < 16; i++)
        portStartPlaced.picks[i] = 0;
}

/* humanCarPlaceAtStart / aiCarStart around their start pick (request 49): `placed` 0 before the pick (the car being
 * placed is not counted from its old position), 1 after it. `car` is the car's record (human: humanCars,
 * else aiCars). */
void PORT_TEXT portStartPlacedMark(void *car, int human, int placed)
{
    int i;

    if (human) {
        i = ((unsigned char *)car - (unsigned char *)humanCars) / 0x814;
        if ((unsigned int)i >= TM2_MAX_HUMANS)
            return;
    } else {
        i = ((unsigned char *)car - (unsigned char *)aiCars) / 0x854;
        if ((unsigned int)i >= TM2_MAX_AI_CARS)
            return;
        i += TM2_PORT_START_AI;
    }
    portStartPlaced.car[i] = placed;
}
#undef START_ID
#undef START_TAKEN

extern struct TM2Settings sessionSettings;
extern char *carTmsFileName(void *settings, int car);
extern char *carDmdFileName(void *settings, int car);
extern int fileioLoadFileIntoRam(unsigned char *dst, char *name);

/* sessionLoadArenaAndCars's one way to load a car file (TM2_PORT_CARS): the buffer the port set in
 * portCarSource[group] for this pass when it has one and it fits below `end` (bytes copied, nothing
 * read from the disc), else the disc file of car-select index `car`, exactly as retail loads it. */
#ifdef TM2_PORT_CAR_DROPIN
static void PORT_TEXT portDropInNoteLoad(unsigned char *dst, int group, int dmd, int size);
#endif

int PORT_TEXT portCarLoadGroup(unsigned char *dst, int group, int car, int dmd, unsigned int end)
{
    const unsigned char *src;
    int size, i;

#ifdef TM2_PORT_CAR_DROPIN
    if (!dmd)
        portDropInNoteLoad(dst, group, 0, 0);   /* request 33 unloading: where this group's image records start */
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    /* request 42 item 3: `car` is a car number; keep it per group (portCarNumberOfGroup), and the disc fallback
       loads its base's files (a mod's DMD uses its base's ids, PER_CAR_PLAN.md E.1) */
    if ((unsigned int)group < TM2_PORT_NUMBER_GROUPS)
        portCarNumbers.group[group] = car + 1;
    if (car >= TM2_PORT_MOD_FIRST)
        car = portCarNumberBase(car);
#endif
    if ((unsigned int)group <= TM2_MAX_LOADED_CARS) {
        src = dmd ? portCarSource[group].dmd : portCarSource[group].tms;
        size = dmd ? portCarSource[group].dmd_size : portCarSource[group].tms_size;
        if (src != 0 && size > 0 && (unsigned int)dst < end && (unsigned int)size <= end - (unsigned int)dst) {
            for (i = 0; i < size; i++)
                dst[i] = src[i];
#ifdef TM2_PORT_CAR_DROPIN
            if (dmd)
                portDropInNoteLoad(dst, group, 1, size);    /* its DMD copy, for an unload */
#endif
            return size;
        }
    }
#ifdef TM2_PORT_CAR_DROPIN
    size = fileioLoadFileIntoRam(dst, dmd ? carDmdFileName(&sessionSettings, car)
                                          : carTmsFileName(&sessionSettings, car));
    if (dmd)
        portDropInNoteLoad(dst, group, 1, size);
    return size;
#else
    return fileioLoadFileIntoRam(dst, dmd ? carDmdFileName(&sessionSettings, car)
                                          : carTmsFileName(&sessionSettings, car));
#endif
}
#endif /* TM2_PORT_CARS */

#if defined(TM2_PORT_CAR_SWAP) || defined(TM2_PORT_CAR_SPECIALS) || defined(TM2_PORT_BALANCE)
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- the car swap (PC port request 32 step 3, docs/CAR_GROUPS.md 8, PER_CAR_PLAN.md B with the port's
 * answers G.3-G.5) ----
 * A record (human i or AI j) drives the car (load group, slot, key) bound to it. portCarSetGroup moves a
 * record to another loaded car; the record that drove that car (if any) takes the first one's car. The
 * pieces are separate functions so that request 33's remove / start-mid-match (PER_CAR_PLAN.md B.6) can
 * reuse them. Nothing here runs unless the port calls it (sessionBindCars reads portCarBindGroup through
 * portCarBindGroupOf / portCarBindTypeOf, which return their argument while no swap was made). */
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#include "semantic/cars.h"

extern void carBodyObjectShow(int key);
extern void carSetDamageStage(int key, int stage, int arg);
extern void carStatsLoad(struct TM2CarStatsBlock *stats, int type);
extern int carStatsSetSpeedMult(struct TM2CarStatsBlock *stats, int value);
extern void carWheelsSetStepHeight(struct TM2CarWheels *wheels, int type);
extern void weaponBlockInit(void *car, struct TM2CarWeapons *weapons, unsigned char human);
extern void viewCameraOffsetsInit(struct TM2HumanRecord *human);
extern void aiCarSetupProfile(struct TM2AIRecord *ai);
extern int lightGetEnv(int);
extern void csSetDrawEnable(void *object, unsigned char value);
extern void clutAnimFadeIn(int id, int index);
extern void objRegistrySetStep(int id, int index, int value);
extern int soundStop(int id);
extern unsigned char soundIsActive(int id);

/* Retail's single special states (round_reset.c and the views of weapon_tornado.c, weapon_basic.c,
 * strike_update.c, weapon_leap_homing_init.cpp; PER_CAR_PLAN.md A.2 rows 3-6). */
struct PortTornadoState {           /* weaponTornadoState 80184DC0 (Twister) */
    short timer, kind, angle, f6;
    int *anchor;                    /* the firer's stats.hp */
    struct TM2CarState *flags;      /* the firer's state */
    struct TM2Object *object;       /* the firer's car object */
};
struct PortLeapState {              /* homingLeapState 80184DD4 (Grasshopper) */
    signed char active, source_mode;
    unsigned char pad[10];
    unsigned short owner_id;        /* the leaping car's slot */
    short age;                      /* +0x0E updates since the launch (request 41 bit 14) */
    unsigned char pad10[0x1C];
    int *position;                  /* +0x2C her motion.pos (request 41 bit 14) */
};
struct PortStrikeBlock {            /* weaponStrikeState 80184E10 (Shadow) */
    unsigned char mode;
    char f1;
    unsigned short owner_id;
    short active, timer;
    int vectors[9];
    int corners[12];
    struct TM2CarWeapons *weapon;   /* the firer's weapons block */
    struct TM2Object *object;       /* the strike's own object */
};
extern struct PortTornadoState weaponTornadoState;
extern struct TM2Object *weaponTornadoObject, *weaponTornadoFunnelObject;
extern struct PortLeapState homingLeapState;
extern struct PortStrikeBlock weaponStrikeState;
extern struct TM2Object *darkToothBossObject;
extern short aiDarkToothShotTimer, aiDarkToothSwoopTimer, darkToothSwooping;
#if defined(TM2_PORT_DARK_TOOTH) && defined(TM2_PORT_CAR_SWAP)
static struct TM2Object *portCarDarkToothHead(int key);
#define DARK_TOOTH_HEAD(key) portCarDarkToothHead(key)      /* request 33-2e: the key's own head */
#else
#define DARK_TOOTH_HEAD(key) darkToothBossObject
#endif
#ifdef TM2_PORT_DARK_TOOTH
/* request 46: a parked car's head hidden and remembered, shown again when the car is driven (portCarDarkToothParked) */
#define DARK_TOOTH_PARK_HEAD(slot, key) portCarDarkToothParkHead(slot)
#else
#define DARK_TOOTH_PARK_HEAD(slot, key) (DARK_TOOTH_HEAD(key)->visible = 0)
#endif
#endif /* TM2_PORT_CAR_SWAP || TM2_PORT_CAR_SPECIALS || TM2_PORT_BALANCE: the declarations they use */

#ifdef TM2_PORT_CAR_SPECIALS
/* ---- per-car specials (PC port request 33 step 2a, docs/CAR_GROUPS.md 10, PER_CAR_PLAN.md C.2, D5-D7) ----
 * The single retail state of Mr. Slam's arm and claw, Twister's tornado, Shadow's strike, Axel's shockwave
 * animation and Grasshopper's leap belongs to the first car of the type (key = type); a duplicate (overflow
 * key (16 + d) << 4) keeps its own copy in portCarSpecials entry d. The game code that uses the state runs
 * unchanged inside a selection: portCarSpecialSelect(key) exchanges the retail storage of the key's special
 * with the key's entry (so the retail globals hold that car's state), portCarSpecialRestore exchanges it
 * back. sel[part] remembers which entry is in the retail storage, so selections nest and a key below 0x100
 * selects the first car's own state (a no-op unless a duplicate is selected). With no duplicate every
 * selection is a no-op: retail's code on retail's storage. */
extern void *slamArmObject, *slamClawObject;           /* 80180F74, 80180F78 (arena_load_register.c) */
extern int slamArmAngle, slamClawAngle;                /* 80180F7C, 80180F80 */
extern void *objRegistryId2324, *objRegistryId2325;    /* 80181064, 80181068 (object_registry_state.c) */
extern int weaponSlamStateCount;                       /* 80180AF0 the claw-record cursor */
#if TM2_MAX_SLAM_RECORDS <= TM2_RETAIL_SLAM_RECORDS
extern short weaponSlamRecords[TM2_MAX_SLAM_RECORDS][18 / 2];   /* round_reset.c (retail storage) */
#endif
extern void slamArmSetAngle(int angle);
extern void slamClawSetAngle(int angle);

#define PS portCarSpecials
#ifdef TM2_PORT_SPECIAL_AS
#define PSA portSpecialAs           /* request 35: the as-entries (entry index TM2_PORT_SPECIAL_KEYS + slot) */
#endif
#define PART_SLAM 0
#define PART_TORNADO 1
#define PART_STRIKE 2
#define PART_AXEL 3
#define PART_LEAP 4
#define PART_DARK_TOOTH 5

#ifdef TM2_PORT_DARK_TOOTH
/* Dark Tooth's single retail state (ai_dark_tooth_boss.c, PER_CAR_PLAN.md A.2 row 6; request 33-2e) */
extern short aiDarkToothShotDelay[];                   /* 80180B4C: 7 x s16 (shot delay .. swoop target) */
extern int darkToothSwoopDelay;                        /* 80180F18: 0x10 bytes (swoop delay, swooping, hover
                                                          height, swoop target position) */
extern int darkToothSwoopVectors[6];                   /* 80183F64 */
extern int aiBossDefeatedFlag;                         /* 8018102C: stage 2 */
extern unsigned char aiBossDefeated(void);
extern void carStatsDarkToothStage2(void *stats);
extern void carBuildCollisionBox(void *car, unsigned char human);
extern void carSpinOut(void *car, int human, int kind, int keep_motion);
#endif

/* The part of a car type's special, -1 if the type has no single special state. */
static int PORT_TEXT portCarSpecialPart(int type)
{
    switch (type) {
    case 0xC0: return PART_SLAM;
    case 0xA0: return PART_TORNADO;
    case 0xD0: return PART_STRIKE;
    case 0xB0: return PART_AXEL;
    case 0x60: return PART_LEAP;
#ifdef TM2_PORT_DARK_TOOTH
    case 0xF0: return PART_DARK_TOOTH;
#endif
    }
    return -1;
}

static void PORT_TEXT portSwapWords(void *a, void *b, int n)
{
    int *p = (int *)a, *q = (int *)b, t;

    while (n-- > 0) {
        t = *p;
        *p++ = *q;
        *q++ = t;
    }
}

#ifdef TM2_PORT_DARK_TOOTH
static void PORT_TEXT portSwapHalves(short *a, short *b, int n)
{
    short t;

    while (n-- > 0) {
        t = *a;
        *a++ = *b;
        *b++ = t;
    }
}
#endif

/* Exchange the retail storage of a part with entry d. */
static void PORT_TEXT portCarSpecialExchange(int part, int d)
{
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35: entries TM2_PORT_SPECIAL_KEYS + s are car slot s's as-entry (a borrowed strike or shockwave) */
    if (d >= TM2_PORT_SPECIAL_KEYS) {
        d -= TM2_PORT_SPECIAL_KEYS;
        if (part == PART_STRIKE) {
            portSwapWords(&weaponStrikeState, PSA.strike[d].block, 25);
            portSwapWords(&objRegistryId2325, &PSA.strike[d].anim, 1);
        } else if (part == PART_AXEL) {
            portSwapWords(&objRegistryId2324, &PSA.axel[d].anim, 1);
        }
        return;
    }
#endif
    switch (part) {
    case PART_SLAM:
        portSwapWords(&slamArmObject, &PS.slam[d].arm, 1);
        portSwapWords(&slamClawObject, &PS.slam[d].claw, 1);
        portSwapWords(&slamArmAngle, &PS.slam[d].arm_angle, 1);
        portSwapWords(&slamClawAngle, &PS.slam[d].claw_angle, 1);
        break;
    case PART_TORNADO:
        portSwapWords(&weaponTornadoState, PS.tornado[d].state, 5);
        portSwapWords(&weaponTornadoObject, &PS.tornado[d].tornado, 1);
        portSwapWords(&weaponTornadoFunnelObject, &PS.tornado[d].funnel, 1);
        break;
    case PART_STRIKE:
        portSwapWords(&weaponStrikeState, PS.strike[d].block, 25);
        portSwapWords(&objRegistryId2325, &PS.strike[d].anim, 1);
        break;
    case PART_AXEL:
        portSwapWords(&objRegistryId2324, &PS.axel_anim[d], 1);
        break;
    case PART_LEAP:
        portSwapWords((int *)&weaponTornadoState + 5, PS.leap[d], 15);   /* homingLeapState = 80184DC0 + 0x14 */
        break;
#ifdef TM2_PORT_DARK_TOOTH
    case PART_DARK_TOOTH:
        portSwapHalves(aiDarkToothShotDelay, PS.dark_tooth[d].scalars, 7);
        portSwapWords(&darkToothSwoopDelay, PS.dark_tooth[d].swoop, 4);
        portSwapWords(&darkToothBossObject, &PS.dark_tooth[d].head, 1);
        portSwapWords(darkToothSwoopVectors, PS.dark_tooth[d].vectors, 6);
        portSwapWords(&aiBossDefeatedFlag, &PS.dark_tooth[d].stage, 1);
        break;
#endif
    }
}

/* Put entry `want` (d + 1, 0 = the first car's own state; request 35: TM2_PORT_SPECIAL_KEYS + 1 + slot = the slot's
 * as-entry) of a part into the retail storage. */
static void PORT_TEXT portCarSpecialSwitch(int part, int want)
{
    int cur = PS.sel[part];

    if (cur == want)
        return;
    if (cur != 0)
        portCarSpecialExchange(part, cur - 1);      /* the selected duplicate back to its entry */
    if (want != 0)
        portCarSpecialExchange(part, want - 1);
    PS.sel[part] = want;
}

/* Select the special state of car key `key` (its type's part): the retail globals hold that car's state
 * until portCarSpecialRestore(token). A key below 0x100 (the first car of its type) selects the retail
 * state itself. Returns 0 for a type without a single special (nothing to restore). */
int PORT_TEXT portCarSpecialSelect(int key)
{
    int part, want, prev;

    part = portCarSpecialPart(portCarKeyType(key));
    if (part < 0)
        return 0;
    want = 0;
    if (key >= 0x100 && (unsigned int)((key >> 4) - 16) < TM2_PORT_SPECIAL_KEYS)
        want = (key >> 4) - 15;
    prev = PS.sel[part];
    portCarSpecialSwitch(part, want);
    return (part + 1) << 8 | prev;
}

void PORT_TEXT portCarSpecialRestore(int token)
{
    if (token != 0)
        portCarSpecialSwitch((token >> 8) - 1, token & 0xFF);
}

/* The per-frame loops (weaponTornadoUpdate, weaponHomingLeapUpdate, weaponShadowStrikeUpdate, D6): after
 * the retail body for the first car, select each duplicate key of `type` handed out in this load, in key
 * order, from entry *index on. Returns the selection's token (run the body, then portCarSpecialRestore),
 * 0 when there is no further key. */
int PORT_TEXT portCarSpecialNext(int type, int *index)
{
    int d;

    for (d = *index; d < TM2_PORT_SPECIAL_KEYS; d++)
        if (portCarGroups.key_type[16 + d] == type) {
            *index = d + 1;
            return portCarSpecialSelect((16 + d) << 4);
        }
    *index = d;
    return 0;
}

/* objRegistryAdd ids 2324 (Axel's shockwave) and 2325 (Shadow's strike animation): the object goes to the
 * state of the car being installed (portCarLoadKey; the level's objects and a car before its root: the
 * retail global, as retail). */
void PORT_TEXT portCarSpecialRegister(int id, void *object)
{
    int t = portCarSpecialSelect(portCarLoadKey());

    if (id == 2324)
        objRegistryId2324 = object;
    else
        objRegistryId2325 = object;
    portCarSpecialRestore(t);
}

/* The car key of the record whose stats block is `stats` (carStatsLoad's argument; human +0x14C, AI
 * +0x4FC), 0 if it is no record's. */
int PORT_TEXT portCarKeyOfStats(void *stats)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int i;

    for (i = 0; i < TM2_MAX_HUMANS; i++) {
        h = (struct TM2HumanRecord *)humanCars[i];
        if ((void *)&h->stats == stats)
            return portCarKey(h->slot, h->type);
    }
    for (i = 0; i < TM2_MAX_AI_CARS; i++) {
        a = (struct TM2AIRecord *)aiCars[i];
        if ((void *)&a->stats == stats)
            return portCarKey(a->slot, a->type);
    }
    return 0;
}

/* Claw record `index` of weaponSlamStateAlloc's pool: weaponSlamRecords (retail's 4 at their retail
 * address, or TM2_PORT_CARS' raised table) first, then the TM2_PORT_CLAW_EXTRA records of portCarSpecials:
 * the allocator never hands out retail's fifth record over weaponRicochetPool (RETAIL_BUGS.md 17). */
void *PORT_TEXT portCarClawRecord(int index)
{
    if (index < TM2_MAX_SLAM_RECORDS)
        return weaponSlamRecords[index];
    if (index - TM2_MAX_SLAM_RECORDS < TM2_PORT_CLAW_EXTRA)
        return PS.claw[index - TM2_MAX_SLAM_RECORDS];
    return PS.claw[TM2_PORT_CLAW_EXTRA - 1];
}

/* portCarGroupsReset (carTablesClear, each load; in arenaResetSystems before carPartsReset and
 * weaponObjectsDetach clear the first cars' objects): every duplicate's state forgotten, as those two clear
 * retail's (objects 0, Mr. Slam's angles 11), and no entry selected. */
void PORT_TEXT portCarSpecialsReset(void)
{
    int *p = (int *)&PS;
    int i;

    for (i = 0; i < (int)(sizeof(PS) / 4); i++)
        p[i] = 0;
    for (i = 0; i < TM2_PORT_SPECIAL_KEYS; i++) {
        PS.slam[i].arm_angle = 11;
        PS.slam[i].claw_angle = 11;
#ifdef TM2_PORT_DARK_TOOTH
        /* the retail initial values of the 7 scalars (ai_dark_tooth_boss_scalars.c: 0, 10, 0, 10, 150, 100, 0;
           the others are 0): each duplicate starts with them */
        PS.dark_tooth[i].scalars[1] = 10;
        PS.dark_tooth[i].scalars[3] = 10;
        PS.dark_tooth[i].scalars[4] = 150;
        PS.dark_tooth[i].scalars[5] = 100;
#endif
    }
#ifdef TM2_PORT_DARK_TOOTH
    portCarDtParkMask[0] = 0;       /* request 46: no head hidden by a park yet (the load makes them new) */
    portCarDtParkMask[1] = 0;
#endif
#ifdef TM2_PORT_SPECIAL_AS
    portCarSpecialAsReset();        /* request 35: every slot its own special, no as-entry, no stale model */
#endif
}

/* weaponsRoundReset (each round and each restart), after retail's part for the first cars: the same for
 * every duplicate entry: the leap's active byte and age, the tornado (timer and its two objects off), the
 * strike's active and timer; each duplicate Mr. Slam's arm and claw back to rest (selected, so the setters
 * rotate that car's objects). The claw records past retail's are reset by weaponsRoundReset's own loop
 * (portCarClawRecord). */
void PORT_TEXT portCarSpecialsRoundReset(void)
{
    int d, t;

    for (d = 0; d < TM2_PORT_SPECIAL_KEYS; d++) {
        ((signed char *)PS.leap[d])[0] = 0;                 /* +0x14 of the shared block: active */
        ((short *)PS.leap[d])[(0x22 - 0x14) / 2] = 0;       /* +0x22: age */
        if (((short *)PS.tornado[d].state)[0] > 0) {
            ((short *)PS.tornado[d].state)[0] = 0;
            if (PS.tornado[d].tornado != 0)
                ((int *)PS.tornado[d].tornado)[2] = 0;      /* +8 */
            if (PS.tornado[d].funnel != 0)
                ((int *)PS.tornado[d].funnel)[2] = 0;
        }
        ((short *)PS.strike[d].block)[2] = 0;               /* +4 active */
        ((short *)PS.strike[d].block)[3] = 0;               /* +6 timer */
        if (portCarGroups.key_type[16 + d] == 0xC0) {
            t = portCarSpecialSelect((16 + d) << 4);
            slamClawSetAngle(739);
            slamArmSetAngle(-113);
            portCarSpecialRestore(t);
        }
    }
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35: every as-entry too (the strike's active and timer, as retail's; the ring hidden) */
    for (d = 0; d < TM2_CAR_SLOT_CAP; d++) {
        ((short *)PSA.strike[d].block)[2] = 0;
        ((short *)PSA.strike[d].block)[3] = 0;
        if (PSA.axel[d].ring != 0)
            ((struct TM2Object *)PSA.axel[d].ring)->visible = 0;
    }
#endif
}

#ifdef TM2_PORT_DARK_TOOTH
/* ---- Dark Tooth per car (PC port request 33 step 2e, docs/CAR_GROUPS.md 11, PER_CAR_PLAN.md C.2 33-2e) ----
 * The head (darkToothBossObject, asset 249, made per BB.DMD install by weaponAssetBind 0xF9 inside the load
 * key's selection), the controller's timers and swoop state and the stage flag (aiBossDefeatedFlag) are the
 * sixth part of the selection mechanism above. aiDarkToothBossUpdate runs inside its car's selection
 * (aiWeaponFireDecide), so every Dark Tooth moves its own head with its own timers; aiBossDefeated() reads the
 * selected car's stage. Retail's rules stay: the stage-2 change is aiCars[0]'s on level 8 (aiCarsCountRemaining)
 * and the round end reads aiCars[0]'s car. portCarOptions bit 6 adds SURVIVAL.md 1.5/1.6. */

/* aiBossDefeated() of the car with key `key` (carContactApplyObject: the car touching the marker). */
int PORT_TEXT portCarBossDefeated(int key)
{
    int t = portCarSpecialSelect(key);
    int r = aiBossDefeated();

    portCarSpecialRestore(t);
    return r;
}

/* carApplyDamage, a Dark Tooth's death, inside its selection: retail sets wreck state 3. With bit 6 a final
 * death (in stage 2) sets 4, so carWreckUpdate does not burst the wreck again and again (SURVIVAL.md 1.4,
 * 1.5 #6); the tournament boss (level 8, aiCars[0]) keeps retail's 3. `ai` = the AI record, 0 for a human (with
 * TM2_PORT_DT_PLAYER always 4: one burst of his model, no endless wreck bursts). */
int PORT_TEXT portCarDarkToothWreck(void *ai)
{
#ifdef TM2_PORT_DT_PLAYER
    if (ai == 0)
        return 4;       /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth dies as a normal car */
#endif
    if (ai != 0 && portCarDarkToothOrdinary(((struct TM2AIRecord *)ai)->slot)) {
        /* request 42 item 1 (bit 10): an ordinary Dark Tooth dies as a car: one burst of his truck (stage 0, the
           caller's), a normal final wreck, his head hidden (it would freeze where it was: retail's stage-1 wait) */
        if (darkToothBossObject != 0)
            darkToothBossObject->visible = 0;
        darkToothSwooping = 0;
        return 4;
    }
    if (!TM2_PORT_BOSS_ANYWHERE || !aiBossDefeated())
        return 3;
    if (ai != 0 && ai == (void *)aiCars[0] && gameGetLevel() == 8)
        return 3;
    return 4;
}

/* Bit 6, from aiCarsCountRemaining each frame (SURVIVAL.md 1.6): every AI Dark Tooth but the tournament boss
 * (level 8, aiCars[0]: retail's own stage code) that is down (HP <= 0, not parked: wreck state 4) and still in
 * stage 1 waits for its wreck timer: it counts as remaining (the round does not end), spins while the timer is
 * >= 121, and at 0 enters stage 2 where it is, as retail does it for aiCars[0] but without ending the round,
 * without Hong Kong's position clamps: stage flag 1, carStatsDarkToothStage2 (HP 110), collision box, alive.
 * Its truck stays hidden (carApplyDamage hid it) and its head comes down onto the car by itself (hover -24).
 * Returns the number of Dark Tooths waiting. */
int PORT_TEXT portCarDarkToothStages(void)
{
    struct TM2AIRecord *a;
    int i, t, n = 0;

    for (i = 0; i < aiCarTotal && i < TM2_MAX_AI_CARS; i++) {
        a = (struct TM2AIRecord *)aiCars[i];
        if (a->type != 0xF0 || a->stats.hp > 0 || a->stats.wreck_state == 4)
            continue;
        if (i == 0 && gameGetLevel() == 8)
            continue;
        if (portCarDarkToothOrdinary(a->slot))
            continue;       /* request 42 item 1 (bit 10): an ordinary Dark Tooth has no stage 2 */
        t = portCarSpecialSelect(portCarKey(a->slot, a->type));
        if (!aiBossDefeatedFlag) {
            n++;
            if (a->stats.wreck_timer <= 0) {
                aiBossDefeatedFlag = 1;
                carStatsDarkToothStage2(&a->stats);
                carBuildCollisionBox(a, 0);
                a->state.dead = 0;
            } else if (a->stats.wreck_timer >= 121) {
                carSpinOut(a, 0, 12, 0);
            }
        }
        portCarSpecialRestore(t);
    }
    return n;
}

/* worldUpdate's round end (retail: `if (aiBossDefeatedFlag) { head hidden; event 3 after 210 }`): the stage
 * flag of aiCars[0]'s car, which is the only one retail sets (with no duplicate this is the retail global).
 * With bit 6 and aiCars[0] not a Dark Tooth it is the first Dark Tooth's (the shorter delay after its
 * stage 2). */
int PORT_TEXT portCarDarkToothRoundEnd(void)
{
    struct TM2AIRecord *a = (struct TM2AIRecord *)aiCars[0];
    int t, f;

    t = portCarSpecialSelect(portCarKey(a->slot, a->type));
    f = aiBossDefeatedFlag;
    if (f && darkToothBossObject != 0)
        darkToothBossObject->visible = 0;
    portCarSpecialRestore(t);
    return f;
}

/* arenaSetup (each sessionBindCars) clears the retail flag; the duplicates' flags with it. */
void PORT_TEXT portCarDarkToothClearStages(void)
{
    int d;

    for (d = 0; d < TM2_PORT_SPECIAL_KEYS; d++)
        PS.dark_tooth[d].stage = 0;
}

/* weaponProjectileUpdate's missile wobble (type 61): is `owner` (the missile's owner id) the id of a Dark
 * Tooth's car object? Retail compares with the first Dark Tooth only (carObjectFromOwner(240)); with one Dark
 * Tooth both are the same test. */
int PORT_TEXT portCarDarkToothShooter(int owner)
{
    struct TM2Object *o;
    int i;

    for (i = 0; i < carWorldObjectCount && i < TM2_CAR_SLOT_CAP; i++) {
        if (portCarKeyType(portCarGroups.slot_key[i]) != 0xF0)
            continue;
        o = (struct TM2Object *)carSlotObjects[i];
        if (o != 0 && o->id == owner)
            return 1;
    }
    return 0;
}

/* For the port (SURVIVAL.md 1.6, "a head as its own enemy"): the stage flag of the Dark Tooth in car slot
 * `slot`; with stage >= 0 also set it (0 stage 1, 1 stage 2). Setting it only moves the head down (or up);
 * the rest of a stage change (carStatsDarkToothStage2, carBuildCollisionBox, hiding the truck object) is the
 * caller's. Returns the flag before, -1 if the slot holds no Dark Tooth. Call it between frames. */
int PORT_TEXT portCarDarkToothStage(int slot, int stage)
{
    int t, f;

    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || slot >= carWorldObjectCount || carSlotOwners[slot] != 0xF0)
        return -1;
    t = portCarSpecialSelect(portCarKey(slot, 0xF0));
    f = aiBossDefeatedFlag;
    if (stage >= 0)
        aiBossDefeatedFlag = stage;
    portCarSpecialRestore(t);
    return f;
}

/* ---- the ordinary Dark Tooth (PC port request 42 item 1, portCarOptions bit 10, DARK_TOOTH_PLAYER.md 10.1) ----
 * With bit 10 a computer Dark Tooth fights as his first phase does (aiDarkToothBossUpdate on every arena: the
 * missiles, the swoop, his head over the truck, no machine guns; the missile wobble; CLUT rows 240..255 for room)
 * and has none of the boss parts: no stage loop or stage 2, no level-8 tail (retail's stage 2, the Hong Kong route
 * patch, jump zones, warps and boss markers), wreck state 4 at his death with his head hidden; after a won round he
 * takes no damage and his wreck timer is 120, as any car's. A car slot marked in portCarDtBossMask
 * (portCarDarkToothRole) keeps the boss parts (bit 6's, or retail's on level 8). Players' Dark Tooths are never
 * ordinary (request 36's own rules). */

/* 1 = car slot `slot` holds a computer Dark Tooth (an AI record of type 0xF0) that is ordinary: bit 10 set and the
 * slot not in the boss mask. 0 with bit 10 clear, so every game test of it is retail's (or bit 6's) logic. */
int PORT_TEXT portCarDarkToothOrdinary(int slot)
{
    if (!(portCarOptions & TM2_PORT_CAR_OPT_DT_ORDINARY))
        return 0;
    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || slot >= carWorldObjectCount)
        return 0;
    if (carSlotOwners[slot] != 0xF0 || carSlotKinds[slot] != 1)
        return 0;
    return (portCarDtBossMask[slot >> 5] & (1u << (slot & 31))) == 0;
}

/* The port's API (between frames): boss 1 = the Dark Tooth of car slot `slot` keeps the boss parts, 0 = he is
 * ordinary while bit 10 is set; boss < 0 only reads. The mask is the port's: the C never clears it, a slot keeps its
 * role over loads until the port changes it (slots are numbered in load order: the humans' cars first). Returns the
 * role before (1 boss, 0 ordinary), -1 for a slot past the mask. */
int PORT_TEXT portCarDarkToothRole(int slot, int boss)
{
    unsigned int bit;
    int was;

    if ((unsigned int)slot >= 64)
        return -1;
    bit = 1u << (slot & 31);
    was = (portCarDtBossMask[slot >> 5] & bit) != 0;
    if (boss > 0)
        portCarDtBossMask[slot >> 5] |= bit;
    else if (boss == 0)
        portCarDtBossMask[slot >> 5] &= ~bit;
    return was;
}

/* aiWeaponFireDecide, inside a Dark Tooth's selection before his boss update: an ordinary one's head is shown while
 * his car object is drawn (his death hid it; the port's AI lives start him again; a cloak hides both). Nothing with
 * bit 10 clear or for a boss. */
void PORT_TEXT portCarDarkToothHeadShow(void *ai)
{
    struct TM2AIRecord *a = (struct TM2AIRecord *)ai;
    struct TM2Object *car;

    if (!portCarDarkToothOrdinary(a->slot) || darkToothBossObject == 0)
        return;
    car = (struct TM2Object *)carSlotObjects[a->slot];
    darkToothBossObject->visible = car != 0 && car->visible;
}

/* ---- a parked Dark Tooth's head (PC port request 46, docs/PLAYTEST_ANSWERS.md 7) ----
 * The head is the only boss part that is a scene object of its own made shown: weaponAssetBind 0xF9 (each BB.DMD
 * install, in the load key's selection) csAllocs it at the world origin (csReset: position 0, word +8 = 1 shown,
 * +0xC = 1 collides) and only the boss update (aiWeaponFireDecide -> aiDarkToothBossUpdate, a live AI record's)
 * or request 36's player head code moves it. A Dark Tooth loaded but not driven (a parked or spare car, a vacant
 * drop-in, an AI record put out of play: dead with wreck state 4, the way SURVIVAL.md 3.1 / portCarParkAi and the
 * port's own Survival waves park a car) left it standing at the origin, drawn and in the hit pass (hdpPassRun skips
 * an object only when (+8 & +0xC) == 0). Here it is hidden with its car and shown again, over the truck, when a
 * computer record drives the car again. Retail never loads BB.DMD without Dark Tooth driving (level 8's boss), so
 * nothing here runs there; with no Dark Tooth parked every test below is false (retail logic). */
extern void mathMulMatVec(void *m, void *v, void *out);

/* Hide the head of the Dark Tooth in car slot `slot` (his car is parked or out of play) and remember it, so that
 * portCarDarkToothParked shows it again when the car is driven. Nothing for a slot without a Dark Tooth. */
void PORT_TEXT portCarDarkToothParkHead(int slot)
{
    int t;

    if ((unsigned int)slot >= 64 || slot >= carWorldObjectCount || carSlotOwners[slot] != 0xF0)
        return;
    t = portCarSpecialSelect(portCarKey(slot, 0xF0));
    if (darkToothBossObject != 0)
        darkToothBossObject->visible = 0;       /* not drawn, and out of the hit pass (+8 & +0xC == 0) */
    portCarSpecialRestore(t);
    portCarDtParkMask[slot >> 5] |= 1u << (slot & 31);
}

/* aiCarsCountRemaining (worldUpdate, each frame, after the car updates and before the scene is drawn): every Dark
 * Tooth car nobody drives (slot kind 2: parked, spare, vacant; or its AI record dead in wreck state 4: out of play)
 * has its head hidden (portCarDarkToothParkHead); one the park hid whose AI record drives again (started:
 * aiCarStart, portCarStartAi, portCarSetGroup) gets it back, placed at its hover over the truck (the boss update's
 * point: 240 up, -24 in stage 2, in the car's frame) with no swoop, so it does not fly in from where it stood.
 * A player's Dark Tooth (slot kind 0) is request 36's (portCarDtPlayerHead shows and hides it each update). */
void PORT_TEXT portCarDarkToothParked(void)
{
    struct TM2AIRecord *a;
    int offset[4], hover[4];
    unsigned int bit;
    int s, i, t, k;

    for (s = 0; s < carWorldObjectCount && s < 64; s++) {
        if (carSlotOwners[s] != 0xF0)
            continue;
        bit = 1u << (s & 31);
        if (carSlotKinds[s] == 0) {
            portCarDtParkMask[s >> 5] &= ~bit;  /* a player's: request 36's head code */
            continue;
        }
        a = 0;
        if (carSlotKinds[s] == 1 && (unsigned int)(i = carSlotIndices[s]) < TM2_MAX_AI_CARS && i < aiCarTotal)
            a = (struct TM2AIRecord *)aiCars[i];
        if (a == 0 || (a->state.dead && a->stats.wreck_state == 4)) {
            portCarDarkToothParkHead(s);
            continue;
        }
        if (!(portCarDtParkMask[s >> 5] & bit))
            continue;
        portCarDtParkMask[s >> 5] &= ~bit;
        t = portCarSpecialSelect(portCarKey(s, 0xF0));
        if (darkToothBossObject != 0) {
            offset[0] = 0;
            offset[1] = 0;
            offset[2] = aiBossDefeatedFlag ? -24 : 240;
            mathMulMatVec(&a->motion.rot_m, offset, hover);
            for (k = 0; k < 3; k++)
                darkToothBossObject->pos[k] = hover[k] + a->motion.pos[k];
            darkToothSwooping = 0;
            darkToothBossObject->visible = 1;
        }
        portCarSpecialRestore(t);
    }
}

#ifdef TM2_PORT_CAR_SWAP
/* The head of the Dark Tooth with key `key` (the swap's look and park functions). */
static struct TM2Object *PORT_TEXT portCarDarkToothHead(int key)
{
    int t = portCarSpecialSelect(key);
    struct TM2Object *h = darkToothBossObject;

    portCarSpecialRestore(t);
    return h;
}
#endif
#endif /* TM2_PORT_DARK_TOOTH */
#ifdef TM2_PORT_DT_PLAYER
/* ---- a player's Dark Tooth (PC port request 36, docs/DARK_TOOTH_PLAYER.md) ----
 * A human record of type 0xF0 plays the boss's weapons. His head is his car key's darkToothBossObject (made by
 * weaponAssetBind 0xF9 inside the load key's selection); the swoop uses his key's boss words (a human never runs
 * aiDarkToothBossUpdate, so they are free): aiDarkToothSwoopTimer, the pitch and yaw steps, aiDarkToothSwoopTarget
 * (the target: human i, or 0x100 + AI j), darkToothSwooping (0 hover, 1 swoop, 2 back to the hover),
 * darkToothSwoopTarget and darkToothSwoopVectors. The machine-gun button uses the record's own gun_cooldown and
 * gun_side (UpdateGuns' words; his guns are off). Every function here selects his key itself. */
typedef struct { int vx, vy, vz, pad; } PortDtVec;
struct PortDtTarget {
    struct TM2Object *obj;
    int *pos;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarWeapons *weapons;
};
extern void mathMulMatVec(void *m, void *v, void *out);
extern void mathMulTransVec(void *m, void *v, void *out);
extern void *RotMatrixYXZ(void *rot, void *m);
extern int mathBlendValue(int value, int target, int pct);
extern int mathBlendAngle(int value, int target, int pct);
extern int rsin(int a);
extern int ratan2(int y, int x);
extern int *hdpQueryPoint(int id, int kind, void *pos, int arg);
extern void fxExplosion48(void *pos, unsigned char flag);
extern void fxExplosion24(void *pos, unsigned char flag);
extern void fxSmokeA12(void *pos, unsigned char flag);
extern struct TM2FireEffect *fxFlameBurning(void *pos, int arg);
extern void carApplyDamage(int id, int amount, int *dir, int type, int attacker);
extern void weaponFindTargetAhead(int owner, int *out, int mode);
extern int weaponFireDarkToothMissile(int owner, int target, int ang, int pos, int damage);
extern struct TM2Object *humanCarObject(int index);
extern struct TM2Object *aiCarObject(int index);
extern int humanCarCount(void);
extern int aiCarCount(void);
extern int *darkToothSwoopTarget;
extern int soundPlay(int, int, int, int);
extern int carSelectStartCar;
extern char *carBioPictureFiles[];

#define DT_SWOOP_TIMER aiDarkToothShotDelay[3]  /* aiDarkToothSwoopTimer */
#define DT_YAW_STEP aiDarkToothShotDelay[4]     /* aiDarkToothYawStep */
#define DT_PITCH_STEP aiDarkToothShotDelay[5]   /* aiDarkToothPitchStep */
#define DT_TARGET aiDarkToothShotDelay[6]       /* aiDarkToothSwoopTarget */
#define DT_ABS(x) ((x) < 0 ? -(x) : (x))
#define DT_MAX(a, b) ((a) < (b) ? (b) : (a))
/* the boss's distance (ai_dark_tooth_boss.c DISTANCE): Chebyshev, in units of 8 */
#define DT_DIST(l, r) (DT_MAX(DT_ABS((l)[0] - (r)[0]), DT_MAX(DT_ABS((l)[1] - (r)[1]), DT_ABS((l)[2] - (r)[2]))) >> 3)
#define DT_RANGE 500                            /* the boss's swoop range: 4000 world units */

/* The car of a swoop target code (human i, or 0x100 + AI j); 0 if there is none. */
static int PORT_TEXT portDtTarget(int code, struct PortDtTarget *t)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;

    if (code >= 0x100) {
        if (code - 0x100 >= aiCarCount())
            return 0;
        a = aiCarGet(code - 0x100);
        t->obj = aiCarObject(code - 0x100);
        t->pos = a->motion.pos;
        t->state = &a->state;
        t->stats = &a->stats;
        t->weapons = &a->weapons;
    } else {
        if (code < 0 || code >= humanCarCount())
            return 0;
        h = humanCarGet(code);
        t->obj = humanCarObject(code);
        t->pos = h->motion.pos;
        t->state = &h->state;
        t->stats = &h->stats;
        t->weapons = &h->weapons;
    }
    return t->obj != 0;
}

/* A car the head may swoop at: alive and drawn (not dead, not cloaked, not parked). */
static int PORT_TEXT portDtTargetLive(struct PortDtTarget *t)
{
    return t->obj->visible && !t->state->dead && t->stats->hp > 0;
}

/* The boss's stage-1 hover (ai_dark_tooth_boss.c): 240 above the truck in the truck's frame, placed exactly from the
 * car (exact) or, back from a swoop, blended 40 toward it as the boss's head always is; then the boss's sway (pitch
 * +-100 per update, turning between 683 and -682, and the yaw step) while the car is not frozen. Returns the
 * distance left to the hover point (the boss's units of 8). No rand() (check 14). */
static int PORT_TEXT portCarDtPlayerPlace(struct TM2HumanRecord *h, struct TM2Object *head, int exact)
{
    PortDtVec offset, hover;
    int *p = &hover.vx;
    int k;

    offset.vx = 0;
    offset.vy = 0;
    offset.vz = aiBossDefeatedFlag ? -24 : 240;     /* request 118: stage 2 on the car, as the boss's (-24) */
    mathMulMatVec(&h->motion.rot_m, &offset, &hover);
    for (k = 0; k < 3; k++) {
        p[k] += h->motion.pos[k];
        head->pos[k] = exact ? p[k] : mathBlendValue(head->pos[k], p[k], 40);
    }
    if (!h->state.frozen) {
        head->rot[1] += DT_PITCH_STEP;
        if (head->rot[1] >= 683)
            DT_PITCH_STEP = -100;
        if (head->rot[1] < -682)
            DT_PITCH_STEP = 100;
        head->rot[2] += DT_YAW_STEP;
        RotMatrixYXZ(head->rot, head->matrix);
    }
    return DT_DIST(head->pos, p);
}

/* One update of the swoop (aiDarkToothBossUpdate's swoop branch, with the target car of the special): the head
 * blends 70 toward the car with the boss's bob and turns toward it; within 20 (160 world units) it hits: 7
 * damage from him (carApplyDamage: x2 as all his damage, retail), the car set burning (the boss's test), sound 6;
 * the world stops it too. It ends (back to the hover) after 40 updates, when the car is 4000 away from the truck,
 * or when the car is gone. */
#ifdef TM2_PORT_BALANCE
#define DT_SWOOP_WORLD(r) portBalanceSwoopHit(r)  /* request 125 (portBalance2 bit 1): pickups' boxes passed */
#else
#define DT_SWOOP_WORLD(r) (*(r))
#endif
static void PORT_TEXT portCarDtPlayerSwoopStep(struct TM2HumanRecord *h, struct TM2Object *head)
{
    struct PortDtTarget t;
    PortDtVec offset;
    int *goal;
    int k;

    DT_SWOOP_TIMER++;
    if (!portDtTarget(DT_TARGET, &t) || !portDtTargetLive(&t)) {
        darkToothSwooping = 2;
        return;
    }
    goal = t.pos;
    darkToothSwoopTarget = goal;
    for (k = 0; k < 3; k++) {
        darkToothSwoopVectors[k] = head->pos[k];
        head->pos[k] = mathBlendValue(head->pos[k], goal[k], 70);
    }
    head->pos[2] += rsin(DT_SWOOP_TIMER * 200) * 160 / 4095;
    offset.vx = head->pos[0] - goal[0];
    offset.vy = head->pos[1] - goal[1];
    if (offset.vy == 0)
        offset.vy = 1;
    head->rot[2] = mathBlendAngle(head->rot[2], ratan2(offset.vx, offset.vy), 60);
    RotMatrixYXZ(head->rot, head->matrix);
    if (DT_DIST(head->pos, goal) < 20) {
        fxExplosion48(head->pos, 0);
        darkToothSwoopVectors[3] = head->pos[0] - darkToothSwoopVectors[0];
        darkToothSwoopVectors[4] = head->pos[1] - darkToothSwoopVectors[1];
        darkToothSwoopVectors[5] = 0;
        carApplyDamage(t.obj->id, 7, &darkToothSwoopVectors[3], 0, h->slot);
        if (!t.state->burning && !t.state->energy_shield && !t.state->in_water && !t.state->in_fire) {
            t.state->burning = 1;
            t.stats->burn_time = 0;
            t.weapons->fire_effect = fxFlameBurning(goal, 1);
            fxSmokeA12(goal, 0);
            fxExplosion24(goal, 0);
            soundPlay(6, 0, 0, 99);
        }
        darkToothSwooping = 2;
    } else if (DT_SWOOP_WORLD(hdpQueryPoint(head->id, -1, head->pos, 0))) {
        fxExplosion48(head->pos, 0);
        darkToothSwooping = 2;
    }
    if (DT_DIST(h->motion.pos, goal) >= DT_RANGE || DT_SWOOP_TIMER >= 41)
        darkToothSwooping = 2;
}

/* ---- request 118: a player's Dark Tooth in his second phase (portDtPlayer.phase2, DARK_TOOTH_PLAYER.md 12) ----
 * The phase is a function of the switch: his key's stage flag (aiBossDefeatedFlag, selected) is the bit, set at
 * each start and each update. Phase 2 also gives him stage 2's stats at every start (carStatsDarkToothStage2, the
 * boss's) and this look: the truck is not drawn but still collides. Retail hides the boss's truck by its object's
 * word +8, which takes the car out of the hit pass too (no one can bump him); here the car object stays drawn-enabled
 * (+8 1) and its model (+0x11C) is portDtPhase2Tree: a sphere and one hit box (kind 8, draws nothing) the size of the
 * head's own box in BB.DMD (80028460: x -55..56, y -84..75, z 0..141) at the head's stage-2 place (-24): x -55..56,
 * y -84..75, z -24..117 in the car's frame. The head object leaves the hit pass (+0xC 0) so the car's box is the one
 * other cars meet (retail: the head's box, as a plain object). The truck's model is kept per car slot and put back
 * when the phase is off. No rand(). */
extern void carStatsDarkToothStage2(void *stats);
extern unsigned int portDtPhase2Tree[27];  /* .port_bss, defined with the other port storage below */
extern void *portDtPhase2Truck[TM2_MAX_CAR_SLOTS];

static void PORT_TEXT portDtPhase2TreeBuild(void)
{
    unsigned char *r = (unsigned char *)portDtPhase2Tree, *b = r + 28;
    short *m;
    int *v;
    int i;

    for (i = 0; i < (int)sizeof(portDtPhase2Tree); i++)
        r[i] = 0;
    r[0] = 1;                           /* kind 1: the bounding sphere, as the truck's root 80025738 */
    r[1] = 7;
    *(unsigned short *)(r + 2) = 0xF0;  /* the truck root's id */
    v = (int *)(r + 4);
    v[0] = 0;                           /* centre and radius around the box */
    v[1] = -4;
    v[2] = 46;
    v[3] = 128;
    r[20] = 1;                          /* one child */
    *(unsigned char **)(r + 24) = b;
    b[0] = 8;                           /* kind 8: a hit box of 6 planes, as the truck's body box 8002568C */
    b[1] = 0xFF;
    b[4] = 6;
    b[5] = 0xFF;                        /* the plane hit: the one the point came through */
    m = (short *)(b + 16);              /* inward normals and depths: z > -24, z < 117, x > -55 */
    m[2] = 4096;
    m[5] = -4096;
    m[6] = 4096;
    v = (int *)(b + 36);
    v[0] = 24;
    v[1] = 117;
    v[2] = 55;
    m = (short *)(b + 48);              /* x < 56, y > -84, y < 75 */
    m[0] = -4096;
    m[4] = 4096;
    m[7] = -4096;
    v = (int *)(b + 68);
    v[0] = 56;
    v[1] = 84;
    v[2] = 75;
}

/* Car slot `slot`'s look: phase 2 (on) the hit-only model in the truck's place and the head out of the hit pass;
 * off the truck back (when it was replaced) and the head in the hit pass again. */
static void PORT_TEXT portDtPhase2Look(int slot, struct TM2Object *head, int on)
{
    unsigned char *obj;
    void **model;

    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || slot >= carWorldObjectCount ||
        (obj = (unsigned char *)carSlotObjects[slot]) == 0)
        return;
    model = (void **)(obj + 0x11C);
    if (on) {
        if (*model != (void *)portDtPhase2Tree) {
            portDtPhase2TreeBuild();
            portDtPhase2Truck[slot] = *model;
            *model = (void *)portDtPhase2Tree;
        }
        if (head != 0)
            head->f00C = 0;
    } else if (*model == (void *)portDtPhase2Tree && portDtPhase2Truck[slot] != 0) {
        *model = portDtPhase2Truck[slot];
        portDtPhase2Truck[slot] = 0;
        if (head != 0)
            head->f00C = 1;
    }
}

/* Inside his key's selection: the stage flag and the look from the switch. */
static void PORT_TEXT portCarDtPlayerPhase(struct TM2HumanRecord *h, struct TM2Object *head)
{
    int on = TM2_DT_PLAYER_PHASE2(h->player);

    aiBossDefeatedFlag = on;
    portDtPhase2Look(h->slot, head, on);
}

/* The swap's computer driver of a Dark Tooth car slot: stage 1 and the truck (a phase-2 player drove it before). */
static void PORT_TEXT __attribute__((unused)) portCarDtPhase2Off(int slot)
{
    int t;

    t = portCarSpecialSelect(portCarKey(slot, 0xF0));
    if (portDtPhase2Truck[slot < TM2_MAX_CAR_SLOTS && slot >= 0 ? slot : 0] != 0) {
        aiBossDefeatedFlag = 0;
        portDtPhase2Look(slot, darkToothBossObject, 0);
    }
    portCarSpecialRestore(t);
}

/* humanCarPlaceAtStart and the swap (portCarApplyType), right after carStatsLoad and before a human's +20: for a
 * human Dark Tooth his phase; in phase 2 stage 2's stats (the boss's: 110 HP, so 130 in play; mass 50, so not the
 * x2 of mass 100; the small chassis and ride height 50; carBuildCollisionBox follows in both callers). */
void PORT_TEXT portCarDtPlayerStart(void *human)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)human;
    int t;

    if (h->type != 0xF0)
        return;
    t = portCarSpecialSelect(portCarKey(h->slot, h->type));
    portCarDtPlayerPhase(h, darkToothBossObject);
    portCarSpecialRestore(t);
    if (TM2_DT_PLAYER_PHASE2(h->player))
        carStatsDarkToothStage2(&h->stats);
}

/* humanCarTick (after weaponTick), for a human record of type 0xF0: his head. Shown while his car object is drawn
 * and he is alive (hidden at death, while cloaked, parked); hovering exactly over the truck, swooping, or blending
 * back after a swoop until within 16 world units (then exact again). */
void PORT_TEXT portCarDtPlayerHead(void *human)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)human;
    struct TM2Object *head, *car;
    int t, show;

    if (h->type != 0xF0)
        return;
    t = portCarSpecialSelect(portCarKey(h->slot, h->type));
    head = darkToothBossObject;
    portCarDtPlayerPhase(h, head);      /* request 118: his phase (the stage flag, the truck's look) from the switch */
    if (head != 0) {
        car = humanCarObject(h->player);
        show = car != 0 && car->visible && !h->state.dead && h->stats.hp > 0;
        head->visible = show;
        if (!show) {
            darkToothSwooping = 0;
        } else if (darkToothSwooping == 1) {
            portCarDtPlayerSwoopStep(h, head);
        } else if (darkToothSwooping == 2) {
            DT_SWOOP_TIMER++;
            if (portCarDtPlayerPlace(h, head, 0) < 2 || DT_SWOOP_TIMER >= 100)
                darkToothSwooping = 0;
        } else {
            darkToothSwooping = 0;
            portCarDtPlayerPlace(h, head, 1);
        }
    }
    portCarSpecialRestore(t);
}

/* weaponFireSpecial, case 0xF0 of a human (inside his selection): the boss's swoop at the nearest car (human or AI,
 * not his own) that is alive and drawn, within 4000 world units of the truck (the boss's range and distance).
 * Returns 0 (the caller refunds the special) when no car is in range, the head is swooping or he has no head. */
int PORT_TEXT portCarDtPlayerSwoop(void *human)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)human;
    struct PortDtTarget c;
    int t, i, n, d, best, best_d;

    if (h->type != 0xF0)
        return 0;
    t = portCarSpecialSelect(portCarKey(h->slot, h->type));
    best = -1;
    if (darkToothBossObject != 0 && darkToothSwooping != 1 && !h->state.dead) {
        best_d = DT_RANGE;
        n = humanCarCount();
        for (i = 0; i < n; i++) {
            if (i == h->player || !portDtTarget(i, &c) || !portDtTargetLive(&c))
                continue;
            d = DT_DIST(h->motion.pos, c.pos);
            if (d < best_d) {
                best_d = d;
                best = i;
            }
        }
        n = aiCarCount();
        for (i = 0; i < n; i++) {
            if (!portDtTarget(0x100 + i, &c) || !portDtTargetLive(&c))
                continue;
            d = DT_DIST(h->motion.pos, c.pos);
            if (d < best_d) {
                best_d = d;
                best = 0x100 + i;
            }
        }
        if (best >= 0) {
            portDtTarget(best, &c);
            DT_TARGET = best;
            darkToothSwoopTarget = c.pos;
            DT_SWOOP_TIMER = 0;
            darkToothSwooping = 1;
#ifdef TM2_PORT_CAR_SOUND
            portCarSoundPlay(66, 0, 0, 99, h->slot);    /* request 33-3: his own swoop sound */
#else
            soundPlay(66, 0, 0, 99);                    /* the boss's swoop sound */
#endif
        }
    }
    portCarSpecialRestore(t);
    return best >= 0;
}

/* weaponTick, for a human record of type 0xF0 (before the gun code, which then sees no request): the machine-gun
 * button fires the boss's missile (aiDarkToothBossUpdate's front launch: from (+-120, 272, 104) in the truck's
 * frame, sides alternating, pitch 170, his heading, the target ahead, weaponFireDarkToothMissile with 5 damage)
 * every portDtPlayer.fire_rate updates (0: 10) while held; not while frozen or dead. The cooldown counts every
 * update. weaponFireDarkToothMissile draws rand() as the boss's do (its turn rate). */
void PORT_TEXT portCarDtPlayerGuns(void *human)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)human;
    struct TM2CarWeapons *w = &h->weapons;
    PortDtVec offset, out, launch;
    int shot, fire;

    fire = h->input.fire_guns;
    h->input.fire_guns = 0;
    if (w->gun_cooldown > 0)
        w->gun_cooldown--;
    if (!fire || w->gun_cooldown > 0 || h->state.dead || h->state.frozen)
        return;
    w->gun_cooldown = portDtPlayer.fire_rate > 0 ? portDtPlayer.fire_rate : TM2_DT_PLAYER_FIRE_RATE;
    w->gun_side ^= 1;
    offset.vx = w->gun_side ? -120 : 120;
    offset.vy = 272;
    offset.vz = 104;
    mathMulTransVec(&h->motion.rot_m, &offset, &out);
    out.vx += h->motion.pos[0];
    out.vy += h->motion.pos[1];
    out.vz += h->motion.pos[2];
    weaponFindTargetAhead(h->player, &shot, 1);
    launch.vx = 170;
    launch.vy = 0;
    launch.vz = h->motion.rot[2] & 4095;
    weaponFireDarkToothMissile(humanCarObject(h->player)->id, shot, (int)&launch, (int)&out, 5);
}

/* weaponProjectileUpdate's missile wobble: is `owner` (a type-61 missile's owner id) a human Dark Tooth's car object?
 * His missiles wobble as the boss's on every arena. */
int PORT_TEXT portCarDtPlayerShooter(int owner)
{
    struct TM2Object *o;
    int i, n = humanCarCount();

    for (i = 0; i < n; i++) {
        if (humanCarGet(i)->type != 0xF0)
            continue;
        o = humanCarObject(i);
        if (o != 0 && o->id == owner)
            return 1;
    }
    return 0;
}

#endif /* TM2_PORT_DT_PLAYER */
#ifdef TM2_PORT_SPECIAL_AS
/* ---- any car fires another car's special (PC port request 35, docs/GUN_GAME_SPECIALS.md "As built") ----
 * portCarSpecialAs[slot] (0 / -1 = own) is the type whose special the car in `slot` fires. weaponFireSpecial switches
 * on it (portCarSpecialSelectFire); the AI's and the crush's type tests read it (portCarSpecialType); weaponBlockInit
 * keeps its ammo (portCarSpecialAsBlockInit). A borrowed Shadow strike or Axel shockwave runs on the slot's own
 * as-entry, selected like 33-2a's duplicates (selection value TM2_PORT_SPECIAL_KEYS + 1 + slot), with its own
 * strike / ring object; weaponShadowStrikeUpdate steps every borrowed strike after the duplicates'
 * (portCarSpecialAsNext, in slot order). Nothing here runs rand(). With every slot -1 and portCarSpecialLoad 0 no
 * as-entry is selected, no object is made and no group is added: retail's code path. */
extern void *csAlloc(void);
extern void csListAdd(void *object);
extern int csPoolCount;
extern int weaponSpiralMissileModel, weaponBoomerangModel, weaponGrimmSpecialModel, weaponGhostMissileModel;
#define AS_CAP TM2_CAR_SLOT_CAP
#define AS_WORD(o, off) (*(int *)((unsigned char *)(o) + (off)))
#define AS_HALF(o, off) (*(short *)((unsigned char *)(o) + (off)))
#define AS_STRIKE_OBJECT(s) (((struct TM2Object **)PSA.strike[s].block)[24])   /* block +0x60 */
#define AS_PARKED_HEIGHT (-1000000) /* SURVIVAL.md 3.1: a parked car's height (portCarParkSlot's) */

/* The type whose special the car in `slot` fires: its as-type, else `type` (its own). */
int PORT_TEXT portCarSpecialType(int slot, int type)
{
    if ((unsigned int)slot < AS_CAP && portCarSpecialAs[slot] > 0)
        return portCarSpecialAs[slot];
    return type;
}

/* Select car slot `slot`'s as-entry of `type`'s part (Shadow's strike, Axel's shockwave): the retail globals hold that
 * car's borrowed state until portCarSpecialRestore(token). 0 (nothing selected) for a type without a single state. */
int PORT_TEXT portCarSpecialSelectAs(int slot, int type)
{
    int part, prev;

    part = portCarSpecialPart(type);
    if ((part != PART_STRIKE && part != PART_AXEL) || (unsigned int)slot >= AS_CAP)
        return 0;
    prev = PS.sel[part];
    portCarSpecialSwitch(part, TM2_PORT_SPECIAL_KEYS + 1 + slot);
    return (part + 1) << 8 | prev;
}

/* weaponFireSpecial: the firer's selection. A borrowing car: *vehicle becomes the borrowed type and its as-entry is
 * selected; else its own key's state (33-2a), as before. */
int PORT_TEXT portCarSpecialSelectFire(int slot, int *vehicle)
{
    int as = portCarSpecialType(slot, 0);

    if (as > 0) {
        *vehicle = as;
        return portCarSpecialSelectAs(slot, as);
    }
    return portCarSpecialSelect(portCarKey(slot, *vehicle));
}

/* The per-frame loop (weaponShadowStrikeUpdate): from slot *slot on, the next slot that borrows `type`, its as-entry
 * selected. Returns the token, 0 when there is none. */
int PORT_TEXT portCarSpecialAsNext(int type, int *slot)
{
    int s, t;

    for (s = *slot; s < AS_CAP; s++)
        if (portCarSpecialAs[s] == type && (t = portCarSpecialSelectAs(s, type)) != 0) {
            *slot = s + 1;
            return t;
        }
    *slot = s;
    return 0;
}

/* weaponFireSpecial 0xB0 (before the shockwave step): a car borrowing Axel's special shows its own ring object at its
 * position and heading (Axel's own ring is a node of Axel's body; the borrowed one stays where it was fired). */
void PORT_TEXT portCarSpecialAsRing(int slot, void *firer)
{
    struct TM2Object *ring, *o = (struct TM2Object *)firer;
    int i;

    if ((unsigned int)slot >= AS_CAP || portCarSpecialAs[slot] != 0xB0 || o == 0)
        return;
    ring = (struct TM2Object *)PSA.axel[slot].ring;
    if (ring == 0)
        return;
    for (i = 0; i < 3; i++) {
        ring->pos[i] = o->pos[i];
        ring->rot[i] = o->rot[i];
    }
    ring->visible = 1;
}

/* End car slot `slot`'s as-entry: a borrowed strike in flight ends as strike_update.c ends it, without the blast,
 * taking back the special its fire refunded; the ring is hidden. portCarSpecialAsSet (a level change) and, with
 * TM2_PORT_CAR_SWAP, portCarResetSpecials (swap, park, drop-in remove) call it. */
void PORT_TEXT portCarSpecialAsEnd(int slot)
{
    struct TM2CarWeapons *w;
    int t;

    if ((unsigned int)slot >= AS_CAP)
        return;
    t = portCarSpecialSelectAs(slot, 0xD0);
    if (weaponStrikeState.active != 0) {
        weaponStrikeState.active = 0;
        weaponStrikeState.timer = 0;
        objRegistrySetStep(0x915, 0, 0);
        if (weaponStrikeState.object != 0)
            weaponStrikeState.object->visible = 0;
        w = weaponStrikeState.weapon;
        if (w != 0 && w->ammo[0] > 0)
            w->ammo[0]--;
    }
    portCarSpecialRestore(t);
    if (PSA.axel[slot].ring != 0)
        ((struct TM2Object *)PSA.axel[slot].ring)->visible = 0;
}

/* The car's own special states that a level change leaves behind (the car's key selected): its strike (ammo taken
 * back), its tornado (the car shown again), its leap (turbo flame off), its shockwave (step 0), its Mr. Slam swing
 * (a held car let go as carDestroyed does it). Flames, missiles and arcs in the air end by themselves. */
static void PORT_TEXT portCarSpecialAsEndOwn(int slot, struct TM2CarWeapons *w, struct TM2CarStatsBlock *stats,
                                             int heading)
{
    struct TM2Object *obj;
    struct TM2CarSlam *s;
    int type, key, t;

    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    key = portCarKey(slot, type);
    t = portCarSpecialSelect(key);
    if (type == 0xD0 && weaponStrikeState.active != 0 && weaponStrikeState.weapon == w) {
        weaponStrikeState.active = 0;
        weaponStrikeState.timer = 0;
        objRegistrySetStep(0x915, 0, 0);
        if (weaponStrikeState.object != 0)
            weaponStrikeState.object->visible = 0;
        if (w->ammo[0] > 0)
            w->ammo[0]--;
    }
    if (type == 0xA0 && weaponTornadoState.timer > 0 && weaponTornadoState.object == obj) {
        weaponTornadoState.timer = 0;
        if (weaponTornadoObject != 0)
            weaponTornadoObject->visible = 0;
        if (weaponTornadoFunnelObject != 0)
            weaponTornadoFunnelObject->visible = 0;
        if (stats->hp > 0) {
            obj->visible = 1;
            carBodyObjectShow(key);
        }
#ifdef TM2_PORT_CAR_SOUND
        portCarTornadoSoundStop(slot);  /* request 33-4: this car's tornado sound only (bit 4) */
#else
        soundStop(0x35);
#endif
    }
    if (type == 0x60 && homingLeapState.active != 0 && homingLeapState.owner_id == slot) {
        homingLeapState.active = 0;
        portCarTurboSetStep(key, 0);
    }
    if (type == 0xB0)
        objRegistrySetStep(0x914, 0, 0);
    s = w->slam;
    if (type == 0xC0 && s != 0 && s->active != 0) {
        if (s->spin_target > 0) {
            carSpinOutById(s->spin_target, (short)heading, 0);
            s->spin_target = -1;
        }
        s->active = 0;
        s->spin_pending = 0;
    }
    portCarSpecialRestore(t);
}

/* weaponBlockInit's max_specials (and recharge_period, where it sets one) of a car type (ai_route_setup.c's switch). */
static int PORT_TEXT portCarSpecialAsAmmo(int type, int *period)
{
    switch (type) {
    case 0x10: *period = 360;  return 5;
    case 0x20: *period = 1200; return 5;
    case 0x30: *period = 1100; return 3;
    case 0x40: *period = 700;  return 5;
    case 0x50: *period = 1400; return 3;
    case 0x60: *period = 700;  return 5;
    case 0x70: *period = 1600; return 5;
    case 0x80: *period = 700;  return 4;
    case 0x90: *period = 700;  return 5;
    case 0xA0: *period = 360;  return 3;
    case 0xB0: *period = 700;  return 5;
    case 0xC0: *period = 1000; return 3;
    case 0xD0: *period = 400;  return 5;
    case 0xE0: *period = 360;  return 5;
    case 0xF0: *period = 700;  return 5;
    }
    *period = -1;
    return 5;
}

/* weaponBlockInit, after its switch (every start, restart and respawn): a borrowing car keeps the borrowed special's
 * max_specials and recharge_period (its Mr. Slam claw record and capacity stay its own type's). */
#ifdef TM2_PORT_SPECIAL_PROFILE
static void PORT_TEXT portSpecialApply(int slot);
static void PORT_TEXT portSpecialAmmo(int slot, struct TM2CarWeapons *w);
#endif

void PORT_TEXT portCarSpecialAsBlockInit(void *car, void *weapons, int human)
{
    struct TM2CarWeapons *w = (struct TM2CarWeapons *)weapons;
    int slot, as, period;

    slot = human ? ((struct TM2HumanRecord *)car)->slot : ((struct TM2AIRecord *)car)->slot;
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialApply(slot);     /* request 73: a mod car's template, the first start of the load (or after a reset) */
#endif
    as = portCarSpecialType(slot, 0);
    if (as > 0 && as <= 0xF0) {
        w->max_specials = portCarSpecialAsAmmo(as, &period);
        if (period >= 0)
            w->recharge_period = period;
    }
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialAmmo(slot, w);   /* request 70's recharge / max over the template's */
#endif
}

/* The record driving car slot `slot` (carSlotKinds 0 / 1): its weapons block, stats and heading; 0 if none. */
static struct TM2CarWeapons *PORT_TEXT portCarSpecialAsDriver(int slot, struct TM2CarStatsBlock **stats, int *heading)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int i;

    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || slot >= carWorldObjectCount || slot >= AS_CAP)
        return 0;
    i = carSlotIndices[slot];
    if (carSlotKinds[slot] == 0 && (unsigned int)i < TM2_MAX_HUMANS && i < humanCarTotal) {
        h = (struct TM2HumanRecord *)humanCars[i];
        if (h->slot == slot) {
            *stats = &h->stats;
            *heading = h->motion.rot[2];
            return &h->weapons;
        }
    } else if (carSlotKinds[slot] == 1 && (unsigned int)i < TM2_MAX_AI_CARS && i < aiCarTotal) {
        a = (struct TM2AIRecord *)aiCars[i];
        if (a->slot == slot) {
            *stats = &a->stats;
            *heading = a->motion.rot[2];
            return &a->weapons;
        }
    }
    return 0;
}

/* Is `type`'s special usable in this load? The four car projectile models (cleared at each load by
 * portCarSpecialAsReset, set by the car file's install), Shadow's strike object and node, Axel's shockwave node (the
 * first car of the type's: retail's globals). The other borrowable types use the level's assets. */
static int PORT_TEXT portCarSpecialAsLoaded(int type)
{
    int t, ok;

    switch (type) {
    case 0x10: return weaponSpiralMissileModel != 0;
    case 0x50: return weaponGrimmSpecialModel != 0;
    case 0x80: return weaponGhostMissileModel != 0;
    case 0x90: return weaponBoomerangModel != 0;
    case 0xB0:
        t = portCarSpecialSelect(0xB0);
        ok = objRegistryId2324 != 0;
        portCarSpecialRestore(t);
        return ok;
    case 0xD0:
        t = portCarSpecialSelect(0xD0);
        ok = weaponStrikeState.object != 0 && objRegistryId2325 != 0;
        portCarSpecialRestore(t);
        return ok;
    }
    return 1;
}

/* A new scene object drawing `model` (weaponAssetBind's way: csAlloc, csListAdd, hidden), 0 when the pool is full. */
static void *PORT_TEXT portCarSpecialAsObject(void *model)
{
    void *o;

#ifdef TM2_PORT_CAR_DROPIN
    /* request 33 unloading: csAlloc also hands out the objects unloads gave back */
    if ((csPoolCount >= TM2_MAX_COORD_SYSTEMS && portCarUnload.cs_count == 0) || (o = csAlloc()) == 0)
#else
    if (csPoolCount >= TM2_MAX_COORD_SYSTEMS || (o = csAlloc()) == 0)
#endif
        return 0;
    csListAdd(o);
    AS_WORD(o, 0x11C) = (int)model;     /* model */
    AS_WORD(o, 0x08) = 0;               /* hidden */
    return o;
}

/* The port's API (Gun Game's level change), between frames, never inside a selection: car slot `slot` fires `type`'s
 * special from now on (-1, 0 or its own type: its own special), with `ammo` specials (-1: keep the count; capped at
 * the type's max_specials). Refused: a type outside TM2_PORT_SPECIAL_AS_MASK (Hammerhead, Grasshopper, Twister,
 * Mr. Slam, Dark Tooth), a type whose car file is not loaded in this match (portCarSpecialLoad, or a car of the type),
 * a slot no record drives, no coordinate system left for the object. Steps: the objects of the as-entry made the
 * first time (Shadow: a strike object with weaponAssetBind's 0x915 defaults drawing the loaded Shadow's node; Axel: a
 * ring object drawing the loaded Axel's shockwave node), the old special ended (the as-entry's and the car's own
 * states), the type set, max_specials / recharge_period from weaponBlockInit's table (the own type's when going
 * back), the recharge counter restarted, the ammo set. Returns TM2_SPECIAL_AS_OK or TM2_SPECIAL_AS_ERR_*. */
/* The objects of slot `slot`'s as-entry of `type`, made the first time (Shadow: a strike object with weaponAssetBind's
 * 0x915 defaults drawing the loaded Shadow's node; Axel: a ring object drawing the loaded Axel's shockwave node).
 * TM2_SPECIAL_AS_OK, or TM2_SPECIAL_AS_ERR_ROOM. */
static int PORT_TEXT portCarSpecialAsMake(int slot, int type)
{
    void *o, *model, *anim;
    int t, i;

    if (type == 0xD0) {
        t = portCarSpecialSelect(0xD0);         /* the first Shadow's: its strike object's model and its node */
        model = (void *)AS_WORD(weaponStrikeState.object, 0x11C);
        anim = objRegistryId2325;
        portCarSpecialRestore(t);
        if (AS_STRIKE_OBJECT(slot) == 0) {
            o = portCarSpecialAsObject(model);
            if (o == 0)
                return TM2_SPECIAL_AS_ERR_ROOM;
            AS_WORD(o, 0x118) = 0x8a2;          /* weaponAssetBind's PROJECTILE(.., 80) */
            AS_WORD(o, 0x0C) = 1;
            AS_WORD(o, 0xB4) = 1;
            AS_HALF(o, 0xB8) = 0;
            AS_HALF(o, 0xBA) = 0;
            AS_HALF(o, 0xBC) = 80;
            AS_STRIKE_OBJECT(slot) = (struct TM2Object *)o;
            for (i = 0; i < 12; i++)            /* the four corner offsets (block +0x2C): weaponAssetBind's 0x915 */
                PSA.strike[slot].block[11 + i] = 0;
            PSA.strike[slot].block[11 + 1] = 40;
            PSA.strike[slot].block[11 + 4] = -40;
            PSA.strike[slot].block[11 + 6] = -40;
            PSA.strike[slot].block[11 + 9] = 40;
        }
        PSA.strike[slot].anim = anim;
    }
    if (type == 0xB0) {
        t = portCarSpecialSelect(0xB0);         /* the first Axel's shockwave node */
        anim = objRegistryId2324;
        portCarSpecialRestore(t);
        if (PSA.axel[slot].ring == 0) {
            o = portCarSpecialAsObject(anim);
            if (o == 0)
                return TM2_SPECIAL_AS_ERR_ROOM;
            AS_WORD(o, 0x0C) = 0;               /* no collision, as Twister's funnel */
            PSA.axel[slot].ring = o;
        }
        PSA.axel[slot].anim = anim;
    }
    return TM2_SPECIAL_AS_OK;
}

#ifdef TM2_PORT_SPECIAL_PROFILE
static int PORT_TEXT portSpecialIsPickup(int type);
/* request 73: "its own" is a mod car's template (portSpecialProfile); a pickup weapon needs nothing loaded */
#define AS_TEMPLATE(slot) portSpecialTemplate(slot)
#define AS_PICKUP(type) portSpecialIsPickup(type)
#else
#define AS_TEMPLATE(slot) (-1)
#define AS_PICKUP(type) 0
#endif

int PORT_TEXT portCarSpecialAsSet(int slot, int type, int ammo)
{
    struct TM2CarWeapons *w;
    struct TM2CarStatsBlock *stats;
    int own, heading, max, period, r;

    w = portCarSpecialAsDriver(slot, &stats, &heading);
    if (w == 0)
        return TM2_SPECIAL_AS_ERR_SLOT;
    own = carSlotOwners[slot];
    if (type <= 0 || type == own)
        type = AS_TEMPLATE(slot);
    if (type <= 0 || type == own) {
        type = -1;
    } else if (!AS_PICKUP(type)) {
        if ((type & 15) != 0 || type > 0xF0 || ((TM2_PORT_SPECIAL_AS_MASK >> (type >> 4)) & 1) == 0)
            return TM2_SPECIAL_AS_ERR_TYPE;
        if (!portCarSpecialAsLoaded(type))
            return TM2_SPECIAL_AS_ERR_NOT_LOADED;
    }
    r = portCarSpecialAsMake(slot, type);
    if (r != TM2_SPECIAL_AS_OK)
        return r;
    portCarSpecialAsEndOwn(slot, w, stats, heading);
    portCarSpecialAsEnd(slot);
    portCarSpecialAs[slot] = type;
    max = portCarSpecialAsAmmo(type > 0 && type <= 0xF0 ? type : own, &period);
    w->max_specials = max;
    if (period >= 0)
        w->recharge_period = period;
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialAmmo(slot, w);   /* request 73: a mod car's own recharge / max (request 70) over the table's */
    max = w->max_specials;
#endif
    w->f2E = 0;
    if (ammo >= 0)
        w->ammo[0] = ammo;
    if (w->ammo[0] > max)
        w->ammo[0] = max;
    return TM2_SPECIAL_AS_OK;
}

/* Each load (portCarSpecialsReset, from carTablesClear, before the level and car DMDs install): every slot fires its
 * own special, the as-entries and their objects are forgotten (the coordinate systems are made again by the load),
 * and the words that keep a car file's special across loads are cleared so that a type is "loaded" only when this
 * load installs its file: the four car projectile models (weaponStoreProjectileModel, never cleared by retail) and
 * the 2324 / 2325 nodes (objRegistryAdd; objRegistriesReset does not clear them). Retail's own use of each word
 * comes after the file that sets it.
 * Only the as-entries (+0x0000..+0x1BFF) are cleared: the load plan (load_n, load_first, load_car from +0x1C00) is
 * made by portCarSpecialLoadPlan at the start of each load (sessionLoadArenaAndCars, before the TMS pass) and read
 * again by its DMD pass, and this reset runs between the two (the level's reset): clearing it here lost the spares'
 * DMDs (fix of 2026-10-14, GUN_GAME_SPECIALS.md 4). */
void PORT_TEXT portCarSpecialAsReset(void)
{
    int *p = (int *)&PSA;
    int i;

    for (i = 0; i < AS_CAP; i++)
        portCarSpecialAs[i] = -1;
    for (i = 0; i < (int)(((char *)&PSA.load_n - (char *)&PSA) / 4); i++)
        p[i] = 0;
    weaponSpiralMissileModel = 0;
    weaponBoomerangModel = 0;
    weaponGrimmSpecialModel = 0;
    weaponGhostMissileModel = 0;
    objRegistryId2324 = 0;
    objRegistryId2325 = 0;
}

#ifdef TM2_PORT_SPECIAL_PROFILE
/* ---- a mod car's special: its template and its numbers (PC port request 73, docs/SPECIAL_TEMPLATES.md 6) ----
 * portSpecialProfile[m] (the port's, by mod) names the special a mod car fires and its numbers. The template is
 * applied as request 35's borrowed special (portCarSpecialAs[slot]) at the car's first start of the load
 * (weaponBlockInit), so the fire switch, the AI's and the crush's type tests and the ammo follow it; a pickup weapon
 * (TM2_SPECIAL_PICKUP_*, never a car type) fires through the level's launcher. The numbers go to whatever special the
 * car fires: the projectile records a fire makes (portSpecialFireBegin / End around weaponFireSpecial's switch), and
 * the hooks of Axel's shockwave, the taser, Thumper's flame and Shadow's strike (TM2_SPECIAL_NUM). Nothing here runs
 * rand(); without a valid profile every hook returns retail's value. */
extern void weaponLaunchProjectile(void *car, int kind, unsigned int human);
extern void *fxRecordGet(unsigned int index);
/* A projectile record's head (weapons.h struct TM2Projectile: fxRecords, 980 bytes each). */
struct PortSpecialProj {
    unsigned char pad_00[2];
    short type;                 /* +0x02 */
    short life;                 /* +0x04 <= 0: free */
    short speed;                /* +0x06 */
    short turn_rate;            /* +0x08 */
    short damage;               /* +0x0A */
};

/* The special profile of the mod car in `slot` (its portCarProfile must be valid), 0 = none. */
static struct TM2PortSpecialProfile *PORT_TEXT portSpecialOf(int slot)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(slot);

    return p != 0 ? &portSpecialProfile[p - portCarProfile] : 0;
}

/* Is `type` a pickup weapon template? */
static int PORT_TEXT portSpecialIsPickup(int type)
{
    return type == TM2_SPECIAL_PICKUP_FIRE || type == TM2_SPECIAL_PICKUP_HOMING || type == TM2_SPECIAL_PICKUP_POWER
        || type == TM2_SPECIAL_PICKUP_NAPALM || type == TM2_SPECIAL_PICKUP_MEGA
        || type == TM2_SPECIAL_PICKUP_FREEZE || type == TM2_SPECIAL_PICKUP_BOMB
        || type == TM2_SPECIAL_PICKUP_RICOCHET;
}

/* The template of the car in `slot`: a pickup weapon, or a borrowable car type (TM2_PORT_SPECIAL_AS_MASK); 0 for
 * none, for its own type, and for a refused one (it keeps its base's special). */
int PORT_TEXT portSpecialTemplate(int slot)
{
    struct TM2PortSpecialProfile *sp = portSpecialOf(slot);
    int t;

    if (sp == 0 || (t = sp->special) <= 0)
        return 0;
    if (portSpecialIsPickup(t))
        return t;
    if ((t & 15) != 0 || t > 0xF0 || ((TM2_PORT_SPECIAL_AS_MASK >> (t >> 4)) & 1) == 0 || t == carSlotOwners[slot])
        return 0;
    return t;
}

/* weaponBlockInit (portCarSpecialAsBlockInit): the car in `slot` fires its template from now on, unless the port
 * set a borrowed special (Gun Game) or it already does; refused while the template's car file is not loaded or no
 * coordinate system is left for its object (the base's special stays). */
static void PORT_TEXT portSpecialApply(int slot)
{
    int t = portSpecialTemplate(slot);

    if (t <= 0 || (unsigned int)slot >= AS_CAP || portCarSpecialAs[slot] > 0)
        return;
    if (!portSpecialIsPickup(t) && !portCarSpecialAsLoaded(t))
        return;
    if (portCarSpecialAsMake(slot, t) != TM2_SPECIAL_AS_OK)
        return;
    portCarSpecialAs[slot] = t;
}

/* Request 70's recharge / max specials of the car in `slot` over a template's (weaponBlockInit's order: the
 * profile's numbers win). */
static void PORT_TEXT portSpecialAmmo(int slot, struct TM2CarWeapons *w)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(slot);

    if (p == 0 || !(p->flags & TM2_PORT_PROFILE_SPECIAL))
        return;
    if (p->recharge > 0)
        w->recharge_period = p->recharge;
    if (p->max_specials > 0)
        w->max_specials = p->max_specials;
}

/* The car in `slot`'s number `field` (TM2_SPN_*), clamped to the field's limit (SPECIAL_TEMPLATES.md 1-2), or
 * `retail` when its profile does not set it. */
int PORT_TEXT portSpecialNum(int slot, int field, int retail)
{
    struct TM2PortSpecialProfile *sp = portSpecialOf(slot);
    int i, n, v, lo, hi;

    if (sp == 0 || field <= 0 || field > TM2_SPN_LAST)
        return retail;
    n = sp->count < TM2_PORT_SPECIAL_NUMS ? sp->count : TM2_PORT_SPECIAL_NUMS;
    for (i = 0; i < n; i++)
        if (sp->num[i].field == field)
            break;
    if (i == n)
        return retail;
    v = sp->num[i].value;
    lo = 1;
    hi = 4000;
    if (field == TM2_SPN_DAMAGE || field == TM2_SPN_DAMAGE2) {
        lo = 0;
        hi = 8191;
    } else if (field == TM2_SPN_LIFE) {
        hi = 8191;
    } else if (field == TM2_SPN_SPEED) {
        hi = 150;
    } else if (field == TM2_SPN_TURN) {
        hi = 4095;
    } else if (field == TM2_SPN_DURATION) {
        lo = 40;
        hi = 400;
    } else if (field == TM2_SPN_FREEZE) {
        hi = 6000;
    } else if (field == TM2_SPN_BURN) {
        hi = 2000;
    } else if (field == TM2_SPN_BLAST) {
        lo = 0;
        hi = 8191;
    } else if (field == TM2_SPN_BLAST_RADIUS) {
        hi = 400;
    }
    return v < lo ? lo : v > hi ? hi : v;
}

/* Stage 2: the car slot of the car whose object id is `id` (a projectile's, an emitter's or a strike's owner: a
 * human's id, an AI's index + 50), -1 when no car has it. */
static int PORT_TEXT portSpecialSlotOfId(int id)
{
    unsigned char human;
    int index;

    if (!(carFindById(id, &human, &index) & 255))
        return -1;
    if (human)
        return (unsigned int)index < TM2_MAX_HUMANS && index < humanCarTotal
            ? ((struct TM2HumanRecord *)humanCars[index])->slot : -1;
    return (unsigned int)index < TM2_MAX_AI_CARS && index < aiCarTotal
        ? ((struct TM2AIRecord *)aiCars[index])->slot : -1;
}

/* The per-frame hooks (TM2_SPECIAL_NUM_ID): the number of the car whose object id is `id`, else `retail`. */
int PORT_TEXT portSpecialNumById(int id, int field, int retail)
{
    return portSpecialNum(portSpecialSlotOfId(id), field, retail);
}

/* areaDamage: the blast's base damage `retail` (25, or 50 on a surface polygon) for Shadow's detonation (big 0) by
 * the car in `slot`: its TM2_SPN_BLAST, the surface's 25 on top; a remote bomb's (big 1) stays retail's. */
int PORT_TEXT portSpecialBlast(int slot, int big, int retail)
{
    if (big)
        return retail;
    return portSpecialNum(slot, TM2_SPN_BLAST, 25) + (retail - 25);
}

/* weaponProjectileUpdate: a boomerang caught by its thrower gives the charge back to the thrower (retail gives it to
 * the first human Roadkill, whoever threw it: SPECIAL_TEMPLATES.md 2). */
void PORT_TEXT portSpecialBoomerangCaught(int id)
{
    unsigned char human;
    int index;
    struct TM2CarWeapons *w;

    if (!(carFindById(id, &human, &index) & 255))
        return;
    if (human) {
        if ((unsigned int)index >= TM2_MAX_HUMANS || index >= humanCarTotal)
            return;
        w = &((struct TM2HumanRecord *)humanCars[index])->weapons;
    } else {
        if ((unsigned int)index >= TM2_MAX_AI_CARS || index >= aiCarTotal)
            return;
        w = &((struct TM2AIRecord *)aiCars[index])->weapons;
    }
    w->ammo[0]++;
}

/* The ricochet pool (weapons.h struct TM2RicochetTable) and the sound calls of weaponFireSelected's case 6. */
struct PortRicochetShot {
    signed char power;
    unsigned char left_owner;
    short owner;                /* +0x02 */
    short life;                 /* +0x04 */
    short bounces;              /* +0x06 */
    int offset[3];              /* +0x08 */
    int vel[3];
    int prev[3];
    unsigned char *obj;         /* +0x2C */
};
struct PortRicochetTable {
    unsigned char *slots[TM2_MAX_RICOCHETS];
    struct PortRicochetShot shots[TM2_MAX_RICOCHETS];
};
extern int weaponRicochetPool[];             /* struct PortRicochetTable (an int array where this file raises it) */
#define RICO ((struct PortRicochetTable *)weaponRicochetPool)
extern int weaponRicochetModel, weaponRicochetModelAlt;
extern int weaponRicochetSlotFind(void);
extern int weaponRemoteBombFind(int id);
extern void weaponDropItem(void *car, int kind, unsigned int human);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *pos);
extern void soundLocateForView(void *pos, int view);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlay(int id, int a, int b, int c);
extern int soundPlayVariantEx(int id, int a, int b, int c, int d, int e);

/* The positional sound at `pos` as weaponFireSelected plays it. */
static void PORT_TEXT portSpecialSound(int *pos, int id, int variant, int slot)
{
    int mtap, pan;

    if (viewGetSplitMode() == 0)
        soundLocateForView0(pos);
    else
        soundLocateForView(pos, -1);
    mtap = PadChkMtap();
    pan = soundGetListenerPan();
    if (variant)
        soundPlayVariantEx(id, mtap, pan, 99, slot, 1);
    else
        soundPlay(id, mtap, pan, 99);
}

/* A ricochet bomb as weaponFireSelected's case 6 fires it (its pool, Dark Tooth's look by the car's own type, 40
 * above the car, 100 a frame forward, life 400; request 41 bit 18's left_owner cleared), with the car's TM2_SPN_LIFE
 * and TM2_SPN_SPEED. */
static void PORT_TEXT portSpecialRicochet(void *car, int human, int slot)
{
    struct TM2CarMotion *motion;
    struct TM2CarWeapons *w;
    unsigned char *o;
    int type, i, n;
    short vec[4];

    if (human) {
        motion = &((struct TM2HumanRecord *)car)->motion;
        w = &((struct TM2HumanRecord *)car)->weapons;
        type = ((struct TM2HumanRecord *)car)->type;
        o = (unsigned char *)humanCarObject(((struct TM2HumanRecord *)car)->player);
    } else {
        motion = &((struct TM2AIRecord *)car)->motion;
        w = &((struct TM2AIRecord *)car)->weapons;
        type = ((struct TM2AIRecord *)car)->type;
        o = (unsigned char *)aiCarObject(((struct TM2AIRecord *)car)->index);
    }
    n = weaponRicochetSlotFind();
    if (n < 0)
        return;
    RICO->shots[n].owner = *(int *)(o + 0x118);
    *(int *)(RICO->slots[n] + 0x11C) = type == 0xF0 ? weaponRicochetModelAlt : weaponRicochetModel;
    *(int *)(RICO->slots[n] + 0x08) = 1;
    *(int *)(RICO->slots[n] + 0x0C) = 0;
    vec[0] = motion->rot[0];
    vec[1] = 0;
    vec[2] = motion->rot[2];
    vec[3] = 0;
    if (w->rear_fire != 0) {
        w->rear_fire = 0;
        vec[2] = (motion->rot[2] + 0x800) & 0xfff;
    }
    for (i = 0; i < 4; i++)
        ((short *)(RICO->slots[n] + 0xA0))[i] = vec[i];
    for (i = 0; i < 3; i++)
        ((int *)(RICO->slots[n] + 0xA8))[i] = motion->pos[i];
    ((int *)(RICO->slots[n] + 0xA8))[2] += 40;
    RICO->shots[n].obj = RICO->slots[n];
    RICO->shots[n].life = portSpecialNum(slot, TM2_SPN_LIFE, 400);
    RICO->shots[n].bounces = 0;
    RICO->shots[n].left_owner = 0;
    RICO->shots[n].offset[1] = portSpecialNum(slot, TM2_SPN_SPEED, 100);
    portSpecialSound(motion->pos, 0x12, 1, n);
}

/* A remote bomb: the car's bomb out (weaponRemoteBombFind by its id) is detonated, else one is placed and the
 * charge given back (as the pickup's placement; the detonation spends it). The weapons block's drop_armed follows,
 * so the pickup's own button sees the same bomb. */
static void PORT_TEXT portSpecialBomb(void *car, unsigned int human)
{
    struct TM2CarMotion *motion;
    struct TM2CarWeapons *w;
    int id;

    if (human & 255) {
        motion = &((struct TM2HumanRecord *)car)->motion;
        w = &((struct TM2HumanRecord *)car)->weapons;
        id = TM2_HUMAN_ID(((struct TM2HumanRecord *)car)->player);
    } else {
        motion = &((struct TM2AIRecord *)car)->motion;
        w = &((struct TM2AIRecord *)car)->weapons;
        id = ((struct TM2AIRecord *)car)->index + 50;
    }
    if (weaponRemoteBombFind(id) < 0) {
#ifdef TM2_PORT_BALANCE
        if (portBalanceBombRefused(car, human & 255)) {     /* request 40 bit 6: refused, no cost */
            w->ammo[0]++;
            return;
        }
#endif
        w->drop_armed = 1;
        w->fire_cooldown = 1;
        w->ammo[0]++;
        portSpecialSound(motion->pos, 0x13, 0, 0);
    } else {
        w->drop_armed = 0;
    }
    weaponDropItem(car, 3, human);
}

/* weaponFireSpecial, before its switch: note the projectile records free now (life <= 0, as the allocator sees
 * them). */
void PORT_TEXT portSpecialFireBegin(void)
{
    struct PortSpecialProj *r;
    int i, n;

    n = TM2_PROJECTILES_LIVE < 64 ? TM2_PROJECTILES_LIVE : 64;
    portSpecialState.free[0] = 0;
    portSpecialState.free[1] = 0;
    for (i = 0; i < n; i++) {
        r = (struct PortSpecialProj *)fxRecordGet(i);
        if (r->life <= 0)
            portSpecialState.free[i >> 5] |= 1u << (i & 31);
    }
}

/* weaponFireSpecial, after its switch: every record the fire made (free before, live now) takes the car's numbers:
 * damage (a Minion's freeze missile: TM2_SPN_DAMAGE2), life, start speed, turn rate (the spiral's never 99: Dark
 * Tooth's wobble). */
void PORT_TEXT portSpecialFireEnd(int slot, int vehicle)
{
    struct TM2PortSpecialProfile *sp = portSpecialOf(slot);
    struct PortSpecialProj *r;
    int i, n, t;

    if (sp == 0 || sp->count == 0)
        return;
    n = TM2_PROJECTILES_LIVE < 64 ? TM2_PROJECTILES_LIVE : 64;
    for (i = 0; i < n; i++) {
        if (!(portSpecialState.free[i >> 5] & (1u << (i & 31))))
            continue;
        r = (struct PortSpecialProj *)fxRecordGet(i);
        if (r->life <= 0)
            continue;
        r->damage = portSpecialNum(slot, vehicle == 0xE0 && r->type == 14 ? TM2_SPN_DAMAGE2 : TM2_SPN_DAMAGE,
                                   r->damage);
        r->life = portSpecialNum(slot, TM2_SPN_LIFE, r->life);
        r->speed = portSpecialNum(slot, TM2_SPN_SPEED, r->speed);
        t = portSpecialNum(slot, TM2_SPN_TURN, r->turn_rate);
        if (r->type == 61 && t == 99)
            t = 98;
        r->turn_rate = t;
    }
}

/* weaponFireSpecial, a pickup template: the level's launcher (weaponLaunchProjectile: its muzzle, aim, sound and
 * retail numbers; it touches no ammo, held or selected), napalm with the weapons block's Mega Napalm flag set for the
 * shot (Mega) or clear (napalm). The charge is weaponFireSelected's, as any special's. */
void PORT_TEXT portSpecialFirePickup(void *car, unsigned int human, int vehicle)
{
    struct TM2CarWeapons *w;
    int kind, keep;

    if (vehicle == TM2_SPECIAL_PICKUP_RICOCHET) {
        portSpecialRicochet(car, human & 255, (human & 255) ? ((struct TM2HumanRecord *)car)->slot
                                                            : ((struct TM2AIRecord *)car)->slot);
        return;
    }
    if (vehicle == TM2_SPECIAL_PICKUP_BOMB) {
        portSpecialBomb(car, human);
        return;
    }
    w = (human & 255) ? &((struct TM2HumanRecord *)car)->weapons : &((struct TM2AIRecord *)car)->weapons;
    kind = vehicle == TM2_SPECIAL_PICKUP_FREEZE ? 100 : vehicle == TM2_SPECIAL_PICKUP_MEGA ? 5 : vehicle & 0xFF;
    keep = w->mega_napalm;
    if (vehicle == TM2_SPECIAL_PICKUP_MEGA)
        w->mega_napalm = 1;
    else if (vehicle == TM2_SPECIAL_PICKUP_NAPALM)
        w->mega_napalm = 0;
    weaponLaunchProjectile(car, kind, human);
    w->mega_napalm = keep;
}

/* aiCarSetupProfile: the type whose weight tables and fire chance a computer car takes: its template's (a car type),
 * else its own. */
int PORT_TEXT portSpecialAiType(struct TM2AIRecord *ai)
{
    int t = portSpecialTemplate(ai->slot);

    return t > 0 && t <= 0xF0 ? t : ai->type;
}

/* aiCarSetupProfile, after the tables: a mod car with ai_ahead / ai_behind (a pickup template: TM2_SPECIAL_PICKUP_AI
 * for an unset one) gets copies of its tables with those slot-0 weights (per mod, in portSpecialState). */
void PORT_TEXT portSpecialAiTables(struct TM2AIRecord *ai)
{
    struct TM2PortSpecialProfile *sp = portSpecialOf(ai->slot);
    unsigned char *a, *b;
    int m, i, wa, wb;

    if (sp == 0)
        return;
    if (sp->fire_chance != 0)   /* request 87: the mod's own, in place of the type's base (the bonuses follow) */
        ai->weapons.fire_chance = sp->fire_chance > 100 ? 100 : sp->fire_chance;
    wa = sp->ai_ahead;
    wb = sp->ai_behind;
    if (portSpecialIsPickup(portSpecialTemplate(ai->slot))) {
        if (wa == 0)
            wa = TM2_SPECIAL_PICKUP_AI;
        if (wb == 0)
            wb = TM2_SPECIAL_PICKUP_AI;
    }
    if ((wa == 0 && wb == 0) || ai->weapons.weights9 == 0 || ai->weapons.weights10 == 0)
        return;
    m = sp - portSpecialProfile;
    a = portSpecialState.w9[m];
    b = portSpecialState.w10[m];
    if (ai->weapons.weights9 != a)
        for (i = 0; i < TM2_SPECIAL_AI_TABLE; i++)
            a[i] = ai->weapons.weights9[i];
    if (ai->weapons.weights10 != b)
        for (i = 0; i < TM2_SPECIAL_AI_TABLE; i++)
            b[i] = ai->weapons.weights10[i];
    if (wa > 0)
        a[0] = wa > 100 ? 100 : wa;
    if (wb > 0)
        b[0] = wb > 100 ? 100 : wb;
    ai->weapons.weights9 = a;
    ai->weapons.weights10 = b;
}
#endif /* TM2_PORT_SPECIAL_PROFILE */

#ifdef TM2_PORT_CARS
/* sessionLoadArenaAndCars's view of the settings record (settings.h struct TM2Settings). */
struct PortSettingsView {
    unsigned int mode;
    unsigned char humans, opponents, f06, f07;
    signed char human_car[2];
    signed char opponent_car[10];
};

/* Car-select index (0..14) to type (carIndexToType), 0 for none. */
static int PORT_TEXT portCarSpecialIndexType(int car)
{
    if (car < 0 || car > 14)
        return 0;
    return car < 12 ? (car + 2) << 4 : car == 12 ? 0x10 : car == 13 ? 0xE0 : 0xF0;
}

/* sessionLoadArenaAndCars, before the cars' textures: the spare groups of this load. With portCarOptions bit 1 (the
 * car DMD area: the low area has no room for more DMDs), one group per bit of portCarSpecialLoad whose type needs
 * its car file (TM2_PORT_SPECIAL_LOAD_FILES) and that no human or opponent has, in car-select order, as long as
 * load groups are left. The groups follow the opponents' (first = humans + opponents + 1). Returns their number. */
int PORT_TEXT portCarSpecialLoadPlan(void *settings)
{
    struct PortSettingsView *s = (struct PortSettingsView *)settings;
    int have, i, t, n, car;

    PSA.load_n = 0;
    PSA.load_first = s->humans + s->opponents + 1;
    if (portCarSpecialLoad == 0 || !(portCarOptions & TM2_PORT_CAR_OPT_HIGH_DMD))
        return 0;
    have = 0;
    for (i = 0; i < s->humans; i++) {
#if TM2_MAX_HUMANS > 2
        car = i < 2 ? s->human_car[i] : sessionPortHumanCars[i - 2];
#else
        car = s->human_car[i];
#endif
        have |= 1 << (portCarSpecialIndexType(car) >> 4);
    }
    for (i = 0; i < s->opponents; i++) {
#if TM2_MAX_AI_CARS > TM2_RETAIL_OPPONENT_SLOTS
        car = i < TM2_RETAIL_OPPONENT_SLOTS ? s->opponent_car[i] : sessionPortOpponentCars[i - TM2_RETAIL_OPPONENT_SLOTS];
#else
        car = s->opponent_car[i];
#endif
        have |= 1 << (portCarSpecialIndexType(car) >> 4);
    }
    n = 0;
    for (car = 0; car < 15; car++) {
        t = portCarSpecialIndexType(car) >> 4;
        if (!(((portCarSpecialLoad & TM2_PORT_SPECIAL_LOAD_FILES & ~have) >> t) & 1))
            continue;
        if (n >= TM2_PORT_SPECIAL_AS_LOADS || PSA.load_first + n > TM2_MAX_LOADED_CARS)
            break;
        PSA.load_car[n++] = car;
    }
    PSA.load_n = n;
    return n;
}

/* Spare i's car-select index (portCarSpecialLoadPlan), -1 after the last. */
int PORT_TEXT portCarSpecialLoadCar(int i)
{
    if (i < 0 || i >= PSA.load_n)
        return -1;
    return PSA.load_car[i];
}

/* After the spares' DMDs are installed: their car slots are parked (not drawn and out of the hit pass, as
 * carRegisterModel leaves them; body hidden, far below the arena; free: kind 2). No record binds to them
 * (sessionBindCars binds the settings' groups); the type's single objects and models are in retail's globals. */
void PORT_TEXT portCarSpecialLoadDone(void)
{
    struct TM2Object *obj;
    int s, g;

    for (s = 0; s < carWorldObjectCount && s < AS_CAP; s++) {
        g = portCarGroups.slot_group[s];
        if (g < PSA.load_first || g >= PSA.load_first + PSA.load_n)
            continue;
        obj = (struct TM2Object *)carSlotObjects[s];
        csSetDrawEnable(obj, 0);
        carBodyObjectHide(portCarKey(s, carSlotOwners[s]));
        obj->pos[2] = AS_PARKED_HEIGHT;
        carSlotKinds[s] = 2;
        carSlotIndices[s] = -1;
    }
}
#endif /* TM2_PORT_CARS */
#undef AS_WORD
#undef AS_HALF
#undef AS_STRIKE_OBJECT
#endif /* TM2_PORT_SPECIAL_AS */
#undef PS
#endif /* TM2_PORT_CAR_SPECIALS */

#ifdef TM2_PORT_CAR_SOUND
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
#if !defined(TM2_PORT_CAR_SWAP) && !defined(TM2_PORT_CAR_SPECIALS)
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#include "semantic/cars.h"
#endif
/* ---- per-car engine sound (PC port request 33 step 3, docs/CAR_GROUPS.md 12, PER_CAR_PLAN.md C.3) ----
 * Retail plays an engine (and Thumper's flame, Dark Tooth's swoops) by sound id with instance 0, one request
 * per id. With portCarOptions bit 4 every car slot s plays them as instance s + 1 of the id. The sound
 * manager's per-id lists (soundManagerObject + id * 20 + 0xCC, sound_manager_play.c) then hold more requests;
 * a play first merges into the request of its own instance, as sndListMergeById does for a full list
 * (engine_sound_lists.c: x, y, z and the extra word copied, the request queued to start), so a car keeps one
 * request however often it plays. Views of the engine's structures (engine_sound_lists.c, engine_sound_driver.c): */
typedef char tm2_port_sound_slots[(TM2_PORT_SOUND_SLOTS == TM2_CAR_SLOT_CAP) ? 1 : -1];
struct PortSndReq {                     /* SndObj, a sound request (pool sndObjPool, 35) */
    int f00;
    struct PortSndReq *next;            /* +0x04 in its id's list */
    struct PortSndReq *prev;
    int task_state;                     /* +0x0C queued: 0 start, 1 stop (Task.state) */
    int f10, f14;
    int sound;                          /* +0x18 the sound id (sndObjSetupFromRecord: record byte 4) */
    int kind;                           /* +0x1C list policy (sndListSubmit): 0..9 merge when full */
    unsigned int x;                     /* +0x20 distance (the driver's priority) */
    int y;                              /* +0x24 pan */
    int z;                              /* +0x28 pitch / volume scale */
    int instance;                       /* +0x2C the variant: soundPlay 0, soundPlayVariant e */
    int extra;                          /* +0x30 */
};
struct PortSndList { int f00, max, count; struct PortSndReq *head, *tail; };
struct PortSndEffect { int state; signed char b[8]; int kind; int pad; };    /* sndEffectTable, 20 bytes */
struct PortTaskMgr { int f00; void *pending[45]; int npending; };           /* TaskManager */
struct PortSndInst {                    /* SoundInst (soundInstPool) */
    unsigned char pad0[2];
    unsigned char prior;                /* +0x02 the tone's VagAtr priority (SsUtGetVagAtr) */
    unsigned char pad3[0x29];
    struct PortSndReq *owner;           /* +0x2C the request it sounds for */
    unsigned char pad30[0x40];
    unsigned int priority;              /* +0x70 the request's distance */
};
extern unsigned char *soundManagerObject;
extern struct PortSndEffect sndEffectTable[];
extern int sndPoolFreeCount;
extern unsigned int sndMaxDistance;
extern int gameMode;
extern int soundPlay(int, int, int, int);
extern int soundPlayVariant(int, int, int, int, int);
extern int soundStop(int);
extern int soundStopInstance(int, int);
extern unsigned char soundIsActive(int);
extern void soundSetActive(int, unsigned char);
extern void sndListSetMax(void *, int);
extern void taskMgrQueueStart(void *, void *);
extern int soundStartPlayerLoop(int, int, int, int, int);
extern int soundStopPlayerLoop(int, int);
extern unsigned char soundGetTurboLoopP1(void);
extern unsigned char soundGetTurboLoopP2(void);
extern int soundGetGunLoopP1(void);
extern int soundGetGunLoopP2(void);
extern unsigned char soundGetLoop32P1(void);
extern unsigned char soundGetLoop32P2(void);
#if TM2_MAX_FLAME_EMITTERS <= TM2_RETAIL_FLAME_EMITTERS
extern int weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS][32 / 4];
#endif
#define SND_LIST(id) ((struct PortSndList *)(soundManagerObject + (id) * 20 + 0xCC))
#define SND_FLAME 47
#define SND_TORNADO 0x35
#define CS portCarSound
/* Request 87 (spu_voices.h): a per-car play tags its request with the car and gives the caller's tag back after it (the
 * flame and Dark Tooth's swoop run inside weaponFireSpecial's tag). Empty without TM2_PORT_SPU48. */
#if TM2_SPU_VOICES > 24
#define PORT_VOICE_DECL int voice_tag_ = portSoundTag;
#define PORT_VOICE_TAG(slot, kind) TM2_VOICE_TAG(slot, kind)
#define PORT_VOICE_END() (portSoundTag = voice_tag_)
#else
#define PORT_VOICE_DECL
#define PORT_VOICE_TAG(slot, kind) ((void)0)
#define PORT_VOICE_END() ((void)0)
#endif

/* Free sound requests the allocator may still hand out: with the pool raised by TM2_PORT_CARS (request 33-4) and
 * portCarOptions bit 3 clear, sndPoolAlloc hands out only the first TM2_RETAIL_SOUND_REQUESTS. */
static int PORT_TEXT portSndPoolFree(void)
{
    int n;

    n = sndPoolFreeCount;
#if TM2_MAX_SOUND_REQUESTS > TM2_RETAIL_SOUND_REQUESTS
    n -= TM2_MAX_SOUND_REQUESTS - TM2_SOUND_REQUESTS_LIVE;
    if (n < 0)
        n = 0;
#endif
    return n;
}

/* The engine sound of a car type (carSoundUpdate's switch; Dark Tooth and others: none). */
static int PORT_TEXT portCarSoundOfType(int type)
{
    switch (type) {
    case 0x10: return 0x3C;
    case 0x20: return 0x24;
    case 0x30: return 0x26;
    case 0x40: return 0x28;
    case 0x50: return 0x2A;
    case 0x60: return 0x2C;
    case 0x70: return 0x2E;
    case 0x80: return 0x30;
    case 0x90: return 0x32;
    case 0xA0: return 0x34;
    case 0xB0: return 0x36;
    case 0xC0: return 0x38;
    case 0xD0: return 0x3A;
    case 0xE0: return 0x41;
    }
    return -1;
}

static int PORT_TEXT portSoundIsEngine(int snd)
{
    int t;

    for (t = 0x10; t <= 0xE0; t += 0x10)
        if (portCarSoundOfType(t) == snd)
            return 1;
    return 0;
}

/* Play sound `snd` as instance `inst`: merged into that instance's request when its list has one (kinds 0..9,
 * the merging kinds: x, y, z, extra copied, the request queued to start unless it already is this frame, the
 * id's active flag set, as soundMgrPlay + sndListMergeById do), else soundPlayVariant (a new request: inserted
 * while the list has room; a full list applies the id's retail policy). A new engine request is refused while
 * the request pool has TM2_PORT_SOUND_POOL_RESERVE or fewer objects free (they stay for the effects).
 * Returns 1 when the instance plays (merged or submitted). */
static int PORT_TEXT portSoundPlayInstance(int snd, int vol, int pan, int pitch, int inst)
{
    struct PortSndReq *n;
    struct PortTaskMgr *m;
    int i;

#ifdef TM2_PORT_ROLLBACK
    if (portRollbackSoundOff())     /* request 48 stage 3 SOUND_GATE: a re-run plays nothing (as if it played) */
        return 1;
#endif
    if (soundManagerObject != 0 && (unsigned int)snd < 67 && sndEffectTable[snd].state == 1 &&
        (unsigned int)sndEffectTable[snd].kind <= 9) {
        for (n = SND_LIST(snd)->head; n != 0; n = n->next) {
            if (n->instance != inst)
                continue;
            n->x = vol;
            n->y = pan;
            n->z = pitch;
            n->extra = 0;
            if (n->x <= sndMaxDistance) {
                m = (struct PortTaskMgr *)soundManagerObject;
                for (i = 0; i < m->npending; i++)
                    if (m->pending[i] == (void *)n)
                        break;
                if (i < m->npending)
                    n->task_state = 0;      /* queued this frame: start it (once) */
                else
                    taskMgrQueueStart(m, n);
            }
            soundSetActive(snd, 1);
            return 1;
        }
    }
    if (portSoundIsEngine(snd) && portSndPoolFree() <= TM2_PORT_SOUND_POOL_RESERVE)
        return 0;
    soundPlayVariant(snd, vol, pan, pitch, inst);
    return 1;
}

/* Request 33-4: the sounds weaponFireSpecial plays when a special fires (one-shots of kind 12: a new request per
 * fire, dropped while the id's list is full), soundMgrInitLists' size of each. With bit 4 (on = 1) every list holds
 * at least TM2_PORT_SPECIAL_VOICES, so two cars of a type firing at once are both heard (retail: 41, 45, 53 and 57
 * hold one, so the second car's special is silent while the first one's sound plays); on = 0: retail's sizes. */
static void PORT_TEXT portSpecialSoundLists(int on)
{
    int id, n;

    for (id = 0x27; id <= 0x3D; id += 2) {          /* 39..61 odd: the ten special-fire sounds */
        if (id == SND_FLAME || id == 0x33)
            continue;                               /* 47 Thumper's flame (above), 51 the boomerang (by projectile) */
        if (id == 0x29 || id == 0x2D || id == 0x35 || id == 0x39)
            n = 1;
        else if (id == 0x27 || id == 0x3B)
            n = 2;
        else
            n = 3;                                  /* 0x2B, 0x31, 0x37, 0x3D */
        if (on && n < TM2_PORT_SPECIAL_VOICES)
            n = TM2_PORT_SPECIAL_VOICES;
        sndListSetMax(SND_LIST(id), n);
    }
}

/* Bit 4 turned off: stop every instance this code started and give the lists retail's size (1). */
static void PORT_TEXT portCarSoundOff(void)
{
    int s, t;

    for (s = 0; s < TM2_PORT_SOUND_SLOTS; s++) {
        if (CS.engine[s] != 0)
            soundStopInstance(CS.engine[s], s + 1);
        if (CS.flame[s] != 0)
            soundStopInstance(SND_FLAME, s + 1);
        if (portCarSoundLoops[s] & TM2_PORT_LOOP_TORNADO)
            soundStopInstance(SND_TORNADO, s + 1);     /* request 33-4 */
        CS.engine[s] = 0;
        CS.flame[s] = 0;
        CS.allow[s] = 0;
        portCarSoundLoops[s] &= ~TM2_PORT_LOOP_TORNADO;
    }
    for (t = 0x10; t <= 0xE0; t += 0x10)
        sndListSetMax(SND_LIST(portCarSoundOfType(t)), 1);
    sndListSetMax(SND_LIST(SND_FLAME), 1);
    sndListSetMax(SND_LIST(61), 3);                 /* soundMgrInitLists: 61 holds 3 */
    sndListSetMax(SND_LIST(66), 1);
    portSpecialSoundLists(0);                       /* request 33-4: retail's sizes (61 again: 3) */
    CS.live = 0;
    CS.candidates = 0;
    CS.allowed = 0;
}

/* Chebyshev distance (the engine's DISTANCE without the >> 3). */
static unsigned int PORT_TEXT portSoundDist(int *a, int *b)
{
    unsigned int d, e;

    d = (unsigned int)__builtin_abs(a[0] - b[0]);
    e = (unsigned int)__builtin_abs(a[1] - b[1]);
    if (e > d)
        d = e;
    e = (unsigned int)__builtin_abs(a[2] - b[2]);
    return e > d ? e : d;
}

/* worldUpdate, first: latch bit 4; with it, the list sizes (engines: 2 per allowed car, room for requests that
 * are stopping; the flame: 2 per emitter; the swoops TM2_PORT_SWOOP_VOICES) and the cars allowed to play their
 * engine: every car that has one to play (a human, or an AI in range or in mode 0; alive) when portEngineVoices
 * is 0, else the portEngineVoices cars nearest to any human (with TM2_PORT_HUMANS a local one: the listeners,
 * HUMANS16_PLAN.md H3) (Chebyshev distance to the nearest human car; a car
 * that plays ranks at 3/4 of its distance, so two cars at the edge do not swap every frame; ties by record
 * order). A slot no record drives any more (unbound, parked by the swap) has its engine stopped here. */
void PORT_TEXT portCarSoundFrame(void)
{
    int slot_of[TM2_MAX_HUMANS + TM2_MAX_AI_CARS];
    unsigned int dist[TM2_MAX_HUMANS + TM2_MAX_AI_CARS];
    unsigned char seen[TM2_PORT_SOUND_SLOTS];
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int *pos;
    int n, i, j, s, t, rank, cap, size;
    unsigned int d, e;

    if (!(portCarOptions & TM2_PORT_CAR_OPT_ENGINE_SOUND) || soundManagerObject == 0) {
        if (CS.live)
            portCarSoundOff();
        return;
    }
    cap = portEngineVoices > 0 ? portEngineVoices : 0;
    if (cap > TM2_PORT_SOUND_SLOTS)
        cap = 0;                                    /* more than there are cars: no cap */
    CS.live = 1;
    CS.voices = cap;
    size = 2 * (cap ? cap : TM2_MAX_HUMANS + TM2_MAX_AI_CARS);
    for (t = 0x10; t <= 0xE0; t += 0x10)
        sndListSetMax(SND_LIST(portCarSoundOfType(t)), size);
    sndListSetMax(SND_LIST(SND_FLAME), 2 * TM2_MAX_FLAME_EMITTERS);
    portSpecialSoundLists(1);                       /* request 33-4: the special-fire sounds */
#ifdef TM2_PORT_DARK_TOOTH
    sndListSetMax(SND_LIST(61), TM2_PORT_SWOOP_VOICES);
    sndListSetMax(SND_LIST(66), TM2_PORT_SWOOP_VOICES);
#endif
    for (s = 0; s < TM2_PORT_SOUND_SLOTS; s++) {
        CS.allow[s] = 0;
        seen[s] = 0;
    }
    n = 0;
    for (i = 0; i < humanCarTotal + aiCarTotal; i++) {
        if (i < humanCarTotal) {
            if (i >= TM2_MAX_HUMANS)
                continue;
            h = (struct TM2HumanRecord *)humanCars[i];
            s = h->slot;
            if ((unsigned int)s >= TM2_PORT_SOUND_SLOTS)
                continue;
            seen[s] = 1;
#ifdef TM2_PORT_DT_PLAYER
            /* request 36: a player's Dark Tooth has an engine (Minion's by default, DARK_TOOTH_PLAYER.md) */
            if ((portCarSoundOfType(h->type) < 0 && (h->type != 0xF0 || TM2_DT_PLAYER_ENGINE() < 0)) ||
                h->state.dead)
#else
            if (portCarSoundOfType(h->type) < 0 || h->state.dead)
#endif
                continue;
            pos = h->motion.pos;
        } else {
            if (i - humanCarTotal >= TM2_MAX_AI_CARS)
                continue;
            a = (struct TM2AIRecord *)aiCars[i - humanCarTotal];
            s = a->slot;
            if ((unsigned int)s >= TM2_PORT_SOUND_SLOTS)
                continue;
            seen[s] = 1;
            if (portCarSoundOfType(a->type) < 0 || a->state.dead || (!a->active && gameMode))
                continue;
            pos = a->motion.pos;
        }
        d = 0x7FFFFFFF;
        for (j = 0; j < humanCarTotal && j < TM2_MAX_HUMANS; j++) {
#ifdef TM2_PORT_HUMANS
            if (portHumanSlot(j) < 0)
                continue;       /* H3 (request 37 answer 12): the listeners are this machine's humans */
#endif
            e = portSoundDist(pos, ((struct TM2HumanRecord *)humanCars[j])->motion.pos);
            if (e < d)
                d = e;
        }
        if (CS.engine[s] != 0)
            d -= d >> 2;
        slot_of[n] = s;
        dist[n] = d;
        n++;
    }
    for (i = 0; i < n; i++) {
        rank = 0;
        if (cap)
            for (j = 0; j < n; j++)
                if (dist[j] < dist[i] || (dist[j] == dist[i] && j < i))
                    rank++;
        if (!cap || rank < cap)
            CS.allow[slot_of[i]] = 1;
    }
    CS.candidates = n;
    CS.allowed = cap && cap < n ? cap : n;
    for (s = 0; s < TM2_PORT_SOUND_SLOTS; s++)
        if (!seen[s] && CS.engine[s] != 0) {
            soundStopInstance(CS.engine[s], s + 1);
            CS.engine[s] = 0;
        }
}

/* The engine of the car in `slot`: carSoundUpdate (snd = its type's engine or -1, on = !state.dead) and
 * worldUpdate's out-of-range AI (on = 0, snd = retail's `sound`). Bit 4 clear: retail's calls (snd != -1:
 * soundPlay when on, else soundIsActive -> soundStop). Bit 4: the slot's own instance plays while on and
 * allowed, and stops otherwise (or when its sound id changes: a swap). */
void PORT_TEXT portCarEngineUpdate(int slot, int snd, int on, int vol, int pan, int pitch)
{
    int cur;
    PORT_VOICE_DECL

    if (!CS.live || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS) {
        if (snd != -1) {
            if (on) {
                PORT_VOICE_TAG(slot, TM2_VOICE_ENGINE);    /* request 87: (one request for the type: the first car's) */
                soundPlay(snd, vol, pan, pitch);
                PORT_VOICE_END();
            }
            else if (soundIsActive(snd))
                soundStop(snd);
        }
        return;
    }
    cur = CS.engine[slot];
    if (cur != 0 && (cur != snd || !on || !CS.allow[slot])) {
        soundStopInstance(cur, slot + 1);
        CS.engine[slot] = 0;
    }
    PORT_VOICE_TAG(slot, TM2_VOICE_ENGINE);
    if (snd != -1 && on && CS.allow[slot] && portSoundPlayInstance(snd, vol, pan, pitch, slot + 1))
        CS.engine[slot] = snd;
    PORT_VOICE_END();
}

/* soundPlay(snd, vol, pan, pitch) as instance slot + 1 with bit 4 (Dark Tooth's swoops), else retail's call. */
void PORT_TEXT portCarSoundPlay(int snd, int vol, int pan, int pitch, int slot)
{
    PORT_VOICE_DECL
    PORT_VOICE_TAG(slot, TM2_VOICE_SPECIAL);    /* request 87: the swoops are Dark Tooth's special */
    if (!CS.live || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS)
        soundPlay(snd, vol, pan, pitch);
    else
        portSoundPlayInstance(snd, vol, pan, pitch, slot + 1);
    PORT_VOICE_END();
}

/* weaponFlameStart: soundPlay(47, vol, pan, 99), with bit 4 as the flaming car's own instance. */
void PORT_TEXT portCarFlameSound(int slot, int vol, int pan)
{
    PORT_VOICE_DECL
    PORT_VOICE_TAG(slot, TM2_VOICE_SPECIAL);    /* request 87 */
    if (!CS.live || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS) {
        soundPlay(SND_FLAME, vol, pan, 99);
    } else {
        portSoundPlayInstance(SND_FLAME, vol, pan, 99, slot + 1);
        CS.flame[slot] = 1;
    }
    PORT_VOICE_END();
}

/* weaponFlameUpdate, an emitter's timer ran out: with bit 4 the car's flame sound stops unless another of its
 * emitters still runs. */
void PORT_TEXT portCarFlameSoundEnd(void *model)
{
    int e, s;

    if (!CS.live)
        return;
    for (e = 0; e < TM2_MAX_FLAME_EMITTERS; e++)
        if (*(short *)&weaponFlameEmitters[e][0] > 0 && *(void **)&weaponFlameEmitters[e][7] == model)
            return;
    for (s = 0; s < carWorldObjectCount && s < TM2_PORT_SOUND_SLOTS; s++)
        if (carSlotObjects[s] == model && CS.flame[s]) {
            soundStopInstance(SND_FLAME, s + 1);
            CS.flame[s] = 0;
        }
}

/* weaponFlameUpdate's end: retail stops sound 47 when no emitter runs and the id is active; with bit 4 also
 * when an instance of it was started (an instance stop clears the id's flag while others play). */
void PORT_TEXT portCarFlameSoundIdle(int active)
{
    int s, any;

    if (active != 0)
        return;
    any = 0;
    if (CS.live)
        for (s = 0; s < TM2_PORT_SOUND_SLOTS; s++)
            if (CS.flame[s]) {
                any = 1;
                CS.flame[s] = 0;
            }
    if (any || (soundIsActive(SND_FLAME) & 255))
        soundStop(SND_FLAME);
}

/* The port's API (and the swap's portCarResetLook): stop the engine and flame instances of `slot`. Between
 * frames; with bit 4 clear nothing was started per slot. */
void PORT_TEXT portCarSoundStopSlot(int slot)
{
    if ((unsigned int)slot >= TM2_PORT_SOUND_SLOTS)
        return;
    if (CS.engine[slot] != 0)
        soundStopInstance(CS.engine[slot], slot + 1);
    if (CS.flame[slot] != 0)
        soundStopInstance(SND_FLAME, slot + 1);
    CS.engine[slot] = 0;
    CS.flame[slot] = 0;
}

/* soundSortInstances with bit 4 (soundDriverUpdate: the sorted instances past soundVoiceLimit, 24 or 44, lose their
 * voice). Retail sorts by distance, then (an unstable quicksort) by the tone's VagAtr priority, so the effective
 * key is the VAB priority with distance only partly kept. Here, stable: every instance of another sound by
 * (VagAtr priority, distance), then the engine instances by distance (the farthest engine is the first to go).
 * Returns 0 (retail sorts) while bit 4 is not live. */
int PORT_TEXT portCarSoundSortInstances(void **a, int lo, int hi)
{
    struct PortSndInst *p, *q;
    int i, j, ep, eq, later;

    if (!CS.live)
        return 0;
    for (i = lo + 1; i <= hi; i++) {
        p = (struct PortSndInst *)a[i];
        ep = p->owner != 0 && portSoundIsEngine(p->owner->sound);
        for (j = i - 1; j >= lo; j--) {
            q = (struct PortSndInst *)a[j];
            eq = q->owner != 0 && portSoundIsEngine(q->owner->sound);
            if (eq != ep)
                later = eq > ep;
            else if (!ep && q->prior != p->prior)
                later = q->prior > p->prior;
            else
                later = q->priority > p->priority;
            if (!later)
                break;
            a[j + 1] = q;
        }
        a[j + 1] = p;
    }
    return 1;
}

/* ---- request 33 step 4 (CAR_GROUPS.md 16): the human loops and the tornado sound per car ---- */
static int PORT_TEXT portLoopBit(int snd)
{
    return snd == 0 ? TM2_PORT_LOOP_TURBO : snd == 9 ? TM2_PORT_LOOP_GUN : snd == 32 ? TM2_PORT_LOOP_BURN : 0;
}

/* The size the lists of 0, 9 and 32 get for the human loops played as instances: every human above 1, twice
 * (requests that are stopping). */
static void PORT_TEXT portHumanLoopList(int snd)
{
    int want;

    want = 2 * (TM2_MAX_HUMANS - TM2_RETAIL_HUMANS);
    if (want < 2)
        want = 2;
    if (SND_LIST(snd)->max < want)
        sndListSetMax(SND_LIST(snd), want);
}

#ifndef TM2_PORT_HUMANS
/* (With TM2_PORT_HUMANS these two are step H3's, by sound side: port_humans.h.)
 * soundStartPlayerLoop(snd, vol, pan, pitch, player) of a human: players 0 and 1 keep retail's call (their side
 * lists); a player of 2 or more, which retail's soundMgrPlaySided leaks a request for, plays the loop as instance
 * slot + 1 in the id's own list (lists 0, 9 and 32 are not used by retail; they get room for every human above
 * 1, twice: requests that are stopping). Not tied to bit 4: retail has no path for these players. */
void PORT_TEXT portHumanLoopPlay(int snd, int vol, int pan, int pitch, int player, int slot)
{
    int bit;

    if ((unsigned int)player < TM2_RETAIL_HUMANS) {
        soundStartPlayerLoop(snd, vol, pan, pitch, player);
        return;
    }
    bit = portLoopBit(snd);
    if (bit == 0 || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS || soundManagerObject == 0)
        return;
    portHumanLoopList(snd);
    if (portSoundPlayInstance(snd, vol, pan, pitch, slot + 1))
        portCarSoundLoops[slot] |= bit;
}

/* The stop beside each of those plays: players 0 and 1 make retail's calls ("if (player == 0 && its P1 flag)
 * soundStopPlayerLoop(snd, 0)", the same for 1 and P2); players 2.. stop their instance when it was started. */
void PORT_TEXT portHumanLoopStop(int snd, int player, int slot)
{
    int bit, on;

    if ((unsigned int)player < TM2_RETAIL_HUMANS) {
        if (snd == 0)
            on = player == 0 ? soundGetTurboLoopP1() : soundGetTurboLoopP2();
        else if (snd == 9)
            on = (player == 0 ? soundGetGunLoopP1() : soundGetGunLoopP2()) & 255;
        else if (snd == 32)
            on = player == 0 ? soundGetLoop32P1() : soundGetLoop32P2();
        else
            on = 0;
        if (on)
            soundStopPlayerLoop(snd, player);
        return;
    }
    bit = portLoopBit(snd);
    if (bit == 0 || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS || !(portCarSoundLoops[slot] & bit))
        return;
    soundStopInstance(snd, slot + 1);
    portCarSoundLoops[slot] &= ~bit;
}
#endif /* !TM2_PORT_HUMANS */

/* weaponFireSpecial, Twister: soundPlay(0x35, vol, pan, 99); with bit 4 the Twister's own instance (kind 12: a new
 * request each fire, dropped while the list is full: TM2_PORT_SPECIAL_VOICES with bit 4, retail 1). */
void PORT_TEXT portCarTornadoSound(int slot, int vol, int pan)
{
    if (!CS.live || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS) {
        soundPlay(SND_TORNADO, vol, pan, 99);
        return;
    }
    portSoundPlayInstance(SND_TORNADO, vol, pan, 99, slot + 1);
    portCarSoundLoops[slot] |= TM2_PORT_LOOP_TORNADO;
}

/* A Twister died (carDestroyed) or its tornado was ended by the port code: retail's soundStop(0x35) stops every
 * tornado sound; with bit 4 only this Twister's instance. */
void PORT_TEXT portCarTornadoSoundStop(int slot)
{
    if (!CS.live || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS) {
        soundStop(SND_TORNADO);
        return;
    }
    if (portCarSoundLoops[slot] & TM2_PORT_LOOP_TORNADO)
        soundStopInstance(SND_TORNADO, slot + 1);
    portCarSoundLoops[slot] &= ~TM2_PORT_LOOP_TORNADO;
}
#undef CS
#endif /* TM2_PORT_CAR_SOUND */

#ifdef TM2_PORT_CAR_SWAP

/* The pools whose records hold a car's object or slot (weapon_flame_update.c, weapon_slot_target.c,
 * asset_pools.c, weapon_slot_writer.c): defined above when TM2_PORT_CARS raises them, else retail's. */
#if TM2_MAX_FLAME_EMITTERS <= TM2_RETAIL_FLAME_EMITTERS
extern int weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS][32 / 4];
#endif
#if TM2_MAX_ARCS <= TM2_RETAIL_ARCS
extern int weaponArcs[TM2_MAX_ARCS][20 / 4];
#endif
#if TM2_MAX_SHIELDS <= TM2_RETAIL_SHIELDS
extern int weaponShieldPool[TM2_MAX_SHIELDS * (4 + 12) / 4];
#endif
#define PORT_DROPS TM2_MAX_DROPS   /* 25; 6 N with TM2_PORT_BALANCE (request 40) */
extern unsigned char weaponDrops[PORT_DROPS * 28];
#define EMITTER_TIMER(e) (*(short *)&weaponFlameEmitters[e][0])          /* +0x00 frames left */
#define EMITTER_MODEL(e) (*(struct TM2Object **)&weaponFlameEmitters[e][7]) /* +0x1C the car object */
#define ARC_OWNER(a) (((short *)weaponArcs[a])[1])                        /* +0x02 taser shooter's slot */
#define SHIELD_SOURCE(s) (*(struct TM2Object **)((unsigned char *)weaponShieldPool + 4 * TM2_MAX_SHIELDS + 12 * (s) + 4))
#define DROP_SLOT(d) (*(short *)(weaponDrops + 28 * (d) + 6))             /* +0x06 dropper's slot */
#define PROJ_TARGET(p) (*(int **)&fxRecords[p][0x38 / 4])                  /* a car object's pos (+0xA8) */
#define PROJ_VICTIM(p) (*(struct TM2Object **)&fxRecords[p][0x3C / 4])    /* boomerang: the shooter's object */

#define PORT_FAR_AWAY (-1000000)    /* SURVIVAL.md 3.1: a parked car's height, out of every distance test */

/* A record seen through its blocks. */
struct PortCarRec {
    int kind, index;                /* 0 human, 1 AI */
    void *rec;
    short *slot;
    int *type;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarMotion *motion;
    struct TM2CarWheels *wheels;
    struct TM2CarWeapons *weapons;
};

/* Human i (< humanCarTotal) or AI j (< aiCarTotal); 0 if there is no such record. */
static int PORT_TEXT portCarRec(int kind, int index, struct PortCarRec *r)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;

    r->kind = -1;
    if (kind == 0) {
        if ((unsigned int)index >= TM2_MAX_HUMANS || index >= humanCarTotal)
            return 0;
        h = (struct TM2HumanRecord *)humanCars[index];
        r->rec = h;
        r->slot = &h->slot;
        r->type = &h->type;
        r->state = &h->state;
        r->stats = &h->stats;
        r->motion = &h->motion;
        r->wheels = &h->wheels;
        r->weapons = &h->weapons;
    } else if (kind == 1) {
        if ((unsigned int)index >= TM2_MAX_AI_CARS || index >= aiCarTotal)
            return 0;
        a = (struct TM2AIRecord *)aiCars[index];
        r->rec = a;
        r->slot = &a->slot;
        r->type = &a->type;
        r->state = &a->state;
        r->stats = &a->stats;
        r->motion = &a->motion;
        r->wheels = &a->wheels;
        r->weapons = &a->weapons;
    } else {
        return 0;
    }
    r->kind = kind;
    r->index = index;
    return 1;
}

/* The n-th record of the match: the humans, then the AI records. */
static int PORT_TEXT portCarRecAt(int n, struct PortCarRec *r)
{
    if (n < humanCarTotal)
        return portCarRec(0, n, r);
    return portCarRec(1, n - humanCarTotal, r);
}

/* A slot of this load (a registered car root). */
static int PORT_TEXT portCarSlotValid(int slot)
{
    return (unsigned int)slot < TM2_MAX_CAR_SLOTS && slot < carWorldObjectCount;
}

/* The record bound to a slot (carSlotKinds 0/1), 0 if none. */
static int PORT_TEXT portCarDriver(int slot, struct PortCarRec *r)
{
    r->kind = -1;
    if (!portCarSlotValid(slot) || (carSlotKinds[slot] != 0 && carSlotKinds[slot] != 1))
        return 0;
    if (!portCarRec(carSlotKinds[slot], carSlotIndices[slot], r) || *r->slot != slot) {
        r->kind = -1;
        return 0;
    }
    return 1;
}

static int PORT_TEXT portCarAlive(struct PortCarRec *r)
{
    return r->state->dead == 0 && r->stats->hp > 0;
}

/* SURVIVAL.md 3.1: an AI record that is off (dead, HP 0, wreck state 4). */
static int PORT_TEXT portCarParked(struct PortCarRec *r)
{
    return r->kind == 1 && r->state->dead != 0 && r->stats->hp <= 0 && r->stats->wreck_state == 4;
}

/* The engine sound of a car type (carSoundUpdate's table; Dark Tooth has none). */
static int PORT_TEXT portCarEngineSound(int type)
{
    switch (type) {
    case 0x10: return 0x3C;
    case 0x20: return 0x24;
    case 0x30: return 0x26;
    case 0x40: return 0x28;
    case 0x50: return 0x2A;
    case 0x60: return 0x2C;
    case 0x70: return 0x2E;
    case 0x80: return 0x30;
    case 0x90: return 0x32;
    case 0xA0: return 0x34;
    case 0xB0: return 0x36;
    case 0xC0: return 0x38;
    case 0xD0: return 0x3A;
    case 0xE0: return 0x41;
    }
    return -1;
}

/* End every special state that holds the car in `slot` (its object, its key, the record driving it), each
 * the way the game ends it itself. With retail's single special states (before request 33-2) a state is
 * ended only when it belongs to this car.
 *   Mr. Slam: a claw that holds this car (spin_target = its key) lets go as carDestroyed does it
 *             (carSpinOutById, active 0, spin_target -1); the driver's own slam stops the same way;
 *   crush:    a record riding this car (stats.crush_owner = its key) loses the contact;
 *   Twister:  the tornado fired from this car's object ends as weaponTornadoUpdate's end block;
 *   Shadow:   the strike fired by the driver's weapons block ends as strike_update.c's end (no blast);
 *   Grasshopper: the leap from this slot stops, its turbo flame off;
 *   Thumper:  its flame emitters (object = this car's) stop;
 *   Axel:     the shockwave animation (2324) back to step 0;
 *   Dark Tooth: the boss controller's counters restart and a swoop ends (the stage flag is the round
 *             rule's and stays); with TM2_PORT_DARK_TOOTH (request 33-2e) this car's own controller, and
 *             its stage goes back to 1 (the record that drives it next gets stage-1 stats);
 *   borrowed: with TM2_PORT_SPECIAL_AS (request 35) the slot's as-entry ends (portCarSpecialAsEnd) and the slot
 *             fires its own special again (portCarSpecialAs -1). */
void PORT_TEXT portCarResetSpecials(int slot)
{
    struct PortCarRec d, r;
    struct TM2Object *obj;
    struct TM2CarSlam *s;
    int type, key, have, n, e, mine;
#ifdef TM2_PORT_CAR_SPECIALS
    int sel;
#endif

    if (!portCarSlotValid(slot))
        return;
    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    key = portCarKey(slot, type);
    have = portCarDriver(slot, &d);
    for (n = 0; portCarRecAt(n, &r); n++) {
        mine = have && r.rec == d.rec;
        s = r.weapons->slam;
        if (s != 0 && *r.type == 0xC0) {
            if (s->spin_target > 0 && (s->spin_target == key || mine)) {
                carSpinOutById(s->spin_target, (short)r.motion->rot[2], 0);
                s->spin_target = -1;
                s->active = 0;
                s->spin_pending = 0;
            } else if (mine) {
                s->active = 0;
                s->spin_pending = 0;
            }
        }
        if (r.stats->crush_owner == key)
            r.state->crush_contact = 0;
    }
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a: the special state below is this car's own (its key's) */
    sel = portCarSpecialSelect(key);
    if (type == 0xC0) {
        slamClawSetAngle(739);          /* the arm and claw back to rest, as carStatsLoad and the round reset */
        slamArmSetAngle(-113);
    }
#endif
    if (weaponTornadoState.timer > 0 && weaponTornadoState.object == obj) {
        weaponTornadoState.timer = 0;
        weaponTornadoObject->visible = 0;
        weaponTornadoFunnelObject->visible = 0;
        if (*weaponTornadoState.anchor > 0) {
            obj->visible = 1;
            carBodyObjectShow(key);
        } else {
            carBodyObjectHide(key);
        }
#ifdef TM2_PORT_CAR_SOUND
        portCarTornadoSoundStop(slot);  /* request 33-4: this car's tornado sound only (bit 4) */
#else
        soundStop(0x35);
#endif
    }
    if (have && weaponStrikeState.active != 0 && weaponStrikeState.weapon == d.weapons) {
        weaponStrikeState.active = 0;
        weaponStrikeState.timer = 0;
        objRegistrySetStep(0x915, 0, 0);
        if (weaponStrikeState.object != 0)
            weaponStrikeState.object->visible = 0;
        if (d.weapons->ammo[0] > 0)
            d.weapons->ammo[0]--;           /* the strike's own end takes the special it refunded at fire */
    }
    if (homingLeapState.active != 0 && homingLeapState.owner_id == slot) {
        homingLeapState.active = 0;
        portCarTurboSetStep(key, 0);
    }
    for (e = 0; e < TM2_MAX_FLAME_EMITTERS; e++)
        if (EMITTER_TIMER(e) > 0 && EMITTER_MODEL(e) == obj)
            EMITTER_TIMER(e) = 0;
    if (type == 0xB0)
        objRegistrySetStep(0x914, 0, 0);
#ifdef TM2_PORT_DARK_TOOTH
    if (type == 0xF0) {             /* request 33-2e: this Dark Tooth's own controller (selected above) */
        aiDarkToothShotTimer = 0;
        aiDarkToothSwoopTimer = 0;
        darkToothSwooping = 0;
        aiBossDefeatedFlag = 0;
    }
#endif
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(sel);
#endif
#ifndef TM2_PORT_DARK_TOOTH
    if (type == 0xF0) {
        aiDarkToothShotTimer = 0;
        aiDarkToothSwoopTimer = 0;
        darkToothSwooping = 0;
    }
#endif
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35: a borrowed strike or ring of this slot ends too, and the slot fires its own special again (the port
       sets the borrowed one again for the record that drives it next) */
    portCarSpecialAsEnd(slot);
    portCarSpecialAs[slot] = -1;
#endif
}

/* Reset the look of the car in `slot` and of the record driving it: the ice (carFreezeSetState) and the
 * turbo flame of its key, its engine sound; the driver's freeze (only while alive: a wreck stays frozen),
 * fire and invisibility (the CLUT fade back, body and object shown). */
void PORT_TEXT portCarResetLook(int slot)
{
    struct PortCarRec d;
    struct TM2Object *obj;
    int type, key, snd, alive;

    if (!portCarSlotValid(slot))
        return;
    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    key = portCarKey(slot, type);
    carFreezeSetState(key, 0);
    portCarTurboSetStep(key, 0);
#ifdef TM2_PORT_CAR_SOUND
    if (portCarSound.live)
        portCarSoundStopSlot(slot);     /* request 33-3: this car's own engine and flame instances */
    else
#endif
    {
        snd = portCarEngineSound(type);
#ifdef TM2_PORT_DT_PLAYER
        /* request 36: a player's Dark Tooth plays an engine (Minion's by default): stop it as the others' */
        if (snd < 0 && type == 0xF0 && portCarDriver(slot, &d) && d.kind == 0)
            snd = TM2_DT_PLAYER_ENGINE();
#endif
        if (snd >= 0 && soundIsActive(snd))
            soundStop(snd);
    }
    if (!portCarDriver(slot, &d))
        return;
    alive = portCarAlive(&d);
    if (alive) {
        d.state->frozen = 0;
        d.stats->freeze_timer = 0;
    }
    d.state->burning = 0;
    d.stats->burn_time = 0;
    if (d.weapons->fire_effect != 0)
        d.weapons->fire_effect->active = 0;
    if (d.weapons->invisible) {
        /* weaponInvisibilityUpdate fades the CLUT out at 120 and back in at 25 */
        if (d.weapons->invis_timer < 120 && d.weapons->invis_timer >= 25)
#ifdef TM2_PORT_TEX_SHARED
            clutAnimFadeIn(portTexFadeId(slot, *d.type), 0);   /* step 4 / 33-1: the car's own fade */
#else
            clutAnimFadeIn(*d.type + 8, 0);
#endif
        d.weapons->invisible = 0;
        d.weapons->invis_timer = 0;
        if (alive) {
            carBodyObjectShow(key);
            obj->visible = 1;
        }
    }
}

/* The record leaves its slot: the slot becomes a loaded car without a driver (kind 2, index -1). The record
 * keeps its object pointers until it is bound again. */
void PORT_TEXT portCarUnbind(int kind, int index)
{
    struct PortCarRec r;
    int s;

    if (!portCarRec(kind, index, &r))
        return;
    s = *r.slot;
    if (portCarSlotValid(s) && carSlotKinds[s] == kind && carSlotIndices[s] == index) {
        carSlotKinds[s] = 2;
        carSlotIndices[s] = -1;
    }
}

/* Bind a record to a slot as carBindHumanSlot (8011A3FC) / carBindAiSlot (8011A5CC) do, without their
 * carRestart / aiCarStart: the object and slot tables, the record's slot, the slot's kind and index, the
 * object id (human TM2_HUMAN_ID(i), AI j + 50: ids follow the record, so kill credits by id stay the player's). */
void PORT_TEXT portCarBindSlot(int kind, int index, int slot)
{
    struct PortCarRec r;
    struct TM2Object *obj;

    if (!portCarRec(kind, index, &r) || !portCarSlotValid(slot))
        return;
    obj = (struct TM2Object *)carSlotObjects[slot];
    if (kind == 0) {
        humanCarObjects[index] = (int)obj;
        humanCarObjectSlots[index] = slot;
        humanCarTypes[index] = carSlotOwners[slot];
        obj->id = TM2_HUMAN_ID(index);
    } else {
        aiCarObjects[index] = (int)obj;
        aiCarObjectSlots[index] = slot;
        obj->id = index + 50;
    }
    carSlotIndices[slot] = index;
    carSlotKinds[slot] = kind;
    *r.slot = slot;
}

/* The record takes on the type of the car its slot holds (carSlotOwners): stats, wheels, collision box,
 * weapons, camera (human) or AI profile, as humanCarPlaceAtStart / aiCarReset / weaponBlockInit set them
 * at a start, while the record keeps where it is and what it does (motion, route, targets).
 *   stats:   reloaded for the new type at the record's own speed multiplier (human 100, AI by difficulty
 *            unless the game changed it); human max_hp + 20 as humanCarPlaceAtStart; the per-life fields
 *            carStatsLoad resets are kept (killer, last attacker, shield, damageable, restart pending and
 *            three unnamed ones), the freeze and burn timers and the riding flag start over;
 *   HP:      hp < 0 scales (hp * new max / old max, at least 1 while alive), hp >= 0 sets (1..new max);
 *            a dead record keeps its HP (no revive outside the game's respawn);
 *   height:  the ride-height difference goes into motion.pos[2];
 *   weapons: weaponBlockInit for the new type (recharge, capacity, max specials, Mr. Slam's claw record),
 *            then the pickups, the selected weapon, energy, turbo and the drop/rear-fire states are put
 *            back; ammo[0] (specials) is kept, clamped to the new type's max_specials (G.4).
 * Also the mod hook of PER_CAR_PLAN.md E. Returns 1, 0 if there is no such record or slot. */
int PORT_TEXT portCarApplyType(int kind, int index, int hp)
{
    struct PortCarRec r;
    struct TM2CarStatsBlock *st;
    struct TM2CarWeapons *w;
    short ammo[10];
    int type, alive, i, v;
    int old_hp, old_max, old_ride, mult;
    int killer, last, shield, damageable, restart, f03, f16, f28, regen, wreck;
    int selected, energy, turbo, held, rear, drop;

    if (!portCarRec(kind, index, &r) || !portCarSlotValid(*r.slot))
        return 0;
    type = carSlotOwners[*r.slot];
    st = r.stats;
    alive = portCarAlive(&r);
    old_hp = st->hp;
    old_max = st->max_hp;
    old_ride = st->ride_height;
    mult = st->speed_mult;
    killer = st->killer;
    last = st->last_attacker;
    shield = st->shield;
    damageable = st->damageable;
    restart = st->restart_pending;
    f03 = st->f03;
    f16 = st->f16;
    f28 = *(short *)((unsigned char *)st + 0x28);
    regen = st->regen_clock;
    wreck = st->wreck_state;

    *r.type = type;
    carStatsSetSpeedMult(st, mult);
    carStatsLoad(st, type);
#ifdef TM2_PORT_DT_PLAYER
    if (kind == 0)
        portCarDtPlayerStart(r.rec);    /* request 118: a human Dark Tooth's phase (stage 2's stats before the +20) */
    else if (type == 0xF0)
        portCarDtPhase2Off(*r.slot);    /* request 118: a computer driver gets the truck and stage 1 back */
#endif
    if (kind == 0)
        st->max_hp += 20;
    else
        carStatsSetTopSpeed(st, st->top_speed);
    st->killer = killer;
    st->last_attacker = last;
    st->shield = shield;
    st->damageable = damageable;
    st->restart_pending = restart;
    st->f03 = f03;
    st->f16 = f16;
    *(short *)((unsigned char *)st + 0x28) = f28;
    st->regen_clock = regen;
    st->wreck_state = wreck;

    if (!alive) {
        st->hp = old_hp;
    } else {
        if (hp < 0)
            v = old_max > 0 ? old_hp * st->max_hp / old_max : st->max_hp;
        else
            v = hp;
        if (v > st->max_hp)
            v = st->max_hp;
        if (v < 1)
            v = 1;
        st->hp = v;
    }
    r.motion->pos[2] += st->ride_height - old_ride;
    carWheelsSetStepHeight(r.wheels, type);
    carBuildCollisionBox(r.rec, kind == 0);

    w = r.weapons;
    for (i = 0; i < 10; i++)
        ammo[i] = w->ammo[i];
    selected = w->selected;
    energy = w->energy;
    turbo = w->turbo;
    held = w->held;
    rear = w->rear_fire;
    drop = w->drop_armed;
    weaponBlockInit(r.rec, w, kind == 0);
    for (i = 0; i < 10; i++)
        w->ammo[i] = ammo[i];
    if (w->ammo[0] > w->max_specials)
        w->ammo[0] = w->max_specials;
    w->selected = selected;
    w->energy = energy;
    w->turbo = turbo;
    w->held = held;
    w->rear_fire = rear;
    w->drop_armed = drop;

    if (kind == 0)
        viewCameraOffsetsInit((struct TM2HumanRecord *)r.rec);
    else
        aiCarSetupProfile((struct TM2AIRecord *)r.rec);
    return 1;
}

/* Show the car a record now drives as the record is: alive = drawn, body shown, damage stage from the HP
 * percentage (> 40 / > 12 / > 0, as the damage updates) with the record's stage field set to it; parked
 * (SURVIVAL.md 3.1) = not drawn, body hidden; dead = drawn as the car before it was (`visible`), body
 * hidden, stage 2. Dark Tooth's head (the single 249 object) is shown for a live AI driver, hidden for a
 * parked one. */
static void PORT_TEXT portCarModelOn(struct PortCarRec *r, int visible)
{
    struct TM2Object *obj;
    int slot, type, key, alive, parked, max, pct, stage;

    slot = *r->slot;
    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    key = portCarKey(slot, type);
    alive = portCarAlive(r);
    parked = portCarParked(r);
    obj->light = lightGetEnv(0);
    csSetDrawEnable(obj, alive ? 1 : parked ? 0 : visible);
    max = r->stats->max_hp > 0 ? r->stats->max_hp : 1;
    pct = r->stats->hp * 100 / max;
    stage = pct > 40 ? 0 : pct > 12 ? 1 : 2;
    carSetDamageStage(key, stage, r->stats->smoke_arg);
    if (alive) {
        r->stats->wreck_state = stage;
        carBodyObjectShow(key);
    } else {
        carBodyObjectHide(key);
    }
    if (type == 0xF0 && DARK_TOOTH_HEAD(key) != 0) {
#ifdef TM2_PORT_DT_PLAYER
        if (alive)          /* request 36: a player's Dark Tooth has his head too (portCarDtPlayerHead) */
#else
        if (r->kind == 1 && alive)
#endif
            DARK_TOOTH_HEAD(key)->visible = 1;
        else if (parked)
            DARK_TOOTH_PARK_HEAD(slot, key);
    }
}

/* A loaded car nobody drives (SURVIVAL.md 3.1 on the object): not drawn and out of the hit pass (word +8),
 * body hidden, far below the arena; the slot free (kind 2, index -1). Dark Tooth's head hidden too. */
void PORT_TEXT portCarParkSlot(int slot)
{
    struct TM2Object *obj;
    int type, key;

    if (!portCarSlotValid(slot))
        return;
    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    key = portCarKey(slot, type);
    csSetDrawEnable(obj, 0);
    carBodyObjectHide(key);
    obj->pos[2] = PORT_FAR_AWAY;
    if (type == 0xF0 && DARK_TOOTH_HEAD(key) != 0)
        DARK_TOOTH_PARK_HEAD(slot, key);
    carSlotKinds[slot] = 2;
    carSlotIndices[slot] = -1;
}

/* References to the cars in two slots held across frames, exchanged a <-> b (G.5: homing projectiles and
 * lock-ons follow the record to its new car), or with b < 0 cleared for a:
 *   by slot:   stats.killer / last_attacker of every record (kill credits), the taser arcs' shooter, the
 *              mines' and remote bombs' dropper;
 *   by object: projectile targets (a car object's position; the only lock-on that persists), the
 *              boomerang's return object, the energy shields' source car.
 * References by object id, record index or record address follow the record and need nothing; the slam
 * claws and crushes (by key) are released by portCarResetSpecials. */
void PORT_TEXT portCarRekeyReferences(int slot_a, int slot_b)
{
    struct PortCarRec r;
    struct TM2Object *oa, *ob;
    int n, i, nb;

    if (!portCarSlotValid(slot_a))
        return;
    oa = (struct TM2Object *)carSlotObjects[slot_a];
    ob = portCarSlotValid(slot_b) ? (struct TM2Object *)carSlotObjects[slot_b] : 0;
    nb = ob != 0 ? slot_b : -1;
    for (n = 0; portCarRecAt(n, &r); n++) {
        if (r.stats->killer == slot_a)
            r.stats->killer = nb;
        else if (ob != 0 && r.stats->killer == slot_b)
            r.stats->killer = slot_a;
        if (r.stats->last_attacker == slot_a)
            r.stats->last_attacker = nb;
        else if (ob != 0 && r.stats->last_attacker == slot_b)
            r.stats->last_attacker = slot_a;
    }
    for (i = 0; i < TM2_MAX_ARCS; i++) {
        if (ARC_OWNER(i) == slot_a)
            ARC_OWNER(i) = nb;
        else if (ob != 0 && ARC_OWNER(i) == slot_b)
            ARC_OWNER(i) = slot_a;
    }
    for (i = 0; i < PORT_DROPS; i++) {
        if (DROP_SLOT(i) == slot_a)
            DROP_SLOT(i) = nb;
        else if (ob != 0 && DROP_SLOT(i) == slot_b)
            DROP_SLOT(i) = slot_a;
    }
    for (i = 0; i < TM2_MAX_PROJECTILES; i++) {
        if (PROJ_TARGET(i) == oa->pos)
            PROJ_TARGET(i) = ob != 0 ? ob->pos : 0;
        else if (ob != 0 && PROJ_TARGET(i) == ob->pos)
            PROJ_TARGET(i) = oa->pos;
        if (ob != 0 && PROJ_VICTIM(i) == oa)
            PROJ_VICTIM(i) = ob;
        else if (ob != 0 && PROJ_VICTIM(i) == ob)
            PROJ_VICTIM(i) = oa;
    }
    if (ob != 0)
        for (i = 0; i < TM2_MAX_SHIELDS; i++) {
            if (SHIELD_SOURCE(i) == oa)
                SHIELD_SOURCE(i) = ob;
            else if (SHIELD_SOURCE(i) == ob)
                SHIELD_SOURCE(i) = oa;
        }
}

static void PORT_TEXT portCarBindSet(int kind, int index, int group)
{
    if (kind == 0 && (unsigned int)index < TM2_PORT_BIND_HUMANS)
        portCarBindGroup[index] = group;
    else if (kind == 1 && (unsigned int)index < TM2_PORT_BIND_ENTRIES - TM2_PORT_BIND_HUMANS)
        portCarBindGroup[TM2_PORT_BIND_HUMANS + index] = group;
}

static int PORT_TEXT portCarBindGet(int kind, int index)
{
    if (kind == 0 && (unsigned int)index < TM2_PORT_BIND_HUMANS)
        return portCarBindGroup[index];
    if (kind == 1 && (unsigned int)index < TM2_PORT_BIND_ENTRIES - TM2_PORT_BIND_HUMANS)
        return portCarBindGroup[TM2_PORT_BIND_HUMANS + index];
    return 0;
}

/* sessionBindCars: the group record (ai, index) binds to: its portCarBindGroup entry when that group has
 * a car in this load, else `group` (the settings' one). */
int PORT_TEXT portCarBindGroupOf(int ai, int index, int group)
{
    int g;

    g = portCarBindGet(ai != 0, index);
    if (g > 0 && portCarSlotOfGroup(g) >= 0)
        return g;
    return group;
}

/* sessionBindCars: the car type the record binds as (the binders check the group's slot holds it). */
int PORT_TEXT portCarBindTypeOf(int ai, int index, int type)
{
    int g, s;

    g = portCarBindGet(ai != 0, index);
    if (g > 0 && (s = portCarSlotOfGroup(g)) >= 0)
        return carSlotOwners[s];
    return type;
}

/* The claw record a record that becomes Mr. Slam on a car nobody drove can use: its own, else one an
 * idle record (not Mr. Slam) holds (moved over), else none (weaponBlockInit allocates one, as retail). */
static void PORT_TEXT portCarSlamRecord(struct PortCarRec *rec)
{
    struct PortCarRec r;
    int n;

    if (rec->weapons->slam != 0)
        return;
    for (n = 0; portCarRecAt(n, &r); n++)
        if (r.rec != rec->rec && *r.type != 0xC0 && r.weapons->slam != 0) {
            rec->weapons->slam = r.weapons->slam;
            r.weapons->slam = 0;
            return;
        }
}

/* Request 32 step 3: record `index` (human != 0: humanCars, else aiCars) drives the car of load group
 * `group` (human i = i + 1, opponent i = humans + i + 1, as sessionLoadArenaAndCars loads them).
 *   - the car may be driven by another record (two-way swap: that record takes this record's car, Q15),
 *     by a parked AI record (the Juggernaut's Minion: it stays parked on this record's car), or by nobody
 *     (this record's car is parked, portCarParkSlot);
 *   - specials of both cars end, their looks reset (portCarResetSpecials, portCarResetLook);
 *   - each record keeps its position, motion, route, pickups, energy, turbo and specials count (clamped);
 *     stats, wheels, box, camera and AI profile become the new type's (portCarApplyType); hp < 0 scales
 *     HP to the new maximum, hp >= 0 sets it (the Juggernaut's HP, Q14); the other record scales;
 *   - the cars exchange places (each takes the position, angles and matrix of the car its new driver had),
 *     each is shown as its driver is (portCarModelOn);
 *   - kill credits, homing targets and the other references follow the records (portCarRekeyReferences);
 *   - portCarBindGroup keeps both bindings through sessionBindCars (modes 3/4 rebind every round).
 * Call it between frames (the port's frame hook before the car updates), never from inside a car update.
 * Returns 1; 0 (nothing changed) if the record or the group does not exist or the record already drives
 * that group's car. */
int PORT_TEXT portCarSetGroup(int human, int index, int group, int hp)
{
    struct PortCarRec rec, other;
    struct TM2Object *oa, *ob;
    struct TM2CarSlam *slam;
    int kind, sa, sb, ga, has_other, vis_a, vis_b, i;
    int place[13];

    kind = human ? 0 : 1;
    if (!portCarRec(kind, index, &rec))
        return 0;
    sa = *rec.slot;
    sb = portCarSlotOfGroup(group);
    if (!portCarSlotValid(sa) || sb < 0 || sb == sa || carSlotKinds[sa] != kind || carSlotIndices[sa] != index)
        return 0;
    ga = portCarGroups.slot_group[sa];
    has_other = portCarDriver(sb, &other);
    oa = (struct TM2Object *)carSlotObjects[sa];
    ob = (struct TM2Object *)carSlotObjects[sb];
    vis_a = oa->visible;
    vis_b = ob->visible;

    /* 1. specials and looks of both cars, while the bindings are the old ones */
    portCarResetSpecials(sa);
    portCarResetSpecials(sb);
    portCarResetLook(sa);
    portCarResetLook(sb);

    /* 2. Mr. Slam's claw records go with the records' new types */
    if (has_other) {
        slam = rec.weapons->slam;
        rec.weapons->slam = other.weapons->slam;
        other.weapons->slam = slam;
    } else if (carSlotOwners[sb] == 0xC0) {
        portCarSlamRecord(&rec);
    }

    /* 3. the cars exchange places: matrix (+0x80), angles (+0xA0), position (+0xA8) */
    for (i = 0; i < 13; i++) {
        place[i] = ((int *)((unsigned char *)ob + 0x80))[i];
        ((int *)((unsigned char *)ob + 0x80))[i] = ((int *)((unsigned char *)oa + 0x80))[i];
        if (has_other)
            ((int *)((unsigned char *)oa + 0x80))[i] = place[i];
    }

    /* 4. bindings */
    portCarUnbind(kind, index);
    if (has_other)
        portCarUnbind(other.kind, other.index);
    portCarBindSlot(kind, index, sb);
    if (has_other)
        portCarBindSlot(other.kind, other.index, sa);
    else
        portCarParkSlot(sa);

    /* 5. types */
    portCarApplyType(kind, index, hp);
    if (has_other)
        portCarApplyType(other.kind, other.index, -1);

    /* 6. the cars shown as their drivers are */
    portCarModelOn(&rec, vis_a);
    if (has_other)
        portCarModelOn(&other, vis_b);

    /* 7. references, and the bindings for the next sessionBindCars */
    portCarRekeyReferences(sa, sb);
    portCarBindSet(kind, index, group);
    if (has_other)
        portCarBindSet(other.kind, other.index, ga);
    return 1;
}

/* SURVIVAL.md 3.1 for AI record `index`: off for good until the port starts it again (aiCarStart with the
 * slot's type, SURVIVAL.md 3.2): specials and look reset, dead then HP 0 (no explosion), wreck state 4 (no
 * update, no physics), out of range (engine sound and gun loop stop), its car not drawn and out of the hit
 * pass, body hidden, record and car far below the arena. The record stays bound to its car, so the car
 * can be given to another record with portCarSetGroup (the Juggernaut's Minion). Returns 1, 0 if there is
 * no such AI record. */
int PORT_TEXT portCarParkAi(int index)
{
    struct PortCarRec r;
    struct TM2Object *obj;
    int slot, type;

    if (!portCarRec(1, index, &r) || !portCarSlotValid(*r.slot))
        return 0;
    slot = *r.slot;
    portCarResetSpecials(slot);
    portCarResetLook(slot);
    r.state->dead = 1;
    r.stats->hp = 0;
    r.stats->wreck_state = 4;
    ((struct TM2AIRecord *)r.rec)->active = 0;
    r.motion->pos[2] = PORT_FAR_AWAY;
    obj = (struct TM2Object *)carSlotObjects[slot];
    type = carSlotOwners[slot];
    csSetDrawEnable(obj, 0);
    carBodyObjectHide(portCarKey(slot, type));
    obj->pos[2] = PORT_FAR_AWAY;
    if (type == 0xF0 && DARK_TOOTH_HEAD(portCarKey(slot, type)) != 0)
        DARK_TOOTH_PARK_HEAD(slot, portCarKey(slot, type));
    return 1;
}
#endif /* TM2_PORT_CAR_SWAP */

#ifdef TM2_PORT_TEX_SHARED
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- texture banks (PC port request 32 step 4, docs/TEXTURE_BANKS.md "As built") ----
 * A bank is a VRAM of its own in the port's GPU (1..127; 0 = the PS1's VRAM). With portCarOptions bit 5
 * latched at the load (bitmapInit), each car load group gets a bank on its first image
 * (vramPlaceTexture -> portTexPlaceBegin) and places all its images and CLUTs there with retail's scan
 * (vramPlaceTexture, vramUploadClut), testing and marking whole rectangles in the list below instead of
 * the VRAM bitmap. A bank is filled by one group in one go (its TMS parse), so one rectangle list (the
 * bank allocated last) is enough; a fresh bank starts with bitmapInit's reservations only (frame buffers,
 * fonts), so the coordinates stay in the areas retail uses for car textures. An image whose key the
 * arena's TMS holds keeps the arena's slot (bank 0), as retail: the car samples the arena's live copy.
 * The image bookkeeping (portTexBanksReset, portTexPlaceBegin, portTexFindReuse, portTexNoteSlot, portTexFadeId)
 * also serves request 33-1's CLUT copies (TM2_PORT_TEX_SHARED, car_cluts.h); without TM2_PORT_TEXTURE_BANKS no
 * bank is ever live. */
typedef char tm2_port_tex_groups_check[(TM2_PORT_TEX_GROUPS >= TM2_CAR_SLOT_CAP &&
                                        TM2_PORT_TEX_GROUPS > TM2_MAX_LOADED_CARS) ? 1 : -1];

extern int *bitmapRecords;          /* image records: key, tpage, x, y, CLUT x, CLUT y (bitmapAddRecord) */
extern int bitmapRecordCount;
extern int bitmapTrackingEnabled;
extern int *vramTexSlots;           /* 5 words per slot: tpage, x, y, CLUT x, CLUT y; group * stride + image */
extern int LoadImage(void *rect, void *data);
extern int DrawSync(int mode);

#define TB portTexBanks

/* bitmapInit (every load, before the level TMS): latch bit 5 for this load and free every bank. The
 * per-group no-reuse flags are the port's and stay. */
void PORT_TEXT portTexBanksReset(void)
{
    int i;

#ifdef TM2_PORT_TEXTURE_BANKS
    TB.live = (portCarOptions & TM2_PORT_CAR_OPT_TEXTURE_BANKS) != 0;
#else
    TB.live = 0;   /* request 33-1 without banks: only the bookkeeping */
#endif
    TB.level_records = -1;
    TB.cur_bank = 0;
    TB.rect_count = 0;
    for (i = 0; i < TM2_PORT_TEX_GROUPS; i++) {
        TB.group_bank[i] = 0;
        TB.body_clut[i] = 0;
        TB.body_bank[i] = 0;
        TB.rec_first[i] = -1;
        TB.rec_end[i] = -1;
    }
    for (i = 0; i < TM2_PORT_TEX_BANKS; i++)
        TB.bank_group[i] = 0;
}

#ifdef TM2_PORT_TEXTURE_BANKS
/* Is (x, y, w, h) free in `bank`? Only in the bank being filled (the one allocated last), inside the
 * 1024 x 512 VRAM, and only while the rectangle list has room. Unlike retail's bitmap test (top-left
 * texel only) the whole rectangle is tested. */
int PORT_TEXT portTexBankIsFree(int bank, int x, int y, int w, int h)
{
    int i;
    short *r;

    if (bank == 0 || bank != TB.cur_bank || TB.rect_count >= TM2_PORT_TEX_BANK_RECTS)
        return 0;
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > 1024 || y + h > 512)
        return 0;
    for (i = 0; i < TB.rect_count; i++) {
        r = TB.rects[i];
        if (x < r[0] + r[2] && r[0] < x + w && y < r[1] + r[3] && r[1] < y + h)
            return 0;
    }
    return 1;
}

/* Take (x, y, w, h) in the bank being filled. */
void PORT_TEXT portTexBankMark(int bank, int x, int y, int w, int h)
{
    short *r;

    if (bank == 0 || bank != TB.cur_bank || TB.rect_count >= TM2_PORT_TEX_BANK_RECTS)
        return;
    r = TB.rects[TB.rect_count++];
    r[0] = x;
    r[1] = y;
    r[2] = w;
    r[3] = h;
}

/* A fresh bank for load group `group` (1..63) while banks are live: the lowest free bank; it becomes the
 * bank being filled, with bitmapInit's reservations taken. The group's bank if it has one; 0 if banks are
 * not live or all 127 are taken (the group then uses bank 0 as retail). For a mid-match load: call it
 * (or let the group's first vramPlaceTexture call it) before the group's TMS is parsed. */
int PORT_TEXT portTexBankAlloc(int group)
{
    int b;

    if (!TB.live || (unsigned int)(group - 1) >= TM2_PORT_TEX_GROUPS - 1)
        return 0;
    if (TB.group_bank[group] != 0)
        return TB.group_bank[group];
    for (b = 1; b < TM2_PORT_TEX_BANKS; b++)
        if (TB.bank_group[b] == 0)
            break;
    if (b >= TM2_PORT_TEX_BANKS)
        return 0;
    TB.bank_group[b] = group;
    TB.group_bank[group] = b;
    TB.cur_bank = b;
    TB.rect_count = 0;
    portTexBankMark(b, 0, 0, 640, 256);
    portTexBankMark(b, 960, 256, 64, 32);
    portTexBankMark(b, 960, 288, 64, 128);
    portTexBankMark(b, 960, 416, 32, 96);
    return b;
}

/* Give load group `group`'s bank back (a car removed for good, before a mid-match load reuses the bank).
 * The group's primitives still carry the bank number: free it only when nothing draws that car any more
 * (and its invisibility fade has ended: portCarResetLook). */
void PORT_TEXT portTexBankFree(int group)
{
    int b;

    if ((unsigned int)group >= TM2_PORT_TEX_GROUPS || (b = TB.group_bank[group]) == 0)
        return;
    TB.bank_group[b] = 0;
    TB.group_bank[group] = 0;
    TB.body_clut[group] = 0;
    TB.body_bank[group] = 0;
    if (TB.cur_bank == b) {
        TB.cur_bank = 0;
        TB.rect_count = 0;
    }
}

/* The bank of load group `group`, 0 = bank 0 (VRAM). */
int PORT_TEXT portTexBankOfGroup(int group)
{
    if ((unsigned int)group >= TM2_PORT_TEX_GROUPS)
        return 0;
    return TB.group_bank[group];
}
#endif /* TM2_PORT_TEXTURE_BANKS */

/* vramPlaceTexture, at each image of group `group`: note the record bitmapAddRecord appends for it next
 * (the group's records are consecutive: its range, for the no-reuse flag), the level's record count at
 * the first car image of the load, and the group's bank (allocated at its first image while live). */
int PORT_TEXT portTexPlaceBegin(int group)
{
    if ((unsigned int)group >= TM2_PORT_TEX_GROUPS)
        return 0;
    if (TB.level_records < 0)
        TB.level_records = bitmapRecordCount;
    if (TB.rec_first[group] < 0)
        TB.rec_first[group] = bitmapRecordCount;
    TB.rec_end[group] = bitmapRecordCount + (bitmapTrackingEnabled != 0);
    if (!TB.live || group == 0)
        return 0;
    if (TB.group_bank[group] != 0)
        return TB.group_bank[group];
#ifdef TM2_PORT_TEXTURE_BANKS
    return portTexBankAlloc(group);
#else
    return 0;
#endif
}

/* The image record group `group` takes image `key` from instead of placing it, 0 = place it.
 * Retail (bitmapFindRecord): the first record with the key. Here, in that order:
 *   - a group with portCarGroupNoReuse set takes nothing;
 *   - a banked group (bank != 0) looks only at the level's records (an arena image keeps the arena's
 *     coordinates in bank 0, TEXTURE_BANKS.md 1.4); another car's image is placed again in its own bank;
 *   - records of a group with portCarGroupNoReuse set are never given to another group.
 * With no flag set and bank 0 this is bitmapFindRecord. */
int *PORT_TEXT portTexFindReuse(int key, int group, int bank)
{
    int i, h, n;
    int *rec;

    if ((unsigned int)group < TM2_PORT_TEX_GROUPS && portCarGroupNoReuse[group])
        return 0;
    n = bitmapRecordCount;
    if (bank != 0 && TB.level_records >= 0 && TB.level_records < n)
        n = TB.level_records;
    for (i = 0; i < n; i++) {
        rec = &bitmapRecords[i * 6];
        if (rec[0] != key)
            continue;
        for (h = 1; h < TM2_PORT_TEX_GROUPS; h++)
            if (portCarGroupNoReuse[h] && TB.rec_first[h] >= 0 && i >= TB.rec_first[h] && i < TB.rec_end[h])
                break;
        if (h < TM2_PORT_TEX_GROUPS)
            continue;
        return rec;
    }
    return 0;
}

/* vramPlaceTexture, after writing slot (group, image): for image 0 (the body, TEAM_COLORS.md 1.1) publish
 * its CLUT id and bank, so that the port can repaint each car's own body CLUT after the load (the slots
 * die with the first frame). */
void PORT_TEXT portTexNoteSlot(int group, int image)
{
    int *s;

    if (image != 0 || (unsigned int)group >= TM2_PORT_TEX_GROUPS)
        return;
    s = &vramTexSlots[(group * TM2_TEX_SLOT_STRIDE) * 5];
    TB.body_clut[group] = ((s[3] >> 4) & 0x3F) | ((s[4] & 0x3FF) << 6);
    TB.body_bank[group] = (s[0] >> 9) & 0x7F;
}

/* The CLUT fade id of the car in `slot` (type `type`): retail's type + 8; while banks are live the car's
 * key + 8 (clutAnimRegister registers each car's +8 fade so), = type + 8 for the first car of a type. */
int PORT_TEXT portTexFadeId(int slot, int type)
{
#ifdef TM2_PORT_CAR_CLUTS
    if (!TB.live && !portCarCluts.live)   /* request 33-1: per key while the CLUT copies are live too */
#else
    if (!TB.live)
#endif
        return type + 8;
    return portCarKey(slot, type) + 8;
}

#ifdef TM2_PORT_TEXTURE_BANKS
/* For the port's own uploads into a car's bank (team paint: TEXTURE_BANKS.md "As built"): LoadImage
 * with the bank announced, as the game's bank-aware uploads do. */
void PORT_TEXT portTexLoadImage(int bank, void *rect, void *data)
{
    portVramUploadBank = bank;
    LoadImage(rect, data);
    DrawSync(0);
    portVramUploadBank = 0;
}
#endif /* TM2_PORT_TEXTURE_BANKS */
#undef TB
#endif /* TM2_PORT_TEX_SHARED */

#ifdef TM2_PORT_CLUT_ROWS
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- per-car CLUTs and VRAM rows 240..255 (PC port request 33 step 1, docs/CAR_GROUPS.md 13) ----
 * Rows 240..255 of x 0..639 lie under the 320x240 frame buffers (nothing draws there: the port's answer
 * PER_CAR_PLAN.md G.1); bitmapInit keeps them reserved in the usage bitmap, so retail's scans never take them,
 * and this allocator owns them for two users:
 *   - request 33-1 (TM2_PORT_CAR_CLUTS, portCarOptions bit 7): the CLUT copies of portCarClutsCopyAll;
 *   - request 33-2e (TM2_PORT_DARK_TOOTH, bit 6): a CLUT upload that finds no room in the upper half
 *     (vramUploadClut -> portClutRowsUpload), before the lower half.
 * 256-colour spots are x 0 and 256 (32), taken from row 240 down; 16-colour spots are taken at x 512..639
 * first (128, no 256-colour spot there), then from row 255 up, x 496 down, so the two kinds meet in the
 * middle. */
extern int *vramTexSlots;           /* 5 words per slot: tpage, x, y, CLUT x, CLUT y; group * stride + image */
extern int LoadImage(void *rect, void *data);
extern int DrawSync(int mode);

#define CC portCarCluts

/* bitmapInit (every load, after portTexBanksReset): latch bits 7, 9 and 6 for this load, clear the tables. Bit 7
 * is ignored while texture banks are live (every car has its own bank then); bit 9 is not (a bank's scan tests
 * whole rectangles anyway, and bank 0 keeps the arena only). */
void PORT_TEXT portCarClutsReset(void)
{
    int g, i;

    CC.live = 0;
    CC.rows_live = 0;
#ifdef TM2_PORT_CAR_CLUTS
#ifdef TM2_PORT_TEXTURE_BANKS
    if ((portCarOptions & TM2_PORT_CAR_OPT_CAR_CLUTS) != 0 && !portTexBanks.live) {
#else
    if ((portCarOptions & TM2_PORT_CAR_OPT_CAR_CLUTS) != 0) {
#endif
        CC.live = 1;
        CC.rows_live |= TM2_PORT_CLUT_ROWS_COPIES;
    }
    if ((portCarOptions & TM2_PORT_CAR_OPT_CLUT_SCAN) != 0)
        CC.rows_live |= TM2_PORT_CLUT_ROWS_SCAN;    /* request 33-1 follow-up: the whole-spot CLUT scan */
#endif
#ifdef TM2_PORT_DARK_TOOTH
    /* bit 6, and request 42's bit 10 (an ordinary Dark Tooth needs the room as much) */
    if (TM2_PORT_BOSS_ANYWHERE || (portCarOptions & TM2_PORT_CAR_OPT_DT_ORDINARY) != 0)
        CC.rows_live |= TM2_PORT_CLUT_ROWS_BOSS;
#endif
    CC.copies = 0;
    CC.failed = 0;
    for (i = 0; i < TM2_PORT_CLUT_ROWS_N; i++) {
        CC.rows[i][0] = 0;
        CC.rows[i][1] = 0;
    }
    for (g = 0; g < TM2_PORT_CLUT_GROUPS; g++) {
        CC.pend16[g] = 0;
        CC.pend256[g] = 0;
        CC.arena[g] = 0;
        CC.shared[g] = 0;
        for (i = 0; i < TM2_PORT_CLUT_IMAGES; i++) {
            CC.clut[g][i] = 0;
            CC.state[g][i] = TM2_PORT_CLUT_NONE;
        }
    }
}

/* A w-texel CLUT spot (w 16 or 256, one row) in rows 240..255: *x, *y and 1, or 0 when there is none. */
int PORT_TEXT portClutRowsAlloc(int w, int *x, int *y)
{
    int r, u, half;
    unsigned int m;

    if (w == 256) {
        for (half = 0; half < 2; half++) {
            m = 0xFFFFu << (half * 16);
            for (r = 0; r < TM2_PORT_CLUT_ROWS_N; r++)
                if ((CC.rows[r][0] & m) == 0) {
                    CC.rows[r][0] |= m;
                    *x = half * 256;
                    *y = TM2_PORT_CLUT_ROW_FIRST + r;
                    return 1;
                }
        }
        return 0;
    }
    if (w != 16)
        return 0;
    for (r = 0; r < TM2_PORT_CLUT_ROWS_N; r++)
        for (u = 32; u < TM2_PORT_CLUT_ROW_UNITS; u++)
            if ((CC.rows[r][1] & (1u << (u - 32))) == 0) {
                CC.rows[r][1] |= 1u << (u - 32);
                *x = u * 16;
                *y = TM2_PORT_CLUT_ROW_FIRST + r;
                return 1;
            }
    for (r = TM2_PORT_CLUT_ROWS_N - 1; r >= 0; r--)
        for (u = 31; u >= 0; u--)
            if ((CC.rows[r][0] & (1u << u)) == 0) {
                CC.rows[r][0] |= 1u << u;
                *x = u * 16;
                *y = TM2_PORT_CLUT_ROW_FIRST + r;
                return 1;
            }
    return 0;
}

#ifdef TM2_PORT_DARK_TOOTH
/* vramUploadClut, when the upper half had no room: with bit 6 latched, a bank-0 CLUT (one row of 16 or 256)
 * goes to rows 240..255 (the room bitmapInit used to free for Dark Tooth, SURVIVAL.md 4.1): slot `slot`'s CLUT
 * x / y, then the upload. 0 = not taken (the scan goes on to the lower half, as retail). */
int PORT_TEXT portClutRowsUpload(int w, int h, void *data, int slot, int bank)
{
    short r[4];
    int x, y;

    if ((CC.rows_live & TM2_PORT_CLUT_ROWS_BOSS) == 0 || bank != 0 || h != 1 || !portClutRowsAlloc(w, &x, &y))
        return 0;
    vramTexSlots[slot * 5 + 3] = x;
    vramTexSlots[slot * 5 + 4] = y;
    r[0] = x;
    r[1] = y;
    r[2] = w;
    r[3] = 1;
    LoadImage(r, data);
    DrawSync(0);
    return 1;
}
#endif

#ifdef TM2_PORT_CAR_CLUTS
extern int *bitmapRecords;          /* image records: key, tpage, x, y, CLUT x, CLUT y (bitmapAddRecord) */
extern int bitmapIsFreeXY(int x, int y);
extern void bitmapMarkRect(int x, int y, int w, int h);
extern void gpuStoreImageLine(void *p, int x, int y, int w);
extern void gpuLoadImageLine(void *p, int x, int y, int w);

/* vramPlaceTexture, after writing slot (group, image) (known = the record it took the image from, 0 = placed
 * here): note the image's CLUT; one taken from another car or from the arena is to be copied (the CLUT of an
 * earlier image of the same TMS is the group's own). */
void PORT_TEXT portCarClutNote(int group, int image, int mode, int *known)
{
    int *s;
    int rec;
    unsigned int bit;

    if (!CC.live || (unsigned int)group >= TM2_PORT_CLUT_GROUPS || (unsigned int)image >= TM2_PORT_CLUT_IMAGES)
        return;
    bit = 1u << image;
    CC.pend16[group] &= ~bit;
    CC.pend256[group] &= ~bit;
    CC.arena[group] &= ~bit;
    CC.clut[group][image] = 0;
    CC.state[group][image] = TM2_PORT_CLUT_NONE;
    if (((mode >> 3) & 1) == 0 || (mode & 7) > 1)
        return;                      /* no CLUT (vramPlaceTexture uploads them for 4- and 8-bit images only) */
    s = &vramTexSlots[(group * TM2_TEX_SLOT_STRIDE + image) * 5];
    CC.clut[group][image] = ((s[3] >> 4) & 0x3F) | ((s[4] & 0x3FF) << 6);
    CC.state[group][image] = TM2_PORT_CLUT_OWN;
    if (known == 0)
        return;
    rec = (known - bitmapRecords) / 6;
    if (portTexBanks.rec_first[group] >= 0 && rec >= portTexBanks.rec_first[group])
        return;
    CC.state[group][image] = TM2_PORT_CLUT_SHARED;
    if (portTexBanks.level_records >= 0 && rec < portTexBanks.level_records)
        CC.arena[group] |= bit;
    if ((mode & 7) == 0)
        CC.pend16[group] |= bit;
    else
        CC.pend256[group] |= bit;
}

/* vramUploadClut's spot test (request 33-1 follow-up, CAR_GROUPS.md 13.12): with bit 9 latched every texel of the
 * w x 1 spot (16 or 256) must be free in the usage bitmap, so an original CLUT never lands on texels already
 * taken (retail tests only the top-left one: RETAIL_BUGS.md 27); bit 9 clear: retail's test. */
int PORT_TEXT portClutSpotFree(int x, int y, int w)
{
    int i;

    if ((CC.rows_live & TM2_PORT_CLUT_ROWS_SCAN) == 0)
        return bitmapIsFreeXY(x, y);
    if (x + w > 1024)
        return 0;
    for (i = 0; i < w; i++)
        if (!bitmapIsFreeXY(x + i, y))
            return 0;
    return 1;
}

/* vramUploadClut with bit 9 latched, when neither half has a whole free spot: a bank-0 CLUT (one row of 16 or 256)
 * takes a spot in rows 240..255 from the allocator (before bit 7's copies, which come after the parse; with bit 6
 * the same rows were already tried between the halves): slot `slot`'s CLUT x / y, then the upload. 0 = none there
 * either: retail's failure (vramUploadClut returns 0, vramPlaceTexture prints "Relocatable texture not loaded",
 * the slot keeps the CLUT x / y it had). */
int PORT_TEXT portClutRowsLast(int w, int h, void *data, int slot, int bank)
{
    short r[4];
    int x, y;

    if ((CC.rows_live & TM2_PORT_CLUT_ROWS_SCAN) == 0 || bank != 0 || h != 1 || !portClutRowsAlloc(w, &x, &y))
        return 0;
    vramTexSlots[slot * 5 + 3] = x;
    vramTexSlots[slot * 5 + 4] = y;
    r[0] = x;
    r[1] = y;
    r[2] = w;
    r[3] = 1;
    LoadImage(r, data);
    DrawSync(0);
    return 1;
}

/* vramPlaceTexture's passes (request 41 item B, CAR_GROUPS.md 13.12): retail's 2; with bit 9 latched 4, two
 * passes testing whole rectangles (portImageSpotFree), then retail's two as the fallback. */
int PORT_TEXT portImagePasses(void)
{
    return (CC.rows_live & TM2_PORT_CLUT_ROWS_SCAN) != 0 ? 4 : 2;
}

/* vramPlaceTexture's spot test for an image of w x h (VRAM units) at (x, y) in pass 1..4: with bit 9 latched and
 * pass 1 or 2, every texel of the rectangle must be free in the usage bitmap and inside VRAM (retail tests the
 * top-left one, RETAIL_BUGS.md 27: an image can be placed over pixels or CLUTs already there); else (bit 9 clear,
 * or the fallback passes 3 and 4) retail's test. */
int PORT_TEXT portImageSpotFree(int x, int y, int w, int h, int pass)
{
    int i, j;

    if ((CC.rows_live & TM2_PORT_CLUT_ROWS_SCAN) == 0 || pass > 2)
        return bitmapIsFreeXY(x, y);
    if (x + w > 1024 || y + h > 512)
        return 0;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (!bitmapIsFreeXY(x + i, y + j))
                return 0;
    return 1;
}

/* Does slot (group, image) belong to the next group? With retail's stride 20 an image 20..31 shares its slot
 * with image 0..11 of the next group, which overwrote it (SIXTEEN_CARS.md 4.5): nothing of it to copy. */
static int PORT_TEXT portCarClutAliased(int group, int image)
{
    return image >= TM2_TEX_SLOT_STRIDE && group + 1 < TM2_PORT_CLUT_GROUPS &&
           portTexBanks.rec_first[group + 1] >= 0;
}

/* A free w x 1 spot in the usage bitmap, in retail's CLUT scan order (vramUploadClut), testing every texel
 * (retail tests the top-left one), so a copy never lands on anything placed. Only during a load (the bitmap
 * lives in packet buffer 0 until the first frame). */
static int PORT_TEXT portCarClutScan(int w, int *xp, int *yp)
{
    int px, py, x, y, i;

    for (py = 0; py < 512; py += 256)
        for (px = 0; px < 1024; px += 256)
            for (y = py; y < py + 256; y++)
                for (x = px; x < px + 256; x += w) {
                    for (i = 0; i < w && bitmapIsFreeXY(x + i, y); i++)
                        ;
                    if (i == w) {
                        bitmapMarkRect(x, y, w, 1);
                        *xp = x;
                        *yp = y;
                        return 1;
                    }
                }
    return 0;
}

/* Copy the w-colour CLUT of slot (group, image) to a spot of its own: rows 240..255, else (scan) the usage
 * bitmap. The copy is read back from VRAM, so it is the row as the arena or the first car uploaded it. full[]
 * remembers a width with no room left (nothing is freed during a load). */
static int PORT_TEXT portCarClutCopyOne(int group, int image, int w, int scan, int *full)
{
    unsigned short line[256];
    int *s;
    int x, y;

    s = &vramTexSlots[(group * TM2_TEX_SLOT_STRIDE + image) * 5];
    if (full[w == 256] || (!portClutRowsAlloc(w, &x, &y) && !(scan && portCarClutScan(w, &x, &y)))) {
        full[w == 256] = 1;
        CC.failed++;
        return 0;
    }
    gpuStoreImageLine(line, s[3], s[4], w);
    gpuLoadImageLine(line, x, y, w);
    s[3] = x;
    s[4] = y;
    CC.clut[group][image] = ((x >> 4) & 0x3F) | ((y & 0x3FF) << 6);
    CC.state[group][image] = TM2_PORT_CLUT_COPY;
#ifdef TM2_PORT_CAR_DROPIN
    /* request 33 unloading (CAR_GROUPS.md 14.7): the copy's width, to give its spot back */
    if (w == 256)
        portCarUnload.clut256[group] |= 1u << image;
    else
        portCarUnload.clut256[group] &= ~(1u << image);
#endif
    CC.copies++;
    return 1;
}

/* One phase of the copy pass for one group: 0 the body (image 0), 1 the other 256-colour CLUTs taken from a
 * car, 2 the 16-colour ones taken from a car, 3 the ones taken from the arena. */
static int PORT_TEXT portCarClutCopyPhase(int group, int phase, int scan, int *full)
{
    unsigned int bit;
    int i, w, want, n;

    n = 0;
    for (i = 0; i < TM2_PORT_CLUT_IMAGES; i++) {
        bit = 1u << i;
        if (CC.pend256[group] & bit)
            w = 256;
        else if (CC.pend16[group] & bit)
            w = 16;
        else
            continue;
        if (CC.arena[group] & bit)
            want = 3;
        else if (i == 0)
            want = 0;
        else if (w == 256)
            want = 1;
        else
            want = 2;
        if (want != phase)
            continue;
        CC.pend16[group] &= ~bit;
        CC.pend256[group] &= ~bit;
        if (portCarClutAliased(group, i)) {
            CC.clut[group][i] = 0;
            CC.state[group][i] = TM2_PORT_CLUT_NONE;
            continue;
        }
        n += portCarClutCopyOne(group, i, w, scan, full);
    }
    return n;
}

/* After the copies: each loaded group's body CLUT into portTexBanks.body_clut (bank 0), and the sharing flags
 * (portCarCluts.shared): a CLUT left shared flags its group and every other group on the same row. */
static void PORT_TEXT portCarClutsPublish(void)
{
    int g, h, i, j;
    unsigned short id;

    for (g = 1; g < TM2_PORT_CLUT_GROUPS; g++) {
        CC.shared[g] = 0;
        if (portTexBanks.rec_first[g] < 0)
            continue;
        for (i = TM2_TEX_SLOT_STRIDE; i < TM2_PORT_CLUT_IMAGES; i++)
            if (portCarClutAliased(g, i)) {
                CC.clut[g][i] = 0;
                CC.state[g][i] = TM2_PORT_CLUT_NONE;
            }
        if (CC.state[g][0] != TM2_PORT_CLUT_NONE) {
            portTexBanks.body_clut[g] = CC.clut[g][0];
            portTexBanks.body_bank[g] = 0;
        }
    }
    for (g = 1; g < TM2_PORT_CLUT_GROUPS; g++) {
        if (portTexBanks.rec_first[g] < 0)
            continue;
        for (i = 0; i < TM2_PORT_CLUT_IMAGES; i++) {
            if (CC.state[g][i] != TM2_PORT_CLUT_SHARED)
                continue;
            if (CC.arena[g] & (1u << i)) {
                CC.shared[g] |= TM2_PORT_CLUT_SHARED_ARENA;
                continue;
            }
            id = CC.clut[g][i];
            CC.shared[g] |= i == 0 ? TM2_PORT_CLUT_SHARED_BODY : TM2_PORT_CLUT_SHARED_PART;
            for (h = 1; h < TM2_PORT_CLUT_GROUPS; h++) {
                if (h == g || portTexBanks.rec_first[h] < 0)
                    continue;
                for (j = 0; j < TM2_PORT_CLUT_IMAGES; j++)
                    if (CC.state[h][j] != TM2_PORT_CLUT_NONE && CC.clut[h][j] == id)
                        CC.shared[h] |= j == 0 ? TM2_PORT_CLUT_SHARED_BODY : TM2_PORT_CLUT_SHARED_PART;
            }
        }
    }
}

/* sessionLoadArenaAndCars, after the last car TMS (every original placed, no DMD remapped yet): with bit 7 the
 * copy pass over every group, phase by phase (with too little room the bodies, which team paint needs, come
 * first), then the table. */
void PORT_TEXT portCarClutsCopyAll(void)
{
    int full[2];
    int phase, g;

    if (!CC.live)
        return;
    full[0] = 0;
    full[1] = 0;
    for (phase = 0; phase < 4; phase++)
        for (g = 1; g < TM2_PORT_CLUT_GROUPS; g++)
            portCarClutCopyPhase(g, phase, 1, full);
    portCarClutsPublish();
}

/* The port's API for one group parsed later (a mid-match load, after its bitmapParseContainer and before its
 * assetInstallImage): its copies from rows 240..255 only (the usage bitmap is gone after the load), then the
 * table. Returns the copies made. */
int PORT_TEXT portCarClutsCopyGroup(int group)
{
    int full[2];
    int phase, n;

    if (!CC.live || (unsigned int)(group - 1) >= TM2_PORT_CLUT_GROUPS - 1)
        return 0;
    full[0] = 0;
    full[1] = 0;
    n = 0;
    for (phase = 0; phase < 4; phase++)
        n += portCarClutCopyPhase(group, phase, 0, full);
    portCarClutsPublish();
    return n;
}
#endif /* TM2_PORT_CAR_CLUTS */
#undef CC
#endif /* TM2_PORT_CLUT_ROWS */

#if defined(TM2_PORT_CAR_DROPIN) || defined(TM2_HUMAN_HANDOVER)
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* (Request 50: the declarations and the blocks up to portDropInRefs, and portDropInSlotSound, are shared with the
 * hand-over of port_humans.h, built with TM2_PORT_HUMANS and TM2_PORT_CAR_SWAP; the rest is the drop-in's.)
 * ---- online with bots, players dropping in (PC port request 33, docs/CAR_GROUPS.md 14, PER_CAR_PLAN.md B.6 /
 * B.7 with the port's answers G.11-G.15) ----
 * Remove a record and its car for good, start a record mid-match as a respawn, load a car mid-match. Built on
 * the swap's blocks (portCarResetSpecials, portCarResetLook, portCarBindSlot, portCarParkSlot,
 * portCarRekeyReferences, portCarModelOn). Nothing here runs unless the port calls it; the two game hooks
 * (portCarBindSkip / portCarBindCount in sessionBindCars) return the retail values while portCarDropIn.live is 0. */
#define DI portCarDropIn
#define DROPIN_VACANT_SLOT (TM2_MAX_CAR_SLOTS - 1)   /* the slot every vacant AI record points at (dummy object) */
#define HREC_AT(i) ((struct TM2HumanRecord *)humanCars[i])
#define AREC_AT(j) ((struct TM2AIRecord *)aiCars[j])
#define DROPIN_OBJ(p) ((struct TM2Object *)(p))

extern unsigned char gameAttractDemoMode;
extern int viewWatchedAiCar;
extern unsigned char carResetWeapons[];
extern unsigned char bulletRecords[];          /* 40 x 60 bytes: owner id +0x24, life +0x28 (weapon_bullets.c) */
extern unsigned char fxSprites[];              /* TM2_MAX_FX_SPRITES x 48 bytes: follow flag +0x01, loops +0x0C, owner_pos +0x2C */
extern void carSetResetWeapons(signed char value, int index);
extern void PadSetConfig(int config, short player);
extern void hudResetSpeed(short player);
extern void carBindHumanSlot(int index, int slot, int type);
extern void carBindAiSlot(int index, int slot, int type);
extern void carSetViewMode(void *human, int mode);
extern void humanCarPlaceAtStart(struct TM2HumanRecord *human);
extern void carShakeStart(void *car, int level, int axes, int time, unsigned char side);
extern void aiClearControls(struct TM2CarInput *input);
extern int PadGetConfig(short player);
extern void carApplyPadConfig(void *car, int config);
extern void fxRecordFree(short index, int unused);
extern void bulletFree(int index);
extern int soundStopInstance(int id, int instance);

#if TM2_MAX_RICOCHETS <= TM2_RETAIL_RICOCHETS
extern int weaponRicochetPool[TM2_MAX_RICOCHETS * (4 + 48) / 4];   /* asset_pools.c: objects[N], shots[N] */
#endif
#define DROPIN_BULLETS 40
#define DROPIN_SPRITES TM2_MAX_FX_SPRITES
#define PROJ_LIFE(p) (*(short *)((unsigned char *)fxRecords[p] + 0x04))
#define PROJ_TYPE(p) (*(short *)((unsigned char *)fxRecords[p] + 0x02))
#define PROJ_OWNER(p) (*(short *)((unsigned char *)fxRecords[p] + 0x42))
#define ARC_ID(a) (((short *)weaponArcs[a])[0])                       /* taser: shooter's id; lightning: index */
#define ARC_COUNT(a) (((short *)weaponArcs[a])[2])                    /* frames left, <= 0 free */
#define ARC_TARGET(a) (((short *)weaponArcs[a])[4])                   /* the victim's object id */
#define ARC_ORIGIN(a) (*(int **)&weaponArcs[a][3])                    /* &record motion.pos of the shooter */
#define ARC_ATTACH(a) (*(int **)&weaponArcs[a][4])                    /* &record motion.pos of the victim */
#define DROP_KIND(d) (weaponDrops[28 * (d)])
#define DROP_HELD(d) (weaponDrops[28 * (d) + 1])
#define DROP_OWNER(d) (*(short *)(weaponDrops + 28 * (d) + 4))         /* the dropper's object id */
#define DROP_STATUS(d) (*(short *)(weaponDrops + 28 * (d) + 8))
#define DROP_OBJ(d) (*(struct TM2Object **)(weaponDrops + 28 * (d) + 0x18))
#define SHIELD_REC(s) ((unsigned char *)weaponShieldPool + 4 * TM2_MAX_SHIELDS + 12 * (s))
#define SHIELD_TIMER(s) (*(short *)SHIELD_REC(s))
#define SHIELD_ID(s) (*(short *)(SHIELD_REC(s) + 2))                    /* the human (player index) */
#define SHIELD_DST(s) (*(struct TM2Object **)(SHIELD_REC(s) + 8))
#define RICO_SHOT(k) ((unsigned char *)weaponRicochetPool + 4 * TM2_MAX_RICOCHETS + 48 * (k))
#define RICO_OWNER(k) (*(short *)(RICO_SHOT(k) + 2))
#define RICO_LIFE(k) (*(short *)(RICO_SHOT(k) + 4))
#define RICO_OBJ(k) (*(struct TM2Object **)(RICO_SHOT(k) + 0x2C))
#define BULLET_OWNER(b) (*(int *)(bulletRecords + 60 * (b) + 0x24))
#define BULLET_LIFE(b) (*(int *)(bulletRecords + 60 * (b) + 0x28))
#define SPRITE_FOLLOW(s) (fxSprites[48 * (s) + 1])
#define SPRITE_LOOPS(s) (*(short *)(fxSprites + 48 * (s) + 0x0C))
#define SPRITE_OWNER(s) (*(unsigned char **)(fxSprites + 48 * (s) + 0x2C))
/* homingLeapState (weapon_leap_homing_init.cpp, 0x3C bytes): pointers into the leaper's and the target's records */
#define LEAP_W(o) (*(unsigned char **)((unsigned char *)&homingLeapState + (o)))
#define LEAP_TARGET_ID (*(unsigned short *)((unsigned char *)&homingLeapState + 0x10))
#define LEAP_FLAGS 0x04
#define LEAP_HEALTH 0x18
#define LEAP_POSITION 0x2C
#define LEAP_ROTATION 0x30
#define LEAP_TARGET 0x34
extern short aiDarkToothSwoopTarget;                       /* the human index a swoop goes at */
extern int *darkToothSwoopTarget;                         /* that human record's motion.pos */

/* The sound instances a car slot may have running (instance slot + 1): its engine and flame (portCarSoundStopSlot),
 * Twister's tornado, a human's turbo / gun / burn loops of players 2.., Dark Tooth's swoops (61, 66), and with
 * TM2_PORT_HUMANS a remote human's shared gun loop. portCarRemove, portCarUnloadGroup and request 50's hand-over;
 * nothing runs while the slot has none. */
extern int soundStopSharedLoop(int, int);
static void PORT_TEXT portDropInSlotSound(int s)
{
    unsigned char loops;

    if ((unsigned int)s >= TM2_PORT_SOUND_SLOTS)
        return;
    loops = portCarSoundLoops[s];
#ifdef TM2_PORT_CAR_SOUND
    portCarSoundStopSlot(s);
    if (loops & TM2_PORT_LOOP_TORNADO)
        soundStopInstance(0x35, s + 1);
    if (loops & TM2_PORT_LOOP_TURBO)
        soundStopInstance(0, s + 1);
    if (loops & TM2_PORT_LOOP_GUN)
        soundStopInstance(9, s + 1);
    if (loops & TM2_PORT_LOOP_BURN)
        soundStopInstance(32, s + 1);
    if (portCarSound.live) {
        soundStopInstance(61, s + 1);
        soundStopInstance(66, s + 1);
    }
    portCarSound.allow[s] = 0;
#endif
#ifdef TM2_PORT_HUMANS
    if (loops & TM2_PORT_LOOP_SHARED_GUN)
        soundStopSharedLoop(9, TM2_PORT_SHARED_GUN(s));
#endif
    portCarSoundLoops[s] = 0;
}

/* The object id of a record (human i: TM2_HUMAN_ID(i), AI j: j + 50; carBindHumanSlot / carBindAiSlot). */
static int PORT_TEXT portDropInId(int kind, int index)
{
    return kind == 0 ? TM2_HUMAN_ID(index) : index + 50;
}

/* A pointer p inside [lo, lo + n): moved by delta (delta != 0) or the test only. */
static int PORT_TEXT portDropInIn(void *p, void *lo, int n)
{
    return (unsigned char *)p >= (unsigned char *)lo && (unsigned char *)p < (unsigned char *)lo + n;
}

/* Every special state that holds a record by address or by object id, run over the first car's (retail) state
 * and, with TM2_PORT_CAR_SPECIALS, each duplicate key's (selected): with `delta` != 0 the pointers into
 * [rec, rec + size) move by delta and the object id `id` becomes `id2` (portHumanCompact); with delta 0 every
 * state aimed at the record (the leap's target, a Dark Tooth swoop at a human) ends (portCarRemove). The states
 * the record itself drives (its own leap, tornado, strike) are ended before by portCarResetSpecials. */
static void PORT_TEXT portDropInSpecialsOne(unsigned char *rec, int size, int delta, int id, int id2, int human)
{
    int o, k;

    if (delta != 0) {
        if (portDropInIn(weaponTornadoState.anchor, rec, size))
            weaponTornadoState.anchor = (int *)((unsigned char *)weaponTornadoState.anchor + delta);
        if (portDropInIn(weaponTornadoState.flags, rec, size))
            weaponTornadoState.flags = (struct TM2CarState *)((unsigned char *)weaponTornadoState.flags + delta);
        if (portDropInIn(weaponStrikeState.weapon, rec, size))
            weaponStrikeState.weapon = (struct TM2CarWeapons *)((unsigned char *)weaponStrikeState.weapon + delta);
        if (weaponStrikeState.owner_id == id)
            weaponStrikeState.owner_id = id2;
        for (k = 0; k < 5; k++) {
            o = k == 0 ? LEAP_FLAGS : k == 1 ? LEAP_HEALTH : k == 2 ? LEAP_POSITION : k == 3 ? LEAP_ROTATION
                                                                                                  : LEAP_TARGET;
            if (portDropInIn(LEAP_W(o), rec, size))
                LEAP_W(o) += delta;
        }
        if (LEAP_TARGET_ID == id)
            LEAP_TARGET_ID = id2;
    } else if (homingLeapState.active != 0
               && (LEAP_TARGET_ID == id || portDropInIn(LEAP_W(LEAP_TARGET), rec, size))) {
        /* a leap at the car that leaves ends as weaponHomingLeapUpdate ends one (its turbo flame off) */
        homingLeapState.active = 0;
        if ((unsigned int)homingLeapState.owner_id < TM2_CAR_SLOT_CAP)
            portCarTurboSetStep(portCarKey(homingLeapState.owner_id, carSlotOwners[homingLeapState.owner_id]), 0);
    }
    if (human >= 0 && aiDarkToothSwoopTarget == human) {
        if (delta != 0) {
            aiDarkToothSwoopTarget = (short)(human + delta / 0x814);
            if (portDropInIn(darkToothSwoopTarget, rec, size))
                darkToothSwoopTarget = (int *)((unsigned char *)darkToothSwoopTarget + delta);
        } else {
            darkToothSwooping = 0;
        }
    }
}

static void PORT_TEXT portDropInSpecials(unsigned char *rec, int size, int delta, int id, int id2, int human)
{
#ifdef TM2_PORT_CAR_SPECIALS
    int k, t, d, tok;
#endif

    portDropInSpecialsOne(rec, size, delta, id, id2, human);
#ifdef TM2_PORT_CAR_SPECIALS
    for (k = 0; k < 3; k++)
        for (t = k == 0 ? 0x60 : k == 1 ? 0xA0 : 0xD0, d = 0; (tok = portCarSpecialNext(t, &d)) != 0;) {
            portDropInSpecialsOne(rec, size, delta, id, id2, -1);
            portCarSpecialRestore(tok);
        }
#ifdef TM2_PORT_SPECIAL_AS
    for (d = 0; (tok = portCarSpecialAsNext(0xD0, &d)) != 0;) {     /* request 35: the borrowed strikes */
        portDropInSpecialsOne(rec, size, delta, id, id2, -1);
        portCarSpecialRestore(tok);
    }
#endif
#ifdef TM2_PORT_DARK_TOOTH
    for (d = 0; (tok = portCarSpecialNext(0xF0, &d)) != 0;) {
        if (human >= 0 && aiDarkToothSwoopTarget == human) {
            if (delta != 0) {
                aiDarkToothSwoopTarget = (short)(human + delta / 0x814);
                if (portDropInIn(darkToothSwoopTarget, rec, size))
                    darkToothSwoopTarget = (int *)((unsigned char *)darkToothSwoopTarget + delta);
            } else {
                darkToothSwooping = 0;
            }
        }
        portCarSpecialRestore(tok);
    }
#endif
#endif
}

/* G.11: everything the leaving car has in the air or on the ground goes at once, each the way the game ends it
 * (no blast, no damage): its projectiles (owner id; a boomerang flying back to it; a missile homing on it flies
 * on straight: portCarRekeyReferences clears its target), the arcs it shoots or that hold it, its mines and remote
 * bombs (the dropper's arming flag cleared), the shields it carries, its ricochet shots and bullets; effect sprites
 * that follow the record stop following and end their loop. */
static void PORT_TEXT portDropInWeapons(struct PortCarRec *r, struct TM2Object *obj, int id, int size)
{
    int i;

    for (i = 0; i < TM2_MAX_PROJECTILES; i++) {
        if (PROJ_LIFE(i) <= 0)
            continue;
        if (PROJ_OWNER(i) == id || PROJ_VICTIM(i) == obj) {
            fxRecordFree(i, 0);
            if (PROJ_TYPE(i) == 10 || PROJ_TYPE(i) == 11 || PROJ_TYPE(i) == 12 || PROJ_TYPE(i) == 15)
                soundStopInstance(PROJ_TYPE(i), i + 1);
        }
    }
    for (i = 0; i < TM2_MAX_ARCS; i++)
        if (ARC_COUNT(i) > 0 && (ARC_TARGET(i) == id || ARC_OWNER(i) == *r->slot
                                  || portDropInIn(ARC_ORIGIN(i), r->rec, size)
                                  || portDropInIn(ARC_ATTACH(i), r->rec, size)))
            ARC_COUNT(i) = 0;
    for (i = 0; i < PORT_DROPS; i++)
        if (DROP_OWNER(i) == id && (DROP_KIND(i) != 0 || DROP_STATUS(i) != 0)) {
            DROP_STATUS(i) = 0;
            DROP_KIND(i) = 0;
            DROP_HELD(i) = 0;
            if (DROP_OBJ(i) != 0) {
                DROP_OBJ(i)->visible = 0;
                DROP_OBJ(i) = 0;
            }
        }
    r->weapons->drop_armed = 0;
    for (i = 0; i < TM2_MAX_SHIELDS; i++)
        if (SHIELD_TIMER(i) > 0 && SHIELD_SOURCE(i) == obj) {
            SHIELD_TIMER(i) = 0;
            if (SHIELD_DST(i) != 0)
                SHIELD_DST(i)->visible = 0;
            r->state->energy_shield = 0;
        }
    for (i = 0; i < TM2_MAX_RICOCHETS; i++)
        if (RICO_LIFE(i) > 0 && RICO_OWNER(i) == id) {
            RICO_LIFE(i) = 0;
            if (RICO_OBJ(i) != 0)
                RICO_OBJ(i)->visible = 0;
        }
    for (i = 0; i < DROPIN_BULLETS; i++)
        if (BULLET_LIFE(i) > 0 && BULLET_OWNER(i) == id)
            bulletFree(i);
    for (i = 0; i < DROPIN_SPRITES; i++)
        if (SPRITE_FOLLOW(i) && portDropInIn(SPRITE_OWNER(i), r->rec, size)) {
            SPRITE_FOLLOW(i) = 0;       /* stays where it is and ends (the burning flame's loop) */
            SPRITE_LOOPS(i) = 0;
        }
}

/* The rest of what names the leaving car (CAR_GROUPS.md 14.3): a car it rides (crush) lets go, its wake effect goes
 * out, AI targets on it are dropped, the watched AI cursor moves off it; and through every special state, a leap
 * or a Dark Tooth swoop aimed at it ends. */
static void PORT_TEXT portDropInRefs(struct PortCarRec *r, int id, int size)
{
    struct PortCarRec p;
    int s, *fx;

    if (r->state->crush_contact) {
        for (s = 0; s < carWorldObjectCount && s < TM2_CAR_SLOT_CAP; s++)
            if (portCarGroups.slot_key[s] == r->stats->crush_owner && portCarDriver(s, &p))
                p.state->crush_peer = 0;
        r->state->crush_contact = 0;
    }
    fx = r->kind == 0 ? ((struct TM2HumanRecord *)r->rec)->fx : r->state->x2C.fx;
    if (fx != 0) {
        fx[2] = 0;
        if (r->kind == 0)
            ((struct TM2HumanRecord *)r->rec)->fx = 0;
        else
            r->state->x2C.fx = 0;
    }
    if (r->kind == 1) {
        for (s = 0; s < TM2_MAX_AI_CARS; s++)
            if (aiCarTargets[s] == r->index)
                aiCarTargets[s] = -1;
        if (viewWatchedAiCar == r->index)
            viewWatchedAiCar = 0;
    }
    portDropInSpecials((unsigned char *)r->rec, size, 0, id, id, r->kind == 0 ? r->index : -1);
}

#ifdef TM2_PORT_CAR_DROPIN
/* (the drop-in only part, up to portCarUnloadGroup) */
/* An AI record off for good, its car given back (B.6.3, CAR_GROUPS.md 14.4): dead then HP 0 (no explosion), wreck
 * state 4, out of range, type 0 (no engine id, no Dark Tooth rule on it), contacts and motion cleared, far below;
 * its slot fields name DROPIN_VACANT_SLOT, whose object is a dummy that is never drawn (not in the scene list),
 * collides with nothing and lies far below, so every loop over aiCarTotal that still reads aiCarObjects[j] /
 * carSlotObjects[slot] finds an object (census: car_contacts.c, fx_ground_fires.c, weapon_slam_swing.c read it
 * unguarded). max_hp is kept above 0 (aiCarDamageUpdate divides by it). aiCarTotal is not lowered. */
static void PORT_TEXT portDropInVacate(struct PortCarRec *r)
{
    struct TM2AIRecord *a = (struct TM2AIRecord *)r->rec;
    struct TM2Object *dummy = DROPIN_OBJ(DI.dummy);
    unsigned char *c;
    int i;

    r->state->dead = 1;
    r->stats->hp = 0;
    r->stats->wreck_state = 4;
    r->stats->killer = -1;
    r->stats->last_attacker = -1;
    if (r->stats->max_hp <= 0)
        r->stats->max_hp = 1;
    a->active = 0;
    a->type = 0;
    c = (unsigned char *)&a->contact;
    for (i = 0; i < (int)sizeof(a->contact); i++)
        c[i] = 0;
    a->motion.vel0 = 0;
    a->motion.speed = 0;
    a->motion.vel2 = 0;
    a->motion.spin0 = 0;
    a->motion.spin1 = 0;
    a->motion.yaw_rate = 0;
    a->motion.vel[0] = 0;
    a->motion.vel[1] = 0;
    a->motion.vel[2] = 0;
    a->motion.pos[2] = PORT_FAR_AWAY;
    dummy->visible = 0;
    dummy->pos[2] = PORT_FAR_AWAY;
    carSlotObjects[DROPIN_VACANT_SLOT] = dummy;
    carSlotKinds[DROPIN_VACANT_SLOT] = 1;
    carSlotIndices[DROPIN_VACANT_SLOT] = r->index;
    carSlotOwners[DROPIN_VACANT_SLOT] = 0;
    aiCarObjects[r->index] = (int)dummy;
    aiCarObjectSlots[r->index] = DROPIN_VACANT_SLOT;
    a->slot = DROPIN_VACANT_SLOT;
    aiCarTargets[r->index] = -1;
    DI.vacant[r->index] = 1;
}

/* Human record `from` moves to index `to` (B.6.4; the last human into a leaver's place, humans stay contiguous):
 * the record (0x814 bytes) and every per-human array; its index fields (player, id), the car slot's index and the
 * car object's id (TM2_HUMAN_ID(to)); every reference by object id or by address into the record (projectile, arc, drop,
 * ricochet and bullet owners, shield holders, effect sprites following it, the specials' pointers, a Dark Tooth
 * swoop at it). Its car, slot, key and everything by slot or object stay. `from` is left unbound and empty (its
 * object 0); the caller lowers humanCarTotal. The port remaps its own tables keyed by human index (net player <->
 * human; local views and HUDs too without TM2_PORT_HUMANS: CAR_GROUPS.md 14.5). With TM2_PORT_HUMANS the human's
 * local slot (portHumanLocal) moves with it, so its view, camera, HUD and radar stay on their slot (H1); `from`
 * becomes remote (-1); and its input word, stick tilt, layout, bindings and lives (H2), so no pad remap is needed
 * either; and its car selection (H6: settings human_car[0..1] / sessionPortHumanCars). Every per-human array (the
 * humans groups of config/US/port_variant_arrays.tsv) is written here: tools/check_humans.py check 11. */
void PORT_TEXT portHumanCompact(int from, int to)
{
    struct TM2HumanRecord *hf, *ht;
    int i, slot, id, id2, delta, *src, *dst;

    if (from == to || (unsigned int)from >= TM2_MAX_HUMANS || (unsigned int)to >= TM2_MAX_HUMANS)
        return;
    hf = HREC_AT(from);
    ht = HREC_AT(to);
    id = TM2_HUMAN_ID(from);
    id2 = TM2_HUMAN_ID(to);
    delta = (int)((unsigned char *)ht - (unsigned char *)hf);
    src = humanCars[from];
    dst = humanCars[to];
    for (i = 0; i < 0x814 / 4; i++)
        dst[i] = src[i];
    ht->player = to;
    ht->f002 = to;
    humanCarObjects[to] = humanCarObjects[from];
    humanCarObjectSlots[to] = humanCarObjectSlots[from];
    humanCarTypes[to] = humanCarTypes[from];
    padConfigs[to] = padConfigs[from];
    hudSmoothedSpeeds[to] = hudSmoothedSpeeds[from];
    carViewHotkeyReleased[to] = carViewHotkeyReleased[from];
    carResetWeapons[to] = carResetWeapons[from];
#ifdef TM2_PORT_HUMANS
    /* H1: the local slot moves with the record (explicitly: the default 0 means a slot by index); the old index
       is left remote, so a human the port starts there later has no slot until the port gives it one */
    i = portHumanSlot(from);
    portHumanLocal[to] = i >= 0 ? i + 1 : -1;
    portHumanLocal[from] = -1;
    /* H2: its inputs, layout, bindings and lives too (the port writes the next frame's word at the new index) */
    portHumanPad[to] = portHumanPad[from];
    portHumanSteer[to] = portHumanSteer[from];
    portHumanThrottle[to] = portHumanThrottle[from];      /* request 121 */
    portHumanLayout[to] = portHumanLayout[from];
    portHumanLives[to] = portHumanLives[from];
    for (i = 0; i < TM2_HUMAN_BINDINGS; i++)
        portHumanBindings[to][i] = portHumanBindings[from][i];
    portHumanPad[from] = 0;
    portHumanSteer[from] = 0;
    portHumanThrottle[from] = 0;
    /* request 66: its control options (match state) */
    portHumanControl[to] = portHumanControl[from];
    portHumanControl[from] = 0;
    /* request 126: its wreck's mark (match state) */
    portHumanWreckLive[to] = portHumanWreckLive[from];
    portHumanWreckLive[from] = 0;
    /* H6: its car selection too (TM2_SETTINGS_HUMAN_CAR of sessionSettings: the record's human_car[0..1], then
       sessionPortHumanCars), so a later load of the match (sessionLoadArenaAndCars) finds its car at the new index */
    {
        signed char *cars = (signed char *)&sessionSettings + 0x08;     /* settings.h: human_car[2] at +0x08 */
#if TM2_MAX_HUMANS > 2
        int car = from < 2 ? cars[from] : sessionPortHumanCars[from - 2];

        if (to < 2)
            cars[to] = car;
        else
            sessionPortHumanCars[to - 2] = car;
#else
        cars[to] = cars[from];      /* the placement check's 2 records: the record holds both */
#endif
#ifdef TM2_PORT_CAR_NUMBERS
        portSettingsHumanCar[to] = portSettingsHumanCar[from];     /* request 42 item 3: its car number too */
#endif
    }
#endif
#ifdef TM2_HUMAN_HANDOVER
    /* request 50: a vacant record stays vacant at its new index */
    if (TM2_HUMAN_VACANT(from))
        portHumanVacant.mask |= 1u << to;
    else
        portHumanVacant.mask &= ~(1u << to);
    portHumanVacant.mask &= ~(1u << from);
    portHumanVacant.ai[to] = portHumanVacant.ai[from];
    portHumanVacant.ai[from] = 0;
#endif
    slot = ht->slot;
    if (portCarSlotValid(slot)) {
        if (carSlotKinds[slot] == 0 && carSlotIndices[slot] == from)
            carSlotIndices[slot] = to;
        ((struct TM2Object *)carSlotObjects[slot])->id = id2;
        portCarBindSet(0, to, portCarGroups.slot_group[slot]);
    }
    for (i = 0; i < TM2_MAX_PROJECTILES; i++)
        if (PROJ_OWNER(i) == id)
            PROJ_OWNER(i) = id2;
    for (i = 0; i < TM2_MAX_ARCS; i++) {
        if (ARC_COUNT(i) <= 0)
            continue;
        if (ARC_TARGET(i) == id)
            ARC_TARGET(i) = id2;
        if (ARC_ID(i) == id)
            ARC_ID(i) = id2;
        if (portDropInIn(ARC_ORIGIN(i), hf, 0x814))
            ARC_ORIGIN(i) = (int *)((unsigned char *)ARC_ORIGIN(i) + delta);
        if (portDropInIn(ARC_ATTACH(i), hf, 0x814))
            ARC_ATTACH(i) = (int *)((unsigned char *)ARC_ATTACH(i) + delta);
    }
    for (i = 0; i < PORT_DROPS; i++)
        if (DROP_OWNER(i) == id)
            DROP_OWNER(i) = id2;
    for (i = 0; i < TM2_MAX_SHIELDS; i++)
        if (SHIELD_TIMER(i) > 0 && SHIELD_ID(i) == from)
            SHIELD_ID(i) = to;
    for (i = 0; i < TM2_MAX_RICOCHETS; i++)
        if (RICO_OWNER(i) == id)
            RICO_OWNER(i) = id2;
    for (i = 0; i < DROPIN_BULLETS; i++)
        if (BULLET_OWNER(i) == id)
            BULLET_OWNER(i) = id2;
    for (i = 0; i < DROPIN_SPRITES; i++)
        if (portDropInIn(SPRITE_OWNER(i), hf, 0x814))
            SPRITE_OWNER(i) += delta;
    portDropInSpecials((unsigned char *)hf, 0x814, delta, id, id2, from);
    /* the old place: unbound, owning nothing (the pointers went with the copy) */
    hf->weapons.slam = 0;
    hf->weapons.fire_effect = 0;
    hf->weapons.f40 = 0;
    hf->fx = 0;
    humanCarObjects[from] = 0;
    humanCarObjectSlots[from] = -1;
    portCarBindSet(0, from, 0);
}

static int PORT_TEXT portDropInAllowed(void)
{
    return !gameAttractDemoMode;     /* the attract demo drives and respawns every AI record itself */
}

/* Request 33 / B.6.3: record (kind, index) and its car leave the match for good. The car becomes a free car of its
 * type (its group can be started again: portCarStartHuman / portCarStartAi; portCarFreeGroupOfType finds it).
 *   1. specials and look of the car ended (portCarResetSpecials, portCarResetLook: engine sound included);
 *   2. its projectiles, mines, drops, arcs, shields, ricochets, bullets removed at once (G.11);
 *   3. references to it cleared: kill credits and attackers, taser and drops by slot, homing targets
 *      (portCarRekeyReferences(slot, -1)); crush, wake, AI targets, the watched AI, leaps and swoops at it;
 *   4. the car parked (not drawn, out of the hit pass, body hidden, far below; slot kind 2, index -1);
 *   5. AI: the record vacant (portDropInVacate); human: the last human moves into its index (portHumanCompact)
 *      and humanCarTotal goes down by one.
 * Returns 1, or TM2_DROPIN_ERR_RECORD (no such bound record), TM2_DROPIN_ERR_LAST (the last human),
 * TM2_DROPIN_ERR_BUSY (attract demo). Between frames only. */
int PORT_TEXT portCarRemove(int kind, int index)
{
    struct PortCarRec r;
    struct TM2Object *obj;
    int slot, id, size, last;

    if (!portDropInAllowed())
        return TM2_DROPIN_ERR_BUSY;
    if (!portCarRec(kind, index, &r))
        return TM2_DROPIN_ERR_RECORD;
    slot = *r.slot;
    if (!portCarSlotValid(slot) || slot >= DROPIN_VACANT_SLOT || carSlotKinds[slot] != kind
        || carSlotIndices[slot] != index || (kind == 1 && DI.vacant[index]))
        return TM2_DROPIN_ERR_RECORD;
    if (kind == 0 && humanCarTotal <= 1)
        return TM2_DROPIN_ERR_LAST;
    if (kind == 1 && carWorldObjectCount > DROPIN_VACANT_SLOT)
        return TM2_DROPIN_ERR_NOLOAD;   /* the load took the slot vacant records point at (64 car roots) */
    DI.live = 1;
    obj = (struct TM2Object *)carSlotObjects[slot];
    id = portDropInId(kind, index);
    size = kind == 0 ? 0x814 : 0x854;
    portCarResetSpecials(slot);
    portCarResetLook(slot);
    portDropInSlotSound(slot);          /* its loops (players 2..: turbo, gun, burn; tornado; swoops) stop too */
    portDropInWeapons(&r, obj, id, size);
    portCarRekeyReferences(slot, -1);
    portDropInRefs(&r, id, size);
    r.state->burning = 0;
    r.weapons->fire_effect = 0;
    portCarParkSlot(slot);
    if (kind == 1) {
        portDropInVacate(&r);
        portCarBindSet(1, index, 0);
    } else {
        last = humanCarTotal - 1;
        if (index != last) {
            portHumanCompact(last, index);
        } else {
            humanCarObjects[last] = 0;
            humanCarObjectSlots[last] = -1;
            portCarBindSet(0, last, 0);
        }
        HREC_AT(last)->stats.hp = 0;
        humanCarTotal = last;
    }
    DI.removed++;
    return 1;
}

/* Cars driving now: every human, and every AI record bound to a car of this load that is not vacant and not parked
 * (SURVIVAL.md 3.1: the Juggernaut's Minion). The rule humans + bots <= TM2_PORT_CARS is checked with it. */
int PORT_TEXT portCarActiveCount(void)
{
    struct PortCarRec r;
    int j, n;

    n = humanCarTotal;
#ifdef TM2_HUMAN_HANDOVER
    n -= portHumanVacantCount();        /* request 50: a vacant human record drives no car */
#endif
    for (j = 0; j < aiCarTotal && j < TM2_MAX_AI_CARS; j++)
        if (!DI.vacant[j] && portCarRec(1, j, &r) && portCarSlotValid(*r.slot) && !portCarParked(&r))
            n++;
    return n;
}

/* The first slot of load group `group` when its car has no driver (slot kind 2, index -1), else -1. */
static int PORT_TEXT portDropInFreeSlot(int group)
{
    int s = portCarSlotOfGroup(group);

    if (s < 0 || s >= DROPIN_VACANT_SLOT || carSlotKinds[s] != 2 || carSlotIndices[s] != -1)
        return -1;
    if (DI.group_live[group] == 2)
        return -1;                      /* unloaded (portCarUnloadGroup) */
    return s;
}

int PORT_TEXT portCarGroupFree(int group)
{
    if ((unsigned int)group >= TM2_PORT_DROPIN_GROUPS)
        return 0;
    return portDropInFreeSlot(group) >= 0;
}

/* A free car of car type `type` (0x10..0xF0): its load group, lowest first; 0 if none. */
int PORT_TEXT portCarFreeGroupOfType(int type)
{
    int g, s;

    for (g = 1; g < TM2_PORT_DROPIN_GROUPS; g++) {
        s = portDropInFreeSlot(g);
        if (s >= 0 && carSlotOwners[s] == type)
            return g;
    }
    return 0;
}

/* A start mid-match picks its start node with the farthest-free rule (portStartNodePick's last case: the start
 * entry farthest from every live car, nothing marked; SIXTEEN_CARS.md 5.3) unless the caller asks for retail's
 * random pick: portCarOptions bit 2 is set for the call, and portCarDropIn.start_live makes the picker skip its
 * "untaken entries" step (the taken marks are the round start's, stale mid-match). */
static int PORT_TEXT portDropInStartBegin(int flags)
{
    int saved = portCarOptions;

    if (!(flags & TM2_DROPIN_START_RANDOM)) {
        portCarOptions |= TM2_PORT_CAR_OPT_FAR_STARTS;
        DI.start_live = 1;
    }
    return saved;
}

static void PORT_TEXT portDropInStartEnd(int saved)
{
    portCarOptions = saved;
    DI.start_live = 0;
}

/* carRestart (800FD550) for a record that joins: the same reset of the record, without the four match-wide resets
 * a lost-a-life respawn makes (weaponDropsReset, weaponsRoundReset, projectilesReset, fxSpritesReset: every car's
 * projectiles, mines and specials would end) and without hudMessageReset (every HUD's message). Fresh weapons. */
static void PORT_TEXT portDropInRestartHuman(struct TM2HumanRecord *human, int type)
{
    struct TM2Object *obj;

    human->type = type;
    human->stats.f00 = 1;
    humanCarPlaceAtStart(human);
    weaponBlockInit(human, &human->weapons, 1);
    human->weapons.invisible = 0;
    human->weapons.invis_timer = 0;
    human->input.f22 = 0;
    human->input.f21 = 0;
    human->input.f24 = 0;
    viewCameraOffsetsInit(human);
    human->state.x2C.route = 0;
    carShakeStart(human, 2, 2, 0, 1);
    human->speed_change = 0;
    aiClearControls(&human->input);
    carApplyPadConfig(human, PadGetConfig(human->player));
    human->stats.cheat_a = 0;
    human->stats.cheat_b = 0;
    human->stats.cheat_released = 0;
    human->stats.f0C = 0;
    human->stats.heal_spot_used = 0;
    human->contact.f01 = 1;
    obj = (struct TM2Object *)humanCarObjects[human->player];
    if (obj) {
        obj->light = lightGetEnv(0);
        obj->visible = 1;
    }
}

/* Request 33 / B.6.5: a human record starts mid-match on the free car of load group `group`, as a respawn. `index`
 * must be humanCarTotal (humans stay contiguous; -1 = that). The per-record part of uaInit (reset weapons,
 * damageable, id, the six flags, pad configuration 1), hudResetSpeed, then the binding (as carBindHumanSlot: tables,
 * player, object id TM2_HUMAN_ID(index), kind 0, drawn) and a fresh start: the record's own part of carRestart (fresh stats,
 * HP and weapons, the start farthest from the live cars), or with TM2_DROPIN_START_RETAIL the retail
 * carBindHumanSlot itself (carRestart's match-wide resets included). carSetViewMode(rec, 2) as carBindHumanSlot
 * (outside the attract demo; it only fills the record's camera fields, so a remote player without a view is fine).
 * Returns the new human index, or TM2_DROPIN_ERR_INDEX (not humanCarTotal / no room in TM2_MAX_HUMANS),
 * TM2_DROPIN_ERR_GROUP (no free car in that group), TM2_DROPIN_ERR_FULL (TM2_PORT_CARS cars drive already),
 * TM2_DROPIN_ERR_BUSY (attract demo). Between frames only. portCarStartHumanAt (HUMANS16_PLAN.md H5): the same
 * at route node `node` (aiRouteNodes index; < 0 = the farthest free start, as portCarStartHuman), or
 * TM2_DROPIN_ERR_NODE when the arena has no such node. */
int PORT_TEXT portCarStartHuman(int index, int group, int flags)
{
    return portCarStartHumanAt(index, group, flags, -1);
}

extern int aiRouteNodeCount;

int PORT_TEXT portCarStartHumanAt(int index, int group, int flags, int node)
{
    struct TM2HumanRecord *h;
    struct PortCarRec r;
    struct TM2Object *obj;
    int i, k, slot, type, saved;

    if (!portDropInAllowed())
        return TM2_DROPIN_ERR_BUSY;
#ifdef TM2_HUMAN_HANDOVER
    /* request 50b (HUMANS16_PLAN.md 8.4): a vacant index is taken back in place (portHumanRejoin; the flags'
       retail restart and random start do not apply there) */
    if (index >= 0 && index < humanCarTotal && index < TM2_MAX_HUMANS && TM2_HUMAN_VACANT(index))
        return portHumanRejoin(index, group, node);
#endif
    i = humanCarTotal;
    if ((index >= 0 && index != i) || i >= TM2_MAX_HUMANS)
        return TM2_DROPIN_ERR_INDEX;
    if (node >= aiRouteNodeCount)
        return TM2_DROPIN_ERR_NODE;
    if ((unsigned int)group >= TM2_PORT_DROPIN_GROUPS || (slot = portDropInFreeSlot(group)) < 0)
        return TM2_DROPIN_ERR_GROUP;
    if (portCarActiveCount() >= TM2_PORT_CARS)
        return TM2_DROPIN_ERR_FULL;
    DI.live = 1;
    h = HREC_AT(i);
    type = carSlotOwners[slot];
    obj = (struct TM2Object *)carSlotObjects[slot];
    /* uaInit's per-record part */
    carSetResetWeapons(1, i);
    *(int *)((unsigned char *)h + 0x1F4) = 1;
    h->f002 = i;
    for (k = 0x96; k < 0x9C; k++)
        ((unsigned char *)h)[k] = 0;
    PadSetConfig(1, i);
#ifdef TM2_PORT_HUMANS
    portHumanSetLayout(i, portHumanLayout[i]);  /* H2: its bindings from the layout the port wrote */
#endif
    hudResetSpeed(i);
    h->stats.hp = 0;                    /* not a live car for the start picker */
    h->weapons.slam = 0;                /* a claw record of a former occupant went with it */
    h->weapons.fire_effect = 0;
    h->weapons.f40 = 0;
    h->fx = 0;
    humanCarObjects[i] = (int)obj;      /* worldUpdate's human loop stops at the first null object */
    humanCarTotal = i + 1;
    saved = portDropInStartBegin(flags);
    if (node >= 0) {
        portCarOptions |= TM2_PORT_CAR_OPT_FAR_STARTS;     /* (restored by portDropInStartEnd) */
        portRound.start_node = node + 1;    /* H5: the port's start (portStartNodePick returns it) */
    }
    if (flags & TM2_DROPIN_START_RETAIL) {
        carBindHumanSlot(i, slot, type);
    } else {
        portCarBindSlot(0, i, slot);
        h->player = i;
        portDropInRestartHuman(h, type);
        if (!gameAttractDemoMode)
            carSetViewMode(h, 2);
        csSetDrawEnable(obj, 1);
    }
    portDropInStartEnd(saved);
    portRound.start_node = 0;
    portCarBindSet(0, i, group);
    if (portCarRec(0, i, &r))
        portCarModelOn(&r, 1);
    DI.started++;
    return i;
}

/* Request 33 / B.6.5: an AI record starts mid-match on the free car of load group `group`, as a respawn: `index`
 * = a vacant record, or aiCarTotal (a new one: aiCarTotal + 1), or -1 (the lowest vacant record, else a new one).
 * carBindAiSlot (8011A5CC: binding, object id index + 50, aiCarStart with fresh stats, HP and weapons at a start
 * node, the AI profile), the start farthest from the live cars (TM2_DROPIN_START_RANDOM: retail's random pick),
 * target the nearest human. Returns the AI index, or TM2_DROPIN_ERR_INDEX, TM2_DROPIN_ERR_GROUP,
 * TM2_DROPIN_ERR_FULL, TM2_DROPIN_ERR_BUSY. Between frames only. */
int PORT_TEXT portCarStartAi(int index, int group, int flags)
{
    struct PortCarRec r;
    int j, k, slot, saved;

    if (!portDropInAllowed())
        return TM2_DROPIN_ERR_BUSY;
    if (index < 0) {
        for (j = 0; j < aiCarTotal && j < TM2_MAX_AI_CARS && !DI.vacant[j]; j++)
            ;
    } else {
        j = index;
    }
    if (j >= TM2_MAX_AI_CARS || j > aiCarTotal || (j < aiCarTotal && !DI.vacant[j]))
        return TM2_DROPIN_ERR_INDEX;
    if ((unsigned int)group >= TM2_PORT_DROPIN_GROUPS || (slot = portDropInFreeSlot(group)) < 0)
        return TM2_DROPIN_ERR_GROUP;
    if (portCarActiveCount() >= TM2_PORT_CARS)
        return TM2_DROPIN_ERR_FULL;
    DI.live = 1;
    AREC_AT(j)->stats.hp = 0;           /* not a live car for the start picker */
    if (j == aiCarTotal) {
        AREC_AT(j)->weapons.slam = 0;   /* never bound in this load (carTablesClear zeroed it): no claw record */
        aiCarTotal = j + 1;
    }
    DI.vacant[j] = 0;
    if (carSlotKinds[DROPIN_VACANT_SLOT] == 1 && carSlotIndices[DROPIN_VACANT_SLOT] == j) {
        /* the dummy slot names another vacant record, if there is one (a slot -> record lookup always finds an
           AI record) */
        for (k = 0; k < aiCarTotal && k < TM2_MAX_AI_CARS && !DI.vacant[k]; k++)
            ;
        if (k < aiCarTotal && k < TM2_MAX_AI_CARS) {
            carSlotIndices[DROPIN_VACANT_SLOT] = k;
        } else {
            carSlotKinds[DROPIN_VACANT_SLOT] = 2;
            carSlotIndices[DROPIN_VACANT_SLOT] = -1;
        }
    }
    saved = portDropInStartBegin(flags);
    carBindAiSlot(j, slot, carSlotOwners[slot]);
    portDropInStartEnd(saved);
    aiCarTargets[j] = -1;
    portCarBindSet(1, j, group);
    if (portCarRec(1, j, &r))
        portCarModelOn(&r, 1);
    DI.started++;
    return j;
}

/* sessionBindCars (modes 3/4 rebind every round): once a drop-in function ran in this load, the records to bind are
 * the records there are (humanCarTotal, aiCarTotal), each on its own group (portCarBindGroup, set by the starts and
 * the compaction), and a vacant AI record is skipped (it stays vacant). Before that: the settings' counts, nothing
 * skipped (retail). */
int PORT_TEXT portCarBindCount(int ai, int count)
{
#ifdef TM2_HUMAN_HANDOVER
    if (!DI.live && ai)
        return portHumanBindAiCount(count);     /* request 50: the AI records a hand-over added */
#endif
    if (!DI.live)
        return count;
    return ai ? aiCarTotal : humanCarTotal;
}

int PORT_TEXT portCarBindSkip(int ai, int index)
{
    return DI.live && ai && (unsigned int)index < TM2_PORT_DROPIN_AI && DI.vacant[index];
}

/* ---- mid-match car loading (B.7, CAR_GROUPS.md 14.6) ---- */
extern int *bitmapWords;                /* the VRAM usage bitmap (1 bit per texel; bitmapIsFreeXY, bitmapMarkRect) */
extern int *vramTexSlots;               /* 5 words per slot: tpage, x, y, CLUT x, CLUT y; group * stride + image */
extern int *bitmapRecords;              /* image records: key, tpage, x, y, CLUT x, CLUT y (bitmapAddRecord) */
extern int bitmapRecordCount;
extern int bitmapTrackingEnabled;
extern int csPoolCount;
extern int animListCount;
extern int clutAnimLastIndex;
extern int assetInstallShift;
extern int bitmapParseContainer(const void *data, int group);
extern void assetInstallImage(void *base, int group);
extern void bitmapDisableTracking(void);
extern void renderSetWorldShift(int shift);
extern void carChoiceRegisterGroup(void *object, int key);
/* what an unload clears (CAR_GROUPS.md 14.7) */
extern void *csDrawListHead;                    /* the drawn-object list, linked through +0x120 */
extern void *animNodeList[100];                 /* animRegister's list (animUpdateAll steps it every frame) */
#if TM2_MAX_WORLD_MODELS <= TM2_RETAIL_WORLD_MODELS
extern void *renderWorldModels[TM2_MAX_WORLD_MODELS];
#endif
#if TM2_MAX_CLUT_ANIM_DEFS <= TM2_RETAIL_CLUT_ANIM_DEFS
extern int clutAnimDefs[TM2_MAX_CLUT_ANIM_DEFS][20 / 4];       /* id/sub, type, x, y, count */
#endif
#if TM2_MAX_CLUT_ANIMS <= TM2_RETAIL_CLUT_ANIMS
extern int clutAnimRunning[TM2_MAX_CLUT_ANIMS][0x424 / 4];      /* active +0x00, def +0x20 */
#endif
extern unsigned char *worldLoadParentObject;    /* worldLoadObject: the last car root (its +7 freeze node's parent) */
extern int weaponSpiralMissileModel, weaponBoomerangModel, weaponGrimmSpecialModel, weaponGhostMissileModel;
extern int weaponRicochetModelAlt, weaponRicochetModel, weaponHomingMissileModel;
extern void *objRegistryId2324, *objRegistryId2325;    /* Axel's shockwave, Shadow's strike animation (nodes) */
extern void *slamArmObject, *slamClawObject;           /* Mr. Slam's arm and claw (nodes) */
#ifdef TM2_PORT_HUMANS
extern int soundStopSharedLoop(int, int);
#endif

#define LT portCarLiveTex
#define UN portCarUnload
#define UOBJ_DRAW(o) (*(int *)((unsigned char *)(o) + 0x08))             /* drawn and in the hit pass */
#define UOBJ_MODEL(o) (*(unsigned char **)((unsigned char *)(o) + 0x11C))
#define UOBJ_NEXT(o) (*(void **)((unsigned char *)(o) + 0x120))          /* drawn-object list */
#define PROJ_OBJ(p) (*(struct TM2Object **)&fxRecords[p][0x34 / 4])      /* the projectile's own object */
#define RICO_SLOT(k) (((struct TM2Object **)weaponRicochetPool)[k])     /* the ricochet shots' objects */
#define CS_WORDS (0x128 / 4)
#define UNLOAD_OWN 48                   /* objects one unload gives back (cars, special objects, leaked ones) */
#define DROPIN_CS_HEADROOM 24           /* coordinate systems a car install may take (roots, weapon objects) */
#define DROPIN_ANIM_HEADROOM 16         /* animRegister entries (of 100) */
#define DROPIN_CLUTDEF_HEADROOM 4       /* clutAnimNewDef entries (Dark Tooth registers two) */
#define DROPIN_MISSING 0x7FFF           /* a slot field not written by the parse */

/* bitmapSetupStorage (every load) under the define: the VRAM usage bitmap, the texture-slot table and the image
 * records in port storage (portCarLiveTex) instead of packet buffer 0 and an ordering table, so they live through
 * the match and a car can be parsed and installed between frames. Same contents, same order of writes as
 * retail; only where they are (retail's copies were overwritten by the first frames). */
void PORT_TEXT portCarLiveStorage(void)
{
    int g;

    bitmapWords = (int *)LT.bitmap;
    vramTexSlots = (int *)LT.slots;
    bitmapRecords = (int *)LT.records;
    /* unloading (14.7): each group's image records, noted as the load parses its TMS (portDropInNoteLoad) */
    for (g = 0; g < TM2_PORT_DROPIN_GROUPS; g++) {
        UN.rec_first[g] = -1;
        UN.rec_end[g] = -1;
        UN.clut256[g] = 0;
    }
    UN.rec_group = 0;
}

/* sessionLoadArenaAndCars, after the car DMDs: the first free byte of the DMD area (`next`), its end and the
 * groups loaded (1..groups - 1), so a later load appends. */
void PORT_TEXT portCarLoadDone(unsigned char *next, unsigned int end, int groups)
{
    DI.dmd_next = (unsigned char *)(((unsigned int)next + 3) & ~3u);
    DI.dmd_end = end;
    DI.next_group = groups;
    if (UN.rec_group > 0 && UN.rec_group < TM2_PORT_DROPIN_GROUPS && UN.rec_end[UN.rec_group] < 0)
        UN.rec_end[UN.rec_group] = bitmapRecordCount;   /* the last TMS's records end (14.7) */
}

/* portCarLoadGroup (the match load, request 32 step 2's one way to load a car file), for an unload (14.7): a car TMS
 * about to be parsed (dmd 0: its group's image records start at the record count; the group before ends there), or
 * the DMD copy just loaded (dmd 1: where and how long). */
static void PORT_TEXT portDropInNoteLoad(unsigned char *dst, int group, int dmd, int size)
{
    if ((unsigned int)group >= TM2_PORT_DROPIN_GROUPS)
        return;
    if (dmd) {
        DI.group_dmd[group] = dst;
        DI.group_size[group] = size > 0 ? size : 0;
        return;
    }
    if (UN.rec_group > 0 && UN.rec_group < TM2_PORT_DROPIN_GROUPS && UN.rec_end[UN.rec_group] < 0)
        UN.rec_end[UN.rec_group] = bitmapRecordCount;
    UN.rec_first[group] = bitmapRecordCount;
    UN.rec_end[group] = -1;             /* closed by the next car TMS or portCarLoadDone */
    UN.rec_group = group;
}

/* portCarGroupsReset (every load): the drop-in state of the last match. */
static void PORT_TEXT portCarDropInReset(void)
{
    int i;

    DI.live = 0;
    DI.removed = 0;
    DI.started = 0;
    DI.loaded = 0;
    DI.start_live = 0;
    DI.next_group = 0;
    DI.dmd_next = 0;
    DI.dmd_end = 0;
    for (i = 0; i < TM2_PORT_DROPIN_AI; i++)
        DI.vacant[i] = 0;
    for (i = 0; i < TM2_PORT_DROPIN_GROUPS; i++) {
        DI.group_live[i] = 0;
        DI.group_dmd[i] = 0;
        DI.group_size[i] = 0;
    }
    /* unloading (14.7): no free DMD range, an empty coordinate-system free list (the pool restarts with the load);
       the image records are the TMS pass's (portCarLiveStorage, before this reset) */
    UN.unloaded = 0;
    UN.cs_count = 0;
    UN.cs_high = 0;
    UN.range_count = 0;
}

/* A TXSP container (bitmapParseContainer's format: magic, version 0x43, result, count, keys, sized entries) fully
 * inside `size` bytes with at most `max` images, word aligned. Returns the image count, -1 if bad (the game would
 * exit(-1) on a bad header). */
static int PORT_TEXT portDropInTmsCount(const unsigned int *d, int size, int max)
{
    unsigned int off, n, i, len;

    if (d == 0 || ((unsigned int)d & 3) || size < 16 || d[0] != 0x50535854u || d[1] != 0x43u)
        return -1;
    n = d[3];
    if (n > (unsigned int)max)
        return -1;
    off = 4 + n;
    for (i = 0; i < n; i++) {
        if ((off + 1) * 4 > (unsigned int)size)
            return -1;
        len = d[off];
        if ((len & 3) || len < 12 || (off + 1) * 4 + len > (unsigned int)size)
            return -1;
        off += 1 + (len >> 2);
    }
    return (int)n;
}

/* A PSXD model file (assetInstallImage's format) of `size` bytes, word aligned, whose roots are car roots
 * (ids 0x10..0xF0, as on the disc and as a mod's must be, PER_CAR_PLAN.md E.1). Returns the first root's type, 0
 * if bad; *roots = the car roots (Minion: 2). The root words are addresses relative to 0x80019C40 (the retail load
 * address: assetInstallImage's delta). */
static int PORT_TEXT portDropInDmdType(const unsigned int *d, int size, int *roots)
{
    unsigned int n, i, off, id;
    int type = 0;

    *roots = 0;
    if (d == 0 || ((unsigned int)d & 3) || size < 32 || d[0] != 0x50535844u || d[1] != 0x43u)
        return 0;
    n = d[6];
    if (n == 0 || (7 + n) * 4 > (unsigned int)size)
        return 0;
    for (i = 0; i < n; i++) {
        off = d[7 + i] - 0x80019C40u;
        if (d[7 + i] == 0)
            break;
        if (off >= (unsigned int)size - 4 || (off & 3))
            return 0;
        id = ((const unsigned short *)((const unsigned char *)d + off))[1];
        if (id >= 0x10 && id <= 0xF0 && (id & 0xF) == 0) {
            if (type == 0)
                type = id;
            ++*roots;
        }
    }
    return type;
}

/* The key portCarRegisterRoot will hand the group: the type when no group holds it, else the lowest free overflow
 * key (portCarFreeDupKey: one an unload gave back, else the next); 0 when none is left (it would share the first
 * car's entries: refused). */
static int PORT_TEXT portDropInNextKey(int type)
{
    int h;

    for (h = 0; h < TM2_CAR_SLOT_CAP; h++)
        if (portCarGroups.group_key[h] == type)
            break;
    if (h >= TM2_CAR_SLOT_CAP)
        return type;
    h = portCarFreeDupKey();
    return h < 0 ? 0 : (16 + h) << 4;
}

/* ---- unloading: the resources a car gives back (CAR_GROUPS.md 14.7) ---- */

/* csAlloc under the define: an object an unload gave back (the last one first), 0 when the list is empty or stale
 * (the pool was reset since: csPoolInit at the next load or screen; the list is then dropped). */
void *PORT_TEXT portCsTake(void)
{
    if (UN.cs_count <= 0)
        return 0;
    if (csPoolCount < UN.cs_high) {
        UN.cs_count = 0;
        return 0;
    }
    UN.cs_count--;
    return csObjectPool[UN.cs_free[UN.cs_count]];
}

/* Coordinate systems a new install can still take: the pool's rest and the free list. */
static int PORT_TEXT portCsRoom(void)
{
    int n = TM2_MAX_COORD_SYSTEMS - csPoolCount;

    if (csPoolCount >= UN.cs_high)
        n += UN.cs_count;
    return n;
}

/* Give scene object `o` back: out of the drawn-object list (csListAdd's), not drawn, no model, on csAlloc's free
 * list. Only an object of the pool that is handed out and not on the list already. */
static void PORT_TEXT portCsGive(void *o)
{
    void **pp;
    int i, k;

    if (o == 0 || o == (void *)DI.dummy)
        return;
    i = (int)((int *)o - csObjectPool[0]) / CS_WORDS;
    if (i < 0 || i >= csPoolCount || i >= TM2_MAX_COORD_SYSTEMS || (int *)o != csObjectPool[i])
        return;
    if (csPoolCount < UN.cs_high)
        UN.cs_count = 0;
    for (k = 0; k < UN.cs_count; k++)
        if (UN.cs_free[k] == i)
            return;
    for (pp = &csDrawListHead; *pp != 0; pp = &UOBJ_NEXT(*pp))
        if (*pp == o) {
            *pp = UOBJ_NEXT(o);
            break;
        }
    UOBJ_NEXT(o) = 0;
    UOBJ_DRAW(o) = 0;
    UOBJ_MODEL(o) = 0;
    if (UN.cs_count < TM2_PORT_UNLOAD_CS) {
        UN.cs_free[UN.cs_count++] = (unsigned short)i;
        UN.cs_high = csPoolCount;
    }
}

/* The DMD area's allocator: first fit over the free ranges (by address), else the tail (DI.dmd_next .. dmd_end).
 * Ranges are 4-aligned; nothing in the area moves (models, registries and objects point into the copies), so a
 * hole only takes a DMD that fits in it. */
static unsigned char *PORT_TEXT portDmdFind(int size)
{
    unsigned int n = ((unsigned int)size + 3) & ~3u;
    int i;

    if (size <= 0)
        return 0;
    for (i = 0; i < UN.range_count; i++)
        if (UN.range_size[i] >= n)
            return (unsigned char *)UN.range_start[i];
    if (DI.dmd_next != 0 && (unsigned int)DI.dmd_next <= DI.dmd_end && n <= DI.dmd_end - (unsigned int)DI.dmd_next)
        return DI.dmd_next;
    return 0;
}

/* Take what portDmdFind returned. */
static void PORT_TEXT portDmdTake(unsigned char *p, int size)
{
    unsigned int n = ((unsigned int)size + 3) & ~3u;
    int i, k;

    for (i = 0; i < UN.range_count; i++)
        if (UN.range_start[i] == (unsigned int)p) {
            UN.range_start[i] += n;
            UN.range_size[i] -= n;
            if (UN.range_size[i] == 0) {
                for (k = i + 1; k < UN.range_count; k++) {
                    UN.range_start[k - 1] = UN.range_start[k];
                    UN.range_size[k - 1] = UN.range_size[k];
                }
                UN.range_count--;
            }
            return;
        }
    DI.dmd_next = p + n;
}

/* Give a DMD copy's bytes back: [p, p + size) shrunk to whole words, merged with its neighbours; a range that
 * reaches the tail becomes tail again. */
static void PORT_TEXT portDmdGive(unsigned char *p, int size)
{
    unsigned int a, e;
    int i, k;

    if (p == 0 || size <= 0)
        return;
    a = ((unsigned int)p + 3) & ~3u;
    e = ((unsigned int)p + (unsigned int)size) & ~3u;
    if (e <= a || DI.dmd_next == 0 || e > (unsigned int)DI.dmd_next)
        return;
    for (i = 0; i < UN.range_count && UN.range_start[i] < a; i++)
        ;
    if (i > 0 && UN.range_start[i - 1] + UN.range_size[i - 1] == a) {
        i--;
        UN.range_size[i] += e - a;
    } else {
        if (UN.range_count >= TM2_PORT_UNLOAD_RANGES)
            return;                     /* cannot happen: at most one hole more than groups */
        for (k = UN.range_count; k > i; k--) {
            UN.range_start[k] = UN.range_start[k - 1];
            UN.range_size[k] = UN.range_size[k - 1];
        }
        UN.range_start[i] = a;
        UN.range_size[i] = e - a;
        UN.range_count++;
    }
    if (i + 1 < UN.range_count && UN.range_start[i] + UN.range_size[i] == UN.range_start[i + 1]) {
        UN.range_size[i] += UN.range_size[i + 1];
        for (k = i + 2; k < UN.range_count; k++) {
            UN.range_start[k - 1] = UN.range_start[k];
            UN.range_size[k - 1] = UN.range_size[k];
        }
        UN.range_count--;
    }
    k = UN.range_count - 1;
    if (k >= 0 && UN.range_start[k] + UN.range_size[k] == (unsigned int)DI.dmd_next) {
        DI.dmd_next = (unsigned char *)UN.range_start[k];
        UN.range_count--;
    }
}

/* The first of `roots` consecutive free car slots below the vacant slot (given back by an unload, or past
 * carWorldObjectCount), -1 if none. */
static int PORT_TEXT portDropInSlotRun(int roots)
{
    int s, k;

    for (s = 0; s + roots <= DROPIN_VACANT_SLOT; s++) {
        for (k = 0; k < roots; k++)
            if (s + k < carWorldObjectCount && portCarGroups.slot_group[s + k] != TM2_PORT_SLOT_FREE)
                break;
        if (k == roots)
            return s;
    }
    return -1;
}

/* The load group number a live load takes: the lowest one an unload gave back, else the next new one. */
static int PORT_TEXT portDropInFreeGroup(void)
{
    int g;

    for (g = 1; g < DI.next_group && g < TM2_PORT_DROPIN_GROUPS; g++)
        if (DI.group_live[g] == 2)
            return g;
    return DI.next_group;
}

/* A model node's children, as dbscanHierarchy pushes them (and portFarLodKids: every frame of an animation, both
 * children of a zone, every child of a select, every record of an LOD). */
static int PORT_TEXT portDropInKids(unsigned char *node)
{
    int n;

    switch (node[0]) {
    case 1:
        return node[20];
    case 2:
        n = *(int *)(node + 16);
        return n < 0 ? 0 : n > 32 ? 32 : n;
    case 3:
        return node[11];
    case 4:
        return node[18];
    case 5:
        return node[38];
    case 9:
        return node[6];
    case 11:
        return 2;
    case 12:
        return *(unsigned short *)(node + 6);
    }
    return 0;
}

static unsigned char *PORT_TEXT portDropInKid(unsigned char *node, int i, unsigned char *lo, int size)
{
    unsigned char *r;

    switch (node[0]) {
    case 1:
        return *(unsigned char **)(node + 24 + i * 4);
    case 2:
        r = *(unsigned char **)(node + 20 + i * 4);
        return portDropInIn(r, lo, size) ? *(unsigned char **)(r + 8) : 0;
    case 3:
        return *(unsigned char **)(node + 32 + i * 4);
    case 4:
        return *(unsigned char **)(node + 20 + i * 4);
    case 5:
        return *(unsigned char **)(node + 40 + i * 4);
    case 9:
        return *(unsigned char **)(node + 16 + i * 4);
    case 11:
    case 12:
        return *(unsigned char **)(node + 8 + i * 4);
    }
    return 0;
}

/* The node with id `id` (u16 at +2: what worldLoadObject dispatches on) in an installed DMD copy (relocated in
 * place: assetInstallImage's roots are file words + base - 0x80019C40), 0 if none. */
static unsigned char *PORT_TEXT portDropInFindNode(unsigned char *base, int size, int id)
{
    unsigned int *d = (unsigned int *)base;
    unsigned char *stack[48], *node, *k;
    unsigned int delta, i, n;
    int sp, c, j, op;

    if (base == 0 || size < 32)
        return 0;
    delta = (unsigned int)base + 0x7FFE63C0u;
    n = d[6];
    for (i = 0; i < n && (7 + i) * 4 < (unsigned int)size && d[7 + i] != 0; i++) {
        sp = 0;
        stack[sp++] = (unsigned char *)(d[7 + i] + delta);
        while (sp > 0) {
            node = stack[--sp];
            if (!portDropInIn(node, base, size - 4) || ((unsigned int)node & 3))
                continue;
            op = node[0];
            if ((op <= 5 || op == 8 || op == 9 || op == 11 || op == 13) && *(unsigned short *)(node + 2) == id)
                return node;
            c = portDropInKids(node);
            for (j = 0; j < c && sp < 48; j++)
                if ((k = portDropInKid(node, j, base, size)) != 0)
                    stack[sp++] = k;
        }
    }
    return 0;
}

/* The same node in another loaded group's copy (the lowest group that has one; the car files of a type are one
 * file, so its node is the same model), 0 if none. */
static unsigned char *PORT_TEXT portDropInOtherNode(int g, int id)
{
    unsigned char *p;
    int h;

    for (h = 1; h < TM2_PORT_DROPIN_GROUPS; h++)
        if (h != g && DI.group_live[h] != 2 && DI.group_dmd[h] != 0 &&
            (p = portDropInFindNode(DI.group_dmd[h], DI.group_size[h], id)) != 0)
            return p;
    return 0;
}

/* A word that may hold a node of the copy [lo, lo + size): the same node of another loaded copy when `id` > 0 and
 * one has it (a model every car of the type shares: retail's "last install wins" global), else 0. */
static void PORT_TEXT portDropInRenode(int *word, unsigned char *lo, int size, int g, int id)
{
    if (!portDropInIn((void *)*word, lo, size))
        return;
    *word = id > 0 ? (int)portDropInOtherNode(g, id) : 0;
}

/* The checks of a live load before anything is done (portCarLoadGroupLive, portCarCanLoad): 0 or the error. */
static int PORT_TEXT portDropInLoadCheck(const unsigned int *t, int tms_size, const void *dmd, int dmd_size, int *np,
                                         int *typep, int *rootsp, int *keyp)
{
    int g, n, type, roots, key;

    if (!portDropInAllowed() || DI.next_group <= 0 || DI.dmd_next == 0)
        return TM2_DROPIN_ERR_BUSY;
    n = portDropInTmsCount(t, tms_size, TM2_TEX_SLOT_STRIDE);
    type = portDropInDmdType((const unsigned int *)dmd, dmd_size, &roots);
    if (n < 0 || type == 0)
        return TM2_DROPIN_ERR_BUFFER;
    g = portDropInFreeGroup();
    if (g >= TM2_PORT_DROPIN_GROUPS || (g + 1) * TM2_TEX_SLOT_STRIDE > TM2_PORT_LIVE_SLOT_BYTES / 20
        || portDropInSlotRun(roots) < 0
        || portDmdFind(dmd_size) == 0
        || portCsRoom() < DROPIN_CS_HEADROOM
        || animListCount + DROPIN_ANIM_HEADROOM > 100
        || clutAnimLastIndex + DROPIN_CLUTDEF_HEADROOM >= TM2_MAX_CLUT_ANIM_DEFS
        || bitmapRecordCount + n > TM2_PORT_LIVE_RECORDS
        || (key = portDropInNextKey(type)) == 0)
        return TM2_DROPIN_ERR_NOLOAD;
#ifndef TM2_PORT_DARK_TOOTH
    /* a second car of a type with a single special state (Twister, Axel, Mr. Slam, Shadow, Dark Tooth) would bind
       the live car's special objects to its own copy (weaponAssetBind, slamArmRegister, objRegistryAdd: the last
       install wins) unless the state is per key (TM2_PORT_CAR_SPECIALS; Dark Tooth: TM2_PORT_DARK_TOOTH) */
    if (key >= 0x100 && (type == 0xF0
#ifndef TM2_PORT_CAR_SPECIALS
                         || type == 0xA0 || type == 0xB0 || type == 0xC0 || type == 0xD0
#endif
                         ))
        return TM2_DROPIN_ERR_TYPE;
#endif
    *np = n;
    *typep = type;
    *rootsp = roots;
    *keyp = key;
    return 0;
}

/* B.7 / request 33 "load a joining player's car on demand": a new load group from buffers the port hands over
 * (G.15: no CD read), installed between frames; its car is parked and free (start it with portCarStartHuman /
 * portCarStartAi). The TMS is parsed where it is (read only, word aligned); the DMD is copied into the DMD area
 * (portCarDmdArea with portCarOptions bit 1, else retail's low area after the match's car DMDs): the first free
 * range that holds it (one an unload gave back), else the tail, and relocated there for the rest of the match.
 * The group number, slots and overflow key are the lowest free ones (given back by unloads, else new ones).
 *   textures: with texture banks live (bit 5) a fresh bank; else bank 0 placed by retail's scan in what the usage
 *             bitmap (now kept through the match) says is free, an image whose key is loaded already (the
 *             arena's, or the first car of the type) taken by key as at the load (portCarGroupNoReuse[group],
 *             which the port may set before the call, stops that); if any image or CLUT finds no room, the bitmap,
 *             the records and rows 240..255 go back to how they were and TM2_DROPIN_ERR_VRAM is returned (the
 *             pixels already uploaded lie in VRAM nobody uses); with bit 7 (per-car CLUTs) the group's copies
 *             (rows 240..255, then free VRAM);
 *   model:    assetInstallImage(copy, group) with the load-time resets left out (arenaResetSystems,
 *             carTablesClear, ...): the root registers a slot and a key (portCarRegisterRoot), the per-key
 *             registries fill, the specials of a duplicate key bind its own objects (TM2_PORT_CAR_SPECIALS); the
 *             world shift is put back after it; the key's freeze record is zeroed first, each new slot gets
 *             carChoiceRegisterGroup (arenaSetup's per-slot work) and is parked.
 * flags: reserved, 0. Returns the new group (its car's slot: portCarSlotOfGroup), or TM2_DROPIN_ERR_BUFFER (bad
 * TMS / DMD), TM2_DROPIN_ERR_NOLOAD (no group, slot, key, DMD space or pool room left), TM2_DROPIN_ERR_VRAM,
 * TM2_DROPIN_ERR_TYPE, TM2_DROPIN_ERR_BUSY (before a match load or in the attract demo). Between frames only. */
int PORT_TEXT portCarLoadGroupLive(const void *tms, int tms_size, const void *dmd, int dmd_size, int flags)
{
    const unsigned int *t = (const unsigned int *)tms;
    unsigned char *dst;
    int g, n, i, type, roots, key, before, records, shift, ok, bank, s, first;
    int *slot;

    (void)flags;
    i = portDropInLoadCheck(t, tms_size, dmd, dmd_size, &n, &type, &roots, &key);
    if (i != 0)
        return i;
    g = portDropInFreeGroup();
    first = portDropInSlotRun(roots);
    dst = portDmdFind(dmd_size);
    DI.live = 1;

    /* textures: a snapshot to go back to, the group's slots marked unwritten, the parse with tracking on */
    for (i = 0; i < TM2_PORT_LIVE_BITMAP_BYTES / 4; i++)
        ((int *)LT.backup)[i] = ((int *)LT.bitmap)[i];
    records = bitmapRecordCount;
#ifdef TM2_PORT_CLUT_ROWS
    for (i = 0; i < TM2_PORT_CLUT_ROWS_N * 2; i++)
        LT.rows[i] = ((unsigned int *)portCarCluts.rows)[i];
#endif
    for (i = 0; i < n; i++) {
        slot = &vramTexSlots[(g * TM2_TEX_SLOT_STRIDE + i) * 5];
        slot[1] = DROPIN_MISSING;
        slot[3] = DROPIN_MISSING;
    }
#ifdef TM2_PORT_TEXTURE_BANKS
    bank = portTexBankAlloc(g);     /* 0 when banks are not live */
#else
    bank = 0;
#endif
    UN.clut256[g] = 0;
    bitmapTrackingEnabled = 1;
    bitmapParseContainer(t, g);
    ok = 1;
    for (i = 0, s = 4 + n; i < n; i++) {
        slot = &vramTexSlots[(g * TM2_TEX_SLOT_STRIDE + i) * 5];
        /* entry i: its size word, then the container's 4 bytes before the TIM; TIM flags word bit 3 = CLUT */
        if (slot[1] == DROPIN_MISSING || (((t[s + 3] >> 3) & 1) && slot[3] == DROPIN_MISSING))
            ok = 0;
        s += 1 + (t[s] >> 2);
    }
    if (!ok) {
        for (i = 0; i < TM2_PORT_LIVE_BITMAP_BYTES / 4; i++)
            ((int *)LT.bitmap)[i] = ((int *)LT.backup)[i];
        bitmapRecordCount = records;
#ifdef TM2_PORT_CLUT_ROWS
        for (i = 0; i < TM2_PORT_CLUT_ROWS_N * 2; i++)
            ((unsigned int *)portCarCluts.rows)[i] = LT.rows[i];
#endif
#ifdef TM2_PORT_TEXTURE_BANKS
        if (bank != 0)
            portTexBankFree(g);
#endif
#ifdef TM2_PORT_TEX_SHARED
        portTexBanks.rec_first[g] = -1;     /* no images of g (portCarClutsPublish, portTexFindReuse) */
#endif
        bitmapDisableTracking();
        return TM2_DROPIN_ERR_VRAM;
    }
    UN.rec_first[g] = records;              /* its image records (one per image), for an unload */
    UN.rec_end[g] = bitmapRecordCount;
#ifdef TM2_PORT_CAR_CLUTS
    if (portCarCluts.live) {        /* bit 7: the group's own CLUT copies, rows 240..255 then free VRAM */
        int full[2];
        full[0] = 0;
        full[1] = 0;
        for (i = 0; i < 4; i++)
            portCarClutCopyPhase(g, i, 1, full);
        portCarClutsPublish();
    }
#endif

    /* model: copied into the area, installed as group g on the first free slots */
    for (i = 0; i < dmd_size; i++)
        dst[i] = ((const unsigned char *)dmd)[i];
    portDmdTake(dst, dmd_size);
    if ((unsigned int)(key >> 4) < TM2_CAR_KEYS)
        for (i = 0; i < 264; i++)
            portCarGroups.freeze[key >> 4][i] = 0;
    before = carWorldObjectCount;
    carWorldObjectCount = first;            /* carRegisterModel appends at the count: the free run's first slot */
    shift = assetInstallShift;
    assetInstallImage(dst, g);
    renderSetWorldShift(shift);
    assetInstallShift = shift;
    bitmapDisableTracking();
    if (carWorldObjectCount < before)
        carWorldObjectCount = before;
    for (s = 0; s < carWorldObjectCount && s < DROPIN_VACANT_SLOT; s++)
        if (portCarGroups.slot_group[s] == g) {
            carChoiceRegisterGroup(carSlotObjects[s], portCarKey(s, carSlotOwners[s]));
            portCarParkSlot(s);
        }
    DI.group_live[g] = 1;
    DI.group_dmd[g] = dst;
    DI.group_size[g] = dmd_size;
    if (g == DI.next_group)
        DI.next_group = g + 1;
    DI.loaded++;
    return g;
}

/* portCarCanLoad(tms, tms_size, dmd, dmd_size): would portCarLoadGroupLive load this car now? 0 = yes (VRAM aside:
 * without banks only the parse finds out, TM2_DROPIN_ERR_VRAM, and changes nothing), else the error it would return.
 * Changes nothing. */
int PORT_TEXT portCarCanLoad(const void *tms, int tms_size, const void *dmd, int dmd_size)
{
    int n, type, roots, key;

    return portDropInLoadCheck((const unsigned int *)tms, tms_size, dmd, dmd_size, &n, &type, &roots, &key);
}

/* portCarRoom(&room): what is free now (struct TM2PortCarRoom, car_dropin.h), for the port's join decisions.
 * Returns room->dmd_largest. Changes nothing; between frames. */
int PORT_TEXT portCarRoom(struct TM2PortCarRoom *room)
{
    unsigned int tail, m;
    int i, s, run, d, g, n;

    tail = DI.dmd_next != 0 && (unsigned int)DI.dmd_next < DI.dmd_end ? DI.dmd_end - (unsigned int)DI.dmd_next : 0;
    room->dmd_free = (int)tail;
    room->dmd_largest = (int)tail;
    room->dmd_holes = UN.range_count;
    for (i = 0; i < UN.range_count; i++) {
        room->dmd_free += (int)UN.range_size[i];
        if (UN.range_size[i] > (unsigned int)room->dmd_largest)
            room->dmd_largest = (int)UN.range_size[i];
    }
    room->slots_free = 0;
    room->slots_run = 0;
    for (s = 0, run = 0; s < DROPIN_VACANT_SLOT; s++) {
        if (s < carWorldObjectCount && portCarGroups.slot_group[s] != TM2_PORT_SLOT_FREE) {
            run = 0;
            continue;
        }
        room->slots_free++;
        if (++run > room->slots_run)
            room->slots_run = run;
    }
    for (d = 0, n = 0; d < portCarGroups.dup_count && d < TM2_CAR_KEYS - 16; d++)
        if (portCarGroups.key_type[16 + d] != 0)
            n++;
    room->keys_free = TM2_CAR_KEYS - 16 - n;
    room->groups_free = 0;
    room->groups_loaded = 0;
    for (g = 1; g < TM2_PORT_DROPIN_GROUPS; g++) {
        if (DI.next_group <= 0 || g >= DI.next_group || DI.group_live[g] == 2)
            room->groups_free++;
        else if (portCarSlotOfGroup(g) >= 0)
            room->groups_loaded++;
    }
    if (DI.next_group <= 0)
        room->groups_free = 0;
    room->cs_free = portCsRoom();
    room->anim_free = 100 - animListCount;
    room->clut_defs_free = TM2_MAX_CLUT_ANIM_DEFS - 1 - clutAnimLastIndex;
    if (room->clut_defs_free < 0)
        room->clut_defs_free = 0;
    room->records_free = TM2_PORT_LIVE_RECORDS - bitmapRecordCount;
    room->banks_free = 0;
#ifdef TM2_PORT_TEXTURE_BANKS
    if (portTexBanks.live)
        for (i = 1; i < TM2_PORT_TEX_BANKS; i++)
            if (portTexBanks.bank_group[i] == 0)
                room->banks_free++;
#endif
    room->rows16_free = 0;
    room->rows256_free = 0;
#ifdef TM2_PORT_CLUT_ROWS
    for (i = 0; i < TM2_PORT_CLUT_ROWS_N; i++) {
        for (d = 0; d < TM2_PORT_CLUT_ROW_UNITS; d++) {
            m = d < 32 ? portCarCluts.rows[i][0] >> d : portCarCluts.rows[i][1] >> (d - 32);
            if ((m & 1) == 0)
                room->rows16_free++;
        }
        for (d = 0; d < 2; d++)
            if ((portCarCluts.rows[i][0] & (0xFFFFu << (d * 16))) == 0)
                room->rows256_free++;
    }
#else
    (void)m;
#endif
    room->unloaded = UN.unloaded;
    return room->dmd_largest;
}

/* Builds without a per-key state for a special (TM2_PORT_CAR_SPECIALS, TM2_PORT_DARK_TOOTH): every car of the type
 * shares the type's single state, objects and nodes (the last install's), so a car of such a type can be unloaded
 * only while no other loaded car has its type. */
static int PORT_TEXT portDropInSingleShared(int g, int type)
{
    int s;

#ifndef TM2_PORT_CAR_SPECIALS
    if (type != 0xA0 && type != 0xB0 && type != 0xC0 && type != 0xD0 && type != 0xF0)
        return 0;
#elif !defined(TM2_PORT_DARK_TOOTH)
    if (type != 0xF0)
        return 0;
#else
    return 0;
#endif
    for (s = 0; s < carWorldObjectCount && s < DROPIN_VACANT_SLOT; s++)
        if (portCarGroups.slot_group[s] != g && portCarGroups.slot_group[s] != TM2_PORT_SLOT_FREE &&
            carSlotOwners[s] == type)
            return 1;
    return 0;
}

static int PORT_TEXT portDropInOwned(void *o, void **own, int n)
{
    int i;

    for (i = 0; i < n; i++)
        if (own[i] == o)
            return 1;
    return 0;
}

static void PORT_TEXT portDropInOwn(void *o, void **own, int *n)
{
    if (o == 0 || o == (void *)DI.dummy || portDropInOwned(o, own, *n))
        return;
    if (*n < UNLOAD_OWN)
        own[(*n)++] = o;
    else
        portCsGive(o);                  /* no room to note it: given back at once (no holder is left) */
}

/* A scene object of the retail special state (or of a duplicate's entry) that draws a node of the copy: owned by
 * the group, given back, the word cleared. */
static void PORT_TEXT portDropInOwnWord(void **word, unsigned char *lo, int size, void **own, int *n)
{
    if (*word != 0 && portDropInIn(UOBJ_MODEL(*word), lo, size)) {
        portDropInOwn(*word, own, n);
        *word = 0;
    }
}

#ifdef TM2_PORT_CAR_SPECIALS
/* A duplicate key's special entry d back to how portCarSpecialsReset leaves it (its objects given back by the
 * caller first). */
static void PORT_TEXT portDropInSpecialEntry(int d)
{
    int *p;
    int i;

    p = (int *)&portCarSpecials.slam[d];
    for (i = 0; i < (int)(sizeof(portCarSpecials.slam[d]) / 4); i++)
        p[i] = 0;
    portCarSpecials.slam[d].arm_angle = 11;
    portCarSpecials.slam[d].claw_angle = 11;
    p = (int *)&portCarSpecials.tornado[d];
    for (i = 0; i < (int)(sizeof(portCarSpecials.tornado[d]) / 4); i++)
        p[i] = 0;
    p = (int *)&portCarSpecials.strike[d];
    for (i = 0; i < (int)(sizeof(portCarSpecials.strike[d]) / 4); i++)
        p[i] = 0;
    portCarSpecials.axel_anim[d] = 0;
    for (i = 0; i < 15; i++)
        portCarSpecials.leap[d][i] = 0;
#ifdef TM2_PORT_DARK_TOOTH
    p = (int *)&portCarSpecials.dark_tooth[d];
    for (i = 0; i < (int)(sizeof(portCarSpecials.dark_tooth[d]) / 4); i++)
        p[i] = 0;
    portCarSpecials.dark_tooth[d].scalars[1] = 10;
    portCarSpecials.dark_tooth[d].scalars[3] = 10;
    portCarSpecials.dark_tooth[d].scalars[4] = 150;
    portCarSpecials.dark_tooth[d].scalars[5] = 100;
#endif
}
#endif

#ifdef TM2_PORT_SPECIAL_AS
/* Request 35's as-entries: a slot that borrows a special whose objects or nodes are gone (its lender unloaded:
 * portCarSpecialAsLoaded no longer holds) fires its own special again (as portCarSpecialAsSet(slot, -1, -1)); the
 * as-entry objects that draw a node of the copy, and those of the group's own slots, are given back and the entry
 * cleared (portCarSpecialAsSet makes new ones from the lender of the day). */
static void PORT_TEXT portDropInSpecialAs(int g, unsigned char *lo, int size)
{
    struct TM2Object *so, *ring;
    int s, mine, i;

    for (s = 0; s < TM2_CAR_SLOT_CAP; s++) {
        mine = s < carWorldObjectCount && portCarGroups.slot_group[s] == g;
        if (!mine && portCarSpecialAs[s] > 0 && !portCarSpecialAsLoaded(portCarSpecialAs[s])) {
            if (portCarSpecialAsSet(s, -1, -1) != TM2_SPECIAL_AS_OK) {
                portCarSpecialAsEnd(s);
                portCarSpecialAs[s] = -1;
            }
        }
        so = ((struct TM2Object **)portSpecialAs.strike[s].block)[24];   /* block +0x60 */
        if (mine || portDropInIn(portSpecialAs.strike[s].anim, lo, size) ||
            (so != 0 && portDropInIn(UOBJ_MODEL(so), lo, size))) {
            if (portCarSpecialAs[s] == 0xD0) {
                portCarSpecialAsEnd(s);
                portCarSpecialAs[s] = -1;
            }
            portCsGive(so);
            for (i = 0; i < 25; i++)
                portSpecialAs.strike[s].block[i] = 0;
            portSpecialAs.strike[s].anim = 0;
        }
        ring = (struct TM2Object *)portSpecialAs.axel[s].ring;
        if (mine || portDropInIn(portSpecialAs.axel[s].anim, lo, size) ||
            (ring != 0 && portDropInIn(UOBJ_MODEL(ring), lo, size))) {
            if (portCarSpecialAs[s] == 0xB0) {
                portCarSpecialAsEnd(s);
                portCarSpecialAs[s] = -1;
            }
            portCsGive(ring);
            portSpecialAs.axel[s].ring = 0;
            portSpecialAs.axel[s].anim = 0;
        }
    }
}
#endif

/* Every other object that draws a node of the copy: a projectile or ricochet shot of another car using a model of
 * this file (the shared model words below) takes the same node of another loaded copy, else a neutral model
 * (projectile: the homing missile's, the shot ended; ricochet: the plain ricochet's, the shot ended); anything else
 * (an object a second install of a type leaked: retail's "last install wins" without per-key specials) is the
 * group's and goes. */
static void PORT_TEXT portDropInSweepObjects(int g, unsigned char *lo, int size, void **own, int *n)
{
    void *o, *next;
    unsigned char *repl;
    int i, k;

    for (o = csDrawListHead; o != 0; o = next) {
        next = UOBJ_NEXT(o);            /* (portDropInOwn may give o back at once, which unlinks it) */
        if (!portDropInIn(UOBJ_MODEL(o), lo, size) || portDropInOwned(o, own, *n))
            continue;
        repl = portDropInOtherNode(g, *(unsigned short *)(UOBJ_MODEL(o) + 2));
        if (repl != 0) {
            UOBJ_MODEL(o) = repl;
            continue;
        }
        for (i = 0; i < TM2_MAX_PROJECTILES; i++)
            if ((void *)PROJ_OBJ(i) == o)
                break;
        if (i < TM2_MAX_PROJECTILES) {
            if (PROJ_LIFE(i) > 0) {
                fxRecordFree(i, 0);
                if (PROJ_TYPE(i) == 10 || PROJ_TYPE(i) == 11 || PROJ_TYPE(i) == 12 || PROJ_TYPE(i) == 15)
                    soundStopInstance(PROJ_TYPE(i), i + 1);
            }
            UOBJ_DRAW(o) = 0;
            UOBJ_MODEL(o) = (unsigned char *)weaponHomingMissileModel;
            continue;
        }
        for (k = 0; k < TM2_MAX_RICOCHETS; k++)
            if ((void *)RICO_SLOT(k) == o)
                break;
        if (k < TM2_MAX_RICOCHETS) {
            RICO_LIFE(k) = 0;
            UOBJ_DRAW(o) = 0;
            UOBJ_MODEL(o) = (unsigned char *)weaponRicochetModel;
            continue;
        }
        portDropInOwn(o, own, n);
    }
}

/* After the group's objects went back: every word that still names one of them (or a position inside one) across
 * frames is cleared, the state behind it ended: Thumper's
 * flame emitters, the energy shields' source car, a projectile's homing target and the boomerang's return object. */
static void PORT_TEXT portDropInForget(void **own, int n)
{
    int i, k;
    void *o;

    /* (a tornado this car fired was ended by portCarResetSpecials: its state's object word is read only while its
       timer runs) */
    for (i = 0; i < TM2_MAX_FLAME_EMITTERS; i++)
        if (portDropInOwned(EMITTER_MODEL(i), own, n)) {
            EMITTER_TIMER(i) = 0;
            EMITTER_MODEL(i) = 0;
        }
    for (i = 0; i < TM2_MAX_SHIELDS; i++)
        if (portDropInOwned(SHIELD_SOURCE(i), own, n)) {
            if (SHIELD_TIMER(i) > 0 && SHIELD_DST(i) != 0)
                SHIELD_DST(i)->visible = 0;
            SHIELD_TIMER(i) = 0;
            SHIELD_SOURCE(i) = 0;
        }
    for (i = 0; i < TM2_MAX_PROJECTILES; i++) {
        for (k = 0; k < n; k++) {
            o = own[k];
            if (PROJ_TARGET(i) == ((struct TM2Object *)o)->pos)
                PROJ_TARGET(i) = 0;
        }
        if (portDropInOwned(PROJ_VICTIM(i), own, n)) {
            if (PROJ_LIFE(i) > 0)
                fxRecordFree(i, 0);
            PROJ_VICTIM(i) = 0;
        }
    }
}

/* clutAnimRegister's definitions of id `id` (a car key's +8 fade while banks or CLUT copies are live) removed: a
 * running animation of one ends, the last definition moves into the hole (the running animations that use it
 * follow it). */
static void PORT_TEXT portDropInClutDefs(int id)
{
    int i, a, last;

    if (clutAnimLastIndex >= TM2_MAX_CLUT_ANIM_DEFS)
        clutAnimLastIndex = TM2_MAX_CLUT_ANIM_DEFS - 1;    /* clutAnimNewDef counts past a full table */
    for (i = 0; i <= clutAnimLastIndex;) {
        if (*(unsigned short *)clutAnimDefs[i] != (unsigned short)id) {
            i++;
            continue;
        }
        for (a = 0; a < TM2_MAX_CLUT_ANIMS; a++)
            if (clutAnimRunning[a][0] != 0 && clutAnimRunning[a][0x20 / 4] == (int)clutAnimDefs[i])
                clutAnimRunning[a][0] = 0;
        last = clutAnimLastIndex;
        if (i != last) {
            for (a = 0; a < 5; a++)
                clutAnimDefs[i][a] = clutAnimDefs[last][a];
            for (a = 0; a < TM2_MAX_CLUT_ANIMS; a++)
                if (clutAnimRunning[a][0x20 / 4] == (int)clutAnimDefs[last])
                    clutAnimRunning[a][0x20 / 4] = (int)clutAnimDefs[i];
        }
        clutAnimLastIndex--;
    }
}

/* The group's image records (one per image of its TMS, [rec_first, rec_end)) out of the table: all of them for a
 * banked group (its bank goes) and, with TM2_PORT_TEX_SHARED, for a portCarGroupNoReuse one; for a group in bank 0
 * only those whose key another usable record has too, so an image of a type keeps one record and a later car of the
 * type takes its pixels by key as at the load (the VRAM of bank 0 is never given back: a cache per key). The table
 * closes up; every group's range follows. */
static void PORT_TEXT portDropInRecords(int g)
{
    unsigned int mask;
    int first, end, r, q, h, w, all, cnt, x;
    int *rec;

    first = UN.rec_first[g];
    end = UN.rec_end[g];
    UN.rec_first[g] = -1;
    UN.rec_end[g] = -1;
    if (first < 0 || end <= first || end > bitmapRecordCount)
        return;
    if (end - first > 32)
        end = first + 32;
    all = 0;
#ifdef TM2_PORT_TEXTURE_BANKS
    all = portTexBanks.group_bank[g] != 0;
#endif
#ifdef TM2_PORT_TEX_SHARED
    all = all || portCarGroupNoReuse[g];
#endif
    mask = 0;
    for (r = first; r < end; r++) {
        rec = &bitmapRecords[r * 6];
        if (!all) {
            for (q = 0; q < bitmapRecordCount; q++) {
                if ((q >= first && q < end) || bitmapRecords[q * 6] != rec[0])
                    continue;
#ifdef TM2_PORT_TEX_SHARED
                for (h = 1; h < TM2_PORT_DROPIN_GROUPS; h++)
                    if (h != g && portCarGroupNoReuse[h] && UN.rec_first[h] >= 0 && q >= UN.rec_first[h] &&
                        q < UN.rec_end[h])
                        break;
                if (h < TM2_PORT_DROPIN_GROUPS)
                    continue;
#endif
                break;
            }
            if (q >= bitmapRecordCount)
                continue;                   /* the only record of its key: kept (no group's) */
        }
        mask |= 1u << (r - first);
    }
    if (mask == 0)
        return;
    for (r = w = first; r < bitmapRecordCount; r++) {
        if (r < end && ((mask >> (r - first)) & 1))
            continue;
        if (w != r)
            for (q = 0; q < 6; q++)
                bitmapRecords[w * 6 + q] = bitmapRecords[r * 6 + q];
        w++;
    }
    bitmapRecordCount = w;
    /* every range after the removed records moves down by the records removed before it */
    for (h = 0; h < 2 * TM2_PORT_DROPIN_GROUPS; h++) {
        int *p = h < TM2_PORT_DROPIN_GROUPS ? &UN.rec_first[h] : &UN.rec_end[h - TM2_PORT_DROPIN_GROUPS];

        x = *p;
        if (x <= first)
            continue;
        for (cnt = 0, r = first; r < end && r < x; r++)
            cnt += (mask >> (r - first)) & 1;
        *p = x - cnt;
    }
#ifdef TM2_PORT_TEX_SHARED
    for (h = 0; h < 2 * TM2_PORT_TEX_GROUPS; h++) {
        short *p = h < TM2_PORT_TEX_GROUPS ? &portTexBanks.rec_first[h] : &portTexBanks.rec_end[h - TM2_PORT_TEX_GROUPS];

        x = *p;
        if (x <= first)
            continue;
        for (cnt = 0, r = first; r < end && r < x; r++)
            cnt += (mask >> (r - first)) & 1;
        *p = (short)(x - cnt);
    }
#endif
}

#ifdef TM2_PORT_CAR_CLUTS
/* Request 33-1: the group's CLUT copies given back (a spot in rows 240..255 to the allocator, one in VRAM proper to
 * the usage bitmap) and its rows of the table cleared; the sharing flags of the others published again. */
static void PORT_TEXT portDropInClutCopies(int g)
{
    int i, k, x, y, w, u, bit;

    for (i = 0; i < TM2_PORT_CLUT_IMAGES; i++) {
        if (portCarCluts.state[g][i] == TM2_PORT_CLUT_COPY) {
            x = (portCarCluts.clut[g][i] & 0x3F) << 4;
            y = (portCarCluts.clut[g][i] >> 6) & 0x3FF;
            w = ((UN.clut256[g] >> i) & 1) ? 256 : 16;
            if (y >= TM2_PORT_CLUT_ROW_FIRST && y < TM2_PORT_CLUT_ROW_FIRST + TM2_PORT_CLUT_ROWS_N &&
                x < TM2_PORT_CLUT_ROW_UNITS * 16) {
                u = x >> 4;
                if (w == 256)
                    portCarCluts.rows[y - TM2_PORT_CLUT_ROW_FIRST][0] &= ~(0xFFFFu << (u & 16));
                else if (u < 32)
                    portCarCluts.rows[y - TM2_PORT_CLUT_ROW_FIRST][0] &= ~(1u << u);
                else
                    portCarCluts.rows[y - TM2_PORT_CLUT_ROW_FIRST][1] &= ~(1u << (u - 32));
            } else {
                for (k = 0; k < w && x + k < 1024; k++) {
                    bit = (y << 10) + x + k;
                    bitmapWords[bit >> 5] &= ~(1 << (bit & 31));
                }
            }
        }
        portCarCluts.state[g][i] = TM2_PORT_CLUT_NONE;
        portCarCluts.clut[g][i] = 0;
    }
    portCarCluts.pend16[g] = 0;
    portCarCluts.pend256[g] = 0;
    portCarCluts.arena[g] = 0;
    portCarCluts.shared[g] = 0;
    UN.clut256[g] = 0;
    if (portCarCluts.live)
        portCarClutsPublish();
}
#endif

/* Request 33 (online with bots, the port's request 37 answer 14), CAR_GROUPS.md 14.7: give load group `group` back
 * for good, between frames: a group no car uses (every slot of it without a driver: a removed car, a spare, a car of
 * the match start nobody drives). In order:
 *   1. each car of it: specials, look and sound ended (portCarResetSpecials, portCarResetLook, its sound instances),
 *      parked, kill credits and slot references cleared (portCarRekeyReferences(slot, -1));
 *   2. its scene objects (the cars, the single special objects its install bound: retail's words for the first car
 *      of a type, a duplicate key's entry, which goes back to its initial state) and the words that keep a node of
 *      its DMD copy (the shared projectile models: the same node of another loaded copy, else 0; the special nodes
 *      2324 / 2325 / Mr. Slam's arm and claw; worldLoadParentObject); request 35's as-entries that borrow from it;
 *      every other object drawing its copy (projectiles and ricochets of other cars: another copy's node, else a
 *      neutral model and the shot ended; leaked objects: given back); every word naming a given-back object;
 *   3. the lists that point into its copy: the animation list (order kept), the world models, the far-LOD records;
 *   4. textures: its +8 fade definitions (banks / CLUT copies live), its CLUT copies, its image records, its bank;
 *   5. its key's registries (body, wreck, damage stage, tires, turbo, parts, freeze record) zeroed and the key free
 *      (an overflow key: key_type 0), its slots free (the dummy object, type 0, kind 2, group TM2_PORT_SLOT_FREE;
 *      carWorldObjectCount drops past free slots at the end), its group number free (group_live 2), its DMD bytes
 *      back to the area's free list, every portCarBindGroup entry naming it cleared.
 * Returns 1, or TM2_DROPIN_ERR_GROUP (no such group, or unloaded), TM2_DROPIN_ERR_BUSY (a record drives one of its
 * cars; attract demo), TM2_DROPIN_ERR_TYPE (a build without per-key specials: another loaded car of its type shares
 * its single special state), TM2_DROPIN_ERR_SHARED (another group has its key). */
int PORT_TEXT portCarUnloadGroup(int group)
{
    void *own[UNLOAD_OWN];
    unsigned char *lo;
    int s, h, k, i, key, type, size, n, any;

    if (!portDropInAllowed())
        return TM2_DROPIN_ERR_BUSY;
    if ((unsigned int)group >= TM2_PORT_DROPIN_GROUPS || group == 0 || DI.group_live[group] == 2 ||
        DI.group_dmd[group] == 0)
        return TM2_DROPIN_ERR_GROUP;
    any = 0;
    type = 0;
    for (s = 0; s < carWorldObjectCount && s < DROPIN_VACANT_SLOT; s++) {
        if (portCarGroups.slot_group[s] != group)
            continue;
        if (carSlotKinds[s] != 2)
            return TM2_DROPIN_ERR_BUSY;
        any = 1;
        type = carSlotOwners[s];
    }
    if (!any)
        return TM2_DROPIN_ERR_GROUP;
    key = portCarGroups.group_key[group];
    for (h = 0; h < TM2_CAR_SLOT_CAP; h++)
        if (h != group && key != 0 && portCarGroups.group_key[h] == key)
            return TM2_DROPIN_ERR_SHARED;
    if (portDropInSingleShared(group, type))
        return TM2_DROPIN_ERR_TYPE;
    DI.live = 1;
    lo = DI.group_dmd[group];
    size = DI.group_size[group];
    n = 0;

    /* 1. the cars */
    for (s = 0; s < carWorldObjectCount && s < DROPIN_VACANT_SLOT; s++)
        if (portCarGroups.slot_group[s] == group) {
            portCarResetSpecials(s);
            portCarResetLook(s);
            portDropInSlotSound(s);
            portCarParkSlot(s);
            portCarRekeyReferences(s, -1);
            portDropInOwn(carSlotObjects[s], own, &n);
        }

    /* 2. the objects and nodes of its install: the single special state is read through the key's selection (a
       duplicate's entry is then in retail's words; the first car's state is there anyway), its words that draw or
       are a node of the copy given back / cleared, then the selection restored and a duplicate's entry reset */
#ifdef TM2_PORT_CAR_SPECIALS
    i = portCarSpecialSelect(key);
#endif
    portDropInOwnWord((void **)&weaponTornadoObject, lo, size, own, &n);
    portDropInOwnWord((void **)&weaponTornadoFunnelObject, lo, size, own, &n);
    portDropInOwnWord((void **)&weaponStrikeState.object, lo, size, own, &n);
    portDropInOwnWord((void **)&darkToothBossObject, lo, size, own, &n);
    portDropInRenode((int *)&objRegistryId2324, lo, size, group, 0);
    portDropInRenode((int *)&objRegistryId2325, lo, size, group, 0);
    portDropInRenode((int *)&slamArmObject, lo, size, group, 0);
    portDropInRenode((int *)&slamClawObject, lo, size, group, 0);
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(i);
    if (key >= 0x100 && (k = (key >> 4) - 16) < TM2_PORT_SPECIAL_KEYS)
        portDropInSpecialEntry(k);
#endif
    portDropInRenode(&weaponSpiralMissileModel, lo, size, group, 0x8FD);
    portDropInRenode(&weaponBoomerangModel, lo, size, group, 0x911);
    portDropInRenode(&weaponGrimmSpecialModel, lo, size, group, 0x912);
    portDropInRenode(&weaponGhostMissileModel, lo, size, group, 0x916);
    portDropInRenode(&weaponRicochetModelAlt, lo, size, group, 0x8A6);
    portDropInRenode((int *)&worldLoadParentObject, lo, size, group, 0);
#ifdef TM2_PORT_SPECIAL_AS
    portDropInSpecialAs(group, lo, size);
#endif
    portDropInSweepObjects(group, lo, size, own, &n);
    for (i = 0; i < n; i++)
        portCsGive(own[i]);
    portDropInForget(own, n);

    /* 3. the lists that point into the copy */
    for (i = k = 0; i < animListCount && i < 100; i++)
        if (!portDropInIn(animNodeList[i], lo, size))
            animNodeList[k++] = animNodeList[i];
    for (i = k; i < animListCount && i < 100; i++)
        animNodeList[i] = 0;
    if (animListCount > k)
        animListCount = k;
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++)
        if (portDropInIn(renderWorldModels[i], lo, size))
            renderWorldModels[i] = 0;
#ifdef TM2_PORT_FAR_LOD
    for (i = 0; i < portFarLodTable.count;)
        if (portDropInIn((void *)((unsigned int)portFarLodTable.records[i] & ~1u), lo, size)) {
            k = --portFarLodTable.count;
            portFarLodTable.records[i] = portFarLodTable.records[k];
            portFarLodTable.maxes[i] = portFarLodTable.maxes[k];
        } else {
            i++;
        }
    for (i = 0; i < portFarLodNear.count;)      /* request 46 item 6 */
        if (portDropInIn((void *)((unsigned int)portFarLodNear.records[i] & ~1u), lo, size)) {
            k = --portFarLodNear.count;
            portFarLodNear.records[i] = portFarLodNear.records[k];
            portFarLodNear.maxes[i] = portFarLodNear.maxes[k];
            portFarLodNear.mins[i] = portFarLodNear.mins[k];
        } else {
            i++;
        }
#endif

    /* 4. textures */
#ifdef TM2_PORT_TEX_SHARED
#ifdef TM2_PORT_CAR_CLUTS
    if (portTexBanks.live || portCarCluts.live)
#else
    if (portTexBanks.live)
#endif
        portDropInClutDefs(key + 8);
#endif
#ifdef TM2_PORT_CAR_CLUTS
    portDropInClutCopies(group);
#endif
    portDropInRecords(group);
#ifdef TM2_PORT_TEXTURE_BANKS
    portTexBankFree(group);
#endif
#ifdef TM2_PORT_TEX_SHARED
    portTexBanks.rec_first[group] = -1;
    portTexBanks.rec_end[group] = -1;
    portTexBanks.body_clut[group] = 0;
    portTexBanks.body_bank[group] = 0;
#endif

    /* 5. key, slots, group number, DMD bytes */
    k = key >> 4;
    if (key != 0 && (unsigned int)k < TM2_CAR_KEYS) {
        portCarGroups.body[k] = 0;
        portCarGroups.wreck[k] = 0;
        portCarGroups.damage_stage[k] = 0;
        portCarGroups.turbo[k] = 0;
        for (i = 0; i < 3; i++)
            portCarGroups.tire[k][i] = 0;
        for (i = 0; i < 6; i++)
            portCarGroups.parts[k][i] = 0;
        for (i = 0; i < 264; i++)
            portCarGroups.freeze[k][i] = 0;
        if (k >= 16)
            portCarGroups.key_type[k] = 0;
    }
    portCarGroups.group_key[group] = 0;
    DROPIN_OBJ(DI.dummy)->visible = 0;
    DROPIN_OBJ(DI.dummy)->pos[2] = PORT_FAR_AWAY;
    for (s = 0; s < carWorldObjectCount && s < DROPIN_VACANT_SLOT; s++)
        if (portCarGroups.slot_group[s] == group) {
            carSlotObjects[s] = DI.dummy;
            carSlotOwners[s] = 0;
            carSlotKinds[s] = 2;
            carSlotIndices[s] = -1;
            portCarGroups.slot_group[s] = TM2_PORT_SLOT_FREE;
            portCarGroups.slot_key[s] = 0;
#ifdef TM2_PORT_SPECIAL_AS
            portCarSpecialAs[s] = -1;
#endif
            portBalanceRideTop[s] = 0;
            portBalanceCars[s].frozen_at = 0;    /* round 3: no freeze chain or gun half point for the next car */
            portBalanceCars[s].freeze_len = 0;
            portBalanceCars[s].gun_half = 0;
            portBalanceCars[s].guns_refused = 0;
            portBalanceJam[s].stops = 0;         /* request 91: no jam count or way round for the next car */
            portBalanceJam[s].avoid_until = 0;
        }
    while (carWorldObjectCount > 0 && portCarGroups.slot_group[carWorldObjectCount - 1] == TM2_PORT_SLOT_FREE)
        carWorldObjectCount--;
    for (i = 0; i < TM2_PORT_BIND_ENTRIES; i++)
        if (portCarBindGroup[i] == group)
            portCarBindGroup[i] = 0;
    portDmdGive(lo, size);
    DI.group_dmd[group] = 0;
#ifdef TM2_PORT_ROLLBACK
    portDmdImages.base[group] = 0;      /* request 54: its model file is no longer listed (portDmdMutableList) */
#endif
    DI.group_size[group] = 0;
    DI.group_live[group] = 2;
    UN.unloaded++;
    return 1;
}
#endif /* TM2_PORT_CAR_DROPIN: the drop-in only part */
#undef UOBJ_DRAW
#undef UOBJ_MODEL
#undef UOBJ_NEXT
#undef PROJ_OBJ
#undef RICO_SLOT
#undef CS_WORDS
#undef UNLOAD_OWN
#undef UN
#undef LT
#undef DI
#endif /* TM2_PORT_CAR_DROPIN || TM2_HUMAN_HANDOVER */

#if TM2_SPU_VOICES > 24
/* Request 34 (docs/SPU_VOICES.md): 48 SPU voices. The libraries' per-voice tables grow to TM2_SPU_VOICES and
 * move here (tsv group spuvoices+, linear: voice v keeps its retail entry for v < 24); the bit words of voices
 * 24..47 are portSpuExt (spu_voices.h). Layouts as the units read them. */
int D_801A5398[TM2_SPU_VOICES][0x34 / 4] PORT_BSS = {{0}};     /* libsnd _svm_voice (52-byte voice records) */
int D_801A5200[TM2_SPU_VOICES][0x10 / 4] PORT_BSS = {{0}};     /* libsnd _svm_sreg_buf (register shadows) */
unsigned char D_801A5380[TM2_SPU_VOICES] PORT_BSS = {0};       /* libsnd _svm_sreg_dirty */
struct TM2PortSpuExt portSpuExt PORT_BSS = {{0}};
/* the game's driver: voice taken (retail .data, zero). Right after portSpuExt on purpose: retail's
 * soundVoiceFree(-1) (an instance whose SsUtKeyOnV failed, keyed off later) stores 0 at [-1], in retail the unused
 * word after soundOverflowInsts[28]; here portSpuExt.envx_hist[15], which nothing reads (SpuVmFlush ANDs 0..14) */
int soundVoiceInUse[TM2_SPU_VOICES] PORT_BSS = {0};
/* libspu _spu_voice_centerNote: retail .data, 0xC000 for every voice (centre note 0xC0, fine 0) */
unsigned short D_8017E15C[TM2_SPU_VOICES] PORT_DATA = {
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
};
#endif /* TM2_SPU_VOICES > 24 */

/* ---- Human object ids (docs/HUMANS16_PLAN.md 1.4, step H0; every port variant) ---- */
#if TM2_MAX_HUMANS > TM2_PORT_BIND_HUMANS
#error "portCarBindGroup holds TM2_PORT_BIND_HUMANS human entries (car_groups.h)"
#endif
/* Step H4 (HUMANS16_PLAN.md): the human storage holds every record (tools/check_humans.py check 9 reads these back). */
#if TM2_PORT_BIND_HUMANS != 16 || TM2_PORT_BIND_ENTRIES - TM2_PORT_BIND_HUMANS < TM2_MAX_AI_CARS
#error "portCarBindGroup: exactly 16 human entries, then one per AI record (car_groups.h)"
#endif
#if TM2_HUMAN_ID(TM2_MAX_HUMANS - 1) >= 50 || TM2_MAX_HUMANS > TM2_PORT_SOUND_SLOTS
#error "a human object id or sound slot out of range"
#endif
typedef char tm2_h4_human_records[(sizeof(humanCars) == TM2_MAX_HUMANS * 0x814) ? 1 : -1];
typedef char tm2_h4_human_words[(sizeof(humanCarObjects) == 4 * TM2_MAX_HUMANS && sizeof(humanCarObjectSlots) ==
    4 * TM2_MAX_HUMANS && sizeof(padConfigs) == 4 * TM2_MAX_HUMANS && sizeof(humanCarTypes) == 4 * TM2_MAX_HUMANS &&
    sizeof(hudSmoothedSpeeds) == 4 * TM2_MAX_HUMANS && sizeof(carViewHotkeyReleased) == 4 * TM2_MAX_HUMANS) ? 1 : -1];
typedef char tm2_h4_human_map[(sizeof(portHumanLocal) == TM2_MAX_HUMANS && sizeof(portHumanPad) == 2 * TM2_MAX_HUMANS
    && sizeof(portHumanSteer) == 2 * TM2_MAX_HUMANS && sizeof(portHumanThrottle) == 2 * TM2_MAX_HUMANS
    && sizeof(portHumanLayout) == TM2_MAX_HUMANS &&
    sizeof(portHumanLives) == TM2_MAX_HUMANS && sizeof(portHumanBindings) == 128 * TM2_MAX_HUMANS) ? 1 : -1];
typedef char tm2_h4_bind_group[(sizeof(portCarBindGroup) >= TM2_MAX_HUMANS + TM2_MAX_AI_CARS) ? 1 : -1];
#if TM2_MAX_HUMANS > TM2_VARIANT_HUMANS
typedef char tm2_h4_reset_weapons[(sizeof(carResetWeapons) == TM2_MAX_HUMANS) ? 1 : -1];
#endif
#if TM2_MAX_HUMANS > 2
typedef char tm2_h4_human_cars[(sizeof(sessionPortHumanCars) == TM2_MAX_HUMANS - 2) ? 1 : -1];
#endif
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
extern int humanCarTotal;

/* The human record index of object id `id` (players.h TM2_HUMAN_ID: 1..7, then 9, 11.. past 7 humans), or -1
 * when no present human has that id: 0, the world root 8, the missile objects' 10, AI ids 50.., ids of track
 * vehicles and targets, and the ids of indices >= humanCarTotal (a human who has left). Pure: reads
 * humanCarTotal only. For the port's kill feed and anything else that turns an id into a player. */
int PORT_TEXT portHumanOfId(int id)
{
    int i;

    if (id <= 0)
        return -1;
    i = TM2_HUMAN_INDEX_OF_ID(id);
    if (i < 0 || i >= TM2_MAX_HUMANS || i >= humanCarTotal || TM2_HUMAN_ID(i) != id)
        return -1;
    return i;
}

#ifdef TM2_PORT_HUMANS
/* ---- the local-player map (docs/HUMANS16_PLAN.md 2.2, step H1; port_humans.h) ---- */
extern void hudRadarPlotCar(int type, int *pos, int angle, int idx);

/* Human h's local slot (view, camera, HUD, radar) on this machine, or -1 (remote, or h out of range). The default
 * entry 0 is retail's identity for h < TM2_MAX_VIEWS. No presence test: carBindHumanSlot sets the view of a human
 * before the match counts it. Pure. */
int PORT_TEXT portHumanSlot(int h)
{
    int v;

    if ((unsigned int)h >= TM2_MAX_HUMANS)
        return -1;
    v = portHumanLocal[h];
    if (v == 0)
        return h < TM2_MAX_VIEWS ? h : -1;
    if (v > 0 && v <= TM2_MAX_VIEWS)
        return v - 1;
    return -1;
}

/* The human on local slot L: the first present human (index < humanCarTotal) whose slot is L, or -1 (no human
 * there: the HUD draws nothing). Pure. */
int PORT_TEXT portLocalHuman(int L)
{
    int h;

    if ((unsigned int)L >= TM2_MAX_VIEWS)
        return -1;
    for (h = 0; h < humanCarTotal && h < TM2_MAX_HUMANS; h++)
        if (portHumanSlot(h) == L)
            return h;
    return -1;
}

/* How many of the slots 0..TM2_MAX_VIEWS-1 have a present human: the views and HUDs this machine shows (the port
 * picks the split mode and its own views 3-4 from it). Pure. */
int PORT_TEXT portLocalHumanCount(void)
{
    int L, n;

    n = 0;
    for (L = 0; L < TM2_MAX_VIEWS; L++)
        if (portLocalHuman(L) >= 0)
            n++;
    return n;
}

/* worldUpdate's radar plots by human: the blip goes on human h's radar, which is its slot's; a remote human has
 * no radar here. The caller has drawn its rand() already (the same on every machine). */
void PORT_TEXT portHumanRadarPlot(void *type, int *delta, int angle, int h)
{
    int L = portHumanSlot(h);

    if (L >= 0)
        hudRadarPlotCar((int)type, delta, angle, L);
}

/* ---- the humans' inputs (docs/HUMANS16_PLAN.md 2.4, step H2; port_humans.h) ---- */
extern unsigned char padCheckAction(int action, int port);
extern int padComboHeld(short combo, short port);
extern int padAnyLatched(int port);
extern unsigned char padDefaultBindings[22][2];     /* {action, button}: padLoadDefaultBindings' table */
extern unsigned char padLayoutTable[43][5];         /* {action, button in layout 0..3}: padLoadLayout's table */
extern int humanCarNearest(int *pos, int exclude);

/* The pad port human h reads on this machine: its local slot when that is 0 or 1 and portHumanOptions bit 0 is
 * clear (retail: humans 0 and 1 on pads 0 and 1), else -1 (h reads its word portHumanPad[h]). */
static int PORT_TEXT portHumanPadPort(int h)
{
    int L;

    if (portHumanOptions & TM2_HUMAN_OPT_INPUT_ALL)
        return -1;
    L = portHumanSlot(h);
    return L == 0 || L == 1 ? L : -1;
}

/* padButtonHeld's test of button 1..14 (select, start, L1, L2, up, down, left, right, R1, R2, triangle, cross,
 * square, circle) on one 16-bit word of padButtonsDecoded's half layout; other buttons (0 = unbound) are 0. Pure. */
int PORT_TEXT portPadButtonInWord(int button, unsigned int word)
{
    switch (button) {
    case 1: return (word >> 8) & 1;
    case 2: return (word >> 11) & 1;
    case 3: return (word >> 2) & 1;
    case 4: return word & 1;
    case 5: return (word >> 12) & 1;
    case 6: return (word >> 14) & 1;
    case 7: return (word >> 15) & 1;
    case 8: return (word >> 13) & 1;
    case 9: return (word >> 3) & 1;
    case 10: return (word >> 1) & 1;
    case 11: return (word >> 4) & 1;
    case 12: return (word >> 6) & 1;
    case 13: return (word >> 7) & 1;
    case 14: return (word >> 5) & 1;
    }
    return 0;
}

/* padCheckAction(action, port) for human h (humanCarReadInput, carReadMoveCodes): on pad 0/1 the retail call;
 * else action 6 = any button of the word, other actions the button of h's binding table. Pure. */
unsigned char PORT_TEXT portHumanAction(int action, int h)
{
    int port = portHumanPadPort(h);
    unsigned int word;

    action &= 0xFF;     /* retail narrows the action to u8 */
    if (port >= 0)
        return padCheckAction(action, port);
    if ((unsigned int)h >= TM2_MAX_HUMANS)
        return 0;
    word = portHumanPad[h];
    if (action == 6)
        return word != 0;
    if (action >= TM2_HUMAN_BINDINGS)
        return 0;
    return (unsigned char)portPadButtonInWord(portHumanBindings[h][action], word);
}

/* padComboHeld(combo, port) for human h (humanCarTick's cheat combos, carViewHotkeys' combo 1): on pad 0/1 the
 * retail call (pad 0 compares the whole 32-bit latch, pad 1 its half); else exactly one of the nine button sets
 * held on the 16-bit word (retail's pad-0 answer while the other pad is idle). Pure. */
int PORT_TEXT portHumanCombo(int combo, int h)
{
    int port = portHumanPadPort(h);
    unsigned int word;

    if (port >= 0)
        return padComboHeld((short)combo, (short)port);
    if ((unsigned int)h >= TM2_MAX_HUMANS)
        return 0;
    word = portHumanPad[h];
    switch ((short)combo) {
    case 1: return word == 0x180B;
    case 2: return word == 0x30C9;
    case 3: return word == 0xC036;
    case 4: return word == 0xC035;
    case 5: return word == 0x30CA;
    case 6: return word == 0x3000;
    case 7: return word == 0x00C0;
    case 8: return word == 0x30C0;
    case 9: return word == 0xC030;
    }
    return 0;
}

/* padAnyLatched(port) for human h (humanCarTick's cheat release): on pad 0/1 the retail call; else any button of
 * the word. Pure. */
int PORT_TEXT portHumanAnyLatched(int h)
{
    int port = portHumanPadPort(h);

    if (port >= 0)
        return padAnyLatched(port);
    if ((unsigned int)h >= TM2_MAX_HUMANS)
        return 0;
    return portHumanPad[h] != 0;
}

/* Human h's control layout (0..3; others ignored, as padSetLayout) and its binding table, built as padInit and
 * padSetLayout build a pad's active table: cleared, padDefaultBindings' 22 pairs, then the layout's 43 rows of
 * padLayoutTable. The port may overwrite single entries of portHumanBindings[h] afterwards (its own buttons). */
void PORT_TEXT portHumanSetLayout(int h, int layout)
{
    unsigned char *b;
    int i;

    if ((unsigned int)h >= TM2_MAX_HUMANS || (unsigned int)layout >= 4)
        return;
    portHumanLayout[h] = layout;
    b = portHumanBindings[h];
    for (i = 0; i < TM2_HUMAN_BINDINGS; i++)
        b[i] = 0;
    for (i = 0; i < 22; i++)
        if (padDefaultBindings[i][0] < TM2_HUMAN_BINDINGS)
            b[padDefaultBindings[i][0]] = padDefaultBindings[i][1];
    for (i = 0; i < 43; i++)
        if (padLayoutTable[i][0] < TM2_HUMAN_BINDINGS)
            b[padLayoutTable[i][0]] = padLayoutTable[i][layout + 1];
}

/* sessionRun, at the match start (after retail's padSetLayout of pads 0/1): every human's bindings from the
 * layout the port filled in portHumanLayout. */
void PORT_TEXT portHumanSetLayouts(void)
{
    int h;

    for (h = 0; h < TM2_MAX_HUMANS; h++)
        portHumanSetLayout(h, portHumanLayout[h]);
}

/* portHumanOptions bit 3: the cheat combos and codes are off (online whenever a human is remote). */
int PORT_TEXT portHumanCheatsOff(void)
{
    return (portHumanOptions & TM2_HUMAN_OPT_NO_CHEATS) != 0;
}

/* carReadMoveCodes: a completed code that is one of the three cheat codes (Mega Napalm 0x12233344, health
 * 0x21431122, god mode / infinite weapons / Mega Guns 0x12344321) while the cheats are off. */
int PORT_TEXT portHumanCheatCode(int code)
{
    if (!portHumanCheatsOff())
        return 0;
    return code == 0x12233344 || code == 0x21431122 || code == 0x12344321;
}

/* weaponFindTargetAhead / Behind for an AI owner outside the attract demo: retail aims at human 0's object.
 * portHumanOptions bit 2 (AI_AIM): at the bot's own target, aiGetTargetCar's car (an AI's object, or the nearest
 * human's); 0 (no target: the shot goes straight) when there is none. Pure, no rand(). */
int *PORT_TEXT portAiAimTarget(int owner)
{
    unsigned char *obj;
    int t;

    if (!(portHumanOptions & TM2_HUMAN_OPT_AI_AIM)) {
        if (humanCarTotal <= 0 || !humanCarObjects[0])
            return 0;   /* H5: no human 0 (a server without a player): no target, the shot goes straight */
        return (int *)((unsigned char *)humanCarObjects[0] + 0xA8);   /* the object's position (+0xA8) */
    }
    obj = 0;
    if ((unsigned int)owner < TM2_MAX_AI_CARS) {
        t = aiCarTargets[owner];
        if (t != -1) {
            if ((unsigned int)t < TM2_MAX_AI_CARS)
                obj = (unsigned char *)aiCarObjects[t];
        } else if (humanCarTotal > 0)
            obj = (unsigned char *)humanCarObjects[humanCarNearest(&aiCars[owner][0x73C / 4], -1)];
    }
    return obj ? (int *)(obj + 0xA8) : 0;
}

/* ---- sound sides and local feedback (docs/HUMANS16_PLAN.md H3; port_humans.h) ---- */
extern void hudMessagePost(int msg, int prio);
extern void soundSetDeniedPending(unsigned char value);
extern int soundPlay(int, int, int, int);
extern int soundStartPlayerLoop(int, int, int, int, int);
extern int soundStopPlayerLoop(int, int);
extern int soundStartSharedLoop(int, int, int, int, int);
extern int soundStopSharedLoop(int, int);
extern unsigned char soundGetTurboLoopP1(void);
extern unsigned char soundGetTurboLoopP2(void);
extern int soundGetGunLoopP1(void);
extern int soundGetGunLoopP2(void);
extern unsigned char soundGetLoop32P1(void);
extern unsigned char soundGetLoop32P2(void);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *pos);
extern void soundLocateForView(void *pos, int view);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);

/* Human h's sound side on this machine: its local slot when that is 0 or 1 (the two fixed lists of each loop),
 * else -1 (a local slot 2-3, or remote). The default map gives humans 0 and 1 sides 0 and 1: retail. Pure. */
int PORT_TEXT portHumanSide(int h)
{
    int L = portHumanSlot(h);

    return L == 0 || L == 1 ? L : -1;
}

/* 1 when human h's own feedback (HUD messages, jingles, the denied sound, retail's full-volume sounds for "the
 * player") belongs to this machine: h is local (request 37 answer 7). Pure. */
int PORT_TEXT portHumanFeedback(int h)
{
    return portHumanSlot(h) >= 0;
}

/* hudMessagePost for human h's own action: shown only where h is local (the message state is HUD only). */
void PORT_TEXT portHumanMessage(int h, int msg, int prio)
{
    if (portHumanFeedback(h))
        hudMessagePost(msg, prio);
}

/* A full-volume sound retail plays for "the player" (a jingle, a turbo or weapon cue): only where h is local. */
void PORT_TEXT portHumanSound(int h, int snd, int vol, int pan, int pitch)
{
    if (portHumanFeedback(h))
        soundPlay(snd, vol, pan, pitch);
}

/* The denied buzz (worldUpdate plays 0x3F once per frame when pending): only for a local human's action. */
void PORT_TEXT portHumanDenied(int h)
{
    if (portHumanFeedback(h))
        soundSetDeniedPending(1);
}

/* A remote human's car sound as a computer car's: located at its car (record +0x6FC, motion.pos), the distance
 * from the listener (PadChkMtap) and, in one view, its pan (weaponTick's rule for the computer cars' guns). */
static void PORT_TEXT portHumanLocate(int h, int *vol, int *pan)
{
    int *pos = &humanCars[h][0x6FC / 4];

    if (viewGetSplitMode() == 0) {
        soundLocateForView0(pos);
        *vol = PadChkMtap();
        *pan = soundGetListenerPan();
    } else {
        soundLocateForView(pos, -1);
        *vol = PadChkMtap();
        *pan = 0;
    }
}

/* soundStartPlayerLoop(snd, vol, pan, pitch, player) of human `player` driving car slot `slot` (carThrottleSteer
 * turbo 0, weaponTick gun 9, carSoundUpdate burn / lava 32):
 * - a human with a sound side (local slot 0/1): retail's call on that side (humans 0/1 by default: retail);
 * - a remote human's gun: the computer cars' shared gun loop (soundStartSharedLoop, positional, the nearest of
 *   them heard), instance TM2_PORT_SHARED_GUN(slot);
 * - with TM2_PORT_CAR_SOUND every other loop as instance slot + 1 in the id's own list (request 33-4): a local
 *   human's (slot 2-3) at the caller's volume (the player's own: full), a remote human's positional (turbo: at
 *   its car; burn: carSoundUpdate's values, positional already). Without TM2_PORT_CAR_SOUND those have no list:
 *   not played (retail leaked a request each frame for a human of index 2 or more). */
void PORT_TEXT portHumanLoopPlay(int snd, int vol, int pan, int pitch, int player, int slot)
{
    int side;

    side = portHumanSide(player);
    if (side >= 0) {
        soundStartPlayerLoop(snd, vol, pan, pitch, side);
        return;
    }
    if ((unsigned int)player >= TM2_MAX_HUMANS || (unsigned int)slot >= TM2_PORT_SOUND_SLOTS)
        return;
    if (!portHumanFeedback(player)) {
        if (snd == 9) {
            portHumanLocate(player, &vol, &pan);
            soundStartSharedLoop(9, vol, pan, pitch, TM2_PORT_SHARED_GUN(slot));
            portCarSoundLoops[slot] |= TM2_PORT_LOOP_SHARED_GUN;
            return;
        }
    }
#ifdef TM2_PORT_CAR_SOUND
    {
        int bit = portLoopBit(snd);

        if (bit == 0 || soundManagerObject == 0)
            return;
        if (snd == 0 && !portHumanFeedback(player))
            portHumanLocate(player, &vol, &pan);
        portHumanLoopList(snd);
        if (portSoundPlayInstance(snd, vol, pan, pitch, slot + 1))
            portCarSoundLoops[slot] |= bit;
    }
#endif
}

/* The stop beside each of those plays: a human with a side makes retail's calls on it ("if (side == 0 && its P1
 * flag) soundStopPlayerLoop(snd, 0)", the same for 1 and P2); then whatever the slot started without a side (its
 * instance, its shared gun) is stopped, so a human whose side changes leaves nothing playing. */
void PORT_TEXT portHumanLoopStop(int snd, int player, int slot)
{
    int side, on;

    side = portHumanSide(player);
    if (side >= 0) {
        if (snd == 0)
            on = side == 0 ? soundGetTurboLoopP1() : soundGetTurboLoopP2();
        else if (snd == 9)
            on = (side == 0 ? soundGetGunLoopP1() : soundGetGunLoopP2()) & 255;
        else if (snd == 32)
            on = side == 0 ? soundGetLoop32P1() : soundGetLoop32P2();
        else
            on = 0;
        if (on)
            soundStopPlayerLoop(snd, side);
    }
    if ((unsigned int)slot >= TM2_PORT_SOUND_SLOTS)
        return;
    if (snd == 9 && (portCarSoundLoops[slot] & TM2_PORT_LOOP_SHARED_GUN)) {
        soundStopSharedLoop(9, TM2_PORT_SHARED_GUN(slot));
        portCarSoundLoops[slot] &= ~TM2_PORT_LOOP_SHARED_GUN;
    }
#ifdef TM2_PORT_CAR_SOUND
    on = portLoopBit(snd);
    if (on != 0 && (portCarSoundLoops[slot] & on)) {
        soundStopInstance(snd, slot + 1);
        portCarSoundLoops[slot] &= ~on;
    }
#endif
}

/* ---- the rules (docs/HUMANS16_PLAN.md H5, request 37 answers 8 and 9; port_humans.h) ----
 * With portHumanOptions bit 1 the port's battle modes decide when a round ends and what it means; the C exposes
 * who is alive or down, ends a round when the port says so, hands the round's result to the port's results and
 * starts a down human again where the port wants it. None of this reads a fixed human index. */
#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"
#define H5_HUMAN(h) ((struct TM2HumanRecord *)humanCars[h])
#define H5_AI(j) ((struct TM2AIRecord *)aiCars[j])

extern unsigned char gameAttractDemoMode;
extern int roundOverFlag, roundWonFlag;
extern int aiRouteNodeCount;
extern char strGameOver[];
extern int gameGetMode(void);
extern void rtScheduleEvent(int value, int time);
extern void sessionBindCars(struct TM2Settings *s);
extern void screenShowMessageLines(struct TM2Settings *s, int font, char *a, int ax, int ay, int bfont, char *b,
                                   int bx, int by, int cfont, char *c, int cx, int cy);
extern void carSetResetWeapons(signed char value, int index);
extern void hudResetSpeed(short player);
extern void humanCarPlaceAtStart(struct TM2HumanRecord *human);
extern void weaponBlockInit(void *car, struct TM2CarWeapons *weapons, unsigned char human);
extern void weaponSlamStateInit(struct TM2CarWeapons *weapons);
extern void viewCameraOffsetsInit(struct TM2HumanRecord *human);
extern void carShakeStart(void *car, int level, int axes, int time, unsigned char side);
extern void aiClearControls(struct TM2CarInput *input);
extern int PadGetConfig(short player);
extern void carApplyPadConfig(void *car, int config);
extern int lightGetEnv(int);
extern void csSetDrawEnable(void *object, unsigned char value);
extern void carSetDamageStage(int key, int stage, int arg);
extern void carBodyObjectShow(int key);

/* portHumanOptions bit 1, outside the attract demo (which has no humans and runs retail's rules): the gate of
 * every rule site (worldUpdate's won test, humanCarDamageUpdate, sessionRun, carNearestToHuman, the split HUDs'
 * lives, gameGetLives). The same on every machine (match state). */
int PORT_TEXT portHumanRules(void)
{
    return (portHumanOptions & TM2_HUMAN_OPT_RULES) != 0 && !gameAttractDemoMode;
}

/* Human h is present: a record the match counts (h < humanCarTotal) bound to a car. */
static int PORT_TEXT portHumanPresent(int h)
{
    return (unsigned int)h < TM2_MAX_HUMANS && h < humanCarTotal && humanCarObjects[h] != 0
           && !TM2_HUMAN_VACANT(h);    /* request 50: a vacant record is not present */
}

/* Human h drives: present, not destroyed (the dead flag), HP above 0. Pure. */
int PORT_TEXT portHumanAlive(int h)
{
    return portHumanPresent(h) && !H5_HUMAN(h)->state.dead && H5_HUMAN(h)->stats.hp > 0;
}

/* Human h is down: present and destroyed (its wreck; with bit 1 nothing restarts it before portHumanRespawn).
 * Pure. */
int PORT_TEXT portHumanDown(int h)
{
    return portHumanPresent(h) && (H5_HUMAN(h)->state.dead || H5_HUMAN(h)->stats.hp <= 0);
}

int PORT_TEXT portHumanAliveCount(void)
{
    int h, n;

    n = 0;
    for (h = 0; h < humanCarTotal && h < TM2_MAX_HUMANS; h++)
        if (portHumanAlive(h))
            n++;
    return n;
}

/* An AI record that drives: below aiCarTotal, not vacant (a drop-in removed it), not destroyed, HP above 0. */
static int PORT_TEXT portAiAlive(int j)
{
    if ((unsigned int)j >= TM2_MAX_AI_CARS || j >= aiCarTotal)
        return 0;
#ifdef TM2_PORT_CAR_DROPIN
    if (j < TM2_PORT_DROPIN_AI && portCarDropIn.vacant[j])
        return 0;
#endif
    return !H5_AI(j)->state.dead && H5_AI(j)->stats.hp > 0;
}

int PORT_TEXT portAiAliveCount(void)
{
    int j, n;

    n = 0;
    for (j = 0; j < aiCarTotal && j < TM2_MAX_AI_CARS; j++)
        if (portAiAlive(j))
            n++;
    return n;
}

/* The round over trigger (between frames, on every machine at the same frame): retail's round end (roundOverFlag,
 * rtScheduleEvent(event, delay): rtMainLoop returns `event` to sessionRun after `delay` ticks, the cars driving on
 * meanwhile), the end kept in portRound for the results. TM2_ROUND_FREEZE: also retail's won-round stop
 * (roundWonFlag: carApplyDamage hurts no car but Dark Tooth, carDestroyed ignores a human's wreck; the computer
 * cars' gun loops stop). Returns 1, or 0 when a round end is pending already (roundOverFlag), the event is not
 * positive, or in the attract demo. Works with bit 1 clear too (the port's modes with retail's rules). */
int PORT_TEXT portRoundEnd(int event, int delay, int winner, int flags)
{
    if (gameAttractDemoMode || event <= 0 || roundOverFlag)
        return 0;
    roundOverFlag = 1;
    if (flags & TM2_ROUND_FREEZE) {
        roundWonFlag = 1;
        soundStopSharedLoop(9, -1);
    }
    portRound.event = event;
    portRound.delay = delay > 0 ? delay : 0;
    portRound.winner = winner;
    rtScheduleEvent(event, portRound.delay);
    return 1;
}

/* sessionRun, after each round with bit 1 (the hook the port wraps for its results screen; it may call this for the
 * C's part): what the runner does with the round's event instead of retail's fixed-human rules. Returns -1 (retail's
 * handling: bit 1 clear, the attract demo's mode 0, a tournament's won round: the level progression, boss
 * introductions and endings, which go by car types, not by human), else 1 = the session ends, 0 = the next round:
 *   TM2_ROUND_QUIT       the session ends (every human's weapons reset, as retail);
 *   TM2_ROUND_GAME_OVER  retail's GAME OVER message, the session ends;
 *   tournament (modes 1/2), any other event: the round again, no lives[0] (the lives are portHumanLives, the
 *                        port's), the weapons of the humans down reset (all humans, not humans 0/1);
 *   battle (modes 3/4), any other event: no 2-player result screen and no tally (the port's results), every
 *                        human's weapons reset, the cars bound again (sessionBindCars, as retail every round). */
int PORT_TEXT portRoundResults(struct TM2Settings *s, int result)
{
    int mode, quit, k;

    if (!portHumanRules())
        return -1;
    mode = gameGetMode();
    if (mode == 0 || ((mode == 1 || mode == 2) && result == TM2_ROUND_WON))
        return -1;
    if (mode >= 3)
        sessionBindCars(s);
    quit = result == TM2_ROUND_QUIT || result == TM2_ROUND_GAME_OVER;
    if (result == TM2_ROUND_GAME_OVER)
        screenShowMessageLines(s, 6, strGameOver, 0x9F, 0x50, 0, 0, 0, 0, 0, 0, 0, 0);
    for (k = 0; k < TM2_MAX_HUMANS; k++)
        if (quit || mode >= 3 || portHumanDown(k))
            carSetResetWeapons(1, k);
    return quit;
}

/* carRestart (800FD550) for a respawn: the record's own reset, without the match-wide ones of a lost-a-life restart
 * (weaponDropsReset, weaponsRoundReset, projectilesReset, fxSpritesReset: every car's projectiles, mines and
 * specials; hudMessageReset: every HUD's message). The same steps as the drop-in's portDropInRestartHuman, the
 * car type kept; `keep`: carRestart's reset_weapons = 0 branch. */
static void PORT_TEXT portHumanRestartRecord(struct TM2HumanRecord *human, int keep)
{
    struct TM2Object *obj;

    human->stats.f00 = 1;
    humanCarPlaceAtStart(human);
    if (!keep) {
        weaponBlockInit(human, &human->weapons, 1);
    } else {
        human->weapons.ammo[0] = 2;
        human->weapons.energy = 1000;
        human->weapons.drop_armed = 0;
        if (human->type == 192)
            weaponSlamStateInit(&human->weapons);
    }
    human->weapons.invisible = 0;
    human->weapons.invis_timer = 0;
    human->input.f22 = 0;
    human->input.f21 = 0;
    human->input.f24 = 0;
    viewCameraOffsetsInit(human);
    human->state.x2C.route = 0;
    carShakeStart(human, 2, 2, 0, 1);
    human->speed_change = 0;
    aiClearControls(&human->input);
    carApplyPadConfig(human, PadGetConfig(human->player));
    human->stats.cheat_a = 0;
    human->stats.cheat_b = 0;
    human->stats.cheat_released = 0;
    human->stats.f0C = 0;
    human->stats.heal_spot_used = 0;
    human->contact.f01 = 1;
    obj = (struct TM2Object *)humanCarObjects[human->player];
    if (obj) {
        obj->light = lightGetEnv(0);
        obj->visible = 1;
    }
}

/* The respawn (request 37 answer 8: a drop-in style start, not carRestart in place): present human h starts again
 * on its own car (between frames, on every machine at the same frame): fresh stats and full HP (carStatsLoad, as
 * every start), fresh weapons (TM2_HUMAN_RESPAWN_KEEP_WEAPONS: retail's lost-a-life rule, 2 specials and full
 * energy, the rest kept), its state flags cleared (dead, burning, frozen, ...), the car drawn, its body shown at
 * damage stage 0, at route node `node` (an aiRouteNodes index the port picked; < 0 = the start farthest from every
 * live car, portStartNodePick's rule: no rand()). The heading draws one rand() as every retail start. The
 * match's projectiles, drops, specials and HUD messages stay; the view mode stays (the port's watch view sets it
 * back itself with carSetViewMode); its killer / last attacker are cleared by carStatsLoad (read them before).
 * Any present human may be restarted (alive too). Returns h, or TM2_HUMAN_ERR_BUSY (attract demo),
 * TM2_HUMAN_ERR_RECORD (no present human h), TM2_HUMAN_ERR_NODE (node past the arena's route nodes). */
int PORT_TEXT portHumanRespawn(int h, int node, int flags)
{
    struct TM2HumanRecord *human;
    int saved, key;

    if (gameAttractDemoMode)
        return TM2_HUMAN_ERR_BUSY;
    if (!portHumanPresent(h))
        return TM2_HUMAN_ERR_RECORD;
    if (node >= aiRouteNodeCount)
        return TM2_HUMAN_ERR_NODE;
    human = H5_HUMAN(h);
    human->stats.hp = 0;                /* not a live car for the start picker */
    saved = portCarOptions;
    portCarOptions |= TM2_PORT_CAR_OPT_FAR_STARTS;
    portRound.start_node = node >= 0 ? node + 1 : -1;
    portHumanRestartRecord(human, flags & TM2_HUMAN_RESPAWN_KEEP_WEAPONS);
    portRound.start_node = 0;
    portCarOptions = saved;
    hudResetSpeed(h);
    csSetDrawEnable((void *)humanCarObjects[h], 1);
    key = TM2_CAR_KEY(human->slot, human->type);
    carSetDamageStage(key, 0, human->stats.smoke_arg);
    human->stats.wreck_state = 0;
    carBodyObjectShow(key);
    return h;
}

/* carNearestToHuman with bit 1 (the target panel of human a's HUD; presentation only, no rand()): retail's rule for
 * one human or co-op (the nearest live computer car), else the nearest of the other live humans and the live
 * computer cars (retail: the nearest human only, so with bots in a battle the panel never showed one). *human = 1
 * for a human (the panel shows none), 0 for a computer car; 0 when nothing is near (|dx| + |dy| < 0x7FFF, as
 * retail's searches). */
void *PORT_TEXT portHumanNearestCar(int a, unsigned char *human)
{
    int *pos, *p, i, d, best, pick, kind;

    *human = 0;
    if (!portHumanPresent(a))
        return 0;
    pos = H5_HUMAN(a)->motion.pos;
    best = 0x7FFF;
    pick = -1;
    kind = 0;
    for (i = 0; i < aiCarTotal && i < TM2_MAX_AI_CARS; i++) {
        if (!portAiAlive(i))
            continue;
        p = H5_AI(i)->motion.pos;
        d = __builtin_abs(pos[0] - p[0]) + __builtin_abs(pos[1] - p[1]);
        if (d < best) {
            best = d;
            pick = i;
        }
    }
    if (humanCarTotal > 1 && gameGetMode() != 2) {
        for (i = 0; i < humanCarTotal && i < TM2_MAX_HUMANS; i++) {
            if (i == a || !portHumanAlive(i))
                continue;
            p = H5_HUMAN(i)->motion.pos;
            d = __builtin_abs(pos[0] - p[0]) + __builtin_abs(pos[1] - p[1]);
            if (d < best) {
                best = d;
                pick = i;
                kind = 1;
            }
        }
    }
    if (pick < 0)
        return 0;
    *human = kind;
    return kind ? (void *)H5_HUMAN(pick) : (void *)H5_AI(pick);
}

#ifdef TM2_HUMAN_HANDOVER
/* ---- request 50: a leaving player's car to a computer driver; vacant human records (port_humans.h "Hand-over",
 * HUMANS16_PLAN.md 8). Built on the swap's blocks (portCarResetSpecials, portCarResetLook, portCarUnbind,
 * portCarBindSlot, portCarApplyType, portCarParkSlot, portCarRekeyReferences) and the drop-in's reference blocks
 * (portDropInWeapons, portDropInRefs, portDropInSpecials, portDropInSlotSound), which the define builds without
 * TM2_PORT_CAR_DROPIN. No rand() here. */
#define HO portHumanVacant
#define HO_SLOT (TM2_MAX_CAR_SLOTS - 1)     /* the slot a vacant human record names (the drop-in's vacant slot) */
#define HO_FAR (-80000)                     /* a vacant record's x, y and z: out of every distance test, and small
                                               enough that 3 x 4096 x (80000 + an arena's 65536) stays in 32 bits
                                               (weaponFindTargetAhead's matrix product, no HP test there) */
#define HO_OFF(T, f) ((int)&((T *)0)->f)

extern int aiDifficultySpeedScale(void);
extern void aiRangeFlagsClear(unsigned char *flags);
extern void aiRouteStateReset(struct TM2AIDrive *drive);

void PORT_TEXT portHumanVacantReset(void)
{
    int i;

    HO.mask = 0;
    HO.handed = 0;
    HO.left = 0;
    HO.returned = 0;
    HO.rejoined = 0;
    for (i = 0; i < TM2_PORT_VACANT_HUMANS; i++)
        HO.ai[i] = 0;
}

int PORT_TEXT portHumanVacantCount(void)
{
    int h, n;

    n = 0;
    for (h = 0; h < humanCarTotal && h < TM2_MAX_HUMANS; h++)
        if (TM2_HUMAN_VACANT(h))
            n++;
    return n;
}

/* humanCarNearest's default (retail 0): the first human record that is not vacant (0 if all are). */
int PORT_TEXT portHumanFirstPresent(void)
{
    int h;

    for (h = 0; h < humanCarTotal && h < TM2_MAX_HUMANS; h++)
        if (!TM2_HUMAN_VACANT(h))
            return h;
    return 0;
}

/* sessionBindCars: the AI records to bind. Once a hand-over added records in this load, every AI record there is
 * (each on its own group: portCarBindGroup), else the settings' count (retail). */
int PORT_TEXT portHumanBindAiCount(int count)
{
    if (HO.handed == 0 || aiCarTotal < count)
        return count;
    return aiCarTotal;
}

/* The blocks both kinds of record have (cars.h): k = 0..7, their offset in a human (ai 0) or AI record (ai 1). */
static int PORT_TEXT portHandOverBlock(int k, int ai, int *size)
{
    if (k == 0) {
        *size = sizeof(struct TM2CarState);
        return ai ? HO_OFF(struct TM2AIRecord, state) : HO_OFF(struct TM2HumanRecord, state);
    }
    if (k == 1) {
        *size = sizeof(struct TM2CarInput);
        return ai ? HO_OFF(struct TM2AIRecord, input) : HO_OFF(struct TM2HumanRecord, input);
    }
    if (k == 2) {
        *size = sizeof(struct TM2CarAim);
        return ai ? HO_OFF(struct TM2AIRecord, aim) : HO_OFF(struct TM2HumanRecord, aim);
    }
    if (k == 3) {
        *size = sizeof(struct TM2CarStatsBlock);
        return ai ? HO_OFF(struct TM2AIRecord, stats) : HO_OFF(struct TM2HumanRecord, stats);
    }
    if (k == 4) {
        *size = sizeof(struct TM2CarContact);
        return ai ? HO_OFF(struct TM2AIRecord, contact) : HO_OFF(struct TM2HumanRecord, contact);
    }
    if (k == 5) {
        *size = sizeof(struct TM2CarWheels);
        return ai ? HO_OFF(struct TM2AIRecord, wheels) : HO_OFF(struct TM2HumanRecord, wheels);
    }
    if (k == 6) {
        *size = sizeof(struct TM2CarMotion);
        return ai ? HO_OFF(struct TM2AIRecord, motion) : HO_OFF(struct TM2HumanRecord, motion);
    }
    *size = sizeof(struct TM2CarWeapons);
    return ai ? HO_OFF(struct TM2AIRecord, weapons) : HO_OFF(struct TM2HumanRecord, weapons);
}

/* The car's state from one record to the other: the eight shared blocks, word by word. */
static void PORT_TEXT portHandOverCopy(unsigned char *dst, int dst_ai, unsigned char *src, int src_ai)
{
    int k, n, a, b, i;

    for (k = 0; k < 8; k++) {
        a = portHandOverBlock(k, dst_ai, &n);
        b = portHandOverBlock(k, src_ai, &n);
        for (i = 0; i < n; i += 4)
            *(int *)(dst + a + i) = *(int *)(src + b + i);
    }
}

/* An address inside record src (an arc's end point, an effect sprite's owner) at the same place of record dst
 * (block by block); outside the shared blocks: dst's motion block (its position). Other addresses stay. */
static void *PORT_TEXT portHandOverAddr(void *p, unsigned char *src, int src_ai, unsigned char *dst, int dst_ai)
{
    int off, k, n, a, b;

    off = (unsigned char *)p - src;
    if ((unsigned int)off >= (unsigned int)(src_ai ? 0x854 : 0x814))
        return p;
    for (k = 0; k < 8; k++) {
        b = portHandOverBlock(k, src_ai, &n);
        if (off >= b && off < b + n) {
            a = portHandOverBlock(k, dst_ai, &n);
            return dst + a + (off - b);
        }
    }
    return dst + portHandOverBlock(6, dst_ai, &n);
}

/* What names the car by its driver (CAR_GROUPS.md 14.5's list): the object id id -> id2 (projectile, arc, drop,
 * ricochet and bullet owners, arc targets), addresses inside record src -> record dst (arc end points, effect
 * sprites following it); the energy shields it carries end (their record names a human by index); the leaps and
 * swoops aimed at src end (portDropInSpecials, as portCarRemove). By slot (kill credits, taser shooter, dropper,
 * homing targets) nothing changes: the slot stays the car's. */
static void PORT_TEXT portHandOverRefs(struct TM2Object *obj, int id, int id2, unsigned char *src, int src_ai,
                                       unsigned char *dst, int dst_ai, int human)
{
    int i, size;

    size = src_ai ? 0x854 : 0x814;
    for (i = 0; i < TM2_MAX_PROJECTILES; i++)
        if (PROJ_LIFE(i) > 0 && PROJ_OWNER(i) == id)
            PROJ_OWNER(i) = id2;
    for (i = 0; i < TM2_MAX_ARCS; i++) {
        if (ARC_COUNT(i) <= 0)
            continue;
        if (ARC_TARGET(i) == id)
            ARC_TARGET(i) = id2;
        if (ARC_ID(i) == id)
            ARC_ID(i) = id2;
        if (portDropInIn(ARC_ORIGIN(i), src, size))
            ARC_ORIGIN(i) = (int *)portHandOverAddr(ARC_ORIGIN(i), src, src_ai, dst, dst_ai);
        if (portDropInIn(ARC_ATTACH(i), src, size))
            ARC_ATTACH(i) = (int *)portHandOverAddr(ARC_ATTACH(i), src, src_ai, dst, dst_ai);
    }
    for (i = 0; i < PORT_DROPS; i++)
        if (DROP_OWNER(i) == id)
            DROP_OWNER(i) = id2;
    for (i = 0; i < TM2_MAX_SHIELDS; i++)
        if (SHIELD_TIMER(i) > 0 && SHIELD_SOURCE(i) == obj) {
            SHIELD_TIMER(i) = 0;
            if (SHIELD_DST(i) != 0)
                SHIELD_DST(i)->visible = 0;
        }
    for (i = 0; i < TM2_MAX_RICOCHETS; i++)
        if (RICO_LIFE(i) > 0 && RICO_OWNER(i) == id)
            RICO_OWNER(i) = id2;
    for (i = 0; i < DROPIN_BULLETS; i++)
        if (BULLET_LIFE(i) > 0 && BULLET_OWNER(i) == id)
            BULLET_OWNER(i) = id2;
    for (i = 0; i < DROPIN_SPRITES; i++)
        if (portDropInIn(SPRITE_OWNER(i), src, size))
            SPRITE_OWNER(i) = (unsigned char *)portHandOverAddr(SPRITE_OWNER(i), src, src_ai, dst, dst_ai);
    portDropInSpecials(src, size, 0, id, id, human);
}

/* The car of a record: what it runs or holds ends as in a swap (its specials, the claws and crushes holding it, its
 * look: freeze, turbo flame, fire, invisibility, engine), its loops stop. */
static void PORT_TEXT portHandOverQuiet(int slot)
{
    portCarResetSpecials(slot);
    portCarResetLook(slot);
    portDropInSlotSound(slot);
}

/* Human record h made vacant (its car already given away or parked): dead, HP 0, wreck state 4, no damage, no
 * probe, no fire, effects and claw dropped, type 0, contacts, wheels and controls cleared, still and far away; its
 * object the dummy (not in the scene list: never drawn, never in a hit pass), its slot HO_SLOT (a slot past
 * carWorldObjectCount whose object is a dummy: carObjectId and the like find an object), remote, no input. */
static void PORT_TEXT portHumanVacateRecord(int h)
{
    struct TM2HumanRecord *hr = HREC_AT(h);
    struct TM2Object *dummy = (struct TM2Object *)HO.dummy;
    unsigned char *c;
    int i;

    hr->state.dead = 1;
    hr->state.burning = 0;
    hr->state.frozen = 0;
    hr->state.energy_shield = 0;
    hr->state.crush_contact = 0;
    hr->state.x2C.route = -1;           /* no section or track route (worldSectionDisable, trackVehiclesShouldStop) */
    hr->stats.hp = 0;
    if (hr->stats.max_hp <= 0)
        hr->stats.max_hp = 1;
    hr->stats.wreck_state = 4;
    hr->stats.killer = -1;
    hr->stats.last_attacker = -1;
    hr->stats.damageable = 0;
    hr->stats.probe_on = 0;
    hr->stats.restart_pending = 0;
    hr->stats.burn_time = 0;
    hr->stats.freeze_timer = 0;
    hr->stats.crush_owner = 0;
    hr->weapons.slam = 0;
    hr->weapons.fire_effect = 0;
    hr->weapons.f40 = 0;
    hr->weapons.invisible = 0;
    hr->weapons.drop_armed = 0;
    hr->fx = 0;
    hr->type = 0;
    humanCarTypes[h] = 0;
    c = (unsigned char *)&hr->contact;
    for (i = 0; i < (int)sizeof(hr->contact); i++)
        c[i] = 0;
    c = (unsigned char *)&hr->wheels;
    for (i = 0; i < (int)sizeof(hr->wheels); i++)
        c[i] = 0;
    c = (unsigned char *)&hr->input;
    for (i = 0; i < (int)sizeof(hr->input); i++)
        c[i] = 0;
    hr->motion.vel0 = 0;
    hr->motion.speed = 0;
    hr->motion.vel2 = 0;
    hr->motion.spin0 = 0;
    hr->motion.spin1 = 0;
    hr->motion.yaw_rate = 0;
    for (i = 0; i < 3; i++) {
        hr->motion.vel[i] = 0;
        hr->motion.pos[i] = HO_FAR;
        dummy->pos[i] = HO_FAR;
    }
    dummy->visible = 0;
    if (carSlotObjects[HO_SLOT] == 0 || (void *)carSlotObjects[HO_SLOT] == (void *)dummy) {
        carSlotObjects[HO_SLOT] = (void *)dummy;
        carSlotKinds[HO_SLOT] = 2;
        carSlotIndices[HO_SLOT] = -1;
        carSlotOwners[HO_SLOT] = 0;
    }
    humanCarObjects[h] = (int)dummy;
    humanCarObjectSlots[h] = HO_SLOT;
    hr->slot = HO_SLOT;
    portHumanLocal[h] = -1;
    portHumanPad[h] = 0;
    portHumanSteer[h] = 0;
    portHumanThrottle[h] = 0;                             /* request 121 */
    portHumanWreckLive[h] = 0;                            /* request 126 */
    portCarBindSet(0, h, 0);
    HO.mask |= 1u << h;
    HO.ai[h] = 0;
}

/* Human h bound to a car of this load (not vacant): its slot, else -1. */
static int PORT_TEXT portHandOverHumanSlot(int h, struct PortCarRec *r)
{
    int slot;

    if (!portCarRec(0, h, r) || TM2_HUMAN_VACANT(h))
        return -1;
    slot = *r->slot;
    if (!portCarSlotValid(slot) || slot >= HO_SLOT || carSlotKinds[slot] != 0 || carSlotIndices[slot] != h)
        return -1;
    return slot;
}

/* Request 50 (a): human h's car, as it is, to AI record j (port_humans.h). The AI record is set up as aiCarStart
 * sets one, without its start (no place, no fresh stats or weapons, no rand()): the car's blocks (state, input,
 * aim, stats, contacts, wheels, motion, weapons) copied from the human record, then its own fields (index, type,
 * started, the range thresholds and flags, route state, controls cleared, gun and fire cooldown 50, the "mines"
 * byte j % 3 where aiCarStart draws rand() % 3), the binding (portCarBindSlot: object id j + 50), the AI's stats
 * of the type at the difficulty's speed scale (portCarApplyType: AI max HP, top speed, collision box, weapons
 * kept, weights and fire chance), target the nearest human. Returns j, or TM2_HUMAN_ERR_BUSY (attract demo),
 * TM2_HUMAN_ERR_RECORD (no human h bound to a car), TM2_HUMAN_ERR_DOWN (its car is destroyed), TM2_HUMAN_ERR_LAST
 * (the last present human), TM2_HUMAN_ERR_INDEX (j not a free AI record), TM2_HUMAN_ERR_FULL (no vacant slot). */
int PORT_TEXT portHumanHandOver(int h, int j)
{
    struct PortCarRec r;
    struct TM2HumanRecord *hr;
    struct TM2AIRecord *ar;
    struct TM2Object *obj;
    struct TM2CarSlam *slam;
    int slot, group, id, id2, hp, k, i;

    if (gameAttractDemoMode)
        return TM2_HUMAN_ERR_BUSY;
    slot = portHandOverHumanSlot(h, &r);
    if (slot < 0)
        return TM2_HUMAN_ERR_RECORD;
    if (!portCarAlive(&r))
        return TM2_HUMAN_ERR_DOWN;
    if (humanCarTotal - portHumanVacantCount() <= 1)
        return TM2_HUMAN_ERR_LAST;
    if (carWorldObjectCount > HO_SLOT)
        return TM2_HUMAN_ERR_FULL;
    if (j < 0) {
        j = aiCarTotal;
#ifdef TM2_PORT_CAR_DROPIN
        for (k = 0; k < aiCarTotal && k < TM2_MAX_AI_CARS; k++)
            if (portCarDropIn.vacant[k]) {
                j = k;
                break;
            }
#endif
    }
    if (j >= TM2_MAX_AI_CARS || j > aiCarTotal)
        return TM2_HUMAN_ERR_INDEX;
#ifdef TM2_PORT_CAR_DROPIN
    if (j < aiCarTotal && !portCarDropIn.vacant[j])
        return TM2_HUMAN_ERR_INDEX;
#else
    if (j < aiCarTotal)
        return TM2_HUMAN_ERR_INDEX;
#endif
    hr = HREC_AT(h);
    ar = AREC_AT(j);
    obj = (struct TM2Object *)carSlotObjects[slot];
    id = TM2_HUMAN_ID(h);
    id2 = j + 50;
    group = portCarGroups.slot_group[slot];
    hp = hr->stats.hp;
    portHandOverQuiet(slot);
    /* the AI record: cleared as carTablesClear leaves a new one, its claw record kept for a car type that has
       none to bring */
    slam = ar->weapons.slam;
    for (i = 0; i < 0x854 / 4; i++)
        ((int *)ar)[i] = 0;
    if (j == aiCarTotal)
        aiCarTotal = j + 1;
#ifdef TM2_PORT_CAR_DROPIN
    portCarDropIn.vacant[j] = 0;
    if (carSlotKinds[HO_SLOT] == 1 && carSlotIndices[HO_SLOT] == j) {
        for (k = 0; k < aiCarTotal && k < TM2_MAX_AI_CARS && !portCarDropIn.vacant[k]; k++)
            ;
        if (k < aiCarTotal && k < TM2_MAX_AI_CARS) {
            carSlotIndices[HO_SLOT] = k;
        } else {
            carSlotKinds[HO_SLOT] = 2;
            carSlotIndices[HO_SLOT] = -1;
        }
    }
#endif
    portHandOverCopy((unsigned char *)ar, 1, (unsigned char *)hr, 0);
    ar->state.x2C.fx = hr->fx;
    if (ar->weapons.slam == 0)
        ar->weapons.slam = slam;
    ar->index = j;
    ar->type = hr->type;
    ar->f000 = 1;
    ar->f001 = j % 3;
    ar->f012 = (j + 1) * 60;
    ar->range_near = 320;
    ar->range_1 = 160;
    ar->range_2 = 600;
    ar->range_3 = 1000;
    ar->range_far = 2000;
    aiRangeFlagsClear(&ar->active);
    aiRouteStateReset(&ar->drive);
    aiClearControls(&ar->input);
    ar->weapons.gun_cooldown = 50;
    ar->weapons.fire_cooldown = 50;
    ar->stats.ai_chase_patience = 0;
    portCarUnbind(0, h);
    portCarBindSlot(1, j, slot);
    portCarBindSet(1, j, group);
    carStatsSetSpeedMult(&ar->stats, aiDifficultySpeedScale());
    portCarApplyType(1, j, hp);
    ar->stats.f00 = 0;
    aiCarTargets[j] = -1;
    portHandOverRefs(obj, id, id2, (unsigned char *)hr, 0, (unsigned char *)ar, 1, h);
    portHumanVacateRecord(h);
    HO.ai[h] = (signed char)(j + 1);
    HO.handed++;
    return j;
}

/* Request 50 (b): human h leaves and its car with it, as portCarRemove removes a car (its projectiles, mines, arcs,
 * shields, ricochets and bullets gone, every reference to it cleared, the car parked: a free car of its group),
 * without the compaction: h vacant. Allowed for a destroyed car too. Returns h, or TM2_HUMAN_ERR_BUSY,
 * TM2_HUMAN_ERR_RECORD, TM2_HUMAN_ERR_LAST, TM2_HUMAN_ERR_FULL. */
int PORT_TEXT portHumanLeave(int h)
{
    struct PortCarRec r;
    struct TM2Object *obj;
    int slot, id;

    if (gameAttractDemoMode)
        return TM2_HUMAN_ERR_BUSY;
    slot = portHandOverHumanSlot(h, &r);
    if (slot < 0)
        return TM2_HUMAN_ERR_RECORD;
    if (humanCarTotal - portHumanVacantCount() <= 1)
        return TM2_HUMAN_ERR_LAST;
    if (carWorldObjectCount > HO_SLOT)
        return TM2_HUMAN_ERR_FULL;
    obj = (struct TM2Object *)carSlotObjects[slot];
    id = TM2_HUMAN_ID(h);
    portHandOverQuiet(slot);
    portDropInWeapons(&r, obj, id, 0x814);
    portCarRekeyReferences(slot, -1);
    portDropInRefs(&r, id, 0x814);
    r.state->burning = 0;
    r.weapons->fire_effect = 0;
    portCarParkSlot(slot);
    portHumanVacateRecord(h);
    HO.left++;
    return h;
}

/* Request 50, the rejoin: vacant human h takes AI record j's car back as it is (the reverse of the hand-over: the
 * blocks copied back, the human's own fields as a drop-in start sets them without its place: the uaInit flags,
 * pad configuration 1, the bindings of its layout, controls, cheat state, camera offsets and view mode 2, the
 * human's stats of the type at speed 100: max HP + 20, HP kept), object id TM2_HUMAN_ID(h) for its things in the
 * air. AI record j leaves the match: the last record (aiCarTotal - 1) is cleared and aiCarTotal lowered; with
 * TM2_PORT_CAR_DROPIN any record is left vacant (portDropInVacate). AI targets on j are dropped. Returns h, or
 * TM2_HUMAN_ERR_BUSY, TM2_HUMAN_ERR_RECORD (h not vacant), TM2_HUMAN_ERR_INDEX (no such AI record bound to a car,
 * or not one that may leave), TM2_HUMAN_ERR_DOWN (its car is destroyed). */
int PORT_TEXT portHumanReturn(int h, int j)
{
    struct PortCarRec a;
    struct TM2HumanRecord *hr;
    struct TM2AIRecord *ar;
    struct TM2Object *obj;
    int slot, group, id, id2, hp, s, i;

    if (gameAttractDemoMode)
        return TM2_HUMAN_ERR_BUSY;
    if ((unsigned int)h >= TM2_MAX_HUMANS || h >= humanCarTotal || !TM2_HUMAN_VACANT(h))
        return TM2_HUMAN_ERR_RECORD;
    if (!portCarRec(1, j, &a))
        return TM2_HUMAN_ERR_INDEX;
#ifdef TM2_PORT_CAR_DROPIN
    if (portCarDropIn.vacant[j])
        return TM2_HUMAN_ERR_INDEX;
#else
    if (j != aiCarTotal - 1)
        return TM2_HUMAN_ERR_INDEX;
#endif
    slot = *a.slot;
    if (!portCarSlotValid(slot) || slot >= HO_SLOT || carSlotKinds[slot] != 1 || carSlotIndices[slot] != j)
        return TM2_HUMAN_ERR_INDEX;
    if (!portCarAlive(&a))
        return TM2_HUMAN_ERR_DOWN;
    hr = HREC_AT(h);
    ar = AREC_AT(j);
    obj = (struct TM2Object *)carSlotObjects[slot];
    id = j + 50;
    id2 = TM2_HUMAN_ID(h);
    group = portCarGroups.slot_group[slot];
    hp = ar->stats.hp;
    portHandOverQuiet(slot);
    for (i = 0; i < 0x814 / 4; i++)
        ((int *)hr)[i] = 0;
    portHandOverCopy((unsigned char *)hr, 0, (unsigned char *)ar, 1);
    hr->fx = ar->state.x2C.fx;
    hr->state.x2C.route = 0;
    hr->player = h;
    hr->f002 = h;
    hr->type = ar->type;
    HO.mask &= ~(1u << h);
    HO.ai[h] = 0;
    portCarUnbind(1, j);
    portCarBindSlot(0, h, slot);
    portCarBindSet(0, h, group);
    portCarBindSet(1, j, 0);
    PadSetConfig(1, h);
    portHumanSetLayout(h, portHumanLayout[h]);
    hudResetSpeed(h);
    carStatsSetSpeedMult(&hr->stats, 100);
    portCarApplyType(0, h, hp);
    hr->stats.f00 = 1;
    hr->stats.cheat_a = 0;
    hr->stats.cheat_b = 0;
    hr->stats.cheat_released = 0;
    hr->stats.f0C = 0;
    aiClearControls(&hr->input);
    carApplyPadConfig(hr, PadGetConfig(h));
    if (!gameAttractDemoMode)
        carSetViewMode(hr, 2);
    portHandOverRefs(obj, id, id2, (unsigned char *)ar, 1, (unsigned char *)hr, 0, -1);
    for (s = 0; s < TM2_MAX_AI_CARS; s++)
        if (aiCarTargets[s] == j)
            aiCarTargets[s] = -1;
    if (viewWatchedAiCar == j)
        viewWatchedAiCar = 0;
#ifdef TM2_PORT_CAR_DROPIN
    if (j != aiCarTotal - 1) {
        portCarDropIn.live = 1;         /* sessionBindCars skips the vacant record from now on */
        portDropInVacate(&a);
        portCarBindSet(1, j, 0);
        HO.returned++;
        return h;
    }
#endif
    for (i = 0; i < 0x854 / 4; i++)
        ((int *)ar)[i] = 0;
    aiCarObjects[j] = 0;
    aiCarObjectSlots[j] = -1;
    aiCarTargets[j] = -1;
    aiCarTotal = j;
    HO.returned++;
    return h;
}

/* Request 50b: the first slot of load group `group` when its car is free (a loaded car nobody drives: slot kind 2,
 * index -1, as portCarParkSlot / portHumanLeave / portCarRemove leave one), else -1. With the drop-in, not a group
 * an unload gave back. */
static int PORT_TEXT portHumanFreeSlot(int group)
{
    int s;

    if ((unsigned int)group >= TM2_CAR_SLOT_CAP)
        return -1;
    s = portCarSlotOfGroup(group);
    if (!portCarSlotValid(s) || s >= HO_SLOT || carSlotKinds[s] != 2 || carSlotIndices[s] != -1)
        return -1;
#ifdef TM2_PORT_CAR_DROPIN
    if ((unsigned int)group < TM2_PORT_DROPIN_GROUPS && portCarDropIn.group_live[group] == 2)
        return -1;
#endif
    return s;
}

/* Request 50b: a free car of car type `type` (0x10..0xF0): its load group, lowest first; 0 if none. */
int PORT_TEXT portHumanFreeGroupOfType(int type)
{
    int s, g, best;

    best = 0;
    for (s = 0; s < carWorldObjectCount && s < HO_SLOT; s++) {
        g = portCarGroups.slot_group[s];
        if (g > 0 && carSlotOwners[s] == type && portHumanFreeSlot(g) == s && (best == 0 || g < best))
            best = g;
    }
    return best;
}

/* Request 50b (HUMANS16_PLAN.md 8.4): a player joining a match in progress takes vacant human record h back, in
 * place (no other record moves, humanCarTotal unchanged), on the free car of load group `group`, as a fresh start:
 * the record cleared, its index fields, the vacancy cleared, the per-record part of uaInit (reset weapons, damageable,
 * the six flags, pad configuration 1), its bindings from portHumanLayout[h], hudResetSpeed, the binding
 * (portCarBindSlot: object id TM2_HUMAN_ID(h)), then portHumanRespawn's start (portHumanRestartRecord: fresh stats,
 * full HP, fresh weapons, at route node `node` or (node < 0) the start farthest from every live car; the heading
 * draws one rand() as every start), view mode 2, the car shown. Write portHumanLocal[h], portHumanLayout[h] and
 * portHumanLives[h] first. Returns h, or TM2_HUMAN_ERR_BUSY, TM2_HUMAN_ERR_RECORD (h not vacant),
 * TM2_HUMAN_ERR_NODE, TM2_HUMAN_ERR_INDEX (no free car in that group), TM2_HUMAN_ERR_FULL (with the drop-in:
 * TM2_PORT_CARS cars drive already). */
int PORT_TEXT portHumanRejoin(int h, int group, int node)
{
    struct PortCarRec r;
    struct TM2HumanRecord *hr;
    int slot, saved, i;

    if (gameAttractDemoMode)
        return TM2_HUMAN_ERR_BUSY;
    if ((unsigned int)h >= TM2_MAX_HUMANS || h >= humanCarTotal || !TM2_HUMAN_VACANT(h))
        return TM2_HUMAN_ERR_RECORD;
    if (node >= aiRouteNodeCount)
        return TM2_HUMAN_ERR_NODE;
    slot = portHumanFreeSlot(group);
    if (slot < 0)
        return TM2_HUMAN_ERR_INDEX;
#ifdef TM2_PORT_CAR_DROPIN
    if (portCarActiveCount() >= TM2_PORT_CARS)
        return TM2_HUMAN_ERR_FULL;
#endif
    hr = HREC_AT(h);
    for (i = 0; i < 0x814 / 4; i++)
        ((int *)hr)[i] = 0;
    hr->player = h;
    hr->f002 = h;
    hr->type = carSlotOwners[slot];
    HO.mask &= ~(1u << h);
    HO.ai[h] = 0;
    carSetResetWeapons(1, h);
    hr->stats.damageable = 1;
    PadSetConfig(1, h);
    portHumanSetLayout(h, portHumanLayout[h]);
    hudResetSpeed(h);
    portCarBindSlot(0, h, slot);
    portCarBindSet(0, h, group);
    saved = portCarOptions;
    portCarOptions |= TM2_PORT_CAR_OPT_FAR_STARTS;
    portRound.start_node = node >= 0 ? node + 1 : -1;
    portHumanRestartRecord(hr, 0);
    portRound.start_node = 0;
    portCarOptions = saved;
    if (!gameAttractDemoMode)
        carSetViewMode(hr, 2);
    if (portCarRec(0, h, &r))
        portCarModelOn(&r, 1);
    HO.rejoined++;
    return h;
}
#undef HO_OFF
#undef HO_FAR
#undef HO_SLOT
#undef HO
#endif /* TM2_HUMAN_HANDOVER */
#undef H5_HUMAN
#undef H5_AI
#endif

#ifdef TM2_PORT_HAZARDS
/* ---- arena hazards and destructibles off (PC port request 38, docs/ARENA_SWITCHES.md) ---- */
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
extern int gameGetLevel(void);
extern void trackVehicleSurfaceSet(int n, int *pos, int size, int angle);
extern void targetSetTrackVehiclePos(int type, int *pos);

/* Hong Kong's train records (trackVehicleRecords, 56 bytes; track_vehicles.c) and the fields of their scene
 * objects the park writes. */
struct PortTrainObject {
    unsigned char pad0[8];
    int visible;            /* +0x08 drawn, and hit by cars (hdpPassRun tests +8 & +0xC) and weapons */
    unsigned char pad12[156];
    int position[3];        /* +0xA8 */
};
struct PortTrainRecord {
    int state;              /* +0x00 path state */
    int speed;              /* +0x04 */
    int turn;               /* +0x08 heading change this frame (carried riders turn by it) */
    int busy;               /* +0x0C 1 = destroyed: the next update re-places it (worldTrackVehiclePlace) */
    int front[3];           /* +0x10 */
    int rear[3];            /* +0x1C */
    int move[3];            /* +0x28 movement this frame (carried riders move by it) */
    struct PortTrainObject *object;     /* +0x34 */
};
typedef char tm2_port_train_record_size[(sizeof(struct PortTrainRecord) == 56) ? 1 : -1];
extern struct PortTrainRecord *trackVehicleRecords;

/* roundStart, before its srand: the items off this round. Bits 0 / 1 of portArenaOptions switch on the hazard /
 * destructible items; portArenaItems adds single items (per-arena switches). No rand(). */
void PORT_TEXT portArenaLatch(void)
{
    unsigned int live = portArenaItems;

    if (portArenaOptions & TM2_ARENA_OPT_HAZARDS_OFF)
        live |= TM2_ARENA_HAZARD_ITEMS;
    if (portArenaOptions & TM2_ARENA_OPT_DESTRUCTIBLES_OFF)
        live |= TM2_ARENA_DESTRUCTIBLE_ITEMS;
    portArenaLive = live;
}

/* Every gate: 1 when `item` (TM2_ARENA_*) is off this round. */
int PORT_TEXT portArenaOff(int item)
{
    return (portArenaLive >> item) & 1;
}

/* targetDamage, at its entry (before the pool search, the counter and every rand()): 1 when target `type` takes
 * no damage this round. The event targets count only on their own arena (the disc has them nowhere else). */
int PORT_TEXT portArenaTargetImmune(int type)
{
    unsigned int items = 0;

    switch (type) {
    case 500: case 501: case 502: case 503: case 504: case 505: case 520:
        items = TM2_ARENA_BIT(TM2_ARENA_PROPS);
        break;
    case 506: case 507:
        if (gameGetLevel() == 8)
            items = TM2_ARENA_BIT(TM2_ARENA_TRAINS) | TM2_ARENA_BIT(TM2_ARENA_TRAIN_TARGETS);
        break;
    case 508:
        if (gameGetLevel() == 1)
            items = TM2_ARENA_BIT(TM2_ARENA_LA_TIMED);
        break;
    case 509: case 519:
        if (gameGetLevel() == 5)
            items = TM2_ARENA_BIT(TM2_ARENA_NY_EVENT);
        break;
    case 510:
        if (gameGetLevel() == 7)
            items = TM2_ARENA_BIT(TM2_ARENA_HOLLAND_BARNS);
        break;
    }
    return (portArenaLive & items) != 0;
}

/* worldUpdate on Hong Kong, in place of worldTrackVehiclesUpdate: with TRAINS off, both trains are removed every
 * frame (idempotent): hidden (not drawn; cars and weapons skip a scene object whose +8 is 0), their objects moved
 * outside the map, no carry for riders (turn and move 0), busy as retail's destroyed train, their roof surface
 * records parked at (24000, 24000) with size 0 as trackVehicleDestroy does, and their targets' positions with it
 * (splash damage cannot reach them; targetDamage keeps them immune). Returns 0 (retail update) with TRAINS on. */
int PORT_TEXT portArenaTrainsPark(void)
{
    int far_pos[3];
    struct PortTrainRecord *rec;
    struct PortTrainObject *obj;
    int i;

    if (!((portArenaLive >> TM2_ARENA_TRAINS) & 1))
        return 0;
    far_pos[0] = 24000;
    far_pos[1] = 24000;
    far_pos[2] = 0;
    for (i = 0; i < 2; i++) {
        rec = &trackVehicleRecords[i];
        obj = rec->object;
        if (obj == 0)
            continue;
        obj->visible = 0;
        obj->position[0] = 24000;
        obj->position[1] = 24000;
        obj->position[2] = -64;
        rec->busy = 1;
        rec->turn = 0;
        rec->move[0] = 0;
        rec->move[1] = 0;
        rec->move[2] = 0;
        trackVehicleSurfaceSet(i, far_pos, 0, 0);
        targetSetTrackVehiclePos(506 + i, far_pos);
    }
    return 1;
}
#endif /* TM2_PORT_HAZARDS */

#ifdef TM2_PORT_BALANCE
/* ---- Vanilla Plus balancing (PC port requests 40 and 41, docs/BALANCE.md) ----
 * The gates' helpers. They read the round's copy (portBalanceLive) and call no rand(); with portBalanceLive 0 each
 * returns the retail value (or does nothing to the game) at its first tests. With their bit set some of them change
 * the game themselves: portBalanceMineSlot removes a driver's oldest mine (bit 2), portBalanceCrushTop applies
 * Hammerhead's +5 through retail's carApplyDamage (bit 3), portBalanceLeapLand sets Grasshopper down (bit 13),
 * portBalanceTornadoThrow throws the caught cars (bit 12, retail's carSpinOutById), portBalanceTaserRetry connects
 * a taser shot late (bit 16, retail's carSpinOut: the connect's lift).
 * The record views are cars.h's (included above for the car swap / specials / balance declarations). */
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
#define BAL(bits) (portBalanceLive & (bits))
#define BAL_H(car) ((struct TM2HumanRecord *)(car))
#define BAL_A(car) ((struct TM2AIRecord *)(car))
extern int weaponDropSlotFind(void);                     /* 80109648 (weapon_tables.c) */
extern int weaponMineObjectFind(void);                   /* 80109E84 (arena_load_register.c) */
extern int weaponShieldSlotFind(void);                   /* the shield pool scan (weaponFireEnergyAttack's) */
extern int *carPairDirection(int other, int id);         /* the pair-table lookup carCrashStart uses */
extern int worldGroundHeightAt(int *pos);                /* 800EF340 (car_wheels.c) */
extern int ratan2(int y, int x);
extern void carApplyDamage(int id, int amount, int *dir, int type, int attacker);
extern struct TM2Object *humanCarObject(int index);
extern struct TM2Object *aiCarObject(int index);
extern unsigned char carFindByObject(void *object, unsigned char *human, int *index);  /* 8011D1CC (car_records.c) */
extern unsigned char weaponDrops[TM2_MAX_DROPS * 28];    /* 28-byte drop records (round_reset.c / above) */
extern void *weaponMineObjects[TM2_MAX_MINE_OBJECTS];    /* asset_pools.c / above */
#define BD(d) (weaponDrops + 28 * (d))
#define BD_KIND(d) (*(signed char *)BD(d))                   /* +0x00 20 mine, 19 remote bomb */
#define BD_HELD(d) (BD(d)[1])                                /* +0x01 */
#define BD_ID(d) (*(unsigned short *)(BD(d) + 4))            /* +0x04 the dropper's object id */
#define BD_STATUS(d) (*(short *)(BD(d) + 8))                 /* +0x08 frames left, <= 0 free */
#define BD_OBJ(d) (*(unsigned char **)(BD(d) + 0x18))        /* +0x18 its scene object */
/* the mines' records [0, end), the remote bombs' [end, TM2_DROPS_LIVE): retail sizes 23 + 2, 16 cars 80 + 16 */
#define BAL_MINE_END (TM2_DROPS_LIVE - TM2_POOL_LIVE(REMOTE_BOMBS))
#define BAL_LEAP_TARGET (*(int **)((unsigned char *)&homingLeapState + 0x34))   /* 80184E08: the target's pos */
/* the block's own names for homingLeapState's fields (the drop-in's LEAP_* are not defined without it) and the
   Thumper flames (declared again: only TM2_PORT_CAR_SOUND or TM2_PORT_CARS declares them otherwise) */
#define BAL_LEAP_TARGET_ID (*(unsigned short *)((unsigned char *)&homingLeapState + 0x10))  /* the target's id */
#define BAL_LEAP_POS (*(unsigned char **)((unsigned char *)&homingLeapState + 0x2C))        /* the leaper's pos */
extern int weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS][32 / 4];
/* the taser / lightning arcs (weapon_slot_target.c, weapon_arcs_draw.c: 20 bytes) */
#if TM2_MAX_ARCS <= TM2_RETAIL_ARCS
extern int weaponArcs[TM2_MAX_ARCS][20 / 4];
#endif
#define BA_ID(a) (((short *)weaponArcs[a])[0])          /* +0x00 the shooter's id (-1 after a miss) */
#define BA_OWNER(a) (((short *)weaponArcs[a])[1])       /* +0x02 the shooter's slot, the damage's source */
#define BA_COUNT(a) (((short *)weaponArcs[a])[2])       /* +0x04 ticks left (40 at the shot) */
#define BA_KIND(a) (((short *)weaponArcs[a])[3])        /* +0x06 0 taser, 1 lightning */
#define BA_TARGET(a) (((short *)weaponArcs[a])[4])      /* +0x08 the victim's object id, -1 none */
#define BA_FROM(a) (*(int **)&weaponArcs[a][3])         /* +0x0C &the shooter's motion.pos */
#define BA_TO(a) (*(int **)&weaponArcs[a][4])           /* +0x10 &the victim's motion.pos */
extern unsigned int GsGetWorkBase_0(void);              /* 80155370: timerFrameCount */
extern int timerGetFrameTicks(void);                     /* 8015531C: the step's ticks (60ths of a second) */

static int PORT_TEXT balType(void *car, int human)
{
    return human ? BAL_H(car)->type : BAL_A(car)->type;
}

static int PORT_TEXT balSlot(void *car, int human)
{
    return human ? BAL_H(car)->slot : BAL_A(car)->slot;
}

/* 1 when the Twister `car` has her tornado running: her key's state (the first Twister's is retail's global, a
 * duplicate's its portCarSpecials entry, selected for the test) is running and was fired by her car object. */
static int PORT_TEXT balTornadoRuns(void *car, int human)
{
    struct TM2Object *obj;
    int r;
#ifdef TM2_PORT_CAR_SPECIALS
    int t;
#endif

    if (balType(car, human) != TM2_BAL_TWISTER_TYPE)
        return 0;
    obj = human ? humanCarObject(BAL_H(car)->player) : aiCarObject(BAL_A(car)->index);
#ifdef TM2_PORT_CAR_SPECIALS
    t = portCarSpecialSelect(portCarKey(balSlot(car, human), TM2_BAL_TWISTER_TYPE));
#endif
    r = weaponTornadoState.timer > 0 && weaponTornadoState.object == obj;
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(t);
#endif
    return r;
}

/* Request 91 item 2 (BALANCE.md 19): 1 when the Mr. Slam `car` has his arm and claw special running: his slam record
 * (weapons +0x48) is active, from weaponFireSpecial 0xC0's start (or the easy-difficulty bump start in
 * carCarCollision) until his car update clears it once neither the grab nor the swing runs. */
static int PORT_TEXT balSlamRuns(void *car, int human)
{
    struct TM2CarSlam *s;

    if (balType(car, human) != TM2_BAL_SLAM_TYPE)
        return 0;
    s = human ? BAL_H(car)->weapons.slam : BAL_A(car)->weapons.slam;
    return s != 0 && s->active != 0;
}

/* The car record of slot `slot` (human or AI), 0 if none; the per-car port state of slot `slot`, 0 if out of range. */
static struct TM2PortBalanceCar *PORT_TEXT balCar(int slot)
{
    return (unsigned int)slot < TM2_MAX_CAR_SLOTS ? &portBalanceCars[slot] : 0;
}

/* 1 when the Grasshopper in `slot` is leaping (her key's leap state is active and hers). */
static int PORT_TEXT balLeaping(int slot, int type)
{
    int r;
#ifdef TM2_PORT_CAR_SPECIALS
    int t;
#endif

    if (type != TM2_BAL_GRASSHOPPER_TYPE)
        return 0;
#ifdef TM2_PORT_CAR_SPECIALS
    t = portCarSpecialSelect(portCarKey(slot, TM2_BAL_GRASSHOPPER_TYPE));
#endif
    r = homingLeapState.active != 0 && homingLeapState.owner_id == slot;
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(t);
#endif
    return r;
}

/* The live mines of car `id` (object id), and its oldest one's record in *oldest (-1 for none). */
static int PORT_TEXT balMines(int id, int *oldest)
{
    int i, n = 0;

    *oldest = -1;
    for (i = 0; i < TM2_DROPS_LIVE; i++)
        if (BD_KIND(i) == 20 && BD_STATUS(i) > 0 && BD_ID(i) == (id & 0xFFFF)) {
            n++;
            if (*oldest < 0 || portBalanceDropSeq[i] < portBalanceDropSeq[*oldest])
                *oldest = i;
        }
    return n;
}

/* The first free record of the mine range, -1 if none. */
static int PORT_TEXT balMineRecord(void)
{
    int i;

    for (i = 0; i < BAL_MINE_END; i++)
        if (BD_STATUS(i) <= 0)
            return i;
    return -1;
}

/* roundStart, before its srand: the rules of this round; no Hammerhead +5 is owed. */
void PORT_TEXT portBalanceLatch(void)
{
    int i;

    portBalanceLive = portBalance;
    portBalanceLive2 = portBalance2;        /* request 124: the second word */
    /* request 127: the mines' limit for this round, in 60ths of a second */
    portBalanceMineTicks = (portBalanceMineSeconds <= 0 ? TM2_BAL2_MINE_SECONDS
                            : portBalanceMineSeconds > TM2_BAL2_MINE_MAX_SECONDS ? TM2_BAL2_MINE_MAX_SECONDS
                            : portBalanceMineSeconds) * 60;
    for (i = 0; i < TM2_MAX_CAR_SLOTS; i++) {
        portBalanceRideTop[i] = 0;
        portBalanceCars[i].frozen_at = 0;
        portBalanceCars[i].freeze_len = 0;
        portBalanceCars[i].gun_half = 0;
        portBalanceCars[i].guns_refused = 0;
        portBalanceJam[i].stops = 0;        /* request 91 item 3 */
        portBalanceJam[i].avoid_until = 0;
    }
    portBalanceGunObjects = 0;
    for (i = 0; i < 4; i++)
        portBalanceBoom[i] = 0;             /* request 102 item 3: no explosion sound yet */
}

/* request 102 item 3 (bit 30, BALANCE.md 23): retail's `ready` (the 6-step cooldown has run), or with the bit an
 * explosion TM2_BAL_BOOM_APART or more from the last one that sounded (|dx| + |dy| + |dz|). A sound to play is
 * remembered as the last. Match state (portBalanceBoom, hashed), cleared by portBalanceLatch; no rand(). */
int PORT_TEXT portBalanceBoomReady(int *pos, int ready)
{
    int d, i;

    if (!BAL(TM2_BAL_BOOM_SOUND))
        return ready;
    if (!ready && portBalanceBoom[3]) {
        d = 0;
        for (i = 0; i < 3; i++)
            d += __builtin_abs(pos[i] - portBalanceBoom[i]);
        if (d < TM2_BAL_BOOM_APART)
            return 0;
    }
    for (i = 0; i < 3; i++)
        portBalanceBoom[i] = pos[i];
    portBalanceBoom[3] = 1;
    return 1;
}

/* weaponLaunchProjectile (bit 0): the homing missile (kind 2) 10 -> 7, the fire missile (kind 1) 7 -> 10. */
int PORT_TEXT portBalanceMissile(int kind, int damage)
{
    if (BAL(TM2_BAL_MISSILES)) {
        if (kind == 2)
            return 7;
        if (kind == 1)
            return 10;
    }
    return damage;
}

/* weaponFireEnergyAttack 103 (bit 1): 1 when the cast is refused at no cost: the car is shielded already, or no
 * shield record is free (retail takes the 400 for nothing in both cases). */
int PORT_TEXT portBalanceShieldRefused(int shielded)
{
    if (!BAL(TM2_BAL_SHIELD))
        return 0;
    return shielded != 0 || weaponShieldSlotFind() < 0;
}

/* weaponFireEnergyAttack 101: 1 when the mine is refused at no cost: bit 6, the car is a Twister whose tornado
 * runs; bit 2, it cannot be placed (fewer than 5 mines of this driver, and no free mine record or object). */
int PORT_TEXT portBalanceMineRefused(void *car, int human)
{
    int id, oldest;

    if (BAL(TM2_BAL_TWISTER) && balTornadoRuns(car, human))
        return 1;
    if (!BAL(TM2_BAL_MINES))
        return 0;
    id = human ? TM2_HUMAN_ID(BAL_H(car)->player) : BAL_A(car)->index + 50;
    if (balMines(id, &oldest) >= TM2_BAL_MINES_PER_DRIVER)
        return 0;                       /* the oldest makes room (portBalanceMineSlot) */
    return balMineRecord() < 0 || weaponMineObjectFind() < 0;
}

/* weaponFireSelected 3 (bit 6): 1 when a remote bomb may not be placed: the car is a Twister whose tornado runs. */
int PORT_TEXT portBalanceBombRefused(void *car, int human)
{
    return BAL(TM2_BAL_TWISTER) && balTornadoRuns(car, human);
}

/* weaponDropWrite 101: the record for a mine of car `id`. Bit 2: at 5 live mines the driver's oldest is removed
 * (silently, as the drop-in removes a leaving car's: record and object freed), then the first free record of the
 * mine range, only when a mine object is free too (so no record is opened without one: RETAIL_BUGS.md 22); its
 * placement order is noted. Else retail's weaponDropSlotFind. */
/* A mine removed silently (bit 2's oldest, request 127's clocks): record and object freed, as the drop-in removes a
 * leaving car's. No effect, no damage, no sound. */
static void PORT_TEXT balMineRemove(int i)
{
    BD_STATUS(i) = 0;
    BD_KIND(i) = 0;
    BD_HELD(i) = 0;
    if (BD_OBJ(i) != 0) {
        *(int *)(BD_OBJ(i) + 8) = 0;    /* hidden: free for weaponMineObjectFind */
        BD_OBJ(i) = 0;
    }
}

/* Request 127: every record a mine is written to starts with its clocks at 0. */
static int PORT_TEXT balMineClocksStart(int i)
{
    if ((unsigned int)i < TM2_MAX_DROPS) {
        portBalanceMineAge[i] = 0;
        portBalanceMineOrphan[i] = 0;
    }
    return i;
}

int PORT_TEXT portBalanceMineSlot(int id)
{
    int i, oldest;

    if (!BAL(TM2_BAL_MINES))
        return balMineClocksStart(weaponDropSlotFind());
    if (balMines(id, &oldest) >= TM2_BAL_MINES_PER_DRIVER && oldest >= 0)
        balMineRemove(oldest);
    i = balMineRecord();
    if (i < 0 || weaponMineObjectFind() < 0)
        return -1;
    portBalanceDropSeq[i] = ++portBalanceDropSeqNext;
    return balMineClocksStart(i);
}

/* Request 127 (portBalance2 bits 2, 3; BALANCE.md 27): 1 when the car of object id `id` (a mine's +4: a human's id,
 * an AI's index + 50) is destroyed (dead, or HP <= 0), or when no car has the id (it left the match). Reads only. */
int PORT_TEXT portBalanceCarDown(int id)
{
    unsigned char human;
    int index;

    if (!(carFindById(id, &human, &index) & 255))
        return 1;
    if (human) {
        if ((unsigned int)index >= TM2_MAX_HUMANS || index >= humanCarTotal)
            return 1;
        return humanCarGet(index)->state.dead || humanCarGet(index)->stats.hp <= 0;
    }
    if ((unsigned int)index >= TM2_MAX_AI_CARS || index >= aiCarTotal)
        return 1;
    return aiCarGet(index)->state.dead || aiCarGet(index)->stats.hp <= 0;
}

/* Request 127: weaponDropsUpdate, before its loop. Once a step (timerFrameCount: weaponDropsUpdate runs once per view
 * in retail), each live mine's clocks run by the step's ticks (60ths of a second: frame-rate free, the same on every
 * machine): bit 2 its age; bit 3 the time since its owner's car was first seen destroyed (once started it keeps
 * running, a respawn does not stop it). A clock at the round's limit (portBalanceMineTicks) removes the mine
 * silently. Neither bit: nothing. No rand(). */
void PORT_TEXT portBalanceMineClock(void)
{
    unsigned int step = GsGetWorkBase_0();
    int i, ticks;

    if (!(portBalanceLive2 & (TM2_BAL2_MINE_AGE | TM2_BAL2_MINE_ORPHAN)) || step == portBalanceMineStep)
        return;
    portBalanceMineStep = step;
    ticks = timerGetFrameTicks();
    for (i = 0; i < TM2_MAX_DROPS; i++) {
        if (BD_KIND(i) != 20 || BD_STATUS(i) <= 0)
            continue;
        if (portBalanceLive2 & TM2_BAL2_MINE_AGE)
            portBalanceMineAge[i] += ticks;
        if ((portBalanceLive2 & TM2_BAL2_MINE_ORPHAN) && (portBalanceMineOrphan[i] > 0 || portBalanceCarDown(BD_ID(i))))
            portBalanceMineOrphan[i] += ticks;
        if (portBalanceMineAge[i] >= portBalanceMineTicks || portBalanceMineOrphan[i] >= portBalanceMineTicks)
            balMineRemove(i);
    }
}

/* weaponDropWrite 3 (placing a remote bomb): bit 2, the bombs' own records first, then any free one. */
int PORT_TEXT portBalanceBombSlot(void)
{
    int i;

    if (BAL(TM2_BAL_MINES))
        for (i = BAL_MINE_END; i < TM2_DROPS_LIVE; i++)
            if (BD_STATUS(i) <= 0)
                return i;
    return weaponDropSlotFind();
}

/* weaponDropsUpdate (bit 2): 1 when the record must not count down: a mine at 1 stays there (never expires; its
 * hit test keeps running). */
int PORT_TEXT portBalanceMineKeep(int kind, int timer)
{
    return BAL(TM2_BAL_MINES) && kind == 20 && timer == 1;
}

/* carCarCollision, for a car with stats mounts_cars: 1 when it may not mount the car it hit. Bit 4 Axel, bit 5 Minion
 * (by car type, borrowing or not); bit 11 (with SPECIAL_AS) a Hammerhead whose special is borrowed. */
int PORT_TEXT portBalanceNoMount(int slot, int type)
{
    if (BAL(TM2_BAL_AXEL) && type == TM2_BAL_AXEL_TYPE)
        return 1;
    if (BAL(TM2_BAL_MINION) && type == TM2_BAL_MINION_TYPE)
        return 1;
#ifdef TM2_PORT_SPECIAL_AS
    if (BAL(TM2_BAL_HH_BORROW) && type == TM2_BAL_HAMMERHEAD_TYPE
        && (unsigned int)slot < TM2_CAR_SLOT_CAP && portCarSpecialAs[slot] > 0)
        return 1;
#endif
    return 0;
}

/* carContactRespond, the car-to-car branch: the ram damage d that `car` takes from the car in slot `other`. None for
 * a Twister whose tornado runs (bit 6) or a leaping Grasshopper (bit 7); else x1.2 when Hammerhead dealt it (bit 3),
 * x1.1 when Axel (bit 4) or Minion (bit 5) did (by car type), rounded to nearest. */
int PORT_TEXT portBalanceRam(void *car, int human, int other, int d)
{
    unsigned char h;
    void *o;
    int type;

    if (portBalanceLive == 0 || d <= 0)
        return d;
    type = balType(car, human);
    if (BAL(TM2_BAL_TWISTER) && type == TM2_BAL_TWISTER_TYPE && balTornadoRuns(car, human))
        return 0;
    if (BAL(TM2_BAL_GRASSHOPPER) && balLeaping(balSlot(car, human), type))
        return 0;
    if (other < 0)
        return d;
    o = carFromSlot(other, &h);
    if (o == 0)
        return d;
    type = balType(o, h);
    if (BAL(TM2_BAL_HAMMERHEAD) && type == TM2_BAL_HAMMERHEAD_TYPE)
        return TM2_BAL_SCALE(d, TM2_BAL_HH_TENTHS);
    if (BAL(TM2_BAL_AXEL) && type == TM2_BAL_AXEL_TYPE)
        return TM2_BAL_SCALE(d, TM2_BAL_AXEL_TENTHS);
    if (BAL(TM2_BAL_MINION) && type == TM2_BAL_MINION_TYPE)
        return TM2_BAL_SCALE(d, TM2_BAL_MINION_TENTHS);
    return d;
}

/* carCrashStart, once per crush (the mount; its 15 stays retail's): bit 3, a Hammerhead crusher is owed +5 the
 * first frame he is on top. */
void PORT_TEXT portBalanceCrushStart(int slot, int kind)
{
    if ((unsigned int)slot < TM2_MAX_CAR_SLOTS)
        portBalanceRideTop[slot] = BAL(TM2_BAL_HAMMERHEAD) && kind == TM2_BAL_HAMMERHEAD_TYPE;
}

/* carCrushContactUpdate, a frame the rider `car` is on top (nearest wheel within half the crush range) of the car
 * found as (peer_human, peer_index): bit 3, the owed +5 once (carApplyDamage as the mount's: the victim's object
 * id, the pair direction, type 0, the rider's slot; 5 < 8 draws no spin roll). */
void PORT_TEXT portBalanceCrushTop(void *car, int human, int peer_human, int peer_index)
{
    struct TM2Object *obj;
    int slot, peer_slot;

    slot = balSlot(car, human);
    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || !portBalanceRideTop[slot])
        return;
    portBalanceRideTop[slot] = 0;
    if (!BAL(TM2_BAL_HAMMERHEAD))
        return;
    if (peer_human) {
        obj = humanCarObject(peer_index);
        peer_slot = humanCarGet(peer_index)->slot;
    } else {
        obj = aiCarObject(peer_index);
        peer_slot = aiCarGet(peer_index)->slot;
    }
    if (obj == 0)
        return;
    carApplyDamage(obj->id, TM2_BAL_RIDE_TOP, carPairDirection(slot, peer_slot), 0, slot);
}

/* carCrushContactUpdate, the ride ended: nothing owed. */
void PORT_TEXT portBalanceCrushEnd(void *car, int human)
{
    int slot = balSlot(car, human);

    if ((unsigned int)slot < TM2_MAX_CAR_SLOTS)
        portBalanceRideTop[slot] = 0;
}

/* weaponTick: the recharge period of the car's special (its own, or the borrowed one with SPECIAL_AS): Hammerhead
 * 700 (bit 3; request 41, request 40 built 900), Minion 600 (bit 5), Twister 600 (bit 6), Outlaw 2 900 (bit 8),
 * Mr. Grimm 1200 (bit 21), Thumper 1500 (bit 22; request 45 split bit 8 per car); else the stored value. Computer
 * cars have full ammo every tick (weaponTick), so this matters for humans only. */
int PORT_TEXT portBalancePeriod(void *car, int human, int period)
{
    int type;

    if (!BAL(TM2_BAL_HAMMERHEAD | TM2_BAL_MINION | TM2_BAL_TWISTER | TM2_BAL_OUTLAW2_RECHARGE
             | TM2_BAL_GRIMM_RECHARGE | TM2_BAL_THUMPER_RECHARGE))
        return period;
    type = TM2_SPECIAL_TYPE(balSlot(car, human), balType(car, human));
    switch (type) {
    case TM2_BAL_HAMMERHEAD_TYPE:
        if (BAL(TM2_BAL_HAMMERHEAD))
            return TM2_BAL_HH_RECHARGE;
        break;
    case TM2_BAL_MINION_TYPE:
        if (BAL(TM2_BAL_MINION))
            return 600;
        break;
    case TM2_BAL_TWISTER_TYPE:
        if (BAL(TM2_BAL_TWISTER))
            return 600;
        break;
    case TM2_BAL_OUTLAW2_TYPE:
        if (BAL(TM2_BAL_OUTLAW2_RECHARGE))
            return 900;
        break;
    case TM2_BAL_GRIMM_TYPE:
        if (BAL(TM2_BAL_GRIMM_RECHARGE))
            return 1200;
        break;
    case TM2_BAL_THUMPER_TYPE:
        if (BAL(TM2_BAL_THUMPER_RECHARGE))
            return 1500;
        break;
    }
    return period;
}

/* weaponHomingLeapUpdate (inside the leaping Grasshopper's selection): the hit's damage, 20 with bit 7 (request 121). */
int PORT_TEXT portBalanceLeapDamage(int damage)
{
    return BAL(TM2_BAL_GRASSHOPPER) ? TM2_BAL_LEAP_DAMAGE : damage;
}

/* 1 when the leaping Grasshopper's contact (the leap's cancel byte, contact.in_contact) is a car-to-car record (f01
 * clear) with the leap's target (that car's motion.pos is the leap's target pointer). */
static int PORT_TEXT balLeapTouchesTarget(void)
{
    unsigned char h, oh;
    void *car, *other;
    struct TM2CarContact *c;

    car = carFromSlot(homingLeapState.owner_id, &h);
    if (car == 0)
        return 0;
    c = h ? &BAL_H(car)->contact : &BAL_A(car)->contact;
    if (!c->in_contact || c->hit.f01)
        return 0;
    other = carFromSlot(c->hit.other, &oh);
    if (other == 0)
        return 0;
    return (oh ? BAL_H(other)->motion.pos : BAL_A(other)->motion.pos) == BAL_LEAP_TARGET;
}

/* weaponHomingLeapUpdate (bit 25; bit 7 until request 88): 1 when her contact is with the leap's target (touching
 * counts as the hit). */
int PORT_TEXT portBalanceLeapTouch(void)
{
    return BAL(TM2_BAL_LEAP_TOUCH) && balLeapTouchesTarget();
}

/* weaponHomingLeapUpdate, at the hit (bit 13; a first cut that needs a runtime look, BALANCE.md 3.8): she is set
 * down on the ground under her (worldGroundHeightAt, as weaponDropItem probes it, + her ride height) when the leap
 * left her above it, and the upward speed of her wheels (kind-7 hop) is cut to a quarter, about half the time in
 * the air (the hop's time grows as the square root of that speed). */
void PORT_TEXT portBalanceLeapLand(void)
{
    unsigned char h;
    void *car;
    struct TM2CarMotion *m;
    struct TM2CarWheels *w;
    int q[3], z, i;

    if (!BAL(TM2_BAL_LEAP_LANDING))
        return;
    car = carFromSlot(homingLeapState.owner_id, &h);
    if (car == 0)
        return;
    m = h ? &BAL_H(car)->motion : &BAL_A(car)->motion;
    w = h ? &BAL_H(car)->wheels : &BAL_A(car)->wheels;
    q[0] = m->pos[0];
    q[1] = m->pos[1];
    q[2] = m->pos[2] + 24;
    z = worldGroundHeightAt(q) + (h ? BAL_H(car)->stats.ride_height : BAL_A(car)->stats.ride_height);
    if (z < m->pos[2]) {
        z -= m->pos[2];
        m->pos[2] += z;
        for (i = 0; i < 4; i++)
            w->wheel[i].pos[2] += z;
    }
    for (i = 0; i < 4; i++)
        if (w->wheel[i].fall > 0)
            w->wheel[i].fall /= 4;
}

/* weaponHomingLeapUpdate, at the hit, after its carApplyDamage (bit 31, request 103, BALANCE.md 24): the target (the
 * leap's target id) pushed TM2_BAL_LEAP_PUSH less its mass% (carApplyDamage's type-1 rule), along her flight at the
 * hit (her motion.pos to the target's, flat, unit 4096 by VectorNormal; straight ahead in +y when they coincide),
 * through carKnockBack (half again at mass 51 or more; the push timer and contact as a missile's). Her own record is
 * not written. No rand(). */
extern int VectorNormal(int *in, int *out);
extern void carKnockBack(void *car, unsigned char human, int *dir, int amount);
void PORT_TEXT portBalanceLeapPush(void)
{
    unsigned char h;
    int index, p, d[3], n[3];
    int *me, *to;
    void *car;

    if (!BAL(TM2_BAL_LEAP_PUSH_BIT) || !(carFindById(BAL_LEAP_TARGET_ID, &h, &index) & 255))
        return;
    if (h ? (unsigned int)index >= TM2_MAX_HUMANS || index >= humanCarTotal
          : (unsigned int)index >= TM2_MAX_AI_CARS || index >= aiCarTotal)
        return;
    car = h ? (void *)humanCars[index] : (void *)aiCars[index];
    me = (int *)BAL_LEAP_POS;
    to = h ? BAL_H(car)->motion.pos : BAL_A(car)->motion.pos;
    if (me == 0)
        return;
    p = TM2_BAL_LEAP_PUSH;
    p -= p * (h ? BAL_H(car)->stats.mass : BAL_A(car)->stats.mass) / 100;
    d[0] = to[0] - me[0];
    d[1] = to[1] - me[1];
    d[2] = 0;
    if (d[0] == 0 && d[1] == 0)
        d[1] = 1;
    VectorNormal(d, n);
    carKnockBack(car, h, n, p);
}

/* areaDamage (bit 10): a car's splash damage; the raised-surface case's +25 is capped. Shadow's detonation (big 0):
 * min(retail, 25) + 3, so 28 on a surface and 28 - d/4 off it (retail 50 - d/4 / 25 - d/4). A remote bomb (big 1,
 * both of its callers): min(retail, 25), so 25 on a surface and 25 - d/4 off it (retail 50 - d/4 / 25 - d/4). */
int PORT_TEXT portBalanceSplash(int damage, int big)
{
    if (!BAL(TM2_BAL_SHADOW))
        return damage;
    if (damage > TM2_BAL_SPLASH_CAP)
        damage = TM2_BAL_SPLASH_CAP;
    return big ? damage : damage + TM2_BAL_SPLASH_BONUS;
}

/* weaponTornadoUpdate (bit 12, BALANCE.md 3.7 option c), called once by the tornado's end block (after its effects,
 * before it clears the humans' caught flags), inside the ending Twister's selection (with TM2_PORT_CAR_SPECIALS
 * the twin's state is the retail global here). Every car caught at the end is thrown, humans first and then the
 * computer cars in index order, as retail's freeze_timer == 1 test meant to throw it: carSpinOutById(its key, the
 * heading from the tornado's centre to it, 0). Caught at the end (request 41 item 5): a human whose state.f16 this
 * tornado's loop set this frame (retail's own caught flag: in her box before this frame's orbit); a computer car
 * (no such flag) in her box on the positions this frame's orbit left (retail's test, DISTANCE(her car object,
 * car) < 35). Her own car never. The catching itself, its damage and the freeze are left as retail.
 * carSpinOutById draws retail's own rand() calls (its spin-out and car shake), this function none. */
void PORT_TEXT portBalanceTornadoThrow(void)
{
    struct TM2Object *her, *obj;
    struct TM2CarMotion *m;
    int n, i, k, d[3], key;
    unsigned char human;

    if (!BAL(TM2_BAL_TWISTER_THROW))
        return;
    her = weaponTornadoState.object;
    for (human = 1; ; human = 0) {
        n = human ? humanCarCount() : aiCarCount();
        for (i = 0; i < n; i++) {
            if (human) {
                obj = humanCarObject(i);
                m = &humanCarGet(i)->motion;
                key = TM2_CAR_KEY(humanCarGet(i)->slot, humanCarGet(i)->type);
            } else {
                obj = aiCarObject(i);
                m = &aiCarGet(i)->motion;
                key = TM2_CAR_KEY(aiCarGet(i)->slot, aiCarGet(i)->type);
            }
            if (obj == her)
                continue;
            if (human) {
                if (!humanCarGet(i)->state.f16)   /* retail's caught flag, set by this frame's loop */
                    continue;
            } else {
                for (k = 0; k < 3; k++) {
                    d[k] = m->pos[k] - her->pos[k];
                    if (d[k] < 0)
                        d[k] = -d[k];
                }
                if (((d[0] > d[1] ? (d[0] > d[2] ? d[0] : d[2]) : (d[1] > d[2] ? d[1] : d[2])) >> 3)
                    >= TM2_BAL_TORNADO_BOX)
                    continue;
            }
            /* retail's push vector (car - tornado centre; the centre is her position: only x and y count) and
               tm2_heading (y 0 -> 1) */
            d[0] = m->pos[0] - weaponTornadoObject->pos[0];
            d[1] = m->pos[1] - weaponTornadoObject->pos[1];
            if (d[1] == 0)
                d[1] = 1;
            carSpinOutById(key, (short)ratan2(d[0], d[1]), 0);
        }
        if (!human)
            break;
    }
}

/* ---- request 41 (round 2, BALANCE.md 12) ---- */

/* weaponFireSelected, weaponFireEnergyAttack (bit 15): 1 when the car may fire nothing: a Twister whose tornado
 * runs (pickups and her special, every energy attack, a remote bomb's detonation; refused at no cost). Request 91
 * item 2 (bit 26, BALANCE.md 19): the same for a Mr. Slam whose arm and claw special runs (his machine guns are not
 * locked: portBalanceGunsRefused tests bit 15 only). */
int PORT_TEXT portBalanceLocked(void *car, int human)
{
    return (BAL(TM2_BAL_TWISTER_LOCK) && balTornadoRuns(car, human))
        || (BAL(TM2_BAL_SLAM_LOCK) && balSlamRuns(car, human));
}

/* weaponFireEnergyAttack (bit 15): as portBalanceLocked, but her shield (energy attack `kind` 103,
 * TM2_BAL_LOCK_FREE_KIND) is let through: request 46 item 3 (BALANCE.md 15), the one thing she may use while her
 * tornado runs; it costs and works as retail's (bit 1's own refusal still applies). */
int PORT_TEXT portBalanceEnergyLocked(void *car, int human, int kind)
{
    return kind != TM2_BAL_LOCK_FREE_KIND && portBalanceLocked(car, human);
}

/* request 121 item 5 (bit 14, BALANCE.md 25): weaponFireSelected, his special (kind 0): 1 when a Thumper (his special
 * Thumper's: a borrower of it too) still has a flame of his own burning: a flame emitter whose timer runs and whose
 * car object is his. Refused at no cost, as bit 15's lock; his pickups and guns as retail. No rand(). */
int PORT_TEXT portBalanceFlameLocked(void *car, int human)
{
    struct TM2Object *obj;
    int e;

    if (!BAL(TM2_BAL_THUMPER_LOCK)
        || TM2_SPECIAL_TYPE(balSlot(car, human), balType(car, human)) != TM2_BAL_THUMPER_TYPE)
        return 0;
    obj = human ? humanCarObject(BAL_H(car)->player) : aiCarObject(BAL_A(car)->index);
    for (e = 0; e < TM2_MAX_FLAME_EMITTERS; e++)
        if (*(short *)&weaponFlameEmitters[e][0] > 0 && *(struct TM2Object **)&weaponFlameEmitters[e][7] == obj)
            return 1;
    return 0;
}

/* ---- request 91 item 3 (bit 27, BALANCE.md 20): computer cars going round a pile ---- */
extern void *aiGetTargetCar(int i, unsigned char *human);

/* The jam record of the computer car `ai`, 0 when bit 27 is clear or its slot is out of range. */
static struct TM2PortBalanceJam *PORT_TEXT balJam(void *ai)
{
    int slot;

    slot = BAL_A(ai)->slot;
    return BAL(TM2_BAL_AI_UNJAM) && (unsigned int)slot < TM2_MAX_CAR_SLOTS ? &portBalanceJam[slot] : 0;
}

/* 1 while the computer car `ai` goes round a pile. */
static int PORT_TEXT balJamRound(struct TM2PortBalanceJam *j)
{
    return j != 0 && j->avoid_until != 0 && (int)(j->avoid_until - (unsigned int)GsGetWorkBase_0()) > 0;
}

/* aiUpdateStop, the car stopped after a contact (just before its re-plan). A stop with no wall bump since the last
 * re-plan (drive.f474 0: retail counts walls and objects only) whose last car hit (contact.last) is not its own
 * target is a stop by a car. TM2_BAL_JAM_STOPS of them within TM2_BAL_JAM_WINDOW frames: 1 (the caller arms the
 * reverse, as for a car knocked round), and for TM2_BAL_JAM_AVOID frames the node it was heading for is avoided and
 * the direct chase is off. No rand(). */
int PORT_TEXT portBalanceJamStop(void *ai)
{
    struct TM2PortBalanceJam *j;
    void *tgt;
    unsigned char th;
    unsigned int now;

    if ((j = balJam(ai)) == 0 || BAL_A(ai)->drive.f474 != 0)
        return 0;
    tgt = aiGetTargetCar(BAL_A(ai)->index, &th);
    if (tgt != 0 && BAL_A(ai)->contact.last == (th ? BAL_H(tgt)->player : BAL_A(tgt)->index))
        return 0;       /* ramming its target: retail's (a number shared by a human and a computer car counts so) */
    now = (unsigned int)GsGetWorkBase_0();
    if (j->stops == 0 || now - j->since > TM2_BAL_JAM_WINDOW) {
        j->stops = 0;
        j->since = now;
    }
    if (++j->stops < TM2_BAL_JAM_STOPS)
        return 0;
    j->stops = 0;
    j->avoid = BAL_A(ai)->drive.node_next;
    j->avoid_until = now + TM2_BAL_JAM_AVOID;
    if (j->avoid_until == 0)
        j->avoid_until = 1;
    return 1;
}

/* aiPickLinkNode, each candidate: 1 = leave out route node `node` (the car goes round the pile it was jammed in
 * toward that node). With every candidate left out the pick is retail's default, the link's first node. */
int PORT_TEXT portBalanceJamAvoid(void *ai, int node)
{
    struct TM2PortBalanceJam *j;

    j = balJam(ai);
    return balJamRound(j) && j->avoid == node;
}

/* aiShouldChaseDirect: 1 = no direct chase while the car goes round (it follows the route nodes). */
int PORT_TEXT portBalanceJamRound(void *ai)
{
    return balJamRound(balJam(ai));
}

/* ---- request 95 (bit 28, BALANCE.md 21): the boomerang caught ---- */

/* A boomerang `rec` (type 51) on its way back (its target its thrower's live position: weaponProjectileUpdate and
 * weaponProjectileSteer set it at the turn) with life left, within TM2_BAL_BOOMERANG_CATCH of its living thrower at
 * `pt`: freed, the thrower's charge back once (at most its max_specials; a car with nothing selected selects it, as
 * the recharge). Only its return to the car gives the charge (the user's rule): a hit or its life's end, retail's.
 * No rand(). */
/* (the record's fields this reads: weapons.h struct TM2Projectile, projectile_hit.c's TM2Proj) */
struct PortBoomerang {
    signed char id;             /* +0x00 */
    unsigned char pad_01;
    short type;                 /* +0x02 */
    short life;                 /* +0x04 */
    unsigned char pad_06[0x38 - 0x06];
    int *target;                /* +0x38 */
    unsigned char *thrower;     /* +0x3C the thrower's object (its position at +0xA8) */
    unsigned char pad_40[2];
    short owner;                /* +0x42 */
};

int PORT_TEXT portBalanceBoomerangCatch(void *rec, int *pt)
{
    struct PortBoomerang *p;
    struct TM2CarWeapons *w;
    struct TM2CarState *st;
    unsigned char human;
    int *at, index, d, k;

    p = (struct PortBoomerang *)rec;
    if (!BAL(TM2_BAL_BOOMERANG) || p->type != 51 || p->life <= 0 || p->thrower == 0)
        return 0;
    at = (int *)(p->thrower + 0xA8);
    if (p->target != at)
        return 0;
    d = 0;
    for (k = 0; k < 3; k++) {
        int v = pt[k] - at[k];
        if (v < 0)
            v = -v;
        if (v > d)
            d = v;
    }
    if ((d >> 3) >= TM2_BAL_BOOMERANG_CATCH || !(carFindById(p->owner, &human, &index) & 255))
        return 0;
    if (human) {
        if ((unsigned int)index >= TM2_MAX_HUMANS || index >= humanCarTotal)
            return 0;
        st = &((struct TM2HumanRecord *)humanCars[index])->state;
        w = &((struct TM2HumanRecord *)humanCars[index])->weapons;
    } else {
        if ((unsigned int)index >= TM2_MAX_AI_CARS || index >= aiCarTotal)
            return 0;
        st = &((struct TM2AIRecord *)aiCars[index])->state;
        w = &((struct TM2AIRecord *)aiCars[index])->weapons;
    }
    if (st->dead)
        return 0;
    fxRecordFree(p->id, 1);
    if (w->ammo[0] < w->max_specials)
        w->ammo[0]++;
    if (w->selected >= 8 && w->ammo[0] > 0)
        w->selected = 0;
    return 1;
}

/* weaponTick, the machine guns (bit 15): `held` = the gun request this frame. 0 = fire as retail; 1 = refused; 2 =
 * refused and the press is new (the denied sound once per press, not every frame the button is held). */
int PORT_TEXT portBalanceGunsRefused(void *car, int human, int held)
{
    struct TM2PortBalanceCar *b;
    int r;

    if (!BAL(TM2_BAL_TWISTER_LOCK) || (b = balCar(balSlot(car, human))) == 0)
        return 0;
    r = 0;
    if (held && balTornadoRuns(car, human))
        r = b->guns_refused ? 1 : 2;
    b->guns_refused = r != 0;
    return r;
}

/* weaponArcsDraw, an arc still counting with no target (bit 16): a taser shot (kind 0; lightning is kind 1) that
 * connected to no one at the shot (weaponTaserTarget) searches again, with retail's test (the nearest car within
 * DISTANCE 80 of the shooter, humans first, then the computer cars alive, not the shooter; strict <), from the
 * arc's second tick (count 39 and below: the shot's own tick is retail's miss) to its last. The shooter is the car
 * whose motion.pos the arc starts at (a miss cleared its id and slot). On a find the arc is connected as
 * weaponTaserTarget connects it (id, slot, target, end point) with the connect's lift, the victim's carSpinOut kind
 * 6 (the car jumps up as on a connect at the shot), and no damage now (round 3, BALANCE.md 13): the arc's one hit
 * is weaponArcsDraw's own count == 0 hit of 10 (its count == 39 hit is already past), so a late connect deals 10,
 * not 2 x 10. A connect on the last tick lifts and hits in the same tick. Calls carSpinOut (retail's rand() call of
 * a connect: the bounce pattern), no rand() itself. */
void PORT_TEXT portBalanceTaserRetry(int arc)
{
    unsigned char h;
    struct TM2Object *obj, *self;
    void *shooter, *victim;
    int *from, *p;
    int i, n, d, k, best, id, slot, vh;

    if (!BAL(TM2_BAL_TASER) || BA_TARGET(arc) >= 0 || BA_KIND(arc) != 0 || BA_COUNT(arc) <= 0
        || BA_COUNT(arc) >= 40)
        return;
    from = BA_FROM(arc);
    shooter = 0;
    for (h = 1; ; h = 0) {
        n = h ? humanCarCount() : aiCarCount();
        for (i = 0; i < n && !shooter; i++)
            if ((h ? humanCarGet(i)->motion.pos : aiCarGet(i)->motion.pos) == from) {
                shooter = h ? (void *)humanCarGet(i) : (void *)aiCarGet(i);
                self = h ? humanCarObject(i) : aiCarObject(i);
                id = h ? TM2_HUMAN_ID(humanCarGet(i)->player) : i + 50;
                slot = h ? humanCarGet(i)->slot : aiCarGet(i)->slot;
            }
        if (shooter || !h)
            break;
    }
    if (!shooter || self == 0)
        return;
    best = TM2_BAL_TASER_RANGE;
    victim = 0;
    for (h = 1; ; h = 0) {
        n = h ? humanCarCount() : aiCarCount();
        for (i = 0; i < n; i++) {
            if (!h && aiCarGet(i)->stats.hp <= 0)
                continue;
            obj = h ? humanCarObject(i) : aiCarObject(i);
            if (obj == 0 || obj->id == self->id)
                continue;
            d = 0;
            for (k = 0; k < 3; k++) {
                p = &obj->pos[k];
                if (__builtin_abs(*p - from[k]) > d)
                    d = __builtin_abs(*p - from[k]);
            }
            d >>= 3;
            if (d < best) {
                best = d;
                victim = h ? (void *)humanCarGet(i) : (void *)aiCarGet(i);
                vh = h;
                BA_TARGET(arc) = (short)obj->id;
                BA_TO(arc) = h ? humanCarGet(i)->motion.pos : aiCarGet(i)->motion.pos;
            }
        }
        if (!h)
            break;
    }
    if (!victim)
        return;
    BA_ID(arc) = (short)id;
    BA_OWNER(arc) = (short)slot;
    carSpinOut(victim, vh, 6, 0);
}

/* projectileHitTest, an ice freeze (type 14: the freeze missile, the energy freeze, Minion's volley), the car already
 * marked frozen: its length (ticks). Bit 17: when the car's chain is live (it was frozen during the last
 * TM2_BAL_FREEZE_WINDOW frames, which includes "frozen now", and the chain had an ice freeze) the length is half
 * the chain's last one, at least TM2_BAL_FREEZE_FLOOR; else `len` (retail's 300), which starts a chain. A car not
 * frozen (a shielded human's timer write) is not a freeze: `len`, the chain untouched. */
int PORT_TEXT portBalanceFreeze(void *car, int human, int len)
{
    struct TM2PortBalanceCar *b;
    unsigned int now;

    if (!BAL(TM2_BAL_FREEZE_DR) || (b = balCar(balSlot(car, human))) == 0)
        return len;
    if (!(human ? BAL_H(car)->state.frozen : BAL_A(car)->state.frozen))
        return len;
    now = GsGetWorkBase_0();
    if (b->freeze_len > 0 && now - b->frozen_at <= TM2_BAL_FREEZE_WINDOW) {
        len = b->freeze_len / 2;
        if (len < TM2_BAL_FREEZE_FLOOR)
            len = TM2_BAL_FREEZE_FLOOR;
    }
    b->freeze_len = (short)len;
    b->frozen_at = now;
    return len;
}

/* humanCarTick, aiCarDrive: every frame the car is frozen (any freeze), before its timer counts down (bit 17): the
 * chain's clock. A frozen streak that starts more than TM2_BAL_FREEZE_WINDOW frames after the last frozen frame
 * starts a new chain (the last ice length forgotten). */
void PORT_TEXT portBalanceFrozenTick(void *car, int human)
{
    struct TM2PortBalanceCar *b;
    unsigned int now;

    if (!BAL(TM2_BAL_FREEZE_DR) || (b = balCar(balSlot(car, human))) == 0)
        return;
    now = GsGetWorkBase_0();
    if (now - b->frozen_at > TM2_BAL_FREEZE_WINDOW)
        b->freeze_len = 0;
    b->frozen_at = now;
}

/* weaponRicochetUpdate, the shot's owner tested (retail: he may be hit once the shot's life is below 398, a human's,
 * or 396, an AI's: 2 or 4 ticks). Bit 18: he may be hit only once the shot has been outside his box (DISTANCE >= 20,
 * the hit test's box) at one of these tests; the shot's byte +0x01 (unused by retail, cleared at the launch)
 * remembers it. `retail` = retail's answer, `dist` = DISTANCE(shot, owner). */
int PORT_TEXT portBalanceRicochetOwner(void *shot, int retail, int dist)
{
    unsigned char *left = (unsigned char *)shot + 1;

    if (!BAL(TM2_BAL_RICOCHET))
        return retail;
    if (dist >= 20)
        *left = 1;
    return *left;
}

/* weaponRicochetUpdate, after each of its two hit passes (hdpCollideObject: the level and every live object with
 * +8 & +0xC set, the shot's owner's car included; retail treats a car's hit box as a wall, its kind = the car type
 * 16..240 is not filtered). Request 45, bit 18: 1 = drop this hit (the caller clears hit.hit: no bounce, no power
 * doubling, no flash, no push-out) when `obj` (the hit record's object, +0x14 of the point) is a car object whose
 * id is the shot's owner and the shot has not yet been outside his box (left_owner, set by
 * portBalanceRicochetOwner). Bit clear: 0 (retail). Reads only; no rand(). */
int PORT_TEXT portBalanceRicochetPass(void *shot, void *obj)
{
    unsigned char *s = (unsigned char *)shot;
    unsigned char human;
    int index;

    if (!BAL(TM2_BAL_RICOCHET) || s[1] != 0 || obj == 0)
        return 0;
    if (*(int *)((unsigned char *)obj + 0x118) != *(short *)(s + 2))     /* the object's id, the shot's owner */
        return 0;
    return carFindByObject(obj, &human, &index) != 0;
}

/* Request 124 (portBalance2 bit 0, BALANCE.md 26): after a shot's hit test, 1 = drop this hit (the caller clears
 * it, as for its own filter) when `kind`, the id of the hit volume, is a pickup's, 401..480. The callers: both of
 * weaponRicochetUpdate's checks (retail's first passes 401..440, its push-out check after a bounce passes none, so a
 * pickup next to a wall turned the bomb back into the wall until its life ran out), weaponProjectileUpdate's and
 * bulletsMove's (401..440). Bit clear: 0, retail's filters alone. Reads only; no rand(). */
int PORT_TEXT portBalancePickupPass(int kind)
{
    return (portBalanceLive2 & TM2_BAL2_PICKUP_PASS) && (unsigned int)(kind - 401) <= TM2_BAL2_PICKUP_LAST - 401;
}

/* Request 125 (portBalance2 bit 1, BALANCE.md 26.4): Dark Tooth's swoop asks the world whether his head is in a box
 * (hdpQueryPoint, no filter: retail ends the swoop at the first box, a pickup's too). `res` is that result: +0 the
 * hit word, +0x0A the id of the hit volume. Returns the hit word as retail reads it (0 = nothing hit), or 0 when bit 1
 * is set and the volume is a pickup's, 401..480. Walls, cars and every other box end the swoop as before. The
 * callers: aiDarkToothBossUpdate (the boss) and portCarDtPlayerSwoopStep (a player's Dark Tooth, request 36).
 * Reads only; no rand(). */
int PORT_TEXT portBalanceSwoopHit(int *res)
{
    if ((portBalanceLive2 & TM2_BAL2_SWOOP_PASS)
        && (unsigned int)(((short *)res)[5] - 401) <= TM2_BAL2_PICKUP_LAST - 401)
        return 0;
    return res[0];
}

/* weaponFireEnergyAttack 104 (bit 19): 1 when the energy jump costs nothing: a Grasshopper (by car type: her
 * passive). Her cooldown is the shared tail's, as retail. */
int PORT_TEXT portBalanceJumpFree(void *car, int human)
{
    return BAL(TM2_BAL_FREE_JUMP) && balType(car, human) == TM2_BAL_GRASSHOPPER_TYPE;
}

/* ---- round 3 (BALANCE.md 13) ---- */

/* bulletsMove, a machine-gun bullet that connected (`hit` = what it hit, weaponApplyHit's type: a car's object id
 * 1..99 except 8, else a target / object; `power` = the bullet's, 1 or 3 with Mega Guns and Minion's rear gun).
 * Bit clear: -1, the caller keeps retail's coin flip (rand() & 1) and `power`. Bit 20: the damage to apply now,
 * for every hit (no coin flip, no rand()): the hit is worth `power` half points, added to the target's carried
 * half point (per car slot, portBalanceCars[slot].gun_half; one shared portBalanceGunObjects for the hits that are
 * not cars); the whole points are applied, the remainder (0 or 1) carried. So a normal bullet does 0, 1, 0, 1 ...
 * and a Mega Guns bullet 1, 2, 1, 2 ...: exactly retail's average (power / 2 a hit), the same on every machine. The
 * caller applies the hit also when it is 0 (carApplyDamage flashes the car, then returns: no HP, no rand()). */
int PORT_TEXT portBalanceGunHit(int hit, int power)
{
    struct TM2PortBalanceCar *b;
    unsigned char h;
    int index, half;
    unsigned int *objects;

    if (!BAL(TM2_BAL_GUNS))
        return -1;
    b = 0;
    if (hit > 0 && hit < 100 && hit != 8 && carFindById(hit, &h, &index))   /* weaponApplyHit's car range */
        b = balCar(h ? humanCarGet(index)->slot : aiCarGet(index)->slot);
    if (b != 0) {
        half = b->gun_half + power;
        b->gun_half = half & 1;
    } else {
        objects = &portBalanceGunObjects;
        half = *objects + power;
        *objects = half & 1;
    }
    return half >> 1;
}

/* request 121 item 4 (bit 9, BALANCE.md 25): bulletSpawn, the new bullet's life (frames of 200 units: retail 12 for a
 * player, 15 for a computer car); with bit 9 a quarter more, rounded: 15 and 19 (3000 and 3800 units). Every bullet
 * of every car (the guns, Mega Guns, Minion's rear gun). No rand(). */
int PORT_TEXT portBalanceGunLife(int life)
{
    return BAL(TM2_BAL_GUN_RANGE) ? (life * TM2_BAL_GUN_RANGE_PCT + 50) / 100 : life;
}
/* ---- request 67 (BALANCE.md 16, PICKUP_LOOKS.md 4): bit 23, no wasted pickup respawn ---- */
/* pickupRespawnTick's one rand() r: with bit 23 the slot to bring back among those that are taken and can show, in
 * slot order: slot i < pickupRegisteredCount (and < 50) whose id is a map pickup (401..430: pickupSetVisible's range),
 * active 0 (pickupApply clears it), whose model (pickupObjects[i]) is hidden (its kind-1 child count +20 / kind-4
 * +18 at 0: pickupSlotsReset leaves active 0 for the pickups the load shows, so the model says which are on the map)
 * and is reached from the level model through the current child of every select (kind 9: a target or level event
 * still whole keeps its pickups in another child; animations, distance switches and zones: every child). Returns
 * r % that count's slot, -1 when there is none (nothing comes back, as retail with nothing to pick); with the bit
 * clear r % pickupRegisteredCount (retail). The caller's health test (417 / 418: a second rand() < 16384) is
 * retail's. Reads only; no rand(). */
#define BAL_PICK_SLOTS 50
#define BAL_PICK_STACK 384
#define BAL_PICK_VISITS 65536   /* nodes visited at most (a damaged tree cannot loop) */
extern int pickupRegisteredCount;
extern unsigned char pickupSlots[];         /* 16-byte entries: +0 id (s16), +2 active (s16) */
extern unsigned char *pickupObjects[];      /* [50] the pickups' model nodes (pickupRegisterObject) */
extern unsigned char *csGetRoot(void);

/* bit i of show[]: pickup model i is reached through the selects' current children */
void PORT_TEXT portBalancePickupShowable(unsigned int *show)
{
    unsigned char *stk[BAL_PICK_STACK];
    unsigned char *nd, **list, *root;
    int depth, n, i, id, k, visits;

    show[0] = show[1] = 0;
    root = csGetRoot();
    if (root == 0 || *(unsigned char **)(root + 284) == 0)
        return;
    stk[0] = *(unsigned char **)(root + 284);
    depth = 1;
    visits = 0;
    while (--depth >= 0 && ++visits <= BAL_PICK_VISITS) {
        nd = stk[depth];
        n = 0;
        list = 0;
        switch (nd[0]) {
        case 1:
        case 4:
            id = *(unsigned short *)(nd + 2);
            if ((unsigned int)(id - 401) < 30) {     /* a pickup node: pickupRegisterObject's index */
                k = nd[0] == 1 ? *(unsigned short *)(nd + 22) : *(unsigned short *)(nd + 16);
                if (k < BAL_PICK_SLOTS)
                    show[k >> 5] |= 1u << (k & 31);
                break;
            }
            if (nd[0] == 1) {
                list = (unsigned char **)(nd + 24);
                n = nd[20];
            } else {
                list = (unsigned char **)(nd + 20);
                n = nd[18];
            }
            break;
        case 2:
            for (i = 0; i < *(int *)(nd + 16) && depth < BAL_PICK_STACK; i++) {
                unsigned char *c = (*(unsigned char ***)(nd + 20 + 4 * i))[2];

                if (c != 0)
                    stk[depth++] = c;
            }
            break;
        case 3:
            list = (unsigned char **)(nd + 32);
            n = nd[11];
            break;
        case 5:
            list = (unsigned char **)(nd + 40);
            n = nd[38];
            break;
        case 9:
            list = (unsigned char **)(nd + 16) + nd[7];
            n = 1;
            break;
        case 11:
            list = (unsigned char **)(nd + 8);
            n = 2;
            break;
        case 12:
            list = (unsigned char **)(nd + 8);
            n = *(unsigned short *)(nd + 6);
            break;
        }
        for (i = 0; i < n && depth < BAL_PICK_STACK; i++)
            if (list[i] != 0)
                stk[depth++] = list[i];
    }
}

/* slot i is taken: a map pickup whose slot is inactive and whose model is hidden */
int PORT_TEXT portBalancePickupTaken(int i)
{
    unsigned char *o;
    int id = *(short *)(pickupSlots + 16 * i);

    if ((unsigned int)(id - 401) >= 30 || *(short *)(pickupSlots + 16 * i + 2) != 0)
        return 0;
    o = pickupObjects[i];
    if (o == 0)
        return 0;
    if (o[0] == 1)
        return o[20] == 0;
    if (o[0] == 4)
        return o[18] == 0;
    return 0;
}

/* carStatsLoad, after the car type's own values and before a mod profile's and the derived fields (request 77,
 * BALANCE.md 17): with bit 24 a Hammerhead's brakes (stats +0x5C) and slide speed (+0x56) are TM2_BAL_HH_BRAKE and
 * TM2_BAL_HH_SLIDE; carStatsSetTopSpeed then derives his braking (+0xCC) and grip (+0xC0) from them. Every load of
 * his type: a human's restart, a computer car's bind and restart, a swap. The bit is read from portBalance, the
 * port's word as it set it at the match's load: computer cars load at bind time, before roundStart's latch
 * (portBalanceLive would still hold the round before's). No rand(). */
void PORT_TEXT portBalanceCarStats(void *stats, int type)
{
    if ((portBalance & TM2_BAL_HH_MOVE) && type == TM2_BAL_HAMMERHEAD_TYPE) {
        *(short *)((unsigned char *)stats + 0x5C) = TM2_BAL_HH_BRAKE;
        *(short *)((unsigned char *)stats + 0x56) = TM2_BAL_HH_SLIDE;
    }
}

int PORT_TEXT portBalancePickupSlot(int r)
{
    unsigned int show[2];
    int i, n, k, count;

    if (!BAL(TM2_BAL_PICKUP_DRAW))
        return r % pickupRegisteredCount;
    portBalancePickupShowable(show);
    count = pickupRegisteredCount < BAL_PICK_SLOTS ? pickupRegisteredCount : BAL_PICK_SLOTS;
    n = 0;
    for (i = 0; i < count; i++)
        if (((show[i >> 5] >> (i & 31)) & 1) && portBalancePickupTaken(i))
            n++;
    if (n == 0)
        return -1;
    k = r % n;
    for (i = 0; i < count; i++)
        if (((show[i >> 5] >> (i & 31)) & 1) && portBalancePickupTaken(i) && k-- == 0)
            return i;
    return -1;
}

/* ---- request 101 (bit 29, BALANCE.md 22): the way by the route graph where the level's tables give none ---- */
/* Route nodes (aiRouteNodes, 24 bytes): x, y, z, the successors next[0..count-1] (at most 4: the PTS's next[4],
 * lane[4]), the route section. The graph is the successor lists; read only, walked on demand (no state, nothing to
 * roll back), the same on every machine. No rand(). */
struct PortBalRouteNode {
    int x, y;
    short z, next[4];
    signed char lane[4], count, route;
};
typedef char portBalRouteNodeSize[sizeof(struct PortBalRouteNode) == 24 ? 1 : -1];
extern int aiRouteNodeCount;
#ifndef TM2_PORT_CARS
extern int aiRouteNodes[];      /* (with TM2_PORT_CARS: struct PortRouteNode, request 85's view) */
#endif
#define BAL_ROUTE ((struct PortBalRouteNode *)aiRouteNodes)

static int PORT_TEXT balRouteCount(void)
{
    return aiRouteNodeCount < 0 ? 0 : aiRouteNodeCount > TM2_BAL_ROUTE_NODES ? TM2_BAL_ROUTE_NODES : aiRouteNodeCount;
}

/* |dx| + |dy| + |dz| from pos to node i (carNearestRoutePoint's measure) */
static int PORT_TEXT balRouteGap(int *pos, int i)
{
    struct PortBalRouteNode *n = &BAL_ROUTE[i];
    int dx = pos[0] - n->x, dy = pos[1] - n->y, dz = pos[2] - n->z;

    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + (dz < 0 ? -dz : dz);
}

/* The node nearest pos, of section sec (sec < 0, or none of it: of any); ties the lower index. */
static int PORT_TEXT balRouteNearest(int *pos, int sec)
{
    int i, d, n, best, bi, pass;

    n = balRouteCount();
    bi = 0;
    for (pass = sec < 0; pass < 2; pass++) {
        best = 0x7FFFFFFF;
        for (i = 0; i < n; i++) {
            if (pass == 0 && BAL_ROUTE[i].route != sec)
                continue;
            d = balRouteGap(pos, i);
            if (d < best) {
                best = d;
                bi = i;
            }
        }
        if (best != 0x7FFFFFFF)
            break;
    }
    return bi;
}

/* The way from node `from` to the target at pos (section sec) by the successor lists (breadth first, the lower
 * index first): the goal is the target's nearest node (balRouteNearest); when the graph cannot reach it, the reached
 * node nearest the target. *blocked: the goal not reached and that node TM2_BAL_ROUTE_NEAR or more from the target.
 * Returns the successor of `from` on the way, `from` itself when it is the goal, -1 when `from` is no node. */
static int PORT_TEXT balRouteWay(int from, int *pos, int sec, int *blocked)
{
    unsigned char prev[TM2_BAL_ROUTE_NODES], queue[TM2_BAL_ROUTE_NODES];
    int n, i, k, m, head, tail, goal, best, d;
    struct PortBalRouteNode *r;

    *blocked = 0;
    n = balRouteCount();
    if (from < 0 || from >= n)
        return -1;
    for (i = 0; i < n; i++)
        prev[i] = 0xFF;
    prev[from] = (unsigned char)from;
    queue[0] = (unsigned char)from;
    head = 0;
    tail = 1;
    while (head < tail) {
        r = &BAL_ROUTE[queue[head++]];
        for (k = 0; k < r->count && k < 4; k++) {
            m = r->next[k];
            if (m >= 0 && m < n && prev[m] == 0xFF) {
                prev[m] = queue[head - 1];
                queue[tail++] = (unsigned char)m;
            }
        }
    }
    goal = balRouteNearest(pos, sec);
    if (prev[goal] == 0xFF) {
        best = 0x7FFFFFFF;
        for (i = 0; i < n; i++)
            if (prev[i] != 0xFF && (d = balRouteGap(pos, i)) < best) {
                best = d;
                goal = i;
            }
        *blocked = best >= TM2_BAL_ROUTE_NEAR;
    }
    while (goal != from && prev[goal] != from)
        goal = prev[goal];
    return goal;
}

int PORT_TEXT portBalanceRouteNearest(int *pos, int retail)
{
    if (!BAL(TM2_BAL_AI_ROUTE))
        return retail;
    return balRouteNearest(pos, -1);
}

/* aiFollowRouteTarget, after retail's choice of `next` (the route table's node for the car's node `node` of section
 * `route` and the target's section `troute`; `carsec` the car's own section): -2 keeps retail's; else the graph's,
 * when the table entry is past the level's nodes, or the target is in the node's section while the car is not (a
 * node tagged with a section it is not in: retail would chase straight): -1 = the car is at the target's node,
 * chase the target; else the node to head for (the car's own when the target cannot be reached: it keeps to the
 * nearest place it can reach). */
int PORT_TEXT portBalanceRouteNext(int node, int carsec, int next, int route, int troute, int *tpos)
{
    int w, blocked;

    if (!BAL(TM2_BAL_AI_ROUTE))
        return -2;
    if (!(next >= aiRouteNodeCount || (troute == route && carsec != route)))
        return -2;
    w = balRouteWay(node, tpos, troute, &blocked);
    if (w < 0)
        return -2;
    if (w == node)
        return blocked ? node : -1;
    return w;
}

/* aiShouldChaseDirect (no direct chase when 1), and the port's targeting: the graph cannot take a car at `node` to
 * the target at tpos (section tsec): neither the target's nearest node nor a node within TM2_BAL_ROUTE_NEAR of it is
 * reached. 0 with the bit clear. */
int PORT_TEXT portBalanceRouteBlocked(int node, int *tpos, int tsec)
{
    int blocked;

    if (!BAL(TM2_BAL_AI_ROUTE))
        return 0;
    if (balRouteWay(node, tpos, tsec, &blocked) < 0)
        return 0;
    return blocked;
}
#undef BAL_ROUTE
#endif /* TM2_PORT_BALANCE */

#ifdef TM2_PORT_VISUAL
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- visual switches (PC port request 42 items 6 and 7, port_visual.h) ---- */

/* weaponArcsDraw with portVisual bit 1 (request 76): the taser's start, Outlaw 2's light bar, for a shooter whose
 * motion.pos is `from` (weaponTaserTarget's origin, weaponArcs[].from): the bar's top on her near model (LOD [0, 200):
 * y -18..-12, z 17..20; +y is the car's front) turned by the car's body matrix motion.body_m (0x50 bytes after pos:
 * rot with the tilt, carMotionUpdate) as the guns turn their muzzle, plus `from`. Read live each draw, so the start
 * stays on the bar as the car moves and turns. A car with a borrowed taser starts at the same place on its own body.
 * Writes `out`, returns it; reads only. */
extern void mathMulTransVec(void *m, void *v, void *out);
int *PORT_TEXT portVisualTaserFrom(int *from, int *out)
{
    int in[3];
    int i;

    in[0] = 0;
    in[1] = TM2_VIS_TASER_SIREN_Y;
    in[2] = TM2_VIS_TASER_SIREN_Z;
    mathMulTransVec((unsigned char *)from + 0x50, in, out);
    for (i = 0; i < 3; i++)
        out[i] += from[i];
    return out;
}

/* weaponArcsDraw with portVisual bit 0: the OT entry of an arc segment whose ends have depth z / 4 = za, zb
 * (weaponArcsDraw's scr[][2], both 1..4090 for a drawn segment). The nearer end's camera depth z is mapped as the
 * level emitter maps a polygon's SZ (z >> 3, then renderCompressDepth 8014D368: linear to 1024, then slopes 1/4,
 * 1/8, 1/32; the same as portFarCompress), TM2_VIS_ARC_BIAS entries in front (the OT is drawn from 4095 down to 0) so
 * the shooter's own body, sorted by its farthest corner, does not cover the arc's start; 1..4094 (entry 0 stays the
 * entry retail draws last). Draws nothing, calls nothing. */
int PORT_TEXT portVisualArcOtz(int za, int zb)
{
    int z = (za < zb ? za : zb) >> 1;      /* (z / 4) >> 1 = z >> 3 */

    if (z >= 1024) {
        if (z < 4096)
            z = ((z - 1024) >> 2) + 1024;
        else if (z < 16384)
            z = ((z - 4096) >> 3) + 1792;
        else
            z = ((z - 16384) >> 5) + 3328;
    }
    z -= TM2_VIS_ARC_BIAS;
    if (z > 4094)
        z = 4094;
    return z < 1 ? 1 : z;
}

/* renderScene with portVisual bit 2 (request 45 item 3, PLAYTEST_ANSWERS.md 3): the scene object of the car of the
 * human whose view `view` is (TM2_LOCAL_HUMAN: with TM2_PORT_HUMANS the local slot's human), to be walked before the
 * level; 0 when the bit is clear, in the mirror view (mode 4), with no human on that view, or when the object is
 * not drawn (+8 == 0) or has no model (+0x11C). Reads only; no rand(). */
extern int viewGetMode(int view);
unsigned char *PORT_TEXT portVisualOwnCar(int view)
{
    unsigned char *o;
    int h;

    if (!(portVisual & TM2_VIS_OWN_CAR) || viewGetMode(view) == 4)
        return 0;
    h = TM2_LOCAL_HUMAN(view);
    if (h < 0 || h >= humanCarTotal || h >= TM2_MAX_HUMANS)
        return 0;
    o = (unsigned char *)humanCarObjects[h];
    if (o == 0 || *(int *)(o + 8) == 0 || *(void **)(o + 284) == 0)
        return 0;
    return o;
}

/* ---- request 99 (RENDER_ANSWERS.md R9): portVisual bit 9, the mirror on the car ---- */

/* The human whose car the mirror view `view` shows: the local human of that view (the port's split-screen mirrors),
 * else view 0's (retail's single-player mirror is view 1); -1 when the bit is clear or its car view is not one of the
 * mirror's (2, 3: car view 4 and the watch view 7 keep retail's). */
static int PORT_TEXT portVisualMirrorHuman(int view)
{
    int h;

    if (!(portVisual & TM2_VIS_MIRROR_CAR))
        return -1;
    h = TM2_LOCAL_HUMAN(view);
    if (h < 0 || h >= humanCarTotal || h >= TM2_MAX_HUMANS)
        h = TM2_LOCAL_HUMAN(0);
    if (h < 0 || h >= humanCarTotal || h >= TM2_MAX_HUMANS)
        return -1;
    if (humanCars[h][0x10C / 4] != 2 && humanCars[h][0x10C / 4] != 3)
        return -1;
    return h;
}

/* viewProc, the mirror view `view` (mode 4), after retail's copy of view 0's camera and its half turn: `pos` (the
 * camera object's +0xA8, the eye in motion.pos's units: its parent pose is zero in the car views) on the car's roof
 * (stats roof_height + TM2_VIS_MIRROR_UP up, TM2_VIS_MIRROR_BACK behind its centre, turned by motion.rot_m), `rot`
 * (+0xA0: pitch, roll, yaw) looking back level along the car's heading (motion.rot yaw + half a turn). Returns 1 when
 * set. Reads the car only; no rand(). */
int PORT_TEXT portVisualMirrorEye(int view, int *pos, short *rot)
{
    unsigned char *c;
    int in[3], out[3], i, h;

    if ((h = portVisualMirrorHuman(view)) < 0)
        return 0;
    c = (unsigned char *)humanCars[h];
    in[0] = 0;
    in[1] = TM2_VIS_MIRROR_BACK;
    in[2] = *(short *)(c + 0x14C + 0x46) + TM2_VIS_MIRROR_UP;     /* stats.roof_height */
    mathMulTransVec(c + 0x6DC + 0x50, in, out);                   /* motion.rot_m */
    for (i = 0; i < 3; i++)
        pos[i] = *(int *)(c + 0x6DC + 0x20 + 4 * i) + out[i];      /* motion.pos */
    rot[0] = TM2_VIS_MIRROR_PITCH;
    rot[1] = 0;
    rot[2] = (*(int *)(c + 0x6DC + 0x2C + 8) + 0x800) & 0xFFF;    /* motion.rot[2], the yaw */
    return 1;
}

/* renderScene's object loop: the car object the view `view` leaves out: with bit 9 in a mirror view (mode 4), the car
 * portVisualMirrorEye put the eye on; else 0. */
unsigned char *PORT_TEXT portVisualMirrorSkip(int view)
{
    int h;

    if (viewGetMode(view) != 4 || (h = portVisualMirrorHuman(view)) < 0)
        return 0;
    return (unsigned char *)humanCarObjects[h];
}

/* ---- request 46 items 4 and 5 (PLAYTEST_ANSWERS.md 11 and 12): portVisual bits 3 and 4 ---- */
#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"
extern int humanCarTotal, aiCarTotal;
extern unsigned char renderMatrixSlots[];
extern unsigned char *renderCtx;
extern unsigned char bulletRecords[];   /* 40 x 60 bytes: pos +0x00, prev +0x0C, owner id +0x24, life +0x28 */
#ifdef TM2_PORT_CAR_GROUPS
#define VIS_BODY(k) ((unsigned char *)portCarGroups.body[k])    /* carBodyPoseUpdate's table, by key >> 4 */
#define VIS_BODIES TM2_CAR_KEYS
typedef char tm2_vis_shadow_keys[(TM2_CAR_KEYS <= TM2_VIS_SHADOW_KEYS) ? 1 : -1];
#else
extern unsigned char *carBodyObjects[];
#define VIS_BODY(k) (carBodyObjects[k])
#define VIS_BODIES 15                   /* carBodyObjectRegister: id >> 4 < 15 */
#endif
#define VS portVisualState

/* The body-object slot (carBodyPoseUpdate's index: key >> 4) of the car whose scene object is `obj`, or -1. */
static int PORT_TEXT portVisualCarKey(unsigned char *obj)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int i;

    for (i = 0; i < humanCarTotal && i < TM2_MAX_HUMANS; i++)
        if ((unsigned char *)humanCarObjects[i] == obj) {
            h = (struct TM2HumanRecord *)humanCars[i];
            return TM2_CAR_KEY(h->slot, h->type) >> 4;
        }
    for (i = 0; i < aiCarTotal && i < TM2_MAX_AI_CARS; i++)
        if ((unsigned char *)aiCarObjects[i] == obj) {
            a = (struct TM2AIRecord *)aiCars[i];
            return TM2_CAR_KEY(a->slot, a->type) >> 4;
        }
    return -1;
}

/* renderScene (bits 3 and 10), after each walk of a world model or an object: remember the draw entries [first, the
 * walker's queue pointer ctx+0x20) it queued when the model is a car's body object (the shadow, walked as a world
 * model) or the object is a car's scene object. Reads only. */
void PORT_TEXT portVisualWalked(unsigned char *obj, void *model, unsigned char *first)
{
    unsigned char *last;
    int k;

    if (!(portVisual & (TM2_VIS_SHADOW | TM2_VIS_SHADOW_GROUND)))
        return;
    last = *(unsigned char **)(renderCtx + 0x20);
    if (last == first)
        return;
    for (k = 0; k < VIS_BODIES; k++)
        if (VIS_BODY(k) != 0 && VIS_BODY(k) == (unsigned char *)model) {
            VS.shadow[k].first = first;
            VS.shadow[k].last = last;
            return;
        }
    k = portVisualCarKey(obj);
    if (k >= 0 && k < TM2_VIS_SHADOW_KEYS) {
        VS.car[k].first = first;
        VS.car[k].last = last;
    }
}

/* One translation component split as the emitter splits it (>> 15 and & 0x7FFF, sign kept on both). */
static void PORT_TEXT portVisualSplit(int v, int *hi, int *lo)
{
    if (v < 0) {
        *hi = -((-v) >> 15);
        *lo = -((-v) & 0x7FFF);
    } else {
        *hi = v >> 15;
        *lo = v & 0x7FFF;
    }
}

/* The farthest OT entry a draw entry's primitives get from the emitter (sub_800CDC54, reference/
 * model_emitter_800CDC54.c): the override word of its light record, else the mesh's fixed entry, else per
 * primitive the deepest SZ of its vertices >> shift, compressed past 1024, the primitive's bias below 1024 (at
 * least 1). SZ = row 3 of the entry's rotation slot times (vertex + translation), as the GTE computes it (the
 * translation's high and low parts as the emitter does). 0 = nothing.
 * `near` (request 102, bit 10): the nearest instead, the shallowest SZ of every vertex, compressed the same way with
 * no bias (a beam: its near end); 0 = nothing, else at least 1. */
static int PORT_TEXT portVisualEntryDepth(int *e, int near)
{
    unsigned char *mesh, *prim, *verts, *v;
    short *m;
    unsigned int ctl;
    int hx, hy, hz, lx, ly, lz, trz, z, sz, otz, best, n, i, k, type, idx[4];

    mesh = (unsigned char *)e[0];
    if (mesh == 0 || (unsigned int)e[6] > 200 || e[5] == 0)
        return 0;
    if (((int *)e[5])[0] != 0)
        return ((int *)e[5])[0];
    if ((((unsigned short *)mesh)[13] >> 3) & 0xFFF)
        return (((unsigned short *)mesh)[13] >> 3) & 0xFFF;
    m = (short *)(renderMatrixSlots + e[6] * 32);
    portVisualSplit(e[1], &hx, &lx);
    portVisualSplit(e[2], &hy, &ly);
    portVisualSplit(e[3], &hz, &lz);
    trz = ((m[6] * hx + m[7] * hy + m[8] * hz) << 3) + ((m[6] * lx + m[7] * ly + m[8] * lz) >> 12);
    verts = *(unsigned char **)(mesh + 4);
    prim = *(unsigned char **)(mesh + 12);
    n = *(int *)(mesh + 16);
    best = near ? 0x7FFFFFFF : 0;
    for (i = 0; i < n && prim != 0; i++, prim += prim[2] * 4) {
        ctl = *(unsigned int *)(prim + 12);
        type = (ctl >> 16) & 7;
        idx[0] = *(short *)(prim + 4);
        k = 1;
        if (type == 3 || type == 4) {
            idx[1] = *(short *)(prim + 6);
            idx[2] = *(short *)(prim + 8);
            k = 3;
            if (type == 4) {
                idx[3] = *(short *)(prim + 10);
                k = 4;
            }
        }
        z = near ? 0x7FFFFFFF : 0;
        while (--k >= 0) {
            v = verts + idx[k] * 8;
            sz = trz + ((m[6] * *(short *)v + m[7] * *(short *)(v + 2) + m[8] * *(short *)(v + 4)) >> 12);
            if (type == 1 && !near)
                sz += __builtin_abs(*(short *)(prim + 8));     /* a beam: its far end */
            if (near ? sz < z : sz > z)
                z = sz;
        }
        if (near) {
            otz = (z < 0 ? 0 : z) >> e[4];
            if (otz >= 4096)
                otz = otz < 16384 ? ((otz - 4096) >> 3) + 1792 : ((otz - 16384) >> 5) + 3328;
            else if (otz >= 1024)
                otz = ((otz - 1024) >> 2) + 1024;
            if (otz < 1)
                otz = 1;
            if (otz < best)
                best = otz;
            continue;
        }
        otz = z >> e[4];
        if (otz >= 1024) {
            if (otz < 4096)
                otz = ((otz - 1024) >> 2) + 1024;
            else if (otz < 16384)
                otz = ((otz - 4096) >> 3) + 1792;
            else
                otz = ((otz - 16384) >> 5) + 3328;
        } else if (otz > 0) {
            otz += (int)(ctl << 19) >> 26;
            if (otz <= 0)
                otz = 1;
        }
        if (otz > best)
            best = otz;
    }
    return near && best == 0x7FFFFFFF ? 0 : best;
}
#define portVisualEntryFar(e) portVisualEntryDepth((e), 0)

/* renderScene (bit 3), before the emitter: every shadow queued this view whose car was walked too gets, for each of
 * its draw entries, the OT entry one farther than the car's farthest primitive (or its own farthest, when that is
 * farther still), 1..4094, through an override pair {entry, the entry's light record} in place of the entry's light
 * pointer (the emitter reads both words from there): the shadow is linked behind every primitive of its car, so the
 * car is drawn over it. Bit 10 (request 102, wins over bit 3; the car walked or not): the entry TM2_VIS_SHADOW_LIFT
 * nearer than the shadow's own nearest corner (at least 1), so every ground polygon under it is drawn first. Then
 * (bits clear too) the view's ranges are cleared. Reads the queue and the models, writes only the shadows' entry
 * word 5; no rand(). */
void PORT_TEXT portVisualShadows(void)
{
    int *e;
    int k, far, own, otz, np, ground;

    np = 0;
    ground = portVisual & TM2_VIS_SHADOW_GROUND;
    for (k = 0; k < TM2_VIS_SHADOW_KEYS; k++) {
        if (VS.shadow[k].first != 0 && (ground || ((portVisual & TM2_VIS_SHADOW) && VS.car[k].first != 0))) {
            far = 0;
            if (ground) {
                own = 0x7FFFFFFF;
                for (e = (int *)VS.shadow[k].first; e < (int *)VS.shadow[k].last; e += 8)
                    if ((otz = portVisualEntryDepth(e, 1)) > 0 && otz < own)
                        own = otz;
                far = own != 0x7FFFFFFF;            /* a shadow polygon to sort */
                otz = own - TM2_VIS_SHADOW_LIFT;
                if (otz < 1)
                    otz = 1;
            } else {
                for (e = (int *)VS.car[k].first; e < (int *)VS.car[k].last; e += 8)
                    if ((otz = portVisualEntryFar(e)) > far)
                        far = otz;
                own = 0;
                for (e = (int *)VS.shadow[k].first; e < (int *)VS.shadow[k].last; e += 8)
                    if ((otz = portVisualEntryFar(e)) > own)
                        own = otz;
                otz = far + 1 > own ? far + 1 : own;
            }
            if (otz > 4094)
                otz = 4094;
            if (far > 0)
                for (e = (int *)VS.shadow[k].first; e < (int *)VS.shadow[k].last && np < TM2_VIS_SHADOW_PAIRS;
                     e += 8) {
                    if (e[5] == 0)
                        continue;
                    VS.pairs[np][0] = otz;
                    VS.pairs[np][1] = ((int *)e[5])[1];
                    e[5] = (int)VS.pairs[np];
                    np++;
                }
        }
        VS.shadow[k].first = VS.shadow[k].last = 0;
        VS.car[k].first = VS.car[k].last = 0;
    }
}

/* ---- request 92 (RENDER_ANSWERS.md R7): portVisual bit 7, the steady shadows ---- */
extern void mathMulMatVec(void *m, void *v, void *out);
extern int ratan2(int y, int x);
extern void *RotMatrixYXZ(void *rot, void *m);

/* Car `wheels` / `motion` (human or computer) whose shadow is body object k: the step's matrix saved once, then
 * carBodyPoseUpdate's pose without the wheels' heights over the ground in the tilt (a[2], b[2] 0: the tilt is the car's
 * own, motion.rot_m), its translation the step's (x, y the car's, z the ground's). A second car of the same body object
 * (without car groups: one per type) poses it again, as the step's last one did. */
static void PORT_TEXT portVisualSteadyPose(int k, struct TM2CarWheels *c, struct TM2CarMotion *m)
{
    unsigned char *body;
    int a[3], b[3], ca[3], cb[3], t[3], i;
    short r[4];

    if ((unsigned int)k >= VIS_BODIES || (body = VIS_BODY(k)) == 0)
        return;
    if (!portSteadyShadow.saved[k]) {
        for (i = 0; i < 8; i++)
            portSteadyShadow.m[k][i] = ((int *)(body + 4))[i];
        portSteadyShadow.saved[k] = 1;
    }
    a[0] = c->wheel[3].pos[0] - c->wheel[2].pos[0];
    a[1] = c->wheel[3].pos[1] - c->wheel[2].pos[1];
    a[2] = 0;
    b[0] = c->wheel[0].pos[0] - c->wheel[2].pos[0];
    b[1] = c->wheel[0].pos[1] - c->wheel[2].pos[1];
    b[2] = 0;
    mathMulMatVec(&m->rot_m, a, ca);
    mathMulMatVec(&m->rot_m, b, cb);
    r[0] = ratan2(cb[2], cb[1]);
    r[1] = -ratan2(ca[2], ca[0]);
    r[2] = m->rot[2];
    r[3] = 0;
    for (i = 0; i < 3; i++)
        t[i] = ((int *)(body + 4))[5 + i];
    RotMatrixYXZ(r, body + 4);
    for (i = 0; i < 3; i++)
        ((int *)(body + 4))[5 + i] = t[i];
}

/* renderScene, before its walks (bit 7): every car's shadow re-posed for this view (portVisualSteadyPose), the
 * humans' first, then the computer cars'. Returns how many cars it posed (0: the bit clear). */
int PORT_TEXT portVisualSteadyBegin(void)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int i, n, done;

    if (!(portVisual & TM2_VIS_STEADY_SHADOW))
        return 0;
    done = 0;
    n = humanCarTotal;
    for (i = 0; i < n; i++) {
        h = humanCarGet(i);
#ifdef TM2_PORT_CAR_GROUPS
        portVisualSteadyPose(TM2_CAR_KEY(h->slot, h->type) >> 4, &h->wheels, &h->motion);
#else
        portVisualSteadyPose(h->type >> 4, &h->wheels, &h->motion);
#endif
        done++;
    }
    n = aiCarTotal;
    for (i = 0; i < n; i++) {
        a = aiCarGet(i);
#ifdef TM2_PORT_CAR_GROUPS
        portVisualSteadyPose(TM2_CAR_KEY(a->slot, a->type) >> 4, &a->wheels, &a->motion);
#else
        portVisualSteadyPose(a->type >> 4, &a->wheels, &a->motion);
#endif
        done++;
    }
    return done;
}

/* renderScene, after its walks (bit 7): every saved shadow matrix back (the step's, which the rollback hash and the
 * next step see). */
void PORT_TEXT portVisualSteadyEnd(void)
{
    unsigned char *body;
    int k, i;

    for (k = 0; k < TM2_VIS_SHADOW_KEYS; k++)
        if (portSteadyShadow.saved[k]) {
            portSteadyShadow.saved[k] = 0;
            if (k < VIS_BODIES && (body = VIS_BODY(k)) != 0)
                for (i = 0; i < 8; i++)
                    ((int *)(body + 4))[i] = portSteadyShadow.m[k][i];
        }
}

/* ---- request 93 (RENDER_ANSWERS.md R8): portVisual bit 8, every polygon whole ---- */

/* renderScene and renderDrawModel, as they set the emitter's near threshold (scratchpad +0x40; `retail` is their 70 /
 * 20): with bit 8 0, so no polygon's OT index is below it and the emitter defers none to the near pass. */
int PORT_TEXT portVisualNearOtz(int retail)
{
    return (portVisual & TM2_VIS_WHOLE_NEAR) ? 0 : retail;
}

/* bulletSpawn (bit 4, recorded always: drawing state only): bullet `slot` was just set up; note its shooter's
 * position (motion.pos of the human or computer car with the bullet's owner id: TM2_HUMAN_ID(i), or 50 + the AI
 * index) and the life it starts with. */
void PORT_TEXT portVisualBulletSpawn(int slot)
{
    struct TM2PortVisualBullet *b;
    unsigned char *r;
    int owner, i;

    if ((unsigned int)slot >= TM2_VIS_BULLETS)
        return;
    b = &VS.bullets[slot];
    r = bulletRecords + slot * 60;
    owner = *(int *)(r + 0x24);
    b->pos = 0;
    b->life = *(int *)(r + 0x28);
    if (owner >= 50) {
        if (owner - 50 < aiCarTotal && owner - 50 < TM2_MAX_AI_CARS)
            b->pos = ((struct TM2AIRecord *)aiCars[owner - 50])->motion.pos;
    } else {
        for (i = 0; i < humanCarTotal && i < TM2_MAX_HUMANS; i++)
            if (TM2_HUMAN_ID(i) == owner) {
                b->pos = ((struct TM2HumanRecord *)humanCars[i])->motion.pos;
                break;
            }
    }
    if (b->pos != 0)
        for (i = 0; i < 3; i++)
            b->at[i] = b->pos[i];
}

/* streaksDraw (bit 4): the tail point to draw for bullet `slot` (the record's previous point `tail`). In the
 * bullet's first moved frame (one bulletsMove since the shot: life one below the start) the tail is the shot's
 * point moved by its shooter's own move since the shot (the muzzle where it is now; the car's turn is not
 * followed), else `tail` itself. Reads only. */
int *PORT_TEXT portVisualBulletTail(int slot, int *tail)
{
    struct TM2PortVisualBullet *b;
    int i;

    if (!(portVisual & TM2_VIS_GUN_START) || (unsigned int)slot >= TM2_VIS_BULLETS)
        return tail;
    b = &VS.bullets[slot];
    if (b->pos == 0 || *(int *)(bulletRecords + slot * 60 + 0x28) != b->life - 1)
        return tail;
    for (i = 0; i < 3; i++)
        VS.tail[i] = tail[i] + b->pos[i] - b->at[i];
    return VS.tail;
}
/* ---- request 79 (PICKUP_LOOKS.md 10): the special's pickup look ---- */
#define PS_W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))
extern unsigned char *pickupObjects[];      /* [50] the pickups' model nodes (pickupRegisterObject) */

/* A mesh of one primitive whose first is a textured quad's camera-facing beam (kind 1): 1. */
static int PORT_TEXT portPickupSpecialBeam(unsigned char *mesh)
{
    unsigned char *p;

    if (mesh == 0 || mesh[0] != 0 || PS_W(mesh, 16) != 1)
        return 0;
    p = (unsigned char *)PS_W(mesh, 12);
    return ((PS_W(p, 12) >> 16) & 7) == 1 && (p[0] & 0x7F) == 4;
}

/* The weapon pickup model `m` is the usual shape (PICKUP_LOOKS.md 1): a group whose first child is the card (a beam)
 * and whose last is the hit box. */
static int PORT_TEXT portPickupSpecialDonor(unsigned char *m)
{
    if (m == 0 || m[0] != 1 || m[20] < 2 || !portPickupSpecialBeam((unsigned char *)PS_W(m, 24)))
        return 0;
    return ((unsigned char *)PS_W(m, 24 + (m[20] - 1) * 4))[0] == 8;
}

/* One beam of the look: the mesh `out` (the donor's `from`'s header), its primitive `prim` (an unlit FT4 beam of half
 * width `w` and half length `h`, colour `rgb`, the texture of the HUD icon mesh `icon`'s first primitive: its texel
 * box, its CLUT and its page, upright: v0 top-left as the weapons' cards) and its centre `vert` (the donor's).
 * Returns 0 when the icon is not a textured quad. */
static int PORT_TEXT portPickupSpecialMesh(unsigned char *out, unsigned char *from, unsigned int *prim, short *vert,
                                           unsigned char *icon, int w, int h, unsigned int rgb)
{
    unsigned char *ip, *fp = (unsigned char *)PS_W(from, 12);
    unsigned int t[4], u0 = 255, u1 = 0, v0 = 255, v1 = 0, u, v;
    short *c;
    int i;

    if (icon == 0 || icon[0] != 0 || PS_W(icon, 16) == 0)
        return 0;
    ip = (unsigned char *)PS_W(icon, 12);
    if ((ip[0] & 0x7F) != 4)
        return 0;
    for (i = 0; i < 4; i++) {
        t[i] = PS_W(ip, 16 + ip[1] * 4 + i * 4);
        u = t[i] & 0xFF;
        v = (t[i] >> 8) & 0xFF;
        if (u < u0) u0 = u;
        if (u > u1) u1 = u;
        if (v < v0) v0 = v;
        if (v > v1) v1 = v;
    }
    for (i = 0; i < 44; i++)
        out[i] = from[i];
    c = (short *)((unsigned char *)PS_W(from, 4) + *(short *)(fp + 4) * 8);
    for (i = 0; i < 4; i++)
        vert[i] = i < 3 ? c[i] : 0;
    prim[0] = 4 | 1 << 8 | 9 << 16 | 9 << 24;           /* 4 texture words, 1 colour, 9 words, an FT4 packet */
    prim[1] = (unsigned int)w << 16;                    /* the centre: vertex 0; the half width */
    prim[2] = (unsigned int)h;                          /* the half length (up and down from the centre) */
    prim[3] = 0x15810000;                               /* kind 1 (beam), unlit, stride 2, 5 words to normals, 1 */
    prim[4] = 0x2C000000 | rgb;                         /* FT4, opaque, shaded by the colour */
    prim[5] = u0 | v0 << 8 | (t[0] & 0xFFFF0000);       /* + the CLUT */
    prim[6] = u1 | v0 << 8 | (t[1] & 0xFFFF0000);       /* + the page */
    prim[7] = u0 | v1 << 8;
    prim[8] = u1 | v1 << 8;
    PS_W(out, 4) = (unsigned int)vert;
    PS_W(out, 12) = (unsigned int)prim;
    PS_W(out, 16) = 1;
    PS_W(out, 28) = 0;                                  /* the sphere about the centre, holding the beam */
    PS_W(out, 32) = 0;
    PS_W(out, 36) = (unsigned int)(int)vert[2];
    PS_W(out, 40) = (unsigned int)(w > h ? w : h) * 3 / 2;
    return 1;
}

/* worldLoadStartObjects, after the level's objects (request 79, PICKUP_LOOKS.md 10): with the load's HUD icon object
 * (worldLoadObject's 1260) and a weapon pickup of the usual shape (pickupObjects, the first of 401, 403, 404, 406,
 * 411, 413, 419), portPickupSpecial.model is the special's look: the donor's group (its sphere) holding the card
 * (the HUD's selected icon 0, the SPECIAL globe) and the donor's hit box (request 83: no blinking name). Otherwise 0.
 * The HUD object is forgotten either way (a load without one builds nothing). No rand(). */
void PORT_TEXT portPickupSpecialBuild(void)
{
    struct TM2PortPickupSpecial *s = &portPickupSpecial;
    unsigned char *hud = (unsigned char *)s->hud, *o, *m = 0;
    int i, id;

    s->model = 0;
    s->donor = 0;
    s->hud = 0;
    if (hud == 0 || hud[0] != 1 || hud[20] < 5)
        return;
    for (i = 0; i < 50 && m == 0; i++) {
        o = pickupObjects[i];
        if (o == 0)
            continue;
        id = *(unsigned short *)(o + 2);
        if (id != 401 && id != 403 && id != 404 && id != 406 && id != 411 && id != 413 && id != 419)
            continue;
        m = (unsigned char *)(o[0] == 1 ? PS_W(o, 24) : o[0] == 4 ? PS_W(o, 20) : 0);
        if (!portPickupSpecialDonor(m))
            m = 0;
    }
    if (m == 0)
        return;
    if (!portPickupSpecialMesh(s->mesh, (unsigned char *)PS_W(m, 24), s->prim, s->vert,
                               (unsigned char *)PS_W(hud, 40), TM2_PICKUP_SPECIAL_CARD, TM2_PICKUP_SPECIAL_CARD,
                               TM2_PICKUP_SPECIAL_CARD_RGB))
        return;
    for (i = 0; i < 24; i++)
        s->group[i] = m[i];
    s->group[20] = 2;
    PS_W(s->group, 24) = (unsigned int)s->mesh;
    PS_W(s->group, 28) = PS_W(m, 24 + (m[20] - 1) * 4); /* the donor's hit box */
    s->donor = m;
    s->model = s->group;
}

/* ---- request 81 (PICKUP_LOOKS.md 11): a mine's and the freeze shot's pictures, for the port's kill feed ---- */
extern void *weaponMineObjects[];           /* the mine pool (asset 2212's scene objects, the model at +0x11C) */
extern int weaponFreezeMissileModel;        /* asset 2202 (weaponStoreProjectileModel) */

/* The mesh under `node`: through groups (the first child), LOD nodes (the first record's child) and animations
 * (frame `frame` modulo their frame count); 0 when another node type or nothing comes first. */
static unsigned char *PORT_TEXT portWeaponIconMesh(unsigned char *node, int frame)
{
    int depth;

    for (depth = 0; node != 0 && depth < 8; depth++) {
        if (node[0] == 0)
            return node;
        if (node[0] == 1 && node[20] != 0)
            node = (unsigned char *)PS_W(node, 24);
        else if (node[0] == 2 && PS_W(node, 16) != 0)
            node = (unsigned char *)PS_W(PS_W(node, 20), 8);
        else if (node[0] == 3 && node[11] != 0)
            node = (unsigned char *)PS_W(node, 32 + (frame % node[11]) * 4);
        else
            return 0;
    }
    return 0;
}

int PORT_TEXT portWeaponIcon(int which, int frame, struct TM2PortIcon *out)
{
    unsigned char *model = 0, *mesh, *p, *best = 0;
    unsigned int t, u0 = 255, u1 = 0, v0 = 255, v1 = 0, u, v;
    int i, n;

    if (which == TM2_ICON_MINE && weaponMineObjects[0] != 0)
        model = (unsigned char *)PS_W(weaponMineObjects[0], 0x11C);
    else if (which == TM2_ICON_FREEZE)
        model = (unsigned char *)weaponFreezeMissileModel;
    if (frame < 0)
        frame = 0;
    mesh = portWeaponIconMesh(model, frame);
    if (mesh == 0)
        return 0;
    /* the largest textured quad sprite or beam (kinds 0 and 1: +6 the half width) */
    p = (unsigned char *)PS_W(mesh, 12);
    n = (int)PS_W(mesh, 16);
    for (i = 0; i < n; i++, p += p[2] * 4)
        if (((PS_W(p, 12) >> 16) & 7) <= 1 && (p[0] & 0x7F) == 4 &&
            (best == 0 || *(short *)(p + 6) > *(short *)(best + 6)))
            best = p;
    if (best == 0)
        return 0;
    for (i = 0; i < 4; i++) {
        t = PS_W(best, 16 + best[1] * 4 + i * 4);
        if (i == 0)
            out->clut = (unsigned short)(t >> 16);
        if (i == 1)
            out->tpage = (unsigned short)(t >> 16);
        u = t & 0xFF;
        v = (t >> 8) & 0xFF;
        if (u < u0) u0 = u;
        if (u > u1) u1 = u;
        if (v < v0) v0 = v;
        if (v > v1) v1 = v;
    }
    out->u = (unsigned char)u0;
    out->v = (unsigned char)v0;
    out->w = (unsigned char)(u1 - u0 + 1);
    out->h = (unsigned char)(v1 - v0 + 1);
    out->colour = PS_W(best, 16);
    return 1;
}

/* ---- request 63 (FAR_LOD.md 10): portVisual bit 5, the chase camera's eye kept inside the level ---- */
/* viewChaseCamera, after the look-at point: the segment from `look` to `eye` is tested against the level model's hit
 * volumes (hdp kind 8 leaves, the convex boxes of up to six planes the cars collide with) and `eye` is pulled in to
 * TM2_VIS_CAMERA_MARGIN in front of the first solid one it enters (at least TM2_VIS_CAMERA_NEAR from `look`). The tree
 * is walked as the cars' pass walks it (hdpPassRun / hdpWalk 8014EA60): translations (4), rotations (5, hdpRotVecSplit's
 * split precision), level changes (12), the current frame / child of an animation (3) / select (9); a distance switch
 * (2) and a zone (11) choose by the look-at point (the car's bands), a bounding sphere (1) is entered when it can touch
 * the segment. A volume holding the look-at point is skipped (a carved space it stands in); the "mark" volumes
 * (n[5] == -2, which free their points from the solids in retail's pass) cut their part out of the solids' spans.
 * Reads the level model only (no hdp state, no scratchpad, no contact record, no rand()); stores only into `eye`
 * and its work (portCameraWallWork: request 89, its pending nodes and spans were 3.5 KB of stack). Each machine's own
 * camera: portVisual, the human records' camera block and portCameraWallWork are hash no. */
#define PCW_STACK 96            /* pending nodes (deeper pushes are dropped) */
#define PCW_SPANS 32            /* solid / mark spans kept per walk */
#define PCW_HOLDS 8             /* request 100: solids holding the look-at point, skipped by the probes too */
#define PCW_VISITS 16384        /* nodes visited per walk at most (a damaged tree cannot loop) */
extern unsigned char *csGetRoot(void);
extern int renderGetWorldShift(void);
struct PortCamWalk {
    unsigned char *node;
    int a[3], b[3];             /* the segment's start (the look-at point, or a probe's eye) and end in the node's
                                   frame */
    int lv;                     /* the frame's level (hdp: the probe's lod >> 16) */
};
/* request 89 (ONLINE_ROLLBACK.md 25.2): the walk's pending nodes and spans, in portCameraWallWork (.port_bss, every
 * variant) instead of the stack: each call fills what it reads (the stack before `depth`, the spans before `ns` /
 * `nm`, request 100's holds before `nhold`), and the camera is drawn by one view at a time */
struct PortCamWork {
    struct PortCamWalk stk[PCW_STACK];
    int solid[PCW_SPANS][2], mark[PCW_SPANS][2];
    unsigned char *hold[PCW_HOLDS];
    int nhold;
};
extern int portCameraWallWork[];
typedef char portCameraWallWorkFits[sizeof(struct PortCamWork) <= TM2_PORT_CAMERA_WORK ? 1 : -1];

/* out = R v >> 12 in hdpRotVecSplit's split precision (R: 3 x 3 s16 at m, rows) */
void PORT_TEXT portCameraWallRot(short *m, int *v, int *out)
{
    int hi[3], lo[3], o[3], i, x;

    for (i = 0; i < 3; i++) {
        x = v[i];
        if (x < 0) {
            x = -x;
            hi[i] = -(x >> 15);
            lo[i] = -(x & 0x7FFF);
        } else {
            hi[i] = x >> 15;
            lo[i] = x & 0x7FFF;
        }
    }
    for (i = 0; i < 3; i++)
        o[i] = ((m[3 * i] * lo[0] + m[3 * i + 1] * lo[1] + m[3 * i + 2] * lo[2]) >> 12)
             + ((m[3 * i] * hi[0] + m[3 * i + 1] * hi[1] + m[3 * i + 2] * hi[2]) << 3);
    for (i = 0; i < 3; i++)
        out[i] = o[i];
}

/* hdpBox's plane values of v against the box at b (count `planes`: 3, or 4..6 with the second matrix); returns how
 * many (inside = all > 0) */
int PORT_TEXT portCameraWallPlanes(unsigned char *b, int planes, int *v, int *f)
{
    int o[3];

    portCameraWallRot((short *)b, v, o);
    f[0] = o[0] + *(int *)(b + 20);
    f[1] = o[1] + *(int *)(b + 24);
    f[2] = o[2] + *(int *)(b + 28);
    if (planes < 4)
        return 3;
    portCameraWallRot((short *)(b + 32), v, o);
    f[3] = o[0] + *(int *)(b + 52);
    f[4] = o[1] + *(int *)(b + 56);
    f[5] = o[2] + *(int *)(b + 60);
    return planes > 6 ? 6 : planes;
}

/* num / den in 1/4096ths, 0 <= num <= den, den > 0 */
int PORT_TEXT portCameraWallFrac(int num, int den)
{
    while (den >= 0x80000) {
        num >>= 1;
        den >>= 1;
    }
    if (den <= 0)
        return 0;
    return (num << 12) / den;
}

/* the part of the segment a..b (node frame) inside the box: t[0], t[1] in 1/4096ths of the segment; 0 = none, 1 = a
 * span, 2 = a span that starts inside (a is in the box) */
int PORT_TEXT portCameraWallClip(unsigned char *box, int planes, int *a, int *b, int *t)
{
    int fa[6], fb[6], n, i, x, t0, t1, in;

    n = portCameraWallPlanes(box, planes, a, fa);
    portCameraWallPlanes(box, planes, b, fb);
    t0 = 0;
    t1 = 4096;
    in = 1;
    for (i = 0; i < n; i++) {
        if (fa[i] > 0) {
            if (fb[i] > 0)
                continue;
            x = portCameraWallFrac(fa[i], fa[i] - fb[i]);       /* leaves by this plane */
            if (x < t1)
                t1 = x;
        } else {
            in = 0;
            if (fb[i] <= 0)
                return 0;
            x = portCameraWallFrac(-fa[i], fb[i] - fa[i]);      /* enters by it */
            if (x > t0)
                t0 = x;
        }
        if (t0 >= t1)
            return 0;
    }
    t[0] = t0;
    t[1] = t1;
    return in ? 2 : 1;
}

/* can a sphere (centre c, radius r, node frame) touch the segment a..b? (conservative: the segment's own sphere) */
int PORT_TEXT portCameraWallNear(int *c, int r, int *a, int *b)
{
    int d[3], i, e, h, lim;
    unsigned int sum;

    if (r >= 0xB504)
        return 1;           /* retail: always entered */
    h = 0;
    for (i = 0; i < 3; i++) {
        e = b[i] - a[i];
        h += e < 0 ? -e : e;
        d[i] = c[i] - (a[i] + (e >> 1));
        if (d[i] < 0)
            d[i] = -d[i];
    }
    lim = r + (h >> 1) + 2;
    for (i = 0; i < 3; i++)
        if (d[i] > lim)
            return 0;
    while (lim > 0x7FFF) {
        lim >>= 1;
        d[0] >>= 1;
        d[1] >>= 1;
        d[2] >>= 1;
    }
    lim++;
    sum = (unsigned int)(d[0] * d[0]) + (unsigned int)(d[1] * d[1]) + (unsigned int)(d[2] * d[2]);
    return sum <= (unsigned int)(lim * lim);
}

/* the integer square root of v */
int PORT_TEXT portCameraWallSqrt(unsigned int v)
{
    unsigned int r = 0, bit = 1u << 30;

    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else
            r >>= 1;
        bit >>= 2;
    }
    return (int)r;
}

/* One walk of the level model from `node` along the segment a..b (world): the first point (1/4096ths of the segment)
 * of a solid span that no mark span covers, 4096 for none. The look-at-to-eye segment (`probe` 0, request 63): a solid
 * holding a (a carved space the car stands in) is skipped and listed in the work's holds. A clearance probe from the
 * eye (`probe` 1, request 100): a solid holding a counts from 0 (a mark holding a cuts it: the eye in a carved
 * passage), and the listed holds are skipped. The distance switches and zones choose by a. */
int PORT_TEXT portCameraWallWalk(unsigned char *node, int *a, int *b, int probe)
{
    struct PortCamWork *w = (struct PortCamWork *)portCameraWallWork;
    struct PortCamWalk cur;
    int f[6];
    int ns, nm, depth, visits, i, j, k, n, best, t, sh, s[2], v[3], o[3];
    unsigned int sum;
    unsigned int *r;
    unsigned char *nd, *m, **list;

    cur.node = node;
    for (i = 0; i < 3; i++) {
        cur.a[i] = a[i];
        cur.b[i] = b[i];
    }
    cur.lv = renderGetWorldShift();
    ns = nm = depth = visits = 0;
    while (++visits <= PCW_VISITS) {
        nd = cur.node;
        n = 0;
        list = 0;
        switch (nd[0]) {
        case 1:     /* bounding sphere: centre +4, radius +16, flags +21 (1 off, 4 always); children +24 (+20) */
            if (!(nd[21] & 1) && ((nd[21] & 4) || portCameraWallNear((int *)(nd + 4), *(int *)(nd + 16), cur.a,
                                                                      cur.b))) {
                list = (unsigned char **)(nd + 24);
                n = nd[20];
            }
            break;
        case 2:     /* distance switch: the record {max, min, child} holding the start's squared distance */
            sh = cur.lv;
            sum = 0;
            for (i = 0; i < 3; i++) {
                k = (short)((*(int *)(nd + 4 + 4 * i) - cur.a[i]) >> sh);       /* the GTE's 16-bit IR */
                sum += (unsigned int)(k * k);
            }
            for (i = 0; i < *(int *)(nd + 16); i++) {
                r = *(unsigned int **)(nd + 20 + 4 * i);
                if (sum >= r[1] && sum < r[0]) {
                    list = (unsigned char **)&r[2];
                    n = 1;
                    break;
                }
            }
            break;
        case 3:     /* animation: the current frame */
            list = (unsigned char **)(nd + 32) + nd[12];
            n = 1;
            break;
        case 4:     /* translation +4; children +20 (+18), flag +19 */
            if (!(nd[19] & 1)) {
                for (i = 0; i < 3; i++) {
                    cur.a[i] -= *(int *)(nd + 4 + 4 * i);
                    cur.b[i] -= *(int *)(nd + 4 + 4 * i);
                }
                list = (unsigned char **)(nd + 20);
                n = nd[18];
            }
            break;
        case 5:     /* rotation +4, translation +24; children +40 (+38), flag +39 */
            if (!(nd[39] & 1)) {
                for (i = 0; i < 3; i++) {
                    v[i] = cur.a[i] - *(int *)(nd + 24 + 4 * i);
                    o[i] = cur.b[i] - *(int *)(nd + 24 + 4 * i);
                }
                portCameraWallRot((short *)(nd + 4), v, cur.a);
                portCameraWallRot((short *)(nd + 4), o, cur.b);
                list = (unsigned char **)(nd + 40);
                n = nd[38];
            }
            break;
        case 8:     /* hit volume: box +16 (planes +4), +5 the plane rule (-2: a mark volume), +7 == 1 off */
            if (nd[7] != 1) {
                k = portCameraWallClip(nd + 16, nd[4], cur.a, cur.b, s);
                if ((signed char)nd[5] == -2) {
                    if (k && nm < PCW_SPANS) {
                        w->mark[nm][0] = s[0];
                        w->mark[nm][1] = s[1];
                        nm++;
                    }
                } else if (k == 2 && !probe) {
                    if (w->nhold < PCW_HOLDS)
                        w->hold[w->nhold++] = nd;
                } else if (k && ns < PCW_SPANS) {
                    for (i = 0; probe && i < w->nhold && w->hold[i] != nd; i++)
                        ;
                    if (!probe || i >= w->nhold) {
                        w->solid[ns][0] = s[0];
                        w->solid[ns][1] = s[1];
                        ns++;
                    }
                }
            }
            break;
        case 9:     /* select: child +7 */
            list = (unsigned char **)(nd + 16) + nd[7];
            n = 1;
            break;
        case 11:    /* zone: +12 when the start is in one of the boxes at +16 (count +6), else +8 */
            if (!(nd[4] & 1)) {
                list = (unsigned char **)(nd + 8);
                k = nd[6];
                i = 0;
                do {
                    m = ((unsigned char **)(nd + 16))[i];
                    j = portCameraWallPlanes(m + 4, m[0], cur.a, f);
                    while (--j >= 0 && f[j] > 0)
                        ;
                    if (j < 0) {
                        list = (unsigned char **)(nd + 12);
                        break;
                    }
                } while (++i < k);
                n = 1;
            }
            break;
        case 12:    /* level change +4; children +8 (+6) */
            k = *(unsigned short *)(nd + 4);
            sh = cur.lv - k;
            for (i = 0; i < 3; i++) {
                if (sh < 0) {
                    cur.a[i] <<= -sh;
                    cur.b[i] <<= -sh;
                } else {
                    cur.a[i] >>= sh;
                    cur.b[i] >>= sh;
                }
            }
            cur.lv = k;
            list = (unsigned char **)(nd + 8);
            n = *(unsigned short *)(nd + 6);
            break;
        }
        for (i = 0; i < n; i++) {
            if (list[i] != 0 && depth < PCW_STACK) {
                w->stk[depth] = cur;
                w->stk[depth].node = list[i];
                depth++;
            }
        }
        if (--depth < 0)
            break;
        cur = w->stk[depth];
    }
    /* the first point of a solid span that no mark span covers */
    best = 4096;
    for (i = 0; i < ns; i++) {
        t = w->solid[i][0];
        for (k = 0; k < nm; k++)
            for (j = 0; j < nm; j++)
                if (w->mark[j][0] <= t && t < w->mark[j][1])
                    t = w->mark[j][1];
        if (t <= w->solid[i][1] && t < best)
            best = t;
    }
    return best;
}

/* request 63: the eye pulled in to TM2_VIS_CAMERA_MARGIN before the point `best` (1/4096ths) of look..eye, at least
 * TM2_VIS_CAMERA_NEAR from look */
void PORT_TEXT portCameraWallPull(int *look, int *eye, int best)
{
    int i, k, t, sh, len, d[3], v[3];
    unsigned int sum;

    sh = 0;
    for (i = 0; i < 3; i++) {
        d[i] = eye[i] - look[i];
        k = d[i] < 0 ? -d[i] : d[i];
        while ((k >> sh) > 0x7FFF)
            sh++;
    }
    sum = 0;
    for (i = 0; i < 3; i++)
        sum += (unsigned int)((d[i] >> sh) * (d[i] >> sh));
    len = portCameraWallSqrt(sum) << sh;
    if (len <= TM2_VIS_CAMERA_NEAR)
        return;
    t = best - portCameraWallFrac(TM2_VIS_CAMERA_MARGIN, len);
    k = portCameraWallFrac(TM2_VIS_CAMERA_NEAR, len);
    if (t < k)
        t = k;
    if (t >= 4096)
        return;
    for (i = 0; i < 3; i++)
        v[i] = look[i] + (d[i] >> 12) * t + (((d[i] & 0xFFF) * t) >> 12);
    eye[0] = v[0];          /* (constant offsets: the stores into the human record's camera block, +0x14..+0x1F) */
    eye[1] = v[1];
    eye[2] = v[2];
}

void PORT_TEXT portVisualCameraWall(int *look, int *eye)
{
    struct PortCamWork *w = (struct PortCamWork *)portCameraWallWork;
    int i, j, k, t, h[2], p[3], v[3];
    unsigned char *root;

    if (!(portVisual & TM2_VIS_CAMERA_WALL))
        return;
    root = csGetRoot();
    if (root == 0 || *(unsigned char **)(root + 284) == 0)
        return;
    root = *(unsigned char **)(root + 284);
    w->nhold = 0;
    t = portCameraWallWalk(root, look, eye, 0);
    if (t < 4096)
        portCameraWallPull(look, eye, t);
    /* request 100: the clearance. A probe twice TM2_VIS_CAMERA_CLEAR long from the eye each way along each axis
       (the free space h[0] back, h[1] forward); the eye moved off what is nearer than the clearance (a wall:
       sideways, a floor: up), to the middle of the free space when it is narrower than twice the clearance (a
       tunnel). Each move is at most the clearance and stays in the free space the probes found: the eye is never
       pulled toward the car more than that. */
    for (k = 0; k < 3; k++) {
        for (j = 0; j < 2; j++) {
            for (i = 0; i < 3; i++)
                p[i] = eye[i];
            p[k] += j ? 2 * TM2_VIS_CAMERA_CLEAR : -2 * TM2_VIS_CAMERA_CLEAR;
            t = portCameraWallWalk(root, eye, p, 1);
            h[j] = (t * (2 * TM2_VIS_CAMERA_CLEAR)) >> 12;
        }
        if (h[0] + h[1] < 2 * TM2_VIS_CAMERA_CLEAR)
            t = (h[1] - h[0]) / 2;
        else if (h[0] < TM2_VIS_CAMERA_CLEAR)
            t = TM2_VIS_CAMERA_CLEAR - h[0];
        else if (h[1] < TM2_VIS_CAMERA_CLEAR)
            t = h[1] - TM2_VIS_CAMERA_CLEAR;
        else
            t = 0;
        v[k] = eye[k] + t;
    }
    eye[0] = v[0];
    eye[1] = v[1];
    eye[2] = v[2];
}
#undef VS
#endif /* TM2_PORT_VISUAL */

#ifdef TM2_PORT_HUMANS
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- request 66 (ANALOG_STEERING.md 2.6): the turn's sign kept in reverse, per human ---- */
/* carThrottleSteer / carRicochetOffObject: 1 when `car` is a human record (human != 0) whose portHumanControl has
 * TM2_HUMAN_CTL_STEER_KEEP, else 0 (the AI cars, a record that is no human's, the bit clear: retail). Reads only. */
int PORT_TEXT portHumanSteerKeep(void *car, int human)
{
    int h;

    if (!(human & 0xFF))
        return 0;
    for (h = 0; h < TM2_MAX_HUMANS; h++)
        if ((void *)humanCars[h] == car)
            return portHumanControl[h] & TM2_HUMAN_CTL_STEER_KEEP;
    return 0;
}

/* ---- request 121 (ANALOG_STEERING.md 6.4): the analog throttle, per human ---- */
/* carThrottleSteer: the share of the speed caps the human `car` drives at, 1..4096 of 4096: portHumanThrottle[h] as
 * the port wrote it for this frame; 4096 (retail) for the AI cars, a record that is no human's, the turbo (always
 * full) and a value of 0 (not written: a digital press), below 0 or above 4096. Reads only, no rand(). */
int PORT_TEXT portHumanThrottleShare(void *car, int human)
{
    int h, a;

    if (!(human & 0xFF))
        return 4096;
    for (h = 0; h < TM2_MAX_HUMANS; h++)
        if ((void *)humanCars[h] == car) {
            a = portHumanThrottle[h];
            if (((struct TM2HumanRecord *)car)->input.turbo || a <= 0 || a > 4096)
                return 4096;
            return a;
        }
    return 4096;
}
#endif /* TM2_PORT_HUMANS */

#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- car select's extended carousel and name labels (PC port requests 36 and 42 item 5, car_numbers.h) ----
 * CARSEL.DMD's select node 9005 (the carousel) and its two name labels 9006 / 9007 (type 9, 14 children each,
 * child pointers at +16) are the models of the scene objects carousel, label_a and label_b (+4). menuCopyChoiceRun
 * copies the carousel's choice into both labels each frame, so a carousel with more children needs labels with as
 * many: retail's labels walk the word after their 14-pointer child table for index 14 (0x0000FF00 for 9006,
 * 0x2330FF00 for 9007: garbage pointers). The copies' extra children are the empty node (the walker draws nothing;
 * the port draws the names past 13 itself), or the carousel's model for Dark Tooth / a mod car. */
extern int carSelectStartCar;
extern char *carBioPictureFiles[];

/* One copy: node k of portCarSel from the scene object's model (the screen's select node), `count` children, the
 * extra ones the empty node; the object's model becomes the copy. 0 (the object left alone) unless the model is a
 * select with `from` children (CARSEL's 14, AICARSEL's 12). */
static int PORT_TEXT portCarSelCopy(int k, void *object, int from, int count)
{
    unsigned char **obj = (unsigned char **)object;
    unsigned char *src = obj[1], *dst = portCarSel.node[k];
    void **child = (void **)(dst + 16);
    int i;

    if (src == 0 || src[0] != 9 || src[6] != from)
        return 0;
    for (i = 0; i < 16 + from * 4; i++)
        dst[i] = src[i];
    for (i = from; i < count; i++)
        child[i] = (void *)portCarSel.blank;
    dst[6] = count;
    obj[1] = dst;
    return 1;
}

/* Each screen's start: no extended node, the empty node built (a hidden group), `retail` = the screen's own count. */
static void PORT_TEXT portCarSelReset(int retail)
{
    int i;

    portCarSel.count = 0;
    portCarSel.shown = -1;
    portCarSel.retail = retail;
    for (i = 0; i < TM2_PORT_CARSEL_LOADS; i++) {   /* request 71: the new copies hold no loaded model */
        portCarSelLoads[i].index = -1;
        portCarSelLoads[i].model = 0;
    }
    for (i = 0; i < (int)sizeof(portCarSel.blank); i++)
        portCarSel.blank[i] = 0;
    portCarSel.blank[0] = 1;            /* type 1: a group */
    portCarSel.blank[21] = 2;           /* hidden: the walker skips it */
}

/* screenCarSelect, after CARSEL is loaded (before the first draw binds the models): the children this car select
 * offers: 15 with TM2_PORT_DT_PLAYER and portCarOptions bit 8 (child 14 Dark Tooth: portDtPlayer.carsel_model or the
 * empty node), 0x20 + portCarSel.mods with TM2_PORT_CAR_NUMBERS and mods (children 15..0x1F and the mods' the empty
 * node until the port sets them, portCarSelSetChild), else CARSEL's 14 (nothing copied). The carousel and both name
 * labels get copies with that many children (request 42: the labels' fix). The step commands wrap at the copy's
 * count (+6); carIsSelectable skips the numbers that are no car. A start car left past the count by an earlier car
 * select goes back to 13. */
void PORT_TEXT portCarSelExtend(void *carousel, void *label_a, void *label_b)
{
    int count = TM2_PORT_CARSEL_RETAIL;

#ifdef TM2_PORT_DT_PLAYER
    portDtPlayer.carsel_count = 0;
    if (portCarOptions & TM2_PORT_CAR_OPT_DT_PLAYER)
        count = TM2_DT_PLAYER_INDEX + 1;
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    if (portCarSel.mods > 0)
        count = TM2_PORT_MOD_FIRST + (portCarSel.mods < TM2_PORT_MOD_CARS ? portCarSel.mods : TM2_PORT_MOD_CARS);
#endif
    portCarSelReset(TM2_PORT_CARSEL_RETAIL);
    if (count == TM2_PORT_CARSEL_RETAIL || !portCarSelCopy(0, carousel, TM2_PORT_CARSEL_RETAIL, count)) {
        if (carSelectStartCar >= TM2_PORT_CARSEL_RETAIL)
            carSelectStartCar = 13;
        return;
    }
    portCarSelCopy(1, label_a, TM2_PORT_CARSEL_RETAIL, count);
    portCarSelCopy(2, label_b, TM2_PORT_CARSEL_RETAIL, count);
#ifdef TM2_PORT_DT_PLAYER
    if ((portCarOptions & TM2_PORT_CAR_OPT_DT_PLAYER) && portDtPlayer.carsel_model != 0)
        ((void **)(portCarSel.node[0] + 16))[TM2_DT_PLAYER_INDEX] = portDtPlayer.carsel_model;
    portDtPlayer.carsel_count = count;
#endif
    portCarSel.count = count;
    if (carSelectStartCar >= count)
        carSelectStartCar = 13;
}

/* The carousel's children this car select: the extended count, else CARSEL's 14. */
int PORT_TEXT portCarSelCount(void)
{
    return portCarSel.count > 0 ? portCarSel.count : TM2_PORT_CARSEL_RETAIL;
}

/* The info picture of carousel index `index` (= its car number): retail's table for 0..13, the port's for Dark Tooth
 * and the mods (0 = none: the info button returns at once). */
const char *PORT_TEXT portCarBioFile(int index)
{
    if ((unsigned int)index < TM2_PORT_CARSEL_RETAIL)
        return carBioPictureFiles[index];
#ifdef TM2_PORT_DT_PLAYER
    if (index == TM2_DT_PLAYER_INDEX)
        return portDtPlayer.bio_picture;
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    if (index >= TM2_PORT_MOD_FIRST && index < TM2_PORT_MOD_FIRST + TM2_PORT_MOD_CARS
        && portModCars[index - TM2_PORT_MOD_FIRST].used)
        return portModCars[index - TM2_PORT_MOD_FIRST].bio_picture;
#endif
    return 0;
}

/* The port's API (between frames, during car select or the AI select): the extended carousel's child `index` (one of
 * the extra children: 14 and past in car select, 12 (Sweet Tooth) in the AI select) becomes `model`, a relocated and
 * textured model node (0 = the empty node). The draw walks only child[choice], so the port can swap the highlighted
 * car's model at any time (one in-game model loaded on demand, the others empty). Returns the old child, 0 when the
 * carousel is not extended or `index` is not one of its extra children. */
void *PORT_TEXT portCarSelSetChild(int index, void *model)
{
    void **child = (void **)(portCarSel.node[0] + 16);
    void *old;

    if (portCarSel.count <= 0 || index < portCarSel.retail || index >= portCarSel.count)
        return 0;
    old = child[index];
    child[index] = model != 0 ? model : (void *)portCarSel.blank;
    return old;
}

/* ---- the carousel's in-game models (PC port request 71, DARK_TOOTH_PLAYER.md 10.3 as code) ----
 * portCarSelLoad(k, index, dmd, dmd_len, tms, tms_len) installs one car's model files in load slot k
 * (portCarSelLoads[k]) and makes it the extended carousel's child `index` (14 Dark Tooth, 0x20 + m mod m in car
 * select, 12 Sweet Tooth in the AI select), slot k's previous model freed: the draw (sceneObjDraw) then shows it with
 * the carousel's position, spin, camera and lights, as a retail child. Nothing of a match is touched: no loader, no
 * registry (dbscanHierarchy, worldLoadObject), no VRAM bitmap, no load group. The TMS's images go to VRAM at the
 * slot's own spots: with TM2_PORT_TEXTURE_BANKS a bank of its own (TM2_PORT_CARSEL_BANK + k, the whole 1024 x 512),
 * else only slot 0, car select's free block (832, 256, 128 x 256; CLUTs at rows 40..63 of x 768..1023). The DMD is
 * relocated in the slot (modelRelocate with a private slot table, so modelRemapTextures points each primitive at its
 * image there and, banked, writes the bank into its tpage bits 9..15). Its car roots (ids 0x10..0xF0: one for every
 * retail car; Dark Tooth's truck, not his 2214) become the child, under a group node when there are several; a file
 * with none: its first root. Request 74: Dark Tooth's head (root 249) goes over his truck as in a match. */
#define CSL portCarSelLoads
extern int *vramTexSlots;           /* 5 words per slot: tpage, x, y, CLUT x, CLUT y; group * stride + image */
extern int vramTexSlotGroup;
extern int LoadImage(void *rect, void *data);
extern int DrawSync(int mode);
extern void modelRelocate(void *root, int delta);

/* The VRAM areas of a slot: pixels in [px0, px1) x [py0, py1), CLUTs in rows [cy0, cy1) of x [cx0, cx1). */
struct PortCarSelArea {
    short px0, py0, px1, py1;
    short cx0, cy0, cx1, cy1;
};

/* One image's pixel spot in the area: the first free (x, y) on a grid of the image's own size (as vramPlaceTexture
 * scans) whose rectangle stays inside one texture page as modelRemapTextures addresses it (x % 64 and y & 0xFF from
 * the page; a page row is 256 texels: 64 VRAM units at 4-bit, 128 at 8-bit, 256 at 15-bit) and meets no rectangle
 * taken. 1, or 0 when none is free. */
static int PORT_TEXT portCarSelPlace(struct TM2PortCarSelLoad *l, const struct PortCarSelArea *a, int mode, int w,
                                     int h, int *ox, int *oy)
{
    int lim = mode == 0 ? 64 : mode == 1 ? 128 : 256;
    int sx = w < 64 ? w : 64, sy = h < 256 ? h : 256;
    int x, y, i;

    if (w <= 0 || h <= 0 || w > lim || h > 256)
        return 0;
    for (y = a->py0; y + h <= a->py1; y += sy) {
        if ((y & 0xFF) + h > 256)
            continue;
        for (x = a->px0; x + w <= a->px1; x += sx) {
            if (x % 64 + w > lim)
                continue;
            for (i = 0; i < l->images; i++)
                if (x < l->rects[i][0] + l->rects[i][2] && l->rects[i][0] < x + w
                    && y < l->rects[i][1] + l->rects[i][3] && l->rects[i][1] < y + h)
                    break;
            if (i == l->images) {
                *ox = x;
                *oy = y;
                return 1;
            }
        }
    }
    return 0;
}

/* One upload: LoadImage of w x h VRAM units at (x, y) of `bank` (the port's word around it, TEXTURE_BANKS.md 6.5),
 * waited for. */
static void PORT_TEXT portCarSelUpload(int bank, int x, int y, int w, int h, const void *data)
{
    short rect[4];

    rect[0] = x;
    rect[1] = y;
    rect[2] = w;
    rect[3] = h;
#ifdef TM2_PORT_TEXTURE_BANKS
    if (bank != 0) {
        portTexLoadImage(bank, rect, (void *)data);
        return;
    }
#endif
    LoadImage(rect, (void *)data);
    DrawSync(0);
}

/* The TMS's images into slot l's VRAM: each pixel block at a free spot of the area, its CLUT's first row (16 colours
 * at 4-bit, 256 at 8-bit: retail uploads no more, FILE_FORMATS.md 3) at the CLUT cursor, and the slot table entry
 * {page bits | bank << 9, x, y, CLUT x, CLUT y} for modelRemapTextures. 1, or TM2_CARSEL_LOAD_TMS / _VRAM. */
static int PORT_TEXT portCarSelTextures(struct TM2PortCarSelLoad *l, const struct PortCarSelArea *a,
                                        const unsigned char *tms, int len)
{
    const unsigned char *e, *tim, *clut, *pix;
    int n, i, size, flags, mode, w, h, x, y, cw, cx, cy;

    if (len < 16 || *(const unsigned int *)tms != 0x50535854 || *(const unsigned int *)(tms + 4) != 0x43)
        return TM2_CARSEL_LOAD_TMS;
    n = *(const int *)(tms + 12);
    if (n < 0 || n > TM2_PORT_CARSEL_IMAGES || 16 + 4 * n > len)
        return TM2_CARSEL_LOAD_TMS;
    e = tms + 16 + 4 * n;
    cx = a->cx0;
    cy = a->cy0;
    for (i = 0; i < n; i++) {
        if (e + 4 > tms + len)
            return TM2_CARSEL_LOAD_TMS;
        size = *(const int *)e;
        tim = e + 4;
        if (size < 20 || (size & 3) || tim + size > tms + len || *(const unsigned int *)tim != 0x10)
            return TM2_CARSEL_LOAD_TMS;
        flags = *(const int *)(tim + 4);
        mode = flags & 7;
        if (mode > 2 || ((flags & 8) != 0) != (mode < 2))
            return TM2_CARSEL_LOAD_TMS;
        clut = 0;
        pix = tim + 8;
        if (flags & 8) {
            clut = pix;
            pix += *(const int *)clut;
        }
        if (pix + 12 > tim + size)
            return TM2_CARSEL_LOAD_TMS;
        w = *(const unsigned short *)(pix + 8);
        h = *(const unsigned short *)(pix + 10);
        if (12 + 2 * w * h > *(const int *)pix || pix + *(const int *)pix > tim + size)
            return TM2_CARSEL_LOAD_TMS;
        if (!portCarSelPlace(l, a, mode, w, h, &x, &y))
            return TM2_CARSEL_LOAD_VRAM;
        portCarSelUpload(l->bank, x, y, w, h, pix + 12);
        l->rects[i][0] = x;
        l->rects[i][1] = y;
        l->rects[i][2] = w;
        l->rects[i][3] = h;
        l->images = i + 1;
        l->slots[i][0] = (x >> 6) | ((y >> 8) << 4) | (l->bank << 9);
        l->slots[i][1] = x;
        l->slots[i][2] = y;
        l->slots[i][3] = 0;
        l->slots[i][4] = 0;
        if (clut != 0) {
            cw = mode == 0 ? 16 : 256;
            if (*(const int *)clut < 12 + 2 * cw)
                return TM2_CARSEL_LOAD_TMS;
            if (cx + cw > a->cx1) {
                cx = a->cx0;
                cy++;
            }
            if (cy >= a->cy1)
                return TM2_CARSEL_LOAD_VRAM;
            portCarSelUpload(l->bank, cx, cy, cw, 1, clut + 12);
            l->slots[i][3] = cx;
            l->slots[i][4] = cy;
            cx += cw;
        }
        e = tim + size;
    }
    return 1;
}

/* Slot k's model off the carousel: its child back to the empty node unless another slot's model was set there since. */
void PORT_TEXT portCarSelUnload(int k)
{
    struct TM2PortCarSelLoad *l;
    void **child;

    if ((unsigned int)k >= TM2_PORT_CARSEL_LOADS)
        return;
    l = &CSL[k];
    if (l->index >= 0 && l->model != 0 && portCarSel.count > 0 && l->index < portCarSel.count) {
        child = (void **)(portCarSel.node[0] + 16);
        if (child[l->index] == l->model)
            child[l->index] = (void *)portCarSel.blank;
    }
    l->index = -1;
    l->model = 0;
}

/* The port's API (request 71): between frames (or from the portCarSelDraw wrap), while car select or the AI select
 * runs (after portCarSelExtend / portAiSelExtend). `dmd` / `tms` are the car's PSXD and PSXT files in guest RAM (a
 * mod's carselect.dmd / .tms or car.dmd / .tms, Dark Tooth's BB.DMD / BB.TMS, a retail car's); `dmd` may be the
 * slot's own buffer (portCarSelLoads[k].dmd, nothing copied), the TMS is read in place. Returns 1, or a
 * TM2_CARSEL_LOAD_* error (slot k is then empty and the child the empty node).
 * Request 130: `in_place` (portCarSelLoadInPlace) relocates and draws the DMD where it is, in the port's buffer, of
 * any size (the slot's 0x20000 bytes then hold only request 74's head node); else a DMD larger than the slot is
 * TM2_CARSEL_LOAD_LARGE. */
static int PORT_TEXT portCarSelInstall(int k, int index, const void *dmd, int dmd_len, const void *tms, int tms_len,
                                       int in_place)
{
    struct PortCarSelArea area, *a = &area;
    struct TM2PortCarSelLoad *l;
    unsigned char *buf, *node, *model, *head;
    unsigned char *roots[TM2_PORT_CARSEL_ROOTS];
    unsigned int *p;
    int *saved_slots;
    int saved_group, delta, count, i, id, r, phase2;
#ifdef TM2_PORT_TEXTURE_BANKS
    int saved_live;
#endif

    phase2 = (index & TM2_CARSEL_DT_PHASE2) != 0;     /* request 118: Dark Tooth's second phase, the head alone */
    index &= ~TM2_CARSEL_DT_PHASE2;
    if ((unsigned int)k >= TM2_PORT_CARSEL_LOADS)
        return TM2_CARSEL_LOAD_SLOT;
    l = &CSL[k];
#ifdef TM2_PORT_TEXTURE_BANKS
    l->bank = TM2_PORT_CARSEL_BANK + k;
    area.px0 = 0;                       /* the slot's own bank: pixels above row 480, CLUTs below */
    area.py0 = 0;
    area.px1 = 1024;
    area.py1 = 480;
    area.cx0 = 0;
    area.cy0 = 480;
    area.cx1 = 1024;
    area.cy1 = 512;
#else
    if (k != 0)
        return TM2_CARSEL_LOAD_SLOT;
    l->bank = 0;
    area.px0 = 832;                     /* car select's free block (DARK_TOOTH_PLAYER.md 10.3) */
    area.py0 = 256;
    area.px1 = 960;
    area.py1 = 512;
    area.cx0 = 768;
    area.cy0 = 40;
    area.cx1 = 1024;
    area.cy1 = 64;
#endif
    portCarSelUnload(k);
    l->images = 0;
    l->roots = 0;
    if (portCarSel.count <= 0 || index < portCarSel.retail || index >= portCarSel.count)
        return TM2_CARSEL_LOAD_INDEX;
    if (l->bank == 0 && portCarSel.retail != TM2_PORT_CARSEL_RETAIL)
        return TM2_CARSEL_LOAD_VRAM;    /* the AI select's free VRAM is not mapped: banks only */
    buf = in_place ? (unsigned char *)dmd : l->dmd;
    if (dmd_len < 0x20 || ((unsigned int)dmd & 3) || ((const unsigned int *)dmd)[0] != 0x50535844
        || ((const unsigned int *)dmd)[1] != 0x43)
        return TM2_CARSEL_LOAD_DMD;
    if (buf == l->dmd && dmd_len > (int)sizeof(l->dmd))
        return TM2_CARSEL_LOAD_LARGE;
    DrawSync(0);                        /* the frame before may still texture from the slot */
    if ((const unsigned char *)dmd != buf)
        for (i = 0; i < dmd_len; i++)
            buf[i] = ((const unsigned char *)dmd)[i];
    r = portCarSelTextures(l, a, (const unsigned char *)tms, tms_len);
    if (r < 0)
        return r;

    /* relocate every root (assetInstallImage's walk without the registration), the remap through l->slots */
    delta = (int)((unsigned int)buf + 0x7FFE63C0u);
    count = ((unsigned int *)buf)[6];
    if (count <= 0 || 0x1C + 4 * count > dmd_len)
        return TM2_CARSEL_LOAD_DMD;
    saved_slots = vramTexSlots;
    saved_group = vramTexSlotGroup;
    vramTexSlots = &l->slots[0][0];
    vramTexSlotGroup = 0;
#ifdef TM2_PORT_TEXTURE_BANKS
    saved_live = portTexBanks.live;
    portTexBanks.live = l->bank != 0;   /* the remap writes the bank bits only while banks are live */
#endif
    p = (unsigned int *)(buf + 0x1C);
    head = 0;
    for (i = 0; i < count; i++) {
        node = (unsigned char *)(p[i] + delta);
        if (node < buf || node >= buf + dmd_len)
            break;
        modelRelocate(node, delta);
        id = *(unsigned short *)(node + 2);
        if (id >= 0x10 && id <= 0xF0 && (id & 0xF) == 0 && l->roots < TM2_PORT_CARSEL_ROOTS)
            roots[l->roots++] = node;
        else if (id == TM2_PORT_CARSEL_HEAD_ID)
            head = node;
    }
    /* request 74: Dark Tooth's head (root 249, placed only by the boss update in a match) on his truck as in a match,
       after the file in the slot's buffer; with the truck under the group below it turns with the carousel's spin and
       lights. Request 109: a transform node (opcode 5, 40 + 4 bytes), TM2_PORT_CARSEL_HEAD_Z above the truck in its
       frame (the boss's hover, without the sway; node offsets count as the vertices do: the file's wheel nodes sit at
       +-72) and turned a half turn about z: the head's face is its -y (the boss's swoop turns it to
       ratan2(head - target)), a truck's front its +y. Request 74's offset node had no turn: the face looked back and
       the head's bulk (its sphere 133 along its +y) hung over the truck's front. */
    if (head != 0 && l->roots == 1 && *(unsigned short *)(roots[0] + 2) == 0xF0
        && (buf != l->dmd || ((dmd_len + 3) & ~3) + 44 <= (int)sizeof(l->dmd))) {
        node = buf != l->dmd ? l->dmd : buf + ((dmd_len + 3) & ~3);   /* request 130: in place, the slot's start */
        for (i = 0; i < 44; i++)
            node[i] = 0;
        node[0] = 5;
        ((short *)(node + 4))[0] = -4096;   /* rows: (-1, 0, 0), (0, -1, 0), (0, 0, 1) */
        ((short *)(node + 4))[4] = -4096;
        ((short *)(node + 4))[8] = 4096;
        ((int *)(node + 24))[2] = TM2_PORT_CARSEL_HEAD_Z;
        node[38] = 1;
        ((unsigned char **)(node + 40))[0] = head;
        if (phase2) {
            /* request 118: the second phase's look, the truck not drawn, the head where it sits in a match (stage
               2's ride height 50 less the boss's -24: its origin 26 over the ground, the truck's origin 116 over
               it) */
            ((int *)(node + 24))[2] = TM2_PORT_CARSEL_PHASE2_Z;
            roots[0] = node;
        } else
            roots[l->roots++] = node;
    }
#ifdef TM2_PORT_TEXTURE_BANKS
    portTexBanks.live = saved_live;
#endif
    vramTexSlots = saved_slots;
    vramTexSlotGroup = saved_group;
    if (l->roots == 0) {
        node = (unsigned char *)(p[0] + delta);
        if (node < buf || node >= buf + dmd_len)
            return TM2_CARSEL_LOAD_DMD;
        roots[l->roots++] = node;
    }

    if (l->roots == 1) {
        model = roots[0];
    } else {
        model = l->group;               /* a group (type 1): the first root's sphere, not culled, the roots */
        for (i = 0; i < (int)sizeof(l->group); i++)
            model[i] = 0;
        model[0] = 1;
        for (i = 4; i < 20; i++)
            model[i] = roots[0][i];
        model[20] = l->roots;
        model[21] = 4;
        for (i = 0; i < l->roots; i++)
            ((unsigned char **)(model + 24))[i] = roots[i];
    }
    portCarSelSetChild(index, model);
    l->index = index;
    l->model = model;
    return 1;
}

int PORT_TEXT portCarSelLoad(int k, int index, const void *dmd, int dmd_len, const void *tms, int tms_len)
{
    return portCarSelInstall(k, index, dmd, dmd_len, tms, tms_len, 0);
}

/* Request 130: as portCarSelLoad, the DMD relocated and drawn in the port's own buffer (no copy, no size limit):
 * `dmd` 4-aligned in guest RAM, holding the file as read; the port keeps it, unchanged, until slot k is loaded again
 * or unloaded (portCarSelUnload), and rewrites it only after that (the C waits for the GPU, DrawSync, at each load). */
int PORT_TEXT portCarSelLoadInPlace(int k, int index, void *dmd, int dmd_len, const void *tms, int tms_len)
{
    return portCarSelInstall(k, index, dmd, dmd_len, tms, tms_len, 1);
}
#undef CSL

/* screenCarSelect: the car number the player picked at carousel index `index`. The carousel's index is the number
 * (0..13 retail, 14 Dark Tooth, 0x20 + m mod m), so this is the identity for every car and -1 for an index that is no
 * car (never accepted by carIsSelectable, so never picked). */
int PORT_TEXT portCarSelNumber(int index)
{
    if ((unsigned int)index < TM2_PORT_CARSEL_RETAIL)
        return index;
#ifdef TM2_PORT_DT_PLAYER
    if (index == TM2_DT_PLAYER_INDEX && (portCarOptions & TM2_PORT_CAR_OPT_DT_PLAYER))
        return index;
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    if (index >= TM2_PORT_MOD_FIRST && index < portCarSel.count && portCarNumberBase(index) >= 0)
        return index;
#endif
    return -1;
}

#ifdef TM2_PORT_CAR_NUMBERS
/* ---- Challenge's opponent picker with Sweet Tooth (request 42 item 2, the user's decision 2026-10-16) ----
 * screenAiSelect's carousel (AICARSEL.DMD's select node 9005) and its name labels (9006 / 9007) hold the 12 regular
 * cars, so retail never offers Sweet Tooth (car 12) there although every computer path supports him. With
 * portCarOptions bit 11 and Sweet Tooth unlocked (unlockSweetTooth) all three get 13-child copies: child 12 is the
 * empty node (the labels: the port draws his name; the carousel: until the port puts a model there with
 * portCarSelSetChild). CARSEL's Sweet Tooth cannot be borrowed: AICARSEL.DMD replaces CARSEL.DMD at 0x80019C40, and
 * AICARSEL.TMS puts another car's image on CARSEL's Sweet Tooth body page (768,128) and its 4-bit CLUTs on his CLUT
 * row (768,35) (DARK_TOOTH_PLAYER.md 10.3's recipe gives the port its own copy). Locked, or the bit clear: AICARSEL's own
 * nodes, retail exactly (a start car left at 12 goes back to 11). carIsSelectable accepts 12 only while he is
 * unlocked; the index is the opponent's car number. */
extern unsigned char unlockSweetTooth;
extern int aiSelectStartCar;

void PORT_TEXT portAiSelExtend(void *carousel, void *label_a, void *label_b)
{
    int count = TM2_PORT_AISEL_RETAIL + 1;

    portCarSelReset(TM2_PORT_AISEL_RETAIL);
    if (!(portCarOptions & TM2_PORT_CAR_OPT_AI_SWEET_TOOTH) || !unlockSweetTooth
        || !portCarSelCopy(0, carousel, TM2_PORT_AISEL_RETAIL, count)) {
        if (aiSelectStartCar >= TM2_PORT_AISEL_RETAIL)
            aiSelectStartCar = TM2_PORT_AISEL_RETAIL - 1;
        return;
    }
    portCarSelCopy(1, label_a, TM2_PORT_AISEL_RETAIL, count);
    portCarSelCopy(2, label_b, TM2_PORT_AISEL_RETAIL, count);
    portCarSel.count = count;
    if (aiSelectStartCar >= count)
        aiSelectStartCar = TM2_PORT_AISEL_RETAIL - 1;
}

/* The AI select carousel's children (the bio wrap): 13 when portAiSelExtend extended it, else AICARSEL's 12. */
int PORT_TEXT portAiSelCount(void)
{
    return portCarSel.count > 0 ? portCarSel.count : TM2_PORT_AISEL_RETAIL;
}
#endif

/* screenEnding, for a human whose car number is past 13 (endingMovieTable has 14 entries; retail read past them): Dark
 * Tooth's movie is the port's (portDtPlayer.ending_movie, TM2_PORT_DT_PLAYER), a mod's the port's too
 * (portModCars[m].ending_movie, TM2_PORT_CAR_NUMBERS); none when 0 or for any other number. YOUWON.STR follows in the
 * caller, as for every car but Grasshopper. */
extern int moviePlayEnding(void *settings, char *name, int frames, int wide, void *skip);
extern unsigned char movieSkipCheckDelayed(int value);
extern struct TM2Settings sessionSettings;

void PORT_TEXT portCarEnding(int number)
{
    const char *name = 0;
    int frames = 0, wide = 0;

#ifdef TM2_PORT_DT_PLAYER
    if (number == TM2_DT_PLAYER_INDEX) {
        name = portDtPlayer.ending_movie;
        frames = portDtPlayer.ending_frames;
        wide = portDtPlayer.ending_wide;
    }
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    if (number >= TM2_PORT_MOD_FIRST && number < TM2_PORT_MOD_FIRST + TM2_PORT_MOD_CARS
        && portModCars[number - TM2_PORT_MOD_FIRST].used) {
        name = portModCars[number - TM2_PORT_MOD_FIRST].ending_movie;
        frames = portModCars[number - TM2_PORT_MOD_FIRST].ending_frames;
        wide = portModCars[number - TM2_PORT_MOD_FIRST].ending_wide;
    }
#endif
    if (name != 0)
        moviePlayEnding(&sessionSettings, (char *)name, frames, wide, (void *)movieSkipCheckDelayed);
}
#endif /* TM2_PORT_DT_PLAYER || TM2_PORT_CAR_NUMBERS */

#ifdef TM2_PORT_CAR_NUMBERS
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
#include "semantic/settings.h"
/* ---- car numbers past retail's (PC port request 42 item 3, car_numbers.h, PER_CAR_PLAN.md E.5) ----
 * sessionSettings' numbers are portSettingsHumanCar / portSettingsOpponentCar (16 bits); its bytes keep each car's
 * base (a mod's retail car). A getter returns the table's number only when it is a mod whose base is the byte, else
 * the byte: so with no mod number set every getter returns retail's byte value, and a byte written around the setters
 * (another record's copy, the port's own byte write) wins over a stale number. */
extern int carIndexToType(struct TM2Settings *settings, int index);
extern char *carDisplayName(struct TM2Settings *settings, unsigned int id);
extern char *carFileCode(struct TM2Settings *settings, unsigned int id);

static struct TM2PortModCar *PORT_TEXT portCarNumberMod(int number)
{
    struct TM2PortModCar *m;

    if (number < TM2_PORT_MOD_FIRST || number >= TM2_PORT_MOD_FIRST + TM2_PORT_MOD_CARS)
        return 0;
    m = &portModCars[number - TM2_PORT_MOD_FIRST];
    return m->used && m->base <= 14 ? m : 0;
}

/* The settings byte of a number: the number itself below 0x20 (-1 for none, as retail stores it), a used mod's base
 * (0..14), else -1. */
int PORT_TEXT portCarNumberBase(int number)
{
    struct TM2PortModCar *m;

    if (number < TM2_PORT_MOD_FIRST)
        return number < 0 ? -1 : number;
    m = portCarNumberMod(number);
    return m != 0 ? m->base : -1;
}

/* The getters: kind 0 human i, 1 opponent i of record `s` (TM2_SETTINGS_HUMAN_CAR / _OPPONENT_CAR's byte, retail's
 * signed value), or the mod number sessionSettings' table holds for it. */
int PORT_TEXT portCarNumberGet(struct TM2Settings *s, int kind, int i)
{
    int byte, n, base;

    byte = kind ? TM2_SETTINGS_OPPONENT_CAR(s, i) : TM2_SETTINGS_HUMAN_CAR(s, i);
    if (s != &sessionSettings || i < 0 || i >= (kind ? TM2_PORT_NUMBER_OPPONENTS : TM2_PORT_NUMBER_HUMANS))
        return byte;
    n = kind ? portSettingsOpponentCar[i] : portSettingsHumanCar[i];
    if (n < TM2_PORT_MOD_FIRST)
        return byte;
    base = portCarNumberBase(n);
    return base >= 0 && base == byte ? n : byte;
}

/* The setters (settingsSetHumanCar, settingsSetOpponentCar, sessionSetOpponentCar): the byte gets the number's base
 * (retail's store for every number below 0x20), sessionSettings' table the number. */
void PORT_TEXT portCarNumberSet(struct TM2Settings *s, int kind, int i, int number)
{
    int byte = number < TM2_PORT_MOD_FIRST ? number : portCarNumberBase(number);

    if (kind)
        TM2_SETTINGS_OPPONENT_CAR(s, i) = byte;
    else
        TM2_SETTINGS_HUMAN_CAR(s, i) = byte;
    if (s != &sessionSettings || i < 0)
        return;
    if (kind && i < TM2_PORT_NUMBER_OPPONENTS)
        portSettingsOpponentCar[i] = number;
    else if (!kind && i < TM2_PORT_NUMBER_HUMANS)
        portSettingsHumanCar[i] = number;
}

/* The clears (settingsClearHumanCars: kinds 3, its 13 bytes reach the opponents; settingsClearOpponentCars: 2):
 * sessionSettings' numbers -1 (the bytes are the caller's). */
void PORT_TEXT portCarNumbersClear(struct TM2Settings *s, int kinds)
{
    int i;

    if (s != &sessionSettings)
        return;
    for (i = 0; i < TM2_PORT_NUMBER_HUMANS; i++)
        if (kinds & 1)
            portSettingsHumanCar[i] = -1;
    for (i = 0; i < TM2_PORT_NUMBER_OPPONENTS; i++)
        if (kinds & 2)
            portSettingsOpponentCar[i] = -1;
}

/* carIsSelectable: 1 for a number that is a car (0..13; 14 with TM2_PORT_DT_PLAYER, whose own test then asks for bit
 * 8; a used mod), 0 for 14 without it, 15..0x1F, an unused mod and anything past (the carousel's empty children). */
int PORT_TEXT portCarNumberPickable(int number)
{
    if ((unsigned int)number < TM2_PORT_CARSEL_RETAIL)
        return 1;
#ifdef TM2_PORT_DT_PLAYER
    if (number == TM2_DT_PLAYER_INDEX)
        return 1;
#endif
    return number >= TM2_PORT_MOD_FIRST && portCarNumberMod(number) != 0;
}

/* carIndexToType past retail's switch: a used mod's base's type (its DMD uses the base's ids, PER_CAR_PLAN.md E.1),
 * else -1. */
int PORT_TEXT portCarNumberType(int number)
{
    struct TM2PortModCar *m = portCarNumberMod(number);

    return m != 0 ? carIndexToType(&sessionSettings, m->base) : -1;
}

/* carDisplayName / carFileCode past retail's switch: a used mod's name / file code (0: its base's), else `unknown`
 * (retail's "unknown"). */
const char *PORT_TEXT portCarNumberName(int number, const char *unknown)
{
    struct TM2PortModCar *m = portCarNumberMod(number);

    if (m == 0)
        return unknown;
    return m->name != 0 ? m->name : carDisplayName(&sessionSettings, m->base);
}

const char *PORT_TEXT portCarNumberCode(int number, const char *unknown)
{
    struct TM2PortModCar *m = portCarNumberMod(number);

    if (m == 0)
        return unknown;
    return m->code != 0 ? m->code : carFileCode(&sessionSettings, m->base);
}

/* The port's API: the car number load group `group` was last loaded with (portCarLoadGroup, TM2_PORT_CARS), -1 if
 * none. With the group of a car (portCarGroups) this names a mod in a match: keys and types stay the base's. */
int PORT_TEXT portCarNumberOfGroup(int group)
{
    if ((unsigned int)group >= TM2_PORT_NUMBER_GROUPS)
        return -1;
    return portCarNumbers.group[group] - 1;
}

/* shellAttractDemo: 0 = keep sessionSettings' numbers before the demo match (its random cars overwrite them), 1 =
 * put them back after (settingsCopy restores only the record's bytes). */
void PORT_TEXT portCarNumbersKeep(int restore)
{
    int i;

    for (i = 0; i < TM2_PORT_NUMBER_HUMANS; i++) {
        if (restore)
            portSettingsHumanCar[i] = portCarNumbers.saved_human[i];
        else
            portCarNumbers.saved_human[i] = portSettingsHumanCar[i];
    }
    for (i = 0; i < TM2_PORT_NUMBER_OPPONENTS; i++) {
        if (restore)
            portSettingsOpponentCar[i] = portCarNumbers.saved_opponent[i];
        else
            portCarNumbers.saved_opponent[i] = portSettingsOpponentCar[i];
    }
}

/* sceneObjDraw, before each draw of the extended carousel (portCarSel.node[0]): note the index shown. The port wraps
 * this to load the highlighted mod car's in-game model on demand (portCarSelLoad, request 71) and the C draws whatever
 * child the index has (the empty node until then). The step commands change the index while the carousel is off
 * screen (during the slide-out), so a load here is not seen. */
void PORT_TEXT portCarSelDraw(void *node)
{
    portCarSel.shown = ((unsigned char *)node)[7];
}
#endif /* TM2_PORT_CAR_NUMBERS */

#ifdef TM2_PORT_CAR_PROFILE
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- a mod car's own numbers (PC port request 70, docs/CAR_PROFILES.md, car_numbers.h) ----
 * The car a record drives is its slot's (record +0x004 human / +0x006 AI): the slot's load group (portCarGroups) was
 * loaded with a car number (portCarNumberOfGroup), and a used mod's number has portCarProfile[number - 0x20]. Every
 * hook below leaves retail's result alone when that is no valid profile. None calls rand() or keeps state. */
extern int carDefaultTopSpeed(int type);
extern char *carTypeName(int type);

/* The stats fields a profile may set: the ones the per-car init functions of carStatsLoad write (CAR_STATS.md 2) and
 * its three defaults (pitch travel, steering lock, handbrake lock), by offset and width. Not the per-life fields
 * (+0x00 human flag, +0x14 frame rate, +0x3C shield, +0xA4 HP, +0xA8 damageable and the timers carStatsLoad clears)
 * nor the derived ones (+0x4A wheelbase, +0x54 / +0x5A / +0x60.. speeds, +0x8C retaliation, +0x84): carStatsLoad
 * computes those after the overrides from the fields set. */
static int PORT_TEXT portCarProfileWidth(int offset)
{
    unsigned int m;
    int h = offset >> 1;

    if (offset == 0x02 || offset == 0x0E)
        return 1;
    if (offset == 0xAC)
        return 4;
    if (offset < 0 || offset >= 0xE8 || (offset & 1))
        return 0;
    /* the 2-byte fields as bits of halfword index offset / 2 (no table: nothing in .rodata) */
    m = h < 32 ? 0xA4240000U : h < 64 ? 0xE0F0CBDEU : h < 96 ? 0x000001BBU : 0x00077000U;
    return (m >> (h & 31)) & 1 ? 2 : 0;
}

/* The port's API too: the valid profile of the car in car slot `slot` (a used mod with valid set), 0 otherwise. */
struct TM2PortCarProfile *PORT_TEXT portCarProfileOfSlot(int slot)
{
    struct TM2PortCarProfile *p;
    int number;

    if ((unsigned int)slot >= TM2_CAR_SLOT_CAP)
        return 0;
    number = portCarNumberOfGroup(portCarGroups.slot_group[slot]);
    if (number < TM2_PORT_MOD_FIRST || portCarNumberBase(number) < 0)
        return 0;
    p = &portCarProfile[number - TM2_PORT_MOD_FIRST];
    return p->valid ? p : 0;
}

/* The slot of the record whose stats block is `stats` or whose wheel block is `wheels`, -1 if none. */
static int PORT_TEXT portCarProfileSlotOf(void *stats, void *wheels)
{
    struct TM2HumanRecord *h;
    struct TM2AIRecord *a;
    int i;

    for (i = 0; i < TM2_MAX_HUMANS; i++) {
        h = (struct TM2HumanRecord *)humanCars[i];
        if ((void *)&h->stats == stats || (void *)&h->wheels == wheels)
            return h->slot;
    }
    for (i = 0; i < TM2_MAX_AI_CARS; i++) {
        a = (struct TM2AIRecord *)aiCars[i];
        if ((void *)&a->stats == stats || (void *)&a->wheels == wheels)
            return a->slot;
    }
    return -1;
}

/* carStatsLoad, after the base type's init function and before the derived fields: the profile's overrides in order
 * (an entry that is no listed field or has another width is skipped). The derived fields then follow from them: the
 * wheelbase, the speeds of the top speed (carStatsSetTopSpeed), the retaliation budget; humanCarPlaceAtStart still
 * adds 20 to a human's max HP (so a profile's +0x3A is the table's value, as CAR_STATS.md lists). */
void PORT_TEXT portCarProfileStats(void *stats)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(portCarProfileSlotOf(stats, 0));
    unsigned char *s = (unsigned char *)stats;
    int i, o, w, v;

    if (p == 0)
        return;
    for (i = 0; i < p->stat_count && i < TM2_PORT_PROFILE_STATS; i++) {
        o = p->stat[i].offset;
        w = p->stat[i].width;
        v = p->stat[i].value;
        if (w == 0 || portCarProfileWidth(o) != w)
            continue;
        if (w == 1)
            s[o] = v;
        else if (w == 2)
            *(short *)(s + o) = v;
        else
            *(int *)(s + o) = v;
    }
}

/* carWheelsSetStepHeight, after the base type's step: the profile's (flag TM2_PORT_PROFILE_STEP, a value > 0). */
void PORT_TEXT portCarProfileStep(struct TM2CarWheels *wheels)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(portCarProfileSlotOf(0, wheels));

    if (p != 0 && (p->flags & TM2_PORT_PROFILE_STEP) && p->step > 0)
        wheels->step = p->step;
}

/* viewCameraOffsetsInit, after the base type's offsets: the profile's for the bits of cam_mask. */
void PORT_TEXT portCarProfileCamera(struct TM2HumanRecord *human)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(human->slot);
    int *dst[6];
    int v, i;

    if (p == 0)
        return;
    dst[0] = human->cam_look_offset;
    dst[1] = human->cam_chase;
    dst[2] = human->cam_offset3;
    dst[3] = human->cam_offset;
    dst[4] = human->cam_split1;
    dst[5] = human->cam_split2;
    for (v = 0; v < 6; v++)
        if (p->cam_mask & (1 << v))
            for (i = 0; i < 3; i++)
                dst[v][i] = p->cam[v][i];
}

/* weaponBlockInit, after the base type's numbers (and a player's Dark Tooth's): the profile's recharge period,
 * capacity and max specials (flag TM2_PORT_PROFILE_SPECIAL; each > 0, else the base's). A borrowed special
 * (TM2_PORT_SPECIAL_AS) still sets its own after this. */
void PORT_TEXT portCarProfileWeapons(void *car, struct TM2CarWeapons *rec, int human)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(human ? ((struct TM2HumanRecord *)car)->slot
                                                             : ((struct TM2AIRecord *)car)->slot);

    if (p == 0 || !(p->flags & TM2_PORT_PROFILE_SPECIAL))
        return;
    if (p->recharge > 0)
        rec->recharge_period = p->recharge;
    if (p->capacity > 0)
        rec->capacity = p->capacity;
    if (p->max_specials > 0)
        rec->max_specials = p->max_specials;
}

/* carDefaultTopSpeed for the car in `slot` (TM2_CAR_DEFAULT_TOP_SPEED: the AI's cruise speeds, the speed given back
 * after a freeze or the shield cheat): the profile's top speed (its +0x52 entry, the last one), else the type's. */
int PORT_TEXT portCarProfileTopSpeed(int slot, int type)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(slot);
    int i, v = carDefaultTopSpeed(type);

    if (p != 0)
        for (i = 0; i < p->stat_count && i < TM2_PORT_PROFILE_STATS; i++)
            if (p->stat[i].offset == 0x52 && p->stat[i].width == 2)
                v = p->stat[i].value;
    return v;
}

/* hudRadarPlotCar's type argument for the car in `slot` of type `type`: the profile's colour (flag
 * TM2_PORT_PROFILE_RADAR) as TM2_PORT_PROFILE_RADAR_KEY | 0xRRGGBB, else `type` (the type's colour). */
int PORT_TEXT portCarProfileRadar(int slot, int type)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(slot);

    if (p == 0 || !(p->flags & TM2_PORT_PROFILE_RADAR))
        return type;
    return TM2_PORT_PROFILE_RADAR_KEY | p->radar[0] << 16 | p->radar[1] << 8 | p->radar[2];
}

/* hudDrawTargetPanel: the target's name, the profile's when it has one, else carTypeName's. */
const char *PORT_TEXT portCarProfileName(int slot, int type)
{
    struct TM2PortCarProfile *p = portCarProfileOfSlot(slot);

    if (p != 0 && p->name != 0)
        return p->name;
    return carTypeName(type);
}
#endif /* TM2_PORT_CAR_PROFILE */

#ifdef TM2_PORT_HIDDEN_AREAS
#ifndef TM2_PORT_VISUAL
#error "TM2_PORT_HIDDEN_AREAS needs TM2_PORT_VISUAL (portVisual bit 6)"
#endif
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- request 72 (HIDDEN_AREAS.md 11): portVisual bit 6, the hidden areas drawn from anywhere ---- */

#define HIDDEN_W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))
extern int *viewGetEyeTrans(int view);

/* One word of the level model for the walk: its value kept, `value` written. */
static void PORT_TEXT portHiddenPatch(unsigned int *word, unsigned int value)
{
    struct TM2PortHiddenAreas *h = &portHiddenAreas;

    if (h->count >= TM2_VIS_HIDDEN_PATCHES)
        return;
    h->word[h->count] = word;
    h->saved[h->count] = *word;
    *word = value;
    h->count++;
}

/* One mesh of the roof (request 75): the nave's header with `n` of its primitives from `prim`, each checked to be a
 * quad whose first vertex lies at the ceiling's height. Returns 1 when built. */
static int PORT_TEXT portHiddenRoofMesh(unsigned char *mesh, unsigned char *nave, unsigned char *prim, int n)
{
    unsigned char *p = prim;
    unsigned char *verts = (unsigned char *)HIDDEN_W(nave, 4);
    int i;

    for (i = 0; i < n; i++) {
        if (((HIDDEN_W(p, 12) >> 16) & 7) != 4 ||
            *(short *)(verts + (short)HIDDEN_W(p, 4) * 8 + 4) != TM2_VIS_HIDDEN_CEILING_Z)
            return 0;
        p += p[2] * 4;
    }
    for (i = 0; i < 44; i++)
        mesh[i] = nave[i];
    HIDDEN_W(mesh, 12) = (unsigned int)prim;
    HIDDEN_W(mesh, 16) = n;
    return 1;
}

/* Request 78 (HIDDEN_AREAS.md 12.6): every vertex of the nave's primitives at z `from` is set to z `to`. With
 * `check` nothing is written and the result is 1 when some vertex already lies at z `to` (then lowering could not be
 * undone exactly). */
static int PORT_TEXT portHiddenRoofMove(unsigned char *nave, int from, int to, int check)
{
    unsigned char *p = (unsigned char *)HIDDEN_W(nave, 12);
    unsigned char *verts = (unsigned char *)HIDDEN_W(nave, 4);
    unsigned int idx[2];
    short *z;
    int i, k;

    for (i = 0; i < (int)HIDDEN_W(nave, 16); i++) {
        idx[0] = HIDDEN_W(p, 4);
        idx[1] = HIDDEN_W(p, 8);
        for (k = 0; k < (((HIDDEN_W(p, 12) >> 16) & 7) == 4 ? 4 : 3); k++) {
            z = (short *)(verts + ((idx[k >> 1] >> ((k & 1) * 16)) & 0xFFFF) * 8 + 4);
            if (check && *z == to)
                return 1;
            if (!check && *z == from)
                *z = (short)to;
        }
        p += p[2] * 4;
    }
    return 0;
}

/* The church's roof (request 75, HIDDEN_AREAS.md 12): the nave `nave`'s ceiling tiles behind the towers, its
 * primitives 13..24 and 31..36, walked again through a turn node (opcode 5: half a turn about y, rotation
 * diag(-1, 1, -1), offset z 1,752), so they lie at z 832 facing up (the ceiling at z 920 faces down; the tiles are
 * symmetric in x, so the turn covers the same area). Seen from inside they are back faces; the nave's own ceiling
 * stays. Returns the group to walk instead of portHiddenAreas.group (it holds that group and the turn node), or 0
 * when the nave is not as listed. */
static unsigned char *PORT_TEXT portHiddenRoofBuild(unsigned char *nave)
{
    struct TM2PortHiddenRoof *r = &portHiddenRoof;
    unsigned char *p, *back = 0, *mid = 0;
    unsigned char *g = r->group, *m = r->turn;
    int i;

    r->built = 0;
    r->nave = 0;
    if (nave[0] != 0 || HIDDEN_W(nave, 16) != 106)
        return 0;
    p = (unsigned char *)HIDDEN_W(nave, 12);
    for (i = 0; i <= TM2_VIS_HIDDEN_ROOF_MID; i++) {
        if (p[2] == 0)
            return 0;
        if (i == TM2_VIS_HIDDEN_ROOF_BACK)
            back = p;
        if (i == TM2_VIS_HIDDEN_ROOF_MID)
            mid = p;
        if (i < TM2_VIS_HIDDEN_ROOF_MID)
            p += p[2] * 4;
    }
    if (!portHiddenRoofMesh(r->mesh[0], nave, back, TM2_VIS_HIDDEN_ROOF_BACK_N) ||
        !portHiddenRoofMesh(r->mesh[1], nave, mid, TM2_VIS_HIDDEN_ROOF_MID_N))
        return 0;
    /* the turn (opcode 5): rotation at +4 (s16, 4096 = 1), offset at +24, count +38, flags +39, children +40 */
    for (i = 0; i < 40; i++)
        m[i] = 0;
    m[0] = 5;
    *(short *)(m + 4) = -4096;
    *(short *)(m + 12) = 4096;
    *(short *)(m + 20) = -4096;
    HIDDEN_W(m, 32) = TM2_VIS_HIDDEN_ROOF_Z + TM2_VIS_HIDDEN_CEILING_Z;
    m[38] = 2;
    HIDDEN_W(m, 40) = (unsigned int)r->mesh[0];
    HIDDEN_W(m, 44) = (unsigned int)r->mesh[1];
    /* the group (opcode 1, never culled): the walls and the inside, then the roof */
    for (i = 0; i < 24; i++)
        g[i] = 0;
    g[0] = 1;
    g[20] = 2;
    g[21] = 4;
    HIDDEN_W(g, 24) = (unsigned int)portHiddenAreas.group;
    HIDDEN_W(g, 28) = (unsigned int)m;
    r->built = 2;
    r->nave = nave;
    return g;
}

/* Paris's church (HIDDEN_AREAS.md 1, 11) at the addresses of one of the two Paris files (the level file is built for
 * 0x80019C40: its nodes have fixed addresses). Zone A `zone` (opcode 11) draws the outer walls `walls` (a group over
 * the LOD node `lod`) outside its region and zone B `zone_b` inside it; zone B draws the real inside `inside` (a
 * group: the nave, the teleporter pad, the windows, the pickup spots) past the door plane and the walls plus a
 * 27-polygon stand-in before it. `lod`'s records: `far` [900,16626) (the facade with no doorway), `mid` [275,900)
 * (with it), then [0,275). The door (a select node outside the zones) has an LOD node for each state: intact `door0`
 * with its far record `rec0` [275,900), broken `door1` with `rec1` [275,1000); past them retail draws no door.
 * Every node and pointer is checked first (another level loaded: nothing is patched). Then, for the walk:
 *   - zone A draws both: the walls and the inside, through portHiddenAreas.group, whatever the eye (zone B and its
 *     stand-in are not walked; from inside, the walls are back faces);
 *   - the walls keep `mid` at every distance (`far` never matches: max 0): the doorway stays a hole;
 *   - the door's far records reach every distance: an intact door closes the doorway from anywhere, a broken one
 *     shows the inside through it;
 *   - request 75: the zone walks the roof's group (the walls, the inside and the roof behind the towers) when the
 *     inside's first child is the nave as listed (portHiddenRoofBuild), else the walls and the inside alone.
 * Returns 1 when it patched. */
static int PORT_TEXT portHiddenChurch(unsigned char *zone, unsigned char *zone_b, unsigned char *walls,
                                      unsigned char *inside, unsigned char *lod, unsigned int *far,
                                      unsigned int *mid, unsigned char *door0, unsigned int *rec0,
                                      unsigned char *door1, unsigned int *rec1)
{
    unsigned char *g, *roof;
    int i;

    portHiddenRoof.built = 0;
    if (zone[0] != 11 || HIDDEN_W(zone, 8) != (unsigned int)walls || HIDDEN_W(zone, 12) != (unsigned int)zone_b)
        return 0;
    if (zone_b[0] != 11 || HIDDEN_W(zone_b, 12) != (unsigned int)inside || inside[0] != 1 || walls[0] != 1)
        return 0;
    if (HIDDEN_W(HIDDEN_W(walls, 24), 24) != (unsigned int)lod || lod[0] != 2 || HIDDEN_W(lod, 16) != 3 ||
        HIDDEN_W(lod, 20) != (unsigned int)far || HIDDEN_W(lod, 24) != (unsigned int)mid)
        return 0;
    if (door0[0] != 2 || HIDDEN_W(door0, 20) != (unsigned int)rec0 ||
        door1[0] != 2 || HIDDEN_W(door1, 20) != (unsigned int)rec1)
        return 0;
    /* the group (opcode 1): no sphere, never culled (+21 bit 2), two children from +24 */
    g = portHiddenAreas.group;
    for (i = 0; i < 24; i++)
        g[i] = 0;
    g[0] = 1;
    g[20] = 2;
    g[21] = 4;
    HIDDEN_W(g, 24) = (unsigned int)walls;
    HIDDEN_W(g, 28) = (unsigned int)inside;
    roof = portHiddenRoofBuild((unsigned char *)HIDDEN_W(inside, 24));
    if (roof != 0)
        g = roof;
    portHiddenPatch((unsigned int *)(zone + 8), (unsigned int)g);
    portHiddenPatch((unsigned int *)(zone + 12), (unsigned int)g);
    portHiddenPatch(far, 0);
    portHiddenPatch(mid, 0xFFFFFFFF);
    portHiddenPatch(rec0, 0xFFFFFFFF);
    portHiddenPatch(rec1, 0xFFFFFFFF);
    return 1;
}

/* Request 97 (HIDDEN_AREAS.md 14): one zone (opcode 11) as listed: its two children `outside` (+8) and `inside`
 * (+12), and with `flags` >= 0 its byte +4 that value (the walker skips a zone on +4 bit 1). */
static int PORT_TEXT portHiddenZoneIs(unsigned int zone, unsigned int outside, unsigned int inside, int flags)
{
    unsigned char *z = (unsigned char *)zone;

    return z[0] == 11 && HIDDEN_W(z, 8) == outside && HIDDEN_W(z, 12) == inside && (flags < 0 || z[4] == flags);
}

/* Hong Kong's temple (request 97, HIDDEN_AREAS.md 14; HKONG.DMD, built for 0x80019C40). The room is a zone (80065664):
 * the eye inside its box walks the temple's inside only (800655FC), outside it the city only (80055270). Each of its
 * three doorways (south, west, east) is a zone too: from most of the room a 4-quad picture of the city (a card), close
 * to the door a 53-58 polygon stand-in; the city's three passages to the doors are zones that draw a 73-polygon
 * stand-in of the inside close to them. Every node is checked first (another level: nothing patched). Then, for the
 * walk:
 *   - the room's zone draws both, the city and the inside, through portHiddenAreas.group, wherever the eye is;
 *   - the three doorway zones are skipped (+4 bit 1): no card, no stand-in, the real city behind the doorway;
 *   - the three passage zones draw their empty outside group close to the doors too: no stand-in over the real
 *     inside.
 * 8 words (TM2_VIS_HIDDEN_PATCHES). Returns 1 when it patched. */
static int PORT_TEXT portHiddenTemple(void)
{
    unsigned char *g;
    int i;

    if (!portHiddenZoneIs(0x80065664, 0x80055270, 0x800655FC, -1) ||
        ((unsigned char *)0x80055270)[0] != 1 || ((unsigned char *)0x800655FC)[0] != 1)
        return 0;
    if (!portHiddenZoneIs(0x80063FB4, 0x800635D0, 0x80063F50, 1) ||
        !portHiddenZoneIs(0x80064524, 0x80063FC8, 0x800644C0, 1) ||
        !portHiddenZoneIs(0x80064B40, 0x80064538, 0x80064ADC, 1))
        return 0;
    if (!portHiddenZoneIs(0x80048CE0, 0x80048178, 0x80048C84, -1) ||
        !portHiddenZoneIs(0x80048D68, 0x80048CF4, 0x80048D0C, -1) ||
        !portHiddenZoneIs(0x80048DF0, 0x80048D7C, 0x80048D94, -1))
        return 0;
    /* the group (opcode 1): no sphere, never culled (+21 bit 2), two children from +24 */
    g = portHiddenAreas.group;
    for (i = 0; i < 24; i++)
        g[i] = 0;
    g[0] = 1;
    g[20] = 2;
    g[21] = 4;
    HIDDEN_W(g, 24) = 0x80055270;
    HIDDEN_W(g, 28) = 0x800655FC;
    portHiddenPatch((unsigned int *)0x8006566C, (unsigned int)g);
    portHiddenPatch((unsigned int *)0x80065670, (unsigned int)g);
    portHiddenPatch((unsigned int *)0x80063FB8, HIDDEN_W(0x80063FB4, 4) | 2);
    portHiddenPatch((unsigned int *)0x80064528, HIDDEN_W(0x80064524, 4) | 2);
    portHiddenPatch((unsigned int *)0x80064B44, HIDDEN_W(0x80064B40, 4) | 2);
    portHiddenPatch((unsigned int *)0x80048CEC, 0x80048178);
    portHiddenPatch((unsigned int *)0x80048D74, 0x80048CF4);
    portHiddenPatch((unsigned int *)0x80048DFC, 0x80048D7C);
    return 1;
}

/* Cyburbia (request 119, HIDDEN_AREAS.md 17; BURB.DMD, built for 0x80019C40). The arena is a zone (8008B580, one
 * 5-plane region 8008B53C: x -400..29,200, y -1,200..15,600, z under 1,600): the eye inside it walks the suburb
 * (8008B51C) only, outside it the ring highway around the arena (8003EBB4) only. The two share no surface; the four
 * curved ramp tunnels that join them are drawn always, outside the zone, and the region's edge crosses each tunnel
 * near its ring end. So from inside a tunnel the ring beyond its far opening was not drawn until the eye passed that
 * edge, and from the ring the suburb never through the tunnels or over the ring's walls. Every node is checked first
 * (another file: nothing patched); then the zone draws both, the ring and the suburb, through portHiddenAreas.group
 * (unused on BURB) wherever the eye is. No stand-in or cap exists on either side, and no band at the mouths ends short
 * at the port's scales. 2 words. Returns 1 when it patched. */
static int PORT_TEXT portHiddenBurb(void)
{
    unsigned char *g;
    int i;

    if (!portHiddenZoneIs(0x8008B580, 0x8003EBB4, 0x8008B51C, -1) || HIDDEN_W(0x8008B580, 16) != 0x8008B53C ||
        ((unsigned char *)0x8003EBB4)[0] != 1 || ((unsigned char *)0x8008B51C)[0] != 1)
        return 0;
    g = portHiddenAreas.group;
    for (i = 0; i < 24; i++)
        g[i] = 0;
    g[0] = 1;
    g[20] = 2;
    g[21] = 4;
    HIDDEN_W(g, 24) = 0x8003EBB4;
    HIDDEN_W(g, 28) = 0x8008B51C;
    portHiddenPatch((unsigned int *)0x8008B588, (unsigned int)g);
    portHiddenPatch((unsigned int *)0x8008B58C, (unsigned int)g);
    return 1;
}

/* Suicide Swamp (request 129, HIDDEN_AREAS.md 18; SWAMP.DMD, built for 0x80019C40). The arena is split into
 * visibility cells: five "show" zones whose child (55-216 meshes) is walked only while the eye is in one of the zone's
 * regions (columns of space, z under 1,600..5,760) and whose outside child is an empty group, so a cell appeared when
 * the eye crossed into one of its columns. One "hide" zone (8002AD54, inside cell 8002F23C) leaves out a stretch of
 * the ground's edge skirt (2 meshes, 16 polygons at the finest band: walls from z 0 down to -400 under the edge, the
 * kind every cell draws always) while the eye is near it: real geometry, kept out only for the PS1's sorting. Every
 * node is checked first (another file: nothing patched); then each show zone's outside word points to its cell and
 * the hide zone's inside word to the skirt, so every cell and the skirt are walked wherever the eye is. The bands are
 * untouched; the hit walk sees the file (End puts the words back). 6 words. Returns 1 when it patched. */
static const unsigned int portHiddenSwampZones[] __attribute__((section(".port_rodata"))) = {
    /* zone, its outside child, its inside child, its first region; the last is the hide zone */
    0x8002F23C, 0x80027328, 0x8002F104, 0x8002F12C,
    0x8003B2EC, 0x8002F25C, 0x8003B280, 0x8003B2A8,
    0x80042568, 0x8003B300, 0x800424FC, 0x80042524,
    0x80045974, 0x80042680, 0x800458C8, 0x800458EC,
    0x8005D854, 0x8004598C, 0x8005D7EC, 0x8005D810,
    0x8002AD54, 0x8002ACD8, 0x8002ACF8, 0x8002AD10,
};
#define PORT_SWAMP_ZONES 6
static int PORT_TEXT portHiddenSwamp(void)
{
    struct TM2PortHiddenAreas *h = &portHiddenAreas;
    const unsigned int *z;
    int i;

    for (i = 0; i < PORT_SWAMP_ZONES; i++) {
        z = portHiddenSwampZones + 4 * i;
        if (!portHiddenZoneIs(z[0], z[1], z[2], 0) || HIDDEN_W(z[0], 16) != z[3] ||
            ((unsigned char *)z[1])[0] != 1 || ((unsigned char *)z[2])[0] != 1)
            return 0;
        /* the outside groups are empty; the hide zone's inside one too */
        if (((unsigned char *)z[i == PORT_SWAMP_ZONES - 1 ? 2 : 1])[20] != 0)
            return 0;
    }
    if (h->count + PORT_SWAMP_ZONES > TM2_VIS_HIDDEN_PATCHES)
        return 0;
    for (i = 0; i < PORT_SWAMP_ZONES - 1; i++) {
        z = portHiddenSwampZones + 4 * i;
        portHiddenPatch((unsigned int *)(z[0] + 8), z[2]);      /* outside: the cell too */
    }
    z = portHiddenSwampZones + 4 * i;
    portHiddenPatch((unsigned int *)(z[0] + 12), z[1]);         /* inside: the skirt too */
    return 1;
}

/* Request 104 (HIDDEN_AREAS.md 15): node `node` a group (opcode 1) whose child `k` is `child`. */
static int PORT_TEXT portHiddenKidIs(unsigned int node, int k, unsigned int child)
{
    unsigned char *g = (unsigned char *)node;

    return g[0] == 1 && k < g[20] && HIDDEN_W(g, 24 + 4 * k) == child;
}

/* Request 104: node `lod` an LOD (opcode 2) of `n` records whose record `k` is `rec`. */
static int PORT_TEXT portHiddenRecIs(unsigned int lod, int n, int k, unsigned int rec)
{
    unsigned char *l = (unsigned char *)lod;

    return l[0] == 2 && HIDDEN_W(l, 16) == (unsigned int)n && HIDDEN_W(l, 20 + 4 * k) == rec;
}

/* Request 104: one of the subway's short bands opened, as Amazonia's (FAR_LOD.md 11): its finest record [0, inf),
 * its coarse one never (the LOD's two records: coarse first, finest second). */
static void PORT_TEXT portHiddenOpen(unsigned int lod)
{
    portHiddenPatch((unsigned int *)HIDDEN_W(lod, 24), 0xFFFFFFFF);
    portHiddenPatch((unsigned int *)HIDDEN_W(lod, 20), 0);
}

/* Hong Kong's subway (request 104, HIDDEN_AREAS.md 15; HKONG.DMD, built for 0x80019C40). The subway is a zone
 * (80070864, root 1): the eye inside its box (z -160..400 over the whole arena) walks the subway's version
 * (800707D4) only, outside it the temple's zone and the city (80065664) only. The two overlap only in the sloping
 * tubes (800382A0); each draws stand-ins of what it lacks: the subway the far mouths' facades, the city a box and
 * two short tunnels in the four mouth zones. So the eye crossing z 400 on a ramp swapped the street, the bridges and
 * the cross streets (absent below) for the subway's signs and deep tunnels (absent above). Every node is checked
 * first (another file: nothing patched). Then, for the walk:
 *   - the subway zone draws portHiddenAreas.subway wherever the eye is: the temple's zone (the city; the temple's own
 *     patches still apply) and subway_parts, the subway's own parts: its corner, end and middle tunnels (3 + 5
 *     transforms), its signs (80070664) and its 11 pickups, not its tubes or facades;
 *   - the four mouth zones draw their inside child (the real tubes) from anywhere: no box;
 *   - the city's two stand-in tunnels never (their records' max 0);
 *   - the short bands of the ends, corners, middles, tubes and the 4 signs open: finest at every distance.
 * No node is walked twice. 24 words. Returns 1 when it patched. */
static int PORT_TEXT portHiddenSubway(void)
{
    struct TM2PortHiddenAreas *h = &portHiddenAreas;
    unsigned char *g, *s;
    int i, n;

    if (!portHiddenZoneIs(0x80070864, 0x80065664, 0x800707D4, -1) || ((unsigned char *)0x800707D4)[20] != 13 ||
        !portHiddenKidIs(0x800707D4, 1, 0x800706B0) || !portHiddenKidIs(0x800706B0, 0, 0x8007068C) ||
        !portHiddenKidIs(0x8007068C, 0, 0x8006DB7C) || !portHiddenKidIs(0x8007068C, 1, 0x8006FF64) ||
        !portHiddenKidIs(0x8007068C, 2, 0x80070664))
        return 0;
    for (i = 0; i < 11; i++)
        if (!portHiddenKidIs(0x800707D4, 2 + i, 0x800706CC + 0x18 * i))
            return 0;
    if (!portHiddenKidIs(0x8006DB7C, 2, 0x8006ADEC) || !portHiddenKidIs(0x8006DB7C, 3, 0x8006D614) ||
        !portHiddenKidIs(0x8006DB7C, 4, 0x8006D62C) || !portHiddenKidIs(0x8006FF64, 2, 0x8006FC78) ||
        !portHiddenKidIs(0x8006FF64, 3, 0x8006FC90) || !portHiddenKidIs(0x8006FF64, 6, 0x8006FCEC) ||
        !portHiddenKidIs(0x8006FF64, 7, 0x8006FD18) || !portHiddenKidIs(0x8006FF64, 8, 0x8006FD30))
        return 0;
    if (!portHiddenZoneIs(0x80038B94, 0x800359A0, 0x80038B28, -1) ||
        !portHiddenZoneIs(0x80038CE0, 0x80038BD4, 0x80038C74, -1) ||
        !portHiddenZoneIs(0x80049F64, 0x80049EDC, 0x80049EF4, -1) ||
        !portHiddenZoneIs(0x8004A000, 0x80049F78, 0x80049F90, -1))
        return 0;
    if (!portHiddenRecIs(0x80038ADC, 1, 0, 0x80038AD0) || !portHiddenRecIs(0x800386F8, 1, 0, 0x800386EC))
        return 0;
    if (!portHiddenRecIs(0x8006D5DC, 2, 0, 0x8006B010) || !portHiddenRecIs(0x8006D5DC, 2, 1, 0x8006D5D0) ||
        !portHiddenRecIs(0x8006ADB4, 2, 0, 0x80067834) || !portHiddenRecIs(0x8006ADB4, 2, 1, 0x8006ADA8) ||
        !portHiddenRecIs(0x8006FC40, 2, 0, 0x8006DED8) || !portHiddenRecIs(0x8006FC40, 2, 1, 0x8006FC34) ||
        !portHiddenRecIs(0x80038284, 2, 0, 0x80035BC4) || !portHiddenRecIs(0x80038284, 2, 1, 0x80038278) ||
        !portHiddenRecIs(0x80070164, 2, 0, 0x80070000) || !portHiddenRecIs(0x80070164, 2, 1, 0x80070158) ||
        !portHiddenRecIs(0x80070374, 2, 0, 0x80070210) || !portHiddenRecIs(0x80070374, 2, 1, 0x80070368) ||
        !portHiddenRecIs(0x800704D0, 2, 0, 0x800703FC) || !portHiddenRecIs(0x800704D0, 2, 1, 0x800704C4) ||
        !portHiddenRecIs(0x8007062C, 2, 0, 0x80070558) || !portHiddenRecIs(0x8007062C, 2, 1, 0x80070620))
        return 0;
    if (h->count + 24 > TM2_VIS_HIDDEN_PATCHES)
        return 0;
    /* the subway's own parts (opcode 1: no sphere, never culled (+21 bit 2), the children from +24) */
    s = h->subway_parts;
    for (i = 0; i < 24; i++)
        s[i] = 0;
    s[0] = 1;
    s[21] = 4;
    n = 0;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006ADEC;     /* the corner tunnels' and the ends' transforms */
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006D614;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006D62C;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006FC78;     /* the middle tunnels', the corners' and the ends' */
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006FC90;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006FCEC;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006FD18;
    HIDDEN_W(s, 24 + 4 * n++) = 0x8006FD30;
    HIDDEN_W(s, 24 + 4 * n++) = 0x80070664;     /* the signs */
    for (i = 0; i < 11; i++)
        HIDDEN_W(s, 24 + 4 * n++) = 0x800706CC + 0x18 * i;     /* the pickups */
    s[20] = (unsigned char)n;
    g = h->subway;
    for (i = 0; i < 24; i++)
        g[i] = 0;
    g[0] = 1;
    g[20] = 2;
    g[21] = 4;
    HIDDEN_W(g, 24) = 0x80065664;
    HIDDEN_W(g, 28) = (unsigned int)s;
    portHiddenPatch((unsigned int *)0x8007086C, (unsigned int)g);
    portHiddenPatch((unsigned int *)0x80070870, (unsigned int)g);
    portHiddenPatch((unsigned int *)0x80038B9C, 0x80038B28);
    portHiddenPatch((unsigned int *)0x80038CE8, 0x80038C74);
    portHiddenPatch((unsigned int *)0x80049F6C, 0x80049EF4);
    portHiddenPatch((unsigned int *)0x8004A008, 0x80049F90);
    portHiddenPatch((unsigned int *)0x80038AD0, 0);
    portHiddenPatch((unsigned int *)0x800386EC, 0);
    portHiddenOpen(0x8006D5DC);
    portHiddenOpen(0x8006ADB4);
    portHiddenOpen(0x8006FC40);
    portHiddenOpen(0x80038284);
    portHiddenOpen(0x80070164);
    portHiddenOpen(0x80070374);
    portHiddenOpen(0x800704D0);
    portHiddenOpen(0x8007062C);
    return 1;
}

/* PC port request 114 (MOD_ARENAS.md, port_mod_arena.h): carTick's hidden full-heal spot. TM2_MOD_HEAL_RETAIL: the
 * level's square (carTick's switch); TM2_MOD_HEAL_SPOT: the 64 x 64 square at (healX, healZ), stored in *x, *z;
 * TM2_MOD_HEAL_NONE: no spot. Any other value of portArenaRules.heal is retail's. */
int PORT_TEXT portArenaHealSpot(int *x, int *z)
{
    if (portArenaRules.heal == TM2_MOD_HEAL_SPOT) {
        *x = portArenaRules.healX;
        *z = portArenaRules.healZ;
        return TM2_MOD_HEAL_SPOT;
    }
    if (portArenaRules.heal == TM2_MOD_HEAL_NONE)
        return TM2_MOD_HEAL_NONE;
    return TM2_MOD_HEAL_RETAIL;
}

/* PC port request 113 (HIDDEN_AREAS.md 16): breakable walls drawn from both sides while they stand. Their intact
 * faces are one-sided (retail culls them from behind), so a car on the far side (spawned inside Paris's church, say)
 * saw through a wall it then hit. Each group: a count, then the face's control word address (primitive +12) and its
 * disc value; bit 31 set is the emitter's "no backface cull" (sub_800CDC54: `(s32)ctl >= 0` culls). A group is set
 * only when every one of its words holds its disc value outside bits 13..15 and has bit 31 clear (that level is
 * loaded, the wall untouched). Bits 13..15 are the face's quadrant, which meshSetPrimQuadrants (dbscan) writes over
 * the disc's when the level loads (request 113b). */
#define PORT_TWO_SIDED_KEY 0xFFFF1FFFu      /* the word without the quadrant bits; bit 31 kept, so it must be clear */
static const unsigned int portTwoSidedFaces[] __attribute__((section(".port_rodata"))) = {
    6,  /* PARIS: the church door (target 503, select 8002EEF0 child 0; LOD near 4 faces, far 2) */
    0x8002E6B0, 0x15844000, 0x8002E6D4, 0x15842000, 0x8002E6F8, 0x15844000, 0x8002E71C, 0x15844000,
    0x8002E630, 0x15840000, 0x8002E654, 0x15844000,
    6,  /* PARIS2: the same door (select 8002F0A8) */
    0x8002E868, 0x15844000, 0x8002E88C, 0x15844000, 0x8002E8B0, 0x15842000, 0x8002E8D4, 0x15844000,
    0x8002E7E8, 0x15844000, 0x8002E80C, 0x15840000,
    4,  /* ISLANDS: the temple's two breakable walls (target 504, select 8006957C), two panels each */
    0x8006942C, 0x15844000, 0x80069450, 0x15842000, 0x80069474, 0x16840000, 0x8006949C, 0x15842000,
    2,  /* ISLANDS: the two lone breakable panels at (-6250, -400) and (-6250, 400) (selects 800696A8, 8006B064) */
    0x80069634, 0x15842000, 0x80069760, 0x15840000,
    1,  /* BURB: the breakable panel at (22752, 8352) (target 502, select 8008A158) */
    0x80089E3C, 0x15844000,
    0
};

/* renderScene, after portHiddenAreasBegin: with portVisual bit 6, bit 31 on the listed faces of the loaded level
 * until portHiddenAreasDrawn. Returns how many (renderScene adds it to its `hidden`, so Drawn runs). */
int PORT_TEXT portTwoSidedBegin(void)
{
    const unsigned int *g;
    int i, n, done = 0;

    if (!(portVisual & TM2_VIS_HIDDEN_AREAS))
        return 0;
    for (g = portTwoSidedFaces; (n = g[0]) != 0; g += 1 + 2 * n) {
        for (i = 0; i < n; i++)
            if ((*(unsigned int *)g[1 + 2 * i] & PORT_TWO_SIDED_KEY) != (g[2 + 2 * i] & PORT_TWO_SIDED_KEY))
                break;
        if (i < n)
            continue;
        for (i = 0; i < n; i++)
            *(unsigned int *)g[1 + 2 * i] |= 0x80000000u;
        done += n;
    }
    return done;
}

/* portHiddenAreasDrawn (after the emitter and the near pass have read the faces): every face Begin set back to what
 * Begin found. Needs no record: a listed word matching its key with bit 31 set is one Begin set (bit 31 was clear),
 * so clearing bit 31 gives it back, the quadrant bits as they are. */
void PORT_TEXT portTwoSidedEnd(void)
{
    const unsigned int *g;
    int i, n;

    for (g = portTwoSidedFaces; (n = g[0]) != 0; g += 1 + 2 * n)
        for (i = 0; i < n; i++)
            if ((*(unsigned int *)g[1 + 2 * i] & PORT_TWO_SIDED_KEY) ==
                ((g[2 + 2 * i] | 0x80000000u) & PORT_TWO_SIDED_KEY))
                *(unsigned int *)g[1 + 2 * i] &= 0x7FFFFFFFu;
}

/* renderScene, before its walks (after portFarLodBegin): with portVisual bit 6, patch the loaded level's hidden
 * areas; records in drawn[view] the words patched (0 with the bit clear or nothing listed). Reads only the level
 * file's fixed addresses; no rand(). */
int PORT_TEXT portHiddenAreasBegin(int view)
{
    struct TM2PortHiddenAreas *h = &portHiddenAreas;

    h->count = 0;
    h->place = TM2_VIS_HIDDEN_NONE;
    portHiddenRoof.built = 0;
    portHiddenRoof.nave = 0;
    portHiddenRoof.lowered = 0;
    if (portVisual & TM2_VIS_HIDDEN_AREAS) {
        if (portHiddenChurch((unsigned char *)0x8002E5E4, (unsigned char *)0x8002E58C, (unsigned char *)0x8002C0B0,
                             (unsigned char *)0x8002E510, (unsigned char *)0x8002C074, (unsigned int *)0x8002B668,
                             (unsigned int *)0x8002B8E0, (unsigned char *)0x8002E740, (unsigned int *)0x8002E66C,
                             (unsigned char *)0x8002EEB0, (unsigned int *)0x8002EC6C))
            h->place = TM2_VIS_HIDDEN_PARIS;
        else if (portHiddenChurch((unsigned char *)0x8002E79C, (unsigned char *)0x8002E744,
                                  (unsigned char *)0x8002C2D8, (unsigned char *)0x8002E6CC,
                                  (unsigned char *)0x8002C29C, (unsigned int *)0x8002B890,
                                  (unsigned int *)0x8002BB08, (unsigned char *)0x8002E8F8,
                                  (unsigned int *)0x8002E824, (unsigned char *)0x8002F068,
                                  (unsigned int *)0x8002EE24))
            h->place = TM2_VIS_HIDDEN_PARIS2;
        else if (portHiddenTemple()) {
            h->place = TM2_VIS_HIDDEN_HKONG;    /* request 97 */
            portHiddenSubway();                 /* request 104 */
        } else if (portHiddenBurb())
            h->place = TM2_VIS_HIDDEN_BURB;     /* request 119 */
        else if (portHiddenSwamp())
            h->place = TM2_VIS_HIDDEN_SWAMP;    /* request 129 */
        /* request 78: the eye above the roof (viewGetEyeTrans is minus the eye's position): the nave's top at the
           roof's height for this walk, the roof's turn moved to keep it at z 832 */
        if (portHiddenRoof.built == 2 && -viewGetEyeTrans(view)[2] > TM2_VIS_HIDDEN_ROOF_Z &&
            !portHiddenRoofMove(portHiddenRoof.nave, TM2_VIS_HIDDEN_CEILING_Z, TM2_VIS_HIDDEN_ROOF_Z, 1)) {
            portHiddenRoofMove(portHiddenRoof.nave, TM2_VIS_HIDDEN_CEILING_Z, TM2_VIS_HIDDEN_ROOF_Z, 0);
            HIDDEN_W(portHiddenRoof.turn, 32) = TM2_VIS_HIDDEN_ROOF_Z + TM2_VIS_HIDDEN_ROOF_Z;
            portHiddenRoof.lowered = 1;
        }
    }
    h->drawn[view & (TM2_VIS_HIDDEN_VIEWS - 1)] = h->count;
    return h->count;
}

/* renderScene, after its walks (before portFarLodEnd): every patched word back, the last first. */
void PORT_TEXT portHiddenAreasEnd(void)
{
    struct TM2PortHiddenAreas *h = &portHiddenAreas;
    int i;

    for (i = h->count - 1; i >= 0; i--)
        *h->word[i] = h->saved[i];
    h->count = 0;
}

/* renderScene, after the emitter and the near pass (request 84, HIDDEN_AREAS.md 12.7): the nave's top back at z 920.
 * The walk only queues the meshes; the emitter and renderNearPolys read their vertices afterwards, so the vertices
 * request 78 moves stay moved until the view is drawn (End, before the emitter, was too early: the walls were drawn
 * whole and the roof, whose turn had moved, 88 lower). Nothing else reads mesh vertices (the hit walk uses hit
 * volumes). */
void PORT_TEXT portHiddenAreasDrawn(void)
{
    portTwoSidedEnd();      /* request 113 */
    if (portHiddenRoof.lowered) {
        portHiddenRoofMove(portHiddenRoof.nave, TM2_VIS_HIDDEN_ROOF_Z, TM2_VIS_HIDDEN_CEILING_Z, 0);
        portHiddenRoof.lowered = 0;
    }
}
#endif /* TM2_PORT_HIDDEN_AREAS */

#ifdef TM2_PORT_ROLLBACK
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
/* ---- online, stage 1 (PC port request 48, docs/ONLINE_ROLLBACK.md 8.1 and "Stage 1 as built") ----
 * None of these calls rand(). With portRollbackOptions zero each one leaves retail's state alone. */
extern short timerFrameTicks, timerFrameRate;
extern unsigned int timerFrameCount, timerLastFrameDelta, timerBaseRate;
extern int rtFrameTime;
extern unsigned int GetCurTics(void);
extern void ResetUpdateRate(void);
extern void srand(unsigned int seed);
extern void rtScheduleEvent(int value, int time);
#pragma tm2_absolute timerFrameTicks
#pragma tm2_absolute timerFrameRate
#pragma tm2_absolute timerFrameCount
#pragma tm2_absolute timerLastFrameDelta
#pragma tm2_absolute timerBaseRate

/* FIXED_TICKS: the ticks of a step: portStepTicks clamped to 3..5 (SetFrameStart's range), 0 = 3 (20 steps a second)
 * in every game mode. The user's decision (2026-10-19): co-op (game mode 2) runs at 20 steps a second online too, not
 * retail's 15 (rtFrameTime 15, 4 ticks), which overrules the port's provisional 15. */
static int PORT_TEXT portRollbackTicks(void)
{
    int t = portStepTicks;

    if (t == 0)
        t = 3;
    if (t < 3)
        t = 3;
    if (t > 5)
        t = 5;
    return t;
}

/* SetFrameStart's frame with FIXED_TICKS: ticks, rate (timerBaseRate 60 / ticks) and the delta (ticks VBlanks of
 * 70,618 tics) fixed, timerFrameCount + 1; timerFrameStart / timerPrevFrameStart are left alone (no clock). */
static void PORT_TEXT portRollbackSetTicks(void)
{
    int t = portRollbackTicks();

    timerFrameTicks = (short)t;
    timerFrameRate = (short)(timerBaseRate / (unsigned int)t);
    timerLastFrameDelta = (unsigned int)t * 70618u;
}

/* portStepBody's first call (request 86, ONLINE_ROLLBACK.md 23): `bytes` (words, at most TM2_RB_SCRUB_MAX) below the
 * caller's frame set to portStepScrubFill. The caller's sp is the address of this function's first argument (o32:
 * a0's home slot is the word at the caller's sp); the function keeps no frame of its own (check_rollback checks it), so
 * nothing it needs lies below that address. */
void PORT_TEXT portStepStackScrub(int bytes)
{
    unsigned int *p = (unsigned int *)&bytes;
    unsigned int fill = portStepScrubFill;
    int n = (bytes > TM2_RB_SCRUB_MAX ? TM2_RB_SCRUB_MAX : bytes) >> 2;

    while (n-- > 0)
        *--p = fill;
}

int PORT_TEXT portRollbackFixedFrame(void)
{
    if (!(portRollbackOptions & TM2_RB_FIXED_TICKS))
        return 0;
    portRollbackSetTicks();
    timerFrameCount += 1u;
    return 1;
}

/* rtMainLoop's entry (before its first worldUpdate): with FIXED_TICKS the round's first step has the fixed ticks
 * too (retail's are the last measurement: 5 after a load or a pause). */
void PORT_TEXT portRollbackLoopEnter(void)
{
    portRollbackHolding = 0;
    portRollbackSplitting = 0;      /* stage 3: no step split before the first top, no end pending */
    portRollbackEnded = 0;
    if (portRollbackOptions & TM2_RB_FIXED_TICKS)
        portRollbackSetTicks();
    portClutShadowCapture();        /* stage 3 CLUT_SHADOW: the round's palette rows from VRAM */
}

/* rtMainLoop, after it set rtFrameTime (the pacing: a step waits until 60 / VBlanks <= rtFrameTime): with
 * FIXED_TICKS a step lasts its ticks (60 / ticks: 20, 15 or 12). */
void PORT_TEXT portRollbackFrameTime(void)
{
    if (portRollbackOptions & TM2_RB_FIXED_TICKS)
        rtFrameTime = 60 / portRollbackTicks();
}

/* The top of each iteration of rtMainLoop (before rtUpdateScheduledEvents). Latches HOLD for the whole iteration:
 * 1 = this iteration only draws (no event countdown, no pad latch, no update, no SetFrameStart, no palette step).
 * Without FIXED_TICKS a held iteration moves the frame measurement's start (ResetUpdateRate), so the step after the
 * hold measures one frame, not the hold. A step that runs first takes a pending quit (PAUSE_PORT): event 2 at
 * time 0, which rtUpdateScheduledEvents fires in this same step, as retail's QUIT does after pauseMenu. */
int PORT_TEXT portRollbackTop(void)
{
    /* stage 3 STEP_SPLIT: latched with HOLD; 1 = rtMainLoop calls portRollbackLoopStep (which runs the step, unless
       held) instead of rtUpdateScheduledEvents, and the iteration's own update calls are skipped (portRollbackHeld) */
    portRollbackSplitting = (portRollbackOptions & TM2_RB_STEP_SPLIT) != 0;
    portRollbackHolding = (portRollbackOptions & TM2_RB_HOLD) && portStepHold;
    if (portRollbackHolding) {
        if (!(portRollbackOptions & TM2_RB_FIXED_TICKS))
            ResetUpdateRate();
        return 1;
    }
    if ((portRollbackOptions & TM2_RB_PAUSE_PORT) && portRollbackQuit) {
        portRollbackQuit = 0;
        rtScheduleEvent(2, 0);
    }
    return portRollbackSplitting || portRollbackEnded;
}

/* RB_STEP's gate in rtMainLoop: 1 = skip the iteration's update call (held, or STEP_SPLIT: the step ran at the top).
 * The drawing gates (portRollbackViewTickOff, ...) read the HOLD latch alone. */
int PORT_TEXT portRollbackHeld(void)
{
    return portRollbackHolding || portRollbackSplitting;
}

/* SEED: the seed of srand site `site` (TM2_RB_SITE_*): portRollbackSeed + 0x9E3779B9 x (the srand calls since the
 * load's seed) + 0x10001 x site, then the count + 1; without the bit retail's GetCurTics(). */
unsigned int PORT_TEXT portRollbackSeedFor(int site)
{
    unsigned int seed;

    if (!(portRollbackOptions & TM2_RB_SEED))
        return GetCurTics();
    seed = portRollbackSeed + portRollbackReseeds * 0x9E3779B9u + (unsigned int)site * 0x10001u;
    portRollbackReseeds++;
    return seed;
}

/* sessionLoadArenaAndCars' start (SEED): the load's draws (targetTimedEventInit, targetPlayerEventInit,
 * targetActorPlace, ...) come from the match seed, and timerFrameCount (its parity is read by the logic) starts at
 * the host's portRollbackFrameCount. Every load of the match does this (also the Minion / Dark Tooth reloads). */
void PORT_TEXT portRollbackPreload(void)
{
    int i;

    /* request 54: a new match, no model file installed yet (assetInstallImage notes each one: portDmdNoteImage) */
    for (i = 0; i < TM2_RB_DMD_IMAGES; i++)
        portDmdImages.base[i] = 0;
    portDmdImages.overflow = 0;
    if (!(portRollbackOptions & TM2_RB_SEED))
        return;
    portRollbackReseeds = 0;
    srand(portRollbackSeedFor(TM2_RB_SITE_LOAD));
    timerFrameCount = portRollbackFrameCount;
}

/* pauseMenu's entry: 1 with PAUSE_PORT (the menu never opens; the port's menu quits through portRollbackQuit). */
int PORT_TEXT portRollbackPauseOff(void)
{
    return (portRollbackOptions & TM2_RB_PAUSE_PORT) != 0;
}

/* ---- online, stage 2 (request 48, docs/ONLINE_ROLLBACK.md 11): drawing independence ----
 * ONCE_PER_STEP (bit 2), ANIM_IN_STEP (bit 3; its step is portRollbackAnimStep in engine_anim_hit_volumes.c),
 * VISUAL_RAND (bit 4). The gates call no rand(); portRollbackStepEffects runs the game's own ticks (which do). */
extern int fxSheetTimer;
extern void fxSheetDisplay(void *ot);
extern void weaponDropsUpdate(void *ctx, int view);
extern void weaponFlameUpdate(void);
extern void weaponArcsDraw(unsigned char **ctx, int view);

/* ONCE_PER_STEP: streaksDraw's per-view work in one call per step. */
int PORT_TEXT portRollbackOncePerStep(void)
{
    return (portRollbackOptions & TM2_RB_ONCE_PER_STEP) != 0;
}

/* The pass of a split tick-and-draw function (fxSheetDisplay, weaponArcsDraw): without ONCE_PER_STEP retail's
 * combined pass; with it the step's call (step_call: OT / context 0) ticks and a view's call draws. */
int PORT_TEXT portRollbackPass(int step_call)
{
    if (!(portRollbackOptions & TM2_RB_ONCE_PER_STEP))
        return TM2_RB_PASS_BOTH;
    return step_call ? TM2_RB_PASS_TICK : TM2_RB_PASS_DRAW;
}

/* streaksDraw, weaponDropsUpdate / weaponFlameUpdate: 1 = skip (a held iteration, stage 1; ONCE_PER_STEP: the step
 * runs them, portRollbackStepEffects). */
int PORT_TEXT portRollbackViewTickOff(void)
{
    return portRollbackHolding || (portRollbackOptions & TM2_RB_ONCE_PER_STEP) != 0;
}

/* streaksDraw, fxSheetDisplay / weaponArcsDraw: 1 = skip (a held iteration without ONCE_PER_STEP: retail's combined
 * pass would tick; with the bit the call only draws, held or not). */
int PORT_TEXT portRollbackViewDrawOff(void)
{
    return portRollbackHolding && !(portRollbackOptions & TM2_RB_ONCE_PER_STEP);
}

/* rtMainLoop, after view 0's streaksDraw (RB_STEP: not while held): with ONCE_PER_STEP streaksDraw's four ticks once
 * per step, in retail's order: the flash countdown and fade, the drops (mine and bomb fuses, the hit tests), the
 * flames (damage, ignition, rand()), the arcs (the taser retry, rand(), the countdown, the damage). */
void PORT_TEXT portRollbackStepEffects(void)
{
    if (!(portRollbackOptions & TM2_RB_ONCE_PER_STEP))
        return;
    if (fxSheetTimer > 0)
        fxSheetDisplay(0);
    weaponDropsUpdate(0, 0);
    weaponFlameUpdate();
    weaponArcsDraw(0, 0);
}

/* animUpdateAll / animUpdateOne: 1 = step nothing (a held iteration, stage 1; ANIM_IN_STEP: the step steps them). */
int PORT_TEXT portRollbackAnimDrawOff(void)
{
    return portRollbackHolding || (portRollbackOptions & TM2_RB_ANIM_IN_STEP) != 0;
}

/* The animators' rand() (animStepRandom, animStepWeightedRandom, animPickPeriod): 1 = the visual stream. VISUAL_RAND
 * moves them only while the renderer steps them (no ANIM_IN_STEP): stepped by the step they are simulation (the hit
 * volumes of DISH and FREEWAY), the same on every machine, and stay on the game's stream. */
int PORT_TEXT portRollbackAnimVisual(void)
{
    return (portRollbackOptions & (TM2_RB_VISUAL_RAND | TM2_RB_ANIM_IN_STEP)) == TM2_RB_VISUAL_RAND;
}

/* weaponArcsDraw's jitter (the end point and the 14 inner points): 1 = the visual stream: a drawing pass of
 * ONCE_PER_STEP (it must leave the game's stream alone), or VISUAL_RAND in any pass. */
int PORT_TEXT portRollbackArcVisual(int pass)
{
    return pass == TM2_RB_PASS_DRAW || (portRollbackOptions & TM2_RB_VISUAL_RAND) != 0;
}

/* The visual stream: retail's rand() (seed x 1103515245 + 12345, bits 16..30) on portRollbackVisualSeed. */
int PORT_TEXT portRollbackVisualRand(void)
{
    portRollbackVisualSeed = portRollbackVisualSeed * 1103515245u + 12345u;
    return (int)((portRollbackVisualSeed >> 16) & 0x7FFF);
}

/* ---- online, section 18 (docs/ONLINE_ROLLBACK.md 18): this machine's view settings ----
 * The split screen (viewSplitScreenMode) and the rear-view mirror (rtRearViewActive / Allowed) are each machine's own:
 * the port sets the split from its local players and the mirror from its hud_mirror setting and the local hotkeys.
 * Every read of them in the step is presentation (sound, camera, HUD, the view hotkeys) except the weapon
 * regeneration of humanCarTick in builds without TM2_PORT_HUMANS (car_tick.c REGEN_SPLIT), which reads this gate.
 * LOCAL_VIEWS (bit 10): the match's split mode, retail's choice for the game mode and level (sessionRun:
 * session_runner.cpp), the same on every machine; without the bit this machine's (retail). Calls no rand(). */
extern int gameGetMode(void);
extern int gameGetLevel(void);
extern int viewGetSplitMode(void);

int PORT_TEXT portRollbackViewSplit(void)
{
    int level;

    if (!(portRollbackOptions & TM2_RB_LOCAL_VIEWS))
        return viewGetSplitMode();
    if (gameGetMode() != 2 && gameGetMode() != 4)
        return 0;
    level = gameGetLevel();
    return level == 2 || level == 3 || level == 8 ? 1 : 2;
}

/* ---- online, request 60 (docs/ONLINE_ROLLBACK.md 21): the projectile records at a round's start ----
 * projectilesReset (carRestart: every bind and round start, every respawn) sets every record's life to -1, the first
 * step of the 11-step fade (fxRecordsUpdate runs weaponProjectileUpdate while life >= -11), and keeps the type, the
 * target, the victim and the rest; missilesCreateObjects (the load) gives each record a new object and life -15 but
 * keeps them too. So a seeker of the round before steers its new object at its old target for 11 steps: after a level
 * change a pointer into the re-allocated csObjectPool, now another object's (a camera's: each machine's own).
 * ROUND_CLEAN (bit 11): roundStart clears every record first (the fields a fire sets: +0x02..+0x33, +0x38..+0x47:
 * type, life -15 as the load's, speeds, damage, hit, origin, target, victim, the trail's counters, owner, drop; not
 * the id +0x00, the object +0x34, the trail's vertices, object and strip from +0x48, which are the load's) and hides
 * its object (+8 = 0, as projectilesReset does); the round's projectilesReset then fades records of type 0 with no
 * target, which only refresh their own trail's slot. Every fire sets these fields again (fxRecordInitObject and the
 * fire functions). And a seeker whose target is no counted car object's position (+0xA8) stops steering: the
 * search's targets, AI_AIM_TARGET and the boomerang's own car are car objects; a stale pointer, RETAIL_BUGS 37's 0xA8
 * and a vacated car's old object are not. Neither calls rand(); without the bit both do nothing. */
extern struct TM2Object *humanCarObject(int index);
extern struct TM2Object *aiCarObject(int index);
extern int humanCarCount(void);
extern int aiCarCount(void);
extern int fxRecords[][980 / 4];

void PORT_TEXT portRoundCleanProjectiles(void)
{
    unsigned char *r;
    int *obj;
    int i, k;

    if (!(portRollbackOptions & TM2_RB_ROUND_CLEAN))
        return;
    for (i = 0; i < TM2_PROJECTILES_LIVE; i++) {
        r = (unsigned char *)fxRecords[i];
        *(short *)(r + 0x02) = 0;           /* type */
        *(short *)(r + 0x04) = -15;         /* life: idle (missilesCreateObjects' value) */
        *(short *)(r + 0x06) = 0;           /* speed */
        for (k = 0x08; k < 0x34; k += 4)    /* rate, damage, hit, +0x10..+0x27, the origin */
            *(int *)(r + k) = 0;
        for (k = 0x38; k < 0x48; k += 4)    /* target, victim, seg / slot / owner, count / timer / drop */
            *(int *)(r + k) = 0;
        obj = *(int **)(r + 0x34);
        if (obj)
            obj[2] = 0;                     /* hidden */
    }
}

int PORT_TEXT portSeekerTargetOk(void *target)
{
    unsigned char *o;
    int i, n;

    if (!(portRollbackOptions & TM2_RB_ROUND_CLEAN))
        return 1;
    n = humanCarCount();
    for (i = 0; i < n && i < TM2_MAX_HUMANS; i++)
        if ((o = (unsigned char *)humanCarObject(i)) != 0 && target == (void *)(o + 0xA8))
            return 1;
    n = aiCarCount();
    for (i = 0; i < n && i < TM2_MAX_AI_CARS; i++)
        if ((o = (unsigned char *)aiCarObject(i)) != 0 && target == (void *)(o + 0xA8))
            return 1;
    return 0;
}

/* ---- online, stage 3 (request 48, docs/ONLINE_ROLLBACK.md 12): the split step, re-runs, sound, palettes, the
 * models' mutable fields ----
 * STEP_SPLIT (bit 8; portStepBody / portRunStep / portRollbackLoopStep are in rt_main.c beside rtMainLoop),
 * SOUND_GATE (bit 5), CLUT_SHADOW (bit 6), portDmdMutableList. None of these calls rand(). */
extern void hudRadarSwapBuffers(int index);
extern unsigned char *soundManagerObject;
extern int soundStopInstance(int, int);
#ifndef TM2_PORT_CAR_SOUND
extern unsigned char sndEffectTable[];     /* 20 bytes per id: +0 state (1 = a list per id, soundMgrPlay) */
#endif
extern int fxRecords[][980 / 4];          /* projectiles, 980 bytes each: +0x02 s16 type, +0x04 s16 life (> 0 live) */
extern void gpuStoreImageLine(void *p, int x, int y, int w);
extern void gpuLoadImageLine(void *p, int x, int y, int w);
extern int clutAnimLastIndex;
extern int clutAnimDefs[TM2_MAX_CLUT_ANIM_DEFS][20 / 4];       /* +0 id/sub, +4 type, +8 x, +12 y, +16 count */

/* STEP_SPLIT: portStepBody, before worldUpdate (whose radar plots fill the back buffer): the swap that retail's
 * hudDisplayRadar does after drawing a radar, once per step for every HUD. So a batch of re-runs leaves only the last
 * step's blips (retail's swap per drawn frame would pile them up), and every frame drawn in a step shows them. */
void PORT_TEXT portRollbackHudStep(void)
{
    int i;

    if (!(portRollbackOptions & TM2_RB_STEP_SPLIT))
        return;
    for (i = 0; i < TM2_MAX_HUDS; i++)
        hudRadarSwapBuffers(i);
}

/* hudDisplayRadar: 1 = leave the swap to the step (this iteration is split). */
int PORT_TEXT portRollbackRadarInStep(void)
{
    return portRollbackSplitting;
}

/* SOUND_GATE: 1 while the port re-runs steps (portResimulating) with the bit: the sound wrappers, soundUpdate, the
 * port's instance merge and worldUpdate's CD track test do nothing. The first run's sounds keep playing. */
int PORT_TEXT portRollbackSoundOff(void)
{
    return (portRollbackOptions & TM2_RB_SOUND_GATE) && portResimulating;
}

/* SOUND_GATE, called by the port after a batch of re-runs (portResimulating cleared): the sounds whose stop site
 * fires only on a transition. The projectiles' flight sounds (ids 10, 11, 12, 15: soundPlayVariant(id, .., slot + 1)
 * at launch, soundStopInstance(id, slot + 1) when the projectile hits or runs out, projectile_hit.c) of a
 * projectile that the corrected steps never launched, or that is gone, would play on: stop every request of those
 * lists whose projectile slot is not live with that type. The loops (turbo, gun, burn, the computer cars' gun, the
 * engines, the flame) need nothing: their stop or start sites test the state every step, so the next live step
 * heals them, provided the sound side's flags (soundTurboLoopActiveP1.., soundActiveIdNN, portCarSoundLoops) are left
 * out of the snapshot (rollback_regions.tsv). One-shots of mispredicted steps play, and a one-shot only the corrected
 * steps have is lost (8.6 risk 5, accepted). */
void PORT_TEXT portSoundReconcile(void)
{
    unsigned char *list, *n, *rec;
    int k, id, inst, live;

    if (!(portRollbackOptions & TM2_RB_SOUND_GATE) || portResimulating || soundManagerObject == 0)
        return;
    for (k = 0; k < 4; k++) {
        id = k < 3 ? 10 + k : 15;
        if (*(int *)((unsigned char *)sndEffectTable + id * 20) != 1)
            continue;
        list = soundManagerObject + id * 20 + 0xCC;     /* +0x0C head; a request: +0x04 next, +0x2C instance */
        n = *(unsigned char **)(list + 12);
        while (n != 0) {
            inst = *(int *)(n + 0x2C);
            live = 0;
            if (inst >= 1 && inst <= TM2_MAX_PROJECTILES) {
                rec = (unsigned char *)fxRecords[inst - 1];
                live = *(short *)(rec + 4) > 0 && *(short *)(rec + 2) == id;
            }
            n = *(unsigned char **)(n + 4);     /* before the stop unlinks the request */
            if (inst >= 1 && !live)
                soundStopInstance(id, inst);
        }
    }
}

/* CLUT_SHADOW: the shadow row of (x, y), or a new one (count 0) when `make`; 0 when none / no room. */
static struct TM2PortClutRow *PORT_TEXT portClutRow(int x, int y, int make)
{
    int i;

    for (i = 0; i < portClutShadow.rows; i++)
        if (portClutShadow.row[i].x == x && portClutShadow.row[i].y == y)
            return &portClutShadow.row[i];
    if (!make)
        return 0;
    if (portClutShadow.rows >= TM2_RB_CLUT_ROWS) {
        portClutShadow.overflow++;
        return 0;
    }
    i = portClutShadow.rows++;
    portClutShadow.row[i].x = x;
    portClutShadow.row[i].y = y;
    portClutShadow.row[i].count = 0;
    return &portClutShadow.row[i];
}

static void PORT_TEXT portClutCopy(unsigned short *dst, const unsigned short *src, int count)
{
    int i;

    if (count > 256)
        count = 256;
    for (i = 0; i < count; i++)
        dst[i] = src[i];
}

/* The palette animations' upload (clutAnimStep*: retail's gpuLoadImageLine). With CLUT_SHADOW the row goes to the
 * shadow, and to VRAM unless re-running; without it, retail's upload. */
void PORT_TEXT portClutLoadLine(unsigned short *buf, int x, int y, int count)
{
    struct TM2PortClutRow *r;

    if (portRollbackOptions & TM2_RB_CLUT_SHADOW) {
        r = portClutRow(x, y, 1);
        if (r != 0) {
            portClutCopy(r->c, buf, count);
            if (count > r->count)
                r->count = count > 256 ? 256 : count;
        }
        if (portResimulating)
            return;
    }
    gpuLoadImageLine(buf, x, y, count);
}

/* The palette animations' read-back (clutAnimStart*: retail's gpuStoreImageLine, VRAM to RAM). With CLUT_SHADOW the
 * row comes from the shadow when it holds it (in a re-run VRAM still has the discarded steps' palettes), else from
 * VRAM, which then seeds the shadow; without it, retail's read. */
void PORT_TEXT portClutStoreLine(unsigned short *buf, int x, int y, int count)
{
    struct TM2PortClutRow *r;

    if (portRollbackOptions & TM2_RB_CLUT_SHADOW) {
        r = portClutRow(x, y, 1);
        if (r != 0 && r->count >= count) {
            portClutCopy(buf, r->c, count);
            return;
        }
        gpuStoreImageLine(buf, x, y, count);
        if (r != 0) {
            portClutCopy(r->c, buf, count);
            r->count = count > 256 ? 256 : count;
        }
        return;
    }
    gpuStoreImageLine(buf, x, y, count);
}

/* CLUT_SHADOW, request 59 (3) (ONLINE_ROLLBACK.md 20.3): the shadow emptied, every word (rows 0, no overflow, the
 * colours 0), so it holds the current round's rows only: the rows of an earlier round (a tournament's earlier level)
 * are never uploaded again by portClutAnimUpload, and two machines hash the same bytes whatever rounds they ran. */
void PORT_TEXT portClutShadowReset(void)
{
    int *w;
    int i;

    w = (int *)&portClutShadow;
    for (i = 0; i < (int)(sizeof(portClutShadow) / sizeof(int)); i++)
        w[i] = 0;
}

/* CLUT_SHADOW, rtMainLoop's entry (portRollbackLoopEnter): every definition's row read from VRAM (the round's
 * palettes after the load, the same on every machine), so no read-back of the round depends on VRAM. The shadow
 * starts empty (portClutShadowReset, request 59 (3)): only this round's definitions' rows. The port may call it again
 * after it reloads palettes outside a step (a drop-in's car, CAR_GROUPS 14): every row is a definition's
 * (clutAnimStart* / clutAnimStep* take def->x, def->y), and each was read from VRAM here before as well. */
void PORT_TEXT portClutShadowCapture(void)
{
    struct TM2PortClutRow *r;
    int i, n;

    if (!(portRollbackOptions & TM2_RB_CLUT_SHADOW))
        return;
    portClutShadowReset();
    for (i = 0; i <= clutAnimLastIndex && i < TM2_MAX_CLUT_ANIM_DEFS; i++) {
        n = clutAnimDefs[i][4];
        if (n <= 0)
            continue;
        if (n > 256)
            n = 256;
        r = portClutRow(clutAnimDefs[i][2], clutAnimDefs[i][3], 1);
        if (r == 0)
            continue;
        gpuStoreImageLine(r->c, clutAnimDefs[i][2], clutAnimDefs[i][3], n);
        r->count = n;
    }
}

/* CLUT_SHADOW, called by the port after a batch of re-runs, and after it loads a snapshot (a join, a rollback with no
 * re-run): every shadow row to VRAM, so the palettes on screen are the state's. */
void PORT_TEXT portClutAnimUpload(void)
{
    int i;

    if (!(portRollbackOptions & TM2_RB_CLUT_SHADOW))
        return;
    for (i = 0; i < portClutShadow.rows && i < TM2_RB_CLUT_ROWS; i++)
        if (portClutShadow.row[i].count > 0)
            gpuLoadImageLine(portClutShadow.row[i].c, portClutShadow.row[i].x, portClutShadow.row[i].y,
                             portClutShadow.row[i].count);
}

/* ---- portDmdMutableList (ONLINE_ROLLBACK.md 3.2, 12, 15.1): the model nodes' fields a step may write ----
 * The models are loaded once and stay where they are (no heap); a step writes a few node fields (3.2). Request 54:
 * the walk covers every model FILE loaded for the match, whatever object holds a model now: every root of the level
 * file (0x80019C40: the arena, the camera-attached model, the effects', weapons' and pickups' models that no object
 * holds until one is made) and every root of every installed file (portDmdImages: the cars, the spare groups, a
 * drop-in's car), as assetInstallImage reaches them (root word + base - 0x80019C40), through every child (all LOD
 * bands, all animation frames, all select children, both zone children). So the list depends on the loads alone:
 * the same for every step of the match (request 48's walk of the objects' current models missed a node that an
 * object made later reached). Nodes outside the files (the projectiles' trail nodes in fxRecordTrailSegments, the
 * car groups' freeze records in portCarGroups) lie in the static sections, which rollback_regions.tsv's ranges copy
 * whole. Every node a file's chained pointers name (animation +0x1C, select +0x0C, hit box +0x0C) is a child in the
 * same file's tree (all 29 DMD files of the disc). Per node type (FILE_FORMATS.md 2.4) the bytes that may change, a
 * little wider than 3.2's table (the flag bytes too: restoring an unchanged byte costs nothing):
 *   1 group [+0x14, +0x18)   3 animation [+0x06, +0x14)   4 offset [+0x04, +0x14)   5 transform [+0x04, +0x28)
 *   8 hit box [+0x04, +0x0C)   9 select [+0x06, +0x0C)   11 zones [+0x04, +0x08)
 * Request 106: the walk reads child counts that lie in those ranges, and one of them changes in play: a pickup's
 * holder (a group or offset node of object id 401..430) is hidden and shown by its count (pickupSetVisible: 0 when
 * taken, 1 when it comes back; worldLoadObject hides one at the load with 0). Every holder of the disc's 29 DMD
 * files has count 1, so the walk takes at least one child of a holder (DMD_KIDS): the file's count, whatever the
 * pickup's state. Three more steps zero a count the walk reads, to hide a piece of the level for the rest of the
 * round: worldCollapseRelease (Antarctica's floes), laEventsUpdate (Los Angeles' quake piece, node 4006) and
 * moscowDishUpdate (request 111 found it; this comment said none). A list made after one of them skips that piece's
 * children (the node holding the count is still listed, so a state from before the hide puts the count back, but
 * not bytes a step wrote below it, if any: not checked). Make the list at the load, before any state. (The selects' +0x07 is the chosen child, not the count; the hidden areas' patches are undone before the
 * step.) */
#define DMD_PICKUP(node) ((unsigned int)(*(unsigned short *)((node) + 2) - 401) < 30)
#define DMD_KIDS(node, n) ((n) == 0 && DMD_PICKUP(node) ? 1 : (n))
struct PortDmdOut {
    unsigned int *out;
    int max, n;
};

static void PORT_TEXT portDmdAdd(struct PortDmdOut *o, unsigned char *node, int from, int to)
{
    if (o->n < o->max) {
        o->out[o->n * 2] = (unsigned int)(node + from);
        o->out[o->n * 2 + 1] = (unsigned int)(to - from);
    }
    o->n++;
}

static void PORT_TEXT portDmdWalk(struct PortDmdOut *o, unsigned char *node, int depth)
{
    unsigned char **child;
    int i, n;

    if (node == 0 || depth > 64)
        return;
    switch (node[0]) {
    case 1:
        portDmdAdd(o, node, 0x14, 0x18);
        n = DMD_KIDS(node, node[0x14]);     /* request 106: a pickup's holder, taken or not */
        child = (unsigned char **)(node + 0x18);
        break;
    case 2:
        n = *(int *)(node + 0x10);
        child = (unsigned char **)(node + 0x14);
        for (i = 0; i < n && i < 64; i++)
            if (child[i] != 0)
                portDmdWalk(o, *(unsigned char **)(child[i] + 8), depth + 1);
        return;
    case 3:
        portDmdAdd(o, node, 0x06, 0x14);
        n = node[0x0B];
        child = (unsigned char **)(node + 0x20);
        break;
    case 4:
        portDmdAdd(o, node, 0x04, 0x14);
        n = DMD_KIDS(node, node[0x12]);     /* request 106: as the group's */
        child = (unsigned char **)(node + 0x14);
        break;
    case 5:
        portDmdAdd(o, node, 0x04, 0x28);
        n = node[0x26];
        child = (unsigned char **)(node + 0x28);
        break;
    case 8:
        portDmdAdd(o, node, 0x04, 0x0C);
        return;
    case 9:
        portDmdAdd(o, node, 0x06, 0x0C);
        n = node[0x06];
        child = (unsigned char **)(node + 0x10);
        break;
    case 11:
        portDmdAdd(o, node, 0x04, 0x08);
        portDmdWalk(o, *(unsigned char **)(node + 0x08), depth + 1);
        portDmdWalk(o, *(unsigned char **)(node + 0x0C), depth + 1);
        return;
    case 12:
        n = *(unsigned short *)(node + 0x06);
        child = (unsigned char **)(node + 0x08);
        break;
    default:
        return;
    }
    for (i = 0; i < n; i++)
        portDmdWalk(o, child[i], depth + 1);
}

/* Every root of the model file at base (a "PSXD" image, version 0x43), as assetInstallImage relocated them: the root
 * word + base - 0x80019C40 (the level file: + 0); a zero root stops, as there. Not a model file: nothing. */
static void PORT_TEXT portDmdWalkFile(struct PortDmdOut *o, unsigned int base)
{
    unsigned int *h = (unsigned int *)base;
    unsigned int delta, root, i;

    if (base == 0 || h[0] != 0x50535844u || h[1] != 0x43u)
        return;
    delta = base - TM2_RB_LEVEL_DMD;
    for (i = 0; i < h[6] && i < 64; i++) {
        root = h[7 + i] + delta;
        if (root == 0)
            return;
        portDmdWalk(o, (unsigned char *)root, 0);
    }
}

/* assetInstallImage (request 54): the file just installed as load group `group` (the match's cars and spare groups,
 * a drop-in's portCarLoadGroupLive); a later install of the same group replaces it. */
void PORT_TEXT portDmdNoteImage(void *base, int group)
{
    if (group > 0 && group < TM2_RB_DMD_IMAGES)
        portDmdImages.base[group] = (unsigned int)base;
    else
        portDmdImages.overflow++;
}

/* Writes (address, bytes) pairs into out (2 words a pair, at most max pairs), sorted by address with overlapping and
 * touching ranges merged (a node shared by several models is listed once); returns the pair count, or minus the raw
 * pairs needed when max is too small (call again with a larger buffer). Call it after the match's load and after
 * each load or unload of a model file between steps (portCarLoadGroupLive, portCarUnloadGroup); nothing else changes
 * it (a car swap, portCarStartHuman / portCarRemove, objects made or freed, the steps). The list is the same on every
 * machine that loaded the same match. Pure: reads the files, writes only out. */
int PORT_TEXT portDmdMutableList(unsigned int *out, int max)
{
    struct PortDmdOut o;
    unsigned int a, s, gap;
    int i, j, k, m;

    o.out = out;
    o.max = max;
    o.n = 0;
    portDmdWalkFile(&o, TM2_RB_LEVEL_DMD);
    for (i = 1; i < TM2_RB_DMD_IMAGES; i++)
        portDmdWalkFile(&o, portDmdImages.base[i]);
    if (o.n > max)
        return -o.n;
    /* shell sort by address */
    for (gap = (unsigned int)o.n / 2; gap > 0; gap /= 2) {
        for (i = (int)gap; i < o.n; i++) {
            a = out[i * 2];
            s = out[i * 2 + 1];
            for (j = i; j >= (int)gap && out[(j - (int)gap) * 2] > a; j -= (int)gap) {
                out[j * 2] = out[(j - (int)gap) * 2];
                out[j * 2 + 1] = out[(j - (int)gap) * 2 + 1];
            }
            out[j * 2] = a;
            out[j * 2 + 1] = s;
        }
    }
    /* merge */
    m = 0;
    for (k = 0; k < o.n; k++) {
        a = out[k * 2];
        s = out[k * 2 + 1];
        if (m > 0 && a <= out[(m - 1) * 2] + out[(m - 1) * 2 + 1]) {
            if (a + s > out[(m - 1) * 2] + out[(m - 1) * 2 + 1])
                out[(m - 1) * 2 + 1] = a + s - out[(m - 1) * 2];
            continue;
        }
        out[m * 2] = a;
        out[m * 2 + 1] = s;
        m++;
    }
    return m;
}
#endif /* TM2_PORT_ROLLBACK */

/* Request 89 (port_rollback.h, ONLINE_ROLLBACK.md 25): the port variant's stack. The variant's code before the
 * game's data is larger than retail's, so its BSS ends at 0x801FE8E8 instead of 801FAD48 and crt0's stack
 * (0x80200000 down) had 5.9 KB before it ran into libsnd's last statics (_svm_okof1 / _svm_okof2). Here the stack
 * is 64 KB of .port_bss: build_source_only.py sets crt0's stack word (2MBYTE's table, __SN_ENTRY_POINT + 0xB0) and
 * the EXE header's s_addr to the end of portStack. portStackGuard lies right below it: a stack that overflows
 * writes it first. Every variant, last in .port_bss (nothing before them moves); snapshot and hash no. */
int portCameraWallWork[TM2_PORT_CAMERA_WORK / 4] PORT_BSS = {0};
/* request 91 item 3 (BALANCE.md 20, port_balance.h struct TM2PortBalanceJam): per car slot, bit 27's jam count and
 * way round; every variant (as portBalanceCars), match state */
struct TM2PortBalanceJam portBalanceJam[TM2_MAX_CAR_SLOTS] PORT_BSS = {{0}};
/* request 92 (port_visual.h struct TM2PortSteadyShadow): portVisual bit 7's saved shadow matrices; every variant,
 * drawing only */
struct TM2PortSteadyShadow portSteadyShadow PORT_BSS = {{0}};
/* request 98 (port_far_lod.h): the per-view budget of the far-LOD band scale; every variant, drawing only */
int portFarLodBudget PORT_BSS = 0;
struct TM2PortFarLodBudget portFarLodBudgetState PORT_BSS = {0};
/* request 102 item 3 (port_balance.h): bit 30's last explosion sound; every variant, match state */
int portBalanceBoom[4] PORT_BSS = {0};
/* request 114 (port_mod_arena.h, MOD_ARENAS.md): a mod arena's health spot and warp exits; every variant, written
   by the port at arena load, 0 = retail */
TM2PortArenaRules portArenaRules PORT_BSS = {0};
/* request 117 (port_wreck_fall.h): per car slot, the wreck has exploded at the floor; every variant, match state */
unsigned char portWreckFallMark[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};
/* request 118 (dt_player.h): a phase-2 player Dark Tooth's hit-only model (a sphere over the head's box, 108 bytes,
   built at its first use) and per car slot the truck model it stands in for; every variant, match state */
unsigned int portDtPhase2Tree[27] PORT_BSS = {0};
void *portDtPhase2Truck[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};
/* request 121 (port_humans.h, ANALOG_STEERING.md 6.4): per human, the analog throttle 0..4096 (0 = not written:
   retail), written by the port with the input word; read by portHumanThrottleShare; every variant, an input */
short portHumanThrottle[TM2_MAX_HUMANS] PORT_BSS = {0};
/* request 123 (port_team_locks.h, PLAYTEST_ANSWERS.md 37.3): per car slot, the car's team as the port sets it, 0 = none
   (retail); read by portTeamMates; every variant, match state */
unsigned char portCarTeam[TM2_MAX_CAR_SLOTS] PORT_BSS = {0};
/* request 124 (port_balance.h, BALANCE.md 26): the second word of Vanilla Plus switches (TM2_BAL2_*, set by the
   port; 0 = retail) and its copy for the round (portBalanceLatch); every variant, used only with TM2_PORT_BALANCE */
int portBalance2 PORT_BSS = 0;
int portBalanceLive2 PORT_BSS = 0;
/* request 127 (port_balance.h, BALANCE.md 27): the mines' clocks (portBalance2 bits 2, 3); every variant, match state */
int portBalanceMineSeconds PORT_BSS = 0;
int portBalanceMineTicks PORT_BSS = 0;
unsigned int portBalanceMineStep PORT_BSS = 0;
/* request 126 (port_human_wreck.h): a player's wreck marked to live a computer car's wreck's life; every variant,
   match state */
int portHumanWreckLive[TM2_MAX_HUMANS] PORT_BSS = {0};
int portBalanceMineAge[TM2_MAX_DROPS] PORT_BSS = {0};
int portBalanceMineOrphan[TM2_MAX_DROPS] PORT_BSS = {0};
unsigned int portStackGuard[TM2_PORT_STACK_GUARD / 4] PORT_BSS = {0};
unsigned int portStack[TM2_PORT_STACK / 4] PORT_BSS __attribute__((aligned(8))) = {0};


#ifdef TM2_PORT_CAR_POSE
/* ---- a car posed for a cutscene (PC port request 108, docs/CAR_POSE.md) ----
 * portCarPose(kind, index, x, y, floor_z, yaw): the car of human record `index` (kind 0) or computer record `index`
 * (kind 1) standing still at (x, y), facing yaw, level, its wheels on a flat floor at floor_z (given, not probed),
 * every speed and spin zero, the turbo flame and the skid smoke off; then what the draw reads of the car as a step
 * leaves it: the three motion matrices (CarRotUpdate), the four wheels, the shadow (carBodyPoseUpdate), the car's
 * object (as humanCarUpdate / aiCarUpdate copy it) and a Dark Tooth's head (240 over the truck as his hover settles;
 * a beaten boss's -24). No ground probe, physics, AI, weapon, damage, sound or rand(). Returns 1, or 0 for an index
 * out of range. For the port between steps (a hold, or its own frame with the RAM put back after); called again
 * every frame it writes the same words. */
#ifndef TM2_PORT_CAR_SPECIALS
#error "TM2_PORT_CAR_POSE needs TM2_PORT_CAR_SPECIALS (a Dark Tooth's head by his car key)"
#endif
extern void CarRotUpdate(void *car, unsigned char human);
extern void carBodyPoseUpdate(void *car, int human);
extern void mathMulTransVec(void *m, void *v, void *out);
extern void mathMulMatVec(void *m, void *v, void *out);

int PORT_TEXT portCarPose(int kind, int index, int x, int y, int floor_z, int yaw)
{
    struct TM2CarState *st;
    struct TM2CarAim *aim;
    struct TM2CarStatsBlock *stats;
    struct TM2CarWheels *w;
    struct TM2CarMotion *m;
    struct TM2CarInput *in;
    struct TM2Object *obj, *head;
    void *car;
    int slot, type, human, k, h, sel;
    int v[3], hover[3];

    if (kind == 0 && (unsigned int)index < TM2_MAX_HUMANS) {
        struct TM2HumanRecord *r = (struct TM2HumanRecord *)humanCars[index];
        car = r;
        st = &r->state; aim = &r->aim; stats = &r->stats; w = &r->wheels; m = &r->motion; in = &r->input;
        slot = r->slot; type = r->type;
        obj = (struct TM2Object *)humanCarObjects[index];
        r->speed_change = 0;                /* carBodySwayUpdate's pitch lean */
    } else if (kind == 1 && (unsigned int)index < TM2_MAX_AI_CARS) {
        struct TM2AIRecord *r = (struct TM2AIRecord *)aiCars[index];
        car = r;
        st = &r->state; aim = &r->aim; stats = &r->stats; w = &r->wheels; m = &r->motion; in = &r->input;
        slot = r->slot; type = r->type;
        obj = (struct TM2Object *)aiCarObjects[index];
    } else
        return 0;
    human = kind == 0;

    /* the motion: still, level, facing yaw, the body at the floor plus the ride height */
    m->steer = 0;
    m->vel0 = m->speed = m->vel2 = 0;
    m->spin0 = m->spin1 = m->yaw_rate = 0;
    m->fE8 = 0;
    for (k = 0; k < 3; k++) {
        m->vel[k] = 0;
        m->body_spin[k] = 0;
        m->rot[k] = m->body_rot[k] = 0;
    }
    m->rot[2] = m->body_rot[2] = yaw;      /* bounded by CarRotUpdate */
    m->pos[0] = x;
    m->pos[1] = y;
    m->pos[2] = floor_z + stats->ride_height;

    /* the body's tilts and shake (CarRotUpdate adds the leans to body_m) */
    aim->shake_level = aim->shake = 0;
    aim->shake_count = aim->shake_phase = 0;
    aim->lean_pitch = aim->lean_roll = 0;
    aim->wheel_pitch = aim->wheel_roll = 0;
    aim->shake_amp_x = aim->shake_amp_y = 0;
    aim->pitch = aim->f32 = 0;
    CarRotUpdate(car, human);

    /* the wheels on the floor (carUpdateContacts' corners, no probe) */
    for (k = 0; k < 3; k++)
        w->move[k] = 0;
    for (k = 0; k < 4; k++) {
        struct TM2CarWheel *e = &w->wheel[k];
        int j;

        v[0] = k & 1 ? stats->track_width / 2 : -stats->track_width / 2;
        v[1] = k < 2 ? stats->axle_front : stats->axle_rear;
        v[2] = -stats->ride_height;
        mathMulTransVec(&m->rot_m, v, e->corner);
        for (j = 0; j < 3; j++)
            e->pos[j] = m->pos[j] + e->corner[j];
        e->h = e->level = e->f1C = e->land_z = e->f26 = floor_z;
        e->sway = e->shake = e->sway_low = e->crush = e->f1A = e->f2C = e->f12 = e->fall = 0;
        e->sway_up = e->clamped = 0;
        e->on = 1;
        e->touch = 0;
    }
    st->wheel_in_air = st->all_in_air = st->falling = 0;
    st->sliding = st->f01 = st->f02 = 0;            /* the skid smoke */
    in->turbo = 0;
    if (st->f1C) {                                  /* the turbo flame */
        st->f1C = 0;
        portCarTurboSetStep(TM2_CAR_KEY(slot, type), 0);
    }
    carBodyPoseUpdate(car, human);                  /* the shadow */

    if (obj != 0) {                                 /* humanCarUpdate / aiCarUpdate */
        obj->rot[0] = m->rot[0];
        obj->rot[1] = m->rot[1];
        obj->rot[2] = m->rot[2];
        for (k = 0; k < 3; k++)
            obj->pos[k] = m->pos[k];
        mathCopyWords(obj->matrix, &m->body_m, 8);
    }

    if (type == 0xF0) {                             /* his head, where the hover settles; turned as it was */
        sel = portCarSpecialSelect(TM2_CAR_KEY(slot, type));
        head = darkToothBossObject;
        if (head != 0) {
            h = !human && aiBossDefeated() ? -24 : 240;
            v[0] = 0;
            v[1] = 0;
            v[2] = h;
            mathMulMatVec(&m->rot_m, v, hover);
            for (k = 0; k < 3; k++)
                head->pos[k] = m->pos[k] + hover[k];
        }
        portCarSpecialRestore(sel);
    }
    return 1;
}

/* ---- request 108's addendum (docs/CAR_POSE.md "Each car's special sound") ----
 * portCarSpecialSound(car): soundPlay's first argument for the special of car number `car` (a car-select index 0..14;
 * with TM2_PORT_CAR_NUMBERS a mod's 0x20 + m) as its firing plays it (weaponFireSpecial; Hammerhead's at the ram,
 * carCrashStart; Mr. Slam's as the swing starts, weaponSlamGrabUpdate; Dark Tooth's swoop); -1 for no car. A mod car
 * fires its portSpecialProfile template (TM2_PORT_SPECIAL_PROFILE: a car type's special, or a pickup weapon's launch
 * sound), else its base's. The sound alone: nothing is fired, played or written.
 * portCarSpecialSoundHeld(code): 1 for a sound the driver holds until soundStop(code) (sndEffectTable kind 3 or 8:
 * Thumper's flame 47, Roadkill's boomerang 0x33, the ricochet bomb 0x12; the game stops them as the effect ends),
 * 0 for a one-shot (kind 12 or 13: every other special's). */
static int PORT_TEXT portCarSpecialSoundOfType(int t)
{
    /* if-chains, not a switch or a table: a jump table or a const array would need .rodata, which the port variant
       has no place for */
    if (t == 0x101)
        return 10;              /* TM2_SPECIAL_PICKUP_FIRE: weaponLaunchProjectile's kind 1 */
    if (t == 0x102)
        return 11;              /* HOMING (2) */
    if (t == 0x103)
        return 0x13;            /* BOMB: portSpecialBomb's placement */
    if (t == 0x104)
        return 12;              /* POWER (4) */
    if (t == 0x105 || t == 0x10A)
        return 15;              /* NAPALM, MEGA (5) */
    if (t == 0x106)
        return 0x12;            /* RICOCHET: portSpecialRicochet (held) */
    if (t == 0x164)
        return 14;              /* FREEZE (energy kind 100) */
    if (t == 0x10)
        return 0x3D;            /* Sweet Tooth: the spiral missile */
    if (t == 0xE0)
        return 0x0D;            /* Minion */
    if (t == 0xF0)
        return 0x42;            /* Dark Tooth: the swoop */
    if (t < 0x20 || t > 0xD0 || (t & 15) != 0)
        return -1;
    /* 0x20 Hammerhead 0x25 (the ram, carCrashStart), 0x30 Outlaw 2 0x27 (the taser, on a hit), 0x40 Warthog 0x29,
       0x50 Mr. Grimm 0x2B, 0x60 Grasshopper 0x2D, 0x70 Thumper 0x2F (the flame, held), 0x80 Spectre 0x31,
       0x90 Roadkill 0x33 (the boomerang, held), 0xA0 Twister 0x35, 0xB0 Axel 0x37, 0xC0 Mr. Slam 0x39 (the swing),
       0xD0 Shadow 0x3B: the odd ids after each engine's */
    return 0x25 + 2 * ((t >> 4) - 2);
}

int PORT_TEXT portCarSpecialSound(int car)
{
    int t;

    if (car >= 0 && car <= 14)
        t = car < 12 ? (car + 2) << 4 : car == 12 ? 0x10 : car == 13 ? 0xE0 : 0xF0;   /* carIndexToType */
#ifdef TM2_PORT_CAR_NUMBERS
    else if (car >= TM2_PORT_MOD_FIRST && car < TM2_PORT_MOD_FIRST + TM2_PORT_MOD_CARS) {
        t = portCarNumberType(car);
        if (t <= 0)
            return -1;
#ifdef TM2_PORT_SPECIAL_PROFILE
        if (portCarProfile[car - TM2_PORT_MOD_FIRST].valid) {
            /* the template as portSpecialTemplate takes it: a pickup, or a borrowable type other than its own */
            int s = portSpecialProfile[car - TM2_PORT_MOD_FIRST].special;

            if (portSpecialIsPickup(s))
                t = s;
            else if (s > 0 && (s & 15) == 0 && s <= 0xF0 && ((TM2_PORT_SPECIAL_AS_MASK >> (s >> 4)) & 1))
                t = s;
        }
#endif
    }
#endif
    else
        return -1;
    return portCarSpecialSoundOfType(t);
}

int PORT_TEXT portCarSpecialSoundHeld(int code)
{
    return code == 0x2F || code == 0x33 || code == 0x12;
}
#endif /* TM2_PORT_CAR_POSE */

#ifdef TM2_PORT_GLACIER_FIRE
/* ---- Antarctica's fire missile on floe 5 (PC port request 120, RETAIL_BUGS.md 45, PLAYTEST_ANSWERS.md 34) ----
 * GLACIER.DMD (built for 0x80019C40): the fire pickup 80051B40 (opcode 4, id 401, at (600, -800, 4800) in floe 5's
 * frame, under its LOD 80051B64 centred on it) lies inside the hit-only mark volume 800561E8 (6 planes, kind -2: the
 * slab 4800 < z < 6400 over part of the floes), which the hit walk reaches first and which marks every car point in it
 * (hit 2): the pickup's own leaf then skips them, so no car ever collects it. The power (404) and homing (403) missiles
 * beside it lie outside the mark. Moved here to (992, 119): (600, -800) turned by the floe's 60 degrees, as if its
 * offset had been written in the unturned frame (it then sits by the other two, on the same ground, section 5 at
 * 4800); its LOD's centre moves with it, z kept. Every word is checked first (another file, or patched already:
 * nothing). Level data: the same on every machine, before dbscanHierarchy registers the pickups. Returns 1 when it
 * moved it. */
extern int gameGetLevel(void);

int PORT_TEXT portGlacierFire(void)
{
    unsigned char *pick = (unsigned char *)0x80051B40, *lod = (unsigned char *)0x80051B64;
    unsigned char *floe = (unsigned char *)0x80051B7C;
    int *p = (int *)pick, *l = (int *)lod;

    if (gameGetLevel() != 6)
        return 0;
    if (pick[0] != 4 || *(unsigned short *)(pick + 2) != 401 || p[1] != 600 || p[2] != -800 || p[3] != 4800)
        return 0;
    if (lod[0] != 2 || l[1] != 600 || l[2] != -800 || l[3] != 4851)
        return 0;
    if (floe[0] != 5 || *(unsigned short *)(floe + 2) != 3100 || *(unsigned char **)(floe + 44) != lod)
        return 0;
    p[1] = l[1] = 992;
    p[2] = l[2] = 119;
    return 1;
}
#endif /* TM2_PORT_GLACIER_FIRE */

#ifdef TM2_PORT_WRECK_FALL
/* ---- a falling wreck hidden with its splash, or exploding at the floor (PC port request 117, port_wreck_fall.h,
 * docs/PLAYTEST_ANSWERS.md 33) ---- */
#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
extern int carWorldObjectCount;
extern int gameGetLevel(void);
extern int worldFallDeathZ(void);
extern void csSetDrawEnable(void *object, unsigned char value);
extern void fxSplashAt(int *pos, int d);
extern void fxSmokeA24(void *pos, int flag);
extern void fxExplosion48(void *pos, unsigned char flag);
extern void fxExplosion24(void *pos, unsigned char flag);
extern void carSpinOut(void *car, int human, int kind, int keep_motion);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *pos);
extern void soundLocateForView(void *pos, int view);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlay(int id, int a, int b, int c);

#ifdef TM2_PORT_FALL_WHEELS
#define WRECK_FALL_Z(st, m) ((m)->pos[2] - (st)->ride_height)   /* request 107: as humanCarTick / aiCarDrive */
#else
#define WRECK_FALL_Z(st, m) ((m)->pos[2])
#endif

/* The car's blocks and its slot (carDestroyed's: humanCarObjectSlots / aiCarObjectSlots by the record's number);
 * -1 when the slot is not one of this load's. */
static int PORT_TEXT portWreckFallCar(void *car, int human, struct TM2CarState **state,
                                      struct TM2CarStatsBlock **stats, struct TM2CarMotion **motion)
{
    int n, slot;

    if (human) {
        struct TM2HumanRecord *r = (struct TM2HumanRecord *)car;
        n = r->player;
        if ((unsigned int)n >= TM2_MAX_HUMANS)
            return -1;
        slot = humanCarObjectSlots[n];
        *state = &r->state; *stats = &r->stats; *motion = &r->motion;
    } else {
        struct TM2AIRecord *r = (struct TM2AIRecord *)car;
        n = r->index;
        if ((unsigned int)n >= TM2_MAX_AI_CARS)
            return -1;
        slot = aiCarObjectSlots[n];
        *state = &r->state; *stats = &r->stats; *motion = &r->motion;
    }
    if ((unsigned int)slot >= TM2_MAX_CAR_SLOTS || slot >= carWorldObjectCount || carSlotObjects[slot] == 0)
        return -1;
    return slot;
}

/* The pit test of the fall death (humanCarTick / aiCarDrive): falling, and past worldFallDeathZ. */
static int PORT_TEXT portWreckFallFloor(struct TM2CarState *state, struct TM2CarStatsBlock *stats,
                                        struct TM2CarMotion *motion)
{
    return state->falling && WRECK_FALL_Z(stats, motion) < worldFallDeathZ();
}

void PORT_TEXT portWreckFallBlown(void *car, int human)
{
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarMotion *motion;
    int slot;

    slot = portWreckFallCar(car, human, &state, &stats, &motion);
    if (slot >= 0)
        portWreckFallMark[slot] = portWreckFallFloor(state, stats, motion);
}

void PORT_TEXT portWreckFall(void *car, int human)
{
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarMotion *motion;
    unsigned char *obj;
    int slot, level, type;
    int pos[3];

    slot = portWreckFallCar(car, human, &state, &stats, &motion);
    if (slot < 0)
        return;
    if (!state->dead) {
        portWreckFallMark[slot] = 0;    /* alive: carDestroyed will decide (portWreckFallBlown) */
        return;
    }
    obj = (unsigned char *)carSlotObjects[slot];
    if (obj[8] == 0)                    /* csSetDrawEnable's byte: hidden already (carDestroyed, here) or parked */
        return;
    level = gameGetLevel();
    if ((level == 6 && motion->pos[2] < 240) || (level == 8 && motion->pos[2] < -40)) {
        /* carDestroyed's hide */
        csSetDrawEnable(obj, 0);
        if (level == 6) {
            fxSplashAt(motion->pos, 2000);
            if (human)
                TM2_HUMAN_SOUND(((struct TM2HumanRecord *)car)->player, 34, 0, 0, 99);
        }
        if (stats->wreck_timer > 0)
            stats->wreck_timer = 0;
        portWreckFallMark[slot] = 1;
        return;
    }
    if (portWreckFallMark[slot] || !portWreckFallFloor(state, stats, motion))
        return;
    /* carDestroyed's explosion, at the floor */
    portWreckFallMark[slot] = 1;
    type = human ? ((struct TM2HumanRecord *)car)->type : ((struct TM2AIRecord *)car)->type;
    if (viewGetSplitMode() == 0)
        soundLocateForView0(motion->pos);
    else
        soundLocateForView(motion->pos, -1);
    if (type == 0x60 || type == 0x30 || type == 0xA0)
        soundPlay(26, PadChkMtap() >> 1, soundGetListenerPan(), 99);
    else if (PadChkMtap() < 1000)
        soundPlay(25, 0, 0, 47);
    else
        soundPlay(25, PadChkMtap() / 3, soundGetListenerPan(), 47);
    pos[0] = motion->pos[0];
    pos[1] = motion->pos[1];
    pos[2] = 0;
    fxSmokeA24(pos, 0);
    carSpinOut(car, human, 4, 0);
    fxExplosion48(motion->pos, 0);
    fxExplosion24(motion->pos, 0);
}
#undef WRECK_FALL_Z
#endif /* TM2_PORT_WRECK_FALL */

#ifdef TM2_PORT_DT_SWOOP
/* ---- a computer Dark Tooth's swoop at the car he is after (PC port request 121 item 1, port_dt_swoop.h,
 * docs/PLAYTEST_ANSWERS.md 35) ---- */
#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"
#include "semantic/port_dt_swoop.h"
#include "semantic/port_team_locks.h"   /* request 123: portTeamNearestFoe */
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif
extern int aiCarTargets[];
extern int aiCarCount(void);
extern int humanCarNearest(int *pos, int skip);
extern struct TM2Object *humanCarObject(int index);
extern struct TM2Object *aiCarObject(int index);
extern int darkToothSwoopVectors[];
extern void carApplyDamage(int id, int amount, int *dir, int type, int attacker);
extern struct TM2FireEffect *fxFlameBurning(void *pos, int arg);
extern void fxSmokeA12(void *pos, unsigned char flag);
extern void fxExplosion24(void *pos, unsigned char flag);
extern int soundPlay(int id, int a, int b, int c);

/* Computer car j as a swoop target: in the match, drawn and alive (as a player Dark Tooth's targets). */
static int PORT_TEXT portDtSwoopAi(int j)
{
    struct TM2Object *obj;
    struct TM2AIRecord *a;

    if (j < 0 || j >= aiCarCount())
        return 0;
    obj = aiCarObject(j);
    a = aiCarGet(j);
    return obj != 0 && obj->visible && !a->state.dead && a->stats.hp > 0;
}

/* The swoop's start: his own target when it is a computer car he can reach (not himself), else retail's nearest
 * player. No rand(). */
int PORT_TEXT portDtSwoopPick(void *boss)
{
    struct TM2AIRecord *b = (struct TM2AIRecord *)boss;
    int j = aiCarTargets[b->index];

    if (j != b->index && portDtSwoopAi(j))
        return TM2_DT_SWOOP_AI + j;
#ifdef TM2_PORT_TEAM_LOCKS
    return portTeamNearestFoe(b->index);    /* request 123: the nearest player not of his team */
#else
    return humanCarNearest(b->motion.pos, -1);
#endif
}

/* The target's position: the record's motion.pos (darkToothSwoopTarget). */
int *PORT_TEXT portDtSwoopPos(int code)
{
    if (code >= TM2_DT_SWOOP_AI)
        return aiCarGet(code - TM2_DT_SWOOP_AI)->motion.pos;
    return humanCarGet(code)->motion.pos;
}

/* While swooping: 0 when a computer car target has left the match (vacated, gone or dead); players as retail. */
int PORT_TEXT portDtSwoopLive(int code)
{
    return code < TM2_DT_SWOOP_AI || portDtSwoopAi(code - TM2_DT_SWOOP_AI);
}

/* The hit (aiDarkToothBossUpdate's): 7 damage from his slot along darkToothSwoopVectors[3..5], then, unless the car
 * burns already, is shielded, in water or in fire, the car set burning with retail's effects and sound 6. The same
 * calls in the same order as retail's on a player. */
void PORT_TEXT portDtSwoopHit(int code, int slot)
{
    struct TM2Object *obj;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarWeapons *weapons;
    int *pos;

    if (code >= TM2_DT_SWOOP_AI) {
        struct TM2AIRecord *a = aiCarGet(code - TM2_DT_SWOOP_AI);

        obj = aiCarObject(code - TM2_DT_SWOOP_AI);
        state = &a->state;
        stats = &a->stats;
        weapons = &a->weapons;
        pos = a->motion.pos;
    } else {
        struct TM2HumanRecord *h = humanCarGet(code);

        obj = humanCarObject(code);
        state = &h->state;
        stats = &h->stats;
        weapons = &h->weapons;
        pos = h->motion.pos;
    }
    carApplyDamage(obj->id, 7, &darkToothSwoopVectors[3], 0, slot);
    if (!state->burning && !state->energy_shield && !state->in_water && !state->in_fire) {
        state->burning = 1;
        stats->burn_time = 0;
        weapons->fire_effect = fxFlameBurning(pos, 1);
        fxSmokeA12(pos, 0);
        fxExplosion24(pos, 0);
        soundPlay(6, 0, 0, 99);
    }
}
#endif /* TM2_PORT_DT_SWOOP */

#ifdef TM2_PORT_TEAM_LOCKS
/* ---- a team game's own-pick locks pass a teammate (PC port request 123 item 3, port_team_locks.h,
 * docs/PLAYTEST_ANSWERS.md 37.3) ---- */
#include "semantic/port_team_locks.h"
#ifndef PORT_TEXT
#define PORT_TEXT __attribute__((section(".port_text")))
#endif

#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"
extern int humanCarCount(void);
extern int humanCarNearest(int *pos, int skip);

/* Car slots a and b on one team: both teams nonzero and equal (weaponHomingLeapInit, weaponTaserTarget). */
int PORT_TEXT portTeamMates(int a, int b)
{
    if (a < 0 || b < 0 || a >= TM2_MAX_CAR_SLOTS || b >= TM2_MAX_CAR_SLOTS)
        return 0;
    return portCarTeam[a] != 0 && portCarTeam[a] == portCarTeam[b];
}

/* Computer car k's player at aiCarTargets -1 (aiGetTargetCar, portDtSwoopPick): humanCarNearest's pick among the
 * players not of its team (|dx| + |dy| below 0x7FFF, else the first such present player); with none, retail's
 * humanCarNearest (every player its teammate). No rand(). */
int PORT_TEXT portTeamNearestFoe(int k)
{
    struct TM2AIRecord *a = aiCarGet(k);
    struct TM2HumanRecord *r;
    int h, d, n = humanCarCount(), best = 0x7FFF, pick = -1;

    for (h = 0; h < n && h < TM2_MAX_HUMANS; h++) {
        if (TM2_HUMAN_VACANT(h))
            continue;
        r = humanCarGet(h);
        if (portTeamMates(a->slot, r->slot))
            continue;
        if (pick < 0)
            pick = h;
        d = __builtin_abs(a->motion.pos[0] - r->motion.pos[0]) + __builtin_abs(a->motion.pos[1] - r->motion.pos[1]);
        if (d < best) {
            best = d;
            pick = h;
        }
    }
    return pick >= 0 ? pick : humanCarNearest(a->motion.pos, -1);
}
#endif /* TM2_PORT_TEAM_LOCKS */

#ifdef TM2_PORT_HUMAN_WRECK
/* ---- a destroyed player's wreck lives a computer car's wreck's life (PC port request 126, port_human_wreck.h,
 * PLAYTEST_ANSWERS.md 40) ---- */
#include "semantic/port_human_wreck.h"
#ifndef TM2_OWN_CAR_ARRAYS
#define TM2_OWN_CAR_ARRAYS          /* humanCars / aiCars are defined above as plain words */
#endif
#include "semantic/cars.h"      /* the records, humanCarGet (the define alone includes no other block) */
extern struct TM2Object *humanCarObject(int index);
extern void carBodyObjectShow(int key);
extern void carApplyDamage(int id, int amount, int *dir, int type, int attacker);
extern void fxExplosion48(void *pos, unsigned char flag);
extern void fxSmokeA48(void *pos, int flag);

/* humanCarDamageUpdate, at human i's death (after carDestroyed): a car still in play (object drawn-enabled; not
 * broken up, sunk or fallen) gets its body object back (carBodyObjectHide removed it at the death), its small-blast
 * flag cleared (a computer car's is cleared at its start) and its wreck marked. */
void PORT_TEXT portHumanWreckStart(int i, int key)
{
    struct TM2Object *obj;

    if ((unsigned int)i >= TM2_MAX_HUMANS)
        return;
    portHumanWreckLive[i] = 0;
    obj = humanCarObject(i);
    if (obj == 0 || obj->visible == 0)
        return;
    carBodyObjectShow(key);
    humanCarGet(i)->state.f26 = 0;
    portHumanWreckLive[i] = 1;
}

/* The flame carWreckUpdate's computer-car branch put on a player's wreck, out (as carCarControl puts a fire out). */
static void PORT_TEXT portHumanWreckFireOut(struct TM2HumanRecord *h)
{
    if (h->state.burning) {
        h->state.burning = 0;
        if (h->weapons.fire_effect)
            h->weapons.fire_effect->active = 0;
    }
}

/* humanCarDamageUpdate, each step, before its own tests: a marked wreck whose car has HP above 0 (a respawn) or is
 * hidden by anything else (+8 = 0) is unmarked and its flame put out. */
void PORT_TEXT portHumanWreckTick(int i)
{
    struct TM2HumanRecord *h;
    struct TM2Object *obj;

    if ((unsigned int)i >= TM2_MAX_HUMANS || portHumanWreckLive[i] == 0)
        return;
    h = humanCarGet(i);
    obj = humanCarObject(i);
    if (h->stats.hp > 0 || obj == 0 || obj->visible == 0) {
        portHumanWreckLive[i] = 0;
        portHumanWreckFireOut(h);
    }
}

/* carWreckUpdate: 1 while this player's wreck is marked (and its HP is still 0 or less). */
int PORT_TEXT portHumanWreckBurns(void *car)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)car;
    int p = h->player;

    return (unsigned int)p < TM2_MAX_HUMANS && portHumanWreckLive[p] != 0 && h->stats.hp <= 0;
}

/* carWreckUpdate, wreck timer < -1200 (22 s after the death): the computer cars' break-up for a marked player's
 * wreck: carApplyDamage(its object's id, 15, 0, 2, 0) (debris, wreck state 4, +8 = 0), the blast 120 above it and the
 * smoke, the flame out; then unmarked (once). */
void PORT_TEXT portHumanWreckBreak(void *car)
{
    struct TM2HumanRecord *h = (struct TM2HumanRecord *)car;
    struct TM2Object *obj;
    int v[3];

    if ((unsigned int)h->player >= TM2_MAX_HUMANS)
        return;
    portHumanWreckLive[h->player] = 0;
    obj = humanCarObject(h->player);
    if (obj == 0)
        return;
    carApplyDamage(obj->id, 15, 0, 2, 0);
    v[0] = h->motion.pos[0];
    v[1] = h->motion.pos[1];
    v[2] = h->motion.pos[2] + 120;
    fxExplosion48(v, 0);
    v[2] += h->stats.ride_height;
    fxSmokeA48(v, 0);
    portHumanWreckFireOut(h);
}
#endif /* TM2_PORT_HUMAN_WRECK */

#endif /* TM2_PORT_VARIANT */
