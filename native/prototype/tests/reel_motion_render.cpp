#include "../src/effects.h" // Production REEL for a genuine old/new comparison.
#include "../src/reel_motion.h"
#include <stdio.h>
#include <stdlib.h>
using namespace moss;
static ReelMotion tape;
static FxState old;
static constexpr unsigned Rate = 32000;
static uint32_t randomState = 1;
static float noise() {randomState=randomState*1664525u+1013904223u;return int32_t(randomState)/2147483648.f;}
static void source(unsigned frame, int kind, float &l, float &r) {
  double t=double(frame)/Rate, beat=fmod(t,.5), hi=fmod(t,.25);
  float n=noise();
  double bass=.17*sin(6.28318530718*55*t)+.08*sin(6.28318530718*110*t);
  double drums=.29*sin(6.28318530718*(48*beat+8*(1-exp(-beat*30))))*exp(-beat*16)
               +.11*n*exp(-hi*80);
  double pad=.075*(sin(6.28318530718*220*t)+sin(6.28318530718*261.625565*t)
                  +sin(6.28318530718*329.627557*t));
  double texture=.045*n*(.5+.5*sin(6.28318530718*1.75*t));
  double envelope=t<.2?t/.2:t>11.5?(12-t)*2:1;
  if(envelope<0)envelope=0;
  l=float(envelope*(kind==0?bass:kind==1?drums:kind==2?pad:kind==3?texture:bass+drums+pad+texture));
  r=float(envelope*(kind==0?bass:kind==1?drums:kind==2?pad*.85:kind==3?-texture:bass+drums+pad*.85-texture));
}
static void little(FILE *f,unsigned value,int bytes) {
  for(int i=0;i<bytes;++i)fputc((value>>(i*8))&255,f);
}
static FILE *wav(const char *name,unsigned frames) {
  FILE *f=fopen(name,"wb");if(!f){perror(name);exit(2);}
  fwrite("RIFF",1,4,f);little(f,36+frames*4,4);fwrite("WAVEfmt ",1,8,f);
  little(f,16,4);little(f,1,2);little(f,2,2);little(f,Rate,4);little(f,Rate*4,4);
  little(f,4,2);little(f,16,2);fwrite("data",1,4,f);little(f,frames*4,4);return f;
}
int main(int argc,char **argv) {
  if(argc!=2)return 1;
  char path[1024];
  // Float diagnostic files retain the actual output and transport position.
  const ReelMotionParams settings[]={
    {10,.1f,0,6,1},{10,.3f,0,6,1},{10,4,0,6,1},
    {0,.3f,10,2,1},{0,.3f,10,6,1},{0,.3f,10,30,1},
    {15,.1f,15,2,1},{15,4,15,30,1},{15,.1f,15,30,1}};
  for(unsigned test=0;test<sizeof(settings)/sizeof(settings[0]);++test) {
    snprintf(path,sizeof(path),"%s/tone-%u.f32",argv[1],test);
    FILE *f=fopen(path,"wb");if(!f)return 2;
    tape.reset(Rate);tape.prepare(settings[test],Rate);
    for(unsigned frame=0;frame<Rate*24;++frame) {
      if(test==8 && !(frame%480)) {
        bool on=(frame/480)&1;
        tape.prepare({on?15.f:0.f,on?.1f:4.f,on?15.f:0.f,on?30.f:2.f,1},Rate);
      }
      float l=float(.5*sin(6.283185307179586*1000*frame/Rate)),r=l;
      tape.process(l,r);float data[]={l,r,tape.position};fwrite(data,sizeof(float),3,f);
    }
    fclose(f);
  }
  const char *names[]={"bass","drums","pad","texture","mix"};
  const char *variants[]={"dry","old-neutral","old-wobble","new-neutral","new-gentle","new-deep-wow","new-flutter","new-short"};
  for(int kind=0;kind<5;++kind)for(int variant=0;variant<8;++variant) {
    snprintf(path,sizeof(path),"%s/%s-%s.wav",argv[1],names[kind],variants[variant]);
    // Align listening files to source time; latency is measured separately.
    unsigned latency=variant==0?0:variant<3?192:variant==7?192:512;
    FILE *f=wav(path,Rate*12);tape.reset(Rate);old.reset();
    if(variant==7) {tape.latency=192;tape.position=192;} // explicit 6 ms experiment
    ReelMotionParams p;
    if(variant==4||variant==7)p={4,.3f,1,6,1};
    if(variant==5)p={15,.1f,2,6,1};
    if(variant==6)p={2,.3f,12,18,1};
    tape.prepare(p,Rate);
    Effect effect;effect.select(Reel);effect.enable(true);
    effect.values[Reel][0]=variant==2?60:0;
    effect.values[Reel][1]=effect.values[Reel][2]=0;effect.values[Reel][3]=100;
    old.prepare(effect.values,Rate,120);randomState=1;
    for(unsigned frame=0;frame<Rate*12+latency;++frame) {
      float l,r;source(frame,kind,l,r);
      if(variant==1||variant==2)old.processFloat(l,r,Reel,true,effect.values[Reel],Rate,120);
      else if(variant>2)tape.process(l,r);
      if(frame>=latency) {
        little(f,unsigned(int16_t(lroundf(fxClamp(l,-.999f,.999f)*32767))),2);
        little(f,unsigned(int16_t(lroundf(fxClamp(r,-.999f,.999f)*32767))),2);
      }
    }
    fclose(f);
  }
  puts("Rendered 9 float tone probes and 40 aligned original audio comparisons.");
}
