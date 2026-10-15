#include "caocao/battle.hpp"
#include "caocao/terrain_effect.hpp"
#include "caocao/selection.hpp"
#include "caocao/panel.hpp"
#include "caocao/attack_animation.hpp"
#include "caocao/tactic_animation.hpp"
#include "caocao/tactic_effect.hpp"
#include "caocao/mcall.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");
 const std::filesystem::path assets=argv[1],fixtures=assets.parent_path().parent_path()/"tests/fixtures";
 auto summon=read_json(fixtures/"original_mcall.json"),summon_meta=read_json(assets/"mcall-05.json");
 auto blends=read_json(fixtures/"original_mcall_blend.json"),summon_palette=read_json(assets/"ui.json").at("combat_palette");
 std::ifstream summon_file(assets/"raw/Mcall05.e5/000.bin",std::ios::binary);
 std::string summon_raw((std::istreambuf_iterator<char>(summon_file)),{});
 if(sha256(summon_raw)!=summon.at("resource_sha256").get<std::string>())throw std::runtime_error("MCALL source bytes changed");
 std::vector<uint8_t> q=summon_palette.at("quantized"),lut=summon_palette.at("lookup"),src(65536),mask(65536),bg(65536);
 for(int n=0;n<65536;++n){src[n]=n/256;bg[n]=n%256;}
 for(const auto& pair:blends.at("pairs")){
  std::fill(mask.begin(),mask.end(),pair.at("mask").get<int>());
  auto out=blend_mcall_pixels(src,mask,bg,q,lut,pair.at("amount"));
  if(sha256(std::string(out.begin(),out.end()))!=pair.at("pixels_sha256").get<std::string>())throw std::runtime_error("original MCALL palette pairs differ");
 }
 for(const auto& frame:blends.at("frames")){
  const auto& f=summon_meta.at("frames").at(frame.at("frame").get<int>());int off=f.at("pixel_offset");
  src.assign(summon_raw.begin()+off,summon_raw.begin()+off+90000);bg.resize(90000);
  for(int n=0;n<90000;++n)bg[n]=(n*17+n/300*3)%256;
  std::vector<uint8_t> out;
  if(f.at("masked")){off=f.at("mask_offset");mask.assign(summon_raw.begin()+off,summon_raw.begin()+off+90000);out=blend_mcall_pixels(src,mask,bg,q,lut,f.at("amount"));}
  else out=blend_poison_pixels(src,bg,q,lut,f.at("weight"),f.at("amount"));
  if(sha256(std::string(out.begin(),out.end()))!=frame.at("pixels_sha256").get<std::string>())throw std::runtime_error("MCALL full frame differs");
 }
 for(const auto& c:summon.at("cases")){
  auto sequence=mcall_animation(c.at("center").at(0),c.at("center").at(1),summon_meta);auto expected=c.at("events");
  if(sequence.size()!=expected.size())throw std::runtime_error("MCALL player event count differs");
  for(size_t n=0;n<sequence.size();++n){auto a=sequence[n],b=expected[n];
   if(b.at("kind")=="restore")b["kind"]="effect_restore";
   if(b.at("kind")=="capture")b.erase("pixels_sha256");
   if(b.at("kind")=="effect"){
    const auto& f=summon_meta.at("frames").at(b.at("frame").get<int>());
    if(f.at("descriptor")!=b.at("descriptor")||f.at("pixels_sha256")!=b.at("image_sha256")||f.at("mask_sha256")!=b.at("mask_sha256")||b.at("palette")!=0)throw std::runtime_error("MCALL descriptor/image/mask differs from original loader");
    b.erase("descriptor");b.erase("image_sha256");b.erase("mask_sha256");b.erase("palette");b["resource"]=105;b["masked"]=f.at("masked");
   }
   if(a!=b)throw std::runtime_error("MCALL player event differs: "+a.dump()+" != "+b.dump());
  }
 }
 std::cout<<summon.at("cases").size()<<" MCALL players, "<<blends.at("pairs").size()*65536<<" masked palette pairs and "<<blends.at("frames").size()<<" full-frame blends passed\n";
 auto original=read_json(fixtures/"original_animations.json"),units=read_json(fixtures/"original_battle_initialization.json");
 auto tutorial_animations=read_json(fixtures/"original_tutorial_animations.json");
 for(const auto& c:tutorial_animations.at("cases"))original["cases"].push_back(c);
 auto fires=read_json(fixtures/"original_fire.json");
 for(const auto& c:fires.at("cases")){
  const auto& before=c.at("before");auto records=before.at("records"),terrain=before.at("terrain"),marks=before.at("effects");
  auto camera=before.at("camera").get<std::array<int,2>>();const auto& a=c.at("args");
  auto steps=fire_placement(records,terrain,20,20,a.at(0),a.at(1),a.at(3)!=0,a.at(4)!=0,camera);Json visible=Json::array();
  for(const auto& step:steps){auto kind=step.at("kind");
   if(kind=="fire_terrain"){records.at(step.at("slot").get<int>())=step.at("record");terrain.at(step.at("tile").get<int>())=26;}
   else if(kind=="fire_mark")marks.at(step.at("tile").get<int>())=step.at("effect");
   else{
    visible.push_back(step);
    if(kind=="camera_request"){auto path=battle_camera_path(camera[0],camera[1],step.at("x"),step.at("y"),20,20);if(!path.empty())camera=path.back();}
   }
  }
  Json expected=Json::array();for(auto event:c.at("events")){if(event.at("kind")=="camera_redraw")continue;event.erase("camera");expected.push_back(event);}
  if(visible!=expected||records!=c.at("after").at("records")||terrain!=c.at("after").at("terrain")||marks!=c.at("after").at("effects")||Json(camera)!=c.at("after").at("camera"))throw std::runtime_error("original fire placement differs: "+c.at("label").get<std::string>());
 }
 std::ifstream mark_file(assets/"raw/Mark.e5/000.bin",std::ios::binary);std::string mark((std::istreambuf_iterator<char>(mark_file)),{});
 for(const auto& frame:fires.at("frames")){
  int offset=fire_frame_offset(frame.at("tick"));
  if(offset!=frame.at("offset")||sha256(mark.substr(offset,0x900))!=frame.at("pixels_sha256").get<std::string>()||frame.at("mode")!=4)throw std::runtime_error("original fire frame pixels differ");
 }
 std::cout<<fires.at("cases").size()<<" fire placements and "<<fires.at("frames").size()<<" original fire frames passed\n";
 size_t frames=0;
 auto presentation=read_json(fixtures/"original_presentation.json");
 for(const auto& c:presentation.at("camera")){
  auto path=battle_camera_path(c.at("start").at(0),c.at("start").at(1),c.at("focus").at(0),c.at("focus").at(1),20,20);
  if(path.size()!=c.at("redraws").size())throw std::runtime_error("camera step count mismatch");
  for(size_t n=0;n<path.size();++n)if(Json(path[n])!=c.at("redraws").at(n).at("camera"))throw std::runtime_error("camera step mismatch");
  Json end=path.empty()?c.at("start"):Json(path.back());if(end!=c.at("end"))throw std::runtime_error("camera final position mismatch");
 }
 for(const auto& c:presentation.at("visible_camera")){
  int x=c.at("start").at(0),y=c.at("start").at(1),fx=c.at("focus").at(0),fy=c.at("focus").at(1);
  int cx=std::clamp(x/48,fx-12,fx),cy=std::clamp(y/48,fy-7,fy);
  auto path=battle_camera_path(x,y,cx+6,cy+4,20,20);
  if(path.size()!=c.at("redraws").size())throw std::runtime_error("visible camera step count mismatch");
  for(size_t n=0;n<path.size();++n)if(Json(path[n])!=c.at("redraws").at(n).at("camera"))throw std::runtime_error("visible camera step mismatch");
 }
 for(const auto& c:presentation.at("sounds")){
  auto file=battle_sound_file(c.at("id"));if(!std::filesystem::is_regular_file(assets/file))throw std::runtime_error("sound resource missing");
  auto name=std::filesystem::path(file).filename().string();std::transform(name.begin(),name.end(),name.begin(),[](unsigned char ch){return char(std::toupper(ch));});
  if(c.at("calls").back().at("file")!=name||c.at("calls").back().at("flags")!=3)throw std::runtime_error("original sound filename or one-shot flags mismatch");
 }
 for(const auto& c:original.at("cases")){
  auto row=std::find_if(units.at("battle").begin(),units.at("battle").end(),[&](const Json& u){return u.at("unit").at("id")==c.at("unit");});
  if(row==units.at("battle").end())throw std::runtime_error("missing unit fixture");
  const auto& live=row->at("live");BattleUnit b;b.id=c.at("unit");b.slot=live.at(4);b.x=live.at(6);b.y=live.at(7);b.lifecycle=live.at(12);b.direction=c.at("direction");
  auto sequence=scripted_battle_animation(b,row->at("sprite"),c.at("animation"),c.at("pre").get<int>()!=0,c.at("post").get<int>()!=0,0,0);
  auto expected=c.at("events");
  if(sequence.size()!=expected.size())throw std::runtime_error("animation sequence length mismatch");
  for(size_t n=0;n<sequence.size();++n){auto actual=sequence[n];auto reference=expected[n];
   if(actual.at("kind")=="frame"){
    ++frames;
    Json selected={{"frame",actual.at("frame")},{"mirror",actual.at("mirror")}};
    if(std::find(reference.at("matches").begin(),reference.at("matches").end(),selected)==reference.at("matches").end())throw std::runtime_error("animation selected wrong atlas frame or mirror");
    std::string archive=actual.at("archive");archive=archive=="UNIT_ATK.E5"?"Unit_atk.e5":archive=="UNIT_SPC.E5"?"Unit_spc.e5":"Unit_mov.e5";
    std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<actual.at("sprite").get<int>()<<".bin";
    std::ifstream f(assets/"raw"/archive/filename.str(),std::ios::binary);if(!f)throw std::runtime_error("missing indexed sprite block");
    const int w=actual.at("width"),h=actual.at("height");std::string pixels(w*h,'\0');f.seekg(actual.at("frame").get<int>()*w*h);f.read(pixels.data(),pixels.size());if(!f)throw std::runtime_error("truncated indexed sprite block");
    if(actual.at("mirror").get<bool>())for(int y=0;y<h;++y)std::reverse(pixels.begin()+y*w,pixels.begin()+(y+1)*w);
    if(sha256(pixels)!=reference.at("pixels_sha256").get<std::string>())throw std::runtime_error("rendered indexed pixels differ from original animation");
    actual.erase("frame");actual.erase("mirror");reference.erase("matches");reference.erase("pixels_sha256");
   }
   if(actual!=reference)throw std::runtime_error("animation event mismatch: "+actual.dump()+" != "+reference.dump());
  }
 }
 BattleUnit b;b.lifecycle=2;bool rejected=false;try{scripted_battle_animation(b,0,10,false,false,0,0);}catch(const std::runtime_error&){rejected=true;}
 if(!rejected)throw std::runtime_error("unverified animation was accepted");
 std::cout<<original.at("cases").size()<<" original animation sequences and "<<frames<<" indexed frame hashes passed.\n";
 std::cout<<presentation.at("camera").size()<<" original camera paths and "<<presentation.at("sounds").size()<<" sound filename/flag fixtures passed.\n";
 auto selection=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_selection.json");
 auto tactic_fixture=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_tactics.json");Json tactics=Json::array();
 for(const auto& r:tactic_fixture.at("records"))tactics.push_back(r.at("fields"));
 auto data=read_json(assets/"data.json");size_t index=0;
 for(const auto& c:selection.at("cases")){
  auto s=battle_selection_sequence(BattleUnit::from_json(c.at("battle")),UnitRecord::from_json(c.at("unit")),c.at("mode"),c.at("target"),c.at("tactic"),c.at("initial_tick"),c.at("interrupt_at"),data,tactics);
  if(s.events!=c.at("events")||s.tick!=c.at("final_tick"))throw std::runtime_error("selection sequence mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original selection display/interrupt/tick sequences passed.\n";
 auto panels=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_panel.json");index=0;
 for(const auto& c:panels.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle"));auto actual=battle_unit_panel(b,UnitRecord::from_json(c.at("unit")),c.at("name"),c.at("camera").get<std::array<int,2>>(),c.at("terrain").at(b.y*20+b.x),data,panels.at("ui"));
  if(actual!=c.at("events"))throw std::runtime_error("panel mismatch "+std::to_string(index)+": "+actual.dump()+" != "+c.at("events").dump());
  ++index;
 }
 std::cout<<index<<" original unit panels, glyph coordinates and bar requests passed.\n";
 auto terrain_panels=read_json(fixtures/"original_terrain_panel.json"),terrain_ui=read_json(assets/"ui.json");
 std::ifstream map_file(assets/"raw/Hm00.e5/000.bin",std::ios::binary);std::string map_pixels((std::istreambuf_iterator<char>(map_file)),{});
 for(int n=0;n<256;++n)if(terrain_grayscale_index(n,terrain_ui)!=terrain_panels.at("grayscale").at(n))throw std::runtime_error("original terrain icon grayscale differs");
 for(const auto& p:terrain_panels.at("predicates")){
  auto info=terrain_panel_info(p.at("terrain"));Json flags=Json::array();for(bool flag:info.tactics)flags.push_back(int(flag));
  if(int(info.recovery)!=p.at("recovery")||int(info.impassable_label)!=p.at("impassable")||flags!=p.at("tactics"))throw std::runtime_error("original terrain description predicates differ");
 }
 for(const auto& c:terrain_panels.at("cases")){
  auto tile=c.at("tile").get<std::array<int,2>>();auto actual=battle_terrain_panel(tile,c.at("camera").get<std::array<int,2>>(),c.at("terrain"),terrain_ui);
  auto tile_hash=sha256(map_pixels.substr((tile[1]*20+tile[0])*2304,2304));
  for(auto& event:actual){auto kind=event.at("kind");
   if(kind=="tile_capture"||kind=="terrain_tile")event["pixels_sha256"]=tile_hash;
   else if(kind=="icon")event["pixels_sha256"]=sha256(mark.substr(event.at("args").at(4).get<int>(),256));
   else if(kind=="gray_icon"){
    int n=event.at("icon_index");std::string pixels=mark.substr(0x1a5c0+n*256,256);Json bytes=Json::array();
    for(char& pixel:pixels){pixel=static_cast<char>(terrain_grayscale_index(static_cast<unsigned char>(pixel),terrain_ui));bytes.push_back(static_cast<unsigned char>(pixel));}
    event.erase("icon_index");event["pixels"]=bytes;event["pixels_sha256"]=sha256(pixels);
   }
  }
  if(actual!=c.at("events"))throw std::runtime_error("terrain panel raster differs: "+actual.dump()+" != "+c.at("events").dump());
 }
 std::cout<<terrain_panels.at("cases").size()<<" original terrain panels, 30 terrain predicates and 256 grayscale indices passed.\n";
 auto attack_panels=read_json(fixtures/"original_attack_panel.json");index=0;
 for(const auto& c:attack_panels.at("cases")){
  auto b=BattleUnit::from_json(c.at("battle")),a=BattleUnit::from_json(c.at("actor"));
  auto actual=battle_attack_panel(b,UnitRecord::from_json(c.at("unit")),c.at("name"),a,UnitRecord::from_json(c.at("actor_unit")),c.at("camera").get<std::array<int,2>>(),c.at("terrain").at(b.y*20+b.x),c.at("terrain").at(a.y*20+a.x),data,attack_panels.at("ui"));
  if(actual!=c.at("events"))throw std::runtime_error("attack panel mismatch "+std::to_string(index)+": "+actual.dump()+" != "+c.at("events").dump());
  ++index;
 }
 std::cout<<index<<" original attack previews and deterministic damage/chance bars passed.\n";
 auto tactic_panels=read_json(fixtures/"original_tactic_panel.json");index=0;
 for(const auto& c:tactic_panels.at("cases")){
  auto actual=battle_tactic_panel(BattleUnit::from_json(c.at("battle")),UnitRecord::from_json(c.at("unit")),c.at("name"),BattleUnit::from_json(c.at("actor")),UnitRecord::from_json(c.at("actor_unit")),c.at("camera").get<std::array<int,2>>(),c.at("tactic"),data,tactic_panels.at("ui"),tactics);
  if(actual!=c.at("events"))throw std::runtime_error("tactic panel mismatch "+std::to_string(index)+": "+actual.dump()+" != "+c.at("events").dump());
  ++index;
 }
 std::cout<<index<<" original tactic preview panels, estimates and glyph coordinates passed.\n";
 auto damage_panels=read_json(fixtures/"original_damage_panel.json");index=0;
 for(const auto& c:damage_panels.at("cases")){
  BattleUnit other;if(!c.at("other").is_null())other=BattleUnit::from_json(c.at("other"));
  auto actual=battle_damage_panel(BattleUnit::from_json(c.at("battle")),UnitRecord::from_json(c.at("unit")),c.at("name"),c.at("other").is_null()?nullptr:&other,c.at("camera").get<std::array<int,2>>(),c.at("hp"),c.at("mp"),c.at("wait"),data,damage_panels.at("ui"));
  if(actual!=c.at("events"))throw std::runtime_error("damage panel sequence mismatch "+std::to_string(index)+": "+actual.dump()+" != "+c.at("events").dump());
  ++index;
 }
 std::cout<<index<<" original NPC HP/MP panel interpolation sequences passed.\n";
 auto camera_fixture=read_json(fixtures/"original_attack_camera.json");
 for(const auto& c:camera_fixture.at("panels")){
  BattleUnit b,a;b.x=c.at("target")[0];b.y=c.at("target")[1];a.x=c.at("other")[0];a.y=c.at("other")[1];
  auto actual=battle_panel_position(b,c.at("camera").get<std::array<int,2>>(),&a);
  if(Json(actual)!=c.at("result"))throw std::runtime_error("attack panel avoidance mismatch");
 }
 for(const auto& c:camera_fixture.at("cameras")){
  auto path=battle_attack_camera_path(48,576,c.at("actor").get<std::array<int,2>>(),c.at("target").get<std::array<int,2>>(),20,20);
  auto last=path.empty()?std::array<int,2>{48,576}:path.back();
  if(Json::array({last[0]/48,last[1]/48})!=c.at("result"))throw std::runtime_error("attack camera destination mismatch");
 }
 std::cout<<camera_fixture.at("panels").size()<<" panel avoidance and "<<camera_fixture.at("cameras").size()<<" attack camera destinations passed.\n";
 auto overlays=read_json(assets.parent_path().parent_path()/"tests/fixtures/original_overlay.json");index=0;
 for(const auto& c:overlays.at("ranges")){
  auto actual=selection_range(c.at("center").get<std::array<int,2>>(),c.at("kind"),c.at("include_center").get<int>(),20,20);
  if(Json(actual)!=c.at("mask"))throw std::runtime_error("UI range mask mismatch "+std::to_string(index));++index;
 }
 for(const auto& c:overlays.at("cases")){
  auto actual=selection_tile_layers(c.at("tile").get<std::array<int,2>>(),{6,4},c.at("style"),c.at("movement").get<int>(),c.at("reachable").get<int>(),c.at("marked").get<int>(),c.at("covered").get<int>(),20);
  if(actual!=c.at("events"))throw std::runtime_error("UI tile layer mismatch "+std::to_string(index)+": "+actual.dump()+" != "+c.at("events").dump());++index;
 }
 std::cout<<overlays.at("ranges").size()<<" original UI masks and "<<overlays.at("cases").size()<<" tile layer orders passed.\n";
 auto blue_overlay=read_json(fixtures/"original_blue_overlay.json");
 for(const auto& c:blue_overlay.at("cases")){
  auto actual=selection_tile_layers(c.at("tile").get<std::array<int,2>>(),{6,4},c.at("style"),c.at("movement").get<int>(),c.at("reachable").get<int>(),c.at("marked").get<int>(),c.at("covered").get<int>(),20,c.at("blue").get<int>(),c.at("terrain_mark")==3?0x1ec0:-1);
  if(actual!=c.at("events"))throw std::runtime_error("blue/terrain layer order differs: "+c.dump());
 }
 std::cout<<blue_overlay.at("cases").size()<<" original blue/green/terrain layer orders passed.\n";
 auto menu=read_json(fixtures/"original_tutorial_menu.json");
 for(const auto& c:menu.at("layouts")){
  auto camera=c.at("camera").get<std::array<int,2>>();for(auto& n:camera)n*=48;
  auto pos=command_menu_position(c.at("tile").get<std::array<int,2>>(),camera,c.at("size").get<std::array<int,2>>(),c.at("viewport").get<std::array<int,2>>(),c.at("client").get<std::array<int,2>>());
  const auto& args=c.at("events").at(0).at("args");
  if(pos[0]!=static_cast<int32_t>(args.at(2).get<uint32_t>())||pos[1]!=static_cast<int32_t>(args.at(3).get<uint32_t>()))throw std::runtime_error("original command menu positioning differs");
 }
 std::cout<<menu.at("layouts").size()<<" original command menu position cases passed.\n";
 auto attacks=read_json(fixtures/"original_attack_animation.json"),palette=read_json(fixtures/"original_combat_palette.json");
 auto quantized=palette.at("quantized").get<std::vector<uint8_t>>(),lookup=palette.at("lookup").get<std::vector<uint8_t>>();
 index=0;frames=0;std::map<std::string,std::string> raw_sprites;
 for(const auto& c:attacks.at("cases")){
  auto a=BattleUnit::from_json(c.at("before")[0]),b=BattleUnit::from_json(c.at("before")[1]);
  auto result=physical_attack_animation(a,UnitRecord::from_json(c.at("actor_unit")),b,UnitRecord::from_json(c.at("target_unit")),c.at("damage"),c.at("mp_damage"),c.at("critical"),c.at("camera")[0],c.at("camera")[1],c.at("tick"),data);
  Json expected=Json::array();for(const auto& e:c.at("events"))if(e.at("kind")!="color_transform")expected.push_back(e);
  if(expected.size()!=result.events.size())throw std::runtime_error("attack animation event count mismatch "+std::to_string(index));
  for(size_t n=0;n<expected.size();++n){auto actual=result.events[n],reference=expected[n];
   if(actual.at("kind")=="frame"){
    ++frames;std::string archive=actual.at("archive");archive=archive=="UNIT_ATK.E5"?"Unit_atk.e5":archive=="UNIT_SPC.E5"?"Unit_spc.e5":"Unit_mov.e5";
    std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<actual.at("sprite").get<int>()<<".bin";
    auto key=archive+"/"+filename.str();
    if(!raw_sprites.contains(key)){std::ifstream f(assets/"raw"/key,std::ios::binary);if(!f)throw std::runtime_error("missing attack sprite "+key);raw_sprites[key]=std::string(std::istreambuf_iterator<char>(f),{});}
    int w=actual.at("width"),h=actual.at("height"),offset=actual.at("frame").get<int>()*w*h;
    auto& raw=raw_sprites.at(key);if(size_t(offset+w*h)>raw.size())throw std::runtime_error("invalid attack frame index");
    std::vector<uint8_t> pixels(raw.begin()+offset,raw.begin()+offset+w*h);
    if(actual.at("mirror").get<bool>())for(int y=0;y<h;++y)std::reverse(pixels.begin()+y*w,pixels.begin()+(y+1)*w);
    pixels=transform_combat_pixels(std::move(pixels),actual.at("effects"),quantized,lookup);
    if(sha256(std::string(pixels.begin(),pixels.end()))!=reference.at("pixels_sha256").get<std::string>())throw std::runtime_error("attack pixels mismatch case "+std::to_string(index)+" event "+std::to_string(n)+": "+actual.dump());
    for(const auto& key:{"archive","sprite","frame","mirror","effects"})actual.erase(key);
    reference.erase("matches");reference.erase("pixels_sha256");
   }
   if(actual!=reference)throw std::runtime_error("attack animation mismatch case "+std::to_string(index)+" event "+std::to_string(n)+": "+actual.dump()+" != "+reference.dump());
  }
  if(Json::array({result.actor.json(),result.target.json()})!=c.at("after")||result.tick!=c.at("final_tick"))throw std::runtime_error("attack animation unit/tick mismatch "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original physical attack animations and "<<frames<<" indexed frame/color hashes passed.\n";
 auto casts=read_json(fixtures/"original_tactic_cast.json"),tactic_records=read_json(assets/"tactics.json");index=0;
 for(const auto& c:casts.at("cases")){
  auto row=[&](const char* stage,int slot)->const Json&{for(const auto& r:c.at(stage).at("units"))if(r.at("battle").at("slot")==slot)return r;throw std::runtime_error("missing caster fixture unit");};
  auto a=BattleUnit::from_json(row("before",35).at("battle")),b=BattleUnit::from_json(row("before",20).at("battle"));
  auto cast=tactic_cast_animation(a,UnitRecord::from_json(row("before",35).at("unit")),b,c.at("tactic"),c.at("before").at("camera"),20,20,c.at("speed"),c.at("elapsed"),tactic_records);
  if(cast.actor.json()!=row("after",35).at("battle")||Json(cast.camera)!=c.at("after").at("camera"))throw std::runtime_error("caster facing/camera mismatch "+std::to_string(index));
  auto expected=c.at("events");
  if(expected.size()!=cast.events.size())throw std::runtime_error("caster event count mismatch "+std::to_string(index)+": "+cast.events.dump());
  for(size_t n=0;n<expected.size();++n){auto actual=cast.events[n],reference=expected[n];
   if(actual.at("kind")=="frame"){
    std::ostringstream name;name<<std::setw(3)<<std::setfill('0')<<actual.at("sprite").get<int>()<<".bin";
    std::ifstream f(assets/"raw/Unit_atk.e5"/name.str(),std::ios::binary);std::string pixels(4096,'\0');
    f.seekg(actual.at("frame").get<int>()*4096);f.read(pixels.data(),pixels.size());if(!f)throw std::runtime_error("missing caster pixels");
    if(actual.at("mirror").get<bool>())for(int y=0;y<64;++y)std::reverse(pixels.begin()+y*64,pixels.begin()+(y+1)*64);
    if(sha256(pixels)!=reference.at("pixels_sha256").get<std::string>())throw std::runtime_error("caster pixels mismatch "+std::to_string(index));
    for(auto key:{"archive","sprite","frame","mirror"})actual.erase(key);reference.erase("matches");reference.erase("pixels_sha256");
   }
   if(actual!=reference)throw std::runtime_error("caster event mismatch "+std::to_string(index)+"/"+std::to_string(n)+": "+actual.dump()+" != "+reference.dump());
  }
  for(const auto& r:c.at("before").at("units"))if(r.at("battle").at("slot")!=35&&r!=row("after",r.at("battle").at("slot")))throw std::runtime_error("caster animation unexpectedly changed another unit");
  ++index;
 }
 auto windows=read_json(fixtures/"original_caption_window.json");
 for(const auto& c:windows.at("cases")){
  auto events=tactic_caption_window(c.at("rectangle"));auto expected=c.at("events");
  for(auto& e:expected)if(e.at("kind")=="caption_tile"){
   std::ifstream f(assets/"raw/Mark.e5/000.bin",std::ios::binary);std::string pixels(2304,'\0');f.seekg(e.at("offset").get<int>());f.read(pixels.data(),pixels.size());
   if(!f||sha256(pixels)!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("caption background pixels differ");e.erase("pixels_sha256");
  }
  if(events!=expected)throw std::runtime_error("caption border/tile requests differ");
 }
 std::cout<<index<<" original tactic caster/GBK caption sequences and indexed frames; "<<windows.at("cases").size()<<" caption windows passed.\n";
 auto poison=read_json(fixtures/"original_tactic_effect.json"),resource=read_json(assets/"poison-effect.json");
 std::ifstream pf(assets/"raw/Meff.e5/023.bin",std::ios::binary);std::string raw_effect(std::istreambuf_iterator<char>(pf),{});
 if(sha256(raw_effect)!=poison.at("resource_sha256").get<std::string>())throw std::runtime_error("poison resource hash mismatch");
 auto check_reaction=[&](Json actual,Json expected){
  if(actual.size()!=expected.size())throw std::runtime_error("poison reaction event count");
  for(size_t n=0;n<actual.size();++n){auto& a=actual[n];auto& e=expected[n];if(a.at("kind")=="frame"){
    std::ostringstream name;name<<std::setw(3)<<std::setfill('0')<<a.at("sprite").get<int>()<<".bin";
    std::ifstream f(assets/"raw/Unit_spc.e5"/name.str(),std::ios::binary);std::vector<uint8_t> pixels(2304);f.seekg(a.at("frame").get<int>()*2304);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());if(!f)throw std::runtime_error("missing poison reaction frame");
    if(a.at("mirror").get<bool>())for(int y=0;y<48;++y)std::reverse(pixels.begin()+y*48,pixels.begin()+(y+1)*48);
    pixels=transform_combat_pixels(std::move(pixels),a.at("effects"),quantized,lookup);
    if(sha256(std::string(pixels.begin(),pixels.end()))!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("poison reaction indexed pixels: "+a.dump()+" vs "+e.dump());
    for(auto k:{"archive","sprite","frame","mirror","effects"})a.erase(k);e.erase("pixels_sha256");e.erase("matches");
   }if(a!=e)throw std::runtime_error("poison reaction sequence mismatch: "+a.dump()+" != "+e.dump());
  }
 };
 for(const auto& c:poison.at("reactions")){
  const auto& t=c.at("target");check_reaction(poison_reaction(BattleUnit::from_json(t.at("battle")),UnitRecord::from_json(t.at("unit")),c.at("damage"),c.at("effect"),c.at("camera")),c.at("events"));
 }
 for(const auto& c:poison.at("cases")){
  const auto& rows=c.at("before").at("units");const auto& t=*std::find_if(rows.begin(),rows.end(),[](const auto& u){return u.at("battle").at("slot")==20;});
  auto actual=poison_effect_animation(BattleUnit::from_json(t.at("battle")),UnitRecord::from_json(t.at("unit")),c.at("damage"),c.at("effect"),c.at("before").at("camera"),20,20,resource);
  Json expected=Json::array();std::vector<uint8_t> bg,composed;int frame=0;bool after_effect=false;
  for(auto e:c.at("events")){
   auto kind=e.at("kind");if(kind=="capture"){
    bg.clear();const auto& r=e.at("rectangle");for(int yy=0;yy<90;++yy)for(int xx=0;xx<75;++xx)bg.push_back(((r.at(0).get<int>()+xx)*7+(r.at(1).get<int>()+yy)*13)&255);
   }
   if(kind=="effect"){
    int offset=resource.at("frames").at(frame).at("pixel_offset");std::vector<uint8_t> pixels(raw_effect.begin()+offset,raw_effect.begin()+offset+6750);
    if(sha256(std::string(pixels.begin(),pixels.end()))!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("poison frame pixels differ");
    composed=blend_poison_pixels(pixels,bg,quantized,lookup);e["frame"]=frame++;after_effect=true;
   }else if(kind=="raster"){
    const auto& pixels=after_effect?composed:bg;
    if(sha256(std::string(pixels.begin(),pixels.end()))!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("poison original compositor mismatch");
    if(after_effect){after_effect=false;continue;}
    e["kind"]="effect_restore";e.erase("pixels_sha256");
   }
   expected.push_back(e);
  }
  check_reaction(actual,expected);
  if(frame!=13)throw std::runtime_error("incomplete poison frames");
 }
 std::vector<uint8_t> ps(65536),pb(65536);for(int n=0;n<65536;++n){ps[n]=n/256;pb[n]=n%256;}
 auto result=blend_poison_pixels(ps,pb,quantized,lookup);
 if(sha256(std::string(result.begin(),result.end()))!=poison.at("blend_pairs_sha256").get<std::string>())throw std::runtime_error("poison exhaustive palette blend mismatch");
 std::cout<<poison.at("cases").size()<<" original poison players, "<<poison.at("reactions").size()<<" reaction sprites and 65536 compositing pairs passed.\n";

 auto confusion=read_json(fixtures/"original_confusion.json"),status=read_json(fixtures/"original_status_event.json");
 auto confusion_meta=read_json(assets/"effect-48.json");
 std::ifstream cf(assets/"raw/Meff.e5/048.bin",std::ios::binary);std::vector<uint8_t> confusion_pixels(std::istreambuf_iterator<char>(cf),{});
 if(sha256(std::string(confusion_pixels.begin(),confusion_pixels.end()))!=confusion.at("resource_sha256").get<std::string>())throw std::runtime_error("MEFF48 source bytes changed");
 for(const auto& pair:confusion.at("blends")){
  auto pixels=blend_poison_pixels(ps,pb,quantized,lookup,pair.at("weight"),pair.at("amount"));
  if(sha256(std::string(pixels.begin(),pixels.end()))!=pair.at("pixels_sha256").get<std::string>())throw std::runtime_error("zero-amount compositor pairs differ");
 }
 // The zero-weight player bypasses that compositor: background first, then
 // transparent raw indices. Verify both raster calls, not an invented blend.
 auto direct=direct_meff_pixels(ps,pb);for(size_t n=0;n<direct.size();++n)if(direct[n]!=(ps[n]?ps[n]:pb[n]))throw std::runtime_error("direct MEFF palette indices changed");
 std::vector<BattleUnit> confused;for(const auto& row:status.at("before").at("units"))confused.push_back(BattleUnit::from_json(row.at("battle")));
 auto camera=status.at("before").at("camera").get<std::array<int,2>>();
 auto sequence=scripted_area_confusion(confused,{0,0,19,19},5,camera,20,20,confusion_meta);
 const auto& requests=status.at("commands").at(0).at("events");size_t ri=0;int effects=0,direct_frames=0;std::vector<uint8_t> captured;
 auto take=[&](){if(ri>=requests.size())throw std::runtime_error("missing original status request");return requests.at(ri++);};
 auto raster=[&](const Json& rect,const std::vector<uint8_t>& pixels){
  Json expected={{"kind","raster"},{"rectangle",rect},{"pixels_sha256",sha256(std::string(pixels.begin(),pixels.end()))}};
  auto actual=take();if(actual!=expected)throw std::runtime_error("original confusion raster differs: "+actual.dump()+" != "+expected.dump());
 };
 for(auto step:sequence){auto kind=step.at("kind");
  if(kind=="camera_request"){
   for(auto xy:battle_camera_path(camera[0],camera[1],step.at("x"),step.at("y"),20,20))camera=xy;
   step={{"kind","camera"},{"tile",{camera[0]/48,camera[1]/48}}};
  }
  if(kind=="capture"){
   captured.clear();const auto& r=step.at("rectangle");for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx)captured.push_back(((r.at(0).get<int>()+xx)*7+(r.at(1).get<int>()+yy)*13)&255);
  }
  if(kind=="effect"){
   ++effects;int offset=confusion_meta.at("frames").at(step.at("frame").get<int>()).at("pixel_offset");
   std::vector<uint8_t> pixels(confusion_pixels.begin()+offset,confusion_pixels.begin()+offset+256);
   if(step.value("direct",false)){
    ++direct_frames;raster(step.at("rectangle"),captured);raster(step.at("rectangle"),pixels);
   }else{
    Json expected=step;expected.erase("resource");expected.erase("frame");if(take()!=expected)throw std::runtime_error("confusion descriptor differs");
    raster(step.at("rectangle"),blend_poison_pixels(pixels,captured,quantized,lookup,step.at("weight"),step.at("amount")));
   }
   continue;
  }
  if(kind=="effect_restore"){raster(step.at("rectangle"),captured);continue;}
  if(kind=="tile")for(auto& b:confused)if(b.id==step.at("battle").at("id"))b=BattleUnit::from_json(step.at("battle"));
  if(take()!=step)throw std::runtime_error("confusion request order differs at "+std::to_string(ri));
 }
 if(ri!=requests.size()||effects!=169||direct_frames!=39||Json(camera)!=status.at("after").at("camera"))throw std::runtime_error("incomplete area confusion presentation");
 for(const auto& row:status.at("after").at("units")){
  auto b=std::find_if(confused.begin(),confused.end(),[&](const auto& u){return u.id==row.at("battle").at("id");});
  if(b==confused.end()||b->json()!=row.at("battle"))throw std::runtime_error("sequential confusion mutation differs");
 }
 std::cout<<effects<<" confusion frames ("<<direct_frames<<" direct), 13 ordered status changes and 327680 compositor pairs passed\n";
 auto secondary=read_json(fixtures/"original_secondary_effect.json");
 for(const auto& c:secondary.at("cases")){
  int id=c.at("resource");auto meta=read_json(assets/("effect-"+std::to_string(id)+".json"));
  std::ifstream file(assets/("raw/Meff.e5/"+(id==49?std::string("049"):std::string("045"))+".bin"),std::ios::binary);
  std::string raw(std::istreambuf_iterator<char>(file),{});
  if(sha256(raw)!=secondary.at("resources").at(std::to_string(id)).get<std::string>())throw std::runtime_error("secondary resource changed");
  auto events=secondary_effect_animation(BattleUnit::from_json(c.at("target").at("battle")),c.at("after").at("camera"),meta);
  Json expected=Json::array();bool active=false,painted=false;int frame=0;std::vector<uint8_t> bg,composed;
  for(auto e:c.at("events")){
   auto kind=e.at("kind");if(kind=="archive")active=true;if(kind=="overlay")break;if(!active)continue;
   if(kind=="capture"){
    bg.clear();const auto& r=e.at("rectangle");for(int yy=0;yy<r.at(3);++yy)for(int xx=0;xx<r.at(2);++xx)bg.push_back(((r.at(0).get<int>()+xx)*7+(r.at(1).get<int>()+yy)*13)&255);
   }else if(kind=="effect"){
    int offset=meta.at("frames").at(frame).at("pixel_offset"),size=meta.at("width").get<int>()*meta.at("height").get<int>();
    std::vector<uint8_t> pixels(raw.begin()+offset,raw.begin()+offset+size);
    if(sha256(std::string(pixels.begin(),pixels.end()))!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("secondary original pixels differ");
    composed=blend_poison_pixels(pixels,bg,quantized,lookup,e.at("weight"),e.at("amount"));e["frame"]=frame++;e["resource"]=id;painted=true;
   }else if(kind=="raster"){
    const auto& pixels=painted?composed:bg;
    if(sha256(std::string(pixels.begin(),pixels.end()))!=e.at("pixels_sha256").get<std::string>())throw std::runtime_error("secondary original indexed compositor differs");
    if(painted){painted=false;continue;}e["kind"]="effect_restore";e.erase("pixels_sha256");
   }
   expected.push_back(e);
  }
  if(events!=expected)throw std::runtime_error("secondary effect order/rectangles/sound/timing mismatch: "+events.dump()+" != "+expected.dump());
 }
 for(const auto& pair:secondary.at("blends")){
  auto pixels=blend_poison_pixels(ps,pb,quantized,lookup,pair.at("weight"),pair.at("amount"));
  if(sha256(std::string(pixels.begin(),pixels.end()))!=pair.at("pixels_sha256").get<std::string>())throw std::runtime_error("secondary exhaustive palette pairs mismatch");
 }
 std::cout<<secondary.at("cases").size()<<" original secondary players and "<<secondary.at("blends").size()*65536<<" original compositing pairs passed.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
