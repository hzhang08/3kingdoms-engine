#include "caocao/diagnostics.hpp"
#include <algorithm>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <unistd.h>

namespace caocao {
namespace {
uint64_t epoch_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
}
void Diagnostics::atomic_file(const std::filesystem::path& path,const std::string& data){
 auto pending=path;pending+=".pending";
 {std::ofstream f(pending,std::ios::binary|std::ios::trunc);f<<data;f.flush();if(!f)throw std::runtime_error("diagnostic write failed");}
 std::filesystem::rename(pending,path);
}
void Diagnostics::prune(const std::filesystem::path& root,size_t keep){
 std::vector<std::pair<uint64_t,std::filesystem::path>> inactive;
 for(const auto& e:std::filesystem::directory_iterator(root)){
  if(!e.is_directory()||e.is_symlink()||!e.path().filename().string().starts_with("run-"))continue;
  try{
   auto m=read_json(e.path()/"session.json");if(m.value("format",std::string())!="caocao-diagnostics-v1")continue;
   int pid=m.at("pid");bool live=!m.value("closed",false)&&pid>0&&(::kill(pid,0)==0||errno==EPERM);
   if(!live)inactive.emplace_back(m.at("started_ms").get<uint64_t>(),e.path());
  }catch(...){/* Unknown directories are not ours to remove. */}
 }
 std::sort(inactive.rbegin(),inactive.rend());
 for(size_t i=keep;i<inactive.size();++i){std::error_code ec;std::filesystem::remove_all(inactive[i].second,ec);}
}
bool Diagnostics::open(const std::filesystem::path& root,size_t segment_bytes,size_t segments,size_t keep) noexcept{
 try{
  if(active_)close(0);
  if(segment_bytes<256||segments<1||segments>16)throw std::runtime_error("invalid diagnostic bounds");
  std::filesystem::create_directories(root);prune(root,keep);
  auto stamp=epoch_ms();bool created=false;
  for(int n=0;n<100&&!created;++n){directory_=root/("run-"+std::to_string(stamp)+"-"+std::to_string(::getpid())+"-"+std::to_string(n));created=std::filesystem::create_directory(directory_);}
  if(!created)throw std::runtime_error("diagnostic run collision");
  metadata_={{"format","caocao-diagnostics-v1"},{"started_ms",stamp},{"pid",::getpid()},{"closed",false}};
  atomic_file(directory_/"session.json",metadata_.dump(2));
  segment_bytes_=segment_bytes;segments_=segments;segment_=0;bytes_=0;sequence_=0;checkpoint_hash_.clear();
  stream_.clear();stream_.open(directory_/"events-0.jsonl",std::ios::trunc);active_=bool(stream_);return active_;
 }catch(...){active_=false;return false;}
}
void Diagnostics::rotate(){
 stream_.close();stream_.clear();segment_=(segment_+1)%segments_;bytes_=0;
 stream_.open(directory_/("events-"+std::to_string(segment_)+".jsonl"),std::ios::trunc);
 if(!stream_)throw std::runtime_error("diagnostic rotation failed");
}
void Diagnostics::write(Json event) noexcept{
 if(!active_)return;
 try{
  event["sequence"]=++sequence_;event["time_unix_ms"]=epoch_ms();auto line=event.dump()+"\n";
  auto kind=event.value("event",std::string());if(kind=="boot"||kind=="session"){metadata_[kind]=event;atomic_file(directory_/"session.json",metadata_.dump(2));}
  if(line.size()>segment_bytes_)line=Json{{"event","oversize_event"},{"sequence",sequence_},{"time_unix_ms",epoch_ms()},{"bytes",line.size()}}.dump()+"\n";
  if(bytes_+line.size()>segment_bytes_)rotate();
  stream_<<line;stream_.flush();if(!stream_)throw std::runtime_error("diagnostic flush failed");bytes_+=line.size();
 }catch(...){active_=false;}
}
bool Diagnostics::checkpoint(const std::string& snapshot,const std::string& reason) noexcept{
 if(!active_)return false;
 try{
  if(snapshot.size()>512*1024)throw std::runtime_error("recovery snapshot too large");
  auto parsed=Json::parse(snapshot);if(parsed.at("sha256")!=sha256(parsed.at("payload").dump()))throw std::runtime_error("recovery checksum mismatch");
  auto digest=sha256(snapshot);if(digest==checkpoint_hash_)return true;
  auto latest=directory_/"recovery-latest.json",previous=directory_/"recovery-previous.json",pending=latest;pending+=".pending";
  // Prepare the replacement before changing either usable checkpoint.
  atomic_file(pending,snapshot);
  if(std::filesystem::exists(latest)){
   auto prev_pending=previous;prev_pending+=".pending";
   std::filesystem::copy_file(latest,prev_pending,std::filesystem::copy_options::overwrite_existing);std::filesystem::rename(prev_pending,previous);
  }
  std::filesystem::rename(pending,latest);checkpoint_hash_=digest;
  write({{"event","recovery_checkpoint"},{"reason",reason},{"file","recovery-latest.json"},{"snapshot_sha256",digest},{"state_sha256",parsed.at("sha256")},{"schema",parsed.at("payload").at("schema")}});return true;
 }catch(const std::exception& e){write({{"event","diagnostic_error"},{"operation","recovery_checkpoint"},{"message",e.what()}});return false;}
}
void Diagnostics::failure_snapshot(const std::string& snapshot) noexcept{
 if(!active_)return;
 try{if(snapshot.size()>512*1024)return;atomic_file(directory_/"failure-snapshot.json",snapshot);write({{"event","failure_snapshot"},{"file","failure-snapshot.json"},{"stable",false}});}catch(...){}
}
void Diagnostics::close(int exit_code) noexcept{
 if(!active_)return;
 try{write({{"event","session_end"},{"exit_code",exit_code}});metadata_["closed"]=true;metadata_["exit_code"]=exit_code;metadata_["ended_ms"]=epoch_ms();atomic_file(directory_/"session.json",metadata_.dump(2));stream_.close();}catch(...){}
 active_=false;
}
}
