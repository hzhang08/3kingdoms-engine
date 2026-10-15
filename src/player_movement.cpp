#include "caocao/player_movement.hpp"
#include "caocao/range.hpp"
#include <stdexcept>
namespace caocao {
namespace {
void request(Json& events,const char* kind,Json args=Json::array()){
 events.push_back({{"kind",kind},{"args",std::move(args)}});
}
void message(Json& events,const char* text){
 request(events,"sound",{2,1});events.push_back({{"kind","message"},{"style",2},{"text",text}});
}
}
PlayerMovementState begin_player_movement(const BattleUnit& b,const UnitRecord& u,
 std::array<int,2> camera,int width,int height,const Json& terrain,
 const std::vector<BattleUnit>& roster,const Json& data){
 if(b.lifecycle!=2)throw std::runtime_error("player movement requires active unit (43d538)");
 PlayerMovementState out;out.actor=b;out.camera=camera;
 if(b.slot>=35)out.mode=0;
 else if((b.flags&128)||b.ai!=7)out.mode=1;
 else if(b.status&8)out.mode=5;
 else if(b.flags&2)out.mode=6;
 out.distance=battle_movement_search(b,u,{b.x,b.y},battle_movement_points(b,u,data),
  {255,255},MovementSearchMode::range,true,width,height,terrain,roster,data).costs;
 out.occupancy.assign(width*height,255);
 for(const auto& other:roster)if(other.lifecycle==2)out.occupancy.at(other.y*width+other.x)=other.slot;
 request(out.events,b.side!=0&&b.ai!=7?"movement_overlay":"blue_movement",{1});
 int kind=battle_attack_range(u,data);
 if(kind!=255)request(out.events,"range",{b.x|(b.y<<8),kind,0});
 request(out.events,"redraw");return out;
}
PlayerMovementState choose_player_movement(const PlayerMovementState& input,
 std::array<int,2> tile,const UnitRecord& u,int speed,int width,int height,
 const Json& terrain,const std::vector<BattleUnit>& roster,const Json& data){
 if(!input.waiting)throw std::runtime_error("player movement already returned (43d538)");
 if(tile!=std::array<int,2>{255,255}&&(tile[0]<0||tile[0]>=width||tile[1]<0||tile[1]>=height))
  throw std::runtime_error("player movement tile outside map (45418f)");
 auto out=input;out.events=Json::array();auto& b=out.actor;auto& events=out.events;
 events.push_back({{"kind","pick"},{"tile",tile}});
 if(out.mode==255){
  if(tile[0]==255)out.mode=2;
  else {
   int offset=tile[1]*width+tile[0],slot=out.occupancy.at(offset);
   // The supplied roster produces occupancy for active units only. Original
   // 4401a8 additionally permits stale lifecycle1 occupants; that host state
   // is outside this interface's verified occupancy invariant.
   if(slot==255&&movement_cost(u,terrain.at(offset),data)!=255&&out.distance.at(offset)!=255)out.mode=4;
   else if(slot==b.slot)out.mode=3;
   else {
    message(events,slot!=255&&out.distance.at(offset)!=255?"因为有其他部队，所以不能移动。":"不是移动范围。");
    return out;
   }
  }
 }
 if(out.mode==2||((out.mode==0||out.mode==1||out.mode==5||out.mode==6)&&tile[0]==255))request(events,"sound",{1,1});
 else if(out.mode==0)message(events,"这是敌军部队。");
 else if(out.mode==1)message(events,"这是友军部队。");
 else if(out.mode==5)message(events,"正处于混乱状态。");
 else if(out.mode==6)message(events,"已经行动完毕。");
 request(events,"blue_movement",{0});request(events,"movement_overlay",{0});
 request(events,"range",{b.x|(b.y<<8),255,0});request(events,"redraw");
 out.waiting=false;
 if(out.mode==3){request(events,"sound",{0,1});out.action_menu=true;}
 if(out.mode==4){
  auto route=battle_movement_route(b,u,{b.x,b.y},tile,255,false,width,height,terrain,
   out.distance,std::vector<int>(256,255),std::vector<int>(width*height),roster,data).scratch;
  out.occupancy.at(b.y*width+b.x)=255;
  constexpr int dx[]{0,1,0,-1},dy[]{-1,0,1,0};size_t n=0;
  while(std::array<int,2>{b.x,b.y}!=tile){
   if(n>=route.size())throw std::runtime_error("player movement route exhausted (43da2e)");
   int direction=route.at(n++);
   if(direction<0||direction>3)throw std::runtime_error("invalid player route direction (43da8d)");
   int x=b.x+dx[direction],y=b.y+dy[direction];
   if(x<0||x>=width||y<0||y>=height)continue;
   auto step=battle_walking_step(b,u.job,direction,speed,out.camera,width,height,out.occupancy);
   for(const auto& event:step.events)events.push_back(event);
   b=step.actor;out.camera=step.camera;b.x=x;b.y=y;
  }
  out.occupancy.at(b.y*width+b.x)=b.slot;b.flags|=4;
  events.push_back({{"kind","unit_redraw"},{"slot",b.slot}});out.action_menu=true;
 }
 return out;
}
}
