#pragma once
#include "caocao/engine.hpp"
#include <atomic>
#include <future>
#include <memory>
namespace caocao {
// Native transport only; core/WASM never link HTTP or read credentials.
class JevEnemy {
 std::string credential_;
 std::string credential_status_="unavailable";
 std::future<Json> job_;
 std::shared_ptr<std::atomic<bool>> cancelled_;
public:
 // Construct at startup. Requests/toggles never reopen the credential file;
 // restart the app to retry a failed load or pick up a rotated key.
 explicit JevEnemy(bool preload_credentials=true);
 ~JevEnemy();
 static std::filesystem::path credential_file();
 bool configured()const{return !credential_.empty();}
 const std::string& credential_status()const{return credential_status_;}
 static Json evaluation(const Json& request);
 static std::string parse_choice(const Json& request,const Json& response);
 bool busy()const{return job_.valid();}
 void start(const Json& request,const std::filesystem::path& logs);
 void cancel();
 Json poll(); // null until finished; includes key, choice/failure, cache status
};
}
