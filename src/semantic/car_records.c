/* Car records and per-player helpers (8011C3EC..8011D854).
 *
 * Record layouts are in include/semantic/cars.h. Whether a car is driven by
 * a pad or by the AI depends only on which array it is in.
 *
 * carSlotObjects lists every car's world object (carWorldObjectCount entries) with its
 * kind in carSlotKinds (0 human, 1 AI) and its index in carSlotIndices. */

#define TM2_OWN_CAR_GETTERS /* defined below through TM2HumanCar / TM2AICar */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map; request 50: vacant records */

typedef struct TM2HumanCar Human;
typedef struct TM2AICar AICar;
typedef struct TM2Object Object;

extern s32 carWorldObjectCount;          /* world objects of all cars */
extern Object *carSlotObjects[];    /* car world objects */
extern s32 carSlotKinds[];        /* car kind: 0 human, 1 AI */
extern s32 carSlotIndices[];        /* index in its array */
extern void *carSlotOwners[];      /* owner key of each car object */
extern s32 aiCarTargets[];        /* AI target: AI index, or -1 for a human */
extern s32 humanCarObjectSlots[];        /* object slot of each human */
extern s32 aiCarObjectSlots[];        /* object slot of each AI car */
extern s32 padConfigs[];        /* pad configuration per player */
extern s32 hudSmoothedSpeeds[];        /* smoothed speed per player */
extern Object *viewCameraObjectTable[];    /* camera objects */
extern s32 carViewHotkeyReleased[];        /* hotkey released, per player */
extern u8 aiBossDefeatedFlag;
extern u8 arenaShortChaseRange;
extern u8 viewExtraViewsUnlocked;           /* 1: extra views unlocked */
extern s32 viewWatchedAiCar;          /* AI car the player is watching */
extern s32 aiDifficulty;          /* difficulty */

extern s32 viewGetSplitMode(void);
extern void viewSetSplitMode(s32);
extern s32 renderGetView0Frames(void);
extern s32 renderGetView1Frames(void);
extern s32 rtGetRearViewActive(void);
extern void rtToggleRearView(void);
extern void rtSetRearView(s32);
extern void viewCreateWithCamera(s32, s32, s32);
#include "semantic/hud.h"
extern void hudToggleRadar(s32);
extern void hudToggleWeaponList(s32);
extern void carSetViewMode(Human *, s32);
#include "semantic/input.h"
extern void humanCarReadInput(Human *);
extern void humanCarTick(Human *);
extern void aiCarDrive(AICar *);
extern void mathCopyWords(void *, void *, s32);
extern s32 timerGetFrameTicks(void);
extern s32 mathBlendValue(s32, s32, s32);
extern void *RotMatrixYXZ(void *, void *);
extern s32 carStatsSetSpeedMult(void *, s32);

void carViewHotkeys(Human *car);
s32 humanCarNearest(s32 *pos, s32 exclude);
void aiApplyDifficulty(void);
void carApplyPadConfig(Human *car, s32 config);

/* @8011C3EC */
u8 __attribute__((section(".text_8011C3EC"))) aiBossDefeated(void)
{
    return aiBossDefeatedFlag;
}

/* @8011C3F8 */
void __attribute__((section(".text_8011C3F8"))) nullsub_8(void)
{
}

/* @8011C400: per-frame update of a human car and its world object. */
void __attribute__((section(".text_8011C400"))) humanCarUpdate(Object *object, Human *car)
{
    humanCarReadInput(car);
    carViewHotkeys(car);
    humanCarTick(car);
    object->rot[0] = car->rot[0];
    object->rot[1] = car->rot[1];
    object->rot[2] = car->rot[2];
    object->pos[0] = car->pos[0];
    object->pos[1] = car->pos[1];
    object->pos[2] = car->pos[2];
    mathCopyWords(object->matrix, car->matrix, 8);
}

/* @8011C4A0: per-frame update of an AI car and its world object. */
void __attribute__((section(".text_8011C4A0"))) aiCarUpdate(Object *object, AICar *car)
{
    aiCarDrive(car);
    object->rot[0] = car->rot[0];
    object->rot[1] = car->rot[1];
    object->rot[2] = car->rot[2];
    object->pos[0] = car->pos[0];
    object->pos[1] = car->pos[1];
    object->pos[2] = car->pos[2];
    mathCopyWords(object->matrix, car->matrix, 8);
}

