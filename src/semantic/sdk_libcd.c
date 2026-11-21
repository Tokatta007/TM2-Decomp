/* PsyQ SDK LIBCD (TM2 links the December 1995 libcd: bios.c v1.71), reconstructed C.
 * See docs/SDK_SOURCES.md and THIRD_PARTY.md.
 *
 * Sources:
 *  - bios.c functions: adapted from the LSD decomp (lsddecomp2), src/psyq/libcd_bios.c,
 *    dedicated to the public domain under CC0 1.0; getintr, CD_vol, CD_shell, CD_flush,
 *    CD_initvol, CD_initintr, cd_read_retry, CD_readm, CD_readsync, CD_getsector, cb_read
 *    follow it (nearly) verbatim. CD_sync, CD_ready, CD_cw, CD_datasync, callback and
 *    CD_init follow sotn-decomp src/main/psxsdk/libcd/bios.c (MIT) instead.
 *    (cd_read_retry addresses the read-state block D_80176E78[] by index, so cc1
 *    relates the field addresses to one base register as retail does.)
 *  - CdInit: loop shape from psyz decomp/src/libcd/event.c (MIT).
 *  - Everything else: adapted from sotn-decomp src/main/psxsdk/libcd/ (MIT):
 *    sys.c, iso9660.c, cdrom.c, cdread2.c, event.c, c_002.c, c_004.c, c_005.c, c_007.c,
 *    c_008.c, c_009.c, c_010.c, c_011.c (StCdInterrupt, mem2mem). dma_execute has no
 *    C source there (INCLUDE_ASM); it is written from the retail disassembly.
 * SPDX-License-Identifier: MIT AND CC0-1.0
 *
 * Library data is not defined here: it is declared extern by its retail address
 * (D_XXXXXXXX); readable SDK names are #defines over those symbols. Recipe:
 * psyq3.3 cc1 (GCC 2.6.0), aspsx 2.21, gp_size 0, no -fvolatile. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef void (*CdlCB)(u8 status, u8 *result);
typedef CdlCB CdIntrCallback;

typedef struct { u8 minute, second, sector, track; } CdlLOC;

typedef struct { u32 madr, bcr, chcr, pad; } DmaCh;

typedef struct { u8 sync, ready, c; } CD_intr;

typedef struct {
    u16 id;
    u16 type;
    u16 secCount;
    u16 nSectors;
    u32 frameCount;
    u32 frameSize;
    u16 width;
    u16 height;
    u32 dummy1;
    u32 dummy2;
    CdlLOC loc;
} StHEADER;

typedef union {
    s32 addr;
    struct {
        u8 unk0;
        u8 unk1;
        u8 unk2;
        u8 unk3;
    } i;
} LBA;

typedef struct {
    s32 unk0;       /* directory number */
    s32 unk4;       /* parent directory number */
    LBA unk8;       /* extent */
    char unkC[0x20];
} CdlDIR;

typedef struct {
    CdlLOC pos;
    u32 size;
    char name[16];
} CdlFILE;

#define NULL 0

