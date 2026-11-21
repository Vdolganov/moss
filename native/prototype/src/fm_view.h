#pragma once
#include "fm_dsp.h"
namespace moss {
struct FmView {
  int active=0,gate=0,amp=0,colour=0,pitch=0,spread=0,ratio=0;
  int points[2][FmDSP::PortraitPoints][2]={};
};
struct FmMailbox {
  static constexpr unsigned Header=7,Words=Header+4*FmDSP::PortraitPoints;
  uint32_t sequence=0;int words[Words]={};
  void publish(const FmDSP &v,bool selected){
    unsigned serial=__atomic_load_n(&sequence,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+1,__ATOMIC_SEQ_CST);
    int head[]={selected&&v.active,selected&&v.gate,int(v.env*v.gain*1000),int((v.modHold+(1-v.modHold)*v.modEnv)*1000),int(v.bendSemitones*100),int(v.spread*1000),int(FmDSP::clamp(v.target[FmRatio],0,15))};
    for(unsigned i=0;i<Header;++i)__atomic_store_n(&words[i],head[i],__ATOMIC_RELAXED);
    unsigned at=Header;
    for(unsigned pair=0;pair<2;++pair)for(unsigned i=0;i<FmDSP::PortraitPoints;++i)for(unsigned axis=0;axis<2;++axis)
      __atomic_store_n(&words[at++],selected&&v.active?int(v.portrait[pair][(v.portraitAt+i)%FmDSP::PortraitPoints][axis]*1000):0,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+2,__ATOMIC_RELEASE);
  }
  FmView read() const {
    for(int attempt=0;attempt<3;++attempt){unsigned before=__atomic_load_n(&sequence,__ATOMIC_ACQUIRE);if(before&1)continue;
      int data[Words];for(unsigned i=0;i<Words;++i)data[i]=__atomic_load_n(&words[i],__ATOMIC_RELAXED);
      __atomic_thread_fence(__ATOMIC_ACQUIRE);if(before!=__atomic_load_n(&sequence,__ATOMIC_RELAXED))continue;
      FmView v;v.active=data[0];v.gate=data[1];v.amp=data[2];v.colour=data[3];v.pitch=data[4];v.spread=data[5];v.ratio=data[6];
      unsigned at=Header;for(auto &pair:v.points)for(auto &point:pair)for(int &axis:point)axis=data[at++];return v;
    }return {};
  }
};
}
