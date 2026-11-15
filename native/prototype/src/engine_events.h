// Engine members: persistent per-parameter events, independent of note gates.
Step eventValues[Tracks];
int16_t voiceEventSource[Tracks][GranularParameterCount];
int16_t fxEventSource[Tracks][EffectTypes][EffectParameters];
int16_t sampleEventSource[Tracks][Keys][4];
AudioRepeat repeats[Tracks];
uint32_t lockEventCount[Tracks] = {}, repeatEventCount[Tracks] = {};
void resetEvents(int t) {
  eventValues[t]=Step{};
  memset(voiceEventSource[t],255,sizeof(voiceEventSource[t]));
  memset(fxEventSource[t],255,sizeof(fxEventSource[t]));
  memset(sampleEventSource[t],255,sizeof(sampleEventSource[t]));
  activeVoiceStep[t]=activeFxStep[t]=-1;
  repeats[t].reset();
}
void voiceEvent(int t,const Step &s,int at,bool note) {
  auto &v=eventValues[t]; const auto &tr=project.tracks[t];
  if(note) {
    for(auto &n:v.grainLocks) n=Inherit;
    for(auto &row:v.synthLocks) for(auto &n:row) n=Inherit;
    memset(voiceEventSource[t],255,sizeof(voiceEventSource[t]));
  }
  int count=tr.instrument==GranularInstrument?GranularParameterCount:8;
  for(int i=0;i<count;++i) {
    int value=tr.instrument==GranularInstrument?s.grainLocks[i]:s.synthLocks[tr.instrument-1][i];
    if(value==Inherit) continue;
    int16_t &dest=tr.instrument==GranularInstrument?v.grainLocks[i]:v.synthLocks[tr.instrument-1][i];
    dest=int16_t(value==ResetLock?Inherit:value); voiceEventSource[t][i]=int16_t(at);
  }
  activeVoiceStep[t]=at;
  if(!note && tr.instrument==GranularInstrument) {
    refreshGranular(t,&v);
    if(s.grainLocks[4]!=Inherit && grainAsset[t]>=0)
      granular[t].seek(grainValues[t][4]*.001f);
  }
}
void trackEvent(int t,const Step &s,int at,bool note) {
  auto &v=eventValues[t];
  if(note) {
    for(auto &row:v.fxLocks) for(auto &n:row) n=Inherit;
    memset(fxEventSource[t],255,sizeof(fxEventSource[t]));
    repeats[t].set(0,rate,project.bpm);
  }
  for(int type=0;type<EffectTypes;++type) for(int i=0;i<EffectParameters;++i) {
    int value=s.fxLocks[type][i]; if(value==Inherit) continue;
    v.fxLocks[type][i]=int16_t(value==ResetLock?Inherit:value);
    fxEventSource[t][type][i]=int16_t(at);
  }
  if(s.repeat!=Inherit) { repeats[t].set(s.repeat,rate,project.bpm); ++repeatEventCount[t]; }
  activeFxStep[t]=at;
}
void sampleEvent(int t,int key,const Hit &h,int at) {
  auto &v=voice(t,key);
  for(int i=0;i<4;++i) if(h.locks[i]!=Inherit) {
    v.locks[i]=int16_t(h.locks[i]==ResetLock?Inherit:h.locks[i]);
    sampleEventSource[t][key][i]=int16_t(at);
  }
  settings(t,nullptr,key,true);
}
void liveEventEdit(const Command &c) {
  if(c.track<0 || c.track>=Tracks) return;
  int t=c.track; auto &v=eventValues[t];
  int value=c.value==ResetLock?Inherit:c.value;
  if(c.op==Op::GranularLock && (voiceEventSource[t][c.field]==c.index || activeVoiceStep[t]==c.index)) {
    v.grainLocks[c.field]=int16_t(value); voiceEventSource[t][c.field]=int16_t(c.index);
  }
  if(c.op==Op::SynthLock && (voiceEventSource[t][c.field%8]==c.index || activeVoiceStep[t]==c.index)) {
    v.synthLocks[c.field/8][c.field%8]=int16_t(value); voiceEventSource[t][c.field%8]=int16_t(c.index);
  }
  if(c.op==Op::FxLock && (fxEventSource[t][c.field/EffectParameters][c.field%EffectParameters]==c.index || activeFxStep[t]==c.index)) {
    v.fxLocks[c.field/EffectParameters][c.field%EffectParameters]=int16_t(value);
    fxEventSource[t][c.field/EffectParameters][c.field%EffectParameters]=int16_t(c.index);
  }
  if(c.op==Op::Hit) {
    int key=c.field/HitFieldCount,field=c.field%HitFieldCount-HitLock0;
    if(field>=0 && field<4 && sampleEventSource[t][key][field]==c.index) {
      voice(t,key).locks[field]=int16_t(value); settings(t,nullptr,key,true);
    }
  }
}
