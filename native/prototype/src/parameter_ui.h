#pragma once
#include "ui_primitives.h"
namespace moss {
inline constexpr int ParameterButtons[]={lab::Y,lab::B,lab::A,lab::X};
inline int parameterSlot(int button){for(int i=0;i<4;++i)if(ParameterButtons[i]==button)return i;return -1;}
inline void bankTabs(lab::Canvas &c,const char *const *labels,int count,int selected){ui::tabs(c,104,labels,count,selected);}
inline void parameterField(lab::Canvas &c,const lab::View &v,int col,const char *label,const char *value,const char *unit){
  int x=ui::Inset+col*152;bool held=(v.buttons.held&lab::bit(ParameterButtons[col]))!=0;
  if(held)ui::box(c,x-4,ui::Fields-4,152,52,0,lab::DIM,lab::PANEL);
  ui::text(c,x+7,ui::Fields,lab::names[ParameterButtons[col]],lab::DIM,ui::Small);
  ui::face(c,x,ui::Fields+20,col);
  const char *u=ui::unit(unit);if(!strcmp(value,"--")||!strcmp(value,"RESET")||!strcmp(value,"OFF")||!strcmp(value,"FULL"))u="";const char *val=!strcmp(value,"RESET")?"RST":value;
  int vw=ui::textWidth(val),uw=ui::textWidth(u,ui::Small),gap=u[0]&&u[0]!='%'?4:0;
  bool longValue=vw+uw+gap>120;
  ui::text(c,x+32,ui::Fields,label,lab::DIM,ui::Small,longValue?(120-uw-8)*7/72:11);
  ui::text(c,x+32,ui::Fields+24,val,lab::TEXT,ui::Body,10);
  if(longValue)ui::text(c,x+152-uw,ui::Fields,u,lab::DIM,ui::Small,10);
  else ui::text(c,x+32+vw+gap,ui::Fields+26,u,lab::DIM,ui::Small,11);
}
}
