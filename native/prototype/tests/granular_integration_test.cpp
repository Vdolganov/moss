#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Engine engine, other;
static App app;
static SampleBuffer samples[3];
static Codec codec;
static Project project, restored;
static SongBank songs;
static Library library;
static lab::Canvas canvas;
static int16_t block[2048], reference[2048];
static uint64_t now = 1000000;
static void render(Engine &e, unsigned n) {
  while(n) { unsigned take=n>1024?1024:n; e.render(block,take); n-=take; }
}
static void tap(int key) { app.button(key,true,now); app.button(key,false,now+30000); now+=100000; }
static void gesture(int face,int direction) {
  app.button(face,true,now); app.button(direction,true,now+10000);
  app.button(direction,false,now+20000); app.button(face,false,now+30000); now+=100000;
}
static void setup() {
  project=Project{}; project.trackCount=1; project.bpm=120;
  auto &t=project.tracks[0]; t.instrument=GranularInstrument; t.granular.asset=0;
  t.granular.values[1]=0; t.granular.values[14]=t.granular.values[15]=20;
  t.steps[0].on=1; t.steps[0].length=16; t.steps[0].note=48;
  t.steps[32].on=1; t.steps[32].note=60; t.steps[32].length=8;
  t.steps[32].grainLocks[0]=-50; t.steps[32].grainLocks[4]=800;
}
static void codecAndProtection() {
  setup(); project.tracks[0].granular.freeze=1;
  for(int i=0;i<GranularParameterCount;++i) project.tracks[0].steps[31].grainLocks[i]=GranularSpecs[i].high;
  assert(codec.encode(project)); assert(!memcmp(codec.bytes,"MS11",4));
  assert(codec.decode(restored,codec.cursor));
  unsigned storedSize=codec.cursor; uint32_t storedHash=codec.hash(storedSize);
  assert(codec.encode(restored)); assert(codec.cursor==storedSize && codec.hash(codec.cursor)==storedHash);
  // MOS7 milliseconds cannot silently become percentages. The beta migration
  // changes only the replaced control; other locks, sources and notes survive.
  project.tracks[0].granular.values[7]=180;
  project.tracks[0].steps[31].grainLocks[7]=500;
  codec.cursor=16; codec.ok=true; codec.fields(project,true,7);
  memcpy(codec.bytes,"MOS7",4); codec.word(4,7); codec.word(8,codec.cursor); codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].granular.values[7]==100);
  assert(restored.tracks[0].steps[31].grainLocks[7]==Inherit);
  assert(restored.tracks[0].steps[31].grainLocks[10]==GranularSpecs[10].high);
  assert(restored.tracks[0].granular.asset==0);
  project=restored;
  // Real MOS6 payload, not a tag-only fake: defaults must fill the new fields.
  project.tracks[0].instrument=MonoInstrument;
  codec.cursor=16; codec.ok=true; codec.fields(project,true,6);
  memcpy(codec.bytes,"MOS6",4); codec.word(4,6); codec.word(8,codec.cursor); codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].granular.asset==-1);
  assert(restored.tracks[0].steps[31].grainLocks[0]==Inherit);
  app.engine=&engine; app.project=project;
  assert(app.projectUses(project,0)); assert(app.assetMask(project)&1); assert(app.assetProtected(0));
  project.tracks[0].granular.values[6]=project.tracks[0].granular.values[5];
  assert(!valid(project));
}
static void playback() {
  setup(); engine.init(project,samples,32000); other.init(project,samples,32000);
  engine.handle({Op::Transport}); other.handle({Op::Transport});
  long long energy=0;
  for(unsigned start=0;start<96000;start+=1024) {
    unsigned n=96000-start<1024?96000-start:1024;
    engine.render(block,n);
    for(unsigned i=0;i<n;++i) other.render(reference+i*2,1);
    assert(!memcmp(block,reference,n*4));
    for(unsigned i=0;i<n*2;++i) energy+=abs(block[i]);
    assert(engine.granular[0].count<=8);
  }
  assert(energy>100000); // deliberately quiet tail of the prepared chime fixture
  assert(engine.seenInstrumentMask&(1u<<GranularInstrument));
  engine.handle({Op::Stop}); render(engine,32000);
  assert(!engine.granular[0].running && !engine.granular[0].count);
  // Audition gate, freeze continuing births, parameter edit and live source swap.
  engine.handle({Op::Audition,0}); render(engine,8000);
  engine.handle({Op::GranularFreeze,0,0,0,1}); render(engine,1024);
  auto head=engine.granular[0].head; auto births=engine.granular[0].stats.spawned;
  render(engine,16000);
  assert(head==engine.granular[0].head && engine.granular[0].stats.spawned>births);
  auto view=engine.grainView[0].read(); assert(view.count<=8 && view.running);
  render(engine,32000*20); assert(!engine.granular[0].running);
  engine.handle({Op::GranularSource,0,0,0,1}); assert(engine.grainAsset[0]==1);
  engine.handle({Op::GranularSource,0,0,0,-1});
  engine.handle({Op::Audition,0}); render(engine,1024); assert(!engine.granular[0].running);
  engine.handle({Op::Instrument,0,0,0,MonoInstrument}); render(engine,1024);
  engine.handle({Op::Instrument,0,0,0,GranularInstrument}); render(engine,1024);
}
static void controls() {
  setup(); app.project=project; app.track=0; app.page=Sampler; app.synthBank=0;
  app.options=app.dirty=app.editing=false; app.clearInput();
  engine.init(project,samples,32000); app.engine=&engine;
  gesture(lab::Y,lab::Right); gesture(lab::Y,lab::Up);
  assert(app.project.tracks[0].granular.values[0]==23);
  int size=app.project.tracks[0].granular.values[2];
  gesture(lab::A,lab::Left); assert(app.project.tracks[0].granular.values[2]==size-1);
  assert(!app.project.tracks[0].granular.freeze);
  tap(lab::X); assert(app.project.tracks[0].granular.freeze==1);
  tap(lab::R1); assert(app.synthBank==1); tap(lab::R1); assert(app.synthBank==2);
  gesture(lab::B,lab::Up); assert(app.project.tracks[0].granular.values[9]==45);
  render(engine,1024);assert(fabsf(engine.granular[0].parameters.jump-.45f)<1e-6f);
  gesture(lab::A,lab::Up); assert(app.project.tracks[0].granular.values[10]==7700);
  tap(lab::R1); assert(app.synthBank==3);
  gesture(lab::Y,lab::Up);
  assert(app.project.tracks[0].granular.values[12]==60 && app.project.tracks[0].granular.values[13]==40);
  gesture(lab::X,lab::Up); assert(app.project.tracks[0].granular.values[15]==120);
  tap(lab::R1); assert(app.synthBank==0);
  app.page=Pattern; app.bank=1; app.synthBank=2; app.step=0;
  gesture(lab::B,lab::Right); assert(app.project.tracks[0].steps[0].grainLocks[9]==46);
  engine.init(app.project,samples,32000);engine.handle({Op::Transport});render(engine,1);
  assert(fabsf(engine.granular[0].parameters.jump-.46f)<1e-6f);
  gesture(lab::A,lab::Down); assert(app.project.tracks[0].steps[0].grainLocks[10]==7600);
  assert(app.currentLocksPresent()); app.option=app.optionIndex(4); app.chooseOption();
  assert(!app.currentLocksPresent());
  app.nested(-1); assert(app.synthBank==1); app.nested(-1); assert(app.synthBank==0);
  app.page=Sampler; app.synthBank=2;
  app.voicePicker=1; app.voiceChoice=MonoInstrument; tap(lab::A);
  assert(app.synthBank==0 && app.project.tracks[0].instrument==MonoInstrument);
  app.voicePicker=1; app.voiceChoice=FmInstrument; app.navigate(lab::Down);
  assert(app.voiceChoice==GranularInstrument); tap(lab::A);
  app.assignSample(2); assert(app.project.tracks[0].granular.asset==2);
  assert(app.optionName(app.optionIndex(4)) && !strcmp(app.optionName(app.optionIndex(4)),"ASSIGN / REPLACE SAMPLE"));
  draw(canvas,app); // All four banks and lock banks use the native renderer.
  for(int bank=0;bank<GranularBankCount;++bank) { app.synthBank=bank; draw(canvas,app); app.page=Pattern;app.bank=1;draw(canvas,app);app.page=Sampler; }
}
static void arrangement() {
  // The static bank is already initialized. A temporary would exceed ARM's 8 MiB stack.
  setup(); songs.count=2;songs.song.count=2;songs.song.bpm=120;
  songs.map[0]=0;songs.map[1]=1;songs.song.slots[0]={0,1,1};songs.song.slots[1]={1,1,1};
  songs.sequences[0]=project;songs.sequences[1]=project;songs.sequences[1].tracks[0].granular.asset=1;
  engine.init(project,samples,32000);engine.startSong(songs,0);
  render(engine,64001); assert(engine.songSlot==1 && engine.grainAsset[0]==1);
  assert(engine.granular[0].sample==&samples[1]);
  engine.handle({Op::CaptureStart,0,0,0,1});render(engine,64000);
  assert(engine.capture.frames>0);
}
static void importWorkflow() {
  char original[512], directory[128];
#ifdef _WIN32
  assert(_getcwd(original,sizeof(original)));
  snprintf(directory,sizeof(directory),"build/grain-import-%lu",GetCurrentProcessId());
  makeDir(directory); assert(!_chdir(directory));
#else
  assert(getcwd(original,sizeof(original)));
  snprintf(directory,sizeof(directory),"grain-import-%ld",long(getpid()));
  makeDir(directory); assert(!chdir(directory));
#endif
  makeDir("library");makeDir("imports");makeDir("captures");makeDir("projects");
  assert(writeWav("imports/cloud.wav",samples[1].pcm,samples[1].frames,samples[1].rate,samples[1].channels));
  library.pcm[0]=samples[0];strcpy(library.names[0],"ORIGINAL");
  app=App{};app.engine=&engine;app.library=&library;setup();app.project=project;app.page=Sampler;app.track=0;
  engine.init(app.project,library.pcm,32000,Assets);
  app.pickAsset();app.action(lab::A);assert(app.importAssign&&app.filePicker);
  app.action(lab::B);app.action(lab::B);assert(app.page==Sampler&&app.project.tracks[0].granular.asset==0);
  app.pickAsset();app.action(lab::A);app.openSource("imports/cloud.wav");app.service();
  assert(app.importAssign&&app.page==Tape&&app.tapeTab==1);
  app.action(lab::A);assert(app.naming);app.action(lab::B);
  assert(app.importAssign&&app.project.tracks[0].granular.asset==0);
  assert(!rename("library","library-off"));
  app.action(lab::A);strcpy(app.nameEdit,"LENS AIR");app.action(lab::Start);app.service();
  assert(app.importAssign&&app.project.tracks[0].granular.asset==0);
  assert(!rename("library-off","library"));
  app.action(lab::A);app.action(lab::Start);app.service();
  int id=app.project.tracks[0].granular.asset;
  assert(id>0&&!app.importAssign&&app.page==Sampler&&!strcmp(library.names[id],"LENS AIR"));
  assert(engine.project.tracks[0].granular.asset==id&&app.assetProtected(id));
  assert(saveProject("projects/cloud.moss",app.project));
  assert(loadProject("projects/cloud.moss",restored)&&restored.tracks[0].granular.asset==id);
  assert(saveProject("projects/project-4.moss",app.project));
  app.synthBank=2; assert(app.open(3)); assert(app.synthBank==0);
#ifdef _WIN32
  assert(!_chdir(original));
#else
  assert(!chdir(original));
#endif
  puts("PASS granular import: cancel, naming, failed write preserves assignment, retry, automatic assignment, reference protection and save/reopen");
}
int main() {
  for(int i=0;i<3;++i) assert(samples[i].load(lab::sampleFiles[i]));
  codecAndProtection(); playback(); controls(); arrangement(); importWorkflow();
  puts("PASS granular integration: MS11/MOS7/MOS6, references, block invariance, gate, freeze, source replacement, physical controls, 16 locks, lens render, SONG and capture");
}
