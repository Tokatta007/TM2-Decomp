/* Online play, stages 1 to 3: the step made deterministic and re-runnable (PC port request 48, docs/ONLINE_ROLLBACK.md
 * 8, "Stage 1 as built" (10), "Stage 2 as built" (11) and "Stage 3 as built" (12); request 48 complete; request 51,
 * section 13: the world collision per drawn view is bit 3's case, no new bit; requests 52-53, section 14: with bit 3
 * the step's animators draw from the game's rand() stream (0x80009010, in RAM): a host-side wrapper that swaps that
 * seed around them (the port's interp.c visual_rand) must pass through while bit 3 is set; requests 54-55, section
 * 15: portDmdMutableList lists every model file loaded for the match (the level's every root, every installed car
 * file: portDmdImages), not what the objects hold now; the drawing's words are out of the hash; portRollbackEndDue
 * peeks the next step's round end; section 18: this machine's view settings (the split screen, the rear-view
 * mirror) audited in the step: one game-state reader, humanCarTick's weapon regeneration without TM2_PORT_HUMANS,
 * behind bit 10 LOCAL_VIEWS; every other read is presentation and the settings are hash no).
 *
 * With -DTM2_PORT_ROLLBACK (the port's `--port-define TM2_PORT_ROLLBACK`) the port gets one options word and a few
 * words it writes before a match or between steps; with the define off, or with every word zero, the build and the
 * game are retail's.
 *
 *   portRollbackOptions     the bits below (8.1's numbering; the bits of later stages are reserved), 0 = retail
 *   portResimulating        set by the port around a batch of re-run steps (portRunStep); read by SOUND_GATE,
 *                           CLUT_SHADOW and portStepBody (padUpdate on live steps only), and by the port's
 *                           hle_SetFrameStart hooks
 *   portStepHold            HOLD: non-zero = the next iterations of rtMainLoop draw only (latched once per iteration)
 *   portStepTicks           FIXED_TICKS: the ticks of every step; 0 = 3 (20 steps a second) in every game mode, co-op
 *                           (mode 2) included: the user's decision, not retail's 15 a second in co-op; 3..5 used as
 *                           is, other values clamped to 3..5
 *   portRollbackSeed        SEED: the match seed (the host's)
 *   portRollbackFrameCount  SEED: timerFrameCount at the start of each load (the host's; usually 0)
 *   portRollbackQuit        PAUSE_PORT: non-zero = quit the battle at the top of the next step (rtScheduleEvent(2, 0),
 *                           as retail's QUIT); the C clears it
 *   portRollbackHolding     written by the C: the HOLD latch of this iteration (1 = drawing only)
 *   portRollbackReseeds     written by the C: the srand calls since the load's seed (SEED's per-site mix)
 *   portRollbackVisualSeed  stage 2: the visual rand() stream's state (portRollbackVisualRand: retail's rand() formula
 *                           on its own word); the drawing-only draws take it (ONCE_PER_STEP's arc drawing,
 *                           VISUAL_RAND); any value; presentation (leave it out of the hash, never restore it)
 *   portRollbackSplitting   stage 3, written by the C: the STEP_SPLIT latch of this iteration
 *   portRollbackEnded       stage 3, written by the C: a re-run fired an event; rtMainLoop leaves at its next top
 *   portClutShadow          stage 3 CLUT_SHADOW: the palette rows in RAM (game state: snapshot and hash)
 * Request 60 (section 21): with bit 11 ROUND_CLEAN each round's start clears the projectile records (fxRecords: the type,
 * the target and the rest a fire sets; a carried-over seeker steered at the old level's pointer) and a seeker whose
 * target is no counted car object's position stops steering.
 *
 *   portDmdImages           request 54, written by the C: the model files installed for the match (assetInstallImage's
 *                           base per load group; cleared at each load and by an unload); portDmdMutableList walks
 *                           them; load-time bookkeeping, not read by a step (snapshot and hash no)
 *
 * Stage 3's port calls: portRunStep() per re-run step (portResimulating set around the batch), then
 * portSoundReconcile() and portClutAnimUpload(); portDmdMutableList(out, max) after each load (the match's, and a
 * drop-in's portCarLoadGroupLive / portCarUnloadGroup); portRollbackEndDue() before each live step. What a
 * snapshot copies: config/US/rollback_regions.tsv, resolved per build (tools/rollback_regions.py,
 * out/TM2_SOURCE_PORT/rollback_regions.tsv).
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss: the stage 1 words after portRenderStats
 * and the matrix slots, portRollbackVisualSeed after request 49's portStartPlaced, stage 3's three after it, request
 * 54's portDmdImages after request 50's portHumanVacant); this header declares it for the
 * port variant and, with the define, the port functions the hooks call. None of them calls rand(). */
