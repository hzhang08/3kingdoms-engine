#include "caocao/engine.hpp"
#include "caocao/battle.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
using namespace caocao;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
std::vector<uint8_t> bytes(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("missing "+path.string());return {std::istreambuf_iterator<char>(f),{}};}
std::string hash_bytes(const std::vector<uint8_t>& b){return sha256(std::string(reinterpret_cast<const char*>(b.data()),b.size()));}
void compare(const Engine& e,const Json& reference){
 const auto& s=e.state();for(const auto& r:reference.at("units")){const auto& u=s.units.at(std::to_string(r.at("unit").at("id").get<int>()));check(u.at("battle")==r.at("battle"),"exit battle unit differs");check(UnitRecord::from_json(u).json()==r.at("unit"),"exit persistent record differs");}
 check(s.rng.state==reference.at("rng")&&s.script_fields.at("battle_tick")==reference.at("tick"),"exit RNG or tick differs");
 check(Json::array({s.camera_x,s.camera_y})==reference.at("camera"),"exit camera differs");
 check(s.terrain==reference.at("terrain")&&s.terrain_marks==reference.at("effects")&&s.terrain_effects==reference.at("records"),"exit terrain differs");
 std::vector<int> occupancy(s.terrain.size(),255);for(const auto& u:s.units)if(u.contains("battle")){auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle==2)occupancy.at(b.y*20+b.x)=b.slot;}
 check(Json(occupancy)==reference.at("occupancy"),"exit occupancy differs");
}
int main(int argc,char** argv){try{
 check(argc==2,"assets required");std::filesystem::path assets=argv[1],fixtures=assets.parent_path().parent_path()/"tests/fixtures";
 auto disappearance=read_json(fixtures/"original_disappearance.json"),banners=read_json(fixtures/"original_start_banner.json"),original=read_json(fixtures/"original_tutorial_exit.json");
 auto maintenance=read_json(fixtures/"original_phase_maintenance.json");
 Json poison_panels=Json::array(),poison_waits=Json::array(),panel=Json::array();
 bool numeric=false;int poison_id=-1,poison_hp=-1;
 for(const auto& event:maintenance.at("phases").at(1).at("events")){
  auto kind=event.at("kind");if(kind=="numbers_begin"){
   numeric=true;int slot=event.at("args").at(0);
   for(const auto& u:event.at("before").at("units"))if(u.at("battle").at("slot")==slot){poison_id=u.at("battle").at("id");poison_hp=u.at("battle").at("hp");}
  }else if(numeric&&kind=="wait"){poison_panels.push_back(panel);poison_waits.push_back(event.at("ticks"));}
  else if(kind=="clear_panel")numeric=false;
  else if(numeric&&kind!="cursor"&&kind!="clear_cursor")panel.push_back(event);
 }
 check(poison_panels.size()==6&&poison_id>=0,"missing original poison interpolation reference");
 const auto mark=bytes(assets/"raw/Mark.e5/000.bin");size_t frames=0;
 for(const auto& c:disappearance.at("cases")){
  std::vector<BattleUnit> units;for(const auto& r:c.at("before"))units.push_back(BattleUnit::from_json(r));
  auto b=*std::find_if(units.begin(),units.end(),[&](const auto& u){return u.slot==c.at("spec").at("slot");});
  auto steps=scripted_battle_disappearance(units,b.id,c.at("sprite"),c.at("terrain"));Json events=Json::array();int cx=96,cy=576;
  for(auto step:steps){auto kind=step.at("kind");
   if(kind=="unit_state"||kind=="tile"){
    auto next=BattleUnit::from_json(step.at("battle"));for(auto& u:units)if(u.slot==next.slot)u=next;
    if(kind=="tile")events.push_back(step);
   }else if(kind=="camera_visible"){
    int tx=std::clamp(cx/48,b.x-12,b.x),ty=std::clamp(cy/48,b.y-7,b.y);
    if(cx!=tx*48||cy!=ty*48){events.push_back({{"kind","camera"},{"tile",{tx,ty}}});cx=tx*48;cy=ty*48;}
   }else if(kind=="frame"){
    std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<step.at("sprite").get<int>()<<".bin";
    std::string archive=step.at("archive")=="UNIT_MOV.E5"?"Unit_mov.e5":"Unit_spc.e5";auto raw=bytes(assets/"raw"/archive/filename.str());
    int start=step.at("frame").get<int>()*2304,count=step.at("width").get<int>()*step.at("height").get<int>();check(start+count<=int(raw.size()),"sprite outside imported atlas");
    std::vector<uint8_t> pixels(raw.begin()+start,raw.begin()+start+count);int mask=step.at("mask_offset");step.erase("mask_offset");step["x"]=b.x*48-cx;step["y"]=b.y*48-cy+56;step["pixels_sha256"]=hash_bytes(pixels);events.push_back(step);
    events.push_back({{"kind","raster"},{"x",step.at("x")},{"y",step.at("y")},{"width",step.at("width")},{"height",step.at("height")},{"pixels_sha256",hash_bytes(mask_battle_sprite(std::move(pixels),mask,mark))}});++frames;
   }else events.push_back(step);
  }
  Json after=Json::array();for(const auto& u:units)after.push_back(u.json());
  check(events==c.at("events"),"original disappearance presentation differs: "+c.at("spec").dump());check(after==c.at("after"),"disappearance cleanup differs");
  check(Json::array({cx,cy})==c.at("camera"),"disappearance camera differs");
 }
 const auto labels=read_json(assets/"ui.json").at("objective_labels");
 for(const auto& c:banners.at("cases")){
  check(battle_start_banner(labels.at("start").at("text"),c.at("viewport"),c.at("client"))==c.at("events"),"battle-start banner differs");
  std::string checker(2304,0);for(int y=0;y<48;++y)for(int x=y%2;x<48;x+=2)checker[y*48+x]=31;
  check(sha256(checker)==c.at("checker_sha256").get<std::string>()&&c.at("final_color")==255,"battle-start checker differs");
 }
 for(int story:{1,2}){
  Engine e(assets),restored(assets);e.start_scenario(1);bool tutorial=false,mirrored=false,exit=false,banner=false,poison=false;int poses=0,sounds=0,poison_pauses=0;std::string last_save,trace;
  for(int n=0;n<6500&&e.state().phase!="blocked";++n){const auto& s=e.state();auto type=s.presentation.value("type","");
   if(type=="tutorial_begin")tutorial=true;
   if(type=="scripted_disappearance"){compare(e,original.at("before"));exit=true;}
   if(type=="battle_start_banner"){compare(e,original.at("commands").at(2).at("before"));banner=true;}
   if(type=="phase_poison"){compare(e,maintenance.at("phases").at(0).at("after"));poison=true;}
   if(s.battle_stage=="phase_after_maintenance"){
    bool rejected=false;try{e.serialize();}catch(const std::exception&){rejected=true;}check(rejected,"transient maintenance snapshot accepted");
    if(type=="animation_wait"){
     check(s.units.at(std::to_string(poison_id)).at("battle").at("hp")==poison_hp,"poison HP changed before display returned");
     const auto& actual=s.pending_action.at("ui").at("panel");const auto& expected=poison_panels.at(poison_pauses);
     if(actual!=expected){size_t n=0;while(n<std::min(actual.size(),expected.size())&&actual.at(n)==expected.at(n))++n;throw std::runtime_error("sequential poison panel differs at pause "+std::to_string(poison_pauses)+" operation "+std::to_string(n)+": "+(n<actual.size()?actual.at(n).dump():"<end>")+" vs "+(n<expected.size()?expected.at(n).dump():"<end>"));}
     check(s.presentation.at("duration_ms")==poison_waits.at(poison_pauses).get<int>()*50,"poison panel wait differs");++poison_pauses;
    }
   }
   if(!s.pending_action.is_null()&&s.pending_action.value("opcode",-1)==49){
    bool rejected=false;try{e.serialize();}catch(const std::exception&){rejected=true;}check(rejected,"transient disappearance snapshot accepted");
    if(type=="sound")++sounds;
    if(type=="animation_wait"){
     const auto& pose=s.battle_poses.at("157");int h=pose.at("height");check(h==(poses==0?48:52-poses*4),"integrated exit crop order differs");check(pose.at("mask_offset")==0x39c0,"guide terrain mask missing");++poses;
    }
   }
   if(s.stable){
    if(tutorial){last_save=e.serialize();restored.restore(last_save);mirrored=true;}
    Command command{type=="tutorial_command_menu"?"tutorial_command":type=="choice"?"choose":"continue",type=="tutorial_command_menu"?255:type=="choice"?(tutorial?1:story):0};e.submit(command);if(mirrored)restored.submit(command);
   }else{e.advance();if(mirrored)restored.advance();}
   if(mirrored)check(e.state_hash()==restored.state_hash(),"exit continuation differs after restore");
   trace+=e.state_hash()+"\n";
  }
  check(exit&&banner&&poses==13&&sounds==2,"integrated exit omitted animation or banner");
  check(poison&&poison_pauses==6,"poison maintenance omitted display steps");
  check(e.state().diagnostic.find("player movement/combat input is not yet verified")!=std::string::npos,"did not stop at the player-input boundary: "+e.state().diagnostic);
  compare(e,maintenance.at("after"));check(e.state().variables.at(13)==1&&original.at("script_variable13")==1,"event completion latch differs");
  restored.restore(last_save);int exits=0;
  for(int n=0;n<100&&restored.state().phase!="blocked";++n){if(restored.state().presentation.value("type","")=="scripted_disappearance")++exits;if(restored.state().stable)restored.submit({"continue"});else restored.advance();}
  check(exits==1&&restored.state_hash()==e.state_hash(),"older farewell snapshot duplicated or skipped exit");
  std::cout<<"story "<<story<<" trace_sha256="<<sha256(trace)<<" final_state_sha256="<<e.state_hash()<<'\n';
 }
 std::cout<<disappearance.at("cases").size()<<" original disappearances; "<<frames<<" source and masked pixel hashes; "<<banners.at("cases").size()<<" start banners; 2 integrated tutorial/poison/player-gate traces and deterministic restores passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
