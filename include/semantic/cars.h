/* Car records (see src/semantic/car_records.c).
 *
 * Human cars: humanCars[TM2_MAX_HUMANS] (2 in retail, players.h), count humanCarTotal. AI cars: aiCars[9], count
 * aiCarTotal. The arrays sit back to back in RAM. The two layouts differ;
 * only fields with a known use are named. */
#ifndef TM2_SEMANTIC_CARS_H
#define TM2_SEMANTIC_CARS_H

#include "semantic/players.h"

#ifndef TM2_BASIC_TYPES
#define TM2_BASIC_TYPES
typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#endif

struct TM2HumanCar {
    s16 player;             /* +0x000 player number */
    s16 id;                 /* +0x002 */
    u8 pad_004[0x98];
    u8 flag_09C;            /* +0x09C */
    u8 pad_09D[0xA];
    u8 flag_0A7;            /* +0x0A7 */
    u8 pad_0A8[0x2F];
    u8 key_view;            /* +0x0D7 view-change hotkey held */
    u8 pad_0D8[0x1];
    u8 key_rear_view;       /* +0x0D9 */
    u8 key_0DA;             /* +0x0DA */
    u8 key_0DB;             /* +0x0DB */
    u8 pad_0DC[0x9];
    u8 pad_config;          /* +0x0E5 pad configuration 0..3 */
    u8 key_0E6;             /* +0x0E6 */
    u8 pad_0E7[0x11];
    u8 buttons[18];         /* +0x0F8 action -> button (padButtonHeld numbers) */
    u8 pad_10A[0x2];
    s32 view;               /* +0x10C camera view, 2..7 */
    u8 pad_110[0x5D6];
    s16 speed;              /* +0x6E6 */
    u8 pad_6E8[0x14];
    s32 pos[3];             /* +0x6FC */
    s32 rot[3];             /* +0x708 */
    u8 pad_714[0x38];
    u8 matrix[32];          /* +0x74C */
    u8 pad_76C[0xA8];
};
typedef char tm2_human_car_size[(sizeof(struct TM2HumanCar) == 0x814) ? 1 : -1];

struct TM2AICar {
    u8 pad_000[0x4FC];
    u8 heat[0xA4];          /* +0x4FC */
    s32 hp;                 /* +0x5A0 */
    u8 pad_5A4[0x182];
    s16 speed;              /* +0x726 */
    u8 pad_728[0x14];
    s32 pos[3];             /* +0x73C */
    s32 rot[3];             /* +0x748 */
    u8 pad_754[0x38];
    u8 matrix[32];          /* +0x78C */
    u8 pad_7AC[0xA8];
};
typedef char tm2_ai_car_size[(sizeof(struct TM2AICar) == 0x854) ? 1 : -1];

/* World object (only the fields used here). */
struct TM2Object {
    /* -- weapons/camera pass: +0x000..+0x07F -- */
    u8 pad_000[0x4];
    s32 light;              /* +0x004 light environment (lightGetEnv; pass 2) */
    s32 visible;            /* +0x008 drawn/active */
    s32 f00C;               /* +0x00C */
    u8 pad_010[0x70];
    /* -- end weapons/camera pass -- */
    u8 matrix[32];          /* +0x080 */
    s16 rot[3];             /* +0x0A0 */
    u8 pad_0A6[0x2];
    s32 pos[3];             /* +0x0A8 */
    s32 box_count;          /* +0x0B4 collision box vertex count (carBuildCollisionBox) */
    s16 box[6][4];          /* +0x0B8 collision box vertices x, y, z */
    u8 pad_0E8[0x30];
    s32 id;                 /* +0x118 */
};

/* A file that declares the two arrays with its own record view (and so
 * keeps retail's symbol+offset addressing) defines TM2_OWN_CAR_ARRAYS. */
#ifndef TM2_OWN_CAR_ARRAYS
extern struct TM2HumanCar humanCars[TM2_MAX_HUMANS];
extern struct TM2AICar aiCars[TM2_MAX_AI_CARS];
#endif
extern s32 humanCarTotal;      /* human cars */
extern s32 aiCarTotal;      /* AI cars */

/* ======================================================================
 * Car record blocks (added for car_control.c, car_tick.c, weapon_select.c,
 * weapon_launch.c, update_guns.c, ...). The human and AI records hold the
 * same blocks at different offsets; code that serves both takes a pointer
 * to the block. Names follow docs/GAME_EVENTS.md, docs/CAR_STATS.md and
 * docs/COOP_LIVES.md; fields named fXX have no known meaning yet.
 *
 *   block     human   AI      size
 *   state     +0x09C  +0x06C  0x30
 *   input     +0x0D0  +0x044  0x28
 *   aim       +0x114  +0x09C  0x38
 *   stats     +0x14C  +0x4FC  0xE8
 *   wheels    +0x5A4  +0x5E4  0x138
 *   motion    +0x6DC  +0x71C  0xEC
 *   weapons   +0x7C8  +0x808  0x4C
 * ====================================================================== */
#define TM2_FIELD_AT(name, type, field, off) \
    typedef char name[((unsigned int)&((type *)0)->field == (off)) ? 1 : -1]

struct TM2CarMatrix { s16 m[3][3]; s16 pad; s32 t[3]; };