/* @8011C530: a human player's view hotkeys (camera view, map, HUD, rear
 * view). Each hotkey acts once per press: carViewHotkeyReleased[player] re-arms when
 * no hotkey is held. TM2_PORT_HUMANS (H1): only a local human has hotkeys; the map
 * and weapon list it toggles are its slot's (retail: the player's, which wrote past
 * the u8[4] hudRadarVisible from human 4 on). */
void __attribute__((section(".text_8011C530"))) carViewHotkeys(Human *car)
{
    s32 player = car->player;
    u32 view = car->view;
    u32 old = view;
#ifdef TM2_PORT_HUMANS
    s32 slot = TM2_HUMAN_LOCAL(player);
#define HOTKEY_HUD slot
#else
#define HOTKEY_HUD player
#endif

#ifdef TM2_PORT_HUMANS
    if (slot < 0)
        return;     /* a remote human: no view, HUD or hotkeys on this machine */
#endif
    if (car->flag_0A7 || car->flag_09C)
        return;
    if (viewGetSplitMode()) {
        if (car->key_view) {
            if (!carViewHotkeyReleased[player])
                return;
            carViewHotkeyReleased[player] = 0;
            if (viewGetSplitMode() == 1) {
                viewSetSplitMode(2);
                viewCreateWithCamera(renderGetView0Frames(), 2, 0);
                viewCreateWithCamera(renderGetView1Frames(), 3, 1);
            } else {
                viewSetSplitMode(1);
                viewCreateWithCamera(renderGetView0Frames(), 5, 0);
                viewCreateWithCamera(renderGetView1Frames(), 6, 1);
            }
            hudInitPanels();
        } else if (car->key_0DA) {
            if (!carViewHotkeyReleased[player])
                return;
            carViewHotkeyReleased[player] = 0;
            hudToggleRadar(HOTKEY_HUD);
        } else if (car->key_0DB) {
            if (!carViewHotkeyReleased[player])
                return;
            carViewHotkeyReleased[player] = 0;
            hudToggleWeaponList(HOTKEY_HUD);
        } else
            carViewHotkeyReleased[player] = 1;
        return;
    }
    if (car->key_view) {
        if (carViewHotkeyReleased[player]) {
            view++;
            carViewHotkeyReleased[player] = 0;
            if (viewExtraViewsUnlocked) {
                if (view >= 5)
                    view = 2;
            } else if (view >= 8)
                view = 7;
            carSetViewMode(car, view);
        }
    } else if (car->key_0DA) {
        if (carViewHotkeyReleased[player]) {
            carViewHotkeyReleased[player] = 0;
            hudToggleRadar(HOTKEY_HUD);
        }
    } else if (car->key_0DB) {
        if (carViewHotkeyReleased[player]) {
            carViewHotkeyReleased[player] = 0;
            hudToggleWeaponList(HOTKEY_HUD);
        }
    } else if (car->key_0E6) {
        if (carViewHotkeyReleased[player])
            carViewHotkeyReleased[player] = 0;
    } else if (car->key_rear_view) {
        if (carViewHotkeyReleased[player]) {
            rtToggleRearView();
            carViewHotkeyReleased[player] = 0;
        }
    } else
        carViewHotkeyReleased[player] = 1;

    if (!viewExtraViewsUnlocked) {
#ifdef TM2_PORT_HUMANS
        if ((u8)portHumanCombo(1, player))  /* H2: pad 0/1 by the slot (retail's call), or the input word */
#else
        if ((u8)padComboHeld(1, player))   /* retail tests the low byte */
#endif
            viewExtraViewsUnlocked = 1;
    } else if (view != old) {
        if (view == 4) {
            if (rtGetRearViewActive())
                rtToggleRearView();
            rtSetRearView(0);
        } else
            rtSetRearView(1);
    }
#undef HOTKEY_HUD
}

/* @8011C8E4: the human car nearest to pos (|dx| + |dy|), skipping one. */
s32 __attribute__((section(".text_8011C8E4"))) humanCarNearest(s32 *pos, s32 exclude)
{
    s32 i;
    s32 nearest = TM2_HUMAN_NEAREST_FIRST;     /* request 50: not a vacant record (0 with no define) */
    s32 best = 0x7FFF;
    s32 d;
    for (i = 0; i < humanCarTotal; i++) {
        TM2_HUMAN_SKIP_VACANT(i);   /* request 50: a vacant record is never a target (nothing with no define) */
        d = __builtin_abs(pos[0] - humanCars[i].pos[0]) + __builtin_abs(pos[1] - humanCars[i].pos[1]);
        if (d < best && i != exclude) {
            nearest = i;
            best = d;
        }
    }
    return nearest;
}

