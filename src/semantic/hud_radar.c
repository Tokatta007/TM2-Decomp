/* Game code (car tuning, HUD radar). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* Car stats block (human record +0x14C, AI record +0x4FC); see docs/CAR_STATS.md. */
typedef struct {
    u8 f00;                  /* 0x00: 0 = AI car */
    u8 pad01[0x51];
    s16 top_speed;           /* 0x52 */
    s16 reverse_speed;       /* 0x54 */
    s16 slide_speed;         /* 0x56 */
    s16 cruise_speed;        /* 0x58 */
    s16 roll_speed;          /* 0x5A */
    s16 brake;               /* 0x5C */
    s16 steer_high;          /* 0x5E */
    s16 steer_high_fps;      /* 0x60 */
    s16 steer_low_fps;       /* 0x62 */
    s16 steer_lock_fps;      /* 0x64 */
    s16 steer_lock_half;     /* 0x66 */
    s16 steer_lock;          /* 0x68 */
    s16 body_lean;           /* 0x6A */
    s16 steer_low;           /* 0x6C */
    s16 handbrake_lock;      /* 0x6E */
    s16 f70;                 /* 0x70 */
    s16 f72;                 /* 0x72 */
    s16 lean_angle;          /* 0x74 */
    s16 mult;                /* 0x76: speed multiplier % */
    u8 pad78[0x34];
    s32 accel;               /* 0xAC */
    s32 accel_top;           /* 0xB0 */
    s32 accel_launch;        /* 0xB4 */
    s32 top_speed_u;         /* 0xB8 */
    s32 fBC;                 /* 0xBC */
    s32 slide_speed_u;       /* 0xC0 */
    s32 cruise_speed_u;      /* 0xC4 */
    s32 reverse_speed_u;     /* 0xC8 */
    s32 brake_decel;         /* 0xCC */
    s32 roll_speed_u;        /* 0xD0 */
} TM2CarStatsD;

extern s32 timerGetFrameRate(void);
#include "semantic/world.h"

/* @800FF878: derive a car's runtime stats (accelerations, steering rates, speed
 * caps in internal units) from its base stats, the speed multiplier (+118) and
 * the frame rate. */
void __attribute__((section(".text_800FF878"))) carStatsDerive(TM2CarStatsD *s)
{
    s32 fps;

    s->accel_top = s->accel;
    s->accel_launch = s->accel_top * 5 * s->mult / 100;
    s->brake_decel = s->accel_top * s->brake;
    if (s->f00 == 0)
        s->accel_launch = s->accel_launch / 2;
    s->steer_high_fps = s->steer_high;
    s->steer_low_fps = s->steer_high_fps * s->steer_low * s->mult / 100;
    fps = (s16)timerGetFrameRate();
    if (fps != 0 && fps != 60) {
        s->accel_top = s->accel_top * 60 / fps;
        s->accel_launch = s->accel_launch * 60 / fps;
        s->brake_decel = s->brake_decel * 60 / fps;
        s->steer_high_fps = s->steer_high_fps * 60 / fps;
        s->steer_low_fps = s->steer_low_fps * 60 / fps;
    }
    s->fBC = s->accel_launch * 10;
    s->f70 = s->steer_low_fps * 5;
    s->f72 = s->steer_low_fps * 2;
    s->lean_angle = s->body_lean * 682 / 100;
    s->steer_lock_fps = s->steer_high_fps * s->steer_lock;
    if (s->f00 == 0)
        s->steer_lock_fps = s->steer_lock_fps * 10;
    s->steer_lock_half = s->steer_lock_fps / 2;
    s->top_speed_u = s->top_speed * 19 * (s16)timerGetFrameTicks() / 100 * (s->mult << 5) / 100;
    s->reverse_speed_u = s->reverse_speed * 19 * (s16)timerGetFrameTicks() / 100 * (s->mult << 5) / 100;
    s->slide_speed_u = s->slide_speed * 19 * (s16)timerGetFrameTicks() / 100 * (s->mult << 5) / 100;
    s->cruise_speed_u = s->cruise_speed * 19 * (s16)timerGetFrameTicks() / 100 * (s->mult << 5) / 100;
    s->roll_speed_u = s->roll_speed * 19 * (s16)timerGetFrameTicks() / 100 * (s->mult << 5) / 100;
}

/* Radar overlay, one 880-byte record per view at 0x801837C8 (see ai_route_setup.c 800FC5C4); with more
 * blips (players.h TM2_MAX_RADAR_BLIPS, TM2_PORT_CARS) the lines grow and the fields after them move. */
