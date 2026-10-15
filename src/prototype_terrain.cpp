#include "caocao/prototype.hpp"
#include "caocao/recovery.hpp"
#include <fstream>
#include <set>
#include <utility>
namespace caocao {
bool location_pickup_eligible(const BattleUnit& b,int x,int y,bool claimed){
 return !claimed&&b.slot<35&&b.lifecycle==2&&b.hp>0&&b.x==x&&b.y==y;
}
void Prototype::load_terrain_benefits(const std::filesystem::path& assets){
 auto folder=assets/"terrain-benefits";if(!std::filesystem::exists(folder/"manifest.json"))return;
 auto m=read_json(folder/"manifest.json");terrain_manifest_=m.at("hash");m.erase("hash");
 if(sha256(m.dump())!=terrain_manifest_)throw std::runtime_error("terrain manifest checksum mismatch");
 for(const auto& e:m.at("assets")){
  auto path=std::filesystem::path(e.at("path").get<std::string>());
  if(path.is_absolute()||path.string().find("..")!=std::string::npos)throw std::runtime_error("invalid terrain asset path");
  std::ifstream f(folder/path,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
  if(sha256(raw)!=e.at("sha256").get<std::string>())throw std::runtime_error("terrain asset checksum mismatch");
  if(path=="catalog.json")terrain_catalog_=Json::parse(raw);
 }
}
void Prototype::enable_terrain_benefits(){
 if(!balanced_||legacy_)return;
 if(terrain_enabled_){new_terrain_=true;return;}
 if(terrain_catalog_.is_null())throw std::runtime_error("missing verified terrain assets");
 enable_tactics();terrain_enabled_=new_terrain_=true;
 // An old save never had this inventory. Upgrade without ticking a turn,
 // granting a pickup, changing a unit or consuming a random draw.
 benefits_={{"claimed",Json::array()},{"inventory",{{"87",0},{"88",0},{"92",0}}}};
 clear_enemy_choice();request_cache_key_.clear();request_cache_=nullptr;
}
Json Prototype::locations()const{
 Json out=Json::array();if(!terrain_enabled_)return out;
 for(auto site:terrain_catalog_.at("locations"))if(site.at("scenario")==scenario_){
  site["claimed"]=std::find(benefits_.at("claimed").begin(),benefits_.at("claimed").end(),site.at("event"))!=benefits_.at("claimed").end();out.push_back(std::move(site));
 }
 return out;
}
int Prototype::healing_at(int id,int x,int y)const{
 if(!terrain_enabled_)return 0;
 const auto& u=state_.at("units").at(std::to_string(id));
 auto b=BattleUnit::from_json(u.at("battle"));return b.lifecycle==2?terrain_recovery_hp(u.at("max_hp"),b.hp,terrain_.at("cells").at(y*map_width()+x)):0;
}
Json Prototype::take_environment(){return std::exchange(environment_,nullptr);}
void Prototype::environment_event(int actor,const Json& event){
 if(environment_.is_null())environment_={{"kind","environment"},{"actor",actor},{"before",state_},{"events",Json::array()}};
 environment_["events"].push_back(event);
}
void Prototype::terrain_phase(bool enemy){
 if(!terrain_enabled_)return;
 // Match the original maintenance roster order, independent of JSON id order.
 auto board=roster();std::sort(board.begin(),board.end(),[](const auto& a,const auto& b){return a.slot<b.slot;});
 for(const auto& b:board){
  if((b.side==2)!=enemy||b.lifecycle!=2||b.hp<=0)continue;
  int amount=healing_at(b.id,b.x,b.y);if(amount<=0)continue;
  auto& u=state_["units"][std::to_string(b.id)];
  environment_event(b.id,{{"kind","notice"},{"actor",b.id},{"text",u.at("name").get<std::string>()+" 地形恢复兵力 +"+std::to_string(amount)}});
  environment_event(b.id,{{"kind","number"},{"slot",b.slot},{"value",amount},{"healing",true}});
  environment_event(b.id,{{"kind","wait"},{"ticks",18}});
  u["battle"]["hp"]=b.hp+amount;
  environment_event(b.id,{{"kind","unit_state"},{"actor",b.id},{"battle",u.at("battle")}});
  environment_event(b.id,{{"kind","notice_clear"}});
 }
}
void Prototype::collect_locations(){
 if(!terrain_enabled_||!state_.at("undo").is_null())return;
 auto sites=locations();
 // Original section-1 selector chooses the last eligible event first.
 for(auto it=sites.rbegin();it!=sites.rend();++it){const auto& site=*it;if(site.at("claimed"))continue;
  int actor=-1;for(const auto& u:state_.at("units")){auto b=BattleUnit::from_json(u.at("battle"));
   if(location_pickup_eligible(b,site.at("x"),site.at("y"),false)){actor=b.id;break;}}
  if(actor<0)continue;
  int item=site.at("item");auto key=std::to_string(item);
  auto label="获得 "+data_.at("items").at(item).at("name").get<std::string>()+" ×1";
  environment_event(actor,{{"kind","notice"},{"actor",actor},{"text",label},{"item",item}});
  environment_event(actor,{{"kind","sound"},{"id",14}});
  environment_event(actor,{{"kind","wait"},{"ticks",25}});
  environment_event(actor,{{"kind","notice_clear"}});
  benefits_["claimed"].push_back(site.at("event"));benefits_["inventory"][key]=benefits_.at("inventory").at(key).get<int>()+site.at("quantity").get<int>();
 }
}
void Prototype::validate_benefits()const{
 if(!benefits_.is_object()||benefits_.size()!=2||!benefits_.at("claimed").is_array()||!benefits_.at("inventory").is_object()||benefits_.at("inventory").size()!=3)throw std::runtime_error("invalid terrain inventory");
 Json expected={{"87",0},{"88",0},{"92",0}};if(revised_rules_&&!campaign_.empty()&&!campaign_.at("battles").empty())expected=campaign_.at("battles").back().at("benefits").at("inventory");std::set<int> seen;
 if(hulao_duel_&&hulao_starting_beans_)expected["87"]=expected.at("87").get<int>()+3;
 if(items_enabled_&&(!consumed_items_.is_object()||consumed_items_.size()!=3))throw std::runtime_error("invalid consumed inventory");
 for(const auto& flag:benefits_.at("claimed")){
  if(!flag.is_number_integer()||flag<0||flag>INT32_MAX||!seen.insert(flag.get<int>()).second)throw std::runtime_error("invalid location claim");
  bool found=false;for(const auto& site:terrain_catalog_.at("locations"))if(site.at("scenario")==scenario_&&site.at("event")==flag){auto key=site.at("item").dump();expected[key]=expected.at(key).get<int>()+site.at("quantity").get<int>();found=true;break;}
  if(!found)throw std::runtime_error("unknown location claim");
 }
 for(auto it=expected.begin();it!=expected.end();++it){
  int spent=0;if(items_enabled_){if(!consumed_items_.contains(it.key())||!consumed_items_.at(it.key()).is_number_integer()||consumed_items_.at(it.key())<0||consumed_items_.at(it.key())>99)throw std::runtime_error("invalid consumed inventory");spent=consumed_items_.at(it.key());}
  if(!benefits_.at("inventory").contains(it.key())||!benefits_.at("inventory").at(it.key()).is_number_integer()||benefits_.at("inventory").at(it.key())<0||benefits_.at("inventory").at(it.key())>99||benefits_.at("inventory").at(it.key()).get<int>()+spent!=it.value())throw std::runtime_error("inventory and location claims disagree");
 }
}
}
