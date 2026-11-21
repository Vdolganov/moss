#include "../src/app.h"
#include "legacy_fixture.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static GranularTables table;
static GranularVoice voice;
static SampleBuffer sample;
static PondDSP pond;
static Codec codec;
static Project project,restored;
static void grains(){
  table.init();sample.rate=32000;sample.channels=2;sample.frames=32000;sample.bytes=128000;
  for(unsigned i=0;i<sample.frames;++i){sample.pcm[i*2]=int16_t(8000*sin(i*.031));sample.pcm[i*2+1]=int16_t(5000*cos(i*.027));}
  const unsigned frameCounts[]={1u,3u,32000u};const float pitches[]={-12.f,0.f,7.f,24.f};
  for(unsigned frames:frameCounts)for(float pitch:pitches){
    sample.frames=frames;assert(voice.init(sample,table,32000,8,72));
    GranularParameters p;p.reverse=1;p.speed=0;p.pitch=pitch;p.sizeMs=1000;p.density=0;p.position=.7f;
    voice.setParameters(p);voice.noteOn();voice.spawn();double expected=voice.grains[0].index+voice.grains[0].fraction;
    for(int n=0;n<1000;++n){float l,r;voice.render(l,r);expected-=voice.readStep;while(expected<0)expected+=frames;
      double actual=voice.grains[0].index+voice.grains[0].fraction;
      assert(fabs(actual-expected)<.03 || fabs(fabs(actual-expected)-frames)<.03);assert(isfinite(l)&&isfinite(r));}
  }
  sample.frames=32000;assert(voice.init(sample,table,32000,8,543));
  GranularParameters p;p.reverse=.5f;p.detune=15;p.sizeVariation=.4f;p.irregularity=.4f;
  p.sizeMs=1200;p.density=8;p.scatter=1;p.spread=1;p.speed=0;
  voice.setParameters(p);voice.noteOn();unsigned reverse=0,total=0,minLength=999999,maxLength=0;uint64_t previous=0;
  for(int n=0;n<32000*12;++n){float l,r;voice.render(l,r);assert(isfinite(l)&&isfinite(r)&&voice.count<=8);
    if(voice.stats.spawned!=previous){previous=voice.stats.spawned;auto &g=voice.grains[voice.count-1];reverse+=g.reverse;++total;minLength=minLength<g.length?minLength:g.length;maxLength=maxLength>g.length?maxLength:g.length;}}
  assert(total>35 && reverse>total/4 && reverse<total*3/4 && maxLength>minLength*1.4);
  auto g=voice.grains[0];p.reverse=1;p.pitch=24;voice.setParameters(p);
  assert(voice.grains[0].reverse==g.reverse && voice.grains[0].stepWhole==g.stepWhole && voice.grains[0].age==g.age);
  printf("PASS cloud: reverse fractional/wrap/tiny sources; mixed %u/%u; length %u..%u; edits preserve living grains\n",reverse,total,minLength,maxLength);
}
static void storage(){
  project=Project{};auto &t=project.tracks[0];t.instrument=GranularInstrument;
  for(int i=16;i<20;++i)t.steps[3].grainLocks[i]=GranularSpecs[i].high;
  for(int i=0;i<8;++i)t.steps[3].fxLocks[Pond][i]=fxHigh(Pond,i);
  assert(codec.encode(project)&&codec.decode(restored,codec.cursor));
  for(int i=16;i<20;++i)assert(restored.tracks[0].steps[3].grainLocks[i]==GranularSpecs[i].high);
  for(int i=0;i<8;++i)assert(restored.tracks[0].steps[3].fxLocks[Pond][i]==fxHigh(Pond,i));
  codec.cursor=16;codec.ok=true;legacyFields(codec,project,13);memcpy(codec.bytes,"MS13",4);codec.word(4,13);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));
  for(int i=16;i<20;++i)assert(restored.tracks[0].granular.values[i]==0 && restored.tracks[0].steps[3].grainLocks[i]==Inherit);
  for(int i=4;i<8;++i)assert(restored.tracks[0].steps[3].fxLocks[Pond][i]==Inherit);
  puts("PASS MS17 round trip and real MS13 field layout; no invented cloud or POND locks");
}
static void reverb(){
  const unsigned rates[]={8000u,32000u,44100u,96000u};
  for(unsigned rate:rates){
    pond=PondDSP{};int p[]={60,65,20,100,20,85,65,0};pond.prepare(p,rate);pond.start();
    double energy=0,difference=0;float max=0;
    for(unsigned i=0;i<rate*3;++i){float in[]={i==0?.5f:0,0},out[2];pond.process(in,out);
      assert(isfinite(out[0])&&isfinite(out[1]));energy+=out[0]*out[0]+out[1]*out[1];difference+=fabs(out[0]-out[1]);
      max=fmaxf(max,fmaxf(fabsf(out[0]),fabsf(out[1])));}
    assert(energy>.001&&difference>.1&&max<1);
    for(unsigned i=0;i<rate;++i){if(i%317==0){p[0]=(i/317)%2?0:100;p[1]=100;p[4]=(i/317)%2?0:200;p[6]=100;pond.prepare(p,rate);}
      float in[]={.05f,.03f},out[2];pond.process(in,out);assert(isfinite(out[0])&&fabsf(out[0])<8 && fabsf(out[1])<8);}
    pond.start();for(unsigned i=0;i<rate/2;++i){float in[2]={},out[2];pond.process(in,out);assert(out[0]==0&&out[1]==0);}
    printf("PASS POND %u Hz: stereo tail, energy %.5f, peak %.5f, rapid locks, logical reset\n",rate,energy,max);
  }
}
int main(){grains();storage();reverb();}
