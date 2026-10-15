#include "caocao/panel.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/tactics.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace caocao {
TerrainPanelInfo terrain_panel_info(int t){
 if(t<0||t>=30)throw std::runtime_error("unverified terrain panel type");
 bool blocked=t==5||t==6||t==11||t==12||t==14||t==15||t==17||t>=25;
 return {t>=18&&t<=22,blocked,{t<=2||t==8||t==16||(t>=18&&t<=24),(t>=7&&t<=10)||t==13,t==3||t==4,t<=1||t==3||t==4||(t>=7&&t<=10)||t==13}};
}
int terrain_grayscale_index(int index,const Json& ui){
 if(index<0||index>255)throw std::runtime_error("invalid palette index");if(!index)return 0;
 const auto& p=ui.at("combat_palette");int value=0;for(int n=0;n<3;++n)value+=p.at("quantized").at(index*3+n).get<int>();value=(value&255)/3;
 return p.at("lookup").at(value*0x111);
}
Json battle_terrain_panel(std::array<int,2> tile,std::array<int,2> camera,int terrain,const Json& ui){
 int rx=tile[0]-camera[0],ry=tile[1]-camera[1];if(rx<0||rx>=13||ry<0||ry>=8)throw std::runtime_error("terrain panel requires a focused viewport");
 // 0x45576d uses viewport WIDTH for both split points, including the Y split.
 int px=rx+(rx<6?1:-3),py=ry+(ry<6?0:-1),x=px*48,y=56+py*48;
 auto info=terrain_panel_info(terrain);Json out=Json::array();
 out.push_back({{"kind","tile_capture"},{"tile",tile},{"screen",{rx*48,56+ry*48}}});
 auto emit=[&](const char* kind,Json args){out.push_back({{"kind",kind},{"args",args}});};
 emit("window",{px,py,3,2});emit("terrain_tile",{x+8,y+8,48,48,0,0});
 auto text=[&](const std::string& str,int yy,int color){Json glyphs=Json::array();int xx=x+64;for(size_t n=0;n<str.size();){unsigned char c=str[n];size_t len=c<128?1:c<224?2:c<240?3:4;if(n+len>str.size())throw std::runtime_error("invalid UTF-8 terrain label");glyphs.push_back({xx,yy,str.substr(n,len)});xx+=c<128?8:16;n+=len;}out.push_back({{"kind","text"},{"text",str},{"x",x+64},{"y",yy},{"foreground",color},{"background",0},{"opaque",false},{"glyphs",glyphs}});};
 text(ui.at("terrain").at(terrain).at("text"),y+12,18);
 if(info.recovery)text(ui.at("terrain_labels").at("recovery").at("text"),y+40,53);
 else if(info.impassable_label)text(ui.at("terrain_labels").at("impassable").at("text"),y+40,21);
 for(int n=0;n<4;++n)out.push_back({{"kind","gray_icon"},{"args",{x+16+n*32,y+68,16,16,0,4}},{"icon_index",n}});
 for(int n=0;n<4;++n)if(info.tactics[n])emit("icon",{x+16+n*32,y+68,16,16,0x1a5c0+n*256,4});
 return out;
}
std::array<int,2> battle_panel_position(const BattleUnit& b,std::array<int,2> camera,const BattleUnit* other){
 int rx=b.x-camera[0],ry=b.y-camera[1];
 if(rx<0||rx>=13||ry<0||ry>=8)throw std::runtime_error("unit panel requires a focused viewport");
 int px=rx+(rx<6?1:-4),py=ry+(ry<4?0:-1);
 if(other){
  const int ox=(other->x-camera[0])&255,oy=(other->y-camera[1])&255;
  auto covers=[&](){return px<=ox&&ox<px+4&&py<=oy&&oy<py+2;};
  if(covers()){
   if(rx<6&&rx>=4)px-=5;
   else if(rx>=6&&13-rx>4)px+=5;
   while(covers()){
    if(ry<4){++py;if(py+2>=13)break;} // Original uses viewport width here.
    else {--py;if(py<=0)break;}
   }
  }
 }
 return {px,py};
}
namespace {
Json panel(const BattleUnit& b,const UnitRecord& u,const std::string& name,
 std::array<int,2> camera,int terrain,const Json& data,const Json& ui,
 const BattleUnit* actor,int damage,int chance,int tactic=-1,int tactic_type=-1){
 auto [px,py]=battle_panel_position(b,camera,actor);
 Json out=Json::array();
 auto emit=[&](const char* kind,const Json& args){out.push_back({{"kind",kind},{"args",args}});};
 emit("window",Json::array({px,py,4,2}));
 auto number=[](int n,int width){std::ostringstream s;s<<std::setw(width)<<n;return s.str();};
 auto label=[&](const char* key){return ui.at("labels").at(key).at("text").get<std::string>();};
 auto text=[&](const std::string& str,int x,int y,int foreground,int background,bool opaque){
  Json glyphs=Json::array();int cursor=x;
  for(size_t i=0;i<str.size();){unsigned char c=str[i];size_t len=c<128?1:c<224?2:c<240?3:4;
   if(i+len>str.size())throw std::runtime_error("invalid UTF-8 panel text");
   glyphs.push_back(Json::array({cursor,y,str.substr(i,len)}));cursor+=c<128?8:16;i+=len;
  }
  out.push_back({{"kind","text"},{"text",str},{"x",x},{"y",y},{"foreground",foreground},{"background",background},{"opaque",opaque},{"glyphs",glyphs}});return cursor;
 };
 int x=px*48+8,y=56+py*48+4;
 std::string shown;for(unsigned char c:name)if(c<'0'||c>'9')shown+=char(c);
 constexpr int name_background[]={42,173,172,172};
 if(b.side<0||b.side>3)throw std::runtime_error("invalid panel side");
 text(shown,x,y,b.side==0&&(b.flags&2)?24:18,name_background[b.side],true);
 int next=text(ui.at("classes").at(u.job).at("text"),x+72,y,18,0,false);
 text(" Lv"+number(u.level,2),next,y,18,0,false);
 auto bar=[&](int kind,int current,int maximum,int yy){
  if(maximum<=0)throw std::runtime_error("original unit panel divides by zero maximum");
  int bx=x+40,by=yy+8,width=(current*100/maximum)*128/100;
  emit("bar_background",Json::array({bx,by,128,8,0,31}));
  if(actor&&kind==0&&(tactic<0||tactic==33)){
   emit("pattern",Json::array({8,8,0}));
   if(width)emit("bar_fill",Json::array({bx,by,bx+width-1,by+7,2}));
   width=((current-damage)*100/maximum)*128/100;
  }
  emit("pattern",Json::array({8,8,kind?64:128}));
  if(width)emit("bar_fill",Json::array({bx,by,bx+width-1,by+7,2}));
  text(number(current,4)+" /"+number(maximum,4),bx+24,yy,18,31,true);
  emit("icon",Json::array({x+8,yy,24,24,kind?960:384,4}));
 };
 bar(0,b.hp,std::min(500,u.hp+equipment_bonus(u,5,data)),y+20);
 bar(1,b.mp,std::min(250,u.mp+equipment_bonus(u,6,data)),y+44);
 x+=8;y+=71;
 if(actor){
  const auto& labels=ui.at(tactic<0?"attack_labels":"tactic_labels");
  if(tactic<0||tactic==33){
   text(labels.at("damage").at("text"),x,y,36,31,false);
   text(number(damage,3),x+40,y,18,31,false);
  }else text(labels.at("types").at(tactic_type).at("text"),x,y,36,31,false);
  text(labels.at("hit").at("text"),x+88,y,67,31,false);
  text(number(chance,3)+"%",x+128,y,18,31,false);
  return out;
 }
 if(b.side==0){text(label("experience"),x,y,18,31,false);text(u.xp==255?label("maximum"):number(u.xp,3),x+40,y,18,31,false);}
 else text(label(b.slot<35?"allied":"enemy"),x,y,b.slot<35?36:53,31,false);
 next=text(ui.at("terrain").at(terrain).at("text"),x+80,y,83,31,false);
 text(number(battle_terrain_factor(u,terrain,data)*10,4)+"%",next,y,18,31,false);
 return out;
}
}
Json battle_unit_panel(const BattleUnit& b,const UnitRecord& u,const std::string& name,
 std::array<int,2> camera,int terrain,const Json& data,const Json& ui){
 return panel(b,u,name,camera,terrain,data,ui,nullptr,0,0);
}
Json battle_attack_panel(const BattleUnit& b,const UnitRecord& u,const std::string& name,
 const BattleUnit& a,const UnitRecord& au,std::array<int,2> camera,
 int terrain,int actor_terrain,const Json& data,const Json& ui){
 OriginalRng unused;
 int damage=physical_damage(a,au,b,u,actor_terrain,terrain,false,false,data,unused);
 int chance=physical_hit_chance(a,au,b,u,data);
 return panel(b,u,name,camera,terrain,data,ui,&a,damage,chance);
}
Json battle_tactic_panel(const BattleUnit& b,const UnitRecord& u,const std::string& name,
 const BattleUnit& a,const UnitRecord& au,std::array<int,2> camera,int tactic,
 const Json& data,const Json& ui,const Json& tactics){
 OriginalRng unused;
 int damage=tactic_damage(a,au,b,u,tactic,false,data,unused);
 int chance=tactic_hit_chance(a,au,b,u,tactic,data);
 return panel(b,u,name,camera,0,data,ui,&a,damage,chance,tactic,tactics.at(tactic).at(15));
}
Json battle_damage_panel(const BattleUnit& b,const UnitRecord& u,const std::string& name,
 const BattleUnit* actor,std::array<int,2> camera,int32_t hp_loss,int32_t mp_loss,
 bool wait,const Json& data,const Json& ui){
 Json out=Json::array();if(!hp_loss&&!mp_loss)return out;
 if(b.side==0)throw std::runtime_error("party result/experience panel is not yet verified (0x450036)");
 auto [px,py]=battle_panel_position(b,camera,actor);int x=px*48+8,y=56+py*48+8;
 auto emit=[&](const char* kind,const Json& args){out.push_back({{"kind",kind},{"args",args}});};
 auto number=[](uint32_t n,int width){std::ostringstream s;s<<std::setw(width)<<n;return s.str();};
 auto text=[&](const std::string& str,int xx,int yy,int background,bool opaque){
  Json glyphs=Json::array();int cursor=xx;
  for(size_t i=0;i<str.size();){unsigned char c=str[i];size_t len=c<128?1:c<224?2:c<240?3:4;
   glyphs.push_back({cursor,yy,str.substr(i,len)});cursor+=c<128?8:16;i+=len;
  }
  out.push_back({{"kind","text"},{"text",str},{"x",xx},{"y",yy},{"foreground",18},{"background",background},{"opaque",opaque},{"glyphs",glyphs}});
 };
 emit("window",{px,py,4,2});out.push_back({{"kind","cursor"},{"tile",{b.x,b.y}}});
 std::string shown;for(unsigned char c:name)if(c<'0'||c>'9')shown+=char(c);
 text(shown,x,y,b.side==1?173:b.side==2||b.side==3?172:31,true);
 text("Lv"+number(u.level,2),x+72,y,31,false);text(ui.at("classes").at(u.job).at("text"),x+112,y,31,false);
 emit("icon",{x+8,y+32,24,24,384,4});emit("icon",{x+8,y+56,24,24,960,4});
 auto bar=[&](int kind,uint32_t current){
  int maximum=kind?std::min(250,u.mp+equipment_bonus(u,6,data)):std::min(500,u.hp+equipment_bonus(u,5,data));
  if(maximum<=0)throw std::runtime_error("original damage panel divides by zero maximum");
  int bx=x+40,yy=y+(kind?56:32);uint32_t width=((current*100u/maximum)<<7)/100u;
  emit("bar_background",{bx,yy+8,128,8,0,31});emit("pattern",{8,8,kind?64:128});
  if(width)emit("bar_fill",{bx,yy+8,uint32_t(bx)+width-1,yy+15,2});
  text(number(current,4)+" /"+number(maximum,4),bx+24,yy,31,true);
 };
 bar(0,b.hp);bar(1,b.mp);
 for(int n=4;n>=0;--n){
  out.push_back({{"kind","wait"},{"ticks",1}});
  bar(0,uint32_t(b.hp)-uint32_t(hp_loss)+uint32_t(hp_loss/4*n));
  bar(1,uint32_t(b.mp)-uint32_t(mp_loss)+uint32_t(mp_loss/4*n));
 }
 out.push_back({{"kind","wait"},{"ticks",wait?8:4}});
 out.push_back({{"kind","clear_cursor"}});out.push_back({{"kind","clear_panel"}});return out;
}
}
