#pragma once
#include <set>
Json hero_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 Json runs=Json::array();
 for(int hero:{0,6,1,11}){std::set<int> choices;for(uint32_t draw=0;draw<3;++draw){int ally=Prototype::duel_ally_for_draw(hero,draw);check(Prototype::duel_hero_valid(ally)&&ally!=hero,"invalid random ally");choices.insert(ally);}check(choices.size()==3,"ally draw omits a remaining hero");}
 for(int hero:{0,6,1,11})for(int enemy:{119,127})for(int ally_index=0;ally_index<(enemy==119?3:1);++ally_index){
  const int ally=enemy==119?Prototype::duel_ally_for_draw(hero,ally_index):-1;
  Prototype g=original;g.start_hulao_duel(seed,hero,enemy,ally);const auto start=g.serialize();auto p=Json::parse(start).at("payload");
  check(p.at("schema")== (ally>=0?15:13)&&g.duel_ally()==ally&&p.at("duel_hero")==hero&&p.at("duel_enemy")==enemy,"match identities not saved");
  int active=0;for(const auto& u:g.state().at("units")){const int id=u.at("battle").at("id");
   check(u.at("record")==original.state().at("units").at(std::to_string(id)).at("record"),"Henry equipment/class/level changed");
   if(u.at("battle").at("lifecycle")==2){++active;check(g.duel_includes(id),"third deployed fighter");}
  }check(active==(ally>=0?3:2)&&g.state().at("rng")==original.state().at("rng"),"match selection changed combat RNG");
  check(g.state().at("inspected")==hero,"inspection does not start on selected hero");roster_check(g,manifest);
  // Exercise both player-controlled units with original movement/attack rules.
  for(int attacker:std::vector<int>{hero,ally}){if(attacker<0)continue;g.restore(start);
  auto pos=g.state().at("units").at(std::to_string(attacker)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));
  bool found=false;auto range=g.movement();std::set<int> occupied;
  for(const auto& u:g.state().at("units")){const auto& b=u.at("battle");if(b.at("id")!=attacker&&b.at("lifecycle")==2)occupied.insert(b.at("y").get<int>()*20+b.at("x").get<int>());}
  for(int cell=0;cell<400&&!found;++cell)if(range[cell]!=255&&!occupied.contains(cell)){Prototype candidate=g;
   if(cell!=pos.at("y").get<int>()*20+pos.at("x").get<int>()){candidate.command("tile",cell%20,cell/20);presentation(candidate);}
   if(!candidate.attackable(attacker,enemy))continue;
   auto target=candidate.state().at("units").at(std::to_string(enemy)).at("battle");candidate.command("tile",target.at("x"),target.at("y"));
   auto attack=candidate.take_attack();check(!attack.is_null(),"hero attack packet absent");
   bool authored=false;for(const auto& e:attack.at("events"))if(e.value("archive",std::string())=="UNIT_ATK.E5"&&e.at("slot")==pos.at("slot"))authored=true;
   check(authored,"hero attack animation absent");found=true;
  }check(found,"player unit cannot move into attack range");
  }
  if(ally>=0){
   g.restore(start);const auto h=g.state().at("units").at(std::to_string(hero)).at("battle");g.command("tile",h.at("x"),h.at("y"));g.command("wait");
   check(!g.player_turn_complete(),"turn ends before extra ally acts");
   auto half=g.serialize();Prototype copy=g;copy.restore(half);check(copy.duel_ally()==ally&&!copy.player_turn_complete(),"load loses ally action");
   const auto a=g.state().at("units").at(std::to_string(ally)).at("battle");g.command("tile",a.at("x"),a.at("y"));g.command("wait");
   check(g.player_turn_complete(),"two completed player actions do not finish turn");
  }
  g.restore(start);std::string trace;int steps=0;
  for(;steps<500&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++steps){
   Prototype copy=g;special_roster::restore(copy,g.serialize(),manifest);check(copy.duel_hero()==hero&&copy.duel_enemy()==enemy&&copy.duel_ally()==ally,"load rerolled matchup");
   step(g);step(copy);check(g.hash()==copy.hash(),"match continuation changed RNG/event state");roster_check(g,manifest);trace+=g.hash()+"\n";
  }check(steps<500,"match never ended");
  auto end=g.serialize();special_roster::restore(g,end,manifest);check(g.serialize()==end,"outcome restoration changed match");
  // Passive players lose only after all deployed friendlies fall (or timeout).
  auto low=Json::parse(start);low["payload"]["state"]["units"][std::to_string(hero)]["battle"]["hp"]=1;low["sha256"]=sha256(low.at("payload").dump());g.restore(low.dump());
  for(int n=0;n<100&&g.state().at("phase")!="defeat";++n){if(g.state().at("phase")=="player")g.command("end_turn");else g.enemy_step();presentation(g);
   if(ally>=0&&g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==0&&g.state().at("units").at(std::to_string(ally)).at("battle").at("hp").get<int>()>0&&!g.state().at("turn_limit_reached").get<bool>()){
    check(g.state().at("phase")!="defeat","primary casualty ends battle before ally falls");auto snapshot=g.serialize();special_roster::restore(g,snapshot,manifest);check(g.serialize()==snapshot,"first casualty changes on reload");
   }
  }
  check(g.state().at("phase")=="defeat","passive match never reaches defeat");
  // A defeated game must reset exactly like a fresh one for either new opponent.
  // Use both draw branches rather than a probabilistic/flaky distribution test.
  for(uint32_t draw:{0u,1u}){
   const int next_enemy=Prototype::duel_enemy_for_draw(draw);
   Prototype restarted=g,fresh=original;
   const int next_ally=next_enemy==119?Prototype::duel_ally_for_draw(hero,ally_index):-1;
   restarted.start_hulao_duel(seed,hero,next_enemy,next_ally);fresh.start_hulao_duel(seed,hero,next_enemy,next_ally);
   check(restarted.serialize()==fresh.serialize(),"defeat leaks into restarted matchup");
   check(restarted.duel_enemy()==next_enemy&&restarted.state().at("phase")=="player","defeat restart ignores opponent draw");
  }
  if(ally>=0&&!g.state().at("turn_limit_reached").get<bool>())check(g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==0&&g.state().at("units").at(std::to_string(ally)).at("battle").at("hp")==0,"defeat while a friendly survives");
  auto before=g.serialize();
  if(ally>=0){
   for(int bad:{-1,hero,enemy,32}){auto broken=Json::parse(start);broken["payload"]["duel_ally"]=bad;broken["sha256"]=sha256(broken.at("payload").dump());bool rejected=false;try{g.restore(broken.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"invalid ally mutates match");}
   auto dead=Json::parse(start);auto& b=dead["payload"]["state"]["units"][std::to_string(ally)]["battle"];b["hp"]=0;b["lifecycle"]=3;dead["sha256"]=sha256(dead.at("payload").dump());Prototype survivor=g;survivor.restore(dead.dump());
   check(survivor.state().at("phase")=="player"&&survivor.duel_ally()==ally,"bonus ally death loses or rerolls match");
  }
  auto invalid=Json::parse(start);invalid["payload"]["duel_hero"]=enemy;invalid["sha256"]=sha256(invalid.at("payload").dump());bool rejected=false;try{g.restore(invalid.dump());}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"invalid hero mutates snapshot");
  rejected=false;try{g.start_hulao_duel(seed,32,enemy);}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"invalid new roster accepted");
  runs.push_back({{"hero",hero},{"enemy",enemy},{"ally",ally},{"steps",steps},{"result",Json::parse(end).at("payload").at("state").at("phase")},{"trace",sha256(trace)},{"final_hash",sha256(end)},{"save_each_action",true},{"all_friendlies_defeat",true},{"restart_after_defeat_both_opponents",true},{"attack_animated",true}});
 }
 check(Prototype::duel_enemy_for_draw(0)==127&&Prototype::duel_enemy_for_draw(1)==119&&Prototype::duel_enemy_for_draw(UINT32_MAX)==119,"opponent draw mapping");
 return runs;
}
