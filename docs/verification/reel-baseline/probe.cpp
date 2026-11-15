// Research probe: measures the existing production REEL, without modifying it.
#include "../../../native/prototype/src/effects.h"
#include <cstdio>
#include <cmath>
#include <set>
static moss::FxState fx;
constexpr unsigned Rate=32000;
constexpr double Tau=6.2831853071795864769;
int main(int argc, char **) {
  if(argc>1) {
    int p[moss::EffectTypes][moss::EffectParameters]={};
    p[moss::Reel][0]=100;p[moss::Reel][3]=100;
    fx.reset();fx.prepare(p,Rate,70);
    unsigned first[2]={},last[2]={},wraps[2]={};
    for(unsigned n=0;n<Rate*64;++n) {
      float previous[2]={fx.reelPhase[0],fx.reelPhase[1]},l=0,r=0;
      fx.processFloat(l,r,moss::Reel,true,p[moss::Reel],Rate,70);
      for(int k=0;k<2;++k) if(fx.reelPhase[k]<previous[k]) {
        if(!wraps[k]) first[k]=n;
        last[k]=n;++wraps[k];
      }
    }
    std::puts("component,measured_hz,wraps,first_frame,last_frame");
    for(int k=0;k<2;++k) std::printf("%s,%.9f,%u,%u,%u\n",k?"flutter":"wow",
      double(wraps[k]-1)*Rate/(last[k]-first[k]),wraps[k],first[k],last[k]);
    return 0;
  }
  std::set<int> thirds={60,120,150,180,240,250,300,750,1250,3000,24000,36000,42000};
  for(int i=0;i<=160;++i) thirds.insert(int(std::lround(60*std::pow(700.,i/160.))));
  std::puts("wear,mix,amplitude,frequency_hz,fundamental_gain_db,total_gain_db");
  for(int wear:{0,30,100}) for(int mix:{0,50,100}) for(double amp:{.1,.5}) {
    int p[moss::EffectTypes][moss::EffectParameters]={};
    p[moss::Reel][1]=wear;p[moss::Reel][3]=mix;
    for(int f3:thirds) {
      fx.reset();fx.prepare(p,Rate,70);
      const double frequency=f3/3.,omega=Tau*frequency/Rate;
      double sinSum=0,cosSum=0,energy=0;
      for(unsigned n=0;n<Rate*4;++n) {
        float l=float(amp*std::sin(omega*n)),r=l;
        fx.processFloat(l,r,moss::Reel,true,p[moss::Reel],Rate,70);
        if(n>=Rate) {sinSum+=l*std::sin(omega*n);cosSum+=l*std::cos(omega*n);energy+=l*l;}
      }
      const double count=Rate*3;
      std::printf("%d,%d,%.3f,%.9f,%.6f,%.6f\n",wear,mix,amp,frequency,
        20*std::log10(2*std::hypot(sinSum,cosSum)/count/amp),
        10*std::log10(energy/count/(amp*amp*.5)));
    }
  }
  return 0;
}
