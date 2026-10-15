#include "caocao/strike_application.hpp"
#include "caocao/experience.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
StrikeApplication apply_strike_target(const BattleUnit& actor,const UnitRecord& au,
 const StrikeTarget& target,uint32_t hp_loss,uint32_t mp_loss,uint32_t armor_xp,
 int area,int beans,const Json& data,OriginalRng& rng){
 if(area<0||area>255||beans<0||beans>255||target.battle.id!=target.unit.id||actor.id!=au.id)
  throw std::invalid_argument("invalid strike application input");
 OriginalRng next=rng;StrikeApplication out;out.target=target;
 auto& b=out.target.battle;auto& u=out.target.unit;
 if(hp_loss||mp_loss){
  for(const auto& rule:std::array<std::array<int,3>,3>{{{35,50,8},{36,49,16},{37,42,2}}}){
   if(equipment_effect(au,rule[0],data))b.status|=rule[2];
   // Class rolls still run when equipment or an existing status already
   // supplies the same bit. Short-circuiting here changes subsequent attacks.
   if(au.job==rule[1]&&next.percent(60))b.status|=rule[2];
  }
  if(equipment_effect(au,38,data))b.status|=4;
 }
 out.requests.push_back({{"kind","numbers"},{"target",b.slot},{"actor",actor.slot},
  {"hp",hp_loss},{"mp",mp_loss},{"xp",0},{"weapon_xp",0},{"armor_xp",armor_xp},
  {"wait",area==0},{"target_before",b.json()}});
 // The original subtracts uint32 values and then caps, even on malformed
 // overkill inputs. Verified strike planning ordinarily caps losses first.
 b.hp=std::min(uint32_t(b.hp)-hp_loss,uint32_t(std::min(500,u.hp+equipment_bonus(u,5,data))));
 b.mp=std::min(uint32_t(b.mp)-mp_loss,uint32_t(std::min(250,u.mp+equipment_bonus(u,6,data))));
 auto levels=add_equipment_experience(u,1,armor_xp,data);
 for(int level:levels.levels)out.requests.push_back({{"kind","equipment_level"},{"unit",u.id},{"slot",1},{"level",level}});
 if(hp_loss>0&&hp_loss<=0x7fffffff&&b.hp>0&&equipment_effect(u,62,data)&&(b.side!=0||beans>0)){
  out.pending_item=true;
  out.requests.push_back({{"kind","automatic_item"},{"item",87},{"target",b.slot},{"argument",0},{"free",b.side!=0}});
 }else b.flags&=~32;
 rng=next;return out;
}
}
