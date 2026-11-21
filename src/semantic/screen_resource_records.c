/* cc: fill */
typedef unsigned short u16;
typedef signed int s32;

extern unsigned char shellScene[];
extern s32 sceneResourceFallbackArg;
#pragma tm2_absolute sceneResourceFallbackArg
extern void (*sceneResourceFallbackHandler)(s32, void *);
#pragma tm2_absolute sceneResourceFallbackHandler
extern s32 sceneGetAuxObjCount(void *);
extern void *sceneGetAuxObj(void *, s32);
extern s32 sceneGetDecorCount(void *);
extern void *sceneGetDecor(void *, s32);
extern s32 sceneGetItemCount(void *);
extern void *sceneGetItem(void *, s32);
extern s32 sceneObjGetId(void *);
extern void sceneObjSetModel(void *, void *);
extern char *strcpy(char *, const char *);
#include "semantic/scene.h"

/* @80121E18: deliver a resource to every screen element (three element lists of the shell
 * screen shellScene) whose id matches the resource's id (u16 at +2); if none matched,
 * hand it to the fallback handler sceneResourceFallbackHandler(sceneResourceFallbackArg, resource). */
void __attribute__((section(".text_80121E18"))) sceneDeliverResource(void *resource)
{
    s32 matched = 0;
    s32 i;
    s32 id = *(u16 *)((char *)resource + 2);
    void *item;

    for (i = 0; i < sceneGetAuxObjCount(shellScene); i++) {
        item = sceneGetAuxObj(shellScene, i);
        if (id == (u16)sceneObjGetId(item)) {
            sceneObjSetModel(sceneGetAuxObj(shellScene, i), resource);
            matched = 1;
        }
    }
    for (i = 0; i < sceneGetDecorCount(shellScene); i++) {
        item = sceneGetDecor(shellScene, i);
        if (id == (u16)sceneObjGetId(item)) {
            sceneObjSetModel(sceneGetDecor(shellScene, i), resource);
            matched = 1;
        }
    }
    for (i = 0; i < sceneGetItemCount(shellScene); i++) {
        item = sceneGetItem(shellScene, i);
        if (id == (u16)sceneObjGetId(item)) {
            sceneObjSetModel(sceneGetItem(shellScene, i), resource);
            matched = 1;
        }
    }
    if (!matched && sceneResourceFallbackHandler != 0) {
        sceneResourceFallbackHandler(sceneResourceFallbackArg, resource);
    }
}

/* @80121FA8: initialise a resource record: name (copied at +0), value at +40, loaded flag at +44 cleared. */
void * __attribute__((section(".text_80121FA8")))
screenImageRecordInit(void *record, const char *name, s32 value)
{
    strcpy((char *)record, name);
    *(s32 *)((char *)record + 40) = value;
    *((char *)record + 44) = 0;
    return record;
}

/* @80121FE8: load the record's named resource and mark it loaded. Retail leaves 1 in v0
 * (callers declare an s32 result but ignore it). */
void __attribute__((section(".text_80121FE8"))) screenImageRecordLoad(void *record)
{
    screenLoadImageFile((const char *)record, 0);
    *((char *)record + 44) = 1;
}
