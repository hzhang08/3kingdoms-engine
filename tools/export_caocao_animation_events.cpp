// Export the existing native helpers; this does not change battle state/assets.
#include "caocao/attack_animation.hpp"
#include "caocao/movement.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("usage: export_caocao_animation_events ASSETS OUTPUT.json");
 auto assets=std::filesystem::path(argv[1]);auto data=read_json(assets/"data.json");
 auto seed=read_json(assets/"special-hulao/seed.json").at("payload").at("state");Json result=Json::array();
 for(int tier=0;tier<3;++tier){
  auto r=UnitRecord::from_json(seed.at("units").at("0").at("record"));r.job=tier;
  auto b=BattleUnit::from_json(seed.at("units").at("0").at("battle"));b.x=b.y=4;b.flags=0;
  auto er=UnitRecord::from_json(seed.at("units").at("210").at("record"));
  auto enemy=BattleUnit::from_json(seed.at("units").at("210").at("battle"));
  int sprite=battle_sprite(0,tier,0);
  auto add=[&](std::string name,int direction,Json events){result.push_back({{"sprite",sprite},{"name",name},{"direction",direction},{"subject_slot",b.slot},{"idle",{{"archive","UNIT_MOV.E5"},{"sprite",sprite},{"frame",battle_frame(b,r.hp,0)},{"width",48},{"height",48},{"x",192},{"y",248},{"mirror",direction==1},{"effects",Json::array()}}},{"events",events}});};
  for(int d=0;d<4;++d){
   b.direction=d;b.flags=d==1?8:0;enemy.x=b.x+std::array<int,4>{0,1,0,-1}[d];enemy.y=b.y+std::array<int,4>{-1,0,1,0}[d];enemy.direction=(d+2)%4;
   std::vector<int> occupancy(400,255);occupancy[b.y*20+b.x]=b.slot;
   auto walk=battle_walking_step(b,tier,d,2,{0,0},20,20,occupancy).events;
   for(auto& e:walk)if(e.at("kind")=="frame")e.update({{"archive","UNIT_MOV.E5"},{"sprite",sprite},{"effects",Json::array()}});
   add("walk",d,walk);
   for(bool critical:{false,true})add(critical?"critical-attack":"attack",d,physical_attack_animation(b,r,enemy,er,12,0,critical,0,0,0,data).events);
   add("receive-hit",d,physical_attack_animation(enemy,er,b,r,12,0,false,0,0,0,data).events);
   add("dodge",d,physical_attack_animation(enemy,er,b,r,0,0,false,0,0,0,data).events);
   for(int id=1;id<=9;++id)add("script-"+std::to_string(id),d,scripted_battle_animation(b,sprite,id,false,true,0,0));
  }
 }
 std::ofstream(argv[2])<<Json{{"tick_ms",40},{"source","Existing C++ native animation helpers; isolated character previews, no shared spell particles or audio."},{"animations",result}}.dump(2);std::cout<<result.size()<<" native helper sequences exported\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