extern CdlCB D_80176B98;            /* CD_cbsync */
extern CdlCB D_80176B9C;            /* CD_cbready */
extern CdlCB D_80176BA0;            /* CD_cbread */
extern s32 D_80176BA4;              /* CD_debug */
extern s32 D_80176BA8;              /* CD_status */
extern s32 D_80176BAC;              /* CD_status1 */
extern s32 D_80176BB0;              /* CD_nopen */
extern u8 D_80176BB4[4];            /* CD_pos */
extern u8 D_80176BB8;               /* CD_mode */
extern u8 cdLastCom;               /* CD_com */
extern char *D_80176BBC[];          /* CD_comstr */
extern char *D_80176C3C[];          /* CD_intstr */
extern s32 D_80176C5C[];            /* command completes on a second interrupt */
extern s32 D_80176CDC[];            /* command clears ready state; +0x40: parameter count */
extern s32 D_80176D5C[];            /* acknowledge updates CD_status */
extern s32 D_80176DDC[];            /* command needs parameters */
extern volatile u8 *D_80176E5C;     /* 0x1F801800 */
extern volatile u8 *D_80176E60;     /* 0x1F801801 */
extern volatile u8 *D_80176E64;     /* 0x1F801802 */
extern volatile u8 *D_80176E68;     /* 0x1F801803 */
extern volatile s32 *D_80176E6C;    /* 0x1F801020 common delay */
extern volatile u16 *D_80176E70;    /* 0x1F801C00 SPU */
extern volatile CD_intr D_80176E74;  /* Intr: sync, ready, c */
#define IntrB ((u8 *)&D_80176E74)
extern volatile u8 D_80176E75;
extern volatile u8 D_80176E76;
extern s32 D_80176E78[10];          /* read state: retry count ... */
extern s32 D_80176E7C;              /* sectors */
extern s32 D_80176E80;              /* buffer */
extern volatile s32 D_80176E84;     /* current buffer */
extern volatile s32 D_80176E88;     /* mode */
extern volatile s32 D_80176E8C;     /* sector size (words) */
extern volatile s32 D_80176E90;     /* sectors left */
extern s32 D_80176E94;              /* last vsync */
extern CdlCB D_80176E98;            /* saved cbsync */
extern CdlCB D_80176E9C;            /* saved cbready */
extern s32 D_80176EA0;              /* shell-open count seen */
extern u8 D_80176EA4[];             /* SeekP parameter */
extern u8 D_80176EA8[];             /* init struct */
extern volatile s32 *D_80176EC0;    /* 0x1F801018 */
extern volatile s32 *D_80176EC4;    /* 0x1F8010F0 DPCR */
extern volatile s32 *D_80176EC8;    /* 0x1F8010B0 */
extern volatile s32 *D_80176ECC;    /* 0x1F8010B4 */
extern volatile s32 *D_80176ED0;    /* 0x1F8010B8 */
extern u8 D_801914E8[3][8];         /* Result */
extern s32 D_80191500;              /* Alarm */
extern s32 D_80191504;
extern char *D_80191508;
extern char D_800D05DC[];
extern char D_800D05EC[];
extern char D_800D0608[];
extern char D_800D0614[];
extern char D_800D0630[];
extern char D_800D0644[];
extern char D_800D0664[];
extern char D_800D066C[];
extern char D_800D0678[];
extern char D_800D0680[];
extern char D_800D0690[];
extern char D_800D0698[];
extern char D_800D06A8[];
extern char D_800D06EC[];
extern char D_800D06F8[];
extern char D_800D0704[];
extern char D_800D0714[];
extern char D_800D0730[];
extern char D_800D0738[];
extern char D_800D0FC4[];
#define CD_cbsync D_80176B98
#define CD_cbready D_80176B9C
#define CD_cbread D_80176BA0
#define CD_debug D_80176BA4
#define CD_status D_80176BA8
#define CD_status1 D_80176BAC
#define CD_nopen D_80176BB0
#define CD_pos D_80176BB4
#define CD_mode D_80176BB8
#define CD_com cdLastCom
#define CD_comstr D_80176BBC
#define CD_intstr D_80176C3C
#define Result D_801914E8
extern int printf(const char *, ...);
extern int sub_8013CF7C(const char *);     /* puts */
#define puts sub_8013CF7C
extern int VSync(int);
extern int CheckCallback(void);
extern void ResetCallback(void);
extern void *InterruptCallback(int, void (*)(void));
s32 sub_8013ADB4(void);
s32 sub_8013B2F8(s32 mode, s32 result);
s32 sub_8013B57C(s32 mode, s32 result);
s32 sub_8013B84C();
void sub_8013BD3C(void);
void sub_8013BE4C(void);
s32 sub_8013C3A8(void);
s32 sub_8013CAD4(s32 mode);
s32 sub_8013CC40(s32 buf, s32 size);
void sub_8013CD30(void);
void sub_8013CE10(s32 status, s32 result);
#define getintr sub_8013ADB4
#define CD_sync sub_8013B2F8
#define CD_ready sub_8013B57C
#define CD_cw sub_8013B84C
#define CD_shell sub_8013BD3C
#define CD_flush sub_8013BE4C
#define cd_read_retry sub_8013C3A8
#define CD_datasync sub_8013CAD4
#define CD_getsector sub_8013CC40
#define callback sub_8013CD30
s32 sub_8013C098(void);
void sub_8013C028(void);
s32 sub_8013BF34(void);
s32 sub_8013C81C(s32 mode, s32 result);
s32 sub_8013C724(s32 buf, s32 sectors, s32 mode);
#define CD_init sub_8013C098
#define CD_initintr sub_8013C028
#define CD_initvol sub_8013BF34
#define CD_readsync sub_8013C81C
#define CD_readm sub_8013C724
extern s32 DMACallback(s32, void (*)());
#define cb_read sub_8013CE10
extern StHEADER *D_801FAC64;        /* ring base */
extern u32 D_801FAD34;              /* ring size (sectors) */
extern s32 D_801F8AB4;              /* next sector to hand out */
extern s32 D_801F8AAC;              /* sector being filled */
extern s32 D_801F8AA8;              /* next sector to fill */
extern s32 D_801B2D0C;
extern s32 D_801AB888;
extern s16 D_801AB058;
extern s32 D_801A5B04;
extern void (*D_801AB840)(void);    /* StSetStream func1 */
extern void (*D_801AB844)(void);    /* StSetStream func2 */
extern s32 D_801AB05C;              /* mode & 1 */
extern s32 D_801AB060;              /* 0: 2340-byte sectors, 1: 2048 */
extern s32 D_801B2B24;
extern s32 D_801AB934;
extern s32 D_801FAC54;
extern u32 D_801FAC5C;              /* StSetMask mask */
extern u32 D_801AB938;              /* start frame */
extern u32 D_801FAC58;              /* end frame */
void sub_8013DA48(void);
int sub_8013DC54(s32 start, u32 count);
void sub_8013DD58(u32 mask, u32 start, u32 end);
void sub_8013DAA8(void);
void sub_8013DD78(void);
extern CdlCB sub_8013A6FC(CdlCB func);      /* CdReadyCallback */
extern int CdControl(u8 com, u8 *param, u8 *result);
extern s32 PadStartCom_1(s32 mode, u8 *result);  /* CdReady */
extern s32 D_801B2DC8;              /* memory-stream sector counter */
extern s32 D_80176F94;              /* last StCdInterrupt status */
extern volatile s32 *D_80176F6C;    /* 0x1F8010B8 CD DMA CHCR */
extern volatile s32 *D_80176F7C;    /* 0x1F8010B8 CD DMA CHCR */
extern volatile u8 *D_80176F4C;     /* 0x1F801800 */
extern volatile u8 *D_80176F54;     /* 0x1F801802 */
extern volatile u8 *D_80176F58;     /* 0x1F801803 */
extern volatile s32 *D_80176F5C;    /* 0x1F801018 */
extern volatile s32 *D_80176F60;    /* 0x1F801020 */
extern volatile u16 *D_8019390C;    /* current sector header */
extern u32 *D_801FAC60;             /* current sector data */
void sub_8013E6F0(u32 *dst, u32 *src, u32 n, u32 unused);
void sub_8013E724(s32 ch, u32 madr, u32 bh, u32 bl, u32 chcr, u8 flag, s32 unused);
extern int strncmp(const char *, const char *, int);
extern CdlFILE D_8019150C[64];      /* file cache */
extern CdlDIR D_80191B0C[128];      /* directory table */
extern u8 D_8019310C[0x800];        /* load_buf */
extern s32 D_80176ED4;              /* cached directory */
extern s32 D_80176ED8;              /* CD_nopen at last media scan */
extern char D_800D0744[];
extern char D_800D0760[];
extern char D_800D0778[];
extern char D_800D0794[];
extern char D_800D07B4[];
extern char D_800D07C0[];
extern char D_800D07D0[];
extern char D_800D07FC[];
extern char D_800D0804[];
extern char D_800D0834[];
extern char D_800D0858[];
extern char D_800D0878[];
extern char D_800D088C[];
extern char D_800D08B0[];
extern char D_800D08D0[];
extern char D_800D08EC[2] __attribute__((aligned(4)));   /* "." */
extern char D_800D08F0[3] __attribute__((aligned(4)));   /* ".." */
extern char D_800D08F4[];
extern char D_800D0910[];
extern int strcmp(const char *, const char *);
extern void sub_8013D960(char *dst, u8 *src, u32 size);    /* memcpy2 */
s32 sub_8013D298(void);
s32 sub_8013D58C(s32 parent, char *name);
s32 sub_8013D638(s32 dir);
s32 sub_8013D274(const char *str1, const char *str2);
s32 sub_8013D8F4(s32 sectors, s32 pos, s32 buf);
#define file D_8019150C
#define dirs D_80191B0C
#define load_buf D_8019310C
extern void sub_8014DC04();     /* def_cbsync */
extern void sub_8014DC2C();     /* def_cbready */
extern void sub_8014DC54();     /* def_cbread */
extern CdlCB PadStopCom_1(CdlCB func);      /* CdSyncCallback */
extern CdlCB sub_8013A714(CdlCB func);      /* CdReadCallback */
extern volatile u32 *D_80176F64;    /* 0x1F8010F0 DPCR */
extern volatile u32 *D_80176F68;    /* 0x1F8010F4 DICR */
extern char D_800D0930[];
#define DMA ((volatile DmaCh *)0x1F801080)
#define DECODE_BCD(x) (((x) >> 4) * 10 + ((x) & 0xF))
#define Intr D_80176E74

#pragma tm2_rdata .tm2_800D0650

/* ---- bios.c (LSD decomp src/psyq/libcd_bios.c, sotn-decomp src/main/psxsdk/libcd/bios.c) ---- */

static __inline__ void copy8(u8 *d, const u8 *s) {
    s32 i;
    if (d != 0) {
        for (i = 7; i != -1; i--) {
            *d++ = *s++;
        }
    }
}

/* @8013ADB4 getintr: read the controller interrupt cause and response bytes, update
 * CD_status/CD_nopen and the sync/ready state; 2 sync, 4 ready, 6 both, 0 none. */
