#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static SampleBuffer sample;
static GranularTables tables;
static GranularVoice voice, peer;
static Engine::GrainMailbox mailbox;
static Codec codec;
static Project project, restored;
static void tick(GranularVoice &v) { float l,r;v.render(l,r);assert(isfinite(l)&&isfinite(r)); }
int main() {
  tables.init(); sample.rate=32000;sample.channels=1;sample.frames=32000;sample.bytes=64000;
  for(unsigned i=0;i<sample.frames;++i) sample.pcm[i]=4000;
  assert(voice.init(sample,tables,32000,8,123));
  GranularParameters p;p.sizeMs=8000;p.density=128;p.scatter=1;p.jump=1;
  p.start=.2f;p.end=.8f;p.position=.3f;p.speed=.1f;p.attackMs=10;
  voice.setParameters(p);voice.noteOn();
  assert(voice.grainFrames==256000 && voice.effectiveDensity==1);
  unsigned jumps=0;float minimum=100,maximum=0;
  for(unsigned n=0;n<32000*24;++n) {
    auto expected=voice.head+voice.scanStep;
    if(expected>=voice.scanLast)expected-=voice.scanLast-voice.scanFirst;
    float l,r;voice.render(l,r);
    if(voice.head!=expected)++jumps;
    assert(voice.head>=voice.scanFirst && voice.head<voice.scanLast);
    assert(voice.count<=8 && isfinite(l) && isfinite(r));
    uint32_t lanes=0;
    for(unsigned i=0;i<voice.count;++i) {
      const auto &g=voice.grains[i];assert(g.length==256000);
      assert(g.index>=g.first && g.index<g.last);
      assert(g.lane<8 && !(lanes&(1u<<g.lane)));lanes|=1u<<g.lane;
    }
    if(n>=32000*10) { minimum=fminf(minimum,l);maximum=fmaxf(maximum,l); }
  }
  assert(jumps>=12 && jumps<=32 && voice.stats.peak==8);
  // Constant source: eight evenly staggered Hann windows must sustain a pad,
  // including replacement boundaries, instead of a burst followed by silence.
  assert(minimum>.1f && maximum/minimum<1.01f);
  mailbox.publish(voice);auto view=mailbox.read();assert(view.count==voice.count && view.density==1000);
  for(unsigned i=0;i<view.count;++i) {
    const auto &g=voice.grains[i];const auto &v=view.grains[i];
    assert(v.lane==g.lane && v.loops>=13 && v.span==65536);
    assert(abs(int(v.position)-int((g.index+g.fraction)*65536/sample.frames))<=1);
    assert(abs(int(v.age)-int(double(g.age)*65536/g.length))<=1);
  }
  auto head=voice.head;auto countdown=voice.jumpCountdown;auto births=voice.stats.spawned;
  p.freeze=true;voice.setParameters(p);for(unsigned i=0;i<64000;++i)tick(voice);
  assert(voice.head==head && voice.jumpCountdown==countdown && voice.stats.spawned>births);
  p.freeze=false;p.jump=0;voice.setParameters(p);
  for(unsigned i=0;i<64000;++i) {
    auto expected=voice.head+voice.scanStep;if(expected>=voice.scanLast)expected-=voice.scanLast-voice.scanFirst;
    tick(voice);assert(voice.head==expected);
  }
  puts("PASS ambient: 8-second sustained pad, staggered replacements, bounded random jumps, continuous scan and FREEZE, stable lanes and truthful snapshots");
  assert(voice.init(sample,tables,32000,8,4));assert(peer.init(sample,tables,32000,8,4));
  p=GranularParameters{};p.scatter=1;p.jump=1;p.sizeMs=700;p.density=8;
  voice.setParameters(p);p.freeze=true;peer.setParameters(p);voice.noteOn();peer.noteOn();
  for(unsigned n=0;n<96000;++n) {
    tick(voice);tick(peer);assert(voice.count==peer.count);
    for(unsigned i=0;i<voice.count;++i)assert(voice.grains[i].origin==peer.grains[i].origin);
  }
  puts("PASS independent head RNG: full SCATTER births unchanged by JUMP and FREEZE");
  project.tracks[0].instrument=GranularInstrument;auto &t=project.tracks[0];
  t.granular.values[2]=8000;t.granular.values[9]=73;t.steps[0].grainLocks[9]=91;
  assert(codec.encode(project) && !memcmp(codec.bytes,"MS12",4));
  assert(codec.decode(restored,codec.cursor));assert(restored.tracks[0].granular.values[2]==8000);
  assert(restored.tracks[0].granular.values[9]==73 && restored.tracks[0].steps[0].grainLocks[9]==91);
  // Genuine old payload, with timing irregularity in the now-replaced slot.
  t.granular.values[2]=440;
  codec.cursor=16;codec.ok=true;codec.fields(project,true,9);
  memcpy(codec.bytes,"MOS9",4);codec.word(4,9);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));assert(restored.tracks[0].granular.values[2]==440);
  assert(restored.tracks[0].granular.values[9]==0 && restored.tracks[0].steps[0].grainLocks[9]==Inherit);
  puts("PASS MS12 long SIZE/JUMP roundtrip and MOS9 timing-irregularity migration");
}
