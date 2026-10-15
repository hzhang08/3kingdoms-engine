#include "caocao/attack_controller.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace caocao;
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("assets path required");auto root=std::filesystem::path(argv[1]);
 auto fixtures=root.parent_path().parent_path()/"tests/fixtures";
 auto data=read_json(root/"data.json"),terrain=read_json(root/"terrain.json").at("cells");
 auto original=read_json(fixtures/"original_attack_controller.json"),palette=read_json(fixtures/"original_combat_palette.json");
 size_t index=0,frames=0;
 for(const auto& c:original.at("cases")){
  std::vector<StrikeTarget> units;for(const auto& row:c.at("before")){
   auto b=BattleUnit::from_json(row.at("battle"));units.push_back({b,UnitRecord::from_json(row.at("unit")),terrain.at(b.y*20+b.x)});
  }
  auto spec=c.at("spec");auto actual=physical_attack(spec.at("actor"),spec.at("target"),units,OriginalRng{spec.at("seed")},{288,576},0,20,20,0,0,data);
  Json reference_events=Json::array(),trace=Json::array();int message=-1;
  for(auto event:c.at("events")){
   auto kind=event.at("kind");
   if(kind=="assembled"||kind=="color_transform")continue;
   if(event.contains("units")){trace.push_back(event);continue;}
   if(kind=="message")message=event.at("index");
   if(kind=="speech"){event.erase("text");event["message"]=message;}
   reference_events.push_back(event);
  }
  if(actual.trace!=trace){for(size_t n=0;n<std::min(actual.trace.size(),trace.size());++n)if(actual.trace[n]!=trace[n])throw std::runtime_error("controller trace mismatch case "+std::to_string(index)+" step "+std::to_string(n)+": "+actual.trace[n].dump()+" != "+trace[n].dump());throw std::runtime_error("controller trace length mismatch");}
  Json actual_events=Json::array();
  for(auto event:actual.events){
   if(event.at("kind")=="unit_state"||event.at("kind")=="record_state")continue;
   actual_events.push_back(event);
  }
  if(actual_events.size()!=reference_events.size())throw std::runtime_error("controller event count mismatch case "+std::to_string(index));
  for(size_t n=0;n<actual_events.size();++n){auto a=actual_events[n],b=reference_events[n];
   if(a.at("kind")=="frame"){
    std::string archive=a.at("archive");archive=archive=="UNIT_ATK.E5"?"Unit_atk.e5":archive=="UNIT_SPC.E5"?"Unit_spc.e5":"Unit_mov.e5";
    std::ostringstream filename;filename<<std::setw(3)<<std::setfill('0')<<a.at("sprite").get<int>()<<".bin";
    std::ifstream f(root/"raw"/archive/filename.str(),std::ios::binary);int w=a.at("width"),h=a.at("height");std::vector<uint8_t> pixels(w*h);
    f.seekg(a.at("frame").get<int>()*w*h);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());if(!f)throw std::runtime_error("missing indexed attack frame");
    if(a.at("mirror").get<bool>())for(int y=0;y<h;++y)std::reverse(pixels.begin()+y*w,pixels.begin()+(y+1)*w);
    pixels=transform_combat_pixels(std::move(pixels),a.at("effects"),palette.at("quantized"),palette.at("lookup"));
    if(sha256(std::string(pixels.begin(),pixels.end()))!=b.at("pixels_sha256").get<std::string>())throw std::runtime_error("controller frame pixels mismatch case "+std::to_string(index));
    for(auto key:{"archive","sprite","frame","mirror","effects"})a.erase(key);
    b.erase("matches");b.erase("pixels_sha256");++frames;
   }
   if(a!=b)throw std::runtime_error("controller event mismatch case "+std::to_string(index)+" event "+std::to_string(n)+": "+a.dump()+" != "+b.dump());
  }
  Json after=Json::array();for(const auto& u:actual.units)after.push_back({{"battle",u.battle.json()},{"unit",u.unit.json()}});
  if(after!=c.at("after")||actual.rng.state!=c.at("rng")||actual.tick!=c.at("tick")||actual.depth!=c.at("depth")||Json(actual.camera)!=c.at("camera"))throw std::runtime_error("controller final state mismatch case "+std::to_string(index));
  ++index;
 }
 std::cout<<index<<" original complete single-target controller sequences and "<<frames<<" indexed frame hashes passed\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
