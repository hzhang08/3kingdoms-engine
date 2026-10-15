#pragma once
#include "caocao/battle.hpp"
namespace caocao {
// Original 0x43c42e / 0x418da0; no RNG or experience change.
struct ConsumableEffect {int hp;int status;};
ConsumableEffect consumable_effect(int item,int strength,int max_hp,int hp,int status);
bool location_pickup_eligible(const BattleUnit&,int x,int y,bool claimed);
// Deliberately simplified rules, separate from the original-script Engine.
class Prototype {
public:
 // Disabling conversations is reserved for historical schema1/2 regression fixtures.
 explicit Prototype(const std::filesystem::path& assets,bool conversations=true,bool balanced=true);
 void start(uint32_t seed=1);
 void start_scenario(int scenario,uint32_t seed=1);
 // New games opt in; loading an older schema retains its original rules.
 void enable_revised_rules();
 bool revised_rules()const{return revised_rules_;}
 // Separate custom scenario; retains combat mechanics and Henry's records.
 // Historical fixtures choose starting_beans explicitly; current squads grant them only against Lu Bu.
 void start_hulao_duel(const std::string& seed,int hero=0,int enemy=127,int ally=-1,int ally2=-1,bool starting_beans=false,int ally3=-1,bool squads=false);
 // Current two/four-friendly rules; historical saves retain their own lineup.
 void start_hulao_squad(const std::string& seed,int hero,int enemy,uint32_t ally_draw=0);
 static bool duel_hero_valid(int id){return id==0||id==6||id==1||id==11;}
 static bool duel_enemy_valid(int id){return id==119||id==127;}
 static int duel_enemy_for_draw(uint32_t draw){return (draw&1)?119:127;}
 static int duel_ally_for_draw(int hero,uint32_t draw);
 static std::array<int,2> duel_allies_for_draw(int hero,uint32_t draw);
 int duel_hero()const{return duel_hero_;}
 int duel_enemy()const{return duel_enemy_;}
 int duel_ally()const{return duel_ally_;}
 int duel_ally2()const{return duel_ally2_;}
 int duel_ally3()const{return duel_ally3_;}
 bool duel_team_survival()const{return hulao_duel_&&hulao_team_survival_;}
 bool duel_includes(int id)const{return hulao_duel_&&id>=0&&(id==duel_hero_||id==duel_enemy_||id==duel_ally_||id==duel_ally2_||id==duel_ally3_);}
 void upgrade_hulao_duel();
 bool hulao_duel()const{return hulao_duel_;}
 void open_hulao_gates();
 bool hulao_gates_open()const{return hulao_duel_&&hulao_gates_open_;}
 void advance_campaign();
 void choose_story(int choice);
 Json story_choices()const;
 const Json& progress()const{return progress_;}
 Json treasures()const;
 void enable_equipment();
 bool equipment_enabled()const{return equipment_enabled_;}
 const Json& equipment_bag()const{return equipment_bag_;}
 bool equipment_compatible(int actor,int slot,int item)const;
 std::string equipment_block_reason(int actor)const;
 // bag_index -1 means remove; preserves the complete item triple.
 void change_equipment(int actor,int slot,int bag_index);

 int scenario()const{return scenario_;}
 int scenario_count()const{return 1+int(scenarios_.size());}
 int map_width()const{return terrain_.at("width");}
 int map_height()const{return terrain_.at("height");}
 int cell_count()const{return map_width()*map_height();}
 std::string name()const;
 std::string objective()const;
 std::string battlefield_image()const;
 std::string scenario_name(int id)const;

