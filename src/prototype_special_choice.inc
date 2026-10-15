// Presentation only: no gameplay/RNG state changes until a hero is chosen.
void draw_special_choice(){
 fill(0,0,width,height,{13,23,31,248});
 const bool en=battle_language.english();
 static const Json seed=read_json(assets/"special-hulao/seed.json").at("payload").at("state").at("units");
 detail_text(en?"HULAO PASS":"虎牢关之战",108,60,1428,{255,217,133,255},1,3.1f);
 detail_text(en?"Choose your hero":"选择出战武将",108,142,1428,white,1,2.2f);
 const std::array<const char*,4> names{"Cao Cao","Xiahou Yuan","Xiahou Dun","Li Dian"};
 const std::array<const char*,4> roles{"Commander","Mounted archer","Cavalry","Infantry"};
 for(int i=0;i<4;++i){
  const auto r=special_hero_card(i);const int id=special_heroes[i];const auto& u=seed.at(std::to_string(id));
  const bool hover=inside(mouse_x,mouse_y,r);fill(r.x,r.y,r.w,r.h,hover?SDL_Color{60,76,86,255}:SDL_Color{32,46,57,255});
  SDL_SetRenderDrawColor(renderer,hover?255:138,hover?216:130,hover?130:99,255);SDL_RenderRect(renderer,&r);
  const auto& meta=voxel_manifest.at("units").at(std::to_string(id));const auto& frame=meta.at("frames").at("move").at(6);
  auto* t=texture("special-hulao/"+frame.at("file").get<std::string>());SDL_SetTextureColorMod(t,255,255,255);
  const float w=frame.at("w"),h=frame.at("h"),scale=std::min(248.f/w,258.f/h);
  SDL_FRect sprite{r.x+130-w*scale/2,r.y+45+(258-h*scale),w*scale,h*scale};SDL_RenderTexture(renderer,t,nullptr,&sprite);
  detail_text(en?names[i]:u.at("name").get<std::string>(),r.x+270,r.y+48,r.x+r.w-24,white,1,2.2f);
  int job=u.at("record").at("class");
  detail_text(en?roles[i]:ui.at("classes").at(job).at("text").get<std::string>(),r.x+270,r.y+111,r.x+r.w-24,{214,198,162,255},1,1.5f);
  detail_text("L"+u.at("record").at("level").dump()+"   HP "+u.at("max_hp").dump(),r.x+270,r.y+162,r.x+r.w-24,white,1,1.4f);
  detail_text(en?"Choose  ["+std::to_string(i+1)+"]":"出战  ["+std::to_string(i+1)+"]",r.x+270,r.y+248,r.x+r.w-24,{255,217,133,255},1,1.8f);
 }
 detail_text(en?"Dong Zhuo: you + 1 random ally. Lu Bu: all 4 heroes.":"迎战董卓：所选武将 + 随机队友（2人）。迎战吕布：全体4人出战。",108,974,1428,{255,217,133,255},1,1.35f);
 detail_text(en?"Random opponent: Lu Bu or Dong Zhuo (50% each)":"敌军随机出战：吕布 或 董卓（各 50%）",108,1014,1428,{214,198,162,255},1,1.45f);
 detail_text(en?"Load save  [F9 / F6]":"读取存档  [F9 / F6]",124,1060,670,white,1,1.55f);
 if(special_has_match)detail_text(en?"Return  [Esc]":"返回战场  [Esc]",1044,1060,1428,white,1,1.55f);
 if(!notice.empty())detail_text(notice,108,1114,1428,{255,190,130,255},1,1.4f);
}
