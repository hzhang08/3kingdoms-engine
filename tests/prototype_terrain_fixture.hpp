#pragma once
#include "caocao/prototype.hpp"
// Explicit synthetic test input, never a user-save migration or game rule.
inline void terrain_fixture_place(caocao::Prototype& g,int id,int x,int y,int hp=-1){
 using namespace caocao;auto e=Json::parse(g.serialize());auto& s=e["payload"]["state"];auto& b=s["units"][std::to_string(id)]["battle"];
 int oldx=b.at("x"),oldy=b.at("y");
 for(auto& u:s["units"]){auto& other=u["battle"];if(other.at("id")!=id&&other.at("hp").get<int>()>0&&other.at("x")==x&&other.at("y")==y){other["x"]=oldx;other["y"]=oldy;}}
 b["x"]=x;b["y"]=y;if(hp>=0)b["hp"]=hp;b["lifecycle"]=b.at("hp")==0?3:2;b["flags"]=0;
 s["selected"]=-1;s["undo"]=nullptr;e["sha256"]=sha256(e.at("payload").dump());g.restore(e.dump());
}
inline void terrain_fixture_next_player(caocao::Prototype& g){
 using namespace caocao;auto e=Json::parse(g.serialize());auto& s=e["payload"]["state"];s["phase"]="enemy";s["selected"]=-1;s["undo"]=nullptr;
 int count=0;for(const auto& u:s.at("units"))if(u.at("battle").at("side")==2)++count;s["enemy_cursor"]=count;
 e["sha256"]=sha256(e.at("payload").dump());g.restore(e.dump());
}
