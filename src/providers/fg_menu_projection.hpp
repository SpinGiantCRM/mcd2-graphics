#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
struct FgMenuProjection {
    std::uint32_t revision=0,provider=0;
    bool enabled=false,pairEligible=false;
    std::uint32_t frameMode=0;
};
// Structural routes using the existing shared SR guide producer. A valid
// preference is not SDK/device approval or proof that current inputs are ready.
inline bool projectFgMenu(const DecodedGraphicsRecord& record,FgMenuProjection& out){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes)||!decodeGraphicsRecord(bytes,canonical)||canonical!=record)return false;
    const auto& i=record.intent;
    const bool single=i.fgStrategy==FgStrategy::Single&&i.requestedMultiplier==2;
    const bool nvidia=single||(i.fgStrategy==FgStrategy::NativeMfg&&i.requestedMultiplier<=6)||i.fgStrategy==FgStrategy::Dynamic;
    const bool pair=(nvidia&&i.fg==FgProvider::Nvidia&&(i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr))||
        (single&&i.fg==FgProvider::Amd&&(i.sr==SrProvider::Native||i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr));
    out={i.revision,unsigned(i.fg),i.fgEnabled,pair,i.fgStrategy==FgStrategy::Dynamic?5u:i.requestedMultiplier-2};return true;
}
constexpr bool fgMenuMatches(const FgMenuProjection& current,std::uint32_t revision,
                            std::uint32_t provider,std::uint32_t mode){
    return current.revision==revision&&current.provider==provider&&unsigned(current.enabled)==mode;
}
// NVIDIA retains its presentation route while user generation is Off.
constexpr std::uint32_t fgStartupOwner(const FgMenuProjection& selection){
    return selection.pairEligible&&selection.provider<2&&
        (selection.enabled||selection.provider==0)?selection.provider+1:0;
}
constexpr bool fgOwnerMatches(std::uint32_t owner,std::uint32_t provider){
    return provider<2&&owner==provider+1;
}
} // namespace mcd2::providers
