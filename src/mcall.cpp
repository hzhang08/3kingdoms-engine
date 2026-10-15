#include "caocao/mcall.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
std::vector<uint8_t> blend_mcall_pixels(const std::vector<uint8_t>& src,
 const std::vector<uint8_t>& mask,const std::vector<uint8_t>& bg,
 const std::vector<uint8_t>& q,const std::vector<uint8_t>& lut,int amount){
 if(amount<0||amount>16||src.size()!=mask.size()||src.size()!=bg.size()||q.size()!=768||lut.size()!=4096)
  throw std::invalid_argument("invalid MCALL compositor input");
 auto out=bg;
 for(size_t n=0;n<src.size();++n){int a=src[n],b=bg[n],m=mask[n];
  if(!a||(!m&&!amount))continue;
  if(m==255){out[n]=lut[(q[a*3]<<8)|(q[a*3+1]<<4)|q[a*3+2]];continue;}
  int brightness=(q[a*3]+q[a*3+1]+q[a*3+2])/3;
  int weight=((256-m)*(256-brightness*amount))>>8;
  int foreground=256-((weight*(16-amount))>>4),rgb[3];
  for(int c=0;c<3;++c)rgb[c]=std::min(15,(q[b*3+c]*weight+q[a*3+c]*foreground)>>8);
  out[n]=lut[(rgb[0]<<8)|(rgb[1]<<4)|rgb[2]];
 }
 return out;
}
Json mcall_animation(int cx,int cy,const Json& resource){
 if(resource.at("resource")!=105||resource.at("width")!=300||resource.at("height")!=300||resource.at("frames").size()!=30||resource.at("interval_ticks")!=2||resource.at("sound")!=112)
  throw std::invalid_argument("unverified MCALL resource");
 Json events=Json::array(),previous;
 for(int n=0;n<30;++n){const auto& f=resource.at("frames").at(n);
  if(f.at("palette")!=255||f.at("image")!=n||f.at("sound_mode")!=0)throw std::runtime_error("unverified MCALL descriptor");
  Json rect={cx-150+f.at("dx").get<int>(),cy-150+f.at("dy").get<int>(),300,300};
  if(n)events.push_back({{"kind","effect_restore"},{"rectangle",previous}});
  events.push_back({{"kind","capture"},{"rectangle",rect}});
  events.push_back({{"kind","effect"},{"resource",105},{"rectangle",rect},{"frame",n},
   {"masked",f.at("masked")},{"weight",f.at("weight")},{"amount",f.at("amount")}});
  if(!n)events.push_back({{"kind","sound"},{"id",112},{"flag",1}});
  events.push_back({{"kind","wait"},{"ticks",2}});previous=rect;
 }
 events.push_back({{"kind","effect_restore"},{"rectangle",previous}});
 return events;
}
}
