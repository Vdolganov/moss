#pragma once
// Four living contours, each driven by one carrier's measured energy and
// slow modulation. This is a voice portrait, not a fabricated oscilloscope.
inline void organismLine(lab::Canvas &c,int x0,int y0,int x1,int y1,uint32_t color,int width=1) {
  int dx=abs(x1-x0),dy=-abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx+dy;
  for(;;){c.rect(x0,y0,width,width,color);if(x0==x1&&y0==y1)break;int twice=2*err;if(twice>=dy){err+=dy;x0+=sx;}if(twice<=dx){err+=dx;y0+=sy;}}
}
inline void drawOrganism(lab::Canvas &c,const App &a) {
  using namespace lab;const auto &tr=a.project.tracks[a.track];const int *p=tr.synthParams[1];
  auto v=a.engine->organismView[a.track].read();const auto &tables=a.engine->tables.mono;
  auto sine=[&](float phase){return tables.sinCycle(SynthTables::wrap(phase));};
  float amp=v.active?v.amp*.001f:0,shape=v.active?v.shape*.001f:p[OrgShape]*.03f;
  float coupling=v.active?v.couple*.001f:p[OrgCouple]*.00135f;
  text(c,18,192,v.active?(v.gate?"GROW":"FADE"):"REST",DIM);
  for(int voice=3;voice>=0;--voice) {
    float phase=v.active?v.phase[voice]*.001f:voice*.23f;
    float energy=v.active?v.energy[voice]*.001f:0;
    float weight=voice==0?1:clamp(p[OrgDensity]*3-(voice-1)*100,0,100)*.01f;
    float xCenter=320+(v.active?v.pan[voice]*.014f:0)+sine(phase)*amp*13;
    float yCenter=252+(voice-1.5f)*5+sine(phase+.25f)*amp*6;
    float rx=158+voice*17+amp*10,ry=22+voice*5+energy*18;
    uint32_t ink=weight<.01f?LINE:!v.active?0x65676d:voice==0?TEXT:voice==1?0xc6c8cc:voice==2?0x96999f:0x73767e;
    int firstX=0,firstY=0,lastX=0,lastY=0;
    for(int point=0;point<=144;++point){
      float angle=point/144.f;
      float fold=sine(angle*2+phase)*coupling*1.6f+sine(angle*3-phase)*shape*.022f;
      float grain=v.active?sine(angle*19+phase)*v.grit*.00011f:0;
      int x=int(xCenter+sine(angle+.25f)*rx*(1+fold*.32f+grain));
      float height=sine(angle)*ry*(1+fold)+sine(angle*2+phase)*amp*9;
      int y=int(yCenter+height/(1+fabsf(height)*.007f));
      if(!point){firstX=lastX=x;firstY=lastY=y;}
      else organismLine(c,lastX,lastY,x,y,ink,voice==0&&energy>.12f?2:1);
      lastX=x;lastY=y;
    }
    // Small carrier witness: follows the actual slow phase, never wall time.
    if(v.active&&energy>.005f)c.rect(firstX-1,firstY-1,3,3,ink);
  }
  text(c,18,306,"4 VOICES",DIM);
  text(c,524,306,"SUB",DIM);c.rect(568,312,54,1,LINE);
  c.rect(568,311,int((v.active?v.sub*.001f*amp:p[OrgSub]*.007f)*54/.7f),2,v.active?TEXT:DIM);
  for(int bank=0;bank<5;++bank){int x=18+bank*120;text(c,x,326,synthBankName(OrganismInstrument,bank),a.synthBank==bank?TEXT:DIM);if(a.synthBank==bank)c.rect(x,343,108,1,TEXT);}
}
