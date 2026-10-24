#pragma once
#include <array>
#include <cstdint>
#include <optional>

// Compute scheduling only. No vendor/model whitelist, presenter or runtime
// qualification claim lives here. Capabilities are supplied by an admitted host.
namespace mcd2::mfg {
enum class Mode : unsigned { Software, Native, Hybrid };
enum class Provider : unsigned { Software, NvidiaNative };
struct Capabilities {
    unsigned softwareMaxMultiplier{}, nativeMaxMultiplier{};
    bool nativeCallerOwnedOutput{};
};
struct Phase { unsigned numerator{},denominator{}; Provider provider{}; };
struct Plan { Mode mode{}; unsigned multiplier{},count{}; std::array<Phase,5> phases{}; };
inline std::optional<Plan> make_plan(Mode mode,unsigned multiplier,Capabilities caps) {
    if(multiplier<2 || multiplier>6) return {};
    if(mode==Mode::Software) {
        if(caps.softwareMaxMultiplier<multiplier) return {};
    } else if(mode==Mode::Native) {
        if(!caps.nativeCallerOwnedOutput || caps.nativeMaxMultiplier<multiplier) return {};
    } else if(mode==Mode::Hybrid) {
        if((multiplier!=4 && multiplier!=6) || !caps.nativeCallerOwnedOutput ||
           caps.nativeMaxMultiplier<2 || caps.softwareMaxMultiplier<multiplier) return {};
    } else return {};
    Plan plan{mode,multiplier,multiplier-1,{}};
    for(unsigned i=0;i<plan.count;++i) {
        auto provider=mode==Mode::Native || (mode==Mode::Hybrid && (i+1)*2==multiplier)
            ? Provider::NvidiaNative : Provider::Software;
        plan.phases[i]={i+1,multiplier,provider};
    }
    return plan;
}
enum class Origin : unsigned { Real, Generated };
struct RealPair {
    uint64_t epoch{},previous{},current{};
    Origin previousOrigin{Origin::Real}, currentOrigin{Origin::Real};
    bool operator==(const RealPair&) const = default;
};
class Batch {
    Plan plan_{};
    RealPair pair_{};
    std::array<uint64_t,5> outputs_{};
    unsigned recorded_{};
    bool active_{},valid_{};
public:
    void invalidate() { active_=valid_=false; recorded_=0; outputs_={}; }
    bool begin(const Plan& plan,RealPair pair) {
        if(active_ || !pair.epoch || !pair.previous || pair.current<=pair.previous ||
           pair.previousOrigin!=Origin::Real || pair.currentOrigin!=Origin::Real) return false;
        const auto expected=make_plan(plan.mode,plan.multiplier,{6,6,true});
        if(!expected || plan.count!=expected->count) return false;
        for(unsigned i=0;i<plan.count;++i) {
            const auto& a=plan.phases[i]; const auto& b=expected->phases[i];
            if(a.numerator!=b.numerator || a.denominator!=b.denominator || a.provider!=b.provider) return false;
        }
        plan_=plan; pair_=pair; outputs_={}; recorded_=0; active_=valid_=true; return true;
    }
    bool record(RealPair pair,unsigned slot,Provider provider,uint64_t output,bool evaluationSucceeded) {
        if(!active_ || !valid_) return false;
        if(pair!=pair_ || !evaluationSucceeded || !output || slot!=recorded_+1 ||
           slot>plan_.count || provider!=plan_.phases[slot-1].provider) {
            valid_=false; return false;
        }
        for(unsigned i=0;i<recorded_;++i) if(outputs_[i]==output) { valid_=false; return false; }
        outputs_[recorded_++]=output; return true;
    }
    // The owner must separately prove queue completion before consuming images.
    bool complete() const { return active_ && valid_ && recorded_==plan_.count; }
};
}
