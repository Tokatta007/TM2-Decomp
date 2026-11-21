/* input.h: shared prototypes (see docs/SIGNATURES.md). Plain C types only, so units
 * with their own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_INPUT_H
#define TM2_SEMANTIC_INPUT_H

/* --- prototypes (shared; definitions in src/semantic) --- */
#ifdef __cplusplus
extern "C" {
#endif

/* car_parts_spin.c */
int padWaitConfirmOrTimeout(int n, int limit);

/* controls.c */
unsigned char padAnyPressed(short int port);
unsigned char padCheckAction(int action, int port);
void padSetRepeatMode(unsigned char mode);
void padUpdate(void);

/* shared_pad.c */
int padAnyLatched(int port);
int padComboHeld(short int combo, short int port);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
