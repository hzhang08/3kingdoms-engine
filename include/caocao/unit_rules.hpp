#pragma once
#include "caocao/engine.hpp"
#include <array>
#include <optional>
namespace caocao {
// Persistent character state, independent of battlefield placement/presentation.
struct UnitRecord {
 int id=0,owner=255,job=0,level=1,xp=0,hp=0,mp=0;
 std::array<int,5> base_stats{},stats{};
 std::array<std::array<int,3>,3> equipment{}; // Original ID, level, experience.
 std::array<int,3> record_counters{};
 static UnitRecord from_json(const Json&);
 Json json()const;
 void validate()const;
};
UnitRecord load_unit_record(const Json& hero);
void grow_unit(UnitRecord&,int increment,const Json& classes);
void default_equipment(UnitRecord&,bool lower_one,const Json& classes);
int original_party_average(std::vector<int> levels);
UnitRecord join_unit(const Json& hero,int party_average,int adjustment,const Json& classes);
// Returns the item whose acquisition the original records in its collection file.
int equip_unit(UnitRecord&,int slot,int encoded_item,int level,const Json& data);
std::vector<int> script_equipment(UnitRecord&,const std::array<int,5>& args,const Json& data);
void initialize_npc(UnitRecord&,int level,int promotion_mode,const Json& data);
int equipment_bonus(const UnitRecord&,int statistic,const Json& data);
// Presence is separate from strength: some effects have strength zero.
std::optional<int> equipment_effect(const UnitRecord&,int effect,const Json& data);
int battle_sprite(int id,int job,int side);
int movement_cost(const UnitRecord&,int terrain,const Json& data);
}
