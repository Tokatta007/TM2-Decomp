#include "semantic/fd44.h"
#include "semantic/weapons.h"

#include "semantic/world.h"
extern int weaponRicochetModelAlt;
extern int weaponRicochetModel;

/* Pooled scene objects created by the asset loader for weapon effects. The table layouts repeat those
 * of weapon_select.c (weaponRicochetPool) and weapon_special.c (weaponShieldPool), whose readers keep them as imports;
 * this TU owns the storage (all zero, CRT-cleared) because it is the only writer of the slot pointers. */
struct TM2SpecialRecord { short life, owner; void *lookup; void *obj; };
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
struct TM2SpecialTable { void *slots[TM2_MAX_SHIELDS]; struct TM2SpecialRecord records[TM2_MAX_SHIELDS]; };
typedef char tm2_pool_sizes[(sizeof(struct TM2RicochetTable) == 52 * TM2_MAX_RICOCHETS && sizeof(struct TM2SpecialTable) == 16 * TM2_MAX_SHIELDS) ? 1 : -1];
/* TM2_PORT_CARS raises the ricochet, shield and remote-bomb pools: port_variant_arrays.c (.port_bss) holds them */
#if TM2_MAX_RICOCHETS > TM2_RETAIL_RICOCHETS
extern struct TM2RicochetTable weaponRicochetPool;
#else
struct TM2RicochetTable weaponRicochetPool __attribute__((section(".tm2_80184860"))) = {{0}};       /* @80184860 ten effect objects + shot records */
#endif
#if TM2_MAX_SHIELDS > TM2_RETAIL_SHIELDS
extern struct TM2SpecialTable weaponShieldPool;
#else
struct TM2SpecialTable weaponShieldPool __attribute__((section(".tm2_80184A68"))) = {{0}};      /* @80184A68 four special objects + records */
#endif
#if TM2_MAX_MINE_OBJECTS > TM2_RETAIL_MINE_OBJECTS
extern unsigned char *weaponMineObjects[TM2_MAX_MINE_OBJECTS];   /* request 40 (TM2_PORT_BALANCE + TM2_PORT_CARS) */
#else
unsigned char *weaponMineObjects[23] __attribute__((section(".tm2_80184AA8"))) = {0};         /* @80184AA8 asset 0x8A4 objects */
#endif
#if TM2_MAX_REMOTE_BOMBS > TM2_RETAIL_REMOTE_BOMBS
extern unsigned char *weaponRemoteBombObjects[TM2_MAX_REMOTE_BOMBS];
#else
unsigned char *weaponRemoteBombObjects[2] __attribute__((section(".tm2_80180F30"))) = {0};          /* @80180F30 asset 0x8A1 objects ($gp) */
#endif
unsigned char *weaponTornadoObject __attribute__((section(".tm2_80180F38"))) = 0;             /* @80180F38 asset 0x913 object ($gp) */
unsigned char *weaponTornadoFunnelObject __attribute__((section(".tm2_80180F3C"))) = 0;             /* @80180F3C asset 0xA0 object ($gp) */
/* @80184E70 asset 0x915 object (CRT-zero storage, resolved by address):
 * retail addresses it absolutely, not through $gp. */
struct TM2StrikeBlock { unsigned char state[44]; int corners[12]; unsigned char *weapon; unsigned char *object; };
extern struct TM2StrikeBlock weaponStrikeState;   /* object = @80184E70 */
int weaponStrikeCornerOffsets[12] __attribute__((section(".tm2_80184E3C"))) = {0};                    /* @80184E3C four corner offsets for the 0x915 object */

#define OBJ(p) ((struct TM2SceneObject *)(p))

/* Pool fill and projectile defaults, repeated per asset in the one retail
 * function; every field access goes back through the table entry or global,
 * as in retail. */
#define POOL_FILL(table, count)     i = 0;     do {         (table)[i] = csAlloc();         csListAdd((table)[i]);         OBJ((table)[i])->model = model;         OBJ((table)[i])->visible = 0;         i++;     } while (i < (count))

#define PROJECTILE(obj, sz)     OBJ(obj)->model = model;     OBJ(obj)->visible = 0;     OBJ(obj)->id = 0x8a2;     OBJ(obj)->collides = 1;     OBJ(obj)->fB4 = 1;     OBJ(obj)->fB8 = 0;     OBJ(obj)->fBA = 0;     OBJ(obj)->size = (sz)

