#pragma once
#include "caocao/deployment.hpp"
#include "caocao/unit_rules.hpp"
namespace caocao {
// Original 0x449f91 / 0x44a1d4, independent of weather presentation.
int battle_environment_class(int region,int selector);
int advance_battle_environment(int region,int selector,int override_selector,OriginalRng&);
struct BattleUnit {
 int id=0,slot=0,side=0,x=0,y=0,target_slot=0,target_x=0,target_y=0;
 int lifecycle=1,flags=0,ai=0,direction=0,hp=0,mp=0,status=0;
 std::array<int,6> grades{3,3,3,3,3,3};
 Json json()const;
 static BattleUnit from_json(const Json&);
 void validate(const UnitRecord&,const Json& data,int width,int height)const;
};
BattleUnit initialize_battle_unit(UnitRecord&,const DeploymentSlot&,const Json& data);
void prepare_new_battle_unit(BattleUnit&);
std::array<int,2> new_battle_focus(const std::vector<BattleUnit>&);
void scripted_hp_loss(BattleUnit&,int amount);
// Original opcode78's non-targeted modes0..2, including party protection.
void scripted_unit_ai(BattleUnit&,const UnitRecord&,int mode);
std::array<int,2> battle_placement(int x,int y,int movement_group,int width,int height,const Json& terrain,const std::vector<int>& occupancy,const Json& data);
int battle_frame(const BattleUnit&,int maximum_hp,int tick);
// Original opcode-80 presentation sequence for verified animation kinds only.
// Coordinates use the original renderer's 56px header. Camera tiles describe
// the view AFTER honoring the first camera_request event. No state is mutated.
Json scripted_battle_animation(const BattleUnit&,int sprite,int animation,
                              bool prewait,bool postwait,int camera_tile_x,int camera_tile_y);
int battle_direction(const BattleUnit& from,const BattleUnit& to);
Json scripted_battle_facing(const BattleUnit&,const BattleUnit* target,int direction,bool reverse,bool prewait,bool postwait);
Json scripted_battle_removal(const std::vector<BattleUnit>&,int id);
// Original opcode49's single-unit, mode1 exit. Distinct from death/retirement.
Json scripted_battle_disappearance(const std::vector<BattleUnit>&,int id,int sprite,int terrain);
std::vector<uint8_t> mask_battle_sprite(std::vector<uint8_t> pixels,int offset,const std::vector<uint8_t>& mark);
Json defeated_battle_unit(const std::vector<BattleUnit>&,const UnitRecord&);
// Original opcode92: first active matching slot, three target flashes.
Json scripted_battle_highlight(const std::vector<BattleUnit>&,int id);
// Original 0x44bd27, including row sweeps and interruptible terminal pause.
Json battle_objective_banner(const std::string& text,int limit,const Json& labels,
    std::array<int,2> viewport={624,384},std::array<int,4> client={0,0,640,440});
Json battle_start_banner(const std::string& text,std::array<int,2> viewport={624,384},std::array<int,4> client={0,0,640,440});
// Original 0x44ac28. Flag reset is deliberately a timed step between text and
// the second sweep; the frontend must not treat this as cosmetic only.
Json battle_turn_banner(int turn,int limit,bool enemy,bool preserve_flags,
 const Json& labels,std::array<int,2> viewport={624,384},std::array<int,4> client={0,0,640,440});
Json scripted_battle_appearance(const BattleUnit&,const DeploymentSlot&,int job,bool by_slot,const Json& terrain,const std::vector<int>& occupancy,const Json& data);
inline constexpr int battle_view_width=624,battle_view_height=384,battle_view_top=56;
// Pixel positions of each original full-redraw step, excluding an unchanged view.
std::vector<std::array<int,2>> battle_camera_path(int camera_x,int camera_y,int focus_x,int focus_y,int map_width,int map_height);
// Original 0x455123 centers the actor, then shifts just enough to include target.
std::vector<std::array<int,2>> battle_attack_camera_path(int camera_x,int camera_y,
 std::array<int,2> actor,std::array<int,2> target,int map_width,int map_height);
std::string battle_sound_file(int id);
}
