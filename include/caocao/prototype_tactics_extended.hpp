#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Bounded original 0x420422/0x43b73f/0x43bfaa/0x43c548 semantics for
// IDs 0,2,5,7,10,15,17,20,23,25,33,35,39,61,63. Terrain is checked by caller.
bool extended_tactic_usable(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,int,const Json&);
int extended_tactic_hit(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,int,const Json&);
int extended_tactic_amount(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,int,bool,const Json&,OriginalRng&);
// Apply the successful grade/status component, separately from damage.
void extended_tactic_status(BattleUnit&,const UnitRecord&,int);
struct TacticExperience {int character=0,weapon=0,armor=0;};
TacticExperience extended_tactic_experience(const BattleUnit&,const UnitRecord&,const BattleUnit&,const UnitRecord&,int,int,int,const Json&);
}