/* State flags (one byte each). */
struct TM2CarState {
    u8 dead;                /* +0x00 */
    u8 f01;                 /* +0x01 */
    u8 f02;                 /* +0x02 */
    u8 f03;                 /* +0x03 */
    u8 accelerating;        /* +0x04 throttle applied this frame */
    u8 f05;                 /* +0x05 */
    u8 f06;                 /* +0x06 */
    u8 reversing;           /* +0x07 */
    u8 f08;                 /* +0x08 */
    u8 weapon_fired;        /* +0x09 a projectile left this frame */
    u8 on_surface;          /* +0x0A over a ramp/plate surface */
    u8 wheel_in_air;        /* +0x0B some wheel in the air */
    u8 all_in_air;          /* +0x0C all four wheels in the air */
    u8 frozen;              /* +0x0D cannot steer, fire or use energy */
    u8 energy_shield;       /* +0x0E blocks all damage */
    u8 f0F;                 /* +0x0F */
    u8 f10;                 /* +0x10 */
    u8 f11;                 /* +0x11 */
    u8 f12;                 /* +0x12 */
    u8 hud_notice;          /* +0x13 regen HUD message pending */
    u8 crush_contact;       /* +0x14 a wheel touches the crush target */
    u8 crush_peer;          /* +0x15 */
    u8 f16;                 /* +0x16 */
    u8 f17;                 /* +0x17 */
    u8 sliding;             /* +0x18 */
    u8 f19;                 /* +0x19 */
    u8 burning;             /* +0x1A */
    u8 f1B;                 /* +0x1B */
    u8 f1C;                 /* +0x1C */
    u8 f1D;                 /* +0x1D */
    u8 hit_from_side;       /* +0x1E knocked from the side (ricochet) */
    u8 falling;             /* +0x1F below the level fall limit */
    u8 f20;                 /* +0x20 */
    u8 in_water;            /* +0x21 */
    u8 in_lava;             /* +0x22 */
    u8 in_fire;             /* +0x23 */
    u8 sinking;             /* +0x24 */
    u8 riding;              /* +0x25 on a moving surface */
    u8 f26;                 /* +0x26 wreck: small explosion done */
    u8 f27;                 /* +0x27 body following the car (carSpinOutUpdate) */
    u8 f28;                 /* +0x28 */
    u8 pad_29;
    s16 route_node;         /* +0x2A human: nearest route node (carNearestRoutePoint) */
    union {
        s16 route;          /* human: route group of the car (AI records
                             * keep theirs in TM2AIDrive.route) */
        s32 *fx;            /* AI: effect object put out when wrecked (human
                             * records keep theirs at TM2HumanRecord.fx) */
    } x2C;                  /* +0x2C */
};
typedef char tm2_car_state_size[(sizeof(struct TM2CarState) == 0x30) ? 1 : -1];

/* Control inputs, one byte each, written by the pad reader (human) or the
 * AI driver. */
struct TM2CarInput {
    u8 slide_inc;           /* +0x00 handbrake turn, steer_inc side */
    u8 slide_dec;           /* +0x01 handbrake turn, steer_dec side */
    u8 slide;               /* +0x02 handbrake turn */
    u8 turbo;               /* +0x03 */
    u8 f04;                 /* +0x04 */
    u8 view_modifier;       /* +0x05 */
    u8 power_slide;         /* +0x06 braking while moving with a direction held (threshold: stats.accel_launch) */
    u8 key_07;              /* +0x07 view hotkeys (human) */
    u8 f08;                 /* +0x08 */
    u8 key_09;              /* +0x09 */
    u8 key_0A;              /* +0x0A */
    u8 key_0B;              /* +0x0B */
    u8 pad_0C;
    u8 f0D;                 /* +0x0D (cleared by aiClearControls; pass 2) */
    u8 reverse;             /* +0x0E reverse button */
    u8 steer_dec;           /* +0x0F steering towards negative angles */
    u8 steer_inc;           /* +0x10 steering towards positive angles */
    u8 forward;             /* +0x11 accelerate button */
    u8 throttle;            /* +0x12 */
    u8 brake;               /* +0x13 */
    u8 drive;               /* +0x14 accelerate or reverse held */
    u8 f15;                 /* +0x15 */
    u8 key_16;              /* +0x16 */
    u8 cycle_next;          /* +0x17 weapon select +1 */
    u8 cycle_prev;          /* +0x18 weapon select -1 */
    u8 cycle_armed;         /* +0x19 */
    u8 fire;                /* +0x1A fire the selected weapon */
    u8 fire_guns;           /* +0x1B machine guns (consumed by UpdateGuns) */
    u8 fire_special;        /* +0x1C special weapon (AI) */
    u8 fire_ready;          /* +0x1D */
    u8 fire_held;           /* +0x1E */
    u8 pad_1F[2];
    u8 f21;                 /* +0x21 (cleared by carRestart) */
    u8 f22;                 /* +0x22 (cleared by carRestart) */
    u8 pad_23;
    s32 f24;                /* +0x24 (cleared by carRestart) */
};
typedef char tm2_car_input_size[(sizeof(struct TM2CarInput) == 0x28) ? 1 : -1];

/* Aim offsets (gun angles relative to the car). */
struct TM2CarAim {
    /* -- raw-offset pass 2: body shake / sway (carShakeStart,
     *    carShakeRandomize, carBodySwayUpdate) -- */
    s32 shake_level;        /* +0x00 level to re-arm with */
    s32 shake;              /* +0x04 active shake level, 0 = none */
    s32 shake_axes;         /* +0x08 0 = x, 1 = y, 2 = both */
    s16 shake_count;        /* +0x0C periods left */
    s16 shake_phase;        /* +0x0E sine phase */
    u8 pad_10[2];
    s16 f12;                /* +0x12 */
    s16 shake_step;         /* +0x14 phase step per frame */
    s16 lean_pitch;         /* +0x16 eased towards the slope */
    s16 lean_roll;          /* +0x18 eased from speed and steering */
    u8 pad_1A[2];
    s16 wheel_pitch;        /* +0x1C body pitch from the wheel heights */
    s16 wheel_roll;         /* +0x1E body roll from the wheel heights */
    s16 shake_x;            /* +0x20 strength, x axis */
    s16 shake_y;            /* +0x22 strength, y axis */
    u8 pad_24[4];
    s16 shake_amp_x;        /* +0x28 current amplitude */
    s16 shake_amp_y;        /* +0x2A */
    u8 pad_2C[4];
    /* -- end -- */
    s16 pitch;              /* +0x30 */
    s16 f32;                /* +0x32 shake offset paired with pitch (pass 2) */
    s16 yaw;                /* +0x34 */
    u8 pad_36[2];
};
typedef char tm2_car_aim_size[(sizeof(struct TM2CarAim) == 0x38) ? 1 : -1];

