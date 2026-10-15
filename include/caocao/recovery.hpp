#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Logical effect requests in the original order. Rendering these requests is a
// separate presentation step; this module neither waits nor touches storage.
struct RecoveryEffects {
 std::vector<int> aura_slots;
 std::vector<std::array<int,3>> changes; // slot, attribute (6=status), encoded value
};
RecoveryEffects recover_battle_grades(BattleUnit&,const UnitRecord&,OriginalRng&,const Json& data);
RecoveryEffects recover_battle_status(BattleUnit&,const UnitRecord&,
 const std::vector<BattleUnit>& roster,const std::vector<UnitRecord>& records,
 OriginalRng&,const Json& data);
}
namespace caocao {
struct RegenerationAmounts {
 int hp=0,mp=0,xp=0,weapon_xp=0,armor_xp=0;
 bool any()const{return hp||mp||xp||weapon_xp||armor_xp;}
};
RegenerationAmounts battle_regeneration(const BattleUnit&,const UnitRecord&,int terrain,const Json& data);
int terrain_recovery_percent(int terrain);
int terrain_recovery_hp(int maximum_hp,int hp,int terrain);
// The original poisoned-HP-zero edge returns -1 (heals to 1); no lifecycle gate.
int battle_poison_loss(const BattleUnit&,const UnitRecord&,const Json& data);
}
