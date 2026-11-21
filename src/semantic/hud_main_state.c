union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
union TM2HudRecord { unsigned int words[475]; unsigned short halves[950]; short signed_halves[950]; unsigned char bytes[1900]; };
struct TM2HudText30 { union TM2FontDrawMode mode; union TM2FontSprite sprites[30]; };
struct TM2HudText10 { union TM2FontDrawMode mode; union TM2FontSprite sprites[10]; };
typedef char hud_record_size[(sizeof(union TM2HudRecord)==1900)?1:-1];
typedef char hud_text30_size[(sizeof(struct TM2HudText30)==612)?1:-1];
typedef char hud_text10_size[(sizeof(struct TM2HudText10)==212)?1:-1];

char strEnemiesToasted[17] __attribute__((section(".tm2_800CEE58"))) = "Enemies Toasted!";
char strEnemies1[11] __attribute__((section(".tm2_800CEE4C"))) = "Enemies: 1";
char strEnemies2[11] __attribute__((section(".tm2_800CEE40"))) = "Enemies: 2";
char strEnemies3[11] __attribute__((section(".tm2_800CEE34"))) = "Enemies: 3";
char strEnemies4[11] __attribute__((section(".tm2_800CEE28"))) = "Enemies: 4";
char strEnemies5[11] __attribute__((section(".tm2_800CEE1C"))) = "Enemies: 5";
char strEnemies6[11] __attribute__((section(".tm2_800CEE10"))) = "Enemies: 6";
char strEnemies7[11] __attribute__((section(".tm2_800CEE04"))) = "Enemies: 7";
char strEnemies8[11] __attribute__((section(".tm2_800CEDF8"))) = "Enemies: 8";
char strEnemies9[11] __attribute__((section(".tm2_800CEDEC"))) = "Enemies: 9";
char strEnemies10[12] __attribute__((section(".tm2_800CEDE0"))) = "Enemies: 10";
char strEnemies11[12] __attribute__((section(".tm2_800CEDD4"))) = "Enemies: 11";
char strEnemies12[12] __attribute__((section(".tm2_800CEDC8"))) = "Enemies: 12";
char strMissilesLoaded[16] __attribute__((section(".tm2_800CEDB8"))) = "Missiles Loaded";
char strHomersLoaded[14] __attribute__((section(".tm2_800CEDA8"))) = "Homers Loaded";
char strPowersLoaded[14] __attribute__((section(".tm2_800CED98"))) = "Powers Loaded";
char strRemotesLoaded[15] __attribute__((section(".tm2_800CED88"))) = "Remotes Loaded";
char strNapalmLoaded[14] __attribute__((section(".tm2_800CED78"))) = "Napalm Loaded";
char strRicochetsLoaded[17] __attribute__((section(".tm2_800CED64"))) = "Ricochets Loaded";
char strHealthRecharged[17] __attribute__((section(".tm2_800CED50"))) = "Health Recharged";
char strLightningLoaded[17] __attribute__((section(".tm2_800CED3C"))) = "Lightning Loaded";
char strTurboLoaded[13] __attribute__((section(".tm2_800CED2C"))) = "Turbo Loaded";
char strSecretCombo[13] __attribute__((section(".tm2_800CED1C"))) = "Secret Combo";
char strGodMode[9] __attribute__((section(".tm2_800CED10"))) = "God Mode";
char strMegaGuns[10] __attribute__((section(".tm2_800CED04"))) = "Mega Guns";
char strEternalLife[13] __attribute__((section(".tm2_800CECF4"))) = "Eternal Life";
char strInfiniteWeapons[17] __attribute__((section(".tm2_800CECE0"))) = "Infinite Weapons";
char strGodModeOff[13] __attribute__((section(".tm2_800CECD0"))) = "God Mode Off";
char strMegaGunsOff[14] __attribute__((section(".tm2_800CECC0"))) = "Mega Guns Off";
char strEternalLifeOff[17] __attribute__((section(".tm2_800CECAC"))) = "Eternal Life Off";
char strShootEmLoseEm[19] __attribute__((section(".tm2_800CEC98"))) = "Shoot 'em Lose 'em";
char strHomingNapalm[14] __attribute__((section(".tm2_800CEC88"))) = "Homing Napalm";
char strMinionSpecial[15] __attribute__((section(".tm2_800CEC78"))) = "Minion Special";
char strSellYourSoul[15] __attribute__((section(".tm2_800CEC68"))) = "Sell Your Soul";
char strDementedBonus[15] __attribute__((section(".tm2_800CEC58"))) = "Demented Bonus";
char strTrainRideBonus[17] __attribute__((section(".tm2_800CEC44"))) = "Train Ride Bonus";
char strWeaponBayFull[17] __attribute__((section(".tm2_800CEC30"))) = "Weapon Bay Full!";
char strLivesLabel[9] __attribute__((section(".tm2_800CEE6C"))) = "Lives:  ";
/* @8016286C: 37 retail message pointers, each to real C string storage. */
char *hudMessageStrs[37] __attribute__((section(".tm2_8016286C"))) = {
    strEnemiesToasted,
    strEnemies1,
    strEnemies2,
    strEnemies3,
    strEnemies4,
    strEnemies5,
    strEnemies6,
    strEnemies7,
    strEnemies8,
    strEnemies9,
    strEnemies10,
    strEnemies11,
    strEnemies12,
    strMissilesLoaded,
    strHomersLoaded,
    strPowersLoaded,
    strRemotesLoaded,
    strNapalmLoaded,
    strRicochetsLoaded,
    strHealthRecharged,
    strLightningLoaded,
    strTurboLoaded,
    strSecretCombo,
    strGodMode,
    strMegaGuns,
    strEternalLife,
    strInfiniteWeapons,
    strGodModeOff,
    strMegaGunsOff,
    strEternalLifeOff,
    strShootEmLoseEm,
    strHomingNapalm,
    strMinionSpecial,
    strSellYourSoul,
    strDementedBonus,
    strTrainRideBonus,
    strWeaponBayFull
};
char *hudLivesText __attribute__((section(".tm2_80180AC4"))) = strLivesLabel;
/* Four-byte backing views preserve the inter-symbol extents; reviewed HUD
 * player indices are 0 and 1. No original array declaration is asserted. */
unsigned char hudRadarVisible[4] __attribute__((section(".tm2_80180A6C"))) = {1,1,0,0};
unsigned char hudWeaponListVisible[4] __attribute__((section(".tm2_80180A70"))) = {0,0,0,0};
unsigned int *hudOrderingTable __attribute__((section(".tm2_80180F0C"))) = 0;
/* CRT-zeroed caller-owned packet/OT backing. Two players; two OT buffers. */
struct TM2HudText30 hudMessageTextMode __attribute__((section(".tm2_80181E28"))) = {0};
struct TM2HudText10 hudLivesTextMode __attribute__((section(".tm2_8018208C"))) = {0};
unsigned int hudOrderTables[2][2][2] __attribute__((section(".tm2_801828D0"))) = {{{0}}};
union TM2HudRecord hudPlayerRecords[2] __attribute__((section(".tm2_801828F0"))) = {{{0}}};
