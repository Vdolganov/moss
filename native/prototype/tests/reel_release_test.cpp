#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static ReelDSP tape;
static ReelMotion motion;
static Engine engine;
static App app;
static Codec codec;
static Project restored;
static uint32_t rng=123;
static float noise(){rng=rng*1664525u+1013904223u;return int32_t(rng)/2147483648.f;}
static void signal() {
  Effect e;int *p=e.values[Reel];float input[4096]={};
  tape.resetReel(32000);tape.prepareReel(p,32000,true);
  for(unsigned n=0;n<96000;++n) {
    float x=noise()*2,l=x,r=-x;input[n&4095]=x;tape.processReel(l,r,true,p,32000);
    float expected=n>=512?input[(n-512)&4095]:0;
    assert(l==expected&&r==-expected);
  }
  // Both independently moving read heads use exactly the same interpolation
  // kernel as the mono-linked path, including the ring seam and warmup.
  motion.reset(32000);motion.written=ReelMotion::Size;
  for(auto &v:motion.history){v[0]=noise();v[1]=noise();}
  for(unsigned i=0;i<4096;++i) {
    motion.cursor=i;float dl=500+(i%100)*.013f,dr=dl+3.13f;
    float l,r,rl,rr,unused;motion.read(dl,rl,unused);motion.read(dr,unused,rr);
    motion.readStereo(dl,dr,l,r);assert(fabsf(l-rl)<2e-6f&&fabsf(r-rr)<2e-6f);
  }
  tape.resetReel(32000);p[ReelHiss]=49;p[ReelFollow]=0;
  tape.prepareReel(p,32000,true);tape.life.running=true;
  double energy=0,cross=0;unsigned measured=0;
  for(unsigned n=0;n<96000;++n) {
    float l=0,r=0;tape.processReel(l,r,true,p,32000);
    if(n>=32000){energy+=double(l)*l;cross+=double(l)*r;++measured;}
  }
  double db=10*log10(energy/measured);
  printf("HISS white RMS %.3f dBFS / channel correlation %.5f\n",db,cross/energy);
  assert(fabs(db+24)<.15 && fabs(cross/energy)<.02);
  tape.life.running=false;
  for(unsigned n=0;n<32000;++n){float l=0,r=0;tape.processReel(l,r,true,p,32000);if(n>30000)assert(l==0&&r==0);}
  // Full LIFE followed by AMOUNT=0 must converge to a truly neutral FIFO.
  for(int i=0;i<16;++i)p[i]=ReelPresets[4][i];p[ReelStereo]=100;p[ReelDrift]=100;
  tape.resetReel(32000,987);tape.prepareReel(p,32000,true);tape.life.running=true;
  float minimumDrop=1;
  for(unsigned n=0;n<32000*12;++n){float l=.1f,r=.1f;tape.processReel(l,r,true,p,32000);assert(isfinite(l)&&isfinite(r));if(tape.life.drop<minimumDrop)minimumDrop=tape.life.drop;}
  assert(minimumDrop<.98f);
  p[ReelAmount]=0;tape.prepareReel(p,32000,true);
  for(unsigned n=0;n<32000*12;++n){float x=noise(),l=x,r=-x;input[n&4095]=x;tape.processReel(l,r,true,p,32000);if(n>32000*11){
    if(fabsf(l-input[(n-512)&4095])>=1e-6f){printf("neutral err %g pos %g offset %g drift %g / moving %d life %d color %d sat %d gain %g lat %u\n",double(l-input[(n-512)&4095]),double(tape.motion.position),double(tape.motion.rightOffset),double(tape.motion.driftOffset),tape.motion.moving,tape.life.active,tape.color.equalizing,tape.color.saturating,double(tape.life.gain),tape.motion.latency);fflush(stdout);assert(false);}
    assert(fabsf(l+r)<1e-6f);}}
  puts("PASS REEL: exact neutral float / dual-head FIR / independent calibrated hiss / STOP / defects / AMOUNT recovery");
}
static uint64_t now=1000000;
static void tap(int key){app.button(key,true,now);app.button(key,false,now+30000);now+=100000;}
static void gesture(int face,int direction){app.button(face,true,now);app.button(direction,true,now+10000);app.button(direction,false,now+20000);app.button(face,false,now+30000);now+=100000;}
static void controls() {
  app.engine=&engine;app.project=Project{};app.page=Space;app.fxDetail=true;app.track=0;
  auto &f=app.project.tracks[0].fx;f.select(Reel);f.enable(true);
  for(int page=0;page<4;++page){assert(app.fxBank==page);gesture(lab::Y,lab::Right);assert(f.values[Reel][page*4]==1);tap(lab::R1);}
  assert(app.fxBank==0);gesture(lab::B,lab::Up);assert(f.values[Reel][ReelTone]==10);
  app.page=Pattern;app.bank=2;gesture(lab::L1,lab::Up);assert(app.fxBank==1);
  gesture(lab::Y,lab::Right);assert(app.project.tracks[0].steps[0].fxLocks[Reel][ReelWow]==2);
  gesture(lab::L1,lab::Up);gesture(lab::L1,lab::Up);assert(app.fxBank==3);
  gesture(lab::X,lab::Up);assert(app.project.tracks[0].steps[0].fxLocks[Reel][ReelOutput]==10);
  assert(codec.encode(app.project)&&codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].steps[0].fxLocks[Reel][ReelOutput]==10);
  assert(restored.tracks[0].steps[0].fxLocks[Reel][ReelWow]==2);
  app.page=Mix;app.mixTab=1;app.project.fx.select(Reel);app.voicePicker=4;app.voiceChoice=2;tap(lab::A);
  for(int i=0;i<16;++i)assert(app.project.fx.values[Reel][i]==ReelPresets[2][i]);
  assert(app.project.tracks[0].fx.values[Reel][ReelDrive]==1);
  puts("PASS REEL: four pages / physical fine-coarse / all lock pages / MS13 roundtrip / atomic master preset");
}
static void captureAlignment() {
  static SampleBuffer sample;static int16_t block[2048];
  sample.rate=32000;sample.channels=2;sample.frames=64000;sample.bytes=sample.frames*4;
  for(unsigned i=0;i<sample.frames*2;++i)sample.pcm[i]=2000;
  Project p;p.trackCount=1;p.bpm=240;p.tracks[0].keys[0].asset=0;
  p.tracks[0].hits[0][0].on=NoteEvent;
  for(int source=0;source<2;++source) {
    engine.init(p,&sample,32000,1);engine.handle({Op::Transport});
    engine.handle({Op::CaptureStart,source,0,0,1});
    unsigned frames=32000+engine.capture.latency;
    while(frames){unsigned n=minimum(1024u,frames);engine.render(block,n);frames-=n;}
    assert(engine.capture.frames==32000 && engine.capture.state==2);
    unsigned first=0;while(first<512 && !engine.capture.pcm[first*2])++first;
    assert(first<32); // The sampler's short anti-click attack remains; the 512/1024-frame tape pre-roll does not.
    engine.handle({Op::CaptureDiscard});engine.handle({Op::CaptureStart,source,0,0,1});
    engine.render(block,73);engine.handle({Op::CaptureStop});
    engine.render(block,engine.capture.latency);
    assert(engine.capture.frames==73 && engine.capture.state==2);
  }
  puts("PASS capture alignment: track/master, exact one-bar length, no leading transport delay, manual finish during warmup");
}
int main(int argc,char **argv) {
  signal();controls();captureAlignment();
  if(argc>1){assert(loadProject(argv[1],restored));assert(valid(restored));assert(codec.encode(restored));printf("PASS actual legacy project: %s / %d tracks\n",restored.name,restored.trackCount);}
}
