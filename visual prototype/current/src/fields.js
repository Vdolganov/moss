import {fxLockPage} from './nested-pages.js';
import {clamp,cells,stride,getStep,getHit,lockValue,setLock,normalizeEvent,RESET,noteName,setCycle,setDivision,sourceFrames,fineWindow,trimSlice,moveSlice} from './model.js';
const field=(label,value,unit,edit,format)=>({label,value,unit,edit,fine:1,coarse:10,text:format?format(value):value===null?'--':String(value)});
const numeric=(obj,key,label,min,max,coarse=10,unit='',format)=>({...field(label,obj[key],unit,(d,big,n)=>obj[key]=clamp(obj[key]+d*(big?coarse:1)*n,min,max),format),coarse});
const enumField=(label,value,list,edit,unit='',format=x=>String(x))=>({...field(label,value,unit,(d,big,n)=>edit(list[clamp(list.indexOf(value)+d*n,0,list.length-1)]),format),fineLabel:'NEXT',coarseLabel:'NEXT',coarse:1});
// Display abbreviations only: keep native units and exact values in the descriptors.
const COMPACT_UNITS={PERCENT:'%',STEPS:'stps',MS:'ms',HZ:'Hz',Hz:'Hz',SEC:'s',CENTS:'ct',SEMITONES:'st',X:'x','GRAINS/S':'gr/s',BIT:'bit',HOLD:'x',FRAMES:'fr',BARS:'bars','TRACK CELLS':'stps',TIMES:'x','PER REPEAT':'/rep','L / R':'L/R',
 'DARK-BRIGHT':'','MOD:CARRIER':'',SYNC:'','WHOLE TRACK':'','STEP LOCK':'',RESOLUTION:'','TRACK AUDIO':'','STEP EVENT':'',TRACK:'',SLOT:'',SONG:'',RESAMPLE:'','ONE SHOT':'','NO NOTE':''};
