// Production-mode conversations, independent of the historical animation fixtures.
void check_dialogues(const std::filesystem::path& assets,const std::filesystem::path& root,const std::string& export_path){
 Prototype g(assets,true,false),mirror(assets,true,false);int pages=0,checkpoints=0;std::string trace;Json commands=Json::array();bool record=false;
 auto restore_payload=[&](Json envelope){envelope["sha256"]=sha256(envelope.at("payload").dump());g.restore(envelope.dump());};
 auto initial=Json::parse(g.serialize());
 auto adjacent=[&](){auto e=initial;e["payload"]["state"]["units"]["0"]["battle"]["x"]=9;e["payload"]["state"]["units"]["0"]["battle"]["y"]=12;restore_payload(e);};
 auto drain=[&](){Json seen=Json::array();while(g.has_dialogue()){
  auto page=g.dialogue();seen.push_back(page);auto snapshot=g.serialize();mirror.restore(snapshot);
  check(mirror.serialize()==snapshot&&mirror.hash()==g.hash()&&mirror.dialogue()==page,"dialogue page restoration differs");
  auto base=g.gameplay_hash();g.command("continue");mirror.command("continue");check(g.hash()==mirror.hash()&&g.gameplay_hash()==base,"dialogue continuation changed simulation");++pages;
 }return seen;};
 adjacent();g.command("tile",9,12);check(!g.has_dialogue(),"selecting triggered dialogue before commitment");g.command("wait");
 check(g.dialogue().at("event")==41&&g.dialogue().at("source_offset")==0x328e,"last eligible original encounter did not win");
 check(g.dialogue().at("speaker")=="曹操"&&g.dialogue().at("text")=="你就是张梁吧，\n虽然身为贼军，毕竟也是一军之将，\n干脆乖乖投降吧。","original Cao/Zhang Liang words changed");
 auto frozen=g.serialize();bool refused=false;try{g.command("end_turn");}catch(const std::exception&){refused=true;}g.enemy_step();
 check(refused&&g.serialize()==frozen,"pending dialogue did not pause commands/enemy actions");
 g.command("continue");check(g.dialogue().at("speaker")=="张梁"&&g.dialogue().at("unit")==147,"speaker/portrait lookup wrong");
 if(!export_path.empty()){
  std::ofstream(export_path+".dialogue.json")<<g.serialize();Prototype next=g;next.command("continue");
  std::ofstream(export_path+".dialogue.json.next.json")<<Json{{"command","continue"},{"x",0},{"y",0},{"hash",next.hash()}}.dump();
 }
 auto seen=drain();check(seen.size()==7&&seen.at(2).at("event")==40&&seen.at(2).at("text")=="事到如今还想顽抗吗？张宝！","diagonal Cao/Zhang Bao dialogue missing");
 auto completed=g.serialize();g.command("end_turn");check(!g.has_dialogue(),"completed encounters repeated");mirror.restore(completed);mirror.command("end_turn");check(g.hash()==mirror.hash(),"restored encounter flags changed next command");
 // A tentative move is reversible and must not consume either encounter flag.
 adjacent();auto e=Json::parse(g.serialize());e["payload"]["state"]["units"]["0"]["battle"]["y"]=13;restore_payload(e);
 g.command("tile",9,13);g.command("tile",9,12);check(!g.has_dialogue(),"tentative movement started dialogue");g.command("cancel");check(!g.has_dialogue(),"cancel evaluated the discarded destination");
 g.command("tile",9,13);g.command("tile",9,12);g.command("end_turn");check(g.has_dialogue()&&g.state().at("phase")=="enemy","end-turn commitment missed dialogue");
 frozen=g.serialize();g.enemy_step();check(g.serialize()==frozen,"enemy phase advanced behind dialogue");drain();
 // Re-signed structurally invalid snapshots must be rejected transactionally.
 adjacent();g.command("cancel");frozen=g.serialize();
 auto reject=[&](Json bad){bad["sha256"]=sha256(bad.at("payload").dump());bool bad_save=false;try{g.restore(bad.dump());}catch(const std::exception&){bad_save=true;}check(bad_save&&g.serialize()==frozen,"invalid dialogue snapshot changed live state");};
 auto bad=Json::parse(frozen);bad["payload"]["dialogue"]["page"]=999;reject(bad);
 bad=Json::parse(frozen);bad["payload"]["dialogue"]["pending"]={40,41};reject(bad);
 bad=Json::parse(frozen);bad["payload"]["dialogue"]["triggered"].push_back(41);reject(bad);
 bad=Json::parse(frozen);bad["payload"]["dialogue"]["pending"].push_back(99);reject(bad);
 bad=Json::parse(frozen);bad["payload"]["state"]["units"]["0"]["battle"]["y"]=13;reject(bad);
 bad=Json::parse(frozen);bad["payload"]["dialogue"]={{"triggered",{100}},{"pending",{100}},{"page",0}};reject(bad);
 bad=Json::parse(frozen);bad["payload"]["dialogue"]={{"triggered",{50}},{"pending",{50}},{"page",0}};reject(bad);
 // Each original retreat speech, with an otherwise legal synthetic casualty.
 for(auto [id,flag]:{std::pair{146,50},{147,51},{32,52},{3,53},{33,54}}){
  e=initial;auto& b=e["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;restore_payload(e);g.command("cancel");
  check(g.dialogue().at("event")==flag&&g.dialogue().at("unit")==id,"original retreat speech missing");drain();g.command("cancel");check(!g.has_dialogue(),"retreat speech repeated");
 }
 // Original victory branch: all three companions survive / one has retreated.
 for(bool companions:{true,false}){
  e=initial;auto& s=e["payload"]["state"];for(auto& u:s["units"]){auto& b=u["battle"];if(b["side"]==2||(!companions&&b["id"]==32)){b["hp"]=0;b["lifecycle"]=3;}}
  s["phase"]="victory";restore_payload(e);
  // Start one action before victory to exercise outcome -> speech scheduling.
  e=Json::parse(g.serialize());e["payload"]["state"]["phase"]="player";auto& last=e["payload"]["state"]["units"]["146"]["battle"];last["hp"]=1;last["lifecycle"]=2;
  e["payload"]["state"]["units"]["0"]["battle"]["x"]=9;e["payload"]["state"]["units"]["0"]["battle"]["y"]=12;restore_payload(e);
  g.command("tile",9,12);g.command("tile",9,11);check(g.state().at("phase")=="victory","last-enemy defeat missing");seen=drain();
  check(seen.back().at("event")== (companions?100:101),"original victory branch selection wrong");
  bool liubei=false;for(const auto& page:seen)liubei|=page.at("event")==100&&page.at("speaker")=="刘备";
  check(liubei==companions,"Cao/Liu victory exchange wrong");mirror.restore(g.serialize());check(!mirror.has_dialogue(),"completed victory speech replayed on load");
 }
 e=initial;e["payload"]["state"]["turn"]=20;restore_payload(e);g.command("end_turn");for(int n=0;n<15;++n)g.enemy_step();
 check(g.state().at("phase")=="defeat"&&g.dialogue().at("text")=="曹操败在黄巾军手下。","turn-limit defeat narration missing");drain();
 // Import both historical schemas without modifying their gameplay/RNG; write schema3 only on Save.
 for(const auto& path:{root/"tests/fixtures/prototype-legacy/attack.snapshot.json",root/"docs/evidence/prototype-linux.snapshot.json"}){
  auto old=read_json(path);Prototype baseline(assets,false,false);baseline.restore(old.dump());g.restore(old.dump());
  check(g.gameplay_hash()==baseline.hash()&&!g.has_dialogue()&&Json::parse(g.serialize()).at("payload").at("schema")==3,"older save migration changed gameplay");
  auto next=read_json(path.string()+".next.json");g.command(next.at("command"),next.at("x"),next.at("y"));baseline.command(next.at("command"),next.at("x"),next.at("y"));
  check(g.gameplay_hash()==baseline.hash(),"migration changed next strike or RNG");
 }
 // Complete production-mode victory/defeat with a fresh restore at every page/action.
 auto checkpoint=[&](){mirror.restore(g.serialize());check(mirror.hash()==g.hash(),"production restore differs");trace+=g.hash()+"\n";++checkpoints;};
 auto command=[&](const std::string& kind,int x=0,int y=0){auto phase=g.state().at("phase"),turn=g.state().at("turn");g.command(kind,x,y);mirror.command(kind,x,y);check(g.hash()==mirror.hash(),"production next command differs");checkpoint();if(record)commands.push_back({{"command",kind},{"x",x},{"y",y},{"phase",phase},{"turn",turn},{"hash",g.hash()}});};
 for(bool passive:{false,true}){
  record=!passive;g.start();checkpoint();
  for(int budget=0;budget<5000;++budget){
   if(g.has_dialogue()){command("continue");++pages;continue;}
   if(g.state().at("phase")=="victory"||g.state().at("phase")=="defeat")break;
   if(g.state().at("phase")=="enemy"){g.enemy_step();mirror.enemy_step();check(g.hash()==mirror.hash(),"production enemy replay differs");checkpoint();continue;}
   if(passive){command("end_turn");continue;}
   int id=-1;for(const auto& u:g.state().at("units")){const auto& b=u.at("battle");if(b.at("side")!=2&&b.at("hp").get<int>()>0&&b.at("flags")==0){id=b.at("id");break;}}
   if(id<0){command("end_turn");continue;}
   const auto b=g.state().at("units").at(std::to_string(id)).at("battle");const auto plan=g.suggest(id);command("tile",b.at("x"),b.at("y"));
   if(plan.at("x")!=b.at("x")||plan.at("y")!=b.at("y"))command("tile",plan.at("x"),plan.at("y"));
   if(plan.at("target")!=-1){const auto t=g.state().at("units").at(std::to_string(plan.at("target").get<int>())).at("battle");command("tile",t.at("x"),t.at("y"));}else command("wait");
  }
  check(g.state().at("phase")== (passive?"defeat":"victory")&&!g.has_dialogue(),"production dialogue playthrough did not finish");
  check(g.state().at("turn")== (passive?13:8),"dialogue changed battle outcome timing");
 }
 if(!export_path.empty())std::ofstream(export_path+".dialogue.commands.json")<<commands.dump();
 std::cout<<"production conversations pages="<<pages<<" checkpoints="<<checkpoints<<" trace_sha256="<<sha256(trace)<<"\n";
}
