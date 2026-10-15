#include "caocao/unit_rules.hpp"
#include "caocao/deployment.hpp"
#include "caocao/battle.hpp"
#include <iostream>
#include <stdexcept>
using namespace caocao;
void equal(const Json& actual,const Json& expected,const char* what){if(actual!=expected)throw std::runtime_error(std::string(what)+"\nactual: "+actual.dump()+"\nexpected: "+expected.dump());}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");std::filesystem::path assets=argv[1];
 auto data=read_json(assets/"data.json"),f=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_initialization.json");
 for(const auto& c:f.at("growth")){auto u=UnitRecord::from_json(c.at("before"));grow_unit(u,c.at("increment"),data.at("classes"));equal(u.json(),c.at("after"),"original growth mismatch");}
 for(const auto& c:f.at("default_equipment")){auto u=UnitRecord::from_json(c.at("before"));default_equipment(u,c.at("lower").get<int>()!=0,data.at("classes"));equal(u.json(),c.at("after"),"original default equipment mismatch");}
 for(const auto& c:f.at("party_average"))equal(original_party_average(c.at("levels").get<std::vector<int>>()),c.at("result"),"original average mismatch");
 for(const auto& c:f.at("join")){auto u=join_unit(data.at("heroes").at(c.at("id").get<int>()),1,c.at("adjustment"),data.at("classes"));equal(u.json(),c.at("after"),"original join mismatch");}
 std::cout<<"Original x86 fixtures: "<<f.at("growth").size()<<" growth, "<<f.at("default_equipment").size()<<" equipment, "<<f.at("party_average").size()<<" averages, "<<f.at("join").size()<<" joins passed.\n";
 auto d=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_deployment.json");
 for(const auto& c:d.at("equipment")){auto u=UnitRecord::from_json(c.at("before"));auto items=script_equipment(u,c.at("args").get<std::array<int,5>>(),data);equal(u.json(),c.at("after"),"script equipment mismatch");Json writes=Json::array();for(int id:items)writes.push_back({id+3,1});equal(writes,c.at("collection"),"collection side effect mismatch");}
 Json slots=Json::array();for(const auto& step:d.at("deployment")){int op=step.at("opcode");const auto a=step.at("args").get<std::vector<int>>();
  if(op==68){DeploymentSlot s;s.id=0;slots.push_back(s.json());}
  if(op==70||op==71){int width=op==70?11:12;for(size_t n=0;n<a.size()/width;++n){auto s=deployment_row((op==70?15:35)+n,op==71,std::vector<int>(a.begin()+n*width,a.begin()+(n+1)*width),1);if(s.id!=65535)slots.push_back(s.json());}}
  if(op==75)for(auto& j:slots)if(j.at("slot")==a[0]){auto s=DeploymentSlot::from_json(j);position_deployment(s,a[1],a[2],a[3],a[4]);j=s.json();}
  if(op==72)for(auto& j:slots)if(j.at("slot").get<int>()>=15&&j.at("id")==a[0]){auto s=DeploymentSlot::from_json(j);equipment_deployment(s,{a[1],a[2],a[3],a[4],a[5]});j=s.json();break;}
  if(step.contains("slots"))equal(slots,step.at("slots"),"deployment handler mismatch");
 }
 std::cout<<d.at("deployment").size()<<" original deployment handler checkpoints passed.\n";
 auto bf=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_battle_initialization.json");
 auto raw=[](const BattleUnit& b){std::vector<int> r(36);r[0]=b.id&255;r[1]=b.id>>8;r[4]=b.slot;r[5]=b.side;r[6]=b.x;r[7]=b.y;r[8]=b.target_slot;r[9]=b.target_x;r[10]=b.target_y;r[12]=b.lifecycle;r[13]=b.flags;r[14]=b.ai;r[15]=b.direction;r[16]=b.hp&255;r[17]=b.hp>>8;r[20]=b.mp;for(int i=0;i<6;++i)r[24+i]=b.grades[i];r[30]=b.status;return r;};
 for(const auto& row:bf.at("battle")){int id=row.at("unit").at("id");auto u=id==0?UnitRecord::from_json(bf.at("equipment").at(0).at("after")):load_unit_record(data.at("heroes").at(id));
  auto descriptor=std::find_if(slots.begin(),slots.end(),[&](const Json& j){return j.at("id")==id;});if(descriptor==slots.end())throw std::runtime_error("missing descriptor");
  auto b=initialize_battle_unit(u,DeploymentSlot::from_json(*descriptor),data);equal(u.json(),row.at("unit"),"live initialization persistent record mismatch");equal(raw(b),row.at("live"),"live initialization byte mismatch");equal(battle_sprite(u.id,u.job,b.side),row.at("sprite"),"battle sprite mismatch");
  equal(b.hp,row.at("max_hp"),"maximum HP mismatch");equal(b.mp,row.at("max_mp"),"maximum MP mismatch");b.validate(u,data,20,20);
  for(const auto& cmd:bf.at("hp_commands"))if(cmd.at("args").at(1)==id){equal(raw(b),cmd.at("before"),"HP command precondition");scripted_hp_loss(b,cmd.at("args").at(11));equal(raw(b),cmd.at("after"),"HP command mismatch");}
 }
 std::cout<<bf.at("battle").size()<<" original live battle units and "<<bf.at("hp_commands").size()<<" scripted HP reductions passed.\n";
 for(const auto& c:bf.at("npc_growth")){auto u=UnitRecord::from_json(c.at("before"));initialize_npc(u,c.at("level"),c.at("promotion_mode"),data);equal(u.json(),c.at("after"),"NPC growth mismatch");}
 for(const auto& c:bf.at("frames")){
  auto row=std::find_if(bf.at("battle").begin(),bf.at("battle").end(),[&](const Json& j){return j.at("slot")==c.at("slot");});auto b=c.at("live").get<std::vector<int>>();BattleUnit live;live.direction=b[15];live.flags=b[13];live.status=b[30];live.hp=b[16]+256*b[17];equal(battle_frame(live,row->at("max_hp"),c.at("tick")),c.at("frame"),"original idle-frame selection mismatch");
 }
 std::cout<<bf.at("npc_growth").size()<<" NPC growth and "<<bf.at("frames").size()<<" idle-frame fixtures passed.\n";
 for(const auto& c:bf.at("movement_costs")){auto u=UnitRecord::from_json(c.at("unit"));for(int terrain=0;terrain<30;++terrain)equal(movement_cost(u,terrain,data),c.at("costs").at(terrain),"original terrain movement cost mismatch");}
 std::cout<<bf.at("movement_costs").size()*30<<" original movement-cost lookups passed (distance-search coverage is in movement_tests).\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
