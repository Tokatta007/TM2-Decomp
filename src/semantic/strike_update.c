struct TM2StrikeState { unsigned char mode; char f1; unsigned short owner_id; short active; short timer; int velocity[3], delta[3], previous[3]; };
/* @80184E10..80184E74: retail addresses the strike state, its four corner
 * offsets (@80184E3C), the owner weapon block (@80184E6C) and the strike
 * object (@80184E70) as one object (shared base registers). */
struct TM2StrikeBlock { struct TM2StrikeState s; int corners[12]; unsigned char *weapon; unsigned char *object; };
extern struct TM2StrikeBlock weaponStrikeState;
/* @80184E6C: owner weapon block, set by weapon_basic.c; retail CRT zero. */
unsigned char *weaponStrikeOwner __attribute__((section(".tm2_80184E6C"))) = 0;
extern void *RotMatrixYXZ(short *, void *);
extern void mathMulTransVec(void *, int *, int *);
extern void *memcpy(void *,const void *,unsigned int);
extern int worldGroundHeightAt(int *);
extern int ratan2(int,int);
extern int projectileHitTest(int *,int,int,int);
extern void objRegistrySetStep(int,int,int);
extern void fxExplosion48(int *, unsigned char flag);
extern void fxExplosion96(int *, unsigned char flag);
#include "semantic/weapon_calls.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"

#define H(p,n) (*(unsigned short *)((unsigned char *)(p)+(n)))
#define W(p,n) (*(unsigned int *)((unsigned char *)(p)+(n)))
typedef char strike_size[(sizeof(struct TM2StrikeState)==44)?1:-1];

/* @8010F9C0. Move the strike object: four transformed corner vectors precede
 * all four height queries. Void; $v0 is left holding the strike timer. */
#define B weaponStrikeState
#ifdef TM2_PORT_CAR_SPECIALS
/* request 33-2a (CAR_GROUPS.md 10): retail's body, run by weaponShadowStrikeUpdate below once per Shadow */
static void __attribute__((section(".port_text"))) weaponShadowStrikeUpdateOne(void)
#else
void __attribute__((section(".text_8010F9C0"))) weaponShadowStrikeUpdate(void)
#endif
{
    short rotation[3];
    int heights[4], corners[4][3];
    int i, hit, angle;
    if (B.s.timer <= 0)
        return;
    B.s.timer--;
    rotation[0] = H(B.object,0xa0);
    rotation[1] = H(B.object,0xa2);
    rotation[2] = H(B.object,0xa4);
    RotMatrixYXZ(rotation, B.object + 0x80);
    mathMulTransVec(B.object + 0x80, B.s.velocity, B.s.delta);
    memcpy(B.s.previous, B.object + 0xa8, 12);
    W(B.object,0xa8) += (unsigned int)B.s.delta[0];
    W(B.object,0xac) += (unsigned int)B.s.delta[1];
    W(B.object,0xb0) += (unsigned int)B.s.delta[2];
    for (i = 0; i < 4; i++) {
        mathMulTransVec(B.object + 0x80, B.corners + i * 3, corners[i]);
        corners[i][0] = (int)((unsigned int)corners[i][0] + W(B.object,0xa8));
        corners[i][1] = (int)((unsigned int)corners[i][1] + W(B.object,0xac));
        corners[i][2] = (int)((unsigned int)corners[i][2] + W(B.object,0xb0));
    }
    for (i = 0; i < 4; i++)
        heights[i] = worldGroundHeightAt(corners[i]);
    angle = ratan2((int)((unsigned int)heights[0] - (unsigned int)heights[1]), 80);
    H(B.object,0xa0) = (unsigned short)(0u - (unsigned int)angle);
    angle = ratan2((int)((unsigned int)heights[3] - (unsigned int)heights[2]), 80);
    H(B.object,0xa2) = (unsigned short)angle;
    rotation[0] = H(B.object,0xa0);
    rotation[1] = H(B.object,0xa2);
    rotation[2] = H(B.object,0xa4);
    RotMatrixYXZ(rotation, B.object + 0x80);
    hit = projectileHitTest((int *)(B.object + 0xa8), (short)B.s.owner_id, 20, 0x915);
    if (hit >= 501 || (hit > 0 && B.s.mode == 0)) {
        B.s.timer = 0;
        B.s.active = 0;
        objRegistrySetStep(0x915, 0, 0);
        fxExplosion48((int *)(B.object + 0xa8), 0);
        weaponApplyHit(hit, 0, 0, TM2_SPECIAL_NUM_ID((short)B.s.owner_id, TM2_SPN_DAMAGE, 25), B.s.delta,
                       (short)B.s.owner_id);
    }
    if (B.s.timer == 0) {
        B.s.active = 0;
        W(B.object,8) = 0;
        objRegistrySetStep(0x915, 0, 0);
        W(B.object,0xb0) += 80u;
        fxExplosion96((int *)(B.object + 0xa8), 0);
        H(B.weapon,14)--;
    }
}

#ifdef TM2_PORT_CAR_SPECIALS
/* @8010F9C0 under TM2_PORT_CAR_SPECIALS (PER_CAR_PLAN.md D6): the first Shadow's strike, then each
 * duplicate Shadow's in key order; with TM2_PORT_SPECIAL_AS (request 35) then each borrowed strike. */
void __attribute__((section(".text_8010F9C0"))) weaponShadowStrikeUpdate(void)
{
    int d = 0, t;

    weaponShadowStrikeUpdateOne();
    while ((t = portCarSpecialNext(0xD0, &d)) != 0) {
        weaponShadowStrikeUpdateOne();
        portCarSpecialRestore(t);
    }
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35 (GUN_GAME_SPECIALS.md "As built"): then each car slot's borrowed strike, in slot order */
    for (d = 0; (t = portCarSpecialAsNext(0xD0, &d)) != 0;) {
        weaponShadowStrikeUpdateOne();
        portCarSpecialRestore(t);
    }
#endif
}
#endif
