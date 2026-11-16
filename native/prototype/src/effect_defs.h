#pragma once
#include "config.h"
#include <stdint.h>
namespace moss {
enum EffectKind { Orbit, Pond, Crush, Reel, Drive };
enum ReelParameter { ReelDrive, ReelTone, ReelBump, ReelAmount,
  ReelWow, ReelWowRate, ReelFlutter, ReelFlutterRate,
  ReelDrift, ReelStereo, ReelLoss, ReelDrop,
  ReelHiss, ReelHissTone, ReelFollow, ReelOutput };
inline constexpr int FxCounts[]={8,8,4,16,8}, FxOffsets[]={0,8,16,20,36}, FxLockCount=44;
struct FxLocks {
  int16_t data[FxLockCount];
  FxLocks(){for(auto &v:data)v=Inherit;}
  int16_t *operator[](int type){return data+FxOffsets[type];}
  const int16_t *operator[](int type)const{return data+FxOffsets[type];}
};
static_assert(sizeof(FxLocks)==88,"Compact FX lock storage");
struct EffectSpec { const char *label; int low, high, coarse; const char *unit; };
inline constexpr EffectSpec FxSpecs[EffectTypes][EffectParameters] = {
  {{"TIME",0,7,1,"SYNC"},{"FEEDBACK",0,95,10,"%"},{"WIDTH",0,100,10,"%"},{"MIX",0,100,10,"%"},
   {"LOW CUT",0,2000,100,"HZ"},{"HIGH CUT",500,14000,1000,"HZ"},{"DRIVE",0,100,10,"%"},{"MOTION",0,100,10,"%"}},
  {{"SIZE",0,100,10,"%"},{"DECAY",0,100,10,"SEC"},{"DAMP",0,100,10,"%"},{"MIX",0,100,10,"%"},
   {"PREDELAY",0,200,10,"MS"},{"DIFFUSE",0,100,10,"%"},{"MOTION",0,100,10,"%"},{"LOW CUT",0,1000,100,"HZ"}},
  {{"DRIVE",0,100,10,"%"},{"BITS",2,16,2,"BIT"},{"RATE",1,32,4,"HOLD"},{"MIX",0,100,10,"%"}},
  {{"DRIVE",0,100,10,"%"},{"TONE",-60,60,10,"dB"},{"BUMP",0,60,10,"dB"},{"AMOUNT",0,100,10,"%"},
   {"WOW",0,150,10,"CENTS"},{"WOW RATE",10,400,10,"Hz"},
   {"FLUTTER",0,150,10,"CENTS"},{"FLT RATE",20,300,10,"Hz"},
   {"DRIFT",0,100,10,"%"},{"STEREO",0,100,10,"%"},{"LOSS",0,100,10,"%"},{"DROP",0,100,10,"%"},
   {"HISS",0,49,6,"dBFS"},{"HISS TONE",-100,100,10,"%"},{"FOLLOW",0,100,10,"%"},{"OUTPUT",-120,60,10,"dB"}},
  {{"DRIVE",0,100,10,"%"},{"TONE",0,100,10,"%"},{"OUTPUT",0,150,10,"%"},{"MIX",0,100,10,"%"},
   {"CURVE",0,2,1,""},{"BIAS",-100,100,10,"%"},{"BODY",0,100,10,"%"},{"DYNAMICS",0,100,10,"%"}}
};
inline int fxBanks(int type) { return FxCounts[type]/ParametersPerPage; }
inline int fxParameters(int type) { return fxBanks(type)*ParametersPerPage; }
inline const char *fxBankName(int bank,int type=Orbit) {
  const char *reel[]={"COLOR","MOTION","LIFE","OUTPUT"};
  return type==Reel?reel[bank]:type==Pond?(bank?"SPACE":"TAIL"):bank?"CHARACTER":"CORE";
}
inline const char *driveCurveName(int curve) {
  const char *names[]={"ROUND","BITE","EDGE"}; return names[curve];
}
inline int fxLow(int type,int slot) { return FxSpecs[type][slot].low; }
inline int fxHigh(int type,int slot) { return FxSpecs[type][slot].high; }
inline int fxCoarseStep(int type,int slot) { return FxSpecs[type][slot].coarse; }
inline const char *fxLabel(int type,int slot) { return FxSpecs[type][slot].label; }
inline const char *fxName(int type) {
  const char *names[]={"ORBIT","POND","CRUSH","REEL","DRIVE"}; return names[type];
}
inline constexpr int ReelPresetCount=5;
inline constexpr const char *ReelPresetNames[]={"CLEAN","STUDIO","WARM CASSETTE","WIDE DRIFT","WORN LOOP"};
inline constexpr int ReelPresets[ReelPresetCount][16]={
  {0,0,0,100,0,30,0,60,0,0,0,0,0,0,100,0},
  {25,0,10,100,10,45,6,90,5,0,0,0,0,0,100,0},
  {55,-15,25,100,35,35,20,85,20,10,12,0,15,-35,100,-10},
  {20,5,5,100,65,18,10,100,85,90,0,0,9,-20,100,0},
  {72,-25,18,100,90,24,65,160,70,30,50,65,25,-60,80,-20}
};
struct Effect {
  int type=0,enabled=0;
  int values[EffectTypes][EffectParameters] = {
    {3,35,60,25,0,9000,15,0}, {60,60,45,25,20,75,20,0}, {20,12,2,25},
    {0,0,0,100,0,30,0,60,0,0,0,0,0,0,100,0},
    {30,50,100,100,0,0,50,0}};
  int enabledTypes[EffectTypes]={},order[EffectTypes]={0,1,2,3,4};
  bool active(int id) const { return id==type ? enabled!=0 : enabledTypes[id]!=0; }
  int count() const { int n=0; for(int i=0;i<EffectTypes;++i) n+=active(i); return n; }
  bool enable(bool on) {
    if(on&&!enabled&&count()>=ActiveEffects) return false;
    enabled=enabledTypes[type]=on; return true;
  }
  void constrain() {
    for(int id:order) if(id<0||id>=EffectTypes) return;
    int n=0; for(int id:order) if(active(id)&&++n>ActiveEffects) {
      enabledTypes[id]=0; if(id==type) enabled=0;
    }
  }
  void select(int id) { enabledTypes[type]=enabled; type=id; enabled=enabledTypes[id]; }
  void move(int id,int direction) {
    for(int i=0;i<EffectTypes;++i) if(order[i]==id) {
      int to=i+direction;
      if(to>=0&&to<EffectTypes) { int old=order[to]; order[to]=id; order[i]=old; }
      break;
    }
  }
};
} // namespace moss
