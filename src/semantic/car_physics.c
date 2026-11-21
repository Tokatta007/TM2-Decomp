/* Per-frame car physics (8011E220): the "humans, then enemies" loops.
 *
 * Every step runs over the human cars (humanCars, count humanCarTotal,
 * human = 1) and then the AI cars (aiCars, count aiCarTotal,
 * human = 0); the record layouts differ (see car_motion.c), so each pass
 * is written out twice. A port with one car layout can run each step over
 * all cars:
 *   1. clear the six contact touch flags;
 *   2. for cars updated this frame: probe the ground (hdpCollideObject) and
 *      test the contact points (carTestContactPoints);
 *   3. dynamics (carContactsCombine); AI cars whose body is turned more than
 *      60 degrees from their heading get flag +0x3F;
 *   4. state f02 set: carContactRespond; otherwise, after 3 frames of contact,
 *      save velocity, position, body angles and the rotation matrix;
 *      then carContactPushUpdate. */
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

/* Contact block (human +0x234, AI +0xD8). */
typedef struct {
    u8 pad_000[2];
    u8 f02;                 /* +0x002 */
    u8 pad_003[3];
    s16 f06;                /* +0x006 */
    u8 pad_008[8];
    s16 contact_frames;     /* +0x010 */
    u8 pad_012[6];
    u8 ground[0x25C];       /* +0x018 hdpCollideObject output */
    struct { u8 touch; u8 pad[0x23]; } point[6];   /* +0x274 */
} TM2Contact;

/* Motion block (human +0x6DC, AI +0x71C), extended view of car_motion.c. */
typedef struct {
    u8 pad_00[8];
    s16 vel[3];             /* +0x08 */
    u8 pad_0E[0x0A];
    s16 saved_vel[3];       /* +0x18 */
    u8 pad_1E[2];
    s32 pos[3];             /* +0x20 */
    s32 rot[3];             /* +0x2C */
    s32 body_rot[3];        /* +0x38 */
    u8 pad_44[0x0C];
    TM2Matrix rot_m;        /* +0x50 */
    u8 pad_70[0x40];
    s32 saved_pos[3];       /* +0xB0 */
    s32 saved_body_rot[3];  /* +0xBC */
    TM2Matrix saved_m;      /* +0xC8 */
} TM2PhysMotion;

typedef struct {
    u8 pad_000[0x1EC];
    s32 updated;            /* +0x1EC */
    u8 pad_1F0[0x44];
    TM2Contact ct;          /* +0x234 */
    u8 pad_580[0x15C];
    TM2PhysMotion mot;      /* +0x6DC */
    u8 pad_7C4[0x50];
} TM2PhysHuman;
typedef char tm2_phys_human_size[(sizeof(TM2PhysHuman) == 0x814) ? 1 : -1];

typedef struct {
    u8 pad_000[0x3F];
    u8 spun;                /* +0x03F */
    u8 pad_040[0x98];
    TM2Contact ct;          /* +0x0D8 */
    u8 pad_424[0x178];
    s32 updated;            /* +0x59C */
    u8 pad_5A0[0x17C];
    TM2PhysMotion mot;      /* +0x71C */
    u8 pad_804[0x50];
} TM2PhysAI;
typedef char tm2_phys_ai_size[(sizeof(TM2PhysAI) == 0x854) ? 1 : -1];

#include "semantic/players.h"
extern TM2PhysHuman humanCars[TM2_MAX_HUMANS];
extern TM2PhysAI aiCars[TM2_MAX_AI_CARS];
extern s32 humanCarTotal;              /* human cars */
extern s32 aiCarTotal;              /* AI cars */
extern void *humanCarObjects[TM2_MAX_HUMANS];         /* human car world objects */
extern void *aiCarObjects[TM2_MAX_AI_CARS];         /* AI car world objects */

extern void hdpCollideObject(void *object, void *out, s32, void *, void *);
extern void carTestContactPoints(void *car, s32 human);
extern void carContactsCombine(void *car, s32 human);
extern void carContactRespond(void *car, s32 human);
extern void carContactPushUpdate(void *car, s32 human);
extern void mathCopyWords(void *dst, void *src, s32 words);

