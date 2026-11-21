/* Car stats (800FE15C..800FF5BC): the per-car tuning values. They are not
 * a data table: carStatsLoad resets a car's stats block and calls one init
 * function per car type, each writing constants. The stats block is part of
 * each car record (human +0x14C, AI +0x4FC); see docs/CAR_STATS.md for what
 * the fields do and the values per car. */
typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

typedef struct TM2CarStats {
    u8 f00;          /* +0x000 */
    u8 pad_001[0x1];
    s8 mounts_cars;          /* +0x002 */
    u8 f03;          /* +0x003 */
    u8 pad_004[0x1];
    u8 f05;          /* +0x005 */
    u8 pad_006[0x1];
    s8 f07;          /* +0x007 */
    s8 f08;          /* +0x008 */
    u8 pad_009[0x5];
    u8 f0E;          /* +0x00E */
    u8 pad_00F[0x5];
    s16 fps;          /* +0x014 */
    s16 f16;          /* +0x016 */
    u8 pad_018[0x2];
    s16 f1A;          /* +0x01A */
    u8 pad_01C[0x6];
    s16 f22;          /* +0x022 */
    s16 ai_chase_patience;          /* +0x024 */
    u8 pad_026[0x2];
    s16 f28;          /* +0x028 */
    s16 ai_turn_range;          /* +0x02A */
    u8 pad_02C[0x8];
    s16 pitch_travel;          /* +0x034 */
    u8 pad_036[0x4];
    s16 max_hp;          /* +0x03A */
    s16 shield;          /* +0x03C */
    s16 rollover;          /* +0x03E */
    u8 pad_040[0x2];
    s16 ride_height;          /* +0x042 */
    s16 launch_height;          /* +0x044 */
    s16 roof_height;          /* +0x046 */
    s16 track_width;          /* +0x048 */
    s16 wheelbase;          /* +0x04A */
    s16 turn_speed;          /* +0x04C */
    s16 axle_front;          /* +0x04E */
    s16 axle_rear;          /* +0x050 */
    s16 top_speed;          /* +0x052 */
    u8 pad_054[0x2];
    s16 slide_speed;          /* +0x056 */
    u8 pad_058[0x4];
    s16 brake;          /* +0x05C */
    s16 steer_high;          /* +0x05E */
    u8 pad_060[0x8];
    s16 steer_lock;          /* +0x068 */
    s16 body_lean;          /* +0x06A */
    s16 steer_low;          /* +0x06C */
    s16 handbrake_lock;          /* +0x06E */
    u8 pad_070[0xA];
    s16 mass;          /* +0x07A */
    s16 hunt_full;          /* +0x07C */
    s16 hunt_low;          /* +0x07E */
    s16 attack_full;          /* +0x080 */
    s16 attack_low;          /* +0x082 */
    s16 f84;          /* +0x084 */
    s16 hunt_time;          /* +0x086 */
    s16 attack_time;          /* +0x088 */
    s16 flee_time;          /* +0x08A */
    s16 retaliate;          /* +0x08C */
    s16 retaliate_pct;          /* +0x08E */
    s16 pass_chance;          /* +0x090 */
    s16 f92;          /* +0x092 */
    u8 pad_094[0x8];
    s32 f9C;          /* +0x09C */
    u8 pad_0A0[0x8];
    s32 damageable;          /* +0x0A8 */
    s32 accel;          /* +0x0AC */
    u8 pad_0B0[0x28];
    s16 gun_x;          /* +0x0D8 */
    s16 gun_y;          /* +0x0DA */
    s16 gun_z;          /* +0x0DC */
    u8 pad_0DE[0x2];
    s16 mount_x;          /* +0x0E0 */
    s16 mount2_y;          /* +0x0E2 */
    s16 mount2_z;          /* +0x0E4 */
    u8 pad_0E6[0x2];
} TM2CarStats;
typedef char tm2carstats_size[(sizeof(TM2CarStats) == 0xE8) ? 1 : -1];

