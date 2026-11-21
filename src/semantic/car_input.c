/* Human car input (800DE5F8): reads the player's bound actions into the
 * car's one-byte input flags (car+0xD0..0xEE), combines them, then runs
 * carReadMoveCodes. Actions are looked up through padCheckAction (controls.c). */

#include "semantic/cars.h"   /* u8..u32, car record blocks */
#include "semantic/car_groups.h"

/* Human car record (0x814 bytes), seen from its input fields. */
struct TM2HumanControl {
    s16 player;             /* +0x000 */
    u8 pad_002[0x2];
    s16 slot;               /* +0x004 car slot */
    u8 pad_006[0x8A];
    s32 type;               /* +0x090 car type (0x10..0xF0) */
    u8 pad_094[0xE];
    u8 steering;            /* +0x0A2 */
    u8 pad_0A3[0x5];
    u8 f_0A8;               /* +0x0A8 */
    u8 f_0A9;               /* +0x0A9 */
    u8 pad_0AA[0x4];
    u8 f_0AE;               /* +0x0AE */
    u8 pad_0AF[0x6];
    u8 f_0B5;               /* +0x0B5 */
    u8 pad_0B6[0x2];
    u8 f_0B8;               /* +0x0B8 */
    u8 f_0B9;               /* +0x0B9 */
    u8 pad_0BA[0x1];
    u8 f_0BB;               /* +0x0BB */
    u8 pad_0BC[0x14];
    u8 in_D0, in_D1, in_D2, in_D3, in_D4, in_D5, in_D6, in_D7;  /* +0x0D0 */
    u8 in_D8, in_D9, in_DA, in_DB, in_DC, in_DD, in_DE, in_DF;  /* +0x0D8 */
    u8 in_E0, in_E1, in_E2, in_E3, in_E4, in_E5, in_E6, in_E7;  /* +0x0E0 */
    u8 in_E8, in_E9, in_EA, in_EB, in_EC, in_ED, in_EE;         /* +0x0E8 */
    u8 pad_0EF[0x101];
    s32 hp;                 /* +0x1F0 */
    u8 pad_1F4[0xC];
    s32 f_200;              /* +0x200 */
    u8 pad_204[0x3A];
    s16 f_23E;              /* +0x23E */
    u8 pad_240[0x4A6];
    s16 speed;              /* +0x6E6 */
    u8 pad_6E8[0x112];
    s16 f_7FA;              /* +0x7FA */
    u8 pad_7FC[0x18];
};
typedef char tm2_human_control_size[(sizeof(struct TM2HumanControl) == 0x814) ? 1 : -1];

typedef struct TM2HumanControl Car;

#include "semantic/input.h"
extern void aiClearControls(u8 *inputs);
void carReadMoveCodes(void *car);
#include "semantic/world.h"

#include "semantic/port_humans.h"   /* H3: TM2_HUMAN_MESSAGE / SOUND (retail's calls without the define) */
#ifdef TM2_PORT_HUMANS
/* HUMANS16_PLAN.md H2: human `player` reads pad 0/1 by its local slot (retail's call), or its input word */
#define ACTION(a) portHumanAction((u8)(a), (s16)player)
#else
#define ACTION(a) padCheckAction((u8)(a), (s16)player)   /* retail narrows both */
#endif

#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxSmokeC6(void *, u8 flag);

/* @800DE4E8: one of the shared car helpers that take (record, isHuman):
 * the field blocks sit at different offsets in human and AI records. Sets
 * flags+0x18 (and spawns two effects) while the car is fast enough. */
