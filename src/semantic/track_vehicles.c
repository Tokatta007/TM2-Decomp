/* Track vehicles (80117B40): the two vehicles that run around an oval
 * track (records trackVehicleRecords[0..1], 56 bytes, the same records trackVehicleGround
 * places on the ground). The "loop bounded by 2" in this function is these
 * two vehicles, not the players.
 *
 * While trackVehiclesShouldStop says they are stopped, a vehicle inside the station
 * window 3201..6400 snaps to 6400 (state 2) or 3200 and is shown (object
 * +8); the matching check for the -6400..-3200 window compares against
 * -6400 twice and never fires (retail bug, kept). Otherwise busy vehicles
 * run worldTrackVehiclePlace and leave the station window hidden.
 *
 * Each frame a vehicle moves 1520 / 100 per tick along its heading, then
 * an 8-state path keeps it on the oval: straights at x = -/+11584 (11776)
 * and y = 2304 / -7104 (-7296 / 2496), arcs of radius 3104 (3296) around
 * (-/+8480, -800) and (-/+8480, -4000). Vehicle 0 runs one way, vehicle 1
 * the other. Then trackVehicleGround grounds it, the record keeps the turn and
 * position deltas, and sound 506 / 507 plays at it. */
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    u8 pad0[8];
    s32 shown;             /* +8 */
    u8 pad12[116];
    u8 matrix[32];         /* +128 */
    s16 rotation[3];       /* +160 (rotation[2] = heading) */
    s16 pad166;
    s32 position[3];       /* +168 */
} TM2TrackObject;

typedef struct {
    s32 state;             /* +0 path state 0..7 */
    s32 speed;             /* +4 */
    s32 turn;              /* +8 heading change this frame */
    s32 busy;              /* +12 */
    s32 front[3];          /* +16 */
    s32 rear[3];           /* +28 */
    s32 move[3];           /* +40 movement this frame */
    TM2TrackObject *object;    /* +52 */
} TM2TrackRecord;
typedef char tm2_track_record_size[(sizeof(TM2TrackRecord) == 56) ? 1 : -1];

extern TM2TrackRecord *trackVehicleRecords;
extern u8 trackVehiclesShouldStop(void);
extern void worldTrackVehiclePlace(s32 index);
extern s32 timerGetFrameTicks(void);
extern void mathMulTransVec(void *matrix, s32 *in, s32 *out);
extern int ratan2(int, int);
extern int rsin(int);
extern int rcos(int);
extern void mathBoundSAngle(s16 *);
extern void trackVehicleGround(s32 index);
extern void targetSetTrackVehiclePos(s32 sound, s32 *pos);

#define R0 trackVehicleRecords[0]
#define R1 trackVehicleRecords[1]
#define O0 trackVehicleRecords[0].object
#define O1 trackVehicleRecords[1].object

#pragma tm2_rdata .tm2_800CF270

