import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
const out=fileURLToPath(new URL('../../../work/interface-v012/grid-pages/',import.meta.url));fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[],results=[];
page.on('pageerror',e=>errors.push(e.message));
const ev=fn=>page.evaluate(fn),cell=i=>page.locator(`.cell[data-value="${i}"]`);
const report=name=>{results.push(name);console.log('PASS '+name);};
const shot=async name=>{await page.mouse.move(1100,900);await page.locator('#device').screenshot({path:out+'/'+name+'.png'});};
const size=n=>page.locator(`[data-action="grid-size"][data-value="${n}"]`).click();
const count=()=>page.locator('.grid-page-count').innerText();
try{
 await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await ev(()=>document.fonts.ready);assert.equal(await ev(()=>moss.ui.gridSize),64);
 await ev(()=>{moss.session.advance=()=>{};});await page.locator('.lab-tools').evaluate(el=>el.open=true);
 const original=await ev(()=>JSON.stringify(moss.data.sequences));
 for(const n of [64,128,512]){
  await page.locator(`[data-scenario="grid-${n}"]`).click();assert.equal(await page.locator('.cell').count(),64);assert.equal(await count(),`1/${n/64}`);
  await ev(async()=>{const M=await import('/src/model.js');M.validateData(moss.C,moss.data);});await shot('study-'+n);
 }
 assert.equal(await ev(()=>JSON.stringify(moss.data.sequences.filter(p=>!/^GRID (64|128|512)$/.test(p.name)))),original);
 assert.equal(await page.locator('[data-action="grid-view"],.density-mini,.density-map').count(),0);
 report('64/128/512 studies are preserved; only 16/64 page modes remain');
 const data=await ev(()=>JSON.stringify(moss.data));
 await cell(63).click();await size(16);assert.equal(await ev(()=>moss.ui.cell),63);assert.equal(await page.locator('.cell').count(),16);assert.equal(await count(),'4/32');
 await size(64);assert.equal(await count(),'1/8');assert.equal(await ev(()=>JSON.stringify(moss.data)),data);
 await page.locator('.grid-page-turn[data-value="1"]').click();assert.equal(await ev(()=>moss.ui.cell),127);assert.equal(await count(),'2/8');
 await page.keyboard.down('e');await page.keyboard.press('ArrowRight');await page.keyboard.up('e');assert.equal(await ev(()=>moss.ui.cell),191);assert.equal(await count(),'3/8');
 await size(16);assert.equal(await count(),'12/32');await page.keyboard.down('e');await page.keyboard.press('ArrowLeft');await page.keyboard.up('e');assert.equal(await ev(()=>moss.ui.cell),175);
 report('mode switching preserves cursor/data; pointer and E+arrows jump by current page size');
 await size(64);await page.keyboard.press('ArrowUp');assert.equal(await ev(()=>moss.ui.cell),159);await page.keyboard.press('ArrowDown');assert.equal(await ev(()=>moss.ui.cell),175);
 await ev(()=>{moss.ui.cell=480;moss.render();});await page.keyboard.down('e');await page.keyboard.press('ArrowRight');await page.keyboard.up('e');assert.equal(await ev(()=>moss.ui.cell),480);
 await ev(()=>{moss.ui.cell=511;moss.render();});assert.equal(await count(),'8/8');assert.equal(await page.locator('.grid-page-turn[data-value="1"]').isDisabled(),true);
 await page.keyboard.press('ArrowRight');assert.equal(await ev(()=>moss.ui.cell),511);await page.keyboard.press('c');assert.equal(await ev(()=>moss.track().steps[511].type),1);
 const velocity=await ev(()=>moss.track().steps[511].velocity);await page.keyboard.down('x');await page.keyboard.press('ArrowLeft');await page.keyboard.up('x');assert.equal(await ev(()=>moss.track().steps[511].velocity),velocity-1);
 await ev(()=>{moss.session.playing=true;moss.session.tick=500;moss.render();});assert.equal(await page.locator('.cell[aria-current="step"]').getAttribute('data-value'),'500');assert.equal(await cell(511).evaluate(e=>e.classList.contains('selected')),true);await shot('512-last-page');await ev(()=>moss.session.stop());
 for(const bank of [0,1,2,3]){await page.evaluate(bank=>{moss.ui.bank=bank;moss.render();},bank);assert.ok(await page.locator('.pattern-context').evaluate(e=>e.scrollWidth<=e.clientWidth));}
 report('compact 16-column navigation, step 512 editing, playback and context fit');
 await page.keyboard.down('q');await page.keyboard.down('e');await page.keyboard.up('e');await page.keyboard.up('q');assert.equal(await ev(()=>moss.ui.dialog.items[0].label),'GRID VIEW');await page.keyboard.press('c');assert.equal(await ev(()=>moss.ui.gridSize),16);assert.equal(await count(),'32/32');await shot('512-size-16');
 await size(64);await ev(()=>{moss.ui.bank=3;moss.render();});await page.keyboard.down('x');await page.keyboard.press('ArrowLeft');await page.keyboard.up('x');assert.equal(await ev(()=>moss.track().division),32);assert.equal(await count(),'4/4');assert.equal(await ev(()=>moss.ui.cell),255);
 await page.keyboard.down('z');await page.keyboard.press('ArrowLeft');await page.keyboard.up('z');assert.equal(await ev(()=>moss.ui.cell),254);assert.equal(await page.locator('.cell').count(),63);
 report('keyboard menu, division changes and shorter cycles recalculate pagination');
 const bounds=[];
 for(const n of [1,15,17,32,63,64,65,128,129,511,512])for(const gridSize of [16,64]){
  bounds.push(await page.evaluate(async({n,gridSize})=>{
   const M=await import('/src/model.js');moss.track().division=64;M.setCycle(moss.track(),n);moss.ui.cell=n-1;moss.setGridSize(gridSize);
   const root=document.querySelector('#device').getBoundingClientRect(),cells=[...document.querySelectorAll('.cell')];
   return {n,gridSize,actual:cells.length,first:+cells[0].dataset.value,selected:+document.querySelector('.cell.selected').dataset.value,page:document.querySelector('.grid-page-count').textContent,
    inside:cells.every(e=>{const r=e.getBoundingClientRect();return r.left>=root.left+15.9&&r.right<=root.left+624.1&&r.top>=root.top+136&&r.bottom<=root.top+288;}),
    captionsFit:[...document.querySelectorAll('.compact-text')].every(e=>e.scrollWidth<=e.clientWidth)};
  },{n,gridSize}));
 }
 for(const b of bounds){assert.equal(b.actual,(b.n-1)%b.gridSize+1);assert.equal(b.first,Math.floor((b.n-1)/b.gridSize)*b.gridSize);assert.equal(b.page,`${Math.ceil(b.n/b.gridSize)}/${Math.ceil(b.n/b.gridSize)}`);assert.ok(b.inside&&b.captionsFit,JSON.stringify(b));assert.equal(b.selected,b.n-1);}
 report('partial final pages retain readable geometry and have no padded interactive cells');
 await ev(async()=>{const M=await import('/src/model.js');const t=moss.track();t.cycle=128;t.division=64;t.steps={60:{...M.emptyStep(),type:1,note:48,length:16},64:{...M.emptyStep(),type:2,locks:{1:{0:1800}}}};moss.ui.cell=60;moss.ui.bank=0;moss.setGridSize(64);});
 assert.equal(await page.locator('.duration-out').count(),1);await page.locator('.grid-page-turn[data-value="1"]').click();assert.equal(await count(),'2/2');assert.equal(await page.locator('.duration-in').count(),1);assert.equal(await page.locator('.duration-end').count(),1);assert.equal(await cell(64).locator('.cell-lock').count(),1);await shot('64-carry');
 await ev(()=>{moss.ui.cell=64;moss.setGridSize(16);});assert.equal(await cell(64).locator('.carry-label').innerText(),'C3');await size(64);await ev(()=>moss.options());const go=await ev(()=>moss.ui.dialog.items.findIndex(x=>x.label==='GO TO NOTE'));await page.locator(`[data-action="menu-row"][data-value="${go}"]`).click();assert.equal(await ev(()=>moss.ui.cell),60);assert.equal(await count(),'1/2');
 report('sustain continues across 64-step pages, 16-step capsules and GO TO NOTE remain correct');
 await ev(()=>{moss.scenario('sustain');moss.track().division=64;moss.track().cycle=512;moss.setGridSize(64);moss.session.start();moss.session.window(70,moss.sequence());moss.ui.cell=70;moss.render();});
 assert.equal(await page.locator('.key-slot.playing').count(),2);await page.locator('[data-action="slot"][data-value="1"]').click();assert.equal(await ev(()=>moss.ui.cell),70);assert.equal(await count(),'2/8');assert.equal(await page.locator('.key-slot.playing').count(),2);await shot('sampler-64');
 await size(16);assert.equal(await count(),'5/32');await size(64);await page.setViewportSize({width:390,height:844});await page.waitForTimeout(60);await page.screenshot({path:out+'/mobile.png',fullPage:true});assert.equal(await ev(()=>document.documentElement.scrollWidth>innerWidth),false);assert.deepEqual(errors,[]);
 report('sampler slot/voice state survives pagination and the narrow shell fits');
 fs.writeFileSync(out+'/checks.json',JSON.stringify({results,errors,bounds},null,2));
}finally{await browser.close();}
