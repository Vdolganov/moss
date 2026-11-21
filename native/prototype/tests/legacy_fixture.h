#pragma once
// The old wire layout had eight ORGANISM values in a different order.
static moss::Project legacyProjectFixture;
inline void legacyFields(moss::Codec &codec,const moss::Project &project,int version) {
  auto &old=legacyProjectFixture;old=project;
  const int original[]={30,35,15,65,55,8,450,2500};
  for(auto &t:old.tracks)memcpy(t.synthParams[1],original,sizeof(original));
  codec.fields(old,true,version);
}
