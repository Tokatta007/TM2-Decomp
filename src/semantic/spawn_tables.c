/* Battle spawn setup (8011EFDC..80120664) and pad-map selection
 * (80120664, 801206F4).
 *
 * demoRespawnAiCars respawns the AI cars for the next battle round (called at
 * the start and, in battle display mode, every 600 tics from the world
 * update; after 5 rounds it ends the session instead). It refills the AI
 * cars, takes a random unused spawn slot 0..3 (demoSpawnSlotsUsed) and runs the
 * level's spawn table with it.
 *
 * The spawn tables (one per level 1..8, demoSpawnFreeway..demoSpawnHongKong) give
 * the first four AI cars their start point (stats.spawn_node) and node (stats.target_node) for
 * that slot, and on a coin flip place player 1's camera at a fixed spot
 * for the intro (demoFollowPlayerCamera set: the view target follows that camera; see
 * world_update.c). */
/* The car arrays are declared through the block views (TM2HumanRecord /
 * TM2AIRecord) instead of cars.h's TM2HumanCar / TM2AICar. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"

extern struct TM2AIRecord aiCars[TM2_MAX_AI_CARS];
extern struct TM2HumanRecord humanCars[TM2_MAX_HUMANS];
extern struct TM2Object *aiCarObjects[TM2_MAX_AI_CARS];
extern s32 demoRoundsPlayed;              /* battle rounds played */
extern u8 gameAttractDemoMode;               /* battle display mode */
extern s32 demoFollowPlayerCamera;              /* view target follows player 1's camera */
extern u8 demoSpawnSlotsUsed[4];            /* spawn slots used */
extern s32 aiCarTargets[TM2_MAX_AI_CARS];
extern u8 padLayout[];             /* pad configuration per port */
extern u8 padLayoutTable[43][5];       /* action, then its button in each of the 4 configurations */

extern int rand(void);
extern int gameGetLevel(void);
extern void rtScheduleEvent(s32, s32);
extern void carFreezeResetAll(void);
extern void debrisClear(void);
extern void startNodesClearUsed(void);
extern void aiPlaceOnRouteNode(struct TM2AIRecord *);
extern void padBindAction(u8 action, u8 button, short port);
extern void padActivateBindings(short port);

void demoSpawnFreeway(int variant);
void demoSpawnMoscow(int variant);
void demoSpawnParis(int variant);
void demoSpawnAmazonia(int variant);
void demoSpawnNewYork(int variant);
void demoSpawnAntarctica(int variant);
void demoSpawnHolland(int variant);
void demoSpawnHongKong(int variant);
void padLoadLayout(short port);

#pragma tm2_rdata .tm2_800CF2E0

/* @8011EFDC */
void __attribute__((section(".text_8011EFDC"))) demoRespawnAiCars(void)
{
    int i, slot;

    if (++demoRoundsPlayed >= 5) {
        rtScheduleEvent(1, 0);
        demoRoundsPlayed = 0;
        gameAttractDemoMode = 0;
        return;
    }
    carFreezeResetAll();
    debrisClear();
    startNodesClearUsed();
    for (i = 0; i < aiCarTotal; i++) {
        aiCarStart(&aiCars[i], aiCars[i].type, 1);
        aiCarObjects[i]->visible = 1;
        aiCars[i].weapons.fire_chance = 100;
        aiCars[i].stats.hp -= rand() & 0x3F;
    }
    slot = rand() % 4;
    if (demoSpawnSlotsUsed[slot]) {
        for (i = 0; i < 4; i++) {
            if (++slot >= 4)
                slot = 0;
            if (!demoSpawnSlotsUsed[slot])
                break;
        }
    }
    demoSpawnSlotsUsed[slot] = 1;
    aiCarTargets[0] = 3;
    aiCarTargets[1] = 3;
    demoFollowPlayerCamera = 0;
    aiCarTargets[2] = 1;
    aiCarTargets[3] = 1;
    switch (gameGetLevel()) {
    case 1:
        demoSpawnFreeway(slot);
        break;
    case 2:
        demoSpawnMoscow(slot);
        break;
    case 3:
        demoSpawnParis(slot);
        break;
    case 4:
        demoSpawnAmazonia(slot);
        break;
    case 5:
        demoSpawnNewYork(slot);
        break;
    case 6:
        demoSpawnAntarctica(slot);
        break;
    case 7:
        demoSpawnHolland(slot);
        break;
    case 8:
        demoSpawnHongKong(slot);
        break;
    }
    for (i = 0; i < aiCarTotal; i++)
        aiPlaceOnRouteNode(&aiCars[i]);
}

