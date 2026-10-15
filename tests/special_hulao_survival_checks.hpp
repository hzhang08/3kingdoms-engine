#pragma once
// Custom-team rules, not the original campaign's Cao Cao defeat condition.
Json survival_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 auto seal=[](Json e){e["sha256"]=sha256(e.at("payload").dump());return e.dump();};
 auto fallen=[](Json& e,int id){auto& b=e["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;};
 Json results=Json::array();
 for(int hero:{0,6,1,11})for(uint32_t draw=0;draw<3;++draw){
  int ally=Prototype::duel_ally_for_draw(hero,draw);Prototype base=original;base.start_hulao_duel(seed,hero,119,ally);
  auto start=Json::parse(base.serialize());
  for(int dead:{hero,ally}){
   int survivor=dead==hero?ally:hero;
   auto one=start;fallen(one,dead);one["payload"]["state"]["units"]["119"]["battle"]["hp"]=1;
   // Fix the combat draw for this outcome fixture; an ordinary miss/counter
   // can legitimately kill a weaker solo survivor and is not a rules failure.
   one["payload"]["state"]["rng"]=1;
   Prototype game=base;special_roster::restore(game,seal(one),manifest);
   check(game.duel_team_survival()&&game.state().at("phase")=="player"&&!game.player_turn_complete(),"first casualty ended battle/turn");
   auto solo=game.serialize();special_roster::restore(game,solo,manifest);check(game.serialize()==solo,"survivor save changed on load");
   const auto b=game.state().at("units").at(std::to_string(survivor)).at("battle");
   game.command("tile",b.at("x"),b.at("y"));check(game.state().at("selected")==survivor,"survivor cannot be selected");game.command("wait");
   check(game.player_turn_complete(),"dead teammate prevents automatic end turn");
   game.restore(solo);std::string trace;int n=0;
   for(;n<100&&game.state().at("phase")!="victory"&&game.state().at("phase")!="defeat";++n){
    Prototype copy=game;special_roster::restore(copy,game.serialize(),manifest);step(game);step(copy);
    check(game.serialize()==copy.serialize(),"surviving unit save/reload changed next action");trace+=game.hash()+"\n";
   }
   if(game.state().at("phase")!="victory")throw std::runtime_error("survivor victory fixture failed: hero="+std::to_string(hero)+" ally="+std::to_string(ally)+" fallen="+std::to_string(dead)+" state="+game.state().dump());
   auto won=game.serialize();special_roster::restore(game,won,manifest);check(game.serialize()==won,"survivor victory does not reload");
   auto both=one;fallen(both,survivor);both["payload"]["state"]["phase"]="defeat";
   special_roster::restore(game,seal(both),manifest);auto lost=game.serialize();special_roster::restore(game,lost,manifest);check(game.serialize()==lost,"both casualties defeat does not reload");
   auto invalid=one;invalid["payload"]["state"]["phase"]="defeat";bool rejected=false;
   try{game.restore(seal(invalid));}catch(...){rejected=true;}check(rejected&&game.serialize()==lost,"premature defeat accepted or mutated game");
   results.push_back({{"hero",hero},{"ally",ally},{"fallen",dead},{"survivor",survivor},{"solo_victory",true},{"all_fallen_defeat",true},{"premature_defeat_rejected",true},{"save_each_action",true},{"trace",sha256(trace)}});
  }
 }
 // Historical two-unit saves are validated first, then only live matches upgrade.
 Prototype game=original;game.start_hulao_duel(seed,0,119,11);auto old=Json::parse(game.serialize());
 old["payload"]["schema"]=14;old["payload"]["engine"]="prototype-0.14-duel";old["payload"]["layout"]="hulao-duel-ally-v1";
 auto old_text=seal(old);game.restore(old_text);check(!game.duel_team_survival()&&game.serialize()==old_text,"schema14 raw restore changed historical rules");
 special_roster::restore(game,old_text,manifest);auto upgraded=Json::parse(game.serialize());
 check(game.duel_team_survival()&&upgraded.at("payload").at("state")==old.at("payload").at("state")&&upgraded.at("payload").at("progress")==old.at("payload").at("progress"),"migration changes actions, RNG or events");
 fallen(old,0);old["payload"]["state"]["phase"]="defeat";old_text=seal(old);special_roster::restore(game,old_text,manifest);
 check(game.serialize()==old_text&&!game.duel_team_survival(),"historical completed defeat changed on load");
 // The 20-turn limit still applies with a living extra ally.
 game.start_hulao_duel(seed,0,119,11);auto timeout=Json::parse(game.serialize());fallen(timeout,0);timeout["payload"]["state"]["turn"]=20;
 game.restore(seal(timeout));
 for(int i=0;i<10&&game.state().at("phase")!="defeat";++i){if(game.state().at("phase")=="player")game.command("end_turn");else game.enemy_step();presentation(game);}
 check(game.state().at("phase")=="defeat"&&game.state().at("turn_limit_reached").get<bool>(),"team survives turn limit");
 auto expired=game.serialize();special_roster::restore(game,expired,manifest);check(game.serialize()==expired,"team timeout changed on load");
 return {{"survivors",results},{"schema14_migration",true},{"completed_historical_defeat_preserved",true},{"survivor_turn_limit",true}};
}
