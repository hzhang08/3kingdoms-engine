#pragma once
#include "caocao/strike.hpp"
namespace caocao {
struct StrikeApplication {
 StrikeTarget target;
 Json requests=Json::array();
 // When true, target is suspended BEFORE original 0x41dc43. The caller must
 // execute that item and only then clear flag 32; it cannot proceed to the
 // next target or double/counter gate using this partial result.
 bool pending_item=false;
};
// Original 0x405889, one ordered target. Numerical changes and presentation
// requests are separate. No item effect, animation, death or event processing
// is implied. Inputs are untouched and RNG commits only on success.
StrikeApplication apply_strike_target(const BattleUnit& actor,const UnitRecord&,
 const StrikeTarget&,uint32_t hp_loss,uint32_t mp_loss,uint32_t armor_xp,
 int area,int beans,const Json& data,OriginalRng&);
}
