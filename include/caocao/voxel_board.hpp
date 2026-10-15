#pragma once
#include <array>
#include <cmath>

namespace caocao::voxel {
struct Point {float x,y;};
// The illustration is a fixed camera backdrop. This affine board projects the
// original 20x20 logical terrain; simulation coordinates never change.
// Taller tiles are easier to pick beneath mounted voxel figures. The complete
// 20x20 board still fits the viewport; this changes no movement costs or ranges.
// Align rows 9/10 with the illustrated east gate rather than the lower wall.
inline constexpr float origin_x=80,origin_y=88,tile_width=72,tile_height=48,skew=3;
inline Point project(float x,float y){return {origin_x+tile_width*x-skew*y,origin_y+tile_height*y};}
inline Point inverse(float x,float y){float row=(y-origin_y)/tile_height;return {(x-origin_x+skew*row)/tile_width,row};}
inline std::array<int,2> cell(float x,float y){
 if(!std::isfinite(x)||!std::isfinite(y))return {-1,-1};
 auto p=inverse(x,y);if(p.x<0||p.y<0||p.x>=20||p.y>=20)return {-1,-1};
 return {int(p.x),int(p.y)};
}
}
