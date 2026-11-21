/* Scene object list consumers. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). */
typedef signed int s32;
/* Script command objects (g++ 2.x: vtable pointer at +8, run() is entry 1). */
struct TM2Object { int duration, remaining; virtual int run(); };
struct TM2Action { s32 key0, key1, type; TM2Object *object; };
struct TM2Event { s32 key0, key1; };
/* Scene state: the action table, the running objects and the pending events. */
struct TM2Scene {
    char pad0[296];
    s32 action_count;
    TM2Action actions[128];
    s32 object_count;
    TM2Object *objects[10];
    s32 event_count;
    TM2Event events[8];
};
extern "C" {
extern s32 DrawSync(s32);
extern s32 VSync(s32);
extern void shellLoadTmsDmd(const char *);
extern s32 sceneStartCommand(TM2Scene *, TM2Object *);
extern s32 sceneStopCommand(TM2Scene *, TM2Object *);

s32 __attribute__((section(".text_80122750"))) sceneLoadTmsDmd(void *unused, const char *name)
{
    DrawSync(0); VSync(0); shellLoadTmsDmd(name); return 1;
}

/* Run every object in the list. */
void __attribute__((section(".text_80122790"))) sceneRunCommands(TM2Scene *scene)
{
    int i;
    for (i = 0; i < scene->object_count; i++)
        scene->objects[i]->run();
}

/* Dispatch pending events to the matching actions (key1 0 = any): type 0
 * adds the object, 1 removes it, 2 runs it. Clears the events; the return
 * value is unset, as in retail. */
s32 __attribute__((section(".text_80122818"))) sceneDispatchEvents(TM2Scene *scene)
{
    int e, a;
    for (e = 0; e < scene->event_count; e++)
        for (a = 0; a < scene->action_count; a++)
            if (scene->events[e].key0 == scene->actions[a].key0
                && (scene->events[e].key1 == scene->actions[a].key1 || scene->actions[a].key1 == 0)) {
                switch (scene->actions[a].type) {
                case 0: sceneStartCommand(scene, scene->actions[a].object); break;
                case 1: sceneStopCommand(scene, scene->actions[a].object); break;
                case 2: scene->actions[a].object->run(); break;
                }
            }
    scene->event_count = 0;
}
}
