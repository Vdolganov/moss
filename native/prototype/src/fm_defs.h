#pragma once
namespace moss {
// Wire slots 0..7 retain their original meaning. New slots are appended.
enum FmParameter { FmIndex, FmRatio, FmFeedback, FmDecay, FmDetune, FmNoise,
  FmAttack, FmBody, FmPunch, FmSweep, FmClick, FmDynamics, FmModDecay,
  FmModHold, FmTone, FmSpread, FmRate, FmMotion, FmRelease, FmGlide, FmParameters };
inline constexpr float FmRatios[]={.5f,1,1.5f,2,3,4,5,7,1.41421356f,1.73205081f,2.41421356f,3.14159265f,6,8,11,16};
inline constexpr const char *FmRatioNames[]={"0.5:1","1:1","1.5:1","2:1","3:1","4:1","5:1","7:1","1.414:1","1.732:1","2.414:1","3.142:1","6:1","8:1","11:1","16:1"};
inline constexpr int FmPresetCount=8;
inline constexpr const char *FmPresetNames[]={"GLASS","BELL","HAZE","METAL","THUMP","SNAP","RUBBER","CLOUD"};
inline constexpr int FmPresetModes[]={0,0,0,1,0,1,1,0};
inline constexpr int FmPresets[FmPresetCount][FmParameters]={
  {45,4,12,2200,4,0,3,0,  0,80,8,65,  1100,12,85,25,  20,5,650,0},
  {70,5,20,4000,12,5,3,0,  5,35,10,70,  1200,25,92,40,  14,8,1800,0},
  {25,1,5,2400,25,18,350,65,  0,300,0,30,  3500,65,65,80,  9,50,3500,140},
  {85,10,65,1600,60,30,5,15,  -5,70,25,80,  500,15,88,45,  130,18,600,0},
  {14,1,6,550,0,0,0,0,  38,65,35,85,  95,0,70,0,  30,0,110,0},
  {62,9,45,170,8,65,0,0,  13,25,60,85,  65,5,95,15,  240,10,80,0},
  {57,1,28,650,5,0,1,12,  19,180,12,90,  240,18,83,12,  85,20,180,45},
  {32,0,16,7000,32,5,1400,90,  0,500,0,25,  6000,72,75,100,  7,65,6500,500}
};
}
