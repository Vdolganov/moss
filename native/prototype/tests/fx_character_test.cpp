#include "../src/app.h"
#include "legacy_fixture.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static FxState rack;
static Engine engine;
static App app;
static Project project, restored;
static Codec codec;
static SampleBuffer sample;
static int16_t output[2048];
static int referenceAudio[32000*3][2];
static uint64_t now=1000000;
static void gesture(int face,int direction) {
  app.button(face,true,now);app.button(direction,true,now+10000);
  app.button(direction,false,now+20000);app.button(face,false,now+30000);now+=100000;
}
static void tap(int key) { app.button(key,true,now);app.button(key,false,now+30000);now+=100000; }
static void controls() {
  app.engine=&engine;app.project=Project{};app.page=Space;app.fxDetail=true;
  auto &f=app.project.tracks[0].fx;
  f.select(Drive);app.fxBank=0;
  tap(lab::R1);assert(app.fxBank==1 && f.type==Drive);
  gesture(lab::B,lab::Up);assert(f.values[Drive][5]==10);
  tap(lab::L1);assert(app.fxBank==0);
  gesture(lab::Y,lab::Right);assert(f.values[Drive][0]==31);
  app.page=Pattern;app.bank=2;app.step=0;
  gesture(lab::L1,lab::Up);assert(app.fxBank==1);
  gesture(lab::X,lab::Right);assert(app.project.tracks[0].steps[0].fxLocks[Drive][7]==1);
  assert(app.project.tracks[0].steps[0].fxLocks[Drive][3]==Inherit);
  f.move(Drive,-1);assert(app.project.tracks[0].steps[0].fxLocks[Drive][7]==1);
  app.page=Mix;app.mixTab=1;app.fxDetail=true;app.project.fx.select(Orbit);app.fxBank=0;
  tap(lab::R1);assert(app.mixTab==1&&app.fxDetail&&app.fxBank==1);
  gesture(lab::B,lab::Up);assert(app.project.fx.values[Orbit][5]==10000);
  assert(valid(app.project));
  assert(codec.encode(app.project)&&!memcmp(codec.bytes,"MS18",4));
  assert(codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].steps[0].fxLocks[Drive][7]==1);
  assert(restored.fx.values[Orbit][5]==10000);
  // A genuine old-shape payload: mix/spread swap, new defaults and empty locks.
  project=Project{};project.tracks[0].fx.values[0][2]=73;project.tracks[0].fx.values[0][3]=22;
  project.tracks[0].steps[0].fxLocks[0][2]=68;
  codec.cursor=16;codec.ok=true;legacyFields(codec,project,8);
  memcpy(codec.bytes,"MOS8",4);codec.word(4,8);codec.word(8,codec.cursor);codec.word(12,codec.hash(codec.cursor));
  assert(codec.decode(restored,codec.cursor));
  assert(restored.tracks[0].fx.values[0][3]==73&&restored.tracks[0].fx.values[0][2]==22);
  assert(restored.tracks[0].steps[0].fxLocks[0][3]==68);
  assert(restored.tracks[0].steps[0].fxLocks[Drive][7]==Inherit);
  puts("PASS FX controls: two banks, physical gestures, master navigation, lock identity/reorder, MS12 and MOS8 migration");
}
static void curves() {
  for(int curve=0;curve<3;++curve) {
    float last=-2;
    for(int i=-2000;i<=2000;++i) {
      float x=i*.001f,y=DriveDSP::shape(x,curve);
      assert(isfinite(y)&&fabsf(y)<=1.00001f&&y>=last-.000001f);last=y;
      float derivative=(DriveDSP::integral(x+.002f,curve)-DriveDSP::integral(x-.002f,curve))/.004f;
      assert(fabsf(derivative-y)<.003f);
      assert(fabsf(DriveDSP::antialiased(x,x,curve)-y)<.00001f);
    }
  }
  Effect f;f.select(Drive);f.enable(true);f.values[Drive][3]=100;
  const unsigned rates[]={8000,32000,48000,96000};
  for(unsigned rate:rates) {
    rack.reset();
    for(unsigned i=0;i<rate*2;++i) {
      if(!(i%257)) {
        for(int k=0;k<8;++k) f.values[Drive][k]=(i/257+k)%2?fxHigh(Drive,k):fxLow(Drive,k);
        f.values[Drive][3]=100;
        rack.prepare(f.values,rate,120);
      }
      int l=int(18000*sinf(i*.17f)),r=-l;
      rack.processChain(l,r,f,f.values,rate,120);
      assert(abs(l)<=8*32768&&abs(r)<=8*32768);
    }
  }
  puts("PASS DRIVE: polynomial primitives, monotonic curves, equal-input ADAA and parameter extrema at four sample rates");
}
static void orbit() {
  Effect f;f.enable(true);
  int *p=f.values[Orbit];p[0]=0;p[1]=95;p[2]=100;p[3]=100;p[4]=20;p[5]=14000;p[6]=100;p[7]=100;
  rack.reset();rack.prepare(f.values,32000,240);
  double energy=0,tail=0;
  for(unsigned i=0;i<32000*8;++i) {
    int l=i<128?16000:0,r=0;
    rack.processChain(l,r,f,f.values,32000,240);
    assert(abs(l)<32768&&abs(r)<32768);
    if(i>1000&&i<32000)energy+=double(l)*l+double(r)*r;
    if(i>32000*7)tail+=double(l)*l+double(r)*r;
  }
  assert(energy>100000&&tail<energy*.02);
  // Reset is logical, and must not expose previous buffer contents.
  rack.invalidate();rack.prepare(f.values,32000,240);
  for(unsigned i=0;i<16000;++i) {int l=0,r=0;rack.processChain(l,r,f,f.values,32000,240);assert(l==0&&r==0);}
  for(unsigned i=0;i<64000;++i) {
    if(!(i%500)) {p[0]=(i/500)%8;rack.prepare(f.values,32000,240);}
    int l=int(10000*sinf(i*.07f)),r=-l;rack.processChain(l,r,f,f.values,32000,240);
    assert(abs(l)<32768&&abs(r)<32768);
  }
  puts("PASS ORBIT: stereo feedback decay, no stale history, bounded rapid TIME locks and modulation");
}
static double samplerTone(int frequency) {
  sample.rate=32000;sample.channels=1;sample.frames=64000;sample.bytes=128000;
  for(unsigned i=0;i<sample.frames;++i) sample.pcm[i]=int16_t(6000*sin(6.283185307179586*frequency*i/sample.rate));
  project=Project{};project.trackCount=1;project.master=100;
  auto &t=project.tracks[0];t.level=100;t.keys[0].asset=0;t.keys[0].pitch=0;t.keys[0].level=100;
  t.cycle=512;t.hits[0][0].on=1;t.hits[0][0].velocity=127;
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});
  double square=0;unsigned measured=0;
  for(unsigned at=0;at<64000;at+=1024) {
    unsigned count=minimum(1024u,64000u-at);engine.render(output,count);
    for(unsigned n=0;n<count;++n) if(at+n>=8000&&at+n<56000) {square+=double(output[n*2])*output[n*2];++measured;}
  }
  return sqrt(square/measured);
}
static void bass() {
  double reference=samplerTone(1000);
  const int tones[]={20,30,40,60,80,100,200,500,1000};
  for(int hz:tones) {
    double rms=samplerTone(hz),db=20*log10(rms/reference);
    printf("BASS dry sampler %d Hz: %.4f dB vs 1 kHz\n",hz,db);
    assert(fabs(db)<.1);
  }
  puts("PASS bass: dry sample -> track -> master -> monitor is flat within 0.1 dB from 20 Hz to 1 kHz");
}
static void neutrality() {
  DriveDSP dsp;Effect f;int *p=f.values[Drive];p[0]=0;
  dsp.prepare(p,32000);dsp.start();
  double input=0,outputEnergy=0;
  for(int i=0;i<64000;++i) {
    float in[2]={.1f*sinf(float(6.28318530718*20*i/32000)),0},out[2];
    dsp.process(in,out);
    if(i>=32000) {input+=double(in[0])*in[0];outputEnergy+=double(out[0])*out[0];}
  }
  assert(fabs(10*log10(outputEnergy/input))<.5);
  for(int curve=0;curve<3;++curve) {
    p[0]=100;p[4]=curve;p[5]=100;dsp.prepare(p,32000);dsp.start();
    for(int i=0;i<64000;++i) {
      float in[2]={i<32000?.2f:0,0},out[2];dsp.process(in,out);
      if(i>63000) assert(fabsf(out[0])<.0001f&&fabsf(out[1])<.0001f);
    }
  }
  puts("PASS DRIVE neutrality: zero drive preserves 20 Hz within 0.5 dB; biased curves settle to silence");
}
static uint32_t partitioned(unsigned block) {
  engine.init(project,&sample,32000,1);engine.handle({Op::Transport});
  uint32_t hash=2166136261u;
  for(unsigned at=0;at<96000;) {
    unsigned count=minimum(block,96000u-at);engine.render(output,count);
    for(unsigned i=0;i<count*2;++i) hash=(hash^uint16_t(output[i]))*16777619u;
    at+=count;
  }
  return hash;
}
static void blocks() {
  project=Project{};project.trackCount=1;project.bpm=240;
  auto &t=project.tracks[0];t.cycle=64;t.division=64;t.keys[0].asset=0;
  t.fx.enable(true);t.fx.select(Drive);t.fx.enable(true);
  for(int i=0;i<64;++i) {
    t.hits[0][i].on=1;t.steps[i].fxLocks[Drive][4]=int16_t(i%3);
    t.steps[i].fxLocks[Orbit][0]=int16_t(i%8);
    t.steps[i].fxLocks[Orbit][4]=int16_t(i%2?2000:0);
  }
  uint32_t reference=partitioned(1024);
  assert(reference==partitioned(127));assert(reference==partitioned(1));
  printf("PASS engine block invariance: DRIVE curve and ORBIT time/filter locks, hash %08x\n",reference);
}
static void effectiveControls() {
  const int kinds[]={Orbit,Drive};
  for(int kind:kinds) {
    for(int control=-1;control<8;++control) {
      Effect f;f.select(kind);f.enable(true);
      if(control>=0) {
        int &v=f.values[kind][control];v=v==fxHigh(kind,control)?fxLow(kind,control):fxHigh(kind,control);
      }
      rack.reset();rack.prepare(f.values,32000,120);
      double difference=0;
      for(unsigned i=0;i<32000*3;++i) {
        int l=int(3000*sinf(i*.019f)+1700*sinf(i*.57f));
        int r=int(2300*sinf(i*.023f)+1000*sinf(i*.71f));
        rack.processChain(l,r,f,f.values,32000,120);
        if(control<0) {referenceAudio[i][0]=l;referenceAudio[i][1]=r;}
        else {difference+=abs(l-referenceAudio[i][0])+abs(r-referenceAudio[i][1]);}
      }
      if(control>=0) assert(difference/(32000*6)>1);
    }
  }
  puts("PASS all 16 DRIVE/ORBIT controls measurably change stereo output");
}
int main() { controls();curves();orbit();bass();neutrality();blocks();effectiveControls();return 0; }
