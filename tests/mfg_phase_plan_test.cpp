// Catches duplicate midpoint output, invented capabilities and stale real pairs.
#include "../src/providers/mfg_phase_plan.hpp"
#include <cassert>
using namespace mcd2::mfg;
int main() {
    Capabilities software{6,0,false};
    auto four = make_plan(Mode::Software,4,software);
    assert(four && four->count==3);
    assert(four->phases[0].numerator==1 && four->phases[0].denominator==4);
    assert(four->phases[1].numerator==2 && four->phases[2].numerator==3);
    for (unsigned n=2;n<=6;++n) {
        auto plan=make_plan(Mode::Software,n,software); assert(plan && plan->count==n-1);
        for(unsigned i=0;i<plan->count;++i) {
            assert(plan->phases[i].provider==Provider::Software);
            assert(plan->phases[i].numerator==i+1 && plan->phases[i].denominator==n);
        }
    }
    assert(!make_plan(Mode::Software,1,software));
    assert(!make_plan(Mode::Software,7,software));
    assert(!make_plan(Mode::Software,4,Capabilities{2,6,true}));
    // A stock native presenter is not a compute output capability.
    assert(!make_plan(Mode::Hybrid,4,Capabilities{6,6,false}));
    assert(!make_plan(Mode::Native,4,Capabilities{6,6,false}));
    assert(!make_plan(Mode::Hybrid,3,Capabilities{6,6,true}));
    assert(!make_plan(Mode::Hybrid,5,Capabilities{6,6,true}));
    auto mixed=make_plan(Mode::Hybrid,6,Capabilities{6,2,true});
    assert(mixed && mixed->count==5);
    assert(mixed->phases[0].provider==Provider::Software);
    assert(mixed->phases[1].provider==Provider::Software);
    assert(mixed->phases[2].provider==Provider::NvidiaNative);
    assert(mixed->phases[3].provider==Provider::Software);
    assert(mixed->phases[4].provider==Provider::Software);
    auto native=make_plan(Mode::Native,4,Capabilities{0,4,true});
    assert(native && native->phases[2].provider==Provider::NvidiaNative);
    assert(!make_plan(Mode::Native,6,Capabilities{6,4,true}));
    assert(!make_plan(static_cast<Mode>(99),4,software));

    Batch batch;
    RealPair pair{7,10,11}; // generation epoch, previous real ID, current real ID
    assert(!batch.begin(*four,RealPair{0,10,11}));
    assert(!batch.begin(*four,RealPair{7,11,11}));
    assert(!batch.begin(*four,RealPair{7,12,11}));
    assert(!batch.begin(*four,RealPair{7,10,11,Origin::Generated,Origin::Real}));
    assert(!batch.begin(*four,RealPair{7,10,11,Origin::Real,Origin::Generated}));
    assert(batch.begin(*four,pair));
    assert(!batch.complete());
    assert(batch.record(pair,1,Provider::Software,101,true));
    assert(!batch.complete());
    assert(batch.record(pair,2,Provider::Software,102,true));
    assert(batch.record(pair,3,Provider::Software,103,true));
    assert(batch.complete());
    assert(!batch.begin(*four,pair)); // no batch replacement before invalidation
    assert(!batch.complete()); // Rejected replacement poisons the prior completed result.
    assert(!batch.record(pair,1,Provider::Software,104,true));
    batch.invalidate(); assert(!batch.complete());
    assert(batch.begin(*four,pair));
    assert(!batch.record(RealPair{8,10,11},1,Provider::Software,101,true));
    assert(!batch.record(pair,1,Provider::Software,101,true)); // poisoned batch
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(!batch.record(pair,2,Provider::Software,101,true)); // out of order
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(batch.record(pair,1,Provider::Software,101,true));
    assert(!batch.record(pair,2,Provider::Software,101,true)); // aliased slot
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(!batch.record(pair,1,Provider::NvidiaNative,101,true));
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(!batch.record(pair,1,Provider::Software,101,false)); // failed evaluation
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(!batch.record(pair,1,Provider::Software,0,true));
    batch.invalidate(); auto corrupt=*four; corrupt.phases[0].numerator=2;
    assert(!batch.begin(corrupt,pair));
    batch.invalidate(); assert(batch.begin(*four,pair));
    assert(batch.record(pair,1,Provider::Software,101,true));
    assert(!batch.begin(*four,RealPair{9,20,21}));
    assert(!batch.record(pair,2,Provider::Software,102,true)); // Failed reentry also poisons a partial batch.
    assert(!batch.complete());
    batch.invalidate(); assert(batch.begin(*four,RealPair{9,20,21}));
    // The host supplies monotonic real IDs and the actual origin of its inputs.
}
