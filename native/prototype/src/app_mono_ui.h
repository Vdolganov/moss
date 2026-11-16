#pragma once
// Included in moss, after App. Only bounded mailbox data crosses audio/UI.
inline void drawMono(lab::Canvas &c,const App &a){
  using namespace lab;const auto &tr=a.project.tracks[a.track];auto v=a.engine->monoView[a.track].read();char b[80];
  snprintf(b,sizeof(b),"VOICE / %d MS",v.milliseconds?v.milliseconds:32);text(c,18,184,b,DIM);
  c.rect(18,251,386,1,LINE);
  for(int i=0;i<MonoDSP::ScopePoints-1;++i){
    int x0=18+i*386/63,x1=18+(i+1)*386/63;
    int y0=251-clamp(v.wave[i]*48/32767,-48,48),y1=251-clamp(v.wave[i+1]*48/32767,-48,48);
    for(int x=x0;x<=x1;++x){int y=y0+(y1-y0)*(x-x0)/(x1-x0);c.rect(x,y,1,2,v.active?TEXT:DIM);}
  }
  text(c,432,184,"VCF NOW",DIM);
  snprintf(b,sizeof(b),"%d HZ",v.active?v.cutoff:tr.synthParams[0][0]);text(c,432,207,b,TEXT);
  text(c,432,240,"AMP",DIM);text(c,432,276,"FILTER EG",DIM);
  c.rect(432,261,190,2,LINE);c.rect(432,297,190,2,LINE);
  c.rect(432,261,clamp(v.active?v.amp:0,0,1000)*190/1000,2,TEXT);
  c.rect(432,297,clamp(v.active?v.filter:0,0,1000)*190/1000,2,TEXT);
  snprintf(b,sizeof(b),"PRE FX / %s",!v.active?"IDLE":v.gate?"GATE":"RELEASE");text(c,18,306,b,DIM);
  text(c,300,306,"LFO",DIM);c.rect(348,313,56,1,LINE);
  c.rect(374+clamp(v.active?v.lfo:0,-1000,1000)*26/1000,310,3,7,v.active?TEXT:DIM);
  for(int i=0;i<5;++i){int x=18+i*120;text(c,x,326,synthBankName(MonoInstrument,i),a.synthBank==i?TEXT:DIM);if(a.synthBank==i)c.rect(x,343,100,1,TEXT);}
}
