// Library members. A kit becomes visible only after its directory is renamed.
static bool entryExists(const char *path) { struct stat st={};return stat(path,&st)==0; }
int kitFor(int id) const {
  for(int k=0;k<kitCount;++k) for(int n:kits[k].ids) if(n==id)return k;
  return -1;
}
static bool safeKitName(const char *name) {
  size_t n=strlen(name);
  if(!n || n>24 || name[0]==' ' || name[n-1]==' ')return false;
  for(const char *p=name;*p;++p)
    if(!((*p>='A'&&*p<='Z')||(*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p==' '||*p=='-'||*p=='_'))return false;
  // DOS device names also fail safely at mkdir; don't create unusable folders.
  return true;
}
void registerKit(const char *name, const int *ids) {
  auto &kit=kits[kitCount++];snprintf(kit.name,sizeof(kit.name),"%s",name);
  for(int i=0;i<4;++i) {
    int id=ids[i];kit.ids[i]=id;
    snprintf(assetFiles[id],512,"library/%s/%02d.wav",name,i+1);
    snprintf(names[id],32,"%.24s %02d",name,i+1);
    WavSource info;if(info.open(assetFiles[id])) catalogBytes[id]=info.frames*info.channels*2;
  }
}
void scanKits() {
  FolderBrowser folders;strcpy(folders.path,"library");folders.scan();
  for(int i=0;i<folders.count && kitCount<CatalogAssets/4;++i) {
    const auto &entry=folders.entries[i];if(!entry.folder || !safeKitName(entry.name))continue;
    char path[512];snprintf(path,sizeof(path),"library/%s/set.moss",entry.name);
    FILE *f=fopen(path,"rb");if(!f)continue;
    char header[24]={};int ids[4]={};
    bool ok=fgets(header,sizeof(header),f) && !strcmp(header,"MOSS-KIT-1\n") &&
      fscanf(f,"%d %d %d %d",ids,ids+1,ids+2,ids+3)==4;
    fclose(f);
    for(int j=0;j<4;++j) {
      if(ids[j]<0 || ids[j]>=CatalogAssets || has(ids[j]))ok=false;
      for(int k=0;k<j;++k)if(ids[j]==ids[k])ok=false;
    }
    if(ok)registerKit(entry.name,ids);
  }
}
template<class Slice> int saveKit(WavSource &source,const Slice *cuts,const char *name) {
  if(!safeKitName(name)){strcpy(error,"USE LETTERS, NUMBERS, SPACE, - OR _");return -1;}
  char dest[128],stage[128];
  snprintf(dest,sizeof(dest),"library/%s",name);
  if(entryExists(dest)){strcpy(error,"FOLDER EXISTS / CHOOSE ANOTHER NAME");return -1;}
  if(kitCount==CatalogAssets/4){strcpy(error,"KIT CATALOG FULL");return -1;}
  int ids[4],found=0;
  for(int id=0;id<CatalogAssets && found<4;++id)if(!has(id))ids[found++]=id;
  if(found<4){strcpy(error,"NEED FOUR FREE CATALOG ENTRIES");return -1;}
  unsigned starts[4],ends[4];
  for(int i=0;i<4;++i) {
    starts[i]=cuts[i].first(source.rate);
    ends[i]=cuts[i].last(source.rate,source.frames);
    if(starts[i]>=ends[i] || ends[i]>source.frames || uint64_t(ends[i]-starts[i])*source.channels*2>SampleBudget){
      strcpy(error,"TRIM EACH PART TO 1 MIB OR LESS");return -1;
    }
  }
  int attempt=0;
  do {snprintf(stage,sizeof(stage),"library/.pending-kit-%04d",attempt++);}while(entryExists(stage)&&attempt<10000);
  if(entryExists(stage)){strcpy(error,"CANNOT CREATE KIT / CHECK CARD");return -1;}
  makeDir(stage);
  bool ok=entryExists(stage);
  char file[180];
  for(int i=0;i<4 && ok;++i) {
    snprintf(file,sizeof(file),"%s/%02d.wav",stage,i+1);
    ok=source.extract(scratch,starts[i],ends[i],cuts[i].fadeIn,cuts[i].fadeOut) &&
      writeWav(file,scratch.pcm,scratch.frames,scratch.rate,scratch.channels);
  }
  if(ok) {
    snprintf(file,sizeof(file),"%s/set.moss",stage);FILE *f=fopen(file,"wb");ok=f!=nullptr;
    if(f){
      ok=fprintf(f,"MOSS-KIT-1\n%d %d %d %d\n",ids[0],ids[1],ids[2],ids[3])>0;
      if(fflush(f))ok=false;
#ifdef _WIN32
      if(_commit(_fileno(f)))ok=false;
#else
      if(fsync(fileno(f)))ok=false;
#endif
      if(fclose(f))ok=false;
    }
  }
  if(ok)ok=rename(stage,dest)==0;
  if(!ok) {
    // Cleanup only the five known files in our newly created staging folder.
    for(int i=1;i<=4;++i){snprintf(file,sizeof(file),"%s/%02d.wav",stage,i);remove(file);strcat(file,".tmp");remove(file);}
    snprintf(file,sizeof(file),"%s/set.moss",stage);remove(file);
#ifdef _WIN32
    _rmdir(stage);
#else
    rmdir(stage);
#endif
    strcpy(error,"KIT NOT SAVED / CHECK CARD AND RETRY");return -1;
  }
  int result=kitCount;registerKit(name,ids);return result;
}
