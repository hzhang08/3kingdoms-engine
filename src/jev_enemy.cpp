#include "caocao/jev_enemy.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <thread>
namespace caocao {
namespace {
std::string read(const std::filesystem::path& p,size_t limit){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("file unavailable");std::string s;char b[4096];while(f){f.read(b,sizeof(b));s.append(b,size_t(f.gcount()));if(s.size()>limit)throw std::runtime_error("file too large");}return s;}
void write(const std::filesystem::path& p,const Json& j){auto tmp=p;tmp+=".pending";{std::ofstream f(tmp,std::ios::trunc);f<<j.dump(2);f.flush();if(!f)throw std::runtime_error("log write failed");}std::filesystem::permissions(tmp,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write);std::filesystem::rename(tmp,p);}
std::string curl_quote(const std::string& s){std::string out="\"";for(char c:s){if(c=='\\'||c=='\"')out+='\\';if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else out+=c;}return out+'"';}
void clear_secret(std::string& s){volatile char* p=s.data();for(size_t n=0;n<s.size();++n)p[n]=0;s.clear();}
Json run(Json request,std::filesystem::path logs,std::shared_ptr<std::atomic<bool>> cancelled,const std::string& secret,const std::string& credential_status){
 auto start=std::chrono::steady_clock::now();auto key=request.at("key").get<std::string>();Json result={{"key",key},{"choice","a0"},{"source","fallback"}};
 Json record={{"protocol",1},{"key",key},{"request",JevEnemy::evaluation(request)},{"status","started"}};std::filesystem::path logfile;SDL_Process* process=nullptr;
 try{
  std::filesystem::create_directories(logs/"cache");
  auto stamp=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  logfile=logs/("decision-"+std::to_string(stamp)+".json");record["started_unix_ns"]=stamp;
  // Durable request BEFORE HTTP: an interrupted attempt remains visible.
  write(logfile,record);
  auto cache=logs/"cache"/(key+".json");
  if(std::filesystem::exists(cache))try{
   auto e=Json::parse(read(cache,4096));auto p=e.at("payload");
   if(e.at("sha256")==sha256(p.dump())&&p.at("key")==key&&request.at("choices").contains(p.at("choice").get<std::string>())){result.update({{"choice",p.at("choice")},{"source","cache"}});record["status"]="cached";}
  }catch(...){record["cache_invalid"]=true;}
  if(result.at("source")!="cache"){
   if(secret.empty())throw std::runtime_error("startup credential "+credential_status);
   auto body=record.at("request").dump();if(body.size()>96000||body.find(secret)!=std::string::npos)throw std::runtime_error("invalid request size/content");
   if(cancelled->load())throw std::runtime_error("cancelled");
   // No shell, curlrc, redirects, credential argv/env/temp-file, or insecure TLS.
   std::string config="url = \"https://ai-gateway.vercel.sh/v1/evaluate\"\nproto = \"=https\"\nrequest = \"POST\"\nheader = "+curl_quote("Authorization: Bearer "+secret)+"\nheader = \"Content-Type: application/json\"\ndata-binary = "+curl_quote(body)+"\nconnect-timeout = 5\nmax-time = 15\nmax-filesize = 131072\nsilent\nwrite-out = \"\\nHTTP:%{http_code}\"\n";
   const char* args[]={"/usr/bin/curl","--disable","--config","-",nullptr};
   auto props=SDL_CreateProperties();SDL_SetPointerProperty(props,SDL_PROP_PROCESS_CREATE_ARGS_POINTER,args);
   SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDIN_NUMBER,SDL_PROCESS_STDIO_APP);SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,SDL_PROCESS_STDIO_APP);SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDERR_NUMBER,SDL_PROCESS_STDIO_NULL);
   process=SDL_CreateProcessWithProperties(props);SDL_DestroyProperties(props);if(!process)throw std::runtime_error("curl unavailable");
   auto* input=SDL_GetProcessInput(process);auto* output=SDL_GetProcessOutput(process);size_t sent=0;std::string raw;int code=-1;bool complete=false;
   auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(17);
   while(!complete){
    if(cancelled->load())throw std::runtime_error("cancelled");
    if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("request timeout");
    if(input){sent+=SDL_WriteIO(input,config.data()+sent,config.size()-sent);if(sent==config.size()){SDL_CloseIO(input);input=nullptr;std::fill(config.begin(),config.end(),'\0');}}
    char b[4096];size_t got=SDL_ReadIO(output,b,sizeof(b));raw.append(b,got);if(raw.size()>131072)throw std::runtime_error("response too large");
    bool exited=SDL_WaitProcess(process,false,&code);complete=exited&&got==0;
    if(!complete)std::this_thread::sleep_for(std::chrono::milliseconds(5));
   }
   SDL_DestroyProcess(process);process=nullptr;
   if(code!=0)throw std::runtime_error("network/TLS/timeout failure");
   auto marker=raw.rfind("\nHTTP:");if(marker==std::string::npos)throw std::runtime_error("invalid HTTP response");
   record["http_status"]=raw.substr(marker+6);if(raw.substr(marker+6)!="200")throw std::runtime_error("HTTP request rejected");raw.resize(marker);
   for(size_t at=0;(at=raw.find(secret,at))!=std::string::npos;)raw.replace(at,secret.size(),"[REDACTED]");
   auto response=Json::parse(raw);record["response"]=response;auto choice=JevEnemy::parse_choice(request,response);
   result.update({{"choice",choice},{"source","JEV"}});record["status"]="accepted";
  }
  // Cache fallback as well, so restoring this exact state does not silently
  // reroll an earlier recorded decision when connectivity changes.
 }catch(const std::exception& e){
  // Never include provider bodies, curl errors, credentials, or filesystem paths.
  std::string why=e.what();if(why.find("[json.exception")!=std::string::npos)why="invalid model response";
  if(!secret.empty()&&why.find(secret)!=std::string::npos)why="redacted failure";
  result["error"]=why;record["status"]=cancelled->load()?"cancelled":"fallback";record["error"]=why;
 }
 if(process){SDL_KillProcess(process,true);SDL_WaitProcess(process,true,nullptr);SDL_DestroyProcess(process);}
 result["cancelled"]=cancelled->load();record["result"]=result;record["elapsed_ms"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
 try{
  if(!logfile.empty())write(logfile,record);
  if(!cancelled->load()&&!logfile.empty()){Json p={{"key",key},{"choice",result.at("choice")}};write(logs/"cache"/(key+".json"),{{"payload",p},{"sha256",sha256(p.dump())}});}
 }catch(...){result["log_error"]=true;}
 return result;
}
}
std::filesystem::path JevEnemy::credential_file(){if(const char* p=std::getenv("CAOCAO_JEV_CREDENTIAL_FILE"))return p;if(const char* h=std::getenv("HOME"))return std::filesystem::path(h)/"Documents/credentials/vercel-cert-jev.txt";return {};}
JevEnemy::JevEnemy(bool preload_credentials){
 if(!preload_credentials){credential_status_="disabled";return;}
 try{
  credential_=read(credential_file(),4096);
  while(!credential_.empty()&&(credential_.back()=='\n'||credential_.back()=='\r'||credential_.back()==' '))credential_.pop_back();
  if(credential_.size()>=2&&(credential_.front()=='"'||credential_.front()=='\'')&&credential_.front()==credential_.back()){credential_.erase(credential_.begin());credential_.pop_back();}
  if(credential_.empty()||std::any_of(credential_.begin(),credential_.end(),[](unsigned char c){return c<=32||c==127;})){
   credential_status_="invalid";clear_secret(credential_);return;
  }
  credential_status_="ready";
 }catch(...){clear_secret(credential_);credential_status_="unavailable";}
}
Json JevEnemy::evaluation(const Json& r){Json criteria=Json::object();for(auto it=r.at("choices").begin();it!=r.at("choices").end();++it)criteria[it.key()]="Execute listed legal action "+it.value().dump();return {{"model","typesafe-ai/jev"},{"state",r.at("state")},{"questions",{{"action",{{"type","choice"},{"instructions","Command the enemy army (side 2). Choose one listed legal action for actor to defeat Cao Cao (id 0), stop the player's stated objective, protect key commanders and concentrate damage. Consider HP, terrain, ranged attacks and exposure. When the terrain-recovery capability is present, use terrain_heal_next_phase to weigh retreat or holding a recovery tile; it heals only at your next phase start, not immediately. Enemy units cannot collect the player location items. Target -1 means move and wait. Choices with kind=tactic cast the named spell for mp_cost after moving; damage_on_hit is an estimate conditional on a successful hit, hit_percent is its chance. Poison uses separate status and damage rolls. Choices without kind=tactic use physical attacks, or wait when target is -1; use the counterattack and critical_double capabilities to assess retaliation. Healing and support tactics affect friendly units; value total_healing_on_hit, status_percent and targets when supplied. Use spells at range when beneficial; conserve MP when a physical attack suffices. Respect the supplied capabilities: no unlisted spells, counterattacks or items. Coordinate with nearby allies. a0 is the deterministic local fallback. Never invent actions."},{"criteria",criteria}}}}}};}
std::string JevEnemy::parse_choice(const Json& r,const Json& response){auto choice=response.at("answers").at("action").at("choice").get<std::string>();if(!r.at("choices").contains(choice))throw std::runtime_error("model selected illegal action");return choice;}
JevEnemy::~JevEnemy(){cancel();if(job_.valid())job_.wait();clear_secret(credential_);}
void JevEnemy::cancel(){if(cancelled_)cancelled_->store(true);}
void JevEnemy::start(const Json& r,const std::filesystem::path& logs){if(busy())throw std::runtime_error("enemy request already pending");cancelled_=std::make_shared<std::atomic<bool>>(false);job_=std::async(std::launch::async,run,r,logs,cancelled_,std::cref(credential_),std::cref(credential_status_));}
Json JevEnemy::poll(){if(!job_.valid()||job_.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return nullptr;return job_.get();}
}
