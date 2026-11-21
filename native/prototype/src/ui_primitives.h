#pragma once
#include "../../../hardware lab/src/lab_ui.h"
namespace moss { namespace ui {
constexpr int Inset=16, Width=608, Footer=444, Tabs=336, Fields=376;
constexpr int Heading=21, Body=14, Small=12, Micro=8;
inline int textWidth(const char *s,int size=Body){return int(strlen(s))*size*6/7;}
// Rasterize the existing instrument alphabet at the approved four roles.
inline void text(lab::Canvas &c,int x,int y,const char *s,uint32_t color=lab::TEXT,int size=Body,int maxChars=80){
  for(int i=0;s[i] && i<maxChars;++i){
    char ch=s[i];if(ch>='a'&&ch<='z')ch-=32;
    const char *g=lab::glyph(ch);int left=x+i*size*6/7;
    if(ch=='^'){for(int d=0;d<3;++d){c.rect(left+4-d,y+2+d,d*2+1,1,color);c.rect(left+4-d,y+size-3-d,d*2+1,1,color);}continue;}
    for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(g[row*5+col]=='1'){
      int x0=left+col*size/7,x1=left+(col+1)*size/7;
      int y0=y+row*size/7,y1=y+(row+1)*size/7;
      c.rect(x0,y0,x1-x0,y1-y0,color);
    }
  }
}
inline uint32_t blend(uint32_t a,uint32_t b,int alpha){uint32_t v=0;for(int s=0;s<24;s+=8)v|=((((a>>s)&255)*(255-alpha)+((b>>s)&255)*alpha)/255)<<s;return v;}
inline void round(lab::Canvas &c,int x,int y,int w,int h,int r,uint32_t color){
  r=lab::clamp(r,0,lab::clamp(w<h?w/2:h/2,0,12));
  for(int row=0;row<h;++row){int dy=row<r?r-1-row:row>=h-r?row-(h-r):0,cut=0;
    if(row<r||row>=h-r)while(cut<r&&(r-cut-1)*(r-cut-1)+dy*dy>r*r)++cut;
    c.rect(x+cut,y+row,w-cut*2,1,color);
  }
}
inline void box(lab::Canvas &c,int x,int y,int w,int h,int r,uint32_t border,uint32_t fill){round(c,x,y,w,h,r,border);round(c,x+1,y+1,w-2,h-2,r>0?r-1:0,fill);}
inline void circle(lab::Canvas &c,int x,int y,int r,uint32_t ink,bool filled){for(int dy=-r;dy<=r;++dy)for(int dx=-r;dx<=r;++dx){int d=dx*dx+dy*dy;if(d<=r*r&&(filled||d>(r-1)*(r-1)))c.rect(x+dx,y+dy,1,1,ink);}}
inline void diamond(lab::Canvas &c,int x,int y,int r,uint32_t ink){for(int dy=-r;dy<=r;++dy){int dx=r-abs(dy);c.rect(x-dx,y+dy,1,1,ink);c.rect(x+dx,y+dy,1,1,ink);}}
inline void face(lab::Canvas &c,int x,int y,int col){const int xs[]={3,11,19,11},ys[]={11,19,11,3};for(int i=0;i<4;++i)circle(c,x+xs[i],y+ys[i],3,i==col?lab::TEXT:lab::DIM,i==col);}
inline void row(lab::Canvas &c,int y,bool selected,int height=48){if(selected)box(c,Inset,y,Width,height,0,lab::DIM,lab::PANEL);else c.rect(Inset,y+height-1,Width,1,lab::LINE);}
inline void tabs(lab::Canvas &c,int y,const char *const *labels,int count,int selected,bool primary=false,bool shoulders=true){
  if(count<=0)return;count=lab::clamp(count,0,16);selected=lab::clamp(selected,0,count-1);
  int widths[16]={},total=0,selectedLeft=0,size=primary?Body:Small;
  for(int i=0;i<count;++i){widths[i]=textWidth(labels[i],size)+24;if(i<selected)selectedLeft+=widths[i]+4;total+=widths[i]+(i?4:0);}
  const int left=76,right=564,available=right-left;
  int offset=total<=available?(available-total)/2:lab::clamp(available/2-selectedLeft-widths[selected]/2,available-total,0);
  uint32_t margins[24][152];
  for(int yy=0;yy<24;++yy)for(int xx=0;xx<76;++xx){margins[yy][xx]=c.pixels[(y+yy)*lab::W+xx];margins[yy][76+xx]=c.pixels[(y+yy)*lab::W+564+xx];}
  int x=left+offset;
  for(int i=0;i<count;++i){if(i==selected)round(c,x,y,widths[i],24,12,lab::TEXT);ui::text(c,x+12,y+(24-size)/2,labels[i],i==selected?lab::BG:lab::DIM,size);x+=widths[i]+4;}
  for(int yy=0;yy<24;++yy){for(int xx=0;xx<76;++xx){c.pixels[(y+yy)*lab::W+xx]=margins[yy][xx];c.pixels[(y+yy)*lab::W+564+xx]=margins[yy][76+xx];}
    if(total>available)for(int edge=0;edge<24;++edge){if(offset<0){auto &p=c.pixels[(y+yy)*lab::W+left+edge];p=blend(lab::BG,p,edge*255/24);}if(total+offset>available){auto &p=c.pixels[(y+yy)*lab::W+right-1-edge];p=blend(lab::BG,p,edge*255/24);}}
  }
  if(shoulders&&count>1){ui::text(c,16,y+6,primary?"L2":"L1",lab::DIM,Small);ui::text(c,56,y+6,"<",lab::DIM,Small);ui::text(c,576,y+6,">",lab::DIM,Small);ui::text(c,604,y+6,primary?"R2":"R1",lab::DIM,Small);}
}
inline const char *unit(const char *s){
  if(!strcmp(s,"PERCENT"))return "%";if(!strcmp(s,"STEPS"))return "STPS";
  if(!strcmp(s,"SEMITONES"))return "ST";if(!strcmp(s,"CENTS"))return "CT";
  if(!strcmp(s,"FRAMES"))return "FR";if(!strcmp(s,"GRAINS/S"))return "GR/S";
  if(strstr(s,"LOCK")||strstr(s,"TRACK")||strstr(s,"RESOLUTION")||strstr(s,"EVENT")||strstr(s,"WHOLE")||strstr(s,"MOVE SLOT")||strstr(s,"PER REPEAT"))return "";
  if(!strcmp(s,"MOD:CARRIER")||!strcmp(s,"DARK-BRIGHT")||!strcmp(s,"SYNC")||!strcmp(s,"NO NOTE")||!strcmp(s,"ONE SHOT"))return "";if(!strcmp(s,"TIMES"))return "X";if(!strcmp(s,"SEC"))return "S";return s;
}
}}
