#pragma once
#include "caocao/attack_animation.hpp"
namespace caocao {
// Presentation only: never owns or mutates simulation state/RNG. Original wait
// counts are retained; 40 ms/tick is a prototype pacing choice, not measured parity.
class PrototypePlayback {
public:
 static constexpr uint32_t tick_ms=40;
 void start(const Json& packet,const Json& following=nullptr,const Json& last=nullptr);
 void clear();
 void advance(uint32_t milliseconds);
 bool active()const{return active_;}
 bool moving()const{return active_&&packet_.value("kind",std::string("attack"))=="movement";}
 const Json& packet()const{return packet_;}
 const Json& before()const{return before_;}
 const Json& poses()const{return poses_;}
 const Json& number()const{return number_;}
 const Json& spell_effect()const{return spell_effect_;}
 const Json& notice()const{return notice_;}
 uint64_t generation()const{return generation_;}
 int tick()const{return tick_;}
 std::vector<int> take_sounds();
private:
 void next();
 void load(const Json& packet);
 bool active_=false;
 Json before_=nullptr,events_=Json::array(),poses_=Json::object(),number_=nullptr;
 Json packet_=Json::object(),following_=Json::array(),notice_=nullptr;
 uint64_t generation_=0;
 Json spell_effect_=nullptr;
 size_t cursor_=0;
 uint32_t remaining_=0;
 int tick_=0,order_=0;
 std::vector<int> sounds_;
};
}
