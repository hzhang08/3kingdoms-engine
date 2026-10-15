#pragma once
#include "json.hpp"
#include <cstdint>
#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace caocao {
using Json=nlohmann::json;
inline constexpr int snapshot_schema=7;
inline constexpr char engine_version[]="0.29.0-investigation";
std::string sha256(const std::string& text);
Json read_json(const std::filesystem::path& path);
struct OriginalRng {
    uint32_t state=1;
    uint32_t next(){state=state*1103515245u+12345u;return (state>>16u)&0x7fffu;}
    uint32_t bounded(uint32_t n){return n<2?0:next()%n;}
    bool percent(uint32_t chance){return bounded(100)<chance;}
};
struct GameState {
    std::string script="R_00", phase="story", background="story", diagnostic;
    int program=0, pc=0, turn=1, turn_limit=20, choice=0, ambition=50;
    int camera_x=0,camera_y=0,music=-1, selected=-1,mcall_resource=-1;
    int battle_phase=0,battle_outcome=0,event_scans=0;
    int environment_region=0,environment_selector=0;
    std::string battle_stage="intro";
    bool stable=false,previous_condition=false, victory=false, defeat=false;
    uint64_t commands=0;
    OriginalRng rng;
    std::vector<int> variables=std::vector<int>(1024,0);
    std::vector<int> collection=std::vector<int>(108,0);
    Json tutorial=nullptr;
    Json units=Json::object(),inventory=Json::object(),terrain=Json::array();
    Json pending=Json::array(),presentation=Json::object(),triggered=Json::array();
    Json pending_action=nullptr,controls=Json::object(),script_fields=Json::object();
    Json battle_poses=Json::object();
    Json terrain_effects=std::vector<std::array<int,5>>(250,{255,255,255,255,255}),terrain_marks=Json::array();
};
struct Instruction {Json wire;int jump=-1;};
struct Program {std::string script;int section=0,offset=0,end=0;Json conditions;std::vector<Instruction> code;};
struct Command {std::string type;int value=0,x=0,y=0,target=-1;};
class Engine {
public:
    explicit Engine(const std::filesystem::path& assets);
    void start_scenario(uint32_t seed=1);
    void submit(const Command& command);
    void advance();
    const GameState& state()const{return state_;}
    const Json& data()const{return data_;}
    const std::vector<Program>& programs()const{return programs_;}
    Json state_json()const;
    std::string state_hash()const;
    std::string serialize()const;
    void restore(const std::string& snapshot);
    const std::string& manifest_hash()const{return manifest_;}
private:
    std::vector<Program> programs_;
    GameState state_;
    Json data_,map_,ui_,tactics_,tactic_records_,combat_text_,poison_effect_,secondary_effects_,mcall_;
    std::string manifest_;
    void compile(const Json& sequence,std::vector<Instruction>& code);
    bool condition(const Json& instruction);
    bool execute(const Instruction& instruction);
    void stop(const Json& wire,const std::string& reason);
    void dialogue(const std::string& text,bool focus_active_speaker=true);
    Json& unit(int id);
    void start_program(int id);
    void next_program();
    void scan_battle_events();
    void begin_battle_events(const std::string& resume);
    void advance_battle_phase();
    void initialize_battle();
    bool advance_animation();
    bool advance_tutorial();
    void queue_battle_action(const Json&,const Json& wire,int uid,int animation=-1);
};
}