/* @8011C98C: the living AI car nearest to pos (|dx| + |dy|). */
s32 __attribute__((section(".text_8011C98C"))) aiCarNearestAlive(s32 *pos)
{
    s32 i;
    s32 nearest = 0;
    s32 best = 0x7FFF;
    s32 d;
    for (i = 0; i < aiCarTotal; i++) {
        if (aiCars[i].hp > 0) {
            d = __builtin_abs(pos[0] - aiCars[i].pos[0]) + __builtin_abs(pos[1] - aiCars[i].pos[1]);
            if (d < best) {
                nearest = i;
                best = d;
            }
        }
    }
    return nearest;
}

/* @8011CA40 */
void __attribute__((section(".text_8011CA40"))) humanCarGetPos(s32 i, s32 *pos)
{
    pos[0] = humanCars[i].pos[0];
    pos[1] = humanCars[i].pos[1];
    pos[2] = humanCars[i].pos[2];
}

/* @8011CA98 */
void __attribute__((section(".text_8011CA98"))) aiCarGetPos(s32 i, s32 *pos)
{
    pos[0] = aiCars[i].pos[0];
    pos[1] = aiCars[i].pos[1];
    pos[2] = aiCars[i].pos[2];
}

/* @8011CBB0 */
void __attribute__((section(".text_8011CBB0"))) hudResetSpeed(short player)
{
    hudSmoothedSpeeds[player] = 0;
}

/* @8011CBD0: smooth a player's displayed speed toward the current one. */
void __attribute__((section(".text_8011CBD0"))) hudSmoothSpeed(short player)
{
    s32 frame = timerGetFrameTicks();
    s32 speed = humanCars[player].speed / 32 * 100 / ((short)frame * 19);
    s32 diff = speed - hudSmoothedSpeeds[player];
    /* a ternary, not if (diff < 0): retail leaves the bgez delay slot empty */
    diff = diff >= 0 ? diff : -diff;
    if (diff >= 16)
        hudSmoothedSpeeds[player] = mathBlendValue(hudSmoothedSpeeds[player], speed, 70);
    else if (diff >= 11)
        hudSmoothedSpeeds[player] = mathBlendValue(hudSmoothedSpeeds[player], speed, 80);
    else if (diff >= 6)
        hudSmoothedSpeeds[player] = mathBlendValue(hudSmoothedSpeeds[player], speed, 90);
    else
        hudSmoothedSpeeds[player] = mathBlendValue(hudSmoothedSpeeds[player], speed, 95);
}

/* @8011CD08: a human car's speed; in view 7 the watched AI car's speed. */
s32 __attribute__((section(".text_8011CD08"))) carGetDisplaySpeed(s32 i)
{
    s32 frame;
    if (humanCars[i].view == 7) {
        frame = timerGetFrameTicks();
        return aiCars[viewWatchedAiCar].speed / 32 * 100 / ((short)frame * 19);
    }
    frame = timerGetFrameTicks();
    return humanCars[i].speed / 32 * 100 / ((short)frame * 19);
}

/* @8011CE14: an AI car's speed. */
s32 __attribute__((section(".text_8011CE14"))) aiCarGetDisplaySpeed(s32 i)
{
    s32 frame = timerGetFrameTicks();
    return aiCars[i].speed / 32 * 100 / ((short)frame * 19);
}

/* @8011CED0 */
s32 __attribute__((section(".text_8011CED0"))) carObjectCount(void)
{
    return carWorldObjectCount;
}

/* @8011CEDC */
s32 __attribute__((section(".text_8011CEDC"))) humanCarCount(void)
{
    return humanCarTotal;
}

/* @8011CEE8 */
Human *__attribute__((section(".text_8011CEE8"))) humanCarGet(s32 i)
{
    return &humanCars[i];
}

/* @8011CF0C: the car an AI car is after: an AI car, or (-1) the human
 * nearest to it. */
void *__attribute__((section(".text_8011CF0C"))) aiGetTargetCar(s32 i, u8 *human)
{
    void *car;
    if (aiCarTargets[i] == -1) {
        *human = 1;
        car = &humanCars[humanCarNearest(aiCars[i].pos, -1)];
    } else {
        *human = 0;
        car = &aiCars[aiCarTargets[i]];
    }
    return car;
}

/* @8011CFD0: the record of car object slot i. */
void *__attribute__((section(".text_8011CFD0"))) carFromSlot(s32 i, u8 *human)
{
    s32 index = carSlotIndices[i];
    void *car = 0;
    if (index >= 0) {
        if (carSlotKinds[i] == 0) {
            car = &humanCars[index];
            *human = 1;
        } else if (carSlotKinds[i] == 1) {
            car = &aiCars[index];
            *human = 0;
        }
    }
    return car;
}

