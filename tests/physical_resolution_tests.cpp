#include "caocao/combat_rules.hpp"
#include "caocao/strike.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_physical_resolution.json");
 size_t index=0;
 for(const auto& c:fixture.at("stats")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));
  for(int n=0;n<5;++n)if(battle_stat(b,u,n,data)!=c.at("values").at(n))throw std::runtime_error("battle stat mismatch case "+std::to_string(index)+" statistic "+std::to_string(n));
  ++index;
 }
 std::cout<<index<<" original five-stat getter cases passed\n";index=0;
 for(const auto& c:fixture.at("cases")){
  auto a=BattleUnit::from_json(c.at("attacker")),b=BattleUnit::from_json(c.at("target"));
  auto au=UnitRecord::from_json(c.at("attacker_unit")),bu=UnitRecord::from_json(c.at("target_unit"));
  auto chance=physical_hit_chance(a,au,b,bu,data);
  if(chance!=c.at("chance"))throw std::runtime_error("physical hit chance mismatch "+std::to_string(index)+": "+std::to_string(chance)+" vs "+c.at("chance").dump());
  OriginalRng rng{c.at("seed").get<uint32_t>()};
  auto damage=physical_damage(a,au,b,bu,c.at("attack_terrain"),c.at("defense_terrain"),c.at("randomize"),c.at("counterattack"),data,rng);
  if(damage!=c.at("damage")||rng.state!=c.at("final_rng"))throw std::runtime_error("physical damage/RNG mismatch "+std::to_string(index)+": "+std::to_string(damage)+" vs "+c.at("damage").dump());
  ++index;
 }
 std::cout<<index<<" original physical chance, damage and final RNG cases passed\n";
 auto speeches=read_json(root.parent_path().parent_path()/"tests/fixtures/original_critical_speech.json");index=0;
 for(const auto& c:speeches.at("cases")){
  int unit=c.at("unit"),group=c.at("group");
  // The immutable source byte is retained by the importer; it is not a
  // randomly inferred voice category or a field to regenerate after load.
  auto raw=data.at("heroes").at(unit).at("raw").get<std::string>();
  if(std::stoi(raw.substr(32,2),nullptr,16)!=group)throw std::runtime_error("speech group source mismatch");
  OriginalRng rng{c.at("seed").get<uint32_t>()};
  auto message=physical_critical_speech(unit,group,c.at("critical"),rng);
  bool expected=!c.at("events").empty();
  if(bool(message)!=expected||(message&&*message!=c.at("events")[0].at("index"))||rng.state!=c.at("final_rng"))throw std::runtime_error("critical speech/RNG mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original critical speech selections and RNG cases passed\n";
 auto strikes=read_json(root.parent_path().parent_path()/"tests/fixtures/original_strikes.json");index=0;
 for(const auto& c:strikes.at("chances")){
  auto a=BattleUnit::from_json(c.at("attacker")),b=BattleUnit::from_json(c.at("target"));
  auto au=UnitRecord::from_json(c.at("attacker_unit")),bu=UnitRecord::from_json(c.at("target_unit"));
  if(physical_critical_chance(a,au,b,bu,data)!=c.at("critical")||physical_double_chance(a,au,b,bu,data)!=c.at("double"))throw std::runtime_error("critical/double chance mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original critical/double chance cases passed\n";index=0;
 for(const auto& group:{"cases","faults"})for(const auto& c:strikes.at(group)){
  auto a=BattleUnit::from_json(c.at("attacker"));auto au=UnitRecord::from_json(c.at("attacker_unit"));
  std::vector<StrikeTarget> targets;size_t i=0;
  for(const auto& t:c.at("targets"))targets.push_back({BattleUnit::from_json(t.at("battle")),UnitRecord::from_json(t.at("unit")),c.at("target_terrain").at(i++)});
  OriginalRng rng{c.at("seed").get<uint32_t>()};bool fault=c.contains("original_fault"),rejected=false;
  try{
   auto result=plan_physical_strike(a,au,c.at("attack_terrain"),targets,c.at("round"),c.at("counter_depth"),c.at("priority_id"),c.at("previous_xp"),c.at("previous_weapon_xp"),data,rng);
   if(fault)throw std::runtime_error("original division fault was accepted");
   auto actual=result.json();for(const auto& [key,value]:actual.items())if(value!=c.at(key))throw std::runtime_error("strike "+std::to_string(index)+" "+key+" mismatch: "+value.dump()+" != "+c.at(key).dump());
   if(rng.state!=c.at("strike_rng"))throw std::runtime_error("strike RNG mismatch "+std::to_string(index));
   bool another=roll_another_strike(a,au,targets[0].battle,targets[0].unit,c.at("round"),c.at("counter_depth"),data,rng);
   if(another!=c.at("another_strike")||rng.state!=c.at("final_rng"))throw std::runtime_error("double roll/RNG mismatch "+std::to_string(index));
  }catch(const std::runtime_error& e){
   if(!fault||std::string(e.what()).find("division by zero")==std::string::npos)throw;
   rejected=true;if(rng.state!=c.at("seed"))throw std::runtime_error("invalid strike consumed RNG");
  }
  if(fault&&!rejected)throw std::runtime_error("division fault guard failed");++index;
 }
 std::cout<<strikes.at("cases").size()<<" original strike plans/XP requests/RNG and "<<strikes.at("faults").size()<<" original-fault guards passed\n";
 auto counters=read_json(root.parent_path().parent_path()/"tests/fixtures/original_counters.json");index=0;
 for(const auto& c:counters.at("cases")){
  auto a=BattleUnit::from_json(c.at("attacker")),b=BattleUnit::from_json(c.at("target"));
  auto au=UnitRecord::from_json(c.at("attacker_unit")),bu=UnitRecord::from_json(c.at("target_unit"));
  std::vector<BattleUnit> board;for(const auto& row:counters.at("roster")){auto u=BattleUnit::from_json(row);board.push_back(u.slot==a.slot?a:u.slot==b.slot?b:u);}
  auto result=plan_counterattack(a,au,b,bu,c.at("depth"),20,20,board,data);Json events=Json::array();
  if(result.attack)for(const auto& kind:{"facing","attack"})events.push_back({{"kind",kind},{"attacker",b.slot},{"target",a.slot}});
  if(result.depth!=c.at("final_depth")||events!=c.at("events"))throw std::runtime_error("counter gate mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original counterattack gates passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
