#pragma once
#include "config.h"
namespace moss {
enum EffectKind { Orbit, Pond, Crush, Reel, Drive };
struct EffectSpec { const char *label; int low, high, coarse; const char *unit; };
inline constexpr EffectSpec FxSpecs[EffectTypes][EffectParameters] = {
  {{"TIME",0,7,1,"SYNC"},{"FEEDBACK",0,95,10,"%"},{"WIDTH",0,100,10,"%"},{"MIX",0,100,10,"%"},
   {"LOW CUT",0,2000,100,"HZ"},{"HIGH CUT",500,14000,1000,"HZ"},{"DRIVE",0,100,10,"%"},{"MOTION",0,100,10,"%"}},
  {{"SIZE",0,100,10,"%"},{"DECAY",0,100,10,"%"},{"DAMP",0,100,10,"%"},{"MIX",0,100,10,"%"}},
  {{"DRIVE",0,100,10,"%"},{"BITS",2,16,2,"BIT"},{"RATE",1,32,4,"HOLD"},{"MIX",0,100,10,"%"}},
  {{"WOBBLE",0,100,10,"%"},{"WEAR",0,100,10,"%"},{"NOISE",0,100,10,"%"},{"MIX",0,100,10,"%"}},
  {{"DRIVE",0,100,10,"%"},{"TONE",0,100,10,"%"},{"OUTPUT",0,150,10,"%"},{"MIX",0,100,10,"%"},
   {"CURVE",0,2,1,""},{"BIAS",-100,100,10,"%"},{"BODY",0,100,10,"%"},{"DYNAMICS",0,100,10,"%"}}
};
inline int fxBanks(int type) { return type == Orbit || type == Drive ? 2 : 1; }
inline int fxParameters(int type) { return fxBanks(type)*ParametersPerPage; }
inline const char *fxBankName(int bank) { return bank ? "CHARACTER" : "CORE"; }
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
struct Effect {
  int type=0,enabled=0;
  int values[EffectTypes][EffectParameters] = {
    {3,35,60,25,0,9000,15,0}, {60,60,45,25}, {20,12,2,25}, {20,30,10,35},
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
