#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static SynthTables tables;static MonoDSP voice;static Engine engine;
static App app;static Project restored;static Codec codec;static int16_t audio[2048];
static void render(unsigned n){for(unsigned i=0;i<n;++i){float x=voice.render(tables.mono);assert(isfinite(x)&&fabsf(x)<=.981f);}}
static void limits(){
  tables.init();const unsigned rates[]={8000,32000,44100,96000};
  for(unsigned rate:rates)for(int mode=0;mode<2;++mode)for(int edge=0;edge<3;++edge){
    int p[SynthParameterCount];for(int i=0;i<SynthParameterCount;++i)p[i]=edge==0?synthMin(1,i):edge==1?synthMax(1,i):presetValues(1,1)[i];
    p[2]=0;voice=MonoDSP{};voice.on(edge?24:96,tables.frequency[edge?24:96],127,p,mode,rate,37);
    render(rate/2);for(int i=0;i<20;++i)p[i]=synthMax(1,i);voice.setTarget(p,!mode);render(rate/2);
    voice.stop(128);render(129);assert(!voice.active);
  }
  for(int i=-24000;i<=24000;i+=17){float x=i*.001f;float error=fabsf(MonoDSP::exp2Fast(x)/exp2f(x)-1);assert(error<.000002f);}
  puts("PASS MONO: both waves, all-control extrema, rapid locks, 8/32/44.1/96 kHz, finite bounded output, exp2 accuracy");
}
static void continuity(){
  int p[SynthParameterCount];memcpy(p,presetValues(1,0),sizeof(p));p[10]=500;p[11]=0;
  voice=MonoDSP{};voice.on(33,55,100,p,1,32000,1);render(8000);
  float phase=voice.phase,env=voice.env;voice.on(45,110,100,p,1,32000,1);
  assert(voice.phase==phase&&voice.env==env);render(1);assert(voice.frequency>55&&voice.frequency<56);
  render(16000);assert(voice.frequency>109&&voice.frequency<110.01f);
  float filterEnv=voice.filterEnv;p[4]=-75;p[17]=60;voice.setTarget(p,1);assert(voice.filterEnv==filterEnv);
  render(3200);assert(voice.p[17]>59&&voice.p[4]<-74);
  voice.off();render(32000*3);assert(!voice.active);
  puts("PASS MONO: legato preserves phase/envelopes, glide reaches destination, live edits do not retrigger, release ends");
}
static void persistence(){
  Project p;p.trackCount=1;p.tracks[0].instrument=MonoInstrument;
  for(int i=0;i<20;++i){p.tracks[0].synthParams[0][i]=synthMax(1,i);p.tracks[0].steps[3].synthLocks[0][i]=synthMin(1,i);}
  assert(codec.encode(p)&&!memcmp(codec.bytes,"MS15",4)&&codec.decode(restored,codec.cursor));
  for(int i=0;i<20;++i)assert(restored.tracks[0].synthParams[0][i]==synthMax(1,i)&&restored.tracks[0].steps[3].synthLocks[0][i]==synthMin(1,i));
  codec.cursor=16;codec.ok=true;codec.fields(p,true,14);memcpy(codec.bytes,"MS14",4);codec.word(4,14);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));assert(restored.tracks[0].synthParams[0][9]==0&&restored.tracks[0].synthParams[0][11]==0&&restored.tracks[0].synthParams[0][15]==0);
  for(int i=8;i<20;++i)assert(restored.tracks[0].steps[3].synthLocks[0][i]==Inherit);
  puts("PASS MONO: twenty controls/locks MS15, real MS14 layout, neutral added modulation and inherited locks");
}
static void controls(){
  app.project=Project{};app.project.trackCount=1;app.project.tracks[0].instrument=MonoInstrument;app.track=0;app.page=Sampler;
  app.engine=&engine;engine.init(app.project,nullptr,32000,0);assert(app.voiceBanks()==5);
  for(int page=0;page<5;++page){assert(app.synthBank==page);int i=page*4;
    int old=app.project.tracks[0].synthParams[0][i];app.edit(0,1,1);assert(app.project.tracks[0].synthParams[0][i]==old+1);app.parameterPage(1);}
  assert(app.synthBank==0);app.page=Pattern;app.bank=1;app.step=3;
  for(int page=0;page<5;++page){app.synthBank=page;app.edit(0,1,1);int i=page*4;assert(app.project.tracks[0].steps[3].synthLocks[0][i]==app.project.tracks[0].synthParams[0][i]+1);}
  for(int i=0;i<8;++i)assert(app.project.tracks[0].steps[3].synthLocks[1][i]==Inherit&&app.project.tracks[0].steps[3].synthLocks[2][i]==Inherit);
  engine.init(app.project,nullptr,32000,0);engine.handle({Op::Audition,0});engine.render(audio,1024);auto v=engine.monoView[0].read();assert(v.active&&v.amp>0&&v.cutoff>0);
  engine.handle({Op::Stop});engine.render(audio,1024);v=engine.monoView[0].read();assert(!v.active);for(int x:v.wave)assert(x==0);
  puts("PASS MONO: five banks, voice and lock edits, isolated other instruments, real scope and idle reset");
}
int main(){limits();continuity();persistence();controls();}