#ifndef TM2_SEMANTIC_PORT_ROLLBACK_H
#define TM2_SEMANTIC_PORT_ROLLBACK_H

/* portRollbackOptions bits (ONLINE_ROLLBACK.md 8.1) */
#define TM2_RB_FIXED_TICKS    0x001   /* bit 0: SetFrameStart: ticks = portStepTicks (rate 60 / ticks), no measurement;
                                         the first step of each round too (rtMainLoop's entry) */
#define TM2_RB_SEED           0x002   /* bit 1: the three srand(GetCurTics()) use portRollbackSeedFor(site); the load's
                                         seed and timerFrameCount = portRollbackFrameCount at the start of
                                         sessionLoadArenaAndCars */
#define TM2_RB_ONCE_PER_STEP  0x004   /* bit 2 (stage 2): streaksDraw only draws, in every view; the step runs its four
                                         ticks once (portRollbackStepEffects after view 0's streaksDraw: the flash
                                         countdown, weaponDropsUpdate, weaponFlameUpdate, the arcs' tick and damage);
                                         fxSheetStart no longer doubles the flash when split; the arcs' drawing takes
                                         its jitter from the visual stream */
#define TM2_RB_ANIM_IN_STEP   0x008   /* bit 3 (stage 2): the animations step in the step (portRollbackAnimStep after
                                         renderFlipFrame: the registered list, then every type-3 node of the level,
                                         the world models, the follow object and the visible objects, no culling);
                                         the renderer's animUpdateAll / animUpdateOne step nothing; needed online:
                                         the level's frames hold hit volumes (DISH, FREEWAY; request 51, 13); the
                                         animators' rand() is then the game's stream, as state (request 52, 14) */
#define TM2_RB_VISUAL_RAND    0x010   /* bit 4 (stage 2): draw-time rand() from the visual stream: the arcs' jitter
                                         (in every pass) and, without ANIM_IN_STEP, the four animator sites */
#define TM2_RB_SOUND_GATE     0x020   /* bit 5 (stage 3): while portResimulating, the sound wrappers (command_wrappers.cpp),
                                         soundUpdate, the port's instance merge and worldUpdate's CD track test do
                                         nothing; portSoundReconcile after a batch stops the projectile sounds whose
                                         projectile is gone */
#define TM2_RB_CLUT_SHADOW    0x040   /* bit 6 (stage 3): the palette animations read and write their VRAM rows through
                                         portClutShadow (RAM, in the snapshot); while portResimulating no upload;
                                         portClutAnimUpload after a batch (or a loaded snapshot) puts every row back;
                                         request 59 (3): each round's capture (rtMainLoop's entry) starts from an
                                         empty shadow (portClutShadowReset), so an earlier round's rows never come
                                         back (a tournament's earlier levels' palettes) */
#define TM2_RB_HOLD           0x080   /* bit 7: portStepHold set: rtMainLoop's iteration draws only, no state changes */
#define TM2_RB_STEP_SPLIT     0x100   /* bit 8 (stage 3): rtMainLoop runs the whole step first (portStepBody, at the
                                         top of the iteration), then draws; the port re-runs steps with portRunStep;
                                         the HUD's radar swap moves into the step */