/* @8011F284 */
void __attribute__((section(".text_8011F284"))) demoSpawnFreeway(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 11;
        aiCars[0].stats.target_node = 10;
        aiCars[1].stats.spawn_node = 13;
        aiCars[1].stats.target_node = 12;
        aiCars[2].stats.spawn_node = 9;
        aiCars[2].stats.target_node = 10;
        aiCars[3].stats.spawn_node = 7;
        aiCars[3].stats.target_node = 8;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -8344;
            humanCars[0].cam_pos[1] = -1032;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 240;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 16;
        aiCars[0].stats.target_node = 17;
        aiCars[1].stats.spawn_node = 15;
        aiCars[1].stats.target_node = 16;
        aiCars[2].stats.spawn_node = 17;
        aiCars[2].stats.target_node = 16;
        aiCars[3].stats.spawn_node = 20;
        aiCars[3].stats.target_node = 19;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 3120;
            humanCars[0].cam_pos[1] = -5840;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 400;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 27;
        aiCars[0].stats.target_node = 34;
        aiCars[1].stats.spawn_node = 28;
        aiCars[1].stats.target_node = 27;
        aiCars[2].stats.spawn_node = 25;
        aiCars[2].stats.target_node = 26;
        aiCars[3].stats.spawn_node = 24;
        aiCars[3].stats.target_node = 25;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 5184;
            humanCars[0].cam_pos[1] = 5800;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 200;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 47;
        aiCars[0].stats.target_node = 49;
        aiCars[1].stats.spawn_node = 44;
        aiCars[1].stats.target_node = 45;
        aiCars[2].stats.spawn_node = 54;
        aiCars[2].stats.target_node = 53;
        aiCars[3].stats.spawn_node = 55;
        aiCars[3].stats.target_node = 54;
        if (rand() & 1) {
            humanCars[0].cam_pos[1] = -1440;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[0] = 0;
            humanCars[0].cam_pos[2] = 640;
        }
        break;
    }
}

/* @8011F510 */
void __attribute__((section(".text_8011F510"))) demoSpawnMoscow(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 27;
        aiCars[0].stats.target_node = 32;
        aiCars[1].stats.spawn_node = 28;
        aiCars[1].stats.target_node = 27;
        aiCars[2].stats.spawn_node = 21;
        aiCars[2].stats.target_node = 30;
        aiCars[3].stats.spawn_node = 22;
        aiCars[3].stats.target_node = 21;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -800;
            humanCars[0].cam_pos[1] = -800;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 800;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 12;
        aiCars[0].stats.target_node = 1;
        aiCars[1].stats.spawn_node = 11;
        aiCars[1].stats.target_node = 4;
        aiCars[2].stats.spawn_node = 8;
        aiCars[2].stats.target_node = 3;
        aiCars[3].stats.spawn_node = 7;
        aiCars[3].stats.target_node = 2;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -656;
            humanCars[0].cam_pos[1] = -280;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 200;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 28;
        aiCars[0].stats.target_node = 17;
        aiCars[1].stats.spawn_node = 27;
        aiCars[1].stats.target_node = 28;
        aiCars[2].stats.spawn_node = 18;
        aiCars[2].stats.target_node = 17;
        aiCars[3].stats.spawn_node = 19;
        aiCars[3].stats.target_node = 18;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -336;
            humanCars[0].cam_pos[1] = 1080;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 720;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 13;
        aiCars[0].stats.target_node = 1;
        aiCars[1].stats.spawn_node = 15;
        aiCars[1].stats.target_node = 3;
        aiCars[2].stats.spawn_node = 14;
        aiCars[2].stats.target_node = 2;
        aiCars[3].stats.spawn_node = 16;
        aiCars[3].stats.target_node = 4;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -680;
            humanCars[0].cam_pos[1] = 680;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 480;
        }
        break;
    }
}

