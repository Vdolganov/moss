#define MOSS_REEL_MOTION_LAB 1
#include "../src/effects.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
using namespace moss;
static ReelMotion tape, other;
static FxState rack;
static uint32_t seed = 5293;
static float randomAudio() { seed = seed * 1664525u + 1013904223u; return int32_t(seed) / 1073741824.f; }
static void neutral() {
  const unsigned rates[] = {8000u, 32000u, 44100u, 48000u, 96000u};
  for (unsigned rate : rates) {
    tape.reset(rate); tape.prepare({}, rate);
    float inputs[4096] = {};
    for (unsigned i = 0; i < rate; ++i) {
      float input = randomAudio(), l = input, r = -input;
      inputs[i & 4095] = input; tape.process(l, r);
      float expected = i < tape.latency ? 0 : inputs[(i - tape.latency) & 4095];
      if(l != expected || r != -expected) {
        printf("neutral mismatch rate=%u frame=%u delay=%f depth=%g/%g L=%g R=%g expected=%g\n",
               rate,i,double(tape.position),double(tape.mod[0].amplitude),double(tape.mod[1].amplitude),
               double(l),double(r),double(expected));fflush(stdout);
      }
      assert(l == expected && r == -expected);
    }
    tape.reset(rate); tape.prepare({15,.1f,15,30,1}, rate);
    for (unsigned i = 0; i < rate; ++i) {
      float l = 0, r = 0; tape.process(l,r); assert(l == 0 && r == 0);
    }
  }
  puts("PASS clean path: exact delayed float input including >0 dBFS; no stale history, silence at five rates");
}
static void interpolation() {
  tape.reset(32000); tape.written = ReelMotion::Size;
  float worst = 0;
  for (unsigned band = 1; band <= 120; ++band) {
    double omega = band * 100 * 6.283185307179586 / 32000;
    for (unsigned i = 0; i < ReelMotion::Size; ++i) {
      tape.history[(-i) & ReelMotion::Mask][0] = float(cos(omega * i));
      tape.history[(-i) & ReelMotion::Mask][1] = float(sin(omega * i));
    }
    for (unsigned phase = 0; phase < 1024; ++phase) {
      float l, r; tape.read(512 + phase / 1024.f, l, r);
      float db = fabsf(10 * log10f(l*l + r*r));
      if (db > worst) worst = db;
    }
  }
  assert(worst < .15f);
  printf("PASS fractional read: worst 100..12000 Hz error %.6f dB over 1024 phases\n", double(worst));
  // Independent scalar oracle also exercises the native NEON path and ring
  // seams with unrelated stereo values, rather than only symmetric sine/DC.
  for(unsigned i=0;i<ReelMotion::Size;++i) {
    tape.history[i][0]=randomAudio();tape.history[i][1]=randomAudio();
  }
  float maximumError=0;
  for(unsigned n=0;n<30000;++n) {
    tape.cursor=(n*251)&ReelMotion::Mask;
    float delay=10+(n%3000)+((n*71)%1024)/1024.f;
    unsigned whole=unsigned(delay);float phase=(delay-whole)*256;
    unsigned cell=unsigned(phase);float fraction=phase-cell;
    float expected[2]={};
    for(unsigned k=0;k<12;++k) {
      float coefficient=ReelSinc[cell][k]+fraction*(ReelSinc[cell+1][k]-ReelSinc[cell][k]);
      for(unsigned ch=0;ch<2;++ch)
        expected[ch]+=coefficient*tape.history[(tape.cursor-whole-6+k)&ReelMotion::Mask][ch];
    }
    float l,r;tape.read(delay,l,r);
    float error=maximum(fabsf(l-expected[0]),fabsf(r-expected[1]));
    maximumError=maximum(maximumError,error);assert(error<.000002f);
  }
  printf("PASS 30000 fractional/ring/stereo comparisons to scalar oracle: max error %.9f\n",double(maximumError));
}
static void movement() {
  const unsigned rates[] = {8000u, 32000u, 44100u, 96000u};
  for (unsigned rate : rates) {
    tape.reset(rate); other.reset(rate);
    for (unsigned i = 0; i < rate * 4; ++i) {
      if (!(i % 257)) {
        bool on = (i / 257) & 1;
        ReelMotionParams p{on ? 15.f : 0.f, on ? .1f : 4.f,
                           on ? 15.f : 0.f, on ? 30.f : 2.f, 1};
        tape.prepare(p, rate); other.prepare(p, rate);
      }
      // Extra prepare calls must not change samples (callback partitioning).
      if (!(i % 113)) {
        bool on = (i / 257) & 1;
        other.prepare({on ? 15.f : 0.f, on ? .1f : 4.f,
                       on ? 15.f : 0.f, on ? 30.f : 2.f, 1}, rate);
      }
      float previous = tape.position;
      float l = 1.5f, r = -1.5f, l2=l, r2=r;
      tape.process(l,r); other.process(l2,r2);
      assert(l == l2 && r == r2);
      assert(fabsf(tape.position - previous) < ReelMotion::MaxDelaySlew + .0002f);
      assert(tape.position > 6 && tape.position < ReelMotion::Size - 7);
      assert(isfinite(l) && l == -r);
      if (i > ReelMotion::Size) assert(fabsf(l - 1.5f) < .000002f);
    }
  }
  // Independent frequency controls preserve the depth of each oscillator.
  for (int oscillator = 0; oscillator < 2; ++oscillator) {
    const float frequencies[] = {.1f,.3f,2.f,4.f,6.f,30.f};
    for (float hz : frequencies) {
      if ((oscillator == 0 && hz > 4) || (oscillator == 1 && hz < 2)) continue;
      tape.reset(32000);
      tape.prepare({oscillator ? 0.f : 10.f, oscillator ? .3f : hz,
                    oscillator ? 10.f : 0.f, oscillator ? hz : 6.f, 1},32000);
      float peak = 0;
      for (unsigned i=0; i<32000*24; ++i) {
        float previous=tape.position, l=0,r=0; tape.process(l,r);
        if (i>32000*3) {
          float speed=fabsf(tape.position-previous);
          if(speed>peak) peak=speed;
        }
      }
      float cents=1200*log2f(1+peak);
      assert(cents>9.6f && cents<10.4f);
      printf("PASS %s %.2f Hz: measured peak %.4f cents\n",oscillator?"flutter":"wow",double(hz),double(cents));
    }
  }
  puts("PASS motion: rate/depth extrema, rapid locks, DC/headroom, stereo, block invariance, bounded head velocity");
}
static void bypass() {
  Effect e; e.select(Reel); e.values[Reel][0]=150; e.values[Reel][1]=10;
  e.values[Reel][2]=150; e.values[Reel][3]=100; e.values[Reel][4]=300;
  rack.reset();
  for (unsigned i=0;i<32000*4;++i) {
    if (!(i%257)) {
      e.enable((i/257)&1); e.values[Reel][1]=(i/257)%2 ? 10:400;
      rack.prepare(e.values,32000,120);
    }
    int l=16384,r=-16384; rack.processChain(l,r,e,e.values,32000,120);
    if(i>4096) assert(abs(l-16384)<=1 && r == -l);
    if(!e.enabled) assert(rack.reelMotion.mod[0].targetAmplitude == 0);
  }
  // Reusing a formerly enabled SONG track with identical controls but bypassed
  // REEL must not retain a cached nonzero motion target after invalidation.
  e.enable(true);rack.prepare(e.values,32000,120);
  {int l=10000,r=5000;rack.processChain(l,r,e,e.values,32000,120);}
  assert(rack.reelMotion.mod[0].targetAmplitude>0);
  rack.invalidate();e.enable(false);rack.prepare(e.values,32000,120);
  assert(rack.reelMotion.mod[0].targetAmplitude==0);
  for(unsigned i=0;i<32000;++i) {
    int l=0,r=0;rack.processChain(l,r,e,e.values,32000,120);assert(l==0 && r==0);
  }
  printf("PASS full rack: bypass/parameter changes retain aligned DC; reset silence; state %u bytes\n",unsigned(sizeof(ReelMotion)));
  tape.reset(32000);tape.prepare({15,.1f,15,2,1},32000);
  float inputs[4096]={};
  for(unsigned i=0;i<32000*4;++i) {
    if(i==32000)tape.prepare({15,.1f,15,2,0},32000);
    float input=randomAudio(),l=input,r=-input;inputs[i&4095]=input;tape.process(l,r);
    if(i>=32000*3)assert(!tape.moving&&l==inputs[(i-tape.latency)&4095]&&r==-l);
  }
  puts("PASS AMOUNT=0 settles to exact delayed audio; reused bypassed SONG rack clears motion targets");
}
int main() { neutral();interpolation();movement();bypass();puts("REEL MOTION TESTS PASSED"); }
