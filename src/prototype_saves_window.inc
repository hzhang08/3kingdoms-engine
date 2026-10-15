// Ten native/browser slots. Historical per-battle saves remain readable.
SDL_FRect save_row(int n){return {204.f+(n%2)*282,340.f+(n/2)*56,270,48};}
SDL_FRect slots_button{468,1080,162,32};
void open_slots(){if(playback.active()||show_unit_details)return;
#ifdef __EMSCRIPTEN__
 if(web_hosted){EM_ASM({window.prototypeSlots?.();});return;}
#endif
 close_action_menu();show_slots=true;show_info=show_inventory=show_battles=false;}
#ifndef __EMSCRIPTEN__
std::mutex backup_mutex;
std::string backup_path,backup_snapshot,backup_error;
bool backup_ready=false,backup_import=false;
void SDLCALL backup_callback(void*,const char* const* files,int){std::lock_guard lock(backup_mutex);backup_path=files&&files[0]?files[0]:"";backup_error=files?"":"文件选择器不可用。";backup_ready=true;}
void backup_finish(){
 std::string path,error;{std::lock_guard lock(backup_mutex);if(!backup_ready)return;path=backup_path;error=backup_error;backup_ready=false;}backup_busy=false;
 try{
  if(!error.empty())throw std::runtime_error(error);if(path.empty()){log_presentation({{"event","backup_cancelled"},{"operation",backup_import?"import":"export"}});return;}
  if(backup_import){if(std::filesystem::file_size(path)>200000)throw std::runtime_error("存档过大。");std::ifstream f(path,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};restore_snapshot(raw);notice="已导入备份；请保存到选择的存档位。";}
  else{auto pending=std::filesystem::path(path);pending+=".pending";{std::ofstream f(pending,std::ios::binary);f<<backup_snapshot;f.flush();if(!f)throw std::runtime_error("导出失败。");}std::filesystem::rename(pending,path);notice="已导出备份。";}
  diagnostic_dirty=true;log_presentation({{"event","backup_success"},{"operation",backup_import?"import":"export"},{"state_hash",game->hash()}});
 }catch(const std::exception& e){notice=e.what();log_presentation({{"event","backup_error"},{"operation",backup_import?"import":"export"},{"message",e.what()}});}
}
#endif
void backup_dialog(bool load){
 if(playback.active()||backup_busy)return;
#ifdef __EMSCRIPTEN__
 EM_ASM({if($0)window.prototypeImport();else window.prototypeExport();},load);
#else
 if(jev&&jev->busy()){notice="JEV: please wait before saving.";return;}
 log_presentation({{"event","backup_begin"},{"operation",load?"import":"export"}});
 backup_import=load;backup_snapshot=load?"":game->serialize();backup_busy=true;
 static const SDL_DialogFileFilter filter[]{ {"Cao Cao snapshot","json"} };
 if(load)SDL_ShowOpenFileDialog(backup_callback,nullptr,window,filter,1,nullptr,false);
 else SDL_ShowSaveFileDialog(backup_callback,nullptr,window,filter,1,"caocao-backup.json");
#endif
}
void slots_click(float x,float y){
 for(int n=0;n<10;++n)if(inside(x,y,save_row(n))){save_slot=n+1;return;}
 if(inside(x,y,{204,640,552,40})){save_slot=0;persist(true);show_slots=false;return;}
 if(inside(x,y,{204,698,132,42})){if(save_slot==0)save_slot=1;persist(false);return;}
 if(inside(x,y,{344,698,132,42})){persist(true);return;}
 if(inside(x,y,{484,698,132,42})){backup_dialog(false);return;}
 if(inside(x,y,{624,698,132,42})){backup_dialog(true);return;}
 if(inside(x,y,{204,766,552,40}))show_slots=false;
}
void draw_slots(){
 fill(0,map_top,width,map_bottom-map_top,{0,0,0,145});bevel({184,282,592,574});ribbon(194,292,572,"本地存档与备份");
 for(int n=0;n<10;++n){auto row=save_row(n);bevel(row,save_slot==n+1);text("存档 "+std::to_string(n+1)+(save_slot==n+1?"  已选择":""),row.x+20,row.y+12,row.x+row.w-8);}
 bevel({204,640,552,40});text("读取当前战役的旧版存档",270,650,744);
 const char* buttons[]{"保存 F5","读取 F9","导出备份","导入备份"};for(int n=0;n<4;++n){bevel({204.f+n*140,698,132,42});text(buttons[n],218.f+n*140,710,330.f+n*140);}
 bevel({204,766,552,40});text("返回游戏 Esc",414,776,744);
 text("导入不会覆盖存档；选定位置保存后才会写入。",204,824,752);
}
