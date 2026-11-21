// Small, validated edit metadata. PCM is never copied into a draft.
struct SliceDraft {
  uint32_t version=1,rate=0,frames=0,channels=0;
  char sourcePath[512]={},label[25]={};
  int count=0,index=0,bank=0,four=0;
  Slice cuts[16];
};
uint32_t draftHash=0;
uint64_t draftNextWrite=0;
static uint32_t draftChecksum(const void *data,size_t size){uint32_t h=2166136261u;const auto *p=static_cast<const unsigned char*>(data);while(size--){h^=*p++;h*=16777619u;}return h;}
static bool draftWrite(const char *path,const void *data,size_t size){
  char temp[128];snprintf(temp,sizeof(temp),"%s.tmp",path);FILE *f=fopen(temp,"wb");if(!f)return false;
  bool ok=fwrite(data,1,size,f)==size;if(fflush(f))ok=false;
#ifdef _WIN32
  if(_commit(_fileno(f)))ok=false;
#else
  if(fsync(fileno(f)))ok=false;
#endif
  if(fclose(f))ok=false;if(!ok)return false;
#ifdef _WIN32
  return MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
  return rename(temp,path)==0;
#endif
}
void draftPath(char *path,size_t size) const {snprintf(path,size,"projects/drafts/%08x.bin",draftChecksum(source.path,strlen(source.path)));}
bool saveSliceDraft(){
  if(!source.frames||!sliceCount)return true;
  SliceDraft d;memset(&d,0,sizeof(d));d.version=1;d.rate=source.rate;d.frames=source.frames;d.channels=source.channels;snprintf(d.sourcePath,sizeof(d.sourcePath),"%s",source.path);snprintf(d.label,sizeof(d.label),"%s",saveLabel);
  d.count=sliceCount;d.index=sliceIndex;d.bank=sliceBank;d.four=fourSlices;memcpy(d.cuts,slices,sizeof(slices));
  uint32_t hash=draftChecksum(&d,sizeof(d));if(hash==draftHash)return true;
  makeDir("projects/drafts");char file[128];draftPath(file,sizeof(file));
  if(!draftWrite(file,&d,sizeof(d))||!draftWrite("projects/drafts/last.txt",source.path,strlen(source.path)+1)){say("DRAFT NOT SAVED / CHECK CARD");return false;}
  draftHash=hash;return true;
}
bool restoreSliceDraft(){
  char file[128];draftPath(file,sizeof(file));FILE *f=fopen(file,"rb");if(!f)return false;
  SliceDraft d;bool ok=fread(&d,1,sizeof(d),f)==sizeof(d);fclose(f);
  if(!ok||d.version!=1||!memchr(d.sourcePath,0,sizeof(d.sourcePath))||!memchr(d.label,0,sizeof(d.label))||strcmp(d.sourcePath,source.path)||d.rate!=source.rate||d.frames!=source.frames||d.channels!=source.channels||d.count<1||d.count>16||d.index<0||d.index>=d.count||d.bank<0||d.bank>2||(d.four&&d.count!=4))return false;
  for(int i=0;i<d.count;++i){const auto &s=d.cuts[i];unsigned first=s.first(d.rate),last=s.last(d.rate,d.frames);if(s.start<0||s.end<0||first>=last||last>d.frames||s.fadeIn<0||s.fadeOut<0||s.fadeIn>5000||s.fadeOut>5000||uint64_t(last-first)*d.channels*2>SampleBudget)return false;}
  memcpy(slices,d.cuts,sizeof(slices));sliceCount=d.count;sliceIndex=d.index;sliceBank=d.bank;fourSlices=d.four!=0;snprintf(saveLabel,sizeof(saveLabel),"%s",d.label);draftHash=0;resetFineView();return true;
}
void restoreLastDraft(){
  FILE *f=fopen("projects/drafts/last.txt","rb");if(!f)return;char path[512]={};size_t n=fread(path,1,sizeof(path)-1,f);fclose(f);if(!n)return;
  WavSource next;if(!next.open(path)){say("TAPE SOURCE MISSING / IMPORT TO CONTINUE");return;}source=next;source.overview();
  if(!restoreSliceDraft()){source=WavSource{};sliceCount=0;}
}
