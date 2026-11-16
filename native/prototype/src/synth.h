#pragma once
#include "dsp_math.h"
#include "synth_defs.h"
#include "mono_dsp.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
// Original MOSS DSP. The algorithms are inspired by the instrument families,
// not component-accurate emulations or ports of their firmware.
struct SynthTables {
  MonoTables mono;
  float sine[2049], frequency[128];
  void init() {
    mono.init();
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
  bool active = false, gate = false;
  int kind = MonoInstrument, mode = 0, note = 48, remaining = 0;
  unsigned rate = 44100, control = 0, previewFrames = 0, forced = 0,
           forcedTotal = 0;
  float phase[4] = {}, previous[4] = {}, drift[4] = {}, driftTarget[4] = {};
  float lfo[2] = {}, params[8] = {}, target[8] = {}, env = 0, gain = 0,
        targetGain = 0, base = 130.8128f;
  float attackStep = .001f,
        releaseCoefficient = .999f;
  float fmDecay = 1, fmCoefficient = .999f, ratio = 1, dcIn = 0, dcOut = 0,
        dcCoefficient = .999f;
  float colorState = 0, colorCoefficient = .5f, filterEnv = 1;
  float fir[17] = {};
  unsigned firPos = 0, randomAt = 0;
  uint32_t random = 1;
  unsigned preparedRate = 0;
  float smoothing = 0, coefficientParams[8] = {};
  bool coefficientsReady = false;
  float fmSteps[4] = {}, fmBase = 0, fmRatio = 0, fmDetune = -1,
        fmBandLimit = 1;
  unsigned fmRate = 0;
  int fmMode = -1;
  static float bounded(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
  }
  static float saturate(float x) { return x / (1 + fabsf(x)); }
  static float blep(float p, float dt) {
    if (p < dt) {
      p /= dt;
      return p + p - p * p - 1;
    }
    if (p > 1 - dt) {
      p = (p - 1) / dt;
      return p * p + p + p + 1;
    }
    return 0;
  }
  static float saw(float p, float dt) { return 2 * p - 1 - blep(p, dt); }
  static float pulse(float p, float dt) {
    float second = p + .5f;
    if (second >= 1)
      second -= 1;
    return (p < .5f ? 1.f : -1.f) + blep(p, dt) - blep(second, dt);
  }
  float randomBipolar() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return float(random & 65535) / 32767.5f - 1;
  }
  void setTarget(const int *v, int selectedMode,int count=8) {
    if(kind==MonoInstrument){int values[SynthParameterCount];for(int i=0;i<SynthParameterCount;++i)values[i]=i<count?v[i]:int(monoVoice.target[i]);monoVoice.setTarget(values,selectedMode);}
    mode = selectedMode;
    for (int i = 0; i < 8; i++)
      target[i] = float(v[i]);
  }
  void on(int type, int midi, float hz, int velocity, int length, const int *v,
          int selectedMode, unsigned sampleRate, uint32_t seed,int count=8) {
    if(type==MonoInstrument){
      if(kind!=MonoInstrument||!active)monoVoice=MonoDSP{};
      int values[SynthParameterCount];for(int i=0;i<SynthParameterCount;++i)values[i]=i<count?v[i]:presetValues(MonoInstrument,0)[i];
      monoVoice.on(midi,hz,velocity,values,selectedMode,sampleRate,seed);
      kind=type;note=midi;base=hz;rate=sampleRate;remaining=length;mode=selectedMode;previewFrames=forced=0;active=gate=true;
      for(int i=0;i<8;++i)params[i]=target[i]=float(v[i]);
      return;
    }
    bool continuous =
        active && kind == OrganismInstrument && type == OrganismInstrument;
    if (!continuous) {
      *this = SynthVoice{};
      kind = type;
      random = seed ? seed : 1;
      phase[1] = .173f;
      phase[2] = .391f;
      phase[3] = .731f;
      for (int i = 0; i < 8; i++)
        params[i] = float(v[i]);
    }
    kind = type;
    note = midi;
    base = hz;
    targetGain = velocity / 127.f;
    remaining = length;
    rate = sampleRate;
    active = gate = true;
    forced = 0;
    previewFrames = 0;
    fmDecay = filterEnv = 1;
    control = 0;
    setTarget(v, selectedMode);
  }
  void off() { gate = false;if(kind==MonoInstrument)monoVoice.off(); }
  void stop(unsigned samples = 128) {
    if (active) {
      if(kind==MonoInstrument)monoVoice.stop(samples);
      forced = forcedTotal = samples;
      gate = false;
    }
  }
  void prepareRate() {
    if (preparedRate == rate)
      return;
    preparedRate = rate;
    smoothing = 1.f / (rate * .005f);
    dcCoefficient = expf(-50.26548246f / rate);
    coefficientsReady = false;
  }
  bool coefficientChanged(int slot) const {
    return !coefficientsReady || params[slot] != coefficientParams[slot];
  }
  void coefficients() {
    prepareRate();
    if (kind == OrganismInstrument) {
      if (coefficientChanged(6))
        attackStep = 1.f / (rate * fmaxf(.003f, params[6] / 1000));
      if (coefficientChanged(4))
        colorCoefficient =
            1 - expf(-6.2831853f * (100 + params[4] * params[4] * 1.2f) /
                     (rate * 2));
      if (coefficientChanged(7))
        releaseCoefficient =
            expf(-9.21034037f / (rate * fmaxf(.005f, params[7] / 1000)));
    } else {
      if (coefficientChanged(6))
        attackStep = 1.f / (rate * fmaxf(.003f, params[6] / 1000));
      if (coefficientChanged(3)) {
        releaseCoefficient =
            expf(-9.21034037f / (rate * fmaxf(.08f, params[3] / 2000)));
        fmCoefficient =
            expf(-4.605170186f / (rate * fmaxf(.05f, params[3] / 1000)));
      }
      const float ratios[] = {.5f, 1, 1.5f, 2, 3, 4, 5, 7};
      ratio = ratios[int(bounded(target[1], 0, 7))];
    }
    for (int i = 0; i < 8; i++)
      coefficientParams[i] = params[i];
    coefficientsReady = true;
  }
  void prepareFm() {
    if (fmBase == base && fmRatio == ratio && fmDetune == params[4] &&
        fmRate == rate && fmMode == mode)
      return;
    fmBase = base;
    fmRatio = ratio;
    fmDetune = params[4];
    fmRate = rate;
    fmMode = mode;
    fmBandLimit = bounded(float(rate) * .12f / (base * (ratio + 1)), .04f, 1);
    float frequency[] = {base, base * ratio, base * (mode ? ratio + 1 : 2.002f),
                         base * (mode ? .5f : ratio * 2.002f)};
    frequency[2] *= 1 + params[4] * .00059463f;
    frequency[3] *= 1 - params[4] * .00028f;
    for (int i = 0; i < 4; i++)
      fmSteps[i] = bounded(frequency[i] / (rate * 2), .000001f, .45f);
  }
  float organism(const SynthTables &t) {
    float tension = params[0] / 100, depth = params[1] / 100,
          chaos = params[2] / 100, density = params[3] / 100;
    lfo[0] += .073f / (rate * 2);
    lfo[1] += .119f / (rate * 2);
    for (float &v : lfo)
      if (v >= 1)
        v -= 1;
    float slow = t.sinCycle(lfo[0]) * .6f + t.sinCycle(lfo[1]) * .4f;
    if (!randomAt) {
      for (int i = 0; i < 4; i++)
        driftTarget[i] = randomBipolar();
      randomAt = rate / 2;
    }
    randomAt--;
    const float ratios[] = {1, 1, 1.5f, 2};
    float signal[4], sum = 0, weight = 0;
    for (int i = 0; i < 4; i++) {
      drift[i] += (driftTarget[i] - drift[i]) / (rate * .7f);
      int source = mode ? (i + 1) % 4 : (i ^ 1);
      float feedback = previous[source] * tension * .3f +
                       previous[(i + 2) % 4] * chaos * .2f;
      float detune =
          1 + depth * (drift[i] * .008f + slow * (i & 1 ? .004f : -.004f));
      float dt =
          bounded(base * ratios[i] * detune * (1 + feedback) / (rate * 2),
                  .000001f, .4f);
      float triangle = 1 - 4 * fabsf(phase[i] - .5f);
      signal[i] = triangle * (1 - tension * .65f) +
                  pulse(phase[i], dt) * tension * .65f;
      phase[i] += dt;
      if (phase[i] >= 1)
        phase[i] -= 1;
      float w = i == 0 ? 1 : bounded(density * 3 - i + 1, 0, 1);
      sum += signal[i] * w;
      weight += w;
    }
    for (int i = 0; i < 4; i++)
      previous[i] = signal[i];
    float raw = sum / weight + randomBipolar() * params[5] * .009f;
    colorState += (raw - colorState) * colorCoefficient;
    return saturate(colorState * (1 + chaos * 1.2f));
  }
  float fm(const SynthTables &t) {
    prepareFm();
    float index = params[0] / 100 * (fmDecay + (1 - fmDecay) * params[7] / 100),
          feedback = params[2] / 100;
    // Reduce modulation before the sidebands approach the output Nyquist limit.
    index *= fmBandLimit;
    float excitation = randomBipolar() * params[5] * .0015f;
    for (int i = 0; i < 4; i++) {
      phase[i] += fmSteps[i];
      if (phase[i] >= 1)
        phase[i] -= 1;
    }
    float m3 = t.sinCycle(phase[3] + previous[3] * feedback * .22f), m1, s;
    if (mode) {
      float m2 = t.sinCycle(phase[2] + m3 * index * .30f);
      m1 = t.sinCycle(phase[1] + m2 * index * .45f + excitation);
      s = t.sinCycle(phase[0] + m1 * index * .70f);
    } else {
      m1 = t.sinCycle(phase[1] + previous[1] * feedback * .22f + excitation);
      s = t.sinCycle(phase[0] + m1 * index * .70f) * .8f +
          t.sinCycle(phase[2] + m3 * index * .35f) * .2f;
    }
    previous[1] = m1;
    previous[3] = m3;
    return s * .65f;
  }
  void render(int &left, int &right, const SynthTables &t) {
    if (!active)
      return;
    if(kind==MonoInstrument){
      if(previewFrames&&! --previewFrames)off();
      int result=int(monoVoice.render(t.mono)*32767);left+=result;right+=result;
      active=monoVoice.active;gate=monoVoice.gate;env=monoVoice.env;gain=monoVoice.gain;filterEnv=monoVoice.filterEnv;
      return;
    }
    prepareRate();
    for (int i = 0; i < 8; i++)
      params[i] += (target[i] - params[i]) * smoothing;
    gain += (targetGain - gain) * smoothing;
    if (!control) {
      coefficients();
      control = 32;
    }
    control--;
    if (previewFrames && !--previewFrames)
      off();
    if (gate)
      env = minimum(1, env + attackStep);
    else
      env *= releaseCoefficient;
    if (kind == FmInstrument) {
      fmDecay *= fmCoefficient;
      if (fmDecay < 1e-10f)
        fmDecay = 0;
    }
    if (!gate && env < .00001f) {
      active = false;
      return;
    }
    for (int os = 0; os < 2; os++) {
      float sample = kind == OrganismInstrument ? organism(t) : fm(t);
      fir[firPos] = sample;
      if (++firPos == 17)
        firPos = 0;
    }
    // Windowed-sinc half-band decimation; fixed storage, same path on
    // desktop/ARM.
    static constexpr unsigned taps[] = {1, 3, 5, 7, 8, 9, 11, 13, 15};
    static constexpr float h[] = {-.005239181f, .023211101f, -.076105845f,
                                  .307698779f,  .500870292f, .307698779f,
                                  -.076105845f, .023211101f, -.005239181f};
    float sample = 0;
    for (unsigned i = 0; i < 9; i++) {
      unsigned at = firPos + 16 - taps[i];
      if (at >= 17)
        at -= 17;
      sample += h[i] * fir[at];
    }
    float clean = sample - dcIn + dcCoefficient * dcOut;
    dcIn = sample;
    dcOut = clean;
    float amplitude =
        env * gain *
        (kind == FmInstrument ? (fmDecay + (1 - fmDecay) * params[7] / 100)
                              : 1);
    if (forced) {
      amplitude *= float(forced) / forcedTotal;
      if (!--forced)
        active = false;
    }
    int result = int(bounded(clean * amplitude, -.98f, .98f) * 32767);
    left += result;
    right += result;
  }
};
} // namespace moss
