/* Small game helpers (800F..800F range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/port_cars.h"   /* request 49: portStartPlacedMark */
#include "semantic/car_numbers.h"   /* request 70: portCarProfileStep, portCarProfileCamera */
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 118: TM2_DT_PLAYER_PHASE2 */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

/* Route point (24 bytes at aiRouteNodes): position, up to 6 successor nodes,
 * successor count, route group (track section). */
typedef struct {
    s32 x, y;
    s16 z;
    s16 next[6];            /* +10 */
    s8 count;               /* +22 */
    s8 route;               /* +23 */
} TM2RouteNode;

/* Ground surface (28 bytes at worldSurfaces, see car_wheels.c TM2Surface). */
typedef struct {
    u8 type;                /* +0x00 */
    u8 f01;                 /* +0x01 */
    s16 section;            /* +0x02 track section the surface belongs to */
    s16 w, l;               /* +0x04 */
    s16 sn, cs;             /* +0x08 */
    s16 h[4];               /* +0x0C corner heights */
    s32 x, y;               /* +0x14 origin */
} TM2Surface;
typedef char tm2_surface_size[(sizeof(TM2Surface) == 28) ? 1 : -1];

typedef struct {
    s32 x, z, y;
} TM2Pos12;

extern s32 aiRouteNodeCount;
#pragma tm2_absolute aiRouteNodeCount
extern s8 startNodeCount;
#pragma tm2_absolute startNodeCount
extern TM2RouteNode aiRouteNodes[];
extern TM2Surface worldSurfaces[];
extern u8 worldSectionFirstSurface[], worldSectionLastSurface[];
extern TM2Pos12 worldSections[];
extern u8 worldSectionRotated[], worldSectionShapes[];
extern s32 worldSectionLengths[], worldSectionWidths[];

extern int rand(void);
extern void *memcpy(void *, const void *, u32);
#include "semantic/world.h"
extern void fxSparkOrange(s32 *pos, u8 arg1);
extern void startNodesRemove(s32);
extern s32 aiCarCount(void);
extern struct TM2Object *carObjectFromOwner(s32);
extern u8 carFindByObject(struct TM2Object *object, u8 *human, s32 *index);
extern void aiStartRouteFollow(struct TM2AIRecord *ai);
extern void aiChooseNextNode(struct TM2AIRecord *ai, s32 node);
extern void aiUpdateStuck(struct TM2AIRecord *ai);
extern void aiCarReset(struct TM2AIRecord *ai);
#include "semantic/weapon_calls.h"
extern void aiSetSegmentSpeed(struct TM2AIRecord *ai, unsigned char mode);
extern void weaponSlamStateInit(struct TM2CarWeapons *weapons);
extern void aiRangeFlagsClear(u8 *flags);
extern void aiRouteStateReset(struct TM2AIDrive *drive);
extern s32 startNodePickRandom(s32);
extern void aiPlaceOnRouteNode(struct TM2AIRecord *ai);
#ifdef TM2_PORT_CARS
/* request 85 (SIXTEEN_CARS.md 5.5): a start along a route link, as aiPlaceOnRouteNode puts a car on a node */
extern struct { s32 base; s32 a, b; } worldSectionFloorHeights[];
extern void wheelBlockReset(struct TM2CarWheels *wheels);
extern void wheelBlockSetSection(struct TM2CarWheels *wheels, s32 section);
extern void wheelBlockSetHeight(struct TM2CarWheels *wheels, s32 height);
extern void carBodyAttitudeUpdate(void *car, s32 human);
#endif
extern void aiClearControls(struct TM2CarInput *input);

/* @800F0340: AI car distance to the nearer edge of its track section
 * (wrap-around x/z extents), 8000 when the section does not wrap. */
s32 __attribute__((section(".text_800F0340"))) aiSectionEdgeDistance(struct TM2AIRecord *ai)
{
    s32 i = DRV(ai)->route;
    s32 r = 8000;
    s32 dx = ai->motion.pos[0] - worldSections[i].x;
    s32 dz = ai->motion.pos[1] - worldSections[i].z;
    s32 *p, *w;

    if (worldSectionRotated[i] == 0 && worldSectionShapes[i] < 2) {
        w = worldSectionLengths; p = w + i;
        if (dx < *p / 2)
            r = dx;
        else
            r = *p - dx;
        w = worldSectionWidths; p = w + i;
        if (dz < *p / 2) {
            if (dz < r)
                r = dz;
        } else {
            /* retail quirk: dead re-read of *p, and it tests dz (not
             * *p - dz) against r before taking the wrapped distance */
            s32 t = *p;
            if (dz < r)
                r = *p - dz;
        }
    }
    return r;
}

/* @800F1FA8: timed effect emitter; while armed waits for its 60-frame
 * delay, otherwise spawns fxSparkOrange at pos+16 when due / on request. */
