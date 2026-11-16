#include "../src/synth.h"
#include <stdio.h>
using namespace moss;
static SynthTables tables;static SynthVoice voice;
int main(int argc,char **argv){if(argc!=2)return 1;tables.init();char path[1024];
  for(int preset=0;preset<4;++preset){
    snprintf(path,sizeof(path),"%s/mono-%d.f32",argv[1],preset);FILE *f=fopen(path,"wb");if(!f)return 2;
    int p[SynthParameterCount];memcpy(p,presetValues(1,preset),sizeof(p));voice=SynthVoice{};
    const int phrase[]={33,33,40,36,31,33,43,40};unsigned length=preset==1?64000:16000;
    for(unsigned i=0;i<32000*12;++i){unsigned n=i/length;
      if(i<32000*10&&i%length==0)voice.on(MonoInstrument,phrase[n%8],tables.frequency[phrase[n%8]],100,64,p,preset==1?1:0,32000,37,20);
      if(preset!=1&&i%length==length*3/4)voice.off();
      if(i==32000*10)voice.off();
      int l=0,r=0;voice.render(l,r,tables);float x=l/32768.f;fwrite(&x,4,1,f);
    }fclose(f);
  }
  puts("Rendered four dry MONO presets at 32 kHz; no effects or monitor gain");
}
