#include "semantic/weapons.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

extern struct TM2SceneObject *humanCarObject(int);
extern struct TM2SceneObject *aiCarObject(int);
extern void weaponFindTargetBehind(int, int *, s32 mode);
extern void weaponFindTargetAhead(int, int *, s32 mode);
extern s16 weaponFireMissile(int, int, int *, int *, int);
extern s16 weaponFireHomingMissile(int, int, int *, int *, int);
extern s16 weaponFireNapalm(int, int, int *, int *, int);
extern s16 weaponFireMegaNapalm(int, int, int *, int *, int);
extern s16 weaponFirePowerMissile(int, int *, int *, int);
extern int weaponFireFreezeMissile(int, int, int *, int *, int);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, int);
extern int PadChkMtap(void);
extern int soundGetListenerPan(void);
extern int soundPlayVariant(int, int, int, int, int);
extern int soundPlay(int, int, int, int);
extern struct TM2Projectile *fxRecordGet(int);

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* @801052BC. Projectile launch shared by the selection step (kinds 1, 2, 4, 5) and the special step
 * (100, 102). The low byte of `human` selects the actor layout (as in the tick): car is a human
 * record when it is set, else an AI record; `kind` is the full weapon kind word. The muzzle position is the motion position with the geometry height +0x44 + 8
 * added to z; the launch angles come from the motion block, or -- when the flip byte weapon+5 is set
 * (and cleared) -- from the yaw turned by 0x800 with a zero pitch. Every kind but 4 first asks the
 * effect id writer (weaponFindTargetBehind when flipped, else weaponFindTargetAhead). The emitter's signed-16 result is
 * the projectile slot: for the player or an active AI (+0x34) a non-negative slot plays the launch
 * sound with the controller pad start/stop on the muzzle vector; a negative slot returns at once.
 * Finally a valid slot marks flags +9, and fxRecordGet(slot)+0x46 receives the geometry +0x42 minus
 * 24 (or 0). The callers ignore $v0 (IDA's int is residue), so the function is void as M2C renders it. */
#define MODE (human & 255)
/* Launch angles from the motion block; a set flip byte (cleared here) turns
 * the yaw by 0x800 with zero pitch. The effect lookups run first. */
#define AIM(flip_effect, effect_call) \
    if (weap->rear_fire != 0) { \
        weap->rear_fire = 0; \
        flip_effect; \
        ang[0] = motion->rot[0]; ang[1] = 0; ang[2] = (motion->rot[2] + 0x800) & 0xfff; \
    } else { \
        effect_call; \
        ang[0] = motion->rot[0]; ang[1] = motion->rot[1]; ang[2] = motion->rot[2]; \
    }
#define AIM_EFFECT AIM(weaponFindTargetBehind(owner, &effect, MODE), weaponFindTargetAhead(owner, &effect, MODE))
/* Player or active AI: a valid slot plays the launch sound with the pad
 * start/stop on the muzzle; a negative slot skips it. The "> -1" test keeps
 * it from being threaded into the final slot test, as in retail. */
#define NOTIFY(sound) \
    if ((MODE || AI(car)->active != 0) && slot > -1) { \
        if (viewGetSplitMode() == 0) \
            soundLocateForView0(pos); \
        else \
            soundLocateForView(pos, -1); \
        mtap = PadChkMtap(); \
        sound; \
    }

void __attribute__((section(".text_801052BC")))
weaponLaunchProjectile(void *car, int kind, unsigned int human)
{
    struct TM2SceneObject *lookup;
    struct TM2CarWeapons *weap;
    struct TM2CarState *flags;
    struct TM2CarMotion *motion;
    struct TM2CarStatsBlock *geom;
    struct TM2Projectile *proj;
    int owner, slot, mtap, effect;
    int pos[3], ang[3];

    if (MODE) {
        geom = &HUMAN(car)->stats; motion = &HUMAN(car)->motion;
        owner = HUMAN(car)->player;   /* read, then re-read after the call, as in retail */
        lookup = humanCarObject(HUMAN(car)->player);
        owner = HUMAN(car)->player; weap = &HUMAN(car)->weapons; flags = &HUMAN(car)->state;
    } else {
        geom = &AI(car)->stats; motion = &AI(car)->motion;
        owner = AI(car)->index;
        lookup = aiCarObject(AI(car)->index);
        owner = AI(car)->index; weap = &AI(car)->weapons; flags = &AI(car)->state;
    }
    pos[0] = motion->pos[0];
    pos[1] = motion->pos[1];
    pos[2] = motion->pos[2] + geom->launch_height + 8;
    switch (kind) {
    case 2:
        AIM_EFFECT
#ifdef TM2_PORT_BALANCE
        /* request 40 bit 0 (BALANCE.md 3.1): 7 */
        slot = (short)weaponFireHomingMissile(lookup->id, effect, ang, pos, portBalanceMissile(2, 10));
#else
        slot = (short)weaponFireHomingMissile(lookup->id, effect, ang, pos, 10);
#endif
        NOTIFY(soundPlayVariant(11, mtap, soundGetListenerPan(), 99, slot + 1))
        break;
    case 5:
        AIM_EFFECT
        if (weap->mega_napalm != 0)
            slot = (short)weaponFireMegaNapalm(lookup->id, effect, ang, pos, 30);
        else
            slot = (short)weaponFireNapalm(lookup->id, effect, ang, pos, 10);
        NOTIFY(soundPlayVariant(15, mtap, soundGetListenerPan(), 99, slot + 1))
        break;
    case 4:
        AIM(, )
        slot = (short)weaponFirePowerMissile(lookup->id, ang, pos, 15);
        NOTIFY(soundPlayVariant(12, mtap, soundGetListenerPan(), 99, slot + 1))
        break;
    case 1:
        AIM_EFFECT
#ifdef TM2_PORT_BALANCE
        /* request 40 bit 0 (BALANCE.md 3.1): 10 */
        slot = (short)weaponFireMissile(lookup->id, effect, ang, pos, portBalanceMissile(1, 7));
#else
        slot = (short)weaponFireMissile(lookup->id, effect, ang, pos, 7);
#endif
        NOTIFY(soundPlayVariant(10, mtap, soundGetListenerPan(), 99, slot + 1))
        break;
    case 100:
        AIM_EFFECT
        slot = (short)weaponFireFreezeMissile(lookup->id, effect, ang, pos, 7);
        NOTIFY(soundPlay(14, mtap, soundGetListenerPan(), 99))
        break;
    case 102:
        AIM_EFFECT
        if (weap->mega_napalm != 0)
            slot = (short)weaponFireMegaNapalm(lookup->id, effect, ang, pos, 30);
        else
            slot = (short)weaponFireNapalm(lookup->id, effect, ang, pos, 10);
        NOTIFY(soundPlayVariant(15, mtap, soundGetListenerPan(), 99, slot + 1))
        break;
    default:
        slot = -1;
        break;
    }
    if (slot >= 0) {
        flags->weapon_fired = 1;
        proj = fxRecordGet(slot);
        proj->ground_offset = (geom->ride_height - 24 > 0) ? (unsigned short)(geom->ride_height - 24) : 0;
    }
}
