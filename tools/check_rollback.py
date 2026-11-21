"""Static checks of online stages 1 to 3 (PC port request 48, docs/ONLINE_ROLLBACK.md 10 "Stage 1 as built", 11
"Stage 2 as built" and 12 "Stage 3 as built"), of the start picker's placed cars (request 49, docs/CAR_GROUPS.md
7.4), of requests 54-55 (docs/ONLINE_ROLLBACK.md 15: the model files' node list, the drawing's words out of the
hash, the round end's peek), of request 57 (17: each machine's own words, the iteration's buffer flip and the
cameras' objects out of the hash), of section 18 (this machine's split screen and rear-view mirror in the step) and
of request 58 (19: the stores through pointers, this machine's humans), of request 59 (20: the arcs' end pointer
out of the hash, the palette shadow emptied each round) and of request 60 (21: the projectile records cleared at a
round's start, a seeker steering only at a car object).

    py -3.14 tools/check_rollback.py work/pv/<tag>_P4 [--e0 DIR] [--base DIR] [--expect-functions]
                                     [--negative-control] [--regions TSV]

On a port variant link built with `--port-define TM2_PORT_ROLLBACK` (build_source_only.py or build.py output):

1. Port functions. PORT_FUNCS are in .port_text (functions.tsv kind port) and reach no rand() (call graph of the
   R_MIPS_26 relocations). portRollbackSeedFor calls GetCurTics (the bit-clear path) and reads portRollbackOptions
   and portRollbackSeed; portRollbackPreload calls srand and portRollbackSeedFor and writes timerFrameCount.
   Stage 2's STEP_RUNNERS are in .port_text and call no rand() themselves (they run the game's own ticks and
   animators, which do): portRollbackStepEffects calls exactly STREAK_TICKS; portRollbackAnimStep reads the level
   (csGetRoot, csListHead, viewGetFollowObj) and reaches the four animators.
   Stage 3: STEP3_RUNNERS (portStepBody, portRunStep, portRollbackLoopStep) are in .port_text and call no rand()
   themselves; portStepBody calls STEP_ORDER in that order (rtMainLoop's update calls in retail's order, stage 2's
   runners at their places, the radar swap before worldUpdate) and nothing else but gameGetLevel /
   timerGetFrameTicks, and reads portResimulating (padUpdate on live steps only); portRunStep and
   portRollbackLoopStep call portStepBody only; the palette, radar and sound helpers call what they wrap.
   Request 54: portDmdMutableList reads portDmdImages and none of the live object tree (csGetRoot, csObjectPool,
   renderWorldModels: the list depends on the loaded files alone); portDmdNoteImage stores portDmdImages and calls
   nothing. Request 55: portRollbackEndDue calls timerGetFrameTicks only, reads rtScheduledEvents and the event
   count, and stores no static word (a peek).
2. No new rand(). The functions the define changes (EXPECTED, and EXPECTED_CARS_49 with TM2_PORT_CARS) call rand()
   and srand() as often as in the port variant without port defines (--base, default the latest full build's
   source_reloc_port: the variant itself already changes some functions' code against E0); the rollback port
   functions call no rand(), and the only new srand is portRollbackPreload's.
3. The seed sites. uaInit, arenaSetup and roundStart call no GetCurTics any more; each of their srand calls takes
   its argument from a portRollbackSeedFor call just before it (no call between, $a0 <- $v0 after the call) and
   each has exactly E0's number of srand calls. sessionLoadArenaAndCars calls portRollbackPreload, and the call
   dominates every other call of the function (the load's rand() draws follow the seed).
4. The gates (the flow graph of check_hazards.py: delay slots, jump tables). Each protected site is dominated by
   a call of its gate function and cannot be reached from the gate's "off" edge (the gate returned non-zero)
   without passing a gate call again, but stays reachable from the "on" edge (retail's path):
     rtMainLoop       portRollbackTop guards rtUpdateScheduledEvents; portRollbackHeld guards the step's calls
                      (STEP_CALLS with stage 2's portRollbackStepEffects and portRollbackAnimStep, worldUpdate
                      past the first); portRollbackLoopEnter dominates the first worldUpdate,
                      portRollbackFrameTime is called
     streaksDraw      portRollbackViewTickOff guards weaponDropsUpdate, weaponFlameUpdate (held, ONCE_PER_STEP);
                      portRollbackViewDrawOff guards fxSheetDisplay, weaponArcsDraw (held without ONCE_PER_STEP)
     animUpdateAll / animUpdateOne   portRollbackAnimDrawOff guards the four animators (held, ANIM_IN_STEP)
     SetFrameStart    portRollbackFixedFrame guards every store to the frame timer's words
     pauseMenu        portRollbackPauseOff guards every other call (no pad read, no loop)
   Stage 2's split functions (dominance): portRollbackPass dominates every other call of fxSheetDisplay and
   weaponArcsDraw; weaponArcsDraw keeps its rand() sites (as many as the base) and has one portRollbackVisualRand
   site per jitter site (6, after portRollbackArcVisual); each animator (ANIM_RAND_FUNCS) has as many
   portRollbackAnimVisual calls and portRollbackVisualRand sites as the base has rand() sites (its own rand() sites
   may be fewer: the compiler merges animPickPeriod's two into one); fxSheetStart calls
   portRollbackOncePerStep.
   Stage 3: rtMainLoop calls portRollbackLoopStep only on portRollbackTop's non-zero edge (dominated, never
   reachable when it returned 0); every call of the 10 sound wrappers and soundUpdate (jal, jalr: the virtual
   command / query, a tail j) is guarded by portRollbackSoundOff; worldUpdate's two CD track calls are dominated by
   portRollbackSoundOff; hudDisplayRadar's hudRadarSwapBuffers is guarded by portRollbackRadarInStep; the three
   clutAnimStart* call portClutStoreLine and the three clutAnimStep* portClutLoadLine as often as the base calls
   gpuStoreImageLine / gpuLoadImageLine, and none calls the gpu function itself; portRollbackLoopEnter calls
   portClutShadowCapture.
   Request 54: assetInstallImage calls portDmdNoteImage once, and the call dominates its relocations and its
   dbscanHierarchy calls (every installed file is noted); portRollbackPreload (the load's start) and, with the
   drop-in, portCarUnloadGroup store portDmdImages.
5. Latch order. portStepHold is read only by portRollbackTop (the latch at the top of each iteration);
   portRollbackHolding is written only by portRollbackTop and portRollbackLoopEnter and read by portRollbackHeld
   (and stage 2's portRollbackViewTickOff, portRollbackViewDrawOff, portRollbackAnimDrawOff);
   the gates in game code read none of the port's words directly (they call the functions); portRollbackQuit is
   read and cleared only by portRollbackTop, which calls rtScheduleEvent; portRollbackVisualSeed is used only by
   portRollbackVisualRand. Stage 3: portRollbackSplitting is written by portRollbackTop / portRollbackLoopEnter and
   read by the split gates; portRollbackEnded is written by portRunStep, portRollbackLoopStep and
   portRollbackLoopEnter; no C code writes portResimulating (the port's), which SOUND_GATE, CLUT_SHADOW and
   portStepBody read; portClutShadow is used only by the palette functions. Request 54: portDmdImages is used only
   by portRollbackPreload, portDmdNoteImage, portDmdMutableList and portCarUnloadGroup; viewFollowObjects (the
   camera-attached model portRollbackAnimStep walks first) is stored only by viewSetFollowObj and viewInit, which
   neither the step nor the drawing reaches (the same object on every machine: the level file's id-2 model).
6. With TM2_PORT_CARS (request 49): startNodesClearUsed calls portStartPlacedClear; humanCarPlaceAtStart and
   aiCarStart call portStartPlacedMark before their startNodePickRandom (it dominates the pick) and again after it
   (the pick dominates it); portStartNodePick reads portStartPlaced; the two port functions reach no rand().
   Request 86 (ONLINE_ROLLBACK.md 23), in check 1: portStepBody calls portStepStackScrub first; it is a frameless
   .port_text leaf; run in LinkMips it fills exactly the words below the caller's sp (64 bytes; 0x9000 capped at
   TM2_RB_SCRUB_MAX) with portStepScrubFill.
   Request 87 (SPU_VOICES.md 11), in check 1 with TM2_PORT_SPU48: soundOn calls portVoiceOwner once, before
   SsUtKeyOnV (sub_8015C7D0); SpuVmAlloc stores portVoiceCar and portVoiceKind; sndPoolAlloc reads portSoundTag and
   stores portSndReqTag; weaponFireSpecial, weaponSlamGrabUpdate and weaponSlamSwingUpdate store portSoundTag; run in
   LinkMips, portVoiceOwner gives 9 made-up requests their car and kind (a tagged special's sound, a tagged explosion:
   none, an engine by its instance or with none, a pickup's launch untagged and tagged, the flame by its instance, no
   request, voice 48: no write).
   Request 89 (ONLINE_ROLLBACK.md 25), in check 1: portStack (64 KB) with portStackGuard right below it; the EXE's
   crt0 stack word and header s_addr hold portStack's end; portVisualCameraWall uses portCameraWallWork and its frame
   is at most 0x200; main's deepest chain of direct calls (printed) fits in half the stack.
   Request 85 (SIXTEEN_CARS.md 5.5): each of the two calls portStartLinkPlace once, after its pick (the pick
   dominates it); portStartLinkPlace reaches no rand(); run in check_custom's LinkMips on a made-up route (a ring of
   8 nodes 2,400 from the centre, each linked to the next, all 8 starts taken): with a live AI car on every node the
   pick returns a link's first node, the point is the farthest of the link samples from the cars (>= 700), and
   portStartLinkPlace gives that x, y, worldGroundHeightAt's height (stubbed), the heading along the link (the link's
   ratan2) and the link's second node, once; with a car also at every link's middle, with one car only, or with the
   ground 2,000 below the route (a gap), no link start (the node, as before).
7. With --expect-functions: the game functions the port's defines changed (report.json port_changed_sections, code)
   include EXPECTED (the 35 functions request 48 stages 1-3 edit and request 54's assetInstallImage) and, with
   TM2_PORT_CARS, EXPECTED_CARS_49; with TM2_PORT_ROLLBACK as the only port define they are exactly EXPECTED.
8. rollback_regions.tsv (tools/rollback_regions.py): every row of config/US/rollback_regions.tsv (or --regions)
   resolves in the link (symbols, sections, fixed ranges; no two symbol rows overlap; every row has a reason), and
   every static word a step stores to (a LO16 / GPREL16 store relocation in a function portStepBody reaches through
   the jal call graph) lies inside a row (a "yes" range, or a row of its own). Stores into "no" rows are listed
   (the reason column says why they are left out). Pointer stores and calls through pointers are not followed.
   And (request 53 (4), ONLINE_ROLLBACK.md 14.6) every symbol of the static sections (the game's .sdata / .sbss /
   .bss, .port_bss, .port_data: bounds from the link, not from the rows) lies inside a row.
9. The hash holds no drawing state (request 54, ONLINE_ROLLBACK.md 15.2). The drawing is what rtMainLoop calls
   outside the step (DRAW_ROOTS: viewProc, streaksDraw, fxSpritesDraw, debrisDraw, renderScene, the three HUD
   drawers; each must be a call of rtMainLoop) and everything they reach through the jal graph, less the edges the
   bits cut (DRAW_PRUNE: streaksDraw's per-view ticks and its split functions, whose drawing pass stores nothing
   (check 4), the animators under animUpdateAll / animUpdateOne, hudDisplayRadar's swap). pauseMenu is the port's
   (PAUSE_PORT, or its own menu; its quit is an event, state by design). (a) Every static word the drawing stores to
   (LO16 / GPREL16 store relocations) lies in a "hash no" row or outside every row; (b) every word of DRAW_STATE
   (the words the port's re-run check found different: the drawing's, the interrupt clock's and the sound side's;
   request 57's: the pads' receive buffers, the sound library's words, the iteration's buffer flip, libgpu's control
   shadow) and of csObjectPool's per-view fields (+0x14..+0x7F, +0x124 of every element) lies in a "hash no" row.
   Request 57: rtMainLoop's own static stores and portRollbackLoopStep's (outside portStepBody: the iteration's buffer
   flip, the pacing) count as the drawing's, and so do rtMainLoop's calls outside the step (DrawOTag, PutDispEnv,
   VSync, the packet buffers: every callee the step does not reach, less the loop's own bookkeeping DRAW_LOOP_SKIP);
   (c) every pointer array of CAMERA_PTRS (the cameras' scene objects: viewProc writes their matrix per drawn view)
   has a "ptr" row with hash no. (d) Request 58 (ONLINE_ROLLBACK.md 19.4): the drawing's stores through pointers
   (PtrStores: an address formed from a LO16 / GPREL16 relocation, carried through addiu / addu / subu / or, the
   stack slots, the arguments and the callees' summaries: which arguments a function stores through, at which
   offsets, and what it returns) lie in "hash no" rows: the HUD's primitives and text sprites, hudDisplayRadar's
   arrows, the font templates, a follow sprite's position (an indexed array element is checked at element 0 plus the
   offset; a pointer loaded from memory is not followed: the cameras' objects are (c)'s). DRAW_DEAD_STORES are
   reviewed exceptions (hudDrawTargetPanel's max_hp = 1 when 0: never 0 in a battle).
10. This machine's view settings (section 18, bit 10 LOCAL_VIEWS). (a) Every function the step reaches that calls a
   VIEW_SOURCES getter (the split mode, the mirror, the render split, the sound listener's volume and pan) or loads a
   VIEW_WORDS word, less the view and sound functions of (b), goes through a forward taint over its flow graph
   (registers, stack slots, and control dependence: what a dependent branch or jump table reaches before its
   immediate post-dominator) to a fixed point: no dependent static store to a "hash yes" row, no dependent store
   through a pointer, no call in a dependent region or with a dependent argument but SOUND_SIDE, VIEW_PRESENT and the
   stage-2 gate; a function returning a dependent value becomes a source for its callers. (b) VIEW_PRESENT and
   SOUND_SIDE's functions the step reaches (with TM2_PORT_HUMANS also LOCAL_PRESENT), and all they reach, call no
   rand() and store static words only in "hash no" rows, directly or through pointers (PtrStores, request 58; less
   the views' lazy allocations VIEW_LAZY_*). (c) arenaSetup calls viewCreateWithCamera with view 0 and
   view 1 on every path (the lazy allocations never run in a battle). (d) portRollbackViewSplit: .port_text, reads
   portRollbackOptions, calls viewGetSplitMode / gameGetMode / gameGetLevel only, called by VIEW_GATED only;
   humanCarTick calls it without TM2_PORT_HUMANS (and no viewGetSplitMode), neither with it; without
   TM2_PORT_HUMANS the taint runs again with the bit modelled clear and must find exactly humanCarTick. (e) Every
   VIEW_SETTINGS word lies in a "hash no" row.
11. This machine's humans (request 58, ONLINE_ROLLBACK.md 19.1; builds with TM2_PORT_HUMANS): portHumanLocal is per
   machine by design (HUMANS16 H1). (a) The same taint as 10 (a) from the map's readers (LOCAL_SOURCES: portHumanSlot,
   portLocalHuman, portLocalHumanCount, portHumanFeedback, portHumanSide) and loads of portHumanLocal, in every step
   function that uses them, less the presentation (LOCAL_PRESENT, VIEW_PRESENT, SOUND_SIDE): no dependent state write,
   call or return (with INPUT_ALL, portHumanPadPort returns before it reads the map). (b) The INPUT_ALL gate:
   portHumanPadPort reads portHumanOptions, calls portHumanSlot only, and the call is reachable neither from its
   start without the bit test nor from the bit-set edge; modelled with the bit clear (portHumanPadPort a source) the
   taint must find exactly the human input readers LOCAL_INPUT (the check is not blind). (c) LOCAL_WORDS lie in
   "hash no" rows.
12. Request 59 (ONLINE_ROLLBACK.md 20): (a) every element's weaponArcs +0x10 (`to`) and fxSprites +0x2C (`owner_pos`),
   the words a re-run leaves another stack address in, lies in a "snapshot yes, hash no" row; (b) weaponArcsDraw stores
   a $sp-formed address (free_end) through a relocation to weaponArcs + 0x10 (the evidence of 20.1); (c)
   portClutShadowReset is in .port_text, calls nothing, addresses portClutShadow and loops over its 4194 words, and
   portClutShadowCapture calls it before any gpuStoreImageLine (each round's shadow starts empty).
13. Request 60 (ONLINE_ROLLBACK.md 21, bit 11 ROUND_CLEAN), the two port functions run on a small MIPS I interpreter
   of the link's code (MiniMips): (a) portRoundCleanProjectiles (.port_text, no calls) on pattern-filled records: with
   the bit every live record's +0x02..+0x33 and +0x38..+0x47 end 0 (life -15) and its object's +8 0, the id, object
   pointer and trail untouched, nothing else written (portCarPoolsLive 0 and 1); with the bit clear nothing written,
   and the comparison then fails (not blind); (b) portSeekerTargetOk with stubbed car tables: 1 only for a counted
   car object's +0xA8, without the bit 1 and no call; (c) roundStart's clean dominates humanCarsRestart,
   weaponProjectileUpdate's test dominates weaponProjectileSteer and the function stores 0 at +0x38; (d) the round's
   resets (21.2's table) are called on the round's path and each addresses its pool.

--negative-control on a build without TM2_PORT_ROLLBACK runs checks 1-5, which must fail (the check is not blind).
A build with one hook removed (e.g. streaksDraw's RB_VIEW_TICK or rtMainLoop's portRollbackStepEffects call) fails
check 4 or 7 (ONLINE_ROLLBACK.md 11.5 records the run). --regions with a copy of the list that lacks a row of a word
the step writes (e.g. timerFrameCount) fails check 8 (ONLINE_ROLLBACK.md 12.6); a copy without the port.data row
fails its static-symbol part alone (carViewHotkeyReleased, viewModeTable: ONLINE_ROLLBACK.md 14.6). --regions with
request 53's list (before request 54's rows) fails check 9 (ONLINE_ROLLBACK.md 15.6), and so does request 56's list
(before request 57's rows: ONLINE_ROLLBACK.md 17.5); request 57's list fails check 10 (the view settings hashed), and
a link of request 57's source without TM2_PORT_HUMANS (humanCarTick's split test ungated) fails check 10 (a) and (d)
(ONLINE_ROLLBACK.md 18.5). Request 57's list (before request 58's rows) fails check 9 (d), 10 (b) and 11 (c)
(ONLINE_ROLLBACK.md 19.5). Request 58's list fails check 12 (a) (weaponArcs.to hashed), and a link of request 58's
source (no portClutShadowReset) fails check 12 (c) and check 1 (ONLINE_ROLLBACK.md 20.5); a link of request 59's source
(no ROUND_CLEAN) fails check 13 and check 1 (ONLINE_ROLLBACK.md 21.5).
Exit status 0 = pass.
"""
import argparse
import bisect
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402
from reloc_image import Link  # noqa: E402
from check_hazards import Flow, load_functions, jump_tables, call_graph, reaches, sext16  # noqa: E402

PORT_FUNCS = {'portRollbackLoopEnter', 'portRollbackFrameTime', 'portRollbackTop', 'portRollbackHeld',
              'portRollbackFixedFrame', 'portRollbackSeedFor', 'portRollbackPreload', 'portRollbackPauseOff'}
PORT_FUNCS |= {'portRollbackOncePerStep', 'portRollbackPass', 'portRollbackViewTickOff', 'portRollbackViewDrawOff',
               'portRollbackAnimDrawOff', 'portRollbackAnimVisual', 'portRollbackArcVisual',
               'portRollbackVisualRand'}     # stage 2's gates