extern void carStatsSetTopSpeed(TM2CarStats *, s32);
#include "semantic/weapon_calls.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
#include "semantic/car_numbers.h"   /* request 70: portCarProfileStats */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 77: portBalanceCarStats */
#endif
void carStatsSweetTooth(TM2CarStats *s);
void carStatsHammerhead(TM2CarStats *s);
void carStatsOutlaw2(TM2CarStats *s);
void carStatsWarthog(TM2CarStats *s);
void carStatsMrGrimm(TM2CarStats *s);
void carStatsGrasshopper(TM2CarStats *s);
void carStatsThumper(TM2CarStats *s);
void carStatsSpectre(TM2CarStats *s);
void carStatsRoadKill(TM2CarStats *s);
void carStatsTwister(TM2CarStats *s);
void carStatsAxel(TM2CarStats *s);
void carStatsMrSlam(TM2CarStats *s);
void carStatsShadow(TM2CarStats *s);
void carStatsMinion(TM2CarStats *s);
void carStatsDarkTooth(TM2CarStats *s);

/* @800FE15C: reset the per-life fields and load the stats of car `type`. */
void __attribute__((section(".text_800FE15C"))) carStatsLoad(TM2CarStats *s, s32 type)
{
    s->steer_lock = 25;
    s->handbrake_lock = 3;
    s->f9C = 0;
    s->f1A = 0;
    s->f22 = 0;
    s->f16 = 0;
    s->f28 = 0;
    s->shield = 0;
    s->f03 = 0;
    s->f05 = 4;
    s->pitch_travel = 8;
    s->f07 = -1;
    s->f08 = -1;
    s->f92 = 0;
    if (!s->f00)
        s->damageable = 1;
    s->mounts_cars = 0;
    switch (type) {
    case 0x10:
        carStatsSweetTooth(s);
        break;
    case 0x20:
        carStatsHammerhead(s);
        break;
    case 0x30:
        carStatsOutlaw2(s);
        break;
    case 0x60:
        carStatsGrasshopper(s);
        break;
    case 0x40:
        carStatsWarthog(s);
        break;
    case 0x70:
        carStatsThumper(s);
        break;
    case 0x50:
        carStatsMrGrimm(s);
        break;
    case 0x80:
        carStatsSpectre(s);
        break;
    case 0xA0:
        carStatsTwister(s);
        break;
    case 0xB0:
        carStatsAxel(s);
        break;
    case 0xC0:
        carStatsMrSlam(s);
#ifdef TM2_PORT_CAR_SPECIALS
        {   /* request 33-2a (CAR_GROUPS.md 10): the arm and claw of this record's car (its key) */
            int t = portCarSpecialSelect(portCarKeyOfStats(s));

            slamClawSetAngle(739);
            slamArmSetAngle(-113);
            portCarSpecialRestore(t);
        }
#else
        slamClawSetAngle(739);
        slamArmSetAngle(-113);
#endif
        break;
    case 0xD0:
        carStatsShadow(s);
        break;
    case 0xF0:
        carStatsDarkTooth(s);
        break;
    case 0xE0:
        carStatsMinion(s);
        break;
    case 0x90:
    default:
        carStatsRoadKill(s);
        break;
    }
#ifdef TM2_PORT_BALANCE
    portBalanceCarStats(s, type);   /* request 77 (BALANCE.md 17): bit 24, Hammerhead's brakes and slide speed */
#endif
#ifdef TM2_PORT_CAR_PROFILE
    portCarProfileStats(s);     /* request 70 (CAR_PROFILES.md): a mod car's own fields over its base's */
#endif
    s->wheelbase = __builtin_abs(s->axle_front) + __builtin_abs(s->axle_rear);
    carStatsSetTopSpeed(s, s->top_speed);
    s->fps = 60;
    s->retaliate = s->max_hp * s->retaliate_pct / 100;
    s->f84 = s->hunt_full;
}

/* @800FE62C: car type 0x10, Sweet Tooth */
void __attribute__((section(".text_800FE62C"))) carStatsSweetTooth(TM2CarStats *s)
{
    s->accel = 5;
    s->brake = 2;
    s->top_speed = 92;
    s->slide_speed = 70;
    s->steer_high = 2;
    s->body_lean = 75;
    s->steer_low = 3;
    s->ride_height = 32;
    s->launch_height = 44;
    s->roof_height = 44;
    s->track_width = 66;
    s->turn_speed = 80;
    s->axle_front = 56;
    s->axle_rear = -33;
    s->gun_x = -36;
    s->gun_y = 75;
    s->gun_z = 9;
    s->mount_x = 0;
    s->mount2_y = 32;
    s->mount2_z = 44;
    s->max_hp = 120;
    s->mass = 30;
    s->rollover = 2 * s->track_width / 3;
    s->ai_chase_patience = 90;
    s->ai_turn_range = 600;
    s->pitch_travel = 16;
    s->f0E = 0;
    s->hunt_full = 75;
    s->hunt_low = 75;
    s->attack_full = 25;
    s->attack_low = 25;
    s->retaliate_pct = 100;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 80;
}

