#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/experience.hpp"
#include "caocao/range.hpp"
#include "prototype_terrain_fixture.hpp"
#include <fstream>
#include <iostream>
#include <set>

namespace caocao {
// Internal pure resolver boundary; not an alternate save/import interface.
Json resolve_prototype_combat(const Json&,int,int,const Json&,const Json&,const Json&);
int prototype_pursuit_target(const BattleUnit&,const UnitRecord&,const BattleUnit&,int,int,const std::vector<BattleUnit>&,const Json&);
}
using namespace caocao;
namespace {
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
void dialogues(Prototype& g){while(g.has_dialogue()||!g.story_choices().empty()){if(!g.story_choices().empty())g.choose_story(0);else g.command("continue");}}
void reseal(Json& j){j["sha256"]=sha256(j.at("payload").dump());}
Json controller_state(const Json& c,const Json& data){
 Json s={{"rng",c.at("spec").at("seed")},{"units",Json::object()}};
 for(const auto& u:c.at("before")){
  auto r=UnitRecord::from_json(u.at("unit"));
  s["units"][std::to_string(r.id)]={{"record",r.json()},{"battle",u.at("battle")},{"max_hp",std::min(500,r.hp+equipment_bonus(r,5,data))},{"name",data.at("heroes").at(r.id).at("name")}};
 }
 return s;
}
int hero_for_slot(const Json& state,int slot){for(const auto& u:state.at("units"))if(u.at("battle").at("slot")==slot)return u.at("battle").at("id");throw std::runtime_error("missing fixture slot");}
}

