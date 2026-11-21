/* 800D1DB0. Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* Registry globals (gp-relative). */
extern s32 worldLevelModelSet;      /* gp+28: one-shot flag for type 1 */
extern u8 *worldLoadParentObject;      /* last object registered by a "parent" type */
extern s32 worldLoadPickupIndex;      /* running index for the 4xx objects */
extern s32 worldLoadTargetIndex;      /* running index for the 5xx objects */

extern void sceneDeliverResource(u8 *);
extern s32 *dbInitCsForAModel(u8 *);
#include "semantic/world.h"
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"
#endif
extern void csSetModel(void *, u8 *);
extern s32 viewGetSplitMode(void);
extern void viewSetFollowObj(void *, s32);
extern void carRegisterModel(void *, s32);
extern void weaponAssetBind(u8 *, s32);
extern void carRegisterTireSwitch(u8 *);
extern void carPartRegisterDarkTooth(u8 *);
extern void carPartRegisterHammerhead(u8 *);
extern void carPartRegisterAxel(u8 *);
extern void carPartRegisterMrSlam(u8 *);
extern void carPartRegisterMinion(u8 *);
extern void carBodyObjectRegister(s32, u8 *);
extern void carWreckModelRegister(s32, u8 *);
extern void carFreezeBuild(s32, u8 *);
extern void clutAnimRegister(u8 *, s32);
extern void objRegistryAdd(u8 *, s32);
extern void targetRegister(s32, s32, s32 *);
extern void targetTimedEventInit(u8 *);
extern void targetLevelEventInit(void);
extern void targetPlayerEventInit(u8 *, s32);
extern void worldTrackVehicleAttach(void *, s32);
extern void targetActorSpawn(s32, s16, u8 *);
extern void weaponStoreProjectileModel(u8 *, s32);
extern void worldWarpObjectsCreate(u8 *);
extern void UAdashInitDashboardIcons(u8 *);
extern void hudRadarArrowsLoad(s32, u8 *);
extern s32 gameGetMode(void);
extern void pickupRegister(s32 *, s32, s32);
extern void pickupRegisterObject(u8 *, s32);
extern void explodeStoreFlameAnimation(u8 *);
extern void explodeStoreExplosionAnimation(u8 *);
extern void explodeStoreContrailAnimation(u8 *);
extern void explodeStoreSmokeAAnimation(u8 *);
extern void explodeStoreSmokeBAnimation(u8 *);
extern void explodeStoreSmokeCAnimation(u8 *);
extern void explodeStoreBulletImpactAnimation(u8 *);
extern void explodeStoreSparkAnimation(u8 *);
extern void explodeStoreLightningAnimation(u8 *);
extern void fxRegisterBlastObject(u8 *);
extern void worldLoadStub688(u8 *);
extern void fxSetPlatformSoundObject(void *);
extern u32 fxInstallWakeObject(void *, s32);
extern void fxRegisterSplashObject(u8 *);
extern void fxRegisterLavaSplashObject(u8 *);
extern void renderRemoveWorldModel(u8 *);
extern void renderAddWorldModel(u8 *);
extern int explodeStoreExplosionBAnimation(u8 *);
extern void fxSetTargetHitObject(void *);
extern void explodeStoreDebrisAnimation(s32, u8 *);
extern void moscowEventRegisterObject(u8 *, s32 *, s32);
extern void laEventRegisterObject(u8 *, s32 *, s32);
extern void moscowDishSetEmitterPos(s32 *, s32);
extern void moscowDishRegister(u8 *, s32 *);
extern void worldFlameMarkerSetPos(s32 *, s32);
extern void slamArmRegister(u8 *);
extern void slamClawRegister(u8 *);
extern void carFreezeSetTemplate(u8 *);
extern void worldCollapseRegister(u8 *);
extern void eiffelTowerRegister(u8 *);
extern u32 *eiffelTowerSetEmitter(u8 *, s32);
extern void worldLoadStub3503(void *, s16);

/* @800D1DB0: per-object loader callback for dbscanHierarchy (used by the start-parameter
 * loader 800D1C70 and the PSXD asset installer 80120EF4). Dispatches on the object
 * type id: ids >= 9000 go to sceneDeliverResource; the rest are a big switch that hands the
 * object to the subsystem owning that type (models via dbInitCsForAModel, the object
 * registry 80123F78, explosion/contrail animation stores, dashboard icons, 4xx objects
 * numbered from worldLoadPickupIndex, 5xx objects numbered from worldLoadTargetIndex, ...). Types
 * 16,32,...,224 (and 160/249) also remember the object in worldLoadParentObject, which the
 * 23,39,...,247 types then receive. */
