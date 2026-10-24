// Every permutation must preserve temporal positions and obey the actual SDK grid.
#include "../src/providers/mfg_phase_plan.hpp"
#include <cassert>
#include <bit>
#include <climits>
#include <tuple>
#include <vector>
using namespace mcd2::mfg;
unsigned reference_count(unsigned total,unsigned mask){
 if(!mask)return 0;
 // Smallest uniform native grid that contains every requested native fraction.
 for(unsigned grid=2;grid<=total;++grid){bool fits=true;
  for(unsigned slot=1;slot<total;++slot)if((mask>>(slot-1))&1u)fits&=(slot*grid)%total==0;
  if(fits)return grid-1;
 }return UINT_MAX;
}
auto reference_score(unsigned total,unsigned mask){
 unsigned largest=0,squares=0;
 for(unsigned provider=0;provider<2;++provider){std::vector<unsigned> points{0};
  for(unsigned slot=1;slot<total;++slot)if(((mask>>(slot-1))&1u)==provider)points.push_back(slot);
  if(points.size()==1)continue;
  points.push_back(total);
  for(unsigned i=1;i<points.size();++i){unsigned gap=points[i]-points[i-1];largest=std::max(largest,gap);squares+=gap*gap;}
 }return std::tuple{largest,squares,reference_count(total,mask),31-mask};
}
int main(){
 // Known ties and a cap that admits only native thirds.
 assert(resolve_order_mask(2,3,0,5)==10u); // F D F D F
 assert(resolve_order_mask(1,1,0,5)==2u); // F D
 assert(resolve_order_mask(1,4,0,1)==4u); // F F D F F
 assert(resolve_order_mask(2,3,0,2)==10u);
 assert(!resolve_order_mask(2,3,32u|3u,2)); // D D needs native sixths
 assert(resolve_order_mask(2,3,32u|3u,5)==3u);
 assert(!valid_order(2,3,64u|32u|3u));
 assert(!valid_order(2,3,32u|1u));assert(!valid_order(1,1,32u|4u));
 assert(!valid_order(UINT_MAX,2,0));assert(!valid_order(0,0,0));
 for(unsigned d=0;d<=5;++d)for(unsigned f=0;f<=5;++f){
  const unsigned count=d+f;if(count<1||count>5)continue;const unsigned total=count+1;
  for(unsigned code=0;code<256;++code){
   bool valid=code==0 || ((code&~63u)==0 && (code&32u) && (code&31u)<(1u<<count) && std::popcount(code&31u)==int(d));
   assert(valid_order(d,f,code)==valid);
   if(!valid){assert(!make_allocation_plan(d,f,{6,6,true},code));continue;}
   for(unsigned cap=0;cap<=5;++cap){std::optional<unsigned> expected;
    for(unsigned mask=0;mask<(1u<<count);++mask){if(std::popcount(mask)!=int(d)||reference_count(total,mask)>cap || (code && mask!=(code&31u)))continue;
     if(!expected || reference_score(total,mask)<reference_score(total,*expected))expected=mask;
    }
    assert(resolve_order_mask(d,f,code,cap)==expected);
    auto p=make_allocation_plan(d,f,{6,cap+1,true},code);assert(bool(p)==bool(expected));
    if(!p)continue;
    assert(p->order==code);
    assert(native_count_for_mask(total,*expected)==reference_count(total,*expected));
    Batch batch;assert(batch.begin(*p,{1,1,2}));
    for(unsigned slot=1;slot<=count;++slot){auto phase=p->phases[slot-1];bool native=(*expected>>(slot-1))&1u;
     assert(phase.numerator==slot && phase.denominator==total);
     assert((phase.provider==Provider::NvidiaNative)==native);
     if(native)assert(native_phase_index(*p,slot)*total==slot*(native_generated_count(*p)+1));
     assert(batch.record({1,1,2},slot,phase.provider,100+slot,true));
    }assert(batch.complete());
   }
  }
 }
 auto manual=*make_allocation_plan(2,3,{6,6,true},35);Batch bad;
 manual.order=34;assert(!bad.begin(manual,{1,1,2}));
 assert(!make_allocation_plan(1,0,{6,6,false},33));
}
