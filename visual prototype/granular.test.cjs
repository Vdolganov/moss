const test=require('node:test'),assert=require('node:assert/strict'),G=require('./granular-model.js');
test('full cloud distributes simultaneous births across the selected region regardless of anchor',()=>{
 const a=new G.Cloud({...G.DEFAULTS,start:20,end:80,position:25,scatter:100,size:500,density:128,irregularity:0});
 const b=new G.Cloud({...a.params,position:75,speed:-3});b.freeze=true;
 a.advance(.07);b.advance(.07);
 assert.equal(a.grains.length,8);assert.equal(new Set(a.grains.map(g=>g.zone)).size,8);
 assert.deepEqual(a.grains.map(g=>g.birth),b.grains.map(g=>g.birth));
 for(let i=0;i<500;i++){a.advance(.02);for(const g of a.grains){assert.ok(g.birth>=.2&&g.birth<.8);assert.ok(a.readPosition(g)>=.2&&a.readPosition(g)<.8);}}
});
test('grain envelopes are separate from note fades and remain latched after edits',()=>{
 let p=G.edit({...G.DEFAULTS},'grainIn',1,true);assert.equal(p.grainIn,60);assert.equal(p.grainOut,40);
 const c=new G.Cloud({...p,grainIn:10,grainOut:20});c.advance(.02);const g=c.grains[0];
 c.configure({...c.params,grainIn:90,grainOut:10});assert.equal(g.grainIn,.1);assert.equal(g.grainOut,.2);
 g.age=g.life*.5;assert.equal(G.windowGain(g),1);g.age=0;assert.equal(G.windowGain(g),0);g.age=g.life;assert.equal(G.windowGain(g),0);
 const s=G.restore({bank:3,params:{fade:900}});assert.equal(s.bank,3);assert.equal(s.params.noteIn,900);assert.equal(s.params.noteOut,900);
});
test('fine and coarse edits use displayed units and preserve ordered scan limits',()=>{
 let p={...G.DEFAULTS};p=G.edit(p,'pitch',1);assert.equal(p.pitch,-6);p=G.edit(p,'pitch',1,true);assert.equal(p.pitch,6);
 p=G.edit(p,'size',1);assert.equal(p.size,321);p=G.edit(p,'size',1,true);assert.equal(p.size,421);
 for(let i=0;i<2000;i++){p=G.edit(p,'start',1,true);p=G.edit(p,'end',-1,true);assert.ok(p.end-p.start>=.09999);assert.ok(p.position>=p.start&&p.position<=p.end);}
});
test('a frozen scan stays still while new grains live and expire; pool never exceeds eight',()=>{
 const c=new G.Cloud({...G.DEFAULTS,size:500,density:128,irregularity:100});c.freeze=true;const head=c.head;
 for(let i=0;i<1000;i++){c.advance(.04);assert.ok(c.grains.length<=8);assert.equal(c.head,head);for(const g of c.grains){assert.ok(g.birth>=0&&g.birth<1);assert.ok(g.age>=0&&g.age<g.life);}}
 assert.ok(c.serial>8&&c.skipped>0);c.configure({...c.params,density:0});for(let i=0;i<20;i++)c.advance(.05);assert.equal(c.grains.length,0);
});
test('reverse scan wraps within a narrow selection; position changes seek without destroying grains',()=>{
 const c=new G.Cloud({...G.DEFAULTS,start:40,end:41,position:40.5,speed:-3});c.prime();const ids=c.grains.map(g=>g.id);c.configure({...c.params,position:40.8});assert.equal(c.head,.408);assert.deepEqual(c.grains.map(g=>g.id),ids);
 for(let i=0;i<100;i++){c.advance(.04);assert.ok(c.head>=.4&&c.head<=.41);}
 c.configure(c.params,null);assert.equal(c.grains.length,0);c.advance(.1);assert.equal(c.grains.length,0);
});
test('persisted study settings are validated independently of existing projects',()=>{
 const s=G.restore({params:{size:1e6,speed:NaN,start:99,end:-1},source:'missing',bank:22,variant:'unknown'});
 assert.equal(s.source,'air');assert.equal(s.variant,'field');assert.equal(s.bank,0);assert.equal(s.params.size,500);assert.equal(s.params.speed,G.DEFAULTS.speed);assert.ok(s.params.end>s.params.start);
 assert.equal(G.restore({source:null}).source,null);
});
