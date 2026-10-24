#include "../src/providers/fg_menu_projection.hpp"
#include <cassert>
using namespace mcd2::providers;
int main(){
 for(auto sr:{SrProvider::NvidiaDlss,SrProvider::AmdFsr})for(auto strategy:{2u,3u})for(unsigned n=2;n<=7;++n){
  GraphicsIntent i;i.sr=sr;i.fgEnabled=true;i.fg=strategy==2?FgProvider::Nvidia:FgProvider::Amd;
  i.fgStrategy=static_cast<FgStrategy>(strategy);i.requestedMultiplier=n;
  GraphicsRecord bytes;DecodedGraphicsRecord r;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));
  FgMenuProjection p;assert(projectFgMenu(r,p));
  const bool eligible=strategy==2?(n==4||n==6):n<=6;
  assert(p.pairEligible==eligible);
  assert(fgStartupOwner(p)==(eligible?4u:0u));
  if(eligible){assert(unsigned(r.bootstrap.requested)==4);assert(fgOwnerMatches(4,unsigned(i.fg),strategy));assert(!fgOwnerMatches(unsigned(i.fg)+1,unsigned(i.fg),strategy));assert(!fgMenuRestartRequired(true,true,4,unsigned(i.fg),false,strategy));}
  i.fgEnabled=false;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r)&&projectFgMenu(r,p));
  assert(fgStartupOwner(p)==0&&r.bootstrap.requested==PresentationOwner::Native);
 }
 FgMenuProjection current{7,0,true,true,2,2};
 assert(fgMenuSourceMatches(current,7,0,1,2,2));
 assert(!fgMenuSourceMatches(current,7,0,1,4,2));
 assert(!fgMenuSourceMatches(current,7,0,1,2,1));
 assert(fgMenuNativeOffRecovered(true,true,false,0,1,1,0,false,2));
 assert(!fgMenuNativeOffRecovered(true,true,false,0,4,1,0,false,2));
 GraphicsIntent i;i.fgEnabled=true;i.sr=SrProvider::Native;i.fg=FgProvider::Amd;i.fgStrategy=static_cast<FgStrategy>(3);i.requestedMultiplier=4;
 GraphicsRecord b;DecodedGraphicsRecord r;FgMenuProjection p;
 assert(encodeGraphicsRecord(i,b)&&decodeGraphicsRecord(b,r)&&projectFgMenu(r,p)&&!p.pairEligible);
 i.sr=SrProvider::AmdFsr;i.fg=FgProvider::Nvidia;assert(encodeGraphicsRecord(i,b)&&decodeGraphicsRecord(b,r)&&projectFgMenu(r,p)&&!p.pairEligible);
 i.fg=FgProvider::Amd;i.fgStrategy=FgStrategy::Hybrid;assert(encodeGraphicsRecord(i,b)&&decodeGraphicsRecord(b,r)&&projectFgMenu(r,p)&&!p.pairEligible);
}
