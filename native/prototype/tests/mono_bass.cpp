// Compile with -I src. To reproduce the baseline, point -I at 4e7def3/src
// in a separate checkout and define MOSS_LEGACY_MONO. No project is opened.
#include <synth.h>
#include <stdio.h>
using namespace moss;
static SynthTables tables;
static SynthVoice voice;
int main(int argc,char **argv){
  if(argc!=2)return 1;
  tables.init();
  for(int variant=0;variant<6;++variant){
    int p[20]={1200,variant%2?85:0,3,600,0,0,0,20,50,0,0,0,500,100,500,0,30,0,0,0};
    int note=variant<2?28:variant<4?33:40;
    char path[1024];snprintf(path,sizeof(path),"%s/bass-%d.f32",argv[1],variant);
    FILE *f=fopen(path,"wb");if(!f)return 2;
    voice=SynthVoice{};
#ifdef MOSS_LEGACY_MONO
    voice.on(MonoInstrument,note,tables.frequency[note],100,64,p,0,32000,13);
#else
    voice.on(MonoInstrument,note,tables.frequency[note],100,64,p,0,32000,13,20);
#endif
    for(int i=0;i<96000;++i){int l=0,r=0;voice.render(l,r,tables);float x=l/32768.f;fwrite(&x,4,1,f);}
    if(fclose(f))return 3;
  }
  puts("Bass reference: E1/A1/E2, resonance 0/85, identical dry controls");
}
