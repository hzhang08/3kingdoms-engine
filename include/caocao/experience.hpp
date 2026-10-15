#pragma once
#include "caocao/unit_rules.hpp"
namespace caocao {
struct ExperienceChange {
 // Preserve one entry per original growth call, including repeated level 50
 // when a single XP grant crosses the cap. Display is handled by the caller.
 std::vector<int> levels;
 bool refresh=false;
};
// Original 0x408950 takes an absolute XP total (old XP plus the action award).
// This mutates persistent records only, without any level-up presentation.
ExperienceChange set_character_experience(UnitRecord&,uint32_t total,const Json& data);
// Original 0x4082d6 takes an XP increment for weapon slot 0 or armor slot 1.
ExperienceChange add_equipment_experience(UnitRecord&,int slot,uint32_t gain,const Json& data);
}
