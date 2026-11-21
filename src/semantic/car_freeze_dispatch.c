/* Car-choice dispatch: per-group (id>>4) car choice objects at carFreezeObjects (16 x 264 bytes). */
typedef signed int s32;
extern unsigned char carFreezeObjects[];
extern void *carTurboObjects[16];
extern void *carFreezeChoiceTemplate;
/* animSetStep is void (v0 at its return is left over from its last call), but retail
 * returns its result here, so this unit keeps the int-returning view it was compiled with. */
extern s32 animSetStep(void *, s32);
#include "semantic/car_groups.h"

/* reset all 16 group choice objects and their secondary objects to state 0 */
void __attribute__((section(".text_80124B80"))) carFreezeResetAll(void)
{
    s32 i;
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): every car key's freeze record and turbo object */
    for (i = 0; i < TM2_CAR_KEYS; i++) {
        animSetStep(portCarGroups.freeze[i], 0);
        animSetStep(portCarGroups.turbo[i], 0);
    }
#else
    for (i = 0; i < 16; i++) {
        animSetStep(carFreezeObjects + i * 264, 0);
        animSetStep(carTurboObjects[i], 0);
    }
#endif
}

/* store the current choice definition pointer */
void __attribute__((section(".text_80124BF4"))) carFreezeSetTemplate(void *definition)
{
    carFreezeChoiceTemplate = definition;
}

/* set the state of group (id>>4)'s choice object; returns 0 for an invalid group */
s32 __attribute__((section(".text_80124C00"))) carFreezeSetState(s32 id, s32 value)
{
    s32 index = id >> 4;
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): id is a car key */
    s32 result = (unsigned int)index < TM2_CAR_KEYS;
    if (result)
        result = animSetStep(portCarGroups.freeze[index], value);
#else
    s32 result = index < 16;
    if (result)
        result = animSetStep(carFreezeObjects + index * 264, value);
#endif
    return result;
}