/* @8011E220 */
void __attribute__((section(".text_8011E220"))) carsPhysicsUpdate(void)
{
    s32 i, k;

    for (i = 0; i < humanCarTotal; i++)
        for (k = 0; k < 6; k++)
            humanCars[i].ct.point[k].touch = 0;
    for (i = 0; i < aiCarTotal; i++)
        for (k = 0; k < 6; k++)
            aiCars[i].ct.point[k].touch = 0;

    for (i = 0; i < humanCarTotal; i++) {
        if (humanCars[i].updated) {
            hdpCollideObject(humanCarObjects[i], humanCars[i].ct.ground, 1,
                         &humanCars[i].mot.saved_m, humanCars[i].mot.saved_pos);
            carTestContactPoints(&humanCars[i], 1);
        }
    }
    for (i = 0; i < aiCarTotal; i++) {
        if (aiCars[i].updated) {
            hdpCollideObject(aiCarObjects[i], aiCars[i].ct.ground, 1,
                         &aiCars[i].mot.saved_m, aiCars[i].mot.saved_pos);
            carTestContactPoints(&aiCars[i], 0);
        }
    }

    for (i = 0; i < humanCarTotal; i++)
        carContactsCombine(&humanCars[i], 1);
    for (i = 0; i < aiCarTotal; i++) {
        carContactsCombine(&aiCars[i], 0);
        if (aiCars[i].ct.f06 > 0
            && __builtin_abs(aiCars[i].mot.rot[2] - aiCars[i].mot.body_rot[2]) >= 683)
            aiCars[i].spun = 1;
    }

    for (i = 0; i < humanCarTotal; i++) {
        if (humanCars[i].ct.f02)
            carContactRespond(&humanCars[i], 1);
        else if (humanCars[i].ct.contact_frames >= 3) {
            humanCars[i].mot.saved_vel[0] = humanCars[i].mot.vel[0];
            humanCars[i].mot.saved_vel[1] = humanCars[i].mot.vel[1];
            humanCars[i].mot.saved_vel[2] = humanCars[i].mot.vel[2];
            humanCars[i].mot.saved_pos[0] = humanCars[i].mot.pos[0];
            humanCars[i].mot.saved_pos[1] = humanCars[i].mot.pos[1];
            humanCars[i].mot.saved_pos[2] = humanCars[i].mot.pos[2];
            humanCars[i].mot.saved_body_rot[0] = humanCars[i].mot.body_rot[0];
            humanCars[i].mot.saved_body_rot[1] = humanCars[i].mot.body_rot[1];
            humanCars[i].mot.saved_body_rot[2] = humanCars[i].mot.body_rot[2];
            mathCopyWords(&humanCars[i].mot.saved_m, &humanCars[i].mot.rot_m, 8);
        }
        carContactPushUpdate(&humanCars[i], 1);
    }
    for (i = 0; i < aiCarTotal; i++) {
        if (aiCars[i].ct.f02)
            carContactRespond(&aiCars[i], 0);
        else if (aiCars[i].ct.contact_frames >= 3) {
            aiCars[i].mot.saved_vel[0] = aiCars[i].mot.vel[0];
            aiCars[i].mot.saved_vel[1] = aiCars[i].mot.vel[1];
            aiCars[i].mot.saved_vel[2] = aiCars[i].mot.vel[2];
            aiCars[i].mot.saved_pos[0] = aiCars[i].mot.pos[0];
            aiCars[i].mot.saved_pos[1] = aiCars[i].mot.pos[1];
            aiCars[i].mot.saved_pos[2] = aiCars[i].mot.pos[2];
            aiCars[i].mot.saved_body_rot[0] = aiCars[i].mot.body_rot[0];
            aiCars[i].mot.saved_body_rot[1] = aiCars[i].mot.body_rot[1];
            aiCars[i].mot.saved_body_rot[2] = aiCars[i].mot.body_rot[2];
            mathCopyWords(&aiCars[i].mot.saved_m, &aiCars[i].mot.rot_m, 8);
        }
        carContactPushUpdate(&aiCars[i], 0);
    }
}
