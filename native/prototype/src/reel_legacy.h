#pragma once
#include "effect_defs.h"
#include <math.h>
namespace moss {
// MS12 and earlier used WOBBLE/WEAR/NOISE/MIX. This is a documented approximate
// conversion, not an attempt to recreate the old comb filtering or clipping.
inline int legacyHiss(int value) {
  if(value<=0)return 0;
  int db=int(lroundf(20*log10f(value*.0006f/1.7320508f)));
  return db<-72?1:db>-24?49:db+73;
}
inline void migrateReel(int *dest,const int *old) {
  const int defaults[]={0,0,0,100,0,62,0,124,0,0,0,0,0,0,100,0};
  for(int i=0;i<16;++i)dest[i]=defaults[i];
  dest[ReelAmount]=old[3];dest[ReelDrive]=old[1]/2;
  dest[ReelWow]=old[0]*3/2;dest[ReelFlutter]=old[0]*3/5;
  dest[ReelLoss]=old[1];dest[ReelHiss]=legacyHiss(old[2]);
}
inline void migrateReelLocks(int16_t *dest,const int *old) {
  for(int i=0;i<16;++i)dest[i]=Inherit;
  auto convert=[&](int slot,int v,int numerator,int denominator) {
    dest[slot]=int16_t(lockValue(v)?v*numerator/denominator:v);
  };
  convert(ReelAmount,old[3],1,1);convert(ReelDrive,old[1],1,2);
  convert(ReelWow,old[0],3,2);convert(ReelFlutter,old[0],3,5);
  convert(ReelLoss,old[1],1,1);
  dest[ReelHiss]=int16_t(lockValue(old[2])?legacyHiss(old[2]):old[2]);
}
}
