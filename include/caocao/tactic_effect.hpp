#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Original 0x421f04 single-target poison reaction, before HP/status changes.
Json poison_reaction(const BattleUnit&,const UnitRecord&,uint32_t damage,int effect,
 std::array<int,2> camera);
// 0x4214bf/0x401e5c, MEFF#23 only. Event stream retains capture/restore order.
Json poison_effect_animation(const BattleUnit&,const UnitRecord&,uint32_t damage,
 int effect,std::array<int,2> camera,int map_width,int map_height,const Json& resource);
// Callback-free MEFF#45/#48/#49 player at the already focused target.
Json secondary_effect_animation(const BattleUnit&,std::array<int,2> camera,const Json& resource);
// Original 0x45ade5: displayed side, active occupancy, then row-major order.
std::vector<BattleUnit> scripted_area_units(const std::vector<BattleUnit>&,
 std::array<int,4> rectangle,int side_filter);
Json scripted_area_confusion(const std::vector<BattleUnit>&,std::array<int,4> rectangle,
 int side_filter,std::array<int,2> camera,int width,int height,const Json& resource);
// The MEFF player bypasses the quantizing compositor for weight=0/palette=255.
std::vector<uint8_t> direct_meff_pixels(const std::vector<uint8_t>& source,
 const std::vector<uint8_t>& background);
// 0x42fd5a MEFF blend: palette-index compositing, not RGB alpha blending.
std::vector<uint8_t> blend_poison_pixels(const std::vector<uint8_t>& source,
 const std::vector<uint8_t>& background,const std::vector<uint8_t>& quantized,
 const std::vector<uint8_t>& lookup,int weight=7,int amount=16);
}
