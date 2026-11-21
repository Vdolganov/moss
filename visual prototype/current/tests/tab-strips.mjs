import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import fs from 'node:fs';
const out=new URL('../../../work/interface-v012/tab-overflow/',import.meta.url);fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[],results=[];page.on('pageerror',e=>errors.push(e.message));
const state=kind=>page.locator(`.${kind}-pages`).evaluate(row=>{
 const el=row.querySelector('.tab-viewport'),active=el.querySelector('[aria-selected="true"]'),r=el.getBoundingClientRect(),a=active.getBoundingClientRect(),scale=r.width/el.clientWidth;
 return{left:el.dataset.overflowLeft==='true',right:el.dataset.overflowRight==='true',position:el.scrollLeft,max:el.scrollWidth-el.clientWidth,visible:a.left>=r.left-.5&&a.right<=r.right+.5,safe:a.left>=r.left+(el.scrollLeft>.5?24*scale:0)-.5&&a.right<=r.right+(el.scrollWidth-el.clientWidth-el.scrollLeft>.5?-24*scale:0)+.5,controls:[...row.querySelectorAll('.page-turn')].map(b=>b.getBoundingClientRect().x)};
});
const settle=()=>page.waitForTimeout(180);
const shot=async name=>{await page.mouse.move(1100,900);await page.locator('#device').screenshot({path:new URL(name+'.png',out).pathname.replace(/^\/([A-Z]:)/,'$1')});};
async function check(name,fn){await fn();results.push(name);console.log('PASS '+name);}
try{
 await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await page.evaluate(()=>document.fonts.ready);
 await page.evaluate(()=>{moss.scenario('fm');moss.ui.page='pattern';moss.ui.bank=0;moss.render();});
 await check('fitting tabs have no fades; fixed shoulder controls work',async()=>{
  for(const kind of ['major','parameter']){const s=await state(kind);assert.equal(s.left,false);assert.equal(s.right,false);assert.ok(s.visible);}
  await page.locator('.major-pages [data-value="R2"]').click();assert.equal(await page.evaluate(()=>moss.ui.page),'voice');
  await shot('normal');
 });
 // Stress the real lists using less available space, without adding fake app pages.
 const dataBeforeToggle=await page.evaluate(()=>JSON.stringify(moss.data));
 await page.locator('#overflow-toggle').click();assert.equal(await page.locator('#overflow-toggle').getAttribute('aria-pressed'),'true');assert.equal(await page.locator('#overflow-note').isVisible(),true);assert.equal(await page.evaluate(()=>JSON.stringify(moss.data)),dataBeforeToggle);
 await page.evaluate(async()=>{const M=await import('/src/model.js');moss.sequence().tracks[moss.ui.ti]=M.newTrack(moss.C,2,'ORGANISM');moss.ui.voiceBank=0;moss.render();});await settle();
 await check('overflow edges follow first, middle and last selections',async()=>{
  let s=await state('parameter');assert.deepEqual([s.left,s.right],[false,true]);assert.ok(s.safe);
  const controls=s.controls;await shot('first');await page.locator('#device').focus();await page.keyboard.press('e');await page.keyboard.press('e');await settle();
  s=await state('parameter');assert.deepEqual([s.left,s.right],[true,true]);assert.ok(s.safe);assert.deepEqual(s.controls,controls);
  const top=await state('major');assert.equal(top.left,top.position>.5);assert.equal(top.right,top.max-top.position>.5);assert.ok(top.safe);await shot('middle');
  await page.keyboard.press('e');await page.keyboard.press('e');await settle();s=await state('parameter');assert.deepEqual([s.left,s.right],[true,false]);assert.ok(s.safe);await shot('last');
  await page.keyboard.press('e');await settle();s=await state('parameter');assert.deepEqual([s.left,s.right],[false,true]);assert.ok(s.safe);
  await page.keyboard.press('q');await settle();s=await state('parameter');assert.deepEqual([s.left,s.right],[true,false]);assert.ok(s.safe);
 });
 await check('screen replacement preserves scroll and fast navigation settles on selection',async()=>{
  const before=await state('parameter');await page.keyboard.down('z');await page.keyboard.press('ArrowRight');await page.keyboard.up('z');await settle();assert.equal((await state('parameter')).position,before.position);
  for(let i=0;i<9;i++)await page.keyboard.press('e');await settle();assert.ok((await state('parameter')).safe);
 });
 await check('clicking a partially visible tab reveals and selects it',async()=>{
  await page.evaluate(()=>{moss.ui.voiceBank=0;moss.render();});await settle();
  const target=await page.locator('.parameter-pages .tab-viewport').evaluate(el=>{
   const v=el.getBoundingClientRect();
   for(const b of el.querySelectorAll('[role="tab"]')){const r=b.getBoundingClientRect();if(r.left<v.right-4&&r.right>v.right)return{x:(r.left+v.right)/2,y:r.top+r.height/2,value:+b.dataset.value};}
  });
  assert.ok(target);await page.mouse.click(target.x,target.y);await settle();assert.equal(await page.evaluate(()=>moss.ui.voiceBank),target.value);assert.ok((await state('parameter')).safe);
 });
 await check('primary tabs reveal both ends and wrap with L2/R2',async()=>{
  await page.keyboard.press(']');await settle();let s=await state('major');assert.deepEqual([s.left,s.right],[true,false]);assert.ok(s.safe);
  await page.keyboard.press(']');await settle();s=await state('major');assert.deepEqual([s.left,s.right],[false,true]);assert.ok(s.safe);
  await page.keyboard.press('[');await settle();s=await state('major');assert.deepEqual([s.left,s.right],[true,false]);assert.ok(s.safe);
 });
 await check('wheel scrolling, resize and reduced motion keep correct edges',async()=>{
  await page.keyboard.press('[');await settle();
  await page.locator('.parameter-pages .tab-viewport').hover();await page.mouse.wheel(0,500);await settle();let s=await state('parameter');assert.equal(s.right,false);
  await page.emulateMedia({reducedMotion:'reduce'});await page.locator('.parameter-pages [data-value="L1"]').click();await page.waitForTimeout(20);assert.ok((await state('parameter')).safe);
  await page.locator('#overflow-toggle').click();assert.equal(await page.locator('#overflow-toggle').getAttribute('aria-pressed'),'false');assert.equal(await page.locator('#overflow-note').isVisible(),false);await settle();for(const kind of ['major','parameter']){s=await state(kind);assert.equal(s.left,false);assert.equal(s.right,false);assert.equal(s.position,0);}
 });
 await check('many-item lists preserve active visibility across replacements',async()=>{
  const report=await page.evaluate(async()=>{
   const {syncTabStrips}=await import('/src/tab-strips.js');const root=document.createElement('div');document.getElementById('device').append(root);
   const source=document.querySelector('.major-pages').cloneNode(true),rail=source.querySelector('.page-options'),template=rail.children[0].cloneNode(true);rail.replaceChildren();
   for(let i=0;i<30;i++){const b=template.cloneNode(true);b.textContent=`PAGE ${i+1}`;b.dataset.value=String(i);rail.append(b);}
   source.querySelector('[data-strip]').dataset.strip='stress-many';
   const checks=[];
   for(const selected of [0,14,29,0]){
    for(const [i,b] of [...rail.children].entries()){b.classList.toggle('active',i===selected);b.setAttribute('aria-selected',String(i===selected));}
    root.innerHTML=source.outerHTML;syncTabStrips(root);
    const v=root.querySelector('[data-strip]'),a=v.querySelector('[aria-selected="true"]'),b=v.getBoundingClientRect(),r=a.getBoundingClientRect();
    checks.push({visible:r.left>=b.left&&r.right<=b.right,left:v.dataset.overflowLeft,right:v.dataset.overflowRight});
   }
   root.remove();return checks;
  });
  assert.ok(report.every(r=>r.visible));assert.deepEqual(report.map(r=>[r.left,r.right]),[['false','true'],['true','true'],['true','false'],['false','true']]);
 });
 await check('library and MIX use the same tabs with context-correct shoulders',async()=>{
  await page.evaluate(()=>moss.scenario('atlas'));await shot('library');
  await page.locator('.library-pages [data-value="R2"]').click();assert.equal(await page.evaluate(()=>moss.ui.atlasTab),1);
  await page.locator('.library-pages [data-value="L2"]').click();assert.equal(await page.evaluate(()=>moss.ui.atlasTab),0);
  await page.evaluate(()=>{moss.scenario('fm');moss.ui.page='mix';moss.ui.mixTab=0;moss.ui.fxDetail=false;moss.render();});await shot('mix');
  await page.locator('.section-pages [data-value="R1"]').click();assert.equal(await page.evaluate(()=>moss.ui.mixTab),1);await shot('master-chain');
  await page.evaluate(()=>{moss.ui.fxId=3;moss.ui.fxBank=0;moss.ui.fxDetail=true;moss.render();});
  assert.equal(await page.locator('.section-pages .page-turn').first().isVisible(),false);
  await page.locator('.parameter-pages [data-value="R1"]').click();assert.deepEqual(await page.evaluate(()=>[moss.ui.mixTab,moss.ui.fxBank]),[1,1]);await shot('master-detail');
  await page.locator('.section-pages [data-action="mix-tab"][data-value="0"]').click();assert.equal(await page.locator('.section-pages .page-turn').first().isVisible(),true);
 });
 await check('TAPE follows every page across mixed action IDs in both cut modes',async()=>{
  await page.evaluate(()=>{moss.scenario('slicer');moss.ui.sliceBank=0;moss.render();});await shot('tape-four');
  await page.locator('#overflow-toggle').click();await page.locator('.section-pages [data-action="tape-tab"][data-value="0"]').click();
  for(const [action,value,label] of [['slice-bank','0','MOVE'],['slice-bank','1','TRIM'],['slice-bank','2','FINE'],['tape-tab','2','LIBRARY'],['tape-tab','0','SOURCE']]){
   await page.locator('.section-pages [data-value="R1"]').click();await settle();const selected=page.locator('.section-pages [aria-selected="true"]');assert.equal(await selected.getAttribute('data-action'),action);assert.equal(await selected.getAttribute('data-value'),value);assert.ok((await state('section')).safe,label);if(label==='FINE')await shot('tape-overflow');
  }
  await page.evaluate(()=>{moss.ui.four=false;moss.render();});await page.locator('.section-pages [data-value="R1"]').click();assert.equal(await page.locator('.section-pages [aria-selected="true"] .tab-label').innerText(),'EDIT');await shot('tape-manual');
  const styles=await page.locator('.nav-tab[aria-selected="true"]').evaluateAll(tabs=>tabs.map(t=>{const s=getComputedStyle(t);return[s.borderRadius,s.backgroundColor,s.color,s.borderBottomWidth];}));
  assert.ok(styles.every(s=>JSON.stringify(s)===JSON.stringify(['12px','rgb(232, 232, 230)','rgb(25, 26, 28)','0px'])));
  await page.locator('#overflow-toggle').click();
 });
 await page.setViewportSize({width:390,height:844});await settle();await page.screenshot({path:new URL('mobile.png',out).pathname.replace(/^\/([A-Z]:)/,'$1'),fullPage:true});
 assert.deepEqual(errors,[]);fs.writeFileSync(new URL('checks.json',out),JSON.stringify({results,errors},null,2));
}finally{await browser.close();}
