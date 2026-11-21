/* Small helpers bounded by the two human players (DECOMP_REQUESTS section 4).
 *
 * worldFallLimitZ: per-level height limit for a car (level from
 * gameGetLevel); on level 8 a human car below z 0 and beyond y -13920
 * gets 0.
 * worldTrackVehiclesReset: reset the two 56-byte player records at worldCollapseSections.
 * carPartsSpinAxel: turn the shared angle carPartsSpinAngleAxel by a / 7 and rotate both
 * players' objects (carAxelObjects[2]) about X by it. */
#include "semantic/cars.h"

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

typedef struct TM2TrackVehicle TM2PlayerRecord56;

extern TM2PlayerRecord56 worldCollapseSections[2];
extern TM2PlayerRecord56 *trackVehicleRecords;
extern s32 carPartsSpinAngleAxel;
extern u8 *carAxelObjects[2];

extern int gameGetLevel(void);
extern void *RotMatrixX(s32, TM2Matrix *);
extern void mathCopyWords(void *, TM2Matrix *, s32);

/* @8011EC0C */
s32 __attribute__((section(".text_8011EC0C"))) worldFallLimitZ(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    s32 h;

    switch ((unsigned)gameGetLevel()) {
    case 8:
        h = -8000;
        if (human && ((struct TM2HumanRecord *)car)->motion.pos[2] < 0)
            h = ((struct TM2HumanRecord *)car)->motion.pos[1] >= -13920 ? h : 0;
        break;
    case 5:
    case 12:
        h = 6000;
        break;
    default:
        h = -8000;
        break;
    case 6:
        h = 0xF00;
        break;
    }
    return h;
}

/* @80117944 */
void __attribute__((section(".text_80117944"))) worldTrackVehiclesReset(void)
{
    TM2PlayerRecord56 *p;
    s32 i;

    trackVehicleRecords = p = worldCollapseSections;
    for (i = 0; i < 2; i++) {
        p[i].obj = 0;
        p[i].busy = 0;
    }
}

/* @801148B8 */
void __attribute__((section(".text_801148B8"))) carPartsSpinAxel(s32 a)
{
    TM2Matrix m;
    s32 i, j, k;

    carPartsSpinAngleAxel -= a / 7;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++)
            m.m[i][j] = (i == j) << 12;
        m.t[i] = 0;
    }
    RotMatrixX(-carPartsSpinAngleAxel, &m);
    for (k = 0; k < 2; k++)
        if (carAxelObjects[k])
            mathCopyWords(carAxelObjects[k] + 4, &m, 5);
}
