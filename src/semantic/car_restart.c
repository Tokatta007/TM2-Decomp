/* Game helpers (800F..8011 range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

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
} TM2Pos12b;

extern TM2Pos12b worldSections[];
extern s32 worldSectionLengths[][35]; /* [0] = 801A99F8, [1] = 801A9A84 (35 route groups) */
extern u8 worldSectionFirstSurface[], worldSectionLastSurface[], worldSectionHasFloor[];
extern TM2Surface worldSurfaces[];

extern unsigned int worldSectionPolyReset(s32);
extern void aiRouteRemoveSectionNodes(s32);
extern s32 humanCarCount(void);
extern u8 worldSectionHasMovingSurfaces(s32);
extern void humanCarPlaceAtStart(struct TM2HumanRecord *human);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-3 (CAR_GROUPS.md 12) */
#endif
#include "semantic/car_numbers.h"   /* request 73: TM2_SPECIAL_NUM (a mod car's flame length) */
extern void weaponSlamStateInit(struct TM2CarWeapons *weapons);
extern void weaponDropsReset(void);
extern void weaponsRoundReset(void);
extern void projectilesReset(void);
extern void fxSpritesReset(void);
extern void viewCameraOffsetsInit(struct TM2HumanRecord *human);
extern void aiClearControls(struct TM2CarInput *input);
extern s32 PadGetConfig(short int player);
extern void carApplyPadConfig(struct TM2HumanCar *car, s32 config);
extern struct TM2Object *humanCarObject(s32);
extern struct TM2Object *aiCarObject(s32);
extern s32 lightGetEnv(s32);
extern void hudMessageReset(void);
extern s16 weaponFlameSlotFind(void);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(s32 *);
extern void soundLocateForView(s32 *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 ratan2(s32, s32);
extern void fxImpactParticlesUpdate(void);

/* 32-byte emitter records @80184774 */
typedef struct {
    s16 timer;   /* +0 */
    s16 age;     /* +2 */
    s32 pos[3];  /* +4 */
    s32 wpos[3]; /* +16 */
    struct TM2Object *model;   /* +28 */
} Emit32;
extern Emit32 weaponFlameEmitters[];

/* 48-byte effect records @80185144 (see fxSpriteSpawn) */
typedef struct {
    u8 pad0[10];
    s8 frame;    /* +10 */
    s8 frames;   /* +11 */
    s16 loops;   /* +12 */
    u8 pad1[2];
    u8 *table;   /* +16 */
    s32 pos[3];  /* +20 */
    s32 vel[3];  /* +32 */
    u8 pad2[4];
} Fx48;
extern Fx48 fxSprites[];
extern u8 fxSmokeFramesA[], fxSmokeFramesB[], fxSmokeFramesC[];
extern s32 fxExplosionCooldown; /* gp+392 pad re-init counter */

/* @800F20C8: reset route state of car id: clear its waypoint records
 * (worldSectionPolyReset), counters, and kick every object bound to it (state 5). */
void __attribute__((section(".text_800F20C8"))) worldSectionDisable(s32 id)
{
    s32 k, i, n;
    struct TM2HumanRecord *human;

    worldSections[id].y = 32000;
    worldSectionLengths[0][id] = 0;
    worldSectionLengths[1][id] = 0;
    for (k = worldSectionFirstSurface[id]; k <= worldSectionLastSurface[id]; k++)
        if (worldSurfaces[k].section == id)
            worldSectionPolyReset(k);
    aiRouteRemoveSectionNodes(id);
    worldSectionHasFloor[id] = 0;
    n = humanCarCount();
    for (i = 0; i < n; i++) {
        human = humanCarGet(i);
        if (human->state.x2C.route == id)
            carSpinOut(human, 1, 5, 0);
    }
}

/* @800F34D4: reset a car's wheel block (its four wheel records). */
void __attribute__((section(".text_800F34D4"))) wheelBlockReset(struct TM2CarWheels *w)
{
    w->surface = -1;
    w->wheel[0].on = 0; w->wheel[0].moving = 0; w->wheel[0].f03 = 0; w->wheel[0].f02 = 0;
    w->wheel[1].on = 0; w->wheel[1].moving = 0; w->wheel[1].f03 = 0; w->wheel[1].f02 = 0;
    w->wheel[2].on = 0; w->wheel[2].moving = 0; w->wheel[2].f03 = 0; w->wheel[2].f02 = 0;
    w->wheel[3].on = 0; w->wheel[3].moving = 0; w->wheel[3].f03 = 0; w->wheel[3].f02 = 0;
    w->wheel[0].h = 0; w->wheel[1].h = 0; w->wheel[2].h = 0; w->wheel[3].h = 0;
    w->wheel[0].f26 = 0; w->wheel[1].f26 = 0; w->wheel[2].f26 = 0; w->wheel[3].f26 = 0;
    w->wheel[0].kind = 0; w->wheel[1].kind = 0; w->wheel[2].kind = 0; w->wheel[3].kind = 0;
    w->wheel[0].pos[2] = 0;
    w->wheel[0].level = 0; w->wheel[0].f1A = 0; w->wheel[0].crush = 0;
    w->wheel[0].sway = 0; w->wheel[0].shake = 0; w->wheel[0].depth = 15;
    w->wheel[1].pos[2] = 0;
    w->wheel[1].level = 0; w->wheel[1].f1A = 0; w->wheel[1].crush = 0;
    w->wheel[1].sway = 0; w->wheel[1].shake = 0; w->wheel[1].depth = 15;
    w->wheel[2].pos[2] = 0;
    w->wheel[2].level = 0; w->wheel[2].f1A = 0; w->wheel[2].crush = 0;
    w->wheel[2].sway = 0; w->wheel[2].shake = 0; w->wheel[2].depth = 15;
    w->wheel[3].pos[2] = 0;
    w->wheel[3].level = 0; w->wheel[3].f1A = 0; w->wheel[3].crush = 0;
    w->wheel[3].sway = 0; w->wheel[3].shake = 0; w->wheel[3].depth = 15;
    w->wheel[0].surface = 0; w->wheel[0].touch = 0; w->wheel[0].f2C = 0;
    w->wheel[0].f12 = 0; w->wheel[0].fall = 0; w->wheel[0].pos[2] = 0;
    w->wheel[1].surface = 0; w->wheel[1].touch = 0; w->wheel[1].f2C = 0;
    w->wheel[1].f12 = 0; w->wheel[1].fall = 0; w->wheel[1].pos[2] = 0;
    w->wheel[2].surface = 0; w->wheel[2].touch = 0; w->wheel[2].f2C = 0;
    w->wheel[2].f12 = 0; w->wheel[2].fall = 0; w->wheel[2].pos[2] = 0;
    w->wheel[3].surface = 0; w->wheel[3].touch = 0; w->wheel[3].f2C = 0;
    w->wheel[3].f12 = 0; w->wheel[3].fall = 0; w->wheel[3].pos[2] = 0;
}

static __inline__ u8 in_zone_800F6DDC(struct TM2AIRecord *ai)
{
    u8 r = 0;

    if (ai->motion.pos[1] >= -8799 && ai->motion.pos[1] < -5600
        && ((ai->motion.pos[0] >= 4321 && ai->motion.pos[1] < 5280)
            || (ai->motion.pos[0] >= -5279 && ai->motion.pos[1] < -4320)))
        r = 1;
    return r;
}

/* @800F6DDC: on route nodes 2/40/11/44, inside two map zones, set drive flag
 * f45D (target's route group >= 8 passing worldSectionHasMovingSurfaces), otherwise f45C. */
void __attribute__((section(".text_800F6DDC"))) aiDarkToothJumpSelect(struct TM2AIRecord *ai)
{
    u8 tmp[8];
    void *o;                /* target: human (tmp[0]) or AI record */
    s32 id;

    DRV(ai)->f45C = 0;
    DRV(ai)->f45D = 0;
#ifdef TM2_PORT_DARK_TOOTH
    /* request 33-2e, portCarOptions bit 6: Hong Kong's jump zones only on level 8 (SURVIVAL.md 1.5 #3); request 42
       item 1: never for an ordinary Dark Tooth (bit 10) */
    if ((TM2_PORT_BOSS_ANYWHERE && gameGetLevel() != 8) || TM2_DT_ORDINARY(ai->slot))
        return;
#endif
    if (DRV(ai)->node == 2 || DRV(ai)->node == 40
        || DRV(ai)->node == 11 || DRV(ai)->node == 44) {
        if (in_zone_800F6DDC(ai)) {
            o = aiGetTargetCar(ai->index, tmp);
            if (tmp[0])
                id = HREC(o)->state.x2C.route;
            else
                id = DRV(o)->route;
            if (worldSectionHasMovingSurfaces(id) && id >= 8)
                DRV(ai)->f45D = 1;
            if (DRV(ai)->f45D == 0)
                DRV(ai)->f45C = 1;
        }
    }
}

/* @800FD550: (re)start a car: mode, transmission/AI init, effect resets,
 * pad config, lighting env for its player record. */
void __attribute__((section(".text_800FD550"))) carRestart(struct TM2HumanRecord *human, s32 type, u8 reset_weapons)
{
    struct TM2Object *obj;

    human->type = type;
    human->stats.f00 = 1;
    humanCarPlaceAtStart(human);
    if (reset_weapons) {
        weaponBlockInit(human, &human->weapons, 1);
    } else {
        human->weapons.ammo[0] = 2;
        human->weapons.energy = 1000;
        human->weapons.drop_armed = 0;
        if (human->type == 192)
            weaponSlamStateInit(&human->weapons);
    }
    weaponDropsReset();
    weaponsRoundReset();
    projectilesReset();
    fxSpritesReset();
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
    carApplyPadConfig((struct TM2HumanCar *)human, PadGetConfig(human->player));
    human->stats.cheat_a = 0;
    human->stats.cheat_b = 0;
    human->stats.cheat_released = 0;
    human->stats.f0C = 0;
    human->stats.heal_spot_used = 0;
    human->contact.f01 = 1;
    obj = humanCarObject(human->player);
    if (obj) {
        obj->light = lightGetEnv(0);
        obj->visible = 1;
    }
    hudMessageReset();
}

/* @800FDFC4: build the 6-vertex box shape of the car model from its
 * stats block dimensions. car is a human record when human is set, else an
 * AI record. */
void __attribute__((section(".text_800FDFC4"))) carBuildCollisionBox(void *car, u8 human)
{
    struct TM2Object *t;
    struct TM2CarStatsBlock *d;
    s16 i;

    if (human) {
        i = HREC(car)->player;
        d = &HREC(car)->stats;
        t = humanCarObject(i);
    } else {
        i = AI(car)->index;
        d = &AI(car)->stats;
        t = aiCarObject(i);
    }
    t->box_count = 6;
    t->box[0][0] = -d->track_width / 2;
    t->box[0][1] = d->axle_front;
    t->box[0][2] = 24 - d->ride_height;
    t->box[1][0] = d->track_width / 2;
    t->box[1][1] = d->axle_front;
    t->box[1][2] = 24 - d->ride_height;
    t->box[2][0] = d->track_width / 2;
    t->box[2][1] = d->axle_rear;
    t->box[2][2] = 24 - d->ride_height;
    t->box[3][0] = -d->track_width / 2;
    t->box[3][1] = d->axle_rear;
    t->box[3][2] = 24 - d->ride_height;
    t->box[4][0] = 0;
    t->box[4][1] = d->axle_front;
    t->box[4][2] = 24 - d->ride_height;
    t->box[5][0] = 0;
    t->box[5][1] = d->axle_front;
    t->box[5][2] = d->roof_height;
}

/* @8010A5F0: allocate an emitter slot for the car's model at pos (+model
 * offset), play sound 47; returns slot+1 or 0. car is a human record when
 * human is set, else an AI record. */
s16 __attribute__((section(".text_8010A5F0"))) weaponFlameStart(void *car, u8 human, s32 *pos)
{
    struct TM2Object *t;
    s16 n;
    s32 *p, *q;
    s32 m;

    if (human)
        t = humanCarObject(HREC(car)->player);
    else
        t = aiCarObject(AI(car)->index);
    n = weaponFlameSlotFind();
    if (n < 0)
        return 0;
    weaponFlameEmitters[n].model = t;
    p = weaponFlameEmitters[n].pos;
    p[0] = pos[0];
    p[1] = pos[1];
    p[2] = pos[2];
    weaponFlameEmitters[n].timer = TM2_SPECIAL_NUM(human ? HREC(car)->slot : AI(car)->slot, TM2_SPN_LIFE, 50);
    q = weaponFlameEmitters[n].wpos;
    q[0] = t->pos[0] + pos[0];
    q[1] = t->pos[1] + pos[1];
    q[2] = t->pos[2] + pos[2];
    if (TM2_CAR_FEEDBACK(human, HREC(car)->player)) {   /* H3: a remote human's positional, as the AI's */
#ifdef TM2_PORT_CAR_SOUND
        portCarFlameSound(HREC(car)->slot, 0, 0);     /* request 33-3: the car's own instance (bit 4) */
#else
        soundPlay(47, 0, 0, 99);
#endif
    } else {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(q);
        else
            soundLocateForView(q, -1);
        m = PadChkMtap();
#if defined(TM2_PORT_CAR_SOUND) && defined(TM2_PORT_HUMANS)
        portCarFlameSound(human ? HREC(car)->slot : AI(car)->slot, m, soundGetListenerPan());
#elif defined(TM2_PORT_CAR_SOUND)
        portCarFlameSound(AI(car)->slot, m, soundGetListenerPan());
#else
        soundPlay(47, m, soundGetListenerPan(), 99);
#endif
    }
    weaponFlameEmitters[n].age = 0;
    return n + 1;
}

/* @8010DB98: reflect vector v about normal n (n/41, products /10000) and
 * store the reflected heading ratan2(x, z) to *out. */
void __attribute__((section(".text_8010DB98"))) mathReflectHeading(s32 *v, s32 *n, s16 *out)
{
    s32 a[3];
    s32 r[3];
    s32 m[3];
    s32 d, z;

    m[0] = n[0] / 41;
    m[1] = n[1] / 41;
    m[2] = n[2] / 41;
    d = v[0] * m[0] + v[1] * m[1] + v[2] * m[2];
    r[0] = d * m[0] / 10000;
    r[1] = d * m[1] / 10000;
    r[2] = d * m[2] / 10000;
    a[0] = v[0] - r[0] * 2;
    a[1] = v[1] - r[1] * 2;
    a[2] = v[2] - r[2] * 2;
    z = a[1];
    if (z == 0)
        z = 1;
    *out = ratan2(a[0], z);
}

/* @80110A40: tick all 150 (TM2_MAX_FX_SPRITES) effect records: advance frame, move by vel; on
 * the last frame loop or expire, and slow the final loop of three tables. */
void __attribute__((section(".text_80110A40"))) fxSpritesUpdate(void)
{
    s32 i;
    Fx48 *r;

    fxExplosionCooldown++;
    fxImpactParticlesUpdate();
    for (i = 0; i < TM2_MAX_FX_SPRITES; i++) {
        r = &fxSprites[i];
        if (r->frame >= 0) {
            r->frame++;
            r->pos[0] += r->vel[0];
            r->pos[1] += r->vel[1];
            r->pos[2] += r->vel[2];
            if (r->frame >= r->frames) {
                if (r->loops-- > 0)
                    r->frame = 0;
                else
                    r->frame = -1;
                if (r->loops == 1
                    && (r->table == fxSmokeFramesA || r->table == fxSmokeFramesB || r->table == fxSmokeFramesC))
                    r->frames += 16;
            }
        }
    }
}
