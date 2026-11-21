import {LiveSession, BUTTONS, NAMES, noteName, mod, segments, pitchRows, livePage} from './live-model.js';

const $ = selector => document.querySelector(selector);
const storageKey = 'moss-live-design-1';
let session = new LiveSession(), variant = 'score', restored = false, gridSize = 64;
try {
  const saved = JSON.parse(localStorage.getItem(storageKey));
  if (saved?.schema === 'moss-live-study-1' && saved.tracks?.length === 4) {
    session = new LiveSession(saved); restored = true;
  }
} catch { /* A missing or inaccessible draft does not prevent playing. */ }
if (!restored) { session.seed(); session.history = []; }
const descriptions = {
  console: 'Крупная схема кнопок — больше внимания рукам и исполнению.',
  score: 'Восемь нотных строк — точнее видны длительности и живые смещения.',
  orbit: 'Длительности показаны дугами внутри цикла; схема кнопок остаётся по бокам.',
};
const device = $('#live-device'), screen = $('#live-screen');
let lastTime = performance.now(), lastPaint = 0, messageUntil = 0, message = '', shoulderChord = false;
const down = new Set(), pointers = new Map();
const consumedShoulders = new Set();
const physical = {ArrowLeft:'LEFT', ArrowDown:'DOWN', ArrowRight:'RIGHT', ArrowUp:'UP',
  KeyZ:'Y', KeyX:'B', KeyC:'A', KeyV:'X', KeyQ:'L1', KeyE:'R1', BracketLeft:'L2', BracketRight:'R2',
  Tab:'SELECT', Space:'START'};
