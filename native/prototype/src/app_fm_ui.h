#pragma once
// Two phase portraits of actual modulator/carrier samples. No UI oscillator,
// random animation, FFT or audio-thread drawing. Envelope scales the imprint.
inline void drawFm(lab::Canvas &c,const App &a){
  using namespace lab;auto v=a.engine->fmView[a.track].read();char b[80];
  const auto &tr=a.project.tracks[a.track];
  text(c,18,194,v.active?(v.gate?"EXCITED":"RELEASING"):"A / STRIKE",DIM);
  text(c,432,194,"ENVELOPES",DIM);
  constexpr int centerX=216,centerY=260;
  c.rect(26,centerY,380,1,LINE);
  const int radius=8+v.amp*43/1000;
  for(int pair=1;pair>=0;--pair){
    int dx=(pair?1:-1)*(14+v.spread*28/1000),lastX=0,lastY=0;
    for(unsigned i=0;i<FmDSP::PortraitPoints;++i){
      int x=centerX+dx+v.points[pair][i][0]*radius*3/1000;
      int y=centerY+v.points[pair][i][1]*radius/1000;
      uint32_t ink=pair?0x96999fu:TEXT;
      if(i<FmDSP::PortraitPoints/2)ink=pair?DIM:LINE;
      if(i&&v.active)organismLine(c,lastX,lastY,x,y,ink);
      lastX=x;lastY=y;
    }
    if(v.active)c.rect(lastX-2,lastY-2,4,4,pair?0x96999fu:TEXT);
  }
  if(!v.active){c.rect(centerX-15,centerY-2,4,4,TEXT);c.rect(centerX+11,centerY-2,4,4,DIM);}
  const int values[]={v.active?v.amp:0,v.active?v.colour:0,v.active?abs(v.pitch)*1000/4800:0};
  const char *labels[]={"AMP","MOD","BEND"};
  for(int i=0;i<3;++i){int y=224+i*28;text(c,432,y,labels[i],DIM);c.rect(498,y+4,122,2,LINE);c.rect(498,y+3,values[i]*122/1000,4,i==0?TEXT:0x96999fu);}
  snprintf(b,sizeof(b),"%s  /  %s",ratioName(v.active?v.ratio:tr.synthParams[2][FmRatio]),v.active?"LIVE":"IDLE");text(c,18,310,b,DIM);
  for(int bank=0;bank<5;++bank){int x=18+bank*116;bool selected=bank==a.synthBank;text(c,x,326,synthBankName(FmInstrument,bank),selected?TEXT:DIM);if(selected)c.rect(x,342,90,1,TEXT); }
}
