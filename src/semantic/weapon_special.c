#pragma tm2_rdata .tm2_800CF134
#include "semantic/cars.h"

extern struct TM2Object *humanCarObject(int);
extern struct TM2Object *aiCarObject(int);
extern void weaponLaunchProjectile(void *car, int kind, int human);
extern void weaponDropItem(void *car, int kind, int human);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlay(int, int, int, int);
extern void soundSetDeniedPending(u8 value);
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
/* request 41 bit 19: the energy jump's cost (a macro: no #ifdef'd brace in the function for the source checks) */
#define JUMP_COST(car, human) (portBalanceJumpFree(car, human) ? 0 : TM2_BAL_JUMP_COST)
#else
#define JUMP_COST(car, human) 120
#endif
extern int weaponShieldSlotFind(void);
extern int roundIsOver(void);
extern void mathMulTransVec(void *, int *, int *);
extern int rsin(int);
extern int rcos(int);
extern void weaponFindTargetAhead(int, int *, s32 mode);
extern s16 weaponFireColoredMissile(int, int, int *, int *, int, void *);
extern int weaponFireFreezeMissile(int, int, int *, int *, int);
extern int weaponFireInterval;

/* @80184A68: four pooled special-effect objects (allocated by weaponAssetBind for asset 2217) followed
 * by four 12-byte records; the pool scan weaponShieldSlotFind and the asset loader remain generated units. */
struct TM2SpecialRecord {
    short life, owner;
    void *lookup;
    void *obj;
};
struct TM2SpecialTable {
    struct TM2Object *slots[TM2_MAX_SHIELDS];
    struct TM2SpecialRecord records[TM2_MAX_SHIELDS];
};
typedef char tm2_special_record_size[(sizeof(struct TM2SpecialRecord) == 12) ? 1 : -1];
extern struct TM2SpecialTable weaponShieldPool;
/* @80180AF8 / @80180B00 / @80180B08: three 8-byte colour constants passed to weaponFireColoredMissile by this
 * function only (retail data, referenced by address). */
extern unsigned char weaponEnergyColorRed[];     /* {0xff, 0, 0, 0, 0, 0, 0, 0} */
extern unsigned char weaponEnergyColorCyan[];     /* {0, 0, 0xff, 0, 0xff, 0, 0, 0} */
extern unsigned char weaponEnergyColorGreen[];     /* {0, 0, 0xff, 0, 0, 0, 0, 0} */

#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

/* @801046FC. Per-actor special-weapon step, called every tick by weaponTick after weaponFireSelected.
 * `human` selects the actor layout (as in the tick): car is a human record when set. The special kind is the
 * signed-8 byte +0xC of the weapon block (100..106; 0x6B never fires); its energy is the halfword
 * +0x30 and each kind has a cost (400, 120 or 900) checked signed-16. The request byte is ctl+0x1C.
 * Nothing happens while the cooldown +0x26 is positive; a blocked flag (+0xD) or a missing request
 * clears the request and returns; zero energy returns. After a dispatch the request is cleared and
 * the cooldown reloads with the interval (halved for the player). The caller ignores $v0, so the
 * function is void (as M2C renders it).
 * Both arms set geom, motion and ctl before the lookup call (that order lets sched put the volatile
 * lookup-argument lhu ahead of ctl's stack spill store, as retail does). */
