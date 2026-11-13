(function(root){
'use strict';
const clamp=(v,lo,hi)=>Math.max(lo,Math.min(hi,v)),wrap=v=>(v%1+1)%1;
const SOURCES=[{id:'air',name:'AIR CUT',seconds:6.4,mib:.78,seed:31},{id:'dust',name:'TAPE DUST',seconds:8,mib:.98,seed:73},{id:'glass',name:'GLASS CHOIR',seconds:2.8,mib:.34,seed:129}];
const BANKS=[['speed','pitch','size','density'],['position','start','end','scatter'],['spread','irregularity','cutoff','level'],['grainIn','grainOut','noteIn','noteOut']];
const SPEC={speed:[-3,3,.01,.1,'×'],pitch:[-24,24,1,12,'st'],size:[1,500,1,100,'ms'],density:[0,128,1,10,'/s'],position:[0,100,.1,1,'%'],start:[0,99.9,.1,1,'%'],end:[.1,100,.1,1,'%'],scatter:[0,100,1,10,'%'],spread:[0,100,1,10,'%'],irregularity:[0,100,1,10,'%'],cutoff:[20,14000,20,100,'Hz'],level:[0,200,1,10,'%'],grainIn:[0,100,1,10,'%'],grainOut:[0,100,1,10,'%'],noteIn:[0,9000,10,100,'ms'],noteOut:[0,9000,10,100,'ms']};
const DEFAULTS={speed:.25,pitch:-7,size:320,density:24,position:36,start:0,end:100,scatter:100,spread:65,irregularity:25,cutoff:7600,level:100,grainIn:50,grainOut:50,noteIn:700,noteOut:700};
function cleanParams(input={}){const p={...DEFAULTS};for(const key of Object.keys(SPEC)){const n=Number(input[key]);if(Number.isFinite(n))p[key]=clamp(n,SPEC[key][0],SPEC[key][1]);}if(Number.isFinite(input.fade)){if(input.noteIn==null)p.noteIn=clamp(input.fade,0,9000);if(input.noteOut==null)p.noteOut=clamp(input.fade,0,9000);}if(p.grainIn+p.grainOut>100)p.grainOut=100-p.grainIn;p.end=Math.max(p.start+.1,p.end);p.position=clamp(p.position,p.start,p.end);return p;}
function edit(p,key,direction,coarse=false,multiplier=1){const result={...p},s=SPEC[key];if(!s)return result;result[key]=Math.round(clamp(result[key]+direction*s[coarse?3:2]*multiplier,s[0],s[1])*100)/100;if(key==='grainIn'||key==='grainOut'){const other=key==='grainIn'?'grainOut':'grainIn';result[other]=Math.min(result[other],100-result[key]);}if(key==='start')result.start=Math.min(result.start,result.end-.1);if(key==='end')result.end=Math.max(result.end,result.start+.1);result.position=clamp(result.position,result.start,result.end);return cleanParams(result);}
function restore(raw){return {params:cleanParams(raw?.params),source:raw?.source===null?null:SOURCES.some(s=>s.id===raw?.source)?raw.source:'air',variant:['splice','field','lens'].includes(raw?.variant)?raw.variant:'field',bank:Number.isInteger(raw?.bank)&&raw.bank>=0&&raw.bank<BANKS.length?raw.bank:0,freeze:raw?.freeze===true,static:raw?.static===true};}
class Cloud {
 constructor(p=DEFAULTS,source=SOURCES[0]){this.params=cleanParams(p);this.source=source;this.head=this.params.position/100;this.grains=[];this.phase=1;this.random=31;this.serial=0;this.freeze=false;this.skipped=0;}
 noise(){this.random=(Math.imul(this.random,1664525)+1013904223)>>>0;return this.random/4294967296;}
 configure(p,source=this.source,freeze=this.freeze){const next=cleanParams(p);if(source?.id!==this.source?.id){this.grains=[];this.head=next.position/100;this.phase=1;}if(next.position!==this.params.position)this.head=next.position/100;this.params=next;this.source=source;this.freeze=freeze;this.head=clamp(this.head,next.start/100,next.end/100);}
 readPosition(g){return g.first+wrap((g.birth-g.first+g.age*g.rate/this.source.seconds)/g.span)*g.span;}
 advance(seconds){
  if(!this.source)return;
  let remaining=clamp(seconds,0,.1);
  while(remaining>0){
   const dt=Math.min(remaining,1/240);remaining-=dt;const p=this.params;
   this.grains=this.grains.filter(g=>(g.age+=dt)<g.life);
   const first=p.start/100,span=(p.end-p.start)/100;
   if(!this.freeze)this.head=first+wrap((this.head-first+p.speed*dt/this.source.seconds)/span)*span;
   this.phase+=p.density*dt;
   if(this.phase<1||!p.density)continue;
   this.phase-=1;this.phase+=(this.noise()-.5)*p.irregularity/100;
   if(this.grains.length>=8){this.skipped++;continue;}
   const counts=Array(8).fill(0);for(const g of this.grains)counts[g.zone]++;
   const least=Math.min(...counts),zones=counts.map((n,i)=>n===least?i:-1).filter(i=>i>=0);
   const zone=zones[Math.floor(this.noise()*zones.length)],fraction=(zone+this.noise())/8;
   const birth=p.scatter===100?first+fraction*span:first+wrap((this.head-first)/span+(fraction-.5)*p.scatter/100)*span;
   const slots=new Set(this.grains.map(g=>g.slot));let slot=0;while(slots.has(slot))slot++;
   this.grains.push({id:this.serial++,slot,zone,first,span,age:0,life:p.size/1000,birth,pan:(this.noise()*2-1)*p.spread/100,grainIn:p.grainIn/100,grainOut:p.grainOut/100,pitch:p.pitch,rate:2**(p.pitch/12)});
  }
 }
 prime(){this.grains=[];this.phase=1;const head=this.head;for(let i=0;i<20;i++)this.advance(.02);this.head=head;}
}
function windowGain(g){const x=g.age/g.life;if(x<=0||x>=1)return 0;if(g.grainIn>0&&x<g.grainIn)return .5-.5*Math.cos(Math.PI*x/g.grainIn);if(g.grainOut>0&&x>1-g.grainOut)return .5-.5*Math.cos(Math.PI*(1-x)/g.grainOut);return 1;}
function wave(x,seed=31,cutoff=14000){const envelope=.16+.55*Math.sin(x*9+seed)**2+.2*Math.sin(x*23-seed)**2;return envelope*(.55*Math.sin(x*107+seed)+.25*Math.sin(x*241+seed*.2)*Math.sqrt(cutoff/14000)+.2*Math.sin(x*541)*cutoff/14000);}
const api={SOURCES,BANKS,SPEC,DEFAULTS,clamp,wrap,cleanParams,edit,restore,Cloud,windowGain,wave};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.GrainStudy=api;
})(typeof window==='undefined'?globalThis:window);
