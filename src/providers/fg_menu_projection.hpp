#pragma once
#include "graphics_record.hpp"
#include "mfg_phase_plan.hpp"

namespace mcd2::providers {
struct FgMenuProjection {
    std::uint32_t revision=0,provider=0;
    bool enabled=false,pairEligible=false;
    std::uint32_t frameMode=0,strategy=0;
    std::uint32_t dlssGenerated=0,fsrGenerated=0;
    RecordStamp stamp{};
};
// Structural routes using the existing shared SR guide producer. A valid
// preference is not SDK/device approval or proof that current inputs are ready.
inline bool projectFgMenu(const DecodedGraphicsRecord& record,FgMenuProjection& out){
    GraphicsRecord bytes;DecodedGraphicsRecord canonical;
    if(!encodeGraphicsRecord(record.intent,bytes)||!decodeGraphicsRecord(bytes,canonical)||canonical!=record)return false;
    const auto& i=record.intent;
    const auto allocation=fgAllocation(i);
    const bool explicitCounts=i.dlssGenerated||i.fsrGenerated;
    const bool single=i.fgStrategy==FgStrategy::Single&&i.requestedMultiplier==2;
    const bool nvidia=single||(i.fgStrategy==FgStrategy::NativeMfg&&i.requestedMultiplier<=6);
    const bool shared=i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr;
    const bool custom=shared&&((i.fgStrategy==FgStrategy::SoftwareMfg&&i.fg==FgProvider::Amd&&i.requestedMultiplier<=6)||
        (i.fgStrategy==FgStrategy::Hybrid&&i.fg==FgProvider::Nvidia&&(explicitCounts||(i.requestedMultiplier==4||i.requestedMultiplier==6))));
    const bool pair=custom||(nvidia&&i.fg==FgProvider::Nvidia&&(i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr))||
        (single&&i.fg==FgProvider::Amd&&(i.sr==SrProvider::Native||i.sr==SrProvider::NvidiaDlss||i.sr==SrProvider::AmdFsr));
    out={i.revision,unsigned(i.fg),i.fgEnabled,pair,i.requestedMultiplier-2,unsigned(i.fgStrategy),
        allocation.dlssGenerated,allocation.fsrGenerated,record.bootstrap.stamp};return true;
}
constexpr bool fgMenuMatches(const FgMenuProjection& current,std::uint32_t revision,
                            std::uint32_t provider,std::uint32_t mode){
    return current.revision==revision&&current.provider==provider&&unsigned(current.enabled)==mode;
}
constexpr bool fgMenuSourceMatches(const FgMenuProjection& p,std::uint32_t revision,std::uint32_t provider,
                                  std::uint32_t mode,std::uint32_t frameMode,std::uint32_t strategy){
    return fgMenuMatches(p,revision,provider,mode)&&p.frameMode==frameMode&&p.strategy==strategy;
}
constexpr bool fgMenuSourceMatches(const FgMenuProjection& p,std::uint32_t revision,std::uint32_t provider,
                                  std::uint32_t mode,std::uint32_t frameMode,std::uint32_t strategy,
                                  std::uint32_t dlssGenerated,std::uint32_t fsrGenerated){
    return fgMenuSourceMatches(p,revision,provider,mode,frameMode,strategy)&&
        p.dlssGenerated==dlssGenerated&&p.fsrGenerated==fsrGenerated;
}
// Total-grid software support and the exact native-grid SDK count must both
// admit a custom allocation. Legacy mixed total masks cannot describe this.
inline bool fgMenuAllocationSupported(const FgMenuProjection& p,std::uint32_t softwareMask,
                                      std::uint32_t nativeGeneratedMax){
    const auto total=p.frameMode+2;
    if(!p.pairEligible || p.frameMode>4 || nativeGeneratedMax>5 ||
       p.dlssGenerated+p.fsrGenerated+1!=total ||
       (p.fsrGenerated && !(softwareMask&(1u<<total)))) return false;
    return bool(mcd2::mfg::make_allocation_plan(p.dlssGenerated,p.fsrGenerated,
        {p.fsrGenerated?total:0,nativeGeneratedMax?nativeGeneratedMax+1:0,nativeGeneratedMax!=0}));
}
// Keep fresh-Off startup native; a multiplier never reserves a presentation owner.
constexpr std::uint32_t fgStartupOwner(const FgMenuProjection& selection){
    return selection.pairEligible&&selection.provider<2&&
        selection.enabled?((selection.strategy==2||selection.strategy==3)?4:selection.provider+1):0;
}
constexpr bool fgOwnerMatches(std::uint32_t owner,std::uint32_t provider,std::uint32_t strategy=0){
    return provider<2&&owner==((strategy==2||strategy==3)?4:provider+1);
}
// A matching startup presenter cannot revive a permanently retired FG session.
// Temporary menu suspension does not retire it; AMD live Off/On retains it.
constexpr bool fgMenuRestartRequired(bool available,bool enabled,std::uint32_t owner,
                                     std::uint32_t provider,bool retired,std::uint32_t strategy=0){
    return available&&(retired||(enabled?!fgOwnerMatches(owner,provider,strategy):owner!=0));
}
// A verified engine resize may already have restored AMD's native Off route.
// This acknowledges that completed Off transition only; it never admits On,
// revives a retired feature or changes NVIDIA's restart requirement.
constexpr bool fgMenuNativeOffRecovered(bool currentRequest,bool available,bool enabled,
                                        std::uint32_t provider,std::uint32_t route,
                                        std::uint32_t canSwitch,std::uint32_t fault,bool retired,std::uint32_t strategy=0){
    return currentRequest&&available&&!enabled&&(provider==1||strategy==2||strategy==3)&&route==1&&canSwitch==1&&fault==0&&!retired;
}
} // namespace mcd2::providers
