#include "caocao/ai.hpp"
#include "caocao/movement.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/tactics.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
std::vector<int> available_ai_tactics(const BattleUnit& b,const UnitRecord& u,const Json& tactics){
 if(tactics.size()!=68||u.job<0||u.job>=53)throw std::runtime_error("invalid original tactic table");
 std::vector<int> out;
 for(int id=0;id<68;++id){const auto& t=tactics.at(id);
  if(t.size()!=70)throw std::runtime_error("invalid original tactic record");
  int level=t.at(17+u.job),cost=t.at(14);
  if(level!=0&&level<=u.level&&cost<=b.mp)out.push_back(id);
 }
 return out;
}
AiDecision decide_battle_ai(const BattleUnit& input,const UnitRecord& u,int priority_id,
 const std::vector<BattleUnit>& roster,const std::vector<UnitRecord>& records,
 int width,int height,const Json& terrain,const Json& data,const Json& tactics,OriginalRng rng){
 using Tile=std::array<int,2>;using Mode=MovementSearchMode;
 AiDecision out;out.actor=input;out.rng=rng;auto& b=out.actor;
 auto slot=[&](int id)->const BattleUnit&{
  auto it=std::find_if(roster.begin(),roster.end(),[&](const auto& r){return r.slot==id;});
  if(it==roster.end())throw std::runtime_error("AI requires original hidden/visible slot "+std::to_string(id));
  return *it;
 };
 int budget=b.flags&128?100:battle_movement_points(b,u,data);
 if(b.target_slot!=255&&slot(b.target_slot).lifecycle!=2){b.target_slot=255;b.ai=1;}
 if(b.lifecycle!=2||(b.flags&2)||(b.status&8))return out;
 out.grids=prepare_battle_ai(b,u,roster,width,height,terrain,data,std::vector<int>(width*height));
 auto& g=out.grids;out.route_scratch.assign(256,0);
 Tile current{b.x,b.y},excluded{255,255};int attack_target=255;
 std::vector<int> available;
 auto score=[&](Tile tile){
  out.trace.push_back({{"kind","candidate"},{"tile",tile}});
  bool occupied=std::any_of(roster.begin(),roster.end(),[&](const auto& r){return r.lifecycle==2&&Tile{r.x,r.y}==tile;});
  if(occupied&&tile!=current)return;
  int at=tile[1]*width+tile[0],attack=0;AiTacticScore magic;auto& value=g.scores.at(at);
  if(!(b.flags&128)){
   if(b.ai!=4){
    if(g.threat.at(at)!=255||tile==current){
     auto result=score_ai_attack(b,u,tile,priority_id,roster,records,width,height,terrain,data);
     attack=result.score;attack_target=result.target;
    }
    if(!(b.status&4))magic=score_ai_tactics(b,u,tile,available,roster,records,width,height,data,tactics,out.rng);
    value=(value+std::max(attack,magic.score))&255; // Original byte addition deliberately wraps.
   }
   int t=terrain.at(at),bonus=t>=18&&t<=22?8:std::max(0,battle_terrain_factor(u,terrain.at(b.y*width+b.x),data)-10);
   value=std::min(255,value+bonus);
   if(tile==current&&attack==0)value=std::min(255,value+1);
  }
  if(excluded[0]!=255&&tile==excluded)value=0;
  if(value>g.best_score){g.best_score=value;g.best_tile=tile;
   g.best_target=magic.score>attack?magic.target:attack_target;g.best_tactic=magic.score>attack?magic.tactic:255;
  }
 };
 auto search=[&](Tile origin,int limit,Mode mode,Tile target=Tile{255,255},bool zoc=true){
  out.trace.push_back({{"kind","search"},{"origin",origin},{"budget",limit},{"mode",static_cast<int>(mode)},{"target",target},{"zoc",zoc}});
  if(origin!=target){
   out.route_scratch[0]=255;
   if(mode==Mode::score){g.best_score=0;g.best_target=255;g.best_tile[0]=255;g.best_tactic=255;available=available_ai_tactics(b,u,tactics);}
  }
  auto result=battle_movement_search(b,u,origin,limit,target,mode,zoc,width,height,terrain,roster,data,g.threat,score);
  if(!result.costs.empty())g.distance=std::move(result.costs);
  return result.result;
 };
 auto seed=[&](Tile center,int value){battle_route_scores(b,u,center,value,width,height,roster,g.scores);};
 auto route=[&](Tile origin){
  auto r=battle_movement_route(b,u,origin,current,budget,true,width,height,terrain,g.distance,std::move(out.route_scratch),std::move(g.scores),roster,data);
  out.route_scratch=std::move(r.scratch);g.scores=std::move(r.scores);
 };
 auto seek=[&](){
  int near=search(current,budget,Mode::opponent_range);
  if(near!=255){out.seek_flag=1;search(current,budget,Mode::score);return;}
  int distant=search(current,255,Mode::opponent_range);
  if(distant==255){g.best_tile=current;return;}
  const auto& opponent=slot(distant);Tile center{opponent.x,opponent.y};
  bool found=search(center,255,Mode::target,current)==1;
  if(!found)found=search(center,255,Mode::target,current,false)!=255;
  if(found){route(center);search(current,budget,Mode::score);}
 };
 auto approach=[&](Tile center){
  bool found=search(center,255,Mode::target,current)==1;
  if(!found)found=search(center,255,Mode::target,current,false)!=255;
  if(found){route(center);search(current,budget,Mode::score);}else seek();
 };
 if((g.flags&1)&&!(b.flags&128)&&b.slot!=35){
  const auto& leader=slot(b.slot<35?0:35);Tile center{leader.x,leader.y};
  if(b.slot>=35&&leader.lifecycle!=2)center[0]=255;
  if(center[0]!=255){seed(center,10);approach(center);}else seek();
 }else switch(b.ai){
  case 0:case 4:{
   Tile center{b.target_x,b.target_y};
   if(b.target_slot!=255){const auto& target=slot(b.target_slot);center={target.x,target.y};
    if(target.ai==0||target.ai==4)excluded={target.target_x,target.target_y};
   }
   if(!(b.flags&128)){
    int near=search(current,budget,Mode::opponent_range);
    if(b.target_slot!=255&&near==b.target_slot){seed(center,20);search(current,budget,Mode::score);}
    else{seed(center,10);approach(center);}
   }else{
    int found=search(current,255,Mode::target,center);
    if(found==1||search(current,255,Mode::target,center,false)!=0)g.best_tile=center;
    else{g.best_tile=current;b.target_x=b.x;b.target_y=b.y;}
   }
   break;
  }
  case 1:seek();break;
  case 2:search(current,0,Mode::score);g.best_tile=current;break;
  case 3:
   if(search(current,budget,Mode::opponent_range)!=255)search(current,budget,Mode::score);
   else g.best_tile=current;
   break;
  default:break; // Original switch has no branch beyond AI modes 0..4.
 }
 out.reached_movement=true;return out;
}
}
