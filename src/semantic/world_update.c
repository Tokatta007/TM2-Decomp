/* World update (8011A700), called twice per frame by rtMainLoop.
 *
 * 1. Human cars (at most 2 in retail, TM2_MAX_HUMANS in the port variant; record
 *    pointer from humanCarObjects): car update, radar pair, speed readout,
 *    camera and view target.
 * 2. Battle display mode (gameAttractDemoMode): blinking message and two timers.
 *    Otherwise the AI director: rank the AI cars and hand out the
 *    attack/hunt/flee states.
 * 3. AI cars: AI update, weapon sounds, taunt sounds.
 * 4. Round end (checks player 1 only), then every world subsystem.
 *
 * The record views below name only the fields used here (see cars.h). */

typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

#include "semantic/players.h"

struct TM2HumanSim {
    u8 pad_000[0x4];
    s16 slot;               /* +0x004 car slot */
    u8 pad_006[0xE];
    s32 f_014;              /* +0x014 */
    s32 f_018;              /* +0x018 */
    u8 pad_01C[0x74];
    u8 *object;             /* +0x090 */
    u8 pad_094[0x8];
    u8 f_09C;               /* +0x09C */
    u8 pad_09D[0x153];
    s32 hp;                 /* +0x1F0 */
    u8 pad_1F4[0x508];
    s32 pos[3];             /* +0x6FC */
    s32 rot[3];             /* +0x708 */
    u8 pad_714[0x100];
};
typedef char tm2_human_sim_size[(sizeof(struct TM2HumanSim) == 0x814) ? 1 : -1];

/* AI director states: 0 roam, 1 hunt, 2 and 3 attack. */
struct TM2AISim {
    u8 pad_000[0x1];
    s8 state;               /* +0x001 */
    u8 pad_002[0x2];
    s16 f_004;              /* +0x004 */
    s16 slot;               /* +0x006 car slot */
    u8 pad_008[0x2];
    s16 closer;             /* +0x00A AI cars nearer the player */
    s16 f_00C;              /* +0x00C */
    s16 f_00E;              /* +0x00E time in state 2 */
    s16 f_010;              /* +0x010 */
    s16 f_012;              /* +0x012 */
    u8 pad_014[0x14];
    s32 distance;           /* +0x028 to the player */
    u8 pad_02C[0x4];
    s32 f_030;              /* +0x030 weapon */
    u8 f_034;               /* +0x034 */
    u8 pad_035[0x37];
    u8 f_06C;               /* +0x06C */
    u8 pad_06D[0x494];
    s8 f_501;               /* +0x501 */
    u8 pad_502[0x7E];
    s16 f_580;              /* +0x580 */
    u8 pad_582[0x2];
    s16 f_584;              /* +0x584 */
    s16 f_586;              /* +0x586 */
    s16 f_588;              /* +0x588 */
    s16 f_58A;              /* +0x58A */
    u8 pad_58C[0x10];
    s32 f_59C;              /* +0x59C updated this frame */
    s32 hp;                 /* +0x5A0 */
    u8 pad_5A4[0x198];
    s32 pos[3];             /* +0x73C */
    u8 pad_748[0xE6];
    s16 f_82E;              /* +0x82E */
    u8 pad_830[0x24];
};
typedef char tm2_ai_sim_size[(sizeof(struct TM2AISim) == 0x854) ? 1 : -1];

typedef struct TM2HumanSim Human;
typedef struct TM2AISim AICar;

