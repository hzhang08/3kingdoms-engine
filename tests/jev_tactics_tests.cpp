#include "caocao/jev_enemy.hpp"
#include "caocao/prototype.hpp"
#include "caocao/movement.hpp"
#include "prototype_tactics_fixture.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <iostream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("assets isolated-output required; this test makes TWO live JEV requests");std::filesystem::path out=argv[2];std::filesystem::create_directories(out);Json results=Json::array();
 for(int scenario:{1,2}){
  Prototype game(argv[1]);game.enable_tactics();game.start_scenario(scenario);while(game.has_dialogue())game.command("continue");prepare_caster_contact(game,scenario==1?135:132);
  auto request=game.enemy_request();std::ofstream(out/(std::to_string(scenario)+"-before.json"))<<game.serialize();std::ofstream(out/(std::to_string(scenario)+"-request.json"))<<request.dump(2);
  JevEnemy client;client.start(request,out/("battle-"+std::to_string(scenario)));Json reply=nullptr;auto until=SDL_GetTicks()+20000;
  while(reply.is_null()&&SDL_GetTicks()<until){reply=client.poll();SDL_Delay(5);}if(reply.is_null()||reply.at("source")!="JEV")throw std::runtime_error("live JEV did not return an accepted choice: "+reply.dump());
  game.choose_enemy(reply.at("key"),reply.at("choice"));auto save=game.serialize();Prototype replay=game;replay.restore(save);game.enemy_step();replay.enemy_step();if(game.hash()!=replay.hash())throw std::runtime_error("JEV spell replay changed outcome");
  auto packet=game.take_attack();results.push_back({{"scenario",scenario},{"reply",reply},{"action",request.at("choices").at(reply.at("choice").get<std::string>())},{"result_hash",game.hash()},{"tactic_executed",!packet.is_null()&&packet.value("kind",std::string())=="tactic"}});
 }
 std::ofstream(out/"verification.json")<<results.dump(2);std::cout<<results.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