/* @8011F794 */
void __attribute__((section(".text_8011F794"))) demoSpawnParis(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 7;
        aiCars[0].stats.target_node = 6;
        aiCars[1].stats.spawn_node = 6;
        aiCars[1].stats.target_node = 5;
        aiCars[2].stats.spawn_node = 11;
        aiCars[2].stats.target_node = 4;
        aiCars[3].stats.spawn_node = 2;
        aiCars[3].stats.target_node = 4;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 640;
            humanCars[0].cam_pos[1] = -4640;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 400;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 53;
        aiCars[0].stats.target_node = 50;
        aiCars[1].stats.spawn_node = 54;
        aiCars[1].stats.target_node = 53;
        aiCars[2].stats.spawn_node = 49;
        aiCars[2].stats.target_node = 50;
        aiCars[3].stats.spawn_node = 51;
        aiCars[3].stats.target_node = 50;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -336;
            humanCars[0].cam_pos[1] = 6720;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 400;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 31;
        aiCars[0].stats.target_node = 32;
        aiCars[1].stats.spawn_node = 28;
        aiCars[1].stats.target_node = 20;
        aiCars[2].stats.spawn_node = 33;
        aiCars[2].stats.target_node = 20;
        aiCars[3].stats.spawn_node = 12;
        aiCars[3].stats.target_node = 18;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -2800;
            humanCars[0].cam_pos[1] = -800;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 800;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 41;
        aiCars[0].stats.target_node = 69;
        aiCars[1].stats.spawn_node = 63;
        aiCars[1].stats.target_node = 66;
        aiCars[2].stats.spawn_node = 70;
        aiCars[2].stats.target_node = 68;
        aiCars[3].stats.spawn_node = 49;
        aiCars[3].stats.target_node = 67;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -440;
            humanCars[0].cam_pos[1] = 3440;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 400;
        }
        break;
    }
}

/* @8011F9F8 */
void __attribute__((section(".text_8011F9F8"))) demoSpawnAmazonia(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 9;
        aiCars[0].stats.target_node = 10;
        aiCars[1].stats.spawn_node = 81;
        aiCars[1].stats.target_node = 9;
        aiCars[2].stats.spawn_node = 11;
        aiCars[2].stats.target_node = 10;
        aiCars[3].stats.spawn_node = 12;
        aiCars[3].stats.target_node = 11;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 3152;
            humanCars[0].cam_pos[1] = 5680;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 480;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 6;
        aiCars[0].stats.target_node = 5;
        aiCars[1].stats.spawn_node = 42;
        aiCars[1].stats.target_node = 43;
        aiCars[2].stats.spawn_node = 3;
        aiCars[2].stats.target_node = 4;
        aiCars[3].stats.spawn_node = 2;
        aiCars[3].stats.target_node = 3;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -2880;
            humanCars[0].cam_pos[1] = 960;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 800;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 15;
        aiCars[0].stats.target_node = 16;
        aiCars[1].stats.spawn_node = 3;
        aiCars[1].stats.target_node = 2;
        aiCars[2].stats.spawn_node = 23;
        aiCars[2].stats.target_node = 22;
        aiCars[3].stats.spawn_node = 25;
        aiCars[3].stats.target_node = 0;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -400;
            humanCars[0].cam_pos[1] = -6720;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 720;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 59;
        aiCars[0].stats.target_node = 54;
        aiCars[1].stats.spawn_node = 55;
        aiCars[1].stats.target_node = 54;
        aiCars[2].stats.spawn_node = 62;
        aiCars[2].stats.target_node = 60;
        aiCars[3].stats.spawn_node = 8;
        aiCars[3].stats.target_node = 62;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 640;
            humanCars[0].cam_pos[1] = 9120;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 720;
        }
        break;
    }
}