extern Human humanCars[TM2_MAX_HUMANS];
extern AICar aiCars[TM2_MAX_AI_CARS];
extern s32 humanCarTotal;              /* human cars */
extern s32 aiCarTotal;              /* AI cars */
extern s32 aiCarsRemaining;
extern void *humanCarObjects[TM2_MAX_HUMANS];         /* human car world objects */
extern void *aiCarObjects[TM2_MAX_AI_CARS];         /* AI car world objects */
extern s32 humanCarObjectSlots[];            /* object slot of each human */
extern s32 aiCarObjectSlots[];            /* object slot of each AI car */
extern s32 gameMode;              /* game type (2: two players, 4: ...) */
extern s32 viewMainPlayer;              /* player whose camera is always updated */
extern u8 gameAttractDemoMode;               /* battle display mode */
extern s32 demoFollowPlayerCamera;
extern s32 viewWatchedAiCar;              /* AI car the player is watching */
extern s16 worldDemoBlinkOn, worldDemoBlinkTimer;  /* blinking message state */
extern char worldDemoText[];
extern s32 demoWatchTimer, demoRespawnTimer;
extern s32 roundWonFlag;              /* round over */
extern s32 roundOverFlag;
extern s32 aiBossDefeatedFlag;

extern int gameGetLevel(void);
extern void worldTrackVehiclesUpdate(void);
extern void worldUpdateStub(void);
extern void humanCarUpdate(void *object, Human *car);
extern void aiCarUpdate(void *object, AICar *car);
extern int rand(void);
extern void hudRadarPlotCar(void *, s32 *, s32, s32);
extern void carSoundUpdate(s32, void *, u8 a2);
extern void humanCarDamageUpdate(s32);
extern void hudSmoothSpeed(short);
extern int viewGetSplitMode(void);
extern void viewCarCameraUpdate(Human *, s32);
extern void viewSetFollowObjPos(s32 *, s32);
#include "semantic/hud.h"
#include "semantic/world.h"
extern void demoRespawnAiCars(void);
extern void demoWatchNextAiCar(void);
#include "semantic/input.h"
extern void rtScheduleEvent(s32, s32);
extern u8 aiCarShouldUpdate(s32);
#include "semantic/sound.h"
extern void aiCarDamageUpdate(s32);
extern void aiCarsCountRemaining(void);
extern s32 *aiDarkToothBossObject(void);
#include "semantic/car_numbers.h"   /* request 70: the radar blip's car */
#ifdef TM2_PORT_CAR_PROFILE
/* Request 70 (CAR_PROFILES.md): the type a blip is plotted with carries the car's slot + 1 in bits 16..23, so
 * hudRadarPlotCar can give a mod car its profile's colour (portCarProfileRadar); no call and no local here, so
 * worldUpdate's frame stays retail's. Retail (and without the define): the type alone. */
#define WORLD_RADAR_AI(i) ((void *)(aiCars[i].f_030 | (aiCars[i].slot + 1) << 16))
#define WORLD_RADAR_HUMAN(i) ((void *)((s32)humanCars[i].object | (humanCars[i].slot + 1) << 16))
#else
#define WORLD_RADAR_AI(i) ((void *)aiCars[i].f_030)
#define WORLD_RADAR_HUMAN(i) (humanCars[i].object)
#endif
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-3 (CAR_GROUPS.md 12) */
#endif
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map; request 50: vacant records */
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"   /* request 48 stage 3 SOUND_GATE: the CD track test */
#endif
extern void pickupRespawnTick(void);
extern void carsPhysicsUpdate(void);
extern void carPairsUpdate(void);
extern u8 audioTakeTrackEnded(void);
extern void audioAlternateMusicTrack(void);
extern u8 soundGetDeniedPending(void);
extern void soundUpdate(void);
extern void groundFiresUpdate(void);
extern void weaponRicochetUpdate(void);
extern void weaponShieldsUpdate(void);
extern void weaponInvisibilityUpdate(void);
extern void weaponTornadoUpdate(void);
extern void weaponHomingLeapUpdate(void);
extern void weaponShadowStrikeUpdate(void);
extern void worldWarpsUpdate(void);
extern void worldCollapseUpdate(void);
extern void nullsub_8(void);

#ifdef TM2_PORT_HUMANS
/* H1 (HUMANS16_PLAN.md): a blip for human h goes on h's radar, the one of its local slot, and on none for a remote
 * human (portHumanRadarPlot). Only the plot is local: the rand() of each test stays, so every machine draws the
 * same numbers. Retail (and without the define): radar h. */
