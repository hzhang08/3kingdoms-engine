#include "caocao/engine.hpp"
#include "caocao/tutorial.hpp"
#include "caocao/unit_rules.hpp"
#include "caocao/movement.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace caocao;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f,const char* message){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,message);}
void compare(const Engine& e,const Json& reference,bool cursor=true){
 const auto& s=e.state();
 for(const auto& row:reference.at("units")){
  int id=row.at("unit").at("id");const auto& u=s.units.at(std::to_string(id));
  check(u.at("battle")==row.at("battle"),"tutorial battle record differs for unit "+std::to_string(id));
  check(UnitRecord::from_json(u).json()==row.at("unit"),"tutorial persistent record differs");
 }
 check(s.rng.state==reference.at("rng"),"tutorial RNG differs");
 check(s.script_fields.value("battle_tick",0)==reference.at("tick"),"tutorial tick differs");
 check(Json::array({s.camera_x,s.camera_y})==reference.at("camera"),"tutorial camera differs at "+s.presentation.dump());
 check(s.terrain==reference.at("terrain")&&s.terrain_marks==reference.at("effects")&&s.terrain_effects==reference.at("records"),"tutorial map/effects differ");
 if(cursor)check(s.tutorial.at("cursor")==reference.at("tutorial_cursor"),"tutorial cursor differs");
 if(!s.tutorial.is_null()&&reference.contains("movement")){
  const auto& ui=s.tutorial.at("ui");
  check(ui.at("movement").get<bool>()==(reference.at("movement").get<int>()!=0)&&ui.at("range")==reference.at("range"),"tutorial display mask differs");
  if(reference.contains("blue_movement"))check(ui.at("blue_movement").get<bool>()==(reference.at("blue_movement").get<int>()!=0),"tutorial blue flag differs");
  if(reference.at("range_computed"))check(ui.at("distance")==reference.at("distance"),"tutorial computed distance scratch differs");
  if(reference.contains("selection_cursor"))check(ui.at("cursor")==reference.at("selection_cursor"),"tutorial selection cursor differs: "+ui.at("cursor").dump()+" != "+reference.at("selection_cursor").dump()+" at "+s.presentation.dump());
  if(reference.value("panel_kind",Json())=="terrain")check(!ui.at("panel").is_null()&&ui.at("panel_tile")==reference.at("tutorial_cursor"),"tutorial terrain panel differs");
  else if(reference.at("panel_slot").is_null())check(ui.at("panel").is_null(),"tutorial panel did not close");
  else check(!ui.at("panel").is_null()&&s.units.at(std::to_string(ui.at("panel_unit").get<int>())).at("battle").at("slot")==reference.at("panel_slot"),"tutorial panel target differs");
 }
}
int main(int argc,char** argv){try{
 check(argc==2,"assets path required");std::filesystem::path assets=argv[1];auto fixtures=assets.parent_path().parent_path()/"tests/fixtures";
 auto setup=read_json(fixtures/"original_tutorial_entry.json"),original=read_json(fixtures/"original_tutorial_basic.json"),ally=read_json(fixtures/"original_tutorial_ally.json"),actions=read_json(fixtures/"original_tutorial_actions.json"),menu=read_json(fixtures/"original_tutorial_menu_event.json"),terrain=read_json(fixtures/"original_tutorial_terrain.json");
 Engine profile(assets);
 // This guide was not one of the earlier movement fixtures. Verify all four
 // walking frames against pixels captured from its actual tutorial walk.
 std::vector<BattleUnit> roster;std::vector<UnitRecord> records;size_t guide=0;
 for(const auto& row:ally.at("before").at("units")){
  if(row.at("unit").at("id")==157)guide=roster.size();
  roster.push_back(BattleUnit::from_json(row.at("battle")));records.push_back(UnitRecord::from_json(row.at("unit")));
 }
 Json tactics=Json::array();for(const auto& row:read_json(assets/"tactics.json"))tactics.push_back(row.at("fields"));
 auto walk=scripted_battle_movement(roster.at(guide),records.at(guide),{10,0},-1,{3,0},1,20,20,ally.at("before").at("terrain"),roster,records,profile.data(),tactics);
 Json expected_frames=Json::array();for(const auto& e:ally.at("events"))if(e.at("kind")=="frame"&&e.at("matches").at(0).at("archive")=="UNIT_MOV.E5")expected_frames.push_back(e);
 size_t frame=0;
 for(const auto& e:walk.events)if(e.at("kind")=="frame"){
  const auto& ref=expected_frames.at(frame++);const auto& match=ref.at("matches").at(0);
  int sprite=battle_sprite(roster.at(guide).id,records.at(guide).job,roster.at(guide).side);
  check(e.at("x")==ref.at("x")&&e.at("y")==ref.at("y")&&e.at("frame")==match.at("frame")&&e.at("mirror")==match.at("mirror")&&sprite==match.at("sprite"),"tutorial walking request differs");
  std::ostringstream name;name<<std::setw(3)<<std::setfill('0')<<sprite<<".bin";std::ifstream f(assets/"raw/Unit_mov.e5"/name.str(),std::ios::binary);std::string pixels(2304,'\0');
  f.seekg(e.at("frame").get<int>()*2304);f.read(pixels.data(),pixels.size());check(bool(f),"missing tutorial walking pixels");
  if(e.at("mirror").get<bool>())for(int y=0;y<48;++y)std::reverse(pixels.begin()+y*48,pixels.begin()+(y+1)*48);
  check(sha256(pixels)==ref.at("pixels_sha256").get<std::string>(),"tutorial walking pixels differ");
 }
 check(frame==4&&frame==expected_frames.size(),"tutorial walk omitted a frame");
 for(const auto& sample:setup.at("setup")){
  auto [count,all]=tutorial_collection_status(sample.at("collection").get<std::vector<int>>(),profile.data());
  check(count==sample.at("after").at("completions")&&int(all)==sample.at("after").at("all_treasures"),"collection setup differs from original");
 }
 Json branches=original.at("alternatives");
 for(const auto& continuation:ally.at("continuations")){
  auto branch=original;branch["choices"].push_back(continuation.at("choice"));branch["pending"]=continuation.at("pending");branch["after"]=continuation.at("after");
  for(const auto& event:continuation.at("events"))branch["events"].push_back(event);
  if(continuation.at("pending")=="0x433bc8"){
   for(const auto& event:actions.at("events"))branch["events"].push_back(event);
   branch["events"].push_back({{"kind","menu"},{"before",menu.at("before")}});
   for(const auto& response:menu.at("branches")){auto completed=branch;completed["menu_response"]=response.at("response");completed["choices"].push_back(response.at("choice"));completed["pending"]=response.at("pending");completed["after"]=response.at("after");for(const auto& event:response.at("events"))completed["events"].push_back(event);if(response.at("choice")==0){for(const auto& event:terrain.at("events"))completed["events"].push_back(event);completed["after"]=terrain.at("after");completed["pending"]=nullptr;completed["terrain_lesson"]=true;}branches.push_back(completed);}
  }else branches.push_back(branch);
 }
 int snapshots=0,corruptions=0,interactions=0;
 for(int story:{1,2})for(const auto& branch:branches){
  Engine e(assets),restored(assets);e.start_scenario(1);bool entered=false,mirrored=false;int on=0,off=0,waits=0;size_t speech=0,answer=0;std::string first_save;
  Json expected=Json::array();for(const auto& ev:branch.at("events"))if(ev.at("kind")=="speech"||ev.at("kind")=="choice"||ev.at("kind")=="menu")expected.push_back(ev);
  for(int step=0;step<5000&&e.state().phase!="blocked";++step){const auto& s=e.state();auto type=s.presentation.value("type","");
   if(entered&&s.tutorial.is_null()&&!s.pending_action.is_null()&&s.pending_action.value("opcode",-1)==49)break;
   if(type=="tutorial_begin"){entered=true;compare(e,branch.at("before"),false);check(s.collection.at(26)==1,"opening equipment did not update collection");}
   if(entered){
    if(type=="tutorial_mark"){if(s.presentation.at("value")==3)++on;else ++off;}
    if(type=="tutorial_wait"){check(s.presentation.at("duration_ms")==200,"tutorial cursor speed differs");++waits;}
    if(s.stable){
     const auto& ref=expected.at(speech++);++interactions;
     check(type==(ref.at("kind")=="speech"?"dialogue":ref.at("kind")=="menu"?"tutorial_command_menu":"choice")&&(type=="tutorial_command_menu"||s.presentation.at("text")==ref.at("text")),"tutorial interaction order/text differs");
     if(type=="dialogue")check(s.presentation.at("speaker")==e.data().at("heroes").at(ref.at("unit").get<int>()).at("name"),"tutorial speaker differs");
     compare(e,ref.at("before"));
     auto save=e.serialize();restored.restore(save);mirrored=true;++snapshots;check(e.state_hash()==restored.state_hash(),"tutorial snapshot hash changed");
     if(first_save.empty())first_save=save;
     // Mutate a real snapshot, recomputing its checksum to exercise semantic
     // validation. Invalid restoration must never replace the live state.
     for(int badcase=0;badcase<17;++badcase){auto bad=Json::parse(save);auto& st=bad["payload"]["state"];auto& t=st["tutorial"];
      if(badcase==0)t["pc"]=0;
      if(badcase==1)t["answers"].push_back(3);
      if(badcase==2)t["cursor"][0]=99;
      if(badcase==3)t["mark"]=0;
      if(badcase==4)st["collection"][0]=256;
      if(badcase==5)t["completed"]=3;
      if(badcase==6)t["ui"]["tick"]=-1;
      if(badcase==7)st["pending"].push_back(st["presentation"]);
      if(badcase==8){if(type=="tutorial_command_menu")st["presentation"]["controls"][0]["enabled"]=false;else st["presentation"]["text"]="modified";}
      if(badcase==9)t["origin"][0]=99;
      if(badcase==10)t["ui"]["distance"][0]=42;
      if(badcase==11)t["ui"]["range"][0]=42;
      if(badcase==12)t["unit_origins"]["0"][0]=99;
      if(badcase==13)t["return_position"]={99,99};
      if(badcase==14)t["ui"]["blue_movement"]=!t.at("ui").at("blue_movement").get<bool>();
      if(badcase==15)t["menu_result"]=1;
      if(badcase==16){if(type=="tutorial_command_menu")st["presentation"]["focused_control"]=0x507;else t["unit_origins"]["157"][0]=99;}
      bad["sha256"]=sha256(bad["payload"].dump());auto hash=restored.state_hash();
      rejects([&]{restored.restore(bad.dump());},("tutorial malformed save accepted case "+std::to_string(badcase)).c_str());check(restored.state_hash()==hash,"rejected tutorial snapshot changed state");++corruptions;
     }
     if(s.tutorial.at("ui").contains("panel_tile"))for(int n=0;n<4;++n){
      auto bad=Json::parse(save);auto& ui=bad["payload"]["state"]["tutorial"]["ui"];
      if(n==0)ui["panel_tile"][0]=ui["panel_tile"][0].get<int>()+1;
      if(n==1)ui["cursor"]=ui.at("panel_tile");
      if(n==2)ui["panel"][0]["tile"]={0,0};
      if(n==3)ui["panel_underlay"]["cursor"]={255,255};
      bad["sha256"]=sha256(bad["payload"].dump());auto hash=restored.state_hash();
      rejects([&]{restored.restore(bad.dump());},"corrupt terrain preview snapshot accepted");check(restored.state_hash()==hash,"bad terrain preview mutated state");++corruptions;
     }
     if(type=="tutorial_command_menu"){
      auto original_menu=read_json(fixtures/"original_tutorial_menu.json");
      for(const auto& c:s.presentation.at("controls"))if(c.at("id")==2)check(c.at("enabled").get<bool>(),"cancel control is disabled");else for(const auto& host:original_menu.at("tutorial").at("events"))if(host.at("kind")=="enable"&&host.at("control")==c.at("id"))check(c.at("enabled").get<bool>()==(host.at("enabled").get<int>()!=0),"original tutorial menu enablement differs");
      Engine focused(assets),roundtrip(assets);focused.restore(save);focused.submit({"tutorial_menu_focus",0x508});roundtrip.restore(focused.serialize());check(focused.state_hash()==roundtrip.state_hash(),"focused menu snapshot differs");focused.submit({"tutorial_command",2});roundtrip.submit({"tutorial_command",2});check(focused.state_hash()==roundtrip.state_hash(),"focused menu continuation differs");
     }
     if(type=="tutorial_command_menu")for(int value:{0,1,3,5,256}){auto hash=e.state_hash();rejects([&]{e.submit({"tutorial_command",value});},"disabled tutorial command accepted");check(e.state_hash()==hash,"disabled command mutated state");}
     if(type=="choice"){auto hash=e.state_hash();rejects([&]{e.submit({"choose",0});},"invalid tutorial choice accepted");check(e.state_hash()==hash,"invalid choice mutated state");}
    }else rejects([&]{e.serialize();},"tutorial animation save accepted");
   }
   if(s.stable){int value=type=="choice"&&entered?branch.at("choices").at(answer++).get<int>()+1:story;Command cmd{type=="tutorial_command_menu"?"tutorial_command":type=="choice"?"choose":"continue",type=="tutorial_command_menu"?branch.at("menu_response").get<int>():value};e.submit(cmd);if(mirrored)restored.submit(cmd);}
   else{e.advance();if(mirrored)restored.advance();}
   if(mirrored)check(e.state_hash()==restored.state_hash(),"tutorial continuation differs after save/reload");
  }
  check(entered&&speech==expected.size()&&answer==branch.at("choices").size(),"tutorial did not reach all expected interactions: "+e.state().diagnostic+" got "+std::to_string(speech)+" / "+std::to_string(expected.size()));
  bool taught_ally=branch.at("choices").size()>=3;
  bool taught_actions=branch.contains("menu_response"),taught_terrain=branch.value("terrain_lesson",false);
  check(on==off&&on==(taught_terrain?28:taught_actions?19:taught_ally?9:branch.at("choices").at(0)==0?7:0)&&waits==(taught_terrain?20:taught_actions?14:taught_ally?7:on?6:0),"tutorial cursor was skipped or not cleaned up");
  bool next=branch.at("pending")=="0x43419f";
  check(next?(e.state().phase=="blocked"&&e.state().diagnostic.find("Ekd5.exe:0x43419f")!=std::string::npos):(e.state().tutorial.is_null()&&!e.state().pending_action.is_null()&&e.state().pending_action.at("opcode")==49),"tutorial did not reach its exact return boundary: "+e.state().diagnostic);
  compare(e,branch.at("after"),next);check(e.state().choice==story,"tutorial choice overwrote opening story choice");
  restored.restore(first_save);check(restored.state().tutorial.at("pc")==1,"older tutorial snapshot did not restore");
 }
 std::cout<<setup.at("setup").size()<<" original collection setups; 4 original guide walk frames/pixel hashes; "<<branches.size()*2<<" integrated tutorial branches; "<<interactions<<" original interactions; "<<snapshots<<" deterministic snapshots; "<<corruptions<<" malformed restores rejected\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