/* @8011FC6C */
void __attribute__((section(".text_8011FC6C"))) demoSpawnNewYork(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 62;
        aiCars[0].stats.target_node = 61;
        aiCars[1].stats.spawn_node = 63;
        aiCars[1].stats.target_node = 62;
        aiCars[2].stats.spawn_node = 58;
        aiCars[2].stats.target_node = 60;
        aiCars[3].stats.spawn_node = 56;
        aiCars[3].stats.target_node = 57;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 480;
            humanCars[0].cam_pos[1] = 600;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 8160;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 79;
        aiCars[0].stats.target_node = 78;
        aiCars[1].stats.spawn_node = 81;
        aiCars[1].stats.target_node = 80;
        aiCars[2].stats.spawn_node = 74;
        aiCars[2].stats.target_node = 76;
        aiCars[3].stats.spawn_node = 75;
        aiCars[3].stats.target_node = 77;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 9200;
            humanCars[0].cam_pos[1] = 1200;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 8080;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 53;
        aiCars[0].stats.target_node = 43;
        aiCars[1].stats.spawn_node = 4;
        aiCars[1].stats.target_node = 54;
        aiCars[2].stats.spawn_node = 50;
        aiCars[2].stats.target_node = 53;
        aiCars[3].stats.spawn_node = 42;
        aiCars[3].stats.target_node = 43;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -7040;
            humanCars[0].cam_pos[1] = 3600;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 8000;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 0;
        aiCars[0].stats.target_node = 31;
        aiCars[1].stats.spawn_node = 5;
        aiCars[1].stats.target_node = 31;
        aiCars[2].stats.spawn_node = 34;
        aiCars[2].stats.target_node = 32;
        aiCars[3].stats.spawn_node = 71;
        aiCars[3].stats.target_node = 36;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -3200;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[1] = 0;
            humanCars[0].cam_pos[2] = 8080;
        }
        break;
    }
}

/* @8011FEF4 */
void __attribute__((section(".text_8011FEF4"))) demoSpawnAntarctica(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 23;
        aiCars[0].stats.target_node = 24;
        aiCars[1].stats.spawn_node = 22;
        aiCars[1].stats.target_node = 23;
        aiCars[2].stats.spawn_node = 27;
        aiCars[2].stats.target_node = 26;
        aiCars[3].stats.spawn_node = 29;
        aiCars[3].stats.target_node = 28;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -2320;
            humanCars[0].cam_pos[1] = -800;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 5200;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 33;
        aiCars[0].stats.target_node = 34;
        aiCars[1].stats.spawn_node = 43;
        aiCars[1].stats.target_node = 42;
        aiCars[2].stats.spawn_node = 39;
        aiCars[2].stats.target_node = 40;
        aiCars[3].stats.spawn_node = 37;
        aiCars[3].stats.target_node = 36;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -6920;
            humanCars[0].cam_pos[1] = 2560;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 5280;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 12;
        aiCars[0].stats.target_node = 4;
        aiCars[1].stats.spawn_node = 1;
        aiCars[1].stats.target_node = 3;
        aiCars[2].stats.spawn_node = 5;
        aiCars[2].stats.target_node = 4;
        aiCars[3].stats.spawn_node = 7;
        aiCars[3].stats.target_node = 6;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 3552;
            humanCars[0].cam_pos[1] = 1904;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 5360;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 56;
        aiCars[0].stats.target_node = 55;
        aiCars[1].stats.spawn_node = 58;
        aiCars[1].stats.target_node = 57;
        aiCars[2].stats.spawn_node = 60;
        aiCars[2].stats.target_node = 61;
        aiCars[3].stats.spawn_node = 53;
        aiCars[3].stats.target_node = 52;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 1600;
            humanCars[0].cam_pos[1] = 1040;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 5440;
        }
        break;
    }
}

