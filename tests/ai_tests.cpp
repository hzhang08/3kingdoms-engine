#include "caocao/ai.hpp"
#include "caocao/tactics.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_ai_priority.json");
 for(const auto& c:fixture.at("cases")){
  std::vector<BattleUnit> units;std::vector<UnitRecord> records;
  for(const auto& row:c.at("roster")){
   auto u=UnitRecord::from_json(row.at("unit"));records.push_back(u);const auto& live=row.at("live");BattleUnit b;
   b.id=u.id;b.slot=live.at(4);b.side=live.at(5);b.x=live.at(6);b.y=live.at(7);b.lifecycle=live.at(12);b.flags=live.at(13);b.ai=live.at(14);
   b.hp=live.at(16).get<int>()+(live.at(17).get<int>()<<8);units.push_back(b);
  }
  std::vector<int> expected;for(const auto& e:c.at("order"))expected.push_back(e.at("slot"));
  // JSON stores records by ID elsewhere; caller order must not affect results.
  std::reverse(units.begin(),units.end());
  auto actual=battle_ai_order(units,records,c.at("allied"),20,c.at("terrain"),data);
  if(actual!=expected)throw std::runtime_error("original AI order mismatch: "+Json(actual).dump()+" != "+Json(expected).dump());
 }
 std::cout<<fixture.at("cases").size()<<" original AI-priority orders passed\n";
 auto gate_fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_player_phase.json");
 for(const auto& c:gate_fixture.at("cases")){
  std::vector<BattleUnit> roster(115);
  for(int slot=0;slot<115;++slot){roster[slot].slot=slot;roster[slot].lifecycle=0;}
  for(const auto& row:fixture.at("cases").at(0).at("roster")){
   const auto& live=row.at("live");auto& b=roster.at(live.at(4).get<int>());
   b.x=live.at(6);b.y=live.at(7);b.lifecycle=live.at(12);b.flags=live.at(13);b.ai=live.at(14);
  }
  const auto& input=c.at("input");if(!input.is_null()){
   auto& b=roster.at(input.at("slot").get<int>());b.x=5;b.y=5;
   b.lifecycle=input.at("lifecycle");b.flags=input.at("flags");b.ai=input.at("ai");
  }
  std::reverse(roster.begin(),roster.end());auto gate=player_phase_gate(roster);
  if(gate.requires_input!=c.at("requires_input")||Json(gate.focus)!=c.at("focus").at(0))throw std::runtime_error("player-phase gate/focus mismatch: "+input.dump());
 }
 std::cout<<gate_fixture.at("cases").size()<<" original player-phase gates and focus requests passed\n";
 auto attacks=read_json(root.parent_path().parent_path()/"tests/fixtures/original_ai_attack.json");size_t index=0;
 for(const auto& c:attacks.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));
  std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
  for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  for(const auto& r:c.at("records"))records.push_back(UnitRecord::from_json(r));
  auto result=score_ai_attack(b,u,c.at("center").get<std::array<int,2>>(),c.at("priority_id"),roster,records,c.at("width"),c.at("height"),c.at("terrain"),data);
  if(result.score!=c.at("score")||result.target!=c.at("target"))throw std::runtime_error("AI attack score mismatch "+std::to_string(index)+": "+std::to_string(result.score)+" vs "+c.at("score").dump());
  ++index;
 }
 std::cout<<index<<" original single-target AI attack scores and target choices passed\n";
 // A class with an unported area attack must stop before producing a score.
 const auto& c=attacks.at("cases").at(0);auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));u.job=16;
 std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
 for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
 for(const auto& r:c.at("records"))records.push_back(UnitRecord::from_json(r));
 auto target=std::find_if(roster.begin(),roster.end(),[](const auto& r){return r.slot==35;});
 if(target==roster.end()||target->y<4)throw std::runtime_error("area guard fixture target invalid");
 target->hp=1;bool rejected=false;
 try{score_ai_attack(b,u,{target->x,target->y-4},0,roster,records,20,20,c.at("terrain"),data);}
 catch(const std::runtime_error& e){rejected=std::string(e.what()).find("unverified area attack scoring")!=std::string::npos;}
 if(!rejected)throw std::runtime_error("unverified area scoring did not stop");
 std::cout<<"Unverified area-attack scoring stops explicitly\n";
 auto decisions=read_json(root.parent_path().parent_path()/"tests/fixtures/original_ai_decision.json");index=0;
 for(const auto& c:decisions.at("cases")){
  auto actor=BattleUnit::from_json(c.at("battle"));auto record=UnitRecord::from_json(c.at("unit"));
  std::vector<BattleUnit> board;std::vector<UnitRecord> units;
  for(const auto& r:c.at("roster"))board.push_back(BattleUnit::from_json(r));
  for(const auto& r:c.at("records"))units.push_back(UnitRecord::from_json(r));
  auto d=decide_battle_ai(actor,record,c.at("priority_id"),board,units,c.at("width"),c.at("height"),c.at("terrain"),data,decisions.at("tactics"));
  auto check=[&](const std::string& key,const Json& actual){if(actual!=c.at(key))throw std::runtime_error("AI decision "+std::to_string(index)+" "+key+" mismatch: "+actual.dump()+" != "+c.at(key).dump());};
  check("reached_movement",d.reached_movement);check("after",d.actor.json());check("trace",d.trace);
  if(d.reached_movement){
   check("best_score",d.grids.best_score);
   if(d.grids.best_tile[0]!=255)check("best_tile",d.grids.best_tile);
   else if(c.at("best_tile").at(0)!=255)throw std::runtime_error("AI invalid destination mismatch");
   // Original target scratch is ignored for movement-only actors.
   if(!(actor.flags&128)&&actor.ai!=4){check("best_target",d.grids.best_target);check("best_tactic",d.grids.best_tactic);}
   check("seek_flag",d.seek_flag);check("distance",d.grids.distance);check("threat",d.grids.threat);check("scores",d.grids.scores);
  }
  ++index;
 }
 std::cout<<index<<" original combined AI decisions and candidate orders passed\n";
 auto tactic_fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_tactics.json");
 Json tactics=Json::array();for(const auto& r:tactic_fixture.at("records"))tactics.push_back(r.at("fields"));
 if(tactics!=decisions.at("tactics"))throw std::runtime_error("tactic oracle tables disagree");
 for(const auto& c:tactic_fixture.at("cases")){
  BattleUnit actor;actor.mp=c.at("mp");UnitRecord unit;unit.job=c.at("job");unit.level=c.at("level");
  if(Json(available_ai_tactics(actor,unit,tactics))!=c.at("available"))throw std::runtime_error("original tactic availability mismatch");
 }
 auto magic=read_json(root.parent_path().parent_path()/"tests/fixtures/original_tactic_scoring.json");index=0;
 if(magic.at("tactics")!=tactics)throw std::runtime_error("tactic scoring table differs");
 for(const auto& c:magic.at("pairs")){
  auto a=BattleUnit::from_json(c.at("actor")),b=BattleUnit::from_json(c.at("target"));
  auto au=UnitRecord::from_json(c.at("actor_unit")),bu=UnitRecord::from_json(c.at("target_unit"));int id=c.at("tactic");
  auto check=[&](const std::string& k,const Json& value){if(value!=c.at(k))throw std::runtime_error("tactic pair "+std::to_string(index)+" "+k+": "+value.dump()+" != "+c.at(k).dump());};
  check("eligible",tactic_eligible(a,b,id,tactics));check("chance",tactic_hit_chance(a,au,b,bu,id,data));
  OriginalRng rng{c.at("seed")};check("damage",tactic_damage(a,au,b,bu,id,true,data,rng));check("damage_rng",rng.state);
  rng.state=c.at("seed");check("estimate",tactic_damage(a,au,b,bu,id,false,data,rng));check("seed",rng.state);
  check("score",score_ai_tactic_target(a,au,b,bu,id,data,tactics,rng));check("score_rng",rng.state);++index;
 }
 std::vector<BattleUnit> magic_board;std::vector<UnitRecord> magic_units;
 for(const auto& b:magic.at("roster"))magic_board.push_back(BattleUnit::from_json(b));
 for(const auto& u:magic.at("records"))magic_units.push_back(UnitRecord::from_json(u));
 index=0;
 for(const auto& c:magic.at("scores")){
  auto a=BattleUnit::from_json(c.at("actor"));auto au=*std::find_if(magic_units.begin(),magic_units.end(),[&](const auto& u){return u.id==a.id;});OriginalRng rng{c.at("seed")};
  auto result=score_ai_tactics(a,au,c.at("center").get<std::array<int,2>>(),c.at("available").get<std::vector<int>>(),magic_board,magic_units,20,20,data,tactics,rng);
  Json actual={{"score",result.score},{"target",result.target},{"tactic",result.tactic},{"final_rng",rng.state}};
  for(auto it=actual.begin();it!=actual.end();++it)if(it.value()!=c.at(it.key()))throw std::runtime_error("tactic candidate "+std::to_string(index)+" "+it.key()+": "+actual.dump());
  ++index;
 }
 std::cout<<magic.at("pairs").size()<<" original tactic hit/damage/score pairs and "<<index<<" candidate scores passed, including RNG\n";
 index=0;
 for(const auto& c:magic.at("decisions")){
  auto a=BattleUnit::from_json(c.at("actor"));auto au=*std::find_if(magic_units.begin(),magic_units.end(),[&](const auto& u){return u.id==a.id;});
  auto board=magic_board;*std::find_if(board.begin(),board.end(),[&](const auto& b){return b.slot==a.slot;})=a;
  auto d=decide_battle_ai(a,au,0,board,magic_units,20,20,magic.at("terrain"),data,tactics,OriginalRng{c.at("seed")});
  Json actual={{"after",d.actor.json()},{"trace",d.trace},{"best_score",d.grids.best_score},{"best_tile",d.grids.best_tile},{"best_target",d.grids.best_target},{"best_tactic",d.grids.best_tactic},{"final_rng",d.rng.state},{"distance",d.grids.distance},{"threat",d.grids.threat},{"scores",d.grids.scores},{"seek_flag",d.seek_flag}};
  for(auto it=actual.begin();it!=actual.end();++it)if(it.value()!=c.at(it.key()))throw std::runtime_error("tactic decision "+std::to_string(index)+" "+it.key()+": "+it.value().dump()+" != "+c.at(it.key()).dump());
  ++index;
 }
 std::cout<<index<<" original enemy tactic decisions and random-number states passed\n";
 auto plans=read_json(root.parent_path().parent_path()/"tests/fixtures/original_tactic_plan.json");index=0;
 for(const auto& c:plans.at("cases")){
  const auto& rows=c.at("before").at("units");
  auto row=[&](int slot)->const Json&{return *std::find_if(rows.begin(),rows.end(),[&](const auto& r){return r.at("battle").at("slot")==slot;});};
  const auto& a=row(35);const auto& b=row(20);
  auto result=plan_enemy_poison(BattleUnit::from_json(a.at("battle")),UnitRecord::from_json(a.at("unit")),BattleUnit::from_json(b.at("battle")),UnitRecord::from_json(b.at("unit")),OriginalRng{c.at("before").at("rng")},data);
  if(result.json()!=c.at("plan")||result.rng.state!=c.at("after").at("rng"))throw std::runtime_error("original tactic plan differs "+std::to_string(index)+": "+result.json().dump());
  ++index;
 }
 std::cout<<index<<" original poison plans with separate status/damage rolls passed\n";
 // Raising the level admits further tactics that must remain explicit guards.
 const auto& guard=decisions.at("cases").back();auto actor=BattleUnit::from_json(guard.at("battle"));actor.status=0;
 auto unit=UnitRecord::from_json(guard.at("unit"));unit.level=99;actor.mp=255;std::vector<BattleUnit> board;std::vector<UnitRecord> units;
 for(const auto& r:guard.at("roster"))board.push_back(BattleUnit::from_json(r));
 for(const auto& r:guard.at("records"))units.push_back(UnitRecord::from_json(r));
 auto before=actor.json();rejected=false;
 try{decide_battle_ai(actor,unit,guard.at("priority_id"),board,units,20,20,guard.at("terrain"),data,tactics);}
 catch(const std::runtime_error& e){rejected=std::string(e.what()).find("unverified AI tactic scoring")!=std::string::npos;}
 if(!rejected||actor.json()!=before)throw std::runtime_error("unverified tactic decision did not stop unchanged");
 std::cout<<tactic_fixture.at("cases").size()<<" original tactic-availability cases and unsupported-tactic guard passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
