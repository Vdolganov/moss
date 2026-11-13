(function(root){
 'use strict';
 const clone=x=>JSON.parse(JSON.stringify(x));
 const uid=()=>Date.now().toString(36)+'-'+Math.random().toString(36).slice(2,8);
 const clamp=(v,a,b)=>Math.min(b,Math.max(a,v));
 const step=()=>({on:false,note:43,velocity:90,gate:1,probability:100,locks:{},fxLocks:{}});
 const engines=['ORGANISM','MONO','SAMPLER'];
 const effect=(enabled=true)=>({type:'ORBIT',enabled,time:3,feedback:42,mix:28,spread:65,size:72,decay:65,damping:35,drive:20,bits:12,rate:1});
 function track(engine='ORGANISM',name=engine){return {id:uid(),name,engine,kit:{keys:{}},fx:effect(false),mute:false,solo:false,level:75,pan:0,send:30,params:{tension:34,drift:62,chaos:8,density:75,cutoff:650,resonance:30,attack:120,release:2500},tape:{sample:null,slices:8,slice:0,pitch:-12,speed:50,reverse:false,loop:true,source:'MASTER',bars:4},lengths:[16,16,16,16],patterns:Array.from({length:4},()=>Array.from({length:64},step))};}
 function snapshot(p){return clone({tracks:p.tracks,pattern:p.pattern,length:p.length,bpm:p.bpm,fx:p.fx});}
 function project(name='UNTITLED'){return {id:uid(),name,bpm:120,key:'G MINOR',length:16,pattern:0,tracks:[],fx:effect(),samples:[],capture:{source:'MASTER',bars:4},scenes:Array(4).fill(null),activeScene:-1};}
 function demo(){const p=project('SLOW SIGNAL');p.bpm=70;p.length=64;p.tracks=[track('ORGANISM','DRONE'),track('MONO','PULSE'),track('TAPE','DUST')];p.tracks.forEach(tr=>tr.lengths.fill(64));p.tracks[0].patterns[0][0]={...step(),on:true,gate:64};[[0,62,100],[20,65,65],[44,58,80],[60,55,35]].forEach(([i,n,prob])=>Object.assign(p.tracks[1].patterns[0][i],{on:true,note:n,probability:prob}));p.tracks[1].patterns[0][44].locks.cutoff=1400;[8,32,52].forEach(i=>Object.assign(p.tracks[2].patterns[0][i],{on:true,note:43,gate:8,probability:65}));p.tracks[2].tape.sample={name:'DRONE / CAPTURE 01',duration:13.714,source:'DRONE + FX',seed:37,simulated:true};p.samples.push({...clone(p.tracks[2].tape.sample),id:uid()});p.scenes[0]={name:'ARRIVAL',data:snapshot(p)};p.activeScene=0;return p;}
 const note=n=>['C','C#','D','D#','E','F','F#','G','G#','A','Bb','B'][n%12]+(Math.floor(n/12)-1);
 function restoreScene(p,i){if(!p.scenes[i])return false;Object.assign(p,clone(p.scenes[i].data));p.activeScene=i;return true;}
 // Strict bounds keep imported UI state finite, predictable and portable.
 function validate(p){
  const num=(n,a,b)=>typeof n==='number'&&Number.isFinite(n)&&n>=a&&n<=b;
  const integer=(n,a,b)=>Number.isInteger(n)&&num(n,a,b);
  const str=(s,max=40)=>typeof s==='string'&&s.length>0&&s.length<=max;
  const fx=f=>f&&(f.enabled===undefined||typeof f.enabled==='boolean')&&['ORBIT','POND','CRUSH'].includes(f.type)&&(f.drive===undefined||num(f.drive,0,100))&&(f.bits===undefined||integer(f.bits,2,16))&&(f.rate===undefined||integer(f.rate,1,32))&&integer(f.time,0,7)&&['feedback','mix','spread','size','decay','damping'].every(k=>num(f[k],0,100));
  const validTrack=t=>t&&(t.lengths===undefined||(Array.isArray(t.lengths)&&t.lengths.length===4&&t.lengths.every(n=>integer(n,1,64))))&&str(t.id,80)&&str(t.name,24)&&[...engines,'TAPE'].includes(t.engine)&&typeof t.mute==='boolean'&&typeof t.solo==='boolean'&&num(t.level,0,100)&&num(t.pan,-100,100)&&num(t.send,0,100)&&t.params&&['tension','drift','chaos','density','resonance'].every(k=>num(t.params[k],0,100))&&num(t.params.cutoff,20,12000)&&num(t.params.attack,0,5000)&&num(t.params.release,0,10000)&&t.tape&&[1,2,4,8,16].includes(t.tape.slices)&&integer(t.tape.slice,0,t.tape.slices-1)&&num(t.tape.pitch,-24,24)&&num(t.tape.speed,25,200)&&typeof t.tape.reverse==='boolean'&&typeof t.tape.loop==='boolean'&&str(t.tape.source,80)&&integer(t.tape.bars,1,8)&&(!t.tape.sample||(str(t.tape.sample.name,80)&&num(t.tape.sample.duration,0,1000)&&str(t.tape.sample.source,80)&&num(t.tape.sample.seed,0,100000)&&t.tape.sample.simulated===true))&&Array.isArray(t.patterns)&&t.patterns.length===4&&t.patterns.every(a=>Array.isArray(a)&&a.length===64&&a.every(s=>s&&typeof s.on==='boolean'&&integer(s.note,0,127)&&num(s.velocity,1,127)&&integer(s.gate,1,64)&&num(s.probability,0,100)&&s.locks&&typeof s.locks==='object'&&!Array.isArray(s.locks)&&Object.entries(s.locks).every(([k,v])=>['tension','drift','chaos','density','cutoff','resonance','attack','release','pitch','speed','slice','level','start','end'].includes(k)&&num(v,k==='pitch'?-48:0,k==='cutoff'?12000:k==='release'?10000:k==='attack'?5000:k==='speed'?200:k==='slice'?15:(k==='start'||k==='end')?86400000:100))));
  const fxBounds={ORBIT:{time:[0,7],feedback:[0,95],mix:[0,100],spread:[0,100]},POND:{size:[0,100],decay:[0,100],damping:[0,100],mix:[0,100]},CRUSH:{drive:[0,100],bits:[2,16],rate:[1,32],mix:[0,100]}};
  const validFxLocks=st=>st.fxLocks===undefined||(st.fxLocks&&typeof st.fxLocks==='object'&&!Array.isArray(st.fxLocks)&&Object.entries(st.fxLocks).every(([type,bank])=>fxBounds[type]&&bank&&typeof bank==='object'&&!Array.isArray(bank)&&Object.entries(bank).every(([key,val])=>fxBounds[type][key]&&integer(val,...fxBounds[type][key]))));
  const data=d=>d&&num(d.bpm,30,240)&&integer(d.length,1,64)&&integer(d.pattern,0,3)&&fx(d.fx)&&Array.isArray(d.tracks)&&d.tracks.length<=8&&d.tracks.every(t=>validTrack(t)&&t.patterns.every(pat=>pat.every(validFxLocks)))&&new Set(d.tracks.map(t=>t.id)).size===d.tracks.length;
  if(!p||!str(p.id,80)||!str(p.name,24)||!['G MINOR','C MAJOR','D MINOR','A MINOR','CHROMATIC'].includes(p.key)||!data(p)||!integer(p.activeScene,-1,3)||!Array.isArray(p.scenes)||p.scenes.length!==4||!p.scenes.every(s=>s===null||(s&&str(s.name,24)&&data(s.data))))throw Error('Это не проект MOSS v1 или значения выходят за допустимые пределы.');
  const validSample=x=>x&&str(x.name,80)&&num(x.duration,0,1000)&&str(x.source,80)&&num(x.seed,0,100000)&&x.simulated===true;
  const allTracks=[...p.tracks,...p.scenes.flatMap(sc=>sc?sc.data.tracks:[])];
  if(allTracks.some(t=>t.fx&&(!fx(t.fx)||(t.fx.enabled!==undefined&&typeof t.fx.enabled!=='boolean'))))throw Error('Неверные эффекты дорожки.');
  if(p.samples!==undefined&&(!Array.isArray(p.samples)||p.samples.some(x=>!validSample(x)||!str(x.id,80))||new Set(p.samples.map(x=>x.id)).size!==p.samples.length))throw Error('Неверная память TAPE.');
  if(p.capture!==undefined&&(!str(p.capture.source,80)||!integer(p.capture.bars,1,8)))throw Error('Неверные настройки capture.');
  const result=clone(p);
  for(const data of [result,...result.scenes.filter(Boolean).map(sc=>sc.data)])for(const tr of data.tracks){tr.lengths??=Array(4).fill(data.length);for(const pat of tr.patterns)for(const st of pat)st.fxLocks??={};}
  for(const tr of [...result.tracks,...result.scenes.flatMap(sc=>sc?sc.data.tracks:[])])tr.fx={...effect(false),...tr.fx};
  result.fx={...effect(),...result.fx};
  for(const sc of result.scenes)if(sc)sc.data.fx={...effect(),...sc.data.fx};
  result.capture??={source:'MASTER',bars:4};
  if(!result.samples){result.samples=[];for(const tr of result.tracks)if(tr.tape.sample)result.samples.push({...clone(tr.tape.sample),id:uid()});}
  return result;
 }
 const length=(p,tr)=>tr?.lengths?.[p.pattern]??p.length;
 const position=(p,tr,tick)=>Math.floor(tick)%length(p,tr);
 const setLength=(p,tr,n)=>{if(!Number.isInteger(n)||n<1||n>64)throw Error('Length must be 1–64');tr.lengths??=Array(4).fill(p.length);tr.lengths[p.pattern]=n;};
 const api={length,position,setLength,clone,uid,clamp,step,track,project,demo,snapshot,restoreScene,note,validate,engines,effect};
 if(typeof module!=='undefined')module.exports=api;else root.Moss=api;
})(typeof globalThis!=='undefined'?globalThis:this);

