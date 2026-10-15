#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/voxel_board.hpp"
#include "caocao/special_hulao.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void drain(Prototype& g){for(int i=0;i<500;++i){if(g.has_dialogue())g.command("continue");else if(!g.story_choices().empty())g.choose_story(0);else return;}throw std::runtime_error("dialogue loop");}
void presentation(Prototype& g){PrototypePlayback p;p.start(g.take_movement(),g.take_attack(),g.take_environment());for(int n=0;p.active();++n){check(n<10000,"presentation stuck");p.advance(120);}}
void roster_check(const Prototype& g,const Json& manifest){
 for(const auto& u:g.state().at("units"))if(!special_roster::included(g,u.at("battle").at("id"))){
  int id=u.at("battle").at("id");check(u.at("battle").at("lifecycle")==3,"excluded unit reappeared");
  for(const auto& other:g.state().at("units")){int actor=other.at("battle").at("id");check(!g.attackable(actor,id)&&!g.attackable(id,actor),"excluded unit can fight");}
 }
}
void step(Prototype& g){
 drain(g);if(g.state().at("phase")!="player"){g.enemy_step();presentation(g);return;}
 int id=-1;for(const auto& u:g.state().at("units")){const auto& b=u.at("battle");if(b.at("side")==0&&b.at("lifecycle")==2&&b.at("hp").get<int>()>0&&!(b.at("flags").get<int>()&2)&&!(b.at("status").get<int>()&8)){id=b.at("id");break;}}
 if(id<0){g.command("end_turn");return;}
 const auto b=g.state().at("units").at(std::to_string(id)).at("battle");g.command("tile",b.at("x"),b.at("y"));auto plan=g.suggest(id);
 if(plan.at("x")!=b.at("x")||plan.at("y")!=b.at("y")){g.command("tile",plan.at("x"),plan.at("y"));presentation(g);}
 int target=plan.at("target");if(target>=0){auto t=g.state().at("units").at(std::to_string(target)).at("battle");g.command("tile",t.at("x"),t.at("y"));}else g.command("wait");presentation(g);
}
#include "special_hulao_gate_checks.hpp"
#include "special_hulao_hero_checks.hpp"
#include "special_hulao_survival_checks.hpp"
#include "special_hulao_trio_checks.hpp"
#include "special_hulao_supplies_checks.hpp"
#include "special_hulao_camp_checks.hpp"
#include "special_hulao_squad_checks.hpp"
int main(int argc,char** argv){try{
 check(argc>=2,"assets required");auto assets=std::filesystem::path(argv[1]);std::ifstream f(assets/"special-hulao/seed.json");std::string seed{std::istreambuf_iterator<char>(f),{}};
 auto manifest=read_json(assets/"special-hulao/manifest.json");Prototype original(assets);original.restore(seed);auto untouched=original.serialize();
 if(argc>3&&std::string(argv[3])=="export-starts"){
  Json starts=Json::object(),fixtures=Json::object();Prototype g=original;
  for(int hero:{0,6,1,11})for(int enemy:{119,127})for(int n=0;n<(enemy==119?3:1);++n){int ally=enemy==119?Prototype::duel_ally_for_draw(hero,n):-1;g.start_hulao_duel(seed,hero,enemy,ally);auto e=Json::parse(g.serialize());auto key=std::to_string(hero)+":"+std::to_string(enemy)+":"+std::to_string(ally);starts[key]=e.at("sha256");fixtures[key]=e;}
  for(int hero:{0,6,1,11})for(uint32_t n=0;n<3;++n){auto pair=Prototype::duel_allies_for_draw(hero,n);g.start_hulao_duel(seed,hero,119,pair[0],pair[1]);auto e=Json::parse(g.serialize());auto key=std::to_string(hero)+":119:"+std::to_string(pair[0])+":"+std::to_string(pair[1]);starts[key]=e.at("sha256");fixtures[key]=e;}
  for(int hero:{0,6,1,11})for(int enemy:{119,127})for(uint32_t n=0;n<(enemy==119?3u:1u);++n){auto pair=enemy==119?Prototype::duel_allies_for_draw(hero,n):std::array{-1,-1};g.start_hulao_duel(seed,hero,enemy,pair[0],pair[1],true);auto e=Json::parse(g.serialize());auto key=std::to_string(hero)+":"+std::to_string(enemy)+":"+std::to_string(pair[0])+(pair[1]>=0?":"+std::to_string(pair[1]):"")+":beans3";starts[key]=e.at("sha256");fixtures[key]=e;}
  for(int hero:{0,6,1,11})for(int enemy:{119,127})for(uint32_t n=0;n<(enemy==127?3u:1u);++n){if(enemy==127)g.start_hulao_duel(seed,hero,enemy,Prototype::duel_ally_for_draw(hero,n),-1,true,-1,true);else g.start_hulao_squad(seed,hero,enemy,n);auto e=Json::parse(g.serialize());auto key=std::to_string(hero)+":"+std::to_string(enemy)+":"+std::to_string(g.duel_ally())+(g.duel_ally2()>=0?":"+std::to_string(g.duel_ally2()):"")+(g.duel_ally3()>=0?":"+std::to_string(g.duel_ally3()):"")+":beans3:squad";starts[key]=e.at("sha256");fixtures[key]=e;}
  for(int hero:{0,6,1,11})for(uint32_t n=0;n<3;++n){g.start_hulao_squad(seed,hero,127,n);auto e=Json::parse(g.serialize());auto key=std::to_string(hero)+":127:"+std::to_string(g.duel_ally())+":beans0:squad";starts[key]=e.at("sha256");fixtures[key]=e;}
  if(argc>4)std::ofstream(argv[4])<<fixtures.dump();g.start_hulao_duel(seed);std::ofstream(argv[2])<<Json{{"starts",starts},{"template",Json::parse(g.serialize()).at("payload")}}.dump(2);return 0;
 }
 if(argc>3&&std::string(argv[3])=="opponent-supplies-only"){auto report=opponent_supplies_checks(original,seed,manifest);std::ofstream(argv[2])<<report.dump(2);return 0;}
 if(argc>3&&std::string(argv[3])=="squads-only"){auto report=squad_checks(original,seed,manifest);std::ofstream(argv[2])<<report.dump(2);return 0;}
 if(argc>3&&std::string(argv[3])=="team-only"){auto report=survival_checks(original,seed,manifest);std::ofstream(argv[2])<<report.dump(2);return 0;}
 if(argc>3&&std::string(argv[3])=="supplies-only"){auto report=supplies_checks(original,seed,manifest);std::ofstream(argv[2])<<report.dump(2);return 0;}
 if(argc>3&&std::string(argv[3])=="camps-only"){auto report=camp_checks(original,seed,manifest);std::ofstream(argv[2])<<report.dump(2);return 0;}
 Prototype game=original;game.start_hulao_duel(seed);game.enable_equipment();game.enable_tactics();game.enable_terrain_benefits();game.enable_items();
 check(game.scenario()==2&&game.state().at("units").size()==39,"Henry source records changed");auto initial=game.serialize();
 int active=0;std::array<int,3> armies{};for(const auto& u:game.state().at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==2){++active;++armies[b.at("side").get<int>()];}}
 check(active==2&&armies==std::array{1,0,1},"image roster counts");roster_check(game,manifest);
 check(game.state().at("selected")==-1&&game.state().at("inspected")==0,"removed selection retained");
 for(const auto& u:original.state().at("units")){auto id=u.at("battle").at("id").get<int>();const auto& b=game.state().at("units").at(std::to_string(id)).at("battle");for(const auto& key:{"hp","mp"})check(b.at(key)==u.at("battle").at(key),"duel changed stats");}
 check(game.state().at("rng")==original.state().at("rng")&&original.serialize()==untouched,"roster changed RNG/source game");
 special_roster::restore(game,initial,manifest);check(game.serialize()==initial,"roster migration not idempotent");
 check(game.state().at("units").at("0").at("record").at("level")==3,"Cao Cao level changed");
 auto gates=gate_checks(game,original,manifest);
 {const auto old=read_json(assets.parent_path().parent_path()/"tests/fixtures/special-hulao-schema13-lubu.json");Prototype legacy=game;legacy.start_hulao_duel(seed,0,119,6);special_roster::restore(legacy,old.dump(),manifest);check(Json::parse(legacy.serialize())==old&&legacy.duel_ally()==-1,"old Lu Bu save gains an ally or changes state");}
 {auto old12=read_json(assets.parent_path().parent_path()/"tests/fixtures/special-hulao-schema12.json");Prototype legacy=game;legacy.restore(old12.dump());check(Json::parse(legacy.serialize())==old12,"schema12 historical roundtrip");special_roster::restore(legacy,old12.dump(),manifest);auto upgraded=Json::parse(legacy.serialize()).at("payload");check(upgraded.at("schema")==13&&upgraded.at("state")==old12.at("payload").at("state")&&legacy.duel_hero()==0&&legacy.duel_enemy()==127,"schema12 migration changed saved match");}

 for(int y=0;y<20;++y)for(int x=0;x<20;++x){auto p=voxel::project(x+.5f,y+.5f);check(voxel::cell(p.x,p.y)==std::array{x,y},"projection center");}
 check(voxel::cell(-100,-100)==std::array{-1,-1},"outside projection");
 auto corrupt=initial;corrupt[corrupt.size()/2]^=1;bool rejected=false;try{special_roster::restore(game,corrupt,manifest);}catch(...){rejected=true;}check(rejected&&game.serialize()==initial,"corrupt save mutation");
 Prototype wrong(assets);wrong.enable_revised_rules();wrong.enable_equipment();rejected=false;try{special_roster::restore(game,wrong.serialize(),manifest);}catch(...){rejected=true;}check(rejected&&game.serialize()==initial,"other scenario mutated special");
 // Old full-army snapshots are never silently converted or overwritten.
 rejected=false;try{special_roster::restore(game,seed,manifest);}catch(...){rejected=true;}
 check(rejected&&game.serialize()==initial,"old special save loaded into duel");
 auto mutate=[&](Prototype& target,auto work){auto e=Json::parse(initial);work(e["payload"]);e["sha256"]=sha256(e.at("payload").dump());special_roster::restore(target,e.dump(),manifest);};
 rejected=false;try{mutate(game,[](Json& p){p["state"]["units"]["1"]["battle"]["lifecycle"]=2;});}catch(...){rejected=true;}
 check(rejected&&game.serialize()==initial,"third unit accepted in duel");
 rejected=false;try{mutate(game,[](Json& p){p["duel_manifest"]="old-art";});}catch(...){rejected=true;}
 check(rejected&&game.serialize()==initial,"incompatible duel manifest accepted");
 // A low-HP defeat fixture verifies the approaching, attacking Dong Zhuo.
 Prototype passive=game;mutate(passive,[](Json& p){p["state"]["units"]["0"]["battle"]["hp"]=1;});int passive_steps=0;for(;passive_steps<200&&passive.state().at("phase")!="defeat";++passive_steps){
  if(passive.state().at("phase")=="player")passive.command("end_turn");else passive.enemy_step();presentation(passive);roster_check(passive,manifest);
 }
 check(passive.state().at("phase")=="defeat"&&passive.state().at("units").at("0").at("battle").at("hp")==0,"Dong Zhuo did not approach and defeat passive player");
 auto defeated=passive.serialize();special_roster::restore(passive,defeated,manifest);check(passive.serialize()==defeated,"defeat restore changed");
 Prototype timeout=game;mutate(timeout,[](Json& p){p["state"]["turn"]=20;});
 for(int i=0;i<10&&timeout.state().at("phase")!="defeat";++i){if(timeout.state().at("phase")=="player")timeout.command("end_turn");else timeout.enemy_step();presentation(timeout);}
 check(timeout.state().at("phase")=="defeat"&&timeout.state().at("turn_limit_reached"),"turn-limit outcome");
 auto expired=timeout.serialize();special_roster::restore(timeout,expired,manifest);check(timeout.serialize()==expired,"timeout restore changed");
 std::string trace;int steps=0;
 for(;steps<2000;++steps){drain(game);auto phase=game.state().at("phase");if(phase=="victory"||phase=="defeat")break;
  auto saved=game.serialize();Prototype mirror=game;special_roster::restore(mirror,saved,manifest);step(game);step(mirror);roster_check(game,manifest);check(game.hash()==mirror.hash(),"save continuation diverged");trace+=game.hash()+"\n";
 }
 drain(game);std::cerr<<"duel result "<<game.state().at("phase")<<" Cao "<<game.state().at("units").at("0").at("battle").at("hp")<<" Dong "<<game.state().at("units").at("127").at("battle").at("hp")<<"\n";check(steps<2000&&game.state().at("phase")=="victory","duel did not reach victory");auto outcome=game.serialize();special_roster::restore(game,outcome,manifest);check(game.serialize()==outcome,"outcome duplicated event/reward");
 Json report{{"roster_version",manifest.at("roster").at("version")},{"active_units",active},{"armies",armies},{"source_records",39},{"legacy_saves_rejected_and_preserved",true},{"defeat_tested",true},{"turn_limit_tested",true},{"excluded_never_reappear",true},{"seed_sha256",sha256(seed)},{"start_hash",sha256(initial)},{"steps",steps},{"result",game.state().at("phase")},{"turn",game.state().at("turn")},{"trace_sha256",sha256(trace)},{"final_hash",game.hash()},{"save_restore_every_action",true},{"corrupt_rejected",true}};
 report["gates"]=gates;report["matchups"]=hero_checks(original,seed,manifest);
 report["team_survival"]=survival_checks(original,seed,manifest);
 report["trio"]=trio_checks(original,seed,manifest);
 report["supplies"]=supplies_checks(original,seed,manifest);
 report["camps"]=camp_checks(original,seed,manifest);
 if(argc>2){std::ofstream(argv[2])<<report.dump(2);std::ofstream(std::string(argv[2])+".snapshot.json")<<outcome;}
 std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
