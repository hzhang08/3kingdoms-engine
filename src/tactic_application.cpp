#include "caocao/tactic_application.hpp"
#include "caocao/experience.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
namespace {
void focus(const BattleUnit& b,std::array<int,2>& camera,int width,int height,Json& events){
 int x=camera[0]/48,y=camera[1]/48;
 if(b.x>=x&&b.x<x+13&&b.y>=y&&b.y<y+8)return;
 auto path=battle_camera_path(camera[0],camera[1],b.x,b.y,width,height);
 if(!path.empty()){
  camera=path.back();events.push_back({{"kind","camera"},{"tile",{camera[0]/48,camera[1]/48}}});
 }
}
void equipment_levels(UnitRecord& u,int slot,uint32_t gain,const Json& data,Json& events){
 for(int level:add_equipment_experience(u,slot,gain,data).levels)
  events.push_back({{"kind","equipment_level"},{"unit",u.id},{"slot",slot},{"level",level}});
}
}
PoisonApplication apply_poison_target(const BattleUnit& actor,const StrikeTarget& target,
 uint32_t damage,int effect,uint32_t armor_xp,int area,int beans,
 std::array<int,2> camera,int tick,int width,int height,const Json& data){
 if((effect!=0&&effect!=112)||target.battle.id!=target.unit.id||area<0||area>255||beans<0||beans>255||tick<0||tick>255)
  throw std::invalid_argument("unverified poison application input (0x4224a8)");
 PoisonApplication out{target,camera,tick};auto& b=out.target.battle;auto& u=out.target.unit;auto& ev=out.requests;
 if(tick%4<2)for(int n=0;n<2;++n){out.tick=(out.tick+1)%8;ev.push_back({{"kind","attack_tick"},{"tick",out.tick}});}
 ev.push_back({{"kind","redraw"},{"args",Json::array()}});
 focus(b,out.camera,width,height,ev);
 int resource=-1;
 // The executable compares the planned loss as signed, before clamping it.
 if(effect==112&&static_cast<int32_t>(damage)<b.hp){
  b.status|=16;ev.push_back({{"kind","refresh"},{"unit",b.id}});resource=49;
 }else if(effect==0&&damage==0)resource=45;
 if(resource>=0){
  ev.push_back({{"kind","effect_load"},{"resource",resource}});
  ev.push_back({{"kind","effect_target"},{"slot",b.slot}});
  ev.push_back({{"kind","effect_play"}});ev.push_back({{"kind","effect_clear"}});
  ev.push_back({{"kind","overlay"},{"x",b.x*48-out.camera[0]},{"y",b.y*48-out.camera[1]+56},{"slot",b.slot}});
 }
 if(damage){
  uint32_t loss=static_cast<int32_t>(damage)>b.hp?uint32_t(b.hp):damage;
  ev.push_back({{"kind","numbers"},{"target",b.slot},{"actor",actor.slot},{"hp",loss},{"mp",0},{"xp",0},
   {"weapon_xp",0},{"armor_xp",armor_xp},{"wait",area==0},{"target_before",b.json()}});
  b.hp=std::min(uint32_t(b.hp)-loss,uint32_t(std::min(500,u.hp+equipment_bonus(u,5,data))));
  b.mp=std::min(b.mp,std::min(250,u.mp+equipment_bonus(u,6,data)));
  if(static_cast<int32_t>(loss)>0&&b.hp>0&&equipment_effect(u,62,data)&&(b.side!=0||beans>0)){
   out.pending_item=true;ev.push_back({{"kind","automatic_item"},{"item",87},{"target",b.slot},{"argument",0},{"free",b.side!=0}});return out;
  }
 }else if(resource>=0)ev.push_back({{"kind","wait"},{"ticks",8}});
 equipment_levels(u,1,armor_xp,data,ev);
 ev.push_back({{"kind","refresh"},{"unit",b.id}});return out;
}
PoisonFinalization finalize_enemy_poison(const StrikeTarget& actor,const BattleUnit& target,
 bool success,uint32_t xp,uint32_t weapon_xp,std::array<int,2> camera,
 int width,int height,const Json& data,const Json& tactics){
 if(actor.battle.id!=actor.unit.id)throw std::invalid_argument("invalid poison caster");
 PoisonFinalization out{actor,camera};if(!success)return out;
 auto& b=out.actor.battle;auto& u=out.actor.unit;auto& ev=out.requests;
 if(u.id<26)throw std::runtime_error("party tactic finalization/level animation is not yet verified (0x422f0d)");
 int cost=tactics.at(33).at(14);
 if(auto divisor=equipment_effect(u,49,data)){
  if(!*divisor)throw std::runtime_error("original tactic MP discount divides by zero");
  cost/=*divisor;
 }
 int mp=std::max(0,b.mp-cost);focus(b,out.camera,width,height,ev);
 ev.push_back({{"kind","numbers"},{"target",b.slot},{"actor",target.slot},{"hp",0},{"mp",cost},{"xp",xp},
  {"weapon_xp",weapon_xp},{"armor_xp",0},{"wait",true},{"target_before",b.json()}});
 b.hp=std::min(b.hp,std::min(500,u.hp+equipment_bonus(u,5,data)));
 b.mp=std::min(mp,std::min(250,u.mp+equipment_bonus(u,6,data)));
 // Original character XP setter rejects IDs >=26; weapon XP still executes.
 equipment_levels(u,0,weapon_xp,data,ev);ev.push_back({{"kind","refresh"},{"unit",u.id}});return out;
}
}
