#pragma once
// Reuse the lens' cached circle geometry. Only the visible effect is drawn.
inline void drawReel(lab::Canvas &c,const App &a,const Effect &effect) {
  using namespace lab;
  static LensGeometry geometry;geometry.init();
  auto view=a.engine->reelView[a.page==Mix?Tracks:a.track].read();
  uint32_t ink=effect.enabled?TEXT:DIM;
  float phase=float(view.phase)/(a.engine->rate*4);
  for(int reel=0;reel<2;++reel) {
    int cx=74+reel*166,cy=270;
    auto point=[&](float turn,int radius,int &x,int &y) {
      geometry.point(turn,float(radius),x,y);x+=cx-320;y+=cy-241;
    };
    for(int k=0;k<32;++k) {
      int x,y,x2,y2;point(k/32.f,25,x,y);point((k+1)/32.f,25,x2,y2);
      grainLine(c,x,y,x2,y2,ink);
    }
    c.box(cx-4,cy-4,9,9,ink);
    for(int k=0;k<3;++k) {
      int x,y,x2,y2;point(phase+k/3.f,10,x,y);point(phase+k/3.f,21,x2,y2);
      grainLine(c,x,y,x2,y2,ink);
    }
  }
  grainLine(c,74,295,132,295,ink);grainLine(c,132,295,146,285,ink);
  c.box(146,277,22,10,ink);
  grainLine(c,168,285,182,295,ink);grainLine(c,182,295,240,295,ink);
  for(int i=0;i<ReelSnapshot::History;++i) {
    int x=78+i*5,y=x<132?295:x<146?295-(x-132)*10/14:x<168?285:x<182?285+(x-168)*10/14:295;
    int height=clamp(view.envelope[i]*20/1000,0,5);
    if(height)c.rect(x,y-height,2,height*2+1,ink);
    int noise=clamp(view.noise[i]*20/1000,0,5);if(noise)c.rect(x,y-noise,2,noise*2+1,DIM);
  }
  c.box(146,277,22,10,ink);
  text(c,34,228,!effect.enabled?"TAPE BYPASS":view.running?"TAPE RUN":"TAPE STOP",DIM);
  c.rect(296,232,1,72,LINE);
  text(c,316,230,"SPEED / CENTS",DIM);
  c.rect(322,264,276,1,LINE);
  // The range is measured across the AUDIO block; 30 Hz flutter cannot alias
  // into a misleading slow UI oscillator. +/-45 cents includes hard-lock slew.
  for(int i=0;i<ReelSnapshot::History;++i) {
    int low=clamp(view.speedLow[i],-4500,4500),high=clamp(view.speedHigh[i],-4500,4500);
    int top=264-high*17/4500,bottom=264-low*17/4500;
    c.rect(322+i*8,top,3,maximum(1,bottom-top+1),i==ReelSnapshot::History-1?ink:DIM);
  }
  char label[64];snprintf(label,sizeof(label),"%+.1f TO %+.1f",view.low*.01,view.high*.01);
  text(c,316,282,label,ink);
  text(c,34,302,"OUTPUT / X4",DIM);
}
