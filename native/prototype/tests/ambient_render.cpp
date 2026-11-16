#include "../src/engine.h"
#include <stdio.h>
using namespace moss;
static SampleBuffer sample;
static GranularTables tables;
static GranularVoice voice;
static PondDSP pond;
int main(int argc,char **argv){
  if(argc!=2)return 1;
  char path[1024];
  for(int variant=0;variant<6;++variant){
    pond=PondDSP{};int p[]={60,50,0,100,0,80,0,0};
    if(variant==1)p[1]=75;if(variant==2)p[5]=0;if(variant==3)p[6]=80;
    if(variant==4)p[4]=100;if(variant==5)p[2]=85;
    pond.prepare(p,32000);pond.start();snprintf(path,sizeof(path),"%s/pond-ir-%d.f32",argv[1],variant);
    FILE *f=fopen(path,"wb");if(!f)return 2;
    for(unsigned i=0;i<32000*24;++i){float in[]={i==0?.5f:0,0},out[2];pond.process(in,out);fwrite(out,sizeof(float),2,f);}fclose(f);
  }
  tables.init();sample.rate=32000;sample.channels=2;sample.frames=128000;sample.bytes=512000;
  for(unsigned i=0;i<sample.frames;++i){double t=double(i)/sample.rate;
    double env=.4+.3*cos(t*1.7);float x=float(env*(sin(t*6.28318530718*110)+.5*sin(t*6.28318530718*164.81)+.3*sin(t*6.28318530718*261.63)));
    sample.pcm[i*2]=int16_t(x*8500);sample.pcm[i*2+1]=int16_t(x*7000+1500*sin(t*6.28318530718*220.13));}
  for(int variant=0;variant<4;++variant){
    voice.init(sample,tables,32000,8,357);GranularParameters p;p.speed=.08f;p.pitch=-12;p.sizeMs=2400;p.density=8;
    p.scatter=.85f;p.spread=.8f;p.attackMs=1400;p.releaseMs=3500;p.cutoff=10000;
    if(variant){p.reverse=variant==2?1:.5f;p.detune=9;p.sizeVariation=.3f;p.irregularity=.15f;}
    voice.setParameters(p);voice.noteOn();pond=PondDSP{};int fx[]={75,72,35,100,30,90,55,0};pond.prepare(fx,32000);pond.start();
    snprintf(path,sizeof(path),"%s/cloud-%d.f32",argv[1],variant);FILE *f=fopen(path,"wb");if(!f)return 2;
    for(unsigned i=0;i<32000*16;++i){if(i==32000*11)voice.noteOff();float x[2],wet[2];voice.render(x[0],x[1]);
      if(variant==3){pond.process(x,wet);x[0]=x[0]*.65f+wet[0]*.35f;x[1]=x[1]*.65f+wet[1]*.35f;}fwrite(x,sizeof(float),2,f);}fclose(f);
  }
  puts("Rendered six POND impulses and forward / mixed / reverse / POND clouds at 32 kHz");
}
