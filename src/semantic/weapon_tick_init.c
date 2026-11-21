#include "semantic/cars.h"

extern int rand(void);
extern int gameGetLevel(void);
extern int aiDarkToothBossUpdate(void *);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#include "semantic/car_specials.h"   /* request 35: TM2_SPECIAL_TYPE */
/* @80180F44: AI fire-interval reload value (retail: zero-initialized, CRT-cleared word read through
 * $gp). Written by weaponsRoundReset, read here and by weaponFireSelected / weaponFireEnergyAttack. Owned provisionally by
 * its first maintained reader; the writer and other readers remain generated units. */
int weaponFireInterval __attribute__((section(".tm2_80180F44"))) = 0;

/* @80103C08. Player (mode-1) weapon-tick prologue: while the select-latch byte +0xE9 is set, a
 * left/right request (+0xE7 / +0xE8) steps the weapon block +0x7C8 by +1 / -1 over 8 slots and
 * clears the latch; with both requests released the latch is re-armed. The single caller
 * (weaponTick) ignores $v0, and M2C renders the function void. */
void __attribute__((section(".text_80103C08")))
weaponHumanCycleInput(struct TM2HumanRecord *human)
{
    if (human->input.cycle_armed != 0) {
        if (human->input.cycle_next != 0) {
            weaponCycleSelection(&human->weapons, 1, 8);
            human->input.cycle_armed = 0;
        } else if (human->input.cycle_prev != 0) {
            weaponCycleSelection(&human->weapons, -1, 8);
            human->input.cycle_armed = 0;
        }
    } else if (human->input.cycle_next == 0 && human->input.cycle_prev == 0) {
        human->input.cycle_armed = 1;
    }
}

/* @80103CA0. AI (mode-0) weapon-tick prologue: when the actor is active (+0x34), the hazard level
 * n = 4..1 from the +0x38..+0x3A bytes narrows a firing cone around the signed-16 aim angle +0x20.
 * Inside |angle| < 136 with range +0x28 below 4000 and +0x75 clear, the fire request +0x5F is set.
 * Inside |angle| < 113*n a rand()/328 roll against the aggression byte +0x812 picks the forward
 * (+0x5E) or special (+0x60) request from the signed-8 ammo count +0x815, toggling +0x815 for
 * vehicle 0x30, or clears +0x5E and reloads the interval +0x82E from weaponFireInterval when it is <= 0.
 * With +0xD4 == 1, a wider cone ((180 - 10*n) << 12) / 360 < |angle| repeats the roll with +0x80D
 * set. Finally, in game mode 8 the vehicle 0xF0 clears +0x5F and runs aiDarkToothBossUpdate. The dead
 * +0x37 load in retail has no effect; the caller ignores $v0. */
void __attribute__((section(".text_80103CA0")))
aiWeaponFireDecide(struct TM2AIRecord *ai)
{
    int n = 4;

    if (ai->active != 0) {
        unsigned char dead = ai->f037;  /* dead read (retail) */
        if (ai->f038 != 0) n = 3;
        if (ai->f039 != 0) n = 2;
        if (ai->f03A != 0) n = 1;
        if (__builtin_abs(ai->target_angle) < 136 && ai->target_dist < 4000 && ai->state.weapon_fired == 0)
            ai->input.fire_guns = 1;
        if (__builtin_abs(ai->target_angle) < n * 113) {
            int chance = ai->weapons.fire_chance;
            if (rand() / 328 < chance) {
                ai->weapons.rear_fire = 0;
                if (ai->weapons.selected < 8) {
                    ai->input.fire = 1;
                    ai->input.fire_special = 0;
                } else {
                    ai->input.fire = 0;
                    ai->input.fire_special = 1;
                }
                if (TM2_SPECIAL_TYPE(ai->slot, ai->type) == 0x30) {   /* request 35: as-type */
                    if (ai->f038 != 0)
                        ai->weapons.selected = 0;
                    else if (ai->weapons.selected == 0)
                        ai->weapons.selected = 1;
                }
            } else {
                ai->input.fire = 0;
                if (ai->weapons.fire_cooldown <= 0)
                    ai->weapons.fire_cooldown = weaponFireInterval;
            }
        }
        if (ai->f0D4 == 1) {
            if (__builtin_abs(ai->target_angle) > ((180 - 10 * n) << 12) / 360) {
                int chance = ai->weapons.fire_chance;
                if (rand() / 328 < chance) {
                    ai->weapons.rear_fire = 1;
                    if (ai->weapons.selected < 8) {
                        ai->input.fire = 1;
                        ai->input.fire_special = 0;
                    } else {
                        ai->input.fire = 0;
                        ai->input.fire_special = 1;
                    }
                } else {
                    ai->input.fire = 0;
                    ai->input.fire_special = 0;
                    if (ai->weapons.fire_cooldown <= 0)
                        ai->weapons.fire_cooldown = weaponFireInterval;
                }
            }
        }
    }
#ifdef TM2_PORT_DARK_TOOTH
    /* request 33-2e (CAR_GROUPS.md 11): the boss update runs on this Dark Tooth's own head and state (its key
       selected); with portCarOptions bit 6 on every arena (SURVIVAL.md 1.5 #1); request 42 item 1: with bit 10 an
       ordinary Dark Tooth's too (his first phase on every arena, his head shown with his car) */
    if ((gameGetLevel() == 8 || TM2_PORT_BOSS_ANYWHERE || TM2_DT_ORDINARY(ai->slot)) && ai->type == 0xf0) {
        int special;

        ai->input.fire_guns = 0;
        special = portCarSpecialSelect(TM2_CAR_KEY(ai->slot, ai->type));
        portCarDarkToothHeadShow(ai);
        aiDarkToothBossUpdate(ai);
        portCarSpecialRestore(special);
    }
#else
    if (gameGetLevel() == 8 && ai->type == 0xf0) {
        ai->input.fire_guns = 0;
        aiDarkToothBossUpdate(ai);
    }
#endif
}