/* @800FE73C: car type 0x20, Hammerhead */
void __attribute__((section(".text_800FE73C"))) carStatsHammerhead(TM2CarStats *s)
{
    s->accel = 8;
    s->brake = 1;
    s->top_speed = 96;
    s->slide_speed = 25;
    s->steer_high = 2;
    s->body_lean = 40;
    s->steer_low = 4;
    s->handbrake_lock = 2;
    s->ride_height = 66;
    s->launch_height = 33;
    s->roof_height = 33;
    s->track_width = 86;
    s->turn_speed = 100;
    s->axle_front = 64;
    s->axle_rear = -76;
    s->gun_x = -24;
    s->gun_y = 55;
    s->gun_z = -12;
    s->mount_x = 0;
    s->mount2_y = 12;
    s->mount2_z = 44;
    s->max_hp = 110;
    s->mass = 35;
    s->rollover = s->track_width;
    s->ai_chase_patience = 90;
    s->ai_turn_range = 520;
    s->mounts_cars = 1;
    s->pitch_travel = 24;
    s->f0E = 0;
    s->hunt_full = 100;
    s->hunt_low = 0;
    s->attack_full = 0;
    s->attack_low = 10;
    s->retaliate_pct = 80;
    s->hunt_time = 3600;
    s->attack_time = 300;
    s->flee_time = 600;
    s->pass_chance = 10;
}

/* @800FE858: car type 0x30, Outlaw 2 */
void __attribute__((section(".text_800FE858"))) carStatsOutlaw2(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 127;
    s->slide_speed = 103;
    s->steer_high = 2;
    s->body_lean = 20;
    s->steer_low = 6;
    s->ride_height = 16;
    s->launch_height = 17;
    s->roof_height = 32;
    s->track_width = 52;
    s->turn_speed = 75;
    s->axle_front = 58;
    s->axle_rear = -62;
    s->gun_x = -28;
    s->gun_y = 42;
    s->gun_z = 6;
    s->mount_x = 0;
    s->mount2_y = -9;
    s->mount2_z = 30;
    s->max_hp = 100;
    s->mass = 25;
    s->rollover = s->track_width;
    s->ai_chase_patience = 40;
    s->ai_turn_range = 600;
    s->f0E = 2;
    s->hunt_full = 20;
    s->hunt_low = 0;
    s->attack_full = 80;
    s->attack_low = 100;
    s->retaliate_pct = 20;
    s->hunt_time = 900;
    s->attack_time = 900;
    s->flee_time = 300;
    s->pass_chance = 70;
}

/* @800FE960: car type 0x40, Warthog */
void __attribute__((section(".text_800FE960"))) carStatsWarthog(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 2;
    s->top_speed = 101;
    s->slide_speed = 110;
    s->steer_high = 2;
    s->body_lean = 15;
    s->steer_low = 3;
    s->handbrake_lock = 2;
    s->ride_height = 26;
    s->launch_height = 31;
    s->roof_height = 32;
    s->track_width = 56;
    s->turn_speed = 90;
    s->axle_front = 64;
    s->axle_rear = -61;
    s->gun_x = -25;
    s->gun_y = 68;
    s->gun_z = 11;
    s->mount_x = 0;
    s->mount2_y = 10;
    s->mount2_z = 33;
    s->max_hp = 130;
    s->mass = 30;
    s->rollover = s->track_width;
    s->ai_chase_patience = 60;
    s->ai_turn_range = 440;
    s->pitch_travel = 16;
    s->f0E = 0;
    s->hunt_full = 60;
    s->hunt_low = 0;
    s->attack_full = 40;
    s->attack_low = 100;
    s->retaliate_pct = 100;
    s->hunt_time = 300;
    s->attack_time = 300;
    s->flee_time = 300;
    s->pass_chance = 30;
}

