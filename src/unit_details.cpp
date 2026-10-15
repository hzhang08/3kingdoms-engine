#include "caocao/unit_details.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/movement.hpp"
#include <algorithm>
namespace caocao {
Json unit_details(const Prototype& game,int id,const Json& ui,const Json& tactics){
 const auto& unit=game.state().at("units").at(std::to_string(id));
 const auto b=BattleUnit::from_json(unit.at("battle"));const auto u=UnitRecord::from_json(unit.at("record"));const auto& data=game.data();
 int terrain=game.terrain().at("cells").at(b.y*game.map_width()+b.x);
 Json out={{"id",id},{"name",unit.at("name")},{"face",data.at("heroes").at(id).at("face")},
  {"side",b.side==0?"我军":b.side==2?"敌军":"友军"},{"class",ui.at("classes").at(u.job).at("text")},
  {"level",u.level},{"xp",u.xp==255?Json(nullptr):Json(u.xp)},{"hp",b.hp},{"max_hp",std::min(500,u.hp+equipment_bonus(u,5,data))},
  {"mp",b.mp},{"max_mp",std::min(250,u.mp+equipment_bonus(u,6,data))},{"movement",battle_movement_points(b,u,data)},
  {"terrain",ui.at("terrain").at(terrain).at("text")},{"terrain_percent",battle_terrain_factor(u,terrain,data)},
  {"stats",Json::array()},{"equipment",Json::array()},{"tactics",Json::array()}};
 std::string status;
 for(auto [bit,label]:{std::pair{2,"麻痹"},std::pair{4,"禁咒"},std::pair{8,"混乱"},std::pair{16,"中毒"}})if(b.status&bit){if(!status.empty())status+="、";status+=label;}
 out["status"]=status.empty()?"正常":status;
 const char* stats[]{"攻击力","防御力","精神力","爆发力","士气"};
 for(int n=0;n<5;++n)out["stats"].push_back({{"name",stats[n]},{"value",battle_stat(b,u,n,data)}});
 const char* slots[]{"武器","护具","辅助"};
 for(int slot=0;slot<3;++slot){const auto& e=u.equipment[slot];Json item={{"slot",slots[slot]},{"id",e[0]},{"name","无"},{"level",nullptr},{"xp",nullptr},{"bonuses",Json::array()}};
  if(e[0]!=255){const auto& raw=data.at("items").at(e[0]);item["name"]=raw.at("name");
   if(slot<2){if(e[1]!=255)item["level"]=e[1];if(e[2]!=255)item["xp"]=e[2];}
   auto single=u;for(auto& v:single.equipment)v={255,255,255};single.equipment[slot]=e;
   const char* labels[]{"攻击力","防御力","精神力","爆发力","士气","兵力","策略值","移动力","经验"};
   for(int n=0;n<9;++n)if(int bonus=equipment_bonus(single,n,data))item["bonuses"].push_back({{"name",labels[n]},{"value",bonus}});
  }
  out["equipment"].push_back(item);
 }
 auto available=game.spells(id);
 for(const auto& t:tactics){const auto& f=t.at("fields");int learn=f.at(17+u.job),cost=f.at(14);if(!learn||learn>u.level)continue;
  int spell=t.at("id");bool supported=spell==0||spell==5||spell==10||spell==15||spell==33;
  if(game.revised_rules())for(int enabled:{2,7,17,20,23,25,35,39,61,63})if(spell==enabled)supported=true;
  if(auto divisor=equipment_effect(u,49,data);divisor&&*divisor>0)cost/=*divisor;
  bool ready=std::any_of(available.begin(),available.end(),[&](const Json& a){return a.at("id")==spell;});
  out["tactics"].push_back({{"id",spell},{"name",t.at("name")},{"learn_level",learn},{"mp_cost",cost},{"supported",supported},
   {"availability",!supported?"暂未开放":!game.tactics_enabled()?"旧版规则":(b.status&4)?"禁咒中":ready?"已习得":"策略值不足"}});
 }
 return out;
}
}
