/* Pickup spawn (8010EEA8): give a world object a pickup record from the
 * 5-slot round-robin pool (weaponSlamStateAlloc, weapon_tables.c) if it has none,
 * and reset the record. */
typedef unsigned char u8;
typedef signed short s16;

/* 18-byte pickup record (weaponSlamRecords, see weapon_tables.c). */
typedef struct {
    u8 b0, b1, b2, b3;
    u8 pad_4[3];
    u8 b7;
    u8 pad_8[3];
    u8 bB;
    u8 pad_C[4];
    s16 h10;
} TM2Pickup;

typedef struct {
    u8 pad_00[0x48];
    TM2Pickup *pickup;      /* +0x48 */
} TM2PickupObject;

extern TM2Pickup *weaponSlamStateAlloc(void);

/* @8010EEA8 */
void __attribute__((section(".text_8010EEA8"))) weaponSlamStateInit(TM2PickupObject *obj)
{
    if (!obj->pickup)
        obj->pickup = weaponSlamStateAlloc();
    if (obj->pickup) {
        obj->pickup->b0 = 0;
        obj->pickup->b1 = 1;
        obj->pickup->b2 = 0;
        obj->pickup->b7 = 0;
        obj->pickup->b3 = 1;
        obj->pickup->h10 = 0;
        obj->pickup->bB = 0;
    }
}
