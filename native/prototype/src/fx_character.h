#pragma once
#include "dsp_math.h"
#include <math.h>
namespace moss {
// Original portable DSP. No firmware code, addresses or lookup tables.
inline float fxClamp(float v,float lo,float hi) { return maximum(lo,minimum(hi,v)); }
inline float fxClean(float v) { return fabsf(v)<1e-18f?0:v; }
inline float fxPole(float hz,unsigned rate) { return 1.f-expf(-6.2831853f*hz/rate); }
struct DriveDSP {
  struct Coeff {
    float gain=1, bias=0, tone=0, body=0, output=1, dynamics=0, amount=0;
    float smooth=0, bodyPole=0, tonePole=0, dcPole=0, attack=0, release=0, fade=0;
    int curve=0;
  } target, current;
  float low[2]={}, highSplit[2]={}, dc[2]={}, previous[2]={}, envelope=0;
  float previousIntegral[3][2]={}, curveBlend=1;
  int curve=0, nextCurve=0;
  static float shape(float x,int curve) {
    x=fxClamp(x,-1,1);
    if(!curve) return x*(1.5f-.5f*x*x);
    x*=2-fabsf(x); x*=2-fabsf(x);
    return curve==2?x*(2-fabsf(x)):x;
  }
  // Even primitive of the odd saturator; continuous across +/-1.
  static float integral(float x,int curve) {
    float u=fabsf(x);
    if(!curve) return u>=1?u-.375f:u*u*(.75f-.125f*u*u);
    if(curve==1) return u>=1?u-.2f:u*u*(2+u*(-2+u*(1-.2f*u)));
    return u>=1?u-1.f/9:u*u*(4+u*(-28.f/3+u*(14+u*(-14+u*(28.f/3+u*(-4+u*(1-u/9)))))));
  }
  static float antialiased(float x,float old,int curve) {
    float delta=x-old;
    // Midpoint fallback prevents cancellation when successive inputs coincide.
    return fabsf(delta)<.001f ? shape((x+old)*.5f,curve)
      : fxClamp((integral(x,curve)-integral(old,curve))/delta,-1,1);
  }
  void prepare(const int *p,unsigned rate) {
    float d=p[0]/100.f;
    target.gain=(1+63*d*d)/(p[4]==0?1.5f:p[4]==1?4.f:8.f);
    target.bias=p[5]*.003f;
    target.tone=(p[1]-50)*.018f;
    target.body=(p[6]-50)*.02f;
    target.output=p[2]*.01f;
    target.dynamics=p[7]*.025f;
    target.amount=minimum(1,d*10);
    target.curve=p[4];
    target.smooth=1.f-expf(-1.f/(rate*.008f));
    target.bodyPole=fxPole(180,rate); target.tonePole=fxPole(2400,rate);
    target.dcPole=fxPole(5,rate);
    target.attack=1.f-expf(-1.f/(rate*.004f));
    target.release=1.f-expf(-1.f/(rate*.12f));
    target.fade=1.f/(rate*.02f);
  }
  void start() {
    current=target; envelope=0; curve=nextCurve=target.curve;curveBlend=1;
    for(int c=0;c<2;++c) low[c]=highSplit[c]=dc[c]=previous[c]=0;
    for(auto &row:previousIntegral) for(float &v:row)v=0;
  }
  float evaluate(float x,int ch,int kind) {
    float value=integral(x,kind),delta=x-previous[ch];
    float out=fabsf(delta)<.001f?shape((x+previous[ch])*.5f,kind)
      :fxClamp((value-previousIntegral[kind][ch])/delta,-1,1);
    previousIntegral[kind][ch]=value;return out;
  }
  void process(const float *in,float *out) {
    const float a=target.smooth;
    current.gain+=(target.gain-current.gain)*a;
    current.bias+=(target.bias-current.bias)*a;
    current.tone+=(target.tone-current.tone)*a;
    current.body+=(target.body-current.body)*a;
    current.output+=(target.output-current.output)*a;
    current.dynamics+=(target.dynamics-current.dynamics)*a;
    current.amount+=(target.amount-current.amount)*a;
    float peak=maximum(fabsf(in[0]),fabsf(in[1]));
    envelope=fxClean(envelope+(peak-envelope)*(peak>envelope?target.attack:target.release));
    float gain=current.gain/(1+envelope*current.dynamics);
    if(curveBlend>=1 && curve!=target.curve) {
      nextCurve=target.curve;curveBlend=0;
      for(int c=0;c<2;++c) previousIntegral[nextCurve][c]=integral(previous[c],nextCurve);
    }
    float offset=shape(current.bias,curve),nextOffset=curveBlend<1?shape(current.bias,nextCurve):0;
    for(int c=0;c<2;++c) {
      low[c]=fxClean(low[c]+target.bodyPole*(in[c]-low[c]));
      float v=in[c]+low[c]*current.body;
      float x=fxClamp(v*gain+current.bias,-64,64);
      float shaped=evaluate(x,c,curve)-offset;
      if(curveBlend<1) shaped+=curveBlend*(evaluate(x,c,nextCurve)-nextOffset-shaped);
      previous[c]=x;
      v+=current.amount*(shaped-v);
      dc[c]=fxClean(dc[c]+target.dcPole*(v-dc[c])); v-=dc[c];
      highSplit[c]=fxClean(highSplit[c]+target.tonePole*(v-highSplit[c]));
      out[c]=(v+current.tone*(v-highSplit[c]))*current.output;
    }
    if(curveBlend<1) { curveBlend=minimum(1,curveBlend+target.fade);if(curveBlend>=1)curve=nextCurve; }
  }
};
struct OrbitDSP {
  struct Coeff { float time=1,feedback=0,width=0,low=0,lowMix=0,high=1,drive=0,motion=0,smooth=0,fade=0,phase=0; } target,current;
  float low[2]={},high[2]={},phase=0,time=1,next=1,blend=1;
  void prepare(const int *p,unsigned rate,int bpm,unsigned capacity) {
    static constexpr float beats[]={.25f,.5f,.75f,1,1.5f,2,3,4};
    target.time=fxClamp(beats[p[0]]*60*rate/bpm,2,float(capacity-4));
    target.feedback=p[1]*.01f; target.width=p[2]*.01f;
    target.low=fxPole(float(maximum(1,p[4])),rate); target.lowMix=p[4]?1:0;
    target.high=fxPole(float(p[5]),rate);
    target.drive=p[6]*.01f; target.motion=p[7]*.000008f*rate;
    target.smooth=1.f-expf(-1.f/(rate*.012f));
    target.fade=1.f/(rate*.04f); target.phase=.19f/rate;
  }
  void start() {
    current=target;time=next=target.time;blend=1;phase=0;
    low[0]=low[1]=high[0]=high[1]=0;
  }
  static float sine(float x) {
    float y=4*x*(1-fabsf(x));return .225f*(y*fabsf(y)-y)+y;
  }
  template<unsigned N> static void read(const float (&buffer)[N][2],unsigned cursor,unsigned written,float distance,float *out) {
    distance=fxClamp(distance,1,float(N-2));
    unsigned whole=unsigned(distance);float fraction=distance-whole;
    unsigned index=cursor>=whole?cursor-whole:cursor+N-whole,previous=index?index-1:N-1;
    for(int c=0;c<2;++c) out[c]=whole+1<=written?buffer[index][c]+fraction*(buffer[previous][c]-buffer[index][c]):0;
  }
  template<unsigned N> void process(const float *in,float *out,float (&buffer)[N][2],unsigned &cursor,unsigned &written) {
    float a=target.smooth;
    current.feedback+=(target.feedback-current.feedback)*a;
    current.width+=(target.width-current.width)*a;
    current.low+=(target.low-current.low)*a;current.high+=(target.high-current.high)*a;
    current.lowMix+=(target.lowMix-current.lowMix)*a;
    current.drive+=(target.drive-current.drive)*a;current.motion+=(target.motion-current.motion)*a;
    // Finish an in-flight crossfade before accepting the latest TIME target.
    // Rapid locks cannot continuously reset the fade or read an invalid head.
    if(blend>=1 && target.time!=time) { next=target.time;blend=0; }
    phase+=target.phase;if(phase>=1)phase-=2;
    float motion=current.motion*sine(phase);
    read(buffer,cursor,written,time+motion,out);
    if(blend<1) {
      float other[2];read(buffer,cursor,written,next+motion,other);
      for(int c=0;c<2;++c)out[c]+=blend*(other[c]-out[c]);
      blend=minimum(1,blend+target.fade);if(blend>=1)time=next;
    }
    for(int c=0;c<2;++c) {
      low[c]=fxClean(low[c]+current.low*(out[c]-low[c]));
      // Fade out subtraction at LOW CUT 0 without a jump from old filter history.
      float v=out[c]-low[c]*current.lowMix;
      high[c]=fxClean(high[c]+current.high*(v-high[c]));out[c]=high[c];
    }
    float gain=1+current.drive*3, inverse=1/gain;
    for(int c=0;c<2;++c) {
      float feedback=out[c]+current.width*(out[1-c]-out[c]);
      float x=fxClamp(in[c]+feedback*current.feedback,-8,8);
      // Normalize the small-signal slope to keep the feedback loop contractive.
      float curved=DriveDSP::shape(x*gain/1.5f,0)*inverse;
      buffer[cursor][c]=fxClamp(x+current.drive*(curved-x),-8,8);
    }
    if(++cursor==N)cursor=0;
    if(written<N)++written;
  }
};
} // namespace moss
