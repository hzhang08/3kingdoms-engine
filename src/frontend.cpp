#include "caocao/engine.hpp"
#include "caocao/battle.hpp"
#include "caocao/terrain_effect.hpp"
#include "caocao/selection.hpp"
#include "caocao/attack_animation.hpp"
#include "caocao/tactic_effect.hpp"
#include "caocao/mcall.hpp"
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif
using caocao::Json;
namespace {
std::unique_ptr<caocao::Engine> engine;
SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
SDL_Texture* scene_target=nullptr;
std::filesystem::path assets;
Json font,music,ui_assets,banner_fonts;
std::map<int,std::vector<uint8_t>> effect_raw;
std::map<int,Json> effect_meta;
std::unordered_map<uint32_t,uint8_t> palette_indices;
bool dark_textures=false;
std::unordered_map<uint32_t,uint32_t> dark_colors;
std::map<std::string,SDL_Texture*> textures;
SDL_AudioStream* music_stream=nullptr;
std::vector<SDL_AudioStream*> effects;
std::vector<uint8_t> looping_effect_pcm;
std::string last_error,observed;uint64_t due=0;bool running=true,started=false;int slot=0;uint64_t frame_count=0;
std::vector<uint32_t> codepoints(const std::string& s){std::vector<uint32_t> out;for(size_t n=0;n<s.size();){uint8_t c=s[n++];uint32_t v=c;int more=0;if((c&0xe0)==0xc0){v=c&31;more=1;}else if((c&0xf0)==0xe0){v=c&15;more=2;}else if((c&0xf8)==0xf0){v=c&7;more=3;}while(more--&&n<s.size())v=(v<<6)|(static_cast<uint8_t>(s[n++])&63);out.push_back(v);}return out;}
SDL_Texture* texture(const std::string& relative){
 auto key=(dark_textures?"dark:":"")+relative;
 if(textures.contains(key))return textures[key];auto path=assets/relative;SDL_Surface* s=SDL_LoadBMP(path.string().c_str());if(!s)throw std::runtime_error("Image "+relative+": "+SDL_GetError());
 if(dark_textures){
  auto* converted=SDL_ConvertSurface(s,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(s);s=converted;if(!s)throw std::runtime_error(SDL_GetError());
  for(int y=0;y<s->h;++y)for(int x=0;x<s->w;++x){auto* p=static_cast<uint8_t*>(s->pixels)+y*s->pitch+x*4;uint32_t rgb=(p[0]<<16)|(p[1]<<8)|p[2];auto it=dark_colors.find(rgb);
   if(it!=dark_colors.end()){auto color=it->second;p[0]=color>>16;p[1]=color>>8;p[2]=color;}
  }
 }
 SDL_SetSurfaceColorKey(s,true,SDL_MapSurfaceRGB(s,247,0,255));auto* t=SDL_CreateTextureFromSurface(renderer,s);SDL_DestroySurface(s);if(!t)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST);textures[key]=t;return t;
}
void fill(float x,float y,float w,float h,int r,int g,int b,int a=255){SDL_SetRenderDrawColor(renderer,r,g,b,a);SDL_FRect rect{x,y,w,h};SDL_RenderFillRect(renderer,&rect);}
void text(const std::string& s,float x,float y,float max=620,SDL_Color color={247,238,218,255}){
 auto* t=texture("images/font.bmp");SDL_SetTextureColorMod(t,color.r,color.g,color.b);float start=x;int cell=font.at("cell");
 for(auto cp:codepoints(s)){if(cp==10){x=start;y+=23;continue;}auto key=std::to_string(cp);if(!font.at("glyphs").contains(key))key="63";auto g=font.at("glyphs").at(key);float advance=g.at("advance");if(x+advance>max){x=start;y+=23;}SDL_FRect from{g.at("x").get<float>(),g.at("y").get<float>(),float(cell),float(cell)},to{x,y,float(cell),float(cell)};SDL_RenderTexture(renderer,t,&from,&to);x+=advance;}
}
std::string number3(int n){std::ostringstream s;s<<std::setw(3)<<std::setfill('0')<<n;return s.str();}
void draw_image(const std::string& name,float x,float y,float w,float h,SDL_FRect* source=nullptr,bool flip=false){SDL_FRect d{x,y,w,h};SDL_RenderTextureRotated(renderer,texture(name),source,&d,0,nullptr,flip?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);}
void draw_combat_frame(const std::string& archive,const Json& pose,float x,float y){
 int size=pose.at("width"),frame=pose.at("frame"),sprite=pose.at("sprite");const auto& effects=pose.at("effects");
 auto key=std::string(dark_textures?"combat-dark:":"combat:")+archive+":"+std::to_string(sprite)+":"+std::to_string(frame)+effects.dump();
 if(!textures.contains(key)){
  std::ifstream f(assets/"raw"/(archive+".e5")/(number3(sprite)+".bin"),std::ios::binary);
  std::vector<uint8_t> pixels(size*size);f.seekg(frame*size*size);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());if(!f)throw std::runtime_error("missing indexed combat frame");
  pixels=caocao::transform_combat_pixels(std::move(pixels),effects,ui_assets.at("combat_palette").at("quantized"),ui_assets.at("combat_palette").at("lookup"));
  auto* surface=SDL_CreateSurface(size,size,SDL_PIXELFORMAT_RGBA32);if(!surface)throw std::runtime_error(SDL_GetError());
  for(int yy=0;yy<size;++yy)for(int xx=0;xx<size;++xx){int index=pixels[yy*size+xx];if(dark_textures&&index)index=ui_assets.at("darken").at(index);
   const auto& color=ui_assets.at("palette").at(index);auto* p=static_cast<uint8_t*>(surface->pixels)+yy*surface->pitch+xx*4;
   p[0]=color[0];p[1]=color[1];p[2]=color[2];p[3]=index?255:0;
  }
  auto* tex=SDL_CreateTextureFromSurface(renderer,surface);SDL_DestroySurface(surface);if(!tex)throw std::runtime_error(SDL_GetError());
  SDL_SetTextureScaleMode(tex,SDL_SCALEMODE_NEAREST);SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);textures[key]=tex;
 }
 SDL_FRect to{x,y,float(size),float(size)};SDL_RenderTextureRotated(renderer,textures.at(key),nullptr,&to,0,nullptr,pose.at("mirror").get<bool>()?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);
}
void draw_masked_frame(const std::string& archive,const Json& pose,float x,float y){
 int width=pose.at("width"),height=pose.at("height"),frame=pose.at("frame"),sprite=pose.at("sprite"),mask=pose.at("mask_offset");
 auto key=std::string(dark_textures?"masked-dark:":"masked:")+archive+pose.dump();
 if(!textures.contains(key)){
  std::ifstream f(assets/"raw"/(archive+".e5")/(number3(sprite)+".bin"),std::ios::binary);
  std::vector<uint8_t> pixels(width*height);f.seekg(frame*width*width);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());if(!f)throw std::runtime_error("missing cropped sprite frame");
  std::ifstream mf(assets/"raw/Mark.e5/000.bin",std::ios::binary);std::vector<uint8_t> mark{std::istreambuf_iterator<char>(mf),{}};
  pixels=caocao::mask_battle_sprite(std::move(pixels),mask,mark);
  auto* surface=SDL_CreateSurface(width,height,SDL_PIXELFORMAT_RGBA32);if(!surface)throw std::runtime_error(SDL_GetError());
  for(int yy=0;yy<height;++yy)for(int xx=0;xx<width;++xx){int index=pixels[yy*width+xx];bool visible=index!=0;if(dark_textures&&visible)index=ui_assets.at("darken").at(index);
   const auto& color=ui_assets.at("palette").at(index);auto* p=static_cast<uint8_t*>(surface->pixels)+yy*surface->pitch+xx*4;
   p[0]=color[0];p[1]=color[1];p[2]=color[2];p[3]=visible?255:0;
  }
  auto* tex=SDL_CreateTextureFromSurface(renderer,surface);SDL_DestroySurface(surface);if(!tex)throw std::runtime_error(SDL_GetError());
  SDL_SetTextureScaleMode(tex,SDL_SCALEMODE_NEAREST);SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);textures[key]=tex;
 }
 SDL_FRect to{x,y,float(width),float(height)};SDL_RenderTexture(renderer,textures.at(key),nullptr,&to);
}
void play_audio(const std::string& file,bool is_music,bool loop=false){
 if(is_music&&music_stream){SDL_DestroyAudioStream(music_stream);music_stream=nullptr;}
 SDL_AudioSpec spec{};void* pcm=nullptr;uint64_t frames=0;Uint32 bytes=0;bool mp3=file.ends_with(".mp3");
 if(mp3){drmp3_config config{};drmp3_uint64 count=0;pcm=drmp3_open_file_and_read_pcm_frames_f32((assets/file).string().c_str(),&config,&count,nullptr);if(!pcm){last_error="Unable to decode "+file;return;}spec={SDL_AUDIO_F32,int(config.channels),int(config.sampleRate)};frames=count;bytes=static_cast<Uint32>(frames*config.channels*sizeof(float));}
 else{Uint8* buffer=nullptr;if(!SDL_LoadWAV((assets/file).string().c_str(),&spec,&buffer,&bytes)){last_error="Unable to decode "+file;return;}pcm=buffer;}
 auto* stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);if(stream){SDL_PutAudioStreamData(stream,pcm,bytes);SDL_ResumeAudioStreamDevice(stream);if(is_music)music_stream=stream;else{effects.push_back(stream);if(loop)looping_effect_pcm.assign(static_cast<uint8_t*>(pcm),static_cast<uint8_t*>(pcm)+bytes);}}else last_error=SDL_GetError();
 if(mp3)drmp3_free(pcm,nullptr);else SDL_free(pcm);
}
void observe(){
 const auto& s=engine->state();auto signature=std::to_string(s.program)+":"+std::to_string(s.pc)+s.presentation.dump();if(signature==observed)return;observed=signature;due=SDL_GetTicks()+s.presentation.value("duration_ms",120);
 auto type=s.presentation.value("type","");if(type=="delay")due=SDL_GetTicks()+s.presentation.value("duration_units",1)*100;
 if(type=="move")due=SDL_GetTicks()+450;
 if(type=="music"){
  if(s.music==255){if(music_stream)SDL_DestroyAudioStream(music_stream);music_stream=nullptr;}
  else{auto key=std::to_string(s.music+1);if(music.contains(key))play_audio(music.at(key),true);else last_error="Unresolved music ID "+std::to_string(s.music);}
 }
 if(type=="sound"){
  int id=s.presentation.value("id",-1),repeat=s.presentation.value("repeat",0);
  for(auto* p:effects)SDL_DestroyAudioStream(p);effects.clear();looping_effect_pcm.clear();
  if(repeat!=255){try{play_audio(caocao::battle_sound_file(id),false,repeat==0);}catch(const std::exception& e){last_error=e.what();}}
 }
}
void resume_audio(){for(auto* p:effects)SDL_DestroyAudioStream(p);effects.clear();looping_effect_pcm.clear();int loop=engine->state().script_fields.value("looping_sound",-1);if(loop>=0)play_audio(caocao::battle_sound_file(loop),false,true);if(engine->state().music>=0&&engine->state().music!=255){auto key=std::to_string(engine->state().music+1);if(music.contains(key))play_audio(music.at(key),true);}}
void send(const caocao::Command& cmd){try{engine->submit(cmd);last_error.clear();observe();}catch(const std::exception&e){last_error=e.what();}}
std::filesystem::path save_dir(){const char* xdg=std::getenv("XDG_DATA_HOME");std::filesystem::path p;if(xdg)p=xdg;else{const char* home=std::getenv("HOME");if(!home)throw std::runtime_error("HOME unavailable");p=std::filesystem::path(home)/".local/share";}p/="caocao-engine";std::filesystem::create_directories(p);return p;}
void native_save(bool load){try{auto p=save_dir()/("slot-"+std::to_string(slot+1)+".json");if(load){std::ifstream f(p);if(!f)throw std::runtime_error("Empty slot");std::string s((std::istreambuf_iterator<char>(f)),{});engine->restore(s);started=true;resume_audio();}else{auto blob=engine->serialize();auto tmp=p;tmp+=".pending";{std::ofstream f(tmp,std::ios::binary);f<<blob;f.flush();if(!f)throw std::runtime_error("Snapshot write failed");}std::filesystem::rename(tmp,p);}last_error=load?"Loaded":"Saved";}catch(const std::exception&e){last_error=e.what();}}
SDL_Color palette_color(int index){const auto& p=ui_assets.at("palette").at(index);return {p[0].get<Uint8>(),p[1].get<Uint8>(),p[2].get<Uint8>(),255};}
void indexed_fill(int x,int y,int w,int h,int index){auto c=palette_color(index);fill(x,y,w,h,c.r,c.g,c.b);}
void battlefield(const Json& selection,int idle_tick,const Json& banner=Json::object()){const auto& s=engine->state();
  SDL_FRect crop{float(s.camera_x),float(s.camera_y),caocao::battle_view_width,caocao::battle_view_height};draw_image("images/battlefield.bmp",0,caocao::battle_view_top,caocao::battle_view_width,caocao::battle_view_height,&crop);
  for(int y=0;y<8;++y)for(int x=0;x<13;++x){int at=(y+s.camera_y/48)*20+x+s.camera_x/48;
   auto cursor=selection.empty()?std::array<int,2>{255,255}:selection.at("cursor").get<std::array<int,2>>();cursor[0]-=s.camera_x/48;cursor[1]-=s.camera_y/48;
   int terrain_offset=s.terrain_marks.at(at)==0?caocao::fire_frame_offset(idle_tick):s.terrain_marks.at(at)==3?0x1ec0:-1;
   auto layers=caocao::selection_tile_layers({x,y},cursor,0,!selection.empty()&&selection.at("movement").get<bool>(),!selection.empty()&&selection.at("distance").at(at)!=255,!selection.empty()&&selection.at("range").at(at)!=0,false,255,!selection.empty()&&selection.value("blue_movement",false),terrain_offset);
   for(const auto& layer:layers){auto kind=layer.at("kind");
    if(kind=="checker"){auto c=ui_assets.at("palette").at(layer.at("color").get<int>());if(dark_textures)c=ui_assets.at("palette").at(ui_assets.at("darken").at(layer.at("color").get<int>()).get<int>());
     for(int py=0;py<48;++py)for(int px=py%2;px<48;px+=2)fill(x*48+px,56+y*48+py,1,1,c[0],c[1],c[2]);
    }else if(kind=="mark"){int offset=layer.at("offset");
     if(offset>=0x8bc0){SDL_FRect frame{0,float((offset-0x8bc0)/48),48,48};draw_image("images/Mark/fire.bmp",x*48,56+y*48,48,48,&frame);}
     else draw_image(std::string("images/Mark/")+(offset==0xcc0?"range":offset==0x1ec0?"cursor":"cursor-neighbor")+".bmp",x*48,56+y*48,48,48);
    }
   }
  }
  if(!banner.empty())for(int row=0;row<8;++row)for(int col=0;col<13;++col){int color=banner.contains("cells")?banner.at("cells").at(row*13+col):banner.contains("rows")?banner.at("rows").at(row):banner.at("columns").at(col);if(color==255)continue;auto c=palette_color(color);
   for(int y=0;y<48;++y)for(int x=y%2;x<48;x+=2)fill(col*48+x,56+row*48+y,1,1,c.r,c.g,c.b);
  }
  for(const auto& u:s.units){if(!u.contains("battle"))continue;auto b=caocao::BattleUnit::from_json(u.at("battle"));if((!s.pending_action.is_null()&&s.pending_action.value("hidden",false)&&s.pending_action.at("unit")==b.id)||b.lifecycle!=2||s.battle_poses.contains(std::to_string(b.id)))continue;
   auto record=caocao::UnitRecord::from_json(u);int sprite=caocao::battle_sprite(b.id,record.job,b.side);
   // Frame sequence is recovered from 0x43e572; the wall-clock cadence is
   // provisional until an original gameplay capture establishes its timing.
   int frame=caocao::battle_frame(b,std::min(500,record.hp+caocao::equipment_bonus(record,5,engine->data())),idle_tick);
   SDL_FRect from{0,float(frame*48),48,48};draw_image("images/Unit_mov/"+number3(sprite)+".bmp",b.x*48-s.camera_x,b.y*48-s.camera_y+caocao::battle_view_top,48,48,&from,(b.flags&8)!=0&&(frame==4||frame==5||frame==8));
  }
  std::vector<std::string> pose_order;for(auto it=s.battle_poses.begin();it!=s.battle_poses.end();++it)pose_order.push_back(it.key());
  std::stable_sort(pose_order.begin(),pose_order.end(),[&](const auto& a,const auto& b){return s.battle_poses.at(a).value("order",0)<s.battle_poses.at(b).value("order",0);});
  for(const auto& key:pose_order){
   const auto& pose=s.battle_poses.at(key);auto b=caocao::BattleUnit::from_json(s.units.at(key).at("battle"));
   int size=pose.at("width"),frame=pose.at("frame"),sprite=pose.at("sprite"),offset=(size-48)/2;
   std::string archive=pose.at("archive");archive=archive=="UNIT_ATK.E5"?"Unit_atk":archive=="UNIT_SPC.E5"?"Unit_spc":"Unit_mov";
   SDL_FRect from{0,float(frame*size),float(size),float(size)};
   float px=pose.value("screen_x",b.x*48-s.camera_x-offset),py=pose.value("screen_y",b.y*48-s.camera_y+caocao::battle_view_top-offset);
   if(pose.contains("mask_offset"))draw_masked_frame(archive,pose,px,py);
   else if(pose.contains("effects")&&!pose.at("effects").empty())draw_combat_frame(archive,pose,px,py);
   else draw_image("images/"+archive+"/"+number3(sprite)+".bmp",px,py,size,size,&from,pose.at("mirror"));
   for(const auto& overlay:pose.value("overlays",Json::array()))for(const auto& u:s.units)if(u.contains("battle")&&u.at("battle").at("slot")==overlay.at("slot")){
    auto other=caocao::BattleUnit::from_json(u.at("battle"));auto record=caocao::UnitRecord::from_json(u);
    int frame=caocao::battle_frame(other,std::min(500,record.hp+caocao::equipment_bonus(record,5,engine->data())),idle_tick);SDL_FRect crop{0,float(frame*48),48,48};
    draw_image("images/Unit_mov/"+number3(caocao::battle_sprite(other.id,record.job,other.side))+".bmp",overlay.at("x"),overlay.at("y"),48,48,&crop,(other.flags&8)!=0&&(frame==4||frame==5||frame==8));
   }
  }

}
void unit_panel(const Json& panel,const Json& underlay){
 int pattern=128;std::array<int,2> captured_tile{-1,-1};
 for(const auto& event:panel){auto kind=event.at("kind");auto a=event.value("args",Json::array());
  if(kind=="tile_capture")captured_tile=event.at("tile").get<std::array<int,2>>();
  else if(kind=="terrain_tile"){
   if(captured_tile[0]<0)throw std::runtime_error("terrain preview without capture");
   SDL_FRect crop{float(captured_tile[0]*48),float(captured_tile[1]*48),48,48};
   draw_image("images/battlefield.bmp",a.at(0),a.at(1),48,48,&crop);
  }else if(kind=="gray_icon")draw_image("images/Mark/tactic-"+std::to_string(event.at("icon_index").get<int>())+"-gray.bmp",a.at(0),a.at(1),16,16);
  else if(kind=="window"){
   int x=a.at(0).get<int>()*48,y=56+a.at(1).get<int>()*48,w=a.at(2).get<int>()*48,h=a.at(3).get<int>()*48;
   SDL_Rect clip{x,y,w,h};SDL_SetRenderClipRect(renderer,&clip);dark_textures=true;battlefield(underlay,underlay.at("tick"));dark_textures=false;
   SDL_SetRenderClipRect(renderer,nullptr);
   for(int n=0;n<3;++n){indexed_fill(x,y+h-3+n,w,1,29+n);indexed_fill(x+w-3+n,y,1,h,29+n);if(!n){indexed_fill(x,y,w,1,24);indexed_fill(x,y,1,h,24);}}
  }else if(kind=="text"){
   for(const auto& glyph:event.at("glyphs")){std::string ch=glyph.at(2);int x=glyph.at(0),y=glyph.at(1),width=static_cast<unsigned char>(ch[0])<128?8:16;
    if(event.at("opaque").get<bool>())indexed_fill(x,y,width,16,event.at("background"));
    text(ch,x,y,640,palette_color(event.at("foreground")));
   }
  }else if(kind=="bar_background")indexed_fill(a.at(0),a.at(1),a.at(2),a.at(3),a.at(5));
  else if(kind=="pattern")pattern=a.at(2);
  else if(kind=="bar_fill"){
   int x=a.at(0),y=a.at(1),end=a.at(2);for(int at=x;at<=end;at+=8){int w=std::min(8,end-at+1);SDL_FRect src{0,0,float(w),8};draw_image(std::string("images/Mark/")+(pattern==128?"hp-pattern":pattern==64?"mp-pattern":"loss-pattern")+".bmp",at,y,w,8,&src);}
  }else if(kind=="icon"){
   int offset=a.at(4);
   if(offset>=0x1a5c0&&offset<=0x1a8c0&&(offset-0x1a5c0)%256==0)draw_image("images/Mark/tactic-"+std::to_string((offset-0x1a5c0)/256)+".bmp",a.at(0),a.at(1),16,16);
   else if(offset==384||offset==960)draw_image(std::string("images/Mark/")+(offset==384?"hp":"mp")+".bmp",a.at(0),a.at(1),24,24);
   else throw std::runtime_error("unknown panel icon offset");
  }
  else throw std::runtime_error("unhandled unit panel raster request");
 }
}
void banner_text(const Json& banner){
 for(const auto& event:banner.at("text")){
  int size=event.at("height");const auto& atlas=banner_fonts.at(std::to_string(size));auto glyphs=codepoints(event.at("text"));int width=0;
  for(auto cp:glyphs)width+=atlas.at("glyphs").at(std::to_string(cp)).at("advance").get<int>();
  const auto& r=event.at("rect");float x=event.at("center")==0?r.at(0).get<int>():(r.at(0).get<int>()+r.at(2).get<int>()-width)/2,y=(r.at(1).get<int>()+r.at(3).get<int>()-size)/2;
  auto* t=texture("images/banner-font-"+std::to_string(size)+".bmp");auto c=palette_color(event.at("color"));SDL_SetTextureColorMod(t,c.r,c.g,c.b);
  for(auto cp:glyphs){const auto& g=atlas.at("glyphs").at(std::to_string(cp));float cell=atlas.at("cell");SDL_FRect from{g.at("x"),g.at("y"),cell,cell},to{x,y,cell,cell};
   // Clearing the original banner repaints one column at a time, including
   // the text pixels over that column. Keep the remaining title visible.
   if(banner.contains("cells")){
    for(int row=0;row<8;++row)for(int col=0;col<13;++col)if(banner.at("cells").at(row*13+col)!=255){SDL_Rect clip{col*48,56+row*48,48,48};SDL_SetRenderClipRect(renderer,&clip);SDL_RenderTexture(renderer,t,&from,&to);}
   }else if(banner.contains("rows")){
    for(int row=0;row<8;++row)if(banner.at("rows").at(row)!=255){SDL_Rect clip{0,56+row*48,624,48};SDL_SetRenderClipRect(renderer,&clip);SDL_RenderTexture(renderer,t,&from,&to);}
   }else for(int col=0;col<13;++col)if(banner.at("columns").at(col)!=255){SDL_Rect clip{col*48,56,48,384};SDL_SetRenderClipRect(renderer,&clip);SDL_RenderTexture(renderer,t,&from,&to);}
   x+=g.at("advance").get<float>();
  }
 }
 SDL_SetRenderClipRect(renderer,nullptr);
}
void tactic_caption(const Json& events){
 for(const auto& e:events){auto kind=e.at("kind");
  if(kind=="caption_tile")draw_image("images/Mark/caption.bmp",e.at("x"),e.at("y"),48,48);
  else if(kind=="caption_line"){
   const auto& a=e.at("args");int x1=a.at(0),y1=a.at(1),x2=a.at(2),y2=a.at(3);
   if(x1!=x2&&y1!=y2)throw std::runtime_error("unverified caption border line");
   indexed_fill(std::min(x1,x2),std::min(y1,y2),std::abs(x2-x1)+1,std::abs(y2-y1)+1,e.at("color"));
  }else if(kind=="caption_text"){
   for(const auto& glyph:e.at("glyphs"))text(glyph.at(2),glyph.at(0),glyph.at(1),640,palette_color(e.at("foreground")));
  }else throw std::runtime_error("unverified caption draw request");
 }
}
void poison_effect(const Json& effect){
 // Capture at the original logical pixel resolution. Matching palette RGB
 // back to indices preserves the original integer compositor in both SDL
 // backends; ordinary alpha blending would produce different colors.
 const auto& r=effect.at("rectangle");int x=r[0],y=r[1],w=r[2],h=r[3];
 int left=std::max(0,x),top=std::max(56,y),right=std::min(624,x+w),bottom=std::min(440,y+h);
 if(left>=right||top>=bottom)return;
 SDL_Rect clipped{left,top,right-left,bottom-top};
 auto* capture=SDL_RenderReadPixels(renderer,&clipped);if(!capture)throw std::runtime_error(SDL_GetError());
 auto* rgba=SDL_ConvertSurface(capture,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(capture);if(!rgba)throw std::runtime_error(SDL_GetError());
 std::vector<uint8_t> background(w*h,0),source(w*h);
 for(int yy=0;yy<rgba->h;++yy)for(int xx=0;xx<rgba->w;++xx){auto* p=static_cast<uint8_t*>(rgba->pixels)+yy*rgba->pitch+xx*4;uint32_t key=(p[0]<<16)|(p[1]<<8)|p[2];
  if(!palette_indices.contains(key)){SDL_DestroySurface(rgba);throw std::runtime_error("poison background contains a non-palette pixel");}
  background[(yy+top-y)*w+xx+left-x]=palette_indices.at(key);
 }
 SDL_DestroySurface(rgba);
 int resource=effect.value("resource",23);const auto& raw=effect_raw.at(resource);
 int offset=effect_meta.at(resource).at("frames").at(effect.at("frame").get<int>()).at("pixel_offset");
 if(offset<0||offset+source.size()>raw.size())throw std::runtime_error("missing poison frame");
 std::copy_n(raw.begin()+offset,source.size(),source.begin());
 std::vector<uint8_t> pixels;
 if(effect.value("direct",false))pixels=caocao::direct_meff_pixels(source,background);
 else if(effect.value("masked",false)){
  int mask_offset=effect_meta.at(resource).at("frames").at(effect.at("frame").get<int>()).at("mask_offset");
  if(mask_offset<0||mask_offset+source.size()>raw.size())throw std::runtime_error("missing MCALL mask");
  std::vector<uint8_t> mask(raw.begin()+mask_offset,raw.begin()+mask_offset+source.size());
  pixels=caocao::blend_mcall_pixels(source,mask,background,ui_assets.at("combat_palette").at("quantized"),ui_assets.at("combat_palette").at("lookup"),effect.at("amount"));
 }else pixels=caocao::blend_poison_pixels(source,background,ui_assets.at("combat_palette").at("quantized"),ui_assets.at("combat_palette").at("lookup"),effect.at("weight"),effect.at("amount"));
 auto* surface=SDL_CreateSurface(w,h,SDL_PIXELFORMAT_RGBA32);if(!surface)throw std::runtime_error(SDL_GetError());
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){auto c=palette_color(pixels[yy*w+xx]);auto* p=static_cast<uint8_t*>(surface->pixels)+yy*surface->pitch+xx*4;p[0]=c.r;p[1]=c.g;p[2]=c.b;p[3]=255;}
 auto* t=SDL_CreateTextureFromSurface(renderer,surface);SDL_DestroySurface(surface);if(!t)throw std::runtime_error(SDL_GetError());
 SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST);SDL_FRect dest{float(x),float(y),float(w),float(h)};SDL_RenderTexture(renderer,t,nullptr,&dest);SDL_DestroyTexture(t);
}
// The original dialog is specified in dialog units. This prototype uses the
// existing 16px atlas (2px per DLU); original Windows font metrics, disabled
// embossing, button bitmap decorations and drag behavior still need comparison.
std::array<int,2> tutorial_menu_origin(){const auto& s=engine->state();const auto& p=s.presentation;const auto& bounds=p.at("dialog").at("bounds_dlu");return caocao::command_menu_position(p.at("cursor").get<std::array<int,2>>(),{s.camera_x,s.camera_y},{bounds.at(2).get<int>()*2,bounds.at(3).get<int>()*2});}
void draw_tutorial_menu(){const auto& p=engine->state().presentation;auto origin=tutorial_menu_origin();const auto& bounds=p.at("dialog").at("bounds_dlu");fill(origin[0],origin[1],bounds.at(2).get<int>()*2,bounds.at(3).get<int>()*2,212,208,200);
 for(const auto& c:p.at("controls")){const auto& b=c.at("bounds_dlu");int x=origin[0]+b.at(0).get<int>()*2,y=origin[1]+b.at(1).get<int>()*2,w=b.at(2).get<int>()*2,h=b.at(3).get<int>()*2;
  fill(x,y,w,h,80,80,80);fill(x,y,w-1,h-1,255,255,255);fill(x+1,y+1,w-3,h-3,212,208,200);
  if(c.at("id")==engine->state().presentation.at("focused_control")){for(int n=3;n<w-3;n+=2){fill(x+n,y+3,1,1,30,30,30);fill(x+n,y+h-5,1,1,30,30,30);}for(int n=3;n<h-4;n+=2){fill(x+3,y+n,1,1,30,30,30);fill(x+w-5,y+n,1,1,30,30,30);}}
  text(c.at("text"),x+(w-32)/2,y+(h-16)/2,x+w,c.at("enabled").get<bool>()?SDL_Color{0,0,0,255}:SDL_Color{128,128,128,255});
 }
}
void tutorial_menu_key(SDL_Keycode key,bool reverse){
 if(key==SDLK_ESCAPE){send({"tutorial_command",255});return;}
 const int enabled[]={2,0x508,0x50b};int selected=0;while(selected<2&&enabled[selected]!=engine->state().presentation.at("focused_control"))++selected;
 if(key==SDLK_UP||key==SDLK_DOWN||key==SDLK_TAB){int delta=key==SDLK_UP||(key==SDLK_TAB&&reverse)?-1:1;send({"tutorial_menu_focus",enabled[(selected+delta+3)%3]});}
 else if(key==SDLK_RETURN||key==SDLK_SPACE){for(const auto& c:engine->state().presentation.at("controls"))if(c.at("id")==engine->state().presentation.at("focused_control")){int value=c.at("result");send({"tutorial_command",value});break;}}
}
void tutorial_menu_click(float x,float y){auto origin=tutorial_menu_origin();for(const auto& c:engine->state().presentation.at("controls")){const auto& b=c.at("bounds_dlu");int left=origin[0]+b.at(0).get<int>()*2,top=origin[1]+b.at(1).get<int>()*2;
 if(c.at("enabled").get<bool>()&&x>=left&&x<left+b.at(2).get<int>()*2&&y>=top&&y<top+b.at(3).get<int>()*2){int value=c.at("result");send({"tutorial_command",value});break;}}
}
void render(){
 fill(0,0,640,440,18,18,21);const auto& s=engine->state();
 if(!started){draw_image("images/story.bmp",0,40,640,400);fill(105,140,430,150,22,20,20,242);text("三国志曹操传",245,165);text("开场脚本研究原型",245,201);text("点击开始 / Enter",245,244);return;}
 if(s.background=="battlefield"){
  SDL_Rect clip{0,caocao::battle_view_top,caocao::battle_view_width,caocao::battle_view_height};SDL_SetRenderClipRect(renderer,&clip);
  auto selection=s.pending_action.is_null()?Json::object():s.pending_action.value("ui",Json::object());
  if(selection.empty()&&!s.tutorial.is_null())selection=s.tutorial.at("ui");
  auto banner=s.pending_action.is_null()?Json::object():s.pending_action.value("banner",Json::object());
  int idle_tick=!banner.empty()?banner.at("tick").get<int>():selection.empty()?int(SDL_GetTicks()/200)%8:selection.at("tick").get<int>();
  if(!s.pending_action.is_null()&&(s.pending_action.value("opcode",-1)==107||s.pending_action.value("opcode",-1)==77))idle_tick=s.script_fields.value("battle_tick",0);
  battlefield(selection,idle_tick,banner);
  if(!s.pending_action.is_null()&&s.pending_action.contains("number")){
   const auto& n=s.pending_action.at("number");for(const auto& u:s.units)if(u.contains("battle")&&u.at("battle").at("slot")==n.at("slot")){
    auto b=caocao::BattleUnit::from_json(u.at("battle"));int x=b.x*48-s.camera_x,y=b.y*48-s.camera_y+56;std::string value=std::to_string(n.at("value").get<uint32_t>());
    for(size_t i=0;i<value.size();++i){indexed_fill(x+i*8,y,8,16,n.at("background"));text(value.substr(i,1),x+i*8,y,640,palette_color(n.at("foreground")));}
   }
  }
  if(!s.pending_action.is_null()&&s.pending_action.contains("effect"))poison_effect(s.pending_action.at("effect"));
  SDL_SetRenderClipRect(renderer,nullptr);
  if(!selection.empty()&&!selection.at("panel").is_null())unit_panel(selection.at("panel"),selection.at("panel_underlay"));
  if(!banner.empty())banner_text(banner);
  if(!s.pending_action.is_null()&&s.pending_action.contains("caption"))tactic_caption(s.pending_action.at("caption"));
 }else draw_image("images/"+s.background+".bmp",0,40,640,400);
 if(s.background=="story"){
  for(const auto& u:s.units){if(!u.value("visible",false))continue;float x=u.at("x"),y=u.at("y");
   if(s.presentation.value("type","")=="move"&&s.presentation.value("unit",-1)==u.at("id")){float t=std::clamp(1.f-float(int64_t(due)-int64_t(SDL_GetTicks()))/450.f,0.f,1.f);x=s.presentation.at("from_x").get<float>()*(1-t)+x*t;y=s.presentation.at("from_y").get<float>()*(1-t)+y*t;}
   // Original scene coordinates are a skewed grid; this research projection
   // still needs a pixel-by-pixel reference capture (see compatibility report).
   float px=8*(x-y)+326,py=4*(x+y)-194+40;
   int sprite=u.at("story_sprite");int direction=u.at("direction");int frame=u.value("action",0);frame=std::clamp(frame==0?0:frame+2,0,19);int atlas=sprite*2+((direction==0||direction==1)?0:1);
   std::string name="images/Pmapobj/"+number3(atlas)+".bmp";
   if(std::filesystem::exists(assets/name)){SDL_FRect from{0,float(frame*64),48,64};draw_image(name,px-24,py-64,48,64,&from,direction==1||direction==2);}
  }
 }
 fill(0,0,640,38,28,24,22);text("三国志曹操传 · 开场研究原型",12,10);
#ifdef __EMSCRIPTEN__
 if(s.stable)text(s.pending_action.is_null()?"空格继续 · 下方控件保存":"空格继续",420,10);
#else
 text("F5 保存  F9 读取  [ ] 槽 "+std::to_string(slot+1),365,10);
#endif
 if(s.phase=="blocked"){
  fill(26,140,588,235,25,21,20,247);text("战斗实现尚未完成",48,159);text("已停止执行，避免使用未验证的规则。",48,194);text(s.diagnostic,48,235,582);text("R：重新开始",48,321);
 }else if(s.stable&&s.presentation.value("type","")=="tutorial_command_menu"){draw_tutorial_menu();}
 else if(s.stable){
  fill(16,292,608,140,30,26,26,248);auto kind=s.presentation.value("type","");
  if(kind=="choice"){
   text("请选择",37,311);std::string str=s.presentation.at("text");std::istringstream lines(str);std::string line;int n=0;while(std::getline(lines,line)){++n;auto prefix=std::to_string(n)+".";text(line.starts_with(prefix)?line:prefix+" "+line,105,310+(n-1)*34);}
  }else{
   int face=s.presentation.value("face",-1);if(face>=0)draw_image("images/Face/"+number3(face)+".bmp",28,327,64,80);
   text(s.presentation.value("speaker",""),105,305,603,{238,205,142,255});text(s.presentation.value("text",""),105,336,603);text(">",598,409);
  }
 }
 if(!last_error.empty()){fill(0,419,640,21,32,23,20);text(last_error,8,421);}
}
void tick(){
 ++frame_count;
 SDL_Event ev;while(SDL_PollEvent(&ev)){
  if(ev.type==SDL_EVENT_QUIT){running=false;return;}
  if(ev.type==SDL_EVENT_KEY_DOWN){auto k=ev.key.key;if(k==SDLK_R){engine->start_scenario(1);started=true;observed.clear();observe();}
#ifndef __EMSCRIPTEN__
   if(k==SDLK_F5)native_save(false);if(k==SDLK_F9)native_save(true);if(k==SDLK_LEFTBRACKET)slot=(slot+9)%10;if(k==SDLK_RIGHTBRACKET)slot=(slot+1)%10;
#endif
   if(!started&&(k==SDLK_RETURN||k==SDLK_SPACE)){started=true;engine->start_scenario(1);observe();}
   else if(engine->state().stable&&engine->state().presentation.value("type","")=="tutorial_command_menu")tutorial_menu_key(k,(ev.key.mod&SDL_KMOD_SHIFT)!=0);
   else if(engine->state().stable){if(k==SDLK_1||k==SDLK_2)send({"choose",k==SDLK_1?1:2});else if(k==SDLK_RETURN||k==SDLK_SPACE)send({"continue"});}
   else if(engine->state().presentation.value("type","")=="objective_pause"&&(k==SDLK_RETURN||k==SDLK_SPACE))send({"continue"});
  }
  if(ev.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&ev.button.button==SDL_BUTTON_RIGHT&&engine->state().stable&&engine->state().presentation.value("type","")=="tutorial_command_menu")send({"tutorial_command",255});
  if(ev.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&ev.button.button==SDL_BUTTON_LEFT){float x,y;SDL_RenderCoordinatesFromWindow(renderer,ev.button.x,ev.button.y,&x,&y);
   if(!started){started=true;engine->start_scenario(1);observe();}
   else if(engine->state().stable&&engine->state().presentation.value("type","")=="tutorial_command_menu")tutorial_menu_click(x,y);
   else if(engine->state().stable){if(engine->state().presentation.value("type","")=="choice"){if(y>=300&&y<383)send({"choose",y<345?1:2});}else send({"continue"});}
   else if(engine->state().presentation.value("type","")=="objective_pause")send({"continue"});
  }
 }
 if(started&&!engine->state().stable&&engine->state().phase!="blocked"&&SDL_GetTicks()>=due){engine->advance();observe();}
 for(auto it=effects.begin();it!=effects.end();){if(SDL_GetAudioStreamQueued(*it)==0){if(!looping_effect_pcm.empty()){SDL_PutAudioStreamData(*it,looping_effect_pcm.data(),int(looping_effect_pcm.size()));++it;}else{SDL_DestroyAudioStream(*it);it=effects.erase(it);}}else ++it;}
 try{
  SDL_SetRenderTarget(renderer,scene_target);render();SDL_SetRenderTarget(renderer,nullptr);
  SDL_RenderTexture(renderer,scene_target,nullptr,nullptr);SDL_RenderPresent(renderer);
 }catch(const std::exception& e){last_error=e.what();std::cerr<<last_error<<'\n';running=false;}
}
#ifndef __EMSCRIPTEN__
void test_tutorial_menu_input(){
 auto original_origin=tutorial_menu_origin();const auto& bounds=engine->state().presentation.at("dialog").at("bounds_dlu");
 if(original_origin[0]<0||original_origin[1]<0||original_origin[0]+bounds.at(2).get<int>()*2>640||original_origin[1]+bounds.at(3).get<int>()*2>440)throw std::runtime_error("tutorial menu is clipped outside native viewport");
 auto saved=engine->serialize();SDL_Event discard;while(SDL_PollEvent(&discard)){};
 auto reset=[&](){engine->restore(saved);observed.clear();observe();due=UINT64_MAX;last_error.clear();};
 auto key=[&](SDL_Keycode code){SDL_Event event{};event.type=SDL_EVENT_KEY_DOWN;event.key.key=code;SDL_PushEvent(&event);tick();};
 auto click=[&](int control,int button=SDL_BUTTON_LEFT){auto origin=tutorial_menu_origin();float x=origin[0]+55,y=origin[1]+5;
  for(const auto& c:engine->state().presentation.at("controls"))if(c.at("id")==control)y=origin[1]+c.at("bounds_dlu").at(1).get<int>()*2+10;
  SDL_Event event{};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;event.button.button=button;
  if(!SDL_RenderCoordinatesToWindow(renderer,x,y,&event.button.x,&event.button.y))throw std::runtime_error(SDL_GetError());SDL_PushEvent(&event);tick();};
 auto result=[&](int value){if(engine->state().tutorial.at("menu_result")!=value||!last_error.empty())throw std::runtime_error("native tutorial menu input failed: "+last_error);};
 for(int control:{0x507,0x509}){reset();auto hash=engine->state_hash();click(control);if(hash!=engine->state_hash())throw std::runtime_error("disabled native menu button mutated state");}
 for(auto pair:{std::pair{2,255},std::pair{0x508,2},std::pair{0x50b,4}}){reset();click(pair.first);result(pair.second);}
 reset();key(SDLK_ESCAPE);result(255);reset();click(2,SDL_BUTTON_RIGHT);result(255);
 reset();key(SDLK_TAB);auto focused=engine->serialize();engine->restore(focused);if(engine->state().presentation.at("focused_control")!=0x508)throw std::runtime_error("menu focus did not survive restore");key(SDLK_RETURN);result(2);
 reset();key(SDLK_UP);key(SDLK_SPACE);result(4);reset();key(SDLK_RETURN);result(255);
 reset();std::cout<<"Native SDL menu: disabled clicks, three enabled clicks, Escape/right-click, Tab/Enter, Up/Space, default Enter, focus save/restore passed.\n";
}
#endif
std::string reply;
}
extern "C" {
API const char* caocao_status(){if(!engine){reply=Json{{"started",false},{"stable",false},{"can_save",false},{"phase","startup_failure"},{"presentation",Json::object()},{"error",last_error}}.dump();return reply.c_str();}reply=Json{{"frames",frame_count},{"renderer",renderer?SDL_GetRendererName(renderer):"none"},{"started",started},{"stable",engine->state().stable},{"can_save",engine->state().stable&&engine->state().pending_action.is_null()},{"phase",engine->state().phase},{"presentation",engine->state().presentation},{"state_hash",engine->state_hash()},{"diagnostic",engine->state().diagnostic},{"error",last_error}}.dump();return reply.c_str();}
API const char* caocao_snapshot(){try{reply=engine->serialize();}catch(const std::exception&e){reply=Json{{"error",e.what()}}.dump();}return reply.c_str();}
API const char* caocao_restore(const char* value){try{engine->restore(value);started=true;observed.clear();resume_audio();reply="{}";}catch(const std::exception&e){reply=Json{{"error",e.what()}}.dump();}return reply.c_str();}
API void caocao_start(){started=true;engine->start_scenario(1);observed.clear();observe();}
}
int main(int argc,char**argv){try{
#ifdef __EMSCRIPTEN__
 assets="/assets";
 // Page controls must retain their own arrow/Enter/Space behavior. SDL's
 // default window-level keyboard binding also advances the game from a select.
 SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT,"#canvas");
#else
 assets=argc>1?argv[1]:"assets/generated";
#endif
 ui_assets=caocao::read_json(assets/"ui.json");
 for(int id:{23,45,48,49}){effect_meta[id]=caocao::read_json(assets/(id==23?"poison-effect.json":"effect-"+std::to_string(id)+".json"));std::ifstream f(assets/("raw/Meff.e5/"+number3(id)+".bin"),std::ios::binary);effect_raw[id].assign(std::istreambuf_iterator<char>(f),{});if(effect_raw[id].size()!=size_t(id==23?88030:id==45?4296:536))throw std::runtime_error("invalid MEFF resource");}
 effect_meta[105]=caocao::read_json(assets/"mcall-05.json");{std::ifstream f(assets/"raw/Mcall05.e5/000.bin",std::ios::binary);effect_raw[105].assign(std::istreambuf_iterator<char>(f),{});if(effect_raw[105].size()!=3780620||caocao::sha256(std::string(effect_raw[105].begin(),effect_raw[105].end()))!=effect_meta.at(105).at("source_sha256").get<std::string>())throw std::runtime_error("invalid MCALL05 resource");}
 for(int n=0;n<256;++n){auto c=palette_color(n);palette_indices[(c.r<<16)|(c.g<<8)|c.b]=n;}
 for(int n=1;n<256;++n){const auto& a=ui_assets.at("palette").at(n);const auto& b=ui_assets.at("palette").at(ui_assets.at("darken").at(n).get<int>());dark_colors[(a[0].get<int>()<<16)|(a[1].get<int>()<<8)|a[2].get<int>()]=(b[0].get<int>()<<16)|(b[1].get<int>()<<8)|b[2].get<int>();}dark_colors[(247<<16)|255]=(247<<16)|255;
 engine=std::make_unique<caocao::Engine>(assets);font=caocao::read_json(assets/"font.json");music=caocao::read_json(assets/"music.json");
 for(int size:{32,48,64})banner_fonts[std::to_string(size)]=caocao::read_json(assets/("banner-font-"+std::to_string(size)+".json"));
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))throw std::runtime_error(SDL_GetError());window=SDL_CreateWindow("三国志曹操传 — 开场研究原型",960,660,SDL_WINDOW_RESIZABLE);renderer=SDL_CreateRenderer(window,nullptr);if(!renderer)throw std::runtime_error(SDL_GetError());SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderLogicalPresentation(renderer,640,440,SDL_LOGICAL_PRESENTATION_LETTERBOX);
 scene_target=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,640,440);if(!scene_target)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(scene_target,SDL_SCALEMODE_NEAREST);
