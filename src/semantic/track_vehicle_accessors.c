typedef signed int s32;
typedef unsigned int u32;

unsigned char *trackVehicleRecords __attribute__((section(".tm2_80180F84"))) = 0;

/* @80117B20: position vector associated with a motion record. */
s32 *__attribute__((section(".text_80117B20"))) worldTrackVehiclePos(s32 index)
{
    return (s32 *)(*(u32 *)(trackVehicleRecords+index*56+52)+168u);
}

/* @80118F18: heading associated with a motion record. */
s32 __attribute__((section(".text_80118F18"))) trackVehicleGetTurn(s32 index)
{
    return *(s32 *)(trackVehicleRecords+index*56+8);
}
