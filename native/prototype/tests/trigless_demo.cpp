#include "../src/engine.h"
#include "../src/library.h"
#include <stdlib.h>
using namespace moss;
static Project project;
static SampleBuffer sample;
static Engine engine;
static int16_t audio[AudioRate*16*2];
int main(int argc,char **argv) {
  if(argc!=4) return 2;
  int asset=atoi(argv[3]);if(asset<0 || asset>=Assets || !sample.load(argv[1])) return 3;
  project.trackCount=1;project.bpm=70;project.master=100;strcpy(project.name,"LOCK STUDY");
  auto &t=project.tracks[0];strcpy(t.name,"HELD CLOUD");t.instrument=GranularInstrument;
  t.level=100;t.division=8;t.cycle=128;t.granular.asset=0;
  t.granular.values[2]=3200;t.granular.values[7]=40;t.granular.values[9]=75;t.granular.values[11]=160;
  t.granular.values[14]=700;t.granular.values[15]=1800;
  t.steps[0].on=NoteEvent;t.steps[0].note=GranularRootNote;t.steps[0].length=128;t.steps[0].velocity=120;
  auto event=[&](int at)->Step& {t.steps[at].on=LockEvent;return t.steps[at];};
  event(56).grainLocks[10]=1200; // Step 8 changes the held note.
  event(64).repeat=16;
  event(80).repeat=64;
  event(96).repeat=0;
  event(104).grainLocks[10]=ResetLock;
  event(112).fxLocks[Orbit][3]=45;
  t.fx.enable(true);t.fx.enabledTypes[Orbit]=t.fx.enabledTypes[Pond]=1;
  t.fx.values[Orbit][1]=40;t.fx.values[Orbit][3]=16;t.fx.values[Pond][3]=24;
  engine.init(project,&sample,AudioRate,1);engine.handle({Op::Transport});
  for(unsigned i=0;i<AudioRate*16;) {unsigned n=AudioRate*16-i;if(n>1024)n=1024;engine.render(audio+i*2,n);i+=n;}
  char path[1024];snprintf(path,sizeof(path),"%s/lock-study.wav",argv[2]);
  if(!writeWav(path,audio,AudioRate*16,AudioRate,2))return 4;
  t.granular.asset=asset;snprintf(path,sizeof(path),"%s/lock-study.moss",argv[2]);
  if(exists(path) || !saveProject(path,project))return 5;
  printf("LOCK STUDY: one 16-step note, cutoff at step 8, repeat at steps 9/11, OFF at 13, RESET at 14, FX at 15; asset %d\n",asset);
}
