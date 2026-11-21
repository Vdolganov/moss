// App members. Inspect only the affected region on the UI thread. A full undo
// snapshot is taken once per accepted gesture, never for every repeated tick.
bool commandChanges(const Command &c){
  if(c.op==Op::Global)return (c.field?project.master:project.bpm)!=c.value;
  if(c.op==Op::FxEnable){const auto &f=c.track==Tracks?project.fx:project.tracks[c.track].fx;return f.active(c.index)!=(c.value!=0);}
  if(c.op==Op::Fx){const auto &f=c.track==Tracks?project.fx:project.tracks[c.track].fx;
    if(c.field==0)return f.type!=c.value;
    if(c.field==1)return bool(f.enabled)!=(c.value!=0);
    if(c.field!=FxMoveField)return f.values[c.index][c.field-2]!=c.value;
    Effect next=f;next.move(c.index,c.value);return memcmp(&next,&f,sizeof(f))!=0;
  }
  if(c.track>=0&&c.track<Tracks){const auto &t=project.tracks[c.track];
    if(c.op==Op::Track){int v[]={t.cycle,t.level,t.pan,t.mute,t.division,t.swing};return c.field>=0&&c.field<6&&v[c.field]!=c.value;}
    if(c.op==Op::Key){const auto &k=t.keys[c.index];int v[]={k.asset,k.pitch,k.start,k.end,k.level,k.mode};return c.field>=0&&c.field<6&&v[c.field]!=c.value;}
    if(c.op==Op::SynthParam)return t.synthParams[c.index][c.field]!=c.value;
    if(c.op==Op::SynthMode)return t.synthMode[c.index]!=c.value;
    if(c.op==Op::AuditionNote)return t.audition!=c.value;
    if(c.op==Op::Instrument)return t.instrument!=c.value;
    if(c.op==Op::GranularParam)return t.granular.values[c.field]!=c.value;
    if(c.op==Op::GranularSource)return t.granular.asset!=c.value;
    if(c.op==Op::GranularFreeze)return t.granular.freeze!=c.value;
    bool event=c.op==Op::Step||c.op==Op::Hit||c.op==Op::SynthLock||c.op==Op::GranularLock||c.op==Op::FxLock||c.op==Op::SetEventType||c.op==Op::ClearEvent||c.op==Op::ClearTrackLocks||c.op==Op::ClearAllAtStep;
    if(event){
      if(c.index<0||c.index>=Steps)return false;
      auto &next=editCandidate.tracks[c.track];next.instrument=t.instrument;next.steps[c.index]=t.steps[c.index];
      for(int k=0;k<Keys;++k)next.hits[k][c.index]=t.hits[k][c.index];
      apply(editCandidate,c);
      if(memcmp(&next.steps[c.index],&t.steps[c.index],sizeof(Step)))return true;
      for(int k=0;k<Keys;++k)if(memcmp(&next.hits[k][c.index],&t.hits[k][c.index],sizeof(Hit)))return true;
      return false;
    }
  }
  editCandidate=project;apply(editCandidate,c);return memcmp(&editCandidate,&project,sizeof(Project))!=0;
}
