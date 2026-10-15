#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/recovery.hpp"
#include <iostream>
#include <fstream>
using namespace caocao;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void drain(Prototype& g){for(int n=0;n<500;++n){if(g.has_dialogue())g.command("continue");else if(!g.story_choices().empty())g.choose_story(0);else return;}throw std::runtime_error("dialogue loop");}
void playback(Prototype& g){PrototypePlayback p;p.start(g.take_movement(),g.take_attack(),g.take_environment());for(int i=0;p.active();++i){check(i<10000,"presentation loop");p.advance(40);}}
int main(int argc,char** argv){try{
 auto assets=std::filesystem::path(argv[1]);Json report=Json::array();
 if(argc>3&&std::string(argv[2])=="--restore"){Prototype g(assets);g.enable_revised_rules();std::ifstream f(argv[3]);std::string s{std::istreambuf_iterator<char>(f),{}};g.restore(s);check(g.serialize()==s,"cross-platform restore differs");std::cout<<g.hash()<<'\n';return 0;}
 for(int scenario=0;scenario<4;++scenario){
  Prototype g(assets);g.enable_revised_rules();g.start_scenario(scenario);drain(g);int actions=0,turns=0;std::string trace;
  // Starting hidden troops and manual control of allies are separate concepts.
  for(const auto& u:g.state().at("units"))if(u.at("battle").at("side")==1&&u.at("battle").at("lifecycle")==2){const auto& b=u.at("battle");g.command("tile",b.at("x"),b.at("y"));check(g.state().at("selected")==-1,"allied unit is player-controlled");break;}
  for(int n=0;n<3000;++n){
   drain(g);auto phase=g.state().at("phase").get<std::string>();if(phase=="victory"||phase=="defeat")break;
   if(phase=="player"){
    int id=-1;for(const auto& u:g.state().at("units")){const auto& b=u.at("battle");if(b.at("side")==0&&b.at("lifecycle")==2&&b.at("hp").get<int>()>0&&!(b.at("flags").get<int>()&2)&&!(b.at("status").get<int>()&8)){id=b.at("id");break;}}
    if(id<0){g.command("end_turn");++turns;}
    else{
     auto b=g.state().at("units").at(std::to_string(id)).at("battle");g.command("tile",b.at("x"),b.at("y"));
     auto plan=g.suggest(id);if(plan.at("x")!=b.at("x")||plan.at("y")!=b.at("y")){g.command("tile",plan.at("x"),plan.at("y"));playback(g);}
     int target=plan.at("target");
     if(target>=0){const auto t=g.state().at("units").at(std::to_string(target)).at("battle");g.command("tile",t.at("x"),t.at("y"));}else g.command("wait");
     ++actions;
    }
   }else g.enemy_step();
   playback(g);auto snapshot=g.serialize();Prototype restored=g;restored.restore(snapshot);check(snapshot==restored.serialize(),"action snapshot changed on restore");trace+=g.hash()+"\n";
  }
  drain(g);check(g.state().at("phase")=="victory"||g.state().at("phase")=="defeat","autoplay did not terminate");
  auto stable=g.serialize();g.restore(stable);check(g.serialize()==stable,"outcome restore differs");
  report.push_back({{"scenario",scenario},{"result",g.state().at("phase")},{"turn",g.state().at("turn")},{"player_actions",actions},{"trace_sha256",sha256(trace)},{"final_state_hash",g.hash()}});
  if(argc>2)std::ofstream(std::string(argv[2])+".battle-"+std::to_string(scenario)+".json")<<g.serialize();
 }
 if(argc>2)std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
