import {selectFx} from './navigation.js';
// Locks keep effect identities; visible tabs follow the enabled chain order.
export function fxLockPage(a,effect=a.ui.fxId){
 const t=a.track();if(!t?.fx.enabled.includes(effect))return null;
 const def=a.C.effects[effect],bank=effect===a.ui.fxId?Math.min(a.ui.fxBank,def.banks.length-1):0;
 return {id:'fx',index:bank,names:def.banks,kind:'PAGE',effect,bank,name:`${def.name} / ${def.banks[bank]}`};
}
export function patternTabs(a){
 const t=a.track();if(!t)return [];
 const voice={id:'voice',index:t.instrument===0?0:a.ui.voiceBank,names:a.C.instruments[t.instrument].banks,kind:'PAGE'};
 return [{name:'STEP',value:'0'}, {name:'VOICE LOCKS',value:'1',nested:voice},
  ...t.fx.order.filter(id=>t.fx.enabled.includes(id)).map(id=>({name:`${a.C.effects[id].name} LOCKS`,value:`fx:${id}`,nested:fxLockPage(a,id)})),
  {name:'TIMING',value:'3',nested:{id:'timing',index:a.ui.timingBank,names:['CYCLE','SWING'],kind:'PAGE'}}];
}
export function patternTabValue(a){return a.ui.bank===2?`fx:${a.ui.fxId}`:String(a.ui.bank);}
export function normalizePatternTab(a){
 if(a.ui.page!=='pattern')return;
 if(a.ui.bank===2&&!fxLockPage(a)){a.ui.bank=0;a.ui.pointerParam=null;}
}
export function selectPatternTab(a,value){
 if(!patternTabs(a).some(tab=>tab.value===String(value)))return false;
 const u=a.ui;
 if(String(value).startsWith('fx:')){
  const id=+value.slice(3);if(id!==u.fxId)selectFx(a,id);
  u.fxBank=Math.min(u.fxBank,a.C.effects[id].banks.length-1);u.bank=2;
 }else u.bank=+value;
 u.pointerParam=null;return true;
}
export function movePatternTab(a,delta){
 normalizePatternTab(a);const tabs=patternTabs(a),index=tabs.findIndex(t=>t.value===patternTabValue(a));
 return tabs.length?selectPatternTab(a,tabs[(index+delta+tabs.length)%tabs.length].value):false;
}
// Only hidden page stacks need counters and rails. Visible rows remain ordinary tabs.
export function activeNestedPage(a){return a.ui.page==='pattern'?patternTabs(a).find(t=>t.value===patternTabValue(a))?.nested||null:null;}
export function parameterPageStack(a){
 const u=a.ui,t=a.track();
 if(u.page==='pattern')return activeNestedPage(a);
 if(u.page==='voice'&&t?.instrument>0)return {id:'voice-editor',index:u.voiceBank,names:a.C.instruments[t.instrument].banks};
 if((u.page==='space'||u.page==='mix'&&u.mixTab===1)&&u.fxDetail)return {id:'fx-editor',index:u.fxBank,names:a.C.effects[u.fxId].banks};
 if(u.page==='tape'&&u.tapeTab===1&&u.four&&a.source())return {id:'tape-editor',index:u.sliceBank,names:['MOVE','TRIM','FINE']};
 return null;
}
export function moveNestedPage(a,id,delta){
 const u=a.ui,current=parameterPageStack(a);if(u.dialog||u.capture||!current||current.id!==id||current.names.length<2)return false;
 const next=(current.index+delta+current.names.length)%current.names.length;
 if(id==='voice'||id==='voice-editor')u.voiceBank=next;
 else if(id==='fx')u.fxBank=next;
 else if(id==='timing')u.timingBank=next;
 else if(id==='fx-editor')u.fxBank=next;
 else if(id==='tape-editor'){
  u.sliceBank=next;
  if(next===2){u.zoom=0;const cut=u.cuts[u.cut];u.fineCenter=(cut.start+cut.end)/2;}
 }
 u.pointerParam=null;return true;
}
export function objectSelection(a){
 const u=a.ui,t=a.track();
 if((u.page==='pattern'||u.page==='voice')&&t?.instrument===0)return {kind:'SLOT',index:u.key,count:t.keys.length};
 if(u.page==='tape'&&u.tapeTab===1&&a.source()&&u.cuts.length)return {kind:u.four?'PART':'SLICE',index:u.cut,count:u.cuts.length};
 return null;
}
export function moveObject(a,delta){
 const u=a.ui,object=objectSelection(a);if(u.dialog||u.capture||!object||object.count<2)return false;
 const next=(object.index+delta+object.count)%object.count;
 if(object.kind==='SLOT')u.key=next;else a.selectCut(next);
 u.pointerParam=null;return true;
}
export function shoulderNavigate(a,key){
 const delta=key==='Left'||key==='Up'?-1:1;
 if(key==='Left'||key==='Right')return moveObject(a,delta);
 const stack=parameterPageStack(a);return stack?moveNestedPage(a,stack.id,delta):false;
}
export function shoulderHints(a){
 if(a.ui.dialog||a.ui.capture)return [];
 const object=objectSelection(a),pages=parameterPageStack(a);
 return [...(object?.count>1?[{axis:'horizontal',text:object.kind}]:[]),
  ...(pages?.names.length>1?[{axis:'vertical',text:'PAGE'}]:[])];
}
