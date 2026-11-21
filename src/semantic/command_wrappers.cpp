/* Command wrappers around the global command target. C++ (PsyQ 3.6 CC1PLPSX);
 * C linkage. The target's vtable pointer is at +200 (g++ 2.x: after the
 * fields of the class); command is virtual entry 1, query entry 2. */
typedef signed int s32;

struct TM2Target {
    char fields[200];
    virtual s32 command(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);
    virtual s32 query();
};

#ifdef TM2_PORT_ROLLBACK
/* request 48 stage 3 SOUND_GATE (ONLINE_ROLLBACK.md 1.2, 12): while the port re-runs steps (portResimulating) every
 * wrapper returns 0 at once (RB_SOUND); every caller in the C discards the result (ONLINE_ROLLBACK.md 12).
 * Without the define RB_SOUND(call) is (call): retail's text. */
#include "semantic/port_rollback.h"
#else
#define RB_SOUND(call) (call)
#endif

extern "C" {
extern TM2Target *soundManagerObject;
/* The soundMgr* functions are void (sound_manager_play.c, sound_manager_reap.c), but
 * retail compiles these wrappers as `return soundMgrX(...)`: GCC turns a void call in
 * tail position into a jump, which retail does not have. So this unit keeps an
 * int-returning view of them. */
s32 soundMgrPlaySided(s32, s32, s32, s32, s32, s32);
s32 soundMgrStopSided(s32, s32, s32);
s32 soundMgrPlayGeneric(s32, s32, s32, s32, s32, s32);
s32 soundMgrStopGeneric(s32, s32, s32);
s32 soundMgrStop(s32, s32, s32);

s32 __attribute__((section(".text_800D3244"))) soundStartPlayerLoop(s32 a, s32 b, s32 c, s32 d, s32 e) { return RB_SOUND(soundMgrPlaySided((s32)soundManagerObject, a, b, c, d, e)); }
s32 __attribute__((section(".text_800D328C"))) soundStopPlayerLoop(s32 a, s32 b) { return RB_SOUND(soundMgrStopSided((s32)soundManagerObject, a, b)); }
s32 __attribute__((section(".text_800D32BC"))) soundStartSharedLoop(s32 a, s32 b, s32 c, s32 d, s32 e) { return RB_SOUND(soundMgrPlayGeneric((s32)soundManagerObject, a, b, c, d, e)); }
s32 __attribute__((section(".text_800D3304"))) soundStopSharedLoop(s32 a, s32 b) { return RB_SOUND(soundMgrStopGeneric((s32)soundManagerObject, a, b)); }
s32 __attribute__((section(".text_800D3334"))) soundPlay(s32 a, s32 b, s32 c, s32 d) { return RB_SOUND(soundManagerObject->command(a, b, c, d, -1, 0)); }
s32 __attribute__((section(".text_800D3398"))) soundPlayVariant(s32 a, s32 b, s32 c, s32 d, s32 e) { return RB_SOUND(soundManagerObject->command(a, b, c, d, e, 0)); }
s32 __attribute__((section(".text_800D33FC"))) soundPlayVariantEx(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f) { return RB_SOUND(soundManagerObject->command(a, b, c, d, e, f)); }
s32 __attribute__((section(".text_800D3464"))) soundStop(s32 a) { return RB_SOUND(soundMgrStop((s32)soundManagerObject, a, -1)); }
s32 __attribute__((section(".text_800D3490"))) soundStopInstance(s32 a, s32 b) { return RB_SOUND(soundMgrStop((s32)soundManagerObject, a, b)); }
s32 __attribute__((section(".text_800D34E8"))) soundFlush(void) { return RB_SOUND(soundManagerObject->query()); }
}