void __attribute__((section(".text_800F1FA8"))) wheelEffectEmit(struct TM2CarWheel *w, s32 t)
{
    s32 pos[3];

    if (w->touch) {
        if (w->f12 > 60)
            w->f02 = 1;
    } else {

        if (t < (w->pos[2] - w->f1C) / (s16)timerGetFrameTicks()) {
            memcpy(pos, w->pos, 12);
            pos[2] += 16;
            fxSparkOrange(pos, 0);
        }
        if (w->f02) {
            memcpy(pos, w->pos, 12);
            pos[2] += 16;
            fxSparkOrange(pos, 0);
            w->f02 = 0;
        }
    }
}

/* @800F2308: remove node id from every node's successor list, then startNodesRemove(id). */
void __attribute__((section(".text_800F2308"))) aiRouteNodeRemove(s32 id)
{
    s32 i, j, k;

    for (i = 0; i < aiRouteNodeCount; i++)
        for (j = 0; j < aiRouteNodes[i].count; j++)
            if (aiRouteNodes[i].next[j] == id) {
                for (k = j; k < aiRouteNodes[i].count; k++)
                    aiRouteNodes[i].next[k] = aiRouteNodes[i].next[k + 1];
                aiRouteNodes[i].count -= 1;
            }
    startNodesRemove(id);
}

/* @800F2420: reset every AI car bound to route group id (its own group,
 * or either of its two current groups owned by id). */
void __attribute__((section(".text_800F2420"))) aiCarsRerouteFromSection(s32 id, s32 arg)
{
    s32 n = aiCarCount();
    s32 i;
    struct TM2AIRecord *ai;

    for (i = 0; i < n; i++) {
        ai = aiCarGet(i);
        if (DRV(ai)->route == id
            || aiRouteNodes[DRV(ai)->node].route == id
            || aiRouteNodes[DRV(ai)->node_next].route == id) {
            aiStartRouteFollow(ai);
            aiChooseNextNode(ai, arg);
            aiUpdateStuck(ai);
            aiSetSegmentSpeed(ai, 0);
        }
    }
}

/* @800F25F4: add value to the four corner fields of every record of id
 * (additive twin of worldSectionSetHeight). */
void __attribute__((section(".text_800F25F4"))) worldSectionAddHeight(s32 id, s32 value)
{
    s32 k;

    for (k = worldSectionFirstSurface[id]; k <= worldSectionLastSurface[id]; k++)
        if (worldSurfaces[k].section == id) {
            worldSurfaces[k].h[0] += value;
            worldSurfaces[k].h[1] += value;
            worldSurfaces[k].h[2] += value;
            worldSurfaces[k].h[3] += value;
        }
}

/* @800F4A58: for the car owning object idx: optional spin-out of kind
 * spin (also advancing the four wheel angles), optional carScrapeSparks. */
void __attribute__((section(".text_800F4A58"))) carSpinOutByObject(s32 idx, s32 spin, s32 other)
{
    struct TM2Object *o;
    u8 human;
    s32 index;
    void *car;              /* human or AI record */
    struct TM2CarWheels *w;

    o = carObjectFromOwner(idx);
    if (o == 0)
        return;
    if (!carFindByObject(o, &human, &index))
        return;
    if (human) {
        car = humanCarGet(index);
        w = &HREC(car)->wheels;
    } else {
        car = aiCarGet(index);
        w = &AI(car)->wheels;
    }
    if (spin) {
        carSpinOut(car, human, spin, 0);
        w->wheel[0].level += w->wheel[0].f1A;
        w->wheel[1].level += w->wheel[1].f1A;
        w->wheel[2].level += w->wheel[2].f1A;
        w->wheel[3].level += w->wheel[3].f1A;
    }
    if (other)
        carScrapeSparks(car, human);
}

/* @800FE4F0: per-vehicle-type value table (type codes 0x10..0xF0). */
s32 __attribute__((section(".text_800FE4F0"))) carDefaultTopSpeed(s32 type)
{
    switch (type) {
    case 0x10: return 0x5C;
    case 0x20: return 0x60;
    case 0x30: return 0x7F;
    case 0x40: return 0x65;
    case 0x50: return 0x7D;
    case 0x60: return 0x79;
    case 0x70: return 0x6F;
    case 0x80: return 0x83;
    case 0x90: return 0x77;
    case 0xA0: return 0x8D;
    case 0xB0: return 0x67;
    case 0xC0: return 0x5C;
    case 0xD0: return 0x71;
    case 0xE0: return 0x7F;
    case 0xF0: return 0x7F;
    }
    return 0x77;
}

/* @800FF638: set the highest climbable step from the car type (default 0x30). */
void __attribute__((section(".text_800FF638"))) carWheelsSetStepHeight(struct TM2CarWheels *w, s32 type)
{
    switch (type) {
    case 0x10: w->step = 0x40; break;
    case 0x20: w->step = 0x50; break;
    case 0x30: w->step = 0x30; break;
    case 0x40: w->step = 0x40; break;
    case 0x50: w->step = 0x28; break;
    case 0x60: w->step = 0x30; break;
    case 0x70: w->step = 0x30; break;
    case 0x80: w->step = 0x30; break;
    case 0x90: w->step = 0x38; break;
    case 0xA0: w->step = 0x30; break;
    case 0xB0: w->step = 0x40; break;
    case 0xC0: w->step = 0x38; break;
    case 0xD0: w->step = 0x30; break;
    case 0xE0: w->step = 0x58; break;
    case 0xF0: w->step = 0xA0; break;
    default: w->step = 0x30; break;
    }
#ifdef TM2_PORT_CAR_PROFILE
    portCarProfileStep(w);      /* request 70 (CAR_PROFILES.md): a mod car's own step */
#endif
}

