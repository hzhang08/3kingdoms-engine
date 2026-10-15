void special_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save directory required");
 restart_special();std::filesystem::create_directories(out);auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
 auto advance=[&](int ms){auto previous=playback.generation();playback.advance(ms);if(playback.active()&&previous!=playback.generation())begin_presentation();};
 int movements=0,attacks=0;bool captured=false;std::set<int> attacking,walking,reacting;
 Json frames=Json::array();std::set<std::string> seen_frames;
 auto present=[&](){
  auto before=game->serialize();
  for(const auto& pose:playback.poses()){
   int id=-1;for(const auto& u:playback.before().at("units"))if(u.at("battle").at("slot")==pose.at("slot"))id=u.at("battle").at("id");
   if(id!=0&&id!=127)throw std::runtime_error("unexpected animated character");
   auto archive=pose.at("archive").get<std::string>();
   if(archive=="UNIT_ATK.E5")attacking.insert(id);
   if(archive=="UNIT_MOV.E5"&&playback.moving()&&id==playback.packet().at("actor"))walking.insert(id);
   if(archive=="UNIT_SPC.E5")reacting.insert(id);
   auto key=std::to_string(id)+"-"+archive+"-"+pose.at("frame").dump()+"-"+pose.at("mirror").dump()+"-"+std::to_string(!pose.at("effects").empty());
   if(seen_frames.insert(key).second){auto file=key+".bmp";draw(out/file);frames.push_back({{"character",id},{"archive",archive},{"frame",pose.at("frame")},{"mirror",pose.at("mirror")},{"effects",pose.at("effects")},{"capture",file}});}
  }
  check(game->serialize()==before,"animation rendering changed game state");
 };
 auto initial=game->serialize();auto initial_hash=game->hash();check(game->scenario()==2&&game->state().at("turn")==1,"wrong seed");
 for(int y=0;y<20;++y)for(int x=0;x<20;++x){auto p=voxel::project(x+.5f,y+.5f);check(voxel::cell(p.x,p.y)==std::array{x,y},"projection roundtrip");}
 draw(out/"initial.bmp");check(game->serialize()==initial,"rendering mutated snapshot");
 for(const auto& u:game->state().at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0||b.lifecycle!=2)continue;auto p=voxel::project(b.x+.5f,b.y+.5f);check(unit_at_pointer(p.x,p.y)==b.id,"unit tile picking");}
 auto cao=game->state().at("units").at("0").at("battle");auto point=voxel::project(cao.at("x").get<float>()+.5f,cao.at("y").get<float>()+.5f);
 click(point.x,point.y,true);check(show_unit_details&&detail_unit==0,"right click inspector");draw(out/"information.bmp");show_unit_details=false;check(game->serialize()==initial,"inspection mutated save");
 int active_units=0;for(const auto& u:game->state().at("units"))if(u.at("battle").at("lifecycle")==2)++active_units;check(active_units==2,"reduced roster count");
 open_unit_details(0);for(int n=0;n<39;++n){cycle_detail_unit(1);check(special_unit_in_roster(detail_unit),"inspector includes excluded unit");}show_unit_details=false;
 for(const auto& row:voxel_manifest.at("roster").at("removed")){open_unit_details(row.at("id"));check(!show_unit_details,"excluded unit information opens");}
 send("cancel");click(point.x,point.y,false);check(game->state().at("selected")==0,"selection");
 check(action_menu_unit<0,"first selection opened menu before move/confirmation");draw(out/"movement-grid.bmp");
 auto selected_snapshot=game->serialize();click(point.x,point.y,false);
 check(action_menu_unit==0&&!action_enabled(0),"current tile menu / out-of-range attack guard");
 draw(out/"stationary-menu-out-of-range.bmp");check(game->serialize()==selected_snapshot,"opening stationary menu changed simulation");
 activate_action(0);check(action_menu_unit==0&&game->serialize()==selected_snapshot,"disabled attack spent action");
 activate_action(4);check(game->state().at("units").at("0").at("battle")==cao,"stationary cancel changed unit");
 click(point.x,point.y,false);auto range=game->movement();int dest=-1;
 for(int p=0;p<400;++p)if(range[p]>0&&range[p]!=255){bool occupied=false;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("hp").get<int>()>0&&b.at("lifecycle")==2&&b.at("x")==p%20&&b.at("y")==p/20)occupied=true;}if(!occupied){dest=p;break;}}
 check(dest>=0,"movement destination");check(send("tile",dest%20,dest/20)&&playback.active(),"movement starts");
 draw(out/"movement.bmp");while(playback.active()){present();++movements;advance(40);}open_action_menu();draw(out/"action-menu.bmp");check(action_menu_unit==0,"postmove menu");
 check(send("cancel"),"undo");check(game->state().at("units").at("0").at("battle")==cao,"undo position/statistics");
 // Dong Zhuo's wide chariot overlaps this empty destination. Ground targeting
 // must win while choosing a move, without breaking sprite inspection.
 click(point.x,point.y,false);auto adjacent=voxel::project(12.5f,10.5f);click(adjacent.x,adjacent.y,false);
 check(game->state().at("units").at("0").at("battle").at("x")==12,"chariot blocked empty movement tile");
 while(playback.active())advance(40);check(send("cancel"),"undo adjacent move");
 persist(false);check(notice=="已保存。","save");auto saved=game->serialize();restart_special();persist(true);check(game->serialize()==saved,"restore");
 restart_special();check(game->hash()==initial_hash,"restart seed");
 // Let Dong Zhuo approach first, then exercise ordinary player/AI actions.
 game->command("end_turn");
 for(int n=0;n<500&&!(captured&&attacking.size()==2&&walking.size()==2&&reacting.size()==2);++n){
  if(game->has_dialogue()){game->command("continue");continue;}
  if(game->state().at("phase")=="player"){
   int id=-1;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("side")==0&&b.at("hp").get<int>()>0&&b.at("lifecycle")==2&&!(b.at("flags").get<int>()&2)&&!(b.at("status").get<int>()&8)){id=b.at("id");break;}}
   if(id<0)game->command("end_turn");else{auto b=game->state().at("units").at(std::to_string(id)).at("battle");game->command("tile",b.at("x"),b.at("y"));auto plan=game->suggest(id);
    if(plan.at("x")!=b.at("x")||plan.at("y")!=b.at("y")){game->command("tile",plan.at("x"),plan.at("y"));collect_presentation();while(playback.active()){++movements;present();advance(40);}}
    int target=plan.at("target");if(target>=0){auto b=game->state().at("units").at(std::to_string(target)).at("battle");game->command("tile",b.at("x"),b.at("y"));}else game->command("wait");
   }
  }else if(game->state().at("phase")=="enemy"||game->state().at("phase")=="ally")game->enemy_step();else break;
  collect_presentation();
  while(playback.active()){
   if(playback.moving())++movements;else ++attacks;
   if(!captured&&!playback.moving()&&playback.packet().value("kind",std::string())=="physical"&&playback.before().at("phase")=="enemy"){draw(out/"enemy-attack.bmp");captured=true;}
   present();advance(40);
  }
 }
 check(movements>0&&attacks>0&&captured&&attacking.size()==2&&walking.size()==2&&reacting.size()==2,"both duelists movement/attack/reaction presentation");auto post=game->serialize();restore_snapshot(post);check(game->serialize()==post,"NPC result restore");
 // Adjacent-enemy fixture verifies a complete attack through the real input
 // menu without creating a movement undo record or playing a move animation.
 auto adjacent_fixture=Json::parse(initial);adjacent_fixture["payload"]["state"]["units"]["127"]["battle"]["x"]=11;
 adjacent_fixture["sha256"]=sha256(adjacent_fixture.at("payload").dump());restore_snapshot(adjacent_fixture.dump());
 click(point.x,point.y,false);check(action_menu_unit<0,"first stationary attack selection");
 auto stationary_selected=game->serialize();check(action_key(SDLK_SPACE,false)&&action_menu_unit==0,"space opens stationary menu");
 check(action_enabled(0)&&game->state().at("undo").is_null()&&game->serialize()==stationary_selected,"stationary attack menu eligibility");
 draw(out/"stationary-attack-menu.bmp");activate_action(0);check(attack_targeting,"stationary attack targeting");
 auto enemy_point=voxel::project(11.5f,10.5f);click(enemy_point.x,enemy_point.y,false);
 check(playback.active()&&!playback.moving()&&playback.packet().at("kind")=="physical","stationary attack did not start combat");
 draw(out/"stationary-attack.bmp");while(playback.active())advance(40);
 const auto& after_attack=game->state().at("units").at("0").at("battle");
 check(after_attack.at("x")==cao.at("x")&&after_attack.at("y")==cao.at("y")&&(after_attack.at("flags").get<int>()&2),"stationary attack moved actor or did not spend action");
 auto attacked=game->serialize();restore_snapshot(attacked);check(game->serialize()==attacked,"stationary attack snapshot restore");
 std::ofstream(out/"verification.json")<<Json{{"initial_hash",initial_hash},{"seed_units",active_units},{"inspector_excludes_removed",true},{"tile_roundtrips",400},{"current_tile_menu",true},{"stationary_attack",true},{"stationary_cancel",true},{"out_of_range_attack_disabled",true},{"authored_frames",frames},{"both_characters_walk_attack_react",true},{"movement_frames",movements},{"combat_frames",attacks},{"save_restore",true},{"restart_preserves_seed",true}}.dump(2);
 restart_special();std::ofstream(out/"seed-current-schema.json")<<game->serialize();
 // Exercise visible ground picking, route animation and resume inside the fort.
 click(point.x,point.y,false);auto courtyard=voxel::project(5.5f,10.5f);click(courtyard.x,courtyard.y,false);
 check(playback.active()&&playback.moving(),"gate movement did not start");
 bool crossed_gate=false;while(playback.active()){
  for(const auto& pose:playback.poses())if(pose.at("slot")==0){float x=(pose.at("x").get<float>()+24)/48-.5f;if(x>=5.75f&&x<=6.25f&&!crossed_gate){draw(out/"entering-gate.bmp");crossed_gate=true;}}
  advance(40);
 }
 check(crossed_gate&&game->state().at("units").at("0").at("battle").at("x")==5,"Cao Cao did not enter courtyard");
 close_action_menu();draw(out/"inside-courtyard.bmp");persist(false);auto courtyard_save=game->serialize();
 restart_special();persist(true);check(game->serialize()==courtyard_save,"courtyard load");open_action_menu();draw(out/"courtyard-restored.bmp");
 activate_action(4);check(game->state().at("units").at("0").at("battle")==cao,"gate move cancel after restore");
 std::ofstream(out/"gate-verification.json")<<Json{{"crossed_gate",crossed_gate},{"courtyard",{5,10}},{"save_restore_undo",true},{"schema",13}}.dump(2);
 restart_special();
 // Shared native/browser chooser: input is modal until an actual hero choice.
 request_special_selection();special_has_match=false;auto parked=game->serialize();
 check(!send("end_turn"),"chooser leaks gameplay command");
 click(1,1,false);check(special_choosing&&game->serialize()==parked,"outside click started match");
 draw(out/"hero-choice-zh.bmp");battle_language.english(true);draw(out/"hero-choice-en.bmp");battle_language.english(false);
 Json choices=Json::array();
 for(int index=0;index<4;++index){request_special_selection();auto card=special_hero_card(index);click(card.x+card.w/2,card.y+card.h/2,false);
  check(!special_choosing&&game->duel_hero()==special_heroes[index]&&Prototype::duel_enemy_valid(game->duel_enemy()),"chooser selected wrong match");
  auto saved=game->serialize();persist(false);request_special_selection();persist(true);check(!special_choosing&&game->serialize()==saved,"native load rerolls match or leaves chooser open");
  choices.push_back({{"hero",game->duel_hero()},{"enemy",game->duel_enemy()}});
 }
 Json matches=Json::array();
 for(int hero:special_heroes)for(int enemy:{119,127}){
  const auto pair=enemy==119?Prototype::duel_allies_for_draw(hero,0):std::array{-1,-1};const int ally=pair[0],ally2=pair[1];
  restart_special(hero,enemy,ally,ally2);auto prefix=std::to_string(hero)+"-vs-"+std::to_string(enemy);draw(out/(prefix+"-initial.bmp"));
  open_unit_details(hero);std::set<int> inspected;for(int n=0;n<(ally>=0?4:2);++n){inspected.insert(detail_unit);cycle_detail_unit(1);}check(inspected.contains(hero)&&inspected.contains(enemy)&&(ally<0||(inspected.contains(ally)&&inspected.contains(ally2)))&&detail_unit==hero,"inspector skips active fighter");show_unit_details=false;
  for(int extra_ally:{ally,ally2}){if(extra_ally<0)continue;const int ally=extra_ally;
   const auto extra=game->state().at("units").at(std::to_string(ally)).at("battle");auto p=voxel::project(extra.at("x").get<float>()+.5f,extra.at("y").get<float>()+.5f);
   click(p.x,p.y,true);check(show_unit_details&&detail_unit==ally,"extra ally cannot be inspected");draw(out/(prefix+"-ally-"+std::to_string(ally)+"-info.bmp"));show_unit_details=false;
   click(p.x,p.y,false);check(game->state().at("selected")==ally,"extra ally cannot be selected");auto path=game->movement();int dest=-1;
   for(int cell=0;cell<400;++cell)if(path[cell]>0&&path[cell]!=255){bool occupied=false;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==2&&b.at("x")==cell%20&&b.at("y")==cell/20)occupied=true;}if(!occupied){dest=cell;break;}}
   check(dest>=0&&send("tile",dest%20,dest/20)&&playback.moving(),"extra ally cannot move");draw(out/(prefix+"-ally-"+std::to_string(ally)+"-walk.bmp"));while(playback.active())advance(40);send("cancel");check(game->state().at("units").at(std::to_string(ally)).at("battle")==extra,"extra ally move cancellation");
   auto snapshot=game->serialize();restore_snapshot(snapshot);check(game->duel_includes(ally)&&game->duel_ally2()==ally2&&game->serialize()==snapshot,"extra ally save restore");
  }
  auto actor=game->state().at("units").at(std::to_string(hero)).at("battle");send("tile",actor.at("x"),actor.at("y"));
  auto range=game->movement();int destination=-1;for(int i=0;i<400;++i)if(range[i]!=255&&range[i]>0){destination=i;break;}
  check(destination>=0&&send("tile",destination%20,destination/20)&&playback.moving(),"selected hero has no walk animation");draw(out/(prefix+"-walk.bmp"));while(playback.active())advance(40);send("cancel");
  restart_special(hero,enemy,ally,ally2);actor=game->state().at("units").at(std::to_string(hero)).at("battle");send("tile",actor.at("x"),actor.at("y"));auto plan=game->suggest(hero);
  if(plan.at("x")!=actor.at("x")||plan.at("y")!=actor.at("y")){send("tile",plan.at("x"),plan.at("y"));while(playback.active())advance(40);}
  check(game->attackable(hero,enemy),"hero smoke attack outside range");auto foe=game->state().at("units").at(std::to_string(enemy)).at("battle");send("tile",foe.at("x"),foe.at("y"));
  bool hero_attack=false,enemy_attack=false;
  auto capture_match=[&](){for(const auto& pose:playback.poses())if(pose.at("archive")=="UNIT_ATK.E5"){
   if(pose.at("slot")==actor.at("slot")&&!hero_attack){draw(out/(prefix+"-hero-attack.bmp"));hero_attack=true;}
   if(pose.at("slot")==foe.at("slot")&&!enemy_attack){draw(out/(prefix+"-enemy-attack.bmp"));enemy_attack=true;}
  }draw();};
  while(playback.active()){capture_match();advance(40);}
  if(game->state().at("phase")=="player")send("end_turn");
  for(int n=0;n<10&&!enemy_attack&&game->state().at("phase")!="defeat";++n){game->enemy_step();collect_presentation();while(playback.active()){capture_match();advance(40);}}
  check(hero_attack&&enemy_attack,"selected matchup attack frames not rendered for both sides");
  matches.push_back({{"hero",hero},{"enemy",enemy},{"walk",true},{"hero_attack",hero_attack},{"enemy_attack",enemy_attack},{"ally",ally},{"ally2",ally2},{"inspector_roster_only",true}});
 }
 std::ofstream(out/"hero-verification.json")<<Json{{"chooser_clicks",choices},{"native_save_restore_match",true},{"chooser_blocks_commands",true},{"matchups",matches}}.dump(2);
 // A fallen chosen hero must not block the surviving ally's actual UI controls.
 restart_special(0,119,11);auto solo=Json::parse(game->serialize());
 solo["payload"]["state"]["units"]["0"]["battle"]["hp"]=0;solo["payload"]["state"]["units"]["0"]["battle"]["lifecycle"]=3;
 solo["payload"]["state"]["message"]="";solo["sha256"]=sha256(solo.at("payload").dump());restore_snapshot(solo.dump());
 std::ofstream(out/"surviving-ally.json")<<game->serialize();
 auto ally_point=voxel::project(10.5f,12.5f);click(ally_point.x,ally_point.y,false);
 check(game->state().at("phase")=="player"&&game->state().at("selected")==11,"fallen chosen hero blocks ally selection");
 auto ally_move=voxel::project(12.5f,10.5f);click(ally_move.x,ally_move.y,false);while(playback.active())advance(40);open_action_menu();
 check(action_menu_unit==11&&action_enabled(0),"surviving ally cannot move/attack");
 auto solo_action=game->serialize();persist(false);persist(true);check(game->serialize()==solo_action,"survivor pending action not restored");
 draw(out/"surviving-ally-action.bmp");activate_action(0);auto lu_point=voxel::project(13.5f,10.5f);click(lu_point.x,lu_point.y,false);
 check(playback.active()&&!playback.moving(),"surviving ally attack not animated");while(playback.active())advance(40);
 restore_snapshot(solo.dump());show_info=true;draw(out/"team-objective-zh.bmp");battle_language.english(true);draw(out/"team-objective-en.bmp");battle_language.english(false);show_info=false;
 auto both=solo;both["payload"]["state"]["units"]["11"]["battle"]["hp"]=0;both["payload"]["state"]["units"]["11"]["battle"]["lifecycle"]=3;
 both["payload"]["state"]["phase"]="defeat";both["sha256"]=sha256(both.at("payload").dump());restore_snapshot(both.dump());draw(out/"team-defeat.bmp");
 std::ofstream(out/"team-verification.json")<<Json{{"fallen_primary_ally_select_move_attack",true},{"survivor_pending_save_restored",true},{"all_fallen_defeat",true},{"objectives_zh_en",true}}.dump(2);
 // The newly added third friendly stays actionable after both teammates fall.
 restart_special(0,119,1,11);draw(out/"three-friendlies-initial.bmp");auto third=Json::parse(game->serialize());
 for(int id:{0,1}){auto& unit=third["payload"]["state"]["units"][std::to_string(id)]["battle"];unit["hp"]=0;unit["lifecycle"]=3;}
 third["payload"]["state"]["message"]="";third["sha256"]=sha256(third.at("payload").dump());restore_snapshot(third.dump());
 auto third_point=voxel::project(10.5f,8.5f);click(third_point.x,third_point.y,true);check(show_unit_details&&detail_unit==11,"third survivor inspection blocked");show_unit_details=false;
 click(third_point.x,third_point.y,false);check(game->state().at("selected")==11,"third survivor selection blocked");
 auto target_point=voxel::project(12.5f,10.5f);click(target_point.x,target_point.y,false);while(playback.active())advance(40);open_action_menu();
 check(action_menu_unit==11&&action_enabled(0),"third survivor move/attack blocked");auto trio_pending=game->serialize();persist(false);persist(true);check(game->serialize()==trio_pending,"third survivor pending save changed");
 draw(out/"third-survivor-action.bmp");activate_action(0);click(lu_point.x,lu_point.y,false);check(playback.active(),"third survivor attack animation missing");while(playback.active())advance(40);
 std::ofstream(out/"trio-verification.json")<<Json{{"three_friendlies_drawn",true},{"four_unit_inspection",true},{"both_random_allies_move",true},{"third_survivor_select_move_attack",true},{"third_survivor_save_restore",true}}.dump(2);
 restart_special();request_special_selection();
 std::cout<<"special Hulao native smoke passed\n";
}
void special_supplies_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save directory required");
 std::filesystem::create_directories(out);auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};Json rows=Json::array();
 for(int enemy:{119,127}){
  restart_special(0,enemy,enemy==119?1:-1,enemy==119?11:-1);check(game->inventory().at("87")==3,"new match inventory missing beans");
  auto start=Json::parse(game->serialize());start["payload"]["state"]["units"]["0"]["battle"]["hp"]=1;start["sha256"]=sha256(start.at("payload").dump());restore_snapshot(start.dump());
  auto actor=voxel::project(10.5f,10.5f);click(actor.x,actor.y,false);show_inventory=true;treasure_view=false;draw(out/(std::to_string(enemy)+"-three-beans.bmp"));
  auto button=item_use_rect(0);click(button.x+button.w/2,button.y+button.h/2,false);check(target_item==87,"bean use button disabled");click(actor.x,actor.y,false);
  check(game->inventory().at("87")==2&&playback.active(),"bean not spent/animated");while(playback.active()){playback.advance(40);draw();}
  auto saved=game->serialize();persist(false);persist(true);check(game->serialize()==saved&&game->inventory().at("87")==2,"bean save refilled inventory");show_inventory=true;draw(out/(std::to_string(enemy)+"-two-beans.bmp"));show_inventory=false;
  rows.push_back({{"enemy",enemy},{"starting_beans",3},{"after_use",2},{"hp_after",game->state().at("units").at("0").at("battle").at("hp")},{"save_resume_no_refill",true}});
 }
 std::ofstream(out/"supplies-verification.json")<<rows.dump(2);std::cout<<"special starter supplies smoke passed\n";
}
void special_camps_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save directory required");
 std::filesystem::create_directories(out);auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};Json rows=Json::array();
 auto complete=[](){while(playback.active()){playback.advance(40);draw();}};
 restart_special(0,127);draw(out/"aligned-camps-initial.bmp");
 // Every visible opaque piece of each empty tent maps to that camp, including
 // the roof extending above its ground square. Units retain input priority.
 for(int x:{8,19}){
  const auto& t=voxel_camps.at("tent");const int w=t.at("w"),h=t.at("h");const auto& raw=voxel_pixels.at("camps-v1/tent.rgba");auto r=voxel_camp_rect(x,9);int tested=0;
  for(int py=4;py<h;py+=12)for(int px=4;px<w;px+=12)if(raw[(py*w+px)*4+3]>128){
   auto cell=map_cell(r.x+(px+.5f)*r.w/w,r.y+(py+.5f)*r.h/h);check(cell==std::array{x,9},"visible tent targets wrong tile");++tested;
  }
  check(tested>100,"camp alpha hit test sampled too few points");
 }
 for(int x:{8,19}){
  restart_special(0,127);terrain_fixture_place(*game,0,x-1,9,30);restore_snapshot(game->serialize());
  auto from=voxel::project(x-.5f,9.5f),camp=voxel::project(x+.5f,9.5f);auto tent=voxel_camp_rect(x,9);
  auto roof=voxel::Point{tent.x+tent.w*.55f,tent.y+tent.h*.3f};check(voxel::cell(roof.x,roof.y)!=std::array{x,9}&&map_cell(roof.x,roof.y)==std::array{x,9},"roof hit test did not resolve to camp");
  click(from.x,from.y,false);click(roof.x,roof.y,false);complete();
  check(game->state().at("units").at("0").at("battle").at("x")==x&&game->state().at("units").at("0").at("battle").at("y")==9&&game->state().at("units").at("0").at("battle").at("hp")==30,"tent click heals immediately or targets wrong tile");
  check(send("cancel"),"camp movement cancel failed");check(game->state().at("units").at("0").at("battle").at("x")==x-1,"camp cancel did not restore position");
  click(from.x,from.y,false);click(roof.x,roof.y,false);complete();check(send("wait"),"camp wait failed");complete();
  mouse_x=camp.x;mouse_y=camp.y;draw(out/(std::to_string(x)+"-camp-before.bmp"));
  check(send("end_turn"),"camp end turn failed");complete();game->enemy_step();collect_presentation();complete();
  check(game->state().at("phase")=="enemy"&&game->state().at("units").at("0").at("battle").at("hp")==30,"camp healed before own turn");
  // Isolate the phase boundary from enemy combat, preserving the actual heal path.
  auto e=Json::parse(game->serialize());auto& p=e["payload"];p["state"]["enemy_cursor"]=p.at("npc_order").size();
  p["state"]["units"]["127"]["battle"]["flags"]=2;e["sha256"]=sha256(p.dump());restore_snapshot(e.dump());
  std::ofstream(out/(std::to_string(x)+"-before-recovery.snapshot.json"))<<game->serialize();
  game->enemy_step();collect_presentation();check(playback.active()&&playback.number().value("healing",false)&&playback.number().at("value")==22,"camp healing feedback missing");
  check(game->state().at("units").at("0").at("battle").at("hp")==52,"camp HP did not increase by 20 percent");
  draw(out/(std::to_string(x)+"-camp-recovery-zh.bmp"));battle_language.english(true);draw(out/(std::to_string(x)+"-camp-recovery-en.bmp"));battle_language.english(false);
  playback.advance(800);check(playback.active()&&!playback.number().is_null(),"camp feedback disappears too quickly");complete();
  auto saved=game->serialize();persist(false);persist(true);check(game->serialize()==saved,"load repeats camp healing");draw(out/(std::to_string(x)+"-camp-after.bmp"));
  rows.push_back({{"camp",{x,9}},{"before",30},{"after",52},{"feedback_at_800ms",true},{"move_cancel_wait_tested",true},{"reload_exact",true},{"visible_tent_pixels_target_camp",true},{"roof_click_moves_to_camp",true}});
 }
 std::ofstream(out/"camp-verification.json")<<rows.dump(2);std::cout<<"special camp recovery UI smoke passed\n";
}
void special_squads_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save directory required");
 std::filesystem::create_directories(out);auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
 auto advance=[&](){while(playback.active()){auto previous=playback.generation();playback.advance(40);if(playback.active()&&previous!=playback.generation())begin_presentation();}};
 request_special_selection();draw(out/"choose-zh.bmp");battle_language.english(true);draw(out/"choose-en.bmp");battle_language.english(false);
 Json choices=Json::array(),matches=Json::array();
 for(int index=0;index<4;++index){request_special_selection();const auto card=special_hero_card(index);click(card.x+card.w/2,card.y+card.h/2,false);
  auto p=Json::parse(game->serialize()).at("payload");check(!special_choosing&&p.at("schema")== (game->duel_enemy()==127?19:18)&&game->duel_hero()==special_heroes[index],"chooser did not start current squad rules");
  int friends=0;for(const auto& u:game->state().at("units"))if(u.at("battle").at("side")==0&&u.at("battle").at("lifecycle")==2)++friends;
  check(friends==(game->duel_enemy()==119?4:2),"chooser wrong team size");choices.push_back({{"hero",game->duel_hero()},{"enemy",game->duel_enemy()},{"friendlies",friends}});
 }
 const auto seed=read_json(assets/"special-hulao/seed.json").dump();
 for(int enemy:{127,119}){
  game->start_hulao_squad(seed,0,enemy,0);restore_snapshot(game->serialize());auto prefix=std::to_string(enemy);draw(out/(prefix+"-initial.bmp"));
  check(game->inventory().at("87")== (enemy==127?0:3),"squad opponent supply policy");show_inventory=true;treasure_view=false;draw(out/(prefix+"-inventory.bmp"));show_inventory=false;
  std::vector<int> friends{game->duel_hero(),game->duel_ally()};if(enemy==119){friends.push_back(game->duel_ally2());friends.push_back(game->duel_ally3());}
  open_unit_details(0);std::set<int> inspected;for(int n=0;n<int(friends.size()+1);++n){inspected.insert(detail_unit);cycle_detail_unit(1);}check(detail_unit==0&&inspected.size()==friends.size()+1,"squad inspector cycle");show_unit_details=false;
  const auto initial=game->serialize();int acted=0;
  for(int id:friends){auto pos=game->state().at("units").at(std::to_string(id)).at("battle");const auto point=voxel::project(pos.at("x").get<float>()+.5f,pos.at("y").get<float>()+.5f);click(point.x,point.y,false);check(game->state().at("selected")==id,"squad sprite selection");
   auto range=game->movement();int dest=-1;for(int cell=0;cell<400;++cell)if(range[cell]>0&&range[cell]!=255){bool occupied=false;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==2&&b.at("x")==cell%20&&b.at("y")==cell/20)occupied=true;}if(!occupied){dest=cell;break;}}
   check(dest>=0&&send("tile",dest%20,dest/20)&&playback.moving(),"squad movement animation");draw(out/(prefix+"-move-"+std::to_string(id)+".bmp"));advance();send("cancel");
   send("tile",pos.at("x"),pos.at("y"));send("wait");advance();++acted;check(game->player_turn_complete()==(acted==int(friends.size())),"squad auto-end skipped teammate");
  }
  restore_snapshot(initial);persist(false);request_special_selection();persist(true);check(game->serialize()==initial&&!special_choosing,"squad native slot restoration");
  if(enemy==119){auto solo=Json::parse(initial);for(int id:{0,6,1}){auto& b=solo["payload"]["state"]["units"][std::to_string(id)]["battle"];b["hp"]=0;b["lifecycle"]=3;}solo["sha256"]=sha256(solo.at("payload").dump());restore_snapshot(solo.dump());
   const auto point=voxel::project(8.5f,10.5f);click(point.x,point.y,false);check(game->state().at("selected")==11,"fourth survivor selection");auto plan=game->suggest(11);send("tile",plan.at("x"),plan.at("y"));advance();check(game->attackable(11,119),"fourth survivor cannot reach enemy");open_action_menu();draw(out/"fourth-survivor-action.bmp");auto foe=game->state().at("units").at("119").at("battle");send("tile",foe.at("x"),foe.at("y"));check(playback.active(),"fourth survivor attack animation");advance();
  }
  matches.push_back({{"enemy",enemy},{"friendlies",friends},{"each_moves_and_acts",true},{"save_restore",true}});
 }
 std::ofstream(out/"verification.json")<<Json{{"chooser",choices},{"matches",matches},{"fourth_survivor_attack",true}}.dump(2);
 std::cout<<"two/four friendly native smoke passed\n";
}
