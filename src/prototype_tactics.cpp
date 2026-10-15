#include "caocao/prototype.hpp"
#include "caocao/tactics.hpp"
#include "caocao/panel.hpp"
#include "caocao/range.hpp"
#include "caocao/recovery.hpp"
#include <algorithm>
namespace caocao {
namespace {
BattleUnit bunit(const Json& u){return BattleUnit::from_json(u.at("battle"));}
UnitRecord record(const Json& u){return UnitRecord::from_json(u.at("record"));}
}
void Prototype::enable_tactics(){
 if(!balanced_||legacy_)return;
 if(tactics_enabled_){new_tactics_=true;return;}
 if(spell_catalog_.is_null())throw std::runtime_error("missing verified tactic assets");
 tactics_enabled_=new_tactics_=true;clear_enemy_choice();request_cache_key_.clear();request_cache_=nullptr;
}
Json Prototype::spells(int id)const{
 if(revised_rules_)return extended_spells(id);
 Json out=Json::array();if(!tactics_enabled_||id<0||!state_.at("units").contains(std::to_string(id)))return out;
 const auto& u=state_.at("units").at(std::to_string(id));auto b=bunit(u);auto r=record(u);
 if(b.hp<=0||(b.status&4))return out;
 for(const auto& t:spell_catalog_.at("tactics")){
  const auto& fields=t.at("fields");int learned=fields.at(17+r.job),cost=fields.at(14);
  if(!learned||learned>r.level||b.mp<cost)continue;
  if(auto divisor=equipment_effect(r,49,data_)){if(*divisor<=0)throw std::runtime_error("invalid MP discount");cost/=*divisor;}
  out.push_back({{"id",t.at("id")},{"name",t.at("name")},{"mp_cost",cost},{"range",fields.at(12)}});
 }
 return out;
}
Json Prototype::spell_options(int id,std::array<int,2> position)const{
 if(revised_rules_)return extended_spell_options(id,position);
 Json out=Json::array();auto available=spells(id);if(available.empty())return out;
 auto a=bunit(state_.at("units").at(std::to_string(id)));auto ar=record(state_.at("units").at(std::to_string(id)));a.x=position[0];a.y=position[1];
 for(const auto& t:available){int spell=t.at("id");auto range=battle_range(a,position,t.at("range"),128,false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},roster());
  for(const auto& u:state_.at("units")){auto b=bunit(u);if(b.hp<=0||(b.side==2)==(a.side==2)||std::find(range.targets.begin(),range.targets.end(),b.slot)==range.targets.end())continue;
   if(!tactic_target_usable(a,b,spell))continue;
   int terrain=terrain_.at("cells").at(b.y*map_width()+b.x);
   if(spell!=33){int group=spell==0?0:spell==5?1:spell==15?2:3;if(!terrain_panel_info(terrain).tactics[group])continue;}
   OriginalRng unused;auto br=record(u);int hit=tactic_hit_chance(a,ar,b,br,spell,data_);int damage=tactic_damage(a,ar,b,br,spell,false,data_,unused);
   out.push_back({{"kind","tactic"},{"x",a.x},{"y",a.y},{"target",b.id},{"tactic",spell},{"name",t.at("name")},{"mp_cost",t.at("mp_cost")},{"hit_percent",hit},{"damage_on_hit",damage},{"poison_percent",spell==33?hit:0}});
  }
 }
 return out;
}
void Prototype::cast(int tactic,int target){
 if(state_.at("phase")!="player"||has_dialogue()||(revised_rules_&&!story_choices().empty()))throw std::runtime_error("现在不能下达命令。");
 int id=state_.at("selected");if(id<0)throw std::runtime_error("请先选择我军部队。");
 auto b=bunit(state_.at("units").at(std::to_string(id)));if(b.hp<=0||b.side==2||(b.flags&2)||(revised_rules_&&(b.side!=0||b.lifecycle!=2||(b.status&8))))throw std::runtime_error("这支部队已行动。");
 environment_=nullptr;spell_attack(id,tactic,target);
}
void Prototype::spell_attack(int id,int tactic,int target){
 if(revised_rules_){extended_spell_attack(id,tactic,target);return;}
 auto a=bunit(state_.at("units").at(std::to_string(id)));auto options=spell_options(id,{a.x,a.y});Json choice=nullptr;
 for(const auto& o:options)if(o.at("tactic")==tactic&&o.at("target")==target){choice=o;break;}
 if(choice.is_null())throw std::runtime_error("策略目标、地形或策略值无效。");
 auto b=bunit(state_.at("units").at(std::to_string(target)));auto ar=record(state_.at("units").at(std::to_string(id))),br=record(state_.at("units").at(std::to_string(target)));
 OriginalRng rng{state_.at("rng").get<uint32_t>()};bool poison=tactic==33&&rng.percent(tactic_hit_chance(a,ar,b,br,tactic,data_));
 int damage=tactic_damage(a,ar,b,br,tactic,true,data_,rng);poison=poison&&damage<b.hp;
 auto visual=state_;visual["selected"]=-1;visual["undo"]=nullptr;int direction=battle_direction(a,b);a.direction=direction;
 Json events=Json::array();auto wait=[&](int ticks){events.push_back({{"kind","wait"},{"ticks",ticks}});};
 wait(8);events.push_back({{"kind","sound"},{"id",8}});
 events.push_back({{"kind","frame"},{"x",a.x*48-8},{"y",a.y*48+48},{"width",64},{"height",64},{"slot",a.slot},{"archive","UNIT_ATK.E5"},{"sprite",battle_sprite(a.id,ar.job,a.side)},{"frame",direction==0?4:direction==2?0:8},{"mirror",direction==1},{"effects",Json::array()}});
 events.push_back({{"kind","sound"},{"id",39}});wait(20);
 events.push_back({{"kind","overlay"},{"slot",a.slot},{"x",a.x*48},{"y",a.y*48+56}});
 const auto& effect=spell_catalog_.at("effects").at(std::to_string(tactic));int w=effect.at("width"),h=effect.at("height");bool reacted=false;
 events.push_back({{"kind","sound"},{"id",effect.at("sound")}});
 for(const auto& f:effect.at("frames")){
  if(f.at("callback")==1){reacted=true;
   events.push_back({{"kind","frame"},{"x",b.x*48},{"y",b.y*48+56},{"width",48},{"height",48},{"slot",b.slot},{"archive","UNIT_SPC.E5"},{"sprite",battle_sprite(b.id,br.job,b.side)},{"frame",poison?3:std::array<int,4>{1,2,0,2}[b.direction]},{"mirror",!poison&&b.direction==1},{"effects",Json::array()}});
   events.push_back({{"kind","number"},{"slot",b.slot},{"value",damage}});
  }
  events.push_back({{"kind","spell_effect"},{"resource",effect.at("resource")},{"image",f.at("image")},{"weight",f.at("weight")},{"amount",f.at("amount")},{"x",b.x*48+24-w/2+f.at("dx").get<int>()},{"y",b.y*48+80-h/2+f.at("dy").get<int>()},{"width",w},{"height",h}});wait(effect.at("ticks"));
 }
 if(!reacted)throw std::runtime_error("spell resource has no hit callback");
 events.push_back({{"kind","spell_clear"}});wait(10);
 attack_={{"kind","tactic"},{"actor",id},{"target",target},{"tactic",tactic},{"name",choice.at("name")},{"damage",damage},{"poison",poison},{"mp_cost",choice.at("mp_cost")},{"before",visual},{"events",events}};
 a.mp-=choice.at("mp_cost").get<int>();b.hp-=damage;if(poison)b.status|=16;if(!b.hp)b.lifecycle=3;
 state_["units"][std::to_string(id)]["battle"]=a.json();state_["units"][std::to_string(target)]["battle"]=b.json();state_["rng"]=rng.state;
 state_["message"]=state_.at("units").at(std::to_string(id)).at("name").get<std::string>()+"使用"+choice.at("name").get<std::string>()+"："+std::to_string(damage)+"点伤害。"+(poison?"中毒。":"");finish(id);
}
void Prototype::poison_phase(bool enemy){
 if(!tactics_enabled_)return;OriginalRng rng{state_.at("rng").get<uint32_t>()};auto board=roster();std::vector<UnitRecord> records;for(const auto& u:state_.at("units"))records.push_back(record(u));
 for(auto& u:state_["units"]){auto b=bunit(u);if(b.hp<=0||!(b.status&16)||(b.side==2)!=enemy)continue;
  auto r=record(u);auto recovery=b;recovery.status=16;recover_battle_status(recovery,r,board,records,rng,data_);b.status=(b.status&~16)|(recovery.status&16);b.hp-=battle_poison_loss(b,r,data_);u["battle"]=b.json();
 }
 state_["rng"]=rng.state;
}
}
