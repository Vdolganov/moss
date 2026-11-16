#pragma once
// Large flat reels and a suspended tape path, following GrainBrick's metaphor.
inline void drawReel(lab::Canvas &c,const App &a,const Effect &effect) {
  using namespace lab;
  static LensGeometry geometry;geometry.init();
  auto view=a.engine->reelView[a.page==Mix?Tracks:a.track].read();
  uint32_t ink=effect.enabled?TEXT:DIM;
  float phase=float(view.phase)/(a.engine->rate*4);
  for(int reel=0;reel<2;++reel){
    int cx=190+reel*260,cy=244;
    auto point=[&](float turn,int radius,int &x,int &y){
      geometry.point(turn,float(radius),x,y);x+=cx-320;y+=cy-241;
    };
    for(int k=0;k<64;++k){int x,y,xx,yy;point(k/64.f,46,x,y);point((k+1)/64.f,46,xx,yy);grainLine(c,x,y,xx,yy,ink);}
    for(int k=0;k<3;++k)for(int i=-7;i<=7;++i){
      int x,y,xx,yy;float turn=phase+k/3.f+i*.003f;
      point(turn,16,x,y);point(turn,35,xx,yy);grainLine(c,x,y,xx,yy,ink);
    }
    for(int k=0;k<24;++k){int x,y,xx,yy;point(k/24.f,8,x,y);point((k+1)/24.f,8,xx,yy);grainLine(c,x,y,xx,yy,ink);}
    c.rect(cx-2,cy-2,5,5,ink);
  }
  int px=190,py=290;
  for(int i=1;i<=64;++i){
    float t=i/64.f;int x=190+int(260*t);
    int envelope=view.envelope[i/2<32?i/2:31]*3/1000;
    int y=290+int(17*4*t*(1-t))+clamp(envelope,0,3);
    grainLine(c,px,py,x,y,ink);px=x;py=y;
  }
  c.rect(317,304,7,7,ink);
  char label[80];snprintf(label,sizeof(label),"%s / SPEED %+.1f TO %+.1f CENTS",
    !effect.enabled?"BYPASS":view.running?"RUN":"STOP",view.low*.01,view.high*.01);
  text(c,18,173,label,DIM);
}
