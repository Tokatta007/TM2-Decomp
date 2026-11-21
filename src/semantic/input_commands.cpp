/* Input commands: read the pad and post actions to the command list. C++
 * (PsyQ 3.6 CC1PLPSX, -fvolatile). Same command class layout as
 * object_control_helpers.cpp (duration +0, remaining +4, vtable +8). */
typedef signed short s16;
typedef unsigned char u8;
typedef signed int s32;

struct TM2Command {                     /* vtable 800CF4B4 */
    int duration;
    int remaining;
    virtual int run();
    TM2Command() {}
};

struct TM2TargetCommand : TM2Command {  /* vtable 800CF484 */
    void *list;
    virtual int run();
    TM2TargetCommand() {}
};

struct TM2InputCommand : TM2TargetCommand {   /* vtable 800CF384 */
    s32 port;
    s32 action;
    virtual int run();
    TM2InputCommand(void *store, s32 pad, s32 timer);
};

#pragma tm2_symbol _vt.10TM2Command D_800CF4B4
#pragma tm2_symbol _vt.16TM2TargetCommand D_800CF484
#pragma tm2_symbol _vt.15TM2InputCommand D_800CF384
#pragma tm2_symbol __15TM2InputCommandPvii sub_801237D4
/* @801237D4 */
TM2InputCommand::TM2InputCommand(void *store, s32 pad, s32 timer)
{
    list = store;
    port = pad;
    remaining = timer;
    duration = remaining;
}

extern "C" {
#include "semantic/input.h"
void *sceneGetSelectedItem(void *);
s32 scenePostEvent(void *, s32, void *);
s32 sceneStopCommand(void *, void *);

/* Common tail of the timed commands (see motion_commands.c). */
#define FINISH(c)                                               \
    if ((c)->remaining == 0) {                                  \
        scenePostEvent((c)->list, 2, (c));                        \
        sceneStopCommand((c)->list, (c));                           \
        t = (c)->duration;                                      \
        (c)->remaining = t;                                     \
        return t;                                               \
    }                                                           \
    if ((t = (c)->remaining) > 0)                               \
        t = --(c)->remaining;                                   \
    return t

/* @80123430: map the pressed button to an action and post it. */
s32 __attribute__((section(".text_80123430"))) sceneCmdPadInputRun(TM2InputCommand *c)
{
    s32 t;
    padUpdate();
    c->action = -1;
    if ((u8)padCheckAction(6, (s16)c->port)) c->remaining = c->duration;
    if((u8)padCheckAction(3,(s16)c->port))c->action=10;else if((u8)padCheckAction(2,(s16)c->port))c->action=9;else if((u8)padCheckAction(0,(s16)c->port))c->action=7;else if((u8)padCheckAction(1,(s16)c->port))c->action=8;else if((u8)padCheckAction(8,(s16)c->port))c->action=3;else if((u8)padCheckAction(10,(s16)c->port))c->action=14;else if((u8)padCheckAction(11,2*(c->port==1)))c->action=16;else if((u8)padCheckAction(12,2*(c->port==1)))c->action=15;else if((u8)padCheckAction(13,(s16)c->port))c->action=13;else if((u8)padCheckAction(14,(s16)c->port))c->action=15;else if((u8)padCheckAction(15,(s16)c->port))c->action=16;else if((u8)padCheckAction(20,(s16)c->port))c->action=11;else if((u8)padCheckAction(21,(s16)c->port))c->action=5;else if((u8)padCheckAction(17,(s16)c->port))c->action=12;else if((u8)padCheckAction(16,(s16)c->port))c->action=6;
    if (c->action != -1) scenePostEvent(c->list, c->action, sceneGetSelectedItem(c->list));
    FINISH(c);
}

/* @80123708: post action 3 when the button is pressed; otherwise count down. */
s32 __attribute__((section(".text_80123708"))) sceneCmdPressAnyRun(TM2InputCommand *c)
{
    s32 t;
    padUpdate();
    if ((u8)padAnyPressed((s16)c->port)) {
        scenePostEvent(c->list, 3, sceneGetSelectedItem(c->list));
        sceneStopCommand(c->list, c);
        t = c->duration;
        c->remaining = t;
        return t;
    }
    FINISH(c);
}
}
