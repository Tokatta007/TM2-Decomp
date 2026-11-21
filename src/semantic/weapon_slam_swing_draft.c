#include "semantic/cars.h"   /* u8..u32, car record blocks, TM2CarSlam, TM2Object */
/* Kept for input +0x1E: the two input bytes +0x1E/+0x1F are read as one halfword. */
#define U16(p,o) (*(u16 *)((u8 *)(p)+(o)))
#define HUMAN ((struct TM2HumanRecord *)car)
#define AI ((struct TM2AIRecord *)car)
/* targetPoolGet records (20 bytes) and targetActorsGet records (60 bytes). */
struct CamNearTarget { s16 a, b, active, pad; s32 pos[3]; };
struct CamTargetActor { u8 pad0[4]; s16 state; u8 pad6[50]; struct TM2Object *object; };
extern s32 weaponHitSpinDefault[3];
extern void weaponSlamCarryTarget(s16,void *,void *,s32,s32);
extern s32 viewGetSplitMode(void);extern void soundLocateForView0(void *);extern void soundLocateForView(void *,s32);extern s32 PadChkMtap(void);extern s32 soundGetListenerPan(void);extern void soundPlay(s32,s32,s32,s32);
extern s32 slamClawGetAngle(void);extern s32 slamArmGetAngle(void);extern s16 timerGetFrameTicks(void);extern void mathBoundAngle(s32 *,s32);extern void slamClawSetAngle(s32);extern void slamArmSetAngle(s32);
/* carSpinOutByObject, carDamageByObject: semantic/car_calls.h (via cars.h) */extern void mathMulTransVec(void *,s32 *,s32 *);extern void *targetPoolGet(void);extern void targetDamage(s16,s16,s32);extern void *targetActorsGet(void);extern void targetActorHit(s32,s32 *);extern s32 weaponSlamFindTarget(void *,s32);extern void weaponSlamRelease(void *,s32);
static __inline__ s32 ab(s32 v){return v<0?-v:v;}
static __inline__ s32 dist(const s32 *a,const s32 *b){s32 x=ab(a[0]-b[0]),y=ab(a[1]-b[1]),z=ab(a[2]-b[2]),d=x;if(d<y)d=y;if(d<z)d=z;return d>>3;}
static __inline__ void feedback(struct TM2CarMotion *motion,s32 command,s32 side){s32 a=0,b=0;if(!side){if(viewGetSplitMode()==0)soundLocateForView0(motion->pos);else soundLocateForView(motion->pos,-1);a=PadChkMtap();b=soundGetListenerPan();}soundPlay(command,a,b,0x63);}
/* @800F4058. Advance one side's staged camera/action sequence. */
void __attribute__((section(".text_800F4058"))) weaponSlamSwingUpdate(u8 *car,s32 side)
{
 struct TM2CarSlam *state;struct TM2CarStatsBlock *geometry;struct TM2CarMotion *motion;struct TM2CarInput *input;struct TM2CarState *flags;
 struct CamNearTarget *near;struct CamTargetActor *actors;struct TM2Object *obj;u16 car_id;s32 angle,pos[3],local[3],rest[3],i;
 if(side&0xff){state=HUMAN->weapons.slam;geometry=&HUMAN->stats;car_id=HUMAN->slot;motion=&HUMAN->motion;input=&HUMAN->input;flags=&HUMAN->state;}
 else{state=AI->weapons.slam;geometry=&AI->stats;car_id=AI->slot;motion=&AI->motion;input=&AI->input;flags=&AI->state;}
 if(state->swing==0)return;
 if(flags->frozen!=0){if(state->spin_pending!=0)weaponSlamCarryTarget(state->spin_target,geometry,motion,0,0);return;}
 if(state->f03!=0){
  feedback(motion,0x39,side&0xff);angle=slamClawGetAngle();
  if(angle>=-681){angle-=timerGetFrameTicks()*34;mathBoundAngle(&angle,angle);if(angle<-682)angle=-682;slamClawSetAngle(angle);angle=slamArmGetAngle();angle-=timerGetFrameTicks()*5;mathBoundAngle(&angle,angle);slamArmSetAngle(angle);return;}
  state->f03=0;state->f05=1;return;
 }
 if(state->f05!=0){state->hold_timer+=timerGetFrameTicks();if(U16(input,0x1e)==0||(s16)state->hold_timer>=601){state->f05=0;state->f04=1;}}
 else if(state->f04!=0){
  angle=slamArmGetAngle();angle+=timerGetFrameTicks()*79;mathBoundAngle(&angle,angle);
  if(angle>=911){
   state->f04=0;angle=910;
   if(state->spin_pending){carSpinOutByObject(state->spin_target,1,1);carDamageByObject(state->spin_target,(s16)car_id,state->half_damage?6:12);i=1;}
   else{feedback(motion,5,side&0xff);i=0;}
   carSpinOutByObject(0xc0,1,i);local[0]=0;local[1]=geometry->axle_front+120;local[2]=0;mathMulTransVec(&motion->rot_m,local,pos);pos[0]+=motion->pos[0];pos[1]+=motion->pos[1];pos[2]+=motion->pos[2];
   near=(struct CamNearTarget *)targetPoolGet();for(i=0;i<50;i++,near++)if(near->active>0&&dist(pos,near->pos)<15)targetDamage(near->a,near->b,0x19);
   actors=(struct CamTargetActor *)targetActorsGet();for(i=0;i<6;i++,actors++)if(actors->state!=-1){obj=actors->object;if(dist(pos,obj->pos)<15){rest[0]=weaponHitSpinDefault[0];rest[1]=weaponHitSpinDefault[1];rest[2]=weaponHitSpinDefault[2];targetActorHit(i,rest);}}
  }else if(angle>=569){
   if(state->spin_pending)weaponSlamCarryTarget(state->spin_target,geometry,motion,0,0);
   else if(weaponSlamFindTarget(car,side&0xff)&0xff){state->f04=0;carSpinOutByObject(state->spin_target,1,1);carDamageByObject(state->spin_target,(s16)car_id,state->half_damage?12:25);carSpinOutByObject(0xc0,1,1);}
  }
  if(state->spin_pending)carSpinOutByObject(state->spin_target,0,1);slamArmSetAngle(angle);return;
 }
 angle=slamClawGetAngle();if(angle<739){angle+=timerGetFrameTicks()*45;mathBoundAngle(&angle,angle);}slamClawSetAngle(angle);
 angle=slamArmGetAngle();if(angle>=-112){angle-=timerGetFrameTicks()*34;mathBoundAngle(&angle,angle);}else if(state->spin_pending)carSpinOutByObject(state->spin_target,0,1);else if(weaponSlamFindTarget(car,side&0xff)&0xff)carSpinOutByObject(state->spin_target,0,1);
 slamArmSetAngle(angle);
 if(slamClawGetAngle()>=739&&slamArmGetAngle()<-112){slamClawSetAngle(739);slamArmSetAngle(-113);weaponSlamRelease(car,side&0xff);}
}
