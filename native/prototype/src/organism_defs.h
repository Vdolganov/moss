#pragma once
namespace moss {
enum OrganismParameter {
  OrgShape, OrgCouple, OrgDensity, OrgSub,
  OrgCutoff, OrgResonance, OrgDrive, OrgNoise,
  OrgDrift, OrgRate, OrgWander, OrgBreath,
  OrgDetune, OrgInterval, OrgSpread, OrgGlide,
  OrgAttack, OrgRelease, OrgGrit, OrgNoiseTone,
  OrganismParameters
};
inline constexpr int OrganismPresets[4][OrganismParameters] = {
  {25,18,65,40,1800,18,20,4,7,8,28,25,8,7,50,300,1200,6500,10,30},
  {12,12,90,25,3200,12,12,30,11,5,55,55,14,12,85,800,2500,12000,20,35},
  {78,78,100,32,5400,48,56,18,20,35,70,45,24,7,80,120,400,8500,65,78},
  {4,0,0,75,700,10,12,0,2,4,5,8,0,0,0,400,1000,8000,0,30}
};
} // namespace moss
