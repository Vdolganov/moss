import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
const out=fileURLToPath(new URL('../../../work/interface-v012/grid-cells/',import.meta.url));fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
const cell=i=>page.locator(`.cell[data-value="${i}"]`);
const ev=fn=>page.evaluate(fn);
const current=()=>page.locator('.cell[aria-current="step"]').getAttribute('data-value');
const inspect=i=>cell(i).evaluate(el=>({caption:el.querySelector('.event').textContent,diamond:!!el.querySelector('.cell-lock'),on:el.classList.contains('on'),lock:el.classList.contains('lock'),label:el.getAttribute('aria-label'),fill:getComputedStyle(el).backgroundColor,radius:getComputedStyle(el).borderRadius,border:getComputedStyle(el).borderColor,outline:getComputedStyle(el).outlineStyle,indexFill:getComputedStyle(el.querySelector('.cell-index')).backgroundColor,indexInk:getComputedStyle(el.querySelector('.cell-index')).color}));
async function fixture(sampler=false){await page.evaluate(async sampler=>{
 const a=moss,M=await import('/src/model.js');a.scenario(sampler?'sampler':'fm');if(!sampler)a.sequence().tracks[a.ui.ti]=M.newTrack(a.C,1,'CELL STUDY');const t=a.track();t.cycle=64;t.division=16;t.steps={};t.hits=Array.from({length:4},()=>({}));
 a.ui.page='pattern';a.ui.gridSize=16;a.ui.cell=2;a.ui.key=0;a.ui.bank=0;a.ui.dialog=null;a.ui.toast=null;a.ui.pointerParam=null;a.session.stop();
 // Keep screenshots and phase-boundary assertions independent of wall-clock timing.
 a.session.advance=()=>{};
 const note=(at,n,locks={})=>{if(sampler)t.hits[0][at]={...M.emptyHit(),type:1,locks};else t.steps[at]={...M.emptyStep(),type:1,note:n,locks:{1:locks}};};
 note(0,48);note(16,52);note(32,55,{0:M.RESET});note(44,54);
 if(sampler)t.hits[0][8]={...M.emptyHit(),type:2,locks:{0:-12}};else t.steps[8]={...M.emptyStep(),type:2,locks:{1:{0:400}}};
 t.steps[16]={...(t.steps[16]||M.emptyStep()),type:sampler?2:1,fx:{3:{4:15}}};
 t.steps[24]={...M.emptyStep(),type:2,shift:0};t.steps[56]={...M.emptyStep(),type:2,repeat:0};
 a.session.tick=32;a.session.playing=true;a.render();
 },sampler);await page.mouse.move(1100,900);}
