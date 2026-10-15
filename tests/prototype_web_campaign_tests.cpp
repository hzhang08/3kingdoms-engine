#include "caocao/prototype.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
static void drain(Prototype& g){
 for(int n=0;n<1000;++n){if(g.has_dialogue())g.command("continue");else if(!g.story_choices().empty())g.choose_story(0);else return;}
 throw std::runtime_error("fixture dialogue stalled");
}
// Synthetic casualties exercise campaign persistence and UI transitions only.
// They are deliberately separate from any claim of a four-battle playthrough.
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("usage: assets output-directory");
 std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 Prototype g(argv[1]);g.enable_revised_rules();g.enable_equipment();
 Json hashes=Json::array();
 for(int battle=0;battle<4;++battle){
  drain(g);auto start=g.serialize();g.restore(start);
  std::ofstream(out/(std::to_string(battle)+"-start.json"))<<start;
  auto s=Json::parse(start);
  for(auto& u:s["payload"]["state"]["units"])if(u["battle"]["side"].get<int>()>=2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}
  s["payload"]["state"]["units"]["0"]["battle"]["flags"]=0;
  s["sha256"]=sha256(s["payload"].dump());g.restore(s.dump());
  auto b=g.state().at("units").at("0").at("battle");g.command("tile",b.at("x"),b.at("y"));g.command("wait");drain(g);
  if(g.state().at("phase")!="victory"||!g.progress().at("active").is_null())throw std::runtime_error("victory/rewards not complete");
  auto won=g.serialize();g.restore(won);if(g.serialize()!=won)throw std::runtime_error("restore changed rewards");
  std::ofstream(out/(std::to_string(battle)+"-victory.json"))<<won;hashes.push_back(g.hash());
  if(battle<3)g.advance_campaign();
 }
 std::ofstream(out/"hashes.json")<<hashes.dump(2);std::cout<<hashes.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
