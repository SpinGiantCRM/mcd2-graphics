#pragma once
#include "graphics_record.hpp"
#include "sr_runtime_protocol.hpp"
namespace mcd2::providers {
// Independent guides are admitted only for the implemented Native / AMD pair.
// This is request/source validation, never a substitute for SDK capability.
inline bool nativeFgSelection(const DecodedGraphicsRecord& r){
 GraphicsRecord bytes;DecodedGraphicsRecord canonical;
 return encodeGraphicsRecord(r.intent,bytes)&&decodeGraphicsRecord(bytes,canonical)&&canonical==r&&
  r.intent.sr==SrProvider::Native&&r.intent.fg==FgProvider::Amd&&r.intent.fgEnabled&&
  r.intent.fgStrategy==FgStrategy::Single&&r.intent.requestedMultiplier==2;
}
inline bool nativeFgContext(const DecodedGraphicsRecord& r,const SrContext& c,std::uint32_t session,
                            std::uint64_t now,std::uint64_t observed){
 return nativeFgSelection(r)&&session&&c.session==session&&c.intent==r.bootstrap.stamp&&c.world&&c.ready&&
  c.observedScaleMicro==100000000&&c.worldToMetersMilli==100000&&observed<=now&&now-observed<=3000;
}
}
