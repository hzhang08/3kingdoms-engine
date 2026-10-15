#pragma once
#include "caocao/strike.hpp"
namespace caocao {
struct PoisonApplication {
 StrikeTarget target;
 std::array<int,2> camera;
 int tick=0;
 Json requests=Json::array();
 bool pending_item=false;
};
// Original 0x4224a8 for one poison target (effect 0 or 0x70). MEFF 45/49 and
// numeric/level windows are ordered requests, not completed presentation.
// Automatic healing suspends before the item, armor XP and final refresh.
PoisonApplication apply_poison_target(const BattleUnit& actor,const StrikeTarget&,
 uint32_t damage,int effect,uint32_t armor_xp,int area,int beans,
 std::array<int,2> camera,int tick,int width,int height,const Json& data);
struct PoisonFinalization {
 StrikeTarget actor;
 std::array<int,2> camera;
 Json requests=Json::array();
};
// Original 0x422f0d poison branch. success is set by 0x42377d only after
// application returns. Party XP/level animation is deliberately unsupported.
PoisonFinalization finalize_enemy_poison(const StrikeTarget& actor,const BattleUnit& target,
 bool success,uint32_t experience,uint32_t weapon_xp,std::array<int,2> camera,
 int width,int height,const Json& data,const Json& tactics);
}
