/* Tournament lookup tables (8012AFE8, 8012B044), see
 * docs/TOURNAMENT_PASSWORDS.md. Both take the settings struct sessionSettings
 * first and ignore it. */
typedef unsigned char u8;
typedef signed int s32;

#include "semantic/settings.h"
#include "semantic/port_cars.h"

#pragma tm2_rdata .tm2_800CFC38

/* @8012AFE8: computer opponents in tournament level 1..8. */
s32 __attribute__((section(".text_8012AFE8"))) tournamentOpponentCount(struct TM2Settings *settings, s32 level)
{
    s32 count;

    switch (level) {
    case 2:
        count = 6;
        break;
    case 6:
        count = 8;
        break;
    case 1:
    case 3:
    case 4:
    case 5:
        count = 7;
        break;
    case 8:
        count = 9;
        break;
    case 7:
        count = 10;
        break;
    case 12:
    default:
        count = 0;
        break;
    }
#ifdef TM2_PORT_CARS
    /* request 32 step 2: the port's cars per arena, humans included (after the switch, so its jump
       table, which shares .tm2_800CFC38 with carIndexToType's, stays retail) */
    if (portCarsPerArena > 0)
        count = portCarsPerArena;
#endif
    return count;
}

/* @8012B044: car-select index (0..14) to car type: the 12 regular cars
 * Hammerhead (0x20) .. Shadow (0xD0), then Sweet Tooth (0x10), Minion
 * (0xE0) and Dark Tooth (0xF0); -1 for anything else. */
s32 __attribute__((section(".text_8012B044"))) carIndexToType(struct TM2Settings *settings, s32 index)
{
    s32 type;

    switch (index) {
    case 0:
        type = 0x20;
        break;
    case 1:
        type = 0x30;
        break;
    case 2:
        type = 0x40;
        break;
    case 3:
        type = 0x50;
        break;
    case 4:
        type = 0x60;
        break;
    case 5:
        type = 0x70;
        break;
    case 6:
        type = 0x80;
        break;
    case 7:
        type = 0x90;
        break;
    case 8:
        type = 0xA0;
        break;
    case 9:
        type = 0xB0;
        break;
    case 10:
        type = 0xC0;
        break;
    case 11:
        type = 0xD0;
        break;
    case 12:
        type = 0x10;
        break;
    case 13:
        type = 0xE0;
        break;
    case 14:
        type = 0xF0;
        break;
    default:
        type = -1;
        break;
    }
#ifdef TM2_PORT_CAR_NUMBERS
    /* request 42 item 3 (car_numbers.h): a mod's number gives its base's type (after the switch: its jump table,
       which shares .tm2_800CFC38 with tournamentOpponentCount's, stays retail) */
    if (type < 0)
        type = portCarNumberType(index);
#endif
    return type;
}
