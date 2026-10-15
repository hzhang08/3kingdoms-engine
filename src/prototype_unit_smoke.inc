void unit_details_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;
 auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 auto key=[&](SDL_Keycode key){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;SDL_PushEvent(&e);tick();};
 auto tap=[&](SDL_FRect r,bool right=false,bool control=false){float x,y;SDL_RenderCoordinatesToWindow(renderer,r.x+r.w/2,r.y+r.h/2,&x,&y);SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=right?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_SetModState(control?SDL_KMOD_CTRL:SDL_KMOD_NONE);SDL_PushEvent(&e);tick();SDL_SetModState(SDL_KMOD_NONE);};
 int count=0;Json summaries=Json::array();
 for(int scenario=0;scenario<4;++scenario){choose_battle(scenario);while(game->has_dialogue())game->command("continue");auto before=game->serialize();
  for(const auto& u:game->state().at("units")){int id=u.at("battle").at("id");open_unit_details(id);check(show_unit_details&&detail_unit==id,"unit inspector did not open");
   for(int tab=0;tab<5;++tab){detail_tab=tab;detail_page=0;draw();if(id==0||((id==132||id==119)&&scenario==2)||((id==210||id==146)&&scenario==0))draw(out/("battle-"+std::to_string(scenario)+"-unit-"+std::to_string(id)+"-tab-"+std::to_string(tab)+".bmp"));}
   summaries.push_back(current_details());show_unit_details=false;++count;
  }
  check(game->serialize()==before,"rendering details changed game");
 }
 choose_battle(0);while(game->has_dialogue())game->command("continue");auto cao=game->state().at("units").at("0").at("battle");
 for(auto size:{std::array{720,840},std::array{1200,960}}){SDL_SetWindowSize(window,size[0],size[1]);SDL_SyncWindow(window);auto before=game->serialize();tap(tile_rect(cao.at("x"),cao.at("y")),false,true);check(show_unit_details&&detail_unit==0,"Control-click/scaled inspector input failed");
  tap(unit_tab_rect(1));check(detail_tab==1,"equipment tab input failed");key(SDLK_E);key(SDLK_R);key(SDLK_P);key(SDLK_B);key(SDLK_J);key(SDLK_F5);key(SDLK_F9);tap(tool_rect(2));check(before==game->serialize(),"modal leaked command or save action");
  tap(unit_tab_rect(2));key(SDLK_PAGEDOWN);key(SDLK_PAGEUP);key(SDLK_TAB);check(detail_tab==3,"tab key did not cycle");tap(unit_next);check(detail_unit!=0,"next officer failed");tap(unit_previous);check(detail_unit==0,"previous officer failed");draw(out/("scaled-"+std::to_string(size[0])+".bmp"));key(SDLK_ESCAPE);check(!show_unit_details&&before==game->serialize(),"close changed game");
 }
 tap(tile_rect(cao.at("x"),cao.at("y")),true);check(show_unit_details,"right-click idle unit did not inspect");key(SDLK_ESCAPE);
 tap(tile_rect(cao.at("x"),cao.at("y")));check(game->state().at("selected")==0,"normal select broken");auto before=game->serialize();tap(unit_button);check(show_unit_details&&detail_unit==0,"visible inspector button failed");key(SDLK_ESCAPE);check(before==game->serialize(),"selected inspector changed selection");
 auto costs=game->movement();bool moved=false;
 for(int p=0;p<game->cell_count()&&!moved;++p)if(costs[p]>0&&costs[p]!=255){bool occupied=false;for(const auto& u:game->state().at("units"))if(u.at("battle").at("hp").get<int>()>0&&u.at("battle").at("x")==p%game->map_width()&&u.at("battle").at("y")==p/game->map_width())occupied=true;if(!occupied)moved=send("tile",p%game->map_width(),p/game->map_width());}
 check(moved&&playback.active(),"movement setup failed");key(SDLK_V);check(!show_unit_details,"inspection opened on animation");
 for(int n=0;playback.active()&&n<2000;++n){animation_clock=SDL_GetTicks()-40;tick();}tick();check(action_menu_unit==0,"move action menu missing");
 auto moved_save=game->serialize();key(SDLK_V);check(show_unit_details,"inspect tentative move failed");key(SDLK_2);draw(out/"during-pending-move.bmp");key(SDLK_ESCAPE);check(action_menu_unit==0&&moved_save==game->serialize(),"inspector discarded tentative move/action menu");
 // Original right-cancel remains first priority for an active move.
 auto now=game->state().at("units").at("0").at("battle");tap(tile_rect(now.at("x"),now.at("y")),true);check(!show_unit_details&&game->state().at("undo").is_null(),"right click no longer cancels a move");
 send("end_turn");check(!playback.active(),"unexpected player-end presentation");open_unit_details(0);check(show_unit_details,"enemy-phase inspection failed");auto enemy=game->serialize();next_enemy=0;key(SDLK_E);tick();check(enemy==game->serialize()&&!playback.active(),"enemy advanced under modal");next_enemy=SDL_GetTicks()+100000;key(SDLK_ESCAPE);
 restore_snapshot(before);persist(false);auto saved=game->serialize();open_unit_details(0);key(SDLK_ESCAPE);persist(true);check(game->serialize()==saved,"save roundtrip changed inspected game");
 std::ofstream(out/"verification.json")<<Json{{"units",count},{"rendered_tabs",count*5},{"details_sha256",sha256(summaries.dump())},{"renderer",SDL_GetRendererName(renderer)},{"scaled_pointer_and_control_click",true},{"nonmutating_modal",true},{"pending_move_preserved",true},{"enemy_paused",true},{"save_roundtrip",true}}.dump(2)<<'\n';
 std::cout<<"Unit details UI passed: "<<count<<" units / "<<count*5<<" tabs; input, modal, pending move, save and enemy pause.\n";
}
