#include "semantic/cars.h"   /* u8..u32, TM2HumanRecord / TM2AIRecord blocks */

#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)

/* pickupAmmoAmounts[k] is pickupAmmoTable[k + 1] (8018429C, indexed by weapon id;
 * arena_load_register.c fills it): this definition starts one entry in. */
s32 pickupAmmoAmounts[7] __attribute__((section(".tm2_801842A0")))={0};
/* @80183F7C: 16-byte pickup slot records (CRT-zero, resolved by address). */
struct TM2PickupSlot { s16 x0, active; u8 pad4[12]; };
extern struct TM2PickupSlot pickupSlots[];
extern void hudMessagePost(s32 sound,s32 value);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *position);
extern void soundLocateForView(void *position,s32 value);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
#include "semantic/weapon_calls.h"
extern void soundSetDeniedPending(u8 value);
#include "semantic/port_humans.h"   /* H3: a human's pickup messages and the "full" buzz only where it is local */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
/* Weapon ammo pickup: add the table amount (capped by the free capacity) to
 * the weapon count, select the weapon when the selector is empty (8). */
#define AMMO(weapon, k, snd) \
    amount = pickupAmmoAmounts[k]; \
    if (remaining < amount) \
        amount = remaining; \
    inventory->ammo[weapon] += amount; \
    if (inventory->selected == 8) \
        inventory->selected = (weapon); \
    if (side & 0xFF) \
        TM2_HUMAN_MESSAGE(HUMAN->player, snd, 10); \
    break

/* The switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800CF154

/* @8010AFC8. Apply one pickup/effect ID to the selected car inventory.
 * Old-style definition: the side byte arrives promoted, as in retail. */
void __attribute__((section(".text_8010AFC8")))
pickupApply(car,side,id,slot_index)
void *car; u8 side; s32 id; s32 slot_index;   /* car: human record when side is set, else AI */
{
    struct TM2CarWeapons *inventory;
    struct TM2CarMotion *motion;
    struct TM2CarStatsBlock *geometry;
    s32 special = 0;
    s32 remaining, amount, i;
    if (id >= 441) { special = 1; id -= 40; }
    inventory = &HUMAN->weapons;
    if (side & 0xFF) { motion = &HUMAN->motion; geometry = &HUMAN->stats; }
    else { inventory = &AI->weapons; motion = &AI->motion; geometry = &AI->stats; }
    remaining = inventory->capacity - inventory->held;
    if (remaining <= 0 && id != 430 && (u32)(id - 417) >= 2 && (side & 0xFF))
        goto full;
    switch (id - 401) {
    case 0: AMMO(1, 0, 0xD);
    case 2: AMMO(2, 1, 0xE);
    case 3: AMMO(4, 3, 0xF);
    case 5: AMMO(3, 2, 0x10);
    case 10: AMMO(5, 4, 0x11);
    case 12: AMMO(6, 5, 0x12);
    case 16:
        if (geometry->hp > 0) {
            geometry->hp = geometry->max_hp;
            if (side & 0xFF)
                TM2_HUMAN_MESSAGE(HUMAN->player, 0x13, 10);
        }
        break;
    case 17:
        if (geometry->hp > 0) {
            geometry->hp += geometry->max_hp / 3;
            geometry->hp = MIN(geometry->hp, geometry->max_hp);
            if (side & 0xFF)
                TM2_HUMAN_MESSAGE(HUMAN->player, 0x13, 10);
        }
        break;
    case 18: AMMO(7, 6, 0x14);
    case 29:
        inventory->turbo += 100;
        if (inventory->turbo >= 1000)
            inventory->turbo = 999;
        if (side & 0xFF)
            TM2_HUMAN_MESSAGE(HUMAN->player, 0x15, 10);
        break;
    }
    if ((side & 0xFF) || AI->active != 0) {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(motion->pos);
        else
            soundLocateForView(motion->pos, -1);
        soundPlay(0x18, PadChkMtap(), soundGetListenerPan(), 0x63);
    }
    if (!special) {
        pickupSetVisible(id, slot_index, 0);
        pickupSlots[slot_index].active = 0;
    }
    inventory->held = 0;
    for (i = 1; i < 8; i++)
        inventory->held += inventory->ammo[i];
    if (inventory->held >= inventory->capacity && id != 430 &&
        (u32)(id - 417) >= 2 && (side & 0xFF)) {
full:
        TM2_HUMAN_MESSAGE(HUMAN->player, 0x24, 3);
        TM2_HUMAN_DENIED(HUMAN->player);
    }
}
