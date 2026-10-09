#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
struct FgMenuProjection {
    std::uint32_t revision=0,provider=0;
    bool enabled=false,pairEligible=false;
};
// Current implemented pairings only. A valid preference is not SDK approval.
inline bool projectFgMenu(const DecodedGraphicsRecord& record,FgMenuProjection& out){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes)||!decodeGraphicsRecord(bytes,canonical)||canonical!=record)return false;
    const auto& i=record.intent;
    const bool single=i.fgStrategy==FgStrategy::Single&&i.requestedMultiplier==2;
    const bool pair=single&&((i.fg==FgProvider::Nvidia&&i.sr==SrProvider::NvidiaDlss)||
        (i.fg==FgProvider::Amd&&(i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr)));
    out={i.revision,unsigned(i.fg),i.fgEnabled,pair};return true;
}
constexpr bool fgMenuMatches(const FgMenuProjection& current,std::uint32_t revision,
                            std::uint32_t provider,std::uint32_t mode){
    return current.revision==revision&&current.provider==provider&&unsigned(current.enabled)==mode;
}
constexpr bool fgOwnerMatches(std::uint32_t owner,std::uint32_t provider){
    return provider<2&&owner==provider+1;
}
} // namespace mcd2::providers
