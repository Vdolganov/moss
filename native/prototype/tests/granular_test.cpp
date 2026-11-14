#undef NDEBUG
#include "../src/granular.h"
#include <assert.h>
#include <stdio.h>
using namespace moss;
static SampleBuffer sample;
static GranularTables tables;
static GranularVoice voice;
static GranularVoice independent;
static float a[64000 * 2], b[64000 * 2];
static void source(unsigned hz = 32000, unsigned channels = 2, unsigned frames = 32000) {
  sample.rate = hz;
  sample.channels = channels;
  sample.frames = frames;
  sample.bytes = frames * channels * 2;
  for (unsigned i = 0; i < frames; ++i)
    for (unsigned ch = 0; ch < channels; ++ch)
      sample.pcm[i * channels + ch] = int16_t(12000 * sin(6.283185307179586 * i / 128));
}
static void render(unsigned n) {
  float l, r;
  for (unsigned i = 0; i < n; ++i) {
    voice.render(l, r);
    assert(isfinite(l) && isfinite(r));
    assert(fabsf(l) < 16 && fabsf(r) < 16);
    assert(voice.count <= voice.capacity);
    for (unsigned k = 0; k < voice.count; ++k) {
      const auto &g = voice.grains[k];
      assert(g.index < sample.frames);
      assert(g.index >= g.first && g.index < g.last);
      assert(g.fraction >= 0 && g.fraction < 1.000001f);
      assert(g.age < g.length);
    }
  }
}
static void event(GranularVoice &v, unsigned frame) {
  GranularParameters p = v.parameters;
  if (frame == 1501) {
    p.speed = -3;
    p.pitch = 24;
    p.scatter = 1;
    p.spread = 1;
  }
  if (frame == 3907) {
    p.sizeMs = 8000;
    p.density = 512;
    p.irregularity = 1;
    p.jump = 1;
  }
  if (frame == 10001) {
    p.sizeMs = 1;
    p.position = .87f;
    p.cutoff = 20;
  }
  if (frame == 17333) {
    p.freeze = true;
    p.start = .65f;
    p.end = .65001f;
  }
  if (frame == 24111) {
    p.freeze = false;
    p.cutoff = 14000;
    p.attackMs = p.releaseMs = 45;
  }
  v.setParameters(p);
  if (frame == 30003)
    v.noteOff();
  if (frame == 41001)
    v.noteOn(-24, .25f);
}
static void invariant(unsigned block, float *out, bool interleave = false) {
  assert(voice.init(sample, tables, 32000, 16, 42));
  voice.noteOn();
  const unsigned events[] = {1501, 3907, 10001, 17333, 24111, 30003, 41001, 64000};
  unsigned at = 0;
  for (unsigned next : events) {
    while (at < next) {
      unsigned n = next - at < block ? next - at : block;
      if (interleave)
        for (unsigned i = 0; i < n; ++i) {
          float l, r;
          independent.render(l, r);
        }
      voice.renderBlock(out + at * 2, n);
      at += n;
    }
    event(voice, at);
  }
}
int main() {
  tables.init();
  assert(tables.hann[0] == 0 && tables.hann[1024] == 0);
  assert(fabsf(tables.hann[512] - 1) < 1e-7f);
  for (unsigned i = 0; i <= 1024; ++i) {
    assert(tables.hann[i] >= 0 && tables.hann[i] <= 1);
    assert(fabsf(tables.hann[i] - tables.hann[1024 - i]) < 1e-6f);
  }
  for (unsigned i = 0; i <= 200; ++i) {
    float x = i / 100.f - 1;
    assert(fabsf(tables.densityFactor(x) - powf(2, x)) < .00002f);
  }
  source();
  // Full cloud covers all eight source regions simultaneously, for long and
  // short PCM alike. Births must not depend on a hidden sequential scan head.
  const unsigned cloudFrames[] = {320u, 32000u, 262144u};
  for(unsigned frames : cloudFrames) {
    source(32000, 1, frames);
    assert(voice.init(sample,tables,32000,8,43));
    assert(independent.init(sample,tables,32000,8,43));
    GranularParameters cloud;
    cloud.start=.2f; cloud.end=.8f; cloud.scatter=1;
    cloud.position=.25f; cloud.speed=-3; cloud.spread=1;
    voice.setParameters(cloud);
    cloud.position=.75f; cloud.speed=3; cloud.freeze=true;
    independent.setParameters(cloud);
    unsigned occupied=0;
    for(unsigned i=0;i<8;++i) {
      voice.spawn(); independent.spawn();
      auto &g=voice.grains[i];
      assert(!(occupied & (1u<<g.zone))); occupied|=1u<<g.zone;
      assert(g.index==independent.grains[i].index);
      assert(g.fraction==independent.grains[i].fraction);
      assert(g.index>=g.first && g.index<g.last);
      unsigned bin=unsigned((double(g.index-g.first)+g.fraction)*8/(g.last-g.first));
      assert(bin==g.zone);
      assert(fabsf(g.panLeft*g.panLeft+g.panRight*g.panRight-2)<1e-5f);
    }
    assert(occupied==255);
    float pan=voice.grains[0].panLeft;
    voice.spawn(); assert(voice.stats.skipped==1 && voice.grains[0].panLeft==pan);
    voice.noteOn(); render(15000);
    cloud=voice.parameters; cloud.scatter=0; cloud.position=.5f; cloud.spread=0;
    voice.stop(); voice.setParameters(cloud);
    for(unsigned i=0;i<8;++i) {voice.spawn(); assert(voice.grains[i].index==unsigned(voice.head>>32));}
    // A bound change affects new grains, not existing windows/read boundaries.
    unsigned first=voice.grains[0].first, last=voice.grains[0].last;
    cloud.start=.6f; cloud.end=.61f; voice.setParameters(cloud);
    assert(voice.grains[0].first==first && voice.grains[0].last==last);
    voice.noteOn(); render(17000);
  }
  puts("PASS granular cloud: eight simultaneous source regions, anchor-independent full scatter, zero-scatter scan, per-grain stereo and latched bounds");
  source();
  const float fades[] = {0, .1f, .5f, .9f, 1};
  for (float in : fades) for (float out : fades) {
    assert(voice.init(sample,tables,32000));
    GranularParameters shape; shape.grainIn=in; shape.grainOut=out; shape.sizeMs=100;
    voice.setParameters(shape); voice.spawn();
    auto &g=voice.grains[0];
    float prior=0;
    for(unsigned age=0;age<g.length;++age) {
      g.age=age; float w=voice.grainWindow(g);
      assert(isfinite(w) && w>=0 && w<=1.000001f);
      if(!age || age+1==g.length) assert(w==0);
      if(age<g.attackFrames) assert(w>=prior);
      if(age>g.releaseStart) assert(w<=prior);
      prior=w;
    }
    unsigned originalAttack=g.attackFrames, originalRelease=g.releaseStart;
    shape.grainIn=.2f; shape.grainOut=.1f; voice.setParameters(shape);
    assert(g.attackFrames==originalAttack && g.releaseStart==originalRelease);
  }
  assert(voice.init(sample,tables,32000));
  GranularParameters loud; loud.sizeMs=500; loud.density=128;
  voice.setParameters(loud);
  assert(voice.gainTarget>.42f && voice.gainTarget<.44f); // old fixed gain was .125
  loud.sizeMs=100; loud.density=2; voice.setParameters(loud);
  assert(fabsf(voice.gainTarget-.75f)<1e-6f);
  loud.level=0; voice.setParameters(loud); assert(voice.gainTarget==0);
  puts("PASS granular envelopes: independent in/out shapes, zero endpoints, latched windows, sparse/dense level compensation");
  assert(voice.init(sample, tables, 32000));
  assert(voice.capacity == 8);
  assert(!voice.init(sample, tables, 32000, 0));
  assert(!voice.init(sample, tables, 32000, 33));
  assert(!voice.init(sample, tables, 7999, 16));
  sample.frames = SampleBudget;
  assert(!voice.init(sample, tables, 32000, 16));
  source();
  invariant(1, a);
  invariant(1024, b);
  assert(!memcmp(a, b, sizeof(a)));
  invariant(257, b);
  assert(!memcmp(a, b, sizeof(a)));
  puts("PASS granular: windows, density lookup, invalid sources, block-invariant "
       "edits/seek/retrigger");

  assert(voice.init(sample, tables, 32000, 16));
  GranularParameters p;
  p.speed = -1;
  p.position = .5f;
  p.start = .25f;
  p.end = .75f;
  voice.setParameters(p);
  voice.noteOn();
  int64_t before = voice.head;
  render(1000);
  assert(voice.head < before);
  p.freeze = true;
  voice.setParameters(p);
  before = voice.head;
  render(1000);
  assert(voice.head == before && voice.stats.spawned > 0);
  p.pitch = 12;
  voice.setParameters(p);
  render(1000);
  assert(voice.head == before && fabsf(voice.readStep - 2) < 1e-6f);
  p.freeze = false;
  voice.setParameters(p);
  render(1000);
  assert(voice.head < before);

  const unsigned capacities[] = {8, 16, 32};
  for (unsigned cap : capacities) {
    assert(voice.init(sample, tables, 32000, cap));
    p = GranularParameters{};
    p.density = 512;
    p.sizeMs = 500;
    p.attackMs = p.releaseMs = 0;
    voice.setParameters(p);
    voice.noteOn();
    render(20000);
    assert(voice.stats.peak >= cap-1 && voice.stats.peak <= cap);
    assert(voice.stats.attempts == voice.stats.spawned + voice.stats.skipped);
    p.sizeMs = 1;
    voice.setParameters(p);
    render(32000);
    voice.noteOff();
    render(1000);
    assert(!voice.running && !voice.count);
    uint64_t attempts = voice.stats.attempts;
    render(1000);
    assert(voice.stats.attempts == attempts);
  }
  puts("PASS granular: reverse scan, freeze/unfreeze, independent pitch, 8/16/32 hard caps, "
       "release and idle");

  source();
  for (unsigned i = 0; i < sample.frames; ++i)
    sample.pcm[i * 2 + 1] = 0;
  assert(voice.init(sample, tables, 32000, 16));
  voice.noteOn();
  voice.renderBlock(a, 32000);
  double energy = 0;
  for (unsigned i = 0; i < 32000; ++i) {
    energy += a[i * 2] * a[i * 2];
    assert(a[i * 2 + 1] == 0);
  }
  assert(energy > 1);
  source(32000, 1);
  assert(voice.init(sample, tables, 32000, 16));
  voice.noteOn();
  voice.renderBlock(a, 32000);
  for (unsigned i = 0; i < 32000; ++i)
    assert(a[i * 2] == a[i * 2 + 1]);
  const float pitches[] = {0, 12};
  for (float pitch : pitches) {
    assert(voice.init(sample, tables, 32000, 16));
    p = GranularParameters{};
    p.pitch = pitch;
    p.sizeMs = 64;
    p.density = 31.25f;
    p.attackMs = p.releaseMs = 0;
    voice.setParameters(p);
    voice.noteOn();
    voice.renderBlock(a, 64000);
    unsigned crossings = 0;
    for (unsigned i = 16001; i < 48000; ++i)
      if (a[(i - 1) * 2] <= 0 && a[i * 2] > 0)
        ++crossings;
    unsigned expected = pitch ? 500 : 250;
    assert(crossings >= expected - 2 && crossings <= expected + 2);
  }
  puts("PASS granular: stereo isolation, mono coherence and measured octave pitch");

  // Parameter locks only affect newly born grains, not active read cursors.
  assert(voice.init(sample, tables, 32000, 16));
  voice.noteOn();
  render(1);
  assert(voice.count == 1);
  unsigned originalLength = voice.grains[0].length;
  unsigned originalStride = voice.grains[0].stepWhole;
  p = voice.parameters;
  p.pitch = 24;
  p.sizeMs = 1;
  voice.setParameters(p);
  assert(voice.grains[0].length == originalLength && voice.grains[0].stepWhole == originalStride);

  invariant(257, a);
  assert(independent.init(sample, tables, 32000, 8, 987));
  independent.noteOn();
  invariant(257, b, true);
  assert(!memcmp(a, b, sizeof(a)));
  const unsigned releaseRates[] = {8000, 32000, 96000};
  for (unsigned hz : releaseRates) {
    assert(voice.init(sample, tables, hz, 16));
    p = GranularParameters{};
    p.attackMs = p.releaseMs = 10;
    voice.setParameters(p);
    voice.noteOn();
    render(hz / 10);
    voice.noteOff();
    render(hz / 100 + 2);
    assert(!voice.running && voice.count == 0);
  }
  puts("PASS granular: per-grain parameter latching, independent random streams and "
       "sample-rate-scaled release");

  const unsigned rates[] = {8000, 32000, 44100, 96000};
  const unsigned channelCounts[] = {1, 2};
  for (unsigned hz : rates)
    for (unsigned channels : channelCounts) {
      const unsigned sizes[] = {1, 2, 17, SampleBudget / (2 * channels)};
      for (unsigned frames : sizes) {
        source(hz, channels, frames);
        assert(voice.init(sample, tables, 8000, 16));
        p = GranularParameters{};
        p.pitch = 24;
        p.speed = -3;
        p.sizeMs = 1;
        p.density = 512;
        p.scatter = 1;
        p.spread = 1;
        p.irregularity = 1;
        p.cutoff = 20;
        p.start = .999999f;
        p.end = 1;
        voice.setParameters(p);
        voice.noteOn(24);
        render(2000);
        p.sizeMs = 500;
        p.cutoff = 3500;
        voice.setParameters(p);
        render(2000);
      }
    }
  source();
  assert(voice.init(sample, tables, 32000, 16));
  p = GranularParameters{};
  p.pitch = NAN;
  p.sizeMs = INFINITY;
  p.cutoff = NAN;
  p.start = 1;
  p.end = -1;
  voice.setParameters(p);
  voice.noteOn();
  render(32000);
  p.density = 0;
  voice.setParameters(p);
  render(32000);
  assert(voice.count == 0);
  puts("PASS granular: 1-frame sources, full 1 MiB, 8..96 kHz, extreme transpose/scatter, invalid "
       "controls and density zero");
  printf("MEMORY voice=%u tables=%u (shared PCM excluded)\n", unsigned(sizeof(voice)),
         unsigned(sizeof(tables)));
}
