/* Controls: action bindings, key repeat and the per-frame pad poll
 * (80150B28..80151374). See shared_pad.c for the decoded pad words.
 *
 * Each port has 128 action slots. padConfiguredBindings holds the configured buttons
 * (defaults from padDefaultBindings); padActiveBindings is the active copy the game reads.
 * A slot's value is a button number for padButtonHeld (1..14). */

typedef unsigned char u8;
typedef unsigned int u32;

typedef struct { u8 button[128]; } TM2Bindings;
typedef struct { u8 action, button; } TM2DefaultBinding;

extern TM2Bindings padActiveBindings[2];     /* active bindings, per port */
extern TM2Bindings padConfiguredBindings[2];     /* configured bindings, per port */
extern u8 padActionCache[128][2];         /* last lookup result, [action][port] */
#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern void *renderWorldModels[TM2_MAX_WORLD_MODELS];          /* registered per-frame callbacks */
extern TM2DefaultBinding padDefaultBindings[22];

extern int hdpEntryStack;
extern int hdpShift;
extern int padDriverStatus;
extern u8 padRepeatMode;                 /* 1: no key repeat, latch every frame */
extern int padRepeatDelay;                /* first repeat delay (tics) */
extern int padRepeatInterval;                /* repeat interval (tics) */
extern int padRepeatHeldP1, padRepeatHeldP2;    /* port 0/1 held */
extern int padRepeatWaitP1, padRepeatWaitP2;    /* port 0/1 current delay */
extern int padRepeatLastTicP1, padRepeatNowTicP1, padRepeatElapsedP1; /* port 0 last, now, elapsed */
extern int padRepeatLastTicP2, padRepeatNowTicP2, padRepeatElapsedP2; /* port 1 last, now, elapsed */

extern void *memset(void *, int, unsigned int);
#include "semantic/world.h"
extern void padStart(void);
extern void padStop(void);
extern void padClear(void);
extern void padDecode(void);
extern void padLatchPort(short port);
extern void padLatchAll(void);
extern u8 padButtonHeld(short int button, short int port);
#include "semantic/input.h"
extern u8 padAnyDown(int port);
extern void padLoadLayout(short port);

int padInitDriver(void);
void padLoadDefaultBindings(void);
void padClearActionCache(short port);
void padClearActiveBindings(short port);
void padBindAction(u8 action, u8 button, short port);
void padActivateBindings(short port);
u8 padGetRepeatMode(void);
u8 padGetActiveBinding(u8 action, short port);
void padClearLatched(void);
u8 padActionHeld(u8 action, short port);
void padLatchWithRepeat(void);

/* @80150B28 */
void __attribute__((section(".text_80150B28"))) hdpSetStackBuffer(int value)
{
    hdpEntryStack = value;
}

/* @80150B34 */
void __attribute__((section(".text_80150B34"))) hdpSetShift(int value)
{
    hdpShift = value;
}

/* @80150B40: reset the controls of both ports to the defaults. */
int __attribute__((section(".text_80150B40"))) padInit(void)
{
    short port;
    padDriverStatus = padInitDriver();
    padLoadDefaultBindings();
    for (port = 0; port < 2; port++) {
        padLoadLayout(port);
        padClearActiveBindings(port);
        padActivateBindings(port);
    }
    return padDriverStatus;
}

/* @80150BB8 */
void __attribute__((section(".text_80150BB8"))) controlsPadStop(void)
{
    padStop();
}

/* @80150BD8: load the default bindings into both ports' configuration. */
void __attribute__((section(".text_80150BD8"))) padLoadDefaultBindings(void)
{
    int i;
    for (i = 0; i < 22; i++) {
        padBindAction(padDefaultBindings[i].action, padDefaultBindings[i].button, 0);
        padBindAction(padDefaultBindings[i].action, padDefaultBindings[i].button, 1);
    }
}

/* @80150C50: set the key-repeat mode (0 repeat, 1 latch every frame);
 * other values are ignored. (`== 0 || == 1` matches; `< 2` does not.) */
void __attribute__((section(".text_80150C50"))) padSetRepeatMode(u8 mode)
{
    if (mode == 0 || mode == 1)
        padRepeatMode = mode;
}

/* @80150C70 */
u8 __attribute__((section(".text_80150C70"))) padGetRepeatMode(void)
{
    return padRepeatMode;
}

/* @80150C7C */
void __attribute__((section(".text_80150C7C"))) padFlush(void)
{
    padClearActionCache(0);
    padClearLatched();
}

/* @80150CA4 */
void __attribute__((section(".text_80150CA4"))) padClearActionCache(short port)
{
    int action;
    for (action = 0; action < 128; action++)
        padActionCache[action][port] = 0;
}

/* @80150CD8 */
u8 __attribute__((section(".text_80150CD8"))) padAnyPressed(short port)
{
    return padAnyLatched(port);
}

/* @80150CFC: is an action's button held? Action 6 means any button.
 * Ports 0..2 query the pad (and cache the result per port); the cached
 * branch for port 1 is unreachable, kept as retail has it. */
u8 __attribute__((section(".text_80150CFC"))) padCheckAction(action, port)
u8 action; unsigned short port;   /* old-style: callers pass ints, the body narrows */
{
    u8 held = 0;
    if (port == 0 || port == 1 || (short)port == 2) {
        if (action == 6)
            held = padAnyLatched((short)port);
        else {
            held = padActionHeld(action, (short)port);
            padActionCache[action][(short)port] = held;
        }
    } else if (port == 1)
        held = padActionCache[action][(short)port];
    return held;
}