/* Stats block (tuning values and per-life state), docs/CAR_STATS.md. */
struct TM2CarStatsBlock {
    u8 f00;                 /* +0x00 */
    u8 heal_spot_used;      /* +0x01 the level's hidden full-heal spot was used */
    s8 mounts_cars;         /* +0x02 */
    u8 f03;                 /* +0x03 */
    s8 smoke_arg;           /* +0x04 third carSetDamageStage argument (pass 2) */
    s8 wreck_state;         /* +0x05 damage smoke level 0..2, 3/4 wrecked */
    u8 pad_06;
    s8 killer;              /* +0x07 slot */
    s8 last_attacker;       /* +0x08 slot */
    u8 cheat_a;             /* +0x09 cheat combo 2 entered */
    u8 cheat_b;             /* +0x0A cheat combo 4 entered */
    u8 cheat_released;      /* +0x0B pad released since */
    u8 f0C;                 /* +0x0C (cleared by carRestart) */
    u8 pad_0D[3];
    s16 cheat_timer;        /* +0x10 */
    u8 pad_12[2];
    s16 fps;                /* +0x14 frame rate the speeds are scaled for */
    s16 f16;                /* +0x16 */
    u8 pad_18[2];
    s16 freeze_timer;       /* +0x1A */
    s16 frozen_by_missile;  /* +0x1C */
    s16 f1E;                /* +0x1E AI: cleared when frozen by a hit (projectileHitTest) */
    s16 f20;                /* +0x20 cleared on landing (carSuspensionUpdate) */
    s16 burn_time;          /* +0x22 0..100 */
    s16 ai_chase_patience;  /* +0x24 */
    s16 wreck_timer;        /* +0x26 counts down while wrecked */
    u8 pad_28[2];
    s16 turn_range;         /* +0x2A AI: distance from the next node at which it turns (docs/CAR_STATS.md) */
    u8 pad_2C[4];
    s16 crush_range;        /* +0x30 */
    s16 crush_peak;         /* +0x32 */
    s16 pitch_travel;       /* +0x34 */
    u8 pad_36[4];
    s16 max_hp;             /* +0x3A */
    s16 shield;             /* +0x3C damage halved */
    s16 rollover;           /* +0x3E */
    s16 target_node;        /* +0x40 AI: route node to head for when placed */
    s16 ride_height;        /* +0x42 */
    s16 launch_height;      /* +0x44 */
    s16 roof_height;        /* +0x46 */
    s16 track_width;        /* +0x48 */
    s16 wheelbase;          /* +0x4A */
    s16 turn_speed;         /* +0x4C */
    s16 axle_front;         /* +0x4E */
    s16 axle_rear;          /* +0x50 */
    s16 top_speed;          /* +0x52 "mph" */
    s16 reverse_speed;      /* +0x54 */
    s16 slide_speed;        /* +0x56 */
    s16 cruise_speed;       /* +0x58 */
    s16 rollover_speed;     /* +0x5A */
    s16 brake;              /* +0x5C */
    s16 steer_high;         /* +0x5E */
    s16 steer_rate_high;    /* +0x60 steering step at high speed */
    s16 steer_rate_low;     /* +0x62 steering step at rest */
    s16 steer_limit;        /* +0x64 */
    s16 steer_limit_half;   /* +0x66 */
    s16 steer_lock;         /* +0x68 */
    s16 body_lean;          /* +0x6A */
    s16 steer_low;          /* +0x6C */
    s16 handbrake_lock;     /* +0x6E steering limit multiplier when sliding */
    s16 steer_tilt_max;     /* +0x70 steering beyond it settles the body (carBodyAttitudeUpdate) */
    u8 pad_72[2];
    s16 lean_max;           /* +0x74 body roll limit (carBodySwayUpdate) */
    s16 speed_mult;         /* +0x76 percent */
    s16 speed_mult2;        /* +0x78 */
    s16 mass;               /* +0x7A */
    u8 pad_7C[0x16];
    s16 regen_clock;        /* +0x92 ticks since the last regen step */
    s16 regen_steps;        /* +0x94 */
    u8 pad_96[2];
    s32 crush_owner;        /* +0x98 owner id of the car being crushed */
    s32 restart_pending;    /* +0x9C */
    s32 probe_on;           /* +0xA0 contact probe and pickups */
    s32 hp;                 /* +0xA4 */
    s32 damageable;         /* +0xA8 0 = takes no damage */
    s32 accel;              /* +0xAC */
    s32 accel_top;          /* +0xB0 at top speed */
    s32 accel_launch;       /* +0xB4 speed gained per frame at rest (accel_top * 5 * mult / 100,
                             * carStatsDerive); also the "moving at all" speed threshold
                             * (car_control.c, car_rollover.c, car_crash.c, car_contacts.c) */
    s32 speed_cap;          /* +0xB8 top_speed in internal units, the forward speed cap
                             * (docs/GAME_EVENTS.md "Speed"; turbo raises it by 4/3) */
    s32 steer_boost_speed;  /* +0xBC */
    s32 slide_cap;          /* +0xC0 */
    s32 cruise_cap;         /* +0xC4 */
    s32 reverse_cap;        /* +0xC8 reverse_speed in internal units, the reverse speed cap
                             * (car_control.c); car-car pushes use it as a hit speed */
    s32 brake_decel;        /* +0xCC */
    s32 rollover_cap;       /* +0xD0 */
    s32 spawn_node;         /* +0xD4 AI: route node the car is placed on */
    s16 gun[3];             /* +0xD8 machine-gun muzzle x, y, z */
    u8 pad_DE[2];
    s16 mount_x;            /* +0xE0 */
    s16 mount2_y;           /* +0xE2 */
    s16 mount2_z;           /* +0xE4 */
    u8 pad_E6[2];
};
typedef char tm2_car_stats_block_size[(sizeof(struct TM2CarStatsBlock) == 0xE8) ? 1 : -1];
TM2_FIELD_AT(tm2_car_stats_hp_at, struct TM2CarStatsBlock, hp, 0xA4);