void __attribute__((section(".text_800DE4E8"))) carSkidSmokeUpdate(void *car, u8 human)
{
    struct TM2CarState *flags;
    struct TM2CarStatsBlock *geometry;
    struct TM2CarMotion *motion;
    struct TM2CarWheels *state;
    if (human) {
        flags = &HUMAN->state;
        geometry = &HUMAN->stats;
        motion = &HUMAN->motion;
        state = &HUMAN->wheels;
    } else {
        flags = &AI->state;
        geometry = &AI->stats;
        motion = &AI->motion;
        state = &AI->wheels;
    }
    if (!flags->wheel_in_air && (flags->f01 || (human && flags->f1C))) {
        if (__builtin_abs(motion->speed) > geometry->reverse_cap / 2) {
            fxSmokeC6(mathJitterPos(state->wheel[2].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC6(mathJitterPos(state->wheel[3].pos, 0x555, 0x555, 0x555), 0);
            flags->sliding = 1;
        }
        return;
    }
    flags->sliding = 0;
}

/* @800DE5F8 */
void __attribute__((section(".text_800DE5F8"))) humanCarReadInput(Car *car)
{
    s32 player = car->player;
    u8 flag;

    if (car->hp <= 0 || car->f_0BB)
        aiClearControls(&car->in_D0);
    else {
        car->in_D4 = ACTION(8);
        car->in_D5 = ACTION(9);
        car->in_D7 = ACTION(0x31);
        car->in_D9 = ACTION(0x39);
        car->in_DA = ACTION(0x3A);
        car->in_DB = ACTION(0x3B);
        car->in_E6 = ACTION(0x3C);
        car->in_D8 = 0;
        /* The view hotkeys need action 9 held as well. */
        car->in_D7 = car->in_D7 && ACTION(9);
        car->in_D9 = car->in_D9 && ACTION(9);
        car->in_DA = car->in_DA && ACTION(9);
        car->in_DB = car->in_DB && ACTION(9);
        car->in_E6 = car->in_E6 && ACTION(9);

        car->in_D3 = ACTION(0x1D) && !car->f_0A9 && !car->f_0B9;
        car->in_E1 = ACTION(0x18);
        car->in_DE = ACTION(0x1B);
        car->in_DE = car->in_DE && !car->in_D4 && !car->in_D5;

        flag = 0;
        if (ACTION(0x19) || (car->in_E1 && car->in_DE) || (car->in_DE && car->speed > 0))
            flag = 1;
        car->in_E3 = flag;

        flag = 0;
        if (car->in_E1 || ACTION(0x1A) || car->in_DE)
            flag = 1;
        car->in_E4 = flag;
        car->in_E4 = car->in_E4 && !car->in_D4 && !car->in_D5;
        car->in_E2 = car->in_E4 && !car->f_0A9 && !car->f_0AE && !car->f_0A8 && !car->f_0B9;

        car->in_E0 = ACTION(0x16);
        car->in_DF = ACTION(0x17);
        car->in_D6 = car->f_200 < __builtin_abs(car->speed) && car->in_E3;
        if (car->in_D6 && (car->in_E0 || car->in_DF)) {
            car->in_E3 = 0;
            car->in_E1 = 0;
        }
        car->in_D0 = ACTION(0x1C) && car->in_E0;
        car->in_D1 = ACTION(0x1C) && car->in_DF;
        car->in_D2 = car->in_D0 || car->in_D1;
        car->steering = car->in_E0 || car->in_DF;

        if (!car->f_0B5) {
            if (car->f_23E >= 181)
                car->f_0B5 = 1;
            else if (car->f_23E >= 31) {
                if (car->steering || car->in_E2 || car->f_0B8)
                    car->f_0B5 = 1;
            } else if (car->f_23E >= 16) {
                if (car->in_E2 || car->f_0B8)
                    car->f_0B5 = 1;
            } else if (car->f_23E >= 8) {
                if (car->f_0B8)
                    car->f_0B5 = 1;
            }
        }

        car->in_E7 = ACTION(0x21);
        car->in_E8 = ACTION(0x20);
        car->in_EA = ACTION(0x1F);
        car->in_EB = ACTION(0x1E);
        carReadMoveCodes(car);
        car->in_EE = ACTION(0x1F);
    }

    if (car->in_D3 && car->f_7FA > 0) {
        if (!car->f_0B8) {
            car->f_0B8 = 1;
#ifdef TM2_PORT_CAR_GROUPS
            portCarTurboSetStep(TM2_CAR_KEY(car->slot, car->type), 1);   /* request 32 */
#else
            objRegistrySetStep(car->type + 6, 0, 1);
#endif
        }
    } else if (car->f_0B8) {
        car->f_0B8 = 0;
#ifdef TM2_PORT_CAR_GROUPS
        portCarTurboSetStep(TM2_CAR_KEY(car->slot, car->type), 0);   /* request 32 */
#else
        objRegistrySetStep(car->type + 6, 0, 0);
#endif
    }
}

/* Human car record (0x814 bytes), seen from its special-move fields. */
struct TM2HumanCombo {
    s16 player;             /* +0x000 */
    u8 pad_002[0xE8];
    u8 in_EA;               /* +0x0EA */
    u8 pad_0EB[0x1];
    u8 in_EC;               /* +0x0EC */
    u8 pad_0ED[0x3];
    u8 armed;               /* +0x0F0 no direction held: next press counts */
    s8 count;               /* +0x0F1 presses recorded */
    s8 timer;               /* +0x0F2 frames since the last press */
    u8 pad_0F3[0x1];
    s32 code;               /* +0x0F4 presses, one nibble each, first press highest */
    u8 pad_0F8[0x8E];
    s16 max_hp;             /* +0x186 */
    u8 pad_188[0x68];
    s32 hp;                 /* +0x1F0 */
    s32 f_1F4;              /* +0x1F4 */
    u8 pad_1F8[0x5D0];
    s32 f_7C8;              /* +0x7C8 */
    u8 pad_7CC[0x1];
    u8 f_7CD;               /* +0x7CD */
    u8 pad_7CE[0x2];
    u8 f_7D0;               /* +0x7D0 */
    u8 f_7D1;               /* +0x7D1 */
    u8 pad_7D2[0x2];
    u8 special;             /* +0x7D4 special move to fire */
    s8 f_7D5;               /* +0x7D5 */
    s16 f_7D6[8];           /* +0x7D6 */
    u8 pad_7E6[0x4];
    s16 f_7EA;              /* +0x7EA */
    u8 pad_7EC[0x28];
};
typedef char tm2_human_combo_size[(sizeof(struct TM2HumanCombo) == 0x814) ? 1 : -1];

extern void hudMessagePost(s32, s32);
#include "semantic/sound.h"

/* Direction actions and their code nibbles. */
#define STEP(action, nibble)                                            if (ACTION(action)) {                                                   car->code |= (nibble) << ((7 - count) * 4);                         ++car->count;                                                       car->timer = 0;                                                 }
#define MIN30(x) ((x) > 30 ? 30 : (x))

/* @800DECA8: special moves. Each new direction press (actions 0x1A, 0x1B,
 * 0x17, 0x16 -> nibbles 1..4) is appended to car->code; after at least three
 * presses a complete code fires its move. The record resets after eight
 * presses or eight idle frames. */
void __attribute__((section(".text_800DECA8"))) carReadMoveCodes(void *p)
{
    struct TM2HumanCombo *car = p;
    s32 player = car->player;
    s32 count = 0;

    if (car->armed) {
        count = car->count;
        car->armed = 0;
        STEP(0x1A, 1)
        STEP(0x1B, 2)
        STEP(0x17, 3)
        STEP(0x16, 4)
        if (car->count >= 8) {
            car->count = 0;
            car->timer = 0;
        }
    }
#ifdef TM2_PORT_HUMANS
    /* H2 (request 37 answer 13): with the cheats off (portHumanOptions bit 3) a completed cheat code is consumed
       with no effect, no message and no sound */
    if (count >= 3 && portHumanCheatCode(car->code)) {
        car->count = 0;
        car->code = 0;
        count = 0;
    }
#endif
    /* H3 (request 37 answer 7): the moves' HUD messages and jingles below are the player's own feedback, shown
       and heard only where the human is local (TM2_HUMAN_MESSAGE / TM2_HUMAN_SOUND) */
    if (count >= 3) {
        switch (car->code) {
        case 0x34100000:
            car->special = 100;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x43200000:
            car->special = 101;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x43100000:
            car->special = 102;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x11400000:
            car->special = 103;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x11300000:
            car->special = 104;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x42310000:
            car->special = 105;
            car->in_EC = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x34200000:
            car->f_7CD = 1;
            car->in_EA = 1;
            car->count = 0;
            car->code = 0;
            break;
        case 0x12233344:
            if (car->f_7D5 == 5 && ACTION(0x1F) && !car->f_7D1) {
                car->f_7D1 = 1;
                car->f_7D6[5] += 10;
                TM2_HUMAN_MESSAGE(player, 0x1F, 0);
                TM2_HUMAN_SOUND(player, 0x18, 0, 0, 99);
            }
            car->count = 0;
            car->code = 0;
            break;
        case 0x12110000:
            if (ACTION(0x1E)) {
                car->special = 106;
                car->in_EC = 1;
                car->count = 0;
                car->code = 0;
            }
            break;
        case 0x21431122: {
            s32 i;
            s32 bonus = MIN30((car->f_7D6[0] + car->f_7EA) * 3);
            car->hp += bonus;
            car->hp = car->hp < car->max_hp ? car->hp : car->max_hp;
            for (i = 0; i < 8; i++)
                car->f_7D6[i] = 0;
            car->f_7EA = 0;
            car->count = 0;
            car->code = 0;
            TM2_HUMAN_MESSAGE(player, 0x21, 0);
            TM2_HUMAN_SOUND(player, 0x18, 0, 0, 99);
            break;
        }
        case 0x12344321:
            if (ACTION(0x21) && ACTION(0x20)) {
                if (car->f_1F4) {
                    car->f_1F4 = 0;
                    if (!ACTION(0x1F) && !ACTION(0x1E))
                        TM2_HUMAN_MESSAGE(player, 0x19, 0);
                } else {
                    car->f_1F4 = 1;
                    if (!ACTION(0x1F) && !ACTION(0x1E))
                        TM2_HUMAN_MESSAGE(player, 0x1D, 0);
                }
            }
            if (ACTION(0x1F) && ACTION(0x1E)) {
                if (car->f_7C8) {
                    car->f_7C8 = 0;
                    if (!ACTION(0x21) && !ACTION(0x20))
                        TM2_HUMAN_MESSAGE(player, 0x1E, 0);
                } else {
                    car->f_7C8 = 1;
                    if (!ACTION(0x21) && !ACTION(0x20))
                        TM2_HUMAN_MESSAGE(player, 0x1A, 0);
                }
            }
            if (ACTION(0x1E) && !ACTION(0x1F)) {
                car->f_7D0 = 1;
                TM2_HUMAN_MESSAGE(player, 0x18, 0);
            }
            if (ACTION(0x21) && ACTION(0x20) && ACTION(0x1F) && ACTION(0x1E)) {
                s32 sound = 0x1B;
                if (!car->f_1F4 && car->f_7C8)
                    sound = 0x17;
                TM2_HUMAN_MESSAGE(player, sound, 0);
            }
            car->count = 0;
            car->code = 0;
            TM2_HUMAN_SOUND(player, 0x18, 0, 0, 99);
            break;
        }
    }
    if (!ACTION(0x1A) && !ACTION(0x1B) && !ACTION(0x16) && !ACTION(0x17))
        car->armed = 1;
    if (car->count) {
        if (car->timer++ >= 8) {
            car->count = 0;
            car->code = 0;
            car->f_7CD = 0;
            car->timer = 0;
        }
    }
}
