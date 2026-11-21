/* Object registry: named object slots, by object id.
 * Objects carry their type (+0, 9 = registrable) and slot index (+4). */
typedef unsigned char u8;typedef unsigned short u16;typedef unsigned int u32;typedef signed int s32;

typedef struct TM2RegObject { u8 type; u8 pad1[3]; u16 index; } TM2RegObject;

extern TM2RegObject *carTurboObjects[],*D_8018F624[],*objRegPedestrians[],*objRegSplashAnims[],*objRegLavaSplashAnims[],*objRegBlastAnims[],*objRegTargetObjects[],*objRegMoscowEvents[],*objRegLaEvents[],*objRegMoscowDishEmitters[];
extern TM2RegObject *objRegistryId2324,*objRegistryId2325,*objRegistryId689,*objRegistryId3103,*regObject669,*regObject3004,*regObject3005,*regObject3101,*regObject3200;
extern s32 regObjectListCount;
#include "semantic/world.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"

/* The ids 500..510 and 519..520 are appended to a list of up to 50 objects. */
#define APPEND_CASES case 500: case 501: case 502: case 503: case 504: case 505: case 506: case 507: case 508: case 509: case 510: case 519: case 520

#define SET(slot) if (object->type == 9) slot = object; break
/* @80123F78 */
void __attribute__((section(".text_80123F78"))) objRegistryAdd(TM2RegObject *object,s32 id)
{
    s32 count;
    if (id >= 16 && id < 400) {
#ifdef TM2_PORT_CAR_GROUPS
        /* request 32 (CAR_GROUPS.md): a car's turbo object (+6, ids below 256) goes to the slot of
           the load group's key; ids 256..399 keep retail's table */
        if (((id - 6) & 15) == 0) {
            if (id < 256)
                portCarGroups.turbo[(portCarLoadKey() ? portCarLoadKey() : id) >> 4] = object;
            else
                carTurboObjects[id >> 4] = object;
        }
#else
        if (((id - 6) & 15) == 0) carTurboObjects[id >> 4] = object;
#endif
        else if (((id - 7) & 15) == 0) D_8018F624[id >> 4] = object;
    }
    switch (id) {
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): Axel's shockwave and Shadow's strike animation of the car being
       installed (a duplicate: its own) */
    case 2324:
    case 2325:
        if (object->type == 9)
            portCarSpecialRegister(id, object);
        break;
#else
    case 2324: SET(objRegistryId2324);
    case 2325: SET(objRegistryId2325);
#endif
    case 514: case 516: case 517: SET(objRegPedestrians[object->index]);
    case 689: SET(objRegistryId689);
    case 3103: SET(objRegistryId3103);
    case 684: SET(objRegSplashAnims[object->index]);
    case 691: SET(objRegLavaSplashAnims[object->index]);
    case 686: SET(objRegBlastAnims[object->index]);
    case 669: SET(regObject669);
    APPEND_CASES:
        if (object->type == 9) {
            count = regObjectListCount;
            if (count < 50) {
                objRegTargetObjects[count] = object;
                object->index = count;
                regObjectListCount = count + 1;
            }
        }
        break;
    case 4000: SET(objRegMoscowEvents[object->index]);
    case 4005: SET(objRegLaEvents[object->index]);
    case 3003: SET(objRegMoscowDishEmitters[object->index]);
    case 3004: SET(regObject3004);
    case 3005: SET(regObject3005);
    case 3101: SET(regObject3101);
    case 3200: SET(regObject3200);
    }
}

#define SEND(slot) animSetStep(slot, value); break
/* @80124384 */
void __attribute__((section(".text_80124384"))) objRegistrySetStep(s32 id,s32 index,s32 value)
{
    if (id < 400) {
#ifdef TM2_PORT_CAR_GROUPS
        /* request 32 (CAR_GROUPS.md): type + 6 reads the first car of the type (retail's object);
           the callers that know their car use portCarTurboSetStep */
        if (((id - 6) & 15) == 0) {
            if ((u32)id < 256)
                animSetStep(portCarGroups.turbo[id >> 4], value);
            else
                animSetStep(carTurboObjects[id >> 4], value);
        }
#else
        if (((id - 6) & 15) == 0) animSetStep(carTurboObjects[id >> 4], value);
#endif
        else if (((id - 7) & 15) == 0) animSetStep(D_8018F624[id >> 4], value);
    }
    switch (id) {
    case 2324: SEND(objRegistryId2324);
    case 2325: SEND(objRegistryId2325);
    case 514: case 516: case 517: SEND(objRegPedestrians[index]);
    case 689: SEND(objRegistryId689);
    case 3103: SEND(objRegistryId3103);
    case 684: SEND(objRegSplashAnims[index]);
    case 691: SEND(objRegLavaSplashAnims[index]);
    case 686: SEND(objRegBlastAnims[index]);
    case 669: SEND(regObject669);
    APPEND_CASES: SEND(objRegTargetObjects[index]);
    case 4000: SEND(objRegMoscowEvents[index]);
    case 4005: SEND(objRegLaEvents[index]);
    case 3003: SEND(objRegMoscowDishEmitters[index]);
    case 3004: SEND(regObject3004);
    case 3005: SEND(regObject3005);
    case 3101: SEND(regObject3101);
    case 3200: SEND(regObject3200);
    }
}
