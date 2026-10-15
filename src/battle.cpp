#include "caocao/battle.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
int battle_environment_class(int region,int selector){
 static constexpr int table[5][6]={{0,0,0,1,2,3},{0,0,0,0,1,2},{0,1,2,2,3,3},{0,0,1,1,4,4},{0,1,4,4,4,4}};
 if(region<0||region>=5||selector<0||selector>=6)throw std::runtime_error("invalid battle environment state");
 return table[region][selector];
}
int advance_battle_environment(int region,int selector,int override_selector,OriginalRng& rng){
 battle_environment_class(region,selector);
 if(override_selector<0||override_selector>255)throw std::runtime_error("invalid battle environment override");
 int roll=rng.bounded(8); // Consumed even when a forced selector is supplied.
 if(override_selector<6)return override_selector;
 int candidate=selector+1;
 if(candidate>roll)--candidate;else if(candidate<roll)++candidate;
 return candidate==0?5:candidate==7?0:candidate-1;
}

Json scripted_battle_highlight(const std::vector<BattleUnit>& units,int id){
 Json steps=Json::array();if(id<0||id>=512)return steps;
 const BattleUnit* found=nullptr;
 for(const auto& b:units)if(b.id==id&&b.lifecycle==2&&(!found||b.slot<found->slot))found=&b;
 if(!found)return steps;
 steps.push_back({{"kind","camera_request"},{"x",found->x},{"y",found->y}});
 steps.push_back({{"kind","selection_panel"},{"slot",found->slot},{"args",{0,255,255}}});
 for(int n=0;n<3;++n)for(int slot:{found->slot,255}){
  steps.push_back({{"kind","selection_target"},{"args",{slot}}});
  steps.push_back({{"kind","unit_redraw"},{"unit",id}});
  steps.push_back({{"kind","wait"},{"ticks",10}});
 }
 steps.push_back({{"kind","selection_clear_panel"},{"args",Json::array()}});return steps;
}
Json battle_start_banner(const std::string& text,std::array<int,2> viewport,std::array<int,4> client){
 if(viewport[0]<=0||viewport[1]<=0||viewport[0]%48||viewport[1]%48||viewport[0]/48>255||viewport[1]/48>255)throw std::runtime_error("invalid battle-start viewport");
 Json steps=Json::array();int cols=viewport[0]/48,rows=viewport[1]/48,size=viewport[0]>=720?64:48;
 auto paint=[&](int x,int y,int w,int h,int color){steps.push_back({{"kind","banner_cells"},{"args",{x,y,w,h}},{"color",color}});};
 for(int x=0;x<cols/2;++x){paint(x,0,1,rows,31);paint(cols-x-1,0,1,rows,31);}
 if(cols%2)paint(cols/2,0,1,rows,31);
 for(auto [offset,color]:{std::pair{2,24},std::pair{1,21},std::pair{0,18}}){auto r=client;for(auto& edge:r)edge+=offset;
  steps.push_back({{"kind","banner_text"},{"rect",r},{"width",size/2},{"height",size},{"text",text},{"color",color},{"center",1},{"opaque",0},{"length",0}});
 }
 steps.push_back({{"kind","wait"},{"ticks",20}});
 for(int y=0;y<rows;++y)paint(0,y,cols,1,255);paint(0,0,cols,rows,255);
 return steps;
}
Json battle_objective_banner(const std::string& text,int limit,const Json& labels,std::array<int,2> viewport,std::array<int,4> client){
 if(limit<0||limit>255||viewport[0]<=0||viewport[1]<=0||viewport[0]%48||viewport[1]%48||viewport[0]/48>255||viewport[1]/48>255)throw std::runtime_error("invalid objective banner input");
 Json steps=Json::array();int cols=viewport[0]/48,rows=viewport[1]/48,size=viewport[0]>=960?64:viewport[0]>=720?48:32;
 auto sweep=[&](int color){for(int row=0;row<rows;++row)steps.push_back({{"kind","banner_region"},{"args",{0,row,cols,1}},{"color",color}});};
 auto title=[&](const std::string& line,int shadow2,int shadow1){
  for(auto [offset,color]:{std::pair{2,shadow2},std::pair{1,shadow1},std::pair{0,18}}){auto r=client;for(auto& edge:r)edge+=offset;
   steps.push_back({{"kind","banner_text"},{"rect",r},{"width",size/2},{"height",size},{"text",line},{"color",color},{"center",0},{"opaque",0},{"length",0}});
  }
  steps.push_back({{"kind","wait"},{"ticks",20}});
 };
 sweep(31);client[1]+=72;client[3]=client[1]+size;title(labels.at("title").at("text"),42,40);
 // Original lines have two leading spaces and at most 28 further GBK bytes.
 // The opening text does not split a double-byte character. Reject that
 // unverified case instead of silently changing the executable's wrapping.
 for(size_t pos=0;pos<text.size();){std::string line="  ";int bytes=2;
  while(pos<text.size()&&bytes<30){unsigned char c=text[pos];if(c=='\n'){++pos;break;}
   size_t length=c<128?1:c<224?2:c<240?3:4;int gbk=c<128?1:2;
   if(pos+length>text.size()||bytes+gbk>30)throw std::runtime_error("unverified GBK split in objective banner");
   line+=text.substr(pos,length);pos+=length;bytes+=gbk;
  }
  client[1]+=size+16;client[3]+=size+16;title(line,24,21);
 }
 client[1]+=size+64;client[3]+=size+64;std::string formatted=labels.at("limit").at("text");auto at=formatted.find("%3u");
 if(at==std::string::npos)throw std::runtime_error("unsupported original objective limit label");
 auto digits=std::to_string(limit);formatted.replace(at,3,std::string(3-digits.size(),' ')+digits);title(formatted,31,24);
 for(int n=0;n<20;++n)steps.push_back({{"kind","input_wait_tick"},{"ticks",1}});
 sweep(255);steps.push_back({{"kind","banner_region"},{"args",{0,0,cols,rows}},{"color",255}});return steps;
}
Json battle_turn_banner(int turn,int limit,bool enemy,bool preserve_flags,const Json& labels,std::array<int,2> viewport,std::array<int,4> client){
 if(turn<0||turn>256||limit<0||limit>255||viewport[0]<=0||viewport[1]<=0||viewport[0]%48||viewport[1]%48)throw std::runtime_error("invalid turn banner input");
 Json steps=Json::array();int cols=viewport[0]/48,rows=viewport[1]/48,size=cols*48>=720?64:48;
 auto sweep=[&](int color){for(int n=0;n<cols;++n)steps.push_back({{"kind","banner_column"},{"args",{enemy?cols-1-n:n,0,1,rows}},{"color",color}});};
 auto title=[&](const std::string& text,std::array<int,4> rect,int shadow2,int shadow1){
  for(auto [offset,color]:{std::pair{2,shadow2},std::pair{1,shadow1},std::pair{0,18}}){
   auto r=rect;for(auto& edge:r)edge+=offset;
   steps.push_back({{"kind","banner_text"},{"rect",r},{"width",size/2},{"height",size},{"text",text},{"color",color},{"center",1},{"opaque",0},{"length",0}});
  }
 };
 sweep(enemy?63:45);title(labels.at(enemy?"enemy":"player").at("text"),client,enemy?58:42,enemy?53:36);
 if(!enemy){
  steps.push_back({{"kind","wait"},{"ticks",10}});client[1]+=size*2+16;
  int number=turn&255;std::string text=labels.at("final").at("text");
  if(number<limit){text=labels.at("turn").at("text");auto at=text.find("%2u");if(at==std::string::npos)throw std::runtime_error("unsupported original turn label");text.replace(at,3,(number<10?" ":"")+std::to_string(number));}
  title(text,client,31,24);
 }
 steps.push_back({{"kind","wait"},{"ticks",20}});
 if(!preserve_flags)steps.push_back({{"kind","reset_action_flags"}});
 steps.push_back({{"kind","minimap"}});sweep(255);return steps;
}
Json scripted_battle_appearance(const BattleUnit& original,const DeploymentSlot& d,int job,bool by_slot,const Json& terrain,const std::vector<int>& occupancy,const Json& data){
 Json steps=Json::array();if(original.lifecycle!=1)return steps;
 if(!by_slot&&((original.id>=162&&original.id<=169)||job==49||job==50))throw std::runtime_error("unverified apparition animation");
 auto b=original;auto position=battle_placement(d.x,d.y,job<=38?job/3:job-26,20,20,terrain,occupancy,data);
 if(position[0]==255)throw std::runtime_error("original reinforcement placement found no free tile");
 b.lifecycle=2;b.flags=(b.flags&~8)|(d.flags&8);b.x=position[0];b.y=position[1];
 Json tile={{"kind","tile"},{"battle",b.json()},{"visible",true}};
 steps.push_back(tile);
 if(!by_slot)steps.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
 steps.push_back({{"kind","sound"},{"id",15},{"flag",1}});
 // The slot form focuses the requested descriptor tile even after fallback.
 steps.push_back({{"kind","camera_request"},{"x",by_slot?d.x:b.x},{"y",by_slot?d.y:b.y}});
 steps.push_back(tile);steps.push_back({{"kind","wait"},{"ticks",4}});return steps;
}
std::array<int,2> battle_placement(int x,int y,int group,int width,int height,const Json& terrain,const std::vector<int>& occupancy,const Json& data){
 if(group<0||group>=27||width<=0||height<=0||terrain.size()!=static_cast<size_t>(width*height)||occupancy.size()!=terrain.size())throw std::runtime_error("invalid placement grid");
 const std::array<std::array<int,2>,4> cardinal{{{0,-1},{1,0},{0,1},{-1,0}}};
 auto neighbor=[&](std::array<int,2> p,int direction){p[0]=(p[0]+cardinal[direction][0])&255;p[1]=(p[1]+cardinal[direction][1])&255;if(p[0]>=width||p[1]>=height)p[0]=255;return p;};
 auto available=[&](std::array<int,2> p){return p[0]<width&&p[1]<height&&occupancy.at(p[1]*width+p[0])==255&&data.at("terrain_raw").at(group*60+30+terrain.at(p[1]*width+p[0]).get<int>())!=255;};
 std::array<int,2> current{x&255,y&255};
 // Four cumulative cardinal steps, checking the center then its neighbors.
 for(int direction=0;direction<4;++direction){
  if(current[0]!=255){if(available(current))return current;for(int n=0;n<4;++n){auto next=neighbor(current,n);if(next[0]!=255&&available(next))return next;}}
  current=neighbor(current,direction);
 }
 int distance=65535;std::array<int,2> result{255,255};
 for(int row=0;row<height;++row)for(int col=0;col<width;++col)if(available({col,row})){
  int next=std::abs((x&255)-col)+std::abs((y&255)-row);if(next<distance){distance=next;result={col,row};}
 }
 return result;
}
Json BattleUnit::json()const{return {{"id",id},{"slot",slot},{"side",side},{"x",x},{"y",y},{"target_slot",target_slot},{"target_x",target_x},{"target_y",target_y},{"lifecycle",lifecycle},{"flags",flags},{"ai",ai},{"direction",direction},{"hp",hp},{"mp",mp},{"status",status},{"grades",grades}};}
BattleUnit BattleUnit::from_json(const Json& j){BattleUnit b;
 #define FIELD(x) j.at(#x).get_to(b.x)
 FIELD(id);FIELD(slot);FIELD(side);FIELD(x);FIELD(y);FIELD(target_slot);FIELD(target_x);FIELD(target_y);FIELD(lifecycle);FIELD(flags);FIELD(ai);FIELD(direction);FIELD(hp);FIELD(mp);FIELD(status);FIELD(grades);
 #undef FIELD
 return b;
}
void BattleUnit::validate(const UnitRecord& u,const Json& data,int width,int height)const{
 if(id!=u.id||slot<0||slot>=115||side<0||side>3||x<0||x>=width||y<0||y>=height||target_slot<0||(target_slot>=115&&target_slot!=255)||target_x<0||target_x>255||target_y<0||target_y>255||lifecycle<1||lifecycle>3||flags<0||flags>255||ai<0||(ai>4&&ai!=7)||direction<0||direction>3||status<0||status>255||hp<0||hp>std::min(500,u.hp+equipment_bonus(u,5,data))||mp<0||mp>std::min(250,u.mp+equipment_bonus(u,6,data)))throw std::runtime_error("invalid battlefield unit");
 for(int g:grades)if(g<0||g>6)throw std::runtime_error("invalid battlefield status grade");
}
BattleUnit initialize_battle_unit(UnitRecord& u,const DeploymentSlot& d,const Json& data){
 if(u.id!=d.id)throw std::runtime_error("deployment character mismatch");
 BattleUnit b;b.id=d.id;b.slot=d.slot;b.side=(d.flags&3)==0?0:(d.flags&3)==2?1:(d.flags&3)==1?2:3;
 b.direction=d.flags&32?0:d.flags&8?1:d.flags&64?2:3;
 if((d.flags&4)==0){b.lifecycle=2;b.x=d.x;b.y=d.y;if(b.direction==1)b.flags|=8;}
 if(u.owner==0)u.record_counters[0]=std::min(255,u.record_counters[0]+1);
 else{
  u.record_counters={0,0,0};initialize_npc(u,d.level,d.promotion_mode,data);
  auto defaults=u;default_equipment(defaults,false,data.at("classes"));
  for(int slot=0;slot<3;++slot){int encoded=d.equipment_overrides[slot];
   if(encoded==0){if(slot<2)u.equipment[slot]=defaults.equipment[slot];}
   else if(encoded==1)u.equipment[slot]={255,255,255};
   else equip_unit(u,slot,encoded,slot==0?d.weapon_level:slot==1?d.armor_level:255,data);
  }
  static constexpr int ai[]{3,1,2,0,0,4,4};
  if(d.ai<0||d.ai>6)throw std::runtime_error("invalid AI deployment mode");
  // Opening descriptors do not use target-unit or target-tile AI. These need
  // original target-resolution fixtures before enabling further scenarios.
  if(d.ai>=3)throw std::runtime_error("targeted AI initialization is not verified");
  b.ai=ai[d.ai];
 }
 b.hp=std::min(500,u.hp+equipment_bonus(u,5,data));b.mp=std::min(250,u.mp+equipment_bonus(u,6,data));return b;
}
void prepare_new_battle_unit(BattleUnit& b){
 b.flags&=~128;b.status&=~30;
 if(b.slot<35&&b.lifecycle!=0&&b.side==0)b.ai=7;
}
std::array<int,2> new_battle_focus(const std::vector<BattleUnit>& units){
 const BattleUnit* first=nullptr;int rank=1000;
 for(const auto& b:units)if(b.lifecycle==2){int priority=b.slot<15?b.slot:b.slot>=35?b.slot-20:b.slot+100;
  if(priority<rank){first=&b;rank=priority;}}
 return first?std::array<int,2>{first->x,first->y}:std::array<int,2>{0,0};
}
void scripted_hp_loss(BattleUnit& b,int amount){if(b.lifecycle==2)b.hp=std::max(1,b.hp-std::min(b.hp,amount&255));}
void scripted_unit_ai(BattleUnit& b,const UnitRecord& r,int mode){
 if(b.id!=r.id)throw std::invalid_argument("AI unit record mismatch");
 mode&=255;if(mode>6||b.lifecycle==0||r.owner==0)return;
 if(mode>2)throw std::runtime_error("targeted script AI assignment is not verified");
 b.flags&=~128;b.ai=std::array<int,3>{3,1,2}[mode];
}
int battle_frame(const BattleUnit& b,int maximum_hp,int tick){
 const bool acted=(b.flags&6)!=0;int facing=b.direction==0?1:b.direction==2?0:2;
 int frame=b.hp<maximum_hp/5||(b.flags&32)?9:acted?facing+6:facing*2;
 if(!acted)frame+=(b.status&2)?((tick&255)/4!=0):tick%2;
 return frame;
}
Json scripted_battle_animation(const BattleUnit& b,int sprite,int animation,
                              bool prewait,bool postwait,int camera_tile_x,int camera_tile_y){
 if(animation<0||animation>9)
  throw std::runtime_error("unverified scripted battle animation "+std::to_string(animation));
 if(b.direction<0||b.direction>3||sprite<0||b.slot<0||b.slot>=115)
  throw std::runtime_error("invalid scripted animation input");
 Json out=Json::array();
 if(b.id<0||b.id>=512||b.lifecycle!=2)return out;
 out.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});
 const int x=b.x-camera_tile_x,y=b.y-camera_tile_y;
 auto wait=[&](int ticks){out.push_back({{"kind","wait"},{"ticks",ticks}});};
 if(animation==0){out.push_back({{"kind","redraw"},{"rectangle",{std::max(x-1,0),std::max(y-1,0),3,3}}});return out;}
 if(prewait)wait(8);
 if(animation==8){
  for(int n=0;n<16;++n){
   if(n%8==0)out.push_back({{"kind","sound"},{"id",6},{"flag",1}});
   out.push_back({{"kind","frame"},{"x",x*48},{"y",y*48+56},{"width",48},{"height",48},
    {"slot",b.slot},{"archive","UNIT_MOV.E5"},{"sprite",sprite},{"frame",(n<8?4:n<12?2:0)+n%2},{"mirror",n<4}});
   wait(2);
  }
  if(postwait)wait(8);return out;
 }
 const bool attack=animation==6||animation==7;
 const bool attack_atlas=attack||animation==5;
 const int size=attack_atlas?64:48;
 const char* archive=attack_atlas?"UNIT_ATK.E5":animation<=3?"UNIT_SPC.E5":"UNIT_MOV.E5";
 const int first=attack_atlas?(b.direction==0?4:b.direction==2?0:8):animation==1?4:animation==2?std::array<int,4>{1,2,0,2}[b.direction]:animation==3?3:9;
 for(int repeat=0;repeat<(animation==7?2:1);++repeat){
  out.push_back({{"kind","sound"},{"id",attack?7:8},{"flag",1}});
  for(int n=0;n<(attack?4:animation==9?6:1);++n){
   out.push_back({{"kind","frame"},{"x",x*48-(attack_atlas?8:0)},
    {"y",y*48+56-(attack_atlas?8:0)},{"width",size},{"height",size},
    {"slot",b.slot},{"archive",archive},{"sprite",sprite},{"frame",first+(animation==9?n%2:n)},
    {"mirror",(attack_atlas||animation==2)&&b.direction==1}});
   if(attack||animation==9)wait(animation==9?8:animation==7?2:1);
  }
 }
 if(animation==7)out.push_back({{"kind","unit_redraw"},{"unit",b.id}});
 if(postwait)wait(8);
 return out;
}
int battle_direction(const BattleUnit& from,const BattleUnit& to){
 if(from.slot==to.slot)return from.direction;
 const int dx=to.x-from.x,dy=to.y-from.y;
 if(std::abs(dy)>std::abs(dx))return dy<0?0:2;
 if(dx!=0)return dx<0?3:1;
 return from.direction;
}
Json scripted_battle_facing(const BattleUnit& original,const BattleUnit* target,int direction,bool reverse,bool prewait,bool postwait){
 Json steps=Json::array();auto b=original;
 if(b.lifecycle!=2||(target&&target->lifecycle!=2))return steps;
 if(!target&&(direction<0||direction>3))throw std::runtime_error("unverified direction sentinel");
 int desired=target?battle_direction(b,*target):direction;
 // Original tests the facing BEFORE applying reverse; unchanged facing is a no-op.
 if(desired==b.direction)return steps;
 if(prewait){steps.push_back({{"kind","camera_request"},{"x",b.x},{"y",b.y}});steps.push_back({{"kind","wait"},{"ticks",8}});}
 steps.push_back({{"kind","sound"},{"id",6},{"flag",1}});
 if(target&&reverse)desired=(desired+2)%4;
 b.direction=desired;b.flags=(b.flags&~8)|(desired==1?8:0);
 auto drawing=b;if((b.flags&6)==6)drawing.flags&=~6;
 steps.push_back({{"kind","tile"},{"battle",drawing.json()},{"visible",true}});
 if(drawing.flags!=b.flags)steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
 if(postwait)steps.push_back({{"kind","wait"},{"ticks",8}});
 return steps;
}
Json defeated_battle_unit(const std::vector<BattleUnit>& units,const UnitRecord& record){
 Json steps=Json::array();auto it=std::find_if(units.begin(),units.end(),[&](const auto& b){return b.id==record.id;});
 if(it==units.end()||it->lifecycle!=2)return steps;
 if(record.owner==0||record.id<26)throw std::runtime_error("party retirement dialogue and record counters are not yet verified (0x441744)");
 if(it->hp!=0)throw std::runtime_error("defeated-unit handler requires zero HP");
 auto b=*it;
 steps.push_back({{"kind","camera_visible"},{"x",b.x},{"y",b.y}});
 steps.push_back({{"kind","sound"},{"id",16},{"flag",1}});
 auto tile=[&](bool visible){steps.push_back({{"kind","tile"},{"battle",b.json()},{"visible",visible}});};
 for(int n=0;n<2;++n){tile(false);steps.push_back({{"kind","wait"},{"ticks",3}});tile(true);tile(true);steps.push_back({{"kind","wait"},{"ticks",3}});}
 tile(false);b.lifecycle=3;b.status=0;
 steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
 steps.push_back({{"kind","minimap"}});
 b.flags&=~8;steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
 for(auto other:units){if(other.id==b.id)other=b;if(other.target_slot==b.slot&&(other.ai==0||other.ai==4)){
  other.target_slot=255;other.ai=1;steps.push_back({{"kind","unit_state"},{"battle",other.json()}});
 }}
 return steps;
}
Json scripted_battle_removal(const std::vector<BattleUnit>& units,int id){
 Json steps=Json::array();auto it=std::find_if(units.begin(),units.end(),[&](const auto& b){return b.id==id;});
 if(it==units.end()||it->lifecycle!=2)return steps;
 if(id==0||it->side==0)throw std::runtime_error("party removal and defeat callbacks are not verified");
 auto b=*it;b.flags|=128;
 steps.push_back({{"kind","camera_visible"},{"x",b.x},{"y",b.y}});
 steps.push_back({{"kind","sound"},{"id",16},{"flag",1}});
 auto tile=[&](bool visible){steps.push_back({{"kind","tile"},{"battle",b.json()},{"visible",visible}});};
 for(int n=0;n<2;++n){tile(false);steps.push_back({{"kind","wait"},{"ticks",3}});tile(true);tile(true);steps.push_back({{"kind","wait"},{"ticks",3}});}
 tile(false);b.lifecycle=3;b.status=0;b.flags&=~(128|8);
 steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
 for(auto other:units){if(other.id==b.id)other=b;if(other.target_slot==b.slot&&(other.ai==0||other.ai==4)){
  other.target_slot=255;other.ai=1;steps.push_back({{"kind","unit_state"},{"battle",other.json()}});
 }}
 return steps;
}
std::vector<uint8_t> mask_battle_sprite(std::vector<uint8_t> pixels,int offset,const std::vector<uint8_t>& mark){
 if(offset<0)return pixels;
 if(static_cast<size_t>(offset)+pixels.size()>mark.size())throw std::runtime_error("missing original terrain mask");
 for(size_t n=0;n<pixels.size();++n)if(mark[offset+n]!=0)pixels[n]=0;
 return pixels;
}
Json scripted_battle_disappearance(const std::vector<BattleUnit>& units,int id,int sprite,int terrain){
 Json steps=Json::array();auto it=std::find_if(units.begin(),units.end(),[&](const auto& b){return b.id==id&&b.lifecycle!=0;});
 if(it==units.end())return steps;
 auto b=*it;
 if(b.lifecycle==2){
  // 0x45137c masks the top rows in place before a same-size transparent blit.
  // Terrain13 has an additional animated overlay, not exercised by the guide.
  if(terrain==13)throw std::runtime_error("disappearance on animated terrain is not verified");
  int mask=terrain==9||terrain==10?0x27c0:terrain==2?0x30c0:terrain==1?0x39c0:terrain==7?0x42c0:-1;
  b.flags|=128;steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
  steps.push_back({{"kind","camera_visible"},{"x",b.x},{"y",b.y}});
  auto frame=[&](const char* archive,int frame,int height){steps.push_back({{"kind","frame"},{"archive",archive},{"sprite",sprite},{"frame",frame},{"mirror",false},{"width",48},{"height",height},{"mask_offset",mask},{"slot",b.slot}});};
  steps.push_back({{"kind","sound"},{"id",6},{"flag",1}});frame("UNIT_MOV.E5",6,48);
  steps.push_back({{"kind","wait"},{"ticks",4}});steps.push_back({{"kind","sound"},{"id",17},{"flag",1}});
  for(int height=48;height>=4;height-=4){frame("UNIT_SPC.E5",0,height);steps.push_back({{"kind","wait"},{"ticks",1}});}
  steps.push_back({{"kind","tile"},{"battle",b.json()},{"visible",false}});
  b.lifecycle=3;b.status=0;steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
  steps.push_back({{"kind","minimap"}});b.flags&=~8;steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
  for(auto other:units){if(other.id==id)other=b;if(other.target_slot==b.slot&&(other.ai==0||other.ai==4)){
   other.target_slot=255;other.ai=1;if(other.id==id)b=other;steps.push_back({{"kind","unit_state"},{"battle",other.json()}});
  }}
 }
 b.flags&=~128;steps.push_back({{"kind","unit_state"},{"battle",b.json()}});
 return steps;
}
std::vector<std::array<int,2>> battle_camera_path(int camera_x,int camera_y,int focus_x,int focus_y,int map_width,int map_height){
 if(map_width<13||map_height<8||focus_x<0||focus_x>=map_width||focus_y<0||focus_y>=map_height||camera_x<0||camera_y<0||camera_x>(map_width-13)*48||camera_y>(map_height-8)*48)
  throw std::runtime_error("unsupported battlefield camera bounds");
 const int target_x=std::clamp(focus_x-6,0,map_width-13),target_y=std::clamp(focus_y-4,0,map_height-8);
 int x=camera_x/48,y=camera_y/48;std::vector<std::array<int,2>> path;
 while(x!=target_x||y!=target_y){
  auto step=[](int current,int target){int amount=std::min(4,std::abs(current-target)/4+1);if(current<target)current+=amount;if(current>target)current-=amount;return current;};
  x=step(x,target_x);y=step(y,target_y);path.push_back({x*48,y*48});
 }
 return path;
}
std::string battle_sound_file(int id){
 // Only these mappings have current original sndPlaySound fixtures.
 if(id==201)return "audio/Se_e_01.wav";
 if(id==112||id==119||id==129||id==131)return "audio/Se_m_"+std::to_string(id-100)+".wav";
 if(id!=6&&id!=7&&id!=8&&id!=15&&id!=16&&id!=17&&id!=19&&id!=39&&(id<30||id>37))throw std::runtime_error("unverified sound ID "+std::to_string(id));
 return "audio/Se"+std::string(id<10?"0":"")+std::to_string(id)+".wav";
}
std::vector<std::array<int,2>> battle_attack_camera_path(int camera_x,int camera_y,
 std::array<int,2> actor,std::array<int,2> target,int map_width,int map_height){
 if(map_width<13||map_height<8||actor[0]<0||actor[0]>=map_width||actor[1]<0||actor[1]>=map_height||target[0]<0||target[0]>=map_width||target[1]<0||target[1]>=map_height)
  throw std::runtime_error("invalid attack camera coordinates");
 int x=std::clamp(actor[0]-6,0,map_width-13),y=std::clamp(actor[1]-4,0,map_height-8);
 x=std::clamp(x,target[0]-12,target[0]);y=std::clamp(y,target[1]-7,target[1]);
 return battle_camera_path(camera_x,camera_y,x+6,y+4,map_width,map_height);
}
}
