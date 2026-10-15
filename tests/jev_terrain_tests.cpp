#include "caocao/jev_enemy.hpp"
#include "caocao/prototype.hpp"
#include "prototype_terrain_fixture.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("assets isolated-output required; makes ONE live JEV request");
 std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 Prototype game(argv[1]);game.enable_terrain_benefits();game.start_scenario(0);while(game.has_dialogue())game.command("continue");terrain_fixture_place(game,146,13,11,1);
 auto e=Json::parse(game.serialize());e["payload"]["state"]["units"]["146"]["battle"]["status"]=0;e["sha256"]=sha256(e.at("payload").dump());game.restore(e.dump());game.command("end_turn");auto request=game.enemy_request();
 if(request.is_null()||request.at("state").at("protocol")!="caocao-terrain-v3")throw std::runtime_error("missing terrain context");
 std::ofstream(out/"before.json")<<game.serialize();std::ofstream(out/"request.json")<<request.dump(2);
 JevEnemy client;client.start(request,out/"runtime");Json reply=nullptr;auto until=SDL_GetTicks()+20000;
 while(reply.is_null()&&SDL_GetTicks()<until){reply=client.poll();SDL_Delay(5);}
 if(reply.is_null()||reply.at("source")!="JEV")throw std::runtime_error("live JEV unavailable: "+reply.dump());
 game.choose_enemy(reply.at("key"),reply.at("choice"));auto saved=game.serialize();Prototype replay=game;replay.restore(saved);game.enemy_step();replay.enemy_step();
 if(game.hash()!=replay.hash())throw std::runtime_error("terrain JEV replay changed");
 Json result={{"reply",reply},{"choice",request.at("choices").at(reply.at("choice").get<std::string>())},{"result_hash",game.hash()},{"restored_result_hash",replay.hash()}};
 std::ofstream(out/"verification.json")<<result.dump(2);std::cout<<result.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
