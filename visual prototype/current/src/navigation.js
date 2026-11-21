// Navigation memory is transient and never enters musical undo history.
export const MENU_PAGE_SIZE=7;
export const LIST_PAGE_SIZE={song:4,mix:4,grid:4,library:5,tape:5};
const trackPages=['pattern','voice','space'];
const trackFields=['cell','key','bank','voiceBank','fxId','fxBank','fxDetail','timingBank'];
export function rememberTrack(a){
 if(!trackPages.includes(a.ui.page)||!a.track())return;
 a.trackPositions??=new Map();
 a.trackPositions.set(a.track().id,Object.fromEntries(trackFields.map(k=>[k,a.ui[k]])));
 rememberFx(a);
}
export function restoreTrack(a){
 Object.assign(a.ui,{cell:0,key:0,bank:0,voiceBank:0,fxId:0,fxBank:0,fxDetail:false,timingBank:0},a.trackPositions?.get(a.track()?.id));
 if(a.track())a.ui.cell=Math.min(a.ui.cell,Math.ceil(a.track().cycle/(64/a.track().division))-1);
}
const effectKey=(a,id=a.ui.fxId)=>`${a.ui.seq}:${a.ui.page==='mix'?'master':a.track()?.id}:${a.ui.page==='pattern'?'lock':'editor'}:${id}`;
export function rememberFx(a){
 a.fxPositions??=new Map();
 if(['pattern','space','mix'].includes(a.ui.page))a.fxPositions.set(effectKey(a),a.ui.fxBank);
}
export function selectFx(a,id,remember=true){if(remember)rememberFx(a);a.ui.fxId=id;a.ui.fxBank=Math.min(a.fxPositions?.get(effectKey(a,id))??0,a.C.effects[id].banks.length-1);}
