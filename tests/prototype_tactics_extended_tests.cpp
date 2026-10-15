#include "caocao/prototype.hpp"
#include "caocao/prototype_tactics_extended.hpp"
#include "caocao/prototype_playback.hpp"
#include <iostream>
#include <fstream>
using namespace caocao;
void check(bool x,const std::string& s){if(!x)throw std::runtime_error(s);}
void restore_edit(Prototype& g,Json p){p["sha256"]=sha256(p.at("payload").dump());g.restore(p.dump());}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("assets required");std::filesystem::path assets=argv[1],root=assets.parent_path().parent_path();auto data=read_json(assets/"data.json");
 auto oracle=read_json(root/"tests/fixtures/original_prototype_extended_tactics.json");int n=0;
 for(const auto& c:oracle.at("cases")){
  auto a=BattleUnit::from_json(c.at("actor")),b=BattleUnit::from_json(c.at("target"));auto ar=UnitRecord::from_json(c.at("actor_unit")),br=UnitRecord::from_json(c.at("target_unit"));int t=c.at("tactic");OriginalRng rng{c.at("seed")};
  auto prefix="original case "+std::to_string(n)+" tactic "+std::to_string(t)+": ";
  check(extended_tactic_usable(a,ar,b,br,t,data)==c.at("usable"),prefix+"target usable");
  check(extended_tactic_hit(a,ar,b,br,t,data)==c.at("chance"),prefix+"hit");
  check(extended_tactic_amount(a,ar,b,br,t,false,data,rng)==c.at("estimate"),prefix+"estimate");check(rng.state==c.at("seed"),prefix+"preview used RNG");
  bool status=(t==23||t==25||t==33||t==35)&&rng.percent(extended_tactic_hit(a,ar,b,br,t,data));
  check(status==c.at("status_hit"),prefix+"status roll");int amount=extended_tactic_amount(a,ar,b,br,t,true,data,rng);
  check(amount==c.at("amount")&&rng.state==c.at("rng"),prefix+"amount/RNG "+std::to_string(amount)+" vs "+c.at("amount").dump());
  auto gain=extended_tactic_experience(a,ar,b,br,t,amount,-1,data);check(gain.character==c.at("experience")&&gain.weapon==c.at("weapon_experience")&&gain.armor==c.at("armor_experience"),prefix+"XP "+Json::array({gain.character,gain.weapon,gain.armor}).dump());
  extended_tactic_status(b,br,t);check(b.grades==c.at("grades_after").get<std::array<int,6>>()&&b.status==c.at("status_after"),prefix+"grade/status");++n;
 }
 std::cout<<n<<" original extended-tactic fixtures passed\n";
 if(argc>2&&std::string(argv[2])=="--rules-only")return 0;
 Prototype game(assets);game.enable_revised_rules();game.start_scenario(1);while(game.has_dialogue())game.command("continue");
 // Synthetic legal state keeps edition records/equipment untouched, moving
 // units into contact and lowering HP/MP only. Used for command integration.
 auto p=Json::parse(game.serialize());auto& s=p["payload"]["state"];
 // The currently deployed Cao Cao knows 小补给; self is a legal target.
 bool knows=false;for(const auto& spell:game.spells(0))if(spell.at("id")==39)knows=true;check(knows,"Cao Cao has no 小补给");
 s["units"]["0"]["battle"]["hp"]=10;s["selected"]=0;s["undo"]=nullptr;restore_edit(game,p);
 auto before=game.serialize();auto before_rng=game.state().at("rng");game.spell_options(0,{game.state().at("units").at("0").at("battle").at("x"),game.state().at("units").at("0").at("battle").at("y")});check(game.state().at("rng")==before_rng,"options consumed RNG");
 Prototype replay=game;replay.restore(before);game.cast(39,0);replay.cast(39,0);check(game.hash()==replay.hash(),"healing save continuation mismatch");
 auto packet=game.take_attack();check(packet.at("healing").get<int>()>0&&game.state().at("units").at("0").at("battle").at("hp").get<int>()>10,"heal did not restore HP");
 check(packet.at("damage")==0&&packet.at("mp_cost")==6,"heal damaged target or incorrect MP");
 PrototypePlayback playback;playback.start(packet);int frames=0;while(playback.active()){playback.advance(40);check(++frames<3000,"heal playback stuck");}
 // Invalid enemy healing and no-MP cast leave serialized progress untouched.
 replay.restore(before);auto stable=replay.serialize();bool rejected=false;try{replay.cast(39,135);}catch(...){rejected=true;}check(rejected&&replay.serialize()==stable,"enemy heal was accepted/mutated state");
 auto low=Json::parse(before);low["payload"]["state"]["units"]["0"]["battle"]["mp"]=0;restore_edit(replay,low);stable=replay.serialize();rejected=false;try{replay.cast(39,0);}catch(...){rejected=true;}check(rejected&&replay.serialize()==stable,"no-MP heal mutated state");
 // An early Daoist has area debuffs. Find a legal multi-target choice using
 // the same options sent to JEV; the effect must be shared by all targets.
 Prototype area(assets);area.enable_revised_rules();area.start_scenario(0);while(area.has_dialogue())area.command("continue");
 bool area_found=false;int area_actor=-1;std::array<int,2> contact{};
 for(const auto& u:area.state().at("units")){
  int id=u.at("battle").at("id");auto b=BattleUnit::from_json(u.at("battle"));auto r=UnitRecord::from_json(u.at("record"));if(b.side!=2||b.lifecycle!=2)continue;
  for(int y=0;y<area.map_height()&&!area_found;++y)for(int x=0;x<area.map_width()&&!area_found;++x){
   bool occupied=false;for(const auto& other:area.state().at("units")){const auto& q=other.at("battle");if(q.at("id")!=id&&q.at("lifecycle")==2&&q.at("x")==x&&q.at("y")==y)occupied=true;}
   if(occupied||movement_cost(r,area.terrain().at("cells").at(y*area.map_width()+x),area.data())==255)continue;
   for(const auto& option:area.spell_options(id,{x,y}))if(option.at("area")==1&&option.at("targets").size()>1){area_found=true;area_actor=id;contact={x,y};break;}
  }
  if(area_found)break;
 }
 check(area_found,"no area tactic with multiple legal targets");
 auto contact_save=Json::parse(area.serialize());auto& state=contact_save["payload"]["state"];state["phase"]="enemy";state["enemy_cursor"]=0;state["selected"]=-1;state["undo"]=nullptr;state["units"][std::to_string(area_actor)]["battle"]["x"]=contact[0];state["units"][std::to_string(area_actor)]["battle"]["y"]=contact[1];state["units"][std::to_string(area_actor)]["battle"]["status"]=0;state["units"][std::to_string(area_actor)]["battle"]["flags"]=0;// Let the real scheduler rebuild its order, with only this caster still to act.
 for(auto& u:state["units"]){auto& b=u["battle"];if(b["side"]==2&&b["id"]!=area_actor)b["flags"]=b["flags"].get<int>()|2;}
 contact_save["payload"]["npc_order"]=Json::array();restore_edit(area,contact_save);
 auto request=area.enemy_request();check(!request.is_null(),"contact caster has no legal enemy decision (confusion/actions/dialogue)");check(request.at("state").at("actor")==area_actor,"scheduler selected a different fixture actor");std::string selected;for(auto it=request.at("choices").begin();it!=request.at("choices").end();++it)if(it.value().value("area",0)==1&&it.value().at("targets").size()>1){selected=it.key();break;}
 check(!selected.empty(),"JEV choices omitted available area tactic");area.choose_enemy(request.at("key"),selected);Prototype area_replay=area;area_replay.restore(area.serialize());area.enemy_step();area_replay.enemy_step();check(area.hash()==area_replay.hash(),"area cast changed after pending-choice save");auto area_packet=area.take_attack();check(!area_packet.is_null()&&area_packet.at("targets").size()>1,"area cast lost secondary targets");
 for(const auto& hit:area_packet.at("targets")){int target=hit.at("target");const auto& before=area_packet.at("before").at("units").at(std::to_string(target)).at("battle");const auto& after=area.state().at("units").at(std::to_string(target)).at("battle");check(before.at("hp").get<int>()-after.at("hp").get<int>()==hit.at("damage"),"area target HP application mismatch");if(hit.at("status_hit").get<bool>()&&(area_packet.at("tactic")==23||area_packet.at("tactic")==25))check(before.at("grades")!=after.at("grades"),"area debuff was not applied");}
 auto area_post=area.serialize();area_replay.restore(area_post);check(area_replay.serialize()==area_post,"area grade/status snapshot failed");
 auto final=game.serialize();replay.restore(final);check(replay.serialize()==final,"extended heal snapshot round trip");
 Json report={{"original_cases",n},{"healing_frames",frames},{"healing_state_hash",game.hash()},{"area_targets",area_found},{"transactional_rejection",true}};if(argc>2)std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
