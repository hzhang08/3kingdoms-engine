#include "caocao/prototype.hpp"

namespace caocao {
bool Prototype::duel_friendlies_defeated(const Json& state)const{
 auto defeated=[&](int id){return state.at("units").at(std::to_string(id)).at("battle").at("hp")==0;};
 return defeated(duel_hero_)&&(!hulao_team_survival_||((duel_ally_<0||defeated(duel_ally_))&&(duel_ally2_<0||defeated(duel_ally2_))&&(duel_ally3_<0||defeated(duel_ally3_))));
}
int Prototype::duel_ally_for_draw(int hero,uint32_t draw){
 if(!duel_hero_valid(hero))throw std::runtime_error("Invalid special battle hero");
 int index=int(draw%3);for(int id:{0,6,1,11})if(id!=hero&&index--==0)return id;
 throw std::runtime_error("Missing special battle ally");
}
std::array<int,2> Prototype::duel_allies_for_draw(int hero,uint32_t draw){
 // Choosing one of the other three heroes to omit makes all three pairs
 // equally likely. Stable order gives each included unit a distinct tile.
 const int omitted=duel_ally_for_draw(hero,draw);std::array<int,2> allies{};int n=0;
 for(int id:{0,6,1,11})if(id!=hero&&id!=omitted)allies[n++]=id;
 return allies;
}
void Prototype::initialize_duel_deployment(){
 if(!hulao_duel_)return;
 state_["inspected"]=duel_hero_;
 // Original Hexzmap #2: east entrance (6,9)..(6,10) is closed gate 17.
 // This custom duel opens only that entrance using original city-floor type 16.
 if(hulao_gates_open_)for(int y:{9,10})terrain_["cells"][y*20+6]=16;
 for(auto& u:state_["units"]){auto& b=u["battle"];int id=b.at("id");
  if(!duel_includes(id)){b["lifecycle"]=3;continue;}
  b["lifecycle"]=2;b["x"]=id==duel_enemy_?13:id==duel_ally3_?8:10;b["y"]=id==duel_ally_?12:id==duel_ally2_?8:10;
  b["direction"]=id==duel_enemy_?3:1;b["ai"]=id==duel_enemy_?1:7;
  b["target_slot"]=255;b["flags"]=0;b["status"]=0;
 }
}
void Prototype::start_hulao_duel(const std::string& seed,int hero,int enemy,int ally,int ally2,bool starting_beans,int ally3,bool squads){
 if(!duel_hero_valid(hero)||!duel_enemy_valid(enemy))throw std::runtime_error("Invalid special battle character");
 if((enemy==119||squads)?(!duel_hero_valid(ally)||ally==hero):ally!=-1)throw std::runtime_error("Invalid special battle ally");
 if(ally2!=-1&&(enemy!=119||!duel_hero_valid(ally2)||ally2==hero||ally2==ally))throw std::runtime_error("Invalid second special battle ally");
 if(squads){
  if((enemy==119&&!starting_beans)||(enemy==119?(!duel_hero_valid(ally3)||ally3==hero||ally3==ally||ally3==ally2||ally2<0):(ally2!=-1||ally3!=-1)))throw std::runtime_error("Invalid special battle squad");
 }else if(ally3!=-1)throw std::runtime_error("Unexpected third ally in historical match");
 if(duel_manifest_.empty())throw std::runtime_error("Missing duel assets");
 Prototype c=*this;c.restore(seed);
 if(c.scenario_!=2||!c.revised_rules_||c.hulao_duel_)throw std::runtime_error("Invalid Hulao source seed");
 c.enable_equipment();c.hulao_duel_=true;c.hulao_heroes_=true;c.duel_hero_=hero;c.duel_enemy_=enemy;c.duel_ally_=ally;c.duel_ally2_=ally2;c.duel_ally3_=ally3;c.hulao_squads_=squads;c.duel_saved_manifest_=duel_manifest_;c.hulao_gates_open_=true;c.conversations_=false;c.dialogue_=nullptr;
 c.hulao_team_survival_=ally>=0;
 c.initialize_events();c.initialize_duel_deployment();
 c.hulao_starting_beans_=starting_beans;
 if(starting_beans)c.benefits_["inventory"]["87"]=c.benefits_.at("inventory").at("87").get<int>()+3;
 c.state_["turn"]=1;c.state_["phase"]="player";c.state_["turn_limit_reached"]=false;
 c.state_["selected"]=-1;c.state_["inspected"]=hero;c.state_["undo"]=nullptr;c.state_["enemy_cursor"]=0;
 c.state_["message"]=c.state_["units"][std::to_string(hero)]["name"].get<std::string>()+" 对 "+c.state_["units"][std::to_string(enemy)]["name"].get<std::string>()+"：击败敌军。";
 if(ally>=0)c.state_["message"]=c.state_["units"][std::to_string(hero)]["name"].get<std::string>()+" + "+c.state_["units"][std::to_string(ally)]["name"].get<std::string>()+(ally2>=0?" + "+c.state_["units"][std::to_string(ally2)]["name"].get<std::string>():"")+(ally3>=0?" + "+c.state_["units"][std::to_string(ally3)]["name"].get<std::string>():"")+" 对 "+c.state_["units"][std::to_string(enemy)]["name"].get<std::string>();
 c.npc_order_.clear();c.clear_enemy_choice();c.request_cache_key_.clear();c.request_cache_=nullptr;
 c.attack_=c.movement_=c.environment_=nullptr;
 // Validate a complete portable snapshot before committing; never mutate seed.
 Prototype verified=c;verified.restore(c.serialize());*this=std::move(verified);
}
void Prototype::start_hulao_squad(const std::string& seed,int hero,int enemy,uint32_t ally_draw){
 if(!duel_hero_valid(hero)||!duel_enemy_valid(enemy))throw std::runtime_error("Invalid special battle character");
 std::array<int,3> allies{-1,-1,-1};
 if(enemy==127)allies[0]=duel_ally_for_draw(hero,ally_draw);
 else {int n=0;for(int id:{0,6,1,11})if(id!=hero)allies[n++]=id;}
 start_hulao_duel(seed,hero,enemy,allies[0],allies[1],enemy==119,allies[2],true);
}
void Prototype::upgrade_hulao_duel(){
 Prototype c=*this;c.open_hulao_gates();c.hulao_heroes_=true;c.duel_saved_manifest_=c.duel_manifest_;
 // Upgrade unfinished two-unit saves without changing turns, actions or RNG.
 // Completed historical outcomes remain historical; a read never resurrects a match.
 if(c.duel_ally_>=0&&c.state_.at("phase")!="victory"&&c.state_.at("phase")!="defeat")c.hulao_team_survival_=true;
 Prototype verified=c;verified.restore(c.serialize());*this=std::move(verified);
}
void Prototype::open_hulao_gates(){
 if(!hulao_duel_)throw std::runtime_error("Gate access is only available in the special duel");
 if(hulao_gates_open_)return;
 Prototype c=*this;c.hulao_gates_open_=true;
 for(int y:{9,10})c.terrain_["cells"][y*20+6]=16;
 // The old snapshot was validated under its old terrain first. Keep every
 // unit, RNG, outcome and tentative move; invalidate only cached AI planning.
 c.clear_enemy_choice();c.request_cache_key_.clear();c.request_cache_=nullptr;
 c.validate(c.state_);*this=std::move(c);
}
}
