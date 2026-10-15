#pragma once
#include "caocao/engine.hpp"
#include <array>
namespace caocao {
// Scenario descriptors are distinct from live battlefield units. Hidden units
// and reinforcements retain their descriptor until the original script uses it.
struct DeploymentSlot {
 int slot=0,id=65535,x=0,y=0,flags=0,ai=0,target=0,promotion_mode=0,level=0;
 std::array<int,3> equipment_overrides{};
 int weapon_level=0,armor_level=0;
 Json json()const;
 static DeploymentSlot from_json(const Json&);
};
int deployment_level(std::vector<int> levels);
DeploymentSlot deployment_row(int slot,bool enemy,const std::vector<int>& args,int base_level);
void position_deployment(DeploymentSlot&,int x,int y,int direction,int hidden);
void equipment_deployment(DeploymentSlot&,const std::array<int,5>& args);
}
