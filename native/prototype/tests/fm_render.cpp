#include "../src/engine.h"
#include "../src/storage.h"
#include <stdio.h>
#include <initializer_list>
using namespace moss;
static SynthTables tables;static SynthVoice voice;static Engine engine;static Project project;
static int16_t buffer[2048];
static void word(FILE *f,unsigned n,int bytes){while(bytes--){fputc(n&255,f);n>>=8;}}
static FILE *wave(const char *folder,const char *name,unsigned frames){char path[1024];snprintf(path,sizeof(path),"%s/%s.wav",folder,name);FILE *f=fopen(path,"wb");if(!f)return nullptr;
  fwrite("RIFF",1,4,f);word(f,36+frames*4,4);fwrite("WAVEfmt ",1,8,f);word(f,16,4);word(f,1,2);word(f,2,2);word(f,32000,4);word(f,128000,4);word(f,4,2);word(f,16,2);fwrite("data",1,4,f);word(f,frames*4,4);return f;}
int main(int argc,char **argv){if(argc!=2)return 1;tables.init();
  for(int preset=0;preset<FmPresetCount;++preset){
    const int notes[]={57,57,45,45,33,50,33,45};int note=notes[preset];
    voice=SynthVoice{};voice.on(FmInstrument,note,tables.frequency[note],110,0,FmPresets[preset],FmPresetModes[preset],32000,17,FmParameters);
    FILE *f=wave(argv[1],FmPresetNames[preset],32000*8);if(!f)return 2;
    for(int n=0;n<32000*8;++n){if(n==32000*5)voice.off();int l=0,r=0;voice.render(l,r,tables);float fade=n>32000*8-3200?float(32000*8-n)/3200:1;int16_t out[]={int16_t(l*fade),int16_t(r*fade)};fwrite(out,2,2,f);}fclose(f);
  }
  project=Project{};project.trackCount=4;project.bpm=106;project.master=90;strcpy(project.name,"ELASTIC MATTER");
  const char *names[]={"THUMP","SNAP","RUBBER","CLOUD"};const int levels[]={90,72,68,52};
  for(int t=0;t<4;++t){auto &tr=project.tracks[t];tr.instrument=FmInstrument;tr.synthPreset[2]=t+4;tr.synthMode[2]=FmPresetModes[t+4];
    memcpy(tr.synthParams[2],FmPresets[t+4],sizeof(tr.synthParams[2]));strcpy(tr.name,names[t]);tr.level=levels[t];tr.cycle=t<2?16:t==2?32:64;tr.swing=56;tr.division=16;tr.audition=t==1?50:t==3?45:33;
    if(t==0)for(int i=0;i<16;i+=4){auto &s=tr.steps[i];s.on=NoteEvent;s.note=33;s.length=3;s.velocity=i==8?100:120;s.synthLocks[2][FmPunch]=i==12?29:38;}
    if(t==1)for(int i: {4,12,14,15}){auto &s=tr.steps[i];s.on=NoteEvent;s.note=i<14?50:62;s.length=1;s.velocity=i<14?112:65;s.synthLocks[2][FmDecay]=i<14?190:65;s.synthLocks[2][FmNoise]=i<14?65:90;}
    if(t==2){const int positions[]={0,3,6,10,14,18,22,27},notes[]={33,33,40,43,36,33,45,40};for(int j=0;j<8;++j){auto &s=tr.steps[positions[j]];s.on=NoteEvent;s.note=notes[j];s.length=j%3+1;s.velocity=90+j%3*12;s.synthLocks[2][FmPunch]=j%2?7:19;s.synthLocks[2][FmRatio]=j%4==3?2:1;s.synthLocks[2][FmFeedback]=18+j*7;}}
    if(t==3){auto &s=tr.steps[0];s.on=NoteEvent;s.note=45;s.length=64;s.velocity=100;for(int i=8;i<64;i+=8){auto &l=tr.steps[i];l.on=LockEvent;l.synthLocks[2][FmIndex]=20+i/2;l.synthLocks[2][FmModHold]=55+i/2;l.synthLocks[2][FmTone]=60+i/3;}}
    if(t>=2){tr.fx.select(Orbit);tr.fx.enable(true);tr.fx.values[Orbit][3]=t==2?16:20;tr.fx.select(Pond);tr.fx.enable(true);tr.fx.values[Pond][3]=t==2?12:30;}
  }
  char path[1024];snprintf(path,sizeof(path),"%s/ELASTIC-MATTER.moss",argv[1]);if(!saveProject(path,project))return 3;
  engine.init(project,nullptr,32000,0);engine.handle({Op::Transport});FILE *f=wave(argv[1],"ELASTIC-MATTER",32000*36);if(!f)return 2;
  for(unsigned at=0;at<32000*36;){unsigned frames=32000*36-at;if(frames>1024)frames=1024;engine.render(buffer,frames);
    for(unsigned i=0;i<frames;++i)if(at+i>32000*34){float fade=float(32000*36-at-i)/(32000*2);buffer[i*2]=int16_t(buffer[i*2]*fade);buffer[i*2+1]=int16_t(buffer[i*2+1]*fade);}fwrite(buffer,4,frames,f);at+=frames;}fclose(f);
  puts("Rendered eight dry FM voices and ELASTIC MATTER: four FM tracks, note and lock-only events.");
}
