#pragma once
#include "caocao/attack_animation.hpp"
namespace caocao {
struct PhysicalAttack {
 std::vector<StrikeTarget> units;
 Json events=Json::array(),trace=Json::array();
 OriginalRng rng;
 std::array<int,2> camera{};
 int tick=0,depth=0,last_allied_actor=-1;
};
// 0x4064f1 single-target controller. All mutations are returned transactionally.
// Area effects, automatic items and level windows throw before any caller state
// is committed. Death processing belongs to the enclosing battle scheduler.
PhysicalAttack physical_attack(int actor_slot,int target_slot,
 const std::vector<StrikeTarget>& units,OriginalRng rng,std::array<int,2> camera,
 int tick,int width,int height,int priority_id,int beans,const Json& data);
}
