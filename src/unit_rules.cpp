#include "caocao/unit_rules.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
namespace caocao {
UnitRecord UnitRecord::from_json(const Json& j){
 UnitRecord u;
 #define FIELD(x) j.at(#x).get_to(u.x)
 FIELD(id);FIELD(owner);FIELD(level);FIELD(xp);FIELD(hp);FIELD(mp);FIELD(base_stats);FIELD(stats);FIELD(equipment);FIELD(record_counters);
 #undef FIELD
 j.at("class").get_to(u.job);u.validate();return u;
}
Json UnitRecord::json()const{return {{"id",id},{"owner",owner},{"class",job},{"level",level},{"xp",xp},{"hp",hp},{"mp",mp},{"base_stats",base_stats},{"stats",stats},{"equipment",equipment},{"record_counters",record_counters}};}
void UnitRecord::validate()const{
 auto byte=[](int n){return n>=0&&n<=255;};
 if(id<0||id>=512||!byte(owner)||job<0||job>=53||level<0||level>50||!byte(xp)||hp<0||hp>500||mp<0||mp>250)throw std::runtime_error("invalid persistent unit statistics");
 for(int n:base_stats)if(!byte(n))throw std::runtime_error("invalid base attribute");
 for(int n:stats)if(!byte(n))throw std::runtime_error("invalid derived attribute");
 for(int n:record_counters)if(!byte(n))throw std::runtime_error("invalid unit counters");
 for(const auto& e:equipment)if((e[0]!=255&&(e[0]<0||e[0]>=104))||!byte(e[1])||!byte(e[2]))throw std::runtime_error("invalid equipment record");
}
UnitRecord load_unit_record(const Json& h){
 UnitRecord u;u.id=h.at("id");u.job=h.at("class");u.level=h.at("initial_level");u.xp=h.at("initial_xp");u.hp=h.at("base_hp");u.mp=h.at("base_mp");u.base_stats=h.at("base_stats").get<std::array<int,5>>();u.stats=u.base_stats;
 u.owner=h.at("initial_owner");
 for(int i=0;i<3;++i){int id=h.at("initial_equipment").at(i);u.equipment[i]={id,id==255||i==2?255:1,id==255||i==2?255:0};}
 u.validate();return u;
}
void grow_unit(UnitRecord& u,int increment,const Json& classes){
 if(increment<0||increment>50)throw std::runtime_error("unsupported growth increment");
 increment=std::min(increment,50-u.level);const auto& c=classes.at(u.job);
 for(int i=0;i<5;++i){int attr=u.base_stats[i],grade=attr>=45?4:attr>=35?3:attr>=25?2:1;int rate=(c.at(i+2).get<int>()+grade)/2;u.stats[i]=std::min(255,u.stats[i]+rate*increment);}
 u.hp=std::min(500,u.hp+c.at(7).get<int>()*increment);u.mp=std::min(250,u.mp+c.at(8).get<int>()*increment);
 u.level+=increment;if(u.level>=50)u.xp=255;
}
void default_equipment(UnitRecord& u,bool lower_one,const Json& classes){
 int tier=std::min(u.level/12,2),level=u.level>=36?3:u.level%12/4+1;
 if(lower_one&&--level<1){if(tier==0)level=1;else{--tier;level=3;}}
 const auto& c=classes.at(u.job);
 for(int slot=0;slot<2;++slot){int begin=slot?14:0,end=slot?18:14;
  for(int type=begin;type<end;++type)if(c.at(9+type).get<int>()!=0){int id=(slot?38:0)+(type-begin)/2*3+tier;u.equipment[slot]={id,level,level>=3?255:0};break;}
 }
}
int original_party_average(std::vector<int> levels){
 if(levels.empty())return 1;
 // 0x40c14f reuses its accumulator for the sorting temporary and does not
 // reset it before summation. Preserve this edition's order-sensitive result.
 int sum=0;for(size_t i=0;i<levels.size();++i)for(size_t j=0;j+1<levels.size();++j)if(levels[j]<levels[j+1]){sum=levels[j];std::swap(levels[j],levels[j+1]);}
 int count=std::min<int>(15,levels.size());for(int i=0;i<count;++i)sum+=levels[i];return std::max(1,sum/count);
}
UnitRecord join_unit(const Json& hero,int average,int adjustment,const Json& classes){
 auto u=load_unit_record(hero);if(u.owner==0)return u;
 if(adjustment<0||adjustment>100)throw std::runtime_error("unsupported join adjustment");
 int target=std::max(1,adjustment<=50?std::min(average+adjustment,50):std::max(average-(adjustment-50),0));
 int promoted=u.job;if(promoted<=38)promoted+=target>=30?2:target>=15?1:0;
 u.level=0;grow_unit(u,target,classes);u.job=promoted;default_equipment(u,true,classes);u.owner=0;return u;
}
int equip_unit(UnitRecord& u,int slot,int encoded,int level,const Json& data){
 if(slot<0||slot>2)throw std::runtime_error("invalid equipment slot");
 int id=encoded+(slot==0?-2:slot==1?36:51);
 if(id<0||id>=static_cast<int>(data.at("items").size()))throw std::runtime_error("invalid encoded equipment ID");
 const auto& item=data.at("items").at(id);int type=item.at("type"),kind=type<14?0:type<18?1:type<63?2:3;
 if(kind!=slot)return -1;
 if(kind==2){int restriction=item.at("class_group");int group=u.job<=38?u.job/3:u.job-26;if(restriction!=255&&restriction!=group)return -1;}
 else if(data.at("classes").at(u.job).at(9+type).get<int>()==0)return -1;
 bool unique=kind<2&&type%2==1;int maximum=unique?9:3;
 if(level==0)level=unique?std::min(u.level/4+1,9):u.level>=36?3:u.level%12/4+1;
 else if(level!=255)level=std::min(level,maximum);
 u.equipment[slot]={id,slot==2?255:level,level>=maximum?255:0};
 return u.owner==0&&item.at("collection").get<int>()!=0?id:-1;
}
std::vector<int> script_equipment(UnitRecord& u,const std::array<int,5>& a,const Json& data){
 std::vector<int> collected;int weapon=a[0]<0?0:a[0];
 for(int slot=0;slot<3;++slot){int encoded=a[slot*2],level=slot==2?255:a[slot*2+1];if(encoded<0)encoded=0;if(level<0)level=0;
  if(encoded==0)continue;
  // The story handler checks the weapon argument for all three removal paths.
  if(weapon==1){u.equipment[slot]={255,255,255};continue;}
  int found=equip_unit(u,slot,encoded,level,data);if(found>=0)collected.push_back(found);
 }
 return collected;
}
void initialize_npc(UnitRecord& u,int level,int mode,const Json& data){
 if(level<1||level>50||mode<0||mode>2)throw std::runtime_error("invalid NPC initialization");
 if(u.id>=174)mode=0; // Campaign stage zero: 0x442890 overrides generic units.
 if(u.job<=38){if(mode==2&&level>=30)u.job+=2;else if(mode>=1&&level>=15)++u.job;}
 u.level=level;
 for(int i=0;i<7;++i){
  auto rate=[&](int job){int n=data.at("classes").at(job).at(i+2);if(i<5){int attr=u.base_stats[i];n=(n+(attr>=45?4:attr>=35?3:attr>=25?2:1))/2;}return n;};
  int remaining=level-1,total=0;
  // 0x4089cb: preserve the original promotion-segment arithmetic, including
  // its use of job-1 for both earlier segments of a twice-promoted unit.
  if(u.job<=38&&u.job%3==2&&remaining+1>=30){total+=rate(u.job-1)*15;remaining=std::max(0,remaining-15);}
  if(u.job<=38&&u.job%3!=0&&remaining+1>=15){total+=rate(u.job-1)*15;remaining=std::max(0,remaining-15);}
  total+=rate(u.job)*remaining;
  if(i<5)u.stats[i]=std::min(255,u.stats[i]+total);else if(i==5)u.hp=std::min(500,u.hp+total);else u.mp=std::min(250,u.mp+total);
 }
}
int equipment_bonus(const UnitRecord& u,int statistic,const Json& data){
 auto primary=[](int type){if(type<=9||type==24)return 0;if(type<14||type==25)return 2;if(type<18||type==26)return 1;if(type>=27&&type<=30)return type-24;if(type==32)return 7;if(type==31)return 8;return 255;};
 static constexpr int secondary[]{0,2,1,3,4,5,6,7,8};int result=0;
 for(int slot=0;slot<3;++slot){const auto& e=u.equipment[slot];if(e[0]==255)continue;const auto& item=data.at("items").at(e[0]);int type=item.at("type");
  if(primary(type)==statistic){int value=item.at("strength");if(slot<2&&type<18)value=std::min(255,value+item.at("class_group").get<int>()*std::max(0,e[1]-1));result+=value;}
  int special=item.at("special_type");if(slot<2&&type<18&&type%2==1&&special>=24&&special<=32&&secondary[special-24]==statistic)result+=item.at("special_strength").get<int>();
 }
 return result&255;
}
std::optional<int> equipment_effect(const UnitRecord& u,int effect,const Json& data){
 std::optional<int> result;
 for(int slot=0;slot<3;++slot){int id=u.equipment[slot][0];if(id==255)continue;
  const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2&&type==effect)result=item.at("strength");
  else if(slot<2&&type<18&&type%2==1&&item.at("special_type")==effect)result=item.at("special_strength");
 }
 return result;
}
int battle_sprite(int id,int job,int side){
 static constexpr int generic[]{5,5,5,0,1,2,6,7,8,3,4,5,9,10,11,18,19,20,12,13,14,15,16,17,21,22,23,24,25,26,27,28,29,23,23,23,14,14,14,33,34,32,31,30,23,23,39,35,36,37,38,68,69};
 int sprite=generic[job],tier=job%3;
 switch(id){case 0:sprite=40+tier;break;case 1:sprite=43+tier;break;case 2:sprite=46+tier;break;case 3:sprite=tier?77:57;break;case 15:sprite=73+tier;break;case 16:sprite=70+tier;break;case 17:sprite=49+tier;break;case 19:sprite=52+tier;break;case 32:sprite=tier?55:76;break;case 33:sprite=tier?78:58;break;case 34:sprite=59;break;case 35:case 170:case 171:case 172:case 173:sprite=56;break;case 72:sprite=79;break;case 76:case 99:case 100:sprite=60;break;case 77:sprite=63;break;case 81:sprite=65;break;case 101:sprite=67;break;case 119:sprite=66;break;case 149:sprite=68;break;case 162:case 169:sprite=82;break;case 163:sprite=80;break;case 164:sprite=81;break;case 165:sprite=64;break;case 166:case 167:case 168:sprite=61;break;default:break;}
 return sprite<34?sprite*3+std::min(side,2):sprite+68;
}
int movement_cost(const UnitRecord& u,int terrain,const Json& data){
 if(terrain<0||terrain>=30)throw std::runtime_error("invalid terrain ID");
 int group=u.job<=38?u.job/3:u.job-26;int cost=data.at("terrain_raw").at(group*60+30+terrain);
 bool reduced=false;for(int slot=0;slot<3;++slot){int id=u.equipment[slot][0];if(id==255)continue;const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2?type==34:(type<18&&type%2==1&&item.at("special_type")==34))reduced=true;
 }
 return cost!=255&&reduced?1:cost;
}
}
