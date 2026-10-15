#include "caocao/prototype.hpp"
#include "caocao/prototype_playback.hpp"
#include "caocao/combat_rules.hpp"
#include "caocao/panel.hpp"
#include "caocao/recovery.hpp"
#include "caocao/unit_details.hpp"
#include "caocao/battle_language.hpp"
#include "caocao/voxel_board.hpp"
#include "caocao/special_hulao.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <set>
#include <chrono>
#include <cmath>
#include <mutex>
#include <random>
#ifndef __EMSCRIPTEN__
#include "caocao/diagnostics.hpp"
#include "caocao/jev_enemy.hpp"
#include "caocao/movement.hpp"
#include "../tests/prototype_tactics_fixture.hpp"
#include "../tests/prototype_terrain_fixture.hpp"
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif
using namespace caocao;
namespace {
// Use the same proportional map transform for terrain, poses and input.
// Opening stays at original size; wider battles fit the existing viewport.
float width=960, height=1120, map_bottom=1048;
constexpr int map_top=88,tile=48;
bool special_hulao=false,special_choosing=false,special_has_match=false;
Json voxel_manifest;
Json voxel_camps;
bool special_unit_in_roster(int id);
std::map<std::string,std::vector<unsigned char>> voxel_pixels;
int voxel_unit_at(float x,float y);
std::array<int,2> voxel_camp_at(float x,float y);
void restart_special(int hero=0,int enemy=127,int ally=-1,int ally2=-1);
void request_special_selection();
void select_special_hero(int index);
SDL_FRect special_hero_card(int index);
constexpr SDL_Color ink{25,25,32,255}, paper{212,212,212,255}, white{255,255,255,255};
std::unique_ptr<Prototype> game;
bool special_unit_in_roster(int id){return !special_hulao||game->duel_includes(id);}
PrototypePlayback playback;
int attack_actor=-1,attack_target=-1;
uint64_t attack_serial=0;
#ifndef __EMSCRIPTEN__
Diagnostics diagnostics;
#endif
const char* diagnostic_stage="startup";
bool diagnostic_dirty=true;
uint64_t diagnostic_heartbeat=0;
std::set<std::string> presented_poses;
bool presented_flash=false;
void log_presentation(Json event) noexcept{
#ifndef __EMSCRIPTEN__
 try{
  if(!diagnostics.active())return;
  event["ms"]=SDL_GetTicks();event["attack"]=attack_serial;event["stage"]=diagnostic_stage;
  if(game){event["scenario"]=game->scenario();if(!event.contains("turn"))event["turn"]=game->state().at("turn");if(!event.contains("phase"))event["phase"]=game->state().at("phase");}
  diagnostics.write(std::move(event));
 }catch(...){}
#else
 (void)event;
#endif
}

uint64_t animation_clock=0;
bool audio_ready=false;
std::vector<SDL_AudioStream*> attack_sounds;
const Json& visible_state(){return playback.active()?playback.before():game->state();}
void stop_attack_sounds(){for(auto* stream:attack_sounds)SDL_DestroyAudioStream(stream);attack_sounds.clear();}
SDL_Window* window=nullptr;
SDL_Renderer* renderer=nullptr;
std::filesystem::path assets;
Json font,ui,detail_tactics;
BattleLanguage battle_language;
std::string tr(const std::string& s){return battle_language.translate(s);}
bool treasure_view=false;
bool show_unit_details=false;
int detail_unit=0,detail_tab=0,detail_page=0;
std::map<std::string,SDL_Texture*> textures;
bool running=true,show_info=false,show_health=false,show_battles=false,show_inventory=false;
float map_scale(){return std::min({1.f,20.f/game->map_width(),20.f/game->map_height()});}
float map_tile(){return tile*map_scale();}
SDL_FRect inventory_button{806,1080,146,32};
constexpr SDL_FRect inventory_close{622,350,104,32};
constexpr SDL_FRect battle_button{796,54,156,28},next_button{440,510,260,40},camp_button{252,510,172,40};
SDL_FRect battle_choice(int id){return {228.f,350.f+id*68,504.f,54.f};}

bool showing_dialogue(){return !playback.active()&&game->has_dialogue();}
uint64_t next_enemy=0,frames=0;
float mouse_x=-1,mouse_y=-1;
int hover_panel_target=-1,visible_hover_panel=-1;
// Touch leaves no hovering pointer: keep its dismissible popup separate from
// unit selection, tentative movement and the full information window.
bool touch_hover_input=false;
int touch_hover_unit=-1;
void clear_touch_hover(){touch_hover_unit=-1;hover_panel_target=-1;visible_hover_panel=-1;}
uint64_t hover_panel_since=0;
constexpr uint64_t hover_panel_delay=220;
int action_menu_unit=-1,action_menu_focus=3,menu_after_move_unit=-1;
bool action_menu_keyboard=false,attack_targeting=false;
bool show_spells=false;
int save_slot=1;bool manual_slots=false,show_slots=false,backup_busy=false;
int target_tactic=-1,target_item=-1;
constexpr int item_ids[]{87,88,92};
SDL_FRect item_use_rect(int row){return {626,410.f+row*86,92,38};}
void choose_item(int row);
void close_action_menu(){action_menu_unit=-1;attack_targeting=false;menu_after_move_unit=-1;action_menu_keyboard=false;show_spells=false;target_tactic=-1;target_item=-1;}
void open_action_menu();
std::string notice,snapshot_buffer;
// Hosted browser mode reuses the native simulation and presentation. The host
// owns login/cloud storage; no credentials enter this translation unit.
bool web_hosted=false,web_paused=false,web_ai=true,web_ai_pending=false,web_mobile_text=false;
float font_raster_scale=1;
std::string web_ai_key;
#ifndef __EMSCRIPTEN__
std::unique_ptr<JevEnemy> jev;
bool jev_enabled=false;
std::string jev_status="Local AI (J: JEV)";
Json last_jev=nullptr;
void cancel_jev(){if(jev)jev->cancel();}
#else
void cancel_jev(){web_ai_pending=false;web_ai_key.clear();}
#endif
void diagnostic_checkpoint(const std::string& reason){
#ifndef __EMSCRIPTEN__
 if(!diagnostic_dirty||!diagnostics.active()||!game||playback.active()||backup_busy||(jev&&jev->busy()))return;
 // Retry a failed checkpoint on the next action, not on every rendered frame.
 diagnostic_dirty=false;try{diagnostics.checkpoint(game->serialize(),reason);}catch(...){}
#else
 (void)reason;
#endif
}
void diagnostic_action(const std::string& action,Json fields=Json::object()) noexcept{try{
 diagnostic_checkpoint("before_"+action);diagnostic_dirty=true;
 fields["event"]="action_begin";fields["action"]=action;
 if(game){fields["state_hash"]=game->hash();fields["selected"]=game->state().at("selected");fields["rng"]=game->state().at("rng");}
 log_presentation(std::move(fields));
}catch(...){} }

std::vector<uint32_t> codepoints(const std::string& s){
 std::vector<uint32_t> out;
 for(size_t n=0;n<s.size();){uint8_t c=s[n++];uint32_t v=c;int more=0;
  if((c&0xe0)==0xc0){v=c&31;more=1;}else if((c&0xf0)==0xe0){v=c&15;more=2;}else if((c&0xf8)==0xf0){v=c&7;more=3;}
  while(more--&&n<s.size())v=(v<<6)|(static_cast<uint8_t>(s[n++])&63);out.push_back(v);
 }
 return out;
}
std::string number3(int n){std::ostringstream s;s<<std::setw(3)<<std::setfill('0')<<n;return s.str();}
SDL_Texture* texture(const std::string& path){
 if(textures.contains(path))return textures.at(path);
 const bool sans=path=="images/font.bmp"&&font_raster_scale>1;
 auto actual=path; if(sans)actual="presentation-font/font-sans.bmp";else if(path=="images/font.bmp"){if(std::filesystem::exists(assets/"unit-details/font.bmp"))actual="unit-details/font.bmp";else if(std::filesystem::exists(assets/"items/font.bmp"))actual="items/font.bmp";else if(std::filesystem::exists(assets/"terrain-benefits/font.bmp"))actual="terrain-benefits/font.bmp";else if(std::filesystem::exists(assets/"expansion/font.bmp"))actual="expansion/font.bmp";}
 if(path.ends_with(".rgba")){
  std::ifstream f(assets/path,std::ios::binary);uint32_t w=0,h=0;f.read(reinterpret_cast<char*>(&w),4);f.read(reinterpret_cast<char*>(&h),4);
  if(!f||w<1||h<1||w>1024||h>1024)throw std::runtime_error("Invalid voxel sprite: "+path);
  auto& raw=voxel_pixels[path.substr(14)];raw.resize(size_t(w)*h*4);f.read(reinterpret_cast<char*>(raw.data()),raw.size());if(!f)throw std::runtime_error("Truncated voxel sprite");
  auto* surface=SDL_CreateSurfaceFrom(w,h,SDL_PIXELFORMAT_RGBA32,raw.data(),w*4);if(!surface)throw std::runtime_error(SDL_GetError());
  auto* t=SDL_CreateTextureFromSurface(renderer,surface);SDL_DestroySurface(surface);if(!t)throw std::runtime_error(SDL_GetError());
  SDL_SetTextureScaleMode(t,SDL_SCALEMODE_LINEAR);SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);textures[path]=t;return t;
 }
 auto* s=SDL_LoadBMP((assets/actual).string().c_str());if(!s)throw std::runtime_error(path+": "+SDL_GetError());
 if(sans){
  // The atlas stores coverage, not colored pixels. Convert it to straight alpha
  // so colored text has smooth edges without a magenta or black halo.
  auto* rgba=SDL_ConvertSurface(s,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(s);s=rgba;
  if(!s)throw std::runtime_error(SDL_GetError());
  for(int y=0;y<s->h;++y){auto* row=static_cast<Uint8*>(s->pixels)+y*s->pitch;for(int x=0;x<s->w;++x){auto* p=row+x*4;p[3]=p[0];p[0]=p[1]=p[2]=255;}}
 }else SDL_SetSurfaceColorKey(s,true,SDL_MapSurfaceRGB(s,247,0,255));
 auto* t=SDL_CreateTextureFromSurface(renderer,s);SDL_DestroySurface(s);if(!t)throw std::runtime_error(SDL_GetError());
 SDL_SetTextureScaleMode(t,(sans||path.starts_with("special-hulao/"))?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);if(sans)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);textures[path]=t;return t;
}
void attack_sound(int id){
 if(!audio_ready)return;
 SDL_AudioSpec spec{};Uint8* pcm=nullptr;Uint32 bytes=0;
 auto path=assets/(id==41?"items/audio/Se41.wav":id==14?"terrain-benefits/audio/Se14.wav":(id>=100||id==8||id==39)?"tactics/audio/"+std::string(id>=100?"Se_m_":"Se")+(id%100<10?"0":"")+std::to_string(id%100)+".wav":battle_sound_file(id));
 if(!std::filesystem::exists(path)&&id>=100)path=assets/("tactics-extended/audio/Se_m_"+std::string(id%100<10?"0":"")+std::to_string(id%100)+".wav");
 if(!std::filesystem::exists(path))path=assets/battle_sound_file(id);
 if(!SDL_LoadWAV(path.string().c_str(),&spec,&pcm,&bytes))throw std::runtime_error("missing attack sound: "+path.string());
 auto* stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
 if(stream){SDL_PutAudioStreamData(stream,pcm,bytes);SDL_ResumeAudioStreamDevice(stream);attack_sounds.push_back(stream);}
 SDL_free(pcm);
}
void begin_presentation(){
 const auto& packet=playback.packet();
 attack_actor=packet.at("actor");attack_target=packet.value("target",-1);++attack_serial;
 presented_poses.clear();presented_flash=false;
 log_presentation({{"event",playback.moving()?"movement_begin":packet.value("kind",std::string())=="environment"?"environment_begin":"attack_begin"},{"actor",attack_actor},{"target",attack_target},
  {"phase",packet.at("before").at("phase")},{"turn",packet.at("before").at("turn")},{"damage",packet.value("damage",0)},{"event_count",packet.at("events").size()}});
 for(const auto& e:packet.at("events"))if(e.at("kind")=="notice")log_presentation({{"event","presentation_notice"},{"notice",e}});
}
void collect_presentation(){
 auto movement=game->take_movement(),attack=game->take_attack(),environment=game->take_environment();if(movement.is_null()&&attack.is_null()&&environment.is_null())return;
 // The special board is fitted down on phones. Keep healing feedback readable
 // without changing simulation timing, HP, random draws or saved state.
 if(special_hulao&&!environment.is_null()){
  bool healing=false;for(auto& e:environment["events"]){
   if(e.at("kind")=="number")healing=e.value("healing",false)&&e.at("value")>0;
   if(e.at("kind")=="wait"&&healing)e["ticks"]=std::max(30,e.at("ticks").get<int>());
   if(e.at("kind")=="notice_clear")healing=false;
  }
 }
 stop_attack_sounds();playback.start(movement,attack,environment);animation_clock=SDL_GetTicks();
 begin_presentation();for(int sound:playback.take_sounds())attack_sound(sound);
}
void restore_snapshot(const std::string& snapshot){
 if(special_hulao)caocao::special_roster::restore(*game,snapshot,voxel_manifest);else {if(Json::parse(snapshot).at("payload").at("schema")>=11)throw std::runtime_error("Open duel saves in the special battle.");game->restore(snapshot);}
 if(game->revised_rules())game->enable_equipment();game->enable_tactics();game->enable_terrain_benefits();game->enable_items();cancel_jev();if(playback.active())log_presentation({{"event","presentation_cancelled_by_restore"}});playback.clear();stop_attack_sounds();animation_clock=SDL_GetTicks();next_enemy=animation_clock+450;
 clear_touch_hover();close_action_menu();show_inventory=false;show_unit_details=false;if(!game->state().at("undo").is_null())menu_after_move_unit=game->state().at("selected");
 special_choosing=false;special_has_match=true;diagnostic_dirty=true;log_presentation({{"event","restore"},{"snapshot_sha256",sha256(snapshot)},{"state_hash",game->hash()}});
}
void draw_unit_pose(const Json& pose){
 const std::string archive=pose.at("archive");
 std::string stem=archive=="UNIT_ATK.E5"?"Unit_atk":archive=="UNIT_SPC.E5"?"Unit_spc":archive=="UNIT_MOV.E5"?"Unit_mov":"";
 if(stem.empty())throw std::runtime_error("unsupported attack sprite archive");
 int size=pose.at("width"),frame=pose.at("frame"),sprite=pose.at("sprite");
 const auto& effects=pose.at("effects");std::string key="strike:"+stem+":"+std::to_string(sprite)+":"+std::to_string(frame)+effects.dump();
 if(!textures.contains(key)){
  std::ifstream f(assets/"raw"/(stem+".e5")/(number3(sprite)+".bin"),std::ios::binary);
  std::vector<uint8_t> pixels(size*size);f.seekg(frame*size*size);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
  if(!f)throw std::runtime_error("missing indexed attack frame: "+key);
  pixels=transform_combat_pixels(std::move(pixels),effects,ui.at("combat_palette").at("quantized"),ui.at("combat_palette").at("lookup"));
  auto* surface=SDL_CreateSurface(size,size,SDL_PIXELFORMAT_RGBA32);if(!surface)throw std::runtime_error(SDL_GetError());
  for(int y=0;y<size;++y)for(int x=0;x<size;++x){int index=pixels[y*size+x];const auto& color=ui.at("palette").at(index);
   auto* out=static_cast<uint8_t*>(surface->pixels)+y*surface->pitch+x*4;out[0]=color[0];out[1]=color[1];out[2]=color[2];out[3]=index?255:0;
  }
  auto* tex=SDL_CreateTextureFromSurface(renderer,surface);SDL_DestroySurface(surface);if(!tex)throw std::runtime_error(SDL_GetError());
  SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(tex,SDL_SCALEMODE_NEAREST);textures[key]=tex;
 }
 // Recovered animation coordinates use the original 56-pixel map origin.
 SDL_FRect to{pose.at("x").get<float>()*map_scale(),(pose.at("y").get<float>()-56)*map_scale()+map_top,size*map_scale(),size*map_scale()};
 SDL_RenderTextureRotated(renderer,textures.at(key),nullptr,&to,0,nullptr,pose.at("mirror").get<bool>()?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);
}
void fill(float x,float y,float w,float h,SDL_Color c){
 SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_FRect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);
}
void image(const std::string& path,SDL_FRect to,const SDL_FRect* from=nullptr){SDL_RenderTexture(renderer,texture(path),from,&to);}
bool inside(float x,float y,SDL_FRect r){return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
SDL_FRect tile_rect(int x,int y){if(special_hulao){auto p=voxel::project(x,y);return {p.x,p.y,voxel::tile_width,voxel::tile_height};}return {x*map_tile(),map_top+y*map_tile(),map_tile(),map_tile()};}
std::array<int,2> map_cell(float x,float y){
 if(special_hulao){int id=voxel_unit_at(x,y);if(id>=0){const auto& b=visible_state().at("units").at(std::to_string(id)).at("battle");return {b.at("x"),b.at("y")};}auto camp=voxel_camp_at(x,y);return camp[0]>=0?camp:voxel::cell(x,y);}
 if(x<0||x>=game->map_width()*map_tile()||y<map_top||y>=map_top+game->map_height()*map_tile())return {-1,-1};
 return {int(x/map_tile()),int((y-map_top)/map_tile())};
}
void bevel(SDL_FRect r,bool pressed=false){
 fill(r.x,r.y,r.w,r.h,paper);
 SDL_Color light=pressed?SDL_Color{100,100,100,255}:white,dark=pressed?white:SDL_Color{100,100,100,255};
 fill(r.x,r.y,r.w,2,light);fill(r.x,r.y,2,r.h,light);
 fill(r.x,r.y+r.h-2,r.w,2,dark);fill(r.x+r.w-2,r.y,2,r.h,dark);
}
// Keep original logical advances/cell geometry; sample the denser sans atlas.
void text_glyph(SDL_Texture* atlas,const SDL_FRect& from,const SDL_FRect& to){
 SDL_FRect source{from.x*font_raster_scale,from.y*font_raster_scale,from.w*font_raster_scale,from.h*font_raster_scale};
 SDL_RenderTexture(renderer,atlas,&source,&to);
}
// English uses word wrapping and measured fitting, independent of mouse hitboxes.
float text_width(const std::string& s,float scale=1){float w=0;for(auto cp:codepoints(tr(s)))w+=font.at("glyphs").at(std::to_string(cp)).at("advance").get<float>()*scale;return w;}
std::vector<std::string> english_lines(const std::string& s,float w,float scale){
 std::vector<std::string> out;std::string line,word;
 auto append=[&](){if(word.empty())return;auto candidate=line.empty()?word:line+" "+word;
  if(!line.empty()&&text_width(candidate,scale)>w){out.push_back(line);line.clear();}
  if(!line.empty())line+=' ';line+=word;word.clear();};
 for(char c:s){if(c==' '||c=='\n'){append();if(c=='\n'){out.push_back(line);line.clear();}}else word+=c;}
 append();if(!line.empty())out.push_back(line);return out;
}
void english_text(const std::string& source,float x,float y,float right,SDL_Color color,int lines,float scale,float step){
 const auto s=tr(source);if(s.empty()||right<=x)return;
 auto wrapped=english_lines(s,right-x,scale);
 // Most controls fit at full size. Fit only the translated label, not the panel.
 for(int i=0;i<45&&(int(wrapped.size())>lines||std::any_of(wrapped.begin(),wrapped.end(),[&](const auto& a){return text_width(a,scale)>right-x;}));++i){scale*=.95f;wrapped=english_lines(s,right-x,scale);}
 auto* atlas=texture("images/font.bmp");SDL_SetTextureColorMod(atlas,color.r,color.g,color.b);float cell=font.at("cell");
 for(const auto& line:wrapped){float xx=x;for(auto cp:codepoints(line)){const auto& g=font.at("glyphs").at(std::to_string(cp));SDL_FRect from{g.at("x"),g.at("y"),cell,cell},to{xx,y,cell*scale,cell*scale};text_glyph(atlas,from,to);xx+=g.at("advance").get<float>()*scale;}y+=step;}
}
void text(const std::string& s,float x,float y,float right=width-8,SDL_Color color=ink,int lines=1){
 if(battle_language.english()){english_text(s,x,y,right,color,lines,1,23);return;}
 auto* t=texture("images/font.bmp");SDL_SetTextureColorMod(t,color.r,color.g,color.b);
 float start=x;int cell=font.at("cell"),line=1;
 for(auto cp:codepoints(s)){
  if(cp==10){if(++line>lines)break;x=start;y+=23;continue;}
  auto key=std::to_string(cp);if(!font.at("glyphs").contains(key))throw std::runtime_error("missing prototype font glyph "+key);
  auto g=font.at("glyphs").at(key);float advance=g.at("advance");
  if(x+advance>right){if(++line>lines)break;x=start;y+=23;}
  SDL_FRect from{g.at("x"),g.at("y"),float(cell),float(cell)},to{x,y,float(cell),float(cell)};
  text_glyph(t,from,to);x+=advance;
 }
}
void ribbon(float x,float y,float w,const std::string& label){
 fill(x,y,w,28,{15,20,100,255});
 for(int row=0;row<28;++row)fill(x,y+row,w,1,{Uint8(8+row),Uint8(16+row),Uint8(130-row*3),255});
 text(label,x+12,y+4,x+w-8,white);
}
#include "prototype_unit_window.inc"
#include "prototype_treasure_window.inc"
int inspected_unit();
int unit_at_pointer(float x,float y){
 auto cell=map_cell(x,y);if(cell[0]<0)return -1;
 for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("hp").get<int>()>0&&b.at("lifecycle")==2&&b.at("x")==cell[0]&&b.at("y")==cell[1])return b.at("id");}return -1;
}
bool send(const std::string& cmd,int x=0,int y=0){
 if(special_choosing)return false;
 if(playback.active()){notice="动画中，请等待。";log_presentation({{"event","command_blocked"},{"command",cmd},{"reason","animation_active"}});return false;}
 diagnostic_action(cmd,{{"x",x},{"y",y}});
 try{if(cmd=="restart"&&special_hulao){request_special_selection();return true;}if(cmd=="restart")cancel_jev();if(cmd=="tactic")game->cast(x,y);else if(cmd=="item")game->use_item(x,y);else game->command(cmd,x,y);log_presentation({{"event","command"},{"command",cmd},{"x",x},{"y",y},{"state_hash",game->hash()}});collect_presentation();close_action_menu();notice.clear();next_enemy=SDL_GetTicks()+350;return true;}
 catch(const std::exception& e){notice=e.what();log_presentation({{"event","command_error"},{"command",cmd},{"x",x},{"y",y},{"message",e.what()}});return false;}
}
std::filesystem::path save_directory(){
 const char* override=std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR");std::filesystem::path p;
 if(override)p=override;
 else if(const char* xdg=std::getenv("XDG_DATA_HOME"))p=std::filesystem::path(xdg)/"caocao-prototype";
 else {const char* home=std::getenv("HOME");if(!home)throw std::runtime_error("HOME unavailable");p=std::filesystem::path(home)/".local/share/caocao-prototype";}
 if(special_hulao)p/="special-hulao-duel-v1";
 std::filesystem::create_directories(p);return p;
}
std::filesystem::path save_path(){
 auto p=save_directory();if(manual_slots&&save_slot>0)return p/("manual-slot-"+std::to_string(save_slot)+".json");return p/(game->scenario()?"slot-battle-"+std::to_string(game->scenario())+".json":"slot-1.json");
}
void persist(bool load){
 if(special_choosing&&!load){notice="请先选择武将。";return;}
 if(backup_busy)return;
 if(!load&&manual_slots&&save_slot==0)save_slot=1;
 if(playback.active()){notice="动画中，请等待。";return;}
#ifndef __EMSCRIPTEN__
 if(!load&&jev&&jev->busy()){notice="JEV: please wait before saving.";return;}
#endif
#ifdef __EMSCRIPTEN__
 EM_ASM({if($0)window.prototypeLoad();else window.prototypeSave();},load);
#else
 try{
  auto path=save_path();
  log_presentation({{"event",load?"load_begin":"save_begin"},{"slot",save_slot},{"file",path.filename().string()}});
  if(load){std::ifstream f(path);if(!f)throw std::runtime_error("没有可读取的存档。");std::string s{std::istreambuf_iterator<char>(f),{}};restore_snapshot(s);notice="已读取存档。";}
  else{auto s=game->serialize();auto pending=path;pending+=".pending";
   {std::ofstream f(pending,std::ios::binary|std::ios::trunc);f<<s;f.flush();if(!f)throw std::runtime_error("保存失败，原存档未改动。");}
   std::filesystem::rename(pending,path);notice="已保存。";
  }
  diagnostic_dirty=true;log_presentation({{"event",load?"load_success":"save_success"},{"slot",save_slot},{"state_hash",game->hash()}});
 }catch(const std::exception& e){notice=e.what();log_presentation({{"event",load?"load_error":"save_error"},{"slot",save_slot},{"message",e.what()}});}
#endif
}
#include "prototype_saves_window.inc"
struct Tool {const char* label;const char* hint;int icon;};
const Tool toolbar[]{
 {"待命","待命 W：结束所选部队的行动。",8},
 {"取消","取消 Esc / 右键：撤回移动和选择。",9},
 {"结束回合","结束回合 E：所有我军待命，敌军开始行动。",11},
 {"保存","保存 F5：保存到本地存档。",2},
 {"读取","读取 F9：恢复本地存档。",1},
 {"重新开始","重新开始 R：重新开始战斗，保留已有存档。",0},
 {"情报","情报 I：胜利条件、部队情报和操作说明。",4},
 {"道具","道具 P：查看数量，选择道具后点击自己或周围友军。",3}
};
SDL_FRect tool_rect(int i){float step=special_hulao?190:119;return {8+i*step,6,step-7,40};}
constexpr SDL_FRect info_close{636,322,92,32};
constexpr int action_ids[]{1287,1288,1289,1291,2};
constexpr float action_menu_scale=1.5f;
const Json& action_control(int row){
 for(const auto& c:ui.at("command_menu").at("controls"))if(c.at("id")==action_ids[row])return c;
 throw std::runtime_error("missing original action menu control");
}
bool actionable_unit(){
 int id=game->state().at("selected");
 if(id<0||playback.active()||game->has_dialogue()||game->state().at("phase")!="player")return false;
 const auto& b=game->state().at("units").at(std::to_string(id)).at("battle");
 return b.at("hp").get<int>()>0&&b.at("lifecycle")==2&&(game->revised_rules()?b.at("side")==0:b.at("side")!=2)&&!(b.at("flags").get<int>()&2)&&!(game->revised_rules()&&(b.at("status").get<int>()&8));
}
bool moved_unit(){return actionable_unit()&&!game->state().at("undo").is_null();}
bool action_menu_ready(){return moved_unit()||(special_hulao&&actionable_unit());}
bool action_enabled(int row){
 if(!actionable_unit())return false;
 if(row==0){int id=game->state().at("selected");for(const auto& u:game->state().at("units"))if(game->attackable(id,u.at("battle").at("id")))return true;return false;}
 if(row==1)return !game->spells(game->state().at("selected")).empty();
 if(row==2){for(int id:item_ids)if(!game->item_options(game->state().at("selected"),id).empty())return true;return false;}
 return row==3||row==4;
}
SDL_FRect action_menu_rect(int x,int y){
 const auto& bounds=ui.at("command_menu").at("dialog").at("bounds_dlu");
 float w=(bounds.at(2).get<float>()*2+2)*action_menu_scale,h=(bounds.at(3).get<float>()*1.5f+2)*action_menu_scale;
 auto unit=tile_rect(x,y);float px=unit.x<width/2?unit.x+unit.w+16:unit.x-w-16;
 float bottom=map_top+game->map_height()*map_tile();
 return {std::clamp(px,4.f,width-w-4),std::clamp(unit.y+(unit.h-h)/2,map_top+4.f,bottom-h-4),w,h};
}
SDL_FRect action_menu_rect(){
 const auto& b=game->state().at("units").at(std::to_string(action_menu_unit)).at("battle");
 return action_menu_rect(b.at("x"),b.at("y"));
}
SDL_FRect action_row(int row){
 auto menu=action_menu_rect();const auto& b=action_control(row).at("bounds_dlu");
 return {menu.x+(1+b.at(0).get<float>()*2)*action_menu_scale,menu.y+(1+b.at(1).get<float>()*1.5f)*action_menu_scale,b.at(2).get<float>()*2*action_menu_scale,b.at(3).get<float>()*1.5f*action_menu_scale};
}
void open_action_menu(){
 if(!action_menu_ready()){close_action_menu();return;}
 if(show_info||show_battles)return;
 close_action_menu();action_menu_unit=game->state().at("selected");action_menu_focus=action_enabled(0)?0:3;hover_panel_target=-1;
}
const char* action_hint(int row){
 constexpr const char* hints[]{"攻击：选择标记目标。","策略：选择策略和目标。","道具：选择道具，再点击自己或周围友军。","待命：结束这支部队的行动。","取消：撤回移动和选择。"};return hints[row];
}
void activate_action(int row){
 if(!action_enabled(row)){notice=action_hint(row);return;}
 if(row==0){close_action_menu();attack_targeting=true;notice=action_hint(0);}
 else if(row==1){close_action_menu();show_spells=true;notice.clear();}
 else if(row==2){close_action_menu();show_inventory=true;treasure_view=false;notice.clear();}
 else if(row==3)send("wait");else if(row==4)send("cancel");
}
bool can_use_item(int row){return row>=0&&row<3&&actionable_unit()&&!game->item_options(game->state().at("selected"),item_ids[row]).empty();}
void choose_item(int row){
 if(!can_use_item(row))return;
 close_action_menu();target_item=item_ids[row];show_inventory=false;
 notice=game->data().at("items").at(target_item).at("name").get<std::string>()+"：点击自己或周围八格友军；无效果也会消耗。右键取消。";
}
bool action_key(SDL_Keycode key,bool reverse){
 if(show_spells){
  if(key==SDLK_F5||key==SDLK_F9)return false;
  if(key==SDLK_ESCAPE){open_action_menu();return true;}
  auto list=game->spells(game->state().at("selected"));int row=int(key-SDLK_1);
  if(row>=0&&row<int(list.size())){target_tactic=list.at(row).at("id");show_spells=false;notice=list.at(row).at("name").get<std::string>()+"：选择标记目标。";}
  return true;
 }
 if(target_item>=0&&key==SDLK_ESCAPE){close_action_menu();show_inventory=true;treasure_view=false;notice.clear();return true;}
 if(target_tactic>=0&&key==SDLK_ESCAPE){open_action_menu();return true;}
 if(action_menu_unit>=0&&!action_menu_ready()){close_action_menu();return false;}
 if(attack_targeting&&key==SDLK_ESCAPE){open_action_menu();return true;}
 if(action_menu_unit<0){if(key==SDLK_SPACE&&action_menu_ready()){open_action_menu();return true;}return false;}
 if(key==SDLK_ESCAPE){activate_action(4);return true;}
 if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_SPACE){activate_action(action_menu_focus);return true;}
 if(key==SDLK_UP||key==SDLK_DOWN||key==SDLK_TAB){
  int delta=key==SDLK_UP||(key==SDLK_TAB&&reverse)?-1:1;
  do{action_menu_focus=(action_menu_focus+delta+5)%5;}while(!action_enabled(action_menu_focus));
  action_menu_keyboard=true;return true;
 }
 return false;
}
void draw_action_menu(){
 auto menu=action_menu_rect();fill(menu.x+3,menu.y+3,menu.w,menu.h,{0,0,0,110});fill(menu.x,menu.y,menu.w,menu.h,ink);
 fill(menu.x+1,menu.y+1,menu.w-2,menu.h-2,white);
 for(int row=0;row<5;++row){
  auto r=action_row(row);bool enabled=action_enabled(row),hover=inside(mouse_x,mouse_y,r);
  fill(r.x,r.y,r.w,r.h,{249,249,249,255});
  if(hover&&enabled)fill(r.x+2,r.y+2,r.w-4,r.h-4,{222,232,241,255});
  fill(r.x,r.y,r.w,2,white);fill(r.x,r.y,2,r.h,white);
  fill(r.x,r.y+r.h-2,r.w,2,{145,145,145,255});fill(r.x+r.w-2,r.y,2,r.h,{145,145,145,255});
  if(row<4){
   auto path="images/pe/Cmdicon.dll-"+std::to_string(100+row)+"-1041.bmp";auto* icon=texture(path);
   SDL_SetTextureAlphaMod(icon,enabled?255:90);
   float size=16*action_menu_scale;
   for(float x:{r.x+3,r.x+r.w-size-3}){SDL_FRect dst{x,r.y+(r.h-size)/2,size,size};SDL_RenderTexture(renderer,icon,nullptr,&dst);}SDL_SetTextureAlphaMod(icon,255);
  }
  // The original dialog uses a smaller font than the battlefield. Fit its two
  // Chinese glyphs between the paired 16px icons at the screenshot's spacing.
  auto* atlas=texture("images/font.bmp");auto c=enabled?ink:SDL_Color{166,166,166,255};SDL_SetTextureColorMod(atlas,c.r,c.g,c.b);
  constexpr float glyph_scale=.75f*action_menu_scale;
  auto label=codepoints(tr(action_control(row).at("text")));float total=0;for(auto cp:label)total+=font.at("glyphs").at(std::to_string(cp)).at("advance").get<float>()*glyph_scale;
  float x=r.x+(r.w-total)/2,y=r.y+(r.h-16*glyph_scale)/2;
  for(auto cp:label){const auto& g=font.at("glyphs").at(std::to_string(cp));float cell=font.at("cell");SDL_FRect from{g.at("x"),g.at("y"),cell,cell},to{x,y,cell*glyph_scale,cell*glyph_scale};text_glyph(atlas,from,to);x+=g.at("advance").get<float>()*glyph_scale;}
  if(action_menu_keyboard&&row==action_menu_focus){
   for(float x=r.x+4;x<r.x+r.w-4;x+=4){fill(x,r.y+4,2,1,ink);fill(x,r.y+r.h-5,2,1,ink);}
   for(float y=r.y+4;y<r.y+r.h-4;y+=4){fill(r.x+4,y,1,2,ink);fill(r.x+r.w-5,y,1,2,ink);}
  }
 }
}
bool enabled(int i){
 if(special_choosing)return i==4;
 if(i==7)return game->terrain_benefits_enabled();
 if(playback.active())return i==6;
 if(game->has_dialogue())return i==3||i==4||i==5;
 bool player=game->state().at("phase")=="player";
 return i>=3||(player&&(i==2||game->state().at("selected").get<int>()>=0));
}
void button(int i){
 if(!enabled(i))return;
#ifdef __EMSCRIPTEN__
 if(web_hosted&&i==5){EM_ASM({window.prototypeRestart?.();});return;}
#endif
 if(i==0)send("wait");else if(i==1)send("cancel");else if(i==2)send("end_turn");
 else if(i==3)persist(false);else if(i==4)persist(true);else if(i==5){send("restart");show_info=false;}
 else if(i==7){close_action_menu();show_inventory=true;treasure_view=false;show_info=false;show_battles=false;}
 else {close_action_menu();show_info=!show_info;}
}
void restart_special(int hero,int enemy,int ally,int ally2){
 std::ifstream f(assets/"special-hulao/seed.json");if(!f)throw std::runtime_error("Missing special battle seed");
 game->start_hulao_duel(std::string{std::istreambuf_iterator<char>(f),{}},hero,enemy,ally,ally2,true);restore_snapshot(game->serialize());show_info=show_battles=show_inventory=show_slots=false;notice.clear();
}
constexpr std::array<int,4> special_heroes{0,6,1,11};
void request_special_selection(){
 if(playback.active()||backup_busy)return;
 cancel_jev();close_action_menu();clear_touch_hover();show_unit_details=show_info=show_battles=show_inventory=show_slots=false;
 special_choosing=true;notice.clear();
}
void select_special_hero(int index){
 if(!special_choosing||index<0||index>=int(special_heroes.size()))return;
 const auto previous_phase=special_has_match?game->state().at("phase").get<std::string>():"setup";
 // Host entropy is used only during match selection. Never consume combat RNG.
 const auto entropy=[]()->uint32_t{
#ifdef __EMSCRIPTEN__
 return EM_ASM_INT({const a=new Uint32Array(1);crypto.getRandomValues(a);return a[0];});
#else
 return std::random_device{}();
#endif
 };
 const uint32_t draw=entropy();const int hero=special_heroes[index],enemy=Prototype::duel_enemy_for_draw(draw);
 uint32_t ally_draw=0;
 if(enemy==127){
  // Uniformly choose one of three distinct teammates. Reject the excess value.
  do{ally_draw=entropy();}while(ally_draw==UINT32_MAX);
 }
 std::ifstream f(assets/"special-hulao/seed.json");if(!f)throw std::runtime_error("Missing special battle seed");
 game->start_hulao_squad(std::string{std::istreambuf_iterator<char>(f),{}},hero,enemy,ally_draw);
 restore_snapshot(game->serialize());show_info=show_battles=show_inventory=show_slots=false;notice.clear();
 Json selection{{"event","special_match_selected"},{"hero",game->duel_hero()},{"enemy",game->duel_enemy()},{"ally",game->duel_ally()},{"ally2",game->duel_ally2()},{"ally3",game->duel_ally3()},{"previous_phase",previous_phase},{"draw",draw}};
 if(enemy==127)selection["ally_draw"]=ally_draw;
 log_presentation(selection);
#ifdef __EMSCRIPTEN__
 // Local diagnostics must never interrupt a successful match or consume RNG.
 const auto record=selection.dump();
 EM_ASM({try{window.prototypeSpecialSelected?.(JSON.parse(UTF8ToString($0)));}catch(e){console.warn("Match diagnostics unavailable");}},record.c_str());
#endif
}
SDL_FRect special_hero_card(int index){return {108.f+(index%2)*680.f,224.f+(index/2)*380.f,640,344};}
void choose_battle(int id){
 if(special_hulao){request_special_selection();return;}
 diagnostic_action("start_scenario",{{"scenario_requested",id}});
 try{cancel_jev();game->start_scenario(id);clear_touch_hover();playback.clear();stop_attack_sounds();close_action_menu();show_battles=false;show_info=false;show_inventory=false;show_unit_details=false;notice.clear();next_enemy=SDL_GetTicks()+450;}
 catch(const std::exception& e){notice=e.what();log_presentation({{"event","scenario_error"},{"scenario_requested",id},{"message",e.what()}});}
}
void select_story(int index){diagnostic_action("story_choice",{{"choice",index}});try{game->choose_story(index);collect_presentation();notice.clear();log_presentation({{"event","story_choice"},{"choice",index},{"state_hash",game->hash()}});}catch(const std::exception& e){notice=e.what();log_presentation({{"event","story_choice_error"},{"choice",index},{"message",e.what()}});}}
SDL_FRect story_row(int n){return {200,450.f+n*56,560,48};}
void click(float x,float y,bool right,bool inspect=false){
 if(x<0||x>=width||y<0||y>=height)return;
 if(special_choosing&&!show_slots){
  if(!right)for(int i=0;i<4;++i)if(inside(x,y,special_hero_card(i))){select_special_hero(i);return;}
  if(!right&&inside(x,y,{108,1010,400,64}))persist(true);
  else if(special_has_match&&(!right&&inside(x,y,{1028,1010,400,64})))special_choosing=false;
  return;
 }
 if(!playback.active()&&!game->story_choices().empty()){auto options=game->story_choices().at("options");for(size_t n=0;n<options.size();++n)if(!right&&inside(x,y,story_row(int(n))))select_story(int(n));return;}
 if(show_slots){if(right)show_slots=false;else slots_click(x,y);return;}
 if(!right&&manual_slots&&inside(x,y,slots_button)){open_slots();return;}
 if(backup_busy)return;
 if(show_unit_details){detail_click(x,y,right);return;}
 if(inspect){open_unit_details(unit_at_pointer(x,y));return;}
 if(!right&&inside(x,y,unit_button)){open_unit_details(inspected_unit());return;}
 if(show_inventory&&treasure_view){treasure_click(x,y,right);return;}
 if(!right&&inside(x,y,treasure_button)){open_treasures();return;}
 if(show_inventory){if(right||inside(x,y,inventory_close)||inside(x,y,inventory_button)||inside(x,y,tool_rect(7)))show_inventory=false;else if(inside(x,y,{244,820,472,36})){open_treasures();}else for(int row=0;row<3;++row)if(inside(x,y,item_use_rect(row)))choose_item(row);return;}
 if(!right&&inside(x,y,inventory_button)&&game->terrain_benefits_enabled()){close_action_menu();show_inventory=true;treasure_view=false;show_info=false;show_battles=false;return;}
 if(show_battles){if(right){show_battles=false;return;}for(int id=0;id<game->scenario_count();++id)if(inside(x,y,battle_choice(id))){choose_battle(id);return;}if(inside(x,y,{228,644,504,40}))show_battles=false;return;}
 if(!right&&inside(x,y,battle_button)&&!playback.active()){
#ifdef __EMSCRIPTEN__
  if(web_hosted){EM_ASM({window.prototypeHome?.();});return;}
#endif
  close_action_menu();if(special_hulao)show_info=true;else show_battles=true;return;}
 if(!right&&!show_info&&!showing_dialogue()&&!playback.active()&&game->state().at("phase")=="victory"&&inside(x,y,camp_button)){open_unit_details(0);detail_tab=1;return;}
 if(!right&&!show_info&&!showing_dialogue()&&!playback.active()&&game->state().at("phase")=="victory"&&inside(x,y,next_button)){
#ifdef __EMSCRIPTEN__
  if(web_hosted){EM_ASM({window.prototypeNextBattle?.();});return;}
#endif
  if(special_hulao){request_special_selection();return;}
  if(game->scenario()+1<game->scenario_count()){if(game->revised_rules()){try{diagnostic_action("advance_campaign");cancel_jev();game->advance_campaign();close_action_menu();notice.clear();log_presentation({{"event","campaign_advanced"},{"state_hash",game->hash()}});}catch(const std::exception& e){notice=e.what();log_presentation({{"event","campaign_error"},{"message",e.what()}});}}else choose_battle(game->scenario()+1);}else show_battles=true;return;}
 if(show_info){if(right||inside(x,y,info_close))show_info=false;return;}
 if(showing_dialogue()){
  if(!right){for(int i=0;i<8;++i)if(inside(x,y,tool_rect(i))){button(i);return;}
   if(inside(x,y,{36,768,888,264}))send("continue");}
  return;
 }
 if(right){if(!playback.active()&&game->state().at("undo").is_null()&&action_menu_unit<0&&!attack_targeting&&!show_spells&&target_tactic<0&&target_item<0){int id=unit_at_pointer(x,y);if(id>=0){open_unit_details(id);return;}}if(target_item>=0){close_action_menu();show_inventory=true;treasure_view=false;notice.clear();return;}if(attack_targeting||show_spells||target_tactic>=0)open_action_menu();else send("cancel");return;}
 for(int i=0;i<8;++i)if(inside(x,y,tool_rect(i))){button(i);return;}
 if(show_spells){auto list=game->spells(game->state().at("selected"));for(size_t n=0;n<list.size();++n)if(inside(x,y,{310,340.f+float(n)*40,340,36})){target_tactic=list.at(n).at("id");show_spells=false;notice=list.at(n).at("name").get<std::string>()+"：选择标记目标。";return;}if(inside(x,y,{310,348.f+float(list.size())*40,340,40}))open_action_menu();return;}
 if(action_menu_unit>=0&&inside(x,y,action_menu_rect())){
  for(int row=0;row<5;++row)if(inside(x,y,action_row(row))){activate_action(row);break;}return;
 }
 auto cell=map_cell(x,y);if(cell[0]<0)return;
 int selected=game->state().at("selected");
 // Tall voxel horses/chariots overlap adjacent tiles. During movement choice,
 // a reachable empty ground tile must remain clickable beneath that artwork.
 // Inspection and attack targeting still use the miniature's alpha hitbox.
 if(special_hulao&&selected>=0&&game->state().at("undo").is_null()&&action_menu_unit<0&&!attack_targeting&&target_tactic<0&&target_item<0){
  auto ground=voxel_camp_at(x,y);if(ground[0]<0)ground=voxel::cell(x,y);
  if(ground[0]>=0){bool occupied=false;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==2&&b.at("x")==ground[0]&&b.at("y")==ground[1])occupied=true;}
   if(!occupied&&game->movement().at(ground[1]*20+ground[0])!=255)cell=ground;
  }
 }
 if(target_item>=0){
  for(const auto& o:game->item_options(selected,target_item)){const auto& b=game->state().at("units").at(std::to_string(o.at("target").get<int>())).at("battle");if(b.at("x")==cell[0]&&b.at("y")==cell[1]){send("item",target_item,o.at("target"));return;}}
  notice="请选择绿色目标：自己或周围八格友军。";return;
 }
 if(target_tactic>=0){
  const auto& b=game->state().at("units").at(std::to_string(selected)).at("battle");
  for(const auto& o:game->spell_options(selected,{b.at("x"),b.at("y")}))if(o.at("tactic")==target_tactic){const auto& target=game->state().at("units").at(std::to_string(o.at("target").get<int>())).at("battle");if(target.at("x")==cell[0]&&target.at("y")==cell[1]){send("tactic",target_tactic,o.at("target"));return;}}
  notice="策略目标、地形或策略值无效。";return;
 }
 if(attack_targeting){
  for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("x")==cell[0]&&b.at("y")==cell[1]&&game->attackable(selected,b.at("id"))){send("tile",cell[0],cell[1]);return;}}
  notice="请选择红色目标，或右键返回。";return;
 }
 if(action_menu_ready()){
  const auto& b=game->state().at("units").at(std::to_string(selected)).at("battle");
  if(b.at("x")==cell[0]&&b.at("y")==cell[1]){open_action_menu();return;}
 }
 action_menu_unit=-1;send("tile",cell[0],cell[1]);
}
int hovered_unit(){
 auto cell=map_cell(mouse_x,mouse_y);if(cell[0]<0||show_unit_details||show_info||show_inventory||show_battles||show_spells||target_tactic>=0||target_item>=0||action_menu_unit>=0)return -1;
 for(const auto& u:visible_state().at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp>0&&b.lifecycle==2&&b.x==cell[0]&&b.y==cell[1])return b.id;}
 return -1;
}
int inspected_unit(){if(playback.active())return playback.notice().is_null()?attack_actor:playback.notice().at("actor").get<int>();int hover=hovered_unit();return hover>=0?hover:visible_state().at("inspected").get<int>();}
int hover_panel_unit(uint64_t now){
 const auto& phase=visible_state().at("phase");
 int id=playback.active()||showing_dialogue()||show_info||show_inventory||show_battles||
  (phase!="player"&&phase!="enemy")?-1:hovered_unit();
 if(touch_hover_input&&id!=touch_hover_unit)id=-1;
 if(id!=hover_panel_target){hover_panel_target=id;hover_panel_since=now;}
 return id>=0&&now-hover_panel_since>=hover_panel_delay?id:-1;
}
SDL_FRect hover_panel_rect(int x,int y){
 auto unit=tile_rect(x,y);
 constexpr float w=288,h=144,gap=8;
 float px=unit.x+unit.w+gap;
 if(px+w>width-gap)px=unit.x-w-gap;
 float bottom=map_top+game->map_height()*map_tile();
 return {std::clamp(px,gap,width-w-gap),std::clamp(unit.y,map_top+gap,bottom-h-gap),w,h};
}
Json hover_panel_events(int id){
 const auto& u=visible_state().at("units").at(std::to_string(id));
 auto b=BattleUnit::from_json(u.at("battle"));auto r=UnitRecord::from_json(u.at("record"));
 int terrain=game->terrain().at("cells").at(b.y*game->map_width()+b.x);
 // The recovered panel expects the original 13x8 viewport. Only its placement
 // is normalized; the unit values, labels, glyph positions and bars are reused.
 b.x=6;b.y=4;
 return battle_unit_panel(b,r,u.at("name"),{0,0},terrain,game->data(),ui);
}
void draw_hover_panel(int id){
 const auto& b=visible_state().at("units").at(std::to_string(id)).at("battle");
 auto rect=hover_panel_rect(b.at("x"),b.at("y"));auto events=hover_panel_events(id);
 const auto& origin=events.at(0).at("args");
 float ox=origin.at(0).get<int>()*48,oy=56+origin.at(1).get<int>()*48;
 constexpr float scale=1.5f;
 auto px=[&](float x){return rect.x+(x-ox)*scale;};
 auto py=[&](float y){return rect.y+(y-oy)*scale;};
 auto color=[&](int index){const auto& c=ui.at("palette").at(index);return SDL_Color{c.at(0),c.at(1),c.at(2),255};};
 auto box=[&](float x,float y,float w,float h,int index){fill(px(x),py(y),w*scale,h*scale,color(index));};
 SDL_Rect old_clip;bool clipped=SDL_RenderClipEnabled(renderer);SDL_GetRenderClipRect(renderer,&old_clip);
 SDL_Rect clip{int(rect.x),int(rect.y),int(rect.w),int(rect.h)};SDL_SetRenderClipRect(renderer,&clip);
 int pattern=128;
 for(const auto& event:events){
  const auto& kind=event.at("kind");auto a=event.value("args",Json::array());
  if(kind=="window"){
   // Adapt the original darkened battlefield backing to this full-map viewport.
   fill(rect.x,rect.y,rect.w,rect.h,{0,0,0,210});
   for(int n=0;n<3;++n){box(ox,oy+93+n,192,1,29+n);box(ox+189+n,oy,1,96,29+n);}
   box(ox,oy,192,1,24);box(ox,oy,1,96,24);
  }else if(kind=="text"){
   if(battle_language.english()){
    float ex=event.at("x"),ey=event.at("y");
    // Header and footer are composed once below; numerical bars keep their values.
    if(ey==oy+4||ey==oy+75)continue;
    auto c=color(event.at("foreground"));if(event.at("opaque").get<bool>())box(ex,ey,112,16,event.at("background"));
    detail_text(event.at("text"),px(ex),py(ey),rect.x+rect.w-8,c,1,scale);continue;
   }
   auto* atlas=texture("images/font.bmp");auto c=color(event.at("foreground"));SDL_SetTextureColorMod(atlas,c.r,c.g,c.b);
   for(const auto& glyph:event.at("glyphs")){
    std::string ch=glyph.at(2);float x=glyph.at(0),y=glyph.at(1);
    if(event.at("opaque").get<bool>())box(x,y,static_cast<unsigned char>(ch[0])<128?8:16,16,event.at("background"));
    const auto& g=font.at("glyphs").at(std::to_string(codepoints(ch).at(0)));float cell=font.at("cell");
    SDL_FRect from{g.at("x"),g.at("y"),cell,cell},to{px(x),py(y),cell*scale,cell*scale};text_glyph(atlas,from,to);
   }
  }else if(kind=="bar_background")box(a.at(0),a.at(1),a.at(2),a.at(3),a.at(5));
  else if(kind=="pattern")pattern=a.at(2);
  else if(kind=="bar_fill"){
   if(pattern!=128&&pattern!=64)throw std::runtime_error("unsupported hover bar pattern");
   int end=a.at(2);for(int x=a.at(0);x<=end;x+=8){float w=std::min(8,end-x+1);SDL_FRect from{0,0,w,8};
    image(std::string("images/Mark/")+(pattern==128?"hp-pattern.bmp":"mp-pattern.bmp"),{px(x),py(a.at(1)),w*scale,8*scale},&from);}
  }else if(kind=="icon"){
   int offset=a.at(4);if(offset!=384&&offset!=960)throw std::runtime_error("unsupported hover icon");
   image(std::string("images/Mark/")+(offset==384?"hp.bmp":"mp.bmp"),{px(a.at(0)),py(a.at(1)),24*scale,24*scale});
  }else throw std::runtime_error("unsupported hover panel instruction");
 }
 if(battle_language.english()){
  const auto& u=visible_state().at("units").at(std::to_string(id));auto r=UnitRecord::from_json(u.at("record"));auto b=BattleUnit::from_json(u.at("battle"));
  int terrain=game->terrain().at("cells").at(b.y*game->map_width()+b.x);
  detail_text(u.at("name"),rect.x+10,rect.y+4,rect.x+135,white,1,1.2f);
  detail_text(tr(ui.at("classes").at(r.job).at("text"))+" L"+std::to_string(r.level),rect.x+140,rect.y+4,rect.x+rect.w-6,white,1,1.1f);
  auto left=b.side==0?"EXP "+std::to_string(r.xp):b.side==1?"Ally":"Enemy";
  detail_text(left,rect.x+14,rect.y+114,rect.x+118,white,1,1.2f);
  detail_text(tr(ui.at("terrain").at(terrain).at("text"))+" "+std::to_string(battle_terrain_factor(r,terrain,game->data())*10)+"%",rect.x+130,rect.y+114,rect.x+rect.w-6,white,1,1.2f);
 }
 SDL_SetRenderClipRect(renderer,clipped?&old_clip:nullptr);
}
void draw_inventory(){
 if(treasure_view){draw_treasures();return;}
 fill(0,map_top,width,map_bottom-map_top,{0,0,0,130});bevel({220,336,520,486});ribbon(230,346,380,"本战道具");bevel(inventory_close);text("关闭 Esc",632,356,718);
 int row=0;for(int id:item_ids){float y=402+row*86;
  image("terrain-benefits/images/"+std::to_string(id)+".bmp",{242,y,48,48});
  text(game->data().at("items").at(id).at("name"),310,y+2,510);
  text("× "+game->inventory().at(std::to_string(id)).dump(),534,y+2,616);
  text(id==87?"恢复兵力 30":id==88?"恢复兵力 80":"解除中毒",310,y+29,610);
  auto rect=item_use_rect(row);bevel(rect);text("使用 "+std::to_string(row+1),rect.x+12,rect.y+10,rect.x+84,can_use_item(row)?ink:SDL_Color{140,140,140,255});++row;
 }
 int selected=game->state().at("selected");
 std::string actor=actionable_unit()?"使用者："+game->state().at("units").at(std::to_string(selected)).at("name").get<std::string>():"使用前，请选择尚未行动的我军。";
 text(actor,244,671,716);
 text("点击使用后，再点击自己或周围八格友军。\n满兵力或未中毒也会消耗；使用结束本次行动。",244,707,716,ink,2);
 text("物资随存档保存；地点只能领取一次。",244,776,716);
 bevel({244,820,472,36});text("查看宝物与装备 G",392,830,704);
}
void draw_info(){
 fill(0,map_top,width,map_bottom-map_top,{0,0,0,115});
 bevel({220,310,520,444});ribbon(228,318,396,"战场情报");bevel(info_close);text("关闭 Esc",642,328,724);
 auto id=inspected_unit();const auto& u=visible_state().at("units").at(std::to_string(id));
 auto b=BattleUnit::from_json(u.at("battle"));auto r=UnitRecord::from_json(u.at("record"));
 int face=game->data().at("heroes").at(id).at("face");image("images/Face/"+number3(face)+".bmp",{240,365,64,80});
 text(u.at("name"),324,368,704);text("兵力 "+std::to_string(b.hp)+" / "+u.at("max_hp").dump(),324,396,704);
 #ifndef __EMSCRIPTEN__
 text(jev_status,240,692,716);text("J: toggle JEV / Local AI",240,716,716);
#endif
 text("等级 "+std::to_string(r.level)+"   "+ui.at("classes").at(r.job).at("text").get<std::string>(),324,424,704);
 text("胜利："+game->objective()+"\n失败："+(special_hulao?(game->duel_team_survival()?"我军全体武将":game->state().at("units").at(std::to_string(game->duel_hero())).at("name").get<std::string>()):"曹操")+"被击败"+(game->legacy()?"":"，或超过20回合"),240,466,716,ink,2);
 text("点击我军选择，点击空地移动，点击敌军攻击。\n右键 / Esc 撤回移动；W 待命；E 结束回合。\nF5 保存；F9 读取；R 重新开始；P 背包。",240,528,716,ink,3);
 text(game->revised_rules()?"反击、连击、经验、伏兵与剧情事件已开放。\nF6 管理十个存档；胜利后下一战继承成长。":game->scenario()?"独立战场：固定阵容，无单挑和伏兵事件。\n伏兵开场显示；所有友军可操控；B 选择战场。":game->legacy()?"旧版存档：固定等级，无回合限制。":"战斗体验版：5种策略、地形恢复、道具收集。\n回合限制20；混乱敌军停止行动一次。",240,628,716,ink,2);
}
std::string status_message(){
 const auto& state=visible_state();
 if(showing_dialogue())return "对话中：点击对话框，或按空格 / 回车继续。";
 if(action_menu_unit>=0){if(!notice.empty())return notice;for(int row=0;row<5;++row)if(inside(mouse_x,mouse_y,action_row(row)))return action_hint(row);return "选择行动；方向键选择，回车确认，右键取消。";}
 if(attack_targeting)return notice.empty()?"请选择红色目标，或右键返回。":notice;
 if(playback.active()){
  if(playback.packet().value("kind",std::string())=="environment")return playback.notice().is_null()?"地形恢复与道具收集。":playback.notice().at("text").get<std::string>();
  auto actor=state.at("units").at(std::to_string(attack_actor)).at("name").get<std::string>();
  auto side=state.at("phase")=="enemy"?"敌军 ":"我军 ";
  if(playback.moving())return side+actor+" 移动中……";
  if(playback.packet().value("kind",std::string())=="tactic")return side+actor+" 使用"+playback.packet().at("name").get<std::string>()+"。";
  return side+actor+" 攻击 "+state.at("units").at(std::to_string(attack_target)).at("name").get<std::string>()+"。";
 }
 // Hovering End Turn must not hide the enemy's action or confusion explanation.
 if(state.at("phase")=="enemy"){
#ifndef __EMSCRIPTEN__
  return (game->balanced()?jev_status:std::string("Local AI (legacy save)"))+" | 敌军 "+state.at("message").get<std::string>();
#else
  return "敌军 "+state.at("message").get<std::string>();
#endif
 }
 for(int i=0;i<8;++i)if(inside(mouse_x,mouse_y,tool_rect(i))&&!show_info)return toolbar[i].hint;
 auto cell=map_cell(mouse_x,mouse_y);if(game->terrain_benefits_enabled()&&cell[0]>=0&&notice.empty()){
  int terrain=game->terrain().at("cells").at(cell[1]*game->map_width()+cell[0]),rate=terrain_recovery_percent(terrain);
  if(rate)return ui.at("terrain").at(terrain).at("text").get<std::string>()+"：回合开始恢复最大兵力的 "+std::to_string(rate)+"%。";
 }
 return notice.empty()?state.at("message").get<std::string>():notice;
}
void draw_dialogue(){
 auto page=game->dialogue();bevel({36,768,888,264});
 ribbon(44,776,872,page.at("speaker").get<std::string>().empty()?"战场消息":page.at("speaker").get<std::string>());
 int face=page.at("face");if(face>=0)image("images/Face/"+number3(face)+".bmp",{58,827,64,80});
 text(page.at("text"),148,829,898,ink,battle_language.english()?6:3);
 text("保存 F5 / 读取 F9",60,983,500);
 bevel({658,974,242,42});text("继续  空格 / 回车",675,984,890);
 text(std::to_string(page.at("page").get<int>()+1)+" / "+page.at("pages").dump(),552,984,650);
}
#include "prototype_voxel.inc"
#include "prototype_special_choice.inc"
void draw(const std::filesystem::path& capture={}){
 if(action_menu_unit>=0&&(!action_menu_ready()||game->state().at("selected")!=action_menu_unit))close_action_menu();
 const auto& s=visible_state();std::string phase=s.at("phase");
 fill(0,0,width,height,paper);
 for(int i=0;i<8;++i){
  auto rect=tool_rect(i);bool over=inside(mouse_x,mouse_y,rect)&&!show_info;
  bevel(rect,over&&enabled(i));
  SDL_FRect icon{float(toolbar[i].icon*24),0,24,24},to{rect.x+8,rect.y+8,24,24};
  auto* t=texture("images/pe/Ekd5.exe-302-2052.bmp");SDL_SetTextureColorMod(t,enabled(i)?255:150,enabled(i)?255:150,enabled(i)?255:150);
  SDL_RenderTexture(renderer,t,&icon,&to);
  text(toolbar[i].label,rect.x+38,rect.y+11,rect.x+rect.w-4,enabled(i)?ink:SDL_Color{110,110,110,255});
 }
 ribbon(8,54,204,game->name());ribbon(220,54,152,"回合 "+s.at("turn").dump()+(game->legacy()?"":"/20"));
 ribbon(380,54,168,playback.active()&&playback.packet().value("kind",std::string())=="environment"?"地形与物资":phase=="player"?(playback.active()?(playback.moving()?"我军移动":"我军攻击"):"我军行动"):phase=="enemy"?(playback.active()?(playback.moving()?"敌军移动":"敌军攻击"):"敌军行动"):phase=="ally"?"友军行动":phase=="victory"?"胜利":"战败");
 int allies=0,enemies=0;for(const auto& u:s.at("units"))if(u.at("battle").at("hp").get<int>()>0&&u.at("battle").at("lifecycle")==2)(u.at("battle").at("side")!=2?allies:enemies)++;
 text(std::to_string(allies)+" / "+std::to_string(enemies),574,60,650);bevel(treasure_button);text("宝物 G",684,60,780);bevel(battle_button);text(special_hulao?(web_hosted?"存档 B":"情报 B"):web_hosted?"返回主页 B":"选择战场 B",811,60,947);
 if(special_hulao)draw_voxel_map(s);else{
 image(game->battlefield_image(),{0,map_top,game->map_width()*map_tile(),game->map_height()*map_tile()});
 int idle_tick=int(SDL_GetTicks()/180)%8;
 for(int p=0;p<game->cell_count();++p)if(game->terrain().at("cells").at(p)==26){
  SDL_FRect from{0,float((idle_tick%4)*48),48,48};image("images/Mark/fire.bmp",tile_rect(p%game->map_width(),p/game->map_width()),&from);
 }
 int selected=s.at("selected");auto range=playback.active()||action_menu_unit>=0||attack_targeting?std::vector<int>(game->cell_count(),255):game->movement();
 for(int p=0;p<game->cell_count();++p)if(range[p]!=255){auto r=tile_rect(p%game->map_width(),p/game->map_width());fill(r.x,r.y,r.w,r.h,{40,108,230,70});}
 int hovered=hovered_unit();
 for(const auto& u:s.at("units")){
  auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0||b.lifecycle!=2||playback.poses().contains(std::to_string(b.slot)))continue;auto r=UnitRecord::from_json(u.at("record"));
  int sprite=battle_sprite(b.id,r.job,b.side),frame=battle_frame(b,u.at("max_hp"),idle_tick);
  SDL_FRect from{0,float(frame*48),48,48},to=tile_rect(b.x,b.y);
  auto* t=texture("images/Unit_mov/"+number3(sprite)+".bmp");Uint8 shade=(b.flags&2)?150:255;SDL_SetTextureColorMod(t,shade,shade,shade);
  SDL_RenderTextureRotated(renderer,t,&from,&to,0,nullptr,b.direction==1?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);
  if(b.status&8)image("images/Mark/confusion.bmp",{to.x+(4+24*(idle_tick&1))*map_scale(),to.y,16*map_scale(),16*map_scale()});
  if(b.id==selected){
   SDL_SetRenderDrawColor(renderer,255,218,84,255);SDL_RenderRect(renderer,&to);
   SDL_FRect inner{to.x+1,to.y+1,to.w-2,to.h-2};SDL_RenderRect(renderer,&inner);
  }
  if(selected>=0&&target_tactic<0&&target_item<0&&game->attackable(selected,b.id))image("images/Mark/range.bmp",to);
  if(target_item>=0&&selected>=0){for(const auto& o:game->item_options(selected,target_item))if(o.at("target")==b.id){SDL_SetRenderDrawColor(renderer,70,255,120,255);SDL_RenderRect(renderer,&to);auto inset=to;inset.x+=2;inset.y+=2;inset.w-=4;inset.h-=4;SDL_RenderRect(renderer,&inset);}}
  if(target_tactic>=0&&selected>=0){const auto& a=s.at("units").at(std::to_string(selected)).at("battle");for(const auto& o:game->spell_options(selected,{a.at("x"),a.at("y")}))if(o.at("tactic")==target_tactic&&o.at("target")==b.id)image("images/Mark/range.bmp",to);}
  if(b.status&16)text("毒",to.x+to.w-18,to.y,to.x+to.w,{84,220,90,255});
  // Keep the untouched battlefield readable, as in the reference screenshot.
  // Team/HP information remains available on hover, selection, and H.
  if(show_health||b.id==selected||b.id==hovered){
   SDL_Color c=b.side==2?SDL_Color{217,67,48,255}:SDL_Color{46,153,224,255};
   fill(to.x+2*map_scale(),to.y+43*map_scale(),44*map_scale(),4*map_scale(),{20,20,20,255});fill(to.x+2*map_scale(),to.y+43*map_scale(),44.f*map_scale()*b.hp/u.at("max_hp").get<int>(),4*map_scale(),c);
  }
 }
 if(playback.active()){
  // Mark the actual participants even when the action is away from the mouse.
  const int presented_actor=playback.notice().is_null()?attack_actor:playback.notice().at("actor").get<int>();
  for(int id:{presented_actor,playback.notice().is_null()?attack_target:playback.notice().value("target",-1)}){
   if(id<0)continue;
   const auto& b=s.at("units").at(std::to_string(id)).at("battle");auto r=tile_rect(b.at("x"),b.at("y"));
   if(playback.moving()&&playback.poses().contains(std::to_string(b.at("slot").get<int>()))){const auto& pose=playback.poses().at(std::to_string(b.at("slot").get<int>()));r.x=pose.at("x").get<float>()*map_scale();r.y=(pose.at("y").get<float>()-56)*map_scale()+map_top;}
   auto c=id==presented_actor?SDL_Color{255,217,64,255}:SDL_Color{255,104,80,255};
   SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderRect(renderer,&r);
   SDL_FRect inner{r.x+1,r.y+1,r.w-2,r.h-2};SDL_RenderRect(renderer,&inner);
  }
  std::vector<Json> poses;for(const auto& pose:playback.poses())poses.push_back(pose);
  std::sort(poses.begin(),poses.end(),[](const auto& a,const auto& b){return a.at("order")<b.at("order");});
  for(const auto& pose:poses)draw_unit_pose(pose);
  if(!playback.spell_effect().is_null()){
   const auto& e=playback.spell_effect();auto* effect=texture(e.value("asset_group",std::string("tactics"))+"/images/"+e.at("resource").dump()+"-"+e.at("image").dump()+".bmp");
   SDL_SetTextureBlendMode(effect,SDL_BLENDMODE_BLEND);SDL_SetTextureAlphaMod(effect,Uint8(e.at("amount").get<int>()*255/16));
   SDL_FRect dst{e.at("x").get<float>()*map_scale(),(e.at("y").get<float>()-56)*map_scale()+map_top,e.at("width").get<float>()*map_scale(),e.at("height").get<float>()*map_scale()};SDL_RenderTexture(renderer,effect,nullptr,&dst);
  }
  if(playback.packet().value("kind",std::string())=="tactic"){
   bevel({370,map_top+10,220,40});text(playback.packet().at("name").get<std::string>()+"  MP -"+playback.packet().at("mp_cost").dump(),386,map_top+20,580);
  }
  if(!playback.notice().is_null()){const auto& n=playback.notice();bevel({196,map_top+12,568,76});
   float x=212;if(n.contains("item")){
    int id=n.at("item");auto path="terrain-benefits/images/"+std::to_string(id)+".bmp";
    // Location pickups contain only beans/rice/antidote. Script treasures use
    // the complete original item atlas already bundled for equipment details.
    if(!std::filesystem::exists(assets/path))path="unit-details/images/Item/"+number3(id)+".bmp";
    image(path,{208,map_top+25,44,44});x=264;
   }text(n.at("text"),x,map_top+25,746,ink,2);}
  const auto& number=playback.number();
  if(!number.is_null())for(const auto& u:s.at("units"))if(u.at("battle").at("slot")==number.at("slot")){
   auto b=BattleUnit::from_json(u.at("battle"));float x=b.x*map_tile()+10*map_scale(),y=map_top+b.y*map_tile()-14;
   auto label=(number.value("healing",false)?"+":"")+number.at("value").dump();
   text(label,x+1,y+1,x+70,{12,12,18,255});text(label,x,y,x+70,number.value("healing",false)?SDL_Color{85,255,130,255}:SDL_Color{255,238,70,255});
  }
 }
 auto cell=map_cell(mouse_x,mouse_y);if(cell[0]>=0&&!show_info&&!playback.active()&&action_menu_unit<0)image("images/Mark/cursor.bmp",tile_rect(cell[0],cell[1]));
 if(action_menu_unit>=0){const auto& selected_unit=s.at("units").at(std::to_string(action_menu_unit)).at("battle");image("images/Mark/cursor.bmp",tile_rect(selected_unit.at("x"),selected_unit.at("y")));}
 }
 auto cell=map_cell(mouse_x,mouse_y);
 bevel({0,float(map_bottom),float(width),float(height-map_bottom)});
 const auto& u=s.at("units").at(std::to_string(inspected_unit()));auto b=BattleUnit::from_json(u.at("battle"));auto r=UnitRecord::from_json(u.at("record"));
 int face=game->data().at("heroes").at(b.id).at("face");image("images/Face/"+number3(face)+".bmp",{8,map_bottom+5,48,60});
 text(u.at("name"),66,map_bottom+5,188);text(b.side==2?"敌军":b.side==1?"友军":"我军",190,map_bottom+5,238);
 text(ui.at("classes").at(r.job).at("text"),250,map_bottom+5,346);
 text("兵力 "+std::to_string(b.hp)+" / "+u.at("max_hp").dump(),354,map_bottom+5,554);
 text("攻击 "+std::to_string(battle_stat(b,r,0,game->data()))+"   防御 "+std::to_string(battle_stat(b,r,1,game->data())),562,map_bottom+5,802);
 int terrain=game->terrain().at("cells").at((cell[0]>=0?cell[1]*game->map_width()+cell[0]:b.y*game->map_width()+b.x));
 text(ui.at("terrain").at(terrain).at("text"),840,map_bottom+5,954);
 text(status_message(),66,map_bottom+31,628,ink,2);
 if(game->terrain_benefits_enabled()){bevel(inventory_button);text("查看道具 P",inventory_button.x+16,inventory_button.y+6,inventory_button.x+136);}
 visible_hover_panel=hover_panel_unit(SDL_GetTicks());if(visible_hover_panel>=0)draw_hover_panel(visible_hover_panel);
 if(action_menu_unit>=0&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles)draw_action_menu();
 if(show_spells){
  auto list=game->spells(s.at("selected"));bevel({290,280,380,120.f+float(list.size())*40});ribbon(300,292,360,"选择策略");
  for(size_t n=0;n<list.size();++n){auto row=SDL_FRect{310,340.f+float(n)*40,340,36};bevel(row);text(std::to_string(n+1)+". "+list.at(n).at("name").get<std::string>()+"   MP "+list.at(n).at("mp_cost").dump(),324,row.y+10,640);}
  bevel({310,348.f+float(list.size())*40,340,40});text("返回 Esc",414,358.f+float(list.size())*40,640);
 }
 if((phase=="victory"||phase=="defeat")&&!showing_dialogue()){
  bevel({236,440,488,132});ribbon(244,448,472,phase=="victory"?"胜利！"+game->name()+"结束。":s.value("turn_limit_reached",false)?"战败：超过回合限制。":"战败："+(special_hulao?(game->duel_team_survival()?"我军全体武将":game->state().at("units").at(std::to_string(game->duel_hero())).at("name").get<std::string>()):"曹操")+"已被击败。");
  if(phase=="victory"){bevel(camp_button);text("整备装备",292,520,416);bevel(next_button);text(special_hulao?"重新开始":game->scenario()+1<game->scenario_count()?"下一战":web_hosted?"返回主页":"选择战场",464,520,684);}
  else text("点击重新开始，或读取存档。",292,516,704);
 }
 bevel(unit_button);text("武将 V",unit_button.x+44,unit_button.y+8,unit_button.x+unit_button.w-8);
 if(show_info)draw_info();
 if(showing_dialogue())draw_dialogue();
 if(manual_slots){bevel(slots_button);text("存档位 F6",slots_button.x+20,slots_button.y+8,slots_button.x+154);}
 if(show_inventory)draw_inventory();
 if(!playback.active()&&!game->story_choices().empty()){auto options=game->story_choices().at("options");bevel({184,390,592,80.f+float(options.size())*56});ribbon(194,400,572,"选择行动");for(size_t n=0;n<options.size();++n){auto row=story_row(int(n));bevel(row);text(std::to_string(n+1)+". "+options.at(n).get<std::string>(),row.x+12,row.y+12,row.x+row.w-12);}}
 if(show_battles){
  fill(0,map_top,width,960,{0,0,0,135});bevel({210,294,540,470});ribbon(220,304,520,"选择独立战场");
  for(int id=0;id<game->scenario_count();++id){auto r=battle_choice(id);bevel(r);text(std::to_string(id+1)+". "+game->scenario_name(id),r.x+20,r.y+17,r.x+r.w-20);}
  bevel({228,644,504,40});text("返回当前战场 Esc",352,654,720);
  text(game->revised_rules()?"选择独立战场会创建新队伍。\n继承成长和物资，请在胜利后点击下一战。":"点击开始新战斗；各战场分别保存，F9 读取。\n固定阵容；伏兵开场显示；无单挑和战役成长。",228,699,731,ink,2);
 }
 if(show_unit_details)draw_unit_details();
 if(special_choosing)draw_special_choice();
 if(show_slots)draw_slots();
 // Read the completed backbuffer before presenting it. Metal readback after
 // SDL_RenderPresent can return the preceding frame, mislabelling evidence.
 if(!capture.empty()){
  auto* pixels=SDL_RenderReadPixels(renderer,nullptr);
  if(!pixels)throw std::runtime_error(SDL_GetError());
  bool saved=SDL_SaveBMP(pixels,capture.string().c_str());SDL_DestroySurface(pixels);
  if(!saved)throw std::runtime_error(SDL_GetError());
 }
 if(!SDL_RenderPresent(renderer))log_presentation({{"event","present_error"},{"error",SDL_GetError()}});
 else if(playback.active()&&!show_info)for(const auto& pose:playback.poses()){
  auto key=pose.dump();if(presented_poses.insert(key).second){
   if(!pose.at("effects").empty())presented_flash=true;
   log_presentation({{"event",playback.moving()?"movement_presented":"attack_presented"},{"actor",attack_actor},{"target",attack_target},{"pose",pose}});
  }
 }
 ++frames;
}
void poll_enemy_decision(){
#ifndef __EMSCRIPTEN__
 if(!jev||!jev->busy())return;
 auto result=jev->poll();if(result.is_null())return;
 if(result.value("cancelled",false)){log_presentation({{"event","enemy_decision_cancelled"}});return;}
 auto request=game->enemy_request();if(request.is_null()||request.at("key")!=result.at("key")){jev_status="JEV: stale response discarded";log_presentation({{"event","enemy_decision_discarded"},{"reason","stale_state"}});return;}
 game->choose_enemy(result.at("key"),result.at("choice"));last_jev=result;
 jev_status=result.at("source").get<std::string>()+" "+result.at("choice").get<std::string>()+(result.at("source")=="fallback"?" (request failed; local AI)":" (J: toggle)");
 Json decision=Json::object();for(const auto* key:{"key","choice","source","cancelled","elapsed_ms","cache_hit"})if(result.contains(key))decision[key]=result.at(key);
 diagnostic_dirty=true;log_presentation({{"event","enemy_decision"},{"decision",decision}});next_enemy=SDL_GetTicks()+450;
#endif
}
bool advance_enemy(){
#ifndef __EMSCRIPTEN__
 if(jev&&jev->busy())return false;
 if(jev_enabled&&jev&&game->balanced()&&!game->has_enemy_choice()){
  auto request=game->enemy_request();if(!request.is_null()){
   diagnostic_checkpoint("before_jev");log_presentation({{"event","enemy_decision_requested"},{"key",request.at("key")}});
   jev->start(request,save_path().parent_path()/"jev");jev_status="JEV: thinking... (J: local AI)";return false;
  }
 }
#else
 if(web_hosted&&web_ai){
  if(web_ai_pending)return false;
  if(!game->has_enemy_choice()){
   auto request=game->enemy_request();if(!request.is_null()){
    web_ai_pending=true;web_ai_key=request.at("key");auto raw=request.dump();
    EM_ASM({window.prototypeEnemy(UTF8ToString($0));},raw.c_str());return false;
   }
  }
 }
#endif
 diagnostic_action("npc_step",{{"cursor",game->state().at("enemy_cursor")}});
 game->enemy_step();log_presentation({{"event","npc_step_complete"},{"cursor",game->state().at("enemy_cursor")},{"state_hash",game->hash()},{"rng",game->state().at("rng")},{"script",game->revised_rules()?game->progress():Json(nullptr)}});return true;
}
void tick(){
 diagnostic_stage="input";auto tick_start=SDL_GetTicks();
 SDL_Event e;
 if(web_hosted&&web_paused){while(SDL_PollEvent(&e)){}animation_clock=SDL_GetTicks();draw();return;}
 while(SDL_PollEvent(&e)){
  if(e.type==SDL_EVENT_QUIT)running=false;
  else if(e.type==SDL_EVENT_MOUSE_MOTION){touch_hover_input=false;touch_hover_unit=-1;SDL_RenderCoordinatesFromWindow(renderer,e.motion.x,e.motion.y,&mouse_x,&mouse_y);}
  else if(e.type==SDL_EVENT_WINDOW_MOUSE_LEAVE){mouse_x=-1;mouse_y=-1;}
  else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
   touch_hover_input=false;touch_hover_unit=-1;
   float x,y;SDL_RenderCoordinatesFromWindow(renderer,e.button.x,e.button.y,&x,&y);
   if(e.button.button==SDL_BUTTON_LEFT||e.button.button==SDL_BUTTON_RIGHT)click(x,y,e.button.button==SDL_BUTTON_RIGHT,e.button.button==SDL_BUTTON_LEFT&&(SDL_GetModState()&SDL_KMOD_CTRL));
  }else if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){
   if(special_choosing&&!show_slots){
    if(e.key.key>=SDLK_1&&e.key.key<=SDLK_4)select_special_hero(int(e.key.key-SDLK_1));
    else if(e.key.key==SDLK_F9)persist(true);else if(e.key.key==SDLK_F6)open_slots();
    else if(e.key.key==SDLK_ESCAPE&&special_has_match)special_choosing=false;
    continue;
   }
   if(!playback.active()&&!game->story_choices().empty()){int n=int(e.key.key-SDLK_1);if(n>=0&&n<int(game->story_choices().at("options").size()))select_story(n);continue;}
   if(backup_busy)continue;
   if(manual_slots&&e.key.key==SDLK_F6){open_slots();continue;}
   if(show_slots){if(e.key.key==SDLK_ESCAPE)show_slots=false;else if(e.key.key>=SDLK_1&&e.key.key<=SDLK_9)save_slot=int(e.key.key-SDLK_1)+1;else if(e.key.key==SDLK_0)save_slot=10;else if(e.key.key==SDLK_F5)persist(false);else if(e.key.key==SDLK_F9)persist(true);continue;}
   if(show_unit_details){detail_key(e.key.key,(e.key.mod&SDL_KMOD_SHIFT)!=0);continue;}
   if(e.key.key==SDLK_V){open_unit_details(inspected_unit());continue;}
   #ifndef __EMSCRIPTEN__
   if(e.key.key==SDLK_J&&!show_inventory){jev_enabled=!jev_enabled&&jev&&jev->configured();cancel_jev();if(!jev_enabled)game->clear_enemy_choice();diagnostic_dirty=true;log_presentation({{"event","ai_mode"},{"jev_enabled",jev_enabled}});jev_status=jev_enabled?"JEV enabled (J: local AI)":jev&&jev->configured()?"Local AI (J: JEV)":"Local AI (JEV unavailable; restart after configuring)";continue;}
#endif
   if(e.key.key==SDLK_G){open_treasures();continue;}
   if(e.key.key==SDLK_P&&show_inventory&&treasure_view){treasure_view=false;continue;}
   if(e.key.key==SDLK_P&&game->terrain_benefits_enabled()){close_action_menu();show_inventory=!show_inventory;treasure_view=false;show_info=false;show_battles=false;continue;}
   if(show_inventory){if(e.key.key==SDLK_ESCAPE)show_inventory=false;else if(!treasure_view)choose_item(int(e.key.key-SDLK_1));else if(e.key.key==SDLK_PAGEDOWN)treasure_page=std::min(treasure_page+1,std::max(0,int(owned_equipment().size())-1)/8);else if(e.key.key==SDLK_PAGEUP)treasure_page=std::max(0,treasure_page-1);continue;}
   if(e.key.key==SDLK_B&&!playback.active()){
#ifdef __EMSCRIPTEN__
    if(web_hosted){EM_ASM({window.prototypeHome?.();});continue;}
#endif
    close_action_menu();if(special_hulao)show_info=!show_info;else show_battles=!show_battles;continue;}
   if(show_battles){if(e.key.key==SDLK_ESCAPE)show_battles=false;else if(e.key.key>=SDLK_1&&e.key.key<SDLK_1+game->scenario_count())choose_battle(int(e.key.key-SDLK_1));continue;}
   if(showing_dialogue()){
    if(e.key.key==SDLK_SPACE||e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER)send("continue");
    else if(e.key.key==SDLK_F5)persist(false);else if(e.key.key==SDLK_F9)persist(true);else if(e.key.key==SDLK_R)button(5);
    continue;
   }
   if(e.key.key==SDLK_I){close_action_menu();show_info=!show_info;continue;}
   if(show_info){if(e.key.key==SDLK_ESCAPE)show_info=false;continue;}
   if(e.key.key==SDLK_T&&actionable_unit()&&!game->spells(game->state().at("selected")).empty()){close_action_menu();show_spells=true;continue;}
   if(!playback.active()&&action_key(e.key.key,(e.key.mod&SDL_KMOD_SHIFT)!=0))continue;
   switch(e.key.key){case SDLK_W:button(0);break;case SDLK_ESCAPE:button(1);break;case SDLK_E:button(2);break;
    case SDLK_F5:button(3);break;case SDLK_F9:button(4);break;case SDLK_R:button(5);break;case SDLK_H:show_health=!show_health;break;}
  }
 }
 #ifndef __EMSCRIPTEN__
 backup_finish();