#include "semantic/players.h"
typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0, x1, y1, x2, y2, x3, y3; } TM2RadarPolyF4;
typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0, x1, y1; } TM2RadarLineF2;
typedef struct {
    TM2RadarPolyF4 back;     /* 0x000 */
    TM2RadarPolyF4 frame;    /* 0x018 */
    TM2RadarLineF2 lines[TM2_HUD_RADAR_LINES];/* 0x030: blips, double-buffered by buf (48 for 12 blips) */
    TM2RadarLineF2 l0, l1;   /* 0x330: centre cross */
    u32 dm[3];               /* 0x350 DR_MODE */
    u8 off_up, off_down, off_right, off_left; /* 0x35C: off-radar arrows */
    s32 count;               /* 0x360: lines used (2 per blip) */
    s32 f364;
    s32 scale;               /* 0x368 */
    s32 buf;                 /* 0x36C */
} TM2Radar;
#include "semantic/players.h"
#include "semantic/car_numbers.h"   /* request 70: portCarProfileRadar */
extern TM2Radar hudRadars[];
extern s32 viewGetSplitMode(void);
extern s32 rcos(s32);
extern s32 rsin(s32);

/* @800FC928: plot a car of `type` at world offset pos (rotated by angle) on view
 * idx radar as a 2x2 blip coloured by car type; off-radar cars set an edge arrow flag. */
void __attribute__((section(".text_800FC928"))) hudRadarPlotCar(s32 type, s32 *pos, s32 angle, s32 idx)
{
    s32 x = pos[0];
    s32 y = pos[1];
    s32 sx, sy, n, o;
    TM2RadarLineF2 *a, *b;

    sx = (x * rcos(angle) - y * rsin(angle)) / hudRadars[idx].scale;
    sy = -(x * rsin(angle) + y * rcos(angle)) / hudRadars[idx].scale;
    if ((u32)(sx + 36) < 72) {
        if ((u32)(sy + 22) >= 44 || hudRadars[idx].count + 2 >= 2 * TM2_MAX_RADAR_BLIPS + 1) {
            if (sy > 0)
                hudRadars[idx].off_down = 1;
            else
                hudRadars[idx].off_up = 1;
            return;
        }
    } else {
        if (sx < 0 && __builtin_abs(sx) > __builtin_abs(sy))
            hudRadars[idx].off_left = 1;
        else if (sx > 0 && __builtin_abs(sx) > __builtin_abs(sy))
            hudRadars[idx].off_right = 1;
        else if (sy > 0)
            hudRadars[idx].off_down = 1;
        else
            hudRadars[idx].off_up = 1;
        return;
    }
    if (viewGetSplitMode() != 0 && viewGetSplitMode() == 1) {
        sx += 44;
        sy += 43;
    } else {
        sx += 43;
        sy += 41;
    }
    n = hudRadars[idx].count * 2;
    o = hudRadars[idx].buf;
    a = &hudRadars[idx].lines[o + n];
    n += 2;
    b = &hudRadars[idx].lines[o + n];
    a->x0 = sx;
    a->y0 = sy;
    a->x1 = sx + 1;
    a->y1 = sy;
    b->x0 = sx;
    b->y0 = sy + 1;
    b->x1 = sx + 1;
    b->y1 = sy + 1;
#define BLIP(r, g, bl) (a->r0 = (r), a->g0 = (g), a->b0 = (bl), b->r0 = (r), b->g0 = (g), b->b0 = (bl))
#ifdef TM2_PORT_CAR_PROFILE
    /* request 70 (CAR_PROFILES.md): worldUpdate passes the type with the car's slot + 1 in bits 16..23; a mod car's
       profile colour comes back as TM2_PORT_PROFILE_RADAR_KEY | 0xRRGGBB, any other car as its type */
    if (type >> 16)
        type = portCarProfileRadar(((type >> 16) & 0xFF) - 1, type & 0xFFFF);
    if (type & TM2_PORT_PROFILE_RADAR_KEY)
        BLIP((type >> 16) & 0xFF, (type >> 8) & 0xFF, type & 0xFF);
    else
#endif
    switch (type) {
    case 0x10: BLIP(255, 127, 80); break;
    case 0x20: BLIP(255, 40, 0); break;
    case 0x30: BLIP(51, 161, 201); break;
    case 0x40: BLIP(151, 72, 45); break;
    case 0x50: BLIP(200, 0, 0); break;
    case 0x60: BLIP(50, 205, 50); break;
    case 0x70: BLIP(255, 63, 156); break;
    case 0x80: BLIP(255, 255, 255); break;
    case 0x90: BLIP(255, 85, 0); break;
    case 0xA0: BLIP(75, 75, 255); break;
    case 0xB0: BLIP(125, 125, 125); break;
    case 0xC0: BLIP(255, 255, 0); break;
    case 0xD0: BLIP(160, 32, 240); break;
    case 0xF0: BLIP(107, 142, 35); break;
    default: BLIP(80, 80, 80); break;
    }
#undef BLIP
    hudRadars[idx].count += 2;
}

