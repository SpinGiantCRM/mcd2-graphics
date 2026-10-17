#include "../src/providers/advanced_settings.hpp"
#include <cassert>
#include <iostream>
using namespace mcd2::providers;
struct Hint {
    unsigned value=12,writes=0;bool readable=true,writable=true,readback=true;
    bool read(unsigned& out){if(!readable)return false;out=value;return true;}
    bool write(unsigned v){++writes;if(!writable)return false;if(readback)value=v;return true;}
};
int main(){
    AdvancedRequest r{123,1,{7,0xfedcba98},20,0};AdvancedRequestBytes b;assert(encodeAdvancedRequest(r,b));
    AdvancedRequest decoded;assert(decodeAdvancedRequest(b,decoded)&&decoded==r);
    unsigned rejects=0;
    for(unsigned n=0;n<b.size();++n)for(unsigned bit=0;bit<8;++bit){auto bad=b;bad[n]^=1u<<bit;assert(!decodeAdvancedRequest(bad,decoded));++rejects;}
    for(unsigned n=0;n<b.size();++n){assert(!decodeAdvancedRequest(std::span(b).first(n),decoded));++rejects;}
    for(unsigned field:{5u,6u,7u,9u,10u,11u,15u}){
        auto bad=b;advancedWord(bad,field,field>=11?1:0xffffffffu);advancedWord(bad,4,recordChecksum(bad));assert(!decodeAdvancedRequest(bad,decoded));++rejects;
    }
    Hint ini;AdvancedRequests requests(123,12);auto fresh=[]{return true;};
    auto stale=r;stale.session=124;requests.consume(stale,r.expected,ini,fresh);assert(ini.writes==0&&requests.state().sequence==0);
    requests.consume(r,r.expected,ini,fresh);assert(ini.value==20&&ini.writes==1&&requests.state().status==AdvancedStatus::Saved&&requests.state().startupHint==12);
    requests.consume(r,r.expected,ini,fresh);assert(ini.writes==1);
    auto changed=r;changed.hint=30;requests.consume(changed,r.expected,ini,fresh);assert(ini.writes==1&&requests.state().status==AdvancedStatus::SequenceReuse);
    r.sequence=2;r.hint=0;requests.consume(r,r.expected,ini,fresh);assert(ini.value==0&&ini.writes==2);
    r.sequence=3;r.hint=15;requests.consume(r,{8,123},ini,fresh);assert(ini.writes==2&&requests.state().status==AdvancedStatus::Conflict);
    r.sequence=4;requests.consume(r,r.expected,ini,[]{return false;});assert(ini.writes==2);
    r.sequence=5;r.migrate=1;ini.value=19;requests.consume(r,r.expected,ini,fresh);assert(ini.writes==2&&requests.state().hint==19&&requests.state().migrated==1);
    r.sequence=6;r.migrate=0;ini.readable=false;requests.consume(r,r.expected,ini,fresh);assert(requests.state().status==AdvancedStatus::Invalid&&ini.writes==2);
    r.sequence=7;ini.readable=true;ini.writable=false;requests.consume(r,r.expected,ini,fresh);assert(requests.state().status==AdvancedStatus::WriteFailure&&ini.writes==3);
    r.sequence=8;ini.writable=true;ini.readback=false;requests.consume(r,r.expected,ini,fresh);assert(requests.state().status==AdvancedStatus::WriteFailure&&ini.writes==4);
    AdvancedStateBytes state;assert(encodeAdvancedState(requests.state(),state));
    auto invalid=requests.state();invalid.modules=16;assert(!encodeAdvancedState(invalid,state));
    invalid.modules=17;invalid.versions[0]=310;assert(encodeAdvancedState(invalid,state));
    invalid.versions[0]=65536;assert(!encodeAdvancedState(invalid,state));
    std::cout<<"Advanced settings: "<<rejects<<" corrupt/truncated requests rejected; session/CAS/replay/migration/reset/readback gates pass\n";
}