#define TM2_RB_PAUSE_PORT     0x200   /* bit 9: pauseMenu returns at once; quit through portRollbackQuit */
#define TM2_RB_LOCAL_VIEWS    0x400   /* bit 10 (ONLINE_ROLLBACK.md 18): this machine's views are its own: the step
                                         reads the match's split mode (portRollbackViewSplit: retail's choice for the
                                         game mode and level, as sessionRun makes it), not viewSplitScreenMode, which
                                         the port sets per machine from its local players; the one game-state reader
                                         is humanCarTick's weapon regeneration in builds without TM2_PORT_HUMANS
                                         (with it H1 already tests the game mode); no effect in the port's build */
#define TM2_RB_ROUND_CLEAN    0x800   /* bit 11 (request 60, ONLINE_ROLLBACK.md 21): roundStart clears every projectile
                                         record before the round's projectilesReset (portRoundCleanProjectiles: type,
                                         life idle, speeds, target, victim, owner, trail counters; the record's own
                                         objects and trail strip stay the load's), so no record of an earlier round or
                                         level steers in the new round's first 11 steps (projectilesReset puts every
                                         record in the fade window, life -1); and weaponProjectileUpdate drops a
                                         target that is no counted car object's position (portSeekerTargetOk) */
#define TM2_RB_STAGE1         (TM2_RB_FIXED_TICKS | TM2_RB_SEED | TM2_RB_HOLD | TM2_RB_PAUSE_PORT)   /* 0x283 */
#define TM2_RB_STAGE2         (TM2_RB_STAGE1 | TM2_RB_ONCE_PER_STEP | TM2_RB_ANIM_IN_STEP | TM2_RB_VISUAL_RAND)
                                                                                                    /* 0x29F */
#define TM2_RB_STAGE3         (TM2_RB_STAGE2 | TM2_RB_SOUND_GATE | TM2_RB_CLUT_SHADOW | TM2_RB_STEP_SPLIT)
                                                                                                    /* 0x3FF */
#define TM2_RB_ONLINE         (TM2_RB_STAGE3 | TM2_RB_LOCAL_VIEWS | TM2_RB_ROUND_CLEAN)          /* 0xFFF */

/* CLUT_SHADOW: the RAM copy of the palette rows the animations touch (portClutShadow, .port_bss, every variant) */
#define TM2_RB_CLUT_ROWS 32
struct TM2PortClutRow {
    int x, y;                   /* the row as clutAnim's definition gives it (y with TEXTURE_BANKS' bank bits) */
    int count;                  /* colours held (16 or 256); 0 = free row */
    unsigned short c[256];
};
struct TM2PortClutShadow {
    int rows;                   /* rows in use */
    int overflow;               /* rows that found no room (they read VRAM as retail: raise TM2_RB_CLUT_ROWS) */
    struct TM2PortClutRow row[TM2_RB_CLUT_ROWS];
};
typedef char tm2_rb_clut_shadow_size[(sizeof(struct TM2PortClutShadow) == 8 + 524 * TM2_RB_CLUT_ROWS) ? 1 : -1];

/* Request 54 (ONLINE_ROLLBACK.md 15.1): the model files installed for the match, by load group (portDmdImages,
 * .port_bss, every variant). base[g] = assetInstallImage's buffer of group g (1..63; 0 = none: the level file is
 * always at 0x80019C40, group 0); overflow = installs whose group was out of range (their nodes are not listed) */
#define TM2_RB_DMD_IMAGES 64
#define TM2_RB_LEVEL_DMD 0x80019C40u   /* sessionLoadArenaAndCars' level file (never relocated) */
struct TM2PortDmdImages {
    unsigned int base[TM2_RB_DMD_IMAGES];
    int overflow;
};
typedef char tm2_rb_dmd_images_size[(sizeof(struct TM2PortDmdImages) == 4 * TM2_RB_DMD_IMAGES + 4) ? 1 : -1];

/* ONCE_PER_STEP's passes of the two split tick-and-draw functions (fxSheetDisplay, weaponArcsDraw; portRollbackPass) */
#define TM2_RB_PASS_BOTH  0   /* retail (the bit clear): the per-view call ticks and draws */
#define TM2_RB_PASS_TICK  1   /* the step's call (portRollbackStepEffects, OT / context 0): the tick only */
#define TM2_RB_PASS_DRAW  2   /* a view's call: the drawing only (no state, no shared rand()) */