s32 __attribute__((section(".text_8013ADB4"))) sub_8013ADB4(void) {
    volatile u8 cause;
    u8 resp[8];
    s32 i;
    s32 flags;

    *D_80176E5C = 1;
    cause = *D_80176E68 & 7;
    if (cause == 0) {
        return 0;
    }
    flags = 0;
    while (cause != (*D_80176E68 & 7)) {
        cause = *D_80176E68 & 7;
    }

    for (i = 0; i < 8 && (*D_80176E5C & 0x20); i++) {
        resp[i] = *D_80176E60;
    }
    for (; i < 8; i++) {
        resp[i] = 0;
    }

    *D_80176E5C = 1;
    *D_80176E68 = 7;
    *D_80176E64 = 7;

    if (cause != 3 || D_80176D5C[CD_com] != 0) {
        if (!(CD_status & 0x10) && (resp[0] & 0x10)) {
            CD_nopen++;
        }
        CD_status = *(volatile u8 *)&resp[0];
        CD_status1 = resp[1];
        flags = CD_status & 0x1D;
    }

    if (cause == 5) {
        puts(D_800D0608);
        if (CD_debug > 0) {
            printf(D_800D0614, CD_comstr[CD_com], CD_status, CD_status1);
        }
    }

    switch (cause) {
        case 3:
            if (flags != 0) {
                *(volatile u8 *)IntrB = 5;
                copy8(Result[0], resp);
                return 2;
            }
            if (D_80176C5C[CD_com] != 0) {
                *(volatile u8 *)IntrB = 3;
                copy8(Result[0], resp);
                return 1;
            }
            *(volatile u8 *)IntrB = 2;
            copy8(Result[0], resp);
            return 2;

        case 2: {
            u8 v;
            if (flags != 0) {
                v = 5;
            } else {
                v = 2;
            }
            IntrB[0] = v;
            copy8(Result[0], resp);
            return 2;
        }

        case 1:
            D_80176E75 = (flags != 0) ? 5 : 1;
            copy8(Result[1], resp);
            return 4;

        case 4:
            D_80176E76 = 4;
            *(volatile u8 *)&D_80176E75 = D_80176E76;
            copy8(Result[2], resp);
            copy8(Result[1], resp);
            return 4;

        case 5:
            D_80176E75 = 5;
            *(volatile u8 *)IntrB = D_80176E75;
            copy8(Result[0], resp);
            copy8(Result[1], resp);
            return 6;

        default:
            puts(D_800D0630);
            printf(D_800D0644, cause);
            return -1;
    }
}

static __inline__ void set_alarm(char *name) {
    D_80191500 = VSync(-1) + 480;
    D_80191504 = 0;
    D_80191508 = name;
}

static __inline__ int get_alarm(void) {
    if (D_80191500 < VSync(-1) || D_80191504++ > 0x1E0000) {
        char **name = &D_80191508;
        puts(D_800D05DC);
        printf(D_800D05EC, *name, CD_comstr[CD_com], CD_intstr[Intr.sync],
               CD_intstr[Intr.ready]);
        CD_flush();
        return -1;
    }
    return 0;
}

static __inline__ void cd_callback(void) {
    int interrupt;
    u8 saved;


    saved = *D_80176E5C & 3;
    while (1) {
        interrupt = getintr();
        if (interrupt == 0) {
            break;
        }
        if ((interrupt & 4) && CD_cbready != NULL) {
            CD_cbready(Intr.ready, Result[1]);
        }
        if ((interrupt & 2) && CD_cbsync != NULL) {
            CD_cbsync(Intr.sync, Result[0]);
        }
    }
    *D_80176E5C = saved;
}

/* @8013B2F8 CD_sync: wait (mode 0) or poll (mode 1) for command completion; returns the
 * sync state (2 complete, 5 error), 0 if pending, -1 on timeout. */
s32 __attribute__((section(".text_8013B2F8"))) sub_8013B2F8(s32 mode, s32 result) {
    int sync;

    set_alarm(D_800D0664);
    while (1) {
        if (get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            cd_callback();
        }
        sync = Intr.sync;
        if (sync == 2 || sync == 5) {
            Intr.sync = 2;
            copy8((u8 *)result, Result[0]);
            return sync;
        }
        if (mode != 0) {
            return 0;
        }
    }
}

/* @8013B57C CD_ready: wait (mode 0) or poll (mode 1) for data ready / data end; returns
 * the ready state, 0 if none, -1 on timeout. */
s32 __attribute__((section(".text_8013B57C"))) sub_8013B57C(s32 mode, s32 result) {
    int c;
    int ready;

    set_alarm(D_800D066C);
    while (1) {
        if (get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            cd_callback();
        }
        c = Intr.c;
        if (c != 0) {
            Intr.c = 0;
            copy8((u8 *)result, Result[2]);
            return c;
        }
        ready = Intr.ready;
        if (ready != 0) {
            Intr.ready = 0;
            copy8((u8 *)result, Result[1]);
            return ready;
        }
        if (mode != 0) {
            return 0;
        }
    }
}

/* @8013B84C CD_cw: issue one controller command with its parameters; unless async,
 * wait for the acknowledge. 0 ok, -1 error/timeout, -2 missing parameter. */
s32 __attribute__((section(".text_8013B84C"))) sub_8013B84C(com, param, result, async)
    u8 com;
    u8 *param;
    u8 *result;
    s32 async;
{
    int i;

    if (CD_debug > 1) {
        printf(D_800D0678, CD_comstr[com]);
    }
    if (D_80176DDC[com] != 0 && param == NULL) {
        if (CD_debug > 0) {
            printf(D_800D0680, CD_comstr[com]);
        }
        return -2;
    }
    CD_sync(0, 0);
    if (com == 2) {
        for (i = 0; i < 4; i++) {
            CD_pos[i] = param[i];
        }
    }
    Intr.sync = 0;
    if (D_80176CDC[com] != 0) {
        Intr.ready = 0;
    }
    *D_80176E5C = 0;
    for (i = 0; i < D_80176CDC[com + 0x40]; i++) {
        *D_80176E64 = param[i];
    }
    CD_com = com;
    *D_80176E60 = com;
    if (async != 0) {
        return 0;
    }

    set_alarm(D_800D0690);
    while (Intr.sync == 0) {
        if (get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            cd_callback();
        }
    }
    if (Intr.sync == 2 && com == 0xE) {
        CD_mode = *param;
    }
    copy8(result, Result[0]);
    return -(Intr.sync == 5);
}

/* @8013BCB4 CD_vol: write the four CD audio attenuation registers. */
s32 __attribute__((section(".text_8013BCB4"))) sub_8013BCB4(u8 *arg0) {
    *D_80176E5C = 2;
    *D_80176E64 = arg0[0];
    *D_80176E68 = arg0[1];
    *D_80176E5C = 3;
    *D_80176E60 = arg0[2];
    *D_80176E64 = arg0[3];
    *D_80176E68 = 0x20;
    return 0;
}

static __inline__ void cd_shell(void) {
    CdIntrCallback saved;
    s32 counter = 0;

    if (D_80176EA0 < CD_nopen) {
        saved = CD_cbsync;
        CD_cbsync = 0;

        while (CD_status & 0x10) {
            if ((u8)counter == 0) {
                puts(D_800D0698);
            }
            counter++;
            CD_cw(1, 0, 0, 0);
        }

        while (CD_cw(0x16, D_80176EA4, 0, 0)) {
            CD_cw(1, 0, 0, 0);
            puts(D_800D06A8);
        }

        CD_cbsync = saved;
        D_80176EA0 = CD_nopen;
    }
}

static __inline__ void cd_flush(void) {
    *D_80176E5C = 1;
    while (*D_80176E68 & 7) {
        *D_80176E5C = 1;
        *D_80176E68 = 7;
        *D_80176E64 = 7;
    }

    Intr.ready = Intr.c = 0;
    CD_mode = 0;
    Intr.sync = 2;
    *D_80176E5C = 0;
    *D_80176E68 = 0;
    *D_80176E6C = 0x1325;
}

/* @8013BD3C CD_shell: after the shell has been opened, wait for it to close and seek
 * back to the saved position. */
void __attribute__((section(".text_8013BD3C"))) sub_8013BD3C(void) {
    cd_shell();
}

/* @8013BE4C CD_flush: drain pending controller interrupts and reset the sync/ready state. */
void __attribute__((section(".text_8013BE4C"))) sub_8013BE4C(void) {
    cd_flush();
}

