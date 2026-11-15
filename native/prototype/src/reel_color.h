#pragma once
#include <math.h>
#include <string.h>

namespace moss {
// Own tape-colour model: reversible emphasis, C2 antiderivative saturation at 2x,
// a high shelf and an independently adjustable broad 80 Hz head bump.
// A halfband pair + compensated antiderivative reserve 12 samples.
// The correction keeps the small-signal response within 0.25 dB through 12 kHz.
struct ReelColor {
  static constexpr unsigned Latency = 12, Ring = 16, Mask = Ring - 1;

  struct Biquad {
    float b0=1,b1=0,b2=0,a1=0,a2=0;
    float x1[2]={},x2[2]={},y1[2]={},y2[2]={};
    float tick(float x,unsigned ch) {
      float y=b0*x+b1*x1[ch]+b2*x2[ch]-a1*y1[ch]-a2*y2[ch];
      x2[ch]=x1[ch];x1[ch]=x;y2[ch]=y1[ch];y1[ch]=fabsf(y)<1e-18f?0:y;
      return y;
    }
    void clear() {for(unsigned ch=0;ch<2;++ch)x1[ch]=x2[ch]=y1[ch]=y2[ch]=0;}
  } shelf,bump;
  // 23-tap halfband, least-squares fit over 0..12 kHz at 64 kHz with exact DC.
  // Centre=.5, alternate taps=0; see the reproducible coefficient generator.
  inline static constexpr float Halfband[12]={
    -.002515569890f,.008517494935f,-.02023682680f,.04300712980f,-.09235986182f,.3135876338f,
    .3135876338f,-.09235986182f,.04300712980f,-.02023682680f,.008517494935f,-.002515569890f};
  inline static constexpr float Correction[4]={-.172f,.672f,.672f,-.172f};
  float dry[Ring][2]={},input[Ring*2][2]={},odd[Ring*2][2]={},even[Ring*2][2]={};
  float aaHistory[16][2]={};
  unsigned innerCursor=0;
  float aaPrevious[2]={};
  float preLow[2]={},postX[2]={},postY[2]={};
  unsigned cursor=0,rate=0,remaining=0;
  float prePole=0,preQ=0,preGain=0,postFeedback=0;
  float drive=0,gain=1,makeup=1,bumpGain=0;
  float targetDrive=0,targetGain=1,targetMakeup=1,targetBump=0;
  float targetToneDb=0,targetBumpDb=0;
  float shelfTarget[5]={1,0,0,0,0},ramp=0;
  bool saturating=false,equalizing=false;
  static float clamp(float x,float lo,float hi) {return !isfinite(x)?lo:x<lo?lo:x>hi?hi:x;}
  static float shape(float x) {
    x=x>1?1:x<-1?-1:x;float u=x*x;
    return x*(1+u*(-2.f/3+u*.2f)); // slope=(1-x*x)^2, continuous second derivative
  }
  void reset(unsigned hz) {
    *this=ReelColor{};rate=hz;
    // Reversible HF pre-emphasis around 3 kHz, followed by its exact inverse.
    prePole=1-expf(-6.28318530718f*3000/rate);preQ=1-prePole;
    preGain=1+preQ;postFeedback=2*preQ/preGain;
    double w=6.283185307179586*80/rate,a=sin(w)/(2*.7),den=1+a;
    bump.b0=float(a/den);bump.b1=0;bump.b2=-bump.b0;
    bump.a1=float(-2*cos(w)/den);bump.a2=float((1-a)/den);
  }
  void prepare(float drivePercent,float toneDb,float bumpDb,float amount,unsigned hz) {
    if(rate!=hz)reset(hz);
    float a=clamp(amount,0,1),d=clamp(drivePercent,0,100)*.01f*a;
    float tone=clamp(toneDb,-6,6)*a,bass=clamp(bumpDb,0,6)*a;
    // Motion locks share this parameter bank. They must neither rebuild colour
    // coefficients nor continually restart an unchanged 20 ms colour ramp.
    if(d==targetDrive && tone==targetToneDb && bass==targetBumpDb)return;
    targetToneDb=tone;targetBumpDb=bass;
    targetDrive=d;targetGain=1+2*d*d; // Up to +9.5 dB; tape density, not a square-wave fuzz.
    targetMakeup=.5f/shape(.5f*targetGain); // fixed reference, no AGC or compressor
    targetBump=powf(10,bass/20)-1;
    double A=pow(10,tone/40),w=6.283185307179586*3000/rate;
    double c=cos(w),s=sin(w),alpha=s*.707106781186548,beta=2*sqrt(A)*alpha;
    double den=(A+1)-(A-1)*c+beta;
    shelfTarget[0]=float(A*((A+1)+(A-1)*c+beta)/den);
    shelfTarget[1]=float(-2*A*((A-1)+(A+1)*c)/den);
    shelfTarget[2]=float(A*((A+1)+(A-1)*c-beta)/den);
    shelfTarget[3]=float(2*((A-1)-(A+1)*c)/den);
    shelfTarget[4]=float(((A+1)-(A-1)*c-beta)/den);
    if(d>0 && !saturating) {
      memset(input,0,sizeof(input));memset(even,0,sizeof(even));memset(odd,0,sizeof(odd));
      memset(aaHistory,0,sizeof(aaHistory));
      innerCursor=0;
      aaPrevious[0]=aaPrevious[1]=0;
      for(unsigned ch=0;ch<2;++ch)preLow[ch]=postX[ch]=postY[ch]=0;
    }
    saturating=saturating||d>0;
    equalizing=equalizing||toneDb*a!=0||targetBump!=0;
    // A finite 20 ms ramp reaches a true zero-cost neutral state.
    remaining=rate/50; if(!remaining)remaining=1;ramp=1.f/remaining;
    finishPrepare();
  }
  void advance() {
    if(!remaining)return;
    // Linear ramps, with no reciprocal in the sample loop.
    drive+=driveStep;gain+=gainStep;makeup+=makeupStep;bumpGain+=bumpStep;
    shelf.b0+=shelfStep[0];shelf.b1+=shelfStep[1];shelf.b2+=shelfStep[2];
    shelf.a1+=shelfStep[3];shelf.a2+=shelfStep[4];
    if(--remaining==0) {
      drive=targetDrive;gain=targetGain;makeup=targetMakeup;bumpGain=targetBump;
      shelf.b0=shelfTarget[0];shelf.b1=shelfTarget[1];shelf.b2=shelfTarget[2];
      shelf.a1=shelfTarget[3];shelf.a2=shelfTarget[4];
      saturating=drive!=0;
      equalizing=targetBump!=0 || shelfTarget[0]!=1;
      if(!equalizing){shelf.clear();bump.clear();}
    }
  }
  float driveStep=0,gainStep=0,makeupStep=0,bumpStep=0,shelfStep[5]={};
  void finishPrepare() {
    driveStep=(targetDrive-drive)*ramp;gainStep=(targetGain-gain)*ramp;
    makeupStep=(targetMakeup-makeup)*ramp;bumpStep=(targetBump-bumpGain)*ramp;
    const float now[]={shelf.b0,shelf.b1,shelf.b2,shelf.a1,shelf.a2};
    for(unsigned k=0;k<5;++k)shelfStep[k]=(shelfTarget[k]-now[k])*ramp;
  }
  template<unsigned N> static void dot(const float *in,const float (&coeff)[N],float *out) {
#if defined(__arm__) && defined(__ARM_NEON)
    const float *weights=coeff;unsigned groups=N/4;
    // Mirrored histories give contiguous stereo taps at every ring position.
    // Only caller-saved D registers: no NEON save/restore for every FIR call.
    asm volatile(
      ".fpu neon\n\t"
      "veor q0, q0, q0\n\t" "veor q1, q1, q1\n\t"
      "1:\n\t" "vld2.32 {d4-d7}, [%[in]]!\n\t"
      "vld1.32 {d16-d17}, [%[weights]]!\n\t"
      "vmla.f32 q0, q2, q8\n\t" "vmla.f32 q1, q3, q8\n\t"
      "subs %[groups], %[groups], #1\n\t" "bne 1b\n\t"
      "vadd.f32 d0, d0, d1\n\t" "vadd.f32 d2, d2, d3\n\t"
      "vpadd.f32 d0, d0, d0\n\t" "vpadd.f32 d2, d2, d2\n\t"
      "vzip.32 d0, d2\n\t" "vst1.32 {d0}, [%[out]]\n\t"
      ".fpu vfpv3-fp16\n\t"
      : [in] "+&r"(in),[weights] "+&r"(weights),[groups] "+&r"(groups)
      : [out] "r"(out)
      : "d0","d1","d2","d3","d4","d5","d6","d7","d16","d17","cc","memory");
#else
    out[0]=out[1]=0;
    for(unsigned k=0;k<N;++k){out[0]+=in[2*k]*coeff[k];out[1]+=in[2*k+1]*coeff[k];}
#endif
  }
  static void write(float (*ring)[2],unsigned at,unsigned size,const float *lr) {
    ring[at][0]=ring[at+size][0]=lr[0];ring[at][1]=ring[at+size][1]=lr[1];
  }
  void antialiased(float *lr) {
    unsigned at=innerCursor;
    float aa[2];
    for(unsigned ch=0;ch<2;++ch) {
      float x=lr[ch]*gain,y=aaPrevious[ch];aaPrevious[ch]=x;
      if(fabsf(x)<=1 && fabsf(y)<=1) {
        float xx=x*x,yy=y*y;
        aa[ch]=(x+y)*(.5f-(xx+yy)/6+(xx*xx+xx*yy+yy*yy)/30);
      } else if((x>=1 && y>=1)||(x<=-1 && y<=-1))aa[ch]=x>0?8.f/15:-8.f/15;
      else {
        auto primitive=[](float v){float u=fabsf(v),q=u*u;return u>=1?u*(8.f/15)-1.f/6:q*(.5f+q*(-1.f/6+q/30));};
        aa[ch]=fabsf(x-y)<.001f?shape((x+y)*.5f):(primitive(x)-primitive(y))/(x-y);
      }
      aa[ch]*=makeup;
    }
    write(aaHistory,at,8,aa);dot(&aaHistory[at+8-3][0],Correction,lr);
    innerCursor=(at+1)&7;

  }
  __attribute__((noinline)) void processSaturation(float &left,float &right) {
    float in[]={left,right};
    for(unsigned ch=0;ch<2;++ch){preLow[ch]+=prePole*(in[ch]-preLow[ch]);in[ch]+=in[ch]-preLow[ch];}
    write(input,cursor,Ring,in);
    float a[2],b[2];dot(&input[cursor+Ring-11][0],Halfband,a);
    for(unsigned ch=0;ch<2;++ch){a[ch]*=2;b[ch]=input[(cursor-5)&Mask][ch];}
    antialiased(a);antialiased(b);write(even,cursor,Ring,a);write(odd,cursor,Ring,b);
    float wetPair[2];dot(&even[cursor+Ring-11][0],Halfband,wetPair);
    float *values[]={&left,&right};
    for(unsigned ch=0;ch<2;++ch) {
      float wet=wetPair[ch]+.5f*odd[(cursor-6)&Mask][ch];
      float restored=(wet-preQ*postX[ch])/preGain+postFeedback*postY[ch];
      postX[ch]=wet;postY[ch]=fabsf(restored)<1e-18f?0:restored;
      float delayed=dry[(cursor-Latency)&Mask][ch];
      *values[ch]=delayed+drive*(restored-delayed);
    }
  }
  void processInput(float &left,float &right,bool updateControls=true) {
    if(updateControls)advance();dry[cursor][0]=left;dry[cursor][1]=right;
    if(saturating)processSaturation(left,right);
    else {left=dry[(cursor-Latency)&Mask][0];right=dry[(cursor-Latency)&Mask][1];}
    cursor=(cursor+1)&Mask;
  }
  void processOutput(float &left,float &right) {
    if(!equalizing)return;
    left+=bumpGain*bump.tick(left,0);right+=bumpGain*bump.tick(right,1);
    left=shelf.tick(left,0);right=shelf.tick(right,1);
  }
};
} // namespace moss