/* SEED's sites (portRollbackSeedFor) */
#define TM2_RB_SITE_LOAD   0   /* sessionLoadArenaAndCars' start (portRollbackPreload): the load's rand() draws */
#define TM2_RB_SITE_SHELL  1   /* uaInit (ua_init.c), the shell */
#define TM2_RB_SITE_ARENA  2   /* arenaSetup's end (arena_setup.c), from sessionBindCars */
#define TM2_RB_SITE_ROUND  3   /* roundStart (scene_init.c), every round */

#ifdef TM2_PORT_VARIANT
#ifdef __cplusplus
extern "C" {
#endif
extern int portRollbackOptions;
extern int portResimulating;
extern int portStepHold;
extern int portStepTicks;
extern unsigned int portRollbackSeed;
extern unsigned int portRollbackFrameCount;
extern int portRollbackQuit;
extern int portRollbackHolding;
extern unsigned int portRollbackReseeds;
extern unsigned int portRollbackVisualSeed;
extern int portRollbackSplitting;
extern int portRollbackEnded;
extern struct TM2PortClutShadow portClutShadow;
extern struct TM2PortDmdImages portDmdImages;
/* Request 86 (ONLINE_ROLLBACK.md 23): the step's stack. The step reads stack words its own code has not written (a
 * local filled only on some paths, an out-parameter a callee leaves unset), so what the drawing left below the loop's
 * frame reaches the battle: each machine draws its own views, and a change to the drawing code (request 82's C near
 * pass) changed the 16-car start. With portStepScrub > 0, portStepBody first sets that many bytes below its frame
 * (rounded down to words, at most TM2_RB_SCRUB_MAX) to portStepScrubFill, so the step sees the same words on every
 * machine and in every build whatever was drawn. 0 = off (as before). The port's settings: snapshot and hash no. */
#define TM2_RB_SCRUB_MAX 0x8000
#define TM2_RB_SCRUB_STEP 0x1200        /* the step's deepest stack (check_rollback, `depth`): 0x11A0 in r82_port_P4 */
extern int portStepScrub;
extern unsigned int portStepScrubFill;
/* Request 89 (ONLINE_ROLLBACK.md 25): the port variant's stack, 64 KB of .port_bss (every variant): crt0's sp is
 * the end of portStack (build_source_only.py sets crt0's stack word and the EXE header's s_addr to it), and
 * portStackGuard is the 16 bytes right below it, which only a stack overflow writes (0 after the port zeroes
 * .port_bss; the port may fill it with a pattern of its own and check it after each step). portCameraWallWork:
 * portVisualCameraWall's pending nodes and spans (were 3.5 KB of stack). All three: snapshot and hash no. */
#define TM2_PORT_STACK 0x10000
#define TM2_PORT_STACK_GUARD 16
#define TM2_PORT_CAMERA_WORK 0x1000
extern unsigned int portStack[];
extern unsigned int portStackGuard[];
#ifdef __cplusplus
}
#endif
#endif

