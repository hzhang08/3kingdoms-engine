// Read-only event export for a character already present in the special seed.
#include "caocao/attack_animation.hpp"
#include "caocao/movement.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("usage: export_character_animation_events ASSETS CHARACTER_ID OUTPUT.json");
 auto assets=std::filesystem::path(argv[1]);int id=std::stoi(argv[2]);
 auto data=read_json(assets/"data.json");
 auto seed=read_json(assets/"special-hulao/seed.json").at("payload").at("state");
 auto unit=seed.at("units").at(std::to_string(id));
 auto r=UnitRecord::from_json(unit.at("record"));
 auto b=BattleUnit::from_json(unit.at("battle"));b.x=b.y=4;b.flags=0;
 auto opponent=seed.at("units").at(id==0?"210":"0");
 auto er=UnitRecord::from_json(opponent.at("record"));
 auto enemy=BattleUnit::from_json(opponent.at("battle"));
 int sprite=battle_sprite(id,r.job,b.side);Json result=Json::array();
 auto add=[&](std::string name,int direction,Json events){result.push_back({{"sprite",sprite},{"name",name},{"direction",direction},{"subject_slot",b.slot},{"idle",{{"archive","UNIT_MOV.E5"},{"sprite",sprite},{"frame",battle_frame(b,r.hp,0)},{"width",48},{"height",48},{"x",192},{"y",248},{"mirror",direction==1},{"effects",Json::array()}}},{"events",events}});};
 for(int d=0;d<4;++d){
  b.direction=d;b.flags=d==1?8:0;enemy.x=b.x+std::array<int,4>{0,1,0,-1}[d];enemy.y=b.y+std::array<int,4>{-1,0,1,0}[d];enemy.direction=(d+2)%4;
  std::vector<int> occupancy(400,255);occupancy[b.y*20+b.x]=b.slot;
  auto walk=battle_walking_step(b,r.job,d,2,{0,0},20,20,occupancy).events;
  for(auto& e:walk)if(e.at("kind")=="frame")e.update({{"archive","UNIT_MOV.E5"},{"sprite",sprite},{"effects",Json::array()}});
  add("walk",d,walk);
  for(bool critical:{false,true})add(critical?"critical-attack":"attack",d,physical_attack_animation(b,r,enemy,er,12,0,critical,0,0,0,data).events);
  add("receive-hit",d,physical_attack_animation(enemy,er,b,r,12,0,false,0,0,0,data).events);
  add("dodge",d,physical_attack_animation(enemy,er,b,r,0,0,false,0,0,0,data).events);
  for(int script=1;script<=9;++script)add("script-"+std::to_string(script),d,scripted_battle_animation(b,sprite,script,false,true,0,0));
 }
 std::ofstream output(argv[3]);if(!output)throw std::runtime_error("cannot open output file");
 output<<Json{{"character_id",id},{"character_name",unit.at("name")},{"sprite",sprite},{"class",r.job},{"side",b.side},{"tick_ms",40},{"source","Existing compiled native C++ animation helpers; isolated character frames and waits."},{"animations",result}}.dump(2);
 if(!output)throw std::runtime_error("cannot write output file");
 std::cout<<result.size()<<" native sequences; character "<<id<<", sprite "<<sprite<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
