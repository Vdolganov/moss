#pragma once
// Actual audio snapshots: no synthetic animation clock and no audio-thread drawing.
inline void grainLine(lab::Canvas &c, int x, int y, int x2, int y2, uint32_t ink) {
  int dx = abs(x2-x), sx = x < x2 ? 1 : -1;
  int dy = -abs(y2-y), sy = y < y2 ? 1 : -1, error = dx+dy;
  for (;;) {
    c.rect(x,y,1,1,ink);
    if (x==x2 && y==y2) break;
    int twice=2*error;
    if (twice>=dy) { error+=dy; x+=sx; }
    if (twice<=dx) { error+=dx; y+=sy; }
  }
}
struct LensGeometry {
  float x[257] = {}, y[257] = {};
  bool ready = false;
  void init() { if(ready) return; for (int i=0;i<=256;++i) { float a=i*6.28318530718f/256; x[i]=sinf(a); y[i]=-cosf(a); } ready=true; }
  void point(float turn, float radius, int &px, int &py) const {
    turn-=floorf(turn); float at=turn*256; int i=int(at); float f=at-i;
    px=320+int((x[i]+(x[i+1]-x[i])*f)*radius);
    py=244+int((y[i]+(y[i+1]-y[i])*f)*radius);
  }
  void radial(lab::Canvas &c,float turn,float first,float last,uint32_t ink) const {
    int x1,y1,x2,y2; point(turn,first,x1,y1); point(turn,last,x2,y2); grainLine(c,x1,y1,x2,y2,ink);
  }
  void arc(lab::Canvas &c,float first,float last,float radius,uint32_t ink) const {
    int x1,y1; point(first,radius,x1,y1);
    int segments=lab::clamp(int((last-first)*100),2,100);
    for(int i=1;i<=segments;++i) { int x2,y2; point(first+(last-first)*i/segments,radius,x2,y2); grainLine(c,x1,y1,x2,y2,ink); x1=x2;y1=y2; }
  }
};
inline void drawGranular(lab::Canvas &c, const App &a) {
  using namespace lab;
  if (a.page!=Sampler || !a.granularTrack()) return;
  static LensGeometry lens;
  lens.init(); // UI thread only; avoids a runtime dependency on C++ static guards.
  const auto &tr=a.project.tracks[a.track]; const auto &p=tr.granular;
  auto view=a.engine->grainView[a.track].read();
  char b[100];
  text(c,18,108,"GRANULAR",TEXT,3);
  snprintf(b,sizeof(b),"%02d / %.24s",a.track+1,tr.name);
  text(c,366,112,b,DIM,2,21);
  text(c,18,144,a.assetName(p.asset),DIM,2,29);
  if(a.available(p.asset)) text(c,478,144,p.freeze?"X FROZEN":"X FREEZE",p.freeze?TEXT:DIM);
  if (a.available(p.asset)) {
    const auto &sample=a.engine->samples[p.asset];
    float head=view.running?view.head/65536.f:p.values[4]*.001f;
    lens.arc(c,0,1,70,LINE);
    // The prepared source's immutable peak overview wraps once around the ring.
    int x1=0,y1=0;
    for(int i=0;i<=128;++i) {
      int at=(i/2)%64, next=(at+1)%64;
      float peak=(sample.peaks[at]+(i%2)*(sample.peaks[next]-sample.peaks[at])*.5f)/32768.f;
      int x2,y2; lens.point(i/128.f,70+peak*8,x2,y2);
      if(i) grainLine(c,x1,y1,x2,y2,DIM); x1=x2;y1=y2;
    }
    lens.arc(c,p.values[5]*.001f,p.values[6]*.001f,80,DIM);
    for(int i=0;i<16;++i) lens.radial(c,i/16.f,84,i%4?86:89,LINE);
    lens.radial(c,p.values[5]*.001f,77,83,DIM);
    lens.radial(c,p.values[6]*.001f,77,83,DIM);
    for(unsigned i=0;i<view.count && i<8;++i) {
      const auto &g=view.grains[i]; float age=g.age/65536.f, at=g.position/65536.f, extent=g.extent/65536.f;
      float radius=14+age*50; uint32_t ink=age<.12f||age>.88f?DIM:TEXT;
      lens.arc(c,0,1,radius,LINE);
      lens.arc(c,at-extent*age,at+extent*(1-age),radius,ink);
      lens.radial(c,at,radius-2,radius+2,ink);
    }
    lens.radial(c,head,68,91,TEXT);
    snprintf(b,sizeof(b),"%.2f S",double(sample.frames)/sample.rate); text(c,18,204,"SOURCE",DIM); text(c,18,225,b,TEXT);
    text(c,490,264,p.freeze?"FROZEN":"SCAN",DIM);
    snprintf(b,sizeof(b),"%.1f%%",head*100); text(c,490,285,b,TEXT);
    text(c,18,285,midiNote(tr.audition),TEXT);
  } else {
    text(c,220,214,"NO SAMPLE",TEXT,3);
    text(c,202,259,"A ASSIGN SAMPLE",DIM);
  }
  for(int i=0;i<3;++i) {
    int x=18+i*140; text(c,x,326,GranularBanks[i],i==a.synthBank?TEXT:DIM);
    if(i==a.synthBank) c.rect(x,343,100,1,TEXT);
  }
  text(c,478,326,"L1/R1 BANK",DIM);
}
