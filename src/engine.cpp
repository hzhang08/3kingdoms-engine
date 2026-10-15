#include "caocao/engine.hpp"
#include "caocao/unit_rules.hpp"
#include "caocao/deployment.hpp"
#include "caocao/battle.hpp"
#include "caocao/terrain_effect.hpp"
#include "caocao/battle_events.hpp"
#include "caocao/recovery.hpp"
#include "caocao/ai.hpp"
#include "caocao/movement.hpp"
#include "caocao/selection.hpp"
#include "caocao/panel.hpp"
#include "caocao/attack_controller.hpp"
#include "caocao/range.hpp"
#include "caocao/tactics.hpp"
#include "caocao/tactic_animation.hpp"
#include "caocao/tactic_effect.hpp"
#include "caocao/mcall.hpp"
#include "caocao/tactic_application.hpp"
#include "caocao/tutorial.hpp"
#include <set>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace caocao {
namespace {
Json walking_presentation(const BattleUnit& b,const UnitRecord& record,const AiMovement& movement,int tick,const Json& data,const Json& tactics){
 Json steps=Json::array();
    auto walking=b;
    for(const auto& step:movement.events){auto kind=step.at("kind");
     if(kind=="selection"){
      auto selected=battle_selection_sequence(walking,record,0,255,255,tick,-1,data,tactics);
      for(auto event:selected.events){auto k=event.at("kind");
       if(k=="camera_request")steps.push_back({{"kind","camera_request"},{"x",event.at("args").at(0)},{"y",event.at("args").at(1)}});
       else if(k=="wait")steps.push_back({{"kind","wait"},{"ticks",event.at("args").at(0)}});
       else {event["kind"]="selection_"+k.get<std::string>();steps.push_back(event);}
      }
      tick=selected.tick;
     }else if(kind=="frame"){
      Json frame=step;frame["archive"]="UNIT_MOV.E5";frame["sprite"]=battle_sprite(b.id,record.job,b.side);frame["screen_position"]=true;steps.push_back(std::move(frame));
     }else if(kind=="final_tile"){
      int nx=step.at("x"),ny=step.at("y");walking.direction=ny<walking.y?0:nx>walking.x?1:ny>walking.y?2:3;
      if(walking.direction==1)walking.flags|=8;if(walking.direction==3)walking.flags&=~8;
      walking.x=nx;walking.y=ny;steps.push_back({{"kind","unit_state"},{"battle",walking.json()}});
     }else if(kind=="redraw"){
      steps.push_back({{"kind","unit_state"},{"battle",movement.actor.json()}});
      steps.push_back({{"kind","clear_walking"}});
     }else if(kind=="camera_set")steps.push_back({{"kind","camera"},{"x",step.at("x").get<int>()*48},{"y",step.at("y").get<int>()*48}});
     else steps.push_back(step);
    }
 return steps;
}
}
Json read_json(const std::filesystem::path& p){std::ifstream f(p);if(!f)throw std::runtime_error("Cannot open "+p.string());Json j;f>>j;return j;}
Engine::Engine(const std::filesystem::path& assets){
 auto manifest=read_json(assets/"manifest.json");manifest_=manifest.at("hash");manifest.erase("hash");
 if(manifest.at("schema")!=1||sha256(manifest.dump())!=manifest_)throw std::runtime_error("asset manifest checksum/schema mismatch");
 auto load=[&](const std::string& relative){
  std::string expected;for(const auto& a:manifest.at("assets"))if(a.at("path")==relative){expected=a.at("sha256");break;}
  if(expected.empty())throw std::runtime_error("asset missing from manifest: "+relative);
  std::ifstream f(assets/relative,std::ios::binary);if(!f)throw std::runtime_error("Cannot open "+relative);
  std::string raw((std::istreambuf_iterator<char>(f)),{});if(sha256(raw)!=expected)throw std::runtime_error("asset checksum mismatch: "+relative);
  return Json::parse(raw);
 };
 data_=load("data.json");map_=load("terrain.json");ui_=load("ui.json");poison_effect_=load("poison-effect.json");tactic_records_=load("tactics.json");tactics_=Json::array();for(const auto& t:tactic_records_)tactics_.push_back(t.at("fields"));
 combat_text_=load("combat_text.json");for(int id:{45,48,49})secondary_effects_[std::to_string(id)]=load("effect-"+std::to_string(id)+".json");mcall_=load("mcall-05.json");
 for(auto name:{"R_00","S_00"}){auto script=load("scripts/"+std::string(name)+".json");int section=0;for(const auto& s:script.at("sections")){for(const auto& e:s.at("events")){Program p;p.script=name;p.section=section;p.offset=e.at("offset");p.end=e.at("end");p.conditions=e.at("conditions");compile(e.at("body"),p.code);programs_.push_back(std::move(p));}++section;}}
}
void Engine::compile(const Json& sequence,std::vector<Instruction>& code){
 for(const auto& wire:sequence){
  if(wire.at("opcode")==1){auto n=code.size();code.push_back({wire.at("test"),-1});compile(wire.at("body"),code);code[n].jump=static_cast<int>(code.size());}
  else code.push_back({wire,-1});
 }
}
void Engine::start_program(int id){state_.program=id;state_.script=programs_.at(id).script;state_.pc=0;}
void Engine::start_scenario(uint32_t seed){state_=GameState{};state_.rng.state=seed;state_.terrain=map_.at("cells");state_.terrain_marks=std::vector<int>(state_.terrain.size(),255);start_program(0);advance();}
Json& Engine::unit(int id){
 if(id<0||id>=static_cast<int>(data_.at("heroes").size()))throw std::runtime_error("invalid concrete unit ID "+std::to_string(id));
 auto key=std::to_string(id);if(!state_.units.contains(key)){const auto& h=data_.at("heroes").at(id);auto u=load_unit_record(h).json();u.update({{"name",h.at("name")},{"face",h.at("face")},{"story_sprite",h.at("story_sprite")},{"x",0},{"y",0},{"direction",0},{"action",0},{"visible",false},{"status",Json::object()}});state_.units[key]=std::move(u);}
 return state_.units[key];
}
void Engine::stop(const Json& wire,const std::string& reason){std::ostringstream s;s<<state_.script<<".eex:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<wire.value("offset",0)<<" opcode 0x"<<wire.value("opcode",0)<<": "<<reason;state_.diagnostic=s.str();state_.stable=false;state_.phase="blocked";state_.presentation={{"type","diagnostic"},{"text",state_.diagnostic}};}
void Engine::initialize_battle(){
 const auto& config=state_.script_fields.at("scenario_configuration").at("raw");
 if(config.at(4)!=0||config.at(5)!=-1||config.at(6)!=1||config.at(7)!=0)throw std::runtime_error("unverified battle outcome configuration");
 state_.environment_region=config.at(8)==-1?0:config.at(8).get<int>();
 state_.environment_selector=config.at(9)==-1?1:config.at(9).get<int>();
 battle_environment_class(state_.environment_region,state_.environment_selector);
 int width=map_.at("width"),height=map_.at("height");std::vector<int> occupied(width*height,255);
 for(auto& u:state_.units)u["visible"]=false;
 for(const auto& descriptor:state_.script_fields.at("deployment")){
  auto d=DeploymentSlot::from_json(descriptor);auto& u=unit(d.id);
  auto record=u.at("owner")==0?UnitRecord::from_json(u):load_unit_record(data_.at("heroes").at(d.id));
  auto b=initialize_battle_unit(record,d,data_);prepare_new_battle_unit(b);b.validate(record,data_,width,height);
  if(b.lifecycle==2){auto p=battle_placement(b.x,b.y,record.job<=38?record.job/3:record.job-26,width,height,state_.terrain,occupied,data_);
   if(p[0]==255)throw std::runtime_error("original placement found no free passable tile");b.x=p[0];b.y=p[1];occupied[b.y*width+b.x]=b.slot;}
  u.update(record.json());u["battle"]=b.json();
 }
 state_.background="battlefield";state_.phase="battle_intro";
 // New-battle setup 0x44d9b6 selects party, enemies, then allies (flag 0).
 std::vector<BattleUnit> live;for(const auto& u:state_.units)if(u.contains("battle"))live.push_back(BattleUnit::from_json(u.at("battle")));
 auto focus=new_battle_focus(live);
 state_.camera_x=0;state_.camera_y=0;state_.battle_poses=Json::object();state_.script_fields["battle_tick"]=0;
 for(auto p:battle_camera_path(0,0,focus[0],focus[1],width,height)){state_.camera_x=p[0];state_.camera_y=p[1];}
}
bool Engine::condition(const Json& i){
 const int op=i.at("opcode");const auto& a=i.at("args");auto v=[&](int n){return a.at(n).at("value").get<int>();};
 switch(op){
 case 2:return true;
 case 3:return !state_.previous_condition;
 case 5:{for(auto x:a.at(0).at("value")){int n=x;if(n<0||n>=1024)throw std::runtime_error("variable index outside state");if(!state_.variables[n])return false;}for(auto x:a.at(1).at("value")){int n=x;if(n<0||n>=1024)throw std::runtime_error("variable index outside state");if(state_.variables[n])return false;}return true;}
 case 19:return state_.choice==v(0);
 default:
  if(state_.script=="S_00"){
   BattleEventState scan;scan.turn=state_.turn;scan.phase=state_.battle_phase;scan.outcome=state_.battle_outcome;
   scan.variables=state_.variables;scan.victory=state_.victory;scan.defeat=state_.defeat;
   for(const auto& u:state_.units)if(u.contains("battle"))scan.units.push_back(BattleUnit::from_json(u.at("battle")));
   try{bool result=opening_battle_condition(i,scan);state_.victory=scan.victory;state_.defeat=scan.defeat;return result;}
   catch(const std::exception& e){stop(i,e.what());return false;}
  }
  stop(i,"condition semantics have not been verified for this edition");return false;
 }
}
void Engine::dialogue(const std::string& raw,bool focus_active_speaker){
 size_t pos=0;
 while(pos<raw.size()){
  std::string speaker;auto begin=pos;
  if(raw[pos]=='&'){auto end=raw.find('\n',pos);if(end==std::string::npos)throw std::runtime_error("dialogue speaker has no newline");speaker=raw.substr(pos+1,end-pos-1);begin=end+1;}
  auto end=raw.find('&',begin);if(end==std::string::npos)end=raw.size();std::string content=raw.substr(begin,end-begin);while(!content.empty()&&content.back()=='\n')content.pop_back();
  int face=-1,focus=-1;for(const auto& h:data_.at("heroes"))if(h.at("name")==speaker){face=h.at("face");break;}
  if(focus_active_speaker&&state_.background=="battlefield"&&!speaker.empty())for(const auto& u:state_.units)if(u.at("name")==speaker&&u.contains("battle")){
   auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle!=2){pos=end;focus=-2;break;}
   if((b.flags&6)==6)throw std::runtime_error("dialogue temporary action-flag presentation is not yet verified (0x45971b)");
   focus=b.id;break;
  }
  if(focus==-2)continue;
  // Page boundaries belong to simulation state, so save/resume restores the
  // exact visible page rather than silently advancing a renderer-only cursor.
  std::vector<std::string> lines;std::string line;int width=0;
  for(size_t n=0;n<content.size();){unsigned char c=content[n];size_t length=c<128?1:(c<224?2:(c<240?3:4));if(n+length>content.size())throw std::runtime_error("invalid UTF-8 dialogue");auto ch=content.substr(n,length);n+=length;if(ch=="\n"){lines.push_back(line);line.clear();width=0;continue;}int advance=c<128?8:16;if(width+advance>480){lines.push_back(line);line.clear();width=0;}line+=ch;width+=advance;}
  if(!line.empty()||lines.empty())lines.push_back(line);
  for(size_t n=0;n<lines.size();n+=3){std::string page;for(size_t k=n;k<std::min(n+3,lines.size());++k){if(k!=n)page+="\n";page+=lines[k];}state_.pending.push_back({{"type","dialogue"},{"speaker",speaker},{"face",face},{"text",page}});if(n==0&&focus>=0)state_.pending.back()["focus_unit"]=focus;}pos=end;
 }
}
bool Engine::execute(const Instruction& ins){
 const auto& i=ins.wire;int op=i.at("opcode");const auto& a=i.at("args");auto v=[&](int n){return a.at(n).at("value").get<int>();};
 if(ins.jump>=0&&op!=18){bool pass=condition(i);state_.previous_condition=pass;if(!pass)state_.pc=ins.jump;return state_.phase!="blocked";}
 switch(op){
 case 0:case 2: return true; // Explicit block terminator / non-executable annotation.
 case 8:state_.controls["menu_enabled"]=v(0);return true;
 case 9:state_.presentation={{"type","delay"},{"duration_units",v(0)}};return false;
 case 10:std::fill(state_.variables.begin(),state_.variables.begin()+256,0);return true;
 case 11:if(v(0)<0||v(0)>=1024)throw std::runtime_error("variable index outside state");state_.variables[v(0)]=v(1);return true;
 case 16:{
  if(state_.script!="S_00"||state_.background!="battlefield"||i.at("offset")!=0x2f51||!state_.tutorial.is_null())throw std::runtime_error("unverified tutorial entry");
  auto guide=BattleUnit::from_json(state_.units.at("157").at("battle"));if(guide.lifecycle!=2)throw std::runtime_error("tutorial guide is not active");
  auto [completed,all]=tutorial_collection_status(state_.collection,data_);
  state_.tutorial={{"pc",0},{"completed",completed},{"all_treasures",all},{"origin",{guide.x,guide.y}},{"unit_origins",{{"157",{guide.x,guide.y}},{"0",{state_.units.at("0").at("battle").at("x"),state_.units.at("0").at("battle").at("y")}}}},{"return_position",nullptr},{"menu_result",nullptr},{"cursor",{guide.x,guide.y}},{"mark",-1},{"answers",Json::array()},
   {"ui",{{"cursor",{255,255}},{"movement",false},{"blue_movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}}}};
  state_.presentation={{"type","tutorial_begin"},{"duration_ms",0}};return false;
 }
 case 18:{state_.presentation={{"type","choice"},{"text",a.at(0).at("value")},{"speaker_id",v(1)}};state_.stable=true;return false;}
 case 20:case 22:case 25:dialogue(a.at(0).at("value"));return true;
 case 26:{
  if(state_.background!="battlefield")return true;
  queue_battle_action(battle_objective_banner(a.at(0).at("value"),state_.turn_limit,ui_.at("objective_labels")),i,-1);
  state_.pending_action["banner"]={{"rows",std::vector<int>(battle_view_height/48,255)},{"text",Json::array()},{"tick",state_.script_fields.value("battle_tick",0)}};return true;
 }
 case 92:{
  if(state_.background!="battlefield")return true;
  std::vector<BattleUnit> units;for(const auto& u:state_.units)if(u.contains("battle"))units.push_back(BattleUnit::from_json(u.at("battle")));
  auto steps=scripted_battle_highlight(units,v(0));if(steps.empty())return true;
  queue_battle_action(steps,i,v(0));
  state_.pending_action["ui"]={{"cursor",{255,255}},{"movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};return true;
 }
 case 21:{
  // Opening summon: both alternatives identify Xu Zijiang, who is not deployed.
  // 0x459777 still displays his portrait/text, without a battlefield focus.
  if(state_.background!="battlefield")return true;
  if(v(1)==-1)return true;
  if(v(0)!=157||v(1)!=157)throw std::runtime_error("unverified alternative-speaker dialogue");
  if(state_.units.contains("157")&&state_.units.at("157").contains("battle")&&state_.units.at("157").at("battle").at("lifecycle")==2)throw std::runtime_error("unverified deployed alternative speaker");
  dialogue("&"+data_.at("heroes").at(157).at("name").get<std::string>()+"\n"+a.at(2).at("value").get<std::string>(),false);return true;
 }
 case 28:state_.battle_poses=Json::object();state_.presentation={{"type","redraw"}};return false;
 case 29:state_.presentation={{"type","fade"}};return false;
 case 35:
  if(v(1)!=0&&v(1)!=1&&v(1)!=255){stop(i,"unverified sound repeat mode");return false;}
  if(v(1)!=255)battle_sound_file(v(0));
  state_.script_fields["looping_sound"]=v(1)==0?v(0):-1;
  state_.presentation={{"type","sound"},{"id",v(0)},{"repeat",v(1)}};return false;
 case 36:state_.music=v(0);state_.presentation={{"type","music"},{"track",v(0)}};return false;
 case 39:
  if(v(0)==2&&v(3)==30)state_.background="story";
  else if(v(0)==3&&v(4)==0&&state_.script=="S_00"){initialize_battle();}
  else{stop(i,"unsupported background reference");return false;}
  return true;
 case 47:for(auto& u:state_.units)u["visible"]=false;return true;
 case 48:{auto& u=unit(v(0));u["x"]=v(1);u["y"]=v(2);u["direction"]=v(3);u["action"]=v(4);u["visible"]=true;state_.presentation={{"type","appearance"},{"unit",v(0)}};return false;}
 case 50:{
  if(state_.script=="R_00"){auto& u=unit(v(1));state_.presentation={{"type","move"},{"unit",v(1)},{"from_x",u.at("x")},{"from_y",u.at("y")},{"to_x",v(3)},{"to_y",v(4)}};u["x"]=v(3);u["y"]=v(4);u["direction"]=v(5);return false;}
  if(state_.background!="battlefield"){stop(i,"scripted movement outside initialized battle");return false;}
  if(v(0)!=0&&v(0)!=-1){stop(i,"scripted movement by party-slot selector is not yet verified");return false;}
  std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
  for(const auto& u:state_.units)if(u.contains("battle")){roster.push_back(BattleUnit::from_json(u.at("battle")));records.push_back(UnitRecord::from_json(u));}
  auto found=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.id==v(1);});if(found==roster.end())return true;
  const auto& record=records.at(found-roster.begin());
  auto result=scripted_battle_movement(*found,record,{v(3),v(4)},v(5),{state_.camera_x/48,state_.camera_y/48},1,map_.at("width"),map_.at("height"),state_.terrain,roster,records,data_,tactics_);
  if(!state_.tutorial.is_null()){if(!result.distance.empty())state_.tutorial["ui"]["distance"]=result.distance;state_.tutorial["ui"]["cursor"]={255,255};}
  // The minimap redraw precedes the AI completion and restoration in 0x459487.
  auto walking=result.moved;walking.ai=result.prepared.ai;
  AiMovement movement{walking,result.camera,{},result.events};
  auto steps=walking_presentation(result.prepared,record,movement,state_.script_fields.value("battle_tick",0),data_,tactics_);
  steps.push_back({{"kind","unit_state"},{"battle",result.actor.json()}});
  steps.push_back({{"kind","scripted_movement_done"}});
  queue_battle_action(steps,i,found->id);
  state_.units.at(std::to_string(found->id))["battle"]=result.prepared.json();return true;
 }
 case 51:{auto& u=unit(v(0));u["action"]=v(1);u["direction"]=v(2);return true;}
 case 52:unit(v(0))["action"]=v(1);return true;
 case 58:if(v(0)!=2||v(1)<0||v(1)>2){stop(i,"unsupported property assignment");return false;}if(v(1)==0)state_.ambition=v(2);else state_.ambition+=(v(1)==1?1:-1)*v(2);return true;
 case 59:{if(v(1)!=0){stop(i,"party removal is not implemented");return false;}auto& u=unit(v(0));if(u.at("owner")==0)return true;std::vector<int> levels;
  for(int id=0;id<26;++id){const auto& member=unit(id);if(member.at("owner")==0)levels.push_back(member.at("level"));}
  u.update(join_unit(data_.at("heroes").at(v(0)),original_party_average(levels),v(2),data_.at("classes")).json());return true;}
 case 62:{if(state_.script!="R_00"){stop(i,"battle equipment handler is not implemented");return false;}auto& u=unit(v(0));auto record=UnitRecord::from_json(u);
  for(int id:script_equipment(record,{v(1),v(2),v(3),v(4),v(5)},data_)){state_.script_fields["collected_items"][std::to_string(id)]=true;state_.collection.at(id+3)=1;}
  u.update(record.json());return true;}
 case 74:{// The sole R_00/S_00 deployment choice has no optional party members.
  if(v(0)!=1){stop(i,"interactive deployment selection is not implemented");return false;}
  for(int n=1;n<11;++n)if(v(n)!=-1){stop(i,"deployment selection constraints are not implemented");return false;}
  if(unit(0).at("owner")!=0){stop(i,"opening deployment requires Cao Cao in the party");return false;}
  state_.script_fields["deployment_selected"]=Json::array({0});return true;}
 case 68:{std::fill(state_.variables.begin(),state_.variables.begin()+256,0);state_.script_fields["deployment"]=Json::array();int slot=0;
  for(int id:state_.script_fields.at("deployment_selected")){DeploymentSlot s;s.id=id;s.slot=slot++;state_.script_fields["deployment"].push_back(s.json());}return true;}
 case 69:{state_.turn_limit=v(2);state_.script_fields["scenario_configuration"]={{"raw",Json::array()},{"level_adjustment",v(3)<0?0:v(3)<=50?v(3):50-v(3)}};for(const auto& arg:a)state_.script_fields["scenario_configuration"]["raw"].push_back(arg.at("value"));return true;}
 case 70:case 71:{std::vector<int> levels;for(int id:state_.script_fields.at("deployment_selected"))levels.push_back(unit(id).at("level"));int base=std::max(1,deployment_level(levels)+state_.script_fields.at("scenario_configuration").at("level_adjustment").get<int>());
  int width=op==70?11:12,count=op==70?20:80,begin=op==70?15:35;auto& slots=state_.script_fields.at("deployment");
  if(a.size()!=static_cast<size_t>(width*count))throw std::runtime_error("invalid deployment table");
  for(int n=0;n<count;++n){std::vector<int> row;for(int k=0;k<width;++k)row.push_back(v(n*width+k));auto d=deployment_row(begin+n,op==71,row,base);if(d.id!=65535)slots.push_back(d.json());}return true;}
 case 75:{for(auto& j:state_.script_fields.at("deployment"))if(j.at("slot")==v(0)){auto s=DeploymentSlot::from_json(j);position_deployment(s,v(1),v(2),v(3),v(4));j=s.json();break;}return true;}
 case 72:{for(auto& j:state_.script_fields.at("deployment"))if(j.at("slot").get<int>()>=15&&j.at("id")==v(0)){auto s=DeploymentSlot::from_json(j);equipment_deployment(s,{v(1),v(2),v(3),v(4),v(5)});j=s.json();break;}return true;}
 case 76:{
  if(state_.phase!="battle_intro"&&state_.phase!="battle_event"){stop(i,"appearance outside initialized battle");return false;}
  int mode=v(0)==-1?0:v(0);if(mode!=0&&mode!=1){stop(i,"unverified appearance selector");return false;}
  if(mode==1&&(v(2)<0||v(2)>=15))return true;
  Json* actor=nullptr;std::vector<int> occupancy(state_.terrain.size(),255);
  for(auto& u:state_.units)if(u.contains("battle")){auto b=BattleUnit::from_json(u.at("battle"));
   if(b.lifecycle==2)occupancy.at(b.y*20+b.x)=b.slot;
   if(mode==1?b.slot==v(2):b.id==v(1))actor=&u;
  }
  if(!actor)return true;auto b=BattleUnit::from_json(actor->at("battle"));
  if(b.lifecycle!=1)return true;
  for(const auto& row:state_.script_fields.at("deployment")){auto d=DeploymentSlot::from_json(row);if(d.slot==b.slot){
   queue_battle_action(scripted_battle_appearance(b,d,UnitRecord::from_json(*actor).job,mode==1,state_.terrain,occupancy,data_),i,b.id);return true;
  }}
  stop(i,"reinforcement descriptor is missing");return false;
 }
 case 77:{
  if(state_.background=="battlefield"&&v(0)==2&&v(1)==-1&&v(2)==0&&v(3)==0&&v(4)==0&&v(5)==19&&v(6)==19&&v(7)==5&&v(8)==-1&&v(9)==-1&&v(10)==8&&v(11)==0&&v(12)==0){
   std::vector<BattleUnit> units;for(const auto& u:state_.units)if(u.contains("battle"))units.push_back(BattleUnit::from_json(u.at("battle")));
   Json steps=Json::array();for(const auto& e:scripted_area_confusion(units,{v(3),v(4),v(5),v(6)},v(7),{state_.camera_x,state_.camera_y},map_.at("width"),map_.at("height"),secondary_effects_.at("48")))
    if(e.at("kind")!="archive"&&e.at("kind")!="message_pump"&&e.at("kind")!="capture"&&e.at("kind")!="refresh")steps.push_back(e);
   queue_battle_action(steps,i,-1);return true;
  }
  if(state_.background!="battlefield"||v(0)!=0||v(2)!=0||v(3)!=0||v(4)!=0||v(5)!=0||v(6)!=0||v(7)!=-1||v(8)!=-1||v(9)!=-1||v(10)!=-1||v(12)!=0){stop(i,"this unit-property command form has not been verified");return false;}
  auto& u=unit(v(1));auto b=BattleUnit::from_json(u.at("battle"));scripted_hp_loss(b,v(11));u["battle"]=b.json();return true;}
 case 78:{
  if(state_.background!="battlefield"||v(0)!=-1||v(1)<0||v(1)>=512||v(2)!=0||v(3)!=0||v(4)!=0||v(5)!=0||v(6)!=-1||v(7)!=1||v(8)!=-1||v(9)!=0||v(10)!=0){stop(i,"this AI assignment form has not been verified");return false;}
  auto key=std::to_string(v(1));if(!state_.units.contains(key)||!state_.units.at(key).contains("battle"))return true;
  auto& u=state_.units.at(key);auto b=BattleUnit::from_json(u.at("battle"));scripted_unit_ai(b,UnitRecord::from_json(u),v(7));u["battle"]=b.json();return true;
 }
 case 49:{
  if(state_.background!="battlefield"||v(0)!=0||v(1)<0||v(1)>=512||v(2)!=0||v(3)!=0||v(4)!=0||v(5)!=0||v(6)!=-1){stop(i,"unverified disappearance command form");return false;}
  auto key=std::to_string(v(1));if(!state_.units.contains(key)||!state_.units.at(key).contains("battle"))return true;
  const auto& u=state_.units.at(key);auto b=BattleUnit::from_json(u.at("battle"));
  std::vector<BattleUnit> units;for(const auto& row:state_.units)if(row.contains("battle"))units.push_back(BattleUnit::from_json(row.at("battle")));
  std::sort(units.begin(),units.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
  queue_battle_action(scripted_battle_disappearance(units,b.id,battle_sprite(b.id,UnitRecord::from_json(u).job,b.side),state_.terrain.at(b.y*map_.at("width").get<int>()+b.x)),i,b.id);
  state_.presentation={{"type","scripted_disappearance"},{"unit",b.id},{"duration_ms",0}};return false;
 }
 case 90:{
  if(state_.background!="battlefield"){stop(i,"battle-start banner outside initialized battle");return false;}
  queue_battle_action(battle_start_banner(ui_.at("objective_labels").at("start").at("text")),i,-1);
  state_.pending_action["banner"]={{"cells",std::vector<int>(104,255)},{"text",Json::array()},{"tick",state_.script_fields.value("battle_tick",0)}};
  state_.presentation={{"type","battle_start_banner"},{"duration_ms",0}};return false;
 }
 case 83:{
  if(state_.background!="battlefield"||v(0)!=0||v(1)<0||v(1)>=512||v(2)!=0||v(3)!=0||v(4)!=0||v(5)!=0||v(6)!=-1||v(7)!=0){stop(i,"unverified removal command form");return false;}
  std::vector<BattleUnit> units;for(const auto& u:state_.units)if(u.contains("battle"))units.push_back(BattleUnit::from_json(u.at("battle")));
  std::sort(units.begin(),units.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
  queue_battle_action(scripted_battle_removal(units,v(1)),i,v(1));return true;
 }
 case 79:{
  if(state_.background!="battlefield"){stop(i,"facing outside initialized battle");return false;}
  auto key=std::to_string(v(0));if(!state_.units.contains(key)||!state_.units.at(key).contains("battle"))return true;
  auto b=BattleUnit::from_json(state_.units.at(key).at("battle"));BattleUnit target;const BattleUnit* towards=nullptr;
  if(v(1)!=-1){auto other=std::to_string(v(1));if(!state_.units.contains(other)||!state_.units.at(other).contains("battle"))return true;target=BattleUnit::from_json(state_.units.at(other).at("battle"));towards=&target;}
  queue_battle_action(scripted_battle_facing(b,towards,v(2),v(3)==1,v(4)==1||v(4)==-1,v(5)==1||v(5)==-1),i,b.id);return true;
 }
 case 33:{
  if(state_.background!="battlefield")return true;
  if(v(2)!=0)throw std::runtime_error("fire removal is not yet verified");
  auto steps=fire_placement(state_.terrain_effects,state_.terrain,map_.at("width"),map_.at("height"),v(0)&255,v(1)&255,v(3)!=-1&&v(3)!=0,v(4)!=-1&&v(4)!=0,{state_.camera_x,state_.camera_y});
  queue_battle_action(steps,i,-1);return true;
 }
 case 107:{
  if(v(2)==-1)return true;
  if(state_.background!="battlefield"||(v(2)!=105&&v(2)!=109))throw std::runtime_error("unverified scripted effect request");
  int x=v(0)&255,y=v(1)&255,cx=state_.camera_x,cy=state_.camera_y;Json steps=Json::array();
  if(v(3)!=0){steps.push_back({{"kind","camera_request"},{"x",x},{"y",y}});for(auto point:battle_camera_path(cx,cy,x,y,map_.at("width"),map_.at("height"))){cx=point[0];cy=point[1];}}
  int px=x*48-cx+24,py=y*48-cy+80;
  if(px>=0&&px<624&&py>=56&&py<440){
   if(v(2)==105)steps.push_back({{"kind","mcall_preload"},{"resource",105}});
   else{
    if(state_.mcall_resource!=105)throw std::runtime_error("MCALL playback has no verified preload");
    for(const auto& step:mcall_animation(px,py,mcall_))if(step.at("kind")!="capture")steps.push_back(step);
    steps.push_back({{"kind","mcall_release"}});steps.push_back({{"kind","wait"},{"ticks",4}});
   }
  }
  queue_battle_action(steps,i,-1);return true;
 }
 case 80:{
  if(state_.background!="battlefield"){stop(i,"battle animation outside initialized battle");return false;}
  if(v(1)==-1)return true; // Explicit original no-animation sentinel.
  if(v(1)<0||v(1)>9){stop(i,"unverified scripted battle animation "+std::to_string(v(1)));return false;}
  auto key=std::to_string(v(0));
  if(!state_.units.contains(key)||!state_.units.at(key).contains("battle"))return true;
  auto& u=state_.units.at(key);auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle!=2)return true;
  int x=state_.camera_x,y=state_.camera_y;
  for(auto point:battle_camera_path(x,y,b.x,b.y,map_.at("width"),map_.at("height"))){x=point[0];y=point[1];}
  auto frames=scripted_battle_animation(b,battle_sprite(b.id,UnitRecord::from_json(u).job,b.side),v(1),v(2)==1||v(2)==-1,v(3)==1||v(3)==-1,x/48,y/48);
  queue_battle_action(frames,i,b.id,v(1));return true;
 }
 default:stop(i,"required instruction is decoded but execution is not implemented/verified");return false;
 }
}
void Engine::next_program(){
 if(state_.script=="R_00"){start_program(1);state_.phase="battle_initialization";state_.background="battlefield";state_.presentation={{"type","transition"}};return;}
 if(programs_.at(state_.program).section==0){state_.battle_phase=0;state_.battle_stage="entry_events";state_.event_scans=0;}
 else if(programs_.at(state_.program).section==1)state_.triggered.push_back(programs_.at(state_.program).offset);
 else{stop({{"offset",programs_.at(state_.program).end},{"opcode",-1}},"post-battle transition is not verified");return;}
 scan_battle_events();
}
void Engine::scan_battle_events(){
 BattleEventState scan;scan.turn=state_.turn;scan.phase=state_.battle_phase;scan.outcome=state_.battle_outcome;
 scan.victory=state_.victory;scan.defeat=state_.defeat;scan.variables=state_.variables;
 for(const auto& u:state_.units)if(u.contains("battle"))scan.units.push_back(BattleUnit::from_json(u.at("battle")));
 int selected=-1,count=0;
 for(size_t n=0;n<programs_.size();++n){const auto& p=programs_[n];if(p.script!="S_00"||p.section!=1)continue;++count;}
 // Original caller 0x44c601 bounds a dispatch batch by section event count.
 while(state_.event_scans<count){
  ++state_.event_scans;
  for(size_t n=0;n<programs_.size();++n){const auto& p=programs_[n];if(p.script!="S_00"||p.section!=1)continue;
   bool eligible=true;
   for(const auto& i:p.conditions){try{bool pass=opening_battle_condition(i,scan);eligible=pass&&eligible;}catch(const std::exception& ex){stop(i,ex.what());return;}}
   if(eligible)selected=static_cast<int>(n);
  }
  if(selected>=0)break;
 }
 state_.victory=scan.victory;state_.defeat=scan.defeat;
 if(selected>=0){start_program(selected);state_.phase="battle_event";state_.presentation={{"type","battle_event"},{"offset",programs_.at(selected).offset},{"turn",state_.turn},{"phase",state_.battle_phase}};return;}
 if(state_.script_fields.contains("event_return")){
  state_.battle_stage=state_.script_fields.at("event_return").get<std::string>();state_.script_fields.erase("event_return");
  state_.phase="battle_turn";state_.presentation={{"type","event_batch_complete"},{"scans",state_.event_scans},{"resume",state_.battle_stage}};return;
 }
 state_.battle_stage="phase_start";
 state_.phase="battle_turn";
 advance_battle_phase();
}
void Engine::begin_battle_events(const std::string& resume){
 if(state_.script_fields.contains("event_return"))throw std::runtime_error("nested event dispatch is not verified");
 state_.script_fields["event_return"]=resume;state_.battle_stage="entry_events";state_.event_scans=0;scan_battle_events();
}
void Engine::advance_battle_phase(){
 const Json wire={{"offset",programs_.at(state_.program).end},{"opcode",-1}};
 try{
  std::vector<BattleUnit> roster;std::vector<UnitRecord> records;
  for(const auto& u:state_.units)if(u.contains("battle")){roster.push_back(BattleUnit::from_json(u.at("battle")));records.push_back(UnitRecord::from_json(u));}
  std::sort(roster.begin(),roster.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
  auto record=[&](int id)->const UnitRecord&{return *std::find_if(records.begin(),records.end(),[&](const auto& u){return u.id==id;});};
  if(state_.battle_stage=="phase_start"){
   if(state_.battle_outcome){stop(wire,"battle outcome transition is not yet verified");return;}
   // Allied phase shares the maintenance already performed for the player.
   if(state_.battle_phase!=1){
    auto candidate=roster;auto rng=state_.rng;
    auto belongs=[&](const BattleUnit& b){return (b.slot<35)==(state_.battle_phase==0);};
    auto require_effects=[&](const RecoveryEffects& fx){
     if(!fx.aura_slots.empty()||!fx.changes.empty())throw std::runtime_error("phase-start recovery animation is not yet verified (0x43e69f)");
    };
    // Preflight on copies: unsupported presentation must not consume RNG or
    // partially apply a turn. Silent grade decay still commits on success.
    for(auto& b:candidate)if(belongs(b)){
     require_effects(recover_battle_status(b,record(b.id),candidate,records,rng,data_));
     require_effects(recover_battle_grades(b,record(b.id),rng,data_));
    }
    const auto recovered=candidate;Json poison=Json::array();
    for(auto& b:candidate)if(belongs(b)&&b.status&16){
     int loss=battle_poison_loss(b,record(b.id),data_);
     if(loss&&b.side==0)throw std::runtime_error("party poison result panel is not yet verified (0x450036)");
     poison.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
     poison.push_back({{"kind","numbers"},{"target",b.slot},{"target_before",b.json()},{"actor",255},{"hp",uint32_t(loss)},{"mp",0},{"xp",0},{"weapon_xp",0},{"armor_xp",0},{"wait",true}});
     b.hp-=loss;poison.push_back({{"kind","unit_state"},{"battle",b.json()}});
    }
    for(auto& b:candidate)if(belongs(b)){
     auto gain=battle_regeneration(b,record(b.id),state_.terrain.at(b.y*map_.at("width").get<int>()+b.x),data_);
     if(gain.any())throw std::runtime_error("regeneration/experience presentation is not yet verified (0x450984)");
    }
    // Commit recovery before showing poison, but keep each target's old HP
    // through all five interpolation steps and the terminal wait (44df61).
    for(const auto& b:recovered)state_.units.at(std::to_string(b.id))["battle"]=b.json();
    state_.rng=rng;roster=recovered;
    if(!poison.empty()){
     queue_battle_action(poison,wire,-1);
     state_.pending_action["ui"]={{"cursor",{255,255}},{"movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
     state_.battle_stage="phase_after_maintenance";state_.presentation={{"type","phase_poison"},{"duration_ms",0}};return;
    }
   }
   state_.battle_stage=state_.battle_phase==0?"player_gate":"ai_priority";
  }
  if(state_.battle_stage=="phase_after_maintenance")state_.battle_stage=state_.battle_phase==0?"player_gate":"ai_priority";
  if(state_.battle_stage=="player_gate"){
   auto gate=player_phase_gate(roster);
   if(gate.requires_input){stop(wire,"player movement/combat input is not yet verified (0x44e1d3)");return;}
   queue_battle_action(Json::array({{{"kind","camera_request"},{"x",gate.focus[0]},{"y",gate.focus[1]}}}),wire,-1);
   state_.battle_stage="allied_entry";state_.presentation={{"type","phase_transition"},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="player_entry"){
   state_.battle_phase=0;state_.battle_stage="entry_events";state_.event_scans=0;
   scan_battle_events();return;
  }
  if(state_.battle_stage=="allied_entry"){
   state_.battle_phase=1;state_.battle_stage="entry_events";state_.event_scans=0;
   scan_battle_events();return;
  }
  if(state_.battle_stage=="enemy_entry"){
   state_.battle_phase=2;state_.battle_stage="entry_events";state_.event_scans=0;
   scan_battle_events();return;
  }
  if(state_.battle_stage=="ai_priority"){
   state_.script_fields["ai_order"]=battle_ai_order(roster,records,state_.battle_phase==1,map_.at("width"),state_.terrain,data_);
   state_.script_fields["ai_cursor"]=0;state_.battle_stage="ai_pick";
  }
  if(state_.battle_stage=="ai_pick"){
   // 0x44e851 keeps the initial priority bytes across actions. Sorting again
   // after HP or terrain changes can change which unit acts next.
   const auto& order=state_.script_fields.at("ai_order");
   while(state_.script_fields.at("ai_cursor").get<size_t>()<order.size()){
    int cursor=state_.script_fields.at("ai_cursor");int slot=order.at(cursor);state_.script_fields["ai_cursor"]=cursor+1;
    auto found=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.slot==slot;});
    if(found==roster.end()||found->lifecycle!=2)continue;const auto& b=*found;
    if(b.ai==7||(b.flags&6))continue; // Caller 0x44ea8d..0x44eaf4.
    int width=map_.at("width"),height=map_.at("height");
    auto decision=decide_battle_ai(b,record(b.id),0,roster,records,width,height,state_.terrain,data_,tactics_,state_.rng);
    if(!decision.reached_movement)throw std::runtime_error("AI early-return completion is not yet verified");
    auto camera=std::array<int,2>{state_.camera_x/48,state_.camera_y/48};
    for(auto p:battle_camera_path(state_.camera_x,state_.camera_y,b.x,b.y,width,height))camera={p[0]/48,p[1]/48};
    auto movement=battle_ai_movement(decision.actor,record(b.id),decision.grids.best_tile,camera,1,width,height,state_.terrain,decision.grids.distance,decision.route_scratch,roster,data_);
    auto steps=walking_presentation(decision.actor,record(b.id),movement,state_.script_fields.value("battle_tick",0),data_,tactics_);
    // Compile all supported presentation before committing the decision.
    queue_battle_action(steps,wire,b.id);
    if(!state_.pending_action.is_null())state_.pending_action["ui"]={{"cursor",Json::array({255,255})},{"movement",false},{"distance",decision.grids.distance},{"range",std::vector<int>(width*height)},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
    state_.units.at(std::to_string(b.id))["battle"]=decision.actor.json();
    state_.rng=decision.rng;
    state_.script_fields["ai_pending"]={{"unit",b.id},{"slot",b.slot},{"target",decision.grids.best_target},{"tactic",decision.grids.best_tactic},{"destination",decision.grids.best_tile}};
    state_.battle_stage="ai_resolution";state_.presentation={{"type","ai_movement"},{"unit",b.id},{"duration_ms",0}};return;
   }
   if(state_.battle_phase==2){
    if(state_.battle_outcome){stop(wire,"battle outcome transition is not yet verified");return;}
    int next_turn=(state_.turn&255)+1;
    if(next_turn-1>=state_.turn_limit){stop(wire,"turn-limit defeat presentation is not yet verified (0x44ef07)");return;}
    auto rng=state_.rng;
    int selector=advance_battle_environment(state_.environment_region,state_.environment_selector,255,rng);
    if(battle_environment_class(state_.environment_region,selector)!=battle_environment_class(state_.environment_region,state_.environment_selector)){
     stop(wire,"environment-change presentation is not yet verified (0x44a0e5)");return;
    }
    queue_battle_action(battle_turn_banner(next_turn,state_.turn_limit,false,false,ui_.at("turn_labels")),wire,-1);
    state_.pending_action["banner"]={{"columns",std::vector<int>(battle_view_width/48,255)},{"text",Json::array()},{"tick",state_.script_fields.value("battle_tick",0)}};
    state_.turn=next_turn;state_.rng=rng;state_.environment_selector=selector;
    for(auto key:{"ai_order","ai_cursor","ai_pending"})state_.script_fields.erase(key);
    state_.battle_stage="player_entry";state_.presentation={{"type","turn_banner"},{"duration_ms",0}};return;
   }
   if(state_.battle_phase!=1)throw std::runtime_error("unexpected automatic phase completion");
   queue_battle_action(battle_turn_banner(state_.turn,state_.turn_limit,true,false,ui_.at("turn_labels")),wire,-1);
   state_.pending_action["banner"]={{"columns",std::vector<int>(battle_view_width/48,255)},{"text",Json::array()},{"tick",state_.script_fields.value("battle_tick",0)}};
   for(auto key:{"ai_order","ai_cursor","ai_pending"})state_.script_fields.erase(key);
   state_.battle_stage="enemy_entry";state_.presentation={{"type","turn_banner"},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="ai_resolution"){
   auto pending=state_.script_fields.at("ai_pending");
   auto b=BattleUnit::from_json(state_.units.at(std::to_string(pending.at("unit").get<int>())).at("battle"));auto u=record(b.id);
   int width=map_.at("width"),height=map_.at("height");
   if(pending.at("tactic")!=255){
    int tactic=pending.at("tactic");
    if(tactic!=33)throw std::runtime_error("AI tactic casting branch is not yet verified (0x42335b)");
    auto found=std::find_if(roster.begin(),roster.end(),[&](const auto& other){return other.slot==pending.at("target");});
    if(found==roster.end())throw std::runtime_error("AI tactic target is absent");
    const auto& target=*found;
    // Unlike physical attacks, this selector does not change actor facing.
    auto selection=battle_selection_sequence(b,u,2,target.slot,tactic,state_.script_fields.value("battle_tick",0),-1,data_,tactics_);
    Json steps=Json::array();
    for(auto event:selection.events){auto kind=event.at("kind");
     if(kind=="camera_request")steps.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
     else if(kind=="focus_target")steps.push_back({{"kind","camera_visible"},{"x",target.x},{"y",target.y}});
     else if(kind=="wait")steps.push_back({{"kind","wait"},{"ticks",event.at("args").at(0)}});
     else {event["kind"]="selection_"+kind.get<std::string>();steps.push_back(event);}
    }
    queue_battle_action(steps,wire,b.id);
    state_.pending_action["ui"]={{"cursor",Json::array({255,255})},{"movement",false},{"distance",std::vector<int>(width*height,255)},{"range",std::vector<int>(width*height)},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
    state_.battle_stage="ai_tactic_plan";state_.presentation={{"type","tactic_selection"},{"unit",b.id},{"target",target.id},{"tactic",tactic},{"duration_ms",0}};return;
   }
   RangeGrids grids{std::vector<int>(width*height,255),std::vector<int>(width*height),std::vector<int>(width*height)};
   auto range=battle_range(b,{b.x,b.y},battle_attack_range(u,data_),0,false,pending.at("target"),width,height,std::move(grids),roster);
   if((b.flags&130)||b.ai==4)throw std::runtime_error("AI restricted-attack completion is not yet verified (0x43aafb)");
   if(range.selected==255){
    // 0x43abb8 returns without attacking or consuming RNG. The outer
    // 0x43b3df caller marks a moved, unrestricted unit as finished.
    if((b.flags&4)&&!(b.flags&128)){b.flags|=2;state_.units.at(std::to_string(b.id))["battle"]=b.json();}
    state_.battle_stage="ai_post_action";state_.presentation={{"type","ai_no_attack"},{"unit",b.id},{"duration_ms",0}};return;
   }
   int weapon=u.equipment[0][0];
   if(weapon!=255){const auto& item=data_.at("items").at(weapon);int type=item.at("type"),group=u.job<=38?u.job/3:u.job-26;
    bool usable=type<18?data_.at("classes").at(u.job).at(9+type).get<int>()!=0:type<63&&(item.at("class_group")==255||item.at("class_group")==group);
    if(!usable)throw std::runtime_error("AI unusable-weapon completion is not yet verified (0x407907)");
   }
   auto target=*std::find_if(roster.begin(),roster.end(),[&](const auto& other){return other.slot==range.selected;});
   b.direction=battle_direction(b,target);b.flags=(b.flags&~8)|(b.direction==1?8:0);
   Json steps=Json::array({{{"kind","unit_state"},{"battle",b.json()}}});
   auto selection=battle_selection_sequence(b,u,1,target.slot,255,state_.script_fields.value("battle_tick",0),-1,data_,tactics_);
   int cx=state_.camera_x,cy=state_.camera_y;
   for(auto event:selection.events){auto kind=event.at("kind");
    if(kind=="camera_request"||kind=="focus_target"){
     int fx=kind=="camera_request"?b.x:target.x,fy=kind=="camera_request"?b.y:target.y;
     auto request=kind=="camera_request"?"camera_request":"camera_visible";
     steps.push_back({{"kind",request},{"x",fx},{"y",fy}});
     if(kind=="focus_target"){fx=std::clamp(cx/48,fx-12,fx)+6;fy=std::clamp(cy/48,fy-7,fy)+4;}
     for(auto p:battle_camera_path(cx,cy,fx,fy,width,height)){cx=p[0];cy=p[1];}
    }else if(kind=="wait")steps.push_back({{"kind","wait"},{"ticks",event.at("args").at(0)}});
    else {event["kind"]="selection_"+kind.get<std::string>();steps.push_back(event);}
   }
   std::vector<StrikeTarget> participants;
   for(const auto& other:roster)participants.push_back({other.slot==b.slot?b:other,record(other.id),state_.terrain.at(other.y*width+other.x)});
   auto attack=physical_attack(b.slot,target.slot,participants,state_.rng,{cx,cy},selection.tick,width,height,0,state_.inventory.value("87",0),data_);
   for(auto event:attack.events){auto kind=event.at("kind");
    if(kind=="camera"){
     int fx=event.at("tile").at(0).get<int>()+6,fy=event.at("tile").at(1).get<int>()+4;
     steps.push_back({{"kind","camera_request"},{"x",fx},{"y",fy}});
    }else if(kind=="message")continue; // Text is resolved by the following speech request.
    else {if(kind=="frame")event["screen_position"]=true;steps.push_back(event);}
   }
   for(const auto& other:attack.units)if(other.battle.slot==target.slot&&other.battle.hp==0&&equipment_effect(u,46,data_))throw std::runtime_error("pursuit attack completion is not yet verified (0x43ac63)");
   steps.push_back({{"kind","attack_done"},{"actor",b.slot},{"rng",attack.rng.state},{"tick",attack.tick},{"last_allied_actor",attack.last_allied_actor}});
   queue_battle_action(steps,wire,b.id);
   state_.pending_action["combat"]=true;
   state_.pending_action["ui"]={{"cursor",Json::array({255,255})},{"movement",false},{"distance",range.grids.distance},{"range",std::vector<int>(width*height)},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
   state_.battle_stage="ai_post_action";state_.presentation={{"type","physical_attack"},{"unit",b.id},{"target",target.id},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="ai_tactic_plan"){
   const auto& pending=state_.script_fields.at("ai_pending");
   const auto& a=state_.units.at(std::to_string(pending.at("unit").get<int>()));
   auto target=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.slot==pending.at("target");});
   if(target==roster.end())throw std::runtime_error("AI tactic target is absent");
   auto plan=plan_enemy_poison(BattleUnit::from_json(a.at("battle")),UnitRecord::from_json(a),*target,record(target->id),state_.rng,data_);
   if(!plan.cast)throw std::runtime_error("AI canceled tactic completion is not yet verified (0x4259ac)");
   // The original caption speed defaults to zero (10 ticks). Use that
   // duration as the deterministic clock input; real host rendering latency
   // is deliberately not allowed to affect simulation or saved state.
   auto cast=tactic_cast_animation(BattleUnit::from_json(a.at("battle")),UnitRecord::from_json(a),*target,33,{state_.camera_x,state_.camera_y},map_.at("width"),map_.at("height"),0,10,tactic_records_);
   Json steps=Json::array();
   for(auto event:cast.events){auto kind=event.at("kind");
    if(kind=="camera")steps.push_back({{"kind","camera_request"},{"x",event.at("tile").at(0).get<int>()+6},{"y",event.at("tile").at(1).get<int>()+4}});
    else if(kind=="cursor"||kind=="message_pump")continue;
    else if(kind=="clear_cursor")steps.push_back({{"kind","selection_clear_cursor"},{"args",Json::array()}});
    else if(kind=="caster_restore")steps.push_back({{"kind","unit_redraw"},{"slot",cast.actor.slot}});
    else{
     if(kind=="unit_redraw")steps.push_back({{"kind","unit_state"},{"battle",cast.actor.json()}});
     if(kind=="frame")event["screen_position"]=true;
     steps.push_back(event);
    }
   }
   queue_battle_action(steps,wire,cast.actor.id);state_.pending_action["combat"]=true;
   state_.pending_action["ui"]={{"cursor",{255,255}},{"movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
   state_.script_fields["tactic_plan"]=plan.json();state_.rng=plan.rng;
   state_.battle_stage="ai_tactic_effect";state_.presentation={{"type","tactic_cast"},{"unit",cast.actor.id},{"tactic",33},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="ai_tactic_effect"){
   const auto& plan=state_.script_fields.at("tactic_plan");
   const auto& target=plan.at("targets").at(0);
   auto it=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.slot==target;});
   if(it==roster.end())throw std::runtime_error("poison target disappeared");
   auto sequence=poison_effect_animation(*it,record(it->id),plan.at("damage").at(0),plan.at("effects").at(0),{state_.camera_x,state_.camera_y},map_.at("width"),map_.at("height"),poison_effect_);
   Json steps=Json::array();for(auto e:sequence){auto kind=e.at("kind");
    if(kind=="archive"||kind=="message_pump"||kind=="capture")continue;
    if(kind=="camera"){steps.push_back({{"kind","camera_request"},{"x",e.at("tile").at(0).get<int>()+6},{"y",e.at("tile").at(1).get<int>()+4}});continue;}
    if(kind=="frame")e["screen_position"]=true;
    steps.push_back(e);
   }
   queue_battle_action(steps,wire,it->id);state_.pending_action["combat"]=true;state_.pending_action["poison"]=true;
   state_.pending_action["ui"]={{"cursor",{255,255}},{"movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
   state_.battle_stage="ai_tactic_apply";state_.presentation={{"type","poison_effect"},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="ai_tactic_apply"){
   const auto& plan=state_.script_fields.at("tactic_plan");
   int id=state_.script_fields.at("ai_pending").at("unit");
   auto actor=BattleUnit::from_json(state_.units.at(std::to_string(id)).at("battle"));
   auto target=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.slot==plan.at("targets").at(0);});
   if(target==roster.end())throw std::runtime_error("poison application target disappeared");
   auto applied=apply_poison_target(actor,{*target,record(target->id),0},plan.at("damage").at(0),plan.at("effects").at(0),plan.at("armor_experience").at(0),0,state_.inventory.value("87",0),{state_.camera_x,state_.camera_y},state_.script_fields.value("battle_tick",0),map_.at("width"),map_.at("height"),data_);
   auto finalized=finalize_enemy_poison({actor,record(id),0},applied.target.battle,true,plan.at("experience"),plan.at("weapon_experience"),applied.camera,map_.at("width"),map_.at("height"),data_,tactics_);
   // Compile the entire result before committing any mutation. Additional
   // status/miss effects and item/level windows must never be silently skipped.
   Json steps=Json::array();
   auto append=[&](const Json& requests){for(size_t n=0;n<requests.size();++n){auto e=requests.at(n);auto kind=e.at("kind");
    if(kind=="effect_load"){
     int resource=e.at("resource");
     if(n+3>=requests.size()||requests.at(n+1)!=Json{{"kind","effect_target"},{"slot",target->slot}}||requests.at(n+2)!=Json{{"kind","effect_play"}}||requests.at(n+3)!=Json{{"kind","effect_clear"}})
      throw std::runtime_error("unverified secondary MEFF request order");
     auto status_target=*target;
     if(resource==49){status_target.status|=16;steps.push_back({{"kind","unit_state"},{"battle",status_target.json()}});}
     auto effect=secondary_effect_animation(status_target,applied.camera,secondary_effects_.at(std::to_string(resource)));
     for(auto frame:effect)if(frame.at("kind")!="archive"&&frame.at("kind")!="message_pump"&&frame.at("kind")!="capture")steps.push_back(frame);
     n+=3;continue;
    }
    if(kind=="automatic_item")throw std::runtime_error("automatic item sequence is not yet verified (0x41dc43)");
    if(kind=="equipment_level")throw std::runtime_error("equipment level presentation is not yet verified (0x4083ef)");
    if(kind=="refresh")continue; // Original external character-window notification.
    if(kind=="camera")steps.push_back({{"kind","camera_request"},{"x",e.at("tile").at(0).get<int>()+6},{"y",e.at("tile").at(1).get<int>()+4}});
    else if(kind=="redraw")steps.push_back({{"kind","selection_redraw"},{"tick",applied.tick},{"args",Json::array()}});
    else steps.push_back(e);
   }};
   append(applied.requests);
   steps.push_back({{"kind","unit_state"},{"battle",applied.target.battle.json()}});
   steps.push_back({{"kind","record_state"},{"record",applied.target.unit.json()}});
   append(finalized.requests);
   steps.push_back({{"kind","unit_state"},{"battle",finalized.actor.battle.json()}});
   steps.push_back({{"kind","record_state"},{"record",finalized.actor.unit.json()}});
   steps.push_back({{"kind","tactic_done"},{"actor",actor.slot}});
   queue_battle_action(steps,wire,id);state_.pending_action["combat"]=true;
   state_.pending_action["ui"]={{"cursor",{255,255}},{"movement",false},{"distance",std::vector<int>(state_.terrain.size(),255)},{"range",std::vector<int>(state_.terrain.size())},{"panel",nullptr},{"tick",state_.script_fields.value("battle_tick",0)}};
   state_.battle_stage="ai_tactic_post";state_.presentation={{"type","tactic_application"},{"duration_ms",0}};return;
  }
  if(state_.battle_stage=="ai_tactic_post"){
   // The completed tactic returns to the same caller as a physical attack.
   // Retire its transient plan before entering a saveable script dialogue.
   state_.script_fields.erase("tactic_plan");state_.battle_stage="ai_post_action";
  }
  if(state_.battle_stage=="ai_post_action"){
   int id=state_.script_fields.at("ai_pending").at("unit");auto b=BattleUnit::from_json(state_.units.at(std::to_string(id)).at("battle"));
   // 0x45b5fe notifies script storage block 3 before the first post-action
   // event batch. It checks outcome, not side/phase.
   state_.battle_stage="ai_deaths";
   if((b.flags&2)&&b.lifecycle==2&&!state_.battle_outcome){
    state_.script_fields["last_action"]={b.id,b.x,b.y};begin_battle_events("ai_deaths");return;
   }
  }
  if(state_.battle_stage=="ai_deaths"){
   // Original 0x44cc03 visits all slots in order, including the actor after
   // a counterattack. Do not silently remove a defeated unit: its handler
   // can dispatch story, defeat, inventory and persistent-record effects.
   for(const auto& b:roster)if(b.lifecycle==2&&b.hp==0){
    queue_battle_action(defeated_battle_unit(roster,record(b.id)),wire,b.id);
    state_.presentation={{"type","unit_defeated"},{"unit",b.id},{"slot",b.slot},{"duration_ms",0}};return;
   }
   if(state_.battle_outcome){stop(wire,"battle outcome transition is not yet verified");return;}
   int id=state_.script_fields.at("ai_pending").at("unit");auto b=BattleUnit::from_json(state_.units.at(std::to_string(id)).at("battle"));
   if(b.flags&2){
    queue_battle_action(Json::array({{{"kind","unit_redraw"},{"unit",b.id}}}),wire,b.id);
    state_.battle_stage="ai_after_redraw";state_.presentation={{"type","post_action_redraw"},{"unit",b.id}};return;
   }
   state_.battle_stage="ai_pick";state_.presentation={{"type","post_action_complete"},{"unit",id}};return;
  }
  if(state_.battle_stage=="ai_after_redraw"){begin_battle_events("ai_pick");return;}
  stop(wire,"unknown battle dispatch stage");
 }catch(const std::exception& e){stop(wire,e.what());}
}
void Engine::queue_battle_action(const Json& input,const Json& wire,int uid,int animation){
 Json steps=Json::array();int x=state_.camera_x,y=state_.camera_y;
 for(const auto& step:input){auto kind=step.at("kind");
  if(kind=="camera_request"||kind=="camera_visible"){
   int fx=step.at("x"),fy=step.at("y");
   if(kind=="camera_visible"){
    int cx=x/48,cy=y/48;
    cx=std::clamp(cx,fx-12,fx);cy=std::clamp(cy,fy-7,fy);
    fx=cx+6;fy=cy+4;
   }
   for(auto point:battle_camera_path(x,y,fx,fy,map_.at("width"),map_.at("height"))){x=point[0];y=point[1];steps.push_back({{"kind","camera"},{"x",x},{"y",y}});}
  }else steps.push_back(step);
 }
 if(!steps.empty())state_.pending_action={{"type","animation"},{"unit",uid},{"animation",animation},{"opcode",wire.at("opcode")},{"offset",wire.at("offset")},{"steps",steps},{"hidden",false}};
}
bool Engine::advance_animation(){
 auto& action=state_.pending_action;
 while(!action.at("steps").empty()){
  auto step=action.at("steps").front();action["steps"].erase(action["steps"].begin());
  auto kind=step.at("kind").get<std::string>();int uid=action.at("unit");
  auto by_slot=[&](int slot)->Json&{for(auto& u:state_.units)if(u.contains("battle")&&u.at("battle").at("slot")==slot)return u;throw std::runtime_error("presentation refers to absent battle slot");};
  if(kind=="mcall_preload"){state_.mcall_resource=step.at("resource");continue;}
  if(kind=="mcall_release"){state_.mcall_resource=-1;continue;}
  if(kind=="fire_terrain"){state_.terrain_effects.at(step.at("slot").get<int>())=step.at("record");state_.terrain.at(step.at("tile").get<int>())=26;continue;}
  if(kind=="fire_mark"){state_.terrain_marks.at(step.at("tile").get<int>())=step.at("effect");continue;}
  if(kind=="scripted_movement_done"){state_.presentation={{"type","scripted_movement_complete"},{"unit",uid},{"offset",action.at("offset")},{"duration_ms",0}};return true;}
  if(kind=="banner_column"){
   int col=step.at("args").at(0);action.at("banner").at("columns").at(col)=step.at("color");
   for(auto it=state_.battle_poses.begin();it!=state_.battle_poses.end();){auto b=BattleUnit::from_json(state_.units.at(it.key()).at("battle"));
    if(b.x==state_.camera_x/48+col)it=state_.battle_poses.erase(it);else ++it;}
   state_.presentation={{"type","banner_column"},{"column",col},{"color",step.at("color")},{"duration_ms",0}};return true;
  }
  if(kind=="banner_text"){action.at("banner").at("text").push_back(step);continue;}
  if(kind=="banner_cells"){
   const auto& a=step.at("args");int x=a.at(0),y=a.at(1),w=a.at(2),h=a.at(3);
   if(x<0||y<0||w<1||h<1||x+w>13||y+h>8)throw std::runtime_error("invalid banner redraw region");
   for(int row=y;row<y+h;++row)for(int col=x;col<x+w;++col)action.at("banner").at("cells").at(row*13+col)=step.at("color");
   for(auto it=state_.battle_poses.begin();it!=state_.battle_poses.end();){auto b=BattleUnit::from_json(state_.units.at(it.key()).at("battle"));int col=b.x-state_.camera_x/48,row=b.y-state_.camera_y/48;
    if(col>=x&&col<x+w&&row>=y&&row<y+h)it=state_.battle_poses.erase(it);else ++it;
   }
   state_.presentation={{"type","banner_cells"},{"args",a},{"color",step.at("color")},{"duration_ms",0}};return true;
  }
  if(kind=="banner_region"){
   const auto& a=step.at("args");if(a.at(0)!=0||a.at(2)!=battle_view_width/48)throw std::runtime_error("unverified objective redraw width");
   int row=a.at(1),height=a.at(3);for(int y=row;y<row+height;++y)action.at("banner").at("rows").at(y)=step.at("color");
   for(auto it=state_.battle_poses.begin();it!=state_.battle_poses.end();){auto b=BattleUnit::from_json(state_.units.at(it.key()).at("battle"));int y=b.y-state_.camera_y/48;
    if(y>=row&&y<row+height)it=state_.battle_poses.erase(it);else ++it;
   }
   state_.presentation={{"type","banner_region"},{"row",row},{"height",height},{"color",step.at("color")},{"duration_ms",0}};return true;
  }
  if(kind=="input_wait_tick"){
   state_.presentation={{"type","objective_pause"},{"duration_ms",step.at("ticks").get<int>()*50}};return true;
  }
  if(kind=="reset_action_flags"){
   for(auto& u:state_.units)if(u.contains("battle"))u["battle"]["flags"]=u.at("battle").at("flags").get<int>()&~6;
   continue;
  }
  if(kind.starts_with("selection_")){
   auto& ui=action.at("ui");auto args=step.value("args",Json::array());
   if(kind=="selection_cursor")ui["cursor"]=args;
   else if(kind=="selection_clear_cursor")ui["cursor"]=Json::array({255,255});
   else if(kind=="selection_movement_overlay")ui["movement"]=args.at(0).get<int>()!=0;
   else if(kind=="selection_range")ui["range"]=selection_range({args.at(0),args.at(1)},args.at(2),args.at(3).get<int>()!=0,map_.at("width"),map_.at("height"));
   else if(kind=="selection_clear_panel")ui["panel"]=nullptr;
   else if(kind=="selection_target"){
    BattleUnit b;const BattleUnit* target=nullptr;if(args.at(0)!=255){b=BattleUnit::from_json(by_slot(args.at(0)).at("battle"));target=&b;}
    ui["range"]=selection_target_mask(target,map_.at("width"),map_.at("height"));
   }
   else if(kind=="selection_panel"){
    const auto& u=step.contains("slot")?by_slot(step.at("slot")):state_.units.at(std::to_string(uid));auto b=BattleUnit::from_json(u.at("battle"));
    ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",ui.at("movement")},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};
    if(args.at(0)==0)ui["panel"]=battle_unit_panel(b,UnitRecord::from_json(u),u.at("name"),{state_.camera_x/48,state_.camera_y/48},state_.terrain.at(b.y*map_.at("width").get<int>()+b.x),data_,ui_);
    else if(args.at(0)==1){const auto& au=by_slot(args.at(1));auto a=BattleUnit::from_json(au.at("battle"));ui["panel"]=battle_attack_panel(b,UnitRecord::from_json(u),u.at("name"),a,UnitRecord::from_json(au),{state_.camera_x/48,state_.camera_y/48},state_.terrain.at(b.y*map_.at("width").get<int>()+b.x),state_.terrain.at(a.y*map_.at("width").get<int>()+a.x),data_,ui_);}
    else if(args.at(0)==2){const auto& au=by_slot(args.at(1));ui["panel"]=battle_tactic_panel(b,UnitRecord::from_json(u),u.at("name"),BattleUnit::from_json(au.at("battle")),UnitRecord::from_json(au),{state_.camera_x/48,state_.camera_y/48},args.at(2),data_,ui_,tactics_);}
    else throw std::runtime_error("unverified action preview panel");
   }else if(kind=="selection_redraw"){ui["tick"]=step.at("tick");state_.script_fields["battle_tick"]=step.at("tick");}
   else throw std::runtime_error("unverified selection request "+kind);
   continue;
  }
  if(kind=="clear_walking"){state_.battle_poses=Json::object();action["ui"]=Json::object();continue;}
  if(kind=="effect"){action["effect"]=step;continue;}
  if(kind=="effect_restore"){action.erase("effect");continue;}
  if(kind=="stop_sound"){
   state_.presentation={{"type","sound"},{"id",0},{"repeat",255},{"duration_ms",0}};return true;
  }
  if(kind=="caption_window"){
   const auto& a=step.at("args");if(a.at(4)!=0)throw std::runtime_error("unverified caption window style");
   action["caption"]=tactic_caption_window({a.at(0),a.at(1),a.at(2),a.at(3)});continue;
  }
  if(kind=="caption_text"){action.at("caption").push_back(step);continue;}
  if(kind=="caption_close"){action.erase("caption");continue;}
  if(kind=="caption_wait"){
   state_.presentation={{"type","tactic_caption"},{"unit",uid},{"duration_ms",step.at("args").at(0).get<int>()*50}};return true;
  }
  if(kind=="attack_tick"){state_.script_fields["battle_tick"]=step.at("tick");action["ui"]["tick"]=step.at("tick");continue;}
  if(kind=="record_state"){const auto& r=step.at("record");state_.units.at(std::to_string(r.at("id").get<int>())).update(r);continue;}
  if(kind=="speech"){
   int message=step.at("message").get<int>()-700;if(message<0||message>=static_cast<int>(combat_text_.size()))throw std::runtime_error("missing critical speech resource");
   const auto& u=state_.units.at(std::to_string(step.at("unit").get<int>()));
   std::string speaker;for(unsigned char c:u.at("name").get<std::string>())if(c<'0'||c>'9')speaker+=char(c);
   state_.presentation={{"type","dialogue"},{"speaker",speaker},{"face",u.at("face")},{"text",combat_text_.at(message).at("text")},{"combat_message",step.at("message")}};state_.stable=true;return true;
  }
  if(kind=="numbers"){
   if(step.at("hp")==0&&step.at("mp")==0&&step.at("xp")==0&&step.at("weapon_xp")==0&&step.at("armor_xp")==0)continue;
   if(step.at("xp")!=0||step.at("weapon_xp")!=0||step.at("armor_xp")!=0)throw std::runtime_error("experience result panel is not yet verified");
   auto& u=by_slot(step.at("target"));auto b=BattleUnit::from_json(step.at("target_before"));
   BattleUnit a;const BattleUnit* actor=nullptr;
   if(step.at("actor")!=255){a=BattleUnit::from_json(by_slot(step.at("actor")).at("battle"));actor=&a;}
   auto sequence=battle_damage_panel(b,UnitRecord::from_json(u),u.at("name"),actor,{state_.camera_x/48,state_.camera_y/48},static_cast<int32_t>(step.at("hp").get<uint32_t>()),static_cast<int32_t>(step.at("mp").get<uint32_t>()),step.at("wait"),data_,ui_);
   for(auto it=sequence.rbegin();it!=sequence.rend();++it){auto e=*it;if(e.at("kind")!="wait")e["kind"]="damage_"+e.at("kind").get<std::string>();action["steps"].insert(action["steps"].begin(),e);}continue;
  }
  if(kind.starts_with("damage_")){
   auto& ui=action["ui"];
   if(kind=="damage_cursor")ui["cursor"]=step.at("tile");
   else if(kind=="damage_clear_cursor")ui["cursor"]={255,255};
   else if(kind=="damage_clear_panel")ui["panel"]=nullptr;
   else {
    if(kind=="damage_window"){ui["panel"]=Json::array();ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",false},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};}
    step["kind"]=kind.substr(7);ui["panel"].push_back(step);
   }continue;
  }
  if(kind=="number"){action["number"]=step;continue;}
  if(kind=="tactic_done"){
   auto& u=by_slot(step.at("actor"));auto b=BattleUnit::from_json(u.at("battle"));b.flags|=6;u["battle"]=b.json();
   state_.battle_poses=Json::object();action["ui"]=Json::object();
   state_.presentation={{"type","tactic_complete"},{"unit",b.id},{"duration_ms",0}};return true;
  }
  if(kind=="attack_done"){
   auto& u=by_slot(step.at("actor"));auto b=BattleUnit::from_json(u.at("battle"));b.flags|=6;u["battle"]=b.json();
   state_.rng.state=step.at("rng");state_.script_fields["battle_tick"]=step.at("tick");state_.script_fields["last_allied_actor"]=step.at("last_allied_actor");
   state_.battle_poses=Json::object();action["ui"]=Json::object();action.erase("number");
   state_.presentation={{"type","attack_complete"},{"unit",b.id},{"duration_ms",0}};return true;
  }
  if(kind=="overlay"){
   if(action.value("combat",false)){
    const auto& u=by_slot(step.at("slot"));auto b=BattleUnit::from_json(u.at("battle"));auto r=UnitRecord::from_json(u);
    int frame=battle_frame(b,std::min(500,r.hp+equipment_bonus(r,5,data_)),state_.script_fields.value("battle_tick",0));
    int order=action.value("paint_order",0)+1;action["paint_order"]=order;
    state_.battle_poses[std::to_string(b.id)]={{"archive","UNIT_MOV.E5"},{"sprite",battle_sprite(b.id,r.job,b.side)},{"frame",frame},{"mirror",bool(b.flags&8)&&(frame==4||frame==5||frame==8)},{"width",48},{"height",48},{"screen_x",step.at("x")},{"screen_y",step.at("y")},{"animation",-1},{"order",order}};
    if(action.contains("number")&&action.at("number").at("slot")==b.slot)action.erase("number");continue;
   }
   auto key=std::to_string(uid);if(state_.battle_poses.contains(key)&&state_.battle_poses.at(key).contains("screen_x"))state_.battle_poses[key]["overlays"].push_back(step);continue;
  }
  if(kind=="camera"){
   state_.camera_x=step.at("x");state_.camera_y=step.at("y");state_.battle_poses=Json::object();
   state_.presentation={{"type","camera"},{"x",state_.camera_x},{"y",state_.camera_y},{"duration_ms",0}};return true;
  }
  if(kind=="redraw"){
   const auto& r=step.at("rectangle");int x=state_.camera_x/48+r.at(0).get<int>(),y=state_.camera_y/48+r.at(1).get<int>();
   for(auto it=state_.battle_poses.begin();it!=state_.battle_poses.end();){auto b=BattleUnit::from_json(state_.units.at(it.key()).at("battle"));
    if(b.x>=x&&b.x<x+r.at(2).get<int>()&&b.y>=y&&b.y<y+r.at(3).get<int>())it=state_.battle_poses.erase(it);else ++it;}
   continue;
  }
  if(kind=="unit_redraw"){int id=step.contains("unit")?step.at("unit").get<int>():by_slot(step.at("slot")).at("id").get<int>();state_.battle_poses.erase(std::to_string(id));if(action.value("poison",false))action.erase("number");continue;}
  if(kind=="minimap"){state_.presentation={{"type","minimap_refresh"},{"duration_ms",0}};return true;}
  if(kind=="tile"||kind=="unit_state"){
   auto b=BattleUnit::from_json(step.at("battle"));state_.units.at(std::to_string(b.id))["battle"]=b.json();
   if(kind=="tile"){state_.battle_poses.erase(std::to_string(b.id));action["hidden"]=!step.at("visible").get<bool>();}
   continue;
  }
  if(kind=="frame"){
   Json pose;for(auto key:{"archive","sprite","frame","mirror","width","height"})pose[key]=step.at(key);
   if(step.contains("mask_offset"))pose["mask_offset"]=step.at("mask_offset");
   if(step.value("screen_position",false)){pose["screen_x"]=step.at("x");pose["screen_y"]=step.at("y");pose["overlays"]=Json::array();}
   if(action.value("combat",false)){uid=by_slot(step.at("slot")).at("id");pose["effects"]=step.value("effects",Json::array());int order=action.value("paint_order",0)+1;action["paint_order"]=order;pose["order"]=order;}
   pose["animation"]=action.at("animation");state_.battle_poses[std::to_string(uid)]=std::move(pose);continue;
  }
  if(kind=="sound"){
   state_.script_fields["looping_sound"]=-1;
   state_.presentation={{"type","sound"},{"id",step.at("id")},{"repeat",1},{"duration_ms",0}};return true;
  }
  if(kind=="wait"){
   state_.presentation={{"type","animation_wait"},{"unit",uid},{"duration_ms",step.at("ticks").get<int>()*50},{"remaining",action.at("steps").size()}};return true;
  }
  throw std::runtime_error("invalid animation step "+kind);
 }
 action=nullptr;return false;
}
bool Engine::advance_tutorial(){
 auto& t=state_.tutorial;auto code=tutorial_program(t.at("completed"),t.at("all_treasures"));
 int pc=t.at("pc");if(pc<0||pc>=static_cast<int>(code.size()))throw std::runtime_error("invalid tutorial instruction position");
 const auto& op=code.at(pc);t["pc"]=pc+1;auto kind=op.at("op").get<std::string>();
 if(kind=="speech"){
  // Original speaker focus calls 454cef even without camera displacement;
  // that routine clears selection before checking its destination.
  t["ui"]["cursor"]={255,255};
  int id=op.at("unit");const auto& text=ui_.at("tutorial_text").at(std::to_string(op.at("text").get<int>())).at("text");
  dialogue("&"+data_.at("heroes").at(id).at("name").get<std::string>()+"\n"+text.get<std::string>());
  for(auto& page:state_.pending)page["tutorial_pc"]=pc;
  return false;
 }
 if(kind=="face"||kind=="pose"){
  Json args=Json::array();for(const auto& a:op.at("args"))args.push_back({{"value",a}});
  execute({{{"opcode",kind=="face"?79:80},{"offset",0x2f51},{"args",args}},-1});return state_.phase=="blocked";
 }
 if(kind=="move"){
  int id=op.at("unit");auto b=BattleUnit::from_json(state_.units.at(std::to_string(id)).at("battle"));Json args=Json::array();
  auto dest=op.value("to_cursor",false)?t.at("cursor").get<std::array<int,2>>():std::array<int,2>{b.x+op.at("delta").at(0).get<int>(),b.y+op.at("delta").at(1).get<int>()};
  for(int a:{0,id,0,dest[0],dest[1],-1})args.push_back({{"value",a}});
  execute({{{"opcode",50},{"offset",0x2f51},{"args",args}},-1});return state_.phase=="blocked";
 }
 if(kind=="remember_position"){auto b=BattleUnit::from_json(state_.units.at(std::to_string(op.at("unit").get<int>())).at("battle"));t["return_position"]={b.x,b.y};return false;}
 if(kind=="restore_cursor"){t["cursor"]=t.at("return_position");return false;}
 if(kind=="cancel_move"){
  auto& actor=state_.units.at(std::to_string(op.at("unit").get<int>())).at("battle");auto b=BattleUnit::from_json(actor);
  if(b.lifecycle!=2||t.at("return_position").is_null())throw std::runtime_error("unverified tutorial cancellation");
  b.x=t.at("return_position").at(0);b.y=t.at("return_position").at(1);actor=b.json();state_.battle_poses=Json::object();
  state_.presentation={{"type","redraw"},{"duration_ms",0}};return true;
 }
 if(kind=="sound"){
  state_.script_fields["looping_sound"]=-1;state_.presentation={{"type","sound"},{"id",op.at("id")},{"repeat",1},{"duration_ms",0}};return true;
 }
 if(kind=="show_range"){tutorial_show_range(t.at("ui"),state_,op.at("unit"),map_.at("width"),map_.at("height"),data_,op.value("blue",false));return false;}
 if(kind=="clear_range"){t["ui"]["movement"]=false;t["ui"]["blue_movement"]=false;t["ui"]["range"]=std::vector<int>(state_.terrain.size());return false;}
 if(kind=="redraw"){state_.battle_poses=Json::object();state_.presentation={{"type","redraw"},{"duration_ms",0}};return true;}
 if(kind=="cursor"){
  for(int n=0;n<2;++n)t["cursor"][n]=(t.at("cursor").at(n).get<int>()+op.at("delta").at(n).get<int>())&255;
  return false;
 }
 if(kind=="mark"){
  int x=t.at("cursor").at(0),y=t.at("cursor").at(1),w=map_.at("width"),h=map_.at("height");
  if(x<0||x>=w||y<0||y>=h)throw std::runtime_error("tutorial cursor outside map");
  int tile=y*w+x,value=op.at("value");
  if(value==3&&state_.terrain_marks.at(tile)!=255)throw std::runtime_error("unverified tutorial cursor collision with another effect");
  if(value==255&&(t.at("mark")!=tile||state_.terrain_marks.at(tile)!=3))throw std::runtime_error("tutorial cursor cleanup mismatch");
  state_.terrain_marks.at(tile)=value;t["mark"]=value==3?tile:-1;
  for(auto it=state_.battle_poses.begin();it!=state_.battle_poses.end();){auto b=BattleUnit::from_json(state_.units.at(it.key()).at("battle"));if(b.x==x&&b.y==y)it=state_.battle_poses.erase(it);else ++it;}
  state_.presentation={{"type","tutorial_mark"},{"tile",tile},{"value",value},{"duration_ms",0}};return true;
 }
 if(kind=="wait"){state_.presentation={{"type","tutorial_wait"},{"duration_ms",op.at("ticks").get<int>()*50}};return true;}
 if(kind=="terrain_panel"){
  auto point=t.at("cursor").get<std::array<int,2>>();int at=point[1]*map_.at("width").get<int>()+point[0];
  if(state_.terrain_marks.at(at)!=255)throw std::runtime_error("terrain preview with an active effect is not yet verified");
  for(const auto& u:state_.units)if(u.contains("battle")){auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle==2&&b.x==point[0]&&b.y==point[1])throw std::runtime_error("terrain preview with an occupying unit is not yet verified");}
  auto& ui=t.at("ui");ui["cursor"]=point;
  ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",ui.at("movement")},{"blue_movement",ui.at("blue_movement")},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};
  ui["panel"]=battle_terrain_panel(point,{state_.camera_x/48,state_.camera_y/48},state_.terrain.at(at),ui_);ui["panel_tile"]=point;return false;
 }
 if(kind=="panel"){
  auto& ui=t.at("ui");const auto& u=state_.units.at(std::to_string(op.at("unit").get<int>()));auto b=BattleUnit::from_json(u.at("battle"));
  ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",ui.at("movement")},{"blue_movement",ui.at("blue_movement")},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};
  ui["panel"]=battle_unit_panel(b,UnitRecord::from_json(u),u.at("name"),{state_.camera_x/48,state_.camera_y/48},state_.terrain.at(b.y*map_.at("width").get<int>()+b.x),data_,ui_);
  ui["panel_unit"]=op.at("unit");return false;
 }
 if(kind=="clear_panel"){t["ui"]["panel"]=nullptr;t["ui"].erase("panel_underlay");t["ui"].erase("panel_unit");t["ui"].erase("panel_tile");return false;}
 if(kind=="menu"){state_.presentation=tutorial_menu_presentation(t.at("cursor"),pc,ui_);state_.stable=true;return true;}
 if(kind=="choice"){
  state_.presentation={{"type","choice"},{"text",ui_.at("tutorial_text").at(std::to_string(op.at("text").get<int>())).at("text")},{"speaker_id",157},{"tutorial_pc",pc}};
  state_.stable=true;return true;
 }
 if(kind=="jump"){t["pc"]=op.at("target");return false;}
 if(kind=="unsupported"){
  std::ostringstream reason;reason<<"Ekd5.exe:0x"<<std::hex<<op.at("address").get<int>()<<": "<<op.at("reason").get<std::string>();throw std::runtime_error(reason.str());
 }
 if(kind=="end"){if(t.at("mark")!=-1||!t.at("ui").at("panel").is_null())throw std::runtime_error("tutorial ended without cleanup");state_.tutorial=nullptr;return false;}
 throw std::runtime_error("unsupported native tutorial operation");
}
void Engine::advance(){
 if(state_.phase=="blocked"||state_.stable)return;
 state_.presentation=Json::object();
 for(int budget=0;budget<4096;++budget){
  if(!state_.pending_action.is_null()){
   try{if(advance_animation())return;}catch(const std::exception& e){stop({{"offset",state_.pending_action.value("offset",0)},{"opcode",state_.pending_action.value("opcode",80)}},e.what());return;}
  }
  if(!state_.pending.empty()&&state_.pending.front().contains("focus_unit")){
   int id=state_.pending.front().at("focus_unit");auto b=BattleUnit::from_json(state_.units.at(std::to_string(id)).at("battle"));
   state_.pending.front().erase("focus_unit");
   queue_battle_action(Json::array({{{"kind","camera_request"},{"x",b.x},{"y",b.y}}}),{{"offset",programs_.at(state_.program).code.at(state_.pc-1).wire.at("offset")},{"opcode",20}},id);continue;
  }
  if(!state_.pending.empty()){state_.presentation=state_.pending.front();state_.pending.erase(state_.pending.begin());state_.stable=true;return;}
  if(!state_.tutorial.is_null()){
   try{if(advance_tutorial())return;}catch(const std::exception& e){stop({{"offset",0x2f51},{"opcode",16}},e.what());return;}
   continue;
  }
  if(state_.battle_stage!="intro"&&state_.battle_stage!="entry_events"){advance_battle_phase();return;}
  auto& p=programs_.at(state_.program);
  if(state_.pc>=static_cast<int>(p.code.size())){next_program();return;}
  auto ins=p.code.at(state_.pc++);
  try{if(!execute(ins))return;}catch(const std::exception& e){stop(ins.wire,e.what());return;}
 }
 stop({{"offset",0},{"opcode",-1}},"instruction budget exceeded");
}
void Engine::submit(const Command& c){
 if(c.type=="start"){start_scenario(static_cast<uint32_t>(c.value));return;}
 if(state_.phase=="blocked")throw std::runtime_error(state_.diagnostic);
 if(c.type=="continue"&&state_.presentation.value("type","")=="objective_pause"&&!state_.pending_action.is_null()){
  auto& steps=state_.pending_action.at("steps");while(!steps.empty()&&steps.front().at("kind")=="input_wait_tick")steps.erase(steps.begin());
  ++state_.commands;advance();return;
 }
 if(!state_.stable)throw std::runtime_error("command requires a stable interaction point");
 auto kind=state_.presentation.value("type","");
 if(c.type=="tutorial_menu_focus"&&kind=="tutorial_command_menu"){
  if(c.value!=2&&c.value!=0x508&&c.value!=0x50b)throw std::runtime_error("tutorial focus target is disabled or invalid");
  state_.presentation["focused_control"]=c.value;++state_.commands;return;
 }
 if(c.type=="tutorial_command"&&kind=="tutorial_command_menu"){
  if(c.value!=2&&c.value!=4&&c.value!=255)throw std::runtime_error("tutorial command is disabled or invalid");
  if(state_.tutorial.is_null()||state_.tutorial.at("pc")!=state_.presentation.at("tutorial_pc").get<int>()+1)throw std::runtime_error("tutorial menu continuation mismatch");
  state_.tutorial["menu_result"]=c.value;
 }
 else if(c.type=="continue"&&kind=="dialogue"){}
 else if(c.type=="choose"&&kind=="choice"){
  auto s=state_.presentation.at("text").get<std::string>();int count=1+static_cast<int>(std::count(s.begin(),s.end(),'\n'));if(c.value<1||c.value>count)throw std::runtime_error("choice out of range");
  if(!state_.tutorial.is_null()){
   auto code=tutorial_program(state_.tutorial.at("completed"),state_.tutorial.at("all_treasures"));int pc=state_.presentation.at("tutorial_pc");
   if(code.at(pc).at("op")!="choice"||state_.tutorial.at("pc")!=pc+1)throw std::runtime_error("tutorial choice continuation mismatch");
   state_.tutorial["pc"]=code.at(pc).at(c.value==1?"accept":"decline");
   state_.tutorial["answers"].push_back(c.value);
  }else state_.choice=c.value;
 }else throw std::runtime_error("unsupported command at current interaction point");
 ++state_.commands;state_.stable=false;advance();
}
Json Engine::state_json()const{
 const auto& s=state_;return {{"script",s.script},{"phase",s.phase},{"background",s.background},{"diagnostic",s.diagnostic},{"program",s.program},{"pc",s.pc},{"turn",s.turn},{"turn_limit",s.turn_limit},{"choice",s.choice},{"ambition",s.ambition},{"camera_x",s.camera_x},{"camera_y",s.camera_y},{"music",s.music},{"selected",s.selected},{"mcall_resource",s.mcall_resource},{"environment_region",s.environment_region},{"environment_selector",s.environment_selector},{"battle_phase",s.battle_phase},{"battle_outcome",s.battle_outcome},{"event_scans",s.event_scans},{"battle_stage",s.battle_stage},{"stable",s.stable},{"previous_condition",s.previous_condition},{"victory",s.victory},{"defeat",s.defeat},{"commands",s.commands},{"rng",s.rng.state},{"variables",s.variables},{"collection",s.collection},{"tutorial",s.tutorial},{"units",s.units},{"inventory",s.inventory},{"terrain",s.terrain},{"terrain_effects",s.terrain_effects},{"terrain_marks",s.terrain_marks},{"pending",s.pending},{"presentation",s.presentation},{"triggered",s.triggered},{"pending_action",s.pending_action},{"controls",s.controls},{"script_fields",s.script_fields},{"battle_poses",s.battle_poses}};
}
std::string Engine::state_hash()const{return sha256(state_json().dump());}
std::string Engine::serialize()const{
 if(!state_.stable||state_.phase=="blocked"||!state_.pending_action.is_null())throw std::runtime_error("snapshot unavailable outside a completed stable interaction point");
 Json payload={{"schema",snapshot_schema},{"engine",engine_version},{"asset_manifest",manifest_},{"state",state_json()}};
 return Json{{"payload",payload},{"sha256",sha256(payload.dump())}}.dump();
}
void Engine::restore(const std::string& text){
 if(text.size()>4*1024*1024)throw std::runtime_error("snapshot too large");
 auto envelope=Json::parse(text);auto p=envelope.at("payload");
 if(!envelope.at("sha256").is_string()||envelope.at("sha256")!=sha256(p.dump()))throw std::runtime_error("snapshot checksum mismatch");
 if(p.at("schema")!=snapshot_schema||p.at("engine")!=engine_version||p.at("asset_manifest")!=manifest_)throw std::runtime_error("snapshot version or asset manifest mismatch");
 const auto& j=p.at("state");GameState s;
 #define LOAD(field) j.at(#field).get_to(s.field)
 LOAD(script);LOAD(phase);LOAD(background);LOAD(diagnostic);LOAD(program);LOAD(pc);LOAD(turn);LOAD(turn_limit);LOAD(choice);LOAD(ambition);LOAD(camera_x);LOAD(camera_y);LOAD(music);LOAD(selected);LOAD(mcall_resource);LOAD(environment_region);LOAD(environment_selector);LOAD(battle_phase);LOAD(battle_outcome);LOAD(event_scans);LOAD(battle_stage);LOAD(stable);LOAD(previous_condition);LOAD(victory);LOAD(defeat);LOAD(commands);LOAD(variables);LOAD(collection);LOAD(tutorial);LOAD(units);LOAD(inventory);LOAD(terrain);LOAD(terrain_effects);LOAD(terrain_marks);LOAD(pending);LOAD(presentation);LOAD(triggered);LOAD(pending_action);LOAD(controls);LOAD(script_fields);LOAD(battle_poses);
 #undef LOAD
 if(!j.at("rng").is_number_unsigned()&&(!j.at("rng").is_number_integer()||j.at("rng").get<int64_t>()<0))throw std::runtime_error("invalid RNG state");
 auto rng=j.at("rng").get<uint64_t>();if(rng>UINT32_MAX)throw std::runtime_error("invalid RNG range");s.rng.state=static_cast<uint32_t>(rng);
 if(s.program<0||s.program>=static_cast<int>(programs_.size())||s.script!=programs_[s.program].script||s.pc<0||s.pc>static_cast<int>(programs_[s.program].code.size())||s.variables.size()!=1024||s.terrain.size()!=map_.at("cells").size()||!s.units.is_object()||!s.inventory.is_object()||!s.pending.is_array()||!s.triggered.is_array()||!s.controls.is_object()||!s.script_fields.is_object()||!s.stable||s.phase=="blocked")throw std::runtime_error("invalid snapshot state");
 if(s.presentation.value("type","")!="dialogue"&&s.presentation.value("type","")!="choice"&&s.presentation.value("type","")!="tutorial_command_menu")throw std::runtime_error("invalid snapshot interaction point");
 auto page_valid=[&](const Json& page){
  if(!page.is_object()||!page.at("text").is_string()||page.at("text").get_ref<const std::string&>().size()>65536)throw std::runtime_error("invalid dialogue state");
  auto type=page.at("type").get<std::string>();
  if(type=="dialogue"){
   if(!page.at("speaker").is_string()||!page.at("face").is_number_integer())throw std::runtime_error("invalid speaker state");
   int face=page.at("face");if(face< -1||face>=228)throw std::runtime_error("invalid portrait reference");
   if(page.contains("focus_unit")){
    if(!page.at("focus_unit").is_number_integer())throw std::runtime_error("invalid dialogue focus type");
    auto id=std::to_string(page.at("focus_unit").get<int>());
    if(!s.units.contains(id)||!s.units.at(id).contains("battle")||s.units.at(id).at("battle").at("lifecycle")!=2||s.units.at(id).at("name")!=page.at("speaker"))throw std::runtime_error("invalid dialogue focus unit");
   }
  }else if(type!="choice")throw std::runtime_error("invalid pending interaction");
 };
 if(s.presentation.value("type","")!="tutorial_command_menu")page_valid(s.presentation);for(const auto& p:s.pending)page_valid(p);
 const bool battle=(s.phase=="battle_intro"||s.phase=="battle_event")&&s.script=="S_00"&&s.background=="battlefield";
 if(s.battle_phase<0||s.battle_phase>2||s.battle_outcome<0||s.battle_outcome>3||s.event_scans<0||s.event_scans>18||s.turn<1||s.turn>256)throw std::runtime_error("invalid battle scheduler state");
 if(!j.at("environment_region").is_number_integer()||!j.at("environment_selector").is_number_integer())throw std::runtime_error("invalid environment field type");
 battle_environment_class(s.environment_region,s.environment_selector);
 if(s.phase=="battle_event"){
  if(programs_.at(s.program).section!=1||s.battle_stage!="entry_events"||s.event_scans<1)throw std::runtime_error("invalid event continuation");
 }else if(s.battle_stage!="intro"||s.event_scans!=0)throw std::runtime_error("invalid intro scheduler state");
 if(!s.pending_action.is_null()||!s.battle_poses.is_object()||s.battle_poses.size()>115||(!battle&&!s.battle_poses.empty()))throw std::runtime_error("invalid stable presentation state");
 if(!battle&&(s.phase!="story"||s.script!="R_00"||s.background!="story"))throw std::runtime_error("snapshot scenario phase is not supported by this build");
 if(!j.at("mcall_resource").is_number_integer()||(s.mcall_resource!=-1&&s.mcall_resource!=105)||(!battle&&s.mcall_resource!=-1))throw std::runtime_error("invalid saved MCALL resource");
 int last_offset=s.pc>0?programs_.at(s.program).code.at(s.pc-1).wire.at("offset").get<int>():-1;
 bool summon_loaded=battle&&s.turn==2&&programs_.at(s.program).offset==0x20a1&&last_offset>=0x22c0&&last_offset<0x23f8;
 if((s.mcall_resource==105)!=summon_loaded)throw std::runtime_error("saved MCALL preload disagrees with script position");
 if(!j.at("collection").is_array()||j.at("collection").size()!=108)throw std::runtime_error("invalid collection state");
 for(const auto& b:j.at("collection"))if(!b.is_number_integer()||b<0||b>255)throw std::runtime_error("invalid collection byte");
 auto [completed,all_treasures]=tutorial_collection_status(s.collection,data_);
 Json normalized_marks=s.terrain_marks;
 if(!s.tutorial.is_null()){
  const auto& t=s.tutorial;
  if(!battle||last_offset!=0x2f51||!t.is_object()||!t.at("pc").is_number_integer()||!t.at("completed").is_number_integer()||!t.at("all_treasures").is_boolean()||t.at("completed")!=completed||t.at("all_treasures")!=all_treasures||!t.at("answers").is_array())throw std::runtime_error("invalid tutorial state");
  const auto code=tutorial_program(completed,all_treasures);int target=t.at("pc");
  if(target<1||target>static_cast<int>(code.size())||!s.presentation.contains("tutorial_pc")||s.presentation.at("tutorial_pc")!=target-1)throw std::runtime_error("invalid tutorial interaction position");
  const auto& interaction=code.at(target-1);auto kind=interaction.at("op");
  if((kind=="speech"?"dialogue":kind=="choice"?"choice":kind=="menu"?"tutorial_command_menu":"invalid")!=s.presentation.at("type"))throw std::runtime_error("invalid tutorial interaction kind");
  if(kind!="menu"&&s.presentation.at("text")!=ui_.at("tutorial_text").at(std::to_string(interaction.at("text").get<int>())).at("text"))throw std::runtime_error("invalid tutorial dialogue/choice text");
  // Every verified tutorial interaction is one page. An extra queued page
  // would execute before the tutorial continuation, even with a valid checksum.
  if(!s.pending.empty())throw std::runtime_error("unexpected queued tutorial dialogue");
  if(kind=="speech"){
   const auto& hero=data_.at("heroes").at(interaction.at("unit").get<int>());
   if(s.presentation.at("speaker")!=hero.at("name")||s.presentation.at("face")!=hero.at("face"))throw std::runtime_error("invalid tutorial speaker or portrait");
  }else if(kind=="menu"){if(s.presentation!=tutorial_menu_presentation(t.at("cursor"),target-1,ui_,s.presentation.at("focused_control").get<int>()))throw std::runtime_error("invalid tutorial command menu");}
  else if(s.presentation.at("speaker_id")!=157)throw std::runtime_error("invalid tutorial choice speaker");
  const auto& origin=t.at("origin");
  if(!origin.is_array()||origin.size()!=2||!origin.at(0).is_number_integer()||!origin.at(1).is_number_integer()||origin.at(0)<0||origin.at(0)>=map_.at("width")||origin.at(1)<0||origin.at(1)>=map_.at("height"))throw std::runtime_error("invalid tutorial origin");
  GameState replay=s;const auto& origins=t.at("unit_origins");
  if(!origins.is_object()||origins.size()!=2||origins.at("157")!=origin)throw std::runtime_error("invalid tutorial unit origins");
  for(const auto& key:{"0","157"}){const auto& point=origins.at(key);if(!point.is_array()||point.size()!=2||!point.at(0).is_number_integer()||!point.at(1).is_number_integer()||point.at(0)<0||point.at(0)>=map_.at("width")||point.at(1)<0||point.at(1)>=map_.at("height"))throw std::runtime_error("invalid tutorial unit origin");replay.units[key]["battle"]["x"]=point.at(0);replay.units[key]["battle"]["y"]=point.at(1);}
  auto cursor=origin.get<std::array<int,2>>();Json return_position=nullptr,panel_tile=nullptr;bool menu_completed=false;int mark=-1,panel=-1,at=0;size_t answer=0;
  Json ui={{"cursor",{255,255}},{"movement",false},{"blue_movement",false},{"distance",std::vector<int>(s.terrain.size(),255)},{"range",std::vector<int>(s.terrain.size())},{"panel",nullptr},{"tick",s.script_fields.value("battle_tick",0)}};
  for(int budget=0;at!=target&&budget<1024;++budget){
   const auto& step=code.at(at++);auto op=step.at("op");
   if(op=="cursor")for(int n=0;n<2;++n)cursor[n]=(cursor[n]+step.at("delta").at(n).get<int>())&255;
   else if(op=="move"){
    int id=step.at("unit");auto& row=replay.units.at(std::to_string(id));auto b=BattleUnit::from_json(row.at("battle"));
    auto dest=step.value("to_cursor",false)?cursor:std::array<int,2>{b.x+step.at("delta").at(0).get<int>(),b.y+step.at("delta").at(1).get<int>()};
    std::vector<BattleUnit> roster;std::vector<UnitRecord> records;for(const auto& u:replay.units)if(u.contains("battle")){roster.push_back(BattleUnit::from_json(u.at("battle")));records.push_back(UnitRecord::from_json(u));}
    auto move=scripted_battle_movement(b,UnitRecord::from_json(row),dest,-1,{s.camera_x/48,s.camera_y/48},1,map_.at("width"),map_.at("height"),s.terrain,roster,records,data_,tactics_);
    row["battle"]=move.actor.json();if(!move.distance.empty())ui["distance"]=move.distance;ui["cursor"]={255,255};
   }
   else if(op=="speech")ui["cursor"]={255,255};
   else if(op=="menu"&&at!=target)menu_completed=true;
   else if(op=="remember_position"){const auto& b=replay.units.at(std::to_string(step.at("unit").get<int>())).at("battle");return_position={b.at("x"),b.at("y")};}
   else if(op=="restore_cursor")cursor=return_position.get<std::array<int,2>>();
   else if(op=="cancel_move"){auto& b=replay.units.at(std::to_string(step.at("unit").get<int>())).at("battle");b["x"]=return_position.at(0);b["y"]=return_position.at(1);}
   else if(op=="show_range")tutorial_show_range(ui,replay,step.at("unit"),map_.at("width"),map_.at("height"),data_,step.value("blue",false));
   else if(op=="clear_range"){ui["movement"]=false;ui["blue_movement"]=false;ui["range"]=std::vector<int>(s.terrain.size());}
   else if(op=="mark")mark=step.at("value")==3?cursor[1]*map_.at("width").get<int>()+cursor[0]:-1;
   else if(op=="terrain_panel"){panel_tile=cursor;ui["cursor"]=cursor;}
   else if(op=="panel")panel=step.at("unit");
   else if(op=="clear_panel"){panel=-1;panel_tile=nullptr;}
   else if(op=="jump")at=step.at("target");
   else if(op=="choice"&&at!=target){
    if(answer>=t.at("answers").size()||!t.at("answers").at(answer).is_number_integer())throw std::runtime_error("missing tutorial choice");
    int value=t.at("answers").at(answer++);if(value!=1&&value!=2)throw std::runtime_error("invalid tutorial choice");at=step.at(value==1?"accept":"decline");
   }else if(op=="unsupported"||op=="end")throw std::runtime_error("saved tutorial crossed an execution boundary");
  }
  if(at!=target||answer!=t.at("answers").size()||t.at("cursor")!=Json(cursor)||t.at("mark")!=mark)throw std::runtime_error("inconsistent tutorial cursor or choices");
  if(menu_completed){if(!t.at("menu_result").is_number_integer()||(t.at("menu_result")!=2&&t.at("menu_result")!=4&&t.at("menu_result")!=255))throw std::runtime_error("invalid tutorial menu result");}
  else if(!t.at("menu_result").is_null())throw std::runtime_error("premature tutorial menu result");
  if(t.at("return_position")!=return_position)throw std::runtime_error("inconsistent tutorial return position");
  for(const auto& key:{"0","157"})for(const auto& coordinate:{"x","y"})if(replay.units.at(key).at("battle").at(coordinate)!=s.units.at(key).at("battle").at(coordinate))throw std::runtime_error("inconsistent tutorial movement");
  if(mark!=-1){if(mark<0||mark>=static_cast<int>(normalized_marks.size())||normalized_marks.at(mark)!=3)throw std::runtime_error("invalid tutorial mark");normalized_marks.at(mark)=255;}
  if(panel!=-1){const auto& u=s.units.at(std::to_string(panel));auto b=BattleUnit::from_json(u.at("battle"));
   ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",ui.at("movement")},{"blue_movement",ui.at("blue_movement")},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};
   ui["panel"]=battle_unit_panel(b,UnitRecord::from_json(u),u.at("name"),{s.camera_x/48,s.camera_y/48},s.terrain.at(b.y*map_.at("width").get<int>()+b.x),data_,ui_);ui["panel_unit"]=panel;
  }
  if(!panel_tile.is_null()){
   auto point=panel_tile.get<std::array<int,2>>();int tile=point[1]*map_.at("width").get<int>()+point[0];
   if(normalized_marks.at(tile)!=255)throw std::runtime_error("invalid tutorial terrain preview effect");
   for(const auto& u:s.units)if(u.contains("battle")){auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle==2&&b.x==point[0]&&b.y==point[1])throw std::runtime_error("invalid tutorial terrain preview occupant");}
   ui["panel_underlay"]={{"cursor",ui.at("cursor")},{"movement",ui.at("movement")},{"blue_movement",ui.at("blue_movement")},{"distance",ui.at("distance")},{"range",ui.at("range")},{"tick",ui.at("tick")}};
   ui["panel_underlay"]["cursor"]=point; // Captured before later speaker-focus clearing.
   ui["panel"]=battle_terrain_panel(point,{s.camera_x/48,s.camera_y/48},s.terrain.at(tile),ui_);ui["panel_tile"]=panel_tile;
  }
  if(t.at("ui")!=ui)throw std::runtime_error("inconsistent tutorial panel state");
 }else if(s.presentation.contains("tutorial_pc")||std::any_of(s.pending.begin(),s.pending.end(),[](const auto& p){return p.contains("tutorial_pc");}))throw std::runtime_error("tutorial interaction has no continuation");
 validate_terrain_effects(s.terrain_effects,normalized_marks,s.terrain,map_.at("cells"),map_.at("width"),map_.at("height"),battle);
 std::set<int> slots,positions;
 if(battle&&(s.camera_x<0||s.camera_x>336||s.camera_y<0||s.camera_y>576||s.camera_x%48||s.camera_y%48))throw std::runtime_error("invalid battlefield viewport");
 for(auto it=s.units.begin();it!=s.units.end();++it){const auto& u=it.value();int id=u.at("id");
  UnitRecord::from_json(u).validate();
  if(id<0||id>=static_cast<int>(data_.at("heroes").size())||it.key()!=std::to_string(id)||!u.at("x").is_number_integer()||!u.at("y").is_number_integer()||!u.at("visible").is_boolean())throw std::runtime_error("invalid unit state");
  const auto& original=data_.at("heroes").at(id);
  for(auto key:{"name","face","story_sprite"})if(u.at(key)!=original.at(key))throw std::runtime_error("invalid unit asset reference");
  int x=u.at("x"),y=u.at("y"),direction=u.at("direction"),action=u.at("action");if(x<0||x>100||y<0||y>100||direction<0||direction>3||action<0||action>17)throw std::runtime_error("invalid story unit position/action");
  if(u.contains("battle")){
   if(!battle)throw std::runtime_error("battlefield unit in story snapshot");
   auto b=BattleUnit::from_json(u.at("battle"));b.validate(UnitRecord::from_json(u),data_,map_.at("width"),map_.at("height"));
   if(!slots.insert(b.slot).second||(b.lifecycle==2&&!positions.insert(b.y*map_.at("width").get<int>()+b.x).second))throw std::runtime_error("duplicate battle slot or occupied tile");
  }
 }
 if(battle){
  if(!s.script_fields.contains("deployment")||!s.script_fields.at("deployment").is_array()||slots.size()!=s.script_fields.at("deployment").size())throw std::runtime_error("incomplete battle deployment snapshot");
  for(const auto& row:s.script_fields.at("deployment")){auto d=DeploymentSlot::from_json(row);auto key=std::to_string(d.id);if(!s.units.contains(key)||!s.units.at(key).contains("battle")||s.units.at(key).at("battle").at("slot")!=d.slot)throw std::runtime_error("deployment does not match live battle state");}
 }
 if(s.script_fields.contains("battle_tick")){int tick=s.script_fields.at("battle_tick");if(tick<0||tick>7)throw std::runtime_error("invalid battle animation tick");}
 if(s.script_fields.contains("ai_order")||s.script_fields.contains("ai_cursor")||s.script_fields.contains("ai_pending")||s.script_fields.contains("event_return")){
  const auto& f=s.script_fields;const auto& order=f.at("ai_order");
  if(!f.at("ai_cursor").is_number_integer()||f.at("ai_cursor")<1||f.at("ai_cursor")>115)throw std::runtime_error("invalid saved AI cursor");
  int cursor=f.at("ai_cursor");
  if(!battle||(s.battle_phase!=1&&s.battle_phase!=2)||!order.is_array()||order.size()>115||cursor<1||cursor>static_cast<int>(order.size()))throw std::runtime_error("invalid saved AI order");
  const int first_slot=s.battle_phase==1?0:35,last_slot=s.battle_phase==1?35:115;
  std::set<int> seen;for(const auto& row:order){if(!row.is_number_integer())throw std::runtime_error("invalid AI slot");int slot=row;if(slot<first_slot||slot>=last_slot||!slots.contains(slot)||!seen.insert(slot).second)throw std::runtime_error("invalid AI slot order");}
  const auto& a=f.at("ai_pending");for(auto key:{"unit","slot","target","tactic"})if(!a.at(key).is_number_integer()||a.at(key)<0||a.at(key)>511)throw std::runtime_error("invalid saved AI action value");
  int id=a.at("unit"),slot=a.at("slot"),target=a.at("target"),tactic=a.at("tactic");
  if(!s.units.contains(std::to_string(id))||s.units.at(std::to_string(id)).at("battle").at("slot")!=slot||order.at(cursor-1)!=slot||(target!=255&&!slots.contains(target))||(tactic!=255&&(s.battle_phase!=2||tactic!=33)))throw std::runtime_error("invalid saved AI action");
  if(tactic!=255&&(target==255||(s.units.at(std::to_string(id)).at("battle").at("flags").get<int>()&2)==0||f.contains("tactic_plan")))throw std::runtime_error("incomplete tactic in stable snapshot");
  const auto& destination=a.at("destination");if(!destination.is_array()||destination.size()!=2||!destination.at(0).is_number_integer()||!destination.at(1).is_number_integer()||destination.at(0)<0||destination.at(0)>=map_.at("width")||destination.at(1)<0||destination.at(1)>=map_.at("height"))throw std::runtime_error("invalid saved AI destination");
  auto resume=f.at("event_return").get<std::string>();if(s.phase!="battle_event"||(resume!="ai_deaths"&&resume!="ai_pick"))throw std::runtime_error("invalid saved event return");
 }
 if(s.script_fields.contains("last_action")){
  const auto& a=s.script_fields.at("last_action");if(!battle||!a.is_array()||a.size()!=3)throw std::runtime_error("invalid action notification");
  for(const auto& x:a)if(!x.is_number_integer())throw std::runtime_error("invalid action notification value");
  if(!s.units.contains(std::to_string(a.at(0).get<int>()))||a.at(1)<0||a.at(1)>=map_.at("width")||a.at(2)<0||a.at(2)>=map_.at("height"))throw std::runtime_error("invalid action notification coordinates");
 }
 if(s.script_fields.contains("looping_sound")){int sound=s.script_fields.at("looping_sound");if(sound!=-1)battle_sound_file(sound);}
 for(auto it=s.battle_poses.begin();it!=s.battle_poses.end();++it){
  if(!s.units.contains(it.key())||!s.units.at(it.key()).contains("battle"))throw std::runtime_error("pose has no battle unit");
  const auto& u=s.units.at(it.key());auto b=BattleUnit::from_json(u.at("battle"));const auto& pose=it.value();int animation=pose.at("animation");
  if(b.lifecycle!=2||(animation!=1&&animation!=2&&animation!=3&&animation!=4&&animation!=5&&animation!=6&&animation!=8&&animation!=9))throw std::runtime_error("invalid retained pose");
  auto steps=scripted_battle_animation(b,battle_sprite(b.id,UnitRecord::from_json(u).job,b.side),animation,false,false,0,0);bool match=false;
  for(const auto& step:steps)if(step.at("kind")=="frame"){
   Json valid;for(auto key:{"archive","sprite","frame","mirror","width","height"})valid[key]=step.at(key);valid["animation"]=animation;if(pose==valid)match=true;
  }
  if(!match)throw std::runtime_error("invalid retained sprite reference");
 }
 state_=std::move(s); // All parsing/validation above is transactional.
}
}
