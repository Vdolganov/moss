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
    py=241+int((y[i]+(y[i+1]-y[i])*f)*radius);
  }
  void radial(lab::Canvas &c,float turn,float first,float last,uint32_t ink) const {
    int x1,y1,x2,y2; point(turn,first,x1,y1); point(turn,last,x2,y2); grainLine(c,x1,y1,x2,y2,ink);
  }
  void arc(lab::Canvas &c,float first,float last,float radius,uint32_t ink) const {
    int x1,y1; point(first,radius,x1,y1);
    int segments=lab::clamp(int((last-first)*100),2,100);
    for(int i=1;i<=segments;++i) { int x2,y2; point(first+(last-first)*i/segments,radius,x2,y2); grainLine(c,x1,y1,x2,y2,ink); x1=x2;y1=y2; }
  }
  void grainArc(lab::Canvas &c,float origin,float span,float first,float last,float radius,uint32_t ink) const {
    if (last<=first || span<=0) return;
    if (span>=last-first) { arc(c,first,last,radius,ink); return; }
    float end=origin+span;
    arc(c,origin,fminf(end,last),radius,ink);
    if(end>last) arc(c,first,first+end-last,radius,ink);
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
    float first=view.running?view.first/65536.f:p.values[5]*.001f;
    float last=view.running?view.last/65536.f:p.values[6]*.001f;
    // Absolute source coordinates: the whole waveform stays in place while
    // START/END dim the excluded samples. No decorative moving rings.
    for(int i=0;i<128;++i) {
      int at=(i/2)%64, next=(at+1)%64;
      float peak=(sample.peaks[at]+(i%2)*(sample.peaks[next]-sample.peaks[at])*.5f)/32768.f;
      float position=i/128.f, amplitude=1+peak*9;
      lens.radial(c,position,67-amplitude,67+amplitude,
                  position>=first && position<last ? DIM : LINE);
    }
    lens.radial(c,first,64,79,TEXT);
    lens.radial(c,last,64,79,TEXT);
    bool looped=false;
    for(unsigned i=0;i<view.count && i<8;++i) {
      const auto &g=view.grains[i];
      float at=g.position/65536.f, origin=g.origin/65536.f;
      float radius=27+g.lane*4;
      uint32_t ink=g.gain>12000 ? TEXT : DIM;
      float arcOrigin=origin,span=g.span/65536.f,range=(g.last-g.first)/65536.f;
      if(g.reverse && range>0){arcOrigin-=fminf(span,range);if(arcOrigin<g.first/65536.f)arcOrigin+=range;}
      lens.grainArc(c,arcOrigin,span,g.first/65536.f,g.last/65536.f,radius,ink);
      lens.radial(c,origin,radius-1,radius+1,DIM);
      int x,y; lens.point(at,radius,x,y);
      c.rect(x-2,y-2,5,5,BG); c.rect(x-1,y-1,3,3,TEXT);
      int ax,ay;lens.point(at+(g.reverse?-.012f:.012f),radius,ax,ay);grainLine(c,x,y,ax,ay,TEXT);
      looped|=g.loops>0;
    }
    int hx,hy; lens.point(head,80,hx,hy); c.box(hx-2,hy-2,5,5,TEXT);
    snprintf(b,sizeof(b),"%.2f S",double(sample.frames)/sample.rate); text(c,18,204,"SOURCE",DIM); text(c,18,225,b,TEXT);
    text(c,478,204,looped?"LOOP GRAINS":"GRAINS",DIM);
    unsigned reverse=0;for(unsigned i=0;i<view.count && i<8;++i)reverse+=view.grains[i].reverse;
    snprintf(b,sizeof(b),"%u / 8",view.count); text(c,478,225,b,TEXT);
    snprintf(b,sizeof(b),"%u F / %u R",view.count-reverse,reverse);text(c,478,245,b,DIM);
    text(c,478,264,p.freeze?"HEAD HELD":"HEAD",DIM);
    snprintf(b,sizeof(b),"%.1f%%",head*100); text(c,478,285,b,TEXT);
    snprintf(b,sizeof(b),"%.1f-%.1f%%",first*100,last*100); text(c,18,264,b,DIM);
    if(view.running) { snprintf(b,sizeof(b),"%.1f G/S",view.density*.001); text(c,18,285,b,TEXT); }
    else text(c,18,285,midiNote(tr.audition),TEXT);
  } else {
    text(c,220,214,"NO SAMPLE",TEXT,3);
    text(c,202,259,"A ASSIGN SAMPLE",DIM);
  }
  for(int i=0;i<GranularBankCount;++i) {
    int x=18+i*120; text(c,x,326,GranularBanks[i],i==a.synthBank?TEXT:DIM);
    if(i==a.synthBank) c.rect(x,343,100,1,TEXT);
  }
}
