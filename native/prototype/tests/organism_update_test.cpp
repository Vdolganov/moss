#include "../src/app.h"
#include "legacy_fixture.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static SynthTables tables;static OrganismDSP voice;static Engine engine;
static App app;static Codec codec;static Project restored;static int16_t audio[2048];
static void render(unsigned count){for(unsigned i=0;i<count;++i){float l,r;voice.render(l,r,tables.organism,tables.mono);assert(isfinite(l)&&isfinite(r)&&fabsf(l)<=.981f&&fabsf(r)<=.981f);}}
static void limits(){
  tables.init();const unsigned rates[]={8000,32000,44100,96000};
  for(unsigned rate:rates)for(int mode=0;mode<2;++mode)for(int edge=0;edge<3;++edge){
    int p[OrganismParameters];for(int i=0;i<OrganismParameters;++i)p[i]=edge==0?synthMin(2,i):edge==1?synthMax(2,i):OrganismPresets[2][i];
    p[OrgAttack]=0;voice=OrganismDSP{};int midi=edge==0?96:24;
    voice.on(midi,tables.frequency[midi],127,p,mode,rate,17);render(rate/2);
    for(int i=0;i<OrganismParameters;++i)p[i]=synthMax(2,i);
    voice.setTarget(p,!mode);render(rate/2);voice.stop(128);render(129);assert(!voice.active);
  }
  puts("PASS ORGANISM: two topologies / all-control extrema / 8,32,44.1,96 kHz / finite bounded stereo");
}
static void continuity(){
  int p[OrganismParameters];memcpy(p,OrganismPresets[0],sizeof(p));p[OrgAttack]=20;p[OrgRelease]=300;p[OrgGlide]=500;
  voice=OrganismDSP{};voice.on(33,55,110,p,0,32000,17);render(8000);
  float phase=voice.phase[0],env=voice.env;voice.on(45,110,110,p,1,32000,77);
  assert(voice.phase[0]==phase&&voice.env==env);render(1);assert(voice.frequency>55&&voice.frequency<57);
  render(16000);assert(voice.frequency>109&&voice.frequency<110.01f);
  phase=voice.phase[0];env=voice.env;p[OrgGrit]=85;p[OrgNoise]=50;p[OrgSub]=75;
  voice.setTarget(p,0);assert(phase==voice.phase[0]&&env==voice.env);
  render(3200);assert(voice.p[OrgGrit]>84&&voice.p[OrgSub]>74);
  voice.off();render(32000);assert(!voice.active);
  puts("PASS ORGANISM: legato and lock edits preserve phase/envelope; glide, live texture and release");
}
static void persistence(){
  Project p;p.trackCount=1;p.tracks[0].instrument=OrganismInstrument;
  for(int i=0;i<20;++i){p.tracks[0].synthParams[1][i]=synthMax(2,i);p.tracks[0].steps[3].synthLocks[1][i]=synthMin(2,i);}
  assert(codec.encode(p)&&!memcmp(codec.bytes,"MS18",4)&&codec.decode(restored,codec.cursor));
  for(int i=0;i<20;++i)assert(restored.tracks[0].synthParams[1][i]==synthMax(2,i)&&restored.tracks[0].steps[3].synthLocks[1][i]==synthMin(2,i));
  p=Project{};p.trackCount=1;p.tracks[0].instrument=OrganismInstrument;
  p.tracks[0].steps[3].on=LockEvent;p.tracks[0].steps[3].synthLocks[1][1]=80;p.tracks[0].steps[3].synthLocks[1][4]=65;
  p.tracks[0].steps[4].synthLocks[1][4]=ResetLock;
  codec.cursor=16;codec.ok=true;legacyFields(codec,p,15);memcpy(codec.bytes,"MS15",4);codec.word(4,15);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));auto &t=restored.tracks[0];
  assert(t.synthParams[1][OrgShape]==30&&t.synthParams[1][OrgCutoff]==3730&&t.synthParams[1][OrgAttack]==450);
  assert(t.steps[3].on==LockEvent&&t.steps[3].synthLocks[1][OrgDrift]==24&&t.steps[3].synthLocks[1][OrgCutoff]==5170);
  assert(t.steps[4].synthLocks[1][OrgCutoff]==ResetLock&&t.steps[3].synthLocks[1][OrgSub]==Inherit);
  puts("PASS ORGANISM: all 20 controls/locks MS17 roundtrip; authentic MS15 field mapping, lock-only and RESET preserved");
}
static void controls(){
  app.project=Project{};app.project.trackCount=1;app.project.tracks[0].instrument=OrganismInstrument;app.track=0;app.page=Sampler;app.engine=&engine;
  engine.init(app.project,nullptr,32000,0);assert(app.voiceBanks()==5);
  for(int page=0;page<5;++page){assert(app.synthBank==page);int i=page*4,old=app.project.tracks[0].synthParams[1][i];app.edit(0,1,1);assert(app.project.tracks[0].synthParams[1][i]==old+1);app.parameterPage(1);}
  assert(app.synthBank==0);app.page=Pattern;app.bank=1;app.step=3;
  for(int page=0;page<5;++page){app.synthBank=page;app.edit(0,1,1);int i=page*4;assert(app.project.tracks[0].steps[3].synthLocks[1][i]==app.project.tracks[0].synthParams[1][i]+1);}
  engine.init(app.project,nullptr,32000,0);engine.handle({Op::Audition,0});engine.render(audio,1024);auto v=engine.organismView[0].read();assert(v.active&&v.amp>0);
  engine.handle({Op::Stop});engine.render(audio,1024);assert(!engine.organismView[0].read().active);
  // A true sequencer lock event must update the already sounding drone.
  Project p;p.trackCount=1;p.bpm=120;auto &t=p.tracks[0];t.instrument=OrganismInstrument;t.cycle=256;
  t.synthParams[1][OrgAttack]=10;t.steps[0].on=NoteEvent;t.steps[0].length=256;
  t.steps[4].on=LockEvent;t.steps[4].synthLocks[1][OrgGrit]=90;
  engine.init(p,nullptr,32000,0);engine.handle({Op::Transport});for(int i=0;i<10;++i)engine.render(audio,1024);
  assert(engine.triggerCount[0]==1&&engine.lockEventCount[0]==1&&engine.synth[0].gate&&engine.synth[0].organismVoice.p[OrgGrit]>89);
  puts("PASS ORGANISM: five voice/lock banks, live portrait and STOP, sequencer lock changes held note without retrigger");
}
int main(int argc,char **argv){limits();continuity();persistence();controls();for(int i=1;i<argc;++i){assert(loadProject(argv[i],restored));printf("PASS device project %s / %s\n",argv[i],restored.name);}}
