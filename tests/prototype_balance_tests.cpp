#include "caocao/prototype.hpp"
#include "caocao/combat_rules.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 if(argc<3)throw std::runtime_error("assets and original initialization fixture required");
 Prototype g(argv[1]),mirror(argv[1]);
 if(std::string(argv[2])=="--recorded"){
  if(argc!=4)throw std::runtime_error("recorded decision file required");auto record=read_json(argv[3]);g.restore(record.at("planned_snapshot"));check(g.has_enemy_choice(),"missing recorded model choice");g.enemy_step();check(g.hash()==record.at("next_hash").get<std::string>(),"cross-platform JEV choice replay differs");std::cout<<"recorded JEV replay passed "<<g.hash()<<'\n';return 0;
 }
 auto oracle=read_json(argv[2]);Json report=Json::array();std::string trace;
 for(int n=0;n<4;++n){g.start_scenario(n);auto init=g.serialize();int checked=0;
  for(const auto& ref:oracle.at("scenarios").at(n).at("npcs")){
   auto id=std::to_string(ref.at("id").get<int>());if(!g.state().at("units").contains(id))continue;
   auto expected=ref;expected["xp"]=0;const auto& actual=g.state().at("units").at(id).at("record");
   if(actual!=expected){std::cerr<<"scenario="<<n<<" id="<<id<<" expected="<<expected.dump()<<" actual="<<actual.dump()<<'\n';throw std::runtime_error("NPC initialization differs from original code");}++checked;
  }
  Json rows=Json::array();for(const auto& u:g.state().at("units")){auto r=UnitRecord::from_json(u.at("record"));auto b=BattleUnit::from_json(u.at("battle"));rows.push_back({{"id",b.id},{"name",u.at("name")},{"side",b.side},{"level",r.level},{"hp",b.hp},{"attack",battle_stat(b,r,0,g.data())},{"defense",battle_stat(b,r,1,g.data())},{"equipment",r.equipment}});}
  int decisions=0;std::string request_snapshot;
  for(int tick=0;tick<500&&g.state().at("phase")!="defeat"&&g.state().at("phase")!="victory";++tick){
   mirror.restore(g.serialize());check(mirror.hash()==g.hash(),"balanced save changed state");
   if(g.has_dialogue()){g.command("continue");mirror.command("continue");}
   else if(g.state().at("phase")=="player"){g.command("end_turn");mirror.command("end_turn");}
   else{
    auto req=g.enemy_request();if(!req.is_null()){
     auto prior=g.serialize();for(auto [key,choice]:{std::pair{std::string("stale"),std::string("a0")},std::pair{req.at("key").get<std::string>(),std::string("illegal")}}){bool rejected=false;try{g.choose_enemy(key,choice);}catch(...){rejected=true;}check(rejected&&g.serialize()==prior,"invalid AI choice mutated state");}
     // A non-default legal decision also survives restore and consumes the same RNG.
     std::string choice=decisions%3==0&&req.at("choices").contains("a2")?"a2":"a0";
     g.choose_enemy(req.at("key"),choice);mirror.restore(g.serialize());check(mirror.has_enemy_choice(),"queued model decision lost on restore");
     if(request_snapshot.empty())request_snapshot=g.serialize();++decisions;
    }
    g.enemy_step();mirror.enemy_step();
   }
   check(g.hash()==mirror.hash(),"recorded AI continuation is not deterministic");trace+=g.hash()+"\n";
  }
  check(decisions>0,"enemy choices not exercised");check(g.state().at("phase")=="defeat","passive play should reach defeat");
  auto end=g.serialize();g.restore(init);auto bad=Json::parse(init);bad["payload"]["state"]["units"]["0"]["record"]["level"]=1;bad["sha256"]=sha256(bad.at("payload").dump());bool rejected=false;try{g.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==init,"forged balance stats accepted");
  Prototype old(argv[1],true,false);old.start_scenario(n);auto oldsave=old.serialize();g.restore(oldsave);check(g.serialize()==oldsave&&!g.balanced(),"old save rebalanced");g.command("restart");check(g.balanced()&&g.serialize()==init,"restart did not opt into new rules");
  report.push_back({{"scenario",n},{"original_npcs_verified",checked},{"units",rows},{"decisions_replayed",decisions},{"passive_outcome",Json::parse(end).at("payload").at("state").at("phase")},{"pending_snapshot",request_snapshot}});
  std::cout<<"battle "<<n<<": "<<checked<<" original NPC records; "<<decisions<<" choices; legacy saves, restart, corruption, pending-decision resume passed\n";
 }
 if(argc>3){std::ofstream(std::string(argv[3])+".json")<<report.dump(2);std::ofstream(std::string(argv[3])+".trace")<<trace;}
 std::cout<<"trace "<<sha256(trace)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
