#define MOSS_REEL_MOTION_LAB 1
#include "../src/effects.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
using namespace moss;
static ReelColor color;
static FxState rack;
static constexpr double Pi2=6.283185307179586;
static uint32_t seed=42;
static unsigned long long micros() {timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (unsigned long long)t.tv_sec*1000000+t.tv_nsec/1000;}
static float noise(){seed=seed*1664525u+1013904223u;return int32_t(seed)/1073741824.f;}
static void convolution() {
  float input[32],worst=0;
  for(unsigned n=0;n<10000;++n) {
    for(float &x:input)x=noise();
    float actual[2],expected[2]={};
    ReelColor::dot(input,ReelColor::Halfband,actual);
    for(unsigned k=0;k<12;++k)for(unsigned ch=0;ch<2;++ch)expected[ch]+=input[k*2+ch]*ReelColor::Halfband[k];
    for(unsigned ch=0;ch<2;++ch)worst=fmaxf(worst,fabsf(actual[ch]-expected[ch]));
    expected[0]=expected[1]=0;ReelColor::dot(input,ReelColor::Correction,actual);
    for(unsigned k=0;k<4;++k)for(unsigned ch=0;ch<2;++ch)expected[ch]+=input[k*2+ch]*ReelColor::Correction[k];
    for(unsigned ch=0;ch<2;++ch)worst=fmaxf(worst,fabsf(actual[ch]-expected[ch]));
  }
  assert(worst<1e-6f);printf("PASS 20000 independent stereo FIR/scalar comparisons: max error %.9f\n",double(worst));
}
static void neutral() {
  const unsigned rates[]={8000u,32000u,44100u,48000u,96000u};
  for(unsigned rate:rates) {
    color.reset(rate);color.prepare(0,0,0,1,rate);
    float history[32]={};
    for(unsigned n=0;n<rate;++n) {
      float x=noise(),l=x,r=-x;history[n&31]=x;
      color.processInput(l,r);color.processOutput(l,r);
      float expected=n<ReelColor::Latency?0:history[(n-ReelColor::Latency)&31];
      assert(l==expected && r==-expected);
    }
  }
  rack.reset();Effect e;e.select(Reel);e.enable(true);
  memset(e.values[Reel],0,sizeof(e.values[Reel]));e.values[Reel][3]=100;
  rack.prepare(e.values,32000,120);int input[1024]={};
  for(unsigned n=0;n<64000;++n) {
    int x=int(noise()*32768),l=x,r=-x;input[n&1023]=x;
    rack.processChain(l,r,e,e.values,32000,120);
    int expected=n<512?0:input[(n-512)&1023];assert(l==expected && r==-expected);
  }
  puts("PASS neutral: exact float at five rates; full FX rack stays at 512 samples including colour latency");
}
static double response(float drive,float tone,float bump,double frequency,double amplitude=.001) {
  color.reset(32000);color.prepare(drive,tone,bump,1,32000);
  double re=0,im=0;
  for(unsigned n=0;n<64000;++n) {
    float l=float(amplitude*sin(Pi2*frequency*n/32000)),r=l;
    color.processInput(l,r);color.processOutput(l,r);assert(l==r && isfinite(l));
    if(n>=32000){re+=l*cos(Pi2*frequency*n/32000);im+=l*sin(Pi2*frequency*n/32000);}
  }
  return 20*log10(2*sqrt(re*re+im*im)/32000/amplitude);
}
static void frequency() {
  double at80=response(0,0,6,80),low=response(0,0,6,10),high=response(0,0,6,12000);
  assert(fabs(at80-6)<.02 && fabs(low)<.5 && fabs(high)<.01);
  double cut=response(0,-6,0,12000),boost=response(0,6,0,12000);
  assert(fabs(cut+6)<.03 && fabs(boost-6)<.03);
  double reference=response(100,0,0,100,.00001),worst=0;
  const double frequencies[]={30.,80.,300.,1000.,3000.,6000.,10000.,12000.};
  for(double hz:frequencies) {
    double db=response(100,0,0,hz,.00001)-reference;
    if(fabs(db)>worst)worst=fabs(db);
  }
  printf("PASS colour response: bump80 %.5f dB, shelf12k %.5f/%.5f dB; saturation small-signal ripple <=12k %.5f dB\n",at80,cut,boost,worst);
  assert(worst<.35);
}
static void transitions() {
  float maximum=0,maxJump=0;
  const unsigned rates[]={8000u,32000u,96000u};
  for(unsigned rate:rates) {
    color.reset(rate);float previous=0;
    for(unsigned n=0;n<rate*3;++n) {
      if(!(n%257))color.prepare((n/257)%2?100:0,(n/257)%2?6:-6,6,(n/257)%3?1:0,rate);
      float x=float(.4*sin(Pi2*110*n/rate)),l=x,r=-x;
      color.processInput(l,r);color.processOutput(l,r);
      assert(isfinite(l)&&fabsf(l+r)<1e-6f&&fabsf(l)<4);
      maximum=fmaxf(maximum,fabsf(l));maxJump=fmaxf(maxJump,fabsf(l-previous));previous=l;
    }
    color.prepare(100,6,6,0,rate);float history[32]={};
    for(unsigned n=0;n<rate;++n) {
      float x=noise(),l=x,r=-x;history[n&31]=x;
      color.processInput(l,r);color.processOutput(l,r);
      if(n>rate/10)assert(!color.saturating&&!color.equalizing&&l==history[(n-ReelColor::Latency)&31]&&r==-l);
    }
  }
  // An inactive/reused SONG rack must also clear its colour and reserved FIFO.
  rack.invalidate();Effect e;e.select(Reel);rack.prepare(e.values,32000,120);
  for(unsigned n=0;n<64000;++n){int l=0,r=0;rack.processChain(l,r,e,e.values,32000,120);assert(l==0&&r==0);}
  printf("PASS rapid locks, bypass, AMOUNT=0, SONG reuse, stereo symmetry: peak %.5f, largest 110 Hz adjacent delta %.5f\n",double(maximum),double(maxJump));
}
static void topology() {
  static ReelMotion referenceMotion;static ReelColor referenceColor;
  rack.reset();Effect e;e.select(Reel);e.enable(true);int *p=e.values[Reel];
  referenceMotion.reset(32000,ReelColor::Latency);referenceColor.reset(32000);
  float worst=0;
  for(unsigned n=0;n<32000*4;++n) {
    if(!(n%4001)) {
      unsigned mode=(n/4001)%5;
      p[ReelWow]=mode==4?60:0;p[ReelWowRate]=30;p[ReelFlutter]=mode==4?30:0;p[ReelAmount]=mode==3?0:100;
      p[ReelFlutterRate]=60;p[ReelDrive]=(mode==1||mode==2||mode==4)?80:0;p[ReelTone]=mode==2?30:0;p[ReelBump]=mode==2?40:0;
      rack.prepare(e.values,32000,120);
      referenceMotion.prepare({p[ReelWow]*.1f,p[ReelWowRate]*.01f,p[ReelFlutter]*.1f,p[ReelFlutterRate]*.1f,p[ReelAmount]*.01f},32000);
      referenceColor.prepare(float(p[ReelDrive]),p[ReelTone]*.1f,p[ReelBump]*.1f,p[ReelAmount]*.01f,32000);
    }
    float x=noise()*.25f,y=noise()*.25f,l=x,r=y,rl=x,rr=y;
    rack.tape.processReel(l,r,true,p,32000);
    referenceColor.processInput(rl,rr);referenceMotion.process(rl,rr);referenceColor.processOutput(rl,rr);
    worst=fmaxf(worst,fmaxf(fabsf(l-rl),fabsf(r-rr)));
  }
  printf("Unified/split FIFO comparison: maximum %.9f\n",double(worst));
  assert(worst<.0004f);
  // At 8 kHz the same maximal motion excursion needs a slightly larger fixed
  // latency to leave room for the colour FIR; no negative read positions.
  rack.reset();p[ReelWow]=p[ReelFlutter]=150;p[ReelWowRate]=10;p[ReelFlutterRate]=20;p[ReelAmount]=p[ReelDrive]=100;
  rack.prepare(e.values,8000,120);
  for(unsigned n=0;n<80000;++n) {
    float l=.1f,r=-l;rack.tape.processReel(l,r,true,p,8000);
    assert(isfinite(l)&&rack.tape.motion.position>=7);
  }
  puts("PASS neutral FIFO merging: reference alignment, rapid reuse and low-rate read margin");
}
static void render(const char *directory) {
  char path[1024];
  const int tones[]={997,3991,7177,11003};
  for(int frequency:tones) {
    snprintf(path,sizeof(path),"%s/alias-%d.f32",directory,frequency);FILE *f=fopen(path,"wb");assert(f);
    color.reset(32000);color.prepare(100,0,0,1,32000);
    float low=0,postX=0,postY=0;
    for(unsigned n=0;n<64000;++n) {
      float x=float(.3*sin(Pi2*frequency*n/32000)),l=x,r=x;
      color.processInput(l,r);color.processOutput(l,r);
      // Same emphasis, drive, shaper, makeup and inverse emphasis at 1x.
      low+=color.prePole*(x-low);float pre=x+(x-low);
      float raw=ReelColor::shape(pre*color.targetGain)*color.targetMakeup;
      float trivial=(raw-color.preQ*postX)/color.preGain+color.postFeedback*postY;postX=raw;postY=trivial;
      if(n>=32000){float values[]={x,l,trivial};assert(fwrite(values,sizeof(float),3,f)==3);}
    }fclose(f);
  }
  // Process original music fixtures made by reel_motion_render.cpp.
  const float settings[][4]={{0,0,0,1},{35,-1,2,1},{70,-2,4,1},{100,3,1,1}};
  for(unsigned mode=0;mode<4;++mode) {
    snprintf(path,sizeof(path),"%s/music-input.f32",directory);FILE *input=fopen(path,"rb");assert(input);
    snprintf(path,sizeof(path),"%s/music-%u.f32",directory,mode);FILE *output=fopen(path,"wb");assert(output);
    color.reset(32000);const auto &p=settings[mode];color.prepare(p[0],p[1],p[2],p[3],32000);
    unsigned frame=0;float lr[2];
    while(fread(lr,sizeof(float),2,input)==2) {
      color.processInput(lr[0],lr[1]);color.processOutput(lr[0],lr[1]);
      if(frame++>=ReelColor::Latency)assert(fwrite(lr,sizeof(float),2,output)==2);
    }
    for(unsigned n=0;n<ReelColor::Latency;++n){float l=0,r=0;color.processInput(l,r);color.processOutput(l,r);float tail[]={l,r};fwrite(tail,sizeof(float),2,output);}
    fclose(input);fclose(output);
  }
}
static void benchmark() {
  volatile float checksum=0;
  for(unsigned mode=0;mode<4;++mode) {
    color.reset(32000);color.prepare(mode&1?100:0,mode&2?6:0,mode&2?6:0,1,32000);
    auto start=micros();
    for(unsigned n=0;n<320000;++n) {
      float l=float(int(n%101)-50)*.01f,r=-l;
      color.processInput(l,r);color.processOutput(l,r);checksum+=l;
    }
    printf("BENCH colour mode %u: %.3f us / 1024 stereo frames\n",mode,double(micros()-start)*1024/320000);
  }
  assert(isfinite(checksum));
}
int main(int argc,char **argv) {
  convolution();neutral();frequency();transitions();topology();
  if(argc==2 && !strcmp(argv[1],"--bench"))benchmark();else if(argc==2)render(argv[1]);
  printf("REEL COLOUR TESTS PASSED; colour state %u bytes\n",unsigned(sizeof(ReelColor)));
}