/* @800FEA68: car type 0x50, Mr. Grimm */
void __attribute__((section(".text_800FEA68"))) carStatsMrGrimm(TM2CarStats *s)
{
    s->accel = 11;
    s->brake = 3;
    s->top_speed = 125;
    s->slide_speed = 110;
    s->steer_high = 2;
    s->body_lean = -70;
    s->steer_low = 5;
    s->ride_height = 17;
    s->launch_height = 13;
    s->roof_height = 32;
    s->track_width = 53;
    s->turn_speed = 65;
    s->axle_front = 50;
    s->axle_rear = -41;
    s->gun_x = -13;
    s->gun_y = 18;
    s->gun_z = 9;
    s->mount_x = 0;
    s->mount2_y = -17;
    s->mount2_z = 35;
    s->max_hp = 65;
    s->mass = 5;
    s->rollover = 3 * s->track_width;
    s->ai_chase_patience = 75;
    s->ai_turn_range = 640;
    s->f0E = 1;
    s->hunt_full = 90;
    s->hunt_low = 60;
    s->attack_full = 10;
    s->attack_low = 30;
    s->retaliate_pct = 7;
    s->hunt_time = 300;
    s->attack_time = 1800;
    s->flee_time = 300;
    s->pass_chance = 80;
}

/* @800FEB8C: car type 0x60, Grasshopper */
void __attribute__((section(".text_800FEB8C"))) carStatsGrasshopper(TM2CarStats *s)
{
    s->accel = 5;
    s->brake = 3;
    s->top_speed = 121;
    s->slide_speed = 90;
    s->steer_high = 2;
    s->body_lean = 40;
    s->steer_low = 5;
    s->ride_height = 17;
    s->launch_height = 16;
    s->roof_height = 32;
    s->track_width = 73;
    s->turn_speed = 65;
    s->axle_front = 41;
    s->axle_rear = -54;
    s->gun_x = -21;
    s->gun_y = 9;
    s->gun_z = 19;
    s->mount_x = 0;
    s->mount2_y = -8;
    s->mount2_z = 44;
    s->max_hp = 90;
    s->mass = 15;
    s->rollover = s->track_width;
    s->ai_chase_patience = 60;
    s->ai_turn_range = 480;
    s->f0E = 3;
    s->hunt_full = 50;
    s->hunt_low = 50;
    s->attack_full = 50;
    s->attack_low = 50;
    s->retaliate_pct = 80;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 80;
}

/* @800FEC8C: car type 0x70, Thumper */
void __attribute__((section(".text_800FEC8C"))) carStatsThumper(TM2CarStats *s)
{
    s->accel = 5;
    s->brake = 4;
    s->top_speed = 111;
    s->slide_speed = 85;
    s->steer_high = 2;
    s->body_lean = 60;
    s->steer_low = 8;
    s->ride_height = 14;
    s->launch_height = 18;
    s->roof_height = 32;
    s->track_width = 52;
    s->turn_speed = 85;
    s->axle_front = 59;
    s->axle_rear = -75;
    s->gun_x = -27;
    s->gun_y = 60;
    s->gun_z = 8;
    s->mount_x = 0;
    s->mount2_y = 3;
    s->mount2_z = 30;
    s->max_hp = 100;
    s->mass = 25;
    s->rollover = s->track_width;
    s->ai_chase_patience = 110;
    s->ai_turn_range = 560;
    s->f0E = 2;
    s->hunt_full = 50;
    s->hunt_low = 10;
    s->attack_full = 20;
    s->attack_low = 40;
    s->retaliate_pct = 80;
    s->hunt_time = 600;
    s->attack_time = 1200;
    s->flee_time = 600;
    s->pass_chance = 70;
}

/* @800FED98: car type 0x80, Spectre */
void __attribute__((section(".text_800FED98"))) carStatsSpectre(TM2CarStats *s)
{
    s->accel = 10;
    s->brake = 2;
    s->top_speed = 131;
    s->slide_speed = 110;
    s->steer_high = 2;
    s->body_lean = 15;
    s->steer_low = 3;
    s->ride_height = 16;
    s->launch_height = 17;
    s->roof_height = 32;
    s->track_width = 52;
    s->turn_speed = 70;
    s->axle_front = 48;
    s->axle_rear = -62;
    s->gun_x = -24;
    s->gun_y = 52;
    s->gun_z = 8;
    s->mount_x = 0;
    s->mount2_y = -14;
    s->mount2_z = 30;
    s->max_hp = 90;
    s->mass = 13;
    s->rollover = s->track_width;
    s->ai_chase_patience = 15;
    s->ai_turn_range = 560;
    s->f0E = 1;
    s->hunt_full = 30;
    s->hunt_low = 30;
    s->attack_full = 60;
    s->attack_low = 60;
    s->retaliate_pct = 20;
    s->hunt_time = 900;
    s->attack_time = 900;
    s->flee_time = 300;
    s->pass_chance = 70;
}