/* Wheel contacts (car_wheels.c). */
struct TM2CarWheel {
    /* -- raw-offset pass 2: names from car_wheels.c -- */
    u8 on;                  /* +0x00 over a surface */
    u8 touch;               /* +0x01 on the ground */
    u8 f02;                 /* +0x02 (cleared by wheelBlockReset) */
    u8 f03;                 /* +0x03 pushes the car (carSpinOutUpdate) */
    u8 pad_04;
    u8 f05;                 /* +0x05 */
    u8 sway_up;             /* +0x06 sway moving towards shake (wheelSwayUpdate) */
    u8 clamped;             /* +0x07 held at the axle limit (set/cleared by carBodyAttitudeUpdate);
                             * car_contacts.c reads wheel 0's to pick the push side */
    u8 moving;              /* +0x08 surface moves */
    u8 f09;                 /* +0x09 no wheel effects */
    s16 surface;            /* +0x0A */
    s16 next_surface;       /* +0x0C found by the probe */
    s16 object;             /* +0x0E moving surface id */
    s16 kind;               /* +0x10 ground kind (worldSectionGroundClass) */
    s16 f12;                /* +0x12 */
    s16 fall;               /* +0x14 */
    s16 level;              /* +0x16 surface height / sink offset */
    /* -- end -- */
    s16 crush;              /* +0x18 crush contact intensity */
    /* -- raw-offset pass 2 -- */
    s16 f1A;                /* +0x1A (cleared by wheelBlockReset) */
    s16 f1C;                /* +0x1C height of pos (carUpdateContacts) */
    s16 shake;              /* +0x1E random crash shake (carCrashStart); upper sway turn point */
    s16 sway_low;           /* +0x20 lower sway turn point (wheelSwayUpdate) */
    s16 sway;               /* +0x22 suspension offset (wheelSwayUpdate) */
    s16 h;                  /* +0x24 */
    s16 f26;                /* +0x26 */
    s16 depth;              /* +0x28 */
    s16 land_z;             /* +0x2A */
    s16 f2C;                /* +0x2C bounce */
    u8 pad_2E[2];
    s32 corner[3];          /* +0x30 contact point in car space */
    /* -- end -- */
    s32 pos[3];             /* +0x3C */
};
typedef char tm2_car_wheel_size[(sizeof(struct TM2CarWheel) == 0x48) ? 1 : -1];
struct TM2CarWheels {
    /* -- raw-offset pass 2: names from car_wheels.c -- */
    s16 surface;            /* +0x00 */
    s16 tilt;               /* +0x02 */
    s32 level;              /* +0x04 */
    s32 step;               /* +0x08 highest climbable step */
    s32 move[3];            /* +0x0C car movement this frame */
    /* -- end -- */
    struct TM2CarWheel wheel[4];    /* +0x18 */
};
typedef char tm2_car_wheels_size[(sizeof(struct TM2CarWheels) == 0x138) ? 1 : -1];

/* Motion block (car_motion.c). */
struct TM2CarMotion {
    s16 steer;              /* +0x00 steering angle */
    s16 f02;                /* +0x02 landing spin-correction target */
    s16 f04;                /* +0x04 */
    u8 pad_06[2];
    s16 vel0;               /* +0x08 */
    s16 speed;              /* +0x0A signed, negative = reverse */
    s16 vel2;               /* +0x0C */
    u8 pad_0E[2];
    s16 spin0;              /* +0x10 */
    s16 spin1;              /* +0x12 */
    s16 yaw_rate;           /* +0x14 */
    u8 pad_16[2];
    s16 vel[3];             /* +0x18 (car_collision.c: vel[1] forward) */
    u8 pad_1E[2];
    s32 pos[3];             /* +0x20 */
    s32 rot[3];             /* +0x2C pitch, roll, yaw */
    s32 body_rot[3];        /* +0x38 visible body angles */
    s32 body_spin[3];       /* +0x44 */
    struct TM2CarMatrix rot_m;      /* +0x50 from rot */
    struct TM2CarMatrix body_m;     /* +0x70 */
    struct TM2CarMatrix body_rot_m; /* +0x90 */
    s32 fB0[3];             /* +0xB0 pos when placed (humanCarPlaceAtStart) */
    u8 pad_BC[8];
    s32 fC4;                /* +0xC4 (car_contacts.c, car_collision.c) */
    struct TM2CarMatrix fC8_m;      /* +0xC8 identity when placed */
    s32 fE8;                /* +0xE8 yaw rate step while wheels push (pass 2) */
};
typedef char tm2_car_motion_size[(sizeof(struct TM2CarMotion) == 0xEC) ? 1 : -1];

