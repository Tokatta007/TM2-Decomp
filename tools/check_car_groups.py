"""Static checks of the car-groups, more-cars, car-swap, texture-bank, per-car-specials, per-car-sound, per-car-CLUT,
drop-in and special-as port options (PC port request 32 steps 1-4, request 33 steps 1, 2a, 2e, 3 and "online with
bots", request 35; docs/CAR_GROUPS.md, docs/GUN_GAME_SPECIALS.md).

    py -3.14 tools/check_car_groups.py work/pv/P4 [--expect-functions] [--dmd] [--key-model]
    py -3.14 tools/check_car_groups.py --dmd --key-model --bank-model --clut-model

On a port variant link built with `--port-define TM2_PORT_CAR_GROUPS` (build_source_only.py or
build.py output directory):

1. No retail per-type table in changed code. Every relocation of the link whose target lies in
   one of retail's per-type car tables (docs/SURVIVAL.md 2.2: carBodyObjects, carWreckModels,
   carDamageStageObjects, carTireSwitchObjects, carPartsTireTurnAccum, carTurboObjects,
   D_8018F624, carFreezeObjects, the part sets and the spin angles) is looked up; the function
   that holds it must be one of ALLOWED:
     carPartsReset, objRegistriesReset   clear the retail tables each load (harmless, unused)
     carPartsSpin*                       dead under the define (carPartsAnimate calls portCarPartsSpin)
     objRegistryAdd, objRegistrySetStep  the +7 path (D_8018F624, never written) and turbo-like ids
                                         256..399, which are not cars and keep retail's table
   A relocation outside any function (a data word pointing at a table) fails too.
   Check 1 runs on a build with TM2_PORT_CAR_GROUPS, or with --negative-control.
2. With --expect-functions: the functions the port's defines changed (report.json
   port_changed_sections, code only) must equal the union of EXPECTED (TM2_PORT_CAR_GROUPS,
   CAR_GROUPS_PLAN.md 1.5-1.8) and EXPECTED_CARS (TM2_PORT_CARS, CAR_GROUPS.md "Step 2") for the
   defines in the build, unless other port defines are in it (then that union must be a subset).
   TM2_PORT_CAR_SWAP (step 3) adds no game function: its only game change is in sessionBindCars,
   which step 1 already changes.
5. On a build with TM2_PORT_CAR_SWAP: the swap's port functions (SWAP_PORT) are in .port_text
   (functions.tsv, kind port), sessionBindCars calls portCarBindGroupOf and portCarBindTypeOf, and no
   port function the swap reaches calls rand() itself (the game functions it calls are listed with the
   ones that can reach rand(): a spin-out when a held car is let go, as the game does it).

3. --dmd: the registration functions take a car's key from its load group, which the car root
   (ids 0x10..0xF0, carRegisterModel) fixes. Every car DMD on the disc (cdimage/, read-only, as
   tools/dmd_lod_dump.py) is walked root by root with the rules of dbscanHierarchy (8014ABF8:
   a node's callback runs when it is popped, after its children are pushed): the first callback
   of the file must be a car root, and every car id (16..255) must come after it.
4. --key-model: a host model of portCarRegisterRoot over load orders (all distinct, all of one
   type, mixed with Minion's two roots, 32 loaded cars): with no duplicate every key equals its
   type; every key index stays below TM2_CAR_KEYS (48).

6. On a build with TM2_PORT_TEXTURE_BANKS (step 4, CAR_GROUPS.md 9, TEXTURE_BANKS.md "As built"):
   - the bank functions (BANKS_PORT) are in .port_text;
   - every remap write sets the bank: in src/semantic every write of a primitive's tpage page bits
     (`((UVTpWord *)w)->tpage = ...`) is followed, under TM2_PORT_TEXTURE_BANKS, by the write of its
     bits 9..15 (`->rest = ... >> 9`), and modelRemapTextures reads portTexBanks (the live switch) in the link;
   - the upload word: the functions that store to portVramUploadBank are exactly UPLOAD_SETTERS; in each,
     in address order, every LoadImage / StoreImage call comes after a store of the bank and is followed
     by a store of 0 (`sw $zero`) before the next store of a bank or the end, and the last store is a 0;
     no other function stores to the word.
7. --bank-model: a host model of the banked placement (vramPlaceTexture / vramUploadClut with
   portTexBank*: a fresh bank per load group with bitmapInit's reservations, retail's scan, whole
   rectangles tested, arena keys left in bank 0) over the 15 car TMS files on each of the 14 arena TMS
   files of the disc: every image and CLUT placed, inside VRAM, no two rectangles of a bank overlapping,
   every image inside one texture page, every CLUT on a 16-texel boundary.

8. On a build with TM2_PORT_CAR_SPECIALS (request 33 step 2a, CAR_GROUPS.md 10): the selection functions
   (SPECIALS_PORT) and the retail bodies moved behind their wrappers (SPECIALS_BODIES) are in .port_text and
   none of the selection functions calls rand(); every function that relocates into the retail special
   state (SPECIAL_STATE: Mr. Slam's arm and claw, the tornado and its objects, the leap, the strike block,
   the 2324/2325 animations) is the selection mechanism, the first cars' own path (SPECIALS_RETAIL_PATH),
   dead (SPECIALS_DEAD), or reached only through callers that select (call portCarSpecialSelect or
   portCarSpecialNext, and portCarSpecialRestore): no path from a function without callers reaches the
   state outside a selection. objRegistrySetStep counts as a user only for calls with $a0 0x914 / 0x915
   (or not a constant). With --negative-control on a build without the define, check 8 runs too and must
   fail.
   With TM2_PORT_DARK_TOOTH (request 33 step 2e, CAR_GROUPS.md 11) Dark Tooth's head and boss state is part of
   the special state (DARK_TOOTH_STATE: the 7 controller scalars, the swoop block, the head, the swoop vectors
   and the stage flag aiBossDefeatedFlag), arenaSetup joins the first cars' path (it clears the first car's
   stage; portCarDarkToothClearStages the duplicates'), and the Dark Tooth port functions (DARK_TOOTH_PORT) must
   be in .port_text and call no rand() (request 42 item 1 adds the ordinary Dark Tooth's three, portCarOptions bit 10:
   tools/check_custom.py checks its gates); --expect-functions adds EXPECTED_DARK_TOOTH (with request 42's roundStart
   and carDestroyed). A negative control on a
   build with TM2_PORT_CAR_SPECIALS but without TM2_PORT_DARK_TOOTH: --negative-control checks the Dark Tooth
   state there too (it must fail: aiDarkToothBossUpdate <- aiWeaponFireDecide unselected).

9. On a build with TM2_PORT_CAR_SOUND (request 33 step 3, CAR_GROUPS.md 12): no changed function plays, stops or
   tests a per-car sound by id itself. Every call of soundPlay / soundPlayVariant / soundStop / soundStopInstance /
   soundIsActive whose $a0 is a per-car sound id (SOUND_IDS: the 14 engines 0x24..0x3C even and 0x41, Thumper's
   flame 47; with TM2_PORT_DARK_TOOTH the swoops 61, 66) is made by a port function (.port_text: the retail
   calls for bit 4 clear are made there, beside the per-instance ones), and the functions that used to make them
   (SOUND_HOOKS) make no such call with a non-constant $a0 either (except SOUND_NONCONST_OK: sound 21); each of them calls its port hook (SOUND_HOOKS),
   soundSortInstances calls portCarSoundSortInstances, the port functions (SOUND_PORT) are in .port_text and call
   no rand(). --expect-functions adds EXPECTED_SOUND (and aiDarkToothBossUpdate with TM2_PORT_DARK_TOOTH).
   With --negative-control on a build with TM2_PORT_CAR_GROUPS but without TM2_PORT_CAR_SOUND check 9 runs too
   and must fail (the retail calls in carSoundUpdate, worldUpdate, weaponFlameStart, weaponFlameUpdate).
   Request 33 step 4 (CAR_GROUPS.md 16) extends it: Twister's tornado sound 0x35 is a per-car id too
   (weaponFireSpecial -> portCarTornadoSound, carDestroyed -> portCarTornadoSoundStop); the human loops by side
   (SOUND_LOOP_API: soundStartPlayerLoop, soundStopPlayerLoop and the six loop-flag getters) are called only by
   the port functions (carThrottleSteer, weaponTick and carSoundUpdate call portHumanLoopPlay / portHumanLoopStop,
   which make retail's calls for players 0 and 1); and with TM2_PORT_CARS the sound request pool is raised:
   main (sub_8012C8DC) builds TM2_MAX_SOUND_REQUESTS objects (portSndObjSlots), sndPoolReset / sndPoolResetAll / sndPoolAlloc
   are in EXPECTED_CARS.

10. On a build with TM2_PORT_CAR_CLUTS (request 33 step 1, CAR_GROUPS.md 13) or TM2_PORT_DARK_TOOTH (its bit 6 shares
   VRAM rows 240..255 with the CLUT copies): the port functions (CLUTS_PORT; ROWS_PORT with Dark Tooth only) are in
   .port_text and call no rand(); bitmapInit calls portCarClutsReset and its first bitmapMarkRect still reserves
   rows 0..255 of x 0..639 ($a3 = 256: retail's scans never see rows 240..255, which only the allocator hands
   out); with TM2_PORT_CAR_CLUTS vramPlaceTexture calls portCarClutNote, sessionLoadArenaAndCars
   portCarClutsCopyAll, weaponInvisibilityUpdate portTexFadeId, vramUploadClut portClutSpotFree and
   portClutRowsLast and vramPlaceTexture portImagePasses and portImageSpotFree (portCarOptions bit 9, CAR_GROUPS.md
   13.12, request 41 item B); with TM2_PORT_DARK_TOOTH vramUploadClut calls
   portClutRowsUpload; no game function but clutAnimRegister (the per-key fade's live test) relocates into
   portCarCluts. --expect-functions adds EXPECTED_CLUTS.
11. --clut-model: a host model of the copies (check_clut_model: the load as the C runs it, rows 240..255 shared with
   bit 6) on the 14 arena TMS files: 16 of one type (each normal type), 4 teams of 4 (TEAM_SETS), the most types
   that fit filled up to 16, 15 of one type + Dark Tooth with bit 6 clear and set; each with portCarOptions bit 9
   clear and set (the whole-spot CLUT scan of the originals, rows 240..255 last; request 41 item B: the whole
   rectangle image scan, retail's scan as the fallback). Fails when a copy overlaps
   anything or another copy, leaves VRAM, is off a 16-texel boundary, takes rows 240..255 outside the allocator
   or beyond x 639, when an original moves or fails because of the copies, or, with bit 9, when an original CLUT
   or image lands on used texels (an image placed by the fallback is reported, not failed); prints what fits per
   arena. Then SIXTEEN_CARS.md 4.4's count: distinct types added
   largest / smallest first, how many load (retail), how many before retail's first CLUT over used texels, how
   many with bit 9 (none may overlap); negative control: bit 9 clear must show such an overlap.

12. On a build with TM2_PORT_CAR_DROPIN (request 33 "online with bots, players dropping in", CAR_GROUPS.md 14): the
   drop-in functions (DROPIN_PORT) are in .port_text and the game hooks are called (DROPIN_HOOKS:
   bitmapSetupStorage, sessionLoadArenaAndCars, sessionBindCars, portCarGroupsReset; portStartNodePick reads
   portCarDropIn); every reference holder of the census (DROPIN_HOLDERS: projectiles, arcs, drops, shields,
   ricochets, bullets, effect sprites, flame emitters, AI targets, the watched AI, the tornado, leap and strike
   states, the Dark Tooth swoop) is written by port code that portCarRemove reaches, which also calls the blocks
   that clear the record fields (DROPIN_REMOVE_CALLS); portHumanCompact writes every per-human array and id /
   address holder (DROPIN_COMPACT_HOLDERS); a call-graph self-test (the match load reaches its resets and, through
   dbscanHierarchy's function pointer, worldLoadObject), then nothing load-time-only (DROPIN_LOAD_ONLY: the resets
   of arenaResetSystems, bitmapInit, the CD and file reads, ...) and no rand() is reachable from
   portCarLoadGroupLive; and every game function that reads aiCarTotal, aiCarObjects, aiCarObjectSlots or
   aiCarTargets is in DROPIN_AI_REVIEWED (the vacant-record census): a new reader fails until it is reviewed.
   --expect-functions adds EXPECTED_DROPIN (bitmapSetupStorage, csAlloc).
   Unloading (CAR_GROUPS.md 14.7): every holder of what a car's install fills or names (UNLOAD_HOLDERS: the per-key
   registries, the slot tables and bindings, the drop-in and unload state, the drawn-object list and the words that
   name a car's scene objects, the projectile / ricochet / flame / shield / arc / drop pools, the words that keep a
   node of its DMD copy, the animation list, the world models, the image records; with the defines that make them
   per car the CLUT animations, portTexBanks, portCarCluts, portCarSpecials, request 35's as-entries, portCarSound,
   the far-LOD records) is written by the port code portCarUnloadGroup reaches, which calls the shared blocks
   (UNLOAD_CALLS, UNLOAD_CALLS_BY_DEFINE); portCarUnloadGroup reaches nothing load-time-only and no CD read; every
   game function that reads the slot tables (UNLOAD_SLOT_REVIEWED) or the per-key registries (UNLOAD_KEY_REVIEWED)
   has a census verdict for a slot / key an unload gave back (it looks like one no car of the load ever had).
15. --unload-model: a host model of what portCarLoadGroupLive / portCarUnloadGroup do with the DMD area (first fit
   over 4-aligned free ranges, coalesced, the tail given back), the overflow keys (the lowest free one), the car slots
   (the first run of free slots, Minion two) and the group numbers (the lowest free one), with the disc's car DMD
   sizes, over long join / leave / load / unload sequences: no two DMDs overlap, no key or slot is held twice, a type
   in no group gets its type key, the free bytes add up; and the fragmentation the DMD area shows (holes, the largest
   block, loads refused while enough bytes were free in total).

13. On a build with TM2_PORT_SPECIAL_AS (request 35, GUN_GAME_SPECIALS.md "As built"):
   - the special-as port functions (SPECIAL_AS_PORT; the load-mask ones with TM2_PORT_CARS) are in .port_text and
     call no rand();
   - the game hooks are called (SPECIAL_AS_HOOKS: weaponFireSpecial selects through portCarSpecialSelectFire and
     never portCarSpecialSelect itself, weaponShadowStrikeUpdate steps the borrowed strikes with portCarSpecialAsNext,
     weaponBlockInit calls portCarSpecialAsBlockInit, the AI's and the crush's type tests call portCarSpecialType,
     sessionLoadArenaAndCars the load-mask functions with TM2_PORT_CARS, portCarResetSpecials portCarSpecialAsEnd
     with TM2_PORT_CAR_SWAP); every borrowable special's state goes through the selection: check 8 counts
     portCarSpecialSelectFire / portCarSpecialSelectAs / portCarSpecialAsNext as selecting;
   - no game function relocates into portCarSpecialAs, portCarSpecialLoad or portSpecialAs (only port code);
   - the projectile model guard: the car projectile model words (MODEL_WORDS: Sweet Tooth's spiral, Roadkill's
     boomerang, Mr. Grimm's skull, Spectre's ghost; never cleared by retail between loads) are stored only by
     weaponStoreProjectileModel and the load reset portCarSpecialAsReset (which clears all four), and every game
     function that loads one tests the loaded register against zero (beq/bne) before it is overwritten, a call or
     a jump (within 40 instructions);
   - the refused types: include/semantic/car_specials.h TM2_PORT_SPECIAL_AS_MASK is the 10 borrowable types
     (SPECIAL_AS_BORROWABLE) and none of the 5 refused (SPECIAL_AS_REFUSED: Hammerhead, Grasshopper, Twister,
     Mr. Slam, Dark Tooth), and portCarSpecialAsSet's code holds that mask as an immediate.
   --expect-functions adds EXPECTED_SPECIAL_AS. With --negative-control on a build with TM2_PORT_CAR_SPECIALS but
   without TM2_PORT_SPECIAL_AS check 13 runs too and must fail (the unguarded spiral model stores, no hooks).

14. On a build with TM2_PORT_DT_PLAYER (request 36, DARK_TOOTH_PLAYER.md):
   - the port functions (DT_PLAYER_PORT) are in .port_text and call no rand() themselves;
   - the game hooks are called (DT_PLAYER_HOOKS: humanCarTick -> portCarDtPlayerHead, weaponTick ->
     portCarDtPlayerGuns, weaponFireSpecial -> portCarDtPlayerSwoop, weaponProjectileUpdate -> portCarDtPlayerShooter,
     screenCarSelect -> portCarSelExtend / portCarSelCount / portCarBioFile; request 42: portCarSelExtend ->
     portCarSelCopy, the carousel and both name labels; screenEnding -> portCarEnding);
   - a human Dark Tooth never reaches the boss controller: aiDarkToothBossUpdate's only caller is aiWeaponFireDecide,
     whose only caller is weaponTick (its AI branch, retail), neither is pointed to, and nothing the port functions
     reach (through port and game code) is either of them;
   - the head placement has no rand(): nothing portCarDtPlayerPlace reaches calls rand(); the port functions that may
     reach rand() through the game (weaponFireDarkToothMissile's turn rate, carApplyDamage's spin-out: as the
     boss's) are listed;
   - portDtPlayer is used only by the port code and DT_PLAYER_STORAGE_USERS (carSoundUpdate, weaponBlockInit;
     request 42: screenEnding through portCarEnding).
   --expect-functions adds EXPECTED_DT_PLAYER (and, without TM2_PORT_CARS, the texture-slot stride's four
   functions DT_PLAYER_STRIDE). With --negative-control on a build with TM2_PORT_DARK_TOOTH but without
   TM2_PORT_DT_PLAYER check 14 runs too and must fail.

16. On a build with TM2_PORT_DARK_TOOTH (request 46, PLAYTEST_ANSWERS.md 7: a parked Dark Tooth's head):
   - the frame hook: aiCarsCountRemaining (worldUpdate, every frame) calls portCarDarkToothParked, which hides through
     portCarDarkToothParkHead and selects the key (portCarSpecialSelect and portCarSpecialRestore) to show it again;
   - every park path hides through it: with TM2_PORT_CAR_SWAP portCarParkSlot and portCarParkAi (and portCarModelOn
     where it is a function of its own; else its callers portCarSetGroup / portCarStartHuman / portCarStartAi) call
     portCarDarkToothParkHead; the paths that park a car without a park function (the spares' portCarSpecialLoadDone,
     the port's own out-of-play writes) are covered by the frame hook (slot kind 2, or dead in wreck state 4);
   - portCarDtParkMask is used only by PARK_MASK_USERS (the two functions and the load reset portCarSpecialsReset).
   With --negative-control on a build without TM2_PORT_DARK_TOOTH check 16 runs too and must fail.

With --negative-control on a build without TM2_PORT_CAR_GROUPS (e.g. the default variant) check 1
lists every retail reader of the tables and fails: a negative control for the check itself. Exit
status 0 = pass.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402
from reloc_image import Link, AddrMap  # noqa: E402

# retail per-type tables: name -> (retail VA, retail size in bytes)
TABLES = {
    'carBodyObjects': (0x801873D0, 15 * 4), 'carWreckModels': (0x8018740C, 15 * 4),
    'carDamageStageObjects': (0x8018F4E4, 16 * 4), 'carTireSwitchObjects': (0x8018F524, 16 * 3 * 4),
    'carPartsTireTurnAccum': (0x80187344, 15 * 4), 'carTurboObjects': (0x8018F5E4, 16 * 4),
    'D_8018F624': (0x8018F624, 16 * 4), 'carFreezeObjects': (0x8018F950, 16 * 264),
    'carPartsHammerhead': (0x801872FC, 4 * 4), 'carPartsMrSlam': (0x8018730C, 4 * 4),
    'carPartsMinion': (0x8018731C, 6 * 4), 'carPartsDarkTooth': (0x80187334, 4 * 4),
    'carAxelObjects': (0x80180F50, 2 * 4), 'carPartsSpinAngleHammerhead': (0x80180B78, 4),
    'carPartsSpinAngleMinion': (0x80180B7C, 4), 'carPartsSpinAngleMrSlam': (0x80180B80, 4),
    'carPartsSpinAngleDarkTooth': (0x80180B84, 4), 'carPartsSpinAngleAxel': (0x80180B88, 4),
}
ALLOWED = {'carPartsReset', 'objRegistriesReset', 'carPartsSpinHammerhead', 'carPartsSpinMinion',
           'carPartsSpinMrSlam', 'carPartsSpinDarkTooth', 'carPartsSpinAxel', 'objRegistryAdd',
           'objRegistrySetStep'}
# CAR_GROUPS_PLAN.md 1.5 (registration and reset), 1.6 (readers), 1.7 (callers), 1.8 (binders, pickers)
EXPECTED = {
    'carTablesClear', 'carRegisterModel', 'carBodyObjectRegister', 'carWreckModelRegister',
    'carRegisterTireSwitch', 'objRegistryAdd', 'objRegistrySetStep', 'carFreezeBuild',
    'carPartRegisterHammerhead', 'carPartRegisterMinion', 'carPartRegisterMrSlam', 'carPartRegisterDarkTooth',
    'carPartRegisterAxel', 'arenaSetup', 'carChoiceRegisterGroup',
    'carBodyObjectHide', 'carBodyObjectShow', 'carBodyPoseUpdate', 'carWreckModelGet', 'carSetDamageStage',
    'uaswToggleCarTireState', 'carPartsAnimate', 'carFreezeSetState', 'carFreezeResetAll', 'carObjectFromOwner',
    'carApplyDamage', 'carGroundEffects', 'weaponFireSpecial', 'weaponInvisibilityUpdate', 'weaponTornadoUpdate',
    'weaponHomingLeapUpdate', 'humanCarDamageUpdate', 'aiCarDamageUpdate', 'humanCarTick', 'aiCarDrive',
    'humanCarReadInput', 'aiInputFinalize', 'projectileHitTest', 'weaponSlamSwingUpdate', 'weaponSlamFindTarget',
    'carCrashStart', 'carWreckUpdate', 'carDestroyed',
    'sessionBindCars', 'carBindHumanByType', 'carBindAiByType', 'carIsSelectable',
}
# CAR_GROUPS.md "Step 2": the functions TM2_PORT_CARS=16 changes (limits, settings, loads, pickers, pools)
EXPECTED_CARS = {
    'aiCarGet', 'aiCarsSetActive', 'arenaSetup', 'bitmapAddRecord', 'carPairDirection', 'carPairDistance',
    'carPairHeading', 'carPairHeadingDelta', 'carPairVector', 'carPairsUpdate', 'carRegisterModel',
    'carTablesClear', 'clutAnimFindRunning', 'clutAnimFreeSlot', 'clutAnimInit', 'clutAnimNewDef', 'clutAnimUpdate',
    'csAlloc', 'fxRecordFindFree', 'fxRecordGet', 'fxRecordsClear', 'fxRecordsUpdate', 'hudDisplayRadar',
    'hudRadarInit', 'hudRadarPlotCar', 'hudRadarSwapBuffers', 'missileSetTrailColor', 'missileTrailsInit',
    'missilesCreateObjects', 'modelRemapTextures', 'projectilesReset', 'renderAddWorldModel',
    'renderClearModelLists', 'renderRemoveWorldModel', 'renderScene', 'screenAiSelect', 'sessionBindCars',
    'sessionGetOpponentCar', 'sessionLoadArenaAndCars', 'sessionSetOpponentCar', 'settingsClearOpponentCars',
    'settingsGetOpponentCar', 'settingsSetOpponentCar', 'soundDriverInit', 'soundDriverReset', 'soundDriverUpdate',
    'soundInstAlloc', 'startNodePickRandom', 'sub_8012A910', 'tournamentOpponentCount', 'tournamentPickOpponents',
    'uaInit', 'vramPlaceTexture', 'vramUploadClut', 'weaponArcSlotFind', 'weaponArcsDraw', 'weaponAssetBind',
    'weaponBombObjectFind', 'weaponDropWrite', 'weaponDropsReset', 'weaponFireEnergyAttack', 'weaponFireSelected',
    'weaponFlameSlotFind', 'weaponFlameUpdate', 'weaponObjectsDetach', 'weaponRicochetSlotFind',
    'weaponRicochetUpdate', 'weaponShieldSlotFind', 'weaponShieldsUpdate', 'weaponSlamStateAlloc',
    'weaponsRoundReset',
    'sub_8012C8DC', 'sndPoolReset', 'sndPoolResetAll', 'sndPoolAlloc',   # request 33-4: the sound request pool (main)
    'startNodesClearUsed', 'humanCarPlaceAtStart', 'aiCarStart',   # request 49: the cars placed this round
}
CAR_KEYS = 48  # include/semantic/car_groups.h TM2_CAR_KEYS
# CAR_GROUPS.md 8: the port functions of TM2_PORT_CAR_SWAP (step 3); no game function beyond step 1's
SWAP_PORT = {
    'portCarSetGroup', 'portCarParkAi', 'portCarBindGroupOf', 'portCarBindTypeOf', 'portCarResetSpecials',
    'portCarResetLook', 'portCarUnbind', 'portCarBindSlot', 'portCarApplyType', 'portCarParkSlot',
    'portCarRekeyReferences',
}
# CAR_GROUPS.md 9: the game functions TM2_PORT_TEXTURE_BANKS (step 4) changes
EXPECTED_BANKS = {
    'bitmapInit', 'vramPlaceTexture', 'vramUploadClut', 'modelRemapTextures', 'clutAnimRegister',
    'gpuLoadImageLine', 'gpuStoreImageLine', 'weaponInvisibilityUpdate',
}
# ... and its port functions (.port_text)
BANKS_PORT = {
    'portTexBanksReset', 'portTexBankIsFree', 'portTexBankMark', 'portTexBankAlloc', 'portTexBankFree',
    'portTexBankOfGroup', 'portTexPlaceBegin', 'portTexFindReuse', 'portTexNoteSlot', 'portTexFadeId',
    'portTexLoadImage',
}
# the functions that announce a bank to the port's GPU (portVramUploadBank) around LoadImage / StoreImage
UPLOAD_SETTERS = {'vramPlaceTexture', 'vramUploadClut', 'gpuLoadImageLine', 'gpuStoreImageLine', 'portTexLoadImage'}
UPLOAD_FUNCS = {'LoadImage', 'StoreImage', 'sub_801416F4', 'sub_80141758'}
# CAR_GROUPS.md 10: the game functions TM2_PORT_CAR_SPECIALS (request 33 step 2a) changes
EXPECTED_SPECIALS = {
    'weaponFireSpecial', 'weaponTornadoUpdate', 'weaponHomingLeapUpdate', 'weaponShadowStrikeUpdate',
    'humanCarTick', 'aiCarDrive', 'slamArmRegister', 'slamClawRegister', 'weaponAssetBind',
    'objRegistryAdd', 'carStatsLoad', 'weaponsRoundReset', 'weaponSlamStateAlloc',
}
# ... its port functions, and retail's bodies moved to .port_text behind the wrappers at the retail addresses
SPECIALS_PORT = {
    'portCarSpecialSelect', 'portCarSpecialRestore', 'portCarSpecialNext', 'portCarSpecialRegister',
    'portCarKeyOfStats', 'portCarClawRecord', 'portCarSpecialsReset', 'portCarSpecialsRoundReset',
}
SPECIALS_BODIES = {'weaponTornadoUpdateOne', 'weaponHomingLeapUpdateOne', 'weaponShadowStrikeUpdateOne'}
# the retail special state (retail VA, bytes): PER_CAR_PLAN.md A.2 rows 1, 3, 4, 5, 7
SPECIAL_STATE = {
    'slam': (0x80180F74, 16), 'tornado_objects': (0x80180F38, 8), 'tornado': (0x80184DC0, 0x14),
    'leap': (0x80184DD4, 0x3C), 'strike': (0x80184E10, 0x64), 'anim2324': (0x80181064, 4), 'anim2325': (0x80181068, 4),
}
SPECIALS_MECHANISM = {'portCarSpecialExchange', 'portCarSpecialSwitch', 'portCarSpecialRegister'}
# the first cars' state outside any selection (the port code does the duplicates' part: portCarSpecialsReset
# at each load, before these clears; portCarSpecialsRoundReset at the end of weaponsRoundReset)
SPECIALS_RETAIL_PATH = {'weaponsRoundReset', 'weaponObjectsDetach', 'carPartsReset'}
SPECIALS_DEAD = {'weaponHomingLeapState'}
# callers of objRegistrySetStep whose id is not a constant the check can read: they pass a target record's id
# (targetRegister: ids 500..520) or a level object's constant (eiffelTowerReset: 0xC80), never 0x914 / 0x915
SPECIALS_OTHER_IDS = {'targetDamage', 'targetActorSpawn', 'targetActorRespawn', 'targetActorsUpdate', 'targetActorHit',
                      'targetLevelEventUpdate', 'eiffelTowerReset'}
# CAR_GROUPS.md 11: the game functions TM2_PORT_DARK_TOOTH (request 33 step 2e) changes
EXPECTED_DARK_TOOTH = {
    'aiWeaponFireDecide', 'aiCarsCountRemaining', 'worldUpdate', 'carApplyDamage', 'carContactApplyObject',
    'arenaSetup', 'weaponProjectileUpdate', 'aiDarkToothJumpSelect', 'aiFollowRouteTarget', 'bitmapInit',
    'vramUploadClut',   # request 33-1: bit 6's room in rows 240..255 comes from the shared allocator
    'roundStart', 'carDestroyed',   # request 42 item 1 (bit 10): the boss route patch, the boss wreck timer
}
# ... its port functions (.port_text)
DARK_TOOTH_PORT = {
    'portCarBossDefeated', 'portCarDarkToothWreck', 'portCarDarkToothStages', 'portCarDarkToothRoundEnd',
    'portCarDarkToothClearStages', 'portCarDarkToothShooter', 'portCarDarkToothStage',
    'portCarDarkToothOrdinary', 'portCarDarkToothRole', 'portCarDarkToothHeadShow',   # request 42 item 1
    'portCarDarkToothParkHead', 'portCarDarkToothParked',   # request 46 (check 16)
}
# request 46 (check 16): the park mask's users, and the park paths that must hide the head through the port function
PARK_MASK_USERS = {'portCarDarkToothParkHead', 'portCarDarkToothParked', 'portCarSpecialsReset'}
PARK_PATHS = ('portCarParkSlot', 'portCarParkAi')
# Dark Tooth's single retail state (retail VA, bytes): PER_CAR_PLAN.md A.2 row 6, SURVIVAL.md 1.6
DARK_TOOTH_STATE = {
    'dt_scalars': (0x80180B4C, 14), 'dt_swoop': (0x80180F18, 16), 'dt_head': (0x80180F40, 4),
    'dt_vectors': (0x80183F64, 24), 'dt_stage': (0x8018102C, 4),
}
# CAR_GROUPS.md 12: the game functions TM2_PORT_CAR_SOUND (request 33 step 3) changes (+ aiDarkToothBossUpdate
# with TM2_PORT_DARK_TOOTH: the swoop sounds per Dark Tooth)
EXPECTED_SOUND = {'carSoundUpdate', 'worldUpdate', 'weaponFlameStart', 'weaponFlameUpdate', 'soundSortInstances',
                  'carThrottleSteer', 'weaponTick', 'weaponFireSpecial', 'carDestroyed'}   # (last 4: request 33-4)
SOUND_PORT = {
    'portCarSoundFrame', 'portCarEngineUpdate', 'portCarSoundPlay', 'portCarFlameSound', 'portCarFlameSoundEnd',
    'portCarFlameSoundIdle', 'portCarSoundStopSlot', 'portCarSoundSortInstances',
    'portHumanLoopPlay', 'portHumanLoopStop', 'portCarTornadoSound', 'portCarTornadoSoundStop',   # request 33-4
}
# the per-car sound ids: engines (carSoundUpdate's switch), Thumper's flame; Dark Tooth's swoops
SOUND_ENGINES = {0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x41}
SOUND_IDS = SOUND_ENGINES | {47, 0x35}   # 0x35: Twister's tornado (request 33-4)
SOUND_SWOOPS = {61, 66}
SOUND_API = {'soundPlay', 'soundPlayVariant', 'soundStop', 'soundStopInstance', 'soundIsActive'}
# request 33-4: the human loops by side 0/1 (soundMgrPlaySided) and their flags: only the port functions call them
SOUND_LOOP_API = {'soundStartPlayerLoop', 'soundStopPlayerLoop', 'soundGetTurboLoopP1', 'soundGetTurboLoopP2',
                  'soundGetGunLoopP1', 'soundGetGunLoopP2', 'soundGetLoop32P1', 'soundGetLoop32P2'}
# function -> the port hooks it must call
SOUND_HOOKS = {
    'carSoundUpdate': {'portCarEngineUpdate'}, 'worldUpdate': {'portCarSoundFrame', 'portCarEngineUpdate'},
    'weaponFlameStart': {'portCarFlameSound'}, 'weaponFlameUpdate': {'portCarFlameSoundEnd', 'portCarFlameSoundIdle'},
    'soundSortInstances': {'portCarSoundSortInstances'},
    # request 33-4
    'carThrottleSteer': {'portHumanLoopPlay', 'portHumanLoopStop'}, 'weaponTick': {'portHumanLoopPlay', 'portHumanLoopStop'},
    'weaponFireSpecial': {'portCarTornadoSound'}, 'carDestroyed': {'portCarTornadoSoundStop'},
}
SOUND_HOOKS_CAR_SOUND_UPDATE_33_4 = {'portHumanLoopPlay', 'portHumanLoopStop'}
# calls of the sound API with a non-constant $a0 that SOUND_FUNCS keep: carSoundUpdate's skid/shield hum, sound 21 by
# player index (soundPlayVariant(21, ..., id) / soundStopInstance(21, id); the compiler keeps 21 in a saved register)
SOUND_NONCONST_OK = {'carSoundUpdate': {'soundPlayVariant', 'soundStopInstance'}}
# CAR_GROUPS.md 13: the game functions TM2_PORT_CAR_CLUTS (request 33 step 1) changes
EXPECTED_CLUTS = {
    'bitmapInit', 'vramPlaceTexture', 'clutAnimRegister', 'weaponInvisibilityUpdate', 'sessionLoadArenaAndCars',
    'vramUploadClut',   # portCarOptions bit 9: the whole-spot CLUT scan (CAR_GROUPS.md 13.12)
}
# ... its port functions (.port_text); ROWS_PORT: the rows 240..255 allocator, also with TM2_PORT_DARK_TOOTH alone
ROWS_PORT = {'portCarClutsReset', 'portClutRowsAlloc'}
CLUTS_PORT = ROWS_PORT | {'portCarClutNote', 'portCarClutsCopyAll', 'portCarClutsCopyGroup', 'portClutSpotFree',
                          'portClutRowsLast', 'portImagePasses', 'portImageSpotFree', 'portTexBanksReset',
                          'portTexPlaceBegin', 'portTexFindReuse', 'portTexNoteSlot', 'portTexFadeId'}
CLUTS_CALLS = {'bitmapInit': 'portCarClutsReset', 'vramPlaceTexture': 'portCarClutNote',
               'sessionLoadArenaAndCars': 'portCarClutsCopyAll', 'weaponInvisibilityUpdate': 'portTexFadeId'}
CLUTS_SIZE = 0x1BD0   # include/semantic/car_cluts.h struct TM2PortCarCluts
# check 11: the 4 x 4 team sets (4 types, 4 cars each): CLUT-heavy (Axel's three 256-colour CLUTs, Sweet Tooth's two,
# Mr. Grimm's 16 and Mr. Slam's 8 part CLUTs), the plain ones, and mixes
TEAM_SETS = [('AX', 'ST', 'MG', 'IR'), ('HH', 'OT', 'WH', 'PV'), ('TH', 'SP', 'RK', 'FL'), ('HS', 'MN', 'AX', 'ST'),
             ('MG', 'IR', 'SP', 'OT'), ('MN', 'HS', 'RK', 'TH')]
KNOWN_DEFINES = {'TM2_PORT_CAR_GROUPS', 'TM2_PORT_CARS', 'TM2_PORT_CAR_SWAP', 'TM2_PORT_TEXTURE_BANKS',
                 'TM2_PORT_CAR_SPECIALS', 'TM2_PORT_DARK_TOOTH', 'TM2_PORT_CAR_SOUND', 'TM2_PORT_CAR_CLUTS',
                 'TM2_PORT_CAR_DROPIN', 'TM2_PORT_SPECIAL_AS', 'TM2_PORT_DT_PLAYER'}
# GUN_GAME_SPECIALS.md "As built": the game functions TM2_PORT_SPECIAL_AS (request 35) changes (+ sessionLoadArenaAndCars
# with TM2_PORT_CARS, already step 2's)
EXPECTED_SPECIAL_AS = {
    'weaponFireSpecial', 'weaponShadowStrikeUpdate', 'weaponBlockInit', 'weaponFireSelected', 'aiWeaponFireDecide',
    'aiTryRouteToSlowTarget', 'aiRollTakeRoute', 'aiShouldChaseDirect', 'sub_800E9E20', 'aiFreeSteer', 'carCrashStart',
    'weaponFireSpiralMissile', 'weaponFireDarkToothMissile',
}
# ... its port functions (.port_text); SPECIAL_AS_LOAD_PORT only with TM2_PORT_CARS
SPECIAL_AS_PORT = {
    'portCarSpecialType', 'portCarSpecialSelectAs', 'portCarSpecialSelectFire', 'portCarSpecialAsNext',
    'portCarSpecialAsRing', 'portCarSpecialAsEnd', 'portCarSpecialAsBlockInit', 'portCarSpecialAsReset',
    'portCarSpecialAsSet',
}
SPECIAL_AS_LOAD_PORT = {'portCarSpecialLoadPlan', 'portCarSpecialLoadCar', 'portCarSpecialLoadDone'}
SPECIAL_AS_SELECT = {'portCarSpecialSelectFire', 'portCarSpecialSelectAs', 'portCarSpecialAsNext'}
# function -> the port functions it must call
SPECIAL_AS_HOOKS = {
    'weaponFireSpecial': {'portCarSpecialSelectFire', 'portCarSpecialAsRing', 'portCarSpecialRestore'},
    'weaponShadowStrikeUpdate': {'portCarSpecialAsNext', 'portCarSpecialRestore'},
    'weaponBlockInit': {'portCarSpecialAsBlockInit'},
    'portCarSpecialsReset': {'portCarSpecialAsReset'},
    'weaponFireSelected': {'portCarSpecialType'}, 'aiWeaponFireDecide': {'portCarSpecialType'},
    'aiTryRouteToSlowTarget': {'portCarSpecialType'}, 'aiRollTakeRoute': {'portCarSpecialType'},
    'aiShouldChaseDirect': {'portCarSpecialType'}, 'sub_800E9E20': {'portCarSpecialType'},
    'aiFreeSteer': {'portCarSpecialType'}, 'carCrashStart': {'portCarSpecialType'},
}
SPECIAL_AS_HOOKS_CARS = {'sessionLoadArenaAndCars': SPECIAL_AS_LOAD_PORT}
# DARK_TOOTH_PLAYER.md: the game functions TM2_PORT_DT_PLAYER (request 36) changes
EXPECTED_DT_PLAYER = {
    'carIsSelectable', 'screenCarSelect', 'screenEnding', 'sessionRun', 'viewCameraOffsetsInit', 'weaponFireSpecial',
    'weaponTick', 'humanCarTick', 'carContactApplyObject', 'projectileHitTest', 'weaponProjectileUpdate',
    'carSoundUpdate', 'weaponBlockInit', 'carApplyDamage',   # request 41 item A: the won-round test
    'pauseMenu',        # request 42: no password line for Dark Tooth in the pause menu either
}
# ... and the texture-slot stride 32 it forces (TM2_PORT_CARS has it already)
DT_PLAYER_STRIDE = {'bitmapAddRecord', 'vramPlaceTexture', 'vramUploadClut', 'modelRemapTextures'}
# ... its port functions (.port_text)
DT_PLAYER_PORT = {
    'portCarDtPlayerHead', 'portCarDtPlayerGuns', 'portCarDtPlayerSwoop', 'portCarDtPlayerShooter', 'portCarSelExtend',
    'portCarSelCount', 'portCarBioFile', 'portCarDtPlayerPlace', 'portCarDtPlayerSwoopStep',
    'portCarSelCopy', 'portCarSelSetChild', 'portCarSelNumber', 'portCarEnding',   # request 42 item 5
}
DT_PLAYER_HOOKS = {
    'humanCarTick': {'portCarDtPlayerHead'}, 'weaponTick': {'portCarDtPlayerGuns'},
    'weaponFireSpecial': {'portCarDtPlayerSwoop', 'portCarSpecialRestore'},
    'weaponProjectileUpdate': {'portCarDtPlayerShooter'},
    'screenCarSelect': {'portCarSelExtend', 'portCarSelCount', 'portCarBioFile'},
    'portCarSelExtend': {'portCarSelCopy'},         # request 42 item 5: the carousel and both name labels
    'screenEnding': {'portCarEnding'},              # request 42: Dark Tooth's (and a mod's) movie
    'portCarDtPlayerHead': {'portCarSpecialSelect', 'portCarSpecialRestore', 'portCarDtPlayerPlace',
                            'portCarDtPlayerSwoopStep'},
    'portCarDtPlayerSwoop': {'portCarSpecialSelect', 'portCarSpecialRestore'},
    'portCarDtPlayerGuns': {'weaponFireDarkToothMissile'},
}
DT_PLAYER_STORAGE_USERS = {'carSoundUpdate', 'weaponBlockInit',   # request 42: screenEnding through portCarEnding
                           'viewCameraOffsetsInit'}               # request 118: a car's camera in phase 2
DT_PLAYER_BOSS = {'aiDarkToothBossUpdate': {'aiWeaponFireDecide'}, 'aiWeaponFireDecide': {'weaponTick'}}
SPECIAL_AS_HOOKS_SWAP = {'portCarResetSpecials': {'portCarSpecialAsEnd'}}
SPECIAL_AS_STORAGE = {'portCarSpecialAs': 128, 'portCarSpecialLoad': 4, 'portSpecialAs': 0x1C18}   # bytes
# the car projectile model words (weaponStoreProjectileModel ids 2301, 2321, 2322, 2326) and their only writers
MODEL_WORDS = ('weaponSpiralMissileModel', 'weaponBoomerangModel', 'weaponGrimmSpecialModel', 'weaponGhostMissileModel')
MODEL_WRITERS = {'weaponStoreProjectileModel', 'portCarSpecialAsReset'}
SPECIAL_AS_BORROWABLE = {0x10, 0x30, 0x40, 0x50, 0x70, 0x80, 0x90, 0xB0, 0xD0, 0xE0}
SPECIAL_AS_REFUSED = {0x20, 0x60, 0xA0, 0xC0, 0xF0}


def functions(out):
    rows = []
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        rows.append((int(f[0], 16), int(f[2]), f[4]))
    rows.sort()
    return rows


def owner(rows, addr):
    lo, hi = 0, len(rows)
    while lo < hi:
        mid = (lo + hi) // 2
        if rows[mid][0] <= addr:
            lo = mid + 1
        else:
            hi = mid
    if lo and rows[lo - 1][0] <= addr < rows[lo - 1][0] + rows[lo - 1][1]:
        return rows[lo - 1][2]
    return None


def dbscan_ids(dmd):
    """Callback ids of a DMD in the order assetInstallImage + dbscanHierarchy produce them."""
    ids = []
    for root in dmd.roots:
        stack = [root]
        while stack:
            p = stack.pop()
            if not p:
                continue
            o = dmd.off(p)
            op, nid = dmd.d[o], dmd.u16(o + 2)
            kids = []
            if op == 1:
                kids = [dmd.u32(o + 24 + 4 * i) for i in range(dmd.d[o + 20])]
            elif op == 2:
                kids = [dmd.u32(dmd.off(dmd.u32(o + 20 + 4 * i)) + 8) for i in range(dmd.u32(o + 16))]
            elif op == 3:
                kids = [dmd.u32(o + 32 + 4 * i) for i in range(dmd.d[o + 11])]
            elif op == 4:
                kids = [dmd.u32(o + 20 + 4 * i) for i in range(dmd.d[o + 18])]
            elif op == 5:
                kids = [dmd.u32(o + 40 + 4 * i) for i in range(dmd.d[o + 38])]
            elif op == 9:
                kids = [dmd.u32(o + 16 + 4 * i) for i in range(dmd.d[o + 6])]
            elif op == 11:
                kids = [dmd.u32(o + 8), dmd.u32(o + 12)]
            elif op == 12:
                kids = [dmd.u32(o + 8 + 4 * i) for i in range(dmd.u16(o + 6))]
                nid = 0
            elif op not in (0, 8, 13):
                nid = 0
            stack.extend(kids)
            if nid:
                ids.append(nid)
    return ids


def check_dmd():
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN, LEVELS
    disc = Disc(DEFAULT_BIN)
    names = sorted(p.split('/')[-1][:-4] for p in disc.files if p.startswith('/DMD/') and p.endswith('.DMD'))
    ok, cars = True, 0
    for name in names:
        if name in LEVELS:
            continue
        ids = dbscan_ids(Dmd(name, disc.get('/DMD/%s.DMD' % name)))
        roots = [i for i in ids if 16 <= i <= 240 and i % 16 == 0]
        if not roots:
            continue
        cars += 1
        first_car = next(k for k, i in enumerate(ids) if 16 <= i < 256)
        good = first_car == 0 and ids[0] in roots
        ok = ok and good
        print('  %-9s %-4s first callback %3d, car roots %s, %d car ids (16..255)' % (
            name, 'ok' if good else 'FAIL', ids[0], roots, sum(1 for i in ids if 16 <= i < 256)))
    print('car DMDs: %d, root first in every one: %s' % (cars, 'ok' if ok else 'FAIL'))
    return ok and cars > 0


def check_key_model():
    """portCarRegisterRoot (port_variant_arrays.c) over load orders: [(group, type), ...] per root."""
    def keys(order):
        group_key, dup, out = {}, [0], []
        for g, t in order:
            if g not in group_key:
                k = t
                if any(h != g and v == t for h, v in group_key.items()) and dup[0] < CAR_KEYS - 16:
                    k = (16 + dup[0]) << 4
                    dup[0] += 1
                group_key[g] = k
            out.append(group_key[g])
        return out
    types = [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0]

    def roots(cars):
        order = []
        for g, t in enumerate(cars, 1):
            order += [(g, t)] * (2 if t == 0xE0 else 1)
        return order
    cases = [('11 distinct', types[:11]), ('15 distinct', types), ('16 Minions', [0xE0] * 16),
             ('4 humans + 12 mixed', [0x10, 0x10, 0xE0, 0xC0] + [0xE0, 0x10, 0xC0, 0x20] * 3),
             ('32 loaded, 16 Minions + spares', [0xE0] * 17 + types[:14] + [0xC0]),
             ('32 Minions', [0xE0] * 32)]
    ok = True
    for label, cars in cases:
        order = roots(cars)
        k = keys(order)
        distinct = len(set(cars)) == len(cars)
        good = max(k) >> 4 < CAR_KEYS and (not distinct or k == [t for _, t in order])
        good = good and all(k[i] == k[j] for i in range(len(order)) for j in range(len(order))
                            if order[i][0] == order[j][0])
        ok = ok and good
        print('  %-20s %-4s keys %s' % (label, 'ok' if good else 'FAIL', ' '.join('%X' % x for x in k)))
    print('key model: %s' % ('ok' if ok else 'FAIL'))
    return ok


UNLOAD_AREA = 2 * 1024 * 1024 + 2048   # players.h TM2_PORT_CAR_DMD_AREA_BYTES
UNLOAD_VACANT_SLOT = 63                 # TM2_MAX_CAR_SLOTS - 1 (car_dropin.c DROPIN_VACANT_SLOT)
UNLOAD_GROUPS = 64                      # TM2_PORT_DROPIN_GROUPS


class UnloadModel:
    """portCarLoadGroupLive / portCarUnloadGroup's allocators (port_variant_arrays.c), on offsets in the DMD area."""

    def __init__(self, end=UNLOAD_AREA):
        self.end, self.next, self.ranges = end, 0, []          # tail start; [(start, size)] by address
        self.groups = {}                                        # g -> dict(type, key, slots, start, size)
        self.state = {}                                         # g -> 'loaded' | 'unloaded'
        self.next_group, self.dup_count, self.key_type = 1, 0, {}
        self.slot_group, self.count = [None] * UNLOAD_VACANT_SLOT, 0
        self.stats = {'loads': 0, 'unloads': 0, 'refused': {}, 'frag_refused': 0, 'max_holes': 0,
                      'min_largest_at_refusal': None}

    def find(self, size):
        n = (size + 3) & ~3
        for s, z in self.ranges:
            if z >= n:
                return s
        return self.next if self.next + n <= self.end else None

    def take(self, p, size):
        n = (size + 3) & ~3
        for i, (s, z) in enumerate(self.ranges):
            if s == p:
                self.ranges[i] = (s + n, z - n)
                if z - n == 0:
                    del self.ranges[i]
                return
        self.next = p + n

    def give(self, p, size):
        a, e = (p + 3) & ~3, (p + size) & ~3
        assert a < e <= self.next
        rs = sorted(self.ranges + [(a, e - a)])
        merged = []
        for s, z in rs:
            if merged and merged[-1][0] + merged[-1][1] == s:
                merged[-1] = (merged[-1][0], merged[-1][1] + z)
            else:
                merged.append((s, z))
        if merged and merged[-1][0] + merged[-1][1] == self.next:
            self.next = merged.pop()[0]
        self.ranges = merged

    def free_dup(self):
        d = 0
        while d < self.dup_count and self.key_type.get(16 + d, 0) != 0:
            d += 1
        return d if d < CAR_KEYS - 16 else -1

    def next_key(self, typ):
        if not any(g['key'] == typ for g in self.groups.values()):
            return typ
        d = self.free_dup()
        return 0 if d < 0 else (16 + d) << 4

    def slot_run(self, roots):
        for s in range(0, UNLOAD_VACANT_SLOT - roots + 1):
            if all(s + k >= self.count or self.slot_group[s + k] == -1 for k in range(roots)):
                return s
        return -1

    def free_group(self):
        for g in range(1, min(self.next_group, UNLOAD_GROUPS)):
            if self.state.get(g) == 'unloaded':
                return g
        return self.next_group

    def load(self, typ, size, roots):
        g, first, p, key = self.free_group(), self.slot_run(roots), self.find(size), self.next_key(typ)
        why = ('group' if g >= UNLOAD_GROUPS else 'slot' if first < 0 else 'dmd' if p is None
               else 'key' if key == 0 else None)
        if why:
            self.stats['refused'][why] = self.stats['refused'].get(why, 0) + 1
            if why == 'dmd':
                free = sum(z for _, z in self.ranges) + self.end - self.next
                if free >= ((size + 3) & ~3):
                    self.stats['frag_refused'] += 1
                    big = max([z for _, z in self.ranges] + [self.end - self.next])
                    m = self.stats['min_largest_at_refusal']
                    self.stats['min_largest_at_refusal'] = big if m is None else min(m, big)
            return None
        self.take(p, size)
        if key >= 0x100:
            self.key_type[key >> 4] = typ
            if (key >> 4) - 16 == self.dup_count:
                self.dup_count += 1
        slots = list(range(first, first + roots))
        for s in slots:
            self.slot_group[s] = g
        self.count = max(self.count, first + roots)
        self.groups[g] = {'type': typ, 'key': key, 'slots': slots, 'start': p, 'size': size}
        self.state[g] = 'loaded'
        if g == self.next_group:
            self.next_group += 1
        self.stats['loads'] += 1
        self.stats['max_holes'] = max(self.stats['max_holes'], len(self.ranges))
        return g

    def unload(self, g):
        grp = self.groups.pop(g)
        self.give(grp['start'], grp['size'])
        for s in grp['slots']:
            self.slot_group[s] = -1
        while self.count > 0 and self.slot_group[self.count - 1] == -1:
            self.count -= 1
        if grp['key'] >= 0x100:
            self.key_type[grp['key'] >> 4] = 0
        self.state[g] = 'unloaded'
        self.stats['unloads'] += 1

    def check(self):
        spans = sorted([(g['start'], g['start'] + ((g['size'] + 3) & ~3)) for g in self.groups.values()] +
                       [(s, s + z) for s, z in self.ranges] + [(self.next, self.end)])
        if any(a[1] > b[0] for a, b in zip(spans, spans[1:])) or spans[0][0] < 0 or spans[-1][1] > self.end:
            return 'overlap'
        if sum(b - a for a, b in spans) != self.end:
            return 'bytes'
        keys = [g['key'] for g in self.groups.values()]
        if len(keys) != len(set(keys)) or any(k >> 4 >= CAR_KEYS for k in keys):
            return 'keys'
        slots = [s for g in self.groups.values() for s in g['slots']]
        if len(slots) != len(set(slots)) or any(s >= UNLOAD_VACANT_SLOT for s in slots):
            return 'slots'
        for g, grp in self.groups.items():
            if any(self.slot_group[s] != g for s in grp['slots']):
                return 'slot table'
        return None


