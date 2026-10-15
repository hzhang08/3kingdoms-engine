#include "caocao/prototype.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/range.hpp"
#include <algorithm>
#include <set>
#include <tuple>
namespace caocao {
int Prototype::next_enemy_id()const{
 if(state_.at("phase")!="enemy"||has_dialogue())return -1;
 if(revised_rules_){auto order=state_.at("enemy_cursor")==0?current_npc_order():npc_order_;for(size_t n=state_.at("enemy_cursor");n<order.size();++n){auto b=BattleUnit::from_json(state_.at("units").at(std::to_string(order[n])).at("battle"));if(b.hp>0&&b.lifecycle==2&&!(b.flags&2))return b.status&8?-1:b.id;}return -1;}
 std::vector<int> order;for(const auto& u:state_.at("units"))if(u.at("battle").at("side")==2)order.push_back(u.at("battle").at("id"));
 std::sort(order.begin(),order.end());
 for(size_t n=state_.at("enemy_cursor");n<order.size();++n){auto b=BattleUnit::from_json(state_.at("units").at(std::to_string(order[n])).at("battle"));if(b.hp>0)return b.status&8?-1:b.id;}
 return -1;
}
Json Prototype::enemy_request()const{
 int id=next_enemy_id();if(id<0||!balanced_)return nullptr;
 auto fingerprint=sha256(Json{{"state",state_},{"dialogue",dialogue_},{"scenario",scenario_},{"terrain_enabled",terrain_enabled_},{"benefits",benefits_},{"items_enabled",items_enabled_},{"consumed_items",consumed_items_}}.dump());if(fingerprint==request_cache_key_)return request_cache_;
 const auto& actor=state_.at("units").at(std::to_string(id));auto a=BattleUnit::from_json(actor.at("battle"));auto r=UnitRecord::from_json(actor.at("record"));auto costs=movement(id);
 auto board=roster();bool holding=revised_rules_&&!npc_engaged(id);
 struct Candidate {Json plan;int score,cost;};std::vector<Candidate> candidates;
 for(int pos=0;pos<cell_count();++pos){int x=pos%map_width(),y=pos/map_width(),occupant=at(x,y);if((holding&&(x!=a.x||y!=a.y))||costs[pos]==255||(occupant>=0&&occupant!=id))continue;
  int distance=10000;auto range=battle_range(a,{x,y},battle_attack_range(r,data_),0,false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},board);
  for(const auto& u:state_.at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0||(revised_rules_&&b.lifecycle!=2)||b.side==2)continue;
   distance=std::min(distance,std::abs(x-b.x)+std::abs(y-b.y));
   if(!holding&&std::find(range.targets.begin(),range.targets.end(),b.slot)!=range.targets.end()){
    auto ar=a;ar.x=x;ar.y=y;OriginalRng unused;auto br=UnitRecord::from_json(u.at("record"));
    int damage=physical_damage(ar,r,b,br,terrain_.at("cells").at(pos),terrain_.at("cells").at(b.y*map_width()+b.x),false,false,data_,unused);
    candidates.push_back({{{"x",x},{"y",y},{"target",b.id}},10000+(damage>=b.hp?1000:0)+std::min(damage,b.hp),costs[pos]});
   }
  }
  candidates.push_back({{{"x",x},{"y",y},{"target",-1}},-distance,costs[pos]});
  if(tactics_enabled_&&!holding)for(const auto& spell:spell_options(id,{x,y})){
   int damage=spell.at("damage_on_hit"),hit=spell.at("hit_percent"),hp=state_.at("units").at(std::to_string(spell.at("target").get<int>())).at("battle").at("hp");
   int value=damage*hit/100+(spell.at("tactic")==33?8*hit/100:0);
   if(revised_rules_)value=spell.value("total_damage_on_hit",damage)*hit/100+spell.value("total_healing_on_hit",0)+(spell.value("effect",std::string())=="support"?25:0)+spell.value("status_percent",0)/5;
   candidates.push_back({spell,10000+(damage>=hp?1000*hit/100:0)+value,costs[pos]});
  }
 }
 std::sort(candidates.begin(),candidates.end(),[](const auto& l,const auto& r){return std::tuple{-l.score,l.cost,l.plan.at("y").template get<int>(),l.plan.at("x").template get<int>(),l.plan.at("target").template get<int>()}<std::tuple{-r.score,r.cost,r.plan.at("y").template get<int>(),r.plan.at("x").template get<int>(),r.plan.at("target").template get<int>()};});
 Json choices=Json::object();std::set<std::string> seen;
 auto add=[&](const Json& p){if(choices.size()<48&&seen.insert(p.dump()).second)choices["a"+std::to_string(choices.size())]=p;};
 if(tactics_enabled_&&!candidates.empty())add(candidates.front().plan);
 if(!holding)add(suggest(id));add({{"x",a.x},{"y",a.y},{"target",-1}});
 // Preserve healing destinations even when offensive variants fill the cap.
 if(terrain_enabled_){int reserved=0;for(const auto& c:candidates)if(c.plan.at("target")==-1&&healing_at(id,c.plan.at("x"),c.plan.at("y"))>0&&reserved++<8)add(c.plan);}
 // Reserve spell/target coverage before physical alternatives fill the cap.
 std::set<std::pair<int,int>> spell_targets;
 for(const auto& c:candidates)if(c.plan.contains("tactic")&&spell_targets.insert(std::pair{c.plan.at("tactic").get<int>(),c.plan.at("target").get<int>()}).second)add(c.plan);
 // Include one option for each attack target, then nearby safe repositioning,
 // then alternatives. Limit model input without excluding the local fallback.
 std::set<int> targets;for(const auto& c:candidates)if(c.plan.at("target")!=-1&&targets.insert(c.plan.at("target").get<int>()).second)add(c.plan);
 int reposition=0;for(const auto& c:candidates)if(c.plan.at("target")==-1&&reposition++<8)add(c.plan);
 for(const auto& c:candidates)add(c.plan);
 if(terrain_enabled_)for(auto& choice:choices)choice["terrain_heal_next_phase"]=healing_at(id,choice.at("x"),choice.at("y"));
 Json units=Json::array();for(const auto& u:state_.at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0||(revised_rules_&&b.lifecycle!=2))continue;auto ur=UnitRecord::from_json(u.at("record"));
  units.push_back({{"id",b.id},{"name",u.at("name")},{"side",b.side},{"x",b.x},{"y",b.y},{"hp",b.hp},{"max_hp",u.at("max_hp")},{"level",ur.level},{"class",ur.job},{"attack",battle_stat(b,ur,0,data_)},{"defense",battle_stat(b,ur,1,data_)},{"equipment",ur.equipment},{"status",b.status}});
  if(revised_rules_)units.back()["grades"]=b.grades;
  if(tactics_enabled_)units.back().update({{"mp",b.mp},{"max_mp",ur.mp},{"spirit",battle_stat(b,ur,2,data_)},{"spells",spells(b.id)}});
 }
 Json context={{"protocol","caocao-physical-v1"},{"battle",name()},{"scenario",scenario_},{"turn",state_.at("turn")},{"turn_limit",20},{"player_objective",objective()},{"actor",id},{"units",units},{"terrain",terrain_},{"choices",choices}};
 if(tactics_enabled_)context.update({{"protocol","caocao-tactics-v2"},{"tactics_manifest",tactics_manifest_},{"capabilities",{{"tactics",{0,5,10,15,33}},{"poison","Independent status and damage checks. Poison may recover before the afflicted side's next turn; otherwise loses 10% maximum HP, never lethal."},{"other_spells",false},{"items",false},{"counterattack",false}}}});
 if(terrain_enabled_){
  context.update({{"protocol","caocao-terrain-v3"},{"terrain_manifest",terrain_manifest_},{"locations",locations()},{"player_inventory",inventory()}});
  context["capabilities"]["terrain_recovery"]={{"18",25},{"19",20},{"20",20},{"21",15},{"22",20}};
  context["capabilities"]["terrain_recovery_rule"]="Percent of maximum HP (floor, capped at missing HP), at the occupying army's next phase start, after poison. No terrain MP regeneration. No immediate healing on movement. Estimate assumes no intervening damage.";
  context["capabilities"]["location_items"]="Once per battle at listed coordinates, on a committed action. Only living friendly slots <35 can collect; side 2 cannot. Collected items are saved but item use is not implemented.";
  if(items_enabled_){context["protocol"]="caocao-items-v4";context["items_manifest"]=items_manifest_;context["capabilities"]["items"]={{"player_only",true},{"healing",{{"87",30},{"88",80}}},{"antidote",92},{"range","Self or eight adjacent living friendly tiles; ends actor action; no RNG or MP cost."},{"enemy_inventory_access",false}};context["capabilities"]["location_items"]="Once per battle on a committed friendly action. Inventory is remaining stock after consumption. Enemies cannot collect or use this player inventory; consider possible player healing.";}
 }
 if(revised_rules_){
  context["protocol"]="caocao-revised-v5";context["extended_tactics_manifest"]=extended_tactics_manifest_;
  context["capabilities"]["tactics"]={0,2,5,7,10,15,17,20,23,25,33,35,39,61,63};
  context["capabilities"]["counterattack"]=true;context["capabilities"]["critical_double"]=true;
  context["capabilities"]["status_recovery"]="Original luck, neighboring healer and recovery equipment rules; confusion may prevent action until recovered. Side 1 is AI-controlled. Hidden troops are omitted.";
  context["capabilities"]["tactic_estimates"]="targets lists every affected unit. total_damage_on_hit sums area damage. Healing_on_hit and total_healing_on_hit restore friendly HP. support improves stats; debuff reduces stats or applies status. Physical choices can provoke counters and extra strikes; listed physical damage is first-strike estimate only.";
  context["script_progress"]=progress_;context["actor_ai"]=a.ai;
 }
 // Identity includes RNG/event flags without sending hidden RNG to the model.
 std::string key=sha256(Json{{"context",context},{"state",state_},{"dialogue",dialogue_},{"assets",manifest_},{"expansion",expansion_manifest_}}.dump());
 request_cache_key_=fingerprint;request_cache_={{"key",key},{"state",context},{"choices",choices}};return request_cache_;
}
void Prototype::choose_enemy(const std::string& key,const std::string& choice){
 auto request=enemy_request();if(request.is_null()||request.at("key")!=key||!request.at("choices").contains(choice))throw std::runtime_error("stale or invalid enemy decision");
 enemy_choice_={{"key",key},{"choice",choice}};
}
}
