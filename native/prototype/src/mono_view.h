#pragma once
#include "mono_dsp.h"
namespace moss {
struct MonoView {
  int active=0,gate=0,cutoff=0,amp=0,filter=0,lfo=0,hz=0,milliseconds=32;
  int wave[MonoDSP::ScopePoints]={};
};
struct MonoMailbox {
  static constexpr unsigned Header=8,Words=Header+MonoDSP::ScopePoints;
  uint32_t sequence=0;int words[Words]={};
  void publish(const MonoDSP &v,bool selected){
    unsigned serial=__atomic_load_n(&sequence,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+1,__ATOMIC_SEQ_CST);
    int head[]={selected&&v.active,selected&&v.gate,int(v.cutoff),int(v.env*1000),int(v.filterEnv*1000),int(v.lfo*1000),int(v.frequency*100),int(1024000u/v.rate)};
    for(unsigned i=0;i<Header;++i)__atomic_store_n(&words[i],head[i],__ATOMIC_RELAXED);
    for(unsigned i=0;i<MonoDSP::ScopePoints;++i)__atomic_store_n(&words[Header+i],selected&&v.active?int(v.scope[(v.scopeAt+i)%MonoDSP::ScopePoints]*32767):0,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+2,__ATOMIC_RELEASE);
  }
  MonoView read() const {
    for(int attempt=0;attempt<3;++attempt){unsigned before=__atomic_load_n(&sequence,__ATOMIC_ACQUIRE);if(before&1)continue;
      int data[Words];for(unsigned i=0;i<Words;++i)data[i]=__atomic_load_n(&words[i],__ATOMIC_RELAXED);
      __atomic_thread_fence(__ATOMIC_ACQUIRE);if(before!=__atomic_load_n(&sequence,__ATOMIC_RELAXED))continue;
      MonoView v;v.active=data[0];v.gate=data[1];v.cutoff=data[2];v.amp=data[3];v.filter=data[4];v.lfo=data[5];v.hz=data[6];v.milliseconds=data[7];
      for(unsigned i=0;i<MonoDSP::ScopePoints;++i)v.wave[i]=data[Header+i];return v;
    }return {};
  }
};
}
