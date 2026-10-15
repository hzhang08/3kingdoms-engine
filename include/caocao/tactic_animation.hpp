#pragma once
#include "caocao/battle.hpp"
namespace caocao {
struct TacticCastAnimation {
 BattleUnit actor;
 std::array<int,2> camera;
 Json events=Json::array();
};
// 0x4218d6 for the opening's tactics 23, 25 and 33. The elapsed caption
// ticks are an explicit clock input; no RNG, HP, status or MP changes here.
TacticCastAnimation tactic_cast_animation(const BattleUnit&,const UnitRecord&,
 const BattleUnit&,int tactic,std::array<int,2> camera,int map_width,int map_height,
 int message_speed,int caption_elapsed_ticks,const Json& tactics);
// 0x473202, type-0 caption background and border raster requests.
Json tactic_caption_window(std::array<int,4> rectangle);
}
