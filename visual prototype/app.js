/* MOSS interaction prototype v0.6. No audio, DSP or microphone access. */
'use strict';
const M=Moss,$=s=>document.querySelector(s),esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const S=M.samples, LEGACY_KEY=new URLSearchParams(location.search).has('test')?'moss.test.v1':'moss.v1', STORAGE_KEY=LEGACY_KEY.replace('v1','v2');
const worlds=['GRID','MIX','TAPE'],tabs=['PATTERN','VOICE','SPACE'];
const icons={ATLAS:'<path d="M3 3h6v6H3zm12 0h6v6h-6zM3 15h6v6H3zm12 0h6v6h-6z"/>',GRID:'<path d="M3 3h18v18H3zM9 3v18M15 3v18M3 9h18M3 15h18"/>',VOICE:'<path d="M2 8c5-12 8 12 13 0s7 0 7 0M2 16c5-12 8 12 13 0s7 0 7 0"/>',TAPE:'<path d="M3 4v16M21 4v16M5 12h2l2-6 3 13 3-11 2 4h2"/>',SPACE:'<circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="3"/><path d="M4 18L20 6"/>',MIX:'<path d="M3 6h18M3 12h18M3 18h18M7 3v6M17 9v6M11 15v6"/>'};
const icon=n=>`<svg class="icon" viewBox="0 0 24 24" aria-hidden="true">${icons[n]||icons.GRID}</svg>`;
const defs={tension:[0,100,''],drift:[0,100,''],chaos:[0,100,''],density:[0,100,''],cutoff:[20,12000,'HZ'],resonance:[0,100,'%'],attack:[0,5000,'MS'],release:[0,10000,'MS'],pitch:[-48,48,'ST'],start:[0,86400000,'MS'],end:[1,86400000,'MS'],fadeIn:[0,86400000,'MS'],fadeOut:[0,86400000,'MS'],speed:[25,200,'%'],time:[0,7,'SYNC'],feedback:[0,95,'%'],mix:[0,100,'%'],spread:[0,100,'%'],size:[0,100,'%'],decay:[0,100,'%'],damping:[0,100,'%'],level:[0,100,'%'],pan:[-100,100,''],send:[0,100,'%'],bpm:[30,240,'BPM'],note:[0,127,''],velocity:[1,127,''],gate:[1,64,'STEP'],length:[1,64,'STEP'],drive:[0,100,'%'],bits:[2,16,'BIT'],rate:[1,32,'DIV'],probability:[0,100,'%'],bars:[1,8,'BARS'],slice:[0,15,''],slices:[1,16,''],source:[0,8,''],mute:[0,1,''],solo:[0,1,'']};
const times=['1/16','1/8','1/8D','1/4','1/4D','1/2','3/4','1/1'];
let projects=[],library=[],activeId,storageError=false;
try{const raw=localStorage.getItem(STORAGE_KEY),saved=JSON.parse(raw||localStorage.getItem(LEGACY_KEY)||'null');
 if(saved?.version===2&&Array.isArray(saved.projects)&&Array.isArray(saved.library)){library=saved.library.map(S.validateAsset);projects=saved.projects.map(M.validate);activeId=saved.activeId;}
 else if(saved?.version===1&&Array.isArray(saved.projects)){projects=saved.projects.map(v=>S.migrate(v,library));activeId=saved.activeId;}
 else if(saved)throw Error('Unknown save version');
}catch(e){storageError=true;try{const raw=localStorage.getItem(STORAGE_KEY)||localStorage.getItem(LEGACY_KEY);if(raw)localStorage.setItem(STORAGE_KEY+'.recovery.'+Date.now(),raw);}catch{}}
if(!projects.length){projects=[S.demo(library)];activeId=projects[0].id;}
if(!projects.some(p=>p.id===activeId))activeId=projects[0].id;
const ui={projectCursor:0,sceneCursor:0,world:'ATLAS',tapeTab:'SOURCE',sliceIndex:0,key:48,keyPage:48,tab:'PATTERN',mixTab:'LEVELS',inspectorTab:'STEP',track:0,step:0,page:0,playing:false,head:0,mode:'edit',modal:null,focus:null,queued:-1,capture:null,history:[],large:false,lockKey:null,sample:0,edit:null,aHeld:false,arrow:null,arrowStart:0,arrowLast:0,pointerA:false};
const p=()=>projects.find(v=>v.id===activeId),t=()=>p().tracks[ui.track],s=()=>t()?.patterns[p().pattern][ui.step];
const trackLength=()=>M.length(p(),t());
const fxKeys=f=>f.type==='ORBIT'?['time','feedback','mix','spread']:f.type==='POND'?['size','decay','damping','mix']:['drive','bits','rate','mix'];
const inspectorTarget=()=>ui.inspectorTab==='CYCLE'?'cycle':ui.inspectorTab==='FX'?'fxLock':ui.inspectorTab==='LOCKS'?'lock':'step';
const currentLocks=()=>ui.inspectorTab==='FX'?(s().fxLocks?.[t().fx.type]||{}):s().locks;
const activeLockCount=st=>lockKeys().filter(k=>Object.hasOwn(st.locks,k)).length+Object.keys(st.fxLocks?.[t().fx.type]||{}).length;
const effectOwner=()=>ui.world==='TRACK'?t().fx:p().fx;
const btn=(label,action,args='',cls='',id='')=>{let aria='';if(action==='step'){const st=+args;aria=` aria-label="Step ${st+1}, ${sFor(st).on?'on':'off'}"`;}if(action==='trackOpen')aria=` title="${esc(p().tracks[+args].name)}"`;return `<button data-action="${action}" data-args="${esc(args)}" data-nav data-id="${esc(id||action+':'+args)}" class="${cls}"${aria}>${label}</button>`;};
const sFor=i=>t().patterns[p().pattern][i];
const format=(k,v)=>v===undefined?'—':k==='source'?(v==='MASTER'?'MASTER / ALL TRACKS':p().tracks.find(tr=>tr.id===v)?.name||'SOURCE MISSING'):typeof v==='boolean'?(v?'ON':'OFF'):k==='time'?times[v]:k==='note'?M.note(v):String(k==='slice'?v+1:v).padStart(2,'0');
function control(label,target,k,value,cls='param',slot=null){const hotkey=slot===null?'Z':['Z','X','C','V'][slot];return `<button data-nav data-action="inline" data-id="edit:${esc(target)}:${k}" data-target="${esc(target)}" data-param="${k}" ${slot===null?'':`data-slot="${slot}"`} class="${cls}" aria-label="${esc(label)} ${esc(format(k,value))}. Hold ${hotkey} and use arrows."><span class="param-label">${slot===null?'':hotkey+' · '}${esc(label)}</span><span class="param-value">${esc(format(k,value))} <span class="param-unit">${defs[k][2]}</span></span></button>`;}
function params(keys,target,owner){return `<div class="param-grid">${keys.map((k,i)=>control(k.toUpperCase(),target,k,owner[k],'param',i)).join('')}</div>`;}
function toast(message){$('#toast').textContent=message;$('#toast').classList.add('visible');clearTimeout(toast.timer);toast.timer=setTimeout(()=>$('#toast').classList.remove('visible'),2600);}
function save(){if(storageError){$('#save-status').textContent='Сохранение повреждено — экспортируйте текущий проект';return;}try{localStorage.setItem(STORAGE_KEY,JSON.stringify({version:2,activeId,projects,library}));$('#save-status').textContent='Сохранено в этом браузере';}catch(e){$('#save-status').textContent='Не сохранено — экспортируйте JSON';}}
function checkpoint(){ui.history.push({projects:M.clone(projects),library:M.clone(library),activeId});if(ui.history.length>30)ui.history.shift();}
function commit(fn){finishEdit();checkpoint();fn();save();render();}
function undo(){finishEdit();const old=ui.history.pop();if(!old)return toast('Пока нечего отменять');projects=old.projects;library=old.library;activeId=old.activeId;ui.modal=null;ui.capture=null;ui.queued=-1;normalize();save();render();toast('Изменение отменено');}
function normalize(){ui.projectCursor=M.clamp(ui.projectCursor,0,projects.length-1);ui.track=M.clamp(ui.track,0,Math.max(0,p().tracks.length-1));ui.step=M.clamp(ui.step,0,trackLength()-1);ui.page=Math.floor(ui.step/16);ui.sample=M.clamp(ui.sample,0,Math.max(0,library.length-1));if(ui.world==='TRACK'&&!t())ui.world='GRID';ui.sliceIndex=M.clamp(ui.sliceIndex,0,Math.max(0,(p().tapeDraft?.slices.length||1)-1));}
function setWorld(w){if(ui.capture&&w!==ui.world)return captureExit(()=>setWorld(w));finishEdit();if(w==='ATLAS'){ui.playing=false;ui.queued=-1;}ui.world=w;ui.modal=null;ui.lockKey=null;ui.focus=w==='GRID'&&t()?'trackOpen:'+ui.track:w==='ATLAS'?'loadProject:'+activeId:null;if(w==='ATLAS')ui.projectCursor=projects.findIndex(v=>v.id===activeId);document.activeElement?.blur();render();$('.view').scrollTop=0;}
function setTab(tab){finishEdit();ui.tab=tab;ui.lockKey=null;ui.focus=null;document.activeElement?.blur();render();}
function openTrack(i){ui.track=+i;ui.tab='PATTERN';ui.inspectorTab='STEP';setWorld('TRACK');}
function back(){if(ui.edit)return finishEdit(true);if(ui.modal)return closeModal();if(ui.lockKey){ui.lockKey=null;return render();}setWorld(ui.world==='TRACK'?'GRID':'ATLAS');}
function navigation(){if(ui.world==='ATLAS')return '';const local=ui.world==='TRACK',items=local?tabs:worlds,current=local?ui.tab:ui.world;return `<span class="nav-key">[</span>${items.map(n=>btn(n,local?'tab':'world',n,n===current?'current':'').replace('<button ','<button aria-current="'+(n===current?'page':'false')+'" ')).join('')}<span class="nav-key">]</span>`;}
function parameterBank(){
 if(ui.world==='TRACK'&&ui.tab==='PATTERN')return {target:inspectorTarget(),keys:inspectorKeys()};
 if(ui.world==='TRACK'&&ui.tab==='VOICE')return {target:t().engine==='SAMPLER'?'sampler':'voice',keys:t().engine==='SAMPLER'?['pitch','start','end','level']:voiceKeys()};
 if(ui.world==='TRACK'&&ui.tab==='SPACE'||ui.world==='MIX'&&ui.mixTab==='FX')return {target:ui.world==='TRACK'?'trackFx':'globalFx',keys:fxKeys(effectOwner())};
 if(ui.world==='TAPE')return ui.tapeTab==='SOURCE'?{target:'capture',keys:['source','bars']}:ui.tapeTab==='EDIT'&&p().tapeDraft?{target:'sliceEdit',keys:['start','end','fadeIn','fadeOut']}:null;
 if(ui.world==='MIX'&&ui.mixTab==='LEVELS'&&t())return {target:'track:'+ui.track,keys:['level','pan','mute','solo']};
 return null;
}
function footer(){
 if(ui.edit)return `<footer class="screen-footer"><span>EDIT ${ui.edit.key.toUpperCase()}</span><span>ARROWS · RELEASE TO KEEP</span><span class="position">TAB / SELECT CANCEL</span></footer>`;
 const scope=objectScope();return `<footer class="screen-footer"><span class="${scope==='—'?'inactive':''}"><b>Q/E</b>${scope}</span>${btn('Q+E OPTIONS','contextOptions','','footer-options')}<span class="position">${ui.world==='ATLAS'?'Z OPEN':ui.world==='TRACK'?'X GRID':'X PROJECTS'}</span></footer>`;
}
function render(){
 normalize();const focus=document.activeElement?.dataset?.id||ui.focus,scroll=$('.view')?.scrollTop||0;
 $('#screen').style.setProperty('--accent','var(--text)');
 $('#app').innerHTML=`<header class="topline">${ui.world==='ATLAS'?'<span class="project-name">MOSS / ATLAS</span><span class="sub">PROJECTS</span>':`<span class="transport-state ${ui.playing?'playing':''}"><i class="state-dot"></i>${ui.playing?'RUN':'STOP'}</span><span class="project-name">${esc(p().name)}${ui.world==='TRACK'?` / ${String(ui.track+1).padStart(2,'0')} ${esc(t().name)}`:''}</span>${control('TEMPO','project','bpm',p().bpm,'tempo inline-small')}<span class="sub">${esc(p().key)}</span>`}</header>${ui.world==='ATLAS'?'':`<nav class="world-nav" aria-label="${ui.world==='TRACK'?'Дорожка':'Разделы проекта'}">${navigation()}</nav>`}<section class="view ${ui.world==='ATLAS'?'atlas-view':''} ${ui.world==='TRACK'&&ui.tab==='PATTERN'?'pattern-view':''}">${views[ui.world]()}</section>${footer()}`;
 renderModal();if(!ui.modal){const el=[...$('#app').querySelectorAll('[data-nav]')].find(e=>e.dataset.id===focus)||(ui.world==='TRACK'&&ui.tab==='VOICE'&&t().engine==='SAMPLER'?document.querySelector('[data-action="keyMenu"][data-args="'+ui.key+'"]'):null)||$('#app .view [data-nav]');el?.focus({preventScroll:true});$('.view').scrollTop=scroll;}
 markEditing();$('#play-button span').textContent=ui.playing?'PAUSE / SPACE':'START / SPACE';$('#play-button').disabled=ui.world==='ATLAS';$('#export').disabled=ui.world==='ATLAS';$('#undo').disabled=!ui.history.length;const scenesButton=$('[data-scenario="scenes"]');if(scenesButton)scenesButton.disabled=ui.world==='ATLAS';paintClock();paintModels(performance.now());
}
const title=(name,sub,tag='')=>`<div class="view-title"><div><h2>${name}</h2><div class="sub">${sub}</div></div></div>`;
function atlas(){return title('Projects',`${projects.length} SAVED · Q/E SELECT · Z OPEN`)+`<div class="project-library">${projects.map((v,i)=>btn(`<span>${esc(v.name)}<small>${v.tracks.length} TRACKS · ${v.bpm} BPM · MASTER ${v.length} STEPS</small></span>`,'loadProject',v.id,'list-row '+(i===ui.projectCursor?'object-selected':''))).join('')}</div><div class="primary-action">${btn('+ NEW PROJECT','newProject')}</div>`;}
function grid(){return `<div class="overview-labels"><span>TRACK · ${p().tracks.length}/8</span><span>VOICE</span><span>SPACE</span></div>${p().tracks.map((tr,i)=>btn(`<span><i>${String(i+1).padStart(2,'0')}</i> ${esc(tr.name)}</span><span>${tr.engine}</span><span>${tr.fx.enabled?tr.fx.type:'BYPASS'}</span>`,'trackOpen',i,'overview-track '+(i===ui.track?'object-selected':''))).join('')}${!t()?'<div class="empty-state"><p>Добавьте первую дорожку.</p>'+btn('+ TRACK','addTrack')+'</div>':'<p class="quiet-hint">Z OPEN TRACK · Q+E ADD / OPTIONS</p>'}`;}
const voiceKeys=()=>t().engine==='MONO'?['cutoff','resonance','attack','release']:t().engine==='SAMPLER'?['pitch','speed']:['tension','drift','chaos','density'];
const lockKeys=()=>t().engine==='SAMPLER'?['pitch','start','end','level']:voiceKeys();
const inspectorKeys=()=>ui.inspectorTab==='CYCLE'?['length']:ui.inspectorTab==='FX'?fxKeys(t().fx):ui.inspectorTab==='LOCKS'?lockKeys():['note','velocity','gate','probability'];
function lockBase(k){if(ui.inspectorTab==='FX')return t().fx[k];return t().engine==='SAMPLER'?keyAssignment(s().note)?.[k]:t().params[k];}

