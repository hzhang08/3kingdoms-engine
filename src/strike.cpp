#include "caocao/strike.hpp"
#include "caocao/range.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
namespace {
int ratio_chance(int attack,int defense){
 if(attack>=defense*3)return 100;
 if(attack>=defense*2)return 20+(attack-defense*2)*80/defense;
 if(attack>=defense)return 2+(attack-defense)*18/defense;
 return 1;
}
uint32_t experience_factor(const UnitRecord& u,const Json& data){return std::max(10,equipment_bonus(u,8,data));}
uint32_t hit_experience(const UnitRecord& a,const StrikeTarget& t,uint32_t hp_damage,int priority_id,const Json& data){
 int diff=t.unit.level-a.level,value;
 if(hp_damage>=uint32_t(t.battle.hp)){
  value=diff>0?std::min(200,32+diff*8):std::max(10,32+diff*4);
  if(t.unit.id==priority_id)value*=2;
 }else value=diff>0?std::min(100,8+diff*2):std::max(1,8+diff);
 return value*experience_factor(a,data)/10;
}
void validate_round(int round,int counter_depth){
 if(round<0||round>1||counter_depth<0||counter_depth>2)throw std::invalid_argument("invalid physical strike round/counter depth");
}
}
Json StrikePlan::json()const{return {{"hp_damage",hp_damage},{"mp_damage",mp_damage},{"armor_xp",armor_xp},{"xp",xp},{"weapon_xp",weapon_xp},{"critical",critical}};}
int physical_critical_chance(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,const Json& data){
 if(equipment_effect(au,41,data))return 100;
 return ratio_chance(battle_stat(a,au,4,data),battle_stat(b,bu,4,data));
}
int physical_double_chance(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,const Json& data){
 return ratio_chance(battle_stat(a,au,3,data),battle_stat(b,bu,3,data));
}
std::optional<int> physical_critical_speech(int unit,int speech_group,bool critical,OriginalRng& rng){
 if(unit<0||unit>=512||speech_group<0||speech_group>255)throw std::invalid_argument("invalid critical speech unit/group");
 if(!critical||!rng.percent(25))return std::nullopt;
 // Edition table at 0x48b230. Named heroes use no second random draw.
 static constexpr int heroes[]{0,1,2,3,6,7,9,13,15,16,17,33,34,36,57,84,95,108,109,119,32};
 for(int n=0;n<21;++n)if(heroes[n]==unit)return 700+n;
 return 721+speech_group*3+rng.bounded(3);
}
StrikePlan plan_physical_strike(const BattleUnit& a,const UnitRecord& au,int terrain,
 const std::vector<StrikeTarget>& targets,int round,int counter_depth,int priority_id,
 uint32_t previous_xp,uint32_t previous_weapon_xp,const Json& data,OriginalRng& rng){
 validate_round(round,counter_depth);
 if(targets.empty()||targets.size()>115)throw std::invalid_argument("invalid physical strike targets");
 // Fail atomically on malformed inputs, including original divide-by-zero
 // edges. No half-consumed RNG is exposed to the engine.
 OriginalRng next=rng;StrikePlan result;result.xp=previous_xp;result.weapon_xp=previous_weapon_xp;
 for(size_t i=0;i<targets.size();++i){
  const auto& t=targets[i];const auto& b=t.battle;const auto& bu=t.unit;
  auto hp=physical_damage(a,au,b,bu,terrain,t.terrain,true,counter_depth!=0,data,next);uint32_t mp=0;
  // The original still checks critical after a miss, but only for target 0
  // and only while its HP is nonzero. This ordering affects later RNG draws.
  if(i==0&&b.hp>0&&next.percent(physical_critical_chance(a,au,b,bu,data))){
   hp=equipment_effect(bu,57,data)?0:hp*3/2;result.critical=true;
  }
  if(round>0&&equipment_effect(bu,58,data))hp=0;
  if(equipment_effect(bu,60,data)&&b.mp>0){mp=std::min(hp,uint32_t(b.mp));hp=0;}
  else hp=std::min(hp,uint32_t(b.hp));
  result.hp_damage.push_back(hp);result.mp_damage.push_back(mp);
  if(a.side==0){
   result.xp=std::max(result.xp,hit_experience(au,t,hp,priority_id,data));
   int item=au.equipment[0][0];
   if(item==255)throw std::runtime_error("physical strike with no weapon is not verified");
   int type=data.at("items").at(item).at("type");
   if(type<=9||type==24){
    uint32_t gain=(hp||mp)?(au.level>bu.level?2:3):1;
    result.weapon_xp=std::max(result.weapon_xp,gain*experience_factor(au,data)/10);
   }
  }
  uint32_t armor=0;
  if(b.side==0){armor=(hp||mp)?(au.level<bu.level?3:4):1;armor=armor*experience_factor(bu,data)/10;}
  result.armor_xp.push_back(armor);
 }
 rng=next;return result;
}
bool roll_another_strike(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,
 int round,int counter_depth,const Json& data,OriginalRng& rng){
 validate_round(round,counter_depth);
 return counter_depth==0&&round==0&&rng.percent(physical_double_chance(a,au,b,bu,data));
}
CounterPlan plan_counterattack(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,
 int counter_depth,int width,int height,const std::vector<BattleUnit>& roster,const Json& data){
 validate_round(0,counter_depth);
 if(width<1||width>40||height<1||height>40)throw std::invalid_argument("invalid counterattack board dimensions");
 if(counter_depth>1||(counter_depth>0&&!equipment_effect(bu,40,data)))return {false,0};
 CounterPlan plan{false,counter_depth};
 if(b.hp==0||(b.status&8)||equipment_effect(au,44,data))return plan;
 RangeGrids grids{std::vector<int>(width*height),std::vector<int>(width*height),std::vector<int>(width*height)};
 auto range=battle_range(b,{b.x,b.y},battle_attack_range(bu,data),0,false,a.slot,width,height,std::move(grids),roster);
 if(range.selected==a.slot){plan.attack=true;++plan.depth;}
 return plan;
}
}
