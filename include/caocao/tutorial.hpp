#pragma once
#include "caocao/engine.hpp"
namespace caocao {
// Recovered executable routines, independent of rendering/audio/storage.
Json tutorial_menu_presentation(const Json& cursor,int pc,const Json& ui,int focus=2);
Json tutorial_program(int completed_campaigns,bool all_treasures);
std::pair<int,bool> tutorial_collection_status(const std::vector<int>&,const Json& data);
void tutorial_show_range(Json& ui,const GameState&,int unit,int width,int height,const Json& data,bool blue=false);
}
