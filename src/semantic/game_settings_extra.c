/* Accessors for the match settings struct sessionSettings (see docs/TOURNAMENT_PASSWORDS.md, docs/MATCH_START.md). */
typedef signed char s8;
typedef unsigned char u8;
typedef signed int s32;

#include "semantic/settings.h"

/* signed byte 48 of a code-detector menu object (set by codeDetectorSetLastCode) */
s32 __attribute__((section(".text_80137490"))) codeDetectorGetLastCode(u8 *detector)
{
    return (s8)detector[48];
}

/* lives[player] = value, clamped to 6 */
void __attribute__((section(".text_80137820"))) settingsSetLives(struct TM2Settings *settings, s32 player, s32 value)
{
    settings = (struct TM2Settings *)((u8 *)settings + player);   /* retail adds first */
    if (value >= 7)
        value = 6;
    settings->lives[0] = (u8)value;
}

/* car-select index of opponent slot `slot` (request 42 item 3: the car number, car_numbers.h) */
TM2CarNumber __attribute__((section(".text_8013783C"))) settingsGetOpponentCar(struct TM2Settings *settings, s32 slot)
{
#ifdef TM2_PORT_CAR_NUMBERS
    return portCarNumberGet(settings, 1, slot);
#else
    return TM2_SETTINGS_OPPONENT_CAR(settings, slot);   /* slots 9..: port table (TM2_PORT_CARS) */
#endif
}

/* clear settings bytes 10..22 to 0xff: the 10 opponent slots and, as in
 * retail, lives[0..2] */
void __attribute__((section(".text_80137864"))) settingsClearOpponentCars(struct TM2Settings *settings)
{
    s32 i;
    for (i = 0; i < 13; i++)
        ((s8 *)settings + i)[10] = -1;
#if TM2_MAX_AI_CARS > TM2_RETAIL_OPPONENT_SLOTS
    for (i = 0; i < TM2_MAX_AI_CARS - TM2_RETAIL_OPPONENT_SLOTS; i++)    /* TM2_PORT_CARS: slots 9.. */
        sessionPortOpponentCars[i] = -1;
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    portCarNumbersClear(settings, 2);       /* request 42 item 3: the opponents' numbers */
#endif
}

/* car-select index of human player `player` (request 42 item 3: the car number, car_numbers.h) */
TM2CarNumber __attribute__((section(".text_8013788C"))) settingsGetHumanCar(struct TM2Settings *settings, s32 player)
{
#if defined(TM2_PORT_CAR_NUMBERS)
    return portCarNumberGet(settings, 0, player);
#elif TM2_MAX_HUMANS > 2
    return TM2_SETTINGS_HUMAN_CAR(settings, player);    /* players 3-4: port table */
#else
    return settings->human_car[player];
#endif
}

/* clear settings bytes 8..20 to 0xff: both human cars, the opponent slots
 * and, as in retail, lives[0] */
void __attribute__((section(".text_801378B4"))) settingsClearHumanCars(struct TM2Settings *settings)
{
    s32 i;
    for (i = 0; i < 13; i++)
        ((s8 *)settings + i)[8] = -1;
#if TM2_MAX_HUMANS > 2
    for (i = 0; i < TM2_MAX_HUMANS - 2; i++)    /* port variant: players 3-4 */
        sessionPortHumanCars[i] = -1;
#endif
#ifdef TM2_PORT_CAR_NUMBERS
    portCarNumbersClear(settings, 3);       /* request 42 item 3: the numbers (the 13 bytes reach the opponents) */
#endif
}
