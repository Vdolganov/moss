import {MENU_PAGE_SIZE} from './navigation.js';
import {BUTTONS,cells,stride,getStep,getHit,noteName,sourceFrames,assetUsed,hasLocks} from './model.js';
import {formatValue,compactUnit} from './fields.js';
import {syncTabStrips} from './tab-strips.js';
import {LAYOUT as L,SPACE as S} from './layout.js';
import {FACE_POSITIONS} from './face-buttons.js';
import {continuationAt} from './note-timing.js';
import {durationLines,durationLabel} from './note-lines.js';
import {patternLayout} from './grid-layout.js';
import {patternTabs,patternTabValue,normalizePatternTab,fxLockPage,activeNestedPage,shoulderHints} from './nested-pages.js';
export const esc=x=>String(x??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const box=(x,y,w,h,content,cls='',attrs='')=>`<div class="pos ${cls}" style="left:${x}px;top:${y}px;width:${w}px;${h?`height:${h}px;`:''}" ${attrs}>${content}</div>`;
const txt=(x,y,text,cls='',width=L.content)=>box(x,y,width,0,esc(text),`cut ${cls}${cls==='dim'?' small':''}`);
const button=(x,y,w,h,label,action,value='',cls='',attrs='')=>`<button tabindex="-1" type="button" class="pos ${cls}" style="left:${x}px;top:${y}px;width:${w}px;height:${h}px" data-action="${action}" data-value="${esc(value)}" ${attrs}>${label}</button>`;
const rule=y=>`<div class="rule" style="top:${y}px"></div>`;
function tabStrip(names,selected,action,values,{kind='parameter',hideTurns=false,nested=[]}={}){
 const labels={'VOICE LOCKS':'V-LOCK',TIMING:'TIME'},major=kind==='major'||kind==='library',label={major:'Main navigation',library:'Library',section:'Section pages',parameter:'Parameter pages'}[kind];
 const turn=(key,next)=>`<button tabindex="-1" type="button" class="page-turn" data-action="action" data-value="${key}" aria-label="${next?'Next':'Previous'} ${major?'section':'page'} (${key})" ${names.length===1||hideTurns?'disabled':''}>${next?'':key}<svg viewBox="0 0 8 12" aria-hidden="true"><path d="${next?'M2 2L6 6L2 10':'M6 2L2 6L6 10'}"/></svg>${next?key:''}</button>`;
 const key=esc([action,...names].join('|'));
 const tabs=names.map((name,i)=>{
  const item=typeof values[i]==='object'?values[i]:{action,value:values[i]},active=i===selected,stack=nested[i],multiple=stack?.names.length>1;
  const extra=multiple?(active?`<span class="nested-count" aria-hidden="true">${stack.index+1}/${stack.names.length}</span>`:axisIcon('vertical')):'';
  const description=multiple?`${name} / ${stack.kind} ${stack.index+1} OF ${stack.names.length} / ${stack.names[stack.index]} / HOLD L1 AND UP OR DOWN`:name;
  const tab=`<button tabindex="-1" type="button" class="nav-tab ${kind==='parameter'?'page-tab':kind==='major'?'major-tab':'section-tab'} ${active?'active':''} ${multiple?'has-nested':''}" data-action="${item.action}" data-value="${esc(item.value)}" role="tab" aria-selected="${active}" aria-label="${esc(description)}"><span class="tab-label">${esc(labels[name]||name)}</span>${extra}</button>`;
  if(!multiple||!active)return tab;
  const arrows=[-1,1].map(delta=>`<button tabindex="-1" type="button" class="nested-turn" data-action="nested-page" data-value="${stack.id}:${delta}" aria-label="${delta<0?'Previous':'Next'} ${stack.kind.toLowerCase()} in ${esc(name)}"><svg viewBox="0 0 10 6" aria-hidden="true"><path d="${delta<0?'M1 5L5 1L9 5':'M1 1L5 5L9 1'}"/></svg></button>`).join('');
  return `<span class="nested-tab active"><span class="nested-tab-label">${tab}</span><span class="nested-controls">${arrows}</span></span>`;
 }).join('');
 return `<div class="tab-strip ${kind}-pages ${names.length===1||hideTurns?'no-turns':''}">${turn(major?'L2':'L1',false)}<div class="tab-viewport" data-strip="${key}"><div class="page-options" role="tablist" aria-label="${label}">${tabs}</div></div>${turn(major?'R2':'R1',true)}</div>`;
}
function axisIcon(axis){return `<svg class="axis-icon ${axis}" viewBox="0 0 12 16" aria-hidden="true"><path d="M2 5L6 1L10 5 M2 11L6 15L10 11"/></svg>`;}
const parameterPages=(names,selected,action,nested=[])=>tabStrip(names,selected,action,names.map((_,i)=>i),{nested});
function nestedRail(a){
 const page=activeNestedPage(a);if(!page||page.names.length<2)return '';
 const count=page.names.length,pitch=Math.min(S[8],L.fieldHeight/count),height=pitch*count;
 const marks=page.names.map((_,i)=>`<rect class="${i===page.index?'current':''}" x="${i===page.index?0:2}" y="${i*pitch+(pitch-2)/2}" width="${i===page.index?8:4}" height="2"/>`).join('');
 return `<svg class="nested-rail" style="left:${S[4]}px;top:${L.fieldTop+(L.fieldHeight-height)/2}px" width="8" height="${height}" viewBox="0 0 8 ${height}" data-index="${page.index}" data-count="${count}" aria-hidden="true">${marks}</svg>`;
}

function row(y,label,detail,selected,action,index,h=L.rowHeight){return button(L.inset,y,L.content,h,`<span class="cut">${esc(label)}</span><span class="right">${esc(detail)}</span>`,action,index,`row ${selected?'active':''}`);}
function footer(a,left,right='OPTIONS'){
 const held=BUTTONS.findIndex(key=>a.input?.held.has(key));
 if(held>=0&&!a.ui.dialog&&a.params()[held]){const f=a.params()[held];return `<div class="footer edit-hint"><span>${esc(BUTTONS[held])} · ←/→ ${esc(f.fineLabel)} · ↑/↓ ${esc(f.coarseLabel)}</span><span>${esc(f.lock?'MENU LOCK OPTIONS':'HOLD ARROW: FASTER')}</span></div>`;}
 if(a.input?.held.has('L1')&&!a.ui.dialog&&!a.ui.capture&&!a.input.held.has('Select')){
  const hints=shoulderHints(a).map(h=>`<span class="shoulder-action">${axisIcon(h.axis)}${esc(h.text)}</span>`).join('');
  return `<div class="footer shoulder-hint"><span class="shoulder-actions"><span class="shoulder-key">L1</span>${hints||'<span>NO NESTED PAGES</span>'}</span><span>+ R1 OPTIONS</span></div>`;
 }
 const actions=left.split(/ \/ (?=[YBAX] |OPTIONS)/).map(part=>part.startsWith('OPTIONS')?`<button tabindex="-1" data-action="options">${esc(part)}</button>`:`<button tabindex="-1" data-action="action" data-value="${part[0]}">${esc(part)}</button>`).join('<span> / </span>');
 const lhs=left==='Y/B/A/X PREVIEW'?`<span class="preview-keys">${BUTTONS.map((key,i)=>`<button tabindex="-1" data-action="preview-part" data-value="${i}" aria-label="Preview part ${i+1} (${key})">${key}</button>`).join('')} PREVIEW · MENU BACK</span>`:`<span class="footer-actions">${actions}</span>`;
 const rhs=right==='OPTIONS'?'<button tabindex="-1" data-action="options">L1+R1 OPTIONS</button>':right==='START PLAY SONG'?'<button tabindex="-1" data-action="transport">START PLAY SONG</button>':`<span>${esc(right)}</span>`;
 return `<div class="footer">${lhs}${rhs}</div>`;
}
function header(a){
 const u=a.ui,song=u.page==='song'?a.song():null,p=a.sequence();
 return txt(L.inset,S[16],a.session.playing?'>':'-','',S[16])
  +button(48,S[16],384,L.line,esc(['pattern','voice','space'].includes(u.page)&&a.track()?`${p.name.slice(0,16)} / ${String(u.ti+1).padStart(2,'0')} ${a.track().name}`:song?.name||p?.name||'MOSS'),'back','','cut')
  +button(448,S[16],176,L.line,esc(`SEL ${song?.bpm||p?.bpm||120} BPM`),'tempo','','dim')+rule(L.headerBottom);
}
function faceGlyph(active){return `<svg class="face-cluster" viewBox="0 0 24 24" aria-hidden="true" focusable="false">${Object.entries(FACE_POSITIONS).map(([key,[cx,cy]])=>`<circle class="face-button ${key===active?'is-active':''}" data-face="${key}" cx="${cx}" cy="${cy}" r="3"/>`).join('')}</svg>`;}
function fieldRows(a){const fields=a.params();if(!fields.length)return'';return '<div class="parameter-fields">'+fields.map((f,i)=>{
 const key=BUTTONS[i],held=a.input?.held.has(key),text=f.text,unit=compactUnit(f);
 // Moss has a fixed 600/700-em advance. Preserve exact long positions by
 // moving their unit to the label row instead of wrapping or shrinking digits.
 const tight=unit==='%',gap=tight?0:S[4];
 const labelUnit=unit&&(text.length*14*6/7+unit.length*12*6/7+gap>L.fieldTextWidth);
 const unitMarkup=unit?`<span class="field-unit">${esc(unit)}</span>`:'';
 const control=`<span class="field-control" aria-hidden="true"><span class="field-key">${key}</span>${faceGlyph(key)}</span>`;
 const content=`<span class="field-content"><span class="field-label">${esc(f.label)}${labelUnit?' '+unitMarkup:''}</span><span class="field-reading ${tight?'tight':''}"><span class="field-value">${esc(text)}</span>${labelUnit?'':unitMarkup}</span></span>`;
 return `<button tabindex="-1" type="button" data-action="param" data-value="${i}" class="field ${held?'held':''} ${f.disabled?'unavailable':''} ${f.lock?'lock-field':''} ${f.lock&&f.lock.value!==null?'has-lock':''} ${text==='RST'?'reset-lock':''} ${a.ui.pointerParam===i?'picked':''}" aria-disabled="${!!f.disabled}" aria-label="${esc(key+' '+f.label+' '+text+' '+f.unit)}">${control+content}</button>`;
}).join('')+'</div>'+nestedRail(a);}
function keySlots(a,y=L.contentY){
 return a.track().keys.map((key,i)=>{const asset=a.data.assets.find(x=>x.id===key.asset),playing=!!a.sampleVoice(i);
  return button(L.inset+(L.slotWidth+S[8])*i,y,L.slotWidth,L.rowHeight,`<span class="slot-note">${esc(noteName(48+i))}</span><span class="slot-name">${esc(asset?.name||'ASSIGN SAMPLE')}</span>`,'slot',i,`key-slot ${a.ui.key===i?'active':''} ${playing?'playing':''}`,`aria-pressed="${a.ui.key===i}" aria-label="${esc(noteName(48+i)+' '+(asset?.name||'ASSIGN SAMPLE')+(a.ui.key===i?' / SELECTED':'')+(playing?' / PLAYING':''))}"`);
 }).join('');
}
function major(a){const track=['pattern','voice','space'].includes(a.ui.page),names=track?['PATTERN','VOICE','SPACE']:['GRID','MIX','TAPE'],pages=track?['pattern','voice','space']:['grid','mix','tape'];return tabStrip(names,pages.indexOf(a.ui.page),'major',pages,{kind:'major'})+rule(L.mainRule);}
function library(a){
 const u=a.ui,list=u.atlasTab?a.data.songs:a.data.sequences,first=Math.floor(u.row/5)*5;
 let html=txt(L.inset,S[16],'MOSS','big',240)+txt(384,S[24],'LIBRARY / 0.12','dim',240)+rule(L.headerBottom)+tabStrip(['SEQUENCES','SONGS'],u.atlasTab,'atlas-tab',[0,1],{kind:'library'})+rule(L.mainRule);
 for(let i=first;i<=Math.min(first+4,list.length);i++){const p=list[i];html+=row(L.contentY+(i-first)*(L.rowHeight+L.rowGap),p?p.name:u.atlasTab?'+ NEW SONG':'+ NEW SEQUENCE',p?(u.atlasTab?`${p.slots.length} SLOTS`:`${p.tracks.length} TRACKS`):'',u.row===i,'atlas-row',i);}
 return html+txt(L.inset,L.metaY,`${list.length} / 32 ${u.atlasTab?'SONGS':'SEQUENCES'}`,'dim')+txt(480,L.metaY,`${Math.floor(u.row/5)+1}/${Math.ceil((list.length+1)/5)}`,'dim',144)+footer(a,'A OPEN / CREATE');
}
function grid(a){
 const p=a.sequence(),u=a.ui,start=L.contentY+L.line+S[16],pitch=L.rowHeight+L.rowGap;
 let html=txt(28,L.contentY,'TRACK','dim',232)+txt(272,L.contentY,'VOICE','dim',192)+txt(476,L.contentY,'SPACE','dim',136);
 p.tracks.forEach((t,i)=>{const y=start+i*pitch;
  html+=button(L.inset,y,L.content,L.rowHeight,`<span class="cut" style="width:232px">${String(i+1).padStart(2,'0')} ${esc(t.name)}</span><span style="width:192px" class="dim">${a.C.instruments[t.instrument].name}</span><span class="dim">${t.fx.enabled.length}/2 ${t.mute?'M':'FX'}</span>`,'track',i,`row ${u.ti===i?'active':''}`);
  if(a.session.playing&&!t.mute)html+=box(28,y+L.rowHeight-S[4],Math.floor((a.session.tick%t.cycle)/t.cycle*(L.content-S[12]*2)),2,'','track-progress');
 });
 html+=row(start+p.tracks.length*pitch,p.tracks.length<4?'+ ADD TRACK':'FOUR TRACKS / FULL','',u.ti===p.tracks.length,'track',p.tracks.length,L.compactRowHeight);
 return html+txt(L.inset,L.metaY,'A OPEN / X MUTE / SELECT + ARROWS BPM','dim')+footer(a,'A OPEN TRACK');
}
function pattern(a){
 const u=a.ui,t=a.track(),n=cells(t),at=u.cell*stride(t),sampler=t.instrument===0,spans=a.noteSpans();
 const g=patternLayout(t,u.gridSize),page=Math.floor(u.cell/g.pageSize),pages=Math.ceil(n/g.pageSize),gridY=g.top,cellHeight=g.height,detailY=g.contextY+S[4];
 const first=page*g.pageSize,last=Math.min(first+g.pageSize,n);
 const finer=new Set(),collect=(events,shared=false)=>{for(const [position,event] of Object.entries(events)){const tick=+position;if(event.type>0&&(!shared||event.type===2)&&tick<t.cycle&&tick%stride(t))finer.add(Math.floor(tick/stride(t)));}};
 collect(sampler?t.hits[u.key]:t.steps);if(sampler)collect(t.steps,true);
 let html=sampler?keySlots(a):txt(L.inset,L.contentY,`${String(u.ti+1).padStart(2,'0')} ${t.name}`,'',384);
 if(!sampler)html+=txt(432,L.contentY,`${n} STEPS / 1/${t.division}`,'dim',192);
 const turn=(delta,disabled)=>`<button tabindex="-1" type="button" class="grid-page-turn" data-action="page" data-value="${delta}" aria-label="${delta<0?'Previous':'Next'} grid page" ${disabled?'disabled':''}><svg viewBox="0 0 8 12" aria-hidden="true"><path d="${delta<0?'M6 2L2 6L6 10':'M2 2L6 6L2 10'}"/></svg></button>`;
 html+=`<div class="grid-mode" style="top:${g.contextY}px" role="group" aria-label="Steps per page"><span class="grid-pagination">${turn(-1,page===0)}<span class="grid-page-count" aria-label="Page ${page+1} of ${pages}">${page+1}/${pages}</span>${turn(1,page===pages-1)}</span>${[16,64].map(size=>`<button type="button" tabindex="-1" class="nav-tab ${size===g.pageSize?'active':''}" data-action="grid-size" data-value="${size}" aria-pressed="${size===g.pageSize}" aria-label="${size} steps per page">${size}</button>`).join('')}</div>`;
 for(let i=first;i<last;i++){
  const tick=i*stride(t),e=sampler?getHit(t,u.key,tick):getStep(t,tick),shared=getStep(t,tick),on=e.type===1;
  const sharedLock=sampler&&shared.type===2,lockOnly=!on&&(e.type===2||sharedLock),locked=lockOnly||sharedLock||(e.type>0&&hasLocks(e));
  const selected=u.cell===i,playhead=a.session.playing&&Math.floor((a.session.tick%t.cycle)/stride(t))===i;
  const continued=continuationAt(spans,tick),carry=g.tier==='full'&&!on&&i===first&&continued;
  const caption=on?(sampler?'HIT':noteName(e.note)):carry?'':lockOnly?'LOCK':'';
  // Preserve finer events when a coarser grid hides their exact positions.
  const sparse=finer.has(i);
  const diamond=locked?'<svg class="cell-lock" viewBox="0 0 12 12" aria-hidden="true" focusable="false"><path d="M6 1 L11 6 L6 11 L1 6 Z"/></svg>':'';
  const label=carry?durationLabel(carry,tick,stride(t)):null;
  const badge=label?`<span class="carry-label" style="width:${label.width}px" aria-hidden="true">${esc(label.text)}</span>`:'';
  const fullContent=`<span class="cell-top" aria-hidden="true"><span class="cell-index ${sparse?'has-substeps':''}">${String(i+1).padStart(2,'0')}</span>${diamond}</span><span class="event" aria-hidden="true">${caption}</span>${badge}`;
  const content=g.tier==='full'?fullContent:`<span class="compact-text ${on?'is-note':''}" aria-hidden="true">${on?(sampler?noteName(48+u.key):noteName(e.note)):lockOnly?'':i+1}</span>${diamond}`;
  const description=`STEP ${i+1} ${on?(sampler?'HIT':'NOTE '+noteName(e.note)):lockOnly?'LOCK ONLY':'EMPTY'}${continued?' / CONTINUES '+noteName(continued.note)+' FROM STEP '+(Math.floor(continued.origin/stride(t))+1):''}${locked?' / LOCKS':''}${sparse?' / FINER EVENTS':''}${selected?' / SELECTED':''}${playhead?' / PLAYING':''}`;
  html+=button(L.inset+(i-first)%g.columns*(g.width+g.gap),gridY+Math.floor((i-first)/g.columns)*(cellHeight+g.gap),g.width,cellHeight,content,'cell',i,`cell ${g.tier==='full'?'':'compact-cell'} ${sparse?'finer-events':''} ${on?'on':''} ${lockOnly?'lock':''} ${selected?'selected':''} ${playhead?'playhead':''}`,`aria-label="${esc(description)}"${playhead?' aria-current="step"':''}`);
 }
 html+=durationLines(spans,t,page,gridY,cellHeight,g);
 const e=sampler?getHit(t,u.key,at):getStep(t,at),continued=continuationAt(spans,at);let detail=`${String(u.cell+1).padStart(2,'0')} / ${n}   ${sampler?noteName(48+u.key):noteName(e.note)}   1/${t.division}`;
 if(continued){const length=+(continued.length/stride(t)).toFixed(1);detail=`${noteName(continued.note)} / FROM ${Math.floor(continued.origin/stride(t))+1} / ${length} stps${e.type===2||sampler&&getStep(t,at).type===2?' / LOCK':''}`;}
 if(u.bank===1)detail=`STEP ${u.cell+1} / ${sampler?'SLOT '+noteName(48+u.key):'TRACK'} / ${a.C.instruments[t.instrument].banks[u.voiceBank]||'SAMPLE'}`;
 
 if(u.bank===3)detail=`TIMING ${u.timingBank+1}/2   SWING ${t.swing}%`;
 if(g.pageSize===64){
  const selectedNote=sampler?noteName(48+u.key):noteName(continued?.note??e.note);
  const context=u.bank===0?`1/${t.division}${continued?' / FROM '+(Math.floor(continued.origin/stride(t))+1):e.type===2?' / LOCK':''}`
   :u.bank===1?`V / ${a.C.instruments[t.instrument].banks[u.voiceBank]||'SAMPLE'}`
   :u.bank===2?''
   :`TIME ${u.timingBank+1}/2 / ${t.swing}%`;
  detail=`${u.cell+1}/${n} ${selectedNote} / ${context}`;
 }
 const fx=fxLockPage(a);
 if(u.bank===2)detail=`STEP ${u.cell+1}/${n} TRACK / ${fx?.name||'ENABLE FX IN SPACE'}`;if(sampler&&u.bank!==2)detail=`STEP ${u.cell+1}/${n} SLOT ${noteName(48+u.key)} / ${u.bank===1?'SAMPLE LOCKS':u.bank===3?'TRACK TIME':`DIV 1/${t.division}`}`;
 const tabs=patternTabs(a),selected=tabs.findIndex(tab=>tab.value===patternTabValue(a));
 return html+txt(L.inset,detailY,detail,'dim pattern-context',416)
  +tabStrip(tabs.map(t=>t.name),selected,'bank',tabs.map(t=>t.value),{nested:tabs.map(t=>t.nested)})
  +fieldRows(a)+footer(a,'A NOTE / OPTIONS CLEAR','R1+ARROWS PAGE');
}
function voice(a){
 const u=a.ui,t=a.track(),inst=a.C.instruments[t.instrument],preset=inst.presets[t.presets[t.instrument]];
 let html=txt(L.inset,L.contentY,`${String(u.ti+1).padStart(2,'0')} ${t.name}${preset?' / '+preset.name:''}`,'dim')+txt(L.inset,L.voiceTitleY,inst.name,'big',352)+txt(384,L.voiceTitleY+S[4],`${inst.modes[t.modes[t.instrument]]||''} / ${noteName(t.audition)}`,'dim',240)+rule(L.voiceRule);
 if(t.instrument===4){const asset=a.data.assets.find(x=>x.id===t.grainAsset);html=txt(L.inset,L.contentY,'GRANULAR','big',320)+txt(368,L.contentY+S[4],`${String(u.ti+1).padStart(2,'0')} / ${t.name}`,'dim',256)+txt(L.inset,L.sectionContentY,asset?.name||'NO SAMPLE','dim',432)+txt(480,L.sectionContentY,t.freeze?'X FROZEN':'X FREEZE','dim',144);}
 if(t.instrument===0){html+=keySlots(a,L.voiceArtY);const asset=a.data.assets.find(x=>x.id===t.keys[u.key].asset);html+=txt(L.inset,L.parameterY,asset?`${(asset.frames/asset.rate).toFixed(2)}S / ONE SHOT`:'A / ASSIGN SAMPLE','dim small context-line');}
 else{html+=parameterPages(inst.banks,u.voiceBank,'voice-bank');if(t.instrument===4&&!t.grainAsset)html+=button(144,240,352,40,'ASSIGN SAMPLE / OPTIONS','options','','tab');}
 return html+fieldRows(a)+footer(a,(t.instrument===0&&!t.keys[u.key].asset||t.instrument===4&&!t.grainAsset)?'A ASSIGN SAMPLE':'A AUDITION');
}
function effects(a,master=false){
 const u=a.ui,f=a.effectChain(),top=master?L.sectionContentY:L.contentY;
 if(!u.fxDetail){
  let html=txt(L.inset,top,master?'MASTER EFFECTS':'TRACK EFFECTS','',432)+txt(504,top,`${f.enabled.length}/2 ON`,'dim',120);
  const height=master?L.compactRowHeight:L.rowHeight;
  f.order.forEach((id,i)=>{html+=row(top+L.line+S[16]+i*(height+L.rowGap),`${i+1}  ${a.C.effects[id].name}`,f.enabled.includes(id)?'ON':'BYPASS',u.fxId===id,'fx',id,height);});
  return html+txt(L.inset,L.metaY,'A EDIT / X BYPASS / Y+UP/DOWN ORDER','dim')+footer(a,'A EDIT EFFECT');
 }
 const def=a.C.effects[u.fxId],titleY=top+L.line+S[16];
 return txt(L.inset,top,master?'MASTER FX':a.track().name,'dim')+txt(L.inset,titleY,def.name,'big',240)+button(480,titleY,144,L.tabHeight,f.enabled.includes(u.fxId)?'ON / X':'BYPASS / X','fx-toggle',u.fxId,'dim')+parameterPages(def.banks,u.fxBank,'fx-bank')+fieldRows(a)+footer(a,'B EFFECT CHAIN');
}
function mix(a){
 const u=a.ui;let html=tabStrip(['LEVELS','MASTER FX'],u.mixTab,'mix-tab',[0,1],{kind:'section',hideTurns:u.mixTab===1&&u.fxDetail});if(u.mixTab===1)return html+effects(a,true);
 const tracks=a.sequence().tracks;if(!tracks.length)html+=txt(L.inset,L.sectionContentY,'NO TRACKS / ADD THEM IN GRID','dim');
 tracks.forEach((t,i)=>{const y=L.sectionContentY+i*(L.rowHeight+L.rowGap);html+=row(y,`${String(i+1).padStart(2,'0')} ${t.name}`,t.mute?'MUTED':`${t.level}%`,u.ti===i,'mix-row',i)+box(384,y+L.rowHeight-S[8],228,S[4],`<i style="width:${a.session.playing&&!t.mute?t.level*(.55+.25*Math.sin(a.session.tick*.2+i)):0}%"></i>`,'meter');});
 return html+fieldRows(a)+footer(a,'A MUTE / UNMUTE');
}
function song(a){
 const u=a.ui,s=a.song(),first=Math.floor(u.row/4)*4,titleY=L.headerBottom+S[16],divider=titleY+24+S[8];
 let html=txt(L.inset,titleY,'SONG','big',160)+txt(192,titleY+S[4],`${s.slots.length} SLOTS / ${s.loop?'LOOP':'ONE PASS'}`,'dim',432)+rule(divider);
 for(let i=first;i<=Math.min(first+3,s.slots.length);i++){const slot=s.slots[i],p=slot?a.session.sequence(slot.sequence):null,active=a.session.playing&&a.session.songSlot===i;html+=row(divider+S[16]+(i-first)*(L.rowHeight+L.rowGap),slot?`${active?'>':String(i+1).padStart(2,'0')} ${p?.name||'MISSING SEQUENCE'}`:'+ ADD SEQUENCE',slot?`${slot.bars}B x${slot.repeats}`:'',u.row===i,'song-row',i);}
 return html+txt(L.inset,L.parameterY,a.session.playing?`SLOT ${a.session.songSlot+1} / REPEAT ${a.session.songRepeat+1}`:s.slots[u.row]?'A OPEN / X REPLACE / Y+ARROWS ORDER':'A ADD SEQUENCE','dim small context-line')+fieldRows(a)+footer(a,'A OPEN / ADD','START PLAY SONG');
}
function tape(a){
 const u=a.ui,source=a.source(),names=u.four?['SOURCE','MOVE','TRIM','FINE','LIBRARY']:['SOURCE','EDIT','LIBRARY'];
 const selected=u.tapeTab===0?0:u.tapeTab===2?names.length-1:u.four?1+u.sliceBank:1;
 const values=names.map((_,i)=>{const action=u.four&&i>0&&i<4?'slice-bank':'tape-tab';return{action,value:action==='slice-bank'?i-1:i===names.length-1?2:i};});
 let html=tabStrip(names,selected,'tape-tab',values,{kind:'section'});if(u.assign)html+=txt(L.inset,128,`ASSIGN → ${a.track()?.name.slice(0,18)||'VOICE'} / ${u.assign.grain?'GRANULAR':noteName(48+u.assign.key)}`,'dim small',608);
 if(u.tapeTab===0){
  html+=txt(L.inset,L.sectionContentY,u.capture?'CAPTURING':'RESAMPLE','big')+txt(L.inset,L.sectionContentY+24+S[16],u.capture?`${Math.floor(u.capture.elapsed/1000)} / ${Math.ceil(u.capture.total/1000)} SEC`:'MASTER OR ONE TRACK','dim')+txt(L.inset,L.sectionContentY+24+S[16]+L.line+S[16],'A CAPTURE / X IMPORT WAV','dim');
  if(u.capture)html+=box(L.inset,264,L.content,S[4],`<i style="width:${u.capture.elapsed/u.capture.total*100}%"></i>`,'meter');
  return html+fieldRows(a)+footer(a,u.capture?'B CANCEL CAPTURE':'A CAPTURE');
 }
 if(u.tapeTab===2){
  const rows=a.libraryRows(),first=Math.floor(u.row/5)*5;
  html+=txt(L.inset,L.sectionContentY,u.libFolder?a.data.kits.find(k=>k.id===u.libFolder)?.name||'LIBRARY':`MEMORY / ${a.data.assets.length} SAMPLES`,'dim');
  rows.slice(first,first+5).forEach((r,j)=>html+=row(L.sectionContentY+L.line+S[16]+j*(L.compactRowHeight+L.rowGap),r.label,r.kind==='kit'?'4 SAMPLES /':r.kind==='asset'?`${(r.asset.frames/r.asset.rate).toFixed(2)}S`:'',u.row===first+j,'library-row',first+j,L.compactRowHeight));
  return html+txt(L.inset,L.metaY,`PAGE ${Math.floor(u.row/5)+1}/${Math.ceil(rows.length/5)} / R1+ARROWS`,'dim')+footer(a,u.assign?'A ASSIGN / Y LOAD FOLDER':'A OPEN / X PREVIEW');
 }
 if(!source)return html+txt(L.inset,L.sectionContentY,'IMPORT OR CAPTURE A SOURCE','dim')+footer(a,'B BACK');
 const cut=u.cuts[u.cut],size=cut?(cut.end-cut.start)*source.channels*2/1024:0;
 html+=txt(L.inset,L.sectionContentY,source.name,'',432)+txt(464,L.sectionContentY,`${(source.frames/source.rate).toFixed(1)}S`,'dim',160);
 const partName=u.four?'PART':'SLICE',turn=delta=>`<button tabindex="-1" type="button" class="grid-page-turn" data-action="object-step" data-value="${delta}" aria-label="${delta<0?'Previous':'Next'} ${partName.toLowerCase()}" ${u.cuts.length<2?'disabled':''}><svg viewBox="0 0 8 12" aria-hidden="true"><path d="${delta<0?'M6 2L2 6L6 10':'M2 2L6 6L2 10'}"/></svg></button>`;
 html+=`<div class="part-selector" style="left:${L.inset}px;top:${L.tapeDetailY}px" role="group" aria-label="Selected ${partName.toLowerCase()}">${turn(-1)}<span class="part-count">${partName} ${u.cut+1}/${u.cuts.length}</span>${turn(1)}</div>`
  +txt(208,L.tapeDetailY,`${size.toFixed(0)} / 1024 KIB`,'dim small context-line',208)
  +txt(448,L.tapeDetailY,`L1 + L/R ${partName}`,'dim small context-line',176);

 return html+fieldRows(a)+footer(a,u.four?'Y/B/A/X PREVIEW':'A SAVE / X PREVIEW');
}
function overlay(a){
 const d=a.ui.dialog;if(!d)return'';
 let html=`<div class="overlay" tabindex="-1" role="dialog" aria-modal="true" aria-label="${esc(d.title)}"><div class="title cut">${esc(d.title)}</div>`;
 if(d.type==='name'){
  html+=`<button tabindex="-1" class="name-mode" data-action="name-mode">${d.typing?'BUTTON PANEL':'TYPE ON KEYBOARD'}</button>`;
  const value=esc(d.value);
  html+=d.typing?`<input class="name-input" aria-label="${esc(d.title)}" value="${value}" maxlength="24" autocomplete="off" spellcheck="false">`:`<div class="name-value">${d.all?`<span class="name-selected">${value}</span>`:esc(d.value.slice(0,d.caret))+`<span class="name-caret">${esc(d.value[d.caret]||' ')}</span>`+esc(d.value.slice(d.caret+1))}</div>`;
  html+='<div class="keyboard">'+[...'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -','DEL','OK'].map((c,i)=>`<button tabindex="-1" class="letter ${d.row===i&&!d.typing?'active':''}" data-action="letter" data-value="${i}">${c===' '?'_':esc(c)}</button>`).join('')+'</div>';
  html+=`<p class="description">${d.typing?'ENTER SAVE / ESC BACK TO PANEL':'A INSERT / X DELETE / START SAVE<br>L1/R1 CARET / B CANCEL'}</p>`;
 }else{
  const first=Math.floor(d.row/MENU_PAGE_SIZE)*MENU_PAGE_SIZE;
  html+=`<div class="menu-context"><span class="cut">${esc(d.context||'A SELECT / B BACK')}</span><span>${d.items.length?`${d.row+1}/${d.items.length}`:'0/0'} ${first?'↑':''}${first+MENU_PAGE_SIZE<d.items.length?'↓':''}</span></div>`;
  html+='<div class="menu-items">'+d.items.slice(first,first+MENU_PAGE_SIZE).map((item,j)=>`<button tabindex="-1" class="row ${d.row===first+j?'active':''} ${item.disabled?'dim':''}" data-action="menu-row" data-value="${first+j}" aria-disabled="${!!item.disabled}"><span class="cut">${esc(item.label)}</span></button>`).join('')+'</div>';
  const item=d.items[d.row];html+=`<p class="description">${esc(item?.disabled?item.reason||'UNAVAILABLE':item?.detail||'A SELECT / B BACK')}</p>`;
 }
 return '<div class="modal-backdrop" aria-hidden="true"></div>'+html+'</div>';
}
export function render(a){normalizePatternTab(a);const u=a.ui;let html='';if(u.page==='library')html=library(a);else if(u.page==='song'&&a.song())html=header(a)+song(a);else if(a.sequence()){html=header(a)+major(a);if(u.page==='grid')html+=grid(a);if(u.page==='pattern'&&a.track())html+=pattern(a);if(u.page==='voice'&&a.track())html+=voice(a);if(u.page==='space'&&a.track())html+=effects(a);if(u.page==='mix')html+=mix(a);if(u.page==='tape')html+=tape(a);}else{u.page='library';html=library(a);}html=`<div id="workspace-view" ${u.dialog?'inert aria-hidden="true"':''}>${html}</div>`+overlay(a);document.getElementById('portrait').setAttribute('aria-hidden','true');if(u.toast)html+=`<div class="toast" role="status">${esc(u.toast.text)}</div>`;if(a.renderedHTML!==html){const screen=document.getElementById('screen'),focusInside=a.device.contains(document.activeElement),caret=document.activeElement?.selectionStart;screen.innerHTML=html;a.renderedHTML=html;syncTabStrips(screen);if(u.dialog&&(focusInside||a.focusDialog)){const target=screen.querySelector(u.dialog.typing?'.name-input':'.overlay');target?.focus({preventScroll:true});if(u.dialog.typing&&caret!==null)target?.setSelectionRange(caret??u.dialog.caret,caret??u.dialog.caret);}else if(!u.dialog&&focusInside)a.device.focus({preventScroll:true});a.focusDialog=false;}const active=document.querySelector('#device .overlay .active')||document.querySelector('#device .picked')||document.querySelector('#device .cell.selected')||document.querySelector('#device .row.active')||document.querySelector('#device .key-slot.active')||document.querySelector('#device .nav-tab.active');if(active){active.id='moss-active';a.device.setAttribute('aria-activedescendant','moss-active');}else a.device.removeAttribute('aria-activedescendant');}
