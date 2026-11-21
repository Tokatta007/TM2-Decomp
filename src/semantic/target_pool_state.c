typedef unsigned char u8;typedef unsigned short u16;typedef unsigned int u32;typedef signed short s16;typedef signed int s32;
typedef struct TargetRecord{s16 type,instance,counter;u8 flag,pad;s32 x,y,z;}TargetRecord;
TargetRecord targetPool[50] __attribute__((section(".tm2_801909D0")))={0};
/* Timed event (target_actor_events.c) and second level event (target_secondary_events.c). */
typedef struct TM2TimedEvent{signed char active,done;s16 instance,clock,rate,boom,pad10;void *object;s32 pos[3];s16 dir[4],spin[4];}TM2TimedEvent;
typedef char tm2_timed_event_size[(sizeof(TM2TimedEvent)==44)?1:-1];
typedef struct TM2LevelEvent{signed char active,stage;s16 instance,instance2,clock,rate,boom;s32 pos[3];}TM2LevelEvent;
typedef char tm2_level_event_size[(sizeof(TM2LevelEvent)==24)?1:-1];
TM2TimedEvent targetTimedEvent __attribute__((section(".tm2_80190DB8")))={0};
TM2LevelEvent targetLevelEvent __attribute__((section(".tm2_80190DE4")))={0};
s32 targetTrackVehicleSlotA __attribute__((section(".tm2_80180BB0")))=0;
s32 targetTrackVehicleSlotB __attribute__((section(".tm2_80180BB4")))=0;
