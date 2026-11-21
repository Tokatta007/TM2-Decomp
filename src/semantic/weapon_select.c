#pragma tm2_rdata .tm2_800CF10C
#include "semantic/weapons.h"
#include "semantic/car_specials.h"   /* request 35: TM2_SPECIAL_TYPE */

extern struct TM2SceneObject *humanCarObject(int);
extern struct TM2SceneObject *aiCarObject(int);
extern int carPairDistance(int, int);
extern int carPairHeading(int, int);
extern void soundSetDeniedPending(u8 value);
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif
extern void weaponFireSpecial(void *car, int human);
extern void weaponLaunchProjectile(void *car, int kind, int human);
extern void weaponDropItem(void *car, int kind, int human);
extern int weaponRicochetSlotFind(void);
extern void *memcpy(void *, const void *, int);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlayVariantEx(int, int, int, int, int, int);
extern int soundPlay(int, int, int, int);
extern int gameGetLevel(void);
extern int moscowDishIsDestroyed(void);
extern int weaponLightningScan(void *car, int human);
extern int weaponFireInterval;

/* @80184860: ten pooled effect objects (allocated by weaponAssetBind for asset 2210) followed by ten
 * 48-byte shot records (include/semantic/weapons.h); storage in asset_pools.c. */
extern struct TM2RicochetTable weaponRicochetPool;
/* @80180F28 / @80180F2C: asset ids for the type-6 shot (vehicle 0xF0 versus others), stored by the
 * asset loader weaponAssetBind (cases 2214 and 2210). Zero, CRT-cleared words read through $gp; owned
 * provisionally by their first maintained reader. */
