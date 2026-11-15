// Engine members. A bounded lookahead allows early events, including the next
// cycle's first step. Gates use the same musical clock as swung note starts.
static constexpr uint64_t NoDeadline = UINT64_MAX;
StepSchedule schedules[Tracks];
uint64_t gateEnd[Tracks][Keys] = {}, timeline = 0, nextDue = NoDeadline;
bool scheduleReady = false;
uint32_t timingEventCount[Tracks] = {}, scheduleOverflows = 0;
uint64_t tickUnit() const { return uint64_t(rate) * 60; }
void nextDeadline() {
  nextDue=NoDeadline;
  for(int t=0;t<project.trackCount;++t) {
    if(schedules[t].count && schedules[t].items[0].due<nextDue)
      nextDue=schedules[t].items[0].due;
    for(uint64_t due:gateEnd[t]) if(due && due<nextDue) nextDue=due;
  }
}
void clearGates(int t) {
  for(auto &due:gateEnd[t]) due=0;
  nextDeadline();
}
void resetSchedule() {
  scheduleReady=false;
  timeline=0;
  for(auto &s:schedules) s=StepSchedule{};
  for(auto &row:gateEnd) for(auto &due:row) due=0;
  nextDue=NoDeadline;
}
ScheduledStep scheduleItem(int t,uint64_t nominal) const {
  const auto &tr=project.tracks[t];
  int at=int(nominal%unsigned(tr.cycle));
  int64_t hundredths=int64_t(nominal)*100+stepOffset(tr,at);
  // Only the very first early event is clipped at START. On later cycles it
  // occurs before the wrap as expected. A SONG never leaks the next slot early.
  uint64_t due=hundredths<=0?0:uint64_t(hundredths)*tickUnit()/100;
  return {due,nominal,at};
}
void queueStep(int t,uint64_t nominal) {
  if(songBank) {
    const auto &slot=songBank->song.slots[songSlot];
    if(nominal>=uint64_t(slot.bars)*TicksPerBar*unsigned(slot.repeats)) return;
  }
  schedules[t].insert(scheduleItem(t,nominal));
  if(schedules[t].overflow) ++scheduleOverflows;
}
void retimePending(int t) {
  auto &schedule=schedules[t];
  ScheduledStep pending[StepSchedule::Capacity];
  int count=schedule.count;
  for(int i=0;i<count;++i) pending[i]=schedule.items[i];
  schedule.count=0;
  for(int i=0;i<count;++i) schedule.insert(scheduleItem(t,pending[i].nominal));
  nextDeadline(); // Already played events are never recreated by an edit.
}
void armGate(int t,int key,int length) {
  gateEnd[t][key]=length>0?timeline+uint64_t(length)*tickUnit():0;
}
void releaseGate(int t,int key) {
  gateEnd[t][key]=0;
  const int instrument=project.tracks[t].instrument;
  if(instrument==GranularInstrument) { grainRemaining[t]=0; granular[t].noteOff(); }
  else if(instrument) { synth[t].remaining=0; synth[t].off(); }
  else { voice(t,key).remaining=0; voice(t,key).stop(); }
}
void updateRemaining() {
  const auto unit=tickUnit();
  for(int t=0;t<project.trackCount;++t) for(int k=0;k<Keys;++k) {
    uint64_t end=gateEnd[t][k];
    if(!end) continue;
    int left=end<=timeline?0:int((end-timeline+unit-1)/unit);
    if(project.tracks[t].instrument==GranularInstrument) grainRemaining[t]=left;
    else if(project.tracks[t].instrument) synth[t].remaining=left;
    else voice(t,k).remaining=left;
  }
}
bool dispatchScheduled() {
  if(timeline<nextDue) return false;
  // Gates finish before an event at the same instant; retriggers get a fresh
  // deadline. Processing all due events also handles live edits and ties.
  for(int t=0;t<project.trackCount;++t) {
    for(int k=0;k<Keys;++k) if(gateEnd[t][k] && gateEnd[t][k]<=timeline) releaseGate(t,k);
    auto &schedule=schedules[t];
    while(schedule.count && schedule.items[0].due<=timeline) {
      const auto event=schedule.pop();
      ++timingEventCount[t];
      dispatchStep(t,event.at,event.nominal);
      __atomic_store_n(&shownStep[t],unsigned(event.at),__ATOMIC_RELEASE);
    }
  }
  nextDeadline();
  return true;
}
