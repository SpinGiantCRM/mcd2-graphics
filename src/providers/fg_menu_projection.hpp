#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
struct FgMenuProjection {
    std::uint32_t revision=0,provider=0;
    bool enabled=false,pairEligible=false;
};
// Structural routes using the existing shared SR guide producer. A valid
// preference is not SDK/device approval or proof that current inputs are ready.
inline bool projectFgMenu(const DecodedGraphicsRecord& record,FgMenuProjection& out){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes)||!decodeGraphicsRecord(bytes,canonical)||canonical!=record)return false;
    const auto& i=record.intent;
    const bool single=i.fgStrategy==FgStrategy::Single&&i.requestedMultiplier==2;
    const bool pair=single&&((i.fg==FgProvider::Nvidia&&(i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr))||
        (i.fg==FgProvider::Amd&&(i.sr==SrProvider::Native||i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr)));
    out={i.revision,unsigned(i.fg),i.fgEnabled,pair};return true;
}
constexpr bool fgMenuMatches(const FgMenuProjection& current,std::uint32_t revision,
                            std::uint32_t provider,std::uint32_t mode){
    return current.revision==revision&&current.provider==provider&&unsigned(current.enabled)==mode;
}
constexpr bool fgOwnerMatches(std::uint32_t owner,std::uint32_t provider){
    return provider<2&&owner==provider+1;
}
// A matching startup presenter cannot revive a permanently retired FG session.
// Temporary menu suspension does not retire it; AMD live Off/On retains it.
constexpr bool fgMenuRestartRequired(bool available,bool enabled,std::uint32_t owner,
                                     std::uint32_t provider,bool retired){
    return available&&(retired||(enabled?!fgOwnerMatches(owner,provider):owner!=0));
}
// A verified engine resize may already have restored AMD's native Off route.
// This acknowledges that completed Off transition only; it never admits On,
// revives a retired feature or changes NVIDIA's restart requirement.
constexpr bool fgMenuNativeOffRecovered(bool currentRequest,bool available,bool enabled,
                                        std::uint32_t provider,std::uint32_t route,
                                        std::uint32_t canSwitch,std::uint32_t fault,bool retired){
    return currentRequest&&available&&!enabled&&provider==1&&route==1&&canSwitch==1&&fault==0&&!retired;
}
} // namespace mcd2::providers