#ifdef TM2_PORT_ROLLBACK
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_ROLLBACK needs the port variant (build_source_only.py --port-variant)"
#endif
#pragma tm2_absolute portRollbackOptions
#pragma tm2_absolute portResimulating
#pragma tm2_absolute portStepScrub
#pragma tm2_absolute portStepScrubFill
#pragma tm2_absolute portStepHold
#pragma tm2_absolute portStepTicks
#pragma tm2_absolute portRollbackSeed
#pragma tm2_absolute portRollbackFrameCount
#pragma tm2_absolute portRollbackQuit
#pragma tm2_absolute portRollbackHolding
#pragma tm2_absolute portRollbackReseeds
#pragma tm2_absolute portRollbackVisualSeed
#pragma tm2_absolute portRollbackSplitting
#pragma tm2_absolute portRollbackEnded
#pragma tm2_absolute portClutShadow
#pragma tm2_absolute portDmdImages
#ifdef __cplusplus
extern "C" {
#endif
/* .port_text (port_variant_arrays.c) */
extern void portRollbackLoopEnter(void);       /* rtMainLoop's entry: FIXED_TICKS' ticks for the round's first step */
extern void portRollbackFrameTime(void);       /* rtMainLoop, after rtFrameTime: FIXED_TICKS paces at 60 / ticks */
extern int portRollbackTop(void);              /* the top of each iteration: latches HOLD (returns 1 = draw only);
                                                  a pending quit schedules event 2 first */
extern int portRollbackHeld(void);             /* the latch: 1 while this iteration draws only */
extern int portRollbackFixedFrame(void);       /* SetFrameStart: FIXED_TICKS' frame (returns 1: no measurement) */
extern unsigned int portRollbackSeedFor(int site);   /* SEED: the seed of a srand site, else GetCurTics() */
extern void portRollbackPreload(void);         /* sessionLoadArenaAndCars' start: SEED's load seed and frame count */
extern int portRollbackPauseOff(void);         /* pauseMenu: 1 with PAUSE_PORT (no pad read, no menu) */
/* stage 2 (.port_text: port_variant_arrays.c, portRollbackAnimStep engine_anim_hit_volumes.c) */
extern int portRollbackOncePerStep(void);      /* 1 with ONCE_PER_STEP (fxSheetStart: no split doubling) */
extern int portRollbackPass(int step_call);    /* TM2_RB_PASS_*: BOTH without ONCE_PER_STEP, else TICK for the
                                                  step's call (step_call != 0), DRAW for a view's */
extern int portRollbackViewTickOff(void);      /* streaksDraw: 1 = skip the per-view ticks (held, or ONCE_PER_STEP) */
extern int portRollbackViewDrawOff(void);      /* streaksDraw: 1 = skip the split functions (held without
                                                  ONCE_PER_STEP; with it they still draw) */
extern void portRollbackStepEffects(void);     /* rtMainLoop after view 0's streaksDraw: ONCE_PER_STEP's ticks */
extern int portRollbackAnimDrawOff(void);      /* animUpdateAll / animUpdateOne: 1 = no step (held, or ANIM_IN_STEP) */
extern void portRollbackAnimStep(void);        /* rtMainLoop after renderFlipFrame: ANIM_IN_STEP's step */
extern int portRollbackAnimVisual(void);       /* the animators: 1 = the visual stream (VISUAL_RAND without
                                                  ANIM_IN_STEP) */
extern int portRollbackArcVisual(int pass);    /* weaponArcsDraw: 1 = the jitter from the visual stream (the
                                                  drawing pass, or VISUAL_RAND) */
extern int portRollbackVisualRand(void);       /* the visual stream: retail's rand() formula on portRollbackVisualSeed */
/* stage 3 (.port_text: rt_main.c portStepBody / portRunStep / portRollbackLoopStep, the rest port_variant_arrays.c) */
extern int portStepBody(void);                 /* STEP_SPLIT: one step, no drawing, retail's order; 1 = an event
                                                  fired (the round is over: rtFiredEventValue) */
extern int portRunStep(void);                  /* the port's re-run (its rollback "advance"): portStepBody whatever
                                                  the HOLD latch; 1 = an event fired (rtMainLoop leaves at its next
                                                  top, portRollbackEnded) */
extern int portRollbackLoopStep(void);         /* rtMainLoop's top when portRollbackTop returned 1: a pending end
                                                  (1), a held iteration (0), else STEP_SPLIT's live step */
extern void portRollbackHudStep(void);         /* portStepBody, before worldUpdate: the radar swap of every HUD */
extern int portRollbackRadarInStep(void);      /* hudDisplayRadar: 1 = no swap (STEP_SPLIT: the step swaps) */
extern int portRollbackSoundOff(void);         /* SOUND_GATE: 1 while re-running (no sound, no CD track test) */
extern void portSoundReconcile(void);          /* the port, after a batch: stop the projectile sounds whose
                                                  projectile is gone */
extern void portClutLoadLine(unsigned short *buf, int x, int y, int count);   /* CLUT_SHADOW: the upload */
extern void portClutStoreLine(unsigned short *buf, int x, int y, int count);  /* CLUT_SHADOW: the read-back */
extern void portClutShadowReset(void);         /* request 59 (3): the shadow emptied (rows 0, every word 0) */
extern void portClutShadowCapture(void);       /* rtMainLoop's entry: the shadow emptied, then every definition's
                                                  row from VRAM (only the round's rows) */
extern void portClutAnimUpload(void);          /* the port, after a batch / a loaded snapshot: rows to VRAM */
extern int portDmdMutableList(unsigned int *out, int max);   /* (address, bytes) pairs of the mutable node fields
                                                  of every model file loaded for the match (request 54: the level's
                                                  every root, every portDmdImages file), sorted and merged;
                                                  negative = minus the raw pairs needed (more than max) */
extern void portDmdNoteImage(void *base, int group);   /* assetInstallImage: note the installed file (request 54) */
extern int portRollbackEndDue(void);           /* request 55 (rt_main.c): 1 = the next portStepBody ends the round
                                                  (a scheduled event expires in its rtUpdateScheduledEvents); a
                                                  peek: nothing is run, counted down or consumed */
/* section 18 (.port_text, port_variant_arrays.c) */
extern int portRollbackViewSplit(void);        /* LOCAL_VIEWS: the split mode the step reads: viewGetSplitMode()
                                                  without the bit, else retail's for the match (sessionRun's rule:
                                                  game mode 2 or 4: 1 on levels 2, 3 and 8, else 2; other modes 0) */
/* request 60 (.port_text, port_variant_arrays.c) */
extern void portRoundCleanProjectiles(void);   /* ROUND_CLEAN: roundStart, before the round's projectilesReset: every
                                                  projectile record cleared (nothing without the bit) */
extern int portSeekerTargetOk(void *target);   /* weaponProjectileUpdate: 1 = steer (the bit clear, or target is
                                                  the position (+0xA8) of a counted human or AI car's object) */
#ifdef __cplusplus
}
#endif
/* the hooks (each expands to retail's text without the define) */
#define RB_SEED(site) portRollbackSeedFor(site)
#define RB_EVENTS() (portRollbackTop() ? portRollbackLoopStep() : rtUpdateScheduledEvents())
#define RB_STEP(call) ((void)(portRollbackHeld() || ((call), 0)))
#define RB_HELD() portRollbackHeld()
/* stage 2: streaksDraw's per-view ticks (skipped when held or with ONCE_PER_STEP) and its two split functions
   (skipped when held without ONCE_PER_STEP; with it they only draw); the animators' rand() */
