import {cells} from './model.js';
import {LAYOUT as L,SPACE as S} from './layout.js';

// View geometry only: changing zoom never changes the musical resolution.
export function patternLayout(track,pageSize=64){
 pageSize=pageSize===64?64:16;
 const total=cells(track),visible=Math.min(total,pageSize),sampler=track.instrument===0;
 const top=L.contentY+(sampler?L.rowHeight:L.line)+S[16];
 const fullHeight=sampler?56:64,areaHeight=fullHeight*2+S[8];
 const tier=visible<=16?'full':'compact';
 const columns=tier==='full'?8:16;
 const rows=tier==='full'?2:Math.ceil(visible/columns);
 const gap=tier==='full'?S[8]:S[4];
 const width=(L.content-gap*(columns-1))/columns,height=(areaHeight-gap*(rows-1))/rows;
 return {pageSize,total,tier,columns,rows,gap,width,height,top,areaHeight,
  contextY:top+areaHeight+S[8],
  inset:tier==='full'?S[8]:S[4],
  lineInset:tier==='full'?L.durationInset:S[4],
  mark:tier==='full'?3:2};
}
