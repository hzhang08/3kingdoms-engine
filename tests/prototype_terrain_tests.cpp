#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/recovery.hpp"
#include "prototype_terrain_fixture.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool pass,const std::string& why){if(!pass)throw std::runtime_error(why);}
void dialogues(Prototype& g){while(g.has_dialogue())g.command("continue");}
void reseal(Json& e){e["sha256"]=sha256(e.at("payload").dump());}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");auto assets=std::filesystem::path(argv[1]);auto root=assets.parent_path().parent_path();
 if(argc>3&&std::string(argv[2])=="--resume"){
  std::ifstream f(argv[3]);std::string snapshot{std::istreambuf_iterator<char>(f),{}};Prototype g(assets);g.restore(snapshot);check(g.serialize()==snapshot,"cross-platform snapshot changed");auto before=g.hash();
  if(g.has_dialogue())g.command("continue");else if(g.state().at("phase")=="enemy")g.enemy_step();else if(g.state().at("phase")=="player")g.command("end_turn");
  std::cout<<"restored "<<before<<" next "<<g.hash()<<'\n';return 0;
 }
 auto oracle=read_json(root/"tests/fixtures/original_terrain_pickups.json");int comparisons=0;
 for(const auto& c:oracle.at("conditions")){BattleUnit b;b.slot=c.at("slot");b.hp=c.at("hp");b.lifecycle=c.at("lifecycle");b.x=c.at("site").at("x").get<int>()+c.at("offset").get<int>();b.y=c.at("site").at("y");
  check(location_pickup_eligible(b,c.at("site").at("x"),b.y,c.at("claimed").get<int>()!=0)==c.at("eligible"),"original pickup condition mismatch");++comparisons;
 }
 // Table/rounding/cap agreement is also covered by the original maintenance oracle.
 for(int t=0;t<28;++t)for(int maximum:{1,79,100,101,499})for(int hp:{0,1,maximum-1,maximum}){
  int rate=t>=18&&t<=22?std::array{25,20,20,15,20}[t-18]:0;
  check(terrain_recovery_hp(maximum,hp,t)==(hp?std::min(maximum-hp,maximum*rate/100):0),"terrain arithmetic");
 }
 Prototype game(assets);game.enable_terrain_benefits();std::string trace;Json reports=Json::array();
 auto sites=read_json(assets/"terrain-benefits/catalog.json").at("locations");
 for(const auto& site:sites){int scenario=site.at("scenario"),x=site.at("x"),y=site.at("y"),item=site.at("item");game.start_scenario(scenario);dialogues(game);
  terrain_fixture_place(game,0,x,y);auto old=game.state();auto rng=old.at("rng");game.command("tile",x,y);game.command("wait");
  check(game.inventory().at(std::to_string(item))==1,"location did not grant item");check(game.state().at("rng")==rng,"pickup changed RNG");
  auto packet=game.take_environment();check(!packet.is_null()&&packet.at("events").at(0).at("item")==item,"pickup presentation absent");
  auto saved=game.serialize();Prototype clone=game;clone.restore(saved);check(clone.serialize()==saved,"inventory did not restore");
  // Revisit through a committed action after a phase transition, not a direct flag edit.
  dialogues(game);terrain_fixture_next_player(game);game.enemy_step();dialogues(game);game.command("tile",x,y);game.command("wait");
  check(game.inventory().at(std::to_string(item))==1&&game.take_environment().is_null(),"repeat collection");
  trace+=game.hash()+"\n";reports.push_back({{"scenario",scenario},{"x",x},{"y",y},{"item",item}});
 }
 // A legal move onto the original camp is tentative until Wait/attack/End Turn.
 game.start_scenario(0);dialogues(game);terrain_fixture_place(game,0,13,11);game.command("tile",13,11);game.command("tile",13,12);
 check(game.inventory().at("87")==0&&game.take_environment().is_null(),"tentative pickup");auto pending=game.serialize();
 game.command("cancel");check(game.inventory().at("87")==0,"cancel collected item");game.restore(pending);game.command("wait");check(game.inventory().at("87")==1,"saved tentative move failed to commit");
 Prototype clone=game;clone.restore(pending);clone.command("wait");check(game.hash()==clone.hash(),"move restore changed pickup/RNG");
 game.restore(pending);game.command("end_turn");check(game.inventory().at("87")==1,"end turn did not commit pickup");
 trace+=game.hash()+"\n";
 // Physical strike presentation completes before a pickup at the actor's tile.
 game.start_scenario(0);dialogues(game);terrain_fixture_place(game,0,13,12);terrain_fixture_place(game,146,13,11);
 game.command("tile",13,12);game.command("tile",13,11);auto strike=game.take_attack(),pickup=game.take_environment();
 check(!strike.is_null()&&!pickup.is_null()&&game.inventory().at("87")==1,"attack did not commit pickup");
 PrototypePlayback sequence;sequence.start(nullptr,strike,pickup);bool saw_pickup=false;int frames=0;
 while(sequence.active()){if(sequence.packet().value("kind",std::string())=="environment"){saw_pickup=true;check(sequence.before().at("units").at("146").at("battle").at("hp")==game.state().at("units").at("146").at("battle").at("hp"),"pickup revealed pre-attack HP");}sequence.advance(40);check(++frames<1000,"attack/pickup sequence stalled");}
 check(saw_pickup,"pickup skipped after attack");trace+=game.hash()+"\n";
 // Actual phase boundaries: both armies heal, MP/RNG do not change, and poison precedes regeneration.
 for(bool enemy:{false,true}){
  game.start_scenario(0);dialogues(game);int id=enemy?146:0;terrain_fixture_place(game,id,13,12,1);
  if(!enemy)terrain_fixture_next_player(game);auto before=game.state();auto b=before.at("units").at(std::to_string(id));int amount=terrain_recovery_hp(b.at("max_hp"),1,22);
  auto snapshot=game.serialize();clone=game;clone.restore(snapshot);
  if(enemy){game.command("end_turn");clone.command("end_turn");}else{game.enemy_step();clone.enemy_step();}
  check(game.hash()==clone.hash(),"phase recovery replay changed");auto after=game.state().at("units").at(std::to_string(id)).at("battle");
  check(after.at("hp")==1+amount&&after.at("mp")==b.at("battle").at("mp")&&game.state().at("rng")==before.at("rng"),"HP/MP/RNG recovery mismatch");
  auto packet=game.take_environment();check(!packet.is_null(),"recovery animation missing");PrototypePlayback p;p.start(nullptr,nullptr,packet);check(p.active()&&p.number().at("healing")==true,"recovery display did not start");
  auto state=game.hash();int ticks=0;while(p.active()){p.advance(40);check(++ticks<500,"environment playback stalled");}check(game.hash()==state,"presentation changed simulation");
  trace+=state+"\n";
 }
 bool poisoned=false;for(int seed=1;seed<40&&!poisoned;++seed){
  game.start_scenario(0,seed);dialogues(game);terrain_fixture_place(game,0,13,12,1);terrain_fixture_next_player(game);
  auto e=Json::parse(game.serialize());e["payload"]["state"]["units"]["0"]["battle"]["status"]=16;reseal(e);game.restore(e.dump());game.enemy_step();
  auto b=game.state().at("units").at("0");if(b.at("battle").at("status").get<int>()&16){check(b.at("battle").at("hp")==1+terrain_recovery_hp(b.at("max_hp"),1,22),"poison/healing order mismatch");poisoned=true;}
 }check(poisoned,"poison order case absent");
 // Old snapshots restore exactly in the engine; frontend opt-in leaves progress untouched.
 for(bool tactics:{false,true}){Prototype old(assets);if(tactics)old.enable_tactics();old.start_scenario(1);dialogues(old);auto save=old.serialize();auto state=old.state();game.restore(save);check(game.serialize()==save,"legacy snapshot changed");game.enable_terrain_benefits();check(game.state()==state&&game.inventory().at("87")==0&&game.take_environment().is_null(),"migration altered progress or granted item");}
 auto valid=game.serialize();for(int test=0;test<5;++test){auto bad=Json::parse(valid);auto& p=bad["payload"];
  if(test==0)p["benefits"]["inventory"]["87"]=1;
  if(test==1)p["benefits"]["claimed"]={100000};
  if(test==2)p["benefits"]["claimed"]={8457,8457};
  if(test==3)p["terrain_manifest"]="wrong";
  if(test==4)p["benefits"]["inventory"]["87"]=0.0;
  reseal(bad);bool rejected=false;try{game.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&game.serialize()==valid,"corrupt inventory accepted or mutated save");
 }
 // Healing alternatives and capability identity reach the existing JEV path.
 game.start_scenario(0);dialogues(game);terrain_fixture_place(game,146,13,11,1);
 auto awake=Json::parse(game.serialize());awake["payload"]["state"]["units"]["146"]["battle"]["status"]=0;reseal(awake);game.restore(awake.dump());game.command("end_turn");auto req=game.enemy_request();
 check(!req.is_null()&&req.at("state").at("protocol")=="caocao-terrain-v3","missing JEV terrain context");bool healing=false;for(const auto& c:req.at("choices"))if(c.at("terrain_heal_next_phase").get<int>()>0)healing=true;check(healing,"JEV omitted healing choices");
 game.choose_enemy(req.at("key"),"a0");auto save=game.serialize();clone=game;clone.restore(save);game.enemy_step();clone.enemy_step();check(game.hash()==clone.hash(),"saved terrain decision changed");trace+=game.hash()+"\n";
 if(argc>2){auto out=std::filesystem::path(argv[2]);std::ofstream(out)<<Json{{"original_condition_cases",comparisons},{"stock_oracle_cases",oracle.at("stock").size()},{"locations",reports},{"trace_sha256",sha256(trace)}}.dump(2);std::ofstream(out.string()+".jev-request.json")<<req.dump(2);std::ofstream(out.string()+".snapshot.json")<<game.serialize();}
 std::cout<<comparisons<<" original location conditions; eight sites, phase healing, cancel/commit, migration/corruption, playback, JEV replay passed: "<<sha256(trace)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
