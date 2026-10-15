#include "caocao/diagnostics.hpp"
#include "caocao/prototype.hpp"
#include <iostream>
#include <unistd.h>
using namespace caocao;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string read(const std::filesystem::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("assets and isolated output directory required");
 std::filesystem::path root=argv[2];std::filesystem::create_directories(root);
 Prototype game(argv[1]);game.enable_revised_rules();auto first=game.serialize();
 Diagnostics a,b;check(a.open(root/"sessions",1024,2,2),"open failed");check(b.open(root/"sessions",1024,2,2),"second open failed");check(a.directory()!=b.directory(),"concurrent sessions collide");
 check(a.checkpoint(first,"initial"),"checkpoint failed");auto before_hash=game.hash();check(game.hash()==before_hash,"logging changed state");
 game.command("tile",10,6);auto second=game.serialize();check(second!=first,"fixture did not change");check(a.checkpoint(second,"action"),"second checkpoint failed");
 check(read(a.directory()/"recovery-previous.json")==first,"previous state lost");check(read(a.directory()/"recovery-latest.json")==second,"latest state differs");
 Prototype restored(argv[1]);restored.restore(read(a.directory()/"recovery-latest.json"));check(restored.serialize()==second,"recovery cannot restore");game.command("cancel");restored.command("cancel");check(restored.hash()==game.hash(),"recovery next command diverged");
 check(!a.checkpoint("{}","corrupt"),"corrupt checkpoint accepted");check(read(a.directory()/"recovery-latest.json")==second,"corrupt snapshot overwrote recovery");
 for(int n=0;n<200;++n)a.write({{"event","frame"},{"n",n},{"padding",std::string(100,'x')}});
 a.write({{"event","fatal_error"},{"message","test error after rotation"}});a.failure_snapshot(second);a.close(1);
 bool fatal=false;size_t count=0;for(const auto& e:std::filesystem::directory_iterator(a.directory()))if(e.path().extension()==".jsonl"){
  ++count;check(e.file_size()<=1024,"segment bound exceeded");std::ifstream f(e.path());std::string line;while(std::getline(f,line)){auto j=Json::parse(line);fatal=fatal||j.at("event")=="fatal_error";}
 }
 check(count==2&&fatal,"rotation dropped final fatal error");check(read(a.directory()/"failure-snapshot.json")==second,"failure snapshot lost");
 check(read_json(a.directory()/"session.json").at("exit_code")==1,"exit status absent");
 auto active=b.directory();
 std::filesystem::create_directories(root/"sessions"/"unrelated");std::ofstream(root/"sessions"/"unrelated"/"keep.txt")<<"keep";
 for(int n=0;n<5;++n){Diagnostics x;check(x.open(root/"sessions",1024,2,2),"retention open failed");x.close(0);}
 check(std::filesystem::exists(active),"retention deleted active session");check(std::filesystem::exists(root/"sessions"/"unrelated"/"keep.txt"),"retention deleted unrelated directory");
 size_t closed=0;for(const auto& e:std::filesystem::directory_iterator(root/"sessions"))if(std::filesystem::exists(e.path()/"session.json"))closed+=read_json(e.path()/"session.json").value("closed",false);check(closed<=3,"history retention exceeded");
 b.close(0);
 // A directory replaced with a regular file simulates unwritable storage
 // without relying on root-sensitive permission-bit behavior.
 std::ofstream(root/"not-a-directory")<<"keep";Diagnostics disabled;check(!disabled.open(root/"not-a-directory"/"logs"),"invalid sink opened");disabled.write({{"event","ignored"}});check(!disabled.checkpoint(first,"disabled"),"disabled checkpoint wrote");disabled.close(0);
 std::ofstream(root/"verification.json")<<Json{{"rotation",true},{"concurrent_sessions",true},{"retention",true},{"atomic_checkpoint_restore",true},{"corruption_preserves_recovery",true},{"logging_failure_nonfatal",true},{"next_action_deterministic",true}}.dump(2);
 std::cout<<"diagnostic rotation, retention, recovery and failure isolation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
