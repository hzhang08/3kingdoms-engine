#include "caocao/attack_controller.hpp"
#include "caocao/strike_application.hpp"
#include "caocao/experience.hpp"
#include <functional>
#include <stdexcept>
namespace caocao {
PhysicalAttack physical_attack(int actor_slot,int target_slot,const std::vector<StrikeTarget>& units,
 OriginalRng rng,std::array<int,2> camera,int tick,int width,int height,int priority_id,int beans,const Json& data){
 PhysicalAttack out;out.units=units;out.rng=rng;out.camera=camera;out.tick=tick;
 auto unit=[&](int slot)->StrikeTarget&{
  auto it=std::find_if(out.units.begin(),out.units.end(),[&](const auto& u){return u.battle.slot==slot;});
  if(it==out.units.end())throw std::runtime_error("physical attack refers to an absent unit");return *it;
 };
 auto face=[&](BattleUnit& b,const BattleUnit& target){
  b.direction=battle_direction(b,target);b.flags=(b.flags&~8)|(b.direction==1?8:0);
  out.events.push_back({{"kind","unit_state"},{"battle",b.json()}});
 };
 std::function<void(int,int)> attack=[&](int a_slot,int b_slot){
  auto& a=unit(a_slot);auto& b=unit(b_slot);
  int area=a.unit.job==16||a.unit.job==17?1:a.unit.job==43?4:equipment_effect(a.unit,43,data).value_or(0);
  if(area)throw std::runtime_error("area attack target/presentation sequence is not yet verified (0x4061fc)");
  if(a_slot<35)out.last_allied_actor=a.unit.id;
  uint32_t xp=0,weapon_xp=0;int round=0;StrikePlan plan;
  auto checkpoint=[&](const char* kind,bool critical){
   out.trace.push_back({{"kind",kind},{"actor",a_slot},{"targets",{b_slot}},{"round",round},{"depth",out.depth},
    {"rng",out.rng.state},{"hp_damage",plan.hp_damage},{"mp_damage",plan.mp_damage},{"armor_xp",plan.armor_xp},
    {"xp",xp},{"weapon_xp",weapon_xp},{"critical",critical},
    {"units",Json::array({{{"battle",a.battle.json()},{"unit",a.unit.json()}},{{"battle",b.battle.json()},{"unit",b.unit.json()}}})}});
  };
  for(;round<2;++round){
   plan=plan_physical_strike(a.battle,a.unit,a.terrain,{b},round,out.depth,priority_id,xp,weapon_xp,data,out.rng);
   xp=plan.xp;weapon_xp=plan.weapon_xp;checkpoint("planned",plan.critical);
   // Sprite preparation faces the target temporarily and then restores it.
   // The final restore also normalizes the horizontal-mirror bit.
   b.battle.flags=(b.battle.flags&~8)|(b.battle.direction==1?8:0);
   auto path=battle_attack_camera_path(out.camera[0],out.camera[1],{a.battle.x,a.battle.y},{b.battle.x,b.battle.y},width,height);
   if(!path.empty())out.camera=path.back();
   out.events.push_back({{"kind","camera"},{"tile",{out.camera[0]/48,out.camera[1]/48}}});
   int group=std::stoi(data.at("heroes").at(a.unit.id).at("raw").get<std::string>().substr(32,2),nullptr,16);
   if(auto message=physical_critical_speech(a.unit.id,group,plan.critical,out.rng)){
    out.events.push_back({{"kind","message"},{"index",*message}});
    out.events.push_back({{"kind","unit_redraw"},{"slot",a_slot}});
    auto speech_camera=battle_camera_path(out.camera[0],out.camera[1],a.battle.x,a.battle.y,width,height);
    if(!speech_camera.empty())out.camera=speech_camera.back();
    out.events.push_back({{"kind","camera"},{"tile",{out.camera[0]/48,out.camera[1]/48}}});
    out.events.push_back({{"kind","speech"},{"unit",a.unit.id},{"message",*message}});
   }
   auto animation=physical_attack_animation(a.battle,a.unit,b.battle,b.unit,plan.hp_damage[0],plan.mp_damage[0],plan.critical,out.camera[0]/48,out.camera[1]/48,out.tick,data);
   for(const auto& event:animation.events)out.events.push_back(event);
   a.battle=animation.actor;b.battle=animation.target;out.tick=animation.tick;checkpoint("presented",plan.critical);
   auto applied=apply_strike_target(a.battle,a.unit,b,plan.hp_damage[0],plan.mp_damage[0],plan.armor_xp[0],area,beans,data,out.rng);
   if(applied.pending_item)throw std::runtime_error("automatic item sequence is not yet verified (0x41dc43)");
   for(const auto& event:applied.requests){
    if(event.at("kind")=="equipment_level")throw std::runtime_error("equipment level presentation is not yet verified (0x4083ef)");
    out.events.push_back(event);
   }
   b=std::move(applied.target);checkpoint("applied",plan.critical);
   out.events.push_back({{"kind","unit_state"},{"battle",b.battle.json()}});
   out.events.push_back({{"kind","record_state"},{"record",b.unit.json()}});
   bool another=roll_another_strike(a.battle,a.unit,b.battle,b.unit,round,out.depth,data,out.rng);
   checkpoint("double_checked",false);
   if(!another)break;
  }
  out.events.push_back({{"kind","numbers"},{"target",a_slot},{"actor",b_slot},{"hp",0},{"mp",0},{"xp",xp},
   {"weapon_xp",weapon_xp},{"armor_xp",0},{"wait",true},{"target_before",a.battle.json()}});
  auto growth=set_character_experience(a.unit,uint32_t(a.unit.xp)+xp,data);
  auto equipment=add_equipment_experience(a.unit,0,weapon_xp,data);
  if(!growth.levels.empty()||!equipment.levels.empty())throw std::runtime_error("level-up presentation is not yet verified (0x4073b8/0x4083ef)");
  if(growth.refresh)out.events.push_back({{"kind","refresh"},{"unit",a.unit.id}});
  out.events.push_back({{"kind","record_state"},{"record",a.unit.json()}});checkpoint("experience_applied",false);
  std::vector<BattleUnit> board;for(const auto& u:out.units)board.push_back(u.battle);
  auto counter=plan_counterattack(a.battle,a.unit,b.battle,b.unit,out.depth,width,height,board,data);
  out.depth=counter.depth;
  if(counter.attack){face(b.battle,a.battle);attack(b_slot,a_slot);}
 };
 attack(actor_slot,target_slot);return out;
}
}
