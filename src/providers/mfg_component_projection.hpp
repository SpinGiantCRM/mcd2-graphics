#pragma once
#include "runtime_diagnostics.h"
namespace mcd2::diagnostics {
struct MfgComponentSample {
 MCD2DiagnosticIdentityV1 identity;
 std::uint64_t observedAtMs{};
 std::uint32_t strategy{},multiplier{},active{},fault{};
 std::uint64_t software{},native{};
};
struct MfgComponents {std::uint32_t strategy{},software{},native{};};
inline MfgComponents mfgComponents(MCD2DiagnosticIdentityV1 current,const MfgComponentSample& s,std::uint64_t now){
 if(!current.session||!current.revision||!current.world||current.session!=s.identity.session||
    current.revision!=s.identity.revision||current.checksum!=s.identity.checksum||current.world!=s.identity.world||
    s.active!=1||s.fault||!s.observedAtMs||s.observedAtMs>now||now-s.observedAtMs>1500)return {};
 if(s.strategy==2){if(s.multiplier!=4&&s.multiplier!=6)return {};}
 else if(s.strategy==3){if(s.multiplier<2||s.multiplier>6||s.native)return {};}
 else return {};
 return {s.strategy,std::uint32_t(s.software&0x7fffffff),std::uint32_t(s.native&0x7fffffff)};
}
}