/* @80120178 */
void __attribute__((section(".text_80120178"))) demoSpawnHolland(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 2;
        aiCars[0].stats.target_node = 14;
        aiCars[1].stats.spawn_node = 1;
        aiCars[1].stats.target_node = 5;
        aiCars[2].stats.spawn_node = 20;
        aiCars[2].stats.target_node = 13;
        aiCars[3].stats.spawn_node = 22;
        aiCars[3].stats.target_node = 19;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 2000;
            humanCars[0].cam_pos[1] = 3280;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 640;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 17;
        aiCars[0].stats.target_node = 24;
        aiCars[1].stats.spawn_node = 15;
        aiCars[1].stats.target_node = 18;
        aiCars[2].stats.spawn_node = 30;
        aiCars[2].stats.target_node = 26;
        aiCars[3].stats.spawn_node = 45;
        aiCars[3].stats.target_node = 29;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -800;
            humanCars[0].cam_pos[1] = -2640;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 480;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 35;
        aiCars[0].stats.target_node = 34;
        aiCars[1].stats.spawn_node = 43;
        aiCars[1].stats.target_node = 36;
        aiCars[2].stats.spawn_node = 27;
        aiCars[2].stats.target_node = 32;
        aiCars[3].stats.spawn_node = 39;
        aiCars[3].stats.target_node = 38;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 3120;
            humanCars[0].cam_pos[1] = -3200;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 560;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 14;
        aiCars[0].stats.target_node = 19;
        aiCars[1].stats.spawn_node = 15;
        aiCars[1].stats.target_node = 19;
        aiCars[2].stats.spawn_node = 26;
        aiCars[2].stats.target_node = 23;
        aiCars[3].stats.spawn_node = 27;
        aiCars[3].stats.target_node = 22;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 1920;
            humanCars[0].cam_pos[1] = -800;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 640;
        }
        break;
    }
}

/* @801203EC */
void __attribute__((section(".text_801203EC"))) demoSpawnHongKong(int variant)
{
    switch (variant) {
    case 0:
    default:
        aiCars[0].stats.spawn_node = 66;
        aiCars[0].stats.target_node = 134;
        aiCars[1].stats.spawn_node = 72;
        aiCars[1].stats.target_node = 74;
        aiCars[2].stats.spawn_node = 70;
        aiCars[2].stats.target_node = 76;
        aiCars[3].stats.spawn_node = 68;
        aiCars[3].stats.target_node = 75;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -1600;
            humanCars[0].cam_pos[1] = -1600;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 1600;
        }
        break;
    case 1:
        aiCars[0].stats.spawn_node = 50;
        aiCars[0].stats.target_node = 58;
        aiCars[1].stats.spawn_node = 49;
        aiCars[1].stats.target_node = 50;
        aiCars[2].stats.spawn_node = 57;
        aiCars[2].stats.target_node = 58;
        aiCars[3].stats.spawn_node = 51;
        aiCars[3].stats.target_node = 52;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = -1600;
            humanCars[0].cam_pos[1] = -10240;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 640;
        }
        break;
    case 2:
        aiCars[0].stats.spawn_node = 11;
        aiCars[0].stats.target_node = 12;
        aiCars[1].stats.spawn_node = 32;
        aiCars[1].stats.target_node = 11;
        aiCars[2].stats.spawn_node = 1;
        aiCars[2].stats.target_node = 0;
        aiCars[3].stats.spawn_node = 2;
        aiCars[3].stats.target_node = 1;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 400;
            humanCars[0].cam_pos[1] = -4480;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 640;
        }
        break;
    case 3:
        aiCars[0].stats.spawn_node = 45;
        aiCars[0].stats.target_node = 44;
        aiCars[1].stats.spawn_node = 46;
        aiCars[1].stats.target_node = 45;
        aiCars[2].stats.spawn_node = 42;
        aiCars[2].stats.target_node = 43;
        aiCars[3].stats.spawn_node = 41;
        aiCars[3].stats.target_node = 42;
        if (rand() & 1) {
            humanCars[0].cam_pos[0] = 5000;
            humanCars[0].cam_pos[1] = -10160;
            demoFollowPlayerCamera = 1;
            humanCars[0].cam_pos[2] = 600;
        }
        break;
    }
}

/* @80120664: load port's button map for its pad configuration. */
void __attribute__((section(".text_80120664"))) padLoadLayout(short port)
{
    int i;

    for (i = 0; i < 43; i++)
        padBindAction(padLayoutTable[i][0], padLayoutTable[i][padLayout[port] + 1], port);
}

/* @801206F4: select pad configuration 0..3 for port. */
void __attribute__((section(".text_801206F4"))) padSetLayout(u8 config, short port)
{
    if (config < 4) {
        padLayout[port] = config;
        padLoadLayout(port);
        padActivateBindings(port);
    }
}
