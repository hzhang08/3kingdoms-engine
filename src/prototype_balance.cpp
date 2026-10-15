#include "caocao/prototype.hpp"
#include <functional>
namespace caocao {
// Consume the verified deployment prefix only. These are NOT substitutes for
// executing the later ambush/duel/event scripts of the full campaign engine.
UnitRecord Prototype::balanced_record(int id,int slot)const{
 const bool carrying=revised_rules_&&!campaign_.empty();
 const Json* carried=carrying?&campaign_.at("battles").back().at("party"):nullptr;
 std::vector<int> carried_levels;if(carried)for(const auto& r:*carried)carried_levels.push_back(r.at("level"));
 const int party_level=carrying?deployment_level(carried_levels):scenario_?config().at("friendly_level").get<int>():3;
 // Campaign continuation keeps the verified character/equipment record exactly;
 // battlefield HP/MP/status are reset by initialize, as a prototype boundary.
 if(slot<15&&carried&&carried->contains(std::to_string(id)))return UnitRecord::from_json(carried->at(std::to_string(id)));
 const auto& script=scenario_?scenario_scripts_.at(std::to_string(scenario_)):opening_script_;
 std::vector<DeploymentSlot> descriptors;int base=party_level;
 bool prefix=true;
 std::function<void(const Json&)> visit=[&](const Json& body){for(const auto& i:body){
  if(!prefix)break;int op=i.at("opcode");
  auto val=[&](int n){return i.at("args").at(n).at("value").get<int>();};
  if(op==1){if(i.at("test").at("opcode")!=2)throw std::runtime_error("unverified balance prefix condition at "+i.at("offset").dump());visit(i.at("body"));continue;}
  if(op==0||op==2||op==68||op==74||op==75)continue;
  if(op==69){int adjustment=val(3);adjustment=adjustment<0?0:adjustment<=50?adjustment:50-adjustment;
   std::vector<int> levels;
   if(carrying){for(const auto& row:config().at("roster"))if(row.at("side")==0){auto key=std::to_string(row.at("id").get<int>());levels.push_back(carried->contains(key)?carried->at(key).at("level").get<int>():party_level);}}
   else levels.assign(scenario_?7:1,party_level);
   base=std::max(1,deployment_level(levels)+adjustment);continue;}
  if(op==70||op==71){int width=op==70?11:12;
   for(size_t n=0;n<i.at("args").size()/width;++n){std::vector<int> a;for(int k=0;k<width;++k)a.push_back(val(int(n)*width+k));
    auto d=deployment_row((op==70?15:35)+int(n),op==71,a,base);if(d.id!=65535)descriptors.push_back(d);
   }continue;}
  if(op==72){for(auto& d:descriptors)if(d.id==val(0)){equipment_deployment(d,{val(1),val(2),val(3),val(4),val(5)});break;}continue;}
  prefix=false;
 }};
 visit(script.at("sections").at(0).at("events").at(0).at("body"));
 auto r=load_unit_record(data_.at("heroes").at(id));
 if(slot<15){
  // Standalone party presets, not recovered campaign saves. Give them the same
  // level-appropriate ordinary gear policy as NPCs and retain Cao Cao's R_00 sword.
  initialize_npc(r,party_level,2,data_);default_equipment(r,false,data_.at("classes"));
  if(id==0)script_equipment(r,{25,0,0,0,0},data_);
 }else{
  auto it=std::find_if(descriptors.begin(),descriptors.end(),[&](const auto& d){return d.id==id;});
  if(it==descriptors.end())throw std::runtime_error("missing original balance descriptor for "+std::to_string(id));
  // Reuse original NPC construction including default gear, explicit removal,
  // auto-levelled unique weapons and class compatibility, not story opcode 62.
  auto d=*it;d.ai=1;initialize_battle_unit(r,d,data_);
 }
 // The revised snapshot format uses the recovered maximum-level XP sentinel.
 // Keep historical standalone initialization unchanged for earlier rulesets.
 r.xp=revised_rules_&&r.level>=50?255:0;return r;
}
}
