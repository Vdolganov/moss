#pragma once
#include "mono_dsp.h"
#include "organism_defs.h"
namespace moss {
// Independent MOSS voice. Four bounded, mutually phase-modulated carriers;
// an uncoupled sub remains centred. No external synth implementation is used.
struct OrganismTables {
  static constexpr unsigned Size=1024, Levels=8;
  int16_t waves[3][Levels][Size+1]={};
  float integral[1025]={};
  bool ready=false;
  void init() {
    if(ready)return;
    for(unsigned i=0;i<=1024;++i)integral[i]=float(log(cosh(i/256.0)));
    for(unsigned level=0;level<Levels;++level) {
      const unsigned harmonics=128u>>level;
      for(unsigned i=0;i<Size;++i) {
        double tri=0,saw=0,square=0,angle=6.283185307179586*i/Size;
        for(unsigned n=1;n<=harmonics;++n) {
          double s=sin(angle*n);
          saw+=s/n;
          if(n&1u) {square+=s/n;tri+=s*((n&2u)?-1:1)/(n*n);}
        }
        // Fundamental has equal amplitude in all tables; common scaling
        // leaves headroom for the Gibbs peak in the saw/square families.
        waves[0][level][i]=int16_t(tri*16000);
        waves[1][level][i]=int16_t(saw*16000);
        waves[2][level][i]=int16_t(square*16000);
      }
      for(int family=0;family<3;++family)waves[family][level][Size]=waves[family][level][0];
    }
    ready=true;
  }
  float read(int family,int lo,int hi,float blend,unsigned at,float fraction,float sine) const {
    if(!family)return sine;
    const int index=family-1;
    float a=waves[index][lo][at],b=waves[index][lo][at+1];
    float low=a+(b-a)*fraction;
    if(lo==hi)return low*(1.f/16000);
    a=waves[index][hi][at];b=waves[index][hi][at+1];
    return (low+(a+(b-a)*fraction-low)*blend)*(1.f/16000);
  }
  float primitive(float x) const {
    float a=fabsf(x)*256;if(a>=1024)return integral[1024]+fabsf(x)-4;
    unsigned at=unsigned(a);return integral[at]+(integral[at+1]-integral[at])*(a-at);
  }
  // First-order antiderivative antialiasing. The midpoint fallback avoids
  // cancellation at small differences; it also bounds LUT interpolation error.
  float saturate(float x,float &previous,const MonoTables &common) const {
    float delta=x-previous;
    float result=fabsf(delta)<.02f?common.saturate((x+previous)*.5f):(primitive(x)-primitive(previous))/delta;
    previous=x;return result;
  }
};
struct OrganismDSP {
  static constexpr unsigned Voices=4, ControlFrames=64;
  float p[OrganismParameters]={},target[OrganismParameters]={};
  float phase[Voices]={0,.013f,.31f,.327f},step[Voices]={},stepDelta[Voices]={};
  float previous[Voices]={},feedback[Voices]={};
  int bandLow[Voices]={},bandHigh[Voices]={},shapeA=0,shapeB=1;
  float bandMix[Voices]={},morph=0;
  float bodyHarmonic[Voices]={};
  float slowPhase[Voices]={0,.31f,.57f,.83f},motion[Voices]={};
  float drift[Voices]={},driftTarget[Voices]={},weights[Voices]={};
  float driftCents[Voices]={},pan[Voices]={},energy[Voices]={};
  float cutoff[2]={},svf1[2]={},svf2[2]={},a1[2]={},a2[2]={},a3[2]={};
  float dcIn[2]={},dcOut[2]={},saturationIn[2]={},saturationOut[2]={};
  float subPhase=0,frequency=55,targetHz=55,env=0,gain=0,targetGain=0;
  float smoothing=0,attack=0,release=0,glide=1,dcCoefficient=0;
  float shape=0,couple=0,drive=1,driveGain=1,mixGain=1,subGain=0;
  float noiseLow=0,noiseMid=0,noiseSlowCoefficient=0,noiseMidCoefficient=0;
  float gritState=0,gritTarget=0,gritCoefficient=0,noiseLevel=0,noiseTone=0;
  float gritAmount=0,topology=0,peak=0;
  float inverseRate=1.f/32000;
  float carrierBus[2]={},lastCarrierBus[2]={};bool carrierTick=false;
  unsigned rate=32000,control=0,driftClock=0,gritClock=0;
  unsigned forced=0,forcedTotal=1;
  uint32_t random=1;int note=33,mode=0;bool active=false,gate=false;
  static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
  static float wrap(float x){if(x<0)x+=1;if(x>=1)x-=1;return x;}
  float noise(){random^=random<<13;random^=random>>17;random^=random<<5;return float(random&65535)/32767.5f-1;}
  void setTarget(const int *values,int selectedMode) {
    for(int i=0;i<OrganismParameters;++i)target[i]=float(values[i]);
    mode=selectedMode;
  }
  void on(int midi,float hz,int velocity,const int *values,int selectedMode,unsigned sampleRate,uint32_t seed) {
    bool continuing=active&&rate==sampleRate;
    if(!continuing) {
      *this=OrganismDSP{};random=seed?seed:1;rate=sampleRate;frequency=hz;
      inverseRate=1.f/rate;smoothing=1-expf(-float(ControlFrames)/(rate*.018f));
      dcCoefficient=expf(-6.28318530718f*3/rate);
      noiseSlowCoefficient=1-expf(-6.28318530718f*100/rate);
      noiseMidCoefficient=1-expf(-6.28318530718f*1400/rate);
      gritCoefficient=1-expf(-1.f/(rate*.002f));
      for(int i=0;i<OrganismParameters;++i)p[i]=float(values[i]);
      for(unsigned i=0;i<Voices;++i)slowPhase[i]=wrap(slowPhase[i]+noise()*.07f);
    }
    note=midi;targetHz=hz;targetGain=velocity/127.f;setTarget(values,selectedMode);
    active=gate=true;forced=control=0;
  }
  void off(){gate=false;}
  void stop(unsigned n){if(active){forced=forcedTotal=n?n:1;gate=false;}}
  void prepare(const MonoTables &tables) {
    constexpr float dt=float(ControlFrames);
    for(int i=0;i<OrganismParameters;++i)p[i]+=(target[i]-p[i])*smoothing;
    attack=1/(rate*clamp(p[OrgAttack]*.001f,.003f,15));
    release=MonoDSP::exp2Fast(-13.28771238f/(rate*clamp(p[OrgRelease]*.001f,.005f,20)));
    glide=p[OrgGlide]<.5f?1:1-MonoDSP::exp2Fast(-6.64385619f*dt/(rate*p[OrgGlide]*.001f));
    frequency+=(targetHz-frequency)*glide;
    topology+=(mode-topology)*smoothing;
    if(!driftClock) {
      for(unsigned i=0;i<Voices;++i)driftTarget[i]=noise();
      driftClock=unsigned(rate*(.35f+1.1f/(.2f+p[OrgRate]*.01f)));
    }
    driftClock=driftClock>ControlFrames?driftClock-ControlFrames:0;
    float driftFollow=clamp(dt/(rate*.7f),0,1);
    float spread=p[OrgSpread]*.01f,density=p[OrgDensity]*.03f;
    const float positions[]={0,-.8f,.75f,.3f};
    const float detunes[]={0,1,-.65f,.65f};
    float weightSum=0;
    for(unsigned i=0;i<Voices;++i) {
      slowPhase[i]=wrap(slowPhase[i]+dt*p[OrgRate]*.01f*(1+i*.173f)/rate);
      motion[i]=tables.sinCycle(slowPhase[i]);
      drift[i]+=(driftTarget[i]-drift[i])*driftFollow;
      driftCents[i]=(drift[i]*.75f+motion[i]*.25f)*p[OrgDrift];
      float semitones=i<2?0:p[OrgInterval];
      float hz=frequency*MonoDSP::exp2Fast(semitones/12+(driftCents[i]+detunes[i]*p[OrgDetune])/1200);
      float nextStep=clamp(hz*inverseRate*2,.000001f,.4f);
      bool initial=step[i]==0;
      if(initial)step[i]=nextStep;
      stepDelta[i]=(nextStep-step[i])/dt;
      weights[i]=(i==0?.52f:clamp(density-i+1,0,1)*.24f)*(1-p[OrgBreath]*.0035f*(1+motion[i]));
      pan[i]=positions[i]*spread;weightSum+=i?weights[i]:0;
      // Guard PM sidebands as well as the unmodulated waveform. Crossfade
      // adjacent harmonic budgets, so crossing an octave never clicks.
      float allowed=rate*.16f/(hz*(1+p[OrgCouple]*.05f+p[OrgGrit]*.015f));
      float index=0,h=128;
      while(index<7&&allowed<h*.5f){h*=.5f;index+=1;}
      float nextBand=clamp(index+2-2*allowed/h,0,7);
      int selected=int(nextBand);if(nextBand>selected)++selected;
      if(initial){bandLow[i]=bandHigh[i]=selected;bandMix[i]=1;}
      else if(selected!=bandHigh[i]){if(bandMix[i]>.5f)bandLow[i]=bandHigh[i];bandHigh[i]=selected;bandMix[i]=0;}
      bandMix[i]=clamp(bandMix[i]+dt*inverseRate*125,0,1);
      if(bandMix[i]>=1)bandLow[i]=bandHigh[i];
    }
    shape=clamp(p[OrgShape]*.03f+motion[1]*p[OrgWander]*.0024f,0,3);
    shapeA=int(shape);shapeB=shapeA<3?shapeA+1:3;morph=shape-shapeA;
    for(unsigned i=2;i<Voices;++i)bodyHarmonic[i]=shape*(1.f/27)*clamp((.45f-step[i]*3)*20,0,1);
    couple=p[OrgCouple]*.00135f*(1+motion[2]*p[OrgWander]*.004f);
    subGain=p[OrgSub]*.007f;mixGain=1/(1+weightSum*.15f+subGain*.2f);
    drive=1+p[OrgDrive]*.035f;driveGain=1/(1+p[OrgDrive]*.009f);
    noiseLevel=p[OrgNoise]*.003f;noiseTone=p[OrgNoiseTone]*.01f;
    gritAmount=p[OrgGrit]*.001f;
    float k=2-p[OrgResonance]*.017f;
    float centerSway=motion[1]*.8f+drift[2]*.2f;
    for(int ch=0;ch<2;++ch) {
      float sideSway=motion[ch+1]*.8f+drift[ch+2]*.2f;
      float sway=(centerSway+(sideSway-centerSway)*spread)*p[OrgWander]*.018f;
      cutoff[ch]=clamp(p[OrgCutoff]*MonoDSP::exp2Fast(sway),30,rate*.35f);
      float G=tables.coefficient(cutoff[ch]/(rate*2)),g=G/(1-G);
      g=2*g/(1-g*g); // double-angle tangent: base-rate TPT from the shared LUT
      a1[ch]=1/(1+g*(g+k));a2[ch]=g*a1[ch];a3[ch]=g*a2[ch];
    }
  }
  float filter(float input,int ch) {
    float v3=input-svf2[ch];
    float v1=a1[ch]*svf1[ch]+a2[ch]*v3;
    float v2=svf2[ch]+a2[ch]*svf1[ch]+a3[ch]*v3;
    svf1[ch]=clamp(2*v1-svf1[ch],-8,8);
    svf2[ch]=clamp(2*v2-svf2[ch],-8,8);
    if(fabsf(svf1[ch])<1e-20f)svf1[ch]=0;
    if(fabsf(svf2[ch])<1e-20f)svf2[ch]=0;
    return v2;
  }
  void render(float &left,float &right,const OrganismTables &waves,const MonoTables &common) {
    left=right=0;if(!active)return;
    if(!control){prepare(common);control=ControlFrames;}--control;
    gain+=(targetGain-gain)*(inverseRate*250);
    if(gate)env=clamp(env+attack,0,1);else env*=release;
    if(!gate&&env<.00001f){active=false;return;}
    for(unsigned i=0;i<Voices;++i)step[i]+=stepDelta[i];
    float result[2];{
      float air=0,white=0;
      if(noiseLevel>.00001f||gritAmount>.00001f) {
        white=noise();noiseLow+=(white-noiseLow)*noiseSlowCoefficient;
        noiseMid+=(white-noiseMid)*noiseMidCoefficient;
        float dark=noiseLow*4,soft=noiseMid*1.4f;
        air=noiseTone<.5f?dark+(soft-dark)*(noiseTone*2):soft+(white-soft)*(noiseTone*2-1);
      }
      if(!gritClock){gritTarget=noise();gritClock=unsigned(rate*.008f);}
      --gritClock;gritState+=(gritTarget-gritState)*gritCoefficient;
      if(!carrierTick){
      float bus[2]={},next[Voices]={};
      for(unsigned i=0;i<Voices;++i) {
        float source=previous[i^1]+(previous[(i+1)%Voices]-previous[i^1])*topology;
        feedback[i]+=(source-feedback[i])*.35f;
        float pm=feedback[i]*couple+(air*.22f+gritState*.3f)*gritAmount;
        pm*=clamp(.09f/(step[i]+.002f),.05f,1);
        float pos=wrap(phase[i]+pm);
        float tablePos=pos*1024;unsigned at=unsigned(tablePos);float fraction=tablePos-at;
        float carrier=common.sine[at]+(common.sine[at+1]-common.sine[at])*fraction;
        float x=carrier;
        if(i<2){
          float a=waves.read(shapeA,bandLow[i],bandHigh[i],bandMix[i],at,fraction,carrier);
          x=morph<.00001f?a:a+(waves.read(shapeB,bandLow[i],bandHigh[i],bandMix[i],at,fraction,carrier)-a)*morph;
        }else{
          // Two darker body carriers anchor the bright morph pair. Their
          // rounded triangle adds only a Nyquist-guarded third harmonic.
          x-=common.sinCycle(pos*3)*bodyHarmonic[i];
        }
        next[i]=carrier;
        phase[i]=wrap(phase[i]+step[i]);
        x*=weights[i]*(1+gritState*gritAmount*.5f);
        energy[i]+=(fabsf(x)-energy[i])*.004f;
        bus[0]+=x*(1-pan[i]*.65f);bus[1]+=x*(1+pan[i]*.65f);
      }
      memcpy(previous,next,sizeof(previous));
      for(int ch=0;ch<2;++ch){lastCarrierBus[ch]=carrierBus[ch];carrierBus[ch]=bus[ch];}
      }
      float sub=common.sinCycle(subPhase)*subGain;
      subPhase=wrap(subPhase+frequency*inverseRate*.5f);
      for(int ch=0;ch<2;++ch) {
        // Four band-limited drone carriers run at half rate, with linear
        // reconstruction. Sub, noise, envelopes and colour remain full-rate.
        float bus=carrierTick?carrierBus[ch]:(carrierBus[ch]+lastCarrierBus[ch])*.5f;
        float input=(bus+air*noiseLevel)*mixGain*drive;
        float x=filter(waves.saturate(input,saturationIn[ch],common),ch);
        result[ch]=waves.saturate((x*driveGain+sub*mixGain)*1.18f,saturationOut[ch],common);
      }
      carrierTick=!carrierTick;
    }
    float amplitude=env*gain;
    if(forced){amplitude*=float(forced)/forcedTotal;if(! --forced)active=false;}
    for(int ch=0;ch<2;++ch) {
      float x=result[ch];
      float clean=x-dcIn[ch]+dcCoefficient*dcOut[ch];dcIn[ch]=x;dcOut[ch]=clean;
      result[ch]=clamp(clean*amplitude,-.98f,.98f);
    }
    left=result[0];right=result[1];peak=fmaxf(fabsf(left),fabsf(right));
  }
};
} // namespace moss
