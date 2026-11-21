/* One-line accessors of game globals (and a few struct fields) spread over the
 * game code: getters return the variable, setters store their argument or a
 * constant. Generated from the retail code; names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;

extern u8 cdLastCom;
extern s32 worldLevelModelSet;
extern s32 musicVolume;
extern u8 soundDeniedPending;
extern u8 audioDarkToothMusicFlag;
extern u8 soundLoop32ActiveP1;
extern u8 soundLoop32ActiveP2;
extern u8 soundTurboLoopActiveP1;
extern u8 soundTurboLoopActiveP2;
extern u8 soundGunLoopActiveP1;
extern u8 soundGunLoopActiveP2;
extern u8 soundSharedLoopActive;
extern s32 worldLoadedArena;
extern s32 D_80180A28;
extern s32 D_80180A2C;
extern s32 fxContrailCellTable;
extern s32 darkToothBossObject;
extern s32 slamArmAngle;
extern s32 slamClawAngle;
extern s32 fxPlatformSoundObject;
extern s32 roundOverFlag;
extern s32 viewMainPlayer;
extern s32 viewWatchedAiCar;
extern u8 roundWonFlag;
extern u8 gameAttractDemoMode;
extern s32 gameMode;
extern s32 humanCarTotal;
extern s32 aiCarsRemaining;
extern s8 homingLeapState;
extern u8 aiCars;
/* Moscow dish event (weapon_fire_missiles.c TM2DishEvent): only the done flag. */
typedef struct { u8 pad_00[5]; s8 destroyed; u8 pad_06[0x30]; } TM2DishEvent;
extern TM2DishEvent moscowDishState;
#pragma tm2_absolute cdLastCom
#pragma tm2_absolute homingLeapState
#pragma tm2_absolute aiCars
#pragma tm2_absolute moscowDishState

void __attribute__((section(".text_800D3198"))) worldResetLevelModelFlag(void)
{
    worldLevelModelSet = 0;
}

void __attribute__((section(".text_800D3620"))) audioSetMusicVolume(s32 value)
{
    musicVolume = value;
}

s32 __attribute__((section(".text_800D362C"))) audioGetMusicVolume(void)
{
    return musicVolume;
}

u8 __attribute__((section(".text_800D3844"))) audioGetDarkToothMusic(void)
{
    return audioDarkToothMusicFlag;
}

void __attribute__((section(".text_800D3850"))) audioSetDarkToothMusic(void)
{
    audioDarkToothMusicFlag = 1;
}

void __attribute__((section(".text_800D3860"))) audioClearDarkToothMusic(void)
{
    audioDarkToothMusicFlag = 0;
}

u8 __attribute__((section(".text_800D386C"))) soundGetLoop32P1(void)
{
    return soundLoop32ActiveP1;
}

u8 __attribute__((section(".text_800D3878"))) soundGetLoop32P2(void)
{
    return soundLoop32ActiveP2;
}

u8 __attribute__((section(".text_800D3884"))) soundGetTurboLoopP1(void)
{
    return soundTurboLoopActiveP1;
}

u8 __attribute__((section(".text_800D3890"))) soundGetTurboLoopP2(void)
{
    return soundTurboLoopActiveP2;
}

s32 __attribute__((section(".text_800D389C"))) soundGetGunLoopP1(void)
{
    return soundGunLoopActiveP1;
}

s32 __attribute__((section(".text_800D38A8"))) soundGetGunLoopP2(void)
{
    return soundGunLoopActiveP2;
}

s32 __attribute__((section(".text_800D38B4"))) soundGetSharedLoop(void)
{
    return soundSharedLoopActive;
}

void __attribute__((section(".text_800D38C0"))) soundSetLoop32P1(u8 value)
{
    soundLoop32ActiveP1 = value;
}

void __attribute__((section(".text_800D38CC"))) soundSetLoop32P2(u8 value)
{
    soundLoop32ActiveP2 = value;
}

