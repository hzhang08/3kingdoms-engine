// Presentation-only voxel edition. Existing battle rules, event ordering,
// animation packets, RNG and snapshot identities remain authoritative.
// Each source frame has a ground pivot. Cropping transparency never changes
// its anchor, including when a west-facing pose is mirrored to face east.
struct VoxelFrame {const Json* meta;bool mirror;};
void load_voxel_camps(){
 const auto folder=assets/"special-hulao/camps-v1";voxel_camps=read_json(folder/"manifest.json");
 if(voxel_camps.at("version")!="hulao-camp-presentation-v1"||voxel_camps.at("background")!="terrain.bmp"||voxel_camps.at("tent").at("file")!="tent.rgba")throw std::runtime_error("Invalid camp presentation assets");
 for(const auto* name:{"terrain.bmp","tent.rgba"}){
  std::ifstream f(folder/name,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
  if(sha256(raw)!=voxel_camps.at("assets").at(name).get<std::string>())throw std::runtime_error("Camp presentation checksum mismatch");
 }
 texture("special-hulao/camps-v1/tent.rgba");
}
SDL_FRect voxel_camp_rect(int x,int y){
 const auto& t=voxel_camps.at("tent");auto center=voxel::project(x+.5f,y+.5f);
 float scale=t.at("render_width").get<float>()/t.at("w").get<float>();
 return {center.x-t.at("pivot").at(0).get<float>()*scale,center.y-t.at("pivot").at(1).get<float>()*scale,t.at("w").get<float>()*scale,t.at("h").get<float>()*scale};
}
std::array<int,2> voxel_camp_at(float x,float y){
 const auto& t=voxel_camps.at("tent");const int w=t.at("w"),h=t.at("h");
 const auto& raw=voxel_pixels.at("camps-v1/tent.rgba");
 for(int p=0;p<400;++p)if(terrain_recovery_percent(game->terrain().at("cells").at(p))){
  auto r=voxel_camp_rect(p%20,p/20);if(!inside(x,y,r))continue;
  int px=std::clamp(int((x-r.x)*w/r.w),0,w-1),py=std::clamp(int((y-r.y)*h/r.h),0,h-1);
  if(raw[(py*w+px)*4+3]>80)return {p%20,p/20};
 }
 return {-1,-1};
}
VoxelFrame voxel_frame(const Json& unit,const Json* pose=nullptr){
 auto b=BattleUnit::from_json(unit.at("battle"));std::string group="move";
 int frame=battle_frame(b,unit.at("max_hp"),int(SDL_GetTicks()/180)%8);bool mirror=b.direction==1;
 if(pose){const auto& archive=pose->at("archive");
  if(archive=="UNIT_ATK.E5")group="attack";else if(archive=="UNIT_SPC.E5")group="special";
  else if(archive!="UNIT_MOV.E5")throw std::runtime_error("Unsupported voxel animation archive");
  frame=pose->at("frame");mirror=pose->at("mirror");
 }
 const auto& frames=voxel_manifest.at("units").at(std::to_string(b.id)).at("frames").at(group);
 if(frame<0||frame>=int(frames.size()))throw std::runtime_error("Missing voxel animation frame");
 return {&frames.at(frame),mirror};
}
SDL_FRect voxel_sprite_rect(const Json& unit,float x,float y,const Json* pose=nullptr){
 auto p=voxel::project(x+.5f,y+.5f);auto f=voxel_frame(unit,pose);const auto& frame=*f.meta;
 const auto& meta=voxel_manifest.at("units").at(unit.at("battle").at("id").dump());
 float scale=meta.at("height").get<float>()/meta.at("body_height").get<float>();
 float w=frame.at("w"),h=frame.at("h"),px=frame.at("pivot").at(0),py=frame.at("pivot").at(1);
 if(f.mirror)px=w-px;
 return {p.x-px*scale,p.y-py*scale,w*scale,h*scale};
}
int voxel_unit_at(float x,float y){
 auto c=voxel::cell(x,y);int id=-1;float depth=-1;
 for(const auto& u:visible_state().at("units")){
  auto b=BattleUnit::from_json(u.at("battle"));if(b.hp<=0||b.lifecycle!=2)continue;
  if(c[0]==b.x&&c[1]==b.y)return b.id;
  auto r=voxel_sprite_rect(u,b.x,b.y);if(!inside(x,y,r))continue;
  auto frame=voxel_frame(u);const auto& meta=*frame.meta;
  auto& raw=voxel_pixels.at(meta.at("file").get<std::string>());
  int w=meta.at("w"),h=meta.at("h");
  int px=std::clamp(int((x-r.x)/r.w*w),0,w-1),py=std::clamp(int((y-r.y)/r.h*h),0,h-1);
  if(frame.mirror)px=w-1-px;
  if(raw[(py*w+px)*4+3]>80&&b.y>depth){id=b.id;depth=b.y;}
 }
 return id;
}
void voxel_cell(int x,int y,SDL_Color color,bool solid){
 SDL_Vertex v[4]{};int n=0;
 for(auto d:{std::array{0,0},std::array{1,0},std::array{1,1},std::array{0,1}}){auto p=voxel::project(float(x+d[0]),float(y+d[1]));v[n++]={{p.x,p.y},{color.r/255.f,color.g/255.f,color.b/255.f,color.a/255.f},{0,0}};}
 if(solid){const int indices[]{0,1,2,0,2,3};SDL_RenderGeometry(renderer,nullptr,v,4,indices,6);}
 // Solid geometry keeps thick borders visible in fitted phone/desktop views.
 auto outline=[&](SDL_Color stroke,float thickness){
  for(int i=0;i<4;++i){auto a=v[i].position,b=v[(i+1)%4].position;
   float dx=b.x-a.x,dy=b.y-a.y,length=std::sqrt(dx*dx+dy*dy);
   float nx=-dy/length*thickness/2,ny=dx/length*thickness/2;
   SDL_FColor c{stroke.r/255.f,stroke.g/255.f,stroke.b/255.f,stroke.a/255.f};
   SDL_Vertex edge[4]{{{a.x+nx,a.y+ny},c,{0,0}},{{b.x+nx,b.y+ny},c,{0,0}},{{b.x-nx,b.y-ny},c,{0,0}},{{a.x-nx,a.y-ny},c,{0,0}}};
   const int indices[]{0,1,2,0,2,3};SDL_RenderGeometry(renderer,nullptr,edge,4,indices,6);
  }
 };
 outline({10,35,66,235},5);
 outline(solid?SDL_Color{101,229,255,245}:SDL_Color{color.r,color.g,color.b,255},2.5f);
}
void draw_voxel_map(const Json& s){
 image("special-hulao/camps-v1/terrain.bmp",{0,float(map_top),1536,1024});
 int selected=s.at("selected");
 auto range=playback.active()||action_menu_unit>=0||attack_targeting?std::vector<int>(400,255):game->movement();
 for(int p=0;p<400;++p)if(range[p]!=255)voxel_cell(p%20,p/20,{0,108,255,145},true);
 // Tent artwork, healing outline and hit target share one terrain cell anchor.
 for(int p=0;p<400;++p){int rate=terrain_recovery_percent(game->terrain().at("cells").at(p));if(!rate)continue;
  const int x=p%20,y=p/20;voxel_cell(x,y,{40,240,123,65},true);voxel_cell(x,y,{60,231,116,255},false);
  image("special-hulao/camps-v1/tent.rgba",voxel_camp_rect(x,y));
  auto center=voxel::project(x+.5f,y+.5f);auto label=battle_language.english()?"Camp +"+std::to_string(rate)+"%":"兵营 +"+std::to_string(rate)+"%";
  const float w=battle_language.english()?158:148;
  bool occupied=false;for(const auto& u:s.at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==2&&b.at("x")==x&&b.at("y")==y){occupied=true;break;}}
  if(!occupied){fill(center.x-w/2,center.y+40,w,28,{16,68,39,245});text(label,center.x-w/2+6,center.y+45,center.x+w/2-4,{215,255,224,255});}
 }
 if(game->hulao_gates_open()){
  for(int y:{9,10})voxel_cell(6,y,{255,214,68,255},false);
  auto entry=voxel::project(7.05f,10.f);bevel({entry.x,entry.y-14,68,28});
  text(battle_language.english()?"Gate":"城门",entry.x+6,entry.y-10,entry.x+64);
 }
 auto cell=map_cell(mouse_x,mouse_y);
 if(cell[0]>=0&&!playback.active()&&!showing_dialogue())voxel_cell(cell[0],cell[1],{255,243,145,150},false);
 std::vector<const Json*> units;for(const auto& u:s.at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.hp>0&&b.lifecycle==2)units.push_back(&u);}
 std::stable_sort(units.begin(),units.end(),[](auto a,auto b){return a->at("battle").at("y")<b->at("battle").at("y");});
 int hovered=voxel_unit_at(mouse_x,mouse_y);
 for(const auto* up:units){const auto& u=*up;auto b=BattleUnit::from_json(u.at("battle"));float x=b.x,y=b.y;const Json* pose=nullptr;
  if(playback.poses().contains(std::to_string(b.slot))){pose=&playback.poses().at(std::to_string(b.slot));float size=pose->at("width");x=(pose->at("x").get<float>()+size/2)/48-.5f;y=(pose->at("y").get<float>()-56+size/2)/48-.5f;}
  bool active=pose&&playback.active();auto r=voxel_sprite_rect(u,x,y,pose);auto foot=voxel::project(x+.5f,y+.5f);
  bool target=selected>=0&&game->attackable(selected,b.id)&&target_tactic<0&&target_item<0;
  if(target_item>=0&&selected>=0)for(const auto& o:game->item_options(selected,target_item))if(o.at("target")==b.id)target=true;
  if(target_tactic>=0&&selected>=0){const auto& a=s.at("units").at(std::to_string(selected)).at("battle");for(const auto& o:game->spell_options(selected,{a.at("x"),a.at("y")}))if(o.at("tactic")==target_tactic&&o.at("target")==b.id)target=true;}
  if(b.id==selected||target||b.id==hovered)voxel_cell(b.x,b.y,b.id==selected?SDL_Color{255,215,54,180}:target?SDL_Color{255,75,58,180}:SDL_Color{255,255,255,130},false);
  // Small team bases and shadows make overlapping miniatures readable.
  fill(foot.x-21,foot.y-4,42,6,{24,31,16,75});fill(foot.x-15,foot.y-1,30,3,b.side==0?SDL_Color{223,58,47,210}:b.side==1?SDL_Color{240,195,60,210}:SDL_Color{65,124,226,210});
  auto frame=voxel_frame(u,pose);auto* t=texture("special-hulao/"+frame.meta->at("file").get<std::string>());
  Uint8 shade=(b.flags&2)?170:255;SDL_SetTextureColorMod(t,shade,shade,shade);
  // Authored frames supply the gait/weapon motion; no procedural tilt replaces
  // them. The original effect events still cue a visible damage flash.
  if(active&&!pose->at("effects").empty())SDL_SetTextureColorMod(t,255,125,125);
  SDL_RenderTextureRotated(renderer,t,nullptr,&r,0,nullptr,frame.mirror?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);
  if(b.status&8)text("?",foot.x-5,r.y-18,foot.x+24,{255,224,40,255});
  if(b.status&16)text("毒",foot.x+10,r.y,foot.x+45,{64,236,102,255});
  const auto& recovery=playback.number();bool healing_here=playback.active()&&!recovery.is_null()&&recovery.value("healing",false)&&recovery.at("slot")==b.slot;
  if(show_health||b.id==selected||b.id==hovered||healing_here){
   int hp=healing_here?std::min(u.at("max_hp").get<int>(),b.hp+recovery.at("value").get<int>()):b.hp;
   fill(foot.x-25,foot.y+5,50,5,{25,30,24,230});fill(foot.x-25,foot.y+5,50.f*hp/u.at("max_hp").get<int>(),5,{92,215,96,255});text(u.at("name"),foot.x-30,foot.y+12,foot.x+80,{255,255,248,255});
  }
 }
 if(playback.active()){
  const auto& e=playback.spell_effect();if(!e.is_null()){
   auto* t=texture(e.value("asset_group",std::string("tactics"))+"/images/"+e.at("resource").dump()+"-"+e.at("image").dump()+".bmp");
   auto p=voxel::project((e.at("x").get<float>()+e.at("width").get<float>()/2)/48,(e.at("y").get<float>()-56+e.at("height").get<float>()/2)/48);
   SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);SDL_SetTextureAlphaMod(t,Uint8(e.at("amount").get<int>()*255/16));
   SDL_FRect r{p.x-e.at("width").get<float>(),p.y-e.at("height").get<float>(),e.at("width").get<float>()*2,e.at("height").get<float>()*1.4f};SDL_RenderTexture(renderer,t,nullptr,&r);
  }
  const auto& n=playback.number();if(!n.is_null())for(const auto* u:units)if(u->at("battle").at("slot")==n.at("slot")){auto r=voxel_sprite_rect(*u,u->at("battle").at("x"),u->at("battle").at("y"));bool healing=n.value("healing",false);auto label=(healing?"+":"-")+n.at("value").dump();if(healing)label+=" HP";text(label,r.x+1,r.y-21,r.x+160,{15,32,20,255});text(label,r.x,r.y-22,r.x+160,healing?SDL_Color{103,255,139,255}:SDL_Color{255,231,65,255});}
  if(!playback.notice().is_null()){
   bevel({480,112,576,64});auto label=playback.notice().at("text").get<std::string>();
   if(!playback.notice().contains("item")&&!n.is_null()&&n.value("healing",false)){
    const auto& u=s.at("units").at(std::to_string(playback.notice().at("actor").get<int>()));
    int hp=u.at("battle").at("hp"),gain=n.at("value"),maximum=u.at("max_hp");
    label=tr(u.at("name"))+ (battle_language.english()?" recovers HP +":" 恢复兵力 +")+std::to_string(gain)+"   "+std::to_string(hp)+" -> "+std::to_string(std::min(maximum,hp+gain))+" / "+std::to_string(maximum);
   }
   text(label,496,124,1040,ink,2);
  }
 }
}
