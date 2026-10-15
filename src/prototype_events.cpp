#include "caocao/prototype.hpp"
#include "caocao/prototype_events.hpp"
#include "caocao/battle_events.hpp"
#include "caocao/range.hpp"
#include <algorithm>
#include <functional>
#include <set>
#include <sstream>
namespace caocao {
namespace {
bool faction(int side,int group){return group==6||(group==4&&side<2)||(group==5&&side>=2)||side==group;}
bool rectangle(const BattleUnit& b,int x1,int y1,int x2,int y2){return b.x>=x1&&b.x<=x2&&b.y>=y1&&b.y<=y2;}
std::string where(int scene,int offset){std::ostringstream s;s<<"S_0"<<scene<<".eex @0x"<<std::hex<<offset;return s.str();}
bool listed(const Json& ids,int id){return std::find(ids.begin(),ids.end(),id)!=ids.end();}
// These are the battlefield events this prototype executes. Location37 events
// use the separately verified terrain pickup controller. Opening intro/tutorial
// execution remains a separate faithful-Engine feature, not this turn-two mode.
bool supported_event(int scene,const Json& event){
 if(scene==0){static const std::set<int> events{0x30c9,0x3235,0x333d,0x37fd,0x389b,0x3929,0x39d0,0x3a5a,0x3b5a,0x3c26};return events.contains(event.at("offset"));}
 for(const auto& i:event.at("conditions"))if(i.at("opcode")==37)return false;
 return true;
}
// Keep source instructions and offsets. Branch results are stored so an ELSE
// after a mutating body observes the original IF result, not reevaluated state.
Json program(const Json& event){
 Json out=Json::array();
 std::function<void(const Json&)> emit=[&](const Json& body){int previous=-1;for(const auto& i:body){
  if(i.at("opcode")==1){int index=int(out.size());out.push_back({{"instruction",i.at("test")},{"branch",true},{"previous",previous},{"skip",0}});emit(i.at("body"));out[index]["skip"]=out.size();previous=index;}
  else if(i.at("opcode")!=0)out.push_back({{"instruction",i},{"branch",false}});
 }};emit(event.at("body"));return out;
}
}
bool prototype_script_condition(const Json& i,const Json& state,const Json& flags,const Json& data,int width,int height,int choice){
 int op=i.at("opcode");auto v=[&](int n){return i.at("args").at(n).at("value").get<int>();};
 std::vector<BattleUnit> units;for(const auto& u:state.at("units"))units.push_back(BattleUnit::from_json(u.at("battle")));
 auto by_id=[&](int id)->const BattleUnit*{const BattleUnit* found=nullptr;for(const auto& b:units)if(b.id==id&&b.lifecycle&&(!found||b.slot<found->slot))found=&b;return found;};
 if(op==19)return choice==v(0);
 if(op==38){
  int id=v(0);if((id<0||id>=512)&&id!=1024&&id!=1025&&id!=1026)throw std::runtime_error("unsupported region selector");
  for(const auto& b:units)if(b.lifecycle==2&&b.hp>0&&rectangle(b,v(1)&255,v(2)&255,v(3)&255,v(4)&255))
   if(id==b.id||id==1024||(id==1025&&b.slot<35)||(id==1026&&b.slot>=35))return true;
  return false;
 }
 if(op==65){
  int group=v(0)==-1?6:v(0),count=0;if(group<0||group>6)throw std::runtime_error("unsupported count faction");
  for(const auto& b:units)if(b.lifecycle==2&&faction(b.side,group)&&(v(3)!=1||rectangle(b,v(4)&255,v(5)&255,v(6)&255,v(7)&255)))++count;
  return v(2)==0?uint32_t(count)>=uint32_t(v(1)):v(2)==1?uint32_t(count)<uint32_t(v(1)):count==v(1);
 }
 if(op==46){
  const auto* a=by_id(v(0));if(!a||a->lifecycle!=2||a->hp<=0)return false;
  auto adjacent=[&](const BattleUnit& b){return b.lifecycle==2&&b.hp>0&&std::max(std::abs(a->x-b.x),std::abs(a->y-b.y))==1;};
  if(v(1)>=1024&&v(1)<=1026){for(const auto& b:units)if(adjacent(b)&&(v(1)==1024||(v(1)==1025&&b.slot<35)||(v(1)==1026&&b.slot>=35)))return true;return false;}
  const auto* b=by_id(v(1));if(!b||!adjacent(*b))return false;if(v(2)!=0&&v(2)!=-1)return true;
  auto in_range=[&](const BattleUnit& from,const BattleUnit& target){
   auto r=UnitRecord::from_json(state.at("units").at(std::to_string(from.id)).at("record"));int group=r.job<=38?r.job/3:r.job-26;
   int kind=(group==2||group==4||group==5)?1:battle_attack_range(r,data);int n=width*height;
   auto result=battle_range(from,{from.x,from.y},kind,0,false,255,width,height,{std::vector<int>(n,255),std::vector<int>(n,255),std::vector<int>(n)},units);
   return std::find(result.targets.begin(),result.targets.end(),target.slot)!=result.targets.end();
  };return in_range(*a,*b)&&in_range(*b,*a);
 }
 BattleEventState s;s.turn=state.at("turn");std::string phase=state.at("phase");s.phase=phase=="enemy"?2:phase=="ally"?1:0;s.units=units;
 for(auto it=flags.begin();it!=flags.end();++it){int id=std::stoi(it.key());if(id<0||id>=int(s.variables.size()))throw std::runtime_error("invalid script flag");s.variables.at(id)=it.value().get<int>();}
 if(op==67&&state.value("turn_limit_reached",false))return true;
 return opening_battle_condition(i,s);
}
void Prototype::initialize_events(){
 if(!revised_rules_)return;
 progress_={{"flags",Json::object()},{"awarded",Json::array()},{"treasures",Json::object()},{"choice",nullptr},{"active",nullptr},{"completed",Json::array()},{"victory",false},{"reward_descriptors",Json::array()}};
 if(hulao_duel_)return;
 if(scenario_){for(const auto& row:config().at("roster")){
  auto& b=state_["units"][std::to_string(row.at("id").get<int>())]["battle"];
  if(row.at("hidden_original").get<bool>())b["lifecycle"]=1;
  if(b.at("side")!=0){int mode=row.at("ai_original");if(mode<0||mode>6)throw std::runtime_error("unsupported deployment AI");b["ai"]=std::array<int,7>{3,1,2,0,0,4,4}[mode];}
 }}else for(auto& u:state_["units"])if(u.at("battle").at("side")==1)u["battle"]["ai"]=1;
 // S_03.eex @0x195f: Dong Zhuo and his two escorts leave before the
 // objective screen. The native intro uses extracted dialogue pages, so its
 // verified terminal deployment must be installed here as well.
 if(scenario_==3)for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle==2&&b.side==2&&rectangle(b,27,9,27,11)){b.lifecycle=3;b.status=0;b.flags&=~(128|8);u["battle"]=b.json();}}
}
Json Prototype::story_choices()const{return revised_rules_?progress_.value("choice",Json(nullptr)):Json(nullptr);}
void Prototype::choose_story(int choice){
 if(!revised_rules_||progress_.at("choice").is_null())throw std::runtime_error("当前没有剧情选项。");
 const auto& c=progress_.at("choice");if(choice<0||choice>=int(c.at("options").size()))throw std::runtime_error("剧情选项无效。");
 progress_["active"]["choice"]=choice+1;progress_["choice"]=nullptr;outcome();queue_dialogues();
}
void Prototype::process_events(){
 if(hulao_duel_)return;
 if(!revised_rules_||has_dialogue()||!progress_.at("choice").is_null()||!state_.at("undo").is_null())return;
 const auto& script=scenario_?scenario_scripts_.at(std::to_string(scenario_)):opening_script_;
 auto event_at=[&](int offset)->const Json&{for(const auto& e:script.at("sections").at(1).at("events"))if(e.at("offset")==offset&&supported_event(scenario_,e))return e;throw std::runtime_error("unknown event "+where(scenario_,offset));};
 auto dead=[&](){return state_.at("units").at("0").at("battle").at("hp")==0||state_.value("turn_limit_reached",false);};
 auto emit_speech=[&](int offset){int id=1000000+offset;if(!dialogue_catalog_.contains(std::to_string(id)))throw std::runtime_error("missing event speech "+where(scenario_,offset));
  if(conversations_){dialogue_["triggered"].push_back(id);dialogue_["pending"].push_back(id);dialogue_["page"]=0;}
 };
 auto grant=[&](int offset,int item){
  if(item<0)return;if(item>=int(data_.at("items").size()))throw std::runtime_error("invalid reward "+where(scenario_,offset));
  if(listed(progress_.at("awarded"),offset))return;progress_["awarded"].push_back(offset);
  auto key=std::to_string(item);progress_["treasures"][key]=progress_.at("treasures").value(key,0)+1;reward_equipment(item);
  state_["message"]="获得 "+data_.at("items").at(item).at("name").get<std::string>()+" ×1";
  environment_event(0,{{"kind","notice"},{"actor",0},{"text",state_.at("message")},{"item",item}});environment_event(0,{{"kind","sound"},{"id",14}});environment_event(0,{{"kind","wait"},{"ticks",25}});environment_event(0,{{"kind","notice_clear"}});
 };
 for(int budget=0;budget<2000;++budget){
  if(progress_.at("active").is_null()){
   if(progress_.at("victory").get<bool>())return;
   int selected=-1;for(const auto& e:script.at("sections").at(1).at("events")){
    int offset=e.at("offset");if(!supported_event(scenario_,e)||listed(progress_.at("completed"),offset))continue;
    bool defeat=false;for(const auto& c:e.at("conditions"))defeat=defeat||c.at("opcode")==67;
    if(dead()!=defeat)continue;
    bool eligible=true;for(const auto& c:e.at("conditions"))try{eligible=prototype_script_condition(c,state_,progress_.at("flags"),data_,map_width(),map_height())&&eligible;}catch(const std::exception& ex){throw std::runtime_error(where(scenario_,c.at("offset"))+": "+ex.what());}
    if(eligible)selected=offset;
   }
   if(selected<0)return;
   progress_["active"]={{"event",selected},{"pc",0},{"choice",0},{"branches",Json::object()},{"duel",Json::array()}};
  }
  auto& active=progress_["active"];auto instructions=program(event_at(active.at("event")));int pc=active.at("pc");
  if(pc==int(instructions.size())){progress_["completed"].push_back(active.at("event"));progress_["active"]=nullptr;continue;}
  if(pc<0||pc>=int(instructions.size()))throw std::runtime_error("invalid event cursor");
  const auto& line=instructions.at(pc);const auto& i=line.at("instruction");int op=i.at("opcode"),offset=i.at("offset");
  auto v=[&](int n){return i.at("args").at(n).at("value").get<int>();};
  auto error=[&](const std::string& why){throw std::runtime_error(where(scenario_,offset)+": "+why);};
  if(line.at("branch")){
   bool pass=false;if(op==3){int previous=line.at("previous");if(previous<0||!active.at("branches").contains(std::to_string(previous)))error("ELSE without evaluated IF");pass=!active.at("branches").at(std::to_string(previous)).get<bool>();}
   else try{pass=prototype_script_condition(i,state_,progress_.at("flags"),data_,map_width(),map_height(),active.at("choice"));}catch(const std::exception& ex){error(ex.what());}
   active["branches"][std::to_string(pc)]=pass;active["pc"]=pass?pc+1:line.at("skip").get<int>();continue;
  }
  // Commit the immutable source cursor before pausing; a saved dialogue/choice
  // resumes after its instruction, never awards or reveals a second time.
  active["pc"]=pc+1;
  if(op==2||op==0)continue;
  // 0x4143ae returns dispatcher status7: leave this scenario immediately,
  // including when the exit is nested inside a conditional victory body.
  if(op==13){progress_["completed"].push_back(active.at("event"));progress_["active"]=nullptr;return;}
  if(op==11){if(v(0)<0||v(0)>=1024)error("flag outside original table");progress_["flags"][std::to_string(v(0))]=v(1);continue;}
  if(op==18){std::string text=i.at("args").at(0).at("value");Json options=Json::array();std::istringstream in(text);std::string option;while(std::getline(in,option))options.push_back(option);if(options.empty())error("empty choice");progress_["choice"]={{"offset",offset},{"options",options}};return;}
  if(op==20||op==22||op==96||op==99){emit_speech(offset);if(has_dialogue())return;continue;}
  if(op==61){if(v(1)!=-1||v(2)!=1)error("unsupported reward quantity");grant(offset,v(0));continue;}
  if(op==89){
   // Original requested treasure identifiers are retained exactly. Cash award
   // arithmetic is not yet verified; no invented money is introduced.
   if(!listed(progress_.at("awarded"),offset)){Json items=Json::array();for(size_t n=1;n<i.at("args").size();++n){int item=v(int(n));if(item>=0){items.push_back(item);auto key=std::to_string(item);progress_["treasures"][key]=progress_.at("treasures").value(key,0)+1;reward_equipment(item);}}
    progress_["awarded"].push_back(offset);progress_["reward_descriptors"].push_back({{"offset",offset},{"arguments",i.at("args")},{"cash_verified",false}});
    if(!items.empty()){state_["message"]="获得 "+data_.at("items").at(items.at(0).get<int>()).at("name").get<std::string>();}
   }progress_["victory"]=true;continue;
  }
  if(op==76){if(v(0)!=0||v(2)!=0)error("unsupported reinforcement form");auto key=std::to_string(v(1));if(!state_.at("units").contains(key))continue;auto& b=state_["units"][key]["battle"];if(b.at("lifecycle")!=1)continue;
   int x=b.at("x"),y=b.at("y");std::vector<int> occupancy(cell_count(),255);for(const auto& u:roster())if(u.lifecycle==2)occupancy.at(u.y*map_width()+u.x)=u.slot;
   auto r=UnitRecord::from_json(state_.at("units").at(key).at("record"));auto pos=battle_placement(x,y,r.job<=38?r.job/3:r.job-26,map_width(),map_height(),terrain_.at("cells"),occupancy,data_);if(pos[0]==255)error("no legal reinforcement tile");
   b["x"]=pos[0];b["y"]=pos[1];b["lifecycle"]=2;state_["message"]=state_.at("units").at(key).at("name").get<std::string>()+"出场。";continue;
  }
  if(op==78){
   if(v(0)<-1||v(0)>1||v(7)<0||v(7)>3)error("unsupported AI assignment form");
   for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));bool selected=v(0)==1?b.lifecycle==2&&rectangle(b,v(2),v(3),v(4),v(5))&&faction(b.side,v(6)):b.id==v(1);
    if(!selected||!b.lifecycle||UnitRecord::from_json(u.at("record")).owner==0)continue;
    // Original 0x459d8b -> 0x45a751 -> 0x443140: mode3 assigns a
    // concrete live slot; an absent target leaves the old target unchanged.
    b.flags&=~128;b.ai=std::array<int,4>{3,1,2,0}[v(7)];if(v(7)==3){for(const auto& target:roster())if(target.id==v(8)&&target.lifecycle){b.target_slot=target.slot;break;}}u["battle"]=b.json();
   }continue;
  }
  if(op==83||op==49){
   if(v(0)!=0&&v(0)!=1)error("unsupported unit removal selector");
   for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));bool selected=v(0)==0?b.id==v(1):b.lifecycle==2&&rectangle(b,v(2),v(3),v(4),v(5))&&faction(b.side,v(6));
    if(selected&&b.lifecycle==2){b.lifecycle=3;b.status=0;b.flags&=~(128|8);u["battle"]=b.json();}
   }
   for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));if(b.ai!=0&&b.ai!=4)continue;bool target_alive=false;for(const auto& target:roster())if(target.slot==b.target_slot&&target.lifecycle==2)target_alive=true;if(!target_alive){b.target_slot=255;b.ai=1;u["battle"]=b.json();}}
   continue;
  }
  if(op==77){
   if(v(0)!=2||v(1)!=-1||v(2)!=0||v(7)!=5||v(8)!=-1||v(9)!=-1||v(10)!=8||v(11)!=0||v(12)!=0)error("unsupported area property form");
   for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));if(b.lifecycle==2&&b.side>=2&&rectangle(b,v(3),v(4),v(5),v(6)))u["battle"]["status"]=b.status|8;}continue;
  }
  if(op==50){
   // S_00 post-victory departures are presentation-only once the battle ends;
   // apply their original terminal coordinates, retaining exact source offset.
   if(scenario_||v(0)!=0||v(2)!=0)error("unsupported scripted walk");auto key=std::to_string(v(1));if(state_.at("units").contains(key)&&state_.at("units").at(key).at("battle").at("lifecycle")==2){auto& b=state_["units"][key]["battle"];b["x"]=v(3);b["y"]=v(4);}continue;
  }
  if(op==104){active["duel"]={v(0),v(1)};continue;}
  // Explicit native presentation adapters. The original duel choreography is
  // not claimed: its text uses the original portrait/speaker and opcode98's
  // outcome is followed by the actual battlefield83/61 instructions above.
  if(op==9||op==79||op==80||op==81||op==90||op==95||op==97||op==98||op==100||op==101||op==102||op==25||op==26||op==91||op==73||op==14)continue;
  error("unsupported opcode "+std::to_string(op));
 }
 throw std::runtime_error("scenario event instruction budget exceeded");
}
void Prototype::validate_events()const{
 if(!revised_rules_)return;
 if(hulao_duel_){
  bool won=state_.at("units").at(std::to_string(duel_enemy_)).at("battle").at("hp")==0&&!duel_friendlies_defeated(state_)&&!state_.at("turn_limit_reached").get<bool>();
  Json expected={{"flags",Json::object()},{"awarded",Json::array()},{"treasures",Json::object()},{"choice",nullptr},{"active",nullptr},{"completed",Json::array()},{"victory",won},{"reward_descriptors",Json::array()}};
  if(progress_!=expected)throw std::runtime_error("Invalid duel outcome/events");return;
 }
 const auto& p=progress_;if(!p.is_object()||p.size()!=8||!p.at("flags").is_object()||!p.at("awarded").is_array()||!p.at("treasures").is_object()||!p.at("completed").is_array()||!p.at("victory").is_boolean()||!p.at("reward_descriptors").is_array())throw std::runtime_error("invalid scenario event state");
 const auto& script=scenario_?scenario_scripts_.at(std::to_string(scenario_)):opening_script_;std::map<int,Json> events;std::map<int,Json> instructions;
 std::function<void(const Json&)> walk=[&](const Json& body){for(const auto& i:body){instructions[i.at("offset")]=i;if(i.contains("body"))walk(i.at("body"));}};
 for(const auto& e:script.at("sections").at(1).at("events"))if(supported_event(scenario_,e)){events[e.at("offset")]=e;walk(e.at("body"));}
 for(auto it=p.at("flags").begin();it!=p.at("flags").end();++it){int id=std::stoi(it.key());if(std::to_string(id)!=it.key()||id<0||id>=1024||!it.value().is_number_integer()||it.value()<0||it.value()>1)throw std::runtime_error("invalid scenario flag");}
 std::set<int> completed,awarded;Json treasures=Json::object(),descriptors=Json::array();bool victory=false;
 for(const auto& x:p.at("completed"))if(!x.is_number_integer()||!events.contains(x.get<int>())||!completed.insert(x.get<int>()).second)throw std::runtime_error("invalid completed scenario event");
 for(const auto& x:p.at("awarded")){
  if(!x.is_number_integer()||!instructions.contains(x.get<int>())||!awarded.insert(x.get<int>()).second)throw std::runtime_error("invalid scenario reward");
  const auto& i=instructions.at(x.get<int>());int op=i.at("opcode");if(op!=61&&op!=89)throw std::runtime_error("invalid reward instruction");
  if(op==89){victory=true;descriptors.push_back({{"offset",x},{"arguments",i.at("args")},{"cash_verified",false}});}
  size_t first=op==61?0:1,last=op==61?1:i.at("args").size();for(size_t n=first;n<last;++n){int item=i.at("args").at(n).at("value");if(item>=0){auto key=std::to_string(item);treasures[key]=treasures.value(key,0)+1;}}
 }
 if(treasures!=p.at("treasures"))throw std::runtime_error("scenario rewards disagree with inventory");
 if(descriptors!=p.at("reward_descriptors")||victory!=p.at("victory").get<bool>())throw std::runtime_error("scenario result disagrees with awarded instructions");
 if(p.at("active").is_null()){if(!p.at("choice").is_null())throw std::runtime_error("orphaned story choice");}
 else{
  const auto& a=p.at("active");if(!a.is_object()||a.size()!=5||!a.at("event").is_number_integer()||!events.contains(a.at("event").get<int>())||completed.contains(a.at("event"))||!a.at("pc").is_number_integer()||!a.at("choice").is_number_integer()||a.at("choice")<0||a.at("choice")>8||!a.at("branches").is_object()||!a.at("duel").is_array())throw std::runtime_error("invalid script continuation");
  auto code=program(events.at(a.at("event")));int pc=a.at("pc");if(pc<0||pc>int(code.size()))throw std::runtime_error("invalid script position");
  for(auto it=a.at("branches").begin();it!=a.at("branches").end();++it){int n=std::stoi(it.key());if(std::to_string(n)!=it.key()||n<0||n>=pc||!code.at(n).at("branch").get<bool>()||!it.value().is_boolean())throw std::runtime_error("invalid script branch");}
  if(!a.at("duel").empty()){
   bool valid=false;for(int n=0;n<pc;++n){const auto& i=code.at(n).at("instruction");if(i.at("opcode")==104&&a.at("duel")==Json::array({i.at("args").at(0).at("value"),i.at("args").at(1).at("value")}))valid=true;}if(!valid)throw std::runtime_error("invalid saved duel participants");
  }
  if(has_dialogue()){
   int id=dialogue_.at("pending").at(0);if(dialogue_.at("pending").size()!=1||pc==0||id!=1000000+code.at(pc-1).at("instruction").at("offset").get<int>()||!p.at("choice").is_null())throw std::runtime_error("saved speech disagrees with script position");
  }
  if(!p.at("choice").is_null()){
   const auto& c=p.at("choice");if(!c.is_object()||c.size()!=2||pc==0||code.at(pc-1).at("instruction").at("opcode")!=18||c.at("offset")!=code.at(pc-1).at("instruction").at("offset"))throw std::runtime_error("invalid saved choice");
   Json expected=Json::array();std::istringstream in(code.at(pc-1).at("instruction").at("args").at(0).at("value").get<std::string>());std::string row;while(std::getline(in,row))expected.push_back(row);if(c.at("options")!=expected)throw std::runtime_error("modified saved choices");
  }
 }
}
}
