#include "caocao/experience.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_experience.json");
 size_t index=0;
 for(const auto& group:{"xp","equipment"}){
  for(const auto& c:fixture.at(group)){
   auto u=UnitRecord::from_json(c.at("before"));bool gear=std::string(group)=="equipment";
   auto change=gear?add_equipment_experience(u,c.at("slot"),c.at("gain"),data):set_character_experience(u,c.at("total"),data);
   Json events=Json::array();
   for(int level:change.levels){Json e={{"kind",gear?"equipment_level":"growth"},{"unit",u.id},{"level",level}};if(gear)e["slot"]=c.at("slot");events.push_back(e);}
   if(change.refresh)events.push_back({{"kind","refresh"},{"unit",u.id}});
   if(u.json()!=c.at("after")||events!=c.at("events"))throw std::runtime_error(std::string(group)+" XP mismatch "+std::to_string(index)+": "+u.json().dump()+" events "+events.dump());
   ++index;
  }
  std::cout<<fixture.at(group).size()<<" original "<<group<<" record changes and level requests passed\n";
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