/* @8011D07C: the human car with an id (the first one if none). */
Human *__attribute__((section(".text_8011D07C"))) humanCarFindById(s32 id)
{
    s32 i;
    for (i = 0; i < humanCarTotal; i++)
        if (humanCars[i].id == id)
            return &humanCars[i];
    return humanCars;
}

/* @8011D0E0 */
s32 __attribute__((section(".text_8011D0E0"))) aiCarCount(void)
{
    return aiCarTotal;
}

/* @8011D0EC */
AICar *__attribute__((section(".text_8011D0EC"))) aiCarGet(u32 i)
{
    if (i >= TM2_MAX_AI_CARS)
        i = 0;
    return &aiCars[i];
}

/* @8011D128 */
AICar *__attribute__((section(".text_8011D128"))) aiGetWatchedCar(void)
{
    return &aiCars[viewWatchedAiCar];
}

/* @8011D15C: a car's object from its owner key. */
Object *__attribute__((section(".text_8011D15C"))) carObjectFromOwner(void *owner)
{
    s32 i;
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): owner is a car key (spin_target, crush_owner, ...) */
    for (i = 0; i < carWorldObjectCount; i++)
        if (portCarGroups.slot_key[i] == (s32)owner)
            return carSlotObjects[i];
#else
    for (i = 0; i < carWorldObjectCount; i++)
        if (carSlotOwners[i] == owner)
            return carSlotObjects[i];
#endif
    return carSlotObjects[i];
}

/* @8011D1CC: find a car object; reports its kind and index. */
u8 __attribute__((section(".text_8011D1CC"))) carFindByObject(Object *object, u8 *human, s32 *index)
{
    u8 found = 0;
    s32 i;
    for (i = 0; i < carWorldObjectCount && !found; i++) {
        if (carSlotObjects[i] == object) {
            if (carSlotKinds[i] == 0) {
                *human = 1;
                found = 1;
                *index = carSlotIndices[i];
            } else if (carSlotKinds[i] == 1) {
                *human = 0;
                found = 1;
                *index = carSlotIndices[i];
            }
        }
    }
    return found;
}

/* @8011D280: the object id of a human (human != 0) or AI car. */
s32 __attribute__((section(".text_8011D280"))) carObjectId(s32 i, u8 human)
{
    Object *object;
    if (human)
        object = carSlotObjects[humanCarObjectSlots[i]];
    else
        object = carSlotObjects[aiCarObjectSlots[i]];
    return object->id;
}

/* @8011D2C8: find a car object by its id; reports its kind and index. */
u8 __attribute__((section(".text_8011D2C8"))) carFindById(s32 id, u8 *human, s32 *index)
{
    u8 found = 0;
    s32 i;
    for (i = 0; i < carWorldObjectCount && !found; i++) {
        if (carSlotObjects[i]->id == id) {
            if (carSlotKinds[i] == 0) {
                *human = 1;
                found = 1;
            } else if (carSlotKinds[i] == 1) {
                *human = 0;
                found = 1;
            }
            *index = carSlotIndices[i];
        }
    }
    return found;
}

/* @8011D388: place a camera object. (Plain viewCameraObjectTable[i] indexing, not a
 * pointer to the slot, gives retail's base-before-index address order.) */
void __attribute__((section(".text_8011D388"))) viewSetCameraPose(s32 i, s32 *pos, s16 *rot)
{
    if (viewCameraObjectTable[i]) {
        viewCameraObjectTable[i]->pos[0] = pos[0];
        viewCameraObjectTable[i]->pos[1] = pos[1];
        viewCameraObjectTable[i]->pos[2] = pos[2];
        viewCameraObjectTable[i]->rot[0] = rot[0];
        viewCameraObjectTable[i]->rot[1] = rot[1];
        viewCameraObjectTable[i]->rot[2] = rot[2];
        RotMatrixYXZ(viewCameraObjectTable[i]->rot, viewCameraObjectTable[i]->matrix);
    }
}

/* @8011D438: read a camera object's placement. */
void __attribute__((section(".text_8011D438"))) viewGetCameraPose(s32 i, s32 *pos, s16 *rot)
{
    pos[0] = viewCameraObjectTable[i]->pos[0];
    pos[1] = viewCameraObjectTable[i]->pos[1];
    pos[2] = viewCameraObjectTable[i]->pos[2];
    rot[0] = viewCameraObjectTable[i]->rot[0];
    rot[1] = viewCameraObjectTable[i]->rot[1];
    rot[2] = viewCameraObjectTable[i]->rot[2];
    RotMatrixYXZ(viewCameraObjectTable[i]->rot, viewCameraObjectTable[i]->matrix);
}

