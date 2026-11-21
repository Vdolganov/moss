import {SPACE} from './layout.js';
// Keep horizontal navigation stable when the instrument replaces its screen HTML.
const controllers=new WeakMap();
const FADE_MARGIN=SPACE[32],SCROLL_MS=140;
class TabStrips{
 constructor(root){
  this.root=root;this.states=new Map();this.frame=0;
  this.resize=new ResizeObserver(entries=>{for(const entry of entries){const s=this.states.get(entry.target.dataset.strip);if(s?.element===entry.target)this.layout(s);}});
  document.fonts.ready.then(()=>{for(const s of this.states.values())if(s.element?.isConnected)this.layout(s);});
 }
 sync(){
  this.resize.disconnect();
  for(const s of this.states.values())s.element=null;
  for(const element of this.root.querySelectorAll('[data-strip]')){
   const key=element.dataset.strip;
   let s=this.states.get(key);
   if(!s){s={position:0,target:0,selected:null,width:0,total:0,animating:false};this.states.set(key,s);}
   s.element=element;element.scrollLeft=s.position;
   element.addEventListener('scroll',()=>{if(s.element!==element)return;if(!s.animating)s.position=element.scrollLeft;this.edges(s);});
   element.addEventListener('wheel',e=>{
    if(element.scrollWidth<=element.clientWidth)return;
    e.preventDefault();s.animating=false;
    element.scrollLeft+=Math.abs(e.deltaX)>Math.abs(e.deltaY)?e.deltaX:e.deltaY;
    s.position=element.scrollLeft;this.edges(s);
   },{passive:false});
   element.addEventListener('pointerdown',()=>{s.animating=false;s.position=element.scrollLeft;});
   this.layout(s);this.resize.observe(element);
  }
 }
 layout(s){
  const el=s.element,active=el?.querySelector('[aria-selected="true"]');if(!active||!el.clientWidth)return;
  const width=el.clientWidth,total=el.scrollWidth,max=Math.max(0,total-width),selected=active.dataset.action+':'+active.dataset.value;
  if(s.selected!==selected||s.width!==width||s.total!==total){
   const changed=s.selected!==null&&s.selected!==selected;
   const item=active.closest('.nested-tab')||active,left=item.offsetLeft,right=left+item.offsetWidth;
   let target=Math.min(s.position,max);
   const safeLeft=target>0?FADE_MARGIN:0,safeRight=target<max?FADE_MARGIN:0;
   if(left<target+safeLeft||right>target+width-safeRight)target=Math.min(max,Math.max(0,left+(item.offsetWidth-width)/2));
   s.selected=selected;s.width=width;s.total=total;s.target=target;
   s.animating=changed&&Math.abs(target-s.position)>.5&&!matchMedia('(prefers-reduced-motion: reduce)').matches;
   if(s.animating){s.from=s.position;s.started=performance.now();this.animate();}
   else{s.position=target;el.scrollLeft=target;}
  }else el.scrollLeft=s.position;
  this.edges(s);
 }
 edges(s){
  const el=s.element;if(!el)return;
  const max=Math.max(0,el.scrollWidth-el.clientWidth);
  el.dataset.overflowLeft=String(el.scrollLeft>.5);
  el.dataset.overflowRight=String(max-el.scrollLeft>.5);
 }
 animate(){
  if(this.frame)return;
  this.frame=requestAnimationFrame(now=>{
   this.frame=0;let pending=false;
   for(const s of this.states.values())if(s.animating){
    if(!s.element?.isConnected){s.animating=false;continue;}
    const progress=Math.min(1,(now-s.started)/SCROLL_MS);
    s.position=s.from+(s.target-s.from)*(1-(1-progress)**3);
    s.element.scrollLeft=s.position;this.edges(s);
    s.animating=progress<1;pending ||= s.animating;
   }
   if(pending)this.animate();
  });
 }
}
export function syncTabStrips(root){let controller=controllers.get(root);if(!controller){controller=new TabStrips(root);controllers.set(root,controller);}controller.sync();}
