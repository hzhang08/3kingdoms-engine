#include "caocao/engine.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/battle.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
using namespace caocao;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
template<class F>void rejects(F f,const char* why){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
int main(int argc,char**argv){try{
 check(sha256("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","SHA256 empty");check(sha256("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA256 abc");
 OriginalRng r{1};check(r.next()==16838&&r.next()==5758&&r.next()==10113,"original executable RNG");auto before=r.state;check(r.bounded(0)==0&&r.bounded(1)==0&&r.state==before,"non-consuming bounded RNG");
 if(argc!=2)throw std::runtime_error("asset argument required");
 auto fixtures=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_rng.json");
 for(const auto& sample:fixtures.at("rng").at("fixtures")){OriginalRng q{sample.at("seed").get<uint32_t>()};for(const auto& output:sample.at("values"))check(q.next()==output.get<uint32_t>(),"x86 emulator RNG fixture mismatch");check(q.state==sample.at("final_state").get<uint32_t>(),"x86 emulator final RNG state");}
 auto damage_fixtures=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_physical_arithmetic.json");
 for(const auto& sample:damage_fixtures.at("cases")){
  PhysicalDamageInputs in;const auto& j=sample.at("input");
  #define INPUT(field) j.at(#field).get_to(in.field)
  INPUT(attack);INPUT(defense);INPUT(attack_terrain);INPUT(defense_terrain);INPUT(level);INPUT(attack_class_group);INPUT(defense_class_group);INPUT(target_hp);INPUT(mounted_resistance_percent);INPUT(anti_infantry_percent);INPUT(randomize);INPUT(counterattack);INPUT(hit);
  #undef INPUT
  OriginalRng q{sample.at("seed").get<uint32_t>()};
  check(physical_damage_arithmetic(in,q)==sample.at("damage").get<uint32_t>(),"original physical damage arithmetic fixture mismatch");
  check(q.state==sample.at("final_rng").get<uint32_t>(),"damage arithmetic RNG consumption mismatch");
 }
 std::cout<<damage_fixtures.at("cases").size()<<" isolated damage arithmetic fixtures passed (stat/hit getters are stubbed).\n";
 for(int choice:{1,2}){
  Engine e(argv[1]),restored(argv[1]);e.start_scenario(1);std::string old,event_save;int snapshots=0,pose_snapshots=0,removal_snapshots=0,event_snapshots=0;Json intro_units,moved_actor,first_attack;int completed_attacks=0;std::array<int,2> player_camera{-1,-1};int panel_frames=0,walking_frames=0,critical_speeches=0;uint32_t attack_input_rng=0;
  auto allied=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_allied_phase.json");
  auto scripted=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_script_movement.json");
  auto enemy=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_enemy_entry.json");
  auto enemy_action=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_enemy_phase.json").at("actions").at(0);
  auto enemy_tactic=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_tactic_plan.json");
  auto enemy_cast=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_tactic_cast.json").at("cases").at(0);
  auto enemy_completed=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_poison_controller.json");
  auto poison_post=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_poison_post.json");
  auto second_enemy=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_second_enemy.json");
  auto remaining=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_enemy_continuation.json");
  auto next_turn=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_next_turn.json");
  auto fire_event=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_fire_event.json");
  auto summon_event=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_summon_event.json");
  auto status_event=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_status_event.json");
  auto arrival_event=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_arrival_event.json");
  auto objective_event=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_objective_event.json");
  Json confused_tiles=Json::array(),confusion_cameras=Json::array();
  for(const auto& ev:status_event.at("commands").at(0).at("events")){if(ev.at("kind")=="tile")confused_tiles.push_back(ev.at("battle"));if(ev.at("kind")=="camera")confusion_cameras.push_back(ev.at("tile"));}
  int fire_waits=0,summon_frames=0,summon_saves=0,confusion_frames=0;
  auto fire_command=[&](int offset)->const Json&{for(const auto* fixture:{&fire_event,&summon_event,&arrival_event})for(const auto& row:fixture->at("commands"))if(row.at("offset")==offset)return row;throw std::runtime_error("missing fire-event command");};
  int second_turn_dialogues=0,second_turn_events=0;std::string second_turn_save;
  auto remaining_action=[&](int id)->const Json&{for(const auto& row:remaining.at("actions"))if(row.at("unit")==id)return row;throw std::runtime_error("missing remaining enemy reference");};
  size_t remaining_frames=0,remaining_frame_total=0;
  Json enemy_frames=Json::array(),second_frames=Json::array();size_t enemy_frame=0,second_frame=0;
  for(const auto& ev:enemy_action.at("events"))if(ev.at("kind")=="frame")enemy_frames.push_back(ev);
  for(size_t n=0;n<second_enemy.at("captures").at(1).at("event_index").get<size_t>();++n){const auto& ev=second_enemy.at("events").at(n);if(ev.at("kind")=="frame")second_frames.push_back(ev);}
  auto compare_original=[&](const Json& reference){const auto& st=e.state();
   for(const auto& row:reference.at("units")){int id=row.at("unit").at("id");const auto& u=st.units.at(std::to_string(id));
    if(u.at("battle")!=row.at("battle"))throw std::runtime_error("allied sequence battle mismatch unit "+std::to_string(id)+": "+u.at("battle").dump()+" != "+row.at("battle").dump());
    check(UnitRecord::from_json(u).json()==row.at("unit"),"allied sequence persistent record mismatch");
   }
   check(st.rng.state==reference.at("rng"),"allied sequence RNG mismatch");
   check(st.script_fields.value("battle_tick",0)==reference.at("tick"),"allied sequence animation tick mismatch");
   if(Json::array({st.camera_x,st.camera_y})!=reference.at("camera"))throw std::runtime_error("sequence camera mismatch at "+st.presentation.dump()+" actual "+Json::array({st.camera_x,st.camera_y}).dump()+" expected "+reference.at("camera").dump());
  };
  int post_batches=0,post_redraws=0,enemy_dialogues=0,enemy_entries=0,banner_columns=0,scripted_moves=0,enemy_decisions=0,tactic_selections=0,tactic_panels=0,tactic_casts=0,tactic_captions=0,poison_frames=0;
  int death_dialogues=0,death_events=0,completed_tactics=0,post_tactic_batches=0,secondary_frames=0,no_attacks=0;std::string death_save;
  auto observe=[&](){const auto& st=e.state();if(st.battle_stage=="allied_entry")player_camera={st.camera_x,st.camera_y};
   if(st.presentation.value("type","")=="banner_region")compare_original(arrival_event.at("after"));
   if(st.presentation.value("type","")=="sound"&&st.presentation.value("repeat",1)!=255){
    auto file=battle_sound_file(st.presentation.at("id"));
    check(std::filesystem::is_regular_file(std::filesystem::path(argv[1])/file),"presentation sound was not imported");
   }
   if(st.presentation.value("type","")=="scripted_movement_complete"){
    const auto& reference=st.turn==1?scripted.at("cases").at(scripted_moves++):fire_command(st.presentation.at("offset"));if(st.turn==2)++scripted_moves;check(st.presentation.at("offset")==reference.at("offset"),"scripted movement source order changed");compare_original(reference.at("after"));
   }
   if(st.presentation.value("type","")=="animation_wait"&&!st.pending_action.is_null()&&st.pending_action.at("opcode")==33){
    ++fire_waits;const auto& expected=fire_command(st.pending_action.at("offset")).at("after");compare_original(expected);
    check(st.terrain==expected.at("terrain")&&st.terrain_effects==expected.at("records")&&st.terrain_marks==expected.at("effects"),"sequential original fire terrain/effects mismatch");
    check(!st.stable&&st.presentation.at("duration_ms")==50,"fire placement wait/save boundary changed");
   }
   if(st.presentation.value("type","")=="animation_wait"&&!st.pending_action.is_null()&&st.pending_action.contains("effect")&&st.pending_action.at("effect").value("resource",23)==105){
    const auto& effect=st.pending_action.at("effect");Json frames=Json::array();
    for(const auto& ev:fire_command(0x23f8).at("events"))if(ev.at("kind")=="effect")frames.push_back(ev);
    const auto& original=frames.at(summon_frames++);
    check(effect.at("frame")==original.at("frame")&&effect.at("rectangle")==original.at("rectangle")&&effect.at("weight")==original.at("weight")&&effect.at("amount")==original.at("amount"),"integrated MCALL frame sequence differs");
    compare_original(fire_command(0x23f8).at("after"));
    check(st.presentation.at("duration_ms")==100&&st.mcall_resource==105&&!st.stable,"MCALL timing or save boundary differs");
   }
   if(st.presentation.value("type","")=="animation_wait"&&!st.pending_action.is_null()&&st.pending_action.at("opcode")==77){
    int target=confusion_frames/13,frame=confusion_frames%13;auto expected=status_event.at("before");
    for(int n=0;n<target;++n)for(auto& u:expected["units"])if(u.at("battle").at("id")==confused_tiles.at(n).at("id"))u["battle"]=confused_tiles.at(n);
    expected["camera"]={confusion_cameras.at(target).at(0).get<int>()*48,confusion_cameras.at(target).at(1).get<int>()*48};compare_original(expected);
    check(!st.stable&&st.presentation.at("duration_ms")==50&&st.pending_action.at("effect").at("resource")==48&&st.pending_action.at("effect").at("frame")==frame,"confusion frame/timing or save boundary differs");
    ++confusion_frames;
   }
   if(st.presentation.value("type","")=="ai_movement"&&st.battle_phase==2){
    // The original waits before drawing each walking frame. Its final frame
    // is immediately redrawn, so compare the compiled draw requests, not just
    // the states at which advance() yields to the renderer.
    Json more_frames=Json::array();size_t more_frame=0;
    if(enemy_decisions>=2){const auto& row=remaining_action(st.presentation.at("unit"));
     for(size_t n=0;n<row.at("captures").at(1).at("event_index").get<size_t>();++n)if(row.at("events").at(n).at("kind")=="frame")more_frames.push_back(row.at("events").at(n));
     remaining_frame_total+=more_frames.size();
    }
    for(const auto& pose:st.pending_action.is_null()?Json::array():st.pending_action.at("steps"))if(pose.at("kind")=="frame"){
     const auto& ref=enemy_decisions==0?enemy_frames.at(enemy_frame++):enemy_decisions==1?second_frames.at(second_frame++):more_frames.at(more_frame++);
     check(pose.at("x")==ref.at("x")&&pose.at("y")==ref.at("y"),"enemy walking frame position mismatch");
     std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<pose.at("sprite").get<int>()<<".bin";
     std::ifstream f(std::filesystem::path(argv[1])/"raw/Unit_mov.e5"/filename.str(),std::ios::binary);std::string pixels(48*48,'\0');
     f.seekg(pose.at("frame").get<int>()*48*48);f.read(pixels.data(),pixels.size());check(bool(f),"missing enemy walk sprite");
     if(pose.at("mirror").get<bool>())for(int y=0;y<48;++y)std::reverse(pixels.begin()+y*48,pixels.begin()+(y+1)*48);
     check(sha256(pixels)==ref.at("pixels_sha256").get<std::string>(),"original enemy walking pixels differ");
    }
    if(enemy_decisions>=2){check(more_frame==more_frames.size(),"remaining enemy walk frames skipped");remaining_frames+=more_frame;}
    const auto& ref=enemy_decisions==0?enemy_action.at("captures").at(0):enemy_decisions==1?second_enemy.at("captures").at(0):remaining_action(st.presentation.at("unit")).at("captures").at(0);
    ++enemy_decisions;compare_original(ref);
    const auto& pending=st.script_fields.at("ai_pending");
    check(pending.at("destination")==ref.at("best_tile")&&pending.at("target")==ref.at("best_target")&&pending.at("tactic")==ref.at("best_tactic"),"enemy tactic decision changed");
   }
   if(st.presentation.value("type","")=="tactic_cast"){compare_original(tactic_casts++==0?enemy_cast.at("before"):second_enemy.at("captures").at(3));}
   if(st.presentation.value("type","")=="tactic_caption"){compare_original(tactic_captions++==0?enemy_cast.at("after"):second_enemy.at("captures").at(4));check(st.battle_poses.at(tactic_captions==1?"146":"147").at("archive")=="UNIT_ATK.E5","caster pose missing");check(st.pending_action.at("caption").back().at("text")=="毒烟","tactic caption missing");}
   if(st.presentation.value("type","")=="animation_wait"&&!st.pending_action.is_null()&&st.pending_action.contains("effect")&&st.pending_action.at("effect").value("resource",23)==23){
    check(st.pending_action.at("effect").at("frame")==poison_frames%13,"poison frame duplicated or missing");
    if(poison_frames>0&&poison_frames<13)check(st.pending_action.at("number").at("value")==10&&st.battle_poses.at("234").at("frame")==3,"poison reaction/damage caption missing");
    else check(!st.pending_action.contains("number"),"zero damage or pre-callback poison displayed a number");
    if(poison_frames>13)check(st.battle_poses.at("210").at("frame")==3,"second poison reaction missing");
    compare_original(poison_frames<13?enemy_cast.at("after"):second_enemy.at("captures").at(4));++poison_frames;
   }
   if(st.presentation.value("type","")=="animation_wait"&&!st.pending_action.is_null()&&st.pending_action.contains("effect")&&st.pending_action.at("effect").value("resource",23)==49){
    check(st.pending_action.at("effect").at("frame")==secondary_frames++,"secondary poison frame order");
    auto reference=second_enemy.at("captures").at(4);for(auto& row:reference["units"])if(row.at("battle").at("id")==210)row["battle"]["status"]=16;
    compare_original(reference);check(!st.stable&&st.presentation.at("duration_ms")==50,"secondary effect incorrectly saveable or timed");
   }
   if(st.presentation.value("type","")=="ai_no_attack"){++no_attacks;compare_original(remaining_action(st.presentation.at("unit")).at("after"));}
   if(st.presentation.value("type","")=="tactic_selection"){compare_original(tactic_selections++==0?enemy_tactic.at("selection").at("before"):second_enemy.at("captures").at(1));}
   if(st.battle_stage=="ai_tactic_plan"&&!st.pending_action.is_null()&&!st.pending_action.at("ui").at("panel").is_null()){
    compare_original(tactic_panels++==0?enemy_tactic.at("selection").at("after"):second_enemy.at("captures").at(2));
   }
   if(st.presentation.value("type","")=="turn_banner"){
    compare_original(st.turn==1?allied.at("transition").at(0):next_turn.at("trace").at(1));
    if(st.turn==2)check(st.battle_phase==2&&st.environment_region==1&&st.environment_selector==1&&!st.script_fields.contains("ai_order"),"next-turn commit/AI cleanup differs from original");
   }
   if(st.presentation.value("type","")=="battle_event"&&st.turn==2){
    ++second_turn_events;compare_original(next_turn.at("after"));check(st.presentation.at("offset")==0x20a1&&st.battle_phase==0,"wrong second-turn event order");
   }
   if(st.presentation.value("type","")=="banner_column")++banner_columns;
   if(st.presentation.value("type","")=="battle_event"&&st.battle_phase==2){
    if(st.presentation.at("offset")==0x3af7){++death_events;compare_original(poison_post.at("stages").at(0).at("before"));}
    else{compare_original(enemy.at("enemy_entry"));check(st.presentation.at("offset")==enemy.at("event_offset"),"wrong enemy entry event");++enemy_entries;}
   }
   if(st.presentation.value("type","")=="tactic_complete"){
    ++completed_tactics;compare_original(completed_tactics==1?enemy_completed.at("after"):second_enemy.at("after"));
    check(st.script_fields.at("tactic_plan")== (completed_tactics==1?enemy_tactic.at("cases").at(0).at("plan"):second_enemy.at("captures").at(4).at("plan")),"integrated poison planning changed original RNG/effect/damage");
   }
   if(st.presentation.value("type","")=="event_batch_complete"&&st.battle_phase==2&&st.script_fields.contains("ai_pending")){
    int actor=st.script_fields.at("ai_pending").at("unit");
    const auto& stages=actor==146?poison_post.at("stages"):actor==147?remaining.at("second_post"):remaining_action(actor).at("post");
    compare_original(stages.at(post_tactic_batches++%2==0?0:3).at("after"));
    check(st.variables[55]==1,"death dialogue flag was not committed");
   }
   if(st.presentation.value("type","")=="event_batch_complete"){++post_batches;check(st.event_scans==18,"post-action event batch not exhausted");}
   if(st.presentation.value("type","")=="post_action_redraw")++post_redraws;
   if(!st.pending_action.is_null()&&st.pending_action.contains("ui")&&!st.pending_action.at("ui").empty()&&!st.pending_action.at("ui").at("panel").is_null())++panel_frames;
   if(st.battle_poses.contains("234")&&st.battle_poses.at("234").contains("screen_x"))++walking_frames;
   if(st.presentation.value("type","")=="physical_attack"){
    compare_original(st.battle_phase==1?allied.at("actions").at(completed_attacks).at("moved"):remaining_action(st.presentation.at("unit")).at("captures").at(1));
    if(completed_attacks==0){moved_actor=st.units.at("234").at("battle");attack_input_rng=st.rng.state;}
   }
   if(st.presentation.value("type","")=="attack_complete"){
    compare_original(st.battle_phase==1?allied.at("actions").at(completed_attacks).at("after"):remaining_action(st.presentation.at("unit")).at("after"));
    if(completed_attacks==0)first_attack=e.state_json();++completed_attacks;
   }
  };
  for(int tick=0;tick<1000&&e.state().phase!="blocked"&&e.state().presentation.value("type","")!="tutorial_begin";++tick){
   observe();if(e.state().variables[500]&&intro_units.is_null())intro_units=e.state().units;
   if(e.state().stable&&!e.state().pending_action.is_null()){
    ++critical_speeches;check(e.state().presentation.at("combat_message")==743,"wrong critical speech");
    check(e.state_hash()==restored.state_hash(),"critical speech diverged after prior save restoration");
    rejects([&]{e.serialize();},"incomplete attack must not be saved");
    e.submit({"continue"});restored.submit({"continue"});
    for(int frames=0;!e.state().stable&&e.state().phase!="blocked"&&e.state().presentation.value("type","")!="tutorial_begin"&&frames<100;++frames){observe();e.advance();restored.advance();check(e.state_hash()==restored.state_hash(),"attack continuation diverged after prior save restoration");}
    continue;
   }
   if(e.state().stable){auto save=e.serialize();auto hash=e.state_hash();restored.restore(save);check(hash==restored.state_hash(),"restore hash");if(old.empty())old=save;
    auto env=Json::parse(save);env["payload"]["state"]["rng"]=123;rejects([&]{restored.restore(env.dump());},"reject corrupt save");check(hash==restored.state_hash(),"corrupt save changed live state");
    env=Json::parse(save);env["payload"]["schema"]=99;env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject incompatible schema");
    env=Json::parse(save);env["payload"]["engine"]="0.1.0-investigation";env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject obsolete engine save");
    env=Json::parse(save);env["payload"]["state"]["pc"]=-1;env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject invalid cursor");
    if(e.state().phase=="battle_intro"||e.state().phase=="battle_event"){
     for(int invalid:{0,1,2}){env=Json::parse(save);auto& state=env["payload"]["state"];if(invalid==0)state["units"]["234"]["battle"]["hp"]=999;else if(invalid==1)state["units"]["234"]["battle"]["slot"]=0;else state["units"]["234"].erase("battle");env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject malformed battle state");check(hash==restored.state_hash(),"invalid battle restore modified live state");}
    }
    if(e.state().phase=="battle_event"){
     if(e.state().turn==2){
      ++second_turn_dialogues;if(second_turn_dialogues==5)second_turn_save=save;
      if(e.state().mcall_resource==105){
       ++summon_saves;
       for(const auto& invalid:Json::array({-1,104,105.5,"105",nullptr})){
        auto bad=Json::parse(save);bad["payload"]["state"]["mcall_resource"]=invalid;bad["sha256"]=sha256(bad["payload"].dump());
        rejects([&]{restored.restore(bad.dump());},"corrupt MCALL preload accepted");check(restored.state_hash()==hash,"corrupt MCALL preload changed live state");
       }
      }
      Json speeches=Json::array();for(const auto& event:next_turn.at("dialogue_prefix").at("events"))if(event.at("kind")=="speech")speeches.push_back(event);
      for(const auto& ev:fire_command(0x222e).at("events"))if(ev.at("kind")=="speech")speeches.push_back(ev);
      for(const auto& cmd:summon_event.at("commands"))for(const auto& ev:cmd.at("events"))if(ev.at("kind")=="speech")speeches.push_back(ev);
      for(const auto& cmd:arrival_event.at("commands"))for(const auto& ev:cmd.at("events"))if(ev.at("kind")=="speech")speeches.push_back(ev);
      // The raster boundary records the whole speech. The engine exposes
      // each three-line page as a stable point; all pages retain that state.
      Json pages=Json::array();for(auto speech:speeches){std::istringstream lines(speech.at("text").get<std::string>());std::string line,page;int count=0;
       while(std::getline(lines,line)){if(count)page+='\n';page+=line;if(++count==3){speech["text"]=page;pages.push_back(speech);page.clear();count=0;}}
       if(count){speech["text"]=page;pages.push_back(speech);}
      }
      const auto& speech=pages.at(second_turn_dialogues-1);compare_original(speech.at("before"));
      check(e.state().presentation.at("text")==speech.at("text"),"second-turn dialogue text differs from original handler");
      check(e.state().presentation.at("speaker")==e.state().units.at(std::to_string(speech.at("unit").get<int>())).at("name"),"second-turn dialogue speaker order differs");
      check(!e.state().script_fields.contains("event_return")&&!e.state().script_fields.contains("ai_order")&&e.state().rng.state==340946258u,"second-turn save contains stale AI or incorrect RNG");
     }else if(e.state().battle_phase==0){event_save=save;++event_snapshots;check(e.state().presentation.at("text").get<std::string>().find("再坚持一会儿")!=std::string::npos,"wrong first-turn event");}
     else if(e.state().script_fields.contains("ai_order")){
      ++death_dialogues;death_save=save;const auto& speech=poison_post.at("stages").at(0).at("events").at(1);
      compare_original(speech.at("before"));check(e.state().presentation.at("text")==speech.at("text")&&e.state().presentation.at("speaker")=="弓兵1","wrong defeated archer dialogue");
      check(e.state().variables[55]==0&&e.state().script_fields.at("event_return")=="ai_deaths"&&!e.state().script_fields.contains("tactic_plan"),"defeat dialogue continuation is not at the original boundary");
      for(int invalid=0;invalid<7;++invalid){env=Json::parse(save);auto& state=env["payload"]["state"];auto& f=state["script_fields"];
       if(invalid==0)f["ai_order"][0]=20;if(invalid==1)state["battle_phase"]=1;
       if(invalid==2)f["ai_pending"]["tactic"]=23;if(invalid==3)f["ai_pending"]["target"]=255;
       if(invalid==4)state["units"]["146"]["battle"]["flags"]=4;
       if(invalid==5)f["tactic_plan"]={{"targets",{20}}};if(invalid==6)f["event_return"]="ai_tactic_apply";
       env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"invalid enemy tactic continuation accepted");check(hash==restored.state_hash(),"invalid enemy restore modified state");
      }
     }else{++enemy_dialogues;compare_original(enemy.at("commands").at(enemy_dialogues==1?3:6));check(e.state().battle_phase==2,"enemy event retained allied scheduler");}
     for(int invalid:{0,1,2,3,4}){env=Json::parse(save);auto& state=env["payload"]["state"];if(invalid==3)state["environment_region"]=5;else if(invalid==4)state["environment_selector"]=-1;else if(invalid==0)state["event_scans"]=999;else if(invalid==1)state["battle_phase"]=9;else state["battle_stage"]="unknown";env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject invalid scheduler continuation");check(hash==restored.state_hash(),"invalid scheduler restore changed state");}
    }
    if(!e.state().battle_poses.empty()){
     ++pose_snapshots;auto pose_id=e.state().battle_poses.begin().key();
     if(pose_id=="235"){check(e.state().battle_poses.at(pose_id).at("frame")==9,"incorrect retained wounded frame");check(e.state().camera_x==48&&e.state().camera_y==576,"original animation camera focus");}
     for(int invalid:{0,1,2,3}){env=Json::parse(save);auto& state=env["payload"]["state"];
      if(invalid==0)state["battle_poses"][pose_id]["frame"]=999;
      if(invalid==1)state["battle_poses"][pose_id]["archive"]="../unknown.bmp";
      if(invalid==2)state["battle_poses"]["0"]=state["battle_poses"][pose_id];
      if(invalid==3)state["pending_action"]={{"type","animation"}};
      env["sha256"]=sha256(env["payload"].dump());rejects([&]{restored.restore(env.dump());},"reject malformed retained presentation state");check(hash==restored.state_hash(),"invalid pose restore changed live state");
     }
    }
    if(e.state().phase=="battle_intro"&&e.state().units.at("235").at("battle").at("lifecycle")==3){++removal_snapshots;check(!e.state().battle_poses.contains("235"),"removed unit retained pose");}
    Command cmd{e.state().presentation.value("type","")=="choice"?"choose":"continue",choice};e.submit(cmd);restored.submit(cmd);check(e.state_hash()==restored.state_hash(),"next command diverged after restore");++snapshots;
    for(int frames=0;!e.state().stable&&e.state().phase!="blocked"&&e.state().presentation.value("type","")!="tutorial_begin"&&frames<100;++frames){
     observe();rejects([&]{e.serialize();},"must not save during animation");e.advance();restored.advance();check(e.state_hash()==restored.state_hash(),"presentation continuation diverged after restore");
    }
   }else e.advance();
  }
  check(pose_snapshots>=3,"wounded dialogue snapshots missing");check(removal_snapshots>=10,"removal state did not persist through dialogue");check(snapshots>25,"opening dialogue missing");check(e.state().variables[300]==(choice==1),"choice flag");check(e.state().ambition==(choice==1?60:40),"opening choice ambition");
  check(e.state().presentation.value("type","")=="tutorial_begin","opening sequence did not reach tutorial entry");rejects([&]{e.serialize();},"unstable tutorial entry must not save");
  auto deployments=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_deployment.json");check(e.state().script_fields.at("deployment")==deployments.at("deployment").back().at("slots"),"engine opening deployment differs from original");
  check(e.state().units.at("234").at("battle").at("hp")==0&&e.state().units.at("235").at("battle").at("hp")==10&&e.state().units.at("334").at("battle").at("hp")==10,"opening scripted damage");
  auto actions=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_battle_actions.json");
  for(const auto& row:actions.at("chain").back().at("after")){
   int id=row.at(0).get<int>()+256*row.at(1).get<int>();const auto& live=intro_units.at(std::to_string(id)).at("battle");
   const std::vector<std::pair<const char*,int>> fields={{"slot",4},{"side",5},{"x",6},{"y",7},{"target_slot",8},{"target_x",9},{"target_y",10},{"lifecycle",12},{"flags",13},{"direction",15},{"status",30}};
   for(auto [key,index]:fields)check(live.at(key)==row.at(index),"engine intro differs from original sequential script state");
   for(int n=0;n<6;++n)check(live.at("grades").at(n)==row.at(24+n),"intro grade mismatch");
   for(auto [key,index]:std::vector<std::pair<const char*,int>>{{"hp",16},{"mp",20}})check(live.at(key)==row.at(index).get<int>()+256*row.at(index+1).get<int>(),"intro health mismatch");
   check(live.at("ai")== (id==0?7:row.at(14).get<int>()),"intro AI mode mismatch");
  }
  check(event_snapshots==1&&e.state().variables[14]==1&&e.state().variables[11]==1&&e.state().triggered==Json::array({0x1e00,0x1ea4,0x3af7}),"events must execute once and survive restore");
  check(death_events==1&&death_dialogues==1&&completed_tactics==2&&post_tactic_batches==26&&e.state().variables[55]==1&&!death_save.empty(),"post-poison dialogue or retirement repeated/skipped");
  check(first_attack.at("units").at("210").at("battle").at("direction")==0,"first-turn facing missing");
  auto transition=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_phase_transition.json");
  check(e.state().turn==2&&e.state().battle_phase==0&&e.state().battle_stage=="entry_events","phase transition differs from original caller");
  int arrival_speeches=0;for(const auto& cmd:arrival_event.at("commands"))for(const auto& ev:cmd.at("events"))if(ev.at("kind")=="speech"){
   const auto text=ev.at("text").get<std::string>();arrival_speeches+=(std::count(text.begin(),text.end(),'\n')+3)/3;
  }
  check(second_turn_events==1&&second_turn_dialogues==15+arrival_speeches&&!second_turn_save.empty(),"second-turn event/dialogue was skipped or repeated");
  check(e.state().event_scans==1&&scripted_moves==12&&enemy_entries==1&&enemy_dialogues==4&&banner_columns==52,"enemy entry event/banner was skipped or repeated");
  check(attack_input_rng==transition.at("rng"),"phase transition changed original RNG");
  auto focus=transition.at("focus").at(0);auto path=battle_camera_path(0,0,focus.at(0),focus.at(1),20,20);
  check(player_camera==path.back(),"empty player phase camera differs from original");
  check(e.state().units.at("157").at("battle").at("lifecycle")==2,"tutorial guide did not appear after target highlights");
  auto moves=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_ai_movement.json");bool matched=false;
  for(const auto& m:moves.at("cases"))if(m.at("battle").at("slot")==20&&m.at("battle").at("ai")==1){check(moved_actor==m.at("after"),"integrated first AI movement differs from original");matched=true;}
  check(matched&&panel_frames>=2&&walking_frames>=10,"first AI selection/walking sequence missing");
  auto attack=read_json(std::filesystem::path(argv[1]).parent_path().parent_path()/"tests/fixtures/original_attack_controller.json").at("cases").at(0);
  check(critical_speeches==1&&first_attack.at("rng")==attack.at("rng"),"integrated attack RNG/speech order differs from original");
  check(first_attack.at("units").at("475").at("battle").at("hp")==45&&first_attack.at("units").at("234").at("battle").at("flags")==6,"first attack HP/action flags differ from original");
  check(first_attack.at("camera_x")==144&&first_attack.at("camera_y")==576&&first_attack.at("battle_poses").empty(),"attack camera or final redraw missing");
  check(enemy_decisions==13&&!e.state().script_fields.contains("ai_pending"),"enemy decision was repeated or missing");
  if(enemy_frame!=enemy_frames.size()||enemy_frame!=8)throw std::runtime_error("enemy walking frames skipped or duplicated: observed "+std::to_string(enemy_frame));
  check(second_frame==second_frames.size()&&second_frame>0,"second enemy walking frames skipped or duplicated");
  check(completed_attacks==8&&tactic_selections==2&&tactic_panels==2,"allied actions/tactic preview must each execute exactly once");check(e.state().rng.state==next_turn.at("after").at("rng"),"next-turn randomness changed");
  compare_original(objective_event.at("after"));check(confusion_frames==169&&e.state().variables[12]==1&&e.state().music==5,"confusion, fire event flag or arrival music missing");
  check(fire_waits==27&&summon_frames==30&&summon_saves==4&&e.state().mcall_resource==-1&&e.state().terrain==summon_event.at("after").at("terrain")&&e.state().terrain_effects==summon_event.at("after").at("records")&&e.state().terrain_marks==summon_event.at("after").at("effects"),"fire event skipped or repeated placement");
  // The final stable dialogue contains Zhang Fei's first fire. Reject corrupt
  // records/planes transactionally, even if an envelope checksum is recomputed.
  restored.restore(second_turn_save);auto fire_hash=restored.state_hash();
  for(int invalid=0;invalid<9;++invalid){auto bad=Json::parse(second_turn_save);auto& st=bad["payload"]["state"];
   if(invalid==0)st["terrain_effects"][0][0]=20;
   if(invalid==1)st["terrain_effects"][0][3]=254;
   if(invalid==2)st["terrain_effects"][0][4]=2;
   if(invalid==3)st["terrain_effects"][1]=st["terrain_effects"][0];
   if(invalid==4)st["terrain_marks"][113]=255;
   if(invalid==5)st["terrain"][113]=0;
   if(invalid==6)st["terrain_effects"][0][0]=13.5;
   if(invalid==7)st["terrain_marks"][113]=0.0;
   if(invalid==8)st["terrain_effects"].erase(st["terrain_effects"].begin());
   bad["sha256"]=sha256(bad["payload"].dump());rejects([&]{restored.restore(bad.dump());},"corrupt fire save accepted");check(restored.state_hash()==fire_hash,"corrupt fire save mutated state");
  }
  check(poison_frames==26&&tactic_casts==2&&tactic_captions==2&&e.state().battle_poses.empty(),"caster sequence duplicated or an old pose survived arrival redraws");
  check(!e.state().script_fields.contains("tactic_plan")&&secondary_frames==13&&no_attacks==6&&remaining_frames==remaining_frame_total,"secondary or movement-only action skipped/repeated");
  int original_batches=0;for(const auto& row:allied.at("trace"))if(row.at("kind")=="event_batch")++original_batches;
  check(post_batches==original_batches+25&&post_redraws==16,"post-action caller event/redraw order incomplete");
  check(e.state().script_fields.at("last_action")==Json::array({485,10,14})&&!e.state().script_fields.contains("event_return"),"post-action event notification/return missing");
  check(e.state().units.at("234").at("battle").at("lifecycle")==3&&e.state().units.at("210").at("battle").at("status")==16&&e.state().units.at("147").at("battle").at("mp")==40,"secondary poison/cost did not apply");
  check(e.state().units.at("475").at("battle").at("lifecycle")==3,"defeated enemy was not removed");
  check(e.state().units.at("0").at("battle").at("ai")==7,"new-battle party control mode");
  check(e.state().variables[500]==1&&e.state().controls.at("menu_enabled")==1,"opening introduction did not finish");
  for(auto id:{"235","334"})check(e.state().units.at(id).at("battle").at("lifecycle")==3,"scripted death missing");
  // A synthetic stable event interruption after the first completed attack
  // exercises the new serialized return point without adding a save-anywhere path.
  auto interrupted=Json::parse(event_save);auto& interrupted_state=interrupted["payload"]["state"];
  for(auto field:{"units","rng","camera_x","camera_y","script_fields"})interrupted_state[field]=first_attack.at(field);
  interrupted_state["battle_phase"]=1;interrupted_state["script_fields"]["event_return"]="ai_deaths";
  interrupted_state["script_fields"]["last_action"]=Json::array({234,9,17});
  interrupted["sha256"]=sha256(interrupted["payload"].dump());restored.restore(interrupted.dump());
  auto interrupted_hash=restored.state_hash();Engine resumed(argv[1]);resumed.restore(restored.serialize());
  for(int invalid=0;invalid<11;++invalid){auto bad=interrupted;auto& f=bad["payload"]["state"]["script_fields"];
   if(invalid==0)f["ai_cursor"]=-1;if(invalid==1)f["ai_cursor"]=116;if(invalid==2)f["ai_order"].push_back(f["ai_order"].at(0));
   if(invalid==3)f["event_return"]="phase_start";if(invalid==4)f["ai_pending"]["unit"]=999;
   if(invalid==5)f["ai_pending"]["destination"]=Json::array({20,0});if(invalid==6)f["last_action"]=Json::array({234,9,99});if(invalid==7)f.erase("ai_order");
   if(invalid==8)f["ai_cursor"]=1.5;if(invalid==9)f["ai_pending"]["destination"]=Json::array({1.5,0});if(invalid==10)f["ai_pending"]["unit"]=234.5;
   bad["sha256"]=sha256(bad["payload"].dump());rejects([&]{restored.restore(bad.dump());},"malformed post-action continuation accepted");check(restored.state_hash()==interrupted_hash,"invalid scheduler restore changed state");
  }
  for(int tick=0;tick<2000&&restored.state().phase!="blocked"&&restored.state().presentation.value("type","")!="tutorial_begin";++tick){
   if(restored.state().stable){restored.submit({"continue"});resumed.submit({"continue"});}else{restored.advance();resumed.advance();}
   check(restored.state_hash()==resumed.state_hash(),"saved event return diverged");
  }
  check(restored.state().presentation.value("type","")=="tutorial_begin"&&restored.state().rng.state==e.state().rng.state,"event return repeated or skipped an AI action");
  for(const auto& u:e.state().units)if(u.contains("battle"))check(restored.state().units.at(std::to_string(u.at("id").get<int>())).at("battle")==u.at("battle"),"event return changed final roster");
  // Required but unported recovery visuals must stop atomically, including RNG.
  auto guarded=Json::parse(event_save);auto& guarded_unit=guarded["payload"]["state"]["units"]["234"];
  guarded_unit["base_stats"][4]=100;guarded_unit["battle"]["grades"]=Json::array({6,2,3,3,3,3});
  guarded["sha256"]=sha256(guarded["payload"].dump());restored.restore(guarded.dump());
  auto before_rng=restored.state().rng.state;
  for(int tick=0;tick<100&&restored.state().phase!="blocked";++tick){if(restored.state().stable)restored.submit({"continue"});else restored.advance();}
  check(restored.state().diagnostic.find("recovery animation")!=std::string::npos,"unimplemented recovery visual was skipped");
  check(restored.state().rng.state==before_rng&&restored.state().units.at("234").at("battle").at("grades")==Json::array({6,2,3,3,3,3}),"failed phase preflight partially mutated state");
  restored.restore(old);check(restored.state().phase=="story","restore older snapshot");std::cout<<"choice "<<choice<<": "<<snapshots<<" snapshot continuations checked\n";
 }
 std::cout<<"Core tests passed (battle acceptance remains incomplete).\n";return 0;
}catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}}
