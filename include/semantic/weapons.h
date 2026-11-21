/* Weapon-side records: pooled scene objects and the ricochet (rolling bomb)
 * shot table. See also include/semantic/cars.h for the car blocks. */
#ifndef TM2_SEMANTIC_WEAPONS_H
#define TM2_SEMANTIC_WEAPONS_H

#include "semantic/cars.h"

/* Scene object (csAlloc / csListAdd), as the weapon code sees it. The
 * hit pass skips an object when (visible & collides) is 0. */
struct TM2SceneObject {
    u8 pad_000[0x8];
    s32 visible;                    /* +0x008 drawn / active */
    s32 collides;                   /* +0x00C */
    u8 pad_010[0x70];
    struct TM2CarMatrix matrix;     /* +0x080 from rot */
    s16 rot[3];                     /* +0x0A0 pitch, roll, yaw */
    u8 pad_0A6[0x2];
    s32 pos[3];                     /* +0x0A8 */
    s32 fB4;                        /* +0x0B4 */
    s16 fB8;                        /* +0x0B8 */
    s16 fBA;                        /* +0x0BA */
    s16 size;                       /* +0x0BC */
    u8 pad_0BE[0x5A];
    s32 id;                         /* +0x118 object id */
    s32 model;                      /* +0x11C */
};
TM2_FIELD_AT(tm2_scene_object_id_at, struct TM2SceneObject, id, 0x118);
TM2_FIELD_AT(tm2_scene_object_pos_at, struct TM2SceneObject, pos, 0xA8);

/* Ricochet (rolling bomb, weapon kind 6) shot, 48 bytes; ten of them follow
 * the ten pooled objects in weaponRicochetPool (storage in asset_pools.c). */
struct TM2RicochetShot {
    s8 power;                       /* +0x00 blast power */
    u8 left_owner;                  /* +0x01 unused by retail; request 41 bit 18: the shot has been outside its
                                       owner's box (port_balance.h portBalanceRicochetOwner) */
    s16 owner;                      /* +0x02 owner object id (>= 50 for AI) */
    s16 life;                       /* +0x04 ticks left (starts at 400) */
    s16 bounces;                    /* +0x06 */
    s32 offset[3];                  /* +0x08 local velocity {0, 100, 0} */
    s32 vel[3];                     /* +0x14 world velocity this tick */
    s32 prev[3];                    /* +0x20 position before the move */
    struct TM2SceneObject *obj;     /* +0x2C */
};
typedef char tm2_ricochet_shot_size[(sizeof(struct TM2RicochetShot) == 48) ? 1 : -1];

#include "semantic/players.h"   /* TM2_MAX_RICOCHETS (10; 2 per car with TM2_PORT_CARS) */
struct TM2RicochetTable {
    struct TM2SceneObject *slots[TM2_MAX_RICOCHETS];   /* pooled objects (asset 0x8A2) */
    struct TM2RicochetShot shots[TM2_MAX_RICOCHETS];
};
typedef char tm2_ricochet_table_size[(sizeof(struct TM2RicochetTable) == 52 * TM2_MAX_RICOCHETS) ? 1 : -1];

/* Projectile record (fxRecordGet(slot), fxRecords[16], 980 bytes; only the
 * head is described), docs/GAME_EVENTS.md 3.2. */
struct TM2Projectile {
    u8 pad_00[0x2];
    s16 type;                       /* +0x02 */
    s16 life;                       /* +0x04 */
    s16 speed;                      /* +0x06 */
    s16 turn_rate;                  /* +0x08 */
    s16 damage;                     /* +0x0A */
    u8 pad_0C[0x28];
    struct TM2SceneObject *obj;     /* +0x34 */
    void *target;                   /* +0x38 */
    u8 pad_3C[0x6];
    s16 owner;                      /* +0x42 owner object id */
    u8 pad_44[0x2];
    u16 ground_offset;              /* +0x46 shooter's ride height - 24 */
};
TM2_FIELD_AT(tm2_projectile_owner_at, struct TM2Projectile, owner, 0x42);

#include "semantic/weapon_calls.h"

#endif
