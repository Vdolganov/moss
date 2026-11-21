#include "../src/engine.h"
#include "../src/storage.h"
#include <stdio.h>
using namespace moss;
static SynthTables tables;static SynthVoice voice;static Engine engine;
static int16_t buffer[2048];
static void word(FILE *f,unsigned n,int bytes){while(bytes--){fputc(n&255,f);n>>=8;}}
static FILE *wave(const char *folder,const char *name,unsigned frames){
  char path[1024];snprintf(path,sizeof(path),"%s/%s.wav",folder,name);FILE *f=fopen(path,"wb");if(!f)return nullptr;
  fwrite("RIFF",1,4,f);word(f,36+frames*4,4);fwrite("WAVEfmt ",1,8,f);word(f,16,4);word(f,1,2);word(f,2,2);word(f,32000,4);word(f,128000,4);word(f,4,2);word(f,16,2);fwrite("data",1,4,f);word(f,frames*4,4);return f;
}
int main(int argc,char **argv){
  if(argc!=2)return 1;tables.init();const char *names[]={"01-root","02-breath","03-tangle","04-bare"};
  for(int preset=0;preset<4;++preset){
    voice=SynthVoice{};voice.on(OrganismInstrument,33,55,110,0,OrganismPresets[preset],preset==2,32000,17,OrganismParameters);
    FILE *f=wave(argv[1],names[preset],32000*16);if(!f)return 2;
    for(int n=0;n<32000*16;++n){if(n==32000*10)voice.off();int l=0,r=0;voice.render(l,r,tables);
      // Last 100 ms fades the file boundary, not the engine's release.
      float fade=n>32000*16-3200?float(32000*16-n)/3200:1;
      int16_t out[]={int16_t(l*fade),int16_t(r*fade)};fwrite(out,2,2,f);
    }fclose(f);
  }
  Project p;p.trackCount=1;p.master=85;strcpy(p.name,"LIVING MATTER");auto &t=p.tracks[0];t.instrument=OrganismInstrument;t.level=80;t.cycle=512;
  strcpy(t.name,"LIVING ROOT");t.audition=33;t.synthPreset[1]=-1;
  memcpy(t.synthParams[1],OrganismPresets[1],sizeof(t.synthParams[1]));t.synthParams[1][OrgSub]=55;t.synthParams[1][OrgAttack]=2500;
  t.steps[0].on=NoteEvent;t.steps[0].note=33;t.steps[0].length=512;
  const int events[]={64,128,192,256,320};
  for(int i=0;i<5;++i){auto &s=t.steps[events[i]];s.on=LockEvent;s.synthLocks[1][OrgShape]=12+i*17;s.synthLocks[1][OrgCouple]=10+i*11;s.synthLocks[1][OrgGrit]=i*14;}
  t.fx.select(Reel);t.fx.enable(true);t.fx.values[Reel][3]=40;
  t.fx.select(Pond);t.fx.enable(true);t.fx.values[Pond][0]=85;t.fx.values[Pond][1]=75;t.fx.values[Pond][3]=30;
  char projectPath[1024];snprintf(projectPath,sizeof(projectPath),"%s/LIVING-MATTER.moss",argv[1]);if(!saveProject(projectPath,p))return 3;
  engine.init(p,nullptr,32000,0);engine.handle({Op::Transport});FILE *f=wave(argv[1],"05-living-matter",32000*40);if(!f)return 2;
  for(unsigned at=0;at<32000*40;){unsigned frames=32000*40-at;if(frames>1024)frames=1024;engine.render(buffer,frames);
    for(unsigned i=0;i<frames;++i)if(at+i>32000*36){float fade=float(32000*40-at-i)/(32000*4);buffer[i*2]=int16_t(buffer[i*2]*fade);buffer[i*2+1]=int16_t(buffer[i*2+1]*fade);}
    fwrite(buffer,4,frames,f);at+=frames;
  }fclose(f);puts("Rendered four dry A1 voices and a 40-second held-note study with live locks, REEL and POND.");
}