export function compactUnit(f){
 if(['--','RESET','RST','OFF','FULL'].includes(f.text))return'';
 // The selected cut's byte size is already shown above the waveform.
 if(/^MS \/ \d+K$/.test(f.unit))return'ms';
 return COMPACT_UNITS[f.unit]??f.unit;
}
export function formatValue(C,kind,id,i,v){if(v==null)return'--';if(v===RESET)return'RST';if(kind==='voice'){if(id===3&&i===1)return C.ratios?.[v]??String(v);if((id===1&&i===16)||(id===2&&i===9)||(id===3&&i===16))return(v*.01).toFixed(2);if(id===4){if(i===0)return(v*.01).toFixed(2);if(i>=4&&i<=6)return(v*.1).toFixed(1);}}else{if(id===0&&i===4&&v===0)return'OFF';if(id===0&&i===0)return['1/16','1/8','3/16','1/4','3/8','1/2','3/4','1 BAR'][v];if(id===4&&i===4)return['SOFT','HARD','FOLD'][v];if(id===3){if([1,2,4,6,15].includes(i))return(v/10).toFixed(1);if(i===5)return(v*.01).toFixed(2);if(i===7)return(v*.1).toFixed(1);if(i===12)return v===0?'OFF':String(v-73);}if(id===1&&i===1)return (.3*Math.pow(133.333333,v/100)).toFixed(1);}return String(v);}
function makeFields(a){const {ui:u,C}=a,p=a.sequence(),t=a.track();if(u.dialog||!p&&u.page!=='song')return[];
 const tick=t?u.cell*stride(t):0,s=t?getStep(t,tick):null,h=t?getHit(t,u.key,tick):null;
 const specFields=(kind,id,bank,values,locked=false)=>{
  const def=(kind==='voice'?C.instruments:C.effects)[id];
  return def.specs.slice(bank*4,bank*4+4).map((spec,col)=>{
   const i=bank*4+col,group=kind==='fx'?'fx':t.instrument===0?'sample':'voice';
   const value=locked?lockValue(t,u.key,tick,group,i,id):values[i];
   const set=v=>{if(locked)setLock(t,u.key,tick,group,i,v,id);else{if(values[i]===v)return;values[i]=v;if(kind==='voice'){t.presets[id]=-1;if(id===4&&i===5)values[5]=Math.min(values[5],values[6]-1);if(id===4&&i===6)values[6]=Math.max(values[6],values[5]+1);}}};
   const result=field(spec.label,value,spec.unit,(d,big,n)=>{
    const current=locked?lockValue(t,u.key,tick,group,i,id):values[i];
    set(clamp((typeof current==='number'?current:values[i])+d*(big?spec.coarse:1)*n,spec.min,spec.max));
   },v=>formatValue(C,kind,id,i,v));
   result.coarse=spec.coarse;
   const unit=COMPACT_UNITS[spec.unit]??spec.unit,scaled=kind==='voice'&&((id===1&&i===16)||(id===2&&i===9)||(id===3&&i===16)||(id===4&&[0,4,5,6].includes(i)))||kind==='fx'&&id===3&&[1,2,4,5,6,7,15].includes(i);
   result.fineLabel=(scaled?formatValue(C,kind,id,i,1):'1')+unit;result.coarseLabel=(scaled?formatValue(C,kind,id,i,spec.coarse):spec.coarse)+unit;
   if(kind==='fx'&&(id===0&&i===0||id===4&&i===4)||kind==='voice'&&id===3&&i===1){result.fineLabel='NEXT';result.coarseLabel=spec.coarse===1?'NEXT':`${spec.coarse} OPTIONS`;}
   if(kind==='fx'&&id===1&&i===1){result.fineLabel='1 UNIT';result.coarseLabel=`${spec.coarse} UNITS`;}
   result.scope=group==='sample'?`SLOT ${noteName(48+u.key)}`:'TRACK';
   if(locked)result.lock={value,base:values[i],baseText:formatValue(C,kind,id,i,values[i]),set};
   return result;
  });
 };
 if(u.page==='pattern'){
  if(u.bank===0){const event=t.instrument||!h.type&&s.type===2?s:h;return t.instrument?[numeric(s,'note','NOTE',24,96,12,'',noteName),numeric(s,'velocity','VEL',0,127,10),field('NOTE LEN',Math.ceil(s.length/stride(t)),'STEPS',(d,big,n)=>s.length=clamp(s.length+d*(big?10:1)*n*stride(t),stride(t),512)),numeric(s,'probability','PROB',0,100,10,'PERCENT')]:[numeric(h,'velocity','VELOCITY',0,127),numeric(h,'fadeIn','FADE IN',0,5000,100,'MS'),field('NOTE LEN',h.length?Math.ceil(h.length/stride(t)):'FULL',h.length?'STEPS':'ONE SHOT',(d,big,n)=>h.length=clamp(h.length+d*(big?10:1)*n*stride(t),0,512)),numeric(event,'probability','PROB',0,100,10,'PERCENT')];}
  if(u.bank===1){if(t.instrument===0){const k=t.keys[u.key],vals=[k.pitch,k.start,k.end,k.level];return specFields('voice',0,0,vals,true);}return specFields('voice',t.instrument,u.voiceBank,t.voices[t.instrument],true);}
  if(u.bank===2){const page=fxLockPage(a);return page?specFields('fx',page.effect,page.bank,t.fx.values[page.effect],true):[];}
  const e=t.instrument?s:h;return [u.timingBank?numeric(t,'swing','SWING',50,75,5,'WHOLE TRACK',v=>`${v}%`):field('CYCLE',cells(t),'TRACK CELLS',(d,big,n)=>{setCycle(t,cells(t)+d*(big?16:1)*n);u.cell=clamp(u.cell,0,cells(t)-1);}),u.timingBank?field('SHIFT',s.shift,'PERCENT',(d,big,n)=>{s.shift=clamp((typeof s.shift==='number'?s.shift:0)+d*(big?10:1)*n,-50,50);normalizeEvent(s);},v=>v==null?'--':v===RESET?'RST':`${v>0?'+':''}${v}`):enumField('DIV',t.division,[8,16,32,64],v=>{const at=u.cell*stride(t);setDivision(t,v);u.cell=clamp(Math.floor(at/stride(t)),0,cells(t)-1);},'RESOLUTION',v=>`1/${v}`),enumField('REPEAT',s.repeat,[null,0,8,16,32,64],v=>{s.repeat=v;normalizeEvent(s);},'TRACK AUDIO',v=>v==null?'--':v===0?'OFF':`1/${v}`),enumField('TYPE',e.type,[0,1,2],v=>e.type=v,'STEP EVENT',v=>['EMPTY','NOTE','LOCK'][v])];
 }
 if(u.page==='voice'){if(t.instrument===0){const k=t.keys[u.key];return [numeric(k,'pitch','PITCH',-24,24,12,'SEMITONES'),field('START',k.start,'PERCENT',(d,big,n)=>k.start=clamp(k.start+d*(big?10:1)*n,0,k.end-1)),field('END',k.end,'PERCENT',(d,big,n)=>k.end=clamp(k.end+d*(big?10:1)*n,k.start+1,100)),numeric(k,'level','LEVEL',0,100,10,'PERCENT')];}return specFields('voice',t.instrument,u.voiceBank,t.voices[t.instrument]);}
 if((u.page==='space'||u.page==='mix'&&u.mixTab===1)&&u.fxDetail){const fx=a.effectChain();return specFields('fx',u.fxId,u.fxBank,fx.values[u.fxId]);}
 if(u.page==='mix'&&u.mixTab===0&&t)return[numeric(t,'level','LEVEL',0,100,10,'PERCENT'),numeric(t,'pan','PAN',-100,100,10,'L / R'),numeric(p,'master','MASTER',0,100,10,'PERCENT'),enumField('MUTE',Number(t.mute),[0,1],v=>t.mute=!!v,'TRACK',v=>v?'ON':'OFF')];
 if(u.page==='song'){const song=a.song(),slot=song?.slots[u.row];if(!song)return[];return [slot?field('POSITION',u.row+1,'SLOT',(d,big,n)=>a.reorderSong((big?-d:d)*n)):{...field('POSITION','--','',()=>{}),disabled:true},slot?numeric(slot,'repeats','REPEATS',1,64,8,'TIMES'):{...field('REPEATS','--','',()=>{}),disabled:true},slot?numeric(slot,'bars','BARS',1,64,4,'PER REPEAT'):{...field('BARS','--','',()=>{}),disabled:true},enumField('LOOP',Number(song.loop),[0,1],v=>song.loop=!!v,'SONG',v=>v?'ON':'OFF')];}
 if(u.page==='tape'){u.captureSource=clamp(u.captureSource,0,p.tracks.length);
  if(u.tapeTab===0)return[enumField('SOURCE',u.captureSource,Array.from({length:p.tracks.length+1},(_,i)=>i),v=>u.captureSource=v,'RESAMPLE',v=>v===0?'MASTER':p.tracks[v-1].name.slice(0,10)),numeric(u,'captureBars','LENGTH',1,8,4,'BARS')];
  if(u.tapeTab===1&&a.source()){const source=a.source(),cut=u.cuts[u.cut];if(!cut)return[];
   if(u.four&&u.sliceBank===0)return u.cuts.map((c,i)=>field(`PART ${i+1}`,Math.floor(c.start*1000/source.rate),`MS / ${Math.ceil((c.end-c.start)*source.channels*2/1024)}K`,(d,big,n)=>{a.selectCut(i);moveSlice(source,c,Math.max(1,Math.round(source.rate*.001))*(big?100:1)*d*n);u.preview=null;}));
   if(u.four&&u.sliceBank===2)return[field('START',cut.start,'FRAMES',(d,big,n)=>{trimSlice(source,cut,0,d*n*(big?10:1));u.fineCenter=cut.start;}),field('END',cut.end,'FRAMES',(d,big,n)=>{trimSlice(source,cut,1,d*n*(big?10:1));u.fineCenter=cut.end;}),numeric(u,'zoom','ZOOM',0,Math.max(0,Math.min(14,Math.floor(Math.log2(Math.min(source.frames,cut.end-cut.start+(cut.end-cut.start)/8+2)/16)))),1,'X',v=>String(2**v)),field('VIEW',Math.floor(fineWindow(source,cut,u.zoom,u.fineCenter)[0]*1000/source.rate),'MS',(d,big,n)=>u.fineCenter=clamp(u.fineCenter+d*n*(big?10:1)*Math.max(1,Math.floor((cut.end-cut.start)/(2**u.zoom)/100)),0,sourceFrames(source)))];
   return[field('START',Math.floor(cut.start*1000/source.rate),'MS',(d,big,n)=>trimSlice(source,cut,0,d*n*(big?100:1)*Math.max(1,Math.round(source.rate/1000)))),field('END',Math.floor(cut.end*1000/source.rate),'MS',(d,big,n)=>trimSlice(source,cut,1,d*n*(big?100:1)*Math.max(1,Math.round(source.rate/1000)))),numeric(cut,'fadeIn','FADE IN',0,Math.floor((cut.end-cut.start)/source.rate*1000),100,'MS'),numeric(cut,'fadeOut','FADE OUT',0,Math.floor((cut.end-cut.start)/source.rate*1000),100,'MS')];
  }
 }return[];
}

