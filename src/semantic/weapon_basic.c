#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-4: Twister's tornado sound per car */
#endif
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 36: a player's Dark Tooth */
#endif
#include "semantic/car_numbers.h"   /* request 73: TM2_SPECIAL_NUM (a mod car's special numbers) */
#include "semantic/spu_voices.h"   /* request 87: the special's sounds carry the firer (TM2_VOICE_TAG) */

extern struct TM2Object *humanCarObject(int);
extern struct TM2Object *aiCarObject(int);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlay(int, int, int, int);
extern int soundPlayVariantEx(int, int, int, int, int, int);
extern void mathMulTransVec(void *, int *, int *);
extern int rsin(int);
extern int rcos(int);
extern void weaponFindTargetBehind(int, int *, s32 mode);
extern void weaponFindTargetAhead(int, int *, s32 mode);
extern int weaponFireSpiralMissile(int, int, int *, int *, int);
extern int weaponTaserTarget(void *, int, int);
extern s16 weaponFireColoredMissile(int, int, int *, int *, int, void *);
extern int weaponFireGrimmSpecial(int, int, int *, int *, int);
extern void weaponHomingLeapInit(void *, int);
extern s16 weaponFlameStart(void *, u8 human, int *);
extern int weaponFireGhostMissile(int, int, int *, int *, int);
extern int weaponFireRoadkillBoomerang(int, int, int *, int *, int, void *);
extern void *memcpy(void *, const void *, int);
extern void objRegistrySetStep(int, int, int);
extern int fxSmokePuffBlue(void *, int);
extern int carObjectCount(void);
extern int carPairDistance(int, int);
extern void fxExplosion48(void *, u8 flag);
extern void carApplyDamage(int, int, int, int, int);
struct TM2NearRecord { short a, b, active, pad; int pos[3]; };
extern struct TM2NearRecord *targetPoolGet(void);
extern int targetDamage(int, int, int);
struct TM2ObjectRecord { unsigned char pad0[4]; short state; unsigned char pad6[50]; struct TM2Object *actor; };
extern struct TM2ObjectRecord *targetActorsGet(void);
extern void targetActorHit(int, int *);
extern void weaponSlamStateInit(void *);
extern void fxExplosion96(void *, u8 flag);
extern void areaDamage(int *, short int strength, short int big);
extern int weaponFireFreezeMissile(int, int, int *, int *, int);

/* Shared weapon state written by other (generated) units as well; imported. */
struct TM2ShieldState { short timer; short f2; short kind; short f6; int *anchor; struct TM2CarState *flags; struct TM2Object *object; };
struct TM2StrikeState { signed char mode; char f1; unsigned short owner_id; short active; short timer; int velocity[3], delta[3], previous[3]; };
extern struct TM2ShieldState weaponTornadoState;      /* @80184DC0 */
/* @80184E10: strike state, corner offsets, owner weapon block (+0x5C) and
 * strike object (+0x60), addressed as one block. */
struct TM2StrikeBlock { struct TM2StrikeState s; int corners[12]; struct TM2CarWeapons *weapon; struct TM2Object *object; };
extern struct TM2StrikeBlock weaponStrikeState;
extern struct TM2Object *weaponTornadoObject;          /* @80180F38 (asset_pools.c) */
extern struct TM2Object *weaponTornadoFunnelObject;          /* @80180F3C (asset_pools.c) */
/* Five 8-byte colour constants and one rest vector referenced only by this function; owned with their
 * retail-identical initializers (checked byte-for-byte by the audit link and the probe builder). */
unsigned char weaponSpecialColorRed[8] __attribute__((section(".tm2_80180B10"))) = {0xff, 0, 0, 0, 0, 0, 0, 0};
unsigned char weaponSpecialColorWhite[8] __attribute__((section(".tm2_80180B18"))) = {0xff, 0, 0xff, 0, 0xff, 0, 0, 0};
unsigned char weaponSpecialColorBlue[8] __attribute__((section(".tm2_80180B20"))) = {0, 0, 0, 0, 0xff, 0, 0, 0};
unsigned char weaponSpecialColorCyan[8] __attribute__((section(".tm2_80180B28"))) = {0, 0, 0xff, 0, 0xff, 0, 0, 0};
unsigned char weaponSpecialColorGreen[8] __attribute__((section(".tm2_80180B30"))) = {0, 0, 0xff, 0, 0, 0, 0, 0};
struct TM2Triple { int v[3]; };
struct TM2Triple weaponHitSpinDefault __attribute__((section(".tm2_800CF0FC"))) = {{0, 0, 4095}};

