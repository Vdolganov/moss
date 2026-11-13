/* Browser-only visual study. The diagrams model controls, not recorded audio. */
(()=>{
'use strict';
const G=GrainStudy,$=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const KEY='moss.granular-study.v1',PHYSICAL=['Y','B','A','X'],CODES=['KeyZ','KeyX','KeyC','KeyV'];
const DESCRIPTIONS={
 splice:['Срез','Сэмпл как партитура. Восемь линий показывают окна зёрен, а вертикальная головка — место, откуда берётся материал. Самый точный и спокойный вид.'],
 field:['Поле','Зёрна рождаются над сэмплом и растворяются по мере жизни. Наклон показывает высоту, разлёт — ширину облака. Самый живой и воздушный вид.'],
 lens:['Линза','Сэмпл замыкается в окружность. Кольца раскрываются по мере жизни зёрен, короткие дуги отмечают чтение. Самый сосредоточенный, почти гипнотический вид.']
};
let stored=null,storageOk=true;try{stored=JSON.parse(localStorage.getItem(KEY)||'null');}catch{storageOk=false;}
let state=G.restore(stored),playing=!stored,gesture=null,modal=null,returnFocus=null,history=[],held=new Set(),shoulderUsed=false,pointerEdit=null;
const requested=new URLSearchParams(location.search).get('view');if(DESCRIPTIONS[requested])state.variant=requested;
const reduced=matchMedia('(prefers-reduced-motion: reduce)');if(reduced.matches)state.static=true;
const source=()=>G.SOURCES.find(s=>s.id===state.source)||null;
const cloud=new G.Cloud(state.params,source());cloud.freeze=state.freeze;cloud.prime();
const screen=$('#grain-screen'),canvas=$('#grain-canvas'),ctx=canvas.getContext('2d');
const css=getComputedStyle(screen),col={bg:css.getPropertyValue('--bg').trim(),line:css.getPropertyValue('--line').trim(),dim:css.getPropertyValue('--dim').trim(),text:css.getPropertyValue('--text').trim()},faces=['--face-y','--face-b','--face-a','--face-x'].map(k=>css.getPropertyValue(k).trim());
let last=0,paintTime=0,reveal=1;
const snapshot=()=>JSON.stringify(state);
function remember(before){if(before!==snapshot()){history.push(before);if(history.length>40)history.shift();}$('#undo-study').disabled=!history.length;}
function save(){try{localStorage.setItem(KEY,snapshot());storageOk=true;}catch{storageOk=false;}$('#study-save').textContent=storageOk?'Настройки сохранены':'Только этот сеанс';if(!storageOk)message('Браузер не разрешил сохранение. Настройки работают до закрытия страницы.');}
function message(text){$('#study-message').textContent=text;}
function configure(){cloud.configure(state.params,source(),state.freeze);if(!playing||state.static)cloud.prime();}
function value(key){const n=key==='position'&&!gesture&&!pointerEdit?cloud.head*100:state.params[key];return key==='speed'?n.toFixed(2):['position','start','end'].includes(key)?n.toFixed(1):String(Math.round(n));}
function editSnapshot(key){if(key==='position'){state.params.position=Math.round(cloud.head*1000)/10;cloud.params.position=state.params.position;}return snapshot();}
function renderFields(){const focused=document.activeElement?.dataset?.parameter;$('#grain-params').innerHTML=G.BANKS[state.bank].map((key,i)=>`<button class="grain-param" data-slot="${i}" data-parameter="${key}" aria-label="${key.toUpperCase()}, ${value(key)} ${G.SPEC[key][4]}. Удерживайте ${['Z','X','C','V'][i]} и нажимайте стрелки"><span class="label"><b class="physical">${PHYSICAL[i]}</b>${key==='irregularity'?'IRREGULAR':key.toUpperCase()}</span><span class="value">${value(key)}<span class="unit">${G.SPEC[key][4]}</span></span></button>`).join('');if(focused)$(`[data-parameter="${focused}"]`)?.focus({preventScroll:true});markGesture();}
function markGesture(){const key=gesture?.key||pointerEdit?.key;$$('.grain-param').forEach(el=>el.classList.toggle('editing',el.dataset.parameter===key));$('#grain-hint').textContent=key?'← → FINE · ↑ ↓ COARSE · ESC CANCEL':source()?'A FREEZE · HOLD + ARROWS EDIT':'A ASSIGN SAMPLE';}
function render(){
 screen.dataset.playing=String(playing&&!state.static&&!!source());
 $('#transport-label').textContent=!source()?'STOP':state.static?'STILL':playing?'PLAY':'PAUSE';
 $('#transport').setAttribute('aria-label',playing?'Приостановить движение':'Запустить движение');
 $('#transport').disabled=!source();$('#motion-toggle').disabled=!source();
 $('#motion-toggle').textContent=state.static?'Новый снимок / Space':playing?'Пауза / Space':'Продолжить / Space';
 $('#static-mode').checked=state.static;
 $('#source-button').textContent=source()?`${source().name} / ${source().seconds.toFixed(1)} s`:'+ ASSIGN SAMPLE';
 $('#freeze').setAttribute('aria-pressed',String(state.freeze));$('#freeze').textContent=state.freeze?'A · FROZEN':'A · FREEZE';$('#freeze').hidden=!source();
 $('#empty-source').hidden=!!source();canvas.hidden=!source();
 $$('[data-variant]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.variant===state.variant)));
 $$('[data-bank]').forEach(b=>b.setAttribute('aria-current',String(+b.dataset.bank===state.bank)));
 $('#variant-title').textContent=DESCRIPTIONS[state.variant][0];$('#variant-copy').textContent=DESCRIPTIONS[state.variant][1];
 canvas.setAttribute('aria-label',`${DESCRIPTIONS[state.variant][0]}. ${state.freeze?'Позиция заморожена. ':''}До восьми зёрен. ${['Скорость, высота, размер, плотность','Позиция, границы сканирования, разброс','Стереоразлёт, нерегулярность, фильтр, проявление'][state.bank]}.`);
 renderFields();$('#undo-study').disabled=!history.length;draw();
}
function finishGesture(cancel=false){if(!gesture&&!pointerEdit)return;if(gesture){if(cancel)state=G.restore(JSON.parse(gesture.before));else remember(gesture.before);gesture=null;}if(pointerEdit){if(cancel)state=G.restore(JSON.parse(pointerEdit.before));else remember(pointerEdit.before);pointerEdit=null;}configure();markGesture();save();}
function change(fn){finishGesture();const before=snapshot();fn();remember(before);configure();save();render();}
function bank(index){finishGesture();state.bank=(index+3)%3;save();render();}
function undo(){finishGesture();if(!history.length)return;state=G.restore(JSON.parse(history.pop()));configure();save();render();message('Изменение отменено.');}
function toggleFreeze(){if(!source())return;change(()=>state.freeze=!state.freeze);message(state.freeze?'Головка заморожена. Зёрна продолжают рождаться.':'Сканирование продолжается.');}
function togglePlay(){if(!source())return;finishGesture();if(state.static){for(let i=0;i<12;i++)cloud.advance(.025);draw();message('Новый неподвижный снимок облака.');return;}playing=!playing;if(playing)reveal=0;last=0;render();}
function dialogIsolation(on){$$('.study-header,.variant-tabs,.study-guide,.study-transport,#grain-screen > :not(#grain-overlay)').forEach(el=>el.inert=on);}
function openDialog(kind){finishGesture();if(!modal)returnFocus=document.activeElement;modal=kind;const overlay=$('#grain-overlay');overlay.hidden=false;dialogIsolation(true);let rows='';
 if(kind==='source')rows=G.SOURCES.map(s=>`<button class="menu-item" data-source="${s.id}"><span>${s.name}</span><small>${s.seconds.toFixed(1)} s · ${s.mib.toFixed(2)} MiB${s.id===state.source?' · CURRENT':''}</small></button>`).join('');
 else rows='<button class="menu-item" data-menu="source">ASSIGN / REPLACE SAMPLE</button><button class="menu-item" data-menu="remove" '+(!source()?'disabled':'')+'>REMOVE ASSIGNMENT</button><button class="menu-item" data-menu="reset">RESET PARAMETERS</button>';
 overlay.innerHTML=`<section class="grain-dialog" role="dialog" aria-modal="true" aria-label="${kind==='source'?'Демосэмплы':'Опции инструмента'}"><header><h3>${kind==='source'?'SAMPLE LIBRARY':'GRANULAR / OPTIONS'}</h3><button data-close>ESC / CLOSE</button></header><p>${kind==='source'?'Подготовленные демосэмплы. Выбор меняет схему волны; звук не воспроизводится.':'Действия применяются к одному облаку. Изменения можно отменить через Ctrl+Z.'}</p>${rows}<span class="dialog-note">${kind==='source'?'ONE SAMPLE · EIGHT GRAINS':'Y / B / A / X · FOUR PARAMETERS'}</span></section>`;
 overlay.querySelector('.menu-item:not(:disabled)')?.focus();
}
function closeDialog(){modal=null;$('#grain-overlay').hidden=true;$('#grain-overlay').innerHTML='';dialogIsolation(false);if(returnFocus?.isConnected)returnFocus.focus({preventScroll:true});else screen.focus({preventScroll:true});}
$('#grain-overlay').addEventListener('click',e=>{const b=e.target.closest('button');if(!b)return;if(b.hasAttribute('data-close'))return closeDialog();if(b.dataset.source){const id=b.dataset.source;closeDialog();change(()=>state.source=id);message('Назначен '+source().name+'.');return;}if(b.dataset.menu==='source')return openDialog('source');const action=b.dataset.menu;closeDialog();change(()=>{if(action==='remove'){state.source=null;state.freeze=false;playing=false;}else {state.params={...G.DEFAULTS};state.freeze=false;}});message(action==='remove'?'Назначение снято. Ctrl+Z вернёт сэмпл.':'Параметры сброшены.');});
$$('[data-variant]').forEach(b=>b.addEventListener('click',()=>{finishGesture();state.variant=b.dataset.variant;save();render();screen.focus({preventScroll:true});}));
$$('[data-bank]').forEach(b=>b.addEventListener('click',()=>bank(+b.dataset.bank)));
$('#source-button').onclick=()=>openDialog('source');$('#assign-empty').onclick=()=>openDialog('source');$('#options').onclick=()=>openDialog('options');
$('#freeze').onclick=toggleFreeze;$('#transport').onclick=togglePlay;$('#motion-toggle').onclick=()=>{togglePlay();screen.focus({preventScroll:true});};$('#undo-study').onclick=undo;
$('#static-mode').onchange=e=>{finishGesture();state.static=e.target.checked;if(state.static)cloud.prime();last=0;save();render();};
$$('[data-preset]').forEach(b=>b.onclick=()=>{change(()=>{state.params=b.dataset.preset==='mist'?{...G.DEFAULTS,size:440,density:14,speed:.12,pitch:-12}:{...G.DEFAULTS,size:65,density:12,speed:.8,pitch:7,jitter:420,spread:90};state.freeze=false;if(!source())state.source='air';});cloud.prime();playing=true;reveal=0;render();screen.focus({preventScroll:true});message('Настройки изменены. Все три вида показывают одно облако.');});
document.addEventListener('keydown',e=>{
 if(e.metaKey||e.ctrlKey){if(e.code==='KeyZ'){e.preventDefault();if(modal)closeDialog();undo();}return;}
 if(modal){if(e.code==='Escape'||e.code==='KeyX'){e.preventDefault();closeDialog();return;}const items=[...$('#grain-overlay').querySelectorAll('button:not(:disabled)')],at=items.indexOf(document.activeElement);if(e.code==='Tab'){e.preventDefault();items[(at+(e.shiftKey?-1:1)+items.length)%items.length].focus();}else if(['ArrowDown','ArrowUp','KeyQ','KeyE'].includes(e.code)){e.preventDefault();items[(at+(['ArrowUp','KeyQ'].includes(e.code)?-1:1)+items.length)%items.length].focus();}else if(e.code==='KeyC'){e.preventDefault();document.activeElement?.click();}return;}
 if(e.target.matches('input,select,textarea'))return;
 if(e.code==='Escape'){e.preventDefault();finishGesture(true);held.clear();render();return;}
 if(e.code==='KeyQ'||e.code==='KeyE'){e.preventDefault();if(e.repeat)return;held.add(e.code);if(held.has('KeyQ')&&held.has('KeyE')){shoulderUsed=true;openDialog('options');}return;}
 const slot=CODES.indexOf(e.code);
 if(slot>=0){e.preventDefault();if(!e.repeat&&!gesture){finishGesture();const key=G.BANKS[state.bank][slot];gesture={code:e.code,key,before:editSnapshot(key),used:false,start:performance.now()};held.add(e.code);markGesture();}return;}
 if(e.code==='Space'&&(e.target===document.body||screen.contains(e.target))){e.preventDefault();if(!e.repeat)togglePlay();return;}
 if(e.code.startsWith('Arrow')){const field=e.target.closest('[data-parameter]'),key=gesture?.key||field?.dataset.parameter;if(!key)return;e.preventDefault();if(!gesture&&!pointerEdit)pointerEdit={key,before:editSnapshot(key),start:performance.now()};if(gesture)gesture.used=true;const age=performance.now()-(gesture||pointerEdit).start,mult=e.repeat?age>1600?5:age>700?2:1:1;state.params=G.edit(state.params,key,['ArrowRight','ArrowUp'].includes(e.code)?1:-1,['ArrowUp','ArrowDown'].includes(e.code),mult);configure();renderFields();draw();}
});
document.addEventListener('keyup',e=>{
 if(e.code==='KeyQ'||e.code==='KeyE'){const was=held.has(e.code);held.delete(e.code);if(!modal&&was&&!shoulderUsed)bank(state.bank+(e.code==='KeyQ'?-1:1));if(!held.has('KeyQ')&&!held.has('KeyE'))shoulderUsed=false;return;}
 if(gesture?.code===e.code){const action=e.code==='KeyC'&&!gesture.used;finishGesture();held.delete(e.code);if(action){if(source())toggleFreeze();else openDialog('source');}render();}
 if(e.code.startsWith('Arrow')&&pointerEdit){finishGesture();render();}
});
window.addEventListener('blur',()=>{finishGesture();held.clear();shoulderUsed=false;last=0;});
document.addEventListener('visibilitychange',()=>{last=0;if(document.hidden){finishGesture();held.clear();}});
reduced.addEventListener('change',e=>{if(e.matches){state.static=true;configure();save();render();}});
new ResizeObserver(()=>{const scale=Math.min(1,$('#grain-wrap').clientWidth/640);screen.style.transform=`scale(${scale})`;$('#grain-wrap').style.height=(480*scale)+'px';}).observe($('#grain-wrap'));

function line(x1,y1,x2,y2,color=col.line,width=1,alpha=1){ctx.globalAlpha=alpha;ctx.strokeStyle=color;ctx.lineWidth=width;ctx.beginPath();ctx.moveTo(x1,y1);ctx.lineTo(x2,y2);ctx.stroke();ctx.globalAlpha=1;}
function label(text,x,y,color=col.dim,align='left'){ctx.fillStyle=color;ctx.font="11px Consolas, 'Courier New', monospace";ctx.textAlign=align;ctx.globalAlpha=1;ctx.fillText(text,x,y);}
function waveLine(y,amplitude){ctx.beginPath();for(let i=0;i<=552;i++){const x=i/552,h=G.wave(x,source().seed,state.params.cutoff)*amplitude;i?ctx.lineTo(26+i,y+h):ctx.moveTo(26+i,y+h);}ctx.strokeStyle=col.dim;ctx.globalAlpha=.65;ctx.lineWidth=1;ctx.stroke();ctx.globalAlpha=1;}
function accent(key){const active=gesture?.key||pointerEdit?.key;return active===key?faces[G.BANKS[state.bank].indexOf(key)]:col.text;}
function grainAlpha(g){return (.15+.85*Math.sin(Math.PI*g.age/g.life)**2)*(state.static?1:reveal);}
function sourceAxis(y){const p=state.params,head=26+cloud.head*552;line(26+p.start/100*552,y,26+p.end/100*552,y,col.dim);for(const x of [26+p.start/100*552,26+p.end/100*552])line(x,y-3,x,y+3,col.dim);line(head,y-8,head,y+3,accent('position'),1.5);label('0',26,187);label(source().seconds.toFixed(1)+' s',578,187,col.dim,'right');}
function drawSplice(){
 const p=state.params,head=26+cloud.head*552;
 for(let i=0;i<8;i++)line(26,26+i*14,578,26+i*14,col.line,.5,.35);
 waveLine(154,15);sourceAxis(172);line(head,20,head,172,accent('position'),1,.6);
 for(const g of cloud.grains){const y=26+g.slot*14,len=g.life*g.rate/source().seconds*552,start=g.birth*552,read=G.wrap(g.birth+g.age*g.rate/source().seconds)*552,alpha=grainAlpha(g);const tone=accent('size');for(const offset of [0,-552]){const x=Math.max(0,start+offset),end=Math.min(552,start+offset+len);if(end>=0&&x<end){ctx.fillStyle=tone;ctx.globalAlpha=alpha*.2;ctx.fillRect(26+x,y-3,end-x,6);ctx.globalAlpha=1;line(26+x,y,26+end,y,tone,2,alpha);line(26+x,y-3,26+x,y+3,tone,1,alpha);}}line(26+read,y-4,26+read,y+4,accent('pitch'),1,alpha);}
}
function drawField(){
 waveLine(157,10);sourceAxis(172);
 const p=state.params;
 for(const g of cloud.grains){const age=g.age/g.life,origin=26+g.birth*552,x=G.clamp(origin+g.pan*age*130,48,556),y=144-age*111,alpha=grainAlpha(g),half=8+g.life*32,tilt=g.pitch/24*.6;
 line(origin,150,x,y,col.line,.7,alpha*.55);line(x-half,y+half*tilt,x+half,y-half*tilt,accent('size'),1.5,alpha);
 line(x,y-2.5,x,y+2.5,accent('pitch'),1,alpha);ctx.fillStyle=col.text;ctx.globalAlpha=alpha;ctx.fillRect(Math.round(x)-1,Math.round(y)-1,2,2);ctx.globalAlpha=1;
 }
 const head=26+cloud.head*552;line(head,143,head,167,accent('position'),1.5);label('BIRTH',26,141);label('LIFE',26,27);
 if(p.density===0)label('DENSITY 0 / NO NEW GRAINS',302,88,col.dim,'center');
}
function point(turn,r){const a=turn*Math.PI*2-Math.PI/2;return [302+Math.cos(a)*r,98+Math.sin(a)*r];}
function arc(start,end,r,color,width=1,alpha=1){ctx.beginPath();ctx.arc(302,98,r,start*Math.PI*2-Math.PI/2,end*Math.PI*2-Math.PI/2);ctx.strokeStyle=color;ctx.lineWidth=width;ctx.globalAlpha=alpha;ctx.stroke();ctx.globalAlpha=1;}
function drawLens(){
 const p=state.params;arc(0,1,70,col.line);arc(p.start/100,p.end/100,78,col.dim,1);
 ctx.beginPath();for(let i=0;i<=220;i++){const t=i/220,r=70+G.wave(t,source().seed,p.cutoff)*5,[x,y]=point(t,r);i?ctx.lineTo(x,y):ctx.moveTo(x,y);}ctx.strokeStyle=col.dim;ctx.globalAlpha=.65;ctx.lineWidth=1;ctx.stroke();ctx.globalAlpha=1;
 for(let i=0;i<16;i++){const a=point(i/16,82),b=point(i/16,i%4===0?87:84);line(...a,...b,col.line);}
 for(const g of cloud.grains){const read=G.wrap(g.birth+g.age*g.rate/source().seconds),length=Math.min(.9,g.life*g.rate/source().seconds),r=14+g.age/g.life*50,alpha=grainAlpha(g);arc(0,1,r,col.dim,.7,alpha*.32);arc(read-length*g.age/g.life,read+length*(1-g.age/g.life),r,accent('size'),1.5,alpha);const a=point(read,r-2),b=point(read,r+2);line(...a,...b,accent('pitch'),1,alpha);}
 const a=point(cloud.head,68),b=point(cloud.head,89);line(...a,...b,accent('position'),1.5);for(const turn of [p.start/100,p.end/100])line(...point(turn,75),...point(turn,82),col.dim);
 label('SOURCE',26,41);label(source().seconds.toFixed(1)+' s',26,57,col.text);label('SCAN',578,133,col.dim,'right');label((cloud.head*100).toFixed(1)+' %',578,149,col.text,'right');
}
function draw(){ctx.clearRect(0,0,604,196);if(!source())return;({splice:drawSplice,field:drawField,lens:drawLens})[state.variant]();label(state.freeze?'POSITION HELD':'SCAN '+(state.params.speed<0?'REVERSE':'FORWARD'),26,11);label((cloud.head*source().seconds).toFixed(2)+' s',578,11,col.dim,'right');if(state.bank===2){label('LP '+state.params.cutoff+' Hz',26,187);label('FADE '+state.params.fade+' ms',578,187,col.dim,'right');}}
function frame(now){const elapsed=last?Math.min((now-last)/1000,.05):0;last=now;if(!document.hidden&&playing&&!state.static&&source()&&!modal){cloud.advance(elapsed);reveal=Math.min(1,reveal+elapsed/Math.max(.002,state.params.fade/1000));if(now-paintTime>=32){draw();paintTime=now;if(state.bank===1&&!gesture&&!pointerEdit){const el=$('[data-parameter="position"]');el.querySelector('.value').firstChild.nodeValue=value('position');el.setAttribute('aria-label',`POSITION, ${value('position')} %. Удерживайте Z и нажимайте стрелки`);}}}requestAnimationFrame(frame);}
render();save();screen.focus({preventScroll:true});requestAnimationFrame(frame);
})();