/* @80117B40 */
void __attribute__((section(".text_80117B40"))) worldTrackVehiclesUpdate(void)
{
    s32 i, a, dx, dy, s, c, prev;
    s32 old[3];
    s32 local[3];
    u8 out;

    if (trackVehiclesShouldStop()) {
        for (i = 0; i < 2; i++) {
            if (!trackVehicleRecords[i].busy && !trackVehicleRecords[i].object->shown) {
                if (trackVehicleRecords[i].object->position[0] < 6401) {
                    if (trackVehicleRecords[i].object->position[0] >= 3201) {
                        if (trackVehicleRecords[i].state == 2)
                            trackVehicleRecords[i].object->position[0] = 6400;
                        else
                            trackVehicleRecords[i].object->position[0] = 3200;
                    } else if (trackVehicleRecords[i].object->position[0] >= -6400
                               && trackVehicleRecords[i].object->position[0] < -6400) {
                        if (trackVehicleRecords[i].state == 2)
                            trackVehicleRecords[i].object->position[0] = -3200;
                        else
                            trackVehicleRecords[i].object->position[0] = -6400;
                    }
                }
                trackVehicleRecords[i].object->shown = 1;
            }
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (trackVehicleRecords[i].busy) {
                worldTrackVehiclePlace(i);
                trackVehicleRecords[i].busy = 0;
            } else if (trackVehicleRecords[i].object->shown) {
                out = 0;
                if (trackVehicleRecords[i].object->position[0] >= 6401
                    || (trackVehicleRecords[i].object->position[0] < 3201
                        && (trackVehicleRecords[i].object->position[0] >= -3199
                            || trackVehicleRecords[i].object->position[0] < -6400)))
                    out = 1;
                if (out)
                    trackVehicleRecords[i].object->shown = 0;
            }
        }
    }

    if (O0) {
        old[0] = O0->position[0];
        old[1] = O0->position[1];
        old[2] = O0->position[2];
        local[0] = 0;
        R0.speed = local[1] = (s16)timerGetFrameTicks() * 1520 / 100;
        local[2] = 0;
        mathMulTransVec(O0->matrix, local, R0.move);
        O0->position[0] += R0.move[0];
        O0->position[1] += R0.move[1];
        prev = O0->rotation[2];
        switch (R0.state) {
        case 0:
            O0->position[0] = -11584;
            O0->rotation[2] = 0;
            if (O0->position[1] >= -799)
                R0.state = 1;
            break;
        case 1:
            dx = O0->position[0] + 8480;
            dy = O0->position[1] + 800;
            a = ratan2(dx, dy);
            O0->rotation[2] = a + 1024;
            s = 3104 * rsin(a) / 4096;
            c = 3104 * rcos(a) / 4096;
            O0->position[0] -= dx - s;
            O0->position[1] -= dy - c;
            if (O0->position[0] >= -8479)
                R0.state = 2;
            break;
        case 2:
            O0->position[1] = 2304;
            O0->rotation[2] = 1024;
            if (O0->position[0] >= 8481)
                R0.state = 3;
            break;
        case 3:
            dx = O0->position[0] - 8480;
            dy = O0->position[1] + 800;
            a = ratan2(dx, dy);
            O0->rotation[2] = a + 1024;
            s = 3104 * rsin(a) / 4096;
            c = 3104 * rcos(a) / 4096;
            O0->position[0] -= dx - s;
            O0->position[1] -= dy - c;
            if (O0->position[1] < -800)
                R0.state = 4;
            break;
        case 4:
            O0->position[0] = 11584;
            O0->rotation[2] = 2048;
            if (O0->position[1] < -4000)
                R0.state = 5;
            break;
        case 5:
            dx = O0->position[0] - 8480;
            dy = O0->position[1] + 4000;
            a = ratan2(dx, dy);
            O0->rotation[2] = a + 1024;
            s = 3104 * rsin(a) / 4096;
            c = 3104 * rcos(a) / 4096;
            O0->position[0] -= dx - s;
            O0->position[1] -= dy - c;
            if (O0->position[0] < 8480)
                R0.state = 6;
            break;
        case 6:
            O0->position[1] = -7104;
            O0->rotation[2] = -1024;
            if (O0->position[0] < -8480)
                R0.state = 7;
            break;
        case 7:
            dx = O0->position[0] + 8480;
            dy = O0->position[1] + 4000;
            a = ratan2(dx, dy);
            O0->rotation[2] = a + 1024;
            s = 3104 * rsin(a) / 4096;
            c = 3104 * rcos(a) / 4096;
            O0->position[0] -= dx - s;
            O0->position[1] -= dy - c;
            if (O0->position[1] >= -3999)
                R0.state = 0;
            break;
        }
        trackVehicleGround(0);
        R0.turn = O0->rotation[2] - prev;
        R0.move[0] = O0->position[0] - old[0];
        R0.move[1] = O0->position[1] - old[1];
        R0.move[2] = O0->position[2] - old[2];
        targetSetTrackVehiclePos(506, O0->position);
    }

    if (O1) {
        old[0] = O1->position[0];
        old[1] = O1->position[1];
        old[2] = O1->position[2];
        local[0] = 0;
        R1.speed = local[1] = (s16)timerGetFrameTicks() * 1520 / 100;
        local[2] = 0;
        mathMulTransVec(O1->matrix, local, R1.move);
        O1->position[0] += R1.move[0];
        O1->position[1] += R1.move[1];
        prev = O1->rotation[2];
        switch (R1.state) {
        case 0:
            O1->position[0] = -11776;
            O1->rotation[2] = 2048;
            if (O1->position[1] < -4000)
                R1.state = 1;
            break;
        case 1:
            dx = O1->position[0] + 8480;
            dy = O1->position[1] + 4000;
            a = ratan2(dx, dy);
            O1->rotation[2] = a - 1024;
            mathBoundSAngle(&O1->rotation[2]);
            s = 3296 * rsin(a) / 4096;
            c = 3296 * rcos(a) / 4096;
            O1->position[0] -= dx - s;
            O1->position[1] -= dy - c;
            if (O1->position[0] >= -8479)
                R1.state = 2;
            break;
        case 2:
            O1->position[1] = -7296;
            O1->rotation[2] = 1024;
            if (O1->position[0] >= 8481)
                R1.state = 3;
            break;
        case 3:
            dx = O1->position[0] - 8480;
            dy = O1->position[1] + 4000;
            a = ratan2(dx, dy);
            O1->rotation[2] = a - 1024;
            s = 3296 * rsin(a) / 4096;
            c = 3296 * rcos(a) / 4096;
            O1->position[0] -= dx - s;
            O1->position[1] -= dy - c;
            if (O1->position[1] >= -3999)
                R1.state = 4;
            break;
        case 4:
            O1->position[0] = 11776;
            O1->rotation[2] = 0;
            if (O1->position[1] >= -799)
                R1.state = 5;
            break;
        case 5:
            dx = O1->position[0] - 8480;
            dy = O1->position[1] + 800;
            a = ratan2(dx, dy);
            O1->rotation[2] = a - 1024;
            s = 3296 * rsin(a) / 4096;
            c = 3296 * rcos(a) / 4096;
            O1->position[0] -= dx - s;
            O1->position[1] -= dy - c;
            if (O1->position[0] < 8480)
                R1.state = 6;
            break;
        case 6:
            O1->position[1] = 2496;
            O1->rotation[2] = -1024;
            if (O1->position[0] < -8480)
                R1.state = 7;
            break;
        case 7:
            dx = O1->position[0] + 8480;
            dy = O1->position[1] + 800;
            a = ratan2(dx, dy);
            O1->rotation[2] = a - 1024;
            s = 3296 * rsin(a) / 4096;
            c = 3296 * rcos(a) / 4096;
            O1->position[0] -= dx - s;
            O1->position[1] -= dy - c;
            if (O1->position[1] < -800)
                R1.state = 0;
            break;
        }
        trackVehicleGround(1);
        R1.turn = O1->rotation[2] - prev;
        R1.move[0] = O1->position[0] - old[0];
        R1.move[1] = O1->position[1] - old[1];
        R1.move[2] = O1->position[2] - old[2];
        targetSetTrackVehiclePos(507, O1->position);
    }
}