/* Burning effect attached to a car (weapons +0x44). */
struct TM2FireEffect {
    u8 pad_00[0xC];
    s16 active;             /* +0x0C 0 = put out */
};

/* Mr. Slam / spin-out controller record (weapons +0x48). */
struct TM2CarSlam {
    u8 active;              /* +0x00 */
    u8 swing;               /* +0x01 */
    u8 grab;                /* +0x02 */
    /* -- weapons/camera pass (weapon_basic.c, weapon_slam_swing_draft.c,
     *    weapon_arcs_draw.c): +0x03..+0x06, +0x08..+0x0D, +0x10 -- */
    u8 f03;                 /* +0x03 grab: arm lowering / swing: claw closing */
    u8 f04;                 /* +0x04 swing: arm swinging */
    u8 f05;                 /* +0x05 swing: claw holding (timer +0x10) */
    u8 f06;                 /* +0x06 grab: claw opening */
    /* -- end -- */
    u8 spin_pending;        /* +0x07 */
    /* -- weapons/camera pass -- */
    u8 f08;                 /* +0x08 grab: target lifted */
    u8 f09;                 /* +0x09 grab: releasing */
    u8 f0A;                 /* +0x0A grab: drop the target */
    u8 half_damage;         /* +0x0B slam damage halved */
    s16 hold;               /* +0x0C grab: damage steps left */
    /* -- end -- */
    s16 spin_target;        /* +0x0E object, -1 = none */
    u16 hold_timer;         /* +0x10 swing: ticks held (weapons/camera pass) */
};
TM2_FIELD_AT(tm2_car_slam_hold_timer_at, struct TM2CarSlam, hold_timer, 0x10);

/* Weapons block. Ammo is per weapon kind: 0 = specials, 1..7 pickups. */
struct TM2CarWeapons {
    s32 infinite;           /* +0x00 cheat: infinite weapons and turbo */
    u8 invisible;           /* +0x04 */
    u8 rear_fire;           /* +0x05 next shot fires backwards */
    u8 drop_armed;          /* +0x06 kind 3 toggle */
    u8 gun_side;            /* +0x07 alternates the muzzle side */
    u8 mega_guns;           /* +0x08 */
    u8 mega_napalm;         /* +0x09 */
    u8 fire_chance;         /* +0x0A AI aggression */
    s8 max_specials;        /* +0x0B */
    u8 f0C;                 /* +0x0C */
    s8 selected;            /* +0x0D weapon kind */
    s16 ammo[10];           /* +0x0E pickup weapon counts by kind (0 = specials) */
    s16 held;               /* +0x22 pickup weapons held */
    s16 capacity;           /* +0x24 */
    s16 fire_cooldown;      /* +0x26 */
    s16 gun_cooldown;       /* +0x28 */
    s16 f2A;                /* +0x2A */
    s16 recharge_period;    /* +0x2C frames per special */
    s16 f2E;                /* +0x2E */
    s16 energy;             /* +0x30 */
    s16 turbo;              /* +0x32 */
    s16 invis_timer;        /* +0x34 */
    s16 f36;                /* +0x36 */
    u8 *weights9;           /* +0x38 AI weapon pick weights, 9 entries (aiCarSetupProfile) */
    u8 *weights10;          /* +0x3C AI weights, 10 entries */
    struct TM2FireEffect *f40;  /* +0x40 second effect object (put out on reset) */
    struct TM2FireEffect *fire_effect;  /* +0x44 burning effect object */
    struct TM2CarSlam *slam;    /* +0x48 */
};
typedef char tm2_car_weapons_size[(sizeof(struct TM2CarWeapons) == 0x4C) ? 1 : -1];

/* Contact record, 36 bytes (car_contacts.c, car_collision.c,
 * car_contact_resolve.c). */
struct TM2ContactRec {
    u8 f00;                 /* +0x00 valid */
    u8 f01;                 /* +0x01 */
    u8 f02;                 /* +0x02 */
    s8 side;                /* +0x03 side hit: probe index (car_contacts.c: 0..3 corners,
                             * 4 front, 5 top); -1 for a car-car hit */
    s16 damage;             /* +0x04 passed to carApplyDamage; also the spin-out strength */
    s16 speed;              /* +0x06 speed the car is given (hit speed) */
    s16 angle;              /* +0x08 push direction */
    s16 f0A;                /* +0x0A contact direction */
    s16 turn;               /* +0x0C */
    s16 force;              /* +0x0E probe force (carContactResolve) */
    s16 f10;                /* +0x10 turn kick */
    s16 push;               /* +0x12 push speed, copied to TM2CarContact.push */
    s16 other;              /* +0x14 car hit */
    s16 f16;                /* +0x16 */
    s32 pos[3];             /* +0x18 */
};
typedef char tm2_contact_rec_size[(sizeof(struct TM2ContactRec) == 36) ? 1 : -1];

/* Contact probe slot, 32 bytes (car_contacts.c, car_contact_resolve.c,
 * car_object_contact.c). */
struct TM2ContactProbe {
    s32 active;             /* +0x00 */
    s32 pos[3];             /* +0x04 */
    s32 force;              /* +0x10 */
    struct TM2Object *obj;  /* +0x14 object touched */
    s32 f18;                /* +0x18 */
    s16 surface;            /* +0x1C */
    s16 material;           /* +0x1E pickup / hazard id */
};
typedef char tm2_contact_probe_size[(sizeof(struct TM2ContactProbe) == 32) ? 1 : -1];

