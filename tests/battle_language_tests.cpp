#include "caocao/battle_language.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace caocao;
int main(int argc,char** argv){
 if(argc!=2)return 2;Json catalog;std::ifstream(argv[1])>>catalog;BattleLanguage lang;lang.load(catalog);
 auto check=[&](const std::string& source,const std::string& expected){auto got=lang.translate(source);if(got!=expected)throw std::runtime_error(source+" => "+got+", expected "+expected);};
 check("乐进，轻步兵 L3","乐进，轻步兵 L3");lang.english(true);
 check("乐进，轻步兵 L3","Yue Jin, Light Infantry L3");
 check("敌军 张宝 使用毒烟。","Enemy Zhang Bao uses Poison Smoke.");
 check("曹操使用恢复用豆：乐进 兵力 +30","Cao Cao uses Healing Beans: Yue Jin HP +30");
 check("张梁：挡我者死！ 让开让开……！","Zhang Liang: Death to those in my way! Stand aside!");
 check("恢复用豆 × 3","Healing Beans × 3");
 check("弓骑兵1","Horse Archer 1");
 check("回合 12/20","Turn 12/20");
 check("曹操 地形恢复兵力 +30","Cao Cao terrain recovery: HP +30");
 check("No translation needed 123","No translation needed 123");
 for(const auto& [zh,en]:catalog.at("translations").items())check(zh,en);
 // Language changes preserve the source and don't poison the opposite cache.
 lang.english(false);check("乐进，轻步兵 L3","乐进，轻步兵 L3");lang.english(true);check("乐进，轻步兵 L3","Yue Jin, Light Infantry L3");
 std::cout<<catalog.at("translations").size()<<" exact entries and composed/status/switch checks passed\n";
}