/* @800FEE98: Roadkill (0x90) and any other car type */
void __attribute__((section(".text_800FEE98"))) carStatsRoadKill(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 119;
    s->slide_speed = 75;
    s->steer_high = 2;
    s->body_lean = 25;
    s->steer_low = 7;
    s->ride_height = 16;
    s->launch_height = 19;
    s->roof_height = 32;
    s->track_width = 52;
    s->turn_speed = 80;
    s->axle_front = 52;
    s->axle_rear = -64;
    s->gun_x = -24;
    s->gun_y = 40;
    s->gun_z = 9;
    s->mount_x = 0;
    s->mount2_y = -14;
    s->mount2_z = 31;
    s->max_hp = 100;
    s->mass = 28;
    s->rollover = s->track_width;
    s->ai_chase_patience = 25;
    s->ai_turn_range = 600;
    s->f0E = 0;
    s->hunt_full = 75;
    s->hunt_low = 30;
    s->attack_full = 15;
    s->attack_low = 70;
    s->retaliate_pct = 80;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 50;
}

/* @800FEF98: car type 0xA0, Twister */
void __attribute__((section(".text_800FEF98"))) carStatsTwister(TM2CarStats *s)
{
    s->accel = 9;
    s->brake = 2;
    s->top_speed = 141;
    s->slide_speed = 140;
    s->steer_high = 2;
    s->body_lean = 20;
    s->steer_low = 5;
    s->ride_height = 12;
    s->launch_height = 12;
    s->roof_height = 32;
    s->track_width = 72;
    s->turn_speed = 70;
    s->axle_front = 56;
    s->axle_rear = -70;
    s->gun_x = -9;
    s->gun_y = 37;
    s->gun_z = 8;
    s->mount_x = 0;
    s->mount2_y = 6;
    s->mount2_z = 32;
    s->max_hp = 75;
    s->mass = 14;
    s->rollover = s->track_width;
    s->ai_chase_patience = 12;
    s->ai_turn_range = 640;
    s->f0E = 1;
    s->hunt_full = 50;
    s->hunt_low = 50;
    s->attack_full = 50;
    s->attack_low = 50;
    s->retaliate_pct = 20;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 90;
}

/* @800FF090: car type 0xB0, Axel */
void __attribute__((section(".text_800FF090"))) carStatsAxel(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 103;
    s->slide_speed = 75;
    s->steer_high = 2;
    s->body_lean = 55;
    s->steer_low = 6;
    s->ride_height = 15;
    s->launch_height = 0;
    s->roof_height = 60;
    s->track_width = 116;
    s->turn_speed = 100;
    s->axle_front = 56;
    s->axle_rear = -56;
    s->gun_x = -25;
    s->gun_y = 46;
    s->gun_z = 0;
    s->mount_x = 0;
    s->mount2_y = -14;
    s->mount2_z = 66;
    s->max_hp = 110;
    s->mass = 30;
    s->rollover = s->track_width;
    s->ai_chase_patience = 25;
    s->mounts_cars = 1;
    s->ai_turn_range = 480;
    s->f0E = 3;
    s->hunt_full = 65;
    s->hunt_low = 35;
    s->attack_full = 65;
    s->attack_low = 35;
    s->retaliate_pct = 50;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 10;
}

