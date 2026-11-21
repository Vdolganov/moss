// Silent UI timing: source position and gate length, with no audio processing.
const EPS=1e-8;
const clamp=(n,lo,hi)=>Math.max(lo,Math.min(hi,n));
function settings(track,key,overrides){
 const k=track.keys[key],p=[k.pitch,k.start,k.end,k.level];
 for(const [i,value] of Object.entries(overrides))if(Number.isFinite(value))p[i]=value;
 p[0]=clamp(p[0],-24,24);p[1]=clamp(p[1],0,99);p[2]=clamp(p[2],p[1]+1,100);return p;
}
export function applySampleLocks(voice,track,key,locks={}){
 for(const [i,value] of Object.entries(locks))if(value!=null){if(Number.isFinite(value))voice.overrides[i]=value;else delete voice.overrides[i];}
 voice.params=settings(track,key,voice.overrides);
 voice.first=Math.floor(voice.frames*voice.params[1]/100);voice.last=Math.floor(voice.frames*voice.params[2]/100);
 if(voice.position<voice.first||voice.position>=voice.last)voice.active=false;
}
export function createSampleVoice(track,key,event,asset,when=0){
 if(!asset?.frames||!asset.rate)return null;
 const voice={asset:asset.id,frames:asset.frames,rate:asset.rate,overrides:{},params:[],position:0,at:when,when,gate:event.length>0?when+event.length:Infinity,active:true};
 // Prepare the crop before positioning a new one-shot at its start.
 applySampleLocks(voice,track,key,event.locks);voice.position=voice.first;voice.active=voice.last>voice.first;return voice;
}
export function advanceSampleVoice(voice,to,bpm){
 if(!voice?.active||to<=voice.at)return voice;
 const speed=voice.rate*2**(voice.params[0]/12)*60/(bpm*16);
 const end=Math.min(to,voice.gate,voice.at+(voice.last-voice.position)/speed);
 voice.position+=Math.max(0,end-voice.at)*speed;voice.at=end;
 if(end>=voice.gate-EPS||voice.position>=voice.last-EPS)voice.active=false;
 return voice;
}
export class SampleActivity{
 constructor(){this.voices=new Map();}
 clear(){this.voices.clear();}
 get(track,key){const v=this.voices.get(`${track}:${key}`);return v?.active?v:null;}
 event(track,key,event,when,bpm,assets){
  const id=`${track.id}:${key}`;
  if(event.type===1){const v=createSampleVoice(track,key,event,assets.find(a=>a.id===track.keys[key].asset),when);if(v)this.voices.set(id,v);else this.voices.delete(id);}
  else {const v=this.get(track.id,key);if(v){advanceSampleVoice(v,when,bpm);if(v.active)applySampleLocks(v,track,key,event.locks);}}
 }
 finish(track,to,bpm){
  for(let k=0;k<track.keys.length;k++){
   const id=`${track.id}:${k}`,v=this.voices.get(id);if(!v)continue;
   if(track.mute||track.instrument!==0||track.keys[k].asset!==v.asset){this.voices.delete(id);continue;}
   advanceSampleVoice(v,to,bpm);if(v.active)applySampleLocks(v,track,k);
   if(!v.active)this.voices.delete(id);
  }
 }
}
// Planned spans assume written triggers/locks fire. Playback activity uses the
// scheduler's actual accepted events, including probability, swing and SHIFT.
export function noteSpans(track,key,assets,bpm){
 const sampler=track.instrument===0,timeline=sampler?track.hits[key]:track.steps,cycle=track.cycle;
 const events=Object.entries(timeline).map(([at,e])=>({at:+at,event:e})).filter(x=>x.at<cycle&&x.event.type>0).sort((a,b)=>a.at-b.at);
 const notes=events.filter(x=>x.event.type===1),locks=events.filter(x=>x.event.type===2),result=[];
 for(let i=0;i<notes.length;i++){
  const {at,event}=notes[i],next=notes[i+1]?.at??notes[0].at+cycle;
  let end=Math.min(at+event.length,next);
  if(sampler){
   const v=createSampleVoice(track,key,event,assets.find(a=>a.id===track.keys[key].asset),at);if(!v)continue;
   for(const offset of [0,cycle])for(const lock of locks){const when=lock.at+offset;if(when<=at||when>=next)continue;advanceSampleVoice(v,when,bpm);if(v.active)applySampleLocks(v,track,key,lock.event.locks);}
   advanceSampleVoice(v,next,bpm);end=v.at;
  }
  if(end<=at+EPS)continue;
  const span={origin:at,length:end-at,note:sampler?48+key:event.note};
  // At most one previous iteration is needed: a lane retriggers every cycle.
  for(const offset of [-cycle,0])if(end+offset>0&&at+offset<cycle)result.push({...span,start:at+offset,end:end+offset});
 }
 return result.sort((a,b)=>a.start-b.start);
}
export function continuationAt(spans,tick){return spans.find(s=>s.start<tick-EPS&&s.end>tick+EPS)||null;}
