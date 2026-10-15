#include "caocao/prototype.hpp"
#include <algorithm>
#include <map>
namespace caocao {
namespace {
int equipment_slot(int type){return type<14?0:type<18?1:type<63?2:-1;}
}
void Prototype::validate_equipment_triple(const Json& e,int slot)const{
 if(!e.is_array()||e.size()!=3)throw std::runtime_error("invalid item instance");
 for(const auto& v:e)if(!v.is_number_integer()||v<0||v>255)throw std::runtime_error("invalid item byte");
 int id=e[0];if(id==255){if(slot<0||e!=Json::array({255,255,255}))throw std::runtime_error("invalid empty equipment");return;}
 if(id>=int(data_.at("items").size()))throw std::runtime_error("invalid equipment ID");
 int type=data_.at("items").at(id).at("type"),kind=equipment_slot(type);
 if(kind<0||(slot>=0&&kind!=slot))throw std::runtime_error("wrong equipment slot");
 if(kind==2){if(e[1]!=255||e[2]!=255)throw std::runtime_error("invalid accessory experience");}
 else {int cap=type%2?9:3;if(e[1]<1||e[1]>cap||(e[1]==cap?e[2]!=255:e[2]>99))throw std::runtime_error("invalid equipment experience");}
}
void Prototype::reward_equipment(int item){
 if(!equipment_enabled_)return;
 int kind=equipment_slot(data_.at("items").at(item).at("type"));if(kind<0)return;
 equipment_bag_.push_back(kind==2?Json::array({item,255,255}):Json::array({item,1,0}));
}
void Prototype::initialize_equipment(){
 equipment_bag_=Json::array();initial_equipment_bag_=Json::array();if(!equipment_enabled_)return;
 if(!campaign_.empty()){
  const auto& previous=campaign_.at("battles").back();
  if(previous.contains("equipment_bag"))equipment_bag_=previous.at("equipment_bag");
  else for(const auto& battle:campaign_.at("battles"))for(auto it=battle.at("progress").at("treasures").begin();it!=battle.at("progress").at("treasures").end();++it)for(int n=0;n<it.value().get<int>();++n)reward_equipment(std::stoi(it.key()));
  // Retain possessions of a member who is absent from the next deployment.
  for(auto it=previous.at("party").begin();it!=previous.at("party").end();++it)
   if(!initial_.at("units").contains(it.key())||initial_.at("units").at(it.key()).at("battle").at("side")!=0)
    for(const auto& e:it.value().at("equipment"))if(e[0]!=255)equipment_bag_.push_back(e);
 }
 initial_equipment_bag_=equipment_bag_;
}
void Prototype::enable_equipment(){
 new_equipment_=true;if(equipment_enabled_||!revised_rules_)return;
 Prototype candidate=*this;candidate.equipment_enabled_=true;candidate.initialize_equipment();
 for(auto it=candidate.progress_.at("treasures").begin();it!=candidate.progress_.at("treasures").end();++it)
  for(int n=0;n<it.value().get<int>();++n)candidate.reward_equipment(std::stoi(it.key()));
 candidate.validate_equipment();*this=std::move(candidate);
}
void Prototype::validate_equipment()const{
 if(!equipment_enabled_)return;
 if(!equipment_bag_.is_array()||equipment_bag_.size()>512)throw std::runtime_error("invalid equipment bag");
 std::map<int,int> expected,actual;
 auto count=[&](const Json& e,auto& counts){if(e.at(0)!=255)++counts[e.at(0).template get<int>()];};
 for(const auto& e:initial_equipment_bag_)count(e,expected);
 for(const auto& u:initial_.at("units"))if(u.at("battle").at("side")==0)for(const auto& e:u.at("record").at("equipment"))count(e,expected);
 for(auto it=progress_.at("treasures").begin();it!=progress_.at("treasures").end();++it)if(equipment_slot(data_.at("items").at(std::stoi(it.key())).at("type"))>=0)expected[std::stoi(it.key())]+=it.value().get<int>();
 for(const auto& e:equipment_bag_){validate_equipment_triple(e);count(e,actual);}
 for(const auto& u:state_.at("units"))if(u.at("battle").at("side")==0)for(const auto& e:u.at("record").at("equipment"))count(e,actual);
 if(actual!=expected)throw std::runtime_error("equipment ownership does not match acquired items");
}
bool Prototype::equipment_compatible(int actor,int slot,int item)const{
 if(slot<0||slot>2||item<0||item>=int(data_.at("items").size())||!state_.at("units").contains(std::to_string(actor)))return false;
 auto r=UnitRecord::from_json(state_.at("units").at(std::to_string(actor)).at("record"));const auto& raw=data_.at("items").at(item);int type=raw.at("type");
 if(equipment_slot(type)!=slot)return false;
 if(slot<2)return data_.at("classes").at(r.job).at(9+type)!=0;
 int group=r.job<=38?r.job/3:r.job-26,restriction=raw.at("class_group");return restriction==255||restriction==group;
}
std::string Prototype::equipment_block_reason(int actor)const{
 if(!equipment_enabled_)return "旧版存档未开放装备更换。";
 if(!state_.at("units").contains(std::to_string(actor)))return "武将不存在。";
 auto b=BattleUnit::from_json(state_.at("units").at(std::to_string(actor)).at("battle"));
 if(b.side!=0)return "只能更换我军装备。";
 if(has_dialogue()||!story_choices().empty()||!progress_.at("active").is_null())return "请先完成剧情。";
 if(state_.at("phase")!="victory")return "战斗中不能更换装备，请在战后整备时更换。";

 return {};
}
void Prototype::change_equipment(int actor,int slot,int bag_index){
 auto reason=equipment_block_reason(actor);if(!reason.empty())throw std::runtime_error(reason);
 if(slot<0||slot>2||bag_index< -1||bag_index>=int(equipment_bag_.size()))throw std::runtime_error("无效的装备选择。");
 if(bag_index>=0&&!equipment_compatible(actor,slot,equipment_bag_.at(bag_index).at(0)))throw std::runtime_error("该兵种不能装备此物品。");
 Prototype candidate=*this;auto& u=candidate.state_["units"][std::to_string(actor)];auto r=UnitRecord::from_json(u.at("record"));auto old=r.equipment[slot];
 r.equipment[slot]=bag_index<0?std::array<int,3>{255,255,255}:equipment_bag_.at(bag_index).get<std::array<int,3>>();
 if(bag_index>=0)candidate.equipment_bag_.erase(candidate.equipment_bag_.begin()+bag_index);
 if(old[0]!=255)candidate.equipment_bag_.push_back(old);
 u["record"]=r.json();u["max_hp"]=std::min(500,r.hp+equipment_bonus(r,5,data_));
 u["battle"]["hp"]=std::min(u.at("battle").at("hp").get<int>(),u.at("max_hp").get<int>());
 u["battle"]["mp"]=std::min(u.at("battle").at("mp").get<int>(),std::min(250,r.mp+equipment_bonus(r,6,data_)));
 candidate.clear_enemy_choice();candidate.request_cache_key_.clear();candidate.request_cache_=nullptr;
 candidate.validate(candidate.state_);candidate.validate_equipment();*this=std::move(candidate);
}
}
