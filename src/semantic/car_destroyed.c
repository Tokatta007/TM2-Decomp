/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map; H3 (retail's calls without it) */
#include "semantic/car_specials.h"  /* request 42 item 1: TM2_DT_BOSS_SLOT */
#include "semantic/car_numbers.h"   /* request 70: portCarProfileName */
#ifdef TM2_PORT_WRECK_FALL
#include "semantic/port_wreck_fall.h"   /* request 117 */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
/* request 42 item 1: a Dark Tooth's 360-tick wreck timer and self-hit only for the boss (a human or a computer Dark
   Tooth that is not ordinary); an ordinary one (portCarOptions bit 10) gets every car's 120 */
#define TM2_DT_DESTROY_BOSS(car, human) ((human) || TM2_DT_BOSS_SLOT(AI(car)->slot))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)


#define S8(p, o) (*(s8 *)((u8 *)(p) + (o)))
#define U8(p, o) (*(u8 *)((u8 *)(p) + (o)))
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define U16(p, o) (*(u16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))
#define PTR(p, o) (*(u8 **)((u8 *)(p) + (o)))

extern int rand(void);
extern u8 *laEventGet(s32);
extern u8 *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion24(void *, u8 flag);
extern void fxExplosion48(void *, u8 flag);
extern void fxExplosion96(void *, u8 flag);
extern void debrisBurstAt(s32, void *, void *, void *, void *, s32, s32, s32, s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern u32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void fxSheetStart(s32, s32, s32, s32);
#include "semantic/world.h"

extern u8 moscowDishState[];
/* Moscow dish emitters (5 x 16 bytes): a flag word and the emitter position;
 * entries 1..4 are the satellites. */
typedef struct {
    s32 flag;
    s32 pos[3];
} TM2DishEmitter;
extern TM2DishEmitter moscowDishEmitters[5];

/* @800D5AF0: per-frame update of random events 1 and 2 (196-byte records from laEventGet):
   tick the timer, spawn random sparks, fire each lane's effect at its start time, end the lane
   with a shake/rumble, and once both lanes are done play the final burst and retire the event. */
void __attribute__((section(".text_800D5AF0"))) laEventsUpdate(void)
{
    s32 k, j;
    u8 *p;
    u8 *o;
    s16 t;

    for (k = 1; k < 3; k++) {
        p = laEventGet(k);
        if (p[1] == 0)
            continue;
        t = ++S16(p, 10);
        if (S8(p, 3)-- < 5)
            S8(p, 3) = 5;
        for (j = 1; j < 3; j++) {
            if (rand() % S8(p, 3) == 0 && p[j + 5] == 0) {
                o = mathJitterPos(p + 56 + j * 12, 0x51, 0x51, 0x147);
                S32(o, 8) += 160;
                if (rand() & 1)
                    fxExplosion24(o, 0);
                else
                    fxExplosion48(o, 0);
            }
            if (S16(p, 10) == S16(p, 14 + j * 2)) {
                debrisBurstAt(S32(p, 36 + j * 4), p + 56 + j * 12, &p[j * 8 + 156], &p[j * 8 + 116], 0,
                             S16(p, 24 + j * 2), 20, 200, 1);
#ifdef TM2_PORT_HAZARDS
                /* request 38 (RETAIL_BUGS.md 18): lane 1 has no object on the disc; retail writes 0 to address
                   0x12 here. Skipped for a null object only: every other case writes as retail. */
                if (PTR(p, 36 + j * 4) != 0)
#endif
                U8(PTR(p, 36 + j * 4), 18) = 0;
            }
            if (S16(p, 10) >= S16(p, 14 + j * 2) + S16(p, 24 + j * 2) &&
                S16(p, 10) < S16(p, 14 + j * 2) + S16(p, 24 + j * 2) + 10) {
                if (rand() % 3 == 0)
                    fxExplosion96(mathJitterPos(p + 56 + j * 12, 0x51, 0x51, 0x147), 0);
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(p + 56 + j * 12);
                else
                    soundLocateForView(p + 56 + j * 12, -1);
                soundPlay(4, PadChkMtap() >> 1, soundGetListenerPan(), 99);
                soundPlay(19, PadChkMtap() >> 1, soundGetListenerPan(), 99);
                p[j + 5] = 1;
            }
        }
        if (p[6] && p[7]) {
            o = mathJitterPos(p + 68, 0x147, 0x147, 0x37);
            S32(o, 8) -= 400;
            fxExplosion96(o, 0);
            o = mathJitterPos(p + 80, 0x147, 0x147, 0x37);
            S32(o, 8) -= 400;
            fxExplosion96(o, 0);
            fxSheetStart(3, 255, 200, 200);
            objRegistrySetStep(0xFA5, k, 1);
            p[4] = 1;
        }
        if (p[6] && p[7]) {
            if (p[2] == 0)
                p[2] = S16(p, 10) + 20;
            if (S16(p, 10) >= S8(p, 2))
                p[1] = 0;
        }
    }
}

/* @800D6188: per-frame update of random event 0 (moscowDishState): tick, spawn random sparks (plus the
   four satellite emitters moscowDishEmitters[1..4] before the start time), fire the main effect at its start
   time, advance its value while running, then end it with a burst and retire it. */
void __attribute__((section(".text_800D6188"))) moscowDishUpdate(void)
{
    u8 *p = moscowDishState;
    u8 *o;
    u8 *q;
    s32 i;
    s16 t;

    if (p[0] == 0)
        return;
    t = ++S16(p, 6);
    if (S8(p, 3)-- < 5)
        S8(p, 3) = 5;
    if (rand() % S8(p, 3) == 0) {
        o = mathJitterPos(p + 16, 0x52, 0x52, 0x52);
        if (rand() & 1)
            fxExplosion24(o, 0);
        else
            fxExplosion48(o, 0);
        if (S16(p, 6) < S16(p, 8)) {
            for (i = 1, q = (u8 *)moscowDishEmitters[1].pos; i < 5; i++, q += 16) {
                o = mathJitterPos(q, 0x80, 0x80, 0x80);
                if (rand() & 1)
                    fxExplosion24(o, 0);
                else
                    fxExplosion48(o, 0);
            }
        }
    }
    if (S16(p, 6) == S16(p, 8)) {
        debrisBurstAt(S32(p, 12), p + 16, p + 48, p + 28, p + 36, S16(p, 10), 20, 200, 0);
        U8(PTR(p, 12), 18) = 0;
        objRegistrySetStep(0xBBC, 0, 1);
        fxSheetStart(3, 255, 200, 200);
        for (i = 1, o = (u8 *)moscowDishEmitters[1].pos; i < 5; i++, o += 16) {
            objRegistrySetStep(0xBBB, i, 1);
            fxExplosion96(mathJitterPos(o, 0x80, 0x80, 0x80), 0);
        }
    }
    if (S16(p, 6) >= S16(p, 8))
        S32(p, 24) += S32(p, 44);
    if (S16(p, 6) >= S16(p, 8) + S16(p, 10) && S16(p, 6) < S16(p, 8) + S16(p, 10) + 10) {
        if (rand() % 3 == 0)
            fxExplosion96(mathJitterPos(p + 16, 0x52, 0x52, 0x52), 0);
        p[5] = 1;
    }
    if (p[5] && p[4] == 0) {
        o = mathJitterPos(p + 16, 0x147, 0x147, 0x37);
        S32(o, 8) -= 400;
        fxExplosion96(o, 0);
        fxSheetStart(3, 255, 200, 200);
        objRegistrySetStep(0xBBD, 0, 1);
        p[4] = 1;
        p[0] = 0;
    }
    if (p[5]) {
        if (p[2] == 0)
            p[2] = S16(p, 6) + 20;
        if (S16(p, 6) >= S8(p, 2))
            p[0] = 0;
    }
}


extern s32 sceneObjGetPosition(s32, s32 *, s32 *, s32 *);
extern void sceneObjSetPosition(s32, s32, s32, s32);
extern void sceneObjSetVisible(s32, u16 v);
#include "semantic/input.h"
extern void menuPasswordDigitSet(s32, u16 index, u16 value);
extern s32 scenePostEvent(s32, s32, void *);
extern void *sceneGetSelectedItem2(s32);
extern s32 sceneStopCommand(s32, void *);
extern u16 menuPasswordRow;
extern u16 menuPasswordInputHeld;

/* C++ virtual call: object at s+0x1C, vptr at +8, vtable slot 1 ({s16 delta; s16; fn}). */
static __inline__ void menu_vcall(u8 *s)
{
    u8 *vt = PTR(PTR(s, 28), 8);
    ((void (*)(u8 *))PTR(vt, 12))(PTR(s, 28) + S16(vt, 8));
}

static __inline__ void menu_reset_call(s32 o, s32 a, void *b)
{
    menuPasswordRow = 5;
    scenePostEvent(o, a, b);
}

static __inline__ void menu_reset_show(s32 o, s32 a)
{
    menuPasswordRow = 5;
    sceneObjSetVisible(o, a);
}

/* @8012D3F0: menu cursor handler: place the cursor model on the current row (menuPasswordRow, 0..5),
   then on pad input move up/down, pick an entry (rows via menuPasswordDigitSet with action 0-4) or cancel
   (button 8) back to the parent; marks the input as consumed in menuPasswordInputHeld. */
void __attribute__((section(".text_8012D3F0"))) menuPasswordCursorRun(u8 *s)
{
    s32 x, y, z;
    u16 back;
    u16 sel;
    s32 i;

    sceneObjGetPosition(S32(s, 20), &x, &y, &z);
    sceneObjSetPosition(S32(s, 20), S32(s, 24) + (5 - menuPasswordRow) * 184, y, z);
    back = 0;
    sel = 0;
    sceneObjSetVisible(S32(s, 20), 1);
    padUpdate();
    if (padAnyPressed(0)) {
        if (menuPasswordInputHeld == 0) {
            if (padCheckAction(2, 0)) {
                if (menuPasswordRow < 5) {
                    menu_vcall(s);
                    menuPasswordRow++;
                } else
                    back = 1;
            } else if (padCheckAction(3, 0)) {
                if (menuPasswordRow != 0) {
                    menu_vcall(s);
                    menuPasswordRow--;
                } else
                    back = 1;
            } else {
                i = 0;
                if (padCheckAction(0, 0) || padCheckAction(1, 0))
                    i = 1;
                if (i) {
                    menu_vcall(s);
                    menuPasswordDigitSet(S32(s, 16), menuPasswordRow, 0);
                    sel = 1;
                } else {
                    /* sel is 0 here (u16 >> 16); a literal 0 would be folded into
                     * the calls, retail keeps move s1,zero / move a1,s1 */
                    s32 j = sel >> 16;

                    if (padCheckAction(13, j)) {
                        menu_vcall(s);
                        menuPasswordDigitSet(S32(s, 16), menuPasswordRow, 1);
                        sel = 1;
                    } else if (padCheckAction(11, j)) {
                        menu_vcall(s);
                        menuPasswordDigitSet(S32(s, 16), menuPasswordRow, 2);
                        sel = 1;
                    } else if (padCheckAction(10, j)) {
                        menu_vcall(s);
                        menuPasswordDigitSet(S32(s, 16), menuPasswordRow, 3);
                        sel = 1;
                    } else if (padCheckAction(12, j)) {
                        menu_vcall(s);
                        menuPasswordDigitSet(S32(s, 16), menuPasswordRow, 4);
                        sel = 1;
                    } else if (padCheckAction(8, j)) {
                        menu_reset_call(S32(s, 12), 2, s);
                        scenePostEvent(S32(s, 12), 3, sceneGetSelectedItem2(S32(s, 12)));
                    }
                }
            }
            if (back) {
                menu_reset_show(S32(s, 20), 0);
                scenePostEvent(S32(s, 12), 2, s);
                sceneStopCommand(S32(s, 12), s);
            }
            if (sel) {
                if (menuPasswordRow == 0)
                    menuPasswordRow = 5;
                else
                    menuPasswordRow--;
            }
            menuPasswordInputHeld = 1;
        }
    } else
        menuPasswordInputHeld = 0;
}

extern s32 humanCarObjectSlots[];
extern s32 aiCarObjectSlots[];
extern u8 *carSlotObjects[];
extern s32 roundWonFlag;
extern s32 gameGetLevel(void);
extern void fxSplashAt(void *, s32);
extern s32 soundStopSharedLoop(s32, s32);
extern s32 soundStop(s32);
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-4 (CAR_GROUPS.md 16) */
#endif
extern void fxSmokeA24(void *, s32);
extern void carApplyDamage(s32, s32, s32, s32, s32);

/* @8011BE10: car destroyed / respawn handler (human = 1 for a player car, 0 for an AI car):
   drop its held pickup, rumble/sfx for the hit type, then either sink it (out of the arena:
   events 6/8) or explode it (smoke, wreck sound, respawn timer 120/360), clear its target link,
   and restart its state machine via carShakeStart. */
void __attribute__((section(".text_8011BE10"))) carDestroyed(void *car, u8 human)
{
    s32 mode;
    struct TM2CarMotion *lane;
    struct TM2CarStatsBlock *st;
    s32 idx;
    struct TM2CarSlam *pick;
    u8 *link;
    struct TM2CarState *fl;
    s32 pos[3];
    s32 dead;

    if (human) {
        HREC(car)->state.f11 = 1;
        mode = HREC(car)->type;
        lane = &HREC(car)->motion;
        st = &HREC(car)->stats;
        idx = humanCarObjectSlots[HREC(car)->player];
        pick = HREC(car)->weapons.slam;
        link = PTR(car, 204);
        fl = &HREC(car)->state;
        if (roundWonFlag != 0)
            return;
    } else {
        mode = AI(car)->type;
        idx = aiCarObjectSlots[AI(car)->index];
        pick = AI(car)->weapons.slam;
        link = PTR(car, 152);
        lane = &AI(car)->motion;
        st = &AI(car)->stats;
        fl = &AI(car)->state;
        if (AI(car)->target_angle > 0)
            AI(car)->state.f11 = 0;
        else
            AI(car)->state.f11 = 1;
    }
    if (mode == 0xC0) {
        if (pick) {
            carSpinOutById(pick->spin_target, lane->rot[2], 0);
            pick->active = 0;
            pick->spin_target = -1;
            pick->spin_pending = 0;
        }
    } else if (fl->f1D) {
#ifdef TM2_PORT_CAR_GROUPS
        carSpinOutById(TM2_CAR_KEY(idx, mode), lane->rot[2], 0);   /* request 32: idx = its slot */
#else
        carSpinOutById(mode, lane->rot[2], 0);
#endif
    }
    if (viewGetSplitMode() == 0)
        soundLocateForView0(lane->pos);
    else
        soundLocateForView(lane->pos, -1);
    if (mode == 0x60 || mode == 0x30 || mode == 0xA0)
        soundPlay(26, PadChkMtap() >> 1, soundGetListenerPan(), 99);
    else if (PadChkMtap() < 1000)
        soundPlay(25, 0, 0, 47);
    else
        soundPlay(25, PadChkMtap() / 3, soundGetListenerPan(), 47);
    soundStopSharedLoop(9, -1);
#ifdef TM2_PORT_CAR_SOUND
    if (mode == 0xA0)
        portCarTornadoSoundStop(idx);   /* request 33-4: this Twister's tornado sound only (bit 4) */
#else
    if (mode == 0xA0)
        soundStop(0x35);
#endif
    if (gameGetLevel() == 6 && lane->pos[2] < 240) {
        csSetDrawEnable(carSlotObjects[idx], 0);
        fxSplashAt(lane->pos, 2000);
        if (human)
            TM2_HUMAN_SOUND(HREC(car)->player, 34, 0, 0, 99);   /* H3: the player's own cue */
        st->wreck_timer = 0;
    } else if (gameGetLevel() == 8 && lane->pos[2] < -40) {
        csSetDrawEnable(carSlotObjects[idx], 0);
        st->wreck_timer = 0;
    } else {
        pos[0] = lane->pos[0];
        pos[1] = lane->pos[1];
        dead = lane->pos[2];
        pos[2] = 0;
        fxSmokeA24(pos, 0);
        carSpinOut(car, human, 4, 0);
        fxExplosion48(lane->pos, 0);
        fxExplosion24(lane->pos, 0);
        fl->hud_notice = 1;
        if (mode == 0xF0 && TM2_DT_DESTROY_BOSS(car, human)) {
            st->wreck_timer = 360;
            carApplyDamage(S32(carSlotObjects[idx], 280), 15, 0, 2, 0);
        } else
            st->wreck_timer = 120;
#ifdef TM2_PORT_WRECK_FALL
        portWreckFallBlown(car, human);   /* request 117: blown up at the floor, or in the air (the wreck falls on) */
#endif
    }
    if (link) {
        S32(link, 8) = 0;
        if (human)
            S32(car, 204) = 0;
        else
            S32(car, 152) = 0;
    }
    fl->f10 = 1;
    fl->frozen = 1;
    st->freeze_timer = 540;
    st->mounts_cars = 0;
    carShakeStart(car, 0, 2, 0, human);
}

typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0, x1, y1, x2, y2, x3, y3; } TM2PolyF4;
typedef struct { u32 tag; u8 r0, g0, b0, code; s16 x0, y0; u8 r1, g1, b1, p1; s16 x1, y1;
    u8 r2, g2, b2, p2; s16 x2, y2; u8 r3, g3, b3, p3; s16 x3, y3; } TM2PolyG4;
typedef struct { u32 tag; u32 code[2]; } TM2DrMode;

/* @801828F0: 1900-byte HUD packet record per player (only the +0x6C enemy-health bar is used here). */
typedef struct {
    u8 pad_000[0x6C];
    TM2PolyG4 g1;       /* +06C */
    TM2PolyF4 f1;       /* +090 */
    TM2DrMode m1;       /* +0A8 */
    u8 pad_0B4[1900 - 0xB4];
} TM2HudRec;

extern TM2HudRec hudPlayerRecords[];
extern u8 hudRadarVisible[];
extern u8 hudTargetNameModes[][12];
extern u8 hudTargetNameSprites[][600];
extern u8 *carNearestToHuman(s32, u8 *);
extern u8 *carTypeName(s32);
#include "semantic/hud.h"
extern s32 hudHealthLevel(s32);
extern void UAdashHealthColor(s32, void *);
extern void AddPrim(u32 *, void *);

#define SETXY4(p, _x0, _y0, _x1, _y1, _x2, _y2, _x3, _y3) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x1), (p)->y1 = (_y1), \
     (p)->x2 = (_x2), (p)->y2 = (_y2), (p)->x3 = (_x3), (p)->y3 = (_y3))