/* @8013BF34 CD_initvol: set SPU main/CD volume and the default CD attenuation. */
s32 __attribute__((section(".text_8013BF34"))) sub_8013BF34(void) {
    u8 buf[4];

    if (D_80176E70[0xDC] == 0 && D_80176E70[0xDD] == 0) {
        D_80176E70[0xC0] = 0x3FFF;
        D_80176E70[0xC1] = 0x3FFF;
    }
    D_80176E70[0xD8] = 0x3FFF;
    D_80176E70[0xD9] = 0x3FFF;
    D_80176E70[0xD5] = 0xC001;

    buf[2] = 0x80;
    buf[0] = 0x80;
    buf[3] = 0;
    buf[1] = 0;
    *D_80176E5C = 2;
    *D_80176E64 = buf[0];
    *D_80176E68 = buf[1];
    *D_80176E5C = 3;
    *D_80176E60 = buf[2];
    *D_80176E64 = buf[3];
    *D_80176E68 = 0x20;
    return 0;
}

/* @8013C028 CD_initintr: clear the callbacks/read state and reinstall the interrupt handler. */
void __attribute__((section(".text_8013C028"))) sub_8013C028(void) {
    s32 *p;
    s32 i;

    CD_cbready = 0;
    CD_cbsync = 0;
    CD_status1 = 0;
    CD_status = 0;
    p = D_80176E78;
    for (i = 9; i != -1; i--) {
        *p = 0;
        p++;
    }
    ResetCallback();
    InterruptCallback(2, callback);
}

/* @8013C098 CD_init: reset the driver state, install the CD interrupt handler, flush,
 * then Nop/Reset/Demute the controller. 0 ok, -1 failure. */
s32 __attribute__((section(".text_8013C098"))) sub_8013C098(void) {
    s32 *p;
    s32 i;

    puts(D_800D06EC);
    printf(D_800D06F8, D_80176EA8);
    CD_com = 0;
    CD_mode = 0;
    CD_cbready = 0;
    CD_cbsync = 0;
    CD_status1 = 0;
    CD_status = 0;
    p = D_80176E78;
    for (i = 9; i != -1; i--) {
        *p = 0;
        p++;
    }
    ResetCallback();
    InterruptCallback(2, callback);

    cd_flush();
    CD_cw(1, 0, 0, 0);
    if (CD_status & 0x10) {
        CD_cw(1, 0, 0, 0);
    }
    cd_shell();

    if (CD_cw(0xA, 0, 0, 0)) {
        return -1;
    }
    if (CD_cw(0xC, 0, 0, 0)) {
        return -1;
    }
    return -(CD_sync(0, 0) != 2);
}

/* @8013C3A8 cd_read_retry: (re)start a CD_readm read: recover from shell open, re-seek
 * on retries, set the mode and issue ReadN. Returns the sector count, -1 when
 * the retries are exhausted. */
s32 __attribute__((section(".text_8013C3A8"))) sub_8013C3A8(void) {
    s32 n;
    s32 *tmp;
    s32 *pRetry;
    s32 saved;
    s32 counter;
    volatile u8 *q;
    u8 buf;

    tmp = D_80176E78;
    n = *tmp;
    CD_cbready = 0;
    CD_cbsync = 0;
    *tmp = n - 1;
    __asm__("");

    if (n > 0) {
        pRetry = tmp;
        do {
            if (*pRetry < 7) {
                counter = 0;
                puts(D_800D0704);
                printf(D_800D0714, *pRetry, CD_pos[0], CD_pos[1], CD_pos[2]);

                if (D_80176EA0 < CD_nopen) {
                    saved = (s32)CD_cbsync;
                    CD_cbsync = 0;

                    while (CD_status & 0x10) {
                        if ((u8)counter == 0) {
                            puts(D_800D0698);
                        }
                        counter++;
                        CD_cw(1, 0, 0, 0);
                    }

                    while (CD_cw(0x16, D_80176EA4, 0, 0)) {
                        CD_cw(1, 0, 0, 0);
                        puts(D_800D06A8);
                    }

                    CD_cbsync = (CdIntrCallback)saved;
                    D_80176EA0 = CD_nopen;
                }

                if (CD_cw(9, 0, 0, 0) != 0) {
                    goto tail;
                }
                if (CD_cw(2, CD_pos, 0, 0) != 0) {
                    goto tail;
                }
            }

            *D_80176E5C = 1;
            while (*D_80176E68 & 7) {
                *D_80176E5C = 1;
                *D_80176E68 = 7;
                *D_80176E64 = 7;
            }

            D_80176E76 = 0;
            q = &D_80176E75;
            CD_mode = 0;
            *q = D_80176E76;
            __asm__("");
            IntrB[0] = 2;
            *D_80176E5C = 0;
            *D_80176E68 = 0;
            *D_80176E6C = 0x1325;

            {
                s32 v0 = ((volatile s32 *)D_80176E78)[4];
                buf = (u8)v0;
                n = ((u8)v0 != CD_mode);
                if (n) {
                    saved = (s32)&buf;
                    if (CD_cw(0xE, (u8 *)saved, 0, 0) != 0) {
                        goto tail;
                    }
                }
            }

            CD_cbready = (CdIntrCallback)cb_read;
            ((volatile s32 *)D_80176E78)[3] = ((volatile s32 *)D_80176E78)[2];
            CD_cw(6, 0, 0, 1);
            ((volatile s32 *)D_80176E78)[6] = ((volatile s32 *)D_80176E78)[1];
            ((volatile s32 *)D_80176E78)[7] = VSync(-1) + 0x1E0;
            return ((volatile s32 *)D_80176E78)[6];

        tail:
            tmp = D_80176E78;
            n = *tmp;
            *tmp = n - 1;
            __asm__("");
        } while (n > 0);
    }

    {
        volatile s32 *q = &D_80176E90;
        s32 m;
        *q = -1;
        m = *q;
        return m;
    }
}

/* @8013C724 CD_readm: set up a sector read (sector size from mode), pause if playing,
 * and start it. 0 ok, -1 failure. */
s32 __attribute__((section(".text_8013C724"))) sub_8013C724(s32 arg0, s32 arg1, s32 arg2) {
    s32 t;
    volatile s32 *p;

    p = &D_80176E88;
    *p = arg2;
    t = *p & 0x30;

    /* MATCHING: the gotos and local volatile pointers keep the three mode stores as separate blocks. */
    if (t == 0) {
        goto case1;
    }
    if (t == 0x20) {
        goto case2;
    }
    goto case3;
case1:
    D_80176E8C = 0x200;
    goto join;
case2:
    D_80176E8C = 0x249;
    goto join;
case3: {
    volatile s32 *q3 = &D_80176E8C;
    *q3 = 0x246;
}
join:

{
    volatile s32 *q4 = &D_80176E80;
    *q4 = arg0;
}
    D_80176E7C = arg1;
    D_80176E78[0] = 8;
    D_80176E98 = CD_cbsync;
    D_80176E9C = CD_cbready;

    if (CD_status & 0xE0) {
        CD_cw(9, 0, 0, 0);
    }
    CD_sync(0, 0);
    return -(cd_read_retry() < 1);
}

/* @8013C81C CD_readsync: wait (mode 0) or poll a CD_readm read, retrying when it stalls;
 * returns the sectors left (0 done, -1 error). */