#define U8(p,n) (*(unsigned char *)((unsigned char *)(p)+(n)))
#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)
#define HUMAN_OF(p) ((struct TM2HumanRecord *)(p))
#define AI_OF(p) ((struct TM2AIRecord *)(p))

#define MODE (human & 255)
#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)
/* AI fires run the controller pad start/stop on the target, then PadChkMtap
 * and soundGetListenerPan feed the sound call; player fires play it directly. */
#define AI_SOUND(code, target) { \
        if (viewGetSplitMode() == 0) \
            soundLocateForView0(target); \
        else \
            soundLocateForView(target, -1); \
        mtap = PadChkMtap(); \
        soundPlay(code, mtap, soundGetListenerPan(), 99); \
    }
#define SOUND(code, target) \
    if (TM2_CAR_FEEDBACK(MODE, HUMAN->player)) /* H3: a remote human's as the AI's */ \
        soundPlay(code, 0, 0, 99); \
    else AI_SOUND(code, target)
/* Launch angles: the rear-fire byte weapon+5 (cleared) selects weaponFindTargetBehind and
 * the yaw turned by 0x800 with a zero pitch, otherwise weaponFindTargetAhead and the
 * motion angles. */
#define AIM \
    if (weap->rear_fire != 0) { \
        weap->rear_fire = 0; \
        weaponFindTargetBehind(owner, &effect, MODE); \
        ang[0] = motion->rot[0]; ang[1] = 0; ang[2] = (motion->rot[2] + 0x800) & 0xfff; \
    } else { \
        weaponFindTargetAhead(owner, &effect, MODE); \
        ang[0] = motion->rot[0]; ang[1] = motion->rot[1]; ang[2] = motion->rot[2]; \
    }

/* @80105A7C. Basic-weapon fire for weapon kind 0, called by weaponFireSelected with the actor and the layout
 * mode (low byte of `human`, as in the tick). Returns without effect while the kind-0 slot count
 * (+0xE) is not positive. Otherwise the controller pad start/stop chosen by viewGetSplitMode runs on the
 * motion position, the muzzle position (geometry offsets transformed by the motion matrix, +24 high)
 * and the facing direction (rsin/rcos of the motion angles) are prepared, and the vehicle word (+0x90 /
 * +0x30) selects the vehicle-specific fire: 0x10 rocket, 0x30 helper, 0x40 triple emitter, 0x50 wide
 * shot, 0x60 spawn, 0x70 direction helper, 0x80/0x90 emitters, 0xA0 shield (shared state weaponTornadoState),
 * 0xB0 area strike over every other actor plus the two object lists, 0xC0 pooled object arm, 0xD0
 * strike toggle (weaponStrikeState / weaponStrikeObject), 0xE0 triple flash. Player fires play the sound directly;
 * AI fires go through the pad start/stop, PadChkMtap and soundGetListenerPan. Every vehicle path but 0x20
 * (which only bumps the slot count) ends by setting flags byte +9. The caller ignores $v0, so the
 * function is void (M2C's u16 return is residue). */
void __attribute__((section(".text_80105A7C")))
weaponFireSpecial(void *car, unsigned int human)
{
    short id;
    struct TM2Object *lookup, *olookup;
    struct TM2CarWeapons *weap;
    struct TM2CarState *flags, *oflags;
    struct TM2CarMotion *motion;
    struct TM2CarStatsBlock *geom;
    struct TM2CarSlam *obj;
    void *other;            /* human (is_main) or AI record */
    int *omotion;
    struct TM2ObjectRecord *objects;
    struct TM2NearRecord *near;
    int pos[3], in[3], dir[3], ang[3];
    int effect;
    unsigned char is_main;
    int vehicle, owner, otherid, slot, r, i, n, oid, mtap;
#ifdef TM2_PORT_CAR_SPECIALS
    int special;
#endif

    if (MODE) {
        id = TM2_HUMAN_ID(HUMAN->player);
        lookup = humanCarObject(HUMAN->player);
        vehicle = HUMAN->type; owner = HUMAN->player; otherid = HUMAN->slot;
        geom = &HUMAN->stats; motion = &HUMAN->motion; weap = &HUMAN->weapons; flags = &HUMAN->state;
    } else {
        id = AI->index + 50;
        lookup = aiCarObject(AI->index);
        vehicle = AI->type; owner = AI->index; otherid = AI->slot;
        geom = &AI->stats; motion = &AI->motion; weap = &AI->weapons; flags = &AI->state;
    }
    if (weap->ammo[0] <= 0)
        return;
    if (viewGetSplitMode() == 0)
        soundLocateForView0(motion->pos);
    else
        soundLocateForView(motion->pos, -1);
    in[0] = 0; in[1] = geom->gun[1]; in[2] = geom->gun[2];
    mathMulTransVec(&motion->rot_m, in, pos);
    pos[0] += motion->pos[0];
    pos[1] += motion->pos[1];
    pos[2] += motion->pos[2] + 24;
    in[0] = 0; in[1] = -100; in[2] = 0;
    mathMulTransVec(&motion->rot_m, in, dir);
    dir[0] = rsin(motion->rot[2]) >> 6;
    dir[1] = rcos(motion->rot[2]) >> 6;
    dir[2] = -rsin(motion->rot[0]) >> 6;
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): the firer's own special state (its car key; the first car of a type:
       retail's) for the type switch: a Shadow's second press detonates its own strike, and a second
       Twister, Shadow or Grasshopper fires while another one's special runs */
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35 (GUN_GAME_SPECIALS.md "As built"): a car with a borrowed special (portCarSpecialAs) fires that type's
       special, on its own as-entry for Shadow's strike and Axel's shockwave; else its own as above */
    special = portCarSpecialSelectFire(otherid, &vehicle);