#define WORLD_RADAR_PLOT(type, delta, angle, h) portHumanRadarPlot((void *)(type), delta, angle, h)

/* H1: human i's camera on its local slot (retail: viewMainPlayer == i || split, view i; viewMainPlayer 0 now
 * means slot 0); a remote human's camera does not run (the shared camera globals stay this machine's). Out of
 * line, as worldRadarPlotHumans, so worldUpdate's frame stays retail's 0xB0 (FX_SPRITE_STACK_POINTER.md). */
static void __attribute__((section(".port_text"))) worldHumanCamera(s32 i)
{
    s32 slot = TM2_HUMAN_LOCAL(i);

    if (slot >= 0 && (viewMainPlayer == slot || viewGetSplitMode()))
        viewCarCameraUpdate(&humanCars[i], slot);
}

/* H1: the sky copy of human i's view follows it; none for a remote human. */
static void __attribute__((section(".port_text"))) worldHumanFollow(s32 *target, s32 i)
{
    s32 slot = TM2_HUMAN_LOCAL(i);

    if (slot >= 0)
        viewSetFollowObjPos(target, slot);
}

/* H2 (H1 review): portHumanOptions bit 4, the AI blips on every local human's radar in every game mode. Retail
 * (and the bit clear) plots them for human 0 always and for the other humans only in co-op (mode 2), each behind
 * its own `abs(dz) < 400 || rand() % 5 == 0`; outside mode 2 humans 1.. get none. With the bit, outside mode 2,
 * every local human 1.. gets them too, with no rand(): a car at another height blinks on this machine's clock
 * (portHumanRadarBlink, one step per frame) instead. Drawing only; retail's plots and rand() calls are unchanged. */
static void __attribute__((section(".port_text"))) worldRadarAiAll(void)
{
    s32 delta[3];
    s32 i, j;
    unsigned int blink;

    if (!(portHumanOptions & TM2_HUMAN_OPT_RADAR_AI) || gameMode == 2)
        return;
    blink = portHumanRadarBlink++;
    for (i = 0; i < aiCarTotal; i++) {
        if (aiCars[i].hp <= 0)
            continue;
        for (j = 1; j < humanCarTotal; j++) {
            if (TM2_HUMAN_LOCAL(j) < 0)
                continue;
            delta[0] = aiCars[i].pos[0] - humanCars[j].pos[0];
            delta[1] = aiCars[i].pos[1] - humanCars[j].pos[1];
            delta[2] = aiCars[i].pos[2] - humanCars[j].pos[2];
            if (__builtin_abs(humanCars[j].pos[2] - aiCars[i].pos[2]) < 400 || (blink + i) % 5 == 0)
                WORLD_RADAR_PLOT(WORLD_RADAR_AI(i), delta, humanCars[j].rot[2], j);
        }
    }
}
#else
#define WORLD_RADAR_PLOT(type, delta, angle, h) hudRadarPlotCar(type, delta, angle, h)
#endif

#if TM2_MAX_HUMANS > 2
/* Port variant, more than two humans: human i on the radar of every other
 * human j (the z test of the pair). A function of its own in .port_text
 * (PC port request 29): inline in worldUpdate its locals grew the frame from
 * retail's 0xB0 to 0xB8, which moved every frame below worldUpdate. */
static void __attribute__((section(".port_text"))) worldRadarPlotHumans(s32 i)
{
    s32 delta[3];
    s32 j;

    for (j = 0; j < humanCarTotal; j++) {
        if (j == i)
            continue;
        delta[0] = humanCars[i].pos[0] - humanCars[j].pos[0];
        delta[1] = humanCars[i].pos[1] - humanCars[j].pos[1];
        delta[2] = humanCars[i].pos[2] - humanCars[j].pos[2];
        if (__builtin_abs(humanCars[i].pos[2] - humanCars[j].pos[2]) < 400 || rand() % 5 == 0)
            WORLD_RADAR_PLOT(WORLD_RADAR_HUMAN(i), delta, humanCars[j].rot[2], j);
    }
}
#endif