/* @800FF764: default chase-camera offsets of a human car for its car type. */
void __attribute__((section(".text_800FF764"))) viewCameraOffsetsInit(struct TM2HumanRecord *human)
{
    human->cam_look_offset[0] = 0;
    human->cam_look_offset[1] = 40;
    human->cam_look_offset[2] = 0;
    human->cam_chase[0] = 0;
    human->cam_chase[1] = -280;
    human->cam_chase[2] = 112;
    human->cam_offset3[0] = 0;
    human->cam_offset3[1] = -480;
    human->cam_offset3[2] = 168;
    human->cam_offset[0] = 0;
    human->cam_offset[1] = -1200;
    human->cam_offset[2] = 800;
    human->cam_split1[0] = 0;
    human->cam_split1[1] = -176;
    human->cam_split1[2] = 80;
    if (human->type == 0xB0 || human->type == 0xE0) {
        human->cam_split1[1] = -192;
        human->cam_split1[2] = 160;
    }
    human->cam_split2[0] = 0;
    human->cam_split2[1] = -240;
    human->cam_split2[2] = 80;
    if (human->type == 0x60 || human->type == 0x20 || human->type == 0xC0) {
        human->cam_split2[1] = -224;
        human->cam_split2[2] = 72;
    } else if (human->type == 0xB0 || human->type == 0xE0) {
        human->cam_split2[1] = -400;
        human->cam_split2[2] = 120;
    }
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth: the cameras back and up for his truck (about twice
       a car) and the head 240 above it; cam_offset (the other full-screen views) and the look-at point stay.
       Request 118: in his second phase (the head on a small car) a car's offsets, as above */
    if (human->type == 0xF0 && !TM2_DT_PLAYER_PHASE2(human->player)) {
        human->cam_chase[1] = -830;
        human->cam_chase[2] = 362;
        human->cam_offset3[1] = -830;
        human->cam_offset3[2] = 618;
        human->cam_split1[1] = -450;
        human->cam_split1[2] = 592;
        human->cam_split2[1] = -1518;
        human->cam_split2[2] = 670;
    }
#endif
#ifdef TM2_PORT_CAR_PROFILE
    portCarProfileCamera(human);    /* request 70: a mod car's own camera offsets */
#endif
}

/* @800FFEFC: (re)start an AI driver of vehicle type `type`. */
void __attribute__((section(".text_800FFEFC"))) aiCarStart(struct TM2AIRecord *ai, s32 type, u8 reset_weapons)
{
    ai->type = type;
    ai->stats.f00 = 0;
    aiCarReset(ai);
    if (reset_weapons) {
        weaponBlockInit(ai, &ai->weapons, 0);
    } else {
        ai->weapons.ammo[0] = 2;
        if (ai->type == 0xC0)
            weaponSlamStateInit(&ai->weapons);
    }
    ai->weapons.gun_cooldown = 50;
    ai->weapons.fire_cooldown = 50;
    ai->f001 = rand() % 3;
    ai->f012 = (ai->index + 1) * 60;
    ai->f00C = 0;
    aiRangeFlagsClear(&ai->active);
    aiRouteStateReset(DRV(ai));
#ifdef TM2_PORT_CARS
    portStartPlacedMark(ai, 0, 0);  /* request 49 (CAR_GROUPS.md 7.4): not counted by its own start pick */
#endif
    ai->stats.spawn_node = startNodePickRandom(startNodeCount);
#ifdef TM2_PORT_CARS
    portStartPlacedMark(ai, 0, 1);  /* request 49: placed this round */
#endif
    aiPlaceOnRouteNode(ai);
#ifdef TM2_PORT_CARS
    {
        /* request 85: the pick left a link start: the car there, on the ground, facing along the link, its next
           node the link's second (the first is its node, the pick's) */
        s32 h_, z_, to_, k_;

        if (portStartLinkPlace(ai->motion.pos, &h_, &z_, &to_)) {
            k_ = worldSectionAt(ai->motion.pos[0], ai->motion.pos[1], z_);
            ai->motion.pos[2] = worldSectionFloorHeights[k_].base;
            ai->motion.rot[2] = h_;
            DRV(ai)->heading = h_;
            ai->stats.target_node = to_;
            wheelBlockReset(&ai->wheels);
            wheelBlockSetSection(&ai->wheels, k_);
            wheelBlockSetHeight(&ai->wheels, z_);
            carBodyAttitudeUpdate(ai, 0);
        }
    }
#endif
    aiClearControls(&ai->input);
    carShakeStart(ai, 2, 2, 0, 0);
    ai->f000 = 1;
}
