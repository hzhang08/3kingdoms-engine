#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/combat_rules.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace caocao;
void check(bool p,const std::string& why){if(!p)throw std::runtime_error(why);}
void step(Prototype& g){
 if(g.has_dialogue()){g.command("continue");return;}
 if(g.state().at("phase")=="enemy"){g.enemy_step();return;}
 if(g.state().at("phase")!="player")return;
 int id=-1;for(const auto& u:g.state().at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp>0&&b.side!=2&&!(b.flags&2)){if(id<0||id==0)id=b.id;if(id!=0)break;}}
 if(g.state().at("selected").get<int>()>=0)id=g.state().at("selected");
 else if(g.scenario()==3)for(const auto& u:g.state().at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp>0&&b.side!=2&&!(b.flags&2)&&g.attackable(b.id,132)){id=b.id;break;}}
 if(id<0){g.command("end_turn");return;}
 auto b=BattleUnit::from_json(g.state().at("units").at(std::to_string(id)).at("battle"));
 if(g.state().at("selected")!=id){g.command("tile",b.x,b.y);return;}
 if(g.scenario()==3&&g.attackable(id,132)){auto t=g.state().at("units").at("132").at("battle");g.command("tile",t.at("x"),t.at("y"));return;}
 if(g.scenario()==3&&id==0){g.command("wait");return;}
 auto p=g.suggest(id);if(g.state().at("undo").is_null()&&(p.at("x")!=b.x||p.at("y")!=b.y)){g.command("tile",p.at("x"),p.at("y"));return;}
 if(!g.state().at("undo").is_null()){
  int target=-1,hp=99999;for(const auto& u:g.state().at("units")){auto t=BattleUnit::from_json(u.at("battle"));if(t.hp>0&&g.attackable(id,t.id)&&std::pair{t.hp,t.id}<std::pair{hp,target}){hp=t.hp;target=t.id;}}p["target"]=target;
 }
 if(p.at("target")!=-1){auto t=g.state().at("units").at(std::to_string(p.at("target").get<int>())).at("battle");g.command("tile",t.at("x"),t.at("y"));}else g.command("wait");
}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");std::filesystem::path assets=argv[1];Prototype g(assets,true,false),mirror(assets,true,false);auto original=g.serialize();
 check(g.scenario_count()==4,"missing battles");
 if(argc==4&&std::string(argv[2])=="--resume"){
  auto e=read_json(argv[3]);g.restore(e.at("snapshot"));step(g);check(g.hash()==e.at("next").get<std::string>(),"cross-platform next action mismatch");std::cout<<"cross-platform battle "<<g.scenario()<<" restore passed "<<g.hash()<<'\n';return 0;
 }
 for(int scenario=(argc>3?std::stoi(argv[3]):1);scenario<4;++scenario){
  std::cerr<<"testing scenario "<<scenario<<'\n';g.start_scenario(scenario);auto start=g.serialize();
  auto configs=read_json(assets/"expansion/scenarios.json");auto c=configs.at(scenario-1);
  check(g.state().at("units").size()==c.at("roster").size(),"roster count differs");
  check(g.cell_count()==(scenario==3?560:400),"wrong grid size");
  for(const auto& r:c.at("roster")){const auto& b=g.state().at("units").at(std::to_string(r.at("id").get<int>())).at("battle");for(auto k:{"id","x","y","direction","side","slot"})check(b.at(k)==r.at(k),"source deployment differs");}
  std::string trace;int checkpoints=0,walks=0,attacks=0,enemyAttacks=0,pages=0;bool wrote=false;
  for(int budget=0;budget<4500;++budget){
   mirror.restore(g.serialize());check(g.hash()==mirror.hash(),"restore changed state");
   if(!g.has_dialogue()&&(g.state().at("phase")=="victory"||g.state().at("phase")=="defeat"))break;
   if(g.has_dialogue())++pages;
   auto before=g.serialize();step(g);step(mirror);check(g.hash()==mirror.hash(),"restored continuation differs");
   trace+=g.hash()+"\n";++checkpoints;
   auto move=g.take_movement(),attack=g.take_attack();if(!move.is_null())++walks;if(!attack.is_null()){++attacks;if(attack.at("before").at("phase")=="enemy")++enemyAttacks;
    if(!wrote&&argc>2){std::ofstream f(std::string(argv[2])+"-"+std::to_string(scenario)+".json");f<<Json{{"snapshot",before},{"next",g.hash()}}.dump();wrote=true;}
   }
   if(!move.is_null()||!attack.is_null()){
    PrototypePlayback playback;playback.start(move.is_null()?attack:move,move.is_null()?Json(nullptr):attack);
    for(int ticks=0;playback.active()&&ticks<5000;++ticks){for(const auto& pose:playback.poses()){
      std::string stem=pose.at("archive");stem=stem=="UNIT_MOV.E5"?"Unit_mov":stem=="UNIT_ATK.E5"?"Unit_atk":"Unit_spc";
      char name[20];snprintf(name,sizeof(name),"%03d.bin",pose.at("sprite").get<int>());
      auto file=assets/"raw"/(stem+".e5")/name;auto end=(pose.at("frame").get<int>()+1)*pose.at("width").get<int>()*pose.at("width").get<int>();check(std::filesystem::file_size(file)>=size_t(end),"missing new animation frame");
     }for(int sound:playback.take_sounds())check(std::filesystem::exists(assets/battle_sound_file(sound)),"missing sound");playback.advance(40);
    }check(!playback.active(),"stuck presentation");
   }
  }
  if(argc>2)std::ofstream(std::string(argv[2])+"-"+std::to_string(scenario)+".trace")<<trace;
  std::cout<<"battle="<<scenario<<" outcome="<<g.state().at("phase")<<" turn="<<g.state().at("turn")<<" checkpoints="<<checkpoints<<" walking="<<walks<<" attacks="<<attacks<<" enemy_attacks="<<enemyAttacks<<" pages="<<pages<<" trace="<<sha256(trace)<<std::endl;
  if(g.state().at("phase")!="victory")std::ofstream("/tmp/caocao-scenario-failure.json")<<g.serialize();check(g.state().at("phase")=="victory","full autoplay did not win");check(walks>0&&attacks>0&&enemyAttacks>0&&pages>0,"missing path coverage");
  auto victory=g.serialize();g.command("restart");check(g.serialize()==start,"restart changed scenario initial state");g.restore(victory);check(g.serialize()==victory,"completed events repeated on restore");
  g.restore(start);auto bad=Json::parse(start);bad["payload"]["expansion_manifest"]="wrong";bad["sha256"]=sha256(bad.at("payload").dump());bool rejected=false;try{g.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==start,"bad identity mutated live state");
  bad=Json::parse(start);bad["payload"]["dialogue"]["page"]=999;bad["sha256"]=sha256(bad.at("payload").dump());rejected=false;try{g.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==start,"bad dialogue mutated state");
  while(g.has_dialogue())g.command("continue");auto t=Json::parse(g.serialize());t["payload"]["state"]["turn"]=20;t["sha256"]=sha256(t.at("payload").dump());g.restore(t.dump());g.command("end_turn");
  for(int n=0;n<100&&g.state().at("phase")=="enemy";++n){if(g.has_dialogue())g.command("continue");else g.enemy_step();}
  check(g.state().at("phase")=="defeat"&&g.state().at("turn_limit_reached")==true,"turn limit missing");mirror.restore(g.serialize());
  g.restore(start);for(int n=0;n<4000&&g.state().at("phase")!="defeat";++n){if(g.has_dialogue())g.command("continue");else if(g.state().at("phase")=="player")g.command("end_turn");else g.enemy_step();}
  check(g.state().at("phase")=="defeat","passive defeat missing");mirror.restore(g.serialize());std::cout<<"battle="<<scenario<<" passive_defeat_turn="<<g.state().at("turn")<<" turn_limit/save/corruption/restart passed\n";
  g.restore(original);check(g.serialize()==original&&g.scenario()==0,"opening save changed");
 }
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
