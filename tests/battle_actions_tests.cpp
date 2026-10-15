#include "caocao/battle.hpp"
#include "caocao/tactic_effect.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace caocao;
BattleUnit decode(const Json& r){
 auto word=[&](int n){return r.at(n).get<int>()+(r.at(n+1).get<int>()<<8)+(r.at(n+2).get<int>()<<16)+(r.at(n+3).get<int>()<<24);};
 BattleUnit b;b.id=word(0);b.slot=r.at(4);b.side=r.at(5);b.x=r.at(6);b.y=r.at(7);b.target_slot=r.at(8);b.target_x=r.at(9);b.target_y=r.at(10);b.lifecycle=r.at(12);b.flags=r.at(13);b.ai=r.at(14);b.direction=r.at(15);b.hp=word(16);b.mp=word(20);for(int n=0;n<6;++n)b.grades[n]=r.at(24+n);b.status=r.at(30);return b;
}
std::vector<BattleUnit> decode_all(const Json& rows){std::vector<BattleUnit> out;for(const auto& r:rows)out.push_back(decode(r));return out;}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");
 auto fixture=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_battle_actions.json");
 auto confusion=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_confusion.json");
 for(const auto& c:confusion.at("cases")){
  auto units=decode_all(c.at("before"));const auto& s=c.at("spec");
  auto selected=scripted_area_units(units,s.at("rect"),s.at("filter"));Json calls=Json::array();
  for(auto b:selected){calls.push_back({{"slot",b.slot},{"category",6},{"value",8},{"status",b.status}});for(auto& u:units)if(u.slot==b.slot)u.status|=8;}
  if(calls!=c.at("presentations"))throw std::runtime_error("area selector/order mismatch: "+s.dump());
  auto expected=decode_all(c.at("after"));for(size_t n=0;n<units.size();++n)if(units[n].json()!=expected[n].json())throw std::runtime_error("area status changed unrelated fields");
 }
 auto script_ai=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_script_ai.json");
 for(const auto& c:script_ai.at("cases")){
  Json after=c.at("before");for(auto& row:after)if(row.at("id")==c.at("id")&&row.at("lifecycle")!=0){
   auto b=BattleUnit::from_json(row);scripted_unit_ai(b,UnitRecord::from_json(c.at("record")),c.at("mode"));row=b.json();break;
  }
  if(after!=c.at("after"))throw std::runtime_error("original script AI assignment differs");
 }
 std::cout<<confusion.at("cases").size()<<" area status cases and "<<script_ai.at("cases").size()<<" script AI assignments passed\n";
 auto entry=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_battle_entry.json");
 auto reinforcement=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_reinforcements.json");
 auto data=read_json(std::filesystem::path(argv[1])/"data.json"),terrain=read_json(std::filesystem::path(argv[1])/"terrain.json");
 auto banners=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_turn_banner.json");
 auto labels=read_json(std::filesystem::path(argv[1])/"ui.json").at("turn_labels");
 for(const auto& c:banners.at("cases")){
  const auto& i=c.at("input");auto steps=battle_turn_banner(i.at("turn"),i.at("limit"),i.at("enemy"),i.at("preserve"),labels,i.at("viewport"),i.at("client"));
  Json events=Json::array();auto flags=c.at("before");bool reset=false;
  for(const auto& step:steps){if(step.at("kind")=="reset_action_flags"){
    if(reset||events.empty()||events.back()!=Json({{"kind","wait"},{"ticks",20}}))throw std::runtime_error("banner reset order");
    reset=true;for(auto& f:flags)f=f.get<int>()&~6;
   }else events.push_back(step);}
  if(events!=c.at("events")||flags!=c.at("after")||reset==i.at("preserve").get<bool>())throw std::runtime_error("original turn banner mismatch: "+i.dump());
  std::string checker(48*48,0);for(int y=0;y<48;++y)for(int x=0;x<48;++x)if((x+y)%2==0)checker[y*48+x]=i.at("enemy").get<bool>()?63:45;
  if(sha256(checker)!=c.at("checker_sha256").get<std::string>()||c.at("final_color")!=255)throw std::runtime_error("original banner checker pixels mismatch");
 }
 std::cout<<banners.at("cases").size()<<" original turn banner sequences passed\n";
 auto defeats=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_defeated_unit.json");
 auto poison_post=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_poison_post.json").at("stages").at(1);
 Json actual_death={{"spec",{{"slot",20}}},{"before",Json::array()},{"after",Json::array()},
  {"events",poison_post.at("events")},{"camera",poison_post.at("after").at("camera")},
  {"initial_camera",poison_post.at("before").at("camera")}};
 for(auto phase:{"before","after"})for(const auto& row:poison_post.at(phase).at("units"))actual_death[phase].push_back(row.at("battle"));
 // The sequential oracle also records the external character-window
 // notification, which the isolated retirement oracle treats as a host no-op.
 actual_death["events"].erase(std::remove_if(actual_death["events"].begin(),actual_death["events"].end(),[](const Json& e){return e==Json({{"kind","refresh"},{"unit",234}});}),actual_death["events"].end());
 defeats["cases"].push_back(actual_death);
 for(const auto& c:defeats.at("cases")){
  std::vector<BattleUnit> units;for(const auto& row:c.at("before"))units.push_back(BattleUnit::from_json(row));
  int slot=c.at("spec").at("slot");auto b=*std::find_if(units.begin(),units.end(),[&](const auto& b){return b.slot==slot;});
  UnitRecord record;record.id=b.id;record.owner=255;
  auto steps=defeated_battle_unit(units,record);Json visible=Json::array();int cx=96,cy=576;
  if(c.contains("initial_camera")){cx=c.at("initial_camera").at(0);cy=c.at("initial_camera").at(1);}
  for(const auto& step:steps){auto kind=step.at("kind");
   if(kind=="unit_state"||kind=="tile"){
    auto next=BattleUnit::from_json(step.at("battle"));for(auto& u:units)if(u.slot==next.slot)u=next;
    if(kind=="tile")visible.push_back(step);
   }else if(kind=="camera_visible"){
    int x=step.at("x"),y=step.at("y");int tx=std::clamp(cx/48,x-12,x),ty=std::clamp(cy/48,y-7,y);
    if(cx!=tx*48||cy!=ty*48){visible.push_back({{"kind","camera"},{"tile",{tx,ty}}});cx=tx*48;cy=ty*48;}
   }else visible.push_back(step);
  }
  Json after=Json::array();for(const auto& u:units)after.push_back(u.json());
  if(visible!=c.at("events")||after!=c.at("after")||Json::array({cx,cy})!=c.at("camera"))throw std::runtime_error("original defeated-unit sequence mismatch: "+c.at("spec").dump()+"\n"+visible.dump()+"\n"+c.at("events").dump());
 }
 std::cout<<defeats.at("cases").size()<<" original non-party defeat sequences passed\n";
 for(const auto& c:reinforcement.at("cases")){
  auto units=decode_all(c.at("before"));int mode=c.at("args").at(0),id=c.at("args").at(1),slot=c.at("args").at(2);BattleUnit* b=nullptr;
  for(auto& u:units)if(mode==1?u.slot==slot:u.id==id)b=&u;
  Json actual=Json::array();if(b){DeploymentSlot d;for(const auto& row:reinforcement.at("descriptors"))if(row.at("slot")==b->slot)d=DeploymentSlot::from_json(row);
   int job=data.at("heroes").at(b->id).at("class");actual=scripted_battle_appearance(*b,d,job,mode==1,terrain.at("cells"),c.at("occupancy").get<std::vector<int>>(),data);
  }
  Json expected=Json::array();for(const auto& event:c.at("events")){
   if(event.at("kind")=="tile")expected.push_back({{"kind","tile"},{"battle",decode(event.at("live")).json()},{"visible",event.at("visible")}});else expected.push_back(event);
  }
  if(actual!=expected)throw std::runtime_error("reinforcement sequence mismatch "+c.at("args").dump()+"\n"+actual.dump()+"\n"+expected.dump());
  for(const auto& step:actual)if(step.at("kind")=="tile")*b=BattleUnit::from_json(step.at("battle"));
  auto after=decode_all(c.at("after"));for(size_t n=0;n<units.size();++n)if(units[n].json()!=after[n].json())throw std::runtime_error("reinforcement final state mismatch");
 }
 std::cout<<reinforcement.at("cases").size()<<" original reinforcement sequences passed\n";
 for(const auto& c:entry.at("cases")){auto b=decode(c.at("before"));prepare_new_battle_unit(b);if(b.json()!=decode(c.at("after")).json())throw std::runtime_error("new-battle cleanup mismatch");}
 auto units=decode_all(entry.at("before"));for(auto& b:units)prepare_new_battle_unit(b);
 if(Json(new_battle_focus(units))!=entry.at("focus").at(0).at("tile"))throw std::runtime_error("new-battle focus mismatch");
 for(const auto& c:fixture.at("directions")){
  BattleUnit a,b;a.slot=0;b.slot=1;a.x=c.at("from_xy").at(0);a.y=c.at("from_xy").at(1);b.x=c.at("to_xy").at(0);b.y=c.at("to_xy").at(1);a.direction=c.at("old");
  if(battle_direction(a,b)!=c.at("direction"))throw std::runtime_error("original direction selection mismatch: "+c.dump());
 }
 for(auto category:{"script","chain","facing","removal"})for(const auto& c:fixture.at(category)){
  auto units=decode_all(c.at("before"));auto find=[&](int id)->BattleUnit*{for(auto& b:units)if(b.id==id)return &b;return nullptr;};
  auto args=c.at("args");bool removal=std::string(category)=="removal"||((std::string(category)=="script"||std::string(category)=="chain")&&c.at("opcode")==83);
  Json steps;
  if(removal)steps=scripted_battle_removal(units,args.at((std::string(category)=="script"||std::string(category)=="chain")?1:0));
  else{bool script=(std::string(category)=="script"||std::string(category)=="chain");int target=args.at(1);auto b=find(args.at(0));auto towards=target==-1||target==65535?nullptr:find(target);
   if(!b||(target>=0&&target!=65535&&!towards))steps=Json::array();
   else steps=scripted_battle_facing(*b,towards,args.at(2),args.at(3)==1,args.at(4)==1||(script&&args.at(4)==-1),args.at(5)==1||(script&&args.at(5)==-1));
  }
  Json actual=Json::array(),expected=Json::array();
  for(const auto& step:steps){auto kind=step.at("kind");
   if(kind=="tile"||kind=="unit_state"){
    auto b=BattleUnit::from_json(step.at("battle"));*find(b.id)=b;
    if(kind=="tile")actual.push_back(step);
   }else if(kind=="camera_visible"){
    // These fixtures begin with an original viewport that already contains the removed unit.
    int x=step.at("x"),y=step.at("y");if(x<1||x>=14||y<12||y>=20)throw std::runtime_error("unverified offscreen removal fixture");
   }else actual.push_back(step);
  }
  for(const auto& e:c.at("events")){
   if(e.at("kind")=="tile")expected.push_back({{"kind","tile"},{"battle",decode(e.at("live")).json()},{"visible",e.at("occupied")!=255}});
   else expected.push_back(e);
  }
  if(actual!=expected)throw std::runtime_error(std::string(category)+" sequence mismatch: "+args.dump()+"\n"+actual.dump()+"\n"+expected.dump());
  auto after=decode_all(c.at("after"));for(size_t n=0;n<units.size();++n)if(units[n].json()!=after[n].json())throw std::runtime_error(std::string(category)+" final state mismatch: "+args.dump()+"\n"+units[n].json().dump()+" != "+after[n].json().dump());
 }
 std::cout<<fixture.at("script").size()<<" original script commands, "<<fixture.at("directions").size()<<" direction cases, "<<fixture.at("facing").size()<<" facing sequences and "<<fixture.at("removal").size()<<" removal sequences passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