/* @8011A700 */
void __attribute__((section(".text_8011A700"))) worldUpdate(void)
{
    s32 delta[3];
    s32 target[3];
    s32 sound = -1;
    s32 i, j;

#ifdef TM2_PORT_CAR_SOUND
    portCarSoundFrame();    /* request 33-3: bit 4 latched, the engines allowed this update */
#endif
    if (gameGetLevel() == 8) {
#ifdef TM2_PORT_HAZARDS
        if (!portArenaTrainsPark())     /* request 38 G4: trains off = removed (parked outside the map) */
#endif
        worldTrackVehiclesUpdate();
        if (gameMode != 2)
            worldUpdateStub();
    }

    for (i = 0; i < humanCarTotal; i++) {
        TM2_HUMAN_SKIP_VACANT(i);   /* request 50: a vacant human record is not updated (nothing with no define) */
        if (!humanCarObjects[i])
            break;
        humanCarUpdate(humanCarObjects[i], &humanCars[i]);
        /* Taunt sound toward the other human. Matching notes: the own car
         * is indexed by i (folded to an absolute address after the i == N
         * test), the other car and the abs() operands by constant index,
         * and delta is written through *(delta + k) (volatile stores). */
#if TM2_MAX_HUMANS > 2
        /* Port variant, more than two humans: worldRadarPlotHumans (above).
         * Two or fewer: retail below (human 0 on radar 1, human 1 on radar 0,
         * even with one human). */
        if (humanCarTotal > 2) {
            if (humanCars[i].hp > 0)
                worldRadarPlotHumans(i);
        } else {
#endif
        if (i == 0 && humanCars[i].hp > 0) {
            *(delta + 0) = humanCars[i].pos[0] - humanCars[1].pos[0];
            *(delta + 1) = humanCars[i].pos[1] - humanCars[1].pos[1];
            *(delta + 2) = humanCars[i].pos[2] - humanCars[1].pos[2];
            if (__builtin_abs(humanCars[0].pos[2] - humanCars[1].pos[2]) < 400 || rand() % 5 == 0)
                WORLD_RADAR_PLOT(WORLD_RADAR_HUMAN(i), delta, humanCars[1].rot[2], 1);
        }
        if (i == 1 && humanCars[i].hp > 0) {
            *(delta + 0) = humanCars[i].pos[0] - humanCars[0].pos[0];
            *(delta + 1) = humanCars[i].pos[1] - humanCars[0].pos[1];
            *(delta + 2) = humanCars[i].pos[2] - humanCars[0].pos[2];
            if (__builtin_abs(humanCars[0].pos[2] - humanCars[1].pos[2]) < 400 || rand() % 5 == 0)
                WORLD_RADAR_PLOT(WORLD_RADAR_HUMAN(i), delta, humanCars[0].rot[2], 0);
        }
#if TM2_MAX_HUMANS > 2
        }
#endif
        carSoundUpdate(humanCarObjectSlots[i], &humanCars[i], 1);
        humanCarDamageUpdate(i);
        hudSmoothSpeed(i);
        /* Only this call is conditional; the camera target below is set
         * for every human car. */
#ifdef TM2_PORT_HUMANS
        worldHumanCamera(i);    /* H1: by local slot, none for a remote human */
#else
        if (viewMainPlayer == i || viewGetSplitMode())
            viewCarCameraUpdate(&humanCars[i], i);
#endif
        if (gameAttractDemoMode) {
            if (demoFollowPlayerCamera) {
                target[0] = humanCars->f_014;
                target[1] = humanCars->f_018;
            } else {
                target[0] = aiCars[viewWatchedAiCar].pos[0];
                target[1] = aiCars[viewWatchedAiCar].pos[1];
            }
        } else {
            target[0] = humanCars[i].pos[0];
            target[1] = humanCars[i].pos[1];
        }
        target[2] = 0;
#ifdef TM2_PORT_HUMANS
        worldHumanFollow(target, i);    /* H1 */
#else
        viewSetFollowObjPos(target, i);
#endif
    }

    if (gameAttractDemoMode) {
        worldDemoBlinkTimer++;
        if (worldDemoBlinkOn) {
            if (worldDemoBlinkTimer < 21) {
                fontSetColor(6, 36, 150, 36);
                fontPrintCenteredXY(6, 160, 40, worldDemoText);
                fontResetColor(6);
            } else {
                worldDemoBlinkOn = 0;
                worldDemoBlinkTimer = 0;
            }
        } else if (worldDemoBlinkTimer >= 20) {
            worldDemoBlinkOn = 1;
            worldDemoBlinkTimer = 0;
        }
        demoWatchTimer += (short)timerGetFrameTicks();
        demoRespawnTimer += (short)timerGetFrameTicks();
        if (demoRespawnTimer >= 601) {
            demoWatchTimer = 0;
            demoRespawnTimer = 0;
            demoRespawnAiCars();
            demoWatchNextAiCar();
        } else if (demoWatchTimer >= 301)
            demoWatchNextAiCar();
        if (aiCars[viewWatchedAiCar].f_82E >= 13)
            aiCars[viewWatchedAiCar].f_82E = 12;
        if (padAnyPressed(0))
            rtScheduleEvent(1, 0);
    } else {
        /* Declaration and init order set the register / stack slot
         * allocation (hunting and most_580_car are spilled). */
        s32 hunting;
        s32 most_00C_car, most_00C;
        s32 nearest_car, nearest;
        s32 most_580_car, most_580;
        s32 least_580_car, least_580;
        s32 attack_car, attack_distance;
        s32 near_count;
        s32 distance;

        hunting = 0;
        most_00C_car = -1; most_00C = 0;
        nearest_car = -1; nearest = -1;
        most_580_car = -1; most_580 = 0;
        least_580_car = -1; least_580 = 100;
        attack_car = -1; attack_distance = 0x7D00;
        near_count = 0;

        for (i = 0; i < aiCarTotal; i++) {
            if (aiCars[i].f_501 == 4)
                continue;
            if (most_00C < aiCars[i].f_00C) {
                most_00C = aiCars[i].f_00C;
                most_00C_car = i;
            }
            if (most_580 < aiCars[i].f_580) {
                most_580 = aiCars[i].f_580;
                most_580_car = i;
            }
            if ((aiCars[i].state == 1 || aiCars[i].state == 3) && aiCars[i].f_010 >= 241
                && aiCars[i].f_580 < least_580) {
                least_580 = aiCars[i].f_580;
                least_580_car = i;
            }
            distance = aiCars[i].distance;
            if (distance < 4800)
                near_count++;
            if (distance < nearest) {
                nearest = distance;
                nearest_car = i;
            }
            /* An if-chain (not a switch); the duplicated attack test is
             * merged by cross-jumping, as in retail. */
            if (aiCars[i].state == 2) {
                aiCars[i].f_00E += timerGetFrameTicks();
                distance = aiCars[i].distance;
                if (distance < attack_distance && aiCars[i].f_012 / 2 < aiCars[i].f_010) {
                    attack_distance = distance;
                    attack_car = i;
                }
            } else if (aiCars[i].state == 3) {
                if (distance < attack_distance && aiCars[i].f_012 / 2 < aiCars[i].f_010) {
                    attack_distance = distance;
                    attack_car = i;
                }
            } else if (aiCars[i].state == 1)
                hunting++;
            aiCars[i].closer = 0;
            for (j = 0; j < aiCarTotal; j++)
                if (i != j && !aiCars[j].f_06C && aiCars[i].distance > aiCars[j].distance)
                    aiCars[i].closer++;
            if (aiCars[i].closer < 3)
                aiCars[i].f_00C = 0;
            else
                aiCars[i].f_00C++;
        }
        if (most_00C_car >= 0)
            aiCars[most_00C_car].f_00C = 0;
        if (hunting >= 4 || near_count >= 4) {
            if (least_580_car >= 0) {
                aiCars[least_580_car].state = 2;
                aiCars[least_580_car].f_00E = 0;
                aiCars[least_580_car].f_012 = aiCars[least_580_car].f_584;
                if (rand() & 1) {
                    aiCars[least_580_car].state = 3;
                    aiCars[least_580_car].f_012 = aiCars[least_580_car].f_586;
                }
                aiCars[least_580_car].f_010 = 0;
            }
        } else if (near_count <= 0) {
            if (nearest_car >= 0 && aiCars[nearest_car].state == 1)
                attack_car = -1;
            if (attack_car >= 0) {
                aiCars[attack_car].state = 0;
                aiCars[attack_car].f_010 = 0;
                aiCars[attack_car].f_012 = 600;
                aiCars[attack_car].f_588 = aiCars[attack_car].hp * aiCars[attack_car].f_58A / 100;
            }
        }
        if (hunting <= 0 && most_580_car >= 0) {
            aiCars[most_580_car].state = 1;
            aiCars[most_580_car].f_012 = 3600;
            aiCars[most_580_car].f_010 = 0;
            aiCars[most_580_car].f_588 = 100;
        }
    }

    for (i = 0; i < aiCarTotal; i++) {
        if (aiCarShouldUpdate(i)) {
            aiCars[i].f_59C = 1;
            aiCarUpdate(aiCarObjects[i], &aiCars[i]);
            aiCars[i].f_00C = 0;
        } else {
            aiCars[i].f_59C = 0;
            aiCars[i].f_00C++;
        }
        if (aiCars[i].f_034 || !gameMode)
            carSoundUpdate(aiCarObjectSlots[i], &aiCars[i], 0);
        else {
            switch (aiCars[i].f_030) {
            case 0x00: break;   /* no code; only shapes the compare tree */
            case 0x10: sound = 0x3C; break;
            case 0x20: sound = 0x24; break;
            case 0x30: sound = 0x26; break;
            case 0x40: sound = 0x28; break;
            case 0x50: sound = 0x2A; break;
            case 0x60: sound = 0x2C; break;
            case 0x70: sound = 0x2E; break;
            case 0x80: sound = 0x30; break;
            case 0x90: sound = 0x32; break;
            case 0xA0: sound = 0x34; break;
            case 0xB0: sound = 0x36; break;
            case 0xC0: sound = 0x38; break;
            case 0xD0: sound = 0x3A; break;
            case 0xE0: sound = 0x41; break;
            }
#ifdef TM2_PORT_CAR_SOUND
            /* request 33-3: this car's own engine instance (bit 4 clear: retail's stop of `sound`) */
            portCarEngineUpdate(aiCars[i].slot, sound, 0, 0, 0, 0);
#else
            if (sound != -1 && soundIsActive(sound))
                soundStop(sound);
#endif
        }
        if (aiCars[i].hp > 0) {
            delta[0] = aiCars[i].pos[0] - humanCars[0].pos[0];
            delta[1] = aiCars[i].pos[1] - humanCars[0].pos[1];
            delta[2] = aiCars[i].pos[2] - humanCars[0].pos[2];
            if (__builtin_abs(humanCars[0].pos[2] - aiCars[i].pos[2]) < 400 || rand() % 5 == 0)
                WORLD_RADAR_PLOT(WORLD_RADAR_AI(i), delta, humanCars[0].rot[2], 0);
            if (gameMode == 2) {
                delta[0] = aiCars[i].pos[0] - humanCars[1].pos[0];
                delta[1] = aiCars[i].pos[1] - humanCars[1].pos[1];
                delta[2] = aiCars[i].pos[2] - humanCars[1].pos[2];
                if (__builtin_abs(humanCars[1].pos[2] - aiCars[i].pos[2]) < 400 || rand() % 5 == 0)
                    WORLD_RADAR_PLOT(WORLD_RADAR_AI(i), delta, humanCars[1].rot[2], 1);
            }
#if TM2_MAX_HUMANS > 2
            /* port variant: the AI blips of co-op (mode 2) on the radars of humans 3-4 too */
            if (gameMode == 2) {
                for (j = 2; j < humanCarTotal; j++) {
                    delta[0] = aiCars[i].pos[0] - humanCars[j].pos[0];
                    delta[1] = aiCars[i].pos[1] - humanCars[j].pos[1];
                    delta[2] = aiCars[i].pos[2] - humanCars[j].pos[2];
                    if (__builtin_abs(humanCars[j].pos[2] - aiCars[i].pos[2]) < 400 || rand() % 5 == 0)
                        WORLD_RADAR_PLOT(WORLD_RADAR_AI(i), delta, humanCars[j].rot[2], j);
                }
            }
#endif
        }
        aiCarDamageUpdate(i);
    }
#ifdef TM2_PORT_HUMANS
    worldRadarAiAll();      /* H2: portHumanOptions bit 4 (drawing only, no rand()) */
#endif
    aiCarsCountRemaining();

#ifdef TM2_PORT_HUMANS
    /* H5: with portHumanOptions bit 1 the port's rules end the round (portRoundEnd), not human 0's survival */
    if (!portHumanRules() && aiCarsRemaining <= 0 && gameMode != 4 && !humanCars[0].f_09C && !roundWonFlag) {
#else
    if (aiCarsRemaining <= 0 && gameMode != 4 && !humanCars[0].f_09C && !roundWonFlag) {
#endif
        roundWonFlag = 1;
        soundStopSharedLoop(9, -1);
#ifdef TM2_PORT_DARK_TOOTH
        /* request 33-2e: aiCars[0]'s stage flag and head (retail's rule; CAR_GROUPS.md 11) */
        if (portCarDarkToothRoundEnd())
            rtScheduleEvent(3, 210);
        else
            rtScheduleEvent(3, 420);
#else
        if (aiBossDefeatedFlag) {
            aiDarkToothBossObject()[2] = 0;
            rtScheduleEvent(3, 210);
        } else
            rtScheduleEvent(3, 420);
#endif
        roundOverFlag = 1;
    }
    pickupRespawnTick();
    carsPhysicsUpdate();
    carPairsUpdate();
#ifdef TM2_PORT_ROLLBACK
    /* request 48 stage 3 SOUND_GATE (ONLINE_ROLLBACK.md 1.2 "CD", 12): the CD player's track-ended flag is consumed by
       the live step only (a re-run neither takes it nor switches the music) */
    if (!portRollbackSoundOff() && audioGetDarkToothMusic() && audioTakeTrackEnded())
        audioAlternateMusicTrack();
#else
    if (audioGetDarkToothMusic() && audioTakeTrackEnded())
        audioAlternateMusicTrack();
#endif
    GetCurTics();
    for (i = 0; i < aiCarTotal; i++)
        if (aiCars[i].f_06C || !aiCars[i].f_034)
            soundStopSharedLoop(9, aiCars[i].f_004);
    if (soundGetDeniedPending()) {
        soundSetDeniedPending(0);
        soundPlay(0x3F, 0, 0, 99);
    }
    soundUpdate();
    soundFlush();
    GetCurTics();
    groundFiresUpdate();
    weaponRicochetUpdate();
    weaponShieldsUpdate();
    weaponInvisibilityUpdate();
    weaponTornadoUpdate();
    weaponHomingLeapUpdate();
    weaponShadowStrikeUpdate();
    worldWarpsUpdate();
#ifdef TM2_PORT_HAZARDS
    if (gameGetLevel() == 6 && !portArenaOff(TM2_ARENA_ICE))   /* request 38 G1: the ice holds */
#else
    if (gameGetLevel() == 6)
#endif
        worldCollapseUpdate();
    nullsub_8();
}
