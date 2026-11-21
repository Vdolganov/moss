import {LAYOUT as L,SPACE as S} from './layout.js';
import {noteName} from './model.js';
// One SVG above the grid joins cells without adding pointer targets or moving text.
export function durationLines(spans,track,page,top,height,layout=null){
 const columns=layout?.columns??8,rows=layout?.rows??2,gap=layout?.gap??S[8],width=layout?.width??L.cellWidth;
 const inset=layout?.inset??S[8],lineInset=layout?.lineInset??L.durationInset,mark=layout?.mark??3;
 const firstCell=page*(layout?.pageSize??16);
 const step=64/track.division,pitch=width+gap,total=Math.ceil(track.cycle/step),paths=[];
 for(let row=0;row<rows;row++){
  const first=firstCell+row*columns,count=Math.min(columns,total-first);if(count<=0)continue;
  const begin=first*step,end=Math.min((first+count)*step,track.cycle),y=row*(height+gap)+height-lineInset;
  const position=(at,ending=false)=>{const u=(at-begin)/step,col=Math.floor(u),fraction=u-col;if(ending&&fraction<1e-8&&col>0)return col*pitch-gap-inset;return col*pitch+inset+fraction*(width-inset*2);};
  for(const span of spans){
   if(span.end<=begin||span.start>=end)continue;
   if(span.length<=step+1e-8&&Math.floor(span.start/step)===Math.floor((span.end-1e-8)/step))continue;
   const incoming=span.start<begin,outgoing=span.end>end;
   const x1=incoming?Math.max(2,inset/2):position(span.start),x2=outgoing?(count-1)*pitch+width-Math.max(2,inset/2):position(span.end,true);
   const marks=(incoming?`<path class="duration-in" d="M${x1-mark} ${y-mark} L${x1} ${y} L${x1-mark} ${y+mark}"/>`:`<circle class="duration-start" cx="${x1}" cy="${y}" r="${mark===3?2:1}"/>`)
    +(outgoing?`<path class="duration-out" d="M${x2-mark} ${y-mark} L${x2} ${y} L${x2-mark} ${y+mark}"/>`:`<path class="duration-end" d="M${x2} ${y-mark} V${y+mark}"/>`);
   paths.push(`<g data-origin="${span.origin}" data-row="${row}"><path d="M${x1} ${y} H${x2}"/>${marks}</g>`);
  }
 }
 const compact=layout?.tier==='compact',areaHeight=height*rows+gap*(rows-1);
 return paths.length?`<svg class="duration-lines ${compact?'duration-compact':''}" style="left:${L.inset}px;top:${top}px" width="${L.content}" height="${areaHeight}" viewBox="0 0 ${L.content} ${areaHeight}" aria-hidden="true" focusable="false">${paths.join('')}</svg>`:'';
}

// Keep the actual endpoint visible when only a tiny fraction carries over.
// The full note name remains in the selected-step context and accessible name.
export function durationLabel(span,tick,step){
 const text=noteName(span.note),width=text.length>2?S[32]:S[24];
 const room=(span.end-tick)/step*(L.cellWidth-S[8]*2);
 return room>=width+S[4]?{text,width}:null;
}
