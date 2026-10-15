#include "caocao/range.hpp"
#include <stdexcept>
namespace caocao {
int battle_attack_range(const UnitRecord& u,const Json& data){
 int kind=data.at("classes").at(u.job).at(1);
 for(int slot=0;slot<3;++slot){int id=u.equipment[slot][0];if(id==255)continue;
  const auto& item=data.at("items").at(id);int type=item.at("type");
  if(slot==2&&type==42)kind=item.at("strength");
  else if(slot<2&&type<18&&type%2==1&&item.at("special_type")==42)kind=item.at("special_strength");
 }
 return kind;
}
RangeResult battle_range(const BattleUnit& b,std::array<int,2> center,int kind,int mode,
 bool mask_only,int preferred,int width,int height,RangeGrids grids,const std::vector<BattleUnit>& roster){
 if(width<1||width>40||height<1||height>40||center[0]<0||center[0]>=width||center[1]<0||center[1]>=height||grids.distance.size()!=static_cast<size_t>(width*height)||grids.threat.size()!=grids.distance.size()||grids.mask.size()!=grids.distance.size()||kind<0||kind>255||mode<0||mode>255)throw std::runtime_error("invalid range input");
 RangeResult out{std::move(grids),{},255};if(kind==255)return out;
 kind&=127;if(kind>15)throw std::runtime_error("unverified range kind");
 auto emit=[&](int x,int y){x&=255;y&=255;if(x>=width||y>=height)return;
  int index=y*width+x;if(!(mode&128)){out.grids.distance[index]=b.slot;out.grids.threat[index]=b.slot;}
  if(mask_only){out.grids.mask[index]=1;return;}
  for(const auto& target:roster)if(target.lifecycle==2&&target.x==x&&target.y==y){
   if(target.hp>0&&(((target.slot<35)==(b.slot<35))==bool(mode&1)))out.targets.push_back(target.slot);
   break;
  }
 };
 auto point=[&](int x,int y){emit(center[0]+x,center[1]+y);};
 auto cardinals=[&](){point(0,-1);point(1,0);point(0,1);point(-1,0);};
 auto diagonals=[&](){point(1,-1);point(1,1);point(-1,1);point(-1,-1);};
 auto ends=[&](int r){point(-r,0);point(r,0);point(0,-r);point(0,r);};
 auto edges=[&](int r){for(int n=-1;n<=1;++n){point(n,-r);point(n,r);}for(int n=-1;n<=1;++n){point(r,n);point(-r,n);}};
 auto ring=[&](int r){for(int y=-r;y<=r;++y){int x=r-std::abs(y);point(-x,y);if(x)point(x,y);}};
 if(mode&1)point(0,0);
 switch(kind){
 case 0:cardinals();break;
 case 1:cardinals();diagonals();break;
 case 2:diagonals();ends(2);break;
 case 3:diagonals();edges(2);ends(3);break;
 case 4:case 13:
  if(kind==13){cardinals();}
  diagonals();edges(2);edges(3);
  point(-2,-2);point(2,2);point(2,-2);point(-2,2);ends(4);break;
 case 5:cardinals();ends(2);break;
 case 6:cardinals();diagonals();ends(2);break;
 case 7:ring(4);break;
 case 8:ring(4);ring(5);break;
 case 9:ends(2);break;
 case 10:for(int y=0;y<height;++y)for(int x=0;x<width;++x)emit(x,y);break;
 case 11:break;
 case 12:cardinals();diagonals();edges(2);ends(3);break;
 case 14:for(int n=-1;n<=1;++n){point(-3,n);point(3,n);point(n,-3);point(n,3);}break;
 case 15:for(int n=-2;n<=2;++n){point(n,-3);point(n,3);point(-3,n);point(3,n);}break;
 }
 if(!mask_only)for(int slot:out.targets){out.selected=slot;if(slot==preferred)break;}
 return out;
}
}
