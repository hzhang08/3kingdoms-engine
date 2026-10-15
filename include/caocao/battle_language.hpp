#pragma once
#include "caocao/engine.hpp"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace caocao {
// Presentation only. Original strings in commands, events and snapshots remain
// untouched; longest matching source phrases also cover composed status text.
class BattleLanguage {
 std::map<std::string,std::string> entries_;
 std::vector<std::pair<std::string,std::string>> phrases_;
 mutable std::map<std::string,std::string> cache_;
 bool english_=false;
public:
 void load(const Json& catalog){
  entries_=catalog.at("translations").get<decltype(entries_)>();
  phrases_.assign(entries_.begin(),entries_.end());
  std::stable_sort(phrases_.begin(),phrases_.end(),[](const auto& a,const auto& b){return a.first.size()>b.first.size();});
  cache_.clear();
 }
 void english(bool value){english_=value;}
 bool english()const{return english_;}
 std::string translate(const std::string& source)const{
  if(!english_)return source;
  if(auto i=cache_.find(source);i!=cache_.end())return i->second;
  if(auto i=entries_.find(source);i!=entries_.end())return i->second;
  std::string out;
  for(size_t p=0;p<source.size();){
   bool found=false;
   for(const auto& [from,to]:phrases_)if(source.compare(p,from.size(),from)==0){out+=(from=="使用"&&p>0)?" uses ":to;p+=from.size();found=true;break;}
   if(!found)out+=source[p++];
  }
  // Dynamic combat messages have many numeric variants. Bound the render cache.
  if(cache_.size()>2048)cache_.clear();
  cache_[source]=out;return out;
 }
};
}
