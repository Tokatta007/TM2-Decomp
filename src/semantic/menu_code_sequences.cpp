/* Movie input sequences: a command object that watches three button
 * sequences and fires an event when one is entered in full.
 * C++ (PsyQ 3.6 CC1PLPSX, -fvolatile).
 *
 * TM2MovieSequence derives from TM2SetValue (object_control_helpers):
 * duration (+0), remaining (+4), vtable (+8), target (+12), value (+16),
 * then the event (+20), three sequences of {values, position} (+24),
 * the last completed sequence (+48), the idle timer (+52) and its reset
 * value (+56). */
typedef signed int s32;

struct TM2Target;
struct TM2Command { int duration; int remaining; virtual int run(); TM2Command() {} };
struct TM2TargetCommand : TM2Command { TM2Target *target; virtual int run(); TM2TargetCommand() {} };
struct TM2SetValue : TM2TargetCommand { int value; virtual int run(); TM2SetValue(TM2Target *t, int v, int frames); };

struct TM2Sequence {
    s32 *values;
    unsigned char position;
};

struct TM2MovieSequence : TM2SetValue {
    int event;
    TM2Sequence sequences[3];
    signed char last;
    int timer;
    int timeout;
    virtual int run();
    TM2MovieSequence(TM2Target *t, int v, s32 *start, int frames);
    int match(int value);
};

#pragma tm2_symbol _vt.16TM2MovieSequence D_800D029C
#pragma tm2_symbol _vt.10TM2Command D_800CF4B4
#pragma tm2_symbol _vt.16TM2TargetCommand D_800CF484
#pragma tm2_symbol __11TM2SetValueP9TM2Targetii sub_80123828

/* @8012DB10 */
#pragma tm2_symbol __16TM2MovieSequenceP9TM2TargetiPii sub_8012DB10
TM2MovieSequence::TM2MovieSequence(TM2Target *t, int v, s32 *start, int frames) : TM2SetValue(t, v, frames)
{
    int i;
    for (i = 0; i < 3; i++) {
        sequences[i].values = start + 4 * i;
        sequences[i].position = 0;
    }
    last = -1;
    timeout = 60;
    timer = timeout;
}

extern "C" {
s32 scenePostEvent(TM2Target *, s32, s32);
s32 sceneGetSelectedItem2(TM2Target *);
int menuCodeDetectorRun(unsigned char *object);
/* g++ 2.x vtable: entry 0 empty, entry 1 = run(). */
struct TM2VtableEntry { short delta, index; void (*function)(void); };
TM2VtableEntry D_800D029C[2] __attribute__((section(".tm2_800D029C"))) = {{0, 0, 0}, {0, 0, (void (*)(void))menuCodeDetectorRun}};
}

/* @8012DB94: advance each sequence on a matching value; a completed
 * sequence fires event 17 + index. Returns the idle timer while it runs;
 * when it has run out every sequence is reset. */
#pragma tm2_symbol match__16TM2MovieSequencei sub_8012DB94
int TM2MovieSequence::match(int value)
{
    int i, j, t;
    TM2Sequence *s;
    for (i = 0; i < 3; i++) {
        s = &sequences[i];
        if (s->values[s->position] == value) {
            s->position++;
            timer = timeout;
        } else
            s->position = 0;
        if (s->position == 4) {
            scenePostEvent(target, i + 17, sceneGetSelectedItem2(target));
            last = i;
            s->position = 0;
        }
    }
    if ((t = timer) > 0)
        return t;
    for (j = 0; j < 3; j++)
        sequences[j].position = 0;
}