int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("prototype assets required");
 auto assets=std::filesystem::path(argv[1]),root=assets.parent_path().parent_path();
 auto oracle=read_json(root/"tests/fixtures/original_attack_controller.json");
 auto data=read_json(assets/"data.json"),terrain=read_json(assets/"terrain.json"),texts=read_json(root/"assets/generated/combat_text.json");
 size_t comparisons=0,strike_count=0,counters=0,doubles=0,criticals=0;std::string trace;
 for(const auto& c:oracle.at("cases")){
  auto before=controller_state(c,data);int actor=hero_for_slot(before,c.at("spec").at("actor")),target=hero_for_slot(before,c.at("spec").at("target"));
  auto actual=resolve_prototype_combat(before,actor,target,terrain,data,texts);Json reference_trace=Json::array();
  for(const auto& e:c.at("events"))if(e.contains("units")&&e.at("kind")!="assembled")reference_trace.push_back(e);
  check(actual.at("trace")==reference_trace,"original controller trace differs at case "+std::to_string(comparisons));
  check(actual.at("state").at("rng")==c.at("rng")&&actual.at("tick")==c.at("tick")&&actual.at("depth")==c.at("depth"),"original controller RNG/tick/depth differs");
  for(const auto& u:c.at("after")){
   auto id=std::to_string(u.at("unit").at("id").get<int>());const auto& out=actual.at("state").at("units").at(id);
   check(out.at("battle")==u.at("battle")&&out.at("record")==u.at("unit"),"original controller final record differs");
  }
  for(const auto& s:actual.at("strikes")){++strike_count;if(s.at("counter_depth").get<int>()>0)++counters;if(s.at("round")==1)++doubles;if(s.at("critical").get<bool>())++criticals;}
  trace+=sha256(actual.dump())+"\n";++comparisons;
 }
 check(counters>0&&doubles>0&&criticals>0,"oracle omitted required attack branches");

 auto pursuit=read_json(root/"tests/fixtures/original_prototype_pursuit.json");int pursuit_cases=0,pursuit_callers=0;
 for(const auto& c:pursuit.at("cases")){
  BattleUnit a,b;a.id=119;a.slot=35;a.side=2;a.x=a.y=10;a.lifecycle=2;a.hp=100;
  b.id=0;b.slot=0;b.side=0;b.x=10;b.y=9;b.lifecycle=2;b.hp=c.at("initial_target_hp");
  auto r=load_unit_record(data.at("heroes").at(119));r.job=9;r.equipment={{{c.at("item").get<int>(),1,0},{255,255,255},{255,255,255}}};
  std::vector<BattleUnit> board{a,b};
  for(int id:c.at("living_targets")){auto t=b;t.id=t.slot=id;t.hp=100;t.x=id==1?11:id==3?9:10;t.y=id==2?11:10;board.push_back(t);}
  auto range=battle_range(a,{10,10},battle_attack_range(r,data),0,false,c.at("preferred"),20,20,{std::vector<int>(400,255),std::vector<int>(400,255),std::vector<int>(400)},board);
  int expected=c.at("calls").empty()?255:c.at("calls").at(1).at("target").get<int>();
  int observed=b.hp==0&&equipment_effect(r,46,data)?range.selected:255;
  check(observed==expected,"original pursuit gate/range dispatch differs");
  if(c.at("preferred")==35){check(prototype_pursuit_target(a,r,b,20,20,board,data)==expected,"production pursuit selector differs");++pursuit_callers;}
  ++pursuit_cases;
 }

 // Character and equipment advancement in a complete attack, including capped
 // HP damage, an already-confused target and an equipment XP boundary.
 Prototype source(assets);source.enable_revised_rules();auto state=source.state();
 auto& ar=state["units"]["0"]["record"];ar["xp"]=99;ar["equipment"][0][2]=99;
 auto& a=state["units"]["0"]["battle"];a["x"]=12;a["y"]=10;a["direction"]=1;
 auto& b=state["units"]["474"]["battle"];b["x"]=13;b["y"]=10;b["hp"]=1;b["status"]=8;
 auto initial_actor=UnitRecord::from_json(ar);int hp=a.at("hp");
 auto grown=resolve_prototype_combat(state,0,474,source.terrain(),data,texts);const auto& after=grown.at("state").at("units").at("0");
 check(after.at("record").at("level").get<int>()==initial_actor.level+1,"attack did not grant level-up");
 check(after.at("record").at("equipment").at(0).at(1).get<int>()==initial_actor.equipment[0][1]+1,"weapon XP did not level equipment");
 check(after.at("battle").at("hp")==hp,"no-animation XP path invented a battle heal");
 check(after.at("max_hp").get<int>()>state.at("units").at("0").at("max_hp").get<int>(),"grown maximum HP not exposed");
 bool level_notice=false,record_update=false;for(const auto& e:grown.at("events")){
  if(e.at("kind")=="notice"&&e.at("text").get<std::string>().find("升至")!=std::string::npos)level_notice=true;
  if(e.at("kind")=="record_state"&&e.at("actor")==0)record_update=true;
 }
 check(level_notice&&record_update,"XP/level presentation absent");trace+=sha256(grown.dump())+"\n";

 // A Lu Bu kill pursues the last eligible target once; a second kill cannot
 // recursively grant another pursuit. Each controller still has normal doubles.
 source.start_scenario(2);auto duel=source.state();Json subset=Json::object();subset["119"]=duel.at("units").at("119");
 auto& lu=subset["119"]["battle"];lu["x"]=10;lu["y"]=10;lu["direction"]=0;
 std::vector<int> victims;for(const auto& u:duel.at("units"))if(u.at("battle").at("side")==0&&victims.size()<3)victims.push_back(u.at("battle").at("id"));
 check(victims.size()==3,"pursuit test needs three party units");
 for(int n=0;n<3;++n){auto id=std::to_string(victims[n]);subset[id]=duel.at("units").at(id);auto& t=subset[id]["battle"];t["x"]=n==2?9:10;t["y"]=n==0?9:n==1?11:10;t["hp"]=1;t["status"]=8;}
 duel["units"]=subset;duel["rng"]=1;
 auto pursuit_attack=resolve_prototype_combat(duel,119,victims[0],source.terrain(),data,texts);std::set<int> attacked;
 for(const auto& s:pursuit_attack.at("strikes"))if(s.at("actor")==119)attacked.insert(s.at("target").get<int>());
 check(attacked==std::set<int>{victims[0],victims[2]},"pursuit did not choose last target once");
 check(pursuit_attack.at("state").at("units").at(std::to_string(victims[1])).at("battle").at("hp")==1,"pursuit chained into third target");
 trace+=sha256(pursuit_attack.dump())+"\n";

 // Resolver inputs remain intact when a previously unverified area controller
 // is requested. Public attack commits only a fully built copy of the game.
 auto unsupported=state;unsupported["units"]["0"]["record"]["class"]=16;auto stable=unsupported.dump();bool rejected=false;
 try{resolve_prototype_combat(unsupported,0,474,source.terrain(),data,texts);}catch(...){rejected=true;}
 check(rejected&&unsupported.dump()==stable,"unsupported physical area action was not atomic");

 // Actual player command, original counter, multi-strike playback and schema9
 // save/reload. Test placement is isolated synthetic input, not a migration.
 Prototype game(assets);game.enable_revised_rules();dialogues(game);
 terrain_fixture_place(game,0,12,10);terrain_fixture_place(game,474,13,10);
 auto setup=Json::parse(game.serialize());setup["payload"]["state"]["rng"]=0;setup["payload"]["state"]["units"]["474"]["battle"]["status"]=0;reseal(setup);game.restore(setup.dump());
 game.command("tile",12,10);auto checkpoint=game.serialize();Prototype mirror=game;mirror.restore(checkpoint);
 game.command("tile",13,10);mirror.command("tile",13,10);
 check(game.hash()==mirror.hash(),"restored next attack changed state/RNG");
 auto packet=game.take_attack();check(packet.at("strikes").size()>=2,"real command omitted counterattack");
 check(game.state().at("units").at("0").at("record").at("xp").get<int>()>0,"real command did not award XP");
 check(game.state().at("units").at("0").at("battle").at("flags")==2,"attacker action not committed");
 check(game.state().at("units").at("474").at("battle").at("flags")==0,"counter consumed defender's turn");
 PrototypePlayback playback;playback.start(packet);int ticks=0;std::set<int> active_actors;bool record_visible=false;
 while(playback.active()){
  if(!playback.notice().is_null())active_actors.insert(playback.notice().at("actor").get<int>());
  if(playback.before().at("units").at("0").at("record").at("xp").get<int>()>0)record_visible=true;
  playback.advance(40);check(++ticks<1000,"multi-strike presentation stalled");
 }
 check(active_actors.contains(0)&&active_actors.contains(474)&&record_visible,"counter/record update missing during playback");
 auto snapshot=game.serialize();game.restore(snapshot);check(game.serialize()==snapshot,"XP save restore changed state");
 trace+=game.hash()+"\n";
 // The public serializer must preserve nonzero XP and reject altered derived
 // statistics even if an attacker recomputes the outer checksum.
 auto corrupt=Json::parse(snapshot);auto& stats=corrupt["payload"]["state"]["units"]["0"]["record"]["stats"];stats[0]=stats[0].get<int>()+1;reseal(corrupt);rejected=false;
 try{game.restore(corrupt.dump());}catch(...){rejected=true;}
 check(rejected&&game.serialize()==snapshot,"forged growth stats accepted or mutated live state");

 game.start();dialogues(game);terrain_fixture_place(game,0,12,10);terrain_fixture_place(game,474,13,10,1);
 auto level_input=Json::parse(game.serialize());auto& level_record=level_input["payload"]["state"]["units"]["0"]["record"];
 level_record["xp"]=99;level_record["equipment"][0][2]=99;level_input["payload"]["state"]["units"]["474"]["battle"]["status"]=8;reseal(level_input);game.restore(level_input.dump());
 int level=game.state().at("units").at("0").at("record").at("level");game.command("tile",12,10);game.command("tile",13,10);
 check(game.state().at("units").at("0").at("record").at("level")==level+1,"public action did not level character");
 auto leveled_snapshot=game.serialize();game.restore(leveled_snapshot);check(game.serialize()==leveled_snapshot,"level-up snapshot rejected or changed");trace+=game.hash()+"\n";

 // A normal attack must not reveal hidden reinforcements or restore living
 // units that departed in S_03's opening script.
 game.start_scenario(3);dialogues(game);Json inactive=Json::object();int enemy=-1;
 for(const auto& u:game.state().at("units")){auto live=BattleUnit::from_json(u.at("battle"));if(live.lifecycle!=2)inactive[std::to_string(live.id)]=live.lifecycle;else if(live.side==2&&enemy<0)enemy=live.id;}
 check(enemy>=0&&!inactive.empty(),"lifecycle fixture absent");bool placed=false;
 auto ra=UnitRecord::from_json(game.state().at("units").at("0").at("record")),rb=UnitRecord::from_json(game.state().at("units").at(std::to_string(enemy)).at("record"));
 for(int y=0;y<game.map_height()&&!placed;++y)for(int x=0;x<std::min(9,game.map_width()-1)&&!placed;++x){
  if(movement_cost(ra,game.terrain().at("cells").at(y*game.map_width()+x),data)==255||movement_cost(rb,game.terrain().at("cells").at(y*game.map_width()+x+1),data)==255)continue;
  terrain_fixture_place(game,0,x,y);terrain_fixture_place(game,enemy,x+1,y);game.command("tile",x,y);game.command("tile",x+1,y);placed=true;
 }
 check(placed,"no legal lifecycle fixture tile");for(auto it=inactive.begin();it!=inactive.end();++it)check(game.state().at("units").at(it.key()).at("battle").at("lifecycle")==it.value(),"attack changed unrelated hidden/retreated deployment");

 // An older ruleset remains deliberately unchanged until explicitly upgraded.
 Prototype old(assets);old.enable_items();auto historical=old.serialize();game.restore(historical);
 check(!game.revised_rules()&&game.serialize()==historical,"legacy snapshot silently changed combat rules");
 Json report={{"original_controller_cases",comparisons},{"strikes",strike_count},{"counters",counters},{"double_strikes",doubles},{"critical_strikes",criticals},
  {"original_pursuit_cases",pursuit_cases},{"pursuit_caller_cases",pursuit_callers},{"single_followup_no_chain",true},
  {"record_growth",true},{"levelup_save_restore",true},{"hidden_retreat_lifecycle_preserved",true},{"current_hp_preserved_on_growth",true},{"command_save_replay",true},{"playback_record_and_counter",true},{"trace_sha256",sha256(trace)}};
 if(argc>2){std::ofstream(argv[2])<<report.dump(2);std::ofstream(std::string(argv[2])+".snapshot.json")<<snapshot;std::ofstream(std::string(argv[2])+".packet.json")<<packet.dump(2);}
 std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
