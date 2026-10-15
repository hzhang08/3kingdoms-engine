#include "caocao/prototype.hpp"
#include "caocao/ai.hpp"
#include "caocao/recovery.hpp"
#include "caocao/experience.hpp"
#include <algorithm>
#include <set>
namespace caocao {
void Prototype::enable_revised_rules(){
 if(revised_rules_){new_revised_rules_=true;return;}
 enable_items();
 if(extended_spell_catalog_.is_null()||combat_text_.is_null())throw std::runtime_error("missing revised battle assets");
 revised_rules_=new_revised_rules_=true;initialize(state_.at("rng"),false,scenario_);
}
void Prototype::maintenance_revised(bool enemy){
 auto board=roster();std::sort(board.begin(),board.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
 std::vector<UnitRecord> records;for(const auto& u:state_.at("units"))records.push_back(UnitRecord::from_json(u.at("record")));
 OriginalRng rng{state_.at("rng").get<uint32_t>()};
 for(auto b:board){
  if((b.slot>=35)!=enemy||b.lifecycle!=2||b.hp<=0)continue;
  auto& u=state_["units"][std::to_string(b.id)];auto r=UnitRecord::from_json(u.at("record"));auto before=b;
  recover_battle_status(b,r,board,records,rng,data_);recover_battle_grades(b,r,rng,data_);
  b.hp-=battle_poison_loss(b,r,data_);
  auto amounts=battle_regeneration(b,r,terrain_.at("cells").at(b.y*map_width()+b.x),data_);
  b.hp+=amounts.hp;b.mp+=amounts.mp;
  if(amounts.xp)set_character_experience(r,r.xp+amounts.xp,data_);
  if(amounts.weapon_xp)add_equipment_experience(r,0,amounts.weapon_xp,data_);
  if(amounts.armor_xp)add_equipment_experience(r,1,amounts.armor_xp,data_);
  if(before.json()!=b.json()||r.json()!=u.at("record")){
   std::string label=u.at("name").get<std::string>()+" 回合恢复";
   if(before.status!=b.status)label+=" 状态解除";
   if(b.hp!=before.hp)label+=" 兵力 "+std::to_string(b.hp-before.hp);
   if(b.mp!=before.mp)label+=" 策略值 +"+std::to_string(b.mp-before.mp);
   environment_event(b.id,{{"kind","notice"},{"actor",b.id},{"text",label}});
   if(b.hp!=before.hp)environment_event(b.id,{{"kind","number"},{"slot",b.slot},{"value",std::abs(b.hp-before.hp)},{"healing",b.hp>before.hp}});
   environment_event(b.id,{{"kind","wait"},{"ticks",12}});
   u["battle"]=b.json();u["record"]=r.json();u["max_hp"]=std::min(500,r.hp+equipment_bonus(r,5,data_));
   environment_event(b.id,{{"kind","unit_state"},{"actor",b.id},{"battle",b.json()}});
   environment_event(b.id,{{"kind","record_state"},{"actor",b.id},{"record",r.json()},{"max_hp",u.at("max_hp")}});
   environment_event(b.id,{{"kind","notice_clear"}});
  }
 }
 state_["rng"]=rng.state;
}
void Prototype::begin_phase_revised(const std::string& phase){
 state_["phase"]=phase;state_["selected"]=-1;state_["undo"]=nullptr;state_["enemy_cursor"]=0;npc_order_.clear();clear_enemy_choice();
 for(auto& u:state_["units"]){auto b=BattleUnit::from_json(u.at("battle"));if((phase=="player"&&b.side==0)||(phase=="ally"&&b.side==1)||(phase=="enemy"&&b.side>=2))u["battle"]["flags"]=b.flags&~2;}
 state_["message"]=phase=="player"?"我军回合。":phase=="ally"?"友军行动中……":"敌军行动中……";
 if(phase!="ally")maintenance_revised(phase=="enemy");
 outcome();queue_dialogues();
 if(state_.at("phase")!=phase||phase=="player")return;
 npc_order_=current_npc_order();
}
std::vector<int> Prototype::current_npc_order()const{
 std::string phase=state_.at("phase");std::vector<int> order;
 auto units=roster();std::vector<UnitRecord> records;for(const auto& u:state_.at("units"))records.push_back(UnitRecord::from_json(u.at("record")));
 for(int slot:battle_ai_order(units,records,phase=="ally",map_width(),terrain_.at("cells"),data_)){
  auto it=std::find_if(units.begin(),units.end(),[&](const auto& b){return b.slot==slot;});
  if(it!=units.end()&&(phase=="ally"?it->side==1:it->side>=2)&&it->lifecycle==2)order.push_back(it->id);
 }
 return order;
}

void Prototype::npc_step_revised(){
 attack_=nullptr;movement_=nullptr;environment_=nullptr;
 std::string phase=state_.at("phase");if((phase!="enemy"&&phase!="ally")||has_dialogue()||!story_choices().empty())return;
 if(state_.at("enemy_cursor")==0)npc_order_=current_npc_order();
 Json chosen=nullptr;
 if(phase=="enemy"){
  auto request=enemy_request();
  if(!request.is_null()){
   auto key=!enemy_choice_.is_null()&&request.at("key")==enemy_choice_.at("key")?enemy_choice_.at("choice").get<std::string>():"a0";
   chosen=request.at("choices").at(key);
  }clear_enemy_choice();
 }
 int cursor=state_.at("enemy_cursor");
 while(cursor<int(npc_order_.size())){
  int id=npc_order_[cursor++];state_["enemy_cursor"]=cursor;auto b=BattleUnit::from_json(state_.at("units").at(std::to_string(id)).at("battle"));
  if(b.lifecycle!=2||b.hp<=0||(b.flags&2)||(phase=="ally"?b.side!=1:b.side<2))continue;
  if(b.status&8){finish(id);state_["message"]="部队混乱，无法行动。";return;}
  auto plan=chosen.is_null()?suggest(id):chosen;
  if(!npc_engaged(id))plan={{"x",b.x},{"y",b.y},{"target",-1}};
  movement_=movement_packet(id,plan.at("x"),plan.at("y"));auto next=b;next.x=plan.at("x");next.y=plan.at("y");if(next.x!=b.x||next.y!=b.y)next.direction=battle_direction(b,next);
  state_["units"][std::to_string(id)]["battle"]=next.json();state_["inspected"]=id;
  if(plan.value("kind",std::string())=="tactic")spell_attack(id,plan.at("tactic"),plan.at("target"));
  else if(plan.at("target")!=-1)attack(id,plan.at("target"));else finish(id);
  return;
 }
 if(phase=="ally"){begin_phase_revised("enemy");return;}
 if(state_.at("turn").get<int>()>=20){state_["turn_limit_reached"]=true;outcome();return;}
 state_["turn"]=state_.at("turn").get<int>()+1;begin_phase_revised("player");
}
void Prototype::validate_revised_record(const Json& raw,const Json& base,bool player)const{
 auto r=UnitRecord::from_json(raw),expected=UnitRecord::from_json(base);
 if(raw.dump()!=r.json().dump()||raw.size()!=base.size()||r.id!=expected.id||r.owner!=expected.owner||r.job!=expected.job||r.base_stats!=expected.base_stats||r.level<expected.level)throw std::runtime_error("invalid saved unit identity");
 grow_unit(expected,r.level-expected.level,data_.at("classes"));
 if(r.hp!=expected.hp||r.mp!=expected.mp||r.stats!=expected.stats||(r.level<50?r.xp>99:r.xp!=255))throw std::runtime_error("invalid saved level growth");
 for(int i=0;i<3;++i){const auto& e=r.equipment[i];const auto& start=expected.equipment[i];
  if(equipment_enabled_&&player){validate_equipment_triple(raw.at("equipment").at(i),i);if(e[0]!=255){int type=data_.at("items").at(e[0]).at("type"),group=r.job<=38?r.job/3:r.job-26;int restriction=data_.at("items").at(e[0]).at("class_group");if(i==2?(restriction!=255&&restriction!=group):data_.at("classes").at(r.job).at(9+type)==0)throw std::runtime_error("incompatible saved equipment");}continue;}
  if(e[0]!=start[0])throw std::runtime_error("invalid saved equipment identity");
  if(i==2||e[0]==255){if(e!=start)throw std::runtime_error("invalid saved accessory");continue;}
  int cap=data_.at("items").at(e[0]).at("type").get<int>()%2?9:3;
  if(e[1]<start[1]||e[1]>cap||(e[1]==cap?e[2]!=255:e[2]>99))throw std::runtime_error("invalid saved equipment experience");
 }
}
void Prototype::validate_revised_state(const Json& s)const{
 auto integer=[](const Json& x,int lo,int hi){return x.is_number_integer()&&x>=lo&&x<=hi;};
 if(!s.is_object()||s.size()!=initial_.size()||s.at("units").size()!=initial_.at("units").size()||!integer(s.at("turn"),scenario_?1:2,20)||!integer(s.at("enemy_cursor"),0,int(npc_order_.size()))||!s.at("rng").is_number_integer()||s.at("rng")<0||s.at("rng")>UINT32_MAX||!s.at("message").is_string()||s.at("message").get_ref<const std::string&>().size()>4096||!s.at("turn_limit_reached").is_boolean())throw std::runtime_error("invalid revised battle snapshot");
 std::string phase=s.at("phase");if(phase!="player"&&phase!="ally"&&phase!="enemy"&&phase!="victory"&&phase!="defeat")throw std::runtime_error("invalid saved phase");
 std::set<int> occupied;
 for(auto it=initial_.at("units").begin();it!=initial_.at("units").end();++it){
  const auto& u=s.at("units").at(it.key());const auto& original=it.value();
  if(u.size()!=original.size()||u.at("name")!=original.at("name"))throw std::runtime_error("invalid saved unit");
  validate_revised_record(u.at("record"),original.at("record"),original.at("battle").at("side")==0);auto r=UnitRecord::from_json(u.at("record"));auto b=BattleUnit::from_json(u.at("battle"));auto baseline=BattleUnit::from_json(original.at("battle"));b.validate(r,data_,map_width(),map_height());
  if(u.at("battle").dump()!=b.json().dump()||u.at("battle").size()!=original.at("battle").size()||b.id!=baseline.id||b.slot!=baseline.slot||b.side!=baseline.side||u.at("max_hp")!=std::min(500,r.hp+equipment_bonus(r,5,data_)))throw std::runtime_error("invalid saved deployment identity");
  if(hulao_duel_){
   if(!duel_includes(b.id)&&b.lifecycle!=3)throw std::runtime_error("Non-duel unit in save");
   if(duel_includes(b.id)&&((b.hp>0)!=(b.lifecycle==2)||b.ai!=(b.id==duel_enemy_?1:7)))throw std::runtime_error("Invalid duelist deployment");
  }
  if(b.lifecycle==2&&(b.hp<=0||!occupied.insert(b.y*map_width()+b.x).second||movement_cost(r,terrain_.at("cells").at(b.y*map_width()+b.x),data_)==255))throw std::runtime_error("invalid active deployment");
 }
 if(!integer(s.at("selected"),-1,511)||!integer(s.at("inspected"),0,511)||!s.at("units").contains(std::to_string(s.at("inspected").get<int>())))throw std::runtime_error("invalid saved selection");
 if(hulao_duel_&&!duel_includes(s.at("inspected")))throw std::runtime_error("Excluded duel inspector");
 int selected=s.at("selected");if(selected>=0){auto b=BattleUnit::from_json(s.at("units").at(std::to_string(selected)).at("battle"));if(phase!="player"||b.side!=0||b.lifecycle!=2||b.hp<=0||(b.flags&2))throw std::runtime_error("invalid saved action");}
 if(!s.at("undo").is_null()){
  const auto& u=s.at("undo");if(selected<0||u.size()!=3||!integer(u.at("x"),0,map_width()-1)||!integer(u.at("y"),0,map_height()-1)||!integer(u.at("direction"),0,3)||occupied.contains(u.at("y").get<int>()*map_width()+u.at("x").get<int>()))throw std::runtime_error("invalid saved undo");
  auto r=UnitRecord::from_json(s.at("units").at(std::to_string(selected)).at("record"));if(movement_cost(r,terrain_.at("cells").at(u.at("y").get<int>()*map_width()+u.at("x").get<int>()),data_)==255)throw std::runtime_error("impassable saved undo");
 }
 if(s.at("turn_limit_reached").get<bool>()&&(s.at("turn")!=20||s.at("enemy_cursor")!=int(npc_order_.size())))throw std::runtime_error("invalid saved turn limit");
 bool lost=(hulao_duel_?duel_friendlies_defeated(s):s.at("units").at("0").at("battle").at("hp")==0)||s.at("turn_limit_reached").get<bool>();
 if((phase=="defeat")!=lost)throw std::runtime_error("invalid saved defeat");
 if(phase=="player"&&s.at("enemy_cursor")!=0)throw std::runtime_error("invalid saved phase cursor");
}
void Prototype::restore_revised(const Json& p){
 bool no_starter=p.at("schema")==19,squads=no_starter||p.at("schema")==18,supplies=!no_starter&&(squads||p.at("schema")==17),trio=p.at("schema")==16||((squads||supplies)&&p.contains("duel_ally2")),team=trio||p.at("schema")==15||((squads||supplies)&&p.contains("duel_ally")),reinforcement=team||p.at("schema")==14,heroes=supplies||reinforcement||p.at("schema")==13,open_gate=heroes||p.at("schema")==12,duel=open_gate||p.at("schema")==11;
 if(no_starter){if(p.at("duel_enemy")!=127||!p.at("duel_starting_beans").is_number_integer()||p.at("duel_starting_beans")!=0)throw std::runtime_error("Invalid Dong Zhuo supply policy");}
 else if(supplies?(!p.at("duel_starting_beans").is_number_integer()||p.at("duel_starting_beans")!=3):p.contains("duel_starting_beans"))throw std::runtime_error("Invalid starting bean grant");
 if(heroes&&(!p.at("duel_hero").is_number_integer()||!p.at("duel_enemy").is_number_integer()||!duel_hero_valid(p.at("duel_hero"))||!duel_enemy_valid(p.at("duel_enemy"))))throw std::runtime_error("Invalid saved duel characters");
 if(reinforcement&&(!p.at("duel_ally").is_number_integer()||!duel_hero_valid(p.at("duel_ally"))||p.at("duel_ally")==p.at("duel_hero")||(!squads&&p.at("duel_enemy")!=119)))throw std::runtime_error("Invalid saved duel ally");
 if(trio&&(!p.at("duel_ally2").is_number_integer()||!duel_hero_valid(p.at("duel_ally2"))||p.at("duel_ally2")==p.at("duel_hero")||p.at("duel_ally2")==p.at("duel_ally")))throw std::runtime_error("Invalid saved second duel ally");
 if(!trio&&p.contains("duel_ally2"))throw std::runtime_error("Unexpected second duel ally in historical save");
 if(squads){
  if(!reinforcement||(p.at("duel_enemy")==119?(!trio||!p.contains("duel_ally3")||!p.at("duel_ally3").is_number_integer()||!duel_hero_valid(p.at("duel_ally3"))||p.at("duel_ally3")==p.at("duel_hero")||p.at("duel_ally3")==p.at("duel_ally")||p.at("duel_ally3")==p.at("duel_ally2")):(trio||p.contains("duel_ally3"))))throw std::runtime_error("Invalid saved squad lineup");
 }else if(p.contains("duel_ally3"))throw std::runtime_error("Unexpected third duel ally in historical save");
 bool manifest_ok=!duel||p.at("duel_manifest")==duel_manifest_;
 if(duel&&!heroes)for(const auto& old:duel_legacy_manifests_)manifest_ok|=p.at("duel_manifest")==old;
 if(open_gate&&p.at("duel_map")!="hulao-open-east-gate-v1")throw std::runtime_error("Incompatible duel terrain");
 if(duel&&(!manifest_ok||duel_manifest_.empty()||p.at("scenario")!=2||p.at("conversations")!=false))throw std::runtime_error("Incompatible duel snapshot");
 if(p.at("engine")!=(no_starter?"prototype-0.19-duel":squads?"prototype-0.18-duel":supplies?"prototype-0.17-duel":trio?"prototype-0.16-duel":team?"prototype-0.15-duel":reinforcement?"prototype-0.14-duel":heroes?"prototype-0.13-duel":open_gate?"prototype-0.12-duel":duel?"prototype-0.11-duel":p.at("schema")==10?"prototype-0.10":"prototype-0.9")||p.at("ruleset")!="revised-battles-v1"||p.at("expansion_manifest")!=expansion_manifest_||p.at("tactics_manifest")!=tactics_manifest_||p.at("extended_tactics_manifest")!=extended_tactics_manifest_||p.at("terrain_manifest")!=terrain_manifest_||p.at("items_manifest")!=items_manifest_)throw std::runtime_error("incompatible revised snapshot");
 int scenario=p.at("scenario");if(scenario<0||scenario>=scenario_count()||p.at("layout")!=(squads?"hulao-duel-squads-v1":trio?"hulao-duel-trio-v1":team?"hulao-duel-team-v1":reinforcement?"hulao-duel-ally-v1":heroes?"hulao-duel-heroes-v1":duel?"hulao-duel-v1":scenario?"script-deployment-v1":"electron-slot1-v1")||!p.at("conversations").is_boolean())throw std::runtime_error("invalid revised scenario");
 Prototype c=*this;c.hulao_duel_=duel;c.hulao_gates_open_=open_gate;c.hulao_heroes_=heroes;c.duel_hero_=heroes?p.at("duel_hero").get<int>():0;c.duel_enemy_=heroes?p.at("duel_enemy").get<int>():127;c.duel_saved_manifest_=duel?p.at("duel_manifest").get<std::string>():"";c.equipment_enabled_=p.at("schema")>=10;c.revised_rules_=true;c.balanced_=c.tactics_enabled_=c.terrain_enabled_=c.items_enabled_=true;c.conversations_=p.at("conversations");c.campaign_=p.at("campaign");
 c.duel_ally_=reinforcement?p.at("duel_ally").get<int>():-1;c.duel_ally2_=trio?p.at("duel_ally2").get<int>():-1;c.duel_ally3_=squads&&trio?p.at("duel_ally3").get<int>():-1;c.hulao_squads_=squads;c.hulao_team_survival_=team;c.hulao_starting_beans_=supplies;c.scenario_=scenario;c.validate_campaign();
 c.initialize(1,false,scenario);c.progress_=p.at("progress");c.npc_order_=p.at("npc_order").get<std::vector<int>>();
 std::set<int> seen;for(int id:c.npc_order_){if(!c.state_.at("units").contains(std::to_string(id))||!seen.insert(id).second)throw std::runtime_error("invalid saved NPC order");auto b=BattleUnit::from_json(c.state_.at("units").at(std::to_string(id)).at("battle"));std::string phase=p.at("state").at("phase");if((duel&&id!=c.duel_enemy_)||b.side==0||(phase=="ally"&&b.side!=1)||(phase=="enemy"&&b.side<2))throw std::runtime_error("wrong army in saved NPC order");}
 c.validate(p.at("state"));c.state_=p.at("state");c.dialogue_=p.at("dialogue");c.validate_dialogue();c.validate_events();c.benefits_=p.at("benefits");c.consumed_items_=p.at("consumed_items");c.validate_benefits();if(c.equipment_enabled_){c.equipment_bag_=p.at("equipment_bag");c.validate_equipment();}
 if((c.state_.at("phase")=="victory")!=c.progress_.value("victory",false)&&c.state_.at("phase")!="defeat")throw std::runtime_error("invalid saved victory");
 if(!p.at("enemy_choice").is_null()){auto choice=p.at("enemy_choice");if(choice.size()!=2)throw std::runtime_error("invalid saved AI choice");c.choose_enemy(choice.at("key"),choice.at("choice"));}
 *this=std::move(c);
}
}