 void command(const std::string&,int x=0,int y=0);
 // Presentation hosts may end the turn only after their animations/modals finish.
 bool player_turn_complete()const;
 void enemy_step();
 Json enemy_request()const;
 void choose_enemy(const std::string& request_key,const std::string& choice);
 void clear_enemy_choice(){enemy_choice_=nullptr;}
 bool has_enemy_choice()const{return !enemy_choice_.is_null();}
 bool balanced()const{return balanced_;}
 bool tactics_enabled()const{return tactics_enabled_;}
 void enable_tactics();
 void enable_terrain_benefits();
 void enable_items();
 bool items_enabled()const{return items_enabled_;}
 Json item_options(int actor,int item)const;
 void use_item(int item,int target);
 bool terrain_benefits_enabled()const{return terrain_enabled_;}
 const Json& inventory()const{return benefits_.at("inventory");}
 Json locations()const;
 int healing_at(int actor,int x,int y)const;
 Json take_environment();
 Json spells(int actor)const;
 Json spell_options(int actor,std::array<int,2> position)const;
 void cast(int tactic,int target);
 // Consume once; transient presentation is excluded from snapshots and state hashes.
 Json take_attack();
 Json take_movement();
 Json suggest(int id)const;
 const Json& state()const{return state_;}
 const Json& data()const{return data_;}
 const Json& terrain()const{return terrain_;}
 bool legacy()const{return legacy_;}
 bool has_dialogue()const{return !dialogue_.is_null()&&!dialogue_.at("pending").empty();}
 Json dialogue()const;
 std::vector<int> movement()const;
 bool attackable(int actor,int target)const;
 std::string serialize()const;
 void restore(const std::string&);
 std::string gameplay_hash()const{return sha256(state_.dump());}
 std::string hash()const{
  if(tactics_enabled_)return sha256(serialize());
  if(balanced_)return sha256(Json{{"rules","script-balance-v1"},{"scenario",scenario_},{"expansion",expansion_manifest_},{"state",state_},{"dialogue",dialogue_},{"enemy_choice",enemy_choice_}}.dump());
  if(scenario_)return sha256(Json{{"scenario",scenario_},{"expansion",expansion_manifest_},{"state",state_},{"dialogue",dialogue_}}.dump());
  return dialogue_.is_null()?gameplay_hash():sha256(Json{{"state",state_},{"dialogue",dialogue_}}.dump());
 }
private:
 bool hulao_duel_=false;
 bool hulao_gates_open_=false;
 bool hulao_heroes_=false;
 bool hulao_team_survival_=false;
 bool hulao_starting_beans_=false,hulao_squads_=false;
 int duel_hero_=0,duel_enemy_=127,duel_ally_=-1,duel_ally2_=-1,duel_ally3_=-1;
 std::string duel_manifest_,duel_saved_manifest_;
 std::vector<std::string> duel_legacy_manifests_;
 void initialize_duel_deployment();
 bool duel_friendlies_defeated(const Json&)const;
 bool equipment_enabled_=false,new_equipment_=false;
 Json equipment_bag_=Json::array(),initial_equipment_bag_=Json::array();
 void initialize_equipment();
 void reward_equipment(int item);
 void validate_equipment()const;
 void validate_equipment_triple(const Json&,int slot=-1)const;
 bool revised_rules_=false,new_revised_rules_=false;
 Json progress_=Json::object(),campaign_=Json::object();
 std::vector<int> npc_order_;
 Json combat_text_=nullptr;
 void initialize_events();
 void process_events();
 void validate_events()const;
 void attack_revised(int id,int target);
 void begin_phase_revised(const std::string& phase);
 void npc_step_revised();
 void maintenance_revised(bool enemy);
 std::vector<int> current_npc_order()const;
 bool npc_engaged(int id)const;
 void validate_campaign()const;
 void validate_revised_state(const Json&)const;
 void restore_revised(const Json&);
 void validate_revised_record(const Json&,const Json&,bool player=false)const;
 Json extended_spell_catalog_=nullptr;
 std::string extended_tactics_manifest_;
 void load_extended_tactics(const std::filesystem::path&);
 Json extended_spells(int)const;
 Json extended_spell_options(int,std::array<int,2>)const;
 void extended_spell_attack(int,int,int);
 bool items_enabled_=false,new_items_=false;
 Json consumed_items_={{"87",0},{"88",0},{"92",0}};
 std::string items_manifest_;
 void load_items(const std::filesystem::path&);
 bool terrain_enabled_=false,new_terrain_=false;
 Json terrain_catalog_=nullptr,benefits_={{"claimed",Json::array()},{"inventory",{{"87",0},{"88",0},{"92",0}}}};
 std::string terrain_manifest_;
 Json environment_=nullptr;
 void load_terrain_benefits(const std::filesystem::path&);
 void validate_benefits()const;
 void collect_locations();
 void terrain_phase(bool enemy);
 void environment_event(int actor,const Json& event);
 bool balanced_=true,new_balanced_=true;
 bool tactics_enabled_=false,new_tactics_=false;
 Json spell_catalog_=nullptr;
 std::string tactics_manifest_;
 void spell_attack(int actor,int tactic,int target);
 void poison_phase(bool enemy);
 Json enemy_choice_=nullptr;
 mutable std::string request_cache_key_;
 mutable Json request_cache_=nullptr;
 UnitRecord balanced_record(int id,int slot)const;
 int next_enemy_id()const;
 Json data_,terrain_,base_terrain_,initial_,state_;
 int scenario_=0;
 std::string expansion_manifest_;
 Json scenarios_=Json::array(),scenario_terrains_=Json::object(),scenario_scripts_=Json::object(),opening_script_;
 const Json& config()const{return scenarios_.at(scenario_-1);}
 void load_expansion(const std::filesystem::path&);
 bool scenario_dialogue_eligible(int id)const;
 void queue_scenario_dialogues();
 int enemy_count()const;
 bool enemies_defeated(const Json&)const;

 Json attack_=nullptr;
 Json movement_=nullptr;
 bool legacy_=false;
 bool conversations_=true;
 Json dialogue_=nullptr,dialogue_catalog_=Json::object();
 void load_dialogues(const Json& script);
 void queue_dialogues();
 void continue_dialogue();
 void validate_dialogue()const;
 void migrate_dialogue();
 void initialize(uint32_t seed,bool legacy,int scenario=0);
 std::string manifest_;
 std::vector<BattleUnit> roster()const;
 std::vector<int> movement(int id)const;
 bool attackable(int actor,int target,std::array<int,2> position)const;
 int at(int x,int y)const;
 void finish(int id);
 void attack(int id,int target);
 Json movement_packet(int id,int x,int y)const;
 void outcome();
 void validate(const Json&)const;
};
}
