import {chromium} from 'playwright';import path from 'node:path';import fs from 'node:fs';
const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('http://127.0.0.1:4174');await page.waitForFunction(()=>window.moss?.input);await page.evaluate(()=>document.fonts.ready);
const output=path.resolve('../../.impeccable/review/interface-lab-v012');fs.mkdirSync(output,{recursive:true});
await page.screenshot({path:path.join(output,'desktop.png')});
for(const kind of ['fm','granular','sampler','slicer','song']){await page.evaluate(kind=>moss.scenario(kind),kind);await page.waitForTimeout(150);await page.locator('#device').screenshot({path:path.join(output,kind+'.png')});}
await page.evaluate(()=>{moss.scenario('fm');moss.ui.page='space';moss.ui.fxId=3;moss.ui.fxDetail=true;moss.render();});await page.waitForTimeout(150);await page.locator('#device').screenshot({path:path.join(output,'reel.png')});
for(const [name,instrument]of[['mono',1],['organism',2]]){await page.evaluate(instrument=>{moss.ui.page='voice';moss.track().instrument=instrument;moss.ui.voiceBank=0;moss.previewVoice();moss.render();},instrument);await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,name+'.png')});}
await page.evaluate(()=>{moss.scenario('fm');moss.previewVoice();});await page.waitForTimeout(150);await page.locator('#device').screenshot({path:path.join(output,'fm-active.png')});
await page.evaluate(()=>{moss.ui.page='pattern';moss.ui.bank=2;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'locks.png')});
await page.evaluate(()=>{moss.ui.page='mix';moss.ui.mixTab=0;moss.ui.fxDetail=false;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'mix.png')});
await page.evaluate(()=>{moss.ui.mixTab=1;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'master-fx.png')});
await page.evaluate(()=>{moss.scenario('slicer');moss.ui.sliceBank=2;moss.ui.zoom=8;moss.ui.fineCenter=25000;moss.selectCut(2);moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'fine.png')});
await page.evaluate(()=>{moss.ui.tapeTab=2;moss.ui.libFolder=null;moss.ui.row=0;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'memory.png')});
await page.evaluate(()=>moss.browseSources());await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'folders.png')});
await page.evaluate(()=>moss.name('SEQUENCE NAME','A LONG SEQUENCE NAME 1234',()=>{}));await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'name.png')});
await page.evaluate(()=>{moss.scenario('fm');moss.ui.page='grid';moss.options();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'options.png')});
await page.evaluate(()=>{moss.ui.dialog=null;moss.scenario('sampler');moss.track().keys[0].asset=null;moss.ui.page='voice';moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'empty-sampler.png')});
await page.evaluate(()=>{moss.ui.page='space';moss.ui.fxDetail=true;moss.ui.fxId=3;moss.ui.fxBank=1;moss.track().fx.values[3][4]=10;moss.track().fx.values[3][6]=10;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'reel-units.png')});
await page.evaluate(()=>{moss.ui.fxBank=3;moss.track().fx.values[3][12]=10;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'reel-hiss.png')});
await page.evaluate(()=>{moss.ui.fxId=1;moss.ui.fxBank=0;moss.track().fx.values[1][1]=50;moss.render();});await page.waitForTimeout(120);await page.locator('#device').screenshot({path:path.join(output,'pond-units.png')});
await page.setViewportSize({width:390,height:844});await page.evaluate(()=>moss.scenario('fm'));await page.waitForTimeout(150);await page.screenshot({path:path.join(output,'mobile.png'),fullPage:true});
console.log(JSON.stringify({errors,output}));await browser.close();
