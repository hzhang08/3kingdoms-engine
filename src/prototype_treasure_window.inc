bool treasure_only=true;
int treasure_page=0,treasure_selected=0;
constexpr SDL_FRect treasure_button{662,54,124,28},treasure_close{730,236,94,32},treasure_owner{570,788,232,36};
SDL_FRect treasure_row(int n){return {160,324.f+n*42,640,40};}
Json owned_equipment(){
 Json items=Json::array();auto append=[&](const Json& e,int owner,int bag){if(e[0]==255)return;int id=e[0];if(treasure_only&&game->data().at("items").at(id).at("collection")==0)return;items.push_back({{"item",e},{"owner",owner},{"bag",bag}});};
 for(const auto& u:game->state().at("units"))if(u.at("battle").at("side")==0&&special_unit_in_roster(u.at("battle").at("id")))for(const auto& e:u.at("record").at("equipment"))append(e,u.at("battle").at("id"),-1);
 for(size_t n=0;n<game->equipment_bag().size();++n)append(game->equipment_bag()[n],-1,int(n));
 std::stable_sort(items.begin(),items.end(),[](const Json& a,const Json& b){return a.at("item").at(0)<b.at("item").at(0);});return items;
}
void open_treasures(){
 if(playback.active()||game->has_dialogue()||!game->story_choices().empty()||show_slots||backup_busy||show_unit_details)return;
 close_action_menu();if(!game->state().at("undo").is_null())menu_after_move_unit=game->state().at("selected");show_inventory=true;treasure_view=true;show_info=false;show_battles=false;treasure_page=treasure_selected=0;notice.clear();
}
void treasure_click(float x,float y,bool right){
 if(right||inside(x,y,treasure_close)||inside(x,y,treasure_button)){show_inventory=false;return;}
 if(inside(x,y,{160,278,176,32})){treasure_only=true;treasure_selected=treasure_page=0;return;}
 if(inside(x,y,{344,278,176,32})){treasure_only=false;treasure_selected=treasure_page=0;return;}
 if(inside(x,y,{624,278,176,32})){treasure_view=false;return;}
 auto list=owned_equipment();for(int n=0;n<8&&treasure_page*8+n<int(list.size());++n)if(inside(x,y,treasure_row(n))){treasure_selected=treasure_page*8+n;return;}
 if(inside(x,y,{160,666,110,32}))treasure_page=std::max(0,treasure_page-1);
 if(inside(x,y,{690,666,110,32}))treasure_page=std::min(std::max(0,int(list.size())-1)/8,treasure_page+1);
 if(inside(x,y,treasure_owner)){
  int actor=0;if(!list.empty()&&treasure_selected<int(list.size())&&list[treasure_selected].at("owner")!=-1)actor=list[treasure_selected].at("owner");
  show_inventory=false;open_unit_details(actor);detail_tab=1;
 }
}
void draw_treasures(){
 auto list=owned_equipment();int pages=std::max(1,(int(list.size())+7)/8);treasure_page=std::clamp(treasure_page,0,pages-1);treasure_selected=std::clamp(treasure_selected,0,std::max(0,int(list.size())-1));
 fill(0,0,width,height,{0,0,0,135});bevel({136,220,696,632});ribbon(146,234,574,"宝物与装备");bevel(treasure_close);text("关闭 Esc",740,244,820);
 for(auto [rect,label,pressed]:{std::tuple{SDL_FRect{160,278,176,32},"宝物",treasure_only},std::tuple{SDL_FRect{344,278,176,32},"全部装备",!treasure_only},std::tuple{SDL_FRect{624,278,176,32},"使用道具 P",false}}){bevel(rect,pressed);text(label,rect.x+32,rect.y+8,rect.x+rect.w-8);}
 if(list.empty())text("尚未持有宝物。战斗获得的宝物将在这里显示。",178,354,780,ink,2);
 for(int n=0;n<8&&treasure_page*8+n<int(list.size());++n){int index=treasure_page*8+n;const auto& item=list[index];const auto& e=item.at("item");auto r=treasure_row(n);bevel(r,index==treasure_selected);
  image("unit-details/images/Item/"+number3(e[0])+".bmp",{r.x+4,r.y+3,32,32});text(game->data().at("items").at(e[0].get<int>()).at("name"),r.x+48,r.y+9,r.x+284);
  if(e[1]!=255)text("Lv "+e[1].dump()+"  Exp "+(e[2]==255?"MAX":e[2].dump()),r.x+296,r.y+9,r.x+460);
  text(item.at("owner")==-1?"行囊":game->state().at("units").at(std::to_string(item.at("owner").get<int>())).at("name").get<std::string>()+" 装备中",r.x+470,r.y+9,r.x+630);
 }
 bevel({160,666,110,32});text("上一页",184,674,264);bevel({690,666,110,32});text("下一页",714,674,794);text(std::to_string(treasure_page+1)+" / "+std::to_string(pages),446,674,600);
 if(!list.empty()){int id=list[treasure_selected].at("item").at(0);text(detail_metadata.at("items").at(id).at("text"),164,718,794,ink,3);}
 text(game->state().at("phase")=="victory"?"战后整备：查看武将装备后可更换。":"战斗中仅供查看；胜利剧情结束后可更换装备。",164,794,552,ink,2);
 bevel(treasure_owner);text(game->state().at("phase")=="victory"?"查看武将 / 装备更换":"查看武将装备",treasure_owner.x+22,treasure_owner.y+10,794);
}
