#include "caocao/battle_events.hpp"
#include <iostream>
#include <stdexcept>
using namespace caocao;
void check(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets required");
 Engine e(argv[1]);e.start_scenario();
 auto environment=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_environment.json");
 for(const auto& sample:environment.at("cases")){
  OriginalRng rng{sample.at("seed")};int region=sample.at("region"),selector=sample.at("selector");
  check(battle_environment_class(region,selector)==sample.at("before_class"),"original environment class mismatch");
  int next=advance_battle_environment(region,selector,sample.at("override"),rng);
  check(next==sample.at("after_selector")&&rng.state==sample.at("rng"),"original environment transition/RNG mismatch");
  check(battle_environment_class(region,next)==sample.at("after_class"),"original updated environment class mismatch");
 }
 std::cout<<environment.at("cases").size()<<" original environment transitions passed\n";
 // Capture input at the original section-0 end, before evaluating any event.
 for(int n=0;n<2000;++n){
  if(e.state().script=="S_00"&&e.state().variables[500]&&e.state().controls.value("menu_enabled",0)==1)break;
  if(e.state().stable)e.submit({e.state().presentation.value("type","")=="choice"?"choose":"continue",1});else e.advance();
 }
 check(e.state().environment_region==environment.at("initialization").at("configuration").at("region")&&e.state().environment_selector==environment.at("initialization").at("configuration").at("selector")&&e.state().turn_limit==environment.at("initialization").at("configuration").at("limit"),"original controller initialization mismatch");
 check(e.state().variables[500]==1,"intro did not finish");
 BattleEventState base;for(const auto& u:e.state().units)if(u.contains("battle"))base.units.push_back(BattleUnit::from_json(u.at("battle")));
 auto fixtures=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_battle_events.json");
 auto placement=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_placement.json");
 for(const auto& sample:placement.at("cases")){
  auto out=battle_placement(sample.at("xy").at(0),sample.at("xy").at(1),sample.at("group"),20,20,placement.at("terrain"),placement.at("occupancy").at(sample.at("grid").get<int>()).get<std::vector<int>>(),e.data());
  check(Json(out)==sample.at("result"),"placement search mismatch "+sample.dump());
 }
 std::cout<<placement.at("cases").size()<<" original placement cases passed\n";
 check(fixtures.at("configuration")==Json::array({0,0,255,255,1,0,0,0}),"unverified outcome configuration");
 size_t index=0;for(const auto& sample:fixtures.at("cases")){
  auto s=base;const auto& input=sample.at("input");s.turn=input.at("turn");s.phase=input.at("phase");s.outcome=input.value("outcome",0);
  s.victory=input.value("victory",0);s.defeat=input.value("defeat",0);s.variables[500]=1;
  for(int v:input.at("variables"))s.variables.at(v)=1;
  for(const auto& u:input.value("units",Json::array()))for(auto& b:s.units)if(b.id==u.at("id")){
   b.x=u.value("x",b.x);b.y=u.value("y",b.y);b.hp=u.value("hp",b.hp);b.lifecycle=u.value("lifecycle",b.lifecycle);
  }
  if(input.value("all_enemies_removed",false))for(auto& b:s.units)if(b.slot>=35)b.lifecycle=3;
  auto original=s;size_t row=0;
  for(const auto& p:e.programs())if(p.script=="S_00"&&p.section==1){size_t n=0;
   for(const auto& i:p.conditions){auto result=opening_battle_condition(i,s);check(result==sample.at("conditions").at(row).at(n++).get<int>(),"condition mismatch case "+std::to_string(index)+" at "+std::to_string(i.at("offset").get<int>()));}++row;
  }
  auto selected=select_opening_event(e.programs(),original);Json offset=selected<0?Json(nullptr):Json(e.programs().at(selected).offset);
  check(offset==sample.at("selected"),"selected event mismatch case "+std::to_string(index));
  check(s.victory==sample.at("victory").get<int>()&&s.defeat==sample.at("defeat").get<int>(),"condition latch mismatch");
  check(original.victory==s.victory&&original.defeat==s.defeat,"selector short-circuited conditions");++index;
 }
 std::cout<<index<<" original event-selection/condition cases passed\n";return 0;
}catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}}
