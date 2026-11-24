/* Game code (800D range). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

#define S8(p, o) (*(s8 *)((u8 *)(p) + (o)))
#define U8(p, o) (*(u8 *)((u8 *)(p) + (o)))
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define U16(p, o) (*(u16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))

/* 60-byte projectile record (table bulletRecords, 40 entries). */
typedef struct {
    s32 pos[3];     /* 0x00 */
    s32 prev[3];    /* 0x0C */
    s32 vel[3];     /* 0x18 */
    s32 owner;      /* 0x24 */
    s32 life;       /* 0x28 */
    s32 power;      /* 0x2C */
    s32 hit;        /* 0x30 */
    u8 kind;        /* 0x34 */
    u8 pad_35[3];
    s32 f38;        /* 0x38 */
} TM2Shot60;

/* 8-byte texture-cell entry (table pointer fxContrailCellTable). */
typedef struct {
    u8 u, v, w, h;
    u16 tpage;
    u16 clut;
} TM2TexCell;

typedef struct {
    s32 x, y, z;
} TM2Vec3;

extern u8 laEvents[];
extern TM2Shot60 bulletRecords[];
extern TM2TexCell *fxContrailCellTable;

extern int rand(void);
extern u8 *moscowEventGet(s32);
#include "semantic/world.h"
extern void laEventStart(s32);
extern s32 bulletFindFree(s32);
extern s32 projectileHitTest(s32 *, s32, s32, s32);
extern u8 *hdpQueryPoint(s32, s32, s32 *, s32 *);
extern void fxSparkOrange(s32 *, u8 arg1);
extern void fxSmokeA6(s32 *, s32);
extern void fxBulletImpact(s32 *, s32);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* round 3 bit 20 (BALANCE.md 13) */
/* bit 20: every connecting bullet counts (no coin flip: the rand() is not called), at half its power with the half
   point carried per target (portBalanceGunHit; -1 with the bit clear: retail's coin flip and power) */
#define GUN_FLIP(s, k) (((k) = portBalanceGunHit((s)->hit, (s)->power)) >= 0 || (rand() & 1))
#define GUN_POWER(s, k) ((k) >= 0 ? (k) : (s)->power)
#else
#define GUN_FLIP(s, k) (rand() & 1)
#define GUN_POWER(s, k) ((s)->power)
#endif
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"    /* request 46 item 4: portVisual bit 4, the shooter's place at the shot */
#endif

#define ABS(x) __builtin_abs(x)
/* Whichever of a/b has the larger magnitude (a on ties). */
#define AMAX(a, b) (ABS(a) < ABS(b) ? (b) : (a))

/* @800D4AAC: arm the 196-byte random event `kind` (moscowEvents table): randomise its four lanes and set the per-kind direction pattern. */
void __attribute__((section(".text_800D4AAC"))) moscowEventStart(s32 kind)
{
    u8 *p = moscowEventGet(kind);
    s32 i;
    s32 n = 4;  /* never read: see below */
    u8 *q;
    u8 *r;
    s16 *t;

#ifdef TM2_PORT_HAZARDS
    if (portArenaOff(TM2_ARENA_MOSCOW_BRIDGES))   /* request 38 G8: the bridges stand (before rand(), sections) */
        return;
#endif
    if (p[1] != 0)
        return;
    if (p[4] != 0)
        return;
    i = 1;
    if (p[0] == 0)
        return;
    q = p + 2;
    r = p + 8;
    p[1] = 1;
    S16(p, 10) = 0;
    p[3] = 30;
    p[2] = 0;
    p[4] = 0;
    do {
        t = (s16 *)(q + 24);
        *t = (rand() >> 9 < 21) ? 20 : rand() >> 9;
        S16(r, 156) = rand() / 8;
        S16(r, 158) = rand() / 8;
        S16(r, 160) = 0xFFF;
        S16(q, 14) = (rand() >> 9) + 40;
        q += 2;
        r += 8;
        (p + i)[5] = 0;
        i++;
    } while (i < 5);
    /* Dead stores to n: CSE then compares `kind == 4` against n's register
     * (s1, live across the rand call) as retail does instead of li v0,4. */
    n = 4;
    S16(p, 22) = (rand() >> 9) + 60;
    switch (kind) {
    case 1:
        S16(p, 124) = -7;
        S16(p, 126) = 0;
        S16(p, 128) = 0;
        S16(p, 132) = 7;
        S16(p, 134) = 0;
        S16(p, 136) = 0;
        S16(p, 140) = -15;
        S16(p, 142) = 0;
        S16(p, 144) = 0;
        S16(p, 148) = -20;
        S16(p, 150) = 0;
        S16(p, 152) = 0;
        break;
    case 2:
        S16(p, 124) = 0;
        S16(p, 126) = 7;
        S16(p, 128) = 0;
        S16(p, 132) = 0;
        S16(p, 134) = -7;
        S16(p, 136) = 0;
        S16(p, 140) = 0;
        S16(p, 142) = 15;
        S16(p, 144) = 0;
        S16(p, 148) = 0;
        S16(p, 150) = 20;
        S16(p, 152) = 0;
        break;
    case 3:
        S16(p, 124) = 7;
        S16(p, 126) = 0;
        S16(p, 128) = 0;
        S16(p, 132) = -7;
        S16(p, 134) = 0;
        S16(p, 136) = 0;
        S16(p, 140) = 15;
        S16(p, 142) = 0;
        S16(p, 144) = 0;
        S16(p, 148) = 20;
        S16(p, 150) = 0;
        S16(p, 152) = 0;
        break;
    case 4:
        S16(p, 124) = 0;
        S16(p, 126) = -7;
        S16(p, 128) = 0;
        S16(p, 132) = 0;
        S16(p, 134) = 7;
        S16(p, 136) = 0;
        S16(p, 140) = 0;
        S16(p, 142) = -15;
        S16(p, 144) = 0;
        S16(p, 148) = 0;
        S16(p, 150) = -20;
        S16(p, 152) = 0;
        break;
    }
    worldSectionDisable(S16(p, 12));
}