/* Contact block (human +0x234, AI +0xD8: TM2HumanRecord.contact /
 * TM2AIRecord.contact): a head, twelve probe slots, then the contact
 * records (car_contacts.c). Code indexed by probe slot (0..11) treats
 * ground[] and object[] as one array of twelve records (ground[i]). */
struct TM2CarContact {
    u8 f00;                 /* +0x00 */
    u8 f01;                 /* +0x01 set by carRestart (human) */
    u8 in_contact;          /* +0x02 */
    u8 f03;                 /* +0x03 */
    u8 f04;                 /* +0x04 */
    u8 f05;                 /* +0x05 */
    s16 timer;              /* +0x06 push timer */
    s16 f08;                /* +0x08 target heading */
    s16 f0A;                /* +0x0A */
    s16 last;               /* +0x0C last car hit */
    s16 f0E;                /* +0x0E hit strength (human: cleared every frame, car_tick.c) */
    s16 free_frames;        /* +0x10 frames without contact */
    s16 push;               /* +0x12 push speed */
    s16 other;              /* +0x14 */
    u8 pad_16[6];
    struct TM2ContactProbe probe[12];   /* +0x01C */
    struct TM2ContactRec ground[6]; /* +0x19C */
    struct TM2ContactRec object[6]; /* +0x274 */
    struct TM2ContactRec hit;       /* +0x34C combined */
};
typedef char tm2_car_contact_size[(sizeof(struct TM2CarContact) == 0x370) ? 1 : -1];
TM2_FIELD_AT(tm2_car_contact_hit_at, struct TM2CarContact, hit, 0x34C);
TM2_FIELD_AT(tm2_car_contact_probe_at, struct TM2CarContact, probe, 0x01C);
TM2_FIELD_AT(tm2_car_contact_ground_at, struct TM2CarContact, ground, 0x19C);

/* AI driver (route following) state, AI record +0x448..+0x4FB
 * (TM2AIRecord.drive; also reached through TM2AIDriveView). Names
 * from ai_steps.c and ai_route_nodes.c (aiChooseNextNode, aiAdvanceRouteNode). */
struct TM2AIDrive {
    u8 f448;                /* +0x448 */
    u8 f449;                /* +0x449 */
    u8 diagonal;            /* +0x44A segment not axis-aligned */
    u8 axis_x;              /* +0x44B segment runs along y (|dx| < 40) */
    u8 axis_y;              /* +0x44C segment runs along x (|dy| < 40) */
    u8 next_diagonal;       /* +0x44D the same three for the next segment */
    u8 next_axis_x;         /* +0x44E */
    u8 next_axis_y;         /* +0x44F */
    u8 side_neg;            /* +0x450 target point shifted by -side_offset */
    u8 side_pos;            /* +0x451 shifted by +side_offset */
    u8 sharp_turn;          /* +0x452 */
    u8 f453;                /* +0x453 */
    u8 route_change;        /* +0x454 next node in another route group */
    u8 link_pending;        /* +0x455 link_node is a pending link target */
    u8 f456;                /* +0x456 */
    u8 f457;                /* +0x457 */
    u8 f458;                /* +0x458 */
    u8 f459;                /* +0x459 */
    u8 f45A;                /* +0x45A */
    u8 f45B;                /* +0x45B */
    u8 f45C;                /* +0x45C */
    u8 f45D;                /* +0x45D */
    u8 f45E;                /* +0x45E */
    u8 f45F;                /* +0x45F */
    u8 f460;                /* +0x460 */
    u8 pad_461;
    s16 node_prev;          /* +0x462 */
    s16 node;               /* +0x464 current route node */
    s16 node_next;          /* +0x466 target node */
    s16 link_node;          /* +0x468 */
    s16 route;              /* +0x46A route group */
    s16 away;               /* +0x46C ticks moving away from the target */
    s16 closer;             /* +0x46E ticks closing in */
    s16 node_repeat;        /* +0x470 times aiPickLinkNode picked node_prev again */
    s16 f472;               /* +0x472 */
    s16 f474;               /* +0x474 */
    s16 backup_time;        /* +0x476 */
    s16 side_offset;        /* +0x478 */
    s16 lane;               /* +0x47A */
    s16 next_lane;          /* +0x47C */
    s16 turned;             /* +0x47E */
    s16 f480;               /* +0x480 */
    s16 f482;               /* +0x482 */
    s16 f484;               /* +0x484 segment speed */
    s16 f486;               /* +0x486 */
    s16 heading;            /* +0x488 heading of the current segment */
    s16 next_heading;       /* +0x48A heading of the next segment */
    s16 turn;               /* +0x48C next_heading - heading */
    s16 goal2_angle;        /* +0x48E */
    s16 f490;               /* +0x490 */
    s16 f492;               /* +0x492 */
    s16 goal_angle;         /* +0x494 */
    u8 pad_496[2];
    s32 f498;               /* +0x498 */
    s32 delta[2];           /* +0x49C node_prev -> node */
    s32 next_delta[2];      /* +0x4A4 node -> node_next (aiSetSegmentSpeed) */
    s32 look_ahead;         /* +0x4AC */
    s32 f4B0;               /* +0x4B0 */
    s32 corner[2];          /* +0x4B4 */
    s32 next_corner[2];     /* +0x4BC corner point of the next segment */
    s32 goal_dist;          /* +0x4C4 */
    s32 goal_dist_prev;     /* +0x4C8 */
    s32 goal2_dist;         /* +0x4CC */
    s32 seg_dist;           /* +0x4D0 length of the current segment */
    s32 next_dist;          /* +0x4D4 length of the next segment */
    s32 f4D8;               /* +0x4D8 */
    s32 goal_prev[2];       /* +0x4DC */
    s32 goal[2];            /* +0x4E4 */
    s32 goal2[2];           /* +0x4EC */
    s32 aim[2];             /* +0x4F4 */
};
typedef char tm2_ai_drive_size[(sizeof(struct TM2AIDrive) == 0xB4) ? 1 : -1];

