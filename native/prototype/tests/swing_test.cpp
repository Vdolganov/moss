#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
using namespace moss;
static Engine engine;
static App app;
static Project project, restored;
static SampleBuffer sample;
static Codec codec;
static SongBank song;
static int16_t output[2048];
static void render(unsigned frames,unsigned block=1024) {
  while(frames) { unsigned n=frames<block?frames:block; engine.render(output,n); frames-=n; }
}
static void setup(int instrument=MonoInstrument) {
  project=Project{}; project.trackCount=1; project.bpm=120;
  auto &t=project.tracks[0];t.instrument=instrument;t.cycle=64;
  t.granular.asset=0;t.keys[0].asset=0;
}
static void start(unsigned rate=32000) {
  assert(valid(project));engine.init(project,&sample,rate,1);engine.handle({Op::Transport});
}
struct Expected { uint64_t frame; int track; };
static Expected expected[4096];
static int order(const void *a,const void *b) {
  auto x=static_cast<const Expected*>(a)->frame,y=static_cast<const Expected*>(b)->frame;
  return x<y?-1:x>y?1:0;
}
// Independent musical-time oracle, including overlapping events, hidden ticks,
// clipped startup, early cycle wraps and arbitrary odd pattern lengths.
static void trace(unsigned rate,int bpm) {
  setup(SampleInstrument);project.bpm=bpm;project.trackCount=Tracks;
  const int cycles[]={60,64,15,7},divisions[]={8,16,32,64},swing[]={75,66,50,58};
  const int shifts[]={-50,0,50,-23,19,Inherit};
  int count=0;
  unsigned end=unsigned(uint64_t(rate)*60*128/(bpm*16));
  for(int t=0;t<Tracks;++t) {
    auto &tr=project.tracks[t];tr.instrument=SampleInstrument;tr.keys[0].asset=0;
    tr.cycle=cycles[t];tr.division=divisions[t];tr.swing=swing[t];
    for(int i=0;i<tr.cycle;++i) {tr.hits[0][i].on=NoteEvent;tr.steps[i].shift=shifts[(i+t)%6];}
    for(int tick=0;tick<140;++tick) {
      const int at=tick%cycles[t],stride=64/divisions[t];
      const int shift=shifts[(at+t)%6]==Inherit?0:shifts[(at+t)%6];
      int within=at%stride;
      int time=tick*100+stride*shift+((at/stride)%2?stride-within:within)*(swing[t]-50)*2;
      if(time<0)time=0;
      uint64_t numerator=uint64_t(time)*rate*60,denominator=uint64_t(bpm)*16*100;
      uint64_t frame=(numerator+denominator-1)/denominator;
      if(frame<end) expected[count++]={frame,t};
    }
  }
  qsort(expected,count,sizeof(Expected),order);start(rate);
  unsigned counts[Tracks]={};int index=0;
  for(unsigned f=0;f<end;++f) {
    while(index<count && expected[index].frame==f) ++counts[expected[index++].track];
    engine.render(output,1);
    for(int t=0;t<Tracks;++t) {
      assert(engine.triggerCount[t]==counts[t]);
      assert(!engine.schedules[t].overflow && engine.schedules[t].count<=StepSchedule::Capacity);
    }
  }
  assert(index==count);
}
static uint32_t hash(unsigned block) {
  start();uint32_t h=2166136261u;
  for(unsigned done=0;done<68000;) {
    unsigned n=block<68000-done?block:68000-done;engine.render(output,n);
    for(unsigned i=0;i<n*2;++i)h=(h^uint16_t(output[i]))*16777619u;
    done+=n;
  }
  return h;
}
int main() {
  setbuf(stdout,nullptr);
  sample.rate=32000;sample.channels=1;sample.frames=100000;sample.bytes=200000;
  for(unsigned i=0;i<sample.frames;++i)sample.pcm[i]=int16_t(sinf(i*.03f)*6000);
  sample.frames=32;
  const int tempos[]={30,70,240};
  for(int bpm:tempos) trace(8000,bpm);
  trace(32000,137);trace(44100,137);
  puts("PASS sample-exact SWING + SHIFT at 8/16/32/64, 30/70/137/240 BPM, 8/32/44.1 kHz, odd cycles, reordered/tied events, early wraps, bounded queue");
  sample.frames=100000;
  for(int kind=0;kind<InstrumentCount;++kind) {
    setup(kind);auto &t=project.tracks[0];t.swing=66;
    if(kind) {t.steps[4].on=NoteEvent;t.steps[4].length=8;}
    else {t.hits[0][4].on=NoteEvent;t.hits[0][4].length=8;}
    start();render(5280);assert(!engine.triggerCount[0]);render(1);assert(engine.triggerCount[0]==1);
    assert(engine.lastTrigger[0]==5280);render(7999);
    if(kind==GranularInstrument) assert(engine.granular[0].gate);
    else if(kind) assert(engine.synth[0].gate);
    else assert(!engine.voice(0,0).release);
    render(1);
    if(kind==GranularInstrument) assert(!engine.granular[0].gate);
    else if(kind) assert(!engine.synth[0].gate);
    else assert(engine.voice(0,0).release);
  }
  puts("PASS all five instruments: shifted notes keep their full LENGTH measured from actual onset");
  setup(GranularInstrument);auto &t=project.tracks[0];
  t.steps[0].on=NoteEvent;t.steps[0].length=32;
  t.steps[8].on=LockEvent;t.steps[8].shift=-25;t.steps[8].grainLocks[10]=1400;t.steps[8].repeat=32;
  t.steps[12].on=LockEvent;t.steps[12].shift=25;t.steps[12].repeat=0;
  start();render(7000);assert(!engine.lockEventCount[0]);auto head=engine.granular[0].head,advance=engine.granular[0].scanStep;
  render(1);assert(engine.lockEventCount[0]==1 && engine.triggerCount[0]==1);
  assert(engine.granular[0].head==head+advance && engine.grainValues[0][10]==1400 && engine.repeats[0].division==32);
  render(6000);assert(engine.lockEventCount[0]==2 && !engine.repeats[0].division);
  render(19000);assert(!engine.granular[0].gate && engine.triggerCount[0]==1);
  assert(hash(1)==hash(257) && hash(257)==hash(1024));
  puts("PASS shifted LOCK ONLY + REPEAT: no retrigger, unchanged gate deadline/head, block-size invariant audio");
  setup();project.tracks[0].steps[0].on=NoteEvent;project.tracks[0].steps[0].shift=-50;
  start();render(62000);assert(engine.triggerCount[0]==1);render(1);assert(engine.triggerCount[0]==2 && engine.lastTrigger[0]==62000);
  engine.handle({Op::Stop});render(4000);assert(engine.triggerCount[0]==2);
  engine.handle({Op::Transport});render(1);assert(engine.triggerCount[0]==3);
  puts("PASS first early step clips only at START; subsequent cycles anticipate the wrap; STOP clears pending events");
  setup();auto &editTrack=project.tracks[0];editTrack.steps[4].on=NoteEvent;editTrack.steps[4].length=8;
  start();render(1000);engine.handle({Op::Track,0,0,TrackSwingField,75});render(5000);assert(!engine.triggerCount[0]);
  render(1);assert(engine.triggerCount[0]==1 && engine.lastTrigger[0]==6000);
  engine.handle({Op::Step,0,4,StepShiftField,-50});render(1);assert(engine.triggerCount[0]==1);
  engine.handle({Op::Track,0,0,0,60});render(7999);assert(!engine.synth[0].gate);
  // A tempo change changes musical time, never drops a pending step.
  setup();project.tracks[0].swing=75;project.tracks[0].steps[4].on=NoteEvent;
  start();render(3000);engine.handle({Op::Global,0,0,0,240});render(1500);assert(!engine.triggerCount[0]);
  render(1);assert(engine.triggerCount[0]==1 && engine.lastTrigger[0]==4500);
  puts("PASS live timing edits affect only pending events; CYCLE does not orphan gates; BPM changes preserve musical deadlines");
  setup();song.count=2;song.song.count=2;song.song.bpm=120;song.song.loop=0;
  song.map[0]=0;song.map[1]=1;song.song.slots[0]={0,1,1};song.song.slots[1]={1,1,1};
  project.tracks[0].steps[0].on=NoteEvent;project.tracks[0].steps[0].shift=-50;
  song.sequences[0]=project;song.sequences[1]=project;
  engine.init(project,&sample,32000,1);engine.startSong(song,0);render(64000);
  assert(engine.triggerCount[0]==1);render(1);assert(engine.songSlot==1 && engine.triggerCount[0]==2);
  render(64000);assert(!engine.playing && engine.triggerCount[0]==2);
  puts("PASS SONG slot boundary: no early next-slot note, no leaked pending events, exact slot duration");
  app=App{};app.engine=&engine;app.project=project;app.page=Pattern;app.bank=3;app.step=8;
  auto gesture=[&](int face,int arrow,uint64_t now) {
    app.button(face,true,now);app.button(arrow,true,now+10000);
    app.button(arrow,false,now+20000);app.button(face,false,now+30000);
  };
  gesture(lab::L1,lab::Right,1000000);assert(app.timingBank==1 && app.bank==3 && app.step==8);
  gesture(lab::Y,lab::Right,2000000);assert(app.project.tracks[0].swing==51);
  gesture(lab::Y,lab::Up,3000000);assert(app.project.tracks[0].swing==61);
  gesture(lab::B,lab::Left,4000000);assert(app.project.tracks[0].steps[8].shift==-1);
  gesture(lab::B,lab::Down,5000000);assert(app.project.tracks[0].steps[8].shift==-11);
  assert(app.project.tracks[0].steps[8].on==EmptyEvent); // Timing metadata creates no voice or phantom lock.
  app.option=app.optionIndex(4);assert(app.option>=0);app.chooseOption();assert(app.project.tracks[0].steps[8].shift==Inherit);
  app.project.tracks[0].steps[8].shift=37;
  assert(codec.encode(app.project) && !memcmp(codec.bytes,"MS13",4) && codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].swing==61 && restored.tracks[0].steps[8].shift==37);
  codec.cursor=16;codec.ok=true;codec.fields(project,true,11);memcpy(codec.bytes,"MS11",4);codec.word(4,11);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor) && restored.tracks[0].swing==50 && restored.tracks[0].steps[0].shift==Inherit);
  restored.tracks[0].swing=76;assert(!valid(restored));restored.tracks[0].swing=50;
  restored.tracks[0].steps[0].shift=-51;assert(!valid(restored));
  puts("PASS physical controls, fine/coarse, nested-page release guard, clear SHIFT, inactive empty-step metadata, MS12 roundtrip/MS11 migration and validation");
}
