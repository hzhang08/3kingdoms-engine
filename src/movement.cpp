#include "caocao/movement.hpp"
#include "caocao/ai.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
namespace caocao {
namespace {constexpr int neighbors[4][2]={{0,-1},{1,0},{0,1},{-1,0}};}
ScriptMovement scripted_battle_movement(const BattleUnit& input,const UnitRecord& u,
 std::array<int,2> destination,int direction,std::array<int,2> camera,int speed,
 int width,int height,const Json& terrain,const std::vector<BattleUnit>& roster,
 const std::vector<UnitRecord>& records,const Json& data,const Json& tactics){
 ScriptMovement out{input,input,input,camera,{}};
 if(input.lifecycle!=2)throw std::runtime_error("scripted movement of a hidden/retired unit is not yet verified (0x4594b5)");
 for(auto& n:destination)n&=255;
 if(destination[0]>=width||destination[1]>=height)return out;
 // Original 0x45b6d0 restores the saved target coordinates, not target_slot.
 // Its invalid-coordinate memory lookup is outside the verified interface.
 if(input.target_x>=width||input.target_y>=height)throw std::runtime_error("scripted movement with an invalid saved target is not yet verified (0x45b6d0)");
 auto& b=out.prepared;b.flags=(b.flags&~6)|128;b.ai=0;
 std::vector<int> occupancy(width*height,255);
 for(const auto& other:roster)if(other.lifecycle==2)occupancy.at(other.y*width+other.x)=other.slot;
 if(occupancy.at(destination[1]*width+destination[0])!=255){
  destination=battle_placement(destination[0],destination[1],u.job<=38?u.job/3:u.job-26,width,height,terrain,occupancy,data);
  if(destination[0]==255)throw std::runtime_error("scripted movement destination fallback failed (0x43f040)");
 }
 b.target_x=destination[0];b.target_y=destination[1];b.target_slot=255;
 auto decision=decide_battle_ai(b,u,0,roster,records,width,height,terrain,data,tactics);
 out.distance=decision.grids.distance;
 auto focus=camera;
 for(auto point:battle_camera_path(camera[0]*48,camera[1]*48,b.x,b.y,width,height))focus={point[0]/48,point[1]/48};
 out.moved=decision.actor;
 if(decision.reached_movement){
  auto walk=battle_ai_movement(decision.actor,u,decision.grids.best_tile,focus,speed,width,height,terrain,decision.grids.distance,decision.route_scratch,roster,data);
  out.events=std::move(walk.events);out.moved=walk.actor;
  if(!out.events.empty())out.camera=walk.camera;
  // 0x440252 changes a completed coordinate-following AI to mode 3.
  if(out.moved.x==out.moved.target_x&&out.moved.y==out.moved.target_y)out.moved.ai=3;
 }
 out.actor=out.moved;out.actor.ai=input.ai;
 out.actor.flags=(out.actor.flags&~128)|(input.flags&~8);
 out.actor.target_slot=255;out.actor.target_x=input.target_x;out.actor.target_y=input.target_y;
 if(direction>=0&&direction<=3){
  out.actor.direction=direction;
  out.actor.flags=(out.actor.flags&~8)|(direction==1?8:0);
  out.events.push_back({{"kind","unit_redraw"},{"slot",input.slot}});
 }
 return out;
}
AiMovement battle_ai_movement(const BattleUnit& input,const UnitRecord& u,std::array<int,2> destination,
 std::array<int,2> camera,int speed,int width,int height,const Json& terrain,
 const std::vector<int>& distance,std::vector<int> route,
 const std::vector<BattleUnit>& roster,const Json& data){
 AiMovement out{input,camera,std::vector<int>(width*height,255)};auto& b=out.actor;auto& events=out.events;
 for(const auto& other:roster)if(other.lifecycle==2)out.occupancy.at(other.y*width+other.x)=other.slot;
 if(b.lifecycle!=2||destination[0]==255||destination==std::array<int,2>{b.x,b.y})return out;
 route=battle_movement_route(b,u,{b.x,b.y},destination,255,false,width,height,terrain,distance,
  std::move(route),std::vector<int>(width*height),roster,data).scratch;
 if(route[0]==255)return out;
 if(b.flags&128)events.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
 else events.push_back({{"kind","selection"},{"slot",b.slot},{"args",Json::array({0,255,255})}});
 out.occupancy.at(b.y*width+b.x)=255;
 size_t n=0;
 while(std::array<int,2>{b.x,b.y}!=destination){
  if(n>=route.size())throw std::runtime_error("original AI route did not reach its destination");
  int direction=route.at(n++);
  if(direction<0||direction>=4)throw std::runtime_error("unverified AI route direction");
  int nx=b.x+neighbors[direction][0],ny=b.y+neighbors[direction][1];
  if(nx<0||nx>=width||ny<0||ny>=height)continue;
  auto step=battle_walking_step(b,u.job,direction,speed,out.camera,width,height,out.occupancy);
  for(const auto& e:step.events)events.push_back(e);
  out.camera=step.camera;b=step.actor;b.x=nx;b.y=ny;
 }
 if(!(b.flags&128))b.flags|=4;
 out.occupancy.at(b.y*width+b.x)=b.slot;events.push_back({{"kind","redraw"}});return out;
}
WalkingStep battle_walking_step(const BattleUnit& input,int job,int direction,int speed,
 std::array<int,2> camera,int width,int height,const std::vector<int>& occupancy){
 if(direction<0||direction>=4||speed<0||speed>=3||job<0||job>=53||occupancy.size()!=static_cast<size_t>(width*height))throw std::runtime_error("invalid walking step input");
 WalkingStep out{input,camera};auto& b=out.actor;auto& events=out.events;
 std::array<int,2> to{b.x+neighbors[direction][0],b.y+neighbors[direction][1]};
 if(to[0]<0||to[0]>=width||to[1]<0||to[1]>=height)throw std::runtime_error("walking destination outside map");
 int sx=(b.x-camera[0])&255,sy=(b.y-camera[1])&255,dx=sx,dy=sy;
 int old=occupancy.at(b.y*width+b.x),next=occupancy.at(to[1]*width+to[0]);
 b.direction=direction;
 if(direction==1)b.flags|=8;
 if(direction==3)b.flags&=~8;
 bool scroll=false;
 if(direction==0){if(to[1]<=camera[1]&&camera[1]>0){--out.camera[1];sy=(sy+1)&255;scroll=true;}else dy=(dy-1)&255;}
 if(direction==1){if(to[0]>=camera[0]+13&&camera[0]+13<width){++out.camera[0];sx=(sx-1)&255;scroll=true;}else dx=(dx+1)&255;}
 if(direction==2){if(to[1]>=camera[1]+8&&camera[1]+8<height){++out.camera[1];sy=(sy-1)&255;scroll=true;}else dy=(dy+1)&255;}
 if(direction==3){if(to[0]<=camera[0]&&camera[0]>0){--out.camera[0];sx=(sx+1)&255;scroll=true;}else dx=(dx-1)&255;}
 if(scroll)events.push_back({{"kind","camera_set"},{"x",out.camera[0]},{"y",out.camera[1]},{"animate",0}});
 int group=job<=38?job/3:job-26;
 const bool fast=group==0||group==3||group==4||group==11||group==13||group==17||group==20;
 const int slow_steps[]{48,12,6},fast_steps[]{48,24,12};int increment=fast?fast_steps[speed]:slow_steps[speed];
 auto overlay=[&](int x,int y,int slot){events.push_back({{"kind","overlay"},{"x",x*48},{"y",y*48+56},{"slot",slot}});};
 for(int distance=increment;distance<=48;distance+=increment){
  events.push_back({{"kind","wait"},{"ticks",1}});
  if(old!=255&&direction!=0)overlay(sx,sy,old);
  int facing=direction==0?1:direction==2?0:2;
  events.push_back({{"kind","frame"},{"x",sx*48+(dx-sx)*distance},{"y",sy*48+(dy-sy)*distance+56},
   {"width",48},{"height",48},{"slot",b.slot},{"frame",facing*2+(distance-1)/24},{"mirror",facing==2&&bool(b.flags&8)}});
  if(next!=255&&direction==2)overlay(dx,dy,next);
  if(old!=255&&direction==0)overlay(sx,sy,old);
 }
 events.push_back({{"kind","final_tile"},{"group",group},{"x",to[0]},{"y",to[1]}});
 return out;
}
int battle_movement_points(const BattleUnit& b,const UnitRecord& u,const Json& data){
 int points=std::min(255,data.at("classes").at(u.job).at(0).get<int>()+equipment_bonus(u,7,data));
 if(b.status&2)return 0;
 if(b.grades[5]>=4)return std::min(255,points+2);
 if(b.grades[5]<=2)return std::max(0,points-1);
 return points;
}
bool battle_zone_of_control(const BattleUnit& b,const UnitRecord& u,std::array<int,2> tile,
 const std::vector<BattleUnit>& roster,const Json& data){
 for(int slot=0;slot<3;++slot){int id=u.equipment[slot][0];if(id==255)continue;
  const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2?type==33:(type<18&&type%2==1&&item.at("special_type")==33))return false;
 }
 for(const auto& other:roster)if(other.lifecycle==2&&(other.slot<35)!=(b.slot<35)&&std::abs(other.x-tile[0])+std::abs(other.y-tile[1])==1)return true;
 return false;
}
MovementSearch battle_movement_search(const BattleUnit& b,const UnitRecord& u,
 std::array<int,2> origin,int budget,std::array<int,2> target,MovementSearchMode mode,
 bool respect_zoc,int width,int height,const Json& terrain,const std::vector<BattleUnit>& roster,const Json& data,const std::vector<int>& threats,const std::function<void(std::array<int,2>)>& score){
 if(width<1||height<1||terrain.size()!=static_cast<size_t>(width*height)||origin[0]<0||origin[0]>=width||origin[1]<0||origin[1]>=height||budget<0||budget>255)throw std::runtime_error("invalid movement search input");
 if(mode!=MovementSearchMode::range&&mode!=MovementSearchMode::target&&mode!=MovementSearchMode::opponent_range&&mode!=MovementSearchMode::score)throw std::runtime_error("unverified movement search mode");
 if(mode==MovementSearchMode::score&&!score)throw std::runtime_error("AI candidate scorer required");
 if(mode==MovementSearchMode::opponent_range&&threats.size()!=terrain.size())throw std::runtime_error("invalid movement target grid");
 MovementSearch out;if(origin==target)return out;
 out.costs.assign(width*height,255);out.costs[origin[1]*width+origin[0]]=0;
 if(mode==MovementSearchMode::score&&!(b.flags&128))score(origin);
 if(movement_cost(u,terrain.at(origin[1]*width+origin[0]),data)==255)return out;
 std::vector<int> occupied(width*height,255);
 for(const auto& other:roster)if(other.lifecycle==2)occupied.at(other.y*width+other.x)=other.slot;
 bool found=true;int current=0;
 while(found&&current<budget){
  found=false;int next=255,older=255;
  for(int y=0;y<height;++y){
   for(int x=0;x<width;++x){int distance=out.costs[y*width+x];
    if(distance>current&&distance!=255)older=std::min(older,distance);
    if(distance!=current)continue;
    std::array<int,2> tile{x,y};
    if(tile!=origin&&respect_zoc&&battle_zone_of_control(b,u,tile,roster,data))continue;
    for(const auto& d:neighbors){int nx=x+d[0],ny=y+d[1];if(nx<0||nx>=width||ny<0||ny>=height)continue;
     auto& cost=out.costs[ny*width+nx];if(cost!=255)continue;
     int terrain_cost=movement_cost(u,terrain.at(ny*width+nx),data),occupant=occupied[ny*width+nx];
     if(terrain_cost==255||distance+terrain_cost>budget)continue;
     if(occupant!=255&&(occupant<35)!=(b.slot<35)&&(respect_zoc||!(b.flags&128)))continue;
     cost=distance+terrain_cost;found=true;
     if(next>cost){
      if(respect_zoc&&!(b.flags&128)){
       if(!battle_zone_of_control(b,u,{nx,ny},roster,data))next=std::min(cost,older);
      }else next=cost;
     }
     if(mode==MovementSearchMode::opponent_range){int slot=threats.at(ny*width+nx);
      if(slot!=255&&std::any_of(roster.begin(),roster.end(),[&](const auto& other){return other.slot==slot&&other.lifecycle==2;})){out.result=slot;return out;}
     }
     if(mode==MovementSearchMode::target&&target==std::array<int,2>{nx,ny}){out.result=1;return out;}
     if(mode==MovementSearchMode::score)score({nx,ny});
    }
   }
   if(!found){if(older!=255){next=older;found=true;}}
   else next=std::min(next,older);
  }
  current=next;
 }
 return out;
}
void battle_route_scores(const BattleUnit& b,const UnitRecord& u,std::array<int,2> p,int value,
 int width,int height,const std::vector<BattleUnit>& roster,std::vector<int>& scores){
 if(width<1||height<1||scores.size()!=static_cast<size_t>(width*height)||p[0]<0||p[0]>=width||p[1]<0||p[1]>=height)throw std::runtime_error("invalid route score input");
 if(value<9)return;
 bool occupied=std::any_of(roster.begin(),roster.end(),[&](const auto& o){return o.lifecycle==2&&o.x==p[0]&&o.y==p[1];});
 auto& cell=scores.at(p[1]*width+p[0]);
 // Terrain getter 0x43f84d turns 255 into 0; its !=255 check always passes.
 if(cell==0&&(!occupied||p==std::array<int,2>{b.x,b.y}))cell=value;
 if(u.job==15||u.job==16)return;
 for(const auto& d:neighbors){std::array<int,2> next{p[0]+d[0],p[1]+d[1]};
  if(next[0]>=0&&next[0]<width&&next[1]>=0&&next[1]<height)battle_route_scores(b,u,next,value>10?value-10:value-1,width,height,roster,scores);
 }
}
MovementRoute battle_movement_route(const BattleUnit& b,const UnitRecord& u,
 std::array<int,2> origin,std::array<int,2> destination,int budget,bool score_on_failure,
 int width,int height,const Json& terrain,const std::vector<int>& costs,
 std::vector<int> scratch,std::vector<int> scores,const std::vector<BattleUnit>& roster,const Json& data){
 auto inside=[&](std::array<int,2> p){return p[0]>=0&&p[0]<width&&p[1]>=0&&p[1]<height;};
 auto bytes=[](const auto& v){return std::all_of(v.begin(),v.end(),[](int n){return n>=0&&n<=255;});};
 if(width<1||height<1||!inside(origin)||!inside(destination)||budget<0||budget>255||terrain.size()!=static_cast<size_t>(width*height)||costs.size()!=terrain.size()||scores.size()!=terrain.size()||scratch.size()<256||!bytes(costs)||!bytes(scores)||!bytes(scratch))throw std::runtime_error("invalid movement route input");
 auto offset=[&](std::array<int,2> p){return p[1]*width+p[0];};
 auto occupant=[&](std::array<int,2> p){for(const auto& other:roster)if(other.lifecycle==2&&other.x==p[0]&&other.y==p[1])return other.slot;return 255;};
 auto current=destination;bool advancing=true;int count=0;
 for(;current!=origin&&advancing;++count){
  int best=costs[offset(current)],direction=255,price=0;
  for(int n=0;n<4;++n){std::array<int,2> next{current[0]+neighbors[n][0],current[1]+neighbors[n][1]};
   if(!inside(next)||(battle_zone_of_control(b,u,next,roster,data)&&next!=origin&&!(b.flags&128)))continue;
   int distance=costs[offset(next)];if(distance>=best)continue;
   int cost=movement_cost(u,terrain.at(offset(next)),data);
   if(cost<=budget){best=distance;direction=n;price=cost;}
  }
  if(direction!=255){current[0]+=neighbors[direction][0];current[1]+=neighbors[direction][1];budget-=price;scratch.at(count)=direction;}
  else advancing=false;
 }
 if(!advancing&&score_on_failure){
  battle_route_scores(b,u,current,occupant(current)==255?10:20,width,height,roster,scores);
 }
 // The original increments count even for a failed descent. That final,
 // untouched caller byte participates in inversion; do not normalize it.
 if(count==0)scratch[0]=255;
 else for(int left=0,right=count-1;left<=right;++left,--right){int old=scratch.at(left);scratch[left]=(scratch.at(right)+2)%4;scratch[right]=(old+2)%4;}
 return {std::move(scratch),std::move(scores)};
}
}
