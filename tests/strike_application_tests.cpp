#include "caocao/strike_application.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_strike_application.json");
 size_t index=0,pending_count=0;
 for(const auto& c:fixture.at("cases")){
  auto actor=BattleUnit::from_json(c.at("actor"));auto au=UnitRecord::from_json(c.at("actor_unit"));
  auto targets=c.at("targets");Json requests=Json::array();bool pending=false;
  OriginalRng rng{c.at("seed").get<uint32_t>()};
  for(size_t i=0;i<targets.size();++i){
   StrikeTarget t{BattleUnit::from_json(targets[i].at("battle")),UnitRecord::from_json(targets[i].at("unit")),0};
   auto r=apply_strike_target(actor,au,t,c.at("hp_damage")[i],c.at("mp_damage")[i],c.at("armor_xp")[i],c.at("area"),c.at("beans"),data,rng);
   targets[i]={{"battle",r.target.battle.json()},{"unit",r.target.unit.json()}};
   for(const auto& request:r.requests)requests.push_back(request);
   pending=r.pending_item;if(pending)break;
  }
  if(targets!=c.at("after")||requests!=c.at("events")||pending!=c.at("pending_item")||rng.state!=c.at("final_rng"))
   throw std::runtime_error("strike application mismatch "+std::to_string(index)+" targets "+targets.dump()+" requests "+requests.dump());
  ++index;if(pending)++pending_count;
 }
 std::cout<<index<<" original strike applications passed, including "<<pending_count<<" explicit automatic-item suspensions\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
