#pragma once
#include "caocao/battle.hpp"
namespace caocao {
bool ai_low_hp(const BattleUnit&,const UnitRecord&,const Json& data);
// Original 0x44da7a priority order, before the caller filters acted/player units.
std::vector<int> battle_ai_order(const std::vector<BattleUnit>&,
 const std::vector<UnitRecord>&,bool allied,int width,const Json& terrain,const Json& data);
struct PlayerPhaseGate {
 bool requires_input=false;
 std::array<int,2> focus{0,0};
};
// 0x44e0d9 with this edition's disabled AutoDLL autosave patch.
PlayerPhaseGate player_phase_gate(const std::vector<BattleUnit>&);
struct AiPreparation {
 std::vector<int> distance,threat,scores;
 int flags=0,best_score=0,best_target=255,best_tactic=255;
 std::array<int,2> best_tile{255,255};
};
// Original 0x43a28a: paints opponents using the actor's range, then seeds
// healing-terrain scores. The incoming distance scratch is only overpainted.
AiPreparation prepare_battle_ai(const BattleUnit&,const UnitRecord&,
 const std::vector<BattleUnit>&,int width,int height,const Json& terrain,
 const Json& data,std::vector<int> distance);
struct AiAttackScore {int score=0,target=255;};
// 0x438965/0x438a20 single-target branch. Area attacks stop explicitly.
AiAttackScore score_ai_attack(const BattleUnit&,const UnitRecord&,
 std::array<int,2> candidate,int priority_id,const std::vector<BattleUnit>&,
 const std::vector<UnitRecord>&,int width,int height,const Json& terrain,const Json& data);
// Original tactics admitted by class, level and current MP (0x43844a).
std::vector<int> available_ai_tactics(const BattleUnit&,const UnitRecord&,const Json& tactics);
struct AiDecision {
 BattleUnit actor;
 OriginalRng rng;
 AiPreparation grids;
 bool reached_movement=false;
 int seek_flag=0;
 std::vector<int> route_scratch;
 Json trace=Json::array();
};
// Original 0x43ad1c up to 0x43a956. Tactic scoring consumes RNG even before
// action selection; caller commits the returned RNG alongside the decision.
// Unsupported tactic and area branches stop explicitly.
AiDecision decide_battle_ai(const BattleUnit&,const UnitRecord&,int priority_id,
 const std::vector<BattleUnit>&,const std::vector<UnitRecord>&,int width,int height,
 const Json& terrain,const Json& data,const Json& tactics,OriginalRng rng={});
}
