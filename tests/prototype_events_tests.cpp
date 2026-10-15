#include "caocao/prototype.hpp"
#include "caocao/prototype_events.hpp"
#include "prototype_terrain_fixture.hpp"
#include <iostream>
#include <fstream>
using namespace caocao;
namespace {
void check(bool p,const std::string& s){if(!p)throw std::runtime_error(s);}
void restore(Prototype& g,Json p){p["sha256"]=sha256(p.at("payload").dump());g.restore(p.dump());}
void drain(Prototype& g){for(int n=0;n<500&&g.has_dialogue();++n){auto snapshot=g.serialize();Prototype mirror=g;mirror.restore(snapshot);g.command("continue");mirror.command("continue");check(g.hash()==mirror.hash(),"dialogue continuation changed after restore");}check(!g.has_dialogue(),"dialogue stalled");}
void finish(Prototype& g){const auto& b=g.state().at("units").at("0").at("battle");g.command("tile",b.at("x"),b.at("y"));g.command("wait");drain(g);}
void casualties(Prototype& g,std::initializer_list<int> ids){auto p=Json::parse(g.serialize());for(int id:ids){auto& b=p["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;}restore(g,p);}
int visible(const Prototype& g){int n=0;for(const auto& u:g.state().at("units"))if(u.at("battle").at("lifecycle")==2)++n;return n;}
}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");std::filesystem::path assets=argv[1],root=assets.parent_path().parent_path();auto data=read_json(assets/"data.json");
 auto oracle=read_json(root/"tests/fixtures/original_prototype_scenario_events.json");size_t cases=0;std::string trace;
 for(const auto& c:oracle.at("cases")){
  Json i={{"opcode",c.at("opcode")},{"args",Json::array()}},state={{"units",Json::object()}};
  for(const auto& v:c.at("args"))i["args"].push_back({{"value",v}});
  for(const auto& u:c.at("units"))state["units"][std::to_string(u.at("battle").at("id").get<int>())]=u;
  bool value=prototype_script_condition(i,state,Json::object(),data,oracle.at("width"),oracle.at("height"));
  check(value==c.at("result").get<bool>(),"original condition differs in case "+std::to_string(cases)+" opcode "+c.at("opcode").dump()+" args "+c.at("args").dump());++cases;
 }
 Prototype g(assets);g.enable_revised_rules();drain(g);check(g.objective()=="击毙张宝和张梁","opening leaders objective missing");
 casualties(g,{146,147});finish(g);
 check(g.state().at("phase")=="victory","leaders-only victory missing");check(g.progress().at("treasures").at("76")==1,"Zhang Bao treasure absent");
 auto victory=g.serialize();g.restore(victory);check(g.serialize()==victory,"victory/reward restore changed state");trace+=g.hash()+"\n";
 auto corrupt=Json::parse(victory);corrupt["payload"]["progress"]["treasures"]["76"]=2;bool rejected=false;try{restore(g,corrupt);}catch(...){rejected=true;}check(rejected&&g.serialize()==victory,"forged treasure accepted");
 corrupt=Json::parse(victory);corrupt["payload"]["progress"]["reward_descriptors"][0]["cash_verified"]=true;rejected=false;try{restore(g,corrupt);}catch(...){rejected=true;}check(rejected&&g.serialize()==victory,"forged reward descriptor accepted");
 // Hidden source deployments are retained until the turn-one reinforcement
 // event finishes. Snapshot/restore is checked after every original speech page.
 g.start_scenario(1);int before=visible(g);drain(g);check(visible(g)==before+6,"Sishui initial hidden reinforcement event differs");
 for(int id:{131,129,210,211,212,213})check(g.state().at("units").at(std::to_string(id)).at("battle").at("lifecycle")==2,"reinforcement still hidden");trace+=g.hash()+"\n";
 // Ordinary Hua Xiong death shares the same original drop as his duel. It is
 // awarded once after the source death speech and remains on a later reload.
 casualties(g,{134});finish(g);check(g.progress().at("treasures").at("70")==1,"Hua Xiong drop missing");auto reward=g.serialize();g.restore(reward);check(g.serialize()==reward,"drop duplicated on reload");trace+=g.hash()+"\n";
 // The duel uses its original text and terminal battlefield effects; its
 // fullscreen animation remains the explicit native presentation adapter.
 g.start_scenario(1);drain(g);bool found=false;
 for(int y=1;y<19&&!found;++y)for(int x=1;x<19&&!found;++x){
  auto a=UnitRecord::from_json(g.state().at("units").at("3").at("record")),b=UnitRecord::from_json(g.state().at("units").at("134").at("record"));
  if(movement_cost(a,g.terrain().at("cells").at(y*20+x),data)!=255&&movement_cost(b,g.terrain().at("cells").at((y+1)*20+x),data)!=255){terrain_fixture_place(g,3,x,y);terrain_fixture_place(g,134,x,y+1);found=true;}
 }
 check(found,"missing duel fixture tile");finish(g);check(g.progress().at("flags").at("40")==1,"Guan Yu duel flag absent");check(g.state().at("units").at("134").at("battle").at("lifecycle")==3,"duel did not remove Hua Xiong");check(g.progress().at("treasures").at("70")==1,"duel reward missing");trace+=g.hash()+"\n";
 g.start_scenario(2);drain(g);casualties(g,{119});finish(g);check(g.progress().at("flags").at("50")==1,"Lu Bu rout event absent");
 for(const auto& u:g.state().at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.side>=2&&b.x<=6)check(b.lifecycle!=2,"inside gate enemy failed to flee");else if(b.side>=2&&b.lifecycle==2)check(b.status&8,"outside gate enemy failed to become confused");}trace+=g.hash()+"\n";
 // Original pursuit deployment: eighteen ambushers hidden and Dong Zhuo's
 // three departing units already removed by S_03.eex @0x195f.
 g.start_scenario(3);drain(g);int hidden=0;for(const auto& u:g.state().at("units"))if(u.at("battle").at("lifecycle")==1)++hidden;check(hidden==18,"pursuit hidden deployment differs");for(int id:{127,210,211})check(g.state().at("units").at(std::to_string(id)).at("battle").at("lifecycle")==3,"Dong Zhuo departure missing");
 int placed=0;std::vector<int> party;for(const auto& u:g.state().at("units"))if(u.at("battle").at("side")==0)party.push_back(u.at("battle").at("id"));
 for(int id:party){terrain_fixture_place(g,id,11+placed%4,16+placed/4);++placed;}
 g.command("end_turn");drain(g);while(g.state().at("phase")=="ally"){g.enemy_step();drain(g);}drain(g);check(!g.story_choices().is_null(),"pursuit retreat choice missing");check(g.story_choices().at("options").size()==2,"pursuit choice options differ");
 auto choice=g.serialize();Prototype mirror=g;mirror.restore(choice);g.choose_story(0);mirror.choose_story(0);check(g.hash()==mirror.hash(),"saved choice changed continuation");drain(g);
 check(g.progress().at("flags").at("2")==1&&g.objective().find("西端")!=std::string::npos,"retreat objective not installed");
 for(const auto& u:g.state().at("units"))check(u.at("battle").at("lifecycle")!=1,"ambush units not revealed");
 auto player=Json::parse(g.serialize());player["payload"]["state"]["phase"]="player";player["payload"]["state"]["enemy_cursor"]=0;player["payload"]["npc_order"]=Json::array();player["payload"]["state"]["units"]["0"]["battle"]["flags"]=0;restore(g,player);
 terrain_fixture_place(g,0,0,4);finish(g);check(g.state().at("phase")=="victory","west-edge escape did not complete scenario");trace+=g.hash()+"\n";g.restore(g.serialize());
 for(int scenario=1;scenario<4;++scenario){
  g.start_scenario(scenario);drain(g);auto all=Json::parse(g.serialize());for(auto& u:all["payload"]["state"]["units"])if(u.at("battle").at("side").get<int>()>=2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}restore(g,all);finish(g);
  check(g.state().at("phase")=="victory","scenario victory exit missing: "+std::to_string(scenario));auto stable=g.serialize();g.restore(stable);check(g.serialize()==stable,"scenario reward repeated on reload");trace+=g.hash()+"\n";
 }
 // Start the last permitted turn from a stable board, then run the real allied
 // and enemy phases. The limit must finish at turn 20 and remain restorable.
 for(int scenario=0;scenario<4;++scenario){
  g.start_scenario(scenario);drain(g);auto last=Json::parse(g.serialize());last["payload"]["state"]["turn"]=20;restore(g,last);g.command("end_turn");
  for(int n=0;n<1000;++n){
   drain(g);if(!g.story_choices().is_null()){g.choose_story(0);continue;}
   if(g.state().at("phase")=="defeat"||g.state().at("phase")=="victory")break;
   auto stable=g.serialize();Prototype resumed=g;resumed.restore(stable);g.enemy_step();resumed.enemy_step();check(g.hash()==resumed.hash(),"last-turn continuation changed after restoration");
  }
  drain(g);check(g.state().at("phase")=="defeat"&&g.state().at("turn")==20&&g.state().at("turn_limit_reached")==true,"revised turn-limit exit missing: "+std::to_string(scenario));
  auto stable=g.serialize();g.restore(stable);check(g.serialize()==stable,"turn-limit defeat did not restore");trace+=g.hash()+"\n";
 }
 std::cout<<cases<<" original condition oracle cases; leaders/rewards/reinforcements/ambush/retreat/turn limits/save continuity passed; trace="<<sha256(trace)<<'\n';
 if(argc>2)std::ofstream(argv[2])<<trace;
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