s32 __attribute__((section(".text_8013C81C"))) sub_8013C81C(s32 arg0, s32 arg1) {
    s32 now;
    s32 old;
    s32 flags;
    char **pEC;
    u8 status;
    char **p6A0;
    u8 *p8D8;
    u8 *p8D9;
    s32 *pF8;
    u8 *dst;
    u8 *src;
    s32 i;
    s32 idx0;
    s32 idx1;
    s32 result;

    now = VSync(-1);
    p6A0 = CD_intstr;
    p8D8 = IntrB;
    p8D9 = &IntrB[1];
    pF8 = &D_80176E78[7];

    D_80191500 = now + 0x1E0;
    D_80191504 = 0;
    D_80191508 = D_800D0730;

    for (;;) {
        now = VSync(-1);
        if (D_80191500 < now) {
            goto timeout;
        }
        old = D_80191504;
        D_80191504 = old + 1;
        if (0x1E0000 >= old) {
            goto success;
        }

    timeout:
        puts(D_800D05DC);
        idx0 = p8D8[0];
        idx1 = p8D8[1];
        /* Keeps both p8D8 byte loads directly after the puts call, ahead of
         * the printf argument loads; without it the p8D8[0] load sinks below them. */
        __asm__("");
        /* MATCHING: &D_80191508 through a local pointer keeps retail's
         * unfolded lui/addiu for this argument; a plain reference folds. */
        pEC = &D_80191508;
        printf(D_800D05EC, *pEC, CD_comstr[CD_com], p6A0[idx0], p6A0[idx1]);
        CD_flush();
        result = -1;
        goto after_diag;

    success:
        result = 0;

    after_diag:
        if (result != 0) {
            return result;
        }
        if (CheckCallback() != 0) {
            status = (u8)(*D_80176E5C & 3);
            for (;;) {
                flags = getintr();
                if (flags == 0) {
                    break;
                }
                if ((flags & 4) && CD_cbready != 0) {
                    CD_cbready(p8D9[0], Result[1]);
                }
                if ((flags & 2) && CD_cbsync != 0) {
                    CD_cbsync(p8D8[0], Result[0]);
                }
            }
            /* MATCHING: do not copy: the test is always true and both arms are one store; it only steers registers. */
            if (p6A0 || pF8) {
                *D_80176E5C = status;
            } else {
                *D_80176E5C = status;
            }
        }

        dst = (u8 *)arg1;
        src = Result[1];
        if (dst != 0) {
            for (i = 7; i != -1; i--) {
                *dst = *src;
                src++;
                dst++;
            }
        }

        if (VSync(-1) > pF8[0] + 0x3C) {
            cd_read_retry();
        }
        if (pF8[-1] == 0) {
            CD_datasync(0);
        }
        if (arg0 != 0 || pF8[-1] <= 0) {
            break;
        }
    }

    return pF8[-1];
}

/* @8013CAD4 CD_datasync: wait (mode 0) or poll for the CD DMA transfer to finish;
 * 0 done, 1 busy (poll), -1 timeout. */
s32 __attribute__((section(".text_8013CAD4"))) sub_8013CAD4(s32 mode) {
    int ret;

    set_alarm(D_800D0738);
    while (1) {
        if (get_alarm()) {
            ret = -1;
            break;
        }
        if (!(*D_80176ED0 & 0x01000000)) {
            ret = 0;
            break;
        }
        if (mode != 0) {
            ret = 1;
            break;
        }
    }
    return ret;
}

/* @8013CC40 CD_getsector: DMA size words of the current sector to buf. */
s32 __attribute__((section(".text_8013CC40"))) sub_8013CC40(s32 arg0, s32 arg1) {
    *D_80176E5C = 0;
    *D_80176E68 = 0x80;
    *D_80176EC0 = 0x20943;
    *D_80176E6C = 0x1323;
    *D_80176EC4 |= 0x8000;
    *D_80176EC8 = arg0;
    *D_80176ECC = arg1 | 0x10000;
    *D_80176ED0 = 0x11000000;

    while (*D_80176ED0 & 0x1000000) {
    }

    *D_80176E6C = 0x1325;
    return 0;
}

/* @8013CD30 callback: CD interrupt handler; dispatches the ready/sync callbacks. */
void __attribute__((section(".text_8013CD30"))) sub_8013CD30(void) {
    cd_callback();
}

/* @8013CE10 cb_read: ready callback of CD_readm; transfers each sector, then restores
 * the callbacks, pauses and reports through CD_cbread. */
void __attribute__((section(".text_8013CE10"))) sub_8013CE10(s32 arg0, s32 arg1) {
    volatile s32 *p;
    s32 code;
    s32 dummy;
    volatile s32 *new_var;

    if (arg0 != 1) {
        goto elseBranch;
    }
    p = &D_80176E90;
    if (*p <= 0) {
        goto shared;
    }
    CD_getsector(D_80176E84, D_80176E8C);
    D_80176E84 = D_80176E84 + D_80176E8C * 4;
    *p = *p - 1;
    dummy = *p;
    (void)dummy;
    goto shared;
elseBranch: {
    volatile s32 *p2 = &D_80176E90;
    *p2 = -1;
}
shared: {
    volatile s32 *pF8 = &D_80176E94;
    *pF8 = VSync(-1);
}

    if (D_80176E90 < 0 && D_80176E78[0] > 0) {
        cd_read_retry();
    }

    if ((*(new_var = &D_80176E90)) <= 0) {
        CD_cbsync = D_80176E98;
        CD_cbready = D_80176E9C;
        CD_cw(9, 0, 0, 0);
        if (CD_cbread != 0) {
            if ((*new_var) == 0) {
                code = 2;
            } else {
                code = 5;
            }
            CD_cbread(code, (u8 *)arg1);
        }
    }
}

/* ---- sys.c (sotn-decomp src/main/psxsdk/libcd/sys.c) ---- */

/* @8013A590 CdReset: mode 2 only reinstalls the interrupt handler; otherwise a full
 * CD_init, plus volume init for mode 1. 1 ok, 0 failure. */
s32 __attribute__((section(".text_8013A590"))) sub_8013A590(s32 mode) {
    if (mode == 2) {
        CD_initintr();
        return 1;
    }
    if (CD_init()) {
        return 0;
    }
    if (mode == 1 && CD_initvol()) {
        return 0;
    }
    return 1;
}

/* @8013A5FC CdFlush. */
void __attribute__((section(".text_8013A5FC"))) sub_8013A5FC(void) {
    CD_flush();
}

/* @8013AB28 CdGetSector: DMA one sector's data from the CD buffer; 1 ok. */
s32 __attribute__((section(".text_8013AB28"))) sub_8013AB28(s32 madr, s32 size) {
    return CD_getsector(madr, size) == 0;
}

/* @8013AB4C CdDataCallback: install the CD DMA (channel 3) callback. */
s32 __attribute__((section(".text_8013AB4C"))) sub_8013AB4C(void (*func)()) {
    return DMACallback(3, func);
}

/* @8013AB70 CdDataSync. */
void __attribute__((section(".text_8013AB70"))) sub_8013AB70(s32 mode) {
    CD_datasync(mode);
}

/* @8013AB90 CdReadSync: wait for / poll a CdRead; returns the sectors left. */
s32 __attribute__((section(".text_8013AB90"))) sub_8013AB90(s32 mode, s32 result) {
    return CD_readsync(mode, result);
}

/* @8013ABB0 CdRead: start reading sectors into buf (up to four attempts); 1 ok. */
s32 __attribute__((section(".text_8013ABB0"))) sub_8013ABB0(s32 sectors, s32 buf, s32 mode) {
    s32 i;

    for (i = 3; i != -1; i--) {
        if (CD_readm(buf, sectors, mode) == 0) {
            return 1;
        }
    }
    return 0;
}

