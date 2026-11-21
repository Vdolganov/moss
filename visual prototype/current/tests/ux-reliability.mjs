import {chromium} from 'playwright';
import assert from 'node:assert/strict';
import fs from 'node:fs';
const out='../../work/ux-fixes-2026-11-20';fs.mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true,executablePath:process.env.MOSS_BROWSER||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[],results=[];
page.on('pageerror',e=>errors.push(e.message));
await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await page.evaluate(()=>document.fonts.ready);
const ev=fn=>page.evaluate(fn),key=k=>page.keyboard.press(k);
async function chord(k,arrow){await page.locator('#device').focus();await page.keyboard.down(k);await key(arrow);await page.keyboard.up(k);}
async function menu(label){await page.evaluate(label=>{const d=moss.ui.dialog;const i=d?.items?.findIndex(x=>x.label===label);if(i===undefined||i<0)throw Error('Missing menu '+label);d.row=i;moss.choose();},label);}
async function reset(){await ev(async()=>{const M=await import('./src/model.js');moss.session.stop();moss.session.data=M.freshState(moss.C);moss.session.undoData=null;moss.ui.dialog=null;moss.ui.toast=null;moss.ui.assign=null;moss.ui.source=null;moss.ui.cuts=[];moss.trackPositions=new Map();moss.fxPositions=new Map();moss.openSequence(moss.data.sequences[1].id);moss.openTrack(0);moss.ui.page='voice';moss.ui.voiceBank=0;moss.render();});}
async function check(name,fn){try{await reset();await fn();results.push({name,pass:true});console.log('PASS '+name);}catch(e){results.push({name,pass:false,error:e.stack});console.error('FAIL '+name+' / '+e.message);await page.screenshot({path:`${out}/failure-${results.length}.png`});}}
await check('undo survives preset menu navigation and a whole held gesture',async()=>{
 const base=await ev(()=>moss.track().voices[3][0]);await chord('z','ArrowRight');await chord('q','e');await menu('PRESET');await key('ArrowDown');await key('Escape');await key('Escape');await ev(()=>moss.options());await menu('UNDO LAST EDIT');assert.equal(await ev(()=>moss.track().voices[3][0]),base);
 await page.locator('#device').focus();await page.keyboard.down('z');await key('ArrowRight');await key('ArrowRight');await page.keyboard.up('z');await ev(()=>moss.undo());assert.equal(await ev(()=>moss.track().voices[3][0]),base);
});
await check('FM RATE and RELEASE use physical units, including locks',async()=>{
 const data=await ev(()=>{moss.ui.voiceBank=4;return moss.params().map(f=>({label:f.label,text:f.text,unit:f.unit,value:f.value}));});assert.equal(data[0].text,(data[0].value*.01).toFixed(2));assert.equal(data[2].text,String(data[2].value));assert.equal(data[2].unit,'MS');
 await ev(()=>{moss.ui.page='pattern';moss.ui.bank=1;moss.ui.voiceBank=4;moss.editField(0,1,false,1);});assert.equal(await ev(()=>moss.params()[0].unit),'HZ');
});
await check('clear selected hit preserves every other lane and track automation',async()=>{
 const result=await ev(async()=>{moss.scenario('sampler');const M=await import('./src/model.js'),t=moss.track();moss.ui.cell=0;moss.ui.key=0;t.hits[0][0]={...M.emptyHit(),type:1};t.hits[1][0]={...M.emptyHit(),type:1};t.steps[0]={...M.emptyStep(),type:2,fx:{0:{0:2}},shift:8,repeat:32};const original=structuredClone(t.steps[0]);moss.change(()=>moss.clearStep());return{selected:t.hits[0][0],other:t.hits[1][0].type,shared:t.steps[0],original};});assert.equal(result.selected,undefined);assert.equal(result.other,1);assert.deepEqual(result.shared,result.original);
 await ev(()=>moss.undo());assert.equal(await ev(()=>moss.track().hits[0][0].type),1);
});
await check('individual lock SET CLEAR RESET works and preserves adjacent locks',async()=>{
 await ev(()=>{moss.ui.page='pattern';moss.ui.cell=2;moss.ui.bank=1;moss.editField(0,1,false,1);moss.editField(1,1,false,1);});
 const adjacent=await ev(()=>moss.params()[1].value);
 await chord('z','Escape');await menu('RESET TO BASE');assert.equal(await ev(()=>moss.params()[0].text),'RST');
 await ev(()=>moss.parameterMenu(0));await menu('CLEAR LOCK');assert.equal(await ev(()=>moss.params()[0].text),'--');assert.equal(await ev(()=>moss.params()[1].value),adjacent);
 await ev(()=>moss.parameterMenu(0));await menu('SET LOCK');assert.equal(await ev(()=>moss.params()[0].value),await ev(()=>moss.track().voices[3][0]));
});
await check('L1/R1 only switch visible tabs; slot selection uses L1+arrows',async()=>{
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.render();});const slot=await ev(()=>moss.ui.key);await key('e');assert.equal(await ev(()=>moss.ui.key),slot);await chord('q','ArrowRight');assert.equal(await ev(()=>moss.ui.key),(slot+1)%4);
 await ev(()=>{moss.ui.page='grid';moss.ui.ti=0;});await key('e');assert.equal(await ev(()=>moss.ui.ti),0);
 await ev(()=>{moss.ui.page='space';moss.ui.fxDetail=false;moss.ui.fxId=0;});await key('e');assert.equal(await ev(()=>moss.ui.fxId),0);
});
await check('track cursor and effect bank contexts are restored',async()=>{
 await ev(()=>{moss.ui.cell=7;moss.ui.voiceBank=3;moss.ui.bank=1;moss.back();moss.openTrack(1);moss.ui.cell=3;moss.back();moss.openTrack(0);});assert.deepEqual(await ev(()=>[moss.ui.cell,moss.ui.voiceBank,moss.ui.bank]),[7,3,1]);
 await ev(()=>{moss.click('major','space');moss.click('fx','3');moss.ui.fxBank=2;moss.click('fx','0');moss.ui.fxBank=1;moss.click('fx','3');});assert.equal(await ev(()=>moss.ui.fxBank),2);
});
await check('assignment back traverses folder, library, then original voice bank',async()=>{
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.ui.key=2;moss.assignSample();moss.ui.row=1;moss.chooseLibrary();});await key('x');assert.equal(await ev(()=>moss.ui.page),'tape');assert.equal(await ev(()=>moss.ui.libFolder),null);await key('x');assert.equal(await ev(()=>moss.ui.page),'voice');assert.equal(await ev(()=>moss.ui.key),2);
});
await check('kit replacement is explicit and a single undo restores all assignments',async()=>{
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.ui.key=1;moss.assignSample();moss.openSource(moss.data.imports[0].id);moss.options();});await menu('SPLIT INTO FOUR PARTS');
 const old=await ev(()=>moss.track().keys.map(k=>k.asset));await ev(()=>moss.options());assert.deepEqual(await ev(()=>moss.ui.dialog.items.filter(x=>x.label.startsWith('SAVE')).map(x=>x.label)),['SAVE KIT','SAVE SELECTED + ASSIGN','SAVE KIT + ASSIGN FOUR','SAVE']);await menu('SAVE KIT + ASSIGN FOUR');assert.equal(await ev(()=>moss.ui.dialog.title),'REPLACE ALL FOUR SLOTS?');await menu('CONFIRM');await ev(()=>{moss.ui.dialog.value='NEW KIT';moss.finishName();});assert.equal(await ev(()=>moss.ui.page),'voice');assert.notDeepEqual(await ev(()=>moss.track().keys.map(k=>k.asset)),old);await ev(()=>moss.undo());assert.deepEqual(await ev(()=>moss.track().keys.map(k=>k.asset)),old);assert.equal(await ev(()=>moss.data.kits.some(k=>k.name==='NEW KIT')),false);
});
await check('draft cuts survive reload and reopen, prepared samples stay distinct',async()=>{
 await ev(async()=>{await moss.openSource(moss.data.imports[0].id);moss.change(()=>{moss.ui.cuts[0].start=123;});await moss.persist();});const id=await ev(()=>moss.ui.source);await page.reload();await page.waitForFunction(()=>window.moss?.input);await page.evaluate(async id=>{moss.ui.seq=moss.data.sequences[0].id;await moss.openSource(id);},id);assert.equal(await ev(()=>moss.ui.cuts[0].start),123);assert.equal(await ev(()=>moss.data.assets.length),7);assert.match(await page.locator('#save-state').textContent(),/черновик/);
});
await check('save exit waits for storage, failure never claims success',async()=>{
 await ev(async()=>{await moss.persist();moss.storageOriginal=moss.storage.save;moss.storage.save=()=>new Promise(resolve=>moss.releaseSave=resolve);moss.change(()=>moss.track().level=55);moss.pendingExit=moss.saveAndExit();});assert.equal(await ev(()=>moss.ui.page),'voice');assert.equal(await ev(()=>moss.dirty),true);await ev(async()=>{moss.releaseSave();await moss.pendingExit;moss.storage.save=moss.storageOriginal;});assert.equal(await ev(()=>moss.ui.page),'library');
 await reset();await ev(async()=>{moss.storage.save=async()=>{throw Error('quota');};moss.change(()=>moss.track().level=42);await moss.saveAndExit();});assert.equal(await ev(()=>moss.ui.page),'voice');assert.equal(await ev(()=>moss.dirty),true);assert.doesNotMatch(await ev(()=>moss.ui.toast.text),/^SAVED/);assert.match(await page.locator('#save-state').textContent(),/Не сохранено/);await ev(()=>{moss.storage.save=moss.storageOriginal;});
});
await check('SONG duplicate, restore and JSON undo are reachable in library',async()=>{
 await ev(()=>{moss.scenario('song');moss.ui.page='library';moss.ui.atlasTab=1;moss.ui.row=0;moss.options();});assert.ok(await ev(()=>moss.ui.dialog.items.some(x=>x.label==='DUPLICATE')));await menu('DELETE');await menu('CONFIRM');await ev(()=>moss.options());await menu('RESTORE DELETED');assert.equal(await ev(()=>moss.data.songs.length),1);
 const exported=await ev(()=>JSON.stringify(moss.data));await page.locator('#project-file').setInputFiles({name:'library.json',mimeType:'application/json',buffer:Buffer.from(exported)});await menu('CONFIRM');await ev(()=>moss.options());assert.ok(await ev(()=>moss.ui.dialog.items.some(x=>x.label==='UNDO LAST EDIT')));await menu('UNDO LAST EDIT');
});
await check('all-failed WAV and invalid JSON settle with useful errors',async()=>{
 await ev(async()=>{await moss.persist();});await page.locator('#wavs').setInputFiles({name:'broken.wav',mimeType:'audio/wav',buffer:Buffer.from('bad')});await page.waitForFunction(()=>moss.ui.dialog?.title==='IMPORT REPORT');assert.doesNotMatch(await page.locator('#save-state').textContent(),/Читаю/);await ev(()=>{moss.ui.dialog=null;});await page.locator('#project-file').setInputFiles({name:'bad.json',mimeType:'application/json',buffer:Buffer.from('{bad')});await page.waitForFunction(()=>moss.ui.toast?.text.includes('CANNOT OPEN JSON'));assert.match(await ev(()=>moss.ui.toast.text),/MOSS LIBRARY EXPORT/);
});
await check('footer transport really plays SONG and empty POSITION is unavailable',async()=>{
 await ev(()=>moss.scenario('song'));assert.equal(await ev(()=>moss.params()[0].text),'--');await ev(()=>moss.addSlot());await page.evaluate(()=>{moss.ui.dialog.row=0;moss.choose();});await page.locator('.footer [data-action=transport]').click();assert.equal(await ev(()=>moss.session.playing),true);assert.equal(await ev(()=>moss.ui.dialog),null);await ev(()=>moss.session.stop());
});
await check('desktop typing, overlay isolation and focus restoration',async()=>{
 await ev(()=>moss.newSequence());await page.locator('[data-action=name-mode]').click();await page.locator('.name-input').fill('ambient one');await key('Home');await key('ArrowRight');await page.keyboard.type('z');assert.equal(await ev(()=>document.activeElement.selectionStart),2);await key('Enter');assert.equal(await ev(()=>moss.sequence().name),'AZMBIENT ONE');assert.equal(await ev(()=>moss.ui.dialog),null);await ev(()=>moss.options());assert.equal(await page.locator('#workspace-view').getAttribute('inert'),'');await key('Escape');assert.equal(await ev(()=>document.activeElement.id),'device');
});
await check('single-slot save and library-only kit save keep other assignments',async()=>{
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.ui.key=2;moss.assignSample();moss.openSource(moss.data.imports[0].id);moss.options();});await menu('SPLIT INTO FOUR PARTS');const old=await ev(()=>moss.track().keys.map(k=>k.asset));
 await ev(()=>{moss.saveCuts(true,'library');moss.ui.dialog.value='LIBRARY KIT';moss.finishName();});assert.deepEqual(await ev(()=>moss.track().keys.map(k=>k.asset)),old);assert.ok(await ev(()=>moss.ui.assign));
 await ev(()=>{moss.ui.tapeTab=1;moss.ui.cut=3;moss.saveCuts(false,'selected');moss.ui.dialog.value='ONLY C D';moss.finishName();});const next=await ev(()=>moss.track().keys.map(k=>k.asset));assert.notEqual(next[2],old[2]);for(const i of[0,1,3])assert.equal(next[i],old[i]);assert.equal(await ev(()=>moss.ui.page),'voice');
});
await check('kit rename, delete, undo and duplicate-name recovery are reachable',async()=>{
 await ev(()=>{moss.scenario('slicer');moss.saveCuts(true,'library');moss.ui.dialog.value='FREE KIT';moss.finishName();moss.options();});await menu('RENAME FOLDER');await ev(()=>{moss.ui.dialog.value='RENAMED';moss.finishName();});assert.equal(await ev(()=>moss.data.kits.at(-1).name),'RENAMED');assert.equal(await ev(()=>moss.data.assets.at(-1).folder),'RENAMED');await ev(()=>moss.options());await menu('DELETE FOLDER');await menu('CONFIRM');assert.equal(await ev(()=>moss.data.kits.some(k=>k.name==='RENAMED')),false);await ev(()=>moss.undo());assert.equal(await ev(()=>moss.data.kits.some(k=>k.name==='RENAMED')),true);
 await ev(()=>{moss.ui.tapeTab=1;moss.saveCuts(true,'library');moss.ui.dialog.value='RENAMED';moss.finishName();});assert.equal(await ev(()=>moss.ui.dialog.type),'name');assert.equal(await ev(()=>moss.ui.dialog.value),'RENAMED');await ev(()=>{moss.ui.dialog.value='RETRY';moss.finishName();});assert.equal(await ev(()=>moss.data.kits.at(-1).name),'RETRY');
});
await check('source folders return one level and footer preview acts on its own button',async()=>{
 await ev(()=>{moss.scenario('sampler');moss.ui.page='voice';moss.assignSample();moss.browseSources();});await menu('AMBIENT CASSETTE LOOPS /');await menu('TEXTURES /');await key('Escape');assert.equal(await ev(()=>moss.ui.browserPath),'Ambient Cassette Loops');await key('Escape');assert.equal(await ev(()=>moss.ui.browserPath),'');await key('Escape');assert.equal(await ev(()=>moss.ui.dialog),null);assert.equal(await ev(()=>moss.ui.page),'tape');
 await ev(()=>{moss.ui.assign=null;moss.scenario('slicer');moss.ui.toast=null;});await page.locator('.footer [data-action="preview-part"][data-value="1"]').click();assert.equal(await ev(()=>moss.ui.cut),1);assert.equal(await ev(()=>moss.ui.preview.first),await ev(()=>moss.ui.cuts[1].start));
});
await check('failed and boundary edits do not replace undo; disabled reasons are visible',async()=>{
 await ev(()=>{moss.change(()=>moss.track().level=44);moss.track().voices[3][0]=100;moss.ui.voiceBank=0;});const before=await ev(()=>JSON.stringify(moss.session.undoData));await chord('z','ArrowRight');assert.ok(await ev(()=>JSON.stringify(moss.session.undoData))===before,'Boundary edit must preserve undo');await ev(()=>{moss.ui.page='grid';moss.options();});await menu('DUPLICATE TRACK');assert.equal(await ev(()=>moss.ui.dialog.items[moss.ui.dialog.row].reason),'FOUR TRACKS MAX');
});
await check('small browser retains readable 1x and offers fit mode without page overflow',async()=>{
 await page.setViewportSize({width:390,height:844});assert.equal(await ev(()=>getComputedStyle(document.documentElement).getPropertyValue('--scale').trim()),'1');assert.equal(await ev(()=>document.documentElement.scrollWidth<=390),true);assert.equal(await ev(()=>document.getElementById('viewport').scrollWidth>=640),true);await page.locator('#scale').click();await page.locator('#scale').click();assert.equal(await page.locator('#scale').textContent(),'По ширине');await page.setViewportSize({width:1200,height:950});await page.locator('#scale').click();
});
await reset();await ev(()=>{moss.ui.page='pattern';moss.ui.bank=1;moss.editField(0,1,false,1);moss.render();});
await page.locator('#device').screenshot({path:`${out}/locks.png`});await ev(()=>moss.options());await page.locator('#device').screenshot({path:`${out}/menu.png`});await ev(()=>{moss.ui.dialog=null;moss.scenario('slicer');});await page.locator('#device').screenshot({path:`${out}/tape.png`});
results.push({name:'no uncaught browser errors',pass:!errors.length,errors});fs.writeFileSync(`${out}/ux-results.json`,JSON.stringify(results,null,2));await browser.close();if(results.some(r=>!r.pass))process.exitCode=1;
