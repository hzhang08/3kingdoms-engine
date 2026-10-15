#include "caocao/tactic_application.hpp"
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_tactic_application.json");
 Json tactics=Json::array();for(const auto& t:read_json(root/"tactics.json"))tactics.push_back(t.at("fields"));
 auto record=[](const Json& j){return StrikeTarget{BattleUnit::from_json(j.at("battle")),UnitRecord::from_json(j.at("unit")),0};};
 auto encode=[](const StrikeTarget& t){return Json{{"battle",t.battle.json()},{"unit",t.unit.json()}};};
 size_t index=0;
 for(const auto& c:fixture.at("cases")){
  auto a=record(c.at("actor")),b=record(c.at("target"));const auto& v=c.at("values");
  Json ev;std::array<int,2> camera=c.at("before_camera");int tick=c.at("before_tick");bool pending=false;
  if(c.at("finalize")){
   auto r=finalize_enemy_poison(a,b.battle,v.at("cast"),v.at("experience"),v.at("weapon_xp"),camera,20,20,data,tactics);
   a=r.actor;ev=r.requests;camera=r.camera;
  }else{
   auto r=apply_poison_target(a.battle,b,v.at("damage"),v.at("effect"),v.at("armor_xp"),v.at("area"),v.at("beans"),camera,tick,20,20,data);
   b=r.target;ev=r.requests;camera=r.camera;tick=r.tick;pending=r.pending_item;
  }
  if(encode(a)!=c.at("after_actor")||encode(b)!=c.at("after_target")||ev!=c.at("events")||Json(camera)!=c.at("after_camera")||tick!=c.at("after_tick")||pending!=(c.at("pending")=="automatic_item"))
   throw std::runtime_error("poison application mismatch "+std::to_string(index)+" events "+ev.dump()+" expected "+c.at("events").dump()+" actor "+encode(a).dump()+" target "+encode(b).dump());
  ++index;
 }
 auto c=fixture.at("cases").front();auto a=record(c.at("actor")),b=record(c.at("target"));bool rejected=false;
 try{apply_poison_target(a.battle,b,10,113,0,0,0,{144,576},6,20,20,data);}catch(const std::invalid_argument&){rejected=true;}
 if(!rejected)throw std::runtime_error("unsupported status effect accepted");
 std::cout<<index<<" original poison applications/finalizations passed, including automatic-item suspension and unsupported-effect rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
