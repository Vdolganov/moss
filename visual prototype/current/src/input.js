const keyMap={ArrowLeft:'Left',ArrowRight:'Right',ArrowUp:'Up',ArrowDown:'Down',KeyZ:'Y',KeyX:'B',KeyC:'A',KeyV:'X',KeyQ:'L1',KeyE:'R1',BracketLeft:'L2',BracketRight:'R2',Space:'Start',Tab:'Select',Escape:'Menu',Enter:'A'};
const arrows=['Left','Right','Up','Down'];
export class Input {
 constructor(app,element){this.app=app;this.element=element;this.held=new Map();this.timer=null;
  element.addEventListener('keydown',e=>{if(e.target.matches('input')){if(e.code==='Enter'){e.preventDefault();this.app.finishName();}else if(e.code==='Escape'){e.preventDefault();this.app.ui.dialog.typing=false;this.app.render();}return;}if(e.ctrlKey||e.metaKey||e.altKey)return;if(e.code==='Tab'&&e.shiftKey){this.clear();return;}const key=keyMap[e.code];if(!key)return;e.preventDefault();if(!e.repeat)this.down(key);});
  window.addEventListener('keyup',e=>{const key=keyMap[e.code];if(key&&this.held.has(key)){e.preventDefault();this.up(key);}});window.addEventListener('blur',()=>this.clear());document.addEventListener('visibilitychange',()=>{if(document.hidden)this.clear();});element.addEventListener('focusout',e=>{if(!element.contains(e.relatedTarget))this.clear();});
  this.timer=setInterval(()=>this.repeat(),35);
 }
 down(key){if(this.held.has(key))return;this.held.set(key,{since:performance.now(),last:performance.now(),used:false});if(!arrows.includes(key))this.app.ui.pointerParam=null;
  if((key==='L1'&&this.held.has('R1'))||(key==='R1'&&this.held.has('L1'))){this.held.get('L1').used=true;this.held.get('R1').used=true;this.app.options();}
  else if((key==='Menu'&&['Y','B','A','X'].some(k=>this.held.has(k)))||(['Y','B','A','X'].includes(key)&&this.held.has('Menu'))){const face=['Y','B','A','X'].find(k=>this.held.has(k));this.held.get(face).used=true;this.held.get('Menu').used=true;this.app.parameterMenu(['Y','B','A','X'].indexOf(face));}
  else if(arrows.includes(key))this.direction(key,1);
  this.app.render();
 }
 direction(key,mult){const modifiers=[...this.held.keys()].filter(k=>!arrows.includes(k));for(const k of modifiers)this.held.get(k).used=true;
  this.app.direction(key,modifiers,mult);
 }
 repeat(){const now=performance.now();for(const [key,v]of this.held){if(key==='Menu'&&!v.used&&now-v.since>=2000){v.used=true;this.app.saveAndExit();}if(arrows.includes(key)&&now-v.since>350&&now-v.last>75){v.last=now;this.direction(key,now-v.since>2000?5:now-v.since>1000?2:1);}}}
 up(key){const v=this.held.get(key);if(!v)return;this.held.delete(key);if(!v.used&&!arrows.includes(key)&&!(key==='L1'&&performance.now()-v.since>=350))this.app.tap(key);if(!this.held.size)this.app.ui.gesture=false;this.app.render();}
 clear(){this.held.clear();this.app.ui.gesture=false;this.app.render?.();}
}
