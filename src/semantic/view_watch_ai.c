/* humanCars is declared below as whole human records. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"

union TM2FlagWord { unsigned int word; unsigned char byte[4]; };
extern union TM2FlagWord arenaHollandFlag, viewWatchingAi;
extern unsigned char arenaShortChaseRange;
extern int viewWatchedAiCar, aiCarTotal;
extern struct TM2HumanRecord humanCars[];
extern void viewSetBgColor(int, int, int);
extern void routeLoadDen(void);
extern void terrainLoadGrpDen(void);
extern void terrainLoadRmpDen(void);
/* carSetViewMode is void (v0 at its return is left over from its last call), but retail
 * returns its result here, so this unit keeps the int-returning view it was compiled with. */
extern int carSetViewMode(struct TM2HumanRecord *human, int mode);

/* @80119EFC */
int __attribute__((section(".text_80119EFC"))) arenaLoadHolland(void)
{
    arenaShortChaseRange = 1;
    viewSetBgColor(0x8F, 0x6F, 0x50);
    routeLoadDen();
    terrainLoadGrpDen();
    terrainLoadRmpDen();
    arenaHollandFlag.word = 1;
    return 1;
}

/* @8011D514 */
unsigned char __attribute__((section(".text_8011D514"))) arenaIsHolland(void) { return arenaHollandFlag.byte[0]; }

/* @8011DAEC */
unsigned char __attribute__((section(".text_8011DAEC"))) viewIsWatchingAi(void) { return viewWatchingAi.byte[0]; }

/* @8011DAF8: unreferenced in retail. Sets the flag, runs carSetViewMode on
 * the first human record and advances the cursor viewWatchedAiCar over the AI
 * cars, wrapping to 0. */
void __attribute__((section(".text_8011DAF8"))) viewWatchNextAiCar(void)
{
    viewWatchingAi.word = 1;
    carSetViewMode(humanCars, 7);
    if (++viewWatchedAiCar >= aiCarTotal)
        viewWatchedAiCar = 0;
}

/* @8011DB48 */
int __attribute__((section(".text_8011DB48"))) viewStopWatchingAi(void)
{
    viewWatchingAi.word = 0;
    return carSetViewMode(humanCars, 2);
}
