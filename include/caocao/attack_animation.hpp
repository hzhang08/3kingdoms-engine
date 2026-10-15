#pragma once
#include "caocao/strike.hpp"
namespace caocao {
struct AttackAnimation {
 BattleUnit actor,target;
 int tick=0;
 Json events=Json::array();
};
// Single-target 0x4044e9 animation after 0x403b8f/0x403d85 preparation.
// Camera is already framed; HP/MP application has not happened. Original
// background capture/restore and numeric glyph layout remain renderer work.
// Each frame's effects is an ordered palette-operation list: {0,brightness}
// or {1,green,blue,red}. Repeated quantization is intentional.
AttackAnimation physical_attack_animation(const BattleUnit&,const UnitRecord&,
 const BattleUnit&,const UnitRecord&,uint32_t hp_damage,uint32_t mp_damage,
 bool critical,int camera_tile_x,int camera_tile_y,int tick,const Json& data);
// Original indexed-pixel transforms, preserving transparent index zero.
// quantized contains 768 bytes; lookup contains 4096 original RGB matches.
std::vector<uint8_t> transform_combat_pixels(std::vector<uint8_t>,const Json& effects,
 const std::vector<uint8_t>& quantized,const std::vector<uint8_t>& lookup);
}