PORT_FUNCS |= {'portRollbackHudStep', 'portRollbackRadarInStep', 'portRollbackSoundOff', 'portSoundReconcile',
               'portClutLoadLine', 'portClutStoreLine', 'portClutShadowCapture', 'portClutAnimUpload',
               'portDmdMutableList'}    # stage 3's gates and helpers
PORT_FUNCS |= {'portDmdNoteImage', 'portRollbackEndDue'}    # requests 54 and 55
PORT_FUNCS |= {'portRollbackViewSplit'}     # section 18: LOCAL_VIEWS' gate
PORT_FUNCS |= {'portClutShadowReset'}       # request 59 (3): each round's capture starts from an empty shadow
PORT_FUNCS |= {'portRoundCleanProjectiles', 'portSeekerTargetOk'}     # request 60: ROUND_CLEAN
STEP_RUNNERS = {'portRollbackStepEffects', 'portRollbackAnimStep'}      # stage 2: run game ticks in the step
STEP3_RUNNERS = {'portStepBody', 'portRunStep', 'portRollbackLoopStep'}  # stage 3: run the whole step
# stage 3: portStepBody's calls in this order (rtMainLoop's update calls, stage 2's runners, the radar swap)
STEP_ORDER = ['rtUpdateScheduledEvents', 'padUpdate', 'targetActorsUpdate', 'fxRecordsUpdate', 'debrisUpdate',
              'lightFlickerUpdate', 'portRollbackStepEffects', 'bulletsUpdate', 'moscowEventsUpdate', 'moscowDishUpdate',
              'laEventsUpdate', 'targetTimedEventUpdate', 'eiffelTowerUpdate', 'targetLevelEventUpdate',
              'targetPlayerEventsUpdate', 'fxSpritesUpdate', 'renderFlipFrame', 'portRollbackAnimStep',
              'portRollbackHudStep', 'worldUpdate', 'rtUpdateFieldCounter', 'SetFrameStart', 'clutAnimUpdate']
SOUND_WRAPPERS = ('soundStartPlayerLoop', 'soundStopPlayerLoop', 'soundStartSharedLoop', 'soundStopSharedLoop',
                  'soundPlay', 'soundPlayVariant', 'soundPlayVariantEx', 'soundStop', 'soundStopInstance', 'soundFlush')
CLUT_STARTS = ('clutAnimStartSweep', 'clutAnimStartFadeOut', 'clutAnimStartCycle')     # read-backs
CLUT_STEPS = ('clutAnimStepSweep', 'clutAnimStepFade', 'clutAnimStepCycle')            # uploads
EXPECTED3 = set(SOUND_WRAPPERS) | {'soundUpdate', 'worldUpdate', 'hudDisplayRadar'} | set(CLUT_STARTS) | set(CLUT_STEPS)
EXPECTED = {'SetFrameStart', 'rtMainLoop', 'uaInit', 'arenaSetup', 'roundStart', 'sessionLoadArenaAndCars',
            'pauseMenu', 'streaksDraw', 'animUpdateAll', 'animUpdateOne',
            'weaponArcsDraw', 'fxSheetDisplay', 'fxSheetStart', 'animStepRandom', 'animStepWeightedRandom',
            'animPickPeriod'}
EXPECTED = EXPECTED | EXPECTED3      # the 35 functions request 48 stages 1-3 edit (rtMainLoop again in stage 3)
EXPECTED = EXPECTED | {'assetInstallImage'}     # request 54: notes each installed model file (36)
EXPECTED = EXPECTED | {'weaponProjectileUpdate'}    # request 60: ROUND_CLEAN's target test (37)
ANIM_RAND_FUNCS = ('animStepRandom', 'animStepWeightedRandom', 'animPickPeriod')
EXPECTED_CARS_49 = {'startNodesClearUsed', 'humanCarPlaceAtStart', 'aiCarStart'}
EXPECTED_VIEWS = {'humanCarTick'}    # section 18 (LOCAL_VIEWS): its regeneration's split test, without TM2_PORT_HUMANS
SEED_SITES = ('uaInit', 'arenaSetup', 'roundStart')
STEP_CALLS = {'padUpdate', 'targetActorsUpdate', 'fxRecordsUpdate', 'debrisUpdate', 'lightFlickerUpdate',
              'bulletsUpdate', 'moscowEventsUpdate', 'moscowDishUpdate', 'laEventsUpdate', 'targetTimedEventUpdate',
              'eiffelTowerUpdate', 'targetLevelEventUpdate', 'targetPlayerEventsUpdate', 'fxSpritesUpdate',
              'renderFlipFrame', 'rtUpdateFieldCounter', 'SetFrameStart', 'clutAnimUpdate',
              'portRollbackStepEffects', 'portRollbackAnimStep'}
STREAK_TICKS = {'fxSheetDisplay', 'weaponDropsUpdate', 'weaponFlameUpdate', 'weaponArcsDraw'}
STREAK_VIEW_TICKS = {'weaponDropsUpdate', 'weaponFlameUpdate'}     # skipped in every view with ONCE_PER_STEP
STREAK_SPLIT = {'fxSheetDisplay', 'weaponArcsDraw'}                 # draw only with ONCE_PER_STEP
ANIMATORS = {'animStepLoop', 'animStepPingPong', 'animStepRandom', 'animStepWeightedRandom'}
TIMER_WORDS = {'timerFrameTicks', 'timerFrameRate', 'timerFrameCount', 'timerLastFrameDelta', 'timerFrameStart',
               'timerPrevFrameStart'}
PORT_WORDS = {'portRollbackOptions', 'portResimulating', 'portStepHold', 'portStepTicks', 'portRollbackSeed',
              'portRollbackFrameCount', 'portRollbackQuit', 'portRollbackHolding', 'portRollbackReseeds',
              'portRollbackVisualSeed', 'portRollbackSplitting', 'portRollbackEnded', 'portClutShadow',
              'portDmdImages'}
# check 9 (request 54): the drawing half of rtMainLoop's iteration, and the edges the bits cut in it
DRAW_ROOTS = ('viewProc', 'streaksDraw', 'fxSpritesDraw', 'debrisDraw', 'renderScene', 'hudDrawSingleView',
              'hudDrawSplitVertical', 'hudDrawSplitHorizontal')
DRAW_PRUNE = ({('streaksDraw', f) for f in ('weaponDropsUpdate', 'weaponFlameUpdate', 'fxSheetDisplay', 'weaponArcsDraw')}
              | {(f, a) for f in ('animUpdateAll', 'animUpdateOne') for a in ('animStepLoop', 'animStepPingPong',
                                                                            'animStepRandom', 'animStepWeightedRandom')}
              | {('hudDisplayRadar', 'hudRadarSwapBuffers')})
# the words the port's re-run check found different after a re-run (request 54 (2)): never "hash yes"
DRAW_STATE = ('portMeshList', 'renderOtView1Buf0', 'renderOtView1Buf1', 'renderOtView2Buf0', 'renderOtView2Buf1',
              'renderObjectMatrices', 'renderMatrixSlots', 'renderSlotMatrixPtrs', 'renderFrustumMatricesA',
              'renderFrustumMatricesB', 'renderCurrentOt', 'renderWalkerSaveRegs', 'D_801A5398', 'D_801A5880',
              'D_801A5200', 'D_801A5380', 'D_801A5878', 'D_801A5AF4', 'D_801B2D50', 'D_801B2D66', 'D_8017E15C',
              'timerTicsAccum', 'taskMgrRunCurrent', 'soundLoopKillFrom', 'soundLoopKeepCount',
              'soundLoopCandidateCount', 'soundActiveTail', 'soundActiveCount', 'soundKeyedOnCount',
              'soundInstFreeCount', 'soundInstSearchStart', 'sndObjDistance', 'sndObjDistanceVolume',
              'sndObjVolumeRight',
              # request 57: each machine's own (the pads' receive buffers, the sound library) and the iteration's
              # buffer flip at rtMainLoop's top (a re-run leaves them), libgpu's control shadow (PutDispEnv)
              'padRecvBufferPort1', 'padRecvBufferPort2', 'portSpuExt', 'D_801A5AFC', 'D_80180EE4', 'renderBufferView1',
              'renderBufferRearView', 'D_80193950',
              # request 58: hudDisplayRadar's arrows, the HUD's message and lives lines (each machine's local humans),
              # libpress's decoder word and the renderer's shared scratch
              'hudRadarArrowUpPrims', 'hudRadarArrowDownPrims', 'hudRadarArrowLeftPrims', 'hudRadarArrowRightPrims',
              'hudMessageTextMode', 'hudMessageTextSprites', 'hudLivesTextMode', 'hudLivesTextSprites', 'dc_y',
              'renderSharedWorkBuffer')
# request 58 (d): the drawing's pointer stores into hashed rows reviewed as never taken in a battle: hudDrawTargetPanel
# sets the target's max_hp to 1 when it is 0 (S16(st, 58)); every car's stats function sets it above 0 (car_stats.c,
# carStatsDarkToothStage2) and the port's vacate / park paths keep it at 1 or more (ONLINE_ROLLBACK.md 19.4)
DRAW_DEAD_STORES = {('hudDrawTargetPanel', 'humanCars'), ('hudDrawTargetPanel', 'aiCars')}
# request 57: rtMainLoop's and portRollbackLoopStep's own stores are the iteration's (the step is portStepBody's), and
# rtMainLoop's callees outside the step are drawing roots too, less the loop's bookkeeping (the latch, the entry)
DRAW_OWN = ('rtMainLoop', 'portRollbackLoopStep')
DRAW_LOOP_SKIP = {'portRollbackLoopStep', 'portRollbackTop', 'portRollbackHeld', 'portRollbackLoopEnter',
                  'portRollbackFrameTime', 'pauseMenu', 'padSetRepeatMode', 'animEnable', 'animDisable', 'nullsub_1'}
# request 57: the pointer arrays whose objects are the cameras' (a "ptr" row each, hash no)
CAMERA_PTRS = ('viewCameraObjects', 'viewCameraObjectTable', 'viewSharedRoot', 'viewFollowObjects')
CS_STRIDE, CS_VIEW_FIELDS = 0x128, (range(0x14, 0x80, 4), (0x124,))    # csObjectPool's per-view words
STORES = {0x28, 0x29, 0x2B}     # sb, sh, sw
LOADS = {0x20, 0x21, 0x23, 0x24, 0x25}


def latest_e0():
    builds = sorted(p for p in (ROOT / 'work').glob('build-*') if (p / 'source_reloc_e0' / 'report.json').exists())
    return builds[-1] / 'source_reloc_e0' if builds else None


class Image:
    def __init__(self, out):
        self.out = out
        self.link = Link(out)
        self.rows, self.kinds = load_functions(out)
        self.tables = jump_tables(out)
        self.names_at = {r[0]: r[2] for r in self.rows}
        self.sizes = {r[2]: (r[0], r[1]) for r in self.rows}
        self.calls, self.callers, self.pointed = call_graph(self.link, self.rows)
        relocs, _ = self.link.relocations()
        self.relocs = relocs
        addr_names = {}
        for name, s in self.link.symbols.items():
            addr_names.setdefault(s['address'], set()).add(name)
        self.addr_names = addr_names

    def flow(self, fn):
        return Flow(self.link, self.sizes[fn][0], self.sizes[fn][1], self.tables, self.names_at)

    def jal_counts(self, fn, callee):
        if fn not in self.sizes or callee not in self.sizes:
            return 0
        start, size = self.sizes[fn]
        target = self.sizes[callee][0]
        n = 0
        for a in range(start, start + size, 4):
            w = self.link.word(a)
            if w >> 26 == 3 and ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2) == target:
                n += 1
        return n

    def word_refs(self, names):
        """{function: {(name, 'load'|'store'|'other')}} for the HI16/LO16/GPREL16 references of the words."""
        out = {}
        for p_c, rtype, v_c, item, sym in self.relocs:
            if rtype not in (6, 7):
                continue
            hit = (self.addr_names.get(v_c, set()) | {sym}) & names
            if not hit:
                continue
            fn = owner(self.rows, p_c)
            op = self.link.word(p_c) >> 26
            kind = 'store' if op in STORES else 'load' if op in LOADS else 'other'
            for h in hit:
                out.setdefault(fn, set()).add((h, kind))
        return out


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


def gate(img, fn, gate_fn, sites, label, dominate_only=False):
    """sites: [(address, what)]. Returns (ok, message lines)."""
    bad = []
    if fn not in img.sizes:
        return False, ['%s: %s not in the link' % (label, fn)]
    fl = img.flow(fn)
    bad += fl.problems
    gates = sorted(a for a, c in fl.calls.items() if c == gate_fn)
    if not gates:
        bad.append('no call of %s' % gate_fn)
    if not sites:
        bad.append('no protected site')
    if gates:
        free = fl.reach(fl.start, set(gates))
        for a, what in sorted(sites):
            if a in free:
                bad.append('%s at %08X reachable without %s' % (what, a, gate_fn))
        if not dominate_only:
            for g in gates:
                edges = fl.off_edge(g)
                if edges is None:
                    bad.append('gate call at %08X: no branch on its result' % g)
                    continue
                off = fl.reach(edges[0], set(gates))
                on = fl.reach(edges[1], set(gates))
                for a, what in sorted(sites):
                    if a in off:
                        bad.append('%s at %08X reachable from the off edge of %08X' % (what, a, g))
                reached = [a for a, _ in sites if a in on]
                if not reached:
                    bad.append('no protected site reachable from the on edge of %08X' % g)
            for a, what in sorted(sites):
                if not any(a in fl.reach(e[1], set(gates)) for e in
                           filter(None, (fl.off_edge(g) for g in gates))):
                    bad.append('%s at %08X unreachable from every on edge' % (what, a))
    good = not bad
    lines = ['%s: %s calls %s %dx; %d protected sites -> %s' % (label, fn, gate_fn, len(gates), len(sites),
                                                                 'ok' if good else 'FAIL')]
    lines += ['     ' + b for b in bad[:10]]
    return good, lines


def call_sites(img, fn, names):
    fl = img.flow(fn)
    return [(a, c) for a, c in fl.calls.items() if c in names]


def any_call_sites(img, fn, skip):
    """Every call site of fn but the gate: jal (by name), jalr (a virtual call) and a j out of the function (a tail
    call)."""
    fl = img.flow(fn)
    out = [(a, c) for a, c in fl.calls.items() if c not in skip]
    for a, w in fl.insn.items():
        op, funct = w >> 26, w & 63
        if op == 0 and funct == 9:
            out.append((a, 'jalr'))
        elif op == 2:
            t = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            if not fl.start <= t < fl.end:
                out.append((a, 'tail %s' % img.names_at.get(t, '%08X' % t)))
    return out


