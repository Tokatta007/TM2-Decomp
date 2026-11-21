/* Menu controller command: the run function @80136F6C and its constructor
 * @80136FB8. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). The constructor stores the
 * vtable of each class in the chain (menu command, menu target command, this
 * class), then its fields; vtables are retail data (#pragma tm2_symbol). */
typedef signed int s32;
typedef unsigned int u32;
typedef unsigned char u8;

struct TM2MenuCommand {             /* vtable 800D03AC */
    int duration;
    int remaining;
    virtual void run();
    TM2MenuCommand() {}
};

struct TM2MenuTargetCommand : TM2MenuCommand {   /* vtable 800D038C */
    s32 target;
    virtual void run();
    TM2MenuTargetCommand() {}
};

struct TM2MenuCallback : TM2MenuTargetCommand {  /* vtable 800D025C */
    s32 source;
    s32 target_copy;
    s32 (*callback)(s32);
    virtual void run();
    TM2MenuCallback(s32 first, s32 second, s32 (*fn)(s32));
};

#pragma tm2_symbol _vt.14TM2MenuCommand D_800D03AC
#pragma tm2_symbol _vt.20TM2MenuTargetCommand D_800D038C
#pragma tm2_symbol _vt.15TM2MenuCallback D_800D025C
#pragma tm2_symbol __15TM2MenuCallbackiiPFi_i sub_80136FB8
TM2MenuCallback::TM2MenuCallback(s32 first, s32 second, s32 (*fn)(s32))
{
    target = first;
    source = second;
    target_copy = first;
    callback = fn;
}

extern "C" {
s32 sceneObjGetChoiceState(s32);
void menuTextSetValue(void *, s32);
void menuCallbackRun(u8 *);

struct TM2VtableEntry { short delta, index; void (*function)(u8 *); };
TM2VtableEntry D_800D025C[2] __attribute__((section(".tm2_800D025C"))) = {{0, 0, 0}, {0, 0, menuCallbackRun}};

/* @80136F6C: run: map the source value, pass it through the callback, apply it to the target. */
void __attribute__((section(".text_80136F6C"))) menuCallbackRun(u8 *object)
{
    s32 value = sceneObjGetChoiceState(*(s32 *)(object + 16));
    value = ((s32 (*)(s32))*(u32 *)(object + 24))(value);
    menuTextSetValue(*(void **)(object + 20), value);
}
}