function selectInspector(mode){finishEdit();ui.tab='PATTERN';ui.inspectorTab=mode;ui.focus='step-'+ui.step;document.activeElement?.blur();render();}
function inspectorTabButton(mode,label){return btn(label,'inspectorTab',mode,ui.inspectorTab===mode?'current':'').replace('<button ','<button role="tab" aria-selected="'+(ui.inspectorTab===mode)+'" ');}
function stepInspector(){
 const st=s(),locked=['LOCKS','FX'].includes(ui.inspectorTab),fx=ui.inspectorTab==='FX',cycle=ui.inspectorTab==='CYCLE',keys=inspectorKeys(),locks=currentLocks(),sampler=t().engine==='SAMPLER',assignment=sampler?keyAssignment(st.note):null,asset=sampler?keyAsset(st.note):null;
 const state=cycle?'THIS TRACK / PATTERN '+String.fromCharCode(65+p().pattern):fx?t().fx.type+' / '+(t().fx.enabled?'ACTIVE':'BYPASS'):sampler?(asset?M.note(st.note)+' · '+asset.name:'UNASSIGNED'):'1 STEP = 1/16 NOTE';
 const tabs=inspectorTabButton('STEP','STEP')+inspectorTabButton('LOCKS','VOICE LOCKS')+inspectorTabButton('FX','FX LOCKS')+inspectorTabButton('CYCLE','CYCLE');
 const fields=cycle?'<div class="cycle-fields">'+control('LENGTH','cycle','length',trackLength(),'param',0)+'<div class="cycle-presets">'+[16,32,64].map(n=>btn(n+' STEPS','cyclePreset',n,trackLength()===n?'current':'')).join('')+'<span>1–64 · HOLD Z + ARROWS<br>Hidden steps are preserved.</span></div></div>':'<div class="step-fields '+(locked?'lock-fields':'')+'" role="tabpanel" aria-label="Step parameters">'+keys.map((k,i)=>{const disabled=sampler&&((ui.inspectorTab==='LOCKS'&&!asset)||(k==='gate'&&assignment?.playMode==='ONE SHOT'));const label=k==='gate'?'LENGTH':k==='note'&&sampler?'KEY':k.toUpperCase();if(disabled)return '<div class="param unavailable" data-slot="'+i+'"><span class="param-label">'+['Z','X','C','V'][i]+' · '+label+'</span><span class="param-value">'+(k==='gate'?'ONE SHOT':'—')+'</span></div>';return control(label,inspectorTarget(),k,locked?locks[k]:st[k],locked?'param '+(Object.hasOwn(locks,k)?'lock-assigned':'lock-unset'):'param',i);}).join('')+'</div>';
 return '<div class="inspector-heading"><span>STEP '+String(ui.step+1).padStart(2,'0')+' <b>'+(st.on?'TRIG ON':'TRIG OFF')+'</b></span><span class="key-status">'+esc(state)+'</span></div><div class="inspector-tabs" role="tablist" aria-label="Параметры шага">'+tabs+'<span class="inspector-hint">'+(locked?'— INHERIT':'V NEXT')+'</span></div>'+fields;
}
function pattern(){return '<div class="track-heading"><span>PATTERN '+String.fromCharCode(65+p().pattern)+' · '+trackLength()+' STEPS · TRACK '+(ui.track+1)+'</span><span class="right">'+String(ui.page*16+1).padStart(2,'0')+'–'+Math.min((ui.page+1)*16,trackLength())+'</span></div><div class="pattern-steps">'+Array.from({length:16},(_,i)=>{const n=ui.page*16+i,st=sForPage()[i];if(!st)return '<span class="step inactive-step" aria-hidden="true">'+String(n+1).padStart(2,'0')+'</span>';const sustained=s().on&&!(t().engine==='SAMPLER'&&keyAssignment(s().note)?.playMode==='ONE SHOT')&&((n-ui.step+trackLength())%trackLength())<s().gate;return btn('<span>'+String(n+1).padStart(2,'0')+'</span><b>'+(st.on?M.note(st.note):'—')+'</b>'+(activeLockCount(st)?'<i></i>':''),'step',n,'step '+(st.on?'on ':'')+(n===ui.step?'selected-step ':'')+(sustained?'sustain ':'')+(activeLockCount(st)?'lock':''),'step-'+n);}).join('')+'</div><div class="page-controls"><span>PAGE '+(ui.page+1)+'/'+Math.ceil(trackLength()/16)+'</span><span>LINE = NOTE LENGTH</span><span>C FIELDS · V BANK</span></div><div id="step-inspector">'+stepInspector()+'</div>';}