#else
    special = portCarSpecialSelect(TM2_CAR_KEY(otherid, vehicle));
#endif
#endif
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialFireBegin();     /* request 73 (SPECIAL_TEMPLATES.md 6): the records free before the fire */
#endif
    /* request 87 (SPU_VOICES.md 11): the sounds this fire plays are the firer's special's (portVoiceOwner keeps the tag
       only for a special's sound id: not a hit's or an explosion's) */
    TM2_VOICE_TAG(otherid, TM2_VOICE_SPECIAL);
    switch (vehicle) {
    case 0x10:
        AIM
        pos[2] += 40;
        slot = (short)weaponFireSpiralMissile(lookup->id, effect, ang, pos, 15);
        if ((MODE || AI->active != 0) && slot >= 0) {
            SOUND(0x3d, pos)
        }
        break;
    case 0x20:
        weap->ammo[0]++;
#ifdef TM2_PORT_CAR_SPECIALS
        portCarSpecialRestore(special);
#endif
        TM2_VOICE_UNTAG();
        return;
    case 0x30:
        if (weaponTaserTarget(car, MODE, id) != 0) {
            SOUND(0x27, motion->pos)
            if (MODE == 0 && AI->f001 == 1)
                DRV(car)->f458 = 1;
        }
        break;
    case 0x40:
        AIM
        in[0] = geom->mount_x; in[1] = 0; in[2] = geom->launch_height + 8;
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireColoredMissile(id, effect, ang, pos, 6, weaponSpecialColorRed);
        if ((short)r >= 0)
            in[0] = geom->gun[0];
        in[1] = geom->gun[1]; in[2] = geom->gun[2];
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireColoredMissile(id, effect, ang, pos, 6, weaponSpecialColorWhite);
        if ((short)r >= 0)
            in[0] = -geom->gun[0];
        in[1] = geom->gun[1]; in[2] = geom->gun[2];
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireColoredMissile(id, effect, ang, pos, 6, weaponSpecialColorBlue);
        if ((short)r >= 0)
            SOUND(0x29, motion->pos)
        break;
    case 0x50:
        AIM
        in[0] = 0; in[1] = 0xa0; in[2] = geom->launch_height + 8;
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireGrimmSpecial(id, effect, ang, pos, 40);
        if ((short)r >= 0)
            SOUND(0x2b, motion->pos)
        break;
    case 0x60:
        carSpinOut(car, MODE, 7, 1);
        weaponHomingLeapInit(car, MODE);
        SOUND(0x2d, motion->pos)
        break;
    case 0x70:
        weaponFlameStart(car, MODE, dir);
        break;
    case 0x80:
        AIM
        in[0] = geom->mount_x; in[1] = 0; in[2] = geom->launch_height + 8;
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireGhostMissile(id, effect, ang, pos, 12);
        if ((short)r >= 0)
            SOUND(0x31, motion->pos)
        break;
    case 0x90:
        AIM
        in[0] = geom->mount_x; in[1] = 0; in[2] = geom->launch_height + 8;
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        slot = (short)weaponFireRoadkillBoomerang(id, effect, ang, pos, 10, lookup);
        if (slot >= 0) {
            if (TM2_CAR_FEEDBACK(MODE, HUMAN->player)) {   /* H3: a remote human's positional */
                soundPlayVariantEx(0x33, 0, 0, 99, slot, 1);
            } else {
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(motion->pos);
                else
                    soundLocateForView(motion->pos, -1);
                mtap = PadChkMtap();
                soundPlayVariantEx(0x33, mtap, soundGetListenerPan(), 99, slot, 1);
            }
        }
        break;
    case 0xa0:
        if (weaponTornadoState.timer <= 0) {
            lookup->visible = 0;
            weaponTornadoObject->visible = 1;
            if (weap->invisible == 0)
                weaponTornadoFunnelObject->visible = 1;
            weaponTornadoState.timer = 0x53;
            weaponTornadoState.flags = flags;
            weaponTornadoState.kind = 2;
            weaponTornadoState.anchor = &geom->hp;
            weaponTornadoState.object = lookup;
            memcpy(weaponTornadoObject->pos, lookup->pos, 12);
            memcpy(weaponTornadoFunnelObject->pos, weaponTornadoState.object->pos, 12);
            memcpy(weaponTornadoFunnelObject->rot, weaponTornadoState.object->rot, 8);
#ifdef TM2_PORT_CAR_GROUPS
            carBodyObjectHide(TM2_CAR_KEY(otherid, 0xa0));   /* request 32: the firer's key */
#else
            carBodyObjectHide(0xa0);
#endif
#ifdef TM2_PORT_CAR_SOUND
            /* request 33-4 (CAR_GROUPS.md 16): the Twister's own tornado sound (bit 4; else retail's soundPlay) */
            if (TM2_CAR_FEEDBACK(MODE, HUMAN->player)) {   /* H3: a remote human's positional */
                portCarTornadoSound(otherid, 0, 0);
            } else {
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(motion->pos);
                else
                    soundLocateForView(motion->pos, -1);
                mtap = PadChkMtap();
                portCarTornadoSound(otherid, mtap, soundGetListenerPan());
            }
#else
            SOUND(0x35, motion->pos)
#endif
            if (MODE == 0 && AI->f001 == 1)
                DRV(car)->f458 = 1;
        } else {
            weap->fire_cooldown = 1;
            weap->ammo[0]++;
        }
        break;
    case 0xb0:
#ifdef TM2_PORT_SPECIAL_AS
        portCarSpecialAsRing(otherid, lookup);  /* request 35: a borrower's own ring object, at the firer */
#endif
        objRegistrySetStep(0x914, 0, 1);
        fxSmokePuffBlue(motion->pos, 0);
        for (i = 0, n = otherid; i < carObjectCount(); i++) {
            if (otherid == i)
                continue;
            other = carFromSlot(i, &is_main);
            if (other == 0)
                continue;
            if (is_main) {
                omotion = HUMAN_OF(other)->motion.pos; oflags = &HUMAN_OF(other)->state; oid = HUMAN_OF(other)->slot;
                olookup = humanCarObject(HUMAN_OF(other)->player);
            } else {
                omotion = AI_OF(other)->motion.pos; oflags = &AI_OF(other)->state; oid = AI_OF(other)->slot;
                olookup = aiCarObject(AI_OF(other)->index);
            }
            if (carPairDistance(otherid, oid) < TM2_SPECIAL_NUM(otherid, TM2_SPN_RANGE, 800) && oflags->dead == 0) {
                if (ABS(motion->pos[2] - omotion[2]) < TM2_SPECIAL_NUM(otherid, TM2_SPN_HEIGHT, 200)) {
                    fxExplosion48(omotion, 0);
                    carSpinOut(other, is_main, 7, 1);
                    ang[0] = omotion[0] - motion->pos[0];
                    ang[1] = omotion[1] - motion->pos[1];
                    ang[2] = omotion[2] - motion->pos[2];
                    carApplyDamage(olookup->id, TM2_SPECIAL_NUM(otherid, TM2_SPN_DAMAGE, 15), 0, 2, n);
                }
            }
        }
        if (MODE) {
            near = targetPoolGet();
            for (i = 0; i < 50; i++) {
                if (near[i].active > 0 && DISTANCE(motion->pos, near[i].pos) < 100)
                    targetDamage(near[i].a, near[i].b, 15);
            }
        }
        objects = targetActorsGet();
        for (i = 0; i < 6; i++) {
            if (objects[i].state != -1) {
                struct TM2Triple rest = weaponHitSpinDefault;
                if (DISTANCE(motion->pos, objects[i].actor->pos) < 100)
                    targetActorHit(i, rest.v);
            }
        }
        SOUND(0x37, motion->pos)
        break;
    case 0xc0:
        obj = weap->slam;
        if (obj != 0 && obj->active == 0) {
            weaponSlamStateInit(weap);
            weap->slam->half_damage = 0;
            weap->slam->active = 1;
            if (MODE == 0)
                DRV(car)->f458 = 1;
        }
        break;
    case 0xd0:
        if (weaponStrikeState.object == 0)
            break;
        weaponStrikeState.s.mode = (signed char)human;
        if (weaponStrikeState.s.active != 0 && MODE) {
            weaponStrikeState.s.active = 0;
            weaponStrikeState.s.timer = 0;
            objRegistrySetStep(0x915, 0, 0);
            weaponStrikeState.object->visible = 0;
            fxExplosion96(weaponStrikeState.object->pos, 0);
            areaDamage(weaponStrikeState.object->pos, otherid, 0);
        } else {
            weaponStrikeState.s.active = 1;
            weaponStrikeState.s.timer = TM2_SPECIAL_NUM(otherid, TM2_SPN_LIFE, 100);
            weaponStrikeState.weapon = weap;
            weaponStrikeState.s.owner_id = id;
            objRegistrySetStep(0x915, 0, 1);
            weap->fire_cooldown = 1;
            weap->ammo[0]++;
            weaponStrikeState.object->visible = 1;
            weaponStrikeState.object->f00C = 0;
            if (weap->rear_fire != 0) {
                weap->rear_fire = 0;
                ang[0] = (motion->rot[0] + 0x800) & 0xfff; ang[1] = 0; ang[2] = (motion->rot[2] + 0x800) & 0xfff;
            } else {
                ang[0] = motion->rot[0]; ang[1] = motion->rot[1]; ang[2] = motion->rot[2];
            }
            weaponStrikeState.object->rot[0] = ang[0];
            weaponStrikeState.object->rot[1] = ang[1];
            weaponStrikeState.object->rot[2] = ang[2];
            memcpy(weaponStrikeState.object->pos, motion->pos, 12);
            weaponStrikeState.s.velocity[1] = TM2_SPECIAL_NUM(otherid, TM2_SPN_SPEED, 0x42);
            SOUND(0x3b, motion->pos)
        }
        break;
    case 0xe0:
        AIM
        in[0] = geom->mount_x; in[1] = 0; in[2] = geom->launch_height + 8;
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        weaponFireColoredMissile(id, effect, ang, pos, 10, weaponSpecialColorCyan);
        in[0] = geom->gun[0]; in[1] = geom->gun[1]; in[2] = geom->gun[2];
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireColoredMissile(id, effect, ang, pos, 10, weaponSpecialColorGreen);
        if ((short)r >= 0)
            in[0] = -geom->gun[0];
        in[1] = geom->gun[1]; in[2] = geom->gun[2];
        mathMulTransVec(&motion->rot_m, in, pos);
        pos[0] += motion->pos[0]; pos[1] += motion->pos[1]; pos[2] += motion->pos[2];
        r = weaponFireColoredMissile(id, effect, ang, pos, 10, weaponSpecialColorRed);
        if ((short)r >= 0)
            pos[2] += 200;
        weaponFireFreezeMissile(lookup->id, effect, ang, pos, 7);
        if (TM2_CAR_FEEDBACK(MODE, HUMAN->player))   /* H3: a remote human's as a computer car's */
            soundPlay(13, 0, 0, 99);
        else if (TM2_CAR_POSITIONAL(MODE, AI->active != 0))
            AI_SOUND(13, pos)
        break;
#ifdef TM2_PORT_SPECIAL_PROFILE
    case TM2_SPECIAL_PICKUP_FIRE:       /* request 73: a pickup weapon as a mod car's special */
    case TM2_SPECIAL_PICKUP_HOMING:
    case TM2_SPECIAL_PICKUP_POWER:
    case TM2_SPECIAL_PICKUP_NAPALM:
    case TM2_SPECIAL_PICKUP_MEGA:
    case TM2_SPECIAL_PICKUP_FREEZE:
        portSpecialFirePickup(car, human, vehicle);
        break;
#endif
    case 0xf0:
#ifdef TM2_PORT_DT_PLAYER
        /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth swoops his head at the nearest car (the boss's
           swoop); no car within 4000: the special is given back */
        if (MODE) {
            if (!portCarDtPlayerSwoop(car)) {
                weap->ammo[0]++;
                portCarSpecialRestore(special);
                TM2_VOICE_UNTAG();
                return;
            }
            break;
        }
#endif
    default:
        break;
    }
    TM2_VOICE_UNTAG();
#ifdef TM2_PORT_SPECIAL_PROFILE
    portSpecialFireEnd(otherid, vehicle);   /* request 73: the projectiles this fire made take the car's numbers */
#endif
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(special);
#endif
    flags->weapon_fired = 1;
}

