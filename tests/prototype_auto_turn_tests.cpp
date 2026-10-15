#include "caocao/prototype.hpp"
#include <iostream>
using namespace caocao;
static void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
static void drain(Prototype& g){for(int n=0;n<1000;++n){if(g.has_dialogue())g.command("continue");else if(!g.story_choices().empty())g.choose_story(0);else return;}throw std::runtime_error("dialogue stalled");}
static void restore(Prototype& g,Json s){s["sha256"]=sha256(s.at("payload").dump());g.restore(s.dump());}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("usage: assets");Prototype g(argv[1]);g.enable_revised_rules();g.enable_equipment();drain(g);
 const auto initial=Json::parse(g.serialize());
 check(!g.player_turn_complete(),"fresh player still has an action");
 g.command("tile",10,6);g.command("tile",10,5);
 check(!g.player_turn_complete(),"tentative movement must keep attack/menu available");
 auto moved=g.serialize();g.restore(moved);check(!g.player_turn_complete(),"restored movement is still tentative");
 g.command("cancel");check(!g.player_turn_complete(),"cancel cannot finish a turn");
 g.command("tile",10,6);g.command("wait");drain(g);
 check(g.player_turn_complete(),"sole own unit finished; allies must not block");
 auto finished=g.serialize();const auto finished_hash=g.hash();
 for(int n=0;n<10;++n)check(g.player_turn_complete(),"query is stable");check(g.hash()==finished_hash,"query changed state or RNG");
 g.command("end_turn");const auto next=g.serialize(),hash=g.hash();check(g.state().at("phase")=="ally","existing phase command reused");check(!g.player_turn_complete(),"must not repeat during ally turn");
 g.restore(finished);check(g.player_turn_complete(),"finished save should advance after restoration");g.command("end_turn");check(next==g.serialize(),"restored turn transition differs");
 // Synthetic casualties/action flags exercise eligibility without changing roster identity.
 g.start_scenario(1);drain(g);auto s=Json::parse(g.serialize());int extra=-1;
 for(auto& u:s["payload"]["state"]["units"])if(u["battle"]["side"]==0){u["battle"]["flags"]=2;if(u["battle"]["id"]!=0)extra=u["battle"]["id"];}
 check(extra>=0,"scenario needs multiple own units");auto& b=s["payload"]["state"]["units"][std::to_string(extra)]["battle"];
 b["flags"]=0;restore(g,s);check(!g.player_turn_complete(),"second unfinished own unit blocks");
 b["flags"]=2;restore(g,s);check(g.player_turn_complete(),"all own actions complete");
 b["flags"]=0;b["hp"]=0;b["lifecycle"]=3;restore(g,s);check(g.player_turn_complete(),"dead unit must not block automatic turn");
 s=Json::parse(finished);s["payload"]["state"]["units"]["0"]["battle"]["flags"]=0;s["payload"]["state"]["units"]["0"]["battle"]["status"]=8;
 restore(g,s);check(g.player_turn_complete(),"confused unit cannot accept a command");
 s=Json::parse(finished);s["payload"]["state"]["phase"]="enemy";restore(g,s);check(!g.player_turn_complete(),"enemy phase cannot auto-end");
 s=Json::parse(finished);s["payload"]["state"]["phase"]="defeat";s["payload"]["state"]["units"]["0"]["battle"]["hp"]=0;s["payload"]["state"]["units"]["0"]["battle"]["lifecycle"]=3;restore(g,s);check(!g.player_turn_complete(),"defeat cannot auto-end");
 s=initial;for(auto& u:s["payload"]["state"]["units"])if(u["battle"]["side"]==2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}
 restore(g,s);g.command("tile",10,6);g.command("wait");check(!g.player_turn_complete(),"victory/dialogue cannot auto-end");drain(g);check(g.state().at("phase")=="victory"&&!g.player_turn_complete(),"completed victory cannot auto-end");
 std::cout<<"PASS: tentative movement, cancellation, multi-unit eligibility, incapacitated units, phase guards, pure query, deterministic save/restore.\ntransition_hash="<<hash<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
