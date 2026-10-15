#pragma once
#include "caocao/prototype_playback.hpp"
#include "caocao/movement.hpp"
void check_walking(const std::filesystem::path& assets,const std::filesystem::path& root){
 Prototype game(assets,false,false);int paths=0,frames=0,overlays=0,queued=0;
 auto verify=[&](const Json& packet){
  check(!packet.is_null()&&packet.at("kind")=="movement","missing walking packet");
  int id=packet.at("actor");auto key=std::to_string(id);const auto& b=packet.at("before").at("units").at(key).at("battle");
  const auto& after=game.state().at("units").at(key).at("battle");const auto& path=packet.at("path");
  check(path.size()>1&&path.front()==Json::array({b.at("x"),b.at("y")})&&path.back()==Json::array({after.at("x"),after.at("y")}),"walking path endpoints differ from simulation");
  auto record=UnitRecord::from_json(packet.at("before").at("units").at(key).at("record"));
  for(size_t n=1;n<path.size();++n){
   int x=path[n][0],y=path[n][1];
   check(std::abs(x-path[n-1][0].get<int>())+std::abs(y-path[n-1][1].get<int>())==1,"walking path skipped a tile");
   check(movement_cost(record,game.terrain().at("cells").at(y*20+x),game.data())!=255,"walking crossed impassable terrain");
   for(const auto& u:packet.at("before").at("units")){const auto& other=u.at("battle");
    check(!(other.at("lifecycle")==2&&(other.at("side")==2)!=(b.at("side")==2)&&other.at("x")==x&&other.at("y")==y),"walking crossed an opposing unit");
   }
  }
  PrototypePlayback playback;playback.start(packet);int observed=0;Json last=nullptr;
  while(playback.active()){
   for(const auto& pose:playback.poses()){
    check(pose.at("x").get<int>()>=0&&pose.at("x").get<int>()<=912&&pose.at("y").get<int>()>=56&&pose.at("y").get<int>()<=968,"walking frame outside full-map coordinates");
    if(pose.at("slot")==b.at("slot")){last=pose;++observed;}
   }
   playback.advance(40);check(++frames<1000000,"walking queue did not terminate");
  }
  check(!playback.moving(),"completed walk still reports movement");
  check(observed>=2&&!last.is_null()&&last.at("x")==after.at("x").get<int>()*48&&last.at("y")==after.at("y").get<int>()*48+56,"destination walking frame was not presented");
  for(const auto& e:packet.at("events"))if(e.at("kind")=="overlay")++overlays;
  ++paths;
 };
 for(bool legacy:{false,true}){
  game.start();if(legacy){std::ifstream f(root/"tests/fixtures/prototype-legacy/attack.snapshot.json");game.restore(std::string{std::istreambuf_iterator<char>(f),{}});game.command("cancel");}
  const auto original=game.serialize();const auto units=game.state().at("units");
  for(const auto& u:units)if(u.at("battle").at("side")!=2&&u.at("battle").at("hp").get<int>()>0){
   const auto& b=u.at("battle");game.restore(original);game.command("tile",b.at("x"),b.at("y"));
   const auto selected=game.serialize();auto costs=game.movement();
   for(int p=0;p<400;++p)if(costs[p]!=255&&costs[p]>0){
    bool occupied=false;for(const auto& other:units)if(other.at("battle").at("hp").get<int>()>0&&other.at("battle").at("x")==p%20&&other.at("battle").at("y")==p/20)occupied=true;
    if(occupied)continue;
    game.restore(selected);game.command("tile",p%20,p/20);auto packet=game.take_movement();verify(packet);
    check(game.take_movement().is_null(),"walking packet was not consume-once");
    auto saved=game.serialize();game.restore(saved);check(game.take_movement().is_null()&&game.serialize()==saved,"restore replayed or changed walking");
    game.command("cancel");check(game.state().at("units")==units,"walking changed movement cancellation");
   }
  }
 }
 check(overlays>0,"walking never exercised friendly pass-through overlays");
 // The enemy atomically moves and attacks; presentation must keep old HP and
 // play its walk completely before showing any weapon frame or hit sound.
 game.start();game.command("end_turn");while(game.state().at("phase")=="enemy")game.enemy_step();game.command("end_turn");
 while(game.state().at("phase")=="enemy"){
  game.enemy_step();auto movement=game.take_movement(),attack=game.take_attack();if(movement.is_null())continue;verify(movement);
  if(attack.is_null())continue;++queued;auto committed=game.serialize();PrototypePlayback playback;playback.start(movement,attack);
  auto target=std::to_string(attack.at("target").get<int>());auto hp=attack.at("before").at("units").at(target).at("battle").at("hp");
  while(playback.moving()){
   check(playback.active()&&playback.number().is_null()&&playback.take_sounds().empty(),"attack presentation started before walking finished");
   check(playback.before().at("units").at(target).at("battle").at("hp")==hp,"enemy walking revealed damage too early");
   playback.advance(40);
  }
  check(playback.active()&&playback.packet()==attack,"enemy walk did not transition to its attack");
  playback.advance(10000);check(!playback.active()&&game.serialize()==committed,"queued playback repeated simulation effects");
  playback.start(movement,attack);playback.clear();playback.advance(10000);check(!playback.active()&&playback.take_sounds().empty(),"clear retained queued attack");
 }
 check(queued>=2,"enemy walking test missed consecutive move/attack sequences");
 std::cout<<"walking: "<<paths<<" legal paths, "<<overlays<<" pass-through overlays, "<<queued<<" queued enemy attacks; full-map frames, final hold, cancel/restore and unchanged simulation passed\n";
}
