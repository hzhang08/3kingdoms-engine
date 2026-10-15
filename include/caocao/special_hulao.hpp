#pragma once
#include "caocao/prototype.hpp"
namespace caocao::special_roster {
inline bool included(const Prototype& game,int id){return game.duel_includes(id);}
inline void restore(Prototype& game,const std::string& snapshot,const Json& manifest){
 Prototype candidate=game;candidate.restore(snapshot);
 if(!candidate.hulao_duel())throw std::runtime_error("Open this save in its original campaign. This is the special duel.");
 // Core validates the historical schema and exact allowed art identity first.
 // Migrate in memory only; a read never overwrites a manual save slot.
 candidate.upgrade_hulao_duel();
 if(Json::parse(candidate.serialize()).at("payload").at("duel_manifest")!=manifest.at("hash"))throw std::runtime_error("Special battle assets disagree");
 game=std::move(candidate);
}
}
