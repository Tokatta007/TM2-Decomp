/* Small game helpers. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile); C linkage. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"

extern "C" {
/* carStatsDerive is void (v0 at its return is left over from its last call), but retail
 * returns its result (a void call in tail position would become a jump) here, so this unit keeps the int-returning view it was compiled with. */
extern s32 carStatsDerive(void *);
extern struct TM2TrackVehicle *trackVehicleRecords;
u32 carWreckModels[15] __attribute__((section(".tm2_8018740C"))) = {0};

/* @800FFECC: update both requested/current values, then refresh the object. */
s32 __attribute__((section(".text_800FFECC"))) carStatsSetSpeedMult(struct TM2CarStatsBlock *object,s32 value)
{
    object->speed_mult=(u16)value;
    object->speed_mult2=object->speed_mult;
    return carStatsDerive(object);
}

/* @801152E0: map a sixteen-wide id range to one of fifteen object slots. */
u32 __attribute__((section(".text_801152E0"))) carWreckModelGet(s32 id)
{
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): id is a car key */
    if (portCarGroups.wreck[id>>4] != 0) return (u32)portCarGroups.wreck[id>>4];
#else
    if (carWreckModels[id>>4] != 0) return carWreckModels[id>>4];
#endif
    return 0;
}

/* @80117AF0: signed rotation associated with a motion record. */
s32 __attribute__((section(".text_80117AF0"))) worldTrackVehicleHeading(s32 index)
{
    struct TM2Object *object=trackVehicleRecords[index].obj;
    return object->rot[2];
}
}
