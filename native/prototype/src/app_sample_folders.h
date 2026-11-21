// App members: filesystem work stays in service(), with audio paused.
FolderBrowser browser;
int libraryFolder=-1, sliceBank=0, previewSliceIndex=-1, pendingKit=-1;
bool fourSlices=false;
int finePart=-1,fineZoom=0;
unsigned fineCenter=0;
void resetFineView(){finePart=-1;}
void ensureFineView() {
  if(finePart==sliceIndex)return;
  finePart=sliceIndex;fineZoom=0;
  fineCenter=sliceStart(slices[sliceIndex])+sliceFrames(slices[sliceIndex])/2;
}
void fineWindow(unsigned &first,unsigned &last) const {
  const auto &cut=slices[sliceIndex];
  unsigned length=sliceFrames(cut);
  unsigned width=sampleMin(source.frames,sampleMax(16u,(length+length/8+2)>>fineZoom));
  unsigned center=finePart==sliceIndex?fineCenter:sliceStart(cut)+length/2;
  first=center>width/2?sampleMin(center-width/2,source.frames-width):0;
  last=first+width;
}
void pumpWaveform() {
  if(source.frames && !previewing() && !captureBusy() && !buttons.held && nowUs>=draftNextWrite && !__atomic_load_n(&engine->shownPlaying,__ATOMIC_ACQUIRE)){saveSliceDraft();draftNextWrite=nowUs+2000000;}
  if(page!=Tape || tapeTab!=1 || !source.frames || !source.rate ||
     __atomic_load_n(&engine->shownPlaying,__ATOMIC_ACQUIRE))return;
  if(!waveform.matches(source,0,source.frames))waveform.begin(source,0,source.frames);
  if(fourSlices && sliceBank==2) {
    ensureFineView();unsigned first,last;fineWindow(first,last);
    if(!detailWaveform.matches(source,first,last))detailWaveform.begin(source,first,last);
    detailWaveform.pump();
  } else waveform.pump();
}
void editSliceEdge(int edge,int delta,bool frames) {
  auto &cut=slices[sliceIndex];
  unsigned first=sliceStart(cut),last=sliceEnd(cut);
  int64_t amount=frames?delta:int64_t(delta)*source.rate/1000;
  if(edge==0) {
    int64_t moved=sampleMax(int64_t(0),sampleMin(int64_t(first)+amount,int64_t(last)-1));
    cut.setFirst(unsigned(moved),source.rate);
  } else {
    int64_t moved=sampleMax(int64_t(first)+1,sampleMin(int64_t(last)+amount,int64_t(source.frames)));
    cut.setLast(unsigned(moved),source.rate,source.frames);
  }
  limitSlice(cut,edge);
  if(previewing())job=StopPreview;
}
void editFineSlice(int parameter,int direction,int repetitions) {
  ensureFineView();
  if(parameter<2) {
    editSliceEdge(parameter,direction*parameterDelta(repetitions,10),true);
    unsigned first,last;fineWindow(first,last);
    unsigned boundary=parameter? sliceEnd(slices[sliceIndex]):sliceStart(slices[sliceIndex]);
    if(boundary<first || boundary>last)fineCenter=boundary;
  } else if(parameter==2) {
    unsigned length=sliceFrames(slices[sliceIndex]);
    unsigned width=sampleMin(source.frames,length+length/8+2);
    int maximum=0;while(maximum<14 && (width>>(maximum+1))>=16)++maximum;
    fineZoom=clamp(fineZoom+direction*repetitions,0,maximum);
  } else {
    unsigned first,last;fineWindow(first,last);
    int64_t delta=int64_t(sampleMax(1u,(last-first)/100))*direction*parameterDelta(repetitions,10);
    fineCenter=unsigned(sampleMax(int64_t(0),sampleMin(int64_t(source.frames),int64_t(fineCenter)+delta)));
  }
}
int libraryRows(int *ids) const {
  int n=0;
  if(libraryFolder>=0 && libraryFolder<library->kitCount) {
    ids[n++]=-2; for(int id:library->kits[libraryFolder].ids)ids[n++]=id;
  } else {
    ids[n++]=-1;
    for(int i=0;i<library->kitCount;++i)ids[n++]=-10-i;
    for(int i=0;i<CatalogAssets;++i)if(library->has(i)&&library->kitFor(i)<0)ids[n++]=i;
  }
  return n;
}
void libraryMove(int delta) {
  int ids[CatalogAssets+1],row=0,n=libraryRows(ids);
  for(int i=0;i<n;++i)if(ids[i]==asset)row=i;
  asset=ids[clamp(row+delta,0,n-1)];
}
void browseSources() {
  browser.root();fileCursor=0;filePicker=true;
}
void browseBack() {
  if(previewing())job=StopPreview;
  int previous=browser.back();
  if(previous<0)filePicker=false;
  else fileCursor=clamp(previous,0,browser.count?browser.count-1:0);
}
void browseOpen() {
  if(!browser.count)return;
  if(previewing())job=StopPreview;
  if(browser.entries[fileCursor].folder) {
    if(browser.enter(fileCursor))fileCursor=0;
    if(browser.error[0])say(browser.error);
  } else {
    char path[512];if(browser.fullPath(fileCursor,path,sizeof(path)))openSource(path);
    else say("PATH TOO LONG / CHOOSE ANOTHER FILE");
  }
}
bool resetSamples() {
  bool ok=!library || library->prepare(project);
  if(library)engine->init(project,library->views,engine->rate,CatalogAssets);
  else engine->init(project,engine->samples,engine->rate,engine->sampleCount);
  if(!ok)say(library->error);
  return ok;
}
void splitFour() {
  if(!source.frames || durationMs()<4){say("SOURCE TOO SHORT FOR FOUR PARTS");return;}
  if(previewing())job=StopPreview;
  fourSlices=true;sliceCount=4;sliceIndex=sliceBank=0;resetFineView();
  unsigned maximum=SampleBudget/(source.channels*2);
  for(int i=0;i<4;++i) {
    unsigned first=source.frames<=maximum*4?unsigned(uint64_t(source.frames)*i/4):maximum*i;
    unsigned last=source.frames<=maximum*4?unsigned(uint64_t(source.frames)*(i+1)/4):maximum*(i+1);
    slices[i]=Slice{};slices[i].setFirst(first,source.rate);slices[i].setLast(last,source.rate,source.frames);
  }
  say("TAP Y B A X PLAY / HOLD + ARROWS MOVE");
}
void moveSlice(int which,int direction,int repetitions) {
  auto &s=slices[which];unsigned length=sliceFrames(s);
  int64_t start=int64_t(sliceStart(s))+int64_t(direction)*parameterDelta(repetitions,100)*source.rate/1000;
  start=sampleMax(int64_t(0),sampleMin(start,int64_t(source.frames-length)));
  s.setFirst(unsigned(start),source.rate);s.setLast(unsigned(start)+length,source.rate,source.frames);sliceIndex=which;
  if(previewing())job=StopPreview;
}
void arrangeSlices() {
  unsigned lengths[4];uint64_t total=0;
  for(int i=0;i<4;++i){lengths[i]=sliceFrames(slices[i]);total+=lengths[i];}
  if(total>source.frames){say("PARTS TOO LONG / TRIM BEFORE ARRANGING");return;}
  unsigned cursor=sampleMin(sliceStart(slices[0]),source.frames-unsigned(total));
  for(int i=0;i<4;++i){slices[i].setFirst(cursor,source.rate);cursor+=lengths[i];slices[i].setLast(cursor,source.rate,source.frames);}
  if(previewing())job=StopPreview;
  resetFineView();say("PARTS JOINED / LENGTHS KEPT");
}
void auditionSlice(int which) {
  if(captureBusy())return;
  sliceIndex=which;
  bool stop=previewing() && !previewWhole && previewSliceIndex==which;
  job=stop?StopPreview:PreviewSlice;
}
void saveFour() { if(fourSlices && sliceCount==4)nameSlice(true); }
void requestKit(int id) {
  if(track>=project.trackCount || project.tracks[track].instrument!=SampleInstrument) {
    say("LOAD FOUR SLOTS FROM A SAMPLER TRACK");return;
  }
  if(!captureBusy()){
    pendingKit=id;bool occupied=false;for(const auto &slot:project.tracks[track].keys)occupied|=slot.asset>=0;
    if(occupied){confirmDelete=true;confirmTarget=9;}else job=LoadKit;
  }
}
void loadKit() {
  if(pendingKit<0 || pendingKit>=library->kitCount)return;
  Project candidate=project;
  for(int i=0;i<4;++i) {
    candidate.tracks[track].keys[i]=Key{};
    candidate.tracks[track].keys[i].asset=library->kits[pendingKit].ids[i];
  }
  if(!library->prepare(candidate)) {say(library->error);resetSamples();return;}
  checkpoint();project=candidate;key=0;
  importAssign=assignMode=false;page=Sampler;reload=false;
  resetSamples();say("FOUR SLOTS LOADED / OPTIONS UNDO");
}

