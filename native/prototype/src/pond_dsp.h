#pragma once
#include <math.h>
#include <stdint.h>
namespace moss {
// Original compact orthogonal FDN with four input diffusers. No allocation, transcendentals or
// integer division in the sample loop. Storage capacity is not delay length.
inline float pondSeconds(int value) {return .3f*powf(133.333333f,value*.01f);}
struct PondDSP {
#ifdef MOSS_POND_EIGHT_LINES
  static constexpr int Lines=8;
#else
  static constexpr int Lines=4;
#endif
  static constexpr unsigned Capacity=16384,Stride=Capacity+16,Pre=32768;
  float lines[Lines][Stride]={},diffuser[6][2048]={},predelay[Pre][2]={};
  float dampState[Lines]={},lowState[2]={};
  float length[Lines]={},lengthTarget[Lines]={},lengthDelta[Lines]={},gain[Lines]={},gainTarget[Lines]={};
  unsigned head=0,written=0,apHead[6]={},apWritten[6]={},apLength[6]={};
  unsigned rate=0,control=0;
  float pre=0,preTarget=0,preDelta=0,phase=0;
  float damping=.5f,dampingTarget=.5f,diffusion=.6f,diffusionTarget=.6f;
  float motion=0,motionTarget=0,lowCut=0,lowCutTarget=0;
  static float clip(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
  static float clean(float x){return fabsf(x)<1e-20f?0:x;}
  void prepare(const int *p,unsigned hz) {
    bool initial=rate!=hz;rate=hz;
    static constexpr float times[]={29.7f,41.1f,59.3f,73.9f,37.1f,43.7f,53.1f,67.7f};
    float size=.4f+p[0]*.018f,seconds=pondSeconds(p[1]);
    for(int i=0;i<Lines;++i){
      lengthTarget[i]=clip(times[i]*.001f*rate*size,8,Capacity-8);
      float nested=i==1?9.7f:i==3?15.7f:0;
      gainTarget[i]=powf(.001f,(lengthTarget[i]+nested*.001f*rate)/(rate*seconds));
      if(initial){length[i]=lengthTarget[i];gain[i]=gainTarget[i];}
    }
    float cutoff=14000.f*powf(500.f/14000.f,p[2]*.01f);
    dampingTarget=1-expf(-6.28318530718f*fminf(cutoff,rate*.4f)/rate);
    preTarget=clip(p[4]*.001f*rate,0,Pre-2);
    diffusionTarget=p[5]*.0072f;
    motionTarget=p[6]*.00000125f*rate; // up to 0.125 ms, slow decorrelation
    lowCutTarget=p[7]?1-expf(-6.28318530718f*p[7]/rate):0;
    const float apMs[]={4.7f,12.7f,6.1f,10.3f,9.7f,15.7f};
    for(int i=0;i<6;++i)apLength[i]=unsigned(clip(apMs[i]*.001f*rate,1,2047));
    if(initial){pre=preTarget;damping=dampingTarget;diffusion=diffusionTarget;motion=motionTarget;lowCut=lowCutTarget;}
  }
  void start(){
    head=written=control=0;phase=0;
    for(int i=0;i<Lines;++i){dampState[i]=0;length[i]=lengthTarget[i];lengthDelta[i]=0;gain[i]=gainTarget[i];}
    for(int i=0;i<6;++i)apHead[i]=apWritten[i]=0;
    lowState[0]=lowState[1]=0;pre=preTarget;preDelta=0;
  }
  float allpass(float x,int k){
    unsigned at=apHead[k],len=apLength[k];
    float old=apWritten[k]>=len?diffuser[k][at]:0;
    float y=old-diffusion*x;
    diffuser[k][at]=clean(x+diffusion*y);
    if(++apHead[k]>=len)apHead[k]=0;
    if(apWritten[k]<2048)++apWritten[k];
    return y;
  }
  float readLine(int k,float delay) const {
    unsigned whole=unsigned(delay),at=(head-whole)&(Capacity-1);
    float f=delay-whole;
    float a=written>=whole?lines[k][at]:0;
    float b=written>=whole+1?lines[k][(at-1)&(Capacity-1)]:0;
    return a+(b-a)*f;
  }
  void process(const float *input,float *output){
    if(!control){
      control=32;phase+=32.f*.17f/rate;if(phase>=1)phase-=1;
      damping+=(dampingTarget-damping)*.04f;diffusion+=(diffusionTarget-diffusion)*.04f;
      motion+=(motionTarget-motion)*.04f;lowCut+=(lowCutTarget-lowCut)*.04f;
      if(fabsf(lowCut-lowCutTarget)<1e-8f)lowCut=lowCutTarget;
      for(int i=0;i<Lines;++i){
        float t=phase+i*(Lines==4?.2317f:.1237f);if(t>=1)t-=1;
        float wave=1-4*fabsf(t-.5f);
        // Bounded read-head motion also makes SIZE locks click-free.
        lengthDelta[i]=clip((lengthTarget[i]+motion*wave-length[i])/32,-.025f,.025f);
        gain[i]+=(gainTarget[i]-gain[i])*.04f;
      }
      preDelta=clip((preTarget-pre)/32,-.025f,.025f);
    }
    --control;pre+=preDelta;
    unsigned ph=head&(Pre-1);predelay[ph][0]=input[0];predelay[ph][1]=input[1];
    unsigned pd=unsigned(pre),pa=(head-pd)&(Pre-1);float pf=pre-pd,in[2];
    for(int ch=0;ch<2;++ch){
      float a=written>=pd?predelay[pa][ch]:0,b=written>pd?predelay[(pa-1)&(Pre-1)][ch]:0;
      float x=a+(b-a)*pf;
      if(lowCut>0){lowState[ch]+=lowCut*(x-lowState[ch]);x-=lowState[ch];}
      in[ch]=allpass(allpass(x,ch*2),ch*2+1);
    }
    float v[Lines],sum=0;
    for(int i=0;i<Lines;++i){length[i]+=lengthDelta[i];v[i]=readLine(i,length[i]);}
    for(int i=0;i<Lines;++i){dampState[i]=clean(dampState[i]+damping*(v[i]-dampState[i]));v[i]=dampState[i];}
    // Nested diffusion builds new echoes on each circulation, so a compact
    // matrix does not depend only on a diffuse input to make a dense late tail.
    v[1]=allpass(v[1],4);v[3]=allpass(v[3],5);
    for(float x:v)sum+=x;
    // Householder reflection is energy preserving; every line feeds every
    // other line. Independent stereo injection/output vectors avoid dual mono.
    static constexpr float il[]={1,1,1,1,-1,-1,-1,-1},ir[]={1,1,-1,-1,1,1,-1,-1};
    static constexpr float ol[]={1,-1,1,-1,1,-1,1,-1},orr[]={1,-1,-1,1,-1,1,1,-1};
    float l=0,r=0;constexpr float norm=Lines==4?.5f:.353553391f;
    for(int i=0;i<Lines;++i){
      float inputL=Lines==4?(i<2?1.f:-1.f):il[i],inputR=Lines==4?(i%2?-1.f:1.f):ir[i];
      float x=(sum*(2.f/Lines)-v[i])*gain[i]+(in[0]*inputL+in[1]*inputR)*norm;
      // Only a safety ceiling; normal operating signals remain linear.
      lines[i][head&(Capacity-1)]=clip(x,-16,16);
      l+=v[i]*(Lines==4?1.f:ol[i]);r+=v[i]*(Lines==4?((i==0||i==3)?1.f:-1.f):orr[i]);
    }
    output[0]=l*norm;output[1]=r*norm;
    ++head;if(written<Pre)++written;
  }
};
}
