/* Per-car engine sound for the PC port (request 33 step 3, docs/CAR_GROUPS.md 12, docs/PER_CAR_PLAN.md C.3
 * with the port's answer G.7 and the user's change of 2026-10-13: no cap by default).
 *
 * Retail plays a car's engine by sound id, one id per car type (carSoundUpdate: soundPlay(snd, ...) = instance
 * 0 every update while the car is in range, else soundIsActive(snd) -> soundStop(snd); each id's request list
 * holds one request), so two cars of one type are one voice whose pan and pitch jump between them, and one of
 * them stopping stops the other. Thumper's flame sound (47) and Dark Tooth's swoop sounds (61, 66) are one id
 * each the same way.
 *
 * With -DTM2_PORT_CAR_SOUND (needs TM2_PORT_CAR_GROUPS) and portCarOptions bit 4 set, every car plays these
 * sounds as its own instance of the id (instance = car slot + 1), the lists hold more requests, a request is
 * merged into its own instance's request before a new one is inserted (portSoundPlayInstance), engine voices
 * rank below every other sound when the voices run out (soundVoiceLimit: 24, 44 with TM2_PORT_SPU48; soundSortInstances), and with
 * portEngineVoices = N > 0 only the engines of the N cars nearest to any human (with TM2_PORT_HUMANS: to a local
 * human, HUMANS16_PLAN.md H3) play. Bit 4 clear: the retail
 * calls, word for word.
 *
 * The storage is defined in every variant (port_variant_arrays.c, .port_bss); this header declares it for the
 * port variant and, with the define, the port functions. Plain C types only (included by engine units that
 * have their own typedefs). */
#ifndef TM2_SEMANTIC_CAR_SOUND_H
#define TM2_SEMANTIC_CAR_SOUND_H

#define TM2_PORT_CAR_OPT_ENGINE_SOUND 0x10   /* portCarOptions bit 4: per-car sound instances, read live */
#define TM2_PORT_SOUND_SLOTS 64              /* car slots (= car_groups.h TM2_CAR_SLOT_CAP); instance = slot + 1 */
#define TM2_PORT_SWOOP_VOICES 4              /* requests the swoop lists (61, 66) hold with bit 4 (TM2_PORT_DARK_TOOTH) */
#define TM2_PORT_SOUND_POOL_RESERVE 12       /* sound request objects (35, sndObjPool) a new engine instance leaves
                                                free for the other sounds */

#ifdef TM2_PORT_VARIANT
/* portCarSound (.port_bss, config/US/port_variant_arrays.tsv group one), written by the C; the port may read
 * it. Offsets in bytes. */
struct TM2PortCarSound {
    int live;                                       /* +0x00 bit 4 as applied by the last worldUpdate */
    int voices;                                     /* +0x04 the engine cap in use: portEngineVoices, 0 = none */
    int candidates;                                 /* +0x08 cars with an engine to play at the last update */
    int allowed;                                    /* +0x0C of them allowed to play (all without a cap) */
    unsigned char engine[TM2_PORT_SOUND_SLOTS];     /* +0x10 sound id slot s plays as instance s + 1, 0 = none */
    unsigned char allow[TM2_PORT_SOUND_SLOTS];      /* +0x50 1 = among the nearest `voices` (or no cap) */
    unsigned char flame[TM2_PORT_SOUND_SLOTS];      /* +0x90 1 = the slot's flame sound (47) instance started */
};
typedef char tm2_port_car_sound_size[(sizeof(struct TM2PortCarSound) == 0xD0) ? 1 : -1];
#endif

/* Request 33 step 4 (CAR_GROUPS.md 16): the human loops and Twister's tornado sound per car.
 * Retail plays a human's turbo (sound 0), gun (9) and burn/lava loop (32) in one of two fixed lists by the
 * human's index (soundStartPlayerLoop -> soundMgrPlaySided, side 0 or 1, flags soundTurboLoopActiveP1/P2 etc.):
 * a human with index 2 or more gets a request from the pool that is neither listed nor freed (the pool of 35
 * runs dry in about half a second of turbo or guns, FOUR_HUMANS.md 3). Twister's tornado (0x35) is one id: a
 * Twister dying stops every Twister's tornado sound. With -DTM2_PORT_CAR_SOUND humans 2.. play these loops as
 * instance slot + 1 in the ids' own lists (soundMgrPlay; lists 0, 9 and 32 are not used by retail), whatever bit 4
 * says (retail has no path for them); humans 0 and 1 keep retail's lists. With bit 4 the tornado sound is the
 * Twister's own instance (slot + 1), stopped per car. Per slot in portCarSoundLoops (.port_bss, every variant): */
