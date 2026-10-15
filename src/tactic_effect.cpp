#include "caocao/tactic_effect.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
std::vector<uint8_t> blend_poison_pixels(const std::vector<uint8_t>& src,
 const std::vector<uint8_t>& bg,const std::vector<uint8_t>& q,const std::vector<uint8_t>& lut,int base_weight,int amount){
 if(base_weight<0||base_weight>16||amount<0||amount>16)throw std::invalid_argument("unverified MEFF blend parameters");
 if(src.size()!=bg.size()||q.size()!=768||lut.size()!=4096)throw std::invalid_argument("invalid poison compositor buffers");
 auto out=bg;
 for(size_t i=0;i<src.size();++i){int a=src[i],b=bg[i];if(!a)continue;
  int brightness=(q[a*3]+q[a*3+1]+q[a*3+2])/3;
  int weight=(base_weight*(256-brightness*amount))>>4;
  int foreground=256-((weight*(16-amount))>>4);int rgb[3];
  for(int c=0;c<3;++c)rgb[c]=std::min(15,(q[b*3+c]*weight+q[a*3+c]*foreground)>>8);
  out[i]=lut[(rgb[0]<<8)|(rgb[1]<<4)|rgb[2]];
 }
 return out;
}
std::vector<uint8_t> direct_meff_pixels(const std::vector<uint8_t>& src,const std::vector<uint8_t>& bg){
 if(src.size()!=bg.size())throw std::invalid_argument("invalid direct MEFF buffers");
 auto out=bg;for(size_t n=0;n<src.size();++n)if(src[n])out[n]=src[n];return out;
}
Json poison_reaction(const BattleUnit& b,const UnitRecord& u,uint32_t damage,int effect,std::array<int,2> camera){
 if(b.id!=u.id||b.direction<0||b.direction>3||(effect!=0&&effect!=112))throw std::invalid_argument("unverified poison reaction input");
 int sprite=battle_sprite(b.id,u.job,b.side),frame=effect==112?3:std::array<int,4>{1,2,0,2}[b.direction];
 Json events=Json::array({{{"kind","archive"},{"name","UNIT_SPC.E5"},{"index",sprite},{"size",11520}}});
 Json transforms=Json::array();if((b.flags&6)==6)transforms.push_back({2,4});
 events.push_back({{"kind","frame"},{"x",b.x*48-camera[0]},{"y",b.y*48-camera[1]+56},
  {"width",48},{"height",48},{"slot",b.slot},{"archive","UNIT_SPC.E5"},{"sprite",sprite},
  {"frame",frame},{"mirror",effect==0&&b.direction==1},{"effects",transforms}});
 if(damage)events.push_back({{"kind","number"},{"slot",b.slot},{"value",damage},{"foreground",18},{"background",24}});
 return events;
}
Json poison_effect_animation(const BattleUnit& b,const UnitRecord& u,uint32_t damage,int effect,
 std::array<int,2> camera,int mw,int mh,const Json& resource){
 if(resource.at("resource")!=23||resource.at("frames").size()!=13||resource.at("width")!=75||resource.at("height")!=90||resource.at("interval_ticks")!=2||resource.at("sound")!=119)
  throw std::invalid_argument("unverified poison effect resource");
 for(auto xy:battle_camera_path(camera[0],camera[1],b.x,b.y,mw,mh))camera=xy;
 Json events=Json::array({{{"kind","camera"},{"tile",{camera[0]/48,camera[1]/48}}},
  {{"kind","archive"},{"name","MEFF.E5"},{"index",23},{"size",88030}},
  {{"kind","message_pump"},{"args",{0,514,0,0}}}});
 int x=b.x*48-camera[0]+24-75/2,y=b.y*48-camera[1]+56+24-90/2;
 for(int n=0;n<13;++n){const auto& f=resource.at("frames").at(n);
  Json rect={x+f.at("dx").get<int>(),y+f.at("dy").get<int>(),75,90};
  if(n)events.push_back({{"kind","effect_restore"},{"rectangle",rect}});
  if(f.at("callback")==1)for(const auto& e:poison_reaction(b,u,damage,effect,camera))events.push_back(e);
  else if(f.at("callback")!=0)throw std::runtime_error("unverified MEFF callback");
  events.push_back({{"kind","capture"},{"rectangle",rect}});
  events.push_back({{"kind","effect"},{"rectangle",rect},{"weight",f.at("weight")},{"amount",f.at("amount")},{"frame",n},{"pixels_sha256",f.at("pixels_sha256")}});
  if(!n){events.push_back({{"kind","stop_sound"}});events.push_back({{"kind","sound"},{"id",119},{"flag",1}});}
  events.push_back({{"kind","wait"},{"ticks",2}});
 }
 events.push_back({{"kind","effect_restore"},{"rectangle",{x,y-14,75,90}}});
 events.push_back({{"kind","unit_redraw"},{"slot",b.slot}});events.push_back({{"kind","wait"},{"ticks",10}});
 return events;
}
Json secondary_effect_animation(const BattleUnit& b,std::array<int,2> camera,const Json& resource){
 int id=resource.at("resource"),w=resource.at("width"),h=resource.at("height"),count=resource.at("frames").size();
 if(!(((id==48||id==49)&&w==16&&h==16&&count==13&&resource.at("sound")==129)||
      (id==45&&w==64&&h==64&&count==9&&resource.at("sound")==131))||resource.at("interval_ticks")!=1)
  throw std::invalid_argument("unverified secondary MEFF resource");
 Json events=Json::array({{{"kind","archive"},{"name","MEFF.E5"},{"index",id},{"size",20+count*20+w*h}},
  {{"kind","message_pump"},{"args",{0,514,0,0}}}});
 int x=b.x*48-camera[0]+24-w/2,y=b.y*48-camera[1]+56+24-h/2;Json previous;
 for(int n=0;n<count;++n){const auto& f=resource.at("frames").at(n);
  if(f.at("callback")!=0||f.at("image")!=0||f.at("palette")!=255)throw std::runtime_error("unverified secondary MEFF descriptor");
  Json rect={x+f.at("dx").get<int>(),y+f.at("dy").get<int>(),w,h};
  if(n)events.push_back({{"kind","effect_restore"},{"rectangle",previous}});
  events.push_back({{"kind","capture"},{"rectangle",rect}});
  events.push_back({{"kind","effect"},{"resource",id},{"rectangle",rect},{"weight",f.at("weight")},{"amount",f.at("amount")},{"frame",n},{"pixels_sha256",f.at("pixels_sha256")}});
  if(f.at("weight")==0)events.back()["direct"]=true;
  if(!n){events.push_back({{"kind","stop_sound"}});events.push_back({{"kind","sound"},{"id",resource.at("sound")},{"flag",1}});}
  events.push_back({{"kind","wait"},{"ticks",1}});previous=rect;
 }
 events.push_back({{"kind","effect_restore"},{"rectangle",previous}});return events;
}
std::vector<BattleUnit> scripted_area_units(const std::vector<BattleUnit>& units,std::array<int,4> r,int filter){
 for(int n:r)if(n<0||n>254)throw std::invalid_argument("unverified area coordinate");
 std::vector<BattleUnit> selected;
 for(const auto& b:units){
  if(b.lifecycle!=2||b.x<r[0]||b.x>r[2]||b.y<r[1]||b.y>r[3])continue;
  int side=b.side<=3?b.side:0;
  if((filter>=0&&filter<=3&&side==filter)||(filter==4&&side<=1)||(filter==5&&side>=2)||filter==6)selected.push_back(b);
 }
 std::sort(selected.begin(),selected.end(),[](const auto& a,const auto& b){return a.y!=b.y?a.y<b.y:a.x<b.x;});
 return selected;
}
Json scripted_area_confusion(const std::vector<BattleUnit>& units,std::array<int,4> r,int filter,
 std::array<int,2> camera,int width,int height,const Json& resource){
 if(resource.at("resource")!=48)throw std::invalid_argument("confusion requires MEFF48");
 Json events=Json::array();
 for(auto b:scripted_area_units(units,r,filter)){
  events.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
  for(auto xy:battle_camera_path(camera[0],camera[1],b.x,b.y,width,height))camera=xy;
  for(const auto& e:secondary_effect_animation(b,camera,resource))events.push_back(e);
  events.push_back({{"kind","refresh"},{"unit",b.id}});
  b.status|=8;events.push_back({{"kind","tile"},{"battle",b.json()},{"visible",true}});
  events.push_back({{"kind","refresh"},{"unit",b.id}});
 }
 return events;
}

}
