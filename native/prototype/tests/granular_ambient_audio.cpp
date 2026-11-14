#include "../src/engine.h"
#include "../src/library.h"
#include <stdlib.h>
// Original synthetic source, so the listening fixture contains no sample-pack audio.
using namespace moss;
static SampleBuffer source;
static Engine engine;
static Project project;
static int16_t output[AudioRate*22*2];
int main(int argc,char **argv) {
  if(argc<2 || argc>3) return 2;
  int asset=argc==3?atoi(argv[2]):0;if(asset<0||asset>=Assets)return 2;
  source.rate=AudioRate;source.channels=1;source.frames=AudioRate*6.4;source.bytes=source.frames*2;
  uint32_t noise=831;
  for(unsigned i=0;i<source.frames;++i) {
    const double t=double(i)/AudioRate,x=double(i)/source.frames;
    const double envelope=sin(3.14159265359*x)*sin(3.14159265359*x);
    noise=noise*1664525u+1013904223u;
    const double harmonics=sin(t*6.28318530718*130.8128)+.5*sin(t*6.28318530718*196)+.3*sin(t*6.28318530718*311.127);
    const double air=(double(noise>>16)/32768-1)*(.12+.14*sin(x*19)*sin(x*19));
    source.pcm[i]=int16_t((harmonics+air)*(4160+6760*sin(x*11)*sin(x*11))*envelope);
  }
  char path[1024];snprintf(path,sizeof(path),"%s/ambient-source.wav",argv[1]);
  if(!writeWav(path,source.pcm,source.frames,source.rate,1))return 3;
  project.trackCount=1;project.bpm=70;project.master=100;strcpy(project.name,"LENS DRIFT");
  auto &tr=project.tracks[0];tr.instrument=GranularInstrument;strcpy(tr.name,"LENS");tr.level=100;
  tr.granular.asset=0;tr.granular.values[2]=3200;tr.granular.values[7]=40;tr.granular.values[9]=75;
  tr.granular.values[11]=160;
  tr.granular.values[14]=900;tr.granular.values[15]=2500;
  tr.cycle=Steps;tr.steps[0].on=1;tr.steps[0].note=GranularRootNote;tr.steps[0].length=Steps;tr.steps[0].velocity=120;
  tr.fx.enable(true);tr.fx.enabledTypes[Orbit]=tr.fx.enabledTypes[Pond]=1;
  tr.fx.values[Orbit][1]=48;tr.fx.values[Orbit][3]=18;tr.fx.values[Pond][3]=28;
  for(int variant=0;variant<2;++variant) {
    tr.granular.values[9]=variant?75:0;engine.init(project,&source,AudioRate,1);engine.handle({Op::Transport});
    for(unsigned at=0;at<AudioRate*22;) {
      if(at==AudioRate*18)engine.handle({Op::Stop});
      unsigned n=AudioRate*22-at;if(n>AudioBlockFrames)n=AudioBlockFrames;
      if(at<AudioRate*18 && at+n>AudioRate*18)n=AudioRate*18-at;
      engine.render(output+at*2,n);
      at+=n;
    }
    snprintf(path,sizeof(path),"%s/ambient-%s.wav",argv[1],variant?"jump":"scan");
    if(!writeWav(path,output,AudioRate*22,AudioRate,2))return 4;
  }
  tr.granular.asset=asset;tr.granular.values[9]=75;
  snprintf(path,sizeof(path),"%s/lens-drift.moss",argv[1]);
  if(exists(path))return 5;
  return saveProject(path,project)?0:6;
}
