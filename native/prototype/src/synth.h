#pragma once
#include "dsp_math.h"
#include "synth_defs.h"
#include "mono_dsp.h"
#include "organism_dsp.h"
#include "fm_dsp.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
// Original MOSS DSP. The algorithms are inspired by the instrument families,
// not component-accurate emulations or ports of their firmware.
struct SynthTables {
  MonoTables mono;
  OrganismTables organism;
  float sine[2049], frequency[128];
  void init() {
    mono.init();
    organism.init();
    for (int i = 0; i <= 2048; i++)
      sine[i] = float(sin(6.283185307179586 * i / 2048));
    for (int i = 0; i < 128; i++)
      frequency[i] = float(440 * pow(2, (i - 69) / 12.0));
  }
  static float wrap(float p) {
    int k = int(p);
    if (p < k)
      --k;
    return p - k;
  }
  float sinCycle(float p) const {
    float index = wrap(p) * 2048;
    int i = int(index);
    float f = index - i;
    return sine[i] + (sine[i + 1] - sine[i]) * f;
  }
};
struct SynthVoice {
  MonoDSP monoVoice;
  OrganismDSP organismVoice;
  FmDSP fmVoice;
  bool active=false,gate=false;
  int kind=MonoInstrument,mode=0,note=48,remaining=0;
  unsigned rate=44100,previewFrames=0,forced=0;
  float params[8]={},target[8]={},env=0,gain=0,base=130.8128f,filterEnv=1;
  void setTarget(const int *v,int selectedMode,int count=8){
    int values[SynthParameterCount];
    for(int i=0;i<SynthParameterCount;++i)values[i]=i<count?v[i]:int(kind==MonoInstrument?monoVoice.target[i]:kind==OrganismInstrument?organismVoice.target[i]:fmVoice.target[i]);
    if(kind==MonoInstrument)monoVoice.setTarget(values,selectedMode);
    else if(kind==OrganismInstrument)organismVoice.setTarget(values,selectedMode);
    else fmVoice.setTarget(values,selectedMode);
    mode=selectedMode;for(int i=0;i<8;++i)target[i]=float(v[i]);
  }
  void on(int type,int midi,float hz,int velocity,int length,const int *v,int selectedMode,unsigned sampleRate,uint32_t seed,int count=8){
    int values[SynthParameterCount];for(int i=0;i<SynthParameterCount;++i)values[i]=i<count?v[i]:presetValues(type,0)[i];
    if(type==MonoInstrument){if(kind!=type||!active)monoVoice=MonoDSP{};monoVoice.on(midi,hz,velocity,values,selectedMode,sampleRate,seed);}
    else if(type==OrganismInstrument){if(kind!=type||!active)organismVoice=OrganismDSP{};organismVoice.on(midi,hz,velocity,values,selectedMode,sampleRate,seed);}
    else {if(kind!=type||!active)fmVoice=FmDSP{};fmVoice.on(midi,hz,velocity,values,selectedMode,sampleRate,seed);}
    kind=type;note=midi;base=hz;rate=sampleRate;remaining=length;mode=selectedMode;previewFrames=forced=0;active=gate=true;
    for(int i=0;i<8;++i)params[i]=target[i]=float(v[i]);
  }
  void off(){gate=false;if(kind==MonoInstrument)monoVoice.off();else if(kind==OrganismInstrument)organismVoice.off();else fmVoice.off();}
  void stop(unsigned n=128){if(active){if(kind==MonoInstrument)monoVoice.stop(n);else if(kind==OrganismInstrument)organismVoice.stop(n);else fmVoice.stop(n);forced=n;gate=false;}}
  void render(int &left,int &right,const SynthTables &t){
    if(!active)return;if(previewFrames&&!--previewFrames)off();
    if(kind==MonoInstrument){int result=int(monoVoice.render(t.mono)*32767);left+=result;right+=result;active=monoVoice.active;gate=monoVoice.gate;env=monoVoice.env;gain=monoVoice.gain;filterEnv=monoVoice.filterEnv;}
    else if(kind==OrganismInstrument){float l,r;organismVoice.render(l,r,t.organism,t.mono);left+=int(l*32767);right+=int(r*32767);active=organismVoice.active;gate=organismVoice.gate;env=organismVoice.env;gain=organismVoice.gain;}
    else {float l,r;fmVoice.render(l,r,t.mono);left+=int(l*32767);right+=int(r*32767);active=fmVoice.active;gate=fmVoice.gate;env=fmVoice.env;gain=fmVoice.gain;}
  }
};
} // namespace moss
