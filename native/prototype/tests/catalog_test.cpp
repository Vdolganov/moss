#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Library library;
static App app;
static Engine engine;
static int16_t audio[2048];
int main(int argc,char **argv) {
  // Read-only source tree; caller must provide an isolated working directory
  // for the single saved test slice. Never run in the user's app directory.
  if(argc!=3) return 2;
  library.scanFolder(argv[1]);
  assert(library.sourceCount==atoi(argv[2]) && library.sourceCount>128);
  int files=library.sourceCount,large=-1;
  for(int i=0;i<files;++i) {
    WavSource source;
    if(!source.open(library.sourcePaths[i])) {
      fprintf(stderr,"FAIL WAV: %s: %s\n",library.sourcePaths[i],source.error);
      return 3;
    }
    if(source.frames*source.channels*2>SampleBudget) large=i;
  }
  assert(large>=0);
  printf("PASS catalog: %d real WAV headers, recursive categories, files beyond index 128\n",files);
  app.library=&library;app.engine=&engine;app.page=Tape;app.filePicker=true;
  app.fileCursor=126;app.jumpPage(1);assert(app.fileCursor==133);
  app.jumpPage(-1);assert(app.fileCursor==126);
  app.fileCursor=files-2;app.jumpPage(1);assert(app.fileCursor==files-1);
  app.fileCursor=0;app.jumpPage(-1);assert(app.fileCursor==0);
  puts("PASS catalog paging: seven visible rows, boundary 128, partial last page");
  app.fileCursor=large;engine.init(app.project,library.pcm,AudioRate,Assets);
  app.job=App::PreviewFile;app.service();assert(app.previewing());
  uint64_t energy=0;
  for(unsigned i=0;i<AudioRate*3/1024;++i) {
    app.pumpPreview();engine.render(audio,1024);
    for(int v:audio) energy+=uint64_t(int64_t(v)*v);
  }
  assert(energy>1000000 && !engine.stream.underruns);
  app.job=App::StopPreview;app.service();
  app.openSource(library.sourcePaths[large]);app.service();
  assert(app.page==Tape && app.tapeTab==1 && !app.filePicker);
  assert(app.sliceFrames(app.slices[0])*app.source.channels*2<=SampleBudget);
  makeDir("library");
  auto &s=app.slices[0];
  int asset=library.saveSlice(app.source,app.sliceStart(s),app.sliceEnd(s),0,0,"CATALOG TEST");
  assert(asset>=0 && library.pcm[asset].frames && library.pcm[asset].bytes<=SampleBudget);
  puts("PASS large WAV preview and bounded named slice import");
  library.sourceCount=0;
  for(int i=0;i<Sources+10;++i) {
    char path[64];snprintf(path,sizeof(path),"imports/source-%04d.wav",i);library.addSource(path);
  }
  assert(library.sourceCount==Sources);
  library.rememberSource("imports/last-source.wav");
  assert(library.sourceCount==Sources && !strcmp(library.sourcePaths[0],"imports/last-source.wav"));
  library.rememberSource("imports/last-source.wav");assert(library.sourceCount==Sources);
  printf("PASS bounded path catalog: %d entries, unchanged 32-slot PCM bank\n",Sources);
}
