#include "caocao/prototype.hpp"
#include "caocao/range.hpp"
#include <fstream>
namespace caocao {
ConsumableEffect consumable_effect(int item,int strength,int max_hp,int hp,int status){
 if(item!=87&&item!=88&&item!=92)throw std::runtime_error("unsupported consumable");
 return {item==92?hp:std::min(max_hp,hp+strength),item==92?status&~16:status};
}
void Prototype::load_items(const std::filesystem::path& assets){
 auto folder=assets/"items";if(!std::filesystem::exists(folder/"manifest.json"))return;
 auto m=read_json(folder/"manifest.json");items_manifest_=m.at("hash");m.erase("hash");
 if(sha256(m.dump())!=items_manifest_)throw std::runtime_error("item manifest checksum mismatch");
 for(const auto& e:m.at("assets")){
  auto path=std::filesystem::path(e.at("path").get<std::string>());if(path.is_absolute()||path.string().find("..")!=std::string::npos)throw std::runtime_error("invalid item asset path");
  std::ifstream f(folder/path,std::ios::binary);std::string raw{std::istreambuf_iterator<char>(f),{}};
  if(sha256(raw)!=e.at("sha256").get<std::string>())throw std::runtime_error("item asset checksum mismatch");
 }
}
void Prototype::enable_items(){
 if(!balanced_||legacy_)return;
 if(items_enabled_){new_items_=true;return;}
 if(items_manifest_.empty())throw std::runtime_error("missing verified item assets");
 enable_terrain_benefits();items_enabled_=new_items_=true;consumed_items_={{"87",0},{"88",0},{"92",0}};
 clear_enemy_choice();request_cache_key_.clear();request_cache_=nullptr;
}
Json Prototype::item_options(int actor,int item)const{
 Json out=Json::array();if(!items_enabled_||actor<0||(item!=87&&item!=88&&item!=92)||!state_.at("units").contains(std::to_string(actor))||inventory().at(std::to_string(item)).get<int>()<=0)return out;
 const auto& source=state_.at("units").at(std::to_string(actor));auto a=BattleUnit::from_json(source.at("battle"));
 if(a.hp<=0||a.lifecycle!=2||a.slot>=35||(a.flags&2)||(revised_rules_&&(a.side!=0||(a.status&8))))return out;
 auto range=battle_range(a,{a.x,a.y},1,1,false,255,map_width(),map_height(),{std::vector<int>(cell_count(),255),std::vector<int>(cell_count(),255),std::vector<int>(cell_count())},roster());
 for(int slot:range.targets)for(const auto& u:state_.at("units")){auto b=BattleUnit::from_json(u.at("battle"));if(b.slot!=slot)continue;
  auto effect=consumable_effect(item,data_.at("items").at(item).at("strength"),u.at("max_hp"),b.hp,b.status);
  int hp=effect.hp-b.hp;
  // Original manual predicate accepts full HP / absent poison. The UI warns
  // about a zero-effect use; it still spends one item and this actor's action.
  out.push_back({{"target",b.id},{"hp",hp},{"clear_poison",item==92&&bool(b.status&16)}});break;
 }
 return out;
}
void Prototype::use_item(int item,int target){
 if(state_.at("phase")!="player"||has_dialogue())throw std::runtime_error("现在不能使用道具。");
 int actor=state_.at("selected");auto options=item_options(actor,item);Json choice=nullptr;
 for(const auto& o:options)if(o.at("target")==target){choice=o;break;}
 if(choice.is_null())throw std::runtime_error("道具不足、部队已行动，或目标不在范围内。");
 // Validate the entire command before changing inventory, presentation or RNG.
 attack_=nullptr;movement_=nullptr;environment_=nullptr;
 auto& u=state_["units"][std::to_string(target)];auto b=BattleUnit::from_json(u.at("battle"));
 int amount=choice.at("hp");bool antidote=item==92;auto item_name=data_.at("items").at(item).at("name").get<std::string>();
 auto result=antidote?(choice.at("clear_poison").get<bool>()?"中毒已解除":"未中毒，仍消耗道具"):"兵力 +"+std::to_string(amount);
 auto caption=state_.at("units").at(std::to_string(actor)).at("name").get<std::string>()+"使用"+item_name+"："+u.at("name").get<std::string>()+" "+result;
 environment_event(actor,{{"kind","notice"},{"actor",actor},{"target",target},{"item",item},{"text",caption}});
 environment_event(actor,{{"kind","sound"},{"id",41}});
 if(!antidote)environment_event(actor,{{"kind","number"},{"slot",b.slot},{"value",amount},{"healing",true}});
 environment_event(actor,{{"kind","wait"},{"ticks",25}});
 b.hp+=amount;if(antidote)b.status&=~16;u["battle"]=b.json();
 environment_event(actor,{{"kind","unit_state"},{"actor",target},{"battle",b.json()}});
 environment_event(actor,{{"kind","notice_clear"}});
 auto key=std::to_string(item);benefits_["inventory"][key]=benefits_.at("inventory").at(key).get<int>()-1;consumed_items_[key]=consumed_items_.at(key).get<int>()+1;
 state_["message"]=caption;finish(actor);
}
}