try{
 await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await ev(()=>document.fonts.ready);await fixture();
 const empty=await inspect(1),note=await inspect(0),lock=await inspect(2),both=await inspect(4),playing=await inspect(8);
 assert.equal(empty.caption,'');assert.equal(empty.diamond,false);assert.equal(note.caption,'C3');assert.equal(note.diamond,false);assert.equal(lock.caption,'LOCK');assert.equal(lock.diamond,true);assert.equal(lock.on,false);assert.notEqual(note.fill,lock.fill);assert.equal(both.caption,'E3');assert.equal(both.on,true);assert.equal(both.lock,false);assert.equal(both.diamond,true);
 for(const i of [0,1,2,4,8])assert.equal((await inspect(i)).radius,'4px');
 assert.equal(lock.border,'rgb(232, 232, 230)');assert.equal(lock.outline,'none');assert.equal(playing.indexFill,'rgb(232, 232, 230)');assert.equal(playing.indexInk,'rgb(25, 26, 28)');assert.equal(playing.fill,note.fill);assert.equal(await current(),'8');
 assert.equal(await page.locator('.cell .bar').count(),0);assert.equal(await cell(8).evaluate(el=>getComputedStyle(el,':before').content),'none');
 await page.locator('#device').screenshot({path:out+'/pattern.png'});
 console.log('PASS empty / note / lock / note with locks have independent content, selection and playhead cues');
 for(const i of [6,8,14])assert.equal((await inspect(i)).diamond,true);
 await ev(()=>{const t=moss.track();t.steps[0].locks={1:{0:null}};t.steps[0].fx={1:{0:null}};t.steps[4]={type:0,locks:{1:{0:100}},fx:{}};moss.render();});assert.equal((await inspect(0)).diamond,false);assert.equal((await inspect(1)).diamond,false);
 console.log('PASS timing zero, explicit OFF and RESET show locks; null and disabled events do not');
 await fixture();await ev(()=>{moss.ui.cell=8;moss.render();});let active=await inspect(8);assert.equal(active.border,lock.border);assert.equal(active.indexFill,playing.indexFill);assert.equal(active.diamond,true);assert.match(active.label,/SELECTED.*PLAYING/);
 await ev(()=>{moss.session.stop();moss.render();});assert.equal(await page.locator('.cell[aria-current]').count(),0);assert.equal((await inspect(8)).diamond,true);assert.equal((await inspect(8)).border,lock.border);
 console.log('PASS selected playing note retains its locks and cursor after transport stops');
 await fixture(true);assert.equal((await inspect(0)).caption,'HIT');assert.equal((await inspect(2)).caption,'LOCK');assert.equal((await inspect(4)).caption,'HIT');assert.equal((await inspect(4)).lock,false);assert.equal((await inspect(4)).diamond,true);
 await page.locator('#device').screenshot({path:out+'/sampler.png'});
 await ev(()=>{moss.ui.key=1;moss.render();});assert.equal((await inspect(0)).caption,'');assert.equal((await inspect(2)).caption,'');assert.equal((await inspect(4)).caption,'LOCK');assert.equal((await inspect(4)).on,false);assert.equal((await inspect(6)).caption,'LOCK');
 console.log('PASS sampler lane locks stay local; shared FX and timing locks remain visible across lanes');
 await fixture();await ev(()=>{moss.track().cycle=60;moss.session.tick=59.9;moss.render();});assert.equal(await current(),'14');await ev(()=>{moss.session.tick=60;moss.render();});assert.equal(await current(),'0');
 await ev(()=>{moss.track().cycle=128;moss.ui.cell=16;moss.session.tick=63.9;moss.render();});assert.equal(await page.locator('.cell[aria-current]').count(),0);await ev(()=>{moss.session.tick=64;moss.render();});assert.equal(await current(),'16');
 console.log('PASS playhead follows a 15-step cycle and page boundaries');
 await fixture();await ev(async()=>{const M=await import('/src/model.js');moss.track().steps[1]={...M.emptyStep(),type:1};moss.render();});assert.equal(await cell(0).locator('.has-substeps').count(),1);
 await ev(()=>{const t=moss.track();t.division=64;t.cycle=512;moss.ui.cell=511;t.steps[511]={...t.steps[0],type:1,note:61,locks:{1:{0:100}}};moss.session.tick=511;moss.render();});
 const bounds=await cell(511).evaluate(el=>{const r=el.getBoundingClientRect(),n=el.querySelector('.cell-index').getBoundingClientRect(),d=el.querySelector('.cell-lock').getBoundingClientRect(),v=el.querySelector('.event').getBoundingClientRect();return{number:el.querySelector('.cell-index').textContent,separate:n.right<d.left,inside:n.left>=r.left&&d.right<=r.right&&v.bottom<=r.bottom};});assert.equal(bounds.number,'512');assert.ok(bounds.separate&&bounds.inside);assert.equal((await inspect(511)).caption,'C#4');
 await page.locator('#device').screenshot({path:out+'/last-page.png'});
 console.log('PASS finer-event hint and three-digit step numbers fit beside lock glyphs');
 await fixture();await cell(2).locator('.cell-lock').click();assert.equal(await ev(()=>moss.track().steps[8].type),1);assert.equal((await inspect(2)).diamond,true);assert.equal((await inspect(2)).on,true);
 await page.locator('#device').focus();await page.keyboard.press('ArrowRight');assert.equal(await ev(()=>moss.ui.cell),3);await page.keyboard.press('c');assert.equal((await inspect(3)).on,true);
 await fixture();await page.setViewportSize({width:390,height:844});await page.waitForTimeout(60);await page.screenshot({path:out+'/mobile.png',fullPage:true});assert.equal(await ev(()=>document.documentElement.scrollWidth>innerWidth),false);assert.deepEqual(errors,[]);
 fs.writeFileSync(out+'/checks.json',JSON.stringify({errors,checks:7,shape:empty.radius,states:{empty,note,lock,noteWithLocks:both,playing},lastPage:bounds},null,2));
 console.log('PASS glyph clicks and keyboard editing work; narrow shell fits with zero runtime errors');
}finally{await browser.close();}