#define RB_VIEW_TICK(call) ((void)(portRollbackViewTickOff() || ((call), 0)))
#define RB_VIEW_DRAW(call) ((void)(portRollbackViewDrawOff() || ((call), 0)))
#define RB_ANIM_RAND() (portRollbackAnimVisual() ? portRollbackVisualRand() : rand())
/* stage 3: the sound wrappers' calls (0 while re-running with SOUND_GATE); the palette rows (CLUT_SHADOW) */
#define RB_SOUND(call) (portRollbackSoundOff() ? 0 : (call))
#define RB_CLUT_LOAD(buf, x, y, n) portClutLoadLine(buf, x, y, n)
#define RB_CLUT_STORE(buf, x, y, n) portClutStoreLine(buf, x, y, n)
#else
#define RB_SEED(site) GetCurTics()
#define RB_EVENTS() rtUpdateScheduledEvents()
#define RB_STEP(call) call
#define RB_HELD() 0
#define RB_VIEW_TICK(call) call
#define RB_VIEW_DRAW(call) call
#define RB_ANIM_RAND() rand()
#define RB_SOUND(call) (call)
#define RB_CLUT_LOAD(buf, x, y, n) gpuLoadImageLine(buf, x, y, n)
#define RB_CLUT_STORE(buf, x, y, n) gpuStoreImageLine(buf, x, y, n)
#endif

#endif
