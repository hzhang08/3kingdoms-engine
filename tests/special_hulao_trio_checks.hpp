#pragma once
Json trio_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 auto seal=[](Json e){e["sha256"]=sha256(e.at("payload").dump());return e.dump();};
 auto fallen=[](Json& e,int id){auto& b=e["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;};
 Json matches=Json::array(),survivors=Json::array();
 for(int hero:{0,6,1,11}){
  std::set<std::array<int,2>> pairs;std::map<int,int> frequency;
  for(uint32_t draw=0;draw<3;++draw){
   const auto pair=Prototype::duel_allies_for_draw(hero,draw);const int a=pair[0],b=pair[1];
   check(a!=b&&a!=hero&&b!=hero&&Prototype::duel_hero_valid(a)&&Prototype::duel_hero_valid(b),"duplicate/invalid trio hero");pairs.insert(pair);++frequency[a];++frequency[b];
   Prototype g=original;g.start_hulao_duel(seed,hero,119,a,b);const auto initial=g.serialize();const auto start=Json::parse(initial);
   check(start.at("payload").at("schema")==16&&g.duel_ally2()==b,"trio identity not saved");
   int friendly=0,enemy=0;for(const auto& u:g.state().at("units"))if(u.at("battle").at("lifecycle")==2){if(u.at("battle").at("side")==0)++friendly;else ++enemy;}
   check(friendly==3&&enemy==1&&g.state().at("rng")==original.state().at("rng"),"trio roster or combat RNG changed");roster_check(g,manifest);
   // Every friendly gets an independent action; the third must not be skipped.
   int acted=0;for(int id:{hero,a,b}){
    auto pos=g.state().at("units").at(std::to_string(id)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));
    auto range=g.movement();int destination=-1;for(int cell=0;cell<400;++cell)if(range[cell]>0&&range[cell]!=255){bool occupied=false;for(const auto& u:g.state().at("units")){const auto& p=u.at("battle");if(p.at("lifecycle")==2&&p.at("x")==cell%20&&p.at("y")==cell/20)occupied=true;}if(!occupied){destination=cell;break;}}
    check(destination>=0,"trio unit has no move");g.command("tile",destination%20,destination/20);presentation(g);
    auto pending=g.serialize();Prototype copy=g;special_roster::restore(copy,pending,manifest);g.command("cancel");copy.command("cancel");check(g.serialize()==copy.serialize(),"trio pending move restore/cancel diverges");
    g.command("tile",pos.at("x"),pos.at("y"));g.command("wait");++acted;
    check(g.player_turn_complete()==(acted==3),"trio auto-end skips a unit");
    auto saved=g.serialize();special_roster::restore(g,saved,manifest);check(g.serialize()==saved,"trio action flags change on restore");
   }
   // Full matches keep the same decisions and RNG after every restoration.
   g.restore(initial);std::string trace;int steps=0;
   for(;steps<500&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++steps){Prototype copy=g;special_roster::restore(copy,g.serialize(),manifest);step(g);step(copy);check(g.serialize()==copy.serialize(),"trio save continuation diverges");roster_check(g,manifest);trace+=g.hash()+"\n";}
   check(steps<500,"trio match never ended");const auto outcome=g.serialize();special_roster::restore(g,outcome,manifest);check(g.serialize()==outcome,"trio outcome changes on restore");
   // Each of the three can be the last survivor; losing one or two is not defeat.
   for(int survivor:{hero,a,b}){
    auto solo=start;for(int id:{hero,a,b})if(id!=survivor)fallen(solo,id);
    solo["payload"]["state"]["units"]["119"]["battle"]["hp"]=1;solo["payload"]["state"]["rng"]=1;
    g.restore(seal(solo));check(g.state().at("phase")=="player"&&!g.player_turn_complete(),"last trio survivor cannot act");
    auto invalid=solo;invalid["payload"]["state"]["phase"]="defeat";bool rejected=false;auto before=g.serialize();try{g.restore(seal(invalid));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"premature trio defeat accepted");
    auto pos=g.state().at("units").at(std::to_string(survivor)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));g.command("wait");check(g.player_turn_complete(),"fallen trio teammates block auto-end");
    g.restore(seal(solo));std::string solo_trace;for(int n=0;n<100&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++n){Prototype copy=g;copy.restore(g.serialize());step(g);step(copy);check(g.serialize()==copy.serialize(),"trio survivor continuation mismatch");solo_trace+=g.hash()+"\n";}
    check(g.state().at("phase")=="victory","trio survivor cannot win");
    auto all=solo;fallen(all,survivor);all["payload"]["state"]["phase"]="defeat";g.restore(seal(all));auto lost=g.serialize();special_roster::restore(g,lost,manifest);check(g.serialize()==lost,"all-three defeat not restored");
    // A completed defeat cannot leak an old lineup into the next match.
    for(int opponent:{119,127}){Prototype fresh=original;int x=opponent==119?a:-1,y=opponent==119?b:-1;fresh.start_hulao_duel(seed,hero,opponent,x,y);g.start_hulao_duel(seed,hero,opponent,x,y);check(g.serialize()==fresh.serialize(),"trio defeat leaks into new match");}
    auto one=start;fallen(one,survivor);g.restore(seal(one));check(g.state().at("phase")=="player","one casualty ends trio battle");
    survivors.push_back({{"hero",hero},{"allies",pair},{"survivor",survivor},{"trace",sha256(solo_trace)},{"solo_victory",true},{"all_three_defeat",true}});
   }
   g.restore(initial);auto before=g.serialize();
   for(int bad:{-1,hero,a,119,32}){auto invalid=start;invalid["payload"]["duel_ally2"]=bad;bool rejected=false;try{g.restore(seal(invalid));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"invalid second ally accepted/mutated state");}
   auto downgrade=start;downgrade["payload"]["schema"]=15;downgrade["payload"]["engine"]="prototype-0.15-duel";downgrade["payload"]["layout"]="hulao-duel-team-v1";bool rejected=false;try{g.restore(seal(downgrade));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"second ally accepted in old schema");
   auto timeout=start;timeout["payload"]["state"]["turn"]=20;g.restore(seal(timeout));for(int i=0;i<10&&g.state().at("phase")!="defeat";++i){if(g.state().at("phase")=="player")g.command("end_turn");else g.enemy_step();presentation(g);}check(g.state().at("phase")=="defeat"&&g.state().at("turn_limit_reached").get<bool>(),"trio ignores turn limit");
   matches.push_back({{"hero",hero},{"allies",pair},{"steps",steps},{"result",Json::parse(outcome).at("payload").at("state").at("phase")},{"trace",sha256(trace)},{"three_independent_actions",true},{"save_each_action",true}});
  }
  check(pairs.size()==3,"trio draw omits an ally pair");for(int id:{0,6,1,11})if(id!=hero)check(frequency[id]==2,"trio pair selection is biased");
 }
 return {{"matchups",matches},{"survivors",survivors}};
}