#endif
 if(special_choosing){animation_clock=SDL_GetTicks();draw();return;}
 diagnostic_stage="simulation";
 if(!show_unit_details&&!show_inventory&&!show_slots&&!backup_busy)poll_enemy_decision();
 uint64_t now=SDL_GetTicks();bool was_animating=playback.active();
 if(was_animating&&!show_slots&&!backup_busy&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles){
  // Never catch up past an original 40ms presentation boundary before drawing.
  // A slow frame used to skip both brief hit-flash frames (ticks13/14).
  const auto generation=playback.generation();
  const bool was_moving=playback.moving();
  const int moving_unit=was_moving?playback.packet().at("actor").get<int>():-1;
  playback.advance(uint32_t(std::min<uint64_t>(now-animation_clock,PrototypePlayback::tick_ms)));
  for(int sound:playback.take_sounds())attack_sound(sound);
  if(!playback.active()||generation!=playback.generation()){
   log_presentation({{"event",was_moving?"movement_end":"attack_end"},{"poses_presented",presented_poses.size()},{"flash_presented",presented_flash}});
   if(playback.active())begin_presentation();else {next_enemy=now+450;notice.clear();if(was_moving&&moved_unit()&&game->state().at("selected")==moving_unit)menu_after_move_unit=moving_unit;}
  }
 }
 animation_clock=now;
 if(menu_after_move_unit>=0&&!playback.active()&&!game->has_dialogue()&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles){
  int completed_unit=menu_after_move_unit;menu_after_move_unit=-1;
  if(game->state().at("selected")==completed_unit)open_action_menu();
 }
 // Auto-end is a hosted-game convenience. A tentative move, open menu, story
 // page or animation must finish first; use the same command as the E button.
 if(web_hosted&&!was_animating&&!playback.active()&&!show_slots&&!backup_busy&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles&&!show_spells&&target_tactic<0&&target_item<0&&action_menu_unit<0&&menu_after_move_unit<0&&game->player_turn_complete())send("end_turn");
 if(!was_animating&&!show_slots&&!backup_busy&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles&&!game->has_dialogue()&&game->story_choices().empty()&&(game->state().at("phase")=="enemy"||game->state().at("phase")=="ally")&&now>=next_enemy){if(advance_enemy()){collect_presentation();if(!playback.active())log_presentation({{"event","enemy_action"},{"turn",game->state().at("turn")},{"cursor",game->state().at("enemy_cursor")},{"message",game->state().at("message")}});next_enemy=now+450;}}
 diagnostic_stage="render";draw();
 diagnostic_stage="checkpoint";diagnostic_checkpoint("stable_interaction");
 if(now-diagnostic_heartbeat>=10000){diagnostic_heartbeat=now;log_presentation({{"event","heartbeat"},{"playback_active",playback.active()},{"dialogue",game->has_dialogue()},{"backup_busy",backup_busy},{"state_hash",game->hash()}});}
 auto elapsed=SDL_GetTicks()-tick_start;if(elapsed>250)log_presentation({{"event","slow_frame"},{"elapsed_ms",elapsed}});
 diagnostic_stage="idle";
}
#ifndef __EMSCRIPTEN__
#include "prototype_unit_smoke.inc"
#include "prototype_equipment_smoke.inc"
#include "prototype_revised_smoke.inc"
void items_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(std::string(why)+": "+notice);};
 auto complete=[&](){for(int n=0;playback.active()&&n<2000;++n){animation_clock=SDL_GetTicks()-40;tick();}check(!playback.active(),"item playback stalled");};
 auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;SDL_PushEvent(&e);tick();};
 auto tap=[&](SDL_FRect r,bool right=false){float x,y;SDL_RenderCoordinatesToWindow(renderer,r.x+r.w/2,r.y+r.h/2,&x,&y);SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=right?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_PushEvent(&e);tick();};
 choose_battle(0);while(game->has_dialogue())game->command("continue");terrain_fixture_place(*game,0,13,12,10);check(send("tile",13,12)&&send("wait"),"pickup fixture failed");complete();while(game->has_dialogue())game->command("continue");
 terrain_fixture_next_player(*game);game->enemy_step();collect_presentation();complete();while(game->has_dialogue())game->command("continue");
 terrain_fixture_place(*game,0,13,12,10);check(send("tile",13,12),"select CaoCao failed");check(action_menu_unit<0,"menu opened before movement");
 auto unchanged=game->serialize();tap(tool_rect(7));check(show_inventory&&can_use_item(0),"top item button/use disabled");draw(out/"items.bmp");key(SDLK_E);check(game->serialize()==unchanged,"inventory leaked battle command");
 tap(item_use_rect(0));check(target_item==87&&!show_inventory,"use button did not target");draw(out/"target.bmp");key(SDLK_ESCAPE);check(show_inventory&&game->serialize()==unchanged,"cancel targeting spent item/action");
 key(SDLK_1);tap(tile_rect(13,12));check(playback.active()&&playback.notice().at("item")==87,"self item animation missing");draw(out/"self-heal.bmp");
 check(game->state().at("units").at("0").at("battle").at("hp")==40&&game->inventory().at("87")==0,"CaoCao bean HP/stock wrong");
 key(SDLK_F5);check(!std::filesystem::exists(save_path()),"save allowed during item animation");complete();while(game->has_dialogue())game->command("continue");
 key(SDLK_F5);check(notice=="已保存。","item save failed");auto saved=game->serialize();key(SDLK_F9);check(notice=="已读取存档。"&&game->serialize()==saved,"item save/load changed state");
 tap(inventory_button);check(show_inventory&&!can_use_item(0),"footer inventory or exhausted stock wrong");draw(out/"spent.bmp");key(SDLK_ESCAPE);
 std::ofstream(out/"saved.json")<<saved;
 // Restore the earlier unspent checkpoint: moving still opens the menu only after animation.
 restore_snapshot(unchanged);check(send("tile",13,11),"item move fixture failed");check(playback.active()&&action_menu_unit<0,"item menu opened before walk");complete();tick();check(action_menu_unit==0&&action_enabled(2),"post-move item row disabled");
 tap(action_row(2));check(show_inventory,"action-menu item did not open inventory");key(SDLK_1);tap(tile_rect(13,11),true);check(show_inventory,"right-click target cancel failed");key(SDLK_ESCAPE);check(send("cancel")&&game->inventory().at("87")==1,"cancel move lost item");
 std::ofstream(out/"result.json")<<Json{{"toolbar_button",true},{"footer_button",true},{"self_heal",30},{"target_cancel",true},{"post_move_menu",true},{"save_reload",true},{"saved_hash",sha256(saved)}}.dump(2);
 std::cout<<"native item UI, self-use, target/move cancellation and save/reload passed "<<sha256(saved)<<'\n';
}
void terrain_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(std::string(why)+": "+notice);};
 auto complete=[&](){for(int n=0;playback.active()&&n<2000;++n){animation_clock=SDL_GetTicks()-40;tick();}check(!playback.active(),"terrain playback stalled");};
 auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;SDL_PushEvent(&e);tick();};
 choose_battle(0);while(game->has_dialogue())game->command("continue");terrain_fixture_place(*game,0,13,11,10);
 check(send("tile",13,11)&&send("tile",13,12),"terrain smoke walk failed");complete();
 check(game->inventory().at("87")==0,"walking granted item before commit");
 check(send("cancel"),"terrain smoke cancel failed");check(game->inventory().at("87")==0,"cancelling granted item");
 check(send("tile",13,11)&&send("tile",13,12),"terrain second walk failed");complete();check(send("wait"),"terrain wait failed");
 check(playback.active()&&!playback.notice().is_null()&&game->inventory().at("87")==1,"pickup feedback absent");draw(out/"pickup.bmp");
 auto slot=save_path();persist(false);check(notice=="动画中，请等待。"&&!std::filesystem::exists(slot),"saved during item animation");complete();while(game->has_dialogue())game->command("continue");
 key(SDLK_P);check(show_inventory,"inventory key failed");auto stable=game->hash();draw(out/"inventory.bmp");key(SDLK_E);check(game->hash()==stable,"inventory leaked key to battle");key(SDLK_ESCAPE);check(!show_inventory,"inventory close failed");
 persist(false);check(notice=="已保存。","terrain save failed");auto saved=game->hash();persist(true);check(notice=="已读取存档。"&&game->hash()==saved&&game->inventory().at("87")==1,"terrain load failed");
 terrain_fixture_next_player(*game);game->enemy_step();collect_presentation();check(playback.active()&&playback.number().value("healing",false),"terrain heal feedback absent");draw(out/"healing.bmp");complete();
 while(game->has_dialogue())game->command("continue");check(send("tile",13,12)&&send("wait"),"terrain revisit failed");check(!playback.active()&&game->inventory().at("87")==1,"revisit duplicated pickup");
 auto rect=inventory_button;click(rect.x+5,rect.y+5,false);check(show_inventory,"inventory mouse button failed");draw(out/"inventory-restored.bmp");click(inventory_close.x+5,inventory_close.y+5,false);
 persist(false);std::ofstream(out/"saved.json")<<game->serialize();
 std::ofstream(out/"result.json")<<Json{{"pickup",true},{"cancel",true},{"healing",true},{"inventory_ui",true},{"save_reload",true},{"repeat_prevention",true},{"hash",game->hash()},{"inventory",game->inventory()}}.dump(2);
 std::cout<<"terrain native presentation, cancel/commit, inventory, save/reload passed "<<game->hash()<<'\n';
}
void menu_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 auto mouse=[&](float x,float y,int button=SDL_BUTTON_LEFT){
  float wx,wy;SDL_RenderCoordinatesToWindow(renderer,x,y,&wx,&wy);SDL_Event e{};
  e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=button;e.button.x=wx;e.button.y=wy;SDL_PushEvent(&e);tick();
 };
 auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;SDL_PushEvent(&e);tick();};
 auto unit_click=[&](int id){const auto& b=game->state().at("units").at(std::to_string(id)).at("battle");auto r=tile_rect(b.at("x"),b.at("y"));mouse(r.x+r.w/2,r.y+r.h/2);};
 auto row_click=[&](int row){auto r=action_row(row);mouse(r.x+r.w/2,r.y+r.h/2);};
 auto finish_animation=[&](){
  // Exercise the real completion scheduler, including deliberately slow frames.
  for(int n=0;playback.active()&&n<2000;++n){
   check(action_menu_unit<0,"menu visible before animation finished");
   animation_clock=SDL_GetTicks()-120;tick();
   check(!playback.active()||action_menu_unit<0,"menu interrupted animation");
  }
  check(!playback.active(),"menu smoke animation stalled");tick();
 };
 auto check_selection=[&](int id){
  check(action_menu_unit<0&&game->state().at("undo").is_null(),"unmoved selection opened menu");
  auto selected=game->serialize();unit_click(id);key(SDLK_SPACE);
  check(action_menu_unit<0&&game->serialize()==selected,"repeat click or Space opened menu before moving");
 };
 auto move_selected=[&](){
  auto costs=game->movement();int destination=-1,best=255;
  for(int p=0;p<game->cell_count();++p)if(costs[p]>0&&costs[p]<best){
   bool occupied=false;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("hp").get<int>()>0&&b.at("lifecycle")==2&&b.at("x")==p%game->map_width()&&b.at("y")==p/game->map_width())occupied=true;}
   if(!occupied){destination=p;best=costs[p];}
  }
  check(destination>=0,"menu regression needs reachable empty tile");
  int id=game->state().at("selected");auto r=tile_rect(destination%game->map_width(),destination/game->map_width());mouse(r.x+r.w/2,r.y+r.h/2);
  check(playback.moving()&&action_menu_unit<0,"movement did not start before menu");
  // A rapid repeat click/Space cannot dismiss or shortcut a walk.
  unit_click(id);key(SDLK_SPACE);check(action_menu_unit<0,"rapid input opened menu during movement");
  finish_animation();check(action_menu_unit==id,"completed walk opened wrong unit menu");
 };
 // Regression: completing one unit's walk must not open the next unit's menu
 // merely because the presentation object retains the completed walk packet.
 choose_battle(0);unit_click(0);auto first_move=game->suggest(0);
 auto first_dest=tile_rect(first_move.at("x"),first_move.at("y"));mouse(first_dest.x+first_dest.w/2,first_dest.y+first_dest.h/2);
 finish_animation();check(action_menu_unit==0,"first move did not open menu");row_click(3);
 while(game->has_dialogue())game->command("continue");
 int next_unit=-1;for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("side")!=2&&b.at("hp").get<int>()>0&&!(b.at("flags").get<int>()&2)){next_unit=b.at("id");break;}}
 check(next_unit>=0,"regression needs a second friendly unit");unit_click(next_unit);
 check(action_menu_unit<0,"selecting next unit after a completed walk opened menu before movement");
 check_selection(next_unit);draw(out/"next-unit-before-moving.bmp");
 auto unmoved=game->serialize();restore_snapshot(unmoved);tick();check_selection(next_unit);
 move_selected();draw(out/"next-unit-after-moving.bmp");row_click(4);
 unit_click(next_unit);check_selection(next_unit);move_selected();row_click(3);
 while(game->has_dialogue())game->command("continue");
 for(const auto& u:game->state().at("units")){const auto& b=u.at("battle");if(b.at("side")!=2&&b.at("hp").get<int>()>0&&!(b.at("flags").get<int>()&2)){int id=b.at("id");unit_click(id);check_selection(id);break;}}
 Json verified=Json::array();
 for(int scenario=0;scenario<4;++scenario){
  choose_battle(scenario);while(game->has_dialogue())game->command("continue");
  for(auto cell:{std::array{0,0},std::array{game->map_width()-1,0},std::array{0,game->map_height()-1},std::array{game->map_width()-1,game->map_height()-1}}){
   auto r=action_menu_rect(cell[0],cell[1]),u=tile_rect(cell[0],cell[1]);
   check(r.x>=0&&r.x+r.w<=width&&r.y>=map_top&&r.y+r.h<=map_top+game->map_height()*map_tile(),"action menu clipped at edge");
   check(!SDL_HasRectIntersectionFloat(&r,&u),"action menu covers unit");
  }
  unit_click(0);check_selection(0);move_selected();auto selected=game->serialize();
  check(action_menu_unit==0&&!game->state().at("undo").is_null(),"menu opened without a move");
  check(!action_enabled(2)&&action_enabled(3)&&action_enabled(4),"menu availability wrong");
  draw(out/("battle-"+std::to_string(scenario)+"-menu.bmp"));
  for(int row=0;row<3;++row)if(!action_enabled(row)){row_click(row);check(game->serialize()==selected&&action_menu_unit==0,"disabled row dispatched an action");}
  check(hover_panel_unit(SDL_GetTicks()+1000)<0,"hover popup overlaps action menu");
  auto r=action_menu_rect();mouse(r.x+2,r.y+1);check(game->serialize()==selected,"menu border leaked map click");
  while(action_menu_focus!=3)key(SDLK_DOWN);check(action_menu_focus==3,"keyboard did not skip disabled commands");
  key(SDLK_DOWN);check(action_menu_focus==4,"keyboard did not focus cancel");key(SDLK_UP);check(action_menu_focus==3,"keyboard did not focus wait");
  row_click(4);check(game->state().at("selected")==-1&&action_menu_unit<0,"menu cancel failed");
  unit_click(0);check_selection(0);move_selected();while(action_menu_focus!=3)key(SDLK_DOWN);key(SDLK_RETURN);check(game->state().at("units").at("0").at("battle").at("flags")==2&&game->state().at("selected")==-1,"menu wait failed");
  verified.push_back({{"scenario",scenario},{"disabled_actions",true},{"wait_cancel_keyboard",true},{"edge_placement",true},{"only_after_movement",true}});
 }
 // An ordinary move, menu save/restore, rollback, and attack from a legal target.
 choose_battle(0);unit_click(0);const auto before=game->state().at("units").at("0").at("battle");
 auto plan=game->suggest(0);check(plan.at("target").get<int>()>=0,"opening test needs a reachable enemy");
 auto dest=tile_rect(plan.at("x"),plan.at("y"));mouse(dest.x+dest.w/2,dest.y+dest.h/2);
 check(playback.moving()&&action_menu_unit<0,"menu interrupted walking");finish_animation();
 check(action_menu_unit==0&&action_enabled(0),"menu absent after walking or attack disabled");draw(out/"attack-enabled.bmp");
 auto moved=game->serialize();key(SDLK_F5);check(notice=="已保存。","menu save failed");row_click(4);
 const auto& rolled=game->state().at("units").at("0").at("battle");for(const auto* field:{"x","y","direction"})check(rolled.at(field)==before.at(field),"menu cancellation did not restore movement");
 key(SDLK_F9);check(game->serialize()==moved&&action_menu_unit==0,"saved movement/menu continuation changed");
 row_click(0);check(attack_targeting&&action_menu_unit<0,"attack targeting not entered");auto stable=game->serialize();mouse(24,map_top+24);check(game->serialize()==stable,"invalid attack target changed state");
 key(SDLK_ESCAPE);check(action_menu_unit==0&&game->serialize()==stable,"attack cancel lost tentative move");
 for(auto size:{std::array{720,840},std::array{1200,960}}){
  SDL_SetWindowSize(window,size[0],size[1]);SDL_SyncWindow(window);tick();row_click(0);
  check(attack_targeting,"scaled menu input failed");key(SDLK_ESCAPE);
 }
 // Compare menu+target input against the exact direct engine command and RNG.
 Prototype expected=*game;int target=plan.at("target");const auto& target_b=game->state().at("units").at(std::to_string(target)).at("battle");
 expected.command("tile",target_b.at("x"),target_b.at("y"));row_click(0);unit_click(target);
 check(playback.active()&&!attack_targeting&&action_menu_unit<0&&game->hash()==expected.hash(),"menu attack differs from direct command");
 draw(out/"attack-start.bmp");finish_animation();while(game->has_dialogue())game->command("continue");
 unit_click(0);check(action_menu_unit<0,"spent unit reopened menu");
 choose_battle(2);while(game->has_dialogue())game->command("continue");unit_click(0);move_selected();key(SDLK_B);check(action_menu_unit<0&&show_battles,"battle selector retained menu");key(SDLK_ESCAPE);
 std::ofstream(out/"verification.json")<<Json{{"scenarios",verified},{"move_then_menu",true},{"consecutive_units",true},{"cancel_then_reselect",true},{"unmoved_restore_no_menu",true},{"slow_frame_completion",true},{"save_restore_cancel",true},{"scaled_input",true},{"attack_matches_direct_engine",true},{"disabled_no_mutation",true},{"presentation_only",true}}.dump(2);
 std::cout<<"action menu: consecutive units, cancel/reselect, only after movement, four battles, keyboard, scaled input, saves and identical attack/RNG passed\n";
}
void tactics_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;Json reports=Json::array();
 auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
 for(int scenario:{1,2}){
  choose_battle(scenario);while(game->has_dialogue())game->command("continue");int wanted=scenario==1?135:132;prepare_caster_contact(*game,wanted);bool cast_seen=false,finished=false;int effect_frames=0;Json request=nullptr;
  for(int n=0;n<12000&&!finished;++n){
   if(!playback.active()){
    if(cast_seen){finished=true;break;}
    if(game->has_dialogue()){send("continue");continue;}
    if(game->state().at("phase")=="player")send("end_turn");
    check(game->state().at("phase")=="enemy","battle ended before caster acted");
    auto r=game->enemy_request();if(!r.is_null()&&r.at("state").at("actor")==wanted)request=r;
   }
   next_enemy=0;animation_clock=SDL_GetTicks()-120;tick();
   if(playback.active()&&playback.packet().value("kind",std::string())=="tactic"&&playback.packet().at("actor")==wanted){
    if(!cast_seen){cast_seen=true;draw(out/("battle-"+std::to_string(scenario)+"-casting.bmp"));std::ofstream(out/("battle-"+std::to_string(scenario)+"-request.json"))<<request.dump(2);std::ofstream(out/("battle-"+std::to_string(scenario)+"-evaluation.json"))<<JevEnemy::evaluation(request).dump(2);}
    if(!playback.spell_effect().is_null()){++effect_frames;if(effect_frames==6)draw(out/("battle-"+std::to_string(scenario)+"-effect.bmp"));}
   }
  }
  check(cast_seen&&finished&&effect_frames>=10,"enemy spell was not visibly presented");
  auto saved=game->serialize();persist(false);check(notice=="已保存。","spell result save failed");persist(true);check(game->serialize()==saved,"spell result load changed MP/status/RNG");
  draw(out/("battle-"+std::to_string(scenario)+"-after.bmp"));reports.push_back({{"scenario",scenario},{"actor",wanted},{"effect_frames",effect_frames},{"save_restore",true}});
 }
 // Player opens the spell picker without moving via its explicit T shortcut.
 choose_battle(0);send("tile",12,12); // The reference opening position.
 if(game->state().at("selected")!=0){const auto& b=game->state().at("units").at("0").at("battle");send("tile",b.at("x"),b.at("y"));}
 SDL_Event shortcut{};shortcut.type=SDL_EVENT_KEY_DOWN;shortcut.key.key=SDLK_T;SDL_PushEvent(&shortcut);tick();check(show_spells,"T did not open unmoved spell picker");
 shortcut.key.key=SDLK_ESCAPE;SDL_PushEvent(&shortcut);tick();check(!show_spells&&action_menu_unit<0,"Esc from unmoved spell picker opened action menu");
 auto costs=game->movement();bool moved=false;
 for(int pos=0;pos<game->cell_count()&&!moved;++pos)if(costs[pos]>0&&costs[pos]!=255&&!game->spell_options(0,{pos%game->map_width(),pos/game->map_width()}).empty()){
  bool occupied=false;for(const auto& u:game->state().at("units"))if(u.at("battle").at("hp").get<int>()>0&&u.at("battle").at("x")==pos%game->map_width()&&u.at("battle").at("y")==pos/game->map_width())occupied=true;
  if(!occupied)moved=send("tile",pos%game->map_width(),pos/game->map_width());
 }
 check(moved,"player needs legal movement into tactic range");
 for(int n=0;playback.active()&&n<2000;++n){animation_clock=SDL_GetTicks()-120;tick();}
 auto options=game->spell_options(0,{game->state().at("units").at("0").at("battle").at("x"),game->state().at("units").at("0").at("battle").at("y")});
 check(!options.empty(),"opening Cao Cao has no legal wind target");
 check(action_menu_unit==0&&action_enabled(1),"post-move strategy row unavailable");auto strategy=action_row(1);click(strategy.x+strategy.w/2,strategy.y+strategy.h/2,false);tick();check(show_spells,"strategy row did not open spell picker");draw(out/"player-spells.bmp");
 SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;
 e.key.key=SDLK_1;SDL_PushEvent(&e);tick();check(target_tactic==10,"spell picker selected wrong spell");
 auto option=options.front();const auto& b=game->state().at("units").at(std::to_string(option.at("target").get<int>())).at("battle");auto r=tile_rect(b.at("x"),b.at("y"));
 auto before=game->serialize();auto mp=game->state().at("units").at("0").at("battle").at("mp").get<int>();
 click(24,map_top+24,false);check(game->serialize()==before,"invalid tactic target changed state");
 click(r.x+r.w/2,r.y+r.h/2,false);check(playback.active()&&playback.packet().at("kind")=="tactic"&&game->state().at("units").at("0").at("battle").at("mp")==mp-6,"player spell did not execute");
 for(int n=0;playback.active()&&n<2000;++n){animation_clock=SDL_GetTicks()-120;tick();}
 check(!playback.active(),"player spell stuck");persist(false);auto post=game->serialize();persist(true);check(game->serialize()==post,"player spell save changed next state");
 std::ofstream(out/"verification.json")<<Json{{"enemy_casts",reports},{"player_spell",true},{"invalid_target_no_mutation",true},{"mp_poison_save_restore",true}}.dump(2);
 std::cout<<"native tactics: Li Su poison, Li Ru elemental effect, player picker/target, MP/status saves passed\n";
}
void hover_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);jev_enabled=false;
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 auto motion=[&](float x,float y){
  float wx,wy;SDL_RenderCoordinatesToWindow(renderer,x,y,&wx,&wy);SDL_Event e{};
  e.type=SDL_EVENT_MOUSE_MOTION;e.motion.x=wx;e.motion.y=wy;SDL_PushEvent(&e);tick();
 };
 auto reveal=[&](){hover_panel_since=SDL_GetTicks()-hover_panel_delay;};
 Json verified=Json::array();
 for(int scenario=0;scenario<4;++scenario){
  choose_battle(scenario);while(game->has_dialogue())game->command("continue");
  auto before=game->serialize();int count=0;
  for(const auto& u:game->state().at("units")){
   auto b=BattleUnit::from_json(u.at("battle"));auto tile=tile_rect(b.x,b.y);
   motion(-1,-1);motion(tile.x+tile.w/2,tile.y+tile.h/2);
   if(hover_panel_target!=b.id)throw std::runtime_error("scaled hover selected wrong unit: scenario="+std::to_string(scenario)+" expected="+std::to_string(b.id)+" actual="+std::to_string(hover_panel_target)+" mouse="+std::to_string(mouse_x)+","+std::to_string(mouse_y));
   check(hover_panel_unit(hover_panel_since+hover_panel_delay-1)<0,"hover appeared before delay");
   check(hover_panel_unit(hover_panel_since+hover_panel_delay)==b.id,"hover did not appear after delay");
   reveal();draw();++count;
   if(b.id==0||b.id==132||b.id==210){
    auto name="battle-"+std::to_string(scenario)+"-unit-"+std::to_string(b.id);
    draw(out/(name+".bmp"));std::ofstream(out/(name+".json"))<<hover_panel_events(b.id).dump(2);
   }
  }
  check(game->serialize()==before,"hover changed gameplay or save state");
  for(auto cell:{std::array{0,0},std::array{game->map_width()-1,0},std::array{0,game->map_height()-1},std::array{game->map_width()-1,game->map_height()-1}}){
   auto panel=hover_panel_rect(cell[0],cell[1]),unit=tile_rect(cell[0],cell[1]);
   check(panel.x>=0&&panel.x+panel.w<=width&&panel.y>=map_top&&panel.y+panel.h<=map_top+game->map_height()*map_tile(),"hover clipped at map edge");
   check(!SDL_HasRectIntersectionFloat(&panel,&unit),"hover covers inspected tile");
  }
  verified.push_back({{"scenario",scenario},{"units",count},{"unchanged_hash",game->hash()}});
 }
 choose_battle(2);while(game->has_dialogue())game->command("continue");
 auto b=BattleUnit::from_json(game->state().at("units").at("0").at("battle"));
 for(auto size:{std::array{720,840},std::array{1200,960}}){
  SDL_SetWindowSize(window,size[0],size[1]);SDL_SyncWindow(window);
  auto r=tile_rect(b.x,b.y);motion(r.x+r.w/2,r.y+r.h/2);reveal();draw(out/("scaled-"+std::to_string(size[0])+".bmp"));
  check(hover_panel_unit(SDL_GetTicks())==0,"letterboxed hover coordinates failed");
 }
 show_info=true;draw();check(hover_panel_target<0,"hover leaked over information modal");show_info=false;
 show_battles=true;draw();check(hover_panel_target<0,"hover leaked over battle selector");show_battles=false;
 auto unchanged=game->serialize();motion(500,map_top+24);check(hover_panel_target<0,"hover stayed on empty tile");
 auto r=tile_rect(b.x,b.y);motion(r.x+r.w/2,r.y+r.h/2);reveal();
 // A visible popup is only paint: clicking the hovered unit still selects it.
 float wx,wy;SDL_RenderCoordinatesToWindow(renderer,r.x+r.w/2,r.y+r.h/2,&wx,&wy);SDL_Event click{};
 click.type=SDL_EVENT_MOUSE_BUTTON_DOWN;click.button.button=SDL_BUTTON_LEFT;click.button.x=wx;click.button.y=wy;SDL_PushEvent(&click);tick();
 check(game->state().at("selected")==0,"hover blocked unit selection");
 SDL_Event leave{};leave.type=SDL_EVENT_WINDOW_MOUSE_LEAVE;SDL_PushEvent(&leave);tick();check(hover_panel_target<0,"hover stayed after window leave");
 game->restore(unchanged);
 std::ofstream(out/"verification.json")<<Json{{"scenarios",verified},{"delay_ms",hover_panel_delay},{"scaled_input",true},{"edge_placement",true},{"nonmutating_hover",true},{"click_through",true}}.dump(2);
 std::cout<<"hover panels: all four rosters, delay, map edges, scaled input, modal suppression, click-through and unchanged snapshots passed\n";
}
void scenarios_combat_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);
 for(int scenario=1;scenario<4;++scenario){
  choose_battle(scenario);bool flashed=false;int walks=0,poses=0;
  for(int n=0;n<200&&!flashed;++n){
   if(game->has_dialogue()){game->command("continue");continue;}
   if(game->state().at("phase")=="player")game->command("end_turn");else game->enemy_step();
   collect_presentation();
   for(int ticks=0;playback.active()&&ticks<3000;++ticks){
    bool flash=false;for(const auto& pose:playback.poses()){++poses;if(!pose.at("effects").empty())flash=true;}
    if(flash&&!flashed){draw(out/(std::to_string(scenario)+"-enemy-flash.bmp"));flashed=true;}
    else draw();
    bool moving=playback.moving();if(moving)++walks;playback.advance(40);
    if(playback.active()&&moving!=playback.moving())begin_presentation();
   }
  }
  if(!flashed||!walks||!poses)throw std::runtime_error("new battle enemy animation missing");
  std::cout<<"native battle "<<scenario<<" enemy move-before-hit, swing and flash rendered; walking frames="<<walks<<" poses="<<poses<<'\n';
 }
}
void scenarios_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("scenario smoke requires isolated saves");
 std::filesystem::create_directories(out);
 auto mouse=[&](float x,float y){float wx,wy;SDL_RenderCoordinatesToWindow(renderer,x,y,&wx,&wy);SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=wx;e.button.y=wy;SDL_PushEvent(&e);tick();};
 auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;SDL_PushEvent(&e);tick();};
 auto opening=game->serialize();persist(false);auto oldpath=save_path();Json hashes=Json::object();
 for(int scenario=1;scenario<4;++scenario){
  key(SDLK_B);if(!show_battles)throw std::runtime_error("battle selector did not open");
  if(scenario==1)draw(out/"selector.bmp");auto r=battle_choice(scenario);mouse(r.x+30,r.y+24);
  if(game->scenario()!=scenario||show_battles)throw std::runtime_error("battle selection failed");
  draw(out/(std::to_string(scenario)+"-intro.bmp"));persist(false);auto intro=game->serialize();
  // Render every original intro page; validate line widths before capture.
  while(game->has_dialogue()){
   auto page=game->dialogue();float line=0;for(auto cp:codepoints(page.at("text"))){if(cp==10){line=0;continue;}line+=font.at("glyphs").at(std::to_string(cp)).at("advance").get<float>();if(line>750)throw std::runtime_error("clipped scenario dialogue");}
   draw();key(SDLK_SPACE);
  }
  auto stable=game->serialize();persist(true);if(game->serialize()!=intro)throw std::runtime_error("scenario intro persistence failed");restore_snapshot(stable);
  auto b=game->state().at("units").at("0").at("battle");
  for(auto size:{std::array{960,1120},std::array{1200,850}}){
   SDL_SetWindowSize(window,size[0],size[1]);SDL_SyncWindow(window);tick();auto pos=tile_rect(b.at("x"),b.at("y"));mouse(pos.x+pos.w/2,pos.y+pos.h/2);
   if(game->state().at("selected")!=0)throw std::runtime_error("scaled scenario input failed");send("cancel");
  }
  SDL_SetWindowSize(window,960,1120);SDL_SyncWindow(window);draw(out/(std::to_string(scenario)+"-battle.bmp"));
  if(scenario==3){auto unchanged=game->hash();mouse(400,map_top+game->map_height()*map_tile()+20);if(game->hash()!=unchanged)throw std::runtime_error("wide map bottom input leaked");}
  // A real player move, native playback frames, cancel, separate save and load.
  auto pos=tile_rect(b.at("x"),b.at("y"));mouse(pos.x+pos.w/2,pos.y+pos.h/2);auto plan=game->suggest(0);pos=tile_rect(plan.at("x"),plan.at("y"));mouse(pos.x+pos.w/2,pos.y+pos.h/2);
  if(!playback.active())throw std::runtime_error("scenario walking missing");int frames=0;
  while(playback.active()){playback.advance(40);draw();++frames;}if(!frames)throw std::runtime_error("no walking frames");
  persist(false);auto moved=game->serialize();send("cancel");persist(true);if(game->serialize()!=moved)throw std::runtime_error("scenario move save failed");
  std::ifstream f(oldpath);std::string saved{std::istreambuf_iterator<char>(f),{}};if(saved!=opening)throw std::runtime_error("opening save overwritten");
  // Render every roster animation via the same atlas loader used in gameplay.
  for(const auto& u:game->state().at("units")){
   auto r=UnitRecord::from_json(u.at("record"));auto b=BattleUnit::from_json(u.at("battle"));
   draw_unit_pose({{"archive","UNIT_ATK.E5"},{"sprite",battle_sprite(b.id,r.job,b.side)},{"frame",0},{"width",64},{"x",b.x*48},{"y",b.y*48+56},{"mirror",false},{"effects",Json::array()}});
  }
  hashes[std::to_string(scenario)]=game->hash();
  // Explicit terminal fixture tests the visible Next Battle button. Engine tests
  // separately reach victories through commands without modifying snapshots.
  auto won=Json::parse(game->serialize());auto& state=won["payload"]["state"];state["selected"]=-1;state["undo"]=nullptr;state["phase"]="victory";
  for(auto& u:state["units"])if(u["battle"]["side"]==2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}
  won["sha256"]=sha256(won.at("payload").dump());restore_snapshot(won.dump());draw(out/(std::to_string(scenario)+"-outcome-control.bmp"));mouse(next_button.x+40,next_button.y+20);
  if(scenario<3&&game->scenario()!=scenario+1)throw std::runtime_error("next battle button failed");
  if(scenario==3){if(!show_battles)throw std::runtime_error("final battle menu failed");key(SDLK_ESCAPE);}
  std::cout<<"native scenario "<<scenario<<" selector, dialogue, scaled input, walking, sprites, separate saves passed "<<game->hash()<<'\n';
 }
 std::ofstream(out/"saved-hashes.json")<<hashes.dump();
 choose_battle(0);persist(true);if(game->serialize()!=opening)throw std::runtime_error("opening save restoration changed");
}
void dialogue_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("dialogue smoke requires an isolated save directory");
 std::filesystem::create_directories(out);SDL_SetWindowSize(window,width,height);SDL_SyncWindow(window);
 auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;SDL_PushEvent(&e);tick();};
 auto restore=[&](Json envelope){envelope["sha256"]=sha256(envelope.at("payload").dump());restore_snapshot(envelope.dump());};
 auto wait_animation=[&](){auto start=SDL_GetTicks();while(playback.active()&&SDL_GetTicks()-start<15000){check(!showing_dialogue(),"dialogue covered unfinished animation");tick();SDL_Delay(16);}check(!playback.active(),"dialogue animation timed out");};
 const auto initial=Json::parse(game->serialize());check(initial.at("payload").at("schema")==3,"dialogue smoke disabled production mode");
 auto near=initial;near["payload"]["state"]["units"]["0"]["battle"]["x"]=9;near["payload"]["state"]["units"]["0"]["battle"]["y"]=13;restore(near);
 click(9.5f*tile,map_top+13.5f*tile,false);click(9.5f*tile,map_top+12.5f*tile,false);
 check(playback.moving()&&!game->has_dialogue(),"tentative walking started conversation");wait_animation();
 click(9.5f*tile,map_top+11.5f*tile,false);check(playback.active()&&game->has_dialogue()&&!showing_dialogue(),"attack/conversation ordering wrong");wait_animation();
 check(showing_dialogue()&&game->dialogue().at("event")==41,"original encounter missing after strike");draw(out/"zhang-liang-start.bmp");
 auto frozen=game->serialize();click(100,150,false);key(SDLK_E);check(game->serialize()==frozen,"map/end-turn input advanced behind dialogue");
 key(SDLK_SPACE);check(game->dialogue().at("speaker")=="张梁","space did not advance dialogue");key(SDLK_F5);check(notice=="已保存。","dialogue save failed");
 const auto saved=game->serialize();key(SDLK_RETURN);key(SDLK_F9);check(game->serialize()==saved&&showing_dialogue(),"dialogue load did not restore exact page");draw(out/"zhang-liang-restored.bmp");
 int captured=0;Json pages=Json::array();
 auto capture_pages=[&](){while(showing_dialogue()){
  auto page=game->dialogue();pages.push_back(page);std::istringstream lines(page.at("text").get<std::string>());std::string line;int count=0;
  while(std::getline(lines,line)){float advance=0;for(auto cp:codepoints(line))advance+=font.at("glyphs").at(std::to_string(cp)).at("advance").get<float>();check(advance<=750,"original dialogue line clips");++count;}check(count<=3,"original dialogue page clips");
  draw(out/("page-"+number3(++captured)+".bmp"));click(700,990,false);
 }};
 capture_pages();check(!game->has_dialogue(),"click did not finish encounter dialogue");
 key(SDLK_E);check(!game->has_dialogue(),"completed encounter replayed");
 // End turn with an eligible encounter: animation-free dialogue must hold AI.
 restore(near);near["payload"]["state"]["units"]["0"]["battle"]["y"]=12;restore(near);key(SDLK_E);
 frozen=game->serialize();next_enemy=0;tick();check(game->serialize()==frozen&&showing_dialogue(),"enemy scheduler ran during dialogue");
 // Both victory branches and all named retreat pages use production rendering.
 for(bool companions:{true,false}){
  auto e=initial;auto& s=e["payload"]["state"];for(auto& u:s["units"]){auto& b=u["battle"];if((b["side"]==2&&b["id"]!=146)||(!companions&&b["id"]==32)){b["hp"]=0;b["lifecycle"]=3;}}
  s["units"]["0"]["battle"]["x"]=9;s["units"]["0"]["battle"]["y"]=12;s["units"]["146"]["battle"]["hp"]=1;restore(e);
  send("tile",9,12);send("tile",9,11);wait_animation();check(game->state().at("phase")=="victory"&&showing_dialogue(),"victory suppressed speech");capture_pages();draw(out/(companions?"victory.bmp":"victory-alternate.bmp"));
 }
 for(int unit:{3,33}){auto e=initial;auto& b=e["payload"]["state"]["units"][std::to_string(unit)]["battle"];b["hp"]=0;b["lifecycle"]=3;restore(e);send("cancel");capture_pages();}
 auto e=initial;e["payload"]["state"]["turn"]=20;restore(e);send("end_turn");for(int n=0;n<15;++n)game->enemy_step();check(showing_dialogue(),"defeat narration missing");capture_pages();draw(out/"defeat.bmp");
 // Leave the mid-dialogue slot intact for a fresh-process --resume-smoke check.
 restore_snapshot(saved);draw(out/"dialogue-final-restored.bmp");
 Json report={{"version","0.4.0"},{"production_schema",3},{"pages_rendered",captured},{"original_pages",pages},{"animation_order",true},{"enemy_pause",true},{"input_guards",true},{"save_restore",true},{"glyphs_and_line_widths",true},{"saved_hash",game->hash()},{"renderer",SDL_GetRendererName(renderer)}};
 std::ofstream(out/"result.json")<<report.dump(2)<<'\n';std::cout<<"dialogue rendering/pages="<<captured<<" save="<<game->hash()<<'\n';
}
void movement_smoke(const std::filesystem::path& out){
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("movement smoke requires an isolated save directory");
 std::filesystem::create_directories(out);SDL_SetWindowSize(window,width,height);SDL_SyncWindow(window);
 auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 auto signature=[](const Json& pose){return Json::array({pose.at("x"),pose.at("y"),pose.at("frame"),pose.at("mirror")}).dump();};
 Prototype expected(assets,false,false);send("tile",10,6);expected.command("tile",10,6);persist(false);
 const auto before=game->serialize();send("tile",8,9);expected.command("tile",8,9);
 check(playback.active()&&playback.moving(),"movement click did not start walking");
 const auto committed=game->serialize();const auto packet=playback.packet();std::set<std::string> wanted,seen;
 for(const auto& e:packet.at("events"))if(e.at("kind")=="frame")wanted.insert(signature(e));
 check(!send("tile",8,10)&&!send("end_turn")&&!send("cancel")&&!send("restart"),"command interrupted walking");
 persist(false);persist(true);check(game->serialize()==committed&&playback.active(),"save/load guard interrupted walking");
 {std::ifstream f(save_path());std::string stored{std::istreambuf_iterator<char>(f),{}};check(stored==before,"walking overwrote stable save");}
 auto bad=Json::parse(before);bad["payload"]["state"]["rng"]=123;bool invalid=false;
 try{restore_snapshot(bad.dump());}catch(const std::exception&){invalid=true;}
 check(invalid&&playback.active()&&game->serialize()==committed,"bad restore interrupted walking");
 auto poses=playback.poses();show_info=true;animation_clock=SDL_GetTicks()-500;tick();
 check(playback.poses()==poses&&playback.moving(),"modal did not pause walking");show_info=false;
 const auto start=SDL_GetTicks();
 while(playback.active()&&SDL_GetTicks()-start<15000){
  tick();if(playback.active())for(const auto& pose:playback.poses())if(pose.at("slot")==0&&seen.insert(signature(pose)).second)
   draw(out/("walk-"+number3(int(seen.size()))+".bmp"));
  SDL_Delay(120); // Exercise the same slow-frame condition as the attack regression.
 }
 check(!playback.active()&&seen==wanted,"slow rendering omitted walking frames");
 check(game->hash()==expected.hash(),"walking altered simulation result");draw(out/"destination.bmp");
 persist(false);send("cancel");expected.command("cancel");check(game->hash()==expected.hash(),"cancel did not restore pre-move state");
 persist(true);check(game->serialize()==committed&&!playback.active(),"stable save did not restore moved state");
 restore_snapshot(before);send("tile",8,9);restore_snapshot(before);
 check(!playback.active()&&game->serialize()==before,"valid restore retained stale walking");
 Json report={{"walking_frames",seen.size()},{"path",packet.at("path")},{"slow_frame_delay_ms",120},{"duration_ms",SDL_GetTicks()-start},{"guards",true},{"restoration",true}};
 std::ofstream(out/"result.json")<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}
