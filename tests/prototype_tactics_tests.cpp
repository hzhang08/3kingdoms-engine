#include "caocao/prototype.hpp"
#include "caocao/tactics.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/movement.hpp"
#include "prototype_tactics_fixture.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool pass,const std::string& why){if(!pass)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");auto assets=std::filesystem::path(argv[1]);auto root=assets.parent_path().parent_path();auto data=read_json(assets/"data.json");int cases=0;
 auto fixture=read_json(root/"tests/fixtures/original_prototype_tactics.json");for(const auto& c:fixture.at("cases")){
  auto a=BattleUnit::from_json(c.at("actor")),b=BattleUnit::from_json(c.at("target"));auto ar=UnitRecord::from_json(c.at("actor_unit")),br=UnitRecord::from_json(c.at("target_unit"));int t=c.at("tactic");OriginalRng rng{c.at("seed")};
  check(tactic_hit_chance(a,ar,b,br,t,data)==c.at("chance"),"original hit mismatch "+std::to_string(cases));
  check(tactic_damage(a,ar,b,br,t,true,data,rng)==c.at("damage")&&rng.state==c.at("rng"),"original damage/RNG mismatch "+std::to_string(cases));
  check(tactic_damage(a,ar,b,br,t,false,data,rng)==c.at("estimate"),"original estimate mismatch");++cases;
 }
 std::cout<<cases<<" original basic-tactic comparisons passed\n";
 Prototype game(assets);game.enable_tactics();std::string trace;Json casts=Json::array();int effects=0;
 for(int scenario=0;scenario<4;++scenario){game.start_scenario(scenario);while(game.has_dialogue())game.command("continue");prepare_caster_contact(game,scenario==0?146:scenario==1?135:132);int count=0;
  for(int n=0;n<2000&&game.state().at("phase")!="victory"&&game.state().at("phase")!="defeat";++n){
   if(game.has_dialogue()){game.command("continue");continue;}
   if(game.state().at("phase")=="player"){game.command("end_turn");continue;}
   auto request=game.enemy_request();if(!request.is_null()){
    if(request.at("state").at("actor")==132&&argc>2)std::ofstream(std::string(argv[2])+".liru-request.json")<<request.dump(2);
    check(request.at("state").at("protocol")=="caocao-tactics-v2","old JEV protocol");auto rng=game.state().at("rng");game.enemy_request();check(game.state().at("rng")==rng,"planning consumed RNG");
    game.choose_enemy(request.at("key"),"a0");
   }
   auto saved=game.serialize();Prototype replay=game;replay.restore(saved);game.enemy_step();replay.enemy_step();check(game.hash()==replay.hash(),"saved decision changed next action");
   auto packet=game.take_attack(),walk=game.take_movement();if(!packet.is_null()&&packet.value("kind",std::string())=="tactic"){
    ++count;casts.push_back({{"scenario",scenario},{"actor",packet.at("actor")},{"tactic",packet.at("tactic")},{"damage",packet.at("damage")},{"poison",packet.at("poison")}});
    const auto& before=packet.at("before").at("units").at(std::to_string(packet.at("actor").get<int>())).at("battle");const auto& after=game.state().at("units").at(std::to_string(packet.at("actor").get<int>())).at("battle");
    check(before.at("mp").get<int>()-after.at("mp").get<int>()==packet.at("mp_cost"),"MP not charged once");
    PrototypePlayback p;p.start(walk.is_null()?packet:walk,walk.is_null()?Json(nullptr):packet);int frames=0;
    while(p.active()){if(!p.spell_effect().is_null())++effects;p.advance(40);check(++frames<2000,"spell playback stuck");}
    auto post=game.serialize();replay.restore(post);check(replay.serialize()==post,"MP/poison snapshot failed");
   }
   trace+=game.hash()+"\n";if(count>0)break;
  }
  check(count>0,"no enemy tactic in scenario "+std::to_string(scenario));
 }
 // Progress-preserving opt-in migration does not rewrite old state or RNG.
 Prototype old(assets);old.start_scenario(1);auto save=old.serialize();auto state=old.state();old.enable_tactics();check(old.state()==state,"upgrade changed progress");Prototype restored=old;restored.restore(save);check(restored.serialize()==save,"schema5 compatibility changed");restored.enable_tactics();check(restored.state()==state,"loaded upgrade changed progress");
 auto bad=Json::parse(game.serialize());auto id=bad["payload"]["state"]["units"].begin().key();bad["payload"]["state"]["units"][id]["battle"]["mp"]=-1;bad["sha256"]=sha256(bad["payload"].dump());auto stable=game.serialize();bool rejected=false;try{game.restore(bad.dump());}catch(...){rejected=true;}check(rejected&&game.serialize()==stable,"corrupt MP accepted or mutated state");
 // Unavailable MP, illegal targets and unimplemented spell IDs are transactional.
 game.start_scenario(0);game.command("tile",game.state().at("units").at("0").at("battle").at("x"),game.state().at("units").at("0").at("battle").at("y"));
 auto low=Json::parse(game.serialize());low["payload"]["state"]["units"]["0"]["battle"]["mp"]=0;low["sha256"]=sha256(low.at("payload").dump());game.restore(low.dump());
 check(game.spells(0).empty(),"zero MP still offers a tactic");auto low_save=game.serialize();bool invalid=false;try{game.cast(10,146);}catch(...){invalid=true;}check(invalid&&game.serialize()==low_save,"zero-MP cast mutated state");
 invalid=false;try{game.cast(67,146);}catch(...){invalid=true;}check(invalid&&game.serialize()==low_save,"unsupported spell mutated state");
 // A restored poison status ticks once at the afflicted side's phase boundary.
 bool ticked=false;for(int seed=1;seed<40&&!ticked;++seed){
  game.start_scenario(0);auto p=Json::parse(game.serialize());auto& state=p["payload"]["state"];state["rng"]=seed;state["units"]["0"]["battle"]["status"]=16;state["phase"]="enemy";int enemies=0;for(const auto& u:state.at("units"))if(u.at("battle").at("side")==2)++enemies;state["enemy_cursor"]=enemies;p["sha256"]=sha256(p.at("payload").dump());game.restore(p.dump());
  int hp=game.state().at("units").at("0").at("battle").at("hp");auto pre=game.serialize();Prototype replay=game;replay.restore(pre);game.enemy_step();replay.enemy_step();check(game.hash()==replay.hash(),"poison recovery/HP replay changed");
  if(game.state().at("units").at("0").at("battle").at("status").get<int>()&16){check(game.state().at("units").at("0").at("battle").at("hp").get<int>()==hp-std::max(1,hp/10),"poison did not apply at player phase");auto once=game.serialize();game.enemy_step();check(game.serialize()==once,"poison applied twice");ticked=true;}
 }
 check(ticked,"poison test missed ongoing status");
 check(effects>0,"spell effect frames missing");Json report={{"original_cases",cases},{"casts",casts},{"effect_frames",effects},{"trace_sha256",sha256(trace)}};
 if(argc>2)std::ofstream(argv[2])<<report.dump(2);
 std::cout<<"four battles, legal JEV spells, MP/poison saves, migration, deterministic replay, effect playback: "<<sha256(trace)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