#ifdef __EMSCRIPTEN__
 // SDL otherwise applies its default swap interval 0 after the first frame,
 // switching Emscripten's loop from requestAnimationFrame to a busy timer.
 if(!SDL_SetRenderVSync(renderer,1))throw std::runtime_error(SDL_GetError());
 emscripten_set_main_loop(tick,0,1);
#else
 std::string smoke_mode=argc>2?argv[2]:"";
 bool critical_smoke=smoke_mode=="--render-critical-smoke";
 bool attack_smoke=smoke_mode=="--render-attack-smoke";
 bool damage_smoke=smoke_mode=="--render-damage-smoke";
 bool preview_smoke=smoke_mode=="--render-preview-smoke";
 bool defeat_smoke=smoke_mode=="--render-defeat-smoke";
 bool after_defeat_smoke=smoke_mode=="--render-after-defeat-smoke";
 bool banner_smoke=smoke_mode=="--render-banner-smoke",reinforcement_smoke=smoke_mode=="--render-reinforcement-smoke";
 bool scripted_walk_smoke=smoke_mode=="--render-scripted-walk-smoke",after_reinforcement_smoke=smoke_mode=="--render-after-reinforcement-smoke";
 bool enemy_walk_smoke=smoke_mode=="--render-enemy-walk-smoke",enemy_ready_smoke=smoke_mode=="--render-enemy-ready-smoke";
 bool poison_smoke=smoke_mode=="--render-poison-smoke";
 bool tactic_damage_smoke=smoke_mode=="--render-tactic-damage-smoke",tactic_cost_smoke=smoke_mode=="--render-tactic-cost-smoke";
 bool tactic_cast_smoke=smoke_mode=="--render-tactic-cast-smoke";
 bool archer_dialogue_smoke=smoke_mode=="--render-archer-dialogue-smoke",archer_defeat_smoke=smoke_mode=="--render-archer-defeat-smoke",second_poison_smoke=smoke_mode=="--render-second-poison-smoke";
 bool tactic_preview_smoke=smoke_mode=="--render-tactic-preview-smoke";
 bool status_smoke=smoke_mode=="--render-poison-status-smoke",enemy_end_smoke=smoke_mode=="--render-enemy-end-smoke";
 bool next_turn_smoke=smoke_mode=="--render-next-turn-smoke",next_banner_smoke=smoke_mode=="--render-next-banner-smoke";
 bool fire_smoke=smoke_mode=="--render-fire-smoke",three_fires_smoke=smoke_mode=="--render-three-fires-smoke";
 bool summon_smoke=smoke_mode=="--render-summon-smoke",summon_end_smoke=smoke_mode=="--render-summon-end-smoke",summon_dialogue_smoke=smoke_mode=="--render-summon-dialogue-smoke",all_fires_smoke=smoke_mode=="--render-all-fires-smoke";
 bool confusion_smoke=smoke_mode=="--render-confusion-smoke",confusion_direct_smoke=smoke_mode=="--render-confusion-direct-smoke",arrival_smoke=smoke_mode=="--render-arrival-smoke";
 bool objective_smoke=smoke_mode=="--render-objective-smoke",highlight_smoke=smoke_mode=="--render-highlight-smoke";
 bool tutorial_cursor_smoke=smoke_mode=="--render-tutorial-cursor-smoke",tutorial_panel_smoke=smoke_mode=="--render-tutorial-panel-smoke";
 bool tutorial_blue_smoke=smoke_mode=="--render-tutorial-blue-smoke",tutorial_menu_smoke=smoke_mode=="--render-tutorial-menu-smoke";
 bool guide_exit_smoke=smoke_mode=="--render-guide-exit-smoke",start_banner_smoke=smoke_mode=="--render-start-banner-smoke";
 bool phase_poison_smoke=smoke_mode=="--render-phase-poison-smoke";
 bool tutorial_terrain_smoke=smoke_mode=="--render-tutorial-terrain-smoke";
 bool tutorial_range_smoke=smoke_mode=="--render-tutorial-range-smoke";
 bool combat_smoke=objective_smoke||highlight_smoke||confusion_smoke||confusion_direct_smoke||arrival_smoke||summon_smoke||summon_end_smoke||summon_dialogue_smoke||all_fires_smoke||fire_smoke||three_fires_smoke||next_turn_smoke||next_banner_smoke||status_smoke||enemy_end_smoke||archer_dialogue_smoke||archer_defeat_smoke||second_poison_smoke||tactic_damage_smoke||tactic_cost_smoke||poison_smoke||tactic_cast_smoke||critical_smoke||attack_smoke||damage_smoke||preview_smoke||defeat_smoke||after_defeat_smoke||banner_smoke||reinforcement_smoke||scripted_walk_smoke||after_reinforcement_smoke||enemy_walk_smoke||enemy_ready_smoke||tactic_preview_smoke;
 bool selection_smoke=argc>2&&std::string(argv[2])=="--render-selection-smoke";
 bool walking_smoke=argc>2&&std::string(argv[2])=="--render-walking-smoke";
 bool event_smoke=argc>2&&std::string(argv[2])=="--render-event-smoke";
 bool intro_smoke=argc>2&&std::string(argv[2])=="--render-intro-smoke";
 bool pose_smoke=argc>2&&std::string(argv[2])=="--render-pose-smoke";
 bool battle_smoke=event_smoke||intro_smoke||pose_smoke||(argc>2&&std::string(argv[2])=="--render-battle-smoke");
 combat_smoke=combat_smoke||phase_poison_smoke||guide_exit_smoke||start_banner_smoke||tutorial_terrain_smoke||tutorial_blue_smoke||tutorial_menu_smoke||tutorial_cursor_smoke||tutorial_panel_smoke||tutorial_range_smoke;
 bool smoke=combat_smoke||selection_smoke||walking_smoke||battle_smoke||smoke_mode=="--render-smoke";
 if(smoke){
  started=true;engine->start_scenario(1);bool reached=false;
  for(int i=0;i<6500;++i){const auto& s=engine->state();
   const auto ui=s.pending_action.is_null()?Json::object():s.pending_action.value("ui",Json::object());
   if(phase_poison_smoke&&s.battle_stage=="phase_after_maintenance"&&s.presentation.value("type","")=="animation_wait"&&s.presentation.value("duration_ms",0)==400)reached=true;
   if(guide_exit_smoke&&s.presentation.value("type","")=="animation_wait"&&!s.pending_action.is_null()&&s.pending_action.value("opcode",-1)==49&&s.battle_poses.contains("157")&&s.battle_poses.at("157").at("height")==24)reached=true;
   if(start_banner_smoke&&s.presentation.value("type","")=="animation_wait"&&!s.pending_action.is_null()&&s.pending_action.value("opcode",-1)==90)reached=true;
   if(banner_smoke&&!s.pending_action.is_null()&&s.pending_action.contains("banner")&&s.pending_action.at("banner").at("text").size()==3&&s.presentation.value("type","")=="animation_wait")reached=true;
   if(objective_smoke&&s.presentation.value("type","")=="objective_pause")reached=true;
   if(tutorial_cursor_smoke&&!s.tutorial.is_null()&&s.stable&&s.presentation.value("text","").starts_with("你了解这个白框吗"))reached=true;
   if(tutorial_panel_smoke&&!s.tutorial.is_null()&&s.stable&&s.presentation.value("text","").starts_with("这个情报视窗在战斗时"))reached=true;
   if(tutorial_blue_smoke&&!s.tutorial.is_null()&&s.stable&&s.tutorial.at("ui").value("blue_movement",false))reached=true;
   if(tutorial_menu_smoke&&s.presentation.value("type","")=="tutorial_command_menu")reached=true;
   if(tutorial_terrain_smoke&&!s.tutorial.is_null()&&s.stable&&s.tutorial.at("ui").contains("panel_tile"))reached=true;
   if(tutorial_range_smoke&&!s.tutorial.is_null()&&s.stable&&s.presentation.value("text","").starts_with("就会显示这些讯息"))reached=true;
   if(highlight_smoke&&!s.pending_action.is_null()&&s.pending_action.at("opcode")==92&&s.presentation.value("type","")=="animation_wait"&&!ui.empty()&&std::count(ui.at("range").begin(),ui.at("range").end(),1)==1)reached=true;
   if(scripted_walk_smoke&&s.battle_poses.contains("32")&&s.battle_poses.at("32").contains("screen_x"))reached=true;
   if(after_reinforcement_smoke&&s.presentation.value("type","")=="scripted_movement_complete"&&s.presentation.at("unit")==33)reached=true;
   if(enemy_walk_smoke&&s.battle_poses.contains("146")&&s.battle_poses.at("146").contains("screen_x"))reached=true;
   if(enemy_ready_smoke&&s.presentation.value("type","")=="poison_effect")reached=true;
   if(tactic_cast_smoke&&s.presentation.value("type","")=="tactic_caption")reached=true;
   if(poison_smoke&&!s.pending_action.is_null()&&s.pending_action.contains("effect")&&s.pending_action.at("effect").at("frame")==5)reached=true;
   if(status_smoke&&!s.pending_action.is_null()&&s.pending_action.contains("effect")&&s.pending_action.at("effect").value("resource",23)==49&&s.pending_action.at("effect").at("frame")==5)reached=true;
   if(enemy_end_smoke&&s.presentation.value("type","")=="turn_banner"&&s.turn==2)reached=true;
   if(next_turn_smoke&&s.turn==2&&s.stable&&s.presentation.value("speaker","")=="刘备")reached=true;
   if(fire_smoke&&s.stable&&s.terrain_effects.at(0).at(4)==0)reached=true;
   if((summon_smoke||summon_end_smoke)&&!s.pending_action.is_null()&&s.pending_action.contains("effect")&&s.pending_action.at("effect").value("resource",23)==105&&s.pending_action.at("effect").at("frame")== (summon_smoke?10:29))reached=true;
   if(summon_dialogue_smoke&&s.turn==2&&s.mcall_resource==105&&s.stable&&s.presentation.value("speaker","")=="许子将")reached=true;
   if((confusion_smoke||confusion_direct_smoke)&&s.presentation.value("type","")=="animation_wait"&&!s.pending_action.is_null()&&s.pending_action.contains("effect")&&s.pending_action.at("effect").value("resource",23)==48&&s.pending_action.at("effect").at("frame")== (confusion_smoke?1:3))reached=true;
   if(arrival_smoke&&s.turn==2&&s.stable&&s.presentation.value("speaker","")=="曹操"&&s.units.at("0").at("battle").at("lifecycle")==2)reached=true;
   if(all_fires_smoke&&s.stable&&s.terrain_effects.at(26).at(4)==0)reached=true;
   if(three_fires_smoke&&s.terrain_effects.at(2).at(4)==0&&s.presentation.value("type","")=="delay")reached=true;
   if(next_banner_smoke&&s.turn==2&&!s.pending_action.is_null()&&s.pending_action.contains("banner")&&s.pending_action.at("banner").at("text").size()==6&&s.presentation.value("type","")=="animation_wait")reached=true;
   if(archer_dialogue_smoke&&s.stable&&s.presentation.value("text","")=="唔，援军…援军还不来吗……？")reached=true;
   if(archer_defeat_smoke&&!s.pending_action.is_null()&&s.pending_action.at("unit")==234&&s.pending_action.value("hidden",false))reached=true;
   if(second_poison_smoke&&!s.pending_action.is_null()&&s.pending_action.contains("effect")&&s.pending_action.at("effect").at("frame")==5&&s.script_fields.at("ai_pending").at("unit")==147)reached=true;
   if((tactic_damage_smoke||tactic_cost_smoke)&&s.battle_stage=="ai_tactic_post"&&!ui.empty()&&!ui.at("panel").is_null())
    for(const auto& e:ui.at("panel"))if(e.value("text","")==(tactic_damage_smoke?"   0 /  90":"  40 /  48"))reached=true;
   if(tactic_preview_smoke&&s.battle_stage=="ai_tactic_plan"&&!ui.empty()&&!ui.at("panel").is_null())reached=true;
   if(reinforcement_smoke&&s.stable&&s.presentation.value("text","").starts_with("看起来好像赶上了"))reached=true;
   if(selection_smoke&&!ui.empty()&&!ui.at("panel").is_null()&&ui.at("movement").get<bool>())reached=true;
   if(walking_smoke&&s.battle_poses.contains("234")&&s.battle_poses.at("234").contains("screen_x"))reached=true;
   if(critical_smoke&&s.stable&&s.presentation.contains("combat_message"))reached=true;
   if(defeat_smoke&&!s.pending_action.is_null()&&s.pending_action.at("unit")==475&&s.pending_action.value("hidden",false))reached=true;
   if(after_defeat_smoke&&s.presentation.value("type","")=="event_batch_complete"&&s.units.at("475").at("battle").at("lifecycle")==3)reached=true;
   if(attack_smoke&&s.battle_poses.contains("475")&&!s.battle_poses.at("475").value("effects",Json::array()).empty())reached=true;
   if(damage_smoke&&!ui.empty()&&!ui.at("panel").is_null())for(const auto& e:ui.at("panel"))if(e.value("text","")=="  67 /  90")reached=true;
   if(preview_smoke&&!ui.empty()&&!ui.at("panel").is_null())for(const auto& e:ui.at("panel"))if(e.value("text","")==ui_assets.at("attack_labels").at("damage").at("text").get<std::string>())reached=true;
   if(s.stable&&((!battle_smoke&&!selection_smoke&&!walking_smoke&&!combat_smoke)||(battle_smoke&&(event_smoke?s.phase=="battle_event":s.phase=="battle_intro")&&(!pose_smoke||!s.battle_poses.empty())&&(!intro_smoke||s.presentation.value("text","").starts_with("胜利条件")))))reached=true;
   if(reached)break;
   if(s.phase=="blocked")break;
   if(s.stable)engine->submit({s.presentation.value("type","")=="tutorial_command_menu"?"tutorial_command":s.presentation.value("type","")=="choice"?"choose":"continue",s.presentation.value("type","")=="tutorial_command_menu"?255:1});else engine->advance();
  }
  if(!reached)throw std::runtime_error("render smoke did not reach requested presentation: "+engine->state().diagnostic);
  observe();due=UINT64_MAX;
  if(tutorial_menu_smoke)test_tutorial_menu_input();
 }
 int frames=0;while(running){tick();SDL_Delay(16);if(smoke&&++frames==5){
  std::string output=smoke_mode.substr(2)+".bmp";auto* image=SDL_RenderReadPixels(renderer,nullptr);
  if(!image||!SDL_SaveBMP(image,output.c_str()))throw std::runtime_error(SDL_GetError());SDL_DestroySurface(image);running=false;
 }}
 for(auto&[name,t]:textures)SDL_DestroyTexture(t);for(auto* s:effects)SDL_DestroyAudioStream(s);if(music_stream)SDL_DestroyAudioStream(music_stream);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
#endif
 return 0;
}catch(const std::exception&e){last_error=e.what();std::cerr<<last_error<<'\n';return 1;}}
