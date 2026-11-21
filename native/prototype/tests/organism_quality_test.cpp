#include "../src/synth.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
using namespace moss;
static SynthTables tables;static OrganismDSP voice;
struct Spectrum { double root=0,sub=0,dc=0,stereo=0;int crossings=0; };
static Spectrum measure(int *p) {
  voice=OrganismDSP{};voice.on(33,55,127,p,0,32000,123);
  for(int n=0;n<32000;++n){float l,r;voice.render(l,r,tables.organism,tables.mono);}
  Spectrum s;double rootRe=0,rootIm=0,subRe=0,subIm=0;float previous=0;
  for(int n=0;n<64000;++n){float l,r;voice.render(l,r,tables.organism,tables.mono);double x=(l+r)*.5;
    double a=6.283185307179586*n/32000;rootRe+=x*cos(a*55);rootIm+=x*sin(a*55);subRe+=x*cos(a*27.5);subIm+=x*sin(a*27.5);
    s.dc+=x;s.stereo+=fabs(l-r);if(l>=0&&previous<0)++s.crossings;previous=l;
  }
  s.root=sqrt(rootRe*rootRe+rootIm*rootIm)/32000;s.sub=sqrt(subRe*subRe+subIm*subIm)/32000;s.dc/=64000;return s;
}
int main(int argc,char **argv){
  tables.init();int p[OrganismParameters];memcpy(p,OrganismPresets[3],sizeof(p));
  const int zero[]={OrgShape,OrgCouple,OrgDensity,OrgSub,OrgResonance,OrgDrive,OrgNoise,OrgDrift,OrgWander,OrgBreath,OrgDetune,OrgSpread,OrgGlide,OrgAttack,OrgGrit};
  for(int i:zero)p[i]=0;p[OrgCutoff]=14000;p[OrgInterval]=12;
  auto bare=measure(p);assert(bare.crossings>=109&&bare.crossings<=111&&bare.root>.4&&fabs(bare.dc)<.001&&bare.stereo==0);
  p[OrgDensity]=100;auto dense=measure(p);assert(dense.root>bare.root*.8);
  p[OrgSub]=100;auto sub=measure(p);assert(sub.sub>.35&&sub.sub>bare.sub*100);
  p[OrgWander]=100;auto mono=measure(p);assert(mono.stereo==0);
  printf("PASS ORGANISM bass: A1 %.3f amplitude, dense %.3f, sub A0 %.3f; correct pitch, no density collapse, SPREAD=0 mono\n",bare.root,dense.root,sub.sub);
  if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);float previous=float(3.5*sin(-6.283185307179586*3101/32000));
    for(int n=0;n<32000;++n){float x=float(3.5*sin(6.283185307179586*3101*n/32000));float y=tables.organism.saturate(x,previous,tables.mono);assert(isfinite(y)&&fabsf(y)<1.001f);
      float pair[]={tanhf(x),y};assert(fwrite(pair,sizeof(float),2,f)==2);
    }fclose(f);
  }
}
