/* Menu object leaf functions. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). */
typedef signed int s32;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
#include "semantic/scene.h"
extern "C" void sceneObjSetVisible(unsigned char *p, unsigned short v);
extern "C" {
extern u32 sceneObjVtbl[4];

void __attribute__((section(".text_80137C60"))) menuHideTargetRun(u8 *object)
{
    sceneObjSetVisible(*(void **)(object + 12), 0);
}

void __attribute__((section(".text_80137CCC"))) menuShowTargetRun(u8 *object)
{
    sceneObjSetVisible(*(void **)(object + 12), 1);
}

void __attribute__((section(".text_80137E2C"))) sceneClearEvents(struct TM2ShellScene *scene)
{
    scene->events = 0;
}

void __attribute__((section(".text_80137E38"))) sceneClearRunning(struct TM2ShellScene *scene)
{
    scene->running = 0;
}

s32 __attribute__((section(".text_80137E44"))) sceneGetBackground2(struct TM2ShellScene *scene)
{
    return scene->background;
}

void __attribute__((section(".text_80137E50"))) sceneSetBackground(struct TM2ShellScene *scene, s32 value)
{
    scene->background = value;
}

s32 __attribute__((section(".text_80137E5C"))) sceneGetSelectedItem2(struct TM2ShellScene *scene)
{
    return scene->items[scene->selection];
}

/* @80137EC8: advance the cyclic index (selection) modulo the count (item_count). */
void __attribute__((section(".text_80137EC8"))) sceneSelectNext(struct TM2ShellScene *scene)
{
    scene->selection = (scene->selection + 1) % scene->item_count;
}

void __attribute__((section(".text_80137F10"))) sceneSetSelection(struct TM2ShellScene *scene, s32 value)
{
    scene->selection = value;
}

s32 __attribute__((section(".text_80137F1C"))) sceneGetExitCode(struct TM2ShellScene *scene)
{
    return scene->exit_code;
}

void __attribute__((section(".text_80137F28"))) sceneSetDone(struct TM2ShellScene *scene, s32 value)
{
    scene->done = value;
}

u16 __attribute__((section(".text_80137F34")))
sceneSetCamera(struct TM2ShellScene *scene, s32 first, s32 second, s32 third,
             u16 fourth, u16 fifth, u16 sixth)
{
    scene->cam_pos[0] = first;
    scene->cam_pos[1] = second;
    scene->cam_pos[2] = third;
    scene->cam_rot[0] = fourth;
    scene->cam_rot[1] = fifth;
    scene->cam_rot[2] = sixth;
    return fourth;
}

}

/* @80137F60: constructor: base constructor (@80123E98), then the vtable pointer
 * (+48) and the start value (+52). A C++ constructor; its compiled symbol is
 * emitted under the retail name. */
struct TM2MenuBase {
    int fields[12];
    TM2MenuBase(unsigned short, unsigned short);
};

struct TM2MenuCycle : TM2MenuBase {
    unsigned int *vtable;
    int value;
    TM2MenuCycle(unsigned short first, unsigned short second, int start);
};

#pragma tm2_symbol __11TM2MenuBaseUsUs sub_80123E98
#pragma tm2_symbol __12TM2MenuCycleUsUsi sub_80137F60
TM2MenuCycle::TM2MenuCycle(unsigned short first, unsigned short second, int start) : TM2MenuBase(first, second)
{
    vtable = sceneObjVtbl;
    value = start;
}

extern "C" {

void * __attribute__((section(".text_80137FB0"))) globeMarkerInit(u8 *object, u16 value)
{
    *(u16 *)object = value;
    *(u32 *)(object + 4) = 0;
    return object;
}

s32 __attribute__((section(".text_80137FC4"))) resourceGetAddress(u8 *object)
{
    return *(s32 *)(object + 40);
}
}