const sForPage=()=>t().patterns[p().pattern].slice(ui.page*16,Math.min(trackLength(),ui.page*16+16));
function trackView(){return ui.tab==='PATTERN'?pattern():ui.tab==='VOICE'?voice():space();}
function voice(){if(t().engine==='SAMPLER')return sampler();return title(t().engine,'PARAMETER MODEL')+`<div class="diagram technical">${t().engine==='ORGANISM'?organism():mono()}</div>${params(voiceKeys(),'voice',t().params)}`;}
function sampler(){const mem=S.memory(p(),library),start=ui.keyPage;return title('SAMPLER',`${M.note(start)}–${M.note(start+15)} · ${mem.count} PCM / ${bytes(mem.bytes)} · LIMIT 8 MiB`)+`<div class="kit-keys">${Array.from({length:16},(_,i)=>{const n=start+i,a=keyAssignment(n),sm=keyAsset(n);return btn(`<b>${M.note(n)}</b><small>${a?(sm?esc(sm.name):'MISSING FILE'):'—'}</small>`,'keyMenu',n,`kit-key ${n===ui.key?'selected-key':''} ${a?'assigned':''} ${a&&!sm?'missing':''}`);}).join('')}</div><div id="key-panel">${keyPanel()}</div>`;}
function space(){const f=effectOwner();return title(f.type,`${ui.world==='TRACK'?'TRACK '+(ui.track+1):'MASTER / ALL TRACKS'} · ${f.enabled?'ACTIVE':'BYPASS'} · PARAMETER MODEL`)+`<div class="diagram technical ${f.enabled?'':'bypassed'}">${f.type==='ORBIT'?orbitDiagram():f.type==='POND'?pondDiagram():crushDiagram()}</div>${params(fxKeys(f),ui.world==='TRACK'?'trackFx':'globalFx',f)}`;}
function mix(){const nav=`<div class="sub-tabs"><span class="sub-key">V</span>${[['LEVELS','LEVELS'],['FX','MASTER FX'],['SCENES','SCENES']].map(([id,label])=>btn(label,'mixTab',id,ui.mixTab===id?'current':'')).join('')}</div>`;if(ui.mixTab==='FX')return nav+space();if(ui.mixTab==='SCENES')return nav+scenes();return nav+`<div class="mix-labels"><span>TRACK</span><span>Z · LEVEL</span><span>X · PAN</span><span>C · MUTE</span><span>V · SOLO</span></div>${p().tracks.map((tr,i)=>`<div class="mix-row mix-v2 ${i===ui.track?'selected-mix':''}" data-mix-track="${i}">${btn(esc(tr.name),'trackOpen',i)}${control('LEVEL','track:'+i,'level',tr.level,'inline-small',0)}${control('PAN','track:'+i,'pan',tr.pan,'inline-small',1)}${control('MUTE','track:'+i,'mute',tr.mute,'inline-small',2)}${control('SOLO','track:'+i,'solo',tr.solo,'inline-small',3)}</div>`).join('')}${!t()?'<p class="empty">Добавьте дорожки в GRID.</p>':''}`;}
function scenes(){return title('SCENES','QUEUE / '+p().length+' STEP MASTER · Z APPLY · Q+E SAVE')+p().scenes.map((sc,i)=>`<div class="scene-row">${btn(`<span>${String.fromCharCode(65+i)} / ${sc?esc(sc.name):'EMPTY'}</span><small>${ui.queued===i?'QUEUED':i===p().activeScene?'CURRENT':sc?'LOAD':'SAVE'}</small>`,'sceneLoad',i,(i===p().activeScene?'scene-active ':'')+(i===ui.sceneCursor?'object-selected':''))}</div>`).join('');}
function sourceLabel(){return p().capture.source==='MASTER'?'MASTER / ALL TRACKS':p().tracks.find(tr=>tr.id===p().capture.source)?.name||'SOURCE MISSING';}
function tape(){
 const nav=`<div class="sub-tabs"><span class="sub-key">V</span>${['SOURCE','EDIT','LIBRARY'].map(n=>btn(n,'tapeTab',n,ui.tapeTab===n?'current':'')).join('')}</div>`,cap=ui.capture,d=p().tapeDraft;
 if(ui.tapeTab==='SOURCE')return nav+title('TAPE','Источник → нарезка → библиотека','UI SIMULATION')+`<div class="capture-controls">${control('SOURCE','capture','source',p().capture.source,'inline-small',0)}${control('LENGTH','capture','bars',p().capture.bars,'inline-small',1)}${btn(cap?'FINISH':'CAPTURE',cap?'finishCapture':'startCapture','','capture-button')}${cap?btn('DISCARD','cancelCapture'):''}</div><div class="capture-progress"><i id="capture-progress"></i></div><div class="tape-info"><span>${cap?'CAPTURING · '+esc(cap.source):'Дорожка + SPACE или мастер со всеми эффектами'}</span><span id="capture-time">${cap?'00:00':''}</span></div><div class="source-choices">${btn('IMPORT WAV','importWav')}${btn('FROM LIBRARY','fromLibrary')}${d?btn('CONTINUE EDIT','tapeTab','EDIT'):''}</div><p class="small">WAV PCM16, mono / stereo. Прототип читает параметры файла; звук и нарезка PCM не выполняются. Длинный источник можно разбить перед сохранением.</p>`;
 if(ui.tapeTab==='EDIT'){
  if(!d)return nav+title('EDIT','Нет источника для нарезки')+btn('CHOOSE SOURCE','tapeTab','SOURCE');
  const c=d.slices[ui.sliceIndex],estimated=Math.round((c.end-c.start)*d.source.rate/1000)*d.source.channels*2;
  return nav+title(esc(d.source.name),`SLICE ${ui.sliceIndex+1}/${d.slices.length} · ${((c.end-c.start)/1000).toFixed(3)} SEC · ${bytes(estimated)}`,'EDIT')+
   `<div class="slice-wave"><svg viewBox="0 0 600 125" preserveAspectRatio="none"><g stroke="currentColor">${waveform(d.source.seed)}</g><rect x="${c.start/d.source.durationMs*600}" y="0" width="${(c.end-c.start)/d.source.durationMs*600}" height="125" fill="currentColor" opacity=".14"/></svg><span>SCHEMATIC · ${d.source.durationMs/1000} SEC</span></div>`+
   params(['start','end','fadeIn','fadeOut'],'sliceEdit',c)+`<div class="slice-strip">${d.slices.map((v,i)=>btn(String(i+1).padStart(2,'0'),'selectSlice',i,i===ui.sliceIndex?'current':'')).join('')}</div><div class="primary-action">${btn('SAVE SLICE','saveSlice')}<span>Q+E · CUT / SAVE ALL</span></div><p class="small ${estimated>S.ASSET_LIMIT?'budget-warning':''}">${estimated>S.ASSET_LIMIT?'Слайс больше 1 MiB. Разделите или подрежьте его.':'Лимит слайса 1 MiB · сохраняются отдельные записи библиотеки.'}</p>`;
 }
 const selected=library[ui.sample],mem=S.memory(p(),library);
 return nav+title('LIBRARY',`${library.length} FILES · SD metadata · RAM ${bytes(mem.bytes)} / 8 MiB`,'SHARED')+`<div class="sample-library">${library.map((sm,i)=>btn(`<span>${esc(sm.name)}<small>${(sm.durationMs/1000).toFixed(3)} SEC · ${bytes(sm.bytes)} · ${sm.channels===1?'MONO':'STEREO'}</small></span><span class="meta">${i===ui.sample?'SELECTED':'OPEN'}</span>`,'sampleSelect',i,'list-row '+(i===ui.sample?'sample-selected':''))).join('')||'<p class="empty">Пока пусто. Импортируйте или захватите источник, нарежьте и сохраните.</p>'}</div>${selected?'<p class="quiet-hint">Q+E · EDIT COPY / RENAME / DELETE</p>':''}<p class="small">Назначение на клавиши — в GRID → дорожка → VOICE → SAMPLER.</p>`;
}


const views={ATLAS:atlas,GRID:grid,TRACK:trackView,MIX:mix,TAPE:tape};

// Editing is an inline gesture: one undo entry per A hold, regardless of repeat count.
function ownerFor(target){if(target==='cycle')return {get length(){return trackLength()},set length(n){M.setLength(p(),t(),n)}};if(target==='fxLock')return (s().fxLocks??={})[t().fx.type]??={};if(target==='project')return p();if(target==='voice')return t().params;if(target==='sampler')return keyAssignment();if(target==='sliceEdit')return p().tapeDraft?.slices[ui.sliceIndex];if(target==='step')return s();if(target==='lock')return s().locks;if(target==='trackFx')return t().fx;if(target==='globalFx')return p().fx;if(target==='capture')return p().capture;if(target.startsWith('track:'))return p().tracks[+target.split(':')[1]];}
function beginEdit(el){if(!el?.dataset.param)return false;if(ui.capture&&el.dataset.target==='capture'){toast('Дождитесь завершения захвата');return true;}ui.edit={target:el.dataset.target,key:el.dataset.param,id:el.dataset.id,original:ownerFor(el.dataset.target)[el.dataset.param],changed:false};markEditing();return true;}
function markEditing(){document.querySelectorAll('[data-param]').forEach(el=>el.classList.toggle('editing',el.dataset.id===ui.edit?.id||!!(ui.stepGesture&&el.dataset.target===ui.edit?.target&&el.dataset.param===ui.edit?.key)));document.querySelectorAll('.pattern-steps .step').forEach(el=>el.classList.toggle('editing-step',!!ui.stepGesture&&el.dataset.id===ui.edit?.id));$('#screen').classList.toggle('editing-parameter',!!ui.edit);const bar=$('.screen-footer');if(bar)bar.outerHTML=footer();}
function adjustInline(direction,heldMs=0){
 if(!ui.edit)return;if(ui.stepGesture)ui.stepGesture.used=true;
 const {target,key}=ui.edit,owner=ownerFor(target);if(!owner)return;if(ui.capture&&target==='capture')return toast('Источник и длина зафиксированы до завершения захвата');
 const sampler=t()?.engine==='SAMPLER';
 if(target==='lock'&&sampler&&!keyAsset(s().note))return toast('Сначала назначьте сэмпл этой клавише в VOICE');
 if(target==='step'&&sampler&&key==='gate'&&keyAssignment(s().note)?.playMode==='ONE SHOT')return toast('ONE SHOT играет целиком — GATE не используется');
 const d=defs[key],sign=direction==='left'||direction==='down'?-1:1,assigned=!['lock','fxLock'].includes(target)||Object.hasOwn(owner,key),current=assigned?owner[key]:lockBase(key);
 let amount=heldMs>=1600?10:heldMs>=600?5:1;if(direction==='up'||direction==='down')amount*=key==='note'?12:10;
 let min=d[0],max=d[1];const base=target==='sampler'?keyAssignment():target==='lock'&&sampler?keyAssignment(s().note):null;
 const asset=target==='sliceEdit'?p().tapeDraft.source:base?library.find(a=>a.id===base.assetId):null;
 if(asset&&['start','end'].includes(key)){
  const other=k=>owner[k]??base?.[k];
  if(key==='start')max=Math.max(0,Math.min(asset.durationMs,other('end'))-1);
  else {min=Math.min(asset.durationMs,other('start')+1);max=asset.durationMs;}
 }
 if(target==='sliceEdit'&&key.startsWith('fade'))max=owner.end-owner.start-owner[key==='fadeIn'?'fadeOut':'fadeIn'];
 const choices=key==='source'?['MASTER',...p().tracks.map(tr=>tr.id)]:key==='slices'?[1,2,4,8,16]:null;
 let next=choices?choices[M.clamp(choices.indexOf(current)+sign,0,choices.length-1)]:typeof current==='boolean'?sign>0:M.clamp(current+sign*amount,min,max);
 if(target==='step'&&sampler&&key==='note'){next=S.nextKey(S.availableKeys(t(),library),current,sign);if(next===null)return toast('Нет назначенных клавиш — откройте VOICE → ASSIGN');}
 if(target==='step'&&sampler&&key==='note'&&s().on){const copy=M.clone(p());copy.tracks[ui.track].patterns[copy.pattern][ui.step].note=next;const mem=S.memory(copy,library);if(mem.bytes>S.RAM_BUDGET||mem.oversized.length)return toast('Лимит PCM превышен — подрежьте сэмпл в TAPE');}
 if(next===current&&assigned)return;if(target==='cycle'){const copy=M.clone(p());M.setLength(copy,copy.tracks[ui.track],next);const mem=S.memory(copy,library);if(mem.bytes>S.RAM_BUDGET||mem.oversized.length)return toast('Лимит PCM — подрежьте сэмплы в TAPE');}if(!ui.edit.changed){checkpoint();ui.edit.changed=true;}owner[key]=next;
 if(target==='sliceEdit'){const length=owner.end-owner.start;owner.fadeIn=Math.min(owner.fadeIn,length);owner.fadeOut=Math.min(owner.fadeOut,length-owner.fadeIn);}
 if(['lock','fxLock'].includes(target))s().on=true;save();render();
}


