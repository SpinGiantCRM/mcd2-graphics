#include "../experiments/providers/mfg_game_timeline.hpp"
#include <cassert>
using namespace mcd2::mfg;
int main(){
 RealCadence c;assert(!c.observe(1000000000));assert(!c.observe(1120000000));assert(c.observe(1240000000));
 auto p=c.schedule(1241000000,1120000000,6,16666666,0,false);assert(p&&p.step==20000000&&p.sync==0);for(unsigned i=1;i<6;++i)assert(p.deadline(i)-p.deadline(i-1)==20000000);assert(p.end-p.start==100000000);
 // Incoming interval is a real-cadence constraint, not repeated on every phase.
 auto v=c.schedule(1241000000,1120000000,4,16666666,2,false);assert(v&&v.sync==1&&v.step==30000000);
 auto limited=c.schedule(1241000000,1120000000,6,25000000,0,false);assert(limited&&limited.limited&&limited.step==25000000);assert(!c.schedule(1241000000,1120000000,6,0,0,false));assert(!c.schedule(1500000000,1120000000,6,16666666,0,false));
 auto t=c.schedule(1241000000,1120000000,6,16666666,0,true);assert(t&&t.sync==0);assert(!c.schedule(1241000000,1120000000,6,16666666,1,true));
 assert(!c.observe(2000000000));assert(!c.period());assert(!c.observe(2000000000));assert(!c.observe(1990000000));
 RealCadence render;assert(!render.observe(1000000000));assert(!render.observe(1030000000,22000000));assert(render.observe(1060000000,22000000));assert(render.period()==8000000);auto fit=render.schedule(1061000000,1030000000,6,4166666,0,false);assert(fit&&fit.limited&&fit.step==4166666);
 assert(!c.observe(2110000000));assert(c.observe(2230000000));assert(!c.observe(2240000000));assert(!c.period());
}
