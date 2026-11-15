#define MOSS_REEL_MOTION_LAB 1
#include "../src/engine.h"
#include "../src/diagnostics/stress_scenario.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
using namespace moss;
static Engine engine;
static Project project;
static SampleBuffer sample;
static int16_t output[2048];
struct Result { uint32_t audio=2166136261u,capture=2166136261u,events=0; };
static Result run(unsigned block) {
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});
  engine.capture.finishedCount=engine.capture.startedCount=0;
  engine.capture.begin(32000,16000,-1);Result result;
  for(unsigned at=0;at<96000;) {
    unsigned count=minimum(block,96000-at);engine.render(output,count);at+=count;
    for(unsigned k=0;k<count*2;++k)result.audio=(result.audio^uint16_t(output[k]))*16777619u;
  }
  for(unsigned k=0;k<engine.capture.frames*2;++k)result.capture=(result.capture^uint16_t(engine.capture.pcm[k]))*16777619u;
  for(unsigned t=0;t<Tracks;++t)result.events+=engine.timingEventCount[t]+engine.lockEventCount[t]+engine.triggerCount[t];
  assert(engine.frame==96000 && engine.scheduleOverflows==0 && engine.capture.finishedCount==1);
  return result;
}
int main() {
  stressSample(sample);
#ifdef __arm__
  const unsigned masks[]={10}; // Focused on-device equivalence check.
#else
  const unsigned masks[]={9,10,24,18};
#endif
  for(int mode=0;mode<3;++mode)for(unsigned mask:masks) {
    stressSwing(project,mask,mode);
    for(auto &track:project.tracks) {track.granular.asset=0;for(auto &key:track.keys)key.asset=0;}
    auto reference=run(1);
#ifdef __arm__
    const unsigned blocks[]={1024u};
#else
    const unsigned blocks[]={31u,1024u};
#endif
    for(unsigned block:blocks) {
      auto actual=run(block);
      assert(actual.audio==reference.audio && actual.capture==reference.capture && actual.events==reference.events);
    }
    printf("PASS span/reference: instruments %d mask %u audio %08x capture %08x events %u\n",mode,mask,reference.audio,reference.capture,reference.events);
  }
}
