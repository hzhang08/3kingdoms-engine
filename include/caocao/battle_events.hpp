#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Script outcome latches are distinct from the controller's finished outcome.
// Evaluating a condition can set a latch even when that event is ineligible.
struct BattleEventState {
 int turn=1,phase=0,outcome=0;
 bool victory=false,defeat=false;
 std::vector<int> variables=std::vector<int>(1024,0);
 std::vector<BattleUnit> units;
};
bool opening_battle_condition(const Json& instruction,BattleEventState&);
// Original 0x4100a1: evaluate every condition, retain the LAST eligible event.
int select_opening_event(const std::vector<Program>&,BattleEventState&);
}
