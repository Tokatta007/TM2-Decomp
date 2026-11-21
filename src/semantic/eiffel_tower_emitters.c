typedef signed int s32;
typedef unsigned int u32;

u32 eiffelTowerEmitters[3] __attribute__((section(".tm2_801AB820"))) = {0,0,0};

/* @800D6E30: install an object pointer in one of the three event slots. */
u32 *__attribute__((section(".text_800D6E30"))) eiffelTowerSetEmitter(u32 object,s32 slot)
{
    eiffelTowerEmitters[slot]=object;
    return eiffelTowerEmitters;
}
