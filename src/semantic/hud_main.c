#include "semantic/cars.h"
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map */
#endif

union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
struct TM2HudText30 { union TM2FontDrawMode mode; union TM2FontSprite sprites[30]; };
struct TM2HudText10 { union TM2FontDrawMode mode; union TM2FontSprite sprites[10]; };
typedef char hud_text30_size[(sizeof(struct TM2HudText30)==612)?1:-1];
typedef char hud_text10_size[(sizeof(struct TM2HudText10)==212)?1:-1];

/* libgpu-style primitives (tag word, then r0 g0 b0 code). */
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0,x1,y1,x2,y2,x3,y3; } TM2PolyF4;
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0; unsigned char r1,g1,b1,p1; short x1,y1;
    unsigned char r2,g2,b2,p2; short x2,y2; unsigned char r3,g3,b3,p3; short x3,y3; } TM2PolyG4;
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0; unsigned char u0,v0; unsigned short clut;
    short w,h; } TM2Sprt;
typedef struct { unsigned int tag; unsigned int code[2]; } TM2DrMode;
typedef struct { TM2Sprt high, low; } TM2HudDigits;

/* @801828F0: one 1900-byte HUD packet record per player. */
struct TM2HudRecord {
    TM2PolyF4 top; TM2DrMode top_mode;              /* +000 */
    TM2PolyG4 health; TM2PolyF4 health_back; TM2DrMode health_mode;  /* +024 */
    TM2PolyG4 g1; TM2PolyF4 f1; TM2DrMode m1;       /* +06C */
    TM2PolyG4 turbo; TM2PolyF4 turbo_back; TM2DrMode turbo_mode;     /* +0B4 */
    short weapon_count; short pad0FE;               /* +0FC */
    TM2Sprt marker[10]; TM2DrMode marker_mode[10];  /* +100 */
    TM2Sprt selected[10]; TM2DrMode selected_mode[10];  /* +240 */
    TM2HudDigits digits[10]; TM2DrMode digit_mode[10];  /* +380 */
    TM2PolyF4 map; TM2PolyF4 frame; TM2DrMode map_mode;  /* +588 */
    unsigned char pad5C4[12];
    unsigned char block5D0[0x50];                   /* +5D0 */
    unsigned char font[0x50];                       /* +620: digit images */
    unsigned char block670[0x3C];                   /* +670 */
    TM2DrMode mode6AC;                              /* +6AC */
    TM2Sprt lap; TM2DrMode lap_mode;                /* +6B8 */
    TM2HudDigits lap_digits; TM2DrMode lap_digit_mode;  /* +6D8 */
    TM2PolyF4 mirror[4];                            /* +70C */
    unsigned char tail[1900-0x76C];
};
typedef char hud_record_size[(sizeof(struct TM2HudRecord)==1900)?1:-1];

extern struct TM2HudRecord hudPlayerRecords[TM2_MAX_HUDS];
/* [buffer][player][2]: more players change the buffer stride */
extern unsigned int hudOrderTables[2][TM2_MAX_HUDS][2];
extern unsigned int *hudOrderingTable;
extern unsigned char hudRadarVisible[4], hudWeaponListVisible[4];
extern struct TM2HudText30 hudMessageTextMode;
extern struct TM2HudText10 hudLivesTextMode;
extern union TM2FontSprite hudMessageTextSprites[30], hudLivesTextSprites[10];
extern char *hudMessageStrs[37], *hudLivesText;
extern int hudMessageCurrent[5];
extern u32 *ClearOTagR(unsigned int *, int);
extern int hudMessageUpdate(void),hudHealthLevel(int);
extern void fontSetColor(int,int,int,int),fontResetColor(int);
extern int fontSpritePrintCenteredXY(int,int,int,union TM2FontDrawMode *,union TM2FontSprite *,const unsigned char *,unsigned int *);
extern void AddPrim(unsigned int *, void *);
extern void hudDisplayRadar(unsigned int *,int),hudDrawTargetPanel(unsigned int *,int);
extern void UAdashHealthColor(int,void *);
extern void hudBlinkWeaponMarker(int,int),hudBlinkWeaponDigits(int,int);
extern void hudDrawNumber(int, void *, void *, unsigned int *);
extern int carGetDisplaySpeed(int),rtGetRearViewActive(void);
/* Retail uses the result unmasked here, as if gameGetLives returned int; it returns u8
 * (misc_one_liners.c), and hud_split.cpp sees that prototype. */
