#include "caocao/tactics.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/range.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
Json EnemyPoisonPlan::json()const{
 if(!cast)return nullptr;
 return {{"targets",Json::array({target})},{"damage",Json::array({damage})},
   {"effects",Json::array({effect})},{"experience",0},{"weapon_experience",0},
   {"armor_experience",Json::array({0})},{"rng",rng.state}};
}
EnemyPoisonPlan plan_enemy_poison(const BattleUnit& a,const UnitRecord& au,
 const BattleUnit& b,const UnitRecord& bu,OriginalRng rng,const Json& data){
 if(a.side==0||a.ai==7||b.side==0)
  throw std::runtime_error("party tactic experience is not yet verified (0x420efb)");
 EnemyPoisonPlan out;out.rng=rng;
 if(!tactic_target_usable(a,b,33))return out;
 out.cast=true;out.target=b.slot;
 out.effect=out.rng.percent(tactic_hit_chance(a,au,b,bu,33,data))?0x70:0;
 out.damage=tactic_damage(a,au,b,bu,33,true,data,out.rng);
 return out;
}
namespace {
void supported(int tactic){
 if(tactic!=0&&tactic!=5&&tactic!=10&&tactic!=15&&tactic!=23&&tactic!=25&&tactic!=33)
  throw std::runtime_error("unverified AI tactic scoring (0x4391dc, tactic "+std::to_string(tactic)+")");
}
}
bool tactic_target_usable(const BattleUnit& a,const BattleUnit& b,int tactic){
 supported(tactic);
 if(tactic==0||tactic==5||tactic==10||tactic==15)return true; // Terrain is checked by the action's target filter.
 // 0x420422 skips the status/grade restriction for player-controlled casters.
 if(a.ai==7)return true;
 if(tactic==23)return b.grades[3]!=2;
 if(tactic==25)return b.grades[4]!=2;
 return !(b.status&16);
}
bool tactic_eligible(const BattleUnit& a,const BattleUnit& b,int tactic,const Json& tactics){
 if(!tactic_target_usable(a,b,tactic))return false;
 // The three caster-usability branches at 0x41fec9 are unconditional true.
 int category=tactics.at(tactic).at(11);
 if(category==0)return (a.slot<35)!=(b.slot<35);
 if(category==1)return (a.slot<35)==(b.slot<35);
 return true;
}
int tactic_hit_chance(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,int tactic,const Json& data){
 if(!tactic_target_usable(a,b,tactic))return 0;
 if(b.status&8)return 100;
 int attack=battle_stat(a,au,2,data)+battle_stat(a,au,4,data);
 int defense=battle_stat(b,bu,2,data)+battle_stat(b,bu,4,data),chance;
 if(attack>=defense*2)chance=100;
 else if(attack>=defense)chance=90+(attack-defense)*10/defense;
 else if(attack>=defense/2)chance=60+(attack-defense/2)*30/(defense/2);
 else chance=30+std::max(0,attack-defense/3)*30/(defense/3);
 if(auto e=equipment_effect(au,52,data))chance=std::min(100,chance+*e);
 if(auto e=equipment_effect(bu,54,data))chance=std::max(0,chance-*e);
 if(auto e=equipment_effect(bu,55,data))chance=std::max(0,chance-*e);
 return tactic==0||tactic==5||tactic==10||tactic==15?chance:chance*9/10;
}
uint32_t tactic_damage(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,int tactic,bool randomize,const Json& data,OriginalRng& rng){
 supported(tactic);if(tactic==23||tactic==25)return 0;
 if(randomize&&!rng.percent(tactic_hit_chance(a,au,b,bu,tactic,data)))return 0;
 if(!tactic_target_usable(a,b,tactic))return 0;
 int attack=battle_stat(a,au,2,data),defense=battle_stat(b,bu,2,data);
 int damage=au.level+25;
 if(attack>=defense)damage+=(attack-defense)/3;
 else damage=std::max(0,damage-(defense-attack)/3);
 damage=damage*(tactic==10?5:7)/10;
 if(bu.job==39||bu.job==42||bu.job==43)damage=damage*11/10;
 else if(bu.job==49||bu.job==50)damage=damage*13/10;
 if(tactic==0)if(auto e=equipment_effect(au,47,data))damage+=damage*(*e)/100;
 if(tactic==10)if(auto e=equipment_effect(au,48,data))damage+=damage*(*e)/100;
 if(auto e=equipment_effect(bu,59,data))damage-=damage*(*e)/100;
 if(randomize){int n=rng.bounded(7);damage=n>2?damage+n-2:std::max(0,damage-n);}
 return std::min(std::max(1,damage),b.hp);
}
int score_ai_tactic_target(const BattleUnit& a,const UnitRecord& au,const BattleUnit& b,const UnitRecord& bu,int tactic,const Json& data,const Json& tactics,OriginalRng& rng){
 if(tactic!=23&&tactic!=25&&tactic!=33)throw std::runtime_error("unverified original AI elemental scoring");
 supported(tactic);
 int maximum=std::min(500,bu.hp+equipment_bonus(bu,5,data));
 if(maximum==0)throw std::runtime_error("original tactic-score division by zero");
 // The original switch explicitly returns zero for 23 and 25, despite
 // computing their area/eligibility first. Neither branch draws randomness.
 if(!tactic_eligible(a,b,tactic,tactics)||tactic==23||tactic==25)return 0;
 int damage=tactic_damage(a,au,b,bu,tactic,true,data,rng);
 if(damage>=b.hp)return 76;
 int fraction=damage*10/maximum,score=fraction>=3?30:fraction>=1?10:1;
 if(((b.hp*100/maximum)&255)>=40)score+=8;
 return score;
}
AiTacticScore score_ai_tactics(const BattleUnit& a,const UnitRecord& au,std::array<int,2> candidate,
 const std::vector<int>& available,const std::vector<BattleUnit>& roster,const std::vector<UnitRecord>& records,
 int width,int height,const Json& data,const Json& tactics,OriginalRng& rng){
 AiTacticScore out;
 for(int tactic:available){
  supported(tactic);
  if(tactics.at(tactic).at(12)!=13||tactics.at(tactic).at(13)!=(tactic==33?0:1))throw std::runtime_error("unverified tactic range/area");
  RangeGrids grids{std::vector<int>(width*height,255),std::vector<int>(width*height,255),std::vector<int>(width*height)};
  auto targets=battle_range(a,candidate,13|128,128,false,255,width,height,std::move(grids),roster).targets;
  for(int slot:targets){
   auto b=std::find_if(roster.begin(),roster.end(),[&](const auto& r){return r.slot==slot;});
   auto bu=std::find_if(records.begin(),records.end(),[&](const auto& r){return r.id==b->id;});
   if(bu==records.end())throw std::runtime_error("AI tactic target record missing");
   int score=std::min(255,score_ai_tactic_target(a,au,*b,*bu,tactic,data,tactics,rng));
   if(score>0&&slot==a.target_slot)score+=10;
   if(score>out.score){out.score=score&255;out.target=slot;out.tactic=tactic;}
  }
 }
 return out;
}
}
