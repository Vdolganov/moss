#include "../src/engine.h"
#include "../src/storage.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Engine engine;
static SampleBuffer sample;
static Project project, restored;
static Codec codec;
static int16_t output[2048];
static bool baseline=false;
static double sourceRms(){double energy=0;for(unsigned i=0;i<sample.frames*sample.channels;++i)energy+=double(sample.pcm[i])*sample.pcm[i];return sqrt(energy/(sample.frames*sample.channels));}
static double measure(int instrument,int velocity=127,int trackLevel=100,int master=100,int keyLevel=100,bool preview=false,int tracks=1,int keys=1,bool audition=false){
  project=Project{};project.trackCount=tracks;project.master=master;
  for(int i=0;i<tracks;++i){auto &t=project.tracks[i];t.instrument=instrument;t.level=trackLevel;
    for(auto &k:t.keys){k.asset=0;k.level=keyLevel;}
    t.granular.asset=0;int values[]={100,0,100,80,0,0,1000,100,0,0,14000,100,50,50,0,0,0,0,0,0};memcpy(t.granular.values,values,sizeof(values));
  }
  engine.init(project,&sample,AudioRate,1);
  if(preview)engine.auditionBuffer(sample);
  else if(audition)engine.handle({Op::Audition,0,0});
  else for(int t=0;t<tracks;++t)for(int k=0;k<keys;++k){Step event;event.key=k;event.note=GranularRootNote;event.velocity=velocity;event.length=0;engine.trigger(t,event);}
  double energy=0;unsigned samples=0,total=instrument==GranularInstrument?AudioRate*6:AudioRate;
  for(unsigned frame=0;frame<total;){unsigned n=minimum(1024u,total-frame);engine.render(output,n);
    if(frame>=(instrument==GranularInstrument?AudioRate*2:AudioRate/4))for(unsigned i=0;i<n*2;++i){energy+=double(output[i])*output[i];++samples;assert(output[i]!=-32768&&output[i]!=32767);}
    frame+=n;
  }
  return sqrt(energy/samples);
}
int main(int argc,char **argv){
  baseline=argc>1&&!strcmp(argv[1],"--baseline");
  sample.rate=AudioRate;sample.channels=2;sample.frames=AudioRate*4;sample.bytes=sample.frames*4;
  for(unsigned i=0;i<sample.frames;++i){sample.pcm[i*2]=i%64<32?6000:-6000;sample.pcm[i*2+1]=-sample.pcm[i*2];}
  double original=sourceRms(),preview=measure(SampleInstrument,127,100,0,100,true),unity=measure(SampleInstrument);
  printf("File %.3f / preview %.3f / sampler at 100%% %.3f: %.5f dB\n",original,preview,unity,20*log10(unity/preview));
  if(!baseline){assert(fabs(preview/original-1)<.001);assert(fabs(unity/original-1)<.001);}
  if(!baseline)assert(fabs(measure(SampleInstrument,127,100,100,100,false,1,1,true)/original-1)<.001);
  if(!baseline)assert(fabs(measure(SampleInstrument,127,100,100,100,false,2)/original-2)<.001);
  Project defaults;double fresh=measure(SampleInstrument,90,defaults.tracks[0].level,defaults.master,defaults.tracks[0].keys[0].level);
  printf("Defaults track=%d master=%d slot=%d; new hit velocity 90: %.3f RMS / %.5f dB\n",defaults.tracks[0].level,defaults.master,defaults.tracks[0].keys[0].level,fresh,20*log10(fresh/original));
  if(!baseline){assert(defaults.master==100);for(auto &t:defaults.tracks){assert(t.level==100);for(auto &k:t.keys)assert(k.level==100);}assert(fabs(fresh/original-90./127)<.001);}
  double half=measure(SampleInstrument,127,100,50),quarter=measure(SampleInstrument,127,50,50);
  if(!baseline){assert(fabs(half/unity-.5)<.001);assert(fabs(quarter/unity-.25)<.001);}
  assert(measure(SampleInstrument,0)==0);assert(measure(SampleInstrument,127,0)==0);assert(measure(SampleInstrument,127,100,0)==0);assert(measure(SampleInstrument,127,100,100,0)==0);
  // Existing musical mix settings must round-trip without becoming new defaults.
  project=Project{};project.master=37;project.tracks[0].level=42;project.tracks[0].keys[0].level=63;
  assert(codec.encode(project)&&codec.decode(restored,codec.cursor));assert(restored.master==37&&restored.tracks[0].level==42&&restored.tracks[0].keys[0].level==63);
  // Overlap compensation is tested with decorrelated band-limited material.
  // A granular cloud is not expected to null against the original waveform.
  uint32_t random=9123457;float filtered[2]={};
  for(unsigned i=0;i<sample.frames;++i)for(int c=0;c<2;++c){random^=random<<13;random^=random>>17;random^=random<<5;float white=(int(random&65535)-32768)*.25f;filtered[c]+=(white-filtered[c])*.3f;sample.pcm[i*2+c]=int16_t(filtered[c]);}
  original=sourceRms();double cloud=measure(GranularInstrument),cloudHalf=measure(GranularInstrument,127,100,50);
  printf("Granular dense cloud %.3f / source %.3f: %.5f dB; master 50%% ratio %.5f\n",cloud,original,20*log10(cloud/original),cloudHalf/cloud);
  if(!baseline){assert(cloud/original>.84&&cloud/original<1.2);assert(fabs(cloudHalf/cloud-.5)<.002);}
  for(unsigned i=0;i<sample.frames*2;++i)sample.pcm[i]=i%128<64?30000:-30000;
  double overload=measure(SampleInstrument,127,100,100,100,false,4,4);assert(overload>0&&overload<32767);
  for(int value=0;value<=32*32768;value+=113){int y=Engine::monitor(value);assert(y>=0&&y<=32767);assert(Engine::monitor(-value)==-y);}
  printf("16 simultaneous loud samples remain bounded; RMS %.3f\n",overload);
  puts(baseline?"BASELINE ONLY":"PASS unity sample path, granular level, default mix, gain controls, persistence and overload bounds");
}