void __attribute__((section(".text_800D1DB0"))) worldLoadObject(s32 id, u8 *obj, s32 *ctx)
{
    s32 *p;
    s32 old;
    s32 i;

    if (id >= 9000) {
        sceneDeliverResource(obj);
        return;
    }
    switch (id) {
    case 3:
        p = dbInitCsForAModel(obj);
        p[0] = 0xffe;
        p[3] = 0;
        break;
    case 1:
        if (worldLevelModelSet == 0) {
            csSetModel((void *)csGetRoot(), obj);
            worldLevelModelSet = 1;
        }
        break;
    case 2:
        p = csAlloc();
        csSetModel(p, obj);
        viewSetFollowObj(p, viewGetSplitMode());
        p[3] = 0;
        break;
    case 16:  carRegisterModel(dbInitCsForAModel(obj), 16);  worldLoadParentObject = obj; break;
    case 32:  carRegisterModel(dbInitCsForAModel(obj), 32);  worldLoadParentObject = obj; break;
    case 48:  carRegisterModel(dbInitCsForAModel(obj), 48);  worldLoadParentObject = obj; break;
    case 64:  carRegisterModel(dbInitCsForAModel(obj), 64);  worldLoadParentObject = obj; break;
    case 80:  carRegisterModel(dbInitCsForAModel(obj), 80);  worldLoadParentObject = obj; break;
    case 96:  carRegisterModel(dbInitCsForAModel(obj), 96);  worldLoadParentObject = obj; break;
    case 112: carRegisterModel(dbInitCsForAModel(obj), 112); worldLoadParentObject = obj; break;
    case 128: carRegisterModel(dbInitCsForAModel(obj), 128); worldLoadParentObject = obj; break;
    case 144: carRegisterModel(dbInitCsForAModel(obj), 144); worldLoadParentObject = obj; break;
    case 160:
        carRegisterModel(dbInitCsForAModel(obj), 160);
        weaponAssetBind(obj, 160);
        worldLoadParentObject = obj;
        break;
    case 176: carRegisterModel(dbInitCsForAModel(obj), 176); worldLoadParentObject = obj; break;
    case 192: carRegisterModel(dbInitCsForAModel(obj), 192); worldLoadParentObject = obj; break;
    case 208: carRegisterModel(dbInitCsForAModel(obj), 208); worldLoadParentObject = obj; break;
    case 224: carRegisterModel(dbInitCsForAModel(obj), 224); worldLoadParentObject = obj; break;
    case 240: carRegisterModel(dbInitCsForAModel(obj), 240); break;
    case 18: case 50: case 66: case 82: case 98: case 114: case 130: case 146:
    case 162: case 210:
        carRegisterTireSwitch(obj);
        break;
    case 242: carPartRegisterDarkTooth(obj); break;
    case 34:  carPartRegisterHammerhead(obj); break;
    case 178: carPartRegisterAxel(obj); break;
    case 194: carPartRegisterMrSlam(obj); break;
    case 226: carPartRegisterMinion(obj); break;
    case 19: case 35: case 51: case 67: case 83: case 99: case 115: case 131:
    case 147: case 163: case 179: case 195: case 211: case 227: case 243:
        carBodyObjectRegister(id, obj);
        break;
    case 21: case 37: case 53: case 69: case 85: case 101: case 117: case 133:
    case 149: case 165: case 181: case 197: case 213: case 229: case 245:
        carWreckModelRegister(id, obj);
        break;
    case 23: case 39: case 55: case 71: case 87: case 103: case 119: case 135:
    case 151: case 167: case 183: case 199: case 215: case 231: case 247:
        carFreezeBuild(id, worldLoadParentObject);
        break;
    case 24: case 40: case 56: case 72: case 88: case 104: case 120: case 136:
    case 152: case 168: case 184: case 200: case 216: case 232: case 248:
        clutAnimRegister(obj, 2);
        break;
    case 249:
        weaponAssetBind(obj, 249);
        worldLoadParentObject = obj;
        break;
    case 500 ... 510:
    case 519 ... 520:
        old = *(u16 *)(obj + 4);
        *(u16 *)(obj + 4) = worldLoadTargetIndex;
        objRegistryAdd(obj, id);
        targetRegister(id, *(u16 *)(obj + 4), ctx);
        worldLoadTargetIndex++;
        if (id == 508)
            targetTimedEventInit(obj);
        if (id == 509)
            targetLevelEventInit();
        if (id == 510)
            targetPlayerEventInit(obj, old);
        if (id == 506)
            worldTrackVehicleAttach(dbInitCsForAModel(obj), 0);
        if (id == 507)
            worldTrackVehicleAttach(dbInitCsForAModel(obj), 1);
        break;
    case 514:
    case 516 ... 517:
        objRegistryAdd(obj, id);
        targetActorSpawn(id, *(u16 *)(obj + 4), obj);
        break;
    case 2201 ... 2202: case 2208: case 2301: case 2321 ... 2322: case 2326:
        weaponStoreProjectileModel(obj, id);
        break;
    case 2209 ... 2210: case 2212: case 2214: case 2217: case 2323:
        weaponAssetBind(obj, id);
        break;
    case 2325:
        weaponAssetBind(obj, 2325);
        objRegistryAdd(obj, 2325);
        break;
    case 2324:
        objRegistryAdd(obj, 2324);
        break;
    case 693:  worldWarpObjectsCreate(obj); break;
#ifdef TM2_PORT_VISUAL
    case 1260:
        UAdashInitDashboardIcons(obj);
        portPickupSpecial.hud = obj;    /* request 79 (PICKUP_LOOKS.md 10): the special's pickup look's icons */
        break;
#else
    case 1260: UAdashInitDashboardIcons(obj); break;
#endif
    case 1290 ... 1293: hudRadarArrowsLoad(id, obj); break;
    case 401: case 403 ... 404: case 406: case 411: case 413: case 417 ... 419: case 430:
        if (*obj == 1) {
            if (gameGetMode() == 4 && id == 418) {
                obj[20] = 0;
                break;
            }
            *(u16 *)(obj + 22) = worldLoadPickupIndex;
            pickupRegister(ctx, id, worldLoadPickupIndex);
        } else if (*obj == 4) {
            if (gameGetMode() == 4 && id == 418) {
                obj[18] = 0;
                break;
            }
            *(u16 *)(obj + 16) = worldLoadPickupIndex;
            pickupRegister(ctx, id, worldLoadPickupIndex);
        } else {
            break;
        }
        worldLoadPickupIndex++;
        pickupRegisterObject(obj, id);
        break;
    case 650: explodeStoreFlameAnimation(obj); break;
    case 654: explodeStoreExplosionAnimation(obj); break;
    case 660: explodeStoreSmokeAAnimation(obj); break;
    case 661: explodeStoreSmokeBAnimation(obj); break;
    case 662: explodeStoreSmokeCAnimation(obj); break;
    case 656: explodeStoreBulletImpactAnimation(obj); break;
    case 653: explodeStoreSparkAnimation(obj); break;
    case 652: explodeStoreContrailAnimation(obj); break;
    case 658: explodeStoreLightningAnimation(obj); break;
    case 685: fxRegisterBlastObject(obj); break;
    case 686:
        objRegistryAdd(obj, 686);
        renderRemoveWorldModel(obj);
        renderAddWorldModel(obj);
        break;
    case 688: worldLoadStub688(obj); break;
    case 689:
        objRegistryAdd(obj, 689);
        renderRemoveWorldModel(obj);
        renderAddWorldModel(obj);
        break;
    case 3103:
        fxSetPlatformSoundObject(dbInitCsForAModel(obj));
        objRegistryAdd(obj, 3103);
        renderRemoveWorldModel(obj);
        renderAddWorldModel(obj);
        break;
    case 681:
        for (i = 0; i < 5; i++)
            fxInstallWakeObject(dbInitCsForAModel(obj), i);
        break;
    case 683: fxRegisterSplashObject(obj); break;
    case 684:
        objRegistryAdd(obj, 684);
        renderRemoveWorldModel(obj);
        renderAddWorldModel(obj);
        break;
    case 690: fxRegisterLavaSplashObject(obj); break;
    case 691:
        objRegistryAdd(obj, 691);
        renderRemoveWorldModel(obj);
        renderAddWorldModel(obj);
        break;
    case 687: explodeStoreExplosionBAnimation(obj); break;
    case 669:
        fxSetTargetHitObject(dbInitCsForAModel(obj));
        objRegistryAdd(obj, 669);
        break;
    case 670 ... 673: explodeStoreDebrisAnimation(id, obj); break;
    case 4000: objRegistryAdd(obj, 4000); break;
    case 4001 ... 4004: moscowEventRegisterObject(obj, ctx, id - 4000); break;
    case 20: case 22: case 36: case 38: case 52: case 54: case 68: case 70:
    case 84: case 86: case 100: case 102: case 116: case 118: case 132: case 134:
    case 148: case 150: case 164: case 166: case 180: case 182: case 196: case 198:
    case 212: case 214: case 228: case 230: case 244: case 246: case 4005:
        objRegistryAdd(obj, id);
        break;
    case 4006 ... 4009: laEventRegisterObject(obj, ctx, id - 4005); break;
    case 3003:
        objRegistryAdd(obj, 3003);
        moscowDishSetEmitterPos(ctx, *(u16 *)(obj + 4));
        break;
    case 3004: objRegistryAdd(obj, 3004); break;
    case 3005: objRegistryAdd(obj, 3005); break;
    case 3006: moscowDishRegister(obj, ctx); break;
    case 5000: clutAnimRegister(obj, 0); break;
    case 5001: clutAnimRegister(obj, 3); break;
    case 5100:
        clutAnimRegister(obj, 1);
        if (*obj == 0)
            worldFlameMarkerSetPos(ctx, *(u16 *)(obj + 20));
        break;
    case 201: slamArmRegister(obj); break;
    case 202: slamClawRegister(obj); break;
    case 692: carFreezeSetTemplate(obj); break;
    case 3100: worldCollapseRegister(obj); break;
    case 3101: objRegistryAdd(obj, 3101); break;
    case 3200: objRegistryAdd(obj, 3200); break;
    case 3201: eiffelTowerRegister(obj); break;
    case 3202: eiffelTowerSetEmitter(obj, 0); break;
    case 3203: eiffelTowerSetEmitter(obj, 1); break;
    case 3204: eiffelTowerSetEmitter(obj, 2); break;
    case 3503 ... 3504:
        worldLoadStub3503(dbInitCsForAModel(obj), id - 3503);
        break;
    case 1300:
    case 3102:
        break;
    }
}