/* @800FD0D0: draw player i's target panel: the targeted car's name (green, y depends on view
   mode) and its health bar (82 px wide, coloured by health %, panel position per view mode).
   TM2_PORT_HUMANS (H1): i is the HUD slot; the human on it (portLocalHuman) picks the target. */
void __attribute__((section(".text_800FD0D0"))) hudDrawTargetPanel(u32 *ot, s32 i)
{
    u8 flag;
    u8 *car;
    s32 id;
    u8 *st;
    s32 w;
    s32 kind;
#ifdef TM2_PORT_HUMANS
    s32 human = TM2_LOCAL_HUMAN(i);

    if (human < 0)
        return;
    car = carNearestToHuman(human, &flag);
#else
    car = carNearestToHuman(i, &flag);
#endif
    if (car == 0 || flag != 0)
        return;
    id = S32(car, 48);
    st = car + 1276;
#ifdef TM2_PORT_CAR_PROFILE
#define carTypeName(id) ((u8 *)portCarProfileName(S16(car, 6), (id)))  /* request 70: a mod car's own name
                                                    (the target is an AI record: its slot at +0x006) */
#endif
    if (S32(car, 1440) > 0) {
        fontSetColor(2, 36, 150, 36);
        if (hudRadarVisible[i])
            fontSpritePrintCenteredXY(2, 48, 79, hudTargetNameModes[i], hudTargetNameSprites[i], carTypeName(id), ot);
        else if (viewGetSplitMode() == 2)
            fontSpritePrintCenteredXY(2, 48, 29, hudTargetNameModes[i], hudTargetNameSprites[i], carTypeName(id), ot);
        else
            fontSpritePrintCenteredXY(2, 48, 49, hudTargetNameModes[i], hudTargetNameSprites[i], carTypeName(id), ot);
        fontResetColor(2);
    }
#ifdef TM2_PORT_CAR_PROFILE
#undef carTypeName
#endif
    if (hudRadarVisible[i]) {
        SETXY4(&hudPlayerRecords[i].g1, 7, 68, 89, 68, 7, 80, 89, 80);
        SETXY4(&hudPlayerRecords[i].f1, 6, 67, 90, 67, 6, 81, 90, 81);
    } else if (viewGetSplitMode() == 2) {
        SETXY4(&hudPlayerRecords[i].g1, 7, 18, 89, 18, 7, 30, 89, 30);
        SETXY4(&hudPlayerRecords[i].f1, 6, 17, 90, 17, 6, 31, 90, 31);
    } else {
        SETXY4(&hudPlayerRecords[i].g1, 7, 38, 89, 38, 7, 50, 89, 50);
        SETXY4(&hudPlayerRecords[i].f1, 6, 37, 90, 37, 6, 51, 90, 51);
    }
    if (S16(st, 58) == 0)
        S16(st, 58) = 1;
    w = S32(st, 164) * 100 / S16(st, 58);
    kind = hudHealthLevel(w);
    w = w * 82 / 100;
    w = w <= 0 ? 1 : (w > 82 ? 82 : w);
    if (w) {
        hudPlayerRecords[i].g1.x1 = hudPlayerRecords[i].g1.x0 + w;
        hudPlayerRecords[i].g1.x3 = hudPlayerRecords[i].g1.x2 + w;
        UAdashHealthColor(kind, &hudPlayerRecords[i].g1);
        AddPrim(ot, &hudPlayerRecords[i].g1);
    }
    AddPrim(ot, &hudPlayerRecords[i].f1);
    AddPrim(ot, &hudPlayerRecords[i].m1);
}
