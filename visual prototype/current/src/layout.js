// CSS owns the spacing scale. DOM views and canvas portraits share this geometry.
const root=document.documentElement,style=getComputedStyle(root);
const px=name=>parseFloat(style.getPropertyValue(name));
export const SPACE=Object.freeze(Object.fromEntries([4,8,12,16,24,32].map((size,i)=>[size,px(`--space-${i+1}`)])));
const width=640,height=480,inset=SPACE[16],content=width-inset*2;
const line=16,tabHeight=24,face=24,footerHeight=36;
const footerY=height-footerHeight,fieldBottom=footerY-SPACE[24];
const fieldHeight=line+SPACE[4]+face,fieldTop=fieldBottom-fieldHeight;
const parameterY=fieldTop-SPACE[16]-tabHeight,portraitBottom=parameterY-SPACE[16];
const headerBottom=48,mainTabsY=headerBottom+SPACE[8],mainRule=mainTabsY+tabHeight+SPACE[8];
const contentY=mainRule+SPACE[16],sectionY=contentY,sectionContentY=sectionY+tabHeight+SPACE[16];
export const LAYOUT=Object.freeze({width,height,inset,content,right:width-inset,line,tabHeight,face,
 durationInset:px('--duration-inset'),
 footerHeight,footerY,fieldHeight,fieldTop,fieldBottom,fieldWidth:content/4,fieldTextWidth:content/4-face-SPACE[8],
 parameterY,portraitBottom,headerBottom,mainTabsY,mainRule,contentY,sectionY,sectionContentY,
 voiceTitleY:contentY+line+SPACE[16],voiceRule:contentY+line+SPACE[16]+24+SPACE[16],
 voiceArtY:contentY+line+SPACE[16]+24+SPACE[16]+SPACE[16],
 rowHeight:line+SPACE[16]*2,compactRowHeight:line+SPACE[12]*2,rowGap:SPACE[8],
 metaY:footerY-SPACE[16]-line,slotWidth:(content-SPACE[8]*3)/4,cellWidth:(content-SPACE[8]*7)/8,
 tapeWaveY:sectionContentY+line+SPACE[16],tapeFineY:sectionContentY+line*2+SPACE[16]*2,
 tapeWaveBottom:portraitBottom,tapeDetailY:parameterY});
for(const [name,value] of Object.entries({
 'footer-height':footerHeight,'control-line':line,'face-size':face,'tab-height':tabHeight,
 'parameter-height':fieldHeight,'parameter-bottom':height-fieldBottom,
 'parameter-pages-bottom':height-parameterY-tabHeight,
 'main-tabs-top':mainTabsY,'section-tabs-top':sectionY,
}))root.style.setProperty(`--${name}`,`${value}px`);
