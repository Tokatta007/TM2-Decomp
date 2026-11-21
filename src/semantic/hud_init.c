typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0,x1,y1,x2,y2,x3,y3; } TM2PolyF4;
typedef struct { unsigned int tag; unsigned char r0,g0,b0,code; short x0,y0; unsigned char r1,g1,b1,p1; short x1,y1;
    unsigned char r2,g2,b2,p2; short x2,y2; unsigned char r3,g3,b3,p3; short x3,y3; } TM2PolyG4;
typedef struct { unsigned int tag; unsigned int code[2]; } TM2DrMode;
/* The parts of a 1900-byte HUD record this unit sets up. */
struct TM2HudRecord {
    TM2PolyF4 top; TM2DrMode top_mode;
    TM2PolyG4 g0; TM2PolyF4 f0; TM2DrMode m0;
    TM2PolyG4 g1; TM2PolyF4 f1; TM2DrMode m1;
    TM2PolyG4 g2; TM2PolyF4 f2; TM2DrMode m2;
    unsigned char gap[0x588-0xFC];
    TM2PolyF4 map; TM2PolyF4 frame; TM2DrMode map_mode;
    unsigned char rest[0x70C-0x5C4];
    TM2PolyF4 mirror[4];
    unsigned char tail[1900-0x76C];
};
#include "semantic/players.h"
extern struct TM2HudRecord hudPlayerRecords[TM2_MAX_HUDS];
extern TM2PolyF4 hudSplitDivider;
extern int viewGetSplitMode(void);
extern void SetSemiTrans(void *, int);
extern void SetDrawMode(void *, int, int, int tpage, void *);
extern void sub_80144CF4(void *);
void uadashInitRearViewMirror(void);
typedef char hud_record_size[(sizeof(struct TM2HudRecord)==1900)?1:-1];

typedef struct { unsigned addr:24; unsigned len:8; unsigned char r0,g0,b0,code; } TM2Tag;
#define SETLEN(p,n) (((TM2Tag *)(p))->len=(unsigned char)(n))
#define SETCODE(p,c) (((TM2Tag *)(p))->code=(unsigned char)(c))
#define F4(p) (SETLEN(p,5), SETCODE(p,0x28))
#define G4(p) (SETLEN(p,8), SETCODE(p,0x38))
#define BOX(p,a,b,c,d) ((p)->x0=(a),(p)->y0=(b),(p)->x1=(c),(p)->y1=(b), \
                        (p)->x2=(a),(p)->y2=(d),(p)->x3=(c),(p)->y3=(d))
#define COL(p,r,g,b) ((p)->r0=(r),(p)->g0=(g),(p)->b0=(b))
#define R hudPlayerRecords[i]

/* @800FB0B0. Every record slot (TM2_MAX_HUDS, 2 in retail) is initialized even in single-view mode.
 * Each mode query is a real call; the mirror helper always owns row zero. */
void __attribute__((section(".text_800FB0B0"))) hudInitPanels(void)
{
    int i;
    if (viewGetSplitMode()!=0) {
        sub_80144CF4(&hudSplitDivider);
        COL(&hudSplitDivider,0,0,0);
        SETCODE(&hudSplitDivider,((TM2Tag *)&hudSplitDivider)->code&~2);
    }
    for (i=0;i<TM2_MAX_HUDS;++i) {
        G4(&R.g0); SetSemiTrans(&R.g0,1);
        F4(&R.f0); SetSemiTrans(&R.f0,1);
        BOX(&R.f0,9,214,81,226); COL(&R.f0,10,10,10);
        SetDrawMode(&R.m0,0,0,0,0);
        G4(&R.g1); SetSemiTrans(&R.g1,1);
        F4(&R.f1); SetSemiTrans(&R.f1,1);
        COL(&R.f1,10,10,10);
        SetDrawMode(&R.m1,0,0,0,0);
        F4(&R.top); SetSemiTrans(&R.top,1);
        BOX(&R.top,84,19,214,34); COL(&R.top,10,10,10);
        SetDrawMode(&R.top_mode,0,0,0,0);
        G4(&R.g2); SetSemiTrans(&R.g2,1);
        F4(&R.f2); SetSemiTrans(&R.f2,1);
        BOX(&R.f2,270,149,318,156); COL(&R.f2,10,10,10);
        SetDrawMode(&R.m2,0,0,0,0);
        F4(&R.map); F4(&R.frame);
        SetSemiTrans(&R.map,1); SetSemiTrans(&R.map+1,1);
        COL(&R.map,0,100,21); COL(&R.frame,17,38,5);
        SetDrawMode(&R.map_mode,0,0,0,0);
        if (viewGetSplitMode()==0) {
            BOX(&R.map,273,160,315,226); BOX(&R.frame,270,157,318,229);
            uadashInitRearViewMirror();
        } else if (viewGetSplitMode()==1) {
            BOX(&R.map,113,160,155,226); BOX(&R.frame,110,157,158,229);
        } else {
            BOX(&R.map,273,45,315,111); BOX(&R.frame,270,42,318,114);
        }
    }
}

#define M hudPlayerRecords[0].mirror

/* @800FB8AC. Four opaque black bars framing the rear-view mirror in record
 * zero. */
void __attribute__((section(".text_800FB8AC"))) uadashInitRearViewMirror(void)
{
    TM2PolyF4 *p = M;
    F4(&M[0]); BOX(&M[0],209,18,314,20); COL(&M[0],0,0,0);
    SetSemiTrans(p,0);
    p++;
    F4(&M[1]); BOX(&M[1],209,50,314,52); COL(&M[1],0,0,0);
    SetSemiTrans(p,0);
    p++;
    F4(&M[2]); BOX(&M[2],209,20,211,50); COL(&M[2],0,0,0);
    SetSemiTrans(p,0);
    p++;
    F4(&M[3]); BOX(&M[3],312,20,314,50); COL(&M[3],0,0,0);
    SetSemiTrans(p,0);
}
