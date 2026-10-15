#include "caocao/range.hpp"
#include "caocao/ai.hpp"
#include "caocao/movement.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_ranges.json");
 size_t index=0;
 for(const auto& c:fixture.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle"));std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  int w=c.at("width"),h=c.at("height");RangeGrids grids{std::vector<int>(w*h,165),std::vector<int>(w*h,90),std::vector<int>(w*h,165)};
  auto result=battle_range(b,c.at("center").get<std::array<int,2>>(),c.at("kind"),c.at("mode"),c.at("mask_only"),c.at("preferred"),w,h,std::move(grids),roster);
  if(result.selected!=c.at("result")||Json(result.targets)!=c.at("targets")||Json(result.grids.distance)!=c.at("distance")||Json(result.grids.threat)!=c.at("threat")||Json(result.grids.mask)!=c.at("mask"))throw std::runtime_error("range mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original range grids and ordered target lists passed\n";
 for(const auto& c:fixture.at("getters")){
  auto u=load_unit_record(data.at("heroes").at(0));u.job=c.at("job");u.equipment={{{255,255,255},{255,255,255},{c.at("accessory"),255,255}}};
  if(battle_attack_range(u,data)!=c.at("kind"))throw std::runtime_error("range getter mismatch: "+c.dump());
 }
 std::cout<<fixture.at("getters").size()<<" original class/equipment range getters passed\n";index=0;
 for(const auto& c:fixture.at("preparations")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  int w=c.at("width"),h=c.at("height");auto result=prepare_battle_ai(b,u,roster,w,h,c.at("terrain"),data,std::vector<int>(w*h,165));
  if(result.flags!=c.at("flags")||result.best_score!=c.at("best_score")||result.best_target!=c.at("best_target")||result.best_tactic!=c.at("best_tactic")||result.best_tile[0]!=c.at("best_tile")||Json(result.distance)!=c.at("distance")||Json(result.threat)!=c.at("threat")||Json(result.scores)!=c.at("scores"))throw std::runtime_error("AI preparation mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original AI preparation cases passed\n";index=0;
 for(const auto& c:fixture.at("searches")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  auto actual=battle_movement_search(b,u,{b.x,b.y},c.at("budget"),{255,255},MovementSearchMode::opponent_range,c.at("respect_zoc"),c.at("width"),c.at("height"),c.at("terrain"),roster,data,c.at("threat").get<std::vector<int>>());
  if(actual.result!=c.at("result")||Json(actual.costs)!=c.at("costs"))throw std::runtime_error("AI target search mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original AI target-search grids and results passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
