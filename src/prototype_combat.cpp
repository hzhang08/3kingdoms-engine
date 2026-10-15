#include "caocao/prototype.hpp"
#include "caocao/attack_animation.hpp"
#include "caocao/experience.hpp"
#include "caocao/range.hpp"
#include "caocao/strike_application.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace caocao {
namespace {
int maximum_hp(const UnitRecord& r,const Json& data){return std::min(500,r.hp+equipment_bonus(r,5,data));}
std::string speech_text(const Json& texts,int index){
 if(!texts.is_array())return {};
 for(const auto& t:texts)if(t.at("id")==index){
  auto s=t.at("text").get<std::string>();
  for(size_t p=0;(p=s.find("\\n",p))!=std::string::npos;)s.replace(p,2," ");
  return s;
 }
 return {};
}
}

// 0x43ac44..0x43acfa after the primary controller. Its preceding counter gate
// sets the preferred slot to the actor (0x40642f), which cannot be an opponent.
// The range selector therefore returns its last eligible ordered target. This
// dispatches one additional controller, not a recursive chain of pursuit kills.
int prototype_pursuit_target(const BattleUnit& actor,const UnitRecord& record,const BattleUnit& target,
 int width,int height,const std::vector<BattleUnit>& board,const Json& data){
 if(target.hp!=0||!equipment_effect(record,46,data))return 255;
 return battle_range(actor,{actor.x,actor.y},battle_attack_range(record,data),0,false,actor.slot,width,height,
  {std::vector<int>(width*height,255),std::vector<int>(width*height,255),std::vector<int>(width*height)},board).selected;
}

