#include "caocao/unit_details.hpp"
#include "caocao/combat_rules.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 std::filesystem::path root=argc>1?argv[1]:"assets/prototype";Prototype g(root);g.enable_tactics();g.enable_terrain_benefits();g.enable_items();
 auto ui=read_json(root/"ui.json"),tactics=read_json(root/"unit-details/tactics.json");
 auto original=read_json(root.parent_path().parent_path()/"tests/fixtures/original_tactics.json");check(tactics==original.at("records"),"tactic catalog differs from original executable loader fixture");
 int units=0,mounts=0,spells=0;Json reports=Json::array();
 for(int scenario=0;scenario<4;++scenario){g.start_scenario(scenario);while(g.has_dialogue())g.command("continue");auto before=g.serialize();
  for(const auto& v:g.state().at("units")){int id=v.at("battle").at("id");auto d=unit_details(g,id,ui,tactics);auto r=UnitRecord::from_json(v.at("record"));auto b=BattleUnit::from_json(v.at("battle"));
   for(int n=0;n<5;++n)check(d.at("stats").at(n).at("value")==battle_stat(b,r,n,g.data()),"combat stats mismatch");
   for(int n=0;n<3;++n){const auto& e=d.at("equipment").at(n);check(e.at("id")==r.equipment[n][0],"equipment slot mismatch");
    if(e.at("id")==255)check(e.at("name")=="无"&&e.at("level").is_null()&&e.at("xp").is_null(),"empty equipment leaked sentinel");
    else{check(e.at("name")==g.data().at("items").at(r.equipment[n][0]).at("name"),"equipment name mismatch");check(std::filesystem::exists(root/"unit-details/images/Item"/(std::string(r.equipment[n][0]<10?"00":r.equipment[n][0]<100?"0":"")+std::to_string(r.equipment[n][0])+".bmp")),"missing original item image");if(n==2)++mounts;}
   }
   for(const auto& t:d.at("tactics")){check(t.at("learn_level").get<int>()<=r.level,"unlearned spell shown");++spells;}
   if(scenario==0&&id==0){check(d.at("equipment").at(0).at("name")=="倚天剑","CaoCao original weapon missing");check(d.at("equipment").at(1).at("name")=="皮铠","CaoCao armor missing");}
   reports.push_back(d);++units;
  }
  check(before==g.serialize(),"inspection mutated battle/snapshot");
  // A zero-MP caster still knows its spells, including unimplemented ones.
  int caster=-1;for(const auto& v:g.state().at("units")){int id=v.at("battle").at("id");if(!g.spells(id).empty()){caster=id;break;}}
  if(caster>=0){auto learned=unit_details(g,caster,ui,tactics).at("tactics");auto envelope=Json::parse(before);envelope["payload"]["state"]["units"][std::to_string(caster)]["battle"]["mp"]=0;envelope["sha256"]=sha256(envelope.at("payload").dump());g.restore(envelope.dump());auto after=unit_details(g,caster,ui,tactics).at("tactics");check(after.size()==learned.size(),"low MP hid learned tactics");check(g.spells(caster).empty(),"fixture still castable");for(const auto& t:after)if(t.at("supported"))check(t.at("availability")=="策略值不足","low-MP reason incorrect");}
  g.restore(before);check(g.serialize()==before,"inspection-era save failed exact roundtrip");
 }
 check(units==130&&mounts>0&&spells>0,"incomplete roster/equipment/tactics coverage");
 Json result={{"units",units},{"accessories",mounts},{"learned_spells",spells},{"details_sha256",sha256(reports.dump())},{"original_tactics_verified",tactics.size()},{"nonmutating",true}};
 if(argc>2)std::ofstream(argv[2])<<result.dump(2)<<'\n';std::cout<<result.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
