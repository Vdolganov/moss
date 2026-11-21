#include "../src/app.h"
#include "legacy_fixture.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static SynthTables tables;
static Engine engine;
static Codec codec,legacy;
static Project project,loaded;
static int16_t audio[2048];
static void advance(unsigned frames){while(frames){unsigned n=frames>1024?1024:frames;engine.render(audio,n);frames-=n;}}
static void storageAndLocks(){
  project=Project{};project.trackCount=1;auto &tr=project.tracks[0];tr.instrument=FmInstrument;
  tr.cycle=512;tr.division=16;tr.steps[0].on=NoteEvent;tr.steps[0].note=33;tr.steps[0].length=512;
  memcpy(tr.synthParams[2],FmPresets[7],sizeof(tr.synthParams[2]));tr.synthPreset[2]=7;
  for(int i=0;i<FmParameters;++i){tr.steps[i+1].on=LockEvent;tr.steps[i+1].synthLocks[2][i]=int16_t(synthMax(FmInstrument,i));}
  assert(valid(project)&&codec.encode(project)&&!memcmp(codec.bytes,"MS18",4));
  assert(codec.decode(loaded,codec.cursor));assert(loaded.tracks[0].synthPreset[2]==7);
  for(int i=0;i<FmParameters;++i)assert(loaded.tracks[0].steps[i+1].synthLocks[2][i]==synthMax(FmInstrument,i));
  // Freeze the pre-v18 counts: an MS17 FM record contains eight parameters.
  tr.synthPreset[2]=0;legacy.cursor=16;legacy.ok=true;legacy.fields(project,true,17);
  memcpy(legacy.bytes,"MS17",4);legacy.word(4,17);legacy.word(8,legacy.cursor);legacy.word(12,legacy.hash(legacy.cursor));
  assert(legacy.decode(loaded,legacy.cursor));assert(loaded.tracks[0].synthParams[2][FmRelease]==tr.synthParams[2][FmDecay]/2);
  assert(loaded.tracks[0].steps[9].synthLocks[2][FmPunch]==Inherit);
  // A held voice gets new targets on lock-only steps without envelope/age reset.
  engine.init(project,nullptr,AudioRate,0);engine.handle({Op::Transport});advance(1);
  auto &v=engine.synth[0].fmVoice;assert(v.age==1);advance(AudioRate*3);
  assert(engine.triggerCount[0]==1&&v.age==AudioRate*3+1&&v.active);
  assert(v.target[FmPunch]==48&&v.target[FmModDecay]==15000);
  engine.handle({Op::Stop});advance(256);assert(!v.active);
  puts("PASS FM MS18 roundtrip, MS17 migration, twenty locks and held-note continuity");
}
static void quality(){
  tables.init();double energies[FmPresetCount]={};
  for(int preset=0;preset<FmPresetCount;++preset){
    FmDSP v;v.on(33,55,110,FmPresets[preset],FmPresetModes[preset],AudioRate,17);
    double sum=0,energy=0,side=0;float peak=0;
    for(unsigned i=0;i<AudioRate*4;++i){float l,r;v.render(l,r,tables.mono);assert(isfinite(l)&&isfinite(r)&&fabsf(l)<=.98f&&fabsf(r)<=.98f);
      energy+=(l*l+r*r)*.5;side+=(l-r)*(l-r);sum+=(l+r)*.5;peak=fmaxf(peak,fmaxf(fabsf(l),fabsf(r)));}
    energies[preset]=energy;assert(energy>1&&peak>.01f);assert(fabs(sum/(AudioRate*4))<.006);
    if(preset==7)assert(side/energy>.01&&v.env>.8f&&v.modEnv>.01f);
    printf("FM %s rms=%.5f peak=%.5f stereo=%.5f\n",FmPresetNames[preset],sqrt(energy/(AudioRate*4)),peak,side/energy);
  }
  assert(fabs(energies[0]-energies[4])>1);
  const unsigned rates[]={8000,32000,48000,96000};
  for(unsigned rate: rates){
    int p[FmParameters];memcpy(p,FmPresets[7],sizeof(p));p[FmIndex]=p[FmFeedback]=p[FmNoise]=p[FmClick]=p[FmMotion]=0;p[FmDetune]=0;p[FmBody]=100;p[FmAttack]=0;p[FmTone]=100;
    for(int octave=0;octave<3;++octave){float hz=27.5f*(1<<octave);FmDSP v;v.on(21+12*octave,hz,127,p,0,rate,9);
      double energy=0,sum=0;int crosses=0;float previous=0;
      for(unsigned i=0;i<rate*3;++i){float l,r;v.render(l,r,tables.mono);if(i>=rate){energy+=l*l;sum+=l;if(l>=0&&previous<0)++crosses;}previous=l;}
      assert(fabs(crosses/2.f-hz)<1);assert(sqrt(energy/(rate*2))>.38);assert(fabs(sum/(rate*2))<.008);
    }
  }
  puts("PASS sustained stereo, deep 27.5/55/110 Hz fundamentals, DC and sample-rate pitch");
}
static void controls(){
  for(int slot=0;slot<FmParameters;++slot){
    FmDSP a,b;int p[FmParameters],q[FmParameters];memcpy(p,FmPresets[6],sizeof(p));
    p[FmModHold]=35;p[FmBody]=65;p[FmAttack]=slot==FmClick?0:15;p[FmSpread]=85;p[FmMotion]=55;p[FmDetune]=45;
    memcpy(q,p,sizeof(q));p[slot]=synthMin(FmInstrument,slot);q[slot]=synthMax(FmInstrument,slot);
    a.on(33,55,80,p,1,32000,17);b.on(33,55,80,q,1,32000,17);double difference=0;
    for(int i=0;i<32000*2;++i){if(i==16000&&slot==FmGlide){a.on(45,110,80,p,1,32000,17);b.on(45,110,80,q,1,32000,17);}if(i==32000){a.off();b.off();}float al,ar,bl,br;a.render(al,ar,tables.mono);b.render(bl,br,tables.mono);difference+=(al-bl)*(al-bl)+(ar-br)*(ar-br);}
    if(difference<=.01)fprintf(stderr,"FM control %d %s difference %.9f\n",slot,synthLabel(FmInstrument,slot),difference);
    assert(difference>.01);
  }
  puts("PASS all twenty FM controls alter rendered strike, sustain, stereo, glide or release");
}
static void telemetry(){
  FmDSP v;FmMailbox box;int p[FmParameters];memcpy(p,FmPresets[4],sizeof(p));p[FmPunch]=48;p[FmDynamics]=100;
  v.on(33,55,24,p,0,AudioRate,17);float l,r;v.render(l,r,tables.mono);box.publish(v,true);auto view=box.read();
  float expected=48*(.3f+.7f*24/127.f);assert(abs(view.pitch-int(expected*100))<=1);
  p[FmRatio]=1;unsigned age=v.age;float env=v.env;v.setTarget(p,0);box.publish(v,true);view=box.read();
  assert(view.ratio==1&&v.age==age&&v.env==env);box.publish(v,false);assert(!box.read().active);
  puts("PASS FM telemetry: effective locked ratio and velocity-scaled pitch, no note restart");
}
int main(){storageAndLocks();quality();controls();telemetry();}
