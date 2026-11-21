#include "semantic/cars.h"

extern struct TM2Object *humanCarObject(int);
extern struct TM2Object *aiCarObject(int);
extern int aiCarCount(void);
extern int humanCarCount(void);
#include "semantic/world.h"

#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"
/* HUMANS16_PLAN.md H2 (request 37 answer 10): an AI owner's aimed shot goes at human 0 (retail), or with
 * portHumanOptions bit 2 at its own target (portAiAimTarget) */
#define AI_AIM_TARGET(owner) ((int)portAiAimTarget(owner))
#else
#define AI_AIM_TARGET(owner) ((int)humanCarObject(0)->pos)
#endif

#define MODE mode
#define ABS(x) __builtin_abs(x)
#define LOOK (MODE ? humanCarObject(owner) : aiCarObject(owner))

/* @80109F94. Nearest target ahead of the owner, written to *out as a pointer to the target's position
 * (object +0xA8), 0 when none. Layout by the low byte of mode: player owner (humanCarObject) or AI owner
 * (aiCarObject); an AI owner with the gameIsAttractDemo byte clear takes player 0's position and returns.
 * Pass 1 scans the AI objects (count aiCarCount) whose record word +0x5A0 (aiCarGet) is positive;
 * pass 2, only when pass 1 found nothing, scans the players (count humanCarCount), skipping the player
 * owner itself. Each candidate's offset from the owner is turned into the owner's frame with
 * mathMulMatVec(owner matrix +0x80, offset, local); it qualifies when local y >= 12, |y| >= |x|,
 * |y| < 8001 (pass 1 also needs local z < 401) and y is below the best so far (or nothing is held yet).
 * The callers (weaponFireEnergyAttack, weaponLaunchProjectile, weaponFireSpecial) ignore $v0. Old-style definition: the
 * mode byte arrives promoted and is narrowed at each use, as in retail. */
void __attribute__((section(".text_80109F94")))
weaponFindTargetAhead(owner, out, mode)
int owner; int *out; unsigned char mode;
{
    struct TM2Object *obj, *car;
    int *self, *cand;
    int best, n, i;
    int delta[3], local[3];

    best = 0;
    if (!MODE && !(gameIsAttractDemo() & 0xff)) {
        *out = AI_AIM_TARGET(owner);
        return;
    }
    self = LOOK->pos;
    n = aiCarCount();
    *out = 0;
    for (i = 0; i < n; i++) {
        obj = aiCarObject(i);
        cand = obj->pos;
        if (cand == 0)
            continue;
        if (aiCarGet(i)->stats.hp <= 0)
            continue;
        *(delta + 0) = obj->pos[0] - self[0];
        *(delta + 1) = obj->pos[1] - self[1];
        *(delta + 2) = obj->pos[2] - self[2];
        mathMulMatVec((short *)LOOK->matrix, delta, local);
        if (local[1] < 12)
            continue;
        if (ABS(local[1]) < ABS(local[0]))
            continue;
        if (ABS(local[1]) >= 8001)
            continue;
        if (local[2] >= 401)
            continue;
        if (best > 0 && local[1] >= best)
            continue;
        best = local[1];
        *out = (int)cand;
    }
    if (best > 0)
        return;
    n = humanCarCount();
    *out = 0;
    for (i = 0; i < n; i++) {
        car = humanCarObject(i);
        cand = car->pos;
        if (cand == 0)
            continue;
        if (MODE && i == owner)
            continue;
        *(delta + 0) = car->pos[0] - self[0];
        *(delta + 1) = car->pos[1] - self[1];
        *(delta + 2) = car->pos[2] - self[2];
        mathMulMatVec((short *)LOOK->matrix, delta, local);
        if (local[1] < 12)
            continue;
        if (ABS(local[1]) < ABS(local[0]))
            continue;
        if (ABS(local[1]) >= 8001)
            continue;
        if (best > 0 && local[1] >= best)
            continue;
        best = local[1];
        *out = (int)cand;
    }
}

/* @8010A2AC. Rear-cone twin of weaponFindTargetAhead: the same owner selection, early return, two passes and
 * matrix transform, but a candidate qualifies when local y < -11; the distance is -y and it must satisfy
 * -y >= |x|, -y < 8001 and be below the best so far. No height gate in either pass. The callers
 * (weaponLaunchProjectile, weaponFireSpecial) ignore $v0. */
void __attribute__((section(".text_8010A2AC")))
weaponFindTargetBehind(owner, out, mode)
int owner; int *out; unsigned char mode;
{
    struct TM2Object *obj, *car;
    int *self, *cand;
    int best, n, i;
    int delta[3], local[3];

    best = 0;
    if (!MODE && !(gameIsAttractDemo() & 0xff)) {
        *out = AI_AIM_TARGET(owner);
        return;
    }
    self = LOOK->pos;
    n = aiCarCount();
    *out = 0;
    for (i = 0; i < n; i++) {
        obj = aiCarObject(i);
        cand = obj->pos;
        if (cand == 0)
            continue;
        if (aiCarGet(i)->stats.hp <= 0)
            continue;
        *(delta + 0) = obj->pos[0] - self[0];
        *(delta + 1) = obj->pos[1] - self[1];
        *(delta + 2) = obj->pos[2] - self[2];
        mathMulMatVec((short *)LOOK->matrix, delta, local);
        if (local[1] >= -11)
            continue;
        if (ABS(local[1]) < ABS(local[0]))
            continue;
        if (ABS(local[1]) >= 8001)
            continue;
        if (best > 0 && ABS(local[1]) >= best)
            continue;
        best = ABS(local[1]);
        *out = (int)cand;
    }
    if (best > 0)
        return;
    n = humanCarCount();
    *out = 0;
    for (i = 0; i < n; i++) {
        car = humanCarObject(i);
        cand = car->pos;
        if (MODE && i == owner)
            continue;
        if (cand == 0)
            continue;
        *(delta + 0) = car->pos[0] - self[0];
        *(delta + 1) = car->pos[1] - self[1];
        *(delta + 2) = car->pos[2] - self[2];
        mathMulMatVec((short *)LOOK->matrix, delta, local);
        if (local[1] >= -11)
            continue;
        if (ABS(local[1]) < ABS(local[0]))
            continue;
        if (ABS(local[1]) >= 8001)
            continue;
        if (best > 0 && ABS(local[1]) >= best)
            continue;
        best = ABS(local[1]);
        *out = (int)cand;
    }
}
