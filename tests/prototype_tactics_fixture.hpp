#pragma once
// Test-only legal contact position: avoids waiting behind the independent
// scenario's static roster while still executing the normal AI/cast pipeline.
inline void prepare_caster_contact(caocao::Prototype& g,int caster){
 using namespace caocao;auto p=Json::parse(g.serialize());auto& s=p["payload"]["state"];auto record=UnitRecord::from_json(s.at("units").at(std::to_string(caster)).at("record"));bool placed=false;
 for(int y=0;y<g.map_height()&&!placed;++y)for(int x=0;x<g.map_width()&&!placed;++x){
  if(movement_cost(record,g.terrain().at("cells").at(y*g.map_width()+x),g.data())==255)continue;
  bool occupied=false;for(const auto& u:s.at("units")){const auto& b=u.at("battle");if(b.at("id")!=caster&&b.at("hp").get<int>()>0&&b.at("x")==x&&b.at("y")==y)occupied=true;}
  if(occupied||g.spell_options(caster,{x,y}).empty())continue;
  s["units"][std::to_string(caster)]["battle"]["x"]=x;s["units"][std::to_string(caster)]["battle"]["y"]=y;placed=true;
 }
 if(!placed)throw std::runtime_error("caster contact fixture has no legal target");
 int cursor=0;for(const auto& u:s.at("units"))if(u.at("battle").at("side")==2&&u.at("battle").at("id").get<int>()<caster)++cursor;
 s["phase"]="enemy";s["enemy_cursor"]=cursor;s["selected"]=-1;s["undo"]=nullptr;
 p["sha256"]=sha256(p.at("payload").dump());g.restore(p.dump());
}
