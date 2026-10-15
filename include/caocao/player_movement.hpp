#pragma once
#include "caocao/movement.hpp"
namespace caocao {
// Original 43d538 with ordinary option flags (4b4230 bits4/8 clear).
// The outer player loop and action menu own selection and action completion.
struct PlayerMovementState {
 BattleUnit actor;
 int mode=255;
 bool waiting=true,action_menu=false;
 std::array<int,2> camera;
 std::vector<int> distance,occupancy;
 Json events=Json::array();
};
PlayerMovementState begin_player_movement(const BattleUnit&,const UnitRecord&,
 std::array<int,2> camera,int width,int height,const Json& terrain,
 const std::vector<BattleUnit>& roster,const Json& data);
// One supplied tile-picker response, or {255,255} to cancel. Returned events
// describe this transition only; an invalid destination remains waiting.
PlayerMovementState choose_player_movement(const PlayerMovementState&,
 std::array<int,2> tile,const UnitRecord&,int speed,int width,int height,
 const Json& terrain,const std::vector<BattleUnit>& roster,const Json& data);
}
