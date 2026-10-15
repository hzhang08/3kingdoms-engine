#pragma once
#include "caocao/battle.hpp"
namespace caocao {
struct SelectionSequence {Json events=Json::array();int tick=0;};
// Original 0x441488 presentation requests. interrupt_at models an input event
// at one of its two polls; -1 means no interruption. UI rendering is separate.
SelectionSequence battle_selection_sequence(const BattleUnit&,const UnitRecord&,
 int mode,int target,int tactic,int tick,int interrupt_at,const Json& data,const Json& tactics);
}
namespace caocao {
std::vector<int> selection_range(std::array<int,2> center,int kind,bool include_center,int width,int height);
// Original 0x453a40: nullptr is the 255 sentinel; invalid coordinates clear.
std::vector<int> selection_target_mask(const BattleUnit* target,int width,int height);
// Original 0x45b930/0x45b9bb. Window dimensions and client origin are host inputs.
std::array<int,2> command_menu_position(std::array<int,2> tile,std::array<int,2> camera_pixels,
 std::array<int,2> size,std::array<int,2> viewport={624,384},std::array<int,2> client_origin={0,0});
// Indexed tile draw order, recovered from 0x45218f. Coordinates are relative
// to the viewport; a panel freezes the covered background until dismissed.
Json selection_tile_layers(std::array<int,2> tile,std::array<int,2> cursor,int style,
 bool movement,bool reachable,bool marked,bool covered,int slot,bool blue=false,int terrain_offset=-1);
}
