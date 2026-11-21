#include "semantic/cars.h"

extern void weaponHumanCycleInput(struct TM2HumanRecord *human);
extern void aiWeaponFireDecide(struct TM2AIRecord *ai);
extern void weaponFireSelected(void *car, int human);
extern void weaponFireEnergyAttack(void *car, int human);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int soundStartPlayerLoop(int, int, int, int, int);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundStartSharedLoop(int, int, int, int, int);
extern int soundGetGunLoopP1(void);
extern int soundGetGunLoopP2(void);
extern int soundStopPlayerLoop(int, int);
extern int soundStopSharedLoop(int, int);
extern unsigned short UpdateGuns(void *car, unsigned int human);
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-4 (CAR_GROUPS.md 16) */
#endif
#if defined(TM2_PORT_HUMANS) || defined(TM2_PORT_BALANCE)
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: the gun loop by sound side; request 41: TM2_HUMAN_DENIED */
#endif
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/car_groups.h"
#include "semantic/dt_player.h"   /* request 36 */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
extern void soundSetDeniedPending(unsigned char value);   /* request 41: TM2_HUMAN_DENIED without TM2_PORT_HUMANS */
#endif

/* car is a human record when the low byte of human is set, else an AI record;
 * the byte is re-tested at each use as in retail. */
#define MODE (human & 255)
#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

/* @8010388C. Per-actor weapon/controller tick. The low byte of `human` selects the actor
 * layout (as in UpdateGuns). Counters are 16-bit with signed-16/signed-8 comparisons taken from
 * the raw sll/sra sequences; the fire-request byte at ctl+0x1B is consumed by UpdateGuns.
 * Returns 0, or UpdateGuns' cooldown when a shot was requested (raw $v0 at the epilogue). */
void __attribute__((section(".text_8010388C")))
weaponTick(void *car, unsigned int human)
{
    struct TM2CarWeapons *weap;
    struct TM2CarState *flags;
    struct TM2CarMotion *motion;
    struct TM2CarInput *ctl;
    int i, mtap, extra;

    if (MODE) {
        weaponHumanCycleInput(car);
        ctl = &HUMAN->input; weap = &HUMAN->weapons; flags = &HUMAN->state; motion = &HUMAN->motion;
    } else {
        aiWeaponFireDecide(car);
        weap = &AI->weapons; ctl = &AI->input; flags = &AI->state; motion = &AI->motion;
    }
    if (weap->infinite != 0 || MODE == 0) {
        for (i = 0; i < 10; i++)
            weap->ammo[i] = 1;
        weap->ammo[8] = 0;
        weap->turbo = 990;
        weap->energy = 1000;
    }
#ifdef TM2_PORT_BALANCE
    /* request 40 (BALANCE.md 3.10): the period of the car's special, read here (its only reader) */
    if (weap->f2E++ >= portBalancePeriod(car, MODE, weap->recharge_period)) {
#else
    if (weap->f2E++ >= weap->recharge_period) {
#endif
        weap->f2E = 0;
        weap->ammo[0] = (unsigned short)(weap->ammo[0] + 1);
        if (weap->ammo[0] > weap->max_specials)
            weap->ammo[0] = (unsigned short)weap->max_specials;
        if (weap->selected >= 8 && weap->ammo[0] > 0)
            weap->selected = 0;
    }
    if (weap->energy++ >= 1001)
        weap->energy = 1000;
    weaponFireSelected(car, MODE);
    weaponFireEnergyAttack(car, MODE);
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth fires the boss's missiles with the machine-gun
       button; it takes the request, so no gun loop starts (a running one stops below) and UpdateGuns is not
       called */
    if (MODE && HUMAN->type == 0xF0)
        portCarDtPlayerGuns(car);
#endif
#ifdef TM2_PORT_BALANCE
    /* request 41 bit 15 (BALANCE.md 12.3): no machine guns while her tornado runs; the request is taken (the loop
       stops below, UpdateGuns is not called), the denied sound once per press */
    if ((i = portBalanceGunsRefused(car, MODE, ctl->fire_guns)) != 0) {
        if (MODE && i == 2)
            TM2_HUMAN_DENIED(HUMAN->player);
        ctl->fire_guns = 0;
    }
#endif
    if (ctl->fire_guns != 0 && flags->dead == 0 && flags->frozen == 0) {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(motion->pos);
        else
            soundLocateForView(motion->pos, -1);
        if (MODE) {
#if defined(TM2_PORT_CAR_SOUND) || defined(TM2_PORT_HUMANS)
            /* request 33-4: players 2.. per car; H3: by sound side, a remote human's in the shared loop */
            portHumanLoopPlay(9, 0, 0, 99, HUMAN->player, HUMAN->slot);
#else
            soundStartPlayerLoop(9, 0, 0, 99, HUMAN->player);
#endif
        } else if (viewGetSplitMode() == 0) {
            mtap = PadChkMtap();
            extra = soundGetListenerPan();
            soundStartSharedLoop(9, mtap, extra, 99, AI->index);
        } else {
            mtap = PadChkMtap();
            soundStartSharedLoop(9, mtap, 0, 99, AI->index);
        }
    } else if (MODE) {
#if defined(TM2_PORT_CAR_SOUND) || defined(TM2_PORT_HUMANS)
        portHumanLoopStop(9, HUMAN->player, HUMAN->slot);   /* request 33-4: retail's calls for players 0 and 1 */
#else
        if (HUMAN->player == 0 && (soundGetGunLoopP1() & 255))
            soundStopPlayerLoop(9, HUMAN->player);
        if (HUMAN->player == 1 && (soundGetGunLoopP2() & 255))
            soundStopPlayerLoop(9, HUMAN->player);
#endif
    } else {
        soundStopSharedLoop(9, AI->index);
    }
    /* Returns by falling off the end, as in retail: the cleared request
     * byte (0) or UpdateGuns' cooldown is left in v0. */
    if (ctl->fire_guns != 0) {
        ctl->fire_guns = 0;
        UpdateGuns(car, MODE);
    }
}