/* @8011D508 */
u8 __attribute__((section(".text_8011D508"))) arenaGetShortChaseRange(void)
{
    return arenaShortChaseRange;
}

/* @8011D528: apply the difficulty to every AI car. */
void __attribute__((section(".text_8011D528"))) aiApplyDifficulty(void)
{
    s32 value;
    s32 i;
    switch (aiDifficulty) {
    case 1: value = 120; break;
    case 0: value = 75; break;
    case 2: value = 140; break;
    default: value = 120; break;
    }
    for (i = 0; i < aiCarTotal; i++)
        carStatsSetSpeedMult(aiCars[i].heat, value);
}

/* @8011D5D0 */
void __attribute__((section(".text_8011D5D0"))) aiSetDifficulty(s32 difficulty)
{
    aiDifficulty = difficulty;
    aiApplyDifficulty();
}

/* @8011D5F4 */
s32 __attribute__((section(".text_8011D5F4"))) aiGetDifficulty(void)
{
    return aiDifficulty;
}

/* @8011D600 */
void __attribute__((section(".text_8011D600"))) PadSetConfig(s32 config, short player)
{
    Human *car = humanCarGet(player);
    padConfigs[player] = config;
    carApplyPadConfig(car, config);
}

/* @8011D660 */
s32 __attribute__((section(".text_8011D660"))) PadGetConfig(short player)
{
    humanCarGet(player);
    return padConfigs[player];
}

#define BUTTON(car, offset) ((car)->buttons[(offset) - 0xF8])

/* @8011D6A4: set a car's action -> button map for one of the four pad
 * configurations (unknown configurations use configuration 1). */
void __attribute__((section(".text_8011D6A4"))) carApplyPadConfig(Human *car, s32 config)
{
    BUTTON(car, 0x109) = 12;
    switch (config) {
    case 0:
        car->pad_config = 0;
        BUTTON(car, 0xF8) = 5;
        BUTTON(car, 0xFA) = 6;
        BUTTON(car, 0xFC) = 8;
        BUTTON(car, 0xFB) = 7;
        BUTTON(car, 0x107) = 2;
        BUTTON(car, 0x108) = 1;
        BUTTON(car, 0xFD) = 12;
        BUTTON(car, 0xF9) = 4;
        BUTTON(car, 0x100) = 10;
        BUTTON(car, 0x101) = 10;
        break;
    default:
    case 1:
        car->pad_config = 1;
        BUTTON(car, 0xF8) = 5;
        BUTTON(car, 0xFA) = 6;
        BUTTON(car, 0xFC) = 8;
        BUTTON(car, 0xFB) = 7;
        BUTTON(car, 0x107) = 2;
        BUTTON(car, 0x108) = 1;
        BUTTON(car, 0xFD) = 13;
        BUTTON(car, 0xFE) = 11;
        BUTTON(car, 0xF9) = 0;
        BUTTON(car, 0xFF) = 14;
        BUTTON(car, 0x102) = 9;
        BUTTON(car, 0x103) = 3;
        BUTTON(car, 0x104) = 4;
        BUTTON(car, 0x105) = 10;
        BUTTON(car, 0x100) = 12;
        BUTTON(car, 0x101) = 12;
        break;
    case 2:
        car->pad_config = 2;
        BUTTON(car, 0xF8) = 5;
        BUTTON(car, 0xFA) = 6;
        BUTTON(car, 0xFC) = 8;
        BUTTON(car, 0xFB) = 7;
        BUTTON(car, 0x107) = 2;
        BUTTON(car, 0x108) = 1;
        BUTTON(car, 0xFD) = 12;
        BUTTON(car, 0xF9) = 4;
        BUTTON(car, 0x100) = 10;
        BUTTON(car, 0x101) = 10;
        break;
    case 3:
        car->pad_config = 3;
        BUTTON(car, 0xF8) = 5;
        BUTTON(car, 0xFA) = 6;
        BUTTON(car, 0xFC) = 8;
        BUTTON(car, 0xFB) = 7;
        BUTTON(car, 0x107) = 2;
        BUTTON(car, 0x108) = 1;
        BUTTON(car, 0xFD) = 10;
        BUTTON(car, 0xF9) = 9;
        BUTTON(car, 0xFF) = 0;
        BUTTON(car, 0x100) = 4;
        BUTTON(car, 0x101) = 4;
        break;
    }
}
