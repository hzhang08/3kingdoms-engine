#include "caocao/battle_events.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace caocao {
bool opening_battle_condition(const Json& i,BattleEventState& s){
 const int op=i.at("opcode");const auto& a=i.at("args");auto v=[&](int n){return a.at(n).at("value").get<int>();};
 auto by_id=[&](int id)->const BattleUnit*{const BattleUnit* found=nullptr;for(const auto& b:s.units)if(b.id==id&&b.lifecycle!=0&&(!found||b.slot<found->slot))found=&b;return found;};
 auto compare=[](uint32_t actual,uint32_t target,int mode){return mode==0?actual>=target:mode==1?actual<target:actual==target;};
 switch(op){
 case 2:return true;
 case 5:
  for(int n:a.at(0).at("value"))if(!s.variables.at(n))return false;
  for(int n:a.at(1).at("value"))if(s.variables.at(n))return false;
  return true;
 case 63:return v(1)==0?s.turn>=v(0):v(1)==1?s.turn<v(0):s.turn==v(0);
 case 64:return s.phase==(v(0)&255);
 case 37:{
  int id=v(0),x=v(1)&255,y=v(2)&255;if(x>=20||y>=20)return false;
  for(const auto& b:s.units)if(b.lifecycle==2&&b.hp>0&&b.x==x&&b.y==y)
   return id==1024||(id==1025&&b.slot<35)||(id==1026&&b.slot>=35)||id==b.id;
  return false;
 }
 case 46:{
  if(v(2)!=1||v(0)<0||v(0)>=512||v(1)<0||v(1)>=512)throw std::runtime_error("unverified adjacency condition form");
  auto x=by_id(v(0)),y=by_id(v(1));
  return x&&y&&x->lifecycle==2&&y->lifecycle==2&&x->hp>0&&y->hp>0&&
   std::max(std::abs(x->x-y->x),std::abs(x->y-y->y))==1;
 }
 case 54:{
  if(v(0)<0||v(0)>=512||v(1)!=7)throw std::runtime_error("unverified unit-stat condition form");
  auto b=by_id(v(0));return b&&compare(b->hp,static_cast<uint32_t>(v(2)),v(3));
 }
 case 66:{
  // S_00 config has no nominated enemy; the default checks visible side 2/3.
  if(s.outcome==0){bool remaining=false;for(const auto& b:s.units)if(b.lifecycle==2&&(b.side==2||b.side==3))remaining=true;if(!remaining)s.victory=true;}
  return s.victory;
 }
 case 67:{
  if(s.outcome==0){
   bool nominated_dead=false;for(const auto& b:s.units)if(b.slot<=35&&b.id==0&&(b.lifecycle==3||b.lifecycle==4))nominated_dead=true;
   const BattleUnit* first=nullptr;for(const auto& b:s.units)if(b.slot==0)first=&b;
   bool remaining=false;
   if(!first||first->id==0){for(const auto& b:s.units)if(b.slot<=15&&(b.lifecycle==1||b.lifecycle==2))remaining=true;}
   else{for(const auto& b:s.units)if(b.lifecycle==2&&b.side==0)remaining=true;}
   if(nominated_dead||!remaining)s.defeat=true;
  }
  return s.defeat;
 }
 default:throw std::runtime_error("unverified opening event condition");
 }
}
int select_opening_event(const std::vector<Program>& programs,BattleEventState& s){
 int selected=-1;
 for(size_t n=0;n<programs.size();++n){const auto& p=programs[n];if(p.script!="S_00"||p.section!=1)continue;
  bool eligible=true;for(const auto& i:p.conditions){bool pass=opening_battle_condition(i,s);eligible=pass&&eligible;}
  if(eligible)selected=static_cast<int>(n);
 }
 return selected;
}
}
