#include "semantic/players.h"   /* TM2_MAX_DROPS, TM2_MAX_MINE_OBJECTS */
typedef unsigned char u8;
/* @80184B04: 25 weapon slots (28 bytes; TM2_MAX_DROPS, request 40), field view used here. */
struct TM2WeaponSlot {
    u8 kind, held, life, pad3;
    unsigned short id, owner;      /* +4, +6 */
    short status;                  /* +8 */
    unsigned short extra;          /* +10 */
    int fC, f10, f14;              /* +12: position */
    int active;                    /* +28: object */
};
typedef char tm2_weapon_slot_size[(sizeof(struct TM2WeaponSlot) == 28) ? 1 : -1];
extern struct TM2WeaponSlot weaponDrops[TM2_MAX_DROPS];
extern u8 *weaponMineObjects[TM2_MAX_MINE_OBJECTS];
extern u8 *weaponRemoteBombObjects[2];
extern int weaponDropSlotFind(void);
extern int weaponRemoteBombFind(int);
extern int weaponMineObjectFind(void);
extern unsigned int weaponBombObjectFind(void);
extern int gameGetLevel(void);
extern void moscowEventsTrigger(int *);
extern void laEventsTrigger(int *);
extern void eiffelTowerBombCheck(int *);
#include "semantic/world.h"
#include "semantic/players.h"   /* TM2_MAX_REMOTE_BOMBS */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif
extern void fxExplosion192(int *, u8 flag);
extern void *memcpy(void *, const void *, unsigned int);

#define H(p,n) (*(unsigned short *)((u8 *)(p)+(n)))
#define B(p,n) (*((u8 *)(p)+(n)))

#define REC weaponDrops[index]

/* @801096F4. Weapon slot writer: command 101 opens a slot with an effect
 * object, command 3 resets a matching slot (or falls back to opening one with
 * a pickup object); anything else clears @80184AF0. Returns slot + 1 (low
 * byte) or 0. */
unsigned int __attribute__((section(".text_801096F4")))
weaponDropWrite(arg0, arg1, arg2, arg3, arg4)
int arg0; int arg1; unsigned short arg2; int *arg3; int arg4;   /* old-style: the caller passes an int, the body narrows */
{
    int index, object_index, found;
    u8 *object;
    int *position;

    switch ((signed char)arg0) {
    case 101:
#ifdef TM2_PORT_BALANCE
        /* request 40 bit 2 (BALANCE.md 3.3): the mine range, the driver's oldest removed at 5, and a record only
           when a mine object is free (retail's stale / null object pointer, RETAIL_BUGS.md 22) */
        index = portBalanceMineSlot(arg1);
#else
        index = weaponDropSlotFind();
#endif
        if (index < 0)
            return 0;
        REC.id = (unsigned short)arg1;
        REC.owner = (unsigned short)arg2;
        REC.extra = (unsigned short)arg4;
        position = &REC.fC;
        position[0] = arg3[0];
        position[1] = arg3[1];
        position[2] = arg3[2] - 24;
        object_index = weaponMineObjectFind();
        if (object_index >= 0) {
            object = weaponMineObjects[object_index];
            REC.active = (int)object;
            *(int *)(object + 8) = 1;
            memcpy((u8 *)REC.active + 0xa8, position, 12);
        }
        REC.life = 15;
        REC.status = 300;
        REC.kind = 20;
        break;
    case 3:
        found = weaponRemoteBombFind(arg1);
        if (found >= 0) {
            index = found;
            REC.status = 0;
            REC.held = 0;
            REC.status = 0;
            if (gameGetLevel() == 2) moscowEventsTrigger(&REC.fC);
            if (gameGetLevel() == 1) laEventsTrigger(&REC.fC);
            if (gameGetLevel() == 3) eiffelTowerBombCheck(&REC.fC);
            areaDamage(&REC.fC, (short)REC.owner, 1);
            REC.f14 += 240;
            fxExplosion192(&REC.fC, 0);
            *(int *)((u8 *)REC.active + 8) = 0;
            break;
        }
#ifdef TM2_PORT_BALANCE
        index = portBalanceBombSlot();   /* request 40 bit 2: the remote bombs' own records first */
#else
        index = weaponDropSlotFind();
#endif
        if (index < 0)
            return 0;
        REC.id = (unsigned short)arg1;
        REC.owner = (unsigned short)arg2;
        REC.extra = (unsigned short)arg4;
        position = &REC.fC;
        position[0] = arg3[0];
        position[1] = arg3[1];
        position[2] = arg3[2] - 24;
        object_index = weaponBombObjectFind();
        if ((unsigned int)object_index < TM2_MAX_REMOTE_BOMBS) {
            object = weaponRemoteBombObjects[object_index];
            REC.active = (int)object;
            *(int *)(object + 8) = 1;
            memcpy((u8 *)REC.active + 0xa8, position, 12);
            REC.status = 0x7fff;
            REC.life = 25;
            REC.kind = 19;
            REC.held = 1;
        }
        break;
    default:
        /* 80184AF0: the slot before weaponDrops[0] (also the low halfword of
         * weaponMineObjects[18]); retail clears it with a direct address. No caller reaches it (only kinds 3 and
         * 101 are passed); once request 40 moves the table (TM2_MAX_DROPS raised) it would land before the moved
         * table, so the store is left out there. */
#if TM2_MAX_DROPS <= TM2_RETAIL_DROPS
        weaponDrops[-1].status = 0;
#endif
        return 0;
    }
    return (unsigned char)(index + 1);
}
