/* Small game helpers (800D..8010 range). Names stay address-based. */
#include "semantic/cars.h"
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"   /* request 48 stage 2: ONCE_PER_STEP (fxSheetDisplay) */
#endif

typedef struct {
    s16 f00;
    s16 f02;
    s16 f04;
    s16 f06;
    s16 f08;
    u8 pad_0A[0x34 - 0x0A];
    u8 *f34;
    u8 pad_38[0x44 - 0x38];
    u8 f44;
    u8 pad_45[0xF0 - 0x45];
    u8 *fF0;
    u8 pad_F4[980 - 0xF4];
} TM2Rec980;

typedef struct {
    s16 a, b, c, d;
} TM2Quad16;

typedef struct {
    u8 pad_00[4];
    u8 r, g, b;
    u8 pad_07[24 - 7];
} TM2Prim24;

/* Ground surface (28 bytes at worldSurfaces, see car_wheels.c TM2Surface). */
typedef struct {
    u8 type;                /* +0x00 */
    u8 f01;                 /* +0x01 */
    s16 section;            /* +0x02 track section the surface belongs to */
    s16 w, l;               /* +0x04 */
    s16 sn, cs;             /* +0x08 */
    s16 h[4];               /* +0x0C corner heights */
    s32 x, y;               /* +0x14 origin */
} TM2Surface;
typedef char tm2_surface_size[(sizeof(TM2Surface) == 28) ? 1 : -1];

typedef struct {
    u8 data[8];
} TM2Sprite8;

typedef void (*TM2VolFn)(u8 *, s32);

extern s32 audioAltMusicMode;
extern u8 *musicManagerObject;
#pragma tm2_absolute musicManagerObject
extern s32 audioCdVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern char strDigitsFill[];
extern s32 bulletsFrameParity, weaponHomingMissileModel, weaponFreezeMissileModel, weaponNapalmModel, weaponColoredMissileModel;
extern s32 weaponGhostMissileModel, weaponBoomerangModel, weaponGrimmSpecialModel, weaponSpiralMissileModel, fxSheetTimer;
extern s32 terrainSurfaceCount;
extern TM2Quad16 weaponGhostMissileColor;
#pragma tm2_absolute weaponGhostMissileColor
extern TM2Rec980 fxRecords[];
extern TM2Prim24 fxSheetPolys[];
extern TM2Surface worldSurfaces[];
extern TM2Sprite8 hudRadarArrowUpSprites[];
#if TM2_MAX_HUDS > 2
extern TM2Sprite8 hudRadarArrowDownSprites[], hudRadarArrowLeftSprites[], hudRadarArrowRightSprites[];
#endif

extern s32 audioGetMusicVolume(void);
extern void SsSetSerialVol(char s_num, short voll, short volr);
extern int sprintf(char *, const char *, ...);
extern int rand(void);
extern s16 weaponSpawnProjectile(s32, s32, s32, s32);
extern s16 weaponFireHomingMissile(s32, s32, s32, s32, s32);
extern s16 weaponFireColoredMissile(s32, s32, s32, s32, s32, TM2Quad16 *);
extern void missileSetTrailColor(s32, s32, s32, s32);
extern void AddPrim(void *, void *);
extern u8 *carObjectFromOwner(s32);
extern u8 carFindByObject(u8 *, u8 *, s32 *);
extern s32 carObjectId(s32, u8 human);
extern void carApplyDamage(s32, s32, s32 *, s32, s32 attacker);
extern int grutilsParse2DSprite(s32, TM2Sprite8 *, s32);
extern int rsin(s32);
extern int rcos(s32);

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] -- stored
 * as retail does it, min then max -- and apply it. Always inlined. */
static __inline__ void SetSerialVolume(s32 v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
    SsSetSerialVol(0, (s16)audioCdVolume, (s16)audioCdVolume);
}

/* Toggle the sound mode between 10 and 11, notify the sound object, then
 * re-read and clamp the serial (CD audio) volume and apply it. */
void __attribute__((section(".text_800D3944"))) audioAlternateMusicTrack(void)
{
    u8 *object;
    u8 *vtable;
    s32 v;
    s32 x;

    if (audioAltMusicMode != 10)
        audioAltMusicMode = 10;
    else
        audioAltMusicMode = 11;
    object = musicManagerObject;
    vtable = *(u8 **)(object + 8);
    (*(TM2VolFn *)(vtable + 20))(object + *(s16 *)(vtable + 16), audioAltMusicMode);
    SetSerialVolume(audioGetMusicVolume());
}

