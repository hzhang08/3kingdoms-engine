#ifndef __EMSCRIPTEN__
void revised_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save directory required");
 std::filesystem::create_directories(out);jev_enabled=false;manual_slots=true;Json report=Json::object();
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 auto drain=[](){for(int i=0;i<500;++i){if(game->has_dialogue())game->command("continue");else if(!game->story_choices().empty())game->choose_story(0);else return;}throw std::runtime_error("story loop");};
 Json rewards=Json::array();
 for(int scenario=0;scenario<2;++scenario){
  choose_battle(scenario);drain();auto terminal=Json::parse(game->serialize());
  for(auto& u:terminal["payload"]["state"]["units"]){int id=u.at("battle").at("id");if((scenario==0&&id==146)||(scenario==1&&id==134)){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}}
  terminal["sha256"]=sha256(terminal.at("payload").dump());restore_snapshot(terminal.dump());
  const auto cao=game->state().at("units").at("0").at("battle");check(send("tile",cao.at("x"),cao.at("y"))&&send("wait"),"reward event did not start");
  int expected=std::array{76,70}[scenario];bool rendered=false;int steps=0;
  for(;steps<2000;++steps){
   if(playback.active()){
    const auto& n=playback.notice();if(!n.is_null()&&n.value("item",-1)==expected&&!rendered){draw(out/("reward-"+std::to_string(expected)+".bmp"));rendered=true;}
    draw();playback.advance(40);for(int id:playback.take_sounds())attack_sound(id);
   }else if(game->has_dialogue())check(send("continue"),"reward dialogue failed");
   else if(!game->story_choices().is_null())select_story(0);
   else break;
  }
  check(rendered&&steps<2000,"treasure reward was not rendered");auto stable=game->serialize();restore_snapshot(stable);check(game->serialize()==stable,"reward restore changed state");rewards.push_back(expected);
 }
 // Render-only coverage for other equipment and the existing consumable
 // notices. This does not claim opcode89 currently queues an animated notice.
 for(int id:{24,64,87,88,92}){
  auto before=game->serialize();Json events=Json::array({{{"kind","notice"},{"actor",0},{"item",id},{"text","获得 "+game->data().at("items").at(id).at("name").get<std::string>()}},{{"kind","wait"},{"ticks",1}},{{"kind","notice_clear"}}});
  playback.start({{"kind","environment"},{"actor",0},{"before",game->state()},{"events",events}});begin_presentation();draw(out/("item-notice-"+std::to_string(id)+".bmp"));playback.advance(40);check(game->serialize()==before,"item icon rendering mutated state");
 }
 for(int n=0;n<4;++n){choose_battle(n);draw(out/("battle-"+std::to_string(n)+"-intro.bmp"));drain();draw(out/("battle-"+std::to_string(n)+".bmp"));}
 choose_battle(0);drain();terrain_fixture_place(*game,0,12,10);terrain_fixture_place(*game,474,13,10);
 auto seed=Json::parse(game->serialize());seed["payload"]["state"]["units"]["474"]["battle"]["status"]=0;seed["sha256"]=sha256(seed.at("payload").dump());game->restore(seed.dump());
 check(send("tile",12,10)&&send("tile",13,10),"counter fixture command failed");int ticks=0;bool counter=false;
 if(diagnostics.active()){
  auto checkpoint=read_json(diagnostics.directory()/"recovery-latest.json");
  diagnostic_checkpoint("animation_must_not_checkpoint");
  check(read_json(diagnostics.directory()/"recovery-latest.json")==checkpoint,"animation overwrote stable recovery");
 }
 while(playback.active()){
  if(!playback.notice().is_null()&&playback.notice().at("actor")==474&&!counter){draw(out/"counterattack.bmp");counter=true;}
  draw();playback.advance(40);for(int id:playback.take_sounds())attack_sound(id);check(++ticks<1200,"counter playback stalled");
 }
 drain();check(counter,"counter attack not presented");draw(out/"after-combat.bmp");
 if(diagnostics.active()){
  diagnostic_checkpoint("after_combat");auto checkpoint=read_json(diagnostics.directory()/"recovery-latest.json");Prototype restored=*game;restored.restore(checkpoint.dump());check(restored.hash()==game->hash(),"stable recovery does not match committed battle");
 }
 for(int n=1;n<=10;++n){save_slot=n;persist(false);check(notice=="已保存。","manual slot save failed");auto before=game->serialize();game->command("cancel");persist(true);check(game->serialize()==before,"manual slot restore changed state");}
 open_slots();draw(out/"ten-save-slots.bmp");show_slots=false;
 // Exercise the same callback completion paths used by native file dialogs.
 const auto snapshot=game->serialize();backup_import=false;backup_snapshot=snapshot;backup_path=(out/"backup.json").string();backup_ready=true;backup_busy=true;backup_finish();check(notice=="已导出备份。","backup export failed");
 game->command("cancel");backup_import=true;backup_ready=true;backup_busy=true;backup_finish();check(game->serialize()==snapshot,"backup import changed snapshot");
 std::ofstream(out/"corrupt.json")<<"{}";backup_path=(out/"corrupt.json").string();backup_ready=true;backup_busy=true;backup_finish();check(game->serialize()==snapshot,"corrupt backup mutated game");
 choose_battle(1);drain();terrain_fixture_place(*game,0,game->state().at("units").at("0").at("battle").at("x"),game->state().at("units").at("0").at("battle").at("y"),10);
 const auto b=game->state().at("units").at("0").at("battle");check(send("tile",b.at("x"),b.at("y")),"healer select failed");show_spells=true;draw(out/"extended-spells.bmp");show_spells=false;
 check(send("tactic",39,0),"healing cast failed");bool heal=false;int h=0;
 while(playback.active()){if(!playback.spell_effect().is_null()&&!heal){draw(out/"healing.bmp");heal=true;}draw();playback.advance(40);for(int id:playback.take_sounds())attack_sound(id);check(++h<1000,"healing playback stalled");}
 check(heal,"healing effect not presented");drain();
 // Exercise diagnostic boundaries that were previously missing: rejected
 // commands and an ordinary allied action through the frontend scheduler.
 choose_battle(0);drain();auto unchanged=game->serialize();
 check(!send("tile",-1,-1)&&game->serialize()==unchanged,"invalid command changed state");
 check(send("end_turn"),"diagnostic phase fixture failed");drain();
 check(advance_enemy(),"diagnostic allied action failed");collect_presentation();
 for(int n=0;playback.active()&&n<1000;++n){draw();auto generation=playback.generation();playback.advance(40);if(playback.active()&&playback.generation()!=generation)begin_presentation();for(int id:playback.take_sounds())attack_sound(id);}
 check(!playback.active(),"diagnostic allied playback stalled");diagnostic_checkpoint("npc_action_complete");
 report={{"version","0.9.2"},{"diagnostic_recovery_verified",diagnostics.active()},{"treasure_rewards_rendered",rewards},{"additional_item_icons_rendered",{24,64,87,88,92}},{"counterattack_presented",counter},{"healing_presented",heal},{"manual_slots",10},{"backup_restore",true},{"corrupt_backup_rejected",true},{"schema",Json::parse(game->serialize()).at("payload").at("schema")}};
 std::ofstream(out/"verification.json")<<report.dump(2);std::cout<<report.dump()<<'\n';
}
#endif
