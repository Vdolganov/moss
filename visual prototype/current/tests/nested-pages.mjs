import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
const out=fileURLToPath(new URL('../../../work/interface-v012/navigation-axes/',import.meta.url));fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[],results=[];
page.on('pageerror',e=>errors.push(e.message));
const ev=fn=>page.evaluate(fn),count=()=>page.locator('.nested-count').innerText();
const shot=async name=>{await page.mouse.move(1100,900);await page.locator('#device').screenshot({path:out+'/'+name+'.png'});};
const report=name=>{results.push(name);console.log('PASS '+name);};
const chord=async (key,modifier='q')=>{await page.locator('#device').focus();await page.keyboard.down(modifier);await page.keyboard.press(key);await page.keyboard.up(modifier);};
const rail=()=>page.locator('.nested-rail').evaluate(el=>({index:+el.dataset.index,count:+el.dataset.count,marks:el.querySelectorAll('rect').length,current:el.querySelectorAll('.current').length}));
try{
 await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await ev(()=>document.fonts.ready);
 await ev(()=>{moss.scenario('sampler');moss.ui.key=0;moss.ui.cell=3;moss.ui.bank=2;moss.ui.fxId=0;moss.ui.fxBank=0;const t=moss.track();t.fx.enabled=[3,0];t.fx.order=[0,1,2,3,4];moss.render();});
 const tabNames=()=>page.locator('.parameter-pages .tab-label').allTextContents();
 assert.deepEqual(await tabNames(),['STEP','V-LOCK','ORBIT LOCKS','REEL LOCKS','TIME']);
 assert.equal(await count(),'1/2');assert.deepEqual(await rail(),{index:0,count:2,marks:2,current:1});assert.match(await page.locator('.pattern-context').innerText(),/TRACK \/ ORBIT \/ CORE/);
 const initial=await ev(()=>JSON.stringify(moss.data));
 await chord('ArrowDown');assert.equal(await count(),'2/2');await chord('ArrowDown');assert.equal(await count(),'1/2');await chord('ArrowUp');assert.equal(await count(),'2/2');assert.equal(await ev(()=>moss.ui.fxId),0);
 await page.locator('[data-action="nested-page"][data-value="fx:1"]').click();assert.equal(await count(),'1/2');
 await page.locator('#device').focus();await page.keyboard.press('e');assert.equal(await ev(()=>moss.ui.fxId),3);assert.equal(await count(),'1/4');await page.keyboard.press('e');assert.equal(await ev(()=>moss.ui.bank),3);await page.keyboard.press('e');assert.equal(await ev(()=>moss.ui.bank),0);await page.keyboard.press('q');assert.equal(await ev(()=>moss.ui.bank),3);
 assert.equal(await ev(()=>JSON.stringify(moss.data)),initial);
 report('enabled effects have individual chain-ordered tabs; horizontal tabs and vertical effect pages wrap independently');
 for(const value of ['0','1','fx:0','fx:3','3']){
  await page.evaluate(value=>moss.click('bank',value),value);
  const before=await ev(()=>({cell:moss.ui.cell,key:moss.ui.key,fx:[moss.ui.fxId,moss.ui.fxBank],voice:moss.ui.voiceBank,time:moss.ui.timingBank}));
  await chord('ArrowRight');assert.equal(await ev(()=>moss.ui.key),(before.key+1)%4);assert.equal(await ev(()=>moss.ui.cell),before.cell);assert.deepEqual(await ev(()=>[moss.ui.fxId,moss.ui.fxBank]),before.fx);assert.equal(await ev(()=>moss.ui.timingBank),before.time);
  await chord('ArrowLeft');assert.equal(await ev(()=>moss.ui.key),before.key);
  await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.key),before.key);
 }
 await ev(()=>{moss.click('bank','fx:3');moss.ui.fxBank=1;moss.render();});
 await page.keyboard.down('q');assert.match(await page.locator('.shoulder-hint').innerText(),/SLOT.*PAGE/s);await shot('sampler-held-l1');await page.waitForTimeout(400);await page.keyboard.up('q');assert.equal(await ev(()=>moss.ui.bank),2);assert.equal(await page.locator('.shoulder-hint').count(),0);
 report('horizontal selection chooses sampler slot in every tab; vertical paging never changes lane or cursor');
 await chord('ArrowRight','z');
 const stored=await ev(()=>JSON.stringify(moss.track().steps));assert.ok(await ev(()=>typeof moss.track().steps[moss.ui.cell*4].fx[3][4]==='number'));
 await ev(()=>{moss.track().fx.order=[3,4,0,1,2];moss.render();});assert.equal(await count(),'2/4');assert.deepEqual(await tabNames(),['STEP','V-LOCK','REEL LOCKS','ORBIT LOCKS','TIME']);assert.match(await page.locator('.pattern-context').innerText(),/REEL \/ MOTION/);assert.equal(await ev(()=>JSON.stringify(moss.track().steps)),stored);
 await chord('ArrowRight');assert.equal(await ev(()=>JSON.stringify(moss.track().steps)),stored);assert.match(await page.locator('.pattern-context').innerText(),/TRACK/);
 await ev(()=>{moss.track().fx.enabled=[0];moss.render();});assert.equal(await ev(()=>moss.ui.bank),0);assert.deepEqual(await tabNames(),['STEP','V-LOCK','ORBIT LOCKS','TIME']);assert.equal(await ev(()=>JSON.stringify(moss.track().steps)),stored);
 await ev(()=>{moss.track().fx.enabled=[0,3];moss.render();moss.click('bank','fx:3');});assert.equal(await count(),'2/4');assert.equal(await ev(()=>JSON.stringify(moss.track().steps)),stored);await shot('sampler-effect-locks');
 report('reorder preserves the selected effect; bypass removes its tab without erasing locks; re-enabling restores editing');
 await ev(()=>{moss.track().fx.enabled=[];moss.render();});assert.deepEqual(await tabNames(),['STEP','V-LOCK','TIME']);assert.equal(await page.locator('.parameter-pages [data-value^="fx:"]').count(),0);
 await ev(()=>moss.options());assert.equal(await ev(()=>moss.ui.dialog.items.some(i=>i.label.includes('LOCK BANK'))),false);await page.keyboard.press('Escape');await shot('no-active-effects');
 await page.locator('#device').focus();for(const bank of [1,3,0]){await page.keyboard.press('e');assert.equal(await ev(()=>moss.ui.bank),bank);}
 await ev(()=>{moss.track().fx.enabled=[2];moss.render();moss.click('bank','fx:2');});assert.equal(await page.locator('.nested-count,.nested-rail,.nested-controls').count(),0);await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.fxId),2);
 report('zero effects leaves three base tabs; single-page CRUSH needs no nested indicators');
 await ev(()=>{moss.scenario('fm');moss.track().fx.enabled=[];moss.ui.page='pattern';moss.ui.bank=1;moss.ui.voiceBank=0;moss.render();});assert.equal(await count(),'1/5');await chord('ArrowLeft');assert.equal(await count(),'1/5');await chord('ArrowUp');assert.equal(await count(),'5/5');await chord('ArrowDown');assert.equal(await count(),'1/5');
 await page.locator('[data-action="bank"][data-value="3"]').click();await ev(()=>{moss.ui.timingBank=0;moss.render();});await chord('ArrowRight');assert.equal(await count(),'1/2');await chord('ArrowDown');assert.equal(await count(),'2/2');assert.equal(await page.locator('.field-label').first().innerText(),'SWING');await shot('timing');
 await page.keyboard.press('q');assert.equal(await ev(()=>moss.ui.bank),1);await page.keyboard.press('e');assert.equal(await ev(()=>moss.ui.bank),3);
 await page.keyboard.down('q');await ev(()=>window.dispatchEvent(new Event('blur')));assert.equal(await page.locator('.shoulder-hint').count(),0);await page.keyboard.up('q');
 await page.locator('#device').focus();await page.keyboard.down('q');await page.keyboard.press('e');await page.keyboard.up('q');assert.equal(await page.locator('.overlay').count(),1);assert.equal(await page.locator('.shoulder-hint').count(),0);await page.keyboard.press('Escape');
 report('synth voice locks and TIME use vertical pages only; short tabs, options and blur remain correct');
 await ev(()=>{moss.ui.page='voice';moss.ui.voiceBank=0;moss.render();});const note=await ev(()=>moss.track().audition);await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.voiceBank),1);await chord('ArrowRight');assert.equal(await ev(()=>moss.track().audition),note);await page.keyboard.press('ArrowRight');assert.equal(await ev(()=>moss.track().audition),note+1);assert.equal(await page.locator('.nested-count,.nested-rail').count(),0);
 for(const section of ['space','mix']){await page.evaluate(section=>{moss.ui.page=section;moss.ui.mixTab=1;moss.ui.fxDetail=true;moss.ui.fxId=3;moss.ui.fxBank=0;moss.render();},section);await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.fxBank),1);await chord('ArrowLeft');assert.equal(await ev(()=>moss.ui.fxId),3);assert.equal(await page.locator('.nested-count,.nested-rail').count(),0);}
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.ui.key=0;moss.render();});await chord('ArrowRight');assert.equal(await ev(()=>moss.ui.key),1);await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.key),1);await shot('sampler-voice');
 report('VOICE and FX editors share the axes without false hidden-page indicators; normal arrows still audition notes');
 await ev(()=>{moss.scenario('slicer');moss.ui.sliceBank=0;moss.ui.cut=0;moss.render();});assert.equal(await page.locator('.nested-count,.nested-rail,.section-pages .axis-icon').count(),0);
 for(let bank=0;bank<3;bank++){
  await page.evaluate(bank=>{moss.ui.sliceBank=bank;moss.render();},bank);
  await chord('ArrowRight');assert.equal(await ev(()=>moss.ui.cut),bank+1);assert.equal(await ev(()=>moss.ui.sliceBank),bank);assert.equal(await page.locator('.part-count').innerText(),`PART ${bank+2}/4`);
 }
 await ev(()=>moss.ui.zoom=6);await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.sliceBank),0);assert.equal(await ev(()=>moss.ui.cut),3);await chord('ArrowUp');assert.equal(await ev(()=>moss.ui.sliceBank),2);assert.equal(await ev(()=>moss.ui.zoom),0);assert.equal(await ev(()=>moss.ui.fineCenter),await ev(()=>{const c=moss.ui.cuts[3];return(c.start+c.end)/2;}));
 await page.locator('[data-action="object-step"][data-value="1"]').click();assert.equal(await ev(()=>moss.ui.cut),0);await shot('tape-fine');
 await ev(()=>{moss.ui.four=false;moss.ui.sliceBank=0;moss.ui.cuts=Array.from({length:16},()=>({...moss.ui.cuts[0]}));moss.ui.cut=15;moss.render();});assert.equal(await page.locator('.part-count').innerText(),'SLICE 16/16');await chord('ArrowDown');assert.equal(await ev(()=>moss.ui.cut),15);await chord('ArrowRight');assert.equal(await ev(()=>moss.ui.cut),0);await shot('manual-slices');
 report('TAPE horizontal parts and vertical MOVE/TRIM/FINE stay independent; manual slices have no fake vertical stack');
 await ev(()=>{moss.scenario('sustain');moss.ui.page='pattern';moss.ui.key=0;moss.ui.bank=0;const t=moss.track();moss.data.assets.find(x=>x.id===t.keys[0].asset).name='VERY LONG SAMPLE NAME FOR TRUNCATION';moss.session.advance=()=>{};moss.session.start();moss.session.window(70,moss.sequence());moss.render();});
 assert.equal(await page.locator('.key-slot.playing').count(),2);assert.equal(await page.locator('.key-slot.active.playing').count(),1);
 const slots=await page.locator('.key-slot').evaluateAll(els=>els.map(el=>{const s=getComputedStyle(el),b=getComputedStyle(el.querySelector('.slot-note')),n=el.querySelector('.slot-name');return {radius:s.borderRadius,bg:s.backgroundColor,fg:s.color,badge:b.backgroundColor,badgeText:b.color,field:el.style.getPropertyValue('--field-color'),nowrap:getComputedStyle(n).whiteSpace,ellipsis:getComputedStyle(n).textOverflow,scroll:n.scrollWidth,client:n.clientWidth};}));
 assert.ok(slots.every(s=>s.radius==='12px'&&!s.field&&s.nowrap==='nowrap'&&s.ellipsis==='ellipsis'));assert.ok(slots[0].scroll>slots[0].client);assert.notEqual(slots[0].badge,slots[0].bg);assert.equal(slots[0].bg,slots[0].badgeText);await shot('simultaneous-slots');
 await ev(()=>{moss.session.stop();moss.ui.page='voice';moss.ui.key=0;moss.previewVoice();moss.render();});await chord('ArrowRight');assert.equal(await page.locator('.key-slot.playing').getAttribute('data-value'),'0');assert.equal(await page.locator('.key-slot.active').getAttribute('data-value'),'1');
 report('rounded monochrome slots distinguish selection and simultaneous playback; long names truncate and audition stays on its origin');
 await ev(()=>{moss.ui.preview=null;moss.ui.page='pattern';moss.ui.bank=2;moss.track().fx.enabled=[0,3];moss.ui.fxId=3;moss.ui.fxBank=1;moss.render();});await page.locator('#overflow-toggle').click();await page.waitForTimeout(180);
 assert.ok(await page.locator('.parameter-pages .tab-viewport').evaluate(el=>{const r=el.getBoundingClientRect(),g=el.querySelector('.nested-tab').getBoundingClientRect();return g.left>=r.left+24&&g.right<=r.right-24;}));await shot('overflow');await page.locator('#overflow-toggle').click();
 const geometry=await page.locator('#device').evaluate(root=>{const r=root.querySelector('.nested-rail').getBoundingClientRect(),f=root.querySelector('.parameter-fields').getBoundingClientRect(),tabs=root.querySelector('.parameter-pages').getBoundingClientRect();return {clear:r.right<f.left&&r.top>=f.top&&r.bottom<=f.bottom,tabGap:f.top-tabs.bottom};});assert.ok(geometry.clear);assert.equal(geometry.tabGap,16);
 await page.setViewportSize({width:390,height:844});await page.waitForTimeout(60);await page.screenshot({path:out+'/mobile.png',fullPage:true});assert.equal(await ev(()=>document.documentElement.scrollWidth>innerWidth),false);assert.deepEqual(errors,[]);
 report('overflow keeps active counter and arrows readable; rail spacing and narrow shell fit');
 fs.writeFileSync(out+'/checks.json',JSON.stringify({results,errors,geometry,slots},null,2));
}finally{await browser.close();}
