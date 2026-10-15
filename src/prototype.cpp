#include "caocao/prototype.hpp"
#include "caocao/prototype_layout.hpp"
#include "caocao/attack_animation.hpp"
#include <utility>
#include "caocao/movement.hpp"
#include "caocao/range.hpp"
#include "caocao/ai.hpp"
#include "caocao/combat_rules.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <queue>
#include <set>
#include <tuple>
namespace caocao {
namespace {
const Json& unit(const Json& s,int id){return s.at("units").at(std::to_string(id));}
BattleUnit battle(const Json& s,int id){return BattleUnit::from_json(unit(s,id).at("battle"));}
bool alive(const Json& u){return u.at("battle").at("hp").get<int>()>0;}
}
Prototype::Prototype(const std::filesystem::path& assets,bool conversations,bool balanced):balanced_(balanced),new_balanced_(balanced),conversations_(conversations){
 auto m=read_json(assets/"manifest.json");manifest_=m.at("hash");m.erase("hash");
 if(sha256(m.dump())!=manifest_)throw std::runtime_error("asset manifest checksum mismatch");
 auto checked=[&](const std::string& relative){
  std::string expected;for(const auto& a:m.at("assets"))if(a.at("path")==relative)expected=a.at("sha256");
  std::ifstream f(assets/relative,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
  if(expected.empty()||sha256(raw)!=expected)throw std::runtime_error("asset checksum mismatch: "+relative);
  return Json::parse(raw);
 };
 data_=checked("data.json");terrain_=checked("terrain.json");base_terrain_=terrain_;
 if(std::filesystem::exists(assets/"tactics/manifest.json")){
  auto manifest=read_json(assets/"tactics/manifest.json");tactics_manifest_=manifest.at("hash");manifest.erase("hash");
  if(sha256(manifest.dump())!=tactics_manifest_)throw std::runtime_error("tactic manifest mismatch");
  for(const auto& entry:manifest.at("assets")){
   auto path=std::filesystem::path(entry.at("path").get<std::string>());if(path.is_absolute()||path.string().find("..")!=std::string::npos)throw std::runtime_error("invalid tactic asset path");
   std::ifstream f(assets/"tactics"/path,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
   if(sha256(raw)!=entry.at("sha256").get<std::string>())throw std::runtime_error("tactic asset checksum mismatch");
   if(path=="catalog.json")spell_catalog_=Json::parse(raw);
  }
 }
 load_terrain_benefits(assets);load_items(assets);load_extended_tactics(assets);
 if(std::filesystem::exists(assets/"combat_text.json"))combat_text_=checked("combat_text.json");
 opening_script_=checked("scripts/S_00.json");load_expansion(assets);
 if(terrain_.at("width")!=20||terrain_.at("height")!=20||terrain_.at("cells").size()!=400)throw std::runtime_error("prototype requires original opening map");
 if(std::filesystem::exists(assets/"special-hulao/manifest.json")){
  const auto m=read_json(assets/"special-hulao/manifest.json");duel_manifest_=m.at("hash");
  duel_legacy_manifests_=m.value("legacy_duel_manifests",std::vector<std::string>{});
 }
 start();
}
Json Prototype::take_attack(){return std::exchange(attack_,nullptr);}
Json Prototype::take_movement(){return std::exchange(movement_,nullptr);}
void Prototype::start(uint32_t seed){hulao_duel_=false;equipment_enabled_=new_equipment_&&new_revised_rules_;revised_rules_=new_revised_rules_;campaign_=Json::object();balanced_=new_balanced_;tactics_enabled_=new_tactics_&&balanced_;terrain_enabled_=new_terrain_&&balanced_;items_enabled_=new_items_&&balanced_;initialize(seed,false);}
void Prototype::start_scenario(int scenario,uint32_t seed){
 if(scenario<0||scenario>=scenario_count())throw std::runtime_error("战场不存在。");
 Prototype next=*this;next.hulao_duel_=false;next.equipment_enabled_=new_equipment_&&new_revised_rules_;next.revised_rules_=new_revised_rules_;next.campaign_=Json::object();next.balanced_=new_balanced_;next.tactics_enabled_=new_tactics_&&next.balanced_;next.terrain_enabled_=new_terrain_&&next.balanced_;next.items_enabled_=new_items_&&next.balanced_;next.initialize(seed,false,scenario);*this=std::move(next);
}
void Prototype::initialize(uint32_t seed,bool legacy,int scenario){
 scenario_=scenario;consumed_items_={{"87",0},{"88",0},{"92",0}};
 benefits_={{"claimed",Json::array()},{"inventory",{{"87",0},{"88",0},{"92",0}}}};environment_=nullptr;

 attack_=nullptr;movement_=nullptr;enemy_choice_=nullptr;request_cache_key_.clear();request_cache_=nullptr;legacy_=legacy;terrain_=scenario_?scenario_terrains_.at(std::to_string(scenario_)):base_terrain_;
 load_dialogues(scenario_?scenario_scripts_.at(std::to_string(scenario_)):opening_script_);
 dialogue_=conversations_?Json{{"triggered",Json::array()},{"pending",Json::array()},{"page",0}}:Json(nullptr);
 state_={{"turn",1},{"phase","player"},{"selected",-1},{"inspected",0},{"undo",nullptr},{"enemy_cursor",0},{"rng",seed},{"message","颍川之战：击败所有敌军，曹操不能战败。"},{"units",Json::object()}};
 // Preserve the old ruleset on schema1 restore; new games use the captured slot.
 const int old_setup[][4]={{0,10,6,0},{32,9,5,0},{3,8,5,0},{33,12,5,0},{258,9,6,0},{259,10,7,0},
 {146,9,13,2},{147,10,12,2},{474,7,15,2},{475,9,15,2},{476,10,15,2},{477,11,16,2},{478,9,16,2},{479,8,15,2}};
 std::vector<PrototypePlacement> setup;
 if(legacy_){int ally=0,enemy=35;for(const auto& row:old_setup)setup.push_back({row[0],row[3]?enemy++:ally++,row[3],row[1],row[2],row[3]?0:2,0});}
 else if(scenario_){
  state_["turn_limit_reached"]=false;
  state_["message"]=name()+"："+objective()+"，曹操不能战败。";
  for(const auto& r:config().at("roster"))setup.push_back({r.at("id"),r.at("slot"),r.at("side"),r.at("x"),r.at("y"),r.at("direction"),0});
 }
 else {
  setup.assign(std::begin(electron_layout),std::end(electron_layout));state_["turn"]=2;state_["turn_limit_reached"]=false;
  for(const auto& fire:electron_fires)terrain_["cells"][fire[1]*20+fire[0]]=26;
 }
 for(const auto& row:setup){
  int id=row.id,side=row.side;auto r=load_unit_record(data_.at("heroes").at(id));
  if(balanced_&&!legacy_)r=balanced_record(id,row.slot);else initialize_npc(r,scenario_?config().at(side==2?"enemy_level":"friendly_level").get<int>():side==2?1:3,0,data_);if(!revised_rules_)r.xp=0;
  BattleUnit b;b.id=id;b.slot=row.slot;b.side=side;b.x=row.x;b.y=row.y;b.lifecycle=2;b.ai=side==2?0:7;b.direction=row.direction;b.status=row.status;
  b.hp=std::min(500,r.hp+equipment_bonus(r,5,data_));b.mp=r.mp;b.target_slot=255;
  if(movement_cost(r,terrain_.at("cells").at(b.y*map_width()+b.x),data_)==255)throw std::runtime_error("prototype spawn is impassable");
  state_["units"][std::to_string(id)]={{"record",r.json()},{"battle",b.json()},{"max_hp",b.hp},{"name",data_.at("heroes").at(id).at("name")}};
 }
 npc_order_.clear();if(revised_rules_){initialize_events();state_["message"]=name()+"："+objective()+"，曹操不能战败。";if(!campaign_.empty()&&!campaign_.at("battles").empty())benefits_["inventory"]=campaign_.at("battles").back().at("benefits").at("inventory");}
 initialize_duel_deployment();initial_=state_;initialize_equipment();validate(state_);if(scenario_&&conversations_)queue_scenario_dialogues();
}
std::vector<BattleUnit> Prototype::roster()const{
 std::vector<BattleUnit> out;for(const auto& u:state_.at("units"))out.push_back(BattleUnit::from_json(u.at("battle")));return out;
}
int Prototype::at(int x,int y)const{for(const auto& u:state_.at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp>0&&(!revised_rules_||b.lifecycle==2)&&b.x==x&&b.y==y)return b.id;}return -1;}
std::vector<int> Prototype::movement(int id)const{
 auto b=battle(state_,id);auto r=UnitRecord::from_json(unit(state_,id).at("record"));
 return battle_movement_search(b,r,{b.x,b.y},revised_rules_&&b.ai==2?0:battle_movement_points(b,r,data_),{255,255},MovementSearchMode::range,true,map_width(),map_height(),terrain_.at("cells"),roster(),data_).costs;
}
bool Prototype::npc_engaged(int id)const{auto b=battle(state_,id);if(b.ai!=3)return true;auto r=UnitRecord::from_json(unit(state_,id).at("record"));return battle_movement_search(b,r,{b.x,b.y},battle_movement_points(b,r,data_),{255,255},MovementSearchMode::opponent_range,true,map_width(),map_height(),terrain_.at("cells"),roster(),data_,prepare_battle_ai(b,r,roster(),map_width(),map_height(),terrain_.at("cells"),data_,std::vector<int>(cell_count())).threat).result!=255;}
std::vector<int> Prototype::movement()const{
 int id=state_.at("selected");if(id<0||!state_.at("undo").is_null())return std::vector<int>(cell_count(),255);return movement(id);
}
Json Prototype::movement_packet(int id,int x,int y)const{
 auto b=battle(state_,id);if(b.x==x&&b.y==y)return nullptr;
 auto r=UnitRecord::from_json(unit(state_,id).at("record"));auto units=roster();auto costs=movement(id);
 auto route=battle_movement_route(b,r,{b.x,b.y},{x,y},255,false,map_width(),map_height(),terrain_.at("cells"),costs,
  std::vector<int>(256,255),std::vector<int>(cell_count()),units,data_).scratch;
 Json events=Json::array(),path=Json::array({Json::array({b.x,b.y})});
 std::vector<int> occupancy(cell_count(),255);for(const auto& other:units)if(other.lifecycle==2&&other.id!=id)occupancy.at(other.y*map_width()+other.x)=other.slot;
 constexpr int delta[4][2]={{0,-1},{1,0},{0,1},{-1,0}};
 size_t n=0;
 while(b.x!=x||b.y!=y){
  if(n>=route.size()||route[n]<0||route[n]>3)throw std::runtime_error("prototype walking route did not reach destination");
  int direction=route[n++],nx=b.x+delta[direction][0],ny=b.y+delta[direction][1];
  if(nx<0||nx>=map_width()||ny<0||ny>=map_height()||costs.at(ny*map_width()+nx)==255||movement_cost(r,terrain_.at("cells").at(ny*map_width()+nx),data_)==255)
   throw std::runtime_error("prototype walking route entered an unreachable tile");
  // Center a temporary original-size viewport on each step to avoid scrolling;
  // translate its verified original frame coordinates into the full-map view.
  std::array<int,2> camera{std::clamp(b.x-6,0,std::max(0,map_width()-13)),std::clamp(b.y-4,0,std::max(0,map_height()-8))};
  auto step=battle_walking_step(b,r.job,direction,2,camera,map_width(),map_height(),occupancy);
  for(auto e:step.events){
   std::string kind=e.at("kind");
   if(kind=="final_tile")continue; // The SDL frontend redraws the entire terrain every frame.
   if(kind=="frame"||kind=="overlay"){
    e["x"]=e.at("x").get<int>()+camera[0]*48;e["y"]=e.at("y").get<int>()+camera[1]*48;
    if(kind=="frame")e.update({{"archive","UNIT_MOV.E5"},{"sprite",battle_sprite(b.id,r.job,b.side)},{"effects",Json::array()}});
   }else if(kind!="wait")throw std::runtime_error("unsupported prototype walking event: "+kind);
   events.push_back(std::move(e));
  }
  b=step.actor;b.x=nx;b.y=ny;path.push_back(Json::array({nx,ny}));
 }
 // Original walks wait before drawing. Hold the final destination frame once
 // before switching to the committed board or a queued attack.
 events.push_back({{"kind","wait"},{"ticks",1}});
 return {{"kind","movement"},{"actor",id},{"before",state_},{"path",path},{"events",events}};
}
bool Prototype::attackable(int id,int target,std::array<int,2> pos)const{
 auto a=battle(state_,id),b=battle(state_,target);if(a.hp<=0||b.hp<=0||(revised_rules_&&(a.lifecycle!=2||b.lifecycle!=2))||(a.side==2)==(b.side==2))return false;
 auto r=UnitRecord::from_json(unit(state_,id).at("record"));
 auto range=battle_range(a,pos,battle_attack_range(r,data_),0,false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},roster());
 return std::find(range.targets.begin(),range.targets.end(),b.slot)!=range.targets.end();
}
bool Prototype::attackable(int id,int target)const{auto a=battle(state_,id);return attackable(id,target,{a.x,a.y});}
void Prototype::outcome(){
 if(hulao_duel_){
  bool lost=duel_friendlies_defeated(state_)||state_.at("turn_limit_reached").get<bool>();
  bool won=!lost&&battle(state_,duel_enemy_).hp==0;progress_["victory"]=won;
  if(lost||won){state_["phase"]=won?"victory":"defeat";state_["message"]=won?"胜利！"+unit(state_,duel_enemy_).at("name").get<std::string>()+"已被击败。":hulao_team_survival_?(state_.at("turn_limit_reached").get<bool>()?"战败：超过回合限制。":"战败：我军全体武将已被击败。"):"战败："+unit(state_,duel_hero_).at("name").get<std::string>()+"战败或超过回合限制。";state_["selected"]=-1;state_["undo"]=nullptr;}
  return;
 }
 if(revised_rules_){
  if(battle(state_,0).hp==0||state_.at("turn_limit_reached").get<bool>()){state_["phase"]="defeat";state_["message"]="战败：曹操战败或超过回合限制。";process_events();}
  else {process_events();if(progress_.value("victory",false)){state_["phase"]="victory";state_["message"]="胜利！"+name()+"结束。";}}
  if(state_.at("phase")=="victory"||state_.at("phase")=="defeat"){state_["selected"]=-1;state_["undo"]=nullptr;}return;
 }
 if(!legacy_&&state_.at("turn_limit_reached").get<bool>()){state_["phase"]="defeat";state_["message"]="战败：超过回合限制。可以重新开始。";}
 else if(battle(state_,0).hp==0){state_["phase"]="defeat";state_["message"]="战败：曹操已被击败。可以重新开始。";}
 else {bool enemies=false;for(const auto& u:state_.at("units"))if(alive(u)&&u.at("battle").at("side")==2)enemies=true;
  if(!enemies||enemies_defeated(state_)){state_["phase"]="victory";state_["message"]="胜利！"+name()+"结束。";}}
 if(state_.at("phase")=="victory"||state_.at("phase")=="defeat"){state_["selected"]=-1;state_["undo"]=nullptr;}
}
void Prototype::finish(int id){state_["units"][std::to_string(id)]["battle"]["flags"]=2;state_["selected"]=-1;state_["undo"]=nullptr;collect_locations();outcome();queue_dialogues();}
void Prototype::attack(int id,int target){
 if(revised_rules_){attack_revised(id,target);return;}
 if(!attackable(id,target))throw std::runtime_error("敌军不在攻击范围内。");
 auto a=battle(state_,id),b=battle(state_,target);auto ar=UnitRecord::from_json(unit(state_,id).at("record")),br=UnitRecord::from_json(unit(state_,target).at("record"));
 OriginalRng rng{state_.at("rng").get<uint32_t>()};
 // Verified physical damage and hit roll. Historical saves retain their
 // 12..45 clamp. Still one strike: no counters/critical/double/status/XP.
 int damage=physical_damage(a,ar,b,br,terrain_.at("cells").at(a.y*map_width()+a.x),terrain_.at("cells").at(b.y*map_width()+b.x),true,false,data_,rng);
 if(!balanced_)damage=std::clamp(damage,12,45);
 damage=std::min(damage,b.hp);
 auto visual_actor=a;visual_actor.direction=battle_direction(a,b);
 visual_actor.flags=(visual_actor.flags&~8)|(visual_actor.direction==1?8:0);
 auto visual_target=b;visual_target.flags=(visual_target.flags&~8)|(visual_target.direction==1?8:0);
 auto visual=state_;visual["selected"]=-1;visual["undo"]=nullptr;
 visual["units"][std::to_string(id)]["battle"]=visual_actor.json();
 visual["units"][std::to_string(target)]["battle"]=visual_target.json();
 auto animation=physical_attack_animation(visual_actor,ar,visual_target,br,damage,0,false,0,0,0,data_);
 attack_={{"actor",id},{"target",target},{"damage",damage},{"before",std::move(visual)},{"events",std::move(animation.events)}};
 // Commit exactly once. The frontend displays the packet before revealing this result.
 b.hp-=damage;if(!b.hp)b.lifecycle=3;
 a.direction=battle_direction(a,b);
 state_["units"][std::to_string(id)]["battle"]=a.json();state_["units"][std::to_string(target)]["battle"]=b.json();state_["rng"]=rng.state;
 state_["message"]=unit(state_,id).at("name").get<std::string>()+"攻击"+unit(state_,target).at("name").get<std::string>()+"："+std::to_string(damage)+"点伤害。";
 finish(id);
}
bool Prototype::player_turn_complete()const{
 if(state_.at("phase")!="player"||state_.at("selected")!=-1||!state_.at("undo").is_null()||has_dialogue()||!story_choices().empty())return false;
 bool deployed=false;
 for(const auto& u:state_.at("units")){
  const auto b=BattleUnit::from_json(u.at("battle"));
  if((revised_rules_?b.side!=0:b.side==2)||b.lifecycle!=2||b.hp<=0)continue;
  deployed=true;
  // Confused own units cannot receive player commands this phase.
  if(!(b.flags&2)&&!(revised_rules_&&(b.status&8)))return false;
 }
 return deployed;
}
void Prototype::command(const std::string& cmd,int x,int y){
 attack_=nullptr;movement_=nullptr;environment_=nullptr;
 if(cmd=="restart"){start_scenario(scenario_);return;}
 if(cmd=="continue"){continue_dialogue();return;}
 if(has_dialogue()||(revised_rules_&&!story_choices().empty()))throw std::runtime_error("请先继续对话。");
 if(state_.at("phase")!="player")throw std::runtime_error("现在不能下达命令。");
 int id=state_.at("selected");
 if(cmd=="cancel"){
  if(!state_.at("undo").is_null()){auto& b=state_["units"][std::to_string(id)]["battle"];for(const auto& k:{"x","y","direction"})b[k]=state_.at("undo").at(k);}
  state_["selected"]=-1;state_["undo"]=nullptr;state_["message"]="已取消移动和选择。";queue_dialogues();return;
 }
 if(cmd=="end_turn"){
  if(revised_rules_){for(auto& u:state_["units"])if(u.at("battle").at("side")==0)u["battle"]["flags"]=u.at("battle").at("flags").get<int>()|2;state_["selected"]=-1;state_["undo"]=nullptr;collect_locations();begin_phase_revised("ally");return;}
  for(auto& u:state_["units"])if(u.at("battle").at("side")!=2&&alive(u))u["battle"]["flags"]=2;
  state_["selected"]=-1;state_["undo"]=nullptr;state_["phase"]="enemy";state_["enemy_cursor"]=0;state_["message"]="敌军行动中……";collect_locations();poison_phase(true);terrain_phase(true);queue_dialogues();return;
 }
 if(cmd=="wait"){
  if(id<0)throw std::runtime_error("请先选择我军部队。");
  finish(id);state_["message"]="部队待命。";return;
 }
 if(cmd!="tile"||x<0||x>=map_width()||y<0||y>=map_height())throw std::runtime_error("无效的地图坐标。");
 int target=at(x,y);
 if(target>=0)state_["inspected"]=target;
 if(id>=0&&target>=0&&battle(state_,target).side==2){attack(id,target);return;}
 if(target>=0){
  auto b=battle(state_,target);if((revised_rules_?b.side==0:b.side!=2)&&b.lifecycle==2&&!(b.flags&2)&&!(revised_rules_&&(b.status&8))){
   if(!state_.at("undo").is_null())throw std::runtime_error("请先攻击、待命或取消移动。");
   state_["selected"]=target;state_["message"]="选择空地移动，选择敌军攻击；右键取消。";
  }else state_["message"]=b.side==2?"敌军部队。":"这支部队已行动。";
  return;
 }
 if(id<0)throw std::runtime_error("请先选择我军部队。");
 if(!state_.at("undo").is_null())throw std::runtime_error("已经移动，请攻击、待命或取消。");
 auto costs=movement(id);if(costs.at(y*map_width()+x)==255)throw std::runtime_error("不是移动范围。");
 movement_=movement_packet(id,x,y);
 auto b=battle(state_,id);state_["undo"]={{"x",b.x},{"y",b.y},{"direction",b.direction}};
 auto dest=b;dest.x=x;dest.y=y;b.direction=battle_direction(b,dest);b.x=x;b.y=y;state_["units"][std::to_string(id)]["battle"]=b.json();
 state_["message"]="已移动。选择敌军攻击，或待命；右键撤回移动。";
}
Json Prototype::suggest(int id)const{
 auto a=battle(state_,id);auto r=UnitRecord::from_json(unit(state_,id).at("record"));auto costs=movement(id);
 // Reverse terrain-distance field guides movement around obstacles. Occupants
 // are excluded by the actual movement search, not by this strategic field.
 constexpr int INF=100000;std::vector<int> distance(cell_count(),INF);
 using Node=std::pair<int,int>;std::priority_queue<Node,std::vector<Node>,std::greater<Node>> queue;
 for(const auto& u:state_.at("units"))if(alive(u)&&(!revised_rules_||u.at("battle").at("lifecycle")==2)&&(u.at("battle").at("side")==2)!=(a.side==2)){int pos=u.at("battle").at("y").get<int>()*map_width()+u.at("battle").at("x").get<int>();distance[pos]=0;queue.push({0,pos});}
 while(!queue.empty()){
  auto [d,pos]=queue.top();queue.pop();if(d!=distance[pos])continue;
  for(auto [dx,dy]:{std::pair{0,-1},{1,0},{0,1},{-1,0}}){int x=pos%map_width()+dx,y=pos/map_width()+dy;if(x<0||x>=map_width()||y<0||y>=map_height())continue;
   int cost=movement_cost(r,terrain_.at("cells").at(pos),data_);if(cost==255)continue;
   int n=y*map_width()+x;if(movement_cost(r,terrain_.at("cells").at(n),data_)==255)continue;
   if(d+cost<distance[n]){distance[n]=d+cost;queue.push({distance[n],n});}
  }
 }
 auto board=roster();
 auto best=std::tuple{2,INF,INF,INF,INF};Json result={{"x",a.x},{"y",a.y},{"target",-1}};
 for(int pos=0;pos<cell_count();++pos){int x=pos%map_width(),y=pos/map_width(),occupant=at(x,y);if(costs[pos]==255||(occupant>=0&&occupant!=id))continue;
  int target=-1,hp=INF;auto range=battle_range(a,{x,y},battle_attack_range(r,data_),0,false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},board);
  for(const auto& b:board){if(b.hp>0&&(!revised_rules_||b.lifecycle==2)&&(b.side==2)!=(a.side==2)&&std::find(range.targets.begin(),range.targets.end(),b.slot)!=range.targets.end()&&std::pair{b.hp,b.id}<std::pair{hp,target}){target=b.id;hp=b.hp;}}
  auto score=std::tuple{target<0?1:0,target<0?distance[pos]:hp,costs[pos],y,x};
  if(score<best){best=score;result={{"x",x},{"y",y},{"target",target}};}
 }
 return result;
}
void Prototype::enemy_step(){
 if(revised_rules_){npc_step_revised();return;}
 attack_=nullptr;movement_=nullptr;environment_=nullptr;
 if(state_.at("phase")!="enemy"||has_dialogue())return;
 Json chosen=nullptr;if(!enemy_choice_.is_null()){auto request=enemy_request();if(!request.is_null()&&request.at("key")==enemy_choice_.at("key"))chosen=request.at("choices").at(enemy_choice_.at("choice").get<std::string>());enemy_choice_=nullptr;}
 if(tactics_enabled_&&chosen.is_null()){auto request=enemy_request();if(!request.is_null())chosen=request.at("choices").at("a0");}
 std::vector<int> order;for(const auto& u:state_.at("units"))if(u.at("battle").at("side")==2)order.push_back(u.at("battle").at("id"));
 std::sort(order.begin(),order.end());int cursor=state_.at("enemy_cursor");
 while(cursor<static_cast<int>(order.size())){int id=order[cursor++];state_["enemy_cursor"]=cursor;if(battle(state_,id).hp<=0)continue;
  if(!legacy_&&(battle(state_,id).status&8)){
   // Prototype rule: initial confusion consumes one enemy action, then clears.
   // Original random status recovery is not claimed by this simplified mode.
   state_["units"][std::to_string(id)]["battle"]["status"]=battle(state_,id).status&~8;finish(id);
   state_["message"]=unit(state_,id).at("name").get<std::string>()+"混乱，停止行动。";return;
  }
  // Resolve the current actor before the cursor advances to the following one.
  auto plan=chosen.is_null()?suggest(id):chosen;movement_=movement_packet(id,plan.at("x"),plan.at("y"));auto b=battle(state_,id);auto next=b;next.x=plan.at("x");next.y=plan.at("y");if(next.x!=b.x||next.y!=b.y)next.direction=battle_direction(b,next);
  state_["units"][std::to_string(id)]["battle"]=next.json();state_["inspected"]=id;
  if(plan.value("kind",std::string())=="tactic")spell_attack(id,plan.at("tactic"),plan.at("target"));
  else if(plan.at("target")!=-1)attack(id,plan.at("target"));else {finish(id);state_["message"]=unit(state_,id).at("name").get<std::string>()+"移动并待命。";}
  return;
 }
 if(!legacy_&&state_.at("turn")==20){state_["turn_limit_reached"]=true;outcome();queue_dialogues();return;}
 for(auto& u:state_["units"])u["battle"]["flags"]=0;
 state_["phase"]="player";state_["turn"]=state_.at("turn").get<int>()+1;state_["enemy_cursor"]=0;state_["message"]="我军回合。";
 poison_phase(false);terrain_phase(false);
}
std::string Prototype::serialize()const{
 Json p={{"mode","opening-prototype"},{"schema",legacy_?1:2},{"engine",legacy_?"prototype-0.1":"prototype-0.2"},{"asset_manifest",manifest_},{"state",state_}};
 if(conversations_){p["ruleset"]=p.at("engine");p["schema"]=3;p["engine"]="prototype-0.3";p["dialogue"]=dialogue_;}
 if(!legacy_)p["layout"]="electron-slot1-v1";
 if(scenario_){p["schema"]=4;p["engine"]="prototype-0.4";p["scenario"]=scenario_;p["expansion_manifest"]=expansion_manifest_;p["layout"]="script-deployment-v1";p["dialogue"]=dialogue_;p["conversations"]=conversations_;}
 if(balanced_){p["schema"]=5;p["engine"]="prototype-0.5";p["ruleset"]="script-balance-v1";p["scenario"]=scenario_;p["expansion_manifest"]=expansion_manifest_;p["conversations"]=conversations_;p["dialogue"]=dialogue_;p["enemy_choice"]=enemy_choice_;}
 if(tactics_enabled_){p["schema"]=6;p["engine"]="prototype-0.6";p["ruleset"]="basic-tactics-v1";p["tactics_manifest"]=tactics_manifest_;}
 if(terrain_enabled_){p["schema"]=7;p["engine"]="prototype-0.7";p["ruleset"]="terrain-benefits-v1";p["terrain_manifest"]=terrain_manifest_;p["benefits"]=benefits_;}
 if(items_enabled_){p["schema"]=8;p["engine"]="prototype-0.8";p["ruleset"]="consumables-v1";p["items_manifest"]=items_manifest_;p["consumed_items"]=consumed_items_;}
 if(revised_rules_){p.update({{"schema",9},{"engine","prototype-0.9"},{"ruleset","revised-battles-v1"},{"extended_tactics_manifest",extended_tactics_manifest_},{"progress",progress_},{"campaign",campaign_},{"npc_order",npc_order_}});}
 if(equipment_enabled_)p.update({{"schema",10},{"engine","prototype-0.10"},{"equipment_bag",equipment_bag_}});
 if(hulao_duel_)p.update({{"schema",11},{"engine","prototype-0.11-duel"},{"layout","hulao-duel-v1"},{"duel_manifest",duel_saved_manifest_}});
 if(hulao_gates_open())p.update({{"schema",12},{"engine","prototype-0.12-duel"},{"duel_map","hulao-open-east-gate-v1"}});
 if(hulao_duel_&&hulao_heroes_)p.update({{"schema",13},{"engine","prototype-0.13-duel"},{"layout","hulao-duel-heroes-v1"},{"duel_hero",duel_hero_},{"duel_enemy",duel_enemy_}});
 if(hulao_duel_&&duel_ally_>=0)p.update({{"schema",14},{"engine","prototype-0.14-duel"},{"layout","hulao-duel-ally-v1"},{"duel_ally",duel_ally_}});
 if(duel_team_survival())p.update({{"schema",15},{"engine","prototype-0.15-duel"},{"layout","hulao-duel-team-v1"}});
 if(hulao_duel_&&duel_ally2_>=0)p.update({{"schema",16},{"engine","prototype-0.16-duel"},{"layout","hulao-duel-trio-v1"},{"duel_ally2",duel_ally2_}});
 if(hulao_duel_&&hulao_starting_beans_)p.update({{"schema",17},{"engine","prototype-0.17-duel"},{"duel_starting_beans",3}});
 if(hulao_duel_&&hulao_squads_){
  p.update({{"schema",18},{"engine","prototype-0.18-duel"},{"layout","hulao-duel-squads-v1"}});
  if(duel_ally3_>=0)p["duel_ally3"]=duel_ally3_;
  if(!hulao_starting_beans_)p.update({{"schema",19},{"engine","prototype-0.19-duel"},{"duel_starting_beans",0}});
 }
 return Json{{"payload",p},{"sha256",sha256(p.dump())}}.dump();
}
void Prototype::validate(const Json& s)const{
 if(revised_rules_){validate_revised_state(s);return;}
 if(!s.is_object()||s.size()!=initial_.size()||s.at("units").size()!=initial_.at("units").size())throw std::runtime_error("存档部队不完整。");
 auto integer=[](const Json& j,int lo,int hi){return j.is_number_integer()&&j>=lo&&j<=hi;};
 if(!integer(s.at("turn"),legacy_||scenario_?1:2,legacy_?100000:20)||!integer(s.at("enemy_cursor"),0,enemy_count())||!s.at("rng").is_number_integer()||s.at("rng")<0||s.at("rng")>UINT32_MAX||!s.at("message").is_string()||s.at("message").get_ref<const std::string&>().size()>4096)throw std::runtime_error("存档状态无效。");
 std::string phase=s.at("phase");if(phase!="player"&&phase!="enemy"&&phase!="victory"&&phase!="defeat")throw std::runtime_error("存档回合无效。");
 if(!integer(s.at("selected"),-1,511)||!integer(s.at("inspected"),0,511))throw std::runtime_error("存档选择无效。");
 int selected=s.at("selected"),inspected=s.at("inspected");std::set<int> tiles;bool enemies=false;
 for(auto it=initial_.at("units").begin();it!=initial_.at("units").end();++it){
  const auto& u=s.at("units").at(it.key());if(u.size()!=it.value().size()||u.at("record")!=it.value().at("record")||u.at("max_hp")!=it.value().at("max_hp")||u.at("name")!=it.value().at("name"))throw std::runtime_error("存档固定属性不一致。");
  const auto& raw=u.at("battle");if(raw.size()!=it.value().at("battle").size())throw std::runtime_error("存档部队字段无效。");
  for(auto f=raw.begin();f!=raw.end();++f)if(f.key()!="x"&&f.key()!="y"&&f.key()!="direction"&&f.key()!="hp"&&f.key()!="flags"&&f.key()!="lifecycle"&&!(f.key()=="mp"&&tactics_enabled_)&&!(f.key()=="status"&&!legacy_)&&f.value()!=it.value().at("battle").at(f.key()))throw std::runtime_error("存档部队固定字段无效。");
  for(auto k:{"x","y"})if(!integer(raw.at(k),0,std::string(k)=="x"?map_width()-1:map_height()-1))throw std::runtime_error("存档坐标无效。");
  if(!integer(raw.at("hp"),0,u.at("max_hp"))||!integer(raw.at("direction"),0,3)||(raw.at("flags")!=0&&raw.at("flags")!=2))throw std::runtime_error("存档兵力或行动标记无效。");
  if(tactics_enabled_){if(!integer(raw.at("mp"),0,it.value().at("battle").at("mp"))||!integer(raw.at("status"),0,24)||(raw.at("status").get<int>()&~(16|it.value().at("battle").at("status").get<int>())))throw std::runtime_error("存档策略值或状态无效。");}
  else if(!legacy_&&raw.at("status")!=0&&raw.at("status")!=it.value().at("battle").at("status"))throw std::runtime_error("存档状态无效。");
  auto b=BattleUnit::from_json(raw);if(b.lifecycle!=(b.hp>0?2:3)||movement_cost(UnitRecord::from_json(u.at("record")),terrain_.at("cells").at(b.y*map_width()+b.x),data_)==255||(b.hp>0&&!tiles.insert(b.y*map_width()+b.x).second))throw std::runtime_error("存档部队坐标重复。");
  if(b.side==2&&b.hp>0)enemies=true;
 }
 if(!s.at("units").contains(std::to_string(inspected)))throw std::runtime_error("存档选择无效。");
 if(selected!=-1){if(phase!="player"||!s.at("units").contains(std::to_string(selected)))throw std::runtime_error("存档行动阶段无效。");auto b=battle(s,selected);if(b.hp<=0||b.side==2||(b.flags&2))throw std::runtime_error("存档行动部队无效。");}
 bool timeout=false;
 if(!legacy_){if(!s.at("turn_limit_reached").is_boolean())throw std::runtime_error("存档状态无效。");timeout=s.at("turn_limit_reached");if(timeout&&(s.at("turn")!=20||s.at("enemy_cursor")!=enemy_count()))throw std::runtime_error("存档回合无效。");}
 bool lost=battle(s,0).hp==0||timeout,won=!lost&&(!enemies||enemies_defeated(s));
 if((phase=="defeat")!=lost||(phase=="victory")!=won)throw std::runtime_error("存档胜负状态无效。");
 if(phase!="enemy"&&s.at("enemy_cursor")!=0&&phase!="victory"&&phase!="defeat")throw std::runtime_error("存档敌军顺序无效。");
 if(!s.at("undo").is_null()){
  const auto& undo=s.at("undo");if(selected<0||undo.size()!=3||!integer(undo.at("x"),0,map_width()-1)||!integer(undo.at("y"),0,map_height()-1)||!integer(undo.at("direction"),0,3))throw std::runtime_error("存档撤回坐标无效。");
  int pos=undo.at("y").get<int>()*map_width()+undo.at("x").get<int>();if(tiles.contains(pos)||movement_cost(UnitRecord::from_json(unit(s,selected).at("record")),terrain_.at("cells").at(pos),data_)==255)throw std::runtime_error("存档撤回坐标不可用。");
 }
}
void Prototype::restore(const std::string& text){
 if(text.size()>200000)throw std::runtime_error("存档过大。");
 auto e=Json::parse(text);const auto& p=e.at("payload");
 if(e.at("sha256")!=sha256(p.dump())||p.at("mode")!="opening-prototype"||p.at("asset_manifest")!=manifest_)throw std::runtime_error("存档错误或版本不兼容。");
 if((p.at("schema")==9||p.at("schema")==10||p.at("schema")==11||p.at("schema")==12||p.at("schema")==13||p.at("schema")==14||p.at("schema")==15||p.at("schema")==16||p.at("schema")==17||p.at("schema")==18||p.at("schema")==19)){restore_revised(p);return;}
 if(p.at("schema")==5||p.at("schema")==6||p.at("schema")==7||p.at("schema")==8){
  bool items=p.at("schema")==8,terrain=p.at("schema")>=7,magic=p.at("schema")>=6;
  if(items&&(items_manifest_.empty()||p.at("items_manifest")!=items_manifest_))throw std::runtime_error("incompatible items snapshot");
  if(terrain&&(terrain_catalog_.is_null()||p.at("terrain_manifest")!=terrain_manifest_))throw std::runtime_error("incompatible terrain snapshot");
  if(p.at("engine")!=(items?"prototype-0.8":terrain?"prototype-0.7":magic?"prototype-0.6":"prototype-0.5")||p.at("ruleset")!=(items?"consumables-v1":terrain?"terrain-benefits-v1":magic?"basic-tactics-v1":"script-balance-v1")||(magic&&(spell_catalog_.is_null()||p.at("tactics_manifest")!=tactics_manifest_))||p.at("expansion_manifest")!=expansion_manifest_||!p.at("scenario").is_number_integer()||!p.at("conversations").is_boolean())throw std::runtime_error("incompatible balanced snapshot");
  int id=p.at("scenario");if(id<0||id>=scenario_count()||p.at("layout")!=(id?"script-deployment-v1":"electron-slot1-v1"))throw std::runtime_error("invalid balanced scenario");
  Prototype candidate=*this;candidate.hulao_duel_=false;candidate.equipment_enabled_=false;candidate.revised_rules_=false;candidate.campaign_=Json::object();candidate.progress_=Json::object();candidate.balanced_=true;candidate.tactics_enabled_=magic;candidate.terrain_enabled_=terrain;candidate.items_enabled_=items;candidate.conversations_=p.at("conversations");candidate.initialize(1,false,id);candidate.validate(p.at("state"));candidate.state_=p.at("state");candidate.dialogue_=p.at("dialogue");candidate.validate_dialogue();
  if(items)candidate.consumed_items_=p.at("consumed_items");
  if(terrain){candidate.benefits_=p.at("benefits");candidate.validate_benefits();}
  if(!p.at("enemy_choice").is_null()){const auto& c=p.at("enemy_choice");if(c.size()!=2)throw std::runtime_error("invalid saved AI decision");candidate.choose_enemy(c.at("key"),c.at("choice"));}
  *this=std::move(candidate);return;
 }
 if(p.at("schema")==4){
  if(p.at("engine")!="prototype-0.4"||p.at("expansion_manifest")!=expansion_manifest_||p.at("layout")!="script-deployment-v1"||!p.at("scenario").is_number_integer()||!p.at("conversations").is_boolean())throw std::runtime_error("存档错误或版本不兼容。");
  int id=p.at("scenario");if(id<1||id>=scenario_count())throw std::runtime_error("存档战场无效。");
  Prototype candidate=*this;candidate.hulao_duel_=false;candidate.equipment_enabled_=false;candidate.revised_rules_=false;candidate.campaign_=Json::object();candidate.progress_=Json::object();candidate.balanced_=false;candidate.tactics_enabled_=false;candidate.terrain_enabled_=false;candidate.items_enabled_=false;candidate.conversations_=p.at("conversations");candidate.initialize(1,false,id);candidate.validate(p.at("state"));candidate.state_=p.at("state");candidate.dialogue_=p.at("dialogue");candidate.validate_dialogue();*this=std::move(candidate);return;
 }
 bool modern=p.at("schema")==3&&p.at("engine")=="prototype-0.3";
 bool legacy=modern?p.value("ruleset","")=="prototype-0.1":p.at("schema")==1&&p.at("engine")=="prototype-0.1";
 if(modern){if(p.value("ruleset","")!="prototype-0.1"&&p.value("ruleset","")!="prototype-0.2")throw std::runtime_error("存档错误或版本不兼容。");}
 else if(!legacy&&(p.at("schema")!=2||p.at("engine")!="prototype-0.2"))throw std::runtime_error("存档错误或版本不兼容。");
 if(!legacy&&p.value("layout","")!="electron-slot1-v1")throw std::runtime_error("存档错误或版本不兼容。");
 Prototype candidate=*this;candidate.hulao_duel_=false;candidate.equipment_enabled_=false;candidate.revised_rules_=false;candidate.campaign_=Json::object();candidate.progress_=Json::object();candidate.balanced_=false;candidate.tactics_enabled_=false;candidate.terrain_enabled_=false;candidate.items_enabled_=false;if(modern)candidate.conversations_=true;
 candidate.initialize(1,legacy);candidate.validate(p.at("state"));candidate.state_=p.at("state");
 if(modern)candidate.dialogue_=p.at("dialogue");else candidate.migrate_dialogue();
 candidate.validate_dialogue();*this=std::move(candidate);
}
}
