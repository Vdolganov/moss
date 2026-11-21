import {SampleActivity} from './note-timing.js';
export const clone = value => structuredClone(value);
export const clamp = (n, lo, hi) => Math.max(lo, Math.min(hi, n));
export const uid = () => globalThis.crypto?.randomUUID?.() ?? `id-${Date.now()}-${Math.random()}`;
export const RESET = 'RESET';
export const BUTTONS = ['Y','B','A','X'];
export const NOTE_NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
export const noteName = n => NOTE_NAMES[((n%12)+12)%12] + (Math.floor(n/12)-1);
export const stride = track => 64/track.division;
export const cells = track => Math.ceil(track.cycle/stride(track));
export const emptyStep = () => ({type:0,note:48,velocity:90,length:4,probability:100,repeat:null,shift:null,locks:{},fx:{}});
export const emptyHit = () => ({type:0,velocity:90,probability:100,fadeIn:0,fadeOut:0,length:0,locks:{}});
export const getStep = (track,tick,create=false) => track.steps[tick] ?? (create ? track.steps[tick]=emptyStep() : emptyStep());
export const getHit = (track,key,tick,create=false) => track.hits[key][tick] ?? (create ? track.hits[key][tick]=emptyHit() : emptyHit());
export const selectedEvent = (t,key,tick) => t.instrument ? getStep(t,tick) : getHit(t,key,tick);
export function hasLocks(event){return Object.values(event.locks||{}).some(v=>typeof v==='object'&&v!==null?Object.values(v).some(x=>x!==null):v!==null)||Object.values(event.fx||{}).some(v=>Object.values(v).some(x=>x!==null))||event.repeat!=null||event.shift!=null;}
export function normalizeEvent(event){if(event.type!==1)event.type=hasLocks(event)?2:0;}
export function newEffects(C){return {order:C.effects.map(x=>x.id),selected:0,enabled:[],values:C.effects.map(x=>clone(x.defaults))};}
export function newTrack(C,instrument=1,name){return {id:uid(),name:name||C.instruments[instrument].name,instrument,audition:48,cycle:64,division:16,swing:50,level:100,pan:0,mute:false,
  voices:C.instruments.map(x=>clone(x.defaults)),modes:[0,0,0,0,0],presets:[-1,0,0,0,-1],grainAsset:null,freeze:false,
  keys:Array.from({length:4},()=>({asset:null,pitch:0,start:0,end:100,level:100})),steps:{},hits:Array.from({length:4},()=>({})),fx:newEffects(C)};}
