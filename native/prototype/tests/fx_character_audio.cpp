#include "../src/engine.h"
#include "../src/library.h"
#include "../src/storage.h"
using namespace moss;
static Engine engine;
static SampleBuffer source;
static int16_t audio[AudioRate*12*2];
static float probes[2][16384];
static bool render(const char *directory,const char *name,Project &p) {
  engine.init(p,&source,AudioRate,1);engine.handle({Op::Transport});
  for(unsigned n=0;n<AudioRate*12;n+=1024)
    engine.render(audio+n*2,minimum(1024u,AudioRate*12-n));
  char path[1024];snprintf(path,sizeof(path),"%s/%s.wav",directory,name);
  return writeWav(path,audio,AudioRate*12,AudioRate,2);
}
int main(int argc,char **argv) {
  if(argc!=2) return 2;
  Project p;p.trackCount=1;p.bpm=100;p.master=85;
  auto &t=p.tracks[0];t.instrument=MonoInstrument;t.cycle=128;t.level=80;
  strcpy(t.name,"ACID PULSE");
  const int synth[]={700,65,4,900,65,50,8,20};memcpy(t.synthParams[0],synth,sizeof(synth));
  const int notes[]={31,43,38,41,31,46,38,43};
  for(int i=0;i<8;++i) {
    auto &s=t.steps[i*16];s.on=1;s.note=notes[i];s.length=8;s.velocity=95;
  }
  if(!render(argv[1],"01-dry",p))return 3;
  t.fx.select(Drive);t.fx.enable(true);
  const int drive[]={50,50,85,80,0,10,60,20};memcpy(t.fx.values[Drive],drive,sizeof(drive));
  const char *names[]={"02-drive-round","03-drive-bite","04-drive-edge"};
  for(int c=0;c<3;++c) {t.fx.values[Drive][4]=c;if(!render(argv[1],names[c],p))return 3;}
  t.fx.enable(false);t.fx.select(Orbit);t.fx.enable(true);
  const int orbit[]={2,65,85,40,80,6000,50,40};memcpy(t.fx.values[Orbit],orbit,sizeof(orbit));
  if(!render(argv[1],"05-orbit-dub",p))return 3;
  t.fx.select(Drive);t.fx.enable(true);t.fx.values[Drive][4]=1;
  t.fx.move(Drive,-1);t.fx.move(Drive,-1);t.fx.move(Drive,-1);t.fx.move(Drive,-1);
  if(!render(argv[1],"06-drive-orbit",p))return 3;
  strcpy(p.name,"DRIVE ORBIT");char path[1024];snprintf(path,sizeof(path),"%s/drive-orbit.moss",argv[1]);
  if(exists(path)||!saveProject(path,p))return 4;

  // Quiet diagnostic fixture; 40/80/160/1000 Hz, two seconds each including fades.
  // It can compare identical files/earphones on PC and device, not ear sensitivity.
  const int tones[]={40,80,160,1000};
  for(unsigned i=0;i<AudioRate*8;++i) {
    unsigned at=i%(AudioRate*2);float envelope=minimum(1.f,minimum(at,AudioRate*2-1-at)/1600.f);
    int16_t value=int16_t(3277*envelope*sin(6.283185307179586*tones[i/(AudioRate*2)]*i/AudioRate));
    audio[2*i]=audio[2*i+1]=value;
  }
  snprintf(path,sizeof(path),"%s/BASS-CHECK.wav",argv[1]);
  if(!writeWav(path,audio,AudioRate*8,AudioRate,2))return 3;

  // Periodic 1978.515625 Hz sine: paired raw float spectra for a bounded ADAA comparison.
  for(int curve=0;curve<3;++curve) {
    float old=0;
    for(unsigned i=0;i<32768;++i) {
      float x=3.f*sinf(float(6.283185307179586*1013*(i%16384)/16384));
      if(i>=16384) {probes[0][i-16384]=DriveDSP::shape(x,curve);probes[1][i-16384]=DriveDSP::antialiased(x,old,curve);}
      old=x;
    }
    snprintf(path,sizeof(path),"%s/alias-curve-%d.f32",argv[1],curve);
    FILE *f=fopen(path,"wb");if(!f)return 3;
    bool ok=fwrite(probes,1,sizeof(probes),f)==sizeof(probes);fclose(f);if(!ok)return 3;
  }
  puts("Rendered six listening examples, a sample-free demo, bass reference and alias probes");
  return 0;
}
