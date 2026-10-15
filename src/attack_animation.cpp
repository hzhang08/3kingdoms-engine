#include "caocao/attack_animation.hpp"
#include <algorithm>
#include <stdexcept>
namespace caocao {
std::vector<uint8_t> transform_combat_pixels(std::vector<uint8_t> pixels,const Json& effects,
 const std::vector<uint8_t>& quantized,const std::vector<uint8_t>& lookup){
 if(quantized.size()!=768||lookup.size()!=4096)throw std::invalid_argument("invalid combat palette");
 for(const auto& effect:effects){
  int kind=effect.at(0);if((kind!=0&&kind!=1&&kind!=2)||effect.size()!=(kind==1?4u:2u))throw std::invalid_argument("invalid combat palette effect");
  for(auto& pixel:pixels){
   if(pixel==0)continue;
   int rgb[3];
   for(int c=0;c<3;++c){
    int value=quantized[pixel*3+c];
    if(kind==0)value=std::min(15,value+(effect.at(1).get<int>()&255));
    else if(kind==2)value=std::max(0,value-(effect.at(1).get<int>()&255));
    else{
     int amount=effect.at(c==0?3:c).get<int>()&255;
     value=amount>7?std::min(15,value+((amount*2-14)&255)):std::max(0,value-(amount==0?15:(7-amount)*2));
    }
    rgb[c]=value;
   }
   pixel=lookup[(rgb[0]<<8)+(rgb[1]<<4)+rgb[2]];
  }
 }
 return pixels;
}
AttackAnimation physical_attack_animation(const BattleUnit& actor,const UnitRecord& au,
 const BattleUnit& target,const UnitRecord& bu,uint32_t hp_damage,uint32_t mp_damage,
 bool critical,int camera_tile_x,int camera_tile_y,int tick,const Json& data){
 if(actor.id!=au.id||target.id!=bu.id||actor.slot==target.slot||actor.direction<0||actor.direction>3||target.direction<0||target.direction>3||tick<0||tick>7)
  throw std::invalid_argument("invalid physical animation input");
 AttackAnimation out{actor,target,tick,Json::array()};
 const int ax=(actor.x-camera_tile_x)*48,ay=(actor.y-camera_tile_y)*48+56;
 const int bx=(target.x-camera_tile_x)*48,by=(target.y-camera_tile_y)*48+56;
 const int as=battle_sprite(au.id,au.job,actor.side),bs=battle_sprite(bu.id,bu.job,target.side);
 const int attack_first=actor.direction==0?4:actor.direction==2?0:8;
 const int reaction_direction=battle_direction(target,actor);
 const bool hit=hp_damage||mp_damage;
 const int reaction_frame=hit?3:reaction_direction==0?1:reaction_direction==2?0:2;
 const bool reaction_mirror=!hit&&reaction_direction==1;
 const int displacement=hit?(critical?8:4):8;
 const int dx=std::array<int,4>{0,-1,0,1}[reaction_direction]*displacement;
 const int dy=std::array<int,4>{1,0,-1,0}[reaction_direction]*displacement;
 const int group=au.job<=38?au.job/3:au.job-26;
 const bool bow=group==2||group==4;
 const bool mounted=group==0||group==1||group==7||group==9||group==10||group==14||group==15||group==18;
 auto wait=[&](int ticks){out.events.push_back({{"kind","wait"},{"ticks",ticks}});};
 auto sound=[&](int id){out.events.push_back({{"kind","sound"},{"id",id},{"flag",1}});};
 auto overlay=[&](bool attacker){out.events.push_back({{"kind","overlay"},{"x",attacker?ax:bx},{"y",attacker?ay:by},{"slot",attacker?actor.slot:target.slot}});};
 auto frame=[&](bool attacker,const char* archive,int index,int size,bool mirror,int x,int y,const Json& effects=Json::array()){
  out.events.push_back({{"kind","frame"},{"x",x},{"y",y},{"width",size},{"height",size},
   {"slot",attacker?actor.slot:target.slot},{"archive",archive},{"sprite",attacker?as:bs},{"frame",index},{"mirror",mirror},{"effects",effects}});
 };
 auto attack_frame=[&](int index,const Json& effects=Json::array()){frame(true,"UNIT_ATK.E5",attack_first+index,64,actor.direction==1,ax-8,ay-8,effects);};
 auto reaction=[&](bool displaced,const Json& effects=Json::array()){frame(false,"UNIT_SPC.E5",reaction_frame,48,reaction_mirror,bx+(displaced?dx:0),by+(displaced?dy:0),effects);};
 for(int n=0;n<27;++n){
  if(n==0){
   out.tick=(out.tick+1)%8;out.events.push_back({{"kind","attack_tick"},{"tick",out.tick}});overlay(false);
   int facing=actor.direction==0?1:actor.direction==2?0:2;
   frame(true,"UNIT_MOV.E5",6+facing,48,facing==2&&bool(actor.flags&8),ax,ay);
  }else if(n==2){overlay(false);attack_frame(0);sound(mounted?34:bow?37:32);}
  else if(n==8){
   out.tick=(out.tick+1)%8;out.events.push_back({{"kind","attack_tick"},{"tick",out.tick}});overlay(false);int index=bow?1:0;
   attack_frame(index);
   if(critical){
    sound(33);Json effects=Json::array();
    for(int k=0;k<6;++k){wait(1);effects.push_back({0,k/2+1});attack_frame(index,effects);}
   }
   if(bow)wait(8);
  }else if(n==12){overlay(false);attack_frame(1);}
  else if(n==13||n==14){
   if(target.hp==0){overlay(false);if(n==13)sound(7);}
   else if(!hit){
    reaction(true);out.target.direction=reaction_direction;
    out.target.flags=(out.target.flags&~8)|(reaction_direction==1?8:0);
    if(n==13)sound(critical?31:30);
   }else{
    Json effects=Json::array({Json::array({0,12})});
    if(n==14&&critical)effects.push_back({1,15,15,4});
    reaction(true,effects);if(n==13)sound(critical?36:35);
   }
   attack_frame(n==13?2:3);
  }else if(n==15||n==17){
   if(target.hp==0)overlay(false);else reaction(false);
   attack_frame(3);
   if(n==17&&target.hp!=0&&(hp_damage||mp_damage))
    out.events.push_back({{"kind","number"},{"slot",target.slot},{"value",hp_damage?hp_damage:mp_damage},{"foreground",hp_damage?18:69},{"background",24}});
  }else if(n==25){
   if(std::max(int64_t(0),int64_t(target.hp)-hp_damage)<std::min(500,bu.hp+equipment_bonus(bu,5,data))/5)out.target.flags|=32;
   overlay(false);overlay(true);
  }
  wait(1);
 }
 return out;
}
}
