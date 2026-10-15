// Dialog 166 and pages 178/179/200/205/284, in original dialog units.
// Keep presentation state separate from the portable battle snapshot.
SDL_FRect unit_button{640,1080,156,32};
constexpr float detail_x=108,detail_y=242;
SDL_FRect dr(float x,float y,float w,float h){return {detail_x+x*4,detail_y+y*3,w*4,h*3};}
const SDL_FRect unit_close=dr(114,189,45,13),unit_previous=dr(4,189,40,13),unit_next=dr(44,189,40,13);
const SDL_FRect spells_previous=dr(92,172,36,11),spells_next=dr(137,172,36,11);
constexpr SDL_FRect detail_equip_button{340,870,280,36};
SDL_FRect unit_tab_rect(int tab){return tab<3?dr(84+tab*33,detail_tab<3?17:5,33,12):dr(84+(tab-3)*49.5f,detail_tab<3?5:17,49.5f,12);}
Json detail_metadata;
int equipment_slot_ui=-1,equipment_selection=0,equipment_page=0;
std::string equipment_notice;
void detail_text(const std::string& s,float x,float y,float right,SDL_Color color=ink,int lines=1,float scale=1.5f){
 if(battle_language.english()){english_text(s,x,y,right,color,lines,scale,18*scale);return;}
 auto* atlas=texture("images/font.bmp");SDL_SetTextureColorMod(atlas,color.r,color.g,color.b);float left=x;int line=1,cell=font.at("cell");
 for(auto cp:codepoints(s)){
  if(cp==10){if(++line>lines)break;x=left;y+=18*scale;continue;}
  const auto& g=font.at("glyphs").at(std::to_string(cp));float advance=g.at("advance").get<float>()*scale;
  if(x+advance>right){if(++line>lines)break;x=left;y+=18*scale;}
  SDL_FRect from{g.at("x"),g.at("y"),float(cell),float(cell)},to{x,y,cell*scale,cell*scale};text_glyph(atlas,from,to);x+=advance;
 }
}
void dt(const std::string& s,float x,float y,float w,int lines=1,SDL_Color c=ink){auto r=dr(x,y,w,8);detail_text(s,r.x,r.y,r.x+r.w,c,lines);}
void detail_paper(SDL_FRect r){
 SDL_Rect old;bool clipped=SDL_RenderClipEnabled(renderer);SDL_GetRenderClipRect(renderer,&old);SDL_Rect clip{int(r.x),int(r.y),int(r.w),int(r.h)};SDL_SetRenderClipRect(renderer,&clip);
 for(float y=detail_y+std::floor((r.y-detail_y)/96)*96;y<r.y+r.h;y+=96)for(float x=detail_x+std::floor((r.x-detail_x)/96)*96;x<r.x+r.w;x+=96)image("unit-details/images/dialog-pattern.bmp",{x,y,96,96});
 SDL_SetRenderClipRect(renderer,clipped?&old:nullptr);
}
void detail_button(SDL_FRect r,bool selected=false){detail_paper(r);fill(r.x,r.y,r.w,2,white);fill(r.x,r.y,2,r.h,white);fill(r.x+r.w-2,r.y,2,r.h,{150,150,150,255});if(!selected)fill(r.x,r.y+r.h-2,r.w,2,{150,150,150,255});}
void detail_border(SDL_FRect r){fill(r.x,r.y,r.w,1,{150,150,150,255});fill(r.x,r.y,1,r.h,{150,150,150,255});fill(r.x+r.w,r.y,1,r.h,white);fill(r.x,r.y+r.h,r.w,1,white);}
void detail_group(float x,float y,float w,float h,const std::string& title){auto r=dr(x,y+3,w,h-3);detail_border(r);auto t=dr(x+4,y,w-8,9);detail_paper({t.x-2,t.y,std::min(t.w,text_width(title,1.5f))+4,26});detail_text(title,t.x,t.y,t.x+t.w);}
void detail_bar(float x,float y,float w,int value,int maximum,SDL_Color color,const std::string& label){auto r=dr(x,y,w,8);detail_border(r);float amount=maximum>0?std::clamp(float(value)/maximum,0.f,1.f):0;
 fill(r.x+1,r.y+2,(r.w-2)*amount,r.h-3,color);fill(r.x+1,r.y+4,(r.w-2)*amount,2,white);
 auto characters=codepoints(label);float extent=0;for(auto cp:characters)extent+=font.at("glyphs").at(std::to_string(cp)).at("advance").get<float>()*1.5f;
 detail_text(label,r.x+r.w-extent-4,r.y-1,r.x+r.w-2);
}
Json current_details(){return unit_details(*game,detail_unit,ui,detail_tactics);}
void open_unit_details(int id){
 if(!special_unit_in_roster(id))return;
 if(playback.active()||game->has_dialogue()||show_inventory||show_battles||show_info||show_spells||target_item>=0||target_tactic>=0)return;
 if(!game->state().at("units").contains(std::to_string(id)))return;
 if(game->state().at("units").at(std::to_string(id)).at("battle").at("hp").get<int>()<=0&&game->state().at("phase")!="victory")return;
 detail_unit=id;detail_page=0;equipment_slot_ui=-1;equipment_notice.clear();show_unit_details=true;clear_touch_hover();
}
void cycle_detail_unit(int direction){
 std::vector<int> ids;for(const auto& u:game->state().at("units"))if(special_unit_in_roster(u.at("battle").at("id"))&&(u.at("battle").at("hp").get<int>()>0||(game->state().at("phase")=="victory"&&u.at("battle").at("side")==0)))ids.push_back(u.at("battle").at("id"));
 if(ids.empty())return;std::sort(ids.begin(),ids.end());auto it=std::find(ids.begin(),ids.end(),detail_unit);int n=it==ids.end()?0:int(it-ids.begin());detail_unit=ids[(n+direction+int(ids.size()))%ids.size()];detail_page=0;equipment_slot_ui=-1;equipment_notice.clear();
}
void detail_page_change(int direction){int pages=std::max(1,(int(current_details().at("tactics").size())+9)/10);detail_page=std::clamp(detail_page+direction,0,pages-1);}
std::vector<int> equipment_choices(){std::vector<int> result{-1};for(size_t n=0;n<game->equipment_bag().size();++n)if(game->equipment_compatible(detail_unit,equipment_slot_ui,game->equipment_bag()[n][0]))result.push_back(int(n));return result;}
SDL_FRect gear_row(int row){return {186,370.f+row*44,584,40};}
constexpr SDL_FRect gear_confirm{552,794,218,38},gear_cancel{186,794,160,38};
void open_equipment_picker(int slot){auto why=game->equipment_block_reason(detail_unit);if(!why.empty()){equipment_notice=why;return;}equipment_slot_ui=slot;equipment_selection=0;equipment_page=0;equipment_notice.clear();}
void confirm_equipment(){
 try{auto choices=equipment_choices();game->change_equipment(detail_unit,equipment_slot_ui,choices.at(equipment_selection));cancel_jev();diagnostic_dirty=true;log_presentation({{"event","equipment_change"},{"actor",detail_unit},{"slot",equipment_slot_ui},{"state_hash",game->hash()}});equipment_slot_ui=-1;equipment_notice="装备已更换。";}catch(const std::exception& e){equipment_notice=e.what();}
}
void detail_click(float x,float y,bool right){
 if(equipment_slot_ui>=0){
  if(right||inside(x,y,gear_cancel)){equipment_slot_ui=-1;return;}
  for(int n=0;n<3;++n)if(inside(x,y,{186.f+n*198,312,188,36})){open_equipment_picker(n);return;}
  auto choices=equipment_choices();for(int n=0;n<7;++n)if(equipment_page*7+n<int(choices.size())&&inside(x,y,gear_row(n))){equipment_selection=equipment_page*7+n;return;}
  if(inside(x,y,{186,742,110,32}))equipment_page=std::max(0,equipment_page-1);
  if(inside(x,y,{660,742,110,32}))equipment_page=std::min(int(choices.size()-1)/7,equipment_page+1);
  if(inside(x,y,gear_confirm))confirm_equipment();return;
 }
 if(right||inside(x,y,unit_close)||inside(x,y,{810,212,32,26})){show_unit_details=false;return;}
 for(int tab=0;tab<5;++tab)if(inside(x,y,unit_tab_rect(tab))){detail_tab=tab;detail_page=0;return;}
 if(inside(x,y,unit_previous))cycle_detail_unit(-1);else if(inside(x,y,unit_next))cycle_detail_unit(1);
 else if(detail_tab==2){if(inside(x,y,spells_previous))detail_page_change(-1);if(inside(x,y,spells_next))detail_page_change(1);}
 if(detail_tab==1&&inside(x,y,detail_equip_button))open_equipment_picker(0);
}
void detail_key(SDL_Keycode key,bool shift){
 if(equipment_slot_ui>=0){if(key==SDLK_ESCAPE){equipment_slot_ui=-1;return;}auto choices=equipment_choices();if(key==SDLK_UP||key==SDLK_DOWN){equipment_selection=std::clamp(equipment_selection+(key==SDLK_UP?-1:1),0,int(choices.size())-1);equipment_page=equipment_selection/7;}if(key==SDLK_RETURN)confirm_equipment();if(key==SDLK_TAB)open_equipment_picker((equipment_slot_ui+(shift?2:1))%3);return;}
 if(key==SDLK_ESCAPE||key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_V){show_unit_details=false;return;}
 if(key==SDLK_TAB||key==SDLK_LEFT||key==SDLK_RIGHT){detail_tab=(detail_tab+(key==SDLK_LEFT||(key==SDLK_TAB&&shift)?4:1))%5;detail_page=0;}
 if(key>=SDLK_1&&key<=SDLK_5){detail_tab=int(key-SDLK_1);detail_page=0;}
 if(key==SDLK_UP)cycle_detail_unit(-1);if(key==SDLK_DOWN)cycle_detail_unit(1);
 if(detail_tab==2){if(key==SDLK_PAGEUP)detail_page_change(-1);if(key==SDLK_PAGEDOWN)detail_page_change(1);}
 if(key==SDLK_G&&detail_tab==1)open_equipment_picker(0);
}
void draw_equipment_picker(){
 fill(0,0,width,height,{0,0,0,90});bevel({168,256,620,590});ribbon(178,266,600,"装备更换 · "+current_details().at("name").get<std::string>());
 const char* slots[]{"武器","护具","辅助"};for(int n=0;n<3;++n){SDL_FRect r{186.f+n*198,312,188,36};bevel(r,n==equipment_slot_ui);text(slots[n],r.x+70,r.y+8,r.x+180);}
 auto choices=equipment_choices();equipment_selection=std::clamp(equipment_selection,0,int(choices.size())-1);
 for(int row=0;row<7&&equipment_page*7+row<int(choices.size());++row){int index=equipment_page*7+row,bag=choices[index];auto r=gear_row(row);bevel(r,index==equipment_selection);
  if(bag<0)text("卸下当前装备",r.x+16,r.y+10,r.x+r.w-8);
  else {const auto& e=game->equipment_bag()[bag];image("unit-details/images/Item/"+number3(e[0])+".bmp",{r.x+6,r.y+4,32,32});text(game->data().at("items").at(e[0].get<int>()).at("name"),r.x+50,r.y+10,r.x+290);if(e[1]!=255)text("Lv "+e[1].dump()+"   Exp "+(e[2]==255?"MAX":e[2].dump()),r.x+318,r.y+10,r.x+r.w-8);}
 }
 int selected=choices[equipment_selection];std::string description=selected<0?"卸下后保留在行囊，等级和经验不会重置。":detail_metadata.at("items").at(game->equipment_bag()[selected][0].get<int>()).at("text").get<std::string>();
 text(description,188,690,766,ink,2);bevel({186,742,110,32});text("上一页",214,750,288);bevel({660,742,110,32});text("下一页",688,750,764);
 text(std::to_string(equipment_page+1)+" / "+std::to_string((choices.size()+6)/7),446,750,620);bevel(gear_cancel);text("取消 Esc",222,804,338);bevel(gear_confirm);text("确定更换",618,804,766);
 if(!equipment_notice.empty())text(equipment_notice,180,852,790,ink,2);
}
void draw_unit_details(){
 auto d=current_details();const auto& raw=game->state().at("units").at(std::to_string(detail_unit));auto r=UnitRecord::from_json(raw.at("record"));auto b=BattleUnit::from_json(raw.at("battle"));
 fill(0,0,width,height,{0,0,0,120});bevel({104,206,752,659});ribbon(108,210,744,"武将情报");bevel({810,212,32,26});text("×",820,215,842);
 detail_paper({detail_x,detail_y,744,619});
 auto portrait=dr(3,3,38,57);image("images/Face/"+number3(d.at("face"))+".bmp",{portrait.x+12,portrait.y+5,128,160});
 dt(d.at("name"),46,9,36);dt("[ "+d.at("side").get<std::string>()+" ]",46,21,37);
 detail_group(2,62,77,41,"部队属性");dt(d.at("class"),6,76,69);dt("Lv "+d.at("level").dump(),8,89,22);dt("Exp",31,89,13);
 detail_bar(44,89,31,d.at("xp").is_null()?100:d.at("xp").get<int>(),100,{25,65,230,255},d.at("xp").is_null()?"MAX":d.at("xp").dump()+"/100");
 detail_group(2,107,77,78,"状态");dt("HP",6,118,12);detail_bar(18,118,57,d.at("hp"),d.at("max_hp"),{26,218,25,255},d.at("hp").dump()+"/"+d.at("max_hp").dump());
 dt("MP",6,131,12);detail_bar(18,131,57,d.at("mp"),d.at("max_mp"),{221,221,0,255},d.at("mp").dump()+"/"+d.at("max_mp").dump());detail_border(dr(7,146,67,33));dt(d.at("status"),7,146,67,3);
 detail_border(dr(83,29,100,156));const char* tabs[]{"能力","装备","策略","武将列传","部队特性"};
 for(int n=0;n<5;++n){auto tab=unit_tab_rect(n);detail_button(tab,n==detail_tab);detail_text(tabs[n],battle_language.english()?tab.x+8:tab.x+(tab.w-float(codepoints(tabs[n]).size())*24)/2,tab.y+3,tab.x+tab.w-2);}
 constexpr float cx=84,cy=30;
 if(detail_tab==0){
  for(int n=0;n<5;++n){const auto& stat=d.at("stats").at(std::array{0,2,1,3,4}[n]);detail_group(cx+8,cy+5+n*24,79,22,stat.at("name"));detail_bar(cx+16,cy+15+n*24,63,stat.at("value"),500,{5,20,240,255},stat.at("value").dump());}
  detail_group(cx+8,cy+125,79,22,"移动力");dt(d.at("movement").dump(),cx+73,cy+135,10);
 }else if(detail_tab==1){
  for(int n=0;n<3;++n){const auto& e=d.at("equipment")[n];float y=cy+4+n*50;
   dt(e.at("slot").get<std::string>()+"："+e.at("name").get<std::string>(),cx+11,y,80);if(e.at("id")!=255)image("unit-details/images/Item/"+number3(e.at("id"))+".bmp",dr(cx+11,y+11,20,20));
   if(n<2&&!e.at("level").is_null()){dt("Lv",cx+37,y+12,10);dt(e.at("level").dump(),cx+52,y+12,20);dt("Exp",cx+37,y+22,13);detail_bar(cx+52,y+22,33,e.at("xp").is_null()?100:e.at("xp").get<int>(),100,{5,20,240,255},e.at("xp").is_null()?"MAX":e.at("xp").dump()+"/100");}
   std::string bonuses;for(const auto& v:e.at("bonuses")){if(!bonuses.empty())bonuses+="  ";bonuses+=v.at("name").get<std::string>()+" +"+v.at("value").dump();}
   if(n==2&&e.at("id")!=255){const auto& item=game->data().at("items").at(e.at("id").get<int>());int type=item.at("type");bonuses=detail_metadata.at("equipment_effects").at(type-18).get<std::string>();int effect=type-18,strength=item.at("strength");if(effect>=6&&effect<=12||effect==14)bonuses+=" +"+std::to_string(strength);else if(effect==21||effect>=34&&effect<=37)bonuses+=" +"+std::to_string(strength)+"%";else if(effect==41||effect==43)bonuses+=" -"+std::to_string(strength)+"%";}
   auto line=dr(cx+13,y+35,72,8);detail_text(bonuses,line.x,line.y,line.x+line.w,ink,1,1.25f);if(n<2)detail_border(dr(cx+2,cy+49+n*51,92,0));
  }
 }else if(detail_tab==2){
  const auto& spells=d.at("tactics");int pages=std::max(1,(int(spells.size())+9)/10);detail_page=std::clamp(detail_page,0,pages-1);detail_border(dr(cx+8,cy+1,79,138));
  for(int n=0;n<10&&detail_page*10+n<int(spells.size());++n){const auto& t=spells[detail_page*10+n];dt(t.at("name"),cx+12,cy+5+n*13,54);dt(t.at("mp_cost").dump(),cx+72,cy+5+n*13,12);}
  if(spells.empty())dt("尚未习得策略",cx+10,cy+10,79);
  bevel(spells_previous);bevel(spells_next);detail_text("上一页",spells_previous.x+20,spells_previous.y+5,spells_previous.x+spells_previous.w);detail_text("下一页",spells_next.x+20,spells_next.y+5,spells_next.x+spells_next.w);
 }else if(detail_tab==3){
  detail_group(cx+7,cy+4,81,45,"基本能力");const char* names[]{"武力","统率","智力","敏捷","运气"};
  for(int n=0;n<5;++n){int column=n>=3;float x=cx+(column?51:15),y=cy+(column?14+(n-3)*11:14+std::array{0,2,1}[n]*11);auto label=dr(x,y,column?17:19,8);detail_text(names[n],label.x,label.y,label.x+label.w,ink,1,1.25f);dt(std::to_string((r.base_stats[n]*2)&255),cx+(column?70:34),y,16);}
  detail_border(dr(cx+7,cy+52,81,83));if(detail_unit<174)dt(detail_metadata.at("biographies").at(detail_unit).at("text"),cx+7,cy+52,81,9);
  if(b.side==0){auto label=dr(cx+7,cy+140,81,10);detail_text("出阵数 "+std::to_string(r.record_counters[0])+"  撤退数 "+std::to_string(r.record_counters[2]),label.x,label.y,label.x+label.w,ink,1,1.1f);}
 }else{
  const auto& c=game->data().at("classes").at(r.job);int sprite=battle_sprite(r.id,r.job,b.side);SDL_FRect crop{0,0,48,48};image("images/Unit_mov/"+number3(sprite)+".bmp",dr(cx+8,cy+1,28,36),&crop);
  dt(d.at("class"),cx+41,cy+3,49);
  auto gear_type=[&](int first,int last){for(int n=first;n<last;++n)if(c.at(9+n*2)!=0)return detail_metadata.at("equipment_types").at(n).get<std::string>();return std::string("无");};
  dt("武器",cx+41,cy+15,24);dt(gear_type(0,7),cx+66,cy+15,27);dt("护具",cx+41,cy+25,24);dt(gear_type(7,9),cx+66,cy+25,27);
  for(int n=0;n<5;++n){int stat=std::array{0,2,1,3,4}[n];auto label=dr(cx+8,cy+40+n*10,25,8);detail_text(d.at("stats").at(stat).at("name"),label.x,label.y,label.x+label.w,ink,1,1.25f);dt(detail_metadata.at("grades").at(c.at(stat+2).get<int>()),cx+34,cy+40+n*10,12);}
  dt("移动力",cx+8,cy+90,26);dt(c.at(0).dump(),cx+34,cy+90,10);dt("攻击范围",cx+48,cy+41,44);
  image("unit-details/images/Hitarea/"+number3(c.at(1))+".bmp",dr(cx+50,cy+51,39,45));dt(detail_metadata.at("classes").at(r.job).at("text"),cx+7,cy+102,82,5);
 }
 detail_button(unit_previous);detail_button(unit_next);detail_button(unit_close);detail_text("上一武将",unit_previous.x+14,unit_previous.y+6,unit_previous.x+unit_previous.w);detail_text("下一武将",unit_next.x+14,unit_next.y+6,unit_next.x+unit_next.w);detail_text("确定",unit_close.x+64,unit_close.y+6,unit_close.x+unit_close.w);
 if(detail_tab==1){bevel(detail_equip_button);text("装备更换 G · 战后整备",detail_equip_button.x+44,detail_equip_button.y+9,detail_equip_button.x+272,game->equipment_block_reason(detail_unit).empty()?ink:SDL_Color{130,130,130,255});}
 if(!equipment_notice.empty())text(equipment_notice,128,918,832,ink,2);
 if(equipment_slot_ui>=0)draw_equipment_picker();
}
