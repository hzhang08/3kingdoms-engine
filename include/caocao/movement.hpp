#pragma once
#include "caocao/battle.hpp"
#include <functional>
namespace caocao {
int battle_movement_points(const BattleUnit&,const UnitRecord&,const Json& data);
bool battle_zone_of_control(const BattleUnit&,const UnitRecord&,std::array<int,2>,
 const std::vector<BattleUnit>&,const Json& data);
struct MovementSearch {
 int result=255;
 // Empty means original early return before touching the distance grid.
 std::vector<int> costs;
};
enum class MovementSearchMode { range=0, target=2, score=4, opponent_range=8 };
// Mode 4 visits candidates in the original order. The AI caller owns scoring.
MovementSearch battle_movement_search(const BattleUnit&,const UnitRecord&,
 std::array<int,2> origin,int budget,std::array<int,2> target,MovementSearchMode,
 bool respect_zoc,int width,int height,const Json& terrain,
 const std::vector<BattleUnit>&,const Json& data,const std::vector<int>& threats={},
 const std::function<void(std::array<int,2>)>& score={});
// Original 0x438178's recursive fallback preferences, shared by AI and routes.
void battle_route_scores(const BattleUnit&,const UnitRecord&,std::array<int,2>,int value,
 int width,int height,const std::vector<BattleUnit>&,std::vector<int>& scores);
struct MovementRoute {
 // Original routine reverses caller scratch in place without a terminator.
 // Consumers must use their destination, not treat this as a sentinel list.
 std::vector<int> scratch;
 std::vector<int> scores;
};
MovementRoute battle_movement_route(const BattleUnit&,const UnitRecord&,
 std::array<int,2> origin,std::array<int,2> destination,int budget,bool score_on_failure,
 int width,int height,const Json& terrain,const std::vector<int>& costs,
 std::vector<int> scratch,std::vector<int> scores,
 const std::vector<BattleUnit>&,const Json& data);
struct WalkingStep {
 BattleUnit actor;
 std::array<int,2> camera;
 Json events=Json::array();
};
// Original 0x44201b. The caller updates occupancy/coordinates after the step.
WalkingStep battle_walking_step(const BattleUnit&,int job,int direction,int speed,
 std::array<int,2> camera,int width,int height,const std::vector<int>& occupancy);
struct AiMovement {
 BattleUnit actor;
 std::array<int,2> camera;
 std::vector<int> occupancy;
 Json events=Json::array();
};
// Original 0x43a956. Selection UI and camera focus are interface requests;
// the caller must resolve them before presenting the walking events.
AiMovement battle_ai_movement(const BattleUnit&,const UnitRecord&,std::array<int,2> destination,
 std::array<int,2> camera,int speed,int width,int height,const Json& terrain,
 const std::vector<int>& distance,std::vector<int> route,
 const std::vector<BattleUnit>& roster,const Json& data);
struct ScriptMovement {
 BattleUnit prepared,moved,actor;
 std::array<int,2> camera;
 std::vector<int> distance;
 Json events=Json::array();
};
// Original 0x459487 for an active unit. Retired-unit reappearance is guarded.
// Camera coordinates are tiles; presentation contains the original walk requests.
ScriptMovement scripted_battle_movement(const BattleUnit&,const UnitRecord&,
 std::array<int,2> destination,int direction,std::array<int,2> camera,int speed,
 int width,int height,const Json& terrain,const std::vector<BattleUnit>&,
 const std::vector<UnitRecord>&,const Json& data,const Json& tactics);
}
