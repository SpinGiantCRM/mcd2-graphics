#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
// Project the single committed authority for the existing display/Reflex
// controller. This is requested state, not capability or an active-mode ACK.
struct DisplayProjection {
    bool valid=false;
    std::uint32_t revision=0,hdr=0,peak=0,paper=0,ui=0,reflex=0;
};
inline DisplayProjection projectDisplay(const DecodedGraphicsRecord& record){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes) || !decodeGraphicsRecord(bytes,canonical) ||
       canonical!=record)return {};
    const auto& i=record.intent;
    const bool reflex=i.latency==LatencyProvider::Automatic || i.latency==LatencyProvider::NvidiaReflex;
    return {true,i.revision,unsigned(i.hdr.enabled),i.hdr.peakNits,i.hdr.paperWhiteNits,i.hdr.uiNits,
            reflex?unsigned(i.latencyMode):0};
}
} // namespace mcd2::providers