extern int gameGetLives(int);
extern void hudPrintNumber3(int,int,int,void *,void *,void *,unsigned int *);
extern void UAdashDrawRearViewMirror(unsigned int *),DrawOTag(unsigned int *);

#define R hudPlayerRecords[player]
#define setXY4(p,_x0,_y0,_x1,_y1,_x2,_y2,_x3,_y3) \
    ((p)->x0=(_x0),(p)->y0=(_y0),(p)->x1=(_x1),(p)->y1=(_y1), \
     (p)->x2=(_x2),(p)->y2=(_y2),(p)->x3=(_x3),(p)->y3=(_y3))
#define setXY0(p,_x0,_y0) ((p)->x0=(_x0),(p)->y0=(_y0))
#define setRGB0(p,r,g,b) ((p)->r0=(r),(p)->g0=(g),(p)->b0=(b))
#define CLAMP_WIDTH(w,max) ((w)<=0 ? 1 : ((w)>(max) ? (max) : (w)))

/* HUD slot `player` shows HUD_HUMAN's car: with TM2_PORT_HUMANS (H1) the human on that local slot (none: no HUD),
 * retail the human of the same index. */
#ifdef TM2_PORT_HUMANS
#define HUD_HUMAN human
#else
#define HUD_HUMAN player
#endif

/* @800F8370. Full single-view HUD caller. Reviewed player/buffer domain 0..1,
 * weapon slots/count 0..9/0..10, and non-faulting health divisor. */
