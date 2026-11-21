import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import fs from 'node:fs';
const out=fileURLToPath(new URL('../../../work/interface-v012/spacing-system/',import.meta.url));fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:1000}}),errors=[],reports=[];page.on('pageerror',e=>errors.push(e.message));
try{
 await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await page.evaluate(()=>document.fonts.ready);
 const cases=['library','grid','pattern','sampler-grid','mono','organism','fm','granular','sampler','mix','master-chain',...Array.from({length:5},(_,i)=>`fx-${i}`),...Array.from({length:5},(_,i)=>`master-${i}`),'source','four-cuts','fine','manual-cuts','samples','song','options','name'];
 for(const scene of cases){
  await page.evaluate(async scene=>{
   const a=moss,u=a.ui;u.dialog=null;u.toast=null;u.fxDetail=false;u.pointerParam=null;a.scenario('fm');
   if(scene==='library')a.scenario('atlas');
   else if(scene==='grid')u.page='grid';
   else if(scene==='pattern'){u.page='pattern';u.bank=0;}
   else if(scene==='sampler-grid'){a.scenario('sampler');u.bank=0;}
   else if(['mono','organism','fm','granular','sampler'].includes(scene)){
    if(['fm','granular','sampler'].includes(scene))a.scenario(scene);
    else{const {newTrack}=await import('/src/model.js'),id=scene==='mono'?1:2;a.sequence().tracks[u.ti]=newTrack(a.C,id,scene.toUpperCase());}
    u.page='voice';u.voiceBank=0;
   }else if(scene==='mix'){u.page='mix';u.mixTab=0;}
   else if(scene==='master-chain'){u.page='mix';u.mixTab=1;}
   else if(/^(fx|master)-\d/.test(scene)){u.page=scene.startsWith('master')?'mix':'space';u.mixTab=1;u.fxDetail=true;u.fxId=+scene.at(-1);u.fxBank=0;}
   else if(['source','four-cuts','fine','manual-cuts','samples'].includes(scene)){a.scenario('slicer');u.tapeTab=scene==='source'?0:scene==='samples'?2:1;u.sliceBank=scene==='fine'?2:0;if(scene==='manual-cuts')u.four=false;}
   else if(scene==='song')a.scenario('song');else if(scene==='options')a.options();else if(scene==='name')a.name('SEQUENCE NAME','UNTITLED',()=>{});
   a.render();
  },scene);
  await page.waitForTimeout(70);await page.mouse.move(1100,950);
  const report=await page.evaluate(scene=>{
   const root=document.querySelector('#device'),origin=root.getBoundingClientRect(),scale=origin.width/640;
   const rect=el=>{if(!el)return null;const r=el.getBoundingClientRect();return {x:+((r.left-origin.left)/scale).toFixed(2),y:+((r.top-origin.top)/scale).toFixed(2),w:+(r.width/scale).toFixed(2),h:+(r.height/scale).toFixed(2),bottom:+((r.bottom-origin.top)/scale).toFixed(2),right:+((r.right-origin.left)/scale).toFixed(2)};};
   const all=selector=>[...root.querySelectorAll(selector)].map(el=>({text:el.textContent,action:el.dataset.action,...rect(el)}));
   const fields=all('.field'),labels=all('.field-label'),values=all('.field-value'),icons=all('.face-cluster'),footer=rect(root.querySelector('.footer'));
   const strips=[...root.querySelectorAll('.tab-strip')].map(el=>({kind:[...el.classList].find(c=>c.endsWith('-pages')),box:rect(el),tabs:[...el.querySelectorAll('[role="tab"]')].map(rect)}));
   const cells=all('.cell'),rows=all('.row'),pos=[...root.querySelectorAll('#screen>.pos')].filter(el=>!el.matches('.field')).map(el=>({text:el.textContent.slice(0,70),...rect(el)}));
   const canvas=root.querySelector('canvas'),data=canvas.getContext('2d').getImageData(0,0,640,480).data;let last=-1,first=480;
   for(let y=0;y<480;y++)for(let x=0;x<640;x++)if(data[(y*640+x)*4+3]>8){first=Math.min(first,y);last=Math.max(last,y);}
   const bank=strips.find(s=>s.kind==='parameter-pages'),end=bank?.box.y??fields[0]?.y??445;
   const content=pos.filter(p=>p.y>=95&&p.bottom<=end+.5),lastDom=content.length?Math.max(...content.map(p=>p.bottom)):null;
   return {scene,strips,fields,labels,values,icons,footer,cells,rows,content,
    metrics:{bankToLabels:bank&&labels.length?labels[0].y-bank.box.bottom:null,labelToValue:labels.length?values[0].y-labels[0].bottom:null,valueToFooter:footer&&values.length?footer.y-Math.max(...values.map(v=>v.bottom)):null,visibleFieldsToFooter:footer&&fields.length?footer.y-Math.max(...values.map(v=>v.bottom),...icons.map(v=>v.bottom)):null,bankToFieldBox:bank&&fields.length?fields[0].y-bank.box.bottom:null,contentToBank:bank?bank.box.y-Math.max(lastDom??0,last+1):null,canvasToBank:bank&&last>=0?bank.box.y-last-1:null,lastCanvasPixel:last,firstCanvasPixel:first},
    rowGaps:rows.slice(1).map((row,i)=>+(row.y-rows[i].bottom).toFixed(2)),gridGaps:cells.length>8?{horizontal:cells[1].x-cells[0].right,vertical:cells[8].y-cells[0].bottom}:null,
    overlay:rect(root.querySelector('.overlay')),keyboard:rect(root.querySelector('.keyboard'))};
  },scene);
  reports.push(report);if(['pattern','sampler-grid','fm','mono','organism','granular','sampler','mix','four-cuts','fine','name','options','fx-3','master-3'].includes(scene))await page.locator('#device').screenshot({path:`${out}/${scene}.png`});
 }

 // Fixed control geometry must hold across every screen with parameters.
 const approx=(got,want,context)=>assert.ok(Math.abs(got-want)<.1,`${context}: ${got}, expected ${want}`);
 for(const r of reports){
  if(r.footer)approx(r.footer.y,444,r.scene+' footer');
  if(r.fields.length){
   assert.ok(r.fields.length>0&&r.fields.length<=4,r.scene+' field count');
   r.fields.forEach((f,i)=>{approx(f.x,16+152*i,r.scene+' field x');approx(f.y,376,r.scene+' field y');approx(f.w,152,r.scene+' field width');approx(f.h,44,r.scene+' field height');});
   approx(r.metrics.visibleFieldsToFooter,24,r.scene+' footer gap');
   approx(r.metrics.labelToValue,8,r.scene+' reading gap');
   if(r.metrics.bankToFieldBox!==null){approx(r.metrics.bankToFieldBox,16,r.scene+' page gap');assert.ok(r.metrics.contentToBank>=16,r.scene+' content overlaps pages');}
  }
  if(r.gridGaps){approx(r.gridGaps.horizontal,8,r.scene+' grid horizontal');approx(r.gridGaps.vertical,8,r.scene+' grid vertical');}
  for(const gap of r.rowGaps)approx(gap,r.scene==='options'?4:8,r.scene+' row gap');
  if(r.metrics.lastCanvasPixel>=0)assert.ok(r.metrics.lastCanvasPixel<320,r.scene+' portrait intrudes into control space');
 }
 console.log('PASS 29 layouts: footer anchoring, parameter gaps, lists, grids and portrait bounds');
 // Last track remains visible and clickable after fitting all four mixer rows.
 await page.evaluate(()=>{moss.ui.dialog=null;moss.scenario('fm');moss.ui.page='mix';moss.ui.mixTab=0;moss.render();});
 assert.equal(await page.locator('[data-action="mix-row"]').count(),4);
 await page.locator('[data-action="mix-row"]').last().click();assert.equal(await page.evaluate(()=>moss.ui.ti),3);
 console.log('PASS four visible mixer tracks retain pointer selection');
 const animated=await page.evaluate(async()=>{
  const a=moss,{newTrack}=await import('/src/model.js'),{draw}=await import('/src/visuals.js'),result=[];
  const canvas=document.getElementById('portrait'),ctx=canvas.getContext('2d');
  const bounds=name=>{const rgba=ctx.getImageData(0,0,640,480).data;let first=480,last=-1,left=640,right=-1;for(let y=0;y<480;y++)for(let x=0;x<640;x++)if(rgba[(y*640+x)*4+3]>8){first=Math.min(first,y);last=Math.max(last,y);left=Math.min(left,x);right=Math.max(right,x);}result.push({name,first,last,left,right});};
  for(const id of [0,1,2,3,4]){
   a.scenario(id===0?'sampler':id===4?'granular':'fm');if(id===1||id===2)a.sequence().tracks[a.ui.ti]=newTrack(a.C,id,'TEST');a.ui.page='voice';
   for(const limit of [-1,1]){
    for(let bank=0;bank<a.C.instruments[id].banks.length;bank++){a.ui.voiceBank=bank;a.params().forEach(f=>f.edit(limit,true,100000));}
    // Direct draw times make the extreme/live test deterministic, independent of rAF.
    for(const elapsed of [0,.1,1,7]){const now=performance.now();a.ui.preview={kind:'voice',track:a.track().id,start:now-elapsed*1000};draw(a,now);bounds(`voice-${id}-${limit}-${elapsed}`);}
   }
  }
  a.ui.preview=null;
  for(const master of [false,true])for(let id=0;id<5;id++){
   a.ui.page=master?'mix':'space';a.ui.mixTab=1;a.ui.fxDetail=true;a.ui.fxId=id;a.session.playing=true;a.effectChain().enabled=[id];
   for(const limit of [-1,1]){
    for(let bank=0;bank<a.C.effects[id].banks.length;bank++){a.ui.fxBank=bank;a.params().forEach(f=>f.edit(limit,true,100000));}
    for(const tick of [0,31,127,499]){a.session.tick=tick;draw(a,performance.now());bounds(`${master?'master':'fx'}-${id}-${limit}-${tick}`);}
   }
  }
  a.session.playing=false;a.scenario('fm');a.ui.dialog=null;a.render();return result;
 });
 for(const r of animated){assert.ok(r.last>=0,r.name+' missing portrait');assert.ok(r.last<320&&r.left>=16&&r.right<624,r.name+' crossed portrait bounds');assert.ok(r.first>=176,r.name+' overlapped heading');}
 console.log(`PASS ${animated.length} live/extreme portrait frames stay above controls`);
 // Dialogs have common insets, readable seven-row menus, and 8px keyboard gaps.
 for(const type of ['menu','name']){
  await page.evaluate(type=>{const a=moss;if(type==='name')a.name('SEQUENCE NAME','ABCDEFGHIJKLMNOPQRSTUVWX',()=>{});else a.ui.dialog={type:'menu',title:'OPTIONS',row:0,items:Array.from({length:8},(_,i)=>({label:'OPTION '+(i+1),detail:'A SELECT / B BACK'}))};a.render();},type);
  const d=await page.locator('.overlay').evaluate(el=>{const r=el.getBoundingClientRect(),desc=el.querySelector('.description').getBoundingClientRect();return{left:el.offsetLeft,top:el.offsetTop,padding:getComputedStyle(el).padding,descriptionInside:desc.bottom<=r.bottom-16,keyboardGap:el.querySelector('.keyboard')?getComputedStyle(el.querySelector('.keyboard')).gap:null};});
  assert.equal(d.left,16);assert.equal(d.top,16);assert.equal(d.padding,'16px');assert.ok(d.descriptionInside);if(type==='name')assert.equal(d.keyboardGap,'8px');
 }
 console.log('PASS naming keyboard and paged menus fit common dialog padding');
 const css=await page.evaluate(()=>{
  const found=[];const walk=rules=>{for(const r of rules){if(r.cssRules)walk(r.cssRules);if(r.style)for(const p of r.style)if(/^(margin|padding)(-|$)|^(row-|column-)?gap$/.test(p))found.push({selector:r.selectorText,property:p,value:r.style.getPropertyValue(p)});}};for(const sheet of document.styleSheets)walk(sheet.cssRules);return found;
 });
 await page.evaluate(()=>{moss.ui.dialog=null;moss.scenario('fm');});
 const shell=[];for(const width of [1200,800,390]){
  await page.setViewportSize({width,height:1000});await page.waitForTimeout(70);
  shell.push(await page.evaluate(width=>({width,horizontalOverflow:document.documentElement.scrollWidth>innerWidth,elements:[...document.querySelectorAll('main,.shell-head,.toolbar,.toolbar>div,#viewport,.transport-strip,.lab-tools,.drop-note')].map(el=>{const s=getComputedStyle(el),r=el.getBoundingClientRect();return{class:el.className||el.id||el.tagName,box:{x:r.x,y:r.y,width:r.width,height:r.height},margin:s.margin,padding:s.padding,gap:s.gap};})}),width));
 }
 fs.writeFileSync(out+'/measurements.json',JSON.stringify({reports,css,shell,animated,errors},null,2));
 assert.equal(shell.some(s=>s.horizontalOverflow),false);assert.deepEqual(errors,[]);
 console.log('PASS browser shell at 390 / 800 / 1200 px, zero runtime errors');

}finally{await browser.close();}
