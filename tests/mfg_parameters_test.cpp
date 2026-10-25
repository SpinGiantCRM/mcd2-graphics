// Catches lossy numeric conversion, pointer/scalar confusion and unbounded keys.
#include "../src/native/mfg_parameters.hpp"
#include <cassert>
#include <cmath>
#include <limits>
using mcd2::mfg::Parameters;
int main() {
    Parameters p; unsigned u=77; int i=77; float f=77; void* ptr=reinterpret_cast<void*>(1);
    assert(!p.get("missing",u) && u==77);
    assert(!p.set(nullptr,1u));
    assert(!p.set("",1u));
    assert(!p.set(std::string(128,'a').c_str(),1u));
    assert(p.set("count",3u) && p.get("count",i) && i==3);
    assert(p.get("count",f) && f==3.f);
    assert(!p.get("count",ptr) && ptr==reinterpret_cast<void*>(1));
    assert(p.set("negative",-1) && !p.get("negative",u));
    assert(p.set("fraction",1.25f) && !p.get("fraction",u));
    assert(p.set("large",std::numeric_limits<unsigned long long>::max()));
    unsigned long long ull=0;
    assert(p.get("large",ull) && ull==std::numeric_limits<unsigned long long>::max());
    assert(!p.get("large",u) && !p.get("large",i));
    double d=77;
    assert(p.set("precision",9007199254740993ull));
    assert(!p.get("precision",d) && d==77);
    assert(!p.get("large",d) && d==77);
    assert(p.set("float-precision",16777217ull)); f=77;
    assert(!p.get("float-precision",f) && f==77);
    assert(p.set("exact-double",9007199254740992ull));
    assert(p.get("exact-double",d) && d==9007199254740992.);
    assert(p.set("overflow",std::ldexp(1.,64)) && !p.get("overflow",ull));
    assert(!p.set("nan",std::nanf("")));
    assert(!p.set("inf",INFINITY));
    assert(p.set("ptr",ptr)); void* result=nullptr;
    assert(p.get("ptr",result) && result==ptr);
    assert(!p.get("ptr",u));
    assert(p.set("null",static_cast<void*>(nullptr)));
    assert(p.get("null",result) && result==nullptr);
    assert(p.set("count",4) && p.get("count",u) && u==4);
    p.reset(); assert(!p.get("count",u));
    for(unsigned n=0;n<128;++n) assert(p.set(std::to_string(n).c_str(),n));
    assert(!p.set("overflow-key",1));
    assert(p.set("0",9) && p.get("0",i) && i==9); // bounded overwrite works
    p.reset(); assert(p.set("new",1));
}