static __inline__ int encode_bcd(int n) {
    return ((n / 10) << 4) + (n % 10);
}

/* @8013AC30 CdIntToPos: absolute sector -> BCD minute/second/sector. */
CdlLOC *__attribute__((section(".text_8013AC30"))) sub_8013AC30(s32 i, CdlLOC *p) {
    i += 150;
    p->sector = encode_bcd(i % 75);
    p->second = encode_bcd(i / 75 % 60);
    p->minute = encode_bcd(i / 75 / 60);
    return p;
}

/* @8013AD34 CdPosToInt: BCD position -> absolute sector. */
s32 __attribute__((section(".text_8013AD34"))) sub_8013AD34(CdlLOC *p) {
    u8 sector = p->sector;
    u8 second = p->second;
    u8 minute = p->minute;

    return (DECODE_BCD(minute) * 60 + DECODE_BCD(second)) * 75 +
           DECODE_BCD(sector) - 150;
}

/* ---- iso9660.c (sotn-decomp src/main/psxsdk/libcd/iso9660.c) ---- */

/* @8013CF8C CdSearchFile: look up an absolute path ("\DIR\FILE;1") in the ISO9660 file
 * system; fills *fp and returns the cache entry, NULL if not found. */
CdlFILE *__attribute__((section(".text_8013CF8C"))) sub_8013CF8C(CdlFILE *fp, char *name) {
    char buf[32];
    int var_a0;
    int i;
    char *namePtr;
    char *bufPtr;

    if (D_80176ED8 != CD_nopen) {
        if (sub_8013D298() == 0) {
            return NULL;
        }
        D_80176ED8 = CD_nopen;
    }
    var_a0 = 1;
    if (name[0] != '\\') {
        return NULL;
    }
    buf[0] = 0;
    namePtr = name;
    for (i = 0; i < 8; i++) {
        bufPtr = buf;
        while (*namePtr != '\\') {
            if (*namePtr == 0) {
                goto out;
            }
            *bufPtr++ = *namePtr++;
        }
        if (*namePtr == 0) {
            goto out;
        }
        namePtr++;
        *bufPtr = 0;
        var_a0 = sub_8013D58C(var_a0, buf);
        if (var_a0 == -1) {
            buf[0] = 0;
            break;
        }
    }
out:
    if (i >= 8) {
        if (CD_debug > 0) {
            printf(D_800D0744, name, i);
        }
        return NULL;
    }
    if (buf[0] == 0) {
        if (CD_debug > 0) {
            printf(D_800D0760, name);
        }
        return NULL;
    }
    *bufPtr = 0;
    if (sub_8013D638(var_a0) == 0) {
        if (CD_debug > 0) {
            printf(D_800D0778);
        }
        return NULL;
    }
    if (CD_debug > 1) {
        printf(D_800D0794, buf);
    }
    for (i = 0; i < 64; i++) {
        if (file[i].name[0] == 0) {
            break;
        }
        if (sub_8013D274(file[i].name, buf)) {
            if (CD_debug > 1) {
                printf(D_800D07B4, buf);
            }
            *fp = file[i];
            return &file[i];
        }
    }
    if (CD_debug > 0) {
        printf(D_800D07C0, buf);
    }
    return NULL;
}

/* @8013D274 _cmp: compare two 12-character file names; 1 if equal. */
s32 __attribute__((section(".text_8013D274"))) sub_8013D274(const char *str1, const char *str2) {
    return strncmp(str1, str2, 0xC) == 0;
}

/* @8013D298 CD_newmedia: read the PVD and path table of a new disc and build the
 * directory table. 1 ok, 0 error. */
s32 __attribute__((section(".text_8013D298"))) sub_8013D298(void) {
    LBA pathTableLBA;
    u8 *pathTablePtr;
    s32 i;

    if (sub_8013D8F4(1, 0x10, (s32)load_buf) != 1) {
        if (CD_debug > 0) {
            printf(D_800D07D0);
        }
        return 0;
    }
    if (strncmp((char *)&load_buf[1], D_800D07FC, 5) != 0) {
        if (CD_debug > 0) {
            printf(D_800D0804);
        }
        return 0;
    }

    (&pathTableLBA)->i = ((LBA *)(&load_buf[0x8C]))->i;
    if (sub_8013D8F4(1, pathTableLBA.addr, (s32)load_buf) != 1) {
        if (CD_debug > 0) {
            printf(D_800D0834, pathTableLBA.addr);
        }
        return 0;
    }
    if (CD_debug > 1) {
        printf(D_800D0858);
    }
    do {
        i = 0;
        pathTablePtr = load_buf;
        while (pathTablePtr < load_buf + 0x800) {
            if (pathTablePtr[0] == 0) {
                break;
            }
            dirs[i].unk8.i = ((LBA *)(&pathTablePtr[2]))->i;
            dirs[i].unk4 = pathTablePtr[6];
            dirs[i].unk0 = i + 1;

            sub_8013D960(dirs[i].unkC, &pathTablePtr[8], pathTablePtr[0]);
            dirs[i].unkC[pathTablePtr[0]] = '\0';
            pathTablePtr += 8 + pathTablePtr[0] + pathTablePtr[0] % 2;
            if (CD_debug > 1) {
                printf(D_800D0878, dirs[i].unk8.addr, dirs[i].unk0, dirs[i].unk4,
                       dirs[i].unkC);
            }
            if (++i >= 128) {
                break;
            }
        }
        if (i < 128) {
            dirs[i].unk4 = 0;
        }
    } while (0);

    D_80176ED4 = 0;
    if (CD_debug > 1) {
        printf(D_800D088C, i);
    }
    return 1;
}

/* @8013D58C CD_searchdir: find the directory called name under parent; returns its
 * number, -1 if absent. */
s32 __attribute__((section(".text_8013D58C"))) sub_8013D58C(s32 parent, char *name) {
    s32 i;

    for (i = 0; i < 128; i++) {
        if (dirs[i].unk4 == 0) {
            return -1;
        }
        if (dirs[i].unk4 != parent) {
            continue;
        }
        if (strcmp(name, dirs[i].unkC) == 0) {
            return i + 1;
        }
    }
    return -1;
}

/* @8013D638 CD_cachefile: read directory dir's extent into the file cache. 1 ok, -1 error. */
s32 __attribute__((section(".text_8013D638"))) sub_8013D638(s32 dir) {
    LBA sp18;
    u8 *ptr;
    s32 i;
    char *namePtr;

    if (dir == D_80176ED4) {
        return 1;
    }

    if (sub_8013D8F4(1, (dirs - 1)[dir].unk8.addr, (s32)load_buf) != 1) {
        if (CD_debug > 0) {
            printf(D_800D08B0);
        }
        return -1;
    }
    if (CD_debug > 1) {
        printf(D_800D08D0);
    }
    i = 0;
    ptr = load_buf;
    while (ptr < load_buf + 0x800) {
        if (ptr[0] == 0) {
            break;
        }
        sp18.i = ((LBA *)&ptr[2])->i;
        sub_8013AC30(sp18.addr, &file[i].pos);
        ((LBA *)&file[i].size)->i = ((LBA *)&ptr[0xA])->i;
        switch (i) {
        case 0:
            namePtr = file[i].name;
            __builtin_memcpy(file[0].name, D_800D08EC, 2);     /* strcpy(namePtr, ".") */
            break;
        case 1:
            namePtr = file[i].name;
            __builtin_memcpy(file[1].name, D_800D08F0, 3);     /* strcpy(namePtr, "..") */
            break;
        default:
            sub_8013D960(file[i].name, &ptr[0x21], ptr[0x20]);
            file[i].name[ptr[0x20]] = '\0';
            break;
        }
        if (CD_debug > 1) {
            printf(D_800D08F4, file[i].pos.minute, file[i].pos.second,
                   file[i].pos.sector, file[i].size, file[i].name);
        }
        ptr += ptr[0];
        if (++i >= 64) {
            break;
        }
    }
    D_80176ED4 = dir;
    if (i < 64) {
        file[i].name[0] = '\0';
    }
    if (CD_debug > 1) {
        printf(D_800D0910, i);
    }
    return 1;
}

