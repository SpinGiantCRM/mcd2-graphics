#pragma once
#include <cstdint>

namespace mcd2::providers {
// ReShade presentation encodings, not the HDR preference or an SDK version.
enum class FgOutputColour : std::uint32_t { Unknown=0, Srgb=1, ScRgb=2, Pq2020=3, Hlg2020=4 };
enum class FgSceneStage : std::uint32_t { HdrCompositor=1, SdrOpaqueSnapshot=2 };
struct FgOutputContract {
    std::uint32_t width=0,height=0,backbufferFormat=0,sceneFormat=0;
    FgOutputColour colour=FgOutputColour::Unknown;
    FgSceneStage stage=FgSceneStage::HdrCompositor;
    bool operator==(const FgOutputContract&) const = default;
};
constexpr bool validFgOutput(const FgOutputContract& p) {
    if(!p.width||!p.height||p.width>7680||p.height>4320)return false;
    if(p.colour==FgOutputColour::Pq2020)
        return p.stage==FgSceneStage::HdrCompositor&&p.backbufferFormat==24&&p.sceneFormat==24;
    if(p.colour==FgOutputColour::Srgb)
        return p.stage==FgSceneStage::SdrOpaqueSnapshot&&
            ((p.backbufferFormat==p.sceneFormat&&(p.sceneFormat==24||p.sceneFormat==28))||
             (p.backbufferFormat==87&&p.sceneFormat==28));
    // No game producer/encoding proof yet for scRGB or HLG.
    return false;
}
constexpr bool fgOutputHdr(const FgOutputContract& p) {
    return validFgOutput(p)&&p.colour==FgOutputColour::Pq2020;
}
// A pre-Reset notification is not evidence that the old recording is gone.
// GPU completion remains a separate requirement after every lease retires.
struct FgSceneReset {
    bool pending=false,known=false;std::uint64_t before=0;
    void request(bool valid,std::uint64_t epoch){pending=true;known=valid;before=epoch;}
    constexpr bool confirmed(bool valid,std::uint64_t epoch)const{
        return pending&&known&&valid&&epoch!=UINT64_MAX&&epoch>before;
    }
};
struct SdrSceneDraw {
    std::uint64_t frame=UINT64_MAX;
    std::uint32_t shader=0,elements=0,instances=0,writeMask=0;
    bool indexed=false,blendKnown=false,blendEnabled=false;
    bool primaryBackbuffer=false,viewportKnown=false,scissorKnown=false;
    float x=0,y=0,width=0,height=0;
    std::int32_t left=0,top=0,right=0,bottom=0;
};
// Steam 25754144 SDR tone-map permutations, captured with game sharpening
// disabled/enabled. Both retain the same View b1 layout and two game outputs.
// The caller executes the original draw, including all outputs, before copying
// the qualified primary backbuffer; this is not a shader replacement.
constexpr bool qualifiedSdrSceneShader(std::uint32_t shader) {
    return shader==0x088c6818u||shader==0xf7e35ae7u;
}
constexpr bool admitSdrSceneDraw(const FgOutputContract& output,const SdrSceneDraw& d) {
    return validFgOutput(output)&&output.colour==FgOutputColour::Srgb&&
        d.frame<UINT32_MAX&&qualifiedSdrSceneShader(d.shader)&&d.indexed&&d.elements==3&&d.instances==1&&
        d.blendKnown&&!d.blendEnabled&&(d.writeMask&7u)==7u&&d.primaryBackbuffer&&
        d.viewportKnown&&d.scissorKnown&&d.x==0&&d.y==0&&
        d.width==float(output.width)&&d.height==float(output.height)&&
        d.left==0&&d.top==0&&d.right==std::int32_t(output.width)&&d.bottom==std::int32_t(output.height);
}
} // namespace mcd2::providers
