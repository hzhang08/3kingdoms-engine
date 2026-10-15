#pragma once
// Isolated phase-boundary fixtures; never a user-save migration.
void camp_skip_enemy_actions(Prototype& g){
 auto e=Json::parse(g.serialize());auto& p=e["payload"];auto& s=p["state"];
 check(s.at("phase")=="enemy","camp fixture requires enemy phase");
 for(auto& u:s["units"])if(u.at("battle").at("side")==2)u["battle"]["flags"]=u.at("battle").at("flags").get<int>()|2;
 s["enemy_cursor"]=p.at("npc_order").size();e["sha256"]=sha256(p.dump());g.restore(e.dump());
}
Json camp_checks(const Prototype& original,const std::string& seed,const Json& manifest){
 Json results=Json::array();
 for(int hero:{0,6,1,11})for(int enemy:{119,127})for(int x:{8,19})for(int missing:{0,1,1000}){
  Prototype g=original;auto allies=enemy==119?Prototype::duel_allies_for_draw(hero,0):std::array{-1,-1};
  g.start_hulao_duel(seed,hero,enemy,allies[0],allies[1],true);
  auto e=Json::parse(g.serialize());auto& u=e["payload"]["state"]["units"][std::to_string(hero)];
  int maximum=u.at("max_hp"),before=std::max(1,maximum-missing),amount=std::min(maximum-before,maximum/5);
  u["battle"]["x"]=x;u["battle"]["y"]=9;u["battle"]["hp"]=before;
  e["sha256"]=sha256(e.at("payload").dump());g.restore(e.dump());
  check(g.healing_at(hero,x,9)==amount,"camp forecast differs from original 20 percent rule");
  g.command("tile",x,9);g.command("wait");presentation(g);
  check(g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==before,"camp heals immediately on wait");
  g.command("end_turn");presentation(g);g.enemy_step();presentation(g);
  check(g.state().at("phase")=="enemy"&&g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==before,"player healed at enemy turn start");
  camp_skip_enemy_actions(g);auto saved=g.serialize();Prototype copy=g;copy.restore(saved);
  g.enemy_step();copy.enemy_step();check(g.serialize()==copy.serialize(),"camp next turn differs after restore");
  check(g.state().at("phase")=="player"&&g.state().at("turn")==2,"camp fixture did not reach next player turn");
  check(g.state().at("units").at(std::to_string(hero)).at("battle").at("hp")==before+amount,"camp HP recovery missing");
  auto environment=g.take_environment();PrototypePlayback p;p.start(environment);bool seen=false;
  for(int n=0;p.active()&&n<1000;++n){if(!p.number().is_null()&&p.number().at("slot")==u.at("battle").at("slot")){check(p.number().at("healing")==true&&p.number().at("value")==amount,"wrong camp recovery number");seen=true;}p.advance(40);}
  check(seen==(amount>0),"camp recovery presentation missing/spurious");
  auto healed=g.serialize();special_roster::restore(g,healed,manifest);check(g.serialize()==healed,"load heals twice");
  results.push_back({{"hero",hero},{"enemy",enemy},{"camp",{x,9}},{"before",before},{"after",before+amount},{"maximum",maximum},{"number_shown",seen},{"state_hash",g.hash()}});
 }
 return {{"cases",results},{"rule","20% max HP, rounded down, at next own turn"}};
}
