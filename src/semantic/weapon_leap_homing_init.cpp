/* @8010EF44: homing weapon launch, picks the closest target ahead of the
 * shooter. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile).
 *
 * Distance is the largest axis difference, MAX(|dx|, MAX(|dy|, |dz|)) >> 3,
 * with abs as the compiler builtin (one evaluation, bgez/negu). Retail
 * evaluates the MAX operands again on the taken side, re-reading the
 * positions. Here the comparisons read each axis by index into the
 * position vector (AXIS_WORD) and the results read the field; written identically, fold would
 * turn each conditional into a MAX_EXPR and drop those re-reads. The
 * third argument of lightFlickerStart is unused by the callee; retail passes
 * weaponShakeColor at every call site. */
#define ABS(x) __builtin_abs(x)

#include "semantic/cars.h"

struct TM2Vec { int x, y, z; };
struct TM2HomingTarget { char pad0[168]; TM2Vec position; char pad180[100]; int model; };
struct TM2Motion { char pad0[32]; TM2Vec position; char pad44[8]; int heading; };

struct TM2HomingState {
    signed char active;
    signed char source_mode;
    unsigned char pad02[2];
    unsigned char *flags;
    int pad08;
    unsigned short owner_id;
    short age;
    unsigned short target_id;
    short pad12;
    int speed;
    int *health;
    int health_at_launch;
    int velocity[3];
    int *position;
    short *rotation;
    int *target_position;
    int pad38;
};

extern "C" {

extern unsigned char weaponShakeColor[];
TM2HomingTarget *humanCarObject(int);
TM2HomingTarget *aiCarObject(int);
int humanCarCount(void);
int aiCarCount(void);
TM2HumanRecord *humanCarGet(int);
TM2AIRecord *aiCarGet(int);
int ratan2(int, int);
#include "semantic/world.h"
}

/* Storage is defined by round_reset.c (weaponTornadoState spans 80184DC0..80184E10). */
extern TM2HomingState homingLeapState;
#pragma tm2_absolute homingLeapState

#define HUMAN(p) ((TM2HumanRecord *)(p))
#define AI(p) ((TM2AIRecord *)(p))
#define AXIS_WORD(m, t, k) ABS(((int *)&(m)->position)[k] - ((int *)&(t)->position)[k])
#define AXIS(m, t, c) ABS((m)->position.c - (t)->position.c)
#define MAX_YZ(m, t) (AXIS_WORD(m, t, 1) < AXIS_WORD(m, t, 2) ? AXIS(m, t, z) : AXIS(m, t, y))
#define DISTANCE(m, t) ((AXIS_WORD(m, t, 0) < MAX_YZ(m, t) ? MAX_YZ(m, t) : AXIS(m, t, x)) >> 3)

extern "C" void __attribute__((section(".text_8010EF44"))) weaponHomingLeapInit(unsigned char *source, int source_mode)
{
    TM2HomingTarget *source_lookup, *target;
    TM2Motion *motion;
    int player_count, ai_count, source_model, best_distance, i, distance, dx, dy, heading;

    if (source_mode) {
        source_lookup = humanCarObject(HUMAN(source)->player);
        homingLeapState.owner_id = HUMAN(source)->slot;
        homingLeapState.health = &HUMAN(source)->stats.hp;
        homingLeapState.health_at_launch = HUMAN(source)->stats.hp;
        motion = (TM2Motion *)&HUMAN(source)->motion;
        homingLeapState.speed = (HUMAN(source)->stats.speed_cap / 32) * 2;
        homingLeapState.position = HUMAN(source)->motion.pos;
        homingLeapState.rotation = (short *)HUMAN(source)->motion.rot;
        homingLeapState.flags = &HUMAN(source)->contact.in_contact;
    } else {
        source_lookup = aiCarObject(AI(source)->index);
        homingLeapState.owner_id = AI(source)->slot;
        motion = (TM2Motion *)&AI(source)->motion;
        homingLeapState.speed = (AI(source)->stats.speed_cap / 32) * 2;
        homingLeapState.health = &AI(source)->stats.hp;
        homingLeapState.health_at_launch = AI(source)->stats.hp;
        homingLeapState.position = AI(source)->motion.pos;
        homingLeapState.rotation = (short *)AI(source)->motion.rot;
        homingLeapState.flags = &AI(source)->contact.in_contact;
    }

    player_count = humanCarCount();
    ai_count = aiCarCount();
    source_model = source_lookup->model;
    best_distance = 400;
    homingLeapState.source_mode = source_mode;
    homingLeapState.velocity[0] = 0;
    homingLeapState.velocity[1] = 0;
    homingLeapState.velocity[2] = 0;

    for (i = 0; i < player_count; i++) {
        target = humanCarObject(i);
        distance = DISTANCE(motion, target);
        dx = target->position.x - motion->position.x;
        dy = target->position.y - motion->position.y;
        if (dy == 0) dy = 1;
        heading = ratan2(dx, dy);
        if (target->model != source_model && distance < 400 && distance < best_distance
            && ABS(motion->heading - heading) < 682) {
            homingLeapState.active = 1;
            homingLeapState.age = 0;
            best_distance = distance;
            homingLeapState.target_id = target->model;
            homingLeapState.target_position = humanCarGet(i)->motion.pos;
            lightFlickerStart(source_lookup, 60, weaponShakeColor);
        }
    }
    for (i = 0; i < ai_count; i++) {
        if (aiCarGet(i)->stats.hp <= 0) continue;
        target = aiCarObject(i);
        distance = DISTANCE(motion, target);
        dx = target->position.x - motion->position.x;
        dy = target->position.y - motion->position.y;
        if (dy == 0) dy = 1;
        heading = ratan2(dx, dy);
        if (target->model != source_model && distance < 400 && distance < best_distance
            && ABS(motion->heading - heading) < 682) {
            homingLeapState.active = 1;
            homingLeapState.age = 0;
            best_distance = distance;
            homingLeapState.target_id = target->model;
            homingLeapState.target_position = aiCarGet(i)->motion.pos;
            lightFlickerStart(source_lookup, 60, weaponShakeColor);
        }
    }
}
