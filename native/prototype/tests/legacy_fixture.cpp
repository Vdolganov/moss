// Build once with the preserved 0.5.0 headers, never with current headers.
#include <storage.h>
using namespace moss;
int main(int argc,char **argv) {
  if(argc!=2 || Tracks!=8 || Keys!=12)return 2;
  static Project p;
  p.trackCount=4; strcpy(p.name,"MOS5 FIXTURE");
  const int assigned[]={0,3,7,11};
  for(int n=0;n<4;++n) {
    auto &t=p.tracks[n];
    t.fx.enabled=1;for(int &v:t.fx.enabledTypes)v=1;
    t.fx.order[0]=3;t.fx.order[1]=1;t.fx.order[2]=0;t.fx.order[3]=2;
    for(int i=0;i<4;++i) {
      t.keys[assigned[i]].asset=i;t.keys[assigned[i]].pitch=i*3;
      t.hits[assigned[i]][i*4].on=1;t.hits[assigned[i]][i*4].velocity=70+i;
    }
  }
  return saveProject(argv[1],p)?0:3;
}
