#pragma once
#include "effect_defs.h"
#include <math.h>
#include <stdint.h>
namespace moss {
// A low-rate control layer; no allocation, tables or random events in prepare().
// Parameter locks change targets without restarting the tape or its defects.
struct ReelLife {
  unsigned rate=32000, divider=0, driftWait=0, dropWait=0, dropHold=0;
  uint32_t random=357;
  float amount=0, driftDepth=0, stereoDepth=0, dropDepth=0;
  float drift=0, driftTarget=0, drop=1, dropTarget=1;
  float loss=0, lossTarget=0, lossCoefficient=1, low[2]={};
  float hiss=0, hissTarget=0, hissTone=0, noiseLow[2]={}, noiseCoefficient=0;
  float follow=1, output=1, outputTarget=1, envelope=0, peak=0;
  float transport=0, smoothing=0, driftSmoothing=0;
  float gain=1, noiseGain=0, gainStep=0, noiseStep=0, noisePeak=0;
  bool running=false, active=false;
  uint32_t next() { random^=random<<13;random^=random>>17;random^=random<<5;return random; }
  float uniform() { return float(next()>>8)*(1.f/16777216.f); }
  void reset(unsigned hz,uint32_t seed=357) {
    *this=ReelLife{};rate=hz;random=seed?seed:357;
    smoothing=1-expf(-16.f/(rate*.025f));
    driftSmoothing=1-expf(-16.f/(rate*.6f));
    noiseCoefficient=1-expf(-6.28318530718f*1200/rate);
    driftWait=rate/16;dropWait=rate/4; // control ticks, i.e. first defect after four seconds
  }
  void prepare(const int *p,float mix) {
    amount=mix;
    driftDepth=p[ReelDrift]*.01f*mix*rate*.0003f;
    stereoDepth=p[ReelStereo]*.01f*mix*rate*.0001f;
    dropDepth=p[ReelDrop]*.009f*mix;
    lossTarget=p[ReelLoss]*.01f*mix;
    lossCoefficient=1-expf(-6.28318530718f*(14000-11600*lossTarget)/rate);
    hissTarget=p[ReelHiss]?sqrtf(3.f)*powf(10.f,(p[ReelHiss]-73)*.05f)*mix:0;
    hissTone=p[ReelHissTone]*.01f;follow=p[ReelFollow]*.01f;
    outputTarget=powf(10.f,p[ReelOutput]*.005f*mix);
    active=true;
  }
  void control() {
    envelope+=(peak-envelope)*(peak>envelope?.4f:.004f);peak=0;
    if(driftWait) --driftWait;
    else {driftTarget=(uniform()*2-1);driftWait=unsigned(rate*(.5f+uniform()*1.3f)/16);}
    drift+=(driftDepth*driftTarget-drift)*driftSmoothing;
    if(driftDepth==0&&fabsf(drift)<1e-5f)drift=0;
    if(dropHold) {if(!--dropHold)dropTarget=1;}
    else if(dropWait) --dropWait;
    else {
      dropTarget=1-dropDepth*(.35f+.65f*uniform());
      dropHold=unsigned(rate*(.025f+.08f*uniform())/16);
      dropWait=unsigned(rate*(4.f+uniform()*8.f)/16);
    }
    if(dropDepth==0)dropTarget=1;
    drop+=(dropTarget-drop)*(dropTarget<drop?.02f:.006f);
    loss+=(lossTarget-loss)*smoothing;hiss+=(hissTarget-hiss)*smoothing;
    output+=(outputTarget-output)*smoothing;
    transport+=((running?1.f:0.f)-transport)*smoothing;
    gainStep=(drop*output-gain)*(1.f/16);
    float follower=envelope*4;if(follower>1)follower=1;
    noiseStep=(hiss*transport*((1-follow)+follow*follower)*output-noiseGain)*(1.f/16);
    if(!running&&transport<1e-6f)transport=0;
    if(lossTarget==0&&loss<1e-6f)loss=0;
    if(hissTarget==0&&hiss<1e-9f)hiss=0;
    // Single precision recurrences stall a few ULPs before unity.
    if(outputTarget==1&&fabsf(output-1)<1e-5f)output=1;
    if(dropTarget==1&&1-drop<1e-5f)drop=1;
    // Neutral racks retain only their transport FIFO.
    active=amount!=0&&(driftDepth!=0||stereoDepth!=0||dropDepth!=0||lossTarget!=0||hissTarget!=0||outputTarget!=1);
    active|=loss!=0||hiss!=0||output!=1||drop!=1||fabsf(drift)>1e-5f;
  }
  void process(float &l,float &r) {
    float left=fabsf(l),right=fabsf(r),input=left>right?left:right;if(input>peak)peak=input;
    gain+=gainStep;noiseGain+=noiseStep;
    if(loss!=0) {
      low[0]+=lossCoefficient*(l-low[0]);low[1]+=lossCoefficient*(r-low[1]);
      l+=loss*(low[0]-l);r+=loss*(low[1]-r);
    } else {low[0]=l;low[1]=r;}
    l*=gain;r*=gain;
    if(noiseGain>1e-9f) {
      float *channels[]={&l,&r};
      for(unsigned ch=0;ch<2;++ch) {
        float n=uniform()*2-1;noiseLow[ch]+=noiseCoefficient*(n-noiseLow[ch]);
        // At zero tone the declared dBFS is the white-noise RMS.
        float colored=hissTone<0?noiseLow[ch]*2.9f:n-noiseLow[ch];
        n+=fabsf(hissTone)*(colored-n);float added=n*noiseGain;*channels[ch]+=added;
        if(fabsf(added)>noisePeak)noisePeak=fabsf(added);
      }
    }
  }
};
} // namespace moss
