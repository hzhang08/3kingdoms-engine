#include "caocao/experience.hpp"
#include <stdexcept>
namespace caocao {
ExperienceChange set_character_experience(UnitRecord& u,uint32_t total,const Json& data){
 ExperienceChange out;
 if(u.id>=26||u.level>=50)return out;
 while(total>=100){grow_unit(u,1,data.at("classes"));out.levels.push_back(u.level);total-=100;}
 u.xp=u.level>=50?255:total;out.refresh=true;return out;
}
ExperienceChange add_equipment_experience(UnitRecord& u,int slot,uint32_t gain,const Json& data){
 ExperienceChange out;
 if(slot<0||slot>2)throw std::invalid_argument("invalid equipment XP slot");
 if(slot==2||u.equipment[slot][0]==255)return out;
 auto& item=u.equipment[slot];int type=data.at("items").at(item[0]).at("type");
 int maximum=type<18&&type%2==1?9:3;
 uint32_t total=uint32_t(item[2])+gain;
 while(total>=100&&item[1]<maximum){++item[1];out.levels.push_back(item[1]);total-=100;}
 item[2]=item[1]>=maximum?255:total&255;return out;
}
}
