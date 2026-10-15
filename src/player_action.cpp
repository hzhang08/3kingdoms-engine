#include "caocao/player_action.hpp"
#include "caocao/combat_rules.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
PlayerHover update_player_hover(int previous,std::array<int,2> tile,bool retained,
 bool focused,bool covered,bool disabled,const std::vector<BattleUnit>& roster){
 if(previous<0||(previous>=115&&previous!=255))throw std::runtime_error("invalid retained player slot (44e283)");
 PlayerHover out{previous,retained};int current=255;
 for(const auto& b:roster)if(b.lifecycle==2&&std::array<int,2>{b.x,b.y}==tile){current=b.slot;break;}
 if(current!=255&&current!=previous){
  if(covered){out.events.push_back({{"kind","clear_panel"}});out.slot=255;}
  if(!disabled){out.slot=current;out.events.push_back({{"kind","panel"},{"slot",current},{"args",{0,255,255}}});}
 }
 if(current==255&&!(retained&&tile[1]==255&&focused)){
  out.slot=255;
  if(covered){out.events.push_back({{"kind","clear_panel"}});out.retained=false;}
 }
 return out;
}
int remaining_player_units(const std::vector<BattleUnit>& roster){
 int count=0;
 for(const auto& b:roster)if(b.slot<35&&b.lifecycle==2&&b.ai==7&&!(b.flags&2)&&!(b.status&8))++count;
 return count;
}
PlayerCommandMenu prepare_player_command_menu(const BattleUnit& b,const UnitRecord& u,
 const std::vector<BattleUnit>& roster,const std::vector<UnitRecord>& records,
 const std::vector<int>& consumables,int width,int height,RangeGrids grids,
 int preferred,const Json& data,const Json& tactics){
 if(b.ai!=7||(b.flags&2)||b.lifecycle!=2||consumables.size()!=17||tactics.size()!=68)
  throw std::runtime_error("unverified player menu entry (43dde3)");
 PlayerCommandMenu out;
 out.attack=battle_range(b,{b.x,b.y},battle_attack_range(u,data),0,false,preferred,width,height,std::move(grids),roster);
 out.enabled[0]=out.attack.selected!=255;
 auto learned=[&](const UnitRecord& record){
  for(const auto& tactic:tactics){int level=tactic.at(17+record.job);if(level!=0&&level<=record.level)return true;}
  return false;
 };
 if(equipment_effect(u,51,data)){
  constexpr int dx[]{0,1,0,-1},dy[]{-1,0,1,0};
  for(int direction=0;direction<4&&!out.enabled[1];++direction){
   int x=b.x+dx[direction],y=b.y+dy[direction];
   if(x<0||x>=width||y<0||y>=height)continue;
   for(const auto& other:roster)if(other.lifecycle==2&&other.x==x&&other.y==y){
    auto record=std::find_if(records.begin(),records.end(),[&](const auto& row){return row.id==other.id;});
    if(record==records.end())throw std::runtime_error("missing adjacent-unit record for player tactic menu");
    out.enabled[1]=learned(*record);break;
   }
  }
 }
 out.enabled[1]=out.enabled[1]||learned(u);
 for(int count:consumables)if(count<0||count>255)throw std::runtime_error("invalid consumable count");
 out.enabled[2]=std::any_of(consumables.begin(),consumables.end(),[](int count){return count!=0;});
 out.enabled[3]=true;return out;
}
PlayerActionTail finish_player_command(const BattleUnit& before,const BattleUnit& current,
 int response,int width,int height,const std::vector<int>& occupancy){
 if(response!=4&&response!=255)throw std::runtime_error("player attack/tactic/item callback is not yet verified (43dada/43dc21/43dcdc)");
 if(before.id!=current.id||before.slot!=current.slot||current.lifecycle!=2||before.direction<0||before.direction>3||
  before.x<0||before.x>=width||before.y<0||before.y>=height||current.x<0||current.x>=width||current.y<0||current.y>=height||
  occupancy.size()!=static_cast<size_t>(width*height)||occupancy.at(current.y*width+current.x)!=current.slot)
  throw std::runtime_error("invalid player action continuation (43dde3)");
 PlayerActionTail out{current,occupancy};auto& b=out.actor;auto& events=out.events;
 events.push_back({{"kind","release_button"}});
 auto redraw=[&](){events.push_back({{"kind","unit_redraw"},{"slot",b.slot}});};
 if(response==255){
  if(out.occupancy.at(before.y*width+before.x)!=255&&out.occupancy.at(before.y*width+before.x)!=b.slot)
   throw std::runtime_error("player movement rollback origin occupied (4350c0)");
  b.flags&=~4;out.occupancy.at(b.y*width+b.x)=255;redraw();
  b.x=before.x;b.y=before.y;out.occupancy.at(b.y*width+b.x)=b.slot;redraw();
  b.direction=before.direction;
  if(b.direction==1)b.flags|=8;
  else if(b.direction==3)b.flags&=~8;
 }else{b.flags|=6;out.completed=true;}
 redraw();events.push_back({{"kind","minimap"}});return out;
}
}
