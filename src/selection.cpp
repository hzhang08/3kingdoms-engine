#include "caocao/selection.hpp"
#include "caocao/range.hpp"
#include <stdexcept>
namespace caocao {
SelectionSequence battle_selection_sequence(const BattleUnit& b,const UnitRecord& u,
 int mode,int target,int tactic,int tick,int interrupt_at,const Json& data,const Json& tactics){
 if(mode<0||mode>2||tick<0||tick>7||interrupt_at< -1||interrupt_at>1||target<0||(target>=115&&!(mode==0&&target==255)))throw std::runtime_error("invalid selection presentation input");
 if(mode==2&&(tactic<0||tactic>=68||tactics.size()!=68))throw std::runtime_error("invalid selection tactic");
 SelectionSequence out;out.tick=tick;auto& events=out.events;
 auto emit=[&](const char* kind,const Json& args=Json::array()){events.push_back({{"kind",kind},{"args",args}});};
 auto panel=[&](int slot,int kind,int other,int item){events.push_back({{"kind","panel"},{"slot",slot},{"args",Json::array({kind,other,item})}});};
 auto redraw=[&](){events.push_back({{"kind","redraw"},{"tick",out.tick}});};
 auto range=[&](int kind){emit("range",Json::array({b.x,b.y,kind,0}));};
 emit("camera_request",Json::array({b.x,b.y}));emit("cursor",Json::array({b.x,b.y}));
 if(!(b.flags&4))panel(b.slot,0,255,255);
 for(int pass=0;pass<2;++pass){
  if(mode==0){if(pass==1){emit("movement_overlay",Json::array({1}));int kind=battle_attack_range(u,data);if(kind!=255)range(kind);}}
  else if(pass==0){int kind=mode==1?battle_attack_range(u,data):tactics.at(tactic).at(12).get<int>();if(kind!=255)range(kind);}
  else{
   emit("clear_panel");emit("focus_target",Json::array({target,0}));emit("target",Json::array({target}));
   panel(target,mode==1?1:tactic>=39&&tactic<=57?3:2,b.slot,mode==1?255:tactic);
  }
  out.tick=(out.tick+1)%8;redraw();
  if(interrupt_at==pass)break;
  emit("wait",Json::array({4}));
 }
 emit("clear_cursor");emit("clear_panel");emit("movement_overlay",Json::array({0}));range(255);redraw();
 return out;
}
}
namespace caocao {
std::vector<int> selection_target_mask(const BattleUnit* target,int width,int height){
 if(width<=0||height<=0||width*height>1600)throw std::runtime_error("invalid selection map dimensions");
 std::vector<int> mask(width*height);
 if(target&&target->x>=0&&target->x<width&&target->y>=0&&target->y<height)mask[target->y*width+target->x]=1;
 return mask;
}
std::vector<int> selection_range(std::array<int,2> center,int kind,bool include_center,int width,int height){
 if(kind==255)return std::vector<int>(width*height);
 if(kind<0||kind>15)throw std::runtime_error("unverified UI range kind");
 RangeGrids grids{std::vector<int>(width*height),std::vector<int>(width*height),std::vector<int>(width*height)};
 return battle_range(BattleUnit{},center,kind,128+(include_center?1:0),true,255,width,height,std::move(grids),{}).grids.mask;
}
std::array<int,2> command_menu_position(std::array<int,2> tile,std::array<int,2> camera,
 std::array<int,2> size,std::array<int,2> viewport,std::array<int,2> client){
 if(size[0]<=0||size[1]<=0||viewport[0]<=0||viewport[1]<=0)throw std::runtime_error("invalid command menu dimensions");
 int x=tile[0]==255?48:tile[0]*48-camera[0],y=tile[0]==255?48:tile[1]*48+56-camera[1];
 x+=x<viewport[0]/2?64:-size[0]-16;y-=(size[1]-48)/2;
 if(y+size[1]>viewport[1])y=viewport[1]-size[1];if(y<0)y=0;
 return {x+client[0],y+client[1]};
}
Json selection_tile_layers(std::array<int,2> tile,std::array<int,2> cursor,int style,
 bool movement,bool reachable,bool marked,bool covered,int slot,bool blue,int terrain_offset){
 if(style<0||style>3)throw std::runtime_error("unverified cursor style");
 Json out=Json::array();if(covered)return out;
 out.push_back({{"kind","map"}});
 if(movement||blue){
  if(reachable)out.push_back({{"kind","checker"},{"color",blue?60:90}});
  if(marked)out.push_back({{"kind","mark"},{"offset",0xcc0}});
 }else if(marked)out.push_back({{"kind","checker"},{"color",40}});
 if(terrain_offset>=0)out.push_back({{"kind","mark"},{"offset",terrain_offset}});
 int dx=std::abs(tile[0]-cursor[0]),dy=std::abs(tile[1]-cursor[1]);
 bool selected=style==0?dx+dy==0:style==1?dx+dy<=1:style==2?std::max(dx,dy)<=1:dx+dy<=2;
 if(selected)out.push_back({{"kind","mark"},{"offset",dx+dy==0?0x1ec0:0x15c0}});
 if(slot!=255)out.push_back({{"kind","unit"},{"slot",slot}});
 return out;
}
}
