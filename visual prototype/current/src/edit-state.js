import {clone} from './model.js';

export function syncDraft(a){
 const u=a.ui,source=a.source();
 if(!source||!u.cuts.length)return;
 a.data.drafts??={};
 const previous=a.data.drafts[source.id];
 const content={source:source.id,sequence:u.seq,cuts:clone(u.cuts),four:u.four};
 if(!previous||JSON.stringify([previous.cuts,previous.four])!==JSON.stringify([content.cuts,content.four]))a.data.drafts[source.id]={...content,prepared:false};
}
export function restoreDraft(a){
 const draft=a.data.drafts?.[a.ui.source];
 if(draft){a.ui.cuts=clone(draft.cuts);a.ui.four=draft.four;a.ui.cut=Math.min(a.ui.cut,draft.cuts.length-1);a.ui.sliceBank=Math.min(a.ui.sliceBank,draft.four?2:0);}
}
export function mutate(a,fn,checkpoint=true){
 if(a.mutationDepth)return fn();
 const before=clone(a.data),oldUndo=a.session.undoData;
 const beforeText=JSON.stringify(before);
 a.mutationDepth=1;
 try{
  fn();syncDraft(a);a.lastMutationError=null;
  if(JSON.stringify(a.data)===beforeText){a.session.undoData=oldUndo;return false;}
  a.session.undoData=checkpoint?before:oldUndo;
  a.changed();return true;
 }catch(e){
  if(JSON.stringify(a.data)!==beforeText){a.session.data=before;restoreDraft(a);}
  a.session.undoData=oldUndo;a.lastMutationError=e.message;a.say(e.message);return false;
 }finally{a.mutationDepth=0;a.render();}
}
