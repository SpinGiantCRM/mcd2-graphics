#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
// Volatile process/world state, never another persisted settings authority.
// UE and the native worker exchange explicit LE words through mod-owned slots.
using SrRuntimeBytes=std::array<std::uint8_t,128>;
enum class SrPhase : std::uint32_t { Native, Preflight, ApplySource, Create, Evaluate, Active,
    Retire, RestoreSource, Fallback, Failed };
struct SrContext {
    std::uint32_t session=0,sequence=0;
    RecordStamp intent;
    std::uint32_t world=0;
    bool ready=false;
    std::uint32_t worldToMetersMilli=0,observedScaleMicro=0;
    std::uint32_t sourceAck=0,sourceScaleMicro=0,sourceError=0;
    bool operator==(const SrContext&)const=default;
};
struct SrRuntimeState {
    std::uint32_t session=0,sequence=0;
    RecordStamp intent;
    std::uint32_t world=0;
    SrPhase phase=SrPhase::Native;
    std::uint32_t error=0,renderWidth=0,renderHeight=0,outputWidth=0,outputHeight=0;
    // Millionths of a percent: 100% = 100,000,000. The source command therefore
    // preserves SDK ratios instead of rounding them to a menu slider percent.
    std::uint32_t sourceScaleMicro=100000000,evaluations=0;
    SrProvider provider=SrProvider::Native;
    Quality quality=Quality::NativeAA;
    std::uint32_t capabilities=0;
    bool operator==(const SrRuntimeState&)const=default;
};
inline bool scaleMicroValid(std::uint32_t n){return n>=1000000 && n<=100000000;}
inline bool validContext(const SrContext& c){
    return validRevision(c.session)&&validRevision(c.sequence)&&validRevision(c.intent.revision)&&
        validRevision(c.world)&&c.worldToMetersMilli<=1000000000 &&
        (!c.ready || c.worldToMetersMilli>0) && (scaleMicroValid(c.observedScaleMicro)||(!c.ready&&!c.observedScaleMicro))&&
        c.sourceAck<=0x7fffffff && c.sourceError<=2 &&
        (!c.sourceAck ? !c.sourceScaleMicro&&!c.sourceError : scaleMicroValid(c.sourceScaleMicro));
}
inline bool validRuntimeState(const SrRuntimeState& s){
    if(!validRevision(s.session)||!validRevision(s.sequence)||!validRevision(s.intent.revision)||
       !validRevision(s.world)||unsigned(s.phase)>9||s.error>64||unsigned(s.provider)>3||
       unsigned(s.quality)>5||!scaleMicroValid(s.sourceScaleMicro)||s.evaluations>0x7fffffff||s.capabilities>15)return false;
    if(s.phase==SrPhase::ApplySource || s.phase==SrPhase::Create || s.phase==SrPhase::Evaluate || s.phase==SrPhase::Active)
        return s.renderWidth>=640&&s.renderHeight>=360&&s.renderWidth<=s.outputWidth&&
            s.renderHeight<=s.outputHeight&&s.outputWidth<=7680&&s.outputHeight<=4320;
    return true;
}
namespace sr_runtime_detail {
inline void put(SrRuntimeBytes& b,unsigned word,std::uint32_t n){setRecordWord(b,word*4,n);}
inline void header(SrRuntimeBytes& b,std::uint32_t kind,std::uint32_t session,std::uint32_t sequence,
                   RecordStamp stamp,std::uint32_t world){
    put(b,0,0x3244434d);put(b,1,kind);put(b,2,1);put(b,3,128);put(b,5,session);
    put(b,6,sequence);put(b,7,stamp.revision);put(b,8,stamp.checksum);put(b,9,world);
}
inline bool framing(std::span<const std::uint8_t> b,std::uint32_t kind,unsigned last){
    if(b.size()!=128||recordWord(b,0)!=0x3244434d||recordWord(b,4)!=kind||
       recordWord(b,8)!=1||recordWord(b,12)!=128||recordWord(b,16)!=recordChecksum(b))return false;
    return std::all_of(b.begin()+last*4,b.end(),[](auto v){return v==0;});
}
}
inline bool encodeSrContext(const SrContext& c,SrRuntimeBytes& out){
    if(!validContext(c))return false;
    SrRuntimeBytes b{};using namespace sr_runtime_detail;
    header(b,0x31584353,c.session,c.sequence,c.intent,c.world);
    put(b,10,c.ready);put(b,11,c.worldToMetersMilli);put(b,12,c.observedScaleMicro);
    put(b,13,c.sourceAck);put(b,14,c.sourceScaleMicro);put(b,15,c.sourceError);
    put(b,4,recordChecksum(b));out=b;return true;
}
inline bool decodeSrContext(std::span<const std::uint8_t> b,SrContext& out){
    if(!sr_runtime_detail::framing(b,0x31584353,16)||recordWord(b,40)>1)return false;
    auto get=[&](unsigned w){return recordWord(b,w*4);};
    SrContext c{get(5),get(6),{get(7),get(8)},get(9),get(10)!=0,get(11),get(12),get(13),get(14),get(15)};
    if(!validContext(c))return false;
    out=c;return true;
}
inline bool encodeSrRuntimeState(const SrRuntimeState& s,SrRuntimeBytes& out){
    if(!validRuntimeState(s))return false;
    SrRuntimeBytes b{};using namespace sr_runtime_detail;
    header(b,0x31545353,s.session,s.sequence,s.intent,s.world);
    put(b,10,unsigned(s.phase));put(b,11,s.error);put(b,12,s.renderWidth);put(b,13,s.renderHeight);
    put(b,14,s.outputWidth);put(b,15,s.outputHeight);put(b,16,s.sourceScaleMicro);put(b,17,s.evaluations);
    put(b,18,unsigned(s.provider));put(b,19,unsigned(s.quality));put(b,20,s.capabilities);
    put(b,4,recordChecksum(b));out=b;return true;
}
inline bool decodeSrRuntimeState(std::span<const std::uint8_t> b,SrRuntimeState& out){
    if(!sr_runtime_detail::framing(b,0x31545353,21))return false;
    auto get=[&](unsigned w){return recordWord(b,w*4);};
    SrRuntimeState s{get(5),get(6),{get(7),get(8)},get(9),SrPhase(get(10)),get(11),get(12),get(13),get(14),get(15),
        get(16),get(17),SrProvider(get(18)),Quality(get(19)),get(20)};
    if(!validRuntimeState(s))return false;
    out=s;return true;
}
// A persistence receipt never satisfies this check. Exact runtime request,
// current process, current world and observed source scale are all required.
inline bool sourceAcknowledged(const SrRuntimeState& s,const SrContext& c){
    return validRuntimeState(s)&&validContext(c)&&(c.ready||s.phase==SrPhase::RestoreSource)&&c.session==s.session&&c.intent==s.intent&&
        c.world==s.world&&c.sourceAck==s.sequence&&!c.sourceError&&c.sourceScaleMicro==s.sourceScaleMicro&&
        (c.observedScaleMicro>s.sourceScaleMicro?c.observedScaleMicro-s.sourceScaleMicro:
            s.sourceScaleMicro-c.observedScaleMicro)<=100;
}
} // namespace mcd2::providers
