// Included after App and note helpers. UI works in canonical ticks, never audio time.
struct PatternGeometry {
  int columns,rows,gap,top,height,width,first,count,total;
  bool compact;
  PatternGeometry(const App &a){
    const auto &t=a.project.tracks[a.track];total=t.cells();count=total<a.gridPageSize?total:a.gridPageSize;
    compact=count>16;columns=compact?16:8;rows=compact?(count+15)/16:2;gap=compact?4:8;
    top=t.instrument?128:160;int area=t.instrument?136:120;
    width=(608-gap*(columns-1))/columns;height=(area-gap*(rows-1))/rows;
    first=(a.step/t.stride())/a.gridPageSize*a.gridPageSize;
  }
  int x(int i) const {return 16+(i%columns)*608/columns;}
  int y(int i) const {return top+i/columns*(height+gap);}
};
inline bool cellLocks(const Track &t,int tick,int key){
  const auto &s=t.steps[tick];bool locked=s.repeat!=Inherit||s.shift!=Inherit;
  if(t.instrument==GranularInstrument){for(int v:s.grainLocks)locked|=v!=Inherit;}
  else if(t.instrument){for(int v:s.synthLocks[t.instrument-1])locked|=v!=Inherit;}
  else for(int v:t.hits[key][tick].locks)locked|=v!=Inherit;
  for(int v:s.fxLocks.data)locked|=v!=Inherit;return locked;
}
// Planned one-shot duration, including pitch/trim locks on a sustaining sample.
inline double sampleDurationTicks(const App &a,int origin){
  const auto &t=a.project.tracks[a.track];const auto &k=t.keys[a.key];const auto &hit=t.hits[a.key][origin];
  int cap=hit.length?clamp(hit.length,1,t.cycle):t.cycle;
  if(!a.available(k.asset)||!a.engine->samples[k.asset].frames)return cap;
  const auto &sample=a.engine->samples[k.asset];if(!sample.rate)return cap;
  int base[]={k.pitch,k.start,k.end},values[3];for(int i=0;i<3;++i)values[i]=lockValue(hit.locks[i])?hit.locks[i]:base[i];
  double position=sample.frames*values[1]/100.,perTick=double(sample.rate)*240./a.project.bpm/64.;
  double speed=perTick*exp2f(values[0]/12.f);
  for(int age=0;age<cap;++age){
    int tick=(origin+age)%t.cycle;const auto &event=t.hits[a.key][tick];
    if(age&&event.on==NoteEvent)return age;
    if(age&&event.on==LockEvent){int oldPitch=values[0];for(int i=0;i<3;++i)if(event.locks[i]!=Inherit)values[i]=event.locks[i]==ResetLock?base[i]:event.locks[i];if(oldPitch!=values[0])speed=perTick*exp2f(values[0]/12.f);}
    int start=clamp(values[1],0,99),end=clamp(values[2],start+1,100);
    if(position<sample.frames*start/100.||position>=sample.frames*end/100.)return age;
    double left=sample.frames*end/100.-position;
    if(left<=speed)return age+left/speed;
    position+=speed;
  }
  return cap;
}
inline void drawDuration(lab::Canvas &c,const App &a,const PatternGeometry &g,int i,double *durations){
  using namespace lab;const auto &t=a.project.tracks[a.track];int tick=(g.first+i)*t.stride();
  int origin=-1,age=0;
  for(int back=0;back<t.cycle;++back){int pos=(tick-back+t.cycle)%t.cycle;
    if((t.instrument?t.steps[pos].on:t.hits[a.key][pos].on)==NoteEvent){origin=pos;age=back;break;}
  }
  if(origin<0)return;
  double length=t.instrument?t.steps[origin].length:durations[origin];
  if(!t.instrument&&length<0)durations[origin]=length=sampleDurationTicks(a,origin);
  if(length>t.cycle)length=t.cycle;if(length<=t.stride()||age>=length)return;
  int next=t.stride();for(int off=1;off<t.stride()&&tick+off<t.cycle;++off)
    if((t.instrument?t.steps[tick+off].on:t.hits[a.key][tick+off].on)==NoteEvent){next=off;break;}
  double remaining=length-age,span=remaining<next?remaining:next;
  int x=g.x(i),y=g.y(i)+g.height-(g.compact?5:12),end=x+g.width;
  bool start=age==0,finish=remaining<=t.stride()||next<t.stride();
  if(start)x+=g.compact?4:8;
  if(finish)end=g.x(i)+int(span*g.width/t.stride())-4;
  else if(i%g.columns<g.columns-1)end+=g.gap;
  if(end>x)c.rect(x,y,end-x,2,DIM);
  if(start)ui::circle(c,x,y,2,DIM,true);
  if(finish)c.rect(end,y-2,1,6,DIM);
  else if(i%g.columns==g.columns-1){c.rect(end-3,y-2,2,1,DIM);c.rect(end-3,y+3,2,1,DIM);}
  if(!start&&i%g.columns==0){
    const char *label=t.instrument?midiNote(t.steps[origin].note):note(a.key);
    int w=ui::textWidth(label,ui::Micro)+8;
    if(end-x>=w+6){ui::round(c,x+2,y-4,w,10,5,DIM);ui::text(c,x+6,y-3,label,BG,ui::Micro);}
  }
}
inline void drawPattern(lab::Canvas &c,const App &a){
  using namespace lab;const auto &t=a.project.tracks[a.track];PatternGeometry g(a);char b[120];
  if(t.instrument){snprintf(b,sizeof(b),"%02d %.32s",a.track+1,t.name);ui::text(c,16,104,b);}
  else drawSampleSlots(c,a,104);
  bool playing=__atomic_load_n(&a.engine->shownPlaying,__ATOMIC_ACQUIRE)!=0;
  int play=int(__atomic_load_n(&a.engine->shownStep[a.track],__ATOMIC_ACQUIRE))/t.stride();
  for(int i=0;i<g.count&&g.first+i<t.cells();++i){
    int cell=g.first+i,tick=cell*t.stride(),x=g.x(i),y=g.y(i);const auto &s=t.steps[tick];const auto &h=t.hits[a.key][tick];
    bool on=(t.instrument?s.on:h.on)==NoteEvent;
    bool locks=cellLocks(t,tick,a.key);bool only=!on&&(t.instrument?s.on==LockEvent:h.on==LockEvent||s.on==LockEvent);
    uint32_t fill=on?0x34363b:BG;
    ui::box(c,x,y,g.width,g.height,g.compact?2:4,tick==a.step?TEXT:LINE,fill);
    if(g.compact&&on)snprintf(b,sizeof(b),"%s",t.instrument?midiNote(s.note):note(a.key));else if(g.compact&&only)strcpy(b,"LK");else snprintf(b,sizeof(b),"%02d",cell+1);
    int inset=g.compact?4:8;
    if(playing&&cell==play)ui::round(c,x+inset-2,y+4,ui::textWidth(b,ui::Small)+4,16,2,TEXT);
    ui::text(c,x+inset,y+6,b,playing&&cell==play?BG:DIM,ui::Small);
    if(!g.compact){
      if(on||only)ui::text(c,x+8,y+24,on?(t.instrument?midiNote(s.note):note(a.key)):"LOCK",TEXT);
      if(locks||only)ui::diamond(c,x+g.width-9,y+12,4,DIM);
    }else if(locks||only){ui::diamond(c,x+g.width-5,y+g.height-6,2,TEXT);}
    bool finer=false;for(int off=1;off<t.stride()&&tick+off<t.cycle;++off)finer|=t.instrument?t.steps[tick+off].on!=0:t.hits[a.key][tick+off].on!=0||t.steps[tick+off].on==LockEvent;
    if(finer)c.rect(x+g.width-4,y+3,2,2,DIM);
  }
  double durations[Steps];for(auto &v:durations)v=-1;
  for(int i=0;i<g.count&&g.first+i<t.cells();++i)drawDuration(c,a,g,i,durations);
  int y=t.instrument?280:296;
  snprintf(b,sizeof(b),"%03d / %d  %s  1/%d",a.step/t.stride()+1,t.cells(),t.instrument?midiNote(t.steps[a.step].note):note(a.key),t.division);
  ui::text(c,16,y,b,DIM,ui::Small);
  snprintf(b,sizeof(b),"VIEW %d   < %d/%d >",a.gridPageSize,g.first/a.gridPageSize+1,(t.cells()+a.gridPageSize-1)/a.gridPageSize);
  ui::text(c,624-ui::textWidth(b,ui::Small),y,b,DIM,ui::Small);
}
inline void drawParameterTabs(lab::Canvas &c,const App &a){
  const auto &t=a.project.tracks[a.track];char names[8][64];const char *labels[8];int count=0,active=0,nested=1,index=0;
  if(a.page==Pattern){
    strcpy(names[count++],"STEP");snprintf(names[count++],64,a.bank==1?"V-LOCK ^ %d/%d":"V-LOCK ^",a.synthBank+1,t.instrument?a.voiceBanks():1);
    for(int id:t.fx.order)if(t.fx.active(id)){if(a.bank==2&&a.effectType()==id)snprintf(names[count],64,"%s LOCKS ^ %d/%d",fxName(id),a.fxBank+1,fxBanks(id));else snprintf(names[count],64,"%s LOCKS ^",fxName(id));++count;}
    if(a.bank==3)snprintf(names[count++],64,"TIME ^ %d/2",a.timingBank+1);else strcpy(names[count++],"TIME ^");
    active=a.patternTabIndex();nested=a.bank==3?2:a.bank==2?fxBanks(a.effectType()):a.bank==1?a.voiceBanks():1;index=a.bank==3?a.timingBank:a.bank==2?a.fxBank:a.synthBank;
  }else if(a.page==Sampler&&t.instrument){
    count=a.voiceBanks();active=a.synthBank;for(int i=0;i<count;++i)snprintf(names[i],64,"%s",t.instrument==GranularInstrument?GranularBanks[i]:synthBankName(t.instrument,i));
  }else if(a.fxPage()&&a.fxDetail){
    const auto f=a.effectView();count=fxBanks(f.type);active=a.fxBank;for(int i=0;i<count;++i)snprintf(names[i],64,"%s",fxBankName(i,f.type));
  }
  for(int i=0;i<count;++i)labels[i]=names[i];
  if(count)ui::tabs(c,ui::Tabs,labels,count,active);
  if(a.page==Pattern&&nested>1){for(int i=0;i<nested;++i)c.rect(632,ui::Fields+i*44/nested,2,32/nested,i==index?lab::TEXT:lab::LINE);}
}
