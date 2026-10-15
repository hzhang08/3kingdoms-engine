#include "caocao/prototype.hpp"
#include "caocao/range.hpp"
#include "caocao/prototype_playback.hpp"
#include "prototype_terrain_fixture.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool p,const char* why){if(!p)throw std::runtime_error(why);}
void dialogues(Prototype& g){while(g.has_dialogue())g.command("continue");}
void reseal(Json& e){e["sha256"]=sha256(e.at("payload").dump());}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");auto assets=std::filesystem::path(argv[1]);auto root=assets.parent_path().parent_path();
 Prototype g(assets);g.enable_items();
 if(argc>3&&std::string(argv[2])=="--resume"){
  std::ifstream f(argv[3]);std::string snapshot{std::istreambuf_iterator<char>(f),{}};g.restore(snapshot);check(g.serialize()==snapshot,"cross-platform restore changed");auto before=g.hash();if(g.has_dialogue())g.command("continue");else if(g.state().at("phase")=="enemy")g.enemy_step();else g.command("end_turn");std::cout<<"restored "<<before<<" next "<<g.hash()<<'\n';return 0;
 }
 auto oracle=read_json(root/"tests/fixtures/original_prototype_items.json");int comparisons=0;
 for(const auto& c:oracle.at("cases")){
  int item=c.at("item"),target=c.at("target");
  for(int n=0;n<2;++n){auto b=BattleUnit::from_json(c.at("before").at(n));auto expected=BattleUnit::from_json(c.at("after").at(n));
   auto effect=target==b.slot?consumable_effect(item,g.data().at("items").at(item).at("strength"),c.at("maximum"),b.hp,b.status):ConsumableEffect{b.hp,b.status};
   check(effect.hp==expected.hp&&effect.status==expected.status&&b.mp==expected.mp,"original consumable effect mismatch");
  }
  check(c.at("stock")== (target==255?2:1)&&c.at("rng")==1,"original stock/RNG mismatch");++comparisons;
 }
 for(const auto& c:oracle.at("ranges")){
  std::vector<BattleUnit> units;for(const auto& u:c.at("roster"))units.push_back(BattleUnit::from_json(u));auto b=BattleUnit::from_json(c.at("actor"));
  auto actual=battle_range(b,{b.x,b.y},1,1,false,255,20,20,{std::vector<int>(400,255),std::vector<int>(400,255),std::vector<int>(400)},units);
  check(Json(actual.targets)==c.at("targets"),"original friendly item range mismatch");
 }
 std::string trace;
 auto collect=[&](int scenario,int x,int y){g.start_scenario(scenario);dialogues(g);terrain_fixture_place(g,0,x,y);g.command("tile",x,y);g.command("wait");dialogues(g);terrain_fixture_next_player(g);g.enemy_step();dialogues(g);g.take_environment();};
 // Self and adjacent ally, three actual location items; actor spends action, target does not.
 for(int item:{87,88,92})for(bool ally:{false,true}){
  collect(item==92?1:item==88?3:0,item==92?2:item==88?8:13,item==92?11:item==88?9:12);
  terrain_fixture_place(g,0,12,10,1);int target=0;
  if(ally){for(const auto& u:g.state().at("units")){int id=u.at("battle").at("id");if(id!=0&&u.at("battle").at("slot").get<int>()<35){target=id;break;}}check(target!=0,"missing ally");terrain_fixture_place(g,target,13,11,1);}
  if(item==92){auto e=Json::parse(g.serialize());e["payload"]["state"]["units"][std::to_string(target)]["battle"]["status"]=16;reseal(e);g.restore(e.dump());}
  g.command("tile",12,10);auto before=g.state();auto saved=g.serialize();Prototype clone=g;clone.restore(saved);
  g.use_item(item,target);clone.use_item(item,target);check(g.hash()==clone.hash(),"item replay changed");
  auto b=g.state().at("units").at(std::to_string(target)).at("battle");
  check(b.at("hp")== (item==92?1:item==87?31:81)&&!(b.at("status").get<int>()&16),"item HP/poison wrong");
  check(g.inventory().at(std::to_string(item))==0&&(g.state().at("units").at("0").at("battle").at("flags").get<int>()&2),"item did not spend stock/action");
  check(before.at("rng")==g.state().at("rng")&&b.at("mp")==before.at("units").at(std::to_string(target)).at("battle").at("mp"),"item spent RNG/MP");
  check(g.state().at("units").at("0").at("record")==before.at("units").at("0").at("record"),"item changed XP/equipment");
  if(ally)check(!(b.at("flags").get<int>()&2),"recipient spent action");
  auto packet=g.take_environment();check(!packet.is_null()&&packet.at("events").at(0).at("target")==target,"item feedback missing");PrototypePlayback playback;playback.start(nullptr,nullptr,packet);check(playback.notice().at("item")==item,"item icon missing");check(playback.take_sounds()==std::vector<int>{41},"item sound missing");int ticks=0;while(playback.active()){playback.advance(40);check(++ticks<100,"item playback stalled");}
  saved=g.serialize();g.restore(saved);check(g.serialize()==saved,"spent inventory restore failed");trace+=g.hash()+"\n";
 }
 collect(0,13,12);terrain_fixture_place(g,0,12,10,1);g.command("tile",12,10);auto valid=g.serialize();
 for(auto [item,target]:std::vector<std::pair<int,int>>{{87,146},{87,3},{88,0},{92,0},{99,0},{87,999}}){bool rejected=false;try{g.use_item(item,target);}catch(...){rejected=true;}check(rejected&&g.serialize()==valid,"illegal item changed state");}
 // Cancel tentative movement has no inventory/action/RNG effect, then use at full HP (original permits waste).
 g.command("tile",12,11);check(g.inventory().at("87")==1,"tentative move spent item");g.command("cancel");check(g.inventory().at("87")==1,"cancel spent item");
 terrain_fixture_place(g,0,12,10,g.state().at("units").at("0").at("max_hp"));g.command("tile",12,10);auto hp=g.state().at("units").at("0").at("battle").at("hp");g.use_item(87,0);check(g.inventory().at("87")==0&&g.state().at("units").at("0").at("battle").at("hp")==hp,"full HP behavior differs");
 valid=g.serialize();for(int n=0;n<7;++n){auto bad=Json::parse(valid);auto& p=bad["payload"];if(n==0)p["consumed_items"]["87"]=-1;if(n==1)p["consumed_items"]["87"]=2;if(n==2)p["consumed_items"]["87"]=1.0;if(n==3)p["consumed_items"]["100"]=0;if(n==4)p["benefits"]["inventory"]["87"]=1;if(n==5)p["items_manifest"]="wrong";if(n==6)p.erase("consumed_items");reseal(bad);bool rejected=false;try{g.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==valid,"corrupt spent inventory accepted or changed game");}
 dialogues(g);terrain_fixture_next_player(g);g.enemy_step();dialogues(g);terrain_fixture_place(g,0,13,12);g.command("tile",13,12);g.command("wait");check(g.inventory().at("87")==0,"revisiting replenished spent item");trace+=g.hash()+"\n";
 // Real schema7 collected inventory restores byte-for-byte, upgrades without touching progress.
 Prototype old(assets);old.enable_terrain_benefits();dialogues(old);terrain_fixture_place(old,0,13,12);old.command("tile",13,12);old.command("wait");auto schema7=old.serialize();g.restore(schema7);check(g.serialize()==schema7,"schema7 restore changed");auto state=g.state();g.enable_items();check(g.state()==state&&g.inventory().at("87")==1&&Json::parse(g.serialize()).at("payload").at("consumed_items").at("87")==0,"schema7 upgrade lost progress");trace+=g.hash()+"\n";
 dialogues(g);terrain_fixture_place(g,146,13,11,1);auto awake=Json::parse(g.serialize());awake["payload"]["state"]["units"]["146"]["battle"]["status"]=0;reseal(awake);g.restore(awake.dump());g.command("end_turn");dialogues(g);auto req=g.enemy_request();check(!req.is_null()&&req.at("state").at("protocol")=="caocao-items-v4"&&req.at("state").at("capabilities").at("items").at("enemy_inventory_access")==false,"JEV item context absent");g.choose_enemy(req.at("key"),"a0");auto saved=g.serialize();Prototype clone=g;clone.restore(saved);g.enemy_step();clone.enemy_step();check(g.hash()==clone.hash(),"JEV saved choice replay changed");trace+=g.hash()+"\n";
 if(argc>2){std::string out=argv[2];std::ofstream(out)<<Json{{"original_cases",comparisons},{"range_cases",8},{"trace_sha256",sha256(trace)}}.dump(2);std::ofstream(out+".snapshot.json")<<g.serialize();std::ofstream(out+".schema7.json")<<schema7;std::ofstream(out+".jev.json")<<req.dump(2);}
 std::cout<<comparisons<<" original item effects; 8 ranges; stock/action, self/ally, cancellation, corrupt saves, migration and JEV passed: "<<sha256(trace)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
