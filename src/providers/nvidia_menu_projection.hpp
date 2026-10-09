#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
// Read-only adapter for the already qualified NVIDIA controller. Source ACKs
// remain process-local runtime data; saved legacy preferences are not authority.
struct NvidiaMenuProjection {
    std::uint32_t revision=0,mode=0,preset=0,custom=6700,scale=10000;
    bool fallback=true;
};
inline bool projectNvidiaMenu(const DecodedGraphicsRecord& record,NvidiaMenuProjection& out){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes)||!decodeGraphicsRecord(bytes,canonical)||canonical!=record)return false;
    const auto& i=record.intent;const auto& pref=i.srPreferences[0];
    const auto q=unsigned(pref.quality);
    const auto preset=q==0?5u:q==5?4u:q-1;
    const auto custom=q==0?10000u:q==1?6700u:q==2?5800u:q==3?5000u:q==4?3300u:pref.customScaleBasisPoints;
    // Keep the established NVIDIA alias contract; another provider's Custom
    // ratios do not use these aliases.
    if(q==5&&(custom==10000||custom==6700||custom==5800||custom==5000||custom==3300))return false;
    const auto mode=i.sr==SrProvider::NvidiaDlss?(q==0?1u:2u):0u;
    const auto scale=mode!=2?10000u:q==1?6667u:q==4?3333u:custom;
    out={i.revision,mode,preset,custom,scale,i.nativeFallbackPreference};return true;
}
}
