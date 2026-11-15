#pragma once
#include "config.h"
#include <stdint.h>
namespace moss {
// One bounded stereo fragment per track, before FX. Captures the first period
// after the event; the source voice always continues running underneath.
struct AudioRepeat {
  static constexpr unsigned Capacity = 96000, Ramp = 128;
  int32_t pcm[Capacity][2] = {};
  unsigned length = 0, at = 0, written = 0, ramp = 0;
  unsigned lastRate = 0;
  int division = 0, lastBpm = 0, last[2] = {}, from[2] = {};
  void reset() { division=0; length=at=written=ramp=0; last[0]=last[1]=0; }
  void edge() { from[0]=last[0]; from[1]=last[1]; ramp=Ramp; }
  void set(int value, unsigned rate, int bpm) {
    if (!repeatValid(value) || value==Inherit) return;
    if(!value && !division) return;
    edge(); division=value; lastRate=rate; lastBpm=bpm;
    at=written=0;
    length=value ? unsigned(uint64_t(rate)*240/(unsigned(bpm)*unsigned(value))) : 0;
    if(length>Capacity) length=Capacity;
    if(value && !length) length=1;
  }
  void prepare(unsigned rate, int bpm) {
    if(division && (rate!=lastRate || bpm!=lastBpm)) set(division,rate,bpm);
  }
  void process(int &left, int &right) {
    if(!division && !ramp) { last[0]=left; last[1]=right; return; }
    int value[2]={left,right};
    if(division) {
      if(written<length) {
        pcm[written][0]=left; pcm[written][1]=right;
        if(++written==length) at=0;
      } else {
        if(!at) edge();
        value[0]=pcm[at][0]; value[1]=pcm[at][1];
        if(++at==length) at=0;
      }
    }
    if(ramp) {
      // 64-bit intermediates also cover a hot sum of four sampler voices.
      for(int ch=0;ch<2;++ch)
        value[ch]=int((int64_t(value[ch])*(Ramp-ramp)+int64_t(from[ch])*ramp)/Ramp);
      --ramp;
    }
    left=last[0]=value[0]; right=last[1]=value[1];
  }
};
} // namespace moss
