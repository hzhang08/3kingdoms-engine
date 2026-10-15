#include "caocao/combat_rules.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
int battle_stat(const BattleUnit& b,const UnitRecord& u,int index,const Json& data){
 if(index<0||index>=5)throw std::invalid_argument("invalid battle statistic");
 int value=std::min(index<3?400:300,u.stats[index]+equipment_bonus(u,index,data));
 if(b.grades[index]>=4)value=value*12/10;
 else if(b.grades[index]<=2)value=value*7/10;
 return std::min(400,value);
}
int battle_terrain_factor(const UnitRecord& u,int terrain,const Json& data){
 if(terrain<0||terrain>=30)throw std::invalid_argument("invalid combat terrain");
 int group=u.job<=38?u.job/3:u.job-26;if(group>=27)return 0;
 int factor=data.at("terrain_raw").at(group*60+terrain);return factor==255?0:factor;
}
int physical_hit_chance(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,const Json& data){
 int group=au.job<=38?au.job/3:au.job-26;
 if((group==2||group==4||group==5)&&equipment_effect(bu,56,data))return 0;
 if(b.status&8)return 100;
 int attack=battle_stat(a,au,3,data),defense=battle_stat(b,bu,3,data),chance;
 if(attack>=defense*2)chance=100;
 else if(attack>=defense)chance=90+(attack-defense)*10/defense;
 else if(attack>=defense/2){
  if(defense/2==0)throw std::runtime_error("original hit-chance division by zero");
  chance=60+(attack-defense/2)*30/(defense/2);
 }else{
  if(defense/3==0)throw std::runtime_error("original hit-chance division by zero");
  chance=30+std::max(0,attack-defense/3)*30/(defense/3);
 }
 if(auto effect=equipment_effect(au,39,data))chance=std::min(100,chance+*effect);
 if(auto effect=equipment_effect(bu,53,data))chance=std::max(0,chance-*effect);
 if(auto effect=equipment_effect(bu,55,data))chance=std::max(0,chance-*effect);
 return chance;
}
uint32_t physical_damage(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,
 int attack_terrain,int defense_terrain,bool randomize,bool counterattack,const Json& data,OriginalRng& rng){
 PhysicalDamageInputs in;
 in.attack=battle_stat(a,au,0,data);in.defense=battle_stat(b,bu,1,data);
 in.attack_terrain=battle_terrain_factor(au,attack_terrain,data);in.defense_terrain=battle_terrain_factor(bu,defense_terrain,data);
 in.level=au.level;in.attack_class_group=au.job<=38?au.job/3:au.job-26;in.defense_class_group=bu.job<=38?bu.job/3:bu.job-26;
 in.target_hp=b.hp;in.randomize=randomize;in.counterattack=counterattack;
 // Legacy arithmetic fixture name: effect 61 is resistance to ranged attacks.
 in.mounted_resistance_percent=equipment_effect(bu,61,data).value_or(0);
 in.anti_infantry_percent=equipment_effect(au,45,data).value_or(0);
 if(randomize)in.hit=rng.percent(physical_hit_chance(a,au,b,bu,data));
 return physical_damage_arithmetic(in,rng);
}
uint32_t physical_damage_arithmetic(const PhysicalDamageInputs& in, OriginalRng& rng) {
    if(in.attack>10000||in.defense>10000||in.attack_terrain>255||in.defense_terrain>255||in.level>255||in.attack_class_group>255||in.defense_class_group>255||in.mounted_resistance_percent>100||in.anti_infantry_percent>100)
        throw std::invalid_argument("physical damage inputs outside verified range");
    if(in.randomize&&!in.hit) return 0;
    const uint32_t attack=in.attack*in.attack_terrain/10;
    const uint32_t defense=in.defense*in.defense_terrain/10;
    auto subtract=[](uint32_t n,uint32_t amount){return n>amount?n-amount:0u;};
    uint32_t damage=in.level+25;
    if(attack>=defense)damage+=(attack-defense)/2;
    else damage=std::max(1u,subtract(damage,(defense-attack)/2));
    // The original switch uses class groups, not Data.e5's promotion class IDs.
    if(in.attack_class_group==1) {
        if(in.defense_class_group==3)damage=std::max(1u,damage*3/5);
        else if(in.defense_class_group==2||in.defense_class_group==5)damage=damage*3/2;
    } else if(in.attack_class_group==2||in.attack_class_group==4||in.attack_class_group==5) {
        if(in.defense_class_group==3||in.defense_class_group==16||in.defense_class_group==17)damage=damage*3/2;
        damage-=damage*in.mounted_resistance_percent/100;
    }
    if(in.defense_class_group==3)damage+=damage*in.anti_infantry_percent/100;
    if(in.counterattack)damage=std::max(1u,damage*3/4);
    if(in.randomize){auto n=rng.bounded(7);damage=n>2?damage+n-2:std::max(1u,subtract(damage,n));}
    return std::min(damage,in.target_hp);
}
}
