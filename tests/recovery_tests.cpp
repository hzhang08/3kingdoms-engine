#include "caocao/recovery.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");
 auto root=std::filesystem::path(argv[1]);auto data=read_json(root/"data.json");
 auto fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_recovery.json");
 for(const auto& c:fixture.at("cases")){
  const auto& input=c.at("input");auto u=UnitRecord::from_json(c.at("unit"));
  BattleUnit b;b.id=0;b.slot=0;b.x=10;b.y=10;b.lifecycle=input.at("lifecycle");b.status=input.at("status");b.grades=input.at("grades").get<std::array<int,6>>();
  OriginalRng rng{input.at("seed").get<uint32_t>()};RecoveryEffects effects;
  if(input.at("kind")=="grades")effects=recover_battle_grades(b,u,rng,data);
  else{
   std::vector<BattleUnit> roster{b};std::vector<UnitRecord> records{u};
   Json neighbors=input.value("neighbors",Json::array());if(input.contains("neighbor"))neighbors.push_back(input.at("neighbor"));
   for(const auto& n:neighbors){
    BattleUnit other;other.slot=n.value("slot",1);other.id=other.slot;other.x=n.at("x");other.y=n.at("y");other.side=n.at("side");other.lifecycle=n.at("lifecycle");roster.push_back(other);
    UnitRecord record;record.id=other.id;record.job=n.at("job");records.push_back(record);
   }
   effects=recover_battle_status(b,u,roster,records,rng,data);
  }
  std::array<int,6> expected;for(int n=0;n<6;++n)expected[n]=c.at("after").at(24+n);
  if(b.grades!=expected||b.status!=c.at("after").at(30)||rng.state!=c.at("rng")||Json(effects.changes)!=c.at("changes")||Json(effects.aura_slots)!=c.at("auras"))throw std::runtime_error("original turn-start recovery mismatch: "+input.dump()+"\n"+Json(effects.changes).dump()+" vs "+c.at("changes").dump());
 }
 auto maintenance=read_json(root.parent_path().parent_path()/"tests/fixtures/original_maintenance.json");
 for(const auto& c:maintenance.at("cases")){
  const auto& input=c.at("input");auto u=UnitRecord::from_json(c.at("unit"));BattleUnit b;
  b.id=0;b.slot=input.at("slot");b.side=input.at("side");b.lifecycle=input.at("lifecycle");b.hp=input.at("hp");b.mp=input.at("mp");
  Json numbers=Json::array();
  if(input.at("kind")=="poison"){
   b.status=16;int damage=battle_poison_loss(b,u,data);numbers.push_back(Json::array({b.slot,255,damage,0,0,0,0,1}));b.hp-=damage;
  }else{
   auto amount=battle_regeneration(b,u,input.at("terrain"),data);b.hp+=amount.hp;b.mp+=amount.mp;
   if(amount.any())numbers.push_back(Json::array({b.slot,255,-amount.hp,-amount.mp,amount.xp,amount.weapon_xp,amount.armor_xp,1}));
  }
  if(b.hp!=c.at("hp")||b.mp!=c.at("mp")||numbers!=c.at("numbers"))throw std::runtime_error("original maintenance mismatch "+input.dump()+"\n"+numbers.dump()+" vs "+c.at("numbers").dump());
 }
 std::cout<<maintenance.at("cases").size()<<" original poison/regeneration numeric requests and HP/MP results passed\n";
 std::cout<<fixture.at("cases").size()<<" original recovery cases, effects and RNG states passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