// Deliberately separate from the original-script Engine's fail-closed renderer.
// Numeric planning/application, critical-speech RNG, double and counter order
// follow 0x4064f1 and original_attack_controller.json. This full-map frontend
// uses notices for speech/XP/level windows and the verified no-animation XP
// setter (0x408950): record growth changes maximum HP, not current battle HP.
// The JSON boundary lets tests replay the original oracle's exact input records
// without weakening Prototype's user-save validation.
Json resolve_prototype_combat(const Json& state,int actor,int target,const Json& terrain,
 const Json& data,const Json& texts){
 Json next=state,events=Json::array(),trace=Json::array(),strikes=Json::array(),limitations=Json::array();
 std::vector<StrikeTarget> units;
 const int width=terrain.at("width"),height=terrain.at("height");
 for(const auto& row:state.at("units")){
  auto b=BattleUnit::from_json(row.at("battle"));
  units.push_back({b,UnitRecord::from_json(row.at("record")),terrain.at("cells").at(b.y*width+b.x)});
 }
 auto unit=[&](int id)->StrikeTarget&{
  auto i=std::find_if(units.begin(),units.end(),[&](const auto& u){return u.battle.id==id;});
  if(i==units.end())throw std::runtime_error("physical attack refers to an absent unit");
  return *i;
 };
 auto name=[&](int id){return state.at("units").at(std::to_string(id)).at("name").get<std::string>();};
 auto wait=[&](int ticks){events.push_back({{"kind","wait"},{"ticks",ticks}});};
 auto notice=[&](int id,const std::string& text,int other=-1){
  events.push_back({{"kind","notice"},{"actor",id},{"target",other},{"text",text}});
 };
 auto battle_event=[&](const StrikeTarget& u){events.push_back({{"kind","unit_state"},{"actor",u.battle.id},{"battle",u.battle.json()}});};
 auto record_event=[&](const StrikeTarget& u){events.push_back({{"kind","record_state"},{"actor",u.battle.id},{"record",u.unit.json()},{"max_hp",maximum_hp(u.unit,data)}});};
 auto gear_notice=[&](const StrikeTarget& u,int slot,int level){
  notice(u.battle.id,name(u.battle.id)+"的"+data.at("items").at(u.unit.equipment[slot][0]).at("name").get<std::string>()+"升至 "+std::to_string(level)+" 级");
  wait(20);events.push_back({{"kind","notice_clear"}});
 };
 if(actor==target||unit(actor).battle.hp<=0||unit(target).battle.hp<=0||
    (unit(actor).battle.side==2)==(unit(target).battle.side==2))
  throw std::runtime_error("invalid physical attack participants");
 OriginalRng rng{state.at("rng").get<uint32_t>()};int depth=0,tick=0;
 std::function<void(int,int)> attack=[&](int aid,int bid){
  auto& a=unit(aid);auto& b=unit(bid);
  int area=a.unit.job==16||a.unit.job==17?1:a.unit.job==43?4:equipment_effect(a.unit,43,data).value_or(0);
  if(area)throw std::runtime_error("area physical attack requires original target assembly (0x4061fc)");
  uint32_t xp=0,weapon_xp=0;int round=0;StrikePlan plan;
  auto checkpoint=[&](const char* kind,bool critical){
   trace.push_back({{"kind",kind},{"actor",a.battle.slot},{"targets",{b.battle.slot}},{"round",round},{"depth",depth},
    {"rng",rng.state},{"hp_damage",plan.hp_damage},{"mp_damage",plan.mp_damage},{"armor_xp",plan.armor_xp},
    {"xp",xp},{"weapon_xp",weapon_xp},{"critical",critical},
    {"units",Json::array({{{"battle",a.battle.json()},{"unit",a.unit.json()}},{{"battle",b.battle.json()},{"unit",b.unit.json()}}})}});
  };
  for(;round<2;++round){
   plan=plan_physical_strike(a.battle,a.unit,a.terrain,{b},round,depth,0,xp,weapon_xp,data,rng);
   xp=plan.xp;weapon_xp=plan.weapon_xp;checkpoint("planned",plan.critical);
   b.battle.flags=(b.battle.flags&~8)|(b.battle.direction==1?8:0);
   battle_event(a);battle_event(b);
   int group=std::stoi(data.at("heroes").at(a.unit.id).at("raw").get<std::string>().substr(32,2),nullptr,16);
   auto speech=physical_critical_speech(a.unit.id,group,plan.critical,rng);
   std::string action=depth?"反击":round?"连击":"攻击";
   notice(aid,name(aid)+" "+action+" "+name(bid)+(plan.critical?"：会心一击！":""),bid);
   if(speech){
    auto text=speech_text(texts,*speech);
    if(!text.empty())events.back()["text"]=name(aid)+"："+text;
    events.back()["source_message"]=*speech;wait(22);
   }else if(depth||round||plan.critical)wait(12);
   auto animation=physical_attack_animation(a.battle,a.unit,b.battle,b.unit,plan.hp_damage[0],plan.mp_damage[0],plan.critical,0,0,tick,data);
   for(const auto& e:animation.events)events.push_back(e);
   a.battle=animation.actor;b.battle=animation.target;tick=animation.tick;checkpoint("presented",plan.critical);
   auto applied=apply_strike_target(a.battle,a.unit,b,plan.hp_damage[0],plan.mp_damage[0],plan.armor_xp[0],area,0,data,rng);
   // None of the four shipped rosters equips the automatic-bean treasure.
   // Its success/presentation controller is still an explicit unsupported path.
   if(applied.pending_item)throw std::runtime_error("automatic healing item controller requires original evidence (0x41dc43)");
   b=std::move(applied.target);checkpoint("applied",plan.critical);
   battle_event(a);battle_event(b);record_event(b);
   if(b.battle.hp==0)events.push_back({{"kind","pose_clear"},{"slot",b.battle.slot}});
   strikes.push_back({{"actor",aid},{"target",bid},{"round",round},{"counter_depth",depth},{"critical",plan.critical},
    {"hp_damage",plan.hp_damage[0]},{"mp_damage",plan.mp_damage[0]},{"armor_xp",plan.armor_xp[0]}});
   for(const auto& e:applied.requests)if(e.at("kind")=="equipment_level")gear_notice(b,e.at("slot"),e.at("level"));
   bool another=roll_another_strike(a.battle,a.unit,b.battle,b.unit,round,depth,data,rng);
   checkpoint("double_checked",false);
   if(!another)break;
  }
  auto growth=set_character_experience(a.unit,uint32_t(a.unit.xp)+xp,data);
  auto equipment=add_equipment_experience(a.unit,0,weapon_xp,data);
  record_event(a);checkpoint("experience_applied",false);
  if(xp||weapon_xp){
   notice(aid,name(aid)+" 经验 +"+std::to_string(xp)+"　武器经验 +"+std::to_string(weapon_xp));wait(18);
  }
  for(int level:growth.levels){notice(aid,name(aid)+"升至 "+std::to_string(level)+" 级！");wait(25);}
  for(int level:equipment.levels)gear_notice(a,0,level);
  events.push_back({{"kind","notice_clear"}});
  std::vector<BattleUnit> board;for(const auto& u:units)board.push_back(u.battle);
  auto counter=plan_counterattack(a.battle,a.unit,b.battle,b.unit,depth,width,height,board,data);depth=counter.depth;
  if(counter.attack){
   b.battle.direction=battle_direction(b.battle,a.battle);
   b.battle.flags=(b.battle.flags&~8)|(b.battle.direction==1?8:0);battle_event(b);attack(bid,aid);
  }
 };
 attack(actor,target);
 std::vector<BattleUnit> board;for(const auto& u:units)board.push_back(u.battle);
 int pursuit=prototype_pursuit_target(unit(actor).battle,unit(actor).unit,unit(target).battle,width,height,board,data);
 if(pursuit!=255){
  auto it=std::find_if(units.begin(),units.end(),[&](const auto& u){return u.battle.slot==pursuit;});
  if(it==units.end())throw std::runtime_error("pursuit target disappeared");
  auto& a=unit(actor);a.battle.direction=battle_direction(a.battle,it->battle);
  a.battle.flags=(a.battle.flags&~8)|(a.battle.direction==1?8:0);battle_event(a);
  notice(actor,name(actor)+"发动方天画戟追击！",it->battle.id);wait(20);
  attack(actor,it->battle.id);
 }
 for(const auto& u:units){auto& row=next["units"][std::to_string(u.battle.id)];row["battle"]=u.battle.json();row["record"]=u.unit.json();row["max_hp"]=maximum_hp(u.unit,data);}
 next["rng"]=rng.state;
 return {{"state",std::move(next)},{"events",std::move(events)},{"trace",std::move(trace)},
  {"strikes",std::move(strikes)},{"limitations",std::move(limitations)},{"tick",tick},{"depth",depth}};
}

