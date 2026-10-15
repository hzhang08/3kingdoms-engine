#include "caocao/ai.hpp"
#include "caocao/range.hpp"
#include "caocao/combat_rules.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
namespace {int group(int job){return job<=38?job/3:job-26;}}
PlayerPhaseGate player_phase_gate(const std::vector<BattleUnit>& roster){
 PlayerPhaseGate out;int first_slot=115;
 for(const auto& b:roster)if(b.lifecycle==2){
  if(b.slot<35&&b.ai==7&&!(b.flags&128))out.requires_input=true;
  // Party, then allies, then enemies: each group occupies consecutive slots.
  if(b.slot<first_slot){first_slot=b.slot;out.focus={b.x,b.y};}
 }
 return out;
}
bool ai_low_hp(const BattleUnit& b,const UnitRecord& u,const Json& data){
 if(!b.hp)return false;
 int maximum=std::min(500,u.hp+equipment_bonus(u,5,data));
 if(b.id<174)return b.hp<=maximum*2/5;
 // This edition tests job 49 but group 50, not job 50, in the second test.
 if(u.job==49||group(u.job)==50)return false;
 return b.hp<=maximum/5;
}
std::vector<int> battle_ai_order(const std::vector<BattleUnit>& roster,
 const std::vector<UnitRecord>& records,bool allied,int width,const Json& terrain,const Json& data){
 std::vector<const BattleUnit*> visible;
 for(const auto& b:roster)if(b.lifecycle==2&&(b.slot<35)==allied)visible.push_back(&b);
 std::sort(visible.begin(),visible.end(),[](auto a,auto b){return a->slot<b->slot;});
 auto priority=[&](const BattleUnit& b){
  int t=terrain.at(b.y*width+b.x);if(t>=18&&t<=22)return 0;
  auto r=std::find_if(records.begin(),records.end(),[&](const UnitRecord& u){return u.id==b.id;});
  if(r==records.end())throw std::runtime_error("AI priority missing persistent unit");
  if(ai_low_hp(b,*r,data))return 1;
  int g=group(r->job);
  if(g==8||g==10||g==11||g==18||g==19||g==20)return 2;
  if(g==2||g==4||g==5)return 3;
  if(g==0||g==3||g==11||g==13||g==17||g==20)return 4;
  return 5;
 };
 std::vector<int> out;
 for(int pass=0;pass<6;++pass)for(auto b:visible)if(priority(*b)==pass)out.push_back(b->slot);
 return out;
}
AiPreparation prepare_battle_ai(const BattleUnit& b,const UnitRecord& u,
 const std::vector<BattleUnit>& roster,int width,int height,const Json& terrain,
 const Json& data,std::vector<int> distance){
 if(width<1||height<1||terrain.size()!=static_cast<size_t>(width*height)||distance.size()!=terrain.size())throw std::runtime_error("invalid AI preparation grid");
 AiPreparation out;RangeGrids grids{std::move(distance),std::vector<int>(width*height,255),std::vector<int>(width*height)};
 int kind=battle_attack_range(u,data);std::vector<BattleUnit> sorted=roster;
 std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
 for(const auto& other:sorted)if(other.lifecycle==2&&(other.slot<35)!=(b.slot<35))grids=battle_range(other,{other.x,other.y},kind,0,false,255,width,height,std::move(grids),roster).grids;
 out.distance=std::move(grids.distance);out.threat=std::move(grids.threat);out.scores.assign(width*height,0);
 if(ai_low_hp(b,u,data)&&b.ai!=2&&b.ai!=4){
  for(size_t i=0;i<terrain.size();++i){int t=terrain.at(i);if(t>=18&&t<=22)out.scores[i]=50;}
  out.flags|=1;
 }
 return out;
}
AiAttackScore score_ai_attack(const BattleUnit& a,const UnitRecord& au,
 std::array<int,2> candidate,int priority_id,const std::vector<BattleUnit>& roster,
 const std::vector<UnitRecord>& records,int width,int height,const Json& terrain,const Json& data){
 if(terrain.size()!=static_cast<size_t>(width*height))throw std::runtime_error("invalid AI attack terrain");
 auto empty_grids=[&](){return RangeGrids{std::vector<int>(width*height,255),std::vector<int>(width*height,255),std::vector<int>(width*height,0)};};
 auto targets=battle_range(a,candidate,battle_attack_range(au,data),128,false,255,width,height,empty_grids(),roster).targets;
 AiAttackScore out;
 for(int slot:targets){
  auto target=std::find_if(roster.begin(),roster.end(),[&](const auto& b){return b.slot==slot;});
  if(target==roster.end())throw std::runtime_error("AI attack target missing");
  auto record=std::find_if(records.begin(),records.end(),[&](const auto& u){return u.id==target->id;});
  if(record==records.end())throw std::runtime_error("AI attack target record missing");
  const auto& b=*target;const auto& bu=*record;OriginalRng unused;
  // The original estimate reads current unit terrain, not the candidate tile.
  int damage=physical_damage(a,au,b,bu,terrain.at(a.y*width+a.x),terrain.at(b.y*width+b.x),false,false,data,unused);
  int maximum=std::min(500,bu.hp+equipment_bonus(bu,5,data)),score;
  if(damage>=b.hp)score=80;
  else{
   if(maximum==0)throw std::runtime_error("original AI damage-score division by zero");
   if(damage*10/maximum>=1||b.hp*100/maximum<=40)score=16;
   else score=a.ai==1?10:2;
  }
  if(b.status&8)score+=8;
  if(score>0&&b.id<174)score+=(b.id==priority_id||b.slot==35)?8:4;
  int g=group(au.job);if(score<75&&((g>=8&&g<=11)||(g>=18&&g<=20)))continue;
  int area=(au.job==16||au.job==17)?1:au.job==43?4:equipment_effect(au,43,data).value_or(0);
  if(area!=0&&area!=255)throw std::runtime_error("unverified area attack scoring (0x435aad/0x438d96)");
  if(score>0){
   auto mask=battle_range(b,{b.x,b.y},battle_attack_range(bu,data),128,true,255,width,height,empty_grids(),roster).grids.mask;
   if(mask.at(candidate[1]*width+candidate[0])==1&&score>1)--score;
  }
  // Strict improvement preserves the first enumerated target on ties.
  if(score>out.score){out.score=score;out.target=slot;}
 }
 return out;
}
}
