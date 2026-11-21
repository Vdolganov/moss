#pragma once
#include "organism_dsp.h"
namespace moss {
struct OrganismView {
  int active=0,gate=0,amp=0,shape=0,couple=0,grit=0,sub=0;
  int phase[4]={},motion[4]={},energy[4]={},pan[4]={};
};
struct OrganismMailbox {
  static constexpr unsigned Words=23;
  uint32_t sequence=0;int words[Words]={};
  void publish(const OrganismDSP &v,bool selected) {
    unsigned serial=__atomic_load_n(&sequence,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+1,__ATOMIC_SEQ_CST);
    int data[Words]={selected&&v.active,selected&&v.gate,int(v.env*1000),int(v.shape*1000),int(v.couple*1000),int(v.p[OrgGrit]),int(v.subGain*1000)};
    for(int i=0;i<4;++i){data[7+i]=int(v.slowPhase[i]*1000);data[11+i]=int(v.motion[i]*1000);data[15+i]=int(v.energy[i]*v.env*v.gain*1000);data[19+i]=int(v.pan[i]*1000);}
    for(unsigned i=0;i<Words;++i)__atomic_store_n(&words[i],data[i],__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+2,__ATOMIC_RELEASE);
  }
  OrganismView read() const {
    for(int attempt=0;attempt<3;++attempt){
      unsigned before=__atomic_load_n(&sequence,__ATOMIC_ACQUIRE);if(before&1)continue;
      int data[Words];for(unsigned i=0;i<Words;++i)data[i]=__atomic_load_n(&words[i],__ATOMIC_RELAXED);
      __atomic_thread_fence(__ATOMIC_ACQUIRE);if(before!=__atomic_load_n(&sequence,__ATOMIC_RELAXED))continue;
      OrganismView v;v.active=data[0];v.gate=data[1];v.amp=data[2];v.shape=data[3];v.couple=data[4];v.grit=data[5];v.sub=data[6];
      for(int i=0;i<4;++i){v.phase[i]=data[7+i];v.motion[i]=data[11+i];v.energy[i]=data[15+i];v.pan[i]=data[19+i];}return v;
    }return {};
  }
};
}
