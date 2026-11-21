/* Pooled weapon effect objects allocated by the asset loader weaponAssetBind (assets 2210 / 2217); word +8
 * of a pooled object is its active flag. The table layouts repeat those of weapon_select.c (weaponRicochetPool)
 * and weapon_special.c (weaponShieldPool); a shared header would change the frozen header hash every compile
 * receipt is checked against, so the declarations stay per TU. */
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
struct TM2EffectShot { short f0, model, range, f6; int f8, life; int f10[7]; void *obj; };
struct TM2EffectTable { void *slots[TM2_MAX_RICOCHETS]; struct TM2EffectShot shots[TM2_MAX_RICOCHETS]; };
struct TM2SpecialRecord { short life, owner; void *lookup; void *obj; };
struct TM2SpecialTable { void *slots[TM2_MAX_SHIELDS]; struct TM2SpecialRecord records[TM2_MAX_SHIELDS]; };
extern struct TM2EffectTable weaponRicochetPool;
extern struct TM2SpecialTable weaponShieldPool;

#define S32(p,n) (*(int *)((unsigned char *)(p)+(n)))

/* @80109F14. First free slot of the ten pooled effect objects (word +8 of the object clear), or -1.
 * Called by weaponFireSelected for weapon kind 6. The result is used by the caller. */
int __attribute__((section(".text_80109F14")))
weaponRicochetSlotFind(void)
{
    int i;

    for (i = 0; i < TM2_POOL_LIVE(RICOCHETS); i++) {   /* the objects created this load */
        if (S32(weaponRicochetPool.slots[i], 8) == 0)
            return i;
    }
    return -1;
}

/* @80109F54. First free slot of the four pooled special-effect objects, or -1.
 * Called by weaponFireEnergyAttack for special kind 103. The result is used by the caller. */
int __attribute__((section(".text_80109F54")))
weaponShieldSlotFind(void)
{
    int i;

    for (i = 0; i < TM2_POOL_LIVE(SHIELDS); i++) {   /* the objects created this load */
        if (S32(weaponShieldPool.slots[i], 8) == 0)
            return i;
    }
    return -1;
}
