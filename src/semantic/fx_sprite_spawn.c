/* Small game helpers (8010..8011 range). Names stay address-based. */
#include "semantic/cars.h"

extern void *memcpy(void *, const void *, u32);
extern s32 rand(void);
extern void *RotMatrixYXZ(void *, void *);
#include "semantic/world.h"
extern void weaponDropCheckHit(s32);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif
extern s32 groundFireFindFree(void);
extern u8 *mathJitterPos(s32 *, s32, s32, s32);
extern s32 fxGroundFlame(u8 *, u8);
extern void fxSmokeA48(u8 *, int arg1);
extern struct TM2FxSprite *fxSpriteFindFree(void);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(s32 *);
extern void soundLocateForView(s32 *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);

/* 28-byte timed-blend slots @80184B04 */
typedef struct {
    s8 kind;      /* +0 */
    u8 pad1[7];
    s16 timer;    /* +8 */
    s16 target;   /* +10 */
    u8 pad2[8];
    s32 value;    /* +20 */
    struct TM2Object *obj;  /* +24 */
} Blend28;
extern Blend28 weaponDrops[];

/* 16-byte random-pick entries @80183F7C */
typedef struct {
    s16 id;
    s16 flag;
    u8 pad[12];
} Pick16;
extern Pick16 pickupSlots[];
extern s32 pickupRespawnTimer; /* gp+364 frame counter */
extern s32 pickupRespawnPeriod; /* gp+276 frame period */
extern s32 pickupRegisteredCount; /* gp+280 entry count */

/* 40-byte burst records @801842C4 */
typedef struct {
    u8 active;   /* +0 */
    u8 pad1[3];
    s16 age;     /* +4 */
    u8 pad2[2];
    s32 pos[3];  /* +8 */
    s32 fx[5];   /* +20 */
} Burst40;
extern Burst40 groundFires[];

/* 12-byte follow records @80184A78 */
typedef struct {
    s16 timer;   /* +0 */
    s16 id;      /* +2 */
    struct TM2Object *src;  /* +4 */
    struct TM2Object *dst;  /* +8 */
} Follow12;
#if TM2_MAX_SHIELDS > TM2_RETAIL_SHIELDS
/* TM2_PORT_CARS: the records follow the TM2_MAX_SHIELDS object pointers of weaponShieldPool */
extern unsigned char weaponShieldPool[];
#define weaponShieldRecords ((Follow12 *)(weaponShieldPool + 4 * TM2_MAX_SHIELDS))
#else
extern Follow12 weaponShieldRecords[];
#endif

extern s32 fxExplosionCooldown; /* gp+392 pad re-init counter */
extern u8 fxSmokeFramesC[];
extern u8 fxExplosionFrames[];  /* 16 8-byte UV frames */
/* 48-byte effect sprite (fxSprites @80185144; car_restart.c Fx48). */
struct TM2FxSprite {
    u8 f00;         /* +0 */
    u8 flag;        /* +1 */
    u8 f02;         /* +2 */
    s8 f03;         /* +3 */
    u8 f04;         /* +4 */
    s8 f05;         /* +5 */
    u8 count;       /* +6 */
    u8 pad_07;
    u8 kind;        /* +8 */
    u8 step;        /* +9 */
    u8 frame;       /* +10 */
    u8 frames;      /* +11 step * c */
    s16 loops;      /* +12 */
    u8 pad_0E[2];
    u8 *table;      /* +16 frame table */
    s32 pos[3];     /* +20 */
    s32 vel[3];     /* +32 */
    s32 *owner_pos; /* +44 */
};
typedef char tm2_fx_sprite_size[(sizeof(struct TM2FxSprite) == 48) ? 1 : -1];

/* 36-byte bouncing impact particles @80184E74 (car_damage_human.c Particle36). */
struct TM2ImpactParticle {
    s32 pos[3];     /* +0 */
    s32 vel[3];     /* +12 */
    u8 frame;       /* +24 0..3 */
    u8 pad_19[3];
    s32 life;       /* +28 */
    void *anim;     /* +32 */
};
typedef char tm2_impact_particle_size[(sizeof(struct TM2ImpactParticle) == 36) ? 1 : -1];
extern struct TM2ImpactParticle fxImpactParticles[];

/* @80109D58: tick 25 blend slots: while timer>0 ease value toward target (90%) into obj+176,
 * decrement timer and refresh; at timer==0 release an obj in state 1 unless kind is 19. */