#define TM2_PORT_LOOP_TURBO 0x01            /* the slot's turbo loop (sound 0) started as its instance */
#define TM2_PORT_LOOP_GUN 0x02              /* gun loop (sound 9) */
#define TM2_PORT_LOOP_BURN 0x04             /* burn / lava loop (sound 32) */
#define TM2_PORT_LOOP_TORNADO 0x08          /* Twister's tornado sound (0x35, bit 4) */
#define TM2_PORT_LOOP_SHARED_GUN 0x10       /* HUMANS16_PLAN.md H3: the slot's remote human fires in the computer
                                               cars' shared gun loop (instance TM2_PORT_SHARED_GUN(slot)) */
#define TM2_PORT_SPECIAL_VOICES 4           /* requests each special-fire sound's list (0x27..0x3D odd, the tornado
                                               0x35 among them) holds at least with bit 4 (retail 1..3) */

#ifdef TM2_PORT_CAR_SOUND
#ifndef TM2_PORT_CAR_GROUPS
#error "TM2_PORT_CAR_SOUND needs TM2_PORT_CAR_GROUPS (the sound instances are per car slot)"
#endif
#pragma tm2_absolute portCarSound
#pragma tm2_absolute portEngineVoices
#ifdef __cplusplus
extern "C" {
#endif
extern struct TM2PortCarSound portCarSound;
extern int portEngineVoices;     /* the port's cap: 0 = every car's engine plays (default); N > 0 = the engines
                                    of the N cars nearest to any human (a local one with TM2_PORT_HUMANS) */
/* .port_text (port_variant_arrays.c) */
extern void portCarSoundFrame(void);            /* worldUpdate, first: latch bit 4, list sizes, the nearest N */
extern void portCarEngineUpdate(int slot, int snd, int on, int vol, int pan, int pitch);
                                                /* carSoundUpdate (on = !state.dead) and worldUpdate's
                                                   out-of-range stop (on = 0): bit 4 clear = retail's calls */
extern void portCarSoundPlay(int snd, int vol, int pan, int pitch, int slot);
                                                /* soundPlay(snd, ...) as the slot's own instance (bit 4) */
extern void portCarFlameSound(int slot, int vol, int pan);   /* weaponFlameStart: sound 47 */
extern void portCarFlameSoundEnd(void *model);  /* weaponFlameUpdate: an emitter of this car object ended */
extern void portCarFlameSoundIdle(int active);  /* weaponFlameUpdate's end: active emitters (retail's stop) */
extern void portCarSoundStopSlot(int slot);     /* the port's API / swap: stop the slot's engine and flame */
extern int  portCarSoundSortInstances(void **a, int lo, int hi);
                                                /* soundSortInstances with bit 4: engines last; 0 = not live */
/* request 33-4 */
#pragma tm2_absolute portCarSoundLoops
extern unsigned char portCarSoundLoops[];       /* per car slot: TM2_PORT_LOOP_* started as its instance */
extern void portHumanLoopPlay(int snd, int vol, int pan, int pitch, int player, int slot);
                                                /* soundStartPlayerLoop(snd, vol, pan, pitch, player); players
                                                   2..: instance slot + 1 of snd (0, 9 or 32) */
extern void portHumanLoopStop(int snd, int player, int slot);
                                                /* retail's "if (player == n && loop flag) soundStopPlayerLoop";
                                                   players 2..: the slot's instance */
extern void portCarTornadoSound(int slot, int vol, int pan);   /* weaponFireSpecial: soundPlay(0x35, ...) */
extern void portCarTornadoSoundStop(int slot);  /* a Twister's tornado ended or it died: soundStop(0x35) */
#ifdef __cplusplus
}
#endif
#endif

#endif