/* |largest-magnitude component of pos - c| / 8. */
#define DIST8(pos, c) \
    (ABS(AMAX((pos)[0] - (c)[0], AMAX((pos)[1] - (c)[1], (pos)[2] - (c)[2]))) >> 3)

/* @800D5584: for random events 1..2 (laEvents): trigger (laEventStart) each armed one whose region matches the region of pos, or whose either anchor point is within 100*8 of pos. */
void __attribute__((section(".text_800D5584"))) laEventsTrigger(s32 *pos)
{
    s32 i;
    s32 region;
    u8 *rec;

    region = worldSectionAt(pos[0], pos[1], pos[2]);
    for (i = 1; i < 3; i++) {
        rec = laEvents + i * 196;
        if (rec[0] != 0) {
            if (region == S16(rec, 12) || DIST8(pos, (s32 *)(rec + 68)) < 100
                || DIST8(pos, (s32 *)(rec + 80)) < 100)
                laEventStart(i);
        }
    }
}

/* @800D7A4C: spawn a projectile (bulletRecords slot from bulletFindFree(40)) at `start` moving along `dir`, scaled to 200 units/frame on its major axis. */
void __attribute__((section(".text_800D7A4C"))) bulletSpawn(s32 owner, s32 kind, s32 *dir, s32 *start, s32 power)
{
    s32 slot = bulletFindFree(40);
    TM2Shot60 *s;
    s32 m;
    s32 vel[3];
    s32 life;

    if (slot < 0)
        return;
    s = &bulletRecords[slot];
    m = ABS(AMAX(dir[0], AMAX(dir[1], dir[2])));
    if (m <= 0)
        m = 1;
    if (owner == 1) {
        vel[0] = dir[0] * 200 / m;
        vel[1] = dir[1] * 200 / m;
        vel[2] = dir[2] * 200 / m;
        s->life = 12;
    } else {
        vel[0] = dir[0] * 200 / m;
        vel[1] = dir[1] * 200 / m;
        vel[2] = dir[2] * 200 / m;
        if (owner >= 50)
            s->life = 15;
        else
            s->life = 12;
    }
    s->prev[0] = start[0];
    s->prev[1] = start[1];
    s->prev[2] = start[2];
    s->vel[0] = vel[0];
    s->vel[1] = vel[1];
    s->vel[2] = vel[2];
    s->pos[0] = start[0];
    s->pos[1] = start[1];
    s->pos[2] = start[2];
    s->owner = owner;
    s->kind = kind;
    s->hit = 0;
    s->power = power;
    s->f38 = 0;
#ifdef TM2_PORT_BALANCE
    s->life = portBalanceGunLife(s->life);  /* request 121 item 4 (bit 9, BALANCE.md 25): 25 % further */
#endif
#ifdef TM2_PORT_VISUAL
    portVisualBulletSpawn(slot);    /* request 46 item 4 (PLAYTEST_ANSWERS.md 11): drawing only */
#endif
}

