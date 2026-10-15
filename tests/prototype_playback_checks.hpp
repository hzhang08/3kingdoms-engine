#pragma once
#include "caocao/prototype_playback.hpp"
// Presentation tests do not contribute to the historical simulation trace.
void check_playback(const std::filesystem::path& assets,const std::filesystem::path& root){
 Prototype game(assets,false,false),restored(assets,false,false);
 game.command("tile",10,6);game.command("tile",8,9);
 const auto saved=game.serialize();game.command("tile",8,10);
 const auto committed=game.serialize(),hash=game.hash();
 check(hash=="908ed536888bb4c0fb0385717c8d560b173b428b553200c56c55266c2ff49f5b","animation changed historical first strike");
 auto packet=game.take_attack();check(!packet.is_null()&&game.take_attack().is_null(),"attack packet not consume-once");
 auto id=std::to_string(packet.at("target").get<int>());
 auto hp=packet.at("before").at("units").at(id).at("battle").at("hp").get<int>();
 check(hp-game.state().at("units").at(id).at("battle").at("hp").get<int>()==packet.at("damage"),"animation lacks pre-damage HP");
 PrototypePlayback playback;playback.start(packet);std::vector<int> sounds;
 auto drain=[&]{auto batch=playback.take_sounds();sounds.insert(sounds.end(),batch.begin(),batch.end());check(playback.take_sounds().empty(),"sound repeated");};
 drain();check(playback.active()&&playback.number().is_null(),"animation start invalid");
 for(int tick=0;tick<27;++tick){
  check(playback.active(),"animation ended early");
  check(playback.before().at("units").at(id).at("battle").at("hp")==hp,"display HP committed prematurely");
  if(tick==13){bool flash=false;for(const auto& pose:playback.poses())if(!pose.at("effects").empty())flash=true;check(flash,"missing impact flash");}
  if(tick==17)check(playback.number().at("value")==packet.at("damage"),"wrong damage number");
  if(tick==25)check(playback.number().is_null(),"number not cleared by redraw");
  playback.advance(39);check(playback.active(),"partial wait ended early");playback.advance(1);drain();
 }
 check(!playback.active()&&playback.poses().empty()&&playback.before().is_null(),"playback did not finish cleanly");
 check(sounds==std::vector<int>({34,35}),"Cao Cao sound order differs");
 playback.advance(10000);check(game.serialize()==committed&&playback.take_sounds().empty(),"presentation repeated damage or sounds");
 PrototypePlayback coarse;coarse.start(packet);auto once=coarse.take_sounds();coarse.advance(10000);auto tail=coarse.take_sounds();once.insert(once.end(),tail.begin(),tail.end());
 check(!coarse.active()&&once==sounds,"frame cadence changed sequence");
 restored.restore(committed);check(restored.hash()==hash&&restored.take_attack().is_null(),"restore replayed an attack");
 game.restore(saved);game.command("tile",8,10);game.start();check(game.take_attack().is_null(),"restart retained attack");
 // Last enemy dies: present its living, pre-impact state before the victory screen.
 auto kill=Json::parse(saved);auto& state=kill["payload"]["state"];
 for(auto& u:state["units"])if(u["battle"]["side"]==2){u["battle"]["hp"]=0;u["battle"]["lifecycle"]=3;}
 state["units"][id]["battle"]["hp"]=1;state["units"][id]["battle"]["lifecycle"]=2;
 kill["sha256"]=sha256(kill.at("payload").dump());game.restore(kill.dump());game.command("tile",8,10);auto death=game.take_attack();
 playback.start(death);check(game.state().at("phase")=="victory"&&playback.before().at("phase")=="player"&&playback.before().at("units").at(id).at("battle").at("hp")==1,"kill/victory presentation boundary wrong");
 playback.clear();check(playback.take_sounds().empty()&&!playback.active(),"clear retained sound/animation");
 // Every current/legacy unit, in all four facings, must have staged swing,
 // reaction and overlay frames. Check actual raw byte extents, not filenames only.
 int cases=0;
 for(bool legacy:{false,true}){
  game.start();if(legacy){std::ifstream f(root/"tests/fixtures/prototype-legacy/attack.snapshot.json");game.restore(std::string{std::istreambuf_iterator<char>(f),{}});}
  for(const auto& u:game.state().at("units"))for(int facing=0;facing<4;++facing){
   auto actor=BattleUnit::from_json(u.at("battle")),target=actor;auto au=UnitRecord::from_json(u.at("record"));
   actor.direction=facing;actor.flags=(actor.flags&~8)|(facing==1?8:0);target.slot=200;target.x=actor.x+1;
   auto animation=physical_attack_animation(actor,au,target,au,10,0,false,0,0,0,game.data());
   for(const auto& e:animation.events){
    if(e.at("kind")=="sound")check(std::filesystem::file_size(assets/battle_sound_file(e.at("id")))>44,"missing staged sound");
    if(e.at("kind")!="frame")continue;
    std::string archive=e.at("archive"),stem=archive=="UNIT_ATK.E5"?"Unit_atk":archive=="UNIT_SPC.E5"?"Unit_spc":"Unit_mov";
    auto number=std::to_string(e.at("sprite").get<int>());number=std::string(3-number.size(),'0')+number;
    auto bytes=std::filesystem::file_size(assets/"raw"/(stem+".e5")/(number+".bin"));
    check(bytes>=size_t((e.at("frame").get<int>()+1)*e.at("width").get<int>()*e.at("height").get<int>()),"staged attack frame truncated");
   }++cases;
  }
 }
 // Unsupported presentation instructions must fail explicitly.
 packet["events"]=Json::array({{{"kind","unknown"}}});bool rejected=false;
 try{playback.start(packet);}catch(const std::exception&){rejected=true;}
 check(rejected,"unsupported presentation event silently skipped");
 std::cout<<"attack playback: atomic HP/RNG, consume-once events, 27 waits, flash/number/sounds, kill/victory, restore/restart and "<<cases<<" roster/facing assets passed\n";
}