void Prototype::attack_revised(int id,int target){
 if(!attackable(id,target))throw std::runtime_error("敌军不在攻击范围内。");
 Prototype candidate=*this;
 auto before=state_;before["selected"]=-1;before["undo"]=nullptr;
 auto a=BattleUnit::from_json(before.at("units").at(std::to_string(id)).at("battle"));
 auto b=BattleUnit::from_json(before.at("units").at(std::to_string(target)).at("battle"));
 a.direction=battle_direction(a,b);a.flags=(a.flags&~8)|(a.direction==1?8:0);
 before["units"][std::to_string(id)]["battle"]=a.json();
 auto result=resolve_prototype_combat(before,id,target,terrain_,data_,combat_text_);
 candidate.state_=result.at("state");
 // The prototype stores action completion, while facing is a separate field.
 // Temporary mirror/wounded-pose bits stay inside presentation and oracle traces.
 for(auto& u:candidate.state_["units"]){auto& live=u["battle"];live["flags"]=live.at("flags").get<int>()&~(8|32);if(live.at("lifecycle")==2&&live.at("hp")==0)live["lifecycle"]=3;}
 int damage=0;for(const auto& strike:result.at("strikes"))if(strike.at("actor")==id&&strike.at("target")==target)damage+=strike.at("hp_damage").get<int>();
 candidate.attack_={{"kind","physical"},{"actor",id},{"target",target},{"damage",damage},{"before",std::move(before)},
  {"events",result.at("events")},{"strikes",result.at("strikes")},{"limitations",result.at("limitations")},{"combat_trace",result.at("trace")}};
 candidate.state_["message"]=state_.at("units").at(std::to_string(id)).at("name").get<std::string>()+"攻击结束，共 "+std::to_string(damage)+" 点伤害。";
 candidate.finish(id);
 *this=std::move(candidate);
}
}
