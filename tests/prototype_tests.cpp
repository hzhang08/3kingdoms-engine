#include "caocao/prototype.hpp"
#include <fstream>
#include <iostream>
using namespace caocao;
void check(bool pass,const std::string& message){if(!pass)throw std::runtime_error(message);}
#include "prototype_playback_checks.hpp"
#include "prototype_walking_checks.hpp"
#include "prototype_dialogue_checks.hpp"
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");Prototype game(argv[1],false,false),mirror(argv[1],false,false);std::string trace,attack_snapshot;Json next_attack,replay=Json::array();int checkpoints=0;
 if(argc==4&&std::string(argv[2])=="--resume"){
  std::ifstream f(argv[3]);check(bool(f),"missing snapshot");std::string saved{std::istreambuf_iterator<char>(f),{}};game.restore(saved);
  auto next=read_json(std::string(argv[3])+".next.json");game.command(next.at("command"),next.at("x"),next.at("y"));
  check(game.hash()==next.at("hash").get<std::string>(),"cross-process next attack differs");
  std::cout<<"fresh-process snapshot and next attack passed "<<game.hash()<<'\n';return 0;
 }
 auto root=std::filesystem::path(argv[1]).parent_path().parent_path();
 check_dialogues(argv[1],root,argc>2?argv[2]:"");
 check_playback(argv[1],root);
 check_walking(argv[1],root);
 auto layout=read_json(root/"tests/fixtures/prototype-electron-layout.json");
 check(game.state().at("units").size()==22&&game.state().at("turn")==2,"reference roster/turn mismatch");
 for(const auto& u:layout.at("units")){
  const auto& b=game.state().at("units").at(std::to_string(u.at("id").get<int>())).at("battle");
  for(auto k:{"id","slot","side","x","y","direction","status"})check(b.at(k)==u.at(k),std::string("reference placement mismatch: ")+k);
 }
 int fires=0;for(auto t:game.terrain().at("cells"))if(t==26)++fires;
 check(fires==27,"fire count mismatch");for(const auto& f:layout.at("fires"))check(game.terrain().at("cells").at(f.at("y").get<int>()*20+f.at("x").get<int>())==26,"fire coordinate mismatch");
 for(const auto& u:layout.at("units"))if(u.at("side")!=2){
  game.command("tile",u.at("x"),u.at("y"));auto range=game.movement();
  for(const auto& f:layout.at("fires"))check(range.at(f.at("y").get<int>()*20+f.at("x").get<int>())==255,"movement enters fire");
  game.command("cancel");
 }
 game.start();auto original_units=game.state().at("units");game.command("end_turn");
 for(int n=0;n<14;++n){game.enemy_step();mirror.restore(game.serialize());check(mirror.hash()==game.hash(),"confusion restoration mismatch");}
 for(const auto& u:game.state().at("units")){const auto& b=u.at("battle");const auto& old=original_units.at(std::to_string(b.at("id").get<int>())).at("battle");check(b.at("x")==old.at("x")&&b.at("y")==old.at("y")&&b.at("hp")==old.at("hp")&&b.at("status")==0,"initial confusion did not skip action");}
 game.start();
 // Legacy restoration must preserve the exact historical next strike/RNG.
 auto oldpath=root/"tests/fixtures/prototype-legacy/attack.snapshot.json";
 std::ifstream oldfile(oldpath);check(bool(oldfile),"legacy fixture missing");std::string oldtext{std::istreambuf_iterator<char>(oldfile),{}};
 mirror.restore(oldtext);check(mirror.legacy()&&mirror.serialize()==oldtext,"legacy snapshot rewritten");
 auto oldnext=read_json(oldpath.string()+".next.json");mirror.command(oldnext.at("command"),oldnext.at("x"),oldnext.at("y"));check(mirror.hash()==oldnext.at("hash").get<std::string>(),"legacy next strike changed");
 auto before_legacy=mirror.serialize();auto bad=Json::parse(game.serialize());bad["payload"]["state"]["units"]["0"]["battle"]["hp"]=999;bad["sha256"]=sha256(bad.at("payload").dump());
 bool invalid=false;try{mirror.restore(bad.dump());}catch(const std::exception&){invalid=true;}check(invalid&&mirror.serialize()==before_legacy&&mirror.legacy(),"failed cross-ruleset load changed state");
 mirror.command("restart");check(!mirror.legacy()&&mirror.state().at("units").size()==22,"legacy restart did not select updated layout");
 // Exercise the new limit without inventing combat outcomes; supply a valid turn20 stable snapshot.
 auto last=Json::parse(game.serialize());last["payload"]["state"]["turn"]=20;last["sha256"]=sha256(last.at("payload").dump());
 mirror.restore(last.dump());mirror.command("end_turn");for(int n=0;n<15;++n)mirror.enemy_step();
 check(mirror.state().at("phase")=="defeat"&&mirror.state().at("turn_limit_reached")==true,"turn-limit defeat missing");
 game.restore(mirror.serialize());check(game.hash()==mirror.hash(),"turn-limit save mismatch");game.start();
 auto checkpoint=[&](){mirror.restore(game.serialize());check(mirror.hash()==game.hash(),"snapshot state mismatch");trace+=game.hash()+"\n";++checkpoints;};
 checkpoint();game.command("tile",10,6);auto costs=game.movement();int dest=-1;
 for(int p=0;p<400;++p)if(costs[p]!=255&&costs[p]>0){bool occupied=false;for(const auto& u:game.state().at("units"))if(u.at("battle").at("x")==p%20&&u.at("battle").at("y")==p/20)occupied=true;if(!occupied){dest=p;break;}}
 check(dest>=0,"no legal move");auto before=game.state().at("units");game.command("tile",dest%20,dest/20);checkpoint();
 game.command("cancel");mirror.command("cancel");check(game.state().at("units")==before&&game.hash()==mirror.hash(),"move rollback/restored rollback differs");
 auto good=game.serialize();auto corrupt=Json::parse(good);corrupt["payload"]["state"]["rng"]=987;bool rejected=false;
 try{game.restore(corrupt.dump());}catch(const std::exception&){rejected=true;}check(rejected&&game.serialize()==good,"corrupt snapshot mutated game");
 corrupt=Json::parse(good);corrupt["payload"]["schema"]=999;corrupt["sha256"]=sha256(corrupt.at("payload").dump());rejected=false;
 try{game.restore(corrupt.dump());}catch(const std::exception&){rejected=true;}check(rejected&&game.serialize()==good,"incompatible snapshot mutated game");
 corrupt=Json::parse(good);corrupt["payload"]["state"]["units"]["0"]["battle"]["hp"]=999;corrupt["sha256"]=sha256(corrupt.at("payload").dump());rejected=false;
 try{game.restore(corrupt.dump());}catch(const std::exception&){rejected=true;}check(rejected&&game.serialize()==good,"invalid snapshot mutated game");
 game.start();checkpoint();
 auto command=[&](const std::string& kind,int x=0,int y=0){auto turn=game.state().at("turn");game.command(kind,x,y);mirror.command(kind,x,y);check(game.hash()==mirror.hash(),"command after reload differs");checkpoint();replay.push_back({{"command",kind},{"x",x},{"y",y},{"turn",turn},{"hash",game.hash()}});};
 for(int budget=0;budget<4000&&game.state().at("phase")!="victory"&&game.state().at("phase")!="defeat";++budget){
  if(game.state().at("phase")=="enemy"){game.enemy_step();mirror.enemy_step();check(game.hash()==mirror.hash(),"next enemy action after reload differs");checkpoint();continue;}
  int id=-1;for(const auto& u:game.state().at("units")){const auto& b=u.at("battle");if(b.at("side")!=2&&b.at("hp").get<int>()>0&&b.at("flags")==0){id=b.at("id");break;}}
  if(id<0){command("end_turn");continue;}
  auto b=game.state().at("units").at(std::to_string(id)).at("battle");auto plan=game.suggest(id);command("tile",b.at("x"),b.at("y"));
  if(plan.at("x")!=b.at("x")||plan.at("y")!=b.at("y"))command("tile",plan.at("x"),plan.at("y"));
  if(plan.at("target")!=-1){auto t=game.state().at("units").at(std::to_string(plan.at("target").get<int>())).at("battle");bool capture=attack_snapshot.empty();if(capture)attack_snapshot=game.serialize();command("tile",t.at("x"),t.at("y"));if(capture)next_attack={{"command","tile"},{"x",t.at("x")},{"y",t.at("y")},{"hash",game.hash()}};}
  else command("wait");
 }
 std::cout<<"autoplay outcome="<<game.state().at("phase")<<" turn="<<game.state().at("turn")<<"\n";
 check(game.state().at("phase")=="victory","autoplay did not reach victory");
 if(argc>2){std::ofstream out(argv[2]);out<<attack_snapshot;check(bool(out),"snapshot export failed");std::ofstream next(std::string(argv[2])+".next.json");next<<next_attack.dump();check(bool(next),"next-action export failed");std::ofstream commands(std::string(argv[2])+".commands.json");commands<<replay.dump();check(bool(commands),"command export failed");}
 game.start();checkpoint();
 for(int n=0;n<2000&&game.state().at("phase")!="defeat";++n){if(game.state().at("phase")=="player")command("end_turn");else{game.enemy_step();mirror.enemy_step();check(game.hash()==mirror.hash(),"defeat continuation diverged");checkpoint();}}
 check(game.state().at("phase")=="defeat","passive play did not reach defeat");
 std::cout<<"defeat turn="<<game.state().at("turn")<<" checkpoints="<<checkpoints<<" trace_sha256="<<sha256(trace)<<"\n";
 game.restore(good);check(game.serialize()==good,"older snapshot restore differs");game.command("restart");check(game.state().at("phase")=="player"&&game.state().at("turn")==2,"restart failed");
 std::cout<<"victory/defeat, undo, stable snapshots, next-action determinism, corruption/version/structure rejection and restart passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
