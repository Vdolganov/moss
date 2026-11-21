#pragma once
#include "mono_dsp.h"
#include "fm_defs.h"
namespace moss {
// Original four-operator PM voice. Two carrier/modulator pairs, or a serial
// chain with a stereo intermediate tap. Independent amplitude, colour and
// pitch envelopes; no external firmware/plugin DSP is included.
struct FmDSP {
  static constexpr unsigned ControlFrames=32, PortraitPoints=96;
  float p[FmParameters]={},target[FmParameters]={};
  float phase[4]={},increment[4]={},incrementDelta[4]={},feedback[2]={};
  float env=0,modEnv=1,pitchEnv=1,clickEnv=1,gain=0,targetGain=1;
  float frequency=55,targetHz=55,bendSemitones=0,attack=1,decay=1,release=1,modDecay=1,pitchDecay=1;
  float smoothing=1,glide=1,clickDecay=1,dcCoefficient=1,lfoPhase=0,lfo=0,topology=0;
  float index=0,indexDelta=0,feedbackAmount=0,feedbackDelta=0,tone=1,toneDelta=0;
  float bandGain=1,bandDelta=0,spread=0,noiseLevel=0,clickLevel=0,sustain=0,modHold=0;
  float low[2]={},dcIn[2]={},dcOut[2]={},fir[2][17]={};
  float portrait[2][PortraitPoints][2]={};
  unsigned rate=32000,control=0,firAt=0,portraitAt=0,portraitClock=0,portraitStride=6,age=0;
  unsigned forced=0,forcedTotal=1;uint32_t random=1;
  bool active=false,gate=false,attacking=true;int mode=0,note=33;
  static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
  static float wrap(float x){int i=int(x);if(x<i)--i;return x-i;}
  float noise(){random^=random<<13;random^=random>>17;random^=random<<5;return float(random&65535)/32767.5f-1;}
  void setTarget(const int *v,int selectedMode){for(int i=0;i<FmParameters;++i)target[i]=float(v[i]);mode=selectedMode;}
  void on(int midi,float hz,int velocity,const int *v,int selectedMode,unsigned sampleRate,uint32_t seed){
    const bool continuing=active&&rate==sampleRate;
    if(!continuing){*this=FmDSP{};rate=sampleRate;frequency=hz;random=seed?seed:1;
      smoothing=1-expf(-float(ControlFrames)/(rate*.008f));dcCoefficient=expf(-6.28318530718f*3/rate);
      for(int i=0;i<FmParameters;++i)p[i]=float(v[i]);topology=float(selectedMode);
    }
    note=midi;targetHz=hz;targetGain=velocity/127.f;
    // The amplitude envelope already ramps a fresh note from zero. A second
    // gain ramp would erase short percussion excitation before it reaches VCA.
    if(!continuing)gain=targetGain;
    setTarget(v,selectedMode);
    // Retriggers re-excite the envelopes but preserve phase/output continuity.
    // A parameter-only event never calls this method.
    active=gate=attacking=true;modEnv=pitchEnv=clickEnv=1;forced=control=age=0;
  }
  void off(){gate=false;}
  void stop(unsigned n){if(active){forced=forcedTotal=n?n:1;gate=false;}}
  void prepare(const MonoTables &tables){
    for(int i=0;i<FmParameters;++i)p[i]+=(target[i]-p[i])*smoothing;
    constexpr float frames=float(ControlFrames);
    const float dynamic=p[FmDynamics]*.01f,accent=(1-dynamic)+dynamic*(.3f+.7f*targetGain);
    const float duration=1+dynamic*(targetGain-.5f)*.55f;
    attack=1/(rate*clamp(p[FmAttack]*.001f,.0008f,15));
    decay=MonoDSP::exp2Fast(-6.64385619f/(rate*clamp(p[FmDecay]*.001f*duration,.01f,20)));
    release=MonoDSP::exp2Fast(-13.28771238f/(rate*clamp(p[FmRelease]*.001f,.005f,20)));
    modDecay=MonoDSP::exp2Fast(-6.64385619f/(rate*clamp(p[FmModDecay]*.001f*duration,.005f,20)));
    clickDecay=MonoDSP::exp2Fast(-6.64385619f/(rate*.006f));
    pitchDecay=MonoDSP::exp2Fast(-6.64385619f/(rate*clamp(p[FmSweep]*.001f,.005f,3)));
    glide=p[FmGlide]<.5f?1:1-MonoDSP::exp2Fast(-6.64385619f*frames/(rate*p[FmGlide]*.001f));
    frequency+=(targetHz-frequency)*glide;
    lfoPhase=wrap(lfoPhase+frames*p[FmRate]*.01f/rate);lfo=tables.sinCycle(lfoPhase);
    topology+=(mode-topology)*smoothing;
    float ratio=FmRatios[int(clamp(target[FmRatio],0,15))];
    portraitStride=unsigned(clamp(float(rate)/(frequency*PortraitPoints),1,64));
    bendSemitones=p[FmPunch]*pitchEnv*accent;
    const float pitch=frequency*MonoDSP::exp2Fast(bendSemitones/12);
    bandDelta=(clamp((rate*.48f-pitch)/(rate*.08f),0,1)-bandGain)/frames;
    const float detune=MonoDSP::exp2Fast((p[FmDetune]*.35f+lfo*p[FmMotion]*.035f)/1200);
    const float hz[]={pitch,pitch*ratio,pitch*detune,pitch*ratio/detune};
    for(int i=0;i<4;++i){float wanted=clamp(hz[i]/(rate*2),.0000001f,.45f);incrementDelta[i]=(wanted-increment[i])/(frames*2);}
    // Budget significant PM sidebands against output Nyquist, before 2x
    // rendering and FIR decimation. This is alias reduction, not a claim of
    // perfect bandlimiting for arbitrarily high feedback.
    float budget=clamp((rate*.42f-pitch)/(pitch*(ratio+1)*7),0,1);
    modHold=p[FmModHold]*.01f;
    float wanted=p[FmIndex]*.007f*accent*(modHold+(1-modHold)*modEnv)*(1+lfo*p[FmMotion]*.008f)*budget;
    indexDelta=(wanted-index)/(frames*2);
    feedbackDelta=(p[FmFeedback]*.00235f*budget-feedbackAmount)/(frames*2);
    float cutoff=160+14840*(p[FmTone]*.01f)*(p[FmTone]*.01f);
    float coefficient=1-MonoDSP::exp2Fast(-9.06472028f*clamp(cutoff,20,rate*.45f)/rate);
    toneDelta=(coefficient-tone)/frames;
    spread=p[FmSpread]*.01f;noiseLevel=p[FmNoise]*.0042f;clickLevel=p[FmClick]*.006f;
    sustain=p[FmBody]*.01f;modHold=p[FmModHold]*.01f;
  }
  void render(float &left,float &right,const MonoTables &tables){
    left=right=0;if(!active)return;
    if(!control){prepare(tables);control=ControlFrames;}--control;++age;
    gain+=(targetGain-gain)*(.0025f*32000/rate);
    if(gate){if(attacking){env+=attack;if(env>=1){env=1;attacking=false;}}else env=sustain+(env-sustain)*decay;}
    else env*=release;
    if(env<1e-15f&&!attacking)env=0;
    modEnv*=modDecay;pitchEnv*=pitchDecay;clickEnv*=clickDecay;
    if(modEnv<1e-15f)modEnv=0;if(pitchEnv<1e-15f)pitchEnv=0;if(clickEnv<1e-15f)clickEnv=0;
    if(!gate&&env<.00001f){active=false;return;}
    float a=0,b=0,ma=0,mb=0;
    const float n=noise();
    for(int os=0;os<2;++os){
      for(int i=0;i<4;++i){increment[i]+=incrementDelta[i];phase[i]=wrap(phase[i]+increment[i]);}
      index+=indexDelta;feedbackAmount+=feedbackDelta;
      mb=tables.sinCycle(wrap(phase[3]+feedback[1]*feedbackAmount));
      b=tables.sinCycle(wrap(phase[2]+mb*index*(.8f+.2f*topology)));
      ma=tables.sinCycle(wrap(phase[1]+feedback[0]*feedbackAmount+b*index*topology*.6f));
      a=tables.sinCycle(wrap(phase[0]+ma*index));
      feedback[0]=.5f*(feedback[0]+ma);feedback[1]=.5f*(feedback[1]+mb);
      float body=tables.sinCycle(phase[0])*.16f;
      float transient=n*(noiseLevel+clickLevel*clickEnv);
      float side=(a-b)*spread*.5f*bandGain,mid=((a+b)*.42f+body)*bandGain+transient;
      fir[0][firAt]=mid+side*.72f;fir[1][firAt]=mid-side*.72f;
      if(++firAt==17)firAt=0;
    }
    static constexpr unsigned taps[]={1,3,5,7,8,9,11,13,15};
    static constexpr float h[]={-.005239181f,.023211101f,-.076105845f,.307698779f,.500870292f,.307698779f,-.076105845f,.023211101f,-.005239181f};
    float amplitude=env*gain*.66f;
    if(forced){amplitude*=float(forced)/forcedTotal;if(!--forced)active=false;}
    tone+=toneDelta;bandGain+=bandDelta;
    float result[2];
    for(int ch=0;ch<2;++ch){float value=0;
      for(unsigned i=0;i<9;++i){unsigned at=firAt+16-taps[i];if(at>=17)at-=17;value+=h[i]*fir[ch][at];}
      low[ch]+=(value-low[ch])*tone;
      float clean=low[ch]-dcIn[ch]+dcCoefficient*dcOut[ch];dcIn[ch]=low[ch];dcOut[ch]=clean;
      if(fabsf(dcOut[ch])<1e-20f)dcOut[ch]=0;
      result[ch]=clamp(clean*amplitude,-.98f,.98f);
    }
    left=result[0];right=result[1];
    if(++portraitClock>=portraitStride){portraitClock=0;portrait[0][portraitAt][0]=ma;portrait[0][portraitAt][1]=a;
      portrait[1][portraitAt][0]=mb;portrait[1][portraitAt][1]=b;portraitAt=(portraitAt+1)%PortraitPoints;}
  }
};
}
