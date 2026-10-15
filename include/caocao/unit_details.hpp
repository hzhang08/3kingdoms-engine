#pragma once
#include "caocao/prototype.hpp"
namespace caocao {
// Presentation-only projection. Does not consume events, RNG, or mutate saves.
Json unit_details(const Prototype&,int id,const Json& ui,const Json& tactics);
}
