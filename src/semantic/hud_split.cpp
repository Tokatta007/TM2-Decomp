/* Split-screen HUD, @800F9084 (lower view) and @800F9D48 (upper view): the
 * same code with different coordinates. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile).
 * Every record access indexes the per-player HUD record array hudPlayerRecords
 * directly; the primitives are libgpu-style (tag word, r0 g0 b0 code, then
 * coordinates). Shapes that retail code depends on:
 *  - one variable carries percent -> bar width; the clamp is followed by
 *    `if (width)` (always true, kept for identical code);
 *  - the ammo digits call hudDrawNumber in each branch; the one-digit case
 *    passes the high sprite + 1 (the low sprite);
 *  - the three weapon frames are an array of two plus a separate third;
 *  - the split-screen divider hudSplitDivider is written through a pointer (a
 *    one-element array), so under -fvolatile each store keeps its own
 *    absolute address. */
struct TM2FontDrawMode { unsigned int words[3]; };
struct TM2FontSprite { unsigned int words[5]; };

struct TM2PolyG4 {             /* 36 bytes */
    unsigned int tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char r1, g1, b1, p1;
    short x1, y1;
    unsigned char r2, g2, b2, p2;
    short x2, y2;
    unsigned char r3, g3, b3, p3;
    short x3, y3;
};

struct TM2LineF4 {             /* 24 bytes */
    unsigned int tag;
    unsigned char r0, g0, b0, code;
    short x0, y0, x1, y1, x2, y2, x3, y3;
};

struct TM2Sprt {               /* 20 bytes */
    unsigned int tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short w, h;
};

struct TM2Mode12 { unsigned int words[3]; };   /* 12-byte draw-mode prim */

struct TM2DigitPair { TM2Sprt high, low; };

struct TM2HudPlayer {          /* 1900 bytes */
    char pad0[0x24];
    TM2PolyG4 health;              /* +0x24 */
    TM2LineF4 health_frame;        /* +0x48 */
    TM2Mode12 health_mode;         /* +0x60 */
    char pad6C[0x48];
    TM2PolyG4 ammo;                /* +0xB4 */
    TM2LineF4 ammo_frame;          /* +0xD8 */
    TM2Mode12 ammo_mode;           /* +0xF0 */
    short weapon_count;            /* +0xFC */
    short padFE;
    TM2Sprt weapon_icon[10];       /* +0x100 */
    TM2Mode12 weapon_icon_mode[10];/* +0x1C8 */
    TM2Sprt weapon_big[10];        /* +0x240 */
    TM2Mode12 weapon_big_mode[10]; /* +0x308 */
    TM2DigitPair digits[10];       /* +0x380 */
    TM2Mode12 digit_mode[10];      /* +0x510 */
    TM2LineF4 weapon_frame[2];     /* +0x588 */
    TM2LineF4 weapon_frame_end;    /* +0x5B8 */
    char pad5D0[0x50];
    char digit_mode_data[0x98];    /* +0x620 */
    TM2Sprt lives_icon;            /* +0x6B8 */
    TM2Mode12 lives_mode;          /* +0x6CC */
    TM2DigitPair lives;            /* +0x6D8 */
    TM2Mode12 lives_digit_mode;    /* +0x700 */
    char pad70C[1900 - 0x70C];
};
typedef char tm2_hud_player_size[(sizeof(TM2HudPlayer) == 1900) ? 1 : -1];


#include "semantic/cars.h"
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map */
#endif

extern "C" {
extern TM2HudPlayer hudPlayerRecords[TM2_MAX_HUDS];
extern unsigned int hudOrderTables[2][TM2_MAX_HUDS][2], *hudOrderingTable;
extern unsigned char hudRadarVisible[4], hudWeaponListVisible[4];
extern char *hudLivesText;
extern TM2FontDrawMode hudSplitVertLivesMode, hudSplitHorzLivesMode;          /* font draw modes */
extern TM2FontSprite hudSplitVertLivesSprites[10], hudSplitHorzLivesSprites[10];    /* their sprite buffers */
extern TM2LineF4 hudSplitDivider[1];   /* split-screen divider */
u32 *ClearOTagR(unsigned int *, int);
void DrawOTag(unsigned int *);
struct TM2HumanRecord *humanCarGet(int);
int gameGetMode(void);
u8 gameGetLives(int);
int hudHealthLevel(int);
void hudDisplayRadar(unsigned int *, int);
void hudDrawTargetPanel(unsigned int *, int);
void fontSetColor(int, int, int, int);
void UAdashHealthColor(int, void *);
int fontSpritePrintXY(int, int, int, TM2FontDrawMode *, TM2FontSprite *, const unsigned char *, unsigned int *);
void AddPrim(unsigned int *, void *);
void hudBlinkWeaponMarker(int, int);
void hudBlinkWeaponDigits(int, int);
unsigned int hudDrawNumber(int, void *, void *, unsigned int *);
}

