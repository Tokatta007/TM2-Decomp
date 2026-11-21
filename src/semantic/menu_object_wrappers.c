typedef signed int s32;
typedef unsigned char u8;
#include "semantic/scene.h"

/* Menu commands that act on their scene (the shell scene, +12). */
struct TM2MenuSceneCmd {
    u8 pad_00[12];
    struct TM2ShellScene *scene;    /* +12 */
    s32 index;                      /* +16 menuSelectIndexRun: item to select */
};
struct TM2MenuPictureCmd {
    u8 pad_00[12];
    struct TM2ShellScene *scene;    /* +12 */
    char name[4];                   /* +16 picture file name (runs on) */
};

extern void sceneSetSelection(struct TM2ShellScene *, s32);
extern void sceneSelectPrev(struct TM2ShellScene *);
extern void sceneSelectNext(struct TM2ShellScene *);
extern s32 sceneGetBackground2(struct TM2ShellScene *);
extern s32 resourceGetAddress(u8 *);
extern s32 fileioLoadFileIntoRam(s32, const char *);

void __attribute__((section(".text_80137AAC"))) menuSelectIndexRun(struct TM2MenuSceneCmd *object)
{
    sceneSetSelection(object->scene, object->index);
}

void __attribute__((section(".text_80137B10"))) menuSelectPrevRun(struct TM2MenuSceneCmd *object)
{
    sceneSelectPrev(object->scene);
}

void __attribute__((section(".text_80137B68"))) menuSelectNextRun(struct TM2MenuSceneCmd *object)
{
    sceneSelectNext(object->scene);
}

s32 __attribute__((section(".text_80137BC0"))) menuLoadPictureRun(struct TM2MenuPictureCmd *object)
{
    s32 entry = sceneGetBackground2(object->scene);
    s32 address = resourceGetAddress((u8 *)entry);
    return fileioLoadFileIntoRam(address, object->name);
}