const escape = value => String(value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&apos;'}[c]));
const text = (x,y,label,cls='',anchor='start') => `<text x="${x}" y="${y}" class="${cls}" text-anchor="${anchor}">${escape(label)}</text>`;
const line = (x1,y1,x2,y2,cls='svg-line') => `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" class="${cls}"/>`;
const rect = (x,y,w,h,cls,rx=0) => `<rect x="${x}" y="${y}" width="${Math.max(0,w)}" height="${h}" rx="${rx}" class="${cls}"/>`;
const buttonLabel = b => ({LEFT:'L',DOWN:'D',RIGHT:'R',UP:'U'}[b] || b);
const padNote = i => session.held.get(BUTTONS[i])?.note ?? session.notes[i];
function arrow(x,y,b) {
  const rotation = {RIGHT:0,DOWN:90,LEFT:180,UP:270}[b];
  return `<path d="M -4 -4 L 0 0 L -4 4 M -7 0 H 0" transform="translate(${x+2} ${y}) rotate(${rotation})" fill="none" stroke="currentColor" stroke-width="1.5"/>`;
}
function pads(cx,cy,r=27,spacing=59) {
  const notes = session.notes;
  const offset = [[-spacing,0],[0,spacing],[spacing,0],[0,-spacing]];
  return [0,1].map(hand => {
    const x = hand ? cx[1] : cx[0];
    return line(x-spacing,cy,x+spacing,cy) + line(x,cy-spacing,x,cy+spacing) + offset.map(([dx,dy],j) => {
      const i = hand*4+j,b = BUTTONS[i],pressed = session.held.has(b), disabled = i >= notes.length;
      return `<g data-pad="${b}" class="${pressed?'pressed':''} ${disabled?'disabled':''}" style="color:${pressed?'var(--bg)':'var(--text)'}">
        <circle cx="${x+dx}" cy="${cy+dy}" r="${r}" class="pad-shape"/>
        ${hand ? text(x+dx,cy+dy-5,b,'svg-small','middle') : arrow(x+dx,cy+dy-9,b)}
        ${text(x+dx,cy+dy+12,disabled?'--':noteName(padNote(i)),'','middle')}</g>`;
    }).join('');
  }).join('');
}
function hardwarePads(cx,cy) {
  const notes=session.notes, left=cx[0], right=cx[1];
  // A single cross silhouette, rather than four circular buttons, matches the D-pad.
  let html=`<path d="M -22 -68 H 22 V -22 H 68 V 22 H 22 V 68 H -22 V 22 H -68 V -22 H -22 Z"
    transform="translate(${left} ${cy})" fill="var(--panel)" stroke="var(--dim)" stroke-width="1" stroke-linejoin="round"/>`;
  const arms=[[-68,-22,46,44,-46,0],[-22,22,44,46,0,46],[22,-22,46,44,46,0],[-22,-68,44,46,0,-46]];
  arms.forEach(([x,y,w,h,dx,dy],i)=>{
    const b=BUTTONS[i],pressed=session.held.has(b);
    html+=`<g data-pad="${b}" class="${pressed?'pressed':''}" style="color:${pressed?'var(--bg)':'var(--text)'}">
      <rect x="${left+x}" y="${cy+y}" width="${w}" height="${h}" rx="3" fill="${pressed?'var(--text)':'transparent'}"/>
      ${arrow(left+dx,cy+dy-10,b)}${text(left+dx,cy+dy+14,noteName(padNote(i)),pressed?'svg-dark':'','middle')}</g>`;
  });
  html+=`<circle cx="${left}" cy="${cy}" r="5" class="svg-line"/>`;
  [[-44,0],[0,44],[44,0],[0,-44]].forEach(([dx,dy],j)=>{
    const i=j+4,b=BUTTONS[i],pressed=session.held.has(b),disabled=i>=notes.length;
    html+=`<g data-pad="${b}" class="${pressed?'pressed':''} ${disabled?'disabled':''}">
      <circle cx="${right+dx}" cy="${cy+dy}" r="25" class="pad-shape"/>
      ${text(right+dx,cy+dy-7,b,'svg-small','middle')}${text(right+dx,cy+dy+14,disabled?'--':noteName(padNote(i)),'','middle')}</g>`;
  });
  return html;
}
function header() {
  const d=session.data, rec=d.recording, mode=rec?'REC':d.playing?'PLAY':'LIVE';
  let html = rect(0,0,640,480,'svg-dark') + text(16,30,d.name) + text(624,30,`${d.bpm} BPM`,'','end') + line(0,48,640,48);
  const total=d.tracks.length, width=112, left=(640-total*width)/2;
  html+=text(16,77,'L2','svg-small')+text(624,77,'R2','svg-small','end');
  d.tracks.forEach((t,i)=>{const active=i===d.track,x=left+i*width;
    if(active)html+=rect(x,58,width-8,26,'svg-paper',13);
    html+=text(x+(width-8)/2,76,t.instrument,`${active?'svg-dark':'svg-dim'} svg-small`,'middle');
  });
  html+=text(16,112,`${NAMES[d.root]} ${d.scale} / OCT ${d.octave}`,'svg-small svg-dim');
  html+=`<g data-command="metronome">`+rect(408,96,112,24,d.metronome?'svg-panel':'svg-dark',12)+
    text(420,112,'MET','svg-small '+(d.metronome?'':'svg-dim'));
  if(d.metronome)for(let beat=0;beat<4;beat++){
    const active=d.playing&&session.beat===beat,size=beat===0?6:4;
    html+=`<rect data-beat="${beat}" data-active="${active}" x="${470+beat*12}" y="${108-size/2}" width="${size}" height="${size}" class="${active?'svg-paper':'svg-line'}"/>`;
  }else html+=text(507,112,'OFF','svg-small svg-dim','end');
  html+='</g>';
  html+=rect(530,96,94,24,rec?'svg-paper':'svg-panel',12)+text(577,112,mode,`svg-small ${rec?'svg-dark':''}`,'middle');
  if(rec)html+=`<circle cx="542" cy="108" r="3" class="svg-dark"/>`;
  return html;
}
function footer() {
  if(down.has('SELECT'))return line(0,444,640,444)+text(16,466,'START RECORD','svg-small')+text(320,466,'R1 METRONOME','svg-small','middle')+text(624,466,'SELECT HELD','svg-small svg-dim','end');
  return line(0,444,640,444)+text(16,466,'SEL+START REC','svg-small')+text(320,466,'L1 / R1 OCT','svg-small svg-dim','middle')+text(624,466,'MENU EXIT','svg-small svg-dim','end');
}
function grid(y=344) {
  const cycle=session.track.cycle,head=session.head,view=livePage(cycle,head,gridSize),page=view.index,pageStart=view.start;
  const count=view.count, events=session.events(), compact=count>16, columns=compact?16:8, gap=compact?4:8;
  const cellWidth=(608-(columns-1)*gap)/columns,cellHeight=compact?14:32,rowHeight=compact?18:40;
  let html=text(16,y-13,`PATTERN  ${Math.floor(head/4)+1}/${Math.ceil(cycle/4)}`,'svg-small svg-dim')+
    text(624,y-13,`${page+1}/${view.total}  AUTO`,'svg-small svg-dim','end');
  for(let i=0;i<count;i++) {
    const x=16+(i%columns)*(cellWidth+gap),top=y+Math.floor(i/columns)*rowHeight,at=pageStart+i*4;
    const notes=events.filter(e=>e.at>=at&&e.at<at+4),playing=session.data.playing&&head>=at&&head<at+4;
    html+=rect(x,top,cellWidth,cellHeight,notes.length?'svg-panel':'svg-dark',compact?2:4)+rect(x,top,cellWidth,cellHeight,'svg-line',compact?2:4);
    if(compact){
      if(playing)html+=rect(x+1,top+1,cellWidth-2,cellHeight-2,'svg-paper',2);
      else if(notes.length)html+=rect(x+cellWidth/2-1,top+3,2,8,'svg-paper');
      for(const e of events)for(const part of segments(e,cycle)) {
        const start=Math.max(at,part.start),end=Math.min(at+4,part.start+part.length);
        if(end>start)html+=line(x+3+(start-at)/4*(cellWidth-6),top+10,x+3+(end-at)/4*(cellWidth-6),top+10,playing?'svg-inverted-ink':'svg-ink');
      }
      continue;
    }
    if(playing)html+=rect(x+4,top+3,18,12,'svg-paper',2);
    html+=text(x+6,top+14,String(pageStart/4+i+1).padStart(2,'0'),`svg-small ${playing?'svg-dark':'svg-dim'}`);
    if(notes.length)html+=text(x+cellWidth-6,top+14,noteName(notes.at(-1).note),'svg-small','end');
    for(const e of events)for(const part of segments(e,cycle)) {
      const start=Math.max(at,part.start),end=Math.min(at+4,part.start+part.length);
      if(end>start)html+=line(x+5+(start-at)/4*(cellWidth-10),top+23,x+5+(end-at)/4*(cellWidth-10),top+23,'svg-ink');
    }
  }
  return html;
}
function score() {
  const events=session.events(),cycle=session.track.cycle,left=64,width=560,top=136,row=18;
  const rows=pitchRows(events,session.notes,session.pitchFocus), view=livePage(cycle,session.head,gridSize);
  let html='';
  if(session.data.playing)html+=rect(left+Math.floor((session.head-view.start)/4)*4/view.ticks*width,top,4/view.ticks*width,144,'svg-panel');
  for(let i=0;i<rows.notes.length;i++) {
    const y=top+i*row;
    html+=`<g data-pitch-row="${rows.notes[i]}">${text(16,y+13,noteName(rows.notes[i]),'svg-small svg-dim')+line(left,y+row,width+left,y+row)}</g>`;
  }
  for(let i=0;i<=view.count;i++){
    const x=left+Math.min(i*4,view.ticks)/view.ticks*width;
    html+=`<g data-grid-column="${i}">${line(x,top,x,top+144,i%4?'svg-minor':'svg-line')}</g>`;
    if(i<view.count && i%(view.count>32?8:4)===0)html+=text(x+2,top-5,String(view.start/4+i+1),'svg-small svg-dim');
  }
  for(const e of events)for(const part of segments(e,cycle)) {
    const index=rows.notes.indexOf(e.note),start=Math.max(view.start,part.start),end=Math.min(view.start+view.ticks,part.start+part.length);
    if(index<0||end<=start)continue;
    const y=top+index*row+5,x=left+(start-view.start)/view.ticks*width,w=(end-start)/view.ticks*width;
    html+=`<g data-event-note="${e.note}" data-row-note="${rows.notes[index]}">`+rect(x,y,w,10,e.pending?'svg-paper':'svg-panel',4);
    html+=line(x,y+10,x+w,y+10,'svg-ink');
    if(!part.continuation&&start===part.start)html+=rect(x,y,2,10,'svg-paper');
    html+='</g>';
  }
  if(session.data.playing)html+=line(left+(session.head-view.start)/view.ticks*width,top-4,left+(session.head-view.start)/view.ticks*width,top+144,'svg-ink');
  html+=text(16,302,`STEP ${String(Math.floor(session.head/4)+1).padStart(2,'0')} / ${Math.ceil(cycle/4)}`,'svg-small svg-dim')+
    text(624,302,`${view.index+1}/${view.total} AUTO`,'svg-small svg-dim','end');
  if(rows.high||rows.low)html+=text(320,302,`HIGH ${rows.high} / LOW ${rows.low}`,'svg-small svg-dim','middle');
  return html+hardwarePads([184,456],366);
}
function polar(radius,tick) {
  const a=tick/session.track.cycle*Math.PI*2-Math.PI/2;
  return [320+Math.cos(a)*radius,228+Math.sin(a)*radius];
}
function arc(radius,start,length) {
  const [x,y]=polar(radius,start),[endX,endY]=polar(radius,start+length);
  return `<path d="M ${x} ${y} A ${radius} ${radius} 0 ${length>session.track.cycle/2?1:0} 1 ${endX} ${endY}" class="svg-ink"/>`;
}
function orbit() {
  const cycle=session.track.cycle;let html='';
  for(let i=0;i<16;i++){const a=polar(88,i*cycle/16),b=polar(i%4?92:96,i*cycle/16);html+=line(...a,...b);}
  html+=`<circle cx="320" cy="228" r="84" class="svg-line"/>`;
  for(const e of session.events()) for(const part of segments(e,cycle)) {
    const radius=82-e.degree*4;
    // Two arcs handle an exactly full-cycle note without a degenerate SVG arc.
    if(part.length>=cycle)html+=arc(radius,0,cycle/2)+arc(radius,cycle/2,cycle/2);
    else html+=arc(radius,part.start,part.length);
    if(!part.continuation){const p=polar(radius,part.start);html+=rect(p[0]-1,p[1]-1,3,3,'svg-paper');}
  }
  const a=polar(86,session.head),b=polar(99,session.head);html+=line(...a,...b,'svg-ink');
  html+=text(320,226,String(Math.floor(session.head/4)+1).padStart(2,'0'),'svg-title','middle')+
    text(320,247,`/ ${Math.ceil(cycle/4)}`,'svg-small svg-dim','middle');
  return html+pads([108,532],228,23,51)+grid();
}
function consoleView() {
  const held=[...session.held.values()].at(-1);
  return pads([184,456],229)+text(320,228,held?noteName(held.note):'LIVE','svg-title','middle')+
    text(320,250,held?'HELD':'READY','svg-small svg-dim','middle')+grid();
}
function syncTime() {const now=performance.now();session.advance((now-lastTime)/1000);lastTime=now;}
function save() {
  try{localStorage.setItem(storageKey,JSON.stringify(session.save()));}
  catch{notify('Не удалось сохранить эскиз в браузере. Используй экспорт JSON.');}
}
function notify(value) {message=value;messageUntil=performance.now()+3500;}
function paint() {
  screen.innerHTML=header()+(variant==='console'?consoleView():variant==='score'?score():orbit())+footer();
  $('#record').setAttribute('aria-pressed',String(session.data.recording));
  $('#record').textContent=session.data.recording?'Остановить запись':'Запись';
  $('#play').textContent=session.data.playing?'Стоп':'Старт';
  $('#undo').disabled=session.data.recording||!session.history.length;
  $('#long-example').disabled=session.data.recording;
  $('#metronome').setAttribute('aria-pressed',String(session.data.metronome));
  $('#metronome').textContent=`Метроном: ${session.data.metronome?'вкл':'выкл'}`;
  for(const el of document.querySelectorAll('#setup select,#setup input,#setup button:not(#export)'))el.disabled=session.data.recording;
  if(session.notice){notify(session.notice);session.notice='';}
  const status=performance.now()<messageUntil?message:session.data.recording?'Запись · живой тайминг':session.data.playing?'Цикл играет · можно записывать':'Играй свободно или запусти цикл';
  if($('#status').textContent!==status)$('#status').textContent=status;
}
function action(control,isDown) {
  syncTime();
  if(isDown){
    if(down.has(control))return;down.add(control);
    if((control==='R1'||control==='SELECT')&&down.has('R1')&&down.has('SELECT')&&!consumedShoulders.has('R1')){
      consumedShoulders.add('R1');session.toggleMetronome();save();
    }
    else if(BUTTONS.includes(control))session.press(control);
    else if(control==='START'){if(down.has('SELECT'))session.toggleRecord();else session.togglePlay();save();}
    else if(control==='L2'||control==='R2'){session.changeTrack(control==='L2'?-1:1);syncSettings();save();}
    else if((control==='L1'||control==='R1')&&down.has('L1')&&down.has('R1')&&!down.has('SELECT')&&!consumedShoulders.size){shoulderChord=true;session.undo();save();}
  } else {
    if(!down.has(control))return;down.delete(control);
    if(BUTTONS.includes(control)){session.release(control);save();}
    if(control==='L1'||control==='R1'){
      if(!shoulderChord&&!consumedShoulders.has(control)){session.changeOctave(control==='L1'?-1:1);save();}
      consumedShoulders.delete(control);
      if(!down.has('L1')&&!down.has('R1'))shoulderChord=false;
    }
  }
  paint();
}
device.addEventListener('keydown',e=>{
  if(e.shiftKey&&e.code==='Tab')return;
  if(e.ctrlKey||e.metaKey||e.altKey)return;
  if(e.code==='Escape'){e.preventDefault();session.loseFocus();save();location.href='index.html';return;}
  if(physical[e.code]){e.preventDefault();action(physical[e.code],true);}
});
window.addEventListener('keyup',e=>{if(physical[e.code]&&down.has(physical[e.code])){e.preventDefault();action(physical[e.code],false);}});
function releaseInput() {syncTime();session.loseFocus();down.clear();pointers.clear();consumedShoulders.clear();shoulderChord=false;save();paint();}
window.addEventListener('blur',releaseInput);
document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseInput();});
device.addEventListener('focusout',()=>{session.releaseAll();down.clear();consumedShoulders.clear();shoulderChord=false;save();});
device.addEventListener('pointerdown',e=>{
  device.focus();if(e.target.closest('[data-command="metronome"]')){e.preventDefault();session.toggleMetronome();save();paint();return;}
  const pad=e.target.closest('[data-pad]');if(!pad)return;
  e.preventDefault();pointers.set(e.pointerId,pad.dataset.pad);action(pad.dataset.pad,true);device.setPointerCapture(e.pointerId);
});
for(const name of ['pointerup','pointercancel'])window.addEventListener(name,e=>{const b=pointers.get(e.pointerId);if(b){action(b,false);pointers.delete(e.pointerId);}});
document.querySelectorAll('[data-control]').forEach(button=>button.addEventListener('click',()=>{
  action(button.dataset.control,true);action(button.dataset.control,false);
}));
// Wrapper transport/view controls must not release a physical key held on the device.
document.querySelectorAll('[data-control],[data-variant],[data-grid-size],#record,#play,#metronome').forEach(button=>{
  button.addEventListener('pointerdown',e=>{if(document.activeElement===device)e.preventDefault();});
});
document.querySelectorAll('[data-variant]').forEach(button=>button.addEventListener('click',()=>{
  variant=button.dataset.variant;document.querySelectorAll('[data-variant]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));
  $('#variant-description').textContent=descriptions[variant];paint();
}));
document.querySelectorAll('[data-grid-size]').forEach(button=>button.addEventListener('click',()=>{
  gridSize=Number(button.dataset.gridSize);
  document.querySelectorAll('[data-grid-size]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));
  paint();
}));
$('#long-example').onclick=()=>{
  if(session.data.recording)return;
  session.snapshot();session.track.cycle=304;session.track.events=[];session.seed();
  // One undo entry for replacing the example, including its cycle.
  session.history.splice(-1,1);session.data.position=0;save();syncSettings();paint();
  notify('Пример: 76 шагов, страницы 64 + 12. Можно отменить.');
};
$('#play').onclick=()=>{syncTime();session.togglePlay();save();paint();};
$('#record').onclick=()=>{syncTime();session.toggleRecord();save();paint();};
$('#metronome').onclick=()=>{session.toggleMetronome();save();paint();};
$('#undo').onclick=()=>{session.undo();save();paint();};
$('#root').innerHTML=NAMES.map((n,i)=>`<option value="${i}">${n}</option>`).join('');
function syncSettings(){for(const name of ['root','scale','bpm'])$('#'+name).value=session.data[name];$('#length').value=session.track.cycle;}
for(const name of ['root','scale','bpm'])$('#'+name).onchange=e=>{
  session.releaseAll();const value=e.target.value;
  session.data[name]=name==='scale'?value:name==='bpm'?Math.max(30,Math.min(240,Number(value)||84)):Number(value);
  syncSettings();save();paint();
};
$('#length').onchange=e=>{
  session.snapshot();session.track.cycle=Number(e.target.value);
  session.track.events.forEach(n=>{n.at=mod(n.at,session.track.cycle);n.rawAt=mod(n.rawAt,session.track.cycle);n.duration=Math.min(n.duration,session.track.cycle);});
  save();paint();
};
$('#quantize').onclick=()=>{session.quantize(Number($('#quantum').value),Number($('#strength').value));save();paint();};
$('#seed').onclick=()=>{session.seed();save();paint();};
$('#clear').onclick=()=>{session.snapshot();session.track.events=[];save();paint();};
$('#export').onclick=()=>{
  const url=URL.createObjectURL(new Blob([JSON.stringify(session.save(),null,2)],{type:'application/json'}));
  const a=document.createElement('a');a.href=url;a.download='moss-live-study.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};
new ResizeObserver(()=>{
  const scale=Math.min(1,($('#viewport').parentElement.clientWidth-32)/640);
  $('#viewport').style.width=`${640*scale}px`;$('#viewport').style.height=`${480*scale}px`;device.style.transform=`scale(${scale})`;
}).observe(document.querySelector('main'));
syncSettings();paint();
function frame(now){syncTime();if(now-lastPaint>32){paint();lastPaint=now;}requestAnimationFrame(frame);}
requestAnimationFrame(frame);
// Read-only-by-convention inspection surface for reproducible UI tests.
window.mossLive={get session(){return session;},get variant(){return variant;},paint};
