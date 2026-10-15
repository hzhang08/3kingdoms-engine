#pragma once
#include "caocao/unit_rules.hpp"
// The custom entrance is a deliberate two-cell adaptation. Verify against the
// immutable edition map and original class costs, not a blanket terrain bypass.
Json gate_checks(const Prototype& initial_game,const Prototype& original,const Json& manifest){
 auto initial=initial_game.serialize();auto r=UnitRecord::from_json(initial_game.state().at("units").at("0").at("record"));
 check(movement_cost(r,17,original.data())==255&&movement_cost(r,15,original.data())==255&&movement_cost(r,16,original.data())==1,"original gate/wall/courtyard costs");
 int changed=0;for(int p=0;p<400;++p)if(original.terrain().at("cells").at(p)!=initial_game.terrain().at("cells").at(p)){
  check((p==9*20+6||p==10*20+6)&&original.terrain().at("cells").at(p)==17&&initial_game.terrain().at("cells").at(p)==16,"non-entrance terrain changed");++changed;
 }check(changed==2,"two entrance tiles required");
 auto old=Json::parse(initial);old["payload"]["schema"]=11;old["payload"]["engine"]="prototype-0.11-duel";old["payload"]["layout"]="hulao-duel-v1";old["payload"]["duel_manifest"]="a31262e53727bc447f0c2bb9edebd9d13c3b205253b994c09d2ab00176fa7c4b";old["payload"].erase("duel_map");old["payload"].erase("duel_hero");old["payload"].erase("duel_enemy");
 old["payload"]["state"]["message"]="曹操 对 董卓：击败董卓，曹操不能战败。";old["sha256"]=sha256(old.at("payload").dump());
 check(sha256(old.dump())=="d90cda4394bc28e3fe7c295d53738931cbc81adc9c972c5e8947eedd85f71d76","historical schema11 fixture changed");
 Prototype legacy=initial_game;legacy.restore(old.dump());legacy.command("tile",10,10);
 check(legacy.movement().at(10*20+5)==255,"old closed gate reproduction");
 special_roster::restore(legacy,old.dump(),manifest);auto migrated=Json::parse(legacy.serialize()),expected=Json::parse(initial);migrated["payload"]["state"]["message"]=expected["payload"]["state"]["message"];check(migrated.at("payload")==expected.at("payload"),"schema11 upgrade changed units/RNG/progress");
 auto fixture=[&](int x,int y){auto e=Json::parse(initial);e["payload"]["state"]["units"]["0"]["battle"]["x"]=x;e["payload"]["state"]["units"]["0"]["battle"]["y"]=y;e["sha256"]=sha256(e.at("payload").dump());Prototype g=initial_game;g.restore(e.dump());return g;};
 Prototype g=initial_game;g.command("tile",10,10);check(g.movement().at(10*20+5)!=255,"courtyard unreachable");
 g.command("tile",5,10);auto path=g.take_movement().at("path");
 check(std::find(path.begin(),path.end(),Json::array({6,10}))!=path.end(),"walk did not traverse gate");
 auto inside=g.serialize();Prototype mirror=g;special_roster::restore(mirror,inside,manifest);check(mirror.serialize()==inside,"inside pending move restore");
 g.command("cancel");mirror.command("cancel");check(g.hash()==mirror.hash()&&g.state().at("units").at("0").at("battle").at("x")==10,"gate move undo");
 // Persisted occupancy on the entrance itself must validate in a fresh engine.
 g=initial_game;g.command("tile",10,10);g.command("tile",6,10);presentation(g);g.command("wait");mirror.restore(g.serialize());check(mirror.serialize()==g.serialize(),"gate tile save restore");
 g=fixture(5,10);g.command("tile",5,10);g.command("tile",10,10);auto exit=g.take_movement().at("path");check(std::find(exit.begin(),exit.end(),Json::array({6,10}))!=exit.end(),"exit route misses entrance");
 g=fixture(4,6);g.command("tile",4,6);check(g.movement().at(7*20+3)==255,"middle wall became walkable");g.command("tile",2,6);presentation(g);check(g.state().at("units").at("0").at("battle").at("x")==2,"inner courtyard passage blocked");
 g=fixture(5,10);g.command("end_turn");bool enemy_entered=false;Json enemy_paths=Json::array();
 for(int i=0;i<20&&!enemy_entered;++i){if(g.state().at("phase")=="player")g.command("end_turn");else g.enemy_step();auto p=g.take_movement();if(!p.is_null()){enemy_paths.push_back(p.at("path"));for(const auto& xy:p.at("path"))if(xy==Json::array({6,9})||xy==Json::array({6,10}))enemy_entered=true;}presentation(g);}
 check(enemy_entered,"enemy AI cannot use opened entrance");
 auto bad=Json::parse(initial);bad["payload"]["duel_map"]="unknown";bad["sha256"]=sha256(bad.at("payload").dump());g=initial_game;bool rejected=false;try{special_roster::restore(g,bad.dump(),manifest);}catch(...){rejected=true;}check(rejected&&g.serialize()==initial,"unknown terrain identity accepted");
 return {{"changed_cells",2},{"original_gate_block_reproduced",true},{"schema11_migration_preserves_state",true},{"entry_path",path},{"exit_path",exit},{"enemy_paths",enemy_paths},{"walls_blocked",true},{"interior_passage",true},{"gate_save_restore",true},{"inside_save_undo",true}};
}