export function fields(a){
 let result=makeFields(a);const u=a.ui,t=a.track();
 result=result.map(f=>{if(f.fineLabel)return f;let coarse=f.coarse;if(f.label==='CYCLE')coarse=16;if(u.page==='tape'&&u.tapeTab===1&&(u.four&&u.sliceBank===0||(!u.four||u.sliceBank===1)&&['START','END'].includes(f.label)))coarse=100;if(f.label==='POSITION')coarse=1;const unit=compactUnit({...f,text:'1'});return{...f,coarse,fineLabel:f.label==='VIEW'?'PAN':f.label==='ZOOM'?'2×':`1${unit}`,coarseLabel:f.label==='VIEW'?'10× PAN':f.label==='ZOOM'?'2×':`${coarse}${unit}`};});
 if(u.page==='pattern'&&u.bank===0&&t){const at=u.cell*stride(t),e=t.instrument?getStep(t,at):getHit(t,u.key,at);if(e.type===2||!e.type&&getStep(t,at).type===2)result=result.map((f,i)=>i<3?{...f,text:'--',unit:'NO NOTE',disabled:true}:f);}
 if(u.page==='pattern'&&u.bank===3&&t){const at=u.cell*stride(t),e=getStep(t,at);result=result.map(f=>{
  const key=f.label==='SHIFT'?'shift':f.label==='REPEAT'?'repeat':null;if(!key)return f;
  return {...f,scope:'TRACK',lock:{value:e[key],base:0,baseText:key==='repeat'?'OFF':'0',reset:key==='repeat'?0:RESET,set:v=>{const step=getStep(t,at,true);step[key]=v;normalizeEvent(step);}}};
 });}
 return result.map((f,i)=>({...f,edit:(d,big,n)=>{
  if(f.disabled)return;
  const t=a.track(),u=a.ui,at=t?u.cell*stride(t):0;
  const oldStep=t?.steps[at],oldHit=t?.hits[u.key]?.[at];
  if(u.page==='pattern'&&t){getStep(t,at,true);if(!t.instrument)getHit(t,u.key,at,true);}
  const stepBefore=t?JSON.stringify(t.steps[at]):null,hitBefore=t?JSON.stringify(t.hits[u.key]?.[at]):null;
  makeFields(a)[i]?.edit(d,big,n);
  if(u.page==='pattern'&&t){if(!oldStep&&JSON.stringify(t.steps[at])===stepBefore)delete t.steps[at];if(!oldHit&&JSON.stringify(t.hits[u.key]?.[at])===hitBefore)delete t.hits[u.key][at];}
 }}));
}
