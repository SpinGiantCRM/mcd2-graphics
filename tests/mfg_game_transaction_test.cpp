#include "../src/providers/mfg_game_transaction.hpp"
#include <cassert>
#include <type_traits>
using namespace mcd2::mfg;
int main(){
 static_assert(!std::is_copy_constructible_v<GameTransaction>);
 static_assert(!std::is_move_constructible_v<GameTransaction>);
 const Identity id{7,9,123,4,8};const RealPair pair{8,100,101};
 auto plan=*make_plan(Mode::Hybrid,4,{6,2,true});
 GameTransaction tx;
 assert(tx.begin(id,pair,plan));
 assert(!tx.output(id,pair,1));
 assert(tx.record(pair,1,Provider::Software,11,true));
 assert(tx.record(pair,2,Provider::NvidiaNative,12,true));
 assert(tx.record(pair,3,Provider::Software,13,true));
 const Completion work{33,44,5};
 assert(tx.submitted(work));
 assert(!tx.completed({33,45,5},5)); // wrong fence cannot unlock the batch
 assert(!tx.output(id,pair,1));
 assert(tx.completed(work,5));
 assert(tx.output(id,pair,1)==11);
 assert(tx.output(id,pair,2)==12);
 assert(tx.output(id,pair,3)==13);
 assert(!tx.output(id,pair,0)&&!tx.output(id,pair,4));
 assert(!tx.presented(id,pair,2,true)); // cannot skip an intermediate
 assert(!tx.output(id,pair,1));
 tx.invalidate();
 assert(tx.begin(id,pair,plan));
 for(unsigned i=1;i<=3;++i)assert(tx.record(pair,i,plan.phases[i-1].provider,20+i,true));
 assert(tx.submitted({33,44,6}));assert(!tx.completed({33,44,6},5));
 assert(tx.completed({33,44,6},6));
 assert(tx.presented(id,pair,1,true));
 assert(tx.presented(id,pair,2,true));
 assert(tx.presented(id,pair,3,true));
 assert(tx.generated()==3&&tx.nativeGenerated()==1&&tx.softwareGenerated()==2);
 assert(!tx.output(id,pair,1)); // no replay after consumption
 assert(tx.realPresented(id,pair,true));assert(tx.rendered()==1);
 // Completed presentations do not allow reuse until a newer retirement proof.
 assert(!tx.retire({33,44,6},6));
 assert(tx.retire({33,44,7},7));
 const Identity changed{7,10,456,4,8};
 assert(tx.begin(changed,{8,101,102},plan));
 for(unsigned i=1;i<=3;++i)assert(tx.record({8,101,102},i,plan.phases[i-1].provider,30+i,true));
 assert(tx.submitted({33,44,8}));assert(tx.completed({33,44,8},8));
 assert(!tx.output(id,{8,101,102},1)); // previous request cannot read new outputs
 assert(!tx.presented(id,{8,101,102},1,true));
 assert(!tx.output(changed,{8,101,102},1));
 tx.invalidate();
 assert(tx.begin(changed,{8,101,102},plan));
 assert(tx.record({8,101,102},1,Provider::Software,51,true));
 assert(!tx.begin(changed,{8,102,103},plan));
 assert(!tx.record({8,101,102},2,Provider::NvidiaNative,52,true));
 assert(!tx.submitted({33,44,9}));
 tx.invalidate();
 assert(!tx.begin(Identity{},pair,plan));
 assert(!tx.begin(id,{9,100,101},plan));
 assert(!tx.begin(id,{8,100,101,Origin::Generated,Origin::Real},plan));
 assert(tx.begin(id,pair,plan));
 assert(!tx.record(pair,1,Provider::Software,77,false));
 assert(!tx.submitted({33,44,10}));
}
