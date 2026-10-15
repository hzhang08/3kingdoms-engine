#include "caocao/prototype_playback.hpp"
#include <utility>
namespace caocao {
void PrototypePlayback::clear(){
 active_=false;before_=nullptr;events_=Json::array();poses_=Json::object();number_=nullptr;
 cursor_=0;remaining_=0;tick_=0;order_=0;sounds_.clear();
 packet_=Json::object();following_=Json::array();spell_effect_=nullptr;notice_=nullptr;
}
void PrototypePlayback::load(const Json& packet){
 packet_=packet;before_=packet.at("before");events_=packet.at("events");active_=true;
 poses_=Json::object();number_=nullptr;spell_effect_=nullptr;notice_=nullptr;cursor_=0;remaining_=0;tick_=0;order_=0;++generation_;
}
void PrototypePlayback::start(const Json& packet,const Json& following,const Json& last){
 clear();for(const auto* p:{&packet,&following,&last})if(!p->is_null())following_.push_back(*p);
 if(following_.empty())return;
 auto first=following_.at(0);following_.erase(following_.begin());load(first);next();
}
std::vector<int> PrototypePlayback::take_sounds(){return std::exchange(sounds_,{});}
void PrototypePlayback::next(){
 while(cursor_<events_.size()){
  auto e=events_.at(cursor_++);std::string kind=e.at("kind");
  if(kind=="wait"){int ticks=e.at("ticks");if(ticks<=0||ticks>100)throw std::runtime_error("invalid animation wait");remaining_=ticks*tick_ms;return;}
  if(kind=="attack_tick"){tick_=e.at("tick");continue;}
  if(kind=="sound"){sounds_.push_back(e.at("id"));continue;}
  if(kind=="notice"){notice_=e;continue;}
  if(kind=="notice_clear"){notice_=nullptr;number_=nullptr;continue;}
  if(kind=="record_state"){auto& u=before_["units"][std::to_string(e.at("actor").get<int>())];u["record"]=e.at("record");u["max_hp"]=e.at("max_hp");continue;}
  if(kind=="pose_clear"){poses_.erase(std::to_string(e.at("slot").get<int>()));continue;}
  if(kind=="unit_state"){before_["units"][std::to_string(e.at("actor").get<int>())]["battle"]=e.at("battle");continue;}
  if(kind=="number"){number_=e;continue;}
  if(kind=="spell_effect"){spell_effect_=e;continue;}
  if(kind=="spell_clear"){spell_effect_=nullptr;continue;}
  if(kind=="overlay"){
   // The original redraw restores the current idle pose and erases its number.
   bool found=false,dead=false;
   for(const auto& u:before_.at("units"))if(u.at("battle").at("slot")==e.at("slot")){
    auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0){poses_.erase(std::to_string(b.slot));dead=found=true;break;}auto r=UnitRecord::from_json(u.at("record"));
    int frame=battle_frame(b,u.at("max_hp"),tick_);
    e.update({{"kind","frame"},{"archive","UNIT_MOV.E5"},{"sprite",battle_sprite(b.id,r.job,b.side)},
      {"frame",frame},{"mirror",bool(b.flags&8)&&(frame==4||frame==5||frame==8)},
      {"width",48},{"height",48},{"effects",Json::array()}});found=true;break;
   }
   if(dead){number_=nullptr;continue;}
   if(!found)throw std::runtime_error("animation overlay unit absent");
   if(!number_.is_null()&&number_.at("slot")==e.at("slot"))number_=nullptr;
  }else if(kind!="frame")throw std::runtime_error("unsupported prototype animation event: "+kind);
  const auto slot=std::to_string(e.at("slot").get<int>());
  e["order"]=++order_;poses_[slot]=std::move(e);
 }
 if(!following_.empty()){auto following=following_.at(0);following_.erase(following_.begin());load(following);next();return;}
 active_=false;before_=nullptr;poses_=Json::object();number_=nullptr;spell_effect_=nullptr;notice_=nullptr;remaining_=0;
}
void PrototypePlayback::advance(uint32_t ms){
 while(active_&&ms>=remaining_){ms-=remaining_;next();}
 if(active_)remaining_-=ms;
}
}
