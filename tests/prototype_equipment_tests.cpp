#include "caocao/prototype.hpp"
#include "caocao/experience.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void load(Prototype& g,Json envelope){envelope["sha256"]=sha256(envelope.at("payload").dump());g.restore(envelope.dump());}
void drain(Prototype& g){for(int n=0;n<1000;++n){if(g.has_dialogue())g.command("continue");else if(!g.story_choices().empty())g.choose_story(0);else return;}throw std::runtime_error("dialogue stuck");}
void win(Prototype& g){drain(g);auto saved=Json::parse(g.serialize());for(auto& u:saved["payload"]["state"]["units"])if(u.at("battle").at("side").get<int>()>=2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}
 saved["payload"]["state"]["units"]["0"]["battle"]["flags"]=0;load(g,saved);auto b=g.state().at("units").at("0").at("battle");g.command("tile",b.at("x"),b.at("y"));g.command("wait");drain(g);check(g.state().at("phase")=="victory","fixture victory missing");}
int bag(const Prototype& g,int id){for(size_t n=0;n<g.equipment_bag().size();++n)if(g.equipment_bag()[n][0]==id)return int(n);throw std::runtime_error("bag item missing");}
void rejects(Prototype& g,const std::function<void()>& f){auto before=g.serialize();bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"rejected operation mutated state");}
}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");Prototype g(argv[1]);g.enable_revised_rules();drain(g);auto legacy=g.serialize();auto state=g.state();g.enable_equipment();check(g.state()==state&&g.equipment_bag().empty(),"migration changed battle or spawned equipment");
 check(Json::parse(g.serialize()).at("payload").at("schema")==10,"wrong equipment schema");rejects(g,[&]{g.change_equipment(0,0,-1);});
 Prototype old(argv[1]);old.restore(legacy);win(old);auto old_won=old.serialize();g.restore(old_won);check(g.serialize()==old_won&&!g.equipment_enabled(),"schema9 compatibility changed");g.enable_equipment();check(g.equipment_bag().size()==1&&g.equipment_bag()[0][0]==76,"earned old treasure not imported once");auto migrated=g.serialize();g.enable_equipment();check(g.serialize()==migrated,"migration duplicated reward");
 auto rng=g.state().at("rng"),turn=g.state().at("turn"),flags=g.state().at("units").at("0").at("battle").at("flags");
 auto weapon=g.state().at("units").at("0").at("record").at("equipment")[0];g.change_equipment(0,0,-1);check(g.equipment_bag().at(bag(g,23))==weapon,"unequip lost item level/XP");g.change_equipment(0,0,bag(g,23));check(g.state().at("units").at("0").at("record").at("equipment")[0]==weapon,"reequip reset experience");
 g.change_equipment(0,2,bag(g,76));check(g.treasures().at("76")==1&&g.equipment_bag().empty(),"equipping changed reward ledger");
 check(g.state().at("rng")==rng&&g.state().at("turn")==turn&&g.state().at("units").at("0").at("battle").at("flags")==flags,"equip spent RNG or action");
 rejects(g,[&]{g.change_equipment(146,2,-1);});rejects(g,[&]{g.change_equipment(0,3,-1);});rejects(g,[&]{g.change_equipment(0,1,100);});
 auto saved=g.serialize();Prototype mirror(argv[1]);mirror.restore(saved);check(mirror.serialize()==saved,"equipment snapshot not identical");
 int invalid=0;for(int kind=0;kind<7;++kind){auto bad=Json::parse(saved);auto& p=bad["payload"];auto& eq=p["state"]["units"]["0"]["record"]["equipment"];
  if(kind==0)p["equipment_bag"].push_back({76,255,255});if(kind==1)eq[2]={255,255,255};if(kind==2)eq[0]={76,255,255};if(kind==3)eq[0][1]=10;if(kind==4)eq[2][2]=0;if(kind==5)p["equipment_bag"]=Json::object();if(kind==6)p["equipment_bag"].push_back({87,255,255});
  rejects(g,[&]{load(g,bad);});++invalid;
 }
 // Weapon/armor XP survives transfers across officers and campaign boundaries.
 auto raised=Json::parse(saved);auto& row=raised["payload"]["state"]["units"]["0"];auto r=UnitRecord::from_json(row.at("record"));add_equipment_experience(r,0,135,g.data());row["record"]=r.json();load(g,raised);weapon=r.json().at("equipment")[0];
 g.change_equipment(0,0,-1);auto carrybag=g.equipment_bag();g.advance_campaign();check(g.equipment_bag()==carrybag,"campaign lost bag");check(g.state().at("units").at("0").at("record").at("equipment")[2][0]==76,"campaign lost equipped treasure");
 rejects(g,[&]{g.change_equipment(0,0,bag(g,23));});drain(g);win(g);
 int recipient=-1;for(const auto& u:g.state().at("units"))if(u.at("battle").at("side")==0&&u.at("battle").at("id")!=0&&g.equipment_compatible(u.at("battle").at("id"),0,23)){recipient=u.at("battle").at("id");break;}
 check(recipient>=0,"transfer fixture missing");auto oldweapon=g.state().at("units").at(std::to_string(recipient)).at("record").at("equipment")[0];g.change_equipment(recipient,0,bag(g,23));check(g.state().at("units").at(std::to_string(recipient)).at("record").at("equipment")[0]==weapon,"transfer lost earned XP");check(g.equipment_bag().at(bag(g,oldweapon[0]))==oldweapon,"replacement discarded previous weapon");
 g.restore(g.serialize());g.advance_campaign();drain(g);saved=g.serialize();
 if(argc>3){std::filesystem::path fixture=argv[3];if(std::filesystem::exists(fixture)){std::ifstream f(fixture);std::string imported((std::istreambuf_iterator<char>(f)),{});check(imported==saved,"cross-platform fixture differs");mirror.restore(imported);}else{std::ofstream(fixture)<<saved;mirror.restore(saved);}}else mirror.restore(saved);
 check(mirror.serialize()==saved,"multi-battle gear history did not restore");
 // Both instances take the same actual NPC commands after resume.
 g.command("end_turn");mirror.command("end_turn");for(int n=0;n<60&&(g.state().at("phase")=="enemy"||g.state().at("phase")=="ally");++n){drain(g);drain(mirror);g.enemy_step();mirror.enemy_step();check(g.hash()==mirror.hash(),"next action/RNG changed after restore");}
 int compatibility_cases=0;Prototype catalog(argv[1]);catalog.enable_revised_rules();
 for(int scenario=0;scenario<catalog.scenario_count();++scenario){catalog.start_scenario(scenario);for(const auto& u:catalog.state().at("units")){int actor=u.at("battle").at("id");auto original=UnitRecord::from_json(u.at("record"));for(int slot=0;slot<3;++slot)for(int item=0;item<int(catalog.data().at("items").size());++item){auto probe=original;probe.equipment[slot]={255,255,255};equip_unit(probe,slot,item+(slot==0?2:slot==1?-36:-51),1,catalog.data());check(catalog.equipment_compatible(actor,slot,item)==(probe.equipment[slot][0]==item),"slot/class compatibility differs from recovered equip routine");++compatibility_cases;}}}
 auto trace=g.hash();g.start();check(g.equipment_bag().empty()&&g.treasures().empty(),"independent restart retained campaign rewards");
 Json report={{"old_save_migration",true},{"between_battles_only",true},{"no_npc_gear_theft",true},{"item_level_xp_preserved",true},{"reward_conservation",true},{"campaign_transitions",2},{"compatibility_cases",compatibility_cases},{"invalid_snapshots_rejected",invalid},{"identical_resumed_actions",true},{"trace_sha256",trace}};
 if(argc>2)std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
