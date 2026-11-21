import {BUTTONS,clamp,fineWindow} from './model.js';
import {LAYOUT as L,SPACE as S} from './layout.js';
import {FACE_POSITIONS} from './face-buttons.js';

// Selection, trim edges and preview have independent visual identities.
export function tapeGeometry(a,now){
 const u=a.ui,source=a.source(),cut=u.cuts[u.cut];if(!source||!cut)return null;
 const fine=u.four&&u.sliceBank===2,[first,last]=fine?fineWindow(source,cut,u.zoom,u.fineCenter):[0,source.frames];
 const x=L.inset,w=L.content,labelY=fine?L.tapeFineY:L.tapeWaveY,y=labelY+S[32],bottom=L.tapeWaveBottom,h=bottom-y;
 const at=frame=>x+clamp((frame-first)/(last-first),0,1)*(w-1);
 const regions=u.cuts.flatMap((part,index)=>part.end<=first||part.start>=last?[]:[{
  index,left:at(part.start),right:at(part.end),before:part.start<first,after:part.end>last,
 }]);
 const selected=regions.find(p=>p.index===u.cut),width=u.four?48:40;
 const center=selected?(selected.left+selected.right)/2:cut.end<=first?x:x+w-1;
 const label={index:u.cut,x:clamp(center-width/2,x,x+w-width),width,active:true};
 const labels=[label];
 // Fit inactive letters in their own spans, never cover the selected badge.
 for(const part of regions){
  if(part.index===u.cut||part.right-part.left<12)continue;
  const size=u.four?16:24,left=clamp((part.left+part.right-size)/2,x,x+w-size);
  if(labels.some(l=>left<l.x+l.width+S[4]&&left+size+S[4]>l.x))continue;
  labels.push({index:part.index,x:left,width:size,active:false});
 }
 const held=BUTTONS.findIndex(key=>a.input?.held.has(key)),field=held>=0?held:u.pointerParam;
 const moving=u.four&&u.sliceBank===0&&field===u.cut;
 const editing=moving?'both':(!u.four||u.sliceBank!==0)&&field===0?'start':(!u.four||u.sliceBank!==0)&&field===1?'end':null;
 const p=u.preview;let preview=null;
 if(p?.kind==='source'&&p.source===source.id&&now>=p.start&&now<p.until){
  const frame=p.first+(p.last-p.first)*(now-p.start)/(p.until-p.start);
  if(frame>=first&&frame<=last)preview={x:at(frame),frame};
 }
 return {source,cut,fine,first,last,x,w,labelY,y,bottom,h,regions,selected,labels,center,editing,preview};
}

export function drawTape(a,c,now,wave,P,TYPE){
 const g=tapeGeometry(a,now);if(!g)return;
 const {x,y,w,h,bottom,labelY,regions,selected,cut}=g;
 const ink={outside:'#626469',idle:'#222326',active:'#34363a'};
 const fill=(left,width,color)=>{c.fillStyle=color;c.fillRect(left,y,Math.max(1,width),h);};
 const path=(points,color,width=1)=>{c.strokeStyle=color;c.lineWidth=width;c.beginPath();points.forEach(([px,py],i)=>i?c.lineTo(px,py):c.moveTo(px,py));c.stroke();c.lineWidth=1;};
 const drawWave=color=>wave(a,c,g.source,x,y,w,h,g.first,g.last,color);
 const clipRegions=parts=>{c.beginPath();parts.forEach(p=>c.rect(p.left,y,Math.max(1,p.right-p.left),h));c.clip();};
 const boundary=(px,start,active,editing=false)=>{
  const left=Math.round(px)+.5,cap=active?Math.min(6,Math.max(1,(selected.right-selected.left)/2)):0,side=start?1:-1;
  path([[left+side*cap,y+.5],[left,y+.5],[left,bottom-.5],[left+side*cap,bottom-.5]],active?P.text:P.line,editing?3:active?2:1);
  if(editing){c.fillStyle=P.text;c.fillRect(clamp(left-2,x,x+w-4),y+h/2-4,4,8);}
 };
 const outsideArrow=(left)=>{
  const px=left?x+2:x+w-3,cy=y+h/2,dir=left?1:-1;
  path([[px+dir*5,cy-4],[px,cy],[px+dir*5,cy+4]],P.bg,4);
  path([[px+dir*5,cy-4],[px,cy],[px+dir*5,cy+4]],P.text,1.5);
 };
 c.save();c.beginPath();c.rect(x,L.tapeWaveY,w,L.tapeWaveBottom-L.tapeWaveY);c.clip();
 // Opaque, identical backgrounds avoid cumulative brightness at overlaps.
 regions.forEach(p=>fill(p.left,p.right-p.left,ink.idle));drawWave(ink.outside);
 if(regions.length){c.save();clipRegions(regions);drawWave(P.dim);c.restore();}
 for(const p of regions)if(p.index!==a.ui.cut){if(!p.before)boundary(p.left,true,false);if(!p.after)boundary(p.right,false,false);}
 if(selected){
  fill(selected.left,selected.right-selected.left,ink.active);
  c.save();clipRegions([selected]);drawWave(P.text);c.restore();
  if(selected.before)outsideArrow(true);else boundary(selected.left,true,true,g.editing==='start'||g.editing==='both');
  if(selected.after)outsideArrow(false);else boundary(selected.right,false,true,g.editing==='end'||g.editing==='both');
 }else outsideArrow(cut.end<=g.first);
 // A leader keeps a floating label associated with even a subpixel selection.
 const label=g.labels[0],mid=label.x+label.width/2;
 path([[mid,labelY+24],[g.center,y-4],[g.center,y]],P.line);
 for(const l of g.labels){
  const key=a.ui.four?BUTTONS[l.index]:String(l.index+1).padStart(2,'0');
  c.font=`${TYPE.small} Moss`;c.textBaseline='top';
  if(l.active){
   c.fillStyle=P.text;c.beginPath();c.roundRect(l.x,labelY,l.width,24,12);c.fill();
   if(a.ui.four){
    c.save();c.translate(l.x+5,labelY+3);c.scale(.75,.75);c.strokeStyle=P.bg;c.fillStyle=P.bg;c.lineWidth=1.1;
    for(const [button,[px,py]]of Object.entries(FACE_POSITIONS)){c.beginPath();c.arc(px,py,3,0,Math.PI*2);if(button===key)c.fill();else c.stroke();}c.restore();
   }
   c.fillStyle=P.bg;c.fillText(key,a.ui.four?l.x+30:l.x+(l.width-c.measureText(key).width)/2,labelY+6);
  }else{c.fillStyle=P.dim;c.fillText(key,l.x+(l.width-c.measureText(key).width)/2,labelY+6);}
 }
 // Preview has a triangular head; trim boundaries have inward-facing brackets.
 if(g.preview){const px=clamp(Math.round(g.preview.x)+.5,x+1,x+w-2);
  path([[px,y-3],[px,bottom]],P.bg,3);path([[px,y-3],[px,bottom]],P.text);
  c.fillStyle=P.text;c.strokeStyle=P.bg;c.lineWidth=1;c.beginPath();c.moveTo(px-4,y-8);c.lineTo(px+4,y-8);c.lineTo(px,y-3);c.closePath();c.fill();c.stroke();
 }
 if(g.fine){c.font=`${TYPE.small} Moss`;c.fillStyle=P.dim;c.fillText(`FRAME ${Math.floor(g.first)}-${Math.floor(g.last)}`,x,L.tapeWaveY);}
 c.restore();
}