/* @8013D8F4 cd_read: synchronously read sectors from absolute sector pos into buf; 1 ok. */
s32 __attribute__((section(".text_8013D8F4"))) sub_8013D8F4(s32 sectors, s32 pos, s32 buf) {
    CdlLOC p;

    sub_8013AC30(pos, &p);
    CdControl(2, (u8 *)&p, NULL);
    sub_8013ABB0(sectors, buf, 0x80);
    return sub_8013AB90(0, 0) == 0;
}

/* ---- streaming (sotn-decomp src/main/psxsdk/libcd/{cdrom.c,cdread2.c,c_002.c..c_011.c}) ---- */

/* @8013A540 StSetRing: set the streaming ring buffer and clear it. */
void __attribute__((section(".text_8013A540"))) sub_8013A540(StHEADER *ring_addr, u32 ring_size) {
    D_801FAC64 = ring_addr;
    D_801FAD34 = ring_size;
    sub_8013DA48();
}

/* @8013DA48 StClearRing: reset the ring indices and sector headers. */
void __attribute__((section(".text_8013DA48"))) sub_8013DA48(void) {
    D_801F8AB4 = 0;
    D_801F8AAC = 0;
    D_801F8AA8 = 0;
    D_801B2D0C = 0;
    sub_8013DC54(0, D_801FAD34);
    D_801AB888 = 0;
    D_801AB058 = 0;
    D_801A5B04 = 0;
}

/* @8013DAA8 data_ready_callback: DMA of a stream sector finished; mark it ready and advance. */
void __attribute__((section(".text_8013DAA8"))) sub_8013DAA8(void) {
    StHEADER *ptr = &D_801FAC64[D_801F8AAC];

    *(u16 *)ptr = 2;
    D_801F8AAC = D_801F8AA8;
    if (D_801AB840 != NULL) {
        D_801AB840();
    }
    D_801B2D0C = 0;
}

/* @8013DB10 StSetStream: configure streaming (mode, frame range, callbacks). */
void __attribute__((section(".text_8013DB10"))) sub_8013DB10(u32 mode, u32 start_frame, u32 end_frame,
                                                              void (*func1)(void), void (*func2)(void)) {
    sub_8013DD58(1, start_frame, end_frame);
    D_801FAC54 = 0;
    D_801AB840 = func1;
    D_801AB05C = mode & 1;
    D_801B2B24 = 0;
    D_801AB934 = 0;
    D_801AB058 = 0;
    D_801A5B04 = 0;
    D_801AB844 = func2;
}

/* @8013DB98 StFreeRing: release the frame at base back to the ring; 0 ok, 1 if it was
 * not a handed-out frame. */
u32 __attribute__((section(".text_8013DB98"))) sub_8013DB98(u32 *base) {
    s32 idx;
    s32 i;
    s16 nSectors;
    StHEADER *hdr;

    idx = (base - (u32 *)&D_801FAC64[D_801FAD34]) / 504;
    hdr = &D_801FAC64[idx];
    nSectors = D_801FAC64[idx].nSectors;
    if ((s16)hdr->id != 4) {
        return 1;
    }
    for (i = 0; i < nSectors; i++) {
        *(s16 *)&D_801FAC64[i + idx] = 0;
    }
    D_801F8AB4 = i + idx;
    return 0;
}

/* @8013DC54 init_ring_status: clear count sector headers from start. */
int __attribute__((section(".text_8013DC54"))) sub_8013DC54(s32 start, u32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        *(s32 *)&D_801FAC64[i + start] = 0;
    }
}

/* @8013DC94 StGetNext: hand out the next complete frame (data, header); 0 ok, 1 none. */
u32 __attribute__((section(".text_8013DC94"))) sub_8013DC94(u32 **addr, u32 **header) {
    volatile StHEADER *ptr = &D_801FAC64[D_801F8AB4];

    if (ptr->id == 1) {
        D_801F8AB4 = 0;
        if (D_801FAC58 != 0) {
            ptr->id = 0;
        }
        ptr = &D_801FAC64[D_801F8AB4];
    }
    if (ptr->id == 2) {
        *(volatile u16 *)ptr = 4;
        *addr = (u32 *)(&D_801FAC64[D_801FAD34] + D_801F8AB4 * 0x3F);
        *header = (u32 *)ptr;
        return 0;
    }
    return 1;
}

/* @8013DD58 StSetMask: set the frame mask and start/end frames. */
void __attribute__((section(".text_8013DD58"))) sub_8013DD58(u32 mask, u32 start, u32 end) {
    D_801FAC5C = mask;
    D_801AB938 = start;
    D_801FAC58 = end;
}

/* @8013D9B4 CdRead2: set the mode and start a ReadS; with CdlModeStream the stream
 * interrupt handlers are installed. */
s32 __attribute__((section(".text_8013D9B4"))) sub_8013D9B4(s32 mode) {
    u8 param = mode;

    CdControl(0xE, &param, NULL);
    if (mode & 0x100) {
        if (mode & 0x20) {
            D_801AB060 = 0;
        } else {
            D_801AB060 = 1;
        }
        sub_8013AB4C(sub_8013DAA8);
        sub_8013A6FC((CdlCB)sub_8013DD78);
    }
    return CdControl(0x1B, NULL, NULL);
}

/* @8013DD78 StCdInterrupt: ready callback for streaming; DMA one sector's header and
 * data into the ring, validating STR frame headers. */
