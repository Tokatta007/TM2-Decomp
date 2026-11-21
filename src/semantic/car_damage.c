/* Car damage (800E5628): carApplyDamage(target, amount, dir, type, attacker)
 * applies `amount` damage to car `target` (a lookup id), hit from direction
 * `dir` (may be 0) by `attacker` (a car id; bit 15 set = no car, e.g. the
 * environment). `type` 1 = a blow that may knock the car back.
 *
 * Order (see docs/CAR_STATS.md "Special weapon damage"):
 *   - x2 if the attacker is a human driving Dark Tooth (stats +122 == 100);
 *   - heavy blows (type 1, >= 11) knock the target back, less for heavy cars;
 *   - while the target holds the energy shield (state +0x0E): no damage,
 *     and a burning target is put out instead;
 *   - otherwise (target active, or the game not in a cut-scene / car 0xF0):
 *     flash the object (red for weapon blows, white otherwise), maybe spin out (>= 8), impact effect, halve for the
 *     shield flag (stats +60), /4 when attacker and target are both AI,
 *     remember the attacker, subtract HP (+164) and the AI retaliation
 *     budget (+140); ram hits under 5 cannot kill; at 0 HP: wreck sound,
 *     score, killer credit, remove the fire effect; then the wreck itself
 *     (big blows: debris, or Minion's special break-up) or a spin-out;
 *   - a frozen target's freeze timer (stats +26) drops by 6 x damage.
 * The stats, state and weapon blocks are the car record's (human +0x14C /
 * +0x9C / +0x7C8, AI +0x4FC / +0x6C / +0x808), as in car_motion.c. */
#include "semantic/weapons.h"
#include "semantic/car_groups.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
/* After a won round a car takes no damage, but for the computer boss (retail: every 0xF0). Request 41 item A: a
   player's Dark Tooth is spared too (TM2_PORT_DT_PLAYER); request 42 item 1: an ordinary Dark Tooth (bit 10). */
#if defined(TM2_PORT_DT_PLAYER)
#define TM2_WON_SPARED (car_type != 0xF0 || human || TM2_DT_ORDINARY(ai_car->slot))
#elif defined(TM2_PORT_DARK_TOOTH)
#define TM2_WON_SPARED (car_type != 0xF0 || (!human && TM2_DT_ORDINARY(ai_car->slot)))
#else
#define TM2_WON_SPARED (car_type != 0xF0)
#endif


typedef struct {
    u8 pad_00[4];
    s8 f04;                 /* +0x04 */
    s8 f05;                 /* +0x05 wreck state */
    u8 pad_06;
    s8 killer;              /* +0x07 */
    s16 last_attacker;      /* +0x08 */
    u8 pad_0A[0x10];
    s16 freeze;             /* +0x1A */
    u8 pad_1C[0x20];
    s16 shield;             /* +0x3C */
    u8 pad_3E[0x3C];
    s16 mass;               /* +0x7A */
    u8 pad_7C[0x10];
    s16 retaliate;          /* +0x8C */
    u8 pad_8E[0x16];
    s32 hp;                 /* +0xA4 */
    s32 damageable;         /* +0xA8 */
} TM2DamageStats;

typedef struct {
    u8 dead;                /* +0x00 */
    u8 pad_01[0x0C];
    u8 frozen;              /* +0x0D */
    u8 energy_shield;       /* +0x0E */
    u8 pad_0F[6];
    u8 f15;                 /* +0x15 */
    u8 pad_16[4];
    u8 burning;             /* +0x1A */
    u8 pad_1B[4];
    u8 f1F;                 /* +0x1F */
} TM2DamageState;

typedef struct {
    u8 pad_00[0x44];
    struct TM2FireEffect *fire;     /* +0x44 */
} TM2DamageWeapon;

extern s32 timerGetFrameTicks(void);
extern struct TM2SceneObject *humanCarObject(s32 index);
extern struct TM2SceneObject *aiCarObject(s32 index);
extern void carKnockBack(void *car, u8 human, s32 *dir, s32 amount);
extern int *fxSmokePuffBlue(s32 *pos, s32);
extern void fxSmokeC48(s32 *pos, s32);
extern u8 roundIsWon(void);
extern void lightFlickerStart(struct TM2SceneObject *object, s32 time, u8 *color);
extern int rand(void);
extern void fxImpactDebris(s32 size, s32 *pos);
extern s32 aiCarsCountAlive(void);
extern void hudMessagePost(s32, s32);
extern int gameGetLevel(void);
extern s32 humanCarNearest(s32 *pos, s32);
extern void *memcpy(void *, void *, unsigned int);
extern void *memset(void *, int, unsigned int);
extern s32 aiDarkToothBossModel(void);
extern struct TM2SceneObject *aiDarkToothBossObject(void);
extern s32 gameGetMode(void);
extern void debrisBurstAt(s32, s32 *, s16 *, s16 *, s32 *, s32, s32, s32, s32);
extern void fxExplosion192(s32 *pos, u8 flag);
extern void fxSmokeA48(s32 *pos, s32);
extern s32 carWreckModelGet(s32 type);
extern void debrisBreakModel(struct TM2SceneObject *object, s32, u8 *, s16 *, s32, s32);
extern void carSetDamageStage(s32 type, s32, s32);
extern void fxSmokeA24(s32 *pos, s32);
/* request 32 (CAR_GROUPS.md): the hit car's key (its slot: human +0x004, AI +0x006) */
#ifdef TM2_PORT_CAR_GROUPS
#define DAMAGE_KEY TM2_CAR_KEY(human ? human_car->slot : ai_car->slot, car_type)
#else
#define DAMAGE_KEY car_type
#endif

