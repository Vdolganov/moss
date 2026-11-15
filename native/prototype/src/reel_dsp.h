#pragma once
#include "reel_motion.h"
#include "reel_color.h"
#include "reel_life.h"
namespace moss {
struct ReelDSP {
  bool lastEnabled=false;
  unsigned transportFrames=0;
  float blockPeak=0;
  float driftStep=0,stereoStep=0;
  ReelLife life;
  ReelMotion motion;
  ReelColor color;
  bool separated=false;
  void resetReel(unsigned rate,uint32_t seed=357) {
    motion.reset(rate);color.reset(rate);life.reset(rate,seed);separated=false;driftStep=stereoStep=0;
    // At unusually low rates also reserve the FIR latency plus the maximum
    // +/-15 cent transport excursion. Production 32 kHz stays exactly 16 ms.
    unsigned safe=unsigned(ceilf((exp2f(15.f/1200)-1)*rate*(10.5f/6.28318530718f)))+7+ReelColor::Latency;
    if(motion.latency<safe)motion.latency=safe;
    motion.position=float(motion.latency);
  }
  void prepareReel(const int *p,unsigned rate,bool enabled) {
    if(motion.rate!=rate) resetReel(rate);
    float amount=enabled?p[ReelAmount]*.01f:0;
    motion.prepare({p[ReelWow]*.1f,p[ReelWowRate]*.01f,p[ReelFlutter]*.1f,p[ReelFlutterRate]*.1f,amount},rate);
    color.prepare(float(p[ReelDrive]),p[ReelTone]*.1f,p[ReelBump]*.1f,amount,rate);
    life.prepare(p,amount);
  }
  __attribute__((noinline)) void splitReel(bool separate) {
    const unsigned count=ReelColor::Latency;
    if(separate) {
      // Prime the small dry FIFO from the unified history, then rewind its
      // write head. Read addresses stay identical: both head and delay move.
      for(unsigned distance=1;distance<=count;++distance)for(unsigned ch=0;ch<2;++ch)
        color.dry[(color.cursor-distance)&ReelColor::Mask][ch]=distance<=motion.written?
          motion.history[(motion.cursor-distance)&ReelMotion::Mask][ch]:0;
      motion.cursor=(motion.cursor-count)&ReelMotion::Mask;
      motion.written=motion.written>count?motion.written-count:0;
      motion.latency-=count;motion.position-=count;
    } else {
      // Once the saturation ramp reaches zero, catch up its FIFO in one bounded
      // operation. No per-frame second delay is needed in neutral/motion-only.
      for(unsigned distance=count;distance;--distance) {
        for(unsigned ch=0;ch<2;++ch)motion.history[motion.cursor][ch]=
          color.dry[(color.cursor-distance)&ReelColor::Mask][ch];
        motion.cursor=(motion.cursor+1)&ReelMotion::Mask;
        if(motion.written<ReelMotion::Size)++motion.written;
      }
      motion.latency+=count;motion.position+=count;
    }
    separated=separate;
  }
  __attribute__((noinline)) void processReel(float &l,float &r,bool enabled,const int *p,unsigned rate) {
    if(enabled!=lastEnabled) {prepareReel(p,rate,enabled);lastEnabled=enabled;}
    if(life.active) {
      if(!life.divider) {
        life.control();
        driftStep=(life.drift-motion.driftOffset)*(1.f/16);
        float stereo=life.stereoDepth*ReelMotion::sine(motion.mod[0].phase+0x40000000u);
        stereoStep=(stereo-motion.stereoOffset)*(1.f/16);
        if(!life.active){motion.driftOffset=motion.stereoOffset=0;driftStep=stereoStep=0;}
      }
      motion.driftOffset+=driftStep;motion.stereoOffset+=stereoStep;
      if(motion.driftOffset!=0||motion.stereoOffset!=0||motion.rightOffset!=0)motion.moving=true;
      life.divider=(life.divider+1)&15;
    }
    color.advance();
    if(separated!=color.saturating)splitReel(color.saturating);
    if(separated)color.processInput(l,r,false);
    motion.process(l,r);color.processOutput(l,r);
    if(life.active)life.process(l,r);
    float leftPeak=fabsf(l),rightPeak=fabsf(r),peak=leftPeak>rightPeak?leftPeak:rightPeak;
    if(peak>blockPeak)blockPeak=peak;
    if(life.running&&enabled)++transportFrames;
  }
};
// Audio writes once per block; the UI never touches live filter/history state.
struct ReelSnapshot {
  static constexpr int History=32;
  int phase=0,low=0,high=0,peak=0,drop=1000,running=0;
  int envelope[History]={},speedLow[History]={},speedHigh[History]={},noise[History]={};
};
struct ReelMailbox {
  static constexpr int History=ReelSnapshot::History,Words=6+4*History;
  uint32_t sequence=0;int words[Words]={0,0,0,0,1000,0};
  unsigned cursor=0;int history[History][4]={};float slowOffset=0;
  void publish(ReelDSP &d) {
    unsigned serial=__atomic_load_n(&sequence,__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+1,__ATOMIC_SEQ_CST);
    // The low-pass read-head displacement gently bends the reel phase. Flutter
    // is shown by block extrema, never by aliasing the reel rotation.
    if(d.life.running&&d.lastEnabled)slowOffset+=(d.motion.position-float(d.motion.latency)-slowOffset)*.12f;
    unsigned period=d.motion.rate?d.motion.rate*4:128000;
    int phase=int(d.transportFrames%period)-int(slowOffset);if(phase<0)phase+=period;
    int values[]={phase%int(period),
      int(d.motion.minimumSpeed*173123.f),int(d.motion.maximumSpeed*173123.f),
      int(fminf(8,d.blockPeak)*1000),int(d.life.drop*1000),d.life.running&&d.lastEnabled};
    for(int i=0;i<6;++i)__atomic_store_n(&words[i],values[i],__ATOMIC_RELAXED);
    history[cursor][0]=values[3];history[cursor][1]=values[1];history[cursor][2]=values[2];
    history[cursor][3]=int(d.life.noisePeak*1000);cursor=(cursor+1)%History;
    for(int i=0;i<History;++i)for(int k=0;k<4;++k)
      __atomic_store_n(&words[6+k*History+i],history[(cursor+i)%History][k],__ATOMIC_RELAXED);
    __atomic_store_n(&sequence,serial+2,__ATOMIC_RELEASE);
    d.blockPeak=d.life.noisePeak=0;d.motion.minimumSpeed=d.motion.maximumSpeed=0;
  }
  ReelSnapshot read() const {
    for(int attempt=0;attempt<3;++attempt) {
      unsigned before=__atomic_load_n(&sequence,__ATOMIC_ACQUIRE);if(before&1)continue;
      int v[Words];for(int i=0;i<Words;++i)v[i]=__atomic_load_n(&words[i],__ATOMIC_RELAXED);
      __atomic_thread_fence(__ATOMIC_ACQUIRE);
      if(__atomic_load_n(&sequence,__ATOMIC_RELAXED)==before) {
        ReelSnapshot out;out.phase=v[0];out.low=v[1];out.high=v[2];out.peak=v[3];out.drop=v[4];out.running=v[5];
        for(int i=0;i<History;++i){out.envelope[i]=v[6+i];out.speedLow[i]=v[6+History+i];out.speedHigh[i]=v[6+History*2+i];out.noise[i]=v[6+History*3+i];}
        return out;
      }
    }
    return {};
  }
};
} // namespace moss