/* @800FF19C: car type 0xC0, Mr. Slam */
void __attribute__((section(".text_800FF19C"))) carStatsMrSlam(TM2CarStats *s)
{
    s->accel = 4;
    s->brake = 3;
    s->top_speed = 92;
    s->slide_speed = 80;
    s->steer_high = 2;
    s->body_lean = 40;
    s->steer_low = 7;
    s->handbrake_lock = 2;
    s->ride_height = 32;
    s->launch_height = 55;
    s->roof_height = 75;
    s->track_width = 74;
    s->turn_speed = 90;
    s->axle_front = 61;
    s->axle_rear = -79;
    s->gun_x = -23;
    s->gun_y = 53;
    s->gun_z = 29;
    s->mount_x = 0;
    s->mount2_y = 4;
    s->mount2_z = 62;
    s->max_hp = 120;
    s->mass = 35;
    s->rollover = s->track_width;
    s->ai_chase_patience = 180;
    s->ai_turn_range = 440;
    s->pitch_travel = 16;
    s->f0E = 2;
    s->hunt_full = 100;
    s->hunt_low = 100;
    s->attack_full = 0;
    s->attack_low = 0;
    s->retaliate_pct = 100;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 10;
}

/* @800FF2B0: car type 0xD0, Shadow */
void __attribute__((section(".text_800FF2B0"))) carStatsShadow(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 113;
    s->slide_speed = 75;
    s->steer_high = 2;
    s->body_lean = 35;
    s->steer_low = 7;
    s->ride_height = 20;
    s->launch_height = 27;
    s->roof_height = 32;
    s->track_width = 50;
    s->turn_speed = 105;
    s->axle_front = 60;
    s->axle_rear = -105;
    s->gun_x = -28;
    s->gun_y = 66;
    s->gun_z = 8;
    s->mount_x = 0;
    s->mount2_y = 4;
    s->mount2_z = 36;
    s->max_hp = 100;
    s->mass = 25;
    s->rollover = s->track_width;
    s->ai_chase_patience = 40;
    s->ai_turn_range = 560;
    s->pitch_travel = 16;
    s->f0E = 0;
    s->hunt_full = 50;
    s->hunt_low = 50;
    s->attack_full = 45;
    s->attack_low = 45;
    s->retaliate_pct = 40;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 80;
}

/* @800FF3B8: car type 0xE0, Minion */
void __attribute__((section(".text_800FF3B8"))) carStatsMinion(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 127;
    s->slide_speed = 150;
    s->steer_high = 2;
    s->body_lean = 20;
    s->steer_low = 6;
    s->ride_height = 27;
    s->launch_height = 66;
    s->roof_height = 66;
    s->track_width = 108;
    s->turn_speed = 75;
    s->axle_front = 126;
    s->axle_rear = -126;
    s->gun_x = -18;
    s->gun_y = 105;
    s->gun_z = 27;
    s->mount_x = 0;
    s->mount2_y = 0;
    s->mount2_z = 50;
    s->max_hp = 210;
    s->mass = 60;
    s->rollover = s->track_width;
    s->mounts_cars = 1;
    s->ai_chase_patience = 40;
    s->ai_turn_range = 600;
    s->pitch_travel = 16;
    s->f0E = 0;
    s->hunt_full = 100;
    s->hunt_low = 100;
    s->attack_full = 0;
    s->attack_low = 0;
    s->retaliate_pct = 90;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 10;
}

/* @800FF4BC: car type 0xF0, Dark Tooth */
void __attribute__((section(".text_800FF4BC"))) carStatsDarkTooth(TM2CarStats *s)
{
    s->accel = 6;
    s->brake = 3;
    s->top_speed = 127;
    s->slide_speed = 75;
    s->steer_high = 2;
    s->body_lean = 75;
    s->steer_low = 7;
    s->ride_height = 116;
    s->launch_height = 56;
    s->roof_height = 106;
    s->track_width = 173;
    s->turn_speed = 80;
    s->axle_front = 216;
    s->axle_rear = -269;
    s->gun_x = -87;
    s->gun_y = 90;
    s->gun_z = 0;
    s->mount_x = 0;
    s->mount2_y = 0;
    s->mount2_z = 200;
    s->max_hp = 300;
    s->mass = 100;
    s->rollover = s->track_width;
    s->ai_chase_patience = 25;
    s->mounts_cars = 1;
    s->ai_turn_range = 880;
    s->pitch_travel = 24;
    s->f0E = 0;
    s->hunt_full = 100;
    s->hunt_low = 100;
    s->attack_full = 0;
    s->attack_low = 0;
    s->retaliate_pct = 100;
    s->hunt_time = 600;
    s->attack_time = 600;
    s->flee_time = 600;
    s->pass_chance = 0;
}
