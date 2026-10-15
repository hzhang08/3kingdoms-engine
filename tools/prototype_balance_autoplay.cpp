#define main historical_scenario_test_main
#include "../tests/prototype_scenarios_tests.cpp"
#undef main
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("assets report required");Prototype g(argv[1]);Json report=Json::array();
 for(int battle=0;battle<4;++battle){g.start_scenario(battle);int attacks=0,misses=0,maximum=0;
 for(int budget=0;budget<3500;++budget){if(!g.has_dialogue()&&(g.state().at("phase")=="victory"||g.state().at("phase")=="defeat"))break;step(g);auto a=g.take_attack();if(!a.is_null()){++attacks;int damage=a.at("damage");misses+=damage==0;maximum=std::max(maximum,damage);}g.take_movement();}
 report.push_back({{"scenario",battle},{"outcome",g.state().at("phase")},{"turn",g.state().at("turn")},{"attacks",attacks},{"misses",misses},{"max_damage",maximum},{"state_hash",g.hash()}});std::cout<<report.back().dump()<<std::endl;}
 std::ofstream(argv[2])<<report.dump(2);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
