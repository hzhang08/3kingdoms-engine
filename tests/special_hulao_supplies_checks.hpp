#pragma once
Json supplies_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 const auto seal=[](Json e){e["sha256"]=sha256(e.at("payload").dump());return e.dump();};
 Json results=Json::array();
 for(int hero:{0,6,1,11})for(int enemy:{119,127})for(uint32_t draw=0;draw<(enemy==119?3u:1u);++draw){
  auto allies=enemy==119?Prototype::duel_allies_for_draw(hero,draw):std::array{-1,-1};Prototype g=original;
  g.start_hulao_duel(seed,hero,enemy,allies[0],allies[1],true);auto initial=g.serialize();
  check(g.inventory().at("87")==3&&Json::parse(initial).at("payload").at("schema")==17,"new special match lacks three beans");
  check(g.state().at("rng")==original.state().at("rng"),"starting beans consume combat RNG");
  special_roster::restore(g,initial,manifest);check(g.serialize()==initial,"loading adds starting beans twice");
  // Keep the enemy away while exercising three uses and each intervening turn.
  auto fixture=Json::parse(initial);int distant=-1,distance=-1;const auto foe=UnitRecord::from_json(g.state().at("units").at(std::to_string(enemy)).at("record"));
  for(int cell=0;cell<400;++cell){int d=std::abs(cell%20-10)+std::abs(cell/20-10);if(d>distance&&movement_cost(foe,g.terrain().at("cells").at(cell),g.data())!=255){distant=cell;distance=d;}}
  check(distant>=0,"no legal distant supply fixture tile");fixture["payload"]["state"]["units"][std::to_string(enemy)]["battle"]["x"]=distant%20;fixture["payload"]["state"]["units"][std::to_string(enemy)]["battle"]["y"]=distant/20;
  fixture["payload"]["state"]["units"][std::to_string(hero)]["battle"]["hp"]=1;g.restore(seal(fixture));std::string trace;
  for(int use=0;use<3;++use){
   auto before=g.serialize();Prototype copy=g;copy.restore(before);auto pos=g.state().at("units").at(std::to_string(hero)).at("battle");const int hp=pos.at("hp");
   for(auto* game:{&g,&copy}){game->command("tile",pos.at("x"),pos.at("y"));game->use_item(87,hero);presentation(*game);}
   check(g.serialize()==copy.serialize(),"bean use differs after restore");
   check(g.inventory().at("87")==2-use&&Json::parse(g.serialize()).at("payload").at("consumed_items").at("87")==use+1,"bean use stock/accounting wrong");
   check(g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==std::min(g.state().at("units").at(std::to_string(hero)).at("max_hp").get<int>(),hp+g.data().at("items").at(87).at("strength").get<int>()),"bean healing changed");
   auto saved=g.serialize();special_roster::restore(g,saved,manifest);check(g.serialize()==saved,"spent beans refill on restore");trace+=g.hash()+"\n";
   if(use<2){g.command("end_turn");presentation(g);for(int n=0;n<20&&g.state().at("phase")!="player"&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++n){g.enemy_step();presentation(g);}
    check(g.state().at("phase")=="player","supply fixture ended before third use");}
  }
  // A fresh-action fixture isolates zero inventory from the already-acted guard.
  auto fresh_action=Json::parse(g.serialize());fresh_action["payload"]["state"]["units"][std::to_string(hero)]["battle"]["flags"]=0;g.restore(seal(fresh_action));
  auto pos=g.state().at("units").at(std::to_string(hero)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));auto exhausted=g.serialize();bool rejected=false;try{g.use_item(87,hero);}catch(...){rejected=true;}check(rejected&&g.serialize()==exhausted,"fourth bean use succeeds or mutates state");
  for(int bad:{-1,0,4}){auto corrupt=Json::parse(initial);corrupt["payload"]["duel_starting_beans"]=bad;rejected=false;try{g.restore(seal(corrupt));}catch(...){rejected=true;}check(rejected&&g.serialize()==exhausted,"invalid starter grant accepted");}
  auto refill=Json::parse(exhausted);refill["payload"]["benefits"]["inventory"]["87"]=3;rejected=false;try{g.restore(seal(refill));}catch(...){rejected=true;}check(rejected&&g.serialize()==exhausted,"spent inventory can be refilled by restore");
  g.start_hulao_duel(seed,hero,enemy,allies[0],allies[1],true);check(g.serialize()==initial,"restart does not replenish exactly three beans");
  // Historical inventory is never topped up, even from a new supplied game.
  Prototype old=original;old.start_hulao_duel(seed,hero,enemy,allies[0],allies[1]);const auto historical=old.serialize();special_roster::restore(g,historical,manifest);check(g.serialize()==historical&&g.inventory().at("87")==0,"old save silently gains beans");
  results.push_back({{"hero",hero},{"enemy",enemy},{"allies",allies},{"three_uses",true},{"fourth_use_rejected",true},{"reload_preserves_stock",true},{"historical_save_unchanged",true},{"restart_exactly_three",true},{"trace",sha256(trace)}});
 }
 return {{"matches",results}};
}
Json opponent_supplies_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 auto seal=[](Json e){e["sha256"]=sha256(e.at("payload").dump());return e.dump();};
 Json matches=Json::array();
 for(int hero:{0,6,1,11})for(uint32_t draw=0;draw<3;++draw){
  Prototype g=original;g.start_hulao_squad(seed,hero,127,draw);const auto initial=g.serialize();auto p=Json::parse(initial).at("payload");
  check(p.at("schema")==19&&p.at("duel_starting_beans")==0&&g.inventory().at("87")==0,"Dong Zhuo still receives starter beans");
  check(g.duel_ally()==Prototype::duel_ally_for_draw(hero,draw)&&g.state().at("rng")==original.state().at("rng"),"supply change alters lineup or combat RNG");
  special_roster::restore(g,initial,manifest);check(g.serialize()==initial,"no-bean restore changed state");
  auto wounded=Json::parse(initial);wounded["payload"]["state"]["units"][std::to_string(hero)]["battle"]["hp"]=1;g.restore(seal(wounded));
  auto pos=g.state().at("units").at(std::to_string(hero)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));auto before=g.serialize();bool rejected=false;try{g.use_item(87,hero);}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"zero-stock bean use changes state");
  for(int grant:{-1,3}){auto bad=Json::parse(initial);bad["payload"]["duel_starting_beans"]=grant;rejected=false;try{g.restore(seal(bad));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"new Dong save accepts starter grant");}
  auto refill=Json::parse(initial);refill["payload"]["benefits"]["inventory"]["87"]=3;rejected=false;try{g.restore(seal(refill));}catch(...){rejected=true;}check(rejected&&g.serialize()==before,"zero-grant save can be refilled");
  Prototype copy=g;g.command("wait");copy.command("wait");presentation(g);presentation(copy);check(g.serialize()==copy.serialize(),"no-bean save continuation differs");
  g.command("end_turn");copy.command("end_turn");for(int n=0;n<10&&g.state().at("phase")!="player"&&g.state().at("phase")!="victory"&&g.state().at("phase")!="defeat";++n){g.enemy_step();copy.enemy_step();presentation(g);presentation(copy);check(g.serialize()==copy.serialize(),"no-bean next attack differs");}
  check(g.inventory().at("87")==0,"turn change grants beans");const auto continued=g.serialize();special_roster::restore(g,continued,manifest);check(g.serialize()==continued,"battle restore grants beans");
  // A previously started schema18 Dong match keeps its remaining supply.
  g.start_hulao_duel(seed,hero,127,Prototype::duel_ally_for_draw(hero,draw),-1,true,-1,true);auto old=Json::parse(g.serialize());old["payload"]["state"]["units"][std::to_string(hero)]["battle"]["hp"]=1;g.restore(seal(old));pos=g.state().at("units").at(std::to_string(hero)).at("battle");g.command("tile",pos.at("x"),pos.at("y"));g.use_item(87,hero);presentation(g);auto spent=g.serialize();special_roster::restore(g,spent,manifest);check(g.serialize()==spent&&g.inventory().at("87")==2,"older Dong save loses or gains beans");
  g.start_hulao_squad(seed,hero,127,draw);check(g.serialize()==initial,"restart inherited previous beans");
  auto all=Json::parse(initial);for(int id:{hero,g.duel_ally()}){auto& b=all["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;}all["payload"]["state"]["phase"]="defeat";g.restore(seal(all));g.start_hulao_squad(seed,hero,127,draw);check(g.serialize()==initial,"defeat restart grants beans");
  matches.push_back({{"hero",hero},{"ally",g.duel_ally()},{"initial_hash",sha256(initial)},{"continuation_hash",sha256(continued)},{"old_spent_save_hash",sha256(spent)},{"zero_stock_use_rejected",true},{"reload_no_refill",true}});
 }
 for(int hero:{0,6,1,11}){Prototype g=original;g.start_hulao_squad(seed,hero,119);check(g.inventory().at("87")==3&&Json::parse(g.serialize()).at("payload").at("schema")==18,"Lu Bu loses starter beans");auto saved=g.serialize();g.restore(saved);check(g.serialize()==saved,"Lu Bu grant changes on reload");}
 return {{"dong_matchups",matches},{"dong_starter_beans",0},{"lubu_starter_beans",3},{"historical_spent_inventory_preserved",true}};
}
