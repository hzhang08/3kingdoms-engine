#include "caocao/prototype.hpp"
#include <algorithm>
#include <fstream>
namespace caocao {
void Prototype::load_expansion(const std::filesystem::path& assets){
 auto root=assets/"expansion";if(!std::filesystem::exists(root/"manifest.json"))return;
 auto m=read_json(root/"manifest.json");expansion_manifest_=m.at("hash");m.erase("hash");
 if(sha256(m.dump())!=expansion_manifest_||m.at("schema")!=1||m.at("original_manifest")!=manifest_)throw std::runtime_error("incompatible battle expansion");
 std::map<std::string,std::string> files;
 for(const auto& a:m.at("assets")){
  std::filesystem::path path=a.at("path").get<std::string>();
  if(path.is_absolute())throw std::runtime_error("invalid expansion path");
  for(const auto& part:path)if(part=="..")throw std::runtime_error("invalid expansion path");
  std::ifstream f(root/path,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
  if(!f||sha256(raw)!=a.at("sha256").get<std::string>())throw std::runtime_error("battle expansion checksum mismatch: "+path.string());files[path.string()]=std::move(raw);
 }
 scenarios_=Json::parse(files.at("scenarios.json"));int expected=1;
 for(const auto& c:scenarios_){
  int id=c.at("id");if(id!=expected++)throw std::runtime_error("invalid scenario id");
  auto t=Json::parse(files.at(c.at("terrain")));int w=t.at("width"),h=t.at("height");
  if(w<13||w>60||h<8||h>60||t.at("cells").size()!=size_t(w*h)||c.at("width")!=w||c.at("height")!=h)throw std::runtime_error("invalid scenario grid");
  scenario_terrains_[std::to_string(id)]=std::move(t);scenario_scripts_[std::to_string(id)]=Json::parse(files.at(c.at("script")));
 }
}
std::string Prototype::scenario_name(int id)const{return id?scenarios_.at(id-1).at("name").get<std::string>():"颍川之战";}
std::string Prototype::name()const{return scenario_name(scenario_);}
std::string Prototype::objective()const{
 if(hulao_duel_)return "击败"+data_.at("heroes").at(duel_enemy_).at("name").get<std::string>();
 if(revised_rules_){if(!scenario_)return "击毙张宝和张梁";if(scenario_==3&&progress_.contains("flags")&&progress_.at("flags").value("2",0))return "曹操到达西端（0,4至0,9），或击退李儒";}
 return scenario_?config().at("objective").get<std::string>():"消灭全部敌军";
}
std::string Prototype::battlefield_image()const{return scenario_?"expansion/"+config().at("image").get<std::string>():"images/battlefield.bmp";}
int Prototype::enemy_count()const{int n=0;for(const auto& u:state_.at("units"))if(u.at("battle").at("side")==2)++n;return n;}
bool Prototype::enemies_defeated(const Json& state)const{
 if(revised_rules_)return progress_.value("victory",false);
 if(scenario_&&config().at("victory_target")!=-1)return state.at("units").at(std::to_string(config().at("victory_target").get<int>())).at("battle").at("hp")==0;
 for(const auto& u:state.at("units")){
  auto b=BattleUnit::from_json(u.at("battle"));if(b.side!=2||b.hp<=0)continue;
  if(scenario_&&!config().at("victory_region").empty()){const auto& r=config().at("victory_region");if(b.x<r.at(0).get<int>()||b.y<r.at(1).get<int>()||b.x>r.at(2).get<int>()||b.y>r.at(3).get<int>())continue;}
  return false;
 }return true;
}
bool Prototype::scenario_dialogue_eligible(int id)const{
 const auto& e=dialogue_catalog_.at(std::to_string(id));std::string kind=e.at("trigger");
 if(kind=="intro")return true;
 if(kind=="victory"||kind=="defeat")return state_.at("phase")==kind;
 auto hp=[&](int u){return state_.at("units").at(std::to_string(u)).at("battle").at("hp").get<int>();};
 if(kind=="death")return hp(e.at("unit"))==0;
 auto a=BattleUnit::from_json(state_.at("units").at(std::to_string(e.at("units").at(0).get<int>())).at("battle"));
 auto b=BattleUnit::from_json(state_.at("units").at(std::to_string(e.at("units").at(1).get<int>())).at("battle"));
 return a.hp>0&&b.hp>0&&std::abs(a.x-b.x)<=1&&std::abs(a.y-b.y)<=1;
}
void Prototype::queue_scenario_dialogues(){
 if(revised_rules_){
  if(conversations_&&!has_dialogue()&&std::find(dialogue_.at("triggered").begin(),dialogue_.at("triggered").end(),1000)==dialogue_.at("triggered").end()){
   dialogue_["triggered"].push_back(1000);dialogue_["pending"].push_back(1000);dialogue_["page"]=0;return;
  }
  process_events();return;
 }
 if(!conversations_||has_dialogue())return;
 auto seen=[&](int id){const auto& ids=dialogue_.at("triggered");return std::find(ids.begin(),ids.end(),id)!=ids.end();};
 auto enqueue=[&](int id){if(dialogue_catalog_.contains(std::to_string(id))&&!seen(id)&&scenario_dialogue_eligible(id)){dialogue_["triggered"].push_back(id);dialogue_["pending"].push_back(id);}};
 enqueue(1000);if(has_dialogue())return;
 if(state_.at("phase")=="defeat"){enqueue(1002);return;}
 std::vector<int> ids;for(auto it=dialogue_catalog_.begin();it!=dialogue_catalog_.end();++it)if(std::stoi(it.key())>1002)ids.push_back(std::stoi(it.key()));
 std::sort(ids.rbegin(),ids.rend());for(int id:ids)enqueue(id);
 enqueue(1001);
}
}
