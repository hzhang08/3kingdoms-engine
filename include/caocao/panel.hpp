#pragma once
#include "caocao/battle.hpp"
namespace caocao {
struct TerrainPanelInfo {bool recovery=false,impassable_label=false;std::array<bool,4> tactics{};};
// Original display predicates 0x435182/0x455c3a and fire/water/earth/wind
// selectors. The label is not a replacement for class-specific movement costs.
TerrainPanelInfo terrain_panel_info(int terrain);
int terrain_grayscale_index(int index,const Json& ui);
Json battle_terrain_panel(std::array<int,2> tile,std::array<int,2> camera,int terrain,const Json& ui);

// 0x450984 NPC result panel. Includes cursor, raster and wait/removal requests.
// Party experience panels are a separate, as-yet unsupported path.
Json battle_damage_panel(const BattleUnit&,const UnitRecord&,const std::string& name,
 const BattleUnit* actor,std::array<int,2> camera,int32_t hp_loss,int32_t mp_loss,
 bool wait,const Json& data,const Json& ui);
// Original 0x4404bd mode 0. UI strings are imported from this edition's PE;
// output captures indexed raster requests and original GBK glyph positions.
// The unit must already be in the focused 13x8-tile viewport.
Json battle_unit_panel(const BattleUnit&,const UnitRecord&,const std::string& name,
 std::array<int,2> camera,int terrain,const Json& data,const Json& ui);
std::array<int,2> battle_panel_position(const BattleUnit&,std::array<int,2> camera,const BattleUnit* other=nullptr);
// Original mode 1: deterministic base damage and hit chance, without RNG draws.
Json battle_attack_panel(const BattleUnit& target,const UnitRecord&,const std::string& name,
 const BattleUnit& actor,const UnitRecord&,std::array<int,2> camera,
 int target_terrain,int actor_terrain,const Json& data,const Json& ui);
// Original mode 2 for the opening's 23/25/33 tactics; preview draws no RNG.
Json battle_tactic_panel(const BattleUnit& target,const UnitRecord&,const std::string& name,
 const BattleUnit& actor,const UnitRecord&,std::array<int,2> camera,int tactic,
 const Json& data,const Json& ui,const Json& tactics);
}
