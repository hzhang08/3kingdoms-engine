#include "caocao/recovery.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
namespace {
bool recovery_equipment(const UnitRecord& u,const Json& data){
 for(int slot=0;slot<3;++slot){
  int id=u.equipment[slot][0];if(id==255)continue;
  const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2?type==20:(type<18&&type%2==1&&item.at("special_type")==20))return true;
 }
 return false;
}
int recovery_chance(const UnitRecord& u,const Json& data){
 // 0x41def0(0) reads the base luck byte at +0x25, not level-grown luck.
 return recovery_equipment(u,data)?100:u.base_stats[4];
}
}
RecoveryEffects recover_battle_grades(BattleUnit& b,const UnitRecord& u,OriginalRng& rng,const Json& data){
 RecoveryEffects out;if(b.lifecycle!=2)return out;
 int positive=u.base_stats[4]>50?10:60-u.base_stats[4],negative=recovery_chance(u,data);
 for(int n=0;n<6;++n){int& g=b.grades[n];
  if(g>=5)--g;
  else if(g<=1)++g;
  else if((g==4&&rng.percent(positive))||(g==2&&rng.percent(negative))){
   out.changes.push_back({b.slot,n,3});g=3;
  }
 }
 return out;
}
RecoveryEffects recover_battle_status(BattleUnit& b,const UnitRecord& u,
 const std::vector<BattleUnit>& roster,const std::vector<UnitRecord>& records,
 OriginalRng& rng,const Json& data){
 RecoveryEffects out;if(b.lifecycle!=2)return out;
 auto clear=[&](int mask){out.changes.push_back({b.slot,6,0x80|mask});b.status&=~(0x80|mask);};
 if(b.status){
  static constexpr int neighbors[8][2]={{0,-1},{1,0},{0,1},{-1,0},{1,-1},{1,1},{-1,1},{-1,-1}};
  for(const auto& d:neighbors){
   auto it=std::find_if(roster.begin(),roster.end(),[&](const BattleUnit& other){return other.lifecycle==2&&other.x==b.x+d[0]&&other.y==b.y+d[1];});
   if(it==roster.end()||(it->slot<35)!=(b.slot<35))continue;
   auto record=std::find_if(records.begin(),records.end(),[&](const UnitRecord& r){return r.id==it->id;});
   if(record==records.end())throw std::runtime_error("missing status-aura unit record");
   int group=record->job<=38?record->job/3:record->job-26;
   if(group==12)out.aura_slots.push_back(it->slot);
  }
  // Every eligible healer animates, in neighbor order. No random draw occurs.
  if(!out.aura_slots.empty()){clear(30);return out;}
 }
 int chance=recovery_chance(u,data);
 for(int mask:{2,4,8,16})if(b.status&mask){
  // Confusion is the special case: chance 100 does NOT consume an RNG draw.
  bool recovered=mask==8?(chance==100||rng.percent(std::max(10,chance-15))):rng.percent(chance);
  if(recovered)clear(mask);
 }
 return out;
}
}
namespace caocao {
namespace {
int effect_strength(const UnitRecord& u,int effect,const Json& data){
 int found=-1;
 for(int slot=0;slot<3;++slot){int id=u.equipment[slot][0];if(id==255)continue;
  const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2){if(type==effect)found=item.at("strength");}
  else if(type<18&&type%2==1&&item.at("special_type")==effect)found=item.at("special_strength");
 }
 return found; // Later equipment slots replace, rather than add, the value.
}
}
int terrain_recovery_percent(int terrain){
 static constexpr int rates[]{25,20,20,15,20};
 return terrain>=18&&terrain<=22?rates[terrain-18]:0;
}
int terrain_recovery_hp(int maximum_hp,int hp,int terrain){
 return hp>0?std::min(maximum_hp-hp,maximum_hp*terrain_recovery_percent(terrain)/100):0;
}
RegenerationAmounts battle_regeneration(const BattleUnit& b,const UnitRecord& u,int terrain,const Json& data){
 RegenerationAmounts out;if(b.lifecycle!=2)return out;
 int maximum_hp=std::min(500,u.hp+equipment_bonus(u,5,data));
 int maximum_mp=std::min(250,u.mp+equipment_bonus(u,6,data));
 out.hp=maximum_hp*terrain_recovery_percent(terrain)/100;
 int divisor=effect_strength(u,18,data);
 if(divisor==0)throw std::runtime_error("original HP-regeneration equipment divides by zero");
 if(divisor>0)out.hp+=maximum_hp/divisor;
 out.hp=std::min(out.hp,maximum_hp-b.hp);
 out.mp=std::max(0,effect_strength(u,19,data));out.mp=std::min(out.mp,maximum_mp-b.mp);
 if(b.side==0){int factor=std::max(10,equipment_bonus(u,8,data));
  out.xp=std::max(0,effect_strength(u,21,data))*factor/10;
  out.weapon_xp=std::max(0,effect_strength(u,22,data))*factor/10;
  out.armor_xp=std::max(0,effect_strength(u,23,data))*factor/10;
 }
 return out;
}
int battle_poison_loss(const BattleUnit& b,const UnitRecord& u,const Json& data){
 if(!(b.status&16))return 0;
 int maximum=std::min(500,u.hp+equipment_bonus(u,5,data));
 return std::min(std::max(1,maximum/10),b.hp-1);
}
}