void __attribute__((section(".text_800D38D8"))) soundSetTurboLoopP1(u8 value)
{
    soundTurboLoopActiveP1 = value;
}

void __attribute__((section(".text_800D38E4"))) soundSetTurboLoopP2(u8 value)
{
    soundTurboLoopActiveP2 = value;
}

void __attribute__((section(".text_800D38F0"))) soundSetGunLoopP1(u8 value)
{
    soundGunLoopActiveP1 = value;
}

void __attribute__((section(".text_800D38FC"))) soundSetGunLoopP2(u8 value)
{
    soundGunLoopActiveP2 = value;
}

void __attribute__((section(".text_800D3908"))) soundSetSharedLoop(u8 value)
{
    soundSharedLoopActive = value;
}

void __attribute__((section(".text_800D3914"))) worldSetLoadedArena(s32 value)
{
    worldLoadedArena = value;
}

s32 __attribute__((section(".text_800D3920"))) worldGetLoadedArena(void)
{
    return worldLoadedArena;
}

void __attribute__((section(".text_800D392C"))) soundSetDeniedPending(u8 value)
{
    soundDeniedPending = value;
}

u8 __attribute__((section(".text_800D3938"))) soundGetDeniedPending(void)
{
    return soundDeniedPending;
}

s32 __attribute__((section(".text_800D3B1C"))) sub_800D3B1C(void)
{
    return D_80180A28;
}

s32 __attribute__((section(".text_800D3B28"))) sub_800D3B28(void)
{
    return D_80180A2C;
}

s32 __attribute__((section(".text_800D65EC"))) moscowDishIsDestroyed(void)
{
    return moscowDishState.destroyed;
}

void __attribute__((section(".text_800D6EF4"))) fxSetContrailCellTable(s32 value)
{
    fxContrailCellTable = value;
}

s32 __attribute__((section(".text_8010F9B0"))) weaponHomingLeapState(void)
{
    return homingLeapState;
}

s32 __attribute__((section(".text_8011090C"))) aiDarkToothBossObject(void)
{
    return darkToothBossObject;
}

s32 __attribute__((section(".text_80115218"))) slamArmGetAngle(void)
{
    return slamArmAngle;
}

s32 __attribute__((section(".text_80115224"))) slamClawGetAngle(void)
{
    return slamClawAngle;
}

void __attribute__((section(".text_80119090"))) fxSetPlatformSoundObject(s32 value)
{
    fxPlatformSoundObject = value;
}

void __attribute__((section(".text_8011A268"))) humanCarsSetCount(s32 value)
{
    humanCarTotal = value;
}

s32 __attribute__((section(".text_8011A390"))) aiCarsGetBase(void)
{
    return aiCars;
}

s32 __attribute__((section(".text_8011C23C"))) aiCarsGetRemaining(void)
{
    return aiCarsRemaining;
}

s32 __attribute__((section(".text_8011DAD4"))) viewGetMainPlayer(void)
{
    return viewMainPlayer;
}

s32 __attribute__((section(".text_8011DAE0"))) viewGetWatchedAiCar(void)
{
    return viewWatchedAiCar;
}

u8 __attribute__((section(".text_8011E214"))) roundIsWon(void)
{
    return roundWonFlag;
}

s32 __attribute__((section(".text_8011EDAC"))) gameGetMode(void)
{
    return gameMode;
}

s32 __attribute__((section(".text_8011EDB8"))) roundIsOver(void)
{
    return roundOverFlag;
}

u8 __attribute__((section(".text_8011EDC4"))) gameIsAttractDemo(void)
{
    return gameAttractDemoMode;
}

s32 __attribute__((section(".text_801395BC"))) sub_801395BC(u8 *p)
{
    return *(u16 *)(p + 0);
}

s32 __attribute__((section(".text_8013A580"))) sub_8013A580(void)
{
    return cdLastCom;
}
