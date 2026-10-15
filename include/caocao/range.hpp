#pragma once
#include "caocao/battle.hpp"
namespace caocao {
int battle_attack_range(const UnitRecord&,const Json& data);
struct RangeGrids {
 std::vector<int> distance,threat,mask;
};
struct RangeResult {
 RangeGrids grids;
 std::vector<int> targets;
 int selected=255;
};
// Original 0x436586: mode bit 1 selects allies and includes the center;
// bit 128 preserves distance/threat grids. mask_only paints the separate mask.
RangeResult battle_range(const BattleUnit&,std::array<int,2> center,int kind,int mode,
 bool mask_only,int preferred,int width,int height,RangeGrids,
 const std::vector<BattleUnit>&);
}
