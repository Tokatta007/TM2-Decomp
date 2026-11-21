/* Motion commands (move/rotate a target by a per-tick delta for a duration).
 * C (PsyQ 3.6 CC1PSX, -fvolatile). FINISH is a macro: as an inline function
 * the motion-list address would be rematerialized instead of kept in s0. */
struct TM2MotionTarget;
struct TM2MotionCommand {
    int duration;
    int remaining;
    char pad8[4];
    struct TM2MotionTarget *target;
    int dx, dy, dz;
    int da, db, dc;
};

extern unsigned char shellScene[];
extern int sceneObjGetPosition(struct TM2MotionTarget *, int *, int *, int *);
extern int sceneObjGetRotation(struct TM2MotionTarget *, int *, int *, int *);
/* sceneObjSetPosition/sceneObjSetRotation is void (v0 at its return is left over from its last call), but retail
 * returns their result here, so this unit keeps the int-returning view it was compiled with. */
extern int sceneObjSetPosition(struct TM2MotionTarget *, int, int, int);
extern int sceneObjSetRotation(struct TM2MotionTarget *, int, int, int);
extern int scenePostEvent(void *, int, void *);
extern int sceneStopCommand(void *, void *);

/* Common tail of the timed commands: when the countdown has expired, remove
 * the command from the motion list and restart it; otherwise count down. The
 * returned value is the counter as read for the test (or after the
 * decrement); with -fvolatile every access re-reads the command. */
#define FINISH(c)                                               \
    if ((c)->remaining == 0) {                                  \
        scenePostEvent(shellScene, 2, (c));                       \
        sceneStopCommand(shellScene, (c));                          \
        t = (c)->duration;                                      \
        (c)->remaining = t;                                     \
        return t;                                               \
    }                                                           \
    if ((t = (c)->remaining) > 0)                               \
        t = --(c)->remaining;                                   \
    return t

int __attribute__((section(".text_80122C80"))) sceneCmdMoveToRun(struct TM2MotionCommand *c)
{ return sceneObjSetPosition(c->target, c->dx, c->dy, c->dz); }

int __attribute__((section(".text_80122CB4"))) sceneCmdTurnToRun(struct TM2MotionCommand *c)
{ return sceneObjSetRotation(c->target, c->dx, c->dy, c->dz); }

int __attribute__((section(".text_80122CE8"))) sceneCmdMoveByRun(struct TM2MotionCommand *c)
{
    int x, y, z, t;
    if (c->remaining) {
        sceneObjGetPosition(c->target, &x, &y, &z);
        x += c->dx; y += c->dy; z += c->dz;
        sceneObjSetPosition(c->target, x, y, z);
    }
    FINISH(c);
}

int __attribute__((section(".text_80122DD8"))) sceneCmdTurnByRun(struct TM2MotionCommand *c)
{
    int x, y, z, t;
    if (c->remaining) {
        sceneObjGetRotation(c->target, &x, &y, &z);
        x += c->dx; y += c->dy; z += c->dz;
        sceneObjSetRotation(c->target, x % 4096, y % 4096, z % 4096);
    }
    FINISH(c);
}

int __attribute__((section(".text_80122F10"))) sceneCmdMoveTurnByRun(struct TM2MotionCommand *c)
{
    int x, y, z, a, b, d, t;
    if (c->remaining) {
        sceneObjGetPosition(c->target, &x, &y, &z);
        sceneObjGetRotation(c->target, &a, &b, &d);
        x += c->dx; y += c->dy; z += c->dz;
        a += c->da; b += c->db; d += c->dc;
        sceneObjSetPosition(c->target, x, y, z);
        sceneObjSetRotation(c->target, a, b, d);
    }
    FINISH(c);
}