/* Fade the serial volume down from n-1 to 0, one step per busy-wait of
 * 800 sprintf calls, clamping to the volume range each step. The delay is
 * a goto loop: a real loop weights i's refs by loop depth and swaps the
 * s0/s1 allocation of n and i. */
void __attribute__((section(".text_800D3A44"))) audioFadeOutMusic(s32 n)
{
    char buf[24];
    s32 i;

    i = 800;
loop:
    if (n <= 0)
        return;
    n--;
    SetSerialVolume(n);
    if (i > 0) {
    again:
        sprintf(buf, strDigitsFill);
        i--;
        if (i > 0)
            goto again;
    }
    i = 800;
    goto loop;
}

/* Pick a weighted random slot (0..9) from the AI car's 10-byte weight table
 * and set its weapon kind (selected) and action code (f0C). */
void __attribute__((section(".text_800D420C"))) aiPickWeaponFromTable10(struct TM2AIRecord *ai)
{
    s32 r;
    s32 sum;
    s32 i;
    u8 *w;

    sum = 0;
    r = rand() / 328;
    i = 0;
    w = ai->weapons.weights10;
    for (; i < 10; i++) {
        sum += *w;
        if (r < sum)
            break;
        w++;
    }
    if (i < 8) {
        ai->weapons.selected = i;
        ai->weapons.f0C = 0x6B;
    } else if (i == 9) {
        ai->weapons.selected = 8;
        ai->weapons.f0C = 0x65;
    } else if (i == 8) {
        ai->weapons.selected = 8;
        ai->weapons.f0C = 0x64;
    } else {
        ai->weapons.selected = 8;
        ai->weapons.f0C = 0x6B;
    }
}

/* Store a value into the global slot selected by id. */
void __attribute__((section(".text_800D6F00"))) weaponStoreProjectileModel(s32 value, s32 id)
{
    switch (id) {
    case 0x899:
        weaponHomingMissileModel = value;
        weaponColoredMissileModel = value;
        break;
    case 0x89A:
        weaponFreezeMissileModel = value;
        break;
    case 0x8A0:
        weaponNapalmModel = value;
        break;
    case 0x911:
        weaponBoomerangModel = value;
        break;
    case 0x912:
        weaponGrimmSpecialModel = value;
        break;
    case 0x916:
        weaponGhostMissileModel = value;
        break;
    case 0x8FD:
        weaponSpiralMissileModel = value;
        break;
    }
}

/* Spawn a record via weaponSpawnProjectile and set it up (timer 150, flag, state 0x2B). */
s32 __attribute__((section(".text_800D9A08"))) weaponFireGrimmSpecial(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4)
{
    s32 idx;
    s32 v;

    idx = weaponSpawnProjectile(a0, a2, a3, a4);
    if (idx < 0)
        return idx;
    if (weaponGrimmSpecialModel != 0)
        *(s32 *)(fxRecords[idx].f34 + 0x11C) = weaponGrimmSpecialModel;
    fxRecords[idx].f06 = 150;
    fxRecords[idx].f44 = 1;
    *(s32 *)(fxRecords[idx].fF0 + 8) = 0;
    fxRecords[idx].f02 = 0x2B;
    return idx;
}

/* Spawn a record via weaponFireHomingMissile and set it up (state 0x3D). */
s32 __attribute__((section(".text_800D9C5C"))) weaponFireSpiralMissile(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4)
{
    s32 idx;

    idx = weaponFireHomingMissile(a0, a1, a2, a3, a4);
    if (idx < 0)
        return idx;
    fxRecords[idx].f08 = 15;
    fxRecords[idx].f06 = 0x25;
    fxRecords[idx].f04 = fxRecords[idx].f04 * 4;
    missileSetTrailColor(idx, 0xFF, 0, 150);
    fxRecords[idx].f02 = 0x3D;
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35: the model word is cleared at each load and set only by a loaded Sweet Tooth / Dark Tooth file: a
       missile without it keeps weaponFireHomingMissile's model (as Mr. Grimm's, Spectre's and Roadkill's do) */
    if (weaponSpiralMissileModel != 0)
#endif
    *(s32 *)(fxRecords[idx].f34 + 0x11C) = weaponSpiralMissileModel;
    return idx;
}

/* Spawn a record via weaponFireColoredMissile with the default quad and set it up
 * (state 0x31). */
s32 __attribute__((section(".text_800D9E60"))) weaponFireGhostMissile(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4)
{
    TM2Quad16 q;
    s32 idx;

    q = weaponGhostMissileColor;
    idx = weaponFireColoredMissile(a0, a1, a2, a3, a4, &q);
    if (idx >= 0) {
        fxRecords[idx].f08 = 100;
        fxRecords[idx].f06 = 75;
        fxRecords[idx].f04 = 80;
        fxRecords[idx].f02 = 0x31;
        if (weaponGhostMissileModel != 0)
            *(s32 *)(fxRecords[idx].f34 + 0x11C) = weaponGhostMissileModel;
    }
    return idx;
}