/* @800D7E30: step all 40 projectiles: advance, test 4 sub-steps for hits (projectileHitTest / hdpQueryPoint), spawn impact effects and damage on a hit. */
void __attribute__((section(".text_800D7E30"))) bulletsMove(void)
{
    s32 i, j;
    TM2Shot60 *s;
    u8 *obj = 0;
    s32 cur[3];
    TM2Vec3 last;
#ifdef TM2_PORT_BALANCE
    s32 k;
#endif

    for (i = 0; i < 40; i++) {
        if (bulletRecords[i].life > 0) {
            s = &bulletRecords[i];
            s->life -= 1;
            s->prev[0] = s->pos[0];
            s->prev[1] = s->pos[1];
            s->prev[2] = s->pos[2];
            s->pos[0] += s->vel[0];
            s->pos[1] += s->vel[1];
            s->pos[2] += s->vel[2];
            cur[0] = s->prev[0];
            cur[1] = s->prev[1];
            cur[2] = s->prev[2];
            for (j = 3; j >= 0; j--) {
                { last.x = cur[0]; last.y = cur[1]; last.z = cur[2]; }
                cur[0] = s->pos[0] - ((s->vel[0] * j) >> 2);
                cur[1] = s->pos[1] - ((s->vel[1] * j) >> 2);
                cur[2] = s->pos[2] - ((s->vel[2] * j) >> 2);
                if (s->owner < 50)
                    s->hit = projectileHitTest(cur, s->owner, 10, -1);
                else
                    s->hit = projectileHitTest(cur, s->owner, 5, -1);
                if (s->hit == s->owner)
                    s->hit = 0;
                if (j == 2 && s->hit == 0) {
                    obj = hdpQueryPoint(s->owner, -1, cur, &s->hit);
                    if (S16(obj, 10) == -s->owner || S16(obj, 10) == 667 || S16(obj, 10) == 668
                        || S16(obj, 10) == 3301 || (S16(obj, 10) > 400 && S16(obj, 10) < 441)
                        || (S16(obj, 10) >= 3500 && S16(obj, 10) < 3510)) {
                        S16(obj, 10) = 0;
                        s->hit = 0;
                    }
                }
                if (s->hit > 0) {
                    if (s->hit == 1)
                        { last.x = cur[0]; last.y = cur[1]; last.z = cur[2]; }
                    fxSparkOrange(&last.x, 0);
                    if (rand() & 1)
                        fxSmokeA6(&last.x, 0);
                    if (s->hit != 8)
                        fxBulletImpact(&last.x, 0);
                    s->life = 0;
                    j = -1;
                    if (GUN_FLIP(s, k)) {
                        if (obj == 0)
                            weaponApplyHit(s->hit, 0, 0, GUN_POWER(s, k), s->vel, s->owner);
                        else
                            weaponApplyHit(s->hit, S16(obj, 10), S16(obj, 12), GUN_POWER(s, k), s->vel, s->owner);
                    }
                }
            }
        }
    }
}

/* 36-byte effect segment; its textured quad's UV/clut/tpage bytes sit at +20.. (POLY_FT4-style). */
typedef struct {
    u8 pad_00[20];
    u8 u0, v0;      /* +20 */
    u16 clut;       /* +22 */
    u8 u1, v1;      /* +24 */
    u16 tpage;      /* +26 */
    u8 u2, v2;      /* +28 */
    u16 pad_1E;
    u8 u3, v3;      /* +32 */
    u16 pad_22;
} TM2FxSeg;

/* Effect record (980 bytes, table fxRecords): start group +0x40, group count +0x44, segments at +0x14C. */
typedef struct {
    u8 pad_00[0x40];
    s8 start;       /* 0x40 */
    u8 pad_41[3];
    s8 count;       /* 0x44 */
    u8 pad_45[0x14C - 0x45];
    TM2FxSeg seg[18];   /* 0x14C */
} TM2FxRec;

/* Copy texture cell c's UVs/clut/tpage into segment n of effect record r; the tpage store indexes with `last` (n plus the caller's step side effect). */
#define FX_SETUV(r, n, c, last) do { \
    (r)->seg[n].u1 = fxContrailCellTable[c].u; \
    (r)->seg[n].v1 = fxContrailCellTable[c].v; \
    (r)->seg[n].u0 = fxContrailCellTable[c].u + fxContrailCellTable[c].w; \
    (r)->seg[n].v0 = fxContrailCellTable[c].v; \
    (r)->seg[n].u3 = fxContrailCellTable[c].u; \
    (r)->seg[n].v3 = fxContrailCellTable[c].v + fxContrailCellTable[c].h; \
    (r)->seg[n].u2 = fxContrailCellTable[c].u + fxContrailCellTable[c].w; \
    (r)->seg[n].v2 = fxContrailCellTable[c].v + fxContrailCellTable[c].h; \
    (r)->seg[n].clut = fxContrailCellTable[c].clut; \
    (r)->seg[last].tpage = fxContrailCellTable[c].tpage; \
} while (0)

/* @800D8CEC: effect record: write texture-cell UVs/clut/tpage into the 3 segments of each of its `count` (+0x44) groups, picking the cell by position (i*8/count, rounded) and walking groups backwards (wrapping) from `start` (+0x40). */
void __attribute__((section(".text_800D8CEC"))) missileTrailSetUVs(TM2FxRec *rec)
{
    s32 i;
    s32 grp = rec->start;
    s32 j;
    s32 c;

    for (i = 0; i < rec->count; i++) {
        c = (i * 8 + (rec->count >> 1)) / rec->count;
        FX_SETUV(rec, (j = grp * 3), c, j++);
        FX_SETUV(rec, j, c, j++);
        FX_SETUV(rec, j, c, (grp--, j));
        if (grp < 0)
            grp = rec->count - 1;
    }
}
