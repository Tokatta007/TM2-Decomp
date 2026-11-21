#include "semantic/cars.h"

extern void aiPickWeapon(struct TM2AIRecord *ai);
#include "semantic/weapon_calls.h"
extern struct TM2Object *humanCarObject(int);
extern struct TM2Object *aiCarObject(int);
extern int worldGroundHeightAt(int *);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int weaponDropWrite(int, int, int, int *, int);

#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

/* @801073B4. Weapon-slot cycling for a weapon block: the signed-8 selector +0xD is reset to 0 when it is
 * outside 0..7, then stepped down (direction 1, wrapping to count-1) or up (any other direction, wrapping
 * to 0) until a slot with a positive signed-16 count (+0xE + 2*selector) is found or the selector returns
 * to its starting value; when the final slot is still empty the selector becomes the count. Leaf function;
 * the callers (weaponHumanCycleInput and weaponFireSelected) ignore $v0. The down loop leaves straight to the final check
 * (retail keeps "bgtz exit; j top"). */
void __attribute__((section(".text_801073B4")))
weaponCycleSelection(struct TM2CarWeapons *weap, int step, int count)
{
    int start;

    if (weap->selected >= 8 || weap->selected < 0)
        weap->selected = 0;
    start = weap->selected;
    if (step == 1) {
        weap->selected--;
        if (weap->selected < 0)
            weap->selected = count - 1;
        if (weap->ammo[weap->selected] <= 0) {
            for (;;) {
                int wrap = count - 1;   /* block scope: keeps this loop unrotated, as in retail */
                if (weap->selected == start)
                    break;
                weap->selected--;
                if (weap->selected < 0)
                    weap->selected = wrap;
                if (weap->ammo[weap->selected] > 0)
                    goto done;
            }
        }
    } else {
        weap->selected++;
        if (weap->selected >= count)
            weap->selected = 0;
        while (weap->ammo[weap->selected] <= 0) {
            if (weap->selected == start)
                break;
            weap->selected++;
        if (weap->selected >= count)
            weap->selected = 0;
        }
    }
done:
    if (weap->ammo[weap->selected] <= 0)
        weap->selected = count;
}

/* @800D3BB8. AI weapon tick caller: for an active actor (+0x34) or the 0xF0 vehicle, runs aiPickWeapon
 * while the weapons fire_cooldown (+0x82E) is below 2, then the weapon/controller tick in the AI layout. The
 * caller ignores $v0. */
void __attribute__((section(".text_800D3BB8")))
aiWeaponsUpdate(struct TM2AIRecord *ai)
{
    if (ai->active != 0 || ai->type == 0xf0) {
        if (ai->weapons.fire_cooldown < 2)
            aiPickWeapon(ai);
        weaponTick(ai, 0);
    }
}

/* @80109518. Weapon-kind impact helper (toggle weapons, special kind 101): looks the owner up (layout by
 * the low byte of `human`), takes the motion position raised by 24, queries worldGroundHeightAt on it, runs the
 * controller pad start/stop chosen by viewGetSplitMode on it, and calls weaponDropWrite(kind, id, other id,
 * position, query) -- a nonzero byte result sets flags +9. The callers ignore $v0 (IDA's constant 1 is the
 * store residue). */
void __attribute__((section(".text_80109518")))
weaponDropItem(void *car, int kind, unsigned int human)
{
    struct TM2CarMotion *motion;
    struct TM2CarState *flags;
    unsigned int mode = human & 255;
    int pos[3], query;
    unsigned short other;
    short id;

    if (mode) {
        motion = &HUMAN->motion; flags = &HUMAN->state; id = TM2_HUMAN_ID(HUMAN->player);
        humanCarObject(HUMAN->player);
        other = HUMAN->slot;
    } else {
        motion = &AI->motion; flags = &AI->state; id = AI->index + 50;
        aiCarObject(AI->index);
        other = AI->slot;
    }
    pos[0] = motion->pos[0];
    pos[1] = motion->pos[1];
    pos[2] = motion->pos[2] + 24;
    query = worldGroundHeightAt(pos);
    if (viewGetSplitMode() == 0)
        soundLocateForView0(pos);
    else
        soundLocateForView(pos, -1);
    if (weaponDropWrite((signed char)kind, id, (short)other, pos, query) & 255)
        flags->weapon_fired = 1;
}