def check_unload_model():
    """--unload-model (check 15): the unload allocators over join / leave sequences with the disc's car sizes."""
    import random
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN, LEVELS
    disc = Disc(DEFAULT_BIN)
    cars = []
    for p in sorted(disc.files):
        name = p.split('/')[-1][:-4]
        if not (p.startswith('/DMD/') and p.endswith('.DMD')) or name in LEVELS:
            continue
        data = disc.get(p)
        ids = dbscan_ids(Dmd(name, data))
        roots = [i for i in ids if 16 <= i <= 240 and i % 16 == 0]
        if roots:
            cars.append((name, roots[0], len(data), len(roots)))
    aligned = all(c[2] % 4 == 0 for c in cars)
    print('  car DMDs: %d, %d..%d bytes, %s' % (len(cars), min(c[2] for c in cars), max(c[2] for c in cars),
                                               'all word-sized' if aligned else 'NOT all word-sized'))
    ok = aligned
    scenarios = [('steady 16 (each leave + join)', 16, 16, 20000, 0), ('churn 8..32 groups', 8, 32, 20000, 0),
                 ('full 32 (each leave + join)', 32, 32, 20000, 0),
                 ('churn 16..40, half Dark Tooth', 16, 40, 20000, 1)]
    big = max(cars, key=lambda c: c[2])

    def pick(rnd, heavy):
        return big if heavy and rnd.random() < 0.5 else rnd.choice(cars)
    for label, lo, hi, events, heavy in scenarios:
        rnd = random.Random(0x7E2 + lo * 100 + hi)
        m = UnloadModel()
        bad = None
        for _ in range(lo):                     # the match load: packed from the area's start
            m.load(*pick(rnd, heavy)[1:])
        for ev in range(events):
            live = sorted(m.groups)
            if lo == hi or (len(live) >= hi or (len(live) > lo and rnd.random() < 0.5)):
                m.unload(rnd.choice(live))
            if len(m.groups) < hi:
                m.load(*pick(rnd, heavy)[1:])
            bad = m.check()
            if bad:
                break
            for typ in {c[1] for c in cars}:     # a type no group has gets its type key
                if not any(g['type'] == typ for g in m.groups.values()) and m.next_key(typ) != typ:
                    bad = 'type key %X' % typ
            if bad:
                break
        st = m.stats
        good = bad is None
        ok = ok and good
        print('  %-32s %-4s %d loads, %d unloads, refused %s (by fragmentation alone %d, the largest block then %s), '
              'most holes %d, groups used %d, keys handed out %d -> %s' % (
                  label, 'ok' if good else 'FAIL', st['loads'], st['unloads'], st['refused'] or 'never',
                  st['frag_refused'], st['min_largest_at_refusal'], st['max_holes'], m.next_group - 1, m.dup_count,
                  'ok' if good else 'FAIL: %s' % bad))
    # the worst case of first fit (no compaction: nothing in the area may move): the area filled with the smallest
    # and the largest car file in turn until it is full, every small one unloaded, then the largest file again: the
    # free bytes stranded in holes too small for it
    small = min(cars, key=lambda c: c[2])
    large = max(cars, key=lambda c: c[2])
    m = UnloadModel()
    gs = []
    while len(gs) < UNLOAD_GROUPS - 2:
        c = small if len(gs) % 2 == 0 else large
        g = m.load(c[1], c[2], c[3])
        if g is None:
            break
        gs.append((g, c))
    for g, c in gs:
        if c is small:
            m.unload(g)
    free = sum(z for _, z in m.ranges) + m.end - m.next
    fits = m.find(large[2]) is not None
    print('  worst case: %s (%d bytes) and %s (%d bytes) in turn until the area is full (%d files), every %s unloaded: '
          '%d bytes free in %d holes + a %d-byte tail; %s %s' % (
              small[0], small[2], large[0], large[2], len(gs), small[0], free, len(m.ranges), m.end - m.next,
              large[0], 'fits' if fits else 'does not fit (fragmentation: refused with TM2_DROPIN_ERR_NOLOAD)'))
    print('unload model: %s' % ('ok' if ok else 'FAIL'))
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', nargs='?', help='port variant link directory (build_source_only.py --output)')
    ap.add_argument('--expect-functions', action='store_true',
                    help='also compare the functions the port define changed with the plan list')
    ap.add_argument('--dmd', action='store_true', help='check the car DMDs on the disc: root first')
    ap.add_argument('--key-model', action='store_true', help='check the key allocation on model load orders')
    ap.add_argument('--bank-model', action='store_true',
                    help='check the banked placement on every car TMS and arena of the disc (host model)')
    ap.add_argument('--clut-model', action='store_true',
                    help='check the per-car CLUT copies on every arena of the disc (host model) and print the room')
    ap.add_argument('--unload-model', action='store_true',
                    help='check the unload allocators (DMD area, keys, slots, groups) over long join/leave sequences '
                         'with the disc car sizes (host model) and print the fragmentation')
    ap.add_argument('--negative-control', action='store_true',
                    help='run check 1 on a build without TM2_PORT_CAR_GROUPS (it must fail)')
    a = ap.parse_args()
    ok = True
    if a.dmd:
        ok = check_dmd() and ok
    if a.key_model:
        ok = check_key_model() and ok
    if a.bank_model:
        ok = check_bank_model() and ok
    if a.clut_model:
        ok = check_clut_model() and ok
    if a.unload_model:
        ok = check_unload_model() and ok
    if not a.out:
        print('CHECK CAR GROUPS %s' % ('OK' if ok else 'FAILED'))
        sys.exit(0 if ok else 1)
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    pv = report.get('port_variant') or {}
    defines = pv.get('port_defines') or []
    names = {d.split('=')[0] for d in defines}
    groups = 'TM2_PORT_CAR_GROUPS' in names
    if not groups and a.negative_control:
        print('note: %s was built without TM2_PORT_CAR_GROUPS (port defines %s): expect a failure' % (a.out, defines))
    if groups or a.negative_control:
        ok = check_tables(out) and ok
    else:
        print('retail per-type tables: not checked (no TM2_PORT_CAR_GROUPS)')
    if a.expect_functions:
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
        want = (EXPECTED if groups else set()) | (EXPECTED_CARS if 'TM2_PORT_CARS' in names else set())
        want |= EXPECTED_BANKS if 'TM2_PORT_TEXTURE_BANKS' in names else set()
        want |= EXPECTED_SPECIALS if 'TM2_PORT_CAR_SPECIALS' in names else set()
        want |= EXPECTED_DARK_TOOTH if 'TM2_PORT_DARK_TOOTH' in names else set()
        want |= EXPECTED_SOUND if 'TM2_PORT_CAR_SOUND' in names else set()
        want |= {'aiDarkToothBossUpdate'} if {'TM2_PORT_CAR_SOUND', 'TM2_PORT_DARK_TOOTH'} <= names else set()
        want |= EXPECTED_CLUTS if 'TM2_PORT_CAR_CLUTS' in names else set()
        want |= EXPECTED_DROPIN if 'TM2_PORT_CAR_DROPIN' in names else set()
        want |= EXPECTED_SPECIAL_AS if 'TM2_PORT_SPECIAL_AS' in names else set()
        want |= EXPECTED_DT_PLAYER if 'TM2_PORT_DT_PLAYER' in names else set()
        want |= DT_PLAYER_STRIDE if 'TM2_PORT_DT_PLAYER' in names and 'TM2_PORT_CARS' not in names else set()
        others = sorted(names - KNOWN_DEFINES)
        missing, extra = want - changed, changed - want
        good = not missing and (not extra or bool(others))
        print('port_changed functions: %d (expected %d)%s; missing %s; extra %s -> %s' % (
            len(changed), len(want), ' with other port defines %s' % others if others else '',
            sorted(missing), sorted(extra), 'ok' if good else 'FAIL'))
        ok = ok and good
    if 'TM2_PORT_CAR_SWAP' in names:
        ok = check_swap(out) and ok
    if 'TM2_PORT_TEXTURE_BANKS' in names:
        ok = check_banks(out) and ok
    dark_tooth = 'TM2_PORT_DARK_TOOTH' in names
    special_as = 'TM2_PORT_SPECIAL_AS' in names
    if 'TM2_PORT_CAR_SPECIALS' in names and (dark_tooth or not a.negative_control):
        ok = check_specials(out, dark_tooth=dark_tooth, special_as=special_as) and ok
    elif 'TM2_PORT_CAR_SPECIALS' in names:
        print('note: TM2_PORT_CAR_SPECIALS without TM2_PORT_DARK_TOOTH: check 8 with the Dark Tooth state must fail '
              '(negative control)')
        ok = check_specials(out, dark_tooth=True, dark_tooth_defined=False) and ok
    elif a.negative_control:
        print('note: no TM2_PORT_CAR_SPECIALS: check 8 must fail (negative control)')
        ok = check_specials(out, defined=False, dark_tooth=True, dark_tooth_defined=False) and ok
    if 'TM2_PORT_CAR_CLUTS' in names or dark_tooth:
        ok = check_cluts(out, 'TM2_PORT_CAR_CLUTS' in names, dark_tooth) and ok
    if 'TM2_PORT_CAR_DROPIN' in names:
        ok = check_dropin(out, names) and ok
    if special_as:
        ok = check_special_as(out, names) and ok
    elif a.negative_control and 'TM2_PORT_CAR_SPECIALS' in names:
        print('note: no TM2_PORT_SPECIAL_AS: check 13 must fail (negative control)')
        ok = check_special_as(out, names, defined=False) and ok
    if 'TM2_PORT_DT_PLAYER' in names:
        ok = check_dt_player(out) and ok
    elif a.negative_control and dark_tooth:
        print('note: no TM2_PORT_DT_PLAYER: check 14 must fail (negative control)')
        ok = check_dt_player(out, defined=False) and ok
    if dark_tooth:
        ok = check_park_head(out, names) and ok
    elif a.negative_control and 'TM2_PORT_CAR_SPECIALS' in names:
        print('note: no TM2_PORT_DARK_TOOTH: check 16 must fail (negative control)')
        ok = check_park_head(out, names, defined=False) and ok
    if 'TM2_PORT_CAR_SOUND' in names:
        ok = check_sound(out, dark_tooth=dark_tooth) and ok
    elif a.negative_control and groups:
        print('note: no TM2_PORT_CAR_SOUND: check 9 must fail (negative control)')
        ok = check_sound(out, defined=False) and ok
    print('CHECK CAR GROUPS %s' % ('OK' if ok else 'FAILED'))
    sys.exit(0 if ok else 1)


