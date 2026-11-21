typedef signed int s32;
typedef unsigned char u8;

/* @80123CF4: step a wrapping index backwards. Same body as menu_object_logic.c
   sceneSelectPrev; returns nothing (v0 only keeps the intermediate sum). Its own unit
   because it byte-matches only with PsyQ 3.6 and -fvolatile, which the rest of
   base_object_accessors.c does not use. */
void __attribute__((section(".text_80123CF4"))) sceneCursorPrev(u8 *p)
{
    *(s32 *)(p+52)=(*(s32 *)(p+52)+*(s32 *)(p+164)-1)%*(s32 *)(p+164);
}
