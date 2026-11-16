#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
// Original MOSS voice: bandlimited oscillators and a four-stage TPT cascade
// with an implicit, saturating feedback input. Not a circuit emulation.
struct MonoTables {
  float soft[1025]={},tangent[1025]={},sine[1025]={};
  void init(){for(int i=0;i<=1024;++i){soft[i]=tanhf(i/256.f);tangent[i]=tanf(3.14159265359f*.225f*i/1024);sine[i]=sinf(6.28318530718f*i/1024);}}
  float saturate(float x) const {
    float a=fabsf(x)*256;if(a>=1024)return x<0?-1.f:1.f;
    unsigned i=unsigned(a);float y=soft[i]+(soft[i+1]-soft[i])*(a-i);return x<0?-y:y;
  }
  float coefficient(float normalized) const {
    float x=normalized*(1024/.225f);if(x>1023.999f)x=1023.999f;
    unsigned i=unsigned(x);float g=tangent[i]+(tangent[i+1]-tangent[i])*(x-i);return g/(1+g);
  }
  float sinCycle(float phase) const {float x=phase*1024;unsigned i=unsigned(x)&1023;return sine[i]+(sine[i+1]-sine[i])*(x-int(x));}
};
struct MonoDSP {
  static constexpr int Parameters=20,ScopePoints=64;
  float p[Parameters]={},target[Parameters]={};
  float phase=0,secondPhase=.313f,subPhase=0,lfoPhase=0,lfo=0;
  float frequency=130.8128f,targetHz=130.8128f,step=0,secondStep=0,stepDelta=0,secondDelta=0,widthDelta=0;
  float state[4]={},u=0,G=.05f,gTarget=.05f,gDelta=0,resonance=0;
  float env=0,filterEnv=0,gain=0,targetGain=0,cutoff=1200,width=.5f;
  float attack=0,decay=0,release=0,filterDecay=0,glide=1,smoothing=0;
  float drift=0,driftTarget=0,drive=1,outputGain=1,mixNorm=1,secondAmount=0;
  float dcIn=0,dcOut=0,dcCoefficient=0,fir[17]={},scope[ScopePoints]={};
  unsigned rate=32000,control=0,driftClock=0,firAt=0,scopeAt=0,scopeClock=0;
  unsigned preview=0,forced=0,forcedTotal=1;
  uint32_t random=1;int note=48,mode=0;bool active=false,gate=false,attacking=true,filterAttacking=true;
  static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
  static float exp2Fast(float x){
    x=clamp(x,-30,30);int whole=int(x+(x<0?-.5f:.5f));float f=x-whole;
    float y=1+f*(.69314718056f+f*(.24022650696f+f*(.05550410866f+f*(.00961812911f+f*(.00133335581f+f*.0001540353f)))));
    uint32_t bits=uint32_t(whole+127)<<23;float scale;memcpy(&scale,&bits,4);return scale*y;
  }
  static float blep(float x,float dt){if(x<dt){x/=dt;return 2*x-x*x-1;}if(x>1-dt){x=(x-1)/dt;return x*x+2*x+1;}return 0;}
  static float oscillator(float x,float dt,float pw,int selectedMode){
    if(!selectedMode)return 2*x-1-blep(x,dt);
    float edge=x-pw;if(edge<0)edge+=1;
    return (x<pw?1.f:-1.f)-(2*pw-1)+blep(x,dt)-blep(edge,dt);
  }
  float noise(){random^=random<<13;random^=random>>17;random^=random<<5;return float(random&65535)/32767.5f-1;}
  void setTarget(const int *values,int selectedMode){for(int i=0;i<Parameters;++i)target[i]=float(values[i]);mode=selectedMode;}
  void on(int midi,float hz,int velocity,const int *values,int selectedMode,unsigned sampleRate,uint32_t seed){
    bool continuing=active&&rate==sampleRate;
    bool legato=continuing&&gate&&values[10]>0;
    if(!continuing){*this=MonoDSP{};random=seed?seed:1;rate=sampleRate;frequency=hz;
      smoothing=1-expf(-16.f/(rate*.008f));dcCoefficient=expf(-6.28318530718f*3/rate);
      for(int i=0;i<Parameters;++i)p[i]=float(values[i]);}
    note=midi;targetHz=hz;targetGain=velocity/127.f;setTarget(values,selectedMode);
    if(!legato){attacking=filterAttacking=true;filterEnv=0;}
    gate=active=true;forced=preview=0;control=0;
  }
  void off(){gate=false;}
  void stop(unsigned n){if(active){forced=forcedTotal=n?n:1;gate=false;}}
  void prepare(const MonoTables &tables){
    for(int i=0;i<Parameters;++i)p[i]+=(target[i]-p[i])*smoothing;
    attack=1/(rate*clamp(p[2]*.001f,.003f,5));
    decay=exp2Fast(-6.64385619f/(rate*clamp(p[12]*.001f,.01f,8)));
    release=exp2Fast(-13.28771238f/(rate*clamp(p[3]*.001f,.005f,10)));
    filterDecay=exp2Fast(-6.64385619f/(rate*clamp(p[14]*.001f,.01f,8)));
    glide=p[10]<.5f?1:1-exp2Fast(-6.64385619f/(rate*p[10]*.001f));
    if(!driftClock){driftTarget=noise();driftClock=rate/4;}
    drift+=(driftTarget-drift)*.002f;driftClock=driftClock>16?driftClock-16:0;
    lfoPhase+=16*p[16]*.01f/rate;if(lfoPhase>=1)lfoPhase-=1;lfo=tables.sinCycle(lfoPhase);
    float pitch=(drift*p[11]*.07f+lfo*p[19])/1200;
    float nextStep=clamp(frequency*exp2Fast(pitch)/(rate*2),.000001f,.4f);
    stepDelta=(nextStep-step)/16;secondDelta=(clamp(nextStep*exp2Fast(p[9]/1200),.000001f,.4f)-secondStep)/16;
    widthDelta=(clamp(p[8]*.01f+lfo*p[17]*.004f,.05f,.95f)-width)/16;
    float octaves=p[4]*.06f*filterEnv+lfo*p[18]*.04f+(note-48)*p[15]/1200;
    cutoff=clamp(p[0]*exp2Fast(octaves),20,rate*.4f);
    gTarget=tables.coefficient(cutoff/(rate*2));gDelta=(gTarget-G)/16;
    resonance=clamp(p[1]*.0397f,0,3.97f);
    drive=1+p[7]*.045f;outputGain=.95f*(1+resonance*.8f)/(1+p[7]*.008f);
    secondAmount=clamp(p[9]/3,0,1)*.3f;
    mixNorm=1/(1+secondAmount*.35f+p[5]*.003f+p[6]*.001f);
  }
  float ladder(float input,const MonoTables &tables){
    float g2=G*G,g3=g2*G,g4=g3*G;
    float history=(1-G)*(g3*state[0]+g2*state[1]+G*state[2]+state[3]);
    float driveIn=input*drive*.75f;
    // Two bounded Newton corrections solve u=tanh(input-k*(G^4*u+history)).
    // A shared LUT avoids transcendentals inside the oversampled loop.
    for(int j=0;j<2;++j){float y=tables.saturate(driveIn-resonance*(g4*u+history));u-=(u-y)/(1+resonance*g4*(1-y*y));}
    u=clamp(u,-1,1);float y=u;
    for(int i=0;i<4;++i){float v=(y-state[i])*G;y=v+state[i];state[i]=y+v;if(fabsf(state[i])<1e-20f)state[i]=0;}
    return tables.saturate(y*outputGain);
  }
  float render(const MonoTables &tables){
    if(!active)return 0;
    frequency+=(targetHz-frequency)*glide;
    if(!control){prepare(tables);control=16;}--control;G+=gDelta;step+=stepDelta;secondStep+=secondDelta;width+=widthDelta;
    if(preview&&! --preview)off();
    gain+=(targetGain-gain)*(1.f/(rate*.004f));
    if(gate){if(attacking){env+=attack;if(env>=1){env=1;attacking=false;}}
      else {float sustain=p[13]*.01f;env=sustain+(env-sustain)*decay;}
      if(filterAttacking){filterEnv+=attack;if(filterEnv>=1){filterEnv=1;filterAttacking=false;}}
      else filterEnv*=filterDecay;
    }else{env*=release;filterEnv*=release;}
    if(!gate&&env<.00001f){active=false;return 0;}
    for(int os=0;os<2;++os){
      float x=oscillator(phase,step,width,mode);
      if(secondAmount>.0001f)x+=secondAmount*oscillator(secondPhase,secondStep,width,mode);
      x+=tables.sinCycle(subPhase)*p[5]*.012f;
      if(p[6]>.0001f)x+=noise()*p[6]*.004f;
      phase+=step;if(phase>=1)phase-=1;secondPhase+=secondStep;if(secondPhase>=1)secondPhase-=1;subPhase+=step*.5f;if(subPhase>=1)subPhase-=1;
      fir[firAt]=ladder(x*mixNorm,tables);if(++firAt==17)firAt=0;
    }
    static constexpr unsigned taps[]={1,3,5,7,8,9,11,13,15};
    static constexpr float h[]={-.005239181f,.023211101f,-.076105845f,.307698779f,.500870292f,.307698779f,-.076105845f,.023211101f,-.005239181f};
    float x=0;for(unsigned i=0;i<9;++i){unsigned at=firAt+16-taps[i];if(at>=17)at-=17;x+=h[i]*fir[at];}
    float clean=x-dcIn+dcCoefficient*dcOut;dcIn=x;dcOut=clean;
    float amplitude=env*gain;
    if(forced){amplitude*=float(forced)/forcedTotal;if(! --forced)active=false;}
    float out=clamp(clean*amplitude,-.98f,.98f);
    if(++scopeClock==16){scopeClock=0;scope[scopeAt]=out;scopeAt=(scopeAt+1)%ScopePoints;}
    return out;
  }
};
} // namespace moss
