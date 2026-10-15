#include "caocao/tutorial.hpp"
#include "caocao/movement.hpp"
#include "caocao/range.hpp"
#include "caocao/selection.hpp"
#include <stdexcept>
namespace caocao {
std::pair<int,bool> tutorial_collection_status(const std::vector<int>& flags,const Json& data){
 if(flags.size()!=108)throw std::runtime_error("invalid collection size");
 for(int b:flags)if(b<0||b>255)throw std::runtime_error("invalid collection byte");
 int count=0;for(int n=0;n<3;++n)count+=flags[n]!=0;bool all=true;
 for(const auto& item:data.at("items"))if(item.at("collection")!=0&&!flags.at(item.at("id").get<int>()+3)){all=false;break;}
 return {count,all};
}
void tutorial_show_range(Json& ui,const GameState& state,int id,int width,int height,const Json& data,bool blue){
 const auto& unit=state.units.at(std::to_string(id));auto b=BattleUnit::from_json(unit.at("battle"));auto u=UnitRecord::from_json(unit);
 std::vector<BattleUnit> roster;for(const auto& row:state.units)if(row.contains("battle"))roster.push_back(BattleUnit::from_json(row.at("battle")));
 auto search=battle_movement_search(b,u,{b.x,b.y},battle_movement_points(b,u,data),{255,255},MovementSearchMode::range,true,width,height,state.terrain,roster,data);
 if(search.costs.empty())throw std::runtime_error("tutorial movement range was not computed");
 ui["distance"]=search.costs;ui["movement"]=!blue;ui["blue_movement"]=blue;ui["range"]=selection_range({b.x,b.y},battle_attack_range(u,data),false,width,height);
}
Json tutorial_menu_presentation(const Json& cursor,int pc,const Json& ui,int focus){
 if(focus!=2&&focus!=0x508&&focus!=0x50b)throw std::runtime_error("invalid tutorial menu focus");
 const auto& resource=ui.at("command_menu");Json controls=resource.at("controls");
 for(auto& control:controls){int id=control.at("id");control["enabled"]=id==2||id==0x508||id==0x50b;control["result"]=id==2?255:id==0x507?1:id==0x508?2:id==0x509?3:4;}
 return {{"type","tutorial_command_menu"},{"focused_control",focus},{"tutorial_pc",pc},{"cursor",cursor},{"resource_id",resource.at("resource_id")},{"dialog",resource.at("dialog")},{"controls",controls}};
}
Json tutorial_program(int completions,bool all_treasures){
 if(completions<0||completions>3)throw std::runtime_error("invalid campaign completion count");
 Json code=Json::array();
 auto emit=[&](const char* op,int address,Json fields=Json::object()){fields["op"]=op;fields["address"]=address;code.push_back(fields);};
 auto say=[&](int text,int unit=157){emit("speech",0x45965d,{{"text",text},{"unit",unit}});};
 auto face=[&](int direction){emit("face",0x457428,{{"args",{157,-1,direction,0,1,1}}});};
 auto pose=[&](int kind){emit("pose",0x45778f,{{"args",{157,kind,1,1}}});};
 auto mark=[&](bool on){emit("mark",on?0x433726:0x43378d,{{"value",on?3:255}});};
 auto cursor=[&](int dx,int dy){emit("cursor",0x43353c,{{"delta",{dx,dy}}});};
 auto choice=[&](int text){int index=static_cast<int>(code.size());emit("choice",0x42de12,{{"text",text},{"accept",index+1},{"decline",-1}});return index;};
 auto jump=[&](){int index=static_cast<int>(code.size());emit("jump",0x433359,{{"target",-1}});return index;};
 say(0x48c9c4);emit("face",0x457428,{{"args",{0,157,0,0,1,1}}});say(0x48c9dc,0);
 if(completions==0){say(0x48ca10);say(0x48ca48);}else{say(0x48ca98);say(0x48cac4);say(0x48cafc);}
 int first=choice(0x48cb50);say(0x48cb70,0);
 say(0x48cc98);say(0x48ccb4);cursor(1,0);mark(true);face(1);say(0x48ccc8);face(2);say(0x48cd08);say(0x48cd40);
 for(int n=0;n<6;++n){mark(false);cursor(0,1);mark(true);emit("wait",0x403340,{{"ticks",4}});}
 emit("panel",0x44049e,{{"unit",0}});say(0x48cd58);pose(1);say(0x48cd88);pose(2);say(0x48cdb8);pose(0);say(0x48cdec);
 emit("clear_panel",0x453e32);mark(false);say(0x48ce24);say(0x48c994);
 int next=choice(0x48c9ac);
 say(0x48ce3c);mark(true);emit("move",0x459487,{{"unit",157},{"delta",{1,0}}});face(3);say(0x48ce58);
 emit("wait",0x403340,{{"ticks",4}});mark(false);cursor(-1,0);mark(true);emit("panel",0x44049e,{{"unit",32}});say(0x48ce7c);
 face(2);say(0x48cea8);say(0x48cedc);pose(2);say(0x48cf24);pose(0);say(0x48cf58);pose(5);say(0x48cf78);face(3);say(0x48cfc0);
 emit("sound",0x47467a,{{"id",0}});emit("show_range",0x438308,{{"unit",32}});emit("clear_panel",0x453e32);emit("redraw",0x453b13);
 face(2);say(0x48cff0);pose(6);say(0x48d030);pose(0);say(0x48d050);say(0x48d09c);
 emit("sound",0x47467a,{{"id",2}});emit("clear_range",0x4350f0);emit("redraw",0x453b13);pose(1);say(0x48d0d0);pose(2);say(0x48d10c);mark(false);pose(0);say(0x48d158);
 say(0x48c994);int actions=choice(0x48c9ac);
 emit("move",0x459487,{{"unit",157},{"delta",{-1,0}}});face(2);say(0x48d17c);
 mark(true);emit("wait",0x403340,{{"ticks",4}});mark(false);cursor(1,0);mark(true);face(1);say(0x48d1cc);
 emit("sound",0x47467a,{{"id",0}});emit("show_range",0x438308,{{"unit",0},{"blue",true}});emit("clear_panel",0x453e32);emit("redraw",0x453b13);
 say(0x48d1f8);face(2);say(0x48d210);pose(1);say(0x48d258);pose(0);say(0x48d298);
 for(int n=0;n<6;++n){mark(false);cursor(0,-1);mark(true);emit("wait",0x403340,{{"ticks",4}});}
 say(0x48d2c0);emit("clear_range",0x451c03);emit("redraw",0x453b13);
 emit("remember_position",0x433e96,{{"unit",0}});emit("move",0x459487,{{"unit",0},{"to_cursor",true}});face(1);say(0x48d2e4);
 emit("sound",0x47467a,{{"id",1}});emit("cancel_move",0x4350c0,{{"unit",0}});emit("redraw",0x453b13);face(2);say(0x48d310);
 mark(false);emit("restore_cursor",0x433f70);say(0x48d34c);mark(true);say(0x48d374);
 emit("sound",0x47467a,{{"id",0}});emit("show_range",0x438308,{{"unit",0},{"blue",true}});emit("redraw",0x453b13);say(0x48d39c);pose(1);say(0x48d3d0);
 emit("sound",0x47467a,{{"id",0}});emit("clear_range",0x451c03);mark(false);emit("redraw",0x453b13);
 emit("menu",0x45e69a);mark(true);pose(0);say(0x48d3f8);pose(2);say(0x48d414);pose(1);say(0x48d448);mark(false);pose(0);say(0x48d48c);
 say(0x48c994);int terrain=choice(0x48c9ac);
 mark(true);pose(0);say(0x48d4a8);face(1);face(2);face(3);say(0x48d4c0);face(2);say(0x48d4dc);pose(1);say(0x48d524);pose(0);say(0x48d554);say(0x48d584);
 for(int n=0;n<2;++n){mark(false);cursor(0,-1);mark(true);emit("wait",0x403340,{{"ticks",4}});}
 mark(false);cursor(1,0);mark(true);face(1);say(0x48d5a0);emit("sound",0x47467a,{{"id",0}});mark(false);emit("terrain_panel",0x45576d);mark(true);face(2);say(0x48d5d0);
 pose(5);say(0x48d614);pose(0);say(0x48d64c);pose(2);say(0x48d67c);pose(0);say(0x48d6a8);pose(1);say(0x48d6dc);pose(0);say(0x48d720);
 emit("sound",0x47467a,{{"id",1}});emit("clear_panel",0x453e32);emit("redraw",0x453b13);
 emit("move",0x459487,{{"unit",157},{"delta",{1,0}}});face(2);say(0x48d748);face(3);say(0x48d774);
 for(int n=0;n<4;++n){mark(false);cursor(n<2?-1:0,n<2?0:1);mark(true);emit("wait",0x403340,{{"ticks",4}});}
 emit("panel",0x44049e,{{"unit",32}});face(2);say(0x48d794);pose(5);say(0x48d7c8);pose(0);say(0x48d7fc);emit("clear_panel",0x453e32);mark(false);say(0x48d83c);
 face(0);say(0x48d858);face(2);say(0x48d888);pose(2);say(0x48d8b4);pose(1);say(0x48d8e4);pose(0);say(0x48d91c);
 code[next]["decline"]=code.size();code[actions]["decline"]=code.size();code[terrain]["decline"]=code.size();say(0x48cc24,0);say(0x48cc50);int completed=jump();
 code[first]["decline"]=code.size();say(0x48cb94,0);say(0x48cbe0);
 code[completed]["target"]=code.size();
 if(completions)emit("unsupported",0x4347ff,{{"reason","tutorial completion rewards are not yet verified"}});
 if(all_treasures)emit("unsupported",0x434d33,{{"reason","tutorial collection rewards are not yet verified"}});
 say(0x48c970);emit("end",0x4333d0);return code;
}
}