/* Whole human record seen through its blocks (same storage as TM2HumanCar). */
struct TM2HumanRecord {
    s16 player;             /* +0x000 */
    s16 f002;               /* +0x002 */
    s16 slot;               /* +0x004 */
    /* -- weapons/camera pass (view_chase_camera_draft.c): chase camera, +0x006..+0x08B -- */
    s16 cam_ease;           /* +0x006 camera ease rate */
    u8 pad_008[4];
    s16 cam_rot[3];         /* +0x00C camera pitch, roll, yaw */
    u8 pad_012[2];
    s32 cam_pos[3];         /* +0x014 camera position */
    u8 pad_020[0xC];
    s32 cam_goal[3];        /* +0x02C camera position goal */
    s32 cam_look[3];        /* +0x038 camera look-at point */
    s32 cam_chase[3];       /* +0x044 offset, views 2 and 7 */
    s32 cam_offset3[3];     /* +0x050 offset, view 3 */
    s32 cam_offset[3];      /* +0x05C offset, other full-screen views */
    s32 cam_split1[3];      /* +0x068 offset, split mode 1 */
    s32 cam_split2[3];      /* +0x074 offset, split mode 2 */
    s32 cam_look_offset[3]; /* +0x080 look-at point in car space */
    /* -- end -- */
    s32 speed_change;       /* +0x08C speed gained this frame */
    s32 type;               /* +0x090 car type 0x10..0xF0 */
    u8 f094;                /* +0x094 cleared by humanCarPlaceAtStart */
    u8 f095;                /* +0x095 cleared by humanCarPlaceAtStart */
    u8 pad_096[6];
    struct TM2CarState state;       /* +0x09C */
    s32 *fx;                /* +0x0CC effect object put out when wrecked (pass 2) */
    struct TM2CarInput input;       /* +0x0D0 */
    /* -- weapons/camera pass: +0x0F8..+0x113 -- */
    u8 pad_0F8[0x14];
    s32 view;               /* +0x10C camera view 2..7 (7 = watch an AI car) */
    s32 view_last;          /* +0x110 last view mode below 5 (carSetViewMode) */
    /* -- end -- */
    struct TM2CarAim aim;           /* +0x114 */
    struct TM2CarStatsBlock stats;  /* +0x14C */
    struct TM2CarContact contact;   /* +0x234 (car_wheels.c, car_contacts.c) */
    struct TM2CarWheels wheels;     /* +0x5A4 */
    struct TM2CarMotion motion;     /* +0x6DC */
    struct TM2CarWeapons weapons;   /* +0x7C8 */
};
typedef char tm2_human_record_size[(sizeof(struct TM2HumanRecord) == 0x814) ? 1 : -1];
TM2_FIELD_AT(tm2_human_record_motion_at, struct TM2HumanRecord, motion, 0x6DC);

/* Whole AI record seen through its blocks (same storage as TM2AICar). */
struct TM2AIRecord {
    /* -- weapons/camera pass: +0x000..+0x003 -- */
    u8 f000;                /* +0x000 set when started (aiCarStart) */
    s8 f001;                /* +0x001 mines need 2; 1 sets +0x458 on some fires */
    u8 pad_002[2];
    /* -- end -- */
    s16 index;              /* +0x004 index in aiCars */
    s16 slot;               /* +0x006 */
    u8 pad_008[4];
    s16 f00C;               /* +0x00C cleared by aiCarStart */
    u8 pad_00E[4];
    s16 f012;               /* +0x012 (index + 1) * 60 at start (aiCarStart) */
    u8 pad_014[2];
    /* -- raw-offset pass 2: names from ai_steps.c -- */
    s16 range_near;         /* +0x016 target distance thresholds */
    s16 range_1;            /* +0x018 */
    s16 range_2;            /* +0x01A */
    s16 range_3;            /* +0x01C */
    s16 range_far;          /* +0x01E */
    /* -- end -- */
    s16 target_angle;       /* +0x020 bearing of the target */
    s16 f022;               /* +0x022 type 0xF0: heading to blend towards (aiCarDrive) */
    u8 pad_024[4];
    s32 target_dist;        /* +0x028 */
    s32 f02C;               /* +0x02C */
    s32 type;               /* +0x030 car type 0x10..0xF0 */
    u8 active;              /* +0x034 */
    u8 pad_035[2];
    u8 f037;                /* +0x037 */
    u8 f038;                /* +0x038 */
    u8 f039;                /* +0x039 */
    u8 f03A;                /* +0x03A */
    u8 pad_03B[4];
    u8 spun;                /* +0x03F body turned > 512 from heading (car_physics.c) */
    u8 f040;                /* +0x040 heading for another route (aiFollowRouteTarget) */
    u8 f041;                /* +0x041 wall bumps start a 30-tick back-up (aiWallBumpCount) */
    u8 pad_042[2];
    struct TM2CarInput input;       /* +0x044 */
    struct TM2CarState state;       /* +0x06C */
    struct TM2CarAim aim;           /* +0x09C */
    s32 f0D4;               /* +0x0D4 target behind */
    struct TM2CarContact contact;   /* +0x0D8 */
    struct TM2AIDrive drive;        /* +0x448 route following */
    struct TM2CarStatsBlock stats;  /* +0x4FC */
    struct TM2CarWheels wheels;     /* +0x5E4 */
    struct TM2CarMotion motion;     /* +0x71C */
    struct TM2CarWeapons weapons;   /* +0x808 */
};
typedef char tm2_ai_record_size[(sizeof(struct TM2AIRecord) == 0x854) ? 1 : -1];
TM2_FIELD_AT(tm2_ai_record_motion_at, struct TM2AIRecord, motion, 0x71C);
TM2_FIELD_AT(tm2_human_record_contact_at, struct TM2HumanRecord, contact, 0x234);
TM2_FIELD_AT(tm2_ai_record_contact_at, struct TM2AIRecord, contact, 0x0D8);
TM2_FIELD_AT(tm2_ai_record_drive_at, struct TM2AIRecord, drive, 0x448);

