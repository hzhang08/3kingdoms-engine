#include "caocao/jev_enemy.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace caocao;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void write(const std::filesystem::path& path,const std::string& value){std::ofstream out(path);out<<value;check(bool(out),"fixture write failed");}
Json await_reply(JevEnemy& client){auto until=SDL_GetTicks()+3000;Json reply;while(SDL_GetTicks()<until){reply=client.poll();if(!reply.is_null())return reply;SDL_Delay(5);}throw std::runtime_error("credential request stalled");}
}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("isolated output directory required");
 auto root=std::filesystem::path(argv[1])/("run-"+std::to_string(getpid()));std::filesystem::create_directories(root);
 auto key=root/"credential-fixture.txt";setenv("CAOCAO_JEV_CREDENTIAL_FILE",key.c_str(),1);
 const std::string sentinel="fake-key-credential-regression-only";
 // Oversized requests always stop before HTTP, even if credential loading
 // regresses. Their failure distinguishes in-memory auth from a file reread.
 Json request={{"key","credential-test"},{"state",{{"padding",std::string(97000,'x')}}},{"choices",{{"a0",{{"target",-1}}}}}};
 JevEnemy missing;check(!missing.configured()&&missing.credential_status()=="unavailable","missing credential accepted");
 write(key,"\""+sentinel+"\"\r\n ");
 missing.start(request,root/"missing-at-startup");auto reply=await_reply(missing);
 check(reply.at("error")=="startup credential unavailable","failed startup retried the credential file");
 JevEnemy cached;check(cached.configured()&&cached.credential_status()=="ready","quoted startup credential not loaded");
 std::filesystem::remove(key);
 check(cached.configured(),"configured() reread a deleted credential");
 cached.start(request,root/"deleted-after-startup");reply=await_reply(cached);
 check(reply.at("error")=="invalid request size/content","uncached decision reread a deleted credential");
 write(key,"invalid credential with spaces");
 cached.start(request,root/"rotated-after-startup");reply=await_reply(cached);
 check(reply.at("error")=="invalid request size/content","credential rotation changed the running session");
 JevEnemy restarted;check(!restarted.configured()&&restarted.credential_status()=="invalid","new session ignored invalid rotated key");
 write(key,"");JevEnemy empty;check(!empty.configured(),"empty credential accepted");
 write(key,std::string(4097,'k'));JevEnemy oversized;check(!oversized.configured(),"oversized credential accepted");
 write(key,sentinel);JevEnemy disabled(false);check(!disabled.configured()&&disabled.credential_status()=="disabled","offline mode loaded credential");
 disabled.start(request,root/"disabled");reply=await_reply(disabled);check(reply.at("error")=="startup credential disabled","offline request lazily loaded key");
 std::filesystem::remove(key);
 for(const auto& entry:std::filesystem::recursive_directory_iterator(root))if(entry.is_regular_file()){
  std::ifstream in(entry.path());std::string text((std::istreambuf_iterator<char>(in)),{});
  check(text.find(sentinel)==std::string::npos,"credential leaked into decision logs/cache");
 }
 write(root/"verification.json",Json{{"status","passed"},{"http_requests",0},{"startup_loaded",true},{"deleted_file_uncached_request",true},{"rotation_requires_restart",true},{"failed_load_not_retried",true},{"disabled_load_skipped",true},{"logs_exclude_credential",true}}.dump(2));
 std::cout<<"Startup credential cache, deleted/rotated file, failed load, offline mode and log redaction passed (0 HTTP requests)\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
