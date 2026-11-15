#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Engine engine;
static App app;
static SampleBuffer sample;
static Project project, restored;
static Codec codec;
static SongBank song;
static AudioRepeat repeat;
static int16_t output[2048];
static void render(unsigned n) {
  while(n) { unsigned amount=n>1024?1024:n; engine.render(output,amount); n-=amount; }
}
static void setup(int kind) {
  project=Project{}; project.trackCount=1; project.bpm=120;
  auto &t=project.tracks[0];t.instrument=kind;t.cycle=128;t.division=16;
  t.granular.asset=0;t.granular.values[2]=8000;t.granular.values[9]=0;
  t.steps[0].on=NoteEvent;t.steps[0].length=64;
  t.steps[28].on=LockEvent;t.steps[28].grainLocks[10]=1400;
  t.steps[28].synthLocks[kind>0 && kind<4?kind-1:0][0]=50;
  t.steps[32].on=LockEvent;t.steps[32].repeat=64;
  t.steps[36].on=LockEvent;t.steps[36].fxLocks[Orbit][3]=75;
  t.steps[40].on=LockEvent;t.steps[40].grainLocks[11]=50;
  t.steps[44].on=LockEvent;t.steps[44].grainLocks[10]=ResetLock;
  t.steps[48].on=LockEvent;t.steps[48].repeat=0;
  t.steps[52].on=LockEvent;t.steps[52].probability=0;t.steps[52].repeat=8;
  t.steps[56].on=LockEvent;t.steps[56].fxLocks[Orbit][3]=ResetLock;
  assert(valid(project));engine.init(project,&sample,32000,1);engine.handle({Op::Transport});
}
static uint32_t streamHash(unsigned block) {
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});
  uint32_t h=2166136261u;
  for(unsigned done=0;done<70000;) {
    unsigned n=block<70000-done?block:70000-done;
    engine.render(output,n);for(unsigned i=0;i<n*2;++i) h=(h^uint16_t(output[i]))*16777619u;done+=n;
  }
  assert(engine.triggerCount[0]==1);return h;
}
int main() {
  sample.rate=32000;sample.channels=1;sample.frames=100000;sample.bytes=200000;
  for(unsigned i=0;i<sample.frames;++i) sample.pcm[i]=int16_t(sinf(i*.017f)*5000);
  setup(GranularInstrument); render(28000);
  auto head=engine.granular[0].head,scan=engine.granular[0].scanStep;
  unsigned age=engine.granular[0].grains[0].age;
  float envelope=engine.granular[0].envelope;
  render(1);
  assert(engine.triggerCount[0]==1 && engine.lockEventCount[0]==1);
  assert(engine.grainRemaining[0]==36 && engine.granular[0].gate);
  assert(engine.granular[0].head==head+scan);
  assert(engine.granular[0].grains[0].age==age+1);
  assert(engine.granular[0].envelope>=envelope);
  assert(engine.grainValues[0][10]==1400);
  render(4000);assert(engine.repeats[0].division==64 && engine.triggerCount[0]==1);
  render(8000);assert(engine.grainValues[0][10]==1400 && engine.grainValues[0][11]==50);
  render(4000);assert(engine.grainValues[0][10]==project.tracks[0].granular.values[10]);
  assert(engine.grainValues[0][11]==50 && engine.eventValues[0].fxLocks[Orbit][3]==75);
  render(8000);assert(engine.repeats[0].division==0 && engine.triggerCount[0]==1);
  render(4000);assert(engine.eventValues[0].fxLocks[Orbit][3]==Inherit);
  render(8000);assert(!engine.granular[0].gate && engine.grainRemaining[0]==0);
  auto h1=streamHash(1),h2=streamHash(257);assert(h1==h2);
  puts("PASS held GRANULAR: locks/repeat preserve gate deadline, read head, grain ages and envelope; per-parameter persistence/RESET/probability; block invariant");
  for(int kind=1;kind<=3;++kind) {
    setup(kind);render(28001);assert(engine.synth[0].remaining==36 && engine.synth[0].gate);
    assert(engine.synth[0].target[0]==50 && engine.triggerCount[0]==1);
    render(36000);assert(!engine.synth[0].gate && engine.triggerCount[0]==1);
  }
  puts("PASS MONO/ORGANISM/FM: no note restart or LENGTH extension on locks/repeat");
  setup(SampleInstrument);auto &t=project.tracks[0];
  t.steps[0]=Step{};t.steps[28]=Step{};
  for(int k=0;k<2;++k) { t.keys[k].asset=0;t.hits[k][0].on=NoteEvent;t.hits[k][0].length=64; }
  t.hits[0][28].on=LockEvent;t.hits[0][28].locks[0]=12;
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});render(28000);
  auto pos=engine.voice(0,0).phase;age=engine.voice(0,0).age;
  render(1);assert(engine.voice(0,0).phase==pos+engine.voice(0,0).increment && engine.voice(0,0).age==age+1);
  assert(engine.voice(0,0).locks[0]==12 && engine.voice(0,1).locks[0]==Inherit);
  assert(engine.voice(0,0).remaining==36 && engine.triggerCount[0]==2);
  engine.handle({Op::Stop});assert(!engine.repeats[0].division && engine.activeFxStep[0]==-1);
  puts("PASS SAMPLER: selected lane only, phase/age preserved; track repeat; stop clears event state");
  // Exact repeat period and stereo capture at every supported division/BPM.
  const int divisions[]={8,16,32,64}; const int tempos[]={30,70,240};
  for(int div:divisions) for(int bpm:tempos) {
    repeat.reset();repeat.set(div,32000,bpm);unsigned period=32000u*240/(bpm*div);
    assert(repeat.length==period && period<=AudioRepeat::Capacity);
    for(unsigned i=0;i<period;++i) { int l=int(i%3000),r=-l;repeat.process(l,r); }
    for(unsigned i=0;i<period*2;++i) { int l=99000,r=-99000;repeat.process(l,r);
      if(i%period>=AudioRepeat::Ramp) assert(l==int((i%period)%3000) && r==-l);
    }
    repeat.set(0,32000,bpm);for(unsigned i=0;i<=AudioRepeat::Ramp;++i) {int l=777,r=-777;repeat.process(l,r);if(i==AudioRepeat::Ramp)assert(l==777&&r==-777);}
  }
  puts("PASS REPEAT: exact 1/8..1/64 periods, 30..240 BPM, stereo fragment, bounded storage and return to live sound");
  setup(GranularInstrument);app.engine=&engine;app.project=project;app.page=Pattern;app.track=0;app.step=8;app.bank=1;app.synthBank=2;
  app.edit(2,-1,1);assert(app.project.tracks[0].steps[8].on==LockEvent);
  app.change({Op::GranularLock,0,8,10,Inherit});assert(app.project.tracks[0].steps[8].on==EmptyEvent);
  app.bank=3;app.edit(2,1,1);assert(app.project.tracks[0].steps[8].repeat==0 && app.project.tracks[0].steps[8].on==LockEvent);
  app.edit(2,1,1);assert(app.project.tracks[0].steps[8].repeat==8);
  app.edit(3,-1,1);assert(app.project.tracks[0].steps[8].on==NoteEvent);
  app.action(lab::A);assert(app.project.tracks[0].steps[8].on==EmptyEvent);
  assert(app.project.tracks[0].steps[8].repeat==8); // Off never silently activates hidden locks.
  assert(app.editLock(20,50,20,100,-1,1)==ResetLock);
  assert(app.editLock(ResetLock,50,20,100,-1,1)==Inherit);
  app.change({Op::GranularLock,0,8,10,ResetLock});assert(app.project.tracks[0].steps[8].on==LockEvent);
  assert(codec.encode(app.project) && !memcmp(codec.bytes,"MS11",4));assert(codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].steps[8].grainLocks[10]==ResetLock && restored.tracks[0].steps[8].repeat==8);
  setup(GranularInstrument);project.tracks[0].steps[28].on=EmptyEvent;
  // Old off steps with locks must stay off, and repeat must default to --.
  for(auto &s:project.tracks[0].steps) { s.on=s.on==LockEvent?EmptyEvent:s.on; for(auto &v:s.grainLocks)if(v==ResetLock)v=Inherit;for(auto &row:s.fxLocks)for(auto &v:row)if(v==ResetLock)v=Inherit; }
  codec.cursor=16;codec.ok=true;codec.fields(project,true,10);memcpy(codec.bytes,"MS10",4);codec.word(4,10);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));assert(restored.tracks[0].steps[28].on==EmptyEvent && restored.tracks[0].steps[32].repeat==Inherit);
  puts("PASS UI/model/MS11: automatic LOCK ONLY, last-lock removal, note-off inert payload, TYPE/REPEAT, distinct RESET/--, MS10 migration");
  setup(GranularInstrument);project.trackCount=2;project.tracks[1]=project.tracks[0];
  project.tracks[0].cycle=60;project.tracks[1].cycle=64;
  project.tracks[0].division=8;project.tracks[1].division=32;
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});render(61001);
  assert(engine.triggerCount[0]==2 && engine.triggerCount[1]==1);
  assert(engine.shownStep[0]==1 && engine.shownStep[1]==61);
  assert(engine.grainValues[0][11]==project.tracks[0].granular.values[11] && engine.grainValues[1][11]==50);
  assert(!engine.repeats[0].division);
  puts("PASS independent 15/16 cycles and 1/8 vs 1/32 resolution; new NOTE resets previous parameter events");
  setup(GranularInstrument);project.tracks[0].steps[0]=Step{};
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});render(60001);
  assert(!engine.granular[0].running && !engine.triggerCount[0] && engine.lockEventCount[0]>0);
  engine.handle({Op::Stop});engine.handle({Op::Transport});render(1);
  assert(engine.eventValues[0].grainLocks[11]==Inherit && !engine.repeats[0].division);
  // A lock can change a delay tail after the input voice has finished.
  setup(SampleInstrument);auto &fxTrack=project.tracks[0];fxTrack.steps[0]=Step{};
  fxTrack.keys[0].asset=0;fxTrack.hits[0][0].on=NoteEvent;sample.frames=800;
  fxTrack.fx.select(Orbit);fxTrack.fx.enable(true);fxTrack.fx.values[Orbit][0]=0;fxTrack.fx.values[Orbit][3]=100;
  fxTrack.steps[4].on=LockEvent;fxTrack.steps[4].fxLocks[Orbit][1]=90;
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});render(4001);
  assert(engine.triggerCount[0]==1 && !engine.voice(0,0).active);
  assert(engine.eventValues[0].fxLocks[Orbit][1]==90);
  unsigned energy=0;for(int i=0;i<8;++i) {engine.render(output,1024);for(int16_t v:output)energy+=unsigned(v<0?-v:v);}
  assert(energy>0);sample.frames=100000;
  puts("PASS silent LOCK ONLY does not create voices; START/STOP reset overrides; FX events process an existing tail");
  setup(GranularInstrument);song.count=2;song.song.count=2;song.song.bpm=120;song.song.loop=0;
  song.map[0]=0;song.map[1]=1;song.song.slots[0]={0,1,1};song.song.slots[1]={1,1,1};
  song.sequences[0]=project;song.sequences[0].tracks[0].steps[0].repeat=8;
  song.sequences[1]=project;for(auto &s:song.sequences[1].tracks[0].steps)s=Step{};
  engine.startSong(song,0);render(64001);
  assert(engine.songSlot==1 && engine.triggerCount[0]==1 && !engine.repeats[0].division);
  assert(engine.eventValues[0].grainLocks[11]==Inherit && !engine.granular[0].running);
  render(64000);assert(!engine.playing);
  puts("PASS SONG transition/end: repeat and parameter events cannot leak into the next sequence");
  app=App{};app.engine=&engine;app.project=project;app.page=Pattern;app.track=0;app.step=8;app.bank=3;
  auto gesture=[&](int face,int direction,uint64_t now) {
    app.button(face,true,now);app.button(direction,true,now+10000);
    app.button(direction,false,now+20000);app.button(face,false,now+30000);
  };
  gesture(lab::A,lab::Right,1000000);assert(app.project.tracks[0].steps[8].repeat==0 && app.project.tracks[0].steps[8].on==LockEvent);
  gesture(lab::A,lab::Right,2000000);assert(app.project.tracks[0].steps[8].repeat==8 && app.step==8 && app.bank==3);
  gesture(lab::X,lab::Left,3000000);assert(app.project.tracks[0].steps[8].on==NoteEvent);
  gesture(lab::X,lab::Right,4000000);assert(app.project.tracks[0].steps[8].on==LockEvent);
  app.bank=1;app.synthBank=2;app.project.tracks[0].steps[8].grainLocks[10]=20;
  gesture(lab::A,lab::Left,5000000);assert(app.project.tracks[0].steps[8].grainLocks[10]==ResetLock);
  gesture(lab::A,lab::Left,6000000);assert(app.project.tracks[0].steps[8].grainLocks[10]==Inherit);
  app.option=app.optionIndex(7);app.chooseOption();assert(app.project.tracks[0].steps[8].on==NoteEvent);
  app.option=app.optionIndex(7);app.chooseOption();assert(app.project.tracks[0].steps[8].on==LockEvent);
  app.option=app.optionIndex(6);app.chooseOption();assert(app.project.tracks[0].steps[8].on==EmptyEvent && app.project.tracks[0].steps[8].repeat==Inherit);
  puts("PASS physical Y/B/A/X mapping: A+arrows REPEAT/RESET, X+arrows TYPE, no short-action leakage, context conversion/clear");
}