int selectedKit() const {return !library?-1:asset<=-10?-10-asset:libraryFolder>=0?libraryFolder:library->kitFor(asset);}
bool selectedKitProtected() const {int id=selectedKit();if(id<0||id>=library->kitCount)return true;for(int n:library->kits[id].ids)if(assetProtected(n))return true;return false;}
bool renameSelectedKit(const char *name){
  int id=selectedKit();if(id<0||!Library::safeKitName(name)){say("USE LETTERS, NUMBERS, SPACE, - OR _");return false;}
  char oldPath[128],newPath[128];snprintf(oldPath,sizeof(oldPath),"library/%s",library->kits[id].name);snprintf(newPath,sizeof(newPath),"library/%s",name);
  if(Library::entryExists(newPath)||rename(oldPath,newPath)){say("FOLDER NOT RENAMED / CHECK NAME AND CARD");return false;}
  snprintf(library->kits[id].name,25,"%s",name);
  for(int i=0;i<4;++i){int assetId=library->kits[id].ids[i];snprintf(library->assetFiles[assetId],512,"library/%s/%02d.wav",name,i+1);snprintf(library->names[assetId],32,"%.24s %02d",name,i+1);}
  say("KIT FOLDER RENAMED");return true;
}
void deleteSelectedKit(){
  if(selectedKitProtected()){say("KIT IN USE / FILES KEPT");return;}
  int id=selectedKit();char dir[128],hidden[128];snprintf(dir,sizeof(dir),"library/%s",library->kits[id].name);
  snprintf(hidden,sizeof(hidden),"library/.removed-kit-%d",library->kits[id].ids[0]);
  if(Library::entryExists(hidden)||rename(dir,hidden)){say("KIT NOT DELETED / CHECK CARD");return;}
  // The rename commits removal from the catalog. Only five owned files follow.
  for(int i=0;i<4;++i){int n=library->kits[id].ids[i];char file[160];snprintf(file,sizeof(file),"%s/%02d.wav",hidden,i+1);remove(file);library->names[n][0]=0;library->assetFiles[n][0]=0;library->catalogBytes[n]=0;}
  char file[160];snprintf(file,sizeof(file),"%s/set.moss",hidden);remove(file);
#ifdef _WIN32
  _rmdir(hidden);
#else
  rmdir(hidden);
#endif
  for(int k=id;k<library->kitCount-1;++k)library->kits[k]=library->kits[k+1];--library->kitCount;libraryFolder=-1;asset=-1;say("UNUSED KIT DELETED");
}