void enemy_animation_smoke(const std::filesystem::path& out,int frame_delay=16,bool player_first=false){
 SDL_SetWindowSize(window,width,height);SDL_SyncWindow(window);
 std::filesystem::create_directories(out);
 auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 auto capture=[&](const std::string& name){draw(out/name);};
 Prototype expected(assets,false,false);
 if(player_first){
  for(auto cell:std::array<std::array<int,2>,3>{{{10,6},{8,9},{8,10}}}){
   expected.command("tile",cell[0],cell[1]);click((cell[0]+0.5f)*tile,map_top+(cell[1]+0.5f)*tile,false);
   if(playback.moving())while(playback.active()){tick();SDL_Delay(16);}
  }
  check(playback.active()&&attack_actor==0&&attack_target==483,"player click sequence did not start its attack");
  while(playback.active()){tick();SDL_Delay(16);}
 }
 for(int phase=0;phase<2;++phase){expected.command("end_turn");while(expected.state().at("phase")=="enemy")expected.enemy_step();}
 auto end_button=tool_rect(2);mouse_x=end_button.x+10;mouse_y=end_button.y+10;
 const auto start=SDL_GetTicks();int count=0,confused=0;Json observed=Json::array();
 bool live=false;uint64_t began=0;std::set<int> swing_frames;bool flash=false,number=false;
 button(2);
 while(SDL_GetTicks()-start<(frame_delay>16?120000:60000)){
  const auto before=game->state();tick();const auto& after=game->state();
  for(const auto& u:after.at("units")){
   auto id=std::to_string(u.at("battle").at("id").get<int>());const auto& old=before.at("units").at(id).at("battle");
   if(old.at("status")!=0&&u.at("battle").at("status")==0){
    ++confused;check(status_message().find("混乱")!=std::string::npos,"toolbar hover hid the confusion explanation");
    if(confused==1)capture("enemy-confused.bmp");
   }
   if(u.at("battle").at("hp")<old.at("hp")){
    check(playback.active()&&!live,"enemy damage without a new animation");
    check(visible_state().at("units").at(id).at("battle").at("hp")==old.at("hp"),"enemy HP changed before animation");
    ++count;live=true;began=SDL_GetTicks();swing_frames.clear();flash=false;number=false;
    observed.push_back({{"target",u.at("battle").at("id")},{"hp_before",old.at("hp")},{"hp_after",u.at("battle").at("hp")},{"turn",after.at("turn")},{"start_ms",began-start}});
    capture("enemy-"+number3(count)+"-start.bmp");
   }
  }
  if(playback.active()){
   check(status_message().find(playback.moving()?"移动":"攻击")!=std::string::npos&&status_message()!=toolbar[2].hint,"toolbar hover hid the action label");
   for(const auto& pose:playback.poses()){
    if(pose.at("archive")=="UNIT_ATK.E5"&&swing_frames.insert(pose.at("frame").get<int>()).second)capture("enemy-"+number3(count)+"-swing-"+pose.at("frame").dump()+".bmp");
    if(!pose.at("effects").empty()&&!flash){flash=true;capture("enemy-"+number3(count)+"-impact.bmp");}
   }
   if(!playback.number().is_null()&&!number){number=true;capture("enemy-"+number3(count)+"-number.bmp");}
  }else if(live){
   auto elapsed=SDL_GetTicks()-began;
   check(elapsed>=1000&&swing_frames.size()==4&&flash&&number,"enemy animation omitted swing/impact/number or completed too quickly");
   observed.back().update({{"duration_ms",elapsed},{"distinct_swing_frames",swing_frames.size()},{"flash",flash},{"number",number}});live=false;
  }
  if(after.at("phase")=="player"){
   if(after.at("turn")==3){check(count==0&&confused==14,"opening confusion did not skip all enemy attacks");button(2);}
   else if(after.at("turn")==4){check(count>=2&&!live,"ordinary enemy turn did not exercise consecutive attacks");break;}
  }
  SDL_Delay(frame_delay);
 }
 check(game->state().at("phase")=="player"&&game->state().at("turn")==4,"real-time enemy playback timed out");
 check(game->hash()==expected.hash(),"presentation changed mixed-turn simulation state");
 Json report={{"player_attacked_first",player_first},{"confused_skipped",confused},{"attacks",observed},{"final_hash",game->hash()},{"elapsed_ms",SDL_GetTicks()-start}};
 std::ofstream(out/"result.json")<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}
