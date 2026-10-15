#include "caocao/tactic_animation.hpp"
#include <stdexcept>
namespace caocao {
Json tactic_caption_window(std::array<int,4> r){
 auto [x,y,w,h]=r;
 if(w<48||h<48||w%48||h%48)throw std::invalid_argument("invalid caption rectangle");
 Json events=Json::array();
 for(int yy=y;yy<y+h;yy+=48)for(int xx=x;xx<x+w;xx+=48)
  events.push_back({{"kind","caption_tile"},{"x",xx},{"y",yy},{"width",48},{"height",48},{"offset",0x18dc0}});
 auto line=[&](int x1,int y1,int x2,int y2,int color){events.push_back({{"kind","caption_line"},{"args",{x1,y1,x2,y2,65535}},{"color",color}});};
 for(int n=0;n<3;++n){line(x+n,y+h+n,x+w,y+h+n,131+n*2);line(x+w+n,y+n,x+w+n,y+h+2,131+n*2);}
 line(x,y-1,x+w,y-1,128);line(x-1,y-1,x-1,y+h,128);return events;
}
TacticCastAnimation tactic_cast_animation(const BattleUnit& a,const UnitRecord& u,
 const BattleUnit& b,int tactic,std::array<int,2> camera,int width,int height,
 int speed,int elapsed,const Json& tactics){
 if((tactic!=23&&tactic!=25&&tactic!=33)||a.id!=u.id||a.slot==b.slot||speed<0||speed>2||elapsed<0)
  throw std::invalid_argument("unverified tactic caster animation input");
 TacticCastAnimation out{a,camera};auto& ev=out.events;
 auto wait=[&](int ticks){ev.push_back({{"kind","wait"},{"ticks",ticks}});};
 auto sound=[&](int id){ev.push_back({{"kind","sound"},{"id",id},{"flag",1}});};
 wait(8);sound(8);
 for(auto p:battle_camera_path(camera[0],camera[1],b.x,b.y,width,height))out.camera=p;
 // 0x45503f still calls the scroll wrapper when the focus is unchanged.
 ev.push_back({{"kind","camera"},{"tile",{out.camera[0]/48,out.camera[1]/48}}});
 ev.push_back({{"kind","clear_cursor"},{"args",Json::array()}});
 out.actor.direction=battle_direction(a,b);out.actor.flags=(a.flags&~8)|(out.actor.direction==1?8:0);
 ev.push_back({{"kind","unit_redraw"},{"slot",a.slot}});
 int x=a.x*48-out.camera[0]-8,y=a.y*48-out.camera[1]+48;
 int frame=out.actor.direction==0?4:out.actor.direction==2?0:8;
 ev.push_back({{"kind","frame"},{"x",x},{"y",y},{"width",64},{"height",64},{"slot",a.slot},
  {"archive","UNIT_ATK.E5"},{"sprite",battle_sprite(a.id,u.job,a.side)},{"frame",frame},{"mirror",out.actor.direction==1}});
 sound(39);
 // All three admitted tactic names are two GBK characters. Decode from
 // the imported data, and fail if this edition's width contract changes.
 std::string name=tactics.at(tactic).at("name");Json glyphs=Json::array();int gx=296;
 for(size_t n=0;n<name.size();){auto c=static_cast<uint8_t>(name[n]);int bytes=c<128?1:c<224?2:c<240?3:4;
  if(n+bytes>name.size())throw std::runtime_error("invalid tactic name UTF-8");
  glyphs.push_back({gx,160,name.substr(n,bytes)});gx+=c<128?8:16;n+=bytes;
 }
 if(gx!=328)throw std::runtime_error("unverified tactic caption width");
 ev.push_back({{"kind","cursor"},{"args",{0}}});
 ev.push_back({{"kind","message_pump"},{"args",{0,514,0,0}}});
 ev.push_back({{"kind","caption_window"},{"args",{264,144,96,48,0}}});
 ev.push_back({{"kind","caption_text"},{"x",296},{"y",160},{"text",name},{"glyphs",glyphs},
  {"foreground",172},{"background",0},{"opaque",false}});
 ev.push_back({{"kind","caption_wait"},{"args",{(speed+1)*10}}});
 ev.push_back({{"kind","caption_close"},{"args",Json::array()}});
 ev.push_back({{"kind","message_pump"},{"args",{0,514,0,0}}});
 if(elapsed<20)wait(20-elapsed);
 ev.push_back({{"kind","caster_restore"},{"slot",a.slot},{"rectangle",{x,y,64,64}}});
 return out;
}
}