#define REC hudPlayerRecords[player]
/* co-op (mode 2) lives: retail prints the team's lives[0] on HUD 0; with portHumanOptions bit 1 (HUMANS16_PLAN.md
   H5) every HUD prints its own human's (portHumanLives through gameGetLives) */
#ifdef TM2_PORT_HUMANS
#define HUD_LIVES_HERE(player) ((player) == 0 || portHumanRules())
#define HUD_LIVES_OF(human) (portHumanRules() ? (human) : 0)
#else
#define HUD_LIVES_HERE(player) ((player) == 0)
#define HUD_LIVES_OF(human) 0
#endif
#define setXY4(p, a, b, c, d, e, f, g, h) \
    ((p)->x0 = (a), (p)->y0 = (b), (p)->x1 = (c), (p)->y1 = (d), (p)->x2 = (e), (p)->y2 = (f), (p)->x3 = (g), (p)->y3 = (h))
#define setXY0(p, a, b) ((p)->x0 = (a), (p)->y0 = (b))
#define setRGB0(p, r, g, b) ((p)->r0 = (r), (p)->g0 = (g), (p)->b0 = (b))

/* @800F9084 */
extern "C" void __attribute__((section(".text_800F9084"))) hudDrawSplitVertical(int player, int buffer)
{
    struct TM2HumanRecord *actor;
    int kind, width, i, amount, raw;
#ifdef TM2_PORT_HUMANS
    int human = TM2_LOCAL_HUMAN(player);   /* H1: the human on HUD slot `player`; none: no HUD */

    if (human < 0)
        return;
#endif

    hudOrderingTable = hudOrderTables[buffer][player];
    ClearOTagR(hudOrderingTable, 2);
#ifdef TM2_PORT_HUMANS
    actor = humanCarGet(human);
#else
    actor = humanCarGet(player);
#endif
    if (hudRadarVisible[player]) hudDisplayRadar(hudOrderingTable, player);
    if (gameGetMode() == 2) {
        if (HUD_LIVES_HERE(player)) {
            hudLivesText[7] = gameGetLives(HUD_LIVES_OF(human)) + 47;
            fontSetColor(2, 36, 150, 36);
            fontSpritePrintXY(2, 11, 222, &hudSplitVertLivesMode, hudSplitVertLivesSprites, (unsigned char *)hudLivesText, hudOrderingTable);
        }
        hudDrawTargetPanel(hudOrderingTable, player);
    }

    raw = actor->weapons.energy;
    width = raw / 10;
    kind = hudHealthLevel(width);
    width = width * 46 / 100;
    width = width <= 0 ? 1 : width > 46 ? 46 : width;
    if (width) {
        if (hudWeaponListVisible[player]) {
            setXY4(&REC.ammo, 112, 150, 158, 150, 112, 155, 158, 155);
            setXY4(&REC.ammo_frame, 111, 149, 159, 149, 111, 156, 159, 156);
        } else {
            setXY4(&REC.ammo, 112, 178, 158, 178, 112, 183, 158, 183);
            setXY4(&REC.ammo_frame, 111, 177, 159, 177, 111, 184, 159, 184);
        }
        REC.ammo.x1 = REC.ammo.x0 + width;
        REC.ammo.x3 = REC.ammo.x2 + width;
        UAdashHealthColor(kind, &REC.ammo);
        AddPrim(hudOrderingTable, &REC.ammo);
        AddPrim(hudOrderingTable, &REC.ammo_frame);
        AddPrim(hudOrderingTable, &REC.ammo_mode);
    }

    if (hudWeaponListVisible[player]) {
        for (i = 0; i < REC.weapon_count; i++) {
            amount = actor->weapons.ammo[i];
            if (amount <= 0) continue;
            if (actor->weapons.selected == i) setRGB0(&REC.weapon_icon[i], 0, 140, 10);
            else setRGB0(&REC.weapon_icon[i], 0, 0, 0);
            if (actor->weapons.drop_armed && i == 3) hudBlinkWeaponMarker(player, 3);
            setXY0(&REC.weapon_icon[i], 128, 163 + i * 8);
            AddPrim(hudOrderingTable, &REC.weapon_icon[i]);
            AddPrim(hudOrderingTable, &REC.weapon_icon_mode[i]);
            if (actor->weapons.selected == i) {
                setRGB0(&REC.digits[i].high, 0, 140, 10);
                setRGB0(&REC.digits[i].low, 0, 140, 10);
            } else {
                setRGB0(&REC.digits[i].high, 0, 0, 0);
                setRGB0(&REC.digits[i].low, 0, 0, 0);
            }
            if (actor->weapons.drop_armed && i == 3) hudBlinkWeaponDigits(player, 3);
            if (amount < 10) {
                setXY0(&REC.digits[i].low, 116, 162 + i * 8);
                hudDrawNumber(amount, &REC.digits[i].high + 1, REC.digit_mode_data, hudOrderingTable);
            } else {
                setXY0(&REC.digits[i].high, 112, 162 + i * 8);
                setXY0(&REC.digits[i].low, 116, 162 + i * 8);
                hudDrawNumber(amount, (char *)&REC + 0x380 + i * 40, REC.digit_mode_data, hudOrderingTable);
            }
            AddPrim(hudOrderingTable, &REC.digit_mode[i]);
        }
        AddPrim(hudOrderingTable, &REC.weapon_frame[0]);
        AddPrim(hudOrderingTable, &REC.weapon_frame[1]);
        AddPrim(hudOrderingTable, &REC.weapon_frame_end);
    } else {
        i = actor->weapons.selected;
        amount = actor->weapons.ammo[i];
        if (amount > 0) {
            setXY0(&REC.weapon_big[i], 123, 191);
            AddPrim(hudOrderingTable, &REC.weapon_big[i]);
            AddPrim(hudOrderingTable, &REC.weapon_big_mode[i]);
            setRGB0(&REC.digits[i].high, 0, 140, 10);
            setRGB0(&REC.digits[i].low, 0, 140, 10);
            if (actor->weapons.drop_armed && i == 3) {
                hudBlinkWeaponMarker(player, 3);
                hudBlinkWeaponDigits(player, 3);
            }
            if (amount < 10) {
                setXY0(&REC.digits[i].low, 116, 190);
                hudDrawNumber(amount, &REC.digits[i].high + 1, REC.digit_mode_data, hudOrderingTable);
            } else {
                setXY0(&REC.digits[i].high, 112, 190);
                setXY0(&REC.digits[i].low, 116, 190);
                hudDrawNumber(amount, (char *)&REC + 0x380 + i * 40, REC.digit_mode_data, hudOrderingTable);
            }
            AddPrim(hudOrderingTable, &REC.digit_mode[i]);
        }
    }

    width = actor->stats.hp * 100 / actor->stats.max_hp;
    kind = hudHealthLevel(width);
    width = width * 70 / 100;
    width = width <= 0 ? 1 : width > 70 ? 70 : width;
    if (width) {
        setXY4(&REC.health, 83, 20, 153, 20, 83, 30, 153, 30);
        REC.health.x1 = REC.health.x0 + width;
        REC.health.x3 = REC.health.x2 + width;
        UAdashHealthColor(kind, &REC.health);
        AddPrim(hudOrderingTable, &REC.health);
    }
    setXY4(&REC.health_frame, 82, 19, 154, 19, 82, 31, 154, 31);
    AddPrim(hudOrderingTable, &REC.health_frame);
    AddPrim(hudOrderingTable, &REC.health_mode);
    setXY0(&REC.lives_icon, 11, 205);
    AddPrim(hudOrderingTable, &REC.lives_icon);
    AddPrim(hudOrderingTable, &REC.lives_mode);
    amount = actor->weapons.turbo / 10;
    if (amount < 10) {
        setXY0(&REC.lives.low, 38, 204);
        hudDrawNumber(amount, &REC.lives.high + 1, REC.digit_mode_data, hudOrderingTable);
    } else {
        setXY0(&REC.lives.high, 34, 204);
        setXY0(&REC.lives.low, 38, 204);
        hudDrawNumber(amount, (char *)&REC + 0x6D8, REC.digit_mode_data, hudOrderingTable);
    }
    AddPrim(hudOrderingTable, &REC.lives_digit_mode);
    if (player == 0) {
        hudSplitDivider->x0 = 157;
        hudSplitDivider->y0 = 0;
        hudSplitDivider->x1 = 163;
        hudSplitDivider->y1 = 0;
        hudSplitDivider->x2 = 157;
        hudSplitDivider->y2 = 240;
        hudSplitDivider->x3 = 163;
        hudSplitDivider->y3 = 240;
        AddPrim(hudOrderingTable, (void *)hudSplitDivider);
    }
    DrawOTag(hudOrderingTable + 1);
}