/* @800E5628: apply damage to a car (see the file header).  Local
 * declaration order fixes retail's spill-slot order. */
void __attribute__((section(".text_800E5628"))) carApplyDamage(target, amount, dir, type, attacker)
s32 target; s32 amount; s32 *dir; s32 type; s16 attacker;   /* old-style: most callers pass an int, the body narrows */
{
    struct TM2HumanRecord *human_car = 0;
    struct TM2AIRecord *ai_car = 0;
    u8 active;
    TM2DamageStats *st;
    TM2DamageState *s;
    TM2DamageWeapon *w;
    s8 *wreck, *killer;
    u8 *last;
    s32 dead;
    s32 shake;
    void *car;              /* human or AI record */
    struct TM2CarMotion *motion;
    struct TM2SceneObject *object;
    s32 car_type;
    s32 *fx;
    u8 attacker_human;
    u8 human;
    s32 index;
    s32 size, n, debris;
    u8 color[4];
    s16 vec[4];
    s16 rot[4];
    s32 spin[4];
#ifdef TM2_PORT_DARK_TOOTH
    s32 special;
#endif
    u8 pos[16];

    if (amount >= 3)
        shake = 16 / (s16)timerGetFrameTicks();
    else
        shake = 6 / (s16)timerGetFrameTicks();
    if (!type) {
        color[0] = 255;
        color[1] = 255;
        color[2] = 255;
    } else {
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
    }
    attacker_human = 0;
    car = carFromSlot(attacker, &attacker_human);
    if (attacker_human && HREC(car)->stats.mass == 100)
        amount *= 2;
    if (!carFindById(target, &human, &index))
        return;
    if (human) {
        car = humanCarGet(index);
        active = 1;
        human_car = humanCarGet(index);
        motion = &human_car->motion;
        dead = human_car->view;
        st = (TM2DamageStats *)&human_car->stats;
        wreck = &human_car->stats.wreck_state;
        killer = &human_car->stats.killer;
        last = (u8 *)&human_car->stats.last_attacker;
        object = humanCarObject(index);
        car_type = human_car->type;
        fx = human_car->fx;
        s = (TM2DamageState *)&human_car->state;
        w = (TM2DamageWeapon *)&human_car->weapons;
    } else {
        car = aiCarGet(index);
        ai_car = aiCarGet(index);
        st = (TM2DamageStats *)&ai_car->stats;
        motion = &ai_car->motion;
        wreck = &ai_car->stats.wreck_state;
        killer = &ai_car->stats.killer;
        last = (u8 *)&ai_car->stats.last_attacker;
        s = (TM2DamageState *)&ai_car->state;
        w = (TM2DamageWeapon *)&ai_car->weapons;
        object = aiCarObject(index);
        active = ai_car->active;
        car_type = ai_car->type;
        fx = ai_car->state.x2C.fx;
    }
    if (type == 1 && amount >= 11) {
        /* knock-back strength; retail keeps it in the (dead) `target` */
        target = 240 * amount / 25 + 40;
        target -= target * st->mass / 100;
        if (dir)
            carKnockBack(car, human, dir, target);
    }
    if (s->energy_shield) {
        if (s->burning) {
            s->burning = 0;
            if (w->fire)
                w->fire->active = 0;
            if (human) {
                fxSmokePuffBlue(human_car->motion.pos, 0);
                fxSmokeC48(human_car->motion.pos, 0);
            }
        }
        return;
    }
    /* request 41 item A (DARK_TOOTH_PLAYER.md 8): after a won round only the computer boss (an AI 0xF0) still takes
       damage, as retail; a player's Dark Tooth is treated like every other car; request 42 item 1: an ordinary Dark
       Tooth (bit 10) too (TM2_WON_SPARED) */
    if (!active || (roundIsWon() && TM2_WON_SPARED))
        return;
    lightFlickerStart(object, shake, color);
    if (amount <= 0)
        return;
    if (amount >= 8 && (rand() & 7) < amount - 7 && !s->f15)
        carSpinOut(car, human, 4, 0);
    if (amount >= 0) {
        /* impact size MIN(MAX(amount, 3), 8), spelt as retail's jumps */
        if (amount < 3 || amount < 8) {
            if (amount < 3)
                size = 3;
            else
                size = amount;
        } else
            size = 8;
        fxImpactDebris(size, object->pos);
    }
    if (st->shield)
        amount /= 2;
    if (!human && !attacker_human)
        amount /= 4;
    if (st->hp > 0 && last && attacker >= 0)
        *last = attacker;
    if (st->damageable) {
        if (!gameIsAttractDemo()) {
            if (human)
                st->hp -= amount;
            else if (attacker >= 0) {
                st->hp -= amount;
                st->retaliate -= amount;
            }
        }
        if (st->hp <= 0 && !s->dead && type == 0 && amount < 5)
            st->hp += amount;
        if (st->hp <= 0 && !s->dead) {
            carBodyObjectHide(DAMAGE_KEY);
            hudMessagePost(aiCarsCountAlive(), 0);
            if (killer && attacker >= 0)
                *killer = attacker;
            if (fx) {
                fx[2] = 0;
                if (human)
                    human_car->fx = 0;
                else
                    ai_car->state.x2C.fx = 0;
            }
        }
        if (st->hp <= 0 && *wreck != 4 && (gameGetLevel() != 6 || !s->f1F)) {
            if (car_type == 0xF0) {
                amount = 6;
                type = 1;
            }
            if (type && amount >= 6) {
                if (dir) {
                    vec[0] = dir[0];
                    vec[1] = dir[1];
                    vec[2] = dir[2];
                } else {
                    vec[0] = 0;
                    vec[1] = 0;
                    vec[2] = 4096;
                }
                memcpy(pos, HREC(humanCarGet(humanCarNearest(object->pos, -1)))->motion.pos, 12);
                if (car_type == 0xF0) {
                    vec[0] = 0;
                    vec[1] = 0;
                    vec[2] = 4096;
                    rot[0] = 0;
                    rot[1] = 0;
                    rot[2] = 0;
                    spin[0] = 0;
                    spin[1] = 0;
                    spin[2] = 0;
#ifdef TM2_PORT_DARK_TOOTH
                    /* request 33-2e: this Dark Tooth's own stage and head */
                    special = portCarSpecialSelect(DAMAGE_KEY);
#endif
                    if (aiBossDefeated()) {
                        debris = aiDarkToothBossModel();
                        aiDarkToothBossObject()->visible = 0;
                        n = 10;
                    } else {
                        debris = object->model;
                        do n = 30; while (0); /* register priority: n in s0 */
                    }
                    if (gameGetMode() != 2)
                        debrisBurstAt(debris, motion->pos, vec, rot, spin, 0, n, 100, 0);
#ifdef TM2_PORT_DARK_TOOTH
                    *wreck = portCarDarkToothWreck(human ? 0 : (void *)ai_car);   /* 3; 4 after a final death with bit 6 */
                    portCarSpecialRestore(special);
#else
                    *wreck = 3;
#endif
                    fxExplosion192(object->pos, 0);
                    fxSmokeA48(object->pos, 0);
                } else {
                    debrisBreakModel(object, carWreckModelGet(DAMAGE_KEY), pos, vec, 10, 200);
                    *wreck = 4;
                }
                if (object)
                    object->visible = 0;
            } else if (!s->dead) {
                carSpinOut(car, human, 4, 0);
                if (st->f05 != 3) {
                    carSetDamageStage(DAMAGE_KEY, 3, st->f04);
                    *wreck = 3;
                    fxSmokeA24(motion->pos, 0);
                }
            }
        }
    }
    if (s->frozen && !s->dead) {
        st->freeze -= 6 * amount;
        if (st->freeze < 0)
            st->freeze = 0;
    }
}

/* @800E5F24: knock car `car` back along `dir` with strength `amount` (heavy
 * cars, mass >= 51, take half). Sets the push speed (+18) and the push timer
 * (+6): 120 below 40, otherwise 300. The intended ramp between 40 and 800,
 * 300 - (800 - amount) / 800, is always 300 (integer division by 800). */
void __attribute__((section(".text_800E5F24"))) carKnockBack(void *car, u8 human, s32 *dir, s32 amount)
{
    struct TM2CarContact *push;
    TM2DamageStats *st;

    if (human) {
        push = &HREC(car)->contact;
        st = (TM2DamageStats *)&HREC(car)->stats;
    } else {
        push = &AI(car)->contact;
        st = (TM2DamageStats *)&AI(car)->stats;
    }
    if (st->mass >= 51)
        amount /= 2;
    memcpy(push->hit.pos, dir, 12);
    push->push = amount;
    push->f05 = 0;
    push->f04 = 0;
    push->f03 = 0;
    if (amount < 40)
        push->timer = 120;
    else if (amount > 800)
        push->timer = 300;
    else
        push->timer = 300 - (800 - amount) / 800;
}
