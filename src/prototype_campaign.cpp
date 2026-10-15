#include "caocao/prototype.hpp"
#include <set>

namespace caocao {
namespace {
Json party_records(const Json& state){
 Json result=Json::object();for(const auto& u:state.at("units"))if(u.at("battle").at("side")==0)
  result[std::to_string(u.at("record").at("id").get<int>())]=u.at("record");
 return result;
}
}
// This is a bounded native-prototype continuation ledger. It does not execute
// the intervening Rxx campaign script or claim its recruitment/cash/equip UI.
// Each prior battle is verified against edition data and the previous verified
// party, before it may become the next battle's initialization baseline.
void Prototype::validate_campaign()const{
 if(campaign_.is_object()&&campaign_.empty())return;
 if(!campaign_.is_object()||campaign_.size()!=3||!campaign_.at("schema").is_number_integer()||campaign_.at("schema")!=1||
    !campaign_.at("origin").is_number_integer()||!campaign_.at("battles").is_array())throw std::runtime_error("invalid campaign ledger");
 int origin=campaign_.at("origin");const auto& history=campaign_.at("battles");
 if(origin<0||origin>=scenario_count()-1||history.empty()||history.size()>=size_t(scenario_count())||origin+int(history.size())!=scenario_)
  throw std::runtime_error("invalid campaign sequence");
 Prototype baseline=*this;baseline.hulao_duel_=false;baseline.campaign_=Json::object();
 for(size_t n=0;n<history.size();++n){
  const auto& entry=history.at(n);int scenario=origin+int(n);
  if(!entry.is_object()||entry.size()!=(entry.contains("equipment_bag")?6:5)||!entry.at("scenario").is_number_integer()||entry.at("scenario")!=scenario||!entry.at("party").is_object())
   throw std::runtime_error("invalid completed campaign battle");
  baseline.equipment_enabled_=entry.contains("equipment_bag");baseline.initialize(1,false,scenario);
  auto expected=party_records(baseline.initial_);
  if(entry.at("party").size()!=expected.size())throw std::runtime_error("invalid carried party roster");
  for(auto it=expected.begin();it!=expected.end();++it)baseline.validate_revised_record(entry.at("party").at(it.key()),it.value(),true);
  baseline.progress_=entry.at("progress");baseline.validate_events();
  if(!baseline.progress_.at("victory").get<bool>()||!baseline.progress_.at("active").is_null()||!baseline.progress_.at("choice").is_null())
   throw std::runtime_error("campaign battle has not completed its rewards");
  if(baseline.equipment_enabled_){for(auto it=entry.at("party").begin();it!=entry.at("party").end();++it)baseline.state_["units"][it.key()]["record"]=it.value();baseline.equipment_bag_=entry.at("equipment_bag");baseline.validate_equipment();}
  baseline.benefits_=entry.at("benefits");baseline.consumed_items_=entry.at("consumed_items");baseline.validate_benefits();
  if(baseline.campaign_.empty())baseline.campaign_={{"schema",1},{"origin",origin},{"battles",Json::array()}};
  baseline.campaign_["battles"].push_back(entry);
 }
}
void Prototype::advance_campaign(){
 if(hulao_duel_)throw std::runtime_error("Duel is separate from the campaign");
 if(!revised_rules_||state_.at("phase")!="victory"||has_dialogue()||!story_choices().empty()||!progress_.at("active").is_null())
  throw std::runtime_error("请完成本战的胜利剧情和奖励，再进入下一战。");
 if(scenario_+1>=scenario_count())throw std::runtime_error("已完成当前原型的全部战役。");
 validate_campaign();validate_events();validate_benefits();validate_equipment();
 Prototype next=*this;
 if(next.campaign_.empty())next.campaign_={{"schema",1},{"origin",scenario_},{"battles",Json::array()}};
 next.campaign_["battles"].push_back({{"scenario",scenario_},{"party",party_records(state_)},{"benefits",benefits_},
  {"consumed_items",consumed_items_},{"progress",progress_}});
 if(equipment_enabled_)next.campaign_["battles"].back()["equipment_bag"]=equipment_bag_;
 ++next.scenario_;next.validate_campaign();
 next.initialize(state_.at("rng"),false,next.scenario_);
 *this=std::move(next);
}
Json Prototype::treasures()const{
 Json result=Json::object();if(!revised_rules_)return result;
 auto add=[&](const Json& items){for(auto it=items.begin();it!=items.end();++it)result[it.key()]=result.value(it.key(),0)+it.value().get<int>();};
 if(!campaign_.empty())for(const auto& battle:campaign_.at("battles"))add(battle.at("progress").at("treasures"));
 add(progress_.at("treasures"));return result;
}
}
