#include "../src/granular.h"
#include "../src/library.h"
#include "../src/engine.h"
// Diagnostic listening fixture, not a preset or a claim about musical quality.
// Eight distinguishable regions become a simultaneous cloud at SCATTER 100%.
using namespace moss;
static SampleBuffer source;
static GranularTables tables;
static GranularVoice voice;
static int16_t output[AudioRate * 8 * 2];
int main(int argc, char **argv) {
  if(argc!=2) return 2;
  source.rate=AudioRate; source.channels=1; source.frames=AudioRate*16;
  source.bytes=source.frames*2;
  const float tones[]={65.406f,98.f,130.813f,196.f,261.626f,311.127f,392.f,523.251f};
  for(unsigned i=0;i<source.frames;++i) {
    float phase=6.28318530718f*tones[i/(AudioRate*2)]*float(i)/AudioRate;
    source.pcm[i]=int16_t(9500*sinf(phase)+2200*sinf(phase*2));
  }
  tables.init();
  for(unsigned example=0;example<2;++example) {
    if(!voice.init(source,tables,AudioRate)) return 3;
    GranularParameters p; p.sizeMs=400;p.density=28;p.scatter=float(example);
    p.speed=1;p.spread=.6f;p.attackMs=300;p.releaseMs=500;p.grainIn=.1f;p.grainOut=.6f;
    voice.setParameters(p);voice.noteOn();
    for(unsigned i=0;i<AudioRate*8;++i) {
      if(i==AudioRate*7) voice.noteOff();
      float l,r;voice.render(l,r);
      output[i*2]=int16_t(Engine::monitor(int(l*16384)));
      output[i*2+1]=int16_t(Engine::monitor(int(r*16384)));
    }
    char path[1024];snprintf(path,sizeof(path),"%s/%s.wav",argv[1],example?"full-cloud":"sequential-scan");
    if(!writeWav(path,output,AudioRate*8,AudioRate,2)) return 4;
  }
}
