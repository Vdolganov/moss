#pragma once
#include "stream_reader.h"
#include "sample_slice.h"
namespace moss {
// Exact signed min/max of both channels, incrementally read on the UI thread.
// No PCM-sized allocation, file handle ownership or work in the audio callback.
struct SampleWaveform {
  static constexpr unsigned Columns=604,BytesPerPump=32768;
  int16_t low[2][Columns]={},high[2][Columns]={};
  bool seen[Columns]={},failed=false;
  WavSource source;
  unsigned first=0,last=0,read=0;
  bool matches(const WavSource &s,unsigned a,unsigned b) const {
    return source.frames==s.frames && source.rate==s.rate &&
      !strcmp(source.path,s.path) && first==a && last==b;
  }
  void begin(const WavSource &s,unsigned a,unsigned b) {
    source=s;first=a;last=b;read=0;failed=false;
    memset(seen,0,sizeof(seen));
    for(int ch=0;ch<2;++ch)for(unsigned x=0;x<Columns;++x){low[ch][x]=32767;high[ch][x]=-32768;}
  }
  bool busy() const {return !failed && last>first && read<last-first;}
  void pump() {
    if(!busy())return;
    FILE *f=fopen(source.path,"rb");
    if(!f){failed=true;return;}
    unsigned char data[BytesPerPump];
    const unsigned stride=source.channels*(source.bits/8),total=last-first;
    unsigned count=sampleMin(total-read,BytesPerPump/stride);
    bool ok=!fseek(f,long(source.offset+uint64_t(first+read)*stride),SEEK_SET) && fread(data,stride,count,f)==count;
    fclose(f);if(!ok){failed=true;return;}
    for(unsigned i=0;i<count;++i) {
      unsigned x=unsigned(uint64_t(read+i)*Columns/total);
      // Repeat a real sample across pixels when zoomed beyond one frame/pixel.
      unsigned next=unsigned(uint64_t(read+i+1)*Columns/total);
      if(next<=x)next=x+1;
      for(unsigned pixel=x;pixel<next && pixel<Columns;++pixel) {
        seen[pixel]=true;
        for(unsigned ch=0;ch<source.channels;++ch) {
          int value=StreamReader::decode(data+i*stride+ch*(source.bits/8),source.bits,source.format);
          if(value<low[ch][pixel])low[ch][pixel]=int16_t(value);
          if(value>high[ch][pixel])high[ch][pixel]=int16_t(value);
        }
      }
    }
    read+=count;
  }
};
}