void __attribute__((section(".text_801046FC")))
weaponFireEnergyAttack(car, human)
void *car; unsigned char human;   /* old-style: the caller passes an int, the body narrows */
{
    int ang[3], out[3], in[3], dir[3];
    int effect;
    void *obj;
    struct TM2Object *lookup;
    struct TM2CarWeapons *weap;
    struct TM2CarInput *ctl;
    struct TM2CarState *flags;
    struct TM2CarMotion *motion;
    struct TM2CarStatsBlock *geom;
    int interval, owner, kind, slot, mtap, extra;
    short id;

    if (human) {
        geom = &HUMAN->stats; motion = &HUMAN->motion; ctl = &HUMAN->input;
        lookup = humanCarObject(HUMAN->player);
        weap = &HUMAN->weapons; flags = &HUMAN->state; interval = 20;
        id = TM2_HUMAN_ID(HUMAN->player); owner = HUMAN->player;
    } else {
        geom = &AI->stats; motion = &AI->motion; ctl = &AI->input;
        lookup = aiCarObject(AI->index);
        weap = &AI->weapons; flags = &AI->state; interval = weaponFireInterval;
        id = AI->index + 50; owner = AI->index;
    }
    kind = (signed char)weap->f0C;
    if (weap->fire_cooldown > 0 || kind == 0x6b)
        return;
    if (flags->frozen != 0 || ctl->fire_special == 0) {
        ctl->fire_special = 0;
        return;
    }
#ifdef TM2_PORT_BALANCE
    /* request 41 bit 15 (BALANCE.md 12.3): no energy attack while her tornado runs: denied, no cost; request 46
       item 3: but the shield (BALANCE.md 15) */
    if (portBalanceEnergyLocked(car, human, kind)) {
        if (human) TM2_HUMAN_DENIED(HUMAN->player);
        ctl->fire_special = 0;
        return;
    }
    /* bit 19 (12.8): Grasshopper's free jump also at 0 energy */
    if (weap->energy == 0 && !(kind == 104 && portBalanceJumpFree(car, human)))
        return;
#else
    if (weap->energy == 0)
        return;
#endif
    switch (kind) {
    case 100: case 102:
        if (weap->energy - 400 < 0) {
            if (human) TM2_HUMAN_DENIED(HUMAN->player);
        } else {
            weaponLaunchProjectile(car, kind, human);
            weap->energy -= 400;
        }
        break;
    case 101:
        if (human == 0 && AI->f001 != 2)
            break;
#ifdef TM2_PORT_BALANCE
        /* request 40 (BALANCE.md 3.3, 3.7): bit 2 no room for the mine, bit 6 her tornado runs: refused, no cost */
        if (portBalanceMineRefused(car, human)) {
            if (human) TM2_HUMAN_DENIED(HUMAN->player);
            break;
        }
#endif
        if (weap->energy - 120 >= 0) {
            weaponDropItem(car, 0x65, human);
            weap->energy -= 120;
            if (viewGetSplitMode() == 0)
                soundLocateForView0(motion->pos);
            else
                soundLocateForView(motion->pos, -1);
            mtap = PadChkMtap();
            extra = soundGetListenerPan();
            soundPlay(0x14, mtap, extra, 99);
        } else if (human) {
            TM2_HUMAN_DENIED(HUMAN->player);
        }
        break;
    case 103:
        if (weap->energy - 400 < 0) {
            if (human) TM2_HUMAN_DENIED(HUMAN->player);
        } else {
#ifdef TM2_PORT_BALANCE
            /* request 40 bit 1 (BALANCE.md 3.2): already shielded (or the pool is full): refused, no cost */
            if (portBalanceShieldRefused(flags->energy_shield)) {
                if (human) TM2_HUMAN_DENIED(HUMAN->player);
                break;
            }
#endif
            if (flags->energy_shield == 0) {
                slot = weaponShieldSlotFind();
                if (slot >= 0) {
                    flags->energy_shield = 1;
                    weaponShieldPool.slots[slot]->visible = 1;
                    if (human)
                        weaponShieldPool.records[slot].owner = HUMAN->player;
                    weaponShieldPool.records[slot].lookup = lookup;
                    obj = weaponShieldPool.slots[slot];
                    weaponShieldPool.records[slot].life = 60;
                    weaponShieldPool.records[slot].obj = obj;
                }
            }
            weap->energy -= 400;
        }
        break;
    case 104:
        /* request 41 bit 19 (BALANCE.md 12.8): Grasshopper's jump costs no energy (the cooldown below is kept) */
        if (weap->energy - JUMP_COST(car, human) < 0) {
            if (human) TM2_HUMAN_DENIED(HUMAN->player);
        } else {
            carSpinOut(car, human, 7, 1);
#ifdef TM2_PORT_BALANCE
            if (!portBalanceJumpFree(car, human))
#endif
            weap->energy -= 120;
            if (viewGetSplitMode() == 0)
                soundLocateForView0(motion->pos);
            else
                soundLocateForView(motion->pos, -1);
            mtap = PadChkMtap();
            extra = soundGetListenerPan();
            soundPlay(0x10, mtap, extra, 99);
        }
        break;
    case 105:
        if (roundIsOver() != 0 || weap->invisible != 0)
            break;
        if (weap->energy - 400 < 0) {
            if (human) TM2_HUMAN_DENIED(HUMAN->player);
        } else {
            weap->invisible = 1;
            weap->invis_timer = 120;
            weap->energy -= 400;
        }
        break;
    case 106:
        if (weap->energy - 900 >= 0) {
            weap->energy -= 900;
            in[0] = 0; in[1] = geom->gun[1]; in[2] = geom->gun[2];
            mathMulTransVec(&motion->rot_m, in, out);
            out[0] += motion->pos[0];
            out[1] += motion->pos[1];
            out[2] += 24 + motion->pos[2];
            in[0] = 0; in[1] = -100; in[2] = 0;
            mathMulTransVec(&motion->rot_m, in, dir);
            dir[0] = rsin(motion->rot[2]) >> 6;
            dir[1] = rcos(motion->rot[2]) >> 6;
            dir[2] = -rsin(motion->rot[0]) >> 6;
            weaponFindTargetAhead(owner, &effect, human);
            ang[0] = motion->rot[0]; ang[1] = motion->rot[1]; ang[2] = motion->rot[2];
            in[0] = geom->mount_x; in[1] = 0; in[2] = geom->launch_height + 8;
            mathMulTransVec(&motion->rot_m, in, out);
            out[0] += motion->pos[0]; out[1] += motion->pos[1]; out[2] += motion->pos[2];
            weaponFireColoredMissile(id, effect, ang, out, 10, weaponEnergyColorCyan);
            in[0] = geom->gun[0]; in[1] = geom->gun[1]; in[2] = geom->gun[2];
            mathMulTransVec(&motion->rot_m, in, out);
            out[0] += motion->pos[0]; out[1] += motion->pos[1]; out[2] += motion->pos[2];
            if ((short)weaponFireColoredMissile(id, effect, ang, out, 10, weaponEnergyColorGreen) >= 0)
                in[0] = -geom->gun[0];
            in[1] = geom->gun[1]; in[2] = geom->gun[2];
            mathMulTransVec(&motion->rot_m, in, out);
            out[0] += motion->pos[0]; out[1] += motion->pos[1]; out[2] += motion->pos[2];
            if ((short)weaponFireColoredMissile(id, effect, ang, out, 10, weaponEnergyColorRed) >= 0)
                out[2] += 200;
            weaponFireFreezeMissile(lookup->id, effect, ang, out, 7);
            /* H3: full volume for a local human; a remote human's as a computer car's (positional) */
            if (TM2_CAR_FEEDBACK(human, HUMAN->player)) {
                soundPlay(13, 0, 0, 99);
            } else if (TM2_CAR_POSITIONAL(human, AI->active != 0)) {
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(out);
                else
                    soundLocateForView(out, -1);
                mtap = PadChkMtap();
                extra = soundGetListenerPan();
                soundPlay(13, mtap, extra, 99);
            }
        } else if (human) {
            TM2_HUMAN_DENIED(HUMAN->player);
        }
        break;
    case 107:
    default:
        break;
    }
    ctl->fire_special = 0;
    if (human)
        weap->fire_cooldown = interval >> 1;
    else
        weap->fire_cooldown = interval;
}