/* @800F9D48: the same layout for the other split-screen view (coordinates only differ). */
extern "C" void __attribute__((section(".text_800F9D48"))) hudDrawSplitHorizontal(int player, int buffer)
{
    struct TM2HumanRecord *actor;
    int kind, width, i, amount, raw;
#ifdef TM2_PORT_HUMANS
    int human = TM2_LOCAL_HUMAN(player);   /* H1: the human on HUD slot `player`; none: no HUD */

    if (human < 0)
        return;
#endif

    hudOrderingTable = hudOrderTables[buffer][player];
    ClearOTagR(hudOrderingTable, 2);
#ifdef TM2_PORT_HUMANS
    actor = humanCarGet(human);
#else
    actor = humanCarGet(player);
#endif
    if (hudRadarVisible[player]) hudDisplayRadar(hudOrderingTable, player);
    if (gameGetMode() == 2) {
        if (HUD_LIVES_HERE(player)) {
            hudLivesText[7] = gameGetLives(HUD_LIVES_OF(human)) + 47;
            fontSetColor(2, 36, 150, 36);
            fontSpritePrintXY(2, 11, 107, &hudSplitHorzLivesMode, hudSplitHorzLivesSprites, (unsigned char *)hudLivesText, hudOrderingTable);
        }
        hudDrawTargetPanel(hudOrderingTable, player);
    }

    raw = actor->weapons.energy;
    width = raw / 10;
    kind = hudHealthLevel(width);
    width = width * 46 / 100;
    width = width <= 0 ? 1 : width > 46 ? 46 : width;
    if (width) {
        if (hudWeaponListVisible[player]) {
            setXY4(&REC.ammo, 271, 35, 317, 35, 271, 40, 317, 40);
            setXY4(&REC.ammo_frame, 270, 34, 318, 34, 270, 41, 318, 41);
        } else {
            setXY4(&REC.ammo, 271, 63, 317, 63, 271, 68, 317, 68);
            setXY4(&REC.ammo_frame, 270, 62, 318, 62, 270, 69, 318, 69);
        }
        REC.ammo.x1 = REC.ammo.x0 + width;
        REC.ammo.x3 = REC.ammo.x2 + width;
        UAdashHealthColor(kind, &REC.ammo);
        AddPrim(hudOrderingTable, &REC.ammo);
        AddPrim(hudOrderingTable, &REC.ammo_frame);
        AddPrim(hudOrderingTable, &REC.ammo_mode);
    }

    if (hudWeaponListVisible[player]) {
        for (i = 0; i < REC.weapon_count; i++) {
            amount = actor->weapons.ammo[i];
            if (amount <= 0) continue;
            if (actor->weapons.selected == i) setRGB0(&REC.weapon_icon[i], 0, 140, 10);
            else setRGB0(&REC.weapon_icon[i], 0, 0, 0);
            if (actor->weapons.drop_armed && i == 3) hudBlinkWeaponMarker(player, 3);
            setXY0(&REC.weapon_icon[i], 288, 48 + i * 8);
            AddPrim(hudOrderingTable, &REC.weapon_icon[i]);
            AddPrim(hudOrderingTable, &REC.weapon_icon_mode[i]);
            if (actor->weapons.selected == i) {
                setRGB0(&REC.digits[i].high, 0, 140, 10);
                setRGB0(&REC.digits[i].low, 0, 140, 10);
            } else {
                setRGB0(&REC.digits[i].high, 0, 0, 0);
                setRGB0(&REC.digits[i].low, 0, 0, 0);
            }
            if (actor->weapons.drop_armed && i == 3) hudBlinkWeaponDigits(player, 3);
            if (amount < 10) {
                setXY0(&REC.digits[i].low, 276, 47 + i * 8);
                hudDrawNumber(amount, &REC.digits[i].high + 1, REC.digit_mode_data, hudOrderingTable);
            } else {
                setXY0(&REC.digits[i].high, 272, 47 + i * 8);
                setXY0(&REC.digits[i].low, 276, 47 + i * 8);
                hudDrawNumber(amount, (char *)&REC + 0x380 + i * 40, REC.digit_mode_data, hudOrderingTable);
            }
            AddPrim(hudOrderingTable, &REC.digit_mode[i]);
        }
        AddPrim(hudOrderingTable, &REC.weapon_frame[0]);
        AddPrim(hudOrderingTable, &REC.weapon_frame[1]);
        AddPrim(hudOrderingTable, &REC.weapon_frame_end);
    } else {
        i = actor->weapons.selected;
        amount = actor->weapons.ammo[i];
        if (amount > 0) {
            setXY0(&REC.weapon_big[i], 283, 76);
            AddPrim(hudOrderingTable, &REC.weapon_big[i]);
            AddPrim(hudOrderingTable, &REC.weapon_big_mode[i]);
            setRGB0(&REC.digits[i].high, 0, 140, 10);
            setRGB0(&REC.digits[i].low, 0, 140, 10);
            if (actor->weapons.drop_armed && i == 3) {
                hudBlinkWeaponMarker(player, 3);
                hudBlinkWeaponDigits(player, 3);
            }
            if (amount < 10) {
                setXY0(&REC.digits[i].low, 276, 75);
                hudDrawNumber(amount, &REC.digits[i].high + 1, REC.digit_mode_data, hudOrderingTable);
            } else {
                setXY0(&REC.digits[i].high, 272, 75);
                setXY0(&REC.digits[i].low, 276, 75);
                hudDrawNumber(amount, (char *)&REC + 0x380 + i * 40, REC.digit_mode_data, hudOrderingTable);
            }
            AddPrim(hudOrderingTable, &REC.digit_mode[i]);
        }
    }

    width = actor->stats.hp * 100 / actor->stats.max_hp;
    kind = hudHealthLevel(width);
    width = width * 70 / 100;
    width = width <= 0 ? 1 : width > 70 ? 70 : width;
    if (width) {
        setXY4(&REC.health, 239, 18, 309, 18, 239, 28, 309, 28);
        REC.health.x1 = REC.health.x0 + width;
        REC.health.x3 = REC.health.x2 + width;
        UAdashHealthColor(kind, &REC.health);
        AddPrim(hudOrderingTable, &REC.health);
    }
    setXY4(&REC.health_frame, 238, 17, 310, 17, 238, 29, 310, 29);
    AddPrim(hudOrderingTable, &REC.health_frame);
    AddPrim(hudOrderingTable, &REC.health_mode);
    setXY0(&REC.lives_icon, 11, 90);
    AddPrim(hudOrderingTable, &REC.lives_icon);
    AddPrim(hudOrderingTable, &REC.lives_mode);
    amount = actor->weapons.turbo / 10;
    if (amount < 10) {
        setXY0(&REC.lives.low, 38, 89);
        hudDrawNumber(amount, &REC.lives.high + 1, REC.digit_mode_data, hudOrderingTable);
    } else {
        setXY0(&REC.lives.high, 34, 89);
        setXY0(&REC.lives.low, 38, 89);
        hudDrawNumber(amount, (char *)&REC + 0x6D8, REC.digit_mode_data, hudOrderingTable);
    }
    AddPrim(hudOrderingTable, &REC.lives_digit_mode);
    if (player == 0) {
        hudSplitDivider->x0 = 0;
        hudSplitDivider->y0 = 117;
        hudSplitDivider->x1 = 320;
        hudSplitDivider->y1 = 117;
        hudSplitDivider->x2 = 0;
        hudSplitDivider->y2 = 123;
        hudSplitDivider->x3 = 320;
        hudSplitDivider->y3 = 123;
        AddPrim(hudOrderingTable, (void *)hudSplitDivider);
    }
    DrawOTag(hudOrderingTable + 1);
}