void __attribute__((section(".text_800F8370"))) hudDrawSingleView(int player,int buffer)
{
    struct TM2HumanRecord *actor;
    int kind,width,i,amount,slot,value;
#ifdef TM2_PORT_HUMANS
    int human=TM2_LOCAL_HUMAN(player);
    if (human<0) return;
#endif
    hudOrderingTable=hudOrderTables[buffer][player];
    ClearOTagR(hudOrderingTable,2);
    actor=humanCarGet(HUD_HUMAN);
    hudMessageUpdate();
    fontSetColor(2,36,150,36);
    fontSpritePrintCenteredXY(2,147,30,&hudMessageTextMode.mode,hudMessageTextSprites,
                             (unsigned char *)hudMessageStrs[hudMessageCurrent[0]],hudOrderingTable);
    fontResetColor(2);
    AddPrim(hudOrderingTable,&R.top); AddPrim(hudOrderingTable,&R.top_mode);
    if (hudRadarVisible[player]) hudDisplayRadar(hudOrderingTable,player);
    hudDrawTargetPanel(hudOrderingTable,player);

    width=actor->stats.hp*100/actor->stats.max_hp;
    kind=hudHealthLevel(width);
    width=width*70/100;
    width=CLAMP_WIDTH(width,70);
    if (width) {
        setXY4(&R.health,10,215,80,215,10,225,80,225);
        R.health.x1=R.health.x0+width; R.health.x3=R.health.x2+width;
        UAdashHealthColor(kind,&R.health);
        AddPrim(hudOrderingTable,&R.health);
        AddPrim(hudOrderingTable,&R.health_back);
        AddPrim(hudOrderingTable,&R.health_mode);
    }

    width=actor->weapons.energy*100/1000;
    kind=hudHealthLevel(width);
    width=width*46/100;
    width=CLAMP_WIDTH(width,46);
    if (width) {
        if (hudWeaponListVisible[player]) {
            setXY4(&R.turbo,271,150,317,150,271,155,317,155);
            setXY4(&R.turbo_back,270,149,318,149,270,156,318,156);
        } else {
            setXY4(&R.turbo,271,178,317,178,271,183,317,183);
            setXY4(&R.turbo_back,270,177,318,177,270,184,318,184);
        }
        R.turbo.x1=R.turbo.x0+width; R.turbo.x3=R.turbo.x2+width;
        UAdashHealthColor(kind,&R.turbo);
        AddPrim(hudOrderingTable,&R.turbo);
        AddPrim(hudOrderingTable,&R.turbo_back);
        AddPrim(hudOrderingTable,&R.turbo_mode);
    }

    if (hudWeaponListVisible[player]) {
        for (slot=0;slot<R.weapon_count;slot++) {
            amount=actor->weapons.ammo[slot];
            if (amount<=0) continue;
            if (actor->weapons.selected==slot) setRGB0(&R.marker[slot],0,140,10);
            else setRGB0(&R.marker[slot],0,0,0);
            if (actor->weapons.drop_armed && slot==3) hudBlinkWeaponMarker(player,3);
            setXY0(&R.marker[slot],288,163+slot*8);
            AddPrim(hudOrderingTable,&R.marker[slot]);
            AddPrim(hudOrderingTable,&R.marker_mode[slot]);
            if (actor->weapons.selected==slot) {
                setRGB0(&R.digits[slot].high,0,140,10); setRGB0(&R.digits[slot].low,0,140,10);
            } else {
                setRGB0(&R.digits[slot].high,0,0,0); setRGB0(&R.digits[slot].low,0,0,0);
            }
            if (actor->weapons.drop_armed && slot==3) hudBlinkWeaponDigits(player,3);
            if (amount<10) {
                setXY0(&R.digits[slot].low,276,162+slot*8);
                hudDrawNumber(amount,&R.digits[slot].low,R.font,hudOrderingTable);
            } else {
                setXY0(&R.digits[slot].high,272,162+slot*8);
                setXY0(&R.digits[slot].low,276,162+slot*8);
                hudDrawNumber(amount,&R.digits[slot].high,R.font,hudOrderingTable);
            }
            AddPrim(hudOrderingTable,&R.digit_mode[slot]);
        }
        AddPrim(hudOrderingTable,&R.map);
        AddPrim(hudOrderingTable,&R.map+1);
        AddPrim(hudOrderingTable,&R.map_mode);
    } else {
        slot=actor->weapons.selected;
        amount=actor->weapons.ammo[slot];
        if (amount>0) {
            setXY0(&R.selected[slot],283,191);
            AddPrim(hudOrderingTable,&R.selected[slot]);
            AddPrim(hudOrderingTable,&R.selected_mode[slot]);
            setRGB0(&R.digits[slot].high,0,140,10); setRGB0(&R.digits[slot].low,0,140,10);
            if (actor->weapons.drop_armed && slot==3) { hudBlinkWeaponMarker(player,3); hudBlinkWeaponDigits(player,3); }
            if (amount<10) {
                setXY0(&R.digits[slot].low,276,190);
                hudDrawNumber(amount,&R.digits[slot].low,R.font,hudOrderingTable);
            } else {
                setXY0(&R.digits[slot].high,272,190);
                setXY0(&R.digits[slot].low,276,190);
                hudDrawNumber(amount,&R.digits[slot].high,R.font,hudOrderingTable);
            }
            AddPrim(hudOrderingTable,&R.digit_mode[slot]);
        }
    }
    AddPrim(hudOrderingTable,&R.lap); AddPrim(hudOrderingTable,&R.lap_mode);
    amount=(short)(actor->weapons.turbo/10);
    if (amount<10) {
        setXY0(&R.lap_digits.low,248,199);
        hudDrawNumber(amount,&R.lap_digits.low,R.font,hudOrderingTable);
    } else {
        setXY0(&R.lap_digits.high,244,199);
        setXY0(&R.lap_digits.low,248,199);
        hudDrawNumber(amount,&R.lap_digits.high,R.font,hudOrderingTable);
    }
    AddPrim(hudOrderingTable,&R.lap_digit_mode);
    value=carGetDisplaySpeed(HUD_HUMAN);
    if (value<0) value=-value;
    hudPrintNumber3(value,216,223,R.block670,R.block5D0,&R.mode6AC,hudOrderingTable);
    value=gameGetLives(HUD_HUMAN);
    hudLivesText[7]=value+47;
    fontSetColor(2,36,150,36);
    fontSpritePrintCenteredXY(2,50,212,&hudLivesTextMode.mode,hudLivesTextSprites,
                             (unsigned char *)hudLivesText,hudOrderingTable);
    if (rtGetRearViewActive()) UAdashDrawRearViewMirror(hudOrderingTable);
    DrawOTag(hudOrderingTable+1);
}

/* @800FBB0C: four fixed rear-view primitives occupy the last 96 bytes of
 * record zero. */
void __attribute__((section(".text_800FBB0C"))) UAdashDrawRearViewMirror(unsigned int *ot)
{
    AddPrim(ot,&hudPlayerRecords[0].mirror[0]);
    AddPrim(ot,&hudPlayerRecords[0].mirror[1]);
    AddPrim(ot,&hudPlayerRecords[0].mirror[2]);
    AddPrim(ot,&hudPlayerRecords[0].mirror[3]);
}