void __attribute__((section(".text_80109D58"))) weaponDropsUpdate(void *ctx, s32 view) /* both unused; the retail caller passes them */
{
    s32 i;

    for (i = 0; i < TM2_MAX_DROPS; i++) {   /* 25 (request 40: TM2_MAX_DROPS) */
        s16 t = weaponDrops[i].timer;

        if (t > 0) {
            if (weaponDrops[i].target < weaponDrops[i].value) {
                s32 v = mathBlendValue(weaponDrops[i].value, weaponDrops[i].target, 90);
                weaponDrops[i].value = v;
                weaponDrops[i].obj->pos[2] = v;
            }
#ifdef TM2_PORT_BALANCE
            if (!portBalanceMineKeep(weaponDrops[i].kind, t))   /* request 40 bit 2: a mine stays at 1 */
#endif
            weaponDrops[i].timer--;
            weaponDropCheckHit(i);
        } else if (weaponDrops[i].obj != 0 && t == 0 &&
                   weaponDrops[i].obj->visible == 1 && weaponDrops[i].kind != 19) {
            weaponDrops[i].obj->visible = 0;
            weaponDrops[i].obj = 0;
        }
    }
}

/* @8010AED0: every (period+1) frames pick a random entry; ids 417/418 only half the time;
 * mark it and trigger it via pickupSetVisible(id, index, 1).
 * (Identical: needs the duplicated mark+trigger in both branches; the `|| rand()` form swaps two insns.) */
void __attribute__((section(".text_8010AED0"))) pickupRespawnTick(void)
{
    s32 n;

    if (pickupRespawnTimer < pickupRespawnPeriod) {
        pickupRespawnTimer++;
        return;
    }
    pickupRespawnTimer = 0;
    if (pickupRegisteredCount > 0) {
#ifdef TM2_PORT_BALANCE
        n = portBalancePickupSlot(rand());    /* request 67 (bit 23): the same one draw, among the pickups to bring back */
        if (n < 0)
            return;
#else
        n = rand() % pickupRegisteredCount;
#endif
        if ((u16)(pickupSlots[n].id - 417) < 2) {
            if (rand() < 16384) {
                pickupSlots[n].flag = 1;
                pickupSetVisible(pickupSlots[n].id, n, 1);
            }
        } else {
            pickupSlots[n].flag = 1;
            pickupSetVisible(pickupSlots[n].id, n, 1);
        }
    }
}

/* @8010C020: allocate a burst record at pos and spawn 5 effect objects for it. */
void __attribute__((section(".text_8010C020"))) groundFireSpawn(s32 *pos)
{
    s32 i = groundFireFindFree();
    s32 j;
    s32 *fx;

    if (i < 30) {
        groundFires[i].active = 1;
        groundFires[i].age = 0;
        groundFires[i].pos[0] = pos[0];
        groundFires[i].pos[1] = pos[1];
        groundFires[i].pos[2] = pos[2];
        for (j = 0, fx = groundFires[i].fx; j < 5; j++, fx++) {
            u8 *o = mathJitterPos(pos, 0xcc, 0xcc, 0x555);
            *fx = fxGroundFlame(o, 0);
            fxSmokeA48(o, 0);
        }
    }
}

/* @8010DD30: 4 follow slots: count timer down (at 0 release dst and clear its owner +170 flag),
 * copy src pose (+168..+179 minus 64 on +176, angles +160) into dst and rebuild its matrix. */
void __attribute__((section(".text_8010DD30"))) weaponShieldsUpdate(void)
{
    s32 i;

    for (i = 0; i < TM2_MAX_SHIELDS; i++) {
        if (weaponShieldRecords[i].timer > 0) {
            weaponShieldRecords[i].timer--;
            if (weaponShieldRecords[i].timer == 0) {
                weaponShieldRecords[i].dst->visible = 0;
                humanCarGet(weaponShieldRecords[i].id)->state.energy_shield = 0;

            }
            memcpy(weaponShieldRecords[i].dst->pos, weaponShieldRecords[i].src->pos, 12);
            weaponShieldRecords[i].dst->pos[2] -= 64;
            memcpy(weaponShieldRecords[i].dst->rot, weaponShieldRecords[i].src->rot, 8);
            RotMatrixYXZ(weaponShieldRecords[i].dst->rot, weaponShieldRecords[i].dst->matrix);
        }
    }
}

/* @80110918: spawn a 48-byte effect object (free slot from fxSpriteFindFree) at pos:
 * kind +8, b +9, b*c +11, table +16, count +6, flag +1; position copied to +20..+28
 * (+120 on z when kind >= 96); owner pos ptr +44. Returns the object or 0. */