void attack_smoke(const std::filesystem::path& out){
 SDL_SetWindowSize(window,width,height);SDL_SyncWindow(window);
 if(!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("attack smoke requires an isolated save directory");
 std::filesystem::create_directories(out);
 auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 auto capture=[&](const std::string& name){draw(out/name);};
 auto read_save=[&]{std::ifstream f(save_path());return std::string{std::istreambuf_iterator<char>(f),{}};};
 auto advance=[&](int ms){playback.advance(ms);for(int sound:playback.take_sounds())attack_sound(sound);};
 send("tile",10,6);send("tile",8,9);advance(10000);persist(false);const auto saved=read_save();
 check(send("tile",8,10)&&playback.active(),"player attack did not start playback");
 const auto committed=game->serialize();capture("before.bmp");
 check(!send("end_turn")&&!send("restart")&&!send("tile",8,10),"command interrupted animation");
 persist(false);persist(true);check(read_save()==saved&&game->serialize()==committed,"save/load guard failed");
 auto poses=playback.poses();auto bad=Json::parse(saved);bad["payload"]["state"]["rng"]=123;
 bool invalid=false;try{restore_snapshot(bad.dump());}catch(const std::exception&){invalid=true;}
 check(invalid&&playback.active()&&playback.poses()==poses&&game->serialize()==committed,"invalid restore interrupted playback");
 show_info=true;animation_clock=SDL_GetTicks()-500;tick();
 check(playback.poses()==poses&&playback.number().is_null(),"information modal advanced attack");
 show_info=false;notice.clear();
 for(int n=0;n<27;++n){check(playback.active(),"renderer attack ended early");capture("tick-"+number3(n)+".bmp");advance(PrototypePlayback::tick_ms);}
 check(!playback.active()&&game->serialize()==committed,"renderer applied damage more than once");
 capture("after.bmp");persist(false);check(read_save()==committed,"post-animation save differs");
 restore_snapshot(saved);send("tile",8,10);restore_snapshot(saved);
 check(!playback.active()&&attack_sounds.empty()&&game->serialize()==saved,"valid restore retained old animation");
 send("tile",8,10);advance(27*PrototypePlayback::tick_ms);
 check(game->serialize()==committed,"next attack after restore changed");
 // Exercise the real enemy scheduler and delay the defeat screen until impact ends.
 auto defeat=Json::parse(saved);auto& state=defeat["payload"]["state"];
 state["units"]["0"]["battle"]["hp"]=1;
 for(auto& u:state["units"])if(u["battle"]["side"]==2){u["battle"]["status"]=0;if(u["battle"]["id"]!=483){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}}
 defeat["sha256"]=sha256(defeat.at("payload").dump());restore_snapshot(defeat.dump());send("end_turn");
 for(int n=0;n<20&&!playback.active()&&game->state().at("phase")=="enemy";++n){next_enemy=0;tick();}
 check(playback.active()&&game->state().at("phase")=="defeat"&&visible_state().at("phase")=="enemy","enemy lethal attack presentation missing");
 auto cursor=game->state().at("enemy_cursor");auto lethal=game->hash();
 for(int n=0;n<3;++n){next_enemy=0;animation_clock=SDL_GetTicks();tick();check(game->state().at("enemy_cursor")==cursor&&game->hash()==lethal,"enemy scheduler overlapped an attack");}
 capture("enemy-before.bmp");advance(13*PrototypePlayback::tick_ms);capture("enemy-impact.bmp");advance(14*PrototypePlayback::tick_ms);
 check(!playback.active()&&visible_state().at("phase")=="defeat","defeat not revealed after animation");capture("defeat.bmp");
 // Leave the isolated smoke save at the normal post-attack checkpoint.
 restore_snapshot(committed);persist(false);
 std::cout<<"native attack frames/audio queued, command/save/load guards, modal pause, transactional restore, enemy scheduler and delayed defeat passed "<<game->hash()<<'\n';
}
void render_smoke(const std::filesystem::path& out){
 std::filesystem::create_directories(out);
 auto capture=[&](const char* name){draw(out/name);};
 auto mouse=[&](float lx,float ly,int button=SDL_BUTTON_LEFT){
  float wx,wy;SDL_RenderCoordinatesToWindow(renderer,lx,ly,&wx,&wy);SDL_Event e{};
  e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=button;e.button.x=wx;e.button.y=wy;SDL_PushEvent(&e);tick();
 };
 auto tool=[&](int i){auto r=tool_rect(i);mouse(r.x+r.w/2,r.y+r.h/2);};
 auto key=[&](SDL_Keycode key){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;SDL_PushEvent(&e);tick();};
 SDL_SetWindowSize(window,1080,1260);SDL_SyncWindow(window);tick();capture("initial.bmp");
 // Portrait framing and a wide, letterboxed window must use the same map input.
 for(auto size:{std::array{1080,1260},std::array{1200,960}}){
  SDL_SetWindowSize(window,size[0],size[1]);SDL_SyncWindow(window);tick();
  mouse(10*tile+24,map_top+6*tile+24);
  if(game->state().at("selected")!=0)throw std::runtime_error("native scaled input selection failed");
  tool(1);
 }
 auto unchanged=game->hash();
 mouse(-1,map_top+24);mouse(width,map_top+24);mouse(24,map_top-1);mouse(24,map_bottom);
 if(game->hash()!=unchanged)throw std::runtime_error("map edge click escaped battlefield bounds");
 key(SDLK_I);capture("information.bmp");mouse(10*tile+24,map_top+6*tile+24);key(SDLK_E);
 if(game->hash()!=unchanged)throw std::runtime_error("information modal leaked gameplay input");
 mouse(info_close.x+20,info_close.y+16);if(show_info)throw std::runtime_error("information close failed");
 mouse(10*tile+24,map_top+6*tile+24);capture("selected.bmp");
 auto before=game->state().at("units");auto costs=game->movement();int dest=-1;
 for(int p=0;p<400;++p)if(costs[p]>0&&costs[p]!=255){
  bool occupied=false;for(const auto& u:game->state().at("units"))if(u.at("battle").at("x")==p%20&&u.at("battle").at("y")==p/20)occupied=true;
  if(!occupied){dest=p;break;}
 }
 if(dest<0)throw std::runtime_error("no test movement");
 mouse((dest%20)*tile+24,map_top+(dest/20)*tile+24);
 if(game->state().at("undo").is_null())throw std::runtime_error("native movement click failed");
 playback.advance(10000);
 auto moved=game->hash();tool(3);mouse(500,500,SDL_BUTTON_RIGHT);
 if(game->state().at("units")!=before)throw std::runtime_error("native cancel failed");
 tool(4);if(game->hash()!=moved)throw std::runtime_error("native toolbar save/load failed");
 key(SDLK_H);capture("moved.bmp");
 tool(0);if(game->state().at("selected")!=-1)throw std::runtime_error("toolbar wait failed");
 tool(4);tool(2);if(game->state().at("phase")!="enemy")throw std::runtime_error("toolbar end turn failed");
 tool(4);tool(5);if(game->state().at("turn")!=2||game->state().at("selected")!=-1)throw std::runtime_error("toolbar restart failed");
 tool(4);if(game->hash()!=moved)throw std::runtime_error("toolbar restart changed saved progress");
 std::ofstream(out/"state.json")<<game->serialize();
 std::cout<<"native original-art layout, scaled input, modal, toolbar, move/cancel/save/load smoke passed "<<game->hash()<<'\n';
}
#include "prototype_special_smoke.inc"
#endif
}
extern "C" {
API const char* prototype_snapshot(){try{if(special_choosing)throw std::runtime_error("Please select a hero first.");if(playback.active())throw std::runtime_error("动画中，请等待。");snapshot_buffer=game->serialize();return snapshot_buffer.c_str();}catch(const std::exception& e){notice=e.what();return nullptr;}}
API int prototype_restore(const char* s){try{restore_snapshot(s);notice="已读取存档。";return 1;}catch(const std::exception& e){notice=e.what();return 0;}}
API const char* prototype_status(){snapshot_buffer=game->state().dump();return snapshot_buffer.c_str();}
API const char* prototype_dialogue(){snapshot_buffer=game->dialogue().dump();return snapshot_buffer.c_str();}
API const char* prototype_hash(){snapshot_buffer=game->hash();return snapshot_buffer.c_str();}
API const char* prototype_error(){return notice.c_str();}
API int prototype_command(const char* cmd,int x,int y){return send(cmd,x,y)?1:0;}
API int prototype_busy(){return playback.active()?1:0;}
API int prototype_slot(){return manual_slots?save_slot:0;}
API int prototype_set_slot(int slot){if(slot<0||slot>10)return 0;save_slot=slot;manual_slots=true;return 1;}
API int prototype_scenario(){return game->scenario();}
API int prototype_frames(){return int(frames);}
// Hosted touch input arrives only after browser gesture arbitration. These are
// presentation entrypoints, not additional battle commands or snapshot fields.
API int prototype_web_tap(float x,float y){
 if(!web_hosted||web_paused||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=width||y>=height)return 0;
 const auto& phase=game->state().at("phase");
 const bool map_tap=!playback.active()&&!backup_busy&&!show_slots&&!show_unit_details&&!show_info&&!show_inventory&&!show_battles&&!show_spells&&target_item<0&&target_tactic<0&&!attack_targeting&&action_menu_unit<0&&!game->has_dialogue()&&game->story_choices().empty()&&(phase=="player"||phase=="enemy");
 const int unit=map_tap?unit_at_pointer(x,y):-1;
 const bool dismiss=touch_hover_input&&unit>=0&&unit==touch_hover_unit;
 mouse_x=x;mouse_y=y;touch_hover_input=true;clear_touch_hover();
 // Keep the campaign's hover-toggle behavior. In the duel, tapping the current
 // ground tile also deliberately confirms acting in place, without a fake move.
 bool current_tile=false;
 if(special_hulao&&actionable_unit()&&unit==game->state().at("selected")){
  const auto& b=game->state().at("units").at(std::to_string(unit)).at("battle");auto cell=voxel_camp_at(x,y);if(cell[0]<0)cell=voxel::cell(x,y);
  current_tile=cell[0]==b.at("x")&&cell[1]==b.at("y");
 }
 if(dismiss&&!current_tile)return 1;
 touch_hover_unit=unit;click(x,y,false);return 1;
}
std::string web_inspection_error;
API int prototype_web_inspect(float x,float y){
 web_inspection_error.clear();
 if(!web_hosted||web_paused){web_inspection_error="战场已暂停。";return 0;}
 if(!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=width||y>=height)return 0;
 if(special_choosing){web_inspection_error="请先选择武将。";return 0;}
 if(show_unit_details){web_inspection_error="请点选情报页面，或关闭情报返回战场。";return 0;}
 if(playback.active()){web_inspection_error="动画中，请等待后再长按。";return 0;}
 if(backup_busy||show_slots||game->has_dialogue()||!game->story_choices().empty()||show_inventory||show_battles||show_info||show_spells||target_item>=0||target_tactic>=0){web_inspection_error="请先完成当前对话或关闭操作窗口。";return 0;}
 const int id=unit_at_pointer(x,y);
 if(id<0){web_inspection_error="请长按战场上的武将。";return 0;}
 open_unit_details(id);return show_unit_details?1:0;
}
API void prototype_web_close_details(){if(!web_hosted)return;show_unit_details=false;equipment_slot_ui=-1;animation_clock=SDL_GetTicks();}
API const char* prototype_web_ui_state(){
 snapshot_buffer=Json{{"width",width},{"height",height},{"choosing",special_choosing},{"duel_hero",special_hulao?game->duel_hero():-1},{"duel_enemy",special_hulao?game->duel_enemy():-1},{"duel_ally",special_hulao?game->duel_ally():-1},{"duel_ally2",special_hulao?game->duel_ally2():-1},{"duel_ally3",special_hulao?game->duel_ally3():-1},{"details",show_unit_details},{"unit",show_unit_details?detail_unit:-1},{"tab",detail_tab},{"action_menu",action_menu_unit},{"hover_unit",visible_hover_panel},{"mobile_text",web_mobile_text},{"language",battle_language.english()?"en":"zh"},{"font",font_raster_scale>1?"Noto Sans CJK SC Bold":"Source Han Serif"},{"bounds",{{"x",104},{"y",206},{"w",752},{"h",710}}},{"error",tr(web_inspection_error)}}.dump();return snapshot_buffer.c_str();
}
API void prototype_web_language(int english){battle_language.english(english!=0);}
API const char* prototype_web_translate(const char* raw){snapshot_buffer=tr(raw?raw:"");return snapshot_buffer.c_str();}
API void prototype_web_mobile_text(int enabled){web_mobile_text=web_hosted&&enabled!=0;}
API void prototype_web_pause(int paused){web_paused=paused!=0;animation_clock=SDL_GetTicks();}
API int prototype_web_tool(int index){
 if(special_choosing||!web_hosted||web_paused||index<0||index>2||playback.active()||show_unit_details||show_inventory||show_slots||show_info||show_battles||show_spells||game->has_dialogue()||!game->story_choices().empty())return 0;
 if(!enabled(index))return 0;button(index);return 1;
}
API int prototype_special_start(){try{if(!special_hulao||playback.active())return 0;request_special_selection();return 1;}catch(const std::exception& e){notice=e.what();return 0;}}
API int prototype_web_start(unsigned seed){try{if(special_hulao)return prototype_special_start();choose_battle(0);game->start_scenario(0,seed?seed:1);show_slots=false;return 1;}catch(const std::exception& e){notice=e.what();return 0;}}
API int prototype_web_next(){try{if(special_hulao)throw std::runtime_error("Special battle ends at Hulao Pass.");if(playback.active())throw std::runtime_error("动画中，请等待。");cancel_jev();game->advance_campaign();clear_touch_hover();close_action_menu();show_unit_details=false;notice.clear();return 1;}catch(const std::exception& e){notice=e.what();return 0;}}
API void prototype_web_ai(int enabled){web_ai=enabled!=0;if(!web_ai){cancel_jev();game->clear_enemy_choice();}}
API int prototype_web_enemy(const char* raw){try{auto r=Json::parse(raw);if(!web_ai_pending||r.at("key")!=web_ai_key)return 0;game->choose_enemy(r.at("key"),r.at("choice"));web_ai_pending=false;web_ai_key.clear();next_enemy=SDL_GetTicks()+100;return 1;}catch(...){web_ai_pending=false;web_ai_key.clear();return 0;}}
API const char* prototype_web_progress(){snapshot_buffer=Json{{"scenario",game->scenario()},{"phase",game->state().at("phase")},{"turn",game->state().at("turn")},{"victory",game->progress().value("victory",false)},{"ready",game->state().at("phase")=="victory"&&!game->has_dialogue()&&game->story_choices().empty()&&game->progress().at("active").is_null()},{"ai_pending",web_ai_pending}}.dump();return snapshot_buffer.c_str();}
}
int main(int argc,char** argv){try{
#ifdef __EMSCRIPTEN__
 assets="/assets";special_hulao=EM_ASM_INT({return Module.specialHulao?1:0;});
#else
 int option=1;
 if(argc>1&&argv[1][0]!='-')assets=argv[option++];
 else{
  const char* base=SDL_GetBasePath();
  auto bundled=base?std::filesystem::path(base)/"assets":std::filesystem::path{};
  assets=!bundled.empty()&&std::filesystem::exists(bundled/"prototype-package.json")?bundled:std::filesystem::path("assets/prototype");
 }
#endif
 #ifndef __EMSCRIPTEN__
 special_hulao=argc>option&&(std::string(argv[option])=="--special-hulao"||std::string(argv[option])=="--special-smoke"||std::string(argv[option])=="--supplies-smoke"||std::string(argv[option])=="--camps-smoke"||std::string(argv[option])=="--squads-smoke");
 if(argc<=option||special_hulao||std::getenv("CAOCAO_PROTOTYPE_LOG_DIR")){
  try{
   const char* override=std::getenv("CAOCAO_PROTOTYPE_LOG_DIR");
   auto root=override?std::filesystem::path(override):save_directory()/"diagnostics"/"sessions-v2";
   if(!diagnostics.open(root))std::cerr<<"Local diagnostics unavailable\n";
   log_presentation({{"event","boot"},{"version","0.10.0"},{"assets",assets.string()},{"platform",SDL_GetPlatform()}});
  }catch(...){std::cerr<<"Local diagnostics unavailable\n";}
 }
 #endif
 diagnostic_stage="load_game_assets";
 bool conversations=true;
#ifndef __EMSCRIPTEN__
 // Existing animation fixtures retain their schema1/2 baseline. The dialogue
 // smoke and every ordinary launch use the complete conversation-enabled mode.
 if(argc>option&&std::string(argv[option])!="--equipment-smoke"&&std::string(argv[option])!="--revised-smoke"&&std::string(argv[option])!="--dialogue-smoke"&&std::string(argv[option])!="--scenarios-smoke"&&std::string(argv[option])!="--hover-smoke"&&std::string(argv[option])!="--menu-smoke"&&std::string(argv[option])!="--tactics-smoke"&&std::string(argv[option])!="--terrain-smoke"&&std::string(argv[option])!="--items-smoke"&&std::string(argv[option])!="--unit-details-smoke")conversations=false;
#endif
 bool balanced=true;
#ifndef __EMSCRIPTEN__
 balanced=argc<=option||(std::string(argv[option])=="--revised-smoke")||(std::string(argv[option])=="--equipment-smoke")||(std::string(argv[option])=="--jev-smoke")||(std::string(argv[option])=="--balance-smoke")||(std::string(argv[option])=="--hover-smoke")||(std::string(argv[option])=="--menu-smoke")||(std::string(argv[option])=="--tactics-smoke")||(std::string(argv[option])=="--terrain-smoke")||(std::string(argv[option])=="--items-smoke")||(std::string(argv[option])=="--unit-details-smoke");
 diagnostic_stage="load_jev_credential";
 const char* mode=std::getenv("CAOCAO_JEV_MODE");bool preload_jev=!special_hulao&&(!mode||std::string(mode)!="off");
 jev=std::make_unique<JevEnemy>(preload_jev);jev_enabled=(mode?preload_jev:balanced)&&jev->configured();
 log_presentation({{"event","jev_credential_startup"},{"status",jev->credential_status()},{"jev_enabled",jev_enabled}});
 jev_status=jev_enabled?"JEV enabled (J: local AI)":jev->configured()?"Local AI (J: JEV)":"Local AI (JEV unavailable; restart after configuring)";
 diagnostic_stage="load_game_assets";
#endif
 if(special_hulao){conversations=balanced=true;width=1536;height=1184;map_bottom=1112;inventory_button.y=unit_button.y=slots_button.y=1144;}
 game=std::make_unique<Prototype>(assets,conversations,balanced);game->enable_tactics();game->enable_terrain_benefits();game->enable_items();detail_tactics=read_json(assets/"unit-details/tactics.json");detail_metadata=read_json(assets/"unit-details/descriptions.json");font=read_json(assets/(std::filesystem::exists(assets/"unit-details/font.json")?"unit-details/font.json":std::filesystem::exists(assets/"items/font.json")?"items/font.json":std::filesystem::exists(assets/"terrain-benefits/font.json")?"terrain-benefits/font.json":std::filesystem::exists(assets/"expansion/font.json")?"expansion/font.json":"font.json"));ui=read_json(assets/"ui.json");
 battle_language.load(read_json(assets/"localization/en.json"));
 #ifdef __EMSCRIPTEN__
 game->enable_revised_rules();game->enable_equipment();manual_slots=true;
#else
 if(special_hulao||argc<=option||std::string(argv[option])=="--revised-smoke"||std::string(argv[option])=="--equipment-smoke"){game->enable_revised_rules();game->enable_equipment();manual_slots=true;}
#endif
 if(special_hulao){voxel_manifest=read_json(assets/"special-hulao/manifest.json");restart_special();special_has_match=false;special_choosing=true;}
 diagnostic_stage="initialize_renderer";
 if(!SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());
 audio_ready=SDL_InitSubSystem(SDL_INIT_AUDIO);
 int initial_width=width,initial_height=height;
#ifndef __EMSCRIPTEN__
 SDL_Rect available;
 if(SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(),&available)){
  float scale=std::min({1.f,(available.w-40.f)/width,(available.h-60.f)/height});
  initial_width=int(width*scale);initial_height=int(height*scale);
 }
#endif
 SDL_WindowFlags window_flags=SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifndef __EMSCRIPTEN__
 // Render to Retina pixels instead of downsampling Chinese glyphs to points
 // and having the window server enlarge that already damaged bitmap.
 window_flags|=SDL_WINDOW_HIGH_PIXEL_DENSITY;
 // Keep real pointer events and focus changes out of the synthetic hover run.
 if(argc>option&&(std::string(argv[option])=="--equipment-smoke"||std::string(argv[option])=="--hover-smoke"||std::string(argv[option])=="--menu-smoke"||std::string(argv[option])=="--tactics-smoke"||std::string(argv[option])=="--terrain-smoke"||std::string(argv[option])=="--items-smoke"||std::string(argv[option])=="--unit-details-smoke"))window_flags|=SDL_WINDOW_HIDDEN;
#endif
 if(!SDL_CreateWindowAndRenderer(special_hulao?"虎牢关之战 · Voxel Special":"三国志曹操传 · 四战体验版 0.10.0",initial_width,initial_height,window_flags,&window,&renderer))throw std::runtime_error(SDL_GetError());
#ifndef __EMSCRIPTEN__
 SDL_SetWindowMinimumSize(window,std::min(720,initial_width),std::min(840,initial_height));
#endif
 if(special_hulao){for(const auto& u:voxel_manifest.at("units"))for(const auto& group:u.at("frames"))for(const auto& frame:group)texture("special-hulao/"+frame.at("file").get<std::string>());load_voxel_camps();font_raster_scale=read_json(assets/"presentation-font/font-sans.json").at("raster_scale");}
 SDL_SetRenderLogicalPresentation(renderer,width,height,SDL_LOGICAL_PRESENTATION_LETTERBOX);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
#ifdef __EMSCRIPTEN__
 web_hosted=EM_ASM_INT({return Module['hostedGame']?1:0;});web_paused=web_hosted;
 if(web_hosted)font_raster_scale=read_json(assets/"presentation-font/font-sans.json").at("raster_scale").get<float>();
 EM_ASM({setTimeout(function(){window.prototypeReady?.();},0);});
 emscripten_set_main_loop(tick,0,1);
#else
 diagnostic_stage="ready";
 if(diagnostics.active())try{
  auto p=Json::parse(game->serialize()).at("payload");Json identity=Json::object();
  for(auto it=p.begin();it!=p.end();++it)if(it.key().find("manifest")!=std::string::npos||it.key()=="schema"||it.key()=="engine"||it.key()=="ruleset")identity[it.key()]=it.value();
  log_presentation({{"event","session"},{"version","0.10.0"},{"renderer",SDL_GetRendererName(renderer)},{"audio_ready",audio_ready},{"jev_enabled",jev_enabled},{"initial_hash",game->hash()},{"identity",identity}});
  diagnostic_checkpoint("startup");
 }catch(...){}
 if(argc>option&&std::string(argv[option])=="--revised-smoke"){if(argc<option+2)throw std::runtime_error("output required");revised_smoke(argv[option+1]);running=false;}
 if(argc>option&&std::string(argv[option])=="--equipment-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated saves and output required");equipment_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--unit-details-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated saves and output required");unit_details_smoke(argv[option+1]);running=false;
 }else if(argc>option&&std::string(argv[option])=="--items-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated saves and output required");items_smoke(argv[option+1]);running=false;
 }else if(argc>option&&std::string(argv[option])=="--terrain-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated saves and output required");terrain_smoke(argv[option+1]);running=false;
 }else if(argc>option&&std::string(argv[option])=="--tactics-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated saves and output required");tactics_smoke(argv[option+1]);running=false;
 }else if(argc>option&&std::string(argv[option])=="--menu-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save dir and menu output dir required");
  menu_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--hover-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save dir and hover output dir required");
  hover_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&(std::string(argv[option])=="--jev-smoke"||std::string(argv[option])=="--balance-smoke")){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated save dir and smoke output dir required");
  bool network=std::string(argv[option])=="--jev-smoke";jev_enabled=network;std::filesystem::path out=argv[option+1];std::filesystem::create_directories(out);
  if(!network){for(int n=0;n<4;++n){choose_battle(n);while(game->has_dialogue())game->command("continue");draw(out/("battle-"+std::to_string(n)+".bmp"));if(n==2){show_info=true;draw(out/"information.bmp");show_info=false;}}}
  else {
   choose_battle(argc>option+2?std::stoi(argv[option+2]):2);while(game->has_dialogue())game->command("continue");game->command("end_turn");auto before=game->state();auto until=SDL_GetTicks()+25000;
   while(last_jev.is_null()&&SDL_GetTicks()<until){if(game->has_dialogue())game->command("continue");else if(game->state().at("phase")=="player")game->command("end_turn");tick();SDL_Delay(16);}
   if(last_jev.is_null())throw std::runtime_error("JEV scheduler stalled");
   auto planned=game->serialize();auto cursor=game->state().at("enemy_cursor");Prototype expected=*game;expected.restore(planned);expected.enemy_step();
   while(game->state().at("enemy_cursor")==cursor&&SDL_GetTicks()<until){tick();SDL_Delay(16);}
   if(game->hash()!=expected.hash())throw std::runtime_error("JEV action/replay/presentation mismatch");
   draw(out/"jev-action.bmp");auto first=game->hash();while(playback.active()){bool moving=playback.moving();playback.advance(40);if(playback.active()&&moving!=playback.moving())begin_presentation();draw();}
   std::ofstream(out/"result.json")<<Json{{"decision",last_jev},{"request_state",before},{"planned_snapshot",planned},{"next_hash",first},{"restored_next_hash",expected.hash()}}.dump(2);
   std::cout<<last_jev.dump()<<'\n';
  }
  running=false;
 }
 if(argc>option&&std::string(argv[option])=="--resume-smoke"){
  persist(true);if(notice!="已读取存档。")throw std::runtime_error(notice);draw();
  std::cout<<"fresh-process load passed "<<game->hash()<<'\n';running=false;
 }
 if(argc>option&&std::string(argv[option])=="--scenarios-combat-smoke"){
  if(argc<option+2)throw std::runtime_error("combat smoke directory required");scenarios_combat_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--scenarios-resume-smoke"){
  if(argc<option+2||!std::getenv("CAOCAO_PROTOTYPE_SAVE_DIR"))throw std::runtime_error("isolated scenario save directory and expected hashes required");
  auto expected=read_json(std::filesystem::path(argv[option+1])/"saved-hashes.json");
  for(int n=1;n<4;++n){choose_battle(n);persist(true);if(notice!="已读取存档。"||game->hash()!=expected.at(std::to_string(n)).get<std::string>())throw std::runtime_error("fresh process scenario restore failed");draw();}
  std::cout<<"fresh-process separate battle saves passed\n";running=false;
 }
 if(argc>option&&std::string(argv[option])=="--scenarios-smoke"){
  if(argc<option+2)throw std::runtime_error("scenario smoke output directory required");scenarios_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--dialogue-smoke"){
  if(argc<option+2)throw std::runtime_error("dialogue smoke output directory required");dialogue_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--movement-smoke"){
  if(argc<option+2)throw std::runtime_error("movement smoke output directory required");movement_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--mixed-enemy-animation-smoke"){
  if(argc<option+2)throw std::runtime_error("enemy smoke output directory required");enemy_animation_smoke(argv[option+1],16,true);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--enemy-animation-slow-smoke"){
  if(argc<option+2)throw std::runtime_error("enemy smoke output directory required");enemy_animation_smoke(argv[option+1],120);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--enemy-animation-smoke"){
  if(argc<option+2)throw std::runtime_error("enemy smoke output directory required");enemy_animation_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--attack-smoke"){
  if(argc<option+2)throw std::runtime_error("attack smoke output directory required");attack_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--smoke"){
  if(argc<option+2)throw std::runtime_error("smoke output directory required");render_smoke(argv[option+1]);running=false;
 }
 if(argc>option&&std::string(argv[option])=="--special-smoke"){if(argc<option+2)throw std::runtime_error("output required");special_smoke(argv[option+1]);running=false;}
 if(argc>option&&std::string(argv[option])=="--supplies-smoke"){if(argc<option+2)throw std::runtime_error("output required");special_supplies_smoke(argv[option+1]);running=false;}
 if(argc>option&&std::string(argv[option])=="--squads-smoke"){if(argc<option+2)throw std::runtime_error("output required");special_squads_smoke(argv[option+1]);running=false;}
 if(argc>option&&std::string(argv[option])=="--camps-smoke"){if(argc<option+2)throw std::runtime_error("output required");special_camps_smoke(argv[option+1]);running=false;}
 if(argc>option&&running&&!special_hulao)throw std::runtime_error("unknown prototype option: "+std::string(argv[option]));
 while(running){tick();SDL_Delay(16);}
 diagnostic_stage="shutdown";cancel_jev();diagnostic_checkpoint("clean_exit");diagnostics.close(0);
 jev.reset();
 stop_attack_sounds();
 for(auto [_,t]:textures)SDL_DestroyTexture(t);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
#endif
 return 0;
}catch(const std::exception& e){
 // Finder does not expose stderr. Keep the exact exception and committed
 // snapshot in the session log without changing any manual save slot.
 try{log_presentation({{"event","fatal_error"},{"message",e.what()},{"exit_code",1}});}catch(...){}
 #ifndef __EMSCRIPTEN__
 try{if(game)diagnostics.failure_snapshot(game->serialize());}catch(...){}
 diagnostics.close(1);
 #endif
 std::cerr<<e.what()<<'\n';return 1;
}}
