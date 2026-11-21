/* Selection commands: run functions of the menu-choice command classes.
 * C++ (PsyQ 3.6 CC1PLPSX, -fvolatile); C linkage. The mode switch of
 * @801232EC compiles to a jump table that retail keeps at 800CF4D4. */
typedef unsigned char u8;
typedef signed int s32;

struct TM2SelectCommand {
    s32 duration;
    s32 remaining;
    void *vtable;
    s32 target;                 /* +12: choice id / menu state */
    s32 mode;                   /* +16 */
    s32 value;                  /* +20 */
    s32 (*accept)(s32);         /* +24 */
};

#define W(p, off) (*(s32 *)((u8 *)(p) + (off)))

#pragma tm2_rdata .tm2_800CF4D4

extern "C" {
u8 *sceneObjGetModel(s32);
s32 rand(void);
s32 menuItemGetHandler(s32);

/* @8012305C: step, set or randomize the current entry of a choice (type 9),
 * repeating while the accept callback rejects the entry. */
void __attribute__((section(".text_8012305C"))) sceneCmdChoiceStepRun(TM2SelectCommand *c)
{
    u8 *found = sceneObjGetModel(c->target), *choice;
    if (found[0] != 9) return;
    choice = found;
    switch (c->mode) {
    case 0:
        choice[7] = (choice[7] + choice[6] + c->value) % choice[6];
        if (c->accept)
            while (c->accept(choice[7]) == 0)
                choice[7] = (choice[7] + choice[6] + c->value) % choice[6];
        break;
    case 1:
        if (!c->accept || c->accept(c->value) != 0) choice[7] = c->value % choice[6];
        break;
    case 2:
        choice[7] = (choice[7] + rand()) % choice[6];
        if (c->accept)
            while (c->accept(choice[7]) == 0)
                choice[7] = (choice[7] + rand()) % choice[6];
        break;
    }
}

/* @801232EC: store the selected result (+3440) of a menu state by mode. */
void __attribute__((section(".text_801232EC"))) sceneCmdExitRun(TM2SelectCommand *c)
{
    W(c->target, 44) = 1;
    W(c->target, 48) = c->mode;
    switch (c->mode) {
    case 0: {
        u8 *state = (u8 *)c->target;
        if (W(state, 52) < 0 || W(state, 52) >= W(state, 164))
            W(state, 3440) = 0;
        else
            W(c->target, 3440) = menuItemGetHandler(W(state + W(state, 52) * 4, 168));
        break;
    }
    case 1: W(c->target, 3440) = W(c->target, 3420); break;
    case 2: W(c->target, 3440) = W(c->target, 3424); break;
    case 3: W(c->target, 3440) = W(c->target, 3428); break;
    case 4: W(c->target, 3440) = 0; break;
    }
}
}