/* @80150E00: per-frame input update. */
void __attribute__((section(".text_80150E00"))) padUpdate(void)
{
    if (!padGetRepeatMode())
        padClearLatched();
    padLatchWithRepeat();
}

/* @80150E3C */
int __attribute__((section(".text_80150E3C"))) padInitDriver(void)
{
    memset(padActiveBindings, 0, sizeof padActiveBindings);
    memset(padConfiguredBindings, 0, sizeof padConfiguredBindings);
    padStart();
    return 1;
}

/* @80150EAC: button bound to an action (active bindings). */
u8 __attribute__((section(".text_80150EAC"))) padGetActiveBinding(u8 action, short port)
{
    return (padActiveBindings + port)->button[action];
}

/* @80150ED4 */
void __attribute__((section(".text_80150ED4"))) padClearActiveBindings(short port)
{
    memset(&padActiveBindings[port], 0, sizeof (TM2Bindings));
}

/* @80150F0C */
void __attribute__((section(".text_80150F0C"))) padClearConfiguredBindings(short port)
{
    memset(&padConfiguredBindings[port], 0, sizeof (TM2Bindings));
}

/* @80150F44 */
int __attribute__((section(".text_80150F44"))) padActionIsBound(u8 action, short port)
{
    return (signed char)padGetActiveBinding(action, port) != 0;
}

/* @80150F70: bind an action to a button in the configured bindings. */
void __attribute__((section(".text_80150F70"))) padBindAction(u8 action, u8 button, short port)
{
    (padConfiguredBindings + port)->button[action] = button;
}

/* @80150FC0: make a port's configured bindings active (inline copy). */
void __attribute__((section(".text_80150FC0"))) padActivateBindings(short port)
{
    padClearActiveBindings(port);
    __builtin_memcpy(&padActiveBindings[port], &padConfiguredBindings[port], sizeof (TM2Bindings));
}

/* @8015109C */
void __attribute__((section(".text_8015109C"))) padClearLatched(void)
{
    padClear();
}

/* @801510BC */
u8 __attribute__((section(".text_801510BC"))) padAnyPressedEitherPort(void)
{
    return padAnyLatched(2);
}

/* @801510DC: is the button bound to an action held on this port? */
u8 __attribute__((section(".text_801510DC"))) padActionHeld(u8 action, short port)
{
    return padButtonHeld((padActiveBindings + port)->button[action], port);
}

/* @80151138: decode the pads and latch new presses. In mode 1 every frame
 * is latched; otherwise a held port latches once, again after padRepeatDelay
 * tics and then every padRepeatInterval tics. */
void __attribute__((section(".text_80151138"))) padLatchWithRepeat(void)
{
    if (padGetRepeatMode() == 1) {
        padDecode();
        padLatchAll();
        padRepeatHeldP1 = 0;
        padRepeatHeldP2 = 0;
        return;
    }
    padDecode();
    if (padAnyDown(0)) {
        if (padRepeatHeldP1) {
            padRepeatNowTicP1 = GetCurTics();
            padRepeatElapsedP1 = padRepeatNowTicP1 - padRepeatLastTicP1;
            if (padRepeatWaitP1 < padRepeatElapsedP1) {
                padLatchPort(0);
                padRepeatWaitP1 = padRepeatInterval;
                padRepeatLastTicP1 = GetCurTics();
            }
        } else {
            padLatchPort(0);
            padRepeatHeldP1 = 1;
            padRepeatLastTicP1 = GetCurTics();
            padRepeatElapsedP1 = 0;
            padRepeatWaitP1 = padRepeatDelay;
        }
    } else {
        padRepeatHeldP1 = 0;
        padRepeatElapsedP1 = 0;
        padRepeatWaitP1 = padRepeatDelay;
    }
    if (padAnyDown(1)) {
        if (padRepeatHeldP2) {
            padRepeatNowTicP2 = GetCurTics();
            padRepeatElapsedP2 = padRepeatNowTicP2 - padRepeatLastTicP2;
            if (padRepeatWaitP2 < padRepeatElapsedP2) {
                padLatchPort(1);
                padRepeatWaitP2 = padRepeatInterval;
                padRepeatLastTicP2 = GetCurTics();
            }
        } else {
            padLatchPort(1);
            padRepeatHeldP2 = 1;
            padRepeatLastTicP2 = GetCurTics();
            padRepeatElapsedP2 = 0;
            padRepeatWaitP2 = padRepeatDelay;
        }
    } else {
        padRepeatHeldP2 = 0;
        padRepeatElapsedP2 = 0;
        padRepeatWaitP2 = padRepeatDelay;
    }
}

/* @801512F8: register a per-frame callback in the first free slot. */
void __attribute__((section(".text_801512F8"))) renderAddWorldModel(void *callback)
{
    int i;
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++) {
        if (!renderWorldModels[i]) {
            renderWorldModels[i] = callback;
            return;
        }
    }
}

/* @80151338: unregister a callback. */
void __attribute__((section(".text_80151338"))) renderRemoveWorldModel(void *callback)
{
    int i;
    for (i = 0; i < TM2_MAX_WORLD_MODELS; i++) {
        if (renderWorldModels[i] == callback) {
            renderWorldModels[i] = 0;
            return;
        }
    }
}