/* v0.4 sample metadata model. No audio is decoded or stored. */
(function(M){
 'use strict';
 const RAM_BUDGET=8*1024*1024,ASSET_LIMIT=1024*1024;
 const tracksOf=p=>[...p.tracks,...p.scenes.flatMap(sc=>sc?sc.data.tracks:[])];
 function asset(name,durationMs,extra={}){
  const rate=extra.rate||44100,channels=extra.channels||1,frames=extra.frames||Math.max(1,Math.round(durationMs*rate/1000));
  return {id:M.uid(),name:String(name).slice(0,80),rate,channels,frames,durationMs:Math.max(1,Math.round(frames*1000/rate)),bytes:frames*channels*2,origin:'SIMULATED',seed:37,simulated:true,...extra};
 }
 function validateAsset(a){
  if(!a||typeof a.id!=='string'||!a.id||a.id.length>120||typeof a.name!=='string'||!a.name||a.name.length>80||!Number.isInteger(a.rate)||a.rate<8000||a.rate>192000||![1,2].includes(a.channels)||!Number.isInteger(a.frames)||a.frames<1||a.frames>192000*3600*24||!Number.isInteger(a.durationMs)||Math.abs(a.durationMs-Math.round(a.frames*1000/a.rate))>1||a.bytes!==a.frames*a.channels*2||!Number.isFinite(a.seed)||typeof a.origin!=='string'||a.origin.length>160||a.simulated!==true)throw Error('Неверные метаданные сэмпла');
  return M.clone(a);
 }
 function assignment(a,extra={}){return {assetId:a.id,pitch:0,start:0,end:a.durationMs,level:80,playMode:'GATE',...extra};}
 const validateOld=M.validate;
 M.validate=function(p){
  const q=validateOld(p);
  for(const tr of tracksOf(q)){
   tr.kit??={keys:{}};
   if(!tr.kit||!tr.kit.keys||Array.isArray(tr.kit.keys)||typeof tr.kit.keys!=='object')throw Error('Неверная карта SAMPLER');
   for(const [key,a] of Object.entries(tr.kit.keys)){
    if(!/^\d+$/.test(key)||+key<0||+key>127||!a||typeof a.assetId!=='string'||!a.assetId||a.assetId.length>120||!Number.isFinite(a.pitch)||a.pitch< -48||a.pitch>48||!Number.isInteger(a.start)||!Number.isInteger(a.end)||a.start<0||a.end<=a.start||a.end>86400000||!Number.isFinite(a.level)||a.level<0||a.level>100||!['ONE SHOT','GATE','LOOP'].includes(a.playMode))throw Error('Неверное назначение SAMPLER');
   }
  }
  if(q.tapeDraft){const d=q.tapeDraft;validateAsset(d.source);if(!Array.isArray(d.slices)||!d.slices.length||d.slices.length>256||d.slices.some(c=>!Number.isInteger(c.start)||!Number.isInteger(c.end)||c.start<0||c.end>d.source.durationMs||c.start>=c.end||!Number.isInteger(c.fadeIn)||!Number.isInteger(c.fadeOut)||c.fadeIn<0||c.fadeOut<0||c.fadeIn+c.fadeOut>c.end-c.start))throw Error('Неверные границы TAPE');}
  return q;
 };
 function migrate(p,library){
  p=M.validate(p);
  const getLegacy=sm=>{const name=sm.name.slice(0,80),durationMs=Math.max(1,Math.round(sm.duration*1000));let a=library.find(a=>a.name===name&&Math.abs(a.durationMs-durationMs)<=1&&a.seed===sm.seed);if(!a){a=asset(name,durationMs,{origin:'LEGACY / '+sm.source,seed:sm.seed});library.push(a);}return a;};
  for(const sm of p.samples)getLegacy(sm);
  for(const tr of tracksOf(p))if(tr.engine==='TAPE'){
   tr.legacyTape=M.clone(tr.tape);tr.engine='SAMPLER';tr.kit={keys:{}};
   if(!tr.tape.sample)continue;
   const a=getLegacy(tr.tape.sample),pitch=tr.tape.pitch+12*Math.log2(tr.tape.speed/100);
   const notes=[...new Set(tr.patterns.flat().filter(st=>st.on).map(st=>st.note))];if(!notes.length)notes.push(48);
   for(const key of notes)tr.kit.keys[key]=assignment(a,{pitch,playMode:tr.tape.loop?'LOOP':'GATE'});
   // Preserve original settings and locks verbatim; translate slice/speed for this metadata prototype.
   for(const pattern of tr.patterns)for(const st of pattern){
    if(!Object.keys(st.locks).length)continue;st.legacyLocks=M.clone(st.locks);
    if(st.locks.speed!==undefined||st.locks.pitch!==undefined)st.locks.pitch=(st.locks.pitch??tr.tape.pitch)+12*Math.log2((st.locks.speed??tr.tape.speed)/100);
    if(st.locks.slice!==undefined){st.locks.start=Math.round(a.durationMs*st.locks.slice/tr.tape.slices);st.locks.end=Math.round(a.durationMs*(st.locks.slice+1)/tr.tape.slices);}
    delete st.locks.speed;delete st.locks.slice;
   }
  }
  return p;
 }
 function demo(library){const p=M.demo();p.tracks[2].engine='SAMPLER';p.tracks[2].kit={keys:{}};p.samples=[];
  const a=asset('DRONE_01',2000),b=asset('DUST_03',1500,{seed:73});library.push(a,b);
  p.tracks[2].kit.keys={48:assignment(a),50:assignment(a,{pitch:-12,start:400,end:1600,level:65}),55:assignment(b,{pitch:7,start:100,end:700,level:50})};
  [8,32,52].forEach((step,i)=>p.tracks[2].patterns[0][step].note=[48,50,55][i]);p.scenes[0].data=M.snapshot(p);return p;
 }
 function availableKeys(tr,library){return Object.keys(tr.kit.keys).map(Number).filter(k=>library.some(a=>a.id===tr.kit.keys[k].assetId)).sort((a,b)=>a-b);}
 function nextKey(keys,current,delta){if(!keys.length)return null;let i=keys.indexOf(current);if(i<0){return delta>0?(keys.find(k=>k>current)??keys.at(-1)):([...keys].reverse().find(k=>k<current)??keys[0]);}return keys[M.clamp(i+delta,0,keys.length-1)];}
 function usedIds(p,all=false){const ids=new Set();for(const tr of all?tracksOf(p):p.tracks)if(all||tr.engine==='SAMPLER'){
  if(all)Object.values(tr.kit.keys).forEach(a=>ids.add(a.assetId));
  else for(const st of tr.patterns[p.pattern].slice(0,M.length(p,tr)))if(st.on&&tr.kit.keys[st.note])ids.add(tr.kit.keys[st.note].assetId);
 }return ids;}
 function memory(p,library){const ids=usedIds(p),assets=library.filter(a=>ids.has(a.id));return {bytes:assets.reduce((sum,a)=>sum+a.bytes,0),count:assets.length,missing:[...ids].filter(id=>!assets.some(a=>a.id===id)),oversized:assets.filter(a=>a.bytes>ASSET_LIMIT)};}
 function draft(a){return {source:M.clone(a),slices:[{start:0,end:a.durationMs,fadeIn:0,fadeOut:0}]};}
 function split(d,i,count){const c=d.slices[i],duration=c.end-c.start;if(count<2||count>256||duration<count||d.slices.length-1+count>256)throw Error('Недостаточно места для такого числа слайсов');d.slices.splice(i,1,...Array.from({length:count},(_,n)=>({start:c.start+Math.round(duration*n/count),end:c.start+Math.round(duration*(n+1)/count),fadeIn:0,fadeOut:0})));}
 function sliceAssets(d,indices){return indices.map(i=>{const c=d.slices[i],first=Math.round(c.start*d.source.rate/1000),last=Math.round(c.end*d.source.rate/1000),frames=last-first;if(frames<1)throw Error('Слишком короткий слайс');const a=asset(d.source.name.slice(0,60)+'_'+String(i+1).padStart(2,'0'),c.end-c.start,{rate:d.source.rate,channels:d.source.channels,frames,origin:'SLICE / '+d.source.name,seed:d.source.seed+i+1,parentId:d.source.id,sourceStart:c.start,sourceEnd:c.end,fadeIn:c.fadeIn,fadeOut:c.fadeOut});if(a.bytes>ASSET_LIMIT)throw Error('Слайс '+(i+1)+' больше 1 MiB — разделите его');return a;});}
 async function readWav(file){
  const read=async(start,len)=>new DataView(await file.slice(start,start+len).arrayBuffer());
  const str=(v,o,n)=>String.fromCharCode(...Array.from({length:n},(_,i)=>v.getUint8(o+i)));
  if(file.size<44)throw Error('Файл слишком короткий для WAV');const header=await read(0,12);
  if(str(header,0,4)!=='RIFF'||str(header,8,4)!=='WAVE')throw Error('Нужен PCM WAV (RIFF/WAVE)');
  let fmt,data,pos=12,count=0;
  while(pos+8<=file.size&&count++<4096){const h=await read(pos,8),type=str(h,0,4),size=h.getUint32(4,true);if(pos+8+size>file.size)throw Error('WAV повреждён или обрезан');
   if(type==='fmt '){if(size<16)throw Error('Неверный fmt WAV');const v=await read(pos+8,16);fmt={encoding:v.getUint16(0,true),channels:v.getUint16(2,true),rate:v.getUint32(4,true),align:v.getUint16(12,true),bits:v.getUint16(14,true)};}
   if(type==='data')data=size;pos+=8+size+(size%2);if(fmt&&data!==undefined)break;
  }
  if(!fmt||data===undefined||!data)throw Error('Нет fmt или аудиоданных WAV');if(fmt.encoding!==1||fmt.bits!==16||![1,2].includes(fmt.channels)||fmt.align!==fmt.channels*2||data%fmt.align)throw Error('Прототип принимает PCM16 WAV mono/stereo');
  const a=asset(file.name,data/fmt.align/fmt.rate*1000,{rate:fmt.rate,channels:fmt.channels,frames:data/fmt.align,origin:'IMPORT / '+file.name.slice(0,100),seed:file.size%1000});validateAsset(a);return a;
 }
 Object.assign(M,{samples:{RAM_BUDGET,ASSET_LIMIT,asset,validateAsset,assignment,migrate,demo,availableKeys,nextKey,usedIds,memory,draft,split,sliceAssets,readWav}});
})(typeof module!=='undefined'?module.exports:Moss);
