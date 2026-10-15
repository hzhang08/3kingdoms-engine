#pragma once
#include "caocao/engine.hpp"
#include "caocao/battle.hpp"
namespace caocao {
// Arithmetic recovered from Ekd5.exe 0x43bc4b. These are resolved inputs;
// Unit-level getters and hit probability are below. This is still not a full
// attack action (critical/double selection, HP/XP, death events, presentation).
struct PhysicalDamageInputs {
    uint32_t attack=0, defense=0, attack_terrain=10, defense_terrain=10;
    uint32_t level=1, attack_class_group=0, defense_class_group=0, target_hp=0;
    uint32_t mounted_resistance_percent=0, anti_infantry_percent=0;
    bool randomize=false, counterattack=false, hit=true;
};
uint32_t physical_damage_arithmetic(const PhysicalDamageInputs& in, OriginalRng& rng);
int battle_stat(const BattleUnit&,const UnitRecord&,int index,const Json& data);
int battle_terrain_factor(const UnitRecord&,int terrain,const Json& data);
int physical_hit_chance(const BattleUnit& attacker,const UnitRecord&,
 const BattleUnit& target,const UnitRecord&,const Json& data);
uint32_t physical_damage(const BattleUnit& attacker,const UnitRecord&,
 const BattleUnit& target,const UnitRecord&,int attack_terrain,int defense_terrain,
 bool randomize,bool counterattack,const Json& data,OriginalRng&);
}
