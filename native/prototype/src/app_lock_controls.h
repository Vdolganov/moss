// App members: an explicit lock action never borrows a numeric sentinel gesture.
bool fieldLockCommand(int field,int action,Command &command) const {
  if(page!=Pattern||field<0||field>=4)return false;
  const auto &t=project.tracks[track];int base=0;
  if(bank==1){int at=synthBank*4+field;
    if(granularTrack()){base=t.granular.values[at];command={Op::GranularLock,track,step,at,0};}
    else if(t.instrument){base=t.synthParams[t.instrument-1][at];command={Op::SynthLock,track,step,(t.instrument-1)*SynthParameterCount+at,0};}
    else{const auto &k=t.keys[key];int values[]={k.pitch,k.start,k.end,k.level};base=values[field];command={Op::Hit,track,step,hitField(key,HitLock0+field),0};}
  }else if(bank==2){int at=clamp(fxBank,0,fxBanks(effectType())-1)*4+field;base=t.fx.values[effectType()][at];command={Op::FxLock,track,step,effectType()*EffectParameters+at,0};}
  else if(bank==3&&(field==2||(timingBank&&field==1))){command={Op::Step,track,step,field==2?StepRepeatField:StepShiftField,0};}
  else return false;
  command.value=action==0?base:action==1?Inherit:ResetLock;
  // SHIFT and REPEAT use explicit zero/OFF for their base reset.
  if(bank==3&&action==2)command.value=0;
  return true;
}
void openLockMenu(int field){Command c{};if(!fieldLockCommand(field,0,c)){say("THIS PARAMETER IS NOT A STEP LOCK");return;}lockMenu=field;lockAction=0;nav=-1;}
void applyFieldLock(int field,int action){Command c{};if(fieldLockCommand(field,action,c)){editing=false;change(c);editing=false;}}
