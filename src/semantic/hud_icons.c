/* libgpu-style primitives (tag word, then r0 g0 b0 code). */
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0,x1,y1,x2,y2,x3,y3; } TM2PolyF4;
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0; unsigned char r1,g1,b1,p1; short x1,y1;
    unsigned char r2,g2,b2,p2; short x2,y2; unsigned char r3,g3,b3,p3; short x3,y3; } TM2PolyG4;
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0; unsigned char u0,v0; unsigned short clut;
    short w,h; } TM2Sprt;
typedef struct { unsigned int tag; unsigned int code[2]; } TM2DrMode;
typedef struct { unsigned addr:24; unsigned len:8; unsigned char r0,g0,b0,code; } TM2Tag;
/* grutilsParse2DSprite output: one 8-byte record per sprite. */
typedef struct { unsigned char u,v,w,h; unsigned short tpage,clut; } TM2HudIcon;

/* @801828F0: one 1900-byte HUD packet record per player (see hud_main.c). */
struct TM2HudRecord {
    TM2PolyF4 top; TM2DrMode top_mode;              /* +000 */
    TM2PolyG4 health; TM2PolyF4 health_back; TM2DrMode health_mode;  /* +024 */
    TM2PolyG4 g1; TM2PolyF4 f1; TM2DrMode m1;       /* +06C */
    TM2PolyG4 turbo; TM2PolyF4 turbo_back; TM2DrMode turbo_mode;     /* +0B4 */
    short weapon_count; short pad0FE;               /* +0FC */
    TM2Sprt marker[10]; TM2DrMode marker_mode[10];  /* +100 */
    TM2Sprt selected[10]; TM2DrMode selected_mode[10];  /* +240 */
    TM2Sprt digits[10][2]; TM2DrMode digit_mode[10];    /* +380 */
    TM2PolyF4 map; TM2PolyF4 frame; TM2DrMode map_mode;  /* +588 */
    unsigned char pad5C4[12];
    unsigned char block5D0[0x50];                   /* +5D0 */
    TM2HudIcon font[10];                            /* +620: digit images */
    unsigned char block670[0x3C];                   /* +670 */
    TM2DrMode mode6AC;                              /* +6AC */
    TM2Sprt lap; TM2DrMode lap_mode;                /* +6B8 */
    TM2Sprt lap_digits[2]; TM2DrMode lap_digit_mode;    /* +6D8 */
    TM2PolyF4 mirror[4];                            /* +70C */
    unsigned char tail[1900-0x76C];
};
typedef char hud_record_size[(sizeof(struct TM2HudRecord)==1900)?1:-1];
typedef char hud_icon_size[(sizeof(TM2HudIcon)==8)?1:-1];

#include "semantic/players.h"
extern struct TM2HudRecord hudPlayerRecords[TM2_MAX_HUDS];
extern int viewGetSplitMode(void);
extern int grutilsParse2DSprite(void *, void *, unsigned int);
extern void SetShadeTex(void *, int);
extern void SetSemiTrans(void *, int);
extern void SetDrawMode(void *, int, int, int tpage, void *);
extern void hudInitPanels(void);

#define SETLEN(p,n) (((TM2Tag *)(p))->len=(unsigned char)(n))
#define SETCODE(p,c) (((TM2Tag *)(p))->code=(unsigned char)(c))
#define setSprt(p) (SETLEN(p,4), SETCODE(p,0x64))
#define setRGB0(p,r,g,b) ((p)->r0=(r),(p)->g0=(g),(p)->b0=(b))
#define setXY0(p,_x0,_y0) ((p)->x0=(_x0),(p)->y0=(_y0))
#define ICON(p,q) (setSprt(p), (p)->clut=(q)->clut, (p)->u0=(q)->u, (p)->v0=(q)->v, \
                   (p)->w=(q)->w, (p)->h=(q)->h)
#define R hudPlayerRecords[player]

/* @800FAA0C. The four asset children are at byte offsets 24, 28, 36,
 * and 40. The parser's eight-byte records share one ten-record workspace.
 * Other HUD packet fields are deliberately preserved. */
void __attribute__((section(".text_800FAA0C"))) UAdashInitDashboardIcons(void *arg0)
{
    TM2HudIcon local[10];
    unsigned char *asset;
    short player=1;
    TM2Sprt *p;
    TM2HudIcon *q;
    int i, j;
    asset=(unsigned char *)arg0;
    if (!asset || asset[0]!=1 || asset[20]<5) return;
    if (viewGetSplitMode()!=0) player=2;
    while (--player!=-1) {
        R.weapon_count=grutilsParse2DSprite(*(void **)(asset+24),local,10);
        for (i=0;i<R.weapon_count;++i) {
            p=&R.marker[i];
            q=&local[i];
            ICON(p,q);
            SetShadeTex(p,0);
            SetSemiTrans(p,0);
            SetDrawMode(&R.marker_mode[i],0,0,q->tpage,0);
        }
        grutilsParse2DSprite(*(void **)(asset+28),local,1);
        p=&R.lap;
        ICON(p,&local[0]);
        setXY0(p,221,200);
        setRGB0(p,36,150,36);
        SetShadeTex(p,0);
        SetSemiTrans(p,0);
        SetDrawMode(&R.lap_mode,0,0,local[0].tpage,0);
        grutilsParse2DSprite(*(void **)(asset+36),R.font,10);
        for (i=0;i<R.weapon_count;++i) {
            for (j=0;j<2;++j) {
                setSprt(&R.digits[i][j]);
                R.digits[i][j].clut=R.font[0].clut;
                setRGB0(&R.digits[i][j],36,150,36);
                SetSemiTrans(&R.digits[i][j],1);
                SetShadeTex(&R.digits[i][j],0);
            }
            SetDrawMode(&R.digit_mode[i],0,0,R.font[0].tpage,0);
        }
        for (i=0;i<2;++i) {
            setSprt(&R.lap_digits[i]);
            R.lap_digits[i].clut=R.font[0].clut;
            setRGB0(&R.lap_digits[i],36,150,36);
            SetSemiTrans(&R.lap_digits[i],1);
            SetShadeTex(&R.lap_digits[i],0);
        }
        SetDrawMode(&R.lap_digit_mode,0,0,R.font[0].tpage,0);
        R.weapon_count=grutilsParse2DSprite(*(void **)(asset+40),local,10);
        for (i=0;i<R.weapon_count;++i) {
            p=&R.selected[i];
            q=&local[i];
            ICON(p,q);
            setRGB0(&R.selected[i],128,128,128);
            SetShadeTex(p,0);
            SetSemiTrans(p,0);
            SetDrawMode(&R.selected_mode[i],0,0,q->tpage,0);
        }
    }
    hudInitPanels();
}
