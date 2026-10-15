#include "caocao/jev_enemy.hpp"
#include "caocao/prototype.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
using namespace caocao;
namespace {
void check(bool condition,const std::string& reason){if(!condition)throw std::runtime_error(reason);}
void write(const std::filesystem::path& path,const Json& value){std::ofstream f(path);f<<value.dump(2)<<'\n';check(bool(f),"cannot write verification evidence");}
void dialogues(Prototype& game){for(int n=0;n<2000&&(game.has_dialogue()||!game.story_choices().empty());++n){if(!game.story_choices().empty())game.choose_story(0);else game.command("continue");}check(!game.has_dialogue()&&game.story_choices().empty(),"dialogue/choice did not reach interaction boundary");}
Json enemy_boundary(Prototype& game){
 // The revised engine has a real allied phase; requests must not be sent while
 // allied units or mandatory story choices still own the turn.
 dialogues(game);game.command("end_turn");
 for(int n=0;n<300;++n){
  dialogues(game);auto phase=game.state().at("phase").get<std::string>();
  check(phase!="victory"&&phase!="defeat","battle ended before an enemy decision");
  if(phase=="enemy"){auto request=game.enemy_request();if(!request.is_null())return request;}
  if(phase=="player")game.command("end_turn");else game.enemy_step();
  game.take_attack();game.take_movement();game.take_environment();
 }
 throw std::runtime_error("no enemy decision at a stable boundary");
}
Json await_reply(JevEnemy& client){Json reply=nullptr;auto deadline=SDL_GetTicks()+20000;while(reply.is_null()&&SDL_GetTicks()<deadline){reply=client.poll();SDL_Delay(5);}check(!reply.is_null(),"JEV request did not complete within its bounded timeout");return reply;}
Json summarize_transport(const std::filesystem::path& logs){
 Json records=Json::array();int responses=0,accepted=0,cached=0;
 if(std::filesystem::exists(logs))for(const auto& e:std::filesystem::directory_iterator(logs)){
  auto name=e.path().filename().string();if(!e.is_regular_file()||!name.starts_with("decision-")||e.path().extension()!=".json")continue;
  auto r=read_json(e.path());Json item={{"file",e.path().filename().string()},{"status",r.value("status",std::string())},{"http_status",r.value("http_status",std::string())},{"elapsed_ms",r.value("elapsed_ms",0)}};
  if(r.contains("http_status"))++responses;if(r.value("status",std::string())=="accepted")++accepted;if(r.value("status",std::string())=="cached")++cached;records.push_back(item);
 }
 return {{"client_starts",records.size()},{"http_responses",responses},{"accepted_decisions",accepted},{"cache_hits",cached},{"records",records}};
}
}
int main(int argc,char** argv){
 Json report={{"live_requested",false},{"live_attempt_limit",1},{"status","started"}};std::filesystem::path out,run;
 try{
  if(argc<3||argc>4||(argc==4&&std::string(argv[3])!="--live"))throw std::runtime_error("usage: jev_revised_tests ASSETS ISOLATED_OUTPUT [--live]; default is entirely offline, --live sends exactly one model request");
  bool live=argc==4;report["live_requested"]=live;out=argv[2];std::filesystem::create_directories(out);
  Prototype game(argv[1]);game.enable_revised_rules();game.start_scenario(3,314159);auto request=enemy_boundary(game);auto context=request.at("state");
  check(game.state().at("phase")=="enemy"&&!game.has_dialogue()&&game.story_choices().empty(),"decision preceded the stable enemy phase");
  check(context.at("protocol")=="caocao-revised-v5","outdated JEV protocol");
  const auto& caps=context.at("capabilities");check(caps.at("tactics")==Json::array({0,2,5,7,10,15,17,20,23,25,33,35,39,61,63}),"incorrect advertised spell subset");
  check(caps.at("counterattack")==true&&caps.at("critical_double")==true,"physical retaliation/extra-strike capabilities missing");
  check(caps.contains("status_recovery")&&caps.contains("tactic_estimates")&&context.contains("script_progress")&&context.contains("extended_tactics_manifest"),"revised rules context incomplete");
  check(context.at("script_progress")==Json::parse(game.serialize()).at("payload").at("progress"),"script progress differs from engine state");
  std::set<int> visible;for(const auto& u:context.at("units")){
   int id=u.at("id");check(visible.insert(id).second,"duplicate visible unit");const auto& original=game.state().at("units").at(std::to_string(id)).at("battle");
   check(original.at("lifecycle")==2&&original.at("hp").get<int>()>0,"hidden/dead unit leaked into decision");
   check(u.at("grades")==original.at("grades")&&u.at("status")==original.at("status"),"status/grade context differs");
  }
  int hidden=0;for(const auto& u:game.state().at("units")){const auto& b=u.at("battle");if(b.at("lifecycle")==1&&b.at("hp").get<int>()>0){++hidden;check(!visible.contains(b.at("id").get<int>()),"hidden ambush leaked into decision");}}
  check(hidden>0,"test scenario has no hidden reinforcement coverage");
  check(context.at("choices")==request.at("choices")&&!request.at("choices").empty(),"legal choices missing or inconsistent");
  int tactic_choices=0;for(auto it=request.at("choices").begin();it!=request.at("choices").end();++it){
   const auto& choice=it.value();check(JevEnemy::parse_choice(request,{{"answers",{{"action",{{"choice",it.key()}}}}}})==it.key(),"legal choice parser rejected action");
   if(choice.value("kind",std::string())=="tactic"){
    ++tactic_choices;int id=choice.at("tactic");check(std::find(caps.at("tactics").begin(),caps.at("tactics").end(),id)!=caps.at("tactics").end(),"unadvertised spell offered");
    check(choice.contains("targets")&&choice.contains("total_healing_on_hit")&&choice.contains("total_damage_on_hit")&&choice.contains("status_percent"),"incomplete spell effects sent to JEV");
   }
  }
  for(const auto& bad:{Json(nullptr),Json{{"answers",{{"action",{{"choice","invented_action"}}}}}},Json{{"answers",{{"action",{{"choice",0}}}}}}}){bool rejected=false;try{JevEnemy::parse_choice(request,bad);}catch(...){rejected=true;}check(rejected,"malformed/illegal JEV response accepted");}
  auto evaluation=JevEnemy::evaluation(request);check(evaluation.at("model")=="typesafe-ai/jev"&&evaluation.at("state")==context,"wrong model or omitted revised state");
  auto stable=game.serialize();check(game.enemy_request()==request&&game.serialize()==stable,"decision preview consumed state or RNG");
  write(out/"before.json",Json::parse(stable));write(out/"request.json",request);write(out/"evaluation.json",evaluation);
  report.update({{"protocol",context.at("protocol")},{"scenario",3},{"actor",context.at("actor")},{"visible_units",visible.size()},{"hidden_units_omitted",hidden},{"legal_choices",request.at("choices").size()},{"tactic_choices",tactic_choices},{"request_bytes",evaluation.dump().size()}});
  Json reply={{"key",request.at("key")},{"choice","a0"},{"source","offline_validation"}};
  if(live){
   JevEnemy client;check(client.configured(),"external credential file unavailable; no live request sent");
   // Use a fresh run directory, leaving prior evidence/caches intact and
   // ensuring this one allowed network decision cannot be an old cache hit.
   auto stamp=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();run=out/("live-"+std::to_string(stamp));
   client.start(request,run);reply=await_reply(client);report["reply"]=reply;report["transport"]=summarize_transport(run);
   check(reply.at("source")=="JEV"&&!reply.value("cancelled",false),"one live JEV attempt was not accepted; see redacted transport record");
   auto cachefile=run/"cache"/(request.at("key").get<std::string>()+".json");check(std::filesystem::exists(cachefile),"accepted decision was not durably cached; stopping without another network attempt");
   auto cache=read_json(cachefile);check(cache.at("sha256")==sha256(cache.at("payload").dump())&&cache.at("payload").at("key")==request.at("key")&&cache.at("payload").at("choice")==reply.at("choice"),"accepted cache is invalid; stopping without another network attempt");
   client.start(request,run);auto cached=await_reply(client);report["cache_reply"]=cached;report["transport"]=summarize_transport(run);
   check(cached.at("source")=="cache"&&cached.at("choice")==reply.at("choice"),"repeated state rerolled the accepted decision");
   check(report.at("transport").at("http_responses")==1&&report.at("transport").at("accepted_decisions")==1&&report.at("transport").at("cache_hits")==1,"unexpected live-call/cache counts");
  }else report["transport"]={{"client_starts",0},{"http_responses",0},{"accepted_decisions",0},{"cache_hits",0}};
  game.choose_enemy(reply.at("key"),reply.at("choice"));auto pending=game.serialize();Prototype replay=game;replay.restore(pending);game.enemy_step();replay.enemy_step();
  check(game.hash()==replay.hash(),"revised JEV decision changed after save/resume");
  report.update({{"status","passed"},{"reply",reply},{"choice",request.at("choices").at(reply.at("choice").get<std::string>())},{"result_hash",game.hash()},{"restored_result_hash",replay.hash()},{"saved_decision_replayed",true}});
  write(out/(live?"verification-live.json":"verification-offline.json"),report);std::cout<<report.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){report["status"]="failed";report["error"]=e.what();if(!run.empty())report["transport"]=summarize_transport(run);if(!out.empty()){try{write(out/(report.value("live_requested",false)?"verification-live.json":"verification-offline.json"),report);}catch(...){}}
  std::cerr<<report.dump(2)<<'\n';return 1;}
}