export function newSequence(C,name='UNTITLED'){return {id:uid(),name,bpm:120,master:100,tracks:[],fx:newEffects(C)};}
export function newSong(name='UNTITLED'){return {id:uid(),name,bpm:120,loop:false,slots:[]};}
export function effectiveBars(sequence){return Math.max(1,Math.ceil(Math.max(0,...sequence.tracks.map(t=>t.cycle))/64));}
export function setDivision(track,division){if(![8,16,32,64].includes(division))throw Error('Invalid step division');track.division=division;}
export function setCycle(track,count){track.cycle=clamp(Math.round(count)*stride(track),1,512);}
export function toggleEvent(track,key,tick){const e=track.instrument?getStep(track,tick,true):getHit(track,key,tick,true);e.type=e.type===1?0:1;normalizeEvent(e);return e;}
export function setLock(track,key,tick,group,index,value,fx=0){
  if(value===null&&lockValue(track,key,tick,group,index,fx)===null)return;
  const e=group==='sample'?getHit(track,key,tick,true):getStep(track,tick,true);
  let target=group==='fx'?(e.fx[fx]??={}):group==='sample'?e.locks:(e.locks[track.instrument]??={});
  if(value===null)delete target[index];else target[index]=value;
  normalizeEvent(e);
}
export function lockValue(track,key,tick,group,index,fx=0){const e=group==='sample'?getHit(track,key,tick):getStep(track,tick);return (group==='fx'?e.fx[fx]:group==='sample'?e.locks:e.locks[track.instrument])?.[index]??null;}
export function effectiveValues(track,tick){const result=clone(track.voices[track.instrument]);for(const at of Object.keys(track.steps).map(Number).sort((a,b)=>a-b)){if(at>tick)break;const e=track.steps[at];if(!e.type)continue;if(e.type===1)result.splice(0,result.length,...track.voices[track.instrument]);for(const [k,v] of Object.entries(e.locks[track.instrument]||{}))result[k]=v===RESET?track.voices[track.instrument][k]:v;}return result;}
export function enableEffect(fx,id){if(fx.enabled.includes(id)){fx.enabled=fx.enabled.filter(x=>x!==id);return false;}if(fx.enabled.length>=2)throw Error('2 EFFECTS ACTIVE / BYPASS ONE FIRST');fx.enabled.push(id);return true;}
export function moveEffect(fx,id,delta){const i=fx.order.indexOf(id),j=clamp(i+delta,0,4);fx.order.splice(i,1);fx.order.splice(j,0,id);}
export function applyPreset(C,track,index){const p=C.instruments[track.instrument].presets[index];if(!p)throw Error('PRESET NOT FOUND');track.voices[track.instrument]=clone(p.values);track.modes[track.instrument]=p.mode;track.presets[track.instrument]=index;}
export function sourceFrames(source){return source.frames||Math.round(source.duration*source.rate);}
export function maxFrames(source){return Math.floor(1048576/(source.channels*2));}
export function newSlice(source,start=0,end){return {start,end:end??Math.min(sourceFrames(source),start+maxFrames(source)),fadeIn:0,fadeOut:0};}
export function splitFour(source){const size=Math.min(Math.floor(sourceFrames(source)/4),maxFrames(source));if(size<1)throw Error('SOURCE TOO SHORT');return Array.from({length:4},(_,i)=>newSlice(source,i*size,i===3&&sourceFrames(source)<=4*maxFrames(source)?sourceFrames(source):(i+1)*size));}
export function moveSlice(source,cut,delta){const length=cut.end-cut.start;cut.start=clamp(Math.round(cut.start+delta),0,sourceFrames(source)-length);cut.end=cut.start+length;}
export function trimSlice(source,cut,edge,delta){if(edge===0){cut.start=clamp(Math.round(cut.start+delta),0,cut.end-1);if(cut.end-cut.start>maxFrames(source))cut.end=cut.start+maxFrames(source);}else{cut.end=clamp(Math.round(cut.end+delta),cut.start+1,sourceFrames(source));if(cut.end-cut.start>maxFrames(source))cut.start=cut.end-maxFrames(source);}}
export function arrangeSlices(source,cuts){const lengths=cuts.map(c=>c.end-c.start),total=lengths.reduce((a,b)=>a+b,0);if(total>sourceFrames(source))throw Error('PARTS DO NOT FIT SOURCE');let at=Math.min(cuts[0].start,sourceFrames(source)-total);cuts.forEach((c,i)=>{c.start=at;c.end=at+lengths[i];at=c.end;});}
export function syntheticWave(seed=1,n=1024){let s=seed;const values=[];for(let i=0;i<n;++i){s=(Math.imul(s,1664525)+1013904223)>>>0;const a=.1+.6*Math.abs(Math.sin(i*.023+seed)*Math.cos(i*.009))+s/4294967296*.2;values.push([-a,a*(.7+.3*Math.sin(i*.1)**2)]);}return values;}
export function makeAsset(name,seconds=2,folder='',seed=1){return {id:uid(),name,folder,rate:32000,channels:2,frames:Math.floor(seconds*32000),bytes:Math.floor(seconds*128000),wave:syntheticWave(seed),virtual:true};}
export function freshState(C){
  const data={schema:'moss-interface-0.12',sequences:[],songs:[],assets:[],kits:[],imports:[],deleted:null,drafts:{}};
  const kit={id:uid(),name:'SOFT DRUMS',assets:[]};for(const [i,name] of ['LOW KICK','PAPER SNARE','SOFT HAT','WOOD'].entries()){const a=makeAsset(name,.45+i*.15,kit.name,i+12);data.assets.push(a);kit.assets.push(a.id);}data.kits.push(kit);
  for(const [i,name]of ['WARM AIR','TAPE CHORD','SLOW DUST'].entries())data.assets.push(makeAsset(name,6+i, '',i+31));
  data.imports=[{...makeAsset('CASSETTE A',90,'Ambient Cassette Loops',3),path:'Ambient Cassette Loops/CASSETTE A.wav'}, {...makeAsset('ROOM TONE',22,'Ambient Cassette Loops/Textures',7),path:'Ambient Cassette Loops/Textures/ROOM TONE.wav'}, {...makeAsset('DRUM BREAK',8,'Drums',22),path:'Drums/DRUM BREAK.wav'}];
  const p=newSequence(C,'SLOW SIGNAL');p.bpm=70;p.tracks=[newTrack(C,2,'DRONE'),newTrack(C,1,'PULSE'),newTrack(C,4,'DUST')];p.tracks[2].grainAsset=data.assets[6].id;
  p.tracks[0].steps[0]={...emptyStep(),type:1,note:31,length:64};for(const at of [0,20,40])p.tracks[1].steps[at]={...emptyStep(),type:1,note:55+at/20*3,length:8};p.tracks[2].steps[0]={...emptyStep(),type:1,note:48,length:64};p.tracks.forEach(t=>t.fx.enabled=[3,1]);data.sequences.push(p);
  const fm=newSequence(C,'ELASTIC MATTER');fm.bpm=106;fm.tracks=['THUMP','SNAP','RUBBER','CLOUD'].map((name,i)=>{const t=newTrack(C,3,name);applyPreset(C,t,i+4);t.cycle=i<2?64:i===2?128:256;t.audition=i===1?50:i===3?45:33;t.level=[90,72,68,52][i];if(i===3){t.steps[0]={...emptyStep(),type:1,note:45,length:256};for(let k=32;k<256;k+=32)t.steps[k]={...emptyStep(),type:2,locks:{3:{0:20+k/8,13:55+k/8}}};}else for(let k=i===1?16:0;k<t.cycle;k+=i===2?12:16)t.steps[k]={...emptyStep(),type:1,note:i===1?50:i===2?33+(k%5):33,length:i===1?4:12};if(i>=2)t.fx.enabled=[0,1];return t;});data.sequences.push(fm);
  return data;
}
export function validateData(C,data){
  const fail=()=>{throw Error('INVALID INTERFACE PROJECT / ORIGINAL DATA KEPT');};
  if(!data||data.schema!=='moss-interface-0.12'||!Array.isArray(data.sequences)||data.sequences.length>32||!Array.isArray(data.songs)||data.songs.length>32||!Array.isArray(data.assets)||data.assets.length>256||!Array.isArray(data.kits)||!Array.isArray(data.imports))fail();
  const unique=list=>new Set(list.map(x=>x.id)).size===list.length;if(!unique(data.sequences)||!unique(data.songs)||!unique(data.assets))fail();
  const name=x=>typeof x==='string'&&x.length>0&&x.length<=24;
  for(const p of data.sequences){if(!name(p.name)||!Number.isInteger(p.bpm)||p.bpm<30||p.bpm>240||!Array.isArray(p.tracks)||p.tracks.length>4)fail();for(const t of p.tracks){if(!name(t.name)||!C.instruments[t.instrument]||!Number.isInteger(t.cycle)||t.cycle<1||t.cycle>512||![8,16,32,64].includes(t.division)||t.keys?.length!==4||t.hits?.length!==4||t.voices?.length!==5)fail();for(let inst=0;inst<5;++inst){if(t.voices[inst]?.length!==C.instruments[inst].specs.length)fail();t.voices[inst].forEach((v,i)=>{const s=C.instruments[inst].specs[i];if(!Number.isFinite(v)||v<s.min||v>s.max)fail();});}for(const [key,e]of Object.entries(t.steps)){if(!Number.isInteger(+key)||+key<0||+key>=512||![0,1,2].includes(e.type)||e.note<24||e.note>96)fail();}}}
  const num=(v,lo,hi)=>Number.isFinite(v)&&v>=lo&&v<=hi;
  const int=(v,lo,hi)=>Number.isInteger(v)&&num(v,lo,hi);
  const id=v=>typeof v==='string'&&v.length>0&&v.length<=160;
  const object=v=>v&&typeof v==='object'&&!Array.isArray(v);
  const specs=(values,defs)=>Array.isArray(values)&&values.length===defs.length&&values.every((v,i)=>num(v,defs[i].min,defs[i].max));
  const locks=(values,defs)=>{if(!object(values))fail();for(const [k,v]of Object.entries(values))if(!int(+k,0,defs.length-1)||(v!==null&&v!==RESET&&!num(v,defs[k].min,defs[k].max)))fail();};
  const fx=f=>{if(!object(f)||!Array.isArray(f.order)||f.order.length!==5||new Set(f.order).size!==5||f.order.some(v=>!int(v,0,4))||!Array.isArray(f.enabled)||f.enabled.length>2||new Set(f.enabled).size!==f.enabled.length||f.enabled.some(v=>!int(v,0,4))||!Array.isArray(f.values)||f.values.length!==5)fail();f.values.forEach((v,i)=>{if(!specs(v,C.effects[i].specs))fail();});};
  for(const p of data.sequences){if(!id(p.id)||!num(p.master,0,100)||!unique(p.tracks))fail();fx(p.fx);for(const t of p.tracks){if(!id(t.id)||!int(t.audition,24,96)||!int(t.swing,50,75)||!num(t.level,0,100)||!num(t.pan,-100,100)||typeof t.mute!=='boolean'||t.modes?.length!==5||t.presets?.length!==5||!t.modes.every(v=>int(v,0,1))||!object(t.steps))fail();fx(t.fx);for(const k of t.keys)if(!object(k)||(k.asset!==null&&!id(k.asset))||!num(k.pitch,-24,24)||!num(k.start,0,99)||!num(k.end,1,100)||k.start>=k.end||!num(k.level,0,100))fail();
    for(const [at,e]of Object.entries(t.steps)){if(!object(e)||!int(+at,0,511)||!int(e.type,0,2)||!int(e.note,24,96)||!num(e.velocity,0,127)||!num(e.length,1,512)||!num(e.probability,0,100)||![null,0,8,16,32,64].includes(e.repeat)||!(e.shift===null||e.shift===RESET||num(e.shift,-50,50))||!object(e.locks)||!object(e.fx))fail();for(const [i,v]of Object.entries(e.locks)){if(!int(+i,0,4))fail();locks(v,C.instruments[i].specs);}for(const [i,v]of Object.entries(e.fx)){if(!int(+i,0,4))fail();locks(v,C.effects[i].specs);}}
    for(const lane of t.hits){if(!object(lane))fail();for(const [at,h]of Object.entries(lane)){if(!int(+at,0,511)||!object(h)||!int(h.type,0,2)||!num(h.velocity,0,127)||!num(h.probability,0,100)||!num(h.length,0,512)||!num(h.fadeIn,0,5000)||!num(h.fadeOut,0,5000))fail();locks(h.locks,C.instruments[0].specs);}}
  }}
  for(const s of data.songs)if(!id(s.id)||!name(s.name)||!int(s.bpm,30,240)||typeof s.loop!=='boolean'||s.slots?.length>64||!Array.isArray(s.slots)||!unique(s.slots)||s.slots.some(x=>!id(x.id)||!id(x.sequence)||!int(x.bars,1,64)||!int(x.repeats,1,64)))fail();
  for(const asset of [...data.assets,...data.imports]){if(!object(asset)||!id(asset.id)||!name(asset.name)||!int(asset.rate,8000,192000)||![1,2].includes(asset.channels)||!int(asset.frames,1,2**40)||!Array.isArray(asset.wave)||!asset.wave.length||asset.wave.length>65536||asset.wave.some(p=>!Array.isArray(p)||p.length!==2||p.some(v=>!num(v,-1,1)))||typeof asset.folder!=='string'||(asset.path!==undefined&&typeof asset.path!=='string')||(asset.waveR&&(!Array.isArray(asset.waveR)||asset.waveR.length>65536||asset.waveR.some(p=>!Array.isArray(p)||p.length!==2||p.some(v=>!num(v,-1,1))))))fail();}
  for(const asset of data.assets)if(asset.frames*asset.channels*2>1048576)fail();
  for(const k of data.kits)if(!object(k)||!id(k.id)||!name(k.name)||!Array.isArray(k.assets)||k.assets.length!==4||k.assets.some(x=>!id(x)))fail();
  if(data.deleted!==null&&(!object(data.deleted)||!['songs','sequences'].includes(data.deleted.kind)||!object(data.deleted.value)))fail();
  data.drafts??={};if(!object(data.drafts))fail();
  for(const [key,d] of Object.entries(data.drafts)){
    const source=[...data.imports,...data.assets].find(x=>x.id===key);
    if(!source){delete data.drafts[key];continue;}
    if(!object(d)||d.source!==key||typeof d.four!=='boolean'||!Array.isArray(d.cuts)||!d.cuts.length||d.cuts.length>16||(d.four&&d.cuts.length!==4))fail();
    for(const c of d.cuts)if(!object(c)||!int(c.start,0,source.frames-1)||!int(c.end,c.start+1,source.frames)||c.end-c.start>maxFrames(source)||!num(c.fadeIn,0,2**40)||!num(c.fadeOut,0,2**40))fail();
  }
  return data;
}
export function residentBytes(data,sequences){const ids=new Set();for(const p of sequences)for(const t of p.tracks){if(t.instrument===0)t.keys.forEach(k=>{if(k.asset)ids.add(k.asset);});if(t.instrument===4&&t.grainAsset)ids.add(t.grainAsset);}return [...ids].reduce((sum,id)=>{const a=data.assets.find(x=>x.id===id);return sum+(a?a.frames*a.channels*2:0);},0);}
export function assetUsed(data,id){return [...data.sequences,...(data.deleted?.kind==='sequences'?[data.deleted.value]:[])].some(p=>p.tracks.some(t=>t.grainAsset===id||t.keys.some(k=>k.asset===id)));}
export function sequenceUsed(data,id){return data.songs.some(s=>s.slots.some(slot=>slot.sequence===id));}
export function addKit(data,source,cuts,name){if(data.assets.length+cuts.length>256)throw Error('LIBRARY FULL / 256 SAMPLES');if(data.kits.some(k=>k.name===name))throw Error('FOLDER NAME EXISTS');const kit={id:uid(),name,assets:[]},assets=cuts.map((cut,i)=>{const frames=cut.end-cut.start;if(frames<1||frames>maxFrames(source))throw Error('PART EXCEEDS 1 MIB');const a={id:uid(),name:`${name.slice(0,20)} ${i+1}`,folder:name,rate:source.rate,channels:source.channels,frames,bytes:frames*source.channels*2,wave:cutWave(source,cut),waveR:source.waveR?cutWave(source,cut,source.waveR):null,sourceOffset:(source.sourceOffset??source.range?.start??0)+cut.start,virtual:source.virtual,sourceId:source.sourceId??source.id,range:clone(cut)};kit.assets.push(a.id);return a;});data.assets.push(...assets);data.kits.push(kit);return kit;}
export function cutWave(source,cut,wave=source.wave){const lo=Math.floor(cut.start/sourceFrames(source)*wave.length),hi=Math.max(lo+1,Math.ceil(cut.end/sourceFrames(source)*wave.length));return clone(wave.slice(lo,hi));}
export function scheduleOffset(track,at){const s=stride(track),cell=Math.floor(at/s),event=getStep(track,at),swing=cell%2?(track.swing-50)/50*s:0;return at+swing+(event.shift==null||event.shift===RESET?0:event.shift*s/100);}
export class Session {
  constructor(C,data=freshState(C)){this.C=C;this.data=data;this.undoData=null;this.tick=0;this.playing=false;this.preview=null;this.lastEvents=new Map();this.voiceEvents=new Map();this.samples=new SampleActivity();this.songSlot=0;this.songRepeat=0;this.songElapsed=0;this.freshStart=true;this.random=Math.random;}
  checkpoint(){this.undoData=clone(this.data);}
  undo(){if(!this.undoData)return false;[this.data,this.undoData]=[this.undoData,null];return true;}
  sequence(id){return this.data.sequences.find(p=>p.id===id);}
  song(id){return this.data.songs.find(s=>s.id===id);}
  start(){this.playing=true;this.tick=0;this.songElapsed=0;this.songRepeat=0;this.freshStart=true;this.lastEvents.clear();this.voiceEvents.clear();this.samples.clear();}
  stop(){this.playing=false;this.preview=null;this.lastEvents.clear();this.voiceEvents.clear();this.samples.clear();}
  window(delta,sequence,bpm=sequence.bpm){const before=this.freshStart?-1e-8:this.tick,end=this.tick+delta;this.freshStart=false;
    for(const t of sequence.tracks){if(t.mute){this.samples.finish(t,end,bpm);continue;}const events=[];const collect=(key,at,e,sample=false)=>{if(!e.type||at>=t.cycle)return;const offset=scheduleOffset(t,at),cycle=Math.floor((end-offset)/t.cycle),when=cycle*t.cycle+offset;if(cycle>=0&&when>before&&when<=end&&this.random()*100<(e.probability??100))events.push({key,at,e,when,sample});};
      for(const [at,e]of Object.entries(t.steps))if(t.instrument||e.type===2)collect(t.id,+at,e);if(t.instrument===0)for(const [k,lane]of t.hits.entries())for(const [at,e]of Object.entries(lane))collect(`${t.id}:${k}`,+at,e,true);
      events.sort((a,b)=>a.when-b.when);for(const {key,at,e,when,sample}of events){const event={at,type:e.type,when,note:e.note,length:e.length};this.lastEvents.set(key,event);if(e.type===1)this.voiceEvents.set(key,event);if(sample)this.samples.event(t,+key.slice(key.lastIndexOf(':')+1),e,when,bpm,this.data.assets);}
      this.samples.finish(t,end,bpm);
    }this.tick=end;
  }
  advance(ms,sequence,song){if(!this.playing||!sequence)return;let delta=ms*(song?.bpm??sequence.bpm)*16/60000;if(!song){this.window(delta,sequence);return;}let guard=0;
    while(delta>1e-9&&this.playing&&guard++<4096){const slot=song.slots[this.songSlot],seq=slot&&this.sequence(slot.sequence);if(!seq){this.stop();break;}const remaining=slot.bars*64-this.songElapsed,chunk=Math.min(delta,remaining);this.window(chunk,seq,song.bpm);this.songElapsed+=chunk;delta-=chunk;if(this.songElapsed>=slot.bars*64-1e-9){this.songElapsed=0;if(++this.songRepeat>=slot.repeats){this.songRepeat=0;this.songSlot++;this.tick=0;this.freshStart=true;this.lastEvents.clear();this.voiceEvents.clear();this.samples.clear();if(this.songSlot>=song.slots.length){if(song.loop)this.songSlot=0;else this.stop();}}}}
  }
}

export function fineWindow(source,cut,zoom,center){const length=cut.end-cut.start,width=Math.min(source.frames,Math.max(16,Math.floor((length+Math.floor(length/8)+2)/2**zoom)));const first=Math.floor(clamp(center-Math.floor(width/2),0,source.frames-width));return[first,first+width];}
