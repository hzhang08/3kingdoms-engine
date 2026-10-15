#pragma once
#include "caocao/range.hpp"
namespace caocao {
struct PlayerHover {
 int slot=255;
 bool retained=false;
 Json events=Json::array();
};
// Original44e26b..44e3a9. World tile, covered state and main-window focus are
// host inputs; this does not map physical mouse coordinates or render panels.
PlayerHover update_player_hover(int previous,std::array<int,2> tile,bool retained,
 bool main_window_focused,bool covered,bool input_disabled,
 const std::vector<BattleUnit>& roster);
// Original44e620 scan after completion; differs from player_phase_gate.
int remaining_player_units(const std::vector<BattleUnit>&);
struct PlayerCommandMenu {
 std::array<bool,4> enabled{}; // attack, tactic, item, wait
 RangeResult attack;
};
// Original43dde3 menu preparation after43d538. Ordinary option flags only.
// Tactic availability here is distinct from castability and MP affordability.
PlayerCommandMenu prepare_player_command_menu(const BattleUnit&,const UnitRecord&,
 const std::vector<BattleUnit>&,const std::vector<UnitRecord>&,
 const std::vector<int>& consumables,int width,int height,RangeGrids,
 int preferred,const Json& data,const Json& tactics);
struct PlayerActionTail {
 BattleUnit actor;
 std::vector<int> occupancy;
 bool completed=false;
 Json events=Json::array();
};
// Original43dde3 after the menu response. Attack/tactic/item callbacks remain
// explicit guards until their player paths are independently verified.
PlayerActionTail finish_player_command(const BattleUnit& before_move,
 const BattleUnit& current,int response,int width,int height,
 const std::vector<int>& occupancy);
}