void __attribute__((section(".text_8013DD78"))) sub_8013DD78(void) {
    volatile s16 subroutine_arg8[4];
    CdlLOC loc;
    u8 result[8];
    u32 *var_a1;
    s32 var_t0;
    u32 *var_a0;
    u32 var_v1_2;
    u32 var_v1_3;
    u8 *var_v1;

    if (D_801B2D0C == 1) {
        return;
    }
    if ((D_801AB05C != 0) && (*D_80176F6C & 0x01000000)) {
        D_801AB888 = 1;
        if (D_801FAC54 != 0) {
            D_801B2DC8++;
        }
        D_80176F94 = 1;
        return;
    }
    if (PadStartCom_1(1, result) == 5) {
        return;
    }
    subroutine_arg8[1] = result[0];
    subroutine_arg8[2] = result[1];
    if (subroutine_arg8[1] & 4) {
        D_80176F94 = 3;
        return;
    }
    D_8019390C = (u16 *)&D_801FAC64[D_801F8AA8];
    if (D_8019390C[0] != 0) {
        if (D_801FAC54 != 0) {
            D_801B2DC8++;
        }
        D_80176F94 = 4;
        return;
    }
    *D_80176F4C = 0;
    *D_80176F58 = 0;
    *D_80176F4C = 0;
    *D_80176F58 = 0x80;
    *D_80176F5C = 0x20943;
    *D_80176F60 = 0x1323;
    if (D_801AB060 == 0) {
        var_v1 = (u8 *)&subroutine_arg8[4];
        do {
            *var_v1++ = *D_80176F54;
        } while (var_v1 < (u8 *)&subroutine_arg8[6]);
        for (var_v1_2 = 0; var_v1_2 < 8; var_v1_2++) {
            *D_80176F54;
        }
    }
    var_t0 = 0x11000000;
    if (D_801FAC54 != 0) {
        sub_8013E6F0((u32 *)D_8019390C, (u32 *)((D_801B2DC8 << 0xB) + D_801FAC54), 8, 0);
    } else {
        sub_8013E724(3, (u32)D_8019390C, 0, 8, var_t0, 0, 0);
    }
    while (*D_80176F7C & 0x01000000) {
    }
    ((StHEADER *)D_8019390C)->loc = loc;
    *D_80176F5C = 0x20843;
    *D_80176F60 = 0x1325;
    if ((D_801FAC5C == 1) && (D_801AB938 != 0)) {
        if (D_801AB938 != D_8019390C[4]) {
            D_8019390C[0] = 0;
            if (D_801FAC54 != 0) {
                D_801B2DC8++;
            }
            return;
        }
        D_801FAC5C = 0;
    }
    if ((D_8019390C[0] != 0x160) ||
        (((D_8019390C[1] >> 0xA) & 0x1F) != D_801B2B24)) {
        if (D_801FAC54 != 0) {
            D_801B2DC8 = 0;
        } else {
            D_8019390C[0];
        }
        D_80176F94 = 5;
        D_8019390C[0] = 0;
        return;
    }
    if ((D_801AB058 != D_8019390C[2]) ||
        ((D_801A5B04 != 0) && (D_801A5B04 != D_8019390C[4]))) {
        D_801A5B04 = 0;
        D_801AB058 = 0;
        sub_8013DC54(D_801F8AAC, D_801F8AA8 - D_801F8AAC);
        D_801F8AA8 = D_801F8AAC;
        D_8019390C[0] = 0;
        if (D_801FAC54 != 0) {
            D_801B2DC8++;
        }
        D_80176F94 = 6;
        return;
    }
    if (D_8019390C[2] == 0) {
        D_801AB058 = 0;
        D_801A5B04 = D_8019390C[4];
        if ((D_801FAC58 != 0) && (D_801A5B04 >= D_801FAC58)) {
            D_801A5B04 = 0;
            D_801AB058 = 0;
            sub_8013DC54(D_801F8AAC, D_801F8AA8 - D_801F8AAC);
            D_801F8AA8 = D_801F8AAC;
            D_8019390C[0] = 0;
            D_801FAC5C = 1;
            if (D_801AB844 != NULL) {
                D_801AB844();
            }
            if (D_801FAC54 != 0) {
                D_801B2DC8++;
            }
            D_80176F94 = 7;
            return;
        }
        if ((u32)(D_801FAD34 - D_801F8AA8 - 1) < D_8019390C[3]) {
            if (D_801FAC58 == 0) {
                D_8019390C[0] = 1;
                D_801FAC5C = 1;
                if (D_801AB844 != NULL) {
                    D_801AB844();
                }
                if (D_801FAC54 != 0) {
                    D_801B2DC8++;
                }
                D_80176F94 = 8;
                return;
            }
            if ((s16)D_801FAC64->id != 0) {
                D_8019390C[0] = 0;
                if (D_801FAC54 != 0) {
                    D_801B2DC8++;
                }
                D_80176F94 = 9;
                return;
            }
            D_8019390C[0] = 1;
            var_a1 = (u32 *)D_801FAC64;
            var_a0 = (u32 *)D_8019390C;
            D_801F8AA8 = 0;
            for (var_v1_3 = 0; var_v1_3 < 8; var_v1_3++) {
                *var_a1++ = *var_a0++;
            }
            D_8019390C = (u16 *)D_801FAC64;
        }
        D_801F8AAC = D_801F8AA8;
    }
    D_80176F94 = 10;
    D_801AB058++;
    D_801FAC60 = (u32 *)(&D_801FAC64[D_801FAD34] + (D_801F8AA8 * 0x3F));

    if (D_801AB05C != 0) {
        var_t0 = 0x11000000;
        *D_80176F5C = 0x20943;
        *D_80176F60 = 0x1323;
    } else {
        *D_80176F5C = 0x21020843;
        var_t0 = 0x11400100;
    }
    if ((D_8019390C[3] - 1) == D_8019390C[2]) {
        D_801B2D0C = 1;
        if (D_801FAC54 != 0) {
            sub_8013E6F0(D_801FAC60, (u32 *)((D_801B2DC8 << 0xB) + D_801FAC54 + 0x20), 0x1F8, 1);
            D_801B2DC8++;
        } else {
            sub_8013E724(3, (u32)D_801FAC60, 0, 0x1F8, var_t0, 1, 0);
        }
        D_801AB058 = 0;
        D_801A5B04 = 0;
        D_801B2B24 = D_801AB934;
    } else {
        if (D_801FAC54 != 0) {
            sub_8013E6F0(D_801FAC60, (u32 *)((D_801B2DC8 << 0xB) + D_801FAC54 + 0x20), 0x1F8, 0);
            D_801B2DC8++;
        } else {
            sub_8013E724(3, (u32)D_801FAC60, 0, 0x1F8, var_t0, 0, 0);
        }
    }
    *D_80176F60 = 0x1325;
    D_8019390C[0] = 3;
    D_801F8AA8 += 1;
    if ((D_801FAC54 != 0) && (D_801B2D0C != 0)) {
        sub_8013DAA8();
    }
}

/* @8013E6F0 mem2mem: copy n words (memory-stream replacement for dma_execute). */
void __attribute__((section(".text_8013E6F0"))) sub_8013E6F0(u32 *dst, u32 *src, u32 n, u32 unused) {
    u32 i;

    for (i = 0; i < n; i++) {
        *dst++ = *src++;
    }
}

/* @8013E724 dma_execute: wait for DMA channel ch to go idle, set its interrupt enable
 * (flag), enable it in DPCR and start a transfer. */
void __attribute__((section(".text_8013E724"))) sub_8013E724(s32 ch, u32 madr, u32 bh, u32 bl, u32 chcr, u8 flag, s32 unused) {
    volatile u32 dummy;
    volatile u32 *p;
    s32 i = 0;

    while (DMA[ch].chcr & 0x01000000) {
        if (i == 0x10000) {
            printf(D_800D0930, DMA[ch].chcr);
            break;
        }
        i++;
    }
    if (flag == 1) {
        ((u8 *)D_80176F68)[2] |= 1 << ch;
    } else {
        ((u8 *)D_80176F68)[2] &= ~(1 << ch);
    }
    dummy = *D_80176F68;
    *D_80176F64 |= 1 << (ch * 4 + 3);
    p = &DMA[ch].madr;
    *p++ = madr;
    *p++ = (bh << 16) | bl;
    *p = chcr;
    dummy = *p;
}

/* ---- event.c (sotn-decomp src/main/psxsdk/libcd/event.c) ---- */

/* @8014DB74 CdInit: reset the CD subsystem (up to five tries) and install the default
 * event-delivering callbacks. 1 ok, 0 failure. */
s32 __attribute__((section(".text_8014DB74"))) sub_8014DB74(void) {
    s32 i = 4;

    while (sub_8013A590(1) != 1) {
        if (--i == -1) {
            printf(D_800D0FC4);
            return 0;
        }
    }
    PadStopCom_1((CdlCB)sub_8014DC04);
    sub_8013A6FC((CdlCB)sub_8014DC2C);
    sub_8013A714((CdlCB)sub_8014DC54);
    return 1;
}
