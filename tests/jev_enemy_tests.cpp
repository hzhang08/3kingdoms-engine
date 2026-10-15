#include "caocao/jev_enemy.hpp"
#include "caocao/prototype.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <thread>
using namespace caocao;
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("assets isolated-log-directory required");Prototype g(argv[1]);g.start_scenario(2);while(g.has_dialogue())g.command("continue");g.command("end_turn");auto r=g.enemy_request();auto body=JevEnemy::evaluation(r);
 check(body.at("model")=="typesafe-ai/jev"&&body.at("state").at("units").size()>0,"state not submitted");
 check(JevEnemy::parse_choice(r,{{"answers",{{"action",{{"choice","a0"}}}}}})=="a0","valid choice rejected");
 for(const auto& j:{Json(nullptr),Json{{"answers",{{"action",{{"choice","run arbitrary command"}}}}}},Json{{"answers",{{"action",{{"choice",42}}}}}}}){bool bad=false;try{JevEnemy::parse_choice(r,j);}catch(...){bad=true;}check(bad,"malformed choice accepted");}
 // Transport injection is intentionally unavailable: fixed HTTPS endpoint only.
 // Missing credentials exercise asynchronous failures without sending a request.
 setenv("CAOCAO_JEV_CREDENTIAL_FILE","/nonexistent/caocao-test-key",1);
 JevEnemy client;client.start(r,argv[2]);auto until=SDL_GetTicks()+3000;Json reply=nullptr;while(reply.is_null()&&SDL_GetTicks()<until){reply=client.poll();SDL_Delay(5);}
 check(!reply.is_null()&&reply.at("source")=="fallback"&&reply.at("choice")=="a0","missing credential stalled the turn");g.choose_enemy(reply.at("key"),reply.at("choice"));auto snapshot=g.serialize();Prototype copy=g;copy.restore(snapshot);g.enemy_step();copy.enemy_step();check(g.hash()==copy.hash(),"fallback replay differs");
 client.start(r,argv[2]);reply=nullptr;until=SDL_GetTicks()+3000;while(reply.is_null()&&SDL_GetTicks()<until){reply=client.poll();SDL_Delay(5);}check(reply.at("source")=="cache","recorded fallback was rerolled");
 client.start(r,std::filesystem::path(argv[2])/"cancel");client.cancel();reply=nullptr;until=SDL_GetTicks()+3000;while(reply.is_null()&&SDL_GetTicks()<until){reply=client.poll();SDL_Delay(5);}check(reply.at("cancelled")==true,"cancelled request applied");
 std::cout<<"JEV protocol, invalid response, asynchronous failure, recorded fallback, cache and cancellation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
