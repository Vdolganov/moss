#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
using namespace moss;
static Library library, reopened;
static Engine engine;
static App app;
static int16_t wave[32000*2*4], audio[2048];
static uint64_t now=1000000;
static void tap(int key){app.button(key,true,now);app.button(key,false,now+10000);now+=50000;}
static void service(){if(app.job!=App::NoJob)app.service();}
static void snapshot(const char *path,bool keepStatus=false) {
  if(!keepStatus)app.status[0]=0;
  for(int i=0;i<200;++i)app.pumpPreview();
  static lab::Canvas c;draw(c,app);FILE *f=fopen(path,"wb");assert(f);
  fwrite("BM",1,2,f);put32(f,54+640*480*3);put32(f,0);put32(f,54);put32(f,40);
  put32(f,640);put32(f,480);put16(f,1);put16(f,24);put32(f,0);put32(f,640*480*3);
  for(int i=0;i<4;++i)put32(f,0);
  for(int y=479;y>=0;--y)for(int x=0;x<640;++x){unsigned p=c.pixels[y*640+x];fputc(p&255,f);fputc((p>>8)&255,f);fputc((p>>16)&255,f);}
  fclose(f);
}
int main(int argc,char **argv) {
  assert(argc==2);
#ifdef _WIN32
  assert(SetCurrentDirectoryA(argv[1]));
#else
  assert(chdir(argv[1])==0);
#endif
  makeDir("imports");makeDir("projects");makeDir("imports/DRUMS");makeDir("imports/DRUMS/BREAKS");
  for(unsigned i=0;i<sizeof(wave)/sizeof(wave[0])/2;++i) {
    double envelope=.15+.65*pow(sin(i*.00021),2);
    wave[i*2]=int16_t(sin(i*.017)*envelope*26000);
    wave[i*2+1]=int16_t(sin(i*.021+.7)*envelope*23000);
  }
  assert(writeWav("imports/DRUMS/BREAKS/FOUR.wav",wave,32000*4,32000,2));
  library.init();app.library=&library;app.engine=&engine;
  app.project=Project{};app.project.trackCount=1;app.track=0;
  app.resetSamples();app.page=Tape;app.tapeTab=0;app.browseSources();
  assert(app.browser.count==3);app.fileCursor=0;app.browseOpen();assert(!strcmp(app.browser.path,"imports"));
  app.browseOpen();assert(!strcmp(app.browser.path,"imports/DRUMS"));app.browseOpen();
  assert(!strcmp(app.browser.path,"imports/DRUMS/BREAKS"));snapshot("folders.bmp");
  app.browseBack();assert(!strcmp(app.browser.path,"imports/DRUMS") && app.fileCursor==0);app.browseOpen();
  app.browseOpen();service();assert(app.sliceCount==1 && !app.filePicker);
  app.splitFour();assert(app.fourSlices && app.sliceCount==4);
  for(int i=0;i<4;++i){assert(app.slices[i].start==i*1000);assert(app.slices[i].end==(i+1)*1000);}
  // Each face auditions its own range; switching starts the new range immediately.
  for(int i=0;i<4;++i){tap(ParameterButtons[i]);service();assert(app.previewing());assert(app.previewSliceIndex==i);
    assert(engine.stream.first==unsigned(i*32000));
    engine.render(audio,1024);app.pumpPreview();
  }
  tap(lab::X);service();assert(!app.previewing());
  auto before=app.slices[1];
  app.button(lab::B,true,now);app.button(lab::Right,true,now+1000);app.button(lab::Right,false,now+2000);app.button(lab::B,false,now+3000);now+=50000;
  assert(app.slices[1].start==before.start+1 && app.slices[1].end==before.end+1);assert(app.job==App::NoJob);
  app.button(lab::B,true,now);app.button(lab::Up,true,now+1000);app.button(lab::Up,false,now+2000);app.button(lab::B,false,now+3000);now+=50000;
  assert(abs(app.slices[1].start-before.start)>=99); // coarse is 100 ms
  app.clearInput();app.splitFour();snapshot("four-parts.bmp");
  tap(lab::R1);assert(app.sliceBank==1 && app.tapeTab==1);
  app.button(lab::R1,true,now);
  for(int i=1;i<=20;++i){app.tick(now+i*100000);assert(app.sliceBank==1);}
  app.button(lab::R1,false,now+2110000);now+=2200000;
  assert(app.sliceBank==1 && app.tapeTab==1);app.clearInput();tap(lab::R1);assert(app.sliceBank==2);
  tap(lab::L1);tap(lab::L1);assert(app.sliceBank==0 && app.tapeTab==1);
  app.importAssign=true;app.importTrack=0;app.importKey=2;
  tap(lab::B);service();assert(app.page==Tape && app.previewSliceIndex==1);
  assert(strstr(app.status,"B STOP"));
  {static lab::Canvas c;draw(c,app);}
  snapshot("playing-part.bmp",true);
  tap(lab::Select);service();assert(app.page==Sampler && !app.importAssign);
  app.page=Tape;app.tapeTab=1;
  app.sliceBank=1;app.sliceIndex=2;snapshot("trim-part.bmp");app.sliceBank=0;
  app.parameterPage(1);app.parameterPage(1);assert(app.sliceBank==2);
  unsigned exactStart=app.sliceStart(app.slices[2]);
  app.edit(0,1,1);assert(app.sliceStart(app.slices[2])==exactStart+1);
  app.buttons.held=lab::bit(lab::Up);app.edit(1,-1,1);app.buttons.held=0;
  assert(app.sliceEnd(app.slices[2])==96000-10);
  app.edit(2,1,4);assert(app.fineZoom==4);snapshot("fine-wave.bmp");
  app.edit(2,1,100);assert(app.fineZoom<14);snapshot("fine-samples.bmp");
  app.sliceBank=0;app.splitFour();
  app.slices[1].setFirst(12000,app.source.rate);app.slices[1].setLast(44000,app.source.rate,app.source.frames);
  snapshot("overlapping-parts.bmp");
  app.option=app.optionIndex(11);assert(app.option>=0);app.chooseOption();
  for(int i=1;i<4;++i)assert(app.sliceStart(app.slices[i])==app.sliceEnd(app.slices[i-1]));
  // Sub-millisecond offsets remain exact at non-integer kHz rates and large positions.
  {SampleSlice precise;unsigned frame=16777219,rate=44100;
   precise.setFirst(frame,rate);precise.setLast(frame+5,rate,frame+100);
   assert(precise.first(rate)==frame && precise.last(rate,frame+100)==frame+5);}
  // Exact one-frame edits survive the four-file export and fresh library read.
  app.sliceIndex=2;app.sliceBank=2;app.edit(0,1,1);app.sliceBank=0;
  app.saveFour();assert(app.naming && app.nameTarget==5);snapshot("folder-name.bmp");
  strcpy(app.nameEdit,"DUST CUTS");app.action(lab::Start);service();
  if(library.kitCount!=1)printf("SAVE STATUS: %s / %s\n",app.status,library.error);
  assert(library.kitCount==1 && exists("library/DUST CUTS/04.wav"));
  assert(!strcmp(library.kits[0].name,"DUST CUTS"));assert(app.tapeTab==2);snapshot("kit-folder.bmp");
  app.project.tracks[0].instrument=OrganismInstrument;snapshot("kit-ineligible.bmp");app.project.tracks[0].instrument=SampleInstrument;
  assert(library.saveKit(app.source,app.slices,"DUST CUTS")==-1);
  assert(library.saveKit(app.source,app.slices,"../ESCAPE")==-1);
  reopened.init();assert(reopened.kitCount==1 && reopened.count()==4);
  {WavSource prepared;assert(prepared.open("library/DUST CUTS/03.wav"));assert(prepared.frames==31999);
   assert(prepared.extract(library.scratch,0,prepared.frames,0,0));
   assert(!memcmp(library.scratch.pcm,wave+64001*2,prepared.frames*4));}
  app.requestKit(0);service();assert(app.page==Sampler);
  for(int i=0;i<4;++i)assert(app.project.tracks[0].keys[i].asset==library.kits[0].ids[i]);
  assert(app.haveUndo && app.undoProject.tracks[0].keys[0].asset==-1);
  snapshot("assigned.bmp");
  for(int i=0;i<4;++i){engine.handle({Op::Audition,0,i});engine.render(audio,1024);assert(engine.voices[0].active||engine.voice(0,i).active);}
  // Fill beyond the old 32-file ceiling, without adding PCM capacity.
  for(int i=4;i<45;++i)assert(library.saveSlice(app.source,0,3200,0,0,"ONE SHOT")==i);
  assert(library.count()==45);assert(sizeof(library.pcm)==sizeof(SampleBuffer)*ResidentAssets);
  app.project.tracks[0].keys[3].asset=44;assert(app.resetSamples());
  assert(engine.samples[44].frames==3200 && engine.samples[0].frames==32000);
  assert(app.save());Project read;assert(loadProject("projects/project-1.moss",read));assert(read.tracks[0].keys[3].asset==44);
  assert(app.assetProtected(44));
  int extraKit=library.saveKit(app.source,app.slices,"BLOOM");assert(extraKit==1);
  reopened.init();assert(reopened.kitCount==2 && reopened.kits[0].ids[0]==45 && reopened.kits[1].ids[0]==0);
  assert(reopened.prepare(read) && reopened.views[44].frames==3200);
  app.page=Tape;app.tapeTab=2;app.libraryFolder=-1;app.asset=-1;
  app.jumpPage(1);int pageAsset=app.asset;assert(pageAsset>=0);app.jumpPage(-1);assert(app.asset==-1);app.jumpPage(1);assert(app.asset==pageAsset);snapshot("library-page.bmp");
  // A failed four-slot load must not mutate assignments.
  auto original=app.project.tracks[0].keys[0].asset;
  assert(rename("library/DUST CUTS/01.wav","library/DUST CUTS/01.hidden")==0);
  for(int &id:library.resident)id=-1;library.syncViews();
  app.requestKit(0);assert(app.confirmDelete);app.action(lab::A);service();assert(app.project.tracks[0].keys[0].asset==original);
  assert(rename("library/DUST CUTS/01.hidden","library/DUST CUTS/01.wav")==0);
  // Oversize equal pieces stay within 1 MiB; moving preserves exact duration.
  app.source.frames=32000*120;app.splitFour();
  for(int i=0;i<4;++i)assert(uint64_t(app.sliceFrames(app.slices[i]))*4<=SampleBudget);
  for(int i=1;i<4;++i)assert(app.sliceStart(app.slices[i])==app.sliceEnd(app.slices[i-1]));
  app.moveSlice(3,1,1000000);assert(app.slices[3].end==app.durationMs());
  // Reject missing source atomically, leaving no published folder.
  app.source.frames=128000;app.splitFour();strcpy(app.source.path,"imports/MISSING.wav");
  int count=library.count();assert(library.saveKit(app.source,app.slices,"FAIL")==-1);
  assert(!exists("library/FAIL") && library.count()==count);
  // Arrangement never silently shortens an overfull set.
  for(auto &s:app.slices){s={0,4000,0,0};}
  auto unchanged=app.slices[0];app.arrangeSlices();
  assert(strstr(app.status,"TOO LONG") && app.slices[0].end==unchanged.end);
  // Overview uses every decoded sample, including transients between old probe windows.
  WavSource real;assert(real.open("imports/DRUMS/BREAKS/FOUR.wav"));
  static SampleWaveform detailed;detailed.begin(real,0,real.frames);
  while(detailed.busy())detailed.pump();assert(!detailed.failed);
  for(unsigned x=0;x<SampleWaveform::Columns;++x)for(int ch=0;ch<2;++ch){
    int lo=32767,hi=-32768;
    unsigned begin=unsigned((uint64_t(x)*real.frames+SampleWaveform::Columns-1)/SampleWaveform::Columns);
    unsigned end=unsigned((uint64_t(x+1)*real.frames+SampleWaveform::Columns-1)/SampleWaveform::Columns);
    for(unsigned i=begin;i<end;++i){lo=sampleMin(lo,int(wave[i*2+ch]));hi=sampleMax(hi,int(wave[i*2+ch]));}
    assert(detailed.low[ch][x]==lo && detailed.high[ch][x]==hi);
  }
  detailed.begin(real,1000,1004);while(detailed.busy())detailed.pump();
  assert(detailed.low[0][0]==wave[2000] && detailed.high[1][603]==wave[2007]);
  app.page=Tape;app.tapeTab=0;app.browseSources();app.fileCursor=1;app.browseOpen();snapshot("empty-folder.bmp");
  AssetMask all;for(int i=0;i<33;++i)all.add(i);assert(!library.prepare(all));
  puts("PASS folders, paging, four auditions, fine/coarse gestures, atomic kit save/load, catalog >32, persistence and 32 MiB cache bound");
}
