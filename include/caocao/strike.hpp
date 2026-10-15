#pragma once
#include "caocao/combat_rules.hpp"
namespace caocao {
// Original 0x405e03 numerical preparation for one strike. The caller provides
// the original ordered targets. This does not apply HP/XP or play effects.
struct StrikeTarget { BattleUnit battle; UnitRecord unit; int terrain=0; };
struct StrikePlan {
 std::vector<uint32_t> hp_damage,mp_damage,armor_xp;
 uint32_t xp=0,weapon_xp=0;
 bool critical=false;
 Json json()const;
};
int physical_critical_chance(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,const Json& data);
int physical_double_chance(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,const Json& data);
// 0x405744/0x40c02f: optional Imsg.e5 record before the strike animation.
// speech_group is Data.e5 hero-record byte 16 (persistent offset 0x26).
// This consumes the presentation RNG draws even when no speech is selected.
std::optional<int> physical_critical_speech(int unit,int speech_group,bool critical,OriginalRng&);
StrikePlan plan_physical_strike(const BattleUnit&,const UnitRecord&,int terrain,
 const std::vector<StrikeTarget>&,int round,int counter_depth,int priority_id,
 uint32_t previous_xp,uint32_t previous_weapon_xp,const Json& data,OriginalRng&);
// Original 0x406181 after presentation and HP/status/experience application.
// Presentation may consume RNG (e.g. the critical-speech roll at 0x40582a),
// so callers must not join this directly to planning as a complete attack.
// A counterattack or
// the second strike skips the double-attack random check entirely.
bool roll_another_strike(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,
 int round,int counter_depth,const Json& data,OriginalRng&);
struct CounterPlan { bool attack=false; int depth=0; };
// Original 0x40639a gate. Caller must supply the board after strike application;
// facing and the recursive attack are returned to the controller, not run here.
CounterPlan plan_counterattack(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,
 int counter_depth,int width,int height,const std::vector<BattleUnit>&,const Json& data);
}
