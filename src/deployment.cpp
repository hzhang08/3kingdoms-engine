#include "caocao/deployment.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
Json DeploymentSlot::json()const{return {{"slot",slot},{"id",id},{"x",x},{"y",y},{"flags",flags},{"ai",ai},{"target",target},{"promotion_mode",promotion_mode},{"level",level},{"equipment_overrides",equipment_overrides},{"weapon_level",weapon_level},{"armor_level",armor_level}};}
DeploymentSlot DeploymentSlot::from_json(const Json& j){DeploymentSlot s;
 #define FIELD(x) j.at(#x).get_to(s.x)
 FIELD(slot);FIELD(id);FIELD(x);FIELD(y);FIELD(flags);FIELD(ai);FIELD(target);FIELD(promotion_mode);FIELD(level);FIELD(equipment_overrides);FIELD(weapon_level);FIELD(armor_level);
 #undef FIELD
 return s;
}
int deployment_level(std::vector<int> levels){
 if(levels.empty()||levels.size()>15)throw std::runtime_error("invalid deployment party");
 std::sort(levels.begin(),levels.end(),std::greater<int>());int trim=levels.size()/4,sum=0;
 for(size_t i=trim;i<levels.size()-trim;++i)sum+=levels[i];
 return sum/static_cast<int>(levels.size()-trim*2);
}
static int facing(int direction){switch(direction){case 0:return 32;case 1:return 8;case 2:return 64;case 3:return 16;default:return 64;}}
DeploymentSlot deployment_row(int slot,bool enemy,const std::vector<int>& a,int base){
 if(a.size()!=(enemy?12u:11u))throw std::runtime_error("invalid deployment row size");
 DeploymentSlot s;s.slot=slot;s.id=a[0]<0?65535:a[0];if(s.id==65535)return s;
 int extra=enemy?1:0;auto value=[&](int i,int fallback){return a.at(i)<0?fallback:a.at(i);};
 s.x=a.at(2+extra)&255;s.y=a.at(3+extra)&255;
 s.flags=(enemy?(1|(value(1,0)?2:0)|(value(2,0)?4:0)):(2|(value(1,0)?4:0)))|facing(a.at(4+extra));
 int adjustment=value(5+extra,0);s.level=adjustment<=50?std::min(50,base+adjustment):std::max(1,base-(adjustment-50));
 s.promotion_mode=value(6+extra,2);s.ai=value(7+extra,1);
 if(s.ai==3||s.ai==5)s.target=a.at(8+extra)&65535;
 if(s.ai==4||s.ai==6)s.target=(a.at(9+extra)&255)|((a.at(10+extra)&255)<<8);
 s.weapon_level=s.armor_level=1;return s;
}
void position_deployment(DeploymentSlot& s,int x,int y,int direction,int hidden){if(s.id==65535||(s.flags&3)!=0)return;s.x=x&255;s.y=y&255;s.flags=facing(direction)|(hidden>0?4:0);}
void equipment_deployment(DeploymentSlot& s,const std::array<int,5>& a){s.equipment_overrides={std::max(0,a[0])&255,std::max(0,a[2])&255,std::max(0,a[4])&255};s.weapon_level=std::max(0,a[1])&255;s.armor_level=std::max(0,a[3])&255;}
}
