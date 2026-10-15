#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Original opcode 33, placement mode 0 / 0x456140. State mutations remain
// ordered steps: terrain record, focus, sound, mark plane, redraw, wait.
Json fire_placement(const Json& records,const Json& terrain,int width,int height,
 int x,int y,bool focus,bool silent,std::array<int,2> camera);
int fire_frame_offset(int tick);
// Validate the currently supported fire-only stable snapshot against its map.
void validate_terrain_effects(const Json& records,const Json& marks,const Json& terrain,
 const Json& original,int width,int height,bool battlefield);
}