def check_stage3(img, base):
    """Check 4, stage 3: the split loop, the sound gate, the radar swap, the palette rows."""
    ok = True
    # rtMainLoop: portRollbackLoopStep only on portRollbackTop's non-zero edge
    bad = []
    fl = img.flow('rtMainLoop')
    tops = sorted(a for a, c in fl.calls.items() if c == 'portRollbackTop')
    steps = sorted(a for a, c in fl.calls.items() if c == 'portRollbackLoopStep')
    if not tops or not steps:
        bad.append('rtMainLoop: %d portRollbackTop, %d portRollbackLoopStep calls' % (len(tops), len(steps)))
    for g in tops:
        e = fl.off_edge(g)
        if e is None:
            bad.append('portRollbackTop at %08X: no branch on its result' % g)
            continue
        on0 = fl.reach(e[1], set(tops))
        on1 = fl.reach(e[0], set(tops))
        for s in steps:
            if s in on0:
                bad.append('portRollbackLoopStep at %08X reachable when portRollbackTop returned 0' % s)
            if s not in on1:
                bad.append('portRollbackLoopStep at %08X unreachable when portRollbackTop returned 1' % s)
    free = fl.reach(fl.start, set(tops))
    for s in steps:
        if s in free:
            bad.append('portRollbackLoopStep at %08X not dominated by portRollbackTop' % s)
    print('4. split step: rtMainLoop calls portRollbackLoopStep only when portRollbackTop returned 1 -> %s' % (
        'ok' if not bad else 'FAIL'))
    for b in bad[:8]:
        print('     ' + b)
    ok = not bad
    # SOUND_GATE
    for fn in SOUND_WRAPPERS + ('soundUpdate',):
        if fn not in img.sizes:
            ok = False
            print('4. sound gate: %s not in the link -> FAIL' % fn)
            continue
        sites = any_call_sites(img, fn, {'portRollbackSoundOff'})
        g, lines = gate(img, fn, 'portRollbackSoundOff', sites, '4. sound gate')
        ok = ok and g
        if not g:
            print('\n'.join(lines))
    print('4. sound gate: the %d wrappers and soundUpdate call nothing before portRollbackSoundOff returned 0 -> %s'
          % (len(SOUND_WRAPPERS), 'ok' if ok else 'FAIL'))
    sites = call_sites(img, 'worldUpdate', {'audioTakeTrackEnded', 'audioAlternateMusicTrack'})
    g, lines = gate(img, 'worldUpdate', 'portRollbackSoundOff', sites, '4. CD track test', dominate_only=True)
    if len(sites) != 2:
        g = False
        lines.append('     %d track test calls' % len(sites))
    ok = ok and g
    print('\n'.join(lines))
    # the radar swap
    sites = call_sites(img, 'hudDisplayRadar', {'hudRadarSwapBuffers'})
    g, lines = gate(img, 'hudDisplayRadar', 'portRollbackRadarInStep', sites, '4. radar swap')
    ok = ok and g
    print('\n'.join(lines))
    # CLUT_SHADOW: the animations reach VRAM only through the shadow functions
    bad = []
    for fns, gpu, port in ((CLUT_STARTS, 'gpuStoreImageLine', 'portClutStoreLine'),
                           (CLUT_STEPS, 'gpuLoadImageLine', 'portClutLoadLine')):
        for fn in fns:
            n_gpu, n_port, n_base = img.jal_counts(fn, gpu), img.jal_counts(fn, port), base.jal_counts(fn, gpu)
            if n_gpu or n_port != n_base or not n_base:
                bad.append('%s: %d %s, %d %s (base %d %s)' % (fn, n_gpu, gpu, n_port, port, n_base, gpu))
    if 'portClutShadowCapture' not in img.calls.get('portRollbackLoopEnter', set()):
        bad.append('portRollbackLoopEnter does not call portClutShadowCapture')
    print('4. palette rows: the 6 clutAnim functions read and write VRAM only through portClutStoreLine / '
          'portClutLoadLine -> %s' % ('ok' if not bad else 'FAIL'))
    for b in bad:
        print('     ' + b)
    ok = ok and not bad
    # request 54: every installed model file is noted before it is relocated and registered
    bad = []
    fn = 'assetInstallImage'
    if fn in img.sizes:
        notes = call_sites(img, fn, {'portDmdNoteImage'})
        if len(notes) != 1:
            bad.append('%s calls portDmdNoteImage %d times' % (fn, len(notes)))
        sites = call_sites(img, fn, {'modelRelocate', 'dbscanHierarchy'})
        g, lines = gate(img, fn, 'portDmdNoteImage', sites, fn, dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    else:
        bad.append('assetInstallImage not in the link')
    writers = {f for f, hits in img.word_refs({'portDmdImages'}).items() if ('portDmdImages', 'store') in hits}
    want = {'portRollbackPreload', 'portDmdNoteImage'} | ({'portCarUnloadGroup'} if 'portCarUnloadGroup' in img.sizes
                                                         else set())
    if writers != want:
        bad.append('portDmdImages written by %s (want %s)' % (sorted(writers, key=str), sorted(want)))
    print('4. model files: assetInstallImage notes each file before relocating it; the load and the unload clear '
          'portDmdImages -> %s' % ('ok' if not bad else 'FAIL'))
    for b in bad:
        print('     ' + b)
    return ok and not bad


def check_regions(img, out, regions_path):
    """Check 8: rollback_regions.tsv resolves, and every static word the step stores to lies in a row."""
    import rollback_regions as rr
    rows, problems = rr.load_rows(regions_path)
    res, p2 = rr.resolve(out, rows)
    problems += p2
    reach, todo = set(), ['portStepBody']
    while todo:
        fn = todo.pop()
        if fn in reach:
            continue
        reach.add(fn)
        todo.extend(img.calls.get(fn, ()))
    stores, uncovered, holes = 0, {}, {}
    for p_c, rtype, v_c, item, sym in img.relocs:
        if rtype not in (6, 7):
            continue
        fn = owner(img.rows, p_c)
        if fn not in reach or img.link.word(p_c) >> 26 not in STORES:
            continue
        stores += 1
        row = rr.covering(res, v_c)
        names = sorted((img.addr_names.get(v_c, set()) | {sym}) - {None, ''}) or ['%08X' % v_c]
        if row is None:
            uncovered.setdefault(names[0], set()).add(fn)
        elif row[2]['snapshot'] == 'no':
            holes.setdefault(row[2]['name'], set()).add(fn)
    # request 53 (4): every symbol of the static sections (the game's .sdata / .sbss / .bss, .port_bss, .port_data;
    # bounds taken from the link, not from the rows) lies in a row: the ranges hold them, a symbol row may cut a hole
    report = json.loads((Path(out) / 'report.json').read_text(encoding='utf-8'))
    secs = {s['name']: s for s in (report.get('port_variant') or {}).get('sections', [])}
    bounds = [rr.game_static(out)]
    for name in ('.port_bss', '.port_data'):
        if name in secs and secs[name]['size']:
            lo = int(secs[name]['address'], 16)
            bounds.append((lo, lo + secs[name]['size']))
    syms, _ = rr.link_symbols(out)
    static_syms = {n: a for n, a in syms.items() if any(lo <= a < hi for lo, hi in bounds)}
    outside = sorted(n for n, a in static_syms.items() if rr.covering(res, a) is None)
    good = not problems and not uncovered and not outside and 'portStepBody' in reach and len(reach) > 50
    print('8. rollback_regions.tsv: %d rows resolved (%s); the step (%d functions from portStepBody) stores to %d '
          'static words, %d outside every row; %d symbols of the static sections, %d outside every row -> %s'
          % (len(res), Path(regions_path).name, len(reach), stores, len(uncovered), len(static_syms), len(outside),
             'ok' if good else 'FAIL'))
    for p in problems[:10]:
        print('     ' + p)
    for name in sorted(uncovered)[:20]:
        print('     not covered: %s (stored by %s)' % (name, ', '.join(sorted(uncovered[name])[:4])))
    if outside:
        print('     static symbols in no row: %s%s' % (', '.join(outside[:12]), ' ...' if len(outside) > 12 else ''))
    if holes:
        print('     stored but not snapshotted (by design, the reason column): %s' % ', '.join(sorted(holes)))
    return good


def check_draw_hash(img, out, regions_path):
    """Check 9 (request 54): no "hash yes" row holds a word the drawing writes."""
    import bisect
    import rollback_regions as rr
    rows, problems = rr.load_rows(regions_path)
    res, p2 = rr.resolve(out, rows)
    problems += p2
    bad = []
    roots = set(img.calls.get('rtMainLoop', set()))
    missing = [r for r in DRAW_ROOTS if r not in roots]
    if missing:
        bad.append('rtMainLoop does not call %s' % missing)
    step, todo = set(), ['portStepBody']
    while todo:
        f = todo.pop()
        if f not in step:
            step.add(f)
            todo.extend(img.calls.get(f, ()))
    # request 57: rtMainLoop's callees outside the step (the frame's presentation: DrawOTag, PutDispEnv, VSync, ...)
    extra = sorted(r for r in roots if r not in step and r not in DRAW_LOOP_SKIP and r not in DRAW_ROOTS)
    if len(step) < 50:
        bad.append('portStepBody reaches %d functions' % len(step))
    draw, todo = set(), list(DRAW_ROOTS) + extra
    while todo:
        f = todo.pop()
        if f in draw:
            continue
        draw.add(f)
        todo.extend(c for c in img.calls.get(f, ()) if (f, c) not in DRAW_PRUNE)
    hashed, outside, own = {}, set(), 0
    for p_c, rtype, v_c, item, sym in img.relocs:
        if rtype not in (6, 7):
            continue
        fn = owner(img.rows, p_c)
        if (fn not in draw and fn not in DRAW_OWN) or img.link.word(p_c) >> 26 not in STORES:
            continue
        own += fn in DRAW_OWN
        row = rr.covering(res, v_c)
        names = sorted((img.addr_names.get(v_c, set()) | {sym}) - {None, ''}) or ['%08X' % v_c]
        if row is None:
            outside.add(names[0])
        elif row[2]['hash'] == 'yes':
            hashed.setdefault('%s (%s)' % (names[0], row[2]['name']), set()).add(fn)
    for name, fns in sorted(hashed.items()):
        bad.append('the drawing stores to hashed %s: %s' % (name, ', '.join(sorted(fns)[:4])))
    syms, addrs = rr.link_symbols(out)
    named = 0
    for name in DRAW_STATE:
        a = syms.get(name)
        if a is None:
            continue    # a port define's symbol (portMeshList: TM2_PORT_FAR_DISTANCE) not in this link
        named += 1
        row = rr.covering(res, a)
        if row is None or row[2]['hash'] == 'yes':
            bad.append('%s is hashed (%s)' % (name, row[2]['name'] if row else 'no row'))
    a = syms.get('csObjectPool')
    if a is None:
        bad.append('csObjectPool not in the link')
    else:
        i = bisect.bisect_right(addrs, a)
        n = (addrs[i] - a) // CS_STRIDE if i < len(addrs) else 0
        hit = 0
        for e in range(n):
            for off in list(CS_VIEW_FIELDS[0]) + list(CS_VIEW_FIELDS[1]):
                row = rr.covering(res, a + e * CS_STRIDE + off)
                if row is None or row[2]['hash'] == 'yes':
                    hit += 1
        if hit or not n:
            bad.append('csObjectPool: %d per-view words of %d elements hashed' % (hit, n))
    # (c) request 57: the cameras' objects (viewProc's matrix per drawn view) are named through their pointer arrays
    cams = 0
    for name in CAMERA_PTRS:
        a = syms.get(name)
        if a is None:
            bad.append('%s not in the link' % name)
            continue
        ptr = [r for x, s, r in res if r['kind'] == 'ptr' and x == a]
        if not ptr or ptr[0]['hash'] == 'yes':
            bad.append('%s: no "ptr" row with hash no (the cameras\' objects hashed)' % name)
        else:
            cams += 1
    if own < 3:
        bad.append('rtMainLoop / portRollbackLoopStep store %d static words (the buffer flip expected)' % own)
    # (d) request 58: the drawing's stores through pointers (the HUD's primitives and text, the arrows, the fonts)
    ph, pseen, pskip = ptr_hashed(img, out, res, sorted(draw | set(DRAW_OWN)), DRAW_DEAD_STORES)
    for name, fns in sorted(ph.items()):
        bad.append('the drawing stores through a pointer to hashed %s: %s' % (name, ', '.join(sorted(fns)[:4])))
    if pseen < 200:
        bad.append('%d pointer stores seen (the census is blind)' % pseen)
    good = not problems and not bad and len(draw) > 50
    print('9. drawing state out of the hash: the drawing (%d functions from %d roots and %d other calls of rtMainLoop, '
          'its and portRollbackLoopStep\'s own %d stores) stores to %d hashed static words, through pointers to %d '
          '(%d pointer stores, %d reviewed); %d of the request\'s %d words and csObjectPool\'s per-view fields '
          'hashed; %d of %d camera pointer arrays out (%s) -> %s'
          % (len(draw), len(DRAW_ROOTS), len(extra), own, len(hashed), len(ph), pseen, pskip,
             sum(1 for b in bad if ' is hashed' in b), named, cams, len(CAMERA_PTRS), Path(regions_path).name,
             'ok' if good else 'FAIL'))
    for b in (problems + bad)[:24]:
        print('     ' + b)
    if outside:
        print('     drawing stores outside every row (not hashed): %s' % ', '.join(sorted(outside)[:12]))
    return good


# ---- check 10 (ONLINE_ROLLBACK.md 18): this machine's view settings in the step ----
# The sources: what reads this machine's split screen, rear-view mirror and sound listener (the listener follows the
# view's camera: each machine's own whatever the split); a call's result or a load of the word is dependent
VIEW_SOURCES = {'viewGetSplitMode', 'rtGetRearViewActive', 'viewGetRenderSplitMode', 'PadChkMtap', 'soundGetListenerPan'}
VIEW_WORDS = {'viewSplitScreenMode', 'rtRearViewActive', 'rtRearViewAllowed', 'viewRenderSplitMode', 'sndListenerDist',
              'sndListenerPan'}
# the per-machine view settings and what their hotkeys write: each must lie in a "hash no" row
VIEW_SETTINGS = ('viewSplitScreenMode', 'rtRearViewActive', 'rtRearViewAllowed', 'viewRenderSplitMode', 'viewFov',
                 'viewModes', 'viewEnvs', 'viewExtraViewsUnlocked', 'hudRadarVisible', 'hudWeaponListVisible',
                 'carViewHotkeyReleased')
# the sound side: where a dependent value may go (the wrappers' results are discarded by every caller: 15.2)
SOUND_SIDE = set(SOUND_WRAPPERS) | {'soundLocateForView0', 'soundLocateForView', 'sndListenerTransform',
                                    'portCarTornadoSound', 'portCarFlameSound', 'portCarEngineUpdate',
                                    'portHumanLoopPlay', 'portHumanLocate'}
OUT_PARAMS = {'portHumanLocate'}    # writes the listener's values through its pointer arguments (the caller's stack)
# presentation as a whole: the view hotkeys and toggles, the cameras, the HUD (their pointer stores: the hotkey latch,
# the human records' camera block, the HUD's primitives; their static stores are checked against the rows)
VIEW_PRESENT = {'carViewHotkeys', 'carSetViewMode', 'rtToggleRearView', 'rtSetRearView', 'rtRearViewOff',
                'viewSetSplitMode', 'viewCreateWithCamera', 'renderGetView0Frames', 'renderGetView1Frames',
                'hudInitPanels', 'hudToggleRadar', 'hudToggleWeaponList', 'hudRadarPlotCar', 'viewChaseCamera',
                'viewCarCameraUpdate', 'worldHumanCamera'}
VIEW_GATES = {'portRollbackOncePerStep'}     # stage 2's gate in fxSheetStart (calls nothing, stores nothing)
# the views' lazy allocations: viewCreate / viewCreateWithCamera allocate a view's camera objects and the shared root
# only while they are null; arenaSetup creates views 0 and 1 on every path (10 (c)), so a battle's split toggle finds
# them and these never run in a step
VIEW_LAZY_CALLS = {('viewCreate', 'csAlloc'), ('viewCreateWithCamera', 'csAlloc')}
VIEW_LAZY_STORES = {('viewCreate', 'viewSharedRoot')}
VIEW_GATED = {'humanCarTick'}       # LOCAL_VIEWS' sites (portRollbackViewSplit) in builds without TM2_PORT_HUMANS
# check 11 (request 58): this machine's humans (portHumanLocal, HUMANS16 H1), builds with TM2_PORT_HUMANS
LOCAL_SOURCES = {'portHumanSlot', 'portLocalHuman', 'portLocalHumanCount', 'portHumanFeedback', 'portHumanSide'}
LOCAL_WORDS = {'portHumanLocal'}
# the presentation by locality (H1 / H3): the gates of a local human's own feedback, the sound sides, the radar plot,
# the camera and follow gates, the AI blips, the engine cap; hudMessagePost / soundSetDeniedPending (what the
# feedback gates call: the HUD message, the denied buzz)
LOCAL_PRESENT = {'portHumanMessage', 'portHumanSound', 'portHumanDenied', 'portHumanLoopPlay', 'portHumanLoopStop',
                 'portHumanRadarPlot', 'worldHumanCamera', 'worldHumanFollow', 'worldRadarAiAll', 'portCarSoundFrame',
                 'hudMessagePost', 'soundSetDeniedPending'}
LOCAL_GATE = 'portHumanPadPort'     # returns -1 before it reads the map when portHumanOptions bit 0 (INPUT_ALL) is set
# with INPUT_ALL clear: the H2 input functions (a local slot 0 / 1 reads its pad) and the game's readers of the input
LOCAL_INPUT = {'portHumanAction', 'portHumanCombo', 'portHumanAnyLatched', 'humanCarTick', 'carReadMoveCodes',
               'humanCarReadInput'}
# the per-machine words of the humans: each must lie in a "hash no" row (humanCars' camera block: one row per record)
LOCAL_ROWS = ('portHumanLocal', 'portHumanRadarBlink', 'soundDeniedPending')
LOCAL_FIELDS = {'humanCars': (0x814, (0x6, 0xC, 0x10, 0x80, 0x88, 0x10C, 0x110))}
R_SP, R_RA, R_HI, R_LO = 29, 31, 32, 33
CALLER_SAVED = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, R_HI, R_LO)


def regmask(regs):
    m = 0
    for r in regs:
        m |= 1 << r
    return m


def mips_du(w):
    """(defs, uses, kind) of one instruction: kind load, store, branch, jr, jalr, j, jal, alu or None."""
    op, rs, rt, rd, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
    if op == 0:
        if fn in (0, 2, 3):
            return [rd], [rt], 'alu'
        if fn in (4, 6, 7) or 0x20 <= fn <= 0x2B:
            return [rd], [rs, rt], 'alu'
        if fn == 8:
            return [], [rs], 'jr'
        if fn == 9:
            return [rd], [rs], 'jalr'
        if fn in (0x10, 0x12):
            return [rd], [R_HI if fn == 0x10 else R_LO], 'alu'
        if fn in (0x11, 0x13):
            return [R_HI if fn == 0x11 else R_LO], [rs], 'alu'
        if 0x18 <= fn <= 0x1B:
            return [R_HI, R_LO], [rs, rt], 'alu'
        return [], [], None
    if op == 1:
        return ([R_RA] if rt in (0x10, 0x11) else []), [rs], 'branch'
    if op in (4, 5, 20, 21):
        return [], [rs, rt], 'branch'
    if op in (6, 7, 22, 23):
        return [], [rs], 'branch'
    if op == 2:
        return [], [], 'j'
    if op == 3:
        return [R_RA], [], 'jal'
    if 8 <= op <= 14:
        return [rt], [rs], 'alu'
    if op == 15:
        return [rt], [], 'alu'
    if 0x20 <= op <= 0x26:
        return [rt], [rs], 'load'
    if op in (0x28, 0x29, 0x2A, 0x2B, 0x2E):
        return [], [rs, rt], 'store'
    if op == 0x12:
        return ([rt], [], 'alu') if rs in (0, 2) else ([], [rt], 'alu')
    return [], [], None


def control_regions(fl):
    """{node: set of nodes control-dependent on it} for every node with two or more successors (a branch's delay
    slot, a jump table's jr): the nodes its successors reach before its immediate post-dominator."""
    nodes = sorted(fl.insn)
    idx = {a: i for i, a in enumerate(nodes)}
    n = len(nodes)
    full = (1 << (n + 1)) - 1
    succ = {}
    for i, a in enumerate(nodes):
        s = [idx[t] for t in fl.succ.get(a, ()) if t in idx]
        succ[i] = s or [n]          # n: the exit
    pd = {i: full for i in range(n)}
    pd[n] = 1 << n
    changed = True
    while changed:
        changed = False
        for i in reversed(range(n)):
            m = full
            for s in succ[i]:
                m &= pd[s]
            m |= 1 << i
            if m != pd[i]:
                pd[i], changed = m, True
    out = {}
    for i, a in enumerate(nodes):
        if len(set(succ[i])) < 2:
            continue
        strict, cands = pd[i] & ~(1 << i), []
        while strict:
            b = strict & -strict
            cands.append(b.bit_length() - 1)
            strict ^= b
        ip = next((c for c in cands if all(pd[c] >> o & 1 for o in cands)), None)
        stop = {nodes[ip]} if ip is not None and ip != n else set()
        reg = set()
        for s in set(succ[i]):
            if s != n:
                reg |= fl.reach(nodes[s], stop)
        out[a] = reg
    return out


class ViewTaint:
    """Forward taint of a function: the values of the sources (calls, loads of the words), the stack slots they are
    stored to, and the nodes control-dependent on a dependent branch (control_regions), to a fixed point."""

    def __init__(self, img, res, sources, words, allowed):
        self.img, self.res, self.sources, self.words, self.allowed = img, res, set(sources), set(words), set(allowed)
        self.lo, self.names = {}, {}
        for p, t, v, it, s in img.relocs:
            if t in (6, 7):
                self.lo[p] = v
                self.names[p] = sorted((img.addr_names.get(v, set()) | {s}) - {None, ''}) or ['%08X' % v]

    def step(self, fl, a, s, ct, stack, calls=True):
        w = fl.insn[a]
        d, u, k = mips_du(w)
        dep = bool(s & regmask(u)) or a in ct
        if k == 'load':
            if ((w >> 21) & 31) == R_SP and sext16(w & 0xFFFF) in stack:
                dep = True
            if a in self.lo and set(self.names[a]) & self.words:
                dep = True
        elif k == 'store':
            if ((w >> 21) & 31) == R_SP and (s >> ((w >> 16) & 31) & 1 or a in ct):
                stack.add(sext16(w & 0xFFFF))
            d = []
        if d and k in ('alu', 'load', 'jalr'):
            s = (s | regmask(d)) if dep else (s & ~regmask(d))
        if calls and a - 4 in fl.calls:
            callee = fl.calls[a - 4]
            if callee in OUT_PARAMS:        # its pointer arguments: the caller's stack slots ($aN = $sp + imm)
                for b in range(a - 40, a + 4, 4):
                    x = fl.insn.get(b)
                    if x is not None and x >> 26 == 9 and (x >> 21) & 31 == R_SP and 4 <= (x >> 16) & 31 <= 7:
                        stack.add(sext16(x & 0xFFFF))
            s &= ~regmask(CALLER_SAVED)
            if callee in self.sources or (a - 4 in ct and callee in self.allowed):
                s |= regmask((2, 3))
        return s & ~1

    def run(self, fn):
        fl = self.img.flow(fn)
        regions = control_regions(fl)
        ct, stack = set(), set()
        while True:
            state, todo = {fl.start: 0}, [fl.start]
            before = set(stack)
            while todo:
                a = todo.pop()
                o = self.step(fl, a, state[a], ct, stack)
                for b in fl.succ.get(a, ()):
                    if fl.start <= b < fl.end and (b not in state or state[b] | o != state[b]):
                        state[b] = state.get(b, 0) | o
                        todo.append(b)
            grew = False
            for a, w in fl.insn.items():
                if a not in state:
                    continue
                d, u, k = mips_du(w)
                if k in ('branch', 'jr') and (state[a] & regmask(u) or a in ct):
                    reg = regions.get(a + 4, set())
                    if not reg <= ct:
                        ct |= reg
                        grew = True
            if not grew and stack == before:
                break
        return self.findings(fl, state, ct, stack), ct

    def findings(self, fl, state, ct, stack):
        """[(address, what)]; 'returns' = the function returns a dependent value (its callers are checked)."""
        import rollback_regions as rr
        bad = []
        for a, w in sorted(fl.insn.items()):
            if a not in state:
                continue
            s = state[a]
            d, u, k = mips_du(w)
            rs = (w >> 21) & 31
            if k == 'store' and (a in ct or s & regmask(u)) and rs != R_SP:
                if a in self.lo:
                    row = rr.covering(self.res, self.lo[a])
                    if row is None or row[2]['hash'] == 'yes':
                        bad.append((a, 'store to hashed %s' % self.names[a][0]))
                else:
                    bad.append((a, 'store through $%d+0x%X' % (rs, sext16(w & 0xFFFF) & 0xFFFF)))
            elif k == 'jalr' and (a in ct or s & regmask(u)):
                bad.append((a, 'call through a pointer'))
            elif k == 'jr' and rs == R_RA and s & regmask((2,)):
                bad.append((a, 'returns'))
            elif k == 'j':
                t = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                if not fl.start <= t < fl.end:
                    callee = self.img.names_at.get(t, '%08X' % t)
                    s2 = self.step(fl, a + 4, state.get(a + 4, s), ct, set(stack), calls=False)
                    if callee not in self.allowed and (a in ct or s2 & regmask((4, 5, 6, 7))):
                        bad.append((a, 'tail call %s' % callee))
            if a - 4 in fl.calls:
                callee = fl.calls[a - 4]
                if callee in self.allowed or callee in self.sources:
                    continue
                s2 = self.step(fl, a, s, ct, set(stack), calls=False)
                if a - 4 in ct:
                    bad.append((a - 4, 'call %s in a dependent region' % callee))
                elif s2 & regmask((4, 5, 6, 7)):
                    bad.append((a - 4, 'call %s with a dependent argument' % callee))
        return bad