/* Off-radar arrow sprite + its draw mode, per view (32 bytes). */
typedef struct {
    u32 tag; u8 r0, g0, b0, code; s16 x0, y0; u8 u0, v0; u16 clut; s16 w, h;
    u32 dm[3];
} TM2RadarArrow;
/* HUD image record (8 bytes): texture u/v/size, tpage, clut. */
typedef struct { u8 u, v, w, h; u16 tpage, clut; } TM2HudImage;
extern TM2RadarArrow hudRadarArrowUpPrims[], hudRadarArrowDownPrims[], hudRadarArrowLeftPrims[], hudRadarArrowRightPrims[];
extern TM2HudImage hudRadarArrowUpSprites[], hudRadarArrowDownSprites[], hudRadarArrowLeftSprites[], hudRadarArrowRightSprites[];
extern void *renderGetCurrentFrame(void);
extern void SetDrawMode(void *, s32, s32, s32, void *);
extern void SetSemiTrans(void *, s32);
extern void AddPrim(u32 *, void *);
extern void hudRadarSwapBuffers(s32);
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"   /* request 48 stage 3 STEP_SPLIT: the radar swap in the step */
#endif

typedef struct { unsigned addr : 24; unsigned len : 8; } TM2RadarTag;
#define RADAR_ADD(ot, p) \
    (((TM2RadarTag *)(p))->addr = ((TM2RadarTag *)(ot))->addr, ((TM2RadarTag *)(ot))->addr = (u32)(p))
/* A plain block with its own draw-mode pointer d, as retail: inside a do/while(0)
 * flow weights every reference by the loop depth, which ranks the shared idx*8 index
 * above ot and d in global allocation (s1/s3 swap). */
#define RADAR_ARROW(p, img, x, y, du) \
    { \
        u32 *d = (p)->dm; \
        ((TM2RadarTag *)(p))->len = 4; \
        (p)->code = 0x64; \
        (p)->r0 = 0x80; (p)->g0 = 0x80; (p)->b0 = 0x80; \
        SetDrawMode(d, 0, 0, 0, 0); \
        (p)->code |= 1; \
        (p)->x0 = (x); \
        (p)->y0 = (y); \
        (p)->w = (img).w; \
        (p)->h = (img).h; \
        SetSemiTrans((p), 1); \
        (p)->u0 = (img).u + (du); \
        (p)->v0 = (img).v; \
        SetDrawMode(d, 0, 0, (img).tpage, 0); \
        (p)->clut = (img).clut; \
        RADAR_ADD(ot, (p)); \
        RADAR_ADD(ot, d); \
    }

/* @800FBE68 (hudDisplayRadar): add view idx radar to the ordering table: edge
 * arrows for off-radar cars, the blips, centre cross, background/frame, draw mode. */
void __attribute__((section(".text_800FBE68"))) hudDisplayRadar(u32 *ot, s32 idx)
{
    s32 i;
    TM2RadarLineF2 *c;

    renderGetCurrentFrame();
    if (hudRadars[idx].off_up) {
        TM2RadarArrow *p = &hudRadarArrowUpPrims[idx];
        RADAR_ARROW(p, hudRadarArrowUpSprites[idx], 37, 12, 1);
    }
    if (hudRadars[idx].off_down) {
        TM2RadarArrow *p = &hudRadarArrowDownPrims[idx];
        RADAR_ARROW(p, hudRadarArrowDownSprites[idx], 37, 63, 1);
    }
    if (hudRadars[idx].off_left) {
        TM2RadarArrow *p = &hudRadarArrowLeftPrims[idx];
        RADAR_ARROW(p, hudRadarArrowLeftSprites[idx], 0, 35, 0);
    }
    if (hudRadars[idx].off_right) {
        TM2RadarArrow *p = &hudRadarArrowRightPrims[idx];
        RADAR_ARROW(p, hudRadarArrowRightSprites[idx], 79, 35, 1);
    }
    for (i = 0; i < hudRadars[idx].count; i++)
        AddPrim(ot, &hudRadars[idx].lines[hudRadars[idx].buf + i * 2]);
    c = &hudRadars[idx].l0;
    AddPrim(ot, c);
    AddPrim(ot, c + 1);
    if (!viewGetSplitMode()) {
        AddPrim(ot, &hudRadars[idx].back);
        AddPrim(ot, &hudRadars[idx].frame);
    } else {
        AddPrim(ot, &hudRadars[idx].back);
    }
    AddPrim(ot, hudRadars[idx].dm);
#ifdef TM2_PORT_ROLLBACK
    /* request 48 stage 3 STEP_SPLIT (ONLINE_ROLLBACK.md 12): the step swaps (portRollbackHudStep, before worldUpdate's
       plots), so re-runs do not pile up blips and every frame of a step shows them */
    if (!portRollbackRadarInStep())
#endif
    hudRadarSwapBuffers(idx);
}
