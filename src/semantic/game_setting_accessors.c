typedef signed char s8;
typedef unsigned char u8;
typedef signed int s32;

#include "semantic/settings.h"

s32 __attribute__((section(".text_8012CB84"))) sessionGetDarkToothStage(struct TM2Settings *settings)
{
    return settings->dark_tooth_stage;
}

s32 __attribute__((section(".text_8012CB90"))) sessionGetLevel(struct TM2Settings *settings)
{
    return settings->level;
}

s32 __attribute__((section(".text_8012CB9C"))) sessionGetLives(struct TM2Settings *settings,s32 player)
{
    return settings->lives[player];
}

/* @8012CBAC: signed byte from the plate-name selector array (request 42 item 3: the car number, car_numbers.h). */
TM2CarNumber __attribute__((section(".text_8012CBAC"))) sessionGetOpponentCar(struct TM2Settings *settings,s32 slot)
{
#ifdef TM2_PORT_CAR_NUMBERS
    return portCarNumberGet(settings, 1, slot);
#else
    return TM2_SETTINGS_OPPONENT_CAR(settings, slot);   /* slots 9..: port table (TM2_PORT_CARS) */
#endif
}

/* @8012CBD4: signed byte from the current game-mode selector array (request 42 item 3: the car number). */
TM2CarNumber __attribute__((section(".text_8012CBD4"))) sessionGetHumanCar(struct TM2Settings *settings,s32 player)
{
#if defined(TM2_PORT_CAR_NUMBERS)
    return portCarNumberGet(settings, 0, player);
#elif TM2_MAX_HUMANS > 2
    return TM2_SETTINGS_HUMAN_CAR(settings, player);    /* players 3-4: port table */
#else
    return settings->human_car[player];
#endif
}

void __attribute__((section(".text_8012CBC4"))) sessionSetOpponentCar(struct TM2Settings *settings,s32 slot,u8 car)
{
#ifdef TM2_PORT_CAR_NUMBERS
    portCarNumberSet(settings, 1, slot, car);       /* request 42 item 3: the number and the byte */
#else
    TM2_SETTINGS_OPPONENT_CAR(settings, slot)=car;
#endif
}

s32 __attribute__((section(".text_8012CBEC"))) sessionGetOpponents(struct TM2Settings *settings)
{
    return settings->opponents;
}

void __attribute__((section(".text_8012CBF8"))) sessionSetOpponents(struct TM2Settings *settings,u8 value)
{
    settings->opponents=value;
}

s32 __attribute__((section(".text_8012CC04"))) sessionGetMode(struct TM2Settings *settings)
{
    return settings->mode;
}
