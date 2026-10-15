#include "caocao/engine.hpp"
#include "caocao/battle.hpp"
#include <iostream>
#include <stdexcept>
using namespace caocao;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"unsupported action/save was accepted");}
int main(int argc,char** argv){try{
 check(argc==2,"assets path required");std::filesystem::path assets=argv[1];
 auto fixture=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_objective_banner.json");
 auto labels=read_json(assets/"ui.json").at("objective_labels");
 for(const auto& c:fixture.at("cases")){const auto& in=c.at("input");
  auto actual=battle_objective_banner(in.at("text"),in.at("limit"),labels,in.at("viewport"),in.at("client"));
  check(actual==c.at("events"),"objective banner differs from original requests");
 }
 Engine e(assets);e.start_scenario(1);std::string before_banner;
 for(int n=0;n<5000&&e.state().presentation.value("type","")!="banner_region";++n){
  if(e.state().stable){if(e.state().pending_action.is_null())before_banner=e.serialize();e.submit({e.state().presentation.value("type","")=="choice"?"choose":"continue",1});}
  else e.advance();check(e.state().phase!="blocked","stopped before objective banner");
 }
 check(e.state().presentation.value("type","")=="banner_region","objective banner was not reached");
 auto units=e.state().units;auto rng=e.state().rng.state;rejects([&]{e.serialize();});rejects([&]{e.submit({"continue"});});
 while(e.state().presentation.value("type","")!="objective_pause"){e.advance();check(e.state().phase!="blocked","stopped during objective banner");}
 int cases=0;
 for(const auto& p:fixture.at("pauses")){
  Engine test(assets);test.restore(before_banner);test.submit({"continue"});int ticks=0,clears=0;bool interrupted=false;
  for(int n=0;n<1000;++n){const auto& s=test.state();auto type=s.presentation.value("type","");
   if(type=="objective_pause"){
    ++ticks;rejects([&]{test.serialize();});
    if(p.at("interrupt_after").get<int>()==ticks){test.submit({"continue"});interrupted=true;continue;}
   }
   if(type=="banner_region"&&s.presentation.at("color")==255){++clears;
    check(s.units==units&&s.rng.state==rng,"banner mutated units or RNG");
    if(s.presentation.at("height")==8)break;
   }
   test.advance();check(test.state().phase!="blocked","banner stopped before final redraw");
  }
  check(ticks==p.at("ticks")&&clears==9,"terminal input timing or cleanup differs");
  check(interrupted==(p.at("interrupt_after")!=0&&p.at("interrupt_after").get<int>()<=20),"interrupt boundary differs");
  ++cases;
 }
 std::cout<<fixture.at("cases").size()<<" original objective sequences; "<<cases<<" input/cleanup/save boundaries passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
