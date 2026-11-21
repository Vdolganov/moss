// Included after App: waveform caches are populated outside the renderer.
inline uint32_t sliceTint(uint32_t color,unsigned opacity) {
  uint32_t result=0;
  for(int shift=0;shift<24;shift+=8) {
    unsigned base=(lab::BG>>shift)&255,ink=(color>>shift)&255;
    result|=((base*(255-opacity)+ink*opacity)/255)<<shift;
  }
  return result;
}
inline int slicePixel(unsigned frame,unsigned first,unsigned last) {
  if(frame<=first)return 18;
  if(frame>=last)return 622;
  return 18+int(uint64_t(frame-first)*604/(last-first));
}
inline void drawWaveform(lab::Canvas &c,const SampleWaveform &wave,int top,int height) {
  using namespace lab;
  if(!wave.source.channels)return;
  int band=height/int(wave.source.channels);
  for(unsigned channel=0;channel<wave.source.channels;++channel) {
    int center=top+int(channel)*band+band/2,scale=band/2-3;
    c.rect(18,center,604,1,LINE);
    for(unsigned x=0;x<SampleWaveform::Columns;++x)if(wave.seen[x]) {
      int upper=center-int(wave.high[channel][x])*scale/32768;
      int lower=center-int(wave.low[channel][x])*scale/32768;
      c.rect(18+int(x),upper,1,lower-upper+1,TEXT);
    }
  }
}
inline void drawFourSlices(lab::Canvas &c,const App &a) {
  using namespace lab;
  char b[128];text(c,18,149,leafName(a.source.path),TEXT,2,49);
  const auto &selected=a.slices[a.sliceIndex];
  snprintf(b,sizeof(b),"PART %d / %u KIB",a.sliceIndex+1,(a.sliceFrames(selected)*a.source.channels*2+1023)/1024);
  text(c,18,177,b,TEXT);
  text(c,490,177,"L1/R1 TABS",DIM);
  unsigned first=0,last=a.source.frames;
  if(a.sliceBank==2)a.fineWindow(first,last);
  if(last<=first)return;
  constexpr int top=204,height=106;
  // Unselected ranges first, selected last: no blended ambiguity at overlaps.
  for(int pass=0;pass<5;++pass) {
    int part=pass<4?pass:a.sliceIndex;
    if(pass<4 && part==a.sliceIndex)continue;
    unsigned begin=a.sliceStart(a.slices[part]),end=a.sliceEnd(a.slices[part]);
    if(end<=first || begin>=last)continue;
    int x=slicePixel(begin,first,last),right=slicePixel(end,first,last);
    uint32_t color=TEXT;
    c.rect(x,top,sampleMax(1,right-x),height,sliceTint(color,part==a.sliceIndex?84:32));
    c.rect(x,top,sampleMax(1,right-x),1,color);
    if(part==a.sliceIndex)c.box(x,top,sampleMax(1,right-x),height,color);
  }
  const auto &wave=a.sliceBank==2?a.detailWaveform:a.waveform;
  if(wave.matches(a.source,first,last))drawWaveform(c,wave,224,82);
  // Labels remain physical-button labels even when the parts overlap or move.
  int labelEnd=0;
  for(int i=0;i<4;++i) {
    unsigned begin=a.sliceStart(a.slices[i]),end=a.sliceEnd(a.slices[i]);
    if(end<=first || begin>=last)continue;
    int x=slicePixel(begin,first,last)+4;
    if(x<labelEnd || x>590)continue;
    snprintf(b,sizeof(b),"%s%d",names[ParameterButtons[i]],i+1);
    text(c,x,208,b,i==a.sliceIndex?TEXT:DIM);labelEnd=x+30;
  }
  if(a.previewing() && !a.previewWhole) {
    unsigned position=__atomic_load_n(&a.engine->stream.shownFrame,__ATOMIC_ACQUIRE);
    if(position>=first && position<last){int x=slicePixel(position,first,last);c.rect(x,222,1,88,TEXT);c.rect(x-2,307,5,3,TEXT);}
  }
  if(wave.failed)text(c,18,324,"WAVE UNAVAILABLE / REOPEN SOURCE",DIM);
  else if(!wave.matches(a.source,first,last) || wave.busy())text(c,18,324,"LOADING WAVEFORM",DIM);
  else {
    snprintf(b,sizeof(b),"%u MS",unsigned(uint64_t(first)*1000/a.source.rate));text(c,18,324,b,DIM);
    snprintf(b,sizeof(b),"%u MS",unsigned(uint64_t(last)*1000/a.source.rate));text(c,622-int(strlen(b))*12,324,b,DIM);
  }
}
inline void drawSampleLibrary(lab::Canvas &c,const App &a) {
  using namespace lab;char b[128];int ids[CatalogAssets+1],row=0,n=a.libraryRows(ids);
  for(int i=0;i<n;++i)if(ids[i]==a.asset)row=i;
  snprintf(b,sizeof(b),"LIBRARY / %s",a.libraryFolder>=0?a.library->kits[a.libraryFolder].name:"SAMPLES + FOLDERS");
  text(c,18,149,b,DIM,2,49);
  int first=row/5*5;
  for(int i=first;i<n && i<first+5;++i) {
    int id=ids[i],y=185+(i-first)*41;
    if(i==row)c.rect(18,y-8,604,37,PANEL);
    if(id==-1)strcpy(b,"+ IMPORT FROM DEVICE");
    else if(id==-2)strcpy(b,".. LIBRARY");
    else if(id<=-10)snprintf(b,sizeof(b),"%s /",a.library->kits[-10-id].name);
    else snprintf(b,sizeof(b),"%s",a.library->names[id]);
    text(c,28,y,b,TEXT,2,37);
    if(id<=-10)text(c,534,y,"4 WAV",DIM);
    else if(id>=0){snprintf(b,sizeof(b),"%u K",(a.library->assetBytes(id)+1023)/1024);text(c,534,y,b,DIM);}
    c.rect(18,y+28,604,1,LINE);
  }
  snprintf(b,sizeof(b),"PAGE %d/%d / R1+LEFT-RIGHT",row/5+1,(n+4)/5);
  text(c,18,391,b,DIM);
  if(a.asset<=-10 || a.libraryFolder>=0) {
    if(a.track<a.project.trackCount && a.project.tracks[a.track].instrument==SampleInstrument)
      snprintf(b,sizeof(b),"Y REPLACE 4 > %02d / %.24s",a.track+1,a.project.tracks[a.track].name);
    else strcpy(b,"LOAD SET: OPEN A SAMPLER TRACK FIRST");
    text(c,18,419,b,DIM,2,50);
  } else text(c,18,419,a.assignMode?"A ASSIGN / X PLAY / B VOICE":"A EDIT COPY / X PLAY / L1+R1 OPTIONS",DIM,2,50);
}
inline void drawFolderPicker(lab::Canvas &c,const App &a){
  using namespace lab;char b[128];const auto &f=a.browser;ui::box(c,16,16,608,448,0,LINE,BG);
  ui::text(c,32,32,"IMPORT SAMPLE",TEXT,ui::Heading);
  snprintf(b,sizeof(b),"%d / %d",a.fileCursor+1,f.count);ui::text(c,608-ui::textWidth(b,ui::Small),38,b,DIM,ui::Small);
  const char *path=f.path[0]?f.path:"DEVICE";int length=int(strlen(path));snprintf(b,sizeof(b),"%s%s",length>55?"...":"",path+(length>55?length-52:0));ui::text(c,32,76,b,DIM,ui::Small,55);
  if(!f.count)ui::text(c,32,216,f.error[0]?f.error:"FOLDER EMPTY / B BACK",DIM,ui::Small,55);
  int first=a.fileCursor/SourcePageRows*SourcePageRows;
  for(int i=first;i<f.count&&i<first+SourcePageRows;++i){int y=104+(i-first)*44;ui::row(c,y,i==a.fileCursor,40);snprintf(b,sizeof(b),"%s%s",f.entries[i].folder?"/ ":"",f.entries[i].name);ui::text(c,32,y+13,b,i==a.fileCursor?TEXT:DIM,ui::Body,47);}
  ui::text(c,32,428,a.previewing()?"X STOP / A OPEN / B UP":"A OPEN / X PLAY / B UP / R1+L-R PAGE",DIM,ui::Small,55);
}
