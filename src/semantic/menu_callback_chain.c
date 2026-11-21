typedef void *(*TM2MenuCallback)(void *,void *);

extern void *shellPlayTitleIntro(void *, void *);
#include "semantic/scene.h"

/* Shell screen loop: start at screenTitle and keep calling the returned
   screen handler (handler(previous, first)) until one returns NULL. */
void __attribute__((section(".text_80136F0C"))) shellScreenLoop(void)
{
    TM2MenuCallback current;
    TM2MenuCallback previous;
    TM2MenuCallback first;
    shellPlayTitleIntro(0,0);
    current=(TM2MenuCallback)screenTitle;
    first=current;
    previous=current;
    while(current!=0){
        TM2MenuCallback next=(TM2MenuCallback)current((void *)previous,(void *)first);
        previous=current;
        current=next;
    }
}
