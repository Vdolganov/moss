#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Engine engine;
static SampleBuffer original,cut,saved;
static StreamReader reader;
static int16_t output[2048],captureCopy[16384];
static double previewRms(bool stream,const char *file,const SampleView &sample) {
  Project p;p.trackCount=0;engine.init(p,nullptr,AudioRate,0);
  if(stream)assert(reader.begin(engine.stream,file,AudioRate,0,0));
  else engine.auditionBuffer(sample);
  double sum=0;unsigned n=0;
  for(int block=0;block<8;++block) {
    reader.pump(engine.stream);engine.render(output,1024);
    if(block>1)for(int16_t value:output){sum+=double(value)*value;++n;}
  }
  reader.close();return sqrt(sum/n);
}
int main(int argc,char **argv) {
  assert(argc>=2);
#ifdef _WIN32
  assert(SetCurrentDirectoryA(argv[1]));
#else
  assert(chdir(argv[1])==0);
#endif
  bool probe=argc>2;
  original.rate=AudioRate;original.channels=2;original.frames=AudioRate;
  original.bytes=original.frames*4;
  for(unsigned i=0;i<original.frames;++i){original.pcm[2*i]=i%64<32?12000:-12000;original.pcm[2*i+1]=-original.pcm[2*i];}
  assert(writeWav("source.wav",original.pcm,original.frames,original.rate,2));
  WavSource source;assert(source.open("source.wav"));
  assert(source.extract(cut,1000,11000,0,0));
  assert(!memcmp(cut.pcm,original.pcm+2000,cut.bytes));
  assert(writeWav("slice.wav",cut.pcm,cut.frames,cut.rate,2));
  assert(saved.load("slice.wav"));
  assert(saved.bytes==cut.bytes && !memcmp(saved.pcm,cut.pcm,cut.bytes));
  double input=previewRms(true,"source.wav",original),prepared=previewRms(false,"source.wav",original);
  printf("Source preview RMS %.3f; prepared preview RMS %.3f; difference %.4f dB\n",input,prepared,20*log10(prepared/input));
  if(!probe){assert(fabs(input-12000)<1);assert(fabs(input-prepared)<1);}
  Project p;p.trackCount=1;p.master=75;
  auto &t=p.tracks[0];t.level=100;t.keys[0].asset=0;t.keys[0].level=100;
  engine.init(p,&original,AudioRate,1);
  Step step;step.key=0;step.velocity=127;step.length=0;engine.trigger(0,step);
  engine.capture.begin(AudioRate,8192,-1,engine.monitorLatency);
  unsigned frame=0;long long maximumError=0;double heard=0,recorded=0;unsigned n=0;
  while(engine.capture.state==1){
    unsigned count=sampleMin(1024u,8192+engine.capture.latency-frame);assert(count);
    engine.render(output,count);
    for(unsigned i=0;i<count;++i)if(frame+i>=engine.capture.latency) {
      unsigned at=frame+i-engine.capture.latency;
      for(int ch=0;ch<2;++ch){int actual=engine.capture.pcm[at*2+ch],expected=output[i*2+ch];
        maximumError=sampleMax(maximumError,(long long)abs(actual-expected));
        if(at>1024){heard+=double(expected)*expected;recorded+=double(actual)*actual;++n;}
      }
    }
    frame+=count;
  }
  memcpy(captureCopy,engine.capture.pcm,sizeof(captureCopy));
  printf("Master capture vs heard: %.4f dB; maximum error %lld PCM units\n",20*log10(sqrt(recorded/heard)),maximumError);
  if(!probe)assert(maximumError==0);
  assert(writeWav("capture.wav",captureCopy,8192,AudioRate,2));
  assert(saved.load("capture.wav"));
  assert(!memcmp(saved.pcm,captureCopy,sizeof(captureCopy)));
  (void)n;
  puts(probe?"BASELINE measured":"PASS exact slice PCM, unity file auditions and master capture matches monitored output");
}
