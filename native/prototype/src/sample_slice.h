#pragma once
#include <stdint.h>
namespace moss {
template<class T> inline T sampleMin(T a,T b){return a<b?a:b;}
template<class T> inline T sampleMax(T a,T b){return a>b?a:b;}
// Millisecond fields retain the manual editor contract. Remainders preserve
// sample-exact bounds through coarse editing, preview and WAV export.
struct SampleSlice {
  int start=0,end=1,fadeIn=0,fadeOut=0;
  unsigned startRemainder=0,endRemainder=0;
  unsigned first(unsigned rate) const {
    return unsigned(uint64_t(start)*rate/1000)+startRemainder;
  }
  unsigned last(unsigned rate,unsigned frames) const {
    if(end==int((uint64_t(frames)*1000+rate-1)/rate) && !endRemainder)return frames;
    return unsigned(uint64_t(end)*rate/1000)+endRemainder;
  }
  void setFirst(unsigned frame,unsigned rate) {
    start=int(uint64_t(frame)*1000/rate);
    startRemainder=frame-unsigned(uint64_t(start)*rate/1000);
  }
  void setLast(unsigned frame,unsigned rate,unsigned frames) {
    end=int((uint64_t(frame)*1000+(frame==frames?rate-1:0))/rate);
    endRemainder=frame==frames?0:frame-unsigned(uint64_t(end)*rate/1000);
  }
};
}
