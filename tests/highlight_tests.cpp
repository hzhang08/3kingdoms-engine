#include "caocao/engine.hpp"
#include "caocao/selection.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace caocao;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 check(argc==2,"assets path required");std::filesystem::path assets=argv[1];
 auto fixture=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_highlight.json");
 for(const auto& c:fixture.at("cases")){
  std::vector<BattleUnit> units;for(const auto& b:c.at("units"))units.push_back(BattleUnit::from_json(b));
  // Deliberately reverse container order; the original searches slot order.
  std::reverse(units.begin(),units.end());auto actual=scripted_battle_highlight(units,c.at("input").at("id"));
  check(actual==c.at("events"),"highlight requests differ from original opcode92");
  check(actual.empty()==(c.at("result")==0),"highlight lookup result differs");
 }
 for(const auto& c:fixture.at("masks")){
  BattleUnit b;b.x=c.at("position").at(0);b.y=c.at("position").at(1);
  check(Json(selection_target_mask(c.at("slot")==255?nullptr:&b,c.at("width"),c.at("height")))==c.at("mask"),"target/sentinel mask differs from original");
 }
 Engine e(assets),restored(assets);e.start_scenario(1);int waits=0;bool mirrored=false;Json units;uint32_t rng=0;
 for(int n=0;n<5000&&e.state().phase!="blocked"&&e.state().presentation.value("type","")!="tutorial_begin";++n){const auto& s=e.state();
  if(s.presentation.value("type","")=="banner_region"&&!mirrored){units=s.units;rng=s.rng.state;}
  if(!s.pending_action.is_null()&&s.pending_action.at("opcode")==92){
   check(s.units==units&&s.rng.state==rng,"highlight modified units or RNG");
   if(s.presentation.value("type","")=="animation_wait"){
    int id=waits<6?146:147;check(s.pending_action.at("unit")==id,"target order differs");
    check(s.presentation.at("duration_ms")==500,"target flash wait differs");
    auto b=BattleUnit::from_json(s.units.at(std::to_string(id)).at("battle"));
    const auto& ui=s.pending_action.at("ui");
    check(ui.at("range")==Json(selection_target_mask(waits%2==0?&b:nullptr,20,20)),"target flash did not alternate");
    check(!ui.at("panel").is_null(),"unit panel disappeared during flash");++waits;
   }
   bool rejected=false;try{e.serialize();}catch(const std::exception&){rejected=true;}check(rejected,"mid-highlight save accepted");
  }
  if(s.stable){
   if(s.pending_action.is_null()){restored.restore(e.serialize());mirrored=true;}
   Command cmd{s.presentation.value("type","")=="choice"?"choose":"continue",1};e.submit(cmd);if(mirrored)restored.submit(cmd);
  }else{e.advance();if(mirrored)restored.advance();}
  if(mirrored)check(e.state_hash()==restored.state_hash(),"highlight continuation diverged after save/reload");
  // Cache the state immediately before the first highlight; earlier stable
  // snapshots also cover the whole banner's deterministic continuation.
  if(!e.state().pending_action.is_null()&&e.state().pending_action.at("opcode")==26){units=e.state().units;rng=e.state().rng.state;}
 }
 check(waits==12,"two three-cycle highlights did not complete");
 check(e.state().presentation.value("type","")=="tutorial_begin","unexpected post-highlight boundary");
 check(e.state().units.at("157").at("battle").at("lifecycle")==2,"tutorial guide did not appear");
 std::cout<<fixture.at("cases").size()<<" original highlight sequences; "<<fixture.at("masks").size()<<" masks; two integrated highlights and save continuation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
