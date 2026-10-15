#include "caocao/terrain_effect.hpp"
#include <set>
#include <stdexcept>
namespace caocao {
namespace {
void record_shape(const Json& records){
 if(!records.is_array()||records.size()!=250)throw std::runtime_error("invalid terrain-effect table");
 for(const auto& row:records){
  if(!row.is_array()||row.size()!=5)throw std::runtime_error("invalid terrain-effect record");
  for(const auto& byte:row)if(!byte.is_number_integer()||byte<0||byte>255)throw std::runtime_error("invalid terrain-effect byte");
 }
}
}
Json fire_placement(const Json& records,const Json& terrain,int width,int height,int x,int y,bool focus,bool silent,std::array<int,2> camera){
 record_shape(records);
 if(width<13||height<8||terrain.size()!=size_t(width*height)||x<0||x>=width||y<0||y>=height)throw std::runtime_error("invalid fire tile");
 int slot=0;for(;slot<250;++slot){const auto& r=records.at(slot);if(r.at(4)==255||(r.at(0)==x&&r.at(1)==y))break;}
 Json steps=Json::array();if(slot==250)return steps;
 const int tile=y*width+x;
 steps.push_back({{"kind","fire_terrain"},{"slot",slot},{"tile",tile},{"record",{x,y,26,terrain.at(tile),0}}});
 if(focus&&!silent){
  steps.push_back({{"kind","camera_request"},{"x",x},{"y",y}});
  auto path=battle_camera_path(camera[0],camera[1],x,y,width,height);if(!path.empty())camera=path.back();
 }
 if(!silent)steps.push_back({{"kind","sound"},{"id",19},{"flag",0}});
 steps.push_back({{"kind","fire_mark"},{"tile",tile},{"effect",0}});
 steps.push_back({{"kind","redraw"},{"rectangle",{(x-camera[0]/48)&255,(y-camera[1]/48)&255,1,1}}});
 if(!silent)steps.push_back({{"kind","wait"},{"ticks",1}});
 return steps;
}
int fire_frame_offset(int tick){if(tick<0||tick>255)throw std::runtime_error("invalid fire tick");return 0x8bc0+(tick%4)*0x900;}
void validate_terrain_effects(const Json& records,const Json& marks,const Json& terrain,const Json& original,int width,int height,bool battlefield){
 record_shape(records);
 if(!marks.is_array()||marks.size()!=size_t(width*height)||!terrain.is_array()||terrain.size()!=marks.size())throw std::runtime_error("invalid terrain-effect planes");
 Json expected=original,expected_marks=std::vector<int>(width*height,255);std::set<int> occupied;
 bool empty_seen=false;
 for(const auto& r:records){
  if(r.at(4)==255){if(r!=Json::array({255,255,255,255,255}))throw std::runtime_error("invalid unused terrain-effect record");empty_seen=true;continue;}
  if(!battlefield||empty_seen||r.at(4)!=0||r.at(2)!=26||r.at(0)>=width||r.at(1)>=height)throw std::runtime_error("unsupported saved terrain effect");
  int tile=r.at(1).get<int>()*width+r.at(0).get<int>();
  if(!occupied.insert(tile).second||(r.at(3)!=original.at(tile)&&r.at(3)!=26))throw std::runtime_error("inconsistent saved terrain effect");
  expected.at(tile)=26;expected_marks.at(tile)=0;
 }
 for(size_t n=0;n<marks.size();++n)if(!marks.at(n).is_number_integer()||!terrain.at(n).is_number_integer()||marks.at(n)!=expected_marks.at(n)||terrain.at(n)!=expected.at(n))throw std::runtime_error("inconsistent saved terrain-effect plane");
}
}