def check_sound(out, defined=True, dark_tooth=False):
    """Check 9 (TM2_PORT_CAR_SOUND, CAR_GROUPS.md 12): the per-car sounds are played, stopped and tested by id
    only in the port functions; the changed functions call their hooks."""
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    rows = functions(out)
    link = Link(out)
    relocs, _ = link.relocations()
    ids = SOUND_IDS | (SOUND_SWOOPS if dark_tooth else set())
    funcs = set(SOUND_HOOKS) | ({'aiDarkToothBossUpdate'} if dark_tooth else set())
    calls, bad, nonconst = {}, [], {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is None or rtype != 4:
            continue
        callee = owner(rows, v_c) or sym
        calls.setdefault(fn, set()).add(callee)
        if callee in SOUND_LOOP_API and kinds.get(fn) != 'port':
            bad.append('%s -> %s' % (fn, callee))   # request 33-4: a human loop by side outside the port code
            continue
        if callee not in SOUND_API or kinds.get(fn) == 'port':
            continue
        a0 = a0_constant(link, p_c)
        if a0 is None:
            nonconst.setdefault(fn, set()).add(callee)
            if fn in funcs and callee not in SOUND_NONCONST_OK.get(fn, ()):
                bad.append('%s -> %s(<not a constant>)' % (fn, callee))
        elif a0 in ids:
            bad.append('%s -> %s(%d)' % (fn, callee, a0))
    hooks = dict(SOUND_HOOKS, **({'aiDarkToothBossUpdate': {'portCarSoundPlay'}} if dark_tooth else {}))
    hooks['carSoundUpdate'] = hooks['carSoundUpdate'] | SOUND_HOOKS_CAR_SOUND_UPDATE_33_4
    missing_hooks = sorted('%s -> %s' % (f, h) for f, hs in hooks.items() for h in hs if h not in calls.get(f, ()))
    ok = not bad
    if defined:
        missing = sorted(n for n in SOUND_PORT if kinds.get(n) != 'port')
        rand_callers = sorted(f for f in SOUND_PORT if 'rand' in calls.get(f, ()))
        ok = ok and not missing and not rand_callers and not missing_hooks
        print('per-car sound: %d port functions (missing %s); rand() called by %s; hooks missing %s' % (
            len(SOUND_PORT) - len(missing), missing, rand_callers or 'none', missing_hooks or 'none'))
    print('per-car sound ids %s and the human loops by side: calls outside the port code: %s -> %s' % (
        ' '.join('%d' % i for i in sorted(ids)), '; '.join(bad) or 'none', 'ok' if ok else 'FAIL'))
    print('  game functions calling the sound API with a non-constant id (not per-car paths): %s' % (
        ' '.join(sorted(f for f in nonconst if f not in funcs)) or 'none'))
    return ok


def check_swap(out):
    """Check 5: the swap's port functions exist, sessionBindCars calls portCarBindGroupOf/TypeOf, and no port
    function the swap reaches calls rand() itself. The game functions it calls are listed with the ones of
    them that can reach rand() (as at the same events in the game: a held car let go, a spin-out)."""
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    missing = sorted(n for n in SWAP_PORT if kinds.get(n) != 'port')
    rows = functions(out)
    link = Link(out)
    relocs, _ = link.relocations()
    calls = {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is not None and rtype == 4:   # R_MIPS_26: jal / j
            calls.setdefault(fn, set()).add(owner(rows, v_c) or sym)

    def reach(starts, follow):
        seen, todo = set(), list(starts)
        while todo:
            fn = todo.pop()
            if fn in seen:
                continue
            seen.add(fn)
            todo.extend(c for c in calls.get(fn, ()) if follow(c))
        return seen

    port = reach([n for n in SWAP_PORT if n in kinds], lambda c: kinds.get(c) == 'port')
    direct_rand = sorted(f for f in port if 'rand' in calls.get(f, ()))
    game = sorted({c for f in port for c in calls.get(f, ()) if c in kinds and kinds[c] != 'port'})
    via_rand = sorted(g for g in game
                      if any('rand' in calls.get(x, ()) for x in reach([g], lambda c: c in kinds and c != 'rand')))
    binds = calls.get('sessionBindCars', set())
    bind_ok = {'portCarBindGroupOf', 'portCarBindTypeOf'} <= binds
    good = not missing and not direct_rand and bind_ok
    print('car swap: %d port functions (missing %s); %d port functions reached, rand() called by %s; '
          'sessionBindCars -> portCarBindGroupOf/TypeOf %s -> %s' % (
              len(SWAP_PORT) - len(missing), missing, len(port), direct_rand or 'none',
              'yes' if bind_ok else 'NO', 'ok' if good else 'FAIL'))
    print('  game functions called by the swap code (%d): %s' % (len(game), ' '.join(game)))
    print('  of them may reach rand() inside the game: %s' % (' '.join(via_rand) or 'none'))
    return good


def check_remap_source():
    """Every write of a primitive's tpage page bits in src/semantic is followed, under
    TM2_PORT_TEXTURE_BANKS, by the write of the bank into its bits 9..15."""
    import re
    writes, good = 0, True
    for path in sorted((ROOT / 'src' / 'semantic').glob('*.c')):
        lines = path.read_text(encoding='latin-1').splitlines()
        for i, line in enumerate(lines):
            if re.search(r'\(\(UVTpWord \*\)\s*\w+\)->tpage\s*=', line):
                writes += 1
                after = lines[i + 1:i + 9]
                ifdef = any(l.strip() == '#ifdef TM2_PORT_TEXTURE_BANKS' for l in after)
                rest = any(re.search(r'->rest\s*=.*>>\s*9', l) for l in after)
                if not (ifdef and rest):
                    good = False
                    print('  %s:%d: tpage write without the bank write' % (path.name, i + 1))
    print('remap writes of the tpage page bits: %d, each followed by the bank write: %s' % (
        writes, 'ok' if good and writes else 'FAIL'))
    return good and writes > 0


def check_banks(out):
    """Check 6 (TM2_PORT_TEXTURE_BANKS): port functions, the remap's bank write, the upload word protocol."""
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    missing = sorted(n for n in BANKS_PORT if kinds.get(n) != 'port')
    print('texture banks: %d port functions (missing %s) -> %s' % (
        len(BANKS_PORT) - len(missing), missing, 'ok' if not missing else 'FAIL'))
    ok = not missing
    ok = check_remap_source() and ok
    link = Link(out)
    rows = functions(out)
    names_at = {}
    for name, s in link.symbols.items():
        names_at.setdefault(s['address'], set()).add(name)
    word = link.symbols['portVramUploadBank']['address']
    tb = link.symbols['portTexBanks']['address']
    relocs, _ = link.relocations()
    events = {}
    remap_reads = False
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn == 'modelRemapTextures' and tb <= v_c < tb + 0x4D0:
            remap_reads = True
        if rtype == 6 and v_c == word:       # R_MIPS_LO16 of the word: sw rt sets it, sw $zero resets it
            w = link.word(p_c)
            if w >> 26 == 0x2B:
                events.setdefault(fn, []).append((p_c, 'zero' if (w >> 16) & 31 == 0 else 'set'))
            else:
                events.setdefault(fn, []).append((p_c, 'other %08X' % w))
        elif rtype == 4 and names_at.get(v_c, set()) & UPLOAD_FUNCS:
            events.setdefault(fn, []).append((p_c, 'upload'))
    print('modelRemapTextures reads portTexBanks (the live switch): %s' % ('ok' if remap_reads else 'FAIL'))
    ok = ok and remap_reads
    setters = {f for f, ev in events.items() if any(k != 'upload' for _, k in ev)}
    good_set = setters == UPLOAD_SETTERS
    print('functions storing portVramUploadBank: %s -> %s' % (
        ' '.join(sorted(setters)), 'ok' if good_set else 'FAIL (expected %s)' % ' '.join(sorted(UPLOAD_SETTERS))))
    ok = ok and good_set
    for fn in sorted(setters):
        ev = sorted(events[fn])
        state, pending, good, uploads = 'zero', False, True, 0
        for _, k in ev:
            if k == 'set':
                if pending:
                    good = False   # a new bank before the last upload's reset
                state = 'set'
            elif k == 'zero':
                state, pending = 'zero', False
            elif k == 'upload':
                uploads += 1
                if state != 'set' or pending:
                    good = False   # an upload without its bank (or two under one store)
                pending = True
            else:
                good = False       # the word is used other than by stores
        good = good and not pending and state == 'zero' and uploads > 0
        print('  %-20s %d uploads, %s -> %s' % (fn, uploads, ' '.join(k for _, k in ev), 'ok' if good else 'FAIL'))
        ok = ok and good
    return ok


def check_bank_model():
    """Check 7: the banked placement (vramPlaceTexture / vramUploadClut / portTexBank*) on the disc's files."""
    from dmd_lod_dump import Disc, DEFAULT_BIN, LEVELS
    import struct
    disc = Disc(DEFAULT_BIN)

    def tms(path):
        data = disc.get(path)
        magic, ver, _res, count = struct.unpack_from('<4I', data, 0)
        assert magic == 0x50535854 and ver == 0x43, path
        keys = struct.unpack_from('<%dI' % count, data, 16)
        p, out = 16 + 4 * count, []
        for i in range(count):
            size = struct.unpack_from('<I', data, p)[0]
            t = p + 4
            flags = struct.unpack_from('<I', data, t + 4)[0]
            q = t + 8
            if flags & 8:
                q += struct.unpack_from('<I', data, q)[0]
            pw, ph = struct.unpack_from('<HH', data, q + 8)
            out.append((keys[i], flags & 7, bool(flags & 8), pw, ph))
            p = t + size
        return out

    reserved = [(0, 0, 640, 256), (960, 256, 64, 32), (960, 288, 64, 128), (960, 416, 32, 96)]

    def place(images, level_keys):
        rects = list(reserved)

        def free(x, y, w, h):
            if x + w > 1024 or y + h > 512 or len(rects) >= 72:
                return False
            return not any(x < rx + rw and rx < x + w and y < ry + rh and ry < y + h for rx, ry, rw, rh in rects)

        def clut(w):
            for py in (0, 256):
                for px in (0, 256, 512, 768):
                    for y in range(py, py + 256):
                        for x in range(px, px + 256, w):
                            if free(x, y, w, 1):
                                rects.append((x, y, w, 1))
                                return x, y
            return None
        placed, problems = [], []
        for key, mode, hasclut, w, h in images:
            if key in level_keys:
                continue                     # the arena's copy, bank 0
            spot, result, pas = None, False, 0
            while True:
                start = 0x3C0 if pas == 0 else 0
                pas += 1
                for px in range(start, 1024, 256):
                    for x in range(px, px + 256, w):
                        for y in range(256, 512, h):
                            if spot is None and free(x, y, w, h):
                                rects.append((x, y, w, h))
                                spot, result = (x, y), True
                                if hasclut and mode in (0, 1):
                                    c = clut(16 if mode == 0 else 256)
                                    result = c is not None
                                    if c and c[0] % 16:
                                        problems.append('CLUT x %d' % c[0])
                if pas >= 2 or result:
                    break
            if not result:
                problems.append('key %X (%dx%d) not placed' % (key, w, h))
                continue
            x, y = spot
            if x % 64 + w > 64 and (mode == 0 or w <= 64):
                problems.append('key %X crosses a texture page' % key)
            placed.append((x, y, w, h))
        body = [r for r in rects if r not in reserved]
        for i in range(len(body)):
            for j in range(i + 1, len(body)):
                a, b = body[i], body[j]
                if a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and a[1] < b[1] + b[3] and b[1] < a[1] + a[3]:
                    problems.append('overlap %s %s' % (a, b))
        return placed, body, problems
    cars = ['HH', 'OT', 'WH', 'MG', 'PV', 'TH', 'SP', 'RK', 'IR', 'AX', 'FL', 'HS', 'ST', 'MN', 'BB']
    car_tms = {c: tms('/TMS/%s.TMS' % c) for c in cars}
    ok, banks, worst = True, 0, (0, '')
    for lv in LEVELS:
        level_keys = {k for k, *_ in tms('/TMS/%s.TMS' % lv)}
        shared, good = 0, True
        for c in cars:
            placed, body, problems = place(car_tms[c], level_keys)
            banks += 1
            shared += len(car_tms[c]) - len(placed)
            texels = sum(w * h for _, _, w, h in body)
            worst = max(worst, (texels, '%s on %s' % (c, lv)))
            if problems:
                good = False
                print('  %-8s %-2s FAIL %s' % (lv, c, '; '.join(problems[:4])))
        ok = ok and good
        print('  %-8s 15 cars, each in a bank of its own: %s (images left in bank 0 by arena key: %d)' % (
            lv, 'ok' if good else 'FAIL', shared))
    print('bank model: %d banks filled, largest %d texels (%s): %s' % (banks, worst[0], worst[1], 'ok' if ok else 'FAIL'))
    return ok


def check_cluts(out, cluts, dark_tooth):
    """Check 10 (TM2_PORT_CAR_CLUTS and / or TM2_PORT_DARK_TOOTH, CAR_GROUPS.md 13): port functions, hooks, the rows
    240..255 reservation, who touches portCarCluts."""
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    want_port = set(CLUTS_PORT if cluts else ROWS_PORT) | ({'portClutRowsUpload'} if dark_tooth else set())
    want_calls = {f: {h} for f, h in CLUTS_CALLS.items()} if cluts else {'bitmapInit': {'portCarClutsReset'}}
    if cluts:
        want_calls['vramUploadClut'] = {'portClutSpotFree', 'portClutRowsLast'}   # bit 9 (CAR_GROUPS.md 13.12)
        # bit 9 for images too (request 41 item B)
        want_calls['vramPlaceTexture'] = {'portCarClutNote', 'portImagePasses', 'portImageSpotFree'}
    if dark_tooth:
        want_calls.setdefault('vramUploadClut', set()).add('portClutRowsUpload')
    rows = functions(out)
    link = Link(out)
    relocs, _ = link.relocations()
    cc = link.symbols['portCarCluts']['address']
    calls, users, marks = {}, set(), []
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is None:
            if cc <= v_c < cc + CLUTS_SIZE:
                users.add('data@%08X' % p_c)
            continue
        if cc <= v_c < cc + CLUTS_SIZE:
            users.add(fn)
        if rtype == 4:
            callee = owner(rows, v_c) or sym
            calls.setdefault(fn, set()).add(callee)
            if fn == 'bitmapInit' and callee == 'bitmapMarkRect':
                marks.append((p_c, a0_constant(link, p_c, 6), a0_constant(link, p_c, 7)))
    missing = sorted(n for n in want_port if kinds.get(n) != 'port')
    rand_callers = sorted(f for f in want_port if 'rand' in calls.get(f, ()))
    hooks = sorted('%s -> %s' % (f, h) for f, hs in want_calls.items() for h in hs if h not in calls.get(f, ()))
    marks.sort()
    reserve = bool(marks) and marks[0][1] == 640 and marks[0][2] == 256
    bad_users = sorted(u for u in users if kinds.get(u) != 'port' and u != 'clutAnimRegister')
    ok = not missing and not rand_callers and not hooks and reserve and not bad_users
    print('per-car CLUTs%s: %d port functions (missing %s); rand() called by %s; hooks missing %s' % (
        '' if cluts else ' (rows 240..255 only, Dark Tooth)', len(want_port) - len(missing), missing,
        rand_callers or 'none', hooks or 'none'))
    print('  bitmapInit reserves x 0..639 rows 0..255 (first bitmapMarkRect $a2 %s, $a3 %s): %s; game functions '
          'relocating into portCarCluts: %s -> %s' % (
              marks[0][1] if marks else None, marks[0][2] if marks else None, 'ok' if reserve else 'FAIL',
              ' '.join(sorted(u for u in users if kinds.get(u) != 'port')) or 'none', 'ok' if ok else 'FAIL'))
    return ok


def check_clut_model(verbose=False):
    """Check 11: the per-car CLUT copies (request 33-1) and the rows 240..255 allocator on the disc's files.

    A host model of a load as the C runs it (bitmapInit's reservations, the level TMS at its fixed places,
    vramPlaceTexture / vramUploadClut with retail's top-left tests, bit 6's fallback into rows 240..255,
    portCarClutNote, portCarClutsCopyAll with portClutRowsAlloc and the whole-rectangle scan; texture slot
    stride 32 as with TM2_PORT_CARS). Scenarios on each of the 14 arena TMS files: 16 cars of one type (each of
    the 14 normal types), 4 teams of 4 (4 types x 4 cars, TEAM_SETS), each with bit 7; then the same with bit 6
    and a Dark Tooth as the 16th car (loaded last). Checked: no copy overlaps anything placed or another copy, every
    copy is inside VRAM on a 16-texel boundary, rows 240..255 stay within x 0..639 and are taken only by the
    allocator, and every original (the arena's, the first car of each key) keeps the position it has with bit 7
    clear (a copy never moves or blocks a retail placement)."""
    from dmd_lod_dump import Disc, DEFAULT_BIN, LEVELS
    import struct
    disc = Disc(DEFAULT_BIN)
    W, H = 1024, 512

    def tms(path):
        data = disc.get(path)
        magic, ver, _res, count = struct.unpack_from('<4I', data, 0)
        assert magic == 0x50535854 and ver == 0x43, path
        keys = struct.unpack_from('<%dI' % count, data, 16)
        p, out = 16 + 4 * count, []
        for i in range(count):
            size = struct.unpack_from('<I', data, p)[0]
            t = p + 4
            flags = struct.unpack_from('<I', data, t + 4)[0]
            q, clut = t + 8, None
            if flags & 8:
                blen, cx, cy, cw, ch = struct.unpack_from('<IHHHH', data, q)
                clut = (cx, cy, cw, ch)
                q += blen
            _blen, px, py, pw, ph = struct.unpack_from('<IHHHH', data, q)
            out.append((keys[i], flags & 7, bool(flags & 8), clut, (px, py, pw, ph)))
            p = t + size
        return out

    class Load:
        """One load: the usage bitmap (a byte per texel, retail's bit index (y << 10) + x, so x >= 1024 runs
        into the next row), the records, the slots, the rows allocator and the copy notes."""

        def __init__(self, bit6, bit7, bit9=False):
            self.bits = bytearray(W * H)
            self.bit6, self.bit7, self.bit9 = bit6, bit7, bit9
            self.img_overlaps = []              # images whose rectangle took used texels (top-left test, retail)
            self.img_fallback = []              # bit 9: images no whole-rectangle pass placed (retail's passes ran)
            self.records = []                   # (key, clut (x, y) or None, group)
            self.rows = [[False] * 40 for _ in range(16)]
            self.row_owner = {}                 # (x, y) -> 'boss' / 'copy'
            self.slots = {}                     # (group, image) -> clut (x, y)
            self.pend = {}                      # (group, image) -> (width, arena)
            self.state = {}                     # (group, image) -> own / copy / shared
            self.failed_orig, self.copies, self.failed = [], [], []
            self.overlaps = []
            # every rectangle bitmapInit reserved (the frame buffers without rows 240..255, which are the
            # allocator's; the fonts), the level and the originals took (x, y, w, h)
            self.placed = [(0, 0, 640, 240), (960, 256, 64, 32), (960, 288, 64, 128), (960, 416, 32, 96)]
            for r in ((0, 0, 640, 256), (960, 256, 64, 32), (960, 288, 64, 128), (960, 416, 32, 96)):
                self.mark(*r)

        def free(self, x, y):
            b = (y << 10) + x
            return 0 <= b < W * H and self.bits[b] == 0

        def mark(self, x, y, w, h):
            for r in range(y, y + h):
                b = (r << 10) + x
                self.bits[max(b, 0):min(b + w, W * H)] = b'\x01' * (min(b + w, W * H) - max(b, 0))

        def level(self, images):
            for key, mode, hasclut, clut, (px, py, pw, ph) in images:
                self.mark(px, py, pw, ph)
                self.placed.append((px, py, pw, ph))
                if hasclut:
                    self.mark(*clut)
                    self.placed.append(clut)
                self.records.append((key, (clut[0], clut[1]) if hasclut else None, 0))
            self.level_records = len(self.records)

        def rows_alloc(self, w, who):
            if w == 256:
                for half in (0, 1):
                    for r in range(16):
                        if not any(self.rows[r][half * 16:half * 16 + 16]):
                            for u in range(half * 16, half * 16 + 16):
                                self.rows[r][u] = True
                            self.row_owner[(half * 256, 240 + r)] = who
                            return half * 256, 240 + r
                return None
            for r in range(16):
                for u in range(32, 40):
                    if not self.rows[r][u]:
                        self.rows[r][u] = True
                        self.row_owner[(u * 16, 240 + r)] = who
                        return u * 16, 240 + r
            for r in range(15, -1, -1):
                for u in range(31, -1, -1):
                    if not self.rows[r][u]:
                        self.rows[r][u] = True
                        self.row_owner[(u * 16, 240 + r)] = who
                        return u * 16, 240 + r
            return None

        def upload_clut(self, w):
            for py in (0, 256):
                if py == 256 and self.bit6:
                    spot = self.rows_alloc(w, 'boss')
                    if spot:
                        self.placed.append((spot[0], spot[1], w, 1))
                        return spot
                for px in range(0, 1024, 256):
                    for y in range(py, py + 256):
                        for x in range(px, px + 256, w):
                            b = (y << 10) + x
                            if self.bit9:       # portClutSpotFree: every texel of the spot
                                ok = x + w <= 1024 and not any(self.bits[b:b + w])
                            else:
                                ok = self.free(x, y)
                            if ok:
                                if any(self.bits[b:b + w]) or x + w > 1024:
                                    self.overlaps.append((x, y, w))   # retail tests the top-left texel only
                                self.mark(x, y, w, 1)
                                self.placed.append((x, y, w, 1))
                                return x, y
            if self.bit9:                       # portClutRowsLast: rows 240..255 as the last resort
                spot = self.rows_alloc(w, 'scan')
                if spot:
                    self.placed.append((spot[0], spot[1], w, 1))
                    return spot
            return None

        def car(self, group, images):
            first = len(self.records)
            for image, (key, mode, hasclut, _clut, (_px, _py, w, h)) in enumerate(images):
                known = next((k for k, r in enumerate(self.records) if r[0] == key), None)
                cl = hasclut and mode in (0, 1)
                if known is not None:
                    c = self.records[known][1]
                    self.slots[(group, image)] = c
                    if cl:
                        if known >= first:
                            self.state[(group, image)] = 'own'
                        else:
                            self.state[(group, image)] = 'shared'
                            if self.bit7:
                                self.pend[(group, image)] = (16 if mode == 0 else 256, known < self.level_records)
                    self.records.append((key, c, group))
                    continue
                spot, c = None, None
                # vramPlaceTexture: retail's two passes (top-left test); bit 9 (portImagePasses /
                # portImageSpotFree, request 41 item B): two passes testing the whole rectangle, then retail's
                for pass_, start in enumerate((0x3C0, 0, 0x3C0, 0) if self.bit9 else (0x3C0, 0), 1):
                    strict = self.bit9 and pass_ <= 2
                    if pass_ == 3:
                        self.img_fallback.append((group, image))
                    for px in range(start, 1024, 256):
                        for x in range(px, px + 256, w):
                            for y in range(256, 512, h):
                                if strict:
                                    ok = x + w <= 1024 and y + h <= 512 and not any(
                                        any(self.bits[(r << 10) + x:(r << 10) + x + w]) for r in range(y, y + h))
                                else:
                                    ok = self.free(x, y)
                                if ok:
                                    spot = (x, y)
                                    break
                            if spot:
                                break
                        if spot:
                            break
                    if spot:
                        break
                if spot is None:
                    self.failed_orig.append((group, image, 'pixels'))
                else:
                    if any(any(self.bits[(r << 10) + spot[0]:(r << 10) + spot[0] + w]) for r in range(spot[1], spot[1] + h)):
                        self.img_overlaps.append((group, image, spot))
                    self.mark(spot[0], spot[1], w, h)
                    self.placed.append((spot[0], spot[1], w, h))
                    if cl:
                        c = self.upload_clut(16 if mode == 0 else 256)
                        if c is None:
                            self.failed_orig.append((group, image, 'CLUT'))
                if cl:
                    self.state[(group, image)] = 'own'
                self.slots[(group, image)] = c
                self.records.append((key, c, group))

        def scan(self, w):
            for py in (0, 256):
                for px in range(0, 1024, 256):
                    for y in range(py, py + 256):
                        for x in range(px, px + 256, w):
                            b = (y << 10) + x
                            if x + w <= 1024 and not any(self.bits[b:b + w]):
                                self.mark(x, y, w, 1)
                                return x, y
            return None

        def copy_all(self):
            full = {16: False, 256: False}
            groups = sorted({g for g, _ in self.pend})
            for phase in range(4):
                for g in groups:
                    for i in range(32):
                        if (g, i) not in self.pend:
                            continue
                        w, arena = self.pend[(g, i)]
                        want = 3 if arena else 0 if i == 0 else 1 if w == 256 else 2
                        if want != phase:
                            continue
                        del self.pend[(g, i)]
                        spot = None if full[w] else (self.rows_alloc(w, 'copy') or self.scan(w))
                        if spot is None:
                            full[w] = True
                            self.failed.append((g, i, w, arena))
                            continue
                        self.copies.append((g, i, w, spot))
                        self.slots[(g, i)] = spot
                        self.state[(g, i)] = 'copy'

    lv_tms = {lv: tms('/TMS/%s.TMS' % lv) for lv in LEVELS}
    cars = ['HH', 'OT', 'WH', 'MG', 'PV', 'TH', 'SP', 'RK', 'IR', 'AX', 'FL', 'HS', 'ST', 'MN']
    car_tms = {c: tms('/TMS/%s.TMS' % c) for c in cars + ['BB']}

    def run(lv, order, bit6, bit7, bit9=False):
        ld = Load(bit6, bit7, bit9)
        ld.level(lv_tms[lv])
        for g, c in enumerate(order, 1):
            ld.car(g, car_tms[c])
        if bit7:
            ld.copy_all()
        return ld

    def problems_of(ld, base):
        bad = []
        own = {}
        for (g, i), st in ld.state.items():
            c = ld.slots.get((g, i))
            if st in ('own', 'copy') and c:
                own.setdefault(c, []).append((g, i))
        for g, i, w, (x, y) in ld.copies:
            if x % 16 or x + w > 1024 or not 0 <= y < 512:
                bad.append('copy %d/%d at (%d,%d)' % (g, i, x, y))
            if 240 <= y < 256 and (x + w > 640 or ld.row_owner.get((x, y)) != 'copy'):
                bad.append('copy %d/%d in rows 240..255 outside the allocator' % (g, i))
            if len(own.get((x, y), [])) != 1:
                bad.append('copy %d/%d at (%d,%d) also used by %s' % (g, i, x, y, own.get((x, y))))
        # no copy overlaps anything the level or an original took, or another copy (an occupancy map of its own,
        # independent of the model's usage bitmap)
        occ = bytearray(W * H)
        for x, y, w, h in ld.placed:
            for row in range(y, y + h):
                b = (row << 10) + x
                occ[max(b, 0):min(b + w, W * H)] = b'' * (min(b + w, W * H) - max(b, 0))
        for g, i, w, (x, y) in ld.copies:
            b = (y << 10) + x
            if any(occ[b:b + w]):
                bad.append('copy %d/%d at (%d,%d) overlaps' % (g, i, x, y))
            occ[b:b + w] = b'' * w
        # the copies never touch an original: every original keeps its bit-7-clear position
        for k, v in base.slots.items():
            if base.state.get(k) == 'own' and ld.slots.get(k) != v:
                bad.append('original %d/%d moved %s -> %s' % (k[0], k[1], v, ld.slots.get(k)))
        if [f for f in ld.failed_orig] != [f for f in base.failed_orig]:
            bad.append('originals failed %s (bit 7 clear: %s)' % (ld.failed_orig, base.failed_orig))
        # bit 9 (portClutSpotFree, portClutRowsLast): no original CLUT over used texels (with or without the copies)
        if ld.bit9:
            for l in (base, ld):
                if l.overlaps:
                    bad.append('bit 9: CLUT over used texels at %s' % (l.overlaps[:3],))
        return bad

    team_sets = TEAM_SETS
    LARGEST = ['MG', 'IR', 'AX', 'HS', 'ST', 'RK', 'OT', 'PV', 'SP', 'MN', 'HH', 'WH', 'TH', 'FL']

    def table(bit9):
        ok = True
        notes = {}
        print('CLUT copies (request 33-1)%s, per arena: room after the level; 16 of one type (14 types); 4 teams of 4 '
              '(%d sets); bit 6 + Dark Tooth as the 16th car' % (
                  ', portCarOptions bit 9 set (whole-spot CLUT scan, rows 240..255 last)' if bit9 else '',
                  len(team_sets)))
        print('  rows / 16s: whole free 256-texel rows and 16-texel spots after the level (besides rows 240..255: 32 + 128)')
        print('  one: 16 of one type, types whose 15 body copies all fit, the type with the fewest copies; 4x4: sets whose')
        print('  copies all fit; mix: the most different types that fit (largest first) filled up to 16 with duplicates:')
        print('  body copies, all copies; dt0 / dt: 15 of one type + Dark Tooth (last) with bit 6 clear / set: types whose 14')
        print('  body copies fit, most originals failing (with bit 7 clear too: retail room), rows 240..255 spots bit 6 took'
              + ('; scan: most rows 240..255 spots bit 9 took for originals (bit 7 clear)' if bit9 else ''))
        print('  %-8s %4s %5s | %-22s | %-3s | %-27s | %-8s | %s' % ('arena', 'rows', '16s', 'one: bodies, copies', '4x4',
                                                                'mix', 'dt0', 'dt' + (' | scan' if bit9 else '')))
        for lv in LEVELS:
            room = Load(False, False)
            room.level(lv_tms[lv])
            free256 = sum(1 for py in (0, 256) for px in range(0, 1024, 256) for y in range(py, py + 256)
                          if px + 256 <= 1024 and not any(room.bits[(y << 10) + px:(y << 10) + px + 256]))
            free16 = sum(1 for y in range(H) for x in range(0, 1024, 16)
                         if not any(room.bits[(y << 10) + x:(y << 10) + x + 16]))
            res = {}
            mix = list(LARGEST)
            while len(mix) > 1 and run(lv, mix, False, False, bit9).failed_orig:
                mix.pop()
            scan = 0
            for scen in ('one', 'teams', 'mix', 'dt0', 'dt'):
                if scen == 'mix':
                    orders = [('%d types' % len(mix), [mix[k % len(mix)] for k in range(16)])]
                elif scen == 'one':
                    orders = [(c, [c] * 16) for c in cars]
                elif scen == 'teams':
                    orders = [('/'.join(s), [c for c in s for _ in range(4)]) for s in team_sets]
                else:
                    orders = [(c, [c] * 15 + ['BB']) for c in cars]
                bit6 = scen == 'dt'
                stats = []
                for name, order in orders:
                    base = run(lv, order, bit6, False, bit9)
                    ld = run(lv, order, bit6, True, bit9)
                    bad = problems_of(ld, base)
                    if bad:
                        ok = False
                        print('  %-8s %s %s FAIL %s' % (lv, scen, name, '; '.join(bad[:4])))
                    if base.overlaps:
                        notes.setdefault(lv, set()).add('%s %s' % (scen, name))
                    scan = max(scan, sum(1 for v in base.row_owner.values() if v == 'scan'))
                    bodies = sum(1 for g, i, *_ in ld.copies if i == 0)
                    need_b = sum(1 for g in range(2, len(order) + 1) if base.state.get((g, 0)) == 'shared')
                    stats.append((name, bodies, need_b, len(ld.copies), len(ld.copies) + len(ld.failed),
                                  len(base.failed_orig), sum(1 for v in ld.row_owner.values() if v == 'boss')))
                    if verbose:
                        print('    %-8s %-5s %-12s bodies %d/%d copies %d/%d originals failed %d boss rows %d' % (
                            (lv, scen) + stats[-1][:1] + stats[-1][1:]))
                res[scen] = stats
            one = res['one']
            full_one = sum(1 for s in one if s[1] == s[2])
            worst_one = min(one, key=lambda s: (s[1] - s[2], s[3] - s[4]))
            teams = res['teams']
            full_t = sum(1 for s in teams if s[3] == s[4])
            dt = res['dt']
            full_dt = sum(1 for s in dt if s[1] == s[2])
            dt0 = res['dt0']
            full_dt0 = sum(1 for s in dt0 if s[1] == s[2])
            fail0 = max(s[5] for s in dt0)
            fail6 = max(s[5] for s in dt)
            boss = max(s[6] for s in dt)
            mx = res['mix'][0]
            print('  %-8s %4d %5d | %2d/14, least %s %3d/%3d | %d/%d | %-8s bodies %2d/%2d, %3d/%3d | %2d/14 %2d | %2d/14 %2d %d%s' % (
                lv, free256, free16, full_one, worst_one[0], worst_one[3], worst_one[4], full_t, len(teams), mx[0],
                mx[1], mx[2], mx[3], mx[4], full_dt0, fail0, full_dt, fail6, boss, ' | %d' % scan if bit9 else ''))
        for lv in LEVELS:
            if lv in notes:
                print('  note %-8s the retail CLUT scan (top-left texel test) put an original 256-colour CLUT over placed '
                      'texels in the lower half (bit 7 clear too): %s' % (lv, ', '.join(sorted(notes[lv]))))
        return ok

    def types():
        """SIXTEEN_CARS.md 4.4: distinct types added largest first / smallest first (bits 6 and 7 clear): how many
        load before the first failure (retail's count), before the first original CLUT over used texels (retail's
        top-left test), and with bit 9; Mr. Grimm, Twister and Dark Tooth."""
        good = True
        retail_overlaps = 0
        print('Types per arena (SIXTEEN_CARS.md 4.4; bits 6 and 7 clear): distinct types added largest first / smallest')
        print('  first. load/clean/img: types loaded before the first failure (retail\'s count) / before the first')
        print('  original CLUT over used texels (retail\'s top-left test) / before the first CLUT or image over used texels;')
        print('  bit 9: types loaded with the whole-spot scans (CLUTs and, request 41 item B, images; never over used texels')
        print('  outside the image fallback), rows 240..255 spots it took at that count; MG+IR+BB: Mr. Grimm, Twister,')
        print('  Dark Tooth')
        print('  %-8s | %-19s | %-19s | %-12s | %-18s | %-18s | %s' % (
            'arena', 'largest', 'smallest', 'MG+IR+BB', 'bit 9 largest', 'bit 9 smallest', 'bit 9 MG+IR+BB'))
        # (a column is load/clean/img for retail)

        def fits(lv, order, bit9):
            clean, iclean, rows, img = None, None, 0, False
            for k in range(1, len(order) + 1):
                ld = run(lv, order[:k], False, False, bit9)
                img = img or bool(ld.img_overlaps)
                if (ld.overlaps or ld.img_overlaps) and iclean is None and not ld.failed_orig:
                    iclean = k - 1
                if bit9 and ld.img_fallback:
                    fallback.add(lv)
                    if ld.img_overlaps:
                        img_fallback_overlaps.add(lv)
                if ld.failed_orig:
                    return (k - 1, k - 1 if clean is None else clean, ld.failed_orig[0][2], rows, img,
                            k - 1 if iclean is None else iclean)
                if ld.overlaps and clean is None:
                    clean = k - 1
                rows = sum(1 for v in ld.row_owner.values() if v == 'scan')
            return len(order), len(order) if clean is None else clean, '', rows, img, len(order) if iclean is None else iclean

        img_notes = []
        fallback, img_fallback_overlaps = set(), set()
        for lv in LEVELS:
            cols = []
            for bit9 in (False, True):
                for order in (LARGEST, LARGEST[::-1]):
                    n, clean, why, rows, img, iclean = fits(lv, order, bit9)
                    if img:
                        img_notes.append(lv + (' (bit 9)' if bit9 else ''))
                        if bit9 and lv not in img_fallback_overlaps:
                            good = False        # bit 9: an image over used texels without the fallback
                    if bit9:
                        good = good and clean == n
                        cols.append('%2d%s, rows %d' % (n, ' (%s)' % why if why else '', rows))
                    else:
                        retail_overlaps += clean != n
                        cols.append('%2d/%2d/%2d%s' % (n, clean, iclean, ' (%s)' % why if why else ''))
                ld = run(lv, ['MG', 'IR', 'BB'], False, False, bit9)
                if bit9 and (ld.overlaps or (ld.img_overlaps and not ld.img_fallback)):
                    good = False
                if bit9 and ld.img_fallback:
                    fallback.add(lv + ' MG+IR+BB')
                    if ld.img_overlaps:
                        img_fallback_overlaps.add(lv + ' MG+IR+BB')
                if ld.img_overlaps:
                    img_notes.append(lv + (' (bit 9)' if bit9 else '') + ' MG+IR+BB')
                cols.append('fails (%s)' % ld.failed_orig[0][2] if ld.failed_orig else
                            'overwrites' if ld.overlaps else 'ok')
            print('  %-8s | %-19s | %-19s | %-12s | %-18s | %-18s | %s' % ((lv,) + tuple(cols)))
        if img_notes:
            print("  note: an image placement (retail's top-left test) took used texels: %s" %
                  ', '.join(sorted(set(img_notes))))
        print("  bit 9: images placed by the fallback (no whole free rectangle; retail's passes): %s; of those over "
              "used texels: %s" % (', '.join(sorted(fallback)) or 'none', ', '.join(sorted(img_fallback_overlaps)) or 'none'))
        print('  bit 9: no original CLUT or image (outside the fallback) over used texels: %s; negative control: with '
              'bit 9 clear %d arena / order runs have a CLUT over used texels: %s; images over used texels with bit 9 '
              'clear: %s' % ('ok' if good else 'FAIL', retail_overlaps, 'ok' if retail_overlaps else 'FAIL',
                             ', '.join(sorted(set(n for n in img_notes if '(bit 9)' not in n))) or 'none'))
        return good and retail_overlaps > 0

    ok = table(False)
    ok = table(True) and ok
    ok = types() and ok
    print('CLUT model: %s' % ('ok' if ok else 'FAIL'))
    return ok

def a0_constant(link, p_c, reg=4):
    """The constant a jal at p_c passes in $a0 (or argument register `reg`: addiu/ori reg, $zero, imm in its delay
    slot or in the 8 instructions before it), None when it comes from anything else."""
    def writes_a0(w):
        op = w >> 26
        if op == 0:
            return (w >> 11) & 31 == reg and (w & 0x3F) not in (8, 9, 0x18, 0x19, 0x1A, 0x1B)   # rd (not jr/jalr/mult/div)
        if op in (2, 3):
            return False
        return op >= 8 and (w >> 16) & 31 == reg and op not in (0x28, 0x29, 0x2B, 0x2A, 0x2E)   # rt of a non-store
    for addr in [p_c + 4] + [p_c - 4 * k for k in range(1, 9)]:
        w = link.word(addr)
        if addr != p_c + 4 and (w >> 26 in (2, 3) or (w >> 26 == 0 and w & 0x3F in (8, 9))):
            return None                       # an earlier call or jump: $a0 is not set here
        if writes_a0(w):
            if w >> 26 in (9, 0xD) and (w >> 21) & 31 == 0:
                imm = w & 0xFFFF
                return imm - 0x10000 if w >> 26 == 9 and imm & 0x8000 else imm
            return None
    return None


def check_specials(out, defined=True, dark_tooth=False, dark_tooth_defined=None, special_as=False):
    """Check 8 (TM2_PORT_CAR_SPECIALS, CAR_GROUPS.md 10): the retail special state is used only inside a
    selection. Every function of the link that relocates into the retail special globals (SPECIAL_STATE,
    retail address and size mapped through addrmap.tsv) is either the selection mechanism itself, a function
    on the first cars' own path (SPECIALS_RETAIL_PATH: round reset and load-time clears, which the port code
    repeats for the duplicates), dead (no caller), or reached only through callers that select: walking up
    the jal graph from it, every path must meet a function that calls portCarSpecialSelect /
    portCarSpecialNext and portCarSpecialRestore before it reaches a function without callers. Calls of
    objRegistrySetStep count only where $a0 is 0x914 / 0x915 (or not a constant). Also: the port functions
    and the moved bodies are in .port_text, and no selection function calls rand().
    dark_tooth: Dark Tooth's state (DARK_TOOTH_STATE) is checked too, and arenaSetup is on the first cars' path;
    dark_tooth_defined (default: dark_tooth): also its port functions (DARK_TOOTH_PORT).
    special_as (request 35): the special-as selections (SPECIAL_AS_SELECT) select too, and portCarSpecialAsReset
    (each load: the 2324 / 2325 nodes cleared before the car files install) is on the first cars' path."""
    if dark_tooth_defined is None:
        dark_tooth_defined = dark_tooth
    link = Link(out)
    amap = AddrMap.read(out / 'addrmap.tsv')
    ranges = []
    state = dict(SPECIAL_STATE, **(DARK_TOOTH_STATE if dark_tooth else {}))
    retail_path = SPECIALS_RETAIL_PATH | ({'arenaSetup'} if dark_tooth else set())
    retail_path |= {'portCarSpecialAsReset'} if special_as else set()
    port_fns = SPECIALS_PORT | (DARK_TOOTH_PORT if dark_tooth_defined else set())
    for name, (va, size) in state.items():
        c0, c1 = amap.r2c(va), amap.r2c(va + size)
        if c0 is None or c1 is None or c1 - c0 != size:
            sys.exit('check_car_groups: %s (retail %08X +0x%X) does not map linearly into the link' % (name, va, size))
        ranges.append((c0, c1, name))
    rows = functions(out)
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    names_at = {}
    for name, s in link.symbols.items():
        names_at.setdefault(s['address'], set()).add(name)
    relocs, _ = link.relocations()
    starts = {a: n for a, _, n in rows}
    users, callers, calls, pointed = {}, {}, {}, set()
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        tab = next((n for lo, hi, n in ranges if lo <= v_c < hi), None)
        if tab is not None:
            users.setdefault(fn, set()).add(tab)
        if rtype != 4 and v_c in starts:
            pointed.add(starts[v_c])           # a function address taken (table, callback): an entry point
        if rtype == 4:
            callee = owner(rows, v_c) or sym
            if callee == fn:
                continue                       # a branch inside the function
            calls.setdefault(fn, set()).add(callee)
            if callee == 'objRegistrySetStep' and (fn in SPECIALS_OTHER_IDS or
                                                   a0_constant(link, p_c) not in (None, 0x914, 0x915)):
                continue                       # another id: not the special's animation
            callers.setdefault(callee, set()).add(fn)
    selecting = {f for f, c in calls.items() if c & ({'portCarSpecialSelect', 'portCarSpecialNext'} |
                                                     (SPECIAL_AS_SELECT if special_as else set()))
                 and 'portCarSpecialRestore' in c}
    ok = True
    if defined:
        missing = sorted(n for n in port_fns | SPECIALS_BODIES if kinds.get(n) != 'port')
        rand_callers = sorted(f for f in port_fns | SPECIALS_MECHANISM if 'rand' in calls.get(f, ()))
        good = not missing and not rand_callers
        print('per-car specials%s: %d port functions and moved bodies (missing %s); rand() called by %s -> %s' % (
            ' + Dark Tooth' if dark_tooth_defined else '', len(port_fns | SPECIALS_BODIES) - len(missing), missing,
            rand_callers or 'none', 'ok' if good else 'FAIL'))
        ok = good
    print('retail special state referenced by %d functions; selecting functions: %s' % (
        len(users), ' '.join(sorted(selecting)) or 'none'))
    for u in sorted(users):
        if u in SPECIALS_MECHANISM:
            verdict = 'ok (selection mechanism)'
        elif u in retail_path:
            verdict = 'ok (first cars, outside selections)'
        elif u in selecting:
            verdict = 'ok (selects)'
        elif not callers.get(u) and u in SPECIALS_DEAD:
            verdict = 'ok (no caller)'
        else:
            bad, dead, seen, todo = [], [], set(), [(u, [u])]
            while todo:
                fn, path = todo.pop()
                if fn in seen:
                    continue
                seen.add(fn)
                up = callers.get(fn, set())
                if not up and (fn in pointed or fn == u or kinds.get(fn) == 'port'):
                    bad.append(path)           # an entry point (pointed to, the port's API) outside a selection
                elif not up:
                    dead.append(fn)            # never called nor pointed to: dead code
                for c in sorted(up):
                    if c in selecting or c in retail_path:
                        continue
                    todo.append((c, path + [c]))
            verdict = 'ok (every caller path selects%s)' % (
                '; dead callers %s' % ' '.join(sorted(dead)) if dead else '') if not bad else                 'FAIL: unselected path %s' % (' <- '.join(min(bad, key=len)))
        good = verdict.startswith('ok')
        ok = ok and good
        print('  %-28s %-34s %s' % (u, ' '.join(sorted(users[u])), verdict))
    return ok



def check_special_as(out, names, defined=True):
    """Check 13 (TM2_PORT_SPECIAL_AS, request 35, GUN_GAME_SPECIALS.md "As built"): port functions in .port_text without
    rand(), the game hooks, no game access to the special-as storage, the car projectile model guard, the refused
    types. defined=False (negative control on a build without the define): the same checks, which must fail."""
    import re
    link = Link(out)
    rows = functions(out)
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    relocs, _ = link.relocations()
    calls = {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is not None and rtype == 4:
            callee = owner(rows, v_c) or sym
            if callee != fn:
                calls.setdefault(fn, set()).add(callee)
    ok = True
    cars = 'TM2_PORT_CARS' in names
    port_fns = SPECIAL_AS_PORT | (SPECIAL_AS_LOAD_PORT if cars else set())
    missing = sorted(n for n in port_fns if kinds.get(n) != 'port')
    rand_callers = sorted(n for n in port_fns if 'rand' in calls.get(n, ()))
    good = not missing and not rand_callers
    print('special-as: %d port functions in .port_text (missing %s); rand() called by %s -> %s' % (
        len(port_fns) - len(missing), missing, rand_callers or 'none', 'ok' if good else 'FAIL'))
    ok = good
    hooks = dict(SPECIAL_AS_HOOKS)
    hooks.update(SPECIAL_AS_HOOKS_CARS if cars else {})
    hooks.update(SPECIAL_AS_HOOKS_SWAP if 'TM2_PORT_CAR_SWAP' in names else {})
    bad = sorted('%s (%s)' % (fn, ' '.join(sorted(want - calls.get(fn, set())))) for fn, want in hooks.items()
                 if not want <= calls.get(fn, set()))
    direct = 'portCarSpecialSelect' in calls.get('weaponFireSpecial', set())
    good = not bad and not direct
    print('special-as hooks: %d functions; missing calls %s; weaponFireSpecial calls portCarSpecialSelect itself: %s -> %s'
          % (len(hooks), bad or 'none', direct, 'ok' if good else 'FAIL'))
    ok = ok and good
    # storage: only port code
    spans = []
    for name, size in SPECIAL_AS_STORAGE.items():
        s = link.symbols.get(name)
        if s is None:
            print('special-as storage: %s not in the link -> FAIL' % name)
            ok = False
            continue
        spans.append((s['address'], s['address'] + size, name))
    users = {}
    for p_c, rtype, v_c, item, sym in relocs:
        if rtype == 4:
            continue
        name = next((n for lo, hi, n in spans if lo <= v_c < hi), None)
        if name is None:
            continue
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        users.setdefault(fn, set()).add(name)
    game_users = sorted(fn for fn in users if kinds.get(fn) != 'port')
    good = not game_users
    print('special-as storage read/written by %d port functions; game functions: %s -> %s' % (
        len(users) - len(game_users), game_users or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    # the car projectile model guard
    words = {}
    for name in MODEL_WORDS:
        s = link.symbols.get(name)
        if s is None:
            sys.exit('check_car_groups: %s not in the link' % name)
        words[s['address']] = name
    loads, stores = {}, {}
    for p_c, rtype, v_c, item, sym in relocs:
        if rtype == 4 or v_c not in words:
            continue
        w = link.word(p_c)
        op = w >> 26
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        if op == 0x23:
            loads.setdefault(fn, []).append((p_c, (w >> 16) & 31, words[v_c]))
        elif op == 0x2B:
            stores.setdefault(words[v_c], set()).add(fn)
    unguarded = []
    for fn, lst in sorted(loads.items()):
        if kinds.get(fn) == 'port':
            continue
        for p_c, rt, name in lst:
            guarded = False
            for k in range(1, 41):
                w = link.word(p_c + 4 * k)
                op, rs, rt2 = w >> 26, (w >> 21) & 31, (w >> 16) & 31
                if op in (4, 5) and ((rs == rt and rt2 == 0) or (rt2 == rt and rs == 0)):
                    guarded = True
                    break
                if op in (2, 3) or (op == 0 and w & 0x3F in (8, 9)):
                    break                      # a call or jump before the test
                if (op == 0 and (w >> 11) & 31 == rt and w & 0x3F not in (0x18, 0x19, 0x1A, 0x1B)) or                         (op >= 8 and rt2 == rt and op not in (0x28, 0x29, 0x2A, 0x2B, 0x2E)):
                    break                      # the register is overwritten before any test
            if not guarded:
                unguarded.append('%s:%s@%08X' % (fn, name, p_c))
    writers_bad = sorted('%s by %s' % (n, ' '.join(sorted(fs - MODEL_WRITERS))) for n, fs in stores.items()
                         if fs - MODEL_WRITERS)
    reset_clears = sorted(n for n in MODEL_WORDS if 'portCarSpecialAsReset' not in stores.get(n, set()))
    good = not unguarded and not writers_bad and not reset_clears
    print('car projectile models: read by %s; unguarded loads %s; other writers %s; not cleared at the load %s -> %s' % (
        ' '.join(sorted(loads)) or 'none', unguarded or 'none', writers_bad or 'none', reset_clears or 'none',
        'ok' if good else 'FAIL'))
    ok = ok and good
    # the refused types
    hdr = (ROOT / 'include' / 'semantic' / 'car_specials.h').read_text(encoding='utf-8')
    m = re.search(r'#define TM2_PORT_SPECIAL_AS_MASK (0x[0-9A-Fa-f]+)', hdr)
    mask = int(m.group(1), 16) if m else -1
    want_mask = sum(1 << (t >> 4) for t in SPECIAL_AS_BORROWABLE)
    refused_in = sorted('0x%02X' % t for t in SPECIAL_AS_REFUSED if mask >= 0 and mask >> (t >> 4) & 1)
    imm = False
    for start, size, name in rows:
        if name != 'portCarSpecialAsSet':
            continue
        for a in range(start, start + size, 4):
            w = link.word(a)
            if w >> 26 in (9, 0xC, 0xD) and w & 0xFFFF == want_mask:
                imm = True
    good = mask == want_mask and not refused_in and imm
    print('borrowable types: mask %s (want 0x%04X: %s); refused types in it %s; portCarSpecialAsSet holds the mask: %s -> %s' % (
        '0x%04X' % mask if mask >= 0 else 'missing', want_mask, ' '.join('0x%02X' % t for t in sorted(SPECIAL_AS_BORROWABLE)),
        refused_in or 'none', imm, 'ok' if good else 'FAIL'))
    ok = ok and good
    if not defined:
        print('special-as (negative control): %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok



def check_dt_player(out, defined=True):
    """Check 14 (TM2_PORT_DT_PLAYER, request 36, DARK_TOOTH_PLAYER.md): port functions in .port_text without rand(), the
    hooks, no path from a player's Dark Tooth to the boss controller, no rand() in the head placement, the storage's
    users. defined=False (negative control on a build without the define): the same checks, which must fail."""
    link = Link(out)
    rows = functions(out)
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    starts = {r[0]: r[2] for r in rows}
    relocs, _ = link.relocations()
    calls, callers, pointed = {}, {}, set()
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        if rtype == 4:
            callee = owner(rows, v_c) or sym
            if callee != fn:
                calls.setdefault(fn, set()).add(callee)
                callers.setdefault(callee, set()).add(fn)
        elif v_c in starts:
            pointed.add(starts[v_c])           # a function address taken (table, callback)

    def reach(first):
        seen, todo = set(), list(first)
        while todo:
            fn = todo.pop()
            if fn in seen:
                continue
            seen.add(fn)
            todo.extend(calls.get(fn, ()))
        return seen

    ok = True
    missing = sorted(n for n in DT_PLAYER_PORT if kinds.get(n) != 'port')
    rand_direct = sorted(n for n in DT_PLAYER_PORT if 'rand' in calls.get(n, ()))
    good = not missing and not rand_direct
    print('player Dark Tooth: %d port functions in .port_text (missing %s); rand() called by %s -> %s' % (
        len(DT_PLAYER_PORT) - len(missing), missing, rand_direct or 'none', 'ok' if good else 'FAIL'))
    ok = good
    bad = sorted('%s (%s)' % (fn, ' '.join(sorted(want - calls.get(fn, set())))) for fn, want in DT_PLAYER_HOOKS.items()
                 if not want <= calls.get(fn, set()))
    good = not bad
    print('player Dark Tooth hooks: %d functions; missing calls %s -> %s' % (
        len(DT_PLAYER_HOOKS), bad or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    # the boss controller: only the AI path reaches it, and nothing the port code reaches
    boss_bad = []
    for fn, want in DT_PLAYER_BOSS.items():
        got = callers.get(fn, set())
        if got != want:
            boss_bad.append('%s called by %s (want %s)' % (fn, ' '.join(sorted(got)) or 'nothing', ' '.join(sorted(want))))
        if fn in pointed:
            boss_bad.append('%s pointed to' % fn)
    port_reach = reach([n for n in DT_PLAYER_PORT if n in kinds])
    boss_reached = sorted(set(DT_PLAYER_BOSS) & port_reach)
    good = not boss_bad and not boss_reached and 'aiDarkToothBossUpdate' in kinds
    print('boss controller: %s; reached from the player Dark Tooth code: %s -> %s' % (
        '; '.join(boss_bad) or 'aiDarkToothBossUpdate <- aiWeaponFireDecide <- weaponTick (AI branch) only',
        boss_reached or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    # the head placement: no rand() anywhere below it
    place = reach(['portCarDtPlayerPlace']) if 'portCarDtPlayerPlace' in kinds else set()
    good = bool(place) and 'rand' not in place
    via = sorted(n for n in DT_PLAYER_PORT if n in kinds and 'rand' in reach([n]))
    print('head placement (portCarDtPlayerPlace) reaches %s: rand() %s -> %s' % (
        ' '.join(sorted(place - {'portCarDtPlayerPlace'})) or 'nothing', 'reached' if 'rand' in place else 'not reached',
        'ok' if good else 'FAIL'))
    print("  port functions that reach rand() through the game (as the boss's weapons): %s" % (' '.join(via) or 'none'))
    ok = ok and good
    # the storage
    sym = link.symbols.get('portDtPlayer')
    users = {}
    if sym is None:
        print('player Dark Tooth storage: portDtPlayer not in the link -> FAIL')
        ok = False
    else:
        for p_c, rtype, v_c, item, name in relocs:
            if rtype != 4 and sym['address'] <= v_c < sym['address'] + 0x94:
                fn = owner(rows, p_c) or 'data@%08X' % p_c
                users.setdefault(fn, set()).add('portDtPlayer')
        game_users = sorted(fn for fn in users if kinds.get(fn) != 'port')
        other = sorted(set(game_users) - DT_PLAYER_STORAGE_USERS)
        good = not other and set(game_users) == DT_PLAYER_STORAGE_USERS
        print('portDtPlayer used by %d port functions and the game functions %s (others: %s) -> %s' % (
            len(users) - len(game_users), ' '.join(game_users) or 'none', other or 'none', 'ok' if good else 'FAIL'))
        ok = ok and good
    if not defined:
        print('player Dark Tooth (negative control): %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


def check_park_head(out, names, defined=True):
    """Check 16 (TM2_PORT_DARK_TOOTH, request 46): the frame hook and the park paths hide a parked Dark Tooth's head
    through portCarDarkToothParkHead; portCarDtParkMask has only its users. defined=False: negative control."""
    link = Link(out)
    rows = functions(out)
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('	')
        kinds[f[4]] = f[6]
    relocs, _ = link.relocations()
    calls = dropin_calls(link, rows, relocs, kinds)
    ok = True
    hook = 'portCarDarkToothParked' in calls.get('aiCarsCountRemaining', ())
    frame = calls.get('portCarDarkToothParked', set())
    good = hook and {'portCarDarkToothParkHead', 'portCarSpecialSelect', 'portCarSpecialRestore'} <= frame and         kinds.get('portCarDarkToothParked') == 'port' and kinds.get('portCarDarkToothParkHead') == 'port'
    print('parked head: aiCarsCountRemaining -> portCarDarkToothParked %s; it calls %s -> %s' % (
        'yes' if hook else 'NO', ' '.join(sorted(frame & {'portCarDarkToothParkHead', 'portCarSpecialSelect',
                                                          'portCarSpecialRestore'})) or 'none', 'ok' if good else 'FAIL'))
    ok = good
    if 'TM2_PORT_CAR_SWAP' in names:
        paths = list(PARK_PATHS)
        if 'portCarModelOn' in kinds:
            paths.append('portCarModelOn')
        bad = [p for p in paths if 'portCarDarkToothParkHead' not in calls.get(p, ())]
        good = not bad
        print('parked head: park paths %s hide through portCarDarkToothParkHead (not: %s) -> %s' % (
            ' '.join(paths), ' '.join(bad) or 'none', 'ok' if good else 'FAIL'))
        ok = ok and good
    sym = link.symbols.get('portCarDtParkMask')
    if sym is None:
        print('parked head: portCarDtParkMask not in the link -> FAIL')
        ok = False
    else:
        users = set()
        for p_c, rtype, v_c, item, name in relocs:
            if rtype != 4 and sym['address'] <= v_c < sym['address'] + 8:
                users.add(owner(rows, p_c) or 'data@%08X' % p_c)
        good = users == PARK_MASK_USERS
        print('parked head: portCarDtParkMask used by %s (expected %s) -> %s' % (
            ' '.join(sorted(users)) or 'none', ' '.join(sorted(PARK_MASK_USERS)), 'ok' if good else 'FAIL'))
        ok = ok and good
    if not defined:
        print('parked head (negative control): %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


# ---- check 12: drop-in (TM2_PORT_CAR_DROPIN, request 33 "online with bots", CAR_GROUPS.md 14) ----
# sessionBindCars, sessionLoadArenaAndCars, startNodePickRandom: steps 1/2; csAlloc: the unloads' free list (14.7)
EXPECTED_DROPIN = {'bitmapSetupStorage', 'csAlloc'}
DROPIN_PORT = {
    'portCarRemove', 'portHumanCompact', 'portCarStartHuman', 'portCarStartAi', 'portCarActiveCount',
    'portCarGroupFree', 'portCarFreeGroupOfType', 'portCarLoadGroupLive', 'portCarUnloadGroup', 'portCarBindSkip',
    'portCarBindCount', 'portCarLoadDone', 'portCarLiveStorage', 'portCarRoom', 'portCarCanLoad', 'portCsTake',
}
# game function -> the drop-in hooks it must call
DROPIN_HOOKS = {
    'bitmapSetupStorage': {'portCarLiveStorage'}, 'sessionLoadArenaAndCars': {'portCarLoadDone'},
    'sessionBindCars': {'portCarBindCount', 'portCarBindSkip'}, 'portCarGroupsReset': {'portCarDropInReset'},
    'csAlloc': {'portCsTake'}, 'portCarLoadGroup': {'portDropInNoteLoad'}, 'portCarRemove': {'portDropInSlotSound'},
    'portCarRegisterRoot': {'portCarFreeDupKey'}, 'portCarLoadGroupLive': {'portDropInLoadCheck'},
}
# Unloading (CAR_GROUPS.md 14.7): every holder of a per-key, per-slot, per-group or per-object state a car's install
# fills, and every word that may name its slots, objects or DMD copy, must be written by the port code
# portCarUnloadGroup reaches (cleared, moved or ended there). name -> (the defines it needs, retail VA and bytes when
# the link has no symbol of its own, else None).
UNLOAD_HOLDERS = {
    # per key / per slot / per group (registries, slot tables, bindings, drop-in and unload state)
    'portCarGroups': ((), None), 'carSlotObjects': ((), None), 'carSlotOwners': ((), None),
    'carSlotKinds': ((), None), 'carSlotIndices': ((), None), 'carWorldObjectCount': ((), None),
    'portCarBindGroup': ((), None), 'portCarDropIn': ((), None), 'portCarUnload': ((), None),
    'portBalanceRideTop': ((), None), 'portBalanceCars': ((), None), 'portCarSoundLoops': ((), None),
    # scene objects and the words that name them
    'csDrawListHead': ((), None), 'weaponTornadoState': ((), (0x80184DC0, 0x14)),
    'weaponTornadoObject': ((), None), 'weaponTornadoFunnelObject': ((), None),
    'weaponStrikeState': ((), (0x80184E10, 0x64)), 'darkToothBossObject': ((), None),
    'fxRecords': ((), (0x801A5B08, 980 * 16)), 'weaponRicochetPool': ((), (0x80184860, 52 * 10)),
    'weaponFlameEmitters': ((), (0x80184774, 32 * 2)),
    'weaponShieldPool': ((), (0x80184A68, 16 * 4)), 'weaponArcs': ((), (0x801847B4, 20 * 5)),
    'weaponDrops': ((), (0x80184B04, 28 * 25)),
    # nodes of the DMD copy
    'weaponSpiralMissileModel': ((), None), 'weaponBoomerangModel': ((), None), 'weaponGrimmSpecialModel': ((), None),
    'weaponGhostMissileModel': ((), None), 'weaponRicochetModelAlt': ((), None), 'objRegistryId2324': ((), None),
    'objRegistryId2325': ((), None), 'slamArmObject': ((), None), 'slamClawObject': ((), None),
    'worldLoadParentObject': ((), None), 'animNodeList': ((), None), 'animListCount': ((), None),
    'renderWorldModels': ((), None),
    # textures (image records always; the rest with the defines that make them per car)
    'bitmapRecordCount': ((), None),
    'clutAnimDefs': (('TM2_PORT_TEXTURE_BANKS', 'TM2_PORT_CAR_CLUTS'), None),
    'clutAnimRunning': (('TM2_PORT_TEXTURE_BANKS', 'TM2_PORT_CAR_CLUTS'), None),
    'clutAnimLastIndex': (('TM2_PORT_TEXTURE_BANKS', 'TM2_PORT_CAR_CLUTS'), None),
    'portTexBanks': (('TM2_PORT_TEXTURE_BANKS', 'TM2_PORT_CAR_CLUTS'), None),
    'portCarCluts': (('TM2_PORT_CAR_CLUTS',), None),
    # per-key special state, request 35's as-entries, per-slot sound, far-LOD records
    'portCarSpecials': (('TM2_PORT_CAR_SPECIALS',), None), 'portSpecialAs': (('TM2_PORT_SPECIAL_AS',), None),
    'portCarSpecialAs': (('TM2_PORT_SPECIAL_AS',), None), 'portCarSound': (('TM2_PORT_CAR_SOUND',), None),
    'portFarLodTable': (('TM2_PORT_FAR_LOD',), None),
    'portFarLodNear': (('TM2_PORT_FAR_LOD',), None),      # request 46 item 6: every LOD record of the match
}
# the blocks portCarUnloadGroup must call (the swap's and remove's, the bank, the cs free list)
UNLOAD_CALLS = {'portCarResetSpecials', 'portCarResetLook', 'portCarParkSlot', 'portCarRekeyReferences',
                'portDropInSlotSound', 'portCsGive', 'portDmdGive', 'portDropInRecords'}
UNLOAD_CALLS_BY_DEFINE = {'TM2_PORT_TEXTURE_BANKS': {'portTexBankFree'}, 'TM2_PORT_CAR_CLUTS': {'portDropInClutCopies'},
                          'TM2_PORT_CAR_SPECIALS': {'portDropInSpecialEntry'},
                          'TM2_PORT_SPECIAL_AS': {'portDropInSpecialAs', 'portCarSpecialAsSet'},
                          'TM2_PORT_CAR_SOUND': {'portCarSoundStopSlot'}}
# Every game function that reads the car slot tables (carSlotObjects, carSlotOwners, carSlotKinds, carSlotIndices,
# carWorldObjectCount) and so can meet a slot an unload gave back (object: the dummy, zeroed, never drawn, far below,
# id 0; type 0; kind 2 (no driver); index -1; group TM2_PORT_SLOT_FREE; key 0), with the census verdict (read in the
# code, CAR_GROUPS.md 14.7): a new reader fails the check until it is reviewed and added here.
_OPP = 'reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which ' \
       'the link places in the slot tables: no slot is read'
UNLOAD_SLOT_REVIEWED = {
    'arenaSetup': 'carChoiceRegisterGroup(dummy, key 0) for a free slot: the dummy has no model, nothing registered',
    'carBindAiByType': 'searches by type (never 0) and by group (portCarSlotOfGroup: a free slot has group -1)',
    'carBindHumanByType': 'searches by type (never 0) and by group (portCarSlotOfGroup: a free slot has group -1)',
    'carBindAiSlot': 'binds the slot it is given (a group\'s slot from the binders or portCarStartAi: never a free one)',
    'carBindHumanSlot': 'binds the slot it is given (a group\'s slot from the binders or portCarStartHuman)',
    'carDestroyed': 'the dying record\'s own slot (bound: an unload refuses a driven group)',
    'carFindById': 'id 0 of the dummy: kind 2 is never "found" (as a spare car\'s object); index -1 unused then',
    'carFindByObject': 'the dummy: kind 2 is never "found"',
    'carFindSlotByOwner': 'searches by type (never 0)',
    'carFromSlot': 'kind 2 / index -1: returns 0; its callers skip 0 (weaponFireSpecial\'s shockwave loop) or pass a slot '
                   'of this frame\'s contacts or attackers (the dummy collides with nothing; credits were cleared)',
    'carObjectCount': 'getter; its caller weaponFireSpecial skips carFromSlot 0',
    'carObjectFromOwner': 'by key (a free slot has key 0, never searched); no match returns carSlotObjects[count]: a '
                          'free slot past the trimmed count holds the dummy, as retail\'s one-past entry',
    'carObjectId': 'a bound record\'s slot',
    'carPairsUpdate': 'every pair of slots: the dummy\'s position (far below), recomputed each frame, never in range',
    'carRegisterModel': 'writes the slot a live load takes (the count is set to the free run for the install)',
    'carTablesClear': 'load reset',
    'sessionBindCars': 'the binders above; ' + _OPP,
    'sessionLoadArenaAndCars': _OPP, 'sessionGetOpponentCar': _OPP, 'sessionSetOpponentCar': _OPP,
    'settingsGetOpponentCar': _OPP, 'settingsSetOpponentCar': _OPP,
}
# Every game function that reads the per-key registries (portCarGroups) and so can meet a key an unload gave back
# (every entry zero, as for a key no car of the load has): the verdict.
_REG = 'registers under the key of the group being installed (portCarLoadKey): a key handed out at that install'
_KEY = 'a bound record\'s or a slot\'s key (no slot has a given-back key; stored keys were let go): never a free key; ' \
       'a zero entry is a key no car has, which retail meets for every type not in the match (null-safe)'
UNLOAD_KEY_REVIEWED = {
    'carBodyObjectRegister': _REG, 'carWreckModelRegister': _REG, 'carRegisterTireSwitch': _REG,
    'carFreezeBuild': _REG, 'carPartRegisterAxel': _REG, 'carPartRegisterDarkTooth': _REG,
    'carPartRegisterHammerhead': _REG, 'carPartRegisterMinion': _REG, 'carPartRegisterMrSlam': _REG,
    'objRegistryAdd': _REG,
    'carChoiceRegisterGroup': 'arenaSetup and the live load: a slot\'s key (a free slot passes 0 with the dummy, which '
                              'has no model: nothing written)',
    'carBodyObjectHide': _KEY, 'carBodyObjectShow': _KEY, 'carBodyPoseUpdate': _KEY, 'carSetDamageStage': _KEY,
    'carWreckModelGet': _KEY, 'carFreezeSetState': _KEY, 'carPartsAnimate': _KEY, 'uaswToggleCarTireState': _KEY,
    'carFreezeResetAll': 'steps all 48 keys\' freeze records and turbo objects: a zeroed record and a null turbo are '
                         'what every unused key holds (animSetStep is null-safe)',
    'objRegistrySetStep': 'the type + 6 path reads turbo[type >> 4]: 0 when the type\'s first car is gone, as for a type '
                          'not in the match',
    'carObjectFromOwner': 'compares slot keys (a free slot has key 0)',
}
# The census of what holds a car across frames (CAR_GROUPS.md 14.3; PER_CAR_PLAN.md B.6.3): name -> (retail VA,
# retail bytes). Every holder must be written by the port code portCarRemove reaches; the record fields (killer,
# last attacker, crush, claw records' spin_target) by the blocks it must call (DROPIN_REMOVE_CALLS).
DROPIN_HOLDERS = {
    'fxRecords': (0x801A5B08, 980 * 16), 'weaponArcs': (0x801847B4, 20 * 5), 'weaponDrops': (0x80184B04, 28 * 25),
    'weaponShieldPool': (0x80184A68, 16 * 4), 'weaponRicochetPool': (0x80184860, 52 * 10),
    'bulletRecords': (0x80181498, 60 * 40), 'fxSprites': (0x80185144, 48 * 150),
    'weaponFlameEmitters': (0x80184774, 32 * 2), 'aiCarTargets': (0x801879FC, 4 * 9),
    'viewWatchedAiCar': (0x80180FAC, 4), 'weaponTornadoState': (0x80184DC0, 0x14),
    'homingLeapState': (0x80184DD4, 0x3C), 'weaponStrikeState': (0x80184E10, 0x64),
    'aiDarkToothSwoopTarget': (0x80180B58, 2), 'darkToothSwooping': (0x80180F1C, 4),
}
DROPIN_REMOVE_CALLS = {'portCarResetSpecials', 'portCarResetLook', 'portCarRekeyReferences', 'portCarParkSlot',
                       'portHumanCompact', 'fxRecordFree', 'bulletFree', 'soundStopInstance'}
# the per-human arrays and the id / address holders portHumanCompact must move (CAR_GROUPS.md 14.5)
DROPIN_COMPACT_HOLDERS = {
    'humanCars': (0x80187B10, 0x814 * 2), 'humanCarObjects': None, 'humanCarObjectSlots': None,
    'humanCarTypes': None, 'padConfigs': None, 'hudSmoothedSpeeds': None, 'carViewHotkeyReleased': None,
    'carResetWeapons': (0x80180FA0, 4), 'portCarBindGroup': None, 'carSlotIndices': None,
    'fxRecords': DROPIN_HOLDERS['fxRecords'], 'weaponArcs': DROPIN_HOLDERS['weaponArcs'],
    'weaponDrops': DROPIN_HOLDERS['weaponDrops'], 'weaponShieldPool': DROPIN_HOLDERS['weaponShieldPool'],
    'weaponRicochetPool': DROPIN_HOLDERS['weaponRicochetPool'], 'bulletRecords': DROPIN_HOLDERS['bulletRecords'],
    'fxSprites': DROPIN_HOLDERS['fxSprites'], 'weaponTornadoState': DROPIN_HOLDERS['weaponTornadoState'],
    'homingLeapState': DROPIN_HOLDERS['homingLeapState'], 'weaponStrikeState': DROPIN_HOLDERS['weaponStrikeState'],
    'aiDarkToothSwoopTarget': DROPIN_HOLDERS['aiDarkToothSwoopTarget'],
}
# what a mid-match load must never reach: the load-time resets and setups (B.7.1) and the CD
DROPIN_LOAD_ONLY = {
    'bitmapInit', 'bitmapSetupStorage', 'arenaResetSystems', 'carTablesClear', 'carPartsReset', 'objRegistriesReset',
    'csPoolInit', 'debrisInit', 'renderClearModelLists', 'viewInit', 'clutAnimInit', 'pickupSlotsReset',
    'targetPoolReset', 'targetActorsReset', 'lightInit', 'weaponObjectsDetach', 'levelEventsReset',
    'worldLoadStartObjects', 'portFarLodScanLevel', 'bulletsClear', 'fxSpritesReset', 'missilesCreateObjects',
    'missileTrailsInit', 'portCarGroupsReset', 'portCarSpecialsReset', 'portTexBanksReset', 'portCarClutsReset',
    'portCarClutsCopyAll', 'sessionLoadArenaAndCars', 'portCarLoadGroup', 'fileioLoadFileIntoRam', 'uaInit',
    'arenaSetup', 'sessionBindCars', 'weaponsRoundReset', 'projectilesReset', 'weaponDropsReset', 'CdRead',
    'CdReadSync', 'CdControl', 'CdSearchFile',
}
# Every function of the link that reads the AI record count or an AI index array (aiCarTotal, aiCarObjects,
# aiCarObjectSlots, aiCarTargets) and so can meet a vacant record (portDropInVacate), with the census verdict
# (CAR_GROUPS.md 14.4; read in the code): a new reader fails the check until it is reviewed and added here.
DROPIN_AI_REVIEWED = {
    'worldUpdate': 'pack loops skip wreck state 4, the closer loop skips dead; aiCarShouldUpdate is 0 (no drive, no '
                   'physics); carSoundUpdate only while active; the out-of-range branch: type 0 has no engine id, slot 63 '
                   'stops only its own instance; radar needs HP > 0; aiCarDamageUpdate: dead, max_hp kept > 0',
    'carsPhysicsUpdate': 'hdpCollideObject only when updated; carContactsCombine reads the dummy object (no points), the '
                         'contact block is zeroed (no respond, no push timer)',
    'aiCarsCountRemaining': 'HP > 0; type 0 keeps the level-8 aiCars[0] Dark Tooth rule off',
    'aiCarsCountAlive': 'HP > 0', 'aiCarNearestAlive': 'HP > 0',
    'aiCarCount': 'getter; its callers (weapons, lock-ons, slam, tornado, fires, targets) test HP > 0 or distance (far)',
    'aiCarObject': 'getter: the dummy object (visible 0, far below, no collision box) for a vacant record',
    'aiCarObjectSlot': 'getter (no caller in the link)',
    'carObjectId': 'carSlotObjects[63] = the dummy: id 0; reached only through contacts, which the dummy never makes',
    'aiGetTargetCar': 'aiCarTargets[j] = -1 (nearest human); an AI target on a removed record is reset to -1',
    'aiApplyDifficulty': 'stats only (difficulty change)',
    'arenaSetup': 'clears aiCarTargets', 'uaInit': 'clears aiCarTargets', 'carTablesClear': 'load reset',
    'carBindAiByType': 'sessionBindCars skips vacant records once drop-in is live',
    'carBindAiSlot': 'the binding itself (portCarStartAi)',
    'carDestroyed': 'only for a car dying now (a vacant record is dead already: no second carDestroyed)',
    'carSaveNodeState': 'no caller in the link', 'carRestoreNodeState': 'no caller in the link',
    'demoRespawnAiCars': 'attract demo only (drop-in is refused there)',
    'demoWatchNextAiCar': 'attract demo only', 'roundStart': 'attract demo branches only',
    'viewWatchNextAiCar': 'view 7 (no caller in the link); viewWatchedAiCar is moved off a removed record',
}
# rand() callers a mid-match install may reach in the static call graph, and why none runs for a car DMD:
# worldLoadObject's cases for the level's target objects and events (ids 500..520 and the event ids; a car DMD holds
# only its car ids, 249, 309, 2214, 2301, 2308, 2309 and 2321..2327: check 3 --dmd lists them) and the animation
# steppers animRegister stores as function pointers (run by the frame's animation update, not by the install).
DROPIN_LOAD_RAND_OK = {'targetPlayerEventInit', 'targetTimedEventInit', 'targetActorPlace', 'animPickPeriod',
                       'animStepRandom', 'animStepWeightedRandom'}


def dropin_calls(link, rows, relocs, kinds):
    """Call edges: jal (R_MIPS_26) and any hi/lo or word relocation to a function's first byte (function pointers:
    dbscanHierarchy's worldLoadObject)."""
    starts = {r[0]: r[2] for r in rows}
    calls = {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is None:
            continue
        if rtype == 4:
            calls.setdefault(fn, set()).add(owner(rows, v_c) or sym)
        elif rtype in (2, 6) and v_c in starts:
            calls.setdefault(fn, set()).add(starts[v_c])
    return calls


def dropin_reach(calls, starts, follow):
    seen, todo = set(), list(starts)
    while todo:
        fn = todo.pop()
        if fn in seen:
            continue
        seen.add(fn)
        todo.extend(c for c in calls.get(fn, ()) if follow(c))
    return seen


def check_dropin(out, names=frozenset()):
    """Check 12 (TM2_PORT_CAR_DROPIN): port functions and game hooks; every reference holder of the census is
    written by the code portCarRemove reaches (and the per-human ones by portHumanCompact); no load-time-only
    function, no CD read and no rand() reachable from portCarLoadGroupLive; the readers of the AI record count and
    AI index arrays are the reviewed ones."""
    kinds = {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        kinds[f[4]] = f[6]
    rows = functions(out)
    link = Link(out)
    amap = AddrMap.read(out / 'addrmap.tsv')
    relocs, _ = link.relocations()
    calls = dropin_calls(link, rows, relocs, kinds)
    ok = True
    missing = sorted(n for n in DROPIN_PORT if kinds.get(n) != 'port')
    hooks = sorted('%s -> %s' % (f, h) for f, hs in DROPIN_HOOKS.items() for h in hs if h not in calls.get(f, ()))
    pick = 'portStartNodePick'
    addrs = sorted(s['address'] for s in link.symbols.values())

    def rng(name, retail):
        if name in link.symbols:
            a = link.symbols[name]['address']
            return a, next((x for x in addrs if x > a), a + 4)
        c0 = amap.r2c(retail[0]) if retail else None
        return (c0, c0 + retail[1]) if c0 is not None else (None, None)

    def writers(names):
        """function -> holders it relocates into"""
        spans = {n: rng(n, r) for n, r in names.items()}
        bad = sorted(n for n, (a, b) in spans.items() if a is None)
        hit = {}
        for p_c, rtype, v_c, item, sym in relocs:
            fn = owner(rows, p_c)
            if fn is None:
                continue
            for n, (a, b) in spans.items():
                if a is not None and a <= v_c < b:
                    hit.setdefault(n, set()).add(fn)
        return hit, bad

    port_reach_remove = dropin_reach(calls, ['portCarRemove'], lambda c: kinds.get(c) == 'port')
    called = {c for f in port_reach_remove for c in calls.get(f, ())}
    hit, unmapped = writers(DROPIN_HOLDERS)
    not_cleared = sorted(n for n in DROPIN_HOLDERS if not (hit.get(n, set()) & port_reach_remove))
    no_call = sorted(DROPIN_REMOVE_CALLS - called - port_reach_remove)
    good = not missing and not hooks and not not_cleared and not no_call and not unmapped
    print('drop-in: %d port functions (missing %s); hooks missing %s; census holders not written by portCarRemove\'s '
          'code %s (unmapped %s); blocks not called %s -> %s' % (
              len(DROPIN_PORT) - len(missing), missing, hooks or 'none', not_cleared or 'none', unmapped or 'none',
              no_call or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    port_reach_compact = dropin_reach(calls, ['portHumanCompact'], lambda c: kinds.get(c) == 'port')
    hit, unmapped = writers({k: v for k, v in DROPIN_COMPACT_HOLDERS.items()})
    not_moved = sorted(n for n in DROPIN_COMPACT_HOLDERS if not (hit.get(n, set()) & port_reach_compact))
    good = not not_moved and not unmapped
    print('  portHumanCompact moves every per-human array and id / address holder: missing %s (unmapped %s) -> %s' % (
        not_moved or 'none', unmapped or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    pick_reads = any(owner(rows, p_c) == pick and v_c == link.symbols['portCarDropIn']['address'] + 0x27C
                     for p_c, rtype, v_c, item, sym in relocs)
    pick_reads = pick_reads or any(owner(rows, p_c) == pick and link.symbols['portCarDropIn']['address'] <= v_c
                                   < link.symbols['portCarDropIn']['address'] + 0x3A8 for p_c, _, v_c, _, _ in relocs)
    print('  portStartNodePick reads portCarDropIn.start_live: %s' % ('ok' if pick_reads else 'FAIL'))
    ok = ok and pick_reads
    # self-test of the call graph: the match load reaches its resets and, through dbscanHierarchy's function
    # pointer, worldLoadObject
    sl = dropin_reach(calls, ['sessionLoadArenaAndCars'], lambda c: c in kinds)
    good = {'arenaResetSystems', 'worldLoadObject', 'carRegisterModel', 'fileioLoadFileIntoRam'} <= sl
    print('  call graph self-test (sessionLoadArenaAndCars reaches arenaResetSystems, worldLoadObject, '
          'carRegisterModel, fileioLoadFileIntoRam): %s' % ('ok' if good else 'FAIL'))
    ok = ok and good
    # the live load: everything it reaches, through game code and function pointers
    load = dropin_reach(calls, ['portCarLoadGroupLive'], lambda c: c in kinds)
    bad = sorted(load & DROPIN_LOAD_ONLY)
    rand = sorted(f for f in load if 'rand' in calls.get(f, ()))
    good = not bad and set(rand) <= DROPIN_LOAD_RAND_OK
    print('  portCarLoadGroupLive reaches %d functions; load-time-only or CD among them: %s; rand() callers: %s '
          '(allowed: level-object cases and animation steppers, DROPIN_LOAD_RAND_OK) -> %s' % (
              len(load), bad or 'none', ' '.join(rand) or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    for start in ('portCarStartHuman', 'portCarStartAi', 'portCarRemove', 'portCarUnloadGroup'):
        r = dropin_reach(calls, [start], lambda c: c in kinds)
        print('  %-20s reaches load-time resets: %s; rand() via %s' % (
            start, ' '.join(sorted(r & DROPIN_LOAD_ONLY)) or 'none',
            ' '.join(sorted(f for f in r if 'rand' in calls.get(f, ()))) or 'none'))
    # the readers of the AI count and AI index arrays (vacant records)
    spans = {n: rng(n, None) for n in ('aiCarTotal', 'aiCarObjects', 'aiCarObjectSlots', 'aiCarTargets')}
    readers = set()
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is None:
            continue
        if any(a is not None and a <= v_c < b for a, b in spans.values()):
            readers.add(fn)
    unreviewed = sorted(f for f in readers if f not in DROPIN_AI_REVIEWED and kinds.get(f) != 'port')
    good = not unreviewed
    print('  readers of aiCarTotal / aiCarObjects / aiCarObjectSlots / aiCarTargets: %d game functions, all reviewed '
          'for vacant records (DROPIN_AI_REVIEWED): %s -> %s' % (
              len([f for f in readers if kinds.get(f) != 'port']), 'yes' if good else 'NO: %s' % unreviewed,
              'ok' if good else 'FAIL'))
    ok = ok and good
    ok = check_unload(names, link, rows, relocs, calls, kinds, rng) and ok
    return ok


def check_unload(names, link, rows, relocs, calls, kinds, rng):
    """Check 12, unloading (CAR_GROUPS.md 14.7): every holder of what a car's install fills is written by the code
    portCarUnloadGroup reaches, which calls the shared blocks; it reaches no load-time-only function and no CD read;
    the readers of the slot tables and of the per-key registries are the reviewed ones (a slot or a key given back
    looks like one no car ever had)."""
    ok = True
    reach = dropin_reach(calls, ['portCarUnloadGroup'], lambda c: kinds.get(c) == 'port')
    called = {c for f in reach for c in calls.get(f, ())}
    want = {n for n, (defs, _) in UNLOAD_HOLDERS.items() if not defs or set(defs) & names}
    spans = {n: rng(n, UNLOAD_HOLDERS[n][1]) for n in want}
    unmapped = sorted(n for n, (a, b) in spans.items() if a is None)
    hit = {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c)
        if fn is None or fn not in reach:
            continue
        for n, (a, b) in spans.items():
            if a is not None and a <= v_c < b:
                hit.setdefault(n, set()).add(fn)
    not_written = sorted(n for n in want if n not in hit and n not in unmapped)
    need = set(UNLOAD_CALLS)
    for d, fs in UNLOAD_CALLS_BY_DEFINE.items():
        if d in names:
            need |= fs
    no_call = sorted(need - called - reach)
    good = not unmapped and not not_written and not no_call
    print('  unload: %d holders of per-key / per-slot / per-group / per-object state and DMD-copy words, written by '
          'the %d port functions portCarUnloadGroup reaches: missing %s (unmapped %s); blocks not called %s -> %s' % (
              len(want), len(reach), not_written or 'none', unmapped or 'none', no_call or 'none',
              'ok' if good else 'FAIL'))
    ok = ok and good
    full = dropin_reach(calls, ['portCarUnloadGroup'], lambda c: c in kinds)
    bad = sorted(full & DROPIN_LOAD_ONLY)
    rand = sorted(f for f in full if 'rand' in calls.get(f, ()))
    good = not bad
    print('  portCarUnloadGroup reaches %d functions; load-time-only or CD among them: %s; rand() via %s -> %s' % (
        len(full), bad or 'none', ' '.join(rand) or 'none', 'ok' if good else 'FAIL'))
    ok = ok and good
    for title, names_, reviewed in (
            ('slot tables (carSlotObjects / Owners / Kinds / Indices, carWorldObjectCount)',
             ('carSlotObjects', 'carSlotOwners', 'carSlotKinds', 'carSlotIndices', 'carWorldObjectCount'),
             UNLOAD_SLOT_REVIEWED),
            ('per-key registries (portCarGroups)', ('portCarGroups',), UNLOAD_KEY_REVIEWED)):
        sp = [rng(n, None) for n in names_]
        readers = set()
        for p_c, rtype, v_c, item, sym in relocs:
            fn = owner(rows, p_c)
            if fn is None or kinds.get(fn) == 'port':
                continue
            if any(a is not None and a <= v_c < b for a, b in sp):
                readers.add(fn)
        unreviewed = sorted(readers - set(reviewed))
        good = not unreviewed
        print('  readers of the %s: %d game functions, all reviewed for what an unload gives back: %s -> %s' % (
            title, len(readers), 'yes' if good else 'NO: %s' % unreviewed, 'ok' if good else 'FAIL'))
        ok = ok and good
    return ok


def check_tables(out):
    link = Link(out)
    amap = AddrMap.read(out / 'addrmap.tsv')
    ranges = []
    for name, (va, size) in TABLES.items():
        c0, c1 = amap.r2c(va), amap.r2c(va + size)
        if c0 is None or c1 is None or c1 - c0 != size:
            sys.exit('check_car_groups: %s (retail %08X +0x%X) does not map linearly into the link' % (name, va, size))
        ranges.append((c0, c1, name))
    rows = functions(out)
    relocs, problems = link.relocations()
    hits = {}
    for p_c, rtype, v_c, item, sym in relocs:
        tab = next((n for lo, hi, n in ranges if lo <= v_c < hi), None)
        if tab is None and sym in TABLES:
            tab = sym
        if tab is None:
            continue
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        hits.setdefault(fn, set()).add(tab)
    bad = {f: t for f, t in hits.items() if f not in ALLOWED}
    print('retail per-type tables referenced by: %d functions' % len(hits))
    for f in sorted(hits):
        print('  %-28s %-8s %s' % (f, 'ok' if f in ALLOWED else 'FAIL', ' '.join(sorted(hits[f]))))
    return not bad


if __name__ == '__main__':
    main()
