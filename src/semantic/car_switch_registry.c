/* Car-choice registry: tables of choice objects (type 9) per car group (id>>4), tire-state slots,
   and per-index display objects (types 1/4). Choice object: +0 type, +6 state count, +7 current state. */
typedef signed char s8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;

extern u8 *carDamageStageObjects[16];
extern u8 *carTireSwitchObjects[16][3];
extern u8 *pickupObjects[50];
#include "semantic/car_groups.h"

/* register owner's (+284) node's choice object (type 9) for group id>>4 and reset its state */
void __attribute__((section(".text_801246A8"))) carChoiceRegisterGroup(u8 *owner, s32 id)
{
    u8 *node = *(u8 **)(owner + 284);
    u8 *choice;
    s32 index;

    if (node != 0 && node[0] == 1) {
        choice = *(u8 **)(node + 24);
        if (choice != 0 && choice[0] == 9) {
            index = id >> 4;
#ifdef TM2_PORT_CAR_GROUPS
            /* request 32 (CAR_GROUPS.md): id is the slot's car key (arenaSetup) */
            if ((u32)index < TM2_CAR_KEYS) {
                portCarGroups.damage_stage[index] = choice;
                choice[7] = 0;
            }
#else
            if (index < 16) {
                carDamageStageObjects[index] = choice;
                choice[7] = 0;
            }
#endif
        }
    }
}

/* set group id>>4's choice state (2 maps to 1, 3 maps to 2) if within the state count */
void __attribute__((section(".text_80124718"))) carSetDamageStage(s32 id, u32 state, s32 arg /* unused; every retail caller passes it */)
{
    s32 index;
    u8 **slot;
    u8 **table;

    if (state == 2)
        state = 1;
    index = id >> 4;
    if (state == 3)
        state = 2;
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): id is a car key */
    if ((u32)index < TM2_CAR_KEYS) {
        table = (u8 **)portCarGroups.damage_stage;
#else
    if (index < 16) {
        table = carDamageStageObjects;
#endif
        slot = &table[index];
        if (*slot != 0 && state < (*slot)[6])
            (*slot)[7] = state;
    }
}

/* register a type-9 choice object in tire slot [group = +2 >> 4][+4 - 1] */
void __attribute__((section(".text_8012478C"))) carRegisterTireSwitch(u8 *object)
{
    s32 group;
    u32 slot;

    if (object != 0 && object[0] == 9) {
        group = *(u16 *)(object + 2) >> 4;
        if (group < 16) {
            slot = *(u16 *)(object + 4) - 1;
#ifdef TM2_PORT_CAR_GROUPS
            /* request 32 (CAR_GROUPS.md): into the slot of the load group's key */
            if (portCarLoadKey())
                group = portCarLoadKey() >> 4;
            if (slot < 3 && group < TM2_CAR_KEYS)
                portCarGroups.tire[group][slot] = object;
#else
            if (slot < 3)
                carTireSwitchObjects[group][slot] = object;
#endif
        }
    }
}

/* step all three tire-slot choices of a group forward (direction >= 0, wrapping to 0) or back (clamping) */
void __attribute__((section(".text_80124800"))) uaswToggleCarTireState(s32 group, s32 direction)
{
    s32 i;
    u8 *choice;

#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): group is a car key index */
    if ((u32)group < TM2_CAR_KEYS) {
        for (i = 0; i < 3; i++) {
            choice = (u8 *)portCarGroups.tire[group][i];
#else
    if (group < 16) {
        for (i = 0; i < 3; i++) {
            choice = carTireSwitchObjects[group][i];
#endif
            if (choice != 0) {
                if (direction < 0) {
                    choice[7]--;
                    if (choice[7] >= choice[6])
                        choice[7] = choice[6] - 1;
                } else {
                    choice[7]++;
                    if (choice[7] >= choice[6])
                        choice[7] = 0;
                }
            }
        }
    }
}

/* register a display object (type 1: index at +22, type 4: index at +16) for ids 401..430 */
void __attribute__((section(".text_801248C8"))) pickupRegisterObject(u8 *object, s32 id)
{
    s32 index;

    if ((u32)(id - 401) < 30) {
        if (object[0] == 1)
            index = *(u16 *)(object + 22);
        else if (object[0] == 4)
            index = *(u16 *)(object + 16);
        else
            return;
        if (index < 50)
            pickupObjects[index] = object;
    }
}

/* set a value on display object [index] for ids 401..430 (type 1 -> +20, type 4 -> +18) */
extern void pickupModelSetVisible(u8 *object, s32 value);
extern void pickupSpriteSetVisible(u8 *object, s32 value);
void __attribute__((section(".text_8012493C"))) pickupSetVisible(s32 id, s32 index, s32 value)
{
    u8 **slot;
    u8 **table;

    if ((u32)(id - 401) < 30) {
        table = pickupObjects;
        slot = &table[index];
        if ((*slot)[0] == 1) {
            if (index < 50)
                pickupModelSetVisible(*slot, value);
        } else if ((*slot)[0] == 4) {
            if (index < 50)
                pickupSpriteSetVisible(*slot, value);
        }
    }
}

/* object[20] = value (null-safe) */
void __attribute__((section(".text_801249D8"))) pickupModelSetVisible(u8 *object, s32 value)
{
    if (object)
        object[20] = value;
}

/* object[18] = value (null-safe) */
void __attribute__((section(".text_801249EC"))) pickupSpriteSetVisible(u8 *object, s32 value)
{
    if (object)
        object[18] = value;
}
