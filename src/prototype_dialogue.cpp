#include "caocao/prototype.hpp"
#include "caocao/battle_events.hpp"
#include <algorithm>
#include <climits>
#include <functional>
#include <set>
#include <sstream>
namespace caocao {
void Prototype::load_dialogues(const Json& script){
 dialogue_catalog_=Json::object();
 std::map<int,Json> instructions;
 std::map<int,int> duel_speakers;std::array<int,2> duel{-1,-1};
 std::function<void(const Json&)> walk=[&](const Json& items){for(const auto& i:items){instructions[i.at("offset")]=i;
  if(i.at("opcode")==104)duel={i.at("args").at(0).at("value"),i.at("args").at(1).at("value")};
  if(i.at("opcode")==96||i.at("opcode")==99){int side=i.at("args").at(0).at("value");if(side<0||side>1||duel[1-side]<0)throw std::runtime_error("duel speech has no original speaker");duel_speakers[i.at("offset")]=duel[1-side];}
  if(i.contains("body"))walk(i.at("body"));}};
 for(const auto& section:script.at("sections"))for(const auto& event:section.at("events"))walk(event.at("body"));
 auto add=[&](int id,int event,std::vector<int> offsets){
  Json entry={{"event_offset",event},{"pages",Json::array()}};
  for(int offset:offsets){
   const auto& i=instructions.at(offset);int op=i.at("opcode");if(op!=20&&op!=22&&op!=96&&op!=99)throw std::runtime_error("unexpected original dialogue opcode at "+std::to_string(offset));
   std::string raw;if(op==96||op==99)raw="&"+data_.at("heroes").at(duel_speakers.at(offset)).at("name").get<std::string>()+"\n"+i.at("args").at(1).at("value").get<std::string>();else raw=i.at("args").at(0).at("value");size_t pos=0;
   while(pos<raw.size()){
    std::string speaker;size_t begin=pos;
    if(raw[pos]=='&'){auto end=raw.find('\n',pos);if(end==std::string::npos)throw std::runtime_error("original dialogue speaker lacks body");speaker=raw.substr(pos+1,end-pos-1);begin=end+1;}
    auto end=raw.find('&',begin);if(end==std::string::npos)end=raw.size();auto text=raw.substr(begin,end-begin);
    while(!text.empty()&&text.back()=='\n')text.pop_back();
    int unit=-1,face=-1;for(size_t n=0;n<data_.at("heroes").size();++n)if(data_.at("heroes").at(n).at("name")==speaker){unit=int(n);face=data_.at("heroes").at(n).at("face");break;}
    if(!speaker.empty()&&unit<0)throw std::runtime_error("unknown original dialogue speaker: "+speaker);
    std::istringstream input(text);std::vector<std::string> lines;std::string line;while(std::getline(input,line))lines.push_back(line);
    if(lines.empty())throw std::runtime_error("empty original dialogue");
    for(size_t n=0;n<lines.size();n+=3){std::string page;for(size_t k=n;k<std::min(n+3,lines.size());++k){if(k!=n)page+='\n';page+=lines[k];}
     entry["pages"].push_back({{"speaker",speaker},{"unit",unit},{"face",face},{"text",page},{"source_offset",offset}});
    }pos=end;
   }
  }
  dialogue_catalog_[std::to_string(id)]=std::move(entry);
 };
 if(revised_rules_){
  if(scenario_){add(1000,0,config().at("intro").get<std::vector<int>>());dialogue_catalog_["1000"]["trigger"]="intro";}
  for(const auto& event:script.at("sections").at(1).at("events")){
   std::function<void(const Json&)> add_pages=[&](const Json& body){for(const auto& i:body){int op=i.at("opcode"),offset=i.at("offset");if(op==20||op==22||op==96||op==99){add(1000000+offset,event.at("offset"),{offset});dialogue_catalog_[std::to_string(1000000+offset)]["trigger"]="script";}if(i.contains("body"))add_pages(i.at("body"));}};
   add_pages(event.at("body"));
  }return;
 }
 if(scenario_){
  auto entry=[&](int id,int offset,const Json& offsets,const std::string& kind){if(offsets.empty())return;add(id,offset,offsets.get<std::vector<int>>());dialogue_catalog_[std::to_string(id)]["trigger"]=kind;};
  entry(1000,0,config().at("intro"),"intro");entry(1001,0,config().at("victory"),"victory");entry(1002,0,config().at("defeat"),"defeat");
  std::set<int> ids;for(const auto& r:config().at("roster"))ids.insert(r.at("id").get<int>());
  for(const auto& e:config().at("deaths"))if(ids.contains(e.at("unit"))){int id=e.at("event_offset");entry(id,id,e.at("offsets"),"death");dialogue_catalog_[std::to_string(id)]["unit"]=e.at("unit");}
  for(const auto& e:config().at("encounters"))if(ids.contains(e.at("units").at(0))&&ids.contains(e.at("units").at(1))){int id=e.at("event_offset");entry(id,id,e.at("offsets"),"encounter");dialogue_catalog_[std::to_string(id)]["units"]=e.at("units");}
  return;
 }
 add(40,0x30c9,{0x3122});add(41,0x3235,{0x328e});
 add(50,0x37fd,{0x3828});add(51,0x389b,{0x38c6});
 add(52,0x3929,{0x3954});add(53,0x39d0,{0x39fb});add(54,0x3a5a,{0x3a85});
 add(100,0x333d,{0x33e5,0x35bd,0x3652,0x36f3});add(101,0x333d,{0x3740});
 add(1,0x3c26,{0x3c3f});
 // Keep the exact source conditions beside the extracted pages, rather than
 // inventing a proximity rule. The existing x86 fixtures verify opcode46.
 for(const auto& event:script.at("sections").at(1).at("events"))for(auto& entry:dialogue_catalog_)
  if(entry.at("event_offset")==event.at("offset"))entry["conditions"]=event.at("conditions");
}
Json Prototype::dialogue()const{
 if(!has_dialogue())return nullptr;
 int id=dialogue_.at("pending").at(0),page=dialogue_.at("page");const auto& entry=dialogue_catalog_.at(std::to_string(id));
 auto result=entry.at("pages").at(page);result.update({{"event",id},{"event_offset",entry.at("event_offset")},{"page",page},{"pages",entry.at("pages").size()}});return result;
}
void Prototype::continue_dialogue(){
 if(!has_dialogue())throw std::runtime_error("当前没有对话。");
 int id=dialogue_.at("pending").at(0),page=dialogue_.at("page").get<int>()+1;
 if(page==int(dialogue_catalog_.at(std::to_string(id)).at("pages").size())){dialogue_["pending"].erase(0);page=0;}
 dialogue_["page"]=page;
 if(revised_rules_&&!has_dialogue()){outcome();queue_dialogues();}
}
void Prototype::queue_dialogues(){
 if(revised_rules_){if(scenario_)queue_scenario_dialogues();else process_events();return;}
 if(scenario_){queue_scenario_dialogues();return;}
 if(!conversations_||has_dialogue())return;
 auto seen=[&](int id){const auto& ids=dialogue_.at("triggered");return std::find(ids.begin(),ids.end(),id)!=ids.end();};
 auto enqueue=[&](int id){if(!seen(id)){dialogue_["triggered"].push_back(id);dialogue_["pending"].push_back(id);}};
 if(state_.at("phase")=="defeat"){enqueue(1);return;}
 BattleEventState state;state.turn=state_.at("turn");state.phase=state_.at("phase")=="enemy"?1:0;state.units=roster();
 for(int id:dialogue_.at("triggered"))if(id<1024)state.variables.at(id)=1;
 // The original selector retains the last eligible event. Queue the supported
 // speeches in descending script order; reserve each flag before presentation.
 for(int id:{54,53,52,51,50,41,40}){
  bool eligible=!seen(id);for(const auto& condition:dialogue_catalog_.at(std::to_string(id)).at("conditions"))eligible=opening_battle_condition(condition,state)&&eligible;
  if(eligible)enqueue(id);
 }
 if(state_.at("phase")=="victory"&&!seen(100)&&!seen(101)){
  bool companions=true;for(int id:{32,3,33})if(state_["units"][std::to_string(id)]["battle"]["hp"]==0)companions=false;
  enqueue(companions?100:101);
 }
}
void Prototype::migrate_dialogue(){
 if(!conversations_)return;
 // Old saves have no conversation history. Do not narrate already-recorded
 // casualties or a completed outcome on load; future encounters remain eligible.
 for(auto [flag,id]:{std::pair{50,146},{51,147},{52,32},{53,3},{54,33}})
  if(state_.at("units").at(std::to_string(id)).at("battle").at("hp")==0)dialogue_["triggered"].push_back(flag);
 if(state_.at("phase")=="victory")dialogue_["triggered"].push_back(100);
 if(state_.at("phase")=="defeat")dialogue_["triggered"].push_back(1);
}
void Prototype::validate_dialogue()const{
 if(!conversations_){if(!dialogue_.is_null())throw std::runtime_error("unexpected dialogue snapshot");return;}
 if(!dialogue_.is_object()||dialogue_.size()!=3||!dialogue_.at("triggered").is_array()||!dialogue_.at("pending").is_array()||!dialogue_.at("page").is_number_integer())throw std::runtime_error("存档对话状态无效。");
 std::set<int> triggered,pending;
 for(const auto& value:dialogue_.at("triggered")){
  if(!value.is_number_integer()||!dialogue_catalog_.contains(value.dump())||!triggered.insert(value.get<int>()).second)throw std::runtime_error("存档对话标记无效。");
 }
 if(triggered.contains(100)&&triggered.contains(101))throw std::runtime_error("存档对话分支无效。");
 int previous=INT_MAX;
 BattleEventState state;state.turn=state_.at("turn");state.phase=state_.at("phase")=="enemy"?1:0;state.units=roster();
 for(const auto& value:dialogue_.at("pending")){
  if(!value.is_number_integer()||!triggered.contains(value.get<int>())||!pending.insert(value.get<int>()).second)throw std::runtime_error("存档对话状态无效。");
  int offset=dialogue_catalog_.at(value.dump()).at("event_offset");if(offset>previous)throw std::runtime_error("存档对话顺序无效。");previous=offset;
  int id=value;
  if(revised_rules_){
   if(id==1000){if(!scenario_)throw std::runtime_error("opening has no scenario intro");}
   else if(dialogue_catalog_.at(value.dump()).value("trigger","")!="script"||progress_.at("active").is_null()||progress_.at("active").at("event")!=offset)throw std::runtime_error("script dialogue has no matching continuation");
   continue;
  }
  if(scenario_){if(!scenario_dialogue_eligible(id))throw std::runtime_error("存档对话条件无效。");continue;}
  if((id==100||id==101)&&state_.at("phase")!="victory")throw std::runtime_error("存档胜利对话无效。");
  if(id==1&&state_.at("phase")!="defeat")throw std::runtime_error("存档战败对话无效。");
  if(id>=40&&id<=54){
   for(const auto& condition:dialogue_catalog_.at(value.dump()).at("conditions"))
    if(!opening_battle_condition(condition,state))throw std::runtime_error("存档对话条件无效。");
  }
  if(id==100||id==101){
   bool companions=true;for(int unit:{32,3,33})if(state_.at("units").at(std::to_string(unit)).at("battle").at("hp")==0)companions=false;
   if(companions!=(id==100))throw std::runtime_error("存档对话分支无效。");
  }
 }
 int page=dialogue_.at("page");int count=pending.empty()?1:int(dialogue_catalog_.at(std::to_string(dialogue_.at("pending").at(0).get<int>())).at("pages").size());
 if(page<0||page>=count)throw std::runtime_error("存档对话页码无效。");
}
}