function finishEdit(cancel=false){const edit=ui.edit;if(!edit)return;ui.edit=null;ui.stepGesture=null;ui.arrow=null;ui.aHeld=false;ui.pointerA=false;if(cancel&&edit.changed){const old=ui.history.pop();projects=old.projects;library=old.library;activeId=old.activeId;save();}markEditing();const bar=$('.screen-footer');if(bar)bar.outerHTML=footer();if(cancel)render();}
function updateInspector(){if(ui.world!=='TRACK'||ui.tab!=='PATTERN'||!$('#step-inspector'))return;$('#step-inspector').innerHTML=stepInspector();document.querySelectorAll('.pattern-steps .step').forEach(el=>{const n=+el.dataset.args;el.classList.toggle('selected-step',n===ui.step);el.classList.toggle('sustain',s().on&&!(t().engine==='SAMPLER'&&keyAssignment(s().note)?.playMode==='ONE SHOT')&&((n-ui.step+trackLength())%trackLength())<s().gate);});}
function openModal(m){finishEdit();m.returnFocus=ui.modal?.returnFocus||document.activeElement?.dataset?.id;ui.modal=m;renderModal();}
function closeModal(){const id=ui.modal?.returnFocus;ui.modal=null;ui.focus=id;document.activeElement?.blur();render();if(id)[...$('#app').querySelectorAll('[data-nav]')].find(e=>e.dataset.id===id)?.focus();}
function menu(title,items,description=''){openModal({type:'menu',title,items,description});}
function textEdit(title,value,onSave){openModal({type:'text',title,value,onSave});}
function confirmAction(title,description,fn){menu(title,[['ПОДТВЕРДИТЬ',()=>{closeModal();fn();}],['ОТМЕНА',closeModal]],description);}
function renderModal(){const m=ui.modal;if(!m){$('#overlay').innerHTML='';return;}const old=document.activeElement?.dataset?.id;let body='';if(m.type==='menu')body=`${m.description?`<p>${esc(m.description)}</p>`:''}${m.items.map(([label],i)=>btn(esc(label),'menuItem',i,'list-row')).join('')}`;else body=`<input id="name-input" aria-label="Название" maxlength="24" value="${esc(m.value)}"><div class="keyboard">${'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -'.split('').map(c=>btn(c===' '?'SPACE':c,'char',c)).join('')}${btn('DEL','backspace')}</div><div class="modal-actions">${btn('CANCEL','cancel')}${btn('SAVE','textSave','','primary')}</div>`;$('#overlay').innerHTML=`<section class="modal" role="dialog" aria-modal="true" aria-label="${esc(m.title)}"><h2>${esc(m.title)}</h2>${body}</section>`;([...$('#overlay').querySelectorAll('[data-nav]')].find(e=>e.dataset.id===old)||$('#overlay [data-nav]'))?.focus({preventScroll:true});}
function addTrack(){if(p().tracks.length>=8)return toast('В прототипе максимум 8 дорожек');menu('ADD TRACK',M.engines.map(e=>[e==='TAPE'?'SAMPLER':e,()=>{closeModal();commit(()=>{p().tracks.push(M.track(e));ui.track=p().tracks.length-1;ui.tab='PATTERN';ui.world='TRACK';});}]));}
function trackMenu(){const project=p(),track=t(),id=track.id;menu('TRACK / '+track.name,[
 ['RENAME',()=>textEdit('TRACK NAME',track.name,v=>commit(()=>track.name=v))],
 ['DUPLICATE',()=>{if(project.tracks.length>=8)return toast('Максимум 8 дорожек');closeModal();commit(()=>{const index=project.tracks.findIndex(v=>v.id===id),copy=M.clone(track);copy.id=M.uid();copy.name=(copy.name+' COPY').slice(0,24);project.tracks.splice(index+1,0,copy);ui.track=index+1;});}],
 ['DELETE TRACK',()=>confirmAction('DELETE TRACK?','OPTIONS → UNDO отменяет удаление.',()=>commit(()=>{const index=project.tracks.findIndex(v=>v.id===id);if(index>=0)project.tracks.splice(index,1);ui.track=Math.max(0,index-1);ui.world='GRID';}))]
]);}
function changeEngine(){menu('VOICE ENGINE',M.engines.map(e=>[(e==='TAPE'?'SAMPLER':e)+(e===t().engine?' / CURRENT':''),()=>{closeModal();commit(()=>t().engine=e);}]),'Шаги, SPACE и параметры сохраняются.');}
function projectMenu(){menu(p().name,[['RENAME',()=>textEdit('PROJECT NAME',p().name,v=>commit(()=>p().name=v))],['KEY / SCALE',()=>menu('PROJECT KEY',['G MINOR','C MAJOR','D MINOR','A MINOR','CHROMATIC'].map(k=>[k,()=>{closeModal();commit(()=>p().key=k);}]))],['DUPLICATE PROJECT',()=>{closeModal();commit(()=>{const copy=M.clone(p());copy.id=M.uid();copy.name=(copy.name+' COPY').slice(0,24);projects.push(copy);activeId=copy.id;ui.capture=null;ui.queued=-1;});}],['EXPORT JSON',exportProject],['DELETE PROJECT',()=>confirmAction('DELETE PROJECT?','Ctrl+Z отменяет удаление.',()=>commit(()=>{projects=projects.filter(v=>v.id!==activeId);if(!projects.length)projects.push(M.project());activeId=projects[0].id;ui.world='ATLAS';ui.playing=false;ui.capture=null;ui.queued=-1;}))]]);}
function newProject(){if(ui.capture)return captureExit(newProject);textEdit('NEW PROJECT','UNTITLED',name=>commit(()=>{const np=M.project(name);projects.push(np);activeId=np.id;resetSession();ui.world='GRID';}));}
function resetSession(){ui.track=0;ui.step=0;ui.head=0;ui.playing=false;ui.queued=-1;ui.capture=null;ui.lockKey=null;ui.focus=null;ui.sample=0;ui.key=48;ui.keyPage=48;ui.tapeTab='SOURCE';ui.sliceIndex=0;ui.tab='PATTERN';ui.inspectorTab='STEP';}
function focusId(id){[...$('#app').querySelectorAll('[data-id]')].find(el=>el.dataset.id===id)?.focus({preventScroll:true});}
function exportProject(){const blob=new Blob([JSON.stringify({format:'moss-project',version:2,project:p(),assets:library.filter(a=>S.usedIds(p(),true).has(a.id))},null,2)],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=p().name.replace(/[^a-z0-9а-я_-]/gi,'_')+'.moss.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);toast('Проект экспортирован в JSON');}
function sceneSave(i){const fn=()=>textEdit('SCENE NAME',p().scenes[i]?.name||['ARRIVAL','BLOOM','DRIFT','RETURN'][i],name=>commit(()=>{p().scenes[i]={name,data:M.snapshot(p())};p().activeScene=i;}));if(p().scenes[i])confirmAction('OVERWRITE SCENE?','Снимок в этом слоте будет заменён.',fn);else fn();}
function finishCapture(){
 if(!ui.capture)return;const cap=ui.capture;ui.capture=null;
 const project=projects.find(v=>v.id===cap.projectId);if(!project)return;
 commit(()=>{project.tapeDraft=S.draft(S.asset('CAPTURE '+new Date().toLocaleTimeString('en-GB'),Math.max(100,Math.min(cap.duration,(performance.now()-cap.started)/1000)*1000),{origin:'RESAMPLE / '+cap.source,seed:Date.now()%1000}));ui.tapeTab='EDIT';ui.sliceIndex=0;});toast('Источник готов к нарезке · визуальная симуляция');
}


const actions={world:setWorld,tab:setTab,trackOpen:openTrack,projectMenu,newProject,trackMenu,addTrack,engine:changeEngine,inspectorTab:selectInspector,
 inline:()=>toast('Удерживайте указанную у параметра кнопку и нажимайте стрелки. Esc отменяет изменение.'),
 loadProject:id=>{finishEdit();activeId=id;resetSession();save();setWorld('GRID');},
 newDemo:()=>commit(()=>{const np=S.demo(library);projects.push(np);activeId=np.id;resetSession();ui.world='GRID';}),
 length:()=>selectInspector('CYCLE'),cyclePreset:n=>setCycleLength(+n),
 pattern:()=>menu('PATTERNS',[...[0,1,2,3].map(i=>['PATTERN '+String.fromCharCode(65+i)+(i===p().pattern?' / CURRENT':''),()=>{closeModal();commit(()=>{p().pattern=i;ui.head=0;});}]),['COPY CURRENT TO…',()=>menu('COPY TO',[0,1,2,3].filter(i=>i!==p().pattern).map(i=>['PATTERN '+String.fromCharCode(65+i),()=>confirmAction('REPLACE PATTERN?','Шаги всех дорожек выбранного паттерна будут заменены.',()=>commit(()=>p().tracks.forEach(tr=>{tr.patterns[i]=M.clone(tr.patterns[p().pattern]);tr.lengths[i]=M.length(p(),tr);})))]))]]),
 step:i=>{ui.step=+i;commit(()=>s().on=!s().on);},page:i=>{ui.step=+i*16+ui.step%16;document.activeElement?.blur();ui.focus='step-'+ui.step;render();},
 clearLocks:()=>commit(()=>{if(ui.inspectorTab==='FX')delete s().fxLocks[t().fx.type];else s().locks={};}),
 preset:()=>menu('PRESET',[[t().engine==='ORGANISM'?'EMPTY DRONE':'SOFT SIGNAL',()=>{closeModal();commit(()=>t().params=M.track().params);}],['RESET PARAMETERS',()=>{closeModal();commit(()=>t().params={tension:0,drift:0,chaos:0,density:50,cutoff:2000,resonance:0,attack:0,release:250});}]]),
 effect:()=>menu('EFFECT',['ORBIT','POND','CRUSH'].map(type=>[type+(effectOwner().type===type?' / CURRENT':''),()=>{closeModal();commit(()=>effectOwner().type=type);}])),effectToggle:()=>commit(()=>effectOwner().enabled=!effectOwner().enabled),
 mixTab:tab=>{finishEdit();ui.mixTab=tab;ui.focus=null;document.activeElement?.blur();render();},mute:i=>commit(()=>p().tracks[+i].mute=!p().tracks[+i].mute),solo:i=>commit(()=>p().tracks[+i].solo=!p().tracks[+i].solo),
 sceneSave:i=>sceneSave(+i),sceneName:i=>textEdit('SCENE NAME',p().scenes[+i].name,v=>commit(()=>p().scenes[+i].name=v)),
 sceneLoad:i=>{i=+i;if(!p().scenes[i])return sceneSave(i);if(ui.playing){ui.queued=ui.queued===i?-1:i;render();toast(ui.queued<0?'Переключение отменено':'Сцена переключится в начале цикла');}else commit(()=>{M.restoreScene(p(),i);ui.head=0;});},
 menuItem:i=>ui.modal.items[+i][1](),cancel:closeModal,char:c=>{ui.modal.value=(ui.modal.value+c).slice(0,24);renderModal();},backspace:()=>{ui.modal.value=ui.modal.value.slice(0,-1);renderModal();},textSave:()=>{const m=ui.modal,name=m.value.trim();if(!name)return toast('Введите название');closeModal();m.onSave(name);},
 captureSource:()=>{if(ui.capture)return toast('Сначала завершите захват');menu('RESAMPLE SOURCE',[['MASTER / ALL TRACKS',()=>{closeModal();commit(()=>p().capture.source='MASTER');}],...p().tracks.map(tr=>[tr.name+' / VOICE + SPACE',()=>{closeModal();commit(()=>p().capture.source=tr.id);}])]);},
 startCapture:()=>{if(!p().tracks.length)return toast('Добавьте дорожку в GRID');if(p().capture.source!=='MASTER'&&!p().tracks.some(tr=>tr.id===p().capture.source))return toast('Выберите существующий источник');ui.capture={projectId:p().id,started:performance.now(),duration:p().capture.bars*4*60/p().bpm,source:sourceLabel()};render();},finishCapture,cancelCapture:()=>{ui.capture=null;render();},
 sampleSelect:i=>{ui.sample=+i;render();},sampleRename:()=>textEdit('SAMPLE NAME',p().samples[ui.sample].name,v=>commit(()=>p().samples[ui.sample].name=v)),sampleDelete:()=>confirmAction('DELETE FROM MEMORY?','Назначенные дорожкам копии сэмпла останутся.',()=>commit(()=>p().samples.splice(ui.sample,1))),
};
// TAPE owns sources and cuts; SAMPLER owns independent key assignments.
const bytes=n=>n>=1048576?(n/1048576).toFixed(2)+' MiB':(n/1024).toFixed(1)+' KiB';
function keyAssignment(n=ui.key){return t()?.kit?.keys[n];}
function keyAsset(n=ui.key){const a=keyAssignment(n);return a&&library.find(sm=>sm.id===a.assetId);}
function keyPanel(){
 const a=keyAssignment(),sm=keyAsset();
 if(!a||!sm)return `<div class="key-empty"><b>${M.note(ui.key)} · ${a?'MISSING FILE':'EMPTY KEY'}</b><span>${a?'Исходный файл отсутствует. Замените назначение.':'Z / ASSIGN — выбрать сэмпл из библиотеки'}</span></div>`;
 return `<div class="key-summary"><span>${M.note(ui.key)} · ${esc(sm.name)}</span><span>${a.playMode}</span></div><div class="key-wave" aria-label="Ручные границы сэмпла"><svg viewBox="0 0 600 125" preserveAspectRatio="none"><g stroke="currentColor" opacity=".4">${waveform(sm.seed)}</g><rect x="${a.start/sm.durationMs*600}" y="0" width="${(a.end-a.start)/sm.durationMs*600}" height="125" fill="currentColor" opacity=".17"/><path d="M${a.start/sm.durationMs*600} 0v125 M${a.end/sm.durationMs*600} 0v125" stroke="currentColor" stroke-width="2"/></svg><span>START ${a.start} ms</span><span>END ${a.end} ms · SCHEMATIC</span></div>${params(['pitch','start','end','level'],'sampler',a)}`;
}
function updateKeyPanel(){if($('#key-panel'))$('#key-panel').innerHTML=keyPanel();document.querySelectorAll('.kit-key').forEach(el=>el.classList.toggle('selected-key',+el.dataset.args===ui.key));markEditing();}
function captureExit(next){menu('CAPTURE IN PROGRESS',[
 ['STAY · продолжить захват',closeModal],
 ['FINISH · открыть текущий результат',()=>{closeModal();finishCapture();next();}],
 ['DISCARD · удалить захват',()=>{ui.capture=null;closeModal();next();}]
],'Выберите, что сделать с текущим захватом.');}
function replaceDraft(asset){const apply=()=>commit(()=>{p().tapeDraft=S.draft(asset);ui.tapeTab='EDIT';ui.sliceIndex=0;});if(p().tapeDraft)confirmAction('REPLACE EDIT SOURCE?','Текущая нарезка будет заменена. Ctrl+Z возвращает её.',apply);else apply();}
function tapeContext(delta){finishEdit();if(ui.tapeTab==='SOURCE')return toast('В SOURCE: Z + стрелки — источник, X + стрелки — длина');if(ui.tapeTab==='EDIT'&&p().tapeDraft){ui.sliceIndex=M.clamp(ui.sliceIndex+delta,0,p().tapeDraft.slices.length-1);ui.focus='selectSlice:'+ui.sliceIndex;}else if(ui.tapeTab==='LIBRARY'){ui.sample=M.clamp(ui.sample+delta,0,Math.max(0,library.length-1));ui.focus='sampleSelect:'+ui.sample;}document.activeElement?.blur();render();document.activeElement?.scrollIntoView({block:'nearest',inline:'nearest'});}
function assignKey(){
 if(!library.length)return menu('LIBRARY EMPTY',[['OPEN TAPE',()=>{closeModal();ui.tapeTab='SOURCE';setWorld('TAPE');}],['BACK',closeModal]],'Импортируйте или ресэмплируйте источник, подрежьте и сохраните сэмпл.');
 const note=ui.key,track=t();
 menu('ASSIGN TO '+M.note(note),library.map(sm=>[sm.name+' · '+bytes(sm.bytes),()=>{
  if(sm.bytes>S.ASSET_LIMIT)return toast('Сэмпл больше 1 MiB. Подрежьте или нарежьте его в TAPE.');
  const copy=M.clone(p()),candidate=copy.tracks.find(v=>v.id===track.id);candidate.kit.keys[note]=S.assignment(sm);const mem=S.memory(copy,library);
  if(mem.bytes>S.RAM_BUDGET)return toast('Лимит 8 MiB. Сохраните более короткий сэмпл в TAPE.');
  closeModal();commit(()=>track.kit.keys[note]=S.assignment(sm));toast(M.note(note)+' ← '+sm.name);
 }]));
}
function saveCuts(all){
 const d=p().tapeDraft;if(!d)return;
 try{const assets=S.sliceAssets(d,all?d.slices.map((_,i)=>i):[ui.sliceIndex]);commit(()=>{library.push(...assets);ui.sample=library.length-assets.length;ui.tapeTab='LIBRARY';});toast(assets.length+' сэмпл(ов) сохранено · метаданные');}catch(err){toast(err.message);}
}
Object.assign(actions,{
 tapeTab:n=>{if(ui.capture&&n!=='SOURCE')return captureExit(()=>actions.tapeTab(n));finishEdit();ui.tapeTab=n;ui.focus=null;document.activeElement?.blur();render();},
 fromLibrary:()=>actions.tapeTab('LIBRARY'),
 importWav:()=>{if(ui.capture)return toast('Сначала завершите захват');$('#wav-file').click();},
 keyPage:delta=>{finishEdit();ui.keyPage=M.clamp(ui.keyPage+(+delta)*16,0,112);ui.key=ui.keyPage;ui.focus='keyMenu:'+ui.key;document.activeElement?.blur();render();},
 keyMenu:note=>{ui.key=+note;const selectedKey=ui.key;updateKeyPanel();if(!keyAssignment())return assignKey();menu(M.note(ui.key)+' / KEY',[
  ['REPLACE SAMPLE',assignKey],['PLAY MODE',()=>actions.keyMode()],
  ['CLEAR ASSIGNMENT',()=>{closeModal();commit(()=>delete t().kit.keys[selectedKey]);toast('Назначение снято. Шаги сохраняют номер клавиши.');}]
 ]);},
 assignKey,
 keyMode:()=>{const assignment=keyAssignment();if(!assignment)return;menu('PLAY MODE',['ONE SHOT','GATE','LOOP'].map(mode=>[mode,()=>{closeModal();commit(()=>assignment.playMode=mode);}]),'ONE SHOT — целиком; GATE — до конца gate; LOOP — повтор в пределах gate.');},
 selectSlice:i=>{finishEdit();ui.sliceIndex=+i;render();},
 splitDraft:()=>{const d=p().tapeDraft;if(!d)return;const c=d.slices[ui.sliceIndex],auto=Math.max(2,Math.ceil((c.end-c.start)/1000*d.source.rate*d.source.channels*2/(S.ASSET_LIMIT-1024)));menu('SPLIT SELECTED SLICE',[2,4,8,16,...(auto>16?[auto]:[])].map(n=>[n+' EQUAL SLICES',()=>{closeModal();try{const copy=M.clone(d);S.split(copy,ui.sliceIndex,n);commit(()=>p().tapeDraft=copy);}catch(e){toast(e.message);}}]),'Нарезка только по команде. START / END каждого фрагмента можно изменить вручную.');},
 addCut:()=>{const d=p().tapeDraft;if(!d)return;try{const copy=M.clone(d);S.split(copy,ui.sliceIndex,2);commit(()=>p().tapeDraft=copy);}catch(e){toast(e.message);}},
 removeCut:()=>{if(p().tapeDraft.slices.length===1)return toast('Оставьте хотя бы один фрагмент');commit(()=>p().tapeDraft.slices.splice(ui.sliceIndex,1));},
 saveSlice:()=>saveCuts(false),saveAllSlices:()=>saveCuts(true),
 sampleSelect:i=>{ui.sample=+i;render();},
 editLibrary:()=>{const sm=library[ui.sample];if(sm)replaceDraft(sm);},
 sampleRename:()=>{const sm=library[ui.sample];if(sm)textEdit('SAMPLE NAME',sm.name,v=>commit(()=>sm.name=v));},
 sampleDelete:()=>{const sm=library[ui.sample];if(!sm)return;const refs=projects.filter(project=>S.usedIds(project,true).has(sm.id));if(refs.length)return menu('SAMPLE IN USE',[['BACK',closeModal]],'Используется в проектах / сценах: '+refs.map(v=>v.name).join(', ')+'. Сначала снимите назначения.');confirmAction('DELETE SAMPLE?',sm.name+' · Ctrl+Z возвращает запись.',()=>commit(()=>library.splice(ui.sample,1)));},
 step:i=>{ui.step=+i;if(!s().on&&t().engine==='SAMPLER'){
  const keys=S.availableKeys(t(),library);if(!keys.length)return toast('Нет назначенных клавиш — VOICE → ASSIGN');
  const note=keys.includes(s().note)?s().note:keys[0],copy=M.clone(p());Object.assign(copy.tracks[ui.track].patterns[copy.pattern][ui.step],{on:true,note});const mem=S.memory(copy,library);
  if(mem.bytes>S.RAM_BUDGET||mem.oversized.length)return toast('Лимит PCM: 8 MiB всего, 1 MiB на сэмпл. Подрежьте источник в TAPE.');
  commit(()=>Object.assign(s(),{note,on:true}));
 }else commit(()=>s().on=!s().on);},
 startCapture:()=>{
  const start=()=>{if(!p().tracks.length)return toast('Добавьте дорожку в GRID');if(p().capture.source!=='MASTER'&&!p().tracks.some(tr=>tr.id===p().capture.source))return toast('Выберите существующий источник');ui.capture={projectId:p().id,started:performance.now(),duration:p().capture.bars*4*60/p().bpm,source:sourceLabel()};render();};
  if(p().tapeDraft)confirmAction('REPLACE EDIT SOURCE?','После захвата текущая нарезка будет заменена. Ctrl+Z вернёт её.',start);else start();
 }
});
$('#wav-file').onchange=async e=>{const file=e.target.files[0],projectId=activeId;if(!file)return;try{const asset=await S.readWav(file);if(projectId!==activeId)return toast('Проект сменился — повторите импорт');replaceDraft(asset);}catch(err){toast('WAV: '+err.message);}finally{e.target.value='';}};

// The same shoulder grammar is shown on-screen and used by keyboard and hardware controls.
function objectScope(){if(ui.world==='ATLAS')return 'PROJECT';if(ui.world==='GRID'||ui.world==='TRACK'||ui.world==='MIX'&&ui.mixTab==='LEVELS')return t()?'TRACK':'—';if(ui.world==='MIX')return ui.mixTab==='SCENES'?'SCENE':'—';if(ui.world==='TAPE')return ui.tapeTab==='EDIT'&&p().tapeDraft?'SLICE':ui.tapeTab==='LIBRARY'&&library.length?'FILE':'—';return '—';}
function moveObject(delta){
 const scope=objectScope();if(scope==='—')return;finishEdit();
 if(scope==='PROJECT'){ui.projectCursor=M.clamp(ui.projectCursor+delta,0,projects.length-1);ui.focus='loadProject:'+projects[ui.projectCursor].id;}
 else if(scope==='TRACK'){ui.track=M.clamp(ui.track+delta,0,p().tracks.length-1);ui.focus=ui.world==='GRID'?'trackOpen:'+ui.track:ui.world==='MIX'?'edit:track:'+ui.track+':level':ui.tab==='PATTERN'?'step-'+ui.step:ui.tab==='VOICE'&&t().engine==='SAMPLER'?'keyMenu:'+ui.key:null;}
 else if(scope==='SCENE'){ui.sceneCursor=M.clamp(ui.sceneCursor+delta,0,3);ui.focus='sceneLoad:'+ui.sceneCursor;}
 else return tapeContext(delta);
 document.activeElement?.blur();render();document.activeElement?.scrollIntoView({block:'nearest'});
}
function contextOptions(){
 if(ui.edit||ui.modal)return;
 const closeRun=fn=>()=>{closeModal();fn();},items=[],add=(label,fn)=>items.push([label,closeRun(fn)]);
 if(ui.world==='ATLAS'){add('NEW PROJECT',newProject);add('DEMO / SLOW SIGNAL',actions.newDemo);add('IMPORT PROJECT JSON',()=>$('#import-file').click());}
 if(ui.world==='GRID'){add('ADD TRACK',addTrack);if(t())add('TRACK / '+t().name,trackMenu);add('PROJECT / '+p().name,projectMenu);}
 if(ui.world==='TRACK'){
  if(ui.tab==='PATTERN'){add('PATTERN '+String.fromCharCode(65+p().pattern)+' / ALL TRACKS',actions.pattern);add('CYCLE LENGTH / THIS TRACK',actions.length);add('NOTE LENGTH / TO CYCLE END',()=>{if(t().engine==='SAMPLER'&&keyAssignment(s().note)?.playMode==='ONE SHOT')return toast('Выберите GATE или LOOP в VOICE');commit(()=>{s().gate=trackLength()-ui.step;s().on=true;});});add('GO TO STEP PAGE',()=>menu('STEP PAGE',Array.from({length:Math.ceil(trackLength()/16)},(_,i)=>[(i*16+1)+'–'+Math.min((i+1)*16,trackLength()),closeRun(()=>actions.page(i))])));if(['LOCKS','FX'].includes(ui.inspectorTab)&&Object.keys(currentLocks()).length){add('CLEAR CURRENT LOCK BANK',actions.clearLocks);add('REMOVE ONE LOCK',()=>menu('REMOVE LOCK',Object.keys(currentLocks()).map(k=>[k.toUpperCase(),closeRun(()=>commit(()=>delete currentLocks()[k]))])));}}
  if(ui.tab==='VOICE'){add('INSTRUMENT / '+t().engine,changeEngine);if(t().engine==='SAMPLER'){add('ASSIGN SAMPLE / '+M.note(ui.key),assignKey);if(keyAssignment()){add('PLAY MODE / '+keyAssignment().playMode,actions.keyMode);add('CLEAR KEY / '+M.note(ui.key),()=>commit(()=>delete t().kit.keys[ui.key]));}add('KEY RANGE',()=>menu('KEY RANGE',Array.from({length:8},(_,i)=>[M.note(i*16)+'–'+M.note(i*16+15),closeRun(()=>{ui.keyPage=i*16;ui.key=ui.keyPage;ui.focus='keyMenu:'+ui.key;document.activeElement.blur();render();})])));}else add('PRESET',actions.preset);}
  if(ui.tab==='SPACE'){add('EFFECT / '+effectOwner().type,actions.effect);add(effectOwner().enabled?'BYPASS EFFECT':'ENABLE EFFECT',actions.effectToggle);}
  add('TRACK / '+t().name,trackMenu);add('PROJECT / '+p().name,projectMenu);
 }
 if(ui.world==='MIX'){
  if(ui.mixTab==='FX'){add('EFFECT / '+p().fx.type,actions.effect);add(p().fx.enabled?'BYPASS EFFECT':'ENABLE EFFECT',actions.effectToggle);}
  if(ui.mixTab==='SCENES'){const i=ui.sceneCursor;add('SAVE SCENE '+String.fromCharCode(65+i),()=>sceneSave(i));if(p().scenes[i])add('RENAME SCENE',()=>actions.sceneName(i));}
  add('PROJECT / '+p().name,projectMenu);
 }
 if(ui.world==='TAPE'){
  if(ui.capture){add('FINISH CAPTURE',finishCapture);add('DISCARD CAPTURE',actions.cancelCapture);}
  else if(ui.tapeTab==='SOURCE'){add('IMPORT WAV',actions.importWav);add('FROM LIBRARY',actions.fromLibrary);}
  else if(ui.tapeTab==='EDIT'&&p().tapeDraft){add('SAVE SELECTED SLICE',actions.saveSlice);add('SAVE ALL SLICES',actions.saveAllSlices);add('SPLIT SELECTED SLICE',actions.splitDraft);add('ADD CUT / MIDPOINT',actions.addCut);add('REMOVE SELECTED SLICE',actions.removeCut);}
  else if(ui.tapeTab==='LIBRARY'&&library.length){add('EDIT COPY',actions.editLibrary);add('RENAME FILE',actions.sampleRename);add('DELETE FILE',actions.sampleDelete);}
 }
 add('UNDO',undo);add('BACK',()=>{});menu('OPTIONS / '+(ui.world==='TRACK'?ui.tab:ui.world),items,'Q+E / L1+R1 · Z select · X back');
}
const shoulderKeys=new Set();let shoulderChord=false;
function shoulderDown(key){if(ui.edit||ui.modal)return;if(shoulderKeys.has(key))return;shoulderKeys.add(key);if(shoulderKeys.size===2){shoulderChord=true;contextOptions();}}
function shoulderUp(key){if(!shoulderKeys.has(key))return;const single=!shoulderChord;shoulderKeys.delete(key);if(single&&!ui.edit&&!ui.modal)moveObject(key==='nextTrack'?1:-1);if(!shoulderKeys.size)shoulderChord=false;}
function resetShoulders(){shoulderKeys.clear();shoulderChord=false;}
const loadProjectWithoutCapture=actions.loadProject,newDemoWithoutCapture=actions.newDemo;Object.assign(actions,{contextOptions,loadProject:id=>ui.capture?captureExit(()=>loadProjectWithoutCapture(id)):loadProjectWithoutCapture(id),newDemo:()=>ui.capture?captureExit(newDemoWithoutCapture):newDemoWithoutCapture()});
function plotText(x,y,text,color='var(--dim)'){return `<text x="${x}" y="${y}" fill="${color}">${esc(text)}</text>`;}
function plotPath(points,color){return `<path d="${points.map(([x,y],i)=>(i?'L':'M')+x.toFixed(2)+' '+y.toFixed(2)).join(' ')}" fill="none" stroke="${color}" stroke-width="1.5"/>`;}
function plotGrid(x,y,w,h,cols,rows){return `<g class="plot-grid">${Array.from({length:cols+1},(_,i)=>`<path d="M${x+i*w/cols} ${y}v${h}"/>`).join('')}${Array.from({length:rows+1},(_,i)=>`<path d="M${x} ${y+i*h/rows}h${w}"/>`).join('')}</g>`;}
function plotSvg(body){return `<svg viewBox="0 0 600 190" role="img" aria-label="Схематическая модель параметров, не измерение аудио">${body}</svg>`;}
function modelCursor(x,w,y,h,seconds){return `<path id="model-cursor" data-x="${x}" data-width="${w}" data-seconds="${seconds}" d="M0 ${y}v${h}" stroke="var(--text)" stroke-width="1" opacity=".6" visibility="hidden"/>`;}
let lastModelPaint=0;
function paintModels(now){if(now-lastModelPaint<50)return;lastModelPaint=now;const cursor=$('#model-cursor');if(!cursor)return;const moving=ui.playing&&!document.hidden&&!matchMedia('(prefers-reduced-motion: reduce)').matches;cursor.setAttribute('visibility',moving?'visible':'hidden');if(moving){const seconds=Number(cursor.dataset.seconds),elapsed=(ui.head+(now-lastTick)/(60000/p().bpm/4))*60/p().bpm/4;cursor.setAttribute('transform',`translate(${Number(cursor.dataset.x)+(elapsed%seconds)/seconds*Number(cursor.dataset.width)} 0)`);}}

$('#screen').addEventListener('click',e=>{const el=e.target.closest('[data-action]');if(el&&!el.disabled)actions[el.dataset.action]?.(el.dataset.args);});
$('#screen').addEventListener('focusin',e=>{if(e.target.dataset.id)ui.focus=e.target.dataset.id;if(e.target.dataset.action==='loadProject')ui.projectCursor=projects.findIndex(v=>v.id===e.target.dataset.args);if(e.target.dataset.action==='sceneLoad')ui.sceneCursor=+e.target.dataset.args;if(e.target.dataset.action==='trackOpen')ui.track=+e.target.dataset.args;if(['loadProject','sceneLoad','trackOpen'].includes(e.target.dataset.action))document.querySelectorAll('[data-action="'+e.target.dataset.action+'"]').forEach(el=>el.classList.toggle('object-selected',el===e.target));if(e.target.dataset.action==='keyMenu'&&ui.key!==+e.target.dataset.args){ui.key=+e.target.dataset.args;updateKeyPanel();}const row=e.target.closest('[data-mix-track]');if(row){ui.track=+row.dataset.mixTrack;document.querySelectorAll('[data-mix-track]').forEach(el=>el.classList.toggle('selected-mix',el===row));}if(e.target.dataset.action==='step'&&ui.step!==+e.target.dataset.args){ui.step=+e.target.dataset.args;updateInspector();}});
$('#screen').addEventListener('pointermove',e=>{if(e.movementX===0&&e.movementY===0)return;const field=e.target.closest('[data-param]');if(field&&!ui.edit&&!ui.modal)field.focus({preventScroll:true});const keyEl=e.target.closest('[data-action="keyMenu"]');if(keyEl&&!ui.edit&&!ui.modal)keyEl.focus({preventScroll:true});const el=e.target.closest('[data-action="step"]');if(el&&!ui.edit&&!ui.modal){el.focus({preventScroll:true});if(ui.step!==+el.dataset.args){ui.step=+el.dataset.args;updateInspector();}}});
$('#screen').addEventListener('input',e=>{if(e.target.id==='name-input')ui.modal.value=e.target.value;});
function toggleTransport(){if(ui.world==='ATLAS')return;if(!p().tracks.length)return toast('Сначала добавьте дорожку');ui.playing=!ui.playing;lastTick=performance.now();if(!ui.playing)ui.queued=-1;render();}
function navigate(direction){const scope=ui.modal?$('#overlay'):$('#app'),all=[...scope.querySelectorAll('[data-nav]:not(:disabled)')].filter(e=>e.getClientRects().length);const current=all.find(e=>e===document.activeElement)||all[0];if(!current)return;const rect=current.getBoundingClientRect(),cx=rect.x+rect.width/2,cy=rect.y+rect.height/2;const candidates=all.filter(e=>e!==current).map(e=>{const r=e.getBoundingClientRect(),dx=r.x+r.width/2-cx,dy=r.y+r.height/2-cy,main=direction==='left'?-dx:direction==='right'?dx:direction==='up'?-dy:dy,cross=Math.abs(direction==='left'||direction==='right'?dy:dx);return {e,score:main+cross*3,main};}).filter(x=>x.main>2).sort((a,b)=>a.score-b.score);if(candidates[0]){candidates[0].e.focus({preventScroll:true});candidates[0].e.scrollIntoView({block:'nearest',inline:'nearest'});}}
function input(key){
 if(ui.modal){if(key==='b'||key==='atlas')return closeModal();if(key==='a')return document.activeElement?.click();if(['left','right','up','down'].includes(key))navigate(key);return;}
 if(key==='options')return contextOptions();if(key==='b')return back();if(key==='play')return toggleTransport();if(key==='atlas')return setWorld('ATLAS');
 if(key==='prevWorld'||key==='nextWorld'){if(ui.world==='ATLAS')return;const delta=key==='nextWorld'?1:-1;if(ui.world==='TRACK')return setTab(tabs[(tabs.indexOf(ui.tab)+delta+3)%3]);return setWorld(worlds[(worlds.indexOf(ui.world)+delta+worlds.length)%worlds.length]);}
 if(key==='prevTrack'||key==='nextTrack')return moveObject(key==='nextTrack'?1:-1);
 if(key==='x'&&ui.world==='TRACK'&&ui.tab==='PATTERN'){const isStep=document.activeElement?.dataset.action==='step';focusId(isStep?'edit:'+inspectorTarget()+':'+inspectorKeys()[0]:'step-'+ui.step);return;}
 if(key==='y'){if(ui.world==='TRACK'&&ui.tab==='PATTERN')return selectInspector(['STEP','LOCKS','FX','CYCLE'][(['STEP','LOCKS','FX','CYCLE'].indexOf(ui.inspectorTab)+1)%4]);if(ui.world==='MIX')return actions.mixTab(['LEVELS','FX','SCENES'][(['LEVELS','FX','SCENES'].indexOf(ui.mixTab)+1)%3]);if(ui.world==='TAPE')return actions.tapeTab(['SOURCE','EDIT','LIBRARY'][(['SOURCE','EDIT','LIBRARY'].indexOf(ui.tapeTab)+1)%3]);}
 if(key==='a')return document.activeElement?.click();
 if(['left','right','up','down'].includes(key)){
  if(ui.world==='TRACK'&&ui.tab==='PATTERN'&&document.activeElement?.dataset.action==='step'){const n=ui.step;if(key==='down'&&n%16>=8){focusId('edit:'+inspectorTarget()+':'+inspectorKeys()[0]);return;}const next=n+{left:-1,right:1,up:-8,down:8}[key];if(next>=0&&next<trackLength()){ui.step=next;ui.focus='step-'+next;document.activeElement.blur();render();return;}}
  if(ui.world==='TRACK'&&ui.tab==='VOICE'&&t().engine==='SAMPLER'&&document.activeElement?.dataset.action==='keyMenu'){
   const next=ui.key+{left:-1,right:1,up:-4,down:4}[key];if(next>=0&&next<=127){ui.key=next;ui.keyPage=Math.floor(next/16)*16;ui.focus='keyMenu:'+next;document.activeElement.blur();render();return;}
  }
  navigate(key);
 }
}
const keymap={ArrowUp:'up',ArrowDown:'down',ArrowLeft:'left',ArrowRight:'right',KeyZ:'a',KeyX:'b',KeyC:'x',KeyV:'y',KeyQ:'prevTrack',KeyE:'nextTrack',BracketLeft:'prevWorld',BracketRight:'nextWorld',Space:'play',Tab:'atlas',Escape:'b',Enter:'a'};
const stepModifiers={a:'note',b:'velocity',x:'gate',y:'probability'};
function isStepFocus(){return !ui.modal&&ui.world==='TRACK'&&ui.tab==='PATTERN'&&document.activeElement?.dataset.action==='step';}
function beginParameterGesture(key,code){
 const bank=parameterBank(),el=document.activeElement;
 if(ui.modal||!bank)return false;if(bank.target==='sampler'&&!keyAsset())return false;
 // Header tempo has its own focused Z gesture; banks apply inside the view.
 if(el?.closest('.topline'))return false;
 const param=bank.keys[['a','b','x','y'].indexOf(key)];if(!param)return false;

 if(isStepFocus())ui.step=+el.dataset.args;
 ui.stepGesture={key,code,used:false,started:performance.now()};
 ui.edit={target:bank.target,key:param,id:isStepFocus()?el.dataset.id:'edit:'+bank.target+':'+param,changed:false};markEditing();return true;
}
function releaseStepEdit(code){const g=ui.stepGesture;if(!g||g.code!==code)return false;const tap=!g.used&&performance.now()-g.started<300;finishEdit();if(tap)input(g.key);return true;}
document.addEventListener('keydown',e=>{
 if(e.target.matches('input')){if(e.code==='Escape'){e.preventDefault();closeModal();}else if(e.code==='Enter'){e.preventDefault();actions.textSave();}else if(e.code==='Tab'){e.preventDefault();closeModal();}return;}
 if((e.ctrlKey||e.metaKey)&&e.code==='KeyZ'){e.preventDefault();undo();return;}
 if(e.ctrlKey||e.metaKey||e.altKey)return;
 const key=keymap[e.code];if(!key)return;e.preventDefault();if(key==='prevTrack'||key==='nextTrack'){if(!e.repeat)shoulderDown(key);return;}
 if(ui.edit){
  if(e.code==='Escape'||e.code==='Tab'||(!ui.stepGesture&&key==='b')){finishEdit(true);return;}
  if(['left','right','up','down'].includes(key)&&!e.repeat&&ui.arrow!==key){ui.arrow=key;ui.arrowStart=performance.now();ui.arrowLast=performance.now();adjustInline(key);}
  return;
 }
 if(!e.repeat&&['KeyZ','KeyX','KeyC','KeyV'].includes(e.code)&&beginParameterGesture(key,e.code))return;
 if(key==='a'&&e.code!=='Enter'&&!ui.modal&&document.activeElement?.dataset.param){if(!e.repeat){ui.aHeld=true;beginEdit(document.activeElement);}return;}
 if(e.repeat&&!['up','down','left','right'].includes(key))return;input(key);
});
document.addEventListener('keyup',e=>{if(e.code==='KeyQ'||e.code==='KeyE'){shoulderUp(keymap[e.code]);return;}if(releaseStepEdit(e.code)){e.preventDefault();return;}const key=keymap[e.code];if(key==='a'&&ui.edit&&!ui.stepGesture){e.preventDefault();finishEdit();}if(key===ui.arrow)ui.arrow=null;});
window.addEventListener('blur',()=>{resetShoulders();finishEdit();});
document.querySelectorAll('[data-key]').forEach(b=>{
 b.addEventListener('pointerdown',e=>{e.preventDefault();const el=[...$('#screen').querySelectorAll('[data-id]')].find(e=>e.dataset.id===ui.focus);el?.focus({preventScroll:true});const key=b.dataset.key;if(key==='prevTrack'||key==='nextTrack'){shoulderDown(key);b.setPointerCapture(e.pointerId);return;}
  if(ui.edit){if(['up','down','left','right'].includes(key))adjustInline(key);return;}
  if(stepModifiers[key]&&beginParameterGesture(key,'pointer:'+key)){b.setPointerCapture(e.pointerId);}
  else if(key==='a'&&el?.dataset.param&&!ui.modal){ui.pointerA=true;ui.aHeld=true;beginEdit(el);b.setPointerCapture(e.pointerId);}
  else input(key);
 });
 b.addEventListener('pointerup',()=>{if(b.dataset.key==='prevTrack'||b.dataset.key==='nextTrack'){shoulderUp(b.dataset.key);return;}if(releaseStepEdit('pointer:'+b.dataset.key))return;if(b.dataset.key==='a'&&!ui.stepGesture)finishEdit();});
 b.addEventListener('pointercancel',()=>{resetShoulders();finishEdit();});
});
$('#play-button').onclick=toggleTransport;$('#export').onclick=exportProject;$('#undo').onclick=undo;$('#import').onclick=()=>{if(ui.capture)return toast('Сначала завершите захват');$('#import-file').click();};
$('#import-file').onchange=async e=>{
 const file=e.target.files[0];if(!file)return;
 try{
  if(file.size>8000000)throw Error('Файл больше 8 MB');const data=JSON.parse(await file.text());if(data.format!=='moss-project'||![1,2].includes(data.version))throw Error('Нужен экспорт MOSS');
  const assets=[];let imported=data.version===1?S.migrate(data.project,assets):M.validate(data.project);
  if(data.version===2){if(!Array.isArray(data.assets)||data.assets.length>4096)throw Error('Неверная библиотека');assets.push(...data.assets.map(S.validateAsset));if(new Set(assets.map(a=>a.id)).size!==assets.length)throw Error('Повтор ID сэмпла');}
  const remap=new Map();for(const asset of assets){const previous=asset.id;asset.id=M.uid();remap.set(previous,asset.id);}
  for(const tr of [...imported.tracks,...imported.scenes.flatMap(sc=>sc?sc.data.tracks:[])])for(const a of Object.values(tr.kit.keys))if(remap.has(a.assetId))a.assetId=remap.get(a.assetId);
  imported.id=M.uid();commit(()=>{library.push(...assets);projects.push(imported);activeId=imported.id;resetSession();ui.world='GRID';});toast('Проект и сэмплы импортированы отдельной копией');
 }catch(err){toast('Импорт не выполнен: '+err.message);}e.target.value='';
};

$('#help-toggle').onclick=()=>{document.body.classList.toggle('no-guide');fit();};$('#size-button').onclick=()=>{ui.large=!ui.large;fit();};
document.querySelectorAll('[data-scenario]').forEach(b=>b.onclick=()=>{const kind=b.dataset.scenario;if(kind==='blank')newProject();if(kind==='demo'){const d=projects.find(v=>v.name==='SLOW SIGNAL');if(d)actions.loadProject(d.id);else actions.newDemo();}if(kind==='scenes'){ui.mixTab='SCENES';setWorld('MIX');}});
function fit(){const available=$('.instrument-area').clientWidth,scale=Math.min(ui.large?1.25:1,available/640);$('#screen').style.transform=`scale(${scale})`;$('#screen-wrap').style.width=640*scale+'px';$('#screen-wrap').style.height=480*scale+'px';$('#size-button').textContent='Масштаб '+scale.toFixed(2).replace('.00','')+'×';}window.addEventListener('resize',fit);
let lastTick=performance.now();
function paintClock(){document.querySelectorAll('.pattern-steps .step').forEach(el=>el.classList.toggle('playhead',ui.playing&&+el.dataset.args===M.position(p(),t(),ui.head)));if(ui.capture){const elapsed=(performance.now()-ui.capture.started)/1000,time=$('#capture-time'),bar=$('#capture-progress');if(time)time.textContent=String(Math.floor(elapsed/60)).padStart(2,'0')+':'+String(Math.floor(elapsed%60)).padStart(2,'0');if(bar)bar.style.width=Math.min(100,elapsed/ui.capture.duration*100)+'%';if(elapsed>=ui.capture.duration)finishCapture();}}
setInterval(()=>{const now=performance.now();if(ui.edit&&ui.arrow&&now-ui.arrowStart>250&&now-ui.arrowLast>=70){ui.arrowLast=now;adjustInline(ui.arrow,now-ui.arrowStart);}if(ui.playing){const duration=60000/p().bpm/4;if(now-lastTick>=duration){const count=Math.floor((now-lastTick)/duration);lastTick+=count*duration;const wraps=Math.floor((ui.head+count)/p().length)>Math.floor(ui.head/p().length);ui.head+=count;if(wraps&&ui.queued>=0&&!ui.edit&&!ui.modal){const q=ui.queued;ui.queued=-1;commit(()=>{M.restoreScene(p(),q);ui.head=0;});}}}paintClock();paintModels(now);},30);
document.addEventListener('visibilitychange',()=>{lastTick=performance.now();if(document.hidden){resetShoulders();finishEdit();}});
// SVG geometry helpers are inserted here by the source migration.
function organism(){
 const a=t().params;let body=plotGrid(100,30,380,126,4,3);const intervals=[0,7,10,12],base=t().patterns[p().pattern].find(st=>st.on)?.note??43;
 for(let i=0;i<4;i++){const y=41+i*32,active=i<Math.max(1,Math.ceil(a.density/25)),spread=a.drift/100*30;
  body+=plotText(8,y+4,'OSC '+(i+1))+plotText(62,y+4,M.note(base+intervals[i]));
  body+=`<rect x="${290-spread}" y="${y-9}" width="${spread*2}" height="18" fill="var(--slot-2)" opacity=".16"/>`;
  const points=Array.from({length:150},(_,j)=>{const x=100+j*380/149,v=Math.sin(j*.22*(1+i*.18))+a.chaos/100*.5*Math.sin(j*.91+i);return [x,y-v*(active?8:2)];});body+=plotPath(points,active?'var(--text)':'var(--line)');
  body+=`<line x1="500" y1="${y}" x2="${500+a.tension*.65}" y2="${y}" stroke="var(--slot-1)" stroke-width="2"/>`;
 }
 body+=plotText(100,180,'DRIFT ±'+Math.round(a.drift*.3)+' ct')+plotText(490,180,'LINK '+a.tension+'%');
 return plotSvg(body+modelCursor(100,380,25,135,2));
}
function mono(){
 const a=t().params,xFor=f=>30+300*Math.log(f/20)/Math.log(1000),cut=xFor(a.cutoff);let body=plotGrid(30,30,300,125,3,3)+plotGrid(380,30,190,125,2,2);
 const pts=Array.from({length:181},(_,i)=>{const f=20*Math.pow(1000,i/180),r=f/a.cutoff,db=-10*Math.log10(1+r**8)+a.resonance*.13*Math.exp(-Math.pow(Math.log2(r)/.28,2));return [30+i*300/180,52-M.clamp(db,-60,12)*1.65];});
 body+=plotPath(pts,'var(--text)')+`<path d="M${cut} 30v125" stroke="var(--slot-1)" stroke-dasharray="3 3"/>`;
 body+=plotText(3,55,'0')+plotText(3,88,'−20')+plotText(3,121,'−40')+plotText(3,154,'−60')+plotText(30,20,'LPF / 24 dB')+plotText(190,20,a.cutoff+' Hz', 'var(--slot-1)')+plotText(30,173,'20')+plotText(125,173,'200')+plotText(228,173,'2k')+plotText(310,173,'20k');
 const total=Math.max(1000,a.attack+a.release+300),ax=380+a.attack/total*190,rx=380+(a.attack+300)/total*190;
 body+=`<path d="M380 155L${ax} 42H${rx}Q${rx+15} 148 570 155" stroke="var(--slot-3)" fill="none" stroke-width="1.5"/>`;
 body+=plotText(380,20,'AMP / A–R')+plotText(380,173,'0')+plotText(501,173,(total/1000).toFixed(1)+' s')+modelCursor(380,190,30,125,total/1000);
 return plotSvg(body);
}
function waveform(seed=37){return Array.from({length:150},(_,i)=>{const h=8+Math.abs(Math.sin(i*1.7+seed)*Math.sin(i*.21+seed))*(32+Math.sin(i*.04)*16);return `<path d="M${i*4} ${62-h}v${h*2}"/>`;}).join('');}
function orbitDiagram(){
 const f=effectOwner(),beats=[.25,.5,.75,1,1.5,2,3,4][f.time],ms=beats*60000/p().bpm,wet=f.mix/100;let body=plotGrid(35,30,535,125,4,2);
 body+=plotText(35,18,'L / R · IMPULSE RESPONSE')+plotText(415,18,Math.round(ms)+' ms');
 for(let i=0;i<=Math.floor(4/beats);i++){const x=35+i*beats/4*535,g=i===0?1-wet:wet*Math.pow(f.feedback/100,i-1);for(let side=0;side<2;side++){const y=83+side*65,pan=i===0?1:1-(i%2===side?0:f.spread/100*.8),h=g*45*pan;body+=`<path d="M${x} ${y}v${-h}" stroke="${i===0?'var(--text)':side===0?'var(--slot-3)':'var(--slot-4)'}" stroke-width="2"/>`;}}
 body+=plotText(7,60,'L')+plotText(7,125,'R');for(let i=0;i<=4;i++)body+=plotText(35+i*535/4,178,i===4?'4 BEATS':String(i));
 return plotSvg(body+modelCursor(35,535,30,125,4*60/p().bpm));
}
function pondDiagram(){
 const f=effectOwner(),decay=.3+f.decay/100*9.7,wet=f.mix/100,start=.02+f.size/100*.18;let body=plotGrid(35,30,535,125,4,3);
 const curve=hf=>Array.from({length:181},(_,i)=>{const sec=i/180*10,amp=sec<start?0:wet*Math.exp(-6.908*(sec-start)/decay*(hf?1+f.damping/35:1));return [35+i/180*535,155-amp*110];});
 body+=plotPath(curve(false),'var(--slot-2)')+plotPath(curve(true),'var(--slot-3)');
 for(let i=0;i<5;i++){const sec=start+i*(.03+f.size*.0012),x=35+sec/10*535;body+=`<path d="M${x} 155v${-wet*80/(1+i*.3)}" stroke="var(--text)" opacity=".5"/>`;}
 body+=plotText(35,18,'REVERB RESPONSE')+plotText(240,18,'LF','var(--slot-2)')+plotText(285,18,'HF','var(--slot-3)')+plotText(375,18,'TAIL '+decay.toFixed(1)+' s')+plotText(35,177,'0')+plotText(285,177,'5')+plotText(540,177,'10 s');
 return plotSvg(body+modelCursor(35,535,30,125,10));
}
render();fit();if(storageError)toast('Ошибка чтения сохранения. Исходные данные оставлены в recovery.');else save();

function setCycleLength(n){const copy=M.clone(p());M.setLength(copy,copy.tracks[ui.track],n);const mem=S.memory(copy,library);if(mem.bytes>S.RAM_BUDGET||mem.oversized.length)return toast('Лимит PCM — подрежьте сэмплы в TAPE');commit(()=>M.setLength(p(),t(),n));}
function crushDiagram(){
 const f=effectOwner(),wet=f.mix/100,levels=2**(f.bits-1),gain=1+f.drive/100*15;
 const shape=x=>{const driven=M.clamp(x*gain,-1,1);return (1-wet)*x+wet*Math.round(driven*levels)/levels;};
 const transfer=Array.from({length:181},(_,i)=>{const x=i/90-1;return [30+i*230/180,95-shape(x)*62];});
 const quantize=x=>Math.round(M.clamp(x*gain,-1,1)*levels)/levels;let held=0;const signal=Array.from({length:256},(_,i)=>{const dry=Math.sin(i/255*Math.PI*4);if(i%f.rate===0)held=quantize(dry);return [310+i*265/255,95-((1-wet)*dry+wet*held)*62];});
 return plotSvg(plotGrid(30,33,230,124,4,4)+plotGrid(310,33,265,124,4,4)+plotText(30,20,'TRANSFER / IN → OUT')+plotText(310,20,'REDUX / TEST SINE')+plotPath(transfer,'var(--text)')+plotPath(signal,'var(--text)')+plotText(30,181,'DRIVE '+f.drive+'% · '+f.bits+' BIT')+plotText(310,181,'RATE ÷'+f.rate+' · WET '+f.mix+'%'));
}