struct TM2FxSprite *__attribute__((section(".text_80110918"))) fxSpriteSpawn(s32 *pos, s32 kind, s32 b, s32 c,
                                                             u8 *table, u16 count, u8 flag)
{
    struct TM2FxSprite *o = fxSpriteFindFree();

    if (o == 0)
        return 0;
    o->step = b;
    o->frames = c * b;
    o->table = table;
    o->frame = 0;
    o->kind = kind;
    o->count = count;
    o->loops = 0;
    o->f00 = 0;
    o->f02 = 0;
    o->f03 = 0;
    o->f04 = 0;
    o->f05 = 0;
    o->vel[0] = 0;
    o->vel[1] = 0;
    o->vel[2] = 0;
    if (kind < 96) {
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
    } else {
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2] + 120;
    }
    o->owner_pos = pos;
    o->flag = flag;
    return o;
}

/* request 102 item 3 (portBalance bit 30, BALANCE.md 23): an explosion far from the one that last sounded sounds
   too, inside the 6-step cooldown */
#ifdef TM2_PORT_BALANCE
#define BOOM_READY(pos) portBalanceBoomReady((int *)(pos), fxExplosionCooldown >= 6)
#else
#define BOOM_READY(pos) (fxExplosionCooldown >= 6)
#endif

/* Every 6th call re-initialise pad comms (multitap probe). */
#define PAD_REINIT(pos)                                             \
    if (BOOM_READY(pos)) {                                                   \
        if (viewGetSplitMode() == 0) {                                  \
            s32 m;                                                  \
            soundLocateForView0(pos);                                       \
            m = PadChkMtap();                                       \
            soundPlay(8, m, soundGetListenerPan(), 75);                 \
        } else {                                                    \
            soundLocateForView(pos, -1);                                    \
            soundPlay(8, PadChkMtap(), 0, 75);                   \
        }                                                           \
        fxExplosionCooldown = 0;                                             \
    }

/* @80111B20: (after periodic pad re-init) spawn effect kind 16 from table 80187064, mode 6. */
struct TM2FxSprite *__attribute__((section(".text_80111B20"))) fxSmokeBurstPink(s32 *pos, u8 flag)
{
    struct TM2FxSprite *o;

    PAD_REINIT(pos)
    o = fxSpriteSpawn(pos, 16, 2, 16, fxSmokeFramesC, 1, flag);
    if (o) {
        o->vel[2] = 6;
        o->f02 = 1;
        o->f03 = -1;
        o->f04 = 0;
        o->f05 = -106;
    }
    return o;
}

/* @80111C24: (after periodic pad re-init) spawn effect kind 6 from table 80187064, mode 0. */
struct TM2FxSprite *__attribute__((section(".text_80111C24"))) fxSmokeBurstRed(s32 *pos, u8 flag)
{
    struct TM2FxSprite *o;

    PAD_REINIT(pos)
    o = fxSpriteSpawn(pos, 6, 1, 16, fxSmokeFramesC, 1, flag);
    if (o) {
        o->vel[2] = 0;
        o->f02 = 1;
        o->f03 = 50;
        o->f04 = 0;
        o->f05 = 0;
    }
    return o;
}

#define RND_DEBRIS_Z() (((rand() & 15) - 5) > 0 ? 0 : (rand() & 15) - 5)

/* @80111FB0: spawn a kind-9 debris particle (random one of 4 sprites at 80186DEC)
 * with random velocity. */
void __attribute__((section(".text_80111FB0"))) fxFlameStreamParticle(s32 *pos)
{
    struct TM2FxSprite *o = fxSpriteSpawn(pos, 9, 1, 2, fxExplosionFrames + 8 + (rand() & 3) * 8, 0, 0);  /* frames 1..4 */

    if (o) {
        o->vel[0] = (rand() & 7) - 3;
        o->vel[1] = (rand() & 7) - 3;
        o->vel[2] = RND_DEBRIS_Z();
        o->f02 = 1;
        o->f03 = -58;
        o->f04 = 66;
        o->f05 = 33;
    }
}

/* @80112E58: integrate 20 bouncing 36-byte particles @80184E74: pos += vel, gravity 4,
 * bounce at z<=0 (halve and reverse, die when slow), count life down and cycle frame 0..3. */
void __attribute__((section(".text_80112E58"))) fxImpactParticlesUpdate(void)
{
    s32 i;
    struct TM2ImpactParticle *p;

    for (i = 0, p = fxImpactParticles; i < 20; i++, p++) {
        if (p->life > 0) {
            p->pos[0] += p->vel[0];
            p->pos[1] += p->vel[1];
            p->pos[2] += p->vel[2];
            p->vel[2] -= 4;
            if (p->pos[2] <= 0 && p->vel[2] < 0) {
                if (p->vel[2] >= -5)
                    p->life = 0;
                else
                    p->vel[2] = -p->vel[2] / 2;
            }
            if (p->life > 0) {
                p->life--;
                p->frame = (p->frame + 1) & 3;
            }
        }
    }
}