/* @80109038. Asset-loader hook (called by worldLoadObject with the loaded object and the asset id): allocates
 * the pooled scene objects for the weapon effect assets and records the object id for the asset kinds the
 * weapon code reads back. Each pooled object is created by csAlloc, initialised by csListAdd, given
 * the model word +0x11C and an inactive flag +8 (asset 0xF9's single object starts active). The projectile
 * pool 0x8A2 and the 0x915 object also get the projectile defaults (+0x118 = 0x8A2, +0xC and +0xB4 = 1,
 * halfwords +0xB8/+0xBA = 0, +0xBC = 40 or 80) and the 0x915 case sets its four corner offsets. Asset 0x8A6
 * only records its id in weaponRicochetModelAlt; the 0x8A2 loop records its id in weaponRicochetModel on every iteration. The
 * caller ignores $v0, so the function is void (M2C void). Field accesses are struct members, which lets
 * the 0x8A2 loop schedule its weaponRicochetModel store above the last object store, as in retail. */
void __attribute__((section(".text_80109038")))
weaponAssetBind(int model, int asset)
{
    int i;
#ifdef TM2_PORT_CAR_SPECIALS
    /* request 33-2a (CAR_GROUPS.md 10): Twister's tornado (2323) and funnel (its root, 160) and Shadow's
       strike object (2325) go to the state of the car being installed (portCarLoadKey: a duplicate's own,
       so they draw that car's DMD copy and, with texture banks, its bank); the first car's: retail's */
    int special = portCarSpecialSelect(portCarLoadKey());
#endif

    switch (asset) {
    case 0x8a4:
        POOL_FILL(weaponMineObjects, TM2_MINE_OBJECTS_LIVE);   /* 23 (request 40: 5 N when raised) */
        break;
    case 0x8a1:
        POOL_FILL(weaponRemoteBombObjects, TM2_POOL_LIVE(REMOTE_BOMBS));
        break;
    case 0x8a2:
        i = 0;
        do {
            weaponRicochetPool.slots[i] = csAlloc();
            csListAdd(weaponRicochetPool.slots[i]);
            PROJECTILE(weaponRicochetPool.slots[i], 40);
            weaponRicochetModel = model;
            i++;
        } while (i < TM2_POOL_LIVE(RICOCHETS));
        break;
    case 0x8a6:
        weaponRicochetModelAlt = model;
        break;
    case 0x8a9:
        POOL_FILL((unsigned char **)weaponShieldPool.slots, TM2_POOL_LIVE(SHIELDS));
        break;
    case 0x913:
        weaponTornadoObject = csAlloc();
        csListAdd(weaponTornadoObject);
        OBJ(weaponTornadoObject)->model = model;
        OBJ(weaponTornadoObject)->visible = 0;
        break;
    case 0xa0:
        weaponTornadoFunnelObject = csAlloc();
        csListAdd(weaponTornadoFunnelObject);
        OBJ(weaponTornadoFunnelObject)->model = model;
        OBJ(weaponTornadoFunnelObject)->visible = 0;
        OBJ(weaponTornadoFunnelObject)->collides = 0;
        break;
    case 0x915:
        weaponStrikeState.object = csAlloc();
        csListAdd(weaponStrikeState.object);
        PROJECTILE(weaponStrikeState.object, 80);
        weaponStrikeCornerOffsets[0] = 0;   weaponStrikeCornerOffsets[1] = 40;  weaponStrikeCornerOffsets[2] = 0;
        weaponStrikeCornerOffsets[3] = 0;   weaponStrikeCornerOffsets[4] = -40; weaponStrikeCornerOffsets[5] = 0;
        weaponStrikeCornerOffsets[6] = -40; weaponStrikeCornerOffsets[7] = 0;   weaponStrikeCornerOffsets[8] = 0;
        weaponStrikeCornerOffsets[9] = 40;  weaponStrikeCornerOffsets[10] = 0;  weaponStrikeCornerOffsets[11] = 0;
        break;
    case 0xf9:
        darkToothBossObject = csAlloc();
        csListAdd(darkToothBossObject);
        OBJ(darkToothBossObject)->model = model;
        OBJ(darkToothBossObject)->visible = 1;
        break;
    }
#ifdef TM2_PORT_CAR_SPECIALS
    portCarSpecialRestore(special);
#endif
}