/* Record getters (car_records.c, which defines TM2_OWN_CAR_GETTERS and
 * declares them through its own record views). Functions that take either
 * kind of record keep void * and a u8 human flag. */
#ifndef TM2_OWN_CAR_GETTERS
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2HumanRecord *humanCarGet(s32 index);
extern struct TM2AIRecord *aiCarGet(s32 index);
#ifdef __cplusplus
}
#endif
#endif
/* Contact block helpers (misc_one_liners.c). */
#ifdef __cplusplus
extern "C" {
#endif
extern void carPushClear(struct TM2CarContact *contact);
extern void carPushTimerArm(struct TM2CarContact *contact);
#ifdef __cplusplus
}
#endif
/* -- weapons/camera pass: offset checks -- */
TM2_FIELD_AT(tm2_human_record_cam_ease_at, struct TM2HumanRecord, cam_ease, 0x006);
TM2_FIELD_AT(tm2_human_record_cam_pos_at, struct TM2HumanRecord, cam_pos, 0x014);
TM2_FIELD_AT(tm2_human_record_cam_look_offset_at, struct TM2HumanRecord, cam_look_offset, 0x080);
TM2_FIELD_AT(tm2_human_record_view_at, struct TM2HumanRecord, view, 0x10C);
TM2_FIELD_AT(tm2_ai_record_f001_at, struct TM2AIRecord, f001, 0x001);
TM2_FIELD_AT(tm2_object_visible_at, struct TM2Object, visible, 0x008);
TM2_FIELD_AT(tm2_object_matrix_at, struct TM2Object, matrix, 0x080);
/* -- end -- */
/* -- raw-offset pass 2: track vehicle record (trackVehicleRecords, 56 bytes;
 *    see also track_vehicle_ground.c) -- */
struct TM2TrackVehicle {
    u8 pad_00[0xC];
    s32 busy;               /* +0x0C */
    s32 front[3];           /* +0x10 */
    s32 rear[3];            /* +0x1C */
    u8 pad_28[0xC];
    struct TM2Object *obj;  /* +0x34 rot[2] = heading */
};
typedef char tm2_track_vehicle_size[(sizeof(struct TM2TrackVehicle) == 56) ? 1 : -1];
/* TM2AIDriveView: the AI record up to its drive block (TM2AIRecord.drive),
 * for code written against the drive block alone. */
struct TM2AIDriveView {
    u8 pad_000[0x448];
    struct TM2AIDrive drive;        /* +0x448 */
};
TM2_FIELD_AT(tm2_ai_drive_view_end, struct TM2AIDriveView, drive.aim[1], 0x4F8);
TM2_FIELD_AT(tm2_ai_record_stats_at, struct TM2AIRecord, stats, 0x4FC);
TM2_FIELD_AT(tm2_ai_record_range_far_at, struct TM2AIRecord, range_far, 0x01E);
TM2_FIELD_AT(tm2_car_state_route_at, struct TM2CarState, x2C.route, 0x2C);
TM2_FIELD_AT(tm2_human_record_fx_at, struct TM2HumanRecord, fx, 0x0CC);

TM2_FIELD_AT(tm2_car_stats_spawn_node_at, struct TM2CarStatsBlock, spawn_node, 0xD4);
TM2_FIELD_AT(tm2_car_motion_vel_at, struct TM2CarMotion, vel, 0x18);
TM2_FIELD_AT(tm2_car_wheel_sway_at, struct TM2CarWheel, sway, 0x22);
TM2_FIELD_AT(tm2_car_weapons_f40_at, struct TM2CarWeapons, f40, 0x40);
TM2_FIELD_AT(tm2_car_wheel_corner_at, struct TM2CarWheel, corner, 0x30);
TM2_FIELD_AT(tm2_car_stats_wreck_timer_at, struct TM2CarStatsBlock, wreck_timer, 0x26);
TM2_FIELD_AT(tm2_ai_record_spun_at, struct TM2AIRecord, spun, 0x03F);
TM2_FIELD_AT(tm2_car_weapons_weights9_at, struct TM2CarWeapons, weights9, 0x38);
TM2_FIELD_AT(tm2_car_aim_pitch_at, struct TM2CarAim, pitch, 0x30);
TM2_FIELD_AT(tm2_car_aim_shake_amp_x_at, struct TM2CarAim, shake_amp_x, 0x28);
TM2_FIELD_AT(tm2_car_state_f27_at, struct TM2CarState, f27, 0x27);
TM2_FIELD_AT(tm2_car_wheel_f03_at, struct TM2CarWheel, f03, 0x03);
TM2_FIELD_AT(tm2_car_motion_fE8_at, struct TM2CarMotion, fE8, 0xE8);
TM2_FIELD_AT(tm2_object_rot_at, struct TM2Object, rot, 0x0A0);
/* -- end -- */
/* ===================== end of car record blocks ===================== */

#include "semantic/car_calls.h"

#endif
