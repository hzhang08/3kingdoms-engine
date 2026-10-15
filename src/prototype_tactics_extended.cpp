#include "caocao/prototype.hpp"
#include "caocao/prototype_tactics_extended.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/experience.hpp"
#include "caocao/panel.hpp"
#include "caocao/range.hpp"
#include <algorithm>
#include <fstream>
#include <tuple>
namespace caocao {
namespace {
void supported(int t){switch(t){case 0:case 2:case 5:case 7:case 10:case 15:case 17:case 20:case 23:case 25:case 33:case 35:case 39:case 61:case 63:return;default:throw std::runtime_error("unverified extended tactic "+std::to_string(t));}}
int group(const UnitRecord& u){return u.job<=38?u.job/3:u.job-26;}
bool mental(const UnitRecord& u){int g=group(u);return (g>=8&&g<=11)||(g>=18&&g<=20);}
bool healing(int t){return t==39;}
bool support(int t){return t==61||t==63;}
bool status_spell(int t){return t==23||t==25||t==33||t==35;}
bool damaging(int t){return !healing(t)&&!support(t)&&t!=23&&t!=25;}
int maximum_hp(const UnitRecord& r,const Json& d){return std::min(500,r.hp+equipment_bonus(r,5,d));}
int factor(const UnitRecord& r,const Json& d){return std::max(10,equipment_bonus(r,8,d));}
bool terrain_ok(int t,int terrain){if(t>=20)return true;int category=t<5?0:t<10?1:t<15?3:2;return terrain_panel_info(terrain).tactics[category];}
BattleUnit battle(const Json& u){return BattleUnit::from_json(u.at("battle"));}
UnitRecord unit_record(const Json& u){return UnitRecord::from_json(u.at("record"));}
// Original 0x425349: center, N/E/S/W for area 1. Damage/status planning
// retains this order; 0x425452 subsequently sorts *application* row-major.
std::vector<int> targets(const Json& state,int actor,int target,int area,int tactic,const Json& terrain,int width){
 if(area<0||area>1)throw std::runtime_error("unverified extended tactic area");
 auto a=battle(state.at("units").at(std::to_string(actor))),center=battle(state.at("units").at(std::to_string(target)));
 std::vector<int> out;const int delta[][2]={{0,0},{0,-1},{1,0},{0,1},{-1,0}};
 for(int n=0;n<(area?5:1);++n)for(const auto& u:state.at("units")){
  auto b=battle(u);if(b.hp<=0||b.lifecycle!=2||b.x!=center.x+delta[n][0]||b.y!=center.y+delta[n][1])continue;
  if(((a.side==2)==(b.side==2))!=(healing(tactic)||support(tactic)))continue;
  if(terrain_ok(tactic,terrain.at("cells").at(b.y*width+b.x)))out.push_back(b.id);
 }
 return out;
}
}
bool extended_tactic_usable(const BattleUnit& a,const UnitRecord& ar,const BattleUnit& b,const UnitRecord& br,int t,const Json& data){
 supported(t);if(a.ai==7)return true;
 switch(t){case 20:return a.hp!=maximum_hp(ar,data);case 23:return b.grades[3]!=2;case 25:return b.grades[4]!=2;case 33:return !(b.status&16);case 35:return !(b.status&2);case 39:return b.hp!=maximum_hp(br,data);case 61:return b.grades[mental(br)?2:0]<4;default:return true;}
}
int extended_tactic_hit(const BattleUnit& a,const UnitRecord& ar,const BattleUnit& b,const UnitRecord& br,int t,const Json& data){
 if(!extended_tactic_usable(a,ar,b,br,t,data))return 0;if(b.status&8)return 100;
 int attack=battle_stat(a,ar,2,data)+battle_stat(a,ar,4,data),defense=battle_stat(b,br,2,data)+battle_stat(b,br,4,data),chance;
 if(attack>=defense*2)chance=100;
 else if(attack>=defense)chance=90+(attack-defense)*10/defense;
 else if(attack>=defense/2)chance=60+(attack-defense/2)*30/(defense/2);
 else chance=30+std::max(0,attack-defense/3)*30/(defense/3);
 if(auto e=equipment_effect(ar,52,data))chance=std::min(100,chance+*e);
 if(auto e=equipment_effect(br,54,data))chance=std::max(0,chance-*e);
 if(auto e=equipment_effect(br,55,data))chance=std::max(0,chance-*e);
 if(t==2||t==7||t==23||t==25||t==33)chance=chance*9/10;
 else if(t==35)chance=chance*4/5;
 else if(t==20)chance=chance*3/5;
 else if(t==39)chance=100;
 if(t==20&&group(ar)==12)chance=chance*3/2;
 return std::min(chance,100);
}
int extended_tactic_amount(const BattleUnit& a,const UnitRecord& ar,const BattleUnit& b,const UnitRecord& br,int t,bool randomize,const Json& data,OriginalRng& rng){
 supported(t);
 if(t==39){int amount=40+battle_stat(a,ar,2,data)/10;if(randomize&&battle_stat(b,br,4,data)>int(rng.bounded(200)))amount+=5+rng.bounded(6);return std::min(amount,maximum_hp(br,data)-b.hp);}
 if(!damaging(t))return 0;
 if(randomize&&!rng.percent(extended_tactic_hit(a,ar,b,br,t,data)))return 0;
 if(!extended_tactic_usable(a,ar,b,br,t,data))return 0;
 int attack=battle_stat(a,ar,2,data),defense=battle_stat(b,br,2,data),damage=ar.level+25;
 damage=attack>=defense?damage+(attack-defense)/3:std::max(0,damage-(defense-attack)/3);
 damage=damage*((t==2||t==7||t==10||t==35)?5:7)/10;
 if(br.job==39||br.job==42||br.job==43)damage=damage*11/10;else if(br.job==49||br.job==50)damage=damage*13/10;
 if(t==20&&group(ar)==12)damage*=2;
 if(t<5)if(auto e=equipment_effect(ar,47,data))damage+=damage*(*e)/100;
 if(t==10)if(auto e=equipment_effect(ar,48,data))damage+=damage*(*e)/100;
 if(auto e=equipment_effect(br,59,data))damage-=damage*(*e)/100;
 if(randomize){int n=rng.bounded(7);damage=n>2?damage+n-2:std::max(0,damage-n);}
 return std::min(std::max(1,damage),b.hp);
}
void extended_tactic_status(BattleUnit& b,const UnitRecord& br,int t){
 supported(t);auto grade=[&](int index,bool up){int& g=b.grades[index];g=up?(g>=3?6:3):(g<=3?0:3);};
 if(t==23)grade(3,false);else if(t==25)grade(4,false);else if(t==61)grade(mental(br)?2:0,true);
 else if(t==63){grade(mental(br)?2:0,true);grade(1,true);grade(3,true);grade(4,true);}
 else if(t==33)b.status|=16;else if(t==35)b.status|=2;
}
TacticExperience extended_tactic_experience(const BattleUnit& a,const UnitRecord& ar,const BattleUnit& b,const UnitRecord& br,int t,int amount,int priority,const Json& data){
 supported(t);TacticExperience out;int diff=br.level-ar.level;
 if(a.side==0||a.ai==7){
  if(damaging(t)&&amount>=b.hp){out.character=diff>0?std::min(200,32+8*diff):std::max(10,32+4*diff);if(br.id==priority)out.character*=2;}
  else out.character=diff>0?std::min(100,10+4*diff):std::max(5,10+diff/2);
  out.character=out.character*factor(ar,data)/10;
  int item=ar.equipment[0][0],type=item==255?-1:data.at("items").at(item).at("type").get<int>();
  if((type>=10&&type<=13)||type==25)out.weapon=(ar.level>br.level?2:3)*factor(ar,data)/10;
 }
 if(b.side==0&&damaging(t)&&amount)out.armor=(br.level>ar.level?3:4)*factor(br,data)/10;
 return out;
}
void Prototype::load_extended_tactics(const std::filesystem::path& assets){
 auto path=assets/"tactics-extended";if(!std::filesystem::exists(path/"manifest.json"))return;
 auto m=read_json(path/"manifest.json");extended_tactics_manifest_=m.at("hash");m.erase("hash");
 if(sha256(m.dump())!=extended_tactics_manifest_)throw std::runtime_error("extended tactics manifest mismatch");
 for(const auto& e:m.at("assets")){
  auto rel=std::filesystem::path(e.at("path").get<std::string>());if(rel.is_absolute()||rel.string().find("..")!=std::string::npos)throw std::runtime_error("invalid extended tactic path");
  std::ifstream f(path/rel,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};if(sha256(raw)!=e.at("sha256").get<std::string>())throw std::runtime_error("extended tactic checksum mismatch");
  if(rel=="catalog.json")extended_spell_catalog_=Json::parse(raw);
 }
}
Json Prototype::extended_spells(int id)const{
 Json out=Json::array();if(!tactics_enabled_||id<0||!state_.at("units").contains(std::to_string(id)))return out;
 if(extended_spell_catalog_.is_null())throw std::runtime_error("missing verified extended tactic assets");
 const auto& u=state_.at("units").at(std::to_string(id));auto b=battle(u);auto r=unit_record(u);if(b.hp<=0||b.lifecycle!=2||(b.status&4))return out;
 for(const auto& t:extended_spell_catalog_.at("tactics")){
  const auto& fields=t.at("fields");int learned=fields.at(17+r.job),cost=fields.at(14);if(!learned||learned>r.level||b.mp<cost)continue;
  if(auto divisor=equipment_effect(r,49,data_)){if(*divisor<=0)throw std::runtime_error("invalid MP discount");cost/=*divisor;}
  out.push_back({{"id",t.at("id")},{"name",t.at("name")},{"mp_cost",cost},{"range",fields.at(12)},{"area",fields.at(13)},{"category",fields.at(11)},{"effect",healing(t.at("id"))?"heal":support(t.at("id"))?"support":damaging(t.at("id"))?"damage":"debuff"}});
 }
 return out;
}
Json Prototype::extended_spell_options(int id,std::array<int,2> position)const{
 Json out=Json::array();auto available=extended_spells(id);if(available.empty())return out;
 auto a=battle(state_.at("units").at(std::to_string(id)));auto ar=unit_record(state_.at("units").at(std::to_string(id)));a.x=position[0];a.y=position[1];
 auto board=roster();for(auto& b:board)if(b.id==id){b.x=a.x;b.y=a.y;}
 for(const auto& t:available){int spell=t.at("id"),category=t.at("category"),area=t.at("area");
  auto range=battle_range(a,position,t.at("range"),128|(category==1?1:0),false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},board);
  for(const auto& u:state_.at("units")){
   auto b=battle(u);if(b.id==id){b.x=a.x;b.y=a.y;}
   if(b.hp<=0||std::find(range.targets.begin(),range.targets.end(),b.slot)==range.targets.end())continue;
   auto br=unit_record(u);if(!extended_tactic_usable(a,ar,b,br,spell,data_)||!terrain_ok(spell,terrain_.at("cells").at(b.y*map_width()+b.x)))continue;
   // AI should not spend MP on a heal or buff that cannot change the board.
   if(a.ai!=7&&((healing(spell)&&b.hp>=maximum_hp(br,data_))||(support(spell)&&b.grades[mental(br)?2:0]>=4)))continue;
   OriginalRng unused;int amount=extended_tactic_amount(a,ar,b,br,spell,false,data_,unused),hit=extended_tactic_hit(a,ar,b,br,spell,data_);
   auto staged=state_;staged["units"][std::to_string(id)]["battle"]["x"]=a.x;staged["units"][std::to_string(id)]["battle"]["y"]=a.y;
   auto affected=targets(staged,id,b.id,area,spell,terrain_,map_width());int total_damage=0,total_heal=0;
   for(int tid:affected){auto tb=battle(staged.at("units").at(std::to_string(tid)));auto tr=unit_record(staged.at("units").at(std::to_string(tid)));int value=extended_tactic_amount(a,ar,tb,tr,spell,false,data_,unused);if(healing(spell))total_heal+=value;else total_damage+=value;}
   out.push_back({{"kind","tactic"},{"x",a.x},{"y",a.y},{"target",b.id},{"targets",affected},{"tactic",spell},{"name",t.at("name")},{"mp_cost",t.at("mp_cost")},{"hit_percent",support(spell)?100:hit},{"damage_on_hit",healing(spell)?0:amount},{"healing_on_hit",healing(spell)?amount:0},{"total_damage_on_hit",total_damage},{"total_healing_on_hit",total_heal},{"effect",t.at("effect")},{"area",area},{"poison_percent",spell==33?hit:0},{"status_percent",status_spell(spell)?hit:0}});
  }
 }
 return out;
}
void Prototype::extended_spell_attack(int id,int tactic,int target){
 auto a=battle(state_.at("units").at(std::to_string(id)));auto ar=unit_record(state_.at("units").at(std::to_string(id)));Json choice=nullptr;
 for(const auto& o:extended_spell_options(id,{a.x,a.y}))if(o.at("tactic")==tactic&&o.at("target")==target){choice=o;break;}
 if(choice.is_null())throw std::runtime_error("策略目标、地形或策略值无效。");
 // Plan and commit a candidate only: unsupported resources or malformed data
 // cannot consume MP/RNG or partially alter an area spell.
 auto next=state_,visual=state_;visual["selected"]=-1;visual["undo"]=nullptr;OriginalRng rng{state_.at("rng").get<uint32_t>()};
 auto center=battle(next.at("units").at(std::to_string(target)));a.direction=battle_direction(a,center);visual["units"][std::to_string(id)]["battle"]["direction"]=a.direction;
 Json resolutions=Json::array();int xp=0,weapon_xp=0,total_damage=0,total_heal=0;bool any_poison=false;
 for(int tid:choice.at("targets").get<std::vector<int>>()){
  auto b=battle(state_.at("units").at(std::to_string(tid)));auto br=unit_record(state_.at("units").at(std::to_string(tid)));
  bool status=support(tactic)||(status_spell(tactic)&&rng.percent(extended_tactic_hit(a,ar,b,br,tactic,data_)));
  int amount=extended_tactic_amount(a,ar,b,br,tactic,true,data_,rng);auto gain=extended_tactic_experience(a,ar,b,br,tactic,amount,0,data_);xp=std::max(xp,gain.character);weapon_xp=std::max(weapon_xp,gain.weapon);
  if((tactic==33||tactic==35)&&amount>=b.hp)status=false;
  if(status)extended_tactic_status(b,br,tactic);
  if(healing(tactic)){b.hp+=amount;total_heal+=amount;}else{b.hp-=amount;total_damage+=amount;}
  if(!b.hp)b.lifecycle=3;
  if(damaging(tactic)&&amount>0&&b.hp>0&&equipment_effect(br,62,data_)&&(b.side!=0||inventory().at("87").get<int>()>0))throw std::runtime_error("automatic healing item controller requires original evidence (0x41dc43)");
  add_equipment_experience(br,1,gain.armor,data_);
  next["units"][std::to_string(tid)]["battle"]=b.json();next["units"][std::to_string(tid)]["record"]=br.json();
  resolutions.push_back({{"target",tid},{"amount",amount},{"damage",healing(tactic)?0:amount},{"healing",healing(tactic)?amount:0},{"status_hit",status},{"armor_xp",gain.armor}});any_poison|=status&&tactic==33;
 }
 // 0x425452 sorts application after all per-target RNG has been consumed.
 std::sort(resolutions.begin(),resolutions.end(),[&](const auto& x,const auto& y){auto bx=battle(state_.at("units").at(std::to_string(x.at("target").template get<int>()))),by=battle(state_.at("units").at(std::to_string(y.at("target").template get<int>())));return std::tie(bx.y,bx.x)<std::tie(by.y,by.x);});
 // Self spells/heals must retain their just-applied state before MP/XP changes.
 a=battle(next.at("units").at(std::to_string(id)));ar=unit_record(next.at("units").at(std::to_string(id)));a.direction=battle_direction(a,center);a.mp-=choice.at("mp_cost").get<int>();
 int drain=0;if(tactic==20){drain=std::min(total_damage,maximum_hp(ar,data_)-a.hp);a.hp+=drain;}
 auto levels=set_character_experience(ar,ar.xp+xp,data_);add_equipment_experience(ar,0,weapon_xp,data_);
 next["units"][std::to_string(id)]["battle"]=a.json();next["units"][std::to_string(id)]["record"]=ar.json();next["units"][std::to_string(id)]["max_hp"]=maximum_hp(ar,data_);next["rng"]=rng.state;
 Json events=Json::array();auto wait=[&](int n){events.push_back({{"kind","wait"},{"ticks",n}});};wait(8);events.push_back({{"kind","sound"},{"id",8}});
 events.push_back({{"kind","frame"},{"x",a.x*48-8},{"y",a.y*48+48},{"width",64},{"height",64},{"slot",a.slot},{"archive","UNIT_ATK.E5"},{"sprite",battle_sprite(a.id,ar.job,a.side)},{"frame",a.direction==0?4:a.direction==2?0:8},{"mirror",a.direction==1},{"effects",Json::array()}});events.push_back({{"kind","sound"},{"id",39}});wait(20);events.push_back({{"kind","overlay"},{"slot",a.slot},{"x",a.x*48},{"y",a.y*48+56}});
 const auto& effect=extended_spell_catalog_.at("effects").at(std::to_string(tactic));int w=effect.at("width"),h=effect.at("height");
 for(const auto& hit:resolutions){int tid=hit.at("target");auto b=battle(state_.at("units").at(std::to_string(tid)));auto br=unit_record(state_.at("units").at(std::to_string(tid)));bool reacted=false;events.push_back({{"kind","sound"},{"id",effect.at("sound")}});
  for(const auto& f:effect.at("frames")){
   if(f.at("callback")==1){reacted=true;
    if(damaging(tactic))events.push_back({{"kind","frame"},{"x",b.x*48},{"y",b.y*48+56},{"width",48},{"height",48},{"slot",b.slot},{"archive","UNIT_SPC.E5"},{"sprite",battle_sprite(b.id,br.job,b.side)},{"frame",std::array<int,4>{1,2,0,2}[b.direction]},{"mirror",b.direction==1},{"effects",Json::array()}});
    if(hit.at("amount").get<int>()||damaging(tactic)||healing(tactic))events.push_back({{"kind","number"},{"slot",b.slot},{"value",hit.at("amount").get<int>()},{"healing",healing(tactic)}});
   }
   events.push_back({{"kind","spell_effect"},{"asset_group","tactics-extended"},{"resource",effect.at("resource")},{"image",f.at("image")},{"weight",f.at("weight")},{"amount",f.at("amount")},{"x",b.x*48+24-w/2+f.at("dx").get<int>()},{"y",b.y*48+80-h/2+f.at("dy").get<int>()},{"width",w},{"height",h}});wait(effect.at("ticks"));
  }
  if(!reacted)throw std::runtime_error("extended spell resource has no callback");events.push_back({{"kind","spell_clear"}});wait(10);
  events.push_back({{"kind","unit_state"},{"actor",tid},{"battle",next.at("units").at(std::to_string(tid)).at("battle")}});
  events.push_back({{"kind","record_state"},{"actor",tid},{"record",next.at("units").at(std::to_string(tid)).at("record")},{"max_hp",next.at("units").at(std::to_string(tid)).at("max_hp")}});
 }
 if(drain){events.push_back({{"kind","number"},{"slot",a.slot},{"value",drain},{"healing",true}});wait(10);}
 events.push_back({{"kind","unit_state"},{"actor",id},{"battle",a.json()}});
 events.push_back({{"kind","record_state"},{"actor",id},{"record",ar.json()},{"max_hp",maximum_hp(ar,data_)}});
 if(xp||weapon_xp){events.push_back({{"kind","notice"},{"actor",id},{"text",state_.at("units").at(std::to_string(id)).at("name").get<std::string>()+" 经验 +"+std::to_string(xp)+"　武器经验 +"+std::to_string(weapon_xp)}});wait(18);}
 for(int level:levels.levels){events.push_back({{"kind","notice"},{"actor",id},{"text",state_.at("units").at(std::to_string(id)).at("name").get<std::string>()+"升至 "+std::to_string(level)+" 级！"}});wait(25);}
 events.push_back({{"kind","notice_clear"}});
 attack_={{"kind","tactic"},{"actor",id},{"target",target},{"tactic",tactic},{"name",choice.at("name")},{"damage",total_damage},{"healing",total_heal},{"drain",drain},{"poison",any_poison},{"targets",resolutions},{"experience",xp},{"weapon_experience",weapon_xp},{"levels",levels.levels},{"mp_cost",choice.at("mp_cost")},{"before",visual},{"events",events}};
 next["message"]=state_.at("units").at(std::to_string(id)).at("name").get<std::string>()+"使用"+choice.at("name").get<std::string>()+(healing(tactic)?"：恢复"+std::to_string(total_heal)+"点体力。":damaging(tactic)?"："+std::to_string(total_damage)+"点伤害。":"。");state_=std::move(next);finish(id);
}
}
