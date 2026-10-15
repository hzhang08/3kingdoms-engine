#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Recovered hit/damage also cover basic 0/5/10/15 elemental spells; callers
// must filter terrain first. Original AI scoring remains limited to 23/25/33.
// These evaluate an action; they do not apply HP, status, MP or presentation.
bool tactic_target_usable(const BattleUnit& actor,const BattleUnit& target,int tactic);
bool tactic_eligible(const BattleUnit& actor,const BattleUnit& target,int tactic,const Json& tactics);
int tactic_hit_chance(const BattleUnit&,const UnitRecord&,const BattleUnit&,
 const UnitRecord&,int tactic,const Json& data);
uint32_t tactic_damage(const BattleUnit&,const UnitRecord&,const BattleUnit&,
 const UnitRecord&,int tactic,bool randomize,const Json& data,OriginalRng&);
int score_ai_tactic_target(const BattleUnit&,const UnitRecord&,const BattleUnit&,
 const UnitRecord&,int tactic,const Json& data,const Json& tactics,OriginalRng&);
struct AiTacticScore {int score=0,target=255,tactic=255;};
AiTacticScore score_ai_tactics(const BattleUnit&,const UnitRecord&,std::array<int,2> candidate,
 const std::vector<int>& available,const std::vector<BattleUnit>&,const std::vector<UnitRecord>&,
 int width,int height,const Json& data,const Json& tactics,OriginalRng&);
struct EnemyPoisonPlan {
 bool cast=false;int target=255,effect=0;uint32_t damage=0;OriginalRng rng;
 Json json()const;
};
// 0x423549 up to 0x4214bf: independent status/damage rolls. Does not apply
// HP/status/MP or play the animation. Party/XP branches remain unsupported.
EnemyPoisonPlan plan_enemy_poison(const BattleUnit&,const UnitRecord&,
 const BattleUnit&,const UnitRecord&,OriginalRng,const Json& data);
}
