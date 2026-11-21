/* Clears every car-choice / display-object registry table and the related globals. */
typedef signed int s32;
extern void *carDamageStageObjects[16], *carTireSwitchObjects[16][3], *carTurboObjects[16], *D_8018F624[16], *pickupObjects[50], *objRegTargetObjects[50], *D_8018F84C[50], *objRegBlastAnims[5], *objRegLavaSplashAnims[5], *objRegSplashAnims[5], *objRegMoscowDishEmitters[5], *objRegLaEvents[5], *objRegMoscowEvents[5], *objRegPedestrians[7];
extern s32 carFreezeChoiceTemplate, objRegistryId3103, regObject3004, regObject3005, objRegistryId689, regObject3101, regObject3200, D_80181164, D_80181168, regObjectListCount, D_8018108C;
#pragma tm2_absolute D_80181168

/* reset all registries (choice groups, tire slots, display objects, misc tables) */
void __attribute__((section(".text_80124A00"))) objRegistriesReset(void)
{
    s32 i, j;

    for (i = 0; i < 16; i++) {
        carDamageStageObjects[i] = 0;
        carTurboObjects[i] = 0;
        D_8018F624[i] = 0;
        for (j = 0; j < 3; j++)
            carTireSwitchObjects[i][j] = 0;
    }
    carFreezeChoiceTemplate = 0;
    for (i = 0; i < 50; i++)
        pickupObjects[i] = 0;
    for (i = 0; i < 50; i++) {
        objRegTargetObjects[i] = 0;
        D_8018F84C[i] = 0;
    }
    for (i = 0; i < 5; i++) {
        objRegMoscowEvents[i] = 0;
        objRegLaEvents[i] = 0;
        objRegMoscowDishEmitters[i] = 0;
        objRegSplashAnims[i] = 0;
        objRegLavaSplashAnims[i] = 0;
        objRegBlastAnims[i] = 0;
    }
    objRegistryId3103 = 0;
    for (i = 0; i < 7; i++)
        objRegPedestrians[i] = 0;
    regObject3004 = 0;
    regObject3005 = 0;
    objRegistryId689 = 0;
    regObject3101 = 0;
    regObject3200 = 0;
    D_80181164 = 0;
    D_80181168 = 0;
    regObjectListCount = 0;
    D_8018108C = 0;
}
