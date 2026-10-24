#include "../src/providers/mfg_component_projection.hpp"
#include <cassert>
using namespace mcd2::diagnostics;
int main(){
 const MCD2DiagnosticIdentityV1 id{1,2,3,4};
 MfgComponentSample s{id,100,2,4,1,0,20,10};
 auto p=mfgComponents(id,s,110);assert(p.strategy==2&&p.software==20&&p.native==10);
 assert(!mfgComponents(id,s,99).strategy);assert(!mfgComponents(id,s,1601).strategy);
 auto wrong=id;wrong.world++;assert(!mfgComponents(wrong,s,110).strategy);
 s.active=0;assert(!mfgComponents(id,s,110).strategy);s.active=1;s.fault=1;assert(!mfgComponents(id,s,110).strategy);s.fault=0;
 s.multiplier=3;assert(!mfgComponents(id,s,110).strategy);
 s.strategy=3;s.multiplier=6;assert(!mfgComponents(id,s,110).strategy);s.native=0;
 p=mfgComponents(id,s,110);assert(p.strategy==3&&p.software==20&&p.native==0);
 s.strategy=4;assert(!mfgComponents(id,s,110).strategy);
}
