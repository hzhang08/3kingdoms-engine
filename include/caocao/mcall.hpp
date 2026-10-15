#pragma once
#include "caocao/engine.hpp"
namespace caocao {
// MCALL05's original 0x403014 player; center is in viewport pixel coordinates.
Json mcall_animation(int center_x,int center_y,const Json& resource);
// 0x4300c8, with the original shared foreground/background palette.
std::vector<uint8_t> blend_mcall_pixels(const std::vector<uint8_t>& source,
 const std::vector<uint8_t>& mask,const std::vector<uint8_t>& background,
 const std::vector<uint8_t>& quantized,const std::vector<uint8_t>& lookup,int amount);
}