class PtrStores:
    """Request 58 (check 9 (d), 10 (b), 11): the static words a function stores to through pointers. A value is an
    address formed from a LO16 / GPREL16 relocation (addiu / ori: ('s', address, offset known 0 | None, origin)) or
    an argument (('a', i, offset | None): $a0..$a3 and the first four stack words), carried through addiu, addu /
    subu / or (offset unknown), the stack slots (flow-insensitive) and the callees' summaries (the arguments a
    function stores through, at which offsets, and what it returns), to a fixed point. More than WIDEN values in a
    register, or an address leaving the symbol it was formed from (a loop's pointer past the end), become the
    symbol's element 0 with the offset unknown; a store then counts at element 0 plus the instruction's offset. A
    pointer loaded from memory and calls through pointers are not followed."""
    def __init__(self, img, starts):
        self.img = img
        self.starts = starts            # sorted symbol addresses of the link
        self.lo = {p: v for p, t, v, it, s in img.relocs if t in (6, 7)}
        self.summ = {}                  # fn -> (frozenset of (arg, offset|None) stored through, frozenset of returns)
        self.flows = {}
        self.done = set()

    def flow(self, fn):
        if fn not in self.flows:
            self.flows[fn] = self.img.flow(fn)
        return self.flows[fn]

    def sym_start(self, addr):
        i = bisect.bisect_right(self.starts, addr) - 1
        return self.starts[i] if i >= 0 else addr

    def widen(self, vals):
        if len(vals) <= PTR_WIDEN:
            return vals
        return frozenset((('s', self.sym_start(v[3]), None, v[3]) if v[0] == 's' else (v[0], v[1], None)) for v in vals)

    def add(self, v, imm):
        if v[2] is None:
            return v
        if v[0] == 'a':
            return (v[0], v[1], v[2] + imm)
        i = bisect.bisect_right(self.starts, v[3]) - 1
        lo = self.starts[i] if i >= 0 else v[3]
        hi = self.starts[i + 1] if i + 1 < len(self.starts) else 1 << 32
        # an address that leaves the symbol it was formed from (a loop's pointer past the end): element 0, inexact
        return ('s', v[1] + imm, 0, v[3]) if lo <= v[1] + imm < hi else ('s', lo, None, v[3])

    @staticmethod
    def blur(v):
        return v[:2] + (None,) + v[3:]

    def analyse(self, fn, seed_args=True):
        """(stores [(insn, address, exact, callee)], args stored through {(i, offset)}, returned values)."""
        fl = self.flow(fn)
        frame = 0
        for b in range(fl.start, min(fl.start + 64, fl.end), 4):
            x = fl.insn[b]
            if x >> 16 == 0x27BD and x & 0x8000:
                frame = -sext16(x & 0xFFFF)
                break
        ctx = {'frame': frame, 'seed': seed_args}
        init = {4 + i: frozenset({('a', i, 0)}) for i in range(4)} if seed_args else {}
        state, stack = {fl.start: init}, {}
        todo = [fl.start]
        while True:
            before = dict(stack)
            while todo:
                a = todo.pop()
                o = self.step(fl, a, dict(state[a]), stack, ctx, None)
                for b in fl.succ.get(a, ()):
                    if not fl.start <= b < fl.end:
                        continue
                    old = state.get(b)
                    if old is None:
                        state[b] = o
                        todo.append(b)
                        continue
                    new, ch = dict(old), False
                    for r, v in o.items():
                        nv = self.widen(old.get(r, frozenset()) | v)
                        if nv != old.get(r, frozenset()):
                            new[r], ch = nv, True
                    if ch:
                        state[b] = new
                        todo.append(b)
            if stack == before:
                break
            todo = list(state)
        rec = ([], set(), set())
        for a in sorted(state):
            self.step(fl, a, dict(state[a]), stack, ctx, rec)
        return rec[0], frozenset(rec[1]), frozenset(rec[2])

    @staticmethod
    def val(regs, r):
        return frozenset() if r == 0 else regs.get(r, frozenset())

    def record(self, rec, a, v, off, callee):
        if v[0] == 's':
            # an inexact address: element 0 of the symbol it was formed from (the origin), plus the known offset
            base = v[1] if v[2] is not None else self.sym_start(v[3])
            rec[0].append((a, base + (off or 0), v[2] is not None and off is not None, callee))
        else:
            rec[1].add((v[1], None if v[2] is None or off is None else v[2] + off))

    def step(self, fl, a, regs, stack, ctx, rec):
        w = fl.insn[a]
        op, rs, rt, rd, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
        imm = sext16(w & 0xFFFF)
        if a in self.lo and op in (9, 0x0D):
            regs[rt] = frozenset({('s', self.lo[a], 0, self.lo[a])})
        elif op == 9 and rt:
            regs[rt] = frozenset(self.add(v, imm) for v in self.val(regs, rs))
        elif op == 0 and fn in (0x21, 0x23, 0x25) and rd:
            vs, vt = self.val(regs, rs), self.val(regs, rt)
            regs[rd] = vs if rt == 0 else vt if rs == 0 else frozenset(self.blur(v) for v in vs | vt)
        else:
            d, u, k = mips_du(w)
            if k == 'load':
                v = frozenset()
                if rs == R_SP and op == 0x23:
                    v = stack.get(imm, frozenset())
                    if ctx['seed'] and ctx['frame'] and ctx['frame'] + 16 <= imm < ctx['frame'] + 32:
                        v = v | {('a', 4 + (imm - ctx['frame'] - 16) // 4, 0)}
                for r in d:
                    regs[r] = v
            elif k == 'store':
                if rs == R_SP:
                    if op == 0x2B and self.val(regs, rt):
                        stack[imm] = self.widen(stack.get(imm, frozenset()) | self.val(regs, rt))
                elif rec is not None:
                    for v in self.val(regs, rs):
                        self.record(rec, a, v, imm, None)
            else:
                for r in d:
                    if r:
                        regs[r] = frozenset()
        callee = None
        if a - 4 in fl.calls:
            callee = fl.calls[a - 4]
        else:
            pw = fl.insn.get(a - 4)
            if pw is not None and pw >> 26 == 2:
                t = (a & 0xF0000000) | ((pw & 0x3FFFFFF) << 2)
                if not fl.start <= t < fl.end:
                    callee = self.img.names_at.get(t, '%08X' % t)
        if callee is not None:
            sp, sr = self.summ.get(callee, (frozenset(), frozenset()))
            args = [self.val(regs, 4 + i) for i in range(4)] + [stack.get(16 + 4 * i, frozenset()) for i in range(4)]
            if rec is not None:
                for i, off in sp:
                    for v in args[i]:
                        self.record(rec, a - 4, v, off, callee)
            for r in PTR_CALLER:
                regs.pop(r, None)
            rv = set()
            for v in sr:
                if v[0] == 's':
                    rv.add(v)
                else:
                    rv |= {self.add(x, v[2]) if v[2] is not None else self.blur(x) for x in args[v[1]]}
            if rv:
                regs[2] = self.widen(frozenset(rv))
        if rec is not None and op == 0 and fn == 8 and rs == R_RA:
            rec[2].update(self.val(regs, 2))
        regs.pop(0, None)
        return regs

    def summarise(self, fns):
        fns = sorted(f for f in fns if f in self.img.sizes and f not in self.done)
        self.done |= set(fns)
        n, changed = 0, True
        while changed and n < 20:
            changed = False
            n += 1
            for f in fns:
                st, p, r = self.analyse(f)
                if self.summ.get(f) != (p, r):
                    self.summ[f] = (p, r)
                    changed = True
        return n


PTR_CALLER = set(range(1, 16)) | {24, 25}
PTR_WIDEN = 8


def ptr_stores(img, out):
    """The image's PtrStores (one per image: the summaries are shared by checks 9, 10 and 11)."""
    import rollback_regions as rr
    if getattr(img, 'ptr', None) is None:
        syms, _ = rr.link_symbols(out)
        img.ptr = PtrStores(img, sorted(set(syms.values())))
    return img.ptr


def ptr_hashed(img, out, res, fns, skip=()):
    """{symbol: {function[->callee]}}: the stores through pointers of fns (and what their callees store through
    their arguments) that land in a "hash yes" row; skip: {(function, symbol)} reviewed. Returns
    (hashed, the number of pointer stores seen, the skipped hits)."""
    import rollback_regions as rr
    P = ptr_stores(img, out)
    closure_, todo = set(), [f for f in fns if f in img.sizes]
    while todo:
        f = todo.pop()
        if f not in closure_:
            closure_.add(f)
            todo.extend(c for c in img.calls.get(f, ()) if c in img.sizes)
    P.summarise(closure_)
    syms, _ = rr.link_symbols(out)
    names = {}
    for n, a in syms.items():
        names.setdefault(a, n)
    hashed, seen, skipped = {}, 0, 0
    for f in sorted(fns):
        if f not in img.sizes:
            continue
        stores, _, _ = P.analyse(f, seed_args=False)
        for a, addr, exact, callee in stores:
            seen += 1
            row = rr.covering(res, addr)
            if row is None or row[2]['hash'] == 'no':
                continue        # outside every row: neither snapshotted nor hashed
            name = names.get(P.sym_start(addr), '%08X' % addr)
            if (f, name) in skip:
                skipped += 1
                continue
            hashed.setdefault(name, set()).add('%s%s' % (f, ' -> ' + callee if callee else ''))
    return hashed, seen, skipped


def call_arg(fl, call, reg):
    """The constant a call passes in $reg (addiu / ori $reg, $zero, imm or addu / or $reg, $zero, $zero in the call's
    delay slot or the 6 instructions before it), else None."""
    for b in [call + 4] + list(range(call - 4, call - 28, -4)):
        w = fl.insn.get(b)
        if w is None:
            continue
        op, rs, rt, rd, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
        if op in (9, 0x0D) and rt == reg:
            return (sext16(w & 0xFFFF) if op == 9 else w & 0xFFFF) if rs == 0 else None
        if op == 0 and rd == reg and fn in (0x21, 0x25) and rs == 0 and rt == 0:
            return 0
        if reg in mips_du(w)[0]:
            return None
    return None


def check_views(img, out, regions_path, humans, bit_clear=False):
    """Check 10: with LOCAL_VIEWS no step-reachable read of this machine's view settings reaches a state write or a
    rand() except through portRollbackViewSplit. bit_clear: model the bit clear (portRollbackViewSplit = this
    machine's split): the gated sites must then be found (the check is not blind)."""
    import rollback_regions as rr
    rows, problems = rr.load_rows(regions_path)
    res, p2 = rr.resolve(out, rows)
    problems += p2
    bad = []
    step, todo = set(), ['portStepBody']
    while todo:
        f = todo.pop()
        if f not in step:
            step.add(f)
            todo.extend(img.calls.get(f, ()))
    sources = set(VIEW_SOURCES) | OUT_PARAMS | ({'portRollbackViewSplit'} if bit_clear else set())
    allowed = VIEW_SOURCES | SOUND_SIDE | VIEW_PRESENT | VIEW_GATES
    exempt = VIEW_SOURCES | SOUND_SIDE | VIEW_PRESENT | {'portRollbackViewSplit'}
    users = set()
    for s in sources:
        users |= set(img.callers.get(s, ()))
    users |= set(img.word_refs(VIEW_WORDS))
    users = {u for u in users if u in step and u not in exempt}
    sites = sum(img.jal_counts(u, s) for u in users for s in sources)
    # (a) every user, and the callers of a user that returns a dependent value
    tool = ViewTaint(img, res, sources, VIEW_WORDS, allowed)
    found, returns, done, nct = {}, set(), set(), 0
    todo = sorted(users)
    while todo:
        fn = todo.pop()
        if fn in done or fn not in img.sizes:
            continue
        done.add(fn)
        got, ct = tool.run(fn)
        nct += len(ct)
        for a, what in got:
            if what == 'returns':
                if fn not in returns:
                    returns.add(fn)
                    tool.sources.add(fn)
                    todo.extend(c for c in img.callers.get(fn, ()) if c in step and c not in exempt)
            else:
                found.setdefault(fn, []).append((a, what))
    if bit_clear:
        want = {f for f in VIEW_GATED if 'portRollbackViewSplit' in img.calls.get(f, set())}
        good = bool(want) and set(found) == want
        print('10. negative model (LOCAL_VIEWS clear: portRollbackViewSplit is this machine\'s split): state writes '
              'found in %s (want %s) -> %s' % (sorted(found), sorted(want), 'ok' if good else 'FAIL'))
        for fn in sorted(found):
            for a, what in found[fn][:4]:
                print('     %s %08X %s' % (fn, a, what))
        return good
    for fn in sorted(found):
        for a, what in found[fn][:6]:
            bad.append('%s %08X: %s' % (fn, a, what))
    # (b) the presentation and sound functions the step reaches: no rand(), static stores in "hash no" rows only
    pres = sorted((VIEW_PRESENT | SOUND_SIDE | (LOCAL_PRESENT if humans else set())) & step)
    reach, todo = set(), list(pres)
    while todo:
        f = todo.pop()
        if f not in reach:
            reach.add(f)
            todo.extend(c for c in img.calls.get(f, ()) if (f, c) not in VIEW_LAZY_CALLS)
    if 'rand' in reach or 'srand' in reach:
        bad.append('the view and sound side reaches rand()')
    hashed, lazy_hit = {}, 0
    for p, t, v, it, sy in img.relocs:
        if t not in (6, 7):
            continue
        f = owner(img.rows, p)
        if f not in reach or img.link.word(p) >> 26 not in STORES:
            continue
        names = sorted((img.addr_names.get(v, set()) | {sy}) - {None, ''}) or ['%08X' % v]
        if any((f, n) in VIEW_LAZY_STORES for n in names):
            lazy_hit += 1
            continue
        row = rr.covering(res, v)
        if row is None or row[2]['hash'] == 'yes':
            hashed.setdefault(names[0], set()).add(f)
    for name, fns in sorted(hashed.items()):
        bad.append('the view and sound side stores to hashed %s (%s)' % (name, ', '.join(sorted(fns)[:4])))
    # request 58: and through pointers (the HUD's primitives, the human records' camera block, the pose objects' table)
    ph, pseen, _ = ptr_hashed(img, out, res, sorted(reach))
    for name, fns in sorted(ph.items()):
        bad.append('the view and sound side stores through a pointer to hashed %s (%s)' % (
            name, ', '.join(sorted(fns)[:4])))
    # (c) arenaSetup creates views 0 and 1 on every path (the lazy allocations never run in a step)
    if 'arenaSetup' in img.sizes:
        fl = img.flow('arenaSetup')
        exits = {a for a, w in fl.insn.items() if w >> 26 == 0 and w & 63 == 8 and (w >> 21) & 31 == R_RA}
        for view in (0, 1):
            cs = {a for a, c in fl.calls.items() if c == 'viewCreateWithCamera' and call_arg(fl, a, 6) == view}
            if not cs or fl.reach(fl.start, cs) & exits:
                bad.append('arenaSetup: a path without viewCreateWithCamera(.., %d)' % view)
    else:
        bad.append('arenaSetup not in the link')
    # (d) the gate and its sites
    g = 'portRollbackViewSplit'
    if img.kinds.get(g) != 'port':
        bad.append('%s not in .port_text' % g)
    else:
        if img.calls.get(g, set()) != {'viewGetSplitMode', 'gameGetMode', 'gameGetLevel'}:
            bad.append('%s calls %s' % (g, sorted(img.calls.get(g, set()))))
        if ('portRollbackOptions', 'load') not in img.word_refs({'portRollbackOptions'}).get(g, set()):
            bad.append('%s does not read portRollbackOptions' % g)
    for f in sorted(VIEW_GATED):
        n_gate, n_split = img.jal_counts(f, g), img.jal_counts(f, 'viewGetSplitMode')
        if humans and (n_gate or n_split):
            bad.append('%s: %d %s, %d viewGetSplitMode calls (TM2_PORT_HUMANS: the game mode rule, H1)' % (
                f, n_gate, g, n_split))
        if not humans and (n_gate < 1 or n_split):
            bad.append('%s: %d %s, %d viewGetSplitMode calls (want the gate only)' % (f, n_gate, g, n_split))
    callers = set(img.callers.get(g, ()))
    if callers - VIEW_GATED:
        bad.append('%s called by %s' % (g, sorted(callers - VIEW_GATED)))
    # (e) the settings' rows
    syms, _ = rr.link_symbols(out)
    n_rows = 0
    for name in VIEW_SETTINGS:
        a = syms.get(name)
        if a is None:
            bad.append('%s not in the link' % name)
            continue
        row = rr.covering(res, a)
        if row is None or row[2]['hash'] == 'yes':
            bad.append('%s is hashed (%s)' % (name, row[2]['name'] if row else 'no row'))
        else:
            n_rows += 1
    if len(users) < 30 or not nct:
        bad.append('only %d users, %d dependent nodes (the check is blind)' % (len(users), nct))
    good = not problems and not bad
    print('10. view settings: %d step functions read this machine\'s split / mirror / listener (%d call sites; %d more '
          'through dependent returns), %d dependent nodes, state writes or rand() in %d (gated sites %s); the view '
          'and sound side (%d roots, %d functions) stores to %d hashed words (%d lazy stores exempt), through '
          'pointers to %d (%d pointer stores); arenaSetup makes views 0 and 1; %d of %d settings out of the hash (%s) '
          '-> %s'
          % (len(users), sites, len(returns), nct, len(found), ', '.join(sorted(f for f in VIEW_GATED
                                                                                   if g in img.calls.get(f, set())))
             or 'none', len(pres), len(reach), len(hashed), lazy_hit, len(ph), pseen, n_rows, len(VIEW_SETTINGS),
             Path(regions_path).name, 'ok' if good else 'FAIL'))
    for b in (problems + bad)[:24]:
        print('     ' + b)
    return good


def local_gate(img):
    """Check 11 (b): LOCAL_GATE tests portHumanOptions bit 0 before it calls portHumanSlot: [problems]."""
    g = LOCAL_GATE
    if g not in img.sizes:
        return ['%s not in the link' % g]
    bad = []
    fl = img.flow(g)
    if img.calls.get(g, set()) != {'portHumanSlot'}:
        bad.append('%s calls %s' % (g, sorted(img.calls.get(g, set()))))
    loads = {p for p, t, v, it, sy in img.relocs if t in (6, 7) and fl.start <= p < fl.end
             and 'portHumanOptions' in (img.addr_names.get(v, set()) | {sy}) and img.link.word(p) >> 26 == 0x23}
    calls = {a for a, c in fl.calls.items() if c == 'portHumanSlot'}
    # the branch on (portHumanOptions & 1): beq / bne $r, $zero after andi $r, $x, 1 of the loaded word
    tests = []
    for a in sorted(fl.insn):
        w = fl.insn[a]
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op not in (4, 5) or rt != 0:
            continue
        src = None
        for b in range(a - 4, max(fl.start, a - 24) - 4, -4):
            x = fl.insn[b]
            if x >> 26 == 0x0C and (x >> 16) & 31 == rs and x & 0xFFFF == 1:
                src = (x >> 21) & 31
                for c in range(b - 4, max(fl.start, b - 24) - 4, -4):
                    if c in loads and (fl.insn[c] >> 16) & 31 == src:
                        tests.append(a)
                        break
                break
            if rs in mips_du(x)[0]:
                break
    if len(loads) != 1 or len(tests) != 1 or not calls:
        bad.append('%s: %d loads of portHumanOptions, %d bit-0 tests, %d portHumanSlot calls' % (
            g, len(loads), len(tests), len(calls)))
        return bad
    a = tests[0]
    w = fl.insn[a]
    target = a + 4 + (sext16(w & 0xFFFF) << 2)
    on, clear = (a + 8, target) if w >> 26 == 4 else (target, a + 8)     # beq: taken when the bit is clear
    if calls & fl.reach(fl.start, {a}):
        bad.append('%s: portHumanSlot reachable without the INPUT_ALL test' % g)
    if calls & fl.reach(on, set()):
        bad.append('%s: portHumanSlot reachable from the INPUT_ALL edge' % g)
    if not calls & fl.reach(clear, set()):
        bad.append('%s: portHumanSlot unreachable with INPUT_ALL clear (the gate is not the retail path)' % g)
    return bad


def check_locals(img, out, regions_path, bit_clear=False):
    """Check 11 (request 58): no step-reachable read of this machine's humans (portHumanLocal) reaches a state write
    outside the presentation. bit_clear: model INPUT_ALL clear (LOCAL_GATE a source): the input readers must be
    found."""
    import rollback_regions as rr
    rows, problems = rr.load_rows(regions_path)
    res, p2 = rr.resolve(out, rows)
    problems += p2
    bad = []
    step, todo = set(), ['portStepBody']
    while todo:
        f = todo.pop()
        if f not in step:
            step.add(f)
            todo.extend(img.calls.get(f, ()))
    sources = set(LOCAL_SOURCES) | ({LOCAL_GATE} if bit_clear else set())
    present = LOCAL_PRESENT | VIEW_PRESENT | SOUND_SIDE
    allowed = sources | present | VIEW_SOURCES | VIEW_GATES
    exempt = LOCAL_SOURCES | present | {LOCAL_GATE}
    users = set()
    for s in sources:
        users |= set(img.callers.get(s, ()))
    users |= set(img.word_refs(LOCAL_WORDS))
    users = {u for u in users if u in step and u not in exempt}
    sites = sum(img.jal_counts(u, s) for u in users for s in sources)
    tool = ViewTaint(img, res, sources, LOCAL_WORDS, allowed)
    found, returns, done, nct = {}, set(), set(), 0
    todo = sorted(users)
    while todo:
        fn = todo.pop()
        if fn in done or fn not in img.sizes:
            continue
        done.add(fn)
        got, ct = tool.run(fn)
        nct += len(ct)
        for a, what in got:
            if what == 'returns':
                if fn not in returns:
                    returns.add(fn)
                    tool.sources.add(fn)
                    todo.extend(c for c in img.callers.get(fn, ()) if c in step and c not in exempt)
            else:
                found.setdefault(fn, []).append((a, what))
    if bit_clear:
        want = {f for f in LOCAL_INPUT if f in step}
        good = bool(want) and set(found) == want
        print('11. negative model (INPUT_ALL clear: %s reads the map): state writes found in %s (want %s) -> %s'
              % (LOCAL_GATE, sorted(found), sorted(want), 'ok' if good else 'FAIL'))
        for fn in sorted(found):
            for a, what in found[fn][:3]:
                print('     %s %08X %s' % (fn, a, what))
        return good
    for fn in sorted(found):
        for a, what in found[fn][:6]:
            bad.append('%s %08X: %s' % (fn, a, what))
    # (a) the presentation by locality: no rand(), static and pointer stores in "hash no" rows only
    pres = sorted(LOCAL_PRESENT & step)
    reach, todo = set(), list(pres)
    while todo:
        f = todo.pop()
        if f not in reach:
            reach.add(f)
            todo.extend(c for c in img.calls.get(f, ()) if (f, c) not in VIEW_LAZY_CALLS)
    if 'rand' in reach or 'srand' in reach:
        bad.append('the local presentation reaches rand()')
    hashed = set()
    for p, t, v, it, sy in img.relocs:
        if t in (6, 7) and owner(img.rows, p) in reach and img.link.word(p) >> 26 in STORES:
            row = rr.covering(res, v)
            if row is not None and row[2]['hash'] == 'yes':
                hashed.add(sorted((img.addr_names.get(v, set()) | {sy}) - {None, ''})[0])
    ph, pseen, _ = ptr_hashed(img, out, res, sorted(reach))
    for name in sorted(hashed | set(ph)):
        bad.append('the local presentation stores to hashed %s' % name)
    # (b) the INPUT_ALL gate
    bad += local_gate(img)
    # (c) the per-machine words and the human records' camera block out of the hash
    syms, addrs = rr.link_symbols(out)
    n_rows = 0
    for name in LOCAL_ROWS:
        a = syms.get(name)
        row = rr.covering(res, a) if a is not None else None
        if row is None or row[2]['hash'] == 'yes':
            bad.append('%s is hashed (%s)' % (name, row[2]['name'] if row else 'no row'))
        else:
            n_rows += 1
    for name, (stride, offs) in sorted(LOCAL_FIELDS.items()):
        a = syms.get(name)
        if a is None:
            bad.append('%s not in the link' % name)
            continue
        i = bisect.bisect_right(addrs, a)
        n = (addrs[i] - a) // stride if i < len(addrs) else 0
        hit = sum(1 for e in range(n) for off in offs
                  if (lambda r: r is None or r[2]['hash'] == 'yes')(rr.covering(res, a + e * stride + off)))
        if hit or not n:
            bad.append('%s: %d camera words of %d records hashed' % (name, hit, n))
        else:
            n_rows += 1
    if len(users) < 5 or not nct:
        bad.append('only %d users, %d dependent nodes (the check is blind)' % (len(users), nct))
    good = not problems and not bad
    print('11. this machine\'s humans: %d step functions read the local map (%d call sites; %d more through dependent '
          'returns), %d dependent nodes, state writes in %d; the local presentation (%d roots, %d functions) stores '
          'to %d hashed words (%d pointer stores); %s gated by INPUT_ALL; %d of %d per-machine rows out of the hash '
          '(%s) -> %s'
          % (len(users), sites, len(returns), nct, len(found), len(pres), len(reach), len(hashed | set(ph)), pseen,
             LOCAL_GATE, n_rows, len(LOCAL_ROWS) + len(LOCAL_FIELDS), Path(regions_path).name,
             'ok' if good else 'FAIL'))
    for b in (problems + bad)[:24]:
        print('     ' + b)
    return good


# ---- check 12 (request 59, ONLINE_ROLLBACK.md 20) ----
# (a) the words a re-run makes another stack address in: every element's field lies in a "hash no" row
STACK_FIELDS = (('weaponArcs', 20, 0x10, 0x14, 'weaponArcs.to'),       # request 59 (1): weaponArcsDraw's free_end
                ('fxSprites', 48, 0x2C, 0x30, 'fxSprites.owner'))      # request 56: a spawn's stack vector
CLUT_SHADOW_WORDS = (8 + 524 * 32) // 4     # sizeof(struct TM2PortClutShadow) / 4 (port_rollback.h, 0x4188 bytes)


def check_request59(img, out, regions_path):
    """Check 12: the arcs' end pointer out of the hash, the palette shadow emptied at each round's capture."""
    import rollback_regions as rr
    rows, problems = rr.load_rows(regions_path)
    res, p2 = rr.resolve(out, rows)
    problems += p2
    bad = []
    syms, _ = rr.link_symbols(out)
    # (a) every element's stack-pointer field in a "hash no" row (the row's elements: the field row's resolution)
    counts = {}
    for sym, stride, lo, hi, row in STACK_FIELDS:
        a = syms.get(sym)
        n = sum(1 for x, sz, r in res if r['kind'] == 'field' and r['name'].startswith(row + '['))
        counts[row] = n
        if a is None or not n:
            bad.append('%s: no "%s" field row resolved' % (sym, row))
            continue
        hit = 0
        for e in range(n):
            for off in range(lo, hi, 4):
                r = rr.covering(res, a + e * stride + off)
                if r is None or r[2]['hash'] == 'yes' or r[2]['snapshot'] != 'yes':
                    hit += 1
        if hit:
            bad.append('%s: %d words of %d elements hashed or not snapshotted (+0x%X)' % (sym, hit, n, lo))
    # (b) the evidence: weaponArcsDraw stores an address formed from $sp (free_end) into weaponArcs[i] +0x10 (a store
    # relocated to weaponArcs + 0x10, its value register set by an addiu from $sp)
    fn = 'weaponArcsDraw'
    sp_stores = 0
    arcs = syms.get('weaponArcs')
    if fn not in img.sizes or arcs is None:
        bad.append('%s / weaponArcs not in the link' % fn)
    else:
        fl = img.flow(fn)
        to_lo = {p for p, t, v, it, sym in img.relocs if t in (6, 7) and fl.start <= p < fl.end and v == arcs + 0x10}
        sp_regs = set()
        for a in sorted(fl.insn):
            w = fl.insn[a]
            op, rs, rt, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
            if op == 0x2B and rt in sp_regs and a in to_lo:
                sp_stores += 1
            if op == 9 and rs == 29 and rt not in (0, 29):
                sp_regs.add(rt)
            elif op == 0 and (w >> 11) & 31 in sp_regs and (w & 63) not in (8, 9):
                sp_regs.discard((w >> 11) & 31)
            elif op in (8, 9, 10, 11, 12, 13, 14, 15) or op in LOADS:
                sp_regs.discard(rt)
        if not sp_stores:
            bad.append('%s: no store of a $sp-formed address at +0x10 (the free end; the evidence moved?)' % fn)
    # (c) the palette shadow: portClutShadowCapture empties it (portClutShadowReset dominates its VRAM reads), and
    # the reset clears every word of portClutShadow (a loop bound of the struct's words) and calls nothing
    fn, rs_fn = 'portClutShadowCapture', 'portClutShadowReset'
    if rs_fn not in img.sizes or img.kinds.get(rs_fn) != 'port':
        bad.append('%s not in .port_text' % rs_fn)
    else:
        if img.calls.get(rs_fn):
            bad.append('%s calls %s' % (rs_fn, sorted(img.calls[rs_fn])))
        if 'portClutShadow' not in {n for n, k in img.word_refs({'portClutShadow'}).get(rs_fn, set())}:
            bad.append('%s does not address portClutShadow' % rs_fn)
        fl = img.flow(rs_fn)
        if not any(w >> 26 == 0x2B for w in fl.insn.values()):
            bad.append('%s stores nothing' % rs_fn)
        # the count: a bound of the words (slti / sltiu) or a down-counter loaded with words - 1 (ori / addiu $zero)
        if not any((w >> 26 in (0x0A, 0x0B) and w & 0xFFFF == CLUT_SHADOW_WORDS)
                   or (w >> 26 in (0x09, 0x0D) and (w >> 21) & 31 == 0 and w & 0xFFFF in (CLUT_SHADOW_WORDS,
                                                                                         CLUT_SHADOW_WORDS - 1))
                   for w in fl.insn.values()):
            bad.append('%s: no loop bound of %d words (the whole shadow)' % (rs_fn, CLUT_SHADOW_WORDS))
    if fn not in img.sizes:
        bad.append('%s not in the link' % fn)
    else:
        sites = call_sites(img, fn, {'gpuStoreImageLine', 'portClutRow'})
        g, lines = gate(img, fn, rs_fn, sites, '12. shadow emptied first', dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    good = not problems and not bad
    print('12. request 59: %s in "hash no" rows (%s elements); the $sp stores of weaponArcsDraw at +0x10: %d; '
          'portClutShadowCapture empties the shadow first (%s, %d words) -> %s'
          % (', '.join(r for *_, r in STACK_FIELDS), ', '.join('%d' % counts.get(r, 0) for *_, r in STACK_FIELDS),
             sp_stores, rs_fn, CLUT_SHADOW_WORDS, 'ok' if good else 'FAIL'))
    for b in (problems + bad)[:16]:
        print('     ' + b)
    return good


# ---- check 13 (request 60, ONLINE_ROLLBACK.md 21) ----
# the round's start: the resets the round runs before its first step (call edges), and the pools each one addresses
# (21.2's table: the field each pool's update tests is what its reset clears)
ROUND_RESETS = (('roundStart', 'weaponsRoundReset'), ('roundStart', 'debrisClear'), ('roundStart', 'humanCarsRestart'),
                ('humanCarsRestart', 'carRestart'), ('carRestart', 'weaponDropsReset'),
                ('carRestart', 'weaponsRoundReset'), ('carRestart', 'projectilesReset'),
                ('carRestart', 'fxSpritesReset'), ('projectilesReset', 'bulletsClear'),
                ('fxSpritesReset', 'fxImpactParticlesClear'), ('roundStart', 'portRoundCleanProjectiles'))
ROUND_POOLS = (('projectilesReset', 'fxRecords'), ('portRoundCleanProjectiles', 'fxRecords'),
               ('bulletsClear', 'bulletsLifeField'), ('fxSpritesReset', 'fxSprites'),
               ('fxImpactParticlesClear', 'fxImpactParticlesLifeField'), ('debrisClear', 'debrisChunks'),
               ('debrisClear', 'debrisBursts'), ('weaponDropsReset', 'weaponDrops'),
               ('weaponDropsReset', 'weaponMineObjects'), ('weaponsRoundReset', 'groundFires'),
               ('weaponsRoundReset', 'weaponArcs'), ('weaponsRoundReset', 'weaponFlameEmitters'),
               ('weaponsRoundReset', 'weaponRicochetPool'), ('weaponsRoundReset', 'weaponShieldPool'))
PROJ_SIZE = 980
PROJ_CLEARED = [(0x02, 0x34), (0x38, 0x48)]       # portRoundCleanProjectiles: the fields a fire sets
PROJ_KEPT = [(0x00, 0x02), (0x34, 0x38), (0x48, PROJ_SIZE)]     # the id, the object, the trail (the load's)
RB_ROUND_CLEAN = 0x800
PROJ_OBJ = 0x80F00000       # the emulation's fake objects (one per record, 0x200 apart)


class MiniMips:
    """A small MIPS I interpreter for the request-60 port functions (no exceptions, no coprocessors): memory is a byte
    dict (0 where never written); a jal goes to a Python stub by name, any other call or jump out is an error."""
    RET = 0xDEAD0000

    def __init__(self, img, stubs):
        self.img, self.stubs = img, stubs
        self.mem, self.writes = {}, set()
        self.r = [0] * 32
        self.hi = self.lo = 0
        self.calls = []

    def rd(self, a, n):
        return sum(self.mem.get(a + k, 0) << (8 * k) for k in range(n))

    def wr(self, a, n, v):
        for k in range(n):
            self.mem[a + k] = (v >> (8 * k)) & 0xFF
            self.writes.add(a + k)

    def fill(self, a, data):
        for k, b in enumerate(data):
            self.mem[a + k] = b

    def set(self, i, v):
        if i:
            self.r[i] = v & 0xFFFFFFFF

    def run(self, fn, args=(), steps=400000):
        r = self.r
        r[29], r[31] = 0x801FF000, self.RET
        for i, v in enumerate(args):
            r[4 + i] = v & 0xFFFFFFFF
        start, size = self.img.sizes[fn]
        pc, delay = start, None
        for _ in range(steps):
            if pc == self.RET:
                return r[2]
            nxt = self.exec(pc, self.img.link.word(pc))
            if delay is not None:
                pc, delay = delay, None
                name = self.img.names_at.get(pc)
                if pc != start and name in self.stubs:
                    self.calls.append(name)
                    self.set(2, self.stubs[name](*[r[4 + k] for k in range(4)]))
                    pc = r[31]
                elif pc != self.RET and not start <= pc < start + size:
                    raise RuntimeError('%s: jump to %08X (%s)' % (fn, pc, name))
                continue
            if nxt is None:
                pc += 4
            else:
                delay, pc = nxt, pc + 4
        raise RuntimeError('%s: no return in %d steps' % (fn, steps))

    def exec(self, pc, w):
        """One instruction; returns the jump / branch target (taken or not: pc + 8) or None."""
        r = self.r
        op, rs, rt, rd, sa, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
        imm, simm = w & 0xFFFF, sext16(w & 0xFFFF)

        def s32(v):
            return v - (1 << 32) if v & 0x80000000 else v

        def br(c):
            return (pc + 4 + (simm << 2)) & 0xFFFFFFFF if c else pc + 8
        if op == 0:
            if fn == 0:
                self.set(rd, r[rt] << sa)
            elif fn == 2:
                self.set(rd, r[rt] >> sa)
            elif fn == 3:
                self.set(rd, s32(r[rt]) >> sa)
            elif fn == 4:
                self.set(rd, r[rt] << (r[rs] & 31))
            elif fn == 6:
                self.set(rd, r[rt] >> (r[rs] & 31))
            elif fn == 7:
                self.set(rd, s32(r[rt]) >> (r[rs] & 31))
            elif fn == 8:
                return r[rs]
            elif fn == 9:
                t = r[rs]
                self.set(rd, pc + 8)
                return t
            elif fn == 16:
                self.set(rd, self.hi)
            elif fn == 18:
                self.set(rd, self.lo)
            elif fn in (24, 25):
                v = s32(r[rs]) * s32(r[rt]) if fn == 24 else r[rs] * r[rt]
                self.lo, self.hi = v & 0xFFFFFFFF, (v >> 32) & 0xFFFFFFFF
            elif fn in (32, 33):
                self.set(rd, r[rs] + r[rt])
            elif fn in (34, 35):
                self.set(rd, r[rs] - r[rt])
            elif fn == 36:
                self.set(rd, r[rs] & r[rt])
            elif fn == 37:
                self.set(rd, r[rs] | r[rt])
            elif fn == 38:
                self.set(rd, r[rs] ^ r[rt])
            elif fn == 39:
                self.set(rd, ~(r[rs] | r[rt]))
            elif fn == 42:
                self.set(rd, int(s32(r[rs]) < s32(r[rt])))
            elif fn == 43:
                self.set(rd, int(r[rs] < r[rt]))
            else:
                raise RuntimeError('%08X: SPECIAL funct %d' % (pc, fn))
            return None
        if op == 1:
            if rt in (0, 1):
                return br((s32(r[rs]) < 0) == (rt == 0))
            raise RuntimeError('%08X: REGIMM %d' % (pc, rt))
        if op in (2, 3):
            if op == 3:
                self.set(31, pc + 8)
            return ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
        if op == 4:
            return br(r[rs] == r[rt])
        if op == 5:
            return br(r[rs] != r[rt])
        if op == 6:
            return br(s32(r[rs]) <= 0)
        if op == 7:
            return br(s32(r[rs]) > 0)
        if op in (8, 9):
            self.set(rt, r[rs] + simm)
        elif op == 10:
            self.set(rt, int(s32(r[rs]) < simm))
        elif op == 11:
            self.set(rt, int(r[rs] < (simm & 0xFFFFFFFF)))
        elif op == 12:
            self.set(rt, r[rs] & imm)
        elif op == 13:
            self.set(rt, r[rs] | imm)
        elif op == 14:
            self.set(rt, r[rs] ^ imm)
        elif op == 15:
            self.set(rt, imm << 16)
        elif op in (32, 33, 35, 36, 37):
            n = {32: 1, 36: 1, 33: 2, 37: 2, 35: 4}[op]
            v = self.rd((r[rs] + simm) & 0xFFFFFFFF, n)
            if op == 32 and v & 0x80:
                v -= 0x100
            if op == 33 and v & 0x8000:
                v -= 0x10000
            self.set(rt, v)
        elif op in (40, 41, 43):
            self.wr((r[rs] + simm) & 0xFFFFFFFF, {40: 1, 41: 2, 43: 4}[op], r[rt])
        else:
            raise RuntimeError('%08X: opcode %d' % (pc, op))
        return None


def proj_fill(i, k):
    return (0xA5 + 3 * i + 7 * k) & 0xFF or 0x11


def run_clean(img, syms, options, pools_live, records):
    """portRoundCleanProjectiles on `records` records of a known pattern (each +0x34 a fake object of 0x5A bytes)."""
    m = MiniMips(img, {})
    m.wr(syms['portRollbackOptions'], 4, options)
    if 'portCarPoolsLive' in syms:
        m.wr(syms['portCarPoolsLive'], 4, pools_live)
    base = syms['fxRecords']
    for i in range(records):
        m.fill(base + i * PROJ_SIZE, bytes(proj_fill(i, k) for k in range(PROJ_SIZE)))
        obj = PROJ_OBJ + i * 0x200
        m.fill(base + i * PROJ_SIZE + 0x34, obj.to_bytes(4, 'little'))
        m.fill(obj, b'\x5A' * 0x128)
    m.writes.clear()
    m.run('portRoundCleanProjectiles')
    return m


def clean_problems(m, base, records, live):
    """How the memory differs from a full clean of records 0..live-1 that touched nothing else."""
    bad = []
    for i in range(records):
        rec, obj = base + i * PROJ_SIZE, PROJ_OBJ + i * 0x200
        cleaned = i < live
        for lo, hi in PROJ_CLEARED:
            for k in range(lo, hi):
                want = (0xF1 if k == 4 else 0xFF if k == 5 else 0) if cleaned else proj_fill(i, k)
                if m.mem.get(rec + k, 0) != want:
                    bad.append('record %d +0x%02X = %02X (want %02X)' % (i, k, m.mem.get(rec + k, 0), want))
                    break
        for lo, hi in PROJ_KEPT:
            if any(rec + k in m.writes for k in range(lo, hi)):
                bad.append('record %d: +0x%02X..+0x%02X written (the load\'s)' % (i, lo, hi))
        hidden = all(m.mem.get(obj + 8 + k, 0x5A) == 0 for k in range(4))
        if hidden != cleaned:
            bad.append('record %d: object +8 %s' % (i, 'still set' if cleaned else 'cleared'))
        if any(obj + k in m.writes for k in range(0x128) if not 8 <= k < 12):
            bad.append('record %d: its object written past +8' % i)
    span = set(range(base, base + records * PROJ_SIZE)) | {PROJ_OBJ + i * 0x200 + 8 + k for i in range(records)
                                                           for k in range(4)}
    stray = sorted(m.writes - span)
    if stray:
        bad.append('%d stray bytes written (first %08X)' % (len(stray), stray[0]))
    return bad


def check_request60(img, out, proj_n):
    """Check 13: the round's start clears the projectile records with ROUND_CLEAN (emulated), a seeker steers only at a
    counted car object, the round's resets reach every pool."""
    import rollback_regions as rr
    syms, _ = rr.link_symbols(out)
    bad, notes = [], []
    for fn in ('portRoundCleanProjectiles', 'portSeekerTargetOk'):
        if fn not in img.sizes or img.kinds.get(fn) != 'port':
            bad.append('%s not in .port_text' % fn)
    if 'fxRecords' not in syms or 'portRollbackOptions' not in syms:
        bad.append('fxRecords / portRollbackOptions not in the link')
    if not bad:
        if img.calls.get('portRoundCleanProjectiles'):
            bad.append('portRoundCleanProjectiles calls %s' % sorted(img.calls['portRoundCleanProjectiles']))
        extra = img.calls.get('portSeekerTargetOk', set()) - {'humanCarCount', 'humanCarObject', 'aiCarCount',
                                                              'aiCarObject'}
        if extra:
            bad.append('portSeekerTargetOk calls %s' % sorted(extra))
        # (a) the clean, emulated: with the bit every live record's fire fields cleared and its object hidden, nothing
        # else written; without it (every other bit set) nothing written, and the comparison then fails (not blind)
        base = syms['fxRecords']
        live_max = proj_n     # the link's fxRecords entries (report.json: 16, 48 with TM2_PORT_CARS=16)
        records = live_max + 4
        for pools in ((0, 1) if 'portCarPoolsLive' in syms else (0,)):
            live = live_max if pools else 16
            try:
                m = run_clean(img, syms, RB_ROUND_CLEAN, pools, records)
                p = clean_problems(m, base, records, live)
                bad += ['clean (pools live %d): %s' % (pools, x) for x in p[:4]]
                m0 = run_clean(img, syms, 0xFFFFFFFF ^ RB_ROUND_CLEAN, pools, records)
                if m0.writes:
                    bad.append('clean without the bit wrote %d bytes' % len(m0.writes))
                if not clean_problems(m0, base, records, live):
                    bad.append('the comparison passed a clean that did nothing (blind)')
                notes.append('%d records' % live)
            except RuntimeError as e:
                bad.append('portRoundCleanProjectiles: %s' % e)
        # (b) the target test, emulated with stubbed car tables: 3 humans (one without an object), 2 AI cars counted
        h_objs, a_objs = [0x80E00000, 0, 0x80E00400], [0x80E10000, 0x80E10400, 0x80E10800]
        stubs = {'humanCarCount': lambda *a: len(h_objs), 'aiCarCount': lambda *a: 2,
                 'humanCarObject': lambda i, *a: h_objs[i] if i < len(h_objs) else 0x80EFFF00,
                 'aiCarObject': lambda i, *a: a_objs[i] if i < len(a_objs) else 0x80EFFF00}
        cases = [(h_objs[0] + 0xA8, 1), (h_objs[2] + 0xA8, 1), (a_objs[1] + 0xA8, 1), (0xA8, 0), (h_objs[0], 0),
                 (h_objs[0] + 0xAC, 0), (0x80E20000 + 0xA8, 0), (a_objs[2] + 0xA8, 0)]
        try:
            for opts, fixed in ((RB_ROUND_CLEAN, None), (0xFFFFFFFF ^ RB_ROUND_CLEAN, 1)):
                for t, want in cases:
                    m = MiniMips(img, stubs)
                    m.wr(syms['portRollbackOptions'], 4, opts)
                    got = m.run('portSeekerTargetOk', (t,))
                    w = want if fixed is None else fixed
                    if got != w:
                        bad.append('portSeekerTargetOk(%08X) = %d with options %X (want %d)' % (t, got, opts, w))
                    if fixed is not None and m.calls:
                        bad.append('portSeekerTargetOk without the bit calls %s' % m.calls)
            notes.append('%d targets' % len(cases))
        except RuntimeError as e:
            bad.append('portSeekerTargetOk: %s' % e)
    # (c) the call sites: the clean dominates roundStart's restarts (the round's projectilesReset), the target test
    # dominates weaponProjectileUpdate's steering, and the function drops the target (a store of $zero at +0x38)
    for fn, gate_fn, callees in (('roundStart', 'portRoundCleanProjectiles', {'humanCarsRestart'}),
                                 ('weaponProjectileUpdate', 'portSeekerTargetOk', {'weaponProjectileSteer'})):
        if fn not in img.sizes:
            bad.append('%s not in the link' % fn)
            continue
        if img.jal_counts(fn, gate_fn) != 1:
            bad.append('%s calls %s %d times' % (fn, gate_fn, img.jal_counts(fn, gate_fn)))
            continue
        g, lines = gate(img, fn, gate_fn, call_sites(img, fn, callees), '13. %s' % fn, dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    if 'weaponProjectileUpdate' in img.sizes:
        fl = img.flow('weaponProjectileUpdate')
        if not any(w >> 26 == 0x2B and (w >> 16) & 31 == 0 and w & 0xFFFF == 0x38 for w in fl.insn.values()):
            bad.append('weaponProjectileUpdate: no store of 0 at +0x38 (the dropped target)')
    # (d) the round's resets reach every pool of 21.2's table (the call edges, each reset's references)
    for a, b in ROUND_RESETS:
        if b not in img.calls.get(a, set()):
            bad.append('%s does not call %s' % (a, b))
    for fn, sym in ROUND_POOLS:
        if sym not in syms:
            bad.append('%s not in the link' % sym)
        elif sym not in {n for n, k in img.word_refs({sym}).get(fn, set())}:
            bad.append('%s does not address %s' % (fn, sym))
    good = not bad
    print('13. request 60: ROUND_CLEAN emulated (%s): portRoundCleanProjectiles clears the fire fields with the bit, '
          'writes nothing without it; portSeekerTargetOk steers only at a counted car object; both dominate their '
          'sites; the round\'s resets address %d pools -> %s' % (', '.join(notes) or '-',
                                                                  len({s for _, s in ROUND_POOLS}),
                                                                  'ok' if good else 'FAIL'))
    for b in bad[:16]:
        print('     ' + b)
    return good


def step_scrub_check(img):
    """Request 86 (ONLINE_ROLLBACK.md 23): portStepBody calls portStepStackScrub before anything else; it is a leaf
    with no frame of its own and no rand(); run in check_custom's LinkMips (sp 0x801FF000): with portStepScrub's
    argument 64 and portStepScrubFill 0xA5A5A5A5 the 16 words below sp hold the fill and nothing else changed but
    a0's home slot; with 0x9000 at most TM2_RB_SCRUB_MAX (0x8000) bytes are written."""
    from check_hazards import sext16
    from check_custom import LinkMips
    bad = []
    fn = 'portStepStackScrub'
    if fn not in img.sizes or 'portStepScrubFill' not in img.link.symbols:
        return ['request 86: %s or portStepScrubFill not in the link' % fn]
    if img.kinds.get(fn) != 'port' or img.calls.get(fn):
        bad.append('request 86: %s not a .port_text leaf (calls %s)' % (fn, sorted(img.calls.get(fn, ()))))
    start, size = img.sizes[fn]
    for a in range(start, start + size, 4):
        w = img.link.word(a)
        if w >> 26 == 9 and (w >> 21) & 31 == 29 and (w >> 16) & 31 == 29:
            bad.append('request 86: %s moves sp at %08X (a frame below the caller: it would fill its own)' % (fn, a))
    if 'portStepBody' in img.sizes:
        fl = img.flow('portStepBody')
        calls = sorted(fl.calls.items())
        first_scrub = [a for a, c in calls if c == fn]
        others = [a for a, c in calls if c != fn]
        if len(first_scrub) != 1 or (others and others[0] < first_scrub[0]):
            bad.append('request 86: portStepBody calls %s %d times, not first' % (fn, len(first_scrub)))
    fill = img.link.symbols['portStepScrubFill']['address']
    for n, want in ((64, 64), (0x9000, 0x8000)):
        lm = LinkMips(img, {})
        m = lm.m
        m.wr(fill, 4, 0xA5A5A5A5)
        sp = 0x801FF000
        m.writes.clear() if hasattr(m, 'writes') else None
        before = {a: m.rd(a, 4) for a in range(sp - want - 64, sp + 64, 4)}
        lm.run(fn, (n,))
        changed = sorted(a for a in before if m.rd(a, 4) != before[a])
        wrote = [a for a in changed if a < sp]
        if wrote != list(range(sp - want, sp, 4)) or any(m.rd(a, 4) != 0xA5A5A5A5 for a in wrote) or                 [a for a in changed if a >= sp + 4]:
            bad.append('request 86: %s(%d) wrote %d words below sp (want %d of the fill), others %s' % (
                fn, n, len(wrote), want // 4, ['%08X' % a for a in changed if a >= sp + 4][:3]))
    return bad


STACK_BYTES, STACK_GUARD, CAMERA_WORK = 0x10000, 16, 0x1000      # port_rollback.h TM2_PORT_STACK, _GUARD, _CAMERA_WORK


def frame_bytes(img, fn):
    """the bytes fn's prologue takes off sp (addiu sp, sp, -N in its first 32 instructions), 0 for none"""
    from check_hazards import sext16
    start, size = img.sizes[fn]
    for a in range(start, min(start + 128, start + size), 4):
        w = img.link.word(a)
        if w >> 26 == 9 and (w >> 21) & 31 == 29 and (w >> 16) & 31 == 29 and sext16(w & 0xFFFF) < 0:
            return -sext16(w & 0xFFFF)
    return 0


def stack_depth(img, root):
    """(bytes, chain): the deepest chain of direct calls (jal) from root, the sum of their frames (a lower bound: calls
    through pointers, the interrupts' and the port's callbacks are not followed; a cycle is cut where it closes)"""
    memo, on = {}, set()

    def depth(f):
        if f in memo:
            return memo[f]
        if f in on or f not in img.sizes:
            return 0, []
        on.add(f)
        best = (0, [])
        for c in sorted(img.calls.get(f, ())):
            d = depth(c)
            if d[0] > best[0]:
                best = d
        on.discard(f)
        memo[f] = (frame_bytes(img, f) + best[0], [f] + best[1])
        return memo[f]
    return depth(root)


def stack_check(img):
    """Request 89 (ONLINE_ROLLBACK.md 25): the port variant's stack. portStack (64 KB, .port_bss) with portStackGuard
    (16 bytes) right below it; the EXE's crt0 stack word (2MBYTE's table, __SN_ENTRY_POINT + 0xB0) holds its end
    without 0x80000000 and the header's s_addr its end; portCameraWallWork is 0x1000 bytes (0xE00 before request 96) and portVisualCameraWall's
    frame at most 0x200 (it was 0xF00); main's deepest chain of direct calls fits in half the stack."""
    import struct as st
    syms = img.link.symbols
    need = ('portStack', 'portStackGuard', 'portCameraWallWork', '__SN_ENTRY_POINT')
    miss = [n for n in need if n not in syms]
    if miss:
        return ['request 89: %s not in the link' % miss]
    bad = []
    a = {n: syms[n]['address'] for n in need}
    size = {n: syms[n].get('size') for n in need}
    top = a['portStack'] + STACK_BYTES
    if size['portStack'] not in (None, 0, STACK_BYTES) or top & 7:     # (ld gives a .port_bss array no size)
        bad.append('request 89: portStack is 0x%X bytes at %08X (want 0x%X, end 8-aligned)' % (
            size['portStack'] or 0, a['portStack'], STACK_BYTES))
    if not 0 <= a['portStack'] - (a['portStackGuard'] + STACK_GUARD) <= 4:
        bad.append('request 89: portStackGuard %08X is not right below portStack %08X' % (a['portStackGuard'],
                                                                                          a['portStack']))
    if size['portCameraWallWork'] not in (None, 0, CAMERA_WORK):
        bad.append('request 89: portCameraWallWork is 0x%X bytes (want 0x%X)' % (size['portCameraWallWork'], CAMERA_WORK))
    raw = img.link.exe['raw']
    word = img.link.word(a['__SN_ENTRY_POINT'] + 0xB0)
    s_addr, s_size = st.unpack_from('<2I', raw, 0x30)
    if word != top & 0x7FFFFFFF or (s_addr, s_size) != (top, 0):
        bad.append('request 89: crt0 stack word %08X, s_addr %08X + %X (want %08X, %08X + 0)' % (
            word, s_addr, s_size, top & 0x7FFFFFFF, top))
    if 'portVisualCameraWall' in img.sizes:
        f = frame_bytes(img, 'portVisualCameraWall')
        if f > 0x200:
            bad.append('request 89: portVisualCameraWall frame 0x%X (want <= 0x200: its work in portCameraWallWork)' % f)
        refs = img.word_refs({'portCameraWallWork'}).get('portVisualCameraWall', set())
        if not refs:
            bad.append('request 89: portVisualCameraWall does not use portCameraWallWork')
    root = next((n for n in ('main', 'sub_8012C8DC') if n in img.sizes), None)   # (functions.tsv: retail's name)
    if root:
        d, chain = stack_depth(img, root)
        print('   request 89: stack %08X..%08X; main\'s deepest direct chain 0x%X bytes (%s)' % (
            a['portStack'], top, d, ' > '.join(chain[:4] + (['...'] if len(chain) > 6 else []) + chain[-2:])))
        if d > STACK_BYTES // 2:
            bad.append('request 89: main\'s deepest chain is 0x%X bytes (more than half of portStack)' % d)
    return bad


def voice_owner_check(img):
    """Request 87 (SPU_VOICES.md 11): with TM2_PORT_SPU48, where the voice tables and the request tags are written, and
    portVoiceOwner run in check_custom's LinkMips on made-up requests."""
    from check_custom import LinkMips
    fn = 'portVoiceOwner'
    syms = img.link.symbols
    if fn not in img.sizes:
        return []
    need = ('portVoiceCar', 'portVoiceKind', 'portSndReqTag', 'portSoundTag')
    miss = [n for n in need if n not in syms]
    if miss:
        return ['request 87: %s not in the link' % miss]
    bad = []
    if 'soundOn' in img.sizes:
        calls = sorted(img.flow('soundOn').calls.items())
        own = [a for a, c in calls if c == fn]
        key = [a for a, c in calls if c == 'sub_8015C7D0']
        if len(own) != 1 or len(key) != 1 or own[0] > key[0]:
            bad.append('request 87: soundOn calls %s %d times, SsUtKeyOnV %d times (want once, before)' % (
                fn, len(own), len(key)))
    refs = img.word_refs(set(need))
    for f, want in (('sub_80158400', {('portVoiceCar', 'other'), ('portVoiceKind', 'other')}),
                    ('sndPoolAlloc', {('portSoundTag', 'load'), ('portSndReqTag', 'other')}),
                    ('weaponFireSpecial', {('portSoundTag', 'store')}),
                    ('weaponSlamGrabUpdate', {('portSoundTag', 'store')}),
                    ('weaponSlamSwingUpdate', {('portSoundTag', 'store')})):
        got = refs.get(f, set())
        lost = [w for w in want if w not in got and not (w[1] == 'other' and (w[0], 'store') in got)]
        if f in img.sizes and lost:
            bad.append('request 87: %s has no %s' % (f, lost))
    A = {n: syms[n]['address'] for n in need}
    q = 0x801E0000
    cases = (   # (voice, request?, sound, instance, index, tag) -> (car, kind) or None (no write)
        (30, 1, 0x3D, 0, 5, 2 << 8 | 7, (7, 2)),        # a tagged special's sound
        (30, 1, 8, 0, 5, 2 << 8 | 7, (0, 0)),           # a tagged explosion: none
        (31, 1, 0x28, 4, 6, 0, (4, 1)),                 # an engine by its instance
        (31, 1, 0x28, 0, 6, 0, (0, 1)),                 # an engine with one request per type
        (40, 1, 11, 3, 7, 0, (0, 0)),                   # a pickup's launch (its instance is no car's)
        (40, 1, 11, 3, 7, 2 << 8 | 3, (3, 2)),          # the same fired as a special
        (47, 1, 47, 9, 8, 0, (9, 2)),                   # the flame by its instance
        (2, 0, 0, 0, 0, 0, (0, 0)),                     # no request
        (48, 1, 0x3D, 0, 5, 2 << 8 | 7, None))          # no voice 48
    for voice, has, snd, inst, index, tag, want in cases:
        lm = LinkMips(img, {})
        m = lm.m
        for v in range(48):
            m.wr(A['portVoiceCar'] + v, 1, 0xEE)
            m.wr(A['portVoiceKind'] + v, 1, 0xEE)
        for o in range(0, 0x78, 4):
            m.wr(q + o, 4, 0)
        m.wr(q + 0x18, 4, snd)
        m.wr(q + 0x2C, 4, inst)
        m.wr(q + 0x70, 4, index)
        m.wr(A['portSndReqTag'] + 2 * index, 2, tag)
        lm.run(fn, (voice, q if has else 0))
        got = [(m.rd(A['portVoiceCar'] + v, 1), m.rd(A['portVoiceKind'] + v, 1)) for v in range(48)]
        changed = [v for v in range(48) if got[v] != (0xEE, 0xEE)]
        if want is None:
            ok = not changed
        else:
            ok = changed == [voice] and got[voice] == want
        if not ok:
            bad.append('request 87: %s(%d) for sound %#x instance %d tag %#x: %s (want %s)' % (
                fn, voice, snd, inst, tag, [(v, got[v]) for v in changed], want))
    return bad


DMD_LIST_LEVELS = ('PARIS', 'HKONG', 'DISH')   # request 106: arenas with group (type 1) and offset (type 4) pickups
DMD_LIST_AT = 0x80019C40                        # the level file's address (its pointers are built for it)
DMD_LIST_OUT = 0x80A00000                       # the run's output buffer
DMD_LIST_MAX = 8192                             # pairs


def dmd_list_model(m, roots, hidden):
    """portDmdMutableList's walk in Python: (address, bytes) ranges sorted and merged, the pickups' holders (id
    401..430, types 1 and 4) at least one child (request 106). `hidden` lists every holder found."""
    out = []

    def walk(node, depth):
        if node == 0 or depth > 64:
            return
        t = m.rd(node, 1)
        pick = (m.rd(node + 2, 2) - 401) & 0xFFFF < 30
        if t == 1:
            out.append((node + 0x14, 4))
            n, kids = m.rd(node + 0x14, 1), node + 0x18
            if pick:
                hidden.add((node, 0x14))
                n = n or 1
        elif t == 2:
            n = m.rd(node + 0x10, 4)
            for i in range(min(n, 64)):
                r = m.rd(node + 0x14 + 4 * i, 4)
                if r:
                    walk(m.rd(r + 8, 4), depth + 1)
            return
        elif t == 3:
            out.append((node + 6, 0xE))
            n, kids = m.rd(node + 0x0B, 1), node + 0x20
        elif t == 4:
            out.append((node + 4, 0x10))
            n, kids = m.rd(node + 0x12, 1), node + 0x14
            if pick:
                hidden.add((node, 0x12))
                n = n or 1
        elif t == 5:
            out.append((node + 4, 0x24))
            n, kids = m.rd(node + 0x26, 1), node + 0x28
        elif t == 8:
            out.append((node + 4, 8))
            return
        elif t == 9:
            out.append((node + 6, 6))
            n, kids = m.rd(node + 6, 1), node + 0x10
        elif t == 11:
            out.append((node + 4, 4))
            walk(m.rd(node + 8, 4), depth + 1)
            walk(m.rd(node + 12, 4), depth + 1)
            return
        elif t == 12:
            n, kids = m.rd(node + 6, 2), node + 8
        else:
            return
        for i in range(n):
            walk(m.rd(kids + 4 * i, 4), depth + 1)
    for r in roots:
        walk(r, 0)
    merged = []
    for a, s in sorted(out):
        if merged and a <= merged[-1][0] + merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], a + s - merged[-1][0])
        else:
            merged.append([a, s])
    return [tuple(x) for x in merged]


def dmd_list_run(img):
    """Request 106: portDmdMutableList in check_custom's LinkMips on DMD_LIST_LEVELS from the disc (no car files):
    the list is the Python model's exactly, and the same with every pickup's holder hidden (its count 0, as
    pickupSetVisible leaves a taken pickup)."""
    import sys as _sys
    _sys.setrecursionlimit(10000)
    from check_custom import LinkMips
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    if 'portDmdMutableList' not in img.sizes or 'portDmdImages' not in img.link.symbols:
        return ['request 106 run: portDmdMutableList / portDmdImages not in the link']
    disc = Disc(DEFAULT_BIN)
    bad, notes = [], []
    for name in DMD_LIST_LEVELS:
        dmd = Dmd(name, disc.get('/DMD/%s.DMD' % name))
        lm = LinkMips(img, {})
        m = lm.m
        m.fill(DMD_LIST_AT, dmd.d)
        hidden = set()
        want = dmd_list_model(m, dmd.roots, hidden)

        def run():
            n = lm.run('portDmdMutableList', (DMD_LIST_OUT, DMD_LIST_MAX), steps=200000000)
            n = n - (1 << 32) if n & 0x80000000 else n
            if n < 0:
                return None
            return [(m.rd(DMD_LIST_OUT + 8 * i, 4), m.rd(DMD_LIST_OUT + 8 * i + 4, 4)) for i in range(n)]
        got = run()
        for node, at in hidden:
            m.wr(node + at, 1, 0)
        taken = run()
        types = sorted({m.rd(node, 1) for node, at in hidden})
        if got != want:
            bad.append('request 106 run, %s: %s ranges, the model %d (first differing: %s)' % (
                name, len(got) if got is not None else 'too many', len(want),
                next((('%08X+%X' % g, '%08X+%X' % w) for g, w in zip(got or [], want) if g != w), None)))
        elif taken != got:
            bad.append('request 106 run, %s: with the %d pickups hidden %s ranges (want the same %d)' % (
                name, len(hidden), len(taken) if taken is not None else 'too many', len(got)))
        else:
            notes.append('%s %d ranges, %d holders (types %s)' % (name, len(got), len(hidden), types))
    dmd_list_run.notes = notes
    return bad


def start_link_run(img):
    """Request 85: portStartNodePick and portStartLinkPlace in check_custom's LinkMips on a made-up route."""
    import math
    from check_custom import LinkMips
    syms = img.link.symbols
    need = ('aiRouteNodes', 'aiRouteNodeCount', 'startNodeList', 'aiCars', 'humanCarTotal', 'aiCarTotal',
            'portStartPlaced', 'portStartLink', 'portCarOptions')
    miss = [n for n in need if n not in syms]
    if miss or 'portStartLinkPlace' not in img.sizes:
        return ['request 85 run: %s not in the link' % (miss or 'portStartLinkPlace')]
    A = {n: syms[n]['address'] for n in need}
    nodes = [(int(round(2400 * math.cos(i * math.pi / 4))), int(round(2400 * math.sin(i * math.pi / 4))), 40 * i)
             for i in range(8)]
    links = [(i, (i + 1) % 8) for i in range(8)]

    def cdiv(a, b):
        return -(-a // b) if (a < 0) != (b < 0) else a // b

    def samples():
        for i, m in links:
            dx, dy = nodes[m][0] - nodes[i][0], nodes[m][1] - nodes[i][1]
            n = max(2, min(16, (abs(dx) + abs(dy)) // 256))
            for k in range(1, n):
                q = lambda d: cdiv(d, n) * k + cdiv((d - n * cdiv(d, n)) * k, n)
                yield i, m, nodes[i][0] + q(dx), nodes[i][1] + q(dy)

    def d2(dx, dy):
        dx, dy = min(abs(cdiv(dx, 16)), 30000), min(abs(cdiv(dy, 16)), 30000)
        return dx * dx + dy * dy
    bad = []
    for case, cars, ground in (('a car on every node', [n[:2] for n in nodes], 1234),
                               ('a car on every node, the ground far below (a gap)', [n[:2] for n in nodes], -2000),
                       ('cars on the nodes and the links\' middles',
                        [n[:2] for n in nodes] + [((nodes[i][0] + nodes[m][0]) // 2, (nodes[i][1] + nodes[m][1]) // 2)
                                                  for i, m in links], 1234),
                       ('one car', [nodes[0][:2]], 1234)):
        lm = LinkMips(img, {'worldGroundHeightAt': lambda mm, g=ground: g})
        m = lm.m
        m.wr(A['portCarOptions'], 4, 4)
        for i, (x, y, z) in enumerate(nodes):
            b = A['aiRouteNodes'] + 24 * i
            m.wr(b, 4, x & 0xFFFFFFFF)
            m.wr(b + 4, 4, y & 0xFFFFFFFF)
            m.wr(b + 8, 2, z)
            m.wr(b + 10, 2, (i + 1) % 8)
            m.wr(b + 22, 1, 1)
        m.wr(A['aiRouteNodeCount'], 4, 8)
        for i in range(15):
            m.wr(A['startNodeList'] + 2 * i, 2, i if i < 8 else 0)
            m.wr(A['startNodeList'] + 0x1E + i, 1, 1 if i < 8 else 0)
        m.wr(A['humanCarTotal'], 4, 0)
        m.wr(A['aiCarTotal'], 4, len(cars))
        for j, (x, y) in enumerate(cars):
            rec = A['aiCars'] + 0x854 * j
            m.wr(rec + 0x73C, 4, x & 0xFFFFFFFF)
            m.wr(rec + 0x740, 4, y & 0xFFFFFFFF)
            m.wr(rec + 0x5A0, 4, 100)
            m.wr(A['portStartPlaced'] + 16 + j, 1, 1)
        got = lm.run('portStartNodePick', (8,))
        got = got - (1 << 32) if got & 0x80000000 else got
        gaps = [(min(d2(t[2] - x, t[3] - y) for x, y in cars), t) for t in samples()]
        top = max(g for g, t in gaps)
        first = next(t for g, t in gaps if g == top)
        want_link = case == 'a car on every node' and top >= (700 // 16) ** 2 and ground >= min(n[2] for n in nodes) - 160
        setv = m.rd(A['portStartLink'], 4)
        if want_link:
            i, mm, x, y = first
            pt = tuple(m.rd(A['portStartLink'] + 4 * k, 4) for k in range(1, 6))
            pt = tuple(v - (1 << 32) if v & 0x80000000 else v for v in pt)
            if got != i or setv != 1 or pt != (x, y, max(nodes[i][2], nodes[mm][2]), i, mm):
                bad.append('request 85 run (%s): pick %d, link %d %s (want %d, 1 %s)' % (
                    case, got, setv, pt, i, (x, y, max(nodes[i][2], nodes[mm][2]), i, mm)))
                continue
            out = 0x80A70000
            for k in range(6):
                m.wr(out + 4 * k, 4, 0)
            rv = lm.run('portStartLinkPlace', (out, out + 12, out + 16, out + 20))
            vals = [m.rd(out + 4 * k, 4) for k in range(6)]
            vals = [v - (1 << 32) if v & 0x80000000 else v for v in vals]
            again = lm.run('portStartLinkPlace', (out + 32, out + 44, out + 48, out + 52))
            dx, dy = nodes[mm][0] - nodes[i][0], nodes[mm][1] - nodes[i][1] or 1
            if rv != 1 or vals[0] != x or vals[1] != y or vals[4] != ground or vals[5] != mm or again != 0:
                bad.append('request 85 run: portStartLinkPlace %d %s, again %d (want 1, x %d y %d ground %d to %d, 0)' % (
                    rv, vals, again, x, y, ground, mm))
            want_h = math.atan2(dx, dy) * 2048 / math.pi
            h = vals[3] if vals[3] < 2048 else vals[3] - 4096
            if abs(((h - want_h + 2048) % 4096) - 2048) > 8:
                bad.append('request 85 run: heading %d (want about %.0f of 4096)' % (vals[3], want_h))
        elif setv != 0 or not 0 <= got < 8:
            bad.append('request 85 run (%s): pick %d, a link start set %d (want a node, none)' % (case, got, setv))
    return bad


def check(img, e0, base, cars, defined=True, humans=False):
    ok = True
    expected = EXPECTED | (set() if humans else EXPECTED_VIEWS)

    # 1. port functions
    missing = sorted(n for n in PORT_FUNCS if img.kinds.get(n) != 'port')
    rand_port = sorted(n for n in PORT_FUNCS if n in img.kinds and reaches(img.calls, n))
    bad = []
    if 'portRollbackSeedFor' in img.kinds:
        if 'GetCurTics' not in img.calls.get('portRollbackSeedFor', set()):
            bad.append('portRollbackSeedFor does not call GetCurTics')
        refs = {n for n, k in img.word_refs(PORT_WORDS).get('portRollbackSeedFor', set())}
        if not {'portRollbackOptions', 'portRollbackSeed'} <= refs:
            bad.append('portRollbackSeedFor reads %s' % sorted(refs))
    if 'portRollbackPreload' in img.kinds:
        if not {'srand', 'portRollbackSeedFor'} <= img.calls.get('portRollbackPreload', set()):
            bad.append('portRollbackPreload calls %s' % sorted(img.calls.get('portRollbackPreload', set())))
        tw = img.word_refs({'timerFrameCount'}).get('portRollbackPreload', set())
        if ('timerFrameCount', 'store') not in tw:
            bad.append('portRollbackPreload does not store timerFrameCount')
    for n in sorted(STEP_RUNNERS):
        if img.kinds.get(n) != 'port':
            missing.append(n)
        elif img.jal_counts(n, 'rand') or img.jal_counts(n, 'srand'):
            bad.append('%s calls rand() / srand() itself' % n)
    if img.kinds.get('portRollbackStepEffects') == 'port':
        got = img.calls.get('portRollbackStepEffects', set())
        if got != STREAK_TICKS:
            bad.append('portRollbackStepEffects calls %s (want %s)' % (sorted(got), sorted(STREAK_TICKS)))
    if img.kinds.get('portRollbackAnimStep') == 'port':
        got = img.calls.get('portRollbackAnimStep', set())
        if not {'csGetRoot', 'csListHead', 'viewGetFollowObj'} <= got:
            bad.append('portRollbackAnimStep calls %s' % sorted(got))
        for anim in ANIMATORS:
            if not reaches(img.calls, 'portRollbackAnimStep', anim):
                bad.append('portRollbackAnimStep does not reach %s' % anim)
    # stage 3: the step runners
    for n in sorted(STEP3_RUNNERS):
        if img.kinds.get(n) != 'port':
            missing.append(n)
        elif img.jal_counts(n, 'rand') or img.jal_counts(n, 'srand'):
            bad.append('%s calls rand() / srand() itself' % n)
    if img.kinds.get('portStepBody') == 'port':
        fl = img.flow('portStepBody')
        seq = [c for a, c in sorted(fl.calls.items()) if c in STEP_ORDER]
        if seq != STEP_ORDER:
            bad.append('portStepBody calls %s (want %s)' % (seq, STEP_ORDER))
        others = sorted({c for c in fl.calls.values()} - set(STEP_ORDER) -
                        {'gameGetLevel', 'timerGetFrameTicks', 'portStepStackScrub'})
        if others:
            bad.append('portStepBody also calls %s' % others)
        if ('portResimulating', 'load') not in img.word_refs({'portResimulating'}).get('portStepBody', set()):
            bad.append('portStepBody does not read portResimulating (padUpdate on live steps only)')
    bad += step_scrub_check(img)
    bad += voice_owner_check(img)
    bad += stack_check(img)
    for n in ('portRunStep', 'portRollbackLoopStep'):
        if img.kinds.get(n) == 'port' and img.calls.get(n, set()) != {'portStepBody'}:
            bad.append('%s calls %s (want portStepBody only)' % (n, sorted(img.calls.get(n, set()))))
    for n, want in (('portClutLoadLine', 'gpuLoadImageLine'), ('portClutStoreLine', 'gpuStoreImageLine'),
                    ('portClutShadowCapture', 'gpuStoreImageLine'), ('portClutAnimUpload', 'gpuLoadImageLine'),
                    ('portRollbackHudStep', 'hudRadarSwapBuffers'), ('portSoundReconcile', 'soundStopInstance')):
        if img.kinds.get(n) == 'port' and want not in img.calls.get(n, set()):
            bad.append('%s does not call %s' % (n, want))
    # request 54: the list walks the loaded files (portDmdImages), not the live object tree
    if img.kinds.get('portDmdMutableList') == 'port':
        got = img.calls.get('portDmdMutableList', set())
        live = img.word_refs({'csObjectPool', 'renderWorldModels', 'csPoolCount'}).get('portDmdMutableList', set())
        if 'csGetRoot' in got or live:
            bad.append('portDmdMutableList still reads the live object tree (%s %s)' % (sorted(got), sorted(live)))
        if not img.word_refs({'portDmdImages'}).get('portDmdMutableList'):     # a load, or its address (the loop)
            bad.append('portDmdMutableList does not read portDmdImages')
        bad += dmd_list_run(img)    # request 106: the list as the model's, the same with the pickups taken
    if img.kinds.get('portDmdNoteImage') == 'port':
        if img.calls.get('portDmdNoteImage'):
            bad.append('portDmdNoteImage calls %s' % sorted(img.calls['portDmdNoteImage']))
        if ('portDmdImages', 'store') not in img.word_refs({'portDmdImages'}).get('portDmdNoteImage', set()):
            bad.append('portDmdNoteImage does not store portDmdImages')
    # request 55: the round end's peek reads the events and the ticks, stores nothing, runs nothing
    if img.kinds.get('portRollbackEndDue') == 'port':
        got = img.calls.get('portRollbackEndDue', set())
        if got != {'timerGetFrameTicks'}:
            bad.append('portRollbackEndDue calls %s (want timerGetFrameTicks only)' % sorted(got))
        ev = img.word_refs({'rtScheduledEvents', 'D_801809D4'}).get('portRollbackEndDue', set())
        if not {('rtScheduledEvents', 'load'), ('D_801809D4', 'load')} <= ev:
            bad.append('portRollbackEndDue reads %s' % sorted(ev))
        start, size = img.sizes['portRollbackEndDue']
        st = sorted('%08X' % p for p, t, v, it, sy in img.relocs if t in (6, 7) and start <= p < start + size
                    and img.link.word(p) >> 26 in STORES)
        if st:
            bad.append('portRollbackEndDue stores to static words at %s (a peek stores nothing)' % st)
    good = not missing and not rand_port and not bad
    if getattr(dmd_list_run, 'notes', None):
        print('1. request 106: portDmdMutableList as the model, the same with every pickup taken: %s' % (
            '; '.join(dmd_list_run.notes)))
    print('1. port functions: %d in .port_text (missing %s); reaching rand(): %s; step runners %s -> %s' % (
        len(PORT_FUNCS) - len(missing), missing or 'none', rand_port or 'none',
        ' '.join(sorted(STEP_RUNNERS | STEP3_RUNNERS)), 'ok' if good else 'FAIL'))
    for b in bad:
        print('     ' + b)
    ok = good

    # 2. no new rand()
    bad = []
    nfun = 0
    for fn in sorted(img.sizes):
        if fn in expected or (cars and fn in EXPECTED_CARS_49):
            nfun += 1
            for callee in ('rand', 'srand'):
                a, b = img.jal_counts(fn, callee), base.jal_counts(fn, callee)
                if fn in ANIM_RAND_FUNCS and callee == 'rand' and 1 <= a <= b:
                    continue    # RB_ANIM_RAND: the compiler may merge the rand() of both branches (animPickPeriod)
                if a != b:
                    bad.append('%s calls %s %d times (base %d)' % (fn, callee, a, b))
        elif img.kinds.get(fn) == 'port' and fn.startswith('portRollback'):
            if img.jal_counts(fn, 'rand'):
                bad.append('%s calls rand' % fn)
            if img.jal_counts(fn, 'srand') and fn != 'portRollbackPreload':
                bad.append('%s calls srand' % fn)
    good = not bad and nfun >= len(expected)
    print('2. rand() / srand() call sites: %d changed functions compared with the base variant -> %s' % (
        nfun, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    ok = ok and good

    # 3. the seed sites
    bad = []
    for fn in SEED_SITES:
        if fn not in img.sizes:
            bad.append('%s not in the link' % fn)
            continue
        fl = img.flow(fn)
        if any(c == 'GetCurTics' for c in fl.calls.values()):
            bad.append('%s still calls GetCurTics' % fn)
        srands = sorted(a for a, c in fl.calls.items() if c == 'srand')
        if len(srands) != e0.jal_counts(fn, 'srand') or not srands:
            bad.append('%s: %d srand calls (E0 %d)' % (fn, len(srands), e0.jal_counts(fn, 'srand')))
        for s in srands:
            before = sorted(a for a in fl.calls if a < s)
            if not before or fl.calls[before[-1]] != 'portRollbackSeedFor':
                bad.append('%s: srand at %08X not right after portRollbackSeedFor' % (fn, s))
                continue
            seedcall = before[-1]
            # $a0 must come from $v0 between the call's delay slot and srand's delay slot
            a0 = None
            for a in range(seedcall + 8, s + 8, 4):
                w = fl.insn[a]
                op, rs, rt, rd, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
                if op == 0 and rd == 4 and funct in (0x21, 0x25) and {rs, rt} == {2, 0}:
                    a0 = 'v0'
                elif (op == 0 and rd == 4 and funct not in (8, 9)) or (op not in (0, 2, 3) and op < 0x28 and rt == 4
                                                                         and op not in (4, 5, 1, 6, 7)):
                    a0 = 'other'
            if a0 != 'v0':
                bad.append('%s: srand at %08X does not take portRollbackSeedFor\'s result in $a0' % (fn, s))
    fn = 'sessionLoadArenaAndCars'
    if fn in img.sizes:
        others = [(a, c) for a, c in img.flow(fn).calls.items() if c != 'portRollbackPreload']
        g, lines = gate(img, fn, 'portRollbackPreload', others, '3. load seed', dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    else:
        bad.append('sessionLoadArenaAndCars not in the link')
    good = not bad
    print('3. seed sites: %s srand from portRollbackSeedFor, the load seed first -> %s' % (
        ' '.join(SEED_SITES), 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    ok = ok and good

    # 4. gates
    if 'rtMainLoop' in img.sizes:
        fl = img.flow('rtMainLoop')
        worlds = sorted(a for a, c in fl.calls.items() if c == 'worldUpdate')
        steps = [(a, c) for a, c in fl.calls.items() if c in STEP_CALLS] + [(a, 'worldUpdate') for a in worlds[1:]]
        missing = sorted(STEP_CALLS - {c for _, c in steps})
        g, lines = gate(img, 'rtMainLoop', 'portRollbackHeld', steps, '4. step calls')
        if missing or len(worlds) < 2:
            g = False
            lines.append('     missing step calls %s, worldUpdate calls %d' % (missing, len(worlds)))
        ok = ok and g
        print('\n'.join(lines))
        g, lines = gate(img, 'rtMainLoop', 'portRollbackTop', call_sites(img, 'rtMainLoop', {'rtUpdateScheduledEvents'}),
                        '4. event countdown')
        ok = ok and g
        print('\n'.join(lines))
        g, lines = gate(img, 'rtMainLoop', 'portRollbackLoopEnter', [(worlds[0], 'worldUpdate')] if worlds else [],
                        '4. first step ticks', dominate_only=True)
        if 'portRollbackFrameTime' not in fl.calls.values():
            g = False
            lines.append('     no call of portRollbackFrameTime')
        ok = ok and g
        print('\n'.join(lines))
    else:
        ok = False
        print('4. rtMainLoop not in the link -> FAIL')
    for fn, names, gate_fn, label in (
            ('streaksDraw', STREAK_VIEW_TICKS, 'portRollbackViewTickOff', '4. streak ticks (per view)'),
            ('streaksDraw', STREAK_SPLIT, 'portRollbackViewDrawOff', '4. streak split functions'),
            ('animUpdateAll', ANIMATORS, 'portRollbackAnimDrawOff', '4. animators (list)'),
            ('animUpdateOne', ANIMATORS, 'portRollbackAnimDrawOff', '4. animators (one)')):
        if fn not in img.sizes:
            ok = False
            print('%s: %s not in the link -> FAIL' % (label, fn))
            continue
        sites = call_sites(img, fn, names)
        g, lines = gate(img, fn, gate_fn, sites, label)
        if fn == 'streaksDraw' and {c for _, c in sites} != names:
            g = False
            lines.append('     ticks found: %s' % sorted({c for _, c in sites}))
        ok = ok and g
        print('\n'.join(lines))
    # stage 2: the split functions and the animators' rand()
    bad = []
    for fn in ('fxSheetDisplay', 'weaponArcsDraw'):
        if fn not in img.sizes:
            bad.append('%s not in the link' % fn)
            continue
        others = [(a, c) for a, c in img.flow(fn).calls.items() if c != 'portRollbackPass']
        g, lines = gate(img, fn, 'portRollbackPass', others, fn, dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    fn = 'weaponArcsDraw'
    if fn in img.sizes:
        nv, nr = img.jal_counts(fn, 'portRollbackVisualRand'), img.jal_counts(fn, 'rand')
        if nv != 6 or nr != base.jal_counts(fn, 'rand'):
            bad.append('%s: %d visual and %d rand() sites (want 6 and the base\'s %d)' % (
                fn, nv, nr, base.jal_counts(fn, 'rand')))
        others = [(a, c) for a, c in img.flow(fn).calls.items() if c == 'portRollbackVisualRand']
        g, lines = gate(img, fn, 'portRollbackArcVisual', others, fn, dominate_only=True)
        if not g:
            bad += [l.strip() for l in lines]
    for fn in ANIM_RAND_FUNCS:
        if fn not in img.sizes:
            bad.append('%s not in the link' % fn)
            continue
        nv, nr = img.jal_counts(fn, 'portRollbackVisualRand'), img.jal_counts(fn, 'rand')
        nb = base.jal_counts(fn, 'rand')
        if nv != nb or not 1 <= nr <= nb:     # merged rand() sites allowed (animPickPeriod: 2 visual, 1 rand)
            bad.append('%s: %d visual and %d rand() sites (base %d)' % (fn, nv, nr, nb))
        vis = sorted(a for a, c in img.flow(fn).calls.items() if c == 'portRollbackAnimVisual')
        if len(vis) != nb:
            bad.append('%s: %d portRollbackAnimVisual calls' % (fn, len(vis)))
    if 'portRollbackOncePerStep' not in img.calls.get('fxSheetStart', set()):
        bad.append('fxSheetStart does not call portRollbackOncePerStep')
    good = not bad
    print('4. split functions: portRollbackPass first in fxSheetDisplay / weaponArcsDraw, the visual stream at the '
          'jitter and animator sites, fxSheetStart without split doubling -> %s' % ('ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    ok = ok and good
    fn = 'SetFrameStart'
    if fn in img.sizes:
        start, size = img.sizes[fn]
        stores = sorted({(p, 'store ' + '/'.join(sorted((img.addr_names.get(v, set()) | {s}) & TIMER_WORDS)))
                         for p, t, v, it, s in img.relocs if t in (6, 7) and start <= p < start + size
                         and img.link.word(p) >> 26 in STORES and (img.addr_names.get(v, set()) | {s}) & TIMER_WORDS})
        g, lines = gate(img, fn, 'portRollbackFixedFrame', stores, '4. frame measurement')
        ok = ok and g
        print('\n'.join(lines))
    else:
        ok = False
        print('4. SetFrameStart not in the link -> FAIL')
    fn = 'pauseMenu'
    if fn in img.sizes:
        others = [(a, c) for a, c in img.flow(fn).calls.items() if c != 'portRollbackPauseOff']
        g, lines = gate(img, fn, 'portRollbackPauseOff', others, '4. pause menu')
        ok = ok and g
        print('\n'.join(lines))
    else:
        ok = False
        print('4. pauseMenu not in the link -> FAIL')
    ok = check_stage3(img, base) and ok

    # 5. latch order
    bad = []
    refs = img.word_refs(PORT_WORDS)
    users = {}
    for fn, hits in refs.items():
        for name, kind in hits:
            users.setdefault((name, kind), set()).add(fn)
    hold_readers = users.get(('portStepHold', 'load'), set())
    if hold_readers != {'portRollbackTop'}:
        bad.append('portStepHold read by %s (want portRollbackTop)' % sorted(hold_readers))
    holding_writers = users.get(('portRollbackHolding', 'store'), set())
    if holding_writers != {'portRollbackTop', 'portRollbackLoopEnter', 'portRunStep'}:
        bad.append('portRollbackHolding written by %s' % sorted(holding_writers))
    if users.get(('portRollbackHolding', 'load'), set()) - {'portRollbackHeld', 'portRollbackTop',
                                                            'portRollbackViewTickOff', 'portRollbackViewDrawOff',
                                                            'portRollbackAnimDrawOff', 'portRunStep',
                                                            'portRollbackLoopStep'}:
        bad.append('portRollbackHolding read by %s' % sorted(users[('portRollbackHolding', 'load')]))
    # stage 3: the split latch, the pending end, the re-run flag (the port's: no C writer), the palette shadow
    w = users.get(('portRollbackSplitting', 'store'), set())
    if w != {'portRollbackTop', 'portRollbackLoopEnter'}:
        bad.append('portRollbackSplitting written by %s' % sorted(w))
    r = users.get(('portRollbackSplitting', 'load'), set())
    if r - {'portRollbackTop', 'portRollbackHeld', 'portRollbackRadarInStep', 'portRollbackLoopStep'} or not r:
        bad.append('portRollbackSplitting read by %s' % sorted(r))
    w = users.get(('portRollbackEnded', 'store'), set())
    if w != {'portRollbackLoopEnter', 'portRollbackLoopStep', 'portRunStep'}:
        bad.append('portRollbackEnded written by %s' % sorted(w))
    if users.get(('portResimulating', 'store')):
        bad.append('portResimulating written by %s (the port\'s flag)' % sorted(users[('portResimulating', 'store')]))
    r = users.get(('portResimulating', 'load'), set())
    if not {'portRollbackSoundOff', 'portClutLoadLine', 'portStepBody'} <= r:
        bad.append('portResimulating read by %s' % sorted(r))
    cs = {fn for (n, k), fns in users.items() if n == 'portClutShadow' for fn in fns}
    if cs - {'portClutRow', 'portClutLoadLine', 'portClutStoreLine', 'portClutShadowCapture', 'portClutAnimUpload',
             'portClutShadowReset', None} or not cs:
        bad.append('portClutShadow used by %s' % sorted(x for x in cs if x))
    quit_users = users.get(('portRollbackQuit', 'load'), set()) | users.get(('portRollbackQuit', 'store'), set())
    if quit_users != {'portRollbackTop'}:
        bad.append('portRollbackQuit used by %s' % sorted(quit_users))
    if 'rtScheduleEvent' not in img.calls.get('portRollbackTop', set()):
        bad.append('portRollbackTop does not call rtScheduleEvent')
    vs_users = users.get(('portRollbackVisualSeed', 'load'), set()) | users.get(('portRollbackVisualSeed', 'store'),
                                                                                set())
    if vs_users != {'portRollbackVisualRand'}:
        bad.append('portRollbackVisualSeed used by %s' % sorted(vs_users))
    di = {fn for (n, k), fns in users.items() if n == 'portDmdImages' for fn in fns} - {None}
    if di - {'portRollbackPreload', 'portDmdNoteImage', 'portDmdMutableList', 'portDmdWalkFile',
             'portCarUnloadGroup'} or not di:
        bad.append('portDmdImages used by %s' % sorted(di))
    # request 54 (3): the follow object portRollbackAnimStep walks first is set at the load only (the level's id-2
    # model), so every machine walks the same object; the parity guard steps a node reached twice once
    fw = {f for f, hits in img.word_refs({'viewFollowObjects'}).items() if ('viewFollowObjects', 'store') in hits}
    reach_sd, todo = set(), ['portStepBody'] + [r for r in DRAW_ROOTS]
    while todo:
        f = todo.pop()
        if f not in reach_sd:
            reach_sd.add(f)
            todo.extend(img.calls.get(f, ()))
    if fw - {'viewSetFollowObj', 'viewInit'} or not fw or fw & reach_sd:
        bad.append('viewFollowObjects written by %s (want viewSetFollowObj, viewInit; none in the step or the '
                   'drawing)' % sorted(fw, key=str))
    game = sorted(fn for fn in refs if fn and img.kinds.get(fn) != 'port')
    if game:
        bad.append('game functions read the port words directly: %s' % game)
    good = not bad
    print('5. latch order: portStepHold latched by portRollbackTop, the gates read the latch -> %s' % (
        'ok' if good else 'FAIL'))
    for b in bad:
        print('     ' + b)
    ok = ok and good

    # 6. request 49
    if cars:
        bad = []
        if 'portStartPlacedClear' not in img.calls.get('startNodesClearUsed', set()):
            bad.append('startNodesClearUsed does not call portStartPlacedClear')
        for fn in ('humanCarPlaceAtStart', 'aiCarStart'):
            if fn not in img.sizes:
                bad.append('%s not in the link' % fn)
                continue
            fl = img.flow(fn)
            marks = sorted(a for a, c in fl.calls.items() if c == 'portStartPlacedMark')
            picks = sorted(a for a, c in fl.calls.items() if c == 'startNodePickRandom')
            if len(marks) != 2 or len(picks) != 1:
                bad.append('%s: %d marks, %d picks' % (fn, len(marks), len(picks)))
                continue
            if picks[0] in fl.reach(fl.start, {marks[0]}):
                bad.append('%s: the pick is reachable without the first mark' % fn)
            if marks[1] in fl.reach(fl.start, {picks[0]}):
                bad.append('%s: the second mark is reachable without the pick' % fn)
        pick_refs = {n for n, k in img.word_refs({'portStartPlaced'}).get('portStartNodePick', set())}
        if 'portStartPlaced' not in pick_refs:
            bad.append('portStartNodePick does not read portStartPlaced')
        for fn in ('portStartPlacedClear', 'portStartPlacedMark', 'portStartLinkPlace'):
            if img.kinds.get(fn) != 'port' or reaches(img.calls, fn):
                bad.append('%s not in .port_text or reaches rand()' % fn)
        # request 85: the link start placed after each pick, and run
        for fn in ('humanCarPlaceAtStart', 'aiCarStart'):
            if fn not in img.sizes:
                continue
            fl = img.flow(fn)
            places = sorted(a for a, c in fl.calls.items() if c == 'portStartLinkPlace')
            picks = sorted(a for a, c in fl.calls.items() if c == 'startNodePickRandom')
            if len(places) != 1 or len(picks) != 1 or places[0] in fl.reach(fl.start, {picks[0]}):
                bad.append('%s: %d portStartLinkPlace calls, not all after the pick' % (fn, len(places)))
        bad += start_link_run(img)
        good = not bad
        print('6. request 49: placed cars cleared, marked around each start pick, read by the picker; request 85: '
              'link starts -> %s' % (
            'ok' if good else 'FAIL'))
        for b in bad:
            print('     ' + b)
        ok = ok and good
    if not defined:
        print('negative control: %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', help='port variant link directory (build_source_only.py --output)')
    ap.add_argument('--e0', help='E0 link (default: the latest full build\'s source_reloc_e0)')
    ap.add_argument('--base', help='the port variant without port defines (default: source_reloc_port '
                    'beside the E0 link)')
    ap.add_argument('--expect-functions', action='store_true',
                    help='also compare the functions the port defines changed with EXPECTED')
    ap.add_argument('--negative-control', action='store_true',
                    help='run checks 1-5 on a build without TM2_PORT_ROLLBACK (they must fail)')
    ap.add_argument('--regions', default=None,
                    help='the region list for checks 8 and 9 (default config/US/rollback_regions.tsv; a copy with a '
                         'row removed is the negative control of check 8, request 53\'s list that of check 9)')
    a = ap.parse_args()
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    e0p = (Path(a.e0) if Path(a.e0).is_absolute() else ROOT / a.e0) if a.e0 else latest_e0()
    if e0p is None:
        raise SystemExit('no E0 link (--e0)')
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    pv = report.get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    defined = 'TM2_PORT_ROLLBACK' in names
    cars = 'TM2_PORT_CARS' in names
    humans = 'TM2_PORT_HUMANS' in names
    if not defined and not a.negative_control:
        print('%s was built without TM2_PORT_ROLLBACK (port defines %s): nothing to check; use --negative-control'
              % (a.out, sorted(names)))
        sys.exit(1)
    basep = (Path(a.base) if Path(a.base).is_absolute() else ROOT / a.base) if a.base else e0p.parent / 'source_reloc_port'
    img, e0, base = Image(out), Image(e0p), Image(basep)
    if not defined:
        print('note: no TM2_PORT_ROLLBACK: checks 1-5 must fail (negative control)')
        ok = not check(img, e0, base, cars=False, defined=False)
        print('CHECK ROLLBACK NEGATIVE CONTROL %s' % ('OK' if ok else 'FAILED'))
        sys.exit(0 if ok else 1)
    ok = check(img, e0, base, cars, humans=humans)
    import rollback_regions
    ok = check_regions(img, out, a.regions or rollback_regions.REGIONS) and ok
    ok = check_draw_hash(img, out, a.regions or rollback_regions.REGIONS) and ok
    ok = check_views(img, out, a.regions or rollback_regions.REGIONS, humans) and ok
    ok = check_request59(img, out, a.regions or rollback_regions.REGIONS) and ok
    proj_n = next((x['entries'] for x in pv.get('arrays') or [] if x.get('name') == 'fxRecords'), 16)
    ok = check_request60(img, out, proj_n) and ok
    if not humans:      # the gated site must be found with the bit clear (the check is not blind)
        ok = check_views(img, out, a.regions or rollback_regions.REGIONS, humans, bit_clear=True) and ok
    else:               # request 58: this machine's humans, and the INPUT_ALL model clear (not blind)
        ok = check_locals(img, out, a.regions or rollback_regions.REGIONS) and ok
        ok = check_locals(img, out, a.regions or rollback_regions.REGIONS, bit_clear=True) and ok
    if a.expect_functions:
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
        want = EXPECTED | (EXPECTED_CARS_49 if cars else set()) | (set() if humans else EXPECTED_VIEWS)
        others = sorted(names - {'TM2_PORT_ROLLBACK'})
        missing, extra = want - changed, changed - want
        good = not missing and (not extra or bool(others))
        print('7. port_changed functions: %d (expected %d)%s; missing %s; extra %s -> %s' % (
            len(changed), len(want), ' with other port defines' if others else '', sorted(missing),
            sorted(extra) if not others else '%d (other defines)' % len(extra), 'ok' if good else 'FAIL'))
        ok = ok and good
    print('CHECK ROLLBACK %s' % ('OK' if ok else 'FAILED'))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