extern int weaponRicochetModelAlt;
extern int weaponRicochetModel;

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* @80103FB8. Per-actor weapon selection/firing step, called every tick by weaponTick after the
 * prologue. `human` selects the actor layout (as in the tick): car is a human record when set. AI actors (mode 0)
 * get an auto-aim pass that forces the basic weapon (kind 0) when a target of the same lane is within
 * 400 units and 227 angle units. A positive cooldown +0x26 just counts down. The fire request is the
 * ctl byte +0x1A with the confirm byte +0x1D; players with an empty slot (or kind 8) cycle to the
 * previous weapon instead of firing. Otherwise the cooldown is reloaded and the weapon kind is
 * dispatched: 0 basic, 1/2/4/5 projectile, 3 toggle, 6 pooled effect shot, 7 special, 8/other none;
 * then the requests are cleared and the slot count (+0xE+2*kind) and total (+0x22) decrement. The
 * caller ignores $v0 (M2C's u16 return is the decrement residue), so the function is void. */
void __attribute__((section(".text_80103FB8")))
weaponFireSelected(car, human)
void *car; unsigned char human;   /* old-style: the caller passes an int, the body narrows */
{
    struct TM2SceneObject *lookup;
    struct TM2CarWeapons *weap;
    struct TM2CarInput *ctl;
    struct TM2CarState *flags;
    struct TM2CarMotion *motion;
    int interval, vehicle, owner, kind, slot, dist, other, mtap, extra;
    unsigned char is_main;
    short vec[4];

    if (human) {
        lookup = humanCarObject(HUMAN(car)->player);
        vehicle = HUMAN(car)->type;
        motion = &HUMAN(car)->motion;
        ctl = &HUMAN(car)->input;
        owner = HUMAN(car)->slot;
        weap = &HUMAN(car)->weapons;
        interval = 20;
        flags = &HUMAN(car)->state;
    } else {
        lookup = aiCarObject(AI(car)->index);
        vehicle = AI(car)->type;
        interval = weaponFireInterval;
        motion = &AI(car)->motion;
        owner = AI(car)->slot;
        ctl = &AI(car)->input;
        weap = &AI(car)->weapons;
        flags = &AI(car)->state;
    }
    kind = weap->selected;
    if (human == 0 && (carHasMeleeSpecial(TM2_SPECIAL_TYPE(owner, vehicle)) & 255)) {   /* request 35: as-type */
        void *target = aiGetTargetCar(AI(car)->index, &is_main);
        other = is_main ? HUMAN(target)->slot : AI(target)->slot;
        if (carPairDistance(owner, other) < 400) {
            if (__builtin_abs(motion->rot[2] - (short)carPairHeading(owner, other)) < 227) {
                kind = 0;
                if (weap->fire_cooldown < interval - 10) {
                    weap->fire_cooldown = 0;
                    ctl->fire = 1;
                    ctl->fire_ready = 1;
                    if (weap->ammo[0] <= 0)
                        weap->ammo[0] = 1;
                }
            }
        }
    }
    if (weap->fire_cooldown > 0) {
        weap->fire_cooldown--;
        return;
    }
    if (!(ctl->fire != 0 && human))
        ctl->fire_ready = 1;
    if (human) {
        if (kind == 8 || weap->ammo[kind] <= 0) {
            weaponCycleSelection(weap, -1, 8);
            if (human && kind == 8 && ctl->fire_ready != 0 && ctl->fire != 0 && flags->frozen == 0) {
                TM2_HUMAN_DENIED(HUMAN(car)->player);
                ctl->fire_ready = 0;
            }
            return;
        }
    }
    if (flags->frozen != 0 || ctl->fire == 0 || ctl->fire_ready == 0) {
        ctl->fire = 0;
        return;
    }
#ifdef TM2_PORT_BALANCE
    /* request 41 bit 15 (BALANCE.md 12.3): while her tornado runs a Twister fires nothing (every kind, her special
       and a remote bomb's detonation too): denied, no cost (no ammo, no cooldown); one denied sound per press */
    if (portBalanceLocked(car, human)) {
        if (human) TM2_HUMAN_DENIED(HUMAN(car)->player);
        ctl->fire_ready = 0;
        ctl->fire = 0;
        return;
    }
#endif
    weap->fire_cooldown = interval;
    switch (kind) {
    case 0:
        weaponFireSpecial(car, human);
        break;
    case 1: case 2: case 4: case 5:
        weaponLaunchProjectile(car, kind, human);
        break;
    case 6:
        slot = weaponRicochetSlotFind();
        if (slot >= 0) {
            weaponRicochetPool.shots[slot].owner = lookup->id;
            if (vehicle == 0xf0)
                weaponRicochetPool.slots[slot]->model = weaponRicochetModelAlt;
            else
                weaponRicochetPool.slots[slot]->model = weaponRicochetModel;
            weaponRicochetPool.slots[slot]->visible = 1;
            weaponRicochetPool.slots[slot]->collides = 0;
            if (weap->rear_fire != 0) {
                weap->rear_fire = 0;
                vec[0] = motion->rot[0];
                vec[1] = 0;
                vec[2] = (motion->rot[2] + 0x800) & 0xfff;
            } else {
                vec[0] = motion->rot[0];
                vec[1] = 0;
                vec[2] = motion->rot[2];
            }
            memcpy(weaponRicochetPool.slots[slot]->rot, vec, 8);
            memcpy(weaponRicochetPool.slots[slot]->pos, motion->pos, 12);
            weaponRicochetPool.slots[slot]->pos[2] = weaponRicochetPool.slots[slot]->pos[2] + 40;
            weaponRicochetPool.shots[slot].obj = weaponRicochetPool.slots[slot];
            weaponRicochetPool.shots[slot].life = 400;
            weaponRicochetPool.shots[slot].bounces = 0;
#ifdef TM2_PORT_BALANCE
            weaponRicochetPool.shots[slot].left_owner = 0;   /* request 41 bit 18: not yet out of his box */
#endif
            weaponRicochetPool.shots[slot].offset[1] = 100;
            if (viewGetSplitMode() == 0)
                soundLocateForView0(motion->pos);
            else
                soundLocateForView(motion->pos, -1);
            mtap = PadChkMtap();
            extra = soundGetListenerPan();
            soundPlayVariantEx(0x12, mtap, extra, 99, slot, 1);
        }
        break;
    case 7:
        if (gameGetLevel() == 2 && moscowDishIsDestroyed() == 1 && human) {
            TM2_HUMAN_DENIED(HUMAN(car)->player);
            return;
        }
        if (gameGetLevel() == 1 || gameGetLevel() == 2 || gameGetLevel() == 8) {
            if (weaponLightningScan(car, human) != 0) {
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(motion->pos);
                else
                    soundLocateForView(motion->pos, -1);
                extra = soundGetListenerPan();
                soundPlay(0x40, 0, extra, 99);
            }
        } else if (human) {
            TM2_HUMAN_DENIED(HUMAN(car)->player);
            return;
        }
        break;
    case 3:
#ifdef TM2_PORT_BALANCE
        /* request 40 bit 6 (BALANCE.md 3.7): no remote bomb placed while her tornado runs (refused as the retail
           refusals above: no cost; detonating one placed before stays allowed) */
        if (weap->drop_armed == 0 && portBalanceBombRefused(car, human)) {
            if (human) TM2_HUMAN_DENIED(HUMAN(car)->player);
            return;
        }
#endif
        if (weap->drop_armed == 0) {
            weap->drop_armed = 1;
            weap->fire_cooldown = 1;
            weap->ammo[kind]++;
            weap->held++;
            if (viewGetSplitMode() == 0)
                soundLocateForView0(motion->pos);
            else
                soundLocateForView(motion->pos, -1);
            mtap = PadChkMtap();
            extra = soundGetListenerPan();
            soundPlay(0x13, mtap, extra, 99);
        } else {
            weap->drop_armed = 0;
        }
        weaponDropItem(car, weap->selected, human);
        break;
    case 8:
    default:
        break;
    }
    ctl->fire_ready = 0;
    ctl->fire = 0;
    weap->ammo[kind]--;
    if (kind != 0)
        weap->held--;
}
