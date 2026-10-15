#include "caocao/movement.hpp"
#include "caocao/player_movement.hpp"
#include "caocao/player_action.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto data=read_json(root/"data.json"),fixture=read_json(root.parent_path().parent_path()/"tests/fixtures/original_movement.json");
 size_t index=0;
 for(const auto& c:fixture.at("search")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));
  std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  auto actual=battle_movement_search(b,u,{b.x,b.y},c.at("budget"),c.at("target").get<std::array<int,2>>(),static_cast<MovementSearchMode>(c.at("mode").get<int>()),c.at("respect_zoc"),c.at("width"),c.at("height"),c.at("terrain"),roster,data);
  if(actual.result!=c.at("result"))throw std::runtime_error("movement return mismatch "+std::to_string(index));
  if(actual.costs.empty())actual.costs.assign(c.at("width").get<int>()*c.at("height").get<int>(),165);
  if(Json(actual.costs)!=c.at("costs")){
   for(size_t p=0;p<actual.costs.size();++p)if(actual.costs[p]!=c.at("costs").at(p))throw std::runtime_error("movement grid mismatch case "+std::to_string(index)+" tile "+std::to_string(p)+": "+std::to_string(actual.costs[p])+" != "+c.at("costs").at(p).dump());
  }
  ++index;
 }
 std::cout<<index<<" original movement-search grids and return values passed\n";
 for(const auto& c:fixture.at("points")){
  auto u=load_unit_record(data.at("heroes").at(0));u.job=c.at("job");u.equipment={{{255,255,255},{255,255,255},{c.at("accessory"),255,255}}};
  BattleUnit b;b.grades[5]=c.at("grade");b.status=c.at("status");
  if(battle_movement_points(b,u,data)!=c.at("points"))throw std::runtime_error("movement-point getter mismatch: "+c.dump());
 }
 std::cout<<fixture.at("points").size()<<" original movement-point cases passed\n";
 auto routes=read_json(root.parent_path().parent_path()/"tests/fixtures/original_routes.json");index=0;
 for(const auto& c:routes.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));
  std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  auto result=battle_movement_route(b,u,c.at("origin").get<std::array<int,2>>(),c.at("destination").get<std::array<int,2>>(),c.at("budget"),c.at("score_on_failure"),c.at("width"),c.at("height"),c.at("terrain"),c.at("costs").get<std::vector<int>>(),c.at("route_before").get<std::vector<int>>(),c.at("scores_before").get<std::vector<int>>(),roster,data);
  if(Json(result.scratch)!=c.at("route_after"))throw std::runtime_error("route scratch mismatch "+std::to_string(index));
  if(Json(result.scores)!=c.at("scores_after"))throw std::runtime_error("route fallback score mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original route scratch and fallback-score grids passed\n";
 auto walking=read_json(root.parent_path().parent_path()/"tests/fixtures/original_movement_animation.json");index=0;
 for(const auto& c:walking.at("cases")){
  auto step=battle_walking_step(BattleUnit::from_json(c.at("battle")),c.at("job"),c.at("direction"),c.at("speed"),c.at("camera").get<std::array<int,2>>(),20,20,c.at("occupancy").get<std::vector<int>>());
  if(step.actor.json()!=c.at("after")||step.events!=c.at("events"))throw std::runtime_error("walking animation mismatch "+std::to_string(index)+": "+step.events.dump()+" != "+c.at("events").dump());
  ++index;
 }
 std::cout<<index<<" original walking frame/timing/scroll/overlay sequences passed\n";
 auto movement=read_json(root.parent_path().parent_path()/"tests/fixtures/original_ai_movement.json");index=0;
 for(const auto& c:movement.at("cases")){
  std::vector<BattleUnit> roster;for(const auto& r:c.at("roster"))roster.push_back(BattleUnit::from_json(r));
  auto m=battle_ai_movement(BattleUnit::from_json(c.at("battle")),UnitRecord::from_json(c.at("unit")),c.at("destination").get<std::array<int,2>>(),c.at("camera").get<std::array<int,2>>(),c.at("speed"),c.at("width"),c.at("height"),c.at("terrain"),c.at("distance").get<std::vector<int>>(),c.at("route").get<std::vector<int>>(),roster,data);
  if(m.actor.json()!=c.at("after")||Json(m.occupancy)!=c.at("after_occupancy")||Json(m.camera)!=c.at("after_camera")||m.events!=c.at("events"))throw std::runtime_error("AI movement execution mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original AI movement states, occupancy and presentation requests passed\n";
 auto script=read_json(root.parent_path().parent_path()/"tests/fixtures/original_script_movement.json");
 Json tactics=Json::array();for(const auto& t:read_json(root/"tactics.json"))tactics.push_back(t.at("fields"));
 index=0;size_t pixels_checked=0;
 for(const auto& c:script.at("cases")){
  std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
  for(const auto& row:c.at("before").at("units")){roster.push_back(BattleUnit::from_json(row.at("battle")));records.push_back(UnitRecord::from_json(row.at("unit")));}
  int id=c.at("args").at(1);auto pos=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.id==id;})-roster.begin();
  auto camera=c.at("before").at("camera").get<std::array<int,2>>();for(auto& n:camera)n/=48;
  auto actual=scripted_battle_movement(roster.at(pos),records.at(pos),{c.at("args").at(3),c.at("args").at(4)},c.at("args").at(5),camera,1,20,20,script.at("terrain"),roster,records,data,tactics);
  auto actor=[&](const Json& st){for(const auto& row:st.at("units"))if(row.at("battle").at("id")==id)return row.at("battle");throw std::runtime_error("missing script actor");};
  if(actual.actor.json()!=actor(c.at("after"))||Json::array({actual.camera[0]*48,actual.camera[1]*48})!=c.at("after").at("camera"))throw std::runtime_error("scripted movement final state mismatch "+std::to_string(index)+": "+actual.actor.json().dump());
  for(const auto& row:c.at("captures")){
   if(row.at("routine")=="0x43ad1c"&&actual.prepared.json()!=actor(row))throw std::runtime_error("script movement setup differs from original");
   if(row.at("routine")=="0x4595da"&&actual.moved.json()!=actor(row))throw std::runtime_error("script movement AI tail differs from original");
  }
  Json events=Json::array();for(auto event:actual.events){
   if(event.at("kind")=="camera_request"){
    auto focus=camera;for(auto p:battle_camera_path(camera[0]*48,camera[1]*48,event.at("x"),event.at("y"),20,20))focus={p[0]/48,p[1]/48};
    events.push_back({{"kind","camera"},{"tile",focus}});
   }else if(event.at("kind")=="redraw"){
    events.push_back({{"kind","unit_redraw"},{"slot",roster.at(pos).slot}});events.push_back({{"kind","minimap"}});
   }else events.push_back(event);
  }
  if(events.size()!=c.at("events").size())throw std::runtime_error("script movement event count mismatch "+std::to_string(index));
  for(size_t n=0;n<events.size();++n){auto a=events[n],b=c.at("events").at(n);
   if(a.at("kind")=="frame"){
    std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<battle_sprite(id,records.at(pos).job,roster.at(pos).side)<<".bin";
    std::ifstream f(root/"raw/Unit_mov.e5"/filename.str(),std::ios::binary);std::string pixels(48*48,'\0');
    f.seekg(a.at("frame").get<int>()*48*48);f.read(pixels.data(),pixels.size());if(!f)throw std::runtime_error("missing walking pixels");
    if(a.at("mirror").get<bool>())for(int y=0;y<48;++y)std::reverse(pixels.begin()+y*48,pixels.begin()+(y+1)*48);
    if(sha256(pixels)!=b.at("pixels_sha256").get<std::string>())throw std::runtime_error("scripted walk pixel mismatch "+std::to_string(index));
    a.erase("frame");a.erase("mirror");b.erase("matches");b.erase("pixels_sha256");++pixels_checked;
   }
   if(a!=b)throw std::runtime_error("scripted movement event mismatch "+std::to_string(index)+" step "+std::to_string(n)+": "+a.dump()+" != "+b.dump());
  }
  ++index;
 }
 std::cout<<index<<" original scripted walks and "<<pixels_checked<<" frame hashes passed\n";
 auto player=read_json(root.parent_path().parent_path()/"tests/fixtures/original_player_movement.json");index=0;
 for(const auto& c:player.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle"));auto u=UnitRecord::from_json(c.at("unit"));
  std::vector<BattleUnit> roster;for(const auto& row:c.at("roster"))roster.push_back(BattleUnit::from_json(row));
  auto state=begin_player_movement(b,u,c.at("camera").get<std::array<int,2>>(),20,20,c.at("terrain"),roster,data);
  Json events=state.events;
  for(const auto& pick:c.at("picks")){
   if(!state.waiting||state.actor.json()!=pick.at("before")||Json(state.distance)!=pick.at("distance"))throw std::runtime_error("player picker state mismatch "+std::to_string(index));
   state=choose_player_movement(state,pick.at("tile").get<std::array<int,2>>(),u,c.at("speed"),20,20,c.at("terrain"),roster,data);
   for(const auto& e:state.events)events.push_back(e);
  }
  if(state.waiting||int(state.action_menu)!=c.at("result")||Json::array({state.mode})!=c.at("modes")||state.actor.json()!=c.at("after")||Json(state.camera)!=c.at("after_camera")||Json(state.occupancy)!=c.at("after_occupancy"))throw std::runtime_error("player movement state/result mismatch "+std::to_string(index));
  if(events!=c.at("events")){
   for(size_t n=0;n<std::min(events.size(),c.at("events").size());++n)if(events.at(n)!=c.at("events").at(n))throw std::runtime_error("player movement event mismatch "+std::to_string(index)+" step "+std::to_string(n)+": "+events.at(n).dump()+" != "+c.at("events").at(n).dump());
   throw std::runtime_error("player movement event count mismatch "+std::to_string(index));
  }
  bool rejected=false;try{choose_player_movement(state,{255,255},u,c.at("speed"),20,20,c.at("terrain"),roster,data);}catch(const std::exception&){rejected=true;}
  if(!rejected)throw std::runtime_error("finished player movement accepted a duplicate response");
  ++index;
 }
 std::cout<<index<<" original player movement eligibility, selection/retry/cancel, occupancy, flags and ordered presentation sequences passed\n";
 auto actions=read_json(root.parent_path().parent_path()/"tests/fixtures/original_player_action.json");index=0;
 for(const auto& c:actions.at("cases")){
  auto before=BattleUnit::from_json(c.at("before"));auto u=UnitRecord::from_json(c.at("unit"));
  const auto& menu=c.at("menus").at(0);auto b=BattleUnit::from_json(menu.at("battle"));
  std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
  for(const auto& row:c.at("roster")){auto other=BattleUnit::from_json(row);roster.push_back(other.id==b.id?b:other);}
  for(const auto& row:c.at("records"))records.push_back(UnitRecord::from_json(row));
  const auto& g=menu.at("range_before").at("grids");
  RangeGrids grids{g.at("distance"),g.at("threat"),g.at("mask")};
  auto prepared=prepare_player_command_menu(b,u,roster,records,c.at("inventory"),20,20,grids,menu.at("range_before").at("preferred"),data,tactics);
  Json flags=Json::array();for(bool enabled:prepared.enabled)flags.push_back(int(enabled));
  Json actual_grids={{"distance",prepared.attack.grids.distance},{"threat",prepared.attack.grids.threat},{"mask",prepared.attack.grids.mask}};
  if(flags!=menu.at("flags")||actual_grids!=menu.at("grids")||Json(prepared.attack.targets)!=menu.at("targets"))throw std::runtime_error("player action menu mismatch "+std::to_string(index)+": "+flags.dump()+" != "+menu.at("flags").dump());
  std::vector<int> occupied(400,255);for(const auto& other:roster)if(other.lifecycle==2)occupied.at(other.y*20+other.x)=other.slot;
  auto tail=finish_player_command(before,b,c.at("input").at("command"),20,20,occupied);
  Json expected=Json::array();for(size_t n=menu.at("event_index");n<c.at("events").size();++n)expected.push_back(c.at("events").at(n));
  if(tail.actor.json()!=c.at("after")||Json(tail.occupancy)!=c.at("occupancy")||int(tail.completed)!=c.at("result")||tail.events!=expected)
   throw std::runtime_error("player action tail mismatch "+std::to_string(index)+": "+tail.events.dump()+" != "+expected.dump());
  for(int unverified:{1,2,3}){
   bool rejected=false;try{finish_player_command(before,b,unverified,20,20,occupied);}catch(const std::exception&){rejected=true;}
   if(!rejected)throw std::runtime_error("unverified player callback did not stop");
  }
  ++index;
 }
 std::cout<<index<<" original player menu availability/grids and wait/cancel/rollback cases passed\n";
 auto input=read_json(root.parent_path().parent_path()/"tests/fixtures/original_player_input.json");
 size_t hover_checks=0,remaining_checks=0;
 for(const auto& c:input.at("cases")){
  std::vector<BattleUnit> roster;for(const auto& row:c.at("roster"))roster.push_back(BattleUnit::from_json(row));
  for(const auto& frame:c.at("frames")){
   auto actor=BattleUnit::from_json(frame.at("before").at("actor"));
   for(auto& b:roster)if(b.slot==actor.slot)b=actor;
   const auto& spec=frame.at("input");const auto& ready=frame.at("hover_ready");
   auto hover=update_player_hover(frame.at("before").at("hover"),spec.at("tile").get<std::array<int,2>>(),spec.value("retain",0)!=0,
    spec.value("focus",1)==1,spec.value("covered",0)!=0,spec.value("disabled",0)!=0,roster);
   Json expected=Json::array();
   for(size_t n=frame.at("event_index");n<ready.at("event_end").get<size_t>();++n){const auto& event=c.at("events").at(n);if(event.at("kind")=="panel"||event.at("kind")=="clear_panel")expected.push_back(event);}
   if(hover.slot!=ready.at("hover")||int(hover.retained)!=ready.at("retain")||hover.events!=expected)
    throw std::runtime_error("player hover transition mismatch "+c.at("name").get<std::string>());
   ++hover_checks;
  }
  auto actor=BattleUnit::from_json(c.at("after").at("actor"));for(auto& b:roster)if(b.slot==actor.slot)b=actor;
  for(const auto& event:c.at("trace"))if(event.at("kind")=="remaining_manual_units"){
   if(remaining_player_units(roster)!=event.at("count"))throw std::runtime_error("remaining player count mismatch "+c.at("name").get<std::string>());
   ++remaining_checks;
  }
 }
 std::cout<<hover_checks<<" original hover transitions and "<<remaining_checks<<" post-action candidate scans passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