/* fxSheetDisplay: add the current full-screen FX prim to the OT; once the
 * timer drops below 8, fade its colour toward grey each frame. */
void __attribute__((section(".text_800DC590"))) fxSheetDisplay(void *ot)
{
    s32 r, g, b;
#ifdef TM2_PORT_ROLLBACK
    s32 pass;

    /* request 48 stage 2 (ONLINE_ROLLBACK.md 11, ONCE_PER_STEP): the step's call (ot 0) counts down and fades, a
       view's call only adds the quad; without the bit both, as retail */
    pass = portRollbackPass(ot == 0);
    if (pass != TM2_RB_PASS_TICK)
#endif
    AddPrim(ot, &fxSheetPolys[bulletsFrameParity]);
#ifdef TM2_PORT_ROLLBACK
    if (pass == TM2_RB_PASS_DRAW)
        return;
#endif
    if (--fxSheetTimer < 8) {
        r = (fxSheetPolys[0].r * 3 + 128) >> 2;
        g = (fxSheetPolys[0].g * 3 + 128) >> 2;
        b = (fxSheetPolys[0].b * 3 + 128) >> 2;
        fxSheetPolys[0].r = r;
        fxSheetPolys[0].g = g;
        fxSheetPolys[0].b = b;
        fxSheetPolys[1].r = r;
        fxSheetPolys[1].g = g;
        fxSheetPolys[1].b = b;
    }
}

/* Look up an object, resolve its model data and hand it to carApplyDamage. */
void __attribute__((section(".text_800F4B74"))) carDamageByObject(s32 key, s32 a1, s32 a2)
{
    u8 *obj;
    u8 flag;
    s32 id;
    struct TM2CarMotion *motion;

    obj = carObjectFromOwner(key);
    if (obj == 0)
        return;
    if (!carFindByObject(obj, &flag, &id))
        return;
    if (flag)
        motion = &humanCarGet(id)->motion;
    else
        motion = &aiCarGet(id)->motion;
    carApplyDamage(carObjectId(id, flag), a2, motion->body_rot, 2, (s16)a1);   /* retail narrows the attacker */
}

/* Parse the two 2D sprites for the given id from the sprite source. Each
 * case makes both calls (cross-jumping merges the tails); one shared call
 * after the switch schedules the last case differently. */
void __attribute__((section(".text_800FD00C"))) hudRadarArrowsLoad(s32 id, s32 src)
{
#if TM2_MAX_HUDS > 2
    /* one copy of the arrow per HUD; the four arrays are separate (retail
       reaches Down/Left/Right as UpSprites[2..7], packed [4][2]) */
    TM2Sprite8 *sprites;
    s32 i;

    switch (id) {
    case 0x50A:
        sprites = hudRadarArrowUpSprites;
        break;
    case 0x50C:
        sprites = hudRadarArrowDownSprites;
        break;
    case 0x50D:
        sprites = hudRadarArrowLeftSprites;
        break;
    case 0x50B:
        sprites = hudRadarArrowRightSprites;
        break;
    default:
        return;
    }
    for (i = 0; i < TM2_MAX_HUDS; i++)
        grutilsParse2DSprite(src, &sprites[i], 1);
#else
    switch (id) {
    case 0x50A:
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[0], 1);
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[1], 1);
        break;
    case 0x50C:
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[2], 1);
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[3], 1);
        break;
    case 0x50D:
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[4], 1);
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[5], 1);
        break;
    case 0x50B:
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[6], 1);
        grutilsParse2DSprite(src, &hudRadarArrowUpSprites[7], 1);
        break;
    }
#endif
}

/* Fill record (count - (n+1)) with sin/cos of angle, a position and sizes. */
void __attribute__((section(".text_80100398"))) trackVehicleSurfaceSet(s32 n, s32 *pos, s32 size, s32 angle)
{
    s32 i;

    n++;
    i = terrainSurfaceCount - n;
    worldSurfaces[i].sn = rsin(angle);
    worldSurfaces[i].cs = rcos(angle);
    worldSurfaces[i].x = pos[0];
    worldSurfaces[i].y = pos[1];
    worldSurfaces[i].h[0] = pos[2];
    worldSurfaces[i].h[1] = pos[2];
    worldSurfaces[i].h[2] = size;     /* moving platform: h[0..1] height, h[2..3] size */
    worldSurfaces[i].h[3] = size;
}
