#pragma once
Json squad_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 auto seal=[](Json e){e["sha256"]=sha256(e.at("payload").dump());return e.dump();};
 auto fallen=[](Json& e,int id){auto& b=e["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;};
 Json matches=Json::array(),survivors=Json::array();
 for(int hero:{0,6,1,11})for(int enemy:{127,119})for(uint32_t draw=0;draw<(enemy==127?3u:1u);++draw){
  Prototype g=original;g.start_hulao_squad(seed,hero,enemy,draw);const auto initial=g.serialize();const auto start=Json::parse(initial);
  std::vector<int> friends{hero,g.duel_ally()};if(enemy==119){friends.push_back(g.duel_ally2());friends.push_back(g.duel_ally3());}
  check(std::set<int>(friends.begin(),friends.end()).size()==friends.size(),"duplicate squad hero");
  check(start.at("payload").at("schema")== (enemy==127?19:18)&&g.inventory().at("87")== (enemy==127?0:3),"squad schema or beans");
  int friendly=0,foes=0;for(const auto& u:g.state().at("units"))if(u.at("battle").at("lifecycle")==2){if(u.at("battle").at("side")==0)++friendly;else ++foes;}
  check(friendly==(enemy==119?4:2)&&foes==1&&g.state().at("rng")==original.state().at("rng"),"squad roster or RNG changed");roster_check(g,manifest);
  int acted=0;for(int id:friends){
   auto pos=g.state().at("units").at(std::to_string(id)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));
   auto range=g.movement();int dest=-1;for(int cell=0;cell<400;++cell)if(range[cell]>0&&range[cell]!=255){bool occupied=false;for(const auto& u:g.state().at("units")){const auto& p=u.at("battle");if(p.at("lifecycle")==2&&p.at("x")==cell%20&&p.at("y")==cell/20)occupied=true;}if(!occupied){dest=cell;break;}}
   check(dest>=0,"squad hero has no move");g.command("tile",dest%20,dest/20);presentation(g);
   Prototype copy=g;copy.restore(g.serialize());g.command("cancel");copy.command("cancel");check(g.serialize()==copy.serialize(),"squad pending move/cancel restore");
   g.command("tile",pos.at("x"),pos.at("y"));g.command("wait");++acted;check(g.player_turn_complete()==(acted==friendly),"auto-end skipped a friendly");
  }
  g.restore(initial);std::string trace;int steps=0;
  for(;steps<500&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++steps){Prototype copy=g;special_roster::restore(copy,g.serialize(),manifest);step(g);step(copy);check(g.serialize()==copy.serialize(),"squad save continuation diverges");roster_check(g,manifest);trace+=g.hash()+"\n";}
  check(steps<500,"squad battle never ended");auto terminal=g.serialize();g.restore(terminal);check(g.serialize()==terminal,"squad terminal restore");
  matches.push_back({{"hero",hero},{"enemy",enemy},{"friends",friends},{"steps",steps},{"result",g.state().at("phase")},{"trace",sha256(trace)}});
  for(int survivor:friends){
   auto solo=start;for(int id:friends)if(id!=survivor)fallen(solo,id);
   solo["payload"]["state"]["units"][std::to_string(enemy)]["battle"]["hp"]=1;solo["payload"]["state"]["rng"]=1;
   g.restore(seal(solo));auto invalid=solo;invalid["payload"]["state"]["phase"]="defeat";auto before=g.serialize();bool rejected=false;try{g.restore(seal(invalid));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"live last survivor accepted as defeat");
   std::string solo_trace;for(int n=0;n<100&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++n){Prototype copy=g;copy.restore(g.serialize());step(g);step(copy);check(g.serialize()==copy.serialize(),"survivor RNG differs on restore");solo_trace+=g.hash()+"\n";}
   check(g.state().at("phase")=="victory","last squad survivor cannot win");
   auto all=solo;fallen(all,survivor);all["payload"]["state"]["phase"]="defeat";g.restore(seal(all));auto lost=g.serialize();special_roster::restore(g,lost,manifest);check(g.serialize()==lost,"all-fallen defeat restore");
   g.start_hulao_squad(seed,hero,enemy,draw);check(g.serialize()==initial,"defeat restart changed matchup");
   survivors.push_back({{"hero",hero},{"enemy",enemy},{"survivor",survivor},{"trace",sha256(solo_trace)}});
  }
  auto timeout=start;timeout["payload"]["state"]["turn"]=20;g.restore(seal(timeout));for(int n=0;n<10&&g.state().at("phase")!="defeat";++n){if(g.state().at("phase")=="player")g.command("end_turn");else g.enemy_step();presentation(g);}check(g.state().at("phase")=="defeat"&&g.state().at("turn_limit_reached").get<bool>(),"squad turn limit");
  for(int bad:{-1,hero,enemy}){auto invalid=start;invalid["payload"][enemy==119?"duel_ally3":"duel_ally"]=bad;bool rejected=false;auto before=g.serialize();try{g.restore(seal(invalid));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"invalid squad identity accepted");}
  auto old=original;old.start_hulao_duel(seed,hero,127);g.restore(old.serialize());check(g.serialize()==old.serialize()&&g.duel_ally3()==-1,"old save gains squad");
 }
 return {{"matchups",matches},{"last_survivors",survivors},{"dong_beans",0},{"lubu_beans",3},{"old_lineups_unchanged",true}};
}
