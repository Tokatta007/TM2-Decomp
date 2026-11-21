/* Shell scene record (the menu screen state, shellScene; 0xD7C bytes).
 *
 * Layout from sceneInit (scene_init.c), which clears it, and the scene
 * accessors in menu_object_leaves.cpp / menu_object_logic.c. Fields named
 * fXX have no known meaning yet. Uses plain C types so that units with their
 * own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_SCENE_H
#define TM2_SEMANTIC_SCENE_H

struct TM2ShellScene {
    int f0, f4, f8, fC;             /* +0x000 view frame counters (renderGetView0/1Frames) */
    int cam_pos[3];                 /* +0x010 camera (sceneSetCamera; cleared by sceneInit) */
    short cam_rot[3];               /* +0x01C */
    short p22;
    int f24;                        /* +0x024 sceneInit n: view mode 7 instead of 1 */
    int f28;                        /* +0x028 set to 1 by sceneInit */
    int done;                       /* +0x02C sceneSetDone */
    int exit_code;                  /* +0x030 sceneGetExitCode */
    int selection;                  /* +0x034 selected item (sceneSelectNext/Prev) */
    int background;                 /* +0x038 sceneSetBackground / sceneGetBackground2 */
    int f3C;                        /* +0x03C */
    unsigned char p40[0x20];
    int f60;                        /* +0x060 */
    unsigned char p64[0x40];
    int item_count;                 /* +0x0A4 */
    int items[32];                  /* +0x0A8 sceneGetSelectedItem2 */
    int f128;                       /* +0x128 */
    unsigned char p12C[0x800];
    int running;                    /* +0x92C sceneClearRunning */
    unsigned char p930[0x28];
    int events;                     /* +0x958 sceneClearEvents */
    unsigned char p95C[0x400];
    void *back;                     /* +0xD5C */
    void *cancel;                   /* +0xD60 */
    void *idle;                     /* +0xD64 */
    int fD68;                       /* +0xD68 */
    int fD6C;                       /* +0xD6C 60 at start */
    int fD70;                       /* +0xD70 */
    int fD74;                       /* +0xD74 */
    int fD78;                       /* +0xD78 */
};
typedef char tm2_shell_scene_size[(sizeof(struct TM2ShellScene) == 0xD7C) ? 1 : -1];
typedef char tm2_shell_scene_back_at[((unsigned int)&((struct TM2ShellScene *)0)->back == 0xD5C) ? 1 : -1];
typedef char tm2_shell_scene_events_at[((unsigned int)&((struct TM2ShellScene *)0)->events == 0x958) ? 1 : -1];

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2Settings;
#ifdef __cplusplus
extern "C" {
#endif

/* ai_route_helpers.c */
unsigned char movieSkipCheckDelayed(int value);

/* base_object_accessors.c */
int sceneObjGetPosition(int *p, int *a, int *b, int *c);

/* bitmap_rect_fill.c */
void bitmapMarkRect(int x, int y, int width, int height);

/* engine_render_scene.c */
void renderFlipFrame(void);

/* menu_state_accessors.c */
short unsigned int menuSliderGetValue(unsigned char *p);

/* menu_text_callbacks.c */
int menuCarNameText(int value);
void *menuRandomCarText(void);

/* password_rebase.c */
unsigned int passwordRebase(unsigned int value, unsigned short output_base, unsigned short input_base);

/* screen_basic.c */
int screenClearScreen(short x, short y, short width, short height);
int screenLoadImageFile(const char *name, int arg);
int screenSetEnv(void);

/* screen_credits.c */
void *screenCredits(void *caller);

/* screen_pause_menu.c */
void *screenHistory(void);

/* screen_resource_records.c */
void screenImageRecordLoad(void *record);

/* screen_title_level_select.c */
void *screenTitle(void);

/* screen_transfer.c */
int screenDisplayToDisplay(int right_buffer, int other);

/* shell_main.c */
int moviePlayEnding(struct TM2Settings *settings, char *name, int frames, int wide, void *skip);

/* shell_title_intro.c */
void *shellAttractCycle(void *value);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
