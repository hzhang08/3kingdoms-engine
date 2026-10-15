#include "caocao/prototype.hpp"
#include "caocao/experience.hpp"
#include "prototype_terrain_fixture.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void restore(Prototype& g,Json saved){saved["sha256"]=sha256(saved.at("payload").dump());g.restore(saved.dump());}
void drain(Prototype& g){int budget=0;while(g.has_dialogue()||!g.story_choices().empty()){
 check(++budget<1000,"campaign dialogue stalled");if(!g.story_choices().empty())g.choose_story(0);else g.command("continue");
}}
// Synthetic casualties are isolated fixtures: this verifies transition state,
// not an end-to-end victory claim or the player's combat strength.
void victory(Prototype& g,bool opening=false){
 drain(g);auto saved=Json::parse(g.serialize());auto& units=saved["payload"]["state"]["units"];
 for(auto& u:units){auto& b=u["battle"];int id=b.at("id");if(b.at("side").get<int>()>=2&&(!opening||id==146||id==147)){b["hp"]=0;b["lifecycle"]=3;}}
 units["0"]["battle"]["flags"]=0;restore(g,saved);
 auto b=g.state().at("units").at("0").at("battle");g.command("tile",b.at("x"),b.at("y"));g.command("wait");drain(g);
 check(g.state().at("phase")=="victory"&&g.progress().at("active").is_null(),"fixture did not finish scenario rewards");
}
}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");Prototype game(argv[1]);game.enable_revised_rules();drain(game);
 auto initial=game.serialize();bool rejected=false;try{game.advance_campaign();}catch(...){rejected=true;}
 check(rejected&&game.serialize()==initial,"premature continuation changed game");
 auto sites=game.locations();check(!sites.empty(),"opening pickup fixture missing");auto site=sites.at(0);
 terrain_fixture_place(game,0,site.at("x"),site.at("y"));game.command("tile",site.at("x"),site.at("y"));game.command("wait");drain(game);
 check(game.inventory().at(std::to_string(site.at("item").get<int>())).get<int>()>0,"pickup not available for carryover");
 auto growth=Json::parse(game.serialize());auto& row=growth["payload"]["state"]["units"]["0"];
 auto r=UnitRecord::from_json(row.at("record"));grow_unit(r,8,game.data().at("classes"));r.xp=37;
 add_equipment_experience(r,0,115,game.data());row["record"]=r.json();row["max_hp"]=std::min(500,r.hp+equipment_bonus(r,5,game.data()));restore(game,growth);
 victory(game,true);auto record=game.state().at("units").at("0").at("record"),inventory=game.inventory(),treasures=game.treasures(),rng=game.state().at("rng");
 check(treasures.at("76")==1,"opening treasure missing before carryover");
 game.advance_campaign();check(game.scenario()==1,"continuation did not enter Sishui");
 check(game.state().at("units").at("0").at("record")==record,"party XP/equipment changed at next battle");
 check(game.inventory()==inventory&&game.treasures()==treasures,"inventory or treasure ledger lost");
 check(game.state().at("rng")==rng,"campaign continuation consumed random draws");
 for(const auto& u:game.state().at("units"))if(u.at("battle").at("side")==0){
  check(u.at("record").at("level")==record.at("level"),"new member was not initialized at carried party level");
  check(u.at("battle").at("hp")==u.at("max_hp")&&u.at("battle").at("mp")==u.at("record").at("mp"),"next battle did not reset live resources");
 }
 Prototype standalone(argv[1]);standalone.enable_revised_rules();standalone.start_scenario(1);
 check(game.state().at("units").at("134").at("record").at("level")!=standalone.state().at("units").at("134").at("record").at("level"),"NPC level still fixed to standalone preset");
 std::string trace=game.hash()+"\n";auto saved=game.serialize();Prototype mirror(argv[1]);mirror.restore(saved);check(mirror.serialize()==saved,"campaign snapshot failed exact restore");
 int corruption_cases=0;
 for(int kind=0;kind<6;++kind){auto invalid=Json::parse(saved);auto& c=invalid["payload"]["campaign"];auto& history=c["battles"][0];
  if(kind==0)history["party"]["0"]["stats"][0]=250;
  if(kind==1)history["party"]["0"]["equipment"][0][0]=27;
  if(kind==2)history["benefits"]["inventory"]["87"]=99;
  if(kind==3)history["progress"]["treasures"]["76"]=99;
  if(kind==4)c["origin"]=1;
  if(kind==5)history["party"].erase("0");
  rejected=false;try{restore(game,invalid);}catch(...){rejected=true;}check(rejected&&game.serialize()==saved,"forged campaign baseline accepted");++corruption_cases;
 }
 drain(game);drain(mirror);check(game.hash()==mirror.hash(),"campaign intro changed after reload");
 victory(game);auto second_record=game.state().at("units").at("0").at("record");auto second_treasures=game.treasures();game.advance_campaign();
 check(game.scenario()==2&&game.state().at("units").at("0").at("record")==second_record&&game.treasures()==second_treasures,"second continuation lost persistent values");
 game.restore(game.serialize());trace+=game.hash()+"\n";
 victory(game);auto third_treasures=game.treasures();game.advance_campaign();check(game.scenario()==3&&game.treasures()==third_treasures,"third continuation lost treasure ledger");
 game.restore(game.serialize());trace+=game.hash()+"\n";
 check(Json::parse(game.serialize()).at("payload").at("campaign").at("battles").size()==3,"campaign ledger length differs");
 game.start_scenario(1);check(Json::parse(game.serialize()).at("payload").at("campaign").empty(),"standalone scenario inherited campaign");
 check(game.state().at("units").at("0").at("record")==standalone.state().at("units").at("0").at("record"),"standalone scenario did not reset preset");
 // Maximum character/gear levels must remain legal initialization baselines.
 // This also exercises new members and script-scaled NPCs reaching the cap.
 Prototype capped(argv[1]);capped.enable_revised_rules();drain(capped);auto cap_input=Json::parse(capped.serialize());
 auto& cap_row=cap_input["payload"]["state"]["units"]["0"];auto cap_record=UnitRecord::from_json(cap_row.at("record"));
 grow_unit(cap_record,50-cap_record.level,capped.data().at("classes"));add_equipment_experience(cap_record,0,1000,capped.data());add_equipment_experience(cap_record,1,1000,capped.data());
 cap_row["record"]=cap_record.json();cap_row["max_hp"]=std::min(500,cap_record.hp+equipment_bonus(cap_record,5,capped.data()));restore(capped,cap_input);
 victory(capped,true);capped.advance_campaign();
 check(capped.state().at("units").at("0").at("record")==cap_record.json(),"maximum-level party/gear changed on continuation");
 for(const auto& u:capped.state().at("units"))if(u.at("record").at("level")==50)check(u.at("record").at("xp")==255,"maximum-level initialization omitted XP sentinel");
 auto cap_snapshot=capped.serialize();capped.restore(cap_snapshot);check(capped.serialize()==cap_snapshot,"maximum-level campaign snapshot changed");trace+=capped.hash()+"\n";
 Json report={{"transitions",3},{"corruption_cases",corruption_cases},{"party_xp_equipment_inventory_treasures",true},{"npc_scaled_from_party",true},{"save_resume",true},{"maximum_level_gear_continuation",true},{"standalone_reset",true},{"trace_sha256",sha256(trace)}};
 if(argc>2)std::ofstream(argv[2])<<report.dump(2);std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
